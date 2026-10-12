#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <misc/freetype/imgui_freetype.h>

#include <settings.h>

#include <Windows.h>

#include <cmath>
#include <cstddef>

namespace aether
{
	namespace fonts
	{

		enum ResId : int
		{
			kResBody        = 2001,
			kResMedium      = 2002,
			kResIcons       = 2003,
			kResNav         = 2004,
			kResInter       = 2005,
			kResInterMedium = 2006,
			kResInterSemi   = 2007,
		};

		inline constexpr float kIconRasterSize = 16.f;

		inline ImFont*& body_font()    { static ImFont* f = nullptr; return f; }
		inline ImFont*& medium_font()  { static ImFont* f = nullptr; return f; }
		inline ImFont*& heading_font() { static ImFont* f = nullptr; return f; }
		inline ImFont*& caption_font() { static ImFont* f = nullptr; return f; }
		inline ImFont*& icon_font()    { static ImFont* f = nullptr; return f; }
		inline ImFont*& nav_font()     { static ImFont* f = nullptr; return f; }

		inline ImFont*& esp_display_font() { static ImFont* f = nullptr; return f; }

		inline ImFont*& esp_clear_font() { static ImFont* f = nullptr; return f; }

		inline ImFont*& esp_imgui_font() { static ImFont* f = nullptr; return f; }

		inline constexpr int kEspSizeFirst = 8;
		inline constexpr int kEspSizeLast  = 24;
		inline constexpr int kEspSizeCount = kEspSizeLast - kEspSizeFirst + 1;

		inline ImFont** esp_display_ladder() { static ImFont* f[kEspSizeCount]{}; return f; }
		inline ImFont** esp_clear_ladder()   { static ImFont* f[kEspSizeCount]{}; return f; }

		inline ImFont* esp_display(float size)
		{
			int i = (int)std::lround(size) - kEspSizeFirst;
			if (i < 0) i = 0;
			if (i >= kEspSizeCount) i = kEspSizeCount - 1;
			return esp_display_ladder()[i] ? esp_display_ladder()[i] : esp_display_font();
		}
		inline ImFont* esp_clear(float size)
		{
			int i = (int)std::lround(size) - kEspSizeFirst;
			if (i < 0) i = 0;
			if (i >= kEspSizeCount) i = kEspSizeCount - 1;
			return esp_clear_ladder()[i] ? esp_clear_ladder()[i] : esp_clear_font();
		}
		inline ImFont* esp_imgui() { return esp_imgui_font() ? esp_imgui_font() : body_font(); }

		inline ImFont*& menu_regular_font() { static ImFont* f = nullptr; return f; }
		inline ImFont*& menu_bold_font()    { static ImFont* f = nullptr; return f; }
		inline ImFont*& menu_title_font()   { static ImFont* f = nullptr; return f; }
		inline ImFont*& menu_small_font()   { static ImFont* f = nullptr; return f; }

		inline ImFont*& menu_display_font()      { static ImFont* f = nullptr; return f; }
		inline ImFont*& menu_display_bold_font() { static ImFont* f = nullptr; return f; }
		inline ImFont*& menu_value_font()        { static ImFont* f = nullptr; return f; }

		inline ImFont*& inter_regular_font() { static ImFont* f = nullptr; return f; }
		inline ImFont*& inter_small_font()   { static ImFont* f = nullptr; return f; }
		inline ImFont*& inter_medium_font()  { static ImFont* f = nullptr; return f; }
		inline ImFont*& inter_title_font()   { static ImFont* f = nullptr; return f; }
		inline ImFont*& inter_display_font()      { static ImFont* f = nullptr; return f; }
		inline ImFont*& inter_display_bold_font() { static ImFont* f = nullptr; return f; }
		inline ImFont*& inter_value_font()        { static ImFont* f = nullptr; return f; }

		inline ImFont*& inter_esp_font()     { static ImFont* f = nullptr; return f; }

		inline bool menu_uses_inter() { return settings::misc_ui::menu_font != 1; }

		inline ImFont* first_of(ImFont* a, ImFont* b, ImFont* c) { return a ? a : (b ? b : c); }

