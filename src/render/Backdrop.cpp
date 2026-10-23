#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define NOMINMAX
#include "Backdrop.h"

// The legacy DirectX SDK headers sit first on this project's include path (it
// is where D3DX11 lives) and its DXGI.h shares an include guard with the modern
// one, so the modern <dxgi1_2.h> parses without DXGI_RGBA - that typedef lives
// in the modern dxgitype.h, which the guard skips.  The colour struct is all
// the 1.2 header is missing here.
struct DXGI_RGBA { float r, g, b, a; };
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#pragma comment(lib, "d3dcompiler.lib")

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include <debug/DebugConsole.h>

// Why this is written the way it is (the previous version flickered):
//
//  * The capture is change driven.  AcquireNextFrame returns WAIT_TIMEOUT on a
//    frame with nothing new in it, and treating that as "no image" is what made
//    the glass blink.  A timeout now means "hold what you have".
//  * The produced image is kept for the whole life of the module.  Only the
//    pixels in it are replaced, never the texture pointer the menu holds.
//  * Nothing here is torn down on a transient error.  The duplication is only
//    re-created on DXGI_ERROR_ACCESS_LOST or a size change, and at most once a
//    second, so a display transition cannot turn into a recreate-per-frame
//    churn.
//
// The blur itself runs on a small BGRA buffer: the desktop frame is copied to
// one intermediate texture, down-scaled to a few hundred pixels wide on the
// GPU, boxed three times and mixed with a little veil for the frost.  The menu
// samples the small texture with bilinear filtering, and that upscale is the
// blur people actually see.
//
// Why the down-scale is a GPU pass: this used to area-average the whole
// captured block on the CPU, which meant reading the block back at 1:1 - a
// megapixel of map, memcpy and pixel loop every captured frame.  That is what
// made the panel's background the one thing on screen whose frame rate was
// visibly not the overlay's: the capture could only afford to run a dozen times
// a second.  Now the GPU samples the block straight down to the small size, the
// readback is a few hundred kilobytes, and the capture can follow the frame
// rate.
namespace backdrop
{
	namespace
	{
		using Microsoft::WRL::ComPtr;

		// Two numbers set the whole look, and they are deliberately separate:
		//
		//  * the image divisor is how much the desktop is thrown away before any
		//    blur runs - it is what makes the upscale soft, and it is what makes
		//    the pass cost nothing at the top end;
		//  * the radius is the blur the slider actually asks for, in *screen*
		//    pixels, converted into image pixels by the divisor.
		//
		// Keeping the slider in screen pixels is the fix for the bottom end: the
		// radius is what the slider asks for, whatever the image beneath it is
		// scaled by.  The bottom of the slider is still the cheap end - one 1-2 px
		// pass at the coarsest image the ceiling allows - but it is no longer the
		// *expensive* end, which is what a 1:1 image made it.
		// A ceiling on the image itself, in pixels, whatever the slider says:
		// 640x360, a third of a 1080p frame.
		//
		// Two things decide it, and the second is why it is this small.  The
		// image is the source of a blur that sits behind a translucent panel,
		// so the effect cannot show much more than this.  And the whole of what
		// a captured frame costs is proportional to its pixels - every one is
		// mapped, blurred and tinted - so this is also the ceiling on the cost,
		// and therefore on how often the capture can afford to run.
		//
		// It used to be 960x540 and then 800x450, and both of those left the
		// *light* end of the slider the expensive one, which is the wrong way
		// round: at 800x450 with the divisor the low end asks for, one captured
		// frame cost several milliseconds of the overlay's own thread.  Past the
		// heavy mark the capture thins itself out, and an image that is
		// refreshed every frame at 640x360 trails the panel far less than one
		// refreshed every other frame at 800x450 - so the smaller image is the
		// lower *latency*, not a cheaper one at the same lag.  Nothing above
		// the middle of the slider changes at all: there the divisor has
		// already taken the image below this.
		constexpr long long k_max_read_pixels = 640 * 360;

		// How many frames the split readback waits for its copy before it is
		// allowed to wait on it properly.  A driver that never reports a staging
		// copy as done would otherwise leave the panel with no image at all, and
		// one stall is a better failure than a permanently blank glass.
		constexpr int k_read_wait_limit = 8;

		constexpr int k_min_w = 220;
		constexpr int k_upload_max_w = 1920;    // 1:1 on a 1080p/1440p display; wider ones scale mildly
		constexpr float k_max_screen_radius = 40.f;   // blur 100, in desktop pixels

		struct state_t
		{
			ComPtr<ID3D11Device> device;
			ComPtr<ID3D11DeviceContext> context;
			ComPtr<IDXGIOutputDuplication> dupl;

			ComPtr<ID3D11Texture2D> mid;          // GPU copy of the block, one tick behind
			bool mid_pending = false;

			// Split readback.  Reading the block the frame *after* it was copied is
			// one frame of visual latency for free, and one frame is exactly what
			// "the blur trails the panel" looks like - so the down-scale pass and
			// its staging copy are issued as soon as the block has been copied, and
			// the map that collects them runs on the next tick, where the copy has
			// landed and the map is a transfer rather than a wait.  `issue_ms` is
			// what issuing costs; the collecting half is in the blur's own cost.
			float issue_ms = 0.f;

			// The readback, as two steps a frame apart: the block is sampled down
			// and copied into `staging_small` on one tick, and mapped on the next,
			// so the map never waits on work issued in the same frame.
			bool small_pending = false;
			int  small_wait    = 0;
			ComPtr<ID3D11Texture2D> staging;      // CPU-readable copy of the captured block
			ComPtr<ID3D11Texture2D> image_tex;    // the small texture ImGui samples
			ComPtr<ID3D11ShaderResourceView> image_view;

			// The down-scale pass: the block (as a shader resource) sampled straight
			// into a texture the size of the published image, which is then the only
			// thing read back.  `staging_small` is that readback, and the shader and
			// sampler are built once.
			ComPtr<ID3D11VertexShader> scale_vs;
			ComPtr<ID3D11PixelShader>  scale_ps;
			ComPtr<ID3D11SamplerState> scale_smp;
			ComPtr<ID3D11Texture2D> scale_tex;
			ComPtr<ID3D11RenderTargetView> scale_rtv;
			ComPtr<ID3D11ShaderResourceView> block_srv;   // over `mid`, the block
			ComPtr<ID3D11Texture2D> staging_small;
			void*  block_srv_of = nullptr;   // the `mid` the SRV was made for
			int    scale_w = 0, scale_h = 0;
			bool   scale_ready = false;
			bool   scale_tried = false;

			int out_w = 0, out_h = 0;             // the whole output
			int cap_screen_x = 0, cap_screen_y = 0;   // the output's origin on the desktop

			// The captured block: a sub-rectangle of the output, in its pixels.  It
			// is the region of interest the caller asked for, grown to a coarse
			// grid and clamped to the output.  Only this block is ever copied,
			// mapped, blurred or uploaded - which is where the cost of a light
			// blur was hiding: at blur 0 the image is 1:1 with the desktop, and
			// reading the whole desktop at 1:1 cost more than 30 ms a frame.
			int cap_x = 0, cap_y = 0, cap_w = 0, cap_h = 0;

			// Requested region, in *client* pixels (the caller's coordinates).
			bool  have_req = false;
			float req_x = 0.f, req_y = 0.f, req_x1 = 0.f, req_y1 = 0.f;
			bool  cap_dirty = false;
			unsigned long long cap_at = 0;

			// The block the image actually covers.  g.desktop_w/h is that block's
			// size, so everything downstream (divisor, scale, uv) is unchanged.
			int desktop_w = 0, desktop_h = 0;
			int small_w = 0, small_h = 0;
			int origin_x = 0, origin_y = 0;
			float scale = 1.f;                    // desktop pixels per image pixel


