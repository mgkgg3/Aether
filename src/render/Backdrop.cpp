
#include "Backdrop.h"

#ifndef DXGI_RGBA_DEFINED
#define DXGI_RGBA_DEFINED
typedef struct DXGI_RGBA
{
    float r;
    float g;
    float b;
    float a;
} DXGI_RGBA;
#endif

#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

namespace backdrop
{
namespace
{

	const char* kHLSL = R"HLSL(
cbuffer PassCB : register(b0)
{
    float4 texel_spread;
    float4 size_pad;
    float4 fill;
    float4 mixp;
};
Texture2D    Src : register(t0);
Texture2D    Aux : register(t1);
SamplerState LinearClamp : register(s0);

struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };

VSOut VSFull(uint id : SV_VertexID)
{
    VSOut o;
    const float2 uv = float2((id << 1) & 2, id & 2);
    o.uv  = uv;
    o.pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return o;
}

float4 PSDown(VSOut i) : SV_Target
{
    const float2 h = texel_spread.xy * (0.5 * texel_spread.z);
    float4 s = Src.Sample(LinearClamp, i.uv) * 4.0;
    s += Src.Sample(LinearClamp, i.uv + float2(-h.x, -h.y));
    s += Src.Sample(LinearClamp, i.uv + float2( h.x, -h.y));
    s += Src.Sample(LinearClamp, i.uv + float2(-h.x,  h.y));
    s += Src.Sample(LinearClamp, i.uv + float2( h.x,  h.y));
    return s / 8.0;
}

float4 PSUp(VSOut i) : SV_Target
{
    const float2 h = texel_spread.xy * (0.5 * texel_spread.z);
    float4 s  = Src.Sample(LinearClamp, i.uv + float2(-h.x * 2.0, 0.0));
    s += Src.Sample(LinearClamp, i.uv + float2(-h.x,  h.y)) * 2.0;
    s += Src.Sample(LinearClamp, i.uv + float2( 0.0,  h.y * 2.0));
    s += Src.Sample(LinearClamp, i.uv + float2( h.x,  h.y)) * 2.0;
    s += Src.Sample(LinearClamp, i.uv + float2( h.x * 2.0, 0.0));
    s += Src.Sample(LinearClamp, i.uv + float2( h.x, -h.y)) * 2.0;
    s += Src.Sample(LinearClamp, i.uv + float2( 0.0, -h.y * 2.0));
    s += Src.Sample(LinearClamp, i.uv + float2(-h.x, -h.y)) * 2.0;
    return s / 12.0;
}

float4 PSFill(VSOut i) : SV_Target
{
    const bool  horiz = fill.w > 0.5;
    const float d     = horiz ? i.uv.y : i.uv.x;
    const float edge  = fill.x;
    const float near  = fill.y;
    const float room  = max(fill.z, 1e-5);

    const float s    = (near >= edge) ? 1.0 : -1.0;
    const float dirn = -s;
    const float w    = (d - near) * s;

    const float tri = abs(frac(w / (2.0 * room) + 0.5) * 2.0 - 1.0) * room;
    const float sd  = edge + dirn * tri;

    return Src.Sample(LinearClamp, saturate(horiz ? float2(i.uv.x, sd) : float2(sd, i.uv.y)));
}

float hash21(float2 p)
{
    p = frac(p * float2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return frac(p.x * p.y);
}

float4 PSMix(VSOut i) : SV_Target
{
    const float3 soft  = Src.Sample(LinearClamp, i.uv).rgb;
    const float3 heavy = Aux.Sample(LinearClamp, i.uv).rgb;
    float3 c = lerp(soft, heavy, saturate(mixp.x));
    c += (hash21(i.uv * size_pad.xy) - 0.5) * mixp.y;
    return float4(saturate(c), 1.0);
}
)HLSL";

	ID3D11Device*        s_device = nullptr;
	ID3D11DeviceContext* s_ctx    = nullptr;
	HWND                 s_hwnd   = nullptr;
	bool                 s_started = false;

