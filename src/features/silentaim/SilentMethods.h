#pragma once

#include <settings.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>

namespace aether_silent
{
	inline float len3(const math::vector3& v)
	{
		return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	inline math::vector3 right_from_forward(const math::vector3& fwd)
	{
		const math::vector3 up{ 0.f, 1.f, 0.f };
		math::vector3 right{
			up.y * fwd.z - up.z * fwd.y,
			up.z * fwd.x - up.x * fwd.z,
			up.x * fwd.y - up.y * fwd.x
		};
		const float m = len3(right);
		if (m < 1e-6f)
			return right;
		return right * (1.f / m);
	}

	inline math::matrix3 matrix_from_forward(const math::vector3& fwd_in)
	{
		const float fm = len3(fwd_in);
		if (fm < 1e-6f)
			return math::matrix3::identity();
		const math::vector3 fwd = fwd_in * (1.f / fm);

		math::vector3 right = right_from_forward(fwd);
		const float rm = len3(right);
		if (rm < 1e-6f)
			return math::matrix3::identity();
		right = right * (1.f / rm);

		const math::vector3 up = fwd.cross(right);

		math::matrix3 mat{};
		mat.m[0] =  right.x; mat.m[3] =  right.y; mat.m[6] =  right.z;
		mat.m[1] =  up.x;    mat.m[4] =  up.y;    mat.m[7] =  up.z;
		mat.m[2] = -fwd.x;   mat.m[5] = -fwd.y;   mat.m[8] = -fwd.z;
		return mat;
	}

	inline math::matrix3 smooth_toward(const math::matrix3& current, const math::vector3& want_fwd, float smoothing)
	{
		float k = 1.f / (smoothing <= 1.f ? 1.f : smoothing);
		if (k < 0.02f)      k = 0.02f;
		else if (k > 1.f)   k = 1.f;

		math::vector3 cur_fwd = current.forward();
		const float m = len3(cur_fwd);
		if (m < 1e-6f)
			return matrix_from_forward(want_fwd);
		cur_fwd = cur_fwd * (1.f / m);

		math::vector3 look{
			cur_fwd.x + (want_fwd.x - cur_fwd.x) * k,
			cur_fwd.y + (want_fwd.y - cur_fwd.y) * k,
			cur_fwd.z + (want_fwd.z - cur_fwd.z) * k
		};
		if (len3(look) < 1e-6f)
			return matrix_from_forward(want_fwd);
		return matrix_from_forward(look);
	}

	struct CameraTarget
	{
		std::uintptr_t address = 0;
		bool           is_part = false;
		bool           valid   = false;
	};

	inline std::uintptr_t find_child_by_name(std::uintptr_t parent, const char* wanted)
	{
		rbx::instance_t inst{ parent };
		rbx::instance_t child = inst.find_first_child(wanted);
		return child.address;
	}

	inline math::vector3 read_part_position(std::uintptr_t part)
	{
		const std::uintptr_t prim = memory->read<std::uintptr_t>(part + Offsets::BasePart::Primitive);
		if (!prim)
			return math::vector3{};
		return memory->read<math::vector3>(prim + Offsets::Primitive::Position);
	}

	inline CameraTarget resolve_camera()
	{
		CameraTarget out{};
		if (!game::datamodel.address)
			return out;

		const std::uintptr_t workspace =
			memory->read<std::uintptr_t>(game::datamodel.address + Offsets::DataModel::Workspace);
		if (!workspace)
			return out;

		std::uintptr_t camera = find_child_by_name(workspace, "Camera");
		if (!camera)
			camera = memory->read<std::uintptr_t>(workspace + Offsets::Workspace::CurrentCamera);
		if (!camera)
			return out;

		const std::uintptr_t part = find_child_by_name(camera, "Part");
		if (part)
		{
			out.address = part;
			out.is_part = true;
			out.valid   = true;
			return out;
		}
		out.address = camera;
		out.is_part = false;
		out.valid   = true;
		return out;
	}

	inline bool read_rotation(const CameraTarget& cam, math::matrix3& out)
	{
		if (!cam.valid)
			return false;
		if (cam.is_part)
		{
			const std::uintptr_t prim = memory->read<std::uintptr_t>(cam.address + Offsets::BasePart::Primitive);
			if (!prim)
				return false;
			out = memory->read<math::matrix3>(prim + Offsets::Primitive::Rotation);
		}
		else
		{
			out = memory->read<math::matrix3>(cam.address + Offsets::Camera::Rotation);
		}
		return true;
	}

	inline void write_rotation(const CameraTarget& cam, const math::matrix3& rot)
	{
		if (!cam.valid)
			return;
		if (cam.is_part)
		{
			const std::uintptr_t prim = memory->read<std::uintptr_t>(cam.address + Offsets::BasePart::Primitive);
			if (!prim)
				return;
			memory->write<math::matrix3>(prim + Offsets::Primitive::Rotation, rot);
		}
		else
		{
			memory->write<math::matrix3>(cam.address + Offsets::Camera::Rotation, rot);
		}
	}

	inline math::vector3 camera_position(const CameraTarget& cam)
	{
		if (cam.is_part)
			return read_part_position(cam.address);
		return memory->read<math::vector3>(cam.address + Offsets::Camera::Position);
	}

	namespace pf
	{
		inline std::atomic<bool> g_active{ false };
		inline std::atomic<bool> g_run{ false };
		inline std::atomic<std::uintptr_t> g_addr{ 0 };
		inline std::atomic<bool> g_is_part{ false };
		inline math::matrix3     g_matrix{};
		inline bool              g_started = false;

		inline void ensure_writer()
		{
			if (g_started)
				return;
			g_started = true;
			g_run = true;
			std::thread([] {
				while (g_run.load())
				{
					if (g_active.load())
					{
						CameraTarget cam{};
						cam.address = g_addr.load();
						cam.is_part = g_is_part.load();
						cam.valid   = cam.address != 0;
						const math::matrix3 m = g_matrix;
						write_rotation(cam, m);
						if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
							write_rotation(cam, m);
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(
						(g_active.load() && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) ? 1 : 8));
				}
			}).detach();
		}

		inline void shutdown()
		{
			g_active = false;
			g_run    = false;
		}
	}

	inline bool apply(const CameraTarget& cam, const math::vector3& target, float smoothing)
	{
		if (!cam.valid)
			return false;

		const math::vector3 pos = camera_position(cam);
		if (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f)
			return false;

		math::vector3 want{ target.x - pos.x, target.y - pos.y, target.z - pos.z };
		if (len3(want) < 1e-6f)
			return false;
		want = want * (1.f / len3(want));

		math::matrix3 current{};
		const		math::matrix3 rot = read_rotation(cam, current)
			? smooth_toward(current, want, smoothing)
			: matrix_from_forward(want);

		write_rotation(cam, rot);
		return true;
	}
}
