#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "AetherIcons.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <map>

namespace aether
{

	namespace glass { inline float frost = 0.30f; }

	namespace ui
	{

		inline bool animations_enabled = true;

		struct motion_slot_t { float v; int frame; };

		inline std::map<ImGuiID, motion_slot_t>& motion_slots()
		{
			static std::map<ImGuiID, motion_slot_t> s;
			return s;
		}

		inline float& motion_slot(ImGuiID id, unsigned salt, float init)
		{
			std::map<ImGuiID, motion_slot_t>& s = motion_slots();
			const int frame = ImGui::GetFrameCount();
			const ImGuiID key = id ^ (ImGuiID)(salt * 0x9e3779b9u);
			auto it = s.find(key);
			if (it == s.end())
			{
				if (s.size() > 2048)
					for (auto i = s.begin(); i != s.end(); )
						i = (frame - i->second.frame > 600) ? s.erase(i) : ++i;
				it = s.emplace(key, motion_slot_t{ init, frame }).first;
			}
			it->second.frame = frame;
			return it->second.v;
		}

		inline float motion_dt() { return ImMin(ImGui::GetIO().DeltaTime, 1.f / 20.f); }

		inline float ease(float cur, float tgt, float half_life)
		{
			if (!animations_enabled || half_life <= 0.0001f)
				return tgt;
			const float k = 1.f - powf(0.5f, motion_dt() / half_life);
			return cur + (tgt - cur) * k;
		}

		inline float anim(ImGuiID id, unsigned salt, float tgt, float init, float half_life = 0.06f)
		{
			float& v = motion_slot(id, salt, init);
			v = ease(v, tgt, half_life);
			return v;
		}

		inline float pulse(ImGuiID id, unsigned salt, bool on, float decay = 3.f)
		{
			float& v = motion_slot(id, salt, 0.f);
			if (on) v = 1.f;
			else    v = ImMax(0.f, v - motion_dt() * decay * (animations_enabled ? 1.f : 12.f));
			return v;
		}

		inline ImU32 rgba(const ImVec4& c, float a)
		{
			ImVec4 t = c; t.w = a;
			return ImGui::GetColorU32(t);
		}

		inline ImU32 with_alpha(ImU32 c, float a)
		{
			return (c & 0x00FFFFFFu) | ((ImU32)(ImClamp(a, 0.f, 1.f) * 255.f + 0.5f) << 24);
		}

		inline ImU32 blend_u32(ImU32 a, ImU32 b, float t)
		{
			t = ImClamp(t, 0.f, 1.f);
			const int ac[4] = { (int)((a >> 24) & 0xFF), (int)((a >> 16) & 0xFF), (int)((a >> 8) & 0xFF), (int)(a & 0xFF) };
			const int bc[4] = { (int)((b >> 24) & 0xFF), (int)((b >> 16) & 0xFF), (int)((b >> 8) & 0xFF), (int)(b & 0xFF) };
			return ((ImU32)(ac[0] + (bc[0] - ac[0]) * t) << 24) | ((ImU32)(ac[1] + (bc[1] - ac[1]) * t) << 16)
				 | ((ImU32)(ac[2] + (bc[2] - ac[2]) * t) << 8) | (ImU32)(ac[3] + (bc[3] - ac[3]) * t);
		}

		inline ImU32 lighten_u32(ImU32 c, float t) { return blend_u32(c, IM_COL32(255, 255, 255, (int)((c >> 24) & 0xFF)), t); }
		inline ImU32 darken_u32(ImU32 c, float t)  { return blend_u32(c, IM_COL32(0, 0, 0, (int)((c >> 24) & 0xFF)), t); }

		inline ImU32 accent_u32(float a = 1.f)  { return ImGui::GetColorU32(ImGuiCol_SliderGrab, a); }
		inline ImU32 text_u32(float a = 1.f)    { return ImGui::GetColorU32(ImGuiCol_Text, a); }
		inline ImU32 dim_u32(float a = 1.f)     { return ImGui::GetColorU32(ImGuiCol_TextDisabled, a); }
		inline ImU32 surface_u32(float a = 1.f) { return ImGui::GetColorU32(ImGuiCol_ChildBg, a); }
		inline ImU32 frame_u32(float a = 1.f)   { return ImGui::GetColorU32(ImGuiCol_FrameBg, a); }
		inline ImU32 border_u32(float a = 1.f)  { return ImGui::GetColorU32(ImGuiCol_Border, a); }

		inline void plate(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 top, ImU32 bot, float rounding)
		{
			const float w = b.x - a.x;
			const float h = b.y - a.y;
			if (w <= 0.0f || h <= 0.0f)
				return;

			const float r = ImClamp(rounding, 0.0f, ImMin(w, h) * 0.5f);
			if (r <= 0.5f)
			{
				dl->AddRectFilledMultiColor(a, b, top, top, bot, bot);
				return;
			}

			const int   bands = (int)ImClamp(ImCeil(h * 0.5f), 2.0f, 10.0f);
			const float bh    = h / (float)bands;
			for (int i = 0; i < bands; ++i)
			{
				const float y0 = a.y + bh * (float)i;
				const float y1 = (i == bands - 1) ? b.y : y0 + bh;
				const ImU32 c  = blend_u32(top, bot, (float)i / (float)(bands - 1));

				const float dt = y0 - a.y;
				const float db = b.y - y1;
				float ins = 0.0f;
				if (dt < r) ins = ImMax(ins, r - sqrtf(ImMax(0.0f, r * r - (r - dt) * (r - dt))));
				if (db < r) ins = ImMax(ins, r - sqrtf(ImMax(0.0f, r * r - (r - db) * (r - db))));

				if (b.x - ins - (a.x + ins) < 0.5f)
					continue;
				dl->AddRectFilled(ImVec2(a.x + ins, y0), ImVec2(b.x - ins, y1), c);
			}
		}

