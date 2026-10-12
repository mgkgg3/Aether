#define NOMINMAX
#include "aimbot.h"

#include <features/aimbot/Telemetry.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <game/game.h>
#include <settings.h>
#include <sdk/math/math.h>
#include <sdk/sdk.h>
#include <memory/memory.h>
#include <game/game_context.h>
#include <sdk/offsets.h>
#include <keybind/keybind.h>
#include <keybind/live_state.h>
#include <cache/cache.h>
#include <debug/DebugConsole.h>
#include <features/worldgeo/WorldGeo.h>
#include <features/opone/OperationOne.h>
#include <features/players/friends.h>
#include <features/players/targets.h>
#include <mutex>
#include <imgui/imgui.h>

static float get_world_distance_to_entity(cache::entity_t& entity, cache::entity_t& local_player);

static bool is_friend_skipped(const cache::entity_t& entity)
{
	return friends::is_skipped(entity.instance.address, targets::get());
}

struct part_list_t { const char* names[5]; int count; };

static part_list_t resolve_part_list(int part)
{
	switch (part)
	{
	case 1: return { { "Head", "head" }, 2 };
	case 2: return { { "HumanoidRootPart", "torso" }, 2 };
	case 3: return { { "Left Arm",  "LeftUpperArm",  "LeftLowerArm",  "LeftHand",  "arm1" }, 5 };
	case 4: return { { "Right Arm", "RightUpperArm", "RightLowerArm", "RightHand", "arm2" }, 5 };
	case 5: return { { "Left Leg",  "LeftUpperLeg",  "LeftLowerLeg",  "LeftFoot",  "leg1" }, 5 };
	case 6: return { { "Right Leg", "RightUpperLeg", "RightLowerLeg", "RightFoot", "leg2" }, 5 };
	default: return { { nullptr }, 0 };
	}
}

namespace
{
	constexpr std::uint8_t k_camera_type_scriptable = 6;

	std::mutex    s_cam_mtx;
	bool          s_cam_want      = false;
	bool          s_cam_target_ok = false;
	math::matrix3 s_cam_target{};
	bool          s_cam_smoothing = false;
	float         s_cam_tx        = 1.f;
	float         s_cam_ty        = 1.f;
	bool          s_cam_taken     = false;
	std::uint8_t  s_cam_orig_type = 0;
	std::atomic<bool> s_cam_writer_started{ false };

	std::atomic<int>  s_candidates{ 0 };
	std::atomic<int>  s_rej_team{ 0 };
	std::atomic<int>  s_rej_friend{ 0 };
	std::atomic<int>  s_rej_dead{ 0 };
	std::atomic<int>  s_rej_knock{ 0 };
	std::atomic<int>  s_rej_health{ 0 };
	std::atomic<int>  s_rej_nopoint{ 0 };
	std::atomic<int>  s_rej_visible{ 0 };
	std::atomic<int>  s_locks{ 0 };
	std::atomic<std::uint64_t> s_mouse_moves{ 0 };
	std::atomic<std::uint64_t> s_mouse_idle{ 0 };

	std::atomic<float> s_mouse_gain_x{ 1.f };
	std::atomic<float> s_mouse_gain_y{ 1.f };

	math::matrix3     s_cam_last_written{};
	bool              s_cam_have_last = false;
	std::atomic<float> s_cam_overwrite_mismatch{ 0.f };
	std::atomic<std::uint64_t> s_cam_writes{ 0 };
	std::atomic<std::uint64_t> s_cam_taken_ms{ 0 };

	std::uint64_t camera_address()
	{

		return rbx::current_camera_address();
	}

	void restore_camera_type()
	{
		std::uint64_t cam = 0;
		std::uint8_t  orig = 0;
		bool          taken = false;
		{
			std::lock_guard<std::mutex> lk(s_cam_mtx);
			taken = s_cam_taken;
			orig  = s_cam_orig_type;
			s_cam_taken = false;
		}

		if (taken && (cam = camera_address()) != 0)
			memory->write<std::uint8_t>(cam + Offsets::Camera::CameraType, orig);
	}
}

void aimbot::release_camera()
{
	{
		std::lock_guard<std::mutex> lk(s_cam_mtx);
		s_cam_want      = false;
		s_cam_target_ok = false;
	}
	restore_camera_type();
}

static bool is_player_knocked(const cache::entity_t& player)
{

	return game_ctx::get_knock(player.instance.address) == game_ctx::KnockState::Yes;
}

struct rig_anchors_t
{
	bool          valid{ false };
	math::vector3 head{}, chest{}, hip{}, root{};
	math::vector3 left_arm{}, right_arm{};
	math::vector3 left_leg{}, right_leg{};
	float         height{ 5.f };
};

