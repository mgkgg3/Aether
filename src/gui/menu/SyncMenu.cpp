
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <gui/AetherFonts.h>
#include <gui/AetherIcons.h>

#include <settings.h>
#include <game/game_context.h>
#include <memory/memory.h>

#include "AetherMenu.h"
#include <keybind/keybind.h>
#include <keybind/live_state.h>

#include "SyncMenu.h"

#include <features/misc/UiFx.h>

#include "SyncBridge.h"

#include <cfloat>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

static sync::Cost g_cost;
static double     g_worst_panel_ms = 0.0;
static double     g_page_ms = 0.0;
static double     g_count_ms = 0.0;

using menu_clock = std::chrono::steady_clock;
static inline menu_clock::time_point cost_now() { return menu_clock::now(); }
static inline double cost_ms(menu_clock::time_point from)
{
    return std::chrono::duration<double, std::milli>(menu_clock::now() - from).count();
}

static HWND g_hwnd = nullptr;

static void DrawBackdropRect(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float rounding, float alpha = 255.f)
{
    if (!dl || !g_hwnd || alpha <= 0.5f)
        return;

    sync::occlusion().add(p0.x, p0.y, p1.x, p1.y);

    const sync::Backdrop& b = sync::backdrop();
    if (!b.ready())
        return;

    POINT origin{};
    if (!::ClientToScreen(g_hwnd, &origin))
        return;

    const auto clamp_uv = [](const ImVec2& uv)
    {
        return ImVec2(ImClamp(uv.x, 0.f, 1.f), ImClamp(uv.y, 0.f, 1.f));
    };

    const ImVec2 uv0 = clamp_uv(b.uv(p0.x, p0.y, (float)origin.x, (float)origin.y));
    const ImVec2 uv1 = clamp_uv(b.uv(p1.x, p1.y, (float)origin.x, (float)origin.y));

    static float s_correction = -1.f;
    {
        const float target = b.capture_has_self ? 1.f : 0.f;
        const float dt = ImGui::GetIO().DeltaTime;
        if (s_correction < 0.f)
            s_correction = target;
        else
        {
            s_correction += (target - s_correction) * ImMin(1.f, (dt > 0.f ? dt : 0.f) * 6.f);
            if (ImFabs(target - s_correction) < 0.004f)
                s_correction = target;
        }
    }

    const float dim_fade = (sync::state().dim_background && s_correction < 0.999f)
                         ? ImClamp(sync::fade(), 0.f, 1.f) * (1.f - s_correction) : 0.f;
    const int lum = (int)(255.f - 120.f * dim_fade + 0.5f);

    const int a = alpha < 0.f ? 0 : (alpha > 255.f ? 255 : (int)(alpha + 0.5f));
    dl->AddImageRounded(b.image, p0, p1, uv0, uv1, IM_COL32(lum, lum, lum, a), rounding);
}

namespace K
{
    constexpr float W = 930.f;
    constexpr float H = 600.f;

    constexpr float PanelY = 90.f;
    constexpr float WinH   = PanelY + H + 10.f;
    constexpr float CardX  = 14.f, CardY = 12.f;
    constexpr float CardW  = 292.f, CardH = 66.f;

    constexpr float RoundWindow = 22.f;

    constexpr float Pad = 16.f;
    constexpr float TopBarH = 44.f;
    constexpr float GapTopBody = 14.f;
    constexpr float SidebarW = 150.f;
    constexpr float DividerGap = 12.f;
    constexpr float ContentPadTop = 2.f;
    constexpr float ScrollReserve = 10.f;

    constexpr float TabH = 38.f;
    constexpr float TabGap = 5.f;
    constexpr float TabInset = 6.f;

    constexpr float RowPad = 5.f;
    constexpr float RowGap = 5.f;

    constexpr float ToggleW = 40.f, ToggleH = 22.f;
    constexpr float ComboW = 158.f, ComboH = 30.f;
    constexpr float KeybindW = 150.f, KeybindH = 34.f;
    constexpr float SliderTrackH = 6.f, SliderThumbD = 14.f;
    constexpr float SwatchD = 30.f;
    constexpr float AvatarD = 26.f;

    constexpr float PreviewW = 280.f, PreviewH = 330.f;

    constexpr float TextSize = 13.f, TextSmall = 11.f, TitleSize = 15.f;

    constexpr float UiAnimSpeed = 18.f;
}

static ImVec2 g_PanelPos = ImVec2(-1.f, -1.f);
static ImVec2 g_CardPos  = ImVec2(K::CardX, K::CardY);

static int GlassAlpha()
{
    if (const char* e = getenv("SYNC_GLASS"))
    {
        const int v = atoi(e);
        if (v >= 0 && v <= 255) return v;
    }

    const int base = 84;

    const float t = ImClamp(sync::state().glass_transparency, 0.f, 100.f) * 0.01f;
    return base - (int)((float)(base - 20) * t + 0.5f);
}

struct Theme
{

    ImU32 windowBg;
    ImU32 windowBorder;
    ImU32 topBarBg;
    ImU32 topBarBorder;
    ImU32 sidebarBg;
    ImU32 divider;
    ImU32 hairline;
    ImU32 rowHover;
    ImU32 glow;
    ImU32 popupBg;

    ImU32 textPrimary;
    ImU32 textDim;
    ImU32 textFaint;
    ImU32 textOnPill;

    ImU32 controlBg;
    ImU32 controlBorder;
    ImU32 controlBorderHi;
    ImU32 pillActive;
    ImU32 toggleOn;
    ImU32 toggleOff;
    ImU32 knobOn;
    ImU32 knobOff;
    ImU32 sliderTrack;
    ImU32 sliderFill;
    ImU32 sliderThumb;

    float roundWindow;
    float roundTopBar;
    float roundPill;
    float roundRow;
    float roundControl;
    float roundPopup;
    float borderThickness;
    float dividerThickness;
};

static Theme g_T;

static ImU32 tint(const float c[4], float to_a[4], float t, int alpha)
{
    const float r = ImClamp(c[0] + (to_a[0] - c[0]) * t, 0.f, 1.f);
    const float g = ImClamp(c[1] + (to_a[1] - c[1]) * t, 0.f, 1.f);
    const float b = ImClamp(c[2] + (to_a[2] - c[2]) * t, 0.f, 1.f);
    return IM_COL32((int)(r * 255.f + 0.5f), (int)(g * 255.f + 0.5f),
                    (int)(b * 255.f + 0.5f), alpha);
}

static ImU32 solid(const float c[4], int alpha)
{
    return IM_COL32((int)(ImClamp(c[0], 0.f, 1.f) * 255.f + 0.5f),
                    (int)(ImClamp(c[1], 0.f, 1.f) * 255.f + 0.5f),
                    (int)(ImClamp(c[2], 0.f, 1.f) * 255.f + 0.5f), alpha);
}

static void ApplyStyle()
{
    float* accent = settings::theme::accent;
    float* bg     = settings::theme::bg_one;
    float* surface = settings::theme::bg_two;
    float* stroke = settings::theme::stroke;
    float* text   = settings::theme::text_active;
    float* dim    = settings::theme::text_inactive;

    nm::sync_theme();

    g_T.windowBg       = solid(bg, GlassAlpha());

    g_T.windowBorder   = tint(stroke, accent, 0.35f,  26);
    g_T.topBarBg       = tint(surface, accent, 0.10f,   8);
    g_T.topBarBorder   = tint(stroke, accent, 0.30f,  24);
    g_T.sidebarBg      = IM_COL32(0, 0, 0, 0);

    g_T.divider        = tint(stroke, accent, 0.15f,  16);
    g_T.hairline       = tint(stroke, accent, 0.10f,  14);
    g_T.rowHover       = tint(surface, accent, 0.22f, 12);
    g_T.glow           = solid(accent, 255);
    g_T.popupBg        = tint(bg, surface, 0.50f, 196);

    g_T.textPrimary    = solid(text, 255);
    g_T.textDim        = solid(dim, 255);
    g_T.textFaint      = tint(dim, bg, 0.35f, 255);
    g_T.textOnPill     = solid(bg, 255);

    g_T.controlBg      = tint(bg, surface, 0.25f,  90);
    g_T.controlBorder  = tint(stroke, accent, 0.35f, 34);
    g_T.controlBorderHi= tint(stroke, accent, 0.60f, 92);
    g_T.pillActive     = solid(accent, 255);
    g_T.toggleOn       = solid(accent, 255);

    g_T.toggleOff      = solid(bg, 210);
    g_T.knobOn         = tint(text, accent, 0.15f, 255);
    g_T.knobOff        = solid(text, 255);

    g_T.sliderTrack    = tint(surface, accent, 0.25f, 40);
    g_T.sliderFill     = solid(accent, 255);
    g_T.sliderThumb    = solid(text, 255);
}

static void ApplyImGuiStyle()
{

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding      = g_T.roundWindow;
    s.WindowBorderSize    = 0.f;
    s.WindowPadding       = ImVec2(0, 0);
    s.ChildRounding       = 0.f;
    s.ChildBorderSize     = 0.f;
    s.PopupRounding       = g_T.roundPopup;
    s.PopupBorderSize     = g_T.borderThickness;
    s.FrameRounding       = g_T.roundControl;
    s.GrabRounding        = g_T.roundControl;
    s.ScrollbarRounding   = 6.f;
    s.ScrollbarSize       = 6.f;
    s.ItemSpacing         = ImVec2(8, 5);
    s.AntiAliasedLines    = true;
    s.AntiAliasedFill     = true;
    s.Alpha               = 1.f;

    s.Colors[ImGuiCol_WindowBg]      = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_Border]        = ImGui::ColorConvertU32ToFloat4(g_T.windowBorder);
    s.Colors[ImGuiCol_ChildBg]       = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_PopupBg]       = ImGui::ColorConvertU32ToFloat4(g_T.popupBg);
    s.Colors[ImGuiCol_ScrollbarBg]   = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1, 1, 1, 0.12f);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1, 1, 1, 0.20f);
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(1, 1, 1, 0.28f);
    s.Colors[ImGuiCol_Text]          = ImGui::ColorConvertU32ToFloat4(g_T.textPrimary);

    g_T.roundWindow    = K::RoundWindow;
    g_T.roundTopBar    = 13.f;
    g_T.roundPill      = 11.f;
    g_T.roundRow       = 9.f;
    g_T.roundControl   = 9.f;
    g_T.roundPopup     = 12.f;
    g_T.borderThickness= 1.f;
    g_T.dividerThickness = 1.f;
}

static const float kPi = 3.14159265358979f;

static inline float Minf(float a, float b)   { return a < b ? a : b; }
static inline float Maxf(float a, float b)   { return a > b ? a : b; }
static inline float Clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float Lerpf(float a, float b, float t)  { return a + (b - a) * t; }
static inline float SmoothStep(float t)      { t = Clampf(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }
static inline float EaseOutCubic(float t)    { const float u = 1.f - Clampf(t, 0.f, 1.f); return 1.f - u * u * u; }

static const float kBubbleClosed   = 0.74f;
static const float kBubbleResponse = 0.34f;
static const float kBubbleDamping  = 0.68f;

static inline float BubbleAlpha(float t) { return Clampf(t, 0.f, 1.f); }

static inline float Approach(float cur, float target, float speed, float dt)
{
    return cur + (target - cur) * (1.f - expf(-speed * dt));
}

struct Spring { float x = 0.f, v = 0.f; };
static float SpringStep(Spring& s, float target, float response, float damping, float dt)
{
    const float omega = 2.f * kPi / Maxf(response, 0.02f);
    const float f = 1.f + 2.f * dt * damping * omega;
    const float oo = omega * omega;
    const float hoo = dt * oo;
    const float hhoo = dt * hoo;
    const float detInv = 1.f / (f + hhoo);
    const float detX = f * s.x + dt * s.v + hhoo * target;
    const float detV = s.v + hoo * (target - s.x);

    s.x = detX * detInv;
    s.v = detV * detInv;
    return s.x;
}

static ImU32 Mix(ImU32 a, ImU32 b, float t)
{
    t = Clampf(t, 0.f, 1.f);
    const float ia  = (float)((a >> IM_COL32_A_SHIFT) & 0xFF), ja = (float)((b >> IM_COL32_A_SHIFT) & 0xFF);
    const float ir  = (float)((a >> IM_COL32_R_SHIFT) & 0xFF), jr = (float)((b >> IM_COL32_R_SHIFT) & 0xFF);
    const float ig  = (float)((a >> IM_COL32_G_SHIFT) & 0xFF), jg = (float)((b >> IM_COL32_G_SHIFT) & 0xFF);
    const float ib  = (float)((a >> IM_COL32_B_SHIFT) & 0xFF), jb = (float)((b >> IM_COL32_B_SHIFT) & 0xFF);
    return IM_COL32((int)(ir + (jr - ir) * t), (int)(ig + (jg - ig) * t),
                    (int)(ib + (jb - ib) * t), (int)(ia + (ja - ia) * t));
}

static float g_AlphaMul = 1.f;

static float g_MenuFade = 1.f;
static inline ImU32 GC(ImU32 c, float mul = 1.f) { return ImGui::GetColorU32(c, g_AlphaMul * g_MenuFade * mul); }

static float g_MenuDrawAlpha = 1.f;

static float g_CascadeShiftDbg = 0.f;

static std::unordered_map<ImGuiID, float>&  AnimStore()  { static std::unordered_map<ImGuiID, float>  m; return m; }
static std::unordered_map<ImGuiID, Spring>& SpringStore(){ static std::unordered_map<ImGuiID, Spring> m; return m; }

static float& AnimF(const char* tag, float init = 0.f)
{
    const ImGuiID id = ImGui::GetID(tag);
    auto it = AnimStore().find(id);
    if (it == AnimStore().end()) it = AnimStore().emplace(id, init).first;
    return it->second;
}

static float SpringToId(ImGuiID id, float target, float response, float damping, float init = 0.f)
{
    auto it = SpringStore().find(id);
    if (it == SpringStore().end()) { Spring s; s.x = init; it = SpringStore().emplace(id, s).first; }
    return SpringStep(it->second, target, response, damping, ImGui::GetIO().DeltaTime);
}

static float SpringToIdDt(ImGuiID id, float target, float response, float damping, float dt,
                          float init = 0.f)
{
    auto it = SpringStore().find(id);
    if (it == SpringStore().end()) { Spring s; s.x = init; it = SpringStore().emplace(id, s).first; }
    return SpringStep(it->second, target, response, damping, Clampf(dt, 0.f, 1.f / 20.f));
}
static float SpringTo(const char* tag, float target, float response, float damping, float init = 0.f)
{
    return SpringToId(ImGui::GetID(tag), target, response, damping, init);
}
static void SpringSetId(ImGuiID id, float x, float v = 0.f)
{
	Spring& s = SpringStore()[id];
	s.x = x;
	s.v = v;
}

static float SpringVelId(ImGuiID id)
{
	auto it = SpringStore().find(id);
	return (it == SpringStore().end()) ? 0.f : it->second.v;
}

static ImFont* g_FontRegular = nullptr;
static ImFont* g_FontBold    = nullptr;
static ImFont* g_FontTitle   = nullptr;
static ImFont* g_FontSmall   = nullptr;

static ImFont* g_FontDisplay     = nullptr;
static ImFont* g_FontDisplayBold = nullptr;
static ImFont* g_FontValue       = nullptr;
static bool    g_BoldIsReal  = true;

static inline ImVec2 TSize(ImFont* f, const char* t) { return f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, t); }

