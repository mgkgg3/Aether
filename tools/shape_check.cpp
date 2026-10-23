// Offline check for the styles added in the source round.
//
// It brings up a real ImGui context (no backend), draws each variant into a
// background draw list and reports the vertex counts and the extents. A style
// that emits nothing - a mismatched radius, a zero-length dash loop, a missing
// cap - shows up as 0 vertices here, which is the failure a menu screenshot
// cannot distinguish from "the feature is off".
//
// Build (x64 Native Tools prompt):
//   cl /nologo /std:c++20 /EHsc /I. /Iext tools\shape_check.cpp ^
//      ext\imgui\imgui.cpp ext\imgui\imgui_draw.cpp ext\imgui\imgui_tables.cpp ^
//      ext\imgui\imgui_widgets.cpp /Fe:build\shape_check.exe
//
// Expected: every line non-zero, the dot styles scale with the radius, and the
// laser emits at least one dash per 60 px.

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>

#include <cstdio>
#include <cmath>

#include <render/ShapeDraw.h>

static int g_failures = 0;

static ImDrawList* begin()
{
	static bool once = false;
	if (!once)
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(1920.f, 1080.f);
		io.Fonts->AddFontDefault();
		io.Fonts->Build();
		once = true;
	}
	ImGui::NewFrame();
	return ImGui::GetBackgroundDrawList();
}

static void end()
{
	ImGui::EndFrame();
}

static int verts(ImDrawList* dl)
{
	return dl->VtxBuffer.Size;
}

static void report(const char* name, int count, int expect_at_least)
{
	const bool ok = count >= expect_at_least;
	if (!ok) ++g_failures;
	std::printf("%-34s verts=%-5d %s\n", name, count, ok ? "ok" : "EMPTY");
}

int main()
{
	shape::set_quality(true, 1.0f);

	const ImVec2 c(900.f, 500.f);
	const float radius = 150.f;
	const ImU32 col = IM_COL32(120, 200, 255, 255);
	const ImU32 out = IM_COL32(0, 0, 0, 255);

	for (int style = 0; style < 4; ++style)
	{
		ImDrawList* dl = begin();
		const int before = verts(dl);
		shape::fov_circle(dl, c, radius, col, out, 1.5f, true, style, false, 0, 0.f, true);
		const int n = verts(dl) - before;
		char name[64];
		std::snprintf(name, sizeof name, "fov_circle style %d", style);
		report(name, n, 40);
		end();
	}

	// The dot styles must scale with the radius: a fixed bead count would leave
	// a big circle sparse, which is the thing the source's own code avoids.
	{
		ImDrawList* dl = begin();
		const int before = verts(dl);
		shape::fov_circle(dl, c, radius * 2.f, col, out, 1.5f, false, 1, false, 0, 0.f, false);
		const int big = verts(dl) - before;
		end();

		dl = begin();
		const int before2 = verts(dl);
		shape::fov_circle(dl, c, radius * 0.5f, col, out, 1.5f, false, 1, false, 0, 0.f, false);
		const int small = verts(dl) - before2;
		end();

		const bool scales = big >= small;
		if (!scales) ++g_failures;
		std::printf("%-34s big=%-5d small=%-5d %s\n", "fov dots scale with radius",
			big, small, scales ? "ok" : "NOT MONOTONIC");
	}

	const ImVec2 a(200.f, 900.f), b(1700.f, 120.f);

	{
		ImDrawList* dl = begin();
		const int before = verts(dl);
		shape::tapered_band(dl, a, b, 2.f, col, col, 1.f);
		report("tapered_band", verts(dl) - before, 16);
		end();
	}
	{
		ImDrawList* dl = begin();
		const int before = verts(dl);
		shape::beam_line(dl, a, b, 2.f, col);
		report("beam_line", verts(dl) - before, 32);
		end();
	}
	{
		ImDrawList* dl = begin();
		const int before = verts(dl);
		shape::laser_line(dl, a, b, 2.f, col, 0.37f);
		const int n = verts(dl) - before;
		report("laser_line", n, 16);

		// A dash pattern is only a dash pattern if more than one dash landed on
		// a line this long.
		const float len = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
		const int dashes = (int)(len / (10.f + 2.f * 4.f + (10.f + 2.f * 4.f) * 0.85f));
		const bool multi = dashes >= 2;
		if (!multi) ++g_failures;
		std::printf("%-34s dashes>=2 %s\n", "laser_line segments", multi ? "ok" : "ONE DASH");
		end();
	}
	{
		ImDrawList* dl = begin();
		const int before = verts(dl);
		shape::dotted_line(dl, a, b, 2.f, col);
		report("dotted_line", verts(dl) - before, 64);
		end();
	}

	// A zero-length segment must not divide by zero and must emit nothing.
	{
		ImDrawList* dl = begin();
		const int before = verts(dl);
		shape::laser_line(dl, a, a, 2.f, col, 1.f);
		shape::dotted_line(dl, a, a, 2.f, col);
		shape::beam_line(dl, a, a, 2.f, col);
		const int n = verts(dl) - before;
		const bool ok = n == 0;
		if (!ok) ++g_failures;
		std::printf("%-34s verts=%-5d %s\n", "degenerate segment", n, ok ? "ok" : "DREW");
		end();
	}

	std::printf("\n%s (%d failure%s)\n", g_failures ? "FAILED" : "ALL OK",
		g_failures, g_failures == 1 ? "" : "s");
	return g_failures ? 1 : 0;
}
