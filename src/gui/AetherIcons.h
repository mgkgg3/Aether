#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <gui/AetherFonts.h>

#include <cmath>

namespace aether
{
	namespace icons
	{
		enum class Id : int
		{
			None = 0,

			Home, Cog, Config, FolderGlyph, Terminal, Music, Settings, Tree,

			Crosshair, Eye, Wrench, Globe,

			Search, Close, ChevronDown, ChevronRight, ChevronUp, ChevronLeft,
			Play, Pause, Stop, Refresh, Trash, Save, Plus, Minus, Check,
			Lock, Download, Upload, Sliders, Cursor,

			Sun, Moon, Snowflake, Bolt, Shield, Target, Star, Sparkle, Flame,
			Ghost, Crown, Dice, Wand, Layers, Heart, Info, Warning, Dot, Logo,

			World, Movement, Palette, Map, Users, Hammer, Toolbox, Gauge, Rocket,
			Chip, Fingerprint, Cube, Camera, Code, Bug, Bars, Brush, Swatch,
			ShieldHalf, UserShield, EyeSlash, Satellite, Route, Sitemap, Droplet,
			Filter, Table, Keyboard, Display, Volume, Shirt, Footprints, Nodes,

			COUNT
		};

		enum class Nav : int
		{
			None = 0,

			Aim,
			Eye,
			Globe,
			Weapon,
			Sliders,
			Gear,

			Users,
			Sitemap,
			Code,
			Terminal,
			Tree,
			Satellite,
			Gauge,
			Palette,
			Route,
			Cube,
			Bolt,

			COUNT
		};

		struct NavGlyph
		{
			bool    pack;
			ImWchar codepoint;
		};

		inline ImWchar nav_codepoint(Nav id)
		{
			switch (id)
			{
			case Nav::Aim:     return 0xF000;
			case Nav::Eye:     return 0xF001;
			case Nav::Globe:   return 0xF002;
			case Nav::Weapon:  return 0xF003;
			case Nav::Sliders: return 0xF004;
			case Nav::Gear:    return 0xF005;
			default:           return 0;
			}
		}

		namespace detail
		{

			struct Ctx
			{
				ImDrawList* dl;
				ImVec2      c;
				float       sz;
				ImU32       col;
				float       t;
			};

			inline ImVec2 pt(const Ctx& d, float x, float y)
			{
				return ImVec2(d.c.x + x * d.sz * 0.5f, d.c.y + y * d.sz * 0.5f);
			}

			inline void seg(const Ctx& d, float x0, float y0, float x1, float y1)
			{
				d.dl->AddLine(pt(d, x0, y0), pt(d, x1, y1), d.col, d.t);
			}

			inline void poly(const Ctx& d, const ImVec2* v, int n, bool closed)
			{
				d.dl->AddPolyline(v, n, d.col, closed ? ImDrawFlags_Closed : 0, d.t);
			}

			inline void circle(const Ctx& d, float x, float y, float r)
			{
				d.dl->AddCircle(pt(d, x, y), r * d.sz * 0.5f, d.col, 0, d.t);
			}

			inline void disc(const Ctx& d, float x, float y, float r)
			{
				d.dl->AddCircleFilled(pt(d, x, y), r * d.sz * 0.5f, d.col, 0);
			}

			inline void box(const Ctx& d, float x0, float y0, float x1, float y1, float round = 0.f)
			{
				d.dl->AddRect(pt(d, x0, y0), pt(d, x1, y1), d.col, round, 0, d.t);
			}

			inline void boxfill(const Ctx& d, float x0, float y0, float x1, float y1, float round = 0.f)
			{
				d.dl->AddRectFilled(pt(d, x0, y0), pt(d, x1, y1), d.col, round);
			}

			inline void tri(const Ctx& d, float x0, float y0, float x1, float y1, float x2, float y2)
			{
				d.dl->AddTriangle(pt(d, x0, y0), pt(d, x1, y1), pt(d, x2, y2), d.col, d.t);
			}

			inline void trifill(const Ctx& d, float x0, float y0, float x1, float y1, float x2, float y2)
			{
				d.dl->AddTriangleFilled(pt(d, x0, y0), pt(d, x1, y1), pt(d, x2, y2), d.col);
			}

			inline void arc(const Ctx& d, float x, float y, float r, float a0, float a1)
			{
				d.dl->PathArcTo(pt(d, x, y), r * d.sz * 0.5f, a0 * IM_PI * 2.f, a1 * IM_PI * 2.f, 24);
				d.dl->PathStroke(d.col, 0, d.t);
			}

