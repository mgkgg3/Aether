#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>
#include <features/misc/TextSheen.h>

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <map>
#include <vector>

namespace uifx
{

	inline bool on() { return settings::misc_ui::enabled; }

	inline float amount() { return ImClamp(settings::misc_ui::intensity, 0.f, 1.f); }

	inline float dt()
	{
		const float d = ImGui::GetIO().DeltaTime;
		return d > 0.f ? ImMin(d, 1.f / 20.f) : 1.f / 60.f;
	}

	struct slot_t { float v; int frame; };

	inline std::map<ImGuiID, slot_t>& slots()
	{
		static std::map<ImGuiID, slot_t> s;
		return s;
	}

	inline float& slot(ImGuiID id, unsigned salt, float init)
	{
		std::map<ImGuiID, slot_t>& s = slots();
		const int frame = ImGui::GetFrameCount();
		const ImGuiID key = id ^ (ImGuiID)(salt * 0x9e3779b9u);
		auto it = s.find(key);
		if (it == s.end())
		{
			if (s.size() > 1024)
				for (auto i = s.begin(); i != s.end(); )
					i = (frame - i->second.frame > 300) ? s.erase(i) : ++i;
			it = s.emplace(key, slot_t{ init, frame }).first;
		}
		it->second.frame = frame;
		return it->second.v;
	}

	inline float ease(float cur, float tgt, float half_life)
	{
		if (half_life <= 0.0001f)
			return tgt;
		const float k = 1.f - powf(0.5f, dt() / half_life);
		return cur + (tgt - cur) * k;
	}

	inline float anim(ImGuiID id, unsigned salt, float tgt, float init, float half_life = 0.07f)
	{
		float& v = slot(id, salt, init);
		v = ease(v, tgt, half_life);
		return v;
	}

	inline float breath(float speed = 1.6f, float low = 0.f)
	{
		const float t = (float)ImGui::GetTime() * speed;
		return low + (1.f - low) * (0.5f + 0.5f * sinf(t));
	}

	inline ImU32 rgba(const ImVec4& c, float a)
	{
		ImVec4 t = c; t.w = a;
		return ImGui::GetColorU32(t);
	}

	inline ImU32 accent_u32(float a = 1.f) { return ImGui::GetColorU32(ImGuiCol_SliderGrab, a); }

	inline ImU32 light(float a = 1.f)
	{
		const float k = ImClamp(a, 0.f, 1.f);
		switch (settings::misc_ui::tint)
		{
		case 1:  return accent_u32(k);
		case 2:  return IM_COL32(226, 238, 255, (int)(k * 255.f + 0.5f));
		default: return sheen::titanium((int)(k * 255.f + 0.5f));
		}
	}

	inline void radial_fan(ImDrawList* dl, ImVec2 c, float r, ImU32 inner, ImU32 outer, int segs = 48)
	{
		if (!dl || r <= 0.5f)
			return;
		segs = ImClamp(segs, 8, 96);
		const ImVec2 uv = dl->_Data ? dl->_Data->TexUvWhitePixel : ImVec2(0.f, 0.f);
		const unsigned int base = dl->_VtxCurrentIdx;

		dl->PrimReserve(segs * 3, segs + 1);
		dl->PrimWriteVtx(c, uv, inner);
		for (int i = 0; i < segs; ++i)
		{
			const float a = (float)i * (6.2831853f / (float)segs);
			dl->PrimWriteVtx(ImVec2(c.x + cosf(a) * r, c.y + sinf(a) * r), uv, outer);
		}
		for (int i = 0; i < segs; ++i)
		{
			dl->PrimWriteIdx((ImDrawIdx)base);
			dl->PrimWriteIdx((ImDrawIdx)(base + 1u + (unsigned)i));
			dl->PrimWriteIdx((ImDrawIdx)(i == segs - 1 ? base + 1u : base + 2u + (unsigned)i));
		}
	}

