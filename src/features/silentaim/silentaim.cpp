#define NOMINMAX
#include "silentaim.h"

#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <windows.h>
#include <game/game.h>
#include <memory/memory.h>
#include <game/game_context.h>
#include <sdk/offsets.h>
#include <settings.h>
#include <keybind/keybind.h>
#include <keybind/live_state.h>
#include <cache/cache.h>
#include <features/aimbot/aimbot.h>
#include <features/raycast/RaycastSilent.h>
#include <features/raycast/RaycastEngine.h>
#include <features/silentaim/SilentMethods.h>
#include <features/worldgeo/WorldGeo.h>
#include <features/opone/OperationOne.h>
#include <features/players/friends.h>
#include <features/players/targets.h>
#include <debug/DebugConsole.h>
#include <mutex>

struct limb_t { const char* names[5]; int count; };

static const limb_t k_limbs[6] = {
	{ { "Head", "head" }, 2 },
	{ { "HumanoidRootPart", "UpperTorso", "LowerTorso", "Torso", "torso" }, 5 },
	{ { "Left Arm",  "LeftUpperArm",  "LeftLowerArm",  "LeftHand",  "arm1" }, 5 },
	{ { "Right Arm", "RightUpperArm", "RightLowerArm", "RightHand", "arm2" }, 5 },
	{ { "Left Leg",  "LeftUpperLeg",  "LeftLowerLeg",  "LeftFoot",  "leg1" }, 5 },
	{ { "Right Leg", "RightUpperLeg", "RightLowerLeg", "RightFoot", "leg2" }, 5 },
};

static int part_to_group(int part)
{
	switch (part)
	{
	case 2: return 0;
	case 3: return 1;
	case 4: return 2;
	case 5: return 3;
	case 6: return 4;
	case 7: return 5;
	}
	return -1;
}

static const char* limb_part_name(const cache::entity_t& entity, int group)
{
	if (group < 0 || group >= 6) return nullptr;
	for (int i = 0; i < k_limbs[group].count; ++i)
	{
		auto it = entity.parts.find(k_limbs[group].names[i]);
		if (it != entity.parts.end() && it->second.address)
			return k_limbs[group].names[i];
	}
	return nullptr;
}

static float get_distance_from_center(const math::vector2& point, const math::vector2& position)
{
	float dx = position.x - point.x;
	float dy = position.y - point.y;
	return std::sqrt(dx*dx + dy*dy);
}

static math::vector3 get_velocity(cache::entity_t& entity)
{
	auto root_it = entity.parts.find("HumanoidRootPart");
	if (root_it == entity.parts.end() || !root_it->second.address)
		return math::vector3{};

	rbx::primitive_t prim = root_it->second.get_primitive();
	if (!prim.address) return math::vector3{};

	return memory->read<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity);
}

static math::vector3 apply_prediction(cache::entity_t& entity, const math::vector3& position)
{
	math::vector3 velocity = get_velocity(entity);
	float fx, fy;

	if (settings::silentaim_autopred::enabled)
	{
		float base = settings::silentaim_autopred::strength * 0.016f;
		fx = base; fy = base;
	}
	else
	{
		fx = (20.f - settings::silentaim::prediction_x) * 0.02f;
		fy = (20.f - settings::silentaim::prediction_y) * 0.02f;
	}

	return { position.x + velocity.x * fx, position.y + velocity.y * fy, position.z };
}

static bool is_silent_aim_active()
{

	live_state::set(live_state::kSilentAim, false);

	if (!game::datamodel.address || !game::visengine.address || !settings::silentaim::enabled)
		return false;

	if (settings::silentaim::guncheck && game_ctx::game_logic_allowed())
	{
		if (!silentaim::state.aim_indicator.address) return false;
		bool is_visible = memory->read<bool>(silentaim::state.aim_indicator.address + Offsets::GuiObject::Visible);
		if (!is_visible) return false;
	}

	if (settings::silentaim::use_aimbot_target)
	{
		if (aimbot::sticky_target.instance.address != 0 || aimbot::player.instance.address != 0 ||
			targets::get() != 0)
		{
			live_state::set(live_state::kSilentAim, true);
			return true;
		}
	}

	static keybind silentaim_kb("silentaim");
	silentaim_kb.key  = settings::silentaim::keybind;
	silentaim_kb.type = static_cast<keybind::c_keybind_type>(settings::silentaim::activation_mode);
	silentaim_kb.update();
	live_state::set(live_state::kSilentAim, silentaim_kb.enabled);
	return silentaim_kb.enabled;
}

