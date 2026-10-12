#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "SkyVideo.h"

#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstring>

namespace SkyVideo
{
namespace
{

	struct pixel_t { std::uint8_t b, g, r, a; };

	pixel_t g_pixels[(std::size_t)k_w * (std::size_t)k_h];

	const char* const g_names[] = { "Aurora Borealis", "Deep Nebula", "Solar Storm" };
	constexpr int     k_count   = 3;

	int        g_clip    = 0;
	bool       g_playing = false;
	float      g_time    = 0.f;
	ULONGLONG  g_last    = 0;

	inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

	inline float hash2(int x, int y)
	{
		std::uint32_t n = (std::uint32_t)(x * 374761393) + (std::uint32_t)(y * 668265263);
		n = (n ^ (n >> 13)) * 1274126177u;
		return (float)((n ^ (n >> 16)) & 0xFFFFFFu) / 16777215.f;
	}

	inline float noise(float x, float y)
	{
		const int   xi = (int)std::floor(x), yi = (int)std::floor(y);
		const float xf = x - (float)xi,      yf = y - (float)yi;
		const float u  = xf * xf * (3.f - 2.f * xf);
		const float v  = yf * yf * (3.f - 2.f * yf);
		const float a = hash2(xi, yi),         b = hash2(xi + 1, yi);
		const float c = hash2(xi, yi + 1),     d = hash2(xi + 1, yi + 1);
		const float ab = a + (b - a) * u;
		const float cd = c + (d - c) * u;
		return ab + (cd - ab) * v;
	}

	inline float fbm(float x, float y)
	{
		return noise(x, y) * 0.62f + noise(x * 2.17f + 7.3f, y * 2.17f - 4.1f) * 0.38f;
	}

	inline void put(int x, int y, float r, float g, float b)
	{
		pixel_t& p = g_pixels[(std::size_t)y * (std::size_t)k_w + (std::size_t)x];
		p.r = (std::uint8_t)(clamp01(r) * 255.f + 0.5f);
		p.g = (std::uint8_t)(clamp01(g) * 255.f + 0.5f);
		p.b = (std::uint8_t)(clamp01(b) * 255.f + 0.5f);
		p.a = 255;
	}

	void advance_time()
	{
		const ULONGLONG now = GetTickCount64();
		if (!g_last)
			g_last = now;

		g_time += (float)((double)((now - g_last) % 1000ull) * 0.001);
		g_last = now;
		if (g_time > 3600.f)
			g_time -= 3600.f;
	}

	void render_aurora()
	{
		const float t = g_time;
		const float c0 = std::cos(t * 0.20f), s0 = std::sin(t * 0.20f);

		for (int y = 0; y < k_h; ++y)
		{
			const float vy = 1.f - (float)y / (float)k_h;
			for (int x = 0; x < k_w; ++x)
			{
				const float u = (float)x / (float)k_w;

				const float uu = u + 0.055f * std::sin(vy * 3.1f + t * 0.31f);

				float curtain = 0.f;
				for (int i = 0; i < 3; ++i)
				{
					const float fi     = (float)i;
					const float wave   = 0.5f * (std::sin(uu * (5.7f + fi * 2.9f) + t * (0.40f + fi * 0.11f)) * c0
					                           + std::cos(uu * (4.3f + fi * 2.1f) - t * (0.27f + fi * 0.07f)) * s0);
					const float centre = 0.60f + 0.11f * fi + wave * 0.10f;
					const float d      = (vy - centre) * 4.6f;
					curtain += std::exp(-d * d) * (0.56f - 0.14f * fi);
				}

				const float n    = fbm(uu * 4.6f, vy * 3.2f - t * 0.11f);
				const float glow = clamp01(curtain * (0.42f + 0.80f * n));

				float r = 0.030f + glow * 0.20f + glow * glow * 0.12f;
				float g = 0.050f + glow * 0.84f - glow * glow * 0.14f;
				float b = 0.110f + glow * 0.58f + glow * glow * 0.10f;

				if (n > 0.72f && glow < 0.25f)
				{
					const float s = hash2(x * 37 + 11, y * 61 + 5);
					if (s > 0.9972f)
					{
						const float tw = (s - 0.9972f) / 0.0028f * (0.62f + 0.38f * std::sin(t * 3.1f + (float)x));
						r += tw; g += tw; b += tw;
					}
				}
				put(x, y, r, g, b);
			}
		}
	}

