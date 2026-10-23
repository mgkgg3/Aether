
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
#include <features/misc/UiPack21.h>

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

// The panel's own metrics.
//
// They are `inline float`s rather than `constexpr` because Enhance scales the
// whole layout: `apply_scale` rewrites every one of them from its base value
// (below) once a frame, and the layout reads them exactly as it always did, so
// a bigger panel needs no second code path and no per-site scaling.
namespace K
{
    namespace base
    {
        // Narrower than it was: the reference this is built against is a tall
        // panel, not a wide one, and every row here is a name, a caption and a
        // control - which reads better on a shorter line.  The height carries
        // the extra type instead of the width.  The width is a setting on top
        // of this (Appearance > Surface > Width).
        inline constexpr float W = 900.f;
        inline constexpr float H = 630.f;

        inline constexpr float PanelY = 90.f;
        inline constexpr float CardX  = 14.f, CardY = 12.f;
        inline constexpr float CardW  = 300.f, CardH = 84.f;

        inline constexpr float RoundWindow = 22.f;

        inline constexpr float Pad = 16.f;
        inline constexpr float TopBarH = 44.f;
        inline constexpr float GapTopBody = 14.f;
        inline constexpr float SidebarW = 150.f;
        inline constexpr float DividerGap = 12.f;
        inline constexpr float ContentPadTop = 2.f;
        inline constexpr float ScrollReserve = 10.f;

        // The content child's own insets: the gap after the divider, and the
        // padding its first column starts from.  They are named here because
        // Test Enhance takes them in with the rest of the left side.
        inline constexpr float ContentInset = 2.f;
        inline constexpr float ContentPadX  = 14.f;

        inline constexpr float TabH = 38.f;
        inline constexpr float TabGap = 5.f;
        inline constexpr float TabInset = 6.f;

        // The icon and the two offsets into a tab, so the nav grows as one
        // piece: a taller row holding a fixed 16 px icon and a fixed text column
        // reads as a smaller nav, not a bigger one.
        inline constexpr float TabIcon  = 16.f;
        inline constexpr float TabIconX = 21.f;
        inline constexpr float TabTextX = 39.f;

        inline constexpr float RowPad = 5.f;
        inline constexpr float RowGap = 5.f;

        // A switch is a target before it is a decoration: 48 x 27 is the size a
        // thumb can find without looking, and it still leaves the row's text
        // the room it had.
        inline constexpr float ToggleW = 48.f, ToggleH = 27.f;
        inline constexpr float ComboW = 158.f, ComboH = 30.f;
        inline constexpr float KeybindW = 150.f, KeybindH = 34.f;
        inline constexpr float SliderTrackH = 8.f, SliderThumbD = 18.f;
        inline constexpr float SwatchD = 30.f;
        inline constexpr float AvatarD = 26.f;

        inline constexpr float PreviewW = 280.f, PreviewH = 330.f;

        inline constexpr float TextSize = 13.f, TextSmall = 11.f, TitleSize = 15.f;

        inline constexpr float UiAnimSpeed = 18.f;

        // Enhance's own step: how much bigger the roomy layout is at scale 1.
        inline constexpr float EnhW = 1.04f;
        inline constexpr float EnhH = 1.06f;
    }

    inline float W = base::W;
    inline float H = base::H;

    inline float PanelY = base::PanelY;
    inline float CardX = base::CardX, CardY = base::CardY;
    inline float CardW = base::CardW, CardH = base::CardH;

    inline float RoundWindow = base::RoundWindow;

    inline float Pad = base::Pad;
    inline float TopBarH = base::TopBarH;
    inline float GapTopBody = base::GapTopBody;
    inline float SidebarW = base::SidebarW;
    inline float DividerGap = base::DividerGap;
    inline float ContentPadTop = base::ContentPadTop;
    inline float ScrollReserve = base::ScrollReserve;
    inline float ContentInset = base::ContentInset;
    inline float ContentPadX  = base::ContentPadX;

    inline float TabH = base::TabH;
    inline float TabGap = base::TabGap;
    inline float TabInset = base::TabInset;
    inline float TabIcon  = base::TabIcon;
    inline float TabIconX = base::TabIconX;
    inline float TabTextX = base::TabTextX;

    inline float RowPad = base::RowPad;
    inline float RowGap = base::RowGap;

    inline float ToggleW = base::ToggleW, ToggleH = base::ToggleH;
    inline float ComboW = base::ComboW, ComboH = base::ComboH;
    inline float KeybindW = base::KeybindW, KeybindH = base::KeybindH;
    inline float SliderTrackH = base::SliderTrackH, SliderThumbD = base::SliderThumbD;
    inline float SwatchD = base::SwatchD;
    inline float AvatarD = base::AvatarD;

    inline float PreviewW = base::PreviewW, PreviewH = base::PreviewH;

    inline float TextSize = base::TextSize, TextSmall = base::TextSmall, TitleSize = base::TitleSize;

    inline float UiAnimSpeed = base::UiAnimSpeed;

    // `s` is the whole scale (1.0 is the layout unchanged), `enh` is how far
    // into Enhance the surface is, and `left` is how far into Test Enhance it
    // is.  They are separate on purpose: `s` is the size, `enh` is the shape -
    // the controls a step smaller, the rows a step closer - and `left` only
    // takes in the left column, which is the dead space Test Enhance is about.
    inline void apply_scale(float s, float enh, float left = 0.f)
    {
        const float sw = s * (1.f + (base::EnhW - 1.f) * (s > 1.f ? 1.f : 0.f));
        const float sh = s * (1.f + (base::EnhH - 1.f) * (s > 1.f ? 1.f : 0.f));

        // Enhance is the tight surface, so the controls come in a step and the
        // rows sit a step closer.  It is applied here rather than as another
        // size, so the two stay independent: the size slider scales the whole
        // panel, and Enhance only changes how much room its own parts take
        // inside it.
        const float tight = 1.f - 0.10f * enh;
        const float close = 1.f - 0.18f * enh;

        // The parts Enhance draws *up* rather than in: a switch, a slider and the
        // nav are what a hand aims at, and at the compact size they are the
        // smallest targets on the surface.  They step up while the rest steps in
        // - by a little, because this is a nudge to the controls and not a second
        // layout - and the rows grow with them, since every row measures itself
        // from its control.
        const float big = 1.f + 0.07f * enh;

        // The width has its own control, independent of the overall scale: a
        // narrower panel is the same panel with less room in the content
        // column, not a shrunken one, so the type and the controls keep their
        // size while the line length comes in.
        const float wide = settings::appearance::panel_width < 0.82f ? 0.82f
                         : (settings::appearance::panel_width > 1.18f ? 1.18f
                                                                     : settings::appearance::panel_width);

        W = base::W * sw * wide;
        H = base::H * sh;

        PanelY = base::PanelY;
        CardX = base::CardX * s;  CardY = base::CardY * s;
        CardW = base::CardW * s;  CardH = base::CardH * s;

        RoundWindow = base::RoundWindow * s;

        // The left side.  Test Enhance takes it in in three places at once and
        // by different amounts, because the space is not in one place: the pad
        // is a frame around the whole plate (so it moves least, or the top bar
        // and the nav would stop lining up), the nav column is mostly empty to
        // the right of its captions (so it moves most), and the divider gap and
        // the content's own inset are pure gutter between the two columns.
        const float in_left  = 1.f - 0.22f * left;   // the plate's frame
        const float in_nav   = 1.f - 0.15f * left;   // the nav column
        const float in_gap   = 1.f - 0.45f * left;   // the gutter
        const float in_inset = 1.f - 0.30f * left;   // the content's own inset

        Pad          = base::Pad * s * (1.f - 0.06f * enh) * in_left;
        TopBarH      = base::TopBarH * s * (1.f - 0.06f * left);
        GapTopBody   = base::GapTopBody * s * (1.f - 0.30f * left);
        SidebarW     = base::SidebarW * s * in_nav;
        DividerGap   = base::DividerGap * s * in_gap;
        ContentPadTop= base::ContentPadTop * s * (1.f - 0.40f * left);
        ScrollReserve= base::ScrollReserve * s;
        ContentInset = base::ContentInset * s * in_inset;
        ContentPadX  = base::ContentPadX * s * in_inset;

        // The nav steps up with the controls rather than in with the plates: it
        // is a column of targets too, and it was the one part of the surface that
        // did not move when the switches did.
        TabH     = base::TabH * s * big * (1.f - 0.03f * left);
        TabGap   = base::TabGap * s * close * (1.f - 0.20f * left);
        TabInset = base::TabInset * s;
        TabIcon  = base::TabIcon * s * big;
        // The icon and the caption sit closer to the pill's edge when the nav is
        // narrower, so the row keeps its own balance instead of hugging the
        // right of a column it no longer fills.
        TabIconX = base::TabIconX * s * big * (1.f - 0.10f * left);
        TabTextX = (base::TabTextX * s * big - 2.f * left);

        // Rows.  Test Enhance is the compact surface, so the row's own padding
        // and the gap under it come in by more than the controls do: the row is
        // where the air is, and the controls are what has to stay hittable.
        RowPad = base::RowPad * s * close * (1.f - 0.20f * left);
        RowGap = base::RowGap * s * close * (1.f - 0.25f * left);

        ToggleW = base::ToggleW * s * big;    ToggleH = base::ToggleH * s * big;
        ComboW  = base::ComboW * s;           ComboH  = base::ComboH * s * tight * (1.f - 0.14f * left);
        KeybindW= base::KeybindW * s;         KeybindH= base::KeybindH * s * tight * (1.f - 0.14f * left);
        // The slider is a bar, not a rule: the reference's tracks are a solid
        // capsule with a knob as big as the row's text, and taking them in with
        // the rest of the compact surface was shrinking them into hairlines.
        // They hold their size (a few percent off at most) so the track stays
        // readable and the knob stays a target at any scale.
        {
            // Maxf() is declared below this (it is the drawing helpers' float
            // math), so the floor is a plain compare here.
            const float trk = base::SliderTrackH * s * big * (1.f - 0.08f * left);
            const float thb = base::SliderThumbD * s * big * (1.f - 0.08f * left);
            SliderTrackH = trk < 6.f ? 6.f : trk;
            SliderThumbD = thb < 14.f ? 14.f : thb;
        }
        SwatchD = base::SwatchD * s * tight;
        AvatarD = base::AvatarD * s * tight;

        PreviewW = base::PreviewW * s;
        PreviewH = base::PreviewH * s;

        UiAnimSpeed = base::UiAnimSpeed;
    }
}

// How far into Enhance the surface is, 0 to 1, animated rather than switched:
// toggling the option glides the panel to its new size and radius over a few
// frames instead of snapping, which is the difference between "bigger" and
// "bigger, smoothly".
static float g_Enhance = 0.f;

static inline float Enh() { return g_Enhance; }

// Every radius the surface uses, plus the size it is drawn at, blended by the
// animation above.  Read by ApplyStyle and by anything that draws its own
// rounding, so a half-way frame is half-way rounded rather than mismatched -
// and it rounds *out* with Enhance by the same step as every other radius in
// the surface, so a plate, a pill, a control, a switch and a popup are one
// family rather than four.
static inline float EnhRadius(float compact) { return compact * (1.f + 0.18f * g_Enhance); }

// The panel is not teleported to the cursor: it is pulled there.
//
//  * `g_PanelPos` is where it is *drawn*, and it is the only one the capture
//    region, the hit tests and the draws read;
//  * `g_PanelTarget` is where the drag wants it - the cursor minus however
//    much of the panel was grabbed, so the drag still moves it 1:1 in intent;
//  * `g_PanelVel` is the spring's own velocity, in pixels per second.
//
// A spring between the two is what "smooth and magnetic" is: the panel starts
// moving a hair after the hand does, glides instead of stepping, and - because
// the target is pulled onto an edge when it comes near one - arrives flush
// against the screen edge rather than two pixels short of it.
static ImVec2 g_PanelPos    = ImVec2(-1.f, -1.f);
static ImVec2 g_PanelTarget = ImVec2(-1.f, -1.f);
static ImVec2 g_PanelVel    = ImVec2(0.f, 0.f);
static ImVec2 g_PanelGrab   = ImVec2(0.f, 0.f);
static bool   g_PanelDrag   = false;
// Placed on the first frame that draws it, because the corner it belongs in
// depends on the display: -1 in y is "not placed yet".
static ImVec2 g_CardPos  = ImVec2(K::CardX, -1.f);

