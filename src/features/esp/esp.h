#pragma once

#include <cstdint>

namespace esp
{
	void run();

	
	
	
	
	
	
	
	
	bool screen_anchor(std::uint64_t instance, float& cx, float& top);

	// The point the aim is holding on that player - their head when they have
	// one. A line drawn to the HUD anchor above ends on the name and avatar
	// the ESP stacks over the box, a font height and a half above the player.
	bool aim_anchor(std::uint64_t instance, float& cx, float& cy);

}
