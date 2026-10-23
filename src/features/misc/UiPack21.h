#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>
#include <features/misc/TextSheen.h>
#include <render/SurfaceTex.h>

#include <cmath>
#include <cstdio>
#include <cstring>

// The surface effects pack.
//
// Everything here is opt-in twice: the master switch is off by default and so
// is every individual effect, so the panel is byte-for-byte the plain plate
// until the user asks for a light rig.
//
// The one rule the drawing follows: light has no edges.  A rectangle of white
// at low alpha over a dark plate is a grey box - it reads as a smudge, not as
// light - and a stack of them reads as banded glass.  So every soft shape here
// is a real gradient: a triangle fan whose centre carries the colour and whose
// rim is fully transparent.  Two or three of those at different radii give a
// falloff with no bands and no borders, for the cost of a few dozen vertices.
namespace ui21
{
	inline bool on() { return settings::ui21::enabled; }

	// 1 while the plate is drawing over a frosted capture, 0 while it is the
	// plain dark fill.  A light rig over frost has to work against a *bright*
	// backdrop rather than a black one: the same alpha that reads as a soft shaft
	// on black disappears into a blurred desktop, and the accent wash goes pale.
	// So while the frost is up the pack deepens the top edge a little first and
	// then lifts the light, instead of drawing the black-plate values and hoping.
	// This is an accommodation only - it reads the backdrop's state, it does not
	// touch the blur, the tint or the transparency.
	inline float g_frost = 0.f;

	// How much the backdrop's own Tint is darkening the capture underneath.  The
	// light rig does not have to lift a plate that is already dark, so a tinted
	// glass gets a little less gain, not more - the tint and the light are doing
	// the same job and two of them at full strength is a grey wash.
	inline float g_tint = 0.f;

	// `veil` is the blur's own strength (0..1): a heavy frost scatters more, so
	// the lights lift a little to stay readable through it.
	inline void set_frost(bool frosted, float veil = 1.f, float tint = 0.f)
	{
		g_frost = frosted ? ImClamp(veil, 0.15f, 1.f) : 0.f;
		g_tint  = ImClamp(tint, 0.f, 1.f);
	}

	inline float frost_gain()
	{
		return (1.f + 0.55f * g_frost) * (1.f - 0.28f * g_tint);
	}

	inline ImU32 rgba(const ImVec4& c, float a)
	{
		ImVec4 t = c; t.w = a;
		return ImGui::GetColorU32(t);
	}

	inline ImU32 blend(ImU32 a, ImU32 b, float t)
	{
		t = ImClamp(t, 0.f, 1.f);
		const int ac[4] = { (int)((a >> 24) & 0xFF), (int)((a >> 16) & 0xFF), (int)((a >> 8) & 0xFF), (int)(a & 0xFF) };
		const int bc[4] = { (int)((b >> 24) & 0xFF), (int)((b >> 16) & 0xFF), (int)((b >> 8) & 0xFF), (int)(b & 0xFF) };
		return ((ImU32)(ac[0] + (bc[0] - ac[0]) * t) << 24) | ((ImU32)(ac[1] + (bc[1] - ac[1]) * t) << 16)
			| ((ImU32)(ac[2] + (bc[2] - ac[2]) * t) << 8) | (ImU32)(ac[3] + (bc[3] - ac[3]) * t);
	}

	inline ImVec4 accent_a()
	{
		return ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
	}

	inline ImVec4 accent_b()
	{
		const float* c = settings::ui21::accent_2;
		return ImVec4(c[0], c[1], c[2], c[3]);
	}

