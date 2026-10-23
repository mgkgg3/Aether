#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "SkyShader.h"
#include "SkyVideo.h"

#include <debug/DebugConsole.h>
#include <game/game.h>
#include <memory/memory.h>
#include <sdk/math/math.h>
#include <sdk/offsets.h>
#include <sdk/sdk.h>
#include <settings.h>

#include <cstdarg>
#include <cstdio>
#include <thread>

#include <Windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#ifndef CFG_CALL_TARGET_VALID
#define CFG_CALL_TARGET_VALID 0x00000001
#endif

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace skyshader {
namespace {

std::uint64_t mod_base_of(const wchar_t* name)
{
	std::string n;
	if (name)
		for (const wchar_t* p = name; *p; ++p)
			n.push_back((char)*p);
	return n.empty() ? 0 : memory->find_module_address(n);
}

bool mem_valid(std::uint64_t a)
{
	return a >= 0x10000ull && a < 0x00007FFFFFFFFFFFull;
}

bool mem_attached()
{
	const HANDLE h = memory->get_process_handle();
	return h && h != INVALID_HANDLE_VALUE;
}

bool mem_alive()
{
	const HANDLE h = memory->get_process_handle();
	return h && h != INVALID_HANDLE_VALUE && WaitForSingleObject(h, 0) == WAIT_TIMEOUT;
}

template <typename T>
bool mem_write(std::uint64_t at, T v)
{
	memory->write<T>(at, v);
	T back{};
	return memory->read_raw(at, &back, sizeof(T)) == sizeof(T) &&
		std::memcmp(&back, &v, sizeof(T)) == 0;
}

bool mem_protect(std::uint64_t at, std::size_t n, DWORD prot, DWORD* old)
{
	const HANDLE h = memory->get_process_handle();
	return h && h != INVALID_HANDLE_VALUE &&
		VirtualProtectEx(h, (void*)at, n, prot, old) != 0;
}

void logpick(bool good, const char* fmt, ...)
{
	char buf[512];
	va_list a;
	va_start(a, fmt);
	vsnprintf(buf, sizeof buf, fmt, a);
	va_end(a);
	if (good) debug::ok("%s", buf);
	else      debug::error("%s", buf);
}

constexpr char k_hlsl[] = R"HLSL(
cbuffer SkyCB : register(b13)
{
    row_major float4x4 inv_vp : packoffset(c0);
    float3 camera : packoffset(c4);
    float  time : packoffset(c4.w);
    float2 res : packoffset(c5);
    float  mode : packoffset(c5.z);
    float  pad0 : packoffset(c5.w);
    float4 tint : packoffset(c6);
};

Texture2D vid : register(t13);
SamplerState smp : register(s13);

float hash(float n) { return frac(sin(n) * 43758.5453); }

float noise(float3 x)
{
    float3 p = floor(x);
    float3 f = frac(x);
    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 57.0 + p.z * 113.0;
    return lerp(lerp(lerp(hash(n), hash(n + 1.0), f.x),
                     lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
               lerp(lerp(hash(n + 113.0), hash(n + 114.0), f.x),
                     lerp(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

float fbm(float3 p)
{
    float a = 0.0, w = 0.5;
    [unroll] for (int i = 0; i < 4; ++i)
    {
        a += w * noise(p);
        p = p * 2.03 + 0.15;
        w *= 0.5;
    }
    return a;
}

float liquid(float3 d, float t)
{
    float3 q = d * 2.35;
    q += t * float3(0.055, 0.028, -0.042);
    float3 w = float3(fbm(q), fbm(q + 17.2), fbm(q + 31.7));
    q += (w - 0.5) * 1.05;
    w = float3(fbm(q * 1.55 + t * 0.035), fbm(q * 1.55 + 9.4), fbm(q * 1.55 + 21.1));
    q += (w - 0.5) * 0.55;
    return fbm(q);
}

float3 ramp(float u)
{
    u = saturate(u);
    float3 c0 = float3(0.06, 0.10, 0.28);
    float3 c1 = float3(0.08, 0.24, 0.34);
    float3 c2 = float3(0.24, 0.12, 0.36);
    float3 c3 = float3(0.38, 0.12, 0.26);
    float3 c4 = float3(0.34, 0.18, 0.24);
    float3 c5 = float3(0.30, 0.22, 0.18);
    if (u < 0.20) return lerp(c0, c1, u / 0.20);
    if (u < 0.40) return lerp(c1, c2, (u - 0.20) / 0.20);
    if (u < 0.60) return lerp(c2, c3, (u - 0.40) / 0.20);
    if (u < 0.80) return lerp(c3, c4, (u - 0.60) / 0.20);
    return lerp(c4, c5, (u - 0.80) / 0.20);
}

float4 ps_main(float4 pos : SV_POSITION) : SV_TARGET
{
    float2 r = res;
    if (r.x < 2.0 || r.y < 2.0)
        r = float2(1920, 1080);
    float2 uv = pos.xy / r * float2(2, -2) + float2(-1, 1);
    float3 dir;
    if (dot(inv_vp[0], inv_vp[0]) < 1e-6)
        dir = normalize(float3(uv, 1));
    else
    {
        float4 n = mul(inv_vp, float4(uv, 0, 1));
        float4 f = mul(inv_vp, float4(uv, 1, 1));
        n.xyz /= max(abs(n.w), 1e-4);
        f.xyz /= max(abs(f.w), 1e-4);
        dir = normalize(f.xyz - n.xyz);
    }

    float t = time;
    int im = (int)(mode + 0.5);
    float3 col;
    if (im == 3)
    {
        float2 vu = pos.xy / r;
        if (vu.x < 0.0 || vu.x > 1.0 || vu.y < 0.0 || vu.y > 1.0)
            return float4(0, 0, 0, 1);
        float3 c = saturate(vid.SampleLevel(smp, vu, 0).rgb);
        float w = max(c.r, max(c.g, c.b));
        c *= 0.42 / (0.42 + w);
        return float4(c, 1);
    }
    else if (im == 1)
    {
        float h = liquid(dir, t);
        float hx = liquid(normalize(dir + float3(0.012, 0, 0)), t);
        float hy = liquid(normalize(dir + float3(0, 0.012, 0)), t);
        float g = length(float2(hx - h, hy - h));
        col = ramp(h * 0.72 + 0.10 + 0.04 * sin(t * 0.18));
        col += pow(saturate(g * 16.0), 7.0) * 0.16;
        float ir = 0.5 + 0.5 * sin(h * 6.2 + t * 0.14);
        col = lerp(col, col.bgr * float3(0.95, 0.88, 1.02), ir * 0.10);
        col = lerp(float3(0.08, 0.10, 0.20), col, 0.84);
    }
    else if (im == 2)
    {
        float az = atan2(dir.z, dir.x);
        float el = dir.y;
        float curtain = 0.0;
        [unroll] for (int i = 0; i < 4; ++i)
        {
            float a = az * (2.15 + i * 0.52) + t * (0.20 + i * 0.045);
            float wave = sin(a) + 0.42 * sin(a * 1.65 + t * 0.14 + i);
            float y = (el - 0.42) * 1.85 - wave * 0.22 - i * 0.05;
            float sheet = exp(-y * y * (6.2 + i * 2.0));
            float hgt = saturate((el - 0.10) * 4.2) * saturate(1.2 - el * 0.85);
            curtain += sheet * hgt * (0.58 - i * 0.08);
        }
        float n = fbm(float3(dir.x, el, dir.z) * 2.35 + t * 0.055);
        float n2 = fbm(dir * 4.1 - t * 0.04);
        float rays = pow(saturate(curtain * (0.72 + 0.48 * n)), 1.08);
        float dome = saturate((el - 0.14) * 1.7);
        col = float3(0.04, 0.06, 0.13);
        col = lerp(col, float3(0.05, 0.22, 0.16), dome * 0.55);
        col = lerp(col, float3(0.05, 0.42, 0.22), saturate(rays * 1.55));
        col = lerp(col, float3(0.16, 0.76, 0.44), saturate(rays * n * 1.85));
        col = lerp(col, float3(0.32, 0.86, 0.80), saturate(rays * n2 * 1.05) * saturate(el * 1.2));
        col += float3(0.64, 0.16, 0.46) * saturate(curtain * 0.7) * saturate(el - 0.18) * saturate(0.55 - el) * (0.5 + 0.5 * n);
    }
    else
    {
        float cs = cos(t * 0.42), sn = sin(t * 0.42);
        float3 p = float3(dir.x * cs + dir.z * sn, dir.y, -dir.x * sn + dir.z * cs);
        p += 0.30 * sin(float3(p.y, p.z, p.x) * 1.62 + t * 0.68);
        p += 0.14 * sin(float3(p.z, p.x, p.y) * 2.35 - t * 0.51);
        float3 k = float3(
            0.502 + 0.048 * sin(t * 0.37),
            0.478 + 0.044 * sin(t * 0.51 + 2.1),
            0.508 + 0.036 * cos(t * 0.29 + 4.2));
        float acc = 0.0;
        float rim = 10.0;
        [unroll] for (int i = 0; i < 5; ++i)
        {
            p = abs(p);
            float d2 = dot(p, p);
            p = p / max(d2, 1e-4) - k;
            float l = length(p);
            rim = min(rim, l);
            acc += exp(-l * 2.15);
        }
        float a = saturate(acc * 0.155);
        float shell = saturate(a * 1.35) * saturate(1.55 - acc * 0.09);
        col = lerp(float3(0.05, 0.06, 0.14), float3(0.12, 0.13, 0.30), saturate(shell * 1.8));
        col = lerp(col, float3(0.58, 0.52, 0.92), saturate(shell * 1.2 - 0.15));
        col += float3(0.86, 0.88, 1.0) * saturate(0.55 - rim) * shell * 0.85;
    }

    float3 mulc = tint.rgb;
    if (dot(mulc, mulc) < 1e-6)
        mulc = 1;
    col = max(col * mulc, float3(0.04, 0.045, 0.07));
    return float4(saturate(col), 1);
}
)HLSL";

#pragma pack(push, 8)
struct SkyCB {
	float inv_vp[16];
	float cam[3];
	float time;
	float res[2];
	float mode;
	float pad0;
	float tint[4];
};

struct RemoteState {
	std::uint32_t enabled;
	std::uint32_t next_sky;
	std::uint64_t our_ps;
	std::uint64_t orig_psset;
	std::uint32_t creating;
	std::uint32_t maps;
	std::uint64_t cb;
	std::uint64_t map_fn;
	std::uint64_t pscb;
	std::uint64_t unmap_fn;
	SkyCB cbdata;
	std::uint64_t create_ps_fn;
	std::uint64_t device;
	std::uint64_t dxbc;
	std::uint64_t dxbc_len;
	std::uint64_t out_ps;
	std::uint64_t create_buf_fn;
	std::uint64_t orig_draw;
	std::uint64_t orig_drawidx;
	std::uint32_t entered_ps;
	std::uint32_t entered_draw;
	std::uint32_t entered_idx;
	std::uint32_t dirty;
	std::uint32_t last_map;
	std::uint32_t pad104;
	std::uint64_t tick_fn;
	std::uint64_t cb2;
	std::uint64_t cb3;
	std::uint64_t tex;
	std::uint64_t srv;
	std::uint64_t samp;
	std::uint64_t pix;
	std::uint32_t pix_w;
	std::uint32_t pix_h;
	std::uint32_t pix_stride;
	std::uint32_t tex_dirty;
	std::uint64_t create_tex_fn;
	std::uint64_t create_srv_fn;
	std::uint64_t create_samp_fn;
	std::uint64_t ps_srv_fn;
	std::uint64_t ps_samp_fn;
	std::uint64_t orig_drawinst;
	std::uint64_t orig_drawidxinst;
	std::uint32_t vid_bound;
	std::uint32_t vid_do_bind;
	std::uint32_t sky_draw;
	std::uint32_t srv_hits;
	std::uint32_t vid_calls;
	std::uint32_t pad19c;
};
#pragma pack(pop)

static_assert(offsetof(RemoteState, creating) == 0x18);
static_assert(offsetof(RemoteState, cb) == 0x20);
static_assert(offsetof(RemoteState, maps) == 0x1C);
static_assert(offsetof(RemoteState, map_fn) == 0x28);
static_assert(offsetof(RemoteState, pscb) == 0x30);
static_assert(offsetof(RemoteState, unmap_fn) == 0x38);
static_assert(offsetof(RemoteState, cbdata) == 0x40);
static_assert(offsetof(RemoteState, create_ps_fn) == 0xB0);
static_assert(offsetof(RemoteState, create_buf_fn) == 0xD8);
static_assert(offsetof(RemoteState, orig_draw) == 0xE0);
static_assert(offsetof(RemoteState, orig_drawidx) == 0xE8);
static_assert(offsetof(RemoteState, entered_ps) == 0xF0);
static_assert(offsetof(RemoteState, entered_draw) == 0xF4);
static_assert(offsetof(RemoteState, entered_idx) == 0xF8);
static_assert(offsetof(RemoteState, dirty) == 0xFC);
static_assert(offsetof(RemoteState, last_map) == 0x100);
static_assert(offsetof(RemoteState, tick_fn) == 0x108);
static_assert(offsetof(RemoteState, cb2) == 0x110);
static_assert(offsetof(RemoteState, cb3) == 0x118);
static_assert(offsetof(RemoteState, tex) == 0x120);
static_assert(offsetof(RemoteState, srv) == 0x128);
static_assert(offsetof(RemoteState, samp) == 0x130);
static_assert(offsetof(RemoteState, pix) == 0x138);
static_assert(offsetof(RemoteState, pix_w) == 0x140);
static_assert(offsetof(RemoteState, create_tex_fn) == 0x150);
static_assert(offsetof(RemoteState, orig_drawinst) == 0x178);
static_assert(offsetof(RemoteState, vid_bound) == 0x188);
static_assert(offsetof(RemoteState, sky_draw) == 0x190);
static_assert(offsetof(RemoteState, srv_hits) == 0x194);
static_assert(offsetof(RemoteState, vid_calls) == 0x198);
static_assert(offsetof(RemoteState, ps_srv_fn) == 0x168);
static_assert(sizeof(SkyCB) == 0x70);

struct Patch {
	std::uintptr_t at = 0;
	std::uint64_t old = 0;
};

struct Hook {
	std::uintptr_t state = 0;
	std::uintptr_t cave = 0;
	std::uintptr_t psset_thunk = 0;
	std::uintptr_t create_stub = 0;
	std::uintptr_t upload_stub = 0;
	std::uintptr_t bind_stub = 0;
	std::uintptr_t draw_thunk = 0;
	std::uintptr_t drawidx_thunk = 0;
	std::uintptr_t drawinst_thunk = 0;
	std::uintptr_t drawidxinst_thunk = 0;
	std::uintptr_t srv_thunk = 0;
	std::size_t cave_n = 0;
	std::uintptr_t device = 0;
	std::uintptr_t ctx = 0;
	std::uintptr_t orig_psset = 0;
	std::uintptr_t video_stub = 0;
	std::uintptr_t pix = 0;
	std::uintptr_t module = 0;
	std::uintptr_t sky_lo = 0;
	std::uintptr_t sky_hi = 0;
	std::uintptr_t r_d3d11 = 0;
	std::uintptr_t dxbc = 0;
	Patch patches[48]{};
	int npatch = 0;
	bool hooked = false;
	bool logged_ok = false;
	bool logged_hit = false;
	bool logged_cb = false;
	bool logged_vid = false;
	bool cfg_ps = false;
	bool cfg_create = false;
	bool cave_alloc = false;
};

struct Snap {
	bool hooked = false;
	int npatch = 0;
	bool cfg_ps = false;
	bool cfg_create = false;
	std::uint32_t entered_ps = 0;
	std::uint32_t entered_draw = 0;
	std::uint32_t entered_idx = 0;
	std::uint32_t swaps = 0;
	std::uintptr_t orig_psset = 0;
	std::uintptr_t orig_draw = 0;
	std::uintptr_t cave = 0;
	std::uintptr_t ctx = 0;
};

Hook g{};
Snap g_snap{};
ID3D11Device* g_local_dev = nullptr;
ID3D11DeviceContext* g_local_ctx = nullptr;
void** g_dev_vt = nullptr;
void** g_ctx_vt = nullptr;
HMODULE g_local_d3d11 = nullptr;
auto g_fail = std::chrono::steady_clock::time_point{};
const char* g_why = nullptr;

bool addr_ok(std::uintptr_t a)
{
	return a >= 0x10000ull && a < 0x00007FFFFFFFFFFFull;
}

void append_u64(std::vector<std::uint8_t>& c, std::uint64_t v)
{
	const auto* b = (const std::uint8_t*)&v;
	c.insert(c.end(), b, b + 8);
}

void append_call_video_if(std::vector<std::uint8_t>& c, std::uintptr_t state, std::uintptr_t video, std::uint32_t do_bind)
{
	if (!video)
		return;
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x83, 0xB8, 0x40, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x74, 0x1B });
	c.insert(c.end(), { 0xC7, 0x80, 0x8C, 0x01, 0x00, 0x00,
		(std::uint8_t)do_bind, 0, 0, 0 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, video);
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0xFF, 0xD0 });
}

void append_blend_off(std::vector<std::uint8_t>& c)
{
	c.insert(c.end(), { 0x31, 0xD2 });
	c.insert(c.end(), { 0xB8, 0x00, 0x00, 0x80, 0x3F });
	c.insert(c.end(), { 0x89, 0x44, 0x24, 0x20 });
	c.insert(c.end(), { 0x89, 0x44, 0x24, 0x24 });
	c.insert(c.end(), { 0x89, 0x44, 0x24, 0x28 });
	c.insert(c.end(), { 0x89, 0x44, 0x24, 0x2C });
	c.insert(c.end(), { 0x4C, 0x8D, 0x44, 0x24, 0x20 });
	c.insert(c.end(), { 0x41, 0xB9, 0xFF, 0xFF, 0xFF, 0xFF });
	c.insert(c.end(), { 0x48, 0x8B, 0x01 });
	c.insert(c.end(), { 0xFF, 0x90, 0x18, 0x01, 0x00, 0x00 });
}

void snapshot()
{
	g_snap.hooked = g.hooked;
	g_snap.npatch = g.npatch;
	g_snap.cfg_ps = g.cfg_ps;
	g_snap.cfg_create = g.cfg_create;
	g_snap.orig_psset = g.orig_psset;
	g_snap.cave = g.cave;
	g_snap.ctx = g.ctx;
	if (!g.state || !memory->get_process_handle())
		return;
	std::uint32_t v = 0;
	if (memory->read_raw(g.state + offsetof(RemoteState, entered_ps), &v, 4) == 4)
		g_snap.entered_ps = v;
	if (memory->read_raw(g.state + offsetof(RemoteState, entered_draw), &v, 4) == 4)
		g_snap.entered_draw = v;
	if (memory->read_raw(g.state + offsetof(RemoteState, entered_idx), &v, 4) == 4)
		g_snap.entered_idx = v;
	if (memory->read_raw(g.state + offsetof(RemoteState, next_sky), &v, 4) == 4)
		g_snap.swaps = v;
	std::uint64_t p = 0;
	if (memory->read_raw(g.state + offsetof(RemoteState, orig_draw), &p, 8) == 8)
		g_snap.orig_draw = (std::uintptr_t)p;
}

void fail(const char* why)
{
	if (g_why == why)
		return;
	g_why = why;
	debug::error("skyshader: %s", why);
}

bool invert(const float in[16], float out[16])
{
	float m[4][8]{};
	for (int r = 0; r < 4; ++r)
	{
		for (int c = 0; c < 4; ++c)
			m[r][c] = in[r * 4 + c];
		m[r][r + 4] = 1.f;
	}
	for (int i = 0; i < 4; ++i)
	{
		int piv = i;
		float best = fabsf(m[i][i]);
		for (int r = i + 1; r < 4; ++r)
		{
			const float v = fabsf(m[r][i]);
			if (v > best)
			{
				best = v;
				piv = r;
			}
		}
		if (best < 1e-8f)
			return false;
		if (piv != i)
		{
			for (int c = 0; c < 8; ++c)
			{
				const float t = m[i][c];
				m[i][c] = m[piv][c];
				m[piv][c] = t;
			}
		}
		const float inv = 1.f / m[i][i];
		for (int c = 0; c < 8; ++c)
			m[i][c] *= inv;
		for (int r = 0; r < 4; ++r)
		{
			if (r == i)
				continue;
			const float f = m[r][i];
			for (int c = 0; c < 8; ++c)
				m[r][c] -= f * m[i][c];
		}
	}
	for (int r = 0; r < 4; ++r)
		for (int c = 0; c < 4; ++c)
			out[r * 4 + c] = m[r][c + 4];
	return true;
}

bool mark_cfg(std::uintptr_t t)
{
	static FARPROC proc = nullptr;
	if (!proc)
	{
		const char* mods[] = {
			"kernelbase.dll", "kernel32.dll",
			"api-ms-win-core-memory-l1-1-3.dll"
		};
		for (auto* m : mods)
		{
			HMODULE h = GetModuleHandleA(m);
			if (!h)
				h = LoadLibraryA(m);
			if (!h)
				continue;
			proc = GetProcAddress(h, "SetProcessValidCallTargets");
			if (proc)
				break;
		}
	}
	if (!proc || !memory->get_process_handle())
		return false;

	SYSTEM_INFO si{};
	GetSystemInfo(&si);
	const std::size_t page = si.dwPageSize ? (std::size_t)si.dwPageSize : 0x1000u;

	struct Info {
		ULONG_PTR Offset;
		ULONG Flags;
	} info{};
	info.Offset = t & (page - 1);
	info.Flags = CFG_CALL_TARGET_VALID;

	using Fn = BOOL(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, void*);
	return ((Fn)proc)(
		memory->get_process_handle(),
		(void*)(t & ~((std::uintptr_t)page - 1)),
		page, 1, &info) != 0;
}

bool write_exec(std::uintptr_t at, const void* data, std::size_t n)
{
	if (!addr_ok(at) || !data || !n)
		return false;
	DWORD old = 0;
	const bool changed = mem_protect(at, n, PAGE_EXECUTE_READWRITE, &old);
	const bool ok = memory->write_raw(at, data, n) == n;
	if (changed)
		mem_protect(at, n, old, nullptr);
	return ok;
}

std::uintptr_t find_cave_in_module(std::uintptr_t mod, std::size_t need, std::uintptr_t min_off, std::uintptr_t ignore)
{
	IMAGE_DOS_HEADER dos{};
	if (memory->read_raw(mod, &dos, sizeof(dos)) != sizeof(dos) || dos.e_magic != IMAGE_DOS_SIGNATURE)
		return 0;
	IMAGE_NT_HEADERS64 nt{};
	const std::uintptr_t nt_at = mod + (std::uintptr_t)dos.e_lfanew;
	if (memory->read_raw(nt_at, &nt, sizeof(nt)) != sizeof(nt) || nt.Signature != IMAGE_NT_SIGNATURE)
		return 0;
	const std::uintptr_t sec_at = nt_at + offsetof(IMAGE_NT_HEADERS64, OptionalHeader) +
		nt.FileHeader.SizeOfOptionalHeader;
	for (WORD i = 0; i < nt.FileHeader.NumberOfSections; ++i)
	{
		IMAGE_SECTION_HEADER sec{};
		if (memory->read_raw(sec_at + (std::uintptr_t)i * sizeof(sec), &sec, sizeof(sec)) != sizeof(sec))
			break;
		if ((sec.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
			continue;
		const std::uintptr_t off = sec.VirtualAddress;
		const std::size_t size = sec.Misc.VirtualSize ? sec.Misc.VirtualSize : sec.SizeOfRawData;
		if (!off || size < need)
			continue;
		std::uintptr_t scan_off = off < min_off ? min_off : off;
		if (scan_off >= off + size)
			continue;
		const std::uintptr_t start = mod + scan_off;
		const std::size_t scan = (std::size_t)(off + size - scan_off);
		std::vector<std::uint8_t> buf(scan);
		if (memory->read_raw(start, buf.data(), buf.size()) != buf.size())
			continue;
		std::size_t run_start = 0;
		std::size_t run_len = 0;
		for (std::size_t j = 0; j < buf.size(); ++j)
		{
			const std::uint8_t b = buf[j];
			if (b != 0x00 && b != 0xCC && b != 0x90)
			{
				run_len = 0;
				run_start = j + 1;
				continue;
			}
			++run_len;
			if (run_len < need)
				continue;
			const std::uintptr_t cand = start + run_start;
			const std::uintptr_t aligned = (cand + 0x0F) & ~std::uintptr_t(0x0F);
			const std::size_t loss = (std::size_t)(aligned - cand);
			if (run_len < need + loss)
				continue;
			if (ignore && aligned == ignore)
				continue;
			auto hit = [&](std::uintptr_t p, std::size_t n)
			{
				return p && aligned < p + n && p < aligned + need;
			};
			if (hit(g.cave, g.cave_n) || hit(g.video_stub, 0x400) || hit(g.srv_thunk, 0xA0))
				continue;
			return aligned;
		}
	}
	return 0;
}

std::uintptr_t find_dll_cave(std::size_t need, std::uintptr_t ignore)
{
	static const wchar_t* pref[] = {
		L"winsta.dll", L"win32u.dll", L"uxtheme.dll", L"dwmapi.dll",
		L"msctf.dll", L"TextInputFramework.dll", L"CoreMessaging.dll",
	};
	for (std::size_t i = 0; i < sizeof(pref) / sizeof(pref[0]); ++i)
	{
		const std::uintptr_t mod = mod_base_of(pref[i]);
		if (!mod)
			continue;
		const std::uintptr_t cave = find_cave_in_module(mod, need, i == 0 ? 0x2000u : 0x1000u, ignore);
		if (cave)
			return cave;
	}
	return 0;
}

bool local_d3d()
{
	if (g_local_dev && g_local_ctx && g_local_d3d11)
		return true;
	const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	D3D_FEATURE_LEVEL got{};
	if (FAILED(D3D11CreateDevice(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
		levels, 2, D3D11_SDK_VERSION, &g_local_dev, &got, &g_local_ctx)))
	{
		if (FAILED(D3D11CreateDevice(
			nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
			levels, 2, D3D11_SDK_VERSION, &g_local_dev, &got, &g_local_ctx)))
			return false;
	}
	g_dev_vt = *(void***)g_local_dev;
	g_ctx_vt = *(void***)g_local_ctx;
	g_local_d3d11 = GetModuleHandleA("d3d11.dll");
	return g_dev_vt && g_ctx_vt && g_local_d3d11;
}

std::uintptr_t module_base()
{
	const std::uintptr_t cached = (std::uintptr_t)memory->get_module_address();
	if (cached)
		return cached;
	return mod_base_of(L"RobloxPlayerBeta.exe");
}

bool pe_sec(std::uintptr_t base, const char* name, std::uintptr_t* va, std::uint32_t* sz)
{
	IMAGE_DOS_HEADER dos{};
	if (memory->read_raw(base, &dos, sizeof(dos)) != sizeof(dos) || dos.e_magic != IMAGE_DOS_SIGNATURE)
		return false;
	IMAGE_NT_HEADERS64 nt{};
	if (memory->read_raw(base + dos.e_lfanew, &nt, sizeof(nt)) != sizeof(nt) || nt.Signature != IMAGE_NT_SIGNATURE)
		return false;
	const std::uintptr_t tab = base + dos.e_lfanew + 4 + sizeof(IMAGE_FILE_HEADER) + nt.FileHeader.SizeOfOptionalHeader;
	const int n = nt.FileHeader.NumberOfSections;
	for (int i = 0; i < n && i < 32; ++i)
	{
		IMAGE_SECTION_HEADER sh{};
		if (memory->read_raw(tab + (std::uintptr_t)i * sizeof(sh), &sh, sizeof(sh)) != sizeof(sh))
			break;
		char nm[9]{};
		std::memcpy(nm, sh.Name, 8);
		if (std::strcmp(nm, name) == 0)
		{
			*va = base + sh.VirtualAddress;
			*sz = sh.Misc.VirtualSize ? sh.Misc.VirtualSize : sh.SizeOfRawData;
			return *sz != 0;
		}
	}
	return false;
}

std::uintptr_t find_cstr(std::uintptr_t va, std::uint32_t sz, const char* s)
{
	const std::size_t n = std::strlen(s);
	if (!va || !sz || n == 0 || n > 64)
		return 0;
	std::uint32_t off = 0;
	while (off < sz)
	{
		const std::uint32_t take = (std::min)(0x100000u, sz - off);
		std::vector<std::uint8_t> buf(take);
		if (memory->read_raw(va + off, buf.data(), take) == take)
		{
			for (std::uint32_t i = 0; i + n < take; ++i)
			{
				if (std::memcmp(buf.data() + i, s, n) != 0 || buf[i + n] != 0)
					continue;
				if (i && (std::isalnum(buf[i - 1]) || buf[i - 1] == '_'))
					continue;
				return va + off + i;
			}
		}
		off += take > (std::uint32_t)n ? take - (std::uint32_t)n : take;
	}
	return 0;
}

std::uintptr_t find_str_mod(std::uintptr_t base, const char* s)
{
	std::uintptr_t va = 0;
	std::uint32_t sz = 0;
	if (pe_sec(base, ".rdata", &va, &sz))
	{
		if (const std::uintptr_t h = find_cstr(va, sz, s))
			return h;
	}
	if (pe_sec(base, ".data", &va, &sz))
		return find_cstr(va, sz, s);
	return 0;
}

void collect_slots(std::uintptr_t va, std::uint32_t sz, std::uintptr_t want, std::vector<std::uintptr_t>& out)
{
	if (!va || !sz || !want)
		return;
	std::uint32_t off = 0;
	while (off + 8 <= sz)
	{
		const std::uint32_t take = (std::min)(0x100000u, sz - off);
		std::vector<std::uint8_t> buf(take);
		if (memory->read_raw(va + off, buf.data(), take) == take)
		{
			for (std::uint32_t i = 0; i + 8 <= take; i += 8)
			{
				std::uintptr_t q = 0;
				std::memcpy(&q, buf.data() + i, 8);
				if (q == want)
					out.push_back(va + off + i);
			}
		}
		off += take;
	}
}

void index_rip_page(std::uintptr_t va, const std::uint8_t* t, std::size_t n, const std::uintptr_t* needles, int nn, std::vector<std::uintptr_t>* xs)
{
	static const std::uint8_t k_mod[] = { 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D };
	for (std::size_t i = 1; i + 6 <= n; ++i)
	{
		const std::uint8_t op = t[i];
		if (op != 0x8D && op != 0x8B && op != 0x89)
			continue;
		if ((t[i - 1] & 0xF0) != 0x40)
			continue;
		bool ripm = false;
		for (auto m : k_mod)
		{
			if (t[i + 1] == m)
			{
				ripm = true;
				break;
			}
		}
		if (!ripm)
			continue;
		std::int32_t rel = 0;
		std::memcpy(&rel, t + i + 2, 4);
		const std::uintptr_t absv = va + i - 1 + 7 + (std::uintptr_t)(std::int64_t)rel;
		for (int k = 0; k < nn; ++k)
		{
			if (needles[k] && absv == needles[k])
				xs[k].push_back(va + i - 1);
		}
	}
}

std::uintptr_t fn_start_at(std::uintptr_t ea, std::uintptr_t text, std::uint32_t tsz)
{
	if (!ea || ea < text || ea >= text + tsz)
		return ea;
	const std::uintptr_t lo = (ea > text + 0x400) ? ea - 0x400 : text;
	const std::size_t n = ea - lo;
	std::uint8_t buf[0x400]{};
	if (!n || n > sizeof(buf) || memory->read_raw(lo, buf, n) != n)
		return ea;
	int hit = -1;
	for (int i = (int)n - 1; i >= 0; --i)
	{
		bool pro = false;
		if (i + 4 <= (int)n && buf[i] == 0x48 && buf[i + 1] == 0x89 && buf[i + 2] == 0x5C && buf[i + 3] == 0x24)
			pro = true;
		if (i + 3 <= (int)n && buf[i] == 0x48 && buf[i + 1] == 0x83 && buf[i + 2] == 0xEC)
			pro = true;
		if (i + 3 <= (int)n && buf[i] == 0x48 && buf[i + 1] == 0x8B && buf[i + 2] == 0xC4)
			pro = true;
		if (!pro)
			continue;
		hit = i;
		if (i > 0 && buf[i - 1] == 0xCC)
			return lo + (std::uintptr_t)i;
	}
	return hit >= 0 ? lo + (std::uintptr_t)hit : ea;
}

bool near_any(std::uintptr_t ea, const std::vector<std::uintptr_t>& xs, std::uintptr_t rad)
{
	for (auto x : xs)
	{
		const std::uintptr_t d = ea > x ? ea - x : x - ea;
		if (d <= rad)
			return true;
	}
	return false;
}

bool resolve_sky_range(std::uintptr_t base, std::uintptr_t* lo, std::uintptr_t* hi)
{
	if (g.sky_lo && g.sky_hi)
	{
		*lo = g.sky_lo;
		*hi = g.sky_hi;
		return true;
	}

	std::uintptr_t text = 0, rdata = 0, data = 0;
	std::uint32_t tsz = 0, rsz = 0, dsz = 0;
	if (!pe_sec(base, ".text", &text, &tsz))
		return false;
	pe_sec(base, ".rdata", &rdata, &rsz);
	pe_sec(base, ".data", &data, &dsz);

	const char* names[] = { "SkyFS", "SkyVS", "AdvSkyFS", "SkyLinearVColorFS", "SkyRGBEFS", "BloomFS" };
	std::uintptr_t strs[6]{};
	for (int i = 0; i < 6; ++i)
		strs[i] = find_str_mod(base, names[i]);
	if (!strs[0])
		return false;

	std::vector<std::uintptr_t> slots;
	if (rdata)
		collect_slots(rdata, rsz, strs[0], slots);
	if (data)
		collect_slots(data, dsz, strs[0], slots);

	std::uintptr_t needles[32]{};
	int nn = 0;
	std::vector<std::uintptr_t> xs[32];
	int map[6]{};
	for (int i = 0; i < 6; ++i)
	{
		map[i] = -1;
		if (!strs[i])
			continue;
		map[i] = nn;
		needles[nn++] = strs[i];
	}
	const int slot0 = nn;
	for (auto s : slots)
	{
		if (nn >= 32)
			break;
		needles[nn++] = s;
	}

	std::uint8_t page[0x1000];
	for (std::uint32_t off = 0; off < tsz; off += 0x1000)
	{
		const std::uint32_t n = (std::min)(0x1000u, tsz - off);
		if (memory->read_raw(text + off, page, n) != n)
			continue;
		index_rip_page(text + off, page, n, needles, nn, xs);
	}

	std::vector<std::uintptr_t> skyfs = map[0] >= 0 ? xs[map[0]] : std::vector<std::uintptr_t>{};
	for (int i = slot0; i < nn; ++i)
		skyfs.insert(skyfs.end(), xs[i].begin(), xs[i].end());
	if (skyfs.empty())
		return false;

	auto list = [&](int name_i) -> const std::vector<std::uintptr_t>* {
		if (map[name_i] < 0)
			return nullptr;
		return &xs[map[name_i]];
	};

	std::uintptr_t setup = 0;
	int best = -100000;
	for (auto x : skyfs)
	{
		int sc = 0;
		if (const auto* v = list(1); v && near_any(x, *v, 0x800))
			sc += 60;
		if (const auto* v = list(3); v && near_any(x, *v, 0x800))
			sc += 50;
		if (const auto* v = list(4); v && near_any(x, *v, 0x800))
			sc += 50;
		if (const auto* v = list(2); v && near_any(x, *v, 0x4000))
			sc += 20;
		if (const auto* v = list(5); v && near_any(x, *v, 0x1000))
			sc -= 120;
		if (sc > best)
		{
			best = sc;
			setup = x;
		}
	}
	if (!setup || best < 40)
		return false;

	std::uintptr_t draw_adv = 0;
	if (const auto* adv = list(2); adv && !adv->empty())
	{
		std::uintptr_t ax = adv->front();
		std::uintptr_t ad = setup > ax ? setup - ax : ax - setup;
		for (auto x : *adv)
		{
			const std::uintptr_t d = setup > x ? setup - x : x - setup;
			if (d < ad)
			{
				ad = d;
				ax = x;
			}
		}
		if (ad < 0x40000)
			draw_adv = fn_start_at(ax, text, tsz);
	}

	std::uintptr_t clo = setup, chi = setup;
	auto widen = [&](int name_i, std::uintptr_t rad) {
		const auto* v = list(name_i);
		if (!v)
			return;
		for (auto x : *v)
		{
			const std::uintptr_t d = x > setup ? x - setup : setup - x;
			if (d > rad)
				continue;
			if (x < clo)
				clo = x;
			if (x > chi)
				chi = x;
		}
	};
	widen(1, 0x2000);
	widen(3, 0x2000);
	widen(4, 0x2000);

	std::uintptr_t win_lo = clo > text + 0x8000 ? clo - 0x8000 : text;
	std::uintptr_t win_hi = chi + 0x20000;
	if (draw_adv)
		win_hi = (std::max)(win_hi, draw_adv + 0x8000);
	if (win_lo < text)
		win_lo = text;
	if (win_hi > text + tsz)
		win_hi = text + tsz;

	struct Call { std::uintptr_t site, target; };
	std::vector<Call> calls;
	for (std::uintptr_t cur = win_lo; cur < win_hi; cur += 0x1000)
	{
		const std::size_t n = (std::min)((std::uintptr_t)0x1000, win_hi - cur);
		if (memory->read_raw(cur, page, n) != n)
			continue;
		for (std::size_t i = 0; i + 5 <= n; ++i)
		{
			if (page[i] != 0xE8)
				continue;
			std::int32_t rel = 0;
			std::memcpy(&rel, page + i + 1, 4);
			const std::uintptr_t tgt = cur + i + 5 + (std::uintptr_t)(std::int64_t)rel;
			if (tgt < text || tgt >= text + tsz)
				continue;
			const std::uintptr_t d = tgt > setup ? tgt - setup : setup - tgt;
			if (d < 0x20000)
				calls.push_back({ cur + i, tgt });
		}
	}

	std::unordered_set<std::uintptr_t> callees;
	for (const auto& c : calls)
	{
		if (c.site >= setup && c.site < setup + 0x8000)
			callees.insert(c.target);
	}
	if (callees.empty())
	{
		for (const auto& c : calls)
		{
			const std::uintptr_t d = c.site > setup ? c.site - setup : setup - c.site;
			if (d < 0x4000)
				callees.insert(c.target);
		}
	}
	if (callees.empty())
		return false;

	std::uintptr_t cube = 0;
	int csc = -1;
	for (auto tgt : callees)
	{
		int sites = 0, after = 0;
		for (const auto& c : calls)
		{
			if (c.target != tgt)
				continue;
			++sites;
			if (c.site >= setup)
				++after;
		}
		int sc = sites * 8 + after * 20;
		if (tgt > setup)
			sc += 15;
		if (sc > csc)
		{
			csc = sc;
			cube = tgt;
		}
	}
	if (!cube)
		return false;

	std::vector<std::uintptr_t> sites;
	for (const auto& c : calls)
	{
		if (c.target == cube)
			sites.push_back(c.site);
	}
	std::sort(sites.begin(), sites.end());
	if (sites.empty())
		return false;

	const std::uintptr_t cube_fn = fn_start_at(cube, text, tsz);
	if (!draw_adv || (draw_adv > cube_fn ? draw_adv - cube_fn : cube_fn - draw_adv) < 0x100)
		draw_adv = sites.back() + 0x8000;

	*lo = sites.front() - 0x2000;
	*hi = draw_adv + 0x20000;
	if (*lo < text)
		*lo = text;
	g.sky_lo = *lo;
	g.sky_hi = *hi;
	debug::ok("skyshader live range %llX-%llX cube=%llX",
		(unsigned long long)(*lo - base), (unsigned long long)(*hi - base),
		(unsigned long long)(cube_fn - base));
	return true;
}

bool sky_range(std::uintptr_t base, std::uintptr_t* lo, std::uintptr_t* hi)
{
	if (resolve_sky_range(base, lo, hi))
		return true;
	if (Offsets::Sky::DrawCubeCallB && Offsets::Sky::DrawAdv)
	{
		*lo = base + Offsets::Sky::DrawCubeCallB - 0x2000;
		*hi = base + Offsets::Sky::DrawAdv + 0x20000;
		g.sky_lo = *lo;
		g.sky_hi = *hi;
		debug::warn("skyshader range from Offsets.h");
		return true;
	}
	return false;
}

std::uintptr_t remote_d3d11()
{
	if (g.r_d3d11)
		return g.r_d3d11;
	g.r_d3d11 = mod_base_of(L"d3d11.dll");
	return g.r_d3d11;
}

// How big a module mapped into *this* process is, from its own PE headers.  The
// remote side of remap() needs the local image's size and this is the only place
// that can read it without a remote round trip.
std::size_t local_module_size(HMODULE mod)
{
	if (!mod)
		return 0;
	const auto* b   = reinterpret_cast<const std::uint8_t*>(mod);
	const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(b);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;
	const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(b + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE)
		return 0;
	return (std::size_t)nt->OptionalHeader.SizeOfImage;
}

std::uintptr_t remap(std::uintptr_t local)
{
	if (!g_local_d3d11 || !local)
		return 0;
	const std::uintptr_t l = (std::uintptr_t)g_local_d3d11;
	const std::uintptr_t r = remote_d3d11();
	if (!r || local < l)
		return 0;

	// Both ends, not just the near one.  A vtable slot can point outside our
	// d3d11.dll - a client-shipped trampoline, an import thunk, a detour - and
	// mapping one of those by its offset produces a plausible-looking address
	// inside the client that belongs to something else entirely.  The scan then
	// matches on it and finds nothing, with no hint as to why; refusing it here
	// keeps the search honest about what it can actually compare.
	const std::size_t sz = local_module_size(g_local_d3d11);
	if (sz && (local - l) >= (std::uintptr_t)sz)
		return 0;

	return r + (local - l);
}

std::uintptr_t remap_fn(void* p)
{
	return remap((std::uintptr_t)p);
}

std::uintptr_t render_view()
{
	// Both chains that reach it, through the SDK: the VisualEngine field and the
	// DataModel walk.  A stale VisualEngine field used to end this feature at
	// "no render view", which names no fix and gives the user nothing to try.
	const std::uint64_t rv = rbx::visualengine_t::render_view();
	return (std::uintptr_t)rv;
}

bool slot_is(std::uintptr_t obj, int slot, const std::uintptr_t* fns, int n)
{
	const std::uintptr_t vt = (std::uintptr_t)memory->read<std::uint64_t>(obj);
	if (!addr_ok(vt))
		return false;
	const std::uintptr_t fn = (std::uintptr_t)memory->read<std::uint64_t>(vt + (std::size_t)slot * 8);
	for (int i = 0; i < n; ++i)
	{
		if (fns[i] && fn == fns[i])
			return true;
	}
	return false;
}

std::uintptr_t scan_slot(std::uintptr_t obj, std::size_t bytes, int slot, const std::uintptr_t* fns, int n)
{
	if (!addr_ok(obj) || !fns || n <= 0)
		return 0;
	if (slot_is(obj, slot, fns, n))
		return obj;
	for (std::size_t o = 0; o + 8 <= bytes; o += 8)
	{
		const std::uintptr_t p = (std::uintptr_t)memory->read<std::uint64_t>(obj + o);
		if (!addr_ok(p))
			continue;
		if (slot_is(p, slot, fns, n))
			return p;
		for (std::size_t i = 0; i + 8 <= 0xC0; i += 8)
		{
			const std::uintptr_t q = (std::uintptr_t)memory->read<std::uint64_t>(p + i);
			if (addr_ok(q) && slot_is(q, slot, fns, n))
				return q;
		}
	}
	return 0;
}

void collect_dev_fns(std::uintptr_t* out, int& n)
{
	n = 0;
	auto add = [&](void* p)
	{
		const std::uintptr_t r = remap_fn(p);
		if (!r || n >= 8)
			return;
		for (int i = 0; i < n; ++i)
		{
			if (out[i] == r)
				return;
		}
		out[n++] = r;
	};
	if (g_dev_vt)
		add(g_dev_vt[15]);

}

// `why` names the step that failed.  Every one of these has a different fix, and
// a single "no device" for all of them is a report nobody can act on - which is
// what this returned before.
bool find_d3d(const char*& why)
{
	if (g.device && mem_valid(g.device))
		return true;
	if (!local_d3d())
	{
		why = "no local d3d11";
		return false;
	}

	std::uintptr_t create_ps[8]{};
	int nps = 0;
	collect_dev_fns(create_ps, nps);

	std::uintptr_t psset[8]{};
	int nset = 0;
	auto add_set = [&](void* p)
	{
		const std::uintptr_t r = remap_fn(p);
		if (!r || nset >= 8)
			return;
		for (int i = 0; i < nset; ++i)
		{
			if (psset[i] == r)
				return;
		}
		psset[nset++] = r;
	};
	if (g_ctx_vt)
		add_set(g_ctx_vt[9]);

	if (!nps)
	{
		// Nothing to compare against: the device-identity function came back
		// empty from our own device's vtable, so no amount of scanning the client
		// can produce an answer.
		why = "no device vtable fn";
		return false;
	}

	const std::uintptr_t rv = render_view();
	if (!rv)
	{
		why = "no render view";
		return false;
	}

	std::uintptr_t wrap = 0;
	if (Offsets::RenderView::DeviceD3D11)
		wrap = (std::uintptr_t)memory->read<std::uint64_t>(rv + Offsets::RenderView::DeviceD3D11);
	std::uintptr_t dev = 0;
	std::uintptr_t ctx = 0;
	if (mem_valid(wrap))
	{
		dev = scan_slot(wrap, 0x200, 15, create_ps, nps);
		if (nset)
			ctx = scan_slot(wrap, 0x200, 9, psset, nset);
	}
	if (!dev)
		dev = scan_slot(rv, 0x100, 15, create_ps, nps);
	if (!ctx && nset)
		ctx = scan_slot(rv, 0x100, 9, psset, nset);
	if (!dev && ctx)
		dev = scan_slot(ctx, 0x100, 15, create_ps, nps);
	if (!dev)
	{
		why = mem_valid(wrap) ? "no device in the d3d11 wrapper" : "no d3d11 wrapper";
		return false;
	}

	g.device = dev;
	g.ctx = ctx;
	return true;
}

void patch_rel32(std::vector<std::uint8_t>& c, std::size_t at, std::size_t dest)
{
	const std::int32_t rel = (std::int32_t)((std::ptrdiff_t)dest - (std::ptrdiff_t)(at + 4));
	std::memcpy(c.data() + at, &rel, 4);
}

std::vector<std::uint8_t> make_upload_stub(std::uintptr_t state)
{
	std::vector<std::uint8_t> c;
	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x48 });
	c.insert(c.end(), { 0x48, 0x89, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x83, 0x78, 0x20, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_done0 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x83, 0xB8, 0x08, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x74, 0x00 });
	const std::size_t j_dirty = c.size() - 1;
	c.insert(c.end(), { 0xFF, 0x90, 0x08, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0x89, 0xC0 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x8B, 0x88, 0x00, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x44, 0x89, 0xC2 });
	c.insert(c.end(), { 0x29, 0xCA });
	c.insert(c.end(), { 0x83, 0xFA, 0x0C });
	c.insert(c.end(), { 0x0F, 0x82, 0, 0, 0, 0 });
	const std::size_t j_bind0 = c.size() - 4;
	c.insert(c.end(), { 0x44, 0x89, 0x80, 0x00, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0xC7, 0x80, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xEB, 0x00 });
	const std::size_t j_map = c.size() - 1;
	const std::size_t dirty = c.size();
	c.insert(c.end(), { 0x8B, 0x88, 0xFC, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xC7, 0x80, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x85, 0xC9 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_bind1 = c.size() - 4;
	c[j_dirty] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)dirty - (std::ptrdiff_t)(j_dirty + 1)));
	c[j_map] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)c.size() - (std::ptrdiff_t)(j_map + 1)));

	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0x48, 0x8B, 0x90, 0x18, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xD2 });
	c.insert(c.end(), { 0x75, 0x10 });
	c.insert(c.end(), { 0x48, 0x8B, 0x90, 0x10, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xD2 });
	c.insert(c.end(), { 0x75, 0x04 });
	c.insert(c.end(), { 0x48, 0x8B, 0x50, 0x20 });
	c.insert(c.end(), { 0x45, 0x31, 0xC0 });
	c.insert(c.end(), { 0x41, 0xB9, 0x04, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xC7, 0x44, 0x24, 0x20, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x54, 0x24, 0x30 });
	c.insert(c.end(), { 0x4C, 0x89, 0x54, 0x24, 0x28 });
	c.insert(c.end(), { 0x48, 0xC7, 0x44, 0x24, 0x30, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xC7, 0x44, 0x24, 0x38, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x50, 0x28 });
	c.insert(c.end(), { 0x85, 0xC0 });
	c.insert(c.end(), { 0x0F, 0x88, 0, 0, 0, 0 });
	const std::size_t j_done1 = c.size() - 4;

	c.insert(c.end(), { 0x4C, 0x8B, 0x44, 0x24, 0x30 });
	c.insert(c.end(), { 0x4D, 0x85, 0xC0 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_unmap0 = c.size() - 4;

	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8D, 0x50, 0x40 });
	c.insert(c.end(), { 0xB9, 0x0E, 0x00, 0x00, 0x00 });
	const std::size_t loop = c.size();
	c.insert(c.end(), { 0x4C, 0x8B, 0x0A });
	c.insert(c.end(), { 0x4D, 0x89, 0x08 });
	c.insert(c.end(), { 0x48, 0x83, 0xC2, 0x08 });
	c.insert(c.end(), { 0x49, 0x83, 0xC0, 0x08 });
	c.insert(c.end(), { 0xFF, 0xC9 });
	c.insert(c.end(), { 0x75, 0x00 });
	c[c.size() - 1] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)loop - (std::ptrdiff_t)c.size()));
	c.insert(c.end(), { 0xFF, 0x40, 0x1C });
	c.insert(c.end(), { 0xC7, 0x44, 0x24, 0x18, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xEB, 0x08 });
	const std::size_t unmap0 = c.size();
	c.insert(c.end(), { 0xC7, 0x44, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00 });
	const std::size_t unmap = c.size();
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0x48, 0x8B, 0x90, 0x18, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xD2 });
	c.insert(c.end(), { 0x75, 0x10 });
	c.insert(c.end(), { 0x48, 0x8B, 0x90, 0x10, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xD2 });
	c.insert(c.end(), { 0x75, 0x04 });
	c.insert(c.end(), { 0x48, 0x8B, 0x50, 0x20 });
	c.insert(c.end(), { 0x45, 0x31, 0xC0 });
	c.insert(c.end(), { 0xFF, 0x50, 0x38 });
	c.insert(c.end(), { 0x83, 0x7C, 0x24, 0x18, 0x00 });
	c.insert(c.end(), { 0x74, 0x00 });
	const std::size_t j_noswap = c.size() - 1;
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x4C, 0x8B, 0x80, 0x18, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x4D, 0x85, 0xC0 });
	c.insert(c.end(), { 0x74, 0x00 });
	const std::size_t j_ping2 = c.size() - 1;
	c.insert(c.end(), { 0x4C, 0x8B, 0x48, 0x20 });
	c.insert(c.end(), { 0x4C, 0x8B, 0x90, 0x10, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x89, 0x40, 0x20 });
	c.insert(c.end(), { 0x4C, 0x89, 0x88, 0x10, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x89, 0x90, 0x18, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0xEB, 0x00 });
	const std::size_t j_didrot = c.size() - 1;
	const std::size_t ping2 = c.size();
	c[j_ping2] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)ping2 - (std::ptrdiff_t)(j_ping2 + 1)));
	c.insert(c.end(), { 0x4C, 0x8B, 0x80, 0x10, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x4D, 0x85, 0xC0 });
	c.insert(c.end(), { 0x74, 0x0F });
	c.insert(c.end(), { 0x4C, 0x8B, 0x48, 0x20 });
	c.insert(c.end(), { 0x4C, 0x89, 0x40, 0x20 });
	c.insert(c.end(), { 0x4C, 0x89, 0x88, 0x10, 0x01, 0x00, 0x00 });
	const std::size_t bind = c.size();
	c[j_didrot] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)bind - (std::ptrdiff_t)(j_didrot + 1)));
	c[j_noswap] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)bind - (std::ptrdiff_t)(j_noswap + 1)));
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	append_blend_off(c);
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0xBA, 0x0D, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0xB8, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x48, 0x20 });
	c.insert(c.end(), { 0xFF, 0x50, 0x30 });

	const std::size_t done = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x48 });
	c.push_back(0xC3);
	patch_rel32(c, j_done0, done);
	patch_rel32(c, j_done1, bind);
	patch_rel32(c, j_bind0, bind);
	patch_rel32(c, j_bind1, bind);
	patch_rel32(c, j_unmap0, unmap0);
	return c;
}

