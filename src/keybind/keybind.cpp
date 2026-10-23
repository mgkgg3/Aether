#include "keybind.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

bool keybind::set_key()
{
    if (ImGui::IsKeyChordPressed(ImGuiKey_Escape))
    {
        key = 0;
        ImGui::ClearActiveID();
        return true;
    }

    for (int i = 1; i < 5; i++)
    {
        if (ImGui::GetIO().MouseDown[i])
        {
            switch (i)
            {
            case 1: key = VK_RBUTTON; break;
            case 2: key = VK_MBUTTON; break;
            case 3: key = VK_XBUTTON1; break;
            case 4: key = VK_XBUTTON2; break;
            }
            return true;
        }
    }

    for (int i = VK_BACK; i <= VK_RMENU; i++)
    {
        if (i == VK_LBUTTON) continue;

        if (GetAsyncKeyState(i) & 0x8000)
        {
            key = i;
            return true;
        }
    }

    std::vector<int> additional_keys = {
        VK_LWIN, VK_RWIN, VK_APPS, VK_SLEEP, VK_NUMPAD0, VK_NUMPAD1,
        VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6,
        VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_MULTIPLY, VK_ADD,
        VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE, VK_F1, VK_F2,
        VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
        VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18,
        VK_F19, VK_F20, VK_F21, VK_F22, VK_F23, VK_F24
    };

    for (int vk : additional_keys)
    {
        if (GetAsyncKeyState(vk) & 0x8000)
        {
            key = vk;
            return true;
        }
    }

    return false;
}

#include <cstdio>
#include <map>

struct kb_key_entry_t { const char* name; int value; };
static const kb_key_entry_t s_kb_keys[] = {
    {"None",0},{"LM",VK_LBUTTON},{"RM",VK_RBUTTON},{"MM",VK_MBUTTON},
    {"MB4",VK_XBUTTON1},{"MB5",VK_XBUTTON2},{"Backspace",VK_BACK},
    {"Tab",VK_TAB},{"Enter",VK_RETURN},{"Shift",VK_SHIFT},{"Ctrl",VK_CONTROL},
    {"Alt",VK_MENU},{"Pause",VK_PAUSE},{"CapsLock",VK_CAPITAL},
    {"Esc",VK_ESCAPE},{"Space",VK_SPACE},{"PgUp",VK_PRIOR},{"PgDn",VK_NEXT},
    {"End",VK_END},{"Home",VK_HOME},{"Left",VK_LEFT},{"Up",VK_UP},
    {"Right",VK_RIGHT},{"Down",VK_DOWN},{"Insert",VK_INSERT},{"Del",VK_DELETE},
    {"0",'0'},{"1",'1'},{"2",'2'},{"3",'3'},{"4",'4'},{"5",'5'},{"6",'6'},
    {"7",'7'},{"8",'8'},{"9",'9'},
    {"A",'A'},{"B",'B'},{"C",'C'},{"D",'D'},{"E",'E'},{"F",'F'},{"G",'G'},
    {"H",'H'},{"I",'I'},{"J",'J'},{"K",'K'},{"L",'L'},{"M",'M'},{"N",'N'},
    {"O",'O'},{"P",'P'},{"Q",'Q'},{"R",'R'},{"S",'S'},{"T",'T'},{"U",'U'},
    {"V",'V'},{"W",'W'},{"X",'X'},{"Y",'Y'},{"Z",'Z'},
    {"Num0",VK_NUMPAD0},{"Num1",VK_NUMPAD1},{"Num2",VK_NUMPAD2},
    {"Num3",VK_NUMPAD3},{"Num4",VK_NUMPAD4},{"Num5",VK_NUMPAD5},
    {"Num6",VK_NUMPAD6},{"Num7",VK_NUMPAD7},{"Num8",VK_NUMPAD8},
    {"Num9",VK_NUMPAD9},
    {"F1",VK_F1},{"F2",VK_F2},{"F3",VK_F3},{"F4",VK_F4},{"F5",VK_F5},
    {"F6",VK_F6},{"F7",VK_F7},{"F8",VK_F8},{"F9",VK_F9},{"F10",VK_F10},
    {"F11",VK_F11},{"F12",VK_F12},
    {"NumLock",VK_NUMLOCK},{"ScrollLock",VK_SCROLL},
};
static const int s_kb_key_count = (int)(sizeof(s_kb_keys)/sizeof(s_kb_keys[0]));

