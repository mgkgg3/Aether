#pragma once

#include "SyncBridge.h"

#include <settings.h>
#include <keybind/keybind.h>

#include <gui/menu/Anim.h>
#include <gui/menu/AimViz.h>
#include <gui/menu/HomePage.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

#include <features/esp/BoxFill.h>
#include <features/misc/HitSounds.h>
#include <features/misc/SkyVideo.h>
#include <features/players/friends.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>

static float g_PreviewScaleDbg = 1.f;

static const ImGuiID kPreviewSpringId    = 0xE5A1B2C3u;
static const float   kPreviewClosedScale = kBubbleClosed;

static inline float RowNameH()
{
    return (g_FontBold ? g_FontBold : g_FontRegular)->FontSize;
}

static void SectionHead(const char* t)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    g_RowY += (g_RowY > g_ContentTop + 2.f) ? 16.f : 1.f;
    TxtBold(dl, t, ImVec2(g_ContentL, g_RowY), GC(g_T.textPrimary));

    g_RowY += RowNameH() + 10.f;
}

static void NoteRow(const char* t)
{
    Txt(ImGui::GetWindowDrawList(), g_FontSmall, ImVec2(g_ContentL, g_RowY), GC(g_T.textFaint), t);
    g_RowY += g_FontSmall->FontSize + 9.f;
}

static void DividerRow()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    g_RowY += 5.f;
    dl->AddLine(ImVec2(g_ContentL, g_RowY), ImVec2(g_ContentR, g_RowY), GC(IM_COL32(255, 255, 255, 14)), 1.f);
    g_RowY += 13.f;
}

static void PushPopupStyle()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, g_T.roundPopup);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_T.popupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, g_T.windowBorder);
}

static void PopPopupStyle()
{
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

namespace keycap
{
    inline int* target = nullptr;
}

static void BeginKeyCapture(int* t)
{
    keycap::target = (keycap::target == t) ? nullptr : t;
}

static bool KeyCapturing(const int* t)
{
    return t != nullptr && keycap::target == t;
}

static void CancelKeyCapture()
{
    keycap::target = nullptr;
}

static const int kMouseVk[4] = { VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };

static void PollKeyCapture()
{
    int* t = keycap::target;
    if (!t)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { *t = 0; keycap::target = nullptr; return; }

    for (int i = 0; i < 4; i++)
        if (ImGui::GetIO().MouseDown[i + 1]) { *t = kMouseVk[i]; keycap::target = nullptr; return; }

    for (int vk = 1; vk <= 0xFE; vk++)
    {
        if (vk == VK_LBUTTON) continue;
        if (GetAsyncKeyState(vk) & 0x8000) { *t = vk; keycap::target = nullptr; return; }
    }
}

static const char* KeyLabel(int vk, char* buf, size_t n)
{
    if (vk <= 0) { snprintf(buf, n, "None"); return buf; }
    const int names = (int)(sizeof(key_namess) / sizeof(key_namess[0]));
    if (vk < names && key_namess[vk] && strcmp(key_namess[vk], "Unknown") != 0)
        snprintf(buf, n, "%s", key_namess[vk]);
    else
        snprintf(buf, n, "0x%02X", vk);
    return buf;
}

static bool KeybindControlVk(const char* tag, ImVec2 pos, ImVec2 size, int* key)
{
    ImGui::PushID(tag);
    ImGui::SetCursorScreenPos(pos);
    const bool pressed = ImGui::InvisibleButton("##hit", size);
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) BeginKeyCapture(key);

    const bool listening = KeyCapturing(key);
    Feedback fb = FeedbackUpdate(hovered, pressed, listening);
    if (listening)
    {
        const float pulse = 0.55f + 0.45f * (0.5f + 0.5f * sinf((float)ImGui::GetTime() * 6.5f));
        fb.shine = Maxf(fb.shine, pulse);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = pos, b(pos.x + size.x, pos.y + size.y);
    const float rad = size.y * 0.5f >= 15.f ? g_T.roundControl : size.y * 0.5f;

    AddGlow(dl, a, b, rad, fb.shine, listening ? 8.f : 5.f);
    AddRipple(dl, a, b, rad, fb.ripple);
    dl->AddRectFilled(a, b, GC(Mix(g_T.controlBg, IM_COL32(255, 255, 255, 40), fb.shine * 0.5f)), rad);
    dl->AddRect(a, b, GC(Mix(g_T.controlBorder, g_T.controlBorderHi, Maxf(fb.shine * 0.8f, hovered ? 0.35f : 0.f))),
                rad, 0, g_T.borderThickness);

    char keys[48];
    const char* label = listening ? "Listening..." : KeyLabel(*key, keys, sizeof keys);
    const ImVec2 ts = TSize(g_FontRegular, label);
    Txt(dl, g_FontRegular, ImVec2((a.x + b.x) * 0.5f - ts.x * 0.5f, (a.y + b.y) * 0.5f - ts.y * 0.5f),
        GC(Mix(g_T.textPrimary, g_T.textDim, listening ? 0.35f : 0.f)), label);

    ImGui::PopID();
    return pressed;
}

static void RowKeybox(const char* tag, float y, float h, int* key)
{
    const ImVec2 size(96.f, 28.f);
    KeybindControlVk(tag, ImVec2(g_ContentR - size.x, y + (h - size.y) * 0.5f), size, key);
}

static bool RowToggleBind(const char* id, const char* label, bool* value, int* key,
                          const int* activation = nullptr)
{
    ImGui::PushID(id);
    const float rowH = K::ToggleH + 2.f * K::RowPad;
    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    const ImVec2 tpos(g_ContentL, g_RowY + K::RowPad);
    const bool before = *value;
    const bool pressed = ToggleControl("tgl", tpos, value);
    if (*value != before) PushToast(label, *value);

    Txt(ImGui::GetWindowDrawList(), g_FontRegular,
        ImVec2(tpos.x + K::ToggleW + 12.f, g_RowY + (rowH - g_FontRegular->FontSize) * 0.5f),
        GC(g_T.textPrimary), label);

    const bool show_key = key && !(activation && *activation == 2);
    if (show_key) RowKeybox("key", g_RowY, rowH, key);

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
    return pressed;
}

static void RowBind(const char* id, const char* label, const char* desc, int* key)
{
    ImGui::PushID(id);
    const float nameH = RowNameH();
    const float textH = nameH + (desc ? 3.f + g_FontSmall->FontSize : 0.f);
    const float rowH  = Maxf(textH, 28.f) + 2.f * K::RowPad;

    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ty = g_RowY + (rowH - textH) * 0.5f;
    TxtBold(dl, label, ImVec2(g_ContentL, ty), GC(g_T.textPrimary));
    if (desc) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + nameH + 3.f), GC(g_T.textDim), desc);

    RowKeybox("key", g_RowY, rowH, key);

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
}

static void RowSliderF(const char* id, const char* name, float* value, float vmin, float vmax, const char* fmt)
{
    char buf[48];
    snprintf(buf, sizeof buf, fmt, *value);
    RowSlider(id, name, value, vmin, vmax, buf, false, nullptr);
}

static void RowSliderI(const char* id, const char* name, int* value, int vmin, int vmax, const char* fmt)
{
    float f = (float)*value;
    char buf[48];
    snprintf(buf, sizeof buf, fmt, *value);
    RowSlider(id, name, &f, (float)vmin, (float)vmax, buf, false, nullptr);
    const int snapped = (int)(f + (f < 0.f ? -0.5f : 0.5f));
    *value = snapped < vmin ? vmin : (snapped > vmax ? vmax : snapped);
}

static ImU32 Col32(const float* c)
{
    return IM_COL32((int)(Clampf(c[0], 0.f, 1.f) * 255.f + 0.5f),
                    (int)(Clampf(c[1], 0.f, 1.f) * 255.f + 0.5f),
                    (int)(Clampf(c[2], 0.f, 1.f) * 255.f + 0.5f),
                    (int)(Clampf(c[3], 0.f, 1.f) * 255.f + 0.5f));
}

static void ColorRow(const char* id, const char* name, const char* desc, float* col, bool alpha)
{
    static const ImU32 kPresets[] = {
        IM_COL32(244, 244, 246, 255), IM_COL32(232,  92,  92, 255), IM_COL32(247, 173,  74, 255),
        IM_COL32(238, 219, 104, 255), IM_COL32(110, 210, 130, 255), IM_COL32( 96, 176, 240, 255),
        IM_COL32(158, 132, 240, 255), IM_COL32(240, 132, 200, 255),
    };

    ImGui::PushID(id);
    const float nameH = RowNameH();
    const float textH = nameH + (desc ? 3.f + g_FontSmall->FontSize : 0.f);
    const float rowH  = Maxf(textH, K::SwatchD) + 2.f * K::RowPad;
    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ty = g_RowY + (rowH - textH) * 0.5f;
    TxtBold(dl, name, ImVec2(g_ContentL, ty), GC(g_T.textPrimary));
    if (desc) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + nameH + 3.f), GC(g_T.textDim), desc);

    const float r = K::SwatchD * 0.5f;
    const ImVec2 center(g_ContentR - r, g_RowY + rowH * 0.5f);
    const ImVec2 a(center.x - r, center.y - r), b(center.x + r, center.y + r);
    const ImU32 cur = Col32(col);

    ImGui::SetCursorScreenPos(a);
    const bool pressed = ImGui::InvisibleButton("##sw", ImVec2(K::SwatchD, K::SwatchD));
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) ImGui::OpenPopup("##picker");

    const Feedback fb = FeedbackUpdate(hovered, pressed);
    AddGlow(dl, a, b, r, fb.shine, 7.f);
    AddRipple(dl, a, b, r, fb.ripple);
    dl->AddCircleFilled(center, r + fb.shine, GC(cur), 48);
    dl->AddCircle(center, r + fb.shine, GC(Mix(g_T.windowBorder, g_T.controlBorderHi, fb.shine)), 48, 1.2f);

    PushPopupStyle();
    ImGui::SetNextWindowPos(ImVec2(b.x - 176.f, b.y + 7.f), ImGuiCond_Always);
    if (ImGui::BeginPopup("##picker"))
    {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        const float cr = 13.f;
        for (int i = 0; i < IM_ARRAYSIZE(kPresets); i++)
        {
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##p", ImVec2(30.f, 30.f));
            const ImVec2 c(p.x + 15.f, p.y + 15.f);
            const float pr = ImGui::IsItemHovered() ? cr + 1.5f : cr;
            pdl->AddCircleFilled(c, pr, GC(kPresets[i]), 32);
            if (ImGui::IsItemClicked())
            {
                col[0] = ((kPresets[i] >> IM_COL32_R_SHIFT) & 0xFF) / 255.f;
                col[1] = ((kPresets[i] >> IM_COL32_G_SHIFT) & 0xFF) / 255.f;
                col[2] = ((kPresets[i] >> IM_COL32_B_SHIFT) & 0xFF) / 255.f;
                col[3] = alpha ? ((kPresets[i] >> IM_COL32_A_SHIFT) & 0xFF) / 255.f : 1.f;
                ImGui::CloseCurrentPopup();
            }
            if ((i % 4) != 3) ImGui::SameLine(0.f, 8.f);
            ImGui::PopID();
        }

        ImGui::Dummy(ImVec2(1.f, 4.f));
        ImGui::SetNextItemWidth(178.f);
        const ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel |
                                          (alpha ? ImGuiColorEditFlags_AlphaBar : ImGuiColorEditFlags_NoAlpha);
        ImGui::ColorPicker4("##pick", col, flags);
        ImGui::EndPopup();
    }
    PopPopupStyle();

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
}

static void ColorRow3(const char* id, const char* name, const char* desc, float* col)
{

    float rgba[4] = { col[0], col[1], col[2], 1.f };
    ColorRow(id, name, desc, rgba, false);
    col[0] = rgba[0]; col[1] = rgba[1]; col[2] = rgba[2];
}

static void RowMulti(const char* id, const char* name, const char* desc, unsigned* mask,
                     const char* const* items, int count)
{
    ImGui::PushID(id);
    const float nameH = RowNameH();
    const float textH = nameH + (desc ? 3.f + g_FontSmall->FontSize : 0.f);
    const float rowH  = Maxf(textH, K::ComboH) + 2.f * K::RowPad;
    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + rowH + 2.f));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ty = g_RowY + (rowH - textH) * 0.5f;
    TxtBold(dl, name, ImVec2(g_ContentL, ty), GC(g_T.textPrimary));
    if (desc) Txt(dl, g_FontSmall, ImVec2(g_ContentL, ty + nameH + 3.f), GC(g_T.textDim), desc);

    const ImVec2 a(g_ContentR - K::ComboW, g_RowY + (rowH - K::ComboH) * 0.5f);
    const ImVec2 b(a.x + K::ComboW, a.y + K::ComboH);
    ImGui::SetCursorScreenPos(a);
    const bool pressed = ImGui::InvisibleButton("##hit", ImVec2(K::ComboW, K::ComboH));
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) ImGui::OpenPopup("##popup");
    const bool open = ImGui::IsPopupOpen("##popup");

    const Feedback fb = FeedbackUpdate(hovered, pressed, open);
    AddGlow(dl, a, b, g_T.roundControl, fb.shine);
    dl->AddRectFilled(a, b, GC(Mix(g_T.controlBg, IM_COL32(255, 255, 255, 34), fb.shine * 0.55f)), g_T.roundControl);
    dl->AddRect(a, b, GC(Mix(g_T.controlBorder, g_T.controlBorderHi, fb.shine * 0.85f)), g_T.roundControl, 0, g_T.borderThickness);

    int on = 0;
    for (int i = 0; i < count; i++) if (*mask & (1u << i)) ++on;
    char label[48];
    if (on == 0)      snprintf(label, sizeof label, "None");
    else if (on == 1) { int only = 0; for (int i = 0; i < count; i++) if (*mask & (1u << i)) only = i;
                        snprintf(label, sizeof label, "%s", items[only]); }
    else              snprintf(label, sizeof label, "%d selected", on);

    Txt(dl, g_FontRegular, ImVec2(a.x + 12.f, (a.y + b.y) * 0.5f - g_FontRegular->FontSize * 0.5f), GC(g_T.textPrimary), label);
    AddChevron(dl, ImVec2(b.x - 16.f, (a.y + b.y) * 0.5f), 4.5f, 2.8f, GC(g_T.textPrimary), 2.0f);

    PushPopupStyle();
    ImGui::SetNextWindowPos(ImVec2(b.x - (K::ComboW + 14.f), b.y + 6.f), ImGuiCond_Always);
    if (ImGui::BeginPopup("##popup"))
    {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        for (int i = 0; i < count; i++)
        {
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const ImVec2 isz(K::ComboW, 26.f);
            ImGui::InvisibleButton("##item", isz);
            const bool ih = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) *mask ^= (1u << i);
            if (ih) pdl->AddRectFilled(p, ImVec2(p.x + isz.x, p.y + isz.y), GC(g_T.rowHover), 7.f);

            const bool on_i = (*mask & (1u << i)) != 0;
            const ImVec2 tc(p.x + 10.f, p.y + (isz.y - g_FontRegular->FontSize) * 0.5f);
            Txt(pdl, g_FontRegular, tc, GC(on_i ? g_T.textPrimary : g_T.textDim), items[i]);
            if (on_i)
            {
                const ImVec2 ts = TSize(g_FontRegular, items[i]);
                const ImVec2 c(tc.x + ts.x + 11.f, tc.y + g_FontRegular->FontSize * 0.5f);
                pdl->AddLine(ImVec2(c.x - 3.6f, c.y), ImVec2(c.x - 1.0f, c.y + 3.0f), GC(g_T.textPrimary), 1.5f);
                pdl->AddLine(ImVec2(c.x - 1.0f, c.y + 3.0f), ImVec2(c.x + 3.8f, c.y - 3.4f), GC(g_T.textPrimary), 1.5f);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    PopPopupStyle();

    g_RowY += rowH + K::RowGap;
    ImGui::PopID();
}

static bool RowButton(const char* id, const char* label, bool primary = false)
{
    const ImVec2 size(120.f, 30.f);
    const bool hit = ButtonControl(id, ImVec2(g_ContentL, g_RowY), size, label, primary, nullptr);
    g_RowY += size.y + K::RowGap;
    return hit;
}

static bool RowInput(const char* id, const char* name, char* buf, size_t n, const char* hint, float fieldW = 190.f)
{
    ImGui::PushID(id);
    const float h = 32.f;
    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + h + 2.f));
    TxtBold(ImGui::GetWindowDrawList(), name, ImVec2(g_ContentL, g_RowY + (h - RowNameH()) * 0.5f), GC(g_T.textPrimary));

    ImGui::SetCursorScreenPos(ImVec2(g_ContentR - fieldW, g_RowY + 1.f));
    ImGui::SetNextItemWidth(fieldW);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9.f, 6.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(g_T.controlBg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1, 1, 1, 0.10f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1, 1, 1, 0.13f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(g_T.controlBorder));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(g_T.textPrimary));
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::ColorConvertU32ToFloat4(g_T.textFaint));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(1, 1, 1, 0.16f));
    const bool changed = ImGui::InputTextWithHint("##in", hint, buf, n,
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(3);

    g_RowY += h + K::RowGap;
    ImGui::PopID();
    return changed;
}

static void StatusRow(const char* label, const char* value, bool ok)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 24.f;
    Txt(dl, g_FontRegular, ImVec2(g_ContentL, g_RowY + (h - g_FontRegular->FontSize) * 0.5f), GC(g_T.textDim), label);
    const ImVec2 ts = TSize(g_FontRegular, value);
    Txt(dl, g_FontRegular, ImVec2(g_ContentR - ts.x, g_RowY + (h - g_FontRegular->FontSize) * 0.5f),
        GC(ok ? IM_COL32(126, 214, 132, 255) : IM_COL32(226, 104, 104, 255)), value);
    g_RowY += h + 3.f;
}

static const char* kAimSubs[] = { "Global", "Silent Aim", "Triggerbot", "Analysis" };

static const char* const* ActivationOptions()
{
    static const char* const v[] = { "Toggle", "Hold", "Always" };
    return v;
}

static const char* const* BoneOptions()
{
    static const char* const v[] = { "Closest", "Head", "Torso", "L.Arm", "R.Arm", "L.Leg", "R.Leg" };
    return v;
}