static bool is_player_knocked(const cache::entity_t& player)
{

	return game_ctx::get_knock(player.instance.address) == game_ctx::KnockState::Yes;
}

static bool aim_point_visible(const math::vector3& point)
{
	if (!math::finite(point))
		return true;
	return worldgeo::is_visible(worldgeo::eye_position(point), point);
}

static bool is_valid_target(const cache::entity_t& entity, const cache::entity_t& local_player, bool ignore_friendly)
{
	if (entity.instance.address == local_player.instance.address) return false;

	if (settings::silentaim::teamcheck && cache::is_teammate(entity)) return false;

	if (friends::is_skipped(entity.instance.address, targets::get())) return false;
	if (settings::silentaim::knock_check && is_player_knocked(entity)) return false;

	if ((settings::silentaim::dead_check && cache::is_dead(entity)) || op1::corpse(entity)) return false;

	if (settings::silentaim::health_check_enabled && entity.health < settings::silentaim::min_health) return false;
	if (ignore_friendly) return false;
	return true;
}

static bool get_part_position(const cache::entity_t& entity, const char* name, math::vector3& out)
{
	auto it = entity.parts.find(name);
	if (it == entity.parts.end()) return false;
	rbx::part_t part = it->second;
	if (!part.address) return false;

	if (op1::active())
		return op1::live_position(part, out);

	rbx::primitive_t prim = part.get_primitive();
	if (!prim.address) return false;
	try { out = prim.get_position(); return true; } catch (...) { return false; }
}