static inline ImVec2 SnapText(ImVec2 p) { return ImVec2(floorf(p.x + 0.5f), floorf(p.y + 0.5f)); }

static const float kTextShadow = 0.16f;

static inline void   Txt(ImDrawList* dl, ImFont* f, ImVec2 p, ImU32 col, const char* t)
{
    const ImVec2 sp = SnapText(p);
    if (kTextShadow > 0.f)
    {
        const float a = (float)((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
        dl->AddText(f, f->FontSize, ImVec2(sp.x, sp.y + 1.f),
                    GC(IM_COL32(0, 0, 0, (int)(255.f * kTextShadow * a))), t);
    }
    dl->AddText(f, f->FontSize, sp, col, t);
}
static inline void TxtBold(ImDrawList* dl, const char* t, ImVec2 p, ImU32 col, bool title = false)
{
    ImFont* f = (title && g_FontTitle) ? g_FontTitle : (g_FontBold ? g_FontBold : g_FontRegular);
    if (g_BoldIsReal) { Txt(dl, f, p, col, t); return; }

    const ImVec2 sp = SnapText(p);
    dl->AddText(f, f->FontSize, sp, col, t);
    dl->AddText(f, f->FontSize, ImVec2(sp.x + 0.5f, sp.y), col, t);
}
static inline ImVec2 TSizeBold(const char* t, bool title = false)
{
    ImFont* f = (title && g_FontTitle) ? g_FontTitle : (g_FontBold ? g_FontBold : g_FontRegular);
    ImVec2 s = TSize(f, t);
    if (!g_BoldIsReal) s.x += 0.45f;
    return s;
}
static void TxtRight(ImDrawList* dl, ImFont* f, float right, float y, ImU32 col, const char* t)
{
    Txt(dl, f, ImVec2(right - TSize(f, t).x, y), col, t);
}

static void AddArc(ImDrawList* dl, ImVec2 c, float r, float a0, float a1, ImU32 col, float th)
{
    dl->PathClear();
    dl->PathArcTo(c, r, a0, a1, 32);
    dl->PathStroke(col, 0, th);
}
static void AddEllipseStroke(ImDrawList* dl, ImVec2 c, float rx, float ry, ImU32 col, float th, int seg = 32)
{
    dl->PathClear();
    for (int i = 0; i <= seg; i++) {
        const float a = (float)i / (float)seg * 2.f * kPi;
        dl->PathLineTo(ImVec2(c.x + cosf(a) * rx, c.y + sinf(a) * ry));
    }
    dl->PathStroke(col, 0, th);
}
static void AddChevron(ImDrawList* dl, ImVec2 c, float halfW, float halfH, ImU32 col, float th)
{
    dl->AddLine(ImVec2(c.x - halfW, c.y - halfH), ImVec2(c.x, c.y + halfH), col, th);
    dl->AddLine(ImVec2(c.x, c.y + halfH), ImVec2(c.x + halfW, c.y - halfH), col, th);
}

static void AddGlow(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, float strength, float spread = 5.f)
{

    uifx::note_control(dl, mn, mx, rounding);

    if (strength <= 0.004f) return;
    const float s = Clampf(strength, 0.f, 1.f);
    const ImU32 col = uifx::on() ? uifx::light(1.f) : g_T.glow;
    const float g = Maxf(1.2f, spread * 0.55f);

    dl->AddRect(ImVec2(mn.x - g, mn.y - g), ImVec2(mx.x + g, mx.y + g), GC(col, 0.10f * s),
                rounding + g, 0, 1.f + g * 0.22f);
    dl->AddRect(ImVec2(mn.x - 1.f, mn.y - 1.f), ImVec2(mx.x + 1.f, mx.y + 1.f), GC(col, 0.34f * s),
                rounding + 1.f, 0, 1.4f);
}

static void AddRipple(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, float r)
{
    if (r <= 0.004f) return;
    const float g = (1.f - r) * 13.f;
    dl->AddRect(ImVec2(mn.x - g, mn.y - g), ImVec2(mx.x + g, mx.y + g), GC(g_T.glow, r * 0.26f),
                rounding + g, 0, 1.6f);
}

struct Feedback { float shine = 0.f; float ripple = 0.f; float hover = 0.f; };
static Feedback FeedbackUpdate(bool hovered, bool pressed, bool active = false)
{
    const float dt = ImGui::GetIO().DeltaTime;
    float& sh = AnimF("##shine", 0.f);
    float& rp = AnimF("##ripple", 0.f);
    float& hv = AnimF("##hoverfb", 0.f);
    if (pressed) rp = 1.f;
    const float target = pressed ? 1.f : (active ? 0.75f : 0.f);
    sh = Approach(sh, target, pressed ? 26.f : 9.f, dt);
    hv = Approach(hv, hovered ? 1.f : 0.f, K::UiAnimSpeed, dt);
    rp = Approach(rp, 0.f, 3.4f, dt);
    Feedback f;
    f.shine = sh;
    f.ripple = rp;
    f.hover = hv;

    uifx::note_feedback(f.hover, f.ripple, f.shine);
    return f;
}

enum TabIcon { ICON_HOME = 0, ICON_AIM, ICON_EYE, ICON_GEAR, ICON_GLOBE, ICON_USERS, ICON_TREE, ICON_LUA, ICON_DOC };

static void DrawLogo(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
{
    if (aether::fonts::draw_icon(dl, aether::icons::codepoint(aether::icons::Id::Logo), c, s * 0.92f, col))
        return;

    AddArc(dl, c, s * 0.46f, -0.55f * kPi, 0.90f * kPi, col, 1.7f);
    AddArc(dl, ImVec2(c.x, c.y + s * 0.10f), s * 0.26f, 0.45f * kPi, 1.80f * kPi, col, 1.7f);
}

static bool DrawGlyphIcon(ImDrawList* dl, int icon, ImVec2 c, float s, ImU32 col)
{
    namespace ic = aether::icons;

    const float sz = s * 0.92f;

    switch (icon)
    {
    case ICON_HOME:  return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Home),      c, sz, col);
    case ICON_AIM:   return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Crosshair), c, sz, col);
    case ICON_EYE:   return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Eye),       c, sz, col);
    case ICON_GEAR:  return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Cog),       c, sz, col);
    case ICON_GLOBE: return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Globe),     c, sz, col);
    case ICON_USERS: return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Users),     c, sz, col);
    case ICON_TREE:  return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Tree),      c, sz, col);
    case ICON_LUA:   return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Code),      c, sz, col);
    default:         return aether::fonts::draw_icon(dl, ic::codepoint(ic::Id::Terminal),  c, sz, col);
    }
}

static void DrawTabIcon(ImDrawList* dl, int icon, ImVec2 c, float s, ImU32 col)
{
    if (DrawGlyphIcon(dl, icon, c, s, col))
        return;

    switch (icon)
    {
    case ICON_HOME:

        dl->AddLine(ImVec2(c.x - s * 0.44f, c.y - s * 0.02f), ImVec2(c.x, c.y - s * 0.44f), col, 1.6f);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.44f), ImVec2(c.x + s * 0.44f, c.y - s * 0.02f), col, 1.6f);
        dl->AddRect(ImVec2(c.x - s * 0.34f, c.y - s * 0.02f), ImVec2(c.x + s * 0.34f, c.y + s * 0.42f), col, 1.5f, 0, 1.5f);
        break;
    case ICON_AIM:
        dl->AddCircle(c, s * 0.44f, col, 24, 1.5f);
        dl->AddCircleFilled(c, s * 0.11f, col, 12);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.44f), ImVec2(c.x, c.y - s * 0.17f), col, 1.5f);
        dl->AddLine(ImVec2(c.x, c.y + s * 0.17f), ImVec2(c.x, c.y + s * 0.44f), col, 1.5f);
        dl->AddLine(ImVec2(c.x - s * 0.44f, c.y), ImVec2(c.x - s * 0.17f, c.y), col, 1.5f);
        dl->AddLine(ImVec2(c.x + s * 0.17f, c.y), ImVec2(c.x + s * 0.44f, c.y), col, 1.5f);
        break;
    case ICON_EYE:
        dl->PathClear();
        dl->PathLineTo(ImVec2(c.x - s * 0.50f, c.y));
        dl->PathBezierQuadraticCurveTo(ImVec2(c.x, c.y - s * 0.62f), ImVec2(c.x + s * 0.50f, c.y), 20);
        dl->PathBezierQuadraticCurveTo(ImVec2(c.x, c.y + s * 0.62f), ImVec2(c.x - s * 0.50f, c.y), 20);
        dl->PathStroke(col, 0, 1.4f);
        dl->AddCircleFilled(c, s * 0.13f, col, 16);
        break;
    case ICON_GEAR:
        dl->AddCircle(c, s * 0.34f, col, 20, 1.5f);
        for (int i = 0; i < 8; i++) {
            const float a = (float)i * kPi * 0.25f;
            dl->AddLine(ImVec2(c.x + cosf(a) * s * 0.40f, c.y + sinf(a) * s * 0.40f),
                        ImVec2(c.x + cosf(a) * s * 0.54f, c.y + sinf(a) * s * 0.54f), col, 1.9f);
        }
        break;
    case ICON_GLOBE:
        dl->AddCircle(c, s * 0.46f, col, 24, 1.4f);
        AddEllipseStroke(dl, c, s * 0.22f, s * 0.46f, col, 1.2f, 24);
        dl->AddLine(ImVec2(c.x - s * 0.46f, c.y), ImVec2(c.x + s * 0.46f, c.y), col, 1.2f);
        break;
    case ICON_USERS:
        dl->AddCircle(ImVec2(c.x - s * 0.17f, c.y - s * 0.17f), s * 0.17f, col, 16, 1.4f);
        AddArc(dl, ImVec2(c.x - s * 0.17f, c.y + s * 0.34f), s * 0.30f, kPi * 1.15f, kPi * 1.85f, col, 1.4f);
        dl->AddCircle(ImVec2(c.x + s * 0.28f, c.y - s * 0.08f), s * 0.13f, col, 16, 1.3f);
        AddArc(dl, ImVec2(c.x + s * 0.28f, c.y + s * 0.40f), s * 0.24f, kPi * 1.20f, kPi * 1.80f, col, 1.3f);
        break;
    case ICON_TREE:
        for (int i = 0; i < 3; i++) {
            const float y = c.y - s * 0.30f + (float)i * s * 0.30f;
            dl->AddRectFilled(ImVec2(c.x - s * 0.44f, y - s * 0.07f), ImVec2(c.x - s * 0.30f, y + s * 0.07f), col, 1.5f);
            dl->AddLine(ImVec2(c.x - s * 0.20f, y), ImVec2(c.x + (i == 0 ? s * 0.44f : s * 0.28f), y), col, 1.5f);
        }
        break;
    case ICON_LUA:

        dl->AddLine(ImVec2(c.x - s * 0.30f, c.y - s * 0.26f), ImVec2(c.x - s * 0.48f, c.y), col, 1.6f);
        dl->AddLine(ImVec2(c.x - s * 0.48f, c.y), ImVec2(c.x - s * 0.30f, c.y + s * 0.26f), col, 1.6f);
        dl->AddLine(ImVec2(c.x + s * 0.30f, c.y - s * 0.26f), ImVec2(c.x + s * 0.48f, c.y), col, 1.6f);
        dl->AddLine(ImVec2(c.x + s * 0.48f, c.y), ImVec2(c.x + s * 0.30f, c.y + s * 0.26f), col, 1.6f);
        break;
    case ICON_DOC:
    default:
        dl->AddRect(ImVec2(c.x - s * 0.38f, c.y - s * 0.46f), ImVec2(c.x + s * 0.38f, c.y + s * 0.46f), col, 2.5f, 0, 1.4f);
        dl->AddLine(ImVec2(c.x - s * 0.20f, c.y - s * 0.20f), ImVec2(c.x + s * 0.20f, c.y - s * 0.20f), col, 1.3f);
        dl->AddLine(ImVec2(c.x - s * 0.20f, c.y), ImVec2(c.x + s * 0.20f, c.y), col, 1.3f);
        dl->AddLine(ImVec2(c.x - s * 0.20f, c.y + s * 0.20f), ImVec2(c.x + s * 0.06f, c.y + s * 0.20f), col, 1.3f);
        break;
    }
}

enum { kTabHome = 0, kTabAim = 1, kTabVisuals = 2, kTabMisc = 3, kTabWorld = 4,
       kTabPlayers = 5, kTabExplorer = 6, kTabConfig = 7 };

static constexpr int kTabCount = 8;

static const char* kTabNames[] = { "Home", "Aim", "Visuals", "Misc", "World", "Players", "Explorer", "Config" };
static const int   kTabIconIds[] = { ICON_HOME, ICON_AIM, ICON_EYE, ICON_GEAR, ICON_GLOBE,
                                     ICON_USERS, ICON_TREE, ICON_DOC };

static const char* kTabSubtitles[] = {
    nullptr,
    "Camera lock, smoothing and hitbox selection.",
    "Player and world ESP.",
    "Movement & utility tweaks.",
    "World, lighting and environment.",
    "Player list and per-player settings.",
    "Instance hierarchy for the current place.",
    "Keybind, HUD and appearance.",
};

static bool ToggleControl(const char* tag, ImVec2 pos, bool* value)
{
    const ImVec2 size(K::ToggleW, K::ToggleH);
    ImGui::PushID(tag);
    ImGui::SetCursorScreenPos(pos);
    const bool pressed = ImGui::InvisibleButton("##hit", size);
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) *value = !*value;

    const Feedback fb = FeedbackUpdate(hovered, pressed);

    const float t  = SpringTo("##knob", *value ? 1.f : 0.f, 0.26f, 0.72f, *value ? 1.f : 0.f);
    const float hv = Approach(AnimF("##hover", 0.f), hovered ? 1.f : 0.f, K::UiAnimSpeed, ImGui::GetIO().DeltaTime);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = pos;
    const ImVec2 b = ImVec2(pos.x + size.x, pos.y + size.y);
    const float rad = size.y * 0.5f;

    AddGlow(dl, a, b, rad, fb.shine);
    AddRipple(dl, a, b, rad, fb.ripple);

    ImU32 track = Mix(g_T.toggleOff, g_T.toggleOn, Clampf(t, 0.f, 1.f));
    track = Mix(track, Mix(track, IM_COL32(255, 255, 255, 255), 0.14f), hv);

    dl->AddRectFilled(a, b, GC(track), rad);
    dl->AddRect(a, b, GC(Mix(g_T.windowBorder, g_T.controlBorderHi, Maxf(hv, fb.shine))), rad, 0, g_T.borderThickness);

    const float knobR = (size.y - 5.f) * 0.5f;
    const float kt = Clampf(t, 0.f, 1.f);
    const float knobX = Lerpf(a.x + 2.5f + knobR, b.x - 2.5f - knobR, kt);
    const ImVec2 kc(knobX, (a.y + b.y) * 0.5f);

    if (kt > 0.01f)
        dl->AddCircle(kc, knobR + 1.3f, GC(IM_COL32(255, 255, 255, (int)(86.f * kt))), 32, 2.4f);
    if (kt > 0.35f)
    {
        const float lit = Clampf((kt - 0.35f) / 0.65f, 0.f, 1.f);
        dl->AddCircleFilled(kc, knobR + 3.2f, GC(IM_COL32(255, 255, 255, (int)(24.f * lit))), 48);
        dl->AddCircle(kc, knobR + 2.1f, GC(IM_COL32(255, 255, 255, (int)(74.f * lit))), 48, 2.2f);
        dl->AddCircle(kc, knobR + 0.5f, GC(IM_COL32(8, 8, 10, (int)(78.f * lit))), 40, 1.4f);
    }
    dl->AddCircleFilled(kc, knobR, GC(Mix(g_T.knobOff, g_T.knobOn, kt)), 32);
    if (fb.shine > 0.01f)
        dl->AddCircle(kc, knobR + 1.7f, GC(g_T.glow, fb.shine * 0.30f), 32, 2.f);

    ImGui::PopID();
    return pressed;
}