		inline ImFont* menu_regular()
		{
			return menu_uses_inter() ? first_of(inter_regular_font(), menu_regular_font(), body_font())
			                         : first_of(menu_regular_font(), inter_regular_font(), body_font());
		}
		inline ImFont* menu_bold()
		{
			return menu_uses_inter() ? first_of(inter_medium_font(), menu_bold_font(), inter_regular_font())
			                         : first_of(menu_bold_font(), inter_medium_font(), medium_font());
		}
		inline ImFont* menu_title()
		{
			return menu_uses_inter() ? first_of(inter_title_font(), menu_title_font(), inter_medium_font())
			                         : first_of(menu_title_font(), inter_title_font(), heading_font());
		}
		inline ImFont* menu_small()
		{
			return menu_uses_inter() ? first_of(inter_small_font(), menu_small_font(), inter_regular_font())
			                         : first_of(menu_small_font(), inter_small_font(), caption_font());
		}

		inline ImFont* menu_display()
		{
			return menu_uses_inter() ? first_of(inter_display_font(), menu_display_font(), inter_regular_font())
			                         : first_of(menu_display_font(), inter_display_font(), menu_regular_font());
		}
		inline ImFont* menu_display_bold()
		{
			return menu_uses_inter() ? first_of(inter_display_bold_font(), menu_display_bold_font(), inter_title_font())
			                         : first_of(menu_display_bold_font(), inter_display_bold_font(), menu_title_font());
		}
		inline ImFont* menu_value()
		{
			return menu_uses_inter() ? first_of(inter_value_font(), menu_value_font(), inter_title_font())
			                         : first_of(menu_value_font(), inter_value_font(), menu_title_font());
		}

		inline ImFont* esp_inter() { return first_of(inter_esp_font(), medium_font(), body_font()); }

		inline ImFont* inter_regular() { return inter_regular_font(); }
		inline ImFont* inter_title()   { return first_of(inter_title_font(), inter_medium_font(), medium_font()); }

		inline ImFont* body()    { return body_font(); }
		inline ImFont* medium()  { return medium_font(); }
		inline ImFont* heading() { return heading_font() ? heading_font() : medium_font(); }
		inline ImFont* caption() { return caption_font() ? caption_font() : body_font(); }
		inline ImFont* icons()   { return icon_font(); }
		inline ImFont* nav()     { return nav_font(); }

		inline ImFont* esp_display() { return esp_display_font() ? esp_display_font() : medium_font(); }
		inline ImFont* esp_clear()   { return esp_clear_font() ? esp_clear_font() : esp_display(); }

		inline const void* resource(int id, unsigned& size_out)
		{
			size_out = 0;

			HMODULE module = GetModuleHandleA(nullptr);
			HRSRC res = FindResourceA(module, MAKEINTRESOURCEA(id), RT_RCDATA);
			if (!res)
				return nullptr;

			HGLOBAL handle = LoadResource(module, res);
			if (!handle)
				return nullptr;

			const void* data = LockResource(handle);
			if (!data)
				return nullptr;

			size_out = SizeofResource(module, res);
			return size_out ? data : nullptr;
		}

		inline const char* utf8(ImWchar codepoint)
		{
			constexpr std::size_t kSlots = 8;
			static char slots[kSlots][4]{};
			static std::size_t next = 0;

			char* out = slots[next];
			next = (next + 1) % kSlots;

			if (codepoint < 0x80)
			{
				out[0] = (char)codepoint;
				out[1] = '\0';
			}
			else if (codepoint < 0x800)
			{
				out[0] = (char)(0xC0 | (codepoint >> 6));
				out[1] = (char)(0x80 | (codepoint & 0x3F));
				out[2] = '\0';
			}
			else
			{
				out[0] = (char)(0xE0 | (codepoint >> 12));
				out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
				out[2] = (char)(0x80 | (codepoint & 0x3F));
				out[3] = '\0';
			}
			return out;
		}

