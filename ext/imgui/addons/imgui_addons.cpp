#include "imgui_addons.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <map>
#include <string>
#include <cmath>

using namespace ImGui;

namespace
{
    struct aether_slot_t { float v; int frame; };

    inline std::map<ImGuiID, aether_slot_t>& slots()
    {
        static std::map<ImGuiID, aether_slot_t> s;
        return s;
    }

    inline float& slot(ImGuiID id, unsigned salt, float init)
    {
        std::map<ImGuiID, aether_slot_t>& s = slots();
        const int frame = ImGui::GetFrameCount();
        const ImGuiID key = id ^ (ImGuiID)(salt * 0x9e3779b9u);
        auto it = s.find(key);
        if (it == s.end())
        {
            if (s.size() > 2048)
                for (auto i = s.begin(); i != s.end(); )
                    i = (frame - i->second.frame > 600) ? s.erase(i) : ++i;
            it = s.emplace(key, aether_slot_t{ init, frame }).first;
        }
        it->second.frame = frame;
        return it->second.v;
    }

    inline float dt_clamped() { return ImMin(ImGui::GetIO().DeltaTime, 1.f / 20.f); }

    inline float approach(float cur, float tgt, float half_life)
    {
        if (!ImAdd::animations || half_life <= 0.0001f)
            return tgt;
        const float k = 1.f - powf(0.5f, dt_clamped() / half_life);
        return cur + (tgt - cur) * k;
    }

    inline float anim(ImGuiID id, unsigned salt, float tgt, float init, float half_life = 0.06f)
    {
        float& v = slot(id, salt, init);
        v = approach(v, tgt, half_life);
        return v;
    }

    inline float pulse(ImGuiID id, unsigned salt, bool on, float decay = 3.0f)
    {
        float& v = slot(id, salt, 0.f);
        if (on) v = 1.f;
        else    v = ImMax(0.f, v - dt_clamped() * decay * (ImAdd::animations ? 1.f : 12.f));
        return v;
    }

    inline ImU32 with_a(ImU32 c, float a)
    {
        return (c & 0x00FFFFFFu) | ((ImU32)(ImClamp(a, 0.f, 1.f) * 255.f + 0.5f) << 24);
    }

    inline ImU32 mix32(ImU32 a, ImU32 b, float t)
    {
        t = ImClamp(t, 0.f, 1.f);
        const int ac[4] = { (int)((a >> 24) & 0xFF), (int)((a >> 16) & 0xFF), (int)((a >> 8) & 0xFF), (int)(a & 0xFF) };
        const int bc[4] = { (int)((b >> 24) & 0xFF), (int)((b >> 16) & 0xFF), (int)((b >> 8) & 0xFF), (int)(b & 0xFF) };
        return ((ImU32)(ac[0] + (bc[0] - ac[0]) * t) << 24) |
               ((ImU32)(ac[1] + (bc[1] - ac[1]) * t) << 16) |
               ((ImU32)(ac[2] + (bc[2] - ac[2]) * t) << 8)  |
               ((ImU32)(ac[3] + (bc[3] - ac[3]) * t));
    }

    inline ImU32 lighten(ImU32 c, float t) { return mix32(c, IM_COL32(255, 255, 255, (int)((c >> 24) & 0xFF)), t); }
    inline ImU32 darken(ImU32 c, float t)  { return mix32(c, IM_COL32(0, 0, 0, (int)((c >> 24) & 0xFF)), t); }

    inline ImU32 accent_u32(float a = 1.f) { return ImGui::GetColorU32(ImGuiCol_SliderGrab, a); }
    inline ImU32 text_u32(float a = 1.f)   { return ImGui::GetColorU32(ImGuiCol_Text, a); }
    inline ImU32 dim_u32(float a = 1.f)    { return ImGui::GetColorU32(ImGuiCol_TextDisabled, a); }
    inline ImU32 frame_u32(float a = 1.f)  { return ImGui::GetColorU32(ImGuiCol_FrameBg, a); }
    inline ImU32 frame_hi_u32(float a = 1.f) { return ImGui::GetColorU32(ImGuiCol_FrameBgHovered, a); }
    inline ImU32 btn_u32(float a = 1.f)    { return ImGui::GetColorU32(ImGuiCol_Button, a); }
    inline ImU32 btn_hi_u32(float a = 1.f) { return ImGui::GetColorU32(ImGuiCol_ButtonHovered, a); }
    inline ImU32 border_u32(float a = 1.f) { return ImGui::GetColorU32(ImGuiCol_Border, a); }

    inline void plate(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 top, ImU32 bot, float rounding)
    {
        const float w = b.x - a.x;
        const float h = b.y - a.y;
        if (w <= 0.0f || h <= 0.0f)
            return;

        const float r = ImClamp(rounding, 0.0f, ImMin(w, h) * 0.5f);
        if (r <= 0.5f)
        {
            dl->AddRectFilledMultiColor(a, b, top, top, bot, bot);
            return;
        }

        const int   bands = (int)ImClamp(ImCeil(h * 0.5f), 2.0f, 10.0f);
        const float bh    = h / (float)bands;
        for (int i = 0; i < bands; ++i)
        {
            const float y0 = a.y + bh * (float)i;
            const float y1 = (i == bands - 1) ? b.y : y0 + bh;
            const ImU32 c  = mix32(top, bot, (float)i / (float)(bands - 1));

            const float dt = y0 - a.y;
            const float db = b.y - y1;
            float ins = 0.0f;
            if (dt < r) ins = ImMax(ins, r - sqrtf(ImMax(0.0f, r * r - (r - dt) * (r - dt))));
            if (db < r) ins = ImMax(ins, r - sqrtf(ImMax(0.0f, r * r - (r - db) * (r - db))));

            if (b.x - ins - (a.x + ins) < 0.5f)
                continue;
            dl->AddRectFilled(ImVec2(a.x + ins, y0), ImVec2(b.x - ins, y1), c);
        }
    }

    inline void halo(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding,
                     float amount, int rings = 3, float spread = 1.0f)
    {
        if (amount <= 0.004f)
            return;
        for (int i = rings; i >= 1; --i)
        {
            const float e = (float)i * spread;
            dl->AddRect(ImVec2(a.x - e, a.y - e), ImVec2(b.x + e, b.y + e),
                with_a(col, amount / (float)i), rounding + e, 0, 1.f);
        }
    }
}

ImVec4 ImAdd::HexToColorVec4(unsigned int hex_color, float alpha)
{
    ImVec4 color;

    color.x = ((hex_color >> 16) & 0xFF) / 255.0f;
    color.y = ((hex_color >> 8) & 0xFF) / 255.0f;
    color.z = (hex_color & 0xFF) / 255.0f;
    color.w = alpha;

    return color;
}

void ImAdd::DoubleText(ImVec4 color1, ImVec4 color2, const char* label1, const char* label2)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(std::string(label1 + std::string(label2)).c_str());
    const ImVec2 label1_size = CalcTextSize(label1, NULL, true);
    const ImVec2 label2_size = CalcTextSize(label2, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize), label1_size.x + label2_size.x, g.FontSize);

    const ImRect total_bb(pos, pos + size);
    ItemSize(total_bb);
    if (!ItemAdd(total_bb, id)) {
        return;
    }

    window->DrawList->AddText(pos, GetColorU32(color1), label1);
    window->DrawList->AddText(pos + ImVec2(size.x - ImGui::CalcTextSize(label2).x, 0), GetColorU32(color2), label2);
}

