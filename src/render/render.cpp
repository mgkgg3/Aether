#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define NOMINMAX
#include <render/render.h>

#include <vector>

#include <dwmapi.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <atomic>
#include <thread>
#include <chrono>
#include <tlhelp32.h>
#include <mutex>

#include <settings.h>
#include <game/game.h>
#include <game/game_context.h>
#include <memory/memory.h>
#include <debug/DebugConsole.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <features/esp/esp.h>
#include <features/silentaim/silentaim.h>
#include <features/aimbot/aimbot.h>
#include <features/world/world_effects.h>
#include <features/mesh/MeshChams.h>
#include <features/mesh/MeshDxShader.h>
#include <features/esp/Avatar.h>
#include <features/esp/BoxFill.h>
#include <features/radar/Radar.h>
#include <features/effects/Effects.h>
#include <features/fusion/SourcePack.h>
#include <features/esp/trails/Trails.h>
#include <features/worldgeo/WorldGeo.h>
#include <cache/cache.h>
#include <features/misc/Sonar.h>
#include <render/Backdrop.h>
#include <render/ShapeDraw.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>

#include <gui/menu/SyncBridge.h>
#include <gui/menu/SyncMenu.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void force_foreground(HWND hwnd)
{
    DWORD current_tid = GetCurrentThreadId();

    HWND fg = GetForegroundWindow();
    DWORD fg_tid = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;

    if (fg_tid && fg_tid != current_tid)
        AttachThreadInput(current_tid, fg_tid, TRUE);

    DWORD game_tid = game::wnd ? GetWindowThreadProcessId(game::wnd, nullptr) : 0;
    bool attached_game = false;
    if (game_tid && game_tid != current_tid && game_tid != fg_tid)
    {
        AttachThreadInput(current_tid, game_tid, TRUE);
        attached_game = true;
    }

    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    BringWindowToTop(hwnd);

    if (fg_tid && fg_tid != current_tid)
        AttachThreadInput(current_tid, fg_tid, FALSE);

    if (attached_game)
        AttachThreadInput(current_tid, game_tid, FALSE);
}

struct wnd_scan { DWORD pid; HWND best; long area; };

static BOOL CALLBACK aether_enum_roblox_window(HWND hwnd, LPARAM lparam)
{
    wnd_scan* s = (wnd_scan*)lparam;
    if (!s)
        return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != s->pid)
        return TRUE;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked)
        return TRUE;

    RECT rc{};
    if (!GetWindowRect(hwnd, &rc))
        return TRUE;

    long w = rc.right - rc.left, h = rc.bottom - rc.top;

    if (w < 200 || h < 150)
        return TRUE;

    long area = w * h;
    if (area > s->area)
    {
        s->area = area;
        s->best = hwnd;
    }
    return TRUE;
}

static void aether_refresh_game_window()
{
    DWORD pid = memory->get_process_id();
    if (!pid)
        return;

    wnd_scan s{ pid, nullptr, 0 };
    EnumWindows(aether_enum_roblox_window, (LPARAM)&s);

    if (s.best && s.best != game::wnd)
    {
        debug::log("render", "roblox window resolved -> 0x%p", (void*)s.best);
        game::wnd = s.best;
    }
    else if (!s.best)
    {
        game::wnd = nullptr;
    }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_F4) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

render_t::render_t()
{
    detail = std::make_unique<detail_t>();
}

render_t::~render_t()
{
    destroy_window();
    destroy_device();
}

namespace
{

    bool layered_optout()
    {
        static const bool off = []
        {
            const char* e = ::getenv("SYNC_LAYERED");
            return e && e[0] == '0';
        }();
        return off;
    }

    DWORD overlay_ex_style(bool click_through)
    {
        DWORD st = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
        if (click_through) st |= WS_EX_TRANSPARENT;
        if (!layered_optout()) st |= WS_EX_LAYERED;
        return st;
    }

    void apply_overlay_style(HWND hwnd, bool click_through)
    {
        SetWindowLongA(hwnd, GWL_EXSTYLE, overlay_ex_style(click_through));

        if (!layered_optout())
            SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    }
}

bool render_t::create_window()
{
    detail->window_class.cbSize = sizeof(detail->window_class);
    detail->window_class.style = CS_CLASSDC;
    detail->window_class.lpszClassName = "T4";
    detail->window_class.hInstance = GetModuleHandleA(0);
    detail->window_class.lpfnWndProc = wnd_proc;

    RegisterClassExA(&detail->window_class);

    detail->window = CreateWindowExA(
        overlay_ex_style(false),
        detail->window_class.lpszClassName,
        "T4",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        0,
        0,
        detail->window_class.hInstance,
        0
    );

    if (!detail->window)
    {
        return false;
    }

    MARGINS margins{ -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(detail->window, &margins);

    {
        const DWORD donotround = DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(detail->window, DWMWA_WINDOW_CORNER_PREFERENCE,
            &donotround, sizeof(donotround));

        const DWORD no_border = DWMWA_COLOR_NONE;
        DwmSetWindowAttribute(detail->window, DWMWA_BORDER_COLOR,
            &no_border, sizeof(no_border));
    }

    ShowWindow(detail->window, SW_SHOW);
    UpdateWindow(detail->window);

    {
        const LONG style = GetWindowLongA(detail->window, GWL_EXSTYLE);
        debug::log("render",
            "overlay style 0x%08lX | layered=%d click-through=%d | accent/extended-frame=%d",
            (unsigned long)style,
            (style & WS_EX_LAYERED) ? 1 : 0,
            (style & WS_EX_TRANSPARENT) ? 1 : 0,
            1);
    }

    return true;
}

bool render_t::create_device()
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};

    swap_chain_desc.BufferCount = 1;

    swap_chain_desc.BufferDesc.Width  = 0;
    swap_chain_desc.BufferDesc.Height = 0;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator   = 0;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;

    swap_chain_desc.OutputWindow = detail->window;

    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Windowed   = 1;

    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    swap_chain_desc.SampleDesc.Count   = 1;
    swap_chain_desc.SampleDesc.Quality = 0;

    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL feature_level;
    D3D_FEATURE_LEVEL feature_level_list[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    const UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        create_flags,
        feature_level_list,
        2,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &detail->swap_chain,
        &detail->device,
        &feature_level,
        &detail->device_context
    );

    if (result == DXGI_ERROR_UNSUPPORTED)
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            create_flags,
            feature_level_list,
            2,
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &detail->swap_chain,
            &detail->device,
            &feature_level,
            &detail->device_context
        );
    }

    if (result != S_OK)
    {
        MessageBoxA(nullptr, "This software can not run on your computer.", "Critical Problem", MB_ICONERROR | MB_OK);
        return false;
    }

    ID3D11Texture2D* back_buffer{ nullptr };
    detail->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));

    if (back_buffer)
    {
        detail->device->CreateRenderTargetView(back_buffer, nullptr, &detail->render_target_view);
        back_buffer->Release();

		if (!mesh::dx::shader::Init(detail->device, detail->device_context))
		{
			char why[224];
			mesh::dx::shader::StatusText(why, sizeof why);
			debug::log("mesh", "chams shader pass unavailable, flat mesh only | %s", why);
		}
		else
			debug::ok("chams shader pass ready");
		{
			D3D11_VIEWPORT vp{};
			UINT vp_count = 1;
			detail->device_context->RSGetViewports(&vp_count, &vp);
			if (vp.Width > 0.f && vp.Height > 0.f)
				mesh::dx::shader::Resize((unsigned)vp.Width, (unsigned)vp.Height);
		}

		avatar::init(detail->device);

		boxfill::init(detail->device);

        gui->Initialize(detail->window, detail->device, detail->device_context);

        return true;
    }

    return false;
}

void render_t::destroy_device()
{
    gui->Shutdown();
	mesh::dx::shader::Shutdown();

		boxfill::shutdown();

    if (detail->render_target_view) detail->render_target_view->Release();
    if (detail->swap_chain) detail->swap_chain->Release();
    if (detail->device_context) detail->device_context->Release();
    if (detail->device) detail->device->Release();
}

void render_t::destroy_window()
{
    DestroyWindow(detail->window);
    UnregisterClassA(detail->window_class.lpszClassName, detail->window_class.hInstance);
}

namespace
{
    constexpr DWORD WDA_NONE_               = 0x00000000;
    constexpr DWORD WDA_MONITOR_            = 0x00000001;
    constexpr DWORD WDA_EXCLUDEFROMCAPTURE_ = 0x00000011;