	ID3D11VertexShader* s_vs      = nullptr;
	ID3D11PixelShader*  s_ps_down = nullptr;
	ID3D11PixelShader*  s_ps_up   = nullptr;
	ID3D11PixelShader*  s_ps_fill = nullptr;
	ID3D11PixelShader*  s_ps_mix  = nullptr;
	ID3D11Buffer*       s_cb      = nullptr;
	ID3D11SamplerState* s_sampler = nullptr;
	ID3D11BlendState*   s_blend   = nullptr;
	ID3D11RasterizerState* s_raster = nullptr;
	ID3D11RasterizerState* s_raster_scissor = nullptr;
	ID3D11DepthStencilState* s_depth = nullptr;

	IDXGIOutputDuplication* s_dupl    = nullptr;
	IDXGIOutput1*           s_output1 = nullptr;
	int s_origin_x = 0, s_origin_y = 0;

	struct rt_t
	{
		ID3D11Texture2D*          tex = nullptr;
		ID3D11RenderTargetView*   rtv = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		int w = 0, h = 0;
	};	rt_t s_desktop;
	rt_t s_masked;
	DXGI_FORMAT s_desktop_format = DXGI_FORMAT_B8G8R8A8_UNORM;
	rt_t s_down[6];
	rt_t s_up[5];
	rt_t s_soft;
	rt_t s_heavy;
	rt_t s_image;
	int  s_w = 0, s_h = 0;
	float s_scale = 1.f;

	params_t s_params;
	bool     s_have_image = false;
	unsigned long long s_retry_at = 0;
	int      s_dup_fail = 0;
	int      s_last_hr  = 0;
	unsigned s_frames   = 0;

	unsigned long long s_rate_at     = 0;
	unsigned           s_rate_frames = 0;
	float              s_caps_per_sec = 0.f;

	float s_occ[8][4]{};
	int   s_occ_n = 0;

	bool s_self_in_capture = false;

	bool s_masked_ok = false;

	void release_rt(rt_t& rt)
	{
		if (rt.srv) { rt.srv->Release(); rt.srv = nullptr; }
		if (rt.rtv) { rt.rtv->Release(); rt.rtv = nullptr; }
		if (rt.tex) { rt.tex->Release(); rt.tex = nullptr; }
		rt.w = rt.h = 0;
	}

	bool create_rt(rt_t& rt, int w, int h, DXGI_FORMAT format, bool want_rtv, bool want_srv)
	{
		release_rt(rt);
		if (w <= 0 || h <= 0)
			return false;

		D3D11_TEXTURE2D_DESC td{};
		td.Width  = (UINT)w;
		td.Height = (UINT)h;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = format;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = (want_rtv ? D3D11_BIND_RENDER_TARGET : 0) | (want_srv ? D3D11_BIND_SHADER_RESOURCE : 0);

		if (FAILED(s_device->CreateTexture2D(&td, nullptr, &rt.tex)))
			return false;
		if (want_rtv && FAILED(s_device->CreateRenderTargetView(rt.tex, nullptr, &rt.rtv)))
			return false;
		if (want_srv && FAILED(s_device->CreateShaderResourceView(rt.tex, nullptr, &rt.srv)))
			return false;

		rt.w = w;
		rt.h = h;
		return true;
	}

	void release_chain()
	{
		release_rt(s_desktop);
		for (auto& r : s_down) release_rt(r);
		for (auto& r : s_up)   release_rt(r);
		release_rt(s_soft);
		release_rt(s_heavy);
		release_rt(s_image);
		release_rt(s_masked);
		s_masked_ok = false;
		s_w = s_h = 0;
		s_have_image = false;
	}

	bool create_chain(int w, int h, DXGI_FORMAT desktop_format)
	{
		release_chain();
		s_desktop_format = desktop_format;

		if (!create_rt(s_desktop, w, h, desktop_format, false, true))
			return false;

		const DXGI_FORMAT F = DXGI_FORMAT_R8G8B8A8_UNORM;
		for (int k = 0; k < 6; ++k)
			if (!create_rt(s_down[k], (w >> (k + 1)) > 0 ? (w >> (k + 1)) : 1,
				(h >> (k + 1)) > 0 ? (h >> (k + 1)) : 1, F, true, true))
				return false;
		for (int k = 0; k < 5; ++k)
			if (!create_rt(s_up[k], s_down[k].w, s_down[k].h, F, true, true))
				return false;

		if (!create_rt(s_soft,   w, h, F, true, true)) return false;
		if (!create_rt(s_heavy,  w, h, F, true, true)) return false;
		if (!create_rt(s_image,  w, h, F, true, true)) return false;

		if (!create_rt(s_masked, w, h, desktop_format, true, true)) return false;

		s_w = w;
		s_h = h;
		return true;
	}

