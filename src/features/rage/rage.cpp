#define NOMINMAX
#include <Windows.h>
#include <thread>
#include <chrono>
#include <cmath>

#include <memory/memory.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <game/game.h>
#include <game/game_context.h>
#include <settings.h>

#include "rage.h"

static std::uint64_t get_local_hrp_primitive()
{
	if (!game::local_player.address) return 0;

	rbx::player_t lp(game::local_player.address);
	rbx::model_instance_t character = lp.get_model_instance();
	if (!character.address) return 0;

	rbx::instance_t hrp_inst = character.find_first_child("HumanoidRootPart");
	if (!hrp_inst.address) return 0;

	rbx::part_t hrp(hrp_inst.address);
	rbx::primitive_t prim = hrp.get_primitive();
	return prim.address;
}

static std::uint64_t get_local_humanoid()
{
	if (!game::local_player.address) return 0;

	rbx::player_t lp(game::local_player.address);
	rbx::model_instance_t character = lp.get_model_instance();
	if (!character.address) return 0;

	rbx::instance_t hum = character.find_first_child("Humanoid");
	return hum.address;
}

void rage::spinbot_run()
{
	bool was_enabled = false;
	static float angle = 0.f;

	while (true)
	{
		if (!settings::spinbot::enabled)
		{
			if (was_enabled)
			{
				std::uint64_t prim = get_local_hrp_primitive();
				std::uint64_t hum  = get_local_humanoid();

				if (prim)
				{
					math::matrix3 identity{};
					identity.m[0] = 1.f; identity.m[4] = 1.f; identity.m[8] = 1.f;
					memory->write<math::matrix3>(prim + Offsets::Primitive::Rotation, identity);
				}
				if (hum)
				{
					memory->write<std::uint8_t>(hum + Offsets::Humanoid::AutoRotate, 1);
				}
				was_enabled = false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		was_enabled = true;

		angle += 0.067f;
		if (angle > 6.2831853f)
			angle -= 6.2831853f;

		std::uint64_t prim = get_local_hrp_primitive();
		std::uint64_t hum  = get_local_humanoid();

		if (prim)
		{
			if (hum)
				memory->write<std::uint8_t>(hum + Offsets::Humanoid::AutoRotate, 0);

			float c = cosf(angle), s = sinf(angle);
			math::matrix3 rot_y{};
			rot_y.m[0] =  c; rot_y.m[1] = 0.f; rot_y.m[2] = -s;
			rot_y.m[3] = 0.f; rot_y.m[4] = 1.f; rot_y.m[5] = 0.f;
			rot_y.m[6] =  s; rot_y.m[7] = 0.f; rot_y.m[8] =  c;

			memory->write<math::matrix3>(prim + Offsets::Primitive::Rotation, rot_y);
		}
	}
}void rage::start_threads()
{
	std::thread(rage::spinbot_run).detach();
}
