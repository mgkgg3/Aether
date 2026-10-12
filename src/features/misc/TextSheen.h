#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <cmath>

namespace sheen
{

	inline ImU32 titanium(int a = 255) { return IM_COL32(247, 246, 243, a); }

	inline ImU32 frost(int a = 255) { return IM_COL32(226, 238, 255, a); }

	inline ImU32 with_alpha(ImU32 c, float k)
	{
		const int a = (int)(((c >> IM_COL32_A_SHIFT) & 0xFF) * ImClamp(k, 0.f, 1.f) + 0.5f);
		return (c & ~IM_COL32_A_MASK) | ((ImU32)ImClamp(a, 0, 255) << IM_COL32_A_SHIFT);
	}

	inline ImU32 mix(ImU32 a, ImU32 b, float k)
	{
		const float t = ImClamp(k, 0.f, 1.f);
		int out[4];
		const ImU32 sh[4] = { IM_COL32_R_SHIFT, IM_COL32_G_SHIFT, IM_COL32_B_SHIFT, IM_COL32_A_SHIFT };
		for (int i = 0; i < 4; ++i)
		{
			const int ca = (int)((a >> sh[i]) & 0xFF);
			const int cb = (int)((b >> sh[i]) & 0xFF);
			out[i] = (int)(ca + (cb - ca) * t + 0.5f);
		}
		return IM_COL32(out[0], out[1], out[2], out[3]);
	}

	inline float phase(float speed, float offset = 0.f)
	{
		const float t = (float)ImGui::GetTime() * (speed > 0.0001f ? speed : 0.0001f);
		float p = std::fmod(t + offset, 1.f);
		if (p < 0.f)
			p += 1.f;
		return p;
	}

	inline float phase_smooth(float speed, float offset = 0.f, float dwell = 0.30f)
	{
		const float sp = speed > 0.0001f ? speed : 0.0001f;
		const float t = std::fmod((float)ImGui::GetTime() * sp + offset, 1.f);
		const float cycle = t < 0.f ? t + 1.f : t;

		const float hold = ImClamp(dwell, 0.f, 0.85f);
		const float travel = 1.f - hold;
		const float p = ImClamp(cycle / travel, 0.f, 1.f);

		return p * p * (3.f - 2.f * p);
	}

	inline float profile(int i, int n, float amount)
	{
		const float u = (n > 1) ? (float)i / (float)(n - 1) : 1.f;
		const float x = 1.f - u;
		return amount * (0.10f + 0.90f * expf(-x * x * 3.4f));
	}

	inline void draw(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 base,
		const char* text, float t, float w = 0.34f, float str = 0.85f,
		ImU32 light = IM_COL32(247, 246, 243, 255), int layers = 6)
	{
		if (!dl || !font || !text || !text[0] || size <= 0.f)
			return;

		if ((base >> IM_COL32_A_SHIFT) != 0)
			dl->AddText(font, size, pos, base, text);

		const float width = font->CalcTextSizeA(size, FLT_MAX, 0.f, text).x;
		if (width <= 2.f)
			return;

		const float amount = ImClamp(str, 0.f, 1.f);
		if (amount <= 0.01f)
			return;

		const int n = ImClamp(layers, 2, 8);
		const float band = ImMax(8.f, width * ImClamp(w, 0.05f, 1.5f));
		const float core = band * 0.5f;
		const float widest = core * 1.85f;

		const float cx = pos.x - widest + ImClamp(t, 0.f, 1.f) * (width + widest * 2.f);
		const float cxs = cx + core;

		const float top = pos.y - size * 0.22f;
		const float bot = pos.y + size * 1.30f;

		float f_prev = 0.f;
		for (int i = 0; i < n; ++i)
		{
			const float f = profile(i, n, amount);
			const float a = (f - f_prev) / ImMax(0.0001f, 1.f - f_prev);
			f_prev = f;
			if (a <= 0.006f)
				continue;

			const float u = (n > 1) ? (float)i / (float)(n - 1) : 1.f;
			const float half = core * (0.30f + 1.55f * (1.f - u));
			const ImU32 col = with_alpha(light, a);
			dl->PushClipRect(ImVec2(cxs - half, top), ImVec2(cxs + half, bot), true);
			dl->AddText(font, size, pos, col, text);
			dl->PopClipRect();
		}
	}

