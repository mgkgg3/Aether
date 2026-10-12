#pragma once

#include <d3d11.h>

#include "imgui.h"

namespace boxfill
{

	inline constexpr int k_fill_image_slots = 1;

	void init(ID3D11Device* device);

	void shutdown();

	void set_remove_bg(bool remove_bg);

	bool load_path(const char* path);

	bool load_embedded(const unsigned char* data, unsigned int size);

	bool has_image();

	ID3D11ShaderResourceView* get(int id);

	void draw(ImDrawList* dl, int id, ImVec2 min, ImVec2 max, float alpha, bool no_plate = false);
}