	// ---------------------------------------------------------------- light
	//
	// A radial gradient drawn as one mesh: the centre vertex has the colour, a
	// ladder of rings carries a gaussian of it, and the outermost ring is fully
	// transparent - so there is no rim, and the falloff between the rings is a
	// curve rather than the four steps it used to be.  `rx`/`ry` make it
	// elliptical, which is how a beam hanging from an edge is drawn: an ellipse
	// anchored above the plate, clipped by the plate.
	inline void soft_light(ImDrawList* dl, ImVec2 c, float rx, float ry, ImU32 col, int seg = 40)
	{
		if (!dl || rx < 0.5f || ry < 0.5f)
			return;

		const int a0 = (col >> IM_COL32_A_SHIFT) & 0xFF;
		if (a0 <= 0)
			return;

		// A mesh, not a sampled texture, and a ladder of rings rather than three.
		//
		// Three rings is four alpha levels across three bands, which on anything
		// wider than a button is four concentric edges you can point at: the
		// "rings" the plate, the cards and the hover discs were all made of.  The
		// falloff here is a gaussian sampled at every ring, and the ring count
		// grows with the light - six on a 12 px hover disc, twelve on a wide
		// bloom - so a gradient of fifty steps is drawn where there used to be
		// three.  The vertex cost stays around two hundred per light.
		seg = seg < 12 ? 12 : (seg > 40 ? 40 : seg);

		const float big = rx > ry ? rx : ry;
		int rings = 6 + (int)(big * 0.04f);
		if (rings < 7) rings = 7;
		if (rings > 12) rings = 12;
		// A 400 px fan needs fewer segments than a small one: at 40 the chord is
		// the eye's problem, not the alpha's, and the middle rings are where the
		// polygon would show.
		if (big > 220.f && seg > 32) seg = 32;

		const ImU32 rgb  = col & 0x00FFFFFFu;
		const ImVec2 uv  = dl->_Data->TexUvWhitePixel;

		// exp(-k t^2) normalised so the outermost ring is exactly transparent:
		// no rim, and the curve between the rings is monotonic, so no ring can
		// read as a separate disc.
		const float k    = 3.2f;
		const float tail = expf(-k);
		const float inv  = 1.f / (1.f - tail);

		dl->PrimReserve(3 * seg * (2 * rings - 1), 1 + rings * seg);
		const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;

		dl->PrimWriteVtx(c, uv, col);
		for (int r = 1; r <= rings; ++r)
		{
			const float t  = (float)r / (float)rings;
			const float g  = (expf(-k * t * t) - tail) * inv;
			const int   al = (int)(a0 * g + 0.5f);
			const ImU32 ringCol = rgb | ((ImU32)al << IM_COL32_A_SHIFT);
			for (int i = 0; i < seg; ++i)
			{
				const float a = (float)i / (float)seg * 6.28318531f;
				dl->PrimWriteVtx(ImVec2(c.x + cosf(a) * rx * t, c.y + sinf(a) * ry * t), uv, ringCol);
			}
		}

		for (int i = 0; i < seg; ++i)
		{
			dl->PrimWriteIdx(base);
			dl->PrimWriteIdx((ImDrawIdx)(base + 1 + i));
			dl->PrimWriteIdx((ImDrawIdx)(base + 1 + (i + 1) % seg));
		}
		for (int r = 0; r < rings - 1; ++r)
		{
			const ImDrawIdx in  = (ImDrawIdx)(base + 1 + r * seg);
			const ImDrawIdx out = (ImDrawIdx)(base + 1 + (r + 1) * seg);
			for (int i = 0; i < seg; ++i)
			{
				const int j = (i + 1) % seg;
				dl->PrimWriteIdx((ImDrawIdx)(in + i));  dl->PrimWriteIdx((ImDrawIdx)(out + i)); dl->PrimWriteIdx((ImDrawIdx)(out + j));
				dl->PrimWriteIdx((ImDrawIdx)(in + i));  dl->PrimWriteIdx((ImDrawIdx)(out + j)); dl->PrimWriteIdx((ImDrawIdx)(in + j));
			}
		}
	}