			inline void cubic(const Ctx& d, float x0, float y0, float c0x, float c0y,
				float c1x, float c1y, float x1, float y1)
			{
				d.dl->AddBezierCubic(pt(d, x0, y0), pt(d, c0x, c0y), pt(d, c1x, c1y), pt(d, x1, y1), d.col, d.t);
			}

			inline void star(const Ctx& d, float r_out, float r_in, bool filled)
			{

				const float yo = 0.0955f * r_out;
				ImVec2 v[10];
				for (int i = 0; i < 10; ++i)
				{
					const float a = -IM_PI * 0.5f + (float)i * IM_PI / 5.f;
					const float r = (i & 1) ? r_in : r_out;
					v[i] = pt(d, cosf(a) * r, sinf(a) * r + yo);
				}

				if (filled)
				{
					const ImVec2 core[5] = { v[1], v[3], v[5], v[7], v[9] };
					d.dl->AddConvexPolyFilled(core, 5, d.col);
					for (int i = 0; i < 10; i += 2)
					{
						const int prev = (i + 9) % 10;
						const int next = (i + 1) % 10;
						d.dl->AddTriangleFilled(v[prev], v[i], v[next], d.col);
					}
				}
				else
				{
					d.dl->AddPolyline(v, 10, d.col, ImDrawFlags_Closed, d.t);
				}
			}
		}

		inline float optical_scale(Id id)
		{
			switch (id)
			{
			case Id::Cog:         return 1.02f;
			case Id::Config:      return 0.93f;
			case Id::FolderGlyph: return 0.93f;
			case Id::Wrench:      return 1.05f;
			case Id::Cursor:      return 0.91f;
			case Id::Star:        return 0.89f;
			case Id::Flame:       return 0.91f;
			case Id::Layers:      return 0.93f;
			case Id::Warning:     return 0.93f;
			case Id::Crosshair:   return 0.94f;
			case Id::Eye:         return 0.87f;
			case Id::Bolt:        return 0.86f;
			case Id::Sparkle:     return 0.88f;
			case Id::Logo:        return 0.88f;
			case Id::Music:       return 1.04f;
			case Id::Tree:        return 1.15f;
			case Id::Close:       return 1.24f;
			case Id::ChevronDown: return 1.24f;
			case Id::ChevronRight:return 1.24f;
			case Id::ChevronUp:   return 1.24f;
			case Id::ChevronLeft: return 1.24f;
			case Id::Plus:        return 1.27f;
			case Id::Minus:       return 1.27f;
			case Id::Play:        return 1.06f;
			case Id::Pause:       return 1.15f;
			case Id::Stop:        return 1.35f;
			case Id::Check:       return 1.15f;
			case Id::Lock:        return 1.10f;
			case Id::Heart:       return 1.02f;
			case Id::Refresh:     return 1.07f;
			case Id::Sliders:     return 1.08f;
			case Id::Moon:        return 1.07f;
			case Id::Download:    return 1.05f;
			case Id::Upload:      return 1.05f;
			case Id::Ghost:       return 1.03f;
			case Id::Trash:       return 1.02f;
			case Id::Save:        return 1.00f;
			case Id::Sun:         return 1.00f;
			case Id::Snowflake:   return 1.00f;
			case Id::Settings:    return 1.00f;
			case Id::Home:        return 0.97f;
			case Id::Terminal:    return 0.95f;
			case Id::Globe:       return 0.96f;
			case Id::Shield:      return 0.99f;
			case Id::Target:      return 0.95f;
			case Id::Search:      return 0.99f;
			case Id::Wand:        return 0.95f;
			case Id::Info:        return 0.98f;
			case Id::Crown:       return 0.96f;
			case Id::Dice:        return 1.02f;
			default:              return 1.00f;
			}
		}