std::vector<std::uint8_t> make_psset_thunk(std::uintptr_t state, std::uintptr_t create, std::uintptr_t upload, std::uintptr_t sky_lo, std::uintptr_t sky_hi, std::uintptr_t bind)
{
	std::vector<std::uint8_t> c;
	c.insert(c.end(), { 0x51, 0x52, 0x41, 0x50, 0x41, 0x51 });
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x41, 0xFF, 0x82, 0xF0, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0x83, 0x3A, 0x01 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j_orig0 = c.size() - 4;

	c.insert(c.end(), { 0x48, 0x8D, 0x44, 0x24, 0x20 });
	c.insert(c.end(), { 0xB9, 0x40, 0x00, 0x00, 0x00 });
	const std::size_t scan = c.size();
	c.insert(c.end(), { 0x4C, 0x8B, 0x00 });
	c.insert(c.end(), { 0x49, 0xBB });
	append_u64(c, sky_lo);
	c.insert(c.end(), { 0x4D, 0x39, 0xD8 });
	c.insert(c.end(), { 0x72, 0x00 });
	const std::size_t j_next0 = c.size() - 1;
	c.insert(c.end(), { 0x49, 0xBB });
	append_u64(c, sky_hi);
	c.insert(c.end(), { 0x4D, 0x39, 0xD8 });
	c.insert(c.end(), { 0x72, 0x00 });
	const std::size_t j_hit = c.size() - 1;
	const std::size_t next = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xC0, 0x08 });
	c.insert(c.end(), { 0xFF, 0xC9 });
	c.insert(c.end(), { 0x75, 0x00 });
	const std::size_t j_scan = c.size() - 1;
	c.insert(c.end(), { 0xE9, 0, 0, 0, 0 });
	const std::size_t j_orig1 = c.size() - 4;

	const std::size_t hit = c.size();
	c.insert(c.end(), { 0x41, 0xC7, 0x82, 0x90, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x49, 0x83, 0x7A, 0x08, 0x00 });
	c.insert(c.end(), { 0x75, 0x00 });
	const std::size_t j_swap0 = c.size() - 1;
	c.insert(c.end(), { 0xB8, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0x87, 0x42, 0x18 });
	c.insert(c.end(), { 0x85, 0xC0 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j_orig2 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x28 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, create);
	c.insert(c.end(), { 0xFF, 0xD0 });
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x28 });
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x49, 0x83, 0x7A, 0x08, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_orig3 = c.size() - 4;

	const std::size_t swap = c.size();
	c.insert(c.end(), { 0x41, 0xFF, 0x42, 0x04 });
	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x28 });
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, upload);
	c.insert(c.end(), { 0xFF, 0xD0 });
	if (bind)
	{
		c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
		c.insert(c.end(), { 0x48, 0xB8 });
		append_u64(c, bind);
		c.insert(c.end(), { 0xFF, 0xD0 });
	}
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x28 });
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59 });
	c.insert(c.end(), { 0x49, 0x8B, 0x52, 0x08 });
	c.insert(c.end(), { 0x41, 0xFF, 0x62, 0x10 });

	const std::size_t orig_keep = c.size();
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59 });
	c.insert(c.end(), { 0x41, 0xFF, 0x62, 0x10 });

	const std::size_t orig = c.size();
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x41, 0xC7, 0x82, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59 });
	c.insert(c.end(), { 0x41, 0xFF, 0x62, 0x10 });

	auto s8 = [&](std::size_t at, std::size_t dest)
	{
		c[at] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)dest - (std::ptrdiff_t)(at + 1)));
	};
	s8(j_next0, next);
	s8(j_hit, hit);
	s8(j_scan, scan);
	s8(j_swap0, swap);
	patch_rel32(c, j_orig0, orig);
	patch_rel32(c, j_orig1, orig);
	patch_rel32(c, j_orig2, orig_keep);
	patch_rel32(c, j_orig3, orig_keep);
	return c;
}