static int GlassAlpha()
{
    // No Transparency is the whole panel at full alpha: the theme's own dark
    // fill, with nothing of the desktop behind it showing through.  It is also
    // what stops the capture - an opaque plate has nothing to sample.
    if (settings::appearance::no_transparency)
        return 255;

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

// A surface fill that is a translucent white wash over the glass.
//
// With No Transparency on there is nothing to wash over, and a white wash over
// a black plate is *grey* - which is the whole of "it still has a grey look".
// The same surface comes back as a solid slate shade instead, so the plate, the
// top bar, the cards, the wells and the pills keep their step apart from one
// another without a grey pixel between them.  `white_alpha` is what the glass
// uses, `slate` the solid the slate uses for the same surface.
// Blend two packed colours.  The theme is built before Mix() is declared (that
// one also handles alpha separately), so this is its own small helper.
static inline ImU32 Mix32(ImU32 a, ImU32 b, float t)
{
    t = ImClamp(t, 0.f, 1.f);
    const int ar = (int)((a >> IM_COL32_R_SHIFT) & 0xFF), br = (int)((b >> IM_COL32_R_SHIFT) & 0xFF);
    const int ag = (int)((a >> IM_COL32_G_SHIFT) & 0xFF), bg = (int)((b >> IM_COL32_G_SHIFT) & 0xFF);
    const int ab = (int)((a >> IM_COL32_B_SHIFT) & 0xFF), bb = (int)((b >> IM_COL32_B_SHIFT) & 0xFF);
    const int aa = (int)((a >> IM_COL32_A_SHIFT) & 0xFF), ba = (int)((b >> IM_COL32_A_SHIFT) & 0xFF);
    return IM_COL32((int)(ar + (br - ar) * t), (int)(ag + (bg - ag) * t),
        (int)(ab + (bb - ab) * t), (int)(aa + (ba - aa) * t));
}

static inline ImU32 SlateFill(float white_alpha, int slate)
{
    if (settings::appearance::no_transparency)
        return IM_COL32(slate, slate, slate + 3, 255);
    return IM_COL32(255, 255, 255, (int)white_alpha);
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

    // How far the custom accent is allowed to reach.  Only a custom accent has
    // a spread: the stock palettes are already balanced around their own
    // accent, and tinting their text would just be a second palette.
    const float sp = settings::appearance::custom_accent
        ? ImClamp(settings::appearance::accent_spread, 0.f, 1.f) : 0.f;

    g_T.windowBg       = solid(bg, GlassAlpha());

    // The outline and the bar's rule stay neutral at every spread.
    //
    // These are the panel's *edges*, and an edge that takes the accent is a
    // line drawn round the window in the accent colour: at full spread it was
    // a bright hairline above the content and around the plate, which is the
    // "lines with the glow" the spread slider produced.  The spread is the
    // light on the plate, not a pen; the bar keeps its accent wash, the lines
    // stay white and faint.
    g_T.windowBorder   = tint(stroke, bg, 0.55f, 24);
    g_T.topBarBg       = tint(surface, accent, 0.10f + 0.50f * sp, (int)(8.f + 46.f * sp));
    g_T.topBarBorder   = tint(stroke, bg, 0.55f, 14);
    g_T.sidebarBg      = IM_COL32(0, 0, 0, 0);

    // Rules and hovers stay neutral at every spread.
    //
    // They are the lines the eye uses to separate groups, not part of the
    // accent: tinting them is what turned a hairline into a coloured glow that
    // ran the width of the page.  The accent still reaches the top bar, the
    // type and every control, which is where it belongs.
    g_T.divider        = tint(stroke, bg, 0.40f, 15);
    g_T.hairline       = tint(stroke, bg, 0.40f, 11);
    g_T.rowHover       = tint(surface, accent, 0.10f, 12);
    g_T.glow           = solid(accent, 255);
    g_T.popupBg        = tint(bg, surface, 0.50f, 196);

    // Text follows the spread only far enough to read as tinted, never far
    // enough to cost contrast.
    //
    // The reach is weighted by how bright the accent is: a saturated blue or a
    // near-black red mixed into 14 px type at a third of its value is the
    // "the text looks odd" that came back from the colour picker.  A bright
    // accent can afford to show; a dark one barely touches the type at all.
    const float accent_luma = 0.2126f * accent[0] + 0.7152f * accent[1] + 0.0722f * accent[2];
    const float text_reach  = (0.06f + 0.14f * ImClamp(accent_luma, 0.f, 1.f)) * sp;

    g_T.textPrimary    = Mix32(solid(text, 255), solid(accent, 255), text_reach);
    g_T.textDim        = Mix32(solid(dim, 255), solid(accent, 255), text_reach * 1.7f);
    g_T.textFaint      = Mix32(tint(dim, bg, 0.35f, 255), solid(accent, 255), text_reach * 1.4f);
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

    // No Transparency is a different material, not the same one at full alpha.
    //
    // Every surface above is a translucent wash of white over the glass, which
    // is what gives the panel its structure while the desktop shows through.
    // With the glass off there is nothing to wash over: an 8-alpha top bar over
    // an opaque plate is invisible, and the panel reads as one flat grey slab -
    // the "grey, not black" it was called for.  These are the same surfaces as
    // *slate*: solid shades of near-black, a step apart, so the plate, the top
    // bar, the docks and the wells still read as separate slabs.
    if (settings::appearance::no_transparency)
    {
        // The plate: the reference's grey-to-black blend, not a flat slab.  The
        // surface is a dark grey that goes black towards the bottom (the blend
        // itself is drawn in DrawMenu), the bar is a clear step up from it, and
        // the cards sit between the two.  The earlier near-black plate with a
        // lit top edge read as a cut-out with a highlight painted on it; a
        // blended grey needs no highlight to look like a surface.
        //
        // The contrast slider is the escape hatch: 1.0 is the reference grey and
        // higher values lift every surface together, so the steps stay steps.
        //
        // The numbers are read off the reference: the plate runs #1a1a1a at the
        // top to #111111 at the bottom (the fall is the blend in DrawMenu, this
        // is the value it lands on), the bar is #303030, and the slider's
        // unfilled groove is barely a step above the plate - in the reference it
        // is the same value as the plate and only the filled part shows.
        const float c = ImClamp(settings::appearance::panel_contrast, 0.4f, 2.0f);
        const int   v  = (int)(17.f * c + 0.5f);
        const int   vs = (int)(24.f * c + 0.5f);

        // The bar: one clear step up from the plate, the way the reference has
        // it (#303030 over #1a1a1a).
        const int   vb = (int)(48.f * c + 0.5f);

        const ImU32 acc = solid(accent, 255);

        g_T.windowBg        = IM_COL32(v, v, v + 2, 255);
        g_T.windowBorder    = IM_COL32(255, 255, 255, 22);

        // The top bar is the one surface that is *lighter* than the plate - it
        // is the reference's own relationship (the bar is a raised plate).  The
        // accent spread is what lifts it off neutral and into the accent's hue.
        g_T.topBarBg        = Mix32(IM_COL32(vb, vb, vb + 4, 255), acc, 0.22f * sp);
        g_T.topBarBorder    = IM_COL32(255, 255, 255, 14);
        g_T.sidebarBg       = IM_COL32(0, 0, 0, 0);

        g_T.divider         = IM_COL32(255, 255, 255, 17);
        g_T.hairline        = IM_COL32(255, 255, 255, 13);
        g_T.rowHover        = Mix32(IM_COL32(255, 255, 255, 12), acc, 0.16f * sp);
        g_T.popupBg         = IM_COL32(34, 34, 37, 252);

        // Controls are *recesses* on this material, not raised grey pills: the
        // fill goes a step below the plate and the border does the reading.
        g_T.controlBg       = IM_COL32(v - 4, v - 4, v - 2, 255);
        g_T.controlBorder   = IM_COL32(255, 255, 255, 30);
        g_T.controlBorderHi = solid(accent, 120);
        g_T.toggleOff       = IM_COL32(v + 3, v + 3, v + 6, 255);
        g_T.sliderTrack     = IM_COL32(vs, vs, vs + 3, 255);
        g_T.textFaint       = Mix32(solid(dim, 255), acc, 0.30f * sp);
    }
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
    s.ItemSpacing         = ImVec2(8.f + 3.f * g_Enhance, 5.f + 2.f * g_Enhance);
    s.AntiAliasedLines    = true;
    s.AntiAliasedFill     = true;
    // Lines from the atlas' own filtered texture rather than four alpha-blended
    // quads per segment.  The panel is built out of 1px rules - the top bar's
    // edge, the nav's divider, the row separators - and on the solid plate those
    // rules are most of the structure the eye has, so they get the smoother of
    // the two paths.
    s.AntiAliasedLinesUseTex = true;
    s.Alpha               = 1.f;

    // Curves.  The stock tessellation is tuned for small widgets at small radii;
    // the panel draws rounded rectangles and rings far larger than that, and a
    // coarse tolerance is exactly the faceting that shows up on a big radius - so
    // it comes down, and the circle error with it.  Enhance buys a little more,
    // and the overlay's own anti-aliasing buys more again: it is one option over
    // every curve in the frame, the overlay's ring included, so a big circle
    // stops showing its facets as well as its steps.
    const float aa_q = settings::appearance::antialiasing
        ? (settings::appearance::aa_quality < 0.5f ? 0.5f
          : (settings::appearance::aa_quality > 2.f ? 2.f : settings::appearance::aa_quality))
        : 0.f;
    const float aa_fine = 1.f - 0.30f * aa_q;      // 1 with the option off
    s.CurveTessellationTol       = (0.55f - 0.30f * g_Enhance) * aa_fine;
    s.CircleTessellationMaxError = (0.20f - 0.10f * g_Enhance) * aa_fine;
    s.ScrollbarSize        = 6.f + 2.f * g_Enhance;
    s.ScrollbarRounding    = 6.f + 5.f * g_Enhance;

    s.Colors[ImGuiCol_WindowBg]      = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_Border]        = ImGui::ColorConvertU32ToFloat4(g_T.windowBorder);
    s.Colors[ImGuiCol_ChildBg]       = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_PopupBg]       = ImGui::ColorConvertU32ToFloat4(g_T.popupBg);
    s.Colors[ImGuiCol_ScrollbarBg]   = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1, 1, 1, 0.12f);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1, 1, 1, 0.20f);
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(1, 1, 1, 0.28f);
    s.Colors[ImGuiCol_Text]          = ImGui::ColorConvertU32ToFloat4(g_T.textPrimary);

    // Radii.  Enhance moves every one of them by the same step, in the same
    // direction, because that is what a surface language is: the plate, the top
    // bar, the nav pill, the rows, the controls, the switches and the popups all
    // round *out* together, and a half-way frame is half-way rounded rather than
    // snapped.
    //
    // It used to move them per part - the plates *in* a step, the controls and
    // popups *out* a step - which is exactly the "the rounding is not
    // consistent" it read as: a tab strip rounded like a capsule sitting on
    // plates that had gone squarer, and a keybind rounder than the row it sat
    // in.  One step, one family.
    const float er = 1.f + 0.18f * g_Enhance;
    g_T.roundWindow    = K::RoundWindow * er;
    g_T.roundTopBar    = 13.f * er;
    g_T.roundPill      = 11.f * er;
    g_T.roundRow       = 9.f * er;
    g_T.roundControl   = 9.f * er;
    g_T.roundPopup     = 11.f * er;
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

// The panel opens like a bubble: it grows out of a slightly smaller copy of
// itself, overshoots by a couple of percent, and settles.  The damping is low
// on purpose - a critically damped spring arrives without ever having been
// bigger, which reads as a fade with a size change rather than as something
// popping open.  Closing uses the same shape in reverse, quicker and with less
// bounce, so putting it away does not bounce at the user.
static const float kBubbleClosed   = 0.75f;
static const float kBubbleResponse = 0.34f;
static const float kBubbleDamping  = 0.45f;