		inline bool load(ImFontAtlas* atlas, const ImWchar* icon_codepoints,
			const ImWchar* nav_codepoints = nullptr)
		{
			if (!atlas)
				return false;

			unsigned body_size = 0;
			unsigned medium_size = 0;
			unsigned icon_size = 0;
			unsigned nav_size = 0;
			unsigned inter_size = 0;
			unsigned inter_medium_size = 0;
			unsigned inter_semi_size = 0;
			const void* body_data = resource(kResBody, body_size);
			const void* medium_data = resource(kResMedium, medium_size);
			const void* icon_data = resource(kResIcons, icon_size);
			const void* nav_data = resource(kResNav, nav_size);
			const void* inter_data = resource(kResInter, inter_size);
			const void* inter_medium_data = resource(kResInterMedium, inter_medium_size);
			const void* inter_semi_data = resource(kResInterSemi, inter_semi_size);

			ImFontConfig cfg;
			cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
			cfg.OversampleH = cfg.OversampleV = 2;
			cfg.PixelSnapH = true;

			cfg.FontDataOwnedByAtlas = false;

			const ImWchar* text_ranges = atlas->GetGlyphRangesDefault();

			if (body_data)
			{
				body_font()    = atlas->AddFontFromMemoryTTF((void*)body_data,   (int)body_size,   14.0f, &cfg, text_ranges);
				caption_font() = atlas->AddFontFromMemoryTTF((void*)body_data,   (int)body_size,   12.5f, &cfg, text_ranges);
			}
			if (medium_data)
			{
				medium_font()  = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 14.0f, &cfg, text_ranges);
				heading_font() = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 16.5f, &cfg, text_ranges);

				ImFontConfig esp_cfg = cfg;
				esp_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
				esp_cfg.OversampleH = esp_cfg.OversampleV = 1;
				esp_display_font() = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 20.0f, &esp_cfg, text_ranges);