std::vector<std::uint8_t> make_create_stub(std::uintptr_t state)
{
	std::vector<std::uint8_t> c;
	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x28 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0xB8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xC9 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_fail0 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x8B, 0x90, 0xC0, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8B, 0x80, 0xC8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4D, 0x31, 0xC9 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x90, 0xD0, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x89, 0x54, 0x24, 0x20 });
	c.insert(c.end(), { 0xFF, 0x90, 0xB0, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0xD0, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xC9 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_fail1 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x89, 0x48, 0x08 });
	c.insert(c.end(), { 0x48, 0x83, 0x78, 0x20, 0x00 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j_cb2 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0xB8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8D, 0x90, 0x00, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x4D, 0x31, 0xC0 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x88, 0x20, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x90, 0xD8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0x20, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xC9 });
	c.insert(c.end(), { 0x74, 0x04 });
	c.insert(c.end(), { 0x48, 0x89, 0x48, 0x20 });
	const std::size_t try_cb2 = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xB8, 0x10, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j_done0 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0xC7, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0xB8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8D, 0x90, 0x00, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x4D, 0x31, 0xC0 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x88, 0x20, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x90, 0xD8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0x20, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xC9 });
	c.insert(c.end(), { 0x74, 0x07 });
	c.insert(c.end(), { 0x48, 0x89, 0x88, 0x10, 0x01, 0x00, 0x00 });
	const std::size_t try_cb3 = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xB8, 0x18, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j_done1 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0xC7, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0xB8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8D, 0x90, 0x00, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x4D, 0x31, 0xC0 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x88, 0x20, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x90, 0xD8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x88, 0x20, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xC9 });
	c.insert(c.end(), { 0x74, 0x07 });
	c.insert(c.end(), { 0x48, 0x89, 0x88, 0x18, 0x01, 0x00, 0x00 });
	const std::size_t done = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x28 });
	c.push_back(0xC3);
	const std::size_t fail = c.size();
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0xC7, 0x40, 0x18, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x28 });
	c.push_back(0xC3);
	patch_rel32(c, j_fail0, fail);
	patch_rel32(c, j_fail1, fail);
	patch_rel32(c, j_cb2, try_cb2);
	patch_rel32(c, j_done0, try_cb3);
	patch_rel32(c, j_done1, done);
	return c;
}

