#pragma once

#include <cstdint>
#include <atomic>

#include <sdk/math/math.h>

namespace aether_raycast
{
	struct Result
	{
		bool  visible = true;
		float coverage = 1.0f;
	};

	void start();
	void stop();
	void notify_teleport();

	Result is_player_visible(std::uintptr_t character_address,
		const math::vector3& camera_pos);

	namespace cfg
	{
		inline std::atomic<bool> enabled{ false };
		inline int   rays_per_target = 5;
		inline float max_distance = 2048.f;
	}
}