static const char* const* SilentTargets()
{
    static const char* const v[] = { "Surface", "Closest", "Head", "Torso", "L.Arm", "R.Arm", "L.Leg", "R.Leg", "Random" };
    return v;
}

static const char* const* TracerOrigins()
{
    static const char* const v[] = { "Bottom", "Centre", "Mouse", "Top" };
    return v;
}

static void DrawAimGlobal()
{
    static const char* const k_aim_method[] = { "Mouse", "Camera" };
    static const char* const k_styles[] = {
        "Default", "Linear", "EaseInQuad", "EaseOutQuad", "EaseInOutQuad",
        "EaseInCubic", "EaseOutCubic", "EaseInOutCubic", "EaseInSine", "EaseOutSine", "EaseInOutSine"
    };

    PageHeader("Aim", "Camera lock, smoothing and hitbox selection.", nullptr);

    SectionHead("Aimbot");
    RowToggleBind("aim_enabled", "Enable Aimbot", &settings::aimbot::enabled, &settings::aimbot::keybind,
                  &settings::aimbot::activation_mode);
    RowCombo("aim_act", "Activation", "Whether the key holds the aim on", &settings::aimbot::activation_mode, ActivationOptions(), 3);
    if (settings::aimbot::activation_mode == 2)
        NoteRow("Always on - no key needed");
    RowToggle("aim_drawfov", "Draw FOV Circle", &settings::aimbot::draw_fov);
    if (settings::aimbot::draw_fov)
    {
        ColorRow("aim_fovcol", "Circle Colour", nullptr, settings::aimbot::fov_circle_colour, true);
        ColorRow("aim_fovout", "Outline Colour", nullptr, settings::aimbot::fov_outline_colour, true);
        RowToggle("aim_fovfill", "Fill Circle", &settings::aimbot::fov_fill);
        if (settings::aimbot::fov_fill)
        {
            ColorRow("aim_fovfillcol", "Fill Colour", nullptr, settings::aimbot::fov_fill_colour, true);
            RowSliderF("aim_fovfillop", "Fill Opacity", &settings::aimbot::fov_fill_opacity, 0.f, 100.f, "%.0f %%");
        }
    }
    RowToggle("aim_instant", "Instant Lock", &settings::aimbot::instant_lock);
    RowToggle("aim_sticky", "Sticky Aim", &settings::aimbot::sticky_aim);
    RowSliderI("aim_pace", "Aim Step Interval", &settings::aimbot::pace_ms, 0, 32, "%d ms");
    NoteRow(settings::aimbot::pace_ms > 0
        ? "How often a mouse step goes out - a game frame is the honest floor"
        : "0 follows the FPS limit (Config > Settings)");
    RowToggle("aim_usefov", "Use FOV", &settings::aimbot::use_fov);

    SectionHead("Targeting");
    RowCombo("aim_method", "Aim Method", "Mouse writes the cursor, Camera the view", &settings::aimbot::mode, k_aim_method, 2);
    RowCombo("aim_bone", "Target Bone", "Which part of the rig is aimed at", &settings::aimbot::target_part, BoneOptions(), 7);
    RowToggle("aim_air", "Air Bone", &settings::aimbot::air_part_enabled);
    if (settings::aimbot::air_part_enabled)
        RowCombo("aim_airbone", "Air Bone", "Used while a target is off the ground", &settings::aimbot::air_part, BoneOptions(), 7);

    static const char* const k_priority[] = { "Crosshair", "Distance", "Health" };
    if (settings::aimbot::priority < 0 || settings::aimbot::priority > 2)
        settings::aimbot::priority = 0;
    RowCombo("aim_priority", "Priority", "Who wins when several are in the FOV",
             &settings::aimbot::priority, k_priority, 3);
    if (settings::aimbot::priority == 0)
        NoteRow("The body under the crosshair wins, then the nearest to it");
    else if (settings::aimbot::priority == 1)
        NoteRow("The closest player wins");
    else
        NoteRow("The player with the least health wins");

    RowToggle("aim_hp", "Health Check", &settings::aimbot::health_check_enabled);
    if (settings::aimbot::health_check_enabled)
        RowSliderF("aim_minhp", "Min Health", &settings::aimbot::min_health, 0.f, 100.f, "%.0f");

    SectionHead("Tuning");
    RowSliderF("aim_fov", "Field of View", &settings::aimbot::fov, 10.f, 800.f, "%.0f deg");
    RowToggle("aim_smooth", "Smoothing", &settings::aimbot::smoothing);
    if (settings::aimbot::smoothing)
    {
        RowSliderF("aim_smx", "Smoothness X", &settings::aimbot::smoothingx, 0.f, 100.f, "%.0f%%");
        RowSliderF("aim_smy", "Smoothness Y", &settings::aimbot::smoothingy, 0.f, 100.f, "%.0f%%");
        RowCombo("aim_smstyle", "Smoothing Style", "Curve the aim travels along", &settings::aimbot::smoothing_style, k_styles, 11);
    }
    RowToggle("aim_pred", "Prediction", &settings::aimbot::enable_prediction);
    if (settings::aimbot::enable_prediction)
    {
        RowSliderF("aim_predx", "Prediction X", &settings::aimbot::prediction_x, 0.f, 100.f, "%.1f");
        RowSliderF("aim_predy", "Prediction Y", &settings::aimbot::prediction_y, 0.f, 100.f, "%.1f");
    }
    RowToggle("aim_airpred", "Air Prediction", &settings::aimbot::air_prediction_enabled);
    if (settings::aimbot::air_prediction_enabled)
    {
        RowSliderF("aim_airpredx", "Air Prediction X", &settings::aimbot::air_prediction_x, 0.f, 100.f, "%.1f");
        RowSliderF("aim_airpredy", "Air Prediction Y", &settings::aimbot::air_prediction_y, 0.f, 100.f, "%.1f");
    }

    SectionHead("Filters");
    RowToggle("aim_dead", "Ignore Dead", &settings::aimbot::dead_check);
    RowToggle("aim_knock", "Ignore Knocked", &settings::aimbot::knock_check);
    RowToggle("aim_team", "Team Check", &settings::aimbot::teamcheck);
    RowToggle("aim_visible", "Visible Check", &settings::aimbot::visible_check);
    RowToggle("aim_offset", "Offset Aim", &settings::aimbot::offset_enabled);
    if (settings::aimbot::offset_enabled)
    {
        RowSliderF("aim_offx", "Offset X", &settings::aimbot::offset_x, -50.f, 50.f, "%.1f");
        RowSliderF("aim_offy", "Offset Y", &settings::aimbot::offset_y, -50.f, 50.f, "%.1f");
    }
    RowToggle("aim_tracer", "Aim Tracer", &settings::aimbot::tracer);
    if (settings::aimbot::tracer)
    {
        RowCombo("aim_trorigin", "Tracer Origin", nullptr, &settings::aimbot::tracer_origin, TracerOrigins(), 4);
        ColorRow("aim_trcol", "Tracer Colour", nullptr, settings::aimbot::tracer_color, true);
    }
}

static void DrawAimSilent()
{
    static const char* const k_methods[] = { "Viewport", "Memory", "Raycast", "PF Silent", "Mouse" };

    PageHeader("Aim", "Shots bent onto a target without moving the camera.", nullptr);

    SectionHead("Silent Aim");
    RowToggleBind("sa_enabled", "Enable Silent Aim", &settings::silentaim::enabled, &settings::silentaim::keybind,
                  &settings::silentaim::activation_mode);
    RowCombo("sa_act", "Activation", nullptr, &settings::silentaim::activation_mode, ActivationOptions(), 3);
    if (settings::silentaim::activation_mode == 2)
        NoteRow("Always on - no key needed");
    RowCombo("sa_method", "Method", "How the shot is redirected", &settings::silentaim::method, k_methods, 5);
    RowToggle("sa_drawfov", "Draw FOV", &settings::silentaim::draw_fov);
    if (settings::silentaim::draw_fov)
    {
        ColorRow("sa_fovcol", "Circle Colour", nullptr, settings::silentaim::fov_circle_colour, true);
        ColorRow("sa_fovout", "Outline Colour", nullptr, settings::silentaim::fov_outline_colour, true);
        RowToggle("sa_fovfill", "Fill Circle", &settings::silentaim::fov_fill);
        if (settings::silentaim::fov_fill)
        {
            ColorRow("sa_fovfillcol", "Fill Colour", nullptr, settings::silentaim::fov_fill_colour, true);
            RowSliderF("sa_fovfillop", "Fill Opacity", &settings::silentaim::fov_fill_opacity, 0.f, 100.f, "%.0f %%");
        }
    }

    SectionHead("Behaviour");
    RowCombo("sa_target", "Target Bone", "Random picks one part per target lock", &settings::silentaim::target_part, SilentTargets(), 9);
    RowToggle("sa_useaim", "Use Aimbot Target", &settings::silentaim::use_aimbot_target);
    RowToggle("sa_sticky", "Sticky Aim", &settings::silentaim::sticky_aim);
    RowToggle("sa_spoof", "Spoof Mouse", &settings::silentaim::spoof_mouse);
    RowToggle("sa_team", "Team Check", &settings::silentaim::teamcheck);
    RowToggle("sa_dead", "Dead Check", &settings::silentaim::dead_check);
    RowToggle("sa_vis", "Visible Check", &settings::silentaim::visible_check);
    RowToggle("sa_knock", "Knock Check", &settings::silentaim::knock_check);
    RowToggle("sa_hp", "Health Check", &settings::silentaim::health_check_enabled);
    if (settings::silentaim::health_check_enabled)
        RowSliderF("sa_minhp", "Min Health", &settings::silentaim::min_health, 0.f, 100.f, "%.0f");

    SectionHead("Tuning");
    RowSliderF("sa_fov", "Field of View", &settings::silentaim::fov, 1.f, 360.f, "%.0f deg");
    RowToggle("sa_usefov", "Use FOV", &settings::silentaim::use_fov);
    RowToggle("sa_chance", "Hit Chance", &settings::silentaim::hit_chance_enabled);
    if (settings::silentaim::hit_chance_enabled)
    {
        RowSliderF("sa_chanceval", "Hit Chance", &settings::silentaim::hit_chance, 0.f, 100.f, "%.0f%%");
        RowSliderF("sa_chancerate", "Fire Rate", &settings::silentaim::hit_chance_fire_rate, 1.f, 30.f, "%.0f/s");
    }
    RowToggle("sa_pred", "Prediction", &settings::silentaim::enable_prediction);
    if (settings::silentaim::enable_prediction)
    {
        RowSliderF("sa_predx", "Prediction X", &settings::silentaim::prediction_x, 0.f, 100.f, "%.1f");
        RowSliderF("sa_predy", "Prediction Y", &settings::silentaim::prediction_y, 0.f, 100.f, "%.1f");
    }
    RowToggle("sa_antipred", "Auto Predict", &settings::silentaim_autopred::enabled);
    if (settings::silentaim_autopred::enabled)
        RowSliderF("sa_antipredstr", "Ping Strength", &settings::silentaim_autopred::strength, 0.2f, 3.f, "%.2f");

    SectionHead("Draw");
    RowToggle("sa_dot", "Draw Target Dot", &settings::silentaim::draw_target_dot);
    if (settings::silentaim::draw_target_dot)
    {
        ColorRow("sa_dotcol", "Dot Colour", nullptr, settings::silentaim::target_dot_color, true);
        RowSliderF("sa_dotsize", "Dot Size", &settings::silentaim::target_dot_size, 1.f, 12.f, "%.1f");
    }
    RowToggle("sa_curvetracer", "Draw Curve Tracer", &settings::silentaim::draw_curve_tracer);
    if (settings::silentaim::draw_curve_tracer)
    {
        ColorRow("sa_curvetcol", "Tracer Colour", nullptr, settings::silentaim::curve_tracer_color, true);
        RowSliderF("sa_curvetth", "Tracer Thickness", &settings::silentaim::curve_tracer_thickness, 0.5f, 6.f, "%.1f px");
    }
}

static void DrawAimTrigger()
{
    static const char* const k_targets[] = { "Any", "Head", "Torso" };

    PageHeader("Aim", "Fires for you the moment the crosshair is on a target.", nullptr);

    SectionHead("Triggerbot");
    RowToggleBind("tb_enabled", "Enable Triggerbot", &settings::triggerbot::enabled, &settings::triggerbot::keybind,
                  &settings::triggerbot::activation_mode);
    RowCombo("tb_act", "Activation", nullptr, &settings::triggerbot::activation_mode, ActivationOptions(), 3);
    RowCombo("tb_target", "Target", "Any part, or the crosshair on the head / torso", &settings::triggerbot::target_part, k_targets, 3);
    RowToggle("tb_cont", "Continuous Fire", &settings::triggerbot::continuous);
    RowToggle("tb_vis", "Visibility Check", &settings::triggerbot::visibility_check);
    RowToggle("tb_gun", "Gun Check", &settings::triggerbot::gun_check);
    RowToggle("tb_team", "Team Check", &settings::triggerbot::team_check);
    RowToggle("tb_knock", "Knock Check", &settings::triggerbot::knock_check);
    RowToggle("tb_dead", "Dead Check", &settings::triggerbot::dead_check);

    SectionHead("Weapon");
    RowSliderF("tb_delay", "Click Delay", &settings::triggerbot::delay_ms, 0.f, 500.f, "%.0f ms");
    RowSliderF("tb_fov", "Tolerance", &settings::triggerbot::fov, 0.f, 100.f, "%.0f px");
    NoteRow(settings::triggerbot::target_part ? "Head / Torso need the crosshair on that part"
                                             : "Tolerance slack applies to the Any target");
    RowToggle("tb_drawtol", "Draw Tolerance", &settings::triggerbot::draw_fov);
    if (settings::triggerbot::draw_fov)
    {
        ColorRow("tb_tolcol", "Circle Colour", nullptr, settings::triggerbot::fov_circle_colour, true);
        ColorRow("tb_tolout", "Outline Colour", nullptr, settings::triggerbot::fov_outline_colour, true);
        RowToggle("tb_tolfill", "Fill Circle", &settings::triggerbot::fov_fill);
        if (settings::triggerbot::fov_fill)
        {
            ColorRow("tb_tolfillcol", "Fill Colour", nullptr, settings::triggerbot::fov_fill_colour, true);
            RowSliderF("tb_tolfillop", "Fill Opacity", &settings::triggerbot::fov_fill_opacity, 0.f, 100.f, "%.0f %%");
        }
    }

    SectionHead("Other Aim");
    RowToggle("aim_hbe_on", "Hitbox Expander", &settings::hitbox_expander::enabled);
    if (settings::hitbox_expander::enabled)
    {
        RowSliderF("aim_hbe", "Size X", &settings::hitbox_expander::size_x, 0.5f, 8.f, "%.2f");
        RowSliderF("aim_hbe_y", "Size Y", &settings::hitbox_expander::size_y, 0.5f, 8.f, "%.2f");
        RowSliderF("aim_hbe_z", "Size Z", &settings::hitbox_expander::size_z, 0.5f, 8.f, "%.2f");
        RowToggle("aim_hbe_knock", "Ignore Knocked", &settings::hitbox_expander::knock_check);
    }
    RowToggle("aim_rapid", "Rapid Fire", &settings::rage::rapidfire);
}

static void DrawAimAnalysis()
{
    static const char* const k_pin[]   =
    {
        "Auto", "Default", "Linear", "EaseInQuad", "EaseOutQuad", "EaseInOutQuad",
        "EaseInCubic", "EaseOutCubic", "EaseInOutCubic", "EaseInSine", "EaseOutSine", "EaseInOutSine"
    };

    PageHeader("Aim", nullptr, nullptr);

    SectionHead("Smoothing preview");
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        g_RowY += 2.f;
        const float h = 240.f;
        aim_viz::preview(dl, ImVec2(g_ContentL, g_RowY), ImVec2(g_ContentR, g_RowY + h), g_AlphaMul);
        g_RowY += h + 14.f;
    }

    RowToggle("viz_cycle", "Cycle Styles", &settings::viz::demo_switch,
        "Walk the family until the aim smooths (Aim > Tuning)");
    if (settings::viz::demo_switch && settings::viz::pin_style < 0)
        RowSliderF("viz_cycle_s", "Time Each", &settings::viz::demo_speed, 0.8f, 12.f, "%.1f s");
    {

        static int pin = 0;
        if (pin == 0 && settings::viz::pin_style >= 0) pin = settings::viz::pin_style + 1;
        RowCombo("viz_pin", "Style", "Pin one curve to study it, or Auto to walk the family",
            &pin, k_pin, 12);
        settings::viz::pin_style = pin - 1;
    }

    RowSliderF("viz_dx", "Demo X", &settings::viz::demo_x, 0.f, 100.f, "%.0f%%");
    RowSliderF("viz_dy", "Demo Y", &settings::viz::demo_y, 0.f, 100.f, "%.0f%%");
    NoteRow("The rig demonstrates the curve; the names are the aim's own.");
}

static void DrawHomePage()
{
    PageHeader(kTabNames[kTabHome], kTabSubtitles[kTabHome], nullptr);
    home_page::draw();
}

static void DrawAimPage()
{
    SubTabStrip(kAimSubs, 4, &g_SubTab[kTabAim]);
    switch (g_SubTab[kTabAim])
    {
    case 1:  DrawAimSilent();   break;
    case 2:  DrawAimTrigger();  break;
    case 3:  DrawAimAnalysis(); break;
    default: DrawAimGlobal();   break;
    }
}

static const char* kVisSubs[] = { "Body", "Info", "Overlay", "Map" };

static const ImU32 kPreviewJointColour = IM_COL32(255, 255, 255, 165);

static ImVec2 g_PreviewStageMin{ 0.f, 0.f };
static ImVec2 g_PreviewStageMax{ 0.f, 0.f };

static ImVec2 g_PreviewFigureMin{ 0.f, 0.f };
static ImVec2 g_PreviewFigureMax{ 0.f, 0.f };
static ImVec2 g_PreviewBoxMin{ 0.f, 0.f };
static ImVec2 g_PreviewBoxMax{ 0.f, 0.f };

struct PreviewProject
{
    bool   ok = false;
    ImVec2 p[sync::kPreviewJointCount];
    ImVec2 b0{}, b1{};
};

