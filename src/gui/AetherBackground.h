#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include <render/SurfaceTex.h>

namespace aether
{
	namespace bg
	{
		enum class Effect : int
		{
			None = 0,
			Snow,
			Rain,
			Stars,
			Bokeh,
			Matrix,
			Embers,
			Confetti,
			Aurora,
			Grid,
			Tiles,
			Topography,

			COUNT
		};

		inline const char* const kEffectNamesCsv =
			"None\0"
			"Snow\0"
			"Rain\0"
			"Stars\0"
			"Bokeh\0"
			"Matrix Rain\0"
			"Embers\0"
			"Confetti\0"
			"Aurora\0"
			"Neon Grid\0"
			"Glass Tiles\0"
			"Topography\0";

		struct Config
		{
			int   effect   = (int)Effect::Snow;
			float density  = 1.0f;
			float speed    = 1.0f;
			float size     = 1.0f;
			float opacity  = 0.85f;
			float color[4] = { 1.f, 1.f, 1.f, 1.f };
			float wind     = 0.20f;
			float glow     = 0.55f;
			bool  parallax = true;

			// The falling elements fall along this angle, in degrees from straight
			// down (negative leans left).  It is the *velocity* the drops are given
			// and the direction their streaks are drawn along, so a drop and its
			// line can never disagree about which way it is going.
			float angle    = 0.0f;

			// Rain streak length, as a multiple of size.  The length itself follows
			// the drop's own speed, so this is a persistence multiplier and not a
			// fixed number of pixels - see draw_rain.
			float streak   = 1.0f;

			// How far each drop may lean off `angle`, in degrees.  Zero makes every
			// drop parallel, which reads as a comb rather than as rain.
			float spread   = 4.0f;

			// The streak's width, as a multiple of the drop's size.
			float width    = 1.0f;

			// The material under the effect.  Neither one is this file's to set -
			// they come from the backdrop's own sliders - but every effect reads
			// them so what it draws belongs to the glass instead of sitting on top
			// of whatever the glass happens to be.  `veil` is the blur (0 is a
			// sharp backdrop, 1 a heavy frost); it widens and softens the light and
			// fades fine detail.  `tint` is the darkening of the capture, and it
			// takes the peak down because a dark plate needs less light.
			float veil     = 0.f;
			float tint     = 0.f;
		};

		namespace detail
		{

			inline std::uint32_t& rng_state()
			{
				static std::uint32_t s = 0x9e3779b9u;
				return s;
			}

			inline std::uint32_t rng_u32()
			{
				std::uint32_t& s = rng_state();
				s ^= s << 13; s ^= s >> 17; s ^= s << 5;
				return s;
			}

			inline float rng01()
			{
				return (float)(rng_u32() >> 8) * (1.0f / 16777216.0f);
			}

			inline float rng_range(float lo, float hi)
			{
				return lo + (hi - lo) * rng01();
			}

			inline ImU32 with_alpha(ImU32 col, float a)
			{
				a = ImClamp(a, 0.0f, 1.0f);
				return (col & 0x00FFFFFFu) | ((ImU32)(a * 255.f + 0.5f) << 24);
			}

			inline ImU32 col4(const float c[4], float a)
			{
				return IM_COL32(
					(int)(ImClamp(c[0], 0.f, 1.f) * 255.f + 0.5f),
					(int)(ImClamp(c[1], 0.f, 1.f) * 255.f + 0.5f),
					(int)(ImClamp(c[2], 0.f, 1.f) * 255.f + 0.5f),
					(int)(ImClamp(c[3], 0.f, 1.f) * a * 255.f + 0.5f));
			}

			inline void soft_dot(ImDrawList* dl, const ImVec2& c, float r, ImU32 col, float alpha, float glow)
			{
				if (r <= 0.f || alpha <= 0.001f)
					return;

				// The halo, sampled: one quad of the dithered falloff instead of three
				// filled circles.  Those circles were the pixely part - three hard
				// steps of alpha, so a bright particle over the frost read as a dot
				// with rings around it.  The dot itself stays a filled circle: at a
				// few pixels across that is exactly what a dot is.
				if (glow > 0.02f)
				{
					if (const ImTextureID tex = surface::radial())
					{
						const float rad = r * (1.f + 1.9f * glow);
						dl->AddImage(tex, ImVec2(c.x - rad, c.y - rad), ImVec2(c.x + rad, c.y + rad),
							ImVec2(0.f, 0.f), ImVec2(1.f, 1.f),
							with_alpha(col, alpha * 0.85f * glow));
					}
					else
					{
						for (int i = 3; i >= 1; --i)
						{
							float rad = r * (1.f + 0.85f * glow * (float)i);
							float a   = alpha * 0.22f * glow / (float)i;
							dl->AddCircleFilled(c, rad, with_alpha(col, a), 0);
						}
					}
				}
				dl->AddCircleFilled(c, r, with_alpha(col, alpha), 0);
			}