const char* keybind_get_key_name(int key)
{
    if (key == 0) return "None";
    for (int i = 0; i < s_kb_key_count; i++)
        if (s_kb_keys[i].value == key) return s_kb_keys[i].name;
    
    if (key >= VK_F1 && key <= VK_F12) {
        static char fb[5]; snprintf(fb, sizeof fb, "F%d", key - VK_F1 + 1);
        return fb;
    }
    return "???";
}

bool keybind_selector(const char* label, int* key, int* activation_mode, float width)
{
    using namespace ImGui;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    bool is_active = (g.ActiveId == id);

    char buf[32];
    if (is_active)
        snprintf(buf, sizeof buf, "[...]");
    else if (*key == 0)
        snprintf(buf, sizeof buf, "[None]");
    else
        snprintf(buf, sizeof buf, "[%s]", keybind_get_key_name(*key));

    ImVec2 label_size = CalcTextSize(buf);
    float w = (width > 0.f) ? width : label_size.x + style.FramePadding.x * 2.f;
    float h = g.FontSize + style.FramePadding.y * 2.f;
    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, h));
    ItemSize(bb);
    if (!ItemAdd(bb, id)) return false;

    bool hovered = ItemHoverable(bb, id, 0);
    bool value_changed = false;

    char popup_id[64]; snprintf(popup_id, sizeof popup_id, "##kb_ctx_%u", id);
    if (hovered && IsMouseClicked(ImGuiMouseButton_Right))
        OpenPopup(popup_id);

    SetNextWindowSize(ImVec2(80.f, 0), ImGuiCond_Always);
    if (BeginPopup(popup_id))
    {
        if (activation_mode)
        {
            if (ImGui::Selectable("Hold",   *activation_mode == 1)) { *activation_mode = 1; value_changed = true; CloseCurrentPopup(); }
            if (ImGui::Selectable("Toggle", *activation_mode == 0)) { *activation_mode = 0; value_changed = true; CloseCurrentPopup(); }
            if (ImGui::Selectable("Always", *activation_mode == 2)) { *activation_mode = 2; value_changed = true; CloseCurrentPopup(); }
            Separator();
        }
        if (ImGui::Selectable("Clear", false))
        {
            *key = 0; value_changed = true;
            if (g.ActiveId == id) ClearActiveID();
            CloseCurrentPopup();
        }
        EndPopup();
    }

    static std::map<ImGuiID, bool> s_just_activated;
    static std::map<ImGuiID, std::map<int,bool>> s_initial;

    if (hovered && IsMouseClicked(ImGuiMouseButton_Left))
    {
        SetActiveID(id, window);
        FocusWindow(window);
        s_just_activated[id] = true;
        s_initial[id].clear();
        for (int i = 0; i < s_kb_key_count; i++) {
            int vk = s_kb_keys[i].value;
            if (vk) s_initial[id][vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
        }
    }
    else if (IsMouseClicked(ImGuiMouseButton_Left) && !hovered && g.ActiveId == id)
    {
        ClearActiveID(); s_just_activated.erase(id); s_initial.erase(id);
    }

    if (is_active)
    {
        if (s_just_activated[id]) { s_just_activated[id] = false; }
        else
        {
            for (int i = 0; i < s_kb_key_count; i++) {
                int vk = s_kb_keys[i].value;
                if (!vk) continue;
                bool pressed = (GetAsyncKeyState(vk) & 0x8000) != 0;
                bool was = s_initial[id].count(vk) && s_initial[id][vk];
                if (pressed && !was) {
                    *key = vk; value_changed = true;
                    ClearActiveID(); s_just_activated.erase(id); s_initial.erase(id);
                    break;
                }
            }
        }
    }

    ImU32 bg = is_active  ? GetColorU32(ImGuiCol_FrameBgActive) :
               hovered    ? GetColorU32(ImGuiCol_FrameBgHovered) :
                            GetColorU32(ImGuiCol_FrameBg);
    window->DrawList->AddRectFilled(bb.Min, bb.Max, bg, style.FrameRounding);
    window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), style.FrameRounding);

    ImVec2 text_pos = bb.Min + ImVec2(
        (w - label_size.x) * 0.5f,
        (h - label_size.y) * 0.5f);
    window->DrawList->AddText(text_pos,
        is_active ? GetColorU32(ImGuiCol_Text) : GetColorU32(ImGuiCol_TextDisabled),
        buf);

    return value_changed;
}