static PreviewProject ProjectRig(const sync::PreviewRig& rig, ImVec2 b0, ImVec2 b1)
{
    PreviewProject out;
    float mnx = FLT_MAX, mxx = -FLT_MAX, mny = FLT_MAX, mxy = -FLT_MAX;
    for (int i = 0; i < sync::kPreviewJointCount; ++i)
    {
        const float x = rig.joints[i][0], y = rig.joints[i][1];
        if (!std::isfinite(x) || !std::isfinite(y)) return out;
        mnx = Minf(mnx, x); mxx = Maxf(mxx, x);
        mny = Minf(mny, y); mxy = Maxf(mxy, y);
    }

    const float sh = Maxf(mxy - mny, 0.05f);
    const float k  = (b1.y - b0.y) / sh;
    const float cx = (b0.x + b1.x) * 0.5f;
    const float mwx = (mnx + mxx) * 0.5f;

    float fx0 = FLT_MAX, fx1 = -FLT_MAX;
    for (int i = 0; i < sync::kPreviewJointCount; ++i)
    {
        out.p[i].x = cx + (rig.joints[i][0] - mwx) * k;
        out.p[i].y = b1.y - (rig.joints[i][1] - mny) * k;
        fx0 = Minf(fx0, out.p[i].x);
        fx1 = Maxf(fx1, out.p[i].x);
    }

    out.b0 = ImVec2(fx0, b0.y);
    out.b1 = ImVec2(fx1, b1.y);
    out.ok = true;
    return out;
}

static const int kPreviewBones[][2] =
{
    { (int)sync::PreviewJoint::Head,         (int)sync::PreviewJoint::Neck },
    { (int)sync::PreviewJoint::Neck,         (int)sync::PreviewJoint::Chest },
    { (int)sync::PreviewJoint::Chest,        (int)sync::PreviewJoint::Pelvis },
    { (int)sync::PreviewJoint::Pelvis,       (int)sync::PreviewJoint::LegsBase },
    { (int)sync::PreviewJoint::Chest,        (int)sync::PreviewJoint::LeftShoulder },
    { (int)sync::PreviewJoint::LeftShoulder, (int)sync::PreviewJoint::LeftHand },
    { (int)sync::PreviewJoint::Chest,        (int)sync::PreviewJoint::RightShoulder },
    { (int)sync::PreviewJoint::RightShoulder,(int)sync::PreviewJoint::RightHand },
    { (int)sync::PreviewJoint::LegsBase,     (int)sync::PreviewJoint::LeftFoot },
    { (int)sync::PreviewJoint::LegsBase,     (int)sync::PreviewJoint::RightFoot },
};

static void DrawEspPreview(ImVec2 mn, ImVec2 mx, float alpha)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float w = mx.x - mn.x, h = mx.y - mn.y;

    dl->AddRectFilled(mn, mx, GC(IM_COL32(8, 8, 10, 45), alpha), 14.f);
    dl->AddRect(mn, mx, GC(IM_COL32(255, 255, 255, 26), alpha), 14.f, 0, 1.f);

    if (!settings::visuals::enable_enemies)
    {
        const ImVec2 ts = TSize(g_FontSmall, "ESP disabled");
        Txt(dl, g_FontSmall, ImVec2((mn.x + mx.x) * 0.5f - ts.x * 0.5f, (mn.y + mx.y) * 0.5f - ts.y * 0.5f),
            GC(g_T.textDim, alpha), "ESP disabled");
        return;
    }

    sync::PreviewRig rig;
    if (sync::hooks().preview_rig) sync::hooks().preview_rig(rig);
    const bool live = rig.valid;

    const long long shown_uid = live ? rig.user_id : 0;

    const int info_rows = (settings::visuals::movement_status ? 1 : 0)
                        + (settings::visuals::rigtype ? 1 : 0)
                        + (settings::visuals::tool ? 1 : 0);
    const float info_h = info_rows ? (13.f + 14.f * (float)info_rows) : 0.f;

    const ImVec2 s0(mn.x + w * 0.10f, mn.y + h * 0.10f);
    const ImVec2 s1(mx.x - w * 0.10f, mn.y + h * 0.90f - info_h);
    g_PreviewStageMin = s0;
    g_PreviewStageMax = s1;

    const float sw  = s1.x - s0.x, sh = s1.y - s0.y;
    const float scx = (s0.x + s1.x) * 0.5f;

    const float  side = Minf(sw - 12.f, sh * 0.90f);
    const ImVec2 f0(scx - side * 0.5f, (s0.y + s1.y) * 0.5f - side * 0.5f);
    const ImVec2 f1(f0.x + side, f0.y + side);
    g_PreviewFigureMin = f0;
    g_PreviewFigureMax = f1;

    const ImVec2 r0(f0.x, f0.y + side * 0.22f), r1(f1.x, f0.y + side * 0.96f);
    const PreviewProject proj = live ? ProjectRig(rig, r0, r1) : PreviewProject{};

    const ImVec2 head = proj.ok ? proj.p[(int)sync::PreviewJoint::Head]
                                : ImVec2(scx, f0.y + side * 0.28f);

    const float def_w = side * 0.42f;
    ImVec2 b0(scx - def_w * 0.5f, f0.y - 4.f), b1(scx + def_w * 0.5f, f1.y + 4.f);
    if (live && proj.ok && settings::visuals::box_mode == 1)
    {
        const float pad = Maxf(3.f, settings::visuals::box_thickness * 2.f);
        b0.x = Clampf(proj.b0.x - pad, f0.x - 5.f, f1.x + 5.f);
        b1.x = Clampf(proj.b1.x + pad, f0.x - 5.f, f1.x + 5.f);
        b0.y = Clampf(proj.b0.y - pad, f0.y - 5.f, f1.y + 5.f);
        b1.y = Clampf(proj.b1.y + pad, f0.y - 5.f, f1.y + 5.f);
    }
    g_PreviewBoxMin = b0;
    g_PreviewBoxMax = b1;

    const float bw = b1.x - b0.x, bh = b1.y - b0.y;
    const ImVec2 bc((b0.x + b1.x) * 0.5f, (b0.y + b1.y) * 0.5f);

    if (shown_uid > 0 && sync::hooks().preview_avatar)
    {
        dl->PushClipRect(mn, mx, true);

        const ImVec2 bust0(f0.x, f0.y), bust1(f1.x, f1.y);
        sync::hooks().preview_avatar(dl, shown_uid, bust0, bust1, GC(IM_COL32(255, 255, 255, 175), alpha));

        dl->AddRectFilledMultiColor(ImVec2(mn.x, mn.y + h * 0.42f), ImVec2(mx.x, mx.y),
            GC(IM_COL32(8, 8, 10, 0), alpha), GC(IM_COL32(8, 8, 10, 0), alpha),
            GC(IM_COL32(8, 8, 10, 190), alpha), GC(IM_COL32(8, 8, 10, 190), alpha));
        dl->PopClipRect();
    }

    const float horizon = b1.y;
    dl->AddLine(ImVec2(mn.x + 12.f, horizon), ImVec2(mx.x - 12.f, horizon), GC(IM_COL32(255, 255, 255, 13), alpha), 1.f);

    const float th = settings::visuals::box_thickness;
    const ImU32 boxCol = GC(settings::visuals::box ? Col32(settings::visuals::box_color)
                                                   : IM_COL32(255, 255, 255, 215), alpha);

    if (settings::visuals::box)
    {
        if (settings::visuals::box_type == 1)
        {
            const float c = Minf(bw, bh) * 0.24f;
            dl->AddLine(ImVec2(b0.x, b0.y), ImVec2(b0.x + c, b0.y), boxCol, th);
            dl->AddLine(ImVec2(b0.x, b0.y), ImVec2(b0.x, b0.y + c), boxCol, th);
            dl->AddLine(ImVec2(b1.x, b0.y), ImVec2(b1.x - c, b0.y), boxCol, th);
            dl->AddLine(ImVec2(b1.x, b0.y), ImVec2(b1.x, b0.y + c), boxCol, th);
            dl->AddLine(ImVec2(b0.x, b1.y), ImVec2(b0.x + c, b1.y), boxCol, th);
            dl->AddLine(ImVec2(b0.x, b1.y), ImVec2(b0.x, b1.y - c), boxCol, th);
            dl->AddLine(ImVec2(b1.x, b1.y), ImVec2(b1.x - c, b1.y), boxCol, th);
            dl->AddLine(ImVec2(b1.x, b1.y), ImVec2(b1.x, b1.y - c), boxCol, th);
        }
        else if (settings::visuals::box_type == 2)
        {
            const float o = Minf(bw, bh) * 0.12f;
            const ImVec2 c0(b0.x - o, b0.y - o * 0.4f), c1(b1.x - o, b1.y - o * 0.4f);
            dl->AddRect(b0, b1, boxCol, 1.f, 0, th);
            dl->AddRect(c0, c1, boxCol, 1.f, 0, th);
            dl->AddLine(ImVec2(b0.x, b0.y), ImVec2(c0.x, c0.y), boxCol, th);
            dl->AddLine(ImVec2(b1.x, b0.y), ImVec2(c1.x, c0.y), boxCol, th);
            dl->AddLine(ImVec2(b0.x, b1.y), ImVec2(c0.x, c1.y), boxCol, th);
            dl->AddLine(ImVec2(b1.x, b1.y), ImVec2(c1.x, c1.y), boxCol, th);
        }
        else
        {
            dl->AddRect(b0, b1, boxCol, 1.f, 0, th);
        }
    }

    if (settings::visuals::box_fill)
    {
        if (settings::visuals::box_gradient)
        {
            const ImU32 top = GC(Col32(settings::visuals::box_gradient_top), alpha);
            const ImU32 bot = GC(Col32(settings::visuals::box_gradient_bottom), alpha);
            dl->AddRectFilledMultiColor(b0, b1, top, top, bot, bot);
        }
        else
        {
            dl->AddRectFilled(b0, b1, GC(Col32(settings::visuals::box_fill_color), alpha), 2.f);
        }
    }

    if (settings::visuals::china_hat)
    {
        const ImU32 hat  = GC(Col32(settings::visuals::china_hat_color), alpha);
        const float hatR = side * 0.20f;
        const float hatY = head.y - side * 0.10f;
        const float hatH = Maxf(3.f, settings::visuals::china_hat_height * side * 0.14f);
        dl->AddTriangleFilled(ImVec2(head.x, hatY - hatH),
            ImVec2(head.x - hatR, hatY), ImVec2(head.x + hatR, hatY), hat);
        dl->AddLine(ImVec2(head.x - hatR, hatY), ImVec2(head.x + hatR, hatY), hat, 1.f);
    }

    if (settings::visuals::healthbar)
    {
        const float frac = live && rig.health > 0.f ? Clampf(rig.health / 100.f, 0.f, 1.f) : 0.74f;
        const ImU32 hp = settings::visuals::health_based_healthbar
            ? GC(IM_COL32(126, 214, 132, 255), alpha)
            : GC(Col32(settings::visuals::healthbar_color), alpha);
        const float barW = Maxf(2.f, settings::visuals::healthbar_size * 0.6f);
        dl->AddRectFilled(ImVec2(b0.x - 7.f, b0.y), ImVec2(b0.x - 7.f + barW, b1.y), GC(IM_COL32(255, 255, 255, 46), alpha), 2.f);
        dl->AddRectFilled(ImVec2(b0.x - 7.f, b1.y - bh * frac), ImVec2(b0.x - 7.f + barW, b1.y), hp, 2.f);
    }

    if (settings::visuals::skeleton)
    {
        const ImU32 sk = GC(Col32(settings::visuals::skeleton_color), alpha);
        if (proj.ok)
        {

            for (const auto& bone : kPreviewBones)
                dl->AddLine(proj.p[bone[0]], proj.p[bone[1]], sk, 1.3f);

            for (int i = 0; i < sync::kPreviewJointCount; ++i)
                dl->AddCircleFilled(proj.p[i], 1.6f, GC(kPreviewJointColour, alpha), 8);
        }
        else
        {
            const ImVec2 neck(bc.x, b0.y + bh * 0.12f), hip(bc.x, b0.y + bh * 0.54f);
            dl->AddLine(neck, hip, sk, 1.3f);
            dl->AddLine(neck, ImVec2(bc.x - bw * 0.44f, b0.y + bh * 0.34f), sk, 1.3f);
            dl->AddLine(neck, ImVec2(bc.x + bw * 0.44f, b0.y + bh * 0.34f), sk, 1.3f);
            dl->AddLine(hip, ImVec2(bc.x - bw * 0.26f, b1.y), sk, 1.3f);
            dl->AddLine(hip, ImVec2(bc.x + bw * 0.26f, b1.y), sk, 1.3f);
        }
    }

    if (settings::visuals::chams)
        dl->AddCircleFilled(ImVec2(bc.x, bc.y), bw * 0.20f, GC(Col32(settings::visuals::chams_fill_color), alpha * 0.35f), 24);

    if (settings::visuals::tracer)
        dl->AddLine(ImVec2(mn.x + w * 0.5f, mx.y - 5.f), ImVec2(bc.x, b1.y), GC(Col32(settings::visuals::tracer_color), alpha), 1.f);

    if (settings::visuals::name)
    {
        const char* nm = live && rig.name[0] ? rig.name : "player";
        const ImU32 ncol = settings::visuals::name ? GC(Col32(settings::visuals::name_color), alpha) : GC(g_T.textPrimary, alpha);
        const ImVec2 ns = TSize(g_FontSmall, nm);
        Txt(dl, g_FontSmall, ImVec2(bc.x - ns.x * 0.5f, b0.y - 16.f), ncol, nm);
    }

    if (settings::visuals::distance)
    {
        char dist[24];
        snprintf(dist, sizeof dist, "%.0f %s", live ? rig.distance : 42.f,
                 settings::visuals::distance_measurement == 1 ? "m" : "studs");
        const ImVec2 ds = TSize(g_FontSmall, dist);

        const ImVec2 t0(Maxf(mn.x + 6.f, b0.x - 12.f - ds.x - 8.f), bc.y - 10.f);
        const ImVec2 t1(t0.x + ds.x + 16.f, t0.y + 20.f);
        dl->AddRectFilled(t0, t1, GC(IM_COL32(0, 0, 0, 120), alpha), 6.f);
        Txt(dl, g_FontSmall, ImVec2(t0.x + 8.f, t0.y + 4.f), GC(g_T.textPrimary, alpha), dist);
    }

    if (settings::visuals::avatar)
    {

        const ImVec2 a1(mx.x - 12.f, mx.y - 12.f), a0(a1.x - 34.f, a1.y - 34.f);
        dl->AddRectFilled(a0, a1, GC(IM_COL32(255, 255, 255, 14), alpha), 8.f);
        bool drew = false;
        if (shown_uid > 0 && sync::hooks().player_avatar_rounded)
            drew = sync::hooks().player_avatar_rounded(dl, shown_uid, a0, a1,
                GC(IM_COL32(255, 255, 255, 255), alpha), 8.f);
        if (!drew)
            PlayerAvatar(dl, ImVec2((a0.x + a1.x) * 0.5f, (a0.y + a1.y) * 0.5f), 15.f, IM_COL32(150, 150, 158, 255));
        dl->AddRect(a0, a1, GC(IM_COL32(255, 255, 255, 20), alpha), 8.f, 0, 1.f);
    }

    if (live && rig.health > 0.f && settings::visuals::health_text)
    {
        char hp[24];
        snprintf(hp, sizeof hp, "%.0f", rig.health);
        TxtRight(dl, g_FontSmall, b1.x, b0.y - 15.f, GC(Col32(settings::visuals::health_text_color), alpha), hp);
    }

    if (live || settings::visuals::movement_status || settings::visuals::rigtype || settings::visuals::tool)
    {
        const ImU32 col = GC(g_T.textDim, alpha);
        float iy = b1.y + 13.f;
        char line[96];

        auto info_line = [&](const char* text)
        {
            Txt(dl, g_FontSmall, ImVec2(mn.x + 12.f, iy), col, text);
            iy += 14.f;
        };

        if (settings::visuals::movement_status)
        {

            const char* mtext = "idle";
            switch (rig.state)
            {
            case 0: case 1:  mtext = "ragdolled";         break;
            case 3: case 5: case 6: mtext = "in air";     break;
            case 4:  mtext = "swimming";                  break;
            case 12: mtext = "climbing";                  break;
            case 13: mtext = "sitting";                   break;
            case 14: mtext = "platform standing";         break;
            default: break;
            }
            snprintf(line, sizeof line, "State  %s", mtext);
            info_line(line);
        }

        if (settings::visuals::rigtype)
        {
            snprintf(line, sizeof line, "Rig    %s", rig.rig_type == 1 ? "R15" : "R6");
            info_line(line);
        }

        if (settings::visuals::tool)
        {
            snprintf(line, sizeof line, "Tool   %s", rig.tool[0] ? rig.tool : "None");
            info_line(line);
        }
    }
}

static const char* FileNameOf(const char* path)
{
    const char* cut = path;
    for (const char* p = path; *p; ++p)
        if (*p == '\\' || *p == '/')
            cut = p + 1;
    return cut;
}

static bool ReloadFillImage()
{
    if (!sync::hooks().fill_image)
        return false;

    return sync::hooks().fill_image(settings::visuals::box_fill_image_path,
                                    settings::visuals::box_fill_remove_bg);
}

static bool ChooseImageFile()
{
    char path[MAX_PATH]{};
    snprintf(path, sizeof path, "%s", settings::visuals::box_fill_image_path);

    HWND overlay = g_hwnd;
    const bool hide = overlay != nullptr && ::IsWindow(overlay) != FALSE &&
                      ::IsWindowVisible(overlay) != FALSE;
    if (hide)
        ::ShowWindow(overlay, SW_HIDE);

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner   = overlay;
    ofn.lpstrFilter = "Images\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif\0All files\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = (DWORD)sizeof path;
    ofn.lpstrTitle  = "Pick a box fill image";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    const BOOL picked = ::GetOpenFileNameA(&ofn);

    if (hide)
        ::ShowWindow(overlay, SW_SHOW);

    if (!picked)
        return false;

    snprintf(settings::visuals::box_fill_image_path,
             sizeof settings::visuals::box_fill_image_path, "%s", path);
    return true;
}

