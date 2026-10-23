#pragma once

// Overlay shapes that ImDrawList cannot draw smoothly on its own.
//
// A thick circle in ImDrawList is a *stroked polyline*: each segment is offset
// by half the thickness along its own normal, and at a join the two offsets do
// not meet - the inside of the bend scallops in and the outside bulges out.  At
// the radii the FOV ring is drawn at (100-360 px) that is plainly visible, and
// it is what "the circle has grooves in it" is.  The same is true of a stroked
// line whose width or colour has to change along its length: there is no
// AddLine form for either.
//
// Both shapes here are built as triangle strips instead - one quad per segment,
// with the inner and outer edge of every quad on the same arc (or the same
// taper) - so the edge is smooth at any width and can fade along its length.

#include <imgui/imgui.h>

#include <cmath>

namespace shape
{
	inline constexpr float kPi = 3.14159265358979f;

	// ---- quality -----------------------------------------------------------
	//
	// The overlay's own anti-aliasing, as one switch its features read instead
	// of each of them deciding for itself.
	//
	// Off, nothing here changes a pixel: strokes sit on whole pixels and are
	// left hard-edged, which is the look every overlay feature was tuned
	// against and the cheapest the overlay draws.
	//
	// On, a stroke is placed on the pixel *centre*.  A 1 px line centred on an
	// integer coordinate covers half of each of two columns - dull, and the
	// whole of "the box looks dirty" - while the same line half a pixel along
	// covers one column exactly, and the anti-aliasing the option turns on then
	// smooths the corners and the joins around it.
	inline bool  g_aa         = false;
	inline float g_aa_quality = 1.f;

	inline void set_quality(bool on, float quality)
	{
		g_aa = on;
		if (!on) { g_aa_quality = 1.f; return; }
		g_aa_quality = quality < 0.5f ? 0.5f : (quality > 2.f ? 2.f : quality);
	}

	inline bool  aa_on()      { return g_aa; }
	inline float aa_quality() { return g_aa ? g_aa_quality : 1.f; }

	// Where a stroke wants to sit, given the two rasterisers: on the pixel edge
	// when the lines are hard-edged, on the pixel centre when they are blended
	// across it.
	inline float snap_stroke(float v)
	{
		return g_aa ? (floorf(v) + 0.5f) : floorf(v + 0.5f);
	}

	// An overlay draw list, brought to the quality the option asks for.
	//
	// The anti-aliased *lines* are always on: that is what the overlay has always
	// drawn with, and it is the flag the style hands every draw list at the start
	// of the frame.  So is the texture line path and the anti-aliased fill - the
	// ImGui defaults, which is why this only ever *adds*: with the option off
	// nothing here takes anything away, and the overlay draws exactly the pixels
	// it drew before it existed.  What it adds is the two flags put back for a
	// list that something in the frame cleared them on.
	inline void apply_flags(ImDrawList* dl)
	{
		if (!dl) return;
		dl->Flags |= ImDrawListFlags_AntiAliasedLines;
		if (g_aa)
			dl->Flags |= ImDrawListFlags_AntiAliasedLinesUseTex | ImDrawListFlags_AntiAliasedFill;
	}

	// Enough segments that the chord error is under a twentieth of a pixel at
	// the radius, capped so a huge ring cannot cost thousands of quads.  The
	// option buys a finer cap: a ring that has been feathered shows its facets
	// before its edges.
	inline int segments_for(float radius)
	{
		const float r = radius > 1.f ? radius : 1.f;
		int n = (int)(r * 1.6f * aa_quality());
		if (n < 24)  n = 24;
		const int cap = g_aa ? 512 : 256;
		if (n > cap) n = cap;
		return n;
	}