		inline void halo(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding,
			float amount, int rings = 3, float spread = 1.f)
		{
			if (amount <= 0.004f)
				return;
			for (int i = rings; i >= 1; --i)
			{
				const float e = (float)i * spread;
				dl->AddRect(ImVec2(a.x - e, a.y - e), ImVec2(b.x + e, b.y + e),
					with_alpha(col, amount / (float)i), rounding + e, 0, 1.f);
			}
		}

		inline void frost_face(ImDrawList* dl, const ImVec2& a, const ImVec2& b,
			float rounding, float amount, unsigned seed = 0u)
		{
			amount = ImClamp(amount, 0.f, 1.f);
			const float w = b.x - a.x, h = b.y - a.y;
			if (amount <= 0.01f || w < 6.f || h < 6.f)
				return;

			dl->PushClipRect(ImVec2(a.x + 1.f, a.y + 1.f), ImVec2(b.x - 1.f, b.y - 1.f), true);
			{
				unsigned s = (seed * 0x9e3779b9u) ^ 0x5bf03635u;
				if (s == 0u) s = 0x2545F491u;

				const auto rnd = [&s]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; };

				const int count = (int)ImClamp(w * h / 900.f * amount, 0.f, 700.f);
				const ImU32 dim = IM_COL32(0, 0, 0, (int)(5.f + 12.f * amount));
				for (int i = 0; i < count; ++i)
				{
					const float x = a.x + 2.f + (float)(rnd() % 99991u) / 99991.f * (w - 4.f);
					const float y = a.y + 2.f + (float)(rnd() % 99991u) / 99991.f * (h - 4.f);
					if (rnd() & 1u)
						dl->AddRectFilled(ImVec2(x, y), ImVec2(x + 1.f, y + 1.f), dim);
				}
			}
			dl->PopClipRect();

			const float r = ImClamp(rounding, 0.f, ImMin(w, h) * 0.5f);

			{
				const float inset = ImMax(r * 0.6f, 1.f);
				const float x0 = a.x + inset, x1 = b.x - inset;
				if (x1 > x0 + 2.f)
				{
					const float band = ImMin(h * 0.55f, h - 2.f);
					const int   hi   = (int)(9.f + 15.f * amount);
					dl->AddRectFilledMultiColor(ImVec2(x0, a.y + 1.f), ImVec2(x1, a.y + band),
						IM_COL32(0, 0, 0, hi), IM_COL32(0, 0, 0, hi),
						IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

					const float bs = ImMin(h * 0.32f, h - 2.f);
					const int   lo = (int)(7.f + 16.f * amount);
					if (b.y - bs > a.y + band)
						dl->AddRectFilledMultiColor(ImVec2(x0, b.y - bs), ImVec2(x1, b.y - 1.f),
							IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
							IM_COL32(0, 0, 0, lo), IM_COL32(0, 0, 0, lo));
				}
			}
		}

		inline ImVec4 card_tint()
		{

			const ImVec4 sfc = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
			return ImVec4(sfc.x * 0.30f, sfc.y * 0.30f, sfc.z * 0.30f, 0.20f);
		}

		inline int& scroll_motion_frame()
		{
			static int f = -1;
			return f;
		}
		inline bool scrolling_now()
		{
			return scroll_motion_frame() == ImGui::GetFrameCount();
		}

		struct scroll_state_t { float target; float applied; int frame; };

		inline std::map<ImGuiID, scroll_state_t>& scroll_states()
		{
			static std::map<ImGuiID, scroll_state_t> s;
			return s;
		}

		inline float wheel_step(ImGuiWindow* w)
		{
			return ImTrunc(ImMin(5.f * w->CalcFontSize(), w->InnerRect.GetHeight() * 0.67f));
		}

		inline bool window_inside(ImGuiWindow* child, ImGuiWindow* ancestor)
		{
			for (ImGuiWindow* w = child; w; w = w->ParentWindow)
				if (w == ancestor)
					return true;
			return false;
		}