static inline float BubbleAlpha(float t) { t = Clampf(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }

static inline float Approach(float cur, float target, float speed, float dt)
{
    return cur + (target - cur) * (1.f - expf(-speed * dt));
}

// How far into Test Enhance the surface is, 0 to 1, animated for the same
// reason Enhance is: a panel that jumps 6% smaller when a switch is pressed
// reads as a glitch, one that settles into it reads as a size.
static float g_Tight = 0.f;

// How far into Test Enhance the surface is, for the layout code in SyncPages.h:
// the compact pass is driven from here so a page can tighten its own spacing
// without a second set of metric constants.
static inline float Tight() { return g_Tight; }

// How much smaller the panel is at the far end of Test Enhance.  Two steps down
// is enough to feel (the rows sit closer and the left column loses its gutter)
// and small enough that nothing measured from the panel's own width has to be
// re-tuned: every metric is scaled, so the layout is the same layout.
static const float kTightScale = 0.92f;

// Drive the Enhance animation, then rebuild the layout metrics from it.
//
// One call a frame, before the style is built: the surface the style describes
// (radii, spacing, tessellation) and the metrics the layout uses both come from
// `g_Enhance`, so the panel cannot be half scaled and half sorted for a frame.
static void EnhanceTick(float dt)
{
    const float want = settings::appearance::enhance ? 1.f : 0.f;

    g_Enhance = Approach(g_Enhance, want, 7.5f, dt);
    if (ImFabs(want - g_Enhance) < 0.002f)
        g_Enhance = want;

    const float want_tight = settings::appearance::test_enhance ? 1.f : 0.f;
    g_Tight = Approach(g_Tight, want_tight, 9.f, dt);
    if (ImFabs(want_tight - g_Tight) < 0.002f)
        g_Tight = want_tight;

    // The panel's own size is the product of the two: Enhance's size slider
    // (1 when it is off) and Test Enhance's single step in.
    K::apply_scale(
        Lerpf(1.f, Clampf(settings::appearance::enhance_size, 0.94f, 1.24f), g_Enhance) *
            Lerpf(1.f, kTightScale, g_Tight),
        g_Enhance, g_Tight);
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

// A vertical gradient that follows a rounded rect instead of filling a
// rectangle.
//
// AddRectFilledMultiColor draws four corners and four straight edges: two of
// them are the gradient's ends, and the other two are hard steps wherever the
// rect stops short of the shape it is meant to fill.  Inset it by the corner
// radius to stay inside the curve and those steps become two vertical seams
// running the height of the plate - the "square" a gradient over a rounded
// surface looks like when it is made of one quad.  This walks the rounded path
// and colours every vertex by its own y, so the fall is one smooth field over
// the whole shape and has no edges to leave.  The alpha is multiplied by the
// coverage ImGui already wrote, which keeps the anti-aliased rim intact.
//
// It is the same technique the reference player uses for its surface.
// The same walk, with a middle stop.  Used for anything that has to be *soft at
// both ends* - a row's hover wash, for instance: a flat rounded fill at twelve
// alpha still has a one-pixel anti-aliased edge along its top and bottom, and on
// a dark plate two of those per row are the thin bright rules the settings page
// was reading as "lines with the glow".  Zero at both ends has no edge to see.
static void GradV3(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float rounding,
                   ImU32 top, ImU32 mid, ImU32 bot)
{
    if (!dl || p1.y - p0.y < 0.5f || p1.x - p0.x < 0.5f)
        return;

    const int r0 = (int)((top >> IM_COL32_R_SHIFT) & 0xFF), r1 = (int)((mid >> IM_COL32_R_SHIFT) & 0xFF), r2 = (int)((bot >> IM_COL32_R_SHIFT) & 0xFF);
    const int g0 = (int)((top >> IM_COL32_G_SHIFT) & 0xFF), g1 = (int)((mid >> IM_COL32_G_SHIFT) & 0xFF), g2 = (int)((bot >> IM_COL32_G_SHIFT) & 0xFF);
    const int b0 = (int)((top >> IM_COL32_B_SHIFT) & 0xFF), b1 = (int)((mid >> IM_COL32_B_SHIFT) & 0xFF), b2 = (int)((bot >> IM_COL32_B_SHIFT) & 0xFF);
    const int a0 = (int)((top >> IM_COL32_A_SHIFT) & 0xFF), a1 = (int)((mid >> IM_COL32_A_SHIFT) & 0xFF), a2 = (int)((bot >> IM_COL32_A_SHIFT) & 0xFF);

    const int vtx0 = dl->VtxBuffer.Size;
    dl->PathRect(p0, p1, ImMin(rounding, ImMin((p1.x - p0.x), (p1.y - p0.y)) * 0.5f));
    dl->PathFillConvex(IM_COL32_WHITE);

    const float h = p1.y - p0.y;
    for (int i = vtx0; i < dl->VtxBuffer.Size; ++i)
    {
        ImDrawVert& v = dl->VtxBuffer[i];
        const float f = ImClamp((v.pos.y - p0.y) / h, 0.f, 1.f);
        const float u = f < 0.5f ? f * 2.f : (f - 0.5f) * 2.f;
        const int r = f < 0.5f ? (int)(r0 + (r1 - r0) * u) : (int)(r1 + (r2 - r1) * u);
        const int g = f < 0.5f ? (int)(g0 + (g1 - g0) * u) : (int)(g1 + (g2 - g1) * u);
        const int b = f < 0.5f ? (int)(b0 + (b1 - b0) * u) : (int)(b1 + (b2 - b1) * u);
        const int av = f < 0.5f ? (int)(a0 + (a1 - a0) * u) : (int)(a1 + (a2 - a1) * u);
        const int cover = (int)((v.col >> IM_COL32_A_SHIFT) & 0xFF);
        const int a = (int)(av * (float)cover / 255.f + 0.5f);
        v.col = IM_COL32(r, g, b, a);
    }
}

static void GradV(ImDrawList* dl, ImVec2 p0, ImVec2 p1, float rounding, ImU32 top, ImU32 bot)
{
    if (!dl || p1.y - p0.y < 0.5f || p1.x - p0.x < 0.5f)
        return;

    const int r0 = (int)((top >> IM_COL32_R_SHIFT) & 0xFF), r1 = (int)((bot >> IM_COL32_R_SHIFT) & 0xFF);
    const int g0 = (int)((top >> IM_COL32_G_SHIFT) & 0xFF), g1 = (int)((bot >> IM_COL32_G_SHIFT) & 0xFF);
    const int b0 = (int)((top >> IM_COL32_B_SHIFT) & 0xFF), b1 = (int)((bot >> IM_COL32_B_SHIFT) & 0xFF);
    const int a0 = (int)((top >> IM_COL32_A_SHIFT) & 0xFF), a1 = (int)((bot >> IM_COL32_A_SHIFT) & 0xFF);

    const int vtx0 = dl->VtxBuffer.Size;
    const int idx0 = dl->IdxBuffer.Size;
    dl->PathRect(p0, p1, ImMin(rounding, ImMin((p1.x - p0.x), (p1.y - p0.y)) * 0.5f));
    dl->PathFillConvex(IM_COL32_WHITE);

    const float h = p1.y - p0.y;
    for (int i = vtx0; i < dl->VtxBuffer.Size; ++i)
    {
        ImDrawVert& v = dl->VtxBuffer[i];
        const float f = ImClamp((v.pos.y - p0.y) / h, 0.f, 1.f);
        const int cover = (int)((v.col >> IM_COL32_A_SHIFT) & 0xFF);
        const int a = (int)((a0 + (a1 - a0) * f) * (float)cover / 255.f + 0.5f);
        v.col = IM_COL32((int)(r0 + (r1 - r0) * f), (int)(g0 + (g1 - g0) * f),
                         (int)(b0 + (b1 - b0) * f), a);
    }

    (void)idx0;
}

// The second line of a row - a description, a note, a section's caption - is
// read, not looked at, so it sits one step below the name and not two.
// textFaint is the colour for things that are *not* meant to be read (a
// placeholder, a disabled row), and small text in it is the "can't read that"
// half of a compact layout: at 11px the faint mix is below the contrast where a
// hinted glyph can hold its shape, so it turns into grey mush.  A step up from
// dim keeps the hierarchy and makes the line legible.
// Only under Test Enhance: the compact surface is the one that asked for it,
// and the roomy layout keeps the contrast it was designed with.
static inline ImU32 SubText()
{
    return Tight() > 0.5f ? GC(Mix(g_T.textDim, g_T.textPrimary, 0.30f)) : GC(g_T.textDim);
}

// A note is a step below a description: it is a caption for a group, not a
// second line of the row above it.  In the compact layout it moves down to the
// description's own colour, because textFaint at 12px on a solid plate is below
// the contrast where a hinted glyph holds its shape.
static inline ImU32 NoteText()
{
    return Tight() > 0.5f ? SubText() : GC(g_T.textFaint);
}

// Every second line is capped at eight words.
//
// The panel's hierarchy is a name and, at most, one short caption.  A sentence
// under a row is a paragraph in a place that has no room for one, and three of
// them in a group is what makes a page read as busy.  Longer strings are cut at
// the eighth word rather than wrapped, so a row's height never depends on how
// much prose it was given.
static inline const char* Cap8(const char* t)
{
    if (!t || !t[0])
        return t;

    int words = 0;
    bool in_word = false;
    size_t cut = 0;
    for (size_t i = 0; t[i]; ++i)
    {
        const bool space = (t[i] == ' ');
        if (!space && !in_word)
        {
            if (++words == 9) { cut = i; break; }
            in_word = true;
        }
        else if (space)
        {
            in_word = false;
        }
    }
    if (words <= 8)
        return t;

    static char buf[8][192];
    static int slot = 0;
    char* out = buf[slot];
    slot = (slot + 1) % 8;

    size_t n = cut < sizeof(buf[0]) - 4 ? cut : sizeof(buf[0]) - 4;
    while (n > 0 && (t[n - 1] == ' ' || t[n - 1] == ',' || t[n - 1] == ';'))
        --n;
    memcpy(out, t, n);
    out[n] = '.'; out[n + 1] = '.'; out[n + 2] = '.'; out[n + 3] = 0;
    return out;
}

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

// A one-pixel drop shadow under every glyph keeps the menu readable over a live
// game behind glass.  On the solid plate there is nothing to read it against,
// and what it does instead is put a second, softer copy of every letter one
// pixel down - which is the difference between crisp small type and type that
// looks out of focus.  So it follows the material, not the panel: glass gets it,
// the solid plate does not.
static inline float TextShadow()
{
    // Ten, down from sixteen.  On the glass the shadow earns its place - it is
    // what keeps a caption off a bright frame of game - but the second copy at
    // sixteen hundredths is enough to soften the edge of a twelve pixel glyph,
    // and "don't blend too much" is that edge.  One pixel down, faint.
    return settings::appearance::no_transparency ? 0.f : 0.10f;
}

static inline void   Txt(ImDrawList* dl, ImFont* f, ImVec2 p, ImU32 col, const char* t)
{
    const ImVec2 sp = SnapText(p);
    if (TextShadow() > 0.f)
    {
        const float a = (float)((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
        dl->AddText(f, f->FontSize, ImVec2(sp.x, sp.y + 1.f),
                    GC(IM_COL32(0, 0, 0, (int)(255.f * TextShadow() * a))), t);
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

// UTF-8-safe truncation with an ellipsis.  The sidebar's name and handle are
// user content, so a per-byte cut could split a glyph; they are also the one
// pair of strings with a fixed column and no room to grow.
static void TxtClipped(ImDrawList* dl, ImFont* f, ImVec2 p, ImU32 col, const char* t, float max_w)
{
    if (!f || !t || !t[0] || max_w <= 1.f)
        return;

    const float fs = f->FontSize;
    if (f->CalcTextSizeA(fs, FLT_MAX, 0.f, t).x <= max_w)
    {
        dl->AddText(f, fs, p, col, t);
        return;
    }

    int cut = (int)strlen(t);
    while (cut > 0)
    {
        --cut;
        while (cut > 0 && ((unsigned char)t[cut] & 0xC0) == 0x80)
            --cut;

        char probe[512];
        if ((size_t)cut >= sizeof(probe) - 4)
            continue;
        memcpy(probe, t, (size_t)cut);
        memcpy(probe + cut, "...", 4);
        if (f->CalcTextSizeA(fs, FLT_MAX, 0.f, probe).x <= max_w)
        {
            dl->AddText(f, fs, p, col, probe);
            return;
        }
    }
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

    // A switch is a capsule and a thumb, and nothing else.  It used to wear a
    // glow outline, a ripple ring, a ring around the knob, a halo behind it, a
    // second ring and a dark inner ring - five edges for one control, which at
    // 22 px is a white blob with a fur around it.  Hover is a flat lift of the
    // track, the thumb is one disc with one hairline, and that is the whole
    // control.
    // On is the accent, and the thumb flips to the dark side when the accent is
    // bright enough to carry it.  A half-tint of a white accent over a dark
    // track is a grey pill with a white dot on it - two greys a step apart, at
    // 48 x 27, which is not a state you can read across a row.  White track,
    // dark thumb is the same switch the rest of the world draws.
    const int acc_r = (int)((g_T.toggleOn >> IM_COL32_R_SHIFT) & 0xFF);
    const int acc_g = (int)((g_T.toggleOn >> IM_COL32_G_SHIFT) & 0xFF);
    const int acc_b = (int)((g_T.toggleOn >> IM_COL32_B_SHIFT) & 0xFF);
    const float acc_luma = (0.2126f * acc_r + 0.7152f * acc_g + 0.0722f * acc_b) / 255.f;

    ImU32 track = Mix(g_T.toggleOff, g_T.toggleOn, Clampf(t, 0.f, 1.f));
    track = Mix(track, Mix(track, IM_COL32(255, 255, 255, 255), 0.10f), hv);

    dl->AddRectFilled(a, b, GC(track), rad);
    dl->AddRect(a, b, GC(Mix(g_T.controlBorder, g_T.controlBorderHi, Maxf(hv, fb.shine))), rad, 0, g_T.borderThickness);

    const float knobR = (size.y - 8.f) * 0.5f;
    const float kt = Clampf(t, 0.f, 1.f);
    const float knobX = Lerpf(a.x + 4.f + knobR, b.x - 4.f - knobR, kt);
    const ImVec2 kc(knobX, (a.y + b.y) * 0.5f);

    // One soft seat under the thumb, so it reads as sitting in the track rather
    // than as a circle drawn on it, and the disc itself.
    dl->AddCircleFilled(ImVec2(kc.x, kc.y + 0.7f), knobR + 0.4f, GC(IM_COL32(0, 0, 0, 40)), 40);
    // Off: the light thumb.  On: dark if the accent is bright, light if it is
    // not - so the thumb always has a step against the track it sits on.
    const ImU32 knob_on = acc_luma > 0.60f ? IM_COL32(18, 18, 22, 255)
                                           : IM_COL32(250, 250, 252, 255);
    dl->AddCircleFilled(kc, knobR, GC(Mix(g_T.knobOff, knob_on, Clampf(t, 0.f, 1.f))), 40);

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
// A lit top edge that follows the corner it is on.
//
// It used to be one straight line laid across the control, inset by the corner
// radius - which is not an edge of the shape but a rule floating over the
// rounded corners, and at the radii Enhance asks for it reads as a stray line
// sitting on top of every bar.  Clipping to the top of the shape and stroking
// the whole rounded outline instead leaves only the arc that really is the top
// edge, so the highlight bends with the control and the corners stay clean.
static void TopSheen(ImDrawList* dl, ImVec2 a, ImVec2 b, float rad, ImU32 col, float thickness = 1.f)
{
    if (!dl || b.x - a.x < 2.f || b.y - a.y < 4.f)
        return;

    const float half = Maxf(0.5f, thickness * 0.5f);
    dl->PushClipRect(ImVec2(a.x, a.y), ImVec2(b.x, a.y + thickness), true);
    dl->AddRect(ImVec2(a.x + half, a.y + half), ImVec2(b.x - half, b.y - half), col, rad, 0, thickness);
    dl->PopClipRect();
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


    dl->AddRectFilled(a, b, GC(Mix(g_T.controlBg, IM_COL32(255, 255, 255, 40), Maxf(fb.shine, fb.hover) * 0.5f)), rad);
    ImU32 border = Mix(g_T.controlBorder, g_T.controlBorderHi, Maxf(fb.shine * 0.8f, fb.hover * 0.35f));
    dl->AddRect(a, b, GC(border), rad, 0, g_T.borderThickness);

    if (Enh() > 0.01f)
        TopSheen(dl, a, b, rad, GC(IM_COL32(255, 255, 255, (int)(34.f * Enh()))), g_T.borderThickness);

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


    dl->AddRectFilled(a, b, GC(Mix(g_T.controlBg, IM_COL32(255, 255, 255, 34), Maxf(fb.shine, fb.hover) * 0.55f)), rad);
    dl->AddRect(a, b, GC(Mix(g_T.controlBorder, g_T.controlBorderHi, Maxf(fb.shine, fb.hover) * 0.85f)), rad, 0, g_T.borderThickness);

    // A lit top edge inside the control, on the corner rather than across it -
    // and part of the roomy surface, so it stays off with Enhance off.
    if (Enh() > 0.01f)
        TopSheen(dl, a, b, rad, GC(IM_COL32(255, 255, 255, (int)(34.f * Enh()))), g_T.borderThickness);

    // The value and the chevron keep off the corners: the inset follows the
    // radius, so a rounder control is never a crowded one.  At 9 px of corner
    // the old fixed 12 px inset sat the label almost on the curve.
    const float inset = rad * 0.55f + 6.f + 4.f * Enh();

    const char* label = (*current >= 0 && *current < count) ? items[*current] : "";
    Txt(dl, g_FontRegular, ImVec2(a.x + inset, (a.y + b.y) * 0.5f - g_FontRegular->FontSize * 0.5f), GC(g_T.textPrimary), label);
    AddChevron(dl, ImVec2(b.x - inset + 1.f, (a.y + b.y) * 0.5f), 4.5f, 2.8f, GC(g_T.textPrimary), 2.0f);

    // The menu opens with a short fade and a slide, and its rows are taller,
    // rounder and hover-animated under Enhance - the row that is already picked
    // carries an accent bar, so the current value is readable at a glance.
    // Everything about the *list* that is dressing rather than function is part
    // of the roomy surface: with Enhance off it is the plain menu it has always
    // been - flat rows, no accent bar, no slide - and only a row's own hover
    // tint is animated.
    const bool  roomy = Enh() > 0.01f;
    const float ih    = 26.f + 7.f * Enh();
    const float irad  = roomy ? g_T.roundControl * 0.85f : 3.f;
    const float ipad  = 6.f + 4.f * Enh();
    const float gap   = 6.f + 3.f * Enh();

    // One progress value drives the panel, the slide *and* the row cascade, and
    // it lives in this combo's own id scope - not in a file-static shared by
    // every dropdown on the page.  (A shared one is reset by every closed combo
    // in the same frame, so the open list never advanced and drew no rows at
    // all.)
    float& pop = AnimF("##popanim", open ? 1.f : 0.f);
    pop = Approach(pop, open ? 1.f : 0.f, 17.f, ImGui::GetIO().DeltaTime);
    const float pop_a = Maxf(0.04f, pop);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ipad, ipad));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, g_T.roundPopup);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, g_T.borderThickness);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 2.f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, pop_a);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_T.popupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, g_T.windowBorder);

    ImGui::SetNextWindowPos(ImVec2(b.x - (K::ComboW + 14.f) - (roomy ? (1.f - pop) * 6.f : 0.f),
                                   b.y + gap + (roomy ? (1.f - pop) * 9.f : 0.f)), ImGuiCond_Always);
    // The list enters the same way a page does: top to bottom, one row after
    // the next, each rising a few pixels and fading up, on the cascade's own
    // rise.  The page transition itself is untouched - this only reads the same
    // settings, so the two motions belong to one family.
    const float c_rise = ImClamp(settings::misc_ui::cascade_rise, 0.f, 40.f);
    const float c_step = 0.42f / (float)ImMax(1, count - 1);
    const float c_span = 1.f - c_step * (float)(count - 1);

    if (ImGui::BeginPopup("##popup"))
    {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        {
            const ImVec2 pw = ImGui::GetWindowPos();
            const ImVec2 ps = ImGui::GetWindowSize();
            DrawBackdropRect(pdl, pw, ImVec2(pw.x + ps.x, pw.y + ps.y), g_T.roundPopup, g_MenuFade * 255.f);

            if (roomy)
                TopSheen(pdl, pw, ImVec2(pw.x + ps.x, pw.y + ps.y), g_T.roundPopup,
                    GC(IM_COL32(255, 255, 255, (int)(28.f * Enh()))), g_T.borderThickness);
        }
        for (int i = 0; i < count; i++)
        {
            ImGui::PushID(i);

            const float ti = ImClamp((pop - (float)i * c_step) / ImMax(0.15f, c_span), 0.f, 1.f);
            const float ra = SmoothStep(ti);
            const float dy = (1.f - ra) * c_rise;

            if (ra <= 0.002f)
            {
                // Keep the slot so the rows below do not jump up while the ones
                // above them are still arriving, but take no clicks yet.
                ImGui::Dummy(ImVec2(K::ComboW, ih));
                ImGui::PopID();
                continue;
            }

            const ImVec2 base = ImGui::GetCursorScreenPos();
            const ImVec2 p = base + ImVec2(0.f, dy);
            const ImVec2 isz(K::ComboW, ih);
            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton("##item", isz);
            const bool ihov = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) { *current = i; ImGui::CloseCurrentPopup(); }

            float& irv = AnimF("##ihov", 0.f);
            irv = Approach(irv, ihov ? 1.f : 0.f, K::UiAnimSpeed, ImGui::GetIO().DeltaTime);
            const bool picked = (i == *current);
            if (irv > 0.003f)
            {
                pdl->AddRectFilled(p, ImVec2(p.x + isz.x, p.y + isz.y), GC(g_T.rowHover, irv * ra), irad);
                pdl->AddRect(p, ImVec2(p.x + isz.x, p.y + isz.y), GC(g_T.controlBorder, irv * ra), irad, 0, 1.f);
            }
            if (picked && roomy)
            {
                pdl->AddRectFilled(ImVec2(p.x + 3.f, p.y + 4.f),
                    ImVec2(p.x + 5.f, p.y + isz.y - 4.f), GC(g_T.pillActive, ra), 1.f);
            }
            else if (picked)
            {
                // Without the accent bar the picked row still has to be the
                // picked one, so the current value carries a settled tint.
                pdl->AddRectFilled(p, ImVec2(p.x + isz.x, p.y + isz.y), GC(g_T.rowHover, 0.55f * ra), irad);
            }
            Txt(pdl, g_FontRegular, ImVec2(p.x + 12.f + 3.f * Enh(), p.y + (isz.y - g_FontRegular->FontSize) * 0.5f),
                GC(picked ? g_T.textPrimary : g_T.textDim, ra), items[i]);

            // The animation moves the row, not the layout: the next row is laid
            // out from where this one would sit at rest.
            ImGui::SetCursorScreenPos(ImVec2(base.x, base.y + isz.y));
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(5);

    ImGui::PopID();
    return pressed;
}

static float SliderTrackH()
{
    float h = settings::misc_ui::slider_hd
        ? Maxf(2.f, K::SliderTrackH * Clampf(settings::misc_ui::slider_hd_size, 0.5f, 2.f))
        : K::SliderTrackH;

    // Whole pixels on the compact surface: a 3.6 px track lands its two rounded
    // ends on half pixels, and the reference's bar is four pixels flat.
    if (Tight() > 0.5f)
        h = floorf(h + 0.5f);
    return h;
}

static float SliderThumbD()
{
    float d = settings::misc_ui::slider_hd
        ? K::SliderThumbD * Clampf(settings::misc_ui::slider_hd_size, 0.5f, 2.f)
        : K::SliderThumbD;

    if (Tight() > 0.5f)
        d = floorf(d + 0.5f);
    return d;
}

// How far the knob sticks out either side of the track.  It used to be
// reported only for the big-slider option, which was true when the two were
// the same size unless that option changed them; now the knob is always wider
// than the bar, so the row has to reserve the overhang or the thumb of every
// slider laps into the row below it (and the hit box has to grow with it, or
// the edge of a big knob is not grabbable).
static float SliderOverhang()
{
    return Maxf(0.f, (SliderThumbD() - SliderTrackH()) * 0.5f);
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

    // The track grows a pixel under the cursor.  It is the smallest possible
    // acknowledgement and it is what makes a row feel like a control rather than
    // a picture of one: the hit area never moves, only what is drawn.
    const float extra = (hd ? 2.2f : 1.6f) * Maxf(fb.hover, fb.shine);
    const float ty0   = trackY - extra * 0.5f;
    const float ty1   = trackY + trackH + extra * 0.5f;

    // A recess, not a bar: the well first, then one dark lip along its top edge
    // so the track sits *into* the plate.  The padding is what keeps the lip
    // inside the pill's own rounding at any size of Enhance.
    //
    // The compact surface draws the reference's slider instead: a flat capsule
    // and a white knob.  No lip and no gloss - at four pixels the two rules are
    // the track, and a slit of black inside a slit of grey is what makes a
    // compact slider look engraved rather than drawn.
    const bool compact = Tight() > 0.5f;

    dl->AddRectFilled(ImVec2(pos.x, ty0), ImVec2(pos.x + width, ty1),
                      GC(g_T.sliderTrack), rad + extra * 0.5f);
    if (!compact)
        dl->AddRectFilled(ImVec2(pos.x + 1.f, ty0 + 0.5f), ImVec2(pos.x + width - 1.f, ty0 + 1.5f),
                          GC(IM_COL32(0, 0, 0, 40)), rad);

    if (t > 0.004f)
    {
        const float fx = pos.x + width * t;
        dl->AddRectFilled(ImVec2(pos.x, ty0), ImVec2(fx, ty1), GC(g_T.sliderFill), rad + extra * 0.5f);

        // One 1 px gloss along the top of the fill.  A flat accent bar reads as
        // a progress bar from 1998; a lit edge reads as a filled channel.
        if (!compact && fx - pos.x > 4.f)
            dl->AddRectFilled(ImVec2(pos.x + 1.f, ty0 + 0.5f), ImVec2(fx - 1.f, ty0 + 1.5f),
                              GC(IM_COL32(255, 255, 255, 46)), rad);
    }

    // The thumb: a white disc and one hairline.  No glow - a light around a knob
    // is the same halo the plate was just stripped of - and no contact shadow at
    // the compact size either: at a five pixel radius the offset disc is not a
    // shadow, it is a soft edge on the knob, which is the one thing a small
    // control cannot afford.
    const ImVec2 tc(thumbX, cy);
    if (!Tight())
        dl->AddCircleFilled(ImVec2(tc.x, tc.y + (hd ? 1.6f : 1.2f)), thumbR * 0.98f,
                            GC(IM_COL32(0, 0, 0, 54)), segs);
    dl->AddCircleFilled(tc, thumbR, GC(g_T.sliderThumb), segs);
    dl->AddCircle(tc, thumbR - 0.5f, GC(IM_COL32(0, 0, 0, compact ? 34 : 64)), segs, 1.f);

    if (!compact && fb.shine > 0.01f)
        dl->AddCircle(tc, thumbR + 2.6f, GC(g_T.glow, fb.shine * 0.40f), segs, 1.2f);

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

// Whether the page being drawn has already opened a feature group.  The header
// closes itself with a rule, so the first group on a page must not add a second
// one straight under it - a page or a sub-page whose first heading sits between
// two lines is the "unnecessary" one.  Reset by PageHeader, set by SectionHead.
static bool g_SectionSeen = false;

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
    {
        const ImU32 wash  = GC(g_T.rowHover, h);
        const ImU32 clear = wash & 0x00FFFFFFu;
        GradV3(ImGui::GetWindowDrawList(), mn, mx, g_T.roundRow, clear, wash, clear);
    }
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

// Toasts are the same plate as everything else.
//
// They used to be a plain rounded rectangle with a line of text in it - the one
// surface in the panel without the plate's light, its shadow or its state mark -
// so a toast read as a debug message laid over the interface.  Now it is a
// small plate: the same fall, a hairline, and a short accent mark on the left
// that says whether the thing was switched on or off.  It slides in from the
// direction it lives in and settles, rather than dropping into place.
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
        const float in   = Clampf(g_Toasts[i].t / 0.24f, 0.f, 1.f);
        const float out  = Clampf((life - g_Toasts[i].t) / 0.40f, 0.f, 1.f);
        const float a    = EaseOutCubic(in) * out;
        const float slide = (1.f - EaseOutCubic(in)) * 14.f;

        const ImVec2 ts = TSize(g_FontRegular, g_Toasts[i].text);
        const float w = ts.x + 44.f, h = kToastH;

        const float px = col.onRight ? col.x0 - slide : (col.x1 - w + slide);
        const ImVec2 p0(px, col.y0 + (float)live * (h + kToastGap));
        const ImVec2 p1(p0.x + w, p0.y + h);

        const float rad = 11.f + 3.f * Enh();

        // One soft step of shadow, so a toast sits on the plate rather than
        // being pasted onto it.
        dl->AddRectFilled(ImVec2(p0.x, p0.y + 2.f), ImVec2(p1.x, p1.y + 3.f),
            GC(IM_COL32(0, 0, 0, 40), a), rad);

        DrawBackdropRect(dl, p0, p1, rad, a * 255.f);
        dl->AddRectFilled(p0, p1, GC(Mix(g_T.windowBg, SlateFill(10.f, 22), 0.65f), a), rad);
        GradV(dl, p0, p1, rad, GC(IM_COL32(255, 255, 255, 20), a), GC(IM_COL32(255, 255, 255, 0), a));
        dl->AddRect(p0, p1, GC(IM_COL32(255, 255, 255, 26), a), rad, 0, g_T.borderThickness);

        // The state mark: a short vertical capsule, the accent for on and a
        // quiet grey for off.
        const ImU32 mark = g_Toasts[i].on ? g_T.pillActive : IM_COL32(170, 172, 182, 255);
        dl->AddRectFilled(ImVec2(p0.x + 11.f, p0.y + 9.f), ImVec2(p0.x + 13.f, p1.y - 9.f),
            GC(mark, a), 1.f);

        Txt(dl, g_FontRegular, ImVec2(p0.x + 24.f, p0.y + (h - g_FontRegular->FontSize) * 0.5f),
            GC(IM_COL32(236, 236, 240, 255), a), g_Toasts[i].text);
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
            ImVec2(tpos.x + K::ToggleW + 12.f, ty + nameH + 3.f), GC(g_T.textDim), Cap8(desc));

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
    if (desc)
    {
        // The control sits at the right end of this row, so the description gets
        // the rest of it and stops there: a line of text running under a
        // dropdown reads as the two being crowded together.
        const float side_gap = 10.f;
        const float desc_right = g_ContentR - K::ComboW - side_gap;
        const bool  clip = desc_right - g_ContentL > 24.f;
        if (clip)
            dl->PushClipRect(ImVec2(g_ContentL, ty + nameH), ImVec2(desc_right, ty + nameH + 3.f + descH), true);
        Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + nameH + 3.f), GC(g_T.textDim), Cap8(desc));
        if (clip)
            dl->PopClipRect();
    }

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
        TxtRight(dl, g_FontRegular, valueRight, ty + (nameH - g_FontRegular->FontSize) * 0.5f,
            Tight() > 0.5f ? GC(g_T.textDim) : GC(g_T.textFaint), valueText);

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

    // The heading is the one place the optional gradient belongs: a page title
    // is a heading, not a control, and a gradient across a row of text is what
    // the web components do with their display type.  The shadow is kept under
    // it so the title still reads over a bright game scene.
    if (settings::ui21::enabled && settings::ui21::gradient_heading)
    {
        const float a = g_AlphaMul * g_MenuFade;
        dl->AddText(tf, titleH, ImVec2(g_ContentL, ty + 1.f),
            ImGui::GetColorU32(IM_COL32(0, 0, 0, 70), a), title);
        ui21::gradient_text(dl, tf, titleH, ImVec2(g_ContentL, ty), title,
            0.08f, a * 0.98f);
    }
    else
    {
        TxtBold(dl, title, ImVec2(g_ContentL, ty), GC(g_T.textPrimary), true);
    }
    if (subtitle) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + titleH + 5.f), GC(g_T.textDim), Cap8(subtitle));

    if (keybind)
        KeybindControlVk("pagekey", ImVec2(g_ContentR - ksize.x, top + (rowH - ksize.y) * 0.5f), ksize, keybind);

    g_RowY += rowH + 12.f * (1.f - 0.40f * Tight());

    // No rule under the title either.  The title is a size and a weight step
    // away from the first group; a line under it was the second half of the
    // "lit lines across the page" the settings tab was covered in.
    g_SectionSeen = false;
    g_RowY += 4.f;
}

