#define NOMINMAX
#include "Radar.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <settings.h>
#include <cache/cache.h>
#include <game/game.h>
#include <memory/memory.h>
#include <sdk/offsets.h>

namespace radar
{

	struct WallBox
	{
		math::vector3 center{};
		math::vector3 half{};
		math::matrix3 rot{};
	};

	static std::vector<WallBox> s_walls;
	static std::mutex           s_walls_mtx;

	static void collect_parts(rbx::instance_t inst, std::vector<WallBox>& out,
		int depth, int& budget, const math::vector3& player, float range_sq,
		const std::unordered_set<std::uint64_t>& skip_parts)
	{
		if (!inst.address || depth > 6 || budget <= 0)
			return;
		try
		{
			for (auto& child : inst.get_children())
			{
				if (budget <= 0) return;
				--budget;
				if (!child.address) continue;
				if (skip_parts.count(child.address)) continue;

				std::string cls;
				try { cls = child.get_class_name(); } catch (...) { continue; }

				const bool is_part =
					cls.find("Part") != std::string::npos ||
					cls == "Terrain" || cls == "SpawnLocation" || cls == "Seat";

				if (is_part)
				{
					try
					{
						rbx::part_t part(child.address);
						rbx::primitive_t prim = part.get_primitive();
						if (prim.address)
						{
							math::vector3 p = prim.get_position();
							math::vector3 sz = prim.get_size();

							if (sz.y >= 1.f && sz.y <= 400.f &&
								sz.x >= 1.f && sz.x <= 600.f &&
								sz.z >= 1.f && sz.z <= 600.f)
							{
								float dx = p.x - player.x, dz = p.z - player.z;
								if (dx * dx + dz * dz <= range_sq)
								{
									WallBox wb;
									wb.center = p;
									wb.half = sz * 0.5f;
									wb.rot = prim.get_rotation();
									bool finite = true;
									for (float f : wb.rot.m)
										if (!std::isfinite(f)) { finite = false; break; }
									if (finite)
										out.push_back(wb);
								}
							}
						}
					}
					catch (...) {}
				}

				collect_parts(child, out, depth + 1, budget, player, range_sq, skip_parts);
			}
		}
		catch (...) {}
	}

	static void refresh_walls(const math::vector3& player)
	{
		std::vector<WallBox> fresh;
		fresh.reserve(512);

		std::unordered_set<std::uint64_t> skip_parts;
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			for (const auto& e : cache::cached_players)
				for (const auto& [name, part] : e.parts)
					if (part.address) skip_parts.insert(part.address);
		}

		if (game::workspace.address)
		{
			int budget = 2500;
			collect_parts(rbx::instance_t(game::workspace.address), fresh, 0, budget,
				player, 340.f * 340.f, skip_parts);
		}

