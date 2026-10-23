#define NOMINMAX
#include "speed.h"
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

// CFrame: the position is stepped by hand instead of handing the
// physics step a velocity.  A client that clamps velocity still accepts a
// position write, which is why this mode exists at all.
static math::vector3 s_cf_pos{};
static bool s_cf_init = false;
static std::uint64_t s_cf_character = 0;

static void speed_hack()
{
	if (cache::cached_local_player.instance.address == 0) return;

	auto root_it = cache::cached_local_player.parts.find("HumanoidRootPart");
	if (root_it == cache::cached_local_player.parts.end() || !root_it->second.address) return;

	rbx::primitive_t prim = root_it->second.get_primitive();
	if (!prim.address) return;

	std::uint64_t workspace = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
	if (!workspace) return;
	std::uint64_t camera = memory->read<std::uint64_t>(workspace + Offsets::Workspace::CurrentCamera);
	if (!camera) return;

	math::matrix3 rotation = memory->read<math::matrix3>(camera + Offsets::Camera::Rotation);

	math::vector3 dir{};
	if (GetAsyncKeyState('W') & 0x8000) dir.z -= 1.f;
	if (GetAsyncKeyState('S') & 0x8000) dir.z += 1.f;
	if (GetAsyncKeyState('A') & 0x8000) dir.x -= 1.f;
	if (GetAsyncKeyState('D') & 0x8000) dir.x += 1.f;

	if (dir.length() > 0.f)
	{
		dir = (rotation * dir.normalized());
		dir.y = 0.f;
		float len = dir.length();
		if (len > 0.f) dir = dir * (1.f / len);

		if (settings::movement::speedhack::mode == 0)
		{
			math::vector3 cur_vel = memory->read<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity);
			math::vector3 new_vel = dir * settings::movement::speedhack::speed;
			new_vel.y = cur_vel.y;
			for (int i = 0; i < 10000; i++)
				memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, new_vel);
		}
		else if (settings::movement::speedhack::mode == 1)
		{
			if (cache::cached_local_player.humanoid.address)
				for (int i = 0; i < 25000; i++)
				{
					memory->write<float>(cache::cached_local_player.humanoid.address + Offsets::Humanoid::Walkspeed,      settings::movement::speedhack::speed);
					memory->write<float>(cache::cached_local_player.humanoid.address + Offsets::Humanoid::WalkspeedCheck, settings::movement::speedhack::speed);
				}
		}
		else if (settings::movement::speedhack::mode == 2)
		{
			const std::uint64_t character = cache::cached_local_player.instance.address;
			if (!s_cf_init || s_cf_character != character)
			{
				s_cf_pos = prim.get_position();
				s_cf_character = character;
				s_cf_init = true;
			}

			s_cf_pos.y = prim.get_position().y;
			const float step = settings::movement::speedhack::speed / 165.f;
			s_cf_pos.x += dir.x * step;
			s_cf_pos.z += dir.z * step;

			const math::matrix3 rot = prim.get_rotation();
			for (int i = 0; i < 1000; i++)
			{
				memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, s_cf_pos);
				memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, rot);
				memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
			}
		}
	}
}

void movement::speed::run()
{
	keybind kb("speedhack");
	bool was_active = false;

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		kb.key  = settings::movement::speedhack::keybind;
		kb.type = static_cast<keybind::c_keybind_type>(settings::movement::speedhack::activation_mode);
		kb.update();

		bool active = settings::movement::speedhack::enabled &&
			(settings::movement::speedhack::keybind == 0 || kb.enabled);

		live_state::set(live_state::kSpeed, active);

		if (active)
		{
			speed_hack();
			was_active = true;
		}
		else if (was_active && settings::movement::speedhack::mode == 1)
		{
			if (cache::cached_local_player.humanoid.address)
				for (int i = 0; i < 100; i++)
				{
					memory->write<float>(cache::cached_local_player.humanoid.address + Offsets::Humanoid::Walkspeed,      16.f);
					memory->write<float>(cache::cached_local_player.humanoid.address + Offsets::Humanoid::WalkspeedCheck, 16.f);
				}
			was_active = false;
		}
		else
		{
			was_active = false;
			s_cf_init = false;
		}
	}
}