	// The same colour at a share of its alpha: what a feather is made of.
	inline ImU32 fade_alpha(ImU32 col, float share)
	{
		const float a = (float)((col >> IM_COL32_A_SHIFT) & 0xFF) * share;
		return (col & 0x00FFFFFFu) |
			((ImU32)(a < 0.f ? 0.f : (a > 255.f ? 255.f : a)) << IM_COL32_A_SHIFT);
	}

	// One annulus, as quads: the body of a ring, with no feather of its own.
	inline void band(ImDrawList* dl, ImVec2 centre, float r_in, float r_out,
		ImU32 col, int segments)
	{
		if (!dl || r_out <= r_in) return;
		if (segments <= 0) segments = segments_for(r_out);

		const float step = 2.f * kPi / (float)segments;
		// The ring is closed, so the last quad shares its edge with the first:
		// every vertex is placed from its own angle rather than reusing the
		// previous one, which is what keeps the seam invisible.
		for (int i = 0; i < segments; ++i)
		{
			const float a0 = (float)i * step;
			const float a1 = a0 + step;
			const float c0 = std::cos(a0), s0 = std::sin(a0);
			const float c1 = std::cos(a1), s1 = std::sin(a1);

			dl->AddQuadFilled(
				ImVec2(centre.x + c0 * r_in,  centre.y + s0 * r_in),
				ImVec2(centre.x + c1 * r_in,  centre.y + s1 * r_in),
				ImVec2(centre.x + c1 * r_out, centre.y + s1 * r_out),
				ImVec2(centre.x + c0 * r_out, centre.y + s0 * r_out), col);
		}
	}

	// A ring: an annulus between r_in and r_out around `centre`.
	//
	// A quad strip cannot be anti-aliased by ImDrawList - the flag only reaches
	// the stroked polylines and the convex fills - so with the overlay's
	// anti-aliasing on the soft edge is *drawn*: a hair of the same colour at a
	// third of its alpha on each side of the band, which is what turns a
	// stair-stepped circumference into a soft one.
	inline void ring_band(ImDrawList* dl, ImVec2 centre, float r_in, float r_out,
		ImU32 col, int segments = 0)
	{
		if (!dl || r_out <= r_in) return;
		if (segments <= 0) segments = segments_for(r_out);

		if (g_aa && (col & IM_COL32_A_MASK) != 0)
		{
			const float f = 0.6f + 0.5f * g_aa_quality;
			const ImU32 soft = fade_alpha(col, 0.38f);
			band(dl, centre, r_out, r_out + f, soft, segments);
			band(dl, centre, r_in - f, r_in, soft, segments);
		}

		band(dl, centre, r_in, r_out, col, segments);
	}

	// A disc with a caller-chosen segment count, so a filled circle can be made
	// to match the ring around it.  AddCircleFilled goes through the convex fill
	// path, so the anti-aliased fill flag the option sets reaches it and the
	// disc's own edge is smoothed without a feather.
	inline void disc(ImDrawList* dl, ImVec2 centre, float radius, ImU32 col, int segments = 0)
	{
		if (!dl || radius <= 0.f) return;
		if (segments <= 0) segments = segments_for(radius);
		dl->AddCircleFilled(centre, radius, col, segments);
	}