void ImAdd::SeparatorText(const char* label, float thickness)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize + 8.f), label_size.x, g.FontSize + 8.f);

    const ImRect total_bb(pos, pos + size);
    ItemSize(total_bb);
    if (!ItemAdd(total_bb, id)) {
        return;
    }

    ImDrawList* sep_dl = window->DrawList;
    const ImU32 sep_accent = GetColorU32(ImGuiCol_SliderGrab);
    const ImU32 sep_fade   = sep_accent & 0x00FFFFFF;

    const float tick_w = 2.0f;
    const float tick_h = ImMin(size.y * 0.78f, label_size.y + 4.0f);
    const float ty0    = pos.y + (size.y - tick_h) * 0.5f;

    sep_dl->AddRectFilled(ImVec2(pos.x - 1.0f, ty0 - 1.0f), ImVec2(pos.x + tick_w + 1.0f, ty0 + tick_h + 1.0f),
                          with_a(sep_accent, 0.16f), 3.0f);
    sep_dl->AddRectFilled(ImVec2(pos.x, ty0), ImVec2(pos.x + tick_w, ty0 + tick_h), sep_accent, 1.0f);

    const ImU32 sep_lab = mix32(GetColorU32(ImGuiCol_TextDisabled), GetColorU32(ImGuiCol_Text), 0.55f);
    sep_dl->AddText(ImVec2(pos.x + tick_w + 8.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 70), label);
    sep_dl->AddText(ImVec2(pos.x + tick_w + 7.0f, pos.y), mix32(sep_lab, sep_accent, 0.10f), label);

    if (thickness > 0)
    {
        const float line_x0 = pos.x + tick_w + 7.0f + label_size.x + style.ItemInnerSpacing.x;
        const float line_y  = pos.y + size.y * 0.5f;
        sep_dl->AddRectFilledMultiColor(ImVec2(line_x0, line_y - thickness * 0.5f),
                                        ImVec2(pos.x + size.x, line_y + thickness * 0.5f),
                                        sep_accent, sep_fade, sep_fade, sep_accent);

        const float line_x1 = pos.x + size.x;
        const float span    = line_x1 - line_x0;
        if (span > 24.0f)
        {
            const float phase = (float)((id >> 5) % 977u) * 0.0174f;
            const float sx    = fmodf((float)ImGui::GetTime() * 0.20f + phase, 1.0f);
            const float gw    = ImMin(span * 0.32f, 110.0f);
            const float g0    = ImClamp(line_x0 + span * sx, line_x0, line_x1);
            const float g1    = ImClamp(line_x0 + span * sx + gw, line_x0, line_x1);
            if (g1 > g0 + 1.0f)
            {
                const ImU32 head = lighten(sep_accent, 0.35f);
                const float hy   = thickness * 0.5f;
                sep_dl->AddRectFilledMultiColor(ImVec2(g0, line_y - hy), ImVec2(g1, line_y + hy),
                                                with_a(sep_accent, 0.0f), with_a(head, 0.85f),
                                                with_a(head, 0.85f), with_a(sep_accent, 0.0f));
            }
        }
    }
}

void ImAdd::VSeparator(float margin, float thickness)
{
    if (thickness <= 0)
        return;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(thickness, -0.1f), thickness, thickness);

    const ImRect bb(pos, pos + size);
    const ImRect bb_rect(pos + ImVec2(0, margin), pos + size - ImVec2(0, margin));

    ItemSize(ImVec2(thickness, 0.0f));
    if (!ItemAdd(bb, 0))
        return;

    window->DrawList->AddRectFilled(bb_rect.Min, bb_rect.Max, GetColorU32(ImGuiCol_Border));
}

bool ImAdd::RadioFrame(const char* label, int* v, int radio_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held, active;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    active = *v == radio_id;
    if (pressed) {
        *v = radio_id;
    }

    ImVec4 colFrameMain = GetStyleColorVec4(ImGuiCol_Header);
    ImVec4 colFrameNull = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame = (active ? colFrameMain : colFrameNull);

    ImVec4 colBorderMain = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 colBorderNull = colBorderMain; colBorderNull.w = 0.0f;
    ImVec4 colBorder = (active ? colBorderMain : colBorderNull);

    ImVec4 colLabel = GetStyleColorVec4(active ? ImGuiCol_SliderGrab : (hovered && !held) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Border;
        ImVec4 Label;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Label = colLabel;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);
    it_anim->second.Border = ImLerp(it_anim->second.Border, colBorder, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);
    it_anim->second.Label = ImLerp(it_anim->second.Label, colLabel, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);

    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(pos, pos + size, GetColorU32(it_anim->second.Frame), style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + size, GetColorU32(it_anim->second.Border), style.FrameRounding, 0, style.FrameBorderSize);

    window->DrawList->AddText(pos + ImVec2(size.x / 2 - label_size.x / 2, size.y / 2 - label_size.y / 2 + 1), GetColorU32(it_anim->second.Label), label);

    return pressed;
}

