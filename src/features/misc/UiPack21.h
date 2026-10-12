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
#include <cstring>

namespace ui21
{
	inline bool on() { return settings::ui21::enabled; }

	inline ImU32 rgba(const ImVec4& c, float a)
	{
		ImVec4 t = c; t.w = a;
		return ImGui::GetColorU32(t);
	}

	inline ImU32 blend(ImU32 a, ImU32 b, float t)
	{
		t = ImClamp(t, 0.f, 1.f);
		const int ac[4] = { (int)((a >> 24) & 0xFF), (int)((a >> 16) & 0xFF), (int)((a >> 8) & 0xFF), (int)(a & 0xFF) };
		const int bc[4] = { (int)((b >> 24) & 0xFF), (int)((b >> 16) & 0xFF), (int)((b >> 8) & 0xFF), (int)(b & 0xFF) };
		return ((ImU32)(ac[0] + (bc[0] - ac[0]) * t) << 24) | ((ImU32)(ac[1] + (bc[1] - ac[1]) * t) << 16)
			| ((ImU32)(ac[2] + (bc[2] - ac[2]) * t) << 8) | (ImU32)(ac[3] + (bc[3] - ac[3]) * t);
	}

	inline ImVec4 accent_a()
	{
		return ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
	}

	inline ImVec4 accent_b()
	{
		const float* c = settings::ui21::accent_2;
		return ImVec4(c[0], c[1], c[2], c[3]);
	}