		inline bool wheel_is_ours(ImGuiWindow* w)
		{
			ImGuiIO& io = ImGui::GetIO();
			if (io.MouseWheel == 0.f)
				return false;

			if (ImGui::IsPopupOpen((ImGuiID)0, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
				return false;

			ImGuiWindow* hovered = GImGui->HoveredWindow;
			if (!hovered)
				return false;

			ImGuiWindow* root = w->RootWindow ? w->RootWindow : w;
			if (!window_inside(hovered, root))
				return false;

			for (ImGuiWindow* p = hovered; p && p != root; p = p->ParentWindow)
			{
				if (p == w || window_inside(w, p))
					break;
				if (p->ScrollMax.y > 1.f)
					return false;
			}

			if (ImGui::IsMouseHoveringRect(w->Pos, w->Pos + w->Size))
				return true;

			ImGuiWindow* parent = w->ParentWindow;
			if (!parent)
				return false;
			return ImGui::IsMouseHoveringRect(parent->Pos, parent->Pos + parent->Size, false);
		}

		inline int& wheel_claimed_frame()
		{
			static int f = -1;
			return f;
		}
		inline bool take_stray_wheel(ImGuiWindow* w)
		{
			const int frame = ImGui::GetFrameCount();
			if (wheel_claimed_frame() == frame || !wheel_is_ours(w))
				return false;
			wheel_claimed_frame() = frame;
			return true;
		}

		inline void smooth_scroll(bool enabled, float speed = 14.f)
		{
			ImGuiWindow* w = ImGui::GetCurrentWindow();
			if (!w || w->SkipItems)
				return;

			for (ImGuiWindow* p = w->ParentWindow; p; p = p->ParentWindow)
				if (p->Scroll.y != 0.f)
					ImGui::SetScrollY(p, 0.f);

			std::map<ImGuiID, scroll_state_t>& s = scroll_states();
			const int frame = ImGui::GetFrameCount();
			auto it = s.find(w->ID);
			if (it == s.end())
			{
				if (s.size() > 256)
					for (auto i = s.begin(); i != s.end(); )
						i = (frame - i->second.frame > 240) ? s.erase(i) : ++i;
				it = s.emplace(w->ID, scroll_state_t{ ImGui::GetScrollY(), ImGui::GetScrollY(), frame }).first;
			}
			scroll_state_t& st = it->second;
			st.frame = frame;

			const float raw  = ImGui::GetScrollY();
			const float maxy = ImGui::GetScrollMaxY();
			float delta = raw - st.applied;

			if (ImAbs(delta) < 1.f)
				delta = 0.f;
			if (delta != 0.f)
				st.target += delta;
			else if (take_stray_wheel(w))
				st.target -= ImGui::GetIO().MouseWheel * wheel_step(w);

			st.target = ImFloor(ImClamp(st.target, 0.f, ImMax(maxy, raw)));

			if (!enabled)
			{
				st.target = st.applied = raw;
				return;
			}

			const float t = ImMin(1.f, ImGui::GetIO().DeltaTime * ImClamp(speed, 1.f, 60.f));
			st.applied += (st.target - st.applied) * t;
			if (ImAbs(st.applied - st.target) < 0.5f)
				st.applied = st.target;
			if (st.applied != raw)
				ImGui::SetScrollY(ImFloor(st.applied));

			if (ImAbs(st.applied - st.target) > 0.5f)
				scroll_motion_frame() = frame;
		}

		inline void reset_scroll()
		{
			ImGuiWindow* w = ImGui::GetCurrentWindow();
			if (!w)
				return;
			ImGui::SetScrollY(0.f);
			scroll_state_t& st = scroll_states()[w->ID];
			st.target = st.applied = 0.f;
			st.frame = ImGui::GetFrameCount();
		}

		inline void tooltip(const char* fmt, ...)
		{
			if (scrolling_now())
				return;
			va_list args;
			va_start(args, fmt);
			ImGui::SetTooltipV(fmt, args);
			va_end(args);
		}
	}
}

namespace widgets
{

	inline void text_outlined(ImDrawList* draw, ImVec2 pos, ImU32 col, const char* text)
	{
		if (!draw || !text)
			return;

		ImU32 outline = IM_COL32(0, 0, 0, 220);
		const float o = 1.f;
		draw->AddText(ImVec2(pos.x - o, pos.y), outline, text);
		draw->AddText(ImVec2(pos.x + o, pos.y), outline, text);
		draw->AddText(ImVec2(pos.x, pos.y - o), outline, text);
		draw->AddText(ImVec2(pos.x, pos.y + o), outline, text);
		draw->AddText(pos, col, text);
	}