static bool BoxFillImageRow()
{
    ImGui::PushID("v_fillimg");
    const float h = 32.f;
    RowHover("##row", ImVec2(g_ContentL - 7.f, g_RowY - 2.f), ImVec2(g_ContentR + 7.f, g_RowY + h + 2.f));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    TxtBold(dl, "Fill Image", ImVec2(g_ContentL, g_RowY + (h - RowNameH()) * 0.5f), GC(g_T.textPrimary));

    const bool have = settings::visuals::box_fill_image_path[0] != '\0';
    const bool loaded = settings::visuals::box_fill_image_loaded;
    char status[72];
    if (!have)         snprintf(status, sizeof status, "no image picked");
    else if (!loaded)  snprintf(status, sizeof status, "%s (failed to load)", FileNameOf(settings::visuals::box_fill_image_path));
    else               snprintf(status, sizeof status, "%s", FileNameOf(settings::visuals::box_fill_image_path));

    const ImVec2 ss = TSize(g_FontSmall, status);
    const float btnW = 120.f;
    const float sx = g_ContentR - btnW - 12.f - ss.x;
    if (sx > g_ContentL + 90.f)
        Txt(dl, g_FontSmall, ImVec2(sx, g_RowY + (h - g_FontSmall->FontSize) * 0.5f),
            GC(loaded ? g_T.textDim : g_T.textFaint), status);

    bool changed = false;
    if (ButtonControl("pick", ImVec2(g_ContentR - btnW, g_RowY + (h - 28.f) * 0.5f),
                      ImVec2(btnW, 28.f), "Choose Image"))
        changed = ChooseImageFile();

    g_RowY += h + K::RowGap;
    ImGui::PopID();
    return changed;
}

static void DrawVisualsBody()
{
    static const char* const k_box_type[] = { "Full", "Cornered" };
    static const char* const k_box_mode[] = { "Static", "Dynamic" };
    static const char* const k_fill_mode[] = { "Colour", "Image" };

    PageHeader("Visuals", "Player and world ESP.", nullptr);

    SectionHead("Body ESP");
    RowToggle("v_enemies", "Enable Enemies", &settings::visuals::enable_enemies);
    RowToggle("v_box", "Box", &settings::visuals::box);
    if (settings::visuals::box)
    {
        if (settings::visuals::box_type < 0 || settings::visuals::box_type > 1)
            settings::visuals::box_type = 0;
        RowCombo("v_boxtype", "Box Type", nullptr, &settings::visuals::box_type, k_box_type, 2);
        RowCombo("v_boxmode", "Box Mode", "Static holds its shape, Dynamic tracks the pose", &settings::visuals::box_mode, k_box_mode, 2);
        ColorRow("v_boxcol", "Box Colour", nullptr, settings::visuals::box_color, true);
        RowSliderF("v_boxth", "Box Thickness", &settings::visuals::box_thickness, 0.5f, 6.f, "%.1f px");
        RowToggle("v_boxfill", "Box Fill", &settings::visuals::box_fill);
        if (settings::visuals::box_fill)
        {
            RowCombo("v_fillmode", "Fill Type", nullptr, &settings::visuals::box_fill_mode, k_fill_mode, 2);
            if (settings::visuals::box_fill_mode == 1)
            {
                const bool bg_hit   = RowToggle("v_fillbg", "Remove Background", &settings::visuals::box_fill_remove_bg);
                const bool new_file = BoxFillImageRow();
                if (bg_hit || new_file)
                    settings::visuals::box_fill_image_loaded = ReloadFillImage();
                RowSliderF("v_fillalpha", "Fill Opacity", &settings::visuals::box_fill_image_alpha, 0.f, 1.f, "%.2f");
                if (!settings::visuals::box_fill_image_loaded)
                    NoteRow("Pick a PNG or JPG - it is loaded into the overlay, nothing is installed");
            }
            else
            {
                ColorRow("v_fillcol", "Fill Colour", nullptr, settings::visuals::box_fill_color, true);
            }
        }
        RowToggle("v_boxgrad", "Box Gradient", &settings::visuals::box_gradient);
        if (settings::visuals::box_gradient)
        {
            ColorRow("v_gradtop", "Gradient Top", nullptr, settings::visuals::box_gradient_top, true);
            ColorRow("v_gradbot", "Gradient Bottom", nullptr, settings::visuals::box_gradient_bottom, true);
        }
        RowToggle("v_bodyonly", "Body Only (skip hulls)", &settings::visuals::box_body_only);
    }

    RowToggle("v_skeleton", "Skeleton", &settings::visuals::skeleton);
    if (settings::visuals::skeleton)
    {
        ColorRow("v_skcol", "Skeleton Colour", nullptr, settings::visuals::skeleton_color, true);
        RowSliderF("v_skth", "Thickness", &settings::visuals::skeleton_thickness, 0.5f, 6.f, "%.1f px");
        RowToggle("v_skanim", "Animate Limbs", &settings::visuals::skeleton_animate);
        if (settings::visuals::skeleton_animate)
            RowSliderF("v_skswing", "Swing", &settings::visuals::skeleton_swing, 0.f, 2.f, "%.2f");
        RowToggle("v_skout", "Outline", &settings::visuals::skeleton_outline);
        if (settings::visuals::skeleton_outline)
            ColorRow("v_skoutcol", "Outline Colour", nullptr, settings::visuals::skeleton_outline_color, true);
    }
    RowToggle("v_chinahat", "China Hat", &settings::visuals::china_hat);
    if (settings::visuals::china_hat)
    {
        ColorRow("v_hatcol", "Hat Colour", nullptr, settings::visuals::china_hat_color, true);
        RowSliderF("v_hatheight", "Hat Height", &settings::visuals::china_hat_height, 0.3f, 3.f, "%.2f");
        RowSliderF("v_hatradius", "Hat Radius", &settings::visuals::china_hat_radius, 0.4f, 3.5f, "%.2f");
    }

    RowToggle("v_headdot", "Head Dot", &settings::visuals::head_dot,
              "A dot where the head projects - draws even when the box cannot");
    if (settings::visuals::head_dot)
    {
        ColorRow("v_headdotcol", "Dot Colour", nullptr, settings::visuals::head_dot_color, true);
        RowSliderF("v_headdotr", "Dot Size", &settings::visuals::head_dot_size, 1.f, 14.f, "%.1f px");
        RowToggle("v_headdotout", "Dot Outline", &settings::visuals::head_dot_outline);
        if (settings::visuals::head_dot_outline)
            ColorRow("v_headdotoutcol", "Outline Colour", nullptr, settings::visuals::head_dot_outline_color, true);
    }
    RowToggle("v_lookdir", "Look Direction", &settings::visuals::look_direction,
              "A line out of the head along the rig's own forward vector");
    if (settings::visuals::look_direction)
    {
        ColorRow("v_lookdircol", "Line Colour", nullptr, settings::visuals::look_direction_color, true);
        RowSliderF("v_lookdirlen", "Line Length", &settings::visuals::look_direction_length, 0.2f, 8.f, "%.2f studs");
        RowSliderF("v_lookdirth", "Line Thickness", &settings::visuals::look_direction_thickness, 0.5f, 6.f, "%.1f px");
    }

    SectionHead("Filtering");
    RowToggle("v_team", "Team Check", &settings::visuals::team_check);
    if (!settings::visuals::body_corpse)
        RowToggle("v_dead", "Dead Check", &settings::visuals::dead_check);
    else
        settings::visuals::dead_check = false;
    if (!settings::visuals::dead_check)
    {
        RowToggle("v_corpse", "Body Corpse", &settings::visuals::body_corpse);
        if (settings::visuals::body_corpse)
            ColorRow("v_corpsecol", "Corpse Colour", nullptr, settings::visuals::corpse_color, true);
    }
    else
    {
        settings::visuals::body_corpse = false;
    }
    RowToggle("v_vis", "Visible Check", &settings::visuals::visible_check);
    RowToggle("v_knock", "Knock Check", &settings::visuals::knock_check);
    RowToggle("v_local", "Local Player", &settings::visuals::localplayer);
    RowToggle("v_distcheck", "Distance Check", &settings::visuals::distance_check);
    if (settings::visuals::distance_check)
    {
        static const char* const k_units[] = { "Studs", "Meters" };
        RowCombo("v_distunit", "Distance Unit", nullptr, &settings::visuals::distance_unit, k_units, 2);
        const bool meters = settings::visuals::distance_unit == 1;
        RowSliderF("v_maxdist", meters ? "Max Distance (m)" : "Max Distance (studs)",
                   &settings::visuals::max_distance, 0.f, meters ? 1400.f : 5000.f, "%.0f");
    }
    RowSliderF("v_fadein", "Fade In", &settings::visuals::fade_in_speed, 0.f, 20.f, "%.1f");
    RowSliderF("v_fadeout", "Fade Out", &settings::visuals::fade_out_speed, 0.f, 20.f, "%.1f");
}

static void DrawVisualsInfo()
{
    static const char* const k_name_style[] = { "Display Name", "Username" };
    static const char* const k_units[] = { "Studs", "Meters" };

    static const char* const k_esp_fonts[] = { "Body", "Caption", "Medium", "Heading", "ImGui", "Display", "Clear", "Inter" };

    PageHeader("Visuals", "Player and world ESP.", nullptr);

    SectionHead("Player Info");
    RowToggle("vi_name", "Draw Names", &settings::visuals::name);
    if (settings::visuals::name)
    {
        RowCombo("vi_namesrc", "Name Source", nullptr, &settings::visuals::name_display_type, k_name_style, 2);
        ColorRow("vi_namecol", "Name Colour", nullptr, settings::visuals::name_color, true);
        RowToggle("vi_blend", "Blend Text", &settings::visuals::blend);
        if (settings::visuals::blend)
        {
            ColorRow("vi_blendstart", "Blend Start", nullptr, settings::visuals::name_color_blend_start, true);
            ColorRow("vi_blendend", "Blend End", nullptr, settings::visuals::name_color_blend_end, true);
        }
    }
    RowToggle("vi_dist", "Draw Distance", &settings::visuals::distance);
    if (settings::visuals::distance)
    {
        RowCombo("vi_units", "Units", nullptr, &settings::visuals::distance_measurement, k_units, 2);
        ColorRow("vi_distcol", "Distance Colour", nullptr, settings::visuals::distance_color, true);
    }
    RowToggle("vi_avatar", "Draw Avatar", &settings::visuals::avatar);
    RowToggle("vi_rig", "Rig State", &settings::visuals::rigtype,
              "R6 / R15 / custom, read off the rig and drawn under the box");
    RowToggle("vi_state", "State", &settings::visuals::state,
              "The Humanoid's own state word, drawn under the box (uses Flag Colour)");
    RowToggle("vi_tool", "Draw Tool", &settings::visuals::tool);
    if (settings::visuals::tool)
        ColorRow("vi_toolcol", "Tool Colour", nullptr, settings::visuals::tool_color, true);
    RowToggle("vi_flags", "Draw Flags", &settings::visuals::flags);
    if (settings::visuals::flags)
        ColorRow("vi_flagcol", "Flag Colour", nullptr, settings::visuals::flags_state_colour, true);
    if (settings::visuals::esp_font < 0 || settings::visuals::esp_font > 7)
        settings::visuals::esp_font = 7;
    RowCombo("vi_font", "ESP Font", nullptr, &settings::visuals::esp_font, k_esp_fonts, 8);
    RowSliderF("vi_fontsize", "ESP Font Size", &settings::visuals::esp_font_size, 8.f, 24.f, "%.0f");

    SectionHead("Combat Tags");
    RowToggle("vi_movestatus", "Movement Status", &settings::visuals::movement_status);
    RowToggle("vi_hpbar", "Health Bar", &settings::visuals::healthbar);
    if (settings::visuals::healthbar)
    {
        ColorRow("vi_hpcol", "Health Colour", nullptr, settings::visuals::healthbar_color, true);
        RowToggle("vi_hpbased", "Health Based", &settings::visuals::health_based_healthbar);
        RowToggle("vi_hppct", "Health Percent", &settings::visuals::health_percent);
        if (settings::visuals::health_percent)
            ColorRow("vi_hppctcol", "Percent Colour", nullptr, settings::visuals::health_percent_color, true);

        RowToggle("vi_hpenh", "Enhanced Bar", &settings::visuals::healthbar_enhanced, "Rounded, with a shadow and a bright head");
        if (settings::visuals::healthbar_enhanced)
        {
            RowSliderF("vi_hpwidth", "Bar Width", &settings::visuals::healthbar_width, 2.f, 24.f, "%.0f px");
            RowSliderF("vi_hpopacity", "Bar Opacity", &settings::visuals::healthbar_opacity, 0.f, 100.f, "%.0f %%");
            RowToggle("vi_hpglow", "Bar Glow", &settings::visuals::healthbar_glow);
        }
    }
    RowToggle("vi_hptext", "Health Text", &settings::visuals::health_text);
    if (settings::visuals::health_text)
        ColorRow("vi_hptextcol", "Health Text Colour", nullptr, settings::visuals::health_text_color, true);
    ColorRow("vi_targetcol", "Aim Target Colour", "Marks whatever the aim is holding", settings::visuals::aim_target_color, true);
    ColorRow("vi_friendcol", "Friend Colour", "Marks players flagged as friends in Players", settings::visuals::friend_color, true);

    SectionHead("On-Screen");
    RowToggle("vi_kbdock", "Keybind Dock", &sync::state().kb_show,
              "A plate listing the features that have a key bound");
    if (sync::state().kb_show)
        NoteRow("Position and the activation column: Config > Menu, under HUD");
    RowToggle("vi_targethud", "Target HUD", &sync::state().th_show,
              "Name, health and tool of whoever the aim is holding");
    if (sync::state().th_show)
        NoteRow("Fields, position and scale: Config > Menu, under HUD");

    SectionHead("Aim Line");
    RowToggle("vi_aimline", "Aim Line", &settings::visuals::aim_line,
              "Draws to whoever the aimbot or the silent aim is holding");
    if (settings::visuals::aim_line)
    {
        static const char* const k_aimline[] = { "Solid", "Dashed", "Dotted", "Tapered", "Double" };
        RowCombo("vi_aimline_style", "Style", nullptr, &settings::visuals::aim_line_style, k_aimline, 5);
        ColorRow("vi_aimline_col", "Colour", nullptr, settings::visuals::aim_line_color, true);
        RowSliderF("vi_aimline_th", "Thickness", &settings::visuals::aim_line_thickness, 0.5f, 6.f, "%.1f px");
        RowToggle("vi_aimline_from", "From Centre", &settings::visuals::aim_line_centre);
        if (!settings::visuals::aim_line_centre)
            NoteRow("The line starts at the cursor instead");
    }
}

static void DrawVisualsOverlay()
{
    static const char* const k_arrow_info[] = { "Name", "Distance", "Health", "Tool" };

    PageHeader("Visuals", "Player and world ESP.", nullptr);

    SectionHead("Tracer");
    RowToggle("vo_tracer", "Draw Tracer", &settings::visuals::tracer);
    if (settings::visuals::tracer)
    {
        RowCombo("vo_trorigin", "Tracer Origin", nullptr, &settings::visuals::tracer_origin, TracerOrigins(), 4);
        ColorRow("vo_trcol", "Line Colour", nullptr, settings::visuals::tracer_color, true);
    }

    SectionHead("Screen Overlay");
    RowToggle("vo_arrows", "Offscreen Arrows", &settings::visuals::offscreen_arrows);
    if (settings::visuals::offscreen_arrows)
    {
        ColorRow("vo_arrowcol", "Arrow Colour", nullptr, settings::visuals::arrow_color, true);
        RowSliderF("vo_arrowsize", "Arrow Size", &settings::visuals::arrow_size, 6.f, 32.f, "%.0f px");
        RowSliderF("vo_arrowradius", "Arrow Radius", &settings::visuals::arrow_radius, 40.f, 500.f, "%.0f px");
        unsigned info = 0;
        for (int i = 0; i < 4; i++) if (settings::visuals::arrow_info[i]) info |= (1u << i);
        RowMulti("vo_arrowinfo", "Arrow Info", "Printed beside the arrow", &info, k_arrow_info, 4);
        for (int i = 0; i < 4; i++) settings::visuals::arrow_info[i] = (info & (1u << i)) != 0;
    }
    RowToggle("vo_viewhitbox", "View Hitbox", &settings::visuals::view_hitbox);
    if (settings::visuals::view_hitbox)
        ColorRow("vo_hitboxcol", "Hitbox Colour", nullptr, settings::visuals::view_hitbox_color, true);
    RowToggle("vo_hitskel", "Hit Skeleton", &settings::visuals::hit_skeleton);
    if (settings::visuals::hit_skeleton)
    {
        ColorRow("vo_hitskelcol", "Skeleton Colour", nullptr, settings::visuals::hit_skeleton_color, true);
        RowSliderF("vo_hitskeldur", "Duration", &settings::visuals::hit_skeleton_duration, 0.2f, 6.f, "%.1f s");
    }
    RowToggle("vo_hitchams", "Hit Chams", &settings::visuals::hit_chams);
    if (settings::visuals::hit_chams)
    {
        ColorRow("vo_hitchamscol", "Flash Colour", nullptr, settings::visuals::hit_chams_color, true);
        RowSliderF("vo_hitchamsdur", "Duration", &settings::visuals::hit_chams_duration, 0.2f, 6.f, "%.1f s");
    }
    RowToggle("vo_hitfreeze", "Freeze On Hit", &settings::visuals::hit_freeze);
}