static bool get_target_position(
	const cache::entity_t& entity,
	const math::matrix4& view,
	const math::vector2& dims,
	const math::vector2& mouse_pos,
	math::vector3& out_pos)
{
	HWND wnd = FindWindowA(nullptr, "Roblox");

	auto to_client = [&](math::vector2& sp) {
		if (!wnd) return;
		POINT pt = { (LONG)sp.x, (LONG)sp.y };
		ScreenToClient(wnd, &pt);
		sp = { (float)pt.x, (float)pt.y };
	};
	int wanted = settings::silentaim::target_part;
	if (wanted >= 8)
	{

		static std::uint64_t s_last_rand_target = 0;
		static int           s_rand_group       = 0;
		if (entity.instance.address != s_last_rand_target)
		{
			s_last_rand_target = entity.instance.address;
			s_rand_group       = rand() % 6;
		}
		wanted = 2 + s_rand_group;
	}

	if (wanted == 0)
	{
		float best_dist_sq = FLT_MAX;
		math::vector3 best_pos;
		bool found = false;

		for (int group = 0; group < 6; ++group)
		{
			const char* part_name = limb_part_name(entity, group);
			if (!part_name) continue;

			try
			{
				auto it = entity.parts.find(part_name);
				if (it == entity.parts.end() || !it->second.address) continue;

				rbx::part_t part = it->second;
				rbx::primitive_t prim = part.get_primitive();
				if (!prim.address) continue;

				math::vector3 part_pos{};
				if (op1::active())
				{
					if (!op1::live_position(part, part_pos)) continue;
				}
				else
				{
					part_pos = prim.get_position();
				}
				math::vector3 part_size = prim.get_size();
				math::matrix3 part_rot  = prim.get_rotation();
				if (part_size.x == 0.f && part_size.y == 0.f && part_size.z == 0.f) continue;

				float hx = part_size.x * 0.5f;
				float hy = part_size.y * 0.5f;
				float hz = part_size.z * 0.5f;

				const float signs[8][3] = {
					{-1,-1,-1}, { 1,-1,-1}, {-1, 1,-1}, { 1, 1,-1},
					{-1,-1, 1}, { 1,-1, 1}, {-1, 1, 1}, { 1, 1, 1}
				};

				for (const auto& s : signs)
				{
					math::vector3 lc{ s[0]*hx, s[1]*hy, s[2]*hz };
					math::vector3 wc = part_pos + part_rot * lc;

					math::vector2 sp;
					if (!game::visengine.world_to_screen(wc, sp, dims, view)) continue;
					to_client(sp);

					float dx = mouse_pos.x - sp.x;
					float dy = mouse_pos.y - sp.y;
					float dsq = dx*dx + dy*dy;
					if (dsq < best_dist_sq) { best_dist_sq = dsq; best_pos = wc; found = true; }
				}
			}
			catch (...) {}
		}

		if (op1::active())
		{
			for (int i = 0; i < 2; ++i)
			{
				math::vector3 p{};
				if (!op1::aim_point(entity, i == 0 ? op1::bone::head : op1::bone::torso, p))
					continue;
				math::vector2 sp;
				if (!game::visengine.world_to_screen(p, sp, dims, view)) continue;
				to_client(sp);
				const float d = get_distance_from_center(mouse_pos, sp);
				if (d * d < best_dist_sq) { best_dist_sq = d * d; best_pos = p; found = true; }
			}
		}

		if (!found) return false;
		out_pos = best_pos;
	}
	else if (wanted == 1)
	{
		float best_dist = FLT_MAX;
		math::vector3 best_pos;
		bool found = false;

		for (int group = 0; group < 6; ++group)
		{
			const char* part_name = limb_part_name(entity, group);
			if (!part_name) continue;

			math::vector3 pos;
			if (!get_part_position(entity, part_name, pos)) continue;
			math::vector2 sp;
			if (!game::visengine.world_to_screen(pos, sp, dims, view)) continue;
			to_client(sp);
			float d = get_distance_from_center(mouse_pos, sp);
			if (d < best_dist) { best_dist = d; best_pos = pos; found = true; }
		}

		if (op1::active())
		{
			for (int i = 0; i < 2; ++i)
			{
				math::vector3 p{};
				if (!op1::aim_point(entity, i == 0 ? op1::bone::head : op1::bone::torso, p))
					continue;
				math::vector2 sp;
				if (!game::visengine.world_to_screen(p, sp, dims, view)) continue;
				to_client(sp);
				const float d = get_distance_from_center(mouse_pos, sp);
				if (d < best_dist) { best_dist = d; best_pos = p; found = true; }
			}
		}

		if (!found) return false;
		out_pos = best_pos;
	}
	else
	{
		const int group = part_to_group(wanted);
		bool have = false;

		if (op1::active() && (group == 0 || group == 1))
			have = op1::aim_point(entity,
				group == 0 ? op1::bone::head : op1::bone::torso, out_pos);

		if (!have)
		{
			const char* part_name = limb_part_name(entity, group);
			if (!part_name) return false;
			try
			{
				if (!get_part_position(entity, part_name, out_pos))
					return false;
			}
			catch (...) { return false; }
		}
	}

	if (settings::silentaim::visible_check)
	{
		const bool visible = op1::multipoint_los_enabled()
			? op1::visible_any(entity, out_pos, out_pos)
			: aim_point_visible(out_pos);

		if (!visible)
			return false;
	}

	if (!op1::no_smoothing_enabled())
	{
		if (settings::silentaim::enable_prediction)
			out_pos = apply_prediction(const_cast<cache::entity_t&>(entity), out_pos);

	}

	return true;
}

