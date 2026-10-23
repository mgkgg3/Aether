#pragma once

#include <cstdint>
#include <sdk/math/math.h>

namespace raycast_silent
{
	// Redirects shots onto a target by swapping the direction the engine
	// hands to WorldRoot::RaycastBound. Nothing is written to the camera, so
	// the view never moves: only the bullet is bent.

	bool install();
	void remove();
	void ensure(bool want);

	void set_active(bool on, const math::vector3& world_target = {});

	bool ready();
	bool aiming();
}