	inline bool slider_float(const char* label, float* value, float min, float max, const char* fmt = "%.1f")
	{
		using namespace aether::ui;

		constexpr float bar_height = 9.f;
		constexpr float knob_r     = (bar_height * 0.5f) + 2.f;
		constexpr float gap        = 4.f;

		ImGui::PushID(label);

		ImVec2 pos = ImGui::GetCursorScreenPos();
		float width = ImGui::CalcItemWidth();
		if (width < 1.f)
			width = ImGui::GetContentRegionAvail().x;

		{
			const float avail = ImGui::GetContentRegionAvail().x;
			if (avail > 24.f && width > avail)
				width = avail;
		}
		width = ImMax(width, 32.f);
		ImVec2 text_size = ImGui::CalcTextSize(label);

		char value_buf[64];
		snprintf(value_buf, sizeof(value_buf), fmt ? fmt : "%.2f", *value);
		ImVec2 value_size = ImGui::CalcTextSize(value_buf);

		int prec = 0;
		if (fmt)
		{
			const char* dot = strchr(fmt, '.');
			if (dot)
			{
				int n = 0;
				for (const char* p = dot + 1; *p >= '0' && *p <= '9'; ++p)
					n = n * 10 + (*p - '0');
				prec = ImClamp(n, 0, 4);
			}
		}
		const float chip_h = text_size.y + 2.f;
		const float chip_w = value_size.x + 10.f;
		const float row_h  = ImMax(text_size.y, chip_h);

		const float pad = 6.f;

		ImVec2 bar_min(pos.x + pad + knob_r, ImFloor(pos.y + pad + row_h + gap + knob_r));
		ImVec2 bar_max(ImFloor(pos.x + width - pad - knob_r), bar_min.y + bar_height);
		if (bar_max.x - bar_min.x < 8.f)
			bar_max.x = bar_min.x + 8.f;
		float total_h = (bar_max.y + knob_r + pad) - pos.y;

		ImGui::InvisibleButton("##sl", ImVec2(width, total_h));
		const ImGuiID id = ImGui::GetItemID();
		bool active  = ImGui::IsItemActive();
		bool hovered = ImGui::IsItemHovered();
		bool changed = false;

		if (active && max > min)
		{
			float nt = (ImGui::GetIO().MousePos.x - bar_min.x) / ImMax(1.f, bar_max.x - bar_min.x);
			nt = ImClamp(nt, 0.f, 1.f);
			float new_v = min + nt * (max - min);
			const float sc = powf(10.f, (float)prec);
			if (sc > 0.f)
				new_v = ImFloor(new_v * sc + 0.5f) / sc;
			new_v = ImClamp(new_v, min, max);
			if (new_v != *value)
			{
				*value = new_v;
				changed = true;
			}
		}

		float t = (max > min) ? ((*value - min) / (max - min)) : 0.f;
		t = ImClamp(t, 0.f, 1.f);

		const float lit     = anim(id, 11, (hovered || active) ? 1.f : 0.f, 0.f, 0.05f);
		const float shown_t = anim(id, 12, t, t, active ? 0.03f : 0.06f);
		const float knob_sc = anim(id, 13, active ? 1.f : hovered ? 0.92f : 0.72f, 0.72f, 0.05f);

		const float fill_x = bar_min.x + (bar_max.x - bar_min.x) * shown_t;

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImU32 accent = accent_u32();
		const float r = bar_height * 0.5f;

		{
			const ImVec2 bx_a(pos.x, pos.y);
			const ImVec2 bx_b(pos.x + width, pos.y + total_h);
			const float  bx_r = 6.f;
			plate(draw, bx_a, bx_b, darken_u32(frame_u32(), 0.30f), frame_u32(), bx_r);
			draw->AddRect(bx_a, bx_b, blend_u32(border_u32(), accent, lit * 0.45f), bx_r, 0, 1.f);
		}

		const ImVec2 inner(pos.x + pad, pos.y + pad);
		draw->PushClipRect(inner, ImVec2(pos.x + width - pad - chip_w - 4.f, inner.y + row_h), true);
		ImGui::RenderText(ImVec2(inner.x, ImFloor(inner.y + (row_h - text_size.y) * 0.5f)), label);
		draw->PopClipRect();

		const bool chip_fits = (chip_w + 24.f < width);
		if (chip_fits)
		{
			const ImVec2 chip_a(pos.x + width - pad - chip_w, pos.y + pad);
			const ImVec2 chip_b(pos.x + width - pad, pos.y + pad + chip_h);
			plate(draw, chip_a, chip_b,
				blend_u32(surface_u32(), accent, 0.16f + 0.16f * lit),
				blend_u32(frame_u32(), accent, 0.12f + 0.14f * lit),
				chip_h * 0.5f);
			draw->AddRect(chip_a, chip_b, with_alpha(accent, 0.20f + 0.45f * lit), chip_h * 0.5f);
		}

		draw->AddText(ImVec2(ImFloor(pos.x + width - pad - 5.f - value_size.x),
				ImFloor(pos.y + pad + (row_h - value_size.y) * 0.5f)),
			blend_u32(dim_u32(), text_u32(), ImMax(lit, 0.35f)), value_buf);

		plate(draw, bar_min, bar_max, darken_u32(frame_u32(), 0.42f), frame_u32(), r);

		for (int i = 1; i <= 3; ++i)
		{
			const float mx = ImFloor(bar_min.x + (bar_max.x - bar_min.x) * (float)i * 0.25f);
			const bool  on = (shown_t >= (float)i * 0.25f);
			draw->AddRectFilled(ImVec2(mx, bar_min.y + 2.f), ImVec2(mx + 1.f, bar_max.y - 2.f),
				on ? IM_COL32(0, 0, 0, 90) : IM_COL32(255, 255, 255, 38));
		}

		const float fill_w = fill_x - bar_min.x;
		if (fill_w > 1.f)
		{
			const float fr = ImMin(r, fill_w * 0.5f + 0.5f);
			plate(draw, bar_min, ImVec2(fill_x, bar_max.y), lighten_u32(accent, 0.26f), accent, fr);
			if (fill_w > 3.f)
				draw->AddRectFilled(ImVec2(bar_min.x + 1.f, bar_min.y + 1.f), ImVec2(fill_x - 1.f, bar_min.y + 2.f),
					IM_COL32(255, 255, 255, 36), 1.f);
		}

		const float kr = knob_r * knob_sc;
		const ImVec2 kc(ImFloor(fill_x), (bar_min.y + bar_max.y) * 0.5f);
		draw->AddCircleFilled(ImVec2(kc.x, kc.y + 1.f), kr, IM_COL32(0, 0, 0, 80), 28);
		draw->AddCircleFilled(kc, kr, accent, 32);
		draw->AddCircleFilled(ImVec2(kc.x, kc.y - kr * 0.22f), kr * 0.42f, lighten_u32(accent, 0.85f), 24);

		draw->AddRect(bar_min, bar_max, blend_u32(border_u32(), accent, lit * 0.65f), r, 0, 1.f);

		ImGui::PopID();
		return changed;
	}