	void release_duplication()
	{
		if (s_dupl)    { s_dupl->Release();    s_dupl = nullptr; }
		if (s_output1) { s_output1->Release(); s_output1 = nullptr; }
	}

	void release_all()
	{
		release_duplication();
		release_chain();
		if (s_depth)          { s_depth->Release();          s_depth = nullptr; }
		if (s_raster_scissor) { s_raster_scissor->Release(); s_raster_scissor = nullptr; }
		if (s_raster)         { s_raster->Release();         s_raster = nullptr; }
		if (s_blend)          { s_blend->Release();          s_blend = nullptr; }
		if (s_sampler)        { s_sampler->Release();        s_sampler = nullptr; }
		if (s_cb)             { s_cb->Release();             s_cb = nullptr; }
		if (s_ps_mix)         { s_ps_mix->Release();         s_ps_mix = nullptr; }
		if (s_ps_fill)        { s_ps_fill->Release();        s_ps_fill = nullptr; }
		if (s_ps_up)          { s_ps_up->Release();          s_ps_up = nullptr; }
		if (s_ps_down)        { s_ps_down->Release();        s_ps_down = nullptr; }
		if (s_vs)             { s_vs->Release();             s_vs = nullptr; }
	}

	bool compile_passes()
	{
		ID3DBlob* vs = nullptr;
		ID3DBlob* down = nullptr;
		ID3DBlob* up = nullptr;
		ID3DBlob* fill = nullptr;
		ID3DBlob* mix = nullptr;
		ID3DBlob* err = nullptr;

		const size_t len = std::strlen(kHLSL);
		if (FAILED(D3DCompile(kHLSL, len, "BackdropVS",   nullptr, nullptr, "VSFull", "vs_5_0", 0, 0, &vs,   &err))) goto fail;
		if (FAILED(D3DCompile(kHLSL, len, "BackdropDown", nullptr, nullptr, "PSDown", "ps_5_0", 0, 0, &down, &err))) goto fail;
		if (FAILED(D3DCompile(kHLSL, len, "BackdropUp",   nullptr, nullptr, "PSUp",   "ps_5_0", 0, 0, &up,   &err))) goto fail;
		if (FAILED(D3DCompile(kHLSL, len, "BackdropFill", nullptr, nullptr, "PSFill", "ps_5_0", 0, 0, &fill, &err))) goto fail;
		if (FAILED(D3DCompile(kHLSL, len, "BackdropMix",  nullptr, nullptr, "PSMix",  "ps_5_0", 0, 0, &mix,  &err))) goto fail;

		if (FAILED(s_device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &s_vs)) ||
			FAILED(s_device->CreatePixelShader(down->GetBufferPointer(), down->GetBufferSize(), nullptr, &s_ps_down)) ||
			FAILED(s_device->CreatePixelShader(up->GetBufferPointer(), up->GetBufferSize(), nullptr, &s_ps_up)) ||
			FAILED(s_device->CreatePixelShader(fill->GetBufferPointer(), fill->GetBufferSize(), nullptr, &s_ps_fill)) ||
			FAILED(s_device->CreatePixelShader(mix->GetBufferPointer(), mix->GetBufferSize(), nullptr, &s_ps_mix)))
			goto fail;

		vs->Release(); down->Release(); up->Release(); fill->Release(); mix->Release();
		return true;

