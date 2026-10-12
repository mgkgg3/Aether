#pragma once

#include <imgui/imgui.h>

#include <settings.h>

#include <gui/menu/Anim.h>
#include <gui/menu/SyncBridge.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

namespace home_page
{

	inline constexpr int kHist = 240;
	inline constexpr float kSampleEvery = 0.5f;

	struct sample_t { float players = 0.f; float fps = 0.f; };

	struct model_t
	{
		sample_t hist[kHist]{};
		int   head = 0;
		int   count = 0;
		float acc = 0.f;
		float t0 = -1.f;
		float uptime = 0.f;
		float shown = 0.f;
		int   last_players = 0;

		int   last_frame = -1;
	};

	inline model_t& model()
	{
		static model_t m;
		return m;
	}

	inline const sample_t& at(const model_t& m, int i)
	{
		const int n = (int)(m.count < kHist ? m.count : kHist);
		const int oldest = (m.head - n + kHist * 2) % kHist;
		return m.hist[(oldest + i) % kHist];
	}

	inline int sample_players()
	{
		sync::SessionInfo s{};
		if (sync::hooks().session && sync::hooks().session(s))
			return s.players;
		if (sync::hooks().players_snapshot)
		{
			static std::vector<sync::PlayerRow> rows;
			rows.resize(sync::kMaxPlayerRows);
			return ImMax(0, sync::hooks().players_snapshot(rows.data(), (int)rows.size()));
		}
		return 0;
	}

	inline void tick(int players)
	{
		model_t& m = model();
		const int frame = (int)ImGui::GetFrameCount();
		if (m.last_frame == frame) return;
		m.last_frame = frame;
		const float dt = anim::dt();

		if (m.t0 < 0.f) m.t0 = (float)ImGui::GetTime();
		m.uptime = (float)ImGui::GetTime() - m.t0;

		m.acc += dt;
		if (m.acc >= kSampleEvery)
		{
			m.hist[m.head] = sample_t{ (float)players, ImGui::GetIO().Framerate };
			m.head = (m.head + 1) % kHist;
			m.count = m.count < kHist ? m.count + 1 : kHist;
			m.acc = 0.f;
			m.last_players = players;
		}
	}

	inline float reveal()
	{
		model_t& m = model();
		m.shown = anim::tween(anim::id(&m), 0x5E11u, 1.f, 1.1f, anim::OutCubic, anim::dt(), 0.f);
		return m.shown;
	}

	inline void fmt_duration(char* buf, size_t n, float seconds)
	{
		if (seconds < 0.f) seconds = 0.f;
		const int t = (int)seconds;
		snprintf(buf, n, "%02d:%02d:%02d", t / 3600, (t / 60) % 60, t % 60);
	}

	inline void fmt_u64(char* buf, size_t n, unsigned long long v)
	{
		if (!v) { snprintf(buf, n, "-"); return; }
		snprintf(buf, n, "%llu", v);
	}

	inline const char* greeting()
	{
		const int h = [&] {
			const time_t now = time(nullptr);
			tm lt{};
			localtime_s(&lt, &now);
			return lt.tm_hour;
		}();
		if (h < 5)  return "Good night";
		if (h < 12) return "Good morning";
		if (h < 18) return "Good afternoon";
		return "Good evening";
	}

	inline void fmt_clock(char* buf, size_t n, bool h24)
	{
		const time_t now = time(nullptr);
		tm lt{};
		localtime_s(&lt, &now);
		if (h24) snprintf(buf, n, "%02d:%02d", lt.tm_hour, lt.tm_min);
		else     snprintf(buf, n, "%d:%02d %s", (lt.tm_hour % 12) == 0 ? 12 : (lt.tm_hour % 12),
			              lt.tm_min, lt.tm_hour < 12 ? "AM" : "PM");
	}

	inline void bloom(ImDrawList* dl, ImVec2 c, float r, ImU32 col, float strength)
	{
		if (strength <= 0.004f) return;
		const int steps = 5;
		for (int i = steps; i >= 1; --i)
		{
			const float t = (float)i / (float)steps;
			dl->AddCircleFilled(c, r * t, ImGui::GetColorU32(col, (1.f - t) * 0.34f * strength), 28);
		}
	}