std::vector<std::uint8_t> make_video_stub(std::uintptr_t state)
{
	std::vector<std::uint8_t> c;
	c.insert(c.end(), { 0x53, 0x56, 0x57 });
	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x60 });
	c.insert(c.end(), { 0x48, 0xBB });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x89, 0x4C, 0x24, 0x58 });

	c.insert(c.end(), { 0x83, 0xBB, 0x40, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_done0 = c.size() - 4;
	c.insert(c.end(), { 0xFF, 0x83, 0x98, 0x01, 0x00, 0x00 });

	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x20, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j_have_tex = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x50, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_done_fn = c.size() - 4;

	c.insert(c.end(), { 0x48, 0x8B, 0x8B, 0xB8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xC9 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_done1 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x8D, 0x93, 0x80, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x45, 0x31, 0xC0 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x8B, 0xB0, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xC7, 0x83, 0xB0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x93, 0x50, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x89, 0x83, 0x04, 0x03, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8B, 0x83, 0xB0, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xC0 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_done2 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x89, 0x83, 0x20, 0x01, 0x00, 0x00 });

	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x58, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_done_srv = c.size() - 4;

	c.insert(c.end(), { 0x48, 0x8B, 0x8B, 0xB8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8B, 0xD0 });
	c.insert(c.end(), { 0x45, 0x31, 0xC0 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x8B, 0xB8, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xC7, 0x83, 0xB8, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x93, 0x58, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8B, 0x83, 0xB8, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x89, 0x83, 0x28, 0x01, 0x00, 0x00 });

	const std::size_t samp_chk = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x30, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x75, 0x00 });
	const std::size_t j_have_samp = c.size() - 1;
	c.insert(c.end(), { 0x48, 0x8B, 0x8B, 0xB8, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8D, 0x93, 0xC0, 0x02, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x83, 0x00, 0x03, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xC7, 0x83, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x93, 0x60, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8B, 0x83, 0x00, 0x03, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x89, 0x83, 0x30, 0x01, 0x00, 0x00 });
	const std::size_t have_tex = c.size();
	c[j_have_samp] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)have_tex - (std::ptrdiff_t)(j_have_samp + 1)));

	c.insert(c.end(), { 0x83, 0xBB, 0x8C, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_done_nobind = c.size() - 4;

	c.insert(c.end(), { 0x83, 0xBB, 0x4C, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_bind0 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x20, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_bind1 = c.size() - 4;
	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x38, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_bind2 = c.size() - 4;

	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x58 });
	c.insert(c.end(), { 0x48, 0x8B, 0x93, 0x20, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x45, 0x31, 0xC0 });
	c.insert(c.end(), { 0x41, 0xB9, 0x04, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xC7, 0x44, 0x24, 0x20, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x54, 0x24, 0x30 });
	c.insert(c.end(), { 0x4C, 0x89, 0x54, 0x24, 0x28 });
	c.insert(c.end(), { 0x48, 0xC7, 0x44, 0x24, 0x30, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0xC7, 0x44, 0x24, 0x38, 0x00, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x53, 0x28 });
	c.insert(c.end(), { 0x85, 0xC0 });
	c.insert(c.end(), { 0x0F, 0x88, 0, 0, 0, 0 });
	const std::size_t j_bind3 = c.size() - 4;

	c.insert(c.end(), { 0x4C, 0x8B, 0x44, 0x24, 0x30 });
	c.insert(c.end(), { 0x4D, 0x85, 0xC0 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_unmap = c.size() - 4;
	c.insert(c.end(), { 0x8B, 0x54, 0x24, 0x38 });
	c.insert(c.end(), { 0x48, 0x8B, 0xB3, 0x38, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x8B, 0x8B, 0x44, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x44, 0x8B, 0x9B, 0x48, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x85, 0xF6 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_unmap2 = c.size() - 4;
	c.insert(c.end(), { 0x85, 0xC9 });
	c.insert(c.end(), { 0x0F, 0x8E, 0, 0, 0, 0 });
	const std::size_t j_unmap3 = c.size() - 4;
	c.insert(c.end(), { 0x45, 0x85, 0xDB });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_unmap4 = c.size() - 4;
	c.insert(c.end(), { 0x85, 0xD2 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j_unmap5 = c.size() - 4;
	c.insert(c.end(), { 0x44, 0x39, 0xDA });
	c.insert(c.end(), { 0x73, 0x03 });
	c.insert(c.end(), { 0x44, 0x8B, 0xDA });
	const std::size_t row = c.size();
	c.insert(c.end(), { 0x51 });
	c.insert(c.end(), { 0x4C, 0x89, 0xC7 });
	c.insert(c.end(), { 0x44, 0x89, 0xD9 });
	c.insert(c.end(), { 0xF3, 0xA4 });
	c.insert(c.end(), { 0x59 });
	c.insert(c.end(), { 0x49, 0x01, 0xD0 });
	c.insert(c.end(), { 0xFF, 0xC9 });
	c.insert(c.end(), { 0x75, 0x00 });
	c[c.size() - 1] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)row - (std::ptrdiff_t)c.size()));

	const std::size_t unmap = c.size();
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x58 });
	c.insert(c.end(), { 0x48, 0x8B, 0x93, 0x20, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x45, 0x31, 0xC0 });
	c.insert(c.end(), { 0xFF, 0x53, 0x38 });
	c.insert(c.end(), { 0xC7, 0x83, 0x4C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

	const std::size_t bind = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x68, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x74, 0x00 });
	const std::size_t j_done_ps = c.size() - 1;
	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x28, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x74, 0x00 });
	const std::size_t j_done3 = c.size() - 1;
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x58 });
	c.insert(c.end(), { 0xBA, 0x0D, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0xB8, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x8B, 0x28, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x93, 0x68, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x83, 0xBB, 0x30, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x74, 0x1D });
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x58 });
	c.insert(c.end(), { 0xBA, 0x0D, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0xB8, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x8B, 0x30, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0xFF, 0x93, 0x70, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0xC7, 0x83, 0x88, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 });

	const std::size_t done = c.size();
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x60 });
	c.insert(c.end(), { 0x5F, 0x5E, 0x5B });
	c.push_back(0xC3);

	patch_rel32(c, j_done0, done);
	patch_rel32(c, j_done1, done);
	patch_rel32(c, j_done2, done);
	patch_rel32(c, j_done_fn, done);
	patch_rel32(c, j_done_srv, done);
	patch_rel32(c, j_done_nobind, done);
	patch_rel32(c, j_have_tex, samp_chk);
	patch_rel32(c, j_bind0, bind);
	patch_rel32(c, j_bind1, bind);
	patch_rel32(c, j_bind2, bind);
	patch_rel32(c, j_bind3, bind);
	patch_rel32(c, j_unmap, unmap);
	patch_rel32(c, j_unmap2, unmap);
	patch_rel32(c, j_unmap3, unmap);
	patch_rel32(c, j_unmap4, unmap);
	patch_rel32(c, j_unmap5, unmap);
	c[j_done_ps] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)done - (std::ptrdiff_t)(j_done_ps + 1)));
	c[j_done3] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)done - (std::ptrdiff_t)(j_done3 + 1)));
	return c;
}