		std::lock_guard<std::mutex> lk(s_walls_mtx);
		s_walls = std::move(fresh);
	}

	static void xz_basis_from_camera(const math::matrix3& rot,
		math::vector3& out_fwd, math::vector3& out_rgt)
	{
		math::vector3 f = rot.forward();
		f.y = 0.f;
		float fl = std::sqrt(f.x * f.x + f.z * f.z);
		out_fwd = (fl > 1e-4f) ? math::vector3{ f.x / fl, 0.f, f.z / fl }
		                       : math::vector3{ 0.f, 0.f, -1.f };

		math::vector3 r = rot.right();
		r.y = 0.f;
		float rl = std::sqrt(r.x * r.x + r.z * r.z);
		out_rgt = (rl > 1e-4f) ? math::vector3{ r.x / rl, 0.f, r.z / rl }
		                       : math::vector3{ 1.f, 0.f, 0.f };
	}

	static ImVec2 world_to_radar_pt(const ImVec2& center, float radar_half, float range,
		const math::vector3& local, const math::vector3& fwd, const math::vector3& rgt,
		const math::vector3& world)
	{
		math::vector3 d = world - local;
		float side  = d.x * rgt.x + d.z * rgt.z;
		float ahead = d.x * fwd.x + d.z * fwd.z;
		return ImVec2(
			center.x + (side  / range) * radar_half,
			center.y - (ahead / range) * radar_half);
	}

	static bool clip_segment_to_rect(const ImVec2& rmin, const ImVec2& rmax,
		ImVec2 a, ImVec2 b, ImVec2& out_a, ImVec2& out_b)
	{
		float t0 = 0.f, t1 = 1.f;
		const float dx = b.x - a.x, dy = b.y - a.y;

		auto clip_dim = [&](float p, float q) -> bool {
			if (std::fabs(p) < 1e-8f)
				return q >= 0.f;
			const float t = q / p;
			if (p < 0.f)
			{
				if (t > t1) return false;
				if (t > t0) t0 = t;
			}
			else
			{
				if (t < t0) return false;
				if (t < t1) t1 = t;
			}
			return true;
		};

		if (!clip_dim(-dx, a.x - rmin.x)) return false;
		if (!clip_dim( dx, rmax.x - a.x)) return false;
		if (!clip_dim(-dy, a.y - rmin.y)) return false;
		if (!clip_dim( dy, rmax.y - a.y)) return false;

		out_a = ImVec2(a.x + t0 * dx, a.y + t0 * dy);
		out_b = ImVec2(a.x + t1 * dx, a.y + t1 * dy);
		return true;
	}

	static bool point_in_rect(const ImVec2& rmin, const ImVec2& rmax, const ImVec2& p)
	{
		return p.x >= rmin.x && p.x <= rmax.x && p.y >= rmin.y && p.y <= rmax.y;
	}

	static std::vector<ImVec2> clip_polygon_to_rect(const std::vector<ImVec2>& poly,
		const ImVec2& rmin, const ImVec2& rmax)
	{
		if (poly.size() < 3) return {};

		auto clip_edge = [&](const std::vector<ImVec2>& in, ImVec2 a, ImVec2 b) {
			std::vector<ImVec2> out;
			const ImVec2 e{ b.x - a.x, b.y - a.y };
			auto inside = [&](const ImVec2& p) {
				const ImVec2 d{ p.x - a.x, p.y - a.y };
				return e.x * d.y - e.y * d.x >= -1e-3f;
			};
			const size_t n = in.size();
			for (size_t i = 0; i < n; ++i)
			{
				const ImVec2 cur = in[i];
				const ImVec2 prv = in[(i + n - 1) % n];
				const bool inc = inside(cur), inp = inside(prv);
				if (inc != inp)
				{
					const float dx = cur.x - prv.x, dy = cur.y - prv.y;
					const float de = e.x * dy - e.y * dx;
					float t = 0.f;
					if (std::fabs(de) > 1e-8f)
					{
						const ImVec2 d0{ prv.x - a.x, prv.y - a.y };
						t = (e.x * d0.y - e.y * d0.x) / de;
					}
					out.push_back(ImVec2(prv.x + dx * t, prv.y + dy * t));
				}
				if (inc) out.push_back(cur);
			}
			return out;
		};

		std::vector<ImVec2> cur = poly;
		cur = clip_edge(cur, ImVec2(rmin.x, rmax.y), ImVec2(rmin.x, rmin.y));
		if (cur.size() < 3) return {};
		cur = clip_edge(cur, ImVec2(rmax.x, rmin.y), ImVec2(rmax.x, rmax.y));
		if (cur.size() < 3) return {};
		cur = clip_edge(cur, ImVec2(rmin.x, rmin.y), ImVec2(rmax.x, rmin.y));
		if (cur.size() < 3) return {};
		cur = clip_edge(cur, ImVec2(rmax.x, rmax.y), ImVec2(rmin.x, rmax.y));
		return cur;
	}

	static float cross_oab(const ImVec2& o, const ImVec2& a, const ImVec2& b)
	{
		return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
	}

	static std::vector<ImVec2> convex_hull(std::vector<ImVec2> pts)
	{
		std::sort(pts.begin(), pts.end(), [](const ImVec2& a, const ImVec2& b) {
			return a.x < b.x || (a.x == b.x && a.y < b.y);
		});
		pts.erase(std::unique(pts.begin(), pts.end(), [](const ImVec2& a, const ImVec2& b) {
			return a.x == b.x && a.y == b.y;
		}), pts.end());

		if (pts.size() <= 1) return pts;

		std::vector<ImVec2> lower;
		for (const auto& p : pts)
		{
			while (lower.size() >= 2 && cross_oab(lower[lower.size()-2], lower.back(), p) <= 1e-5f)
				lower.pop_back();
			lower.push_back(p);
		}
		std::vector<ImVec2> upper;
		for (int i = (int)pts.size() - 1; i >= 0; --i)
		{
			const ImVec2& p = pts[(size_t)i];
			while (upper.size() >= 2 && cross_oab(upper[upper.size()-2], upper.back(), p) <= 1e-5f)
				upper.pop_back();
			upper.push_back(p);
		}
		lower.pop_back();
		upper.pop_back();
		lower.insert(lower.end(), upper.begin(), upper.end());
		return lower;
	}

	static ImU32 col_of(const float c[4], float alpha = 1.f)
	{
		auto ch = [](float v) {
			v = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
			return (int)(v * 255.f + 0.5f);
		};
		return IM_COL32(ch(c[0]), ch(c[1]), ch(c[2]), ch(c[3] * alpha));
	}

	static ImU32 fade(ImU32 c, float f)
	{
		const float k = f < 0.f ? 0.f : (f > 1.f ? 1.f : f);
		const int a = (int)(((c >> IM_COL32_A_SHIFT) & 0xFF) * k);
		return (c & ~IM_COL32_A_MASK) | ((ImU32)a << IM_COL32_A_SHIFT);
	}

	void draw()
	{
		if (!settings::visuals::radar_enabled)
			return;

		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		if (!draw)
			return;

		const float half  = std::clamp(settings::visuals::radar_size, 40.f, 300.f);
		const float range = (float)std::clamp((int)settings::visuals::radar_range, 20, 800);
		const bool  round = settings::visuals::radar_shape == 1;
		const bool  spin  = settings::visuals::radar_rotate;
		const float plate = std::clamp(settings::visuals::radar_opacity, 0.15f, 1.f);

		const ImVec2 display = ImGui::GetIO().DisplaySize;
		const float  margin  = 24.f;
		const int    corner  = std::clamp(settings::visuals::radar_position, 0, 3);

		ImVec2 center;
		switch (corner)
		{
		case 1:  center = ImVec2(half + margin, half + margin); break;
		case 2:  center = ImVec2(display.x - half - margin, display.y - half - margin); break;
		case 3:  center = ImVec2(half + margin, display.y - half - margin); break;
		default: center = ImVec2(display.x - half - margin, half + margin); break;
		}

		const ImVec2 rmin(center.x - half, center.y - half);
		const ImVec2 rmax(center.x + half, center.y + half);

		const float clip_half = round ? half * 0.72f : half;
		const ImVec2 cmin(center.x - clip_half, center.y - clip_half);
		const ImVec2 cmax(center.x + clip_half, center.y + clip_half);

		math::vector3 local_pos{};
		bool has_lp = false;
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			if (cache::cached_local_player.instance.address)
			{
				auto it = cache::cached_local_player.parts.find("HumanoidRootPart");
				if (it != cache::cached_local_player.parts.end() && it->second.address)
				{
					try
					{
						local_pos = it->second.get_primitive().get_position();
						has_lp = std::isfinite(local_pos.x) && std::isfinite(local_pos.y) && std::isfinite(local_pos.z);
					}
					catch (...) {}
				}
			}
		}

		math::matrix3 cam_rot = math::matrix3::identity();
		bool has_cam = false;
		if (game::workspace.address)
		{
			try
			{
				const std::uint64_t cam_addr = memory->read<std::uint64_t>(
					game::workspace.address + Offsets::Workspace::CurrentCamera);
				if (cam_addr)
				{
					rbx::camera_t cam{ cam_addr };
					if (!has_lp)
					{
						const math::vector3 p = cam.get_position();
						if (std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z))
						{
							local_pos = p;
							has_lp = true;
						}
					}
					cam_rot = cam.get_rotation();
					has_cam = true;
				}
			}
			catch (...) {}
		}

		if (!has_lp || !has_cam)
			return;

		math::vector3 fwd{}, rgt{};
		if (spin)
			xz_basis_from_camera(cam_rot, fwd, rgt);
		else
		{
			fwd = math::vector3{ 0.f, 0.f, -1.f };
			rgt = math::vector3{ 1.f, 0.f, 0.f };
		}

		const ImU32 accent = col_of(settings::visuals::radar_local_color);

		if (round)
		{
			draw->AddCircleFilled(center, half, IM_COL32(10, 12, 16, (int)(210.f * plate)), 64);
			draw->AddCircle(center, half, IM_COL32(0, 0, 0, 200), 64, 2.f);
			draw->AddCircle(center, half - 1.f, IM_COL32(255, 255, 255, 34), 64, 1.f);
		}
		else
		{
			draw->AddRectFilled(rmin, rmax, IM_COL32(10, 12, 16, (int)(210.f * plate)), 10.f);
			draw->AddRect(rmin, rmax, IM_COL32(0, 0, 0, 200), 10.f, 0, 2.f);
			draw->AddRect(ImVec2(rmin.x + 1.f, rmin.y + 1.f), ImVec2(rmax.x - 1.f, rmax.y - 1.f),
				IM_COL32(255, 255, 255, 30), 9.f, 0, 1.f);
			draw->AddRectFilled(rmin, ImVec2(rmax.x, rmin.y + 2.f), fade(accent, 0.45f * plate), 10.f);
		}

		const float ring_frac[3] = { 1.f / 3.f, 2.f / 3.f, 1.f };
		if (settings::visuals::radar_grid)
		{
			for (int i = 0; i < 3; ++i)
			{
				const float r = half * ring_frac[i];
				if (round)
					draw->AddCircle(center, r, IM_COL32(255, 255, 255, 16), 64, 1.f);
				else
				{
					const ImVec2 g0(center.x - r, center.y - r), g1(center.x + r, center.y + r);
					draw->AddRect(g0, g1, IM_COL32(255, 255, 255, 16), 4.f, 0, 1.f);
				}
			}
		}
		draw->AddLine(ImVec2(center.x, rmin.y + 2.f), ImVec2(center.x, rmax.y - 2.f), IM_COL32(255, 255, 255, 22), 1.f);
		draw->AddLine(ImVec2(rmin.x + 2.f, center.y), ImVec2(rmax.x - 2.f, center.y), IM_COL32(255, 255, 255, 22), 1.f);

		if (ImFont* rf = ImGui::GetFont())
		{
			const float fh = ImGui::GetFontSize() * 0.82f;
			if (settings::visuals::radar_grid && settings::visuals::radar_labels)
			{
				for (int i = 0; i < 3; ++i)
				{
					char lb[16];
					snprintf(lb, sizeof lb, "%d", (int)(range * ring_frac[i] + 0.5f));
					draw->AddText(rf, fh, ImVec2(center.x + 4.f, center.y - half * ring_frac[i] + 1.f),
						IM_COL32(150, 150, 158, 160), lb);
				}
			}

			if (spin)
				draw->AddText(rf, fh, ImVec2(center.x - 3.f, center.y - half + 5.f),
					IM_COL32(150, 150, 158, 170), "N");
		}

		if (settings::visuals::radar_sweep)
		{
			static float s_angle = 0.f;
			s_angle += ImGui::GetIO().DeltaTime * 1.9f;
			if (s_angle > 6.2831853f) s_angle -= 6.2831853f;

			for (int i = 0; i < 8; ++i)
			{
				const float a  = s_angle - (float)i * 0.035f;
				const float x1 = center.x + std::cos(a) * half;
				const float y1 = center.y + std::sin(a) * half;
				draw->AddLine(center, ImVec2(x1, y1),
					fade(accent, (1.f - (float)i / 8.f) * 0.30f * plate), 1.f);
			}
		}

		static auto s_last_wall_refresh = std::chrono::steady_clock::now() - std::chrono::seconds(2);
		static math::vector3 s_last_wall_pos{};
		const auto now = std::chrono::steady_clock::now();
		const bool moved = (local_pos - s_last_wall_pos).length() > 12.f;
		if (moved && std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_wall_refresh).count() > 1000)
		{
			s_last_wall_refresh = now;
			s_last_wall_pos = local_pos;
			refresh_walls(local_pos);
		}

		if (settings::visuals::radar_show_walls)
		{
			std::vector<WallBox> walls;
			{
				std::lock_guard<std::mutex> lk(s_walls_mtx);
				walls = s_walls;
			}

			const ImU32 wall_fill   = col_of(settings::visuals::radar_wall_color, 0.55f * plate);
			const ImU32 wall_stroke = col_of(settings::visuals::radar_wall_color, 0.85f * plate);

			for (const auto& wb : walls)
			{
				const int idx[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };
				math::vector3 c[4];
				for (int i = 0; i < 4; ++i)
				{
					const float sx = idx[i][0] ? 1.f : -1.f;
					const float sz = idx[i][1] ? 1.f : -1.f;
					c[i] = wb.center + wb.rot * math::vector3{ sx * wb.half.x, -wb.half.y, sz * wb.half.z };
				}

				std::vector<ImVec2> pts;
				pts.reserve(4);
				for (int i = 0; i < 4; ++i)
					pts.push_back(world_to_radar_pt(center, half, range, local_pos, fwd, rgt, c[i]));

				auto hull = convex_hull(pts);
				if (hull.size() < 3) continue;
				auto clipped = clip_polygon_to_rect(hull, cmin, cmax);
				if (clipped.size() < 3) continue;

				bool sane = true;
				for (const auto& p : clipped)
					if (!std::isfinite(p.x) || !std::isfinite(p.y) ||
						p.x < -10000.f || p.x > 10000.f || p.y < -10000.f || p.y > 10000.f)
					{ sane = false; break; }
				if (!sane) continue;

				draw->AddConvexPolyFilled(clipped.data(), (int)clipped.size(), wall_fill);
				draw->AddPolyline(clipped.data(), (int)clipped.size(), wall_stroke, ImDrawFlags_Closed, 1.f);
			}
		}

		std::vector<cache::entity_t> snapshot;
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			snapshot = cache::cached_players;
		}

		std::uint64_t local_addr = 0;
		if (game::local_player.address)
			local_addr = game::local_player.address;
		else if (game::players.address)
			local_addr = memory->read<std::uint64_t>(game::players.address + Offsets::Player::LocalPlayer);

		const ImU32 ally_col  = col_of(settings::visuals::radar_team_color);
		const ImU32 enemy_col = col_of(settings::visuals::radar_enemy_color);
		ImFont* font = ImGui::GetFont();
		const float font_h = ImGui::GetFontSize() * 0.82f;

		for (const auto& entity : snapshot)
		{
			if (!entity.instance.address) continue;

			auto pit = entity.parts.find("HumanoidRootPart");
			if (pit == entity.parts.end()) pit = entity.parts.find("Head");
			if (pit == entity.parts.end() || !pit->second.address) continue;

			math::vector3 pos;
			try { pos = pit->second.get_primitive().get_position(); }
			catch (...) { continue; }
			if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) continue;

			const ImVec2 p = world_to_radar_pt(center, half, range, local_pos, fwd, rgt, pos);
			if (!point_in_rect(cmin, cmax, p)) continue;

			const bool is_local = (entity.instance.address == local_addr);
			const bool dead     = cache::is_dead(entity);
			if (dead && !settings::visuals::radar_dead && !is_local) continue;

			if (is_local)
			{
				const ImVec2 u(fwd.x, fwd.z);
				const float  s = 6.f;
				ImVec2 tri[3] = {
					ImVec2(p.x + u.x * s,         p.y + u.y * s),
					ImVec2(p.x - u.y * s * 0.72f, p.y + u.x * s * 0.72f),
					ImVec2(p.x + u.y * s * 0.72f, p.y - u.x * s * 0.72f),
				};
				draw->AddConvexPolyFilled(tri, 3, accent);
				draw->AddPolyline(tri, 3, IM_COL32(255, 255, 255, 190), ImDrawFlags_Closed, 1.f);
				continue;
			}

			const ImU32 col = cache::is_teammate(entity) ? ally_col : enemy_col;
			const ImU32 dot = dead ? fade(col, 0.45f) : col;

			if (dead)
			{
				draw->AddCircle(p, 3.5f, fade(dot, 0.75f), 12, 1.4f);
				const float k = 2.4f;
				draw->AddLine(ImVec2(p.x - k, p.y - k), ImVec2(p.x + k, p.y + k), dot, 1.2f);
				draw->AddLine(ImVec2(p.x - k, p.y + k), ImVec2(p.x + k, p.y - k), dot, 1.2f);
			}
			else
			{
				draw->AddCircleFilled(p, 6.f, fade(dot, 0.28f), 14);
				draw->AddCircleFilled(p, 3.f, dot, 14);
				draw->AddCircle(p, 3.f, IM_COL32(0, 0, 0, 170), 14, 1.f);

				if (settings::visuals::radar_health && entity.health > 0.f)
				{
					const float max_hp = entity.max_health > 0.f ? entity.max_health : 100.f;
					const float frac = std::clamp(entity.health / max_hp, 0.f, 1.f);
					const float a0 = -1.5707963f, a1 = a0 + 6.2831853f * frac;
					draw->PathArcTo(p, 6.4f, a0, a1, 24);
					draw->PathStroke(frac > 0.55f ? IM_COL32(126, 214, 132, 220)
						: (frac > 0.25f ? IM_COL32(226, 196, 106, 220) : IM_COL32(226, 108, 108, 220)),
						0, 1.6f);
				}
			}

			if (settings::visuals::radar_names && font)
			{
				const std::string& shown = entity.display_name.empty() ? entity.name : entity.display_name;
				if (!shown.empty())
				{
					const ImVec2 sz = font->CalcTextSizeA(font_h, FLT_MAX, 0.f, shown.c_str());
					draw->AddText(font, font_h, ImVec2(p.x - sz.x * 0.5f - 1.f, p.y - 16.f),
						IM_COL32(0, 0, 0, 190), shown.c_str());
					draw->AddText(font, font_h, ImVec2(p.x - sz.x * 0.5f, p.y - 15.f),
						fade(dot, 0.94f), shown.c_str());
				}
			}
		}
	}
}