	inline void avatar_box(ImDrawList* dl, ImVec2 mn, ImVec2 mx, long long uid, float alpha)
	{
		const float d = mx.x - mn.x;
		const float r = d * 0.5f;
		const ImVec2 c((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);

		dl->AddCircleFilled(c, r + 1.f, ImGui::GetColorU32(IM_COL32(14, 14, 18, 210), alpha), 48);

		bool drawn = false;
		if (sync::hooks().player_avatar_rounded && uid > 0)
			drawn = sync::hooks().player_avatar_rounded(dl, uid, mn, mx,
				ImGui::GetColorU32(IM_COL32(255, 255, 255, 255), alpha), r);

		if (!drawn)
		{

			const float s2 = d * 0.34f;
			dl->AddCircleFilled(ImVec2(c.x, c.y - s2 * 0.55f), s2 * 0.42f,
				ImGui::GetColorU32(IM_COL32(150, 150, 158, 255), alpha * 0.85f), 28);
			dl->PathClear();
			dl->PathArcTo(ImVec2(c.x, c.y + s2 * 1.06f), s2 * 0.92f, 3.14159f * 1.15f, 3.14159f * 1.85f, 24);
			dl->PathStroke(ImGui::GetColorU32(IM_COL32(150, 150, 158, 255), alpha * 0.85f), 0, 2.2f);
		}

		if (settings::home::avatar_ring)
			dl->AddCircle(c, r - 0.5f, ImGui::GetColorU32(IM_COL32(255, 255, 255, 34), alpha), 48, 1.f);
	}

	struct card_t
	{
		const char* label = "";
		const char* value = "";
		const char* sub = "";
		ImU32       accent = 0;
		int         light = -1;
	};

	inline void stat_card(ImDrawList* dl, ImVec2 mn, ImVec2 mx, const card_t& c, float t_reveal, float alpha)
	{
		const float a = alpha * t_reveal;
		if (a <= 0.01f) return;

		const float dy = (1.f - anim::ease(anim::OutCubic, t_reveal)) * 9.f;
		const ImVec2 p0(mn.x, mn.y + dy), p1(mx.x, mx.y + dy);

		dl->AddRectFilled(p0, p1, ImGui::GetColorU32(IM_COL32(255, 255, 255, 7), a), 12.f);
		dl->AddRect(p0, p1, ImGui::GetColorU32(IM_COL32(255, 255, 255, 22), a), 12.f, 0, 1.f);

		const float pad = 12.f;
		float y = p0.y + 11.f;

		{
			char up[32] = {};
			snprintf(up, sizeof up, "%s", c.label);
			for (char* p = up; *p; ++p) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32);
			float x = p0.x + pad;
			for (const char* p = up; *p; ++p)
			{
				char one[2] = { *p, 0 };
				ImGui::GetWindowDrawList()->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(x, y),
					ImGui::GetColorU32(IM_COL32(146, 146, 152, 255), a), one);
				x += g_FontSmall->CalcTextSizeA(g_FontSmall->FontSize, FLT_MAX, 0.f, one).x + 0.9f;
			}
			y += g_FontSmall->FontSize + 7.f;
		}

		{
			ImFont* f = g_FontValue ? g_FontValue : (g_FontBold ? g_FontBold : g_FontRegular);
			const float sz = f->FontSize;
			const ImU32 col = c.accent ? c.accent : g_T.textPrimary;
			dl->AddText(f, sz, SnapText(ImVec2(p0.x + pad, y)), ImGui::GetColorU32(col, a), c.value);

			if (c.light >= 0)
			{
				const ImVec2 ts = f->CalcTextSizeA(sz, FLT_MAX, 0.f, c.value);
				const float r = 3.6f;
				const ImVec2 lc(p0.x + pad + ts.x + 9.f, y + sz * 0.5f);
				ImU32 lcol = g_T.textFaint;
				if (c.light == 1) lcol = IM_COL32(112, 214, 126, 255);
				if (c.light == 2) lcol = IM_COL32(232, 178, 82, 255);
				const float pulse = c.light >= 1 ? (0.55f + 0.45f * anim::breath(2.1f)) : 1.f;
				if (c.light >= 1)
					dl->AddCircleFilled(lc, r * 2.6f, ImGui::GetColorU32(lcol, a * 0.16f * pulse), 20);
				dl->AddCircleFilled(lc, r, ImGui::GetColorU32(lcol, a * pulse), 20);
			}
			y += sz + 3.f;
		}

