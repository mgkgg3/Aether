#pragma once

#include <cstdint>

namespace esp
{
	void run();









	bool screen_anchor(std::uint64_t instance, float& cx, float& top);

	bool aim_anchor(std::uint64_t instance, float& cx, float& cy);

}