	fail:
		if (err) { OutputDebugStringA((const char*)err->GetBufferPointer()); err->Release(); }
		if (vs)   vs->Release();
		if (down) down->Release();
		if (up)   up->Release();
		if (fill) fill->Release();
		if (mix)  mix->Release();
		return false;
	}

	bool create_pipeline()
	{
		if (!compile_passes())
			return false;

		D3D11_BUFFER_DESC bd{};
		bd.ByteWidth = 64;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(s_device->CreateBuffer(&bd, nullptr, &s_cb)))
			return false;

		D3D11_SAMPLER_DESC sd{};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sd.MaxLOD = D3D11_FLOAT32_MAX;
		if (FAILED(s_device->CreateSamplerState(&sd, &s_sampler)))
			return false;

		D3D11_BLEND_DESC bld{};
		bld.RenderTarget[0].BlendEnable = FALSE;
		bld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (FAILED(s_device->CreateBlendState(&bld, &s_blend)))
			return false;

		D3D11_RASTERIZER_DESC rd{};
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.DepthClipEnable = TRUE;
		if (FAILED(s_device->CreateRasterizerState(&rd, &s_raster)))
			return false;

		rd.ScissorEnable = TRUE;
		if (FAILED(s_device->CreateRasterizerState(&rd, &s_raster_scissor)))
			return false;

		D3D11_DEPTH_STENCIL_DESC dd{};
		dd.DepthEnable = FALSE;
		dd.StencilEnable = FALSE;
		if (FAILED(s_device->CreateDepthStencilState(&dd, &s_depth)))
			return false;

		return true;
	}

	struct pass_args_t
	{
		ID3D11ShaderResourceView* src = nullptr;
		ID3D11ShaderResourceView* aux = nullptr;
		int src_w = 0, src_h = 0;
		float spread = 1.f;
		float grain = 0.f;
		float blur_mix = 1.f;
		const float* fill_uv = nullptr;
		float fill_axis = 1.f;
	};

	void pass(ID3D11PixelShader* ps, const rt_t& dst, const pass_args_t& a)
	{
		ID3D11ShaderResourceView* no_srv[2] = { nullptr, nullptr };
		s_ctx->PSSetShaderResources(0, 2, no_srv);
		s_ctx->OMSetRenderTargets(1, &dst.rtv, nullptr);

		D3D11_VIEWPORT vp{};
		vp.Width = (float)dst.w;
		vp.Height = (float)dst.h;
		vp.MaxDepth = 1.f;
		s_ctx->RSSetViewports(1, &vp);

		D3D11_MAPPED_SUBRESOURCE m{};
		if (SUCCEEDED(s_ctx->Map(s_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)))
		{
			float* f = (float*)m.pData;
			f[0]  = a.src_w > 0 ? 1.f / (float)a.src_w : 0.f;
			f[1]  = a.src_h > 0 ? 1.f / (float)a.src_h : 0.f;
			f[2]  = a.spread;
			f[3]  = 0.f;
			f[4]  = (float)a.src_w;
			f[5]  = (float)a.src_h;
			f[6]  = 0.f;
			f[7]  = 0.f;
			f[8]  = a.fill_uv ? a.fill_uv[0] : 0.f;
			f[9]  = a.fill_uv ? a.fill_uv[1] : 0.f;
			f[10] = a.fill_uv ? a.fill_uv[2] : 0.f;
			f[11] = a.fill_axis;
			f[12] = a.blur_mix;
			f[13] = a.grain;
			f[14] = 0.f;
			f[15] = 0.f;
			s_ctx->Unmap(s_cb, 0);
		}

		ID3D11ShaderResourceView* srvs[2] = { a.src, a.aux };
		s_ctx->VSSetShader(s_vs, nullptr, 0);
		s_ctx->PSSetShader(ps, nullptr, 0);
		s_ctx->PSSetConstantBuffers(0, 1, &s_cb);
		s_ctx->PSSetSamplers(0, 1, &s_sampler);
		s_ctx->PSSetShaderResources(0, 2, srvs);
		s_ctx->Draw(3, 0);
	}

	void begin_draws(ID3D11RasterizerState* raster)
	{
		const float bf[4] = { 0.f, 0.f, 0.f, 0.f };
		s_ctx->IASetInputLayout(nullptr);
		s_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		s_ctx->OMSetBlendState(s_blend, bf, 0xffffffff);
		s_ctx->OMSetDepthStencilState(s_depth, 0);
		s_ctx->RSSetState(raster);
	}

	void end_draws()
	{
		ID3D11ShaderResourceView* no_srv[2] = { nullptr, nullptr };
		s_ctx->PSSetShaderResources(0, 2, no_srv);
		ID3D11RenderTargetView* no_rtv = nullptr;
		s_ctx->OMSetRenderTargets(1, &no_rtv, nullptr);
	}

	float measure_scale()
	{
		RECT rc{};
		if (!s_hwnd || !s_w || !::GetClientRect(s_hwnd, &rc) || rc.right <= 0)
			return 1.f;
		const float k = (float)s_w / (float)rc.right;
		return (k > 0.99f && k < 1.01f) ? 1.f : k;
	}

	void mask_footprints()
	{
		if (!s_occ_n || !s_masked.rtv || !s_masked.w || !s_desktop.srv)
			return;

		s_ctx->CopyResource(s_masked.tex, s_desktop.tex);
		s_masked_ok = true;

		POINT origin{};
		if (!::ClientToScreen(s_hwnd, &origin))
			return;

		begin_draws(s_raster_scissor);

		for (int i = 0; i < s_occ_n; ++i)
		{
			const float x0f = (float)origin.x * s_scale + s_occ[i][0] * s_scale - (float)s_origin_x;
			const float y0f = (float)origin.y * s_scale + s_occ[i][1] * s_scale - (float)s_origin_y;
			const float x1f = (float)origin.x * s_scale + s_occ[i][2] * s_scale - (float)s_origin_x;
			const float y1f = (float)origin.y * s_scale + s_occ[i][3] * s_scale - (float)s_origin_y;

			D3D11_RECT sc{};
			sc.left   = (LONG)(x0f < 0.f ? 0.f : (x0f > (float)s_masked.w ? (float)s_masked.w : x0f));
			sc.top    = (LONG)(y0f < 0.f ? 0.f : (y0f > (float)s_masked.h ? (float)s_masked.h : y0f));
			sc.right  = (LONG)(x1f < 0.f ? 0.f : (x1f > (float)s_masked.w ? (float)s_masked.w : x1f));
			sc.bottom = (LONG)(y1f < 0.f ? 0.f : (y1f > (float)s_masked.h ? (float)s_masked.h : y1f));
			if (sc.right <= sc.left || sc.bottom <= sc.top)
				continue;

			const float margin   = 6.f;
			const float min_band = 40.f;
			const float ix0 = x0f, iy0 = y0f, ix1 = x1f, iy1 = y1f;

			struct band_t { float edge, own, room; float axis; };
			band_t best{};
			float  best_score = -1.f;

			band_t fallback{};
			int    fallback_side = -1;
			float  fallback_room = -1.f;

			static int s_side[16] = { -1, -1, -1, -1, -1, -1, -1, -1,
			                          -1, -1, -1, -1, -1, -1, -1, -1 };
			const int  prev_side = (i < 16) ? s_side[i] : -1;
			float      side_score[4] = { -1.f, -1.f, -1.f, -1.f };
			band_t     side_band[4]{};

			for (int side = 0; side < 4; ++side)
			{
				band_t c{};
				c.axis = (side < 2) ? 1.f : 0.f;
				if (side == 0)      { c.edge = iy0 - margin; c.own = iy0; c.room = c.edge; }
				else if (side == 1) { c.edge = iy1 + margin; c.own = iy1; c.room = (float)s_h - c.edge; }
				else if (side == 2) { c.edge = ix0 - margin; c.own = ix0; c.room = c.edge; }
				else                { c.edge = ix1 + margin; c.own = ix1; c.room = (float)s_w - c.edge; }

				if (c.room < min_band)
				{
					if (c.room > fallback_room)
					{
						fallback_room = c.room;
						fallback_side = side;
						fallback      = c;
					}
					continue;
				}

				float bx0, by0, bx1, by1;
				if (side == 0)      { bx0 = ix0; by0 = c.edge - c.room; bx1 = ix1; by1 = c.edge; }
				else if (side == 1) { bx0 = ix0; by0 = c.edge; bx1 = ix1; by1 = c.edge + c.room; }
				else if (side == 2) { bx0 = c.edge - c.room; by0 = iy0; bx1 = c.edge; by1 = iy1; }
				else                { bx0 = c.edge; by0 = iy0; bx1 = c.edge + c.room; by1 = iy1; }

				float score = c.room;
				for (int j = 0; j < s_occ_n; ++j)
				{
					if (j == i)
						continue;
					const float ox0 = (float)origin.x * s_scale + s_occ[j][0] * s_scale - (float)s_origin_x;
					const float oy0 = (float)origin.y * s_scale + s_occ[j][1] * s_scale - (float)s_origin_y;
					const float ox1 = (float)origin.x * s_scale + s_occ[j][2] * s_scale - (float)s_origin_x;
					const float oy1 = (float)origin.y * s_scale + s_occ[j][3] * s_scale - (float)s_origin_y;
					if (ox1 <= bx0 + 2.f || ox0 >= bx1 - 2.f || oy1 <= by0 + 2.f || oy0 >= by1 - 2.f)
						continue;
					score -= 1.0e6f;
				}

				if (score > side_score[side])
				{
					side_score[side] = score;
					side_band[side]  = c;
				}
			}

			int pick = -1;
			for (int side = 0; side < 4; ++side)
				if (side_score[side] > best_score)
				{
					best_score = side_score[side];
					pick = side;
				}

			if (pick < 0 && fallback_side >= 0 && fallback.room > 0.f)
			{
				side_score[fallback_side] = fallback.room;
				side_band[fallback_side]  = fallback;
				best_score = fallback.room;
				pick = fallback_side;
			}

			if (prev_side >= 0 && prev_side != pick && side_score[prev_side] > 0.f &&
				best_score <= side_score[prev_side] * 1.25f)
			{
				pick = prev_side;
				best_score = side_score[prev_side];
			}

			if (pick < 0 || best_score < 0.f)
				continue;
			best = side_band[pick];
			if (i < 16)
				s_side[i] = pick;

			const float fw = (best.axis > 0.5f) ? (float)s_h : (float)s_w;
			float fill_uv[3];
			fill_uv[0] = best.edge / fw;
			fill_uv[1] = best.own / fw;
			fill_uv[2] = best.room / fw;

			s_ctx->RSSetScissorRects(1, &sc);
			pass_args_t a;
			a.src = s_desktop.srv;
			a.src_w = s_desktop.w;
			a.src_h = s_desktop.h;
			a.fill_uv = fill_uv;
			a.fill_axis = best.axis;
			pass(s_ps_fill, s_masked, a);
		}

		s_ctx->RSSetScissorRects(0, nullptr);
		end_draws();
	}

	void run_chain()
	{
		if (!s_ps_down || !s_desktop.srv || !s_down[0].rtv || !s_image.rtv)
			return;

		if (s_occ_n && !(s_masked_ok && s_masked.srv))
			return;

		const float blur  = s_params.blur  < 0.f ? 0.f : (s_params.blur  > 100.f ? 100.f : s_params.blur);
		const float frost = s_params.frost < 0.f ? 0.f : (s_params.frost > 100.f ? 100.f : s_params.frost);

		const int   rungs  = 2 + (int)(blur / 25.f + 0.5f);
		const float spread = 0.55f + blur * 0.019f;

		begin_draws(s_raster);

		rt_t& src = (s_masked_ok && s_masked.srv) ? s_masked : s_desktop;

		pass_args_t a;
		a.src = src.srv;
		a.src_w = src.w;
		a.src_h = src.h;
		a.spread = spread;
		pass(s_ps_down, s_down[0], a);
		for (int k = 1; k < rungs; ++k)
		{
			a.src = s_down[k - 1].srv;
			a.src_w = s_down[k - 1].w;
			a.src_h = s_down[k - 1].h;
			pass(s_ps_down, s_down[k], a);
		}
		a.src = s_down[rungs - 1].srv;
		a.src_w = s_down[rungs - 1].w;
		a.src_h = s_down[rungs - 1].h;
		pass(s_ps_up, s_up[rungs - 2], a);
		for (int k = rungs - 3; k >= 0; --k)
		{
			a.src = s_up[k + 1].srv;
			a.src_w = s_up[k + 1].w;
			a.src_h = s_up[k + 1].h;
			pass(s_ps_up, s_up[k], a);
		}
		a.src = s_up[0].srv;
		a.src_w = s_up[0].w;
		a.src_h = s_up[0].h;
		pass(s_ps_up, s_heavy, a);

		a.src = src.srv;
		a.src_w = src.w;
		a.src_h = src.h;
		pass(s_ps_up, s_soft, a);

		pass_args_t mix;
		mix.src = s_soft.srv;
		mix.aux = s_heavy.srv;
		mix.src_w = s_soft.w;
		mix.src_h = s_soft.h;
		mix.blur_mix = 0.55f + blur * 0.0045f;
		mix.grain = frost * 0.0006f;
		pass(s_ps_mix, s_image, mix);

		end_draws();
	}

	bool create_duplication()
	{
		release_duplication();

		s_rate_at      = 0;
		s_rate_frames  = 0;
		s_caps_per_sec = 0.f;

		IDXGIDevice* dxgi = nullptr;
		if (FAILED(s_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi)))
			return false;

		IDXGIAdapter* adapter = nullptr;
		const HRESULT got_adapter = dxgi->GetAdapter(&adapter);
		dxgi->Release();
		if (FAILED(got_adapter) || !adapter)
			return false;

		RECT wr{};
		::GetWindowRect(s_hwnd, &wr);
		const POINT centre{ (wr.left + wr.right) / 2, (wr.top + wr.bottom) / 2 };

		IDXGIOutput* chosen = nullptr;
		DXGI_OUTPUT_DESC chosen_desc{};
		for (UINT i = 0;; ++i)
		{
			IDXGIOutput* output = nullptr;
			if (adapter->EnumOutputs(i, &output) == DXGI_ERROR_NOT_FOUND)
				break;

			DXGI_OUTPUT_DESC desc{};
			output->GetDesc(&desc);
			const RECT& box = desc.DesktopCoordinates;
			const bool inside = centre.x >= box.left && centre.x < box.right &&
			                    centre.y >= box.top  && centre.y < box.bottom;
			if (inside || !chosen)
			{
				if (chosen)
					chosen->Release();
				chosen = output;
				chosen_desc = desc;
				if (inside)
					break;
			}
			else
			{
				output->Release();
			}
		}
		adapter->Release();

		if (!chosen)
			return false;

		HRESULT hr = chosen->QueryInterface(__uuidof(IDXGIOutput1), (void**)&s_output1);
		chosen->Release();
		if (FAILED(hr) || !s_output1)
		{
			s_last_hr = (int)hr;
			release_duplication();
			return false;
		}

		hr = s_output1->DuplicateOutput(s_device, &s_dupl);
		if (FAILED(hr) || !s_dupl)
		{

			s_last_hr = (int)hr;
			release_duplication();
			return false;
		}

		s_origin_x = chosen_desc.DesktopCoordinates.left;
		s_origin_y = chosen_desc.DesktopCoordinates.top;
		s_last_hr = 0;
		s_dup_fail = 0;
		return true;
	}
}