static void DrawVisualsBodyExtras()
{
    SectionHead("Engine Chams");
    RowToggle("v_echams", "Engine Chams", &settings::visuals::engine_chams,
              "Restyles the engine's render clusters - scene wide, through walls");
    if (settings::visuals::engine_chams)
    {
        static const char* const k_styles[] = { "Through Walls", "Ghost", "Wireframe", "Mesh",
                                                "Charwire", "Glass", "Glaze", "Smoke",
                                                "Depth", "Hologram" };
        if (settings::visuals::engine_chams_style < 0 || settings::visuals::engine_chams_style > 9)
            settings::visuals::engine_chams_style = 0;
        RowCombo("v_echamstyle", "Style", nullptr, &settings::visuals::engine_chams_style, k_styles, 10);
        ColorRow("v_echamcol", "Colour", nullptr, settings::visuals::engine_chams_color, true);
    }

    SectionHead("Normal Chams");
    RowToggle("v_nchams", "Normal Chams", &settings::visuals::nchams);
    if (settings::visuals::nchams)
    {
        static const char* const k_nmodes[] = { "Box", "Box Filled", "Clipper", "Shader" };
        if (settings::visuals::nchams_mode < 0 || settings::visuals::nchams_mode > 3)
            settings::visuals::nchams_mode = 0;
        RowCombo("v_nchamode", "Mode", "Edges, hulls, one union silhouette, or a shader",
                 &settings::visuals::nchams_mode, k_nmodes, 4);

        ColorRow("v_nchamout", "Outline Colour", nullptr, settings::visuals::nchams_outline_color, true);
        if (settings::visuals::nchams_mode >= 1)
            ColorRow("v_nchamfill", "Fill Colour", nullptr, settings::visuals::nchams_fill_color, true);

        RowToggle("v_nchamglow", "White Glow", &settings::visuals::nchams_glow, "A soft white halo under the outline");
        if (settings::visuals::nchams_glow)
        {
            RowSliderF("v_nchamglowo", "Glow Opacity", &settings::visuals::nchams_glow_opacity, 0.f, 100.f, "%.0f %%");
            RowSliderF("v_nchamglows", "Glow Size", &settings::visuals::nchams_glow_size, 1.f, 12.f, "%.0f px");
        }
        RowToggle("v_nchamcore", "Inner Fill", &settings::visuals::nchams_inner_fill, "A solid core inside the silhouette");
        if (settings::visuals::nchams_inner_fill)
            RowSliderF("v_nchamcores", "Inner Scale", &settings::visuals::nchams_inner_fill_scale, 5.f, 100.f, "%.0f %%");

        if (settings::visuals::nchams_mode == 3)
        {

            static const char* const k_shader_styles[] = {
                "plasma", "ember", "toxic", "ice", "void",
                "neon", "gold", "matrix", "aurora", "blood",
                "ocean", "hologram", "sunset", "ghost", "chromatic"
            };
            if (settings::visuals::nchams_shader_style < 0 || settings::visuals::nchams_shader_style > 14)
                settings::visuals::nchams_shader_style = 0;
            RowCombo("v_nchamstyle", "Shader Style", "Animated fill painted into the silhouette",
                     &settings::visuals::nchams_shader_style, k_shader_styles, 15);
        }
    }

    SectionHead("Mesh Chams");
    RowToggle("v_chams", "Chams", &settings::visuals::chams);
    if (settings::visuals::chams)
    {
        ColorRow("v_chamscol", "Fill Colour", nullptr, settings::visuals::chams_fill_color, true);

        static const char* const k_shader_names[] = {
            "flat", "chrome", "rainbow", "pearl", "glossy", "holographic",
            "fade", "wireframe", "glass", "ropes", "liquid metal",
            "plasma", "gold", "ice", "toxic", "galaxy", "matrix"
        };

        const bool pass_up = !sync::hooks().chams_pass || sync::hooks().chams_pass();
        if (pass_up)
        {
            if (settings::visuals::mesh_shader_mode < 0 ||
                settings::visuals::mesh_shader_mode >= IM_ARRAYSIZE(k_shader_names))
                settings::visuals::mesh_shader_mode = 0;
            RowCombo("v_shader", "Shaders", "Style the mesh pass draws with",
                     &settings::visuals::mesh_shader_mode, k_shader_names, IM_ARRAYSIZE(k_shader_names));
        }
        else
        {
            NoteRow("shader pass unavailable - the mesh is drawn flat");
        }
        ColorRow("v_occluded", "Occluded Colour", "Geometry behind cover", settings::visuals::mesh_occluded_color, true);
    }
}

static void DrawVisualsMap()
{
    PageHeader("Visuals", "Player and world ESP.", nullptr);

    SectionHead("Radar");
    RowToggle("vm_radar", "Enable Radar", &settings::visuals::radar_enabled);
    if (settings::visuals::radar_enabled)
    {
        static const char* const k_radar_corner[] = { "Top Right", "Top Left", "Bottom Right", "Bottom Left" };
        static const char* const k_radar_shape[]  = { "Square", "Round" };
        if (settings::visuals::radar_position < 0 || settings::visuals::radar_position > 3)
            settings::visuals::radar_position = 0;
        RowCombo("vm_radarpos", "Position", nullptr, &settings::visuals::radar_position, k_radar_corner, 4);
        RowCombo("vm_radarshape", "Shape", nullptr, &settings::visuals::radar_shape, k_radar_shape, 2);
        RowSliderF("vm_radarsize", "Radar Size", &settings::visuals::radar_size, 60.f, 320.f, "%.0f px");
        RowSliderI("vm_radarrange", "Range", &settings::visuals::radar_range, 40, 800, "%d studs");
        RowSliderF("vm_radarop", "Background", &settings::visuals::radar_opacity, 0.25f, 1.f, "%.2f");
        RowToggle("vm_radarrotate", "Rotate With Camera", &settings::visuals::radar_rotate,
                  "Off keeps the map north-up");
        RowToggle("vm_radargrid", "Range Grid", &settings::visuals::radar_grid);
        if (settings::visuals::radar_grid)
            RowToggle("vm_radarlabels", "Range Labels", &settings::visuals::radar_labels);
        RowToggle("vm_radarsweep", "Sweep Line", &settings::visuals::radar_sweep);
        RowToggle("vm_radarnames", "Names", &settings::visuals::radar_names);
        RowToggle("vm_radarhealth", "Health Ring", &settings::visuals::radar_health);
        RowToggle("vm_radardead", "Show Dead", &settings::visuals::radar_dead);
        RowToggle("vm_radarwalls", "Show Walls", &settings::visuals::radar_show_walls);
        if (settings::visuals::radar_show_walls)
            ColorRow("vm_radarwallcol", "Wall Colour", nullptr, settings::visuals::radar_wall_color, true);
        ColorRow("vm_radarlocalcol", "You", nullptr, settings::visuals::radar_local_color, true);
        ColorRow("vm_radarallycol", "Teammate", nullptr, settings::visuals::radar_team_color, true);
        ColorRow("vm_radarenemycol", "Enemy", nullptr, settings::visuals::radar_enemy_color, true);
    }

    SectionHead("Terrain");
    RowToggle("vm_terrain", "Custom Terrain", &settings::world::terrain::enabled,
              "Override the map's grass and water material slots");
    if (settings::world::terrain::enabled)
    {
        RowSliderF("vm_grasslen", "Grass Length", &settings::world::terrain::grass_length, 0.f, 1.f, "%.2f");
        ColorRow3("vm_grasscol", "Grass Colour", nullptr, settings::world::terrain::grass_color);
        ColorRow3("vm_watercol", "Water Colour", nullptr, settings::world::terrain::water_color);
        RowSliderF("vm_waterrefl", "Water Reflectance", &settings::world::terrain::water_reflectance, 0.f, 1.f, "%.2f");
        RowSliderF("vm_watertrans", "Water Transparency", &settings::world::terrain::water_transparency, 0.f, 1.f, "%.2f");
        NoteRow("Also on World > Terrain - one value, two views");
    }
}

static void DrawVisualsPage()
{

    const float savedR = g_ContentR;
    g_ContentR = savedR - K::PreviewW - 26.f;

    SubTabStrip(kVisSubs, 4, &g_SubTab[kTabVisuals]);

    switch (g_SubTab[kTabVisuals])
    {
    case 1:  DrawVisualsInfo();         break;
    case 2:  DrawVisualsOverlay();      break;
    case 3:  DrawVisualsMap();          break;
    default: DrawVisualsBody();
             DrawVisualsBodyExtras();   break;
    }

    g_ContentR = savedR;

    const float s = SpringToId(kPreviewSpringId, 1.f, kBubbleResponse, kBubbleDamping, kPreviewClosedScale);
    const float a = BubbleAlpha((s - kPreviewClosedScale) / (1.f - kPreviewClosedScale));
    g_PreviewScaleDbg = s;

    const char* caption = "ESP Preview";
    const ImVec2 cs = TSize(g_FontSmall, caption);
    const float cx = g_ContentR - K::PreviewW * 0.5f;
    Txt(ImGui::GetWindowDrawList(), g_FontSmall,
        ImVec2(cx - cs.x * 0.5f, g_ContentTop - 2.f), GC(g_T.textDim, a), caption);

    const float cy = g_ContentTop + 22.f + K::PreviewH * 0.5f;
    const ImVec2 half(K::PreviewW * 0.5f * s, K::PreviewH * 0.5f * s);
    DrawEspPreview(ImVec2(cx - half.x, cy - half.y), ImVec2(cx + half.x, cy + half.y), a);
}

static const char* kMiscSubs[] = { "Movement", "Physics", "Effects", "Utility" };

static void DrawMiscMovement()
{
    static const char* const k_speed_mode[] = { "Velocity", "Humanoid" };
    static const char* const k_fly_mode[] = { "Velocity", "Position", "Position Lock" };

    PageHeader("Misc", "Movement & utility tweaks.", nullptr);

    SectionHead("Walk Speed");
    RowToggleBind("m_ws", "Walk Speed", &settings::walkspeed::enabled, &settings::walkspeed::keybind,
                  &settings::walkspeed::keybind_mode);
    if (settings::walkspeed::enabled)
    {
        RowCombo("m_wsact", "Activation", nullptr, &settings::walkspeed::keybind_mode, ActivationOptions(), 3);
        RowSliderF("m_wsspeed", "Speed", &settings::walkspeed::speed, 1.f, 500.f, "%.0f");
    }

    SectionHead("Speed Hack");
    RowToggleBind("m_sh", "Enable Speed", &settings::movement::speedhack::enabled, &settings::movement::speedhack::keybind,
                  &settings::movement::speedhack::activation_mode);
    if (settings::movement::speedhack::enabled)
    {
        RowCombo("m_shmode", "Mode", nullptr, &settings::movement::speedhack::mode, k_speed_mode, 2);
        RowCombo("m_shact", "Activation", nullptr, &settings::movement::speedhack::activation_mode, ActivationOptions(), 3);
        RowSliderF("m_shspeed", "Speed", &settings::movement::speedhack::speed, 1.f, 500.f, "%.0f");
    }

    SectionHead("Fly");
    RowToggleBind("m_fly", "Enable Fly", &settings::movement::flyhack::enabled, &settings::movement::flyhack::keybind,
                  &settings::movement::flyhack::activation_mode);
    if (settings::movement::flyhack::enabled)
    {
        RowCombo("m_flymode", "Mode", nullptr, &settings::movement::flyhack::mode, k_fly_mode, 3);
        RowCombo("m_flyact", "Activation", nullptr, &settings::movement::flyhack::activation_mode, ActivationOptions(), 3);
        RowSliderF("m_flyspeed", "Fly Speed", &settings::movement::flyhack::speed, 1.f, 500.f, "%.0f");
    }
}

static void DrawMiscPhysics()
{
    PageHeader("Misc", "Movement & utility tweaks.", nullptr);

    SectionHead("Noclip");
    RowToggleBind("m_noclip", "Noclip", &settings::noclip::enabled, &settings::noclip::keybind,
                  &settings::noclip::activation_mode);
    if (settings::noclip::enabled)
        RowCombo("m_noclipact", "Activation", nullptr, &settings::noclip::activation_mode, ActivationOptions(), 3);

    SectionHead("Jumping & Time");
    RowToggle("m_nojump", "No Jump Cooldown", &settings::nojumpcooldown::enabled);
    RowToggle("m_tick", "Tick Rate", &settings::movement::tickrate::enabled);
    if (settings::movement::tickrate::enabled)
        RowSliderF("m_tickval", "Tickrate", &settings::movement::tickrate::value, 60.f, 1000.f, "%.0f");
    RowToggle("m_spin360", "360 Spin", &settings::spin360::enabled);
    if (settings::spin360::enabled)
    {
        RowBind("m_spin360key", "Spin Key", nullptr, &settings::spin360::keybind);
        RowCombo("m_spin360act", "Activation", nullptr, &settings::spin360::activation_mode, ActivationOptions(), 3);
        RowSliderF("m_spin360speed", "Speed", &settings::spin360::speed, 1.f, 60.f, "%.1f");
    }

    SectionHead("Physics");
    RowToggle("m_spinbot", "Spinbot", &settings::spinbot::enabled);
    RowToggle("m_gravity", "Low Gravity", &settings::exploits::low_gravity);
    if (settings::exploits::low_gravity)
        RowSliderF("m_gravityval", "Gravity", &settings::exploits::gravity_value, 0.f, 196.2f, "%.1f");
}

static void DrawMiscEffects()
{
    static const char* const k_lines[] = { "Damage", "Health", "Distance" };

    PageHeader("Misc", "Movement & utility tweaks.", nullptr);

    SectionHead("Hit Marker");
    RowToggle("me_marker", "Hitmarker", &settings::effects::hitmarker);
    if (settings::effects::hitmarker)
    {
        RowSliderF("me_markersize", "Size", &settings::effects::hitmarker_size, 0.5f, 2.5f, "%.2f");
        RowSliderF("me_markerdur", "Duration", &settings::effects::hitmarker_duration, 0.2f, 1.5f, "%.2f s");
        ColorRow("me_markercol", "Colour", nullptr, settings::effects::hitmarker_color, true);
        ColorRow("me_markerhead", "Head Colour", nullptr, settings::effects::hitmarker_head_color, true);

        static const char* const k_marker_styles[] =
            { "Classic", "Cross", "Chevron", "Ring", "Star", "Bracket" };
        RowCombo("me_markerstyle", "Marker Style", "Which shape the marker is drawn as",
                 &settings::effects::hitmarker_style, k_marker_styles, 6);
        RowSliderF("me_markerpop", "Pop", &settings::effects::hitmarker_pop, 0.f, 2.f, "%.2f");
        RowSliderF("me_markerglow", "Halo", &settings::effects::hitmarker_glow, 0.f, 1.f, "%.2f");

        RowToggle("me_markershock", "Shockwave", &settings::effects::hitmarker_shockwave);
        if (settings::effects::hitmarker_shockwave)
            RowSliderF("me_markershocksize", "Wave Size", &settings::effects::hitmarker_shockwave_size, 0.2f, 3.f, "%.2f");
        RowToggle("me_markersparks", "Sparks", &settings::effects::hitmarker_sparks);
        if (settings::effects::hitmarker_sparks)
        {
            RowSliderI("me_markersparkcount", "Spark Count", &settings::effects::hitmarker_spark_count, 1, 40, "%d");
            RowSliderF("me_markersparkspeed", "Spark Speed", &settings::effects::hitmarker_spark_speed, 0.2f, 3.f, "%.2f");
        }
        RowToggle("me_markerflash", "Impact Flash", &settings::effects::hitmarker_flash);
        RowToggle("me_markercrit", "Crit Pulse", &settings::effects::hitmarker_crit_pulse);
        RowToggle("me_markerstreak", "Chain Counter", &settings::effects::hitmarker_streak);
        if (settings::effects::hitmarker_streak)
            RowSliderF("me_markerstreakwin", "Chain Window", &settings::effects::hitmarker_streak_window, 0.3f, 5.f, "%.1f s");
        RowSliderF("me_markerflashscreen", "Screen Flash", &settings::effects::hitmarker_screen_flash, 0.f, 1.f, "%.2f");
    }

    SectionHead("Hit Data");
    RowToggle("me_hitdata", "Hit Data", &settings::effects::hitdata);
    if (settings::effects::hitdata)
    {
        unsigned lines = 0;
        if (settings::effects::hitdata_damage)   lines |= 1u;
        if (settings::effects::hitdata_health)   lines |= 2u;
        if (settings::effects::hitdata_distance) lines |= 4u;
        RowMulti("me_datalines", "Data Lines", nullptr, &lines, k_lines, 3);
        settings::effects::hitdata_damage   = (lines & 1u) != 0;
        settings::effects::hitdata_health   = (lines & 2u) != 0;
        settings::effects::hitdata_distance = (lines & 4u) != 0;
        RowSliderF("me_datatextsize", "Text Size", &settings::effects::hitdata_size, 10.f, 28.f, "%.0f");
        RowSliderF("me_datatime", "Text Time", &settings::effects::hitdata_duration, 0.4f, 3.f, "%.2f s");
        if (settings::effects::hitdata_distance)
            NoteRow("Distance follows the ESP's studs / metres setting");
    }

    SectionHead("Hit Sound");
    RowToggle("me_hitsound", "Hit Sound", &settings::effects::hitsound);
    if (settings::effects::hitsound)
    {
        const int count = hitsounds::count();
        if (count > 0)
        {
            if (settings::effects::hitsound_index < 0 || settings::effects::hitsound_index >= count)
                settings::effects::hitsound_index = 0;
            RowCombo("me_sound", "Sound", "109 sounds ride inside the exe", &settings::effects::hitsound_index,
                     hitsounds::names(), count);
        }
        else
        {
            NoteRow("no sounds compiled into this build");
        }
        RowSliderF("me_soundvol", "Volume", &settings::effects::hitsound_volume, 0.f, 100.f, "%.0f%%");
        if (RowButton("me_soundplay", "Preview")) hitsounds::play(settings::effects::hitsound_index);
    }

    SectionHead("Crosshair");
    RowToggle("me_cross", "Crosshair", &settings::effects::crosshair);
    if (settings::effects::crosshair)
    {
        RowToggle("me_crosslock", "Lock To Centre", &settings::effects::crosshair_lock);
        if (settings::effects::crosshair_lock)
            NoteRow("Pinned to the middle instead of following the cursor");
        RowSliderF("me_crosslen", "Length", &settings::effects::crosshair_length, 1.f, 40.f, "%.1f");

        RowSliderF("me_crosslenx", "Length X", &settings::effects::crosshair_len_x, 0.10f, 3.f, "%.2f");
        RowSliderF("me_crossleny", "Length Y", &settings::effects::crosshair_len_y, 0.10f, 3.f, "%.2f");
        RowSliderF("me_crosstaper", "Taper", &settings::effects::crosshair_taper, 0.f, 0.95f, "%.2f");
        if (settings::effects::crosshair_taper > 0.02f)
            NoteRow("Arms are wedges: full thickness inside, thinner at the tip");
        RowSliderF("me_crossgap", "Gap", &settings::effects::crosshair_gap, 0.f, 30.f, "%.1f");
        RowSliderF("me_crossth", "Thickness", &settings::effects::crosshair_thickness, 1.f, 6.f, "%.1f");
        static const char* const k_cross_styles[] =
            { "Cross", "X", "T", "Circle", "Chevron", "Dots" };
        RowCombo("me_crossstyle", "Style", "Which shape is drawn at the centre",
                 &settings::effects::crosshair_style, k_cross_styles, 6);

        RowToggle("me_crossspin", "Spin", &settings::effects::crosshair_spin);
        if (settings::effects::crosshair_spin)
        {

            RowSliderF("me_crossspinspeed", "Spin Speed", &settings::effects::crosshair_spin_speed, -3600.f, 3600.f, "%.0f");
            RowToggle("me_crosswindup", "Wind Up", &settings::effects::crosshair_spin_windup);
            if (settings::effects::crosshair_spin_windup)
                NoteRow("The spin accelerates from a standstill instead of starting at speed");
        }

        RowToggle("me_crossoutline", "Outline", &settings::effects::crosshair_outline);
        if (settings::effects::crosshair_outline)
            ColorRow("me_crossoutcol", "Outline Colour", nullptr, settings::effects::crosshair_outline_color, true);

        RowToggle("me_crossdot", "Centre Dot", &settings::effects::crosshair_dot);
        if (settings::effects::crosshair_dot)
            RowSliderF("me_crossdotsize", "Dot Size", &settings::effects::crosshair_dot_size, 1.f, 8.f, "%.1f");

        DividerRow();

        RowSliderF("me_crossglow", "Halo", &settings::effects::crosshair_glow, 0.f, 1.f, "%.2f");
        RowToggle("me_crossrainbow", "Hue Cycle", &settings::effects::crosshair_rainbow);
        if (settings::effects::crosshair_rainbow)
            RowSliderF("me_crossrainbowspeed", "Cycle Speed", &settings::effects::crosshair_rainbow_speed, 0.05f, 2.f, "%.2f");
        else
            ColorRow("me_crosscol", "Colour", nullptr, settings::effects::crosshair_color, true);

        RowToggle("me_crosspulse", "Hit Pulse", &settings::effects::crosshair_hit_pulse);
        if (settings::effects::crosshair_hit_pulse)
        {
            RowSliderF("me_crosspulsesize", "Pulse Size", &settings::effects::crosshair_hit_pulse_size, 0.2f, 2.5f, "%.2f");
            NoteRow("The crosshair swells and warms on a hit of yours");
        }

        RowToggle("me_crossdynamic", "Dynamic Spread", &settings::effects::crosshair_dynamic);
        if (settings::effects::crosshair_dynamic)
        {
            RowSliderF("me_crossdynamicamt", "Spread", &settings::effects::crosshair_dynamic_amount, 0.f, 3.f, "%.2f");
            NoteRow("The arms open while the fire button is down and settle back");
        }

        RowToggle("me_crossstretch", "Stretch", &settings::effects::crosshair_stretch);
        if (settings::effects::crosshair_stretch)
        {
            RowSliderF("me_crossstretchamt", "Amount", &settings::effects::crosshair_stretch_amount, 0.05f, 0.90f, "%.2f");
            RowSliderF("me_crossstretchspd", "Rate", &settings::effects::crosshair_stretch_speed, 0.10f, 6.f, "%.2f");
            NoteRow("The arms breathe in length only - the gap never moves");
        }
    }
}