			struct particle_t
			{
				float x, y;
				float vx, vy;
				float size;
				float rot;
				float rot_v;
				float phase;
				float depth;
				float seed;
				float life;
			};

			inline std::vector<particle_t>& pool()
			{
				static std::vector<particle_t> v;
				return v;
			}

			inline int& last_effect()
			{
				static int e = -1;
				return e;
			}

			inline int base_count(Effect fx)
			{
				switch (fx)
				{
				case Effect::Snow:     return 260;
				case Effect::Rain:     return 170;
				case Effect::Stars:    return 150;
				case Effect::Bokeh:    return 48;
				case Effect::Matrix:   return 64;
				case Effect::Embers:   return 100;
				case Effect::Confetti: return 96;
				case Effect::Tiles:    return 40;
				default:               return 0;
				}
			}

			inline void spawn(particle_t& p, Effect fx, float w, float h, const Config& cfg, bool initial)
			{
				p.depth = rng01();
				p.seed  = rng01();
				p.phase = rng01() * 6.2831853f;
				p.rot   = rng01() * 6.2831853f;
				p.life  = 1.0f;

				const float depth_scale = cfg.parallax ? (0.45f + 0.55f * p.depth) : 1.0f;

				switch (fx)
				{
				case Effect::Snow:
					p.x    = rng_range(-20.f, w + 20.f);
					p.y    = initial ? rng_range(-20.f, h + 20.f) : rng_range(-30.f, -4.f);
					p.vx   = 0.f;
					p.vy   = rng_range(18.f, 52.f) * depth_scale;
					p.size = rng_range(1.1f, 2.6f) * cfg.size;
					p.rot_v = rng_range(-0.9f, 0.9f);
					break;

				case Effect::Rain:
				{
					// The fall is one vector at `angle` from straight down, plus this
					// drop's own lean inside `spread`: the drop travels along it and the
					// streak is drawn along it, so a slanted drop is a slanted line and
					// not an upright one sliding sideways.  Without the lean every drop
					// is parallel to every other and the element reads as a comb.
					const float lean = rng_range(-cfg.spread, cfg.spread);
					const float rad  = (cfg.angle + lean) * 0.01745329252f;
					const float spd  = rng_range(620.f, 1050.f) * depth_scale;

					// Spawned up-wind of the rectangle when the fall is not vertical,
					// or the drops would only appear part-way across it.  The lead is
					// the widest lean any drop may have, since a drop leaning further
					// than the band is wide would enter from outside it.
					const float wide = (ImFabs(cfg.angle) + ImFabs(cfg.spread)) * 0.01745329252f;
					const float lead = ImFabs(std::sin(wide > 1.4f ? 1.4f : wide)) * h;

					p.x    = rng_range(-30.f - lead, w + 30.f + lead);
					p.y    = initial ? rng_range(-h * 0.5f, h) : rng_range(-58.f, -10.f);
					p.vx   = spd * std::sin(rad);
					p.vy   = spd * std::cos(rad);
					p.size = rng_range(0.9f, 1.7f) * cfg.size;
					p.rot_v = 0.f;
				}
				break;

				case Effect::Stars:
					p.x    = rng_range(0.f, w);
					p.y    = rng_range(0.f, h);
					p.vx   = rng_range(-3.f, 3.f);
					p.vy   = rng_range(-2.f, 2.f);
					p.size = rng_range(0.6f, 1.7f) * cfg.size * (0.6f + 0.6f * p.depth);
					p.rot_v = 0.f;
					break;

				case Effect::Bokeh:
					p.x    = rng_range(0.f, w);
					p.y    = initial ? rng_range(0.f, h) : rng_range(h + 6.f, h + 30.f);
					p.vx   = 0.f;
					p.vy   = rng_range(-26.f, -8.f) * depth_scale;
					p.size = rng_range(7.f, 22.f) * cfg.size * (0.5f + 0.9f * p.depth);
					p.rot_v = 0.f;
					break;

				case Effect::Matrix:
					p.x    = rng_range(0.f, w);
					p.y    = initial ? rng_range(0.f, h) : rng_range(-56.f, -18.f);
					p.vx   = 0.f;
					p.vy   = rng_range(90.f, 210.f) * depth_scale;
					p.size = rng_range(10.f, 11.f) * cfg.size;
					p.rot_v = 0.f;
					break;

				case Effect::Embers:
					p.x    = rng_range(0.f, w);
					p.y    = initial ? rng_range(0.f, h) : rng_range(h + 4.f, h + 28.f);
					p.vx   = 0.f;
					p.vy   = rng_range(-70.f, -22.f) * depth_scale;
					p.size = rng_range(1.0f, 2.4f) * cfg.size;
					p.rot_v = 0.f;
					break;

				case Effect::Confetti:
					p.x    = rng_range(-20.f, w + 20.f);
					p.y    = initial ? rng_range(-20.f, h + 20.f) : rng_range(-40.f, -6.f);
					p.vx   = 0.f;
					p.vy   = rng_range(50.f, 130.f) * depth_scale;
					p.size = rng_range(2.2f, 4.4f) * cfg.size;
					p.rot_v = rng_range(-4.f, 4.f);
					break;

				case Effect::Tiles:

					p.x    = rng_range(-30.f, w + 30.f);
					p.y    = initial ? rng_range(-30.f, h + 30.f) : rng_range(h + 8.f, h + 46.f);
					p.vx   = 0.f;
					p.vy   = -rng_range(14.f, 46.f) * depth_scale;
					p.size = rng_range(16.f, 52.f) * cfg.size * depth_scale;
					p.rot_v = rng_range(-0.45f, 0.45f);
					break;

				default:
					break;
				}
			}

