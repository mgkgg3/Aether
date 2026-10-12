#pragma once

#include <imgui/imgui.h>

#include <sdk/math/math.h>

#include <cstdint>

namespace esp_trails
{

	void draw(ImDrawList* dl, const math::vector2& dims, const math::matrix4& view);

	void reset();

	int tracked();

	bool head_position(std::uint64_t instance, math::vector3& out);
}
