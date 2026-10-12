#define NOMINMAX
#include "triggerbot.h"

#include <game/game.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <sdk/math/math.h>
#include <memory/memory.h>
#include <game/game_context.h>
#include <sdk/offsets.h>
#include <settings.h>
#include <keybind/keybind.h>
#include <keybind/live_state.h>
#include <features/worldgeo/WorldGeo.h>
#include <features/opone/OperationOne.h>
#include <features/aimbot/aimbot.h>
#include <features/players/friends.h>
#include <features/players/targets.h>
#include <imgui/imgui.h>

#include <windows.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
	using tb_clock = std::chrono::steady_clock;

	bool tb_is_knocked(const cache::entity_t& player)
	{

		return game_ctx::get_knock(player.instance.address) == game_ctx::KnockState::Yes;
	}

	std::uint64_t tb_camera()
	{
		if (!game::datamodel.address) return 0;
		std::uint64_t ws = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
		if (!ws) return 0;
		return memory->read<std::uint64_t>(ws + Offsets::Workspace::CurrentCamera);
	}

	void tb_part_names(int mode, std::vector<const char*>& out)
	{
		static const char* const k_head[]  = { "Head", "head" };
		static const char* const k_torso[] = { "UpperTorso", "LowerTorso", "Torso", "torso", "HumanoidRootPart" };
		static const char* const k_any[] = {
			"Head", "head",
			"UpperTorso", "LowerTorso", "Torso", "torso", "HumanoidRootPart",
			"LeftUpperArm", "LeftLowerArm", "Left Arm", "arm1",
			"RightUpperArm", "RightLowerArm", "Right Arm", "arm2",
			"LeftUpperLeg", "LeftLowerLeg", "Left Leg", "leg1",
			"RightUpperLeg", "RightLowerLeg", "Right Leg", "leg2",
		};

		static const char* const k_op1[] = {
			"collision", "collision2", "collision3", "legs"
		};

		switch (mode)
		{
		case 1:  out.assign(k_head,  k_head  + sizeof(k_head)  / sizeof(k_head[0]));  break;
		case 2:  out.assign(k_torso, k_torso + sizeof(k_torso) / sizeof(k_torso[0])); break;
		default: out.assign(k_any,   k_any   + sizeof(k_any)   / sizeof(k_any[0]));   break;
		}

		if (op1::active())
			out.insert(out.end(), k_op1, k_op1 + sizeof(k_op1) / sizeof(k_op1[0]));
	}

	void tb_click()
	{
		INPUT inp[2]{};
		inp[0].type       = INPUT_MOUSE;
		inp[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		inp[1].type       = INPUT_MOUSE;
		inp[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(2, inp, sizeof(INPUT));
	}

	const math::vector3 k_corners[8] = {
		{-1,-1,-1},{1,-1,-1},{-1,1,-1},{1,1,-1},
		{-1,-1, 1},{1,-1, 1},{-1,1, 1},{1,1, 1}
	};

	struct screen_box { float left{}, top{}, right{}, bottom{}; };

	bool project_part_box(const cache::entity_t& entity, const char* name,
		const math::matrix4& view, const math::vector2& dims, screen_box& out)
	{
		auto it = entity.parts.find(name);
		if (it == entity.parts.end() || !it->second.address) return false;

		try
		{
			rbx::primitive_t prim = it->second.get_primitive();
			if (!prim.address) return false;

			math::vector3 sz = prim.get_size();
			if (!(std::isfinite(sz.x) && std::isfinite(sz.y) && std::isfinite(sz.z))) return false;
			if (std::fabs(sz.x) > 512.f || std::fabs(sz.y) > 512.f || std::fabs(sz.z) > 512.f) return false;
			if (sz.x == 0.f && sz.y == 0.f && sz.z == 0.f) return false;

			math::vector3 pos{};
			if (op1::active())
			{
				if (!op1::live_position(it->second, pos)) return false;
			}
			else
			{
				pos = prim.get_position();
			}
			if (!(std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z))) return false;

			math::matrix3 rot = prim.get_rotation();

			float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = -FLT_MAX;
			bool  any = false;

			for (const auto& c : k_corners)
			{
				math::vector3 w = pos + rot * math::vector3{ c.x * sz.x * 0.5f, c.y * sz.y * 0.5f, c.z * sz.z * 0.5f };
				math::vector2 s{};
				if (!game::visengine.world_to_screen(w, s, dims, view)) continue;
				if (!(std::isfinite(s.x) && std::isfinite(s.y))) continue;

				if (s.x < -dims.x || s.x > dims.x * 2.f || s.y < -dims.y || s.y > dims.y * 2.f) continue;

				any    = true;
				left   = (std::min)(left, s.x);
				top    = (std::min)(top, s.y);
				right  = (std::max)(right, s.x);
				bottom = (std::max)(bottom, s.y);
			}

			if (!any) return false;
			out = { left, top, right, bottom };
			return true;
		}
		catch (...) { return false; }
	}

	float dist2_to_box(const math::vector2& p, const screen_box& b)
	{
		const float dx = (std::max)((std::max)(b.left - p.x, 0.f), p.x - b.right);
		const float dy = (std::max)((std::max)(b.top - p.y, 0.f), p.y - b.bottom);
		return dx * dx + dy * dy;
	}

	bool part_centre(const cache::entity_t& entity, const char* name, math::vector3& out)
	{
		auto it = entity.parts.find(name);
		if (it == entity.parts.end() || !it->second.address) return false;
		try
		{
			rbx::primitive_t prim = it->second.get_primitive();
			if (!prim.address) return false;
			const math::vector3 pos = prim.get_position();
			if (!(std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z))) return false;
			out = pos;
			return true;
		}
		catch (...) { return false; }
	}
}