			inline void draw_snow(ImDrawList* dl, const particle_t& p, const Config& cfg, float t, ImU32 base)
			{
				(void)t;
				const float a = cfg.opacity * (0.28f + 0.72f * p.depth);
				const float r = p.size * (0.55f + 0.45f * p.depth);
				ImVec2 c(p.x, p.y);

				if (r < 1.35f)
				{
					soft_dot(dl, c, r, base, a, cfg.glow * 0.5f);
					return;
				}

				soft_dot(dl, c, r * 0.75f, base, a, cfg.glow);
				ImU32 line = with_alpha(base, a * 0.55f);
				for (int i = 0; i < 3; ++i)
				{
					const float ang = p.rot + (float)i * 1.0471976f;
					const float dx = std::cos(ang) * r * 2.1f;
					const float dy = std::sin(ang) * r * 2.1f;
					dl->AddLine(ImVec2(c.x - dx, c.y - dy), ImVec2(c.x + dx, c.y + dy), line, 1.f);
				}
			}

			inline void draw_rain(ImDrawList* dl, const particle_t& p, const Config& cfg, ImU32 base)
			{
				const float a = cfg.opacity * (0.20f + 0.55f * p.depth);

				// Along the drop's own velocity, so the streak is the path it is
				// travelling: straight down for a vertical fall, slanted for anything
				// else, at any angle, and leaning with the drop when it has a lean of
				// its own.
				const float vlen = std::sqrt(p.vx * p.vx + p.vy * p.vy);
				const float ux = vlen > 0.001f ? p.vx / vlen : 0.f;
				const float uy = vlen > 0.001f ? p.vy / vlen : 1.f;
				const ImVec2 head(p.x, p.y);

				// How long a drop trails behind itself.  It is the distance a drop
				// covers in a short, fixed *time*, so it follows the speed the drop
				// is actually travelling at - `cfg.speed` is the same factor the
				// simulation's own step is scaled by.  A fixed length instead is what
				// made slow rain look wrong: at a tenth of the speed the drops crawl
				// but the streaks stay as long as they were, so the element turns
				// into a field of near-stationary sticks rather than a drizzle.
				constexpr float k_trail_seconds = 0.030f;
				const float pers = k_trail_seconds * ImClamp(cfg.streak, 0.2f, 3.f) *
					ImClamp(cfg.size, 0.2f, 3.f) * (0.55f + 0.75f * p.depth);
				const float len = ImClamp(vlen * ImClamp(cfg.speed, 0.05f, 4.f) * pers, 3.f, 180.f);

				const float w = ImMax(1.f, p.size * 0.6f * ImClamp(cfg.width, 0.2f, 3.f));

				// Three segments, each dimmer than the last, so the tail fades
				// instead of stopping: a streak that ends in a hard edge is what
				// makes cheap rain read as sticks.
				for (int i = 0; i < 3; ++i)
				{
					const float t0 = (float)i / 3.f;
					const float t1 = (float)(i + 1) / 3.f;
					const ImVec2 a0(head.x - ux * len * t0, head.y - uy * len * t0);
					const ImVec2 a1(head.x - ux * len * t1, head.y - uy * len * t1);
					dl->AddLine(a0, a1, with_alpha(base, a * (1.f - t0 * 0.82f)), w);
				}

				soft_dot(dl, head, ImMax(0.6f, p.size * 0.7f * ImClamp(cfg.width, 0.2f, 3.f)),
					base, a * 0.85f, cfg.glow * 0.35f);
			}

