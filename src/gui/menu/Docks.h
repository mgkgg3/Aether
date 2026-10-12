#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>

#include <gui/AetherIcons.h>
#include <gui/menu/Anim.h>
#include <gui/menu/HomePage.h>
#include <gui/menu/SyncBridge.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace docks
{
	using settings::docks::kCount;

	struct info_t { const char* title; int icon; float w; };

	inline const info_t& info(int i)
	{
		namespace ic = aether::icons;
		static const info_t k[settings::docks::Count] =
		{
			{ "Session",   (int)ic::Id::Bars,      228.f },
			{ "Nearby",    (int)ic::Id::Users,     228.f },
			{ "Sonar",     (int)ic::Id::Satellite, 234.f },
		};
		const int n = (int)settings::docks::Count;
		return k[(i < 0) ? 0 : (i >= n ? n - 1 : i)];
	}

	namespace dim
	{
		inline constexpr float PadX      = 14.f;
		inline constexpr float PadTop    = 12.f;
		inline constexpr float HeadH     = 30.f;
		inline constexpr float BodyGap   = 10.f;
		inline constexpr float RowH      = 19.f;
		inline constexpr float PadBot    = 12.f;
		inline constexpr float TraceH    = 32.f;
		inline constexpr float DialD     = 72.f;
		inline constexpr float Rounding  = 12.f;
		inline constexpr float RuleA     = 14.f;
		inline constexpr float MarkD     = 15.f;
		inline constexpr float MarkGap   = 8.f;
		inline constexpr float ArmPad    = 17.f;
	}

	struct ink_t
	{
		ImU32 key;
		ImU32 name;
		ImU32 dim;
		ImU32 val;
		ImU32 live;
		ImU32 warn;
		ImU32 bad;
		ImU32 sep;
		ImU32 rule;
		ImU32 edge;
	};

	inline ink_t ink()
	{
		ink_t k;
		k.key  = IM_COL32(238, 238, 242, 255);
		k.name = IM_COL32(186, 186, 192, 255);
		k.dim  = IM_COL32(132, 132, 140, 255);
		k.val  = IM_COL32(226, 226, 230, 255);
		k.live = IM_COL32(126, 214, 132, 255);
		k.warn = IM_COL32(226, 196, 106, 255);
		k.bad  = IM_COL32(220, 104, 100, 255);
		k.sep  = IM_COL32(124, 124, 130, 200);
		k.rule = IM_COL32(255, 255, 255, 14);
		k.edge = IM_COL32(255, 255, 255, 26);
		return k;
	}

	struct runtime_t
	{
		float drawn_x[kCount]{}, drawn_y[kCount]{};
		bool  placed = false;

		static constexpr int kNearby = 5;
		struct nearby_row { char name[64]; float dist; float health; float max_health; bool enemy; };
		nearby_row nearby[kNearby]{};
		int  nearby_n     = 0;
		int  nearby_total = 0;
		int  nearby_frame = -1;

		int   drag_id = -1;
		float grab_x = 0.f, grab_y = 0.f;
		float hover_t[kCount]{};
		float fold_t[kCount]{};
		float open_t[kCount]{};
	};

	inline runtime_t& rt()
	{
		static runtime_t r;
		return r;
	}

	inline bool on()
	{
		return settings::docks::enabled;
	}

	inline float s(float v) { return v * ImClamp(settings::docks::scale, 0.75f, 1.5f); }

	inline float dur(float seconds)
	{
		const float sp = ImClamp(settings::docks::anim_speed, 0.5f, 2.f);
		return settings::docks::animate ? (seconds / sp) : 0.f;
	}

	inline ImU32 a(ImU32 c, float alpha) { return ImGui::GetColorU32(c, alpha); }

	inline void txt(ImDrawList* dl, ImFont* f, ImVec2 p, ImU32 col, const char* t, float alpha)
	{
		if (!f || !t || !t[0] || alpha <= 0.004f) return;
		const ImVec2 sp(floorf(p.x + 0.5f), floorf(p.y + 0.5f));
		dl->AddText(f, f->FontSize, ImVec2(sp.x, sp.y + 1.f), a(IM_COL32(0, 0, 0, 41), alpha), t);
		dl->AddText(f, f->FontSize, sp, a(col, alpha), t);
	}

	inline float tw(ImFont* f, const char* t) { return f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, t).x; }

	inline void face(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float alpha, float hot)
	{
		const float o = ImClamp(settings::docks::opacity, 0.35f, 1.f);
		ImU32 fill = g_T.windowBg;
		if (o < 0.999f)
		{
			const int A = (int)((float)((fill >> IM_COL32_A_SHIFT) & 0xFF) * o + 0.5f);
			fill = (fill & ~IM_COL32_A_MASK) | ((ImU32)A << IM_COL32_A_SHIFT);
		}

		DrawBackdropRect(dl, p0, p1, dim::Rounding);
		dl->AddRectFilled(p0, p1, a(fill, alpha), dim::Rounding);

		const float lift = settings::docks::hover_lift ? ImClamp(hot, 0.f, 1.f) : 0.f;
		dl->AddRect(p0, p1, a(IM_COL32(255, 255, 255, (int)(26.f + 20.f * lift)), alpha),
			dim::Rounding, 0, g_T.borderThickness);
	}

	inline float header(ImDrawList* dl, int id, ImVec2 p0, ImVec2 p1, float alpha,
		bool hovered, const char* meta)
	{
		const ink_t k = ink();
		const bool fold_on = settings::docks::fold_on_click;
		const float arm = fold_on ? dim::ArmPad : 0.f;
		const float cy = p0.y + dim::PadTop + g_FontBold->FontSize * 0.5f;

		if (settings::docks::show_titles)
		{

			const float mx = p0.x + dim::PadX + dim::MarkD * 0.5f;
			const ImU32 mk = a(k.name, alpha * (hovered ? 1.f : 0.82f));
			if (!aether::fonts::draw_icon(dl, aether::icons::codepoint((aether::icons::Id)info(id).icon),
				ImVec2(mx, cy), dim::MarkD * 0.92f, mk))
				dl->AddCircle(ImVec2(mx, cy), dim::MarkD * 0.36f, mk, 20, 1.4f);

			txt(dl, g_FontBold, ImVec2(p0.x + dim::PadX + dim::MarkD + dim::MarkGap,
				p0.y + dim::PadTop), k.key, info(id).title, alpha * (hovered ? 1.f : 0.92f));
		}

		if (meta && meta[0])
		{
			const float mw = tw(g_FontSmall, meta);
			txt(dl, g_FontSmall, ImVec2(p1.x - dim::PadX - arm - mw, p0.y + dim::PadTop + 1.f),
				k.dim, meta, alpha);
		}

		if (fold_on)
		{

			const float cx = p1.x - dim::PadX - 4.f;
			const float t = ImClamp(rt().fold_t[id], 0.f, 1.f);
			const float a2 = 3.4f, b2 = 1.9f;
			const ImU32 col = a(k.name, alpha * (hovered ? 0.95f : 0.6f));
			const ImVec2 A(Lerpf(-a2, -b2, t), Lerpf(-b2, -a2, t));
			const ImVec2 B(Lerpf(0.f,  b2, t), Lerpf( b2, 0.f, t));
			const ImVec2 C(Lerpf( a2, -b2, t), Lerpf(-b2,  a2, t));
			dl->AddLine(ImVec2(cx + A.x, cy + A.y), ImVec2(cx + B.x, cy + B.y), col, 1.4f);
			dl->AddLine(ImVec2(cx + B.x, cy + B.y), ImVec2(cx + C.x, cy + C.y), col, 1.4f);
		}

		const float y = p0.y + dim::HeadH;
		dl->AddLine(ImVec2(p0.x + dim::PadX, y - 0.5f), ImVec2(p1.x - dim::PadX, y - 0.5f), a(k.rule, alpha), 1.f);
		return y + dim::BodyGap;
	}

	inline void row(ImDrawList* dl, ImVec2 c0, ImVec2 c1, float y, float lead_w,
		const char* lead, const char* name, const char* tail, float alpha,
		ImU32 lead_col, ImU32 name_col, ImU32 tail_col)
	{
		txt(dl, g_FontBold, ImVec2(c0.x, y + 3.f), lead_col, lead, alpha);
		if (name && name[0])
			txt(dl, g_FontRegular, ImVec2(c0.x + lead_w + 10.f, y + 3.f), name_col, name, alpha);
		if (tail && tail[0])
		{
			const float w = tw(g_FontSmall, tail);
			txt(dl, g_FontSmall, ImVec2(c1.x - w, y + 4.f), tail_col, tail, alpha);
		}
	}

	inline void reading(ImDrawList* dl, float& x, float y, float alpha, const ink_t& k,
		const char* val, const char* unit, ImU32 col)
	{
		txt(dl, g_FontBold, ImVec2(x, y), col, val, alpha);
		x += tw(g_FontBold, val) + 5.f;
		txt(dl, g_FontRegular, ImVec2(x, y), k.dim, unit, alpha);
		x += tw(g_FontRegular, unit);
	}

	inline void reading_dot(ImDrawList* dl, float& x, float y, float alpha, const ink_t& k)
	{
		dl->AddCircleFilled(ImVec2(x + 7.5f, y + 6.5f), 1.6f, a(k.sep, alpha));
		x += 17.f;
	}

	inline void trace_band(ImDrawList* dl, ImVec2 g0, ImVec2 g1, float alpha, const ink_t& k)
	{
		dl->AddLine(ImVec2(g0.x, g1.y - 0.5f), ImVec2(g1.x, g1.y - 0.5f), a(k.rule, alpha), 1.f);
	}

	inline void polyline(ImDrawList* dl, const ImVec2* pts, int n, ImU32 col, float alpha, float th)
	{
		if (n < 2) return;
		dl->AddPolyline(pts, n, a(col, alpha), 0, th);
	}

	inline void dial(ImDrawList* dl, ImVec2 c, float r, float alpha, const ink_t& k,
		float sweep_deg, bool sweep_hot)
	{
		dl->AddCircle(c, r, a(IM_COL32(255, 255, 255, 22), alpha), 48, 1.f);
		dl->AddCircle(c, r * 0.58f, a(IM_COL32(255, 255, 255, 12), alpha), 40, 1.f);
		dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), a(IM_COL32(255, 255, 255, 14), alpha), 1.f);
		dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), a(IM_COL32(255, 255, 255, 14), alpha), 1.f);

		if (sweep_deg >= 0.f)
		{
			const float ang = sweep_deg * 0.0174532925f;
			const ImU32 hand = sweep_hot ? a(k.live, alpha) : a(k.key, alpha * 0.55f);
			dl->AddLine(c, ImVec2(c.x + cosf(ang) * r, c.y + sinf(ang) * r), hand, 1.4f);
		}
	}

	inline bool mouse_in(ImVec2 mn, ImVec2 mx, float pad = 0.f)
	{
		const ImVec2 m = ImGui::GetIO().MousePos;
		return m.x >= mn.x - pad && m.x <= mx.x + pad && m.y >= mn.y - pad && m.y <= mx.y + pad;
	}

	inline bool mouse_down()
	{
#ifdef _WIN32
		return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
#else
		return ImGui::IsMouseDown(ImGuiMouseButton_Left);
#endif
	}

	inline void refresh_nearby()
	{
		runtime_t& r = rt();
		const int f = ImGui::GetFrameCount();
		if (r.nearby_frame == f) return;
		r.nearby_frame = f;
		r.nearby_n = r.nearby_total = 0;
		if (!sync::hooks().players_snapshot) return;

		static std::vector<sync::PlayerRow> rows;
		rows.resize(sync::kMaxPlayerRows);
		const int total = sync::hooks().players_snapshot(rows.data(), (int)rows.size());

		int idx[64], n = 0;
		for (int i = 0; i < total && i < sync::kMaxPlayerRows && n < 64; ++i)
		{
			const sync::PlayerRow& p = rows[(size_t)i];
			if (!p.valid || p.local || p.dead || p.distance < 0.f) continue;
			idx[n++] = i;
		}
		for (int i = 0; i < n; ++i)
			for (int j = i + 1; j < n; ++j)
				if (rows[(size_t)idx[j]].distance < rows[(size_t)idx[i]].distance)
				{
					const int t = idx[i];
					idx[i] = idx[j];
					idx[j] = t;
				}

		r.nearby_total = n;
		r.nearby_n = (n < runtime_t::kNearby) ? n : runtime_t::kNearby;
		for (int i = 0; i < r.nearby_n; ++i)
		{
			const sync::PlayerRow& p = rows[(size_t)idx[i]];
			const char* nm = p.display_name[0] ? p.display_name : p.name;
			snprintf(r.nearby[i].name, sizeof r.nearby[i].name, "%s", nm);
			r.nearby[i].dist = p.distance;
			r.nearby[i].health = p.health;
			r.nearby[i].max_health = p.max_health;
			r.nearby[i].enemy = p.enemy;
		}
	}

	inline void meta_for(int i, char* out, int cap)
	{
		out[0] = 0;
		switch (i)
		{
		case settings::docks::Session:
		{
			sync::SessionInfo sn{};
			if (sync::hooks().session && sync::hooks().session(sn))
				snprintf(out, (size_t)cap, "%.14s", sn.place_name[0] ? sn.place_name : "attached");
			else
				snprintf(out, (size_t)cap, "no game");
			break;
		}
		case settings::docks::Nearby:
			refresh_nearby();
			if (rt().nearby_total <= 0) snprintf(out, (size_t)cap, "nobody");
			else snprintf(out, (size_t)cap, "%d in range", rt().nearby_total);
			break;
		default:
		{
			sync::SonarInfo sn{};
			const bool have = sync::hooks().sonar_info && sync::hooks().sonar_info(sn);
			snprintf(out, (size_t)cap, "%s", !have ? "not linked" : (sn.active ? "sweeping" : "off"));
			break;
		}
		}
	}

	inline void body_session(ImDrawList* dl, ImVec2 c0, ImVec2 c1, float alpha)
	{
		runtime_t& r = rt();
		const ink_t k = ink();
		home_page::tick(home_page::sample_players());
		home_page::model_t& m = home_page::model();
		sync::SessionInfo sn{};
		const bool have = sync::hooks().session && sync::hooks().session(sn);

		char buf[64];
		home_page::fmt_duration(buf, sizeof buf, m.uptime);
		float x = c0.x;
		reading(dl, x, c0.y + 3.f, alpha, k, buf, "uptime", k.live);
		reading_dot(dl, x, c0.y + 3.f, alpha, k);
		snprintf(buf, sizeof buf, "%d", have ? sn.players : 0);
		reading(dl, x, c0.y + 3.f, alpha, k, buf, "players", k.live);

		const ImVec2 g0(c0.x, c0.y + dim::RowH + 7.f), g1(c1.x, c1.y);
		trace_band(dl, g0, g1, alpha, k);
		const float top = g0.y + 2.f, bot = g1.y - 1.5f;

		const int n = (int)(m.count < home_page::kHist ? m.count : home_page::kHist);
		if (n >= 2)
		{
			float vmax = 4.f;
			for (int i = 0; i < n; ++i) vmax = ImMax(vmax, home_page::at(m, i).players);
			vmax = ceilf(vmax * 1.15f);

			ImVec2 pts[home_page::kHist];
			for (int i = 0; i < n; ++i)
				pts[i] = ImVec2(g0.x + (g1.x - g0.x) * ((float)i / (float)(n - 1)),
					bot - (bot - top) * (home_page::at(m, i).players / vmax));
			polyline(dl, pts, n, IM_COL32(126, 214, 132, 255), alpha * 0.9f, 1.2f);
			dl->AddCircleFilled(pts[n - 1], 2.f, a(k.live, alpha), 12);
		}
		else
		{
			txt(dl, g_FontSmall, ImVec2(g0.x, (g0.y + g1.y) * 0.5f - g_FontSmall->FontSize * 0.5f),
				k.dim, "collecting", alpha);
		}

		if (have && sn.ping_ms >= 0.f)
		{
			snprintf(buf, sizeof buf, "tick %.0f ms", sn.ping_ms);
			txt(dl, g_FontSmall, ImVec2(c1.x - tw(g_FontSmall, buf), c1.y + 1.f), k.dim, buf, alpha);
		}
	}

	inline void body_nearby(ImDrawList* dl, ImVec2 c0, ImVec2 c1, float alpha)
	{
		runtime_t& r = rt();
		const ink_t k = ink();
		refresh_nearby();

		if (r.nearby_n <= 0)
		{
			const char* msg = sync::hooks().players_snapshot ? "nobody in range" : "no game attached";
			txt(dl, g_FontRegular, ImVec2(c0.x, c0.y + 3.f), k.dim, msg, alpha);
			return;
		}

		float lead_w = 0.f;
		char lead[runtime_t::kNearby][16];
		for (int i = 0; i < r.nearby_n; ++i)
		{
			snprintf(lead[i], sizeof lead[i], "%dm", (int)(r.nearby[i].dist + 0.5f));
			lead_w = ImMax(lead_w, tw(g_FontBold, lead[i]));
		}

		const float farthest = ImMax(1.f, r.nearby[r.nearby_n - 1].dist);
		for (int i = 0; i < r.nearby_n; ++i)
		{
			const float y = c0.y + (float)i * dim::RowH;
			char tail[32] = "";
			if (r.nearby[i].max_health > 0.f)
				snprintf(tail, sizeof tail, "%.0f hp", r.nearby[i].health);

			row(dl, c0, c1, y, lead_w, lead[i], r.nearby[i].name, tail, alpha,
				k.key, r.nearby[i].enemy ? k.live : k.name, k.dim);

			const float t = ImClamp(1.f - r.nearby[i].dist / (farthest + 1.f), 0.f, 1.f);
			dl->AddRectFilled(ImVec2(c0.x, y + dim::RowH - 5.f), ImVec2(c0.x + (c1.x - c0.x) * 0.34f, y + dim::RowH - 4.f),
				a(k.rule, alpha), 0.5f);
			dl->AddRectFilled(ImVec2(c0.x, y + dim::RowH - 5.f), ImVec2(c0.x + (c1.x - c0.x) * 0.34f * t, y + dim::RowH - 4.f),
				a(k.name, alpha * 0.45f), 0.5f);
		}
	}

	inline void body_sonar(ImDrawList* dl, ImVec2 c0, ImVec2 c1, float alpha)
	{
		const ink_t k = ink();
		sync::SonarInfo sn{};
		const bool have = sync::hooks().sonar_info && sync::hooks().sonar_info(sn);

		const float d = dim::DialD;
		const ImVec2 c(c0.x + d * 0.5f, c0.y + d * 0.5f);
		const float rad = d * 0.5f - 2.f;
		dial(dl, c, rad, alpha, k, have ? sn.sweep : -1.f, sn.has_ping);

		if (!have)
		{
			txt(dl, g_FontSmall, ImVec2(c0.x + d + 12.f, c0.y + d * 0.5f - g_FontSmall->FontSize * 0.5f),
				k.dim, "not linked", alpha);
			return;
		}

		if (sn.has_ping)
		{
			const float pa = sn.bearing * 0.0174532925f;
			const float fade = ImClamp(0.35f + sn.glow, 0.f, 1.f);
			const ImVec2 q(c.x + cosf(pa) * rad * 0.72f, c.y + sinf(pa) * rad * 0.72f);
			dl->AddCircle(q, 4.f + 7.f * (1.f - fade), a(k.warn, alpha * fade * 0.7f), 20, 1.2f);
			dl->AddCircleFilled(q, 2.2f, a(k.warn, alpha * fade), 14);
			dl->AddLine(c, q, a(k.warn, alpha * fade * 0.5f), 1.f);
		}

		const float lx = c0.x + d + 12.f;
		const float lead_w = 54.f;
		char lead[32], tail[32];

		if (sn.has_ping)
		{
			txt(dl, g_FontBold, ImVec2(lx, c0.y + 3.f), k.live, sn.name, alpha);
		}
		else
		{
			txt(dl, g_FontRegular, ImVec2(lx, c0.y + 3.f), k.dim,
				sn.active ? "sweeping" : "not sweeping", alpha);
		}

		snprintf(lead, sizeof lead, "%.0f", sn.has_ping ? sn.distance : 0.f);
		row(dl, ImVec2(lx, c0.y), c1, c0.y + dim::RowH, lead_w, lead, "studs", nullptr, alpha, k.key, k.name, k.dim);

		if (sn.has_ping)
		{
			snprintf(lead, sizeof lead, "%.0f", sn.bearing);
			snprintf(tail, sizeof tail, "%.0f hp", sn.health);
			row(dl, ImVec2(lx, c0.y), c1, c0.y + dim::RowH * 2.f, lead_w, lead, "bearing", tail, alpha, k.key, k.name, k.dim);
		}
		else
		{
			txt(dl, g_FontSmall, ImVec2(lx, c0.y + dim::RowH * 2.f + 4.f), k.dim, "nothing on the sweep", alpha);
		}
	}

	inline float body_h(int i)
	{
		switch (i)
		{
		case settings::docks::Session:
			return dim::RowH + 7.f + dim::TraceH;
		case settings::docks::Nearby:
			refresh_nearby();
			return (rt().nearby_n > 0) ? (float)rt().nearby_n * dim::RowH : 18.f;
		default:
			return ImMax(dim::DialD, dim::RowH * 3.f);
		}
	}

	inline float plate_h(int i)
	{
		return dim::HeadH + dim::BodyGap + body_h(i) + dim::PadBot;
	}

	inline bool drag_update(int i, ImVec2 p0, ImVec2 p1, ImVec2 display, bool over_header)
	{
		if (!g_hwnd) return false;

		POINT cur = {};
		RECT  wr  = {};
		if (!::GetCursorPos(&cur) || !::GetWindowRect(g_hwnd, &wr)) return false;
		const float mx = (float)(cur.x - wr.left), my = (float)(cur.y - wr.top);
		const bool  down = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		const float w = p1.x - p0.x, h = p1.y - p0.y;

		runtime_t& r = rt();
		if (r.drag_id != i)
		{
			if (!down || r.drag_id >= 0) return false;
			if (!over_header || mx < p0.x - 4.f || mx > p1.x + 4.f) return false;

			const bool on_arm = settings::docks::fold_on_click &&
				mx > p0.x + w - dim::PadX - dim::ArmPad;
			if (on_arm)
			{
				settings::docks::folded[i] = !settings::docks::folded[i];
				return false;
			}

			r.drag_id = i;
			r.grab_x = mx - settings::docks::x[i];
			r.grab_y = my - settings::docks::y[i];
			return true;
		}
		if (!down)
		{

			if (settings::docks::snap)
			{
				const float g = settings::docks::snap_px, gap = settings::docks::snap_gap;
				float tx = settings::docks::x[i], ty = settings::docks::y[i];
				if (tx < g) tx = gap;
				else if (tx + w > display.x - g) tx = display.x - w - gap;
				if (ty < g) ty = gap;
				else if (ty + h > display.y - g) ty = display.y - h - gap;
				settings::docks::x[i] = tx;
				settings::docks::y[i] = ty;
			}
			r.drag_id = -1;
			return false;
		}

		settings::docks::x[i] = ImClamp(mx - r.grab_x, 0.f, ImMax(0.f, display.x - w));
		settings::docks::y[i] = ImClamp(my - r.grab_y, 0.f, ImMax(0.f, display.y - h));
		return true;
	}

	inline void draw_one(ImDrawList* dl, int i, ImVec2 display, bool interactive)
	{
		runtime_t& r = rt();
		const ink_t k = ink();

		r.open_t[i] = anim::tween(anim::id(&r), 0x3000u + (ImGuiID)i, settings::docks::open[i] ? 1.f : 0.f,
			dur(0.28f), anim::OutCubic, anim::dt(), 0.f);
		r.fold_t[i] = anim::tween(anim::id(&r), 0x4000u + (ImGuiID)i, settings::docks::folded[i] ? 1.f : 0.f,
			dur(0.22f), anim::InOutCubic, anim::dt(), 0.f);

		const float open = r.open_t[i];
		if (open <= 0.004f) return;

		if (!r.placed)
		{
			r.placed = true;
			for (int j = 0; j < kCount; ++j) { r.drawn_x[j] = settings::docks::x[j]; r.drawn_y[j] = settings::docks::y[j]; }
		}

		const float w = s(info(i).w);
		float h = s(plate_h(i));
		const float px = ImClamp(r.drawn_x[i], 2.f, ImMax(2.f, display.x - w - 2.f));
		const float py = ImClamp(r.drawn_y[i], 2.f, ImMax(2.f, display.y - h - 2.f));

		const float fold = ImClamp(r.fold_t[i], 0.f, 1.f);
		h = s(dim::HeadH) + (h - s(dim::HeadH)) * (1.f - fold);

		const float sc = 0.94f + 0.06f * anim::ease(anim::OutCubic, open);
		const ImVec2 c(px + w * 0.5f, py + h * 0.5f);
		const ImVec2 p0(c.x - w * 0.5f * sc, c.y - h * 0.5f * sc);
		const ImVec2 p1(c.x + w * 0.5f * sc, c.y + h * 0.5f * sc);
		const float alpha = open;

		const bool hot = interactive && mouse_in(p0, p1);
		const bool in_head = hot && ImGui::GetIO().MousePos.y <= p0.y + dim::HeadH * sc;
		r.hover_t[i] = anim::follow(r.hover_t[i], hot ? 1.f : 0.f, hot ? 0.05f : 0.12f, anim::dt());
		drag_update(i, p0, p1, display, in_head);

		r.drawn_x[i] = anim::follow(r.drawn_x[i], settings::docks::x[i], r.drag_id == i ? 0.012f : 0.05f, anim::dt());
		r.drawn_y[i] = anim::follow(r.drawn_y[i], settings::docks::y[i], r.drag_id == i ? 0.012f : 0.05f, anim::dt());

		face(dl, p0, p1, alpha, (r.drag_id == i) ? 1.f : r.hover_t[i]);

		char meta[48];
		meta_for(i, meta, (int)sizeof meta);
		const float body_y = header(dl, i, p0, p1, alpha, in_head, meta);

		if (h > s(dim::HeadH) + s(dim::BodyGap) + 4.f)
		{
			const ImVec2 c0(p0.x + s(dim::PadX), body_y);
			const ImVec2 c1(p1.x - s(dim::PadX), p1.y - s(dim::PadBot));
			dl->PushClipRect(ImVec2(p0.x + 1.f, body_y - 2.f), ImVec2(p1.x - 1.f, p1.y - 1.f), true);
			switch (i)
			{
			case settings::docks::Session: body_session(dl, c0, c1, alpha); break;
			case settings::docks::Nearby:  body_nearby(dl, c0, c1, alpha); break;
			case settings::docks::Sonar:   body_sonar(dl, c0, c1, alpha); break;
			default: break;
			}
			dl->PopClipRect();
		}
	}

	inline void draw(ImDrawList* dl, ImVec2 display, bool interactive)
	{
		if (!on() || !dl) return;

		runtime_t& r = rt();

		for (int i = 0; i < kCount; ++i)
		{
			if (!settings::docks::open[i]) continue;
			draw_one(dl, i, display, interactive);
		}
	}

	inline bool pointer_owned_by_panel(ImVec2 mn, ImVec2 mx, bool menu_open)
	{
		return menu_open && mouse_in(mn, mx, 2.f);
	}

	inline bool any_open()
	{
		for (int i = 0; i < kCount; ++i)
			if (settings::docks::open[i]) return true;
		return false;
	}
}