static bool build_rig_anchors(const cache::entity_t& entity, rig_anchors_t& out)
{
	struct body_part_t
	{
		math::vector3 pos{}, size{};
		math::matrix3 rot{};
		float         half_h{ 0.f };
		float         volume{ 0.f };
		bool          root{ false };
	};

	body_part_t parts[16]{};
	int count = 0;

	for (auto& [nm, part] : entity.parts)
	{
		if (count >= 16 || !part.address) continue;

		try
		{
			rbx::primitive_t prim = rbx::part_t(part).get_primitive();
			if (!prim.address) continue;

			const math::vector3 pos  = prim.get_position();
			const math::vector3 size = prim.get_size();
			if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) continue;
			if (!std::isfinite(size.x) || !std::isfinite(size.y) || !std::isfinite(size.z)) continue;
			if (size.x <= 0.f || size.y <= 0.f || size.z <= 0.f) continue;
			if (size.x > 64.f || size.y > 64.f || size.z > 64.f) continue;

			math::matrix3 rot = prim.get_rotation();
			if (!std::isfinite(rot.m[3]) || !std::isfinite(rot.m[4]) || !std::isfinite(rot.m[5]))
				rot = math::matrix3::identity();

			body_part_t& bp = parts[count++];
			bp.pos     = pos;
			bp.size    = size;
			bp.rot     = rot;
			bp.half_h  = cache::part_half_height(size, rot);
			bp.volume  = size.x * size.y * size.z;
			bp.root    = (nm == "HumanoidRootPart");
		}
		catch (...) {}
	}

	if (count == 0) return false;

	int root = -1;
	for (int i = 0; i < count; ++i)
		if (parts[i].root) { root = i; break; }

	if (root < 0)
	{
		float biggest = 0.f;
		for (int i = 0; i < count; ++i)
			if (parts[i].volume > biggest) { biggest = parts[i].volume; root = i; }
	}
	if (root < 0) return false;

	const math::vector3 root_pos = parts[root].pos;

	float top = -FLT_MAX, bottom = FLT_MAX;
	for (int i = 0; i < count; ++i)
	{
		const math::vector3 d = parts[i].pos - root_pos;
		if (sqrtf(d.x*d.x + d.y*d.y + d.z*d.z) > 12.f) continue;
		if (parts[i].pos.y + parts[i].half_h > top)    top    = parts[i].pos.y + parts[i].half_h;
		if (parts[i].pos.y - parts[i].half_h < bottom) bottom = parts[i].pos.y - parts[i].half_h;
	}

	float height = top - bottom;
	if (!std::isfinite(height) || height < 2.f || height > 12.f)
	{
		height = parts[root].half_h * 2.f;
		if (!std::isfinite(height) || height < 2.f || height > 12.f)
			height = 5.f;
		top    = root_pos.y + height * 0.5f;
		bottom = root_pos.y - height * 0.5f;
	}

	math::vector3 fwd = parts[root].rot.forward();
	fwd.y = 0.f;
	if (!std::isfinite(fwd.x) || !std::isfinite(fwd.z) || fwd.length() < 0.5f)
		fwd = math::vector3{ 0.f, 0.f, -1.f };
	fwd = fwd.normalized();

	const math::vector3 right{ -fwd.z, 0.f, fwd.x };

	const float half_w   = std::clamp(height * 0.17f, 0.35f, 1.6f);
	const float leg_span = half_w * 0.45f;

	auto at = [&](float from_top, float lateral)
	{
		return math::vector3{
			root_pos.x + right.x * lateral,
			top - height * from_top,
			root_pos.z + right.z * lateral
		};
	};

	out.root      = root_pos;
	out.head      = at(0.08f,  0.f);
	out.chest     = at(0.30f,  0.f);
	out.hip       = at(0.52f,  0.f);
	out.left_arm  = at(0.32f,  half_w);
	out.right_arm = at(0.32f, -half_w);
	out.left_leg  = at(0.80f,  leg_span);
	out.right_leg = at(0.80f, -leg_span);
	out.height    = height;
	out.valid     = true;
	return true;
}

static bool is_jumping(const cache::entity_t& entity)
{

	if (entity.state == 3 || entity.state == 5)
		return true;

	auto root_it = entity.parts.find("HumanoidRootPart");
	if (root_it == entity.parts.end() || !root_it->second.address)
		return false;

	try
	{
		std::uint64_t prim_address = memory->read<std::uint64_t>(root_it->second.address + Offsets::BasePart::Primitive);
		if (!prim_address) return false;
		math::vector3 velocity = memory->read<math::vector3>(prim_address + Offsets::Primitive::AssemblyLinearVelocity);
		return velocity.y > 5.0f;
	}
	catch (...) { return false; }
}

static math::vector3 get_velocity(cache::entity_t& entity)
{
	auto root_it = entity.parts.find("HumanoidRootPart");
	if (root_it == entity.parts.end() || !root_it->second.address)
		return math::vector3(0.f, 0.f, 0.f);

	rbx::primitive_t prim = root_it->second.get_primitive();
	if (!prim.address) return math::vector3(0.f, 0.f, 0.f);

	try { return memory->read<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity); }
	catch (...) { return math::vector3(0.f, 0.f, 0.f); }
}

static math::vector3 apply_prediction(cache::entity_t& entity, const math::vector3& position, float body_height)
{
	math::vector3 velocity = get_velocity(entity);

	float px, py;
	if (settings::aimbot::air_prediction_enabled && is_jumping(entity))
	{
		px = (20.f - settings::aimbot::air_prediction_x) * 0.02f;
		py = (20.f - settings::aimbot::air_prediction_y) * 0.02f;
	}
	else
	{
		px = (20.f - settings::aimbot::prediction_x) * 0.02f;
		py = (20.f - settings::aimbot::prediction_y) * 0.02f;
	}

	const float body = (body_height > 1.f && body_height < 20.f) ? body_height : 5.f;
	const float max_vertical = body * 0.30f;
	const float lead_y = std::clamp(velocity.y * py, -max_vertical, max_vertical);

	return { position.x + velocity.x * px,
	         position.y + lead_y,
	         position.z + velocity.z * px };
}

namespace easing
{
	float linear(float t)              { return t; }
	float ease_in_quad(float t)        { return t * t; }
	float ease_out_quad(float t)       { return t * (2 - t); }
	float ease_in_out_quad(float t)    { return t < 0.5f ? 2*t*t : -1+(4-2*t)*t; }
	float ease_in_cubic(float t)       { return t * t * t; }
	float ease_out_cubic(float t)      { return (--t)*t*t+1; }
	float ease_in_out_cubic(float t)   { return t<0.5f ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1; }
	float ease_in_sine(float t)        { return 1.f - cosf((t*3.14159265f)/2.f); }
	float ease_out_sine(float t)       { return sinf((t*3.14159265f)/2.f); }
	float ease_in_out_sine(float t)    { return -(cosf(3.14159265f*t)-1.f)/2.f; }
}

static constexpr float k_choice_margin = 6.f;

static constexpr float         k_dwell_margin_px = 26.f;
static constexpr std::uint64_t k_switch_dwell_ms = 250;

static constexpr float k_priority_margin_frac = 0.12f;
static constexpr float k_health_margin       = 5.f;

static constexpr float k_reaim_px = 25.f;

static float get_distance_from_center(const math::vector2& point, const math::vector2& position)
{
	float dx = position.x - point.x;
	float dy = position.y - point.y;
	return std::sqrt(dx*dx + dy*dy);
}

static bool aim_reference(math::vector2& out)
{
	if (game::visengine.address)
	{
		const math::vector2 dims = game::visengine.get_dimensions();
		if (dims.x > 1.f && dims.y > 1.f)
		{
			rbx::visualengine_t::update_window_offset();
			out = { rbx::visualengine_t::window_offset_x() + dims.x * 0.5f,
			        rbx::visualengine_t::window_offset_y() + dims.y * 0.5f };
			return true;
		}
	}

	POINT cursor{};
	if (!GetCursorPos(&cursor))
		return false;

	out = { (float)cursor.x, (float)cursor.y };
	return true;
}