	// The quads of a tapered band, and nothing else: the body of tapered_band,
	// so the feathered copy of it below can be the same shape at another width.
	inline void tapered_quads(ImDrawList* dl, ImVec2 a, ImVec2 b,
		float width, ImU32 col_start, ImU32 col_end, float tip_scale)
	{
		if (!dl) return;

		const float dx = b.x - a.x, dy = b.y - a.y;
		const float len = std::sqrt(dx * dx + dy * dy);
		if (len < 1.f || width <= 0.f) return;

		const float ux = dx / len, uy = dy / len;
		const float nx = -uy, ny = ux;

		// The taper is a gradient along the band, so the segments are its
		// resolution: a finer step is a smoother fade, and the option buys one.
		int segs = (int)(len / (g_aa ? 3.f : 6.f));
		if (segs < 8)  segs = 8;
		const int seg_cap = g_aa ? 128 : 64;
		if (segs > seg_cap) segs = seg_cap;

		for (int i = 0; i < segs; ++i)
		{
			const float t0 = (float)i / (float)segs;
			const float t1 = (float)(i + 1) / (float)segs;
			const float w0 = width * (1.f + (tip_scale - 1.f) * t0) * 0.5f;
			const float w1 = width * (1.f + (tip_scale - 1.f) * t1) * 0.5f;

			const ImVec2 p0(a.x + ux * len * t0, a.y + uy * len * t0);
			const ImVec2 p1(a.x + ux * len * t1, a.y + uy * len * t1);

			// The colour is sampled at the segment and applied to the whole
			// quad: at 8+ segments per line that is a gradient to the eye, and
			// it keeps every vertex on the same two edges.
			const float tm = (t0 + t1) * 0.5f;
			const auto mix = [](ImU32 x, ImU32 y, float t) -> ImU32
			{
				const float xa = (float)((x >> IM_COL32_A_SHIFT) & 0xFF);
				const float ya = (float)((y >> IM_COL32_A_SHIFT) & 0xFF);
				const float xr = (float)((x >> IM_COL32_R_SHIFT) & 0xFF);
				const float yr = (float)((y >> IM_COL32_R_SHIFT) & 0xFF);
				const float xg = (float)((x >> IM_COL32_G_SHIFT) & 0xFF);
				const float yg = (float)((y >> IM_COL32_G_SHIFT) & 0xFF);
				const float xb = (float)((x >> IM_COL32_B_SHIFT) & 0xFF);
				const float yb = (float)((y >> IM_COL32_B_SHIFT) & 0xFF);
				return IM_COL32((int)(xr + (yr - xr) * t), (int)(xg + (yg - xg) * t),
					(int)(xb + (yb - xb) * t), (int)(xa + (ya - xa) * t));
			};
			const ImU32 c = mix(col_start, col_end, tm);

			dl->AddQuadFilled(
				ImVec2(p0.x + nx * w0, p0.y + ny * w0),
				ImVec2(p1.x + nx * w1, p1.y + ny * w1),
				ImVec2(p1.x - nx * w1, p1.y - ny * w1),
				ImVec2(p0.x - nx * w0, p0.y - ny * w0), c);
		}

	}

