#pragma once

#include <imgui/imgui.h>
#include <sdk/math/math.h>

// The smaller feature helpers - antifling, jump power, hip height, freecam,
// orbit, headless, korblox, anti-AFK and anti-recoil - kept in one translation
// unit instead of a folder per feature.
//
// Their visual variants live in the feature that already draws the shape
// (esp.cpp, render.cpp, Effects.cpp, Radar.cpp) and are named in the dropdowns;
// this file is only the behaviour.
namespace fusion
{
	void run();

	// The frame-rate readout this module draws itself.  Called from the overlay pass
	// because the menu is not the only place it is wanted.
	void draw_overlay(ImDrawList* dl, const math::vector2& dims);
}