static void PollKeybind(char* keyName, size_t n, bool* listening)
{
    if (!*listening) return;
    for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k++)
    {
        if (!ImGui::IsKeyPressed((ImGuiKey)k, false)) continue;
        if (k == ImGuiKey_Escape) snprintf(keyName, n, "None");
        else                      snprintf(keyName, n, "%s", ImGui::GetKeyName((ImGuiKey)k));
        *listening = false;
        break;
    }
}
static bool KeybindControl(const char* tag, ImVec2 pos, char* keyName, size_t n, bool* listening)
{
    const ImVec2 size(K::KeybindW, K::KeybindH);
    ImGui::PushID(tag);
    ImGui::SetCursorScreenPos(pos);
    const bool pressed = ImGui::InvisibleButton("##hit", size);
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) *listening = !*listening;

    Feedback fb = FeedbackUpdate(hovered, pressed, *listening);

    if (*listening)
    {
        const float pulse = 0.55f + 0.45f * (0.5f + 0.5f * sinf((float)ImGui::GetTime() * 6.5f));
        fb.shine = Maxf(fb.shine, pulse);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = pos, b = ImVec2(pos.x + size.x, pos.y + size.y);
    const float rad = g_T.roundControl;

    AddGlow(dl, a, b, rad, fb.shine, *listening ? 8.f : 5.f);
    AddRipple(dl, a, b, rad, fb.ripple);

    dl->AddRectFilled(a, b, GC(Mix(g_T.controlBg, IM_COL32(255, 255, 255, 40), Maxf(fb.shine, fb.hover) * 0.5f)), rad);
    ImU32 border = Mix(g_T.controlBorder, g_T.controlBorderHi, Maxf(fb.shine * 0.8f, fb.hover * 0.35f));
    dl->AddRect(a, b, GC(border), rad, 0, g_T.borderThickness);

    char label[48];
    if (*listening) snprintf(label, sizeof(label), "Listening...");
    else            snprintf(label, sizeof(label), "%s", (keyName && keyName[0]) ? keyName : "None");

    const ImVec2 ts = TSize(g_FontRegular, label);
    Txt(dl, g_FontRegular, ImVec2((a.x + b.x) * 0.5f - ts.x * 0.5f, (a.y + b.y) * 0.5f - ts.y * 0.5f),
        GC(Mix(g_T.textPrimary, g_T.textDim, *listening ? 0.35f : 0.f)), label);

    ImGui::PopID();
    return pressed;
}

static bool ComboControl(const char* tag, ImVec2 pos, int* current, const char* const* items, int count)
{
    const ImVec2 size(K::ComboW, K::ComboH);
    ImGui::PushID(tag);
    ImGui::SetCursorScreenPos(pos);
    const bool pressed = ImGui::InvisibleButton("##hit", size);
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) ImGui::OpenPopup("##popup");
    const bool open = ImGui::IsPopupOpen("##popup");

    const Feedback fb = FeedbackUpdate(hovered, pressed, open);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = pos, b = ImVec2(pos.x + size.x, pos.y + size.y);
    const float rad = g_T.roundControl;

    AddGlow(dl, a, b, rad, fb.shine);
    AddRipple(dl, a, b, rad, fb.ripple);

    dl->AddRectFilled(a, b, GC(Mix(g_T.controlBg, IM_COL32(255, 255, 255, 34), Maxf(fb.shine, fb.hover) * 0.55f)), rad);
    dl->AddRect(a, b, GC(Mix(g_T.controlBorder, g_T.controlBorderHi, Maxf(fb.shine, fb.hover) * 0.85f)), rad, 0, g_T.borderThickness);

    const char* label = (*current >= 0 && *current < count) ? items[*current] : "";
    Txt(dl, g_FontRegular, ImVec2(a.x + 12.f, (a.y + b.y) * 0.5f - g_FontRegular->FontSize * 0.5f), GC(g_T.textPrimary), label);
    AddChevron(dl, ImVec2(b.x - 16.f, (a.y + b.y) * 0.5f), 4.5f, 2.8f, GC(g_T.textPrimary), 2.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, g_T.roundPopup);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, g_T.borderThickness);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_T.popupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, g_T.windowBorder);

    ImGui::SetNextWindowPos(ImVec2(b.x - (K::ComboW + 14.f), b.y + 6.f), ImGuiCond_Always);
    if (ImGui::BeginPopup("##popup"))
    {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        {
            const ImVec2 pw = ImGui::GetWindowPos();
            const ImVec2 ps = ImGui::GetWindowSize();
            DrawBackdropRect(pdl, pw, ImVec2(pw.x + ps.x, pw.y + ps.y), g_T.roundPopup, g_MenuFade * 255.f);
        }
        for (int i = 0; i < count; i++)
        {
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const ImVec2 isz(K::ComboW, 26.f);
            ImGui::InvisibleButton("##item", isz);
            const bool ih = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) { *current = i; ImGui::CloseCurrentPopup(); }
            if (ih)
            {
                pdl->AddRectFilled(p, ImVec2(p.x + isz.x, p.y + isz.y), GC(g_T.rowHover), 7.f);
                pdl->AddRect(p, ImVec2(p.x + isz.x, p.y + isz.y), GC(g_T.controlBorder), 7.f, 0, 1.f);
            }
            Txt(pdl, g_FontRegular, ImVec2(p.x + 10.f, p.y + (isz.y - g_FontRegular->FontSize) * 0.5f),
                GC(i == *current ? g_T.textPrimary : g_T.textDim), items[i]);
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    ImGui::PopID();
    return pressed;
}

static float SliderTrackH()
{
    return settings::misc_ui::slider_hd
        ? Maxf(2.f, K::SliderTrackH * Clampf(settings::misc_ui::slider_hd_size, 0.5f, 2.f))
        : K::SliderTrackH;
}

static float SliderThumbD()
{
    return settings::misc_ui::slider_hd
        ? K::SliderThumbD * Clampf(settings::misc_ui::slider_hd_size, 0.5f, 2.f)
        : K::SliderThumbD;
}

static float SliderOverhang()
{
    return settings::misc_ui::slider_hd
        ? Maxf(0.f, (SliderThumbD() - SliderTrackH()) * 0.5f)
        : 0.f;
}

static bool SliderControl(const char* tag, ImVec2 pos, float width, float* value, float vmin, float vmax)
{
    const bool  hd     = settings::misc_ui::slider_hd;
    const float trackH = SliderTrackH();
    const float trackY = pos.y;
    const ImVec2 hitMin(pos.x, trackY - 8.f - SliderOverhang() * 0.5f);
    const ImVec2 hitMax(pos.x + width, trackY + trackH + 8.f + SliderOverhang() * 0.5f);

    ImGui::PushID(tag);
    ImGui::SetCursorScreenPos(hitMin);
    ImGui::InvisibleButton("##hit", ImVec2(hitMax.x - hitMin.x, hitMax.y - hitMin.y));
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    if (active && width > 0.f) {
        const float t = Clampf((ImGui::GetIO().MousePos.x - pos.x) / width, 0.f, 1.f);
        *value = vmin + t * (vmax - vmin);
    }

    const Feedback fb = FeedbackUpdate(hovered, false, active);

    const float target = Clampf((*value - vmin) / (vmax - vmin), 0.f, 1.f);
    const float t = Clampf(SpringTo("##fill", target, 0.24f, 0.85f, target), 0.f, 1.f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float rad = trackH * 0.5f;
    const float thumbR = SliderThumbD() * 0.5f + Maxf(fb.shine, fb.hover) * (hd ? 1.9f : 1.6f);
    const float thumbX = pos.x + width * t;
    const float cy = trackY + rad;

    const int segs = hd ? 48 : 32;
    dl->AddRectFilled(ImVec2(pos.x, trackY), ImVec2(pos.x + width, trackY + trackH),
                      GC(g_T.sliderTrack), rad);
    if (t > 0.004f)
        dl->AddRectFilled(ImVec2(pos.x, trackY), ImVec2(thumbX, trackY + trackH),
                          GC(g_T.sliderFill), rad);

    const ImVec2 tc(thumbX, cy);
    AddGlow(dl, ImVec2(tc.x - thumbR, tc.y - thumbR), ImVec2(tc.x + thumbR, tc.y + thumbR), thumbR, fb.shine, 7.f);
    dl->AddCircleFilled(tc, thumbR, GC(g_T.sliderThumb), segs);
    dl->AddCircle(tc, thumbR, GC(IM_COL32(18, 18, 20, 70)), segs, 1.f);

    if (fb.shine > 0.01f)
        dl->AddCircle(tc, thumbR + 1.4f, GC(g_T.glow, fb.shine * 0.35f), hd ? 48 : 32, 1.6f);

    ImGui::PopID();
    return active;
}

static void SwatchControl(const char* tag, ImVec2 center, ImU32* col)
{
    static const ImU32 kPresets[] = {
        IM_COL32(244, 244, 246, 255), IM_COL32(232,  92,  92, 255), IM_COL32(247, 173,  74, 255),
        IM_COL32(238, 219, 104, 255), IM_COL32(110, 210, 130, 255), IM_COL32( 96, 176, 240, 255),
        IM_COL32(158, 132, 240, 255), IM_COL32(240, 132, 200, 255),
    };
    const float r = K::SwatchD * 0.5f;
    const ImVec2 a(center.x - r, center.y - r);
    const ImVec2 b(center.x + r, center.y + r);

    ImGui::PushID(tag);
    ImGui::SetCursorScreenPos(a);
    const bool pressed = ImGui::InvisibleButton("##sw", ImVec2(K::SwatchD, K::SwatchD));
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) ImGui::OpenPopup("##picker");

    const Feedback fb = FeedbackUpdate(hovered, pressed);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    AddGlow(dl, a, b, r, fb.shine, 7.f);
    AddRipple(dl, a, b, r, fb.ripple);

    dl->AddCircleFilled(center, r + Maxf(fb.shine, fb.hover), GC(*col), 48);
    dl->AddCircle(center, r + Maxf(fb.shine, fb.hover), GC(Mix(g_T.windowBorder, g_T.controlBorderHi, Maxf(fb.shine, fb.hover))), 48, 1.2f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, g_T.roundPopup);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_T.popupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, g_T.windowBorder);

    ImGui::SetNextWindowPos(ImVec2(b.x - 160.f, b.y + 7.f), ImGuiCond_Always);
    if (ImGui::BeginPopup("##picker"))
    {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        const float cr = 13.f;
        for (int i = 0; i < IM_ARRAYSIZE(kPresets); i++)
        {
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##p", ImVec2(30.f, 30.f));
            const bool ih = ImGui::IsItemHovered();
            const ImVec2 c(p.x + 15.f, p.y + 15.f);
            if (ih) pdl->AddCircleFilled(c, cr + 3.f, GC(g_T.glow, 0.16f), 32);
            pdl->AddCircleFilled(c, cr, GC(kPresets[i]), 32);
            if (kPresets[i] == *col) pdl->AddCircle(c, cr + 2.f, GC(g_T.glow, 0.9f), 32, 1.6f);
            if (ImGui::IsItemClicked()) { *col = kPresets[i]; ImGui::CloseCurrentPopup(); }
            if ((i % 4) != 3) ImGui::SameLine(0.f, 8.f);
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
}

static float g_RowY = 0.f;
static float g_ContentL = 0.f;
static float g_ContentR = 0.f;
static float g_ContentTop = 0.f;

static float g_ContentH   = 0.f;
static float g_ContentBottom = 0.f;

static float g_ScrollShown = 0.f;
static float g_ScrollTarget = 0.f;
static const float kScrollStep     = 78.f;
static const float kScrollResponse = 0.22f;
static const float kScrollDamping  = 1.0f;
static const ImGuiID kScrollSpringId = 0x5C0110ADu;

static float RowHover(const char* tag, ImVec2 mn, ImVec2 mx)
{
    const float dt = ImGui::GetIO().DeltaTime;
    float& h = AnimF(tag, 0.f);
    h = Approach(h, ImGui::IsMouseHoveringRect(mn, mx) ? 1.f : 0.f, 16.f, dt);
    if (h > 0.003f)
        ImGui::GetWindowDrawList()->AddRectFilled(mn, mx, GC(g_T.rowHover, h), g_T.roundRow);
    return h;
}

struct Toast { char text[44]; float t; bool on; };
static Toast g_Toasts[4] = {};
static int   g_ToastCount = 0;

static const float kToastH   = 30.f;
static const float kToastGap = 7.f;

struct ToastLayout { float x0, x1, y0, y1; bool onRight; };
static ToastLayout ToastColumn()
{
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    float colW = 130.f;
    for (int i = 0; i < g_ToastCount; i++)
        colW = Maxf(colW, TSize(g_FontRegular, g_Toasts[i].text).x + 32.f);
    const float stackH = Maxf(kToastH, Maxf((float)g_ToastCount, 1.f) * (kToastH + kToastGap) - kToastGap);

    float x0  = g_CardPos.x + K::CardW + 12.f;
    bool  right = true;
    if (x0 + colW > ds.x - 8.f) { x0 = g_CardPos.x - 12.f - colW; right = false; }
    x0 = Clampf(x0, 4.f, Maxf(4.f, ds.x - colW - 4.f));
    const float y0 = Clampf(g_CardPos.y, 4.f, Maxf(4.f, ds.y - stackH - 4.f));

    ToastLayout l;
    l.x0 = x0; l.x1 = x0 + colW; l.y0 = y0; l.y1 = y0 + stackH; l.onRight = right;
    return l;
}

static void PushToast(const char* text, bool on)
{
    if (g_ToastCount < 4) ++g_ToastCount;
    for (int i = g_ToastCount - 1; i > 0; --i) g_Toasts[i] = g_Toasts[i - 1];
    snprintf(g_Toasts[0].text, sizeof(g_Toasts[0].text), "%s", text);
    g_Toasts[0].t  = 0.f;
    g_Toasts[0].on = on;
}

static void DrawToasts(ImDrawList* dl, ImVec2 wp)
{
    (void)wp;
    const float dt = ImGui::GetIO().DeltaTime;
    const float life = 2.4f;
    int live = 0;
    const ToastLayout col = ToastColumn();
    for (int i = 0; i < g_ToastCount; i++)
    {
        g_Toasts[i].t += dt;
        if (g_Toasts[i].t > life) continue;
        const float in   = Clampf(g_Toasts[i].t / 0.20f, 0.f, 1.f);
        const float out  = Clampf((life - g_Toasts[i].t) / 0.40f, 0.f, 1.f);
        const float a    = EaseOutCubic(in) * out;
        const float drop = (1.f - EaseOutCubic(in)) * 10.f;

        const ImVec2 ts = TSize(g_FontRegular, g_Toasts[i].text);
        const float w = ts.x + 32.f, h = kToastH;

        const float px = col.onRight ? col.x0 : (col.x1 - w);
        const ImVec2 p0(px, col.y0 + (float)live * (h + kToastGap) - drop);
        const ImVec2 p1(p0.x + w, p0.y + h);

        DrawBackdropRect(dl, p0, p1, 15.f, a * 255.f);
        dl->AddRectFilled(p0, p1, GC(g_T.windowBg, a), 15.f);
        dl->AddRect(p0, p1, GC(g_T.windowBorder, a), 15.f, 0, g_T.borderThickness);

        Txt(dl, g_FontRegular, ImVec2(p0.x + 16.f, p0.y + (h - g_FontRegular->FontSize) * 0.5f),
            GC(IM_COL32(232, 232, 236, (int)(255.f * a))), g_Toasts[i].text);
        ++live;
    }

    int keep = 0;
    for (int i = 0; i < g_ToastCount; i++)
        if (g_Toasts[i].t <= life) g_Toasts[keep++] = g_Toasts[i];
    g_ToastCount = keep;
}

static bool RowToggle(const char* id, const char* label, bool* value, const char* desc = nullptr)
{
    ImGui::PushID(id);
    const float nameH = (g_FontBold ? g_FontBold : g_FontRegular)->FontSize;
    const float textH = nameH + (desc ? 3.f + g_FontSmall->FontSize : 0.f);
    const float rowH  = Maxf(textH, K::ToggleH) + 2.f * K::RowPad;
    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    const ImVec2 tpos(g_ContentL, g_RowY + (rowH - K::ToggleH) * 0.5f);
    const bool before = *value;
    const bool pressed = ToggleControl("tgl", tpos, value);
    if (*value != before) PushToast(label, *value);

    const float ty = g_RowY + (rowH - textH) * 0.5f;
    Txt(ImGui::GetWindowDrawList(), g_FontRegular,
        ImVec2(tpos.x + K::ToggleW + 12.f, ty + (nameH - g_FontRegular->FontSize) * 0.5f),
        GC(g_T.textPrimary), label);
    if (desc)
        Txt(ImGui::GetWindowDrawList(), g_FontSmall,
            ImVec2(tpos.x + K::ToggleW + 12.f, ty + nameH + 3.f), GC(g_T.textDim), desc);

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
    return pressed;
}

static void RowCombo(const char* id, const char* name, const char* desc, int* current,
                     const char* const* items, int count)
{
    ImGui::PushID(id);
    const float nameH = (g_FontBold ? g_FontBold : g_FontRegular)->FontSize;
    const float descH = g_FontSmall->FontSize;
    const float textH = nameH + (desc ? 3.f + descH : 0.f);
    const float rowH  = Maxf(textH, K::ComboH) + 2.f * K::RowPad;

    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ty = g_RowY + (rowH - textH) * 0.5f;
    TxtBold(dl, name, ImVec2(g_ContentL, ty), GC(g_T.textPrimary));
    if (desc) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + nameH + 3.f), GC(g_T.textDim), desc);

    ComboControl("combo", ImVec2(g_ContentR - K::ComboW, g_RowY + (rowH - K::ComboH) * 0.5f), current, items, count);

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
}