static cache::entity_t get_closest_to_mouse()
{
	if (!game::visengine.address || !game::datamodel.address)
		return cache::entity_t{};

	const bool ignore_friendly   = (settings::silentaim::priorities & (1 << 0)) != 0;
	const bool prioritise_hostile = (settings::silentaim::priorities & (1 << 1)) != 0;

	const math::matrix4 view = game::visengine.get_viewmatrix();
	math::vector2 dims = game::visengine.get_dimensions();

	HWND wnd = FindWindowA(nullptr, "Roblox");
	if (wnd)
	{
		RECT rc{};
		if (GetClientRect(wnd, &rc))
		{
			dims.x = (float)(rc.right  - rc.left);
			dims.y = (float)(rc.bottom - rc.top);
		}
	}

	POINT cursor{};
	GetCursorPos(&cursor);
	POINT client_cursor = cursor;
	if (wnd) ScreenToClient(wnd, &client_cursor);
	const math::vector2 mouse_pos{ (float)client_cursor.x, (float)client_cursor.y };

	std::vector<cache::entity_t> entities;
	cache::entity_t local;
	{
		std::lock_guard<std::mutex> lk(cache::mtx);
		entities = cache::cached_players;
		local    = cache::cached_local_player;
	}

	cache::entity_t best_target{};
	float best_dist = FLT_MAX;
	cache::entity_t best_hostile{};
	float best_hostile_dist = FLT_MAX;

	for (const auto& entity : entities)
	{
		if (settings::silentaim::knock_check && is_player_knocked(entity)) continue;
		if (!is_valid_target(entity, local, ignore_friendly)) continue;

		math::vector3 target_pos;
		if (!get_target_position(entity, view, dims, mouse_pos, target_pos)) continue;

		math::vector2 sp{};
		if (!game::visengine.world_to_screen(target_pos, sp, dims, view)) continue;

		POINT pt2 = { (LONG)sp.x, (LONG)sp.y };
		if (wnd) ScreenToClient(wnd, &pt2);
		sp = { (float)pt2.x, (float)pt2.y };

		const float dist = get_distance_from_center(mouse_pos, sp);
		if (settings::silentaim::use_fov && dist > settings::silentaim::fov) continue;

		if (prioritise_hostile)
		{
			if (dist < best_hostile_dist) { best_hostile_dist = dist; best_hostile = entity; }
		}
		else if (dist < best_dist)
		{
			best_dist = dist; best_target = entity;
		}
	}

	return (prioritise_hostile && best_hostile.instance.address) ? best_hostile : best_target;
}

static void update_raycast_subsystems(std::chrono::steady_clock::time_point now)
{
	const bool want = settings::silentaim::method == 2;
	static bool s_raycast_wanted = false;
	static auto s_next_try = std::chrono::steady_clock::now();

	if (want != s_raycast_wanted)
	{
		s_raycast_wanted = want;
		if (want)
		{
			raycast_silent::ensure(true);
			debug::log("silentaim", "raycast method selected, hook ensure()");
		}
		else
		{
			raycast_silent::ensure(false);
			debug::log("silentaim", "raycast method deselected, hook removed");
		}
		s_next_try = now + std::chrono::seconds(2);
	}
	else if (want && !raycast_silent::ready() && now >= s_next_try)
	{
		raycast_silent::ensure(true);
		s_next_try = now + std::chrono::seconds(2);
	}

	const bool want_engine = want;
	if (aether_raycast::cfg::enabled != want_engine)
		aether_raycast::cfg::enabled = want_engine;
}

static bool target_visible_to_raycast(const cache::entity_t& entity)
{

	if (!aether_raycast::cfg::enabled)
		return true;
	return aether_raycast::is_player_visible(entity.instance.address, {}).visible;
}