			inline void draw_stars(ImDrawList* dl, const particle_t& p, const Config& cfg, float t, ImU32 base)
			{
				const float twinkle = 0.55f + 0.45f * std::sin(t * (1.2f + p.seed * 2.4f) + p.phase);
				const float a = cfg.opacity * twinkle * (0.35f + 0.65f * p.depth);
				ImVec2 c(p.x, p.y);
				soft_dot(dl, c, p.size, base, a, cfg.glow);

				if (p.seed > 0.88f)
				{
					const float s = p.size * 3.4f;
					ImU32 lc = with_alpha(base, a * 0.55f);
					dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x + s, c.y), lc, 1.f);
					dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x, c.y + s), lc, 1.f);
				}
			}

			inline void draw_bokeh(ImDrawList* dl, const particle_t& p, const Config& cfg, ImU32 base)
			{
				const float a = cfg.opacity * (0.05f + 0.13f * p.depth);
				ImVec2 c(p.x, p.y);
				dl->AddCircleFilled(c, p.size, with_alpha(base, a), 0);
				dl->AddCircleFilled(c, p.size * 0.72f, with_alpha(base, a * 1.5f), 0);
				dl->AddCircle(c, p.size * 0.98f, with_alpha(base, a * 3.2f), 0, ImMax(1.f, p.size * 0.06f));
			}

			inline void draw_matrix(ImDrawList* dl, const particle_t& p, const Config& cfg, ImU32 base)
			{
				static const char kGlyphs[] = "01<>[]{}#$%&*+=?/\\|ABCDEFGHIJKLMNOPQRSTUVWXYZ";
				const int n = (int)(sizeof(kGlyphs) - 1);

				ImFont* font = ImGui::GetFont();
				if (!font || n <= 0)
					return;

				const float fs = p.size;
				const float step = fs * 0.95f;
				const int   trail = 10;

				for (int i = 0; i < trail; ++i)
				{
					const float fade = 1.0f - (float)i / (float)trail;
					const float a = cfg.opacity * fade * fade * (0.25f + 0.75f * p.depth);
					if (a <= 0.01f)
						continue;

					char ch[2];
					ch[0] = kGlyphs[(int)((p.seed * 977.f + (float)i * 131.f)) % n];
					ch[1] = 0;

					const ImVec2 pos(p.x, p.y - (float)i * step);
					dl->AddText(font, fs, ImVec2(pos.x + 1.f, pos.y), IM_COL32(0, 0, 0, (int)(a * 90.f)), ch, ch + 1);
					dl->AddText(font, fs, pos, i == 0 ? with_alpha(0xFFFFFFFFu, a) : with_alpha(base, a), ch, ch + 1);
				}
			}

			inline void draw_ember(ImDrawList* dl, const particle_t& p, const Config& cfg, float t, ImU32 base)
			{
				const float flick = 0.55f + 0.45f * std::sin(t * (5.f + p.seed * 7.f) + p.phase);
				const float a = cfg.opacity * p.life * flick * (0.35f + 0.65f * p.depth);
				ImVec2 c(p.x, p.y);
				soft_dot(dl, c, p.size, base, a, cfg.glow * 1.35f);
				dl->AddLine(ImVec2(c.x, c.y + p.size * 1.2f), ImVec2(c.x, c.y - p.size * 2.6f),
					with_alpha(base, a * 0.35f), ImMax(1.f, p.size * 0.8f));
			}

			inline void draw_confetti(ImDrawList* dl, const particle_t& p, const Config& cfg, ImU32 base)
			{
				static const ImU32 kPalette[6] = {
					IM_COL32(255, 92, 138, 255), IM_COL32(255, 196, 87, 255),
					IM_COL32(94, 214, 255, 255), IM_COL32(150, 255, 170, 255),
					IM_COL32(196, 148, 255, 255), IM_COL32(255, 255, 255, 255)
				};
				(void)base;

				const ImU32 col = kPalette[(int)(p.seed * 5.999f)];
				const float a = cfg.opacity * p.life * (0.35f + 0.65f * p.depth);
				const float hw = p.size * 2.0f;
				const float hh = p.size * 0.9f;

				const float cs = std::cos(p.rot), sn = std::sin(p.rot);
				ImVec2 pts[4];
				const float lx[4] = { -hw,  hw,  hw, -hw };
				const float ly[4] = { -hh, -hh,  hh,  hh };
				for (int i = 0; i < 4; ++i)
				{
					pts[i].x = p.x + lx[i] * cs - ly[i] * sn;
					pts[i].y = p.y + lx[i] * sn + ly[i] * cs;
				}

				const float squash = 0.25f + 0.75f * std::fabs(std::sin(p.rot * 1.7f + p.phase));
				for (int i = 0; i < 4; ++i)
					pts[i].y = p.y + (pts[i].y - p.y) * squash;

				dl->AddQuadFilled(pts[0], pts[1], pts[2], pts[3], with_alpha(col, a));
			}

			inline void tile_quad(ImDrawList* dl, const ImVec2& c, float half, float co, float si, ImU32 col)
			{
				const float lx[4] = { -half, half, half, -half };
				const float ly[4] = { -half, -half, half, half };
				ImVec2 q[4];
				for (int i = 0; i < 4; ++i)
				{
					q[i].x = c.x + lx[i] * co - ly[i] * si;
					q[i].y = c.y + lx[i] * si + ly[i] * co;
				}
				dl->AddQuadFilled(q[0], q[1], q[2], q[3], col);
			}

			inline void draw_tiles(ImDrawList* dl, const particle_t& p, const Config& cfg, float t, ImU32 base)
			{
				const float depth = 0.40f + 0.60f * p.depth;
				const float half  = p.size * 0.5f;
				if (half < 2.0f)
					return;

				const float co = std::cos(p.rot), si = std::sin(p.rot);
				const float breathe = 0.72f + 0.28f * std::sin(t * 0.7f + p.phase);
				const float a = cfg.opacity * depth * breathe;
				const ImVec2 c(p.x, p.y);

				if (cfg.glow > 0.05f)
					for (int i = 3; i >= 1; --i)
						tile_quad(dl, ImVec2(c.x + 1.5f, c.y + 2.5f), half + (float)i * 1.8f, co, si,
							IM_COL32(0, 0, 0, (int)(18.f * a / (float)i)));

				tile_quad(dl, c, half, co, si, with_alpha(base, a * 0.20f));

				const float lx[4] = { -half, half, half, -half };
				const float ly[4] = { -half, -half, half, half };
				ImVec2 q[4];
				for (int i = 0; i < 4; ++i)
				{
					q[i].x = c.x + lx[i] * co - ly[i] * si;
					q[i].y = c.y + lx[i] * si + ly[i] * co;
				}
				dl->AddQuad(q[0], q[1], q[2], q[3], with_alpha(base, a * 0.55f), 1.0f);

				const float ih = half * 0.58f;
				tile_quad(dl, ImVec2(c.x - ih * 0.35f, c.y - ih * 0.35f), ih, co, si,
					IM_COL32(255, 255, 255, (int)(26.f * a)));
			}

			// The aurora, as three bands of light that are gradients rather than
			// stacks of horizontal strips.
			//
			// What it was: 64 vertical strips per ribbon, each a four-corner quad
			// with a flat alpha.  A strip has no vertical falloff, so the band had a
			// hard top edge and a hard bottom edge, and the only softening was the
			// strip-to-strip step in alpha - which, stacked over the frost, is what
			// read as pixelated bands.  A ribbon is a light, so it now samples the
			// ribbon texture: a bell across the band and a bell down its length, the
			// second one dithered so the fade has no rings.  The same curve costs
			// ninety-six quads instead of the strips' hundred and twenty-eight, and
			// has neither an edge nor a step in it.
			inline void draw_aurora_ribbon(ImDrawList* dl, const ImVec2& a, const ImVec2& b,
				const Config& cfg, float t, int ribbon, ImU32 tint, float alpha)
			{
				const float w = b.x - a.x, h = b.y - a.y;
				const float veil = ImClamp(cfg.veil, 0.f, 1.f);

				const float speed  = (0.22f + 0.16f * (float)ribbon) * cfg.speed;
				const float amp    = h * (0.055f + 0.030f * (float)ribbon);
				const float center = a.y + h * (0.30f + 0.16f * (float)ribbon);

				// A heavy frost scatters light: the bands spread and their peak comes
				// down, which is the difference between a ribbon *under* frost and a
				// ribbon painted on top of it.
				const float half   = h * (0.16f + 0.05f * (float)ribbon) * (1.f + 0.75f * veil);
				const float peak   = alpha * (1.f - 0.30f * veil);

				auto centre_at = [&](float u)
				{
					return center
						+ std::sin(t * speed + u * 6.0f + (float)ribbon * 1.7f) * amp
						+ std::sin(t * speed * 2.1f + u * 13.f) * amp * 0.35f;
				};

				const ImTextureID tex = surface::ribbon();
				if (!tex)
				{
					// Nothing sampled yet: fall back to the old strip wash, which at
					// least still fades in alpha.
					const int strips = 64;
					const float step = w / (float)strips;
					for (int i = 0; i < strips; ++i)
					{
						const float u0 = (float)i / (float)strips;
						const float u1 = (float)(i + 1) / (float)strips;
						const float yc = centre_at((u0 + u1) * 0.5f);
						dl->AddRectFilledMultiColor(
							ImVec2(a.x + u0 * w, yc - half), ImVec2(a.x + u1 * w + 0.5f, yc + half),
							with_alpha(tint, 0.f), with_alpha(tint, 0.f),
							with_alpha(tint, peak), with_alpha(tint, peak));
					}
					return;
				}

				const int steps = 96;
				const ImU32 core = with_alpha(0xFFFFFFFFu, 1.f);

				for (int i = 0; i < steps; ++i)
				{
					const float u0 = (float)i / (float)steps;
					const float u1 = (float)(i + 1) / (float)steps;

					const float x0 = a.x + u0 * w;
					const float x1 = a.x + u1 * w;
					const float y0 = centre_at(u0);
					const float y1 = centre_at(u1);

					// The band breathes wider and narrower along its length, so it
					// never reads as a straight strip of light.
					const float th0 = half * (0.86f + 0.14f * std::sin(u0 * 9.f + t * 0.3f + (float)ribbon));
					const float th1 = half * (0.86f + 0.14f * std::sin(u1 * 9.f + t * 0.3f + (float)ribbon));

					dl->AddImageQuad(tex,
						ImVec2(x0, y0 - th0), ImVec2(x1, y1 - th1),
						ImVec2(x1, y1 + th1), ImVec2(x0, y0 + th0),
						ImVec2(u0, 0.f), ImVec2(u1, 0.f), ImVec2(u1, 1.f), ImVec2(u0, 1.f),
						with_alpha(tint, peak));

					// A narrow, brighter core along the middle of the band: the same
					// texture squeezed to a third of the height is the hot line.
					dl->AddImageQuad(tex,
						ImVec2(x0, y0 - th0 * 0.34f), ImVec2(x1, y1 - th1 * 0.34f),
						ImVec2(x1, y1 + th1 * 0.34f), ImVec2(x0, y0 + th0 * 0.34f),
						ImVec2(u0, 0.f), ImVec2(u1, 0.f), ImVec2(u1, 1.f), ImVec2(u0, 1.f),
						with_alpha(core & 0x00FFFFFFu, peak * 0.42f));
				}
			}

			inline void draw_aurora(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const Config& cfg, float t, ImU32 base)
			{
				const float veil = ImClamp(cfg.veil, 0.f, 1.f);
				const float tint = ImClamp(cfg.tint, 0.f, 1.f);
				const float gain = (1.f - 0.35f * tint) * (1.f + 0.20f * veil);

				for (int ribbon = 0; ribbon < 3; ++ribbon)
				{
					const float alpha = cfg.opacity * (0.26f - 0.06f * (float)ribbon) *
					                    (0.6f + 0.4f * cfg.glow) * gain;

					ImU32 colour = base;
					if (ribbon == 1) colour = IM_COL32(150, 255, 220, 255);
					if (ribbon == 2) colour = IM_COL32(190, 160, 255, 255);

					draw_aurora_ribbon(dl, a, b, cfg, t, ribbon, colour, alpha);
				}
			}

			// Topography: a contour map of a slowly drifting height field.
			//
			// The map is a texture because contours are detail, and detail drawn as
			// thousands of line segments per frame is both slow and never quite
			// clean - a 1 px line stepped across a curved level crawls.  The two
			// iso-line maps are baked tileable and anti-aliased once, then laid down
			// as scrolled quads: one coarse set that gives the shape, one finer set
			// drifting at its own rate that gives it depth.  Both are white, so the
			// element's colour and the accent both come through the vertex tint.
			inline void draw_topography(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const Config& cfg, float t, ImU32 base)
			{
				const ImTextureID coarse = surface::topo_coarse();
				const ImTextureID fine   = surface::topo_fine();
				if (!coarse && !fine)
					return;

				const float w = b.x - a.x, h = b.y - a.y;
				const float veil = ImClamp(cfg.veil, 0.f, 1.f);
				const float tint = ImClamp(cfg.tint, 0.f, 1.f);

				// Fine detail is the first thing a frost eats, so the lines lose
				// contrast as the blur rises - the map is still there, it is just
				// seen through the same glass as everything else.
				const float gain  = (1.f - 0.45f * tint) * (1.f - 0.40f * veil);
				const float alpha = cfg.opacity * (0.09f + 0.13f * cfg.glow) * gain;
				if (alpha <= 0.002f)
					return;

				// `size` is the zoom of the map: a larger tile is a wider country.
				// The default is well over one screen wide, because a contour map is
				// the one element where a repeat is obvious - a landscape does not
				// tile, and the eye finds the seam before it finds the shape.
				const float tile = ImMax(320.f, 1200.f * ImClamp(cfg.size, 0.2f, 3.f));
				const float dx = t * 11.f * cfg.speed;
				const float dy = t * 6.f * cfg.speed;

				if (coarse)
				{
					dl->AddImage(coarse, a, b,
						ImVec2(dx / tile, dy / tile),
						ImVec2((dx + w) / tile, (dy + h) / tile),
						with_alpha(base, alpha));
				}

				if (fine)
				{
					// The second set runs at a different scale and drifts the other
					// way, which is what stops two contour sets reading as one sheet
					// of wallpaper.
					const float ftile = tile * 0.58f;
					const float fu = -dx * 0.7f / ftile + 0.31f;
					const float fv =  dy * 0.4f / ftile + 0.13f;
					const float fa = alpha * (0.35f + 0.35f * ImClamp(cfg.density, 0.f, 1.5f)) * (1.f - 0.45f * veil);
					if (fa > 0.002f)
						dl->AddImage(fine, a, b,
							ImVec2(fu, fv),
							ImVec2(fu + w / ftile, fv + h / ftile),
							with_alpha(base, fa));
				}
			}

			inline void draw_grid(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const Config& cfg, float t, ImU32 base)
			{
				const float w = b.x - a.x, h = b.y - a.y;
				const float horizon = a.y + h * 0.42f;
				const float cx = a.x + w * 0.5f;
				const float bottom = b.y;
				// Lines are detail: a frost takes their contrast down, a tint takes
				// their peak, so the grid stays behind the glass rather than on it.
				const float alpha = cfg.opacity * (0.14f + 0.20f * cfg.glow)
					* (1.f - 0.45f * ImClamp(cfg.tint, 0.f, 1.f))
					* (1.f - 0.35f * ImClamp(cfg.veil, 0.f, 1.f));
				const float pulse = 0.6f + 0.4f * std::sin(t * 0.7f * cfg.speed);

				const int rails = 21;
				for (int i = 0; i <= rails; ++i)
				{
					const float u = (float)i / (float)rails;
					const float x = a.x + u * w;
					const float x_top = cx + (x - cx) * 0.10f;
					dl->AddLine(ImVec2(x_top, horizon), ImVec2(x, bottom), with_alpha(base, alpha * 0.7f), 1.f);
					dl->AddLine(ImVec2(x_top, horizon), ImVec2(x, bottom), with_alpha(base, alpha * 0.22f), 2.5f);
				}

				const int rows = 12;
				float offset = std::fmod(t * 0.35f * cfg.speed, 1.0f);
				for (int i = 0; i <= rows; ++i)
				{
					float u = (float)i / (float)rows + offset;
					u -= std::floor(u);
					const float k = u * u;
					const float y = horizon + (bottom - horizon) * k;
					const float fade = 1.f - k;
					dl->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), with_alpha(base, alpha * pulse * fade * 1.2f), 1.f);
				}

				dl->AddRectFilledMultiColor(
					ImVec2(a.x, horizon - h * 0.05f), ImVec2(b.x, horizon + h * 0.05f),
					with_alpha(base, 0.f), with_alpha(base, 0.f),
					with_alpha(base, alpha * 0.9f), with_alpha(base, alpha * 0.9f));
			}
		}

		inline void render(ImDrawList* dl, const ImVec2& rect_min, const ImVec2& rect_max, const Config& cfg)
		{
			if (!dl)
				return;

			const Effect fx = (Effect)cfg.effect;
			if (fx == Effect::None || fx >= Effect::COUNT)
				return;

			const float w = rect_max.x - rect_min.x;
			const float h = rect_max.y - rect_min.y;
			if (w < 4.f || h < 4.f)
				return;

			using namespace detail;

			ImGuiIO& io = ImGui::GetIO();
			float dt = io.DeltaTime;
			if (!(dt > 0.f) || dt > 0.1f)
				dt = 1.f / 60.f;
			dt *= ImClamp(cfg.speed, 0.05f, 4.0f);

			const float t = (float)ImGui::GetTime();
			const ImU32 base = col4(cfg.color, 1.f);

			dl->PushClipRect(rect_min, rect_max, true);

			if (fx == Effect::Aurora)
			{
				draw_aurora(dl, rect_min, rect_max, cfg, t, base);
				dl->PopClipRect();
				return;
			}
			if (fx == Effect::Grid)
			{
				draw_grid(dl, rect_min, rect_max, cfg, t, base);
				dl->PopClipRect();
				return;
			}
			if (fx == Effect::Topography)
			{
				draw_topography(dl, rect_min, rect_max, cfg, t, base);
				dl->PopClipRect();
				return;
			}

			// The elements belong to the material too: a tinted plate needs less
			// light on it, and a frost scatters the elements behind it, so they lose
			// a little of their peak instead of being drawn at the strength they
			// would have over a sharp screen.
			Config look = cfg;
			look.opacity *= (1.f - 0.35f * ImClamp(cfg.tint, 0.f, 1.f)) *
			                (1.f + 0.15f * ImClamp(cfg.veil, 0.f, 1.f));
			// Fine detail (rain streaks, glyphs, small flakes) is what a frost eats
			// first; growing them slightly keeps the element legible instead of
			// turning it into a smear.
			look.size *= 1.f + 0.35f * ImClamp(cfg.veil, 0.f, 1.f);

			std::vector<particle_t>& ps = pool();
			const int want = ImClamp((int)((float)base_count(fx) * ImClamp(cfg.density, 0.05f, 4.0f)), 1, 900);

			if (last_effect() != cfg.effect)
			{
				ps.clear();
				last_effect() = cfg.effect;
			}
			if ((int)ps.size() != want)
			{
				const std::size_t old = ps.size();
				ps.resize((std::size_t)want);
				for (std::size_t i = old; i < ps.size(); ++i)
					spawn(ps[i], fx, w, h, cfg, true);
			}

			const float wind = ImClamp(cfg.wind, -2.f, 2.f);
			const float margin = 40.f;

			for (particle_t& p : ps)
			{

				switch (fx)
				{
				case Effect::Snow:
					p.x += (wind * 34.f + std::sin(t * 0.9f + p.phase) * 9.f) * dt;
					p.y += p.vy * dt;
					p.rot += p.rot_v * dt;
					break;

				case Effect::Rain:
					// The angle is the direction, not the wind: a slanted fall keeps its
					// slant instead of being carried back upright by a sideways push.
					p.x += p.vx * dt;
					p.y += p.vy * dt;
					break;

				case Effect::Stars:
					p.x += p.vx * dt;
					p.y += p.vy * dt;
					break;

				case Effect::Bokeh:
					p.x += (wind * 12.f + std::sin(t * 0.35f + p.phase) * 6.f) * dt;
					p.y += p.vy * dt;
					break;

				case Effect::Matrix:
					p.y += p.vy * dt;
					break;

				case Effect::Embers:
					p.x += (wind * 18.f + std::sin(t * 1.6f + p.phase) * 12.f) * dt;
					p.y += p.vy * dt;
					p.life = ImClamp(p.life - dt * 0.22f, 0.f, 1.f);
					break;

				case Effect::Confetti:
					p.x += (wind * 46.f + std::sin(t * 1.1f + p.phase) * 26.f) * dt;
					p.y += p.vy * dt;
					p.rot += p.rot_v * dt;
					break;

				case Effect::Tiles:
					p.x += (wind * 16.f + std::sin(t * 0.4f + p.phase) * 10.f) * dt;
					p.y += p.vy * dt;
					p.rot += p.rot_v * dt;
					break;

				default:
					break;
				}

				bool out = false;
				if (p.x < -margin)              { p.x = w + margin * 0.5f; }
				else if (p.x > w + margin)      { p.x = -margin * 0.5f; }
				if (p.y > h + margin)           out = true;
				else if (p.y < -margin * 1.6f)  out = true;
				if (fx == Effect::Embers && p.life <= 0.f) out = true;
				if (out)
					spawn(p, fx, w, h, look, false);

				particle_t pp = p;
				pp.x += rect_min.x;
				pp.y += rect_min.y;

				switch (fx)
				{
				case Effect::Snow:     draw_snow(dl, pp, look, t, base); break;
				case Effect::Rain:     draw_rain(dl, pp, look, base); break;
				case Effect::Stars:    draw_stars(dl, pp, look, t, base); break;
				case Effect::Bokeh:    draw_bokeh(dl, pp, look, base); break;
				case Effect::Matrix:   draw_matrix(dl, pp, look, base); break;
				case Effect::Embers:   draw_ember(dl, pp, look, t, base); break;
				case Effect::Confetti: draw_confetti(dl, pp, look, base); break;
				case Effect::Tiles:    draw_tiles(dl, pp, look, t, base); break;
				default: break;
				}
			}

			dl->PopClipRect();
		}

		inline void reset()
		{
			detail::pool().clear();
			detail::last_effect() = -1;
		}
	}
}
