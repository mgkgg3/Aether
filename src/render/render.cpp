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
#include <features/esp/trails/Trails.h>
#include <features/misc/Sonar.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>

#include <gui/menu/SyncBridge.h>

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

    void sync_streamproof(HWND hwnd, bool also_exclude = false)
    {
        static HWND  s_hwnd     = (HWND)-1;
        static bool  s_enabled  = false;
        static bool  s_excluded = false;
        static DWORD s_last_run = 0;

        if (!hwnd)
            return;

        const bool  want      = settings::streamproof::enabled || also_exclude;
        const DWORD now       = GetTickCount();
        const bool  changed   = (hwnd != s_hwnd) || (want != s_enabled) || (also_exclude != s_excluded);
        const bool  heartbeat = (now - s_last_run) > 2000;
        if (!changed && !heartbeat)
            return;

        s_last_run = now;
        s_excluded = also_exclude;

        const DWORD desired = want ? WDA_EXCLUDEFROMCAPTURE_ : WDA_NONE_;

        const bool for_backdrop = also_exclude && !settings::streamproof::enabled;

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

        if (for_backdrop && desired != wda_query(hwnd))
            debug::log("render", "backdrop: overlay excluded from capture (it must not capture the panel)");

        BOOL ok = set(hwnd, desired);

        if (!ok && want)
        {
            ok = set(hwnd, WDA_MONITOR_);
            if (ok)
                debug::log("render", "streamproof: WDA_EXCLUDEFROMCAPTURE rejected, fell back to WDA_MONITOR (pre-2004 Windows)");
        }

        if (!ok && want)
            debug::warn("streamproof could not be applied (error %lu); overlay may still appear in captures", GetLastError());

        const DWORD applied = wda_query(hwnd);
        const char* state = (applied == WDA_EXCLUDEFROMCAPTURE_) ? "excluded from capture"
                          : (applied == WDA_MONITOR_)            ? "monitor-only (blank in captures)"
                          : (want ? "NOT APPLIED" : "off");
        debug::log("render", "streamproof %s | affinity=0x%02lX", state, applied);

        s_hwnd = hwnd;
        s_enabled = want;
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

    int sample_threads(thread_sample_t* out, int max)
    {
        const HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE)
            return 0;

        const unsigned long self = ::GetCurrentProcessId();
        int count = 0;

        THREADENTRY32 te{}; te.dwSize = sizeof te;
        if (::Thread32First(snap, &te))
        {
            do
            {
                if (te.th32OwnerProcessID != self) continue;
                if (count >= max) break;

                const HANDLE th = ::OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te.th32ThreadID);
                if (!th) continue;

                FILETIME creation{}, exit{}, kernel{}, user{};
                if (::GetThreadTimes(th, &creation, &exit, &kernel, &user))
                {
                    out[count].id     = te.th32ThreadID;
                    out[count].cpu_ms = filetime_ms(kernel) + filetime_ms(user);
                    ++count;
                }
                ::CloseHandle(th);
            } while (::Thread32Next(snap, &te));
        }

        ::CloseHandle(snap);
        return count;
    }

    void report_threads(double elapsed_ms)
    {
        constexpr int k_max_threads = 128;
        static thread_sample_t s_prev[k_max_threads]{};
        static int s_prev_count = 0;

        thread_sample_t now[k_max_threads]{};
        const int now_count = sample_threads(now, k_max_threads);
        if (now_count <= 0)
            return;

        struct line_t { unsigned long id; double ms; };
        line_t top[4]{};
        int top_count = 0;

        for (int i = 0; i < now_count; ++i)
        {
            double before = 0.0;
            for (int j = 0; j < s_prev_count; ++j)
            {
                if (s_prev[j].id == now[i].id) { before = s_prev[j].cpu_ms; break; }
            }
            const double delta = now[i].cpu_ms - before;
            if (delta <= 0.0) continue;

            if (top_count < 4) { top[top_count++] = { now[i].id, delta }; }
            else
            {
                int least = 0;
                for (int k = 1; k < 4; ++k) if (top[k].ms < top[least].ms) least = k;
                if (delta > top[least].ms) top[least] = { now[i].id, delta };
            }
        }

        for (int i = 0; i < now_count && i < k_max_threads; ++i)
            s_prev[i] = now[i];
        s_prev_count = now_count;

        char buf[320]{};
        int used = 0;
        for (int i = 0; i < top_count; ++i)
            used += snprintf(buf + used, sizeof buf - (std::size_t)used, " tid %lu=%.0f%%",
                top[i].id, elapsed_ms > 0.0 ? top[i].ms * 100.0 / elapsed_ms : 0.0);

        debug::log("render", "threads %d | busiest:%s",
            now_count, used > 0 ? buf : " (none)");
    }

    void phase_add(const char* name, double ms)
    {
        if (!name || ms < 0.0)
            return;

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
            "fps=%.1f | frame build=%.2fms present=%.2fms pace=%.2fms | worst=%.2fms | layered=%d |%s",
            n * 1000.0 / elapsed,
            g_frame_totals.build_ms / n,
            g_frame_totals.present_ms / n,
            g_frame_totals.pace_ms / n,
            g_frame_totals.worst_ms,
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

    sync_streamproof(detail->window, false);
    sync::backdrop() = sync::Backdrop{};
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

static void draw_fov_circle(float fov_radius, float circle_colour[4], float outline_colour[4],
    bool attach_to_target = false,
    bool fill = false, float fill_colour[4] = nullptr, float fill_opacity = 0.f)
{
    if (!game::visengine.address) return;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
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

    if (fill && fill_colour)
    {
        const float a = fill_opacity * 2.55f;
        const ImU32 fill_col = IM_COL32(
            (int)(fill_colour[0]*255.f), (int)(fill_colour[1]*255.f),
            (int)(fill_colour[2]*255.f), (int)(fill_colour[3]*a));
        draw->AddCircleFilled(center, fov_radius, fill_col, 0);
    }

    draw->AddCircle(center, fov_radius, outline_col, 0, 3.f);
    draw->AddCircle(center, fov_radius, circle_col,  0, 1.f);
}

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

static void draw_silent_aim_curve_tracer()
{
	if (!settings::silentaim::draw_curve_tracer || !settings::silentaim::enabled) return;
	if (!silentaim::state.data_ready || !silentaim::state.target.instance.address) return;
	if (!game::visengine.address) return;

	POINT cursor{}; if (!GetCursorPos(&cursor)) return;
	const math::matrix4 view = game::visengine.get_viewmatrix();
	const math::vector2 dims = game::visengine.get_dimensions();

	math::vector3 head = silentaim::state.target_world_pos;
	{ const std::uint64_t head_addr = silentaim::target_part("Head");
	  if (head_addr) {
		  rbx::primitive_t p = rbx::part_t(head_addr).get_primitive();
		  if (p.address) try { head = p.get_position(); } catch(...) {}
	  }
	}
	math::vector2 hs;
	if (!game::visengine.world_to_screen(head, hs, dims, view)) return;
	if (hs.x < 0.f || hs.y < 0.f || hs.x > 10000.f || hs.y > 10000.f) return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	ImVec2 start((float)cursor.x, (float)cursor.y);
	ImVec2 end(hs.x, hs.y);

	float midX = (start.x + end.x) * 0.5f;
	float midY = (start.y + end.y) * 0.5f;
	ImVec2 control(midX, midY + 120.f);

	const auto& tc = settings::silentaim::curve_tracer_color;
	const int segs = 35;
	const float thick = settings::silentaim::curve_tracer_thickness;
	ImU32 col = IM_COL32((int)(tc[0]*255.f),(int)(tc[1]*255.f),(int)(tc[2]*255.f),(int)(tc[3]*255.f));

	draw->AddBezierCubic(start, control, control, end, col, thick, segs);
}

static void draw_aim_line()
{
    if (!settings::visuals::aim_line) return;
    if (!game::visengine.address) return;

    unsigned long long target = 0;
    if (sync::hooks().aim_target_instance) target = sync::hooks().aim_target_instance();
    if (!target) return;

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
    const ImU32 dark = IM_COL32(0, 0, 0, (int)(c[3] * 90.f));

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
            dl->AddLine(a, b, dark, th + 2.f);
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
            dl->AddCircleFilled(p, th + 1.6f, IM_COL32(0, 0, 0, (int)(c[3] * 70.f * f)), 10);
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
        for (int i = 0; i < segs; ++i)
        {
            const float t0 = dist * (float)i / (float)segs;
            const float t1 = dist * (float)(i + 1) / (float)segs;
            const float w0 = th * 0.5f * (1.f - (float)i / (float)segs);
            const float w1 = th * 0.5f * (1.f - (float)(i + 1) / (float)segs);
            dl->AddQuadFilled(
                ImVec2(start.x + dir.x * t0 + n.x * w0, start.y + dir.y * t0 + n.y * w0),
                ImVec2(start.x + dir.x * t1 + n.x * w1, start.y + dir.y * t1 + n.y * w1),
                ImVec2(start.x + dir.x * t1 - n.x * w1, start.y + dir.y * t1 - n.y * w1),
                ImVec2(start.x + dir.x * t0 - n.x * w0, start.y + dir.y * t0 - n.y * w0), col);
        }
        break;
    }
    case 4:
    {
        const float off = 2.4f;
        const ImVec2 n(-dir.y * off, dir.x * off);
        dl->AddLine(ImVec2(start.x + n.x, start.y + n.y), ImVec2(end.x + n.x, end.y + n.y), col, th * 0.7f);
        dl->AddLine(ImVec2(start.x - n.x, start.y - n.y), ImVec2(end.x - n.x, end.y - n.y), col, th * 0.7f);
        break;
    }
    default:
        dl->AddLine(start, end, dark, th + 2.f);
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
        if (!game_crosshair_point(from))
            from = ImVec2(ds.x * 0.5f, ds.y * 0.5f);
        break;
    }
    }

    const float* c = settings::aimbot::tracer_color;
    const ImVec2 to(sp.x, sp.y);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddLine(from, to, IM_COL32(0, 0, 0, (int)(c[3] * 180.f)), 2.5f);
    dl->AddLine(from, to,
        IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f), (int)(c[2] * 255.f), (int)(c[3] * 255.f)), 1.25f);
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

    rbx::visualengine_t::update_window_offset();
    log_viewport_map(rbx::visualengine_t::window_offset_x(),
                     rbx::visualengine_t::window_offset_y(),
                     game::visengine.get_dimensions());

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
    if (settings::aimbot::draw_fov && settings::aimbot::enabled)
        draw_fov_circle(settings::aimbot::fov, settings::aimbot::fov_circle_colour, settings::aimbot::fov_outline_colour, false,
            settings::aimbot::fov_fill, settings::aimbot::fov_fill_colour, settings::aimbot::fov_fill_opacity);
    if (settings::silentaim::draw_fov && settings::silentaim::enabled)
        draw_fov_circle(settings::silentaim::fov, settings::silentaim::fov_circle_colour, settings::silentaim::fov_outline_colour, true,
            settings::silentaim::fov_fill, settings::silentaim::fov_fill_colour, settings::silentaim::fov_fill_opacity);
    if (settings::triggerbot::draw_fov && settings::triggerbot::enabled)
        draw_fov_circle(settings::triggerbot::fov, settings::triggerbot::fov_circle_colour, settings::triggerbot::fov_outline_colour, false,
            settings::triggerbot::fov_fill, settings::triggerbot::fov_fill_colour, settings::triggerbot::fov_fill_opacity);
    phase("fov");

    if (game::visengine.address)
        effects::draw(ImGui::GetBackgroundDrawList(),
            game::visengine.get_dimensions(), game::visengine.get_viewmatrix());

    draw_custom_cursor();
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
