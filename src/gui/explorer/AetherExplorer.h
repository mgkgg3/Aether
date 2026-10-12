#pragma once

#include <imgui/imgui.h>

struct ID3D11Device;

namespace aether_explorer
{

	void init(ID3D11Device* device);
	void shutdown();

	void page(ImVec2 pos, ImVec2 size);
}