bool ImAdd::SelectableFrameIcon(const char* str_id, ImTextureID icon_texture, bool* active, const ImVec2& size_arg, const ImVec2& icon_padding)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(str_id);

    const ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, GetFrameHeight(), GetFrameHeight());

    const ImRect total_bb(pos, pos + size);
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
    {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
        return false;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
    {
        *active = !*active;
        MarkItemEdited(id);
    }

    ImVec4 colFrame = GetStyleColorVec4(*active ? ImGuiCol_Header : ImGuiCol_ChildBg);
    ImVec4 colIcon = GetStyleColorVec4((*active || (hovered && !held)) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Icon;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Icon = colIcon;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / IMADD_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Icon = ImLerp(it_anim->second.Icon, colIcon, 1.0f / IMADD_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    RenderNavHighlight(total_bb, id);

    window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32(it_anim->second.Frame), style.TabRounding);
    if (style.FrameBorderSize > 0 && *active)
        window->DrawList->AddRect(total_bb.Min, total_bb.Max, GetColorU32(ImGuiCol_Border), style.TabRounding, 0, style.FrameBorderSize);

    if (icon_texture != nullptr)
        window->DrawList->AddImage(icon_texture, pos + icon_padding, pos + size - icon_padding, ImVec2(), ImVec2(1, 1), GetColorU32(it_anim->second.Icon));

    return pressed;
}

bool ImAdd::RadioFrameGradient(const char* label, int* v, int radio_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held, active;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    active = *v == radio_id;
    if (pressed) {
        *v = radio_id;
    }

    ImVec4 colFrameMain = GetStyleColorVec4(ImGuiCol_SliderGrab); colFrameMain.w = 0.2f;
    ImVec4 colFrameNull = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame = (active ? colFrameMain : colFrameNull);

    ImVec4 colLabel = GetStyleColorVec4((active || (hovered && !held)) ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    ImVec4 colLine = GetStyleColorVec4(active ? ImGuiCol_SliderGrab : (hovered && !held) ? ImGuiCol_SliderGrabActive : ImGuiCol_Border);

    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Label;
        ImVec4 Line;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Label = colLabel;
        it_anim->second.Line = colLine;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);
    it_anim->second.Label = ImLerp(it_anim->second.Label, colLabel, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);
    it_anim->second.Line = ImLerp(it_anim->second.Line, colLine, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);

    RenderNavHighlight(bb, id);

    ImVec4 transparent_col = it_anim->second.Frame; transparent_col.w = 0.0f;

    ImDrawList* tab_dl = window->DrawList;
    const float tab_r = style.TabRounding;

    tab_dl->AddRectFilledMultiColor(pos, pos + size, GetColorU32(transparent_col), GetColorU32(transparent_col), GetColorU32(it_anim->second.Frame), GetColorU32(it_anim->second.Frame));

    if (style.FrameBorderSize > 0) {
        const float th = ImMax(2.0f, style.FrameBorderSize * 1.6f);
        const ImVec2 ia(pos.x + style.FramePadding.x * 0.5f, pos.y + size.y - th);
        const ImVec2 ib(pos.x + size.x - style.FramePadding.x * 0.5f, pos.y + size.y);
        const ImU32  line = GetColorU32(it_anim->second.Line);

        for (int gi = 3; gi >= 1; --gi)
            tab_dl->AddRectFilled(ImVec2(ia.x - (float)gi, ia.y - (float)gi * 0.6f),
                                  ImVec2(ib.x + (float)gi, ib.y + (float)gi * 0.6f),
                                  (line & 0x00FFFFFF) | ((ImU32)(32 / gi) << 24), tab_r);
        tab_dl->AddRectFilled(ia, ib, line, tab_r);
    }

    if (it_anim->second.Frame.w > 0.01f)
        tab_dl->AddRectFilled(ImVec2(pos.x + 1.0f, pos.y + 1.0f), ImVec2(pos.x + size.x - 1.0f, pos.y + 2.0f),
                              IM_COL32(255, 255, 255, 14), tab_r);

    tab_dl->AddText(pos + ImVec2(style.FramePadding.x + style.FrameBorderSize * 2, size.y / 2 - label_size.y / 2 + 1), GetColorU32(it_anim->second.Label), label);

    return pressed;
}

bool ImAdd::RadioFrameText(const char* label, int* v, int radio_id, bool underline, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);

    const ImRect bb(pos, pos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held, active;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    active = *v == radio_id;
    if (pressed) {
        *v = radio_id;
    }

    ImVec4 colLabel = GetStyleColorVec4(active ? ImGuiCol_SliderGrab : (hovered && !held) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    ImVec4 colUnderlineMain = GetStyleColorVec4(ImGuiCol_SliderGrab);
    ImVec4 colUnderlineNull = colUnderlineMain; colUnderlineNull.w = 0.0f;
    ImVec4 colUnderline = (active ? colUnderlineMain : colUnderlineNull);

    struct stColors_State {
        ImVec4 Label;
        ImVec4 Underline;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Label = colLabel;
        it_anim->second.Underline = colUnderline;
    }

    it_anim->second.Label = ImLerp(it_anim->second.Label, colLabel, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);
    it_anim->second.Underline = ImLerp(it_anim->second.Underline, colUnderline, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);

    RenderNavHighlight(bb, id);

    ImDrawList* rdl = window->DrawList;
    const ImU32 accent = ImGui::GetColorU32(ImGuiCol_SliderGrab);

    float& lit_s = slot(id, 51, active ? 1.0f : 0.0f);
    const float lit = (lit_s = approach(lit_s, (hovered || active) ? 1.0f : 0.0f, 0.05f));
    if (lit > 0.01f)
        rdl->AddRectFilled(ImVec2(bb.Min.x - 4.0f, bb.Min.y + 0.5f), ImVec2(bb.Max.x + 4.0f, bb.Max.y + 0.5f),
                           with_a(accent, 0.10f * lit), 3.0f);

    window->DrawList->AddText(pos + ImVec2(size.x / 2 - label_size.x / 2, size.y / 2 - label_size.y / 2 + 1), GetColorU32(it_anim->second.Label), label);
    if (underline)
    {
        const float uw = (label_size.x + 2.0f) * lit;
        const float cx = pos.x + size.x * 0.5f;
        const float uy = pos.y + label_size.y + style.FrameBorderSize * 2;
        rdl->AddRectFilled(ImVec2(cx - uw * 0.5f, uy), ImVec2(cx + uw * 0.5f, uy + ImMax(1.0f, style.FrameBorderSize)),
                           ImGui::GetColorU32(it_anim->second.Underline), 1.0f);
    }

    return pressed;
}

bool ImAdd::SelectableFrame(const char* str_id, int selected, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(str_id);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, g.FontSize, g.FontSize);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    ImVec4 colFrameTop = GetStyleColorVec4(selected ? ImGuiCol_SliderGrab : ImGuiCol_Button);
    ImVec4 colFrameBottom = GetStyleColorVec4(selected ? ImGuiCol_SliderGrabActive : ImGuiCol_ButtonActive);

    struct stColors_State {
        ImVec4 FrameTop;
        ImVec4 FrameBottom;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.FrameTop = colFrameTop;
        it_anim->second.FrameBottom = colFrameBottom;
    }

    it_anim->second.FrameTop = ImLerp(it_anim->second.FrameTop, colFrameTop, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);
    it_anim->second.FrameBottom = ImLerp(it_anim->second.FrameBottom, colFrameBottom, 1.0f / IMADD_ANIMATIONS_SPEED * GetIO().DeltaTime);

    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(pos, pos + size, ImGui::GetColorU32(it_anim->second.FrameTop), style.FrameRounding);
    if (style.FrameBorderSize > 0)
    {
        window->DrawList->AddRect(pos, pos + size, ImGui::GetColorU32(ImGuiCol_Border));
    }

    return pressed;
}

void ImAdd::BeginChild(const char* label, const ImVec2& size_arg)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImGui::BeginChild(label, size_arg, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    pDrawList->AddRectFilled(pos + ImVec2(0, (float)(int)(label_size.y / 2)), pos + size, ImGui::GetColorU32(ImGuiCol_ChildBg), style.ChildRounding);
    pDrawList->AddRect(pos + ImVec2(0, (float)(int)(label_size.y / 2)), pos + size, ImGui::GetColorU32(ImGuiCol_Border), style.ChildRounding);
    pDrawList->AddRectFilled(pos + ImVec2(style.WindowPadding.x - style.ChildBorderSize * 5, 0), pos + ImVec2(style.WindowPadding.x + label_size.x + style.ChildBorderSize * 4, label_size.y), ImGui::GetColorU32(ImGuiCol_ChildBg));
    pDrawList->AddText(pos + ImVec2(style.WindowPadding.x, 0), ImGui::GetColorU32(ImGuiCol_Text), label);

    ImGui::SetCursorScreenPos(pos + ImVec2(0, (float)(int)(label_size.y / 2)));
    ImGui::BeginChild(std::string(std::string(label) + "##Main").c_str(), ImVec2(0, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_NoBackground);
}

void ImAdd::EndChild()
{
    ImGui::EndChild();
    ImGui::EndChild();
}

bool ImAdd::Button(const char* label, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    const float lit   = anim(id, 11, (hovered || held) ? 1.0f : 0.0f, 0.0f, 0.05f);
    const float press = anim(id, 12, held ? 1.0f : 0.0f, 0.0f, 0.035f);
    const float ring  = pulse(id, 13, pressed, 2.6f);

    RenderNavHighlight(bb, id);

    ImDrawList* btn_dl = window->DrawList;
    const ImU32 accent = accent_u32();
    const float r      = style.FrameRounding;

    const ImU32 base = mix32(mix32(btn_u32(), accent, 0.20f), accent, 0.66f * lit);
    plate(btn_dl, pos, pos + size, lighten(base, 0.12f + 0.08f * lit), darken(base, 0.07f), r);

    halo(btn_dl, pos, pos + size, accent, r, 0.20f * lit, 3, 1.2f);
    if (ring > 0.01f)
    {
        const float e = ring * 5.0f;
        btn_dl->AddRect(ImVec2(pos.x - e, pos.y - e), ImVec2(pos.x + size.x + e, pos.y + size.y + e),
                        with_a(accent, 0.28f * ring), r + e, 0, 1.2f);
    }

    if (size.y > 3.0f && size.x > 4.0f)
    {
        const float si = r * 0.55f;
        btn_dl->AddRectFilledMultiColor(ImVec2(pos.x + si, pos.y + 1.0f),
                                        ImVec2(pos.x + size.x - si, pos.y + size.y * 0.46f),
                                        IM_COL32(255, 255, 255, (int)(10.0f + 12.0f * lit)), IM_COL32(255, 255, 255, (int)(10.0f + 12.0f * lit)),
                                        IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
    }

    if (size.x > 6.0f && size.y > 4.0f)
        btn_dl->AddRectFilledMultiColor(ImVec2(pos.x + r * 0.5f, bb.Max.y - 1.0f), ImVec2(bb.Max.x - r * 0.5f, bb.Max.y),
                                        IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
                                        IM_COL32(0, 0, 0, (int)(40.0f + 30.0f * lit)), IM_COL32(0, 0, 0, (int)(40.0f + 30.0f * lit)));

    if (hovered && ImAdd::animations)
    {
        float& sw = slot(id, 14, -0.45f);
        sw += dt_clamped() * 1.7f;
        if (sw > 1.45f)
            sw = -0.45f;
        if (sw > 0.0f && sw < 1.0f && size.x > 8.0f)
        {
            const float band = ImMax(14.0f, size.x * 0.22f);
            const float cx   = pos.x - band + sw * (size.x + band * 2.0f);
            const int   wa   = (int)(10.0f + 14.0f * lit);
            btn_dl->PushClipRect(pos, pos + size, true);
            btn_dl->AddRectFilledMultiColor(ImVec2(cx, pos.y), ImVec2(cx + band, pos.y + size.y),
                                            IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, wa),
                                            IM_COL32(255, 255, 255, wa), IM_COL32(255, 255, 255, 0));
            btn_dl->PopClipRect();
        }
    }
    else
    {
        slot(id, 14, -0.45f) = -0.45f;
    }

    btn_dl->AddRect(pos, pos + size, mix32(border_u32(), accent, ImMax(lit * 0.85f, press)), r, 0, ImMax(1.0f, style.FrameBorderSize));

    PushStyleColor(ImGuiCol_Text, mix32(text_u32(), ImGui::GetColorU32(ImGuiCol_CheckMark), ImSaturate((lit - 0.35f) / 0.5f)));
    RenderTextClipped(ImVec2(bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y + press * 1.0f),
                      ImVec2(bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y + press),
                      label, NULL, &label_size, style.ButtonTextAlign, &bb);
    PopStyleColor();

    return pressed;
}

bool ImAdd::CheckBox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float height  = g.FontSize;
    const float track_w = ImMax(height * 1.78f, height + 10.0f);
    const float track_h = height * 0.94f;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImVec2(label_size.x > 0 ? label_size.x + style.ItemInnerSpacing.x + track_w : track_w, height);

    const ImRect bb(pos, pos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *v = !*v;

    const float t    = anim(id, 1, *v ? 1.0f : 0.0f, *v ? 1.0f : 0.0f, 0.055f);
    const float lit  = anim(id, 2, (hovered || held) ? 1.0f : 0.0f, 0.0f, 0.06f);
    const float dip  = pulse(id, 3, pressed, 7.0f);
    const float ring = pulse(id, 4, pressed, 2.4f);

    ImDrawList* cb_dl  = window->DrawList;
    const ImU32 accent = accent_u32();
    float label_x = pos.x + track_w + style.ItemInnerSpacing.x;

    if (ImAdd::use_switches)
    {

        const float dy = dip * 1.2f;
        const ImVec2 tr_a(bb.Min.x, bb.Min.y + (height - track_h) * 0.5f + dy);
        const ImVec2 tr_b(tr_a.x + track_w, tr_a.y + track_h);
        const float  tr_r = track_h * 0.5f;

        if (ring > 0.01f)
        {
            const float e = ring * 4.5f;
            cb_dl->AddRect(ImVec2(tr_a.x - e, tr_a.y - e), ImVec2(tr_b.x + e, tr_b.y + e),
                           with_a(accent, 0.26f * ring), tr_r + e, 0, 1.2f);
        }

        const float hover_t = ImMax(t, 0.12f * lit);

        const ImU32 off_top = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
        const ImU32 off_bot = ImGui::GetColorU32(ImGuiCol_FrameBg);
        plate(cb_dl, tr_a, tr_b, mix32(off_top, lighten(accent, 0.06f), hover_t), mix32(off_bot, accent, hover_t), tr_r);

        if (t < 0.99f && tr_b.y - tr_a.y > 3.0f)
            cb_dl->AddRectFilledMultiColor(ImVec2(tr_a.x + tr_r * 0.6f, tr_a.y + 1.0f), ImVec2(tr_b.x - tr_r * 0.6f, tr_a.y + 2.4f),
                                           IM_COL32(0, 0, 0, (int)(70.0f * (1.0f - t))), IM_COL32(0, 0, 0, (int)(70.0f * (1.0f - t))),
                                           IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

        cb_dl->AddRect(tr_a, tr_b, mix32(border_u32(), accent, ImMax(t, lit * 0.7f)), tr_r, 0, 1.0f);

        const float kr    = tr_r - ImMax(1.5f, track_h * 0.17f);
        const float slide = (track_w - track_h) * t;
        const ImVec2 kc(tr_a.x + tr_r + slide, (tr_a.y + tr_b.y) * 0.5f);

        for (int i = 4; i >= 1; --i)
            cb_dl->AddCircleFilled(ImVec2(kc.x, kc.y + 1.3f), kr + (float)i * 0.75f,
                                   IM_COL32(0, 0, 0, 30 - i * 6), 24);

        const ImU32 knob_col = mix32(mix32(dim_u32(), text_u32(), 0.40f), lighten(accent, 0.88f), t);
        cb_dl->AddCircleFilled(kc, kr, knob_col, 32);
        cb_dl->AddCircleFilled(ImVec2(kc.x, kc.y - kr * 0.24f), kr * 0.60f,
                               IM_COL32(255, 255, 255, (int)(20.0f + 26.0f * t)), 24);

    }
    else
    {

        const float inset = dip * height * 0.09f;
        const float drop  = dip * height * 0.04f;
        const ImVec2 a(pos.x + inset, pos.y + inset + drop);
        const ImVec2 b(pos.x + height - inset, pos.y + height - inset + drop);
        const float cb_r = ImMin(height * 0.30f, 4.5f);
        label_x = pos.x + height + style.ItemInnerSpacing.x;

        if (ring > 0.01f)
        {
            const float e = ring * height * 0.85f;
            cb_dl->AddRect(ImVec2(a.x - e, a.y - e), ImVec2(b.x + e, b.y + e), with_a(accent, 0.30f * ring), cb_r + e, 0, 1.2f);
        }

        const ImU32 off_top = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
        const ImU32 off_bot = ImGui::GetColorU32(ImGuiCol_FrameBg);
        const ImU32 top = mix32(hovered ? mix32(off_top, accent, 0.16f) : off_top, lighten(accent, 0.30f), t);
        const ImU32 bot = mix32(hovered ? mix32(off_bot, accent, 0.10f) : off_bot, accent, t);
        plate(cb_dl, a, b, top, bot, cb_r);

        if (t < 0.99f && b.y - a.y > 3.0f)
            cb_dl->AddRectFilledMultiColor(ImVec2(a.x + cb_r * 0.55f, a.y + 1.0f), ImVec2(b.x - cb_r * 0.55f, a.y + 2.6f),
                                           IM_COL32(0, 0, 0, (int)(80.0f * (1.0f - t))), IM_COL32(0, 0, 0, (int)(80.0f * (1.0f - t))),
                                           IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

        cb_dl->AddRect(a, b, mix32(border_u32(), accent, ImMax(t, lit * 0.75f)), cb_r, 0, 1.0f);

        if (t > 0.02f)
        {
            const float  msz = ImFloor(height * 0.62f);
            const ImVec2 mpos(ImFloor(a.x + (height - msz) * 0.5f), ImFloor(a.y + (height - msz) * 0.5f));
            RenderCheckMark(cb_dl, mpos, with_a(ImGui::GetColorU32(ImGuiCol_CheckMark), ImMin(1.0f, t * 1.4f)), msz);
        }
    }

    if (label_size.x > 0)
    {

        ImGui::PushStyleColor(ImGuiCol_Text, mix32(dim_u32(), text_u32(), ImMax(t, lit * 0.6f)));
        RenderText(ImVec2(label_x, pos.y + (height - label_size.y) * 0.5f), label);
        ImGui::PopStyleColor();
    }

    return pressed;
}

bool ImAdd::ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(desc_id);
    const float default_size = GetFrameHeight();
    const ImVec2 size(size_arg.x == 0.0f ? default_size : size_arg.x, size_arg.y == 0.0f ? default_size : size_arg.y);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    if (flags & ImGuiColorEditFlags_NoAlpha)
        flags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

    ImVec4 col_rgb = col;
    if (flags & ImGuiColorEditFlags_InputHSV)
        ColorConvertHSVtoRGB(col_rgb.x, col_rgb.y, col_rgb.z, col_rgb.x, col_rgb.y, col_rgb.z);

    ImVec4 col_rgb_without_alpha(col_rgb.x, col_rgb.y, col_rgb.z, 1.0f);
    float grid_step = ImMin(size.x, size.y) / 2.99f;
    float rounding = ImMin(g.Style.FrameRounding, grid_step * 0.5f);
    ImRect bb_inner = bb;
    float off = 0.0f;
    if ((flags & ImGuiColorEditFlags_NoBorder) == 0)
    {
        off = -0.75f;
        bb_inner.Expand(off);
    }
    if ((flags & ImGuiColorEditFlags_AlphaPreviewHalf) && col_rgb.w < 1.0f)
    {
        float mid_x = IM_ROUND((bb_inner.Min.x + bb_inner.Max.x) * 0.5f);
        RenderColorRectWithAlphaCheckerboard(window->DrawList, ImVec2(bb_inner.Min.x + grid_step, bb_inner.Min.y), bb_inner.Max, GetColorU32(col_rgb), grid_step, ImVec2(-grid_step + off, off), rounding, ImDrawFlags_RoundCornersRight);
        window->DrawList->AddRectFilled(bb_inner.Min, ImVec2(mid_x, bb_inner.Max.y), GetColorU32(col_rgb_without_alpha), rounding, ImDrawFlags_RoundCornersLeft);
    }
    else
    {

        ImVec4 col_source = (flags & ImGuiColorEditFlags_AlphaPreview) ? col_rgb : col_rgb_without_alpha;
        if (col_source.w < 1.0f)
            RenderColorRectWithAlphaCheckerboard(window->DrawList, bb_inner.Min, bb_inner.Max, GetColorU32(col_source), grid_step, ImVec2(off, off), rounding);
        else
            window->DrawList->AddRectFilled(bb_inner.Min, bb_inner.Max, GetColorU32(col_source), rounding);
    }
    RenderNavHighlight(bb, id);

    {
        float& lit_s = slot(id, 71, 0.0f);
        const float lit = (lit_s = approach(lit_s, hovered ? 1.0f : 0.0f, 0.05f));
        halo(window->DrawList, bb.Min, bb.Max, GetColorU32(ImGuiCol_SliderGrab), rounding, 0.34f * lit, 3, 1.1f);
    }

    if ((flags & ImGuiColorEditFlags_NoBorder) == 0)
    {
        if (g.Style.FrameBorderSize > 0.0f)
        {
            RenderFrameBorder(bb.Min, bb.Max, rounding);
        }
        else
        {
            window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), rounding);
        }
    }

    if (g.ActiveId == id && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropSource())
    {
        if (flags & ImGuiColorEditFlags_NoAlpha)
            SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F, &col_rgb, sizeof(float) * 3, ImGuiCond_Once);
        else
            SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F, &col_rgb, sizeof(float) * 4, ImGuiCond_Once);
        ColorButton(desc_id, col, flags);
        SameLine();
        TextEx("Color");
        EndDragDropSource();
    }

    if (!(flags & ImGuiColorEditFlags_NoTooltip) && hovered && IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        ColorTooltip(desc_id, &col.x, flags & (ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf));

    return pressed;
}

