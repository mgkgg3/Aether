#include "BoxFill.h"

#include <d3d11.h>

#include <cstdio>
#include <string>

#include <settings.h>

#include "../../../thirdparty/stb/stb_image.h"

namespace boxfill
{
	namespace
	{
		struct entry_t
		{
			ID3D11ShaderResourceView* srv = nullptr;
			int w = 0;
			int h = 0;
		};

		entry_t      g_tex[k_fill_image_slots]{};
		bool         g_ok = false;
		bool         g_remove_bg = false;
		ID3D11Device* g_device = nullptr;

		bool upload(ID3D11Device* device, const unsigned char* pixels, int w, int h, entry_t& out)
		{
			if (!device || !pixels || w <= 0 || h <= 0)
				return false;

			D3D11_TEXTURE2D_DESC desc{};
			desc.Width = (UINT)w;
			desc.Height = (UINT)h;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			D3D11_SUBRESOURCE_DATA sub{};
			sub.pSysMem = pixels;
			sub.SysMemPitch = (UINT)(w * 4);

			ID3D11Texture2D* tex = nullptr;
			HRESULT hr = device->CreateTexture2D(&desc, &sub, &tex);
			if (FAILED(hr) || !tex)
				return false;

			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = 1;

			ID3D11ShaderResourceView* srv = nullptr;
			hr = device->CreateShaderResourceView(tex, &srv_desc, &srv);
			tex->Release();
			if (FAILED(hr) || !srv)
				return false;

			if (out.srv) out.srv->Release();
			out.srv = srv;
			out.w = w;
			out.h = h;
			return true;
		}

		void key_out_white(unsigned char* pixels, int w, int h)
		{
			const int count = w * h;
			for (int i = 0; i < count; ++i)
			{
				unsigned char* p = pixels + i * 4;
				if (p[0] > 240 && p[1] > 240 && p[2] > 240)
					p[3] = 0;
			}
		}

		bool stage(ID3D11Device* device, unsigned char* pixels, int w, int h)
		{
			if (!device || !pixels || w <= 0 || h <= 0)
				return false;
			if (g_remove_bg)
				key_out_white(pixels, w, h);
			return upload(device, pixels, w, h, g_tex[0]);
		}
	}

	bool load_embedded(const unsigned char* data, unsigned int size)
	{
		if (!g_device || !data || size == 0)
			return false;

		int w = 0, h = 0, channels = 0;
		unsigned char* pixels = stbi_load_from_memory(data, (int)size, &w, &h, &channels, 4);
		if (!pixels)
			return false;

		const bool ok = stage(g_device, pixels, w, h);
		stbi_image_free(pixels);
		return ok;
	}

	bool load_path(const char* path)
	{
		if (!g_device || !path || !path[0])
			return false;

		int w = 0, h = 0, channels = 0;
		unsigned char* pixels = stbi_load(path, &w, &h, &channels, 4);
		if (!pixels)
			return false;

		const bool ok = stage(g_device, pixels, w, h);
		stbi_image_free(pixels);
		return ok;
	}

	void init(ID3D11Device* device)
	{
		if (!device)
			return;

		g_device = device;
		g_ok = true;

		settings::visuals::box_fill_image_loaded = false;
		if (settings::visuals::box_fill_image_path[0])
			settings::visuals::box_fill_image_loaded = load_path(settings::visuals::box_fill_image_path);
	}

	void shutdown()
	{
		for (int i = 0; i < k_fill_image_slots; ++i)
		{
			if (g_tex[i].srv)
			{
				g_tex[i].srv->Release();
				g_tex[i].srv = nullptr;
			}
			g_tex[i].w = 0;
			g_tex[i].h = 0;
		}

		g_ok = false;
		g_device = nullptr;
	}

	void set_remove_bg(bool remove_bg)
	{
		if (g_remove_bg == remove_bg)
			return;

		g_remove_bg = remove_bg;

		const char* path = settings::visuals::box_fill_image_path;
		if (g_device && path[0])
			load_path(path);
	}

	bool has_image()
	{
		return g_tex[0].srv != nullptr;
	}

	ID3D11ShaderResourceView* get(int id)
	{
		if (id < 0 || id >= k_fill_image_slots)
			return nullptr;
		return g_tex[id].srv;
	}

	void draw(ImDrawList* dl, int id, ImVec2 min, ImVec2 max, float alpha, bool no_plate)
	{
		if (!dl)
			return;

		ID3D11ShaderResourceView* srv = get(id);
		if (!srv)
			return;

		if (alpha < 0.f) alpha = 0.f;
		if (alpha > 1.f) alpha = 1.f;

		const int a8 = (int)(alpha * 255.f);

		if (!no_plate && !g_remove_bg)
			dl->AddRectFilled(min, max, IM_COL32(255, 255, 255, a8));

		const ImU32 col = IM_COL32(255, 255, 255, a8);
		dl->AddImage((ImTextureID)(intptr_t)srv, min, max, ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), col);
	}
}
