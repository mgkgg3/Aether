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
#include <cstdio>
#include <deque>
#include <vector>

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

		// ---------------------------------------------------------------- faces
		//
		// The menu's typeface is a choice, not a constant.  Every face is built
		// once, at its own sizes, and the picker only moves a pointer - so
		// changing the type is instant and never rebuilds the atlas mid-frame.
		//
		// Inter and Geist are compiled in.  The rest are read from the system's
		// font folder at start-up and skipped without a trace when a machine
		// does not have them, which is what keeps the list a menu rather than a
		// dependency.
		enum MenuFace : int
		{
			kFaceInter = 0,
			kFaceGeist,
			kFaceSegoe,
			kFaceTrebuchet,
			kFaceVerdana,
			kFaceTahoma,
			kFaceConsolas,
			kFaceCascadia,
			kFaceCount
		};

		inline const char* menu_face_name(int i)
		{
			static const char* const names[kFaceCount] = {
				"Inter", "Geist", "Segoe UI", "Trebuchet",
				"Verdana", "Tahoma", "Consolas", "Cascadia Mono"
			};
			return (i >= 0 && i < kFaceCount) ? names[i] : names[0];
		}

		struct face_fonts
		{
			ImFont* f_regular      = nullptr;
			ImFont* f_bold         = nullptr;
			ImFont* f_title        = nullptr;
			ImFont* f_small        = nullptr;
			ImFont* f_display      = nullptr;
			ImFont* f_display_bold = nullptr;
			ImFont* f_value        = nullptr;
		};

		inline face_fonts* face_table() { static face_fonts f[kFaceCount]; return f; }
		inline face_fonts& face(int i)
		{
			if (i < 0) i = 0;
			if (i >= kFaceCount) i = kFaceCount - 1;
			return face_table()[i];
		}

		inline int menu_face()
		{
			int i = settings::misc_ui::menu_font;
			if (i < 0) i = 0;
			if (i >= kFaceCount) i = kFaceCount - 1;
			return i;
		}

		inline bool menu_uses_inter() { return menu_face() == kFaceInter; }

		inline ImFont* first_of(ImFont* a, ImFont* b, ImFont* c) { return a ? a : (b ? b : c); }

		inline ImFont* menu_regular()
		{
			ImFont* f = face(menu_face()).f_regular;
			return f ? f : first_of(inter_regular_font(), menu_regular_font(), body_font());
		}
		inline ImFont* menu_bold()
		{
			ImFont* f = face(menu_face()).f_bold;
			return f ? f : first_of(inter_medium_font(), menu_bold_font(), inter_regular_font());
		}
		inline ImFont* menu_title()
		{
			ImFont* f = face(menu_face()).f_title;
			return f ? f : first_of(inter_title_font(), menu_title_font(), inter_medium_font());
		}
		inline ImFont* menu_small()
		{
			ImFont* f = face(menu_face()).f_small;
			return f ? f : first_of(inter_small_font(), menu_small_font(), inter_regular_font());
		}

		inline ImFont* menu_display()
		{
			ImFont* f = face(menu_face()).f_display;
			return f ? f : first_of(inter_display_font(), menu_display_font(), menu_regular());
		}
		inline ImFont* menu_display_bold()
		{
			ImFont* f = face(menu_face()).f_display_bold;
			return f ? f : first_of(inter_display_bold_font(), menu_display_bold_font(), menu_title());
		}
		inline ImFont* menu_value()
		{
			ImFont* f = face(menu_face()).f_value;
			return f ? f : first_of(inter_value_font(), menu_value_font(), menu_title());
		}

		inline ImFont* esp_inter() { return first_of(inter_esp_font(), medium_font(), body_font()); }

		// The other two Inter cuts as ESP fonts: the same family as `esp_inter()`
		// at a heavier weight, for names and distances that have to hold up over a
		// busy scene.  Both fall back through the family rather than to nothing.
		inline ImFont* esp_inter_medium(){ return first_of(inter_medium_font(), inter_esp_font(), medium_font()); }
		inline ImFont* esp_inter_title() { return first_of(inter_title_font(),  inter_medium_font(), esp_inter_medium()); }

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

		// A system font, read into memory once.  The atlas only borrows the bytes
		// (FontDataOwnedByAtlas is off), so the buffer has to outlive it - a deque
		// never moves what it holds, and a moved vector keeps its own heap.
		inline const void* system_ttf(const char* file, unsigned& size_out)
		{
			size_out = 0;
			static std::deque<std::vector<unsigned char>> owned;

			char dir[MAX_PATH]{};
			const UINT n = ::GetWindowsDirectoryA(dir, MAX_PATH);
			if (!n || n >= MAX_PATH)
				return nullptr;

			char path[MAX_PATH * 2]{};
			snprintf(path, sizeof path, "%s\\Fonts\\%s", dir, file);

			FILE* f = nullptr;
			if (fopen_s(&f, path, "rb") != 0 || !f)
				return nullptr;

			fseek(f, 0, SEEK_END);
			const long len = ftell(f);
			fseek(f, 0, SEEK_SET);
			if (len <= 0) { fclose(f); return nullptr; }

			owned.emplace_back();
			std::vector<unsigned char>& buf = owned.back();
			buf.resize((size_t)len);
			const size_t got = fread(buf.data(), 1, (size_t)len, f);
			fclose(f);
			if (got != (size_t)len) { owned.pop_back(); return nullptr; }

			size_out = (unsigned)len;
			return buf.data();
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

			// The menu's text is rasterized 1:1 and pixel-snapped.
			//
			// This is the opposite of the usual advice, and it is what the panel
			// needs: it is drawn at exactly one pixel per texel on a solid plate, so
			// every texel in the atlas is a pixel on screen.  Anything that makes
			// the glyph sample more texels than it has pixels - horizontal
			// oversampling, a rasterization density above 1 - is averaged back down
			// by the sampler's bilinear filter, and that average is *blur*: a 13px
			// stem arriving as two half-lit columns reads as a soft, low-resolution
			// font no matter how many samples went into it.  Oversample 1 plus
			// PixelSnapH is a glyph rendered into the pixel grid it is shown on.
			//
			// RasterizerMultiply is the one addition: at these sizes hinting leaves
			// the thin strokes at about half alpha, and a six percent lift gives the
			// small labels their weight back without smearing them.  The ESP keeps
			// the roomy settings - its labels are scaled and drawn over a moving 3D
			// scene, where a little softness is the cheaper trade.
			ImFontConfig menu_cfg = cfg;
			menu_cfg.OversampleH = menu_cfg.OversampleV = 1;
			menu_cfg.PixelSnapH  = true;
			menu_cfg.RasterizerDensity  = 1.f;
			// Ten percent, up from six.  At twelve pixels hinting leaves the thin
			// strokes of an Inter stem at half coverage, and half coverage on a
			// dark plate is the grey, dusty look the captions had.  This lifts the
			// stems back to a full column without smearing the counters.
			menu_cfg.RasterizerMultiply = 1.10f;

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
				// Fourteen, not thirteen: the panel's rows are read at a glance and
				// Inter's x-height at 13 is where its counters start closing up.  The
				// caption steps up with it (13, from 11 - eleven pixels was the
				// pixelation itself).
				inter_regular_font() = atlas->AddFontFromMemoryTTF((void*)inter_data, (int)inter_size, 14.0f, &menu_cfg, text_ranges);
				inter_small_font()   = atlas->AddFontFromMemoryTTF((void*)inter_data, (int)inter_size, 13.0f, &menu_cfg, text_ranges);
				inter_display_font() = atlas->AddFontFromMemoryTTF((void*)inter_data, (int)inter_size, 21.0f, &menu_cfg, text_ranges);
			}
			if (inter_medium_data)
			{
				inter_medium_font() = atlas->AddFontFromMemoryTTF((void*)inter_medium_data, (int)inter_medium_size, 14.0f, &menu_cfg, text_ranges);
				inter_value_font()  = atlas->AddFontFromMemoryTTF((void*)inter_medium_data, (int)inter_medium_size, 18.0f, &menu_cfg, text_ranges);
			}
			if (inter_semi_data)
			{
				inter_title_font() = atlas->AddFontFromMemoryTTF((void*)inter_semi_data, (int)inter_semi_size, 16.0f, &menu_cfg, text_ranges);
				inter_display_bold_font() = atlas->AddFontFromMemoryTTF((void*)inter_semi_data, (int)inter_semi_size, 21.0f, &menu_cfg, text_ranges);

				ImFontConfig inter_esp_cfg = cfg;
				inter_esp_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
				inter_esp_cfg.OversampleH = inter_esp_cfg.OversampleV = 1;
				inter_esp_font() = atlas->AddFontFromMemoryTTF((void*)inter_semi_data, (int)inter_semi_size, 24.0f, &inter_esp_cfg, text_ranges);
			}

			// Every face the picker offers, one pass.  The two compiled-in families
			// come from the resources already loaded above; the system ones are read
			// from the font folder and fall back to Inter when a file is missing, so
			// a choice is always a real font and never a blank row.
			{
				const auto add_face = [&](int i, const void* reg, unsigned rn,
					const void* bold, unsigned bn)
				{
					if (!reg || !rn) return;
					face_fonts& f = face(i);
					f.f_regular = atlas->AddFontFromMemoryTTF((void*)reg, (int)rn, 14.0f, &menu_cfg, text_ranges);
					f.f_small   = atlas->AddFontFromMemoryTTF((void*)reg, (int)rn, 13.0f, &menu_cfg, text_ranges);
					f.f_display = atlas->AddFontFromMemoryTTF((void*)reg, (int)rn, 21.0f, &menu_cfg, text_ranges);

					const void* bd = (bold && bn) ? bold : reg;
					const unsigned bdn = (bold && bn) ? bn : rn;
					f.f_bold         = atlas->AddFontFromMemoryTTF((void*)bd, (int)bdn, 14.0f, &menu_cfg, text_ranges);
					f.f_title        = atlas->AddFontFromMemoryTTF((void*)bd, (int)bdn, 16.0f, &menu_cfg, text_ranges);
					f.f_display_bold = atlas->AddFontFromMemoryTTF((void*)bd, (int)bdn, 21.0f, &menu_cfg, text_ranges);
					f.f_value        = atlas->AddFontFromMemoryTTF((void*)bd, (int)bdn, 18.0f, &menu_cfg, text_ranges);
				};

				add_face(kFaceInter, inter_data, inter_size, inter_semi_data, inter_semi_size);
				add_face(kFaceGeist, body_data, body_size, medium_data, medium_size);

				const auto sys_face = [&](int i, const char* reg, const char* bold)
				{
					unsigned rn = 0, bn = 0;
					const void* r = system_ttf(reg, rn);
					const void* b = (bold && bold[0]) ? system_ttf(bold, bn) : nullptr;
					add_face(i, r, rn, b, bn);
				};
				sys_face(kFaceSegoe,    "segoeui.ttf",      "seguisb.ttf");
				sys_face(kFaceTrebuchet,"trebuc.ttf",       "trebucbd.ttf");
				sys_face(kFaceVerdana,  "verdana.ttf",      "verdanab.ttf");
				sys_face(kFaceTahoma,   "tahoma.ttf",       "tahomabd.ttf");
				sys_face(kFaceConsolas, "consola.ttf",      "consolab.ttf");
				sys_face(kFaceCascadia, "CascadiaMono.ttf", nullptr);
			}

			esp_imgui_font() = atlas->AddFontDefault();			if (body_data)
			{
				// A step up from 13 / 11.  Eleven pixels is below the size where a
				// x-height survives hinting: the captions were legible but they read
				// as pixelated, and "make it high definition" is really "do not draw
				// text at eleven pixels".  Twelve is the smallest the atlas holds
				// now, and the rows that hold it measure themselves from it.
				menu_regular_font() = atlas->AddFontFromMemoryTTF((void*)body_data,   (int)body_size, 14.0f, &menu_cfg, text_ranges);
				menu_small_font()   = atlas->AddFontFromMemoryTTF((void*)body_data,   (int)body_size, 13.0f, &menu_cfg, text_ranges);
				menu_display_font() = atlas->AddFontFromMemoryTTF((void*)body_data,   (int)body_size, 21.0f, &menu_cfg, text_ranges);
			}
			if (medium_data)
			{
				menu_bold_font()  = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 14.0f, &menu_cfg, text_ranges);
				menu_title_font() = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 16.0f, &menu_cfg, text_ranges);
				menu_display_bold_font() = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 21.0f, &menu_cfg, text_ranges);
				menu_value_font()        = atlas->AddFontFromMemoryTTF((void*)medium_data, (int)medium_size, 18.0f, &menu_cfg, text_ranges);
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
			{
				// A face that could not be read is Inter's set, not nothing: the
				// picker stays honest and the menu never draws a null font.
				for (int i = 0; i < kFaceCount; ++i)
				{
					face_fonts& f = face(i);
					if (!f.f_regular)      f.f_regular      = menu_regular_font();
					if (!f.f_bold)         f.f_bold         = menu_bold_font();
					if (!f.f_title)        f.f_title        = menu_title_font();
					if (!f.f_small)        f.f_small        = menu_small_font();
					if (!f.f_display)      f.f_display      = menu_display_font();
					if (!f.f_display_bold) f.f_display_bold = menu_display_bold_font();
					if (!f.f_value)        f.f_value        = menu_value_font();
				}
			}

			if (!inter_esp_font())     inter_esp_font()     = esp_display_font();
			if (!esp_display_font())  esp_display_font()  = medium_font();
			if (!esp_clear_font())    esp_clear_font()    = esp_display_font();
			if (!esp_imgui_font())    esp_imgui_font()    = body_font();
			for (int i = 0; i < kEspSizeCount; ++i)
			{
				if (!esp_display_ladder()[i]) esp_display_ladder()[i] = medium_font();
				if (!esp_clear_ladder()[i])   esp_clear_ladder()[i]   = esp_display_ladder()[i];
			}

			// The atlas gets a 4096 wide sheet and no baked lines.  Nothing here
			// needs more video memory for its own sake - the sheet only grows to the
			// height the glyphs actually use - but at 4096 the menu ladder, the
			// icon faces and the ESP font all fit without the packer shrinking any
			// of them, and a glyph that fits is a glyph rasterized at its own size
			// rather than a glyph that came out of a resampled atlas.
			if (atlas->TexDesiredWidth < 4096)
				atlas->TexDesiredWidth = 4096;
			atlas->Flags |= ImFontAtlasFlags_NoBakedLines;

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
