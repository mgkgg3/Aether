#define NOMINMAX
#include "rapidfire.h"
#include <game/game.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <settings.h>
#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <chrono>
#include <mutex>

#pragma comment(lib, "winmm.lib")

void rage::rapidfire::run()
{
	static LARGE_INTEGER frequency{};
	static LARGE_INTEGER lastTime{};
	static bool timeInitialized = false;

	while (true)
	{
		if (!settings::rage::rapidfire)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			continue;
		}

		rbx::instance_t local_character;
		{
			std::lock_guard<std::mutex> lock(cache::mtx);
			local_character = cache::cached_local_player.instance;
		}

		if (!local_character.address)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			continue;
		}

		rbx::instance_t backpack = local_character.find_first_child("Backpack");
		if (!backpack.address)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			continue;
		}

		for (rbx::instance_t tool : backpack.get_children())
		{
			if (tool.get_class_name() != "Tool" && tool.get_name() != "Combat") continue;

			rbx::instance_t shooting   = tool.find_first_child("ShootingCooldown");
			rbx::instance_t tolerance  = tool.find_first_child("ToleranceCooldown");

			if (shooting.address)   shooting.set_instance_double(0.0001);
			if (tolerance.address)  tolerance.set_instance_double(0.0001);
		}

		if (!timeInitialized)
		{
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&lastTime);
			timeBeginPeriod(1);
			timeInitialized = true;
		}

		constexpr double targetFrameTime = 1.0 / 800.0;

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		double elapsed = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)frequency.QuadPart;

		if (elapsed < targetFrameTime)
		{
			DWORD ms = (DWORD)((targetFrameTime - elapsed) * 1000.0);
			if (ms > 0) Sleep(ms);
		}

		do {
			QueryPerformanceCounter(&currentTime);
			elapsed = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)frequency.QuadPart;
		} while (elapsed < targetFrameTime);

		lastTime = currentTime;
		SwitchToThread();
	}
}