static void DrawMiscUtility()
{
    PageHeader("Misc", "Movement & utility tweaks.", nullptr);

    SectionHead("Teleport");
    RowToggle("mf_tpcursor", "Teleport to Cursor", &settings::exploits::tp_cursor);
    if (settings::exploits::tp_cursor)
    {
        RowBind("mf_tpkey", "Teleport Key", nullptr, &settings::exploits::tp_cursor_key);
        RowSliderF("mf_tpdist", "Distance", &settings::exploits::tp_cursor_dist, 5.f, 500.f, "%.0f");
    }

    SectionHead("Player Trail");
    RowToggle("tr_on", "Player Trail", &settings::visuals::trail);
    if (settings::visuals::trail)
    {
        static const char* const k_trail_styles[] =
            { "Ribbon", "Line", "Comet", "Beam", "Neon" };
        static const char* const k_trail_colors[] =
            { "Custom", "Team", "Health", "Rainbow", "Speed" };
        static const char* const k_trail_follow[] =
            { "Root", "Feet", "Head" };
        RowCombo("tr_style", "Style", "How the history is drawn",
                 &settings::visuals::trail_style, k_trail_styles, 5);
        RowCombo("tr_color", "Colour Mode", "Team hashes the team id to a hue",
                 &settings::visuals::trail_color_mode, k_trail_colors, 5);
        if (settings::visuals::trail_color_mode == 0)
            ColorRow("tr_col", "Colour", nullptr, settings::visuals::trail_color, true);
        RowCombo("tr_follow", "Follow", "Which part of the rig is sampled",
                 &settings::visuals::trail_follow, k_trail_follow, 3);
        RowSliderF("tr_thick", "Thickness", &settings::visuals::trail_thickness, 0.4f, 10.f, "%.2f");
        RowSliderF("tr_dur", "Duration", &settings::visuals::trail_duration, 0.2f, 5.f, "%.2f s");
        RowSliderI("tr_points", "Max Points", &settings::visuals::trail_max_points, 8, 256, "%d");
        RowSliderF("tr_interval", "Sample Rate", &settings::visuals::trail_update_interval, 0.004f, 0.2f, "%.3f s");
        RowSliderF("tr_minmove", "Min Movement", &settings::visuals::trail_min_movement, 0.f, 2.f, "%.2f");
        RowToggle("tr_moving", "Only When Moving", &settings::visuals::trail_only_moving);
        RowToggle("tr_taper", "Taper", &settings::visuals::trail_taper);
        RowToggle("tr_smooth", "Curve Smoothing", &settings::visuals::trail_smooth);
        if (settings::visuals::trail_smooth)
            RowSliderI("tr_segs", "Curve Segments", &settings::visuals::trail_spline_segments, 2, 16, "%d");
        RowToggle("tr_glow", "Glow", &settings::visuals::trail_glow);
        if (settings::visuals::trail_glow)
        {
            RowSliderF("tr_glowint", "Glow Amount", &settings::visuals::trail_glow_intensity, 0.f, 2.f, "%.2f");
            RowSliderI("tr_glowlayers", "Glow Layers", &settings::visuals::trail_glow_layers, 1, 6, "%d");
        }
        RowToggle("tr_fade", "Fade Out", &settings::visuals::trail_fade);
        if (settings::visuals::trail_fade)
            RowSliderF("tr_fadestart", "Fade Start", &settings::visuals::trail_fade_start, 0.f, 1.f, "%.2f");
        RowToggle("tr_spark", "Head Light", &settings::visuals::trail_head_spark);
        if (settings::visuals::trail_head_spark)
            ColorRow("tr_headcol", "Head Colour", nullptr, settings::visuals::trail_head_color, true);
        RowToggle("tr_drop", "Height Fall", &settings::visuals::trail_height_fall);
        if (settings::visuals::trail_height_fall)
            RowSliderF("tr_dropamt", "Fall Length", &settings::visuals::trail_height_drop, 0.f, 2.f, "%.2f");
        RowToggle("tr_team", "Team Check", &settings::visuals::trail_team_check);
    }

    SectionHead("Text Sheen");
    NoteRow("The band of light that travels across a name while it is drawn");
    RowToggle("sh_name", "Name Sheen", &settings::visuals::name_sheen);
    if (settings::visuals::name_sheen)
    {
        RowSliderF("sh_speed", "Sweep Speed", &settings::visuals::name_sheen_speed, 0.05f, 3.f, "%.2f");
        RowSliderF("sh_width", "Band Width", &settings::visuals::name_sheen_width, 0.05f, 1.f, "%.2f");
        RowSliderF("sh_str", "Brightness", &settings::visuals::name_sheen_strength, 0.f, 1.f, "%.2f");
    }

    SectionHead("Sonar");
    NoteRow("Sweeps the screen and marks what the world geometry says it can see");
    RowToggleBind("so_on", "Sonar", &settings::sonar::enabled, &settings::sonar::keybind,
                  &settings::sonar::activation_mode);
    if (settings::sonar::enabled)
    {
        RowCombo("so_act", "Activation", nullptr, &settings::sonar::activation_mode, ActivationOptions(), 3);
        static const char* const k_sonar_styles[] =
            { "Sweep", "Reticle", "Ring", "Pulse" };
        static const char* const k_sonar_info[] =
            { "Off", "Name", "Name + Range", "Name + Range + HP" };
        RowCombo("so_style", "Style", "What the sweep is drawn as",
                 &settings::sonar::style, k_sonar_styles, 4);
        RowSliderF("so_speed", "Sweep Speed", &settings::sonar::sweep_speed, -600.f, 600.f, "%.0f deg/s");
        RowToggle("so_back", "Ping-Pong", &settings::sonar::sweep_back);
        RowSliderF("so_range", "Range", &settings::sonar::range, 20.f, 2000.f, "%.0f");
        RowSliderF("so_width", "Beam Width", &settings::sonar::beam_width, 0.4f, 8.f, "%.2f");
        RowSliderF("so_reach", "Beam Reach", &settings::sonar::beam_reach, 0.3f, 2.f, "%.2f");
        RowSliderF("so_taper", "Beam Taper", &settings::sonar::beam_taper, 0.f, 0.95f, "%.2f");
        RowSliderF("so_tail", "Afterglow", &settings::sonar::tail_length, 0.f, 180.f, "%.0f deg");
        if (settings::sonar::tail_length > 1.f)
            RowSliderF("so_tailstr", "Afterglow Bright", &settings::sonar::tail_strength, 0.f, 1.5f, "%.2f");

        DividerRow();
        static const char* const k_sonar_color[] =
            { "Custom", "Rainbow", "Heat (Range)" };
        RowCombo("so_colmode", "Colour Mode", "Custom uses the two pickers below",
                 &settings::sonar::color_mode, k_sonar_color, 3);
        if (settings::sonar::color_mode == 1)
            RowSliderF("so_hue", "Cycle Speed", &settings::sonar::hue_speed, 0.005f, 0.6f, "%.3f");
        ColorRow("so_col", "Colour", nullptr, settings::sonar::color, true);
        ColorRow("so_pingcol", "Ping Colour", nullptr, settings::sonar::ping_color, true);

        DividerRow();
        RowToggle("so_grid", "Range Grid", &settings::sonar::grid,
                  "Rings around the hub, spaced across the beam's reach");
        if (settings::sonar::grid)
        {
            RowSliderI("so_gridrings", "Grid Rings", &settings::sonar::grid_rings, 1, 8, "%d");
            RowSliderF("so_gridalpha", "Grid Faintness", &settings::sonar::grid_alpha, 0.02f, 0.8f, "%.2f");
            RowToggle("so_gridlabels", "Range Labels", &settings::sonar::grid_labels);
        }
        RowToggle("so_glow", "Glow", &settings::sonar::glow);
        if (settings::sonar::glow)
            RowSliderF("so_glowamt", "Glow Amount", &settings::sonar::glow_strength, 0.f, 1.f, "%.2f");
        RowToggle("so_rings", "Ping Marks", &settings::sonar::ping_rings);
        if (settings::sonar::ping_rings)
        {
            static const char* const k_sonar_ping[] =
                { "Rings", "Bracket", "Diamond", "Beacon" };
            RowCombo("so_pingstyle", "Mark Style", "What a find is drawn as",
                     &settings::sonar::ping_style, k_sonar_ping, 4);
            RowSliderF("so_pinglife", "Ping Life", &settings::sonar::ping_life, 0.2f, 4.f, "%.2f s");
            RowSliderF("so_pingradius", "Ping Radius", &settings::sonar::ping_radius, 6.f, 120.f, "%.0f");
            if (settings::sonar::ping_style == 0)
                RowToggle("so_pingticks", "Bearing Ticks", &settings::sonar::ping_ticks);
            RowToggle("so_pingspark", "Landing Spark", &settings::sonar::ping_spark,
                      "A short burst on the frame the sweep lands");
            RowToggle("so_pingdrop", "Drop Line", &settings::sonar::ping_drop,
                      "A line from the mark to the target's feet");
        }
        RowCombo("so_info", "Readout", "What is printed beside a ping",
                 &settings::sonar::info_mode, k_sonar_info, 4);
        if (settings::sonar::info_mode > 0)
            RowSliderF("so_infosize", "Readout Size", &settings::sonar::info_size, 8.f, 26.f, "%.0f");
        RowToggle("so_plate", "Readout Plate", &settings::sonar::readout_plate);
        RowToggle("so_latch", "Hold Last Find", &settings::sonar::latch);
        if (settings::sonar::latch)
            RowSliderF("so_hold", "Hold Time", &settings::sonar::hold_time, 0.2f, 5.f, "%.2f s");
        RowToggle("so_los", "Line Of Sight Only", &settings::sonar::los_only,
                  "Skip anything the geometry says is behind a wall");
        NoteRow("Only what the geometry cache can see is marked at full strength");
    }
}

static void DrawMiscPage()
{
    SubTabStrip(kMiscSubs, 4, &g_SubTab[kTabMisc]);
    switch (g_SubTab[kTabMisc])
    {
    case 1:  DrawMiscPhysics(); break;
    case 2:  DrawMiscEffects(); break;
    case 3:  DrawMiscUtility(); break;
    default: DrawMiscMovement(); break;
    }
}

static const char* kWorldSubs[] = { "Lighting", "Atmosphere", "Weather", "Post FX", "Shaders" };

static void DrawWorldLighting()
{
    PageHeader("World", "World, lighting and environment.", nullptr);

    SectionHead("Time");
    RowToggle("w_time", "Time Changer", &settings::world::lighting::time_on);
    if (settings::world::lighting::time_on)
        RowSliderF("w_clock", "Clock Time", &settings::world::lighting::clock_time, 0.f, 24.f, "%.1f h");

    SectionHead("Lighting");
    RowToggle("w_bright", "Brightness", &settings::world::lighting::brightness_on);
    if (settings::world::lighting::brightness_on)
        RowSliderF("w_brightv", "Brightness", &settings::world::lighting::brightness, 0.f, 20.f, "%.2f");
    RowToggle("w_exposure", "Exposure", &settings::world::lighting::exposure_on);
    if (settings::world::lighting::exposure_on)
        RowSliderF("w_exposurev", "Exposure", &settings::world::lighting::exposure_compensation, -5.f, 5.f, "%.2f");
    RowToggle("w_env", "Environment Scales", &settings::world::lighting::env_on);
    if (settings::world::lighting::env_on)
    {
        RowSliderF("w_envdiff", "Env Diffuse", &settings::world::lighting::env_diffuse_scale, 0.f, 2.f, "%.2f");
        RowSliderF("w_envspec", "Env Specular", &settings::world::lighting::env_specular_scale, 0.f, 2.f, "%.2f");
    }
    RowToggle("w_shadows", "No Shadows", &settings::world::lighting::no_shadow);

    SectionHead("Sun");
    RowToggle("w_sun", "Light (sun colour)", &settings::world::lighting::light_on);
    if (settings::world::lighting::light_on)
    {
        ColorRow3("w_suncol", "Light Colour", nullptr, settings::world::lighting::light_color);
        RowSliderF("w_sundirx", "Direction X", &settings::world::lighting::light_dir[0], -1.f, 1.f, "%.2f");
        RowSliderF("w_sundiry", "Direction Y", &settings::world::lighting::light_dir[1], -1.f, 1.f, "%.2f");
        RowSliderF("w_sundirz", "Direction Z", &settings::world::lighting::light_dir[2], -1.f, 1.f, "%.2f");
    }

    SectionHead("Colour");
    RowToggle("w_ambient", "Ambient", &settings::world::lighting::ambient_on);
    if (settings::world::lighting::ambient_on)
        ColorRow3("w_ambientv", "Ambient Colour", nullptr, settings::world::lighting::ambient);
    RowToggle("w_outdoor", "Outdoor Ambient", &settings::world::lighting::outdoor_on);
    if (settings::world::lighting::outdoor_on)
        ColorRow3("w_outdoorv", "Outdoor Colour", nullptr, settings::world::lighting::outdoor_ambient);
    RowToggle("w_fog", "Fog", &settings::world::lighting::fog_on);
    if (settings::world::lighting::fog_on)
    {
        ColorRow3("w_fogcol", "Fog Colour", nullptr, settings::world::lighting::fog_color);
        RowSliderF("w_fogstart", "Fog Start", &settings::world::lighting::fog_start, 0.f, 5000.f, "%.0f");
        RowSliderF("w_fogend", "Fog End", &settings::world::lighting::fog_end, 100.f, 100000.f, "%.0f");
    }
    RowToggle("w_shift", "Color Shift", &settings::world::lighting::shift_on);
    if (settings::world::lighting::shift_on)
    {
        ColorRow3("w_shifttop", "Shift Top", nullptr, settings::world::lighting::color_shift_top);
        ColorRow3("w_shiftbottom", "Shift Bottom", nullptr, settings::world::lighting::color_shift_bottom);
    }
}