static void RowSlider(const char* id, const char* name, float* value, float vmin, float vmax,
                      const char* valueText, bool swatch, ImU32* swatchCol)
{
    ImGui::PushID(id);
    const float nameH  = (g_FontBold ? g_FontBold : g_FontRegular)->FontSize;

    const float over   = SliderOverhang();
    const float rowH   = nameH + 11.f + SliderTrackH() + 2.f * over + 2.f * K::RowPad;

    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ty = g_RowY + K::RowPad;
    TxtBold(dl, name, ImVec2(g_ContentL, ty), GC(g_T.textPrimary));

    float valueRight = g_ContentR;
    if (swatch && swatchCol)
    {
        const float cx = g_ContentR - K::SwatchD * 0.5f;
        SwatchControl("swatch", ImVec2(cx, ty + nameH * 0.5f + 1.f), swatchCol);
        valueRight = cx - K::SwatchD * 0.5f - 9.f;
    }

    SliderControl("sld", ImVec2(g_ContentL, ty + nameH + 11.f + over), g_ContentR - g_ContentL, value, vmin, vmax);

    if (valueText)
        TxtRight(dl, g_FontRegular, valueRight, ty + (nameH - g_FontRegular->FontSize) * 0.5f, GC(g_T.textFaint), valueText);

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
}

static bool KeybindControlVk(const char* tag, ImVec2 pos, ImVec2 size, int* key);

static void PageHeader(const char* title, const char* subtitle, int* keybind)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* tf = g_FontTitle ? g_FontTitle : g_FontRegular;
    const float titleH = tf->FontSize;
    const float subH   = g_FontSmall->FontSize;
    const float blockH = titleH + (subtitle ? 5.f + subH : 0.f);
    const ImVec2 ksize(K::KeybindW, K::KeybindH);
    const float rowH   = Maxf(blockH, keybind ? ksize.y : 0.f);

    const float top = g_RowY;
    const float ty = top + (rowH - blockH) * 0.5f;
    TxtBold(dl, title, ImVec2(g_ContentL, ty), GC(g_T.textPrimary), true);
    if (subtitle) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + titleH + 5.f), GC(g_T.textDim), subtitle);

    if (keybind)
        KeybindControlVk("pagekey", ImVec2(g_ContentR - ksize.x, top + (rowH - ksize.y) * 0.5f), ksize, keybind);

    g_RowY += rowH + 12.f;
}

static void DimNote(const char* text)
{
    Txt(ImGui::GetWindowDrawList(), g_FontSmall, ImVec2(g_ContentL, g_RowY), GC(g_T.textDim), text);
    g_RowY += g_FontSmall->FontSize + 10.f;
}

static void TxtTracked(ImDrawList* dl, ImFont* f, ImVec2 p, ImU32 col, const char* t, float extra)
{
    float x = p.x;
    for (const char* c = t; *c; ++c)
    {
        char one[2] = { *c, 0 };
        Txt(dl, f, ImVec2(x, p.y), col, one);
        x += TSize(f, one).x + extra;
    }
}

static void CardCaption(ImDrawList* dl, float x, float y, const char* t)
{
    TxtTracked(dl, g_FontSmall, ImVec2(x, y), GC(g_T.textDim), t, 0.7f);
}

static void SubTabStrip(const char* const* items, int n, int* current)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 27.f, padX = 13.f, gap = 3.f;
    const float dt = ImGui::GetIO().DeltaTime;
    const float y0 = g_RowY + 4.f;

    float px[10] = {}, pw[10] = {};
    float x = g_ContentL;
    for (int i = 0; i < n && i < 10; i++)
    {
        const float w = TSize(g_FontRegular, items[i]).x + padX * 2.f;
        px[i] = x; pw[i] = w;
        ImGui::PushID(900 + i);
        ImGui::SetCursorScreenPos(ImVec2(x, y0));
        const bool hit = ImGui::InvisibleButton("##sub", ImVec2(w, h));
        const bool hov = ImGui::IsItemHovered();
        float& hv = AnimF("##subhov", 0.f);
        hv = Approach(hv, (hov && i != *current) ? 1.f : 0.f, K::UiAnimSpeed, dt);
        ImGui::PopID();
        if (hv > 0.003f)
            dl->AddRectFilled(ImVec2(x, y0), ImVec2(x + w, y0 + h), GC(IM_COL32(255, 255, 255, 12), hv), 8.f);
        if (hit && i != *current)
        {
            *current = i;
            g_ScrollTarget = 0.f;
            g_ScrollShown  = 0.f;
            PushToast(items[i], true);
        }
        x += w + gap;
    }

    const int sel = (*current >= 0 && *current < n) ? *current : 0;
    const float pilX = SpringTo("##subx", px[sel], 0.30f, 0.80f, px[sel]);
    const float pilW = SpringTo("##subw", pw[sel], 0.30f, 0.80f, pw[sel]);
    dl->AddRectFilled(ImVec2(pilX, y0), ImVec2(pilX + pilW, y0 + h), GC(IM_COL32(255, 255, 255, 20)), 8.f);
    dl->AddRect(ImVec2(pilX, y0), ImVec2(pilX + pilW, y0 + h), GC(IM_COL32(255, 255, 255, 28)), 8.f, 0, g_T.borderThickness);

    for (int i = 0; i < n && i < 10; i++)
        Txt(dl, g_FontRegular, ImVec2(px[i] + padX, y0 + (h - g_FontRegular->FontSize) * 0.5f),
            GC(i == sel ? g_T.textPrimary : g_T.textDim), items[i]);

    const float ly = y0 + h + 13.f;
    dl->AddLine(ImVec2(g_ContentL, ly), ImVec2(g_ContentR + 2.f, ly), GC(g_T.hairline), g_T.dividerThickness);
    g_RowY = ly + 15.f;
}

static int g_SubTab[kTabCount] = {};

static void CardBox(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding = 12.f,
                    float fillA = 7.f, float borderA = 22.f)
{
    dl->AddRectFilled(mn, mx, GC(IM_COL32(255, 255, 255, (int)fillA)), rounding);
    dl->AddRect(mn, mx, GC(IM_COL32(255, 255, 255, (int)borderA)), rounding, 0, g_T.borderThickness);
}

static void WellBox(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding = 10.f, float a = 128.f)
{
    dl->AddRectFilled(mn, mx, GC(IM_COL32(9, 9, 11, (int)a)), rounding);
    dl->AddRect(mn, mx, GC(IM_COL32(255, 255, 255, 16)), rounding, 0, g_T.borderThickness);
}

static void DrawGlyph(ImDrawList* dl, const char* g, ImVec2 c, float s, ImU32 col)
{
    if (!g || !g[0]) return;
    if      (g[0] == '+' && !g[1]) {
        dl->AddLine(ImVec2(c.x - s * 0.42f, c.y), ImVec2(c.x + s * 0.42f, c.y), col, 1.5f);
        dl->AddLine(ImVec2(c.x, c.y - s * 0.42f), ImVec2(c.x, c.y + s * 0.42f), col, 1.5f); }
    else if (g[0] == 'x' && !g[1]) {
        dl->AddLine(ImVec2(c.x - s * 0.36f, c.y - s * 0.36f), ImVec2(c.x + s * 0.36f, c.y + s * 0.36f), col, 1.4f);
        dl->AddLine(ImVec2(c.x + s * 0.36f, c.y - s * 0.36f), ImVec2(c.x - s * 0.36f, c.y + s * 0.36f), col, 1.4f); }
    else if (!strcmp(g, "play")) {
        dl->AddTriangleFilled(ImVec2(c.x - s * 0.26f, c.y - s * 0.42f), ImVec2(c.x - s * 0.26f, c.y + s * 0.42f),
                              ImVec2(c.x + s * 0.44f, c.y), col); }
    else if (!strcmp(g, "copy")) {
        dl->AddRect(ImVec2(c.x - s * 0.46f, c.y - s * 0.46f), ImVec2(c.x + s * 0.12f, c.y + s * 0.12f), col, 2.f, 0, 1.3f);
        dl->AddRect(ImVec2(c.x - s * 0.12f, c.y - s * 0.12f), ImVec2(c.x + s * 0.46f, c.y + s * 0.46f), col, 2.f, 0, 1.3f); }
    else if (!strcmp(g, "clear")) {
        dl->AddLine(ImVec2(c.x - s * 0.40f, c.y - s * 0.34f), ImVec2(c.x + s * 0.40f, c.y - s * 0.34f), col, 1.4f);
        dl->AddRect(ImVec2(c.x - s * 0.30f, c.y - s * 0.26f), ImVec2(c.x + s * 0.30f, c.y + s * 0.44f), col, 2.f, 0, 1.3f);
        dl->AddLine(ImVec2(c.x - s * 0.14f, c.y - s * 0.06f), ImVec2(c.x - s * 0.14f, c.y + 0.24f), col, 1.2f);
        dl->AddLine(ImVec2(c.x + s * 0.14f, c.y - s * 0.06f), ImVec2(c.x + s * 0.14f, c.y + 0.24f), col, 1.2f); }
    else if (!strcmp(g, "search")) {
        dl->AddCircle(ImVec2(c.x - s * 0.12f, c.y - s * 0.12f), s * 0.34f, col, 20, 1.4f);
        dl->AddLine(ImVec2(c.x + s * 0.12f, c.y + s * 0.12f), ImVec2(c.x + s * 0.46f, c.y + s * 0.46f), col, 1.5f); }
    else if (!strcmp(g, "chevron")) {
        AddChevron(dl, c, s * 0.40f, s * 0.24f, col, 1.6f); }
    else if (!strcmp(g, "swap")) {
        dl->AddLine(ImVec2(c.x - s * 0.40f, c.y - s * 0.18f), ImVec2(c.x + s * 0.40f, c.y - s * 0.18f), col, 1.3f);
        dl->AddLine(ImVec2(c.x - s * 0.40f, c.y + 0.18f), ImVec2(c.x + s * 0.40f, c.y + 0.18f), col, 1.3f); }
}

static bool ButtonControl(const char* id, ImVec2 pos, ImVec2 size, const char* label,
                          bool primary = false, const char* glyph = nullptr)
{
    ImGui::PushID(id);
    ImGui::SetCursorScreenPos(pos);
    const bool pressed = ImGui::InvisibleButton("##b", size);
    const bool hovered = ImGui::IsItemHovered();
    const Feedback fb  = FeedbackUpdate(hovered, pressed);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = pos, b(pos.x + size.x, pos.y + size.y);
    const float rad = size.y * 0.5f;
    const float gs  = 12.f;
    const float tw  = label ? TSize(g_FontRegular, label).x : 0.f;
    const float total = tw + (glyph ? (label ? gs + 8.f : gs) : 0.f);
    const float x  = (a.x + b.x) * 0.5f - total * 0.5f;
    const ImU32 fg = primary ? IM_COL32(22, 22, 26, 255) : g_T.textPrimary;

    AddGlow(dl, a, b, rad, fb.shine, 5.f);
    AddRipple(dl, a, b, rad, fb.ripple);
    dl->AddRectFilled(a, b, GC(primary ? Mix(IM_COL32(226, 226, 230, 238), IM_COL32(255, 255, 255, 255), Maxf(fb.shine, fb.hover))
                                      : Mix(g_T.controlBg, IM_COL32(255, 255, 255, 42), Maxf(fb.shine, fb.hover) * 0.6f)), rad);
    dl->AddRect(a, b, GC(Mix(primary ? IM_COL32(255, 255, 255, 70) : g_T.controlBorder,
                             g_T.controlBorderHi, Maxf(fb.shine, fb.hover))), rad, 0, g_T.borderThickness);

    if (glyph) DrawGlyph(dl, glyph, ImVec2(x + gs * 0.5f, (a.y + b.y) * 0.5f), gs, GC(fg));
    if (label) Txt(dl, g_FontRegular,
                   ImVec2(x + (glyph ? gs + 8.f : 0.f), (a.y + b.y) * 0.5f - g_FontRegular->FontSize * 0.5f),
                   GC(fg), label);
    ImGui::PopID();
    return pressed;
}

