#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define NOMINMAX

#include "SurfaceTex.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace surface
{
	namespace
	{
		ID3D11ShaderResourceView* g_radial = nullptr;
		ID3D11ShaderResourceView* g_ribbon = nullptr;
		ID3D11ShaderResourceView* g_topo_fine = nullptr;
		ID3D11ShaderResourceView* g_topo_coarse = nullptr;

		// The 8x8 ordered-dither matrix, 0..63.  A gradient that lands on whole
		// alpha values banded into visible rings once it was blended over the
		// frost; nudging each pixel by up to one level in this pattern turns a
		// ring into grain at a scale the eye reads as smooth.
		const int k_bayer[64] = {
			 0, 32,  8, 40,  2, 34, 10, 42,
			48, 16, 56, 24, 50, 18, 58, 26,
			12, 44,  4, 36, 14, 46,  6, 38,
			60, 28, 52, 20, 62, 30, 54, 22,
			 3, 35, 11, 43,  1, 33,  9, 41,
			51, 19, 59, 27, 49, 17, 57, 25,
			15, 47,  7, 39, 13, 45,  5, 37,
			63, 31, 55, 23, 61, 29, 53, 21,
		};

		float dither(int x, int y)
		{
			return ((float)k_bayer[((y & 7) << 3) | (x & 7)] + 0.5f) / 64.f; // 0..1
		}

		float smoothstep01(float t)
		{
			t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
			return t * t * (3.f - 2.f * t);
		}

		std::uint8_t alpha_byte(float a, int x, int y)
		{
			// One level of dither either way.
			float v = a * 255.f + (dither(x, y) - 0.5f) * 2.0f;
			if (v < 0.f) v = 0.f;
			if (v > 255.f) v = 255.f;
			return (std::uint8_t)(v + 0.5f);
		}

		ID3D11ShaderResourceView* upload(ID3D11Device* device, int w, int h,
			const std::vector<unsigned char>& rgba)
		{
			if (!device || w <= 0 || h <= 0 || rgba.size() < (std::size_t)w * (std::size_t)h * 4)
				return nullptr;

			D3D11_TEXTURE2D_DESC desc{};
			desc.Width            = (UINT)w;
			desc.Height           = (UINT)h;
			desc.MipLevels        = 1;
			desc.ArraySize        = 1;
			desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.Usage            = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem     = rgba.data();
			data.SysMemPitch = (UINT)w * 4;

			ID3D11Texture2D* texture = nullptr;
			if (FAILED(device->CreateTexture2D(&desc, &data, &texture)) || !texture)
				return nullptr;

			D3D11_SHADER_RESOURCE_VIEW_DESC view{};
			view.Format                        = desc.Format;
			view.ViewDimension                 = D3D11_SRV_DIMENSION_TEXTURE2D;
			view.Texture2D.MostDetailedMip     = 0;
			view.Texture2D.MipLevels           = 1;

			ID3D11ShaderResourceView* srv = nullptr;
			device->CreateShaderResourceView(texture, &view, &srv);
			texture->Release();
			return srv;
		}

		void radial_pixels(int n, std::vector<unsigned char>& px)
		{
			px.assign((std::size_t)n * (std::size_t)n * 4, 0);

			for (int y = 0; y < n; ++y)
			{
				for (int x = 0; x < n; ++x)
				{
					const float fx = ((float)x + 0.5f) / (float)n * 2.f - 1.f;
					const float fy = ((float)y + 0.5f) / (float)n * 2.f - 1.f;
					const float r  = std::sqrt(fx * fx + fy * fy);

					// A gaussian core with a hard-ish cut inside the rim: sampled
					// bilinearly and stretched, this is a light with no edge.
					float a = std::exp(-4.2f * r * r);
					a *= 1.f - smoothstep01((r - 0.72f) / 0.28f);
					if (r >= 1.f)
						a = 0.f;

					unsigned char* o = &px[((std::size_t)y * (std::size_t)n + (std::size_t)x) * 4];
					o[0] = o[1] = o[2] = 255;
					o[3] = alpha_byte(a, x, y);
				}
			}
		}

		// A band whose alpha is a bell across *and* down: the ribbon texture.  Its
		// edges are already zero, so the sampler's wrap never shows and a ribbon
		// can be laid down as a run of quads with no seam and no hard top edge.
		void ribbon_pixels(int w, int h, std::vector<unsigned char>& px)
		{
			px.assign((std::size_t)w * (std::size_t)h * 4, 0);

			for (int y = 0; y < h; ++y)
			{
				const float v  = ((float)y + 0.5f) / (float)h;
				const float sv = std::sin(3.14159265f * v);
				const float vb = std::pow(sv, 1.45f);

				for (int x = 0; x < w; ++x)
				{
					const float u  = ((float)x + 0.5f) / (float)w;
					const float su = std::sin(3.14159265f * u);
					const float hb = std::pow(su, 0.80f);

					unsigned char* o = &px[((std::size_t)y * (std::size_t)w + (std::size_t)x) * 4];
					o[0] = o[1] = o[2] = 255;
					o[3] = alpha_byte(hb * vb, x, y);
				}
			}
		}

		// A tileable height field from integer-frequency waves.  Integer
		// frequencies are what makes it tile: every wave is period-exact at the
		// texture edge, so a scrolled layer has nothing to line up.
		float height_at(int x, int y, int n)
		{
			static const int   fx[5]    = {  2, -1,  4,  3, -5 };
			static const int   fy[5]    = {  1,  3, -2,  3,  4 };
			static const float amp[5]   = { 0.42f, 0.26f, 0.16f, 0.10f, 0.06f };
			static const float phase[5] = { 0.7f, 2.1f, 4.3f, 1.2f, 5.0f };

			const float u = (float)x / (float)n;
			const float v = (float)y / (float)n;

			float h = 0.f;
			for (int i = 0; i < 5; ++i)
				h += amp[i] * std::sin(6.28318531f * ((float)fx[i] * u + (float)fy[i] * v) + phase[i]);

			// The high-frequency term is what gives the contours their tight,
			// map-like curls instead of a set of parallel bands.
			h += 0.055f * std::sin(6.28318531f * (7.f * u - 5.f * v) + 2.6f);
			h += 0.030f * std::sin(6.28318531f * (-3.f * u + 9.f * v) + 0.4f);

			return h * 0.5f + 0.5f;
		}

		void topo_pixels(int n, float levels, float half_width, std::vector<unsigned char>& px)
		{
			std::vector<float> field((std::size_t)n * (std::size_t)n);
			for (int y = 0; y < n; ++y)
				for (int x = 0; x < n; ++x)
					field[(std::size_t)y * (std::size_t)n + (std::size_t)x] = height_at(x, y, n);

			auto at = [&](int x, int y) -> float
			{
				x = ((x % n) + n) % n;
				y = ((y % n) + n) % n;
				return field[(std::size_t)y * (std::size_t)n + (std::size_t)x];
			};

			px.assign((std::size_t)n * (std::size_t)n * 4, 0);

			for (int y = 0; y < n; ++y)
			{
				for (int x = 0; x < n; ++x)
				{
					const float v = at(x, y) * levels;
					const float d = std::fabs(v - std::round(v));

					// Convert the distance in level-units into texels, so a line is
					// the same width where the contours crowd and where they spread.
					const float gx = (at(x + 1, y) - at(x - 1, y)) * 0.5f * levels;
					const float gy = (at(x, y + 1) - at(x, y - 1)) * 0.5f * levels;
					const float g  = std::sqrt(gx * gx + gy * gy);

					const float dpx = d / (g > 1e-5f ? g : 1e-5f);
					float a = 1.f - smoothstep01((dpx - half_width) / 1.35f);
					a *= 0.92f;

					unsigned char* o = &px[((std::size_t)y * (std::size_t)n + (std::size_t)x) * 4];
					o[0] = o[1] = o[2] = 255;
					o[3] = alpha_byte(a, x, y);
				}
			}
		}

		ID3D11ShaderResourceView* build_radial(ID3D11Device* device, int n)
		{
			std::vector<unsigned char> px;
			radial_pixels(n, px);
			return upload(device, n, n, px);
		}

		ID3D11ShaderResourceView* build_ribbon(ID3D11Device* device, int w, int h)
		{
			std::vector<unsigned char> px;
			ribbon_pixels(w, h, px);
			return upload(device, w, h, px);
		}

		ID3D11ShaderResourceView* build_topo(ID3D11Device* device, int n, float levels, float half_width)
		{
			std::vector<unsigned char> px;
			topo_pixels(n, levels, half_width, px);
			return upload(device, n, n, px);
		}
	}

	bool init(ID3D11Device* device)
	{
		if (!device)
			return false;
		if (ready())
			return true;

		g_radial      = build_radial(device, 256);
		g_ribbon      = build_ribbon(device, 256, 64);
		// Thin lines: the maps are stretched to well over a screen wide, so a wide
		// texel line would arrive as a band.  These are about two texels at the
		// bake and a few pixels on the panel, which is what a contour is.
		g_topo_coarse = build_topo(device, 512, 6.f, 0.60f);
		g_topo_fine   = build_topo(device, 512, 15.f, 0.38f);

		return ready();
	}

	void shutdown()
	{
		if (g_radial)      { g_radial->Release();      g_radial = nullptr; }
		if (g_ribbon)      { g_ribbon->Release();      g_ribbon = nullptr; }
		if (g_topo_fine)   { g_topo_fine->Release();   g_topo_fine = nullptr; }
		if (g_topo_coarse) { g_topo_coarse->Release(); g_topo_coarse = nullptr; }
	}

	bool ready()
	{
		return g_radial && g_ribbon && g_topo_fine && g_topo_coarse;
	}

	ImTextureID radial()      { return (ImTextureID)(intptr_t)g_radial; }
	ImTextureID ribbon()      { return (ImTextureID)(intptr_t)g_ribbon; }
	ImTextureID topo_fine()   { return (ImTextureID)(intptr_t)g_topo_fine; }
	ImTextureID topo_coarse() { return (ImTextureID)(intptr_t)g_topo_coarse; }
}