static void DrawWorldAtmosphere()
{
    static const char* const k_skybox[] = { "Crystal", "Liquid", "Aurora", "Video" };

    PageHeader("World", "World, lighting and environment.", nullptr);

    SectionHead("Atmosphere");
    RowToggle("wa_on", "Override Atmosphere", &settings::world::atmosphere::enabled);
    if (settings::world::atmosphere::enabled)
    {
        RowSliderF("wa_density", "Density", &settings::world::atmosphere::density, 0.f, 1.f, "%.2f");
        RowSliderF("wa_offset", "Offset", &settings::world::atmosphere::offset, 0.f, 1.f, "%.2f");
        RowSliderF("wa_haze", "Haze", &settings::world::atmosphere::haze, 0.f, 10.f, "%.2f");
        RowSliderF("wa_glare", "Glare", &settings::world::atmosphere::glare, 0.f, 1.f, "%.2f");

        ColorRow3("wa_color", "Haze Colour", nullptr, settings::world::atmosphere::color);
        ColorRow3("wa_decay", "Decay", nullptr, settings::world::atmosphere::decay);
    }

    SectionHead("World Presets");
    RowToggle("wa_night", "Night", &settings::world::presets::night);
    RowToggle("wa_sunset", "Sunset", &settings::world::presets::sunset);
    RowToggle("wa_rainbow", "Rainbow", &settings::world::presets::rainbow);
    RowToggle("wa_fogpulse", "Fog Pulse", &settings::world::presets::fog_pulse);

    SectionHead("Skybox");
    RowToggle("wa_skyslots", "Sun & Moon Size", &settings::world::sky_slots::enabled);
    if (settings::world::sky_slots::enabled)
    {
        RowSliderF("wa_sunang", "Sun Size", &settings::world::sky_slots::sun_angular, 0.f, 60.f, "%.1f");
        RowSliderF("wa_moonang", "Moon Size", &settings::world::sky_slots::moon_angular, 0.f, 60.f, "%.1f");
        RowSliderF("wa_orix", "Orient X", &settings::world::sky_slots::orientation[0], -180.f, 180.f, "%.1f");
        RowSliderF("wa_oriy", "Orient Y", &settings::world::sky_slots::orientation[1], -180.f, 180.f, "%.1f");
        RowSliderF("wa_oriz", "Orient Z", &settings::world::sky_slots::orientation[2], -180.f, 180.f, "%.1f");
    }
    RowToggle("wa_skyon", "Enable Skybox", &settings::skybox::enabled);
    RowCombo("wa_skyshader", "Sky Shader", "Crystal, liquid, aurora or an animated clip",
             &settings::skybox::shader, k_skybox, 4);
    if (settings::skybox::shader == 3)
    {
        SkyVideo::Ensure();
        const std::vector<std::string> clips = SkyVideo::List();
        if (clips.empty())
        {
            NoteRow("No clips available");
        }
        else
        {
            std::vector<const char*> names;
            names.reserve(clips.size());
            for (const std::string& c : clips) names.push_back(c.c_str());
            if (settings::skybox::video < 0 || settings::skybox::video >= (int)names.size())
                settings::skybox::video = 0;
            RowCombo("wa_skyclip", "Clip", "Generated in-process - nothing to install",
                     &settings::skybox::video, names.data(), (int)names.size());
        }
    }
}

static void DrawWorldWeather()
{

    static const char* const k_level[] = { "Off", "Original", "V2", "V3", "V4" };
    static const char* const k_storm[] = { "Off", "Storm", "Storm V2" };

    PageHeader("World", "World, lighting and environment.", nullptr);

    SectionHead("Weather");
    RowCombo("ww_storm", "Storm", "Sets rain, wind, fog and lighting", &settings::world::effects_3d::storm_mode, k_storm, 3);

    RowCombo("ww_snow", "Snow", "V3 lands on roofs, V4 also sizes itself to the space you are in", &settings::world::effects_3d::snow_mode, k_level, 5);
    if (settings::world::effects_3d::snow_mode)
    {
        RowSliderF("ww_snowint", "Intensity", &settings::world::effects_3d::snow_intensity, 0.1f, 5.f, "%.2f");
        RowSliderF("ww_snowspeed", "Speed", &settings::world::effects_3d::snow_speed, 0.1f, 5.f, "%.2f");
        ColorRow3("ww_snowcol", "Colour", nullptr, settings::world::effects_3d::snow_color);
    }

    RowCombo("ww_rain", "Rain", "V3 lands on roofs, V4 also sizes itself to the space you are in", &settings::world::effects_3d::rain_mode, k_level, 5);
    if (settings::world::effects_3d::rain_mode)
    {
        RowSliderF("ww_rainint", "Intensity", &settings::world::effects_3d::rain_intensity, 0.1f, 5.f, "%.2f");
        RowSliderF("ww_rainspeed", "Speed", &settings::world::effects_3d::rain_speed, 0.1f, 5.f, "%.2f");
        ColorRow3("ww_raincol", "Colour", nullptr, settings::world::effects_3d::rain_color);
        RowToggle("ww_ripples", "Ripples", &settings::world::effects_3d::ripples);
        if (settings::world::effects_3d::ripples)
            RowSliderF("ww_ripplesize", "Ripple Size", &settings::world::effects_3d::ripple_size, 0.2f, 4.f, "%.2f");
        RowToggle("ww_rainsound", "Rain Sound", &settings::world::effects_3d::rain_sound);
        if (settings::world::effects_3d::rain_sound)
            RowSliderF("ww_rainvol", "Rain Volume", &settings::world::effects_3d::rain_volume, 0.f, 1.f, "%.2f");
    }

    SectionHead("Wind");
    RowSliderF("ww_windx", "Wind X", &settings::world::effects_3d::wind_x, -30.f, 30.f, "%.1f");
    RowSliderF("ww_windz", "Wind Z", &settings::world::effects_3d::wind_z, -30.f, 30.f, "%.1f");
}

static void DrawWorldPostFx()
{
    PageHeader("World", "World, lighting and environment.", nullptr);

    SectionHead("Bloom & Rays");
    RowToggle("wp_bloom", "Bloom", &settings::world::postfx::bloom::enabled);
    if (settings::world::postfx::bloom::enabled)
    {
        RowSliderF("wp_bloomint", "Intensity", &settings::world::postfx::bloom::intensity, 0.f, 5.f, "%.2f");
        RowSliderF("wp_bloomsize", "Size", &settings::world::postfx::bloom::size, 0.f, 56.f, "%.1f");
        RowSliderF("wp_bloomth", "Threshold", &settings::world::postfx::bloom::threshold, 0.f, 1.f, "%.2f");
    }
    RowToggle("wp_rays", "Sun Rays", &settings::world::postfx::sunrays::enabled);
    if (settings::world::postfx::sunrays::enabled)
    {
        RowSliderF("wp_raysint", "Rays Intensity", &settings::world::postfx::sunrays::intensity, 0.f, 1.f, "%.2f");
        RowSliderF("wp_raysspread", "Rays Spread", &settings::world::postfx::sunrays::spread, 0.f, 1.f, "%.2f");
    }

    SectionHead("Grade & Depth");
    RowToggle("wp_cc", "Color Correction", &settings::world::postfx::colorcorrect::enabled);
    if (settings::world::postfx::colorcorrect::enabled)
    {
        ColorRow3("wp_cctint", "Tint", nullptr, settings::world::postfx::colorcorrect::tint);
        RowSliderF("wp_ccbright", "Brightness", &settings::world::postfx::colorcorrect::brightness, -1.f, 1.f, "%.2f");
        RowSliderF("wp_cccontrast", "Contrast", &settings::world::postfx::colorcorrect::contrast, -1.f, 1.f, "%.2f");
    }
    RowToggle("wp_grade", "Color Grade", &settings::world::postfx::colorgrade::enabled);
    if (settings::world::postfx::colorgrade::enabled)
    {
        static const char* const k_tonemapper[] = { "Default", "Futuristic" };
        RowCombo("wp_tonemap", "Tonemapper", nullptr, &settings::world::postfx::colorgrade::tonemapper, k_tonemapper, 2);
    }
    RowToggle("wp_dof", "Depth of Field", &settings::world::postfx::dof::enabled);
    if (settings::world::postfx::dof::enabled)
    {
        RowSliderF("wp_doffocus", "Focus Distance", &settings::world::postfx::dof::focus_distance, 0.f, 500.f, "%.1f");
        RowSliderF("wp_dofradius", "Focus Radius", &settings::world::postfx::dof::in_focus_radius, 0.f, 50.f, "%.1f");
        RowSliderF("wp_doffar", "Far Blur", &settings::world::postfx::dof::far_intensity, 0.f, 1.f, "%.2f");
        RowSliderF("wp_dofnear", "Near Blur", &settings::world::postfx::dof::near_intensity, 0.f, 1.f, "%.2f");
    }
}

static void DrawWorldShaders()
{
    PageHeader("World", "World, lighting and environment.", nullptr);

    SectionHead("Screen Shaders");
    RowToggle("ws_vignette", "Vignette", &settings::world::shaders::vignette);
    if (settings::world::shaders::vignette)
    {
        RowSliderF("ws_vigstrength", "Strength", &settings::world::shaders::vignette_strength, 0.f, 1.f, "%.2f");
        RowSliderF("ws_vigradius", "Radius", &settings::world::shaders::vignette_radius, 0.3f, 1.f, "%.2f");
    }
    RowToggle("ws_contrast", "Contrast", &settings::world::shaders::contrast);
    if (settings::world::shaders::contrast)
    {
        RowSliderF("ws_contrastval", "Contrast Value", &settings::world::shaders::contrast_value, 0.5f, 2.5f, "%.2f");
        RowSliderF("ws_brightval", "Brightness Value", &settings::world::shaders::brightness_value, -1.f, 1.f, "%.2f");
    }
    RowToggle("ws_chromatic", "Chromatic Aberration", &settings::world::shaders::chromatic_aberration);
    if (settings::world::shaders::chromatic_aberration)
        RowSliderF("ws_chromaticstr", "Chromatic Strength", &settings::world::shaders::chromatic_strength, 0.5f, 8.f, "%.2f");
    RowToggle("ws_grain", "Film Grain", &settings::world::shaders::film_grain);
    if (settings::world::shaders::film_grain)
        RowSliderF("ws_grainstr", "Grain Strength", &settings::world::shaders::film_grain_strength, 0.f, 0.5f, "%.3f");

    SectionHead("World Shaders");
    RowToggle("ws_glossy", "Glossy (world shine)", &settings::world::shaders::glossy);
    if (settings::world::shaders::glossy)
    {
        RowSliderF("ws_glossspec", "Gloss Specular", &settings::world::shaders::glossy_specular, 0.f, 5.f, "%.2f");
        RowSliderF("ws_glossdiff", "Gloss Diffuse", &settings::world::shaders::glossy_diffuse, 0.f, 2.f, "%.2f");
        RowToggle("ws_glossshimmer", "Gloss Shimmer", &settings::world::shaders::glossy_shimmer);
    }

    SectionHead("Terrain");
    RowToggle("ws_terrain", "Override Terrain", &settings::world::terrain::enabled);
    if (settings::world::terrain::enabled)
    {
        RowSliderF("ws_grasslen", "Grass Length", &settings::world::terrain::grass_length, 0.f, 1.f, "%.2f");
        ColorRow3("ws_grasscol", "Grass Colour", nullptr, settings::world::terrain::grass_color);
        ColorRow3("ws_watercol", "Water Colour", nullptr, settings::world::terrain::water_color);
        RowSliderF("ws_waterrefl", "Water Reflectance", &settings::world::terrain::water_reflectance, 0.f, 1.f, "%.2f");
        RowSliderF("ws_watertrans", "Water Transparency", &settings::world::terrain::water_transparency, 0.f, 1.f, "%.2f");
    }
}

static void DrawWorldPage()
{
    SubTabStrip(kWorldSubs, 5, &g_SubTab[kTabWorld]);
    switch (g_SubTab[kTabWorld])
    {
    case 1:  DrawWorldAtmosphere(); break;
    case 2:  DrawWorldWeather();    break;
    case 3:  DrawWorldPostFx();     break;
    case 4:  DrawWorldShaders();    break;
    default: DrawWorldLighting();   break;
    }
}

static const char* kPlayersSubs[] = { "Roster" };

static char  g_PlayerSearch[48] = {};

static unsigned long long g_PlayerSelected = 0;
static bool  g_PlayerShowLocal = true;
static bool  g_PlayerHideTeammates = false;

static unsigned long long g_PlayerSpectating = 0;

static sync::PlayerStatus RowStatus(const sync::PlayerRow& r)
{
    if (r.local) return sync::PlayerStatus::Self;
    if (r.dead)  return sync::PlayerStatus::Dead;
    if (r.instance && friends::is_friend(r.instance)) return sync::PlayerStatus::Friend;
    if (r.teammate) return sync::PlayerStatus::Teammate;
    if (r.enemy)    return sync::PlayerStatus::Enemy;
    return sync::PlayerStatus::Neutral;
}

static ImU32 PlayerStatusColour(sync::PlayerStatus s)
{
    switch (s)
    {
    case sync::PlayerStatus::Friend:
    case sync::PlayerStatus::Teammate: return IM_COL32(107, 217, 117, 240);
    case sync::PlayerStatus::Enemy:    return IM_COL32(226, 104, 104, 240);
    case sync::PlayerStatus::Dead:     return IM_COL32(150, 150, 158, 220);
    case sync::PlayerStatus::Self:     return IM_COL32(126, 176, 255, 240);
    default:                           return IM_COL32(180, 184, 194, 230);
    }
}

static bool PlayerMatch(const sync::PlayerRow& r)
{
    if (g_PlayerHideTeammates && r.teammate && !r.local) return false;
    if (r.local && !g_PlayerShowLocal) return false;
    if (g_PlayerSearch[0] &&
        !IContains(r.name, g_PlayerSearch) &&
        !IContains(r.display_name, g_PlayerSearch) &&
        !IContains(r.team, g_PlayerSearch)) return false;
    return true;
}

static ImU32 PlayerHealthColour(float hp, float maxhp)
{
    const float f = maxhp > 0.f ? Clampf(hp / maxhp, 0.f, 1.f) : 0.f;
    if (f >= 0.5f) return Mix(IM_COL32(240, 199, 89, 255), IM_COL32(107, 217, 117, 255), (f - 0.5f) * 2.f);
    return Mix(IM_COL32(230, 89, 89, 255), IM_COL32(240, 199, 89, 255), f * 2.f);
}

static void PlayerFace(ImDrawList* dl, const sync::PlayerRow& r, ImVec2 mn, ImVec2 mx)
{
    dl->AddRectFilled(mn, mx, GC(IM_COL32(255, 255, 255, 12)), 10.f);
    bool drew = false;
    if (r.user_id > 0 && sync::hooks().player_avatar)
        drew = sync::hooks().player_avatar(dl, r.user_id, mn, mx, GC(IM_COL32(255, 255, 255, 255)));
    if (!drew)
    {
        const char initial[2] = { r.name[0] ? r.name[0] : '?', 0 };
        const ImVec2 ts = TSize(g_FontRegular, initial);
        Txt(dl, g_FontRegular, ImVec2((mn.x + mx.x) * 0.5f - ts.x * 0.5f, (mn.y + mx.y) * 0.5f - ts.y * 0.5f),
            GC(g_T.textDim), initial);
    }
    dl->AddRect(mn, mx, GC(IM_COL32(255, 255, 255, 20)), 10.f, 0, 1.f);
}