	// ---- FOV circle variants -------------------------------------------
	//
	// `style` is the source-named entry from the menu:
	//   0 ring     (Aether)      the concentric-band circle
	//   1 dots      beads orbiting the radius
	//   2 dynamic   beads that warm while a visible player is inside
	//   3 pulse          the ring with waves leaving it
	//   4 arc          the ring, a white inner ring, three arcs turning
	//
	// `hot` is the caller's visibility answer for style 2 - it is not computed
	// here because the ring does not know about players, only about pixels.
	inline void fov_circle(ImDrawList* dl, ImVec2 centre, float radius,
		ImU32 col, ImU32 outline, float thickness, bool outline_on, int style,
		bool fill, ImU32 fill_col, float fill_alpha, bool hot)
	{
		if (!dl || radius <= 0.f) return;

		const float th = thickness < 0.5f ? 0.5f : (thickness > 12.f ? 12.f : thickness);
		const int   segs = segments_for(radius + th + 1.f);
		const float now = (float)ImGui::GetTime();

		if (fill && (fill_col & IM_COL32_A_MASK) != 0)
			disc(dl, centre, radius - th * 0.5f, fill_col, segs);

		if (style == 1 || style == 2)
		{
			const float circ = 6.2831853f * radius;
			int count = (int)(circ / 10.f);
			if (count < 12) count = 12;
			if (count > 128) count = 128;

			float dot_r = 1.5f + radius * 0.004f;
			if (dot_r > 3.f) dot_r = 3.f;

			const ImU32 dot_col = (style == 2 && !hot) ? fade_alpha(col, 0.38f) : col;
			const float spin = now * 0.4f;

			for (int i = 0; i < count; ++i)
			{
				const float ang = spin + (float)i / (float)count * 6.2831853f;
				const ImVec2 p(centre.x + std::cos(ang) * radius,
					centre.y + std::sin(ang) * radius);
				if (outline_on)
					disc(dl, p, dot_r + 1.f, outline, 8);
				disc(dl, p, dot_r, dot_col, 8);
			}
			return;
		}

		if (style == 3)
		{
			if (outline_on)
			{
				const float r0 = radius + th * 0.5f;
				band(dl, centre, r0, r0 + th + 1.f,
					(outline & 0x00FFFFFFu) | 0xFF000000u, segs);
			}
			band(dl, centre, radius - th * 0.5f, radius + th * 0.5f, col, segs);

			// Three waves leaving the ring, out of phase, so there is always one
			// on its way out.  They carry no hit information - this style is
			// decoration, which is what the ring is.
			for (int i = 0; i < 3; ++i)
			{
				float p = std::fmod(now * 0.75f + (float)i / 3.f, 1.f);
				if (p < 0.f) p += 1.f;
				const float r = radius + 2.f + p * 16.f;
				band(dl, centre, r, r + 1.6f, fade_alpha(col, (1.f - p) * 0.55f), segs);
			}
			return;
		}

		if (style == 4)
		{
			// the ring: the accent circle, a hairline white circle just
			// inside it, and three arcs sweeping outside it, one third of a turn
			// apart.  The arcs are the only moving part - the radius never does.
			if (outline_on)
			{
				const float r0 = radius + th * 0.5f;
				band(dl, centre, r0, r0 + th + 1.f,
					(outline & 0x00FFFFFFu) | 0xFF000000u, segs);
			}
			band(dl, centre, radius - th * 0.5f, radius + th * 0.5f, col, segs);
			band(dl, centre, radius - 2.6f, radius - 1.8f, fade_alpha(IM_COL32_WHITE, 0.10f), segs);

			const float spin = now * 1.4f;
			const float arc  = 3.14159265f * 0.34f;
			for (int i = 0; i < 3; ++i)
			{
				const float start = spin + (float)i * 2.09439510f;
				dl->PathArcTo(centre, radius + 4.f, start, start + arc, 18);
				dl->PathStroke(fade_alpha(col, i == 1 ? 0.72f : 0.78f), 0, 2.1f);
			}
			return;
		}

		// style 0: the ring every other feature was tuned against.
		if (outline_on)
		{
			const float ow = th + 1.f;
			const float r0 = radius + th * 0.5f;
			band(dl, centre, r0, r0 + ow, (outline & 0x00FFFFFFu) | 0xFF000000u, segs);
		}
		band(dl, centre, radius - th * 0.5f, radius + th * 0.5f, col, segs);
	}

	// A straight band from a to b: `width` px wide at a, tapering to
	// `width * tip_scale` at b, fading from col_start to col_end, with round
	// caps.  This is the smooth style of a tracer - one shape, both ends
	// rounded, and no join anywhere along it.
	//
	// `outline` is drawn first, as the same band a little wider with a blunt
	// cap, so the colour keeps its own width instead of being eaten by it.
	//
	// Both edges of the band are quads, and ImDrawList cannot anti-alias a quad -
	// so with the overlay's anti-aliasing on the soft edge is a wider, faint copy
	// of the whole shape underneath the real one, which is what turns the band's
	// two hard edges into a stroke.
	inline void tapered_band(ImDrawList* dl, ImVec2 a, ImVec2 b,
		float width, ImU32 col_start, ImU32 col_end, float tip_scale = 0.6f)
	{
		if (!dl || width <= 0.f)
			return;

		if (g_aa)
		{
			const float f = 0.7f + 0.6f * g_aa_quality;
			tapered_quads(dl, a, b, width + f * 2.f,
				fade_alpha(col_start, 0.34f), fade_alpha(col_end, 0.34f), tip_scale);
		}

		tapered_quads(dl, a, b, width, col_start, col_end, tip_scale);

		// Round caps: the segments above meet at a blunt edge, and a disc at
		// each end turns the band into a stroke.
		const int cap_segs = (int)(20.f * aa_quality());
		disc(dl, a, width * 0.5f, col_start, cap_segs);
		disc(dl, b, width * tip_scale * 0.5f, col_end, cap_segs);
	}

