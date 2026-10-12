#pragma once

#include <d3d11.h>

#include "imgui.h"

namespace avatar
{

	void init(ID3D11Device* device);

	void shutdown();

	bool draw(ImDrawList* dl, long long user_id, ImVec2 min, ImVec2 max, ImU32 tint);

	bool draw_rounded(ImDrawList* dl, long long user_id, ImVec2 min, ImVec2 max, ImU32 tint, float rounding);

	bool draw_head(ImDrawList* dl, long long user_id, ImVec2 min, ImVec2 max, ImU32 tint, float rounding);
}
