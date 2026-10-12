#pragma once

#include <cstdint>
#include <string>

#include <d3d11.h>

#include "MeshMath.h"

namespace mesh
{
	namespace dx
	{
		namespace shader
		{
			bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
			void Shutdown();
			void Resize(unsigned width, unsigned height);

			void SetOrigin(float ox, float oy);

			void BeginFrame(const Mesh::Matrix4x4& view, const Mesh::Vector3& camera, float time);

			void QueueMesh(const std::string& mesh_id, const Mesh::Matrix4x4& world);
			void QueueBox(const Mesh::Matrix4x4& world);

			void Flush(ID3D11RenderTargetView* rtv);

			bool IsFrameValid();

			bool PassAvailable();

			const char* StatusText(char* buffer, int buffer_size);

			const char* const* ModeNames();
			int ModeNameCount();
		}
	}
}