    using set_wda_t = BOOL(WINAPI*)(HWND, DWORD);
    using get_wda_t = BOOL(WINAPI*)(HWND, DWORD*);

    set_wda_t wda_set()
    {
        static set_wda_t fn = (set_wda_t)GetProcAddress(GetModuleHandleA("user32.dll"), "SetWindowDisplayAffinity");
        return fn;
    }

    get_wda_t wda_get()
    {
        static get_wda_t fn = (get_wda_t)GetProcAddress(GetModuleHandleA("user32.dll"), "GetWindowDisplayAffinity");
        return fn;
    }

    DWORD wda_query(HWND hwnd)
    {
        DWORD affinity = WDA_NONE_;
        get_wda_t get = wda_get();
        if (get && get(hwnd, &affinity))
            return affinity;
        return WDA_NONE_;
    }

    // Keep the overlay out of capture APIs.
    //
    // Two different things ask for this, and they share one state on purpose -
    // flipping a window's affinity twice a frame is itself a flicker source:
    //
    //  * Hide From Captures, the user's own answer for streams and screenshots;
    //  * the glass, which is *made of* a capture of the desktop.  Without the
    //    exclusion the duplication hands back the frame we just drew, the
    //    panel samples that, the fill darkens it, and the loop settles on
    //    black with a halo of the panel's own border - which is exactly what
    //    "the blur is just black with a glow" looked like.
    void sync_capture_affinity(HWND hwnd, bool want)
    {
        static HWND  s_hwnd     = (HWND)-1;
        static bool  s_enabled  = false;
        static DWORD s_last_run = 0;

        if (!hwnd)
            return;

        const DWORD now       = GetTickCount();
        const bool  changed   = (hwnd != s_hwnd) || (want != s_enabled);
        const bool  heartbeat = (now - s_last_run) > 2000;
        if (!changed && !heartbeat)
            return;

        s_last_run = now;

        const DWORD desired = want ? WDA_EXCLUDEFROMCAPTURE_ : WDA_NONE_;

        if (!changed && wda_query(hwnd) == desired)
            return;

        set_wda_t set = wda_set();
        if (!set)
        {
            if (want)
                debug::warn("streamproof unavailable: SetWindowDisplayAffinity not exported by user32.dll");
            s_hwnd = hwnd;
            s_enabled = want;
            return;
        }

        BOOL ok = set(hwnd, desired);

        // The monitor affinity blanks the window *to black* in captures, so it
        // is only a fallback for the user's own Hide From Captures - as the
        // glass's exclusion it would poison the very image it is meant to
        // clean, and the glass is skipped instead when it cannot be applied.
        if (!ok && want && settings::streamproof::enabled)
        {
            ok = set(hwnd, WDA_MONITOR_);
            if (ok)
                debug::log("render", "capture affinity: WDA_EXCLUDEFROMCAPTURE rejected, "
                    "fell back to WDA_MONITOR (pre-2004 Windows)");
        }

        if (!ok && want)
            debug::warn("streamproof could not be applied (error %lu); overlay may still appear in captures", GetLastError());

        const DWORD applied = wda_query(hwnd);
        const char* state = (applied == WDA_EXCLUDEFROMCAPTURE_) ? "excluded from capture"
                          : (applied == WDA_MONITOR_)            ? "monitor-only (blank in captures)"
                          : (want ? "NOT APPLIED" : "off");
        debug::log("render", "capture affinity %s | affinity=0x%02lX", state, applied);

        s_hwnd = hwnd;
        s_enabled = want;
    }

    // The glass behind the panel: one capture of the desktop, down-scaled hard
    // and blurred small, published as a texture the menu samples.  Keeping our
    // own overlay out of that capture is part of starting it (see
    // sync_capture_affinity above); it is only set on transitions, because
    // flipping a window's display affinity at frame rate is itself a flicker
    // source.
    void update_backdrop(HWND overlay_window, ID3D11Device* device, ID3D11DeviceContext* context)
    {
        sync::Backdrop& b = sync::backdrop();
        b = sync::Backdrop{};

        if (!overlay_window || !device || !context)
            return;

        // The rectangles the image is ever sampled by: the panel while it is
        // up, the watermark card and the open plates.  Worked out *before* the
        // capture is considered at all, because the one thing that must not
        // happen is a capture with nothing to frost - an empty request goes
        // back to the full desktop inside backdrop::set_region, and a full
        // desktop captured per frame behind a closed menu is the stutter the
        // blur was blamed for (the log's own line: "1720x1080 block of a
        // 1720x1080 desktop").  No rectangle, no capture.
        //
        // `backdrop::wanted()` carries the other half of that answer now: the
        // Capture Cap slider.  At 0 it is false whatever is on screen, so the
        // duplication is never started - that is "capture 0", and it costs the
        // desktop nothing at all rather than a smaller copy of something.
        const bool opaque = settings::appearance::no_transparency;

        // The glass is a capture of this desktop, so it must not be in its own
        // source.  With the overlay captureable, the duplication hands back the
        // panel we just drew, the panel samples that, and the loop settles on a
        // blur of itself - which reads as "the blur is black with a halo".
        //
        // This is the merged tree's arrangement, and it is the one that worked:
        // the exclusion is held for as long as the effect runs and released when
        // it stops - two transitions per session.  Two other things were tried
        // here and both are wrong.  Holding the last frame captured while the
        // panel was shut cannot start at all if the blur is switched on from the
        // panel, which is where the switch is.  Borrowing the exclusion for each
        // copy makes the duplication alternate between frames with and without
        // the overlay, which is the shimmer.  Constant exclusion, live capture.
        //
        // The price is explicit and it is the user's switch: with the blur on,
        // the panel is out of screenshots too.  With it off, the overlay is
        // captureable exactly as before.
        const bool panel_up = sync::panel_visible();

        float gx0 = 0.f, gy0 = 0.f, gx1 = 0.f, gy1 = 0.f;
        const bool have_region = sync::hud_backdrop_rect(
            settings::backdrop::capture_pad, gx0, gy0, gx1, gy1, true);

        // The merged arrangement, and the only one that has ever held together:
        // the exclusion is on for as long as the glass is running, the capture
        // runs live the whole time, and nothing is held or seeded.  Every
        // alternative tried in this round - holding a frame, borrowing the
        // exclusion per copy or per section, blanking the plate - traded one of
        // the three things a frost has to be at once: present, adjustable, and
        // steady.  This is all three.
        //
        // The price is unavoidable and is stated in the settings row: a capture
        // of the desktop that includes this window is a blur of this window, so
        // while the blur is on the panel is out of screenshots.  Turn the blur
        // off and the overlay is captureable again.
        const bool glass_on = settings::backdrop::enabled && !opaque;
        sync_capture_affinity(overlay_window, settings::streamproof::enabled || glass_on);

        const bool clean = (wda_query(overlay_window) == WDA_EXCLUDEFROMCAPTURE_);

        // An opaque panel has nothing to frost either: the capture is skipped
        // outright rather than blurred to nothing, so the option costs less
        // than the blur it replaces.
        //
        // And the gate is the *rectangle*, not the menu.  The card and the
        // plates sample the same image and they are on screen with the panel
        // shut, so gating on the panel's own fade left them drawing a frost
        // that was never captured - the watermark and the docks had no blur of
        // their own.  The rectangle is also the cheaper answer of the two: it
        // vanishes when nothing that samples it is on screen, so no panel, no
        // card and no open plate still means no capture at all.
        // Asked of the *settings*, not of `backdrop::wanted()`.  That helper
        // answers from the params of the last frame, and the params are only
        // pushed once `running` is true - so with Blur and Tint both at 0 the
        // capture would switch itself off and then never see the sliders move
        // again.  One frame of stale state was enough to wedge it permanently.
        const bool wants  = settings::backdrop::blur > 2.f || settings::backdrop::tint > 0.5f;
        const bool running = settings::backdrop::enabled && !opaque &&
                             have_region && wants;


        static bool s_started  = false;
        static bool s_warned   = false;
        static bool s_idle_logged = false;

        if (!running)
        {
            if (s_started)
            {
                backdrop::stop();
                s_started = false;
                debug::log("render", "backdrop: stopped (nothing on screen samples it)");
            }

            // The one state worth a line: blur on, and the capture held off
            // because there is nothing to frost.  That is the "capture 0"
            // state - no duplication running, so nothing behind the overlay
            // pays for an image nobody samples.
            if (!s_idle_logged && settings::backdrop::enabled && !opaque && clean &&
                backdrop::wanted() && !have_region)
            {
                s_idle_logged = true;
                debug::log("render", "backdrop: capture held off - the panel and the plates are "
                    "the only things that sample it and none of them is on screen");
            }
            else if (have_region && clean)
            {
                s_idle_logged = false;
            }
            s_warned = false;
            return;
        }

        if (!s_started)
        {
            if (!backdrop::start(device, context, overlay_window))
            {
                if (!s_warned)
                {
                    s_warned = true;
                    debug::warn("backdrop: this display cannot be captured (another capture is running?)");
                }
                return;
            }
            s_started = true;
            debug::log("render", "backdrop: started (%.0f blur / %.0f tint)",
                settings::backdrop::blur, settings::backdrop::tint);
        }

        backdrop::params_t params;
        // An opaque panel has no frost at all, and Tint Only asks for the
        // darkening without it: both are the blur at 0 with the tint left on,
        // which is the cheap path - no blur pass runs.
        params.blur        = (opaque || settings::backdrop::tint_only) ? 0.f : settings::backdrop::blur;
        params.tint        = settings::backdrop::tint;
        params.capture_cap = settings::backdrop::capture_cap;
        backdrop::set_params(params);

        // Only the panel, the card and the plates are ever sampled, so only they
        // are ever captured.  This is the single biggest saving in the effect:
        // the image stops being the desktop and becomes the rectangle that needs
        // it, which is what keeps the game behind the panel at its own frame
        // rate while the blur is on.  Nothing outside the cap is copied, mapped
        // or blurred - and asking again with the same rectangle is free, so a
        // still panel keeps its region without rebuilding anything.
        //
        // While the panel is shut the region still holds its rectangle (that is
        // what `!clean` asks hud_backdrop_rect for): the pre-roll image the
        // glass will hold once the panel opens.
        static unsigned long long s_next_capture = 0;
        const unsigned long long now = ::GetTickCount64();

        backdrop::set_region(gx0, gy0, gx1, gy1);

        if (now >= s_next_capture)
        {
            backdrop::update();

            const float cost_ms = backdrop::cost_ms();
            unsigned long long delay = cost_ms > 8.f
                ? (unsigned long long)(cost_ms * 2.f)
                : 0ull;

            // The pre-roll only feeds the first frame the panel opens on, so it
            // does not need the panel's frame rate: eight a second keeps the
            // held image current enough and costs a closed menu almost nothing.
            if (!clean && !panel_up && delay < 125ull)
                delay = 125ull;

            s_next_capture = now + delay;

            if (settings::backdrop::status_line)
            {
                // The three numbers behind "the blur lags": what the capture cost,
                // how old the image it published is, and which read path it took.
                // Guessing at any of them is how the pacing below got set to a
                // fifth of a frame *and* five times the cost in the first place.
                const float age = backdrop::image_age_ms();
                debug::log("render",
                    "backdrop %.2f ms | image %.0f ms old | %s %.2f ms | %s | %s",
                    cost_ms, age,
                    backdrop::split_readback() ? "split read" : "blocking read",
                    backdrop::readback_ms(),
                    delay ? "thinned" : "every frame",
                    backdrop::status());
            }
        }

        b.image            = (ImTextureID)(intptr_t)backdrop::image();
        b.tex_w            = backdrop::image_w();
        b.tex_h            = backdrop::image_h();
        b.desktop_x        = backdrop::origin_x();
        b.desktop_y        = backdrop::origin_y();
        b.image_scale      = backdrop::image_scale();
        b.capture_has_self = !clean;
    }
}