static void DimNote(const char* text)
{
    Txt(ImGui::GetWindowDrawList(), g_FontSmall, ImVec2(g_ContentL, g_RowY), SubText(), Cap8(text));
    g_RowY += g_FontSmall->FontSize + 10.f * (1.f - 0.40f * Tight());
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

    // The strip is the one control that reads as a row of tabs, so it is where
    // Enhance shows the most: taller, wider apart, and rounded into capsules -
    // the pill is half the strip's height rather than an 8 px corner.
    const float h    = 27.f + 6.f * Enh();
    const float padX = 13.f + 3.f * Enh();
    const float gap  = 3.f + 2.f * Enh();
    const float rad  = h * (0.34f + 0.16f * Enh());
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
            dl->AddRectFilled(ImVec2(x, y0), ImVec2(x + w, y0 + h),
                GC(IM_COL32(255, 255, 255, (int)(12 + 8 * Enh())), hv), rad);
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
    const ImVec2 pa(pilX, y0), pb(pilX + pilW, y0 + h);

    dl->AddRectFilled(pa, pb, GC(SlateFill(20.f, 30)), rad);
    dl->AddRect(pa, pb, GC(IM_COL32(255, 255, 255, 28 + 12 * Enh())), rad, 0, g_T.borderThickness);

    // With Enhance on the selected tab carries a short accent bar under it, and
    // its top edge is lit the way every other control's is - on the corner,
    // rather than a rule laid across it.
    if (Enh() > 0.01f)
    {
        TopSheen(dl, pa, pb, rad, GC(IM_COL32(255, 255, 255, (int)(70.f * Enh()))), g_T.borderThickness);

        const float bw = pilW * 0.30f;
        const ImVec2 ba(pilX + pilW * 0.5f - bw * 0.5f, y0 + h - 3.f);
        dl->AddRectFilled(ba, ImVec2(ba.x + bw, ba.y + 2.f),
            GC(g_T.pillActive, Enh() * 0.9f), 1.f);
    }

    for (int i = 0; i < n && i < 10; i++)
        Txt(dl, g_FontRegular, ImVec2(px[i] + padX, y0 + (h - g_FontRegular->FontSize) * 0.5f),
            GC(i == sel ? g_T.textPrimary : g_T.textDim), items[i]);

    // Air under the strip, not a rule.  A hairline across the top of every
    // Config sub-page was one of the lines in the background this page was
    // called out for: the pills already say where the strip ends.
    g_RowY = y0 + h + 22.f;
}

static int g_SubTab[kTabCount] = {};

static void CardBox(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding = 12.f,
                    float fillA = 7.f, float borderA = 22.f)
{
    // A card is the surface most likely to read as a grey slab: a 7-alpha white
    // wash is a fine lift on the glass and a grey rectangle on the slate, so
    // the slate gets a solid step above the plate instead.
    const ImU32 fill = fillA <= 0.5f ? IM_COL32(255, 255, 255, 0) : SlateFill(fillA, 22);
    dl->AddRectFilled(mn, mx, GC(fill), rounding);
    dl->AddRect(mn, mx, GC(IM_COL32(255, 255, 255, (int)borderA)), rounding, 0, g_T.borderThickness);
}

