#define NOMINMAX
#include "fly.h"

#include <game/game.h>
#include <cache/cache.h>
#include <sdk/math/math.h>
#include <sdk/sdk.h>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <settings.h>
#include <keybind/keybind.h>
#include <keybind/live_state.h>

#include <windows.h>
#include <thread>
#include <chrono>
#include <mutex>

static void fly_hack()
{
	if (game::datamodel.address == 0)
		return;

	std::uint64_t hrp_addr = 0;
	{
		std::lock_guard<std::mutex> lock(cache::mtx);
		if (cache::cached_local_player.instance.address == 0)
			return;
		auto root_it = cache::cached_local_player.parts.find("HumanoidRootPart");
		if (root_it != cache::cached_local_player.parts.end())
			hrp_addr = root_it->second.address;
	}
	if (!hrp_addr)
		return;

	std::uint64_t ws_addr = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
	if (!ws_addr) return;
	std::uint64_t camera_addr = memory->read<std::uint64_t>(ws_addr + Offsets::Workspace::CurrentCamera);
	if (!camera_addr) return;

	rbx::humanoid_t::write_gravity(0.0f);

	try
	{
		rbx::primitive_t prim = rbx::part_t{ hrp_addr }.get_primitive();
		if (prim.address == 0)
			return;

		math::matrix3 rotation = memory->read<math::matrix3>(camera_addr + Offsets::Camera::Rotation);

		math::vector3 move_direction(0.0f, 0.0f, 0.0f);
		bool is_moving = false;

		if (GetAsyncKeyState('W') & 0x8000) { move_direction.z -= 1.0f; is_moving = true; }
		if (GetAsyncKeyState('S') & 0x8000) { move_direction.z += 1.0f; is_moving = true; }
		if (GetAsyncKeyState('A') & 0x8000) { move_direction.x -= 1.0f; is_moving = true; }
		if (GetAsyncKeyState('D') & 0x8000) { move_direction.x += 1.0f; is_moving = true; }
		if (GetAsyncKeyState(VK_SPACE)    & 0x8000) { move_direction.y += 1.0f; is_moving = true; }
		if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) { move_direction.y -= 1.0f; is_moving = true; }

		if (!is_moving)
		{
			math::vector3 cur_pos = prim.get_position();

			if (settings::movement::flyhack::mode == 3)
			{
				// Glide: keep whatever momentum the last step left and
				// bleed it off.  Unlike the position modes this never pins the
				// character, so collisions still resolve and the camera keeps
				// its own smoothing.
				math::vector3 vel = memory->read<math::vector3>(
					prim.address + Offsets::Primitive::AssemblyLinearVelocity);
				vel.y = 0.f;
				vel = vel * 0.90f;
				for (int i = 0; i < 1000; i++)
				{
					rbx::humanoid_t::write_gravity(0.0f);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, vel);
				}
				return;
			}

			if (settings::movement::flyhack::mode == 0)
			{
				try {
					std::uint64_t ws = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
					if (ws) {
						std::uint64_t world = memory->read<std::uint64_t>(ws + Offsets::Workspace::World);
						if (world) memory->write<float>(world + Offsets::World::Gravity, 0.0f);
						memory->write<float>(ws + Offsets::Workspace::ReadOnlyGravity, 0.0f);
					}
				} catch (...) {}
				for (int i = 0; i < 10000; i++) {
					rbx::humanoid_t::write_gravity(0.0f);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, cur_pos);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
				}
			}
			else
			{
				for (int i = 0; i < 10000; i++) {
					memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, cur_pos);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
				}
			}
			return;
		}

		if (move_direction.length() > 0.0f)
		{
			move_direction = move_direction.normalized();
			math::vector3 rotated = rotation * move_direction;
			math::vector3 cur_pos = prim.get_position();
			math::matrix3 cur_rot = prim.get_rotation();

			if (settings::movement::flyhack::mode == 0)
			{
				try {
					std::uint64_t ws = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
					if (ws) {
						std::uint64_t world = memory->read<std::uint64_t>(ws + Offsets::Workspace::World);
						if (world) memory->write<float>(world + Offsets::World::Gravity, 0.0f);
						memory->write<float>(ws + Offsets::Workspace::ReadOnlyGravity, 0.0f);
					}
				} catch (...) {}
				math::vector3 vel = rotated * settings::movement::flyhack::speed;
				for (int i = 0; i < 10000; i++) {
					rbx::humanoid_t::write_gravity(0.0f);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, vel);
				}
			}
			else if (settings::movement::flyhack::mode == 1)
			{
				math::vector3 new_pos = cur_pos + (rotated * (settings::movement::flyhack::speed / 165.0f));
				for (int i = 0; i < 10000; i++) {
					memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, new_pos);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
				}
			}
			else if (settings::movement::flyhack::mode == 2)
			{
				math::vector3 new_pos = cur_pos + (rotated * (settings::movement::flyhack::speed / 165.0f));
				for (int i = 0; i < 10000; i++) {
					memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, new_pos);
					memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, cur_rot);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
				}
			}
			else if (settings::movement::flyhack::mode == 3)
			{
				math::vector3 vel = rotated * settings::movement::flyhack::speed;
				for (int i = 0; i < 1000; i++)
				{
					rbx::humanoid_t::write_gravity(0.0f);
					memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, vel);
				}
			}
		}
	}
	catch (...) {}
}

void movement::fly::run()
{
	keybind kb("flyhack");
	static bool was_active = false;

	for (;;)
	{

		std::this_thread::sleep_for(std::chrono::milliseconds(4));

		kb.key  = settings::movement::flyhack::keybind;
		kb.type = static_cast<keybind::c_keybind_type>(settings::movement::flyhack::activation_mode);
		kb.update();

		bool active = settings::movement::flyhack::enabled &&
			(settings::movement::flyhack::keybind == 0 || kb.enabled);

		live_state::set(live_state::kFly, active);

		if (active)
		{
			rbx::humanoid_t::write_gravity(0.0f);
			fly_hack();
			was_active = true;
		}
		else if (was_active)
		{
			rbx::humanoid_t::write_gravity(196.2f);
			try {
				std::lock_guard<std::mutex> lock(cache::mtx);
				auto it = cache::cached_local_player.parts.find("HumanoidRootPart");
				if (it != cache::cached_local_player.parts.end() && it->second.address) {
					rbx::primitive_t prim = it->second.get_primitive();
					if (prim.address)
						memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
				}
			} catch (...) {}
			was_active = false;
		}
	}
}
