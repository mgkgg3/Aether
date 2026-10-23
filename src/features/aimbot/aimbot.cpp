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
#include <render/ShapeDraw.h>
#include <features/players/targets.h>
#include <mutex>
#include <imgui/imgui.h>

static float get_world_distance_to_entity(cache::entity_t& entity, cache::entity_t& local_player);

// Friends are held back like teammates, except when the players tab's target
// button named them - that is the user asking for them, so it outranks the flag.
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
	bool          s_cam_smoothing = false;		float         s_cam_tx        = 0.f;   // ms; 0 is a lock
		float         s_cam_ty        = 0.f;
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
	// Where the mouse path stopped before it ever reached a count.  Both used to
	// return silently: a lock was logged every five seconds and nothing said the
	// aim had no local player to measure from, which reads exactly like "the
	// aimbot does nothing".
	std::atomic<std::uint64_t> s_mouse_no_local{ 0 };
	std::atomic<std::uint64_t> s_mouse_no_point{ 0 };

	std::atomic<float> s_mouse_gain_x{ 1.f };
	std::atomic<float> s_mouse_gain_y{ 1.f };

	math::matrix3     s_cam_last_written{};
	bool              s_cam_have_last = false;

	// A matrix entry's noise.  Below this the game's rotation is our own write
	// read back, above it the camera really moved.
	constexpr float   k_cam_readback_noise = 0.004f;
	std::chrono::steady_clock::time_point s_cam_step_at{};
	bool                                  s_cam_have_step = false;
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

// Milliseconds on the performance counter.
//
// `GetTickCount64` is the system timer, and by default it answers in steps of
// 15.6 ms: every interval measured with it is either 0 or a whole tick.  The aim
// loop runs at a millisecond, so with that clock it could only step once every
// 15.6 ms and every "time since the last step" came out the same - the steps
// arrived in lumps and the point filter advanced by a whole tick at a time.  A
// high resolution clock is what makes a millisecond mean a millisecond, which is
// what a paced loop, a time constant and a filter all need to be even.
static double now_ms_hires()
{
	using clock = std::chrono::steady_clock;
	static const clock::time_point start = clock::now();
	return std::chrono::duration<double, std::milli>(clock::now() - start).count();
}

// The speed model both aim modes are built on.
//
// The slider is a time constant now, not a percentage: it is how long the
// approach takes to close most of what is left, in milliseconds.  Everything
// else - the share one step earns, the shape of the approach - is derived from
// that, so both modes move at the speed the slider names instead of at the
// speed of whichever loop happens to read it.
namespace smoothing
{
	// 12 ms at 0 (a quick lock) to 362 ms at 100: 63% of the gap is gone after
	// that long and ~95% after three of them, so the slider is a real glide at
	// the top end instead of a lock with a label.  The old curve topped out at
	// 99 ms, which is under two frames of an approach - every setting above the
	// middle read as an instant lock, which is why turning smoothing up barely
	// changed how the aim felt.
	float tau_ms(float slider)
	{
		const float s = std::clamp(slider, 0.f, 100.f);
		return 12.f + s * 3.5f;
	}

	// The share of the gap a step taken `dt_ms` after the last one may close.
	// Exponential in time, so the slider means the same thing at 60, 144 or 360
	// fps: a shorter interval gets a proportionally smaller step instead of a
	// slower aim.  A step is sometimes taken between two of the client's frames
	// and sometimes several times inside one, and this is what makes those two
	// cases the same speed.
	float for_step(float tau, double dt_ms)
	{
		if (!(tau > 0.f)) return 1.f;
		const float a = 1.f - std::exp(-(float)std::clamp(dt_ms, 0.5, 250.0) / tau);
		return std::clamp(a, 0.0005f, 1.f);
	}

	// The style's speed at `progress`, relative to a straight line.
	//
	// This is the derivative of the curve, not its value.  Read as a speed,
	// ease-out starts quick and lands soft, ease-in the other way round, and
	// every style arrives in about the same time because they are all the same
	// approach under a different profile.  The floor matters: a profile that
	// starts at zero speed is an aim that does not move for the first tenth of
	// a second, and that is half of what "barely moves" was.
	float speed(int style, float progress)
	{
		const float p = std::clamp(progress, 0.f, 1.f);
		float k = 1.f;
		switch (style)
		{
		case 2:  k = 2.f * p; break;                                  // ease-in quad
		case 3:  k = 2.f * (1.f - p); break;                          // ease-out quad
		case 4:  k = p < 0.5f ? 4.f * p : 4.f * (1.f - p); break;     // ease-in-out quad
		case 5:  k = 3.f * p * p; break;                              // ease-in cubic
		case 6:  k = 3.f * (1.f - p) * (1.f - p); break;              // ease-out cubic
		case 7:  k = p < 0.5f ? 12.f * p * p : 12.f * (1.f - p) * (1.f - p); break;
		case 8:  k = 1.5708f * std::sin(1.5708f * p); break;          // ease-in sine
		case 9:  k = 1.5708f * std::cos(1.5708f * p); break;          // ease-out sine
		case 10: k = 1.5708f * std::sin(3.1416f * p); break;          // ease-in-out sine
		default: k = 1.f; break;
		}
		// The floor keeps the soft end of ease-out and the slow start of ease-in
		// from stretching an approach into a creep; the cap keeps the fast end
		// of the sharp curves from reading as a snap.
		return std::clamp(k, 0.55f, 1.8f);
	}

	// The share of the gap this step takes: the time it earned, shaped by the
	// style at how far along the approach is.  It never rounds to nothing on its
	// own - the remainder carries the sub-pixel part to the next step, and the
	// floor in `speed` keeps the first steps of every style moving.
	float step(int style, float progress, float tau, double dt_ms)
	{
		return std::clamp(for_step(tau, dt_ms) * speed(style, progress), 0.f, 1.f);
	}