namespace
{
    using frame_clock = std::chrono::steady_clock;

    frame_clock::time_point g_frame_begin{};

    struct frame_totals_t
    {
        unsigned long long frames     = 0;
        double             build_ms   = 0.0;
        double             present_ms = 0.0;
        double             pace_ms    = 0.0;
        double             worst_ms   = 0.0;
        frame_clock::time_point window_start{};
    };
    frame_totals_t g_frame_totals;

    double g_window_cpu_at_start = 0.0;
    double g_window_thread_cpu_at_start = 0.0;

    constexpr int k_max_phases = 32;
    struct phase_total_t { const char* name; double ms; };
    phase_total_t g_phases[k_max_phases]{};
    int g_phase_count = 0;

    // The heaviest single phase sample of the current window, so a bad frame
    // can be attributed to a pass instead of only to a number.
    struct phase_worst_t { const char* name; double ms; };
    phase_worst_t g_worst_phase{};

    double filetime_ms(const FILETIME& f)
    {
        const unsigned long long ticks = (((unsigned long long)f.dwHighDateTime) << 32) | f.dwLowDateTime;
        return (double)ticks / 10000.0;
    }

    double process_cpu_ms()
    {
        FILETIME creation{}, exit{}, kernel{}, user{};
        if (!::GetProcessTimes(::GetCurrentProcess(), &creation, &exit, &kernel, &user))
            return 0.0;
        return filetime_ms(kernel) + filetime_ms(user);
    }

    double thread_cpu_ms()
    {
        FILETIME creation{}, exit{}, kernel{}, user{};
        if (!::GetThreadTimes(::GetCurrentThread(), &creation, &exit, &kernel, &user))
            return 0.0;
        return filetime_ms(kernel) + filetime_ms(user);
    }

    struct thread_sample_t { unsigned long id; double cpu_ms; };

    // The thread list was the expensive half of this report, and it ran on the
    // render thread: a TH32CS_SNAPTHREAD snapshot walks the thread table of the
    // *whole machine* - Roblox, the browser, everything - and then every one of
    // our own threads was opened again.  In a session with a few thousand
    // threads alive that is tens of milliseconds, once per accounting window,
    // which is the periodic hitch the panel showed (one frozen frame every few
    // seconds, worst in the panel's own 5 s report).
    //
    // So the list is built once and kept: the ids and the handles are held open
    // and sampling becomes one GetThreadTimes per thread, a few microseconds.
    // The snapshot only runs again when the list ages out (or when a sample
    // finds a handle that no longer answers), which is cheap enough to pay once
    // a minute.  A thread created between rebuilds is simply missing from the
    // percentage list; it still shows up in the next rebuild.
    constexpr int    k_max_threads        = 128;
    constexpr double k_thread_list_seconds = 30.0;

    struct thread_list_t
    {
        std::vector<HANDLE>        handles;
        std::vector<unsigned long> ids;
        double                     built_at = -1.0;
    };

    thread_list_t& thread_list()
    {
        static thread_list_t list;
        return list;
    }

    void close_thread_list()
    {
        thread_list_t& list = thread_list();
        for (HANDLE th : list.handles)
            if (th)
                ::CloseHandle(th);
        list.handles.clear();
        list.ids.clear();
        list.built_at = -1.0;
    }