bool ImAdd::ColorEdit4(const char* label, float col[4])
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec4 col_v4(col[0], col[1], col[2], col[3]);

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview;

    PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    bool result = ImAdd::ColorButton(label, col_v4, flags, ImVec2(g.FontSize * 2, g.FontSize));
    PopStyleVar();
    if (result)
    {
        OpenPopup(std::string(std::string(label) + "##ColorEdit4").c_str());
    }
    if (BeginPopup(std::string(std::string(label) + "##ColorEdit4").c_str()))
    {
        ColorPicker4(label, col, flags);
        EndPopup();
    }

    return result;
}

bool ImAdd::SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    float w = CalcItemWidth();

    ImVec2 pos = window->DC.CursorPos;

    const float label_height = label_size.x > 0 ? g.FontSize + style.ItemInnerSpacing.y : 0.0f;
    const ImRect frame_bb(pos + ImVec2(0, label_height), pos + ImVec2(w, g.FontSize + label_height));
    const ImRect total_bb(pos, pos + ImVec2(w, g.FontSize + label_height));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;

    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
    const bool clicked = hovered && IsMouseClicked(0, id, 0);
    const bool make_active = (clicked || g.NavActivateId == id);

    if (make_active)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    ImVec4 colFrame = GetStyleColorVec4(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImVec4 colGrab = GetStyleColorVec4(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);

    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Grab;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Grab = colGrab;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / IMADD_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Grab = ImLerp(it_anim->second.Grab, colGrab, 1.0f / IMADD_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(it_anim->second.Frame), true, g.Style.FrameRounding);

    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, 0, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    const bool  active = (g.ActiveId == id);
    float& lit_s = slot(id, 21, 0.0f);
    const float lit = (lit_s = approach(lit_s, (hovered || active) ? 1.0f : 0.0f, 0.05f));

    float tgt = 0.0f;
    if (data_type == ImGuiDataType_Float && *(const float*)p_max != *(const float*)p_min)
        tgt = (*(const float*)p_data - *(const float*)p_min) / (*(const float*)p_max - *(const float*)p_min);
    else if (data_type == ImGuiDataType_S32)
    {
        const float lo = (float)*(const int*)p_min, hi = (float)*(const int*)p_max;
        tgt = (hi > lo) ? (((float)*(const int*)p_data - lo) / (hi - lo)) : 0.0f;
    }
    else if (grab_bb.Max.x > grab_bb.Min.x)
        tgt = (grab_bb.Min.x - frame_bb.Min.x) / ImMax(1.0f, frame_bb.GetWidth());
    tgt = ImClamp(tgt, 0.0f, 1.0f);
    float& t_s = slot(id, 22, tgt);
    const float t = (t_s = approach(t_s, tgt, active ? 0.03f : 0.06f));

    if (frame_bb.GetHeight() > 2.0f)
    {
        ImDrawList* sdl = window->DrawList;
        const ImU32 accent = GetColorU32(it_anim->second.Grab);
        const float r  = frame_bb.GetHeight() * 0.5f;
        const float fw = frame_bb.GetWidth();

        plate(sdl, frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBgActive), GetColorU32(ImGuiCol_FrameBg), r);
        sdl->AddRectFilledMultiColor(ImVec2(frame_bb.Min.x + r * 0.55f, frame_bb.Min.y + 1.0f),
                                     ImVec2(frame_bb.Max.x - r * 0.55f, frame_bb.Min.y + 2.4f),
                                     IM_COL32(0, 0, 0, 70), IM_COL32(0, 0, 0, 70), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

        for (int i = 1; i <= 3; ++i)
        {
            const float mx = frame_bb.Min.x + fw * (float)i * 0.25f;
            const bool  on = (t >= (float)i * 0.25f);
            sdl->AddRectFilled(ImVec2(mx - 0.5f, frame_bb.Min.y + 3.0f), ImVec2(mx + 0.5f, frame_bb.Max.y - 3.0f),
                               on ? IM_COL32(0, 0, 0, 90) : IM_COL32(255, 255, 255, 46), 0.5f);
        }

        const float fill_x = frame_bb.Min.x + fw * t;
        const float fill_w = fill_x - frame_bb.Min.x;
        if (fill_w > 1.0f)
        {
            const float fr = ImMin(r, fill_w * 0.5f + 0.5f);
            plate(sdl, frame_bb.Min, ImVec2(fill_x, frame_bb.Max.y), lighten(accent, 0.22f), accent, fr);
            if (fill_w > 3.0f)
                sdl->AddRectFilled(ImVec2(frame_bb.Min.x + 1.0f, frame_bb.Min.y + 1.0f), ImVec2(fill_x - 1.0f, frame_bb.Min.y + 2.0f),
                                   IM_COL32(255, 255, 255, 46), 1.0f);
            if (active)
                sdl->AddRectFilled(ImVec2(fill_x - 2.0f, frame_bb.Min.y), ImVec2(fill_x, frame_bb.Max.y), IM_COL32(255, 255, 255, 110), 1.0f);
        }

        float& ks = slot(id, 23, 1.0f);
        const float kr = frame_bb.GetHeight() * 0.46f * (ks = approach(ks, active ? 1.0f : hovered ? 0.86f : 0.66f, 0.05f)) + 1.4f;
        const ImVec2 kc(fill_x, (frame_bb.Min.y + frame_bb.Max.y) * 0.5f);
        sdl->AddCircleFilled(ImVec2(kc.x, kc.y + 1.0f), kr, IM_COL32(0, 0, 0, 70), 24);
        sdl->AddCircleFilled(kc, kr, accent, 24);
        sdl->AddCircleFilled(kc, kr * 0.44f, lighten(accent, 0.85f), 20);
        if (lit > 0.01f)
            halo(sdl, ImVec2(kc.x - kr, kc.y - kr), ImVec2(kc.x + kr, kc.y + kr), accent, kr, 0.30f * lit, 3, 1.2f);

        sdl->AddRect(frame_bb.Min, frame_bb.Max, mix32(border_u32(), accent, lit * 0.7f), r, 0, 1.0f);
    }

    float extra_width;
    if (label_size.x > 0) {
        extra_width = -ImGui::CalcTextSize(value_buf).x;
    }
    else {
        extra_width = style.ItemInnerSpacing.x;
    }

    window->DrawList->AddText(pos + ImVec2(w + extra_width, 0), GetColorU32(ImGuiCol_TextDisabled), value_buf);

    if (label_size.x > 0.0f)
        RenderText(pos, label);

    return value_changed;
}

