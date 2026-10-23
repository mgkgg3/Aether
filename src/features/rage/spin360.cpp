#define NOMINMAX
#include "spin360.h"

#include <game/game.h>
#include <sdk/math/math.h>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <settings.h>
#include <keybind/keybind.h>
#include <keybind/live_state.h>

#include <windows.h>
#include <thread>
#include <chrono>
#include <cmath>

static math::matrix3 rotation_from_yaw_pitch(float yaw, float pitch)
{
	float cy = std::cos(yaw),  sy = std::sin(yaw);
	float cp = std::cos(pitch), sp = std::sin(pitch);

	math::vector3 fwd(-sy * cp, sp, -cy * cp);
	math::vector3 up(0.f, 1.f, 0.f);
	math::vector3 right = up.cross(fwd);
	float rlen = right.length();
	if (rlen < 0.0001f) {
		up = {0.f, 0.f, 1.f};
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

void rage::spin360::run()
{
	static float start_yaw   = 0.f;
	static float start_pitch = 0.f;
	static float cur_angle   = 0.f;
	static bool  spinning    = false;
	static bool  was_pressed = false;
	static math::matrix3 orig_rot{};
	const float TWO_PI = 6.28318530718f;

	while (true)
	{
		
		
		
		live_state::set(live_state::kSpin360, spinning);

		if (!settings::spin360::enabled || !game::datamodel.address) {
			if (spinning) {
				spinning = false; cur_angle = 0.f;
				start_yaw = 0.f; start_pitch = 0.f;
			}
			was_pressed = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		std::uint64_t camera_addr = 0;
		try {
			std::uint64_t ws = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
			if (ws) camera_addr = memory->read<std::uint64_t>(ws + Offsets::Workspace::CurrentCamera);
		} catch (...) {}

		if (!camera_addr) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		if (!spinning && settings::spin360::keybind != 0) {
			bool pressed = (GetAsyncKeyState(settings::spin360::keybind) & 0x8000) != 0;
			if (pressed && !was_pressed) {
				orig_rot = memory->read<math::matrix3>(camera_addr + Offsets::Camera::Rotation);
				math::vector3 fwd = orig_rot.forward();
				start_yaw   = std::atan2(-fwd.x, -fwd.z);
				start_pitch = std::asin(fwd.y);
				cur_angle   = 0.f;
				spinning    = true;
			}
			was_pressed = pressed;
		}

		if (!spinning) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		float spd_rad_per_ms = (settings::spin360::speed * TWO_PI) / 1000.f;
		static auto last_t = std::chrono::steady_clock::now();
		auto now_t  = std::chrono::steady_clock::now();
		long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_t - last_t).count();
		last_t = now_t;

		if (ms > 0) {
			cur_angle += spd_rad_per_ms * ms;
			if (cur_angle >= TWO_PI) {
				memory->write<math::matrix3>(camera_addr + Offsets::Camera::Rotation, orig_rot);
				cur_angle = 0.f; spinning = false;
				start_yaw = 0.f; start_pitch = 0.f; was_pressed = false;
			} else {
				float yaw = start_yaw + cur_angle;
				memory->write<math::matrix3>(camera_addr + Offsets::Camera::Rotation,
					rotation_from_yaw_pitch(yaw, start_pitch));
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