	inline void annulus(ImDrawList* dl, ImVec2 c, float r, float band, float a_peak,
		ImU32 col, int segs = 48)
	{
		if (!dl || r <= 1.f || a_peak <= 0.004f)
			return;

		segs = ImClamp(segs, 12, 96);
		const float half = ImMax(0.6f, band * 0.5f);
		const float ri = ImMax(0.f, r - half);
		const float ro = r + half;
		const ImU32 none = sheen::with_alpha(col, 0.f);
		const ImU32 peak = sheen::with_alpha(col, a_peak);

		const ImVec2 uv = dl->_Data ? dl->_Data->TexUvWhitePixel : ImVec2(0.f, 0.f);
		const unsigned int base = dl->_VtxCurrentIdx;

		dl->PrimReserve(segs * 12, (segs + 1) * 3);
		for (int i = 0; i <= segs; ++i)
		{
			const float t = (float)i * (6.2831853f / (float)segs);
			const float cs = cosf(t), sn = sinf(t);
			dl->PrimWriteVtx(ImVec2(c.x + cs * ri, c.y + sn * ri), uv, none);
			dl->PrimWriteVtx(ImVec2(c.x + cs * r,  c.y + sn * r),  uv, peak);
			dl->PrimWriteVtx(ImVec2(c.x + cs * ro, c.y + sn * ro), uv, none);
		}
		for (int i = 0; i < segs; ++i)
		{
			const ImDrawIdx in0  = (ImDrawIdx)(base + (unsigned)i);
			const ImDrawIdx in1  = (ImDrawIdx)(in0 + 1);
			const ImDrawIdx mid0 = (ImDrawIdx)(base + (unsigned)(segs + 1 + i));
			const ImDrawIdx mid1 = (ImDrawIdx)(mid0 + 1);
			const ImDrawIdx out0 = (ImDrawIdx)(base + (unsigned)((segs + 1) * 2 + i));
			const ImDrawIdx out1 = (ImDrawIdx)(out0 + 1);
			dl->PrimWriteIdx(in0);  dl->PrimWriteIdx(mid0); dl->PrimWriteIdx(mid1);
			dl->PrimWriteIdx(in0);  dl->PrimWriteIdx(mid1); dl->PrimWriteIdx(in1);
			dl->PrimWriteIdx(mid0); dl->PrimWriteIdx(out0); dl->PrimWriteIdx(out1);
			dl->PrimWriteIdx(mid0); dl->PrimWriteIdx(out1); dl->PrimWriteIdx(mid1);
		}
	}

	inline void comet_ribbon(ImDrawList* dl, const ImVec2* pts, int n, float width_scale, float vis, ImU32 col)
	{
		if (!dl || n < 2 || vis <= 0.02f)
			return;

		const ImVec2 uv = dl->_Data ? dl->_Data->TexUvWhitePixel : ImVec2(0.f, 0.f);
		const unsigned int base = dl->_VtxCurrentIdx;
		dl->PrimReserve((n - 1) * 6, n * 2);

		for (int i = 0; i < n; ++i)
		{
			const ImVec2 p = pts[i];
			const ImVec2 a = pts[i > 0 ? i - 1 : i];
			const ImVec2 b = pts[i < n - 1 ? i + 1 : i];
			ImVec2 d(b.x - a.x, b.y - a.y);
			const float len = sqrtf(d.x * d.x + d.y * d.y);
			if (len > 0.0001f) { d.x /= len; d.y /= len; } else { d.x = 1.f; d.y = 0.f; }

			const float f = (float)i / (float)(n - 1);
			const float w = (0.35f + 3.4f * f * vis) * width_scale * 0.5f;
			const ImU32 c = sheen::with_alpha(col, f * f * vis);
			dl->PrimWriteVtx(ImVec2(p.x + d.y * w, p.y - d.x * w), uv, c);
			dl->PrimWriteVtx(ImVec2(p.x - d.y * w, p.y + d.x * w), uv, c);
		}
		for (int i = 0; i < n - 1; ++i)
		{
			const ImDrawIdx a = (ImDrawIdx)(base + (unsigned)(i * 2));
			dl->PrimWriteIdx(a);
			dl->PrimWriteIdx((ImDrawIdx)(a + 1));
			dl->PrimWriteIdx((ImDrawIdx)(a + 3));
			dl->PrimWriteIdx(a);
			dl->PrimWriteIdx((ImDrawIdx)(a + 3));
			dl->PrimWriteIdx((ImDrawIdx)(a + 2));
		}
	}