	// How far along an approach is, from the gap it started at.
	//
	// Measured from the distance actually closed, not counted in steps.  The old
	// count advanced on every step whether or not the aim moved, so a slider
	// that started gentle collapsed into an instant lock a few frames later, and
	// a target that ran kept the aim pinned at the lock end for as long as it
	// ran: the same setting was three different speeds depending on the fight.
	float progress_from(float start_gap, float gap)
	{
		if (!(start_gap > 1e-6f)) return 1.f;
		return std::clamp(1.f - gap / start_gap, 0.f, 1.f);
	}
}

// How far ahead of a moving target a smoothed aim has to point.
//
// A smoothed approach settles with a constant error behind a target that walks
// away from it: each step closes a share of a gap that is itself moving, so the
// gap it comes to rest at is the target's speed times the time constant.  At 10
// that is a few centimetres and invisible; at 80 it is most of a body, and the
// aim reads as *behind* the target however smooth it looks - which is the other
// half of "smoothing barely moves".  Pointing the step at where the target will
// be one time constant from now removes the lag without touching the approach:
// the step is still a share of the gap, the gap is just measured to the
// target's future instead of to its past.
static math::vector3 smoothing_lead(cache::entity_t& entity)
{
	if (!settings::aimbot::smoothing || settings::aimbot::instant_lock)
		return { 0.f, 0.f, 0.f };

	struct vel_t { math::vector3 v{}; double ms{ 0.0 }; };
	static thread_local std::unordered_map<std::uintptr_t, vel_t> s_vel;
	if (s_vel.size() > 128) s_vel.clear();

	const double now_ms = now_ms_hires();
	vel_t& st = s_vel[(std::uintptr_t)entity.instance.address];

	math::vector3 raw = get_velocity(entity);
	if (!math::finite(raw)) raw = { 0.f, 0.f, 0.f };

	if (raw.length() > 250.f)   // not a person walking - a teleport, or a bad read
	{
		st = vel_t{};
		return { 0.f, 0.f, 0.f };
	}

	if (!st.ms)
	{
		st.v  = raw;
		st.ms = now_ms;
	}
	else if (now_ms > st.ms)
	{
		// 60 ms of averaging.  The root's velocity is read from memory the
		// physics step rewrites every frame, and one noisy read must not swing
		// the aim point: that would be the smoothing adding shake instead of
		// taking it out.
		const float a = 1.f - (float)std::exp(-std::min(now_ms - st.ms, 250.0) / 60.0);
		st.v = st.v + (raw - st.v) * a;
		st.ms = now_ms;
	}
	// Several candidates are resolved inside the same millisecond while the
	// aim is picked; the estimate stands for all of them.

	// The point filter's own lag is part of the lead too: it is a first-order
	// filter of the same kind, so it costs its time constant the same way.
	constexpr float k_point_lag_s = 0.015f;

	const float tx = smoothing::tau_ms(settings::aimbot::smoothingx) * 0.001f + k_point_lag_s;
	const float ty = smoothing::tau_ms(settings::aimbot::smoothingy) * 0.001f + k_point_lag_s;

	math::vector3 lead{ st.v.x * tx, st.v.y * ty, st.v.z * tx };

	const float len = lead.length();
	if (len > 4.f)
		lead = lead * (4.f / len);

	return lead;
}

static constexpr float k_choice_margin = 6.f;

// A target that was just picked keeps a wider margin for a moment, so a second
// player a couple of pixels closer cannot take the frame every other tick.
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

// The crosshair - where the shot goes - in the same desktop pixels world_to_screen
// answers in. The game holds the physical cursor here while it has the focus, so a
// reading taken mid-re-centre carries a step of our own noise; the crosshair never
// moves. Everything that decides what the aim is on is measured from this point.
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

// How hard the aim point is smoothed, and how far it may move before it is a jump
// rather than noise.  15 ms is under a frame at 60 Hz: enough to take the shiver
// out of a rig whose parts are re-read between client frames, short enough that
// the aim is not visibly behind a running target - and the lead covers the rest.
static constexpr float k_point_tau_ms  = 15.f;
// A jump bigger than this is a new aim point, not noise: a teleport, or the aim
// moving to another part of the body.  It was 60 px, which a fast target crosses
// between two of the client's frames at close range, so the filter snapped on
// every frame and passed the whole staircase through.
static constexpr float k_point_snap_px = 140.f;