std::vector<std::uint8_t> make_bind_stub(std::uintptr_t state, std::uintptr_t video)
{
	std::vector<std::uint8_t> c;
	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x48 });
	c.insert(c.end(), { 0x48, 0x89, 0x4C, 0x24, 0x40 });
	append_call_video_if(c, state, video, 1);
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x83, 0x78, 0x20, 0x00 });
	c.insert(c.end(), { 0x74, 0x00 });
	const std::size_t j_skip = c.size() - 1;
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	append_blend_off(c);
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, state);
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0xBA, 0x0D, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x41, 0xB8, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x4C, 0x8D, 0x48, 0x20 });
	c.insert(c.end(), { 0xFF, 0x50, 0x30 });
	const std::size_t done = c.size();
	c[j_skip] = (std::uint8_t)((std::int8_t)((std::ptrdiff_t)done - (std::ptrdiff_t)(j_skip + 1)));
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x48 });
	c.push_back(0xC3);
	return c;
}

std::vector<std::uint8_t> make_srv_thunk(std::uintptr_t state, std::uintptr_t bind)
{
	std::vector<std::uint8_t> c;
	if (!bind)
		return c;
	c.insert(c.end(), { 0x51, 0x52, 0x41, 0x50, 0x41, 0x51 });
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x41, 0x83, 0x3A, 0x01 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j0 = c.size() - 4;
	c.insert(c.end(), { 0x41, 0x83, 0xBA, 0x90, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j1 = c.size() - 4;
	c.insert(c.end(), { 0x41, 0x83, 0xBA, 0x40, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j2 = c.size() - 4;
	c.insert(c.end(), { 0x41, 0xFF, 0x82, 0x94, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x28 });
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0x48, 0x8B, 0x54, 0x24, 0x38 });
	c.insert(c.end(), { 0x4C, 0x8B, 0x44, 0x24, 0x30 });
	c.insert(c.end(), { 0x4C, 0x8B, 0x4C, 0x24, 0x28 });
	c.insert(c.end(), { 0x41, 0xFF, 0x92, 0x68, 0x01, 0x00, 0x00 });
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, bind);
	c.insert(c.end(), { 0xFF, 0xD0 });
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x28 });
	c.insert(c.end(), { 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59 });
	c.push_back(0xC3);
	const std::size_t orig = c.size();
	c.insert(c.end(), { 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59 });
	c.insert(c.end(), { 0x41, 0xFF, 0xA2, 0x68, 0x01, 0x00, 0x00 });
	patch_rel32(c, j0, orig);
	patch_rel32(c, j1, orig);
	patch_rel32(c, j2, orig);
	return c;
}