	inline float& pending_hover() { static float v = 0.f; return v; }
	inline float& pending_press() { static float v = 0.f; return v; }

	inline void note_feedback(float hover, float press, float shine)
	{
		if (!on()) { pending_hover() = 0.f; pending_press() = 0.f; return; }
		pending_hover() = ImClamp(hover, 0.f, 1.f);

		pending_press() = ImClamp(press, 0.f, 1.f);
	}

	struct control_t
	{
		ImDrawList* dl = nullptr;
		ImVec2 mn{}, mx{};
		float  rounding = 0.f;
		float  hover = 0.f;
		float  press = 0.f;
	};

	inline std::vector<control_t>& controls()
	{
		static std::vector<control_t> v;
		return v;
	}

	inline void note_control(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding)
	{
		if (!on() || !dl)
			return;
		const float h = pending_hover();
		if (h <= 0.01f)
			return;
		if (mx.x - mn.x < 3.f || mx.y - mn.y < 3.f)
			return;
		std::vector<control_t>& v = controls();
		if (v.size() >= 512)
			return;
		control_t c;
		c.dl = dl; c.mn = mn; c.mx = mx; c.rounding = rounding;
		c.hover = h;
		c.press = pending_press();
		v.push_back(c);

		pending_hover() = 0.f;
		pending_press() = 0.f;
	}

	inline float value_pop(ImGuiID id, float value)
	{
		if (!on() || !settings::misc_ui::chip_pop)
			return 0.f;
		float& last = slot(id, 0x7A11u, value);
		float& pop  = slot(id, 0x7A12u, 0.f);
		if (last != value)
		{
			last = value;
			pop  = 1.f;
		}
		pop = ImMax(0.f, pop - dt() * 4.6f);
		return pop;
	}

	inline void knob_spring(float& response, float& damping)
	{
		const bool extra = on() && settings::misc_ui::toggle_overshoot;
		response = extra ? 0.24f : 0.26f;
		damping  = extra ? 0.58f : 0.72f;
	}

	inline float press_inset(float press)
	{
		if (!on() || !settings::misc_ui::press_squish)
			return 0.f;
		return ImClamp(press, 0.f, 1.f) * 1.15f;
	}

	inline ImVec2 cursor_pos()
	{
		ImVec2 p = ImGui::GetIO().MousePos;
		if (!ImGui::IsMousePosValid(&p))
		{
			POINT pt{};
			if (GetCursorPos(&pt))
				p = ImVec2((float)pt.x, (float)pt.y);
			else
				p = ImVec2(-4096.f, -4096.f);
		}
		return p;
	}

	struct trail_pt_t { ImVec2 p; float stamp; };

	inline std::vector<trail_pt_t>& trail_pts()
	{
		static std::vector<trail_pt_t> v;
		return v;
	}

	inline constexpr int kTrailMax = 26;

	struct wave_t { ImVec2 p; float age{ 0.f }; float life{ 0.55f }; };

	inline std::vector<wave_t>& waves()
	{
		static std::vector<wave_t> v;
		return v;
	}

	inline bool&  prev_mouse_down() { static bool b = false; return b; }
	inline ImVec2& prev_cursor()    { static ImVec2 p(-4096.f, -4096.f); return p; }
	inline float&  cursor_speed()   { static float s = 0.f; return s; }
	inline int&    stagger_epoch()  { static int e = 0; return e; }
	inline ImVec2& panel_min()      { static ImVec2 p(0.f, 0.f); return p; }
	inline ImVec2& panel_max()      { static ImVec2 p(0.f, 0.f); return p; }
	inline float&  panel_round()    { static float r = 0.f; return r; }
	inline bool&   panel_live()     { static bool b = false; return b; }

	inline float&  menu_fade()      { static float f = 0.f; return f; }

	inline void frame_begin()
	{
		controls().clear();
		if (!on())
		{
			panel_live() = false;
			prev_mouse_down() = (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]);
			return;
		}