	void render_nebula()
	{
		const float t = g_time;
		for (int y = 0; y < k_h; ++y)
		{
			const float vy = 1.f - (float)y / (float)k_h;
			for (int x = 0; x < k_w; ++x)
			{
				const float u = (float)x / (float)k_w;

				const float n1 = fbm(u * 3.1f + t * 0.021f,        vy * 3.1f - t * 0.014f);
				const float n2 = fbm(u * 5.7f - t * 0.032f + 3.3f, vy * 5.7f + t * 0.011f - 2.1f);

				const float cloud = clamp01(n1 * 1.30f - 0.30f) * (0.34f + 0.66f * n2);
				const float core  = cloud * cloud;

				float r = 0.035f + cloud * 0.44f + core * 0.18f;
				float g = 0.025f + cloud * 0.16f;
				float b = 0.100f + cloud * 0.56f + core * 0.20f;

				const float diff = std::fabs(n1 - n2);
				g += diff * 0.10f;
				b -= diff * 0.04f;

				if (n1 + n2 < 0.85f)
				{
					const float s = hash2(x * 53 + 7, y * 29 + 13);
					if (s > 0.9968f)
					{
						const float tw = (s - 0.9968f) / 0.0032f;
						r += tw; g += tw; b += tw * 1.06f;
					}
				}
				put(x, y, r, g, b);
			}
		}
	}

	void render_storm()
	{
		const float t = g_time;

		const float ph   = std::fmod(t * 0.55f, 1.f);
		const float f1   = ph < 0.05f ? (1.f - ph / 0.05f) : 0.f;
		const float f2p  = ph - 0.16f;
		const float f2   = (f2p > 0.f && f2p < 0.035f) ? (1.f - f2p / 0.035f) * 0.55f : 0.f;
		const float flash = f1 + f2;

		for (int y = 0; y < k_h; ++y)
		{
			const float vy = 1.f - (float)y / (float)k_h;

			const float sun = std::exp(-(vy - 0.16f) * (vy - 0.16f) * 5.2f);

			for (int x = 0; x < k_w; ++x)
			{
				const float u = (float)x / (float)k_w;

				const float band = fbm(u * 3.6f + t * 0.045f, vy * 2.4f - t * 0.020f);
				const float thin = fbm(u * 7.9f - t * 0.060f + 5.1f, vy * 6.3f + t * 0.030f);

				const float stratus = clamp01(band * 1.35f - 0.35f) * (0.45f + 0.55f * thin);

				float r = 0.070f + sun * 0.78f + stratus * 0.38f;
				float g = 0.035f + sun * 0.34f + stratus * 0.14f;
				float b = 0.040f + sun * 0.14f + stratus * 0.06f;

				r += flash * (0.55f + 0.45f * stratus);
				g += flash * (0.50f + 0.40f * stratus);
				b += flash * (0.46f + 0.34f * stratus);

				put(x, y, r, g, b);
			}
		}
	}
}

void Ensure()
{

}

std::vector<std::string> List()
{
	std::vector<std::string> out;
	out.reserve(k_count);
	for (int i = 0; i < k_count; ++i)
		out.emplace_back(g_names[i]);
	return out;
}

void Play(const std::string& name)
{
	if (name.empty())
	{
		g_playing = false;
		return;
	}
	for (int i = 0; i < k_count; ++i)
	{
		if (name == g_names[i])
		{
			g_clip    = i;
			g_playing = true;
			g_last    = 0;
			return;
		}
	}
	g_playing = false;
}

bool Frame(const void** px, int* w, int* h)
{
	if (px) *px = nullptr;
	if (w)  *w  = 0;
	if (h)  *h  = 0;
	if (!g_playing)
		return false;

	advance_time();

	switch (g_clip)
	{
	case 1:  render_nebula(); break;
	case 2:  render_storm();  break;
	default: render_aurora(); break;
	}

	if (px) *px = g_pixels;
	if (w)  *w  = k_w;
	if (h)  *h  = k_h;
	return true;
}
}