bool start(ID3D11Device* device, ID3D11DeviceContext* context, HWND overlay_window)
{
	if (s_started)
		return true;
	if (!device || !context || !overlay_window)
		return false;

	s_device = device;
	s_ctx    = context;
	s_hwnd   = overlay_window;

	if (!create_pipeline())
	{
		release_all();
		s_device = nullptr;
		s_ctx = nullptr;
		s_hwnd = nullptr;
		return false;
	}

	s_started = true;
	return true;
}

void stop()
{
	release_all();
	s_device = nullptr;
	s_ctx = nullptr;
	s_hwnd = nullptr;
	s_started = false;
	s_have_image = false;
	s_occ_n = 0;
	s_self_in_capture = false;
	s_masked_ok = false;
	s_scale = 1.f;
	s_frames = 0;
	s_dup_fail = 0;
	s_retry_at = 0;
	s_last_hr = 0;
}

bool started()
{
	return s_started;
}

void set_params(const params_t& params)
{
	s_params = params;
}

params_t params()
{
	return s_params;
}

void set_capture_has_self(bool in_capture)
{
	s_self_in_capture = in_capture;
}

void set_occlusion(const float* rects_xyxy, int count)
{
	s_occ_n = 0;
	if (!rects_xyxy)
		return;
	if (count > 8)
		count = 8;

	for (int i = 0; i < count; ++i)
	{
		const float* r = rects_xyxy + i * 4;
		if (!(r[2] > r[0]) || !(r[3] > r[1]))
			continue;
		for (int k = 0; k < 4; ++k)
			s_occ[s_occ_n][k] = r[k];
		++s_occ_n;
	}
}