struct point_filter_t
{
	math::vector2 p{};
	double        ms{ 0.0 };
	bool          valid{ false };
};

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

	// Every distance that picks or holds a target is measured from the crosshair, the
	// same point the mouse loop drives. The cursor sits on it while the game has the
	// focus, but only between re-centres, and a mid-re-centre reading is a step of noise.
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

	// The body part the aim is on is held in every mode. It used to be op1 only, so
	// with a part target of "closest to the cursor" the point jumped between head,
	// arms and torso as the cursor drifted - the aim shook without changing target.
	const bool hold_choice = true;

	struct choice_t { std::string name; double ms{ 0.0 }; };
	static thread_local std::unordered_map<std::uintptr_t, choice_t> s_choice;
	if (s_choice.size() > 128) s_choice.clear();

	const double choice_now = now_ms_hires();
	const char* held_name = nullptr;
	if (hold_choice)
	{
		auto it = s_choice.find((std::uintptr_t)entity.instance.address);
		if (it != s_choice.end() && !it->second.name.empty() &&
			choice_now - it->second.ms <= 400)
			held_name = it->second.name.c_str();
	}

	// One lead per resolve, in 3D, beside the prediction it is added to - so a
	// moving target is aimed where it is going rather than where it was when
	// the smoothing started its approach.
	const math::vector3 lead = smoothing_lead(const_cast<cache::entity_t&>(entity));

	auto try_point = [&](math::vector3 pos_3d, float body_height, const char* name)
	{
		if (!std::isfinite(pos_3d.x) || !std::isfinite(pos_3d.y) || !std::isfinite(pos_3d.z))
			return;

		if (settings::aimbot::enable_prediction && !op1::no_smoothing_enabled())
		{
			try { pos_3d = apply_prediction(const_cast<cache::entity_t&>(entity), pos_3d, body_height); } catch (...)
			{}
		}

		pos_3d = pos_3d + lead;

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

	// The aim point, low-passed before it reaches the mouse.
	//
	// A rig's part positions are read once per cache pass and this point is
	// rebuilt from them on every ask - the same body part answers with a
	// slightly different pixel each read, from its own rotation, from the
	// prediction lead, from the cache landing between two of the client's
	// frames.  The aim followed that noise exactly, which is a crosshair that
	// shivers on a standing target.  It is a filter on the point, not on the
	// error, so nothing about how a step is sized changes; a jump passes
	// through untouched, because that is a new aim point and not noise.
	static thread_local std::unordered_map<std::uintptr_t, point_filter_t> s_point;
	if (s_point.size() > 128) s_point.clear();

	point_filter_t& pf = s_point[(std::uintptr_t)entity.instance.address];
	const float jump_x = chosen->screen.x - pf.p.x;
	const float jump_y = chosen->screen.y - pf.p.y;
	const float jump   = std::sqrt(jump_x * jump_x + jump_y * jump_y);

	if (!pf.valid || !std::isfinite(pf.p.x) || !std::isfinite(pf.p.y) ||
		jump > k_point_snap_px || choice_now - pf.ms > 250.0)
	{
		pf.p     = chosen->screen;
		pf.ms    = choice_now;
		pf.valid = true;
	}
	else
	{
		// Sub-millisecond time, so the filter converges in a steady curve instead
		// of in whole ticks - a filter fed a 0/15.6 ms clock alternates between
		// not moving and moving a tick's worth, which is a shiver of its own.
		const float a = 1.f - (float)std::exp(-(choice_now - pf.ms) / (double)k_point_tau_ms);
		pf.p.x += jump_x * a;
		pf.p.y += jump_y * a;
		pf.ms = choice_now;
	}

	out_screen = pf.p;
	remember_choice();

	out_distance = best_distance;
	return true;
}

// How long to wait between steps. The game applies the mouse counts the OS accumulated
// since its last frame in one go, so the loop must not command the same correction twice
// inside one frame. The overlay's own frame limit (`ui.fps_limit`, in the config) is the
// frame count it knows about; the freshness check below keeps the loop honest when the
// game renders slower than that.
static std::uint64_t mouse_send_interval_ms()
{
	const int limit = settings::menu::fps_limit;
	if (limit <= 0)   return 4;
	if (limit > 1000) return 1;
	return (std::uint64_t)std::max(1, 1000 / limit);
}

// Idle distance. The steps are sized from a measured gain, so this can be tight; it
// widens with the gain, because one count of mouse is worth that many pixels and no
// finer correction exists.
static std::uint64_t camera_step_interval_ms()
{
	const int limit = settings::menu::fps_limit;
	if (limit <= 0)   return 4;
	if (limit > 1000) return 1;
	return (std::uint64_t)std::max(1, 1000 / limit);
}