				ImFontConfig clear_cfg = cfg;
				clear_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_NoHinting;
				clear_cfg.OversampleH = clear_cfg.OversampleV = 1;
				esp_clear_font() = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 24.0f, &clear_cfg, text_ranges);

				for (int i = 0; i < kEspSizeCount; ++i)
				{
					const float px = (float)(kEspSizeFirst + i);
					esp_display_ladder()[i] = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, px, &esp_cfg, text_ranges);
					esp_clear_ladder()[i]   = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, px, &clear_cfg, text_ranges);
				}
			}

			if (inter_data)
			{
				inter_regular_font() = atlas->AddFontFromMemoryTTF((void*)inter_data, (int)inter_size, 13.0f, &cfg, text_ranges);
				inter_small_font()   = atlas->AddFontFromMemoryTTF((void*)inter_data, (int)inter_size, 11.0f, &cfg, text_ranges);
				inter_display_font() = atlas->AddFontFromMemoryTTF((void*)inter_data, (int)inter_size, 21.0f, &cfg, text_ranges);
			}
			if (inter_medium_data)
			{
				inter_medium_font() = atlas->AddFontFromMemoryTTF((void*)inter_medium_data, (int)inter_medium_size, 13.0f, &cfg, text_ranges);
				inter_value_font()  = atlas->AddFontFromMemoryTTF((void*)inter_medium_data, (int)inter_medium_size, 18.0f, &cfg, text_ranges);
			}
			if (inter_semi_data)
			{
				inter_title_font() = atlas->AddFontFromMemoryTTF((void*)inter_semi_data, (int)inter_semi_size, 15.0f, &cfg, text_ranges);
				inter_display_bold_font() = atlas->AddFontFromMemoryTTF((void*)inter_semi_data, (int)inter_semi_size, 21.0f, &cfg, text_ranges);

				ImFontConfig inter_esp_cfg = cfg;
				inter_esp_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
				inter_esp_cfg.OversampleH = inter_esp_cfg.OversampleV = 1;
				inter_esp_font() = atlas->AddFontFromMemoryTTF((void*)inter_semi_data, (int)inter_semi_size, 24.0f, &inter_esp_cfg, text_ranges);
			}

			esp_imgui_font() = atlas->AddFontDefault();

			if (body_data)
			{
				menu_regular_font() = atlas->AddFontFromMemoryTTF((void*)body_data, (int)body_size, 13.0f, &cfg, text_ranges);
				menu_small_font()   = atlas->AddFontFromMemoryTTF((void*)body_data, (int)body_size, 11.0f, &cfg, text_ranges);
				menu_display_font() = atlas->AddFontFromMemoryTTF((void*)body_data, (int)body_size, 21.0f, &cfg, text_ranges);
			}
			if (medium_data)
			{
				menu_bold_font()  = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 13.0f, &cfg, text_ranges);
				menu_title_font() = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 15.0f, &cfg, text_ranges);
				menu_display_bold_font() = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 21.0f, &cfg, text_ranges);
				menu_value_font()        = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 18.0f, &cfg, text_ranges);
			}
			if (icon_data && icon_codepoints && icon_codepoints[0])
			{

				ImFontConfig icon_cfg;
				icon_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
				icon_cfg.OversampleH = icon_cfg.OversampleV = 1;
				icon_cfg.PixelSnapH = true;
				icon_cfg.FontDataOwnedByAtlas = false;
				icon_font() = atlas->AddFontFromMemoryTTF((void*)icon_data, (int)icon_size,
					kIconRasterSize, &icon_cfg, icon_codepoints);
			}
			if (nav_data && nav_codepoints && nav_codepoints[0])
			{
				ImFontConfig nav_cfg;
				nav_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
				nav_cfg.OversampleH = nav_cfg.OversampleV = 1;
				nav_cfg.PixelSnapH = true;
				nav_cfg.FontDataOwnedByAtlas = false;
				nav_font() = atlas->AddFontFromMemoryTTF((void*)nav_data, (int)nav_size,
					kIconRasterSize, &nav_cfg, nav_codepoints);
			}

			if (!body_font())
			{
				body_font()    = atlas->AddFontDefault();
				caption_font() = body_font();
			}
			if (!caption_font()) caption_font() = body_font();
			if (!medium_font())  medium_font()  = body_font();
			if (!heading_font()) heading_font() = medium_font();
			if (!menu_regular_font()) menu_regular_font() = body_font();
			if (!menu_small_font())   menu_small_font()   = caption_font();
			if (!menu_bold_font())    menu_bold_font()    = medium_font();
			if (!menu_title_font())   menu_title_font()   = heading_font();

			if (!menu_display_font())      menu_display_font()      = menu_regular_font();
			if (!menu_display_bold_font()) menu_display_bold_font() = menu_title_font();
			if (!menu_value_font())        menu_value_font()        = menu_bold_font();

			if (!inter_regular_font()) inter_regular_font() = menu_regular_font();
			if (!inter_small_font())   inter_small_font()   = inter_regular_font();
			if (!inter_medium_font())  inter_medium_font()  = menu_bold_font();
			if (!inter_title_font())   inter_title_font()   = inter_medium_font();
			if (!inter_display_font())      inter_display_font()      = menu_display_font();
			if (!inter_display_bold_font()) inter_display_bold_font() = menu_display_bold_font();
			if (!inter_value_font())        inter_value_font()        = menu_value_font();
			if (!inter_esp_font())     inter_esp_font()     = esp_display_font();
			if (!esp_display_font())  esp_display_font()  = medium_font();
			if (!esp_clear_font())    esp_clear_font()    = esp_display_font();
			if (!esp_imgui_font())    esp_imgui_font()    = body_font();
			for (int i = 0; i < kEspSizeCount; ++i)
			{
				if (!esp_display_ladder()[i]) esp_display_ladder()[i] = medium_font();
				if (!esp_clear_ladder()[i])   esp_clear_ladder()[i]   = esp_display_ladder()[i];
			}

			atlas->Build();
			ImGui::GetIO().FontDefault = body_font();
			return body_font() != nullptr;
		}

		inline bool draw_glyph(ImDrawList* dl, ImFont* font, ImWchar codepoint, const ImVec2& centre,
			float size, ImU32 col)
		{
			if (!dl || !font || !codepoint || size <= 0.f)
				return false;

			const ImFontGlyph* glyph = font->FindGlyphNoFallback(codepoint);
			if (!glyph)
				return false;

			const float scale = size / font->FontSize;
			const ImVec2 pos(centre.x - (glyph->X0 + glyph->X1) * 0.5f * scale,
				centre.y - (glyph->Y0 + glyph->Y1) * 0.5f * scale);

			dl->AddText(font, size, pos, col, utf8(codepoint));
			return true;
		}

		inline bool draw_icon(ImDrawList* dl, ImWchar codepoint, const ImVec2& centre, float size, ImU32 col)
		{
			return draw_glyph(dl, icon_font(), codepoint, centre, size, col);
		}

		inline bool draw_nav_icon(ImDrawList* dl, ImWchar codepoint, const ImVec2& centre, float size, ImU32 col)
		{
			return draw_glyph(dl, nav_font(), codepoint, centre, size, col);
		}
	}
}
