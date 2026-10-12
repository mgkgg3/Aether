#include "tickrate.h"
#include <sdk/sdk.h>
#include <memory/memory.h>
#include <settings.h>
#include <thread>
#include <chrono>

namespace movement
{
	namespace tickrate
	{
		static constexpr float DEFAULT_TICKRATE = 240.f;
		static bool was_enabled = false;

		static void tickrate_thread()
		{
			using namespace std::chrono_literals;
			for (;;)
			{
				std::this_thread::sleep_for(50ms);

				if (!memory->get_module_address()) continue;

				if (settings::movement::tickrate::enabled)
				{
					float v = settings::movement::tickrate::value;
					if (v < 0.f)    v = 0.f;
					if (v > 1000.f) v = 1000.f;
					rbx::humanoid_t::write_tickrate(v);
					was_enabled = true;
				}
				else if (was_enabled)
				{
					rbx::humanoid_t::write_tickrate(DEFAULT_TICKRATE);
					was_enabled = false;
				}
			}
		}

		void run()
		{
			std::thread(tickrate_thread).detach();
		}
	}
}
