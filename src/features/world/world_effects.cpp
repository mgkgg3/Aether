#define NOMINMAX
#define _USE_MATH_DEFINES
#include "world_effects.h"

#include <settings.h>
#include <game/game.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <cache/cache.h>
#include <debug/DebugConsole.h>
#include <features/worldgeo/WorldGeo.h>
#include <imgui/imgui.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

struct RbxColor3 {
    float r, g, b;
    bool operator==(const RbxColor3& o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const RbxColor3& o) const { return !(*this == o); }
};
static RbxColor3 to_rbx(const float c[3]) { return { c[0], c[1], c[2] }; }

static float clampf(float v, float lo, float hi)
{
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

static std::uint64_t find_service(const char* class_name, const char* name)
{
    if (!game::datamodel.address) return 0;

    try
    {
        std::vector<rbx::instance_t> kids = game::datamodel.get_children();
        for (rbx::instance_t& c : kids)
        {
            std::string cls;
            try { cls = c.get_class_name(); } catch (...) { continue; }
            if (cls == class_name) return c.address;
        }

        for (rbx::instance_t& c : kids)
        {
            std::string nm;
            try { nm = c.get_name(); } catch (...) { continue; }

            while (!nm.empty() && (nm.back() == ' ' || nm.back() == '\t')) nm.pop_back();
            if (nm == name) return c.address;
        }
    }
    catch (...) {}

    return 0;
}

static std::uint64_t get_lighting()
{
    return find_service("Lighting", "Lighting");
}
static std::uint64_t get_atmosphere(std::uint64_t L)
{
    if (!L) return 0;
    try { return rbx::instance_t(L).find_first_child_by_class("Atmosphere").address; }
    catch (...) { return 0; }
}
static std::uint64_t get_postfx(std::uint64_t L, const char* cls)
{
    if (!L) return 0;
    try { return rbx::instance_t(L).find_first_child_by_class(cls).address; }
    catch (...) { return 0; }
}

static constexpr float PI = 3.14159265f;
static constexpr int   SR = 44100;

struct LowPass {
    float s{ 0.f };
    float process(float x, float a) { return s = s + a*(x - s); }
};

static inline float xshift(unsigned& r)
{
    r ^= r<<13; r ^= r>>17; r ^= r<<5;
    return (float)(int)r / (float)0x7fffffffu;
}

static constexpr int RAIN_FRAMES = SR / 4;
static constexpr int RAIN_BUFS   = 3;

static HWAVEOUT            s_rain_wout = nullptr;
static WAVEHDR             s_rain_hdr[RAIN_BUFS]{};
static std::vector<short>  s_rain_buf[RAIN_BUFS];
static std::atomic<bool>   s_rain_running{ false };

static void fill_rain(short* buf, int n, float vol)
{
    static LowPass lp1, lp2;
    static unsigned rng = 0x11223344u;
    const float a = 1.f - expf(-2.f*PI*800.f/SR);
    for (int i = 0; i < n; i++) {
        float w = xshift(rng);
        float s = lp2.process(lp1.process(w, a), a*.6f);
        buf[i] = (short)(s * vol * 32767.f * 3.5f);
    }
}

static void rain_thread()
{
    WAVEFORMATEX wfx{}; wfx.wFormatTag=WAVE_FORMAT_PCM; wfx.nChannels=1;
    wfx.nSamplesPerSec=SR; wfx.wBitsPerSample=16; wfx.nBlockAlign=2;
    wfx.nAvgBytesPerSec=SR*2;
    if (waveOutOpen(&s_rain_wout, WAVE_MAPPER, &wfx,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR) return;
    for (int b=0;b<RAIN_BUFS;b++) {
        s_rain_buf[b].resize(RAIN_FRAMES);
        fill_rain(s_rain_buf[b].data(), RAIN_FRAMES, settings::world::effects_3d::rain_volume);
        s_rain_hdr[b]={};
        s_rain_hdr[b].lpData=(LPSTR)s_rain_buf[b].data();
        s_rain_hdr[b].dwBufferLength=(DWORD)(RAIN_FRAMES*2);
        waveOutPrepareHeader(s_rain_wout,&s_rain_hdr[b],sizeof(WAVEHDR));
        waveOutWrite(s_rain_wout,&s_rain_hdr[b],sizeof(WAVEHDR));
    }
    while (s_rain_running.load()) {
        for (int b=0;b<RAIN_BUFS;b++) {
            if (s_rain_hdr[b].dwFlags & WHDR_DONE) {
                waveOutUnprepareHeader(s_rain_wout,&s_rain_hdr[b],sizeof(WAVEHDR));
                fill_rain(s_rain_buf[b].data(), RAIN_FRAMES, settings::world::effects_3d::rain_volume);
                s_rain_hdr[b].dwFlags=0;
                waveOutPrepareHeader(s_rain_wout,&s_rain_hdr[b],sizeof(WAVEHDR));
                waveOutWrite(s_rain_wout,&s_rain_hdr[b],sizeof(WAVEHDR));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    waveOutReset(s_rain_wout);
    for (int b=0;b<RAIN_BUFS;b++) waveOutUnprepareHeader(s_rain_wout,&s_rain_hdr[b],sizeof(WAVEHDR));
    waveOutClose(s_rain_wout); s_rain_wout=nullptr;
}

static std::mutex              s_thunder_mtx;
static std::atomic<bool>       s_thunder_pending{ false };
static float                   s_thunder_vol     = 0.85f;

static void thunder_thread()
{
    const int FRAMES = (int)(SR * 1.8f);
    std::vector<short> buf(FRAMES);

    static unsigned rng = 0xdeadbeef;
    LowPass lp_crack, lp_rumble1, lp_rumble2;
    const float a_crack  = 1.f - expf(-2.f*PI*2200.f/SR);
    const float a_rumble = 1.f - expf(-2.f*PI*55.f/SR);

    float vol = s_thunder_vol;
    for (int i = 0; i < FRAMES; i++) {
        float t   = (float)i / SR;
        float w   = xshift(rng);

        float crack  = lp_crack.process(w, a_crack);
        float rumble = lp_rumble2.process(lp_rumble1.process(w, a_rumble), a_rumble * 0.7f);

        float env_crack  = expf(-18.f * t) * (t < 0.04f ? (t / 0.04f) : 1.f);
        float env_rumble = expf(-1.8f * t) * std::min(t / 0.05f, 1.f);

        float s = crack * env_crack * 0.6f + rumble * env_rumble * 1.4f;
        buf[i] = (short)(s * vol * 32767.f * 5.f);
    }

    HWAVEOUT wout = nullptr;
    WAVEFORMATEX wfx{}; wfx.wFormatTag=WAVE_FORMAT_PCM; wfx.nChannels=1;
    wfx.nSamplesPerSec=SR; wfx.wBitsPerSample=16; wfx.nBlockAlign=2;
    wfx.nAvgBytesPerSec=SR*2;
    if (waveOutOpen(&wout,WAVE_MAPPER,&wfx,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR) return;

    WAVEHDR hdr{}; hdr.lpData=(LPSTR)buf.data(); hdr.dwBufferLength=(DWORD)(FRAMES*2);
    waveOutPrepareHeader(wout,&hdr,sizeof(WAVEHDR));
    waveOutWrite(wout,&hdr,sizeof(WAVEHDR));

    while (!(hdr.dwFlags & WHDR_DONE))
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

    waveOutUnprepareHeader(wout,&hdr,sizeof(WAVEHDR));
    waveOutClose(wout);
}

static void trigger_thunder(float vol)
{
    s_thunder_vol = vol;
    std::thread(thunder_thread).detach();
}

void world_effects::update_rain_sound()
{
    bool want = settings::world::effects_3d::rain_sound && settings::world::effects_3d::rain_mode > 0;
    if (want && !s_rain_running.load()) {
        s_rain_running = true;
        std::thread(rain_thread).detach();
    } else if (!want && s_rain_running.load()) {
        s_rain_running = false;
    }
}

template <typename T>
static bool write_changed(std::uint64_t addr, const T& value)
{
    try
    {
        const T cur = memory->read<T>(addr);
        if (cur == value) return false;
        memory->write<T>(addr, value);
        return true;
    }
    catch (...) { return false; }
}

template <typename T>
static void force_write(std::uint64_t addr, const T& value)
{
    try { memory->write<T>(addr, value); } catch (...) {}
}

static bool write_changed_v3(std::uint64_t addr, const math::vector3& v)
{
    try
    {
        const math::vector3 cur = memory->read<math::vector3>(addr);
        if (cur.x == v.x && cur.y == v.y && cur.z == v.z) return false;
        memory->write<math::vector3>(addr, v);
        return true;
    }
    catch (...) { return false; }
}

namespace wown
{
    enum class feat : std::uint8_t
    {
        none = 0, shadow, time, ambient, outdoor, brightness, exposure,
        light, fog, env, shift, atmo,
    };

    enum class field : std::uint8_t
    {
        shadows = 0, clock, sun, moon, grad_top, grad_bot,
        amb, outdoor, bri, expo, light_col, light_dir,
        fog_start, fog_end, fog_col, env_diff, env_spec,
        shift_top, shift_bot, atmo, count,
    };

    constexpr int k_field_count = (int)field::count;

    static std::atomic<std::uint8_t> g_owner[k_field_count]{};

    inline void claim(feat f, const field* fields, int n)
    {
        if (!fields || n <= 0) return;
        for (int i = 0; i < n; ++i)
        {
            const int idx = (int)fields[i];
            if (idx < 0 || idx >= k_field_count) continue;
            g_owner[idx].store((std::uint8_t)f);
        }
    }

    inline bool owns(feat f, field fd)
    {
        const int idx = (int)fd;
        if (idx < 0 || idx >= k_field_count) return false;
        return g_owner[idx].load() == (std::uint8_t)f;
    }

    // Claims each field that nobody else owns yet and answers how many it got.
    // The atmosphere's fog fallback uses this rather than claim(): the Fog
    // section drives the same three fields, and whichever of the two came
    // second has to stand down instead of the two of them writing over each
    // other every tick.
    inline int try_claim(feat f, const field* fields, int n)
    {
        if (!fields || n <= 0) return 0;

        int got = 0;
        for (int i = 0; i < n; ++i)
        {
            const int idx = (int)fields[i];
            if (idx < 0 || idx >= k_field_count) continue;

            std::uint8_t held = (std::uint8_t)feat::none;
            if (g_owner[idx].compare_exchange_strong(held, (std::uint8_t)f) ||
                held == (std::uint8_t)f)
                ++got;
        }
        return got;
    }

    inline void release(feat f, const field* fields, int n)
    {
        if (!fields || n <= 0) return;
        for (int i = 0; i < n; ++i)
        {
            const int idx = (int)fields[i];
            if (idx < 0 || idx >= k_field_count) continue;
            std::uint8_t expect = (std::uint8_t)f;
            g_owner[idx].compare_exchange_strong(expect, (std::uint8_t)feat::none);
        }
    }
}

static std::atomic<bool> s_invalidate_ready{ false };

static void lighting_invalidate();
static void invalidate_sky();

// Anything that writes the Lighting or the Sky under it says so, so the world
// tab and the skybox presets share the one invalidation rather than each
// growing its own.
void world_effects::notify_lighting_changed()
{
    lighting_invalidate();
}

// What a write into the Sky instance itself needs: the client builds the sky
// environment it draws from that instance and keeps a flag saying the one it
// has is still current, so the fields can be written and the client keeps
// drawing the sky it built earlier.  The world tab's own sky rows clear that
// flag (invalidate_sky, next to apply_sky); the preset table writes the same
// instance, so it clears it too - without it a preset lands in the instance
// and never reaches a frame.
void world_effects::notify_sky_changed()
{
    invalidate_sky();
    lighting_invalidate();
}

static void lighting_invalidate()
{
    try
    {
        const std::uint64_t rv = rbx::visualengine_t::render_view();
        if (!rv) return;

        static std::uint32_t  s_quiet = 0;
        static std::uintptr_t s_fields[8]{};
        static int            s_field_count = 0;

        if (s_field_count == 0)
        {

            const std::uintptr_t candidates[] = {
                Offsets::RenderView::LightingValid, 0x278, 0x140, 0x130, 0x160,
            };
            const std::size_t count = sizeof(candidates) / sizeof(candidates[0]);

            for (std::size_t i = 0; i < count; ++i)
            {
                const std::uintptr_t off = candidates[i];
                if (!off) continue;

                bool already = false;
                for (int s = 0; s < s_field_count; ++s)
                    if (s_fields[s] == off) already = true;
                if (already) continue;

                if (!(s_quiet & (1u << i)))
                {
                    s_quiet |= (1u << i);
                    const std::uint64_t window = memory->read<std::uint64_t>(rv + off);
                    const bool is_pointer = window >= 0x10000ull && window < 0x7fffffffffffull;
                    const bool is_flag    = (window & 0xFFull) <= 1ull;
                    debug::log("world", "lighting invalidate: clearing RenderView+0x%llx (reads %s) - if the tab works, "
                        "pin it into Offsets::RenderView::LightingValid",
                        (unsigned long long)off,
                        is_pointer ? "a pointer" : (is_flag ? "a flag byte" : "a non-flag value"));
                }

                if (s_field_count < (int)(sizeof(s_fields) / sizeof(s_fields[0])))
                    s_fields[s_field_count++] = off;
            }
        }

        s_invalidate_ready.store(true);

        for (int i = 0; i < s_field_count; ++i)
            memory->write<unsigned char>(rv + s_fields[i], 0);
    }
    catch (...) {}
}

static std::atomic<bool> s_lighting_found{ false };
static std::atomic<bool> s_atmosphere_found{ false };

bool world_effects::lighting_found()   { return s_lighting_found.load(); }
bool world_effects::atmosphere_found() { return s_atmosphere_found.load(); }
bool world_effects::invalidate_ready() { return s_invalidate_ready.load(); }

static void probe_world()
{
    const std::uint64_t L = get_lighting();
    s_lighting_found.store(L != 0);
    s_atmosphere_found.store(L != 0 && get_atmosphere(L) != 0);
}

static void verify_ambient(std::uint64_t L)
{
    if (!settings::world::lighting::ambient_on) return;

    static std::uint64_t s_last_ms = 0;
    const std::uint64_t now = (std::uint64_t)GetTickCount64();
    if (now - s_last_ms < 2000) return;

    try
    {
        const RbxColor3 a = memory->read<RbxColor3>(L + Offsets::Lighting::Ambient);
        const float* want = settings::world::lighting::ambient;
        const bool differs = std::fabs(a.r - want[0]) > 0.02f ||
                             std::fabs(a.g - want[1]) > 0.02f ||
                             std::fabs(a.b - want[2]) > 0.02f;
        if (!differs) return;

        s_last_ms = now;
        debug::log("world", "ambient reads (%.2f,%.2f,%.2f) but (%.2f,%.2f,%.2f) was written - the map is "
            "rewriting Lighting between ticks (our writes run 120 times a second, so this should be rare)",
            a.r, a.g, a.b, want[0], want[1], want[2]);
    }
    catch (...) {}
}

static void tick_shadow(std::uint64_t L)
{
    static const wown::field fields[] = { wown::field::shadows };
    static bool          was = false;
    static std::uint8_t  bak = 1;

    if (!L) return;
    bool dirty = false;

    if (settings::world::lighting::no_shadow)
    {
        if (!was)
        {
            wown::claim(wown::feat::shadow, fields, 1);
            try { bak = memory->read<std::uint8_t>(L + Offsets::Lighting::GlobalShadows); }
            catch (...) { bak = 1; }
            was = true;
        }

        if (wown::owns(wown::feat::shadow, wown::field::shadows))
            dirty |= write_changed<std::uint8_t>(L + Offsets::Lighting::GlobalShadows, 0);
    }

    else if (was)
    {
        if (wown::owns(wown::feat::shadow, wown::field::shadows))
        {
            write_changed<std::uint8_t>(L + Offsets::Lighting::GlobalShadows, bak);
            dirty = true;
        }

        wown::release(wown::feat::shadow, fields, 1);
        was = false;
    }

    if (dirty) lighting_invalidate();
}

static bool clock_is_double(std::uint64_t L)
{
    const std::uint64_t addr = L + Offsets::Lighting::ClockTime;
    try
    {
        const float  cur_f = memory->read<float>(addr);
        const double cur_d = memory->read<double>(addr);

        const bool f_sane = std::isfinite(cur_f) && cur_f >= 0.f && cur_f <= 24.f &&
                            (cur_f == 0.f || cur_f >= 1e-3f);
        const bool d_sane = std::isfinite(cur_d) && cur_d >= 0.0 && cur_d <= 24.0;

        if (d_sane != f_sane) return d_sane;
        if (!d_sane) return false;

        const std::uint32_t hi = memory->read<std::uint32_t>(addr + 4);
        return hi >= 0x3FE00000u && hi <= 0x40380000u;
    }
    catch (...) { return false; }
}

static void tick_time(std::uint64_t L)
{
    static const wown::field fields[] = {
        wown::field::clock, wown::field::sun, wown::field::moon,
        wown::field::grad_top, wown::field::grad_bot,
    };

    static bool          was = false;
    static bool          as_double = false;
    static float         bak_clock = 12.f;
    static math::vector3 bak_sun{}, bak_moon{}, bak_gt{}, bak_gb{};

    if (!L) return;

    const bool on = settings::world::lighting::time_on;
    bool dirty = false;

    if (on)
    {
        if (!was)
        {
            wown::claim(wown::feat::time, fields, 5);
            as_double = clock_is_double(L);

            try
            {
                const std::uint64_t clock = L + Offsets::Lighting::ClockTime;
                bak_clock = as_double ? (float)memory->read<double>(clock)
                                      : memory->read<float>(clock);
                bak_sun  = memory->read<math::vector3>(L + Offsets::Lighting::SunPosition);
                bak_moon = memory->read<math::vector3>(L + Offsets::Lighting::MoonPosition);
                bak_gt   = memory->read<math::vector3>(L + Offsets::Lighting::GradientTop);
                bak_gb   = memory->read<math::vector3>(L + Offsets::Lighting::GradientBottom);
            }
            catch (...) {}
            was = true;
        }

        float t = settings::world::lighting::clock_time;
        if (t < 0.f) t = 0.f;
        if (t > 24.f) t = 24.f;

        const float sun_angle = (t / 24.f - 0.25f) * 2.f * PI;
        const float sun_y = sinf(sun_angle);
        const math::vector3 sun_pos{ 0.f, sun_y, -cosf(sun_angle) };
        const math::vector3 moon_pos{ -sun_pos.x, -sun_pos.y, -sun_pos.z };

        math::vector3 grad_top{}, grad_bot{};
        if (sun_y > 0.15f)
        {
            grad_top = { 0.35f, 0.65f, 0.95f };
            grad_bot = { 0.75f, 0.85f, 0.95f };
        }
        else if (sun_y > 0.f)
        {
            const float st = sun_y / 0.15f;
            grad_top = { 0.15f + st * 0.2f, 0.15f + st * 0.5f, 0.25f + st * 0.7f };
            grad_bot = { 0.95f, 0.45f + st * 0.4f, 0.25f + st * 0.7f };
        }
        else
        {
            float nt = (-sun_y) / 0.3f;
            if (nt > 1.f) nt = 1.f;
            const float inv = 1.f - nt;
            grad_top = { 0.05f + inv * 0.1f, 0.05f + inv * 0.1f, 0.12f + inv * 0.13f };
            grad_bot = { 0.08f + inv * 0.07f, 0.08f + inv * 0.07f, 0.15f + inv * 0.1f };
        }

        if (wown::owns(wown::feat::time, wown::field::clock))
        {
            const std::uint64_t clock = L + Offsets::Lighting::ClockTime;
            if (as_double) force_write<double>(clock, (double)t);
            else           force_write<float>(clock, t);
            dirty = true;
        }

        if (wown::owns(wown::feat::time, wown::field::sun))
            dirty |= write_changed_v3(L + Offsets::Lighting::SunPosition, sun_pos);

        if (wown::owns(wown::feat::time, wown::field::moon))
            dirty |= write_changed_v3(L + Offsets::Lighting::MoonPosition, moon_pos);

        if (wown::owns(wown::feat::time, wown::field::grad_top))
            dirty |= write_changed_v3(L + Offsets::Lighting::GradientTop, grad_top);

        if (wown::owns(wown::feat::time, wown::field::grad_bot))
            dirty |= write_changed_v3(L + Offsets::Lighting::GradientBottom, grad_bot);
    }

    else if (was)
    {
        if (wown::owns(wown::feat::time, wown::field::clock))
        {
            const std::uint64_t clock = L + Offsets::Lighting::ClockTime;
            if (as_double) force_write<double>(clock, (double)bak_clock);
            else           force_write<float>(clock, bak_clock);
            dirty = true;
        }

        if (wown::owns(wown::feat::time, wown::field::sun))
            force_write<math::vector3>(L + Offsets::Lighting::SunPosition, bak_sun);

        if (wown::owns(wown::feat::time, wown::field::moon))
            force_write<math::vector3>(L + Offsets::Lighting::MoonPosition, bak_moon);

        if (wown::owns(wown::feat::time, wown::field::grad_top))
            force_write<math::vector3>(L + Offsets::Lighting::GradientTop, bak_gt);

        if (wown::owns(wown::feat::time, wown::field::grad_bot))
            force_write<math::vector3>(L + Offsets::Lighting::GradientBottom, bak_gb);

        wown::release(wown::feat::time, fields, 5);
        was = false;
        dirty = true;
    }

    if (dirty) lighting_invalidate();
}

static void tick_color_field(std::uint64_t L, wown::feat who, wown::field fd,
    std::uintptr_t offset, const float* rgb, bool on)
{
    static bool         was[(int)wown::field::count]{};
    static RbxColor3    bak[(int)wown::field::count]{};

    const int idx = (int)fd;
    if (!L || idx < 0 || idx >= wown::k_field_count) return;

    const wown::field one[1] = { fd };

    bool dirty = false;

    if (on)
    {
        if (!was[idx])
        {
            wown::claim(who, one, 1);
            try { bak[idx] = memory->read<RbxColor3>(L + offset); } catch (...) {}
            was[idx] = true;
        }

        if (wown::owns(who, fd))
            dirty |= write_changed<RbxColor3>(L + offset, to_rbx(rgb));
    }

    else if (was[idx])
    {
        if (wown::owns(who, fd))
        {
            force_write<RbxColor3>(L + offset, bak[idx]);
            dirty = true;
        }

        wown::release(who, one, 1);
        was[idx] = false;
    }

    if (dirty) lighting_invalidate();
}

static void tick_float_field(std::uint64_t L, wown::feat who, wown::field fd,
    std::uintptr_t offset, float value, bool on, float lo, float hi)
{
    static bool  was[(int)wown::field::count]{};
    static float bak[(int)wown::field::count]{};

    const int idx = (int)fd;
    if (!L || idx < 0 || idx >= wown::k_field_count) return;

    const wown::field one[1] = { fd };
    float v = value;
    if (v < lo) v = lo;
    if (v > hi) v = hi;

    bool dirty = false;

    if (on)
    {
        if (!was[idx])
        {
            wown::claim(who, one, 1);
            try { bak[idx] = memory->read<float>(L + offset); } catch (...) {}
            was[idx] = true;
        }

        if (wown::owns(who, fd))
            dirty |= write_changed<float>(L + offset, v);
    }

    else if (was[idx])
    {
        if (wown::owns(who, fd))
        {
            force_write<float>(L + offset, bak[idx]);
            dirty = true;
        }

        wown::release(who, one, 1);
        was[idx] = false;
    }

    if (dirty) lighting_invalidate();
}

static void tick_fog(std::uint64_t L)
{
    static const wown::field fields[] = {
        wown::field::fog_start, wown::field::fog_end, wown::field::fog_col,
    };

    static bool      was = false;
    static float     bak_s = 0.f, bak_e = 100000.f;
    static RbxColor3 bak_c{};

    if (!L) return;

    const bool on = settings::world::lighting::fog_on;
    bool dirty = false;

    if (on)
    {
        if (!was)
        {
            wown::claim(wown::feat::fog, fields, 3);
            try
            {
                bak_s = memory->read<float>(L + Offsets::Lighting::FogStart);
                bak_e = memory->read<float>(L + Offsets::Lighting::FogEnd);
                bak_c = memory->read<RbxColor3>(L + Offsets::Lighting::FogColor);
            }
            catch (...) {}
            was = true;
        }

        float start = settings::world::lighting::fog_start;
        if (start < 0.f) start = 0.f;
        if (start > 100.f) start = 100.f;

        float end = settings::world::lighting::fog_end;
        if (end > 2000.f) end = 2000.f;
        if (end < start + 1.f) end = start + 1.f;

        if (wown::owns(wown::feat::fog, wown::field::fog_start))
            dirty |= write_changed<float>(L + Offsets::Lighting::FogStart, start);

        if (wown::owns(wown::feat::fog, wown::field::fog_end))
            dirty |= write_changed<float>(L + Offsets::Lighting::FogEnd, end);

        if (wown::owns(wown::feat::fog, wown::field::fog_col))
            dirty |= write_changed<RbxColor3>(L + Offsets::Lighting::FogColor,
                to_rbx(settings::world::lighting::fog_color));
    }

    else if (was)
    {
        if (wown::owns(wown::feat::fog, wown::field::fog_start))
            force_write<float>(L + Offsets::Lighting::FogStart, bak_s);

        if (wown::owns(wown::feat::fog, wown::field::fog_end))
            force_write<float>(L + Offsets::Lighting::FogEnd, bak_e);

        if (wown::owns(wown::feat::fog, wown::field::fog_col))
            force_write<RbxColor3>(L + Offsets::Lighting::FogColor, bak_c);

        wown::release(wown::feat::fog, fields, 3);
        was = false;
        dirty = true;
    }

    if (dirty) lighting_invalidate();
}

static void tick_light(std::uint64_t L)
{
    static const wown::field fields[] = {
        wown::field::light_col, wown::field::light_dir,
    };

    static bool          was = false;
    static RbxColor3     bak_col{};
    static math::vector3 bak_dir{};

    if (!L) return;

    const bool on = settings::world::lighting::light_on;
    bool dirty = false;

    if (on)
    {
        if (!was)
        {
            wown::claim(wown::feat::light, fields, 2);
            try
            {
                bak_col = memory->read<RbxColor3>(L + Offsets::Lighting::LightColor);
                bak_dir = memory->read<math::vector3>(L + Offsets::Lighting::LightDirection);
            }
            catch (...) {}
            was = true;
        }

        if (wown::owns(wown::feat::light, wown::field::light_col))
            dirty |= write_changed<RbxColor3>(L + Offsets::Lighting::LightColor,
                to_rbx(settings::world::lighting::light_color));

        if (wown::owns(wown::feat::light, wown::field::light_dir))
            dirty |= write_changed_v3(L + Offsets::Lighting::LightDirection,
                { settings::world::lighting::light_dir[0],
                  settings::world::lighting::light_dir[1],
                  settings::world::lighting::light_dir[2] });
    }

    else if (was)
    {
        if (wown::owns(wown::feat::light, wown::field::light_col))
            force_write<RbxColor3>(L + Offsets::Lighting::LightColor, bak_col);

        if (wown::owns(wown::feat::light, wown::field::light_dir))
            force_write<math::vector3>(L + Offsets::Lighting::LightDirection, bak_dir);

        wown::release(wown::feat::light, fields, 2);
        was = false;
        dirty = true;
    }

    if (dirty) lighting_invalidate();
}

static void tick_env(std::uint64_t L)
{
    if (!L) return;

    tick_float_field(L, wown::feat::env, wown::field::env_diff,
        Offsets::Lighting::EnvironmentDiffuseScale,
        settings::world::lighting::env_diffuse_scale,
        settings::world::lighting::env_on, 0.f, 2.f);

    tick_float_field(L, wown::feat::env, wown::field::env_spec,
        Offsets::Lighting::EnvironmentSpecularScale,
        settings::world::lighting::env_specular_scale,
        settings::world::lighting::env_on, 0.f, 2.f);
}

static bool lighting_any_on()
{
    namespace L = settings::world::lighting;
    return L::time_on || L::brightness_on || L::exposure_on || L::ambient_on ||
           L::outdoor_on || L::fog_on || L::shift_on || L::env_on ||
           L::no_shadow || L::light_on;
}

static void apply_lighting(std::uint64_t L)
{
    if (!L) return;

    tick_shadow(L);
    tick_time(L);

    tick_float_field(L, wown::feat::brightness, wown::field::bri,
        Offsets::Lighting::Brightness, settings::world::lighting::brightness,
        settings::world::lighting::brightness_on, 0.f, 20.f);

    tick_float_field(L, wown::feat::exposure, wown::field::expo,
        Offsets::Lighting::ExposureCompensation,
        settings::world::lighting::exposure_compensation,
        settings::world::lighting::exposure_on, -5.f, 5.f);

    tick_color_field(L, wown::feat::ambient, wown::field::amb,
        Offsets::Lighting::Ambient, settings::world::lighting::ambient,
        settings::world::lighting::ambient_on);

    tick_color_field(L, wown::feat::outdoor, wown::field::outdoor,
        Offsets::Lighting::OutdoorAmbient, settings::world::lighting::outdoor_ambient,
        settings::world::lighting::outdoor_on);

    tick_color_field(L, wown::feat::shift, wown::field::shift_top,
        Offsets::Lighting::ColorShift_Top, settings::world::lighting::color_shift_top,
        settings::world::lighting::shift_on);

    tick_color_field(L, wown::feat::shift, wown::field::shift_bot,
        Offsets::Lighting::ColorShift_Bottom, settings::world::lighting::color_shift_bottom,
        settings::world::lighting::shift_on);

    tick_fog(L);
    tick_light(L);
    tick_env(L);

    if (lighting_any_on()) lighting_invalidate();

    verify_ambient(L);
}

// ---- the atmosphere ----
//
// An atmosphere is an instance under the Lighting, and a place does not have to
// have one: the place this build is pointed at has none, so the override used
// to do nothing at all there and said nothing about it.  Two paths now:
//
//   * the instance exists: the six fields are written straight into it, which
//     is the whole feature;
//   * it does not: the same rows stand in through the Lighting's own fog (fog
//     end from Density, fog start from Offset, fog colour mixed from Haze
//     Colour by Haze), which is the one thing every place has.  Those three
//     fields are claimed only if the Fog section is not already driving them,
//     and either side stands down rather than the two overwriting each other.
//
// The status the tab shows says which of the two is in play, so "it does
// nothing" is answerable from the page instead of from a guess.
static std::atomic<bool> s_atmo_on_fog{ false };

bool world_effects::atmosphere_on_fog() { return s_atmo_on_fog.load(); }

static void apply_atmosphere_fog(std::uint64_t L, bool on)
{
    static const wown::field fields[] = {
        wown::field::fog_start, wown::field::fog_end, wown::field::fog_col,
    };

    static bool      was = false;
    static bool      have_backup = false;
    static float     bak_s = 0.f, bak_e = 100000.f;
    static RbxColor3 bak_c{};

    if (!L) return;

    if (!on)
    {
        if (!was) return;

        if (wown::owns(wown::feat::atmo, wown::field::fog_start))
            force_write<float>(L + Offsets::Lighting::FogStart, bak_s);
        if (wown::owns(wown::feat::atmo, wown::field::fog_end))
            force_write<float>(L + Offsets::Lighting::FogEnd, bak_e);
        if (wown::owns(wown::feat::atmo, wown::field::fog_col))
            force_write<RbxColor3>(L + Offsets::Lighting::FogColor, bak_c);

        wown::release(wown::feat::atmo, fields, 3);
        was = false;
        have_backup = false;
        s_atmo_on_fog.store(false);
        lighting_invalidate();
        return;
    }

    namespace atmo = settings::world::atmosphere;

    if (!was)
    {
        const int got = wown::try_claim(wown::feat::atmo, fields, 3);
        if (got == 0)
        {
            // The Fog section owns all three: the atmosphere's rows have
            // nothing left to move, and saying so once is better than silently
            // fighting it every tick.
            static bool warned = false;
            if (!warned)
            {
                warned = true;
                debug::log("world", "no Atmosphere in this place and the Fog section owns the fog "
                    "- the atmosphere override has nothing to write (turn the Fog section off)");
            }
            s_atmo_on_fog.store(false);
            return;
        }

        try
        {
            bak_s = memory->read<float>(L + Offsets::Lighting::FogStart);
            bak_e = memory->read<float>(L + Offsets::Lighting::FogEnd);
            bak_c = memory->read<RbxColor3>(L + Offsets::Lighting::FogColor);
            have_backup = true;
        }
        catch (...) { have_backup = false; }

        was = true;
        s_atmo_on_fog.store(got > 0);

        debug::log("world", "atmosphere: no Atmosphere instance in this place - standing in on the "
            "Lighting's fog (fog end from Density, fog start from Offset, fog colour from Haze Colour)");
    }

    const float d = clampf(atmo::density, 0.f, 1.f);
    const float o = clampf(atmo::offset, 0.f, 1.f);
    const float h = clampf(atmo::haze, 0.f, 10.f);

    // Density 0 with no haze is the place's own air: nothing to write, and the
    // fields go back to what they held (the switch stays where it is).
    const bool wants = d > 0.001f || h > 0.001f;

    bool dirty = false;

    if (wown::owns(wown::feat::atmo, wown::field::fog_end))
        dirty |= write_changed<float>(L + Offsets::Lighting::FogEnd,
            wants ? std::max(40.f, 4000.f * (1.f - d)) : (have_backup ? bak_e : 100000.f));

    if (wown::owns(wown::feat::atmo, wown::field::fog_start))
        dirty |= write_changed<float>(L + Offsets::Lighting::FogStart,
            wants ? std::max(0.f, 4000.f * (1.f - d)) * o * 0.4f : (have_backup ? bak_s : 0.f));

    if (wown::owns(wown::feat::atmo, wown::field::fog_col))
    {
        // Haze mixes the haze colour toward the decay colour the way the
        // instance would: more haze, more of the decay's tint in the air.
        const float t = clampf(h / 10.f, 0.f, 1.f);
        float c[3];
        for (int i = 0; i < 3; ++i)
            c[i] = atmo::color[i] + (atmo::decay[i] - atmo::color[i]) * t;

        dirty |= write_changed<RbxColor3>(L + Offsets::Lighting::FogColor,
            wants ? to_rbx(c) : (have_backup ? bak_c : to_rbx(atmo::color)));
    }

    if (dirty) lighting_invalidate();
}

static void apply_atmosphere(std::uint64_t L, bool force_off)
{
    static const wown::field fields[] = { wown::field::atmo };
    static bool was = false;

    if (!L) return;

    const bool on = settings::world::atmosphere::enabled && !force_off;

    // One lookup for the tick, used by both paths.
    const std::uint64_t A = get_atmosphere(L);

    // No Atmosphere under this Lighting: the rows go on the fog instead.
    if (!A)
    {
        apply_atmosphere_fog(L, on);
        if (was)
        {
            // The instance came or went with the place: give back what this
            // path owns before the other one takes over.
            wown::release(wown::feat::atmo, fields, 1);
            was = false;
        }
        return;
    }

    // An instance exists again: drop whatever the fog fallback was holding.
    apply_atmosphere_fog(L, false);

    if (on)
    {
        if (!was)
        {
            wown::claim(wown::feat::atmo, fields, 1);
            was = true;
        }

        if (!wown::owns(wown::feat::atmo, wown::field::atmo))
            return;

        namespace atmo = settings::world::atmosphere;
        bool dirty = false;
        try {
            dirty |= write_changed<float>(A + Offsets::Atmosphere::Density, clampf(atmo::density, 0.f, 1.f));
            dirty |= write_changed<float>(A + Offsets::Atmosphere::Haze,    clampf(atmo::haze,    0.f, 10.f));
            dirty |= write_changed<float>(A + Offsets::Atmosphere::Glare,   clampf(atmo::glare,   0.f, 10.f));
            dirty |= write_changed<float>(A + Offsets::Atmosphere::Offset,  clampf(atmo::offset,  0.f, 1.f));
            dirty |= write_changed<RbxColor3>(A + Offsets::Atmosphere::Color, to_rbx(atmo::color));
            dirty |= write_changed<RbxColor3>(A + Offsets::Atmosphere::Decay, to_rbx(atmo::decay));
        }
        catch (...) { return; }

        // One line every few seconds while the atmosphere is on, so "it stopped
        // working" can be answered from the log instead of guessed at.
        static unsigned long long s_atmo_log_at = 0;
        const unsigned long long atmo_now = ::GetTickCount64();
        if (atmo_now >= s_atmo_log_at)
        {
            s_atmo_log_at = atmo_now + 5000ull;
            debug::log("world", "atmosphere applied at 0x%llX (density %.2f haze %.2f glare %.2f)%s",
                (unsigned long long)A, atmo::density, atmo::haze, atmo::glare,
                dirty ? " - the game had different values" : "");
        }

        lighting_invalidate();
        return;
    }

    if (was)
    {
        wown::release(wown::feat::atmo, fields, 1);
        was = false;
    }
}

static void apply_postfx()
{
    std::uint64_t L = get_lighting(); if (!L) return;
    auto write_pfx = [&](const char* cls, auto fn) {
        std::uint64_t h = get_postfx(L, cls);
        if (h) try { fn(h); } catch (...) {}
    };
    write_pfx("BloomEffect", [](uint64_t h){
        memory->write<bool> (h+Offsets::BloomEffect::Enabled,   settings::world::postfx::bloom::enabled);
        memory->write<float>(h+Offsets::BloomEffect::Intensity, settings::world::postfx::bloom::intensity);
        memory->write<float>(h+Offsets::BloomEffect::Size,      settings::world::postfx::bloom::size);
        memory->write<float>(h+Offsets::BloomEffect::Threshold, settings::world::postfx::bloom::threshold);
    });
    write_pfx("ColorCorrectionEffect", [](uint64_t h){
        memory->write<bool>     (h+Offsets::ColorCorrectionEffect::Enabled,    settings::world::postfx::colorcorrect::enabled);
        memory->write<RbxColor3>(h+Offsets::ColorCorrectionEffect::TintColor,  to_rbx(settings::world::postfx::colorcorrect::tint));
        memory->write<float>    (h+Offsets::ColorCorrectionEffect::Brightness, settings::world::postfx::colorcorrect::brightness);
        memory->write<float>    (h+Offsets::ColorCorrectionEffect::Contrast,   settings::world::postfx::colorcorrect::contrast);
    });
    write_pfx("DepthOfFieldEffect", [](uint64_t h){
        memory->write<bool> (h+Offsets::DepthOfFieldEffect::Enabled,       settings::world::postfx::dof::enabled);
        memory->write<float>(h+Offsets::DepthOfFieldEffect::FocusDistance, settings::world::postfx::dof::focus_distance);
        memory->write<float>(h+Offsets::DepthOfFieldEffect::InFocusRadius, settings::world::postfx::dof::in_focus_radius);
        memory->write<float>(h+Offsets::DepthOfFieldEffect::FarIntensity,  settings::world::postfx::dof::far_intensity);
        memory->write<float>(h+Offsets::DepthOfFieldEffect::NearIntensity, settings::world::postfx::dof::near_intensity);
    });
    write_pfx("SunRaysEffect", [](uint64_t h){
        memory->write<bool> (h+Offsets::SunRaysEffect::Enabled,   settings::world::postfx::sunrays::enabled);
        memory->write<float>(h+Offsets::SunRaysEffect::Intensity, settings::world::postfx::sunrays::intensity);
        memory->write<float>(h+Offsets::SunRaysEffect::Spread,    settings::world::postfx::sunrays::spread);
    });

    if (settings::world::shaders::glossy &&
        !wown::owns(wown::feat::env, wown::field::env_diff) &&
        !wown::owns(wown::feat::env, wown::field::env_spec))
    {
        try {
            memory->write<float>(L+Offsets::Lighting::EnvironmentSpecularScale, settings::world::shaders::glossy_specular);
            memory->write<float>(L+Offsets::Lighting::EnvironmentDiffuseScale,  settings::world::shaders::glossy_diffuse);
        } catch (...) {}
    }

    namespace pf = settings::world::postfx;
    if (pf::bloom::enabled || pf::colorcorrect::enabled || pf::dof::enabled ||
        pf::sunrays::enabled || pf::colorgrade::enabled)
        lighting_invalidate();
}

static void invalidate_sky()
{
    try
    {
        // Both chains to the render view: a stale VisualEngine field is the one
        // reason a sky write lands in the instance and never reaches a frame.
        const std::uint64_t rv = rbx::visualengine_t::render_view();
        if (!rv) return;
        memory->write<unsigned char>(rv + Offsets::RenderView::SkyValid, 0);
    }
    catch (...) {}
}

static std::uint64_t get_sky(std::uint64_t L)
{
    if (!L) return 0;
    try
    {
        const std::uint64_t sky = memory->read<std::uint64_t>(L + Offsets::Lighting::Sky);
        if (sky) return sky;
    }
    catch (...) {}
    try { return rbx::instance_t(L).find_first_child_by_class("Sky").address; }
    catch (...) { return 0; }
}

static std::uint64_t get_terrain()
{
    std::uint64_t ws = game::workspace.address;

    if (!ws && game::datamodel.address)
    {
        try { ws = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace); }
        catch (...) { ws = 0; }
    }

    if (!ws) ws = find_service("Workspace", "Workspace");
    if (!ws) return 0;

    try { return rbx::instance_t(ws).find_first_child_by_class("Terrain").address; }
    catch (...) { return 0; }
}

static float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

static void apply_sky()
{
    if (!settings::world::sky_slots::enabled) return;

    const std::uint64_t L = get_lighting(); if (!L) return;
    const std::uint64_t sky = get_sky(L); if (!sky) return;

    try
    {
        memory->write<float>(sky + Offsets::Sky::SunAngularSize,
            std::min(60.f, std::max(0.f, settings::world::sky_slots::sun_angular)));
        memory->write<float>(sky + Offsets::Sky::MoonAngularSize,
            std::min(60.f, std::max(0.f, settings::world::sky_slots::moon_angular)));

        // The orientation belongs to the Skybox rows: writing it here too made
        // two loops race for the same float, and the slower one won half the
        // time.  Size only.
        invalidate_sky();
    }
    catch (...) {}
}

static void apply_terrain()
{
    if (!settings::world::terrain::enabled) return;

    const std::uint64_t terrain = get_terrain(); if (!terrain) return;

    try
    {
        memory->write<float>(terrain + Offsets::Terrain::GrassLength,
            clamp01(settings::world::terrain::grass_length));

        const std::uint64_t mc = memory->read<std::uint64_t>(
            terrain + Offsets::Terrain::MaterialColors);
        if (mc > 0x10000ull)
        {
            const float* g = settings::world::terrain::grass_color;
            unsigned char rgb[3] = {
                (unsigned char)(clamp01(g[0]) * 255.f),
                (unsigned char)(clamp01(g[1]) * 255.f),
                (unsigned char)(clamp01(g[2]) * 255.f),
            };
            memory->write_raw(mc + Offsets::MaterialColors::Grass, rgb, 3);
        }

        memory->write<RbxColor3>(terrain + Offsets::Terrain::WaterColor,
            to_rbx(settings::world::terrain::water_color));
        memory->write<float>(terrain + Offsets::Terrain::WaterReflectance,
            clamp01(settings::world::terrain::water_reflectance));
        memory->write<float>(terrain + Offsets::Terrain::WaterTransparency,
            clamp01(settings::world::terrain::water_transparency));
    }
    catch (...) {}
}

static void apply_colorgrade()
{
    if (!settings::world::postfx::colorgrade::enabled) return;

    const std::uint64_t L = get_lighting(); if (!L) return;
    const std::uint64_t cg = get_postfx(L, "ColorGradingEffect"); if (!cg) return;

    int tm = settings::world::postfx::colorgrade::tonemapper;
    if (tm < 0) tm = 0;
    if (tm > 1) tm = 1;

    try
    {
        memory->write<bool>(cg + Offsets::ColorGradingEffect::Enabled, true);
        memory->write<std::int32_t>(cg + Offsets::ColorGradingEffect::TonemapperPreset, tm);
        lighting_invalidate();
    }
    catch (...) {}
}

static float s_rainbow_hue = 0.f;
static float s_fog_phase   = 0.f;

static void apply_presets(float dt_sec)
{
    using namespace settings::world;

    if (presets::night) {
        lighting::brightness_on = true;
        lighting::brightness = 0.05f;
        lighting::time_on = true;
        lighting::clock_time = 0.f;
        lighting::exposure_on = true;
        lighting::exposure_compensation = -1.5f;
        lighting::ambient_on = true;
        lighting::ambient[0]=0.02f; lighting::ambient[1]=0.02f; lighting::ambient[2]=0.08f;
        lighting::outdoor_on = true;
        lighting::outdoor_ambient[0]=0.01f; lighting::outdoor_ambient[1]=0.01f; lighting::outdoor_ambient[2]=0.06f;
        atmosphere::enabled = true;
        atmosphere::density = 0.6f; atmosphere::haze = 0.3f; atmosphere::glare = 0.f;
        atmosphere::color[0]=0.05f; atmosphere::color[1]=0.06f; atmosphere::color[2]=0.12f;
        postfx::sunrays::enabled = false;
    }

    if (presets::sunset) {
        lighting::brightness_on = true;
        lighting::brightness = 2.5f;
        lighting::time_on = true;
        lighting::clock_time = 18.5f;
        lighting::exposure_on = true;
        lighting::exposure_compensation = 0.3f;
        lighting::ambient_on = true;
        lighting::ambient[0]=0.85f; lighting::ambient[1]=0.45f; lighting::ambient[2]=0.15f;
        lighting::outdoor_on = true;
        lighting::outdoor_ambient[0]=0.9f; lighting::outdoor_ambient[1]=0.5f; lighting::outdoor_ambient[2]=0.2f;
        lighting::shift_on = true;
        lighting::color_shift_top[0]=0.4f; lighting::color_shift_top[1]=0.15f; lighting::color_shift_top[2]=0.f;
        lighting::fog_on = true;
        lighting::fog_color[0]=0.9f; lighting::fog_color[1]=0.55f; lighting::fog_color[2]=0.2f;
        lighting::fog_start=0.f; lighting::fog_end=1200.f;
        atmosphere::enabled=true; atmosphere::density=0.35f; atmosphere::haze=0.6f; atmosphere::glare=0.3f;
        atmosphere::color[0]=1.0f; atmosphere::color[1]=0.72f; atmosphere::color[2]=0.45f;
        postfx::sunrays::enabled=true; postfx::sunrays::intensity=0.5f; postfx::sunrays::spread=0.9f;
        postfx::bloom::enabled=true; postfx::bloom::intensity=0.8f; postfx::bloom::size=30.f; postfx::bloom::threshold=0.8f;
    }

    if (presets::rainbow) {
        lighting::shift_on = true;
        s_rainbow_hue += dt_sec * 25.f;
        if (s_rainbow_hue > 360.f) s_rainbow_hue -= 360.f;
        float h = s_rainbow_hue, s6 = h/60.f;
        int   i = (int)s6 % 6;
        float f = s6 - floorf(s6), p = 0.1f, q = 0.5f*(1.f-f), t2 = 0.5f*f;
        float rv,gv,bv;
        switch(i) {
            case 0: rv=0.5f;  gv=t2;   bv=p;    break;
            case 1: rv=q;     gv=0.5f; bv=p;    break;
            case 2: rv=p;     gv=0.5f; bv=t2;   break;
            case 3: rv=p;     gv=q;    bv=0.5f; break;
            case 4: rv=t2;    gv=p;    bv=0.5f; break;
            default:rv=0.5f;  gv=p;    bv=q;    break;
        }
        lighting::color_shift_top[0]=rv; lighting::color_shift_top[1]=gv; lighting::color_shift_top[2]=bv;
        lighting::color_shift_bottom[0]=bv; lighting::color_shift_bottom[1]=rv; lighting::color_shift_bottom[2]=gv;
    }

    if (presets::fog_pulse) {
        lighting::fog_on = true;
        s_fog_phase += dt_sec * 0.4f;
        float t = (sinf(s_fog_phase * PI * 2.f) + 1.f) * 0.5f;
        lighting::fog_end = 150.f + t * 1200.f;
        lighting::fog_start = 0.f;
        atmosphere::enabled = true;
        atmosphere::density  = 0.3f + t * 0.5f;
        atmosphere::haze     = t * 0.8f;
    }
}

static float s_storm_lightning_timer = 8.f;
static float s_storm_flash_alpha     = 0.f;
static float s_storm_thunder_delay   = 0.f;
static bool  s_storm_thunder_pending = false;

static void schedule_next_lightning()
{ s_storm_lightning_timer = 4.f + (float)(rand()%14000)/1000.f; }

static void storm_apply(bool v2)
{

    settings::world::effects_3d::rain_mode = v2 ? 2 : 1;
    settings::world::effects_3d::rain_intensity=3.5f;
    settings::world::effects_3d::rain_speed=1.4f;
    settings::world::effects_3d::wind_x=-14.f; settings::world::effects_3d::wind_z=-6.f;
    settings::world::effects_3d::ripples=true; settings::world::effects_3d::ripple_size=1.8f;
    settings::world::lighting::brightness_on=true; settings::world::lighting::brightness=0.4f;
    settings::world::lighting::exposure_on=true; settings::world::lighting::exposure_compensation=-0.8f;
    settings::world::lighting::time_on=true; settings::world::lighting::clock_time=16.5f;
    settings::world::lighting::ambient_on=true;
    settings::world::lighting::ambient[0]=0.35f; settings::world::lighting::ambient[1]=0.38f; settings::world::lighting::ambient[2]=0.45f;
    settings::world::lighting::outdoor_on=true;
    settings::world::lighting::outdoor_ambient[0]=0.30f; settings::world::lighting::outdoor_ambient[1]=0.33f; settings::world::lighting::outdoor_ambient[2]=0.42f;
    settings::world::lighting::fog_on=true;
    settings::world::lighting::fog_start=0.f; settings::world::lighting::fog_end=400.f;
    settings::world::lighting::fog_color[0]=0.30f; settings::world::lighting::fog_color[1]=0.32f; settings::world::lighting::fog_color[2]=0.38f;
    settings::world::postfx::bloom::enabled=true; settings::world::postfx::bloom::intensity=1.4f;
    settings::world::postfx::bloom::size=28.f; settings::world::postfx::bloom::threshold=0.85f;
    settings::world::effects_3d::rain_sound=true; settings::world::effects_3d::rain_volume=0.85f;
}

static void storm_clear()
{
    settings::world::effects_3d::rain_mode=0; settings::world::effects_3d::wind_x=0.f; settings::world::effects_3d::wind_z=0.f;
    settings::world::lighting::brightness_on=false; settings::world::lighting::exposure_on=false;
    settings::world::lighting::time_on=false; settings::world::lighting::ambient_on=false;
    settings::world::lighting::outdoor_on=false; settings::world::lighting::fog_on=false;
    settings::world::postfx::bloom::enabled=false;
    settings::world::effects_3d::rain_sound=false;
}

void world_effects::run()
{
    schedule_next_lightning();
    auto last = std::chrono::steady_clock::now();
    for (;;) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        dt = std::min(dt, 0.1f);

        probe_world();

        const std::uint64_t L = get_lighting();
        apply_lighting(L);
        apply_atmosphere(L, false);
        apply_postfx();
        apply_sky();
        apply_terrain();
        apply_colorgrade();
        apply_presets(dt);

        bool want_rain_snd = settings::world::effects_3d::rain_sound && settings::world::effects_3d::rain_mode > 0;
        if (want_rain_snd  && !s_rain_running.load()) update_rain_sound();
        if (!want_rain_snd &&  s_rain_running.load()) update_rain_sound();

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

static constexpr float HRP_ABOVE_GROUND = 3.0f;
static float  s_ground_y      = -1e9f;
static bool   s_ground_valid  = false;
static float  s_hrp_y_prev    = 0.f;
static float  s_hrp_y         = 0.f;
static float  s_hrp_x         = 0.f;
static float  s_hrp_z         = 0.f;
static bool   s_hrp_valid     = false;

static float terrain_noise(float wx, float wz, float amp = 1.2f)
{
    int ix=(int)floorf(wx/8.f), iz=(int)floorf(wz/8.f);
    float fx=(wx/8.f)-floorf(wx/8.f), fz=(wz/8.f)-floorf(wz/8.f);
    fx=fx*fx*(3.f-2.f*fx); fz=fz*fz*(3.f-2.f*fz);
    auto hf=[](int a,int b)->float{
        unsigned v=(unsigned)(a*1619+b*31337); v^=v>>16; v*=0x45d9f3bu; v^=v>>16;
        return (float)(v&0xffffu)/65535.f;
    };
    float v=hf(ix,iz)*(1-fx)*(1-fz)+hf(ix+1,iz)*fx*(1-fz)+hf(ix,iz+1)*(1-fx)*fz+hf(ix+1,iz+1)*fx*fz;
    return (v*2.f-1.f)*amp;
}

static float sample_ground_y(float dt_sec)
{
    {
        std::lock_guard<std::mutex> lk(cache::mtx);
        auto it = cache::cached_local_player.parts.find("HumanoidRootPart");
        if (it != cache::cached_local_player.parts.end() && it->second.address)
        {
            try {
                rbx::primitive_t prim = it->second.get_primitive();
                if (prim.address)
                {
                    math::vector3 hrp_pos = prim.get_position();
                    float hrp_y   = hrp_pos.y;
                    float surf_y  = hrp_y - HRP_ABOVE_GROUND;

                    bool warped = false;
                    {
                        float dxw = hrp_pos.x - s_hrp_x;
                        float dzw = hrp_pos.z - s_hrp_z;
                        if (s_hrp_valid && (dxw * dxw + dzw * dzw) > 60.f * 60.f)
                            warped = true;
                    }

                    s_hrp_x = hrp_pos.x;
                    s_hrp_z = hrp_pos.z;
                    s_hrp_y = hrp_y;
                    s_hrp_valid = true;

                    float vy = (dt_sec > 0.f) ? (hrp_y - s_hrp_y_prev) / dt_sec : 0.f;
                    s_hrp_y_prev = hrp_y;

                    if (warped) {
                        s_ground_y     = surf_y;
                        s_ground_valid = true;
                        return s_ground_y;
                    }

                    if (!s_ground_valid) {
                        s_ground_y     = surf_y;
                        s_ground_valid = true;
                    } else {
                        if (surf_y < s_ground_y) {
                            s_ground_y += (surf_y - s_ground_y) * 0.25f;
                        } else if (vy <= 1.5f) {
                            s_ground_y += (surf_y - s_ground_y) * 0.04f;
                        }
                    }

                    return s_ground_y;
                }
            } catch (...) {}
        }
    }

    s_hrp_valid = false;
    try
    {
        std::uint64_t cam_addr = memory->read<std::uint64_t>(
            game::workspace.address + Offsets::Workspace::CurrentCamera);
        if (cam_addr)
        {
            rbx::camera_t cam{ cam_addr };
            math::vector3 cp = cam.get_position();

            s_hrp_x = cp.x;
            s_hrp_z = cp.z;

            float surf_y = cp.y - 12.f;

            if (!s_ground_valid || fabsf(surf_y - s_ground_y) > 40.f)
            {

                s_ground_y     = surf_y;
                s_ground_valid = true;
            }
            else
            {
                s_ground_y += (surf_y - s_ground_y) * 0.08f;
            }
            return s_ground_y;
        }
    }
    catch (...) {}

    return s_ground_valid ? s_ground_y : 0.f;
}

struct WorldParticle {
    float wx,wy,wz, vx,vy,vz, rot,rotspd, size, ground_y, ground_fade;

    float last_y = 0.f;
    bool  surface_checked = false;
};
struct Ripple        { float wx,wz,wy, radius,max_r, age,life; };

static std::vector<WorldParticle> g_snow3d, g_rain3d, g_snow_ground;
static std::vector<Ripple>        g_ripples;

static void draw_snowflake_shape(ImDrawList* dl, float cx, float cy,
                                 float r, float angle, ImU32 col, ImU32 col_core)
{
    const float PI3 = 3.14159265f / 3.f;
    for (int i = 0; i < 6; ++i)
    {
        float a  = angle + i * PI3;
        float ca = cosf(a), sa = sinf(a);
        float ex = cx + ca * r, ey = cy + sa * r;
        dl->AddLine(ImVec2(cx, cy), ImVec2(ex, ey), col, 1.0f);

        float bl = r * 0.42f;
        float bx = cx + ca * r * 0.55f;
        float by = cy + sa * r * 0.55f;
        for (int b = -1; b <= 1; b += 2)
        {
            float ba = a + b * PI3;
            dl->AddLine(ImVec2(bx, by),
                        ImVec2(bx + cosf(ba) * bl, by + sinf(ba) * bl),
                        col, 0.8f);
        }
    }
    dl->AddCircleFilled(ImVec2(cx, cy), r * 0.20f, col_core, 6);
}
static float g_snow_acc = 0.f, g_rain_acc = 0.f;

// Weather volume, in studs. The radius is measured from the world every frame
// instead of being fixed, so the fall-off sits at the wall of whatever space the
// player is in; the cap keeps it from spreading over half the map in the open.
static float s_weather_radius = 46.f;
static constexpr float k_weather_radius_min = 10.f;
static constexpr float k_weather_radius_max = 46.f;

// Particles actually drawn versus dropped by the V4 occlusion test, for the
// diagnostic below.
static int s_weather_drawn  = 0;
static int s_weather_culled = 0;

struct SurfaceAABB { float x, z, hx, hz, top_y; };
static std::vector<SurfaceAABB> g_surface_cache;

static std::vector<SurfaceAABB>  g_world_geo;
static std::mutex                g_world_geo_mtx;

void world_effects::render(float screen_w, float screen_h)
{
    using namespace settings::world;

    static auto s_last_frame = std::chrono::steady_clock::now();
    auto   now = std::chrono::steady_clock::now();
    float  dt  = std::chrono::duration<float>(now - s_last_frame).count();
    s_last_frame = now;
    dt = std::min(dt, 0.1f);

    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    static int s_storm_last = 0;
    if (effects_3d::storm_mode && s_storm_last != effects_3d::storm_mode)
    {
        storm_apply(effects_3d::storm_mode == 2);
        schedule_next_lightning();
    }
    if (!effects_3d::storm_mode && s_storm_last != 0) storm_clear();
    s_storm_last = effects_3d::storm_mode;

    if (effects_3d::storm_mode)
    {
        s_storm_lightning_timer -= dt;
        if (s_storm_flash_alpha > 0.f)
        {
            ImU32 flash_col = IM_COL32(255, 255, 255,
                (int)(std::min(s_storm_flash_alpha, 1.f) * 200.f));
            draw->AddRectFilled(ImVec2(0, 0), ImVec2(screen_w, screen_h), flash_col);
            s_storm_flash_alpha -= dt * 4.f;
        }
        if (s_storm_thunder_pending)
        {
            s_storm_thunder_delay -= dt;
            if (s_storm_thunder_delay <= 0.f)
            {
                s_storm_thunder_pending = false;
                trigger_thunder(0.85f);
            }
        }
        if (s_storm_lightning_timer <= 0.f)
        {
            s_storm_flash_alpha = 0.85f + (float)(rand() % 1000) / 3333.f;
            s_storm_thunder_delay   = 0.4f + (float)(rand() % 3000) / 1000.f;
            s_storm_thunder_pending = true;
            schedule_next_lightning();
        }
    }

    bool do_snow = effects_3d::snow_mode > 0;
    bool do_rain = effects_3d::rain_mode > 0;
    bool snow_v2 = effects_3d::snow_mode >= 2;
    bool rain_v2 = effects_3d::rain_mode >= 2;
    // V3 and up sample the real surface under each particle so weather lands on
    // roofs; V4 additionally drops whatever a wall stands between us and it.
    const bool snow_elev = effects_3d::snow_mode >= 3;
    const bool rain_elev = effects_3d::rain_mode >= 3;
    const bool snow_occl = effects_3d::snow_mode >= 4;
    const bool rain_occl = effects_3d::rain_mode >= 4;

    // Declared out here because the draw pass below is a separate block and needs
    // the same two values.
    math::vector3 eye{ s_hrp_x, s_hrp_y, s_hrp_z };
    float         SPAWN_RADIUS = k_weather_radius_max;

    if (do_snow || do_rain)
    {
        float ground_y = sample_ground_y(dt);
        float cx = s_hrp_x, cz = s_hrp_z;

        // Size the volume from the geometry in front of the camera: a small room
        // gives a small volume, so the weather stops at its walls instead of
        // spraying particles through them, and open ground grows it to the cap.
        // Smoothed, or stepping past a corner would pop the whole volume.
        eye = worldgeo::eye_position(eye);
        const float want_radius = worldgeo::open_radius(eye, k_weather_radius_max);
        s_weather_radius += (want_radius - s_weather_radius) * std::min(1.f, dt * 4.f);
        SPAWN_RADIUS = std::max(k_weather_radius_min, s_weather_radius);

        const float VIEW_HEIGHT  = 30.f;

        // Caps follow the volume so a small opening gets fewer particles instead
        // of the same count packed into it; the floor keeps a light drizzle alive.
        auto weather_cap = [&](float base) -> int
        {
            const float scale = (SPAWN_RADIUS / 50.f) * (SPAWN_RADIUS / 50.f);
            return (int)std::max(24.f, base * scale);
        };

        if (do_snow)
        {
            float intensity = std::max(0.f, effects_3d::snow_intensity);

            g_snow_acc += intensity * (snow_elev ? 260.f : 120.f) * dt;
            int spawn_n = (int)g_snow_acc;
            g_snow_acc -= (float)spawn_n;

            const int snow_cap = weather_cap((snow_elev ? 700.f : 500.f) * intensity);
            for (int i = 0; i < spawn_n; i++)
            {
                if ((int)g_snow3d.size() >= snow_cap) break;
                WorldParticle p{};
                p.wx = cx + ((float)(rand()%2001)/1000.f - 1.f) * SPAWN_RADIUS;
                p.wz = cz + ((float)(rand()%2001)/1000.f - 1.f) * SPAWN_RADIUS;
                p.wy = ground_y + VIEW_HEIGHT + (float)(rand()%1000)/1000.f * 10.f;

                p.vx = effects_3d::wind_x * (0.5f + (float)(rand()%1000)/2000.f);
                p.vy = -(effects_3d::snow_speed * (snow_elev ? 11.5f : 0.8f) * (1.f + (float)(rand()%400)/1000.f));
                p.vz = effects_3d::wind_z * (0.5f + (float)(rand()%1000)/2000.f);
                p.rot    = (float)(rand()%628)/100.f;
                p.rotspd = ((float)(rand()%200)/100.f - 1.f) * 2.f;

                p.size   = snow_elev ? (0.018f + (float)(rand()%50)/2200.f)
                                   : (0.08f + (float)(rand()%100)/1000.f);
                p.ground_y    = ground_y;
                p.ground_fade = 1.f;
                p.last_y      = p.wy;
                g_snow3d.push_back(p);
            }

            for (auto it = g_snow3d.begin(); it != g_snow3d.end(); )
            {
                it->last_y = it->wy;
                it->wx += it->vx * dt;
                it->wy += it->vy * dt;
                it->wz += it->vz * dt;
                it->rot += it->rotspd * dt;

                if (snow_elev)
                {
                    float surf = 0.f;
                    if (worldgeo::ground_y_at(it->wx, it->wz, it->last_y, surf))
                    {
                        it->ground_y = surf;
                        it->surface_checked = true;
                    }
                    else
                    {

                        it->ground_y = ground_y;
                        it->surface_checked = true;
                    }
                }

                float dist_to_ground = it->wy - it->ground_y;
                it->ground_fade = std::max(0.f, std::min(1.f, dist_to_ground / 1.5f));

                bool landed = snow_elev
                    ? (it->surface_checked && it->last_y >= it->ground_y && it->wy < it->ground_y)
                    : (it->wy < it->ground_y);

                const bool lost_surface = snow_elev && it->surface_checked &&
                    it->wy < it->ground_y - 1.5f;
                bool out_of_bounds =
                    (it->wy > ground_y + VIEW_HEIGHT + 15.f)
                    || lost_surface
                    || fabsf(it->wx - cx) > SPAWN_RADIUS + 5.f
                    || fabsf(it->wz - cz) > SPAWN_RADIUS + 5.f;

                if (landed)
                {

                    const int patch_cap = snow_elev ? 520 : 320;
                    if (snow_v2 && (int)g_snow_ground.size() < patch_cap)
                    {
                        WorldParticle gp = *it;
                        gp.wy       = it->ground_y;
                        gp.vx = gp.vy = gp.vz = 0.f;

                        gp.ground_fade = snow_elev ? 3.2f : 1.6f;
                        g_snow_ground.push_back(gp);
                    }
                    it = g_snow3d.erase(it);
                }
                else if (out_of_bounds) it = g_snow3d.erase(it);
                else ++it;
            }
        }
        else
        {
            g_snow3d.clear();
            g_snow_ground.clear();
        }

        if (!snow_v2 && !g_snow_ground.empty())
            g_snow_ground.clear();

        for (auto it = g_snow_ground.begin(); it != g_snow_ground.end(); )
        {
            it->ground_fade -= dt * (snow_elev ? 0.55f : 0.45f);
            if (it->ground_fade <= 0.f) it = g_snow_ground.erase(it);
            else ++it;
        }

        if (do_rain)
        {
            float intensity = std::max(0.f, effects_3d::rain_intensity);
            g_rain_acc += intensity * 300.f * dt;
            int spawn_n = (int)g_rain_acc;
            g_rain_acc -= (float)spawn_n;

            for (int i = 0; i < spawn_n; i++)
            {
                if ((int)g_rain3d.size() >= weather_cap(800.f * intensity)) break;
                WorldParticle p{};
                p.wx = cx + ((float)(rand()%2001)/1000.f - 1.f) * SPAWN_RADIUS;
                p.wz = cz + ((float)(rand()%2001)/1000.f - 1.f) * SPAWN_RADIUS;
                p.wy = ground_y + VIEW_HEIGHT + (float)(rand()%1000)/1000.f * 5.f;
                p.vx = effects_3d::wind_x * (0.8f + (float)(rand()%400)/2000.f);
                p.vy = -(14.f * effects_3d::rain_speed * (0.9f + (float)(rand()%200)/1000.f));
                p.vz = effects_3d::wind_z * (0.8f + (float)(rand()%400)/2000.f);
                p.size   = 0.05f + (float)(rand()%60)/2000.f;
                p.ground_y    = ground_y;
                p.ground_fade = 1.f;
                p.last_y      = p.wy;
                g_rain3d.push_back(p);
            }

            for (auto it = g_rain3d.begin(); it != g_rain3d.end(); )
            {
                it->last_y = it->wy;
                it->wx += it->vx * dt;
                it->wy += it->vy * dt;
                it->wz += it->vz * dt;

                if (rain_elev)
                {
                    float surf = 0.f;
                    if (worldgeo::ground_y_at(it->wx, it->wz, it->last_y, surf))
                    {
                        it->ground_y = surf;
                        it->surface_checked = true;
                    }
                    else
                    {

                        it->ground_y = ground_y;
                        it->surface_checked = true;
                    }
                }

                float dist_to_ground = it->wy - it->ground_y;
                it->ground_fade = std::max(0.f, std::min(1.f, dist_to_ground / 1.0f));

                bool hit_ground = rain_elev
                    ? (it->surface_checked && it->last_y >= it->ground_y && it->wy < it->ground_y)
                    : (it->wy <= it->ground_y);
                if (hit_ground && effects_3d::ripples)
                {
                    Ripple r{};
                    r.wx = it->wx; r.wz = it->wz; r.wy = it->ground_y;
                    r.radius  = 0.f;
                    r.max_r   = 0.6f * effects_3d::ripple_size;
                    r.age     = 0.f;
                    r.life    = 0.45f;
                    g_ripples.push_back(r);
                }

                const bool lost_surface = rain_elev && it->surface_checked &&
                    it->wy < it->ground_y - 1.5f;
                bool dead = hit_ground
                    || lost_surface
                    || (it->wy > ground_y + VIEW_HEIGHT + 10.f)
                    || fabsf(it->wx - cx) > SPAWN_RADIUS + 5.f
                    || fabsf(it->wz - cz) > SPAWN_RADIUS + 5.f;
                if (dead) it = g_rain3d.erase(it);
                else ++it;
            }

            for (auto it = g_ripples.begin(); it != g_ripples.end(); )
            {
                it->age += dt;
                it->radius = (it->age / it->life) * it->max_r;
                if (it->age >= it->life) it = g_ripples.erase(it);
                else ++it;
            }
        }
        else
        {
            g_rain3d.clear();
            g_ripples.clear();
        }
    }
    else
    {
        g_snow3d.clear();
        g_rain3d.clear();
        g_ripples.clear();
    }

    if (game::visengine.address)
    {
        const math::matrix4 view = game::visengine.get_viewmatrix();
        const math::vector2 dims = game::visengine.get_dimensions();

        s_weather_drawn  = 0;
        s_weather_culled = 0;

        if (do_snow && !g_snow3d.empty())
        {
            const float* sc = effects_3d::snow_color;
            for (const auto& p : g_snow3d)
            {
                if (snow_occl && !worldgeo::is_visible(eye, {p.wx, p.wy, p.wz}))
                {
                    ++s_weather_culled;
                    continue;
                }

                s_weather_drawn++;

                math::vector2 sp{};
                if (!game::visengine.world_to_screen({p.wx, p.wy, p.wz}, sp, dims, view))
                    continue;

                float alpha = p.ground_fade;
                if (alpha <= 0.f) continue;

                ImU32 col = IM_COL32(
                    (int)(sc[0]*255.f), (int)(sc[1]*255.f), (int)(sc[2]*255.f),
                    (int)(alpha * 220.f));

                float depth = std::max(1.f, fabsf(view.m[2][0]*p.wx + view.m[2][1]*p.wy + view.m[2][2]*p.wz + view.m[2][3]));
                float ss    = (p.size * screen_h) / depth;
                ss = std::max(1.f, std::min(ss, 12.f));

                if (snow_elev)
                {

                    draw->AddCircleFilled(ImVec2(sp.x, sp.y), ss * 1.45f,
                        IM_COL32((int)(sc[0]*255.f), (int)(sc[1]*255.f), (int)(sc[2]*255.f),
                            (int)(alpha * 50.f)), 8);
                    draw->AddCircleFilled(ImVec2(sp.x, sp.y), ss, col, 8);
                }
                else if (snow_v2)
                {

                    if (ss > 2.5f)
                        draw_snowflake_shape(draw, sp.x, sp.y, ss, p.rot, col,
                            IM_COL32(255, 255, 255, (int)(p.ground_fade * 255.f)));
                    else
                        draw->AddCircleFilled(ImVec2(sp.x, sp.y), ss, col, 6);
                }
                else
                {

                    float cosR = cosf(p.rot) * ss;
                    float sinR = sinf(p.rot) * ss;
                    ImVec2 c(sp.x, sp.y);
                    ImVec2 a(c.x - cosR + sinR, c.y - sinR - cosR);
                    ImVec2 b(c.x + cosR + sinR, c.y + sinR - cosR);
                    ImVec2 d(c.x - cosR - sinR, c.y - sinR + cosR);
                    ImVec2 e(c.x + cosR - sinR, c.y + sinR + cosR);
                    draw->AddQuadFilled(a, b, e, d, col);
                }
            }
        }

        if (do_snow && snow_v2 && !g_snow_ground.empty())
        {

            const float* sc = effects_3d::snow_color;
            for (const auto& p : g_snow_ground)
            {
                math::vector2 sp{};
                if (!game::visengine.world_to_screen({p.wx, p.wy, p.wz}, sp, dims, view))
                    continue;

                float melt = std::max(0.f, std::min(1.f, p.ground_fade));
                float depth = std::max(1.f, fabsf(view.m[2][0]*p.wx + view.m[2][1]*p.wy + view.m[2][2]*p.wz + view.m[2][3]));
                float rx = (p.size * 2.4f * screen_h) / depth;
                rx = std::max(1.f, std::min(rx, 14.f));
                float ry = std::max(0.5f, rx * 0.3f);

                int a = (int)(melt * 200.f);
                draw->AddEllipseFilled(ImVec2(sp.x, sp.y), ImVec2(rx + 1.5f, ry + 0.8f),
                    IM_COL32((int)(sc[0]*120.f), (int)(sc[1]*140.f), (int)(sc[2]*200.f), a / 4), 0.f, 10);
                draw->AddEllipseFilled(ImVec2(sp.x, sp.y), ImVec2(rx, ry),
                    IM_COL32((int)(sc[0]*255.f), (int)(sc[1]*255.f), (int)(sc[2]*255.f), a), 0.f, 10);
            }
        }

        if (do_rain && !g_rain3d.empty())
        {
            const float* rc = effects_3d::rain_color;
            for (const auto& p : g_rain3d)
            {
                if (rain_occl && !worldgeo::is_visible(eye, {p.wx, p.wy, p.wz}))
                {
                    ++s_weather_culled;
                    continue;
                }

                s_weather_drawn++;

                math::vector2 sp{};
                if (!game::visengine.world_to_screen({p.wx, p.wy, p.wz}, sp, dims, view))
                    continue;

                float alpha = p.ground_fade;
                if (alpha <= 0.f) continue;

                float depth = std::max(1.f, fabsf(view.m[2][0]*p.wx + view.m[2][1]*p.wy + view.m[2][2]*p.wz + view.m[2][3]));
                float ss    = (0.12f * screen_h) / depth;
                ss = std::max(2.f, std::min(ss, 18.f));

                math::vector2 sp2{};
                float tail_wy = p.wy - p.vy * 0.04f;
                float tail_wx = p.wx - p.vx * 0.04f;
                float tail_wz = p.wz - p.vz * 0.04f;
                if (!game::visengine.world_to_screen({tail_wx, tail_wy, tail_wz}, sp2, dims, view))
                    sp2 = { sp.x, sp.y + ss };

                ImU32 col = IM_COL32(
                    (int)(rc[0]*255.f), (int)(rc[1]*255.f), (int)(rc[2]*255.f),
                    (int)(alpha * 190.f));
                if (rain_elev)
                {

                    draw->AddLine(ImVec2(sp.x, sp.y), ImVec2(sp2.x, sp2.y),
                        IM_COL32((int)(rc[0]*255.f), (int)(rc[1]*255.f), (int)(rc[2]*255.f),
                            (int)(alpha * 70.f)), 2.6f);
                    draw->AddLine(ImVec2(sp.x - 1.0f, sp.y), ImVec2(sp2.x - 1.0f, sp2.y),
                        IM_COL32(0, 0, 0, (int)(alpha * 60.f)), 1.8f);
                    draw->AddLine(ImVec2(sp.x, sp.y), ImVec2(sp2.x, sp2.y), col, 1.0f);
                    draw->AddCircleFilled(ImVec2(sp.x, sp.y), 1.3f,
                        IM_COL32(255, 255, 255, (int)(alpha * 170.f)), 6);
                }
                else if (rain_v2)
                {

                    draw->AddLine(ImVec2(sp.x - 0.5f, sp.y), ImVec2(sp2.x - 0.5f, sp2.y),
                        IM_COL32(0, 0, 0, (int)(alpha * 50.f)), 1.5f);
                    draw->AddLine(ImVec2(sp.x, sp.y), ImVec2(sp2.x, sp2.y), col, 0.8f);
                }
                else
                {

                    draw->AddLine(ImVec2(sp.x, sp.y), ImVec2(sp2.x, sp2.y), col, 1.0f);
                }
            }
        }

        if (do_rain && !g_ripples.empty())
        {
            const float* rc = effects_3d::rain_color;
            for (const auto& r : g_ripples)
            {
                math::vector2 sp_c{};
                if (!game::visengine.world_to_screen({r.wx, r.wy, r.wz}, sp_c, dims, view))
                    continue;

                math::vector2 sp_edge{};
                if (!game::visengine.world_to_screen({r.wx + r.radius, r.wy, r.wz}, sp_edge, dims, view))
                    continue;

                float sr = fabsf(sp_edge.x - sp_c.x);
                if (sr < 0.5f) continue;

                float fade = 1.f - (r.age / r.life);
                ImU32 col = IM_COL32(
                    (int)(rc[0]*255.f), (int)(rc[1]*255.f), (int)(rc[2]*255.f),
                    (int)(fade * 130.f));
                if (rain_v2)
                {

                    draw->AddEllipse(ImVec2(sp_c.x, sp_c.y), ImVec2(sr, std::max(1.f, sr * 0.4f)),
                        col, 0.f, 16, 1.f);
                }
                else
                {

                    draw->AddCircle(ImVec2(sp_c.x, sp_c.y), sr, col, 16, 1.f);
                }
            }
        }

        // Every few seconds while V3 or V4 is on, so it can be checked without
        // guessing: a radius that never leaves the cap means the geometry snapshot
        // is empty, and zero culls with V4 on means nothing is being occluded.
        if (snow_elev || rain_elev)
        {
            static auto s_weather_log = std::chrono::steady_clock::now() - std::chrono::seconds(10);
            const auto  now_weather = std::chrono::steady_clock::now();
            if (now_weather - s_weather_log >= std::chrono::seconds(5))
            {
                s_weather_log = now_weather;
                debug::log("weather",
                    "snow=%d rain=%d radius=%.1f boxes=%d drawn=%d culled=%d",
                    effects_3d::snow_mode, effects_3d::rain_mode,
                    SPAWN_RADIUS, worldgeo::box_count(),
                    s_weather_drawn, s_weather_culled);
            }
        }
    }

    if (shaders::contrast)
    {
        float b = shaders::brightness_value;
        float c = shaders::contrast_value - 1.f;
        if (b < 0.f)
        {
            ImU32 dc = IM_COL32(0, 0, 0, (int)(std::min(-b, 1.f) * 200.f));
            draw->AddRectFilled(ImVec2(0, 0), ImVec2(screen_w, screen_h), dc);
        }
        else if (b > 0.f)
        {
            ImU32 bc = IM_COL32(255, 255, 255, (int)(std::min(b, 1.f) * 120.f));
            draw->AddRectFilled(ImVec2(0, 0), ImVec2(screen_w, screen_h), bc);
        }
        if (c > 0.f)
        {
            ImU32 cc = IM_COL32(0, 0, 0, (int)(std::min(c, 1.f) * 60.f));
            draw->AddRectFilled(ImVec2(0, 0), ImVec2(screen_w, screen_h), cc);
        }
    }

    if (shaders::vignette)
    {
        float str = shaders::vignette_strength;
        float rad = shaders::vignette_radius;
        float vig_a = std::min(str, 1.f);
        float edge_w = screen_w * (1.f - rad) * 0.5f + 20.f;
        float edge_h = screen_h * (1.f - rad) * 0.5f + 20.f;
        ImU32 dark   = IM_COL32(0, 0, 0, (int)(vig_a * 180));
        ImU32 clear  = IM_COL32(0, 0, 0, 0);
        draw->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(edge_w, screen_h),
            dark, clear, clear, dark);
        draw->AddRectFilledMultiColor(ImVec2(screen_w - edge_w, 0), ImVec2(screen_w, screen_h),
            clear, dark, dark, clear);
        draw->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(screen_w, edge_h),
            dark, dark, clear, clear);
        draw->AddRectFilledMultiColor(ImVec2(0, screen_h - edge_h), ImVec2(screen_w, screen_h),
            clear, clear, dark, dark);
    }

    if (shaders::chromatic_aberration)
    {
        float s2 = shaders::chromatic_strength;
        float off = s2 * 1.5f;
        draw->AddRectFilled(ImVec2(-off, 0), ImVec2(screen_w - off, screen_h),
            IM_COL32(255, 0, 0, 12));
        draw->AddRectFilled(ImVec2(off, 0), ImVec2(screen_w + off, screen_h),
            IM_COL32(0, 0, 255, 12));
    }

    if (shaders::film_grain)
    {
        static unsigned s_grain_rng = 0x13371337;
        float str2 = shaders::film_grain_strength;
        int   grain_alpha = (int)(str2 * 80.f);
        if (grain_alpha > 0)
        {
            int  grain_n = (int)(screen_w * screen_h * str2 * 0.002f);
            grain_n = std::min(grain_n, 4000);
            for (int i = 0; i < grain_n; i++)
            {
                s_grain_rng ^= s_grain_rng << 13;
                s_grain_rng ^= s_grain_rng >> 17;
                s_grain_rng ^= s_grain_rng << 5;
                float gx = (float)(s_grain_rng & 0x3fff) / 16383.f * screen_w;
                float gy = (float)((s_grain_rng >> 14) & 0x3fff) / 16383.f * screen_h;
                int   gv = (s_grain_rng >> 28) & 0xff;
                draw->AddRectFilled(ImVec2(gx, gy), ImVec2(gx + 1.f, gy + 1.f),
                    IM_COL32(gv, gv, gv, grain_alpha));
            }
        }
    }

    if (shaders::glossy && shaders::glossy_shimmer)
    {
        static float s_shimmer_t = 0.f;
        s_shimmer_t += dt * 0.8f;
        float shimmer = (sinf(s_shimmer_t * PI * 2.f) * 0.5f + 0.5f) * 0.06f;
        draw->AddRectFilled(ImVec2(0, 0), ImVec2(screen_w, screen_h),
            IM_COL32(255, 255, 255, (int)(shimmer * 255.f)));
    }
}