void rage::triggerbot::run()
{
	keybind kb("triggerbot");

	using namespace std::chrono_literals;

	std::uint64_t locked_target = 0;
	tb_clock::time_point enter_time{};
	tb_clock::time_point last_shot{};
	bool shot_armed = false;

	for (;;)
	{
		std::this_thread::sleep_for(1ms);

		if (!settings::triggerbot::enabled) { locked_target = 0; shot_armed = false; live_state::set(live_state::kTriggerbot, false); continue; }

		kb.key  = settings::triggerbot::keybind;
		kb.type = static_cast<keybind::c_keybind_type>(settings::triggerbot::activation_mode);
		kb.update();

		live_state::set(live_state::kTriggerbot, kb.enabled);

		if (!kb.enabled) { locked_target = 0; shot_armed = false; continue; }

		if (!game::datamodel.address || !game::visengine.address) continue;

		std::uint64_t camera = tb_camera();
		if (!camera) continue;

		math::vector2 dims = game::visengine.get_dimensions();
		math::matrix4 view = game::visengine.get_viewmatrix();
		if (dims.x <= 0.f || dims.y <= 0.f) continue;

		POINT cursor{};
		if (!GetCursorPos(&cursor)) continue;
		const math::vector2 crosshair{ (float)cursor.x, (float)cursor.y };

		std::vector<cache::entity_t> snap;
		cache::entity_t local;
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			snap  = cache::cached_players;
			local = cache::cached_local_player;
		}

		if (settings::triggerbot::gun_check && local.instance.address &&
			!local.parts.empty() && local.tool_name.empty())
		{
			locked_target = 0; shot_armed = false; continue;
		}

		std::vector<const char*> parts;
		tb_part_names(settings::triggerbot::target_part, parts);

		const bool  part_locked = (settings::triggerbot::target_part != 0);
		const float fov2        = settings::triggerbot::fov * settings::triggerbot::fov;
		const float tol2        = part_locked ? 0.f : fov2;
		std::uint64_t best_addr   = 0;
		float         best_d2     = tol2;
		float         locked_d2   = -1.f;

		const std::uint64_t pinned = targets::get();

		for (const cache::entity_t& entity : snap)
		{
			if (!entity.instance.address) continue;
			if (entity.instance.address == local.instance.address) continue;

			if (pinned != 0 && entity.instance.address != pinned) continue;

			if (settings::triggerbot::team_check && cache::is_teammate(entity)) continue;
			if (friends::is_skipped(entity.instance.address, pinned)) continue;
			if (settings::triggerbot::knock_check && tb_is_knocked(entity)) continue;

			if ((settings::triggerbot::dead_check && cache::is_dead(entity)) || op1::corpse(entity)) continue;

			const bool body_only = op1::has_animated_body(entity);

			for (const char* pname : parts)
			{
				if (body_only && op1::is_collider_body(pname)) continue;

				screen_box box{};
				if (!project_part_box(entity, pname, view, dims, box)) continue;

				const float d2 = dist2_to_box(crosshair, box);
				if (d2 > tol2) continue;

				if (settings::triggerbot::visibility_check)
				{
					math::vector3 centre{};
					if (!part_centre(entity, pname, centre)) continue;

					const bool visible = op1::multipoint_los_enabled()
						? op1::visible_any(entity, centre, centre)
						: worldgeo::is_visible(worldgeo::eye_position(centre), centre);

					if (!visible) continue;
				}

				if (entity.instance.address == locked_target)
					locked_d2 = (locked_d2 < 0.f) ? d2 : (std::min)(locked_d2, d2);

				if (d2 <= best_d2)
				{
					best_d2   = d2;
					best_addr = entity.instance.address;
				}
			}
		}

		if (locked_target != 0 && locked_d2 >= 0.f && locked_d2 <= tol2)
		{
			best_addr = locked_target;
			best_d2   = locked_d2;
		}

		const tb_clock::time_point now = tb_clock::now();

		if (!best_addr)
		{
			locked_target = 0;
			shot_armed    = false;
			continue;
		}

		if (best_addr != locked_target)
		{
			locked_target = best_addr;
			enter_time    = now;
			shot_armed    = false;
		}

		const float delay_ms = (std::max)(0.f, settings::triggerbot::delay_ms);

		const float on_target_ms =
			std::chrono::duration<float, std::milli>(now - enter_time).count();
		if (on_target_ms < delay_ms) continue;

		bool fire = false;
		if (settings::triggerbot::continuous)
		{
			const float since_shot =
				std::chrono::duration<float, std::milli>(now - last_shot).count();
			fire = since_shot >= (std::max)(delay_ms, 15.f);
		}
		else
		{

			fire = !shot_armed;
		}

		if (fire)
		{
			tb_click();
			last_shot  = now;
			shot_armed = true;
		}
	}
}