		const ImVec2 p = cursor_pos();
		const ImVec2 prev = prev_cursor();
		const float moved = (prev.x < -2048.f) ? 0.f
			: sqrtf((p.x - prev.x) * (p.x - prev.x) + (p.y - prev.y) * (p.y - prev.y));
		cursor_speed() = cursor_speed() * 0.72f + moved * 0.28f;
		prev_cursor() = p;

		std::vector<trail_pt_t>& tp = trail_pts();
		if (moved > 0.35f || tp.empty())
		{
			trail_pt_t e;
			e.p = p;
			e.stamp = (float)ImGui::GetTime();
			tp.push_back(e);
			if ((int)tp.size() > kTrailMax)
				tp.erase(tp.begin(), tp.begin() + (tp.size() - kTrailMax));
		}
		else if (!tp.empty())
		{

			trail_pts().back().stamp = (float)ImGui::GetTime();
		}

		const float now = (float)ImGui::GetTime();
		std::vector<trail_pt_t>& keep = trail_pts();
		for (auto it = keep.begin(); it != keep.end(); )
			it = (now - it->stamp > 0.30f) ? keep.erase(it) : it + 1;

		const bool down = ImGui::GetIO().MouseDown[ImGuiMouseButton_Left];
		if (down && !prev_mouse_down() && settings::misc_ui::click_shockwave)
		{
			std::vector<wave_t>& w = waves();
			wave_t n;
			n.p = p;
			w.push_back(n);
			if (w.size() > 8)
				w.erase(w.begin());
		}
		prev_mouse_down() = down;

