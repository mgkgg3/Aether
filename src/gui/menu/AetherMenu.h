#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <Windows.h>

#include <settings.h>

#include <gui/AetherFonts.h>
#include <gui/AetherIcons.h>

#include <cmath>
#include <cstdio>
#include <cstddef>
#include <cstring>

namespace nm
{

	inline constexpr float kOuterMargin     = 16.0f;
	inline constexpr float kPanelGap        = 14.0f;
	inline constexpr float kTopBarHeight    = 44.0f;
	inline constexpr float kLeftPanelWidth  = 58.0f;
	inline constexpr float kPanelRadius     = 22.0f;
	inline constexpr float kTopBarRadius    = 13.0f;
	inline constexpr float kCardRadius      = 12.0f;

	inline constexpr float kCardInset       = 0.0f;
	inline constexpr float kPageInset       = 24.0f;

	inline constexpr float kCardTopPad      = 26.0f;
	inline constexpr float kCardGap         = 20.0f;
	inline constexpr float kRowGap          = 10.0f;

	inline constexpr float kRowPad          = 5.0f;
	inline constexpr float kBrandPadding    = 22.0f;
	inline constexpr float kScrollFade      = 34.0f;
	inline constexpr float kToggleW         = 40.0f;
	inline constexpr float kToggleH         = 22.0f;
	inline constexpr float kFieldW          = 158.0f;
	inline constexpr float kFieldH          = 30.0f;
	inline constexpr float kBindW           = 150.0f;
	inline constexpr float kBindH           = 34.0f;
	inline constexpr float kSwatchD         = 30.0f;

	inline constexpr float kSidebarWidth    = 150.0f;
	inline constexpr float kSidebarItemH    = 38.0f;
	inline constexpr float kSidebarItemGap  = 5.0f;
	inline constexpr float kSidebarInset    = 6.0f;
	inline constexpr float kTopTabH         = 36.0f;

	inline constexpr float kSubTabH         = 27.0f;
	inline constexpr float kMenuWidth       = 930.0f;
	inline constexpr float kMenuHeight      = 600.0f;

	inline constexpr const char* kBrandName = "aether";

	struct Style
	{

		ImVec4 top_surface  = ImVec4(36 / 255.f, 36 / 255.f, 42 / 255.f, 1.f);
		ImVec4 rail_surface = ImVec4(36 / 255.f, 36 / 255.f, 42 / 255.f, 1.f);
		ImVec4 main_surface = ImVec4(36 / 255.f, 36 / 255.f, 42 / 255.f, 1.f);
		ImVec4 card_surface = ImVec4(44 / 255.f, 44 / 255.f, 51 / 255.f, 1.f);

		ImVec4 text         = ImVec4(222 / 255.f, 222 / 255.f, 224 / 255.f, 1.f);
		ImVec4 text_dim     = ImVec4(146 / 255.f, 146 / 255.f, 152 / 255.f, 1.f);
		ImVec4 text_faint   = ImVec4(118 / 255.f, 118 / 255.f, 124 / 255.f, 1.f);
		ImVec4 text_on_pill = ImVec4(24 / 255.f, 24 / 255.f, 27 / 255.f, 1.f);
		ImVec4 pill_active  = ImVec4(222 / 255.f, 222 / 255.f, 224 / 255.f, 1.f);

		ImVec4 accent       = ImVec4(206 / 255.f, 206 / 255.f, 212 / 255.f, 1.f);

		ImFont* body        = nullptr;
		ImFont* body_medium = nullptr;
		ImFont* heading     = nullptr;
		ImFont* caption     = nullptr;

		bool animations     = true;
	};

	inline Style& style()
	{
		static Style s;
		return s;
	}

	inline ImVec4 mixv(const ImVec4& a, const ImVec4& b, float t)
	{
		return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
		              a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
	}

	inline void sync_theme()
	{
		Style& s = style();

		const ImVec4 accent(settings::theme::accent[0], settings::theme::accent[1],
		                    settings::theme::accent[2], 1.f);
		const ImVec4 bg(settings::theme::bg_one[0], settings::theme::bg_one[1],
		                settings::theme::bg_one[2], 1.f);
		const ImVec4 surface(settings::theme::bg_two[0], settings::theme::bg_two[1],
		                     settings::theme::bg_two[2], 1.f);
		const ImVec4 stroke(settings::theme::stroke[0], settings::theme::stroke[1],
		                    settings::theme::stroke[2], 1.f);
		const ImVec4 text(settings::theme::text_active[0], settings::theme::text_active[1],
		                  settings::theme::text_active[2], 1.f);
		const ImVec4 text_dim(settings::theme::text_inactive[0], settings::theme::text_inactive[1],
		                      settings::theme::text_inactive[2], 1.f);

		s.main_surface = bg;
		s.top_surface  = bg;
		s.rail_surface = bg;
		s.card_surface = surface;

		s.text       = text;
		s.text_dim   = text_dim;
		s.text_faint = mixv(text_dim, bg, 0.35f);

		s.accent       = accent;
		s.pill_active  = accent;
		s.text_on_pill = mixv(bg, stroke, 0.15f);
	}

	inline void init_fonts(ImFontAtlas* atlas)
	{
		Style& s = style();
		s.body        = aether::fonts::body();
		s.body_medium = aether::fonts::medium();
		s.heading     = aether::fonts::heading();
		s.caption     = aether::fonts::caption();

		if (!atlas || atlas->Fonts.Size <= 0)
			return;
		if (!s.body)        s.body        = atlas->Fonts[0];
		if (!s.body_medium) s.body_medium = atlas->Fonts.Size > 2 ? atlas->Fonts[2] : atlas->Fonts[0];
		if (!s.heading)     s.heading     = s.body_medium;
		if (!s.caption)     s.caption     = atlas->Fonts.Size > 3 ? atlas->Fonts[3] : atlas->Fonts[0];
	}