bool ImAdd::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format)
{
    return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format);
}

bool ImAdd::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
{
    return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format);
}

bool ImAdd::Selectable(const char* label, bool selected, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    const float lit = anim(id, 31, (hovered || selected) ? 1.0f : 0.0f, selected ? 1.0f : 0.0f, 0.05f);
    const float sel = anim(id, 32, selected ? 1.0f : 0.0f, selected ? 1.0f : 0.0f, 0.07f);

    RenderNavHighlight(bb, id);

    ImDrawList* sdl = window->DrawList;
    const ImU32 accent = accent_u32();
    const float r = style.FrameRounding;

    if (lit > 0.01f)
        sdl->AddRectFilled(ImVec2(bb.Min.x + 2.0f, bb.Min.y + 1.0f), ImVec2(bb.Max.x - 2.0f, bb.Max.y - 1.0f),
                           mix32(ImGui::GetColorU32(ImGuiCol_HeaderHovered), accent, 0.18f * lit),
                           ImMax(1.0f, r * 0.6f));
    if (sel > 0.01f)
        sdl->AddRectFilled(ImVec2(bb.Min.x + 2.0f, bb.Min.y + 1.0f), ImVec2(bb.Max.x - 2.0f, bb.Max.y - 1.0f),
                           with_a(accent, 0.13f * sel), ImMax(1.0f, r * 0.6f));

    if (sel > 0.01f)
    {
        const float bh = (bb.GetHeight() - 6.0f) * sel;
        const float cy = (bb.Min.y + bb.Max.y) * 0.5f;
        sdl->AddRectFilled(ImVec2(bb.Min.x + 1.0f, cy - bh * 0.5f), ImVec2(bb.Min.x + 2.5f, cy + bh * 0.5f), accent, 0.75f);
    }

    const ImU32 tc = mix32(mix32(dim_u32(), text_u32(), lit * 0.85f), accent, sel * 0.5f);
    PushStyleColor(ImGuiCol_Text, tc);
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
    PopStyleColor();

    return pressed;
}