	// Two stacked fans: a wide, faint one and a tighter, brighter one.  The
	// sum is close enough to a gaussian to read as a light source rather than a
	// disc with a visible edge.
	inline void glow(ImDrawList* dl, ImVec2 c, float r, ImU32 col, float alpha, float aspect = 1.f)
	{
		const float a = ImClamp(alpha, 0.f, 1.f);
		if (a <= 0.002f || r < 1.f)
			return;

		const int alpha_byte = (col >> IM_COL32_A_SHIFT) & 0xFF;
		const auto with = [col](float k)
		{
			const int a8 = (int)ImClamp((float)((col >> IM_COL32_A_SHIFT) & 0xFF) * k, 0.f, 255.f);
			return (col & 0x00FFFFFFu) | ((ImU32)a8 << IM_COL32_A_SHIFT);
		};
		(void)alpha_byte;

		soft_light(dl, c, r, r * aspect, with(a * 0.34f), 48);
		soft_light(dl, c, r * 0.62f, r * 0.62f * aspect, with(a * 0.42f), 40);
		soft_light(dl, c, r * 0.34f, r * 0.34f * aspect, with(a * 0.50f), 32);
	}

	// A lit top edge: the source the shafts are supposed to come from.  It is
	// an ellipse anchored *above* the plate, so only its lower half lands on
	// the glass and the brightest part is off-plate - which is what stops it
	// reading as a bar of white along the top.
	inline void top_light(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, float strength)
	{
		if (!dl || !on() || !settings::ui21::light_rays)
			return;

		const float w = mx.x - mn.x;
		const float h = mx.y - mn.y;
		if (w < 80.f || h < 80.f)
			return;

		const float s = ImClamp(strength, 0.f, 1.f);
		const float t = (float)ImGui::GetTime();
		const float gain = frost_gain();

		const ImU32 white = IM_COL32(236, 242, 255, 255);
		const ImU32 cool  = rgba(accent_a(), 1.f);

		dl->PushClipRect(ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), true);

		// Over frost the lip needs depth before it needs light: one very soft
		// dark band under the top edge, so the shafts have something to stand on.
		if (g_frost > 0.01f)
			soft_light(dl, ImVec2(mn.x + w * 0.5f, mn.y - 10.f), w * 0.75f, 120.f,
				IM_COL32(0, 0, 0, (ImU32)(46.f * s * g_frost)), 64);

		// The wash along the lip, breathing very slowly so it is alive without
		// being a pulse.
		const float breathe = 0.86f + 0.14f * sinf(t * 0.35f);
		soft_light(dl, ImVec2(mn.x + w * 0.5f, mn.y - 16.f), w * 0.62f, 66.f,
			(white & 0x00FFFFFFu) | ((ImU32)ImClamp(70.f * s * breathe * gain, 0.f, 255.f) << IM_COL32_A_SHIFT), 64);

		// Shafts.  Each is one wide ellipse whose centre sits just above the
		// top edge, leaning a little as it falls, so the light fans *down* off
		// the lip.  Because the ellipse's own edge is transparent there is no
		// box: only the middle of the fan is ever visible.
		constexpr int kRays = 6;
		for (int i = 0; i < kRays; ++i)
		{
			const float seed = (float)i * 1.913f;
			const float fx = (float)(i + 0.5f) / (float)kRays
				+ 0.035f * sinf(t * 0.11f + seed);
			const float len = h * (0.55f + 0.30f * (0.5f + 0.5f * sinf(seed * 2.7f)));
			const float half_w = w * (0.055f + 0.030f * (0.5f + 0.5f * sinf(seed * 1.7f)));
			const float lean = 0.10f * w * sinf(t * 0.07f + seed * 0.6f);
			const float flick = 0.66f + 0.34f * sinf(t * 0.33f + seed * 2.1f);

			const ImVec2 apex(mn.x + fx * w + lean, mn.y - 10.f);
			const ImU32 tint = (i & 1) ? cool : white;

			// Three nested fans per shaft: the wide haze, the body, the core.
			// Additive-looking because each layer is faint and they overlap.
			const int a0 = (int)ImClamp(10.f * s * flick * gain, 0.f, 255.f);
			const int a1 = (int)ImClamp(13.f * s * flick * gain, 0.f, 255.f);
			const int a2 = (int)ImClamp(15.f * s * flick * gain, 0.f, 255.f);

			soft_light(dl, apex, half_w * 2.4f, len * 1.25f,
				(tint & 0x00FFFFFFu) | ((ImU32)a0 << IM_COL32_A_SHIFT), 40);
			soft_light(dl, apex, half_w * 1.35f, len * 1.05f,
				(tint & 0x00FFFFFFu) | ((ImU32)a1 << IM_COL32_A_SHIFT), 36);
			soft_light(dl, apex, half_w * 0.62f, len * 0.82f,
				(tint & 0x00FFFFFFu) | ((ImU32)a2 << IM_COL32_A_SHIFT), 28);
		}