	inline void glass_face(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, float depth = 1.f)
	{
		if (!on() || !dl || !settings::ui21::glass_layers)
			return;
		const float w = mx.x - mn.x, h = mx.y - mn.y;
		if (w < 8.f || h < 8.f)
			return;

		const float d = ImClamp(depth * settings::ui21::glass_depth, 0.f, 2.f);
		const ImVec4 a = accent_a();
		const ImVec4 b = accent_b();

		dl->AddRectFilled(mn, mx, rgba(ImVec4(0.055f, 0.058f, 0.075f, 0.72f), 1.f), rounding);

		{
			constexpr int kSteps = 5;
			const float span = ImMin(w, h);
			for (int i = 0; i < kSteps; ++i)
			{
				const float t = (float)i / (float)kSteps;
				const float inset = ImMin(t * span * 1.10f, span);
				if (inset <= 1.f)
					continue;
				const ImU32 c = rgba(b, 0.055f * d * (1.f - t));
				dl->AddTriangleFilled(ImVec2(mx.x - inset, mx.y), ImVec2(mx.x, mx.y),
					ImVec2(mx.x, mx.y - inset), c);
			}
		}

		if (h > 10.f && w > 16.f)
		{
			const float inner = ImMax(rounding * 0.6f, 2.f);
			dl->AddRectFilledMultiColor(ImVec2(mn.x + inner, mn.y + 1.f),
				ImVec2(mx.x - inner, mn.y + 1.f + ImMin(h * 0.10f, 9.f)),
				rgba(ImVec4(1.f, 1.f, 1.f, 1.f), 0.075f * d), rgba(ImVec4(1.f, 1.f, 1.f, 1.f), 0.012f * d),
				rgba(ImVec4(1.f, 1.f, 1.f, 1.f), 0.012f * d), rgba(ImVec4(1.f, 1.f, 1.f, 1.f), 0.075f * d));
		}

		if (h > 14.f)
		{
			const float inner = ImMax(rounding * 0.6f, 2.f);
			dl->AddRectFilledMultiColor(ImVec2(mn.x + inner, mx.y - 1.f - ImMin(h * 0.16f, 14.f)),
				ImVec2(mx.x - inner, mx.y - 1.f),
				rgba(ImVec4(0.f, 0.f, 0.f, 1.f), 0.f), rgba(ImVec4(0.f, 0.f, 0.f, 1.f), 0.f),
				rgba(ImVec4(0.f, 0.f, 0.f, 1.f), 0.16f * d), rgba(ImVec4(0.f, 0.f, 0.f, 1.f), 0.16f * d));
		}

		if (settings::ui21::noise && settings::ui21::noise_amount > 0.01f)
		{
			const float amount = ImClamp(settings::ui21::noise_amount, 0.f, 1.f);
			unsigned s = (unsigned)((size_t)dl * 2654435761u) ^ 0x5bf03635u;
			if (s == 0u) s = 0x2545F491u;
			const auto rnd = [&s]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; };

			dl->PushClipRect(mn, mx, true);
			const int count = (int)ImClamp(w * h / 620.f * amount, 0.f, 900.f);
			const ImU32 dim = IM_COL32(0, 0, 0, (int)(6.f + 14.f * amount));
			const ImU32 lit = IM_COL32(255, 255, 255, (int)(4.f + 8.f * amount));
			for (int i = 0; i < count; ++i)
			{
				const float x = mn.x + 1.f + (float)(rnd() % 99991u) / 99991.f * (w - 2.f);
				const float y = mn.y + 1.f + (float)(rnd() % 99991u) / 99991.f * (h - 2.f);
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + 1.f, y + 1.f), (rnd() & 1u) ? dim : lit);
			}
			dl->PopClipRect();
		}

		dl->AddRect(mn, mx, rgba(ImVec4(1.f, 1.f, 1.f, 1.f), 0.10f), rounding, 0, 1.f);
	}

	inline void spotlight_border(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding,
		ImVec2 cursor, float window_len = 150.f)
	{
		if (!on() || !dl || !settings::ui21::spotlight_border)
			return;
		const float w = mx.x - mn.x, h = mx.y - mn.y;
		if (w < 24.f || h < 24.f)
			return;

		const float len = ImClamp(window_len * settings::ui21::spotlight_size, 30.f, 900.f);
		const float guard = ImMax(2.f, rounding + 1.f);

		const ImU32 hot = rgba(accent_a(), 0.85f);
		const ImU32 cold = rgba(accent_a(), 0.f);

		const float d_top = ImAbs(cursor.y - mn.y);
		const float d_bot = ImAbs(cursor.y - mx.y);
		const float d_left = ImAbs(cursor.x - mn.x);
		const float d_right = ImAbs(cursor.x - mx.x);
		float best = d_top;
		int edge = 0;
		if (d_bot < best) { best = d_bot; edge = 1; }
		if (d_left < best) { best = d_left; edge = 2; }
		if (d_right < best) { best = d_right; edge = 3; }

		constexpr float kT = 1.6f;
		switch (edge)
		{
		case 0:
		{
			const float x0 = ImClamp(cursor.x - len, mn.x + guard, mx.x - guard);
			const float x1 = ImClamp(cursor.x + len, mn.x + guard, mx.x - guard);
			if (x1 > x0) dl->AddRectFilledMultiColor(ImVec2(x0, mn.y), ImVec2(x1, mn.y + kT), cold, cold, hot, hot);
			break;
		}
		case 1:
		{
			const float x0 = ImClamp(cursor.x - len, mn.x + guard, mx.x - guard);
			const float x1 = ImClamp(cursor.x + len, mn.x + guard, mx.x - guard);
			if (x1 > x0) dl->AddRectFilledMultiColor(ImVec2(x0, mx.y - kT), ImVec2(x1, mx.y), hot, hot, cold, cold);
			break;
		}
		case 2:
		{
			const float y0 = ImClamp(cursor.y - len, mn.y + guard, mx.y - guard);
			const float y1 = ImClamp(cursor.y + len, mn.y + guard, mx.y - guard);
			if (y1 > y0) dl->AddRectFilledMultiColor(ImVec2(mn.x, y0), ImVec2(mn.x + kT, y1), cold, hot, hot, cold);
			break;
		}
		default:
		{
			const float y0 = ImClamp(cursor.y - len, mn.y + guard, mx.y - guard);
			const float y1 = ImClamp(cursor.y + len, mn.y + guard, mx.y - guard);
			if (y1 > y0) dl->AddRectFilledMultiColor(ImVec2(mx.x - kT, y0), ImVec2(mx.x, y1), hot, cold, cold, hot);
			break;
		}
		}
	}

	inline void dot_grid(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding)
	{
		if (!on() || !dl || !settings::ui21::dot_grid)
			return;
		const float spacing = ImClamp(settings::ui21::dot_grid_spacing, 6.f, 80.f);
		if (mx.x - mn.x < spacing * 2.f || mx.y - mn.y < spacing * 2.f)
			return;

		const ImU32 col = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.055f));

		const float x0 = mn.x + rounding * 0.5f;
		const float y0 = mn.y + rounding * 0.5f;
		const float x1 = mx.x - rounding * 0.5f;
		const float y1 = mx.y - rounding * 0.5f;
		dl->PushClipRect(ImVec2(x0, y0), ImVec2(x1, y1), true);
		const int nx = (int)((x1 - x0 - 1.f) / spacing);
		const int ny = (int)((y1 - y0 - 1.f) / spacing);
		for (int iy = 0; iy <= ny; ++iy)
			for (int ix = 0; ix <= nx; ++ix)
				dl->AddRectFilled(ImVec2(x0 + ix * spacing, y0 + iy * spacing),
					ImVec2(x0 + ix * spacing + 1.f, y0 + iy * spacing + 1.f), col);
		dl->PopClipRect();
	}

	inline void gradient_text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos,
		const char* text, float t = 0.f, float alpha = 1.f)
	{
		if (!on() || !dl || !font || !text || !text[0] || size <= 0.f)
			return;

		if (!settings::ui21::gradient_heading)
		{
			dl->AddText(font, size, pos, rgba(accent_a(), alpha), text);
			return;
		}

		const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, text);
		if (ts.x <= 1.f)
			return;

		const ImVec4 a = accent_a();
		const ImVec4 b = accent_b();
		const float phase = ImClamp(t, 0.f, 1.f) * 0.5f;

		constexpr float kSlice = 2.f;
		const int slices = (int)ImCeil(ts.x / kSlice);
		for (int i = 0; i < slices; ++i)
		{
			const float x0 = pos.x + (float)i * kSlice;

			const float k = ImClamp((float)i / (float)ImMax(1, slices - 1) + phase, 0.f, 1.f);
			const ImVec4 c(
				(a.x + (b.x - a.x) * k),
				(a.y + (b.y - a.y) * k),
				(a.z + (b.z - a.z) * k), alpha);
			dl->PushClipRect(ImVec2(x0, pos.y - size * 0.35f),
				ImVec2(x0 + kSlice, pos.y + size * 1.35f), true);
			dl->AddText(font, size, pos, rgba(c, 1.f), text);
			dl->PopClipRect();
		}
	}

	inline void odometer(ImDrawList* dl, ImGuiID id, ImFont* font, float size, ImVec2 pos,
		const char* text, ImU32 col)
	{
		if (!on() || !dl || !font || !text)
			return;
		if (!settings::ui21::odometer)
		{
			dl->AddText(font, size, pos, col, text);
			return;
		}

		const float dt = ImClamp(ImGui::GetIO().DeltaTime, 0.f, 1.f / 20.f);
		ImGuiStorage* store = ImGui::GetStateStorage();

		float x = pos.x;
		for (int i = 0; text[i]; ++i)
		{
			char glyph[2] = { text[i], 0 };

			const bool digit = (text[i] >= '0' && text[i] <= '9');
			const float gw = font->CalcTextSizeA(size, FLT_MAX, 0.f, glyph).x;

			if (!digit)
			{
				dl->AddText(font, size, ImVec2(x, pos.y), col, glyph);
				x += gw;
				continue;
			}

			const ImGuiID key = (ImGuiID)(id ^ (ImGuiID)(i * 2654435761u));
			const ImGuiID key_old = (ImGuiID)(key ^ 0x51edu);
			int current = (int)text[i] - '0';
			int previous = store->GetInt(key_old, current);
			if (previous != current)
			{
				store->SetInt(key_old, current);
				store->SetFloat(key, 1.f);
			}
			float roll = store->GetFloat(key, 0.f);

			if (roll <= 0.001f)
			{
				dl->AddText(font, size, ImVec2(x, pos.y), col, glyph);
				x += gw;
				continue;
			}

			roll = ImMax(0.f, roll - dt * 4.2f);
			store->SetFloat(key, roll);

			const float e = 1.f - (roll * roll * (3.f - 2.f * roll));
			dl->PushClipRect(ImVec2(x - 1.f, pos.y - 1.f), ImVec2(x + gw + 1.f, pos.y + size + 1.f), true);
			char old_glyph[2] = { (char)('0' + previous), 0 };
			const int old_alpha = (int)(255.f * (1.f - e) * ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.f);
			const int new_alpha = (int)(255.f * e * ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.f);
			dl->AddText(font, size, ImVec2(x, pos.y - e * size), (col & 0x00FFFFFFu) | ((ImU32)ImClamp(old_alpha, 0, 255) << 24), old_glyph);
			dl->AddText(font, size, ImVec2(x, pos.y + (1.f - e) * size), (col & 0x00FFFFFFu) | ((ImU32)ImClamp(new_alpha, 0, 255) << 24), glyph);
			dl->PopClipRect();
			x += gw;
		}
	}

	inline void pill_tabs(ImDrawList* dl, ImFont* font, ImVec2 mn, ImVec2 mx,
		const char* const* labels, int count, int active, float pill_x, float pill_w, float rounding)
	{
		if (!on() || !dl || !font || count <= 0 || mx.x < mn.x)
			return;

		dl->AddRectFilled(mn, mx, rgba(ImVec4(0.f, 0.f, 0.f, 1.f), 0.28f), rounding);

		const ImVec4 a = accent_a();
		const ImVec4 b = accent_b();
		const ImVec2 pa(pill_x + 2.f, mn.y + 2.f);
		const ImVec2 pb(pill_x + pill_w - 2.f, mx.y - 2.f);
		if (pb.x > pa.x && pb.y > pa.y)
		{

			const float h = pb.y - pa.y;
			const int slabs = (int)ImClamp(ImCeil(h * 0.7f), 2.f, 14.f);
			for (int i = 0; i < slabs; ++i)
			{
				const float y0 = pa.y + h * (float)i / (float)slabs;
				const float y1 = (i == slabs - 1) ? pb.y : pa.y + h * (float)(i + 1) / (float)slabs;
				const float k = (float)i / (float)(slabs - 1);
				dl->AddRectFilled(ImVec2(pa.x, y0), ImVec2(pb.x, y1),
					rgba(ImVec4(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
						a.z + (b.z - a.z) * k, 1.f), 0.92f), i == 0 || i == slabs - 1 ? rounding - 2.f : 0.f);
			}
		}

		const float lum = a.x * 0.299f + a.y * 0.587f + a.z * 0.114f;		const ImU32 on_pill = rgba(lum > 0.55f ? ImVec4(0.06f, 0.06f, 0.07f, 1.f)
			: ImVec4(0.99f, 0.99f, 1.f, 1.f), 1.f);
		const ImU32 off_pill = rgba(ImVec4(0.78f, 0.80f, 0.86f, 1.f), 0.75f);

		const float seg = (mx.x - mn.x) / (float)count;
		for (int i = 0; i < count; ++i)
		{
			const char* lab = labels[i] ? labels[i] : "";
			if (!lab[0]) continue;
			const ImVec2 ts = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, lab);
			const float cx = mn.x + seg * ((float)i + 0.5f);
			dl->AddText(font, font->FontSize, ImVec2(cx - ts.x * 0.5f, (mn.y + mx.y - ts.y) * 0.5f),
				i == active ? on_pill : off_pill, lab);
		}
	}
}
