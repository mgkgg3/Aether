#pragma once

#include <cstdint>

#include <imgui/imgui.h>
#include <sdk/math/math.h>

namespace effects
{

	void run();

	void draw(ImDrawList* draw, const math::vector2& dims, const math::matrix4& view);

	void play_hit_sound();

	bool own_hit_recent(std::uint64_t addr, float window_s);

	float own_hit_fade(std::uint64_t addr, float duration_s);

	void draw_crosshair_preview(ImDrawList* dl, const ImVec2& centre);

	void reset();
}