static bool get_target_point_for_entity(
	const cache::entity_t& entity,
	const math::matrix4& view,
	const math::vector2& dims,
	float fov,
	float& out_distance,
	math::vector2& out_screen,
	bool* out_crosshair = nullptr)
{
	if (out_crosshair) *out_crosshair = false;

	math::vector2 cursor_pos{};
	if (!aim_reference(cursor_pos))
		return false;
	float best_distance = FLT_MAX;
	bool found = false;
	const char* best_name = nullptr;

	struct cand_t { const char* name; math::vector2 screen; float distance; };
	std::vector<cand_t> cands;
	cands.reserve(16);

	auto project_only = [&](const math::vector3& p, math::vector2& out) -> bool
	{
		if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
			return false;
		return game::visengine.world_to_screen(p, out, dims, view);
	};

	const bool hold_choice = true;

	struct choice_t { std::string name; std::uint64_t ms{ 0 }; };
	static thread_local std::unordered_map<std::uintptr_t, choice_t> s_choice;
	if (s_choice.size() > 128) s_choice.clear();

	const std::uint64_t choice_now = ::GetTickCount64();
	const char* held_name = nullptr;
	if (hold_choice)
	{
		auto it = s_choice.find((std::uintptr_t)entity.instance.address);
		if (it != s_choice.end() && !it->second.name.empty() &&
			choice_now - it->second.ms <= 400)
			held_name = it->second.name.c_str();
	}

	auto try_point = [&](math::vector3 pos_3d, float body_height, const char* name)
	{
		if (!std::isfinite(pos_3d.x) || !std::isfinite(pos_3d.y) || !std::isfinite(pos_3d.z))
			return;

		if (settings::aimbot::enable_prediction && !op1::no_smoothing_enabled())
		{
			try { pos_3d = apply_prediction(const_cast<cache::entity_t&>(entity), pos_3d, body_height); } catch (...)
			{}
		}

		math::vector2 pos_2d{};
		if (!game::visengine.world_to_screen(pos_3d, pos_2d, dims, view)) return;

		const float distance = get_distance_from_center(cursor_pos, pos_2d);

		const bool gate_fov = settings::aimbot::use_fov ||
			(op1::active() && settings::aimbot::draw_fov);
		if (gate_fov && distance > fov) return;

		cands.push_back({ name, pos_2d, distance });
		found = true;
	};

	auto remember_choice = [&]()
	{
		if (!hold_choice || !best_name) return;
		s_choice[(std::uintptr_t)entity.instance.address] = choice_t{ best_name, choice_now };
	};

		const bool body_only = op1::has_animated_body(entity);
	auto try_part = [&](const char* name)
	{
		if (body_only && op1::is_collider_body(name))
			return;

		rbx::part_t part{};
		if (const rbx::part_t* found = op1::find_part(entity, name))
			part = *found;
		if (!part.address) return;
	rbx::primitive_t primitive = part.get_primitive();
	if (!primitive.address) return;

	math::vector3 pos_3d;
	if (op1::active())
	{

		if (!op1::live_position(part, pos_3d)) return;
	}
	else
	{
		try { pos_3d = primitive.get_position(); } catch (...) { return; }
	}
	try_point(pos_3d, 0.f, name);
	};

	int target_part_to_use = settings::aimbot::target_part;
	if (settings::aimbot::air_part_enabled && is_jumping(entity))
		target_part_to_use = settings::aimbot::air_part;

	const part_list_t parts = resolve_part_list(target_part_to_use);
	if (parts.count == 0)
	{

		for (auto& [nm, part] : entity.parts)
			if (part.address && !(body_only && op1::is_collider_body(nm)))
				try_part(nm.c_str());
	}
	else
	{
		for (int i = 0; i < parts.count; ++i)
			try_part(parts.names[i]);
	}

	if (!found && op1::active())
	{
		switch (target_part_to_use)
		{
		case 1:
		{
			math::vector3 p{};
			if (op1::aim_point(entity, op1::bone::head, p)) try_point(p, 0.f, "head");
			break;
		}
		default:
		{
			math::vector3 head{}, chest{};
			if (op1::aim_point(entity, op1::bone::head, head))   try_point(head,  0.f, "head");
			if (op1::aim_point(entity, op1::bone::torso, chest)) try_point(chest, 0.f, "torso");
			break;
		}
		}
	}

	if (!found)
	{
		rig_anchors_t anchors{};
		if (build_rig_anchors(entity, anchors))
		{

			switch (target_part_to_use)
			{
			case 1:  try_point(anchors.head,      anchors.height, nullptr); break;
			case 2:  try_point(anchors.root,      anchors.height, nullptr); break;
			case 3:  try_point(anchors.left_arm,  anchors.height, nullptr); break;
			case 4:  try_point(anchors.right_arm, anchors.height, nullptr); break;
			case 5:  try_point(anchors.left_leg,  anchors.height, nullptr); break;
			case 6:  try_point(anchors.right_leg, anchors.height, nullptr); break;
			default:

				try_point(anchors.head,      anchors.height, nullptr);
				try_point(anchors.chest,     anchors.height, nullptr);
				try_point(anchors.left_arm,  anchors.height, nullptr);
				try_point(anchors.right_arm, anchors.height, nullptr);
				try_point(anchors.left_leg,  anchors.height, nullptr);
				try_point(anchors.right_leg, anchors.height, nullptr);
				break;
			}
		}
	}

	if (cands.empty()) return false;

	if (out_crosshair && op1::active())
	{
		static const op1::bone k_outline[] = {
			op1::bone::head, op1::bone::torso,
			op1::bone::left_arm,  op1::bone::right_arm,
			op1::bone::left_leg,  op1::bone::right_leg
		};

		float min_x = FLT_MAX, min_y = FLT_MAX, max_x = -FLT_MAX, max_y = -FLT_MAX;
		for (op1::bone b : k_outline)
		{
			math::vector3 world{};
			math::vector2 screen{};
			if (!op1::aim_point(entity, b, world)) continue;
			if (!project_only(world, screen))       continue;
			min_x = std::min(min_x, screen.x); max_x = std::max(max_x, screen.x);
			min_y = std::min(min_y, screen.y); max_y = std::max(max_y, screen.y);
		}

		if (min_x <= max_x)
		{
			constexpr float k_pad = 2.f;
			*out_crosshair = cursor_pos.x >= min_x - k_pad && cursor_pos.x <= max_x + k_pad &&
			                 cursor_pos.y >= min_y - k_pad && cursor_pos.y <= max_y + k_pad;
		}
	}

	const cand_t* chosen = &cands[0];
	for (const cand_t& c : cands)
		if (c.distance < chosen->distance)
			chosen = &c;

	if (hold_choice && held_name)
	{
		const cand_t* held = nullptr;
		for (const cand_t& c : cands)
			if (c.name && std::strcmp(c.name, held_name) == 0)
			{
				held = &c;
				break;
			}

		if (held && held->distance <= chosen->distance + k_choice_margin)
			chosen = held;
	}

	best_distance = chosen->distance;
	best_name     = chosen->name;
	out_screen    = chosen->screen;
	remember_choice();

	out_distance = best_distance;
	return true;
}