    double seconds_now()
    {
        static const auto s_origin = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - s_origin).count();
    }

    void build_thread_list()
    {
        close_thread_list();
        thread_list_t& list = thread_list();
        list.built_at = seconds_now();

        const HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE)
            return;

        const unsigned long self = ::GetCurrentProcessId();

        THREADENTRY32 te{}; te.dwSize = sizeof te;
        if (::Thread32First(snap, &te))
        {
            do
            {
                if (te.th32OwnerProcessID != self) continue;
                if ((int)list.handles.size() >= k_max_threads) break;

                const HANDLE th = ::OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te.th32ThreadID);
                if (!th) continue;

                list.handles.push_back(th);
                list.ids.push_back(te.th32ThreadID);
            } while (::Thread32Next(snap, &te));
        }

        ::CloseHandle(snap);
    }

    // Returns the number of threads written.  Sampling never rebuilds the list
    // inside a window: it answers from the cached handles, and the rebuild -
    // the part that costs real time - happens at most once every thirty
    // seconds, between reports rather than in the middle of one.
    int sample_threads(thread_sample_t* out, int max)
    {
        if (thread_list().built_at < 0.0)
            build_thread_list();

        const thread_list_t& list = thread_list();
        int count = 0;
        bool lost = false;

        for (std::size_t i = 0; i < list.handles.size() && count < max; ++i)
        {
            FILETIME creation{}, exit{}, kernel{}, user{};
            if (!::GetThreadTimes(list.handles[i], &creation, &exit, &kernel, &user))
            {
                lost = true;
                continue;
            }

            out[count].id     = list.ids[i];
            out[count].cpu_ms = filetime_ms(kernel) + filetime_ms(user);
            ++count;
        }

        if (lost || seconds_now() - list.built_at > k_thread_list_seconds)
            build_thread_list();

        return count;
    }

    // The report is sampled on its own thread now, not in the frame.
    //
    // Caching the handles made a sample cheap, but the snapshot behind them is
    // still the machine's whole thread table - measured at 51 ms on this desktop
    // (6,535 threads alive, mostly the browser) against 0.03 ms for a hundred
    // GetThreadTimes.  As long as that ran on the render thread it had to land
    // in a frame, and a rebuild every thirty seconds would only have traded one
    // periodic freeze for a rarer one.  So the worker owns the snapshot and the
    // frame owns four numbers: `report_threads` copies them out under a lock
    // that is never held across a syscall.
    constexpr int k_thread_report_top = 4;

    struct thread_report_t
    {
        bool          fresh = false;
        int           count = 0;
        int           top   = 0;
        unsigned long ids[k_thread_report_top]{ };
        double        percent[k_thread_report_top]{ };
    };

    std::mutex      s_thread_report_mtx;
    thread_report_t s_thread_report;

    void thread_report_worker()
    {
        thread_sample_t prev[k_max_threads]{};
        int prev_count = 0;
        auto last = std::chrono::steady_clock::now();

        for (;;)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            thread_sample_t now[k_max_threads]{};
            const int now_count = sample_threads(now, k_max_threads);

            const auto   at      = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double, std::milli>(at - last).count();
            last = at;

            if (now_count <= 0)
                continue;

            struct line_t { unsigned long id; double ms; };
            line_t top[k_thread_report_top]{};
            int top_count = 0;

            for (int i = 0; i < now_count; ++i)
            {
                double before = 0.0;
                for (int j = 0; j < prev_count; ++j)
                {
                    if (prev[j].id == now[i].id) { before = prev[j].cpu_ms; break; }
                }
                const double delta = now[i].cpu_ms - before;
                if (delta <= 0.0) continue;

                if (top_count < k_thread_report_top) { top[top_count++] = { now[i].id, delta }; }
                else
                {
                    int least = 0;
                    for (int k = 1; k < k_thread_report_top; ++k)
                        if (top[k].ms < top[least].ms) least = k;
                    if (delta > top[least].ms) top[least] = { now[i].id, delta };
                }
            }

            for (int i = 0; i < now_count && i < k_max_threads; ++i)
                prev[i] = now[i];
            prev_count = now_count;

            thread_report_t report;
            report.fresh = true;
            report.count = now_count;
            report.top   = top_count;
            for (int i = 0; i < top_count; ++i)
            {
                report.ids[i]     = top[i].id;
                report.percent[i] = elapsed > 0.0 ? top[i].ms * 100.0 / elapsed : 0.0;
            }

            std::lock_guard<std::mutex> lk(s_thread_report_mtx);
            s_thread_report = report;
        }
    }

    void report_threads(double elapsed_ms)
    {
        (void)elapsed_ms;

        static bool s_started = false;
        if (!s_started)
        {
            s_started = true;
            try { std::thread(thread_report_worker).detach(); }
            catch (...) { s_started = false; }
        }

        thread_report_t report;
        {
            std::lock_guard<std::mutex> lk(s_thread_report_mtx);
            if (!s_thread_report.fresh)
                return;
            report = s_thread_report;
            s_thread_report.fresh = false;
        }

        char buf[320]{};
        int used = 0;
        for (int i = 0; i < report.top; ++i)
            used += snprintf(buf + used, sizeof buf - (std::size_t)used, " tid %lu=%.0f%%",
                report.ids[i], report.percent[i]);

        debug::log("render", "threads %d | busiest:%s",
            report.count, used > 0 ? buf : " (none)");
    }

    void phase_add(const char* name, double ms)
    {
        if (!name || ms < 0.0)
            return;

        if (g_worst_phase.name == nullptr || ms > g_worst_phase.ms)
        {
            g_worst_phase.name = name;
            g_worst_phase.ms   = ms;
        }

        for (int i = 0; i < g_phase_count; ++i)
        {
            if (std::strcmp(g_phases[i].name, name) == 0)
            {
                g_phases[i].ms += ms;
                return;
            }
        }

        if (g_phase_count < k_max_phases)
        {
            g_phases[g_phase_count].name = name;
            g_phases[g_phase_count].ms   = ms;
            ++g_phase_count;
        }
    }

    double ms_between(frame_clock::time_point a, frame_clock::time_point b)
    {
        return std::chrono::duration<double, std::milli>(b - a).count();
    }

    void frame_account(frame_clock::time_point render_done,
                       frame_clock::time_point present_done,
                       frame_clock::time_point end)
    {
        if (g_frame_totals.window_start == frame_clock::time_point{})
        {
            g_frame_totals.window_start = g_frame_begin;
            g_window_cpu_at_start        = process_cpu_ms();
            g_window_thread_cpu_at_start = thread_cpu_ms();
        }

        const double whole = ms_between(g_frame_begin, end);

        g_frame_totals.frames++;
        g_frame_totals.build_ms   += ms_between(g_frame_begin, render_done);
        g_frame_totals.present_ms += ms_between(render_done, present_done);
        g_frame_totals.pace_ms    += ms_between(present_done, end);
        if (whole > g_frame_totals.worst_ms)
            g_frame_totals.worst_ms = whole;

        const double elapsed = ms_between(g_frame_totals.window_start, end);
        if (elapsed < 5000.0)
            return;

        const double n = (double)g_frame_totals.frames;

        char phases[640]{};
        char menu[640]{};
        int  used = 0, menu_used = 0;
        for (int i = 0; i < g_phase_count; ++i)
        {
            const bool is_menu = std::strncmp(g_phases[i].name, "sync.", 5) == 0;
            char* buf = is_menu ? menu : phases;
            int&  at  = is_menu ? menu_used : used;
            const int room = (int)sizeof phases - at;
            at += snprintf(buf + at, (std::size_t)(room > 0 ? room : 0),
                " %s=%.2fms", g_phases[i].name + (is_menu ? 5 : 0), g_phases[i].ms / n);
        }

        debug::log("render",
            "fps=%.1f | frame build=%.2fms present=%.2fms pace=%.2fms | worst=%.2fms (%s %.2fms) | layered=%d |%s",
            n * 1000.0 / elapsed,
            g_frame_totals.build_ms / n,
            g_frame_totals.present_ms / n,
            g_frame_totals.pace_ms / n,
            g_frame_totals.worst_ms,
            g_worst_phase.name ? g_worst_phase.name : "-",
            g_worst_phase.ms,
            layered_optout() ? 0 : 1,
            used > 0 ? phases : " (no phases reported)");

        if (menu_used > 0)
            debug::log("render", "menu |%s", menu);

        debug::log("render", "cpu | render thread %.0f%% of the window | process %.0f%%",
            (thread_cpu_ms() - g_window_thread_cpu_at_start) * 100.0 / elapsed,
            (process_cpu_ms() - g_window_cpu_at_start) * 100.0 / elapsed);

        report_threads(elapsed);

        for (int i = 0; i < g_phase_count; ++i)
            g_phases[i].ms = 0.0;
        g_worst_phase = phase_worst_t{};

        g_frame_totals = frame_totals_t{};
        g_frame_totals.window_start = end;
        g_window_cpu_at_start        = process_cpu_ms();
        g_window_thread_cpu_at_start = thread_cpu_ms();
    }
}

void render_t::trace(const char* phase, double ms)
{
    phase_add(phase, ms);
}