bool ImAdd::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview));
    if (flags & ImGuiComboFlags_WidthFitPreview)
        IM_ASSERT((flags & (ImGuiComboFlags_NoPreview | ImGuiComboFlags_CustomPreview)) == 0);

    const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float preview_width = ((flags & ImGuiComboFlags_WidthFitPreview) && (preview_value != NULL)) ? CalcTextSize(preview_value, NULL, true).x : 0.0f;
    const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : ((flags & ImGuiComboFlags_WidthFitPreview) ? (arrow_size + preview_width + style.FramePadding.x * 2.0f) : CalcItemWidth());

    const ImRect bb(window->DC.CursorPos + ImVec2(0.0f, label_size.x > 0 ? label_size.y + style.ItemInnerSpacing.y : 0.0f), window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f) + ImVec2(0.0f, label_size.x > 0 ? label_size.y + style.ItemInnerSpacing.y : 0.0f));
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f) + ImVec2(0.0f, label_size.x > 0 ? label_size.y + style.ItemInnerSpacing.y : 0.0f));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &bb))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    float& lit_s = slot(id, 60, 0.0f);
    const float lit = (lit_s = approach(lit_s, (hovered || popup_open) ? 1.0f : 0.0f, 0.06f));

    const ImVec4 colFrame = ImLerp(GetStyleColorVec4(ImGuiCol_FrameBg),
                                   GetStyleColorVec4(ImGuiCol_FrameBgHovered), lit);
    const ImVec4 colText  = ImLerp(GetStyleColorVec4(ImGuiCol_TextDisabled),
                                   GetStyleColorVec4(ImGuiCol_Text), lit);

    const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    RenderNavHighlight(bb, id);
    if (!(flags & ImGuiComboFlags_NoPreview)) {

        plate(window->DrawList, bb.Min, bb.Max,
              lighten(GetColorU32(colFrame), 0.05f), GetColorU32(colFrame), style.FrameRounding);
    }
    if (!(flags & ImGuiComboFlags_NoArrowButton))
    {
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
        {

            float& rot_s = slot(id, 61, popup_open ? 1.0f : 0.0f);
            const float rot = (rot_s = approach(rot_s, popup_open ? 1.0f : 0.0f, 0.07f));
            const float ang = rot * IM_PI;
            const ImVec2 cc(value_x2 + arrow_size * 0.5f, (bb.Min.y + bb.Max.y) * 0.5f);
            const float  cs = ImMin(arrow_size, bb.GetHeight()) * 0.17f;
            const float  ca = cosf(ang), sa = sinf(ang);
            const ImVec2 q0(-cs, cs * 0.55f), q1(0.0f, -cs * 0.5f), q2(cs, cs * 0.55f);
            const ImU32 ccol = GetColorU32((popup_open || hovered) ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            const ImVec2 r0(cc.x + q0.x * ca - q0.y * sa, cc.y + q0.x * sa + q0.y * ca);
            const ImVec2 r1(cc.x + q1.x * ca - q1.y * sa, cc.y + q1.x * sa + q1.y * ca);
            const ImVec2 r2(cc.x + q2.x * ca - q2.y * sa, cc.y + q2.x * sa + q2.y * ca);
            window->DrawList->AddLine(r0, r1, ccol, 1.6f);
            window->DrawList->AddLine(r1, r2, ccol, 1.6f);
        }
    }

    window->DrawList->AddRect(bb.Min, bb.Max,
        mix32(GetColorU32(ImGuiCol_Border), accent_u32(), lit * 0.55f),
        style.FrameRounding, 0, 1.0f);

    if (popup_open)
    {
        const ImU32 cacc = GetColorU32(ImGuiCol_SliderGrab);
        halo(window->DrawList, bb.Min, bb.Max, cacc, style.FrameRounding, 0.22f, 3, 1.1f);
        window->DrawList->AddRect(bb.Min, bb.Max, with_a(cacc, 0.85f), style.FrameRounding, 0, 1.2f);
    }

    if (flags & ImGuiComboFlags_CustomPreview)
    {
        g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
        preview_value = NULL;
    }

    if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
    {
        if (g.LogEnabled)
            LogSetNextTextDecoration("{", "}");
        PushStyleColor(ImGuiCol_Text, colText);
        RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
        PopStyleColor();
    }
    if (label_size.x > 0)
        RenderText(total_bb.Min, label);

    if (!popup_open)
        return false;

    g.NextWindowData.Flags = backup_next_window_data_flags;
    return BeginComboPopup(popup_id, bb, flags);
}

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0)
        return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool ImAdd::BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(popup_id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();
        return false;
    }

    float w = bb.GetWidth();
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
    {
        g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
    }
    else
    {
        if ((flags & ImGuiComboFlags_HeightMask_) == 0)
            flags |= ImGuiComboFlags_HeightRegular;
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));
        int popup_max_height_in_items = -1;
        if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
        else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
        else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
        ImVec2 constraint_min(0.0f, 0.0f), constraint_max(FLT_MAX, FLT_MAX);
        if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.x <= 0.0f)
            constraint_min.x = w;
        if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.y <= 0.0f)
            constraint_max.y = CalcMaxPopupHeightFromItemCount(popup_max_height_in_items);
        SetNextWindowSizeConstraints(constraint_min, constraint_max);
    }

    char name[16];
    ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size);

    if (ImGuiWindow* popup_window = FindWindowByName(name))
        if (popup_window->WasActive)
        {

            ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
            popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down;
            ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
            ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
            SetNextWindowPos(pos + ImVec2(0, g.Style.ItemSpacing.y));
        }

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(g.Style.FramePadding.x + 4.0f, g.Style.FramePadding.y + 4.0f));
    PushStyleVar(ImGuiStyleVar_PopupRounding, ImMax(g.Style.FrameRounding + 2.0f, g.Style.PopupRounding));
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    bool ret = Begin(name, NULL, window_flags);
    PopStyleVar(3);
    if (!ret)
    {
        EndPopup();
        IM_ASSERT(0);
        return false;
    }
    return true;
}

