#pragma once

#include <cstdint>

#include "sdk/math/math.h"

namespace worldgeo
{

	bool wants_data();

	void start();
	void stop();

	void invalidate();

	math::vector3 eye_position(const math::vector3& fallback);

	bool is_visible(const math::vector3& cam, const math::vector3& target);

	float open_radius(const math::vector3& fallback_eye, float max_radius);

	int box_count();

	bool ground_y_at(float x, float z, float from_y, float& out_y);

	void force_reparse();
}