		inline ImWchar codepoint(Id id)
		{
			switch (id)
			{

			case Id::Home:        return 0xF015;
			case Id::Cog:         return 0xF013;
			case Id::Config:      return 0xF013;
			case Id::FolderGlyph: return 0xF07C;
			case Id::Terminal:    return 0xF120;
			case Id::Music:       return 0xF001;
			case Id::Settings:    return 0xF1DE;
			case Id::Tree:        return 0xF1BB;

			case Id::Crosshair:   return 0xF05B;
			case Id::Eye:         return 0xF06E;
			case Id::Wrench:      return 0xF0AD;
			case Id::Globe:       return 0xF0AC;

			case Id::Search:      return 0xF002;
			case Id::Close:       return 0xF00D;
			case Id::ChevronDown: return 0xF078;
			case Id::ChevronRight:return 0xF054;
			case Id::ChevronUp:   return 0xF077;
			case Id::ChevronLeft: return 0xF053;
			case Id::Play:        return 0xF04B;
			case Id::Pause:       return 0xF04C;
			case Id::Stop:        return 0xF04D;
			case Id::Refresh:     return 0xF021;
			case Id::Trash:       return 0xF1F8;
			case Id::Save:        return 0xF0C7;
			case Id::Plus:        return 0xF055;
			case Id::Minus:       return 0xF056;
			case Id::Check:       return 0xF00C;
			case Id::Lock:        return 0xF023;
			case Id::Download:    return 0xF019;
			case Id::Upload:      return 0xF093;
			case Id::Sliders:     return 0xF1DE;
			case Id::Cursor:      return 0xF245;

			case Id::Sun:         return 0xF185;
			case Id::Moon:        return 0xF186;
			case Id::Snowflake:   return 0xF2DC;
			case Id::Bolt:        return 0xF0E7;
			case Id::Shield:      return 0xF132;
			case Id::Target:      return 0xF140;
			case Id::Star:        return 0xF005;
			case Id::Sparkle:     return 0xE2CA;
			case Id::Flame:       return 0xF06D;
			case Id::Ghost:       return 0xF6E2;
			case Id::Crown:       return 0xF521;
			case Id::Dice:        return 0xF522;
			case Id::Wand:        return 0xE2CA;
			case Id::Layers:      return 0xF5FD;
			case Id::Heart:       return 0xF004;
			case Id::Info:        return 0xF05A;
			case Id::Warning:     return 0xF071;
			case Id::Dot:         return 0xF10C;

			case Id::Logo:        return 0xE699;

			case Id::World:       return 0xF57D;
			case Id::Movement:    return 0xF70C;
			case Id::Palette:     return 0xF53F;
			case Id::Map:         return 0xF278;
			case Id::Users:       return 0xF0C0;
			case Id::Hammer:      return 0xF6E3;
			case Id::Toolbox:     return 0xF552;
			case Id::Gauge:       return 0xF0E4;
			case Id::Rocket:      return 0xF135;
			case Id::Chip:        return 0xF2DB;
			case Id::Fingerprint: return 0xF577;
			case Id::Cube:        return 0xF1B2;
			case Id::Camera:      return 0xF030;
			case Id::Code:        return 0xF121;
			case Id::Bug:         return 0xF188;
			case Id::Bars:        return 0xF0C9;
			case Id::Brush:       return 0xF1FC;
			case Id::Swatch:      return 0xF5C3;
			case Id::ShieldHalf:  return 0xF3ED;
			case Id::UserShield:  return 0xF505;
			case Id::EyeSlash:    return 0xF070;
			case Id::Satellite:   return 0xF7C0;
			case Id::Route:       return 0xF4D7;
			case Id::Sitemap:     return 0xF0E8;
			case Id::Droplet:     return 0xF043;
			case Id::Filter:      return 0xF0B0;
			case Id::Table:       return 0xF0CE;
			case Id::Keyboard:    return 0xF11C;
			case Id::Display:     return 0xE163;
			case Id::Volume:      return 0xF028;
			case Id::Shirt:       return 0xF553;
			case Id::Footprints:  return 0xF54B;
			case Id::Nodes:       return 0xE4E2;

			default:              return 0;
			}
		}

		inline const ImWchar* all_codepoints()
		{
			static ImWchar ranges[(int)Id::COUNT * 2 + 1]{};
			static bool built = false;

			if (!built)
			{
				int count = 0;
				for (int i = 0; i < (int)Id::COUNT; ++i)
				{
					const ImWchar cp = codepoint((Id)i);
					if (!cp)
						continue;

					bool seen = false;
					for (int j = 0; j < count; ++j)
					{
						if (ranges[j * 2] == cp) { seen = true; break; }
					}
					if (seen)
						continue;

					ranges[count * 2]     = cp;
					ranges[count * 2 + 1] = cp;
					++count;
				}
				ranges[count * 2] = 0;
				built = true;
			}

			return ranges;
		}

		inline int codepoint_count()
		{
			const ImWchar* ranges = all_codepoints();
			int count = 0;
			while (ranges[count * 2])
				++count;
			return count;
		}