static void targeting_thread()
{
	using namespace std::chrono_literals;
	static std::int32_t aim_indicator_counter = 0;

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (!game::datamodel.address || !game::visengine.address)
		{
			std::this_thread::sleep_for(100ms);
			continue;
		}

		if (aim_indicator_counter++ % 10 == 0)
		{
			try
			{
				rbx::instance_t player_gui{};
				{
					std::lock_guard<std::mutex> lk(cache::mtx);
					if (cache::cached_local_player.instance.address)
						player_gui = cache::cached_local_player.instance.find_first_child("PlayerGui");
				}

				if (!player_gui.address)
				{
					silentaim::state.aim_indicator = rbx::instance_t{};
					continue;
				}

				rbx::instance_t aim_frame{};
				for (auto& child : player_gui.get_children())
				{
					if (!child.address) continue;
					std::string child_name = child.get_name();
					if (child_name == "Aim") { aim_frame = child; break; }

					std::string cls = child.get_class_name();
					if (cls == "Frame" || cls == "ScreenGui" || cls == "GuiObject")
					{
						std::string lower = child_name;
						std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
						if (lower.find("main") != std::string::npos)
						{
							for (auto& gc : child.get_children())
							{
								if (gc.address && gc.get_name() == "Aim") { aim_frame = gc; break; }
							}
							if (aim_frame.address) break;
						}
					}
				}

				if (aim_frame.address != silentaim::state.aim_indicator.address)
				{
					if (silentaim::state.aim_indicator.address && silentaim::state.has_original_sizes)
					{
						memory->write<math::vector2>(silentaim::state.aim_indicator.address + Offsets::GuiObject::Size, silentaim::state.original_size);
						for (const auto& [addr, sz] : silentaim::state.original_children_sizes)
							memory->write<math::vector2>(addr + Offsets::GuiObject::Size, sz);
					}

					silentaim::state.aim_indicator = aim_frame;
					silentaim::state.has_original_sizes = false;
					silentaim::state.original_children_sizes.clear();

					if (silentaim::state.aim_indicator.address)
					{
						silentaim::state.original_size = memory->read<math::vector2>(silentaim::state.aim_indicator.address + Offsets::GuiObject::Size);
						for (auto& child : silentaim::state.aim_indicator.get_children())
						{
							math::vector2 sz = memory->read<math::vector2>(child.address + Offsets::GuiObject::Size);
							silentaim::state.original_children_sizes.push_back({ child.address, sz });
						}
						silentaim::state.has_original_sizes = true;
					}
				}

				if (settings::silentaim::enabled && silentaim::state.aim_indicator.address && silentaim::state.has_original_sizes)
				{
					memory->write<math::vector2>(silentaim::state.aim_indicator.address + Offsets::GuiObject::Size, { 0, 0 });
					for (auto& child : silentaim::state.aim_indicator.get_children())
						memory->write<math::vector2>(child.address + Offsets::GuiObject::Size, { 0, 0 });
				}
				else if (silentaim::state.aim_indicator.address && silentaim::state.has_original_sizes)
				{
					memory->write<math::vector2>(silentaim::state.aim_indicator.address + Offsets::GuiObject::Size, silentaim::state.original_size);
					for (const auto& [addr, sz] : silentaim::state.original_children_sizes)
						memory->write<math::vector2>(addr + Offsets::GuiObject::Size, sz);
				}
			}
			catch (...)
			{
				silentaim::state.aim_indicator = rbx::instance_t{};
			}
		}

		if (!settings::silentaim::enabled && silentaim::state.aim_indicator.address && silentaim::state.has_original_sizes)
		{
			memory->write<math::vector2>(silentaim::state.aim_indicator.address + Offsets::GuiObject::Size, silentaim::state.original_size);
			for (const auto& [addr, sz] : silentaim::state.original_children_sizes)
				memory->write<math::vector2>(addr + Offsets::GuiObject::Size, sz);
		}

		const bool active = is_silent_aim_active();
		if (!active)
		{
			silentaim::state.data_ready    = false;
			silentaim::publish_target(cache::entity_t{});
			silentaim::state.target_world_pos = math::vector3{};
			continue;
		}

		const bool ignore_friendly = (settings::silentaim::priorities & (1 << 0)) != 0;

		if (settings::silentaim::sticky_aim && silentaim::state.target.instance.address)
		{
			cache::entity_t refreshed{};
			cache::entity_t local{};
			bool found = false;
			{
				std::lock_guard<std::mutex> lk(cache::mtx);
				local = cache::cached_local_player;
				for (const auto& e : cache::cached_players)
				{
					if (e.instance.address == silentaim::state.target.instance.address)
					{
						refreshed = e; found = true; break;
					}
				}
			}

			if (found)
			{
				if (is_valid_target(refreshed, local, ignore_friendly))
					silentaim::publish_target(refreshed);
				else
				{
					silentaim::publish_target(refreshed);
					silentaim::state.data_ready = false;
					continue;
				}
			}
			else
			{
				silentaim::publish_target(cache::entity_t{});
				silentaim::state.target_world_pos = math::vector3{};
				silentaim::state.data_ready    = false;
				continue;
			}
		}
		else if (!settings::silentaim::sticky_aim)
		{
			silentaim::publish_target(cache::entity_t{});
			silentaim::state.target_world_pos = math::vector3{};
		}

		if (!silentaim::state.target.instance.address)
		{
			cache::entity_t new_target{};

			if (settings::silentaim::use_aimbot_target)
			{
				cache::entity_t local{};
				{
					std::lock_guard<std::mutex> lk(cache::mtx);
					local = cache::cached_local_player;

					std::uint64_t pref_addr = targets::get();
					if (pref_addr == 0)
						pref_addr = aimbot::sticky_target.instance.address != 0
							? aimbot::sticky_target.instance.address
							: aimbot::player.instance.address;

					if (pref_addr != 0)
					{
						for (const auto& e : cache::cached_players)
						{
							if (e.instance.address == pref_addr) { new_target = e; break; }
						}
					}
				}

				if (new_target.instance.address && !is_valid_target(new_target, local, ignore_friendly))
					new_target = cache::entity_t{};
			}

			if (!new_target.instance.address && targets::get() == 0)
				new_target = get_closest_to_mouse();

			if (new_target.instance.address &&
				settings::silentaim::method == 2 &&
				!target_visible_to_raycast(new_target))
			{
				new_target = cache::entity_t{};
			}

			if (!new_target.instance.address)
			{
				silentaim::state.data_ready = false;
				continue;
			}

			silentaim::publish_target(new_target);
		}

		if (!game::visengine.address) { silentaim::state.data_ready = false; continue; }

		const math::matrix4 view = game::visengine.get_viewmatrix();

		HWND wnd = FindWindowA(nullptr, "Roblox");
		math::vector2 dims = game::visengine.get_dimensions();
		if (wnd)
		{
			RECT rc{};
			if (GetClientRect(wnd, &rc))
			{
				dims.x = (float)(rc.right  - rc.left);
				dims.y = (float)(rc.bottom - rc.top);
			}
		}

		POINT cursor{};
		GetCursorPos(&cursor);
		POINT client_cursor = cursor;
		if (wnd) ScreenToClient(wnd, &client_cursor);
		const math::vector2 mouse_pos{ (float)client_cursor.x, (float)client_cursor.y };

		math::vector3 target_pos;
		if (!get_target_position(silentaim::state.target, view, dims, mouse_pos, target_pos))
		{
			silentaim::state.target_world_pos = math::vector3{};
			silentaim::state.data_ready = false;
			continue;
		}

		silentaim::state.target_world_pos = target_pos;

		if (!game::visengine.world_to_screen(target_pos, silentaim::state.target_screen_pos, dims, view))
		{
			silentaim::state.target_world_pos = math::vector3{};
			silentaim::state.data_ready = false;
			continue;
		}

		if (wnd)
		{
			POINT pt = { (LONG)silentaim::state.target_screen_pos.x, (LONG)silentaim::state.target_screen_pos.y };
			ScreenToClient(wnd, &pt);
			silentaim::state.target_screen_pos = { (float)pt.x, (float)pt.y };
		}

		if (settings::silentaim::use_fov)
		{
			const float dist = get_distance_from_center(mouse_pos, silentaim::state.target_screen_pos);
			if (dist > settings::silentaim::fov)
			{
				silentaim::publish_target(cache::entity_t{});
				silentaim::state.target_world_pos = math::vector3{};
				silentaim::state.data_ready    = false;
				continue;
			}
		}

		silentaim::state.spoof_pos_x = (std::uint64_t)client_cursor.x;
		silentaim::state.spoof_pos_y = (std::uint64_t)client_cursor.y;
		silentaim::state.data_ready  = true;
	}
}