void render_t::start_render()
{
    g_frame_begin = frame_clock::now();

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    static DWORD s_last_wnd_scan = GetTickCount() - 2000;
    DWORD now_ms = GetTickCount();
    if (now_ms - s_last_wnd_scan > 1000)
    {
        s_last_wnd_scan = now_ms;
        aether_refresh_game_window();
    }

    static bool s_focus_grabbed = false;
    if (running && !s_focus_grabbed)
    {
        s_focus_grabbed = true;
        force_foreground(detail->window);
    }

    // the gate card needs real clicks, so the overlay stops being click-through
    // while the cursor is over it
    const bool want_input = running || sync::gate_wants_mouse();
    static bool s_last_running = false;
    if (want_input != s_last_running)
    {

        if (want_input)
        {
            apply_overlay_style(detail->window, false);
            SetWindowPos(detail->window, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            force_foreground(detail->window);
        }
        else
        {
            apply_overlay_style(detail->window, true);
            SetWindowPos(detail->window, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            if (game::wnd) SetForegroundWindow(game::wnd);
        }
        s_last_running = want_input;
    }

    float clear_color[4]{ 0.f, 0.f, 0.f, 0.f };
    detail->device_context->OMSetRenderTargets(1, &detail->render_target_view, nullptr);
    detail->device_context->ClearRenderTargetView(detail->render_target_view, clear_color);

    update_backdrop(detail->window, detail->device, detail->device_context);
}

void render_t::end_render()
{
    const frame_clock::time_point render_done = frame_clock::now();

    detail->swap_chain->Present(settings::menu::vsync ? 1 : 0, 0);

    const frame_clock::time_point present_done = frame_clock::now();

    if (!settings::menu::vsync && settings::menu::fps_limit > 0)
    {
        static auto s_last = std::chrono::steady_clock::now();
        const auto budget = std::chrono::microseconds(1000000 / settings::menu::fps_limit);
        const auto now    = std::chrono::steady_clock::now();
        const auto spent  = now - s_last;
        if (spent < budget)
            std::this_thread::sleep_for(budget - spent);
        s_last = std::chrono::steady_clock::now();
    }

    if (settings::menu::performance_mode)
        std::this_thread::sleep_for(std::chrono::microseconds(500));

    frame_account(render_done, present_done, frame_clock::now());
}

void render_t::render_menu()
{
    POINTS window_size = { 640, 480 };
    gui->Render("Aether", window_size, &running);
}

// The FOV ring, drawn the way the enhanced health bar is: concentric bands,
// each on its own radius so no band eats the one it is meant to edge.  The
// colour band is the circle itself; the outline - when it is on - sits just
// *outside* it, and the fill stops just inside it.  The old ring centred a
// 2.4 px dark line on a 1.4 px colour line, so half the dark sat on top of the
// colour and the whole thing read as a thick smudged circle at any thickness.
// Style 2 of the FOV circle warms while a player it can see is inside it.  The
// ring itself only knows pixels, so the answer is measured here: every head is
// projected and the ones inside the radius are tested against the geometry
// snapshot the visibility checks already use.
static bool fov_has_visible_target(const ImVec2& centre, float radius)
{
    if (!game::visengine.address || radius <= 0.f) return false;

    const math::matrix4 view = game::visengine.get_viewmatrix();
    const math::vector2 dims = game::visengine.get_dimensions();
    const math::vector3 eye = worldgeo::eye_position({});
    const float r2 = radius * radius;

    std::lock_guard<std::mutex> lk(cache::mtx);
    for (const auto& e : cache::cached_players)
    {
        if (e.instance.address == 0) continue;
        if (e.instance.address == cache::cached_local_player.instance.address) continue;

        auto it = e.parts.find("Head");
        if (it == e.parts.end()) it = e.parts.find("HumanoidRootPart");
        if (it == e.parts.end() || !it->second.address) continue;

        rbx::primitive_t prim = it->second.get_primitive();
        if (!prim.address) continue;

        const math::vector3 world = memory->read<math::vector3>(
            prim.address + Offsets::Primitive::Position);

        math::vector2 sp{};
        if (!game::visengine.world_to_screen(world, sp, dims, view)) continue;
        const float dx = sp.x - centre.x, dy = sp.y - centre.y;
        if (dx * dx + dy * dy > r2) continue;
        if (worldgeo::is_visible(eye, world)) return true;
    }
    return false;
}

static void draw_fov_circle(float fov_radius, float circle_colour[4], float outline_colour[4],
    float thickness, bool outline, int style,
    bool attach_to_target = false,
    bool fill = false, float fill_colour[4] = nullptr, float fill_opacity = 0.f)
{
    if (!game::visengine.address) return;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    shape::apply_flags(draw);
    ImVec2 center{};

    if (attach_to_target && settings::silentaim::enabled &&
        silentaim::state.data_ready && silentaim::state.target.instance.address != 0)
    {
        const math::matrix4 view = game::visengine.get_viewmatrix();
        const math::vector2 dims = game::visengine.get_dimensions();
        math::vector2 sp;
        if (game::visengine.world_to_screen(silentaim::state.target_world_pos, sp, dims, view))
            center = ImVec2(sp.x, sp.y);
        else { POINT c{}; if (!GetCursorPos(&c)) return; center = ImVec2((float)c.x, (float)c.y); }
    }
    else
    {
        POINT c{}; if (!GetCursorPos(&c)) return;
        center = ImVec2((float)c.x, (float)c.y);
    }

    ImU32 outline_col = IM_COL32(
        (int)(outline_colour[0]*255.f), (int)(outline_colour[1]*255.f),
        (int)(outline_colour[2]*255.f), (int)(outline_colour[3]*255.f));
    ImU32 circle_col = IM_COL32(
        (int)(circle_colour[0]*255.f), (int)(circle_colour[1]*255.f),
        (int)(circle_colour[2]*255.f), (int)(circle_colour[3]*255.f));

    const float th = std::max(0.5f, std::min(thickness, 12.f));

    ImU32 fill_col = 0;
    float fill_a = 0.f;
    if (fill && fill_colour)
    {
        fill_a = fill_opacity * 2.55f;
        fill_col = IM_COL32(
            (int)(fill_colour[0]*255.f), (int)(fill_colour[1]*255.f),
            (int)(fill_colour[2]*255.f), (int)(fill_colour[3]*fill_a));
    }

    const bool hot = (style == 2) && fov_has_visible_target(center, fov_radius);

    shape::fov_circle(draw, center, fov_radius, circle_col, outline_col, th,
        outline, style, fill, fill_col, fill_a, hot);
}

// The game holds the mouse at the centre of its own viewport while it has the
// focus, and that is the point "the crosshair" means here. The overlay's
// DisplaySize is the whole monitor, so on a windowed or maximized client the two
// are not the same point and anything anchored to it starts a title bar high.
// Callers keep the overlay centre when the game is not readable.
static bool game_crosshair_point(ImVec2& out)
{
    if (!game::visengine.address) return false;

    rbx::visualengine_t::update_window_offset();
    const math::vector2 dims = game::visengine.get_dimensions();
    if (!(dims.x > 1.f && dims.y > 1.f)) return false;

    out = ImVec2(rbx::visualengine_t::window_offset_x() + dims.x * 0.5f,
                 rbx::visualengine_t::window_offset_y() + dims.y * 0.5f);
    return true;
}

// The frame every screen coordinate is built on: world_to_screen() answers in
// desktop pixels, so the game client's own origin has to sit on top of the
// viewport mapping (features/esp/esp.cpp refreshes it too). Logged when it
// moves - an origin that is stale or zero shifts the whole overlay, and every
// aim write, which converges on the same mapping, by a caption's height, and
// nothing else in the log would show it.
static void log_viewport_map(float origin_x, float origin_y, const math::vector2& dims)
{
    if (!game::wnd || !game::visengine.address)
        return;

    static float s_x = -1.f, s_y = -1.f, s_w = -1.f, s_h = -1.f;
    if (origin_x == s_x && origin_y == s_y && dims.x == s_w && dims.y == s_h)
        return;

    s_x = origin_x; s_y = origin_y; s_w = dims.x; s_h = dims.y;

    debug::log("geo",
        "projection origin=%.0f,%.0f viewport=%.0fx%.0f | screen centre=%.1f,%.1f",
        origin_x, origin_y, dims.x, dims.y,
        origin_x + dims.x * 0.5f, origin_y + dims.y * 0.5f);
}

// TEMP probe: one line every 5 s that says where the overlay believes the game
// is, so the viewport mapping can be checked against the client and the camera.
static void log_viewport_probe(HWND overlay)
{
    static auto s_last = std::chrono::steady_clock::now() - std::chrono::seconds(30);
    const auto now = std::chrono::steady_clock::now();
    if (now - s_last < std::chrono::seconds(5)) return;
    s_last = now;

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const math::vector2 dims = game::visengine.address
        ? game::visengine.get_dimensions() : math::vector2{};
    const float ox = rbx::visualengine_t::window_offset_x();
    const float oy = rbx::visualengine_t::window_offset_y();

    RECT gc{}, gw{}, oc{};
    POINT corigin{};
    if (game::wnd)
    {
        GetClientRect(game::wnd, &gc);
        GetWindowRect(game::wnd, &gw);
        ClientToScreen(game::wnd, &corigin);
    }
    if (overlay) GetClientRect(overlay, &oc);

    POINT cur{};
    GetCursorPos(&cur);

    rbx::vector2int16 cam_vp{};
    math::vector2     cam_vp_size{};
    const std::uint64_t cam = rbx::current_camera_address();
    if (cam)
    {
        try { cam_vp = memory->read<rbx::vector2int16>(cam + Offsets::Camera::Viewport); }
        catch (...) {}
        try { cam_vp_size = memory->read<math::vector2>(cam + Offsets::Camera::ViewportSize); }
        catch (...) {}
    }

    debug::log("probe",
        "mon=%dx%d | overlay=%ldx%ld | game win=%ld,%ld %ldx%ld | game client=%ldx%ld origin=%ld,%ld | win_off=%.0f,%.0f | visengine=%.0fx%.0f | map centre=%.1f,%.1f | display=%.0fx%.0f | cursor=%ld,%ld | cam vp=%d,%d size=%.0fx%.0f",
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        overlay ? (long)(oc.right - oc.left) : -1L, overlay ? (long)(oc.bottom - oc.top) : -1L,
        gw.left, gw.top, (long)(gw.right - gw.left), (long)(gw.bottom - gw.top),
        (long)(gc.right - gc.left), (long)(gc.bottom - gc.top), corigin.x, corigin.y,
        ox, oy, dims.x, dims.y,
        ox + dims.x * 0.5f, oy + dims.y * 0.5f,
        ds.x, ds.y, cur.x, cur.y,
        (int)cam_vp.x, (int)cam_vp.y, cam_vp_size.x, cam_vp_size.y);
}

static void draw_custom_cursor()
{
    if (!settings::silentaim::enabled || !silentaim::state.aim_indicator.address) return;
    bool vis = false;
    try { vis = memory->read<bool>(silentaim::state.aim_indicator.address + Offsets::GuiObject::Visible); } catch(...){ return; }
    if (!vis) return;

    POINT pt; if (!GetCursorPos(&pt)) return;
    bool rclick = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    float gap = rclick ? 4.f : 10.f;
    const float ds = 4.f, lw = 2.f, ll = 10.f;
    ImDrawList* d = ImGui::GetBackgroundDrawList();
    ImU32 col = IM_COL32(255,255,255,255);
    ImVec2 c = {(float)pt.x,(float)pt.y};
    d->AddRectFilled({c.x-ds*.5f,c.y-ds*.5f},{c.x+ds*.5f,c.y+ds*.5f},col,0.f);
    d->AddRectFilled({c.x-lw*.5f,c.y-gap-ll},{c.x+lw*.5f,c.y-gap},col,0.f);
    d->AddRectFilled({c.x-lw*.5f,c.y+gap},{c.x+lw*.5f,c.y+gap+ll},col,0.f);
    d->AddRectFilled({c.x-gap-ll,c.y-lw*.5f},{c.x-gap,c.y+lw*.5f},col,0.f);
    d->AddRectFilled({c.x+gap,c.y-lw*.5f},{c.x+gap+ll,c.y+lw*.5f},col,0.f);
}

static void draw_silent_aim_target_dot()
{
    if (!settings::silentaim::draw_target_dot || !settings::silentaim::enabled) return;
    if (!silentaim::state.data_ready || !silentaim::state.target.instance.address) return;
    if (!game::visengine.address) return;
    const math::matrix4 view = game::visengine.get_viewmatrix();
    const math::vector2 dims = game::visengine.get_dimensions();
    math::vector2 sp;
    if (!game::visengine.world_to_screen(silentaim::state.target_world_pos, sp, dims, view)) return;
    if (sp.x < 0.f || sp.y < 0.f || sp.x > dims.x || sp.y > dims.y) return;
    const auto& c = settings::silentaim::target_dot_color;
    ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(sp.x,sp.y), settings::silentaim::target_dot_size,
        IM_COL32((int)(c[0]*255.f),(int)(c[1]*255.f),(int)(c[2]*255.f),(int)(c[3]*255.f)), 0);
}

// Defined with the aim tracer below; the curve tracer points at a body through
// the same lookup, so the two never disagree on where a body is.
static bool target_screen_point(const cache::entity_t& e, math::vector2& out);

// The nearest enemy to the cursor, in screen space.  This is what the curve
// tracer points at when it is asked to draw without silent aim and nothing is
// locked: the aimbot only holds a target *while the key is down* (its player is
// cleared the moment the key comes up), so a curve tied to it would blink in
// and out of existence - which is exactly how it was reported.  The checks are
// the ESP's own two, so the curve cannot follow a player the ESP is hiding.
static bool closest_enemy_screen_point(math::vector2& out)
{
	if (!game::visengine.address) return false;

	POINT cursor{};
	if (!GetCursorPos(&cursor)) return false;

	std::uint64_t local = 0;
	std::vector<cache::entity_t> snap;
	{
		std::lock_guard<std::mutex> lk(cache::mtx);
		local = cache::cached_local_player.instance.address;
		snap  = cache::cached_players;
	}

	float best = 0.f;
	bool  found = false;

	for (const auto& e : snap)
	{
		if (!e.instance.address || e.instance.address == local) continue;
		if (settings::visuals::team_check && cache::is_teammate(e)) continue;
		if (settings::visuals::dead_check && cache::is_dead(e))     continue;

		math::vector2 sp{};
		if (!target_screen_point(e, sp)) continue;

		const float dx = sp.x - (float)cursor.x;
		const float dy = sp.y - (float)cursor.y;
		const float d2 = dx * dx + dy * dy;
		if (!found || d2 < best) { best = d2; out = sp; found = true; }
	}

	return found;
}

static void draw_silent_aim_curve_tracer()
{
	if (!settings::silentaim::draw_curve_tracer) return;

	// Which target the curve points at.  0 - the default - is silent aim's own
	// target: the curve is a readout of what silent aim is aiming at, and it
	// only draws while silent aim has somebody.  1 is the nearest player to the
	// cursor, so the curve can be drawn on its own, with silent aim off or on
	// and simply not locked yet.
	const int  want = settings::silentaim::curve_tracer_target;
	const bool sa_live = settings::silentaim::enabled &&
		silentaim::state.data_ready && silentaim::state.target.instance.address != 0;

	if (!sa_live && want != 1) return;
	if (!game::visengine.address) return;

	POINT cursor{}; if (!GetCursorPos(&cursor)) return;
	const math::matrix4 view = game::visengine.get_viewmatrix();
	const math::vector2 dims = game::visengine.get_dimensions();

	math::vector2 hs{};
	bool have = false;

	if (sa_live && want != 1)
	{
		math::vector3 head = silentaim::state.target_world_pos;
		{ const std::uint64_t head_addr = silentaim::target_part("Head");
		  if (head_addr) {
			  rbx::primitive_t p = rbx::part_t(head_addr).get_primitive();
			  if (p.address) try { head = p.get_position(); } catch(...) {}
		  }
		}
		have = game::visengine.world_to_screen(head, hs, dims, view);
	}

	// The nearest player to the cursor, either because that is what the option
	// asks for or because silent aim has nobody yet.
	if (!have && want == 1)
		have = closest_enemy_screen_point(hs);

	if (!have) return;
	if (hs.x < 0.f || hs.y < 0.f || hs.x > 10000.f || hs.y > 10000.f) return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	ImVec2 start((float)cursor.x, (float)cursor.y);
	ImVec2 end(hs.x, hs.y);

	// The control point is offset along the curve's own normal rather than a
	// fixed +120 px below the midpoint, so a short curve between two nearly
	// level points is a shallow arc instead of a loop that leaves the screen.
	const float dx = end.x - start.x;
	const float dy = end.y - start.y;
	const float len = std::sqrt(dx * dx + dy * dy);
	const float bow = std::min(160.f, std::max(36.f, len * 0.28f));
	const ImVec2 control((start.x + end.x) * 0.5f + (len > 1.f ? -dy / len * bow * 0.25f : 0.f),
	                     (start.y + end.y) * 0.5f + bow);

	const auto& tc = settings::silentaim::curve_tracer_color;
	const float thick = std::max(0.5f, std::min(settings::silentaim::curve_tracer_thickness, 12.f));
	ImU32 col = IM_COL32((int)(tc[0]*255.f),(int)(tc[1]*255.f),(int)(tc[2]*255.f),(int)(tc[3]*255.f));

	// A bezier is a stroked polyline, so its joins are as lumpy as any other
	// stroke of the same width - which is what "grooves" is on a curve this
	// thick.  It is stroked once per pass instead, each pass a pixel apart, and
	// the segments are dense enough that no straight run of the polyline can
	// show: the curve is smooth by construction rather than by luck.
	const int segs = std::min(160, std::max(28, (int)(len / 4.f)));

	// Anti-aliased lines, forced on for the curve and put back afterwards: the
	// ESP draw list is the same list and a disabled flag there used to leave
	// every line in the frame - including this one - with hard stepped edges.
	const ImDrawListFlags old_flags = draw->Flags;
	draw->Flags = old_flags | ImDrawListFlags_AntiAliasedLines;

	// The outline, then the colour - the same two passes the ESP lines use.  What
	// is gone is the wide translucent pass that sat under the curve at two and a
	// half times its width: under a thin line that is not an edge, it is a glow,
	// and a glow around a colour the user picked is a halo of a colour they did
	// not choose.  One solid black stroke a pixel wider on each side does what the
	// halo was there for - the colour reads over a busy scene - and leaves the
	// line exactly the colour it was asked to be.
	if (settings::silentaim::curve_tracer_outline)
		draw->AddBezierCubic(start, control, control, end,
			IM_COL32(0, 0, 0, 255), thick + 1.4f, segs);
	draw->AddBezierCubic(start, control, control, end, col, thick, segs);

	// Round caps at both ends, so the curve starts at the cursor and meets the
	// target as a stroke rather than as a cut-off edge.
	draw->AddCircleFilled(start, thick * 0.5f + 0.5f, col, 24);
	draw->AddCircleFilled(end,   thick * 0.5f, col, 24);

	draw->Flags = old_flags;
}

static void draw_aim_line()
{
    if (!settings::visuals::aim_line) return;
    if (!game::visengine.address) return;

    // aimbot target first, then - with the option on - the silent aim one.
    unsigned long long target = 0;
    if (sync::hooks().aim_target_instance) target = sync::hooks().aim_target_instance();

    // The bridge hook deliberately has no silent aim fallback: its target flips
    // as the cursor drifts, which reads as jitter under a lock the aimbot does
    // not have.  A line is not a lock, though, so the line may follow it when
    // asked to - otherwise the line needs the aimbot (or a pin) before it draws
    // at all, which is what "the aim line needs silent aim on" really was.
    if (!target && settings::visuals::aim_line_silent && settings::silentaim::enabled &&
        silentaim::state.data_ready)
        target = (unsigned long long)silentaim::state.target.instance.address;

    if (!target) return;

    // The aim point first: the line is about where the aim is on the player. The
    // HUD anchor answers with the top of the name and avatar the ESP stacks above
    // the box, so the line used to end a font height and a half high.
    float ax = 0.f, ay = 0.f;
    bool  have = sync::hooks().player_aim_anchor
        && sync::hooks().player_aim_anchor(target, ax, ay);
    if (!have && sync::hooks().player_hud_anchor)
        have = sync::hooks().player_hud_anchor(target, ax, ay);
    if (!have) return;
    if (!std::isfinite(ax) || !std::isfinite(ay)) return;

    ImVec2 end(ax, ay);
    ImVec2 start;
    if (settings::visuals::aim_line_centre)
    {
        if (!game_crosshair_point(start))
        {
            const ImVec2 ds = ImGui::GetIO().DisplaySize;
            start = ImVec2(ds.x * 0.5f, ds.y * 0.5f);
        }
        start = ImVec2(std::floor(start.x), std::floor(start.y));
    }
    else
    {
        POINT cursor{};
        if (!GetCursorPos(&cursor)) return;
        start = ImVec2((float)cursor.x, (float)cursor.y);
    }

    const auto& c = settings::visuals::aim_line_color;
    const float th = std::max(0.5f, settings::visuals::aim_line_thickness);
    const ImU32 col  = IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f),
                                (int)(c[2] * 255.f), (int)(c[3] * 255.f));
    // Solid black, and the same line rather than a copy of it a pixel down:
    // the shadow was translucent and off-centre, which is what left the line
    // looking doubled instead of outlined.  One pixel of edge, not 1.6: past
    // that the pair stops reading as a stroked line and starts reading thick.
    const ImU32 dark = IM_COL32(0, 0, 0, 255);
    const float outline = th + 1.2f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImDrawListFlags backup = dl->Flags;
    dl->Flags |= ImDrawListFlags_AntiAliasedLines;

    const ImVec2 d(end.x - start.x, end.y - start.y);
    const float dist = std::sqrt(d.x * d.x + d.y * d.y);
    if (dist < 2.f) { dl->Flags = backup; return; }
    const ImVec2 dir(d.x / dist, d.y / dist);

    int aim_line_style = settings::visuals::aim_line_style;
    if (aim_line_style < 0) aim_line_style = 0;
    if (aim_line_style > 4) aim_line_style = 4;

    switch (aim_line_style)
    {
    case 1:
    {
        const float dash = 9.f, gap = 6.f;
        for (float t = 0.f; t < dist; t += dash + gap)
        {
            const float t1 = std::min(dist, t + dash);
            const ImVec2 a(start.x + dir.x * t,  start.y + dir.y * t);
            const ImVec2 b(start.x + dir.x * t1, start.y + dir.y * t1);
            dl->AddLine(a, b, dark, outline);
            dl->AddLine(a, b, col, th);
        }
        break;
    }
    case 2:
    {
        const float spc = 11.f;
        for (float t = 0.f; t <= dist; t += spc)
        {
            const float f = 1.f - (t / dist) * 0.65f;
            const ImVec2 p(start.x + dir.x * t, start.y + dir.y * t);
            dl->AddCircleFilled(p, th + 1.2f, IM_COL32(0, 0, 0, 255), 12);
            dl->AddCircleFilled(p, th + 0.4f,
                IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f), (int)(c[2] * 255.f),
                         (int)(c[3] * 255.f * f)), 10);
        }
        break;
    }
    case 3:
    {
        const ImVec2 n(-dir.y, dir.x);
        const int segs = 24;

        // The taper, drawn twice: once in solid black a little wider and once
        // in the colour.  Both strips share the same centre line, so the
        // outline is even on both sides - the tapered shape has no AddLine
        // form, which is why the outline has to be built the same way.
        const auto strip = [&](float width, ImU32 colour)
        {
            for (int i = 0; i < segs; ++i)
            {
                const float t0 = dist * (float)i / (float)segs;
                const float t1 = dist * (float)(i + 1) / (float)segs;
                const float w0 = width * 0.5f * (1.f - (float)i / (float)segs);
                const float w1 = width * 0.5f * (1.f - (float)(i + 1) / (float)segs);
                dl->AddQuadFilled(
                    ImVec2(start.x + dir.x * t0 + n.x * w0, start.y + dir.y * t0 + n.y * w0),
                    ImVec2(start.x + dir.x * t1 + n.x * w1, start.y + dir.y * t1 + n.y * w1),
                    ImVec2(start.x + dir.x * t1 - n.x * w1, start.y + dir.y * t1 - n.y * w1),
                    ImVec2(start.x + dir.x * t0 - n.x * w0, start.y + dir.y * t0 - n.y * w0), colour);
            }
        };

        strip(th + 1.6f, dark);
        strip(th, col);
        break;
    }
    case 4:
    {
        const float off = 2.4f;
        const ImVec2 n(-dir.y * off, dir.x * off);
        const float wt = th * 0.7f;
        dl->AddLine(ImVec2(start.x + n.x, start.y + n.y), ImVec2(end.x + n.x, end.y + n.y), dark, wt + 1.6f);
        dl->AddLine(ImVec2(start.x - n.x, start.y - n.y), ImVec2(end.x - n.x, end.y - n.y), dark, wt + 1.6f);
        dl->AddLine(ImVec2(start.x + n.x, start.y + n.y), ImVec2(end.x + n.x, end.y + n.y), col, wt);
        dl->AddLine(ImVec2(start.x - n.x, start.y - n.y), ImVec2(end.x - n.x, end.y - n.y), col, wt);
        break;
    }
    default:
        // Solid black under a solid line, both centred on the same line: the
        // outline is a real outline instead of a shadow down one side.
        dl->AddLine(start, end, dark, outline);
        dl->AddLine(start, end, col, th);
        break;
    }

    dl->Flags = backup;
}

