#pragma once

// Clipping a world segment to what the camera can actually see.
//
// sdk's world_to_screen refuses a point at or behind the near plane (its
// clip.w test) and a point past its own NDC bound - and for a long-lived world
// line either end can be outside both.  A first-person muzzle sits on the near
// plane, and a tracer lives for over a second while the player keeps turning,
// so the point the shot landed on walks off the side of the view, or behind the
// player, long before the line has finished fading.  Asking the projection to
// agree about *both* ends drops the whole line the moment either one fails,
// which reads as a streak that flickers out mid-turn instead of sliding off the
// edge of the screen.
//
// So a segment is clipped first and projected afterwards.  Clipping a straight
// segment anywhere past the screen edge cannot change the pixels it covers -
// both parts project onto the same 2D line through the same camera - so the
// result is exactly the visible part of the path, and both ends then satisfy
// every bound world_to_screen tests, so it accepts them.

#include <sdk/math/math.h>

namespace viewclip
{
	struct segment_t
	{
		math::vector3 from{};
		math::vector3 to{};
		bool  visible{ false };

		// The far end was cut away, so there is no point on screen to mark at
		// the end of the line.
		bool  end_clipped{ true };
	};

	// Above the 0.1 world_to_screen tests for, so a clipped end is always
	// accepted by it.
	inline constexpr float k_near = 0.15f;

	// The side planes are in NDC: well inside the 60 the projection allows,
	// and several screens outside anything that can be on screen.
	inline constexpr float k_wide = 8.f;

	// Signed distance to one of the five planes in clip space; positive is
	// inside.  The view matrix is the world-to-clip matrix whose w the
	// projection divides by, so a point's depth is read straight out of it.
	inline float plane_distance(const math::matrix4& view, const math::vector3& p, int which)
	{
		const math::vector4 c = view.multiply({ p.x, p.y, p.z, 1.f });
		switch (which)
		{
		case 0:  return c.w - k_near;          // near plane
		case 1:  return k_wide * c.w - c.x;    // left
		case 2:  return k_wide * c.w + c.x;    // right
		case 3:  return k_wide * c.w - c.y;    // top
		default: return k_wide * c.w + c.y;    // bottom
		}
	}

	inline segment_t clip(const math::matrix4& view, const math::vector3& a, const math::vector3& b)
	{
		segment_t out;
		out.from = a;
		out.to   = b;
		out.end_clipped = false;

		// Every plane is an affine function of the world point (the matrix is
		// fixed), so one interpolation lands exactly on it - no iteration.
		for (int which = 0; which < 5; ++which)
		{
			const float d0 = plane_distance(view, out.from, which);
			const float d1 = plane_distance(view, out.to, which);

			if (d0 < 0.f && d1 < 0.f)
			{
				out.visible     = false;
				out.end_clipped = true;
				return out;
			}
			if (d0 >= 0.f && d1 >= 0.f)
				continue;

			const float t = d0 / (d0 - d1);
			const math::vector3 mid{
				out.from.x + (out.to.x - out.from.x) * t,
				out.from.y + (out.to.y - out.from.y) * t,
				out.from.z + (out.to.z - out.from.z) * t };

			if (d0 < 0.f) out.from = mid;                  // the muzzle went behind the eye
			else { out.to = mid; out.end_clipped = true; } // the impact did
		}

		out.visible = true;
		return out;
	}
}