		dl->PopClipRect();
	}

	// ------------------------------------------------------------- backdrop

	// Two or three very broad colour fields drifting behind the content.  Not a
	// gradient image: three soft fans, the accent and its companion, at very
	// low alpha, moving on their own slow orbits.
	inline void aurora(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding)
	{
		if (!dl || !on() || !settings::ui21::aurora)
			return;

		const float strength = ImClamp(settings::ui21::aurora_strength, 0.f, 1.f);
		if (strength < 0.01f || mx.x - mn.x < 60.f || mx.y - mn.y < 60.f)
			return;

		const float w = mx.x - mn.x;
		const float h = mx.y - mn.y;
		const float t = (float)ImGui::GetTime();
		const ImVec4 a = accent_a();
		const ImVec4 b = accent_b();

		dl->PushClipRect(ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), true);

		struct blob_t { ImVec4 col; float x, y, r, phase, speed; };
		const blob_t blobs[3] = {
			{ a, 0.20f, 0.10f, 0.72f, 0.0f, 0.11f },
			{ b, 0.82f, 0.22f, 0.62f, 2.1f, 0.08f },
			{ ImVec4(a.x * 0.35f + b.x * 0.65f, a.y * 0.35f + b.y * 0.65f,
			          a.z * 0.35f + b.z * 0.65f, 1.f), 0.52f, 0.92f, 0.78f, 4.2f, 0.09f },
		};

		for (const blob_t& bl : blobs)
		{
			const float bx = mn.x + w * (bl.x + 0.05f * sinf(t * bl.speed + bl.phase));
			const float by = mn.y + h * (bl.y + 0.04f * cosf(t * bl.speed * 0.8f + bl.phase));
			const float br = ImMin(w, h) * bl.r;

			glow(dl, ImVec2(bx, by), br, rgba(bl.col, 1.f), 0.10f * strength * frost_gain(), 0.72f);
		}

		dl->PopClipRect();
	}

	// A halo that hugs the plate's own curve: light spilling *out* of the edge,
	// not a second border.  Each step is one pixel further out and fainter than
	// the last, so the falloff is continuous and there is no line to notice.
	inline void edge_bloom(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding)
	{
		if (!dl || !on() || !settings::ui21::card_glow)
			return;

		const float s = ImClamp(settings::ui21::card_glow_strength, 0.f, 1.f);
		if (s < 0.01f)
			return;

		const ImU32 acc = rgba(accent_a(), 1.f) & 0x00FFFFFFu;
		// Sixteen steps at nine tenths of a pixel, not seven at a pixel and a
		// half.  Seven outlines that far apart are seven lines you can count
		// around the plate - the "rings" the edge was made of - because each one
		// is a hard one pixel stroke with a gap between it and the next.  At this
		// spacing the strokes overlap each other, and what accumulates is a fall
		// rather than a set of concentric rules.
		constexpr int kSteps = 16;
		for (int i = 0; i < kSteps; ++i)
		{
			const float k = 1.f - (float)i / (float)kSteps;
			const float g = 0.6f + (float)i * 0.85f;
			const int a = (int)(11.f * s * k * k * frost_gain());
			if (a <= 0)
				continue;
			dl->AddRect(ImVec2(mn.x - g, mn.y - g), ImVec2(mx.x + g, mx.y + g),
				acc | ((ImU32)ImClamp(a, 0, 255) << IM_COL32_A_SHIFT), rounding + g, 0, 1.15f);
		}
	}

	inline void dot_grid(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding)
	{
		if (!on() || !dl || !settings::ui21::dot_grid)
			return;
		const float spacing = ImClamp(settings::ui21::dot_grid_spacing, 6.f, 80.f);
		if (mx.x - mn.x < spacing * 2.f || mx.y - mn.y < spacing * 2.f)
			return;

		const ImU32 col = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.05f * (1.f + 0.7f * g_frost)));

		const float x0 = mn.x + rounding * 0.5f;
		const float y0 = mn.y + rounding * 0.5f;
		const float x1 = mx.x - rounding * 0.5f;
		const float y1 = mx.y - rounding * 0.5f;
		dl->PushClipRect(ImVec2(x0, y0), ImVec2(x1, y1), true);
		const int nx = (int)((x1 - x0 - 1.f) / spacing);
		const int ny = (int)((y1 - y0 - 1.f) / spacing);
		for (int iy = 0; iy <= ny; ++iy)
			for (int ix = 0; ix <= nx; ++ix)
				dl->AddRectFilled(ImVec2(x0 + ix * spacing, y0 + iy * spacing),
					ImVec2(x0 + ix * spacing + 1.f, y0 + iy * spacing + 1.f), col);
		dl->PopClipRect();
	}

	// A soft dark falloff under the plate.  Black, not white: this is the
	// shadow the panel sits in, and it is the one decoration that reads as
	// depth rather than as another sheet of glass.
	inline void panel_shadow(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding)
	{
		if (!dl || !on() || !settings::ui21::panel_shadow)
			return;

		const float s = ImClamp(settings::ui21::panel_shadow_strength, 0.f, 1.f);
		const float spread = 18.f;
		for (int i = 0; i < 8; ++i)
		{
			const float k = 1.f - (float)i / 8.f;
			const float g = spread * k;
			const int a = (int)(18.f * s * (1.f - k) * 2.2f);
			dl->AddRect(ImVec2(mn.x - g, mn.y - g + 3.f), ImVec2(mx.x + g, mx.y + g + 3.f),
				IM_COL32(0, 0, 0, a), rounding + g, 0, g);
		}
	}

	// Two more soft fans over the plate's corners.  This is the "glass lit from
	// one side" read: the top-left is warmer, the bottom-right stays cold, and
	// neither has an edge anywhere.
	inline void corner_light(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding)
	{
		if (!dl || !on() || !settings::ui21::top_sheen)
			return;

		const float w = mx.x - mn.x;
		const float h = mx.y - mn.y;
		if (w < 120.f || h < 120.f)
			return;

		const float t = (float)ImGui::GetTime() * ImMax(0.02f, settings::ui21::top_sheen_speed);

		const float gain = frost_gain();
		dl->PushClipRect(ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), true);
		soft_light(dl, ImVec2(mn.x + w * 0.22f, mn.y + 4.f), w * 0.34f, h * 0.30f,
			IM_COL32(255, 255, 255, (ImU32)ImClamp(14.f * gain, 0.f, 255.f)), 48);
		soft_light(dl, ImVec2(mx.x - w * 0.16f, mx.y - 2.f), w * 0.30f, h * 0.26f,
			(rgba(accent_a(), 1.f) & 0x00FFFFFFu) | ((ImU32)ImClamp(12.f * gain, 0.f, 255.f) << IM_COL32_A_SHIFT), 44);

		// One slow highlight crossing the lip, as a fan rather than a band.
		const float phase = fmodf(t, 1.f);
		const float x = mn.x - w * 0.2f + (w * 1.4f) * phase;
		soft_light(dl, ImVec2(x, mn.y + 1.f), w * 0.12f, 26.f,
			IM_COL32(255, 255, 255, (ImU32)ImClamp(34.f * gain, 0.f, 255.f)), 32);
		dl->PopClipRect();
	}

	// --------------------------------------------------------------- cursor

	// The pointer, as a soft light on the plate.  This replaces the old rim
	// chase, which drew two 1.6 px bars along whichever edge the cursor was
	// nearest - a pair of white sticks rather than a light.
	inline void cursor_light(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding,
		ImVec2 cursor, float size = 1.f)
	{
		if (!dl || !on() || !settings::ui21::spotlight_border)
			return;
		if (mx.x - mn.x < 60.f || mx.y - mn.y < 60.f)
			return;

		const float w = mx.x - mn.x;
		const float h = mx.y - mn.y;

		// Outside the plate the light has nothing to fall on, so it is parked
		// just inside the nearest edge instead - and it fades as the pointer
		// leaves, which is what makes it feel attached to the hand.
		const float inset = 26.f;
		const ImVec2 c(ImClamp(cursor.x, mn.x + inset, mx.x - inset),
			ImClamp(cursor.y, mn.y + inset, mx.y - inset));

		const float dist = ImMax(ImFabs(cursor.x - c.x), ImFabs(cursor.y - c.y));
		const float fall = 1.f - ImClamp((dist - 8.f) / 120.f, 0.f, 1.f);
		if (fall <= 0.01f)
			return;

		const float r = 150.f * ImClamp(size, 0.2f, 3.f);

		dl->PushClipRect(ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), true);
		glow(dl, c, r, IM_COL32(232, 238, 255, 255), 0.055f * fall * frost_gain(), 0.85f);
		dl->PopClipRect();

		// The rim keeps its even hairline; only a whisper of the same light is
		// added where the cursor is closest, drawn as a fan under the border so
		// no piece of it is a bar.
		const float edge_d = ImMin(ImMin(ImFabs(cursor.x - mn.x), ImFabs(mx.x - cursor.x)),
			ImMin(ImFabs(cursor.y - mn.y), ImFabs(mx.y - cursor.y)));
		if (edge_d < 24.f)
		{
			const float k = 1.f - ImClamp(edge_d / 24.f, 0.f, 1.f);
			dl->PushClipRect(ImVec2(mn.x, mn.y), ImVec2(mx.x, mx.y), true);
			soft_light(dl, ImVec2(ImClamp(cursor.x, mn.x, mx.x), ImClamp(cursor.y, mn.y, mx.y)),
				90.f * k, 90.f * k, IM_COL32(255, 255, 255, (ImU32)(22.f * k)), 40);
			dl->PopClipRect();
		}
	}

	// -------------------------------------------------------------- surfaces

	inline void glass_face(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, float depth = 1.f)
	{
		if (!on() || !dl || !settings::ui21::glass_layers)
			return;
		const float w = mx.x - mn.x, h = mx.y - mn.y;
		if (w < 8.f || h < 8.f)
			return;

		const float d = ImClamp(depth * settings::ui21::glass_depth, 0.f, 2.f);
		const ImVec4 b = accent_b();

		dl->PushClipRect(mn, mx, true);

		// A faint wash from the top-left, and a little bounce of the companion
		// accent in the far corner.  Both are fans, so the face of a plate has
		// no gradient seam across it.
		soft_light(dl, ImVec2(mn.x + w * 0.30f, mn.y + 1.f), w * 0.62f, h * 0.42f,
			IM_COL32(255, 255, 255, (ImU32)((int)(9.f * d))), 48);
		soft_light(dl, ImVec2(mx.x, mx.y), w * 0.45f, h * 0.40f,
			rgba(b, 0.05f * d), 40);

		dl->PopClipRect();

		if (settings::ui21::noise && settings::ui21::noise_amount > 0.01f)
		{
			const float amount = ImClamp(settings::ui21::noise_amount, 0.f, 1.f);
			unsigned s = (unsigned)((size_t)dl * 2654435761u) ^ 0x5bf03635u;
			if (s == 0u) s = 0x2545F491u;
			const auto rnd = [&s]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; };

			dl->PushClipRect(mn, mx, true);
			const int count = (int)ImClamp(w * h / 620.f * amount, 0.f, 900.f);
			const ImU32 dim = IM_COL32(0, 0, 0, (int)(6.f + 14.f * amount));
			for (int i = 0; i < count; ++i)
			{
				const float x = mn.x + 1.f + (float)(rnd() % 99991u) / 99991.f * (w - 2.f);
				const float y = mn.y + 1.f + (float)(rnd() % 99991u) / 99991.f * (h - 2.f);
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + 1.f, y + 1.f), dim);
			}
			dl->PopClipRect();
		}

		dl->AddRect(mn, mx, IM_COL32(255, 255, 255, 22), rounding, 0, 1.f);
	}

	inline void gradient_text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos,
		const char* text, float t = 0.f, float alpha = 1.f)
	{
		if (!on() || !dl || !font || !text || !text[0] || size <= 0.f)
			return;

		if (!settings::ui21::gradient_heading)
		{
			dl->AddText(font, size, pos, rgba(accent_a(), alpha), text);
			return;
		}

		const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, text);
		if (ts.x <= 1.f)
			return;

		const ImVec4 a = accent_a();
		const ImVec4 b = accent_b();
		const float phase = ImClamp(t, 0.f, 1.f) * 0.5f;

		constexpr float kSlice = 2.f;
		const int slices = (int)ImCeil(ts.x / kSlice);
		for (int i = 0; i < slices; ++i)
		{
			const float x0 = pos.x + (float)i * kSlice;

			const float k = ImClamp((float)i / (float)ImMax(1, slices - 1) + phase, 0.f, 1.f);
			const ImVec4 c(
				(a.x + (b.x - a.x) * k),
				(a.y + (b.y - a.y) * k),
				(a.z + (b.z - a.z) * k), alpha);
			dl->PushClipRect(ImVec2(x0, pos.y - size * 0.35f),
				ImVec2(x0 + kSlice, pos.y + size * 1.35f), true);
			dl->AddText(font, size, pos, rgba(c, 1.f), text);
			dl->PopClipRect();
		}
	}

	inline void odometer(ImDrawList* dl, ImGuiID id, ImFont* font, float size, ImVec2 pos,
		const char* text, ImU32 col)
	{
		if (!on() || !dl || !font || !text)
			return;
		if (!settings::ui21::odometer)
		{
			dl->AddText(font, size, pos, col, text);
			return;
		}

		const float dt = ImClamp(ImGui::GetIO().DeltaTime, 0.f, 1.f / 20.f);
		ImGuiStorage* store = ImGui::GetStateStorage();

		float x = pos.x;
		for (int i = 0; text[i]; ++i)
		{
			char glyph[2] = { text[i], 0 };

			const bool digit = (text[i] >= '0' && text[i] <= '9');
			const float gw = font->CalcTextSizeA(size, FLT_MAX, 0.f, glyph).x;

			if (!digit)
			{
				dl->AddText(font, size, ImVec2(x, pos.y), col, glyph);
				x += gw;
				continue;
			}

			const ImGuiID key = (ImGuiID)(id ^ (ImGuiID)(i * 2654435761u));
			const ImGuiID key_old = (ImGuiID)(key ^ 0x51edu);
			int current = (int)text[i] - '0';
			int previous = store->GetInt(key_old, current);
			if (previous != current)
			{
				store->SetInt(key_old, current);
				store->SetFloat(key, 1.f);
			}
			float roll = store->GetFloat(key, 0.f);

			if (roll <= 0.001f)
			{
				dl->AddText(font, size, ImVec2(x, pos.y), col, glyph);
				x += gw;
				continue;
			}

			roll = ImMax(0.f, roll - dt * 4.2f);
			store->SetFloat(key, roll);

			const float e = 1.f - (roll * roll * (3.f - 2.f * roll));
			dl->PushClipRect(ImVec2(x - 1.f, pos.y - 1.f), ImVec2(x + gw + 1.f, pos.y + size + 1.f), true);
			char old_glyph[2] = { (char)('0' + previous), 0 };
			const int old_alpha = (int)(255.f * (1.f - e) * ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.f);
			const int new_alpha = (int)(255.f * e * ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.f);
			dl->AddText(font, size, ImVec2(x, pos.y - e * size), (col & 0x00FFFFFFu) | ((ImU32)ImClamp(old_alpha, 0, 255) << 24), old_glyph);
			dl->AddText(font, size, ImVec2(x, pos.y + (1.f - e) * size), (col & 0x00FFFFFFu) | ((ImU32)ImClamp(new_alpha, 0, 255) << 24), glyph);
			dl->PopClipRect();
			x += gw;
		}
	}

	inline void pill_tabs(ImDrawList* dl, ImFont* font, ImVec2 mn, ImVec2 mx,
		const char* const* labels, int count, int active, float pill_x, float pill_w, float rounding)
	{
		if (!on() || !dl || !font || count <= 0 || mx.x < mn.x)
			return;

		dl->AddRectFilled(mn, mx, rgba(ImVec4(0.f, 0.f, 0.f, 1.f), 0.28f), rounding);

		const ImVec4 a = accent_a();
		const ImVec4 b = accent_b();
		const ImVec2 pa(pill_x + 2.f, mn.y + 2.f);
		const ImVec2 pb(pill_x + pill_w - 2.f, mx.y - 2.f);
		if (pb.x > pa.x && pb.y > pa.y)
		{
			const float h = pb.y - pa.y;
			const int slabs = (int)ImClamp(ImCeil(h * 0.7f), 2.f, 14.f);
			for (int i = 0; i < slabs; ++i)
			{
				const float y0 = pa.y + h * (float)i / (float)slabs;
				const float y1 = (i == slabs - 1) ? pb.y : pa.y + h * (float)(i + 1) / (float)slabs;
				const float k = (float)i / (float)(slabs - 1);
				dl->AddRectFilled(ImVec2(pa.x, y0), ImVec2(pb.x, y1),
					rgba(ImVec4(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
						a.z + (b.z - a.z) * k, 1.f), 0.92f), i == 0 || i == slabs - 1 ? rounding - 2.f : 0.f);
			}
		}

		const float lum = a.x * 0.299f + a.y * 0.587f + a.z * 0.114f;
		const ImU32 on_pill = rgba(lum > 0.55f ? ImVec4(0.06f, 0.06f, 0.07f, 1.f)
			: ImVec4(0.99f, 0.99f, 1.f, 1.f), 1.f);
		const ImU32 off_pill = rgba(ImVec4(0.78f, 0.80f, 0.86f, 1.f), 0.75f);

		const float seg = (mx.x - mn.x) / (float)count;
		for (int i = 0; i < count; ++i)
		{
			const char* lab = labels[i] ? labels[i] : "";
			if (!lab[0]) continue;
			const ImVec2 ts = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, lab);
			const float cx = mn.x + seg * ((float)i + 0.5f);
			dl->AddText(font, font->FontSize, ImVec2(cx - ts.x * 0.5f, (mn.y + mx.y - ts.y) * 0.5f),
				i == active ? on_pill : off_pill, lab);
		}
	}
}