	inline void round_span(float y, ImVec2 mn, ImVec2 mx, float rounding, float& xl, float& xr)
	{
		xl = mn.x;
		xr = mx.x;

		const float hw = (mx.x - mn.x) * 0.5f;
		const float hh = (mx.y - mn.y) * 0.5f;
		const float r = ImMin(rounding, ImMin(hw, hh));
		if (r <= 0.5f)
			return;

		float dy = 0.f;
		if (y < mn.y + r)        dy = (mn.y + r) - y;
		else if (y > mx.y - r)   dy = y - (mx.y - r);
		else                     return;

		dy = ImClamp(dy, 0.f, r);
		const float dx = r - sqrtf(ImMax(0.f, r * r - dy * dy));
		xl = mn.x + dx;
		xr = mx.x - dx;
	}

	inline void band_over_rect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding,
		float t, float w = 0.34f, float str = 0.16f,
		ImU32 light = IM_COL32(247, 246, 243, 255), int layers = 4)
	{
		if (!dl || mx.x <= mn.x + 1.f || mx.y <= mn.y + 1.f)
			return;

		const float amount = ImClamp(str, 0.f, 1.f);
		if (amount <= 0.004f)
			return;

		const float width = mx.x - mn.x;
		const float hgt = mx.y - mn.y;

		const float band = ImMax(7.f, width * ImClamp(w, 0.05f, 1.5f));
		const float core = band * 0.5f;
		const float widest = core * 1.85f;
		const float sigma = ImMax(0.5f, core * 0.75f);
		const float cx = mn.x - widest + ImClamp(t, 0.f, 1.f) * (width + widest * 2.f);
		const float cxs = cx + core;

		const float skew = hgt * 0.55f;
		const float mid = (mn.y + mx.y) * 0.5f;
		auto centre_at = [&](float y)
		{
			return cxs + skew * ((y - mid) / ImMax(1.f, hgt)) * 2.f;
		};

		auto glow = [&](float x, float c)
		{
			const float d = (x - c) / sigma;
			return amount * expf(-d * d);
		};

		int n = ImClamp(layers + 1, 3, 7);
		if (!(n & 1))
			++n;
		float off[7];
		float lit[7];
		for (int i = 0; i < n; ++i)
		{
			const float d = -2.4f + 4.8f * (float)i / (float)(n - 1);
			off[i] = d * sigma;
			lit[i] = sheen::with_alpha(light, amount * expf(-d * d));
		}

		const int rows = (int)ImClamp(ImCeil(hgt / 3.f), 3.f, 14.f);

		const ImVec2 uv = dl->_Data ? dl->_Data->TexUvWhitePixel : ImVec2(0.f, 0.f);
		const unsigned int base = dl->_VtxCurrentIdx;
		dl->PrimReserve(rows * (n - 1) * 6, rows * n * 2);

		for (int row = 0; row < rows; ++row)
		{
			const float y0 = mn.y + hgt * (float)row / (float)rows;
			const float y1 = mn.y + hgt * (float)(row + 1) / (float)rows;

			float xl0, xr0, xl1, xr1;
			round_span(y0, mn, mx, rounding, xl0, xr0);
			round_span(y1, mn, mx, rounding, xl1, xr1);

			const float c0 = centre_at(y0);
			const float c1 = centre_at(y1);
			const unsigned int row_base = base + (unsigned)(row * n * 2);

			for (int i = 0; i < n; ++i)
			{
				const ImU32 col = lit[i];
				dl->PrimWriteVtx(ImVec2(ImClamp(c0 + off[i], xl0, xr0), y0), uv, col);
				dl->PrimWriteVtx(ImVec2(ImClamp(c1 + off[i], xl1, xr1), y1), uv, col);
			}

			for (int i = 0; i < n - 1; ++i)
			{
				const ImDrawIdx a = (ImDrawIdx)(row_base + (unsigned)(i * 2));
				const ImDrawIdx b = (ImDrawIdx)(a + 2);
				const ImDrawIdx c = (ImDrawIdx)(a + 3);
				const ImDrawIdx d = (ImDrawIdx)(a + 1);
				dl->PrimWriteIdx(a); dl->PrimWriteIdx(b); dl->PrimWriteIdx(c);
				dl->PrimWriteIdx(a); dl->PrimWriteIdx(c); dl->PrimWriteIdx(d);
			}
		}
	}

	inline float measure(ImFont* font, float size, const char* text)
	{
		if (!font || !text || !text[0] || size <= 0.f)
			return 0.f;
		return font->CalcTextSizeA(size, FLT_MAX, 0.f, text).x;
	}
}