static void DrawPlayersPage()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    static sync::PlayerRow rows[sync::kMaxPlayerRows];
    int count = 0;
    if (sync::hooks().players_snapshot)
        count = sync::hooks().players_snapshot(rows, sync::kMaxPlayerRows);
    if (count < 0) count = 0;
    if (count > sync::kMaxPlayerRows) count = sync::kMaxPlayerRows;

    std::sort(rows, rows + count, [](const sync::PlayerRow& a, const sync::PlayerRow& b)
    {
        if (a.local != b.local) return a.local;
        return a.distance < b.distance;
    });

    const unsigned long long aimed = sync::hooks().aim_target_instance ? sync::hooks().aim_target_instance() : 0ULL;

    const float savedR = g_ContentR;
    const float rightW = 250.f, gap = 16.f;
    g_ContentR = savedR - rightW - gap;

    PageHeader("Players", "Player list and per-player settings.", nullptr);

    const float topY = g_RowY;

    {
        char cnt[16];
        snprintf(cnt, sizeof cnt, "%d", count);
        const ImVec2 cs = TSize(g_FontRegular, cnt);
        Txt(dl, g_FontRegular, ImVec2(savedR - 26.f - cs.x, g_RowY + 9.f), GC(g_T.textPrimary), cnt);
        DrawTabIcon(dl, ICON_USERS, ImVec2(savedR - 9.f, g_RowY + 9.f + g_FontRegular->FontSize * 0.5f),
                    15.f, GC(g_T.textDim));
    }

    SearchField("psearch", ImVec2(g_ContentL, g_RowY), g_ContentR - g_ContentL, 32.f,
                g_PlayerSearch, sizeof g_PlayerSearch, "search players");
    g_RowY += 32.f + 8.f;

    RowToggle("p_showlocal", "Show Local Player", &g_PlayerShowLocal);
    RowToggle("p_hideteam", "Hide Teammates", &g_PlayerHideTeammates);
    g_RowY += 2.f;

    const float rowH = 36.f;
    const float listTop = g_RowY;
    int shown = 0;
    for (int i = 0; i < count; i++)
    {
        const sync::PlayerRow& r = rows[i];
        if (!PlayerMatch(r)) continue;

        const ImVec2 a(g_ContentL, g_RowY), b(g_ContentR, g_RowY + rowH);
        ImGui::PushID(6000 + i);
        ImGui::SetCursorScreenPos(a);
        const bool pressed = ImGui::InvisibleButton("##row", ImVec2(b.x - a.x, b.y - a.y));
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        const bool selected = (!r.local && r.instance != 0 && r.instance == g_PlayerSelected);
        float& hv = AnimF("##pr", selected ? 1.f : 0.f);
        hv = Approach(hv, (hovered || selected) ? 1.f : 0.f, K::UiAnimSpeed, ImGui::GetIO().DeltaTime);
        if (hv > 0.003f) dl->AddRectFilled(a, b, GC(IM_COL32(255, 255, 255, (int)(15.f * hv))), 10.f);
        if (selected)    dl->AddRect(a, b, GC(IM_COL32(255, 255, 255, 34)), 10.f, 0, g_T.borderThickness);

        const float cy = (a.y + b.y) * 0.5f;
        PlayerFace(dl, r, ImVec2(a.x + 5.f, cy - 13.f), ImVec2(a.x + 31.f, cy + 13.f));

        char title[96];
        if (r.local) snprintf(title, sizeof title, "%s (you)", r.display_name[0] ? r.display_name : r.name);
        else         snprintf(title, sizeof title, "%s", r.display_name[0] ? r.display_name : r.name);
        Txt(dl, g_FontRegular, ImVec2(a.x + 40.f, cy - g_FontRegular->FontSize - 1.f), GC(g_T.textPrimary), title);
        Txt(dl, g_FontSmall, ImVec2(a.x + 40.f, cy + 2.f), GC(g_T.textDim), r.name[0] ? r.name : "?");

        if (r.dead)
            TxtRight(dl, g_FontSmall, b.x - 44.f, cy - 5.f, GC(g_T.textDim), "dead");
        else if (!r.local && friends::is_friend(r.instance))
            TxtRight(dl, g_FontSmall, b.x - 44.f, cy - 5.f, GC(PlayerStatusColour(sync::PlayerStatus::Friend)), "friend");
        else if (r.teammate && !r.local)
            TxtRight(dl, g_FontSmall, b.x - 44.f, cy - 5.f, GC(g_T.textDim), "team");

        char hp[16];
        snprintf(hp, sizeof hp, "%.0f", r.health);
        TxtRight(dl, g_FontRegular, b.x - 12.f, cy - g_FontRegular->FontSize * 0.5f,
                 GC(r.dead ? g_T.textDim : PlayerHealthColour(r.health, r.max_health)), hp);

        if (pressed && !r.local && r.instance != 0) g_PlayerSelected = r.instance;
        g_RowY += rowH + 2.f;
        ++shown;
    }
    (void)listTop;
    if (shown == 0)
        NoteRow(count == 0 ? "nothing in the cache yet - is the game attached?" : "no player matches that search");

    g_ContentR = savedR;
    const float cx0 = savedR - rightW;

    const float cardH = 452.f;

    const sync::PlayerRow* picked = nullptr;
    for (int i = 0; i < count; i++)
        if (!rows[i].local && rows[i].instance != 0 && rows[i].instance == g_PlayerSelected) picked = &rows[i];

    CardBox(dl, ImVec2(cx0, topY), ImVec2(savedR, topY + cardH));
    if (!picked)
    {
        Txt(dl, g_FontSmall, ImVec2(cx0 + 16.f, topY + 20.f), GC(g_T.textDim), "Pick a player on the left.");
        Txt(dl, g_FontSmall, ImVec2(cx0 + 16.f, topY + 38.f), GC(g_T.textFaint), "Their live readings show up here.");
        return;
    }

    {
        const float av = 104.f;
        const ImVec2 a0(cx0 + 14.f, topY + 14.f);
        const ImVec2 a1(a0.x + av, a0.y + av);
        dl->AddRectFilled(a0, a1, GC(IM_COL32(255, 255, 255, 12)), 10.f);
        bool drew = false;
        if (picked->user_id > 0 && sync::hooks().player_avatar)
            drew = sync::hooks().player_avatar(dl, picked->user_id, a0, a1, GC(IM_COL32(255, 255, 255, 255)));
        if (!drew)
            PlayerFace(dl, *picked, a0, a1);
        dl->AddRect(a0, a1, GC(IM_COL32(255, 255, 255, 22)), 10.f, 0, 1.f);

        const float tx = a1.x + 12.f;
        const float tr = savedR - 14.f;
        TxtBold(dl, picked->display_name[0] ? picked->display_name : picked->name,
                ImVec2(tx, a0.y + 10.f), GC(g_T.textPrimary));

        char handle[80];
        snprintf(handle, sizeof handle, "@%s", picked->name[0] ? picked->name : "?");
        Txt(dl, g_FontSmall, ImVec2(tx, a0.y + 32.f), GC(g_T.textDim), handle);

        const sync::PlayerStatus status = RowStatus(*picked);
        TxtRight(dl, g_FontSmall, tr, a0.y + 32.f, PlayerStatusColour(status),
                 sync::PlayerStatusName(status));

        {
            const float barW = Maxf(40.f, tr - tx);
            const float frac = picked->max_health > 0.f
                ? Clampf(picked->health / picked->max_health, 0.f, 1.f) : 0.f;
            const float by = a0.y + 58.f;
            dl->AddRectFilled(ImVec2(tx, by), ImVec2(tx + barW, by + 6.f),
                              GC(IM_COL32(255, 255, 255, 26)), 3.f);
            if (frac > 0.f)
                dl->AddRectFilled(ImVec2(tx, by), ImVec2(tx + barW * frac, by + 6.f),
                                  GC(PlayerHealthColour(picked->health, picked->max_health)), 3.f);

            char hv[32];
            if (picked->max_health > 0.f) snprintf(hv, sizeof hv, "%.0f / %.0f", picked->health, picked->max_health);
            else                          snprintf(hv, sizeof hv, "%.0f", picked->health);
            Txt(dl, g_FontSmall, ImVec2(tx, by + 10.f), GC(g_T.textDim), hv);
        }
    }

    float y = topY + 14.f + 104.f + 12.f;

    char buf[64];

    snprintf(buf, sizeof buf, "%lld", picked->user_id);
    KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "userid", buf, g_T.textPrimary); y += 26.f;

    if (picked->distance >= 0.f) snprintf(buf, sizeof buf, "%.0f m", picked->distance);
    else                         snprintf(buf, sizeof buf, "unknown");
    KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "distance", buf, g_T.textPrimary); y += 26.f;
    KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "team",
          picked->team[0] ? picked->team : "none", g_T.textPrimary); y += 26.f;
    KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "relationship",
          picked->teammate ? "teammate" : "enemy", g_T.textPrimary); y += 26.f;
    snprintf(buf, sizeof buf, "%s%s", picked->rig_type ? "R15" : "R6", picked->dead ? " (corpse)" : "");
    KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "rig", buf, g_T.textPrimary); y += 26.f;

    KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "tool",
          picked->tool[0] ? picked->tool : "-", g_T.textPrimary); y += 26.f;

    const int age_days = sync::hooks().player_account_age && picked->instance
                       ? sync::hooks().player_account_age(picked->instance)
                       : -1;
    if (age_days >= 0)
    {
        snprintf(buf, sizeof buf, "%d d", age_days);
        KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "age", buf, g_T.textPrimary); y += 26.f;
    }
    else
    {
        KVRow(dl, cx0 + 14.f, savedR - 14.f, y, 26.f, "age", "-", g_T.textDim); y += 26.f;
    }

    const bool locked = aimed != 0 && picked->instance != 0 && aimed == picked->instance;
    const bool  is_friend = picked->instance && friends::is_friend(picked->instance);
    const float bw = (rightW - 28.f - 8.f) * 0.5f;

    if (ButtonControl("p_friend", ImVec2(cx0 + 14.f, y), ImVec2(bw, 30.f),
                      is_friend ? "unfriend" : "friend"))
    {
        const bool now_friend = friends::toggle(picked->instance);
        if (now_friend && locked && sync::hooks().player_target)
            sync::hooks().player_target(0ULL);
        PushToast(is_friend ? "friend removed" : "marked as friend", true);
    }
    if (ButtonControl("p_target", ImVec2(cx0 + 22.f + bw, y), ImVec2(bw, 30.f),
                      locked ? "untarget" : "target"))
    {
        if (sync::hooks().player_target)
        {
            sync::hooks().player_target(locked ? 0ULL : picked->instance);
            PushToast(locked ? "target cleared" : "aim and trigger focus on them", true);
        }
        else PushToast("aim target is not wired in this build", false);
    }
    y += 30.f + 8.f;

    if (ButtonControl("p_spec", ImVec2(cx0 + 14.f, y), ImVec2(bw, 30.f),
                      g_PlayerSpectating == picked->instance ? "unspectate" : "spectate"))
    {
        const bool on = g_PlayerSpectating != picked->instance;
        if (sync::hooks().player_spectate && sync::hooks().player_spectate(picked->instance, on))
        {
            g_PlayerSpectating = on ? picked->instance : 0;
            PushToast(on ? "spectating" : "camera released", true);
        }
        else PushToast("the camera could not be taken", false);
    }
    if (ButtonControl("p_tp", ImVec2(cx0 + 22.f + bw, y), ImVec2(bw, 30.f), "teleport"))
    {
        if (sync::hooks().player_teleport && sync::hooks().player_teleport(picked->instance))
            PushToast("moved to them", true);
        else
            PushToast("that player has no readable position", false);
    }
    y += 30.f + 10.f;

    if (ButtonControl("p_copy_id", ImVec2(cx0 + 14.f, y), ImVec2(bw, 28.f), "copy id"))
    {
        snprintf(buf, sizeof buf, "%lld", picked->user_id);
        ImGui::SetClipboardText(buf);
        PushToast("UserId copied", true);
    }
    if (ButtonControl("p_copy_name", ImVec2(cx0 + 22.f + bw, y), ImVec2(bw, 28.f), "copy name"))
    {
        ImGui::SetClipboardText(picked->name);
        PushToast("name copied", true);
    }
    y += 28.f + 8.f;

    g_RowY = Maxf(g_RowY, y);
}

static void DrawExplorerPage()
{
    PageHeader("Explorer", "Instance hierarchy for the current place.", nullptr);

    if (!sync::hooks().explorer_page)
    {
        NoteRow("the workspace shows up once the overlay is attached to a game");
        return;
    }

    const ImVec2 pos(g_ContentL, g_RowY);
    const ImVec2 size(g_ContentR - g_ContentL, Maxf(120.f, g_ContentBottom - g_RowY));
    sync::hooks().explorer_page(pos, size);

    g_RowY = pos.y + size.y;
}

static const char* kCfgSubs[] = { "Configs", "Settings", "Appearance" };

static char  g_CfgNewName[32] = {};
static const char* g_CfgStatus = nullptr;
static bool  g_CfgStatusOk = true;

static void DrawConfigConfigs()
{
    sync::Hooks& h = sync::hooks();

    PageHeader("Config", "Theme, profiles & HUD.", nullptr);

    std::vector<const char*> names;
    names.reserve(sync::state().cfg_names.size());
    for (const std::string& n : sync::state().cfg_names) names.push_back(n.c_str());
    const bool has_configs = !names.empty();

    SectionHead("Configs");
    if (has_configs)
    {
        if (sync::state().cfg_selected < 0 || sync::state().cfg_selected >= (int)names.size())
            sync::state().cfg_selected = 0;
        RowCombo("cfg_sel", "Selected", "Kept in the overlay's own config folder",
                 &sync::state().cfg_selected, names.data(), (int)names.size());
    }
    else
    {
        NoteRow("No configs found - save one below to make the first");
    }

    {
        const float bw = 104.f, gap = 8.f;
        const float bx = g_ContentL, by = g_RowY;
        if (ButtonControl("cfg_load", ImVec2(bx, by), ImVec2(bw, 30.f), "Load") && has_configs && h.cfg_load)
        {
            g_CfgStatusOk = h.cfg_load(names[sync::state().cfg_selected]);
            g_CfgStatus = g_CfgStatusOk ? "config loaded" : "load failed";
        }
        if (ButtonControl("cfg_save", ImVec2(bx + bw + gap, by), ImVec2(bw, 30.f), "Save") && has_configs && h.cfg_save)
        {
            g_CfgStatusOk = h.cfg_save(names[sync::state().cfg_selected]);
            g_CfgStatus = g_CfgStatusOk ? "config saved" : "save failed";
        }
        if (ButtonControl("cfg_del", ImVec2(bx + (bw + gap) * 2.f, by), ImVec2(bw, 30.f), "Delete") && has_configs && h.cfg_delete)
        {
            g_CfgStatusOk = h.cfg_delete(names[sync::state().cfg_selected]);
            g_CfgStatus = g_CfgStatusOk ? "config deleted" : "delete failed";
            sync::state().cfg_selected = 0;
        }
        if (ButtonControl("cfg_ref", ImVec2(bx + (bw + gap) * 3.f, by), ImVec2(bw, 30.f), "Refresh") && h.cfg_refresh)
        {
            h.cfg_refresh();
            g_CfgStatusOk = true;
            g_CfgStatus = "config list refreshed";
        }
        g_RowY = by + 30.f + 8.f;
    }

    SectionHead("New Config");
    if (RowInput("cfg_newname", "Name", g_CfgNewName, sizeof g_CfgNewName, "my config"))
    {
        if (g_CfgNewName[0] && h.cfg_save)
        {
            g_CfgStatusOk = h.cfg_save(g_CfgNewName);
            g_CfgStatus = g_CfgStatusOk ? "config created" : "create failed";
            if (g_CfgStatusOk)
            {
                g_CfgNewName[0] = '\0';
                if (h.cfg_refresh) h.cfg_refresh();
            }
        }
        else
        {
            g_CfgStatusOk = false;
            g_CfgStatus = "enter a config name first";
        }
    }
    if (RowButton("cfg_create", "Create Config", true))
    {
        if (g_CfgNewName[0] && h.cfg_save)
        {
            g_CfgStatusOk = h.cfg_save(g_CfgNewName);
            g_CfgStatus = g_CfgStatusOk ? "config created" : "create failed";
            if (g_CfgStatusOk)
            {
                g_CfgNewName[0] = '\0';
                if (h.cfg_refresh) h.cfg_refresh();
            }
        }
        else
        {
            g_CfgStatusOk = false;
            g_CfgStatus = "enter a config name first";
        }
    }
    if (g_CfgStatus) StatusRow("Status", g_CfgStatus, g_CfgStatusOk);
}


static void DrawConfigSettings()
{
    PageHeader("Config", "Keybind, HUD and the plates.", nullptr);

    SectionHead("Menu");
    RowBind("cfg_menukey", "Menu Key", "Opens and closes this panel", &sync::state().menu_key);
    RowToggle("cfg_wmhide", "Hide Watermark", &settings::menu::hide_watermark);
    NoteRow("Hides the little HUD card in the corner");

    SectionHead("Frame");
    RowToggle("cfg_vsync", "V-Sync", &settings::menu::vsync,
              "Present on the display's own rate; the FPS limit is ignored while it is on");
    RowSliderI("cfg_fpslimit", "FPS Limit", &settings::menu::fps_limit, 0, 480, "%d fps");
    NoteRow(settings::menu::fps_limit > 0
        ? "Caps the overlay's frames; the aim sizes its mouse steps from this number"
        : "0 = uncapped - the aim then falls back to one step every 4 ms");
    RowToggle("cfg_perfmode", "Performance Mode", &settings::menu::performance_mode,
              "Yields half a millisecond of every frame to the game");

    SectionHead("Places");
    RowToggle("cfg_ignoreplace", "Ignore Unsupported Place",
              &settings::menu::ignore_unsupported_place,
              "Skips the warning popup on every place that is not the supported one");

    SectionHead("Draggable plates");
    RowToggle("dk_on", "Enable Plates", &settings::docks::enabled, "The HUD surfaces beside the panel");
    if (settings::docks::enabled)
    {
        static const char* const k_dock_names[] = { "Session", "Nearby", "Sonar" };
        for (int i = 0; i < settings::docks::kCount; ++i)
        {
            char id[32];
            snprintf(id, sizeof id, "dk_open%d", i);
            RowToggle(id, k_dock_names[i], &settings::docks::open[i]);
        }
        RowSliderF("dk_op", "Opacity", &settings::docks::opacity, 0.35f, 1.f, "%.2f");
        RowSliderF("dk_scale", "Scale", &settings::docks::scale, 0.75f, 1.5f, "%.2f x");
        RowToggle("dk_snap", "Edge Snap", &settings::docks::snap, "Magnetic screen edges on release");
        if (settings::docks::snap)
        {
            RowSliderF("dk_snap_px", "Snap Range", &settings::docks::snap_px, 4.f, 64.f, "%.0f px");
            RowSliderF("dk_snap_gap", "Snap Margin", &settings::docks::snap_gap, 0.f, 48.f, "%.0f px");
        }
        RowToggle("dk_lift", "Hover Edge", &settings::docks::hover_lift, "The plate's edge brightens under the pointer");
        RowToggle("dk_fold", "Fold on Click", &settings::docks::fold_on_click);
        RowToggle("dk_anim", "Animate", &settings::docks::animate, "Open, fold and snap motion");
        if (settings::docks::animate)
            RowSliderF("dk_anim_s", "Motion Speed", &settings::docks::anim_speed, 0.5f, 2.f, "%.2f x");
        RowToggle("dk_titles", "Caption", &settings::docks::show_titles, "The plate's own name in its header");
        NoteRow("Plates drag with the left mouse button, with the panel open or shut");

        if (RowButton("dk_reset", "Reset plate positions"))
        {
            static const float kx[settings::docks::kCount] = { 260.f, 16.f, 300.f };
            static const float ky[settings::docks::kCount] = { 232.f, 232.f, 232.f };
            for (int i = 0; i < settings::docks::kCount; ++i)
            {
                settings::docks::x[i] = kx[i];
                settings::docks::y[i] = ky[i];
                settings::docks::folded[i] = false;
            }
        }
    }
}

static void DrawConfigAppearance()
{
    sync::Hooks& h = sync::hooks();

    PageHeader("Config", "Palette and accent.", nullptr);

    SectionHead("Palette");
    const int theme_count = h.theme_count ? h.theme_count() : 0;
    if (theme_count > 0 && h.theme_name)
    {
        std::vector<const char*> theme_names;
        theme_names.reserve(theme_count);
        for (int i = 0; i < theme_count; ++i) theme_names.push_back(h.theme_name(i));
        const int before = settings::theme::index;
        RowCombo("cfg_theme", "Palette", "Used by the overlay's own widgets",
                 &settings::theme::index, theme_names.data(), theme_count);
        if (settings::theme::index != before && h.theme_apply)
            h.theme_apply(settings::theme::index);
    }
    if (RowToggle("cfg_accent", "Custom Accent", &settings::appearance::custom_accent))
    {
        if (h.theme_apply) h.theme_apply(settings::theme::index);
    }
    if (settings::appearance::custom_accent)
    {
        const float was[4] = { settings::appearance::accent[0], settings::appearance::accent[1], settings::appearance::accent[2], settings::appearance::accent[3] };
        ColorRow("cfg_accentcol", "Accent", nullptr, settings::appearance::accent, true);
        const bool accent_moved = was[0] != settings::appearance::accent[0] || was[1] != settings::appearance::accent[1] || was[2] != settings::appearance::accent[2] || was[3] != settings::appearance::accent[3];
        if (accent_moved && h.theme_apply) h.theme_apply(settings::theme::index);
    }
}

static void DrawConfigPage()
{
    if (g_SubTab[kTabConfig] < 0 || g_SubTab[kTabConfig] > 2)
        g_SubTab[kTabConfig] = 0;

    SubTabStrip(kCfgSubs, 3, &g_SubTab[kTabConfig]);
    switch (g_SubTab[kTabConfig])
    {
    case 1:  DrawConfigSettings();   break;
    case 2:  DrawConfigAppearance(); break;
    default: DrawConfigConfigs();    break;
    }
}

static int SyncPlayerCount()
{
    if (!sync::hooks().players_snapshot) return 0;
    static sync::PlayerRow scratch[sync::kMaxPlayerRows];
    return sync::hooks().players_snapshot(scratch, sync::kMaxPlayerRows);
}