bool update()
{
	if (!s_started || !s_device)
		return false;

	if (!s_dupl)
	{
		if (::GetTickCount64() < s_retry_at)
			return s_have_image;
		if (!create_duplication())
		{
			++s_dup_fail;
			s_retry_at = ::GetTickCount64() + 4000;
			return s_have_image;
		}
	}

	IDXGIResource* resource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO info{};

	const UINT timeout = s_have_image ? 0u : 60u;

	const HRESULT hr = s_dupl->AcquireNextFrame(timeout, &info, &resource);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		return s_have_image;
	if (hr == DXGI_ERROR_ACCESS_LOST)
	{

		release_duplication();
		s_retry_at = ::GetTickCount64() + 1000;
		return s_have_image;
	}
	if (FAILED(hr))
	{
		s_last_hr = (int)hr;
		release_duplication();
		s_retry_at = ::GetTickCount64() + 4000;
		return s_have_image;
	}

	ID3D11Texture2D* frame = nullptr;
	if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frame)))
	{
		D3D11_TEXTURE2D_DESC desc{};
		frame->GetDesc(&desc);

		const bool sized = (s_w == (int)desc.Width && s_h == (int)desc.Height);
		if ((!sized && create_chain((int)desc.Width, (int)desc.Height, desc.Format)) ||
			(sized && s_desktop.tex))
		{

			const bool moved = (info.LastPresentTime.QuadPart != 0) || (info.AccumulatedFrames != 0) || !s_have_image;
			if (moved)
			{

				s_ctx->CopyResource(s_desktop.tex, frame);
				++s_frames;
				++s_rate_frames;
				{
					const unsigned long long now = ::GetTickCount64();
					if (s_rate_at == 0)
						s_rate_at = now;
					else if (now - s_rate_at >= 1000)
					{
						s_caps_per_sec = (float)s_rate_frames * 1000.f / (float)(now - s_rate_at);
						s_rate_frames  = 0;
						s_rate_at      = now;
					}
				}

				s_scale = measure_scale();
				s_masked_ok = false;
				if (s_occ_n)
					mask_footprints();
				run_chain();

				s_have_image = true;
			}
		}
		frame->Release();
	}

	resource->Release();
	s_dupl->ReleaseFrame();
	return s_have_image;
}