std::vector<std::uint8_t> make_draw_thunk(std::uintptr_t state, std::uintptr_t upload, std::uint32_t orig_off, std::uint32_t enter_off)
{
	std::vector<std::uint8_t> c;
	c.insert(c.end(), { 0x51, 0x52, 0x41, 0x50, 0x41, 0x51 });
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x41, 0xFF, 0x82 });
	c.insert(c.end(), {
		(std::uint8_t)enter_off, (std::uint8_t)(enter_off >> 8),
		(std::uint8_t)(enter_off >> 16), (std::uint8_t)(enter_off >> 24)
	});
	c.insert(c.end(), { 0x41, 0x83, 0x3A, 0x01 });
	c.insert(c.end(), { 0x0F, 0x85, 0, 0, 0, 0 });
	const std::size_t j0 = c.size() - 4;
	c.insert(c.end(), { 0x41, 0x83, 0xBA, 0x90, 0x01, 0x00, 0x00, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j1 = c.size() - 4;
	c.insert(c.end(), { 0x49, 0x83, 0x7A, 0x08, 0x00 });
	c.insert(c.end(), { 0x0F, 0x84, 0, 0, 0, 0 });
	const std::size_t j2 = c.size() - 4;

	c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x28 });
	c.insert(c.end(), { 0x48, 0x8B, 0x4C, 0x24, 0x40 });
	c.insert(c.end(), { 0x48, 0xB8 });
	append_u64(c, upload);
	c.insert(c.end(), { 0xFF, 0xD0 });
	c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x28 });

	const std::size_t out = c.size();
	c.insert(c.end(), { 0x49, 0xBA });
	append_u64(c, state);
	c.insert(c.end(), { 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59 });
	c.insert(c.end(), { 0x41, 0xFF, 0xA2 });
	const std::uint32_t off = orig_off;
	c.insert(c.end(), { (std::uint8_t)off, (std::uint8_t)(off >> 8), (std::uint8_t)(off >> 16), (std::uint8_t)(off >> 24) });

	patch_rel32(c, j0, out);
	patch_rel32(c, j1, out);
	patch_rel32(c, j2, out);
	return c;
}

bool compile_ps(std::vector<std::uint8_t>& out)
{
	ID3DBlob* blob = nullptr;
	ID3DBlob* err = nullptr;
	const HRESULT hr = D3DCompile(
		k_hlsl, sizeof(k_hlsl) - 1, nullptr, nullptr, nullptr,
		"ps_main", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &blob, &err);
	if (FAILED(hr) || !blob)
	{
		if (err)
		{
			static bool logged = false;
			if (!logged)
			{
				logged = true;
				debug::error("skyshader compile: %s", (const char*)err->GetBufferPointer());
			}
			err->Release();
		}
		return false;
	}
	if (err)
		err->Release();
	const auto* p = (const std::uint8_t*)blob->GetBufferPointer();
	out.assign(p, p + blob->GetBufferSize());
	blob->Release();
	return !out.empty();
}

bool add_patch(std::uintptr_t at, std::uintptr_t thunk)
{
	if (!addr_ok(at) || !addr_ok(thunk) || g.npatch >= 48)
		return false;
	const std::uint64_t old = memory->read<std::uint64_t>(at);
	if (!old || old == thunk)
		return old == thunk;
	for (int i = 0; i < g.npatch; ++i)
	{
		if (g.patches[i].at == at)
			return true;
	}
	DWORD prot = 0;
	if (!mem_protect(at, 8, PAGE_READWRITE, &prot))
		return false;
	const bool ok = mem_write<std::uint64_t>(at, thunk);
	mem_protect(at, 8, prot, nullptr);
	if (!ok)
		return false;
	g.patches[g.npatch].at = at;
	g.patches[g.npatch].old = old;
	++g.npatch;
	if (!g.orig_psset)
		g.orig_psset = (std::uintptr_t)old;
	return true;
}

void unpatch()
{
	for (int i = 0; i < g.npatch; ++i)
	{
		if (!g.patches[i].at)
			continue;
		DWORD prot = 0;
		if (mem_protect(g.patches[i].at, 8, PAGE_READWRITE, &prot))
		{
			memory->write<std::uint64_t>(g.patches[i].at, g.patches[i].old);
			mem_protect(g.patches[i].at, 8, prot, nullptr);
		}
		g.patches[i] = {};
	}
	g.npatch = 0;
}

bool patch_vtables()
{
	if (!g.psset_thunk || !g.ctx)
		return false;
	const std::uintptr_t vt = (std::uintptr_t)memory->read<std::uint64_t>(g.ctx);
	if (!addr_ok(vt))
		return false;
	if (!add_patch(vt + 9 * 8, g.psset_thunk) || !g.orig_psset)
		return false;
	if (g.srv_thunk)
		add_patch(vt + 8 * 8, g.srv_thunk);
	if (g.drawidx_thunk)
		add_patch(vt + 12 * 8, g.drawidx_thunk);
	if (g.draw_thunk)
		add_patch(vt + 13 * 8, g.draw_thunk);
	if (g.drawidxinst_thunk)
		add_patch(vt + 20 * 8, g.drawidxinst_thunk);
	if (g.drawinst_thunk)
		add_patch(vt + 21 * 8, g.drawinst_thunk);
	return true;
}