		std::vector<wave_t>& w = waves();
		for (auto it = w.begin(); it != w.end(); )
			it = ((it->age += dt()) >= it->life) ? w.erase(it) : it + 1;
	}

	inline int& wipe_serial() { static int s = 0; return s; }

	inline bool& wipe_is_view() { static bool b = false; return b; }

	inline void reset_stagger(bool view_change = false)
	{
		wipe_is_view() = view_change;
		++stagger_epoch();
		++wipe_serial();
	}

	inline float page_wipe()
	{
		if (!on() || !settings::misc_ui::stagger)
			return 1.f;

		if (!(wipe_is_view() ? settings::misc_ui::cascade_views : settings::misc_ui::cascade_pages))
			return 1.f;
		static int   seen = 0;
		static float t = 1.f;
		if (seen != wipe_serial())
		{
			seen = wipe_serial();
			t = 0.f;
		}

		const float span = settings::misc_ui::cascade_time > 0.02f
			? settings::misc_ui::cascade_time
			: ImMax(0.06f, settings::misc_ui::stagger_step * 14.f);
		t = ImMin(1.f, t + dt() / span);
		return t;
	}

	inline float wipe_eased(float t)
	{
		const float k = ImClamp(t, 0.f, 1.f);
		return k * k * (3.f - 2.f * k);
	}

	inline float stagger(ImGuiID id)
	{
		if (!on() || !settings::misc_ui::stagger)
			return 1.f;
		float& seen = slot(id, 0xC0DEu, -1.f);
		if (seen < 0.f)
		{
			seen = 0.f;
			return 0.f;
		}
		seen = ease(seen, 1.f, 0.10f);
		return ImClamp(seen, 0.f, 1.f);
	}

	inline void note_menu_fade(float f) { menu_fade() = ImClamp(f, 0.f, 1.f); }

	inline void note_panel(ImVec2 mn, ImVec2 mx, float rounding)
	{
		panel_min() = mn;
		panel_max() = mx;
		panel_round() = rounding;
		panel_live() = on();
	}

	inline void paint_controls()
	{
		if (!on())
			return;

		std::vector<control_t>& v = controls();
		if (v.empty())
			return;

		const float amt = amount() * menu_fade();
		if (amt <= 0.004f)
			return;
		const float speed = ImMax(0.05f, settings::misc_ui::hover_sheen_speed);

		const float phase = sheen::phase_smooth(0.42f * speed, 0.f, 0.24f);
		const bool soft_band = settings::misc_ui::sheen_soft;

		for (size_t i = 0; i < v.size(); ++i)
		{
			const control_t& c = v[i];
			ImDrawList* dl = c.dl;
			if (!dl)
				continue;

			const ImVec2 mn = c.mn;
			const ImVec2 mx = c.mx;
			const float r = ImMax(0.f, c.rounding);
			const float h = ImClamp(c.hover, 0.f, 1.f);
			const float press = ImClamp(c.press, 0.f, 1.f);
			const float w = mx.x - mn.x;
			const float hgt = mx.y - mn.y;

			if (settings::misc_ui::hover_tint && h > 0.01f)
				dl->AddRectFilled(mn, mx, light(0.070f * h * amt), r);

			if (settings::misc_ui::hover_sheen && h > 0.02f)
				sheen::band_over_rect(dl, mn, mx, soft_band ? r : 0.f, phase, 0.34f,
					settings::misc_ui::hover_sheen_strength * h * amt, light(),
					soft_band ? 5 : 3);

			if (settings::misc_ui::focus_ring && h > 0.30f)
			{
				const float k = ImClamp((h - 0.30f) / 0.70f, 0.f, 1.f);
				dl->AddRect(mn, mx, light(0.50f * k * amt), r, 0, 1.f);

				// No halo.  It was three one pixel rules around the control, and
				// then a seven step version of the same thing; both read as an
				// outline drawn twice.  Focus is the one hairline above, and the
				// hover tint is what says the pointer is there.
			}

			if (press > 0.02f)
			{
				const float depth = ImMin(hgt * 0.5f, 3.2f);
				dl->AddRectFilledMultiColor(ImVec2(mn.x + 1.f, mn.y + 1.f),
					ImVec2(mx.x - 1.f, mn.y + 1.f + depth),
					IM_COL32(0, 0, 0, (int)(70.f * press)), IM_COL32(0, 0, 0, (int)(70.f * press)),
					IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
				dl->AddRect(ImVec2(mn.x + 0.5f, mn.y + 0.5f), ImVec2(mx.x - 0.5f, mx.y - 0.5f),
					IM_COL32(0, 0, 0, (int)(60.f * press)), ImMax(0.f, r - 1.f), 0, 1.f);
				if (w > 18.f)
					dl->AddRectFilledMultiColor(ImVec2(mn.x + 1.f, mx.y - 1.f - depth),
						ImVec2(mx.x - 1.f, mx.y - 1.f),
						IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0),
						IM_COL32(255, 255, 255, (int)(34.f * press)), IM_COL32(255, 255, 255, (int)(34.f * press)));
			}
		}
	}

	// A soft pool of light that follows the pointer inside the plate.
	//
	// This used to be a *rim* chase: two 2 px bars (one on each half of
	// whichever edge was nearest) that ran along the border under the cursor.
	// On a wide panel that is two white sticks appearing out of nowhere at the
	// top, bottom or sides - it read as UI debris, not as light.  A light has no
	// edge, so there is no bar here any more: one radial falloff, drawn as a
	// triangle fan whose rim is transparent, clipped to the plate.
	inline void paint_rim(ImDrawList* dl)
	{
		if (!on() || !settings::misc_ui::panel_rim || !panel_live() || !dl)
			return;

		const ImVec2 a = panel_min();
		const ImVec2 b = panel_max();
		if (b.x - a.x < 60.f || b.y - a.y < 60.f)
			return;

		const float k = amount() * menu_fade();
		if (k <= 0.004f)
			return;

		const ImVec2 p = cursor_pos();
		const float inset = 28.f;
		const ImVec2 c(ImClamp(p.x, a.x + inset, b.x - inset), ImClamp(p.y, a.y + inset, b.y - inset));

		const float r = 150.f * ImClamp(settings::misc_ui::panel_rim_size, 0.3f, 2.5f);
		const ImU32 base = light(1.f) & 0x00FFFFFFu;

		dl->PushClipRect(ImVec2(a.x + 1.f, a.y + 1.f), ImVec2(b.x - 1.f, b.y - 1.f), true);

		const int seg = 44;
		dl->PrimReserve(seg * 3 * 3, seg * 3 + 3);
		const ImDrawIdx base_idx = (ImDrawIdx)dl->_VtxCurrentIdx;
		const ImVec2 uv = dl->_Data->TexUvWhitePixel;
		int vtx = 0;
		for (int layer = 0; layer < 3; ++layer)
		{
			const float rr = r * (1.f - 0.24f * (float)layer);
			const int alpha = (int)(5.f * k * (1.f + 0.6f * (float)layer));
			const ImU32 col = base | ((ImU32)ImClamp(alpha, 0, 255) << IM_COL32_A_SHIFT);
			const ImU32 rim = base;

			dl->PrimWriteVtx(c, uv, col);
			for (int i = 0; i < seg; ++i)
			{
				const float ang = (float)i / (float)seg * 6.28318531f;
				dl->PrimWriteVtx(ImVec2(c.x + cosf(ang) * rr, c.y + sinf(ang) * rr * 0.8f), uv, rim);
			}
			const ImDrawIdx b0 = (ImDrawIdx)(base_idx + vtx);
			for (int i = 0; i < seg; ++i)
			{
				dl->PrimWriteIdx(b0);
				dl->PrimWriteIdx((ImDrawIdx)(b0 + 1 + i));
				dl->PrimWriteIdx((ImDrawIdx)(b0 + 1 + (i + 1) % seg));
			}
			vtx += seg + 1;
		}

		dl->PopClipRect();
	}

	inline void indicator_glow(ImDrawList* dl, float prev_x, float cur_x,
		float y0, float y1, float w, float rounding)
	{
		if (!on() || !settings::misc_ui::indicator_glow || !dl)
			return;
		const float d = cur_x - prev_x;
		const float dist = fabsf(d);
		if (dist < 1.5f)
			return;

		const int steps = (int)ImClamp(dist / 5.f, 2.f, 7.f);
		const float k = amount();
		for (int i = 1; i <= steps; ++i)
		{
			const float f = (float)i / (float)steps;
			const float x = cur_x - d * f * 0.55f;
			const float a = (1.f - f) * 0.16f * k;
			if (a <= 0.004f)
				continue;
			dl->AddRectFilled(ImVec2(x - w * 0.5f, y0), ImVec2(x + w * 0.5f, y1),
				light(a), rounding);
		}
	}

	inline bool tooltip_card(const char* text)
	{
		if (!on() || !settings::misc_ui::tooltip_fade || !text || !text[0])
			return false;

		ImFont* font = ImGui::GetFont();
		const float size = ImGui::GetFontSize();
		if (!font || size <= 0.f)
			return false;

		const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, text);
		const ImVec2 c = cursor_pos();
		const float rise = anim(ImGui::GetID("##uifx_tt"), 0xF00Du, 1.f, 0.f, 0.055f);

		const float pad_x = 9.f, pad_y = 5.f;
		const ImVec2 mn(c.x + 16.f, c.y + 14.f - (1.f - rise) * 6.f);
		const ImVec2 mx(mn.x + ts.x + pad_x * 2.f, mn.y + ts.y + pad_y * 2.f);

		ImDrawList* dl = ImGui::GetForegroundDrawList();
		const float k = amount() * rise;
		for (int i = 4; i >= 1; --i)
			dl->AddRectFilled(ImVec2(mn.x - (float)i, mn.y - (float)i + 1.f),
				ImVec2(mx.x + (float)i, mx.y + (float)i + 1.f),
				IM_COL32(0, 0, 0, (int)(10.f * k)), 7.f + (float)i);
		dl->AddRectFilled(mn, mx, ImGui::GetColorU32(ImVec4(0.06f, 0.06f, 0.08f, 0.94f * k)), 7.f);
		dl->AddRect(mn, mx, light(0.35f * k), 7.f, 0, 1.f);
		dl->AddText(font, size, ImVec2(mn.x + pad_x, mn.y + pad_y),
			ImGui::GetColorU32(ImVec4(0.92f, 0.93f, 0.96f, k)), text);
		return true;
	}

	inline void draw_overlay(ImDrawList* fg, bool menu_open)
	{
		if (!on() || !fg || !menu_open)
			return;

		const float k = amount();
		const ImVec2 c = cursor_pos();
		if (c.x < -2000.f)
			return;

		if (settings::misc_ui::cursor_aura)
		{
			const float s = ImClamp(settings::misc_ui::cursor_aura_size, 0.2f, 3.f);
			const float pulse = settings::misc_ui::nav_breath ? (0.94f + 0.06f * breath(1.9f)) : 1.f;
			const float r = 31.f * s * pulse;
			if (settings::misc_ui::bloom_soft)
			{
				radial_fan(fg, c, r, light(0.17f * k), light(0.f), 48);

				radial_fan(fg, c, r * 1.85f, light(0.05f * k), light(0.f), 48);
			}
			else
			{
				for (int i = 5; i >= 1; --i)
					fg->AddCircleFilled(c, r * (0.30f + (float)i * 0.17f), light(0.055f * k / (float)i), 26);
			}
		}

		if (settings::misc_ui::cursor_trail)
		{
			std::vector<trail_pt_t>& tp = trail_pts();
			const float gain = ImClamp(cursor_speed() / 26.f, 0.f, 1.f);
			const float len = ImClamp(settings::misc_ui::cursor_trail_length, 0.2f, 3.f);
			const float vis = gain * len * k;
			if (vis > 0.02f && tp.size() >= 2)
			{

				static std::vector<ImVec2> pts;
				pts.clear();
				pts.reserve(tp.size());
				for (const trail_pt_t& p : tp)
					pts.push_back(p.p);
				comet_ribbon(fg, pts.data(), (int)pts.size(), 1.f, 0.42f * vis, light());
				radial_fan(fg, tp.back().p, 4.2f + 3.4f * vis, light(0.40f * vis), light(0.f), 20);
			}
		}

		for (const wave_t& w : waves())
		{
			const float t = ImClamp(w.age / ImMax(0.05f, w.life), 0.f, 1.f);
			const float fade = (1.f - t) * (1.f - t);
			const float ease = t * t * (3.f - 2.f * t);

			const int style = ImClamp(settings::misc_ui::click_style, 0, 2);
			const float r = 5.f + ease * 30.f;
			if (style == 1)
			{

				for (int i = 0; i < 3; ++i)
				{
					const float lag = (float)i * 0.16f;
					const float rt = ImClamp((t - lag) / ImMax(0.05f, 1.f - lag), 0.f, 1.f);
					if (rt <= 0.f)
						continue;
					const float re = rt * rt * (3.f - 2.f * rt);
					const float rf = (1.f - rt) * (1.f - rt);
					annulus(fg, w.p, 4.f + re * (28.f + 9.f * (float)i),
						2.6f + 1.4f * re, 0.40f * rf * k * (1.f - 0.28f * (float)i), light(), 44);
				}
			}
			else if (style == 2)
			{

				radial_fan(fg, w.p, 6.f + ease * 34.f, light(0.30f * fade * k), light(0.f), 40);
			}
			else
			{

				annulus(fg, w.p, r, 2.2f + 1.6f * ease, 0.46f * fade * k, light(), 48);
				annulus(fg, w.p, r * 0.74f, 5.0f, 0.14f * fade * k, light(), 40);
			}

			if (settings::misc_ui::click_spark && t < 0.5f)
			{
				const float st = t / 0.5f;
				const float sa = (1.f - st) * (1.f - st) * 0.85f * k;
				const int n = 6;
				for (int i = 0; i < n; ++i)
				{
					const float ang = (float)i * (6.2831853f / (float)n) + (float)(int)(w.p.x) * 0.11f;
					const float cs = cosf(ang), sn = sinf(ang);
					const float d0 = 3.f + st * 22.f;
					const float d1 = d0 + 7.f + st * 16.f;
					const ImVec2 p0(w.p.x + cs * d0, w.p.y + sn * d0);
					const ImVec2 p1(w.p.x + cs * d1, w.p.y + sn * d1);
					const ImVec2 nrm(-sn, cs);
					const float w0 = 1.5f, w1 = 0.35f;
					fg->AddQuadFilled(ImVec2(p0.x + nrm.x * w0, p0.y + nrm.y * w0),
						ImVec2(p1.x + nrm.x * w1, p1.y + nrm.y * w1),
						ImVec2(p1.x - nrm.x * w1, p1.y - nrm.y * w1),
						ImVec2(p0.x - nrm.x * w0, p0.y - nrm.y * w0),
						light(sa));
				}
			}
		}
	}
}