static void aim_application_thread()
{
	using namespace std::chrono_literals;

	std::uint64_t mouse_service = 0;
	rbx::c_silent_help mouse_helper{};
	bool mouse_initialized = false;

	math::vector2 last_target_pos{};
	auto last_valid_time = std::chrono::steady_clock::now();
	bool had_valid_target = false;

	for (;;)
	{
		const bool active     = is_silent_aim_active();
		const bool has_target = silentaim::state.data_ready && silentaim::state.target.instance.address != 0;
		const auto now        = std::chrono::steady_clock::now();

		update_raycast_subsystems(now);

		if (!active)
		{
			raycast_silent::set_active(false);
			aether_silent::pf::g_active = false;
			mouse_service      = 0;
			mouse_initialized  = false;
			had_valid_target   = false;
			std::this_thread::sleep_for(100ms);
			continue;
		}

		if (!game::datamodel.address)
		{
			raycast_silent::set_active(false);
			mouse_service     = 0;
			mouse_initialized = false;
			std::this_thread::sleep_for(10ms);
			continue;
		}

		if (has_target)
		{
			had_valid_target = true;
			last_valid_time  = now;
			last_target_pos  = silentaim::state.target_screen_pos;
		}
		else if (had_valid_target)
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_valid_time);
			if (elapsed.count() > 1) { had_valid_target = false; continue; }
		}
		else
		{
			continue;
		}

		math::vector2 target_pos = has_target ? silentaim::state.target_screen_pos : last_target_pos;
		if (target_pos.x < 0.f || target_pos.y < 0.f || target_pos.x > 10000.f || target_pos.y > 10000.f)
		{
			std::this_thread::sleep_for(10ms);
			continue;
		}

		if (settings::silentaim::hit_chance_enabled)
		{
			static unsigned  s_hc_rng      = 0xCAFEBABEu;
			static bool      s_hc_allowed  = true;
			static auto      s_hc_next     = std::chrono::steady_clock::now();

			if (now >= s_hc_next)
			{
				s_hc_rng ^= s_hc_rng << 13;
				s_hc_rng ^= s_hc_rng >> 17;
				s_hc_rng ^= s_hc_rng << 5;
				float roll = ((s_hc_rng >> 8) & 0xFFFFu) / 65535.f * 100.f;
				s_hc_allowed = (roll <= settings::silentaim::hit_chance);

				float fire_rate  = std::max(0.5f, settings::silentaim::hit_chance_fire_rate);
				long long interval_ms = (long long)(1000.f / fire_rate);
				s_hc_next = now + std::chrono::milliseconds(interval_ms);
			}

			if (!s_hc_allowed)
				continue;
		}

		try
		{
			if (settings::silentaim::method == 2)
			{

				raycast_silent::set_active(has_target, silentaim::state.target_world_pos);
			}
			else if (settings::silentaim::method == 1)
			{

				raycast_silent::set_active(false);
				aether_silent::pf::g_active = false;
				if (has_target)
				{
					const aether_silent::CameraTarget cam = aether_silent::resolve_camera();
					if (cam.valid)
						aether_silent::apply(cam, silentaim::state.target_world_pos,
							settings::aimbot::smoothingx);
				}
			}
			else if (settings::silentaim::method == 3)
			{

				raycast_silent::set_active(false);
				if (has_target)
				{
					const aether_silent::CameraTarget cam = aether_silent::resolve_camera();
					if (cam.valid)
					{
						math::vector3 want = silentaim::state.target_world_pos;
						const math::vector3 pos = aether_silent::camera_position(cam);
						math::vector3 fwd{ want.x - pos.x, want.y - pos.y, want.z - pos.z };
						const float m = aether_silent::len3(fwd);
						if (m > 1e-6f)
						{
							fwd = fwd * (1.f / m);
							math::matrix3 cur{};
							const math::matrix3 rot = aether_silent::read_rotation(cam, cur)
								? aether_silent::smooth_toward(cur, fwd, settings::aimbot::smoothingx)
								: aether_silent::matrix_from_forward(fwd);
							aether_silent::write_rotation(cam, rot);
							aether_silent::pf::g_matrix  = rot;
							aether_silent::pf::g_addr    = cam.address;
							aether_silent::pf::g_is_part = cam.is_part;
							aether_silent::pf::ensure_writer();
							aether_silent::pf::g_active  = true;
						}
					}
				}
				else
				{
					aether_silent::pf::g_active = false;
				}
			}
			else if (settings::silentaim::method == 0)
			{
				std::uint64_t workspace = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
				if (!workspace) { std::this_thread::sleep_for(10ms); continue; }

				std::uint64_t camera_addr = memory->read<std::uint64_t>(workspace + Offsets::Workspace::CurrentCamera);
				if (!camera_addr) { std::this_thread::sleep_for(10ms); continue; }

				rbx::camera_t cam{ camera_addr };

				HWND wnd = FindWindowA(nullptr, "Roblox");
				math::vector2 dims = game::visengine.get_dimensions();
				if (wnd)
				{
					RECT rc{};
					if (GetClientRect(wnd, &rc))
					{
						dims.x = (float)(rc.right  - rc.left);
						dims.y = (float)(rc.bottom - rc.top);
					}
				}

				POINT cursor{};
				GetCursorPos(&cursor);
				POINT cc = cursor;
				if (wnd) ScreenToClient(wnd, &cc);
				math::vector2 mouse_pos{ (float)cc.x, (float)cc.y };

				rbx::vector2int16 vp = rbx::camera_calculate_viewport(target_pos, dims, mouse_pos);
				cam.set_viewport(vp);
			}
			else
			{

				raycast_silent::set_active(false);
				aether_silent::pf::g_active = false;

				if (!mouse_service)
				{
					rbx::instance_t svc = game::datamodel.find_first_child("MouseService");
					mouse_service = svc.address;
					if (!mouse_service) { std::this_thread::sleep_for(10ms); continue; }
				}

				if (!mouse_initialized)
				{
					mouse_helper.initialize_mouse_service(mouse_service);
					mouse_initialized = true;
				}

				float mouse_y = has_target ? target_pos.y : target_pos.y + 58.f;
				math::vector2 spoofed{ target_pos.x, mouse_y };
				mouse_helper.write_mouse_position(mouse_service, spoofed.x, spoofed.y);

				if (settings::mouse_spoof::enabled)
				{
					HWND wnd2 = FindWindowA(nullptr, "Roblox");
					float abs_x = spoofed.x, abs_y = spoofed.y;
					if (wnd2)
					{
						POINT pt = { (LONG)spoofed.x, (LONG)spoofed.y };
						ClientToScreen(wnd2, &pt);
						abs_x = (float)pt.x;
						abs_y = (float)pt.y;
					}
					int screen_w = GetSystemMetrics(SM_CXSCREEN);
					int screen_h = GetSystemMetrics(SM_CYSCREEN);
					if (screen_w > 0 && screen_h > 0)
					{
						INPUT inp{};
						inp.type = INPUT_MOUSE;
						inp.mi.dx = (LONG)(abs_x * 65535.f / (float)screen_w);
						inp.mi.dy = (LONG)(abs_y * 65535.f / (float)screen_h);
						inp.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
						SendInput(1, &inp, sizeof(INPUT));
					}
				}
			}
		}
		catch (...)
		{
			mouse_service     = 0;
			mouse_initialized = false;
			std::this_thread::sleep_for(10ms);
		}
	}
}

void silentaim::run()
{
	std::thread(targeting_thread).detach();
	std::thread(aim_application_thread).detach();
}