static void WellBox(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding = 10.f, float a = 128.f)
{
    // A recess, so it is darker than the plate it is cut into.  On the black
    // slate that means black, with the frame doing the separation; the glass
    // keeps its near-black wash, which already sits under the panel's fill.
    // A recess, so it sits *below* the plate it is cut into.  The glass keeps
    // its near-black wash (a wash over black is black anyway); the slate takes
    // a solid step under the plate, which is what keeps it a recess there.
    const ImU32 fill = settings::appearance::no_transparency
        ? IM_COL32(13, 13, 15, 255)
        : IM_COL32(9, 9, 11, (int)a);
    dl->AddRectFilled(mn, mx, GC(fill), rounding);
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

// A text field, not a pill.
//
// The rounded end caps were the problem: at 34 px tall a full-radius field is a
// capsule, and every button beside it was also a capsule, so a page with three
// inputs read as a row of identical grey lozenges.  An input is a recess - a
// shallow well with a hairline, a quiet icon, and a clear cross once there is
// something to clear.
static bool SearchField(const char* id, ImVec2 pos, float w, float h, char* buf, size_t n, const char* hint)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = pos, b(pos.x + w, pos.y + h);
    const float rad = Minf(10.f, h * 0.5f);

    ImGui::PushID(id);

    // The well, drawn first so the field's own cursor can sit on it.
    float& hv = AnimF("##searchhov", 0.f);
    const ImU32 well = settings::appearance::no_transparency
        ? IM_COL32(12, 12, 14, 255)
        : IM_COL32(6, 6, 8, 112);
    dl->AddRectFilled(a, b, GC(Mix(well, IM_COL32(255, 255, 255, 12), hv * 0.5f)), rad);
    dl->AddRect(a, b, GC(Mix(g_T.controlBorder, g_T.controlBorderHi, hv * 0.7f)), rad, 0, g_T.borderThickness);
    DrawGlyph(dl, "search", ImVec2(a.x + 15.f, (a.y + b.y) * 0.5f), 12.f, GC(g_T.textFaint));

    const bool has_clear = buf && buf[0];
    const float text_w = w - (has_clear ? 56.f : 40.f);

    ImGui::SetCursorScreenPos(ImVec2(a.x + 30.f, (a.y + b.y) * 0.5f - g_FontRegular->FontSize * 0.5f - 4.f));
    ImGui::SetNextItemWidth(text_w);
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

    if (has_clear)
    {
        const ImVec2 cx(b.x - 17.f, (a.y + b.y) * 0.5f);
        ImGui::SetCursorScreenPos(ImVec2(cx.x - 9.f, cx.y - 9.f));
        const bool hit = ImGui::InvisibleButton("##clr", ImVec2(18.f, 18.f));
        const bool hot = ImGui::IsItemHovered();
        if (hot)
            dl->AddCircleFilled(cx, 8.f, GC(IM_COL32(255, 255, 255, 16)), 20);
        DrawGlyph(dl, "x", cx, 11.f, GC(hot ? g_T.textPrimary : g_T.textFaint));
        if (hit)
        {
            buf[0] = 0;
            ImGui::PopID();
            return true;
        }
    }

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
    if (desc) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + nameH + 3.f), GC(g_T.textDim), Cap8(desc));
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
static float g_TabRise  = 0.f;

static void SetActiveTab(int i)
{
    if (i == g_ActiveTab) return;
    PushToast(kTabNames[i], true);
    g_TabSlideX = 0.f;
    g_TabFade = 0.f;
    g_TabRise = 7.f;

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
    // The column is laid out around the footer rather than from the top down.
    // It used to be a fixed pitch from the first row, which left whatever did
    // not fit as one dead block between the last tab and the user: at 552 px of
    // panel that was a hundred pixels of nothing under Config, and it read as a
    // column that had run out rather than a column with room.  The rows are
    // spread into that room instead, and the footer is measured from the
    // bottom, so the two always meet.
    const float avatarD = K::AvatarD + 8.f;
    const float nameH   = (g_FontBold ? g_FontBold : g_FontRegular)->FontSize;
    const float smallH  = g_FontSmall->FontSize;
    const float blockH  = Maxf(avatarD, nameH + 3.f + smallH);
    const float footerH = smallH + 8.f + blockH;
    const float footTop = bottom - 16.f - footerH;

    const float firstY = top + 5.f;
    const float dt     = ImGui::GetIO().DeltaTime;

    float pitch = K::TabH + K::TabGap;
    if (kTabCount > 1)
    {
        const float room  = Maxf(0.f, (footTop - 12.f) - firstY);
        const float slack = room - (float)kTabCount * K::TabH;
        const float gap   = slack > 0.f ? Minf(18.f, slack / (float)(kTabCount - 1)) : K::TabGap;
        pitch = K::TabH + gap;
    }

    const float targetY = firstY + (float)g_ActiveTab * pitch;
    if (g_PillY < 0.f) g_PillY = targetY;
    g_PillY = SpringTo("##pill", targetY, 0.30f, 0.80f, targetY);

    const ImVec2 pa(pillX0, g_PillY), pb(pillX0 + pillW, g_PillY + K::TabH);
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

        // No halo under the caption.
        //
        // The "glow" was the same word drawn eight more times, one pixel in each
        // direction, at 30 alpha.  At 14 px that is not a glow - it is eight
        // ghost edges on a glyph, and it is most of what made the selected tab's
        // text look soft and doubled.  The pill already carries the state.
        const ImVec2 tp(mn.x + K::TabTextX, (mn.y + mx.y) * 0.5f - g_FontRegular->FontSize * 0.5f);

        const ImU32 col = Mix(Mix(g_T.textDim, g_T.textPrimary, hv), g_T.textOnPill, act);
        DrawTabIcon(dl, kTabIconIds[i], ImVec2(mn.x + K::TabIconX, (mn.y + mx.y) * 0.5f), K::TabIcon, GC(col));
        Txt(dl, g_FontRegular, tp, GC(col), kTabNames[i]);

        y += pitch;
    }

    // The footer: one rule above it, the label, and a card-sized identity block.
    dl->AddLine(ImVec2(x0 + 10.f, footTop - 8.f), ImVec2(x1 - 10.f, footTop - 8.f),
        GC(g_T.hairline), 1.f);

    float fy = footTop;
    Txt(dl, g_FontSmall, ImVec2(x0 + 10.f, fy), GC(g_T.textDim), "Welcome back");
    fy += smallH + 8.f;

    const ImVec2 ac(x0 + 10.f + avatarD * 0.5f, fy + blockH * 0.5f);
    const float  ar = avatarD * 0.5f;

    // The face is a circle: the same round slot the home card and the nearby
    // list fill, drawn from the same head-shot hook.  It is also the one shape
    // no radius blend can make rounder, so it is the anchor the rest of the
    // surface's rounding is read against - a dark seat under the picture, the
    // head shot clipped to the disc, a silhouette until the thumbnail arrives,
    // and one hairline for the edge.
    const ImVec2 amn(ac.x - ar, ac.y - ar);
    const ImVec2 amx(ac.x + ar, ac.y + ar);

    dl->AddCircleFilled(ac, ar + 1.f, GC(IM_COL32(14, 14, 18, 210)), 48);

    bool have_face = false;
    if (sync::hooks().player_avatar_rounded)
    {
        sync::SessionInfo me_sn{};
        if (sync::hooks().session && sync::hooks().session(me_sn) && me_sn.local_user_id > 0)
        {
            have_face = sync::hooks().player_avatar_rounded(dl, me_sn.local_user_id,
                amn, amx, GC(IM_COL32(255, 255, 255, 255)), ar);
        }
    }
    if (!have_face)
    {
        dl->AddCircleFilled(ImVec2(ac.x, ac.y - 4.f), 4.f, GC(g_T.textDim), 20);
        AddArc(dl, ImVec2(ac.x, ac.y + 8.5f), 7.5f, kPi * 1.15f, kPi * 1.85f, GC(g_T.textDim), 1.9f);
    }

    dl->AddCircle(ac, ar, GC(g_T.windowBorder), 48, g_T.borderThickness);

    const float nx = x0 + 10.f + avatarD + 10.f;

    char me[64] = "not attached";
    char handle[64] = "no game";
    if (sync::hooks().local_player)
        sync::hooks().local_player(me, sizeof me, handle, sizeof handle);

    // Clipped to the column rather than drawn through the divider: Test Enhance
    // takes the sidebar in, and a display name long enough to leave the nav is
    // the one thing in here that has no room to grow back.
    const float name_w = Maxf(24.f, x1 - nx - 4.f);
    const float textY  = fy + (blockH - (nameH + 3.f + smallH)) * 0.5f;
    TxtClipped(dl, g_FontBold ? g_FontBold : g_FontRegular, ImVec2(nx, textY),
        GC(g_T.textPrimary), me, name_w);
    TxtClipped(dl, g_FontSmall, ImVec2(nx, textY + nameH + 3.f), GC(g_T.textFaint), handle, name_w);
}

static void PanelSpringStep(bool active)
{
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float  maxx = Maxf(2.f, ds.x - K::W - 2.f);
    const float  maxy = Maxf(2.f, ds.y - K::H - 2.f);

    // How close to an edge the panel has to be before the edge takes it, and
    // how fast the chase is.  A grab that is tracking well is critically
    // damped - no overshoot, no wobble - and a released panel is damped a
    // little past critical so it settles instead of ringing.
    constexpr float k_magnet  = 28.f;
    constexpr float k_stiff   = 240.f;

    if (active)
    {
        if (g_PanelTarget.x < 2.f + k_magnet)          g_PanelTarget.x = 2.f;
        if (g_PanelTarget.x > maxx - k_magnet)         g_PanelTarget.x = maxx;
        if (g_PanelTarget.y < 2.f + k_magnet)          g_PanelTarget.y = 2.f;
        if (g_PanelTarget.y > maxy - k_magnet)         g_PanelTarget.y = maxy;
    }

    const float damp = (active ? 2.0f : 2.35f) * std::sqrt(k_stiff);

    // Fixed-step, so a frame that took 40 ms does not overshoot the way a
    // single 40 ms step of a stiff spring does.
    float left = ImClamp(ImGui::GetIO().DeltaTime, 0.f, 0.10f);
    while (left > 0.f)
    {
        const float dt = Minf(left, 1.f / 120.f);
        left -= dt;

        const float ax = (g_PanelTarget.x - g_PanelPos.x) * k_stiff - g_PanelVel.x * damp;
        const float ay = (g_PanelTarget.y - g_PanelPos.y) * k_stiff - g_PanelVel.y * damp;
        g_PanelVel.x += ax * dt;
        g_PanelVel.y += ay * dt;
        g_PanelPos.x += g_PanelVel.x * dt;
        g_PanelPos.y += g_PanelVel.y * dt;
    }

    // Settled: under half a pixel out and under half a pixel a second of
    // movement, the spring is done and is snapped rather than left to creep.
    if (ImFabs(g_PanelTarget.x - g_PanelPos.x) < 0.5f && ImFabs(g_PanelVel.x) < 2.f)
    {
        g_PanelPos.x = g_PanelTarget.x;
        g_PanelVel.x = 0.f;
    }
    if (ImFabs(g_PanelTarget.y - g_PanelPos.y) < 0.5f && ImFabs(g_PanelVel.y) < 2.f)
    {
        g_PanelPos.y = g_PanelTarget.y;
        g_PanelVel.y = 0.f;
    }

    g_PanelPos.x = Clampf(g_PanelPos.x, 2.f, maxx);
    g_PanelPos.y = Clampf(g_PanelPos.y, 2.f, maxy);
}