static bool SearchField(const char* id, ImVec2 pos, float w, float h, char* buf, size_t n, const char* hint)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = pos, b(pos.x + w, pos.y + h);
    const float rad = h * 0.5f;

    float& hv = AnimF("##searchhov", 0.f);
    dl->AddRectFilled(a, b, GC(Mix(g_T.controlBg, IM_COL32(255, 255, 255, 38), hv * 0.5f)), rad);
    dl->AddRect(a, b, GC(Mix(g_T.controlBorder, g_T.controlBorderHi, hv * 0.7f)), rad, 0, g_T.borderThickness);
    DrawGlyph(dl, "search", ImVec2(a.x + 15.f, (a.y + b.y) * 0.5f), 13.f, GC(g_T.textFaint));

    ImGui::PushID(id);
    ImGui::SetCursorScreenPos(ImVec2(a.x + 30.f, (a.y + b.y) * 0.5f - g_FontRegular->FontSize * 0.5f - 4.f));
    ImGui::SetNextItemWidth(w - 42.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 4.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(g_T.textFaint));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(1, 1, 1, 0.16f));
    const bool changed = ImGui::InputTextWithHint("##q", hint, buf, n);
    const bool hovered = ImGui::IsItemHovered();
    const bool active  = ImGui::IsItemActive();
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
    hv = Approach(hv, (hovered || active) ? 1.f : 0.f, K::UiAnimSpeed, ImGui::GetIO().DeltaTime);
    ImGui::PopID();
    return changed;
}

static void KVRow(ImDrawList* dl, float x0, float x1, float y, float h,
                  const char* k, const char* v, ImU32 vcol)
{
    const float ty = y + (h - g_FontRegular->FontSize) * 0.5f;
    Txt(dl, g_FontRegular, ImVec2(x0, ty), GC(g_T.textDim), k);
    TxtRight(dl, g_FontRegular, x1, ty, GC(vcol), v);
    dl->AddLine(ImVec2(x0, y + h), ImVec2(x1, y + h), GC(IM_COL32(255, 255, 255, 12)), 1.f);
}

static bool RowSwatch(const char* id, const char* name, const char* desc, ImU32* col)
{
    ImGui::PushID(id);
    const float nameH = (g_FontBold ? g_FontBold : g_FontRegular)->FontSize;
    const float textH = nameH + (desc ? 3.f + g_FontSmall->FontSize : 0.f);
    const float rowH  = Maxf(textH, K::SwatchD) + 2.f * K::RowPad;
    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ty = g_RowY + (rowH - textH) * 0.5f;
    TxtBold(dl, name, ImVec2(g_ContentL, ty), GC(g_T.textPrimary));
    if (desc) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + nameH + 3.f), GC(g_T.textDim), desc);
    SwatchControl("sw", ImVec2(g_ContentR - K::SwatchD * 0.5f, g_RowY + rowH * 0.5f), col);

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
    return false;
}

static void PlayerAvatar(ImDrawList* dl, ImVec2 c, float r, ImU32 tint)
{
    dl->AddCircleFilled(c, r, GC(tint), 32);
    dl->AddCircle(c, r, GC(IM_COL32(255, 255, 255, 26)), 32, 1.f);
    dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.24f), r * 0.32f, GC(IM_COL32(10, 10, 12, 96)), 20);
    AddArc(dl, ImVec2(c.x, c.y + r * 0.62f), r * 0.52f, kPi * 1.10f, kPi * 1.90f, GC(IM_COL32(10, 10, 12, 96)), r * 0.30f);
}

static ImU32 HealthColor(int hp)
{
    if (hp >= 100) return IM_COL32(126, 214, 132, 255);
    if (hp >= 50)  return IM_COL32(226, 196, 108, 255);
    return IM_COL32(226, 104, 104, 255);
}

static char LowerC(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static bool IContains(const char* hay, const char* needle)
{
    if (!needle || !needle[0]) return true;
    for (const char* h = hay; *h; ++h)
    {
        const char* a = h; const char* b = needle;
        while (*a && *b && LowerC(*a) == LowerC(*b)) { ++a; ++b; }
        if (!*b) return true;
    }
    return false;
}


#include "SyncPages.h"

#include "Anim.h"
#include "Docks.h"

static int   g_ActiveTab = 0;
static float g_PillY = -1.f;
static float g_TabFade = 1.f;
static float g_TabSlideX = 0.f;

static void SetActiveTab(int i)
{
    if (i == g_ActiveTab) return;
    PushToast(kTabNames[i], true);
    g_TabSlideX = -12.f;
    g_TabFade = 0.f;

    g_ScrollTarget = 0.f;
    g_ScrollShown  = 0.f;
    g_ActiveTab = i;
    if (i == kTabVisuals)
        SpringSetId(kPreviewSpringId, kPreviewClosedScale, 0.f);
}

static void DrawSidebar(ImDrawList* dl, ImVec2 wp, float top, float bottom)
{
    const float x0 = wp.x + K::Pad;
    const float x1 = x0 + K::SidebarW;

    if (((g_T.sidebarBg >> IM_COL32_A_SHIFT) & 0xFF) != 0)
        dl->AddRectFilled(ImVec2(x0, top), ImVec2(x1, bottom), GC(g_T.sidebarBg), g_T.roundPill);
    dl->AddLine(ImVec2(x1 + K::DividerGap, top + 4.f), ImVec2(x1 + K::DividerGap, bottom - 4.f),
                GC(g_T.divider), g_T.dividerThickness);

    const float pillX0 = x0 + K::TabInset;
    const float pillW  = K::SidebarW - K::TabInset * 2.f;
    const float firstY = top + 5.f;
    const float pitch  = K::TabH + K::TabGap;
    const float dt     = ImGui::GetIO().DeltaTime;

    const float targetY = firstY + (float)g_ActiveTab * pitch;
    if (g_PillY < 0.f) g_PillY = targetY;
    g_PillY = SpringTo("##pill", targetY, 0.30f, 0.80f, targetY);

    const ImVec2 pa(pillX0, g_PillY), pb(pillX0 + pillW, g_PillY + K::TabH);
    AddGlow(dl, pa, pb, g_T.roundPill, 0.30f, 6.f);
    dl->AddRectFilled(pa, pb, GC(g_T.pillActive), g_T.roundPill);

    float y = firstY;
    for (int i = 0; i < kTabCount; i++)
    {
        const ImVec2 mn(pillX0, y), mx(pillX0 + pillW, y + K::TabH);
        const bool isActive = (i == g_ActiveTab);
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(mn);
        if (ImGui::InvisibleButton("tab", ImVec2(pillW, K::TabH))) SetActiveTab(i);
        const bool hov = ImGui::IsItemHovered();
        const Feedback fb = FeedbackUpdate(hov, false, isActive);
        float& hv  = AnimF("##hover",  isActive ? 1.f : 0.f);
        float& act = AnimF("##active", isActive ? 1.f : 0.f);
        hv  = Approach(hv,  (hov && !isActive) ? 1.f : 0.f, K::UiAnimSpeed, dt);
        act = Approach(act, isActive ? 1.f : 0.f, K::UiAnimSpeed, dt);
        const float glow = fb.shine;
        ImGui::PopID();

        if (hv > 0.003f)
            dl->AddRectFilled(mn, mx, GC(g_T.rowHover, hv), g_T.roundPill);
        AddGlow(dl, ImVec2(mn.x + 3.f, mn.y + 3.f), ImVec2(mx.x - 3.f, mx.y - 3.f), g_T.roundPill, glow * 0.55f, 5.f);

        const ImU32 col = Mix(Mix(g_T.textDim, g_T.textPrimary, hv), g_T.textOnPill, act);
        DrawTabIcon(dl, kTabIconIds[i], ImVec2(mn.x + 21.f, (mn.y + mx.y) * 0.5f), 16.f, GC(col));
        Txt(dl, g_FontRegular, ImVec2(mn.x + 39.f, (mn.y + mx.y) * 0.5f - g_FontRegular->FontSize * 0.5f),
            GC(col), kTabNames[i]);

        y += pitch;
    }

    const float nameH  = (g_FontBold ? g_FontBold : g_FontRegular)->FontSize;
    const float smallH = g_FontSmall->FontSize;
    const float blockH = nameH + 3.f + smallH;
    float fy = bottom - 14.f - (smallH + 9.f + blockH);
    Txt(dl, g_FontSmall, ImVec2(x0 + 10.f, fy), GC(g_T.textDim), "Welcome back");
    fy += smallH + 9.f;

    const ImVec2 ac(x0 + 10.f + K::AvatarD * 0.5f, fy + blockH * 0.5f);
    const float  ar = K::AvatarD * 0.5f;

    bool have_face = false;
    if (sync::hooks().player_avatar_rounded)
    {
        sync::SessionInfo me_sn{};
        if (sync::hooks().session && sync::hooks().session(me_sn) && me_sn.local_user_id > 0)
        {
            have_face = sync::hooks().player_avatar_rounded(dl, me_sn.local_user_id,
                ImVec2(ac.x - ar, ac.y - ar), ImVec2(ac.x + ar, ac.y + ar),
                GC(IM_COL32(255, 255, 255, 255)), ar);
        }
    }
    if (!have_face)
    {
        dl->AddCircleFilled(ac, ar, GC(IM_COL32(255, 255, 255, 20)), 32);
        dl->AddCircleFilled(ImVec2(ac.x, ac.y - 4.f), 4.f, GC(g_T.textDim), 16);
        AddArc(dl, ImVec2(ac.x, ac.y + 8.5f), 7.5f, kPi * 1.15f, kPi * 1.85f, GC(g_T.textDim), 1.9f);
    }
    dl->AddCircle(ac, ar, GC(g_T.windowBorder), 40, 1.f);

    const float nx = x0 + 10.f + K::AvatarD + 10.f;

    char me[64] = "not attached";
    char handle[64] = "no game";
    if (sync::hooks().local_player)
        sync::hooks().local_player(me, sizeof me, handle, sizeof handle);
    TxtBold(dl, me, ImVec2(nx, fy), GC(g_T.textPrimary));
    Txt(dl, g_FontSmall, ImVec2(nx, fy + nameH + 3.f), GC(g_T.textFaint), handle);
}

static void PanelDragUpdate(bool active)
{
    static POINT last = {};
    static bool  wasActive = false;

    if (!active) { wasActive = false; return; }

    POINT cur = {};
    if (!::GetCursorPos(&cur)) return;
    if (!wasActive) { wasActive = true; last = cur; return; }

    const int dx = cur.x - last.x;
    const int dy = cur.y - last.y;
    if (dx == 0 && dy == 0) return;
    last = cur;

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    g_PanelPos.x = Clampf(g_PanelPos.x + (float)dx, 2.f, Maxf(2.f, ds.x - K::W - 2.f));
    g_PanelPos.y = Clampf(g_PanelPos.y + (float)dy, 2.f, Maxf(2.f, ds.y - K::H - 2.f));
}

static void DrawTopBar(ImDrawList* dl, ImVec2 wp)
{
    const float x0 = wp.x + K::Pad;
    const float x1 = wp.x + K::W - K::Pad;
    const ImVec2 a(x0, wp.y + K::Pad);
    const ImVec2 b(x1, wp.y + K::Pad + K::TopBarH);

    ImGui::SetCursorScreenPos(a);
    ImGui::InvisibleButton("##topbar_drag", ImVec2(b.x - a.x, b.y - a.y));
    const bool dragging = ImGui::IsItemActive();
    PanelDragUpdate(dragging);

    dl->AddRectFilled(a, b, GC(Mix(g_T.topBarBg, IM_COL32(255, 255, 255, 16), dragging ? 1.f : 0.f)), g_T.roundTopBar);
    dl->AddRect(a, b, GC(Mix(g_T.topBarBorder, g_T.controlBorderHi, dragging ? 1.f : 0.f)), g_T.roundTopBar, 0, g_T.borderThickness);

    const ImVec2 lc(a.x + 22.f, (a.y + b.y) * 0.5f);
    DrawLogo(dl, lc, 17.f, GC(g_T.textPrimary));
    const ImVec2 ts0 = TSizeBold(sync::kBrandName, true);
    TxtBold(dl, sync::kBrandName, ImVec2(lc.x + 16.f, (a.y + b.y) * 0.5f - ts0.y * 0.5f), GC(g_T.textPrimary), true);

    const ImVec2 ts = TSizeBold("ADMIN");
    const float rright = b.x - 20.f;
    const float badgeD = 18.f;
    const ImVec2 bc(rright - ts.x - 9.f - badgeD * 0.5f, (a.y + b.y) * 0.5f);
    dl->AddRectFilled(ImVec2(bc.x - badgeD * 0.5f, bc.y - badgeD * 0.5f),
                      ImVec2(bc.x + badgeD * 0.5f, bc.y + badgeD * 0.5f), GC(IM_COL32(255, 255, 255, 22)), 5.f);
    const ImVec2 qs = TSize(g_FontRegular, "?");
    Txt(dl, g_FontRegular, ImVec2(bc.x - qs.x * 0.5f, bc.y - qs.y * 0.5f), GC(g_T.textPrimary), "?");
    TxtBold(dl, "ADMIN", ImVec2(rright - ts.x, (a.y + b.y) * 0.5f - ts.y * 0.5f), GC(g_T.textPrimary));
}

static void ScrollBarV(const char* id, ImVec2 p0, ImVec2 p1, float viewH, float contentH)
{
    if (contentH <= viewH + 1.f) return;

    const float maxScroll = contentH - viewH;
    const float trackH    = Maxf(1.f, p1.y - p0.y);
    const float thumbH    = Clampf(trackH * (viewH / contentH), 30.f, trackH);
    const float t         = Clampf(g_ScrollShown / maxScroll, 0.f, 1.f);
    const float thumbY    = p0.y + (trackH - thumbH) * t;

    ImGui::PushID(id);
    ImGui::SetCursorScreenPos(ImVec2(p0.x - 5.f, p0.y));
    ImGui::InvisibleButton("##bar", ImVec2((p1.x - p0.x) + 10.f, trackH));
    const bool hovered = ImGui::IsItemHovered();
    const bool active  = ImGui::IsItemActive();
    if (active)
    {

        const float u = Clampf((ImGui::GetIO().MousePos.y - p0.y - thumbH * 0.5f) / Maxf(1.f, trackH - thumbH), 0.f, 1.f);
        g_ScrollTarget = u * maxScroll;
    }
    ImGui::PopID();

    float& av = AnimF("##barvis", 0.f);
    const bool moving = fabsf(g_ScrollTarget - g_ScrollShown) > 0.5f;
    av = Approach(av, (hovered || active || moving) ? 1.f : 0.f, 8.f, ImGui::GetIO().DeltaTime);
    if (av < 0.01f) return;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, p1, GC(IM_COL32(255, 255, 255, (int)(9.f * av))), (p1.x - p0.x) * 0.5f);
    dl->AddRectFilled(ImVec2(p0.x, thumbY), ImVec2(p1.x, thumbY + thumbH),
                      GC(IM_COL32(255, 255, 255, (int)((active ? 120.f : hovered ? 78.f : 52.f) * av))),
                      (p1.x - p0.x) * 0.5f);
}