bool setup_meta()
{
	if (!g.device || !g.state)
		return false;

	std::vector<std::uint8_t> dxbc;
	if (!compile_ps(dxbc))
		return false;
	if (!g.dxbc)
		g.dxbc = memory->alloc(dxbc.size() + 16, PAGE_READWRITE);
	if (!g.dxbc)
		return false;
	if (memory->write_raw(g.dxbc, dxbc.data(), dxbc.size()) != dxbc.size())
		return false;

	const std::uintptr_t dev_vt = (std::uintptr_t)memory->read<std::uint64_t>(g.device);
	std::uintptr_t create_ps = addr_ok(dev_vt) ? (std::uintptr_t)memory->read<std::uint64_t>(dev_vt + 15 * 8) : 0;
	std::uintptr_t create_buf = addr_ok(dev_vt) ? (std::uintptr_t)memory->read<std::uint64_t>(dev_vt + 3 * 8) : 0;
	std::uintptr_t create_tex = addr_ok(dev_vt) ? (std::uintptr_t)memory->read<std::uint64_t>(dev_vt + 5 * 8) : 0;
	std::uintptr_t create_srv = addr_ok(dev_vt) ? (std::uintptr_t)memory->read<std::uint64_t>(dev_vt + 7 * 8) : 0;
	std::uintptr_t create_samp = addr_ok(dev_vt) ? (std::uintptr_t)memory->read<std::uint64_t>(dev_vt + 23 * 8) : 0;
	if (!addr_ok(create_ps))
		create_ps = remap_fn(g_dev_vt[15]);
	if (!addr_ok(create_buf))
		create_buf = remap_fn(g_dev_vt[3]);
	if (!addr_ok(create_tex))
		create_tex = remap_fn(g_dev_vt[5]);
	if (!addr_ok(create_srv))
		create_srv = remap_fn(g_dev_vt[7]);
	if (!addr_ok(create_samp))
		create_samp = remap_fn(g_dev_vt[23]);
	std::uintptr_t map_fn = 0;
	std::uintptr_t unmap_fn = 0;
	std::uintptr_t pscb = 0;
	std::uintptr_t orig_draw = 0;
	std::uintptr_t orig_drawidx = 0;
	std::uintptr_t orig_drawinst = 0;
	std::uintptr_t orig_drawidxinst = 0;
	std::uintptr_t ps_srv = 0;
	std::uintptr_t ps_samp = 0;
	if (g.ctx)
	{
		const std::uintptr_t cvt = (std::uintptr_t)memory->read<std::uint64_t>(g.ctx);
		if (addr_ok(cvt))
		{
			map_fn = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 14 * 8);
			unmap_fn = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 15 * 8);
			pscb = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 16 * 8);
			orig_draw = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 13 * 8);
			orig_drawidx = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 12 * 8);
			orig_drawidxinst = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 20 * 8);
			orig_drawinst = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 21 * 8);
			ps_srv = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 8 * 8);
			ps_samp = (std::uintptr_t)memory->read<std::uint64_t>(cvt + 10 * 8);
		}
	}
	if (!addr_ok(map_fn))
		map_fn = remap_fn(g_ctx_vt[14]);
	if (!addr_ok(unmap_fn))
		unmap_fn = remap_fn(g_ctx_vt[15]);
	if (!addr_ok(pscb))
		pscb = remap_fn(g_ctx_vt[16]);
	if (!addr_ok(orig_draw))
		orig_draw = remap_fn(g_ctx_vt[13]);
	if (!addr_ok(orig_drawidx))
		orig_drawidx = remap_fn(g_ctx_vt[12]);
	if (!addr_ok(orig_drawidxinst))
		orig_drawidxinst = remap_fn(g_ctx_vt[20]);
	if (!addr_ok(orig_drawinst))
		orig_drawinst = remap_fn(g_ctx_vt[21]);
	if (!addr_ok(ps_srv))
		ps_srv = remap_fn(g_ctx_vt[8]);
	if (!addr_ok(ps_samp))
		ps_samp = remap_fn(g_ctx_vt[10]);
	if (!addr_ok(create_ps) || !addr_ok(create_buf) || !addr_ok(map_fn) || !addr_ok(unmap_fn) || !addr_ok(pscb))
		return false;
	if (!addr_ok(orig_draw) || !addr_ok(orig_drawidx))
		return false;

	RemoteState st{};
	memory->read_raw(g.state, &st, sizeof(st));
	st.create_ps_fn = create_ps;
	st.create_buf_fn = create_buf;
	st.device = g.device;
	st.dxbc = g.dxbc;
	st.dxbc_len = dxbc.size();
	st.out_ps = 0;
	st.our_ps = 0;
	st.cb = 0;
	st.map_fn = map_fn;
	st.pscb = pscb;
	st.unmap_fn = unmap_fn;
	st.maps = 0;
	st.dirty = 0;
	st.last_map = 0;
	st.tick_fn = 0;
	st.cb2 = 0;
	st.cb3 = 0;
	{
		const std::uintptr_t rk = mod_base_of(L"kernel32.dll");
		const HMODULE lk = GetModuleHandleW(L"kernel32.dll");
		const FARPROC tp = lk ? GetProcAddress(lk, "GetTickCount") : nullptr;
		if (rk && lk && tp)
			st.tick_fn = rk + ((std::uintptr_t)tp - (std::uintptr_t)lk);
	}
	st.creating = 0;
	st.orig_draw = orig_draw;
	st.orig_drawidx = orig_drawidx;
	st.orig_drawinst = orig_drawinst;
	st.orig_drawidxinst = orig_drawidxinst;
	st.create_tex_fn = create_tex;
	st.create_srv_fn = create_srv;
	st.create_samp_fn = create_samp;
	st.ps_srv_fn = ps_srv;
	st.ps_samp_fn = ps_samp;
	memory->write_raw(g.state, &st, sizeof(st));

	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = (sizeof(SkyCB) + 15u) & ~15u;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	memory->write_raw(g.state + 0x200, &desc, sizeof(desc));
	memory->write<std::uint64_t>(g.state + 0x220, 0);

	D3D11_TEXTURE2D_DESC td{};
	td.Width = (UINT)SkyVideo::k_w;
	td.Height = (UINT)SkyVideo::k_h;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DYNAMIC;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	memory->write_raw(g.state + 0x280, &td, sizeof(td));

	D3D11_SAMPLER_DESC sd{};
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.MaxLOD = D3D11_FLOAT32_MAX;
	sd.MaxAnisotropy = 1;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	memory->write_raw(g.state + 0x2C0, &sd, sizeof(sd));
	return true;
}

void write_enabled(std::uint32_t v)
{
	if (g.state)
		memory->write<std::uint32_t>(g.state + offsetof(RemoteState, enabled), v);
}

bool push_video()
{
	if (!g.state)
		return false;
	if (settings::skybox::shader != 3)
	{
		memory->write<std::uint32_t>(g.state + offsetof(RemoteState, pix_w), 0);
		memory->write<std::uint32_t>(g.state + offsetof(RemoteState, vid_bound), 0);
		SkyVideo::Play({});
		return false;
	}

	SkyVideo::Ensure();
	const auto files = SkyVideo::List();
	if (files.empty())
	{
		memory->write<std::uint32_t>(g.state + offsetof(RemoteState, pix_w), 0);
		memory->write<std::uint32_t>(g.state + offsetof(RemoteState, vid_bound), 0);
		SkyVideo::Play({});
		return false;
	}

	int i = settings::skybox::video;
	if (settings::skybox::video_file[0])
	{
		for (int n = 0; n < (int)files.size(); ++n)
		{
			if (files[n] == settings::skybox::video_file)
			{
				i = n;
				break;
			}
		}
	}
	if (i < 0)
		i = 0;
	if (i >= (int)files.size())
		i = (int)files.size() - 1;
	settings::skybox::video = i;
	strcpy_s(settings::skybox::video_file, files[i].c_str());
	SkyVideo::Play(files[i]);

	const void* px = nullptr;
	int w = 0, h = 0;
	if (!SkyVideo::Frame(&px, &w, &h) || !px || w <= 0 || h <= 0)
		return false;

	if (!g.pix)
	{
		g.pix = memory->alloc((SIZE_T)SkyVideo::k_w * SkyVideo::k_h * 4, PAGE_READWRITE);
		if (g.pix)
			memory->write<std::uint64_t>(g.state + offsetof(RemoteState, pix), g.pix);
	}
	if (!g.pix)
		return false;

	memory->write_raw(g.pix, px, (SIZE_T)w * (SIZE_T)h * 4);
	memory->write<std::uint32_t>(g.state + offsetof(RemoteState, pix_w), (std::uint32_t)w);
	memory->write<std::uint32_t>(g.state + offsetof(RemoteState, pix_h), (std::uint32_t)h);
	memory->write<std::uint32_t>(g.state + offsetof(RemoteState, pix_stride), (std::uint32_t)w * 4);
	memory->write<std::uint32_t>(g.state + offsetof(RemoteState, tex_dirty), 1);
	return true;
}

void update_cb()
{
	if (!g.state)
		return;

	SkyCB cb{};
	memory->read_raw(g.state + offsetof(RemoteState, cbdata), &cb, sizeof(cb));
	cb.tint[0] = cb.tint[1] = cb.tint[2] = cb.tint[3] = 1.f;
	cb.time = (float)(GetTickCount64() % 600000ull) * 0.001f;
	int im = settings::skybox::shader;
	if (im < 0)
		im = 0;
	if (im > 3)
		im = 3;
	const bool video_ok = push_video();
	if (im == 3 && !video_ok)
		im = 0;
	if (im == 3 && !memory->read<std::uint32_t>(g.state + offsetof(RemoteState, vid_bound)))
		im = 0;
	cb.mode = (float)im;
	cb.cam[0] = cb.cam[1] = cb.cam[2] = 0.f;

	const std::uintptr_t base = module_base();
	if (base)
	{
		const std::uint64_t ve = memory->read<std::uint64_t>(base + Offsets::VisualEngine::Pointer);
		if (mem_valid(ve))
		{
			float vp[16]{};
			float vp2[16]{};
			memory->read_raw(ve + Offsets::VisualEngine::ViewMatrix, vp, sizeof(vp));
			memory->read_raw(ve + Offsets::VisualEngine::ViewMatrix, vp2, sizeof(vp2));
			if (std::memcmp(vp, vp2, sizeof(vp)) == 0)
			{
				float inv[16]{};
				bool ok = invert(vp, inv);
				if (ok)
				{
					for (int i = 0; i < 16; ++i)
					{
						if (!std::isfinite(inv[i]))
						{
							ok = false;
							break;
						}
					}
				}
				if (ok)
					std::memcpy(cb.inv_vp, inv, sizeof(inv));
			}
			const math::vector2 dim = memory->read<math::vector2>(ve + Offsets::VisualEngine::Dimensions);
			if (dim.x > 2.f && dim.y > 2.f && std::isfinite(dim.x) && std::isfinite(dim.y))
			{
				cb.res[0] = dim.x;
				cb.res[1] = dim.y;
			}
		}
	}

	memory->write_raw(g.state + offsetof(RemoteState, cbdata), &cb, sizeof(cb));
	memory->write<std::uint32_t>(g.state + offsetof(RemoteState, dirty), 1);
}