			// One BGRA buffer at the current internal size (not "small":
			// rpcndr.h owns that word).
			std::vector<unsigned char> scaled;
			std::vector<unsigned char> scratch;

			// The vertical box pass's running column sum, one entry per channel
			// per column: what turns that pass from a column-by-column walk into
			// a linear one (see box_v).
			std::vector<unsigned int> colsum;

			float blur = 30.f, tint = 0.f;

			// Adaptive quality: one extra step of down-scale while a captured
			// frame measures heavy, given back once it has been cheap for a
			// while.  The image behind a blur is soft by definition, so the
			// step is invisible and it is the only way to keep a slow machine
			// from paying the same per-frame cost as a fast one.
			int auto_div = 0;
			unsigned long long quality_at = 0;
			float cheap_run = 0.f;

			// Percent of the output the capture may copy (see params_t).  0 is
			// the off switch, checked in wanted().
			float cap_pct = 100.f;

			// CPU time of the last frame's copy + blur + veil, in milliseconds.
			// Shown in the panel and in the status log, so "is it cheaper" is a
			// measurement rather than an opinion.
			float cpu_ms = 0.f;

			HWND overlay = nullptr;

			bool has_image = false;
			bool frame_held = false;
			bool logged_first = false;
			unsigned long long retry_at = 0;
			unsigned long long last_frame = 0;
			unsigned long long frames = 0;

			// Published geometry.  Written only in the same breath as a fresh
			// image, so a region change can never leave the menu sampling the old
			// block through the new mapping.
			void* pub_view = nullptr;
			int   pub_w = 0, pub_h = 0;
			float pub_origin_x = 0.f, pub_origin_y = 0.f, pub_scale = 1.f;

			char status[128] = "idle";
			const char* key = "idle";

			void note(const char* k, const char* text)
			{
				key = k;
				snprintf(status, sizeof status, "%s", text ? text : k);
			}
		};

		state_t g;

		unsigned long long now_ms() { return ::GetTickCount64(); }

		// 2..6, from the blur slider.  It used to reach 1 - a 1:1 desktop image,
		// with no upscale at all - and that one step was most of "the blur eats
		// the frame rate at low values": the image was a megapixel of desktop,
		// mapped, memcpy'd, blurred and tinted on every captured frame, to blur it
		// by a couple of pixels.  The bottom of the range is half the block now,
		// which a linear upsample behind the panel's own fill does not show, and
		// the curve above 2 is exactly what it was.
		int internal_divisor()
		{
			const float t = std::min(std::max(g.blur, 0.f), 100.f) / 100.f;
			const int base = std::min(6, std::max(2, (int)std::lround(1.f + 5.f * std::pow(t, 1.35f))));
			return std::min(10, base + g.auto_div);
		}

		// The blur the slider asks for, in screen pixels.  1 px at the very
		// bottom (visibly a hair), 40 px at 100 - quadratic, because a linear
		// ramp front-loads the blur into the part of the slider people use.
		float screen_radius()
		{
			const float t = std::min(std::max(g.blur, 0.f), 100.f) / 100.f;
			return 1.f + (k_max_screen_radius - 1.f) * t * t;
		}

		// Nothing at all to compute when both sliders are at the bottom: the
		// desktop behind the panel is already the sharp backdrop.  "Bottom" for
		// the blur is 2/100 - below that the pass would draw a single pixel.
		bool effect_wanted()
		{
			return g.blur > 2.f || g.tint > 0.5f;
		}

		// A hair of blur is worth one cheap pass; the deep end needs three to
		// look like a gaussian.  Fewer passes at the bottom is where the frame
		// time actually came from.
		int blur_passes(int radius)
		{
			if (radius <= 2)  return 1;
			if (radius <= 6)  return 2;
			return 3;
		}

		// Turn the caller's client-space request into the desktop-space block that
		// will actually be copied.  Returns false when there is nothing sensible
		// to restrict the capture to, in which case the whole output is used.
		bool compute_block()
		{
			if (!g.have_req || !g.overlay || g.out_w <= 0 || g.out_h <= 0)
				return false;

			// The overlay's client pixels and the output's pixels have to agree for
			// any of this to mean anything.  On a scaled display they do not, and a
			// wrong block is worse than a large one.
			RECT cr{};
			if (!::GetClientRect(g.overlay, &cr))
				return false;
			const int cw = (int)(cr.right - cr.left);
			const int ch = (int)(cr.bottom - cr.top);
			if (cw <= 0 || ch <= 0)
				return false;
			if (cw > g.out_w + 2 || ch > g.out_h + 2 || cw < g.out_w - 2 || ch < g.out_h - 2)
				return false;

			POINT org{};
			if (!::ClientToScreen(g.overlay, &org))
				return false;

			// Screen space -> output space.  The output's own origin comes from the
			// capture; add the client's offset inside the output.
			const int base_x = (int)org.x - g.cap_screen_x;
			const int base_y = (int)org.y - g.cap_screen_y;

			// The margin is a *movement* margin, not a bleed margin.  The menu can be
			// dragged, and the panel's rectangle walks across the desktop as it
			// moves: the block has to be wide enough that the panel stays inside
			// the image for as long as the drag lasts, or the panel samples past
			// the edge of the image and the blur reads as lagging behind the
			// panel.  It also buys the block time to be rebuilt - a re-acquire
			// costs a frame, and doing one every few pixels of movement is what
			// that lag actually was.
			// The margin is a share of the panel, not a constant: a wide panel is
			// dragged further in a frame than a narrow one, and the block has to
			// cover the whole drag or the panel samples past its edge.  40% of the
			// panel's own size on each side means a drag can travel most of the
			// panel's width before the block needs rebuilding at all.
			const int move_x = (int)std::max(180.f, (g.req_x1 - g.req_x) * 0.40f);
			const int move_y = (int)std::max(180.f, (g.req_y1 - g.req_y) * 0.40f);

			int x0 = base_x + (int)std::floor(g.req_x) - move_x;
			int y0 = base_y + (int)std::floor(g.req_y) - move_y;
			int x1 = base_x + (int)std::ceil(g.req_x1) + move_x;
			int y1 = base_y + (int)std::ceil(g.req_y1) + move_y;

			// Grow to a coarse grid so a dragged dock does not rebuild the capture
			// every frame, and clamp to the output.
			const int grid = 64;
			x0 = (x0 / grid) * grid;                 // rounds toward -inf, which is what growing wants
			y0 = (y0 / grid) * grid;
			x1 = ((x1 + grid - 1) / grid) * grid;
			y1 = ((y1 + grid - 1) / grid) * grid;

			if (x0 < 0) x0 = 0;
			if (y0 < 0) y0 = 0;
			if (x1 > g.out_w) x1 = g.out_w;
			if (y1 > g.out_h) y1 = g.out_h;

			// The cap: the block is never allowed to grow past this share of the
			// output, however large the panels ask for.  Shrunk around the middle
			// of the request rather than its top-left corner, so the rectangle the
			// panel samples stays inside the image and the frost covers what it is
			// asked to cover.  A cap of 100 leaves the block alone.
			if (g.cap_pct < 99.5f)
			{
				const int64_t cap_area =
					(int64_t)((double)g.out_w * (double)g.out_h * (double)g.cap_pct / 100.0);
				const int64_t have_area = (int64_t)(x1 - x0) * (int64_t)(y1 - y0);

				if (cap_area <= 0)
					return false;

				if (have_area > cap_area)
				{
					const double k = std::sqrt((double)cap_area / (double)have_area);
					const int cxm = (x0 + x1) / 2;
					const int cym = (y0 + y1) / 2;
					const int w2 = std::max(32, (int)((double)(x1 - x0) * k));
					const int h2 = std::max(32, (int)((double)(y1 - y0) * k));

					x0 = cxm - w2 / 2;
					y0 = cym - h2 / 2;
					x1 = x0 + w2;
					y1 = y0 + h2;

					// Back onto the grid, this time rounding inward, and back
					// inside the output.
					x0 = (x0 / grid) * grid;
					y0 = (y0 / grid) * grid;
					x1 = ((x1 / grid) + 1) * grid;
					y1 = ((y1 / grid) + 1) * grid;

					if (x0 < 0) { x1 -= x0; x0 = 0; }
					if (y0 < 0) { y1 -= y0; y0 = 0; }
					if (x1 > g.out_w) x1 = g.out_w;
					if (y1 > g.out_h) y1 = g.out_h;

					if (x1 - x0 < 32 || y1 - y0 < 32)
						return false;
				}
			}

			// A block that covers most of the output is not worth restricting to.
			if (x1 - x0 < 32 || y1 - y0 < 32) return false;
			if ((int64_t)(x1 - x0) * (int64_t)(y1 - y0) >= (int64_t)g.out_w * g.out_h * 3 / 4)
				return false;

			g.cap_x = x0;
			g.cap_y = y0;
			g.cap_w = x1 - x0;
			g.cap_h = y1 - y0;
			return true;
		}