static void DrawContent(ImVec2 wp, float top, float bottom)
{
    const float childX = wp.x + K::Pad + K::SidebarW + K::DividerGap + 2.f;
    const float childR = wp.x + K::W - 9.f;

    ImGui::SetCursorScreenPos(ImVec2(childX, top));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 7.f);
    ImGui::BeginChild("##content", ImVec2(childR - childX, bottom - top),
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoScrollWithMouse);

    ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    g_TabFade   = Approach(g_TabFade, 1.f, 22.f, dt);
    g_TabSlideX = Approach(g_TabSlideX, 0.f, 18.f, dt);

    if (fabsf(g_TabSlideX) < 0.05f) g_TabSlideX = 0.f;

    g_ContentL   = wp.x + K::Pad + K::SidebarW + K::DividerGap + 17.f + g_TabSlideX;
    g_ContentR   = wp.x + K::W - K::Pad - K::ScrollReserve + g_TabSlideX;
    g_ContentBottom = bottom - 8.f;

    g_ContentTop = top + K::ContentPadTop - g_ScrollShown;
    g_RowY       = g_ContentTop;

    g_AlphaMul = EaseOutCubic(g_TabFade);

    ImDrawList* body_dl = ImGui::GetWindowDrawList();
    float wipe = uifx::page_wipe();
    const bool wiping = wipe < 0.999f;

    const int  cascade_mode = ImClamp(uifx::wipe_is_view() ? settings::misc_ui::cascade_mode_view
                                                          : settings::misc_ui::cascade_mode, 0, 5);
    const float wipec = uifx::wipe_eased(wipe);
    float wipe_y = bottom;
    float wipe_x = childX;
    float drift  = 0.f;

    g_CascadeShiftDbg = 0.f;

    if (wiping && (cascade_mode == 2 || cascade_mode == 4))
    {
        const float d  = ImClamp(settings::misc_ui::cascade_rise, 0.f, 80.f) * (1.f - wipec);
        const float dy = (cascade_mode == 2) ? d : -d;
        g_RowY += dy;
        g_CascadeShiftDbg = dy;
        const ImVec2 cur = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(cur.x, cur.y + dy));
    }
    if (wiping && cascade_mode == 5)
    {

        drift = ImClamp(settings::misc_ui::cascade_drift, 0.f, 80.f) * (1.f - wipec);
        g_ContentL += drift;
        g_ContentR += drift;
        g_CascadeShiftDbg = drift;
    }
    if (wiping && (cascade_mode == 1 || cascade_mode == 2 || cascade_mode == 4 || cascade_mode == 5))
        g_AlphaMul = EaseOutCubic(g_TabFade) * wipec;

    if (wiping && (cascade_mode == 0 || cascade_mode == 3))
    {
        if (cascade_mode == 0)
        {
            wipe_y = top + (bottom - top) * wipec;
            ImGui::PushClipRect(ImVec2(childX - 20.f, top - 2.f),
                ImVec2(childR + 20.f, wipe_y), true);
        }
        else
        {
            wipe_x = childX - 20.f + (childR + 20.f - (childX - 20.f)) * wipec;
            ImGui::PushClipRect(ImVec2(childX - 20.f, top - 2.f),
                ImVec2(wipe_x, bottom), true);
        }
    }

    if      (g_ActiveTab == kTabHome)     DrawHomePage();
    else if (g_ActiveTab == kTabAim)      DrawAimPage();
    else if (g_ActiveTab == kTabVisuals)  DrawVisualsPage();
    else if (g_ActiveTab == kTabMisc)     DrawMiscPage();
    else if (g_ActiveTab == kTabWorld)    DrawWorldPage();
    else if (g_ActiveTab == kTabPlayers)  DrawPlayersPage();
    else if (g_ActiveTab == kTabExplorer) DrawExplorerPage();
    else                                  DrawConfigPage();

    g_AlphaMul = 1.f;

    if (drift != 0.f)
    {
        g_ContentL -= drift;
        g_ContentR -= drift;
    }

    if (wiping && (cascade_mode == 0 || cascade_mode == 3))
    {
        ImGui::PopClipRect();

        if (settings::misc_ui::cascade_hairline)
        {
            const float edge = ImMax(14.f, K::W * 0.06f);
            const ImU32 hot = uifx::light(0.30f * (1.f - wipec));
            const ImU32 cold = uifx::light(0.f);
            if (cascade_mode == 0)
            {
                body_dl->AddRectFilledMultiColor(ImVec2(childX, wipe_y - edge), ImVec2(childR, wipe_y), cold, cold, hot, hot);
                body_dl->AddRectFilled(ImVec2(childX, wipe_y - 1.f), ImVec2(childR, wipe_y),
                    uifx::light(0.55f * (1.f - wipec)), 0.f);
            }
            else
            {
                body_dl->AddRectFilledMultiColor(ImVec2(wipe_x, top), ImVec2(wipe_x + edge, bottom), cold, hot, hot, cold);
                body_dl->AddRectFilled(ImVec2(wipe_x, top), ImVec2(wipe_x + 1.f, bottom),
                    uifx::light(0.55f * (1.f - wipec)), 0.f);
            }
        }
    }

    const float viewH = bottom - top;

    g_ContentH = Maxf(0.f, (g_RowY + g_ScrollShown) - (top + K::ContentPadTop) + 14.f);
    const float maxScroll = Maxf(0.f, g_ContentH - viewH);
    g_ScrollTarget = Clampf(g_ScrollTarget, 0.f, maxScroll);

    const bool popupOpen = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    if (!popupOpen && maxScroll > 0.f && io.MouseWheel != 0.f && ImGui::IsWindowHovered())
        g_ScrollTarget = Clampf(g_ScrollTarget - io.MouseWheel * kScrollStep, 0.f, maxScroll);

    g_ScrollShown = SpringToId(kScrollSpringId, g_ScrollTarget, kScrollResponse, kScrollDamping, 0.f);
    if (fabsf(g_ScrollShown - g_ScrollTarget) < 0.05f) g_ScrollShown = g_ScrollTarget;

    ScrollBarV("##pagescroll", ImVec2(wp.x + K::W - K::Pad - 4.f, top + 3.f),
               ImVec2(wp.x + K::W - K::Pad + 1.f, bottom - 3.f), viewH, g_ContentH);

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

static void DrawMenu(ImVec2 wp)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    g_T.windowBg = IM_COL32(36, 36, 42, GlassAlpha());

    const ImVec2 p0(wp.x, wp.y);
    const ImVec2 p1(wp.x + K::W, wp.y + K::H);

    DrawBackdropRect(dl, p0, p1, g_T.roundWindow, g_MenuFade * 255.f);
    dl->AddRectFilled(p0, p1, GC(g_T.windowBg), g_T.roundWindow);
    dl->AddRect(p0, p1, GC(g_T.windowBorder), g_T.roundWindow, 0, g_T.borderThickness);

    {
        static int seen_page = -1;
        static int seen_sub = -1;
        const int sub = (g_ActiveTab >= 0 && g_ActiveTab < kTabCount) ? g_SubTab[g_ActiveTab] : -1;
        if (seen_page != g_ActiveTab || seen_sub != sub)
        {

            const bool view_change = (seen_page == g_ActiveTab) && (seen_sub != sub);
            seen_page = g_ActiveTab;
            seen_sub = sub;
            uifx::reset_stagger(view_change);
        }
    }

    uifx::note_panel(p0, p1, g_T.roundWindow);
    uifx::paint_rim(dl);

    DrawTopBar(dl, wp);

    const float bodyTop = wp.y + K::Pad + K::TopBarH + K::GapTopBody;
    const float bodyBot = wp.y + K::H - K::Pad;

    DrawSidebar(dl, wp, bodyTop, bodyBot);

    {
        const menu_clock::time_point page_t0 = cost_now();
        DrawContent(wp, bodyTop, bodyBot);
        g_page_ms = cost_ms(page_t0);
    }

    DrawToasts(dl, wp);
}

static bool g_MenuOpen = true;
static float g_MenuAlpha = 1.f;

static float g_MenuVisualFade = 1.f;
static float g_MenuScaleDbg = 1.f;

static int   g_FrameDbg = 0;
static FILE* g_Log = nullptr;

static void LogFrame(const char* what)
{
    if (!g_Log) return;
    if (++g_FrameDbg % 10) return;
    fprintf(g_Log, "%6d  open=%d scale=%.3f alpha=%.3f preview=%.3f slide=%.2f tab=%d what=%s\n",
            g_FrameDbg, g_MenuOpen ? 1 : 0, g_MenuScaleDbg, g_MenuAlpha,
            g_PreviewScaleDbg, g_TabSlideX, g_ActiveTab, what);
    fflush(g_Log);
}

static bool MenuOwnsWindow(ImGuiWindow* w)
{
    for (int guard = 0; w && guard < 16; ++guard)
    {
        if (w->Name && strcmp(w->Name, "##sync_menu") == 0)
            return true;
        w = w->ParentWindow;
    }
    return false;
}

static void ApplyMenuTransform(ImVec2 pivot, float s, ImVec2 off)
{
    if (s == 1.f && off.x == 0.f && off.y == 0.f) return;
    ImGuiContext& g = *ImGui::GetCurrentContext();
    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* w = g.Windows[n];
        if (!w || !MenuOwnsWindow(w)) continue;
        ImDrawList* cl = w->DrawList;
        for (int i = 0; i < cl->VtxBuffer.Size; i++)
        {
            ImVec2& p = cl->VtxBuffer[i].pos;
            p.x = pivot.x + (p.x - pivot.x) * s + off.x;
            p.y = pivot.y + (p.y - pivot.y) * s + off.y;
        }
        for (int i = 0; i < cl->CmdBuffer.Size; i++)
        {
            ImVec4& c = cl->CmdBuffer[i].ClipRect;
            const float x0 = c.x, y0 = c.y, x1 = c.z, y1 = c.w;
            c.x = pivot.x + (x0 - pivot.x) * s + off.x;
            c.y = pivot.y + (y0 - pivot.y) * s + off.y;
            c.z = pivot.x + (x1 - pivot.x) * s + off.x;
            c.w = pivot.y + (y1 - pivot.y) * s + off.y;

            const ImGuiViewport* vp = ImGui::GetMainViewport();
            const ImVec2 dpos = vp->Pos, dsize = vp->Size;
            c.x = ImMax(c.x, dpos.x);               c.y = ImMax(c.y, dpos.y);
            c.z = ImMin(c.z, dpos.x + dsize.x);     c.w = ImMin(c.w, dpos.y + dsize.y);
        }
    }
}

static const float kClosedScale   = kBubbleClosed;

static const float kOpenResponse  = kBubbleResponse, kOpenDamping  = kBubbleDamping;

static const float kCloseResponse = 0.30f;
static const float kCloseDamping  = 1.0f;
static const ImGuiID kMenuSpringId = 0x5A17C0DEu;

static bool g_PrevMenuOpen = true;

static float g_OpenSettleT = 0.f;

static const float kFirstOpenScale    = 0.55f;
static const float kFirstOpenResponse = 0.42f;
static const float kFirstOpenDamping  = 0.62f;

static bool  g_FirstEntrance = true;
static bool  g_FirstRide     = false;
static float g_FlashT        = 1.f;

static float g_FpsHist[64] = {};
static int   g_FpsHistN = 0;

static bool   g_CardDrag = false;
static POINT  g_CardGrab = {};

static bool CardDragUpdate(ImVec2 p0, ImVec2 p1)
{
    if (!g_hwnd) return false;

    POINT cur = {};
    RECT  wr  = {};
    if (!::GetCursorPos(&cur) || !::GetWindowRect(g_hwnd, &wr)) return false;
    const float mx = (float)(cur.x - wr.left), my = (float)(cur.y - wr.top);
    const bool  down = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    if (!g_CardDrag)
    {
        if (!down) return false;
        if (mx < p0.x - 4.f || mx > p1.x + 4.f || my < p0.y - 4.f || my > p1.y + 4.f) return false;
        g_CardDrag  = true;
        g_CardGrab.x = (LONG)(mx - p0.x);
        g_CardGrab.y = (LONG)(my - p0.y);
        return true;
    }
    if (!down) { g_CardDrag = false; return false; }

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    g_CardPos.x = Clampf(mx - (float)g_CardGrab.x, 4.f, Maxf(4.f, ds.x - K::CardW - 4.f));
    g_CardPos.y = Clampf(my - (float)g_CardGrab.y, 4.f, Maxf(4.f, ds.y - K::CardH - 4.f));
    return true;
}

static void DrawWatermark()
{
    if (settings::menu::hide_watermark || !sync::state().wm_show) { g_CardDrag = false; return; }

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImGuiIO& io = ImGui::GetIO();
    const float fps = io.Framerate > 0.f ? io.Framerate : 0.f;
    const float ms  = fps > 0.f ? 1000.f / fps : 0.f;

    g_FpsHist[g_FpsHistN & 63] = fps;
    ++g_FpsHistN;

    const ImVec2 p0(g_CardPos.x, g_CardPos.y);
    const ImVec2 p1(g_CardPos.x + K::CardW, g_CardPos.y + K::CardH);
    const bool dragging = CardDragUpdate(p0, p1);

    DrawBackdropRect(dl, p0, p1, 12.f);
    dl->AddRectFilled(p0, p1, g_T.windowBg, 12.f);
    dl->AddRect(p0, p1, IM_COL32(255, 255, 255, dragging ? 46 : 26), 12.f, 0, g_T.borderThickness);

    const ImVec2 lc(p0.x + 23.f, p0.y + 19.f);
    DrawLogo(dl, lc, 16.f, IM_COL32(238, 238, 242, 255));
    TxtBold(dl, sync::kBrandName, ImVec2(lc.x + 15.f, lc.y - 8.f), IM_COL32(238, 238, 242, 255));
    {
        char me[64] = "not attached";
        char handle[64] = "";
        if (sync::hooks().local_player)
            sync::hooks().local_player(me, sizeof me, handle, sizeof handle);
        const char* user = handle[0] ? handle : me;
        const ImVec2 s = TSize(g_FontSmall, user);
        Txt(dl, g_FontSmall, ImVec2(p1.x - 14.f - s.x, lc.y - 7.f), IM_COL32(152, 152, 158, 255), user);
    }

    const ImU32 green = IM_COL32(126, 214, 132, 255);
    const ImU32 dim   = IM_COL32(146, 146, 152, 255);
    const ImU32 sep   = IM_COL32(124, 124, 130, 200);
    const ImU32 val   = IM_COL32(226, 226, 230, 255);
    const float y = p0.y + 41.f;
    float x = p0.x + 16.f;
    char buf[24];
    bool first = true;

    if (sync::state().wm_fps)
    {
        snprintf(buf, sizeof(buf), "%.0f", fps);
        TxtBold(dl, buf, ImVec2(x, y), green);
        x += TSizeBold(buf).x + 5.f;
        Txt(dl, g_FontRegular, ImVec2(x, y), dim, "FPS");
        x += TSize(g_FontRegular, "FPS").x + 13.f;

        snprintf(buf, sizeof(buf), "%.0f", ms);
        TxtBold(dl, buf, ImVec2(x, y), green);
        x += TSizeBold(buf).x + 5.f;
        Txt(dl, g_FontRegular, ImVec2(x, y), dim, "MS");
        x += TSize(g_FontRegular, "MS").x + 11.f;
        first = false;
    }

    if (!first)
    {
        dl->AddCircleFilled(ImVec2(x + 1.5f, y + 8.f), 1.6f, sep);
        x += 13.f;
    }

    {

        const menu_clock::time_point count_t0 = cost_now();
        int n = SyncPlayerCount();
        g_count_ms = cost_ms(count_t0);
        if (n < 0) n = 0;
        DrawTabIcon(dl, ICON_USERS, ImVec2(x + 7.f, y + 8.f), 15.f, dim);
        x += 18.f;
        snprintf(buf, sizeof(buf), "%d", n);
        Txt(dl, g_FontRegular, ImVec2(x, y), val, buf);
        x += TSize(g_FontRegular, buf).x + 11.f;
    }

    if (sync::state().wm_time)
    {
        dl->AddCircleFilled(ImVec2(x + 1.5f, y + 8.f), 1.6f, sep);
        x += 13.f;

        SYSTEMTIME st = {};
        ::GetLocalTime(&st);
        snprintf(buf, sizeof(buf), "%02d:%02d", (int)st.wHour, (int)st.wMinute);
        Txt(dl, g_FontRegular, ImVec2(x, y), val, buf);
    }

    const int n = g_FpsHistN < 64 ? g_FpsHistN : 64;
    if (n > 2)
    {
        const float gx0 = p0.x + 16.f, gx1 = p1.x - 16.f;
        const float gy = p0.y + K::CardH - 10.f, gh = 5.f;
        ImVec2 prev(0.f, 0.f);
        for (int i = 0; i < n; i++)
        {
            const float v = Clampf(g_FpsHist[(g_FpsHistN - n + i) & 63] / 240.f, 0.f, 1.f);
            const ImVec2 p(gx0 + (gx1 - gx0) * ((float)i / (float)(n - 1)), gy + gh - gh * v);
            if (i) dl->AddLine(prev, p, IM_COL32(126, 214, 132, 150), 1.2f);
            prev = p;
        }
    }
}