static void PanelDragUpdate(bool active)
{
    POINT cur = {};
    const bool have_cursor = ::GetCursorPos(&cur) != 0;

    if (active && have_cursor)
    {
        const ImVec2 at((float)cur.x, (float)cur.y);

        if (!g_PanelDrag)
        {
            // The grab offset is taken from where the panel *is*, so a panel
            // that is still gliding when it is grabbed does not jump to the
            // cursor: it is pulled from where it was.
            g_PanelDrag = true;
            g_PanelGrab = ImVec2(at.x - g_PanelPos.x, at.y - g_PanelPos.y);
        }

        g_PanelTarget = ImVec2(at.x - g_PanelGrab.x, at.y - g_PanelGrab.y);

        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        g_PanelTarget.x = Clampf(g_PanelTarget.x, 2.f, Maxf(2.f, ds.x - K::W - 2.f));
        g_PanelTarget.y = Clampf(g_PanelTarget.y, 2.f, Maxf(2.f, ds.y - K::H - 2.f));
    }
    else
    {
        g_PanelDrag = false;
    }

    PanelSpringStep(active);
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

    // The bar is lit from above, like the plate and the cards: the reference's
    // bar runs #383838 along its top edge into #303030, so it is the one solid
    // surface with a gradient inside it.  A flat fill of the mid value was the
    // last slab on the panel with no fall on it at all.
    if (settings::appearance::no_transparency)
    {
        // The fall, and nothing else: the 1 px white line that used to sit on
        // the top edge was a highlight drawn *on* the bar instead of the bar
        // being lit, and at one pixel it read as a white box outline.
        GradV(dl, a, b, g_T.roundTopBar,
            GC(IM_COL32(255, 255, 255, 16)), GC(IM_COL32(255, 255, 255, 0)));
    }

    // No outline on the bar.  A one pixel line round the header is the same
    // rule the page's own groups just lost: the bar is a step lighter than the
    // plate, and that step is the edge.

    const ImVec2 lc(a.x + 22.f, (a.y + b.y) * 0.5f);
    DrawLogo(dl, lc, 17.f, GC(g_T.textPrimary));
    const ImVec2 ts0 = TSizeBold(sync::kBrandName, true);
    TxtBold(dl, sync::kBrandName, ImVec2(lc.x + 16.f, (a.y + b.y) * 0.5f - ts0.y * 0.5f), GC(g_T.textPrimary), true);

    // The bar stops at the brand.  A status pill sat here for one round and it
    // answered a question the panel does not ask: whether a place is attached is
    // already visible in every row that can only exist in a game, and a badge
    // that flips between "idle" and "in game" is one more thing to read before
    // the menu can be used.
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
    // Both insets come from the metrics rather than from literals: Test Enhance
    // takes the gutter between the divider and the content in with the rest of
    // the left side, and the first text column has to move with it or the rows
    // stop lining up with the nav above them.
    const float childX = wp.x + K::Pad + K::SidebarW + K::DividerGap + K::ContentInset;
    const float childR = wp.x + K::W - 9.f;

    ImGui::SetCursorScreenPos(ImVec2(childX, top));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(K::ContentPadX, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 7.f);
    ImGui::BeginChild("##content", ImVec2(childR - childX, bottom - top),
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoScrollWithMouse);

    ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    g_TabFade   = Approach(g_TabFade, 1.f, 26.f, dt);
    g_TabSlideX = Approach(g_TabSlideX, 0.f, 22.f, dt);
    g_TabRise   = Approach(g_TabRise, 0.f, 20.f, dt);

    if (fabsf(g_TabSlideX) < 0.05f) g_TabSlideX = 0.f;
    if (g_TabRise < 0.05f) g_TabRise = 0.f;

    g_ContentL   = wp.x + K::Pad + K::SidebarW + K::DividerGap + K::ContentInset + K::ContentPadX + 1.f + g_TabSlideX;
    g_ContentR   = wp.x + K::W - K::Pad - K::ScrollReserve + g_TabSlideX;
    g_ContentBottom = bottom - 8.f;

    g_ContentTop = top + K::ContentPadTop - g_ScrollShown - g_TabRise;
    g_RowY       = g_ContentTop;

    g_AlphaMul = SmoothStep(g_TabFade);

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
        g_AlphaMul = SmoothStep(g_TabFade) * wipec;

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

// No plate decoration.
//
// The panel has been through both answers to "a dark plate on a dark scene has
// no edge": six concentric white strokes (a glow with no light source, which is
// what read as odd over the blur), then a fourteen-step black shadow around it
// (a halo of its own, and up to 42 px of gradient over whatever the game was
// doing at that edge).  Both were an outline drawn *outside* the panel, and an
// outline outside a frosted surface is exactly what a clean panel does not have:
// the frost already ends at a curve, and the one hairline border below is the
// whole edge.  Nothing is drawn around the plate any more.
static void DrawMenu(ImVec2 wp)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // The glass fill, and only the glass fill.  ApplyStyle() has already put
    // the plate's colour together from the theme and the transparency slider,
    // which with No Transparency on is the reference's grey slate - so this
    // must not overwrite it.  It used to write the same hardcoded (36, 36, 42) every
    // frame whatever the theme said, which is the grey the panel was called for
    // and which no theme colour could win against.
    // The plate is the frost, not a wash laid over it.
    //
    // This line used to force (7, 7, 9) at GlassAlpha() - a near-black fill at
    // 84 of 255 - and that fill is what was left when the capture never produced
    // an image: a dark tint with no blur behind it, which is exactly how it read.
    // Reporting the blur *and* hiding it under an opaque wash cannot both be
    // true, so with a usable frost the fill is a whisper of neutral (16 alpha)
    // and the darkening is the backdrop's own Tint slider, where the user put it.
    // With no frost - blur off, tint-only, or a capture that has not landed yet -
    // the fill comes back, because then it is the plate and not a veil.
    // The effects pack draws against whatever the plate is made of this frame,
    // so it is told before anything is drawn.  With No Transparency on there is
    // no frost by definition, and the lights use their black-plate values.
    if (!settings::appearance::no_transparency)
    {
        const bool frost = settings::backdrop::enabled && !settings::backdrop::tint_only &&
                           sync::backdrop().ready();
        g_T.windowBg = frost ? IM_COL32(10, 10, 13, 16)
                             : IM_COL32(9, 9, 12, GlassAlpha());
        // The pack is told how strong the frost is and how much the tint is
        // darkening it, so its light is a function of the material it sits on
        // rather than a fixed value that only happens to look right at one
        // setting of the two sliders.
        ui21::set_frost(frost, settings::backdrop::blur / 100.f, settings::backdrop::tint / 100.f);
    }
    else
    {
        ui21::set_frost(false);
    }

    const ImVec2 p0(wp.x, wp.y);
    const ImVec2 p1(wp.x + K::W, wp.y + K::H);

    // The frost is not composited under an opaque plate: with No Transparency
    // there is nothing of the desktop to show, so the capture is not sampled.
    if (!settings::appearance::no_transparency)
        DrawBackdropRect(dl, p0, p1, g_T.roundWindow, g_MenuFade * 255.f);

    // The optional depth layer sits *behind* the plate, so it is drawn first.
    // Everything else in the pack is light on the glass, drawn over the fill
    // and under the content.  All of it is off unless the master switch is on
    // and the individual switch is on, so the plate above is the whole look by
    // default.
    ui21::panel_shadow(dl, p0, p1, g_T.roundWindow);

    // Flat on purpose.  The solid plate is a grey surface with a hairline and
    // two lighter surfaces inside it; a gradient over the whole panel was the
    // thing that made it look painted rather than made.
    dl->AddRectFilled(p0, p1, GC(g_T.windowBg), g_T.roundWindow);

    // The grey-to-black blend, solid mode only.  The reference's plate is not
    // one flat grey: it is lighter at the top and falls to near-black at the
    // bottom, which is what stops a big dark surface reading as a cut-out.  The
    // glass builds do not need it (they have the frost), and adding it there
    // would fight the backdrop they sit on.
    //
    // Inset by the corner radius so the band never paints outside the curve,
    // and short of the bottom so the plate still ends at the darker value.
    {
        // The fall is the full height of the plate and it follows the plate's
        // own rounding: GradV colours the rounded path vertex by vertex, so
        // there is no inset rectangle and therefore no vertical seam where the
        // old band started and stopped.
        //
        // On the glass it is drawn too, one step lighter: "transparency + blur"
        // is the same grey blend sitting on the frost rather than a different
        // surface, and leaving it out left the glass plate flatter than the
        // solid one.
        //
        // Through GC(), because this is part of the plate and has to fade with
        // it: raw colours keep their own alpha, so during the open animation the
        // blend stayed at full strength over a half-faded plate and read as a
        // grey band floating in front of the panel.
        const int a = settings::appearance::no_transparency ? 10 : 8;
        GradV(dl, p0, p1, g_T.roundWindow,
            GC(IM_COL32(255, 255, 255, a)), GC(IM_COL32(255, 255, 255, 0)));
    }

    // The light the site launcher wears, on the panel: one broad wash off the
    // top edge and one counter-wash low on the other side.  It is the same
    // fall the floating cards get, and it is what stops the plate reading as
    // one value from corner to corner - which is the difference between a
    // surface and a cut-out.  Drawn after the fill and before the content, so
    // it is light on the plate rather than a tint over everything.
    // Strength follows Accent Spread - and follows it on the stock palette too,
    // which is what "it will not on the default one" was about: the spread was
    // gated on a custom accent, so the default theme had no way to touch it.
    // Zero spread is off, which is the slider's own lowest position.
    if (settings::ui21::panel_glow)
    {
        const float sp = ImClamp(settings::appearance::accent_spread, 0.f, 1.f);
        const float k  = 0.25f + 1.35f * sp;   // 0.25 at the left end, 1.6 at the right
        const int top  = (int)((settings::appearance::no_transparency ? 18.f : 12.f) * k + 0.5f);
        const int low  = (int)((settings::appearance::no_transparency ? 6.f : 4.f) * k + 0.5f);

        // Drawn as a vertical fall over the plate's own rounded shape, not as
        // an ellipse on top of it.  An ellipse reaches past the corners, the
        // window clips it on its square bounds, and the result is a bright
        // square on each corner - the "it looks square" the panel was called
        // out for.  A gradient that follows the path has nothing outside the
        // plate to clip, so the light ends at the edge it belongs to.
        GradV(dl, p0, p1, g_T.roundWindow,
            GC(IM_COL32(255, 255, 255, ImClamp(top, 0, 60))), GC(IM_COL32(255, 255, 255, 0)));
        // The counter-wash: the bottom of the plate gets a hair of light back, so
        // the fall is not a straight ramp from a lit top to a dead bottom.
        GradV(dl, p0, p1, g_T.roundWindow,
            GC(IM_COL32(255, 255, 255, 0)), GC(IM_COL32(255, 255, 255, ImClamp(low, 0, 40))));
    }

    ui21::aurora(dl, p0, p1, g_T.roundWindow);
    ui21::top_light(dl, p0, p1, g_T.roundWindow, settings::ui21::light_rays_strength);
    ui21::dot_grid(dl, p0, p1, g_T.roundWindow);
    dl->AddRect(p0, p1, GC(g_T.windowBorder), g_T.roundWindow, 0, g_T.borderThickness);
    ui21::edge_bloom(dl, p0, p1, g_T.roundWindow);
    ui21::corner_light(dl, p0, p1, g_T.roundWindow);
    ui21::cursor_light(dl, p0, p1, g_T.roundWindow, ImGui::GetIO().MousePos,
        settings::ui21::spotlight_size);

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

// The overlay starts *shut*, because it now opens with its own card and the
// panel is what comes after it.  It used to start open, which is why the card
// was never seen: it was drawn on the background list, underneath a panel that
// was already on screen.
//
// `g_BootHold` is the gate.  It is here, at the panel's own open flag, because
// the host owns that flag and pushes it every frame (`sync::set_open`), so a
// card that merely *hid* the panel would be arguing with set_open sixty times a
// second instead of opening after the intro.  `g_HostOpen` is what the host
// last said, kept so the hold can be lifted to exactly what was asked for.
static bool  g_MenuOpen  = false;
static bool  g_BootHold  = true;

// The overlay's own intro.  Two seconds, of which a third is the card opening
// and a sixth is it folding away: the rest is the bar filling, which is the only
// part that is a wait.
static constexpr float kIntroSeconds = 2.0f;
static float g_BootStart = -1.f;
static bool  g_HostOpen  = false;
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

static const float kCloseResponse = 0.28f;
static const float kCloseDamping  = 0.72f;
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

    // The card starts in the bottom-left corner: the panel is anchored to the
    // top-left, so the old default put the card straight on top of the panel's
    // own header the moment the menu opened, which is not a watermark, it is a
    // card in the way.
    if (g_CardPos.y < 0.f)
    {
        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        g_CardPos = ImVec2(16.f, Maxf(16.f, ds.y - K::CardH - 16.f));
    }

    const ImVec2 p0(g_CardPos.x, g_CardPos.y);
    const ImVec2 p1(g_CardPos.x + K::CardW, g_CardPos.y + K::CardH);
    const bool dragging = CardDragUpdate(p0, p1);

    // The card's whole geometry, in one place.  It used to be literals scattered
    // down the function (+19 for the logo, +41 for the stats, +56 for the strip)
    // against a 66 px card, which is why the strip sat a pixel under the text
    // and the handle could run into the brand: there was no layout, only three
    // numbers that happened to fit once.  Every row is measured from the one
    // above it now.
    const float pad   = 13.f;
    const float brandTop = p0.y + pad + 1.f;
    const float logoCy   = brandTop + 6.f;
    const float sepY     = p0.y + 35.f;
    const float statTop  = sepY + 7.f;
    const float chartH   = 9.f;
    const float chartY   = p1.y - pad - chartH;

    const float plate_rad = g_T.roundRow;
    DrawBackdropRect(dl, p0, p1, plate_rad);

    // The card's material: the same one the docks are made of, so the floating
    // card belongs to the same surface language as the floating plates.  The
    // step up from the plate is what the light falls on - at the plate's own
    // value a card has nothing to be lighter than and reads as a hole.
    const bool opaque = settings::appearance::no_transparency;
    const ImU32 wm_fill = opaque
        ? Mix(g_T.windowBg, IM_COL32(255, 255, 255, 255), 0.055f)
        : g_T.windowBg;
    dl->AddRectFilled(p0, p1, wm_fill, plate_rad);

    // One fall over the card's own shape, top to bottom, and no line drawn on
    // the top edge: the card is lit, it does not wear a highlight.
    GradV(dl, p0, p1, plate_rad,
        IM_COL32(255, 255, 255, opaque ? 18 : 10), IM_COL32(255, 255, 255, 0));

    dl->AddRect(p0, p1, IM_COL32(255, 255, 255, dragging ? 46 : 26), plate_rad, 0, g_T.borderThickness);

    const ImVec2 lc(p0.x + pad + 8.f, logoCy);
    DrawLogo(dl, lc, 15.f, IM_COL32(238, 238, 242, 255));
    TxtBold(dl, sync::kBrandName, ImVec2(lc.x + 13.f, brandTop), IM_COL32(238, 238, 242, 255));
    {
        char me[64] = "not attached";
        char handle[64] = "";
        if (sync::hooks().local_player)
            sync::hooks().local_player(me, sizeof me, handle, sizeof handle);
        const char* user = handle[0] ? handle : me;
        const ImVec2 s = TSize(g_FontSmall, user);
        Txt(dl, g_FontSmall, ImVec2(p1.x - pad - s.x, brandTop + 1.f), IM_COL32(152, 152, 158, 255), user);
    }

    // The rule under the header: the card's one internal division, at the same
    // weight the panel uses between its own groups.
    dl->AddLine(ImVec2(p0.x + pad, sepY), ImVec2(p1.x - pad, sepY),
        IM_COL32(255, 255, 255, opaque ? 18 : 14), 1.f);

    const ImU32 green = IM_COL32(134, 220, 140, 255);
    const ImU32 dim   = IM_COL32(150, 150, 156, 255);
    const ImU32 sep   = IM_COL32(124, 124, 130, 200);
    const ImU32 val   = IM_COL32(226, 226, 230, 255);
    const float y = statTop;
    const float labelY = statTop + 2.f;   // the small labels sit on the value's line
    float x = p0.x + pad;
    char buf[24];
    bool first = true;

    if (sync::state().wm_fps)
    {
        snprintf(buf, sizeof(buf), "%.0f", fps);
        TxtBold(dl, buf, ImVec2(x, y), green);
        x += TSizeBold(buf).x + 5.f;
        Txt(dl, g_FontSmall, ImVec2(x, labelY), dim, "FPS");
        x += TSize(g_FontSmall, "FPS").x + 13.f;

        snprintf(buf, sizeof(buf), "%.0f", ms);
        TxtBold(dl, buf, ImVec2(x, y), green);
        x += TSizeBold(buf).x + 5.f;
        Txt(dl, g_FontSmall, ImVec2(x, labelY), dim, "MS");
        x += TSize(g_FontSmall, "MS").x + 11.f;
        first = false;
    }

    if (!first)
    {
        dl->AddCircleFilled(ImVec2(x + 1.5f, y + 7.f), 1.6f, sep);
        x += 13.f;
    }

    {
        const menu_clock::time_point count_t0 = cost_now();
        int n = SyncPlayerCount();
        g_count_ms = cost_ms(count_t0);
        if (n < 0) n = 0;
        DrawTabIcon(dl, ICON_USERS, ImVec2(x + 7.f, y + 7.f), 15.f, dim);
        x += 18.f;
        snprintf(buf, sizeof(buf), "%d", n);
        TxtBold(dl, buf, ImVec2(x, y), val);
        x += TSizeBold(buf).x + 11.f;
    }

    if (sync::state().wm_time)
    {
        dl->AddCircleFilled(ImVec2(x + 1.5f, y + 7.f), 1.6f, sep);
        x += 13.f;

        SYSTEMTIME st = {};
        ::GetLocalTime(&st);
        snprintf(buf, sizeof(buf), "%02d:%02d", (int)st.wHour, (int)st.wMinute);
        TxtBold(dl, buf, ImVec2(x, y), val);
    }

    // The FPS strip.  It is scaled to its own window rather than to 240 fps:
    // against a fixed 240 the whole line sat in the bottom pixel and a half of
    // its band and read as a green jitter along the card's edge, which is what
    // made it look wrong at every frame rate below the cap.  Clipped to the
    // card as well, so a spike cannot draw over the rounded corner.
    const int n = g_FpsHistN < 64 ? g_FpsHistN : 64;
    if (n > 2)
    {
        float lo = 1e9f, hi = -1e9f;
        for (int i = 0; i < n; i++)
        {
            const float v = g_FpsHist[(g_FpsHistN - n + i) & 63];
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }
        // At least a 20 fps window, so a steady frame rate is a flat line rather
        // than noise magnified to the full band.
        if (hi - lo < 20.f) { const float m = (hi + lo) * 0.5f; lo = m - 10.f; hi = m + 10.f; }

        const float gx0 = p0.x + pad, gx1 = p1.x - pad;
        const float gh  = chartH;
        const float gy  = chartY;

        dl->PushClipRect(ImVec2(p0.x + 2.f, p0.y), ImVec2(p1.x - 2.f, p1.y - 2.f), true);

        // A baseline under the trace, so the strip is a chart with a floor
        // rather than a line floating in the card.
        dl->AddLine(ImVec2(gx0, gy + gh), ImVec2(gx1, gy + gh),
            IM_COL32(255, 255, 255, opaque ? 16 : 12), 1.f);

        ImVec2 prev(0.f, 0.f);
        for (int i = 0; i < n; i++)
        {
            const float f = g_FpsHist[(g_FpsHistN - n + i) & 63];
            const float v = Clampf((f - lo) / Maxf(1.f, hi - lo), 0.f, 1.f);
            const ImVec2 p(gx0 + (gx1 - gx0) * ((float)i / (float)(n - 1)), gy + gh - gh * v);
            if (i) dl->AddLine(prev, p, IM_COL32(134, 220, 140, 210), 1.4f);
            prev = p;
        }
        dl->PopClipRect();
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
            // an always-on bind only has something to say when the feature is on
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

    const float pad_x = 16.f, pad_y = 12.f, row_h = 21.f;
    const float col_gap = 14.f;

    float key_w = 0.f, name_w = 0.f, mode_w = 0.f;
    for (int i = 0; i < n; ++i)
    {
        key_w  = Maxf(key_w,  TSizeBold(binds[i].mode == 2 ? "-" : DockKeyName(binds[i].key)).x);
        name_w = Maxf(name_w, TSize(g_FontRegular, binds[i].name).x);
        mode_w = Maxf(mode_w, TSize(g_FontSmall, DockModeName(binds[i].mode)).x);
    }
    if (key_w < 26.f) key_w = 26.f;

    const float w = pad_x * 2.f + key_w + col_gap + name_w + col_gap + mode_w;
    const float h = pad_y * 2.f + (float)n * row_h;

    {
        const ImVec2 at(st.kb_x, st.kb_y);
        DockDragUpdate(at, ImVec2(at.x + w, at.y + h));
    }

    const ImVec2 p0(st.kb_x, st.kb_y);
    const ImVec2 p1(p0.x + w, p0.y + h);
    const float rad = 13.f;

    // The same plate as the panel and the media card: frost or slate, one fall
    // of light, one hairline.  It used to be a flat rectangle with a border,
    // which is why the plate read as a separate widget instead of a surface.
    DrawBackdropRect(dl, p0, p1, rad);
    dl->AddRectFilled(p0, p1, GC(g_T.windowBg), rad);
    GradV(dl, p0, p1, rad, GC(IM_COL32(255, 255, 255, 16)), GC(IM_COL32(255, 255, 255, 0)));
    dl->AddRect(p0, p1, GC(IM_COL32(255, 255, 255, 28)), rad, 0, g_T.borderThickness);

    const float x_key  = p0.x + pad_x;
    const float x_name = x_key + key_w + col_gap;
    const float x_mode = p1.x - pad_x - mode_w;

    float y = p0.y + pad_y;
    for (int i = 0; i < n; ++i)
    {
        const bool firing = live_state::get(binds[i].live);
        const char* kname = (binds[i].mode == 2) ? "-" : DockKeyName(binds[i].key);
        const ImVec2 ks = TSizeBold(kname);

        if (firing)
            dl->AddCircleFilled(ImVec2(p0.x + pad_x * 0.42f, y + g_FontRegular->FontSize * 0.5f),
                2.f, GC(g_T.pillActive), 12);

        TxtBold(dl, kname, ImVec2(x_key + (key_w - ks.x), y),
            GC(firing ? g_T.textPrimary : g_T.textPrimary, firing ? 1.f : 0.86f));
        Txt(dl, g_FontRegular, ImVec2(x_name, y), GC(g_T.textDim), binds[i].name);
        Txt(dl, g_FontSmall, ImVec2(x_mode, y + 1.f), GC(g_T.textFaint), DockModeName(binds[i].mode));
        y += row_h;
    }
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
    const float pad   = 12.f * s;
    const float avd   = 32.f * s;
    const float gap   = 5.f * s;
    const float bar_h = 4.f * s;

    char hp[16];
    snprintf(hp, sizeof hp, "%d", (int)(who->health + 0.5f));

    const float nameH  = g_FontBold->FontSize;
    const float smallH = g_FontSmall->FontSize;

    // One column, measured from its own contents: name, a slim health bar with
    // the number at its end, then the tool.  The old card laid these out with
    // fixed row heights and a heart glyph, so a long name and a missing tool
    // both left holes in it.
    const bool has_tool = st.th_tool && who->tool[0];
    float text_w = 108.f * s;
    if (st.th_name) text_w = Maxf(text_w, TSizeBold(who->name).x);
    if (st.th_health) text_w = Maxf(text_w, TSize(g_FontSmall, hp).x + 74.f * s);
    if (has_tool) text_w = Maxf(text_w, TSize(g_FontSmall, who->tool).x);

    float rows_h = 0.f;
    if (st.th_name) rows_h += nameH;
    if (st.th_health) rows_h += (rows_h > 0.f ? gap : 0.f) + Maxf(bar_h, smallH * 0.7f);
    if (has_tool) rows_h += (rows_h > 0.f ? gap : 0.f) + smallH;

    const float w = pad * 2.f + (st.th_avatar ? avd + 11.f * s : 0.f) + text_w;
    const float h = pad * 2.f + Maxf(st.th_avatar ? avd : 0.f, rows_h);

    ImVec2 pp(st.th_x, st.th_y);
    float ax = 0.f, ay = 0.f;
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
    const float rad = 13.f;

    DrawBackdropRect(dl, p0, p1, rad);
    dl->AddRectFilled(p0, p1, GC(g_T.windowBg), rad);
    GradV(dl, p0, p1, rad, GC(IM_COL32(255, 255, 255, 16)), GC(IM_COL32(255, 255, 255, 0)));
    dl->AddRect(p0, p1, GC(IM_COL32(255, 255, 255, 28)), rad, 0, g_T.borderThickness);

    float tx = p0.x + pad;

    if (st.th_avatar)
    {
        const ImVec2 ac(tx + avd * 0.5f, p0.y + h * 0.5f);
        dl->AddCircleFilled(ac, avd * 0.5f, GC(SlateFill(18.f, 26)), 40);

        const bool drew = sync::hooks().player_avatar
            && sync::hooks().player_avatar(dl, who->user_id,
                ImVec2(ac.x - avd * 0.5f, ac.y - avd * 0.5f),
                ImVec2(ac.x + avd * 0.5f, ac.y + avd * 0.5f), IM_COL32_WHITE);
        if (!drew)
        {
            char initial[2] = { who->name[0] ? who->name[0] : '?', 0 };
            const ImVec2 is = TSizeBold(initial);
            TxtBold(dl, initial, ImVec2(ac.x - is.x * 0.5f, ac.y - is.y * 0.5f),
                GC(g_T.textPrimary));
        }
        dl->AddCircle(ac, avd * 0.5f, GC(IM_COL32(255, 255, 255, 34)), 40, 1.f);

        tx += avd + 11.f * s;
    }

    float ty = p0.y + (h - rows_h) * 0.5f;

    if (st.th_name)
    {
        TxtBold(dl, who->name, ImVec2(tx, ty), GC(g_T.textPrimary));
        ty += nameH + gap;
    }

    if (st.th_health)
    {
        const float hpv = who->max_health > 0.f ? Clampf(who->health / who->max_health, 0.f, 1.f) : 0.f;
        const ImU32 hp_col = hpv > 0.6f ? IM_COL32(126, 214, 132, 255)
                           : hpv > 0.3f ? IM_COL32(226, 200, 106, 255)
                                        : IM_COL32(226, 106, 106, 255);

        // The bar is the reading, the number is the detail: a bar alone cannot
        // be told apart from a decoration, and a number alone has no length.
        const float by = ty + (Maxf(bar_h, smallH * 0.7f) - bar_h) * 0.5f;
        const float bw = text_w - TSize(g_FontSmall, hp).x - 10.f * s;

        dl->AddRectFilled(ImVec2(tx, by), ImVec2(tx + bw, by + bar_h),
            GC(IM_COL32(255, 255, 255, 26)), bar_h * 0.5f);
        if (hpv > 0.002f)
            dl->AddRectFilled(ImVec2(tx, by), ImVec2(tx + bw * hpv, by + bar_h),
                GC(hp_col), bar_h * 0.5f);

        const ImVec2 hs = TSize(g_FontSmall, hp);
        Txt(dl, g_FontSmall, ImVec2(tx + bw + 10.f * s, by + (bar_h - hs.y) * 0.5f),
            GC(g_T.textDim), hp);
        ty += Maxf(bar_h, smallH * 0.7f) + gap;
    }

    if (has_tool)
        Txt(dl, g_FontSmall, ImVec2(tx, ty), GC(g_T.textFaint), who->tool);
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


// Keeps the two gates honest: the overlay watches for the client
// disappearing, and the unsupported-place notice re-arms when its config
// switch is turned back off.
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

    // The notice is about a newly entered experience, so it only ever arms while
    // the client is inside a place - sitting on the home screen shows nothing.
    // Leaving re-arms it, so the next join is judged on its own.
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

// The waiting card.
//
// Two waits share one plate.  The overlay's own is the one that was missing:
// it runs from the first frame, needs no game and no session, and gives the
// overlay a moment of its own instead of appearing as nothing and then as a
// menu.  The other is a place being joined, which may or may not happen.
//
// Both used to be drawn through GC(), which carries the panel's own fade - and
// with the panel shut that fade is zero, so the plate, the mark and the name
// were the transparent ones and only the wash and the bar ever showed.  The
// card now sets the fade for itself, so it is drawn whole whether the panel is
// up or not.
// The overlay's own intro, drawn once.
//
// It is a card that opens - unfolding from a thin line into its own rounded
// plate - holds while its bar fills, then folds back into that line and leaves
// the screen to the panel.  There is no ring around the mark and no spinner:
// the progress is the only thing moving, so a second loop going round it would
// be a second thing to watch for no reason.
//
// It runs on the overlay's clock, not the game's, and it is gated by
// `g_BootHold` so the panel cannot arrive underneath it.  When the hold lifts
// the card is done for good; joining a place later does not bring it back,
// because a loading screen that reappears every round is a loading screen you
// stop reading.
static void DrawLoadingCard(ImVec2 display)
{
    const float dt   = ImGui::GetIO().DeltaTime;
    const float step = (dt > 0.f && dt < 0.25f) ? dt : 0.f;

    static float s_boot = 0.f;
    static bool  s_done = false;

    if (s_done || !g_BootHold)
        return;

    s_boot += step;
    if (s_boot >= kIntroSeconds)
    {
        s_done = true;
        return;
    }

    const float p = ImClamp(s_boot / kIntroSeconds, 0.f, 1.f);

    // open: 0 -> 0.22, hold: 0.22 -> 0.84, close: 0.84 -> 1.0
    const float open  = EaseOutCubic(ImClamp(p / 0.22f, 0.f, 1.f));
    const float close = 1.f - EaseOutCubic(ImClamp((p - 0.84f) / 0.16f, 0.f, 1.f));
    const float vis   = open * close;
    if (vis <= 0.002f)
        return;

    // The content arrives after the plate has opened, and leaves before it has
    // finished folding: text that is still fading while its card collapses
    // reads as a glitch, not as a transition.
    const float content = ImClamp((open - 0.35f) / 0.65f, 0.f, 1.f) * close;

    const float saved_mul  = g_AlphaMul;
    const float saved_fade = g_MenuFade;
    g_MenuFade = 1.f;
    g_AlphaMul = 1.f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    dl->AddRectFilled(ImVec2(0.f, 0.f), display, IM_COL32(5, 5, 7, (int)(150.f * vis)));

    // The plate's own rectangle, interpolated in both directions: it opens
    // vertically from a line (h) and horizontally from a little narrower (w),
    // so the first frame is a thin lit rule in the middle of the screen rather
    // than a small card that happens to be there.
    const float wFull = 300.f;
    const float hFull = 166.f;
    const float w = wFull * (0.86f + 0.14f * open);
    const float h = hFull * (0.06f + 0.94f * open) * (0.06f + 0.94f * close);

    const float rise = (1.f - open) * -10.f + (1.f - close) * 8.f;
    const ImVec2 p0((display.x - w) * 0.5f, (display.y - h) * 0.5f + rise);
    const ImVec2 p1(p0.x + w, p0.y + h);
    const float rad = EnhRadius(22.f);
    const float cx  = (p0.x + p1.x) * 0.5f;

    // Seated, not ringed: two soft steps of shadow under the plate.
    for (int i = 3; i >= 1; --i)
    {
        const float e = (float)i * 3.f + 1.f;
        dl->AddRectFilled(ImVec2(p0.x - e, p0.y - e + 4.f), ImVec2(p1.x + e, p1.y + e + 6.f),
            IM_COL32(0, 0, 0, (int)(13.f * vis)), rad + e);
    }

    dl->AddRectFilled(p0, p1, SlateFill(10.f, 18), rad);
    // The plate's light, and no outline: the border was a second line around a
    // card that already has a shadow and a fill step doing the same job.
    GradV(dl, p0, p1, rad, IM_COL32(255, 255, 255, (int)(20.f * vis)),
        IM_COL32(255, 255, 255, 0));

    // The mark: one rounded tile.  No ring, no orbit - the ring was a second
    // circle around a square, and at 60 px it read as a badge rather than as
    // progress.
    const float ts = 44.f;
    const ImVec2 tc(cx, p0.y + 30.f + ts * 0.5f);

    if (content > 0.01f)
    {
        const int ca = (int)(255.f * content);
        dl->AddRectFilled(ImVec2(tc.x - ts * 0.5f, tc.y - ts * 0.5f),
                          ImVec2(tc.x + ts * 0.5f, tc.y + ts * 0.5f),
            IM_COL32(247, 247, 250, ca), ts * 0.30f);

        // One glint travelling over the tile as it settles, clipped to the
        // tile's inner rect so it never touches the rounded edge.
        const float sweep = ImClamp((open - 0.55f) / 0.45f, 0.f, 1.f);
        if (sweep > 0.f && sweep < 1.f)
        {
            const float sx = (tc.x - ts * 0.7f) + (ts * 1.4f) * EaseOutCubic(sweep);
            dl->PushClipRect(ImVec2(tc.x - ts * 0.5f + 2.f, tc.y - ts * 0.5f + 2.f),
                             ImVec2(tc.x + ts * 0.5f - 2.f, tc.y + ts * 0.5f - 2.f), true);
            dl->AddRectFilledMultiColor(ImVec2(sx - 8.f, tc.y - ts * 0.5f), ImVec2(sx, tc.y + ts * 0.5f),
                IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, (int)(22.f * content)),
                IM_COL32(255, 255, 255, (int)(22.f * content)), IM_COL32(255, 255, 255, 0));
            dl->PopClipRect();
        }

        DrawLogo(dl, tc, ts * 0.50f, IM_COL32(13, 13, 17, ca));

        const char* name = sync::kBrandName;
        const ImVec2 ns = TSizeBold(name, true);
        TxtBold(dl, name, ImVec2(cx - ns.x * 0.5f, tc.y + ts * 0.5f + 13.f),
            IM_COL32(240, 240, 244, ca), true);

        // The bar is the fill and nothing else - no track behind it.
        //
        // An unfilled line across the card is a grey rule that happens to have a
        // bright end, and at this width it is one more line in the background,
        // which is exactly what the panel just lost.  What is drawn is only how
        // much has arrived.
        const float bw = w * 0.56f;
        const float bx = cx - bw * 0.5f;
        const float by = p1.y - 44.f;

        const float pe = 1.f - (1.f - p) * (1.f - p);
        const float fw = 3.f + (bw - 3.f) * pe;
        dl->AddRectFilledMultiColor(ImVec2(bx, by), ImVec2(bx + fw, by + 3.f),
            IM_COL32(200, 202, 212, (int)(215.f * content)), IM_COL32(252, 252, 255, (int)(240.f * content)),
            IM_COL32(252, 252, 255, (int)(240.f * content)), IM_COL32(200, 202, 212, (int)(215.f * content)));

        char pc[8];
        snprintf(pc, sizeof pc, "%d%%", (int)(p * 100.f + 0.5f));
        const ImVec2 ps = TSize(g_FontSmall, pc);
        Txt(dl, g_FontSmall, ImVec2(bx, by + 13.f),
            IM_COL32(150, 150, 160, (int)(230.f * content)), "Loading");
        Txt(dl, g_FontSmall, ImVec2(bx + bw - ps.x, by + 13.f),
            IM_COL32(150, 150, 160, (int)(230.f * content)), pc);
    }

    g_AlphaMul = saved_mul;
    g_MenuFade = saved_fade;
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

    // The notice arrives rather than appearing: a short fade, a small rise, and
    // the rounded card and its shadow are drawn from the same radius so the
    // edge, the fill and the halo cannot disagree at any size of Enhance.
    static float s_gate_in = 0.f;
    s_gate_in = Approach(s_gate_in, 1.f, 6.5f, ImGui::GetIO().DeltaTime);
    const float gi   = s_gate_in;
    const float rise = (1.f - gi) * 16.f;
    const float rad  = 16.f + 9.f * Enh();

    const ImVec2 p0(g_GateX, g_GateY + rise);

    ImGui::SetNextWindowPos(ImVec2(g_GateX, g_GateY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rad);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Maxf(0.02f, gi));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));

    // The card's own colours go through GC so the fade reaches them too; the
    // widgets below read the style alpha pushed above.
    const float saved_mul = g_AlphaMul;
    g_AlphaMul = saved_mul * gi;

    ImGui::Begin("##aether_gate", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 q0(p0.x, p0.y);
    const ImVec2 q1(p0.x + w, p0.y + h);

    // A soft shadow in three steps rather than one hard halo: it has to read
    // over any game scene without cutting a black band around the card.
    for (int i = 3; i >= 1; --i)
    {
        const float o = 3.f * (float)i + 2.f;
        dl->AddRectFilled(ImVec2(q0.x - o, q0.y - o + 3.f), ImVec2(q1.x + o, q1.y + o + 3.f),
            GC(IM_COL32(0, 0, 0, 22)), rad + o);
    }

    DrawBackdropRect(dl, q0, q1, rad);
    dl->AddRectFilled(q0, q1, GC(IM_COL32(15, 16, 20, 252)), rad);

    // Neutral frame: the status reads from the text, a coloured ring only fought
    // whatever the card happened to sit over.  A lit top edge sits inside it, so
    // the card reads as a raised plate rather than a rectangle.
    dl->AddRect(q0, q1, GC(IM_COL32(255, 255, 255, 34)), rad, 0, 1.f);
    dl->AddRect(ImVec2(q0.x + 1.f, q0.y + 1.f), ImVec2(q1.x - 1.f, q1.y - 1.f),
        GC(IM_COL32(255, 255, 255, 16)), rad - 1.f, 0, 1.f);
    dl->AddLine(ImVec2(q0.x + rad * 0.9f, q0.y + 1.5f), ImVec2(q1.x - rad * 0.9f, q0.y + 1.5f),
        GC(IM_COL32(255, 255, 255, 30)), 1.f);

    const ImVec2 lc(q0.x + 28.f, q0.y + 30.f);
    DrawLogo(dl, lc, 15.f, GC(IM_COL32(238, 238, 242, 255)));
    TxtBold(dl, sync::kBrandName, ImVec2(lc.x + 14.f, lc.y - 8.f), GC(IM_COL32(238, 238, 242, 255)));

    // A hairline under the header, the way the panel's own header separates
    // itself, so the title is not floating on the same sheet as the body.
    dl->AddLine(ImVec2(q0.x + 28.f, q0.y + 46.f), ImVec2(q1.x - 28.f, q0.y + 46.f),
        GC(IM_COL32(255, 255, 255, 14)), 1.f);

    if (missing)
    {
        TxtBold(dl, "Roblox not found", ImVec2(q0.x + 28.f, q0.y + 58.f), GC(IM_COL32(240, 240, 244, 255)));
        Txt(dl, g_FontSmall, ImVec2(q0.x + 28.f, q0.y + 82.f), GC(IM_COL32(204, 204, 212, 255)),
            "Open Roblox to use the overlay.");

        char left[48];
        snprintf(left, sizeof left, "Closing in %d s", st.roblox_missing_seconds > 0 ? st.roblox_missing_seconds : 0);
        Txt(dl, g_FontSmall, ImVec2(q0.x + 28.f, q0.y + 104.f), GC(IM_COL32(226, 196, 106, 255)), left);

        const float bar_w = w - 56.f;
        const float bar_h = 5.f;
        const float frac  = Clampf((float)st.roblox_missing_seconds / 20.f, 0.f, 1.f);
        const float brad  = bar_h * 0.5f;
        const float by    = q1.y - 34.f;

        dl->AddRectFilled(ImVec2(q0.x + 28.f, by), ImVec2(q0.x + 28.f + bar_w, by + bar_h),
            GC(IM_COL32(255, 255, 255, 26)), brad);
        if (frac > 0.001f)
            dl->AddRectFilled(ImVec2(q0.x + 28.f, by), ImVec2(q0.x + 28.f + bar_w * frac, by + bar_h),
                GC(IM_COL32(226, 196, 106, 224)), brad);
    }
    else
    {
        TxtBold(dl, "Unsupported place!", ImVec2(q0.x + 28.f, q0.y + 56.f), GC(IM_COL32(240, 240, 244, 255)));
        Txt(dl, g_FontSmall, ImVec2(q0.x + 28.f, q0.y + 80.f), GC(IM_COL32(204, 204, 212, 255)),
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

    g_AlphaMul = saved_mul;
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
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
            g_TabSlideX = 0.f;
            g_TabRise   = 7.f;
            if (g_ActiveTab == kTabVisuals)
                SpringSetId(kPreviewSpringId, kPreviewClosedScale, 0.f);
        }
        else if (g_MenuOpen)
        {

            SpringSetId(kMenuSpringId, 1.f, 0.f);
            g_TabFade   = 1.f;
            g_TabSlideX = 0.f;
            g_TabRise   = 0.f;
        }
    }

    if (g_PrevMenuOpen != g_MenuOpen)
    {
        CancelKeyCapture();
        if (g_MenuOpen)
        {

            g_FlashT = 0.f;

            g_TabFade   = 0.f;
            g_TabSlideX = 0.f;
            g_TabRise   = 7.f;
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

    // Dead centre: a bubble grows out of its middle, and that is the whole
    // read.  Growing from an edge is a door opening; this is the panel arriving
    // in place, so the pivot is the middle of the plate.
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

    // Fade a little ahead of the size on the way out: a panel that is still
    // fully opaque at half size reads as shrinking, where one that is already
    // dissolving reads as leaving.
    g_MenuAlpha = (t < 0.001f) ? 0.f : BubbleAlpha(t) * (g_MenuOpen ? 1.f : t);

    uifx::note_menu_fade(g_MenuAlpha);

    g_MenuVisualFade = g_MenuAlpha;

    if (t > 0.002f || g_MenuAlpha > 0.002f)
	{        // No translation: the whole motion is the scale, so the panel inflates
        // and deflates around its own centre instead of sliding into place.
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

    DrawLoadingCard(displaySize);
}

namespace sync
{
	// The rectangle the capture is capped to: the panel while it is up, the
	// watermark card, and every open plate, with `pad` pixels of margin.  The
	// capture used to be the whole desktop, which is what made the game behind
	// the panel stutter - a megapixel of map was copied, mapped and blurred every
	// captured frame for an image that is only ever sampled by these rectangles.
	// With the cap the image becomes roughly a quarter of a 1080p screen, and the
	// margin keeps the blur from sampling past the edge of what was captured.
	bool panel_visible()
	{
		// The alpha, not the switch: the panel keeps being drawn while it closes,
		// and a capture taken in that half second still contains it.
		return g_MenuAlpha > 0.002f;
	}

	// The one or two frames the glass needs to take its first useful capture.
	//
	// The panel is off the screen for them - not the overlay, just the plate - so
	// the capture sees the game and nothing else.  It is the difference between a
	// blur that works the moment it is switched on and one that can only start if
	// you happen to close the menu, and it costs one flicker of the plate instead
	// of hiding the overlay from every capture API for as long as the effect runs.
	static bool g_PanelBlanked = false;

	void note_panel_blank(bool blank)
	{
		g_PanelBlanked = blank;
	}

	bool panel_blanked()
	{
		return g_PanelBlanked;
	}

	// Timed rather than latched: the settings page only calls it on the frames it
	// draws the section, which is what "on screen" means - scrolling away or
	// changing page stops the calls and the window lapses on its own.
	static unsigned long long g_BackdropSettingsAt = 0;

	void note_backdrop_settings()
	{
		g_BackdropSettingsAt = ::GetTickCount64();
	}

	bool backdrop_settings_visible()
	{
		return g_BackdropSettingsAt != 0 &&
		       ::GetTickCount64() - g_BackdropSettingsAt < 120ull;
	}

	bool panel_rect(float pad, float& x0, float& y0, float& x1, float& y1)
	{
		const float m = pad > 0.f ? pad : 0.f;
		x0 = g_PanelPos.x - m;
		y0 = g_PanelPos.y - m;
		x1 = g_PanelPos.x + K::W + m;
		y1 = g_PanelPos.y + K::H + m;
		return K::W > 1.f && K::H > 1.f;
	}

	bool hud_backdrop_rect(float pad, float& x0, float& y0, float& x1, float& y1,
		bool include_panel)
	{
		bool any = false;
		const auto add = [&](float a, float b, float c, float d)
		{
			if (!any) { x0 = a; y0 = b; x1 = c; y1 = d; any = true; return; }
			if (a < x0) x0 = a;
			if (b < y0) y0 = b;
			if (c > x1) x1 = c;
			if (d > y1) y1 = d;
		};

		const float m = pad > 0.f ? pad : 0.f;

		// The panel is added when it is on screen, or when the caller is asking
		// for the glass's pre-roll - the panel's rectangle captured while the
		// panel is shut, so the frame the glass holds when it opens is one the
		// panel was never in.
		if (include_panel || g_MenuAlpha > 0.002f)
			add(g_PanelPos.x - m, g_PanelPos.y - m,
				g_PanelPos.x + K::W + m, g_PanelPos.y + K::H + m);

		if (!settings::menu::hide_watermark && state().wm_show)
			add(g_CardPos.x - m, g_CardPos.y - m,
				g_CardPos.x + K::CardW + m, g_CardPos.y + K::CardH + m);

		for (int i = 0; i < settings::docks::kCount; ++i)
		{
			float w = 0.f, h = 0.f;
			if (!docks::plate_bounds(i, w, h))
				continue;
			add(settings::docks::x[i] - m, settings::docks::y[i] - m,
				settings::docks::x[i] + w + m, settings::docks::y[i] + h + m);
		}

		return any;
	}

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
		g_HostOpen = value;

		// The intro owns the screen for its three seconds.  The host starts with
		// the panel open, and honouring that on frame one is what hid the card.
		g_MenuOpen = g_BootHold ? false : g_HostOpen;
	}

	bool intro_hold()
	{
		return g_BootHold;
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
	// The overlay's intro, and the reason it is enforced here rather than in the
	// card: the host sets the open state every frame before this runs, so the
	// panel was on screen from the first frame and the card was drawn underneath
	// it.  While the hold is on, set_open() reports the panel shut whatever the
	// host says; when the intro's wall time is up the hold lifts and the
	// host's own answer - open - arrives on the next frame.
    if (g_BootHold)
    {
        const float t = (float)ImGui::GetTime();
        if (g_BootStart < 0.f)
            g_BootStart = t;
        else if (t - g_BootStart >= kIntroSeconds)
            g_BootHold = false;
    }

	anim::begin_frame();

	// Enhance first: it rescales the layout and moves the radii, and everything
	// below - the theme's rounding, the ImGui style, the whole layout - reads what
	// it leaves behind.
	EnhanceTick(ImGui::GetIO().DeltaTime);

	ApplyStyle();

	// The ImGui colours are the theme's too (the popup background is the plate's
	// own fill), so they follow a live change to the theme or to No Transparency
	// - init() alone would leave the popups on the palette that was loaded.
	ApplyImGuiStyle();

		UpdateGate();

		sync::occlusion().n = 0;

		if (g_PanelPos.x < 0.f || g_PanelPos.y < 0.f)
		{
			g_PanelPos.x = Clampf((display_size.x - K::W) * 0.5f, 2.f,
								  Maxf(2.f, display_size.x - K::W - 2.f));
			g_PanelPos.y = Clampf(K::PanelY, 2.f,
								  Maxf(2.f, display_size.y - K::H - 2.f));

			// The spring starts where the panel does.  Without this the first
			// frame would pull it in from (0,0) - a slide of half the screen.
			g_PanelTarget = g_PanelPos;
			g_PanelVel    = ImVec2(0.f, 0.f);
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
