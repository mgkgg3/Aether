#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>

#include <gui/menu/Anim.h>
#include <gui/menu/SyncBridge.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace aim_viz
{

	inline constexpr int kStyles = 11;

	struct style_t { const char* name; const char* note; };

	inline const style_t& style(int i)
	{
		static const style_t k[kStyles] =
		{
			{ "Default",       "linear, the raw factor" },
			{ "Linear",        "linear, the raw factor" },
			{ "EaseInQuad",    "starts slow" },
			{ "EaseOutQuad",   "starts fast" },
			{ "EaseInOutQuad", "slow, fast, slow" },
			{ "EaseInCubic",   "very slow start" },
			{ "EaseOutCubic",  "very fast start" },
			{ "EaseInOutCubic","long, fast middle" },
			{ "EaseInSine",    "gentle start" },
			{ "EaseOutSine",   "gentle finish" },
			{ "EaseInOutSine", "the softest of them" },
		};
		if (i < 0) i = 0;
		if (i >= kStyles) i = kStyles - 1;
		return k[i];
	}

	inline float ease_linear(float t)      { return t; }
	inline float ease_in_quad(float t)     { return t * t; }
	inline float ease_out_quad(float t)    { return t * (2.f - t); }
	inline float ease_inout_quad(float t)  { return t < 0.5f ? 2.f * t * t : -1.f + (4.f - 2.f * t) * t; }
	inline float ease_in_cubic(float t)    { return t * t * t; }
	inline float ease_out_cubic(float t)   { return (t - 1.f) * (t - 1.f) * (t - 1.f) + 1.f; }
	inline float ease_inout_cubic(float t) { return t < 0.5f ? 4.f * t * t * t : (t - 1.f) * (2.f * t - 2.f) * (2.f * t - 2.f) + 1.f; }
	inline float ease_in_sine(float t)     { return 1.f - cosf(t * 3.14159265f * 0.5f); }
	inline float ease_out_sine(float t)    { return sinf(t * 3.14159265f * 0.5f); }
	inline float ease_inout_sine(float t)  { return -(cosf(3.14159265f * t) - 1.f) * 0.5f; }

	inline float shape(int style_index, float t)
	{
		switch (style_index)
		{
		case 1:  return ease_linear(t);
		case 2:  return ease_in_quad(t);
		case 3:  return ease_out_quad(t);
		case 4:  return ease_inout_quad(t);
		case 5:  return ease_in_cubic(t);
		case 6:  return ease_out_cubic(t);
		case 7:  return ease_inout_cubic(t);
		case 8:  return ease_in_sine(t);
		case 9:  return ease_out_sine(t);
		case 10: return ease_inout_sine(t);
		default: return t;
		}
	}

	inline float factor(int style_index, float smoothness)
	{
		if (smoothness <= 0.01f) return 1.f;
		float t = 1.f / smoothness;
		t = ImClamp(t, 0.01f, 1.f);
		return ImClamp(shape(style_index, t), 0.f, 1.f);
	}

	inline constexpr float kCountsPerDeg = 18.f;

	inline int  mode()       { return settings::aimbot::mode; }
	inline bool mouse_mode() { return mode() == 0; }
	inline const char* unit() { return mouse_mode() ? "cnt" : "deg"; }
	inline float to_units(float degrees) { return mouse_mode() ? degrees * kCountsPerDeg : degrees; }

	inline int shown_style()
	{
		if (settings::viz::pin_style >= 0) return ImClamp(settings::viz::pin_style, 0, kStyles - 1);

		static float t = 0.f;
		static unsigned long long last = ~0ull;
		const unsigned long long f = ImGui::GetFrameCount();
		if (last != f)
		{
			last = f;
			t += anim::dt();
		}
		const float period = ImClamp(settings::viz::demo_speed, 0.8f, 12.f);
		const int idx = (int)(t / period) % kStyles;
		return ImClamp(settings::viz::demo_switch ? idx : 0, 0, kStyles - 1);
	}

	inline float shown_progress()
	{
		if (settings::viz::pin_style >= 0 || !settings::viz::demo_switch) return 0.f;
		static float t = 0.f;
		static unsigned long long last = ~0ull;
		const unsigned long long f = ImGui::GetFrameCount();
		if (last != f) { last = f; t += anim::dt(); }
		const float period = ImClamp(settings::viz::demo_speed, 0.8f, 12.f);
		return fmodf(t, period) / period;
	}

	inline constexpr int kTrail = 90;
	inline constexpr float kTickHz = 240.f;
	inline constexpr float kDeg = 7.5f;

	struct demo_t
	{
		float t = 0.f;
		float sim = 0.f;
		float tx = 0.f, ty = 0.f;
		float ax = 0.f, ay = 0.f;
		float raw_x = 0.f, raw_y = 0.f;
		float step_x = 0.f, step_y = 0.f;
		float trail_t[kTrail][2]{};
		float trail_a[kTrail][2]{};
		int   trail_n = 0;
		int   seen_style = -1;
		float style_fade = 1.f;
		float ping = 0.f;
		float err_avg = 0.f;
		float step_avg = 0.f;
		float rem_x = 0.f, rem_y = 0.f;
	};

	inline demo_t& demo()
	{
		static demo_t d;
		return d;
	}

	inline void demo_target(demo_t& d, float& out_x, float& out_y)
	{
		out_x = 0.64f * sinf(d.t * 1.05f) + 0.24f * sinf(d.t * 2.70f + 1.10f);
		out_y = 0.42f * cosf(d.t * 1.70f + 0.60f) + 0.20f * sinf(d.t * 3.30f + 0.20f);
	}

	inline constexpr int kMax = 1024;
	inline sync::AimSample* demo_buf()
	{
		static sync::AimSample b[kMax];
		return b;
	}

	inline int& demo_count()
	{
		static int n = 0;
		return n;
	}

	inline void demo_step(int style_index, float sx, float sy, float dt)
	{
		demo_t& d = demo();
		const float fx = factor(style_index, sx);
		const float fy = factor(style_index, sy);

		if (d.seen_style != style_index)
		{
			d.seen_style = style_index;
			d.trail_n = 0;
			d.style_fade = 0.f;
		}
		d.style_fade = ImMin(1.f, d.style_fade + dt * 3.f);

		d.t += dt;

		int ticks = (int)(dt * kTickHz);
		if (ticks > 32) ticks = 32;
		if (ticks < 1) ticks = 1;

		for (int i = 0; i < ticks; ++i)
		{
			float nx, ny;
			demo_target(d, nx, ny);
			d.tx = nx; d.ty = ny;

			d.raw_x = d.tx - d.ax;
			d.raw_y = d.ty - d.ay;
			d.step_x = d.raw_x * fx;
			d.step_y = d.raw_y * fy;

			if (mouse_mode())
			{
				d.rem_x += d.step_x * kCountsPerDeg;
				d.rem_y += d.step_y * kCountsPerDeg;
				const float mx2 = roundf(d.rem_x);
				const float my2 = roundf(d.rem_y);
				d.rem_x -= mx2;
				d.rem_y -= my2;
				d.step_x = mx2 / kCountsPerDeg;
				d.step_y = my2 / kCountsPerDeg;
			}
			d.ax += d.step_x;
			d.ay += d.step_y;

			{
				const int n = demo_count();
				sync::AimSample& s = demo_buf()[n % kMax];
				if (n < kMax)
				{
					s.t           = (float)n * (1.f / kTickHz);
					s.raw_yaw     = d.raw_x;
					s.raw_pitch   = d.raw_y;
					s.step_yaw    = d.step_x;
					s.step_pitch  = d.step_y;
					s.smoothing_x = sx;
					s.smoothing_y = sy;
					s.dist        = sqrtf(d.raw_x * d.raw_x + d.raw_y * d.raw_y);
					s.primary     = true;
					demo_count() = n + 1;
				}
				else
				{

					memmove(demo_buf(), demo_buf() + 1, sizeof(sync::AimSample) * (kMax - 1));
					sync::AimSample& last = demo_buf()[kMax - 1];
					last.t           = demo_buf()[kMax - 2].t + (1.f / kTickHz);
					last.raw_yaw     = d.raw_x;
					last.raw_pitch   = d.raw_y;
					last.step_yaw    = d.step_x;
					last.step_pitch  = d.step_y;
					last.smoothing_x = sx;
					last.smoothing_y = sy;
					last.dist        = sqrtf(d.raw_x * d.raw_x + d.raw_y * d.raw_y);
					last.primary     = true;
				}
			}
		}

		float* tp = &d.trail_t[0][0];
		if (d.trail_n < kTrail)
			++d.trail_n;
		else
			memmove(tp, tp + 2, sizeof(float) * 2 * (kTrail - 1));
		d.trail_t[d.trail_n - 1][0] = d.tx;
		d.trail_t[d.trail_n - 1][1] = d.ty;

		float* ap = &d.trail_a[0][0];
		memmove(ap, ap + 2, sizeof(float) * 2 * (kTrail - 1));
		d.trail_a[kTrail - 1][0] = d.ax;
		d.trail_a[kTrail - 1][1] = d.ay;

		const float dist = sqrtf(d.raw_x * d.raw_x + d.raw_y * d.raw_y);
		if (dist < 0.9f && d.ping <= 0.f) d.ping = 1.f;
		if (d.ping > 0.f) d.ping = ImMax(0.f, d.ping - dt * 1.7f);

		const float k = 1.f - powf(0.5f, dt / 0.35f);
		d.err_avg  += (dist - d.err_avg) * k;
		d.step_avg += (sqrtf(d.step_x * d.step_x + d.step_y * d.step_y) - d.step_avg) * k;
	}

	inline const sync::AimSample* trace_buf()
	{
		return demo_buf();
	}

	inline int trace_n()
	{
		return ImMin(demo_count(), kMax);
	}

	inline void tracked_text(ImDrawList* dl, ImFont* f, ImVec2 p, ImU32 col, const char* t, float tracking)
	{
		if (!f || !t) return;
		float x = p.x;
		for (const char* c = t; *c; ++c)
		{
			char one[2] = { *c, 0 };
			dl->AddText(f, f->FontSize, ImVec2(x, p.y), col, one);
			x += f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, one).x + tracking;
		}
	}

	inline void chase_well(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float alpha, int style_index)
	{
		demo_t& d = demo();

		const float pad = 16.f;
		const ImVec2 c((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
		const float rx = (mx.x - mn.x) * 0.5f - pad;
		const float ry = (mx.y - mn.y) * 0.5f - pad;

		dl->PushClipRect(mn, mx, true);

		dl->AddLine(ImVec2(mn.x, mx.y - 0.5f), ImVec2(mx.x, mx.y - 0.5f), ImGui::GetColorU32(IM_COL32(255, 255, 255, 14), alpha), 1.f);
		dl->AddLine(ImVec2(c.x, mn.y + 8.f), ImVec2(c.x, mx.y - 8.f), ImGui::GetColorU32(IM_COL32(255, 255, 255, 12), alpha), 1.f);
		dl->AddLine(ImVec2(mn.x + 8.f, c.y), ImVec2(mx.x - 8.f, c.y), ImGui::GetColorU32(IM_COL32(255, 255, 255, 12), alpha), 1.f);

		auto to_screen = [&](float dx, float dy) {
			return ImVec2(c.x + (dx / kDeg) * rx, c.y - (dy / kDeg) * ry);
		};

		const ImU32 raw_col = ImGui::GetColorU32(ImVec4(settings::viz::raw_color[0], settings::viz::raw_color[1],
			settings::viz::raw_color[2], alpha * settings::viz::raw_color[3]));
		const ImU32 aim_col = ImGui::GetColorU32(ImVec4(settings::viz::smooth_color[0], settings::viz::smooth_color[1],
			settings::viz::smooth_color[2], alpha * settings::viz::smooth_color[3]));

		if (d.trail_n >= 2)
		{
			std::vector<ImVec2> pts;
			pts.reserve((size_t)d.trail_n);
			for (int i = 0; i < d.trail_n; ++i)
				pts.push_back(to_screen(d.trail_t[i][0], d.trail_t[i][1]));
			dl->AddPolyline(pts.data(), (int)pts.size(), ImGui::GetColorU32(ImVec4(
				settings::viz::raw_color[0], settings::viz::raw_color[1], settings::viz::raw_color[2],
				alpha * settings::viz::raw_color[3] * 0.35f)), 0, 1.f);
		}

		const ImVec2 t = to_screen(d.tx, d.ty);
		const ImVec2 a = to_screen(d.ax, d.ay);

		{
			std::vector<ImVec2> pts;
			pts.reserve(kTrail);
			for (int i = 0; i < kTrail; ++i)
				pts.push_back(to_screen(d.trail_a[i][0], d.trail_a[i][1]));
			dl->AddPolyline(pts.data(), (int)pts.size(), ImGui::GetColorU32(ImVec4(
				settings::viz::smooth_color[0], settings::viz::smooth_color[1], settings::viz::smooth_color[2],
				alpha * settings::viz::smooth_color[3] * 0.8f * d.style_fade)), 0, 1.2f);
		}

		if (settings::viz::band)
			dl->AddLine(a, t, ImGui::GetColorU32(IM_COL32(170, 140, 255, 90), alpha * d.style_fade), 1.f);

		dl->AddCircle(t, 7.f, raw_col, 40, 1.f);
		dl->AddCircleFilled(t, 1.4f, raw_col, 12);
		if (d.ping > 0.f)
		{
			const float r = 8.f + 18.f * (1.f - d.ping);
			dl->AddCircle(t, r, ImGui::GetColorU32(IM_COL32(255, 255, 255, 90), alpha * d.ping), 44, 1.f);
		}

		dl->AddCircle(a, 5.f, aim_col, 32, 1.f);
		dl->AddCircleFilled(a, 1.4f, aim_col, 12);

		if (g_FontSmall)
		{
			dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(mn.x + 12.f, mn.y + 10.f),
				ImGui::GetColorU32(IM_COL32(150, 150, 156, 255), alpha), "target");
			const ImVec2 ts = g_FontSmall->CalcTextSizeA(g_FontSmall->FontSize, FLT_MAX, 0.f, "aim");
			dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(mx.x - 12.f - ts.x, mn.y + 10.f),
				ImGui::GetColorU32(IM_COL32(150, 150, 156, 255), alpha), "aim");
		}

		(void)style_index;
		dl->PopClipRect();
	}

	inline void curves_well(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float alpha, int style_index, float cur_x, float cur_y)
	{

		dl->AddLine(ImVec2(mn.x, mx.y - 0.5f), ImVec2(mx.x, mx.y - 0.5f), ImGui::GetColorU32(IM_COL32(255, 255, 255, 14), alpha), 1.f);

		const float padL = 34.f, padR = 14.f, padT = 14.f, padB = 22.f;
		const ImVec2 g0(mn.x + padL, mn.y + padT), g1(mx.x - padR, mx.y - padB);

		for (int i = 0; i <= 4; ++i)
		{
			const float u = (float)i / 4.f;
			const float y = g1.y - (g1.y - g0.y) * u;
			dl->AddLine(ImVec2(g0.x, y), ImVec2(g1.x, y), ImGui::GetColorU32(IM_COL32(255, 255, 255, 9), alpha), 1.f);
			if (i % 2 == 0 && g_FontSmall)
			{
				char lab[16];
				snprintf(lab, sizeof lab, "%.0f%%", u * 100.f);
				const ImVec2 ts = g_FontSmall->CalcTextSizeA(g_FontSmall->FontSize, FLT_MAX, 0.f, lab);
				dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(mn.x + 8.f, y - ts.y * 0.5f),
					ImGui::GetColorU32(IM_COL32(120, 120, 126, 255), alpha), lab);
			}
		}

		const int steps = 60;
		auto curve_point = [&](int s, float u) {
			const float smoothness = ImMax(1.f, u * 100.f);
			const float f = factor(s, smoothness);
			return ImVec2(g0.x + (g1.x - g0.x) * u, g1.y - (g1.y - g0.y) * f);
		};

		for (int s = 0; s < kStyles; ++s)
		{
			if (s == style_index) continue;
			std::vector<ImVec2> pts;
			pts.reserve(steps + 1);
			for (int i = 0; i <= steps; ++i)
				pts.push_back(curve_point(s, (float)i / (float)steps));
			dl->AddPolyline(pts.data(), (int)pts.size(), ImGui::GetColorU32(IM_COL32(255, 255, 255, 26), alpha), 0, 1.f);
		}

		{
			std::vector<ImVec2> pts;
			pts.reserve(steps + 1);
			for (int i = 0; i <= steps; ++i)
				pts.push_back(curve_point(style_index, (float)i / (float)steps));
			const ImU32 col = ImGui::GetColorU32(ImVec4(settings::viz::smooth_color[0], settings::viz::smooth_color[1],
				settings::viz::smooth_color[2], alpha));
			dl->AddPolyline(pts.data(), (int)pts.size(), col, 0, 1.5f);

			const float ux = ImClamp(cur_x / 100.f, 0.f, 1.f);
			const float uy = ImClamp(cur_y / 100.f, 0.f, 1.f);
			const ImVec2 px = curve_point(style_index, ux);
			const ImVec2 py = curve_point(style_index, uy);
			dl->AddLine(ImVec2(px.x, g0.y), ImVec2(px.x, g1.y), ImGui::GetColorU32(IM_COL32(255, 255, 255, 26), alpha), 1.f);
			dl->AddCircleFilled(px, 3.4f, ImGui::GetColorU32(ImVec4(settings::viz::smooth_color[0],
				settings::viz::smooth_color[1], settings::viz::smooth_color[2], alpha)), 16);
			dl->AddCircleFilled(py, 3.4f, ImGui::GetColorU32(ImVec4(settings::viz::raw_color[0],
				settings::viz::raw_color[1], settings::viz::raw_color[2], alpha)), 16);

			if (g_FontSmall)
			{
				char buf[64];
				snprintf(buf, sizeof buf, "y %.0f%% -> %.2f", cur_y, factor(style_index, cur_y));
				const ImVec2 ts = g_FontSmall->CalcTextSizeA(g_FontSmall->FontSize, FLT_MAX, 0.f, buf);
				dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(mx.x - padR - ts.x, mx.y - 17.f),
					ImGui::GetColorU32(IM_COL32(150, 150, 156, 255), alpha), buf);
				snprintf(buf, sizeof buf, "x %.0f%% -> %.2f", cur_x, factor(style_index, cur_x));
				dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(mn.x + 8.f, mx.y - 17.f),
					ImGui::GetColorU32(IM_COL32(150, 150, 156, 255), alpha), buf);
			}
		}
	}

	inline ImVec2 preview_min{ 0.f, 0.f };
	inline ImVec2 preview_max{ 0.f, 0.f };

	inline void preview(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float alpha)
	{
		demo_t& d = demo();
		preview_min = mn;
		preview_max = mx;
		const int  si  = shown_style();
		const float sx = settings::viz::demo_x;
		const float sy = settings::viz::demo_y;

		demo_step(si, sx, sy, anim::dt());

		ImFont* fb = g_FontValue ? g_FontValue : (g_FontBold ? g_FontBold : g_FontRegular);
		ImFont* fs = g_FontSmall ? g_FontSmall : g_FontRegular;

		float head_h = 30.f;
		{
			if (fs)
				tracked_text(dl, fs, ImVec2(mn.x, mn.y + 7.f),
					ImGui::GetColorU32(IM_COL32(132, 132, 138, 255), alpha), "SMOOTHING", 1.1f);

			const float name_x = mn.x + 96.f;
			if (fb)
				dl->AddText(fb, fb->FontSize, ImVec2(name_x, mn.y + 2.f), ImGui::GetColorU32(IM_COL32(236, 236, 240, 255), alpha),
					style(si).name);

			if (fs)
			{

				char note[96];
				snprintf(note, sizeof note, "%s, %s mode", style(si).note, mouse_mode() ? "mouse" : "camera");
				dl->AddText(fs, fs->FontSize, ImVec2(name_x + 1.f, mn.y + 22.f),
					ImGui::GetColorU32(IM_COL32(126, 126, 132, 255), alpha), note);
			}

			{
				const float dot_y = mn.y + 15.f;
				float dx = mx.x - 12.f - (float)kStyles * 7.f;
				for (int i = 0; i < kStyles; ++i)
				{
					const bool on = (i == si);
					const float r = on ? 2.6f : 1.7f;
					dl->AddCircleFilled(ImVec2(dx + (float)i * 7.f, dot_y), r,
						ImGui::GetColorU32(on ? IM_COL32(236, 236, 240, 255) : IM_COL32(255, 255, 255, 40), alpha), 10);
				}

				if (settings::viz::pin_style < 0 && settings::viz::demo_switch)
				{
					const float px = dx + (float)si * 7.f;
					dl->AddLine(ImVec2(px - 3.4f, dot_y + 6.f), ImVec2(px + 3.4f, dot_y + 6.f),
						ImGui::GetColorU32(IM_COL32(255, 255, 255, 26), alpha), 1.f);
					dl->AddLine(ImVec2(px - 3.4f, dot_y + 6.f), ImVec2(px - 3.4f + 6.8f * shown_progress(), dot_y + 6.f),
						ImGui::GetColorU32(IM_COL32(236, 236, 240, 200), alpha), 1.f);
				}
			}
		}

		const float body_y = mn.y + head_h + 6.f;
		const float body_h = ImMax(90.f, (mx.y - body_y) - 26.f);
		const float gap = 12.f;
		const float w = mx.x - mn.x;
		const float chase_w = ImMax(200.f, w * 0.56f);

		const ImVec2 c0(mn.x, body_y), c1(mn.x + chase_w, body_y + body_h);
		const ImVec2 r0(c1.x + gap, body_y), r1(mx.x, body_y + body_h);

		chase_well(dl, c0, c1, alpha, si);
		curves_well(dl, r0, r1, alpha, si, sx, sy);

		if (fs)
		{
			const float y = mx.y - 16.f;
			char buf[96];

			snprintf(buf, sizeof buf, "x %.2f   y %.2f   per tick%s", factor(si, sx), factor(si, sy),
				mouse_mode() ? ", whole counts" : "");
			dl->AddText(fs, fs->FontSize, ImVec2(mn.x, y), ImGui::GetColorU32(IM_COL32(150, 150, 156, 255), alpha), buf);

			const float f = ImMax(0.001f, factor(si, (sx + sy) * 0.5f));
			const int settle = f >= 0.999f ? 1 : (int)ceilf(logf(0.05f) / logf(1.f - f));
			if (mouse_mode())
				snprintf(buf, sizeof buf, "settles in ~%d ticks to half a count", settle);
			else
				snprintf(buf, sizeof buf, "settles in ~%d ticks", settle);
			const ImVec2 ts = fs->CalcTextSizeA(fs->FontSize, FLT_MAX, 0.f, buf);
			dl->AddText(fs, fs->FontSize, ImVec2((mn.x + mx.x) * 0.5f - ts.x * 0.5f, y),
				ImGui::GetColorU32(IM_COL32(190, 220, 255, 255), alpha), buf);

			snprintf(buf, sizeof buf, "err %.2f %s   held %.2f", to_units(d.err_avg), unit(),
				to_units(ImMax(0.f, d.err_avg * (1.f - f))));
			const ImVec2 ts2 = fs->CalcTextSizeA(fs->FontSize, FLT_MAX, 0.f, buf);
			dl->AddText(fs, fs->FontSize, ImVec2(mx.x - ts2.x, y), ImGui::GetColorU32(IM_COL32(150, 150, 156, 255), alpha), buf);
		}
	}

}