struct DockBind { const char* name; int key; int mode; bool on; int live; };

static const char* DockKeyName(int vk)
{
    if (vk <= 0) return "None";
    const int names = (int)(sizeof(key_namess) / sizeof(key_namess[0]));
    if (vk < names && key_namess[vk] && strcmp(key_namess[vk], "Unknown") != 0)
        return key_namess[vk];

    static char fb[16];
    if (vk >= VK_F1 && vk <= VK_F24)
        snprintf(fb, sizeof fb, "F%d", vk - VK_F1 + 1);
    else
        snprintf(fb, sizeof fb, "0x%02X", vk);
    return fb;
}

static int CollectBinds(DockBind* out, int max, bool active_only)
{
    using namespace settings;

    DockBind all[] = {
        { "Aimbot",        aimbot::keybind,    aimbot::activation_mode,    aimbot::enabled, live_state::kAimbot },
        { "Silent Aim",    silentaim::keybind,  silentaim::activation_mode,  silentaim::enabled, live_state::kSilentAim },
        { "Triggerbot",    triggerbot::keybind, triggerbot::activation_mode, triggerbot::enabled, live_state::kTriggerbot },
        { "Speed Hack",    movement::speedhack::keybind, movement::speedhack::activation_mode, movement::speedhack::enabled, live_state::kSpeed },
        { "Fly Hack",      movement::flyhack::keybind,   movement::flyhack::activation_mode,   movement::flyhack::enabled, live_state::kFly },
        { "Noclip",        noclip::keybind,    noclip::activation_mode,    noclip::enabled, live_state::kNoclip },
        { "Spin 360",      spin360::keybind,   spin360::activation_mode,   spin360::enabled, live_state::kSpin360 },
        { "Walkspeed",     walkspeed::keybind, walkspeed::keybind_mode,    walkspeed::enabled, live_state::kWalkspeed },
        { "Sonar",         sonar::keybind,     sonar::activation_mode,     sonar::enabled,  live_state::kSonar },
    };

    int n = 0;
    for (const DockBind& b : all)
    {
        if (b.mode == 2)
        {
            (void)active_only;
            if (!b.on) continue;
        }
        else if (!b.key)
        {
            continue;
        }
        if (n < max) out[n++] = b;
    }
    return n;
}

static bool   g_DockDrag = false;
static POINT  g_DockGrab{};

static bool DockDragUpdate(ImVec2 p0, ImVec2 p1)
{
    if (!g_hwnd) return false;

    POINT cur = {};
    RECT  wr  = {};
    if (!::GetCursorPos(&cur) || !::GetWindowRect(g_hwnd, &wr)) return false;
    const float mx = (float)(cur.x - wr.left), my = (float)(cur.y - wr.top);
    const bool  down = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    sync::State& st = sync::state();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;

    if (!g_DockDrag)
    {
        if (!down) return false;
        if (mx < p0.x - 4.f || mx > p1.x + 4.f || my < p0.y - 4.f || my > p1.y + 4.f) return false;

        g_DockDrag  = true;
        g_DockGrab.x = (LONG)(mx - p0.x);
        g_DockGrab.y = (LONG)(my - p0.y);
        return true;
    }
    if (!down) { g_DockDrag = false; return false; }

    const float w = p1.x - p0.x, h = p1.y - p0.y;
    st.kb_x = Clampf(mx - (float)g_DockGrab.x, 4.f, Maxf(4.f, ds.x - w - 4.f));
    st.kb_y = Clampf(my - (float)g_DockGrab.y, 4.f, Maxf(4.f, ds.y - h - 4.f));
    return true;
}

static const char* DockModeName(int mode)
{
    return mode == 0 ? "toggle" : (mode == 1 ? "hold" : "always");
}

static void DrawKeybindDock()
{
    sync::State& st = sync::state();
    if (!st.kb_show) return;

    DockBind binds[16];
    const int n = CollectBinds(binds, 16, st.kb_only_active);
    if (n <= 0) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    const float pad_x = 15.f, pad_y = 11.f, row_h = 19.f;
    const float col_gap = 13.f;

    float key_w = 0.f, name_w = 0.f, mode_w = 0.f;
    for (int i = 0; i < n; ++i)
    {
        key_w  = Maxf(key_w,  TSizeBold(binds[i].mode == 2 ? "-" : DockKeyName(binds[i].key)).x);
        name_w = Maxf(name_w, TSize(g_FontRegular, binds[i].name).x);
        mode_w = Maxf(mode_w, TSize(g_FontSmall, DockModeName(binds[i].mode)).x);
    }
    if (key_w < 24.f) key_w = 24.f;

    const float w = pad_x * 2.f + key_w + col_gap + name_w + col_gap + mode_w;
    const float h = pad_y * 2.f + (float)n * row_h;

    {
        const ImVec2 at(st.kb_x, st.kb_y);
        DockDragUpdate(at, ImVec2(at.x + w, at.y + h));
    }

    const ImVec2 p0(st.kb_x, st.kb_y);
    const ImVec2 p1(p0.x + w, p0.y + h);

    DrawBackdropRect(dl, p0, p1, 12.f);
    dl->AddRectFilled(p0, p1, g_T.windowBg, 12.f);
    dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 26), 12.f, 0, g_T.borderThickness);

    const float x_key  = p0.x + pad_x;
    const float x_name = x_key + key_w + col_gap;
    const float x_mode = p1.x - pad_x - mode_w;

    const ImU32 key_col  = IM_COL32(238, 238, 242, 255);
    const ImU32 name_col = IM_COL32(186, 186, 192, 255);
    const ImU32 dim_col  = IM_COL32(132, 132, 140, 255);
    const ImU32 on_col   = IM_COL32(126, 214, 132, 255);

    float y = p0.y + pad_y;
    for (int i = 0; i < n; ++i)
    {
        const bool firing = live_state::get(binds[i].live);
        const char* kname = (binds[i].mode == 2) ? "-" : DockKeyName(binds[i].key);
        const ImVec2 ks = TSizeBold(kname);

        if (firing)
            dl->AddCircleFilled(ImVec2(p0.x + pad_x * 0.42f, y + g_FontRegular->FontSize * 0.5f),
                2.2f, on_col, 12);

        TxtBold(dl, kname, ImVec2(x_key + (key_w - ks.x), y), firing ? on_col : key_col);
        Txt(dl, g_FontRegular, ImVec2(x_name, y), name_col, binds[i].name);
        Txt(dl, g_FontSmall, ImVec2(x_mode, y + 1.f), dim_col, DockModeName(binds[i].mode));
        y += row_h;
    }
}

static void DrawHeartShape(ImDrawList* dl, ImVec2 c, float k, ImU32 col)
{
    const int segs = 48;
    ImVec2 pts[segs];
    for (int i = 0; i < segs; ++i)
    {
        const float t  = (float)i / (float)segs * 6.2831853f;
        const float st = sinf(t), ct = cosf(t);
        const float x  = 16.f * st * st * st;
        const float y  = 13.f * ct - 5.f * cosf(2.f * t) - 2.f * cosf(3.f * t) - cosf(4.f * t);

        pts[i].x = c.x + (x / 16.f) * k;
        pts[i].y = c.y - (y / 17.f + 0.15f) * k;
    }
    dl->AddConcavePolyFilled(pts, segs, col);
}

static void DrawTargetHud()
{
    sync::State& st = sync::state();
    if (!st.th_show || !sync::hooks().aim_target_instance) return;

    const unsigned long long target = sync::hooks().aim_target_instance();
    if (!target || !sync::hooks().players_snapshot) return;

    static sync::PlayerRow rows[sync::kMaxPlayerRows];
    const int n = sync::hooks().players_snapshot(rows, sync::kMaxPlayerRows);

    const sync::PlayerRow* who = nullptr;
    for (int i = 0; i < n; ++i)
        if (rows[i].instance == target) { who = &rows[i]; break; }
    if (!who) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    const float s     = Clampf(st.th_scale, 0.6f, 2.f);
    const float pad   = 10.f * s;
    const float av_d  = 30.f * s;
    const float row_h = 20.f * s;

    char hp[16];
    snprintf(hp, sizeof hp, "%d", (int)(who->health + 0.5f));

    float text_w = 0.f;
    if (st.th_name)   text_w = Maxf(text_w, TSizeBold(who->name).x);
    if (st.th_health) text_w = Maxf(text_w, TSizeBold(hp).x + 8.f * s + 12.f * s);
    if (st.th_tool)   text_w = Maxf(text_w, TSize(g_FontSmall, who->tool).x);
    if (text_w < 40.f * s) text_w = 40.f * s;

    const float w = pad * 2.f + (st.th_avatar ? av_d + 10.f * s : 0.f) + text_w;
    const float h = pad * 2.f + Maxf(av_d, row_h * 2.f);

    ImVec2 pp(st.th_x, st.th_y);
    float  ax = 0.f, ay = 0.f;
    if (sync::hooks().player_hud_anchor && sync::hooks().player_hud_anchor(target, ax, ay))
    {
        pp.x = ax - w * 0.5f;
        pp.y = ay - h - 4.f;

        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        pp.x = Clampf(pp.x, 4.f, Maxf(4.f, ds.x - w - 4.f));
        pp.y = Clampf(pp.y, 4.f, Maxf(4.f, ds.y - h - 4.f));
    }
    const ImVec2 p0(pp.x, pp.y);
    const ImVec2 p1(p0.x + w, p0.y + h);

    DrawBackdropRect(dl, p0, p1, 14.f);
    dl->AddRectFilled(p0, p1, g_T.windowBg, 14.f);
    dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 26), 14.f, 0, g_T.borderThickness);

    float tx = p0.x + pad;

    if (st.th_avatar)
    {
        const ImVec2 ac(tx + av_d * 0.5f, p0.y + h * 0.5f);
        dl->AddCircleFilled(ac, av_d * 0.5f, IM_COL32(255, 255, 255, 18), 32);

        const bool drew = sync::hooks().player_avatar
            && sync::hooks().player_avatar(dl, who->user_id,
                ImVec2(ac.x - av_d * 0.5f, ac.y - av_d * 0.5f),
                ImVec2(ac.x + av_d * 0.5f, ac.y + av_d * 0.5f), IM_COL32_WHITE);
        if (!drew)
        {

            char initial[2] = { who->name[0] ? who->name[0] : '?', 0 };
            const ImVec2 is = TSizeBold(initial);
            TxtBold(dl, initial, ImVec2(ac.x - is.x * 0.5f, ac.y - is.y * 0.5f),
                IM_COL32(206, 206, 212, 255));
        }
        dl->AddCircle(ac, av_d * 0.5f, IM_COL32(255, 255, 255, 40), 32, 1.f);

        tx += av_d + 10.f * s;
    }

    const float cy = p0.y + h * 0.5f;
    const bool two_rows = st.th_health || st.th_tool;
    float ty = two_rows ? (cy - row_h) : (cy - 7.f * s);

    if (st.th_name)
    {
        TxtBold(dl, who->name, ImVec2(tx, ty), IM_COL32(240, 240, 244, 255));
        ty += row_h;
    }

    if (st.th_health)
    {

        const float hpv = who->max_health > 0.f ? Clampf(who->health / who->max_health, 0.f, 1.f) : 0.f;
        const ImU32 hp_col = hpv > 0.6f ? IM_COL32(126, 214, 132, 255)
                           : hpv > 0.3f ? IM_COL32(226, 200, 106, 255)
                                        : IM_COL32(226, 106, 106, 255);

        const ImVec2 hs = TSizeBold(hp);
        TxtBold(dl, hp, ImVec2(tx, ty), hp_col);

        const float hk = 6.2f * s;
        const ImVec2 hc(tx + hs.x + 6.f * s + hk, ty + g_FontBold->FontSize * 0.5f);
        DrawHeartShape(dl, hc, hk, IM_COL32(226, 86, 96, 255));
        ty += row_h;
    }

    if (st.th_tool && who->tool[0])
    {
        Txt(dl, g_FontSmall, ImVec2(tx, ty + 2.f * s), IM_COL32(158, 158, 166, 255), who->tool);
        ty += row_h;
    }
}

static void DrawEntranceFlash(ImVec2 pivot, float scale, float dt)
{
    if (!settings::misc_ui::open_flash)
    {
        g_FlashT = 1.f;
        return;
    }
    if (g_FlashT >= 1.f)
        return;

    g_FlashT = Minf(1.f, g_FlashT + dt / 0.42f);
    const float left = 1.f - g_FlashT;
    const float out  = left * left;
    const float str  = Clampf(settings::misc_ui::open_flash_strength, 0.f, 1.5f);
    if (out <= 0.004f)
        return;

    const float hw = K::W * 0.5f * scale;
    const float hh = K::H * 0.5f * scale;
    const ImVec2 mn(pivot.x - hw, pivot.y - hh);
    const ImVec2 mx(pivot.x + hw, pivot.y + hh);

    ImDrawList* fg = ImGui::GetForegroundDrawList();

    const float spread = 5.f + 34.f * (1.f - out);
    const float wide   = spread * 1.9f;

    fg->AddRect(ImVec2(mn.x - wide, mn.y - wide), ImVec2(mx.x + wide, mx.y + wide),
        uifx::light(0.12f * out * str), g_T.roundWindow + wide, 0, 3.2f);
    fg->AddRect(ImVec2(mn.x - spread, mn.y - spread), ImVec2(mx.x + spread, mx.y + spread),
        uifx::light(0.38f * out * str), g_T.roundWindow + spread, 0, 1.6f);
}