		inline Id fa_fallback(Nav id)
		{
			switch (id)
			{
			case Nav::Aim:      return Id::Crosshair;
			case Nav::Eye:      return Id::Eye;
			case Nav::Globe:    return Id::Globe;
			case Nav::Weapon:   return Id::Target;
			case Nav::Sliders:  return Id::Sliders;
			case Nav::Gear:     return Id::Cog;
			case Nav::Users:    return Id::Users;
			case Nav::Sitemap:  return Id::Sitemap;
			case Nav::Code:     return Id::Code;
			case Nav::Terminal: return Id::Terminal;
			case Nav::Tree:     return Id::Tree;
			case Nav::Satellite:return Id::Satellite;
			case Nav::Gauge:    return Id::Gauge;
			case Nav::Palette:  return Id::Palette;
			case Nav::Route:    return Id::Route;
			case Nav::Cube:     return Id::Cube;
			case Nav::Bolt:     return Id::Bolt;
			default:            return Id::Dot;
			}
		}

		inline NavGlyph nav_glyph(Nav id)
		{
			return NavGlyph{ false, codepoint(fa_fallback(id)) };
		}

		inline float nav_optical(Nav id)
		{
			return nav_codepoint(id) ? 1.00f : 0.92f;
		}

		inline const ImWchar* nav_codepoints()
		{
			static ImWchar ranges[(int)Nav::COUNT * 2 + 1]{};
			static bool built = false;

			if (!built)
			{
				int count = 0;
				for (int i = 0; i < (int)Nav::COUNT; ++i)
				{
					const ImWchar cp = nav_codepoint((Nav)i);
					if (!cp)
						continue;
					ranges[count * 2]     = cp;
					ranges[count * 2 + 1] = cp;
					++count;
				}
				ranges[count * 2] = 0;
				built = true;
			}

			return ranges;
		}