		// True while the wanted block is inside the captured one; a region that
		// still fits costs nothing to keep.
		bool block_covers_request()
		{
			if (!g.have_req)
			{
				// A capped capture is a deliberate sub-rectangle, so it is never
				// "the whole output" - it has to be rebuilt when a region lands.
				if (g.cap_pct < 99.5f)
					return false;
				return g.cap_w > 0 && g.cap_x == 0 && g.cap_y == 0 &&
					g.cap_w == g.out_w && g.cap_h == g.out_h;
			}
			if (g.cap_w <= 0 || !g.overlay)
				return false;

			RECT cr{};
			POINT org{};
			if (!::GetClientRect(g.overlay, &cr) || !::ClientToScreen(g.overlay, &org))
				return false;
			if ((int)(cr.right - cr.left) != g.out_w || (int)(cr.bottom - cr.top) != g.out_h)
				return false;

			const int base_x = (int)org.x - g.cap_screen_x;
			const int base_y = (int)org.y - g.cap_screen_y;
			const int x0 = base_x + (int)std::floor(g.req_x);
			const int y0 = base_y + (int)std::floor(g.req_y);
			const int x1 = base_x + (int)std::ceil(g.req_x1);
			const int y1 = base_y + (int)std::ceil(g.req_y1);

			return x0 >= g.cap_x && y0 >= g.cap_y &&
				x1 <= g.cap_x + g.cap_w && y1 <= g.cap_y + g.cap_h;
		}

		// Resize the working buffers and the published texture to match the
		// slider.  Called from update(), so moving the slider never touches the
		// capture itself.
		void ensure_internal()
		{
			if (g.desktop_w <= 0)
				return;

			const int div = internal_divisor();
			int w = (int)std::lround((float)g.desktop_w / (float)div);
			if (w > k_upload_max_w) w = k_upload_max_w;
			if (w < k_min_w) w = k_min_w;
			if (w > g.desktop_w) w = g.desktop_w;

			int h = (int)std::lround((float)g.desktop_h * (float)w / (float)g.desktop_w);
			if (h < 1) h = 1;

			// And the ceiling on the image itself, whatever the slider says.
			// Nothing below this point is free - the map, the memcpy, every blur
			// pass and the tint all walk the whole image - so the image is capped
			// by area rather than trusted to the divisor alone.
			if ((long long)w * (long long)h > k_max_read_pixels)
			{
				const double k = std::sqrt((double)k_max_read_pixels / ((double)w * (double)h));
				w = (int)std::lround((double)w * k);
				h = (int)std::lround((double)h * k);
				if (w < 1) w = 1;
				if (h < 1) h = 1;
			}

			if (w == g.small_w && h == g.small_h)
				return;

			g.small_w = w;
			g.small_h = h;
			g.scale = (float)g.desktop_w / (float)w;

			g.scaled.assign((size_t)w * (size_t)h * 4, 0);
			g.scratch.assign((size_t)w * (size_t)h * 4, 0);
		}

		// ---- the down-scale pass --------------------------------------------
		//
		// The block is sampled straight down to the published size by the GPU.  The
		// shader is three lines and a full-screen triangle with no vertex buffer:
		// one sample per output pixel with a linear filter, which for a 4x
		// reduction is a 2x2 box - and three box passes run over it afterwards
		// anyway.  The point is not the filter quality, it is that the CPU stops
		// reading a megapixel of desktop per captured frame.
		const char* const k_scale_hlsl =
			"struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
			"VSOut vs_main(uint id : SV_VertexID)\n"
			"{\n"
			"    VSOut o;\n"
			"    float2 p = float2((float)((id << 1) & 2), (float)(id & 2));\n"
			"    o.uv = float2(p.x, 1.0 - p.y);\n"
			"    o.pos = float4(p.x * 2.0 - 1.0, 1.0 - p.y * 2.0, 0.0, 1.0);\n"
			"    return o;\n"
			"}\n"
			"Texture2D src : register(t0);\n"
			"SamplerState smp : register(s0);\n"
			"float4 ps_main(VSOut i) : SV_Target { return src.Sample(smp, i.uv); }\n";

		bool build_scale_pipeline()
		{
			if (g.scale_ready)
				return true;
			if (g.scale_tried || !g.device || !g.context)
				return false;

			g.scale_tried = true;

			ComPtr<ID3DBlob> vs_blob, ps_blob, err_blob;
			if (FAILED(D3DCompile(k_scale_hlsl, strlen(k_scale_hlsl), "backdrop_scale",
				nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vs_blob, &err_blob)) ||
				FAILED(D3DCompile(k_scale_hlsl, strlen(k_scale_hlsl), "backdrop_scale",
					nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &ps_blob, &err_blob)))
			{
				g.note("no-scale-shader", "the down-scale shader would not compile - using the CPU path");
				return false;
			}

			if (FAILED(g.device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
				nullptr, &g.scale_vs)) ||
				FAILED(g.device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
					nullptr, &g.scale_ps)))
			{
				g.note("no-scale-shader", "the down-scale shader could not be created");
				return false;
			}

			D3D11_SAMPLER_DESC smp{};
			smp.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			smp.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
			smp.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
			smp.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
			smp.ComparisonFunc = D3D11_COMPARISON_NEVER;
			smp.MaxLOD         = D3D11_FLOAT32_MAX;
			if (FAILED(g.device->CreateSamplerState(&smp, &g.scale_smp)))
			{
				g.note("no-scale-sampler", "the down-scale sampler could not be created");
				return false;
			}