	// ---- tracer variants ----------------------------------------------
	//
	// The same segment, drawn the way each source tree drew it.  All three are
	// here rather than in the callers so the aim tracer, the ESP tracer and the
	// bullet tracer cannot drift apart about what a style means.

	// A beam: the colour at three widths, wide and faint to narrow and solid.
	// The tracer is this stack; it reads as a glow around a core
	// rather than a line with an outline.
	inline void beam_line(ImDrawList* dl, ImVec2 a, ImVec2 b, float width, ImU32 col)
	{
		if (!dl || width <= 0.f) return;
		const ImU32 outer = fade_alpha(col, 0.16f);
		const ImU32 mid   = fade_alpha(col, 0.42f);
		tapered_band(dl, a, b, width * 3.4f, outer, outer, 1.f);
		tapered_band(dl, a, b, width * 1.9f, mid, mid, 1.f);
		tapered_band(dl, a, b, width, col, col, 1.f);
	}

	// A laser: dashes travelling toward the target.  `phase` is seconds; the
	// dashes are offset by it so the pattern moves.
	inline void laser_line(ImDrawList* dl, ImVec2 a, ImVec2 b, float width,
		ImU32 col, float phase)
	{
		if (!dl || width <= 0.f) return;

		const float dx = b.x - a.x, dy = b.y - a.y;
		const float len = std::sqrt(dx * dx + dy * dy);
		if (len < 2.f) return;

		const float ux = dx / len, uy = dy / len;
		const float dash = 10.f + width * 4.f;
		const float gap  = dash * 0.85f;
		const float step = dash + gap;
		float offset = std::fmod(phase * 90.f, step);
		if (offset < 0.f) offset += step;

		const ImU32 dim = fade_alpha(col, 0.45f);

		for (float d = -offset; d < len; d += step)
		{
			const float d0 = d < 0.f ? 0.f : d;
			const float d1 = d + dash;
			if (d1 <= 0.f) continue;
			const float e1 = d1 > len ? len : d1;
			if (e1 <= d0) continue;

			const ImVec2 p0(a.x + ux * d0, a.y + uy * d0);
			const ImVec2 p1(a.x + ux * e1, a.y + uy * e1);

			// The head of each dash is brighter than its tail: that is what
			// makes a still image of a dash read as moving.
			tapered_band(dl, p0, p1, width, col, dim, 1.f);
		}
	}

	// A dotted tracer: a bead every few pixels, sized from the width.
	inline void dotted_line(ImDrawList* dl, ImVec2 a, ImVec2 b, float width, ImU32 col)
	{
		if (!dl || width <= 0.f) return;

		const float dx = b.x - a.x, dy = b.y - a.y;
		const float len = std::sqrt(dx * dx + dy * dy);
		if (len < 2.f) return;

		const float step  = 6.f + width * 2.4f;
		const float bead  = width * 0.62f + 0.4f;
		const int   count = (int)(len / step) + 1;

		for (int i = 0; i <= count; ++i)
		{
			const float t = (float)i / (float)(count > 0 ? count : 1);
			const ImVec2 p(a.x + dx * t, a.y + dy * t);
			disc(dl, p, bead + 0.9f, fade_alpha(IM_COL32(0, 0, 0, 220), 0.8f), 10);
			disc(dl, p, bead, col, 10);
		}
	}
}