static bool target_screen_point(const cache::entity_t& e, math::vector2& out)
{
    if (!game::visengine.address || !e.instance.address) return false;
    const math::matrix4 view = game::visengine.get_viewmatrix();
    const math::vector2 dims = game::visengine.get_dimensions();

    static const char* const k_points[] = { "Head", "HitboxHead", "HumanoidRootPart" };
    for (const char* n : k_points)
    {
        auto it = e.parts.find(n);
        if (it == e.parts.end() || !it->second.address) continue;
        try
        {
            rbx::primitive_t prim = it->second.get_primitive();
            if (!prim.address) continue;
            return game::visengine.world_to_screen(prim.get_position(), out, dims, view);
        }
        catch (...) {}
    }
    return false;
}

static void draw_aim_tracer()
{
    if (!settings::aimbot::tracer) return;

    math::vector2 sp{};
    bool have = false;

    if (settings::aimbot::enabled && aimbot::player.instance.address)
        have = target_screen_point(aimbot::player, sp);

    if (!have && settings::silentaim::enabled && silentaim::state.data_ready &&
        silentaim::state.target.instance.address)
        have = target_screen_point(silentaim::state.target, sp);

    if (!have) return;

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImVec2 from(ds.x * 0.5f, ds.y * 0.5f);
    switch (settings::aimbot::tracer_origin)
    {
    case 0: from = ImVec2(ds.x * 0.5f, ds.y); break;
    case 3: from = ImVec2(ds.x * 0.5f, 0.f);    break;
    case 2:
    {
        POINT pt{};
        if (GetCursorPos(&pt)) from = ImVec2((float)pt.x, (float)pt.y);
        break;
    }
    default:
    {
        // "Centre" is the game's crosshair, not the middle of the monitor.
        if (!game_crosshair_point(from))
            from = ImVec2(ds.x * 0.5f, ds.y * 0.5f);
        break;
    }
    }

    const float* c = settings::aimbot::tracer_color;
    const ImVec2 to(sp.x, sp.y);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    shape::apply_flags(dl);

    // The width is the user's now, and the outline is the same line one pixel
    // wider underneath it rather than a fixed pair of widths - a 1.25 px line
    // under a 2.6 px band was the "thick and dark" the tracer was called.
    const float th = std::max(0.5f, std::min(settings::aimbot::tracer_thickness, 12.f));
    const ImU32 col = IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f),
                               (int)(c[2] * 255.f), (int)(c[3] * 255.f));

    if (settings::aimbot::tracer_style == 1)
    {
        // The smooth style: one tapered band, round caps, fading toward the
        // target.  Same colour and width at the origin, no join anywhere.
        const ImU32 tip = IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f),
                                   (int)(c[2] * 255.f), (int)(c[3] * 255.f * 0.45f));
        if (settings::aimbot::tracer_outline)
            shape::tapered_band(dl, from, to, th + 1.6f,
                IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 160), 0.55f);
        shape::tapered_band(dl, from, to, th, col, tip, 0.55f);
    }
    else if (settings::aimbot::tracer_style == 2)
    {
        shape::beam_line(dl, from, to, th, col);
    }
    else if (settings::aimbot::tracer_style == 3)
    {
        shape::laser_line(dl, from, to, th, col, (float)ImGui::GetTime());
    }
    else
    {
        if (settings::aimbot::tracer_outline)
            dl->AddLine(from, to, IM_COL32(0, 0, 0, 255), th + 1.4f);
        dl->AddLine(from, to, col, th);
    }
}