			g.scale_ready = true;
			return true;
		}

		// Everything the pass needs for the current sizes: a view of the block, the
		// target it is sampled into, and a staging copy of that target.
		bool ensure_scale_targets()
		{
			if (!build_scale_pipeline())
				return false;
			if (g.small_w <= 0 || g.small_h <= 0 || !g.mid)
				return false;

			if ((void*)g.mid.Get() != g.block_srv_of || !g.block_srv)
			{
				g.block_srv.Reset();
				if (FAILED(g.device->CreateShaderResourceView(g.mid.Get(), nullptr, &g.block_srv)))
				{
					g.note("no-block-view", "the captured block could not be sampled");
					return false;
				}
				g.block_srv_of = (void*)g.mid.Get();
			}

			if (g.scale_tex && g.staging_small && g.scale_w == g.small_w && g.scale_h == g.small_h)
				return true;

			g.scale_tex.Reset();
			g.scale_rtv.Reset();
			g.staging_small.Reset();
			g.scale_w = g.scale_h = 0;
			g.small_pending = false;      // the copy belonged to the old size
			g.small_wait    = 0;

			D3D11_TEXTURE2D_DESC d{};
			d.Width = (UINT)g.small_w;
			d.Height = (UINT)g.small_h;
			d.MipLevels = 1;
			d.ArraySize = 1;
			d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			d.SampleDesc.Count = 1;
			d.Usage = D3D11_USAGE_DEFAULT;
			d.BindFlags = D3D11_BIND_RENDER_TARGET;

			ComPtr<ID3D11Texture2D> tex;
			if (FAILED(g.device->CreateTexture2D(&d, nullptr, &tex)) ||
				FAILED(g.device->CreateRenderTargetView(tex.Get(), nullptr, &g.scale_rtv)))
			{
				g.note("no-scale-target", "the down-scale target could not be created");
				return false;
			}

			d.BindFlags = 0;
			d.Usage = D3D11_USAGE_STAGING;
			d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

			ComPtr<ID3D11Texture2D> small_staging;
			if (FAILED(g.device->CreateTexture2D(&d, nullptr, &small_staging)))
			{
				g.note("no-small-staging", "the small readback texture could not be created");
				return false;
			}

			g.scale_tex = tex;
			g.staging_small = small_staging;
			g.scale_w = g.small_w;
			g.scale_h = g.small_h;
			return true;
		}

		// Sample the block down to the published size, on the GPU, into the readback
		// texture - the area average over the block the CPU path used to do, in one
		// pass.  The map that consumes it is a step of its own, a frame later: see
		// process_pending().
		bool start_scaled_copy()
		{
			if (!ensure_scale_targets())
				return false;

			// The pass needs pipeline state of its own: the panel draws with a
			// scissor and blending, and both would clip or dim a full-screen write.
			static ComPtr<ID3D11RasterizerState> rs;
			static ComPtr<ID3D11BlendState> bs;
			if (!rs)
			{
				D3D11_RASTERIZER_DESC rd{};
				rd.FillMode = D3D11_FILL_SOLID;
				rd.CullMode = D3D11_CULL_NONE;
				rd.ScissorEnable = FALSE;
				rd.DepthClipEnable = TRUE;
				if (FAILED(g.device->CreateRasterizerState(&rd, &rs)))
					return false;
			}
			if (!bs)
			{
				D3D11_BLEND_DESC bd{};
				bd.RenderTarget[0].BlendEnable = FALSE;
				bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				if (FAILED(g.device->CreateBlendState(&bd, &bs)))
					return false;
			}

			ID3D11RenderTargetView* rtv = g.scale_rtv.Get();
			g.context->OMSetRenderTargets(1, &rtv, nullptr);

			D3D11_VIEWPORT vp{};
			vp.Width  = (float)g.small_w;
			vp.Height = (float)g.small_h;
			vp.MaxDepth = 1.f;
			g.context->RSSetViewports(1, &vp);
			g.context->RSSetState(rs.Get());
			const float blend[4] = { 0.f, 0.f, 0.f, 0.f };
			g.context->OMSetBlendState(bs.Get(), blend, 0xffffffff);

			g.context->IASetInputLayout(nullptr);
			g.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			g.context->VSSetShader(g.scale_vs.Get(), nullptr, 0);
			g.context->PSSetShader(g.scale_ps.Get(), nullptr, 0);

			ID3D11ShaderResourceView* srv = g.block_srv.Get();
			ID3D11SamplerState* smp = g.scale_smp.Get();
			g.context->PSSetShaderResources(0, 1, &srv);
			g.context->PSSetSamplers(0, 1, &smp);
			g.context->Draw(3, 0);

			// Unbind before the copy: a resource that is still bound as a target
			// cannot be copied, and the block view has to go too or the next capture
			// cannot write the block.
			ID3D11ShaderResourceView* none = nullptr;
			g.context->PSSetShaderResources(0, 1, &none);
			ID3D11RenderTargetView* no_rtv = nullptr;
			g.context->OMSetRenderTargets(1, &no_rtv, nullptr);

			g.context->CopyResource(g.staging_small.Get(), g.scale_tex.Get());

			// Issued, not read: the map waits for this copy on the next tick,
			// where it has already landed and costs a transfer rather than a wait.
			g.small_pending = true;
			return true;
		}

		// Consume the copy the previous tick issued.  1 read, 0 no path at all,
		// -1 not ready yet.  The map never blocks: a staging texture that is still
		// being written is left for a later tick, and until then the menu goes on
		// sampling the image it already has - which is what the frame time bought.
		int consume_scaled_copy()
		{
			if (!g.context || !g.staging_small)
				return 0;

			D3D11_MAPPED_SUBRESOURCE map{};
			const HRESULT hr = g.context->Map(g.staging_small.Get(), 0,
				(D3D11_MAP)(D3D11_MAP_READ | D3D11_MAP_FLAG_DO_NOT_WAIT), 0, &map);

			if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
			{
				// One transfer in flight at a time is the design, so this is a
				// frame or two.  A driver that never reports the copy as done would
				// leave the glass blank for good, so after a few tries the wait is
				// taken once rather than never.
				if (++g.small_wait < k_read_wait_limit)
					return -1;

				g.small_wait = 0;
				if (FAILED(g.context->Map(g.staging_small.Get(), 0, D3D11_MAP_READ, 0, &map)))
				{
					g.small_pending = false;
					g.note("map-failed", "the scaled block could not be read");
					return 0;
				}
			}
			else if (FAILED(hr))
			{
				g.small_pending = false;
				g.note("map-failed", "the scaled block could not be read");
				return 0;
			}

			g.small_wait    = 0;
			g.small_pending = false;

			const size_t row = (size_t)g.small_w * 4;
			for (int y = 0; y < g.small_h; ++y)
				memcpy(g.scaled.data() + (size_t)y * row,
					(const unsigned char*)map.pData + (size_t)y * map.RowPitch, row);

			g.context->Unmap(g.staging_small.Get(), 0);
			return 1;
		}

		// Everything except the published image goes; the image is what the menu
		// holds and it stays valid so the glass keeps showing the last frame.
		void release_capture()
		{
			if (g.dupl && g.frame_held)
			{
				g.dupl->ReleaseFrame();
				g.frame_held = false;
			}
			g.dupl.Reset();
			g.staging.Reset();
			g.mid.Reset();
			g.mid_pending = false;

			// The copy in `staging_small` is of the block that was just thrown
			// away, and the mapping that goes with it is gone too.
			g.small_pending = false;
			g.small_wait    = 0;
		}

		void release_all()
		{
			release_capture();
			g.image_tex.Reset();
			g.image_view.Reset();
			g.scale_tex.Reset();
			g.scale_rtv.Reset();
			g.block_srv.Reset();
			g.staging_small.Reset();
			g.block_srv_of = nullptr;
			g.scale_w = g.scale_h = 0;
			g.scaled.clear();
			g.scratch.clear();
			g.has_image = false;
			g.frames = 0;
			g.logged_first = false;
			g.desktop_w = g.desktop_h = g.small_w = g.small_h = 0;
			g.out_w = g.out_h = 0;
			g.cap_w = g.cap_h = 0;
			g.cap_dirty = false;
			g.pub_view = nullptr;
			g.pub_w = g.pub_h = 0;
			g.pub_scale = 1.f;
			g.device.Reset();
			g.context.Reset();
			g.note("idle", "idle");
		}

		// The duplication is per output, so pick the one the overlay sits on.
		ComPtr<IDXGIOutput> output_for_window(HWND window)
		{
			ComPtr<IDXGIOutput> out;
			if (!g.device)
				return out;

			const HMONITOR monitor = window
				? ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)
				: ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
			if (!monitor)
				return out;

			ComPtr<IDXGIDevice> dxgi_device;
			if (FAILED(g.device.As(&dxgi_device)))
				return out;

			ComPtr<IDXGIAdapter> adapter;
			if (FAILED(dxgi_device->GetAdapter(&adapter)))
				return out;

			for (UINT i = 0;; ++i)
			{
				ComPtr<IDXGIOutput> candidate;
				if (adapter->EnumOutputs(i, &candidate) == DXGI_ERROR_NOT_FOUND)
					break;

				DXGI_OUTPUT_DESC desc{};
				if (SUCCEEDED(candidate->GetDesc(&desc)) && desc.Monitor == monitor)
				{
					out = candidate;
					break;
				}
			}
			return out;
		}

		// Create (or replace) the duplication and the staging texture that the
		// frames are copied into.  Returns false with `status` set on failure.
		bool acquire_capture(HWND overlay_window)
		{
			release_capture();

			ComPtr<IDXGIOutput> output = output_for_window(overlay_window);
			if (!output)
			{
				g.note("no-output", "no output for this monitor");
				return false;
			}

			DXGI_OUTPUT_DESC desc{};
			if (FAILED(output->GetDesc(&desc)))
			{
				g.note("no-desc", "output description failed");
				return false;
			}

			// The duplication needs the output's own adapter, which is the same
			// one the overlay's device was made on.
			ComPtr<IDXGIOutput1> output1;
			if (FAILED(output.As(&output1)))
			{
				g.note("no-output1", "capture interface unavailable");
				return false;
			}

			ComPtr<IDXGIOutputDuplication> dupl;
			const HRESULT hr = output1->DuplicateOutput(g.device.Get(), &dupl);
			if (FAILED(hr) || !dupl)
			{
				char text[128];
				snprintf(text, sizeof text,
					"the display could not be duplicated (0x%08lX) - another capture may hold it",
					(unsigned long)hr);
				g.note("capture-failed", text);
				return false;
			}

			const RECT r = desc.DesktopCoordinates;
			const int ow = (int)(r.right - r.left);
			const int oh = (int)(r.bottom - r.top);
			if (ow <= 0 || oh <= 0)
			{
				g.note("bad-size", "the display reports no size");
				return false;
			}

			g.out_w = ow;
			g.out_h = oh;
			g.cap_screen_x = (int)r.left;
			g.cap_screen_y = (int)r.top;

			// The block the caller asked for, or the whole desktop when there is
			// nothing worth restricting to.
			if (!compute_block())
			{
				g.cap_x = 0;
				g.cap_y = 0;
				g.cap_w = ow;
				g.cap_h = oh;
			}

			const int w = g.cap_w;
			const int h = g.cap_h;

			D3D11_TEXTURE2D_DESC sd{};
			sd.Width = (UINT)w;
			sd.Height = (UINT)h;
			sd.MipLevels = 1;
			sd.ArraySize = 1;
			sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			sd.SampleDesc.Count = 1;
			sd.Usage = D3D11_USAGE_STAGING;
			sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

			ComPtr<ID3D11Texture2D> staging;
			if (FAILED(g.device->CreateTexture2D(&sd, nullptr, &staging)))
			{
				g.note("no-staging", "staging texture could not be created");
				return false;
			}

			// The intermediate the frame is copied into first.  Reading the block
			// one capture tick after it was copied is what removes the stall: a Map
			// issued right behind a desktop copy waits for the whole frame to be
			// composited and transferred, and that wait was ~15 ms of the ~21 ms a
			// capture cost even after the capture was cut down to the panel.
			D3D11_TEXTURE2D_DESC md{};
			md.Width = (UINT)w;
			md.Height = (UINT)h;
			md.MipLevels = 1;
			md.ArraySize = 1;
			md.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			md.SampleDesc.Count = 1;
			md.Usage = D3D11_USAGE_DEFAULT;

			ComPtr<ID3D11Texture2D> mid;
			if (FAILED(g.device->CreateTexture2D(&md, nullptr, &mid)))
			{
				g.note("no-mid", "the intermediate copy could not be created");
				return false;
			}

			g.mid = mid;
			g.mid_pending = false;
			g.dupl = dupl;
			g.staging = staging;
			g.desktop_w = w;
			g.desktop_h = h;
			g.origin_x = (int)r.left + g.cap_x;
			g.origin_y = (int)r.top + g.cap_y;

			g.small_w = 0;
			g.small_h = 0;
			g.scale = 1.f;
			ensure_internal();

			// The published image and its geometry are deliberately left alone.  The
			// old block's pixels stay valid with the old block's mapping until a
			// frame arrives for the new one (_publish_ is the only writer of both),
			// so a region rebuild is invisible instead of a blank frame.
			g.cap_dirty = false;

			g.note("waiting", "waiting for the first frame");
			return true;
		}

		// Create the small texture if there is none.  It is *not* replaced when
		// the wanted size changes: the swap happens in upload(), in the same
		// breath as the pixels that need the new size.  Releasing it here would
		// drop the shader view the menu is already holding, and one frame of an
		// absent texture is exactly what a flickering panel looks like.
		bool ensure_image_texture()
		{
			if (g.image_view && g.image_tex)
				return true;

			D3D11_TEXTURE2D_DESC d{};
			d.Width = (UINT)g.small_w;
			d.Height = (UINT)g.small_h;
			d.MipLevels = 1;
			d.ArraySize = 1;
			d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			d.SampleDesc.Count = 1;
			d.Usage = D3D11_USAGE_DYNAMIC;
			d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			ComPtr<ID3D11Texture2D> tex;
			if (FAILED(g.device->CreateTexture2D(&d, nullptr, &tex)))
			{
				g.note("no-image", "blur texture could not be created");
				return false;
			}

			ComPtr<ID3D11ShaderResourceView> view;
			if (FAILED(g.device->CreateShaderResourceView(tex.Get(), nullptr, &view)))
			{
				g.note("no-view", "blur view could not be created");
				return false;
			}

			g.image_tex = tex;
			g.image_view = view;
			return true;
		}

		// The cheap path: at a 3x or deeper divisor an area average reads nine to
		// thirty-six pixels per output pixel, and all of that detail is about to
		// be boxed away anyway.  Two taps diagonally through the block centre
		// keep enough of the frame to avoid a patched look and cost four reads.
		void downsample_taps(const unsigned char* src, int pitch)
		{
			const float scale = g.scale > 0.01f ? g.scale : 1.f;
			const int   span  = std::max(1, (int)scale);

			for (int y = 0; y < g.small_h; ++y)
			{
				int cy = (int)((y + 0.5f) * scale);
				int y0 = std::min(std::max(cy - 1, 0), g.desktop_h - 1);
				int y1 = std::min(y0 + std::max(1, span / 2), g.desktop_h - 1);
				if (y1 == y0 && y0 < g.desktop_h - 1) y1 = y0 + 1;

				unsigned char* dst = g.scaled.data() + (size_t)y * (size_t)g.small_w * 4;

				for (int x = 0; x < g.small_w; ++x)
				{
					int cx = (int)((x + 0.5f) * scale);
					int x0 = std::min(std::max(cx - 1, 0), g.desktop_w - 1);
					int x1 = std::min(x0 + std::max(1, span / 2), g.desktop_w - 1);
					if (x1 == x0 && x0 < g.desktop_w - 1) x1 = x0 + 1;

					const unsigned char* p00 = src + (size_t)y0 * (size_t)pitch + (size_t)x0 * 4;
					const unsigned char* p01 = src + (size_t)y0 * (size_t)pitch + (size_t)x1 * 4;
					const unsigned char* p10 = src + (size_t)y1 * (size_t)pitch + (size_t)x0 * 4;
					const unsigned char* p11 = src + (size_t)y1 * (size_t)pitch + (size_t)x1 * 4;

					for (int c = 0; c < 4; ++c)
						dst[c] = (unsigned char)((p00[c] + p01[c] + p10[c] + p11[c] + 2) >> 2);

					dst += 4;
				}
			}
		}

		// Area-average the desktop down to the small buffer.  Reads the whole
		// frame linearly (cheap) and writes one pixel per block.
		void downsample(const unsigned char* src, int pitch)
		{
			const float scale = g.scale > 0.01f ? g.scale : 1.f;

			// Three-by-three blocks and up go through the tap path above.
			if (scale >= 2.6f)
			{
				downsample_taps(src, pitch);
				return;
			}

			for (int y = 0; y < g.small_h; ++y)
			{
				int y0 = (int)(y * scale);
				int y1 = (int)((y + 1) * scale);
				if (y1 <= y0) y1 = y0 + 1;
				if (y0 < 0) y0 = 0;
				if (y1 > g.desktop_h) y1 = g.desktop_h;

				unsigned char* dst = g.scaled.data() + (size_t)y * (size_t)g.small_w * 4;

				for (int x = 0; x < g.small_w; ++x)
				{
					int x0 = (int)(x * scale);
					int x1 = (int)((x + 1) * scale);
					if (x1 <= x0) x1 = x0 + 1;
					if (x0 < 0) x0 = 0;
					if (x1 > g.desktop_w) x1 = g.desktop_w;

					unsigned int b = 0, g_ = 0, r = 0, a = 0, n = 0;
					for (int sy = y0; sy < y1; ++sy)
					{
						const unsigned char* row = src + (size_t)sy * (size_t)pitch + (size_t)x0 * 4;
						for (int sx = x0; sx < x1; ++sx)
						{
							b += row[0]; g_ += row[1]; r += row[2]; a += row[3];
							row += 4;
							++n;
						}
					}
					if (!n) n = 1;

					dst[0] = (unsigned char)(b / n);
					dst[1] = (unsigned char)(g_ / n);
					dst[2] = (unsigned char)(r / n);
					dst[3] = (unsigned char)(a / n);
					dst += 4;
				}
			}
		}

		// One separable box pass.  A running sum, so the cost does not depend on
		// the radius - three of these approximate a gaussian closely enough for a
		// frosted panel.
		//
		// Three things make this cheap, and each of them was measured on the
		// image the log actually publishes (800x450 at the bottom of the slider,
		// where the divisor curve is at its widest):
		//
		//  * the reciprocal.  A division per channel per pixel was the whole cost
		//    of this loop - an integer divide is around twenty cycles and there
		//    were four of them per pixel per pass.  A reciprocal computed once per
		//    pass and a multiply-shift is the same average to within one level;
		//  * the row-major walk.  The vertical half used to run column by column,
		//    so every pixel of a row was a whole row away from the last - a cache
		//    miss per pixel on any image wider than L1.  A single column-sum
		//    buffer makes both halves walk their input and their output linearly;
		//  * three channels instead of four.  The image is BGRA, but the alpha is
		//    put back as opaque when it is uploaded, so half the blur was carrying
		//    a channel nothing downstream reads.
		void box_h(const unsigned char* src, unsigned char* dst, int w, int h, int radius)
		{
			const unsigned int inv = (unsigned int)((1ull << 24) / (unsigned int)(radius * 2 + 1));

			for (int y = 0; y < h; ++y)
			{
				const unsigned char* row = src + (size_t)y * (size_t)w * 4;
				unsigned char* out = dst + (size_t)y * (size_t)w * 4;

				unsigned int sum[3] = { 0, 0, 0 };
				for (int i = -radius; i <= radius; ++i)
				{
					const int cx = std::min(std::max(i, 0), w - 1);
					for (int c = 0; c < 3; ++c) sum[c] += row[cx * 4 + c];
				}

				for (int x = 0; x < w; ++x)
				{
					out[0] = (unsigned char)((sum[0] * inv) >> 24);
					out[1] = (unsigned char)((sum[1] * inv) >> 24);
					out[2] = (unsigned char)((sum[2] * inv) >> 24);
					out[3] = 255;

					const int add = std::min(std::max(x + radius + 1, 0), w - 1);
					const int sub = std::min(std::max(x - radius, 0), w - 1);
					for (int c = 0; c < 3; ++c)
						sum[c] = sum[c] + row[add * 4 + c] - row[sub * 4 + c];

					out += 4;
				}
			}
		}

		// The vertical half, as a sliding window over one column-sum buffer: the
		// window is added and dropped a whole row at a time, so both the reads
		// and the writes are linear.
		void box_v(const unsigned char* src, unsigned char* dst, int w, int h, int radius)
		{
			const unsigned int inv = (unsigned int)((1ull << 24) / (unsigned int)(radius * 2 + 1));

			const size_t cols = (size_t)w * 3;
			if (g.colsum.size() < cols)
				g.colsum.resize(cols);
			unsigned int* sums = g.colsum.data();
			std::memset(sums, 0, cols * sizeof(unsigned int));

			for (int i = -radius; i <= radius; ++i)
			{
				const int cy = std::min(std::max(i, 0), h - 1);
				const unsigned char* row = src + (size_t)cy * (size_t)w * 4;
				unsigned int* s = sums;
				for (int x = 0; x < w; ++x, row += 4, s += 3)
				{
					s[0] += row[0];
					s[1] += row[1];
					s[2] += row[2];
				}
			}

			for (int y = 0; y < h; ++y)
			{
				unsigned char* out = dst + (size_t)y * (size_t)w * 4;
				{
					const unsigned int* s = sums;
					for (int x = 0; x < w; ++x, out += 4, s += 3)
					{
						out[0] = (unsigned char)((s[0] * inv) >> 24);
						out[1] = (unsigned char)((s[1] * inv) >> 24);
						out[2] = (unsigned char)((s[2] * inv) >> 24);
						out[3] = 255;
					}
				}

				const int cya = std::min(std::max(y + radius + 1, 0), h - 1);
				const int cys = std::min(std::max(y - radius, 0), h - 1);
				const unsigned char* pa = src + (size_t)cya * (size_t)w * 4;
				const unsigned char* ps = src + (size_t)cys * (size_t)w * 4;
				{
					unsigned int* s = sums;
					for (int x = 0; x < w; ++x, pa += 4, ps += 4, s += 3)
					{
						s[0] = s[0] + pa[0] - ps[0];
						s[1] = s[1] + pa[1] - ps[1];
						s[2] = s[2] + pa[2] - ps[2];
					}
				}
			}
		}

		void blur_small()
		{
			if (g.blur <= 2.f)
				return;                     // 0 is 0: no passes at all

			// The slider is a *screen* radius; the image is smaller than the block it
			// came from, so the same look costs a smaller box.  The real ratio is
			// read from g.scale rather than assumed to be the divisor, because the
			// width cap can make it larger.
			const float div = g.scale > 0.01f ? g.scale : 1.f;
			int radius = (int)std::lround(screen_radius() / div);
			if (radius < 1) radius = 1;

			const int passes = blur_passes(radius);
			for (int pass = 0; pass < passes; ++pass)
			{
				box_h(g.scaled.data(), g.scratch.data(), g.small_w, g.small_h, radius);
				box_v(g.scratch.data(), g.scaled.data(), g.small_w, g.small_h, radius);
			}
		}

		// Write the buffer into the texture the menu samples.  The texture is
		// replaced only when its size no longer matches, and only after the new
		// one holds pixels - the old image stays valid (and sampled) right up to
		// the frame that publishes the new one, so a region rebuild cannot blank
		// the glass for even a single frame.
		bool upload()
		{
			if (!g.context || g.small_w <= 0 || g.small_h <= 0)
				return false;

			ComPtr<ID3D11Texture2D> tex = g.image_tex;
			ComPtr<ID3D11ShaderResourceView> view = g.image_view;

			bool fresh = false;
			if (!tex || !view)
			{
				fresh = true;
			}
			else
			{
				D3D11_TEXTURE2D_DESC have{};
				tex->GetDesc(&have);
				if ((int)have.Width != g.small_w || (int)have.Height != g.small_h)
					fresh = true;
			}

			if (fresh)
			{
				D3D11_TEXTURE2D_DESC d{};
				d.Width = (UINT)g.small_w;
				d.Height = (UINT)g.small_h;
				d.MipLevels = 1;
				d.ArraySize = 1;
				d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				d.SampleDesc.Count = 1;
				d.Usage = D3D11_USAGE_DYNAMIC;
				d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				ComPtr<ID3D11Texture2D> made;
				ComPtr<ID3D11ShaderResourceView> made_view;
				if (FAILED(g.device->CreateTexture2D(&d, nullptr, &made)) ||
					FAILED(g.device->CreateShaderResourceView(made.Get(), nullptr, &made_view)))
				{
					g.note("no-image", "blur texture could not be created");
					return false;
				}
				tex = made;
				view = made_view;
			}

			D3D11_MAPPED_SUBRESOURCE map{};
			if (FAILED(g.context->Map(tex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
			{
				g.note("no-map", "blur texture could not be written");
				return false;
			}

			// The tint rides the copy.  Darkening is a multiply and the blur is
			// a mean, so the two commute: the image that comes out of this one
			// pass - read the buffer, darken it, write the texture - is the
			// image that came out of the copy followed by a whole separate pass
			// over every byte of the buffer, and that extra pass is gone.  The
			// row copy already touches every byte, so the tint is free here.
			const float k = std::min(std::max(g.tint, 0.f), 100.f) / 100.f;
			const int dark_i = (int)((1.f - k * 0.55f) * 1024.f + 0.5f);   // up to 55% darker

			const size_t row = (size_t)g.small_w * 4;
			for (int y = 0; y < g.small_h; ++y)
			{
				unsigned char* dst = (unsigned char*)map.pData + (size_t)y * map.RowPitch;
				const unsigned char* src = g.scaled.data() + (size_t)y * row;

				if (dark_i >= 1024)
				{
					// Nothing to darken: the rows are copied whole.  The alpha is
					// opaque in the buffer already (box_h writes it), so a memcpy
					// is the whole of it.
					memcpy(dst, src, row);
					continue;
				}

				for (size_t x = 0; x < row; x += 4)
				{
					dst[x + 0] = (unsigned char)(((int)src[x + 0] * dark_i) >> 10);
					dst[x + 1] = (unsigned char)(((int)src[x + 1] * dark_i) >> 10);
					dst[x + 2] = (unsigned char)(((int)src[x + 2] * dark_i) >> 10);
					dst[x + 3] = 255;      // opaque: the blur does not carry this channel
				}
			}

			g.context->Unmap(tex.Get(), 0);

			// Pixels are in.  Only now does the published texture change, and the
			// caller is on this thread, so the swap and the publish are one step.
			g.image_tex = tex;
			g.image_view = view;
			return true;
		}
	}

	bool start(ID3D11Device* device, ID3D11DeviceContext* context, HWND overlay_window)
	{
		release_all();

		if (!device || !context)
			return false;

		g.device = device;
		g.context = context;
		g.overlay = overlay_window;

		if (!acquire_capture(overlay_window))
			return false;

		if (!ensure_image_texture())
		{
			release_all();
			return false;
		}

		g.retry_at = 0;
		g.last_frame = 0;
		g.note("waiting", "waiting for the first frame");
		return true;
	}

	void stop()
	{
		release_all();
	}

	bool started()
	{
		return g.device && g.dupl && g.staging;
	}

	// False when there is nothing to capture at all: the sliders are at the
	// bottom, or the cap is at 0 and the capture must not be started.  The
	// caller skips the whole duplication on this answer, which is what makes
	// "cap 0" free rather than merely cheap.
	bool wanted()
	{
		return effect_wanted() && g.cap_pct > 0.5f;
	}

	void set_params(const params_t& params)
	{
		g.blur    = params.blur;
		g.tint    = params.tint;

		// A cap that moved has to rebuild: the block the last one chose is not
		// necessarily inside the new ceiling.
		const float was_cap = g.cap_pct;
		g.cap_pct = params.capture_cap;
		if (std::fabs(was_cap - g.cap_pct) > 0.01f)
			g.cap_dirty = true;
	}

	params_t params()
	{
		params_t p;
		p.blur        = g.blur;
		p.tint        = g.tint;
		p.capture_cap = g.cap_pct;
		return p;
	}

		void set_region(float x0, float y0, float x1, float y1)
		{
			if (x1 - x0 < 8.f || y1 - y0 < 8.f)
			{
				if (!g.have_req) return;
				g.have_req = false;
				g.cap_dirty = true;
				return;
			}

			// The same rectangle again is the common case by far: no work, and no
			// rebuild, so a still panel costs nothing to keep asking.
			if (g.have_req &&
				std::fabs(g.req_x - x0) < 2.f && std::fabs(g.req_y - y0) < 2.f &&
				std::fabs(g.req_x1 - x1) < 2.f && std::fabs(g.req_y1 - y1) < 2.f)
				return;

			g.have_req = true;
			g.req_x  = x0;
			g.req_y  = y0;
			g.req_x1 = x1;
			g.req_y1 = y1;

			if (g.cap_w <= 0 || !block_covers_request())
				g.cap_dirty = true;
		}		void publish()
		{
			const float s = 1.f / (g.scale > 0.01f ? g.scale : 1.f);
			g.pub_view     = (void*)g.image_view.Get();
			g.pub_w        = g.small_w;
			g.pub_h        = g.small_h;
			g.pub_origin_x = (float)g.origin_x * s;
			g.pub_origin_y = (float)g.origin_y * s;
			g.pub_scale    = s;
		}

		// Everything after the pixels are in the buffer: the blur, the upload -
		// which is where the tint is applied now - and the publish, whichever
		// path read them.
		bool finish_frame(std::chrono::steady_clock::time_point work0)
		{
			blur_small();

			if (!upload())
				return false;

			g.cpu_ms = std::chrono::duration<float, std::milli>(
				std::chrono::steady_clock::now() - work0).count();

			// One quality step at a time, and never more than twice a second:
			// a frame that spikes once must not resize the image under the
			// panel, and a machine that is fast again must get the detail
			// back rather than staying coarse for the session.
			{
				const unsigned long long t = now_ms();
				if (g.cpu_ms > 3.5f && g.auto_div < 3 && t >= g.quality_at + 500)
				{
					++g.auto_div;
					g.quality_at = t;
					g.cheap_run  = 0.f;
				}
				else if (g.cpu_ms < 0.9f && g.auto_div > 0)
				{
					g.cheap_run += 1.f;
					if (g.cheap_run > 180.f && t >= g.quality_at + 1000)
					{
						--g.auto_div;
						g.quality_at = t;
						g.cheap_run  = 0.f;
					}
				}
				else
				{
					g.cheap_run = 0.f;
				}
			}

			// Everything the menu reads changes in this one step: the image, its
			// size and where it sits on the screen.
			publish();
			g.has_image = true;
			g.last_frame = now_ms();
			++g.frames;

			// The status line carries the cost, so a slider move can be checked
			// against a number instead of a feeling.
			char text[96];
			snprintf(text, sizeof text, "live - %.2f ms, age %.0f ms, %dx%d from a %dx%d block",
				g.cpu_ms, (float)(now_ms() > g.last_frame ? now_ms() - g.last_frame : 0),
				g.small_w, g.small_h, g.desktop_w, g.desktop_h);
			g.note("live", text);

			if (!g.logged_first)
			{
				g.logged_first = true;
				debug::log("render", "backdrop: first frame %dx%d from a %dx%d block of a %dx%d desktop",
					g.small_w, g.small_h, g.desktop_w, g.desktop_h, g.out_w, g.out_h);
			}

			return true;
		}

		// The CPU path: the block itself is copied into a readback texture and
		// mapped on the spot, which is the stall the split readback exists to
		// avoid.  Kept for the one case it is the only option - a device where the
		// down-scale shader could not be built at all - where one stall is better
		// than no glass.
		bool process_pending_cpu(std::chrono::steady_clock::time_point work0)
		{
			g.small_pending = false;
			g.context->CopyResource(g.staging.Get(), g.mid.Get());

			D3D11_MAPPED_SUBRESOURCE map{};
			if (FAILED(g.context->Map(g.staging.Get(), 0, D3D11_MAP_READ, 0, &map)))
			{
				g.note("map-failed", "the captured block could not be read");
				return false;
			}

			// Always the area average: the image is never the block at 1:1 any
			// more, so the row copy that used to be here is gone with the divisor
			// of one that asked for it.
			downsample((const unsigned char*)map.pData, (int)map.RowPitch);
			g.context->Unmap(g.staging.Get(), 0);

			return finish_frame(work0);
		}

		// Issue the down-scale pass and its staging copy for the block that was just
		// captured.  Cheap by construction - one full-screen triangle and one copy,
		// both on the GPU - which is what lets it run on every captured frame.  The
		// only path that reaches the CPU fallback is the one with no GPU path at
		// all, and it has to read the block here, in the tick that copied it.
		void issue_pending_scale()
		{
			const auto tick0 = std::chrono::steady_clock::now();

			if (!start_scaled_copy())
				process_pending_cpu(tick0);

			g.issue_ms = std::chrono::duration<float, std::milli>(
				std::chrono::steady_clock::now() - tick0).count();
		}

		// Collect the copy the last tick issued, and publish what comes out of it.
		// The copy has had a whole frame to land, so the map below is a transfer
		// and nothing here waits on the GPU.
		bool process_pending()
		{
			if (!g.mid_pending || !g.mid || !g.staging || !g.context)
				return false;

			g.mid_pending = false;

			if (!ensure_image_texture())
				return false;

			const auto work0 = std::chrono::steady_clock::now();

			// Nothing in flight: the issue step has not run since the last publish,
			// so there is nothing to collect and the menu keeps the image it has.
			if (!g.small_pending)
				return false;

			const int read = consume_scaled_copy();
			if (read < 0)
				return false;              // still landing; try again next tick

			if (read == 0)
			{
				// The GPU path is not there at all.  Issue one and read it here, on
				// the CPU, which is the only case that is allowed to stall.
				if (!start_scaled_copy())
					return process_pending_cpu(work0);
				return false;
			}

			return finish_frame(work0);
		}

		bool update()
		{
			if (!g.dupl || !g.staging || !g.context)
				return false;

			// The slider may have moved since the last frame; the buffers follow it
			// and the capture is left alone.
			ensure_internal();
			if (!ensure_image_texture())
				return false;

		// The block copied last tick is read first, so the work below never waits
		// on the desktop: by now the GPU finished that copy long ago - and it is
		// read *before* a region rebuild, because a rebuild throws the pending
		// frame away and a drag does not get a new block every frame.  Doing it
		// the other way round held the image still for the whole throttle below
		// whenever the panel moved, which is the "the blur lags behind the panel"
		// this ordering fixes.
		bool processed = process_pending();

		// A region that no longer fits needs a new block.  Throttled, because a
		// dragged panel walks its own bounds and every rebuild is a re-acquire -
		// but only briefly: the margin in compute_block() means a normal drag
		// stays inside the block it already has, and this is the backstop for a
		// fast one.  It used to be 400 ms, which is longer than a drag lasts.
		if (g.cap_dirty)
		{
			const unsigned long long t = now_ms();
			if (t >= g.cap_at)
			{
				// One frame's worth of wait at most: a block that is rebuilt while
				// the panel walks out of it is an image of the wrong rectangle, and
				// that reads as the blur lagging behind the panel just as much as a
				// stale image does.
				g.cap_at = t + 16;
				acquire_capture(g.overlay);
				return processed;        // this frame keeps the previous image
			}
		}

		if (!effect_wanted())
		{
			// The blur is at 0, so the glass has nothing to add and the capture
			// would be a copy of what the panel already sits over.  The published
			// image goes with it - the menu must not keep drawing the last frame
			// once the effect is off.
			g.has_image = false;
			g.pub_view = nullptr;
			g.pub_w = g.pub_h = 0;
			g.mid_pending = false;
			g.note("idle", "blur is 0 - capture idle");
			return false;
		}

		DXGI_OUTDUPL_FRAME_INFO info{};
		ComPtr<IDXGIResource> resource;
		const HRESULT hr = g.dupl->AcquireNextFrame(0, &info, &resource);

		if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		{
			// Nothing new on the desktop this frame.  Hold the image; this is
			// the branch the old version got wrong.
			return processed;
		}

		if (FAILED(hr))
		{
			const unsigned long long now = now_ms();
			if (now >= g.retry_at)
			{
				g.retry_at = now + 1000;
				if (hr == DXGI_ERROR_ACCESS_LOST)
				{
					g.note("lost", "capture lost - re-acquiring");
					acquire_capture(g.overlay);
				}
				else
				{
					char text[128];
					snprintf(text, sizeof text, "capture error 0x%08lX", (unsigned long)hr);
					g.note("error", text);
					acquire_capture(g.overlay);
				}
			}
			return processed;
		}

		g.frame_held = true;

		ComPtr<ID3D11Texture2D> frame_tex;
		if (resource && SUCCEEDED(resource.As(&frame_tex)) && g.mid)
		{
			// Only the block is copied, and only into the intermediate: the read
			// happens next tick, which is what takes the desktop's own latency
			// out of the frame.  A full CopyResource of an 8 MB frame was the
			// other half of what made a light blur expensive.
			if (g.cap_w == g.out_w && g.cap_h == g.out_h && g.cap_x == 0 && g.cap_y == 0)
			{
				g.context->CopyResource(g.mid.Get(), frame_tex.Get());
			}
			else
			{
				const D3D11_BOX box{
					(UINT)g.cap_x, (UINT)g.cap_y, 0,
					(UINT)(g.cap_x + g.cap_w), (UINT)(g.cap_y + g.cap_h), 1 };
				g.context->CopySubresourceRegion(g.mid.Get(), 0, 0, 0, 0, frame_tex.Get(), 0, &box);
			}

			g.mid_pending = true;
		}

		g.dupl->ReleaseFrame();
		g.frame_held = false;

		// The capture tick ends by issuing the pass for the block it just copied.
		// The next tick collects it, by which time the copy has landed - so the
		// image is one frame old and refreshed on every frame, and the map on the
		// collecting side never waits for anything.  That wait is what this used to
		// measure and retire itself over; there is nothing left to retire.
		if (g.mid_pending)
			issue_pending_scale();

		return processed;
	}

	bool ready()
	{
		return g.pub_view != nullptr && g.pub_w > 0 && g.pub_h > 0;
	}

	void* image()
	{
		return g.pub_view;
	}

	float image_w() { return (float)g.pub_w; }
	float image_h() { return (float)g.pub_h; }

	float origin_x() { return g.pub_origin_x; }
	float origin_y() { return g.pub_origin_y; }

	float image_scale() { return g.pub_scale; }

	const char* status()
	{
		return g.status;
	}

	float cost_ms()
	{
		return g.cpu_ms;
	}

	// How old the published image is, in milliseconds.  This is the number the
	// "the blur lags behind the panel" reports are about, so it is published next
	// to the cost instead of being guessed at.
	float image_age_ms()
	{
		if (!g.has_image)
			return -1.f;

		const unsigned long long now = now_ms();
		return (float)(now > g.last_frame ? now - g.last_frame : 0);
	}

	float readback_ms()
	{
		return g.issue_ms;
	}

	// The read path is split across two ticks by construction: the pass is issued
	// with the capture, and the map collects it on the next frame.
	bool split_readback()
	{
		return true;
	}

	const char* status_key()
	{
		return g.key;
	}
}