	inline void push_surfaces_style()
	{
		Style& s = style();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, kPanelRadius);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, kCardRadius);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 8.f);
		ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 10.f);
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 10.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.f, 12.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9.f, 6.f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 6.f));
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 9.f);

		const ImVec4 white(1.f, 1.f, 1.f, 1.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg,           s.main_surface);
		ImGui::PushStyleColor(ImGuiCol_ChildBg,            s.card_surface);
		ImGui::PushStyleColor(ImGuiCol_PopupBg,            s.top_surface);
		ImGui::PushStyleColor(ImGuiCol_Border,             ImVec4(1.f, 1.f, 1.f, 0.07f));
		ImGui::PushStyleColor(ImGuiCol_BorderShadow,       ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_Text,               s.text);
		ImGui::PushStyleColor(ImGuiCol_TextDisabled,       s.text_dim);
		ImGui::PushStyleColor(ImGuiCol_FrameBg,            ImVec4(24 / 255.f, 27 / 255.f, 35 / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,     ImVec4(31 / 255.f, 35 / 255.f, 44 / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive,      ImVec4(35 / 255.f, 39 / 255.f, 49 / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_Header,             ImVec4(1.f, 1.f, 1.f, 0.06f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered,      ImVec4(1.f, 1.f, 1.f, 0.09f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive,       ImVec4(1.f, 1.f, 1.f, 0.12f));
		ImGui::PushStyleColor(ImGuiCol_Button,             ImVec4(1.f, 1.f, 1.f, 0.06f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,      ImVec4(1.f, 1.f, 1.f, 0.10f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,       ImVec4(1.f, 1.f, 1.f, 0.14f));
		ImGui::PushStyleColor(ImGuiCol_CheckMark,          s.accent);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab,         s.accent);
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,   white);
		ImGui::PushStyleColor(ImGuiCol_Separator,          ImVec4(1.f, 1.f, 1.f, 0.08f));
		ImGui::PushStyleColor(ImGuiCol_SeparatorHovered,   ImVec4(1.f, 1.f, 1.f, 0.18f));
		ImGui::PushStyleColor(ImGuiCol_SeparatorActive,    s.accent);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,        ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,      ImVec4(1.f, 1.f, 1.f, 0.16f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(1.f, 1.f, 1.f, 0.24f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImVec4(1.f, 1.f, 1.f, 0.32f));
		ImGui::PushStyleColor(ImGuiCol_TitleBg,            s.top_surface);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive,      s.top_surface);
		ImGui::PushStyleColor(ImGuiCol_Tab,                ImVec4(1.f, 1.f, 1.f, 0.05f));
		ImGui::PushStyleColor(ImGuiCol_TabHovered,         ImVec4(1.f, 1.f, 1.f, 0.10f));
		ImGui::PushStyleColor(ImGuiCol_TabActive,          ImVec4(1.f, 1.f, 1.f, 0.12f));
		ImGui::PushStyleColor(ImGuiCol_ResizeGrip,         ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered,  ImVec4(1.f, 1.f, 1.f, 0.20f));
		ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,     ImVec4(1.f, 1.f, 1.f, 0.16f));
		ImGui::PushStyleColor(ImGuiCol_NavHighlight,       ImVec4(0.f, 0.f, 0.f, 0.f));
	}

	inline void pop_surfaces_style()
	{
		ImGui::PopStyleColor(35);
		ImGui::PopStyleVar(14);
	}

	inline constexpr int kMaxOpacityDepth = 6;

	inline float* opacity_stack()
	{
		static float values[kMaxOpacityDepth] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f };
		return values;
	}

	inline int& opacity_depth()
	{
		static int depth = 0;
		return depth;
	}

	inline float opacity()
	{
		return opacity_stack()[opacity_depth()];
	}

	inline void push_opacity(float alpha)
	{
		const int depth = opacity_depth();
		if (depth + 1 >= kMaxOpacityDepth)
			return;
		opacity_stack()[depth + 1] = opacity_stack()[depth] * ImClamp(alpha, 0.f, 1.f);
		opacity_depth() = depth + 1;
	}

	inline void pop_opacity()
	{
		if (opacity_depth() > 0)
			--opacity_depth();
	}

	inline ImVec4 fade(const ImVec4& c)
	{
		ImVec4 out = c;
		out.w *= opacity();
		return out;
	}

	inline int fade_alpha(int alpha)
	{
		const int scaled = (int)(alpha * opacity());
		return scaled < 0 ? 0 : (scaled > 255 ? 255 : scaled);
	}

	inline ImU32 rgb(int r, int g, int b, int a = 255)
	{
		const int fa = fade_alpha(a);
		return IM_COL32(r, g, b, fa);
	}

	inline ImU32 to_u32(const ImVec4& c)
	{
		return ImGui::ColorConvertFloat4ToU32(fade(c));
	}

	inline float smooth_to(float current, float target, float rate, float delta)
	{
		if (!style().animations)
			return target;
		const float remaining = target - current;
		if (std::fabs(remaining) < 0.0008f)
			return target;
		return current + remaining * (1.f - std::exp(-rate * delta));
	}

	inline float anim_delta()
	{
		const float dt = ImGui::GetIO().DeltaTime;
		return dt > 0.f ? ImMin(dt, 1.f / 20.f) : 1.f / 60.f;
	}

	inline ImVec4 mix(const ImVec4& a, const ImVec4& b, float t)
	{
		const float k = ImClamp(t, 0.f, 1.f);
		return ImVec4(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
			a.z + (b.z - a.z) * k, a.w + (b.w - a.w) * k);
	}

	inline float text_w(const char* text, ImFont* font)
	{
		if (!text || !*text)
			return 0.f;
		if (font)
			ImGui::PushFont(font);
		const float w = ImGui::CalcTextSize(text).x;
		if (font)
			ImGui::PopFont();
		return w;
	}

	inline float text_h(ImFont* font)
	{
		return font ? font->FontSize : ImGui::GetFontSize();
	}

	inline void text_at(ImVec2 pos, const char* text, ImFont* font, const ImVec4& color)
	{
		if (!text || !*text)
			return;
		const ImVec2 snapped(std::floor(pos.x), std::floor(pos.y));
		if (font)
			ImGui::PushFont(font);
		ImGui::GetWindowDrawList()->AddText(snapped, to_u32(color), text);
		if (font)
			ImGui::PopFont();
	}

	inline void text_centered(ImVec2 origin, float width, const char* text, ImFont* font, const ImVec4& color)
	{
		text_at(ImVec2(origin.x + (width - text_w(text, font)) * 0.5f, origin.y), text, font, color);
	}

	inline float text_clipped(ImVec2 pos, const char* text, ImFont* font, const ImVec4& color, float max_width)
	{
		if (!text || !*text || max_width <= 1.f)
			return 0.f;

		ImFont* use = font ? font : ImGui::GetFont();
		const float size = use->FontSize;
		const ImVec2 measured = use->CalcTextSizeA(size, FLT_MAX, 0.f, text, nullptr);
		if (measured.x <= max_width)
		{
			text_at(pos, text, use, color);
			return measured.x;
		}

		static constexpr const char* k_ellipsis = "...";
		const float ellipsis_w = use->CalcTextSizeA(size, FLT_MAX, 0.f, k_ellipsis, nullptr).x;
		const float budget = max_width - ellipsis_w;
		if (budget <= 0.f)
			return 0.f;

		char probe[512];
		const size_t length = ImMin(std::strlen(text), sizeof(probe) - 1);
		size_t low = 0, high = length;
		while (low < high)
		{
			const size_t middle = (low + high + 1) / 2;
			const size_t take = middle;
			std::memcpy(probe, text, take);
			probe[take] = '\0';
			if (use->CalcTextSizeA(size, FLT_MAX, 0.f, probe, nullptr).x <= budget)
				low = middle;
			else
				high = middle - 1;
		}

		while (low > 0 && ((unsigned char)text[low] & 0xC0u) == 0x80u)
			--low;

		char drawn[512];
		const size_t take = low < sizeof(drawn) - 4 ? low : sizeof(drawn) - 4;
		std::memcpy(drawn, text, take);
		std::memcpy(drawn + take, k_ellipsis, 4);
		text_at(pos, drawn, use, color);
		return use->CalcTextSizeA(size, FLT_MAX, 0.f, drawn, nullptr).x;
	}

	inline void panel(ImVec2 a, ImVec2 b, float radius, const ImVec4& surface, bool border = false, int border_alpha = 12)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(a, b, to_u32(surface), radius);
		if (border)
			dl->AddRect(a, b, rgb(255, 255, 255, border_alpha), radius);
	}

	inline void divider(ImVec2 a, ImVec2 b)
	{
		ImGui::GetWindowDrawList()->AddLine(a, b, rgb(255, 255, 255, 13), 1.f);
	}

	inline void glass_panel(ImVec2 a, ImVec2 b, float radius, const ImVec4& surface,
		bool shadow = true, bool sheen = true, bool border = true, float shadow_strength = 0.55f)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec4 sh(0.f, 0.f, 0.f, 0.16f * shadow_strength);

		if (shadow)
		{

			dl->AddRectFilled(ImVec2(a.x - 2.f, a.y + 3.f), ImVec2(b.x + 2.f, b.y + 5.f),
				to_u32(sh), radius + 2.f);
			dl->AddRectFilled(ImVec2(a.x - 5.f, a.y + 7.f), ImVec2(b.x + 5.f, b.y + 12.f),
				to_u32(ImVec4(0.f, 0.f, 0.f, sh.w * 0.55f)), radius + 5.f);
		}

		dl->AddRectFilled(a, b, to_u32(surface), radius);

		if (sheen)
			dl->AddRect(ImVec2(a.x + 1.f, a.y + 1.f), ImVec2(b.x - 1.f, b.y - 1.f),
				rgb(255, 255, 255, 14), radius - 1.f, 0, 1.f);

		if (border)
			dl->AddRect(a, b, rgb(255, 255, 255, 11), radius, 0, 1.f);
	}

	inline const char* key_name(int vk, char* buffer, size_t size)
	{
		if (!buffer || size == 0)
			return "None";
		buffer[0] = '\0';
		switch (vk)
		{
		case 0:              return "None";
		case VK_LBUTTON:     return "Mouse 1";
		case VK_RBUTTON:     return "Mouse 2";
		case VK_MBUTTON:     return "Mouse 3";
		case VK_XBUTTON1:    return "Mouse 4";
		case VK_XBUTTON2:    return "Mouse 5";
		case VK_BACK:        return "Backspace";
		case VK_TAB:         return "Tab";
		case VK_RETURN:      return "Enter";
		case VK_SHIFT:       return "Shift";
		case VK_CONTROL:     return "Ctrl";
		case VK_MENU:        return "Alt";
		case VK_PAUSE:       return "Pause";
		case VK_CAPITAL:     return "Caps Lock";
		case VK_ESCAPE:      return "Escape";
		case VK_SPACE:       return "Space";
		case VK_PRIOR:       return "Page Up";
		case VK_NEXT:        return "Page Down";
		case VK_END:         return "End";
		case VK_HOME:        return "Home";
		case VK_LEFT:        return "Left";
		case VK_UP:          return "Up";
		case VK_RIGHT:       return "Right";
		case VK_DOWN:        return "Down";
		case VK_INSERT:      return "Insert";
		case VK_DELETE:      return "Delete";
		case VK_LWIN:        return "Left Windows";
		case VK_RWIN:        return "Right Windows";
		case VK_NUMLOCK:     return "Num Lock";
		case VK_SCROLL:      return "Scroll Lock";
		case VK_LSHIFT:      return "Left Shift";
		case VK_RSHIFT:      return "Right Shift";
		case VK_LCONTROL:    return "Left Ctrl";
		case VK_RCONTROL:    return "Right Ctrl";
		case VK_LMENU:       return "Left Alt";
		case VK_RMENU:       return "Right Alt";
		case VK_MULTIPLY:    return "Numpad *";
		case VK_ADD:         return "Numpad +";
		case VK_SUBTRACT:    return "Numpad -";
		case VK_DECIMAL:     return "Numpad .";
		case VK_DIVIDE:      return "Numpad /";
		case VK_OEM_1:       return ";";
		case VK_OEM_PLUS:    return "=";
		case VK_OEM_COMMA:   return ",";
		case VK_OEM_MINUS:   return "-";
		case VK_OEM_PERIOD:  return ".";
		case VK_OEM_2:       return "/";
		case VK_OEM_3:       return "`";
		case VK_OEM_4:       return "[";
		case VK_OEM_5:       return "\\";
		case VK_OEM_6:       return "]";
		case VK_OEM_7:       return "'";
		default: break;
		}
		if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z'))
		{
			buffer[0] = (char)vk;
			buffer[1] = '\0';
			return buffer;
		}
		if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
		{
			snprintf(buffer, size, "Numpad %d", vk - VK_NUMPAD0);
			return buffer;
		}
		if (vk >= VK_F1 && vk <= VK_F24)
		{
			snprintf(buffer, size, "F%d", vk - VK_F1 + 1);
			return buffer;
		}
		snprintf(buffer, size, "VK %02X", vk);
		return buffer;
	}

	struct BindCapture
	{
		bool  active   = false;
		bool  armed    = false;
		int*  target   = nullptr;
		int   cooldown = 0;
		bool  held[256]{};
	};

	inline BindCapture& bind_capture()
	{
		static BindCapture c;
		return c;
	}

	inline void cancel_bind_capture()
	{
		BindCapture& c = bind_capture();
		c.active   = false;
		c.armed    = false;
		c.target   = nullptr;
		c.cooldown = 6;
	}

	inline const char* bind_label(int* key, int slot)
	{
		static char buffers[4][32]{};
		const int index = slot & 3;
		const BindCapture& cap = bind_capture();
		if (cap.active && cap.target == key)
			return "Press key...";
		buffers[index][0] = '\0';
		return key_name(key ? *key : 0, buffers[index], sizeof(buffers[index]));
	}

	inline void begin_bind_capture(int* target)
	{
		BindCapture& c = bind_capture();

		if (c.cooldown > 0)
			return;

		if (c.active && c.target == target)
		{
			cancel_bind_capture();
			return;
		}

		c.active   = true;
		c.armed    = false;
		c.target   = target;
		c.cooldown = 0;
		for (int vk = 0; vk < 256; ++vk)
			c.held[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
	}

	inline bool capturing_any()
	{
		return bind_capture().active;
	}

	inline bool poll_bind_capture()
	{
		BindCapture& c = bind_capture();

		if (c.cooldown > 0)
			--c.cooldown;

		if (!c.active || !c.target)
			return false;

		bool blocked = false;
		for (int vk = 1; vk < 256; ++vk)
		{
			const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
			if (!down)
				c.held[vk] = false;
			else if (c.held[vk])
				blocked = true;
		}
		c.armed = !blocked;

		if (!c.armed)
			return false;

		if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
		{
			cancel_bind_capture();
			return false;
		}

		for (int vk = 1; vk < 256; ++vk)
		{
			if (vk == VK_ESCAPE || c.held[vk])
				continue;
			if ((GetAsyncKeyState(vk) & 0x8000) != 0)
			{
				*c.target  = vk;
				c.active   = false;
				c.armed    = false;
				c.target   = nullptr;
				c.cooldown = 8;
				return true;
			}
		}

		return false;
	}

	inline float anim_toggle(ImGuiID id, bool on)
	{
		ImGuiStorage* storage = ImGui::GetStateStorage();
		const float target = on ? 1.f : 0.f;
		if (!style().animations)
		{
			storage->SetFloat(id, target);
			return target;
		}
		float value = storage->GetFloat(id, target);
		const float step = anim_delta() / 0.09f;
		value = (value < target) ? ImMin(target, value + step) : ImMax(target, value - step);
		storage->SetFloat(id, value);
		return value;
	}

	inline float anim_to(ImGuiID id, bool on, float speed = 14.f)
	{
		ImGuiStorage* storage = ImGui::GetStateStorage();
		float value = storage->GetFloat(id, on ? 1.f : 0.f);
		if (!style().animations)
		{
			storage->SetFloat(id, on ? 1.f : 0.f);
			return on ? 1.f : 0.f;
		}
		value = smooth_to(value, on ? 1.f : 0.f, speed, anim_delta());
		storage->SetFloat(id, value);
		return value;
	}

	inline float anim_hover(ImGuiID id, bool hovered, float target_rate = 16.f)
	{
		ImGuiStorage* storage = ImGui::GetStateStorage();
		const ImGuiID key = (ImGuiID)(id ^ 0x5bf03635u);
		const float target = hovered ? 1.f : 0.f;
		if (!style().animations)
		{
			storage->SetFloat(key, target);
			return target;
		}
		const float value = smooth_to(storage->GetFloat(key, target), target, target_rate, anim_delta());
		storage->SetFloat(key, value);
		return value;
	}

	inline bool toggle(const char* id, ImVec2 origin, bool* value, bool enabled = true)
	{
		constexpr ImVec2 size(kToggleW, kToggleH);
		const ImGuiID anim_id = ImGui::GetID(id);

		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(origin);
		if (!enabled)
			ImGui::BeginDisabled();
		const bool clicked = ImGui::InvisibleButton("##toggle", size);
		const bool hovered = ImGui::IsItemHovered();
		if (!enabled)
			ImGui::EndDisabled();
		ImGui::PopID();

		if (enabled && clicked && value)
			*value = !*value;

		const float animation = anim_toggle(anim_id, value && *value);
		const float hover = enabled ? anim_hover(anim_id, hovered, 12.f) : 0.f;

		const float eased = animation * animation * (3.f - 2.f * animation);
		const int track_r = (int)(14.f + eased * 192.f + hover * 10.f);
		const int track_g = (int)(14.f + eased * 192.f + hover * 10.f);
		const int track_b = (int)(16.f + eased * 196.f + hover * 12.f);
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
			rgb(track_r, track_g, track_b, (int)(210.f + eased * 45.f)), size.y * 0.5f);
		dl->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y),
			rgb(255, 255, 255, enabled ? (int)(14.f + eased * 20.f) : 6), size.y * 0.5f);
		const float knob_x = origin.x + 11.f + eased * (size.x - 22.f);
		dl->AddCircleFilled(ImVec2(knob_x, origin.y + 11.f), 7.f,
			eased > 0.5f ? rgb(255, 255, 255) : rgb(242, 242, 244), 18);
		return enabled && clicked && value && *value;
	}

	inline bool slider_float_field(const char* id, ImVec2 origin, float width,
		float* value, float minimum, float maximum)
	{
		constexpr float height = 26.f;
		constexpr float inset  = 7.f;
		const float track = width - inset * 2.f;
		if (track <= 1.f || !value)
			return false;
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(origin);
		ImGui::InvisibleButton("##slider", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		const bool active  = ImGui::IsItemActive();
		if (active)
		{
			const float raw = (ImGui::GetIO().MousePos.x - origin.x - inset) / track;
			const float t = ImClamp(raw, 0.f, 1.f);
			*value = minimum + t * (maximum - minimum);
		}
		ImGui::PopID();

		const float t = ImClamp((*value - minimum) / (maximum - minimum), 0.f, 1.f);
		const float y = origin.y + height * 0.5f;
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(ImVec2(origin.x + inset, y - 2.f), ImVec2(origin.x + inset + track, y + 2.f),
			rgb(255, 255, 255, 28), 2.f);
		dl->AddRectFilled(ImVec2(origin.x + inset, y - 2.f), ImVec2(origin.x + inset + track * t, y + 2.f),
			rgb(206, 206, 211), 2.f);
		dl->AddCircleFilled(ImVec2(origin.x + inset + track * t, y),
			active ? 7.f : (hovered ? 6.5f : 6.f), rgb(246, 246, 248), 20);
		return active;
	}

	inline bool slider_int_field(const char* id, ImVec2 origin, float width,
		int* value, int minimum, int maximum)
	{
		constexpr float height = 26.f;
		constexpr float inset  = 7.f;
		const float track = width - inset * 2.f;
		if (track <= 1.f || !value)
			return false;
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(origin);
		ImGui::InvisibleButton("##slider", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		const bool active  = ImGui::IsItemActive();
		bool changed = false;
		if (active)
		{
			const float raw = (ImGui::GetIO().MousePos.x - origin.x - inset) / track;
			const float t = ImClamp(raw, 0.f, 1.f);
			const int next = minimum + (int)(t * (maximum - minimum) + 0.5f);
			changed = next != *value;
			*value = next;
		}
		ImGui::PopID();

		const float t = maximum > minimum
			? ImClamp((float)(*value - minimum) / (float)(maximum - minimum), 0.f, 1.f) : 0.f;
		const float y = origin.y + height * 0.5f;
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(ImVec2(origin.x + inset, y - 2.f), ImVec2(origin.x + inset + track, y + 2.f),
			rgb(255, 255, 255, 28), 2.f);
		dl->AddRectFilled(ImVec2(origin.x + inset, y - 2.f), ImVec2(origin.x + inset + track * t, y + 2.f),
			rgb(206, 206, 211), 2.f);
		dl->AddCircleFilled(ImVec2(origin.x + inset + track * t, y),
			active ? 7.f : (hovered ? 6.5f : 6.f), rgb(246, 246, 248), 20);
		return active || changed;
	}

	inline void push_popup_style()
	{

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7.f, 7.f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(28 / 255.f, 28 / 255.f, 34 / 255.f, 0.96f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.10f));
	}

	inline void pop_popup_style()
	{
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);
	}

	inline bool dropdown_field(const char* id, ImVec2 origin, float width, const char* label,
		int* selected, const char* const* options, int option_count, bool enabled = true)
	{
		if (!selected || !options || option_count <= 0)
			return false;

		const ImVec2 field_min(origin.x + width - kFieldW, origin.y);
		const ImVec2 field_max(field_min.x + kFieldW, field_min.y + kFieldH);
		ImVec4 label_color = enabled ? style().text : style().text_dim;

		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(field_min);
		if (!enabled)
			ImGui::BeginDisabled();
		const bool clicked = ImGui::InvisibleButton("##field", ImVec2(kFieldW, kFieldH));
		const bool hovered = ImGui::IsItemHovered();
		if (clicked)
			ImGui::OpenPopup("##options");
		const bool open = ImGui::IsPopupOpen("##options");

		dl->AddRectFilled(field_min, field_max,
			rgb(16, 16, 19, enabled ? (open ? 120 : (hovered ? 110 : 90)) : 60), 9.f);
		dl->AddRect(field_min, field_max,
			rgb(255, 255, 255, enabled ? (open ? 92 : (hovered ? 60 : 34)) : 16), 9.f);

		const int shown = ImClamp(*selected, 0, option_count - 1);
		const float label_h = text_h(style().caption);
		text_at(ImVec2(field_min.x + 12.f, field_min.y + (kFieldH - label_h) * 0.5f),
			options[shown], style().caption, label_color);

		{
			const float cx = field_max.x - 15.f;
			const float cy = field_min.y + kFieldH * 0.5f;
			const ImU32 chev = rgb(200, 203, 212, enabled ? 220 : 80);
			dl->AddLine(ImVec2(cx - 4.f, cy - 2.f), ImVec2(cx, cy + 2.f), chev, 1.6f);
			dl->AddLine(ImVec2(cx, cy + 2.f), ImVec2(cx + 4.f, cy - 2.f), chev, 1.6f);
		}

		bool changed = false;
		ImGui::SetNextWindowPos(ImVec2(field_min.x, field_max.y + 7.f));
		ImGui::SetNextWindowSize(ImVec2(kFieldW + 2.f, option_count * 30.f + 16.f));
		push_popup_style();
		if (ImGui::BeginPopup("##options", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImDrawList* pdl = ImGui::GetWindowDrawList();
			for (int i = 0; i < option_count; ++i)
			{
				const ImVec2 option_min = ImGui::GetCursorScreenPos();
				ImGui::PushID(i);
				if (ImGui::InvisibleButton("##option", ImVec2(kFieldW - 12.f, 28.f)))
				{
					*selected = i;
					changed = true;
					ImGui::CloseCurrentPopup();
				}
				const bool option_hovered = ImGui::IsItemHovered();
				ImGui::PopID();
				if (option_hovered || shown == i)
					pdl->AddRectFilled(option_min, ImVec2(option_min.x + kFieldW - 12.f, option_min.y + 28.f),
						shown == i ? rgb(255, 255, 255, 18) : rgb(255, 255, 255, 10), 11.f);
				text_at(ImVec2(option_min.x + 10.f, option_min.y + 5.f), options[i], style().caption,
					shown == i ? style().text : style().text_dim);
				if (shown == i)
					pdl->AddCircleFilled(ImVec2(option_min.x + kFieldW - 24.f, option_min.y + 14.f), 2.5f,
						rgb(225, 228, 236), 12);
				ImGui::SetCursorScreenPos(ImVec2(option_min.x, option_min.y + 30.f));
			}
			ImGui::EndPopup();
		}
		pop_popup_style();
		ImGui::PopID();
		if (!enabled)
			ImGui::EndDisabled();
		return changed;
	}

	inline bool multi_dropdown_field(const char* id, ImVec2 origin, float width, const char* label,
		unsigned int* selected, const char* const* options, int option_count)
	{
		if (!selected || !options || option_count <= 0)
			return false;
		char summary[64]{};
		int  count = 0;
		for (int i = 0; i < option_count; ++i)
		{
			if ((*selected & (1u << i)) == 0)
				continue;
			if (count == 0)
				snprintf(summary, sizeof summary, "%s", options[i]);
			++count;
		}
		if (count == 0)
			snprintf(summary, sizeof summary, "None");
		else if (count > 1)
			snprintf(summary, sizeof summary, "%d selected", count);

		const ImVec2 field_min(origin.x + width - kFieldW, origin.y);
		const ImVec2 field_max(field_min.x + kFieldW, field_min.y + kFieldH);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(field_min);
		if (ImGui::InvisibleButton("##field", ImVec2(kFieldW, kFieldH)))
			ImGui::OpenPopup("##multi");
		const bool hovered = ImGui::IsItemHovered();
		const bool open = ImGui::IsPopupOpen("##multi");
		dl->AddRectFilled(field_min, field_max,
			rgb(16, 16, 19, open ? 120 : (hovered ? 110 : 90)), 9.f);
		dl->AddRect(field_min, field_max, rgb(255, 255, 255, open ? 92 : (hovered ? 60 : 34)), 9.f);
		text_at(ImVec2(field_min.x + 12.f, field_min.y + (kFieldH - text_h(style().caption)) * 0.5f),
			summary, style().caption, style().text);

		bool changed = false;
		ImGui::SetNextWindowPos(ImVec2(field_min.x, field_max.y + 7.f));
		ImGui::SetNextWindowSize(ImVec2(kFieldW + 2.f, option_count * 30.f + 16.f));
		push_popup_style();
		if (ImGui::BeginPopup("##multi", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImDrawList* pdl = ImGui::GetWindowDrawList();
			for (int i = 0; i < option_count; ++i)
			{
				const ImVec2 option_min = ImGui::GetCursorScreenPos();
				ImGui::PushID(i);
				if (ImGui::InvisibleButton("##option", ImVec2(kFieldW - 12.f, 28.f)))
				{
					const unsigned int next = *selected ^ (1u << i);
					if (next != 0)
					{
						*selected = next;
						changed = true;
					}
				}
				ImGui::PopID();
				const bool active = (*selected & (1u << i)) != 0;
				if (ImGui::IsItemHovered() || active)
					pdl->AddRectFilled(option_min, ImVec2(option_min.x + kFieldW - 12.f, option_min.y + 28.f),
						active ? rgb(255, 255, 255, 18) : rgb(255, 255, 255, 10), 11.f);
				text_at(ImVec2(option_min.x + 10.f, option_min.y + 5.f), options[i], style().caption,
					active ? style().text : style().text_dim);
				const ImVec2 check_min(option_min.x + kFieldW - 35.f, option_min.y + 7.f);
				pdl->AddRect(check_min, ImVec2(check_min.x + 14.f, check_min.y + 14.f),
					rgb(255, 255, 255, active ? 85 : 24), 4.f);
				if (active)
					pdl->AddRectFilled(ImVec2(check_min.x + 3.f, check_min.y + 3.f),
						ImVec2(check_min.x + 11.f, check_min.y + 11.f), rgb(222, 225, 233), 2.f);
				ImGui::SetCursorScreenPos(ImVec2(option_min.x, option_min.y + 30.f));
			}
			ImGui::EndPopup();
		}
		pop_popup_style();
		ImGui::PopID();
		return changed;
	}

	inline bool keybind_field(const char* id, ImVec2 origin, float width, const char* text, bool capturing)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(origin);
		const bool clicked = ImGui::InvisibleButton("##bind", ImVec2(width, kBindH));
		const bool hovered = ImGui::IsItemHovered();
		const bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
		ImGui::PopID();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + kBindH),
			rgb(16, 16, 19, capturing ? 120 : (hovered ? 110 : 90)), 9.f);
		dl->AddRect(origin, ImVec2(origin.x + width, origin.y + kBindH),
			rgb(255, 255, 255, capturing ? 92 : (hovered ? 60 : 34)), 9.f);
		text_centered(ImVec2(origin.x, origin.y + (kBindH - text_h(style().caption)) * 0.5f), width,
			text, style().caption, capturing ? style().text : style().text_dim);
		return clicked || right_clicked;
	}

	inline void color_field(const char* id, ImVec2 origin, float width, const char* label, float* rgba)
	{
		if (!rgba)
			return;
		const float label_h = text_h(style().body);
		text_at(ImVec2(origin.x, origin.y + (kFieldH - label_h) * 0.5f), label, style().body,
			style().text);
		const ImVec2 swatch_min(origin.x + width - kSwatchD, origin.y + (kFieldH - kSwatchD) * 0.5f);
		const ImVec2 swatch_max(swatch_min.x + kSwatchD, swatch_min.y + kSwatchD);
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(swatch_min);
		if (ImGui::InvisibleButton("##swatch", ImVec2(kSwatchD, kSwatchD)))
		{
			ImGui::OpenPopup("##picker");
		}
		const ImVec4 current(rgba[0], rgba[1], rgba[2], rgba[3]);
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(swatch_min, swatch_max, to_u32(current), 7.f);
		dl->AddRect(swatch_min, swatch_max, rgb(255, 255, 255, 34), 7.f);

		constexpr ImVec2 popup_size(194.f, 226.f);
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImVec2 work_min = viewport->WorkPos;
		const ImVec2 work_max(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y);
		float popup_x = swatch_max.x - popup_size.x;
		float popup_y = swatch_max.y + 6.f;
		if (popup_y + popup_size.y > work_max.y - 8.f)
			popup_y = swatch_min.y - popup_size.y - 6.f;
		popup_x = ImClamp(popup_x, work_min.x + 8.f, work_max.x - popup_size.x - 8.f);
		popup_y = ImClamp(popup_y, work_min.y + 8.f, work_max.y - popup_size.y - 8.f);
		ImGui::SetNextWindowPos(ImVec2(popup_x, popup_y));
		ImGui::SetNextWindowSize(popup_size);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 9.f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.055f, 0.065f, 0.085f, 0.98f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.10f));
		if (ImGui::BeginPopup("##picker"))
		{
			text_at(ImGui::GetCursorScreenPos(), label, style().body_medium, style().text);
			ImGui::Dummy(ImVec2(1.f, 20.f));
			const ImVec4 presets[] = {
				ImVec4(0.96f, 0.96f, 0.98f, 1.f), ImVec4(0.36f, 0.82f, 1.f, 1.f),
				ImVec4(0.50f, 0.91f, 0.63f, 1.f), ImVec4(0.73f, 0.51f, 1.f, 1.f),
				ImVec4(1.00f, 0.45f, 0.54f, 1.f), ImVec4(1.00f, 0.72f, 0.32f, 1.f)
			};
			ImDrawList* pdl = ImGui::GetWindowDrawList();
			const ImVec2 preset = ImGui::GetCursorScreenPos();
			for (int i = 0; i < 6; ++i)
			{
				const ImVec2 preset_pos(preset.x + i * 28.f, preset.y);
				ImGui::SetCursorScreenPos(preset_pos);
				ImGui::PushID(i);
				if (ImGui::InvisibleButton("##preset", ImVec2(22.f, 18.f)))
				{
					rgba[0] = presets[i].x;
					rgba[1] = presets[i].y;
					rgba[2] = presets[i].z;
				}
				ImGui::PopID();
				pdl->AddRectFilled(preset_pos, ImVec2(preset_pos.x + 22.f, preset_pos.y + 18.f), to_u32(presets[i]), 6.f);
				pdl->AddRect(preset_pos, ImVec2(preset_pos.x + 22.f, preset_pos.y + 18.f), rgb(255, 255, 255, 28), 6.f);
			}
			ImGui::SetCursorScreenPos(ImVec2(preset.x, preset.y + 25.f));
			ImGui::SetNextItemWidth(174.f);
			ImGui::ColorPicker3("##hsv", rgba,
				ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayHSV |
				ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview);
			const ImVec2 picker_min = ImGui::GetItemRectMin();
			const ImVec2 picker_max = ImGui::GetItemRectMax();
			pdl->AddRect(picker_min, picker_max, rgb(255, 255, 255, 28), 10.f, 0, 1.f);
			ImGui::EndPopup();
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(4);
		ImGui::PopID();
	}

	inline bool text_button(const char* id, ImVec2 origin, float width, const char* label,
		bool enabled = true, bool accent = false)
	{
		constexpr float height = 32.f;
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(origin);
		if (!enabled)
			ImGui::BeginDisabled();
		const bool clicked = ImGui::InvisibleButton("##btn", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		if (!enabled)
			ImGui::EndDisabled();
		ImGui::PopID();

		ImVec4 fill = accent ? ImVec4(0.87f, 0.88f, 0.92f, 1.f) : ImVec4(1.f, 1.f, 1.f, 0.045f);
		if (hovered && enabled)
			fill = accent ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(1.f, 1.f, 1.f, 0.085f);
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), to_u32(fill), 10.f);
		dl->AddRect(origin, ImVec2(origin.x + width, origin.y + height),
			rgb(255, 255, 255, enabled ? 20 : 8), 10.f);
		text_centered(ImVec2(origin.x, origin.y + 8.f), width, label, style().caption,
			accent ? ImVec4(0.05f, 0.06f, 0.08f, enabled ? 1.f : 0.5f)
			       : ImVec4(style().text.x, style().text.y, style().text.z, enabled ? 1.f : 0.45f));
		return clicked && enabled;
	}

	inline void status_row(ImVec2 origin, float width, const char* label, const char* value, bool good)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const float pulse = 0.85f + 0.15f * std::sin((float)ImGui::GetTime() * 2.4f);
		if (good)
			dl->AddCircleFilled(ImVec2(origin.x + 5.f, origin.y + 8.f), 7.f,
				rgb(112, 185, 144, (int)(30.f * pulse)), 20);
		dl->AddCircleFilled(ImVec2(origin.x + 5.f, origin.y + 8.f), 3.5f,
			good ? rgb(112, 185, 144) : rgb(221, 145, 72), 16);

		constexpr float k_dot_column = 18.f;
		constexpr float k_value_gap  = 12.f;
		const float label_room = ImMax(24.f, width - k_dot_column - k_value_gap);
		const float label_width = ImMin(text_w(label, style().caption), label_room);
		text_clipped(ImVec2(origin.x + k_dot_column, origin.y), label, style().caption,
			style().text_dim, label_room);

		const float value_room = width - k_dot_column - label_width - k_value_gap;
		const float value_width = text_w(value, style().caption);
		const bool fits = value_width <= value_room;
		const ImVec4 value_color = good ? style().text : ImVec4(0.87f, 0.57f, 0.28f, 1.f);
		text_clipped(ImVec2(fits ? origin.x + width - value_width : origin.x + k_dot_column + label_width + k_value_gap,
			origin.y), value, style().caption, value_color,
			ImMax(0.f, fits ? value_width : value_room));
	}

	struct Card
	{
		ImVec2 min{};
		float  right  = 0.f;
		float  x      = 0.f;
		float  width  = 0.f;
		float  y      = 0.f;
		const char* title = nullptr;
		bool   opened = false;
		bool   boxed  = false;
	};