void render_t::render_visuals()
{

    frame_clock::time_point phase_at = frame_clock::now();
    auto phase = [&](const char* name)
    {
        const frame_clock::time_point now = frame_clock::now();
        trace(name, ms_between(phase_at, now));
        phase_at = now;
    };

    // world_to_screen() answers in desktop pixels and every aim thread and
    // overlay pass goes through it, so the origin is refreshed here - in the
    // one pass that always runs - rather than only inside the ESP.
    rbx::visualengine_t::update_window_offset();
    log_viewport_map(rbx::visualengine_t::window_offset_x(),
                     rbx::visualengine_t::window_offset_y(),
                     game::visengine.get_dimensions());

    log_viewport_probe(detail->window);

    // The overlay's own line quality, set once for the pass that draws the
    // boxes, the skeleton, the ring and the tracers: every one of them reads it
    // from here rather than each feature keeping its own copy of the option.
    shape::set_quality(settings::appearance::antialiasing, settings::appearance::aa_quality);

    esp::run();
    phase("esp");

    if (game::visengine.address)
    {
        const math::vector2 fx_dims = game::visengine.get_dimensions();
        const math::matrix4 fx_view = game::visengine.get_viewmatrix();
        ImDrawList* fx_dl = ImGui::GetBackgroundDrawList();
        esp_trails::draw(fx_dl, fx_dims, fx_view);
        sonar::draw(fx_dl, fx_dims, fx_view);
    }
    phase("trail");
    // The aimbot's own ring is drawn once, by aimbot::draw_fov from inside the
    // ESP pass (it carries the fill and the outline there).  It used to be drawn
    // here as well, from the same cursor with different widths - two rings a
    // pixel apart, which is what made the circle look thick.
    if (settings::silentaim::draw_fov && settings::silentaim::enabled)
        draw_fov_circle(settings::silentaim::fov, settings::silentaim::fov_circle_colour,
            settings::silentaim::fov_outline_colour, settings::silentaim::fov_thickness,
            settings::silentaim::fov_outline, settings::silentaim::fov_style,
            settings::silentaim::attach_fov_to_target,
            settings::silentaim::fov_fill, settings::silentaim::fov_fill_colour, settings::silentaim::fov_fill_opacity);
    if (settings::triggerbot::draw_fov && settings::triggerbot::enabled)
        draw_fov_circle(settings::triggerbot::fov, settings::triggerbot::fov_circle_colour,
            settings::triggerbot::fov_outline_colour, settings::triggerbot::fov_thickness,
            settings::triggerbot::fov_outline, settings::triggerbot::fov_style, false,
            settings::triggerbot::fov_fill, settings::triggerbot::fov_fill_colour, settings::triggerbot::fov_fill_opacity);
    phase("fov");

    if (game::visengine.address)
        effects::draw(ImGui::GetBackgroundDrawList(),
            game::visengine.get_dimensions(), game::visengine.get_viewmatrix());

    draw_custom_cursor();
    fusion::draw_overlay(ImGui::GetBackgroundDrawList(), game::visengine.get_dimensions());
    draw_aim_tracer();
    draw_silent_aim_target_dot();
    draw_aim_line();
    draw_silent_aim_curve_tracer();

    phase("fx");

    radar::draw();
    phase("radar");

    ImVec2 ds = ImGui::GetIO().DisplaySize;
    world_effects::render(ds.x, ds.y);
    phase("world");
}