	inline bool slider_int(const char* label, int* value, int min, int max)
	{
		float f = (float)*value;
		bool changed = slider_float(label, &f, (float)min, (float)max, "%.0f");
		*value = (int)(f + (f >= 0.f ? 0.5f : -0.5f));
		if (*value < min) *value = min;
		if (*value > max) *value = max;
		return changed;
	}
}

namespace aether
{
	namespace ui
	{

		inline float anim_to(ImGuiID id, float target, float speed)
		{
			return anim(id, 900u, target, target, 0.693f / ImMax(0.01f, speed));
		}

		inline bool nav_item(const char* id, icons::Id icon, const char* label, bool active, bool show_label)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			ImGuiStyle& style = ImGui::GetStyle();
			const ImGuiID uid = window->GetID(id);

			const bool  with_label = show_label && label && label[0];
			const float icon_sz  = ImGui::GetFontSize() + 5.f;
			const float label_w  = with_label ? ImGui::CalcTextSize(label).x : 0.f;
			const float pad_x    = with_label ? style.WindowPadding.x : style.WindowPadding.x * 0.7f;
			const float w = pad_x * 2.f + icon_sz + (with_label ? style.ItemInnerSpacing.x + label_w : 0.f);
			const float h = icon_sz + style.WindowPadding.y * 1.5f;

			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
			ImGui::ItemSize(ImVec2(w, h), style.FramePadding.y);
			if (!ImGui::ItemAdd(bb, uid))
				return false;

			bool hov = false, held = false;
			const bool pressed = ImGui::ButtonBehavior(bb, uid, &hov, &held);

			const float lit = anim_to(uid, (hov || active) ? 1.f : 0.f, 13.f);

			ImDrawList* dl = window->DrawList;
			const ImVec4 accent  = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
			const ImVec4 surface = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
			const float  rnd = style.FrameRounding + 2.f;

			dl->AddRectFilled(bb.Min, bb.Max, rgba(surface, 0.28f + 0.34f * lit), rnd);

			if (active)
			{
				dl->AddRectFilledMultiColor(bb.Min, bb.Max,
					rgba(accent, 0.02f), rgba(accent, 0.02f), rgba(accent, 0.30f), rgba(accent, 0.30f));
				for (int i = 2; i >= 1; --i)
					dl->AddRect(ImVec2(bb.Min.x - (float)i, bb.Min.y - (float)i),
						ImVec2(bb.Max.x + (float)i, bb.Max.y + (float)i),
						rgba(accent, 0.13f / (float)i), rnd + (float)i);
				dl->AddRect(bb.Min, bb.Max, rgba(accent, 0.55f), rnd);
			}
			else if (lit > 0.01f)
			{
				dl->AddRect(bb.Min, bb.Max, rgba(accent, 0.24f * lit), rnd);
			}

			ImVec4 ico_col = ImGui::GetStyleColorVec4(active ? ImGuiCol_Text : ImGuiCol_TextDisabled);
			if (active) ico_col = ImLerp(ico_col, accent, 0.85f);
			else        ico_col = ImLerp(ico_col, ImGui::GetStyleColorVec4(ImGuiCol_Text), lit);

			const float cy = (bb.Min.y + bb.Max.y) * 0.5f;
			float cx = with_label ? (bb.Min.x + pad_x + icon_sz * 0.5f) : ((bb.Min.x + bb.Max.x) * 0.5f);
			icons::draw(dl, icon, ImVec2(cx, cy), icon_sz, ImGui::GetColorU32(ico_col), 0.f);

			if (with_label)
			{
				const ImVec2 ts = ImGui::CalcTextSize(label);
				dl->AddText(ImVec2(bb.Min.x + pad_x + icon_sz + style.ItemInnerSpacing.x, cy - ts.y * 0.5f),
					ImGui::GetColorU32(ico_col), label);
			}
			else if (hov && label && label[0])
			{
				ImGui::SetTooltip("%s", label);
			}

			return pressed;
		}

		inline float rail_row_height()
		{
			return ImGui::GetFontSize() + 4.f + ImGui::GetStyle().FramePadding.y * 2.4f;
		}

		inline void rail_selection(const ImVec2& a, const ImVec2& b, float live, float rounding)
		{
			if (live <= 0.01f)
				return;
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);

			dl->AddRectFilled(a, b, rgba(accent, 0.10f * live), rounding);
			dl->AddRectFilled(a, b, rgba(accent, 0.14f * live), rounding);
			dl->AddRect(a, b, rgba(accent, 0.45f * live), rounding);
		}