// The mouse path.
//
// The aim is a guided motion, not a series of corrections.  Every tick the gap is
// measured, the pixels already asked for but not yet seen in the game are taken off
// it, and a share of what is left goes out as counts.  Booking that debt is what
// takes the stutter out: a step sent between two of the client's frames is not sent
// again inside the same frame, so there is no "has it been applied yet" guess to
// stall on - and the old loop stalled for a frame and a half whenever the guess was
// wrong, which on a moving target is most of the time.  The counts are carried in
// pixels as well, so a slow plan becomes a steady trickle of one count at a time
// instead of a count and then nothing for four frames.
static void mouse_aim(const math::vector2& target, std::uint64_t addr)
{
	math::vector2 centre{};
	if (!aim_reference(centre)) return;

	const float delta_x  = target.x - centre.x;
	const float delta_y  = target.y - centre.y;
	const float distance = std::sqrt(delta_x*delta_x + delta_y*delta_y);

	static thread_local float  s_kx = 3.f, s_ky = 3.f;              // screen px per count
	static thread_local float  s_carry_x = 0.f, s_carry_y = 0.f;    // px owed to the next count
	static thread_local float  s_owed_x = 0.f, s_owed_y = 0.f;      // px asked for, not applied yet
	static thread_local float  s_seen_x = 0.f, s_seen_y = 0.f;      // the gap the debt is booked against
	static thread_local float  s_win_x = 0.f, s_win_y = 0.f;        // the gap at the last observed move
	static thread_local LONG   s_win_counts_x = 0, s_win_counts_y = 0;
	static thread_local bool   s_on_target = false;                 // inside the deadband
	static thread_local std::uint64_t s_approach_addr = 0;          // (not `s_addr`: winsock macros that name)
	static thread_local float  s_start_gap = 0.f;
	static thread_local float  s_start_x = 1.f, s_start_y = 1.f;
	static thread_local double s_paced_ms = 0.0;                    // when the loop last decided
	static thread_local double s_sent_ms  = 0.0;                    // when the last count went out
	// Damping: the fraction of what is left each send takes.  It is adapted, not
	// measured - see the block below.
	static thread_local float  s_damp = 0.55f;
	static thread_local float  s_prev_err_x = 0.f, s_prev_err_y = 0.f;
	static thread_local float  s_prev_step  = 0.f;
	static thread_local double s_prev_send_ms = 0.0;
	// The share of the gap the smoothing asks for, and the same share after
	// the startup ramp - see the gain block below.
	static thread_local float  s_share_x = 0.f, s_share_y = 0.f;
	static thread_local float  s_kx_prev = 0.f, s_ky_prev = 0.f;
	static thread_local int    s_kx_agree = 0, s_ky_agree = 0;

	const double now_ms      = now_ms_hires();
	const double interval_ms = (double)mouse_send_interval_ms();

	auto forget_approach = [&]()
	{
		s_carry_x = s_carry_y = 0.f;
		s_owed_x  = s_owed_y  = 0.f;
		s_seen_x  = delta_x;
		s_seen_y  = delta_y;
		s_win_x   = delta_x;
		s_win_y   = delta_y;
		s_win_counts_x = s_win_counts_y = 0;
		s_start_gap = 0.f;
		s_approach_addr = 0;
		s_paced_ms = 0.0;
		s_sent_ms  = 0.0;
		s_share_x  = s_share_y = 0.f;
	};

	// A count is the smallest move the game can be asked for, so the aim cannot be
	// held closer than one of them.  Inside the deadband every correction would be a
	// whole count, and a whole count back and forth is the tremor that sits on a
	// standing target - a body's idle animation is worth about a pixel and it never
	// stops.  The band is a little wider than one count and is released wider still,
	// so a target drifting inside it is left alone while one that walks out is
	// picked up again on the next tick.
	const float dead_band = std::max(2.5f, 1.2f * std::max(s_kx, s_ky));

	if (s_on_target)
	{
		if (distance < dead_band * 1.6f)
		{
			s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
			return;
		}
		s_on_target = false;
		forget_approach();
	}

	if (distance <= dead_band)
	{
		s_on_target = true;
		forget_approach();
		s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	// ---- what one count is worth ---------------------------------------------
	//
	// One sample per observed move: the counts sent since the gap last changed,
	// against the pixels it changed by.  A slow average, and it never jumps - the
	// old form took the overshoot case whole, so one frame of a target drifting
	// away priced the mouse at a third of its value and the next step was three
	// times too big.  The step size is the aim's speed, so a step size that follows
	// every reading is a speed that wanders, and a speed that wanders is the tremor.
	{
		auto learn = [](float& k, float& previous, int& agrees, float moved, LONG counts)
		{
			const float n = (float)counts;
			if (std::fabs(n) < 2.f) return;                  // one count is mostly rounding
			const float sample = moved / n;                   // signed: > 0 means it moved with us
			if (!std::isfinite(sample) || sample <= 0.f) return;
			if (sample < 0.3f || sample > 16.f) return;       // not this step's doing

			// Two readings in a row that agree are a measurement; one on its own
			// is often the target walking at the same time as the step (a player
			// moving away looks exactly like a smaller ratio).  Taking a single
			// reading whole is what made the step size wander from tick to tick,
			// and a step whose size changes is a shiver on the crosshair.
			const bool same = previous > 0.f && sample > previous * 0.8f && sample < previous * 1.25f;
			agrees   = same ? agrees + 1 : 0;
			previous = sample;

			float alpha = 0.10f;
			if (agrees >= 2 && std::fabs(sample - k) > k * 0.5f)
				alpha = 0.45f;   // a consistently different sensitivity: converge quickly

			k = std::clamp(k + (sample - k) * alpha, 0.5f, 16.f);
		};

		const float moved_total = std::max(std::fabs(delta_x - s_win_x), std::fabs(delta_y - s_win_y));
		if (moved_total >= 2.f)
		{
			learn(s_kx, s_kx_prev, s_kx_agree, s_win_x - delta_x, s_win_counts_x);
			learn(s_ky, s_ky_prev, s_ky_agree, s_win_y - delta_y, s_win_counts_y);

			s_win_x = delta_x;
			s_win_y = delta_y;
			s_win_counts_x = s_win_counts_y = 0;
		}
	}

	if (s_paced_ms && now_ms - s_paced_ms < interval_ms)
	{
		s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	// How long the last step was given to work.  A first step has no history and is
	// given one frame.  A pause is capped at two frames rather than at a tenth of a
	// second: a step is a share of the gap, and after a pause the share would have
	// been almost all of it - the aim snapped on the first frame after the key went
	// down, however smooth the slider was.
	const double dt_ms   = s_sent_ms ? std::min(now_ms - s_sent_ms, 33.0) : 1000.0 / 60.0;
	const double idle_ms = s_paced_ms ? std::min(now_ms - s_paced_ms, 50.0) : 4.0;
	s_paced_ms = now_ms;

	// What the game has applied since the last tick: the gap moving toward zero is
	// the debt being paid.  What it has not paid is forgotten slowly rather than
	// held forever, so a command the game dropped cannot pin the aim to nothing.
	const float moved_x = s_seen_x - delta_x;
	const float moved_y = s_seen_y - delta_y;
	if (moved_x > 0.f) s_owed_x = std::max(0.f, s_owed_x - moved_x);
	if (moved_y > 0.f) s_owed_y = std::max(0.f, s_owed_y - moved_y);
	s_seen_x = delta_x;
	s_seen_y = delta_y;

	// The debt is forgotten in a few frames rather than held for a fifth of a
	// second: a count the game dropped must not pin the aim, and an estimate of
	// pixels-per-count that is too high makes every command look unpaid.  The
	// leak is what stops that from stalling the approach.
	const float forget = (float)std::exp(-(float)idle_ms / 80.f);
	s_owed_x *= forget;
	s_owed_y *= forget;

	// A command whose sign is the opposite of the gap crossed over while it was in
	// flight and is dropped; the rest is capped at the gap it belongs to - and to
	// about half of it, so even a badly wrong estimate cannot leave the aim with
	// almost no error left to act on.
	if (delta_x * s_owed_x < 0.f) s_owed_x = 0.f;
	if (delta_y * s_owed_y < 0.f) s_owed_y = 0.f;
	s_owed_x = std::copysign(std::min(std::fabs(s_owed_x), std::fabs(delta_x) * 0.55f), delta_x);
	s_owed_y = std::copysign(std::min(std::fabs(s_owed_y), std::fabs(delta_y) * 0.55f), delta_y);

	const float err_x = delta_x - s_owed_x;
	const float err_y = delta_y - s_owed_y;

	// ---- the damping ---------------------------------------------------------
	//
	// What the last send bought, as a fraction of itself: about 1 means it landed,
	// over 1 means it went past, below 0 means the error grew.  A sign flip with
	// nothing to show for it is a ring - the client's frame of delay has caught up
	// and the loop is chasing itself - so the next step is halved; a step that
	// landed walks the damping back up towards a full send.  This is the one part
	// that needs no pixels-per-count at all: whatever a count is worth, the damping
	// settles around it, and a loop that settles cannot ring.
	// Judged only when the game has actually answered - a step read on the tick
	// after it went out has not moved anything yet, and reading that as "the step
	// bought nothing" would teach the damping to sit at its floor.
	if (s_prev_step > 0.5f && now_ms - s_prev_send_ms < 300.0 &&
		(std::fabs(moved_x) + std::fabs(moved_y)) >= 0.5f)
	{
		const float err_dist  = std::sqrt(err_x*err_x + err_y*err_y);
		const float prev_dist = std::sqrt(s_prev_err_x*s_prev_err_x + s_prev_err_y*s_prev_err_y);
		const float response  = (prev_dist - err_dist) / s_prev_step;
		const bool  flipped   = (err_x * s_prev_err_x + err_y * s_prev_err_y) < 0.f;

		if (flipped && err_dist > 8.f && response < 0.5f)
			s_damp = std::max(0.15f, s_damp * 0.5f);
		else if (!flipped && response > 0.45f)
			s_damp = std::min(1.f, s_damp * 1.15f);

		s_prev_step = 0.f;   // each send is judged once
	}
	else if (now_ms - s_prev_send_ms > 300.0)
	{
		s_damp = 0.55f;      // a new engagement: start from a plain proportion
	}

	// Commanded a little under what the estimate says.  An estimate that is a touch
	// low undershoots and takes one more tick; one that is a touch high overshoots
	// and has to come back - and coming back is the shake.
	const float kx = std::max(0.3f, s_kx * 1.10f);
	const float ky = std::max(0.3f, s_ky * 1.10f);

	// The debt is booked at the honest estimate, not at the padded one the step is
	// divided by.  Booking the padding too made every step owe 10% more than the
	// cursor could ever pay back (the payment is the movement actually seen), so the
	// phantom debt piled up until it was most of the gap: the aim decelerated early
	// and then trickled the last stretch one count at a time, which is exactly the
	// stall at the end of a smoothed approach.  The padding still does its job -
	// the step it commands is a little short - because only the conversion uses it.
	const float kx_book = std::max(0.3f, s_kx);
	const float ky_book = std::max(0.3f, s_ky);

	// How much of what is left this step takes.
	//
	// Instant lock takes all of it.  With smoothing on the slider is a time
	// constant and the time since the last count is what earns the step its share,
	// so the aim moves at the slider's speed rather than at the speed of the loop;
	// the style decides where in the approach that speed is spent.  The share is
	// read from the approach's own progress, so a long approach stays gentle
	// instead of collapsing into a lock a few frames in.
	float gain_x = s_damp, gain_y = s_damp;
	if (settings::aimbot::smoothing && !settings::aimbot::instant_lock)
	{
		// A new player, or a gap that grew, is a new approach.
		if (addr != s_approach_addr || !(s_start_gap > 0.f) || distance > s_start_gap * 1.4f + 6.f)
		{
			s_approach_addr = addr;
			s_start_gap = distance;
			s_start_x   = std::max(1.f, std::fabs(delta_x));
			s_start_y   = std::max(1.f, std::fabs(delta_y));
		}

		const float tx = smoothing::tau_ms(settings::aimbot::smoothingx);
		const float ty = smoothing::tau_ms(settings::aimbot::smoothingy);
		const float px = smoothing::progress_from(s_start_x, std::fabs(delta_x));
		const float py = smoothing::progress_from(s_start_y, std::fabs(delta_y));

		const float want_x = smoothing::step(settings::aimbot::smoothing_style, px, tx, dt_ms);
		const float want_y = smoothing::step(settings::aimbot::smoothing_style, py, ty, dt_ms);

		// Ease the share itself in.
		//
		// The exponential share above is what the slider asks for in the steady
		// state; applied straight from the first tick it starts the aim at full
		// speed, and that flick at the start of every engagement is the part that
		// read as "smooth until it isn't".  A short filter on the share ramps the
		// first steps up from rest the way a hand does.  It only adds the ramp
		// once per approach, and the tail is exponential already, so the aim still
		// lands as softly as before.
		const float ramp_ms = std::max(5.f, std::min(tx, ty) * 0.30f);
		const float ramp    = 1.f - (float)std::exp(-std::clamp(dt_ms, 1.0, 33.0) / ramp_ms);
		s_share_x += (want_x - s_share_x) * ramp;
		s_share_y += (want_y - s_share_y) * ramp;

		// One tick's share, not one frame's: at 250 ticks a second four of these
		// make a frame, and a step that took most of the gap in a single tick would
		// be a flick with a crawl behind it.
		constexpr float k_tick_share_max = 0.6f;
		gain_x = std::min(s_share_x, k_tick_share_max);
		gain_y = std::min(s_share_y, k_tick_share_max);
	}
	else
	{
		s_start_gap = 0.f;
		s_share_x = s_share_y = 1.f;
	}

	// Carried in pixels, not counts: the fraction of a count a tick is short
	// survives to the next one, so a slow plan is followed by a steady trickle of
	// counts rather than a whole count every few frames.
	s_carry_x = std::clamp(s_carry_x + err_x * gain_x, -std::fabs(err_x), std::fabs(err_x));
	s_carry_y = std::clamp(s_carry_y + err_y * gain_y, -std::fabs(err_y), std::fabs(err_y));

	LONG move_x = (LONG)std::lround(s_carry_x / kx);
	LONG move_y = (LONG)std::lround(s_carry_y / ky);

	// Never more than what is left: whatever the estimate says, one step lands on
	// the target or short of it, never past it.
	const LONG cap_x = (LONG)std::max(1.f, std::fabs(err_x) / kx + 0.5f);
	const LONG cap_y = (LONG)std::max(1.f, std::fabs(err_y) / ky + 0.5f);
	move_x = std::clamp(move_x, -cap_x, cap_x);
	move_y = std::clamp(move_y, -cap_y, cap_y);

	if (move_x == 0 && move_y == 0)
	{
		// Not worth a count yet: the carry keeps it for the next decision.
		s_mouse_idle.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	s_carry_x -= (float)move_x * kx;
	s_carry_y -= (float)move_y * ky;
	s_owed_x  += (float)move_x * kx_book;
	s_owed_y  += (float)move_y * ky_book;

	s_win_counts_x += move_x;
	s_win_counts_y += move_y;

	s_sent_ms = now_ms;

	s_prev_err_x  = err_x;
	s_prev_err_y  = err_y;
	s_prev_step   = std::sqrt((float)move_x * (float)move_x * kx * kx +
	                          (float)move_y * (float)move_y * ky * ky);
	s_prev_send_ms = now_ms;

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

	if (get_world_distance_to_entity(aimbot::player, local) == FLT_MAX)
	{
		s_mouse_no_local.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	float distance{};
	math::vector2 target_screen{};
	// The FOV circle decides who is picked, it is not a leash on the aim: the player the aim
	// is already on has to keep resolving their aim point even a little past its edge.
	// Stopping there read as a target switch - the aim went dead for those ticks, and the
	// next one picked whoever was closest instead.
	if (get_target_point_for_entity(aimbot::player, view, dims, FLT_MAX, distance, target_screen))
	{
		if (settings::aimbot::offset_enabled)
		{
			target_screen.x += settings::aimbot::offset_x;
			target_screen.y += settings::aimbot::offset_y;
		}
		mouse_aim(target_screen, aimbot::player.instance.address);
	}
	else
	{
		s_mouse_no_point.fetch_add(1, std::memory_order_relaxed);
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

static void publish_camera_target(const math::matrix3& rot, bool smoothing)
{
	std::lock_guard<std::mutex> lk(s_cam_mtx);
	s_cam_want      = true;
	s_cam_target_ok = true;
	s_cam_target    = rot;
	s_cam_smoothing = smoothing;

	// The sliders as time constants, in milliseconds; 0 is a lock.  The writer
	// turns them into a per-step share with the time that really passed, so they
	// mean the same thing here as they do on the mouse path.
	s_cam_tx = smoothing ? smoothing::tau_ms(settings::aimbot::smoothingx) : 0.f;
	s_cam_ty = smoothing ? smoothing::tau_ms(settings::aimbot::smoothingy) : 0.f;
}

static void clear_camera_target()
{
	std::lock_guard<std::mutex> lk(s_cam_mtx);
	s_cam_want      = false;
	s_cam_target_ok = false;
}

static void camera_writer()
{
	// Where the current turn started, per axis.  The share a step takes is read
	// from how far along *this* turn is, so a long turn stays gentle instead of
	// collapsing into a lock after a few frames.
	float start_gap_yaw = 0.f, start_gap_pitch = 0.f;

	for (;;)
	{
		bool          want = false, target_ok = false, smooth = false;
		math::matrix3 target{};
		float         tx = 0.f, ty = 0.f;   // ms; 0 is a lock
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
			start_gap_yaw = start_gap_pitch = 0.f;
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
			float factor_x = 1.f, factor_y = 1.f;
			bool  write   = true;

			if (smooth)
			{
				// One step per frame budget, the cadence the mouse path works to.
				// This loop runs every millisecond while the game reads the camera
				// once a frame, so stepping between two of its frames bought
				// nothing and left the approach sized by the loop's tick rate
				// rather than by the smoothing sliders.
				const auto   now      = std::chrono::steady_clock::now();
				const double interval = (double)camera_step_interval_ms();
				// Capped at two frames: a pause is not paid for in one step that
				// is almost the whole turn, which is the same snap the mouse path
				// avoids.
				const double since    = s_cam_have_step
				                      ? std::min(std::chrono::duration<double, std::milli>(now - s_cam_step_at).count(), 33.0)
				                      : interval;

				if (s_cam_have_step && since < interval)
					write = false;

				if (write)
				{
					const math::matrix3 cur = memory->read<math::matrix3>(cam + Offsets::Camera::Rotation);

					float cur_yaw{}, cur_pitch{}, tgt_yaw{}, tgt_pitch{};
					rotation_to_yaw_pitch(cur, cur_yaw, cur_pitch);

					// Where the step is measured from.  The client rebuilds the
					// camera from the mouse and the character every frame, so a small
					// difference between its rotation and the one we wrote is
					// usually its own reconstruction rather than a move.  Reading
					// it back and correcting for the whole of it is what made a
					// smoothed aim shiver, because every step then carried a
					// correction for a rotation nobody asked for.  Here the client's
					// move is followed in proportion to its size: in full once it is
					// as large as a move, not at all while it is only noise, so the
					// approach stays monotone and a real move is never held against.
					if (s_cam_have_last)
					{
						float worst = 0.f;
						for (int i = 0; i < 9; ++i)
							worst = std::max(worst, std::fabs(cur.m[i] - s_cam_last_written.m[i]));

						const float w = std::min(1.f, worst / k_cam_readback_noise);
						if (w < 1.f)
						{
							float last_yaw{}, last_pitch{};
							rotation_to_yaw_pitch(s_cam_last_written, last_yaw, last_pitch);
							cur_yaw   = last_yaw   + wrap_pi(cur_yaw - last_yaw) * w;
							cur_pitch = last_pitch + (cur_pitch - last_pitch) * w;
						}
					}

					rotation_to_yaw_pitch(target, tgt_yaw, tgt_pitch);

					const float d_yaw   = wrap_pi(tgt_yaw - cur_yaw);
					const float d_pitch = tgt_pitch - cur_pitch;
					const float pitch_limit = 1.5533f;

					if (std::fabs(d_yaw) > pitch_limit || std::fabs(d_pitch) > pitch_limit)
					{
						// Most of the way around is a frame whose projection
						// flipped, not an aim: skipped, so the camera never takes a
						// quarter turn in one step.
						write = false;
					}
					else
					{
						// How much of the turn this step takes: the slider is a time
						// constant, the time since the last write earns the share, and
						// the style says where in the turn that share is spent.
						const float gap_yaw   = std::fabs(d_yaw);
						const float gap_pitch = std::fabs(d_pitch);

						// A gap that grew is a new turn, so the profile starts where
						// that turn actually started rather than resuming at the
						// careful end of the last one.
						if (!(start_gap_yaw > 1e-4f) || gap_yaw > start_gap_yaw * 1.4f + 0.002f)
							start_gap_yaw = std::max(gap_yaw, 1e-4f);
						if (!(start_gap_pitch > 1e-4f) || gap_pitch > start_gap_pitch * 1.4f + 0.002f)
							start_gap_pitch = std::max(gap_pitch, 1e-4f);

						const float px = smoothing::progress_from(start_gap_yaw, gap_yaw);
						const float py = smoothing::progress_from(start_gap_pitch, gap_pitch);

						const float step_x = smoothing::step(settings::aimbot::smoothing_style, px, tx, since);
						const float step_y = smoothing::step(settings::aimbot::smoothing_style, py, ty, since);

						const float new_pitch =
							std::max(-pitch_limit, std::min(pitch_limit, cur_pitch + d_pitch * step_y));
						out = rotation_from_yaw_pitch(cur_yaw + d_yaw * step_x, new_pitch);

						raw_yaw   = d_yaw;
						raw_pitch = d_pitch;
						step_yaw   = d_yaw   * step_x;
						step_pitch = d_pitch * step_y;
						factor_x   = step_x;
						factor_y   = step_y;
					}
				}
			}

			if (!write)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			memory->write<math::matrix3>(cam + Offsets::Camera::Rotation, out);

			{
				aimbot::telemetry_sample_t ts{};
				ts.t          = aimbot::telemetry_now();
				ts.raw_yaw    = raw_yaw;
				ts.raw_pitch  = raw_pitch;
				ts.step_yaw   = step_yaw;
				ts.step_pitch = step_pitch;
				ts.sx         = factor_x;
				ts.sy         = factor_y;

				ts.primary    = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
				aimbot::telemetry_push(ts);
			}

			s_cam_last_written = out;
			s_cam_have_last    = true;
			s_cam_step_at   = std::chrono::steady_clock::now();
			s_cam_have_step = true;
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

	// The smoothing's own lead.  The turn lags a walking target by its time
	// constant, so the camera is pointed where that target will be one time
	// constant from now - without it, a high slider is an aim that is always
	// a little behind, which reads as "barely moves" on anyone who strafes.
	target_pos = target_pos + smoothing_lead(aimbot::player);

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

// The cache thread rewrites the player vector, so every read of it is taken
// under the lock. Returns an empty entity when the address is not cached.
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

	// Whatever the aim held last frame, op1 rig or not. This used to be gated on
	// op1::active(), which left every hold below unreachable for a normal rig: the
	// selection was recomputed from scratch each frame, so two players within a few
	// pixels of each other traded the frame back and forth.
	const std::uint64_t previous = aimbot::player.instance.address;

	// How far the crosshair may leave the engaged target before another player can take
	// the frame. Tied to the FOV circle, floored so a tiny radius still allows a lock and
	// capped so a huge one cannot pin the aim forever.
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
		// The engaged target keeps a gate wider than the FOV circle: drifting onto its
		// edge used to drop its aim point, and a dropped point is a lost lock - one of
		// the ways the pick flickered between two players.
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
		// A lock survives a frame that cannot resolve the aim point - a part that
		// was not in the cache yet, a projection that failed. Two hundred ms of
		// grace, for every rig: the switch it used to cause was permanent, because
		// the next frame then held the new target.
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

	// The engaged target is held for every rig, not only the op1 one: the lock moves
	// when the crosshair has actually left it (a deliberate re-aim) or when it is gone.
	// Without this the pick was recomputed from scratch every frame, so two players a few
	// pixels apart traded the frame back and forth - that is the switching.
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

	const math::vector3 eye = worldgeo::eye_position(body);

	// Body-shaped, not one ray through the chest.  A target behind a crate
	// with their head out, or leaning out from a corner with one shoulder, is
	// a target; the single centre ray called every one of those hidden, which
	// is why a vis check this strict still felt like it was skipping people
	// who were plainly in the open.
	math::vector3 head{};
	bool have_head = false;
	{
		auto head_it = entity.parts.find("Head");
		if (head_it != entity.parts.end() && head_it->second.address)
		{
			try
			{
				rbx::primitive_t prim = head_it->second.get_primitive();
				if (prim.address)
				{
					head      = prim.get_position();
					have_head = math::finite(head);
				}
			}
			catch (...) { have_head = false; }
		}
	}

	if (have_head && (head - body).length() > 0.5f)
		return worldgeo::is_body_visible(eye, head, body);

	return worldgeo::is_visible(eye, body);
}

// The aimbot's FOV ring.
//
// This is the *only* place it is drawn.  render_t::render_visuals() used to
// draw the same circle from the same cursor a second time with its own widths
// (1.4 px colour over a 2.4 px dark band) on top of this one (1.6 px over a
// translucent rim), and two rings half a pixel apart is exactly what "the FOV
// circle is a bit thick" was: neither of them was thick, the pair was.
void aimbot::draw_fov(ImDrawList* dl, const math::vector2& dims)
{
	(void)dims;
	if (!settings::aimbot::draw_fov) return;

	const auto& fc = settings::aimbot::fov_circle_colour;
	const auto& oc = settings::aimbot::fov_outline_colour;

	POINT cursor{};
	if (!GetCursorPos(&cursor)) return;
	ImVec2 center((float)cursor.x, (float)cursor.y);
	const float radius = settings::aimbot::fov;

	ImU32 outline_col = IM_COL32((int)(oc[0]*255),(int)(oc[1]*255),(int)(oc[2]*255),(int)(oc[3]*255));
	ImU32 circle_col  = IM_COL32((int)(fc[0]*255),(int)(fc[1]*255),(int)(fc[2]*255),(int)(fc[3]*255));

	// Concentric bands, each on its own radius, and each built as a triangle
	// strip (ShapeDraw.h): the fill stops inside the colour, the colour is the
	// circle, and the outline sits just outside it rather than centred on it.
	// Half a dark line on top of the colour was one half of what read as thick;
	// a stroked polyline at this radius was the other, and its joins were the
	// grooves.
	const float th = std::max(0.5f, std::min(settings::aimbot::fov_thickness, 12.f));

	ImU32 fill_col = 0;
	float fill_alpha = 0.f;
	if (settings::aimbot::fov_fill)
	{
		const auto& fl = settings::aimbot::fov_fill_colour;
		fill_alpha = settings::aimbot::fov_fill_opacity * 2.55f;
		fill_col = IM_COL32((int)(fl[0]*255),(int)(fl[1]*255),(int)(fl[2]*255),(int)(fl[3]*fill_alpha));
	}

	// Dynamic has to know whether anything it can see is inside
	// the circle.  The aim's own target is the honest answer here: the ring
	// belongs to the aim, and a target it cannot see is not one it would take.
	bool hot = false;
	if (settings::aimbot::fov_style == 2 && player.instance.address != 0)
	{
		auto head = player.parts.find("Head");
		if (head == player.parts.end()) head = player.parts.find("HumanoidRootPart");
		if (head != player.parts.end() && head->second.address)
		{
			rbx::primitive_t prim = head->second.get_primitive();
			if (prim.address)
			{
				const math::vector3 world = memory->read<math::vector3>(
					prim.address + Offsets::Primitive::Position);
				hot = worldgeo::is_visible(worldgeo::eye_position(world), world);
			}
		}
	}

	shape::fov_circle(dl, center, radius, circle_col, outline_col, th,
		settings::aimbot::fov_outline, settings::aimbot::fov_style,
		settings::aimbot::fov_fill, fill_col, fill_alpha, hot);
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

		// Picking is the expensive half of this loop: the whole player list is
		// copied out of the cache and every candidate is projected part by part,
		// with a visibility ray for each one when that check is on.  The cache it
		// reads is rebuilt every 40 ms, so picking a thousand times a second
		// re-derives the same answer from the same numbers; holding one pick for a
		// frame budget costs the aim nothing it can see - a switch can only happen
		// when the cache changes anyway - and takes the largest single cost this
		// thread puts on a frame out of it.
		static std::uint64_t s_pick_ms   = 0;
		static std::uint64_t s_pick_addr = 0;
		static bool          s_pick_done = false;

		const std::uint64_t pick_now      = ::GetTickCount64();
		const std::uint64_t pick_interval = mouse_send_interval_ms();
		const bool          pick_held     = !pinned && s_pick_done &&
		                                    (pick_now - s_pick_ms) < pick_interval;

		if (pinned)
		{
			// The pin is the user naming a player outright, so it takes the frame:
			// nothing else is picked up while it is set, or the aim would slide onto
			// whoever is closest the moment the pin stepped behind cover.
			target = find_cached_player(pinned);

			if (target.instance.address && !is_target_valid(target))
				target = cache::entity_t{};
		}
		else if (pick_held)
		{
			// Between picks the answer stands; only its validity is re-checked,
			// which is a read rather than a projection.
			target = find_cached_player(s_pick_addr);

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

		if (!pick_held)
		{
			s_pick_done = true;
			s_pick_ms   = pick_now;
			s_pick_addr = target.instance.address;
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
					"%s key=%s | cand=%d locks=%d | rejected team=%d friend=%d dead=%d knock=%d hp=%d noaim=%d vis=%d | cam writes=%llu %s overwrite=%.3f | mouse moves=%llu already-on-target=%llu skip=no-local:%llu no-point:%llu pxpercount=%.2f/%.2f cursor=%ld,%ld focus=%s",
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
					(unsigned long long)s_mouse_no_local.exchange(0, std::memory_order_relaxed),
					(unsigned long long)s_mouse_no_point.exchange(0, std::memory_order_relaxed),
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
