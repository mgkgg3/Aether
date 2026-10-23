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

	// The same question asked of several points at once: true if any of them
	// is clear.  It is one snapshot, one march per point, and it stops on the
	// first clear line.
	bool is_visible_any(const math::vector3& cam, const math::vector3* points, int count);

	// The body-shaped version: a short column from `head` to `root`, tried
	// head first, then chest, then hips, then either shoulder.  A player
	// behind a crate with their head out is visible; a single ray through the
	// middle of them is what calls that hidden, and that false negative is
	// what makes a vis check feel like it is ignoring half the players on the
	// map.  `head` and `root` are world positions.
	bool is_body_visible(const math::vector3& cam, const math::vector3& head,
	                     const math::vector3& root);

	// How far the world stays open in front of the camera, in studs, up to
	// max_radius. Weather sizes its volume with this so rain stops at the wall of
	// a tight space and spreads out over open ground without running away.
	float open_radius(const math::vector3& fallback_eye, float max_radius);

	int box_count();

	bool ground_y_at(float x, float z, float from_y, float& out_y);

	void force_reparse();
}
