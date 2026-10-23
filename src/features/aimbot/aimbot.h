#pragma once
#include <cache/cache.h>
#include <unordered_map>
#include <sdk/math/math.h>
#include <imgui/imgui.h>

namespace aimbot
{
	inline cache::entity_t player{};
	inline cache::entity_t sticky_target{};
	inline std::unordered_map<std::uint64_t, math::vector3> previous_positions{};

	void run();
	void draw_fov(ImDrawList* dl, const math::vector2& dims);

	
	
	
	void release_camera();
}