		if (c.sub && c.sub[0])
			ImGui::GetWindowDrawList()->AddText(g_FontSmall, g_FontSmall->FontSize,
				SnapText(ImVec2(p0.x + pad, y)), ImGui::GetColorU32(g_T.textFaint, a), c.sub);
	}

	inline float catmull(float p0, float p1, float p2, float p3, float t)
	{
		const float t2 = t * t, t3 = t2 * t;
		return 0.5f * ((2.f * p1) + (-p0 + p2) * t +
			(2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * t2 +
			(-p0 + 3.f * p1 - 3.f * p2 + p3) * t3);
	}

	inline void activity_chart(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float alpha)
	{
		model_t& m = model();
		const int n = (int)(m.count < kHist ? m.count : kHist);
		const float reveal = home_page::reveal();
		const float a = alpha * reveal;

		dl->AddRectFilled(mn, mx, ImGui::GetColorU32(IM_COL32(9, 9, 11, 128), alpha), 10.f);
		dl->AddRect(mn, mx, ImGui::GetColorU32(IM_COL32(255, 255, 255, 16), alpha), 10.f, 0, 1.f);

		const float padL = 14.f, padR = 14.f, padT = 12.f, padB = 12.f;
		const ImVec2 g0(mn.x + padL, mn.y + padT), g1(mx.x - padR, mx.y - padB);

		if (n < 3)
		{

			const char* msg = "Collecting data";
			ImFont* f = g_FontRegular;
			const ImVec2 ts = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, msg);
			const ImVec2 c((g0.x + g1.x) * 0.5f, (g0.y + g1.y) * 0.5f - ts.y * 0.5f);
			dl->AddText(f, f->FontSize, SnapText(ImVec2(c.x - ts.x * 0.5f, c.y)),
				ImGui::GetColorU32(g_T.textDim, alpha), msg);

			const int dots = 1 + (int)(anim::breath(0.9f, 0.f, 3.0f));
			for (int i = 0; i < dots; ++i)
			{
				const float r = 2.f;
				dl->AddCircleFilled(ImVec2(c.x + ts.x * 0.5f + 6.f + (float)i * 7.f, c.y + ts.y * 0.5f),
					r, ImGui::GetColorU32(IM_COL32(200, 200, 206, 255), alpha), 12);
			}

			dl->AddRect(g0, g1, ImGui::GetColorU32(IM_COL32(255, 255, 255, 12), alpha), 4.f, 0, 1.f);
			return;
		}

		float vmax = 4.f;
		for (int i = 0; i < n; ++i) vmax = ImMax(vmax, at(m, i).players);
		vmax = ceilf(vmax * 1.15f);
		if (vmax < 4.f) vmax = 4.f;

		if (settings::home::activity_grid)
		{
			for (int i = 1; i <= 3; ++i)
			{
				const float t = (float)i / 4.f;
				const float y = g1.y - (g1.y - g0.y) * t;
				dl->AddLine(ImVec2(g0.x, y), ImVec2(g1.x, y), ImGui::GetColorU32(IM_COL32(255, 255, 255, 9), alpha), 1.f);
			}
		}

		const int shown = ImMax(3, (int)((float)n * reveal + 0.5f));
		std::vector<ImVec2> pts;
		pts.reserve((size_t)shown * 2);
		const float dx = (n > 1) ? (g1.x - g0.x) / (float)(n - 1) : 0.f;
		for (int i = 0; i < shown; ++i)
		{
			const float v = at(m, i).players;
			const float x = g0.x + dx * (float)i;
			const float y = g1.y - (g1.y - g0.y) * (v / vmax);
			pts.push_back(ImVec2(x, y));
		}

		std::vector<ImVec2> path;
		if (settings::home::activity_smooth && pts.size() >= 2)
		{
			path.push_back(pts[0]);
			for (size_t i = 0; i + 1 < pts.size(); ++i)
			{
				const ImVec2 p0 = pts[i > 0 ? i - 1 : 0];
				const ImVec2 p1 = pts[i];
				const ImVec2 p2 = pts[i + 1];
				const ImVec2 p3 = pts[i + 2 < pts.size() ? i + 2 : pts.size() - 1];
				for (int s = 1; s <= 6; ++s)
				{
					const float t = (float)s / 6.f;
					path.push_back(ImVec2(catmull(p0.x, p1.x, p2.x, p3.x, t),
					                      catmull(p0.y, p1.y, p2.y, p3.y, t)));
				}
			}
		}
		else
		{
			path = pts;
		}

		if (settings::home::activity_fill && path.size() >= 2)
		{
			const float step = 3.f;
			for (float x = g0.x; x < g1.x; x += step)
			{

				float yTop = g1.y;
				for (size_t i = 0; i + 1 < path.size(); ++i)
				{
					if (x < path[i].x || x > path[i + 1].x) continue;
					const float u = (path[i + 1].x - path[i].x) > 0.0001f
						? (x - path[i].x) / (path[i + 1].x - path[i].x) : 0.f;
					yTop = path[i].y + (path[i + 1].y - path[i].y) * u;
					break;
				}
				if (yTop >= g1.y) continue;
				dl->AddRectFilledMultiColor(ImVec2(x, yTop), ImVec2(x + step, g1.y),
				ImGui::GetColorU32(IM_COL32(255, 255, 255, 30), a * 0.55f),
				ImGui::GetColorU32(IM_COL32(255, 255, 255, 30), a * 0.55f),
				ImGui::GetColorU32(IM_COL32(255, 255, 255, 0), a * 0.55f),
				ImGui::GetColorU32(IM_COL32(255, 255, 255, 0), a * 0.55f));
			}
		}

		if (path.size() >= 2)
			dl->AddPolyline(path.data(), (int)path.size(), ImGui::GetColorU32(IM_COL32(226, 226, 230, 235), a),
				ImDrawFlags_None, 1.4f);

		if (!path.empty())
			dl->AddCircleFilled(path.back(), 2.4f, ImGui::GetColorU32(IM_COL32(255, 255, 255, 255), a), 16);

		if (settings::home::activity_grid)
		{
			char buf[16];
			snprintf(buf, sizeof buf, "%d", (int)vmax);
			dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(g0.x + 2.f, g0.y - 1.f),
				ImGui::GetColorU32(g_T.textFaint, alpha), buf);
			snprintf(buf, sizeof buf, "%d", 0);
			dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(g0.x + 2.f, g1.y - g_FontSmall->FontSize + 2.f),
				ImGui::GetColorU32(g_T.textFaint, alpha), buf);
		}
	}

	struct near_t
	{
		sync::PlayerRow row{};
		float dist = 0.f;
	};

	inline void nearby_plate(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float alpha)
	{
		if (!sync::hooks().players_snapshot)
		{
			const char* msg = "No game attached";
			ImFont* f = g_FontRegular;
			const ImVec2 ts = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, msg);				dl->AddText(f, f->FontSize, SnapText(ImVec2(mn.x + 14.f, mn.y + 14.f)),
				ImGui::GetColorU32(g_T.textDim, alpha), msg);
			return;
		}

		static std::vector<sync::PlayerRow> rows;
		rows.resize(sync::kMaxPlayerRows);
		const int total = sync::hooks().players_snapshot(rows.data(), (int)rows.size());
		rows.resize((size_t)ImMax(0, total));

		std::vector<near_t> list;
		list.reserve(rows.size());
		for (const sync::PlayerRow& r : rows)
		{
			if (!r.valid || r.local || r.dead) continue;
			if (r.distance < 0.f) continue;
			near_t e; e.row = r; e.dist = r.distance;
			list.push_back(e);
		}
		std::sort(list.begin(), list.end(), [](const near_t& a, const near_t& b) { return a.dist < b.dist; });

		const int show = ImClamp(settings::home::nearby_rows, 1, 16);
		if (list.empty())
		{
			dl->AddText(g_FontRegular, g_FontRegular->FontSize, SnapText(ImVec2(mn.x + 14.f, mn.y + 14.f)),
				ImGui::GetColorU32(g_T.textDim, alpha), "Nobody in range");
			return;
		}

		float farthest = 1.f;
		for (int i = 0; i < ImMin(show, (int)list.size()); ++i)
			farthest = ImMax(farthest, list[(size_t)i].dist);

		float y = mn.y;
		for (int i = 0; i < ImMin(show, (int)list.size()); ++i)
		{
			const near_t& e = list[(size_t)i];
			const float h = 27.f;
			const ImVec2 r0(mn.x, y), r1(mx.x, y + h);

			const ImGuiID who = (ImGuiID)(e.row.instance ^ (unsigned long long)e.row.user_id * 2654435761ull);
			const float t = anim::tween(who, 0x1111u, 1.f, 0.45f, anim::OutCubic, anim::dt(), 0.f);
			const float a = alpha * t;
			const float dx = (1.f - anim::ease(anim::OutCubic, t)) * 10.f;

			const ImVec2 p0(r0.x + dx, r0.y), p1(r1.x + dx, r1.y);

			ImGui::SetCursorScreenPos(r0);
			ImGui::PushID(9000 + i);
			ImGui::InvisibleButton("##near", ImVec2(mx.x - mn.x, h));
			const bool hov = ImGui::IsItemHovered();
			ImGui::PopID();
			const float hv = anim::tween(who, 0x2222u, hov ? 1.f : 0.f, 0.18f,
				anim::OutQuad, anim::dt(), 0.f);
			if (hv > 0.01f)
				dl->AddRectFilled(p0, ImVec2(p1.x - 4.f, p1.y), ImGui::GetColorU32(IM_COL32(255, 255, 255, 14), hv * alpha), 6.f);

			float nx = p0.x + 10.f;
			if (settings::home::nearby_faces && e.row.user_id > 0)
			{
				const float d = 18.f;
				const ImVec2 amin(nx, p0.y + (h - d) * 0.5f), amax(amin.x + d, amin.y + d);
				const ImVec2 ac((amin.x + amax.x) * 0.5f, (amin.y + amax.y) * 0.5f);
				dl->AddCircleFilled(ac, d * 0.5f + 0.5f, ImGui::GetColorU32(IM_COL32(255, 255, 255, 16), a), 24);
				bool got_face = false;
				if (sync::hooks().player_avatar_rounded)
					got_face = sync::hooks().player_avatar_rounded(dl, e.row.user_id, amin, amax,
						ImGui::GetColorU32(IM_COL32(255, 255, 255, 255), a), d * 0.5f);
				if (!got_face)
				{

					char ini[2] = { 0, 0 };
					const char* nm = e.row.display_name[0] ? e.row.display_name : e.row.name;
					if (nm[0]) ini[0] = (nm[0] >= 'a' && nm[0] <= 'z') ? (char)(nm[0] - 32) : nm[0];
					if (ini[0])
						dl->AddText(g_FontBold, g_FontBold->FontSize,
							ImVec2(ac.x - g_FontBold->CalcTextSizeA(g_FontBold->FontSize, FLT_MAX, 0.f, ini).x * 0.5f,
								ac.y - g_FontBold->FontSize * 0.5f),
							ImGui::GetColorU32(IM_COL32(200, 200, 206, 255), a), ini);
				}
				nx += d + 9.f;
			}

			const char* name = e.row.display_name[0] ? e.row.display_name : e.row.name;
			dl->AddText(g_FontRegular, g_FontRegular->FontSize, SnapText(ImVec2(nx, p0.y + (h - g_FontRegular->FontSize) * 0.5f)),
				ImGui::GetColorU32(g_T.textPrimary, a), name);

			char dbuf[24];
			snprintf(dbuf, sizeof dbuf, "%dm", (int)(e.dist + 0.5f));
			const ImVec2 ds = g_FontSmall->CalcTextSizeA(g_FontSmall->FontSize, FLT_MAX, 0.f, dbuf);
			dl->AddText(g_FontSmall, g_FontSmall->FontSize,
				SnapText(ImVec2(p1.x - 10.f - ds.x, p0.y + (h - g_FontSmall->FontSize) * 0.5f)),
				ImGui::GetColorU32(g_T.textDim, a), dbuf);

			if (settings::home::nearby_bars)
			{
				const float bw = ImClamp(mx.x - mn.x - 20.f, 20.f, 220.f);
				const float t2 = ImClamp(e.dist / farthest, 0.f, 1.f);
				const ImVec2 b0(p1.x - 10.f - bw, p1.y - 4.f), b1(p1.x - 10.f, p1.y - 3.f);
				dl->AddRectFilled(b0, b1, ImGui::GetColorU32(IM_COL32(255, 255, 255, 12), a), 1.f);
				dl->AddRectFilled(b0, ImVec2(b0.x + bw * t2, b1.y),
					ImGui::GetColorU32(IM_COL32(226, 226, 230, 130), a), 1.f);
			}

			y += h;
		}
	}

	inline void draw()
	{
		model_t& m = model();

		sync::SessionInfo s{};
		const bool have = sync::hooks().session && sync::hooks().session(s);

		const int players = sample_players();
		tick(players);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const float L = g_ContentL, R = g_ContentR;
		const float W = R - L;
		const float d = anim::dt();

		char sbuf[64];

		float y = g_RowY + 6.f;
		{
			const float av = 44.f;

			ImFont* ft = g_FontDisplay ? g_FontDisplay : (g_FontTitle ? g_FontTitle : g_FontRegular);
			const float greet_sz = ft->FontSize;

			float text_x = L;
			if (settings::home::avatar)
			{
				avatar_box(dl, ImVec2(L, y), ImVec2(L + av, y + av), s.local_user_id, g_AlphaMul);
				text_x = L + av + 15.f;
			}

			const char* greet = settings::home::greeting ? greeting() : "Welcome back";
			const char* who = s.local_handle[0] ? s.local_handle : (s.local_name[0] ? s.local_name : "not attached");
			char line[160];
			snprintf(line, sizeof line, "%s,", greet);

			dl->AddText(ft, greet_sz, SnapText(ImVec2(text_x, y + 3.f)),
				ImGui::GetColorU32(g_T.textPrimary, g_AlphaMul), line);

			const ImVec2 gs = ft->CalcTextSizeA(greet_sz, FLT_MAX, 0.f, line);
			ImFont* fb = g_FontDisplayBold ? g_FontDisplayBold : ft;

			dl->AddText(fb, fb->FontSize, SnapText(ImVec2(text_x + gs.x + 6.f, y + 3.f)),
				ImGui::GetColorU32(g_T.textPrimary, g_AlphaMul), who);

			char clock[24] = {};
			if (settings::home::clock) fmt_clock(clock, sizeof clock, settings::home::clock_format == 1);
			snprintf(line, sizeof line, "Bring out greatness%s%s", clock[0] ? ", " : "", clock);
			dl->AddText(g_FontSmall, g_FontSmall->FontSize, SnapText(ImVec2(text_x, y + 30.f)),
				ImGui::GetColorU32(g_T.textDim, g_AlphaMul), line);

			(void)d;
			y += av + 22.f;
		}

		if (settings::home::cards)
		{
			const float gap = 12.f;
			const float cw = (W - gap * 3.f) / 4.f;
			const float ch = 76.f;

			card_t cards[4];

			{
			cards[0].label = "Connection";
			cards[0].value = have && s.attached ? "Connected" : "Waiting";

			cards[0].accent = have && s.attached ? g_T.textPrimary : IM_COL32(232, 178, 82, 255);
			cards[0].light = -1;

				static char conn_sub[32];
				if (s.ping_ms >= 0.f) snprintf(conn_sub, sizeof conn_sub, "tick %.0f ms", s.ping_ms);
				else                  snprintf(conn_sub, sizeof conn_sub, "no reader yet");
				cards[0].sub = conn_sub;
			}

			{
				static char up[24];
				fmt_duration(up, sizeof up, m.uptime);
				cards[1].label = "Uptime";
				cards[1].value = up;
				cards[1].sub = "since launch";
			}

			{
				static char pid[32];
				fmt_u64(pid, sizeof pid, s.place_id);
				cards[2].label = "Place Id";
				cards[2].value = pid;
				cards[2].sub = (s.place_name[0] ? s.place_name : "sample");
			}

			{
				static char pc[16];
				snprintf(pc, sizeof pc, "%d", players);
				cards[3].label = "Players";
				cards[3].value = pc;
				cards[3].sub = have && s.max_players ? "in list / cap" : "in list";
			}

			for (int i = 0; i < 4; ++i)
			{

				const float delay = settings::home::stagger * (float)i;
				const float t = anim::tween(anim::id(&m), 0x2000u + (ImGuiID)i, 1.f,
					settings::home::stagger * 3.f + 0.5f, anim::OutCubic, d, 0.f);
				const float tt = ImClamp((t - delay) / ImMax(0.001f, 1.f - delay), 0.f, 1.f);
				stat_card(dl, ImVec2(L + (cw + gap) * (float)i, y), ImVec2(L + (cw + gap) * (float)i + cw, y + ch),
					cards[i], tt, g_AlphaMul);
			}
			y += ch + 22.f;
		}

		if (settings::home::activity)
		{

			float x = L;
			for (const char* p = "PLAYER ACTIVITY"; *p; ++p)
			{
				char one[2] = { *p, 0 };					dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(x, y),
						ImGui::GetColorU32(g_T.textDim, g_AlphaMul), one);
					x += g_FontSmall->CalcTextSizeA(g_FontSmall->FontSize, FLT_MAX, 0.f, one).x + 1.f;
				}

				const float shown = settings::home::count_up
				? anim::count(anim::id(&m), (float)players, d, (float)m.last_players)
				: (float)players;
			snprintf(sbuf, sizeof sbuf, "%d", (int)(shown + 0.5f));
			ImFont* fb = g_FontValue ? g_FontValue : (g_FontBold ? g_FontBold : g_FontRegular);
			dl->AddText(fb, fb->FontSize, SnapText(ImVec2(R - 4.f - fb->CalcTextSizeA(fb->FontSize, FLT_MAX, 0.f, sbuf).x, y - 5.f)),
				ImGui::GetColorU32(g_T.textPrimary, g_AlphaMul), sbuf);

			y += g_FontSmall->FontSize + 9.f;
			activity_chart(dl, ImVec2(L, y), ImVec2(R, y + 158.f), g_AlphaMul);
			y += 158.f + 22.f;
		}

		if (settings::home::nearby)
		{
			float x = L;
			for (const char* p = "NEARBY"; *p; ++p)
			{
				char one[2] = { *p, 0 };					dl->AddText(g_FontSmall, g_FontSmall->FontSize, ImVec2(x, y),
						ImGui::GetColorU32(g_T.textDim, g_AlphaMul), one);
					x += g_FontSmall->CalcTextSizeA(g_FontSmall->FontSize, FLT_MAX, 0.f, one).x + 1.f;
				}
				y += g_FontSmall->FontSize + 8.f;

			const int rows = ImClamp(settings::home::nearby_rows, 1, 16);
			nearby_plate(dl, ImVec2(L, y), ImVec2(R, y + 27.f * (float)rows), g_AlphaMul);
			y += 27.f * (float)rows + 10.f;
		}

		g_RowY = y;
	}
}