static int   s_ping_ms      = -1;
static bool  s_ping_started = false;

static void ping_measure_thread()
{
    using namespace std::chrono;

    double s_last_game_time = 0.0;
    auto   s_last_wall_time = steady_clock::now();

    for (;;)
    {
        std::this_thread::sleep_for(milliseconds(500));

        try
        {
            if (!game::datamodel.address) { s_ping_ms = -1; continue; }

            double game_time = 0.0;
            try {
                std::uint64_t ws = memory->read<std::uint64_t>(
                    game::datamodel.address + Offsets::DataModel::Workspace);
                if (ws)
                    game_time = memory->read<double>(ws + Offsets::Workspace::DistributedGameTime);
            } catch (...) {}

            auto   now     = steady_clock::now();
            double wall_dt = duration_cast<duration<double>>(now - s_last_wall_time).count();
            double game_dt = game_time - s_last_game_time;

            if (s_last_game_time > 0.0 && wall_dt > 0.05 && game_dt > 0.0)
            {
                double drift = (wall_dt - game_dt) * 1000.0;
                int ping     = (int)(std::abs(drift) * 2.0);
                ping         = std::max(0, std::min(ping, 999));
                s_ping_ms    = (s_ping_ms < 0) ? ping
                                               : (int)(s_ping_ms * 0.7 + ping * 0.3);
            }

            s_last_game_time = game_time;
            s_last_wall_time = now;
        }
        catch (...) { s_ping_ms = -1; }
    }
}

int render_t::get_ping_ms()
{
    if (!s_ping_started)
    {
        s_ping_started = true;
        std::thread(ping_measure_thread).detach();
    }
    return s_ping_ms;
}