static bool ComboRow(const char* label, bool selected)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float  h = g.FontSize + style.FramePadding.y * 1.8f;
    const float  w = ImMax(1.0f, GetContentRegionAvail().x);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));

    ItemSize(ImVec2(w, h), style.FramePadding.y * 0.5f);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    float& hov_s = slot(id, 71, 0.0f);
    const float hov = (hov_s = approach(hov_s, hovered ? 1.0f : 0.0f, 0.05f));

    ImDrawList* dl = window->DrawList;
    const ImU32 accent   = accent_u32();
    const float rounding = ImMax(3.0f, style.FrameRounding + 1.0f);

    if (selected)
        dl->AddRectFilled(bb.Min, bb.Max, with_a(accent, 0.16f), rounding);
    if (hov > 0.004f)
        dl->AddRectFilled(bb.Min, bb.Max, with_a(accent, 0.13f * hov), rounding);
    if (selected || hov > 0.004f)
        dl->AddRect(bb.Min, bb.Max, with_a(accent, selected ? 0.34f : 0.20f * hov),
            rounding, 0, 1.0f);

    const float cy  = (bb.Min.y + bb.Max.y) * 0.5f;
    const float pad = style.FramePadding.x + 2.0f;

    if (selected)
    {
        const float dot = ImMax(2.5f, g.FontSize * 0.17f);
        dl->AddCircleFilled(ImVec2(bb.Max.x - pad - dot, cy), dot, accent, 16);
    }

    PushStyleColor(ImGuiCol_Text,
        selected ? accent : mix32(dim_u32(), text_u32(), hov));
    const ImVec2 text_min(bb.Min.x + pad, bb.Min.y);
    const ImVec2 text_max(bb.Max.x - pad * 2.5f, bb.Max.y);
    RenderTextClipped(text_min, text_max, label, NULL, NULL);
    PopStyleColor();

    return pressed;
}

