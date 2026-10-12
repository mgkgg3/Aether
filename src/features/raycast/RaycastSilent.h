#pragma once

#include <cstdint>
#include <sdk/math/math.h>

namespace raycast_silent
{

	bool install();
	void remove();
	void ensure(bool want);

	void set_active(bool on, const math::vector3& world_target = {});

	bool ready();
	bool aiming();
}
