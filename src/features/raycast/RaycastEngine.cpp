
#include "RaycastEngine.h"

#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>
#include <game/game_context.h>
#include <cache/cache.h>
#include <debug/DebugConsole.h>
#include <settings.h>

#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace aether_raycast
{

	struct Occluder
	{
		std::uint64_t primitive = 0;
		math::vector3 pos{};
		math::vector3 half{};
		math::matrix3 rot{};
	};

	static std::mutex                 s_occ_mtx;
	static std::vector<Occluder>      s_occluders;
	static std::atomic<bool>          s_running{ false };
	static std::atomic<std::uint64_t> s_teleport_gen{ 0 };

	struct Verdict { bool visible; float coverage; std::uint64_t gen; };
	static std::mutex        s_verdict_mtx;
	static std::unordered_map<std::uintptr_t, Verdict> s_verdicts;

	static bool read_camera_pos(math::vector3& out)
	{
		try
		{
			if (!game::workspace.address) return false;
			std::uint64_t cam_addr = memory->read<std::uint64_t>(
				game::workspace.address + Offsets::Workspace::CurrentCamera);
			if (!cam_addr) return false;
			rbx::camera_t cam{ cam_addr };
			math::vector3 p = cam.get_position();
			if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
				return false;
			out = p;
			return true;
		}
		catch (...) { return false; }
	}

	static math::vector3 rot_transpose_mul(const math::matrix3& m, const math::vector3& v)
	{

		return {
			m.m[0] * v.x + m.m[3] * v.y + m.m[6] * v.z,
			m.m[1] * v.x + m.m[4] * v.y + m.m[7] * v.z,
			m.m[2] * v.x + m.m[5] * v.y + m.m[8] * v.z,
		};
	}

	static bool matrix_valid(const math::matrix3& m)
	{
		for (float f : m.m)
			if (!std::isfinite(f)) return false;
		return true;
	}

	static bool ray_hits_box(const math::vector3& ro, const math::vector3& rd,
		const Occluder& b, float max_dist)
	{
		math::vector3 o = rot_transpose_mul(b.rot, ro - b.pos);
		math::vector3 d = rot_transpose_mul(b.rot, rd);

		float tmin = 0.f, tmax = max_dist;
		const float* oo = &o.x;
		const float* dd = &d.x;
		const float* hh = &b.half.x;

		for (int i = 0; i < 3; ++i)
		{
			if (std::fabs(dd[i]) < 1e-6f)
			{
				if (std::fabs(oo[i]) > hh[i])
					return false;
			}
			else
			{
				float inv = 1.f / dd[i];
				float t1 = (-hh[i] - oo[i]) * inv;
				float t2 = ( hh[i] - oo[i]) * inv;
				if (t1 > t2) std::swap(t1, t2);
				tmin = (std::max)(tmin, t1);
				tmax = (std::min)(tmax, t2);
				if (tmin > tmax)
					return false;
			}
		}
		return true;
	}

	static std::unordered_set<std::uint64_t> collect_player_part_addresses()
	{
		std::unordered_set<std::uint64_t> out;
		std::lock_guard<std::mutex> lk(cache::mtx);
		for (const auto& e : cache::cached_players)
			for (const auto& [name, part] : e.parts)
				if (part.address) out.insert(part.address);
		return out;
	}

	static void collect_parts_from_instance(rbx::instance_t inst,
		std::vector<Occluder>& out, int depth, int& budget,
		const math::vector3& cam, float max_dist_sq,
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

				bool is_part =
					cls.find("Part") != std::string::npos ||
					cls == "Terrain" || cls == "SpawnLocation";

				if (is_part)
				{
					rbx::part_t part(child.address);
					rbx::primitive_t prim = part.get_primitive();
					if (prim.address)
					{
						math::vector3 p = prim.get_position();
						float dx = p.x - cam.x, dy = p.y - cam.y, dz = p.z - cam.z;
						if (dx * dx + dy * dy + dz * dz <= max_dist_sq)
						{
							Occluder occ;
							occ.primitive = prim.address;
							occ.pos = p;
							occ.half = prim.get_size() * 0.5f;
							occ.rot = prim.get_rotation();
							if (matrix_valid(occ.rot) &&
								std::isfinite(occ.pos.x) && occ.half.x > 0.05f &&
								occ.half.x < 512.f && occ.half.y < 512.f && occ.half.z < 512.f)
							{
								out.push_back(occ);
							}
						}
					}
				}

				collect_parts_from_instance(child, out, depth + 1, budget, cam, max_dist_sq, skip_parts);
			}
		}
		catch (...) {}
	}

	static void rebuild_occluders()
	{
		std::vector<Occluder> fresh;

		try
		{
			math::vector3 cam{};
			if (!read_camera_pos(cam)) return;
			float maxd = cfg::max_distance;
			float maxd_sq = maxd * maxd;

			int budget = 4000;

			const auto skip_parts = collect_player_part_addresses();

			rbx::instance_t ws(game::workspace.address);
			if (ws.address)
				collect_parts_from_instance(ws, fresh, 0, budget, cam, maxd_sq, skip_parts);
		}
		catch (...) {}

		{
			std::lock_guard<std::mutex> lk(s_occ_mtx);
			s_occluders = std::move(fresh);
		}
	}

	static void evaluate_targets()
	{
		if (!cfg::enabled) return;
		if (!game::visengine.address || !game::workspace.address) return;

		try
		{
			math::vector3 cam{};
			if (!read_camera_pos(cam)) return;

			std::vector<std::pair<std::uintptr_t, rbx::model_instance_t>> targets;
			{
				std::lock_guard<std::mutex> lk(cache::mtx);
				const std::uint64_t local_skip = cache::cached_local_player.instance.address
					? cache::cached_local_player.instance.address
					: game::local_player.address;
				for (auto& e : cache::cached_players)
				{
					if (e.instance.address == local_skip) continue;

					bool is_player = true;
					try { is_player = (rbx::instance_t(e.instance.address).get_class_name() == "Player"); }
					catch (...)
					{
						continue;
					}
					rbx::model_instance_t mdl = is_player
						? rbx::player_t(e.instance.address).get_model_instance()
						: rbx::model_instance_t(e.instance.address);
					if (mdl.address)
						targets.push_back({ e.instance.address, mdl });
				}
			}

			std::vector<Occluder> occ;
			{
				std::lock_guard<std::mutex> lk(s_occ_mtx);
				occ = s_occluders;
			}

			std::uint64_t gen = s_teleport_gen.load();

			for (auto& [addr, mdl] : targets)
			{

				rbx::instance_t hrp_i = mdl.find_first_child("HumanoidRootPart");
				rbx::instance_t head_i = mdl.find_first_child("Head");
				if (!hrp_i.address && !head_i.address) continue;

				math::vector3 base{};
				if (head_i.address)
				{
					rbx::part_t h(head_i.address);
					base = h.get_primitive().get_position();
				}
				else
				{
					rbx::part_t h(hrp_i.address);
					base = h.get_primitive().get_position();
				}

				const math::vector3 offsets[] = {
					{0.f, 1.0f, 0.f}, {0.f, 0.5f, 0.f},
					{0.9f, 0.f, 0.f}, {-0.9f, 0.f, 0.f}, {0.f, 0.f, 0.9f},
				};
				int n_rays = std::clamp(cfg::rays_per_target, 1, 5);

				int hits = 0;
				for (int i = 0; i < n_rays; ++i)
				{
					math::vector3 tp{ base.x + offsets[i].x, base.y + offsets[i].y, base.z + offsets[i].z };

					math::vector3 dir = tp - cam;
					float dist = dir.length();
					if (dist < 4.f) continue;
					dir = dir * (1.f / dist);

					bool blocked = false;
					for (const auto& b : occ)
					{
						if (ray_hits_box(cam, dir, b, dist - 1.5f))
						{
							blocked = true;
							break;
						}
					}
					if (blocked) ++hits;
				}

				float coverage = 1.f - (float)hits / (float)n_rays;
				Verdict v{ coverage > 0.34f, coverage, gen };

				std::lock_guard<std::mutex> lk(s_verdict_mtx);
				s_verdicts[addr] = v;
			}

			{
				std::lock_guard<std::mutex> lk(s_verdict_mtx);
				if (s_verdicts.size() > 128)
					s_verdicts.clear();
			}
		}
		catch (...) {}
	}

	static void worker()
	{
		for (;;)
		{
			if (!s_running.load())
				break;

			bool want = cfg::enabled && game::workspace.address != 0;
			if (want)
			{
				rebuild_occluders();
				evaluate_targets();
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
			else
			{

				std::lock_guard<std::mutex> lk(s_verdict_mtx);					s_verdicts.clear();
					std::this_thread::sleep_for(std::chrono::milliseconds(250));
				}
			}
		}

	static std::thread s_thread;

	void start()
	{
		if (s_running.exchange(true))
			return;
		try { s_thread = std::thread(worker); }
		catch (...) { s_running = false; }
	}

	void stop()
	{
		if (!s_running.exchange(false))
			return;
		if (s_thread.joinable())
			s_thread.join();
	}

	void notify_teleport()
	{
		++s_teleport_gen;
	}

	Result is_player_visible(std::uintptr_t character_address,
		const math::vector3& camera_pos)
	{
		(void)camera_pos;
		Result r{};
		if (!cfg::enabled || !character_address)
			return r;

		std::lock_guard<std::mutex> lk(s_verdict_mtx);
		auto it = s_verdicts.find(character_address);
		if (it == s_verdicts.end())
			return r;

		r.visible = it->second.visible;
		r.coverage = it->second.coverage;
		return r;
	}
}