static void UpdateGate()
{
    sync::State& st = sync::state();

    const bool roblox = (memory != nullptr) && (memory->get_process_id() != 0);
    st.roblox_found = roblox;

    static menu_clock::time_point s_lost_at{};
    if (roblox)
    {
        s_lost_at = menu_clock::time_point{};
        st.roblox_missing = false;
        st.roblox_missing_seconds = 0;
    }
    else
    {
        if (s_lost_at.time_since_epoch().count() == 0)
            s_lost_at = menu_clock::now();
        const int gone = (int)std::chrono::duration_cast<std::chrono::seconds>(
            menu_clock::now() - s_lost_at).count();
        st.roblox_missing = true;
        st.roblox_missing_seconds = gone >= 20 ? 0 : (20 - gone);
        if (gone >= 20)
            settings::settings::should_unload = true;
    }

    st.place_id = game_ctx::place_id;

    if (st.place_ignore_last && !settings::menu::ignore_unsupported_place)
        st.place_notice_done = false;
    st.place_ignore_last = settings::menu::ignore_unsupported_place;

    static std::uint64_t s_notice_place = 0;

    if (!(roblox && game_ctx::in_place && game_ctx::place_id != 0))
    {
        if (s_notice_place != 0)
        {
            s_notice_place       = 0;
            st.place_notice_done = false;
        }
        st.place_notice_pending = false;
        return;
    }

    if (game_ctx::place_id != s_notice_place)
    {
        s_notice_place       = game_ctx::place_id;
        st.place_notice_done = false;
    }

    const bool supported = game_ctx::place_supported();
    if (!supported && !settings::menu::ignore_unsupported_place && !st.place_notice_done)
        st.place_notice_pending = true;
    else
        st.place_notice_pending = false;
}

    static float g_GateX = -1.f, g_GateY = -1.f;

    static void gate_track_drag(ImVec2 size, ImVec2 display)
    {
        if (g_GateX < 0.f)
        {
            g_GateX = floorf(display.x * 0.5f - size.x * 0.5f);
            g_GateY = floorf(display.y * 0.5f - size.y * 0.5f);
        }

        if (!g_hwnd) return;

        POINT cur{};
        RECT  wr{};
        if (!::GetCursorPos(&cur) || !::GetWindowRect(g_hwnd, &wr)) return;
        const float mx = (float)(cur.x - wr.left), my = (float)(cur.y - wr.top);
        const bool  down = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        static bool  s_drag = false;
        static float gx = 0.f, gy = 0.f;

        const bool over_header = mx >= g_GateX - 2.f && mx <= g_GateX + size.x + 2.f &&
                                 my >= g_GateY - 2.f && my <= g_GateY + 46.f;

        if (!s_drag)
        {
            if (!down || !over_header) return;
            s_drag = true;
            gx = mx - g_GateX;
            gy = my - g_GateY;
        }
        else if (!down)
        {
            s_drag = false;
            return;
        }

        g_GateX = Clampf(mx - gx, 2.f, Maxf(2.f, display.x - size.x - 2.f));
        g_GateY = Clampf(my - gy, 2.f, Maxf(2.f, display.y - size.y - 2.f));
    }

static void DrawGateOverlay()
{
    sync::State& st = sync::state();
    const bool missing = st.roblox_missing;
    const bool place   = !missing && st.place_notice_pending;
    if (!missing && !place) return;

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float w = 438.f;
    const float h = missing ? 152.f : 186.f;
    gate_track_drag(ImVec2(w, h), ds);
    const ImVec2 p0(g_GateX, g_GateY);

    ImGui::SetNextWindowPos(p0, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));

    ImGui::Begin("##aether_gate", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 q0(p0.x, p0.y);
    const ImVec2 q1(p0.x + w, p0.y + h);

    dl->AddRectFilled(ImVec2(q0.x - 6.f, q0.y - 6.f), ImVec2(q1.x + 6.f, q1.y + 6.f),
        IM_COL32(0, 0, 0, 110), 20.f);
    DrawBackdropRect(dl, q0, q1, 16.f);
    dl->AddRectFilled(q0, q1, IM_COL32(15, 16, 20, 252), 16.f);
    dl->AddRect(q0, q1, IM_COL32(255, 255, 255, 30), 16.f, 0, 1.f);
    dl->AddRect(ImVec2(q0.x + 1.f, q0.y + 1.f), ImVec2(q1.x - 1.f, q1.y - 1.f),
        IM_COL32(255, 255, 255, 14), 15.f, 0, 1.f);

    const ImVec2 lc(q0.x + 28.f, q0.y + 30.f);
    DrawLogo(dl, lc, 15.f, IM_COL32(238, 238, 242, 255));
    TxtBold(dl, sync::kBrandName, ImVec2(lc.x + 14.f, lc.y - 8.f), IM_COL32(238, 238, 242, 255));

    if (missing)
    {
        TxtBold(dl, "Roblox not found", ImVec2(q0.x + 28.f, q0.y + 54.f), IM_COL32(240, 240, 244, 255));
        Txt(dl, g_FontSmall, ImVec2(q0.x + 28.f, q0.y + 78.f), IM_COL32(204, 204, 212, 255),
            "Open Roblox to use the overlay.");

        char left[48];
        snprintf(left, sizeof left, "Closing in %d s", st.roblox_missing_seconds > 0 ? st.roblox_missing_seconds : 0);
        Txt(dl, g_FontSmall, ImVec2(q0.x + 28.f, q0.y + 100.f), IM_COL32(226, 196, 106, 255), left);

        const float bar_w = w - 56.f;
        const float frac  = Clampf((float)st.roblox_missing_seconds / 20.f, 0.f, 1.f);
        dl->AddRectFilled(ImVec2(q0.x + 28.f, q1.y - 34.f), ImVec2(q0.x + 28.f + bar_w, q1.y - 30.f),
            IM_COL32(255, 255, 255, 26), 2.f);
        dl->AddRectFilled(ImVec2(q0.x + 28.f, q1.y - 34.f), ImVec2(q0.x + 28.f + bar_w * frac, q1.y - 30.f),
            IM_COL32(226, 196, 106, 220), 2.f);
    }
    else
    {
        TxtBold(dl, "Unsupported place!", ImVec2(q0.x + 28.f, q0.y + 52.f), IM_COL32(240, 240, 244, 255));
        Txt(dl, g_FontSmall, ImVec2(q0.x + 28.f, q0.y + 76.f), IM_COL32(204, 204, 212, 255),
            "Features may not work as intended");

        const float bw = (w - 56.f - 10.f) * 0.5f;
        const float by = q1.y - 62.f;

        if (ButtonControl("gate_continue", ImVec2(q0.x + 28.f, by), ImVec2(bw, 32.f), "Continue", true))
        {
            st.place_notice_pending = false;
            st.place_notice_done    = true;
        }
        if (ButtonControl("gate_ignore", ImVec2(q0.x + 38.f + bw, by), ImVec2(bw, 32.f), "Don't ask again"))
        {
            settings::menu::ignore_unsupported_place = true;
            st.place_notice_pending = false;
            st.place_notice_done    = true;
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

static void DrawFrameAndAnimate(ImVec2 displaySize)
{
    ImGuiIO& io = ImGui::GetIO();
    const float dt = Minf(io.DeltaTime, 1.f / 20.f);

    g_cost = sync::Cost{};
    g_page_ms = 0.0;
    g_count_ms = 0.0;

    {
        static int picked = -2;
        if (picked != settings::misc_ui::menu_font)
        {
            picked        = settings::misc_ui::menu_font;
            g_FontRegular = aether::fonts::menu_regular();
            g_FontBold    = aether::fonts::menu_bold();
            g_FontTitle   = aether::fonts::menu_title();
            g_FontSmall   = aether::fonts::menu_small();
            g_FontDisplay     = aether::fonts::menu_display();
            g_FontDisplayBold = aether::fonts::menu_display_bold();
            g_FontValue       = aether::fonts::menu_value();
            g_BoldIsReal  = (g_FontBold != nullptr && g_FontBold != g_FontRegular);
        }
    }

    if (g_FirstEntrance)
    {
        g_FirstEntrance = false;
        if (settings::misc_ui::first_pop)
        {
            g_FirstRide = true;
            g_FlashT    = 0.f;
            SpringSetId(kMenuSpringId, kFirstOpenScale, 0.f);
            g_TabFade   = 0.f;
            g_TabSlideX = -12.f;
            if (g_ActiveTab == kTabVisuals)
                SpringSetId(kPreviewSpringId, kPreviewClosedScale, 0.f);
        }
        else if (g_MenuOpen)
        {

            SpringSetId(kMenuSpringId, 1.f, 0.f);
            g_TabFade   = 1.f;
            g_TabSlideX = 0.f;
        }
    }

    if (g_PrevMenuOpen != g_MenuOpen)
    {
        CancelKeyCapture();
        if (g_MenuOpen)
        {

            g_FlashT = 0.f;

            g_TabFade   = 0.f;
            g_TabSlideX = -12.f;
            if (g_ActiveTab == kTabVisuals)
                SpringSetId(kPreviewSpringId, kPreviewClosedScale, 0.f);
        }
        g_PrevMenuOpen = g_MenuOpen;
    }

    {
        const menu_clock::time_point poll_t0 = cost_now();
        PollKeyCapture();
        g_cost.poll = cost_ms(poll_t0);
    }

    const ImVec2 pivot(g_PanelPos.x + K::W * 0.5f, g_PanelPos.y + K::H * 0.5f);

    const float open_response = (g_MenuOpen && g_FirstRide) ? kFirstOpenResponse : kOpenResponse;
    const float open_damping  = (g_MenuOpen && g_FirstRide) ? kFirstOpenDamping  : kOpenDamping;

    float scale = SpringToIdDt(kMenuSpringId,
                               g_MenuOpen ? 1.f : kClosedScale,
                               g_MenuOpen ? open_response : kCloseResponse,
                               g_MenuOpen ? open_damping  : kCloseDamping,
                               dt, kClosedScale);

    if (g_FirstRide && (!g_MenuOpen || scale > 0.995f))
        g_FirstRide = false;

    g_MenuScaleDbg = scale;

    float t = Clampf((scale - kClosedScale) / (1.f - kClosedScale), 0.f, 1.f);

    if (!g_MenuOpen && scale <= kClosedScale + 0.002f)
    {
        SpringSetId(kMenuSpringId, kClosedScale, 0.f);
        scale = kClosedScale;
        t = 0.f;
    }

    if (g_MenuOpen)
    {
        g_OpenSettleT += dt;
        const float vel = ImFabs(SpringVelId(kMenuSpringId));
        const bool at_rest = ImFabs(scale - 1.f) < 0.0015f && vel < 0.03f;
        const bool capped  = g_OpenSettleT > 2.f && ImFabs(scale - 1.f) < 0.02f;
        if (at_rest || capped)
        {
            SpringSetId(kMenuSpringId, 1.f, 0.f);
            scale = 1.f;
            t = 1.f;
        }
    }
    else
    {
        g_OpenSettleT = 0.f;
    }

    g_MenuAlpha = (t < 0.001f) ? 0.f : BubbleAlpha(t);

    uifx::note_menu_fade(g_MenuAlpha);

    g_MenuVisualFade = g_MenuAlpha;

    if (t > 0.002f || g_MenuAlpha > 0.002f)
    {
        const ImVec2 offset(0.f, 0.f);
        ImGuiStyle& style = ImGui::GetStyle();
        style.Alpha = g_MenuAlpha;

        g_MenuFade = g_MenuAlpha;
        g_MenuDrawAlpha = (float)((GC(IM_COL32(255, 255, 255, 255)) >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;

        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(displaySize, ImGuiCond_Always);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("##sync_menu", nullptr, flags);
        const menu_clock::time_point panel_t0 = cost_now();
        if (g_FontRegular) ImGui::PushFont(g_FontRegular);

        DrawMenu(ImVec2(ImGui::GetWindowPos().x + g_PanelPos.x, ImGui::GetWindowPos().y + g_PanelPos.y));
        if (g_FontRegular) ImGui::PopFont();
        ImGui::End();
        style.Alpha = 1.f;
        g_MenuFade = 1.f;

        g_cost.panel = cost_ms(panel_t0);
        g_cost.page  = g_page_ms;
        g_cost.open  = true;
        if (g_cost.panel > g_worst_panel_ms)
            g_worst_panel_ms = g_cost.panel;

        {
            const menu_clock::time_point xform_t0 = cost_now();
            ApplyMenuTransform(pivot, scale, offset);
            g_cost.xform = cost_ms(xform_t0);
        }

        DrawEntranceFlash(pivot, scale, dt);
    }
    else if (!g_MenuOpen)
    {

        g_MenuAlpha = 0.f;
        g_MenuVisualFade = 0.f;
    }

    {
        const menu_clock::time_point card_t0 = cost_now();
        DrawWatermark();
        g_cost.card  = cost_ms(card_t0);
        g_cost.count = g_count_ms;
    }

    {
        const menu_clock::time_point kb_t0 = cost_now();
        DrawKeybindDock();
        DrawTargetHud();
        g_cost.docks = cost_ms(kb_t0);
    }

    DrawGateOverlay();

    {
        const menu_clock::time_point dk_t0 = cost_now();
        const bool panel_up = g_MenuOpen && g_MenuAlpha > 0.002f;
        ImDrawList* list = panel_up ? ImGui::GetBackgroundDrawList() : ImGui::GetForegroundDrawList();
        const ImVec2 dp0(g_PanelPos.x, g_PanelPos.y);
        const ImVec2 dp1(g_PanelPos.x + K::W, g_PanelPos.y + K::H);
        const bool interactive = !docks::pointer_owned_by_panel(dp0, dp1, panel_up);
        docks::draw(list, displaySize, interactive);
        g_cost.docks += cost_ms(dk_t0);
    }
}

namespace sync
{
	bool gate_wants_mouse()
	{
		State& st = state();
		if (!st.place_notice_pending || !g_hwnd) return false;

		POINT cur{};
		RECT  wr{};
		if (!::GetCursorPos(&cur) || !::GetWindowRect(g_hwnd, &wr)) return false;
		const float mx = (float)(cur.x - wr.left), my = (float)(cur.y - wr.top);
		return mx >= g_GateX - 10.f && mx <= g_GateX + 448.f &&
		       my >= g_GateY - 10.f && my <= g_GateY + 196.f;
	}

	bool init()
	{
		g_FontRegular = aether::fonts::menu_regular();
		g_FontBold    = aether::fonts::menu_bold();
		g_FontTitle   = aether::fonts::menu_title();
		g_FontSmall   = aether::fonts::menu_small();
		g_FontDisplay     = aether::fonts::menu_display();
		g_FontDisplayBold = aether::fonts::menu_display_bold();
		g_FontValue       = aether::fonts::menu_value();
		g_BoldIsReal  = (g_FontBold != nullptr && g_FontBold != g_FontRegular);

		ApplyStyle();
		ApplyImGuiStyle();

		return g_FontRegular != nullptr;
	}

	bool open()
	{
		return g_MenuOpen;
	}

	void set_open(bool value)
	{
		g_MenuOpen = value;
	}

	float fade()
	{
		return g_MenuVisualFade;
	}

	void set_hwnd(void* hwnd)
	{

		g_hwnd = (HWND)hwnd;
	}

void draw(ImVec2 display_size)
{

	anim::begin_frame();
	ApplyStyle();

		UpdateGate();

		sync::occlusion().n = 0;

		if (g_PanelPos.x < 0.f || g_PanelPos.y < 0.f)
		{
			g_PanelPos.x = Clampf((display_size.x - K::W) * 0.5f, 2.f,
								  Maxf(2.f, display_size.x - K::W - 2.f));
			g_PanelPos.y = Clampf(K::PanelY, 2.f,
								  Maxf(2.f, display_size.y - K::H - 2.f));
		}

		DrawFrameAndAnimate(display_size);

	}

	Cost cost()
	{
		return g_cost;
	}

	double cost_take_worst_panel()
	{
		const double worst = g_worst_panel_ms;
		g_worst_panel_ms = 0.0;
		return worst;
	}

	Material material()
	{

		Material m;
		m.surface      = g_T.windowBg;
		m.card         = g_T.topBarBg;
		m.well         = IM_COL32(0, 0, 0, 46);
		m.hairline     = g_T.windowBorder;
		m.rule         = g_T.hairline;
		m.text         = g_T.textPrimary;
		m.textDim      = g_T.textDim;
		m.textFaint    = g_T.textFaint;
		m.accent       = g_T.pillActive;
		m.roundWindow  = g_T.roundWindow;
		m.roundControl = g_T.roundControl;
		return m;
	}
}