		inline void draw(ImDrawList* dl, Id id, const ImVec2& centre, float sz, ImU32 col, float thickness = 0.f)
		{
			if (!dl || id == Id::None || sz <= 0.f)
				return;

			if (fonts::draw_icon(dl, codepoint(id), centre, sz, col))
				return;

			detail::Ctx d;
			d.dl  = dl;
			d.c   = ImVec2(centre.x - 0.5f, centre.y - 0.5f);
			d.sz  = sz * optical_scale(id);
			d.col = col;
			d.t   = thickness > 0.f ? thickness : ImMax(1.f, d.sz * 0.085f);

			using namespace detail;

			switch (id)
			{
			case Id::Home:
			{
				const ImVec2 roof[3] = { pt(d, -0.82f, 0.02f), pt(d, 0.f, -0.72f), pt(d, 0.82f, 0.02f) };
				poly(d, roof, 3, false);
				const ImVec2 body[4] = { pt(d, -0.52f, 0.06f), pt(d, -0.52f, 0.72f), pt(d, 0.52f, 0.72f), pt(d, 0.52f, 0.06f) };
				poly(d, body, 4, false);
				box(d, -0.14f, 0.30f, 0.14f, 0.72f, d.t * 0.5f);
				break;
			}
			case Id::Cog:
			case Id::Config:
			case Id::FolderGlyph:
			{
				const ImVec2 f[7] = {
					pt(d, -0.82f, -0.55f), pt(d, -0.28f, -0.55f), pt(d, -0.12f, -0.30f),
					pt(d, 0.82f, -0.30f),  pt(d, 0.82f, 0.62f),  pt(d, -0.82f, 0.62f),
					pt(d, -0.82f, -0.55f)
				};
				poly(d, f, 7, false);
				break;
			}
			case Id::Terminal:
			{
				box(d, -0.82f, -0.66f, 0.82f, 0.66f, d.t);
				const ImVec2 caret[3] = { pt(d, -0.46f, -0.22f), pt(d, -0.18f, 0.06f), pt(d, -0.46f, 0.34f) };
				poly(d, caret, 3, false);
				seg(d, 0.02f, 0.36f, 0.46f, 0.36f);
				break;
			}
			case Id::Music:
			{

				d.dl->AddEllipseFilled(pt(d, -0.47f, 0.60f), ImVec2(0.30f * d.sz * 0.5f, 0.22f * d.sz * 0.5f), d.col, -0.35f);
				d.dl->AddEllipseFilled(pt(d, 0.39f, 0.44f), ImVec2(0.30f * d.sz * 0.5f, 0.22f * d.sz * 0.5f), d.col, -0.35f);
				seg(d, -0.17f, 0.60f, -0.17f, -0.56f);
				seg(d, 0.69f, 0.44f, 0.69f, -0.72f);
				seg(d, -0.17f, -0.56f, 0.69f, -0.72f);
				seg(d, -0.17f, -0.26f, 0.69f, -0.42f);
				break;
			}
			case Id::Settings:
			{
				circle(d, 0.f, 0.f, 0.42f);
				disc(d, 0.f, 0.f, 0.14f);
				for (int i = 0; i < 8; ++i)
				{
					const float a = (float)i * IM_PI * 0.25f;
					const float sx = cosf(a), sy = sinf(a);
					seg(d, sx * 0.56f, sy * 0.56f, sx * 0.86f, sy * 0.86f);
				}
				break;
			}
			case Id::Tree:
			{

				boxfill(d, -0.72f, -0.10f, -0.52f, 0.10f);
				seg(d, -0.52f, 0.f, 0.f, 0.f);
				seg(d, 0.f, -0.62f, 0.f, 0.62f);
				seg(d, 0.f, -0.62f, 0.50f, -0.62f);
				seg(d, 0.f, 0.62f, 0.50f, 0.62f);
				boxfill(d, 0.50f, -0.75f, 0.76f, -0.49f);
				boxfill(d, 0.50f, 0.49f, 0.76f, 0.75f);
				break;
			}
			case Id::Crosshair:
			{

				circle(d, 0.f, 0.f, 0.62f);
				seg(d, 0.f, -0.94f, 0.f, -0.52f);
				seg(d, 0.f, 0.52f, 0.f, 0.94f);
				seg(d, -0.94f, 0.f, -0.52f, 0.f);
				seg(d, 0.52f, 0.f, 0.94f, 0.f);
				const float k = 0.7071f;
				seg(d,  0.30f * k, -0.30f * k,  0.50f * k, -0.50f * k);
				seg(d,  0.30f * k,  0.30f * k,  0.50f * k,  0.50f * k);
				seg(d, -0.30f * k,  0.30f * k, -0.50f * k,  0.50f * k);
				seg(d, -0.30f * k, -0.30f * k, -0.50f * k, -0.50f * k);
				disc(d, 0.f, 0.f, 0.13f);
				break;
			}
			case Id::Eye:
			{

				cubic(d, -0.90f, 0.02f, -0.44f, -0.60f, 0.44f, -0.60f, 0.90f, 0.02f);
				cubic(d, -0.90f, 0.02f, -0.44f, 0.72f, 0.44f, 0.72f, 0.90f, 0.02f);
				circle(d, 0.f, 0.04f, 0.30f);
				disc(d, 0.f, 0.04f, 0.135f);
				disc(d, -0.09f, -0.05f, 0.055f);
				break;
			}
			case Id::Wrench:
			{

				const float hx = -0.42f, hy = -0.42f;
				const float u  = 0.7071f;
				circle(d, hx, hy, 0.26f);
				disc(d, hx, hy, 0.10f);
				const ImVec2 e0(hx + u * 0.28f, hy + u * 0.28f);
				const ImVec2 t0( 0.56f, 0.56f);
				const float w = 0.105f;
				const ImVec2 e1(e0.x - u * w, e0.y + u * w);
				const ImVec2 e2(e0.x + u * w, e0.y - u * w);
				const ImVec2 t1(t0.x - u * w, t0.y + u * w);
				const ImVec2 t2(t0.x + u * w, t0.y - u * w);
				seg(d, e1.x, e1.y, t1.x, t1.y);
				seg(d, e2.x, e2.y, t2.x, t2.y);
				seg(d, t1.x, t1.y, t2.x, t2.y);
				break;
			}
			case Id::Globe:
			{
				circle(d, 0.f, 0.f, 0.80f);
				seg(d, -0.80f, 0.f, 0.80f, 0.f);

				seg(d, -0.68f, -0.42f, 0.68f, -0.42f);
				seg(d, -0.68f, 0.42f, 0.68f, 0.42f);
				d.dl->AddEllipse(pt(d, 0.f, 0.f), ImVec2(0.40f * d.sz * 0.5f, 0.80f * d.sz * 0.5f), d.col, 0.f, 0, d.t);
				break;
			}
			case Id::Search:
			{
				circle(d, -0.18f, -0.18f, 0.54f);
				seg(d, 0.22f, 0.22f, 0.84f, 0.84f);
				break;
			}
			case Id::Close:
			{
				seg(d, -0.62f, -0.62f, 0.62f, 0.62f);
				seg(d, 0.62f, -0.62f, -0.62f, 0.62f);
				break;
			}
			case Id::ChevronDown:
			{
				const ImVec2 v[3] = { pt(d, -0.62f, -0.26f), pt(d, 0.f, 0.34f), pt(d, 0.62f, -0.26f) };
				poly(d, v, 3, false);
				break;
			}
			case Id::ChevronUp:
			{
				const ImVec2 v[3] = { pt(d, -0.62f, 0.26f), pt(d, 0.f, -0.34f), pt(d, 0.62f, 0.26f) };
				poly(d, v, 3, false);
				break;
			}
			case Id::ChevronRight:
			{
				const ImVec2 v[3] = { pt(d, -0.26f, -0.62f), pt(d, 0.34f, 0.f), pt(d, -0.26f, 0.62f) };
				poly(d, v, 3, false);
				break;
			}
			case Id::ChevronLeft:
			{
				const ImVec2 v[3] = { pt(d, 0.26f, -0.62f), pt(d, -0.34f, 0.f), pt(d, 0.26f, 0.62f) };
				poly(d, v, 3, false);
				break;
			}
			case Id::Play:
			{
				trifill(d, -0.44f, -0.70f, 0.70f, 0.f, -0.44f, 0.70f);
				break;
			}
			case Id::Pause:
			{
				boxfill(d, -0.56f, -0.66f, -0.16f, 0.66f, d.t * 0.4f);
				boxfill(d, 0.16f, -0.66f, 0.56f, 0.66f, d.t * 0.4f);
				break;
			}
			case Id::Stop:
			{
				boxfill(d, -0.56f, -0.56f, 0.56f, 0.56f, d.t * 0.8f);
				break;
			}
			case Id::Refresh:
			{
				arc(d, 0.f, 0.f, 0.62f, 0.10f, 0.86f);
				trifill(d, 0.86f, -0.34f, 0.40f, -0.16f, 0.80f, 0.12f);
				break;
			}
			case Id::Trash:
			{
				seg(d, -0.82f, -0.46f, 0.82f, -0.46f);
				seg(d, -0.26f, -0.46f, -0.26f, -0.74f);
				seg(d, 0.26f, -0.46f, 0.26f, -0.74f);
				seg(d, -0.26f, -0.74f, 0.26f, -0.74f);
				const ImVec2 bin[5] = { pt(d, -0.58f, -0.46f), pt(d, -0.44f, 0.76f), pt(d, 0.44f, 0.76f), pt(d, 0.58f, -0.46f), pt(d, -0.58f, -0.46f) };
				poly(d, bin, 5, false);
				seg(d, -0.16f, -0.16f, -0.12f, 0.48f);
				seg(d, 0.16f, -0.16f, 0.12f, 0.48f);
				break;
			}
			case Id::Save:
			{
				box(d, -0.78f, -0.78f, 0.78f, 0.78f, d.t * 1.4f);
				boxfill(d, -0.42f, -0.78f, 0.42f, -0.18f, d.t * 0.4f);
				box(d, -0.46f, 0.14f, 0.46f, 0.78f, d.t * 0.5f);
				break;
			}
			case Id::Plus:
			{
				seg(d, 0.f, -0.68f, 0.f, 0.68f);
				seg(d, -0.68f, 0.f, 0.68f, 0.f);
				break;
			}
			case Id::Minus:
			{
				seg(d, -0.68f, 0.f, 0.68f, 0.f);
				break;
			}
			case Id::Check:
			{
				const ImVec2 v[3] = { pt(d, -0.66f, 0.04f), pt(d, -0.16f, 0.54f), pt(d, 0.68f, -0.54f) };
				poly(d, v, 3, false);
				break;
			}
			case Id::Lock:
			case Id::Shield:
			{
				if (id == Id::Lock)
				{
					arc(d, 0.f, -0.20f, 0.40f, 0.50f, 1.00f);
					box(d, -0.62f, -0.20f, 0.62f, 0.78f, d.t * 1.6f);
					disc(d, 0.f, 0.28f, 0.11f);
				}
				else
				{
					const ImVec2 sh[6] = {
						pt(d, -0.68f, -0.66f), pt(d, 0.68f, -0.66f), pt(d, 0.62f, 0.16f),
						pt(d, 0.f, 0.82f),     pt(d, -0.62f, 0.16f), pt(d, -0.68f, -0.66f)
					};
					poly(d, sh, 6, false);
					const ImVec2 tick[3] = { pt(d, -0.30f, -0.04f), pt(d, -0.06f, 0.22f), pt(d, 0.34f, -0.26f) };
					poly(d, tick, 3, false);
				}
				break;
			}
			case Id::Download:
			case Id::Upload:
			{
				const float dir = (id == Id::Download) ? 1.f : -1.f;
				seg(d, 0.f, -0.72f * dir, 0.f, 0.40f * dir);
				const ImVec2 head[3] = { pt(d, -0.40f, 0.06f * dir), pt(d, 0.f, 0.50f * dir), pt(d, 0.40f, 0.06f * dir) };
				poly(d, head, 3, false);
				seg(d, -0.74f, 0.82f * dir, 0.74f, 0.82f * dir);
				break;
			}
			case Id::Sliders:
			{
				seg(d, -0.80f, -0.32f, 0.80f, -0.32f);
				seg(d, -0.80f, 0.32f, 0.80f, 0.32f);
				disc(d, 0.18f, -0.32f, 0.20f);
				disc(d, -0.30f, 0.32f, 0.20f);
				break;
			}
			case Id::Cursor:
			{

				const ImVec2 cs[8] = {
					pt(d,  0.00f, -0.80f), pt(d,  0.62f,  0.26f), pt(d,  0.20f,  0.26f),
					pt(d,  0.45f,  0.80f), pt(d,  0.20f,  0.90f), pt(d, -0.06f,  0.34f),
					pt(d, -0.44f,  0.68f), pt(d,  0.00f, -0.80f)
				};
				poly(d, cs, 8, false);
				break;
			}
			case Id::Sun:
			{
				circle(d, 0.f, 0.f, 0.34f);
				for (int i = 0; i < 8; ++i)
				{
					const float a = (float)i * IM_PI * 0.25f;
					const float sx = cosf(a), sy = sinf(a);
					seg(d, sx * 0.52f, sy * 0.52f, sx * 0.86f, sy * 0.86f);
				}
				break;
			}
			case Id::Moon:
			{

				const float r_out = 0.70f, r_in = 0.58f;
				const float ox    = 0.16f;
				const int   steps = 26;
				for (int i = 0; i <= steps; ++i)
				{
					const float a = IM_PI * (0.30f + 1.40f * ((float)i / (float)steps));
					d.dl->PathLineTo(pt(d, ox + cosf(a) * r_out, sinf(a) * r_out));
				}
				for (int i = steps; i >= 0; --i)
				{
					const float a = IM_PI * (0.34f + 1.32f * ((float)i / (float)steps));
					d.dl->PathLineTo(pt(d, ox + 0.26f + cosf(a) * r_in, sinf(a) * r_in));
				}
				d.dl->PathStroke(d.col, ImDrawFlags_Closed, d.t);
				break;
			}
			case Id::Snowflake:
			{
				for (int i = 0; i < 3; ++i)
				{
					const float a = (float)i * IM_PI / 3.f;
					const float cx = cosf(a), sy = sinf(a);
					seg(d, -cx * 0.86f, -sy * 0.86f, cx * 0.86f, sy * 0.86f);
					seg(d, cx * 0.52f, sy * 0.52f, cx * 0.34f - sy * 0.24f, sy * 0.34f + cx * 0.24f);
					seg(d, cx * 0.52f, sy * 0.52f, cx * 0.34f + sy * 0.24f, sy * 0.34f - cx * 0.24f);
					seg(d, -cx * 0.52f, -sy * 0.52f, -cx * 0.34f + sy * 0.24f, -sy * 0.34f - cx * 0.24f);
					seg(d, -cx * 0.52f, -sy * 0.52f, -cx * 0.34f - sy * 0.24f, -sy * 0.34f + cx * 0.24f);
				}
				disc(d, 0.f, 0.f, 0.10f);
				break;
			}
			case Id::Bolt:
			{
				trifill(d, 0.14f, -0.82f, -0.52f, 0.12f, 0.02f, 0.12f);
				trifill(d, -0.14f, 0.82f, 0.52f, -0.12f, -0.02f, -0.12f);
				break;
			}
			case Id::Target:
			{
				circle(d, 0.f, 0.f, 0.82f);
				circle(d, 0.f, 0.f, 0.48f);
				disc(d, 0.f, 0.f, 0.14f);
				break;
			}
			case Id::Star:
			{

				star(d, 0.68f, 0.27f, false);
				break;
			}
			case Id::Sparkle:
			{
				star(d, 0.82f, 0.22f, true);
				break;
			}
			case Id::Flame:
			{
				cubic(d, 0.f, 0.86f, -0.86f, 0.36f, -0.30f, -0.36f, 0.06f, -0.88f);
				cubic(d, 0.06f, -0.88f, 0.18f, -0.28f, 0.86f, -0.10f, 0.f, 0.86f);
				cubic(d, 0.f, 0.52f, -0.24f, 0.18f, 0.06f, -0.12f, 0.02f, -0.30f);
				break;
			}
			case Id::Ghost:
			{
				const ImVec2 g[8] = {
					pt(d, -0.62f, 0.66f),  pt(d, -0.62f, -0.06f), pt(d, -0.34f, -0.72f), pt(d, 0.34f, -0.72f),
					pt(d, 0.62f, -0.06f),  pt(d, 0.62f, 0.66f),   pt(d, 0.20f, 0.44f),   pt(d, -0.20f, 0.72f)
				};
				poly(d, g, 8, true);
				disc(d, -0.22f, -0.24f, 0.11f);
				disc(d, 0.22f, -0.24f, 0.11f);
				break;
			}
			case Id::Crown:
			{

				const float dy = 0.12f;
				const ImVec2 c[6] = {
					pt(d, -0.78f, 0.46f + dy), pt(d, -0.78f, -0.56f + dy), pt(d, -0.38f, -0.10f + dy),
					pt(d, 0.f, -0.76f + dy),   pt(d, 0.38f, -0.10f + dy),  pt(d, 0.78f, -0.56f + dy)
				};
				poly(d, c, 6, false);
				seg(d, 0.78f, -0.56f + dy, 0.78f, 0.46f + dy);
				seg(d, -0.78f, 0.46f + dy, 0.78f, 0.46f + dy);
				break;
			}
			case Id::Dice:
			{
				box(d, -0.76f, -0.76f, 0.76f, 0.76f, d.t * 1.6f);
				disc(d, -0.38f, -0.38f, 0.12f);
				disc(d, 0.38f, -0.38f, 0.12f);
				disc(d, 0.f, 0.f, 0.12f);
				disc(d, -0.38f, 0.38f, 0.12f);
				disc(d, 0.38f, 0.38f, 0.12f);
				break;
			}
			case Id::Wand:
			{
				seg(d, -0.74f, 0.74f, 0.34f, -0.34f);
				star(d, 0.86f, 0.30f, true);
				break;
			}
			case Id::Layers:
			{
				const ImVec2 a[4] = { pt(d, 0.f, -0.70f), pt(d, 0.70f, -0.32f), pt(d, 0.f, 0.08f), pt(d, -0.70f, -0.32f) };
				poly(d, a, 4, true);
				const ImVec2 b[3] = { pt(d, -0.70f, 0.18f), pt(d, 0.f, 0.58f), pt(d, 0.70f, 0.18f) };
				poly(d, b, 3, false);
				break;
			}
			case Id::Heart:
			{
				disc(d, -0.32f, -0.26f, 0.40f);
				disc(d, 0.32f, -0.26f, 0.40f);
				trifill(d, -0.72f, -0.04f, 0.72f, -0.04f, 0.f, 0.82f);
				break;
			}
			case Id::Info:
			{
				circle(d, 0.f, 0.f, 0.80f);
				seg(d, 0.f, -0.14f, 0.f, 0.52f);
				disc(d, 0.f, -0.50f, 0.12f);
				break;
			}
			case Id::Warning:
			{
				const ImVec2 t[3] = { pt(d, 0.f, -0.70f), pt(d, 0.74f, 0.62f), pt(d, -0.74f, 0.62f) };
				poly(d, t, 3, true);
				seg(d, 0.f, -0.26f, 0.f, 0.18f);
				disc(d, 0.f, 0.42f, 0.10f);
				break;
			}
			case Id::Dot:
			{
				disc(d, 0.f, 0.f, 0.46f);
				break;
			}
			case Id::Logo:
			{

				const ImVec2 hex[6] = {
					pt(d, 0.f, -0.86f), pt(d, 0.76f, -0.44f), pt(d, 0.76f, 0.44f),
					pt(d, 0.f, 0.86f),  pt(d, -0.76f, 0.44f), pt(d, -0.76f, -0.44f)
				};
				poly(d, hex, 6, true);
				seg(d, 0.f, -0.86f, 0.f, 0.f);
				seg(d, -0.76f, -0.44f, 0.f, 0.f);
				seg(d, 0.76f, -0.44f, 0.f, 0.f);
				seg(d, 0.f, 0.f, 0.f, 0.86f);
				break;
			}
			default:
			{
				circle(d, 0.f, 0.f, 0.40f);
				break;
			}
			}
		}

		inline void draw_nav(ImDrawList* dl, Nav id, const ImVec2& centre, float sz, ImU32 col)
		{
			if (!dl || id == Nav::None || sz <= 0.f)
				return;

			const NavGlyph g = nav_glyph(id);
			const float size = sz * nav_optical(id);

			if (g.pack)
			{
				if (fonts::draw_nav_icon(dl, g.codepoint, centre, size, col))
					return;
			}
			else if (fonts::draw_icon(dl, g.codepoint, centre, size, col))
			{
				return;
			}

			draw(dl, fa_fallback(id), centre, sz, col);
		}

		inline void pip(ImDrawList* dl, const ImVec2& centre, float radius, ImU32 col)
		{
			if (dl && radius > 0.f)
				dl->AddCircleFilled(centre, radius, col, 16);
		}
	}
}