bool install()
{
	if (g.hooked)
		return true;

	const auto now = std::chrono::steady_clock::now();
	if (g_fail.time_since_epoch().count() != 0 &&
		now - g_fail < std::chrono::milliseconds(800))
		return false;

	const std::uintptr_t base = module_base();
	if (!base || !memory->get_process_handle() || !mem_alive())
	{
		g_fail = now;
		return false;
	}
	if (!local_d3d())
	{
		fail("local d3d");
		g_fail = now;
		return false;
	}
	if (!remote_d3d11())
	{
		fail("no d3d11");
		g_fail = now;
		return false;
	}
	if (!g.state)
		g.state = memory->alloc(0x1000, PAGE_READWRITE);
	if (!g.state)
	{
		fail("alloc");
		g_fail = now;
		return false;
	}
	const char* why = "no device";
	if (!find_d3d(why))
	{
		fail(why);
		g_fail = now;
		return false;
	}
	if (!g.ctx)
	{
		fail("no ctx");
		g_fail = now;
		return false;
	}
	if (!setup_meta())
	{
		fail("gpu meta");
		g_fail = now;
		return false;
	}
	std::uintptr_t sky_lo = 0, sky_hi = 0;
	if (!sky_range(base, &sky_lo, &sky_hi))
	{
		fail("sky range");
		g_fail = now;
		return false;
	}
	auto wipe_cave = []()
	{
		g.psset_thunk = 0;
		g.create_stub = 0;
		g.upload_stub = 0;
		g.bind_stub = 0;
		g.draw_thunk = 0;
		g.drawidx_thunk = 0;
		g.drawinst_thunk = 0;
		g.drawidxinst_thunk = 0;
		g.srv_thunk = 0;
		g.cave_n = 0;
	};
	auto try_draw_cave = [&](std::uintptr_t cand, bool instanced) -> bool
	{
		g.psset_thunk = cand;
		g.create_stub = cand + 0x200;
		g.upload_stub = cand + 0x370;
		g.draw_thunk = cand + 0x580;
		g.drawidx_thunk = cand + 0x690;
		g.bind_stub = cand + 0x7A0;
		g.drawinst_thunk = instanced ? cand + 0x8A0 : 0;
		g.drawidxinst_thunk = instanced ? cand + 0x9B0 : 0;
		auto up = make_upload_stub(g.state);
		auto ps = make_psset_thunk(g.state, g.create_stub, g.upload_stub, sky_lo, sky_hi, g.bind_stub);
		auto cr = make_create_stub(g.state);
		auto bn = make_bind_stub(g.state, 0);
		auto dw = make_draw_thunk(g.state, g.bind_stub,
			(std::uint32_t)offsetof(RemoteState, orig_draw),
			(std::uint32_t)offsetof(RemoteState, entered_draw));
		auto di = make_draw_thunk(g.state, g.bind_stub,
			(std::uint32_t)offsetof(RemoteState, orig_drawidx),
			(std::uint32_t)offsetof(RemoteState, entered_idx));
		if (up.size() > 0x210 || ps.size() > 0x200 || cr.size() > 0x170 || bn.size() > 0x100
			|| dw.size() > 0x110 || di.size() > 0x110)
		{
			debug::error("skyshader stub up=%zu ps=%zu cr=%zu bn=%zu dw=%zu di=%zu",
				up.size(), ps.size(), cr.size(), bn.size(), dw.size(), di.size());
			fail("stub size");
			return false;
		}
		if (!write_exec(g.upload_stub, up.data(), up.size())
			|| !write_exec(g.psset_thunk, ps.data(), ps.size())
			|| !write_exec(g.create_stub, cr.data(), cr.size())
			|| !write_exec(g.bind_stub, bn.data(), bn.size())
			|| !write_exec(g.draw_thunk, dw.data(), dw.size())
			|| !write_exec(g.drawidx_thunk, di.data(), di.size()))
		{
			wipe_cave();
			return false;
		}
		if (instanced)
		{
			const auto odi = (std::uintptr_t)memory->read<std::uint64_t>(
				g.state + offsetof(RemoteState, orig_drawinst));
			const auto odii = (std::uintptr_t)memory->read<std::uint64_t>(
				g.state + offsetof(RemoteState, orig_drawidxinst));
			if (!addr_ok(odi) || !addr_ok(odii))
			{
				g.drawinst_thunk = 0;
				g.drawidxinst_thunk = 0;
				instanced = false;
			}
			else
			{
				auto dwi = make_draw_thunk(g.state, g.bind_stub,
					(std::uint32_t)offsetof(RemoteState, orig_drawinst),
					(std::uint32_t)offsetof(RemoteState, entered_draw));
				auto dii = make_draw_thunk(g.state, g.bind_stub,
					(std::uint32_t)offsetof(RemoteState, orig_drawidxinst),
					(std::uint32_t)offsetof(RemoteState, entered_idx));
				if (dwi.size() > 0x110 || dii.size() > 0x110
					|| !write_exec(g.drawinst_thunk, dwi.data(), dwi.size())
					|| !write_exec(g.drawidxinst_thunk, dii.data(), dii.size()))
				{
					wipe_cave();
					return false;
				}
			}
		}
		g.cave = cand;
		g.cave_n = instanced ? 0xB00u : 0x8A0u;
		g.cave_alloc = false;
		return true;
	};
	std::uintptr_t ignore = 0;
	for (int attempt = 0; attempt < 4 && !g.cave; ++attempt)
	{
		const std::uintptr_t cand = find_dll_cave(0xB00, ignore);
		if (!cand)
			break;
		if (try_draw_cave(cand, true))
			break;
		if (g_why)
		{
			g_fail = now;
			return false;
		}
		ignore = cand;
	}
	if (!g.cave)
	{
		ignore = 0;
		for (int attempt = 0; attempt < 4 && !g.cave; ++attempt)
		{
			const std::uintptr_t cand = find_dll_cave(0x8A0, ignore);
			if (!cand)
				break;
			if (try_draw_cave(cand, false))
				break;
			if (g_why)
			{
				g_fail = now;
				return false;
			}
			ignore = cand;
		}
	}
	if (!g.cave)
	{
		ignore = 0;
		for (int attempt = 0; attempt < 4 && !g.cave; ++attempt)
		{
			const std::uintptr_t cand = find_dll_cave(0x580, ignore);
			if (!cand)
				break;
			g.psset_thunk = cand;
			g.create_stub = cand + 0x200;
			g.upload_stub = cand + 0x370;
			auto up = make_upload_stub(g.state);
			auto ps = make_psset_thunk(g.state, g.create_stub, g.upload_stub, sky_lo, sky_hi, 0);
			auto cr = make_create_stub(g.state);
			if (up.size() > 0x210 || ps.size() > 0x200 || cr.size() > 0x170)
			{
				debug::error("skyshader stub up=%zu ps=%zu cr=%zu",
					up.size(), ps.size(), cr.size());
				fail("stub size");
				g_fail = now;
				return false;
			}
			if (write_exec(g.upload_stub, up.data(), up.size())
				&& write_exec(g.psset_thunk, ps.data(), ps.size())
				&& write_exec(g.create_stub, cr.data(), cr.size()))
			{
				g.cave = cand;
				g.cave_n = 0x580;
				g.cave_alloc = false;
				break;
			}
			wipe_cave();
			ignore = cand;
		}
	}
	if (!g.cave)
	{
		fail("cave");
		g_fail = now;
		return false;
	}
	FlushInstructionCache(memory->get_process_handle(), (void*)g.cave, g.cave_n);
	if (!g.video_stub)
	{
		auto vid = make_video_stub(g.state);
		if (vid.size() <= 0x3F0)
		{
			std::uintptr_t ig = 0;
			for (int i = 0; i < 6 && !g.video_stub; ++i)
			{
				const std::uintptr_t cand = find_dll_cave(0x400, ig);
				if (!cand)
					break;
				if (write_exec(cand, vid.data(), vid.size()))
				{
					g.video_stub = cand;
					mark_cfg(cand);
					FlushInstructionCache(memory->get_process_handle(), (void*)cand, 0x400);
					break;
				}
				ig = cand;
			}
		}
	}
	if (g.video_stub && g.bind_stub)
	{
		auto bn = make_bind_stub(g.state, g.video_stub);
		if (bn.size() <= 0x100 && write_exec(g.bind_stub, bn.data(), bn.size()))
			FlushInstructionCache(memory->get_process_handle(), (void*)g.cave, g.cave_n);
	}
	if (!g.srv_thunk && g.bind_stub)
	{
		auto sv = make_srv_thunk(g.state, g.bind_stub);
		if (!sv.empty() && sv.size() <= 0xA0)
		{
			std::uintptr_t ig = 0;
			for (int i = 0; i < 6 && !g.srv_thunk; ++i)
			{
				const std::uintptr_t cand = find_dll_cave(0xA0, ig);
				if (!cand)
					break;
				if (write_exec(cand, sv.data(), sv.size()))
				{
					g.srv_thunk = cand;
					mark_cfg(cand);
					FlushInstructionCache(memory->get_process_handle(), (void*)cand, 0xA0);
					break;
				}
				ig = cand;
			}
		}
	}
	g.cfg_ps = mark_cfg(g.psset_thunk);
	g.cfg_create = mark_cfg(g.create_stub);
	if (g.upload_stub)
		mark_cfg(g.upload_stub);
	if (g.bind_stub)
		mark_cfg(g.bind_stub);
	if (g.srv_thunk)
		mark_cfg(g.srv_thunk);
	if (g.draw_thunk)
		mark_cfg(g.draw_thunk);
	if (g.drawidx_thunk)
		mark_cfg(g.drawidx_thunk);
	if (g.drawinst_thunk)
		mark_cfg(g.drawinst_thunk);
	if (g.drawidxinst_thunk)
		mark_cfg(g.drawidxinst_thunk);

	if (!patch_vtables())
	{
		fail("psset vt");
		g_fail = now;
		return false;
	}
	memory->write<std::uint64_t>(g.state + offsetof(RemoteState, orig_psset), g.orig_psset);

	g.module = base;
	g.hooked = true;
	update_cb();
	write_enabled(1);
	snapshot();
	if (!g.logged_ok)
	{
		g.logged_ok = true;
		debug::ok("skyshader on patches=%d cfg=%d/%d",
			g.npatch, g.cfg_ps ? 1 : 0, g.cfg_create ? 1 : 0);
	}
	return true;
}

void teardown()
{
	write_enabled(0);
	if (!g.hooked)
		return;
	unpatch();
	g.hooked = false;
}

}

void Tick()
{
	if (!mem_attached() || !mem_alive())
	{
		teardown();
		g.cave = 0;
		g.psset_thunk = 0;
		g.create_stub = 0;
		g.upload_stub = 0;
		g.bind_stub = 0;
		g.draw_thunk = 0;
		g.drawidx_thunk = 0;
		g.drawinst_thunk = 0;
		g.drawidxinst_thunk = 0;
		g.srv_thunk = 0;
		g.device = 0;
		g.ctx = 0;
		g.r_d3d11 = 0;
		g.orig_psset = 0;
		return;
	}

	const std::uintptr_t base = module_base();
	if (g.module && base && g.module != base)
	{
		teardown();
		if (g.state)
		{
			memory->free(g.state);
			g.state = 0;
		}
		if (g.cave && g.cave_alloc)
			memory->free(g.cave);
		g.cave = 0;
		if (g.dxbc)
		{
			memory->free(g.dxbc);
			g.dxbc = 0;
		}
		g.video_stub = 0;
		if (g.pix)
		{
			memory->free(g.pix);
			g.pix = 0;
		}
		g = {};
		g_why = nullptr;
	}

	if (!g.hooked && !install())
		return;

	update_cb();
	write_enabled(1);
	snapshot();
	if (g.hooked && !g.logged_hit && g_snap.swaps)
	{
		g.logged_hit = true;
		const std::uint64_t cb = g.state
			? memory->read<std::uint64_t>(g.state + offsetof(RemoteState, cb))
			: 0;
		const std::uint64_t cb2 = g.state
			? memory->read<std::uint64_t>(g.state + offsetof(RemoteState, cb2))
			: 0;
		const std::uint64_t cb3 = g.state
			? memory->read<std::uint64_t>(g.state + offsetof(RemoteState, cb3))
			: 0;
		debug::ok("skyshader swaps=%u cb=%d/%d/%d",
			g_snap.swaps, cb ? 1 : 0, cb2 ? 1 : 0, cb3 ? 1 : 0);
	}
	if (g.hooked && g.logged_hit && !g.logged_cb && g.state && g_snap.swaps > 8)
	{
		const std::uint32_t maps = memory->read<std::uint32_t>(
			g.state + offsetof(RemoteState, maps));
		const float t = memory->read<float>(
			g.state + offsetof(RemoteState, cbdata) + offsetof(SkyCB, time));
		g.logged_cb = true;
		logpick(maps != 0,
			"skyshader maps=%u time=%.2f", maps, t);
	}
	if (g.hooked && g.state && !g.logged_vid && settings::skybox::shader == 3)
	{
		const std::uint32_t pix_w = memory->read<std::uint32_t>(
			g.state + offsetof(RemoteState, pix_w));
		if (pix_w)
		{
			const std::uint32_t bound = memory->read<std::uint32_t>(
				g.state + offsetof(RemoteState, vid_bound));
			const std::uint64_t tex = memory->read<std::uint64_t>(
				g.state + offsetof(RemoteState, tex));
			const std::uint64_t srv = memory->read<std::uint64_t>(
				g.state + offsetof(RemoteState, srv));
			const std::uint32_t vcalls = memory->read<std::uint32_t>(
				g.state + offsetof(RemoteState, vid_calls));
			if (bound || vcalls || g_snap.swaps > 80)
			{
				g.logged_vid = true;
				logpick(bound != 0,
					"skyvideo gpu bound=%u tex=%d srv=%d stub=%d sky=%u pssrv=%u vid=%u draw=%u idx=%u",
					bound, tex ? 1 : 0, srv ? 1 : 0, g.video_stub ? 1 : 0,
					memory->read<std::uint32_t>(g.state + offsetof(RemoteState, sky_draw)),
					memory->read<std::uint32_t>(g.state + offsetof(RemoteState, srv_hits)),
					vcalls,
					g_snap.entered_draw, g_snap.entered_idx);
			}
		}
	}
}

void Stop()
{
	teardown();
	SkyVideo::Play({});
}

void Dump()
{
	snapshot();
	debug::info("skyshader hooked=%d patches=%d cfg=%d/%d",
		g_snap.hooked ? 1 : 0, g_snap.npatch,
		g_snap.cfg_ps ? 1 : 0, g_snap.cfg_create ? 1 : 0);
	const std::uint32_t maps = g.state
		? memory->read<std::uint32_t>(g.state + offsetof(RemoteState, maps))
		: 0;
	debug::info("skyshader enter ps=%u draw=%u idx=%u swaps=%u maps=%u",
		g_snap.entered_ps, g_snap.entered_draw, g_snap.entered_idx, g_snap.swaps, maps);
	debug::info("skyshader orig_ps=0x%llX cave=0x%llX ctx=0x%llX",
		(unsigned long long)g_snap.orig_psset,
		(unsigned long long)g_snap.cave,
		(unsigned long long)g_snap.ctx);
}

void run()
{
	static bool active = false;
	for (;;)
	{
		if (settings::skybox::enabled)
		{
			Tick();
			active = true;
		}
		else if (active)
		{
			Stop();
			active = false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

// The entry the client's own d3d11 build keeps in slot `slot`. The dll is the
// same file in both processes, so a table built here and shifted onto the
// client's base names the function the client itself calls - no dump offsets
// and no vtable guessing involved.
std::uint64_t remote_d3d_fn(bool context, int slot)
{
	if (slot < 0 || !local_d3d())
		return 0;
	void** vt = context ? g_ctx_vt : g_dev_vt;
	if (!vt)
		return 0;
	return (std::uint64_t)remap_fn(vt[slot]);
}

}