		inline bool rail_item(const char* id, icons::Id icon, const char* label, bool active,
			const ImVec2& size = ImVec2(0, 0), bool draw_plate = true)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			ImGuiStyle& style = ImGui::GetStyle();
			const ImGuiID uid = window->GetID(id);

			const float icon_sz = ImGui::GetFontSize() + 4.f;
			const float h = size.y > 0.f ? size.y : icon_sz + style.FramePadding.y * 2.4f;
			const float w = size.x > 0.f ? size.x : ImGui::GetContentRegionAvail().x;

			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
			ImGui::ItemSize(ImVec2(w, h), style.FramePadding.y);
			if (!ImGui::ItemAdd(bb, uid))
				return false;

			bool hov = false, held = false;
			const bool pressed = ImGui::ButtonBehavior(bb, uid, &hov, &held);

			const float lit  = anim_to(uid, (hov || (active && draw_plate)) ? 1.f : 0.f, 13.f);
			const float live = anim_to(uid ^ 0x9e3779b9u, (active && draw_plate) ? 1.f : 0.f, 11.f);

			ImDrawList* dl = window->DrawList;
			const ImVec4 accent  = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
			const ImVec4 surface = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
			const float  rnd = style.FrameRounding + 2.f;

			if (lit > 0.01f)
				dl->AddRectFilled(bb.Min, bb.Max, rgba(surface, 0.34f * lit), rnd);

			ImVec4 ico_col = ImGui::GetStyleColorVec4(active ? ImGuiCol_Text : ImGuiCol_TextDisabled);
			if (active) ico_col = ImLerp(ico_col, accent, 0.8f);
			else        ico_col = ImLerp(ico_col, ImGui::GetStyleColorVec4(ImGuiCol_Text), lit);

			const float cy = (bb.Min.y + bb.Max.y) * 0.5f;
			const float ix = bb.Min.x + style.WindowPadding.x + icon_sz * 0.5f;
			icons::draw(dl, icon, ImVec2(ix, cy), icon_sz, ImGui::GetColorU32(ico_col), 0.f);

			if (label && label[0])
			{
				const ImVec2 ts = ImGui::CalcTextSize(label);
				dl->AddText(ImVec2(ix + icon_sz * 0.5f + style.ItemInnerSpacing.x + 2.f, cy - ts.y * 0.5f),
					ImGui::GetColorU32(ico_col), label);
			}

			return pressed;
		}

		inline void pane_chrome(float header_h, float rounding, bool shadow, bool accent_glow, float glow_amount)
		{
			ImDrawList* dl  = ImGui::GetWindowDrawList();
			ImDrawList* bgl = ImGui::GetBackgroundDrawList();
			const ImVec2 pos = ImGui::GetWindowPos();
			const ImVec2 sz  = ImGui::GetWindowSize();
			const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
			const float  g = ImClamp(glow_amount, 0.f, 1.f);
			const float  hh = ImMin(header_h, sz.y);

			if (shadow)
			{
				for (int i = 6; i >= 1; --i)
				{
					const float e = (float)i * 2.4f;
					const int   a = (int)(12.f * (1.f - (float)i / 7.f)) + 3;
					bgl->AddRectFilled(ImVec2(pos.x - e, pos.y - e + 1.f),
						ImVec2(pos.x + sz.x + e, pos.y + sz.y + e + 2.f), IM_COL32(0, 0, 0, a), rounding + e);
				}
			}

			if (accent_glow && g > 0.01f)
			{
				for (int i = 5; i >= 1; --i)
					dl->AddRect(ImVec2(pos.x - (float)i, pos.y - (float)i),
						ImVec2(pos.x + sz.x + (float)i, pos.y + sz.y + (float)i),
						rgba(accent, 0.10f * g / (float)i), rounding + (float)i);
			}

			aether::ui::frost_face(dl, pos, ImVec2(pos.x + sz.x, pos.y + sz.y), rounding, aether::glass::frost,
				ImGui::GetCurrentWindowRead()->ID);			if (hh > 2.f)
			{

				const float inset = rounding * 0.6f;
				const float x0 = pos.x + inset;
				const float x1 = pos.x + sz.x - inset;
				if (x1 > x0)
					dl->AddRectFilledMultiColor(ImVec2(x0, pos.y + 1.f), ImVec2(x1, pos.y + hh - 1.f),
						IM_COL32(0, 0, 0, 40), IM_COL32(0, 0, 0, 40),
						IM_COL32(0, 0, 0, 5),  IM_COL32(0, 0, 0, 5));

				const float bar_h = 2.f;
				dl->AddRectFilled(ImVec2(pos.x + 1.f, pos.y + hh - 1.f - bar_h),
					ImVec2(pos.x + sz.x - 1.f, pos.y + hh - 1.f),
					rgba(accent, 0.80f), 0.f);
			}
		}