bool ready()
{
	return s_have_image;
}

void* image()
{
	return s_have_image ? (void*)s_image.srv : nullptr;
}

float image_w()     { return (float)s_w; }
float image_h()     { return (float)s_h; }
float origin_x()    { return (float)s_origin_x; }
float origin_y()    { return (float)s_origin_y; }
float image_scale() { return s_scale; }

static const char* mask_state()
{
	if (!s_occ_n)
		return "idle";
	if (s_masked_ok)
		return "on";
	return "failed";
}

const char* status_key()
{
	static char line[192];

	if (!s_started)
		return "off (nothing started)";

	if (!s_have_image)
	{
		if (s_dup_fail > 0)
			snprintf(line, sizeof line, "no duplication (0x%08lX, %d attempt%s) - another capture?",
				(unsigned long)s_last_hr, s_dup_fail, s_dup_fail == 1 ? "" : "s");
		else
			snprintf(line, sizeof line, "starting (waiting for the desktop's first frame)");
		return line;
	}

	snprintf(line, sizeof line, "live %dx%d %s self=%s mask=%s scale=%.2f blur=%.0f frost=%.0f",
		s_w, s_h,
		s_desktop_format == DXGI_FORMAT_B8G8R8A8_UNORM ? "bg8" : "rg8",
		s_self_in_capture ? "in-capture" : "excluded",
		mask_state(),
		s_scale, s_params.blur, s_params.frost);
	return line;
}

const char* status()
{
	static char line[256];

	if (!s_started || !s_have_image)
		return status_key();

	snprintf(line, sizeof line, "live %dx%d %s frames=%u rects=%d self=%s mask=%s scale=%.2f blur=%.0f frost=%.0f caps=%.0f/s",
		s_w, s_h,
		s_desktop_format == DXGI_FORMAT_B8G8R8A8_UNORM ? "bg8" : "rg8",
		s_frames, s_occ_n,
		s_self_in_capture ? "in-capture" : "excluded",
		mask_state(),
		s_scale, s_params.blur, s_params.frost, s_caps_per_sec);
	return line;
}
}