#ifdef NM_LAYOUT_AUDIT

	struct CardAuditSink
	{
		int    count = 0;
		ImVec2 min[192]{};
		ImVec2 max[192]{};
	};
	inline CardAuditSink& card_audit() { static CardAuditSink sink; return sink; }
	inline void card_audit_record(ImVec2 a, ImVec2 b)
	{
		CardAuditSink& sink = card_audit();
		if (sink.count < 192)
		{
			sink.min[sink.count] = a;
			sink.max[sink.count] = b;
			++sink.count;
		}
	}
#endif

	inline Card card_begin(const char* title, ImVec2 min, float width, float top_pad = kCardTopPad,
		bool boxed = false)
	{
		Card c;
		c.min    = min;
		c.right  = min.x + width;
		c.x      = min.x + kCardInset;
		c.width  = width - kCardInset * 2.f;
		c.y      = min.y + top_pad;
		c.title  = title;
		c.boxed  = boxed;
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->ChannelsSplit(2);
		dl->ChannelsSetCurrent(1);
		c.opened = true;
		return c;
	}

	inline float card_end(Card& c)
	{
		const float bottom = c.y + 6.f;
		if (!c.opened)
			return bottom;
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->ChannelsSetCurrent(0);
		if (c.boxed)
		{
			dl->AddRectFilled(c.min, ImVec2(c.right, bottom), to_u32(style().card_surface), kCardRadius);
			dl->AddRect(c.min, ImVec2(c.right, bottom), rgb(255, 255, 255, 12), kCardRadius);
		}
		if (c.title)
			text_at(ImVec2(c.min.x, c.min.y + 2.f), c.title, style().body_medium, style().text);
		dl->ChannelsMerge();
#ifdef NM_LAYOUT_AUDIT
		card_audit_record(c.min, ImVec2(c.right, bottom));
#endif
		c.opened = false;
		return bottom;
	}

	inline void row_gap(Card& c, float height)
	{
		c.y += height;
	}

	inline void row_section(Card& c, const char* label)
	{
		text_at(ImVec2(c.x, c.y + 2.f), label, style().body_medium, style().text);
		c.y += text_h(style().body_medium) + 8.f;
	}

	inline void row_note(Card& c, const char* label)
	{
		text_at(ImVec2(c.x, c.y), label, style().caption, style().text_dim);
		c.y += text_h(style().caption) + 8.f;
	}

	inline bool row_toggle(Card& c, const char* label, bool* value, bool enabled = true)
	{
		const float row_h = kToggleH + kRowPad * 2.f;
		const ImVec2 origin(c.x, c.y + kRowPad);
		toggle(label, origin, value, enabled);
		const float label_h = text_h(style().body);
		text_at(ImVec2(origin.x + kToggleW + 12.f, origin.y + (kToggleH - label_h) * 0.5f), label,
			style().body, enabled ? style().text : style().text_dim);
		c.y += row_h + kRowGap;
		return enabled && value && *value;
	}

	inline bool row_toggle_bind(Card& c, const char* label, bool* value, int* key)
	{
		const float row_h = ImMax(kToggleH, kBindH) + kRowPad * 2.f;
		const ImVec2 origin(c.x, c.y + kRowPad);
		toggle(label, origin, value);

		const float label_h = text_h(style().body);
		text_at(ImVec2(origin.x + kToggleW + 12.f, origin.y + (kToggleH - label_h) * 0.5f), label,
			style().body, style().text);

		const bool bind_clicked = keybind_field(label, ImVec2(c.x + c.width - kBindW, origin.y), kBindW,
			bind_label(key, 0), bind_capture().active && bind_capture().target == key);
		c.y += row_h + kRowGap;
		return bind_clicked;
	}

	inline bool row_bind(Card& c, const char* label, int* key)
	{
		const float row_h = kBindH + kRowPad * 2.f;
		const ImVec2 origin(c.x, c.y + kRowPad);
		const float label_h = text_h(style().body);
		text_at(ImVec2(c.x, origin.y + (kBindH - label_h) * 0.5f), label, style().body, style().text);
		const bool clicked = keybind_field(label, ImVec2(c.x + c.width - kBindW, origin.y), kBindW,
			bind_label(key, 1), bind_capture().active && bind_capture().target == key);
		c.y += row_h + kRowGap;
		return clicked;
	}

	inline void row_slider_i(Card& c, const char* label, int* value, int minimum, int maximum,
		const char* suffix = "", bool enabled = true)
	{
		char value_text[32];
		snprintf(value_text, sizeof value_text, "%d%s", value ? *value : 0, suffix);
		const float name_h = text_h(style().body_medium);
		text_at(ImVec2(c.x, c.y + kRowPad), label, style().body_medium,
			enabled ? style().text : style().text_dim);
		const float value_w = text_w(value_text, style().body);
		text_at(ImVec2(c.x + c.width - value_w,
			c.y + kRowPad + (name_h - text_h(style().body)) * 0.5f), value_text, style().body,
			enabled ? style().text_faint : style().text_dim);
		if (!enabled)
			ImGui::BeginDisabled();
		slider_int_field(label, ImVec2(c.x - 7.f, c.y + kRowPad + name_h + 8.f), c.width + 14.f,
			value, minimum, maximum);
		if (!enabled)
			ImGui::EndDisabled();
		c.y += kRowPad * 2.f + name_h + 34.f + kRowGap;
	}

	inline void row_slider_f(Card& c, const char* label, float* value, float minimum, float maximum,
		const char* format = "%.2f", bool enabled = true)
	{
		char value_text[32];
		snprintf(value_text, sizeof value_text, format, value ? *value : 0.f);
		const float name_h = text_h(style().body_medium);
		text_at(ImVec2(c.x, c.y + kRowPad), label, style().body_medium,
			enabled ? style().text : style().text_dim);
		const float value_w = text_w(value_text, style().body);
		text_at(ImVec2(c.x + c.width - value_w,
			c.y + kRowPad + (name_h - text_h(style().body)) * 0.5f), value_text, style().body,
			enabled ? style().text_faint : style().text_dim);
		slider_float_field(label, ImVec2(c.x - 7.f, c.y + kRowPad + name_h + 8.f), c.width + 14.f,
			value, minimum, maximum);
		c.y += kRowPad * 2.f + name_h + 34.f + kRowGap;
	}

	inline bool row_dropdown(Card& c, const char* label, int* selected, const char* const* options,
		int option_count, bool enabled = true, const char* desc = nullptr)
	{
		const float name_h = text_h(style().body_medium);
		const float block_h = name_h + (desc ? 3.f + text_h(style().caption) : 0.f);
		const float row_h = ImMax(block_h, kFieldH) + kRowPad * 2.f;
		const float ty = c.y + (row_h - block_h) * 0.5f;
		text_at(ImVec2(c.x, ty), label, style().body_medium, enabled ? style().text : style().text_dim);
		if (desc)
			text_at(ImVec2(c.x, ty + name_h + 3.f), desc, style().caption, style().text_dim);
		const bool changed = dropdown_field(label, ImVec2(c.x, c.y + (row_h - kFieldH) * 0.5f),
			c.width, label, selected, options, option_count, enabled);
		c.y += row_h + kRowGap;
		return changed;
	}

	inline bool row_multi(Card& c, const char* label, unsigned int* selected, const char* const* options,
		int option_count, const char* desc = nullptr)
	{
		const float name_h = text_h(style().body_medium);
		const float block_h = name_h + (desc ? 3.f + text_h(style().caption) : 0.f);
		const float row_h = ImMax(block_h, kFieldH) + kRowPad * 2.f;
		const float ty = c.y + (row_h - block_h) * 0.5f;
		text_at(ImVec2(c.x, ty), label, style().body_medium, style().text);
		if (desc)
			text_at(ImVec2(c.x, ty + name_h + 3.f), desc, style().caption, style().text_dim);
		const bool changed = multi_dropdown_field(label, ImVec2(c.x, c.y + (row_h - kFieldH) * 0.5f),
			c.width, label, selected, options, option_count);
		c.y += row_h + kRowGap;
		return changed;
	}

	inline void row_color(Card& c, const char* label, float* rgba)
	{
		const float row_h = kSwatchD + kRowPad * 2.f;
		color_field(label, ImVec2(c.x, c.y + kRowPad), c.width, label, rgba);
		c.y += row_h + kRowGap;
	}

	inline void row_color3(Card& c, const char* label, float* rgb)
	{
		if (!rgb)
			return;
		float rgba[4] = { rgb[0], rgb[1], rgb[2], 1.f };
		const float row_h = kSwatchD + kRowPad * 2.f;
		color_field(label, ImVec2(c.x, c.y + kRowPad), c.width, label, rgba);
		rgb[0] = rgba[0];
		rgb[1] = rgba[1];
		rgb[2] = rgba[2];
		c.y += row_h + kRowGap;
	}

	inline bool row_input(Card& c, const char* id, const char* label, char* buffer, int size,
		const char* hint = "")
	{
		text_at(ImVec2(c.x, c.y + 3.f), label, style().caption, style().text_dim);
		ImGui::PushID(id);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9.f, 6.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(24 / 255.f, 27 / 255.f, 35 / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(31 / 255.f, 35 / 255.f, 44 / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(35 / 255.f, 39 / 255.f, 49 / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_Text, style().text);
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, style().text_dim);
		const float field_w = ImMin(c.width - 96.f, 190.f);
		ImGui::SetCursorScreenPos(ImVec2(c.x + c.width - field_w, c.y - 2.f));
		ImGui::SetNextItemWidth(field_w);
		const bool changed = ImGui::InputTextWithHint("##input", hint, buffer, size,
			ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::PopStyleColor(5);
		ImGui::PopStyleVar(2);
		ImGui::PopID();
		c.y += 32.f;
		return changed;
	}

	inline bool row_button(Card& c, const char* label)
	{
		const bool clicked = text_button(label, ImVec2(c.x, c.y), c.width, label);
		c.y += 40.f;
		return clicked;
	}

	inline void row_status(Card& c, const char* label, const char* value, bool good)
	{
		status_row(ImVec2(c.x, c.y + kRowPad), c.width, label, value, good);
		c.y += kRowPad * 2.f + 18.f + kRowGap;
	}

	inline int row_button_pair(Card& c, const char* id_a, const char* label_a,
		const char* id_b, const char* label_b, bool enabled = true)
	{
		const float half = (c.width - 10.f) * 0.5f;
		const bool a = text_button(id_a, ImVec2(c.x, c.y), half, label_a, enabled);
		const bool b = text_button(id_b, ImVec2(c.x + half + 10.f, c.y), half, label_b, enabled);
		c.y += 40.f;
		return a ? 1 : (b ? 2 : 0);
	}

	inline void row_text(Card& c, const char* label, const char* value)
	{
		const float row_h = text_h(style().body) + kRowPad * 2.f;
		const float label_h = text_h(style().body);
		text_at(ImVec2(c.x, c.y + (row_h - label_h) * 0.5f), label, style().body, style().text_dim);
		const float value_w = text_w(value, style().body);
		text_at(ImVec2(c.x + c.width - value_w, c.y + (row_h - label_h) * 0.5f), value,
			style().body, style().text);
		c.y += row_h + kRowGap;
	}

	inline void row_divider(Card& c)
	{
		divider(ImVec2(c.x, c.y + 4.f), ImVec2(c.x + c.width, c.y + 4.f));
		c.y += 14.f;
	}

	struct Grid
	{
		ImVec2 origin{};
		float  width = 0.f;
		float  gap   = kCardGap;
		float  y[2]  = { 0.f, 0.f };

		float col_w() const { return width; }
		ImVec2 col(int index) const
		{
			(void)index;
			return ImVec2(origin.x, origin.y + y[0]);
		}
		void advance(int index, float bottom)
		{
			(void)index;
			y[0] = bottom - origin.y + kRowGap;
			y[1] = y[0];
		}
		void row_end() {}
	};

	inline constexpr float kTabIcon = 20.f;

	inline float tab_pill_width(const char* label)
	{
		return 14.f + kTabIcon + 10.f + text_w(label, style().body) + 16.f;
	}

	inline bool tab_pill(const char* id, aether::icons::Id icon, const char* label,
		bool active, ImVec2 tab_min, float width, float height, float* hover_t)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(tab_min);
		const bool clicked = ImGui::InvisibleButton("##tab", ImVec2(width, height));

		const ImVec2 tab_max(tab_min.x + width, tab_min.y + height);
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const bool inside = mouse.x >= tab_min.x && mouse.x <= tab_max.x &&
		                    mouse.y >= tab_min.y && mouse.y <= tab_max.y;
		const bool hovered = ImGui::IsItemHovered() && inside;
		ImGui::PopID();

		const float ease = smooth_to(*hover_t, hovered ? 1.f : 0.f, 16.f, anim_delta());
		*hover_t = ease;
		const float amount = ImMax(ease, active ? 1.f : 0.f);

		ImDrawList* dl = ImGui::GetWindowDrawList();

		if (active)
			dl->AddRectFilled(tab_min, tab_max, IM_COL32(255, 255, 255, fade_alpha(18)), 12.f);

		const ImVec4 icon_color = fade(mix(style().text_dim, style().text, amount));
		const ImVec4 label_color = fade(mix(style().text_dim, style().text, amount));
		const float icon_h = kTabIcon;
		const float label_h = text_h(style().body);
		aether::icons::draw(dl, icon,
			ImVec2(std::floor(tab_min.x + 14.f + icon_h * 0.5f), std::floor((tab_min.y + tab_max.y) * 0.5f)),
			icon_h, ImGui::ColorConvertFloat4ToU32(icon_color), 0.f);
		text_at(ImVec2(tab_min.x + 14.f + kTabIcon + 10.f, (tab_min.y + tab_max.y - label_h) * 0.5f),
			label, style().body, label_color);
		return clicked;
	}

	inline constexpr float kSidebarIcon = 16.f;

	inline bool nav_page(const char* id, aether::icons::Nav icon, const char* label, bool active,
		ImVec2 item_min, float width, float height, float* hover_t)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(item_min);
		const bool clicked = ImGui::InvisibleButton("##page", ImVec2(width, height));

		const ImVec2 item_max(item_min.x + width, item_min.y + height);
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const bool inside = mouse.x >= item_min.x && mouse.x <= item_max.x &&
		                    mouse.y >= item_min.y && mouse.y <= item_max.y;
		const bool hovered = ImGui::IsItemHovered() && inside;
		ImGui::PopID();

		const float ease = smooth_to(*hover_t, (hovered && !active) ? 1.f : 0.f, 18.f, anim_delta());
		*hover_t = ease;

		ImDrawList* dl = ImGui::GetWindowDrawList();
		if (ease > 0.004f)
			dl->AddRectFilled(item_min, item_max, rgb(255, 255, 255, fade_alpha((int)(12.f * ease))), 11.f);

		const ImVec4 glyph_col = active
			? fade(ImVec4(24 / 255.f, 24 / 255.f, 27 / 255.f, 1.f))
			: fade(mix(style().text_dim, style().text, ease));
		const ImVec4 label_col = glyph_col;

		aether::icons::draw_nav(dl, icon,
			ImVec2(std::floor(item_min.x + 21.f), std::floor((item_min.y + item_max.y) * 0.5f)),
			kSidebarIcon, ImGui::ColorConvertFloat4ToU32(glyph_col));
		const float label_h = text_h(style().body);
		text_clipped(ImVec2(item_min.x + 39.f, (item_min.y + item_max.y - label_h) * 0.5f),
			label, style().body, label_col, width - 39.f - 10.f);
		return clicked;
	}

	inline constexpr float kSubTabPadX = 13.0f;
	inline constexpr float kSubTabGap  = 3.0f;

	inline float sub_tab_width(const char* label)
	{
		return text_w(label, style().caption) + kSubTabPadX * 2.f;
	}

	inline bool nav_sub(const char* id, const char* label, bool active,
		ImVec2 item_min, float width, float height, float* hover_t)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(item_min);
		const bool clicked = ImGui::InvisibleButton("##sub", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		const float ease = smooth_to(*hover_t, (hovered && !active) ? 1.f : 0.f, 18.f, anim_delta());
		*hover_t = ease;

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 item_max(item_min.x + width, item_min.y + height);
		if (ease > 0.004f)
			dl->AddRectFilled(item_min, item_max, rgb(255, 255, 255, fade_alpha((int)(12.f * ease))), 8.f);

		const ImVec4 col = active
			? fade(style().text)
			: fade(mix(style().text_dim, style().text, ease));
		const float label_h = text_h(style().caption);
		text_clipped(ImVec2(item_min.x + kSubTabPadX, (item_min.y + item_max.y - label_h) * 0.5f),
			label, style().caption, col, width - kSubTabPadX * 2.f);
		return clicked;
	}

	inline bool top_bar_item(const char* label, ImVec2 min, float height, bool active, float* hover_t)
	{
		ImGui::PushID(label);
		ImGui::SetCursorScreenPos(min);
		const float width = text_w(label, style().body) + 22.f;
		const bool clicked = ImGui::InvisibleButton("##topitem", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		const float ease = smooth_to(*hover_t, hovered ? 1.f : 0.f, 18.f, anim_delta());
		*hover_t = ease;
		const ImVec4 col = active ? style().text : mix(style().text_faint, style().text, ease);
		text_centered(ImVec2(min.x, min.y + (height - text_h(style().body)) * 0.5f), width, label,
			style().body, fade(col));
		return clicked;
	}

	inline bool rail_item(const char* id, aether::icons::Id icon, const char* label, bool active,
		ImVec2 item_min, float width, float height, float* hover_t)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(item_min);
		const bool clicked = ImGui::InvisibleButton("##rail", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		const float ease = smooth_to(*hover_t, (hovered || active) ? 1.f : 0.f, 16.f, anim_delta());
		*hover_t = ease;
		const float amount = ImMax(ease, active ? 1.f : 0.f);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 item_max(item_min.x + width, item_min.y + height);
		const int surface = fade_alpha((int)(ease * (active ? 20.f : 12.f)));
		if (surface > 0)
			dl->AddRectFilled(item_min, item_max, IM_COL32(255, 255, 255, surface), 10.f);

		const ImVec4 color = fade(mix(style().text_dim, style().text, amount));
		const float icon_h = 18.f;
		const float label_h = text_h(style().caption);
		aether::icons::draw(dl, icon,
			ImVec2(std::floor(item_min.x + 12.f + icon_h * 0.5f), std::floor((item_min.y + item_max.y) * 0.5f)),
			icon_h, ImGui::ColorConvertFloat4ToU32(color), 0.f);
		text_clipped(ImVec2(item_min.x + 12.f + icon_h + 10.f, (item_min.y + item_max.y - label_h) * 0.5f),
			label, style().caption, color, width - (12.f + icon_h + 10.f) - 8.f);
		return clicked;
	}

	inline bool top_bar_button(const char* id, aether::icons::Id icon, bool active, ImVec2 item_min, float side)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(item_min);
		const bool clicked = ImGui::InvisibleButton("##topbtn", ImVec2(side, side));
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 item_max(item_min.x + side, item_min.y + side);
		if (active)
			dl->AddRectFilled(item_min, item_max, rgb(255, 255, 255, 18), 11.f);
		else if (hovered)
			dl->AddRectFilled(item_min, item_max, rgb(255, 255, 255, 11), 11.f);
		aether::icons::draw(dl, icon,
			ImVec2(std::floor((item_min.x + item_max.x) * 0.5f), std::floor((item_min.y + item_max.y) * 0.5f)),
			19.f, rgb(255, 255, 255, active ? 245 : 185), 0.f);
		return clicked;
	}

	inline bool close_button(const char* id, ImVec2 origin, float side)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(origin);
		const bool clicked = ImGui::InvisibleButton("##close", ImVec2(side, side));
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		if (hovered)
			dl->AddRectFilled(origin, ImVec2(origin.x + side, origin.y + side), to_u32(ImVec4(0.91f, 0.31f, 0.28f, 0.9f)), 8.f);
		const ImVec2 center(origin.x + side * 0.5f, origin.y + side * 0.5f);
		const float arm = 5.f;
		const ImU32 color = rgb(255, 255, 255, hovered ? 255 : 200);
		dl->AddLine(ImVec2(center.x - arm, center.y - arm), ImVec2(center.x + arm, center.y + arm), color, 1.6f);
		dl->AddLine(ImVec2(center.x - arm, center.y + arm), ImVec2(center.x + arm, center.y - arm), color, 1.6f);
		return clicked;
	}
}
