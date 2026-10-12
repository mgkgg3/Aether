#define NOMINMAX
#include "jumpcooldown.h"

#include <settings.h>
#include <game/game.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <thread>
#include <chrono>
#include <mutex>

void movement::jumpcooldown::run()
{
	for (;;)
	{
		if (!settings::nojumpcooldown::enabled)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		rbx::humanoid_t hum{ 0 };
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			hum = cache::cached_local_player.humanoid;
		}

		if (hum.address)
		{
			try
			{
				memory->write<bool> (hum.address + Offsets::Humanoid::UseJumpPower, true);
				float cur = memory->read<float>(hum.address + Offsets::Humanoid::JumpPower);
				if (cur < 1.f)
					memory->write<float>(hum.address + Offsets::Humanoid::JumpPower, 50.f);
			}
			catch (...) {}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}
}