		inline void card_begin(const char* id, const ImVec2& size, bool auto_resize_y = true)
		{

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 9.f));
			ImGui::BeginChild(id, size,
				(auto_resize_y ? ImGuiChildFlags_AutoResizeY : ImGuiChildFlags_None));
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 p = ImGui::GetWindowPos();
			const ImVec2 s = ImGui::GetWindowSize();
			const float  r = ImGui::GetStyle().ChildRounding;

			const ImVec4 tint = card_tint();
			dl->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y),
				ImGui::GetColorU32(tint), r);

			if (s.x > 36.f)
			{
				const ImVec4 ac = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
				dl->AddRectFilledMultiColor(ImVec2(p.x + 8.f, p.y), ImVec2(p.x + s.x - 8.f, p.y + 1.f),
					rgba(ac, 0.38f), rgba(ac, 0.04f), rgba(ac, 0.04f), rgba(ac, 0.38f));
			}

			aether::ui::frost_face(dl, p, ImVec2(p.x + s.x, p.y + s.y), r, aether::glass::frost,
				ImGui::GetCurrentWindowRead()->ID ^ ImGui::GetID(id));
		}

		inline bool icon_button(const char* id, icons::Id icon, float side, bool active = false,
			const char* tooltip = nullptr, float icon_scale = 0.f)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			ImGuiStyle& style = ImGui::GetStyle();
			const ImGuiID uid = window->GetID(id);

			if (side <= 0.f) side = ImGui::GetFrameHeight();
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, ImVec2(pos.x + side, pos.y + side));
			ImGui::ItemSize(ImVec2(side, side), style.FramePadding.y);
			if (!ImGui::ItemAdd(bb, uid))
				return false;

			bool hov = false, held = false;
			const bool pressed = ImGui::ButtonBehavior(bb, uid, &hov, &held);

			const float lit = anim_to(uid, (hov || active) ? 1.f : 0.f, 14.f);

			ImDrawList* dl = window->DrawList;
			const ImVec4 accent  = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
			const ImVec4 surface = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
			const float  rnd = style.FrameRounding + 1.f;

			dl->AddRectFilled(bb.Min, bb.Max, rgba(surface, 0.34f + 0.36f * lit), rnd);
			if (active)
				dl->AddRectFilled(bb.Min, bb.Max, rgba(accent, 0.22f), rnd);
			dl->AddRect(bb.Min, bb.Max, rgba(accent, (active ? 0.55f : 0.18f) * (0.4f + lit)), rnd);

			ImVec4 ico_col = ImGui::GetStyleColorVec4(active ? ImGuiCol_Text : ImGuiCol_TextDisabled);
			if (active) ico_col = ImLerp(ico_col, accent, 0.8f);
			else        ico_col = ImLerp(ico_col, ImGui::GetStyleColorVec4(ImGuiCol_Text), lit);

			const float isz = icon_scale > 0.f ? side * icon_scale : side * 0.52f;
			icons::draw(dl, icon, ImVec2((bb.Min.x + bb.Max.x) * 0.5f, (bb.Min.y + bb.Max.y) * 0.5f),
				isz, ImGui::GetColorU32(ico_col), 0.f);

			if (hov && tooltip && tooltip[0])
				ImGui::SetTooltip("%s", tooltip);

			return pressed;
		}

		inline bool segmented(const char* id, const char* const* labels, int count,
			int* current, float width)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems || count <= 0 || width < 24.f)
				return false;

			ImGuiStyle& style = ImGui::GetStyle();
			const ImGuiID uid = window->GetID(id);
			const float h = ImGui::GetFrameHeight() + 5.f;

			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, ImVec2(pos.x + width, pos.y + h));
			ImGui::ItemSize(bb, style.FramePadding.y);
			if (!ImGui::ItemAdd(bb, uid))
				return false;

			bool hov = false, held = false;
			const bool clicked = ImGui::ButtonBehavior(bb, uid, &hov, &held);

			const float seg = width / (float)count;
			const float mx  = ImGui::GetIO().MousePos.x;
			const int   hot = hov ? (int)ImClamp((mx - bb.Min.x) / seg, 0.f, (float)count - 0.001f) : -1;

			bool changed = false;
			if (clicked && hot >= 0 && hot != *current)
			{
				*current = hot;
				changed = true;
			}

			ImDrawList* dl = window->DrawList;
			const ImVec4 accent  = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
			const ImVec4 surface = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
			const ImVec4 dim     = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
			const float  rnd = style.FrameRounding + 3.f;

			dl->AddRectFilled(bb.Min, bb.Max, rgba(surface, 0.90f), rnd);
			dl->AddRectFilledMultiColor(ImVec2(bb.Min.x + rnd * 0.7f, bb.Min.y + 1.f),
				ImVec2(bb.Max.x - rnd * 0.7f, bb.Min.y + h * 0.45f),
				IM_COL32(0, 0, 0, 46), IM_COL32(0, 0, 0, 46), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
			dl->AddRect(bb.Min, bb.Max, with_alpha(border_u32(), 0.85f), rnd);

			if (hot >= 0 && hot != *current)
				dl->AddRectFilled(ImVec2(bb.Min.x + seg * (float)hot + 2.f, bb.Min.y + 2.f),
					ImVec2(bb.Min.x + seg * (float)(hot + 1) - 2.f, bb.Max.y - 2.f),
					rgba(accent, 0.10f), rnd - 2.f);

			const float pill_x = bb.Min.x + seg * (float)(*current);
			const float sx = anim(uid, 0xA15u, pill_x, pill_x, 0.075f);
			const ImVec2 pill_a(sx + 2.f, bb.Min.y + 2.f);
			const ImVec2 pill_b(sx + seg - 2.f, bb.Max.y - 2.f);
			if (pill_b.x > pill_a.x)
			{
				plate(dl, pill_a, pill_b, lighten_u32(with_alpha(accent_u32(), 0.92f), 0.10f),
					with_alpha(accent_u32(), 0.78f), rnd - 2.f);
				halo(dl, pill_a, pill_b, accent_u32(), rnd - 2.f, 0.30f, 3, 1.2f);
			}

			const float lum = accent.x * 0.299f + accent.y * 0.587f + accent.z * 0.114f;
			const ImVec4 on_accent = lum > 0.55f ? ImVec4(0.06f, 0.06f, 0.07f, 1.f) : ImVec4(0.99f, 0.99f, 1.f, 1.f);
			for (int i = 0; i < count; ++i)
			{
				const char* lab = labels[i] ? labels[i] : "";
				const ImVec2 ts = ImGui::CalcTextSize(lab);
				ImVec4 col = dim;
				if (i == *current)      col = on_accent;
				else if (i == hot)      col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
				const float tx = bb.Min.x + seg * ((float)i + 0.5f) - ts.x * 0.5f;
				dl->AddText(ImVec2(tx, (bb.Min.y + bb.Max.y - ts.y) * 0.5f), ImGui::GetColorU32(col), lab);
			}

			if (hot >= 0 && hot != *current && labels[hot])
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			return changed;
		}

		inline bool theme_swatch(const char* id, const char* name, const ImVec4& bg, const ImVec4& surface,
			const ImVec4& accent, const ImVec4& own_text, bool selected, const ImVec2& size)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			ImGuiStyle& style = ImGui::GetStyle();
			const ImGuiID uid = window->GetID(id);

			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
			ImGui::ItemSize(bb, style.FramePadding.y);
			if (!ImGui::ItemAdd(bb, uid))
				return false;

			bool hov = false, held = false;
			const bool pressed = ImGui::ButtonBehavior(bb, uid, &hov, &held);

			const float lift = anim(uid, 0x5C1u, (hov || selected) ? 1.f : 0.f, 0.f, 0.07f);
			ImDrawList* dl = window->DrawList;
			const float rnd = style.FrameRounding + 2.f;

			dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(bg), rnd);

			dl->PushClipRect(ImVec2(bb.Min.x + rnd * 0.7f, bb.Min.y + rnd * 0.7f),
				ImVec2(bb.Max.x - rnd * 0.7f, bb.Max.y - rnd * 0.7f), true);
			{
				const float w = size.x, h = size.y;
				dl->AddQuadFilled(ImVec2(bb.Max.x - w * 0.62f, bb.Max.y), ImVec2(bb.Max.x, bb.Max.y),
					ImVec2(bb.Max.x, bb.Max.y - h * 0.72f), ImVec2(bb.Max.x - w * 0.20f, bb.Max.y),
					with_alpha(ImGui::GetColorU32(accent), 0.85f));
			}
			dl->PopClipRect();

			dl->AddRectFilled(ImVec2(bb.Min.x + rnd * 0.5f, bb.Min.y + 1.f),
				ImVec2(bb.Max.x - rnd * 0.5f, bb.Min.y + ImMax(4.f, size.y * 0.20f)),
				ImGui::GetColorU32(surface));

			{
				const char* lab = name ? name : "";
				const ImVec2 ts = ImGui::CalcTextSize(lab);
				ImVec4 tc = own_text;
				if (ts.x > size.x - 8.f)
				{

					dl->PushClipRect(ImVec2(bb.Min.x + 3.f, bb.Min.y), ImVec2(bb.Max.x - 3.f, bb.Max.y), true);
					dl->AddText(ImVec2(bb.Min.x + 4.f, bb.Min.y + size.y * 0.34f), ImGui::GetColorU32(tc), lab);
					dl->PopClipRect();
				}
				else
				{
					dl->AddText(ImVec2(bb.Min.x + (size.x - ts.x) * 0.5f, bb.Min.y + size.y * 0.34f),
						ImGui::GetColorU32(tc), lab);
				}
			}

			const float ring = selected ? 0.95f : 0.16f + 0.34f * lift;
			dl->AddRect(bb.Min, bb.Max, with_alpha(ImGui::GetColorU32(accent), ring), rnd, 0, selected ? 2.f : 1.f);
			if (selected)
				halo(dl, bb.Min, bb.Max, ImGui::GetColorU32(accent), rnd, 0.32f, 3, 1.4f);

			return pressed;
		}
	}
}

namespace aether
{
	namespace glass
	{

		inline float window_alpha = 0.86f;
		inline float dock_alpha   = 0.78f;

		inline void apply(ImGuiStyle& s)
		{

			ImVec4& wb = s.Colors[ImGuiCol_WindowBg];
			wb.w = window_alpha;
			s.Colors[ImGuiCol_PopupBg] = wb;

			s.Colors[ImGuiCol_ChildBg].w = window_alpha;

		}
	}
}
