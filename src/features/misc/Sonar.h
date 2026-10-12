#pragma once

#include <imgui/imgui.h>

#include <sdk/math/math.h>

#include <cstdint>

namespace sonar
{

	void draw(ImDrawList* dl, const math::vector2& dims, const math::matrix4& view);

	void reset();

	std::uint64_t latched();

	bool active();

	struct snapshot_t
	{
		bool  active = false;
		bool  has_ping = false;
		char  name[64]{};
		float distance = 0.f;
		float health = 0.f;
		float bearing_deg = 0.f;
		float sweep_deg = 0.f;
		float glow = 0.f;
	};

	snapshot_t snapshot();
}