bool ImAdd::Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
{
    ImGuiContext& g = *GImGui;

    const char* preview_value = NULL;
    if (*current_item >= 0 && *current_item < items_count)
        preview_value = getter(user_data, *current_item);

    if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
        SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

    if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
        return false;

    bool value_changed = false;
    for (int i = 0; i < items_count; i++)
    {
        const char* item_text = getter(user_data, i);
        if (item_text == NULL)
            item_text = "*Unknown item*";

        PushID(i);
        const bool item_selected = (i == *current_item);
        if (ComboRow(item_text, item_selected))
        {
            if (*current_item != i)
            {
                value_changed = true;
                *current_item = i;
            }

            CloseCurrentPopup();
        }
        if (item_selected)
            SetItemDefaultFocus();
        PopID();
    }

    EndCombo();

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

static const char* Items_ArrayGetter(void* data, int idx)
{
    const char* const* items = (const char* const*)data;
    return items[idx];
}

static const char* Items_SingleStringGetter(void* data, int idx)
{
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += strlen(p) + 1;
        items_count++;
    }
    return *p ? p : NULL;
}

bool ImAdd::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
    const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
    return value_changed;
}

bool ImAdd::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, float width, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        p += strlen(p) + 1;
        items_count++;
    }

    const float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;
    PushItemWidth(w);
    bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
    PopItemWidth();

    return value_changed;
}

bool ImAdd::KeyBind(const char* str_id, int* k, float custom_width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(str_id);

    char buf_display[32] = "None";
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 buf_display_size = ImGui::CalcTextSize(buf_display, NULL, true);
    float width = custom_width == 0 ? ImGui::CalcItemSize(ImVec2(-0.1f, 0), 0, 0).x : custom_width;
    float height = ImGui::GetFontSize();

    ImVec2 size = ImVec2(width, height);
    ImRect frame_bb(pos, pos + size);
    ImRect total_bb(pos, frame_bb.Max + ImVec2(size.x, 0));

    ImGui::ItemSize(total_bb);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    const bool hovered = ImGui::ItemHoverable(frame_bb, id, 0);

    if (hovered)
    {
        ImGui::SetHoveredID(id);
        g.MouseCursor = ImGuiMouseCursor_Hand;
    }

    const bool user_clicked = hovered && io.MouseClicked[0];

    if (user_clicked)
    {
        if (g.ActiveId != id)
        {
            memset(io.MouseDown, 0, sizeof(io.MouseDown));
            memset(io.KeysDown, 0, sizeof(io.KeysDown));
            *k = 0;
        }
        ImGui::SetActiveID(id, window);
        ImGui::FocusWindow(window);
    }
    else if (io.MouseClicked[0])
    {
        if (g.ActiveId == id)
            ImGui::ClearActiveID();
    }

    bool value_changed = false;
    int key = *k;

    if (g.ActiveId == id)
    {
        if (!value_changed)
        {
            for (auto i = 0x08; i <= 0xA5; i++)
            {
                if (io.KeysDown[i])
                {
                    key = i;
                    value_changed = true;
                    ImGui::ClearActiveID();
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            *k = 0;
            ImGui::ClearActiveID();
        }
        else *k = key;
    }

    ImGui::RenderNavHighlight(total_bb, id);

    const bool  listening = (g.ActiveId == id);
    float& lit_s   = slot(id, 41, 0.0f);
    float& bound_s = slot(id, 42, (*k != 0) ? 1.0f : 0.0f);
    const float lit   = (lit_s   = approach(lit_s,   (hovered || listening) ? 1.0f : 0.0f, 0.05f));
    const float bound = (bound_s = approach(bound_s, (*k != 0) ? 1.0f : 0.0f, 0.07f));

    if (*k != 0 && !listening)
        strcpy_s(buf_display, sizeof buf_display, szKeyNames[*k]);
    else if (listening)
        strcpy_s(buf_display, sizeof buf_display, "...");

    ImDrawList* kdl = window->DrawList;
    const ImU32 accent = accent_u32();
    const float kr = style.FrameRounding;

    plate(kdl, frame_bb.Min, frame_bb.Max, lighten(frame_hi_u32(), 0.08f + 0.06f * lit), frame_u32(), kr);
    if (*k == 0 && !listening && lit < 0.5f)
        kdl->AddRectFilled(frame_bb.Min, frame_bb.Max, IM_COL32(0, 0, 0, 40), kr);

    if (listening)
    {
        const float p = 0.55f + 0.45f * sinf((float)ImGui::GetTime() * 7.0f);
        halo(kdl, frame_bb.Min, frame_bb.Max, accent, kr, 0.45f * p, 3, 1.3f);
        kdl->AddRect(frame_bb.Min, frame_bb.Max, with_a(accent, p), kr, 0, 1.2f);
        kdl->AddRectFilled(ImVec2(frame_bb.Min.x + 1.0f, frame_bb.Min.y + 1.0f), ImVec2(frame_bb.Max.x - 1.0f, frame_bb.Max.y - 1.0f),
                           with_a(accent, 0.10f), ImMax(0.0f, kr - 1.0f));
    }
    else
    {
        halo(kdl, frame_bb.Min, frame_bb.Max, accent, kr, 0.24f * lit, 2, 1.1f);
        kdl->AddRect(frame_bb.Min, frame_bb.Max, mix32(border_u32(), accent, ImMax(bound * 0.55f, lit * 0.7f)), kr, 0, 1.0f);
    }

    if (bound > 0.01f && frame_bb.GetHeight() > 6.0f)
    {
        const float bh = (frame_bb.GetHeight() - 6.0f) * bound;
        const float cy = (frame_bb.Min.y + frame_bb.Max.y) * 0.5f;
        kdl->AddRectFilled(ImVec2(frame_bb.Min.x + 1.5f, cy - bh * 0.5f), ImVec2(frame_bb.Min.x + 3.0f, cy + bh * 0.5f),
                           with_a(accent, bound), 0.75f);
    }

    const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y);
    ImGui::PushStyleColor(ImGuiCol_Text, listening ? with_a(accent, 1.0f) : mix32(dim_u32(), text_u32(), ImMax(bound, lit * 0.8f)));
    ImGui::RenderTextClipped(frame_bb.Min, frame_bb.Max, buf_display, NULL, NULL, style.ButtonTextAlign, &clip_rect);
    ImGui::PopStyleColor();

    return value_changed;
}

void ImAdd::KeyBindPopup(const char* str_id, int* k, int* mode)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    ImAdd::KeyBind(str_id, k, ImGui::CalcTextSize(szKeyNames[*k]).x);

    static const char* popup_name = std::string(str_id + std::string("##popup")).c_str();

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right, false))
    {
        ImGui::OpenPopup(popup_name);
    }
    if (ImGui::BeginPopup(popup_name))
    {
        ImGui::BeginChild("PopupRect", ImVec2(ImGui::CalcTextSize("togglehold").x + 1 + style.ItemSpacing.x * 2, ImGui::GetFontSize()), ImGuiActivateFlags_None, ImGuiWindowFlags_NoBackground);
        {
            ImAdd::RadioFrameText("toggle", mode, 0, false);
            ImGui::SameLine(); ImAdd::VSeparator(style.WindowPadding.y, 1.0f); ImGui::SameLine();
            ImAdd::RadioFrameText("hold", mode, 1, false);
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}