static std::uint64_t mouse_send_interval_ms()
{
	const int manual = settings::aimbot::pace_ms;
	if (manual > 0) return (std::uint64_t)std::min(manual, 64);

	const int limit = settings::menu::fps_limit;
	if (limit <= 0)   return 4;
	if (limit > 1000) return 1;
	return (std::uint64_t)std::max(1, 1000 / limit);
}

static constexpr float k_mouse_settled_px = 2.5f;

static constexpr std::uint64_t k_mouse_stale_ms = 25;

struct mouse_step_t { float ex = 0.f, ey = 0.f; LONG sx = 0, sy = 0; };

static void mouse_aim(const math::vector2& target)
{
	math::vector2 centre{};
	if (!aim_reference(centre)) return;

	const float delta_x  = target.x - centre.x;
	const float delta_y  = target.y - centre.y;
	const float distance = std::sqrt(delta_x*delta_x + delta_y*delta_y);

	static thread_local mouse_step_t  s_last{};
	static thread_local bool          s_last_valid = false;
	static thread_local std::uint64_t s_last_ms    = 0;
	static thread_local std::uint64_t s_paced_ms   = 0;
	static thread_local float         s_kx = 3.f, s_ky = 3.f;
	static thread_local float         s_rem_x = 0.f, s_rem_y = 0.f;

	const std::uint64_t now_ms = ::GetTickCount64();
	const std::uint64_t interval_ms = mouse_send_interval_ms();
	const float settled = std::max(k_mouse_settled_px, 0.55f * std::max(s_kx, s_ky));

	if (distance < settled)
	{
		s_last_valid = false;
		s_last_ms    = 0;
		s_paced_ms   = 0;
		s_rem_x = s_rem_y = 0.f;
		s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	if (s_paced_ms && now_ms - s_paced_ms < interval_ms)
	{
		s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
		return;
	}
	s_paced_ms = now_ms;

	if (s_last_valid)
	{
		const float answered_x = std::max(1.f, 0.25f * s_kx * (float)std::labs(s_last.sx));
		const float answered_y = std::max(1.f, 0.25f * s_ky * (float)std::labs(s_last.sy));

		if (std::fabs(delta_x - s_last.ex) < answered_x &&
			std::fabs(delta_y - s_last.ey) < answered_y &&
			now_ms - s_last_ms < k_mouse_stale_ms)
		{
			s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
			return;
		}

		if (s_last.sx != 0)
		{
			const float measured = (s_last.ex - delta_x) / (float)s_last.sx;
			if (std::isfinite(measured) && measured > 0.02f && measured < 50.f)
				s_kx = s_kx * 0.5f + measured * 0.5f;
		}
		if (s_last.sy != 0)
		{
			const float measured = (s_last.ey - delta_y) / (float)s_last.sy;
			if (std::isfinite(measured) && measured > 0.02f && measured < 50.f)
				s_ky = s_ky * 0.5f + measured * 0.5f;
		}

		s_last_valid = false;
	}

	const float kx = std::max(0.05f, s_kx);
	const float ky = std::max(0.05f, s_ky);

	float gain_x = 0.9f, gain_y = 0.9f;
	if (settings::aimbot::smoothing && !settings::aimbot::instant_lock)
	{
		float tx = 1.f / settings::aimbot::smoothingx;
		float ty = 1.f / settings::aimbot::smoothingy;
		tx = std::max(0.01f, std::min(1.f, tx));
		ty = std::max(0.01f, std::min(1.f, ty));

		float sfx, sfy;
		switch (settings::aimbot::smoothing_style) {
		case 1:  sfx=easing::linear(tx);          sfy=easing::linear(ty);          break;
		case 2:  sfx=easing::ease_in_quad(tx);    sfy=easing::ease_in_quad(ty);    break;
		case 3:  sfx=easing::ease_out_quad(tx);   sfy=easing::ease_out_quad(ty);   break;
		case 4:  sfx=easing::ease_in_out_quad(tx);sfy=easing::ease_in_out_quad(ty);break;
		case 5:  sfx=easing::ease_in_cubic(tx);   sfy=easing::ease_in_cubic(ty);   break;
		case 6:  sfx=easing::ease_out_cubic(tx);  sfy=easing::ease_out_cubic(ty);  break;
		case 7:  sfx=easing::ease_in_out_cubic(tx);sfy=easing::ease_in_out_cubic(ty);break;
		case 8:  sfx=easing::ease_in_sine(tx);    sfy=easing::ease_in_sine(ty);    break;
		case 9:  sfx=easing::ease_out_sine(tx);   sfy=easing::ease_out_sine(ty);   break;
		case 10: sfx=easing::ease_in_out_sine(tx);sfy=easing::ease_in_out_sine(ty);break;
		default: sfx=tx; sfy=ty; break;
		}

		gain_x = std::min(1.f, sfx * (float)interval_ms);
		gain_y = std::min(1.f, sfy * (float)interval_ms);
	}

	const float limit = distance * 1.5f;
	s_rem_x += std::clamp(delta_x * gain_x / kx, -limit, limit);
	s_rem_y += std::clamp(delta_y * gain_y / ky, -limit, limit);

	const LONG move_x = (LONG)std::round(s_rem_x);
	const LONG move_y = (LONG)std::round(s_rem_y);

	if (move_x == 0 && move_y == 0)
	{
		s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	s_rem_x -= (float)move_x;
	s_rem_y -= (float)move_y;

	s_last.ex = delta_x;  s_last.ey = delta_y;
	s_last.sx = move_x;   s_last.sy = move_y;
	s_last_valid = true;
	s_last_ms    = now_ms;

	s_mouse_gain_x.store(s_kx, std::memory_order_relaxed);
	s_mouse_gain_y.store(s_ky, std::memory_order_relaxed);

	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dx = move_x;
	input.mi.dy = move_y;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(INPUT));
	s_mouse_moves.fetch_add(1, std::memory_order_relaxed);
}

static void mouse_aimbot()
{
	if (aimbot::player.instance.address == 0) return;

	math::matrix4 view = game::visengine.get_viewmatrix();
	math::vector2 dims = game::visengine.get_dimensions();

	cache::entity_t local;
	{ std::lock_guard<std::mutex> lk(cache::mtx); local = cache::cached_local_player; }

	if (get_world_distance_to_entity(aimbot::player, local) == FLT_MAX) return;

	float distance{};
	math::vector2 target_screen{};
	if (get_target_point_for_entity(aimbot::player, view, dims, FLT_MAX, distance, target_screen))
	{
		if (settings::aimbot::offset_enabled)
		{
			target_screen.x += settings::aimbot::offset_x;
			target_screen.y += settings::aimbot::offset_y;
		}
		mouse_aim(target_screen);
	}
}

static math::matrix3 look_at(const math::vector3& from, const math::vector3& to)
{
	math::vector3 forward = to - from;
	float length = forward.length();
	if (length < 0.0001f) return math::matrix3::identity();
	forward = forward * (1.f / length);

	math::vector3 world_up(0.f, 1.f, 0.f);
	math::vector3 right = world_up.cross(forward);
	length = right.length();
	if (length < 0.0001f)
	{
		world_up = math::vector3(0.f, 0.f, 1.f);
		right = world_up.cross(forward);
		length = right.length();
		if (length < 0.0001f) return math::matrix3::identity();
	}
	right = right * (1.f / length);
	math::vector3 up = forward.cross(right);

	math::matrix3 result{};
	result.m[0] = -right.x;  result.m[1] = up.x;  result.m[2] = -forward.x;
	result.m[3] = -right.y;  result.m[4] = up.y;  result.m[5] = -forward.y;
	result.m[6] = -right.z;  result.m[7] = up.z;  result.m[8] = -forward.z;
	return result;
}

static float wrap_pi(float a)
{
	while (a >  3.14159265f) a -= 6.28318531f;
	while (a < -3.14159265f) a += 6.28318531f;
	return a;
}

static void rotation_to_yaw_pitch(const math::matrix3& rot, float& yaw, float& pitch)
{
	math::vector3 fwd = rot.forward();
	float horiz = std::sqrt(fwd.x * fwd.x + fwd.z * fwd.z);
	yaw   = std::atan2(-fwd.x, -fwd.z);
	pitch = std::atan2(fwd.y, horiz);
}

static math::matrix3 rotation_from_yaw_pitch(float yaw, float pitch)
{
	float cy = std::cos(yaw),  sy = std::sin(yaw);
	float cp = std::cos(pitch), sp = std::sin(pitch);

	math::vector3 fwd(-sy * cp, sp, -cy * cp);
	math::vector3 up(0.f, 1.f, 0.f);
	math::vector3 right = up.cross(fwd);
	float rlen = right.length();
	if (rlen < 0.0001f)
	{
		up = math::vector3(0.f, 0.f, 1.f);
		right = up.cross(fwd);
		rlen = right.length();
		if (rlen < 0.0001f) return math::matrix3::identity();
	}
	right = right * (1.f / rlen);
	math::vector3 u = fwd.cross(right);

	math::matrix3 r{};
	r.m[0] = -right.x; r.m[1] = u.x; r.m[2] = -fwd.x;
	r.m[3] = -right.y; r.m[4] = u.y; r.m[5] = -fwd.y;
	r.m[6] = -right.z; r.m[7] = u.z; r.m[8] = -fwd.z;
	return r;
}

static float aim_smoothing_factor(float raw)
{
	float t = 1.f / raw;
	t = std::max(0.01f, std::min(1.f, t));

	switch (settings::aimbot::smoothing_style) {
	case 1:  return easing::linear(t);
	case 2:  return easing::ease_in_quad(t);
	case 3:  return easing::ease_out_quad(t);
	case 4:  return easing::ease_in_out_quad(t);
	case 5:  return easing::ease_in_cubic(t);
	case 6:  return easing::ease_out_cubic(t);
	case 7:  return easing::ease_in_out_cubic(t);
	case 8:  return easing::ease_in_sine(t);
	case 9:  return easing::ease_out_sine(t);
	case 10: return easing::ease_in_out_sine(t);
	default: return t;
	}
}

static void publish_camera_target(const math::matrix3& rot, bool smoothing)
{
	std::lock_guard<std::mutex> lk(s_cam_mtx);
	s_cam_want      = true;
	s_cam_target_ok = true;
	s_cam_target    = rot;
	s_cam_smoothing = smoothing;
	s_cam_tx = smoothing ? aim_smoothing_factor(settings::aimbot::smoothingx) : 1.f;
	s_cam_ty = smoothing ? aim_smoothing_factor(settings::aimbot::smoothingy) : 1.f;
}

static void clear_camera_target()
{
	std::lock_guard<std::mutex> lk(s_cam_mtx);
	s_cam_want      = false;
	s_cam_target_ok = false;
}

static void camera_writer()
{
	for (;;)
	{
		bool          want = false, target_ok = false, smooth = false;
		math::matrix3 target{};
		float         tx = 1.f, ty = 1.f;
		{
			std::lock_guard<std::mutex> lk(s_cam_mtx);
			want      = s_cam_want;
			target_ok = s_cam_target_ok;
			smooth    = s_cam_smoothing;
			target    = s_cam_target;
			tx        = s_cam_tx;
			ty        = s_cam_ty;
		}

		const std::uint64_t cam = camera_address();

		if (!cam)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			continue;
		}

		if (!want)
		{
			restore_camera_type();
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			continue;
		}

		{
			std::lock_guard<std::mutex> lk(s_cam_mtx);
			if (!s_cam_taken)
			{
				s_cam_orig_type = memory->read<std::uint8_t>(cam + Offsets::Camera::CameraType);
				s_cam_taken     = true;
			}
		}
		memory->write<std::uint8_t>(cam + Offsets::Camera::CameraType, k_camera_type_scriptable);

		if (s_cam_have_last)
		{
			const math::matrix3 now_rot = memory->read<math::matrix3>(cam + Offsets::Camera::Rotation);
			float worst = 0.f;
			for (int i = 0; i < 9; ++i)
				worst = std::max(worst, std::fabs(now_rot.m[i] - s_cam_last_written.m[i]));
			s_cam_overwrite_mismatch.store(worst, std::memory_order_relaxed);
		}

		if (target_ok)
		{
			math::matrix3 out = target;

			float raw_yaw = 0.f, raw_pitch = 0.f, step_yaw = 0.f, step_pitch = 0.f;

			if (smooth)
			{

				const math::matrix3 cur = memory->read<math::matrix3>(cam + Offsets::Camera::Rotation);
				float cur_yaw{}, cur_pitch{}, tgt_yaw{}, tgt_pitch{};
				rotation_to_yaw_pitch(cur,    cur_yaw, cur_pitch);
				rotation_to_yaw_pitch(target, tgt_yaw, tgt_pitch);

				const float d_yaw   = wrap_pi(tgt_yaw - cur_yaw);
				const float d_pitch = tgt_pitch - cur_pitch;
				const float pitch_limit = 1.5533f;

				const float new_pitch =
					std::max(-pitch_limit, std::min(pitch_limit, cur_pitch + d_pitch * ty));
				out = rotation_from_yaw_pitch(cur_yaw + d_yaw * tx, new_pitch);

				raw_yaw   = d_yaw;
				raw_pitch = d_pitch;
				step_yaw   = d_yaw   * tx;
				step_pitch = d_pitch * ty;
			}

			memory->write<math::matrix3>(cam + Offsets::Camera::Rotation, out);

			{
				aimbot::telemetry_sample_t ts{};
				ts.t          = aimbot::telemetry_now();
				ts.raw_yaw    = raw_yaw;
				ts.raw_pitch  = raw_pitch;
				ts.step_yaw   = step_yaw;
				ts.step_pitch = step_pitch;
				ts.sx         = tx;
				ts.sy         = ty;

				ts.primary    = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
				aimbot::telemetry_push(ts);
			}

			s_cam_last_written = out;
			s_cam_have_last    = true;
			s_cam_writes.fetch_add(1, std::memory_order_relaxed);
			s_cam_taken_ms.store(GetTickCount64(), std::memory_order_relaxed);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

static void ensure_camera_writer()
{
	bool expected = false;
	if (s_cam_writer_started.compare_exchange_strong(expected, true))
		std::thread(camera_writer).detach();
}

static void camera_aimbot()
{
	if (aimbot::player.instance.address == 0) { clear_camera_target(); return; }

	std::uint64_t camera = camera_address();
	if (!camera) { clear_camera_target(); return; }

	math::vector3 target_pos;
	bool found_part = false;

	int target_part_to_use = settings::aimbot::target_part;
	if (settings::aimbot::air_part_enabled && is_jumping(aimbot::player))
		target_part_to_use = settings::aimbot::air_part;

	auto try_get = [&](const char* name) -> bool {
		auto it = aimbot::player.parts.find(name);
		if (it == aimbot::player.parts.end()) return false;
		rbx::part_t p = it->second;
		if (!p.address) return false;
		rbx::primitive_t prim = p.get_primitive();
		if (!prim.address) return false;
		try { target_pos = prim.get_position(); return true; } catch (...) { return false; }
	};

	const part_list_t parts = resolve_part_list(target_part_to_use);
	if (parts.count == 0)
	{

		const math::vector3 cam_pos_tmp = memory->read<math::vector3>(camera + Offsets::Camera::Position);
		float best = FLT_MAX;
		for (auto& [name, part] : aimbot::player.parts)
		{
			if (!part.address) continue;
			rbx::primitive_t prim = part.get_primitive();
			if (!prim.address) continue;
			math::vector3 pos = prim.get_position();
			if (!math::finite(pos)) continue;
			float d = (pos - cam_pos_tmp).length();
			if (d < best) { best = d; target_pos = pos; found_part = true; }
		}
	}
	else
	{
		for (int i = 0; i < parts.count && !found_part; ++i)
			found_part = try_get(parts.names[i]);
	}

	if (!found_part) { clear_camera_target(); return; }

	if (settings::aimbot::enable_prediction)
	{

		target_pos = apply_prediction(aimbot::player, target_pos, 0.f);
	}

	math::vector3 camera_pos = memory->read<math::vector3>(camera + Offsets::Camera::Position);
	math::matrix3 target_rot = look_at(camera_pos, target_pos);
	math::matrix3 current_rot = memory->read<math::matrix3>(camera + Offsets::Camera::Rotation);

	if (settings::aimbot::offset_enabled)
	{

		const math::vector3 right = current_rot.right();
		const math::vector3 up    = current_rot.up();
		const math::vector2 dims  = game::visengine.get_dimensions();
		const float dist = (target_pos - camera_pos).length();

		const float tan_y  = std::tan(rbx::camera_t(camera).get_field_of_view() * 0.5f);
		const float aspect = (dims.y > 1.f) ? dims.x / dims.y : 1.7778f;
		const float tan_x  = tan_y * aspect;

		const float scale_x = (dims.x > 1.f) ? dist * tan_x * 2.f / dims.x : 0.f;
		const float scale_y = (dims.y > 1.f) ? dist * tan_y * 2.f / dims.y : 0.f;

		target_pos = target_pos + right * (settings::aimbot::offset_x * scale_x)
		                       + up * (-settings::aimbot::offset_y * scale_y);
		target_rot = look_at(camera_pos, target_pos);
	}

	const bool smooth = settings::aimbot::smoothing && !settings::aimbot::instant_lock;
	publish_camera_target(target_rot, smooth);
}

static float get_world_distance_to_entity(cache::entity_t& entity, cache::entity_t& local_player)
{
	auto local_it = local_player.parts.find("HumanoidRootPart");
	if (local_it == local_player.parts.end() || !local_it->second.address) return FLT_MAX;

	auto target_it = entity.parts.find("HumanoidRootPart");
	if (target_it == entity.parts.end() || !target_it->second.address) return FLT_MAX;

	rbx::primitive_t local_prim  = local_it->second.get_primitive();
	rbx::primitive_t target_prim = target_it->second.get_primitive();
	if (!local_prim.address || !target_prim.address) return FLT_MAX;

	try
	{
		math::vector3 lp = local_prim.get_position();
		math::vector3 tp = target_prim.get_position();
		return lp.distance(tp);
	}
	catch (...) { return FLT_MAX; }
}

static bool target_is_visible(const cache::entity_t& entity);

static cache::entity_t find_cached_player(std::uint64_t address)
{
	cache::entity_t out{};
	if (!address)
		return out;

	std::lock_guard<std::mutex> lk(cache::mtx);
	for (const cache::entity_t& entity : cache::cached_players)
		if (entity.instance.address == address)
			return entity;

	return out;
}

static cache::entity_t get_closest_player(const math::matrix4& view, const math::vector2& dims)
{
	cache::entity_t best{};
	float closest = FLT_MAX;

	std::vector<cache::entity_t> entities;
	cache::entity_t local;
	{
		std::lock_guard<std::mutex> lk(cache::mtx);
		entities = cache::cached_players;
		local    = cache::cached_local_player;
	}

	const std::uint64_t previous = aimbot::player.instance.address;

	const float hold_px = std::clamp(settings::aimbot::fov, k_reaim_px, 120.f);
	bool  prev_seen = false;
	bool  prev_under = false;
	float prev_distance = FLT_MAX;
	float prev_priority = 0.f;
	bool  prev_priority_ok = false;

	cache::entity_t best_crosshair{};
	float closest_crosshair = FLT_MAX;

	struct priority_pick_t { bool valid = false; float score = 0.f; cache::entity_t ent{}; };
	priority_pick_t best_priority;

	for (cache::entity_t& entity : entities)
	{
		if (!entity.instance.address) continue;
		if (entity.instance.address == local.instance.address) continue;

		s_candidates.fetch_add(1, std::memory_order_relaxed);

		if (settings::aimbot::teamcheck && cache::is_teammate(entity)) { s_rej_team.fetch_add(1, std::memory_order_relaxed); continue; }
		if (is_friend_skipped(entity)) { s_rej_friend.fetch_add(1, std::memory_order_relaxed); continue; }
		if (settings::aimbot::knock_check && is_player_knocked(entity)) { s_rej_knock.fetch_add(1, std::memory_order_relaxed); continue; }

		if ((settings::aimbot::dead_check && cache::is_dead(entity)) || op1::corpse(entity))
		{ s_rej_dead.fetch_add(1, std::memory_order_relaxed); continue; }
		if (settings::aimbot::visible_check && !target_is_visible(entity)) { s_rej_visible.fetch_add(1, std::memory_order_relaxed); continue; }
		if (settings::aimbot::health_check_enabled && entity.health < settings::aimbot::min_health) { s_rej_health.fetch_add(1, std::memory_order_relaxed); continue; }

		bool on_crosshair = false;
		float distance{};
		math::vector2 target_screen{};
		const float ent_fov = (previous && entity.instance.address == previous)
			? settings::aimbot::fov + hold_px
			: settings::aimbot::fov;

		if (!get_target_point_for_entity(entity, view, dims, ent_fov, distance, target_screen, &on_crosshair))
		{
			s_rej_nopoint.fetch_add(1, std::memory_order_relaxed);
			continue;
		}

		if (on_crosshair && distance < closest_crosshair)
		{
			closest_crosshair = distance;
			best_crosshair    = entity;
		}

		if (distance < closest) { closest = distance; best = entity; }

		if (settings::aimbot::priority == 1)
		{
			const float wd = get_world_distance_to_entity(entity, local);
			if (wd < FLT_MAX && (!best_priority.valid || wd < best_priority.score))
			{
				best_priority.valid = true;
				best_priority.score = wd;
				best_priority.ent   = entity;
			}
		}
		else if (settings::aimbot::priority == 2)
		{
			if (!best_priority.valid || entity.health < best_priority.score)
			{
				best_priority.valid = true;
				best_priority.score = entity.health;
				best_priority.ent   = entity;
			}
		}

		if (previous && entity.instance.address == previous)
		{
			prev_seen     = true;
			prev_under    = on_crosshair;
			prev_distance = distance;

			if (settings::aimbot::priority == 1)
			{
				prev_priority    = get_world_distance_to_entity(entity, local);
				prev_priority_ok = prev_priority < FLT_MAX;
			}
			else if (settings::aimbot::priority == 2)
			{
				prev_priority    = entity.health;
				prev_priority_ok = true;
			}
		}
	}

	auto entity_at = [&](std::uint64_t address) -> cache::entity_t
	{
		for (cache::entity_t& entity : entities)
			if (entity.instance.address == address)
				return entity;
		return cache::entity_t{};
	};

	if (settings::aimbot::priority != 0 && best_priority.valid)
	{
		if (prev_seen && prev_priority_ok && best_priority.ent.instance.address != previous)
		{
			const float keep_within = (settings::aimbot::priority == 1)
				? prev_priority * (1.f + k_priority_margin_frac)
				: prev_priority + k_health_margin;
			if (prev_priority <= keep_within)
			{
				cache::entity_t held = entity_at(previous);
				if (held.instance.address) return held;
			}
		}
		return best_priority.ent;
	}

	{
		static std::uint64_t s_lock_addr = 0;
		static std::uint64_t s_lock_ms   = 0;
		constexpr std::uint64_t k_lock_grace_ms = 200;

		const std::uint64_t now_ms = ::GetTickCount64();

		if (!previous)
		{

			s_lock_addr = 0;
		}
		else if (prev_seen)
		{
			s_lock_addr = previous;
			s_lock_ms   = now_ms;
		}
		else if (s_lock_addr == previous && now_ms - s_lock_ms <= k_lock_grace_ms)
		{

			for (cache::entity_t& entity : entities)
			{
				if (entity.instance.address != previous) continue;
				if (entity.state == 15) break;
				if (settings::aimbot::teamcheck && cache::is_teammate(entity)) break;
				if (is_friend_skipped(entity)) break;
				if (settings::aimbot::knock_check && is_player_knocked(entity)) break;
				return entity;
			}
		}
	}

	if (prev_seen && prev_distance <= hold_px)
	{
		cache::entity_t held = entity_at(previous);
		if (held.instance.address)
			return held;
	}

	if (best_crosshair.instance.address)
	{

		if (prev_seen && prev_under &&
			prev_distance <= closest_crosshair + k_choice_margin)
			return entity_at(previous);
		return best_crosshair;
	}

	{
		static std::uint64_t s_held_addr = 0;
		static std::uint64_t s_held_ms   = 0;

		const std::uint64_t held_now = ::GetTickCount64();

		if (previous != s_held_addr)
		{
			s_held_addr = previous;
			s_held_ms   = held_now;
		}

		const bool  fresh  = previous != 0 && (held_now - s_held_ms) <= k_switch_dwell_ms;
		const float margin = fresh ? k_dwell_margin_px : k_choice_margin;

		if (prev_seen && best.instance.address != previous && !prev_under &&
			prev_distance <= closest + margin)
		{
			cache::entity_t held = entity_at(previous);
			if (held.instance.address)
				return held;
		}
	}

	return best;
}

static bool is_target_valid(const cache::entity_t& entity)
{
	if (entity.instance.address == 0) return false;
	if (entity.instance.address == cache::cached_local_player.instance.address) return false;
	if (settings::aimbot::teamcheck && cache::is_teammate(entity)) return false;
	if (is_friend_skipped(entity)) return false;
	if (settings::aimbot::knock_check && is_player_knocked(entity)) return false;
	if ((settings::aimbot::dead_check && cache::is_dead(entity)) || op1::corpse(entity)) return false;
	if (settings::aimbot::visible_check && !target_is_visible(entity)) return false;
	if (settings::aimbot::health_check_enabled && entity.health < settings::aimbot::min_health) return false;
	return true;
}

static bool target_is_visible(const cache::entity_t& entity)
{
	auto it = entity.parts.find("HumanoidRootPart");
	if (it == entity.parts.end() || !it->second.address)
		return true;

	math::vector3 body{};
	try
	{
		rbx::primitive_t prim = it->second.get_primitive();
		if (!prim.address)
			return true;
		body = prim.get_position();
	}
	catch (...) { return true; }

	if (!math::finite(body))
		return true;

	if (op1::multipoint_los_enabled())
		return op1::visible_any(entity, body, body);

	return worldgeo::is_visible(worldgeo::eye_position(body), body);
}

void aimbot::draw_fov(ImDrawList* dl, const math::vector2& dims)
{
	if (!settings::aimbot::draw_fov) return;

	const auto& fc = settings::aimbot::fov_circle_colour;
	const auto& oc = settings::aimbot::fov_outline_colour;

	POINT cursor{};
	if (!GetCursorPos(&cursor)) return;
	ImVec2 center((float)cursor.x, (float)cursor.y);
	float radius = settings::aimbot::fov;

	ImU32 outline_col = IM_COL32((int)(oc[0]*255),(int)(oc[1]*255),(int)(oc[2]*255),(int)(oc[3]*255));
	ImU32 circle_col  = IM_COL32((int)(fc[0]*255),(int)(fc[1]*255),(int)(fc[2]*255),(int)(fc[3]*255));

	dl->AddCircle(center, radius, outline_col, 0, 3.f);
	dl->AddCircle(center, radius, circle_col,  0, 1.f);
}

void aimbot::run()
{
	using namespace std::chrono_literals;

	keybind aimbot_kb("aimbot");
	aimbot_kb.key  = settings::aimbot::keybind;
	aimbot_kb.type = static_cast<keybind::c_keybind_type>(settings::aimbot::activation_mode);

	ensure_camera_writer();

	for (;;)
	{
		if (!settings::aimbot::enabled)
		{
			aimbot::sticky_target = cache::entity_t{};
			clear_camera_target();
			live_state::set(live_state::kAimbot, false);
			Sleep(1);
			continue;
		}

		aimbot_kb.key  = settings::aimbot::keybind;
		aimbot_kb.type = static_cast<keybind::c_keybind_type>(settings::aimbot::activation_mode);
		aimbot_kb.update();

		live_state::set(live_state::kAimbot, aimbot_kb.enabled);

		if (!aimbot_kb.enabled)
		{
			aimbot::sticky_target = cache::entity_t{};
			aimbot::player        = cache::entity_t{};
			clear_camera_target();
			Sleep(1);
			continue;
		}

		math::matrix4 view = game::visengine.get_viewmatrix();
		math::vector2 dims = game::visengine.get_dimensions();

		cache::entity_t target{};

		const bool sticky_mode = settings::aimbot::sticky_aim || settings::aimbot::mode == 1;

		const std::uint64_t pinned = targets::get();

		if (pinned)
		{
			target = find_cached_player(pinned);

			if (target.instance.address && !is_target_valid(target))
				target = cache::entity_t{};
		}
		else if (sticky_mode && aimbot::sticky_target.instance.address != 0)
		{
			target = find_cached_player(aimbot::sticky_target.instance.address);

			bool found = target.instance.address != 0;
			if (found && is_target_valid(target) && settings::aimbot::use_fov)
			{

				float d{}; math::vector2 sp{};
				if (!get_target_point_for_entity(target, view, dims, settings::aimbot::fov, d, sp))
					found = false;
			}
			if (!found || !is_target_valid(target))
			{
				aimbot::sticky_target = cache::entity_t{};
				target = get_closest_player(view, dims);
			}
		}
		else
		{
			target = get_closest_player(view, dims);
		}

		if (target.instance.address)
			s_locks.fetch_add(1, std::memory_order_relaxed);

		{
			static auto s_aim_log = std::chrono::steady_clock::now() - std::chrono::seconds(6);
			const auto log_now = std::chrono::steady_clock::now();
			if (log_now - s_aim_log >= std::chrono::seconds(5))
			{
				s_aim_log = log_now;

				const HWND foreground = GetForegroundWindow();
				const bool focused = foreground != nullptr && game::wnd != nullptr
					&& (foreground == game::wnd || GetAncestor(foreground, GA_ROOT) == GetAncestor(game::wnd, GA_ROOT));

				const float overwrite = s_cam_overwrite_mismatch.load(std::memory_order_relaxed);
				const bool  held = overwrite < 0.002f;

				POINT cur{ };
				POINT* curp = GetCursorPos(&cur) ? &cur : nullptr;

				debug::log("aimbot",
					"%s key=%s | cand=%d locks=%d | rejected team=%d friend=%d dead=%d knock=%d hp=%d noaim=%d vis=%d | cam writes=%llu %s overwrite=%.3f | mouse moves=%llu already-on-target=%llu pxpercount=%.2f/%.2f cursor=%ld,%ld focus=%s",
					settings::aimbot::mode == 0 ? "mouse" : "camera",
					aimbot_kb.enabled ? "down" : "up",
					s_candidates.exchange(0, std::memory_order_relaxed),
					s_locks.exchange(0, std::memory_order_relaxed),
					s_rej_team.exchange(0, std::memory_order_relaxed),
					s_rej_friend.exchange(0, std::memory_order_relaxed),
					s_rej_dead.exchange(0, std::memory_order_relaxed),
					s_rej_knock.exchange(0, std::memory_order_relaxed),
					s_rej_health.exchange(0, std::memory_order_relaxed),
					s_rej_nopoint.exchange(0, std::memory_order_relaxed),
					s_rej_visible.exchange(0, std::memory_order_relaxed),
					(unsigned long long)s_cam_writes.load(std::memory_order_relaxed),
					settings::aimbot::mode == 0 ? "-" : (held ? "held" : "LOST"),
					overwrite,
					(unsigned long long)s_mouse_moves.load(std::memory_order_relaxed),
					(unsigned long long)s_mouse_idle.exchange(0, std::memory_order_relaxed),
					(double)s_mouse_gain_x.load(std::memory_order_relaxed),
					(double)s_mouse_gain_y.load(std::memory_order_relaxed),
					curp ? curp->x : -1L, curp ? curp->y : -1L,
					focused ? "game" : "NOT the game");
			}
		}

		if (!target.instance.address) { clear_camera_target(); Sleep(1); continue; }

		if (sticky_mode)
			aimbot::sticky_target = target;

		aimbot::player = target;

		if (settings::aimbot::mode == 0)
		{
			clear_camera_target();
			mouse_aimbot();
		}
		else
		{
			camera_aimbot();
		}

		Sleep(1);
	}
}
