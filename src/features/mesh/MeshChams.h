#pragma once

#include "imgui.h"

#include <cstdint>
#include <unordered_map>

#include <sdk/math/math.h>
#include <sdk/sdk.h>

struct ID3D11RenderTargetView;

namespace mesh
{

	void chams_draw(ImDrawList* dl,
		const std::unordered_map<std::string, rbx::part_t>& parts,
		std::uint64_t key,
		const math::matrix4& view, const math::vector2& dims,
		ImU32 fill_col, bool full_detail);

	void chams_begin_frame(const math::matrix4& view, const math::vector2& dims,
		const math::vector3& camera);

	void chams_flush(ID3D11RenderTargetView* rtv);
}
