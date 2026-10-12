#define NOMINMAX
#include "esp.h"

#include <vector>
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>
#include <game/game.h>
#include <cache/cache.h>
#include <memory/memory.h>
#include <game/game_context.h>
#include <sdk/offsets.h>
#include <debug/DebugConsole.h>
#include <features/aimbot/aimbot.h>
#include <features/silentaim/silentaim.h>
#include <features/worldgeo/WorldGeo.h>
#include <features/opone/OperationOne.h>
#include <features/mesh/MeshChams.h>
#include <features/mesh/MeshDxShader.h>
#include <features/chams/ShaderChams.h>
#include <features/effects/Effects.h>
#include <features/esp/Avatar.h>
#include <features/esp/BoxFill.h>
#include <features/players/friends.h>
#include <features/players/targets.h>
#include <features/misc/TextSheen.h>
#include <gui/AetherFonts.h>

namespace helper
{
	__forceinline bool finite3(const math::vector3& v)
	{
		return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
	}

	struct world_box_t
	{
		bool          valid{ false };
		math::vector3 min{};
		math::vector3 max{};
	};

	__forceinline void include_point(world_box_t& b, const math::vector3& p)
	{
		if (!b.valid)
		{
			b.min = b.max = p;
			b.valid = true;
			return;
		}
		if (p.x < b.min.x) b.min.x = p.x;
		if (p.y < b.min.y) b.min.y = p.y;
		if (p.z < b.min.z) b.min.z = p.z;
		if (p.x > b.max.x) b.max.x = p.x;
		if (p.y > b.max.y) b.max.y = p.y;
		if (p.z > b.max.z) b.max.z = p.z;
	}

	__forceinline void include_oriented(world_box_t& b, const math::vector3& pos,
		const math::matrix3& rot, const math::vector3& size)
	{
		const float hx = size.x * 0.5f;
		const float hy = size.y * 0.5f;
		const float hz = size.z * 0.5f;

		const float ex = std::fabs(rot.m[0]) * hx + std::fabs(rot.m[1]) * hy + std::fabs(rot.m[2]) * hz;
		const float ey = std::fabs(rot.m[3]) * hx + std::fabs(rot.m[4]) * hy + std::fabs(rot.m[5]) * hz;
		const float ez = std::fabs(rot.m[6]) * hx + std::fabs(rot.m[7]) * hy + std::fabs(rot.m[8]) * hz;

		include_point(b, { pos.x - ex, pos.y - ey, pos.z - ez });
		include_point(b, { pos.x + ex, pos.y + ey, pos.z + ez });
	}

	__forceinline bool looks_like_hull(const std::string& name, const rbx::part_t& part)
	{
		std::string lowered;
		lowered.reserve(name.size());
		for (char c : name) lowered.push_back((char)std::tolower((unsigned char)c));

		if (lowered.find("collision") != std::string::npos ||
			lowered.find("hitbox")    != std::string::npos ||
			lowered.find("hull")      != std::string::npos)
			return true;

		try
		{
			const std::string cls = part.get_class_name();
			return cls == "UnionOperation" || cls == "IntersectOperation" || cls == "NegateOperation";
		}
		catch (...) { return false; }
	}

	__forceinline ImU32 apply_opacity(ImU32 color, float opacity)
	{
		ImU32 r = (color >> 0) & 0xFF;
		ImU32 g = (color >> 8) & 0xFF;
		ImU32 b = (color >> 16) & 0xFF;
		ImU32 a = (color >> 24) & 0xFF;
		a = static_cast<ImU32>(a * opacity);
		return IM_COL32(r, g, b, a);
	}

	__forceinline ImU32 color_with_opacity(float r, float g, float b, float a, float opacity)
	{
		return IM_COL32(
			static_cast<int>(r * 255.f),
			static_cast<int>(g * 255.f),
			static_cast<int>(b * 255.f),
			static_cast<int>(a * opacity * 255.f)
		);
	}

	__forceinline void draw_text_outlined(ImDrawList* draw, ImFont* font, float font_size,
		ImVec2 pos, ImU32 col, const char* text_begin, const char* text_end = nullptr)
	{
		pos.x = roundf(pos.x);
		pos.y = roundf(pos.y);

		ImU32 outline_col = IM_COL32(0, 0, 0, 255);
		ImDrawListFlags old_flags = draw->Flags;
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		const char* end = text_end ? text_end : text_begin + std::strlen(text_begin);
		int len = static_cast<int>(end - text_begin);
		if (len == 0) return;

		float cx = pos.x;
		for (int i = 0; i < len; i++)
		{
			const char* cb = text_begin + i;
			const char* ce = text_begin + i + 1;
			for (int ox = -1; ox <= 1; ox++)
				for (int oy = -1; oy <= 1; oy++)
					if (ox != 0 || oy != 0)
						draw->AddText(font, font_size, ImVec2(cx + ox, pos.y + oy), outline_col, cb, ce);
			float cw = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, cb, ce).x;
			draw->AddText(font, font_size, ImVec2(cx, pos.y), col, cb, ce);
			cx += cw + 1.f;
		}
		draw->Flags = old_flags;
	}

	__forceinline void draw_text_blended(ImDrawList* draw, ImFont* font, float font_size,
		ImVec2 pos, ImU32 col_start, ImU32 col_end,
		const char* text_begin, const char* text_end = nullptr)
	{
		pos.x = roundf(pos.x);
		pos.y = roundf(pos.y);

		ImU32 outline_col = IM_COL32(0, 0, 0, 255);
		ImDrawListFlags old_flags = draw->Flags;
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		const char* end = text_end ? text_end : text_begin + std::strlen(text_begin);
		int len = static_cast<int>(end - text_begin);
		if (len == 0) return;

		float sr = (float)((col_start >>  0) & 0xFF);
		float sg = (float)((col_start >>  8) & 0xFF);
		float sb = (float)((col_start >> 16) & 0xFF);
		float sa = (float)((col_start >> 24) & 0xFF);
		float er = (float)((col_end   >>  0) & 0xFF);
		float eg = (float)((col_end   >>  8) & 0xFF);
		float eb = (float)((col_end   >> 16) & 0xFF);
		float ea = (float)((col_end   >> 24) & 0xFF);

		float cx = pos.x;
		for (int i = 0; i < len; i++)
		{
			float t = len > 1 ? (float)i / (float)(len - 1) : 0.f;
			ImU32 cc = IM_COL32(
				(int)(sr + (er - sr) * t), (int)(sg + (eg - sg) * t),
				(int)(sb + (eb - sb) * t), (int)(sa + (ea - sa) * t));
			const char* cb = text_begin + i;
			const char* ce = text_begin + i + 1;
			for (int ox = -1; ox <= 1; ox++)
				for (int oy = -1; oy <= 1; oy++)
					if (ox != 0 || oy != 0)
						draw->AddText(font, font_size, ImVec2(cx + ox, pos.y + oy), outline_col, cb, ce);
			float cw = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, cb, ce).x;
			draw->AddText(font, font_size, ImVec2(cx, pos.y), cc, cb, ce);
			cx += cw + 1.f;
		}
		draw->Flags = old_flags;
	}

	__forceinline void box(ImVec2& c1, ImVec2& c2, ImU32 color, float thickness = 1.f)
	{
		c1.x = roundf(c1.x); c1.y = roundf(c1.y);
		c2.x = roundf(c2.x); c2.y = roundf(c2.y);
		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		draw->Flags |= ImDrawListFlags_AntiAliasedLines;
		const float t = std::max(0.5f, thickness);
		ImRect rect(c1.x, c1.y, c1.x + c2.x, c1.y + c2.y);
		draw->AddRect(rect.Min, rect.Max, IM_COL32(0, 0, 0, color >> 24), 0.f, 0, t);
		draw->AddRect({rect.Min.x - 1.f, rect.Min.y - 1.f}, {rect.Max.x + 1.f, rect.Max.y + 1.f}, color, 0.f, 0, t);
		draw->AddRect({rect.Min.x - 2.f, rect.Min.y - 2.f}, {rect.Max.x + 2.f, rect.Max.y + 2.f}, IM_COL32(0, 0, 0, color >> 24), 0.f, 0, t);
	}

	__forceinline void corner_box(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 col, float thickness = 1.f)
	{
		float x1 = roundf(min.x) - 1, y1 = roundf(min.y) - 1;
		float x2 = roundf(max.x) + 1, y2 = roundf(max.y) + 1;
		ImU32 oc = IM_COL32(0, 0, 0, 255);
		float bw = x2 - x1, bh = y2 - y1;
		float len = std::min(std::min(bw * 0.3f, bh * 0.3f), 15.f);
		len = std::max(len, 5.f);
		float xl = x1 + len, yl = y1 + len, xr = x2 - len, yr = y2 - len;
		draw->AddRectFilled({x1-1,y1-1},{xl+1,y1+thickness+1},oc);
		draw->AddRectFilled({x1-1,y1-1},{x1+thickness+1,yl+1},oc);
		draw->AddRectFilled({xr-1,y1-1},{x2+1,y1+thickness+1},oc);
		draw->AddRectFilled({x2-thickness-1,y1-1},{x2+1,yl+1},oc);
		draw->AddRectFilled({x1-1,y2-thickness-1},{xl+1,y2+1},oc);
		draw->AddRectFilled({x1-1,yr-1},{x1+thickness+1,y2+1},oc);
		draw->AddRectFilled({xr-1,y2-thickness-1},{x2+1,y2+1},oc);
		draw->AddRectFilled({x2-thickness-1,yr-1},{x2+1,y2+1},oc);
		draw->AddRectFilled({x1,y1},{xl,y1+thickness},col);
		draw->AddRectFilled({x1,y1},{x1+thickness,yl},col);
		draw->AddRectFilled({xr,y1},{x2,y1+thickness},col);
		draw->AddRectFilled({x2-thickness,y1},{x2,yl},col);
		draw->AddRectFilled({x1,y2-thickness},{xl,y2},col);
		draw->AddRectFilled({x1,yr},{x1+thickness,y2},col);
		draw->AddRectFilled({xr,y2-thickness},{x2,y2},col);
		draw->AddRectFilled({x2-thickness,yr},{x2,y2},col);
	}

	inline float cross2d(const ImVec2& O, const ImVec2& A, const ImVec2& B)
	{
		return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
	}

	inline std::vector<ImVec2> convex_hull(const std::vector<ImVec2>& pts)
	{
		if (pts.size() <= 3) return pts;
		std::vector<ImVec2> p = pts;
		auto it = std::min_element(p.begin(), p.end(), [](const ImVec2& a, const ImVec2& b){
			return a.y < b.y || (a.y == b.y && a.x < b.x); });
		std::swap(p[0], *it);
		ImVec2 p0 = p[0];
		std::sort(p.begin() + 1, p.end(), [&p0](const ImVec2& a, const ImVec2& b){
			float c = cross2d(p0, a, b);
			if (c != 0) return c > 0;
			float da = (a.x-p0.x)*(a.x-p0.x)+(a.y-p0.y)*(a.y-p0.y);
			float db = (b.x-p0.x)*(b.x-p0.x)+(b.y-p0.y)*(b.y-p0.y);
			return da < db;
		});
		std::vector<ImVec2> h;
		for (auto& v : p) {
			while (h.size() > 1 && cross2d(h[h.size()-2], h.back(), v) <= 0)
				h.pop_back();
			h.push_back(v);
		}
		return h;
	}
}

struct esp_fade_t
{
	float box          = 0.f;
	float box_fill     = 0.f;
	float name         = 0.f;
	float healthbar    = 0.f;
	float health_pct   = 0.f;
	float distance     = 0.f;
	float tool         = 0.f;
	float flags        = 0.f;
	float chams        = 0.f;
	float arrows       = 0.f;
	float tracer       = 0.f;
	float china_hat    = 0.f;
	float health_text  = 0.f;
	float head_dot     = 0.f;
	float look_dir     = 0.f;
	float rig_state    = 0.f;
	float state_tag    = 0.f;
};
static esp_fade_t g_fade;

enum arrow_tint_t : int { arrow_base = 0, arrow_aim, arrow_friend };

static const float* highlight_colour(std::uint64_t instance, bool aim_target)
{
	if (aim_target)
		return settings::visuals::aim_target_color;
	if (friends::is_friend(instance))
		return settings::visuals::friend_color;
	return nullptr;
}

struct offscreen_arrow_t { float dx; float dy; arrow_tint_t tint; std::string info; };
static std::vector<offscreen_arrow_t> g_offscreen;

static float fade_tick(float cur, bool enabled, float in_spd, float out_spd)
{
	float target = enabled ? 1.f : 0.f;
	float spd    = enabled ? in_spd : out_spd;
	float dt     = ImGui::GetIO().DeltaTime;
	cur += (target - cur > 0 ? 1 : -1) * spd * dt;
	return std::min(std::max(cur, 0.f), 1.f);
}

static std::uint64_t geometry_now_ms()
{
	return (std::uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct health_smooth_t { float hp = 0.f; bool seen = false; };
static std::unordered_map<std::uint64_t, health_smooth_t> g_health_smooth;

static bool is_valid_r15_part(const std::string& n)
{
	return n=="Head"||n=="UpperTorso"||n=="LowerTorso"||
	       n=="LeftUpperArm"||n=="LeftLowerArm"||n=="LeftHand"||
	       n=="RightUpperArm"||n=="RightLowerArm"||n=="RightHand"||
	       n=="LeftUpperLeg"||n=="LeftLowerLeg"||n=="LeftFoot"||
	       n=="RightUpperLeg"||n=="RightLowerLeg"||n=="RightFoot";
}
static bool is_r6_part(const std::string& n)
{
	return n=="Head"||n=="Torso"||n=="Left Arm"||n=="Right Arm"||n=="Left Leg"||n=="Right Leg";
}

static bool is_player_knocked(const cache::entity_t& player)
{

	return game_ctx::get_knock(player.instance.address) == game_ctx::KnockState::Yes;
}

static bool entity_is_dead(const cache::entity_t& entity)
{
	if (op1::active() && settings::op1::live_health)
		return entity.state == 15 || op1::corpse(entity);

	return op1::corpse(entity) ||
		(cache::is_dead(entity) && (entity.state == 15 || settings::visuals::dead_check));
}

static const math::vector3 k_corners[8] = {
	{-1,-1,-1},{1,-1,-1},{-1,1,-1},{1,1,-1},
	{-1,-1, 1},{1,-1, 1},{-1,1, 1},{1,1, 1}
};

static void draw_hitbox_3d(const cache::entity_t& entity,
	const math::matrix4& view, const math::vector2& dims,
	ImDrawList* draw, std::uint64_t local_addr)
{
	if (!settings::visuals::view_hitbox) return;
	if (entity.instance.address == local_addr) return;

	auto it = entity.parts.find("HumanoidRootPart");
	if (it == entity.parts.end() || !it->second.address) return;
	rbx::primitive_t prim = it->second.get_primitive();
	if (!prim.address) return;
	auto pos  = prim.get_position();
	auto sz   = prim.get_size();
	auto rot  = prim.get_rotation();
	if (sz.x == 0 || sz.y == 0 || sz.z == 0) return;

	math::vector3 xsz{
		settings::hitbox_expander::size_x,
		settings::hitbox_expander::size_y,
		settings::hitbox_expander::size_z
	};

	ImVec2 pc[8]; bool valid[8]{};
	for (int i = 0; i < 8; i++)
	{
		math::vector3 w = pos + rot * math::vector3{
			k_corners[i].x * xsz.x * .5f,
			k_corners[i].y * xsz.y * .5f,
			k_corners[i].z * xsz.z * .5f};
		math::vector2 s{};
		valid[i] = game::visengine.world_to_screen(w, s, dims, view);
		pc[i] = {s.x, s.y};
	}

	ImU32 col = IM_COL32(
		(int)(settings::visuals::view_hitbox_color[0]*255.f),
		(int)(settings::visuals::view_hitbox_color[1]*255.f),
		(int)(settings::visuals::view_hitbox_color[2]*255.f),
		(int)(settings::visuals::view_hitbox_color[3]*255.f));

	ImDrawListFlags old = draw->Flags;
	draw->Flags |= ImDrawListFlags_AntiAliasedLines;

	static const int edges[12][2] = {
		{0,1},{1,3},{3,2},{2,0},
		{4,5},{5,7},{7,6},{6,4},
		{0,4},{1,5},{2,6},{3,7}
	};
	for (auto& e : edges)
		if (valid[e[0]] && valid[e[1]])
			draw->AddLine(pc[e[0]], pc[e[1]], col, 1.f);

	draw->Flags = old;
}

struct geom_view_t
{
	bool          ok{ false };
	math::vector3 pos{}, size{};
	math::matrix3 rot = math::matrix3::identity();
	float         half_h{ 0.f };
	float         volume{ 0.f };
	bool          hull{ false };
};

static bool get_cached_geometry(const std::string& name, const rbx::part_t& part, geom_view_t& out);

static bool plausible_point(const math::vector3& v);
static bool plausible_size(const math::vector3& v);
static bool plausible_rotation(const math::matrix3& r);

static const math::vector3 k_cham_corners[8] = {
	{ -1, -1, -1 }, { -1, -1,  1 }, { -1,  1, -1 }, { -1,  1,  1 },
	{  1, -1, -1 }, {  1, -1,  1 }, {  1,  1, -1 }, {  1,  1,  1 },
};

static const int k_cham_edges[12][2] = {
	{0,1},{0,2},{0,4},{1,3},{1,5},{2,3},
	{2,6},{3,7},{4,5},{4,6},{5,7},{6,7},
};

static std::vector<ImVec2> cham_convex_hull(std::vector<ImVec2> pts)
{
	if (pts.size() < 3) return pts;
	std::sort(pts.begin(), pts.end(), [](const ImVec2& a, const ImVec2& b) {
		return a.x < b.x || (a.x == b.x && a.y < b.y);
	});
	auto cross = [](const ImVec2& o, const ImVec2& a, const ImVec2& b) {
		return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
	};
	std::vector<ImVec2> hull(pts.size() * 2);
	int k = 0;
	for (std::size_t i = 0; i < pts.size(); ++i) {
		while (k >= 2 && cross(hull[k - 2], hull[k - 1], pts[i]) <= 0.0f) k--;
		hull[k++] = pts[i];
	}
	for (int i = (int)pts.size() - 2, t = k + 1; i >= 0; --i) {
		while (k >= t && cross(hull[k - 2], hull[k - 1], pts[i]) <= 0.0f) k--;
		hull[k++] = pts[i];
	}
	hull.resize(k > 0 ? (std::size_t)(k - 1) : 0);
	return hull;
}

static bool cham_seg_inside(const ImVec2& a, const ImVec2& b,
	const std::vector<ImVec2>& poly, float& out_t0, float& out_t1)
{
	const int n = (int)poly.size();
	if (n < 3) return false;

	ImVec2 c(0.f, 0.f);
	for (const ImVec2& p : poly) { c.x += p.x; c.y += p.y; }
	c.x /= (float)n;
	c.y /= (float)n;

	float t0 = 0.f, t1 = 1.f;
	const float eps = 0.25f;

	for (int i = 0; i < n; ++i)
	{
		const ImVec2& p = poly[i];
		const ImVec2& q = poly[(i + 1) % n];
		const float ex = q.x - p.x, ey = q.y - p.y;

		auto side = [&](const ImVec2& v) { return ex * (v.y - p.y) - ey * (v.x - p.x); };

		const float s = side(c) >= 0.f ? 1.f : -1.f;
		const float f0 = s * side(a), f1 = s * side(b);
		const float df = f1 - f0;

		if (std::fabs(df) < 1e-6f)
		{
			if (f0 < -eps) return false;
			continue;
		}

		const float tc = (-eps - f0) / df;
		if (df > 0.f) { if (tc > t0) t0 = tc; }
		else          { if (tc < t1) t1 = tc; }

		if (t0 >= t1) return false;
	}

	if (t0 < 0.f) t0 = 0.f;
	if (t1 > 1.f) t1 = 1.f;
	out_t0 = t0;
	out_t1 = t1;
	return out_t1 > out_t0;
}

static std::vector<ImVec2> cham_clip_half(const std::vector<ImVec2>& poly,
	const ImVec2& p, const ImVec2& q, float s)
{
	std::vector<ImVec2> out;
	const int n = (int)poly.size();
	if (n < 3) return out;
	out.reserve((std::size_t)n + 2);

	auto f = [&](const ImVec2& v) {
		return s * ((q.x - p.x) * (v.y - p.y) - (q.y - p.y) * (v.x - p.x));
	};

	for (int i = 0; i < n; ++i)
	{
		const ImVec2& a = poly[i];
		const ImVec2& b = poly[(i + 1) % n];
		const float fa = f(a), fb = f(b);

		if (fa >= 0.f) out.push_back(a);
		if ((fa < 0.f) != (fb < 0.f))
		{
			const float t = fa / (fa - fb);
			out.push_back(ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t));
		}
	}

	if (out.size() < 3) out.clear();
	return out;
}

static void cham_subtract(std::vector<ImVec2> piece, const std::vector<ImVec2>& cut,
	std::vector<std::vector<ImVec2>>& out)
{
	const int n = (int)cut.size();
	if (n < 3)
	{
		if (piece.size() >= 3) out.push_back(std::move(piece));
		return;
	}

	ImVec2 c(0.f, 0.f);
	for (const ImVec2& v : cut) { c.x += v.x; c.y += v.y; }
	c.x /= (float)n;
	c.y /= (float)n;

	for (int i = 0; i < n && piece.size() >= 3; ++i)
	{
		const ImVec2& p = cut[i];
		const ImVec2& q = cut[(i + 1) % n];
		const float cs = (q.x - p.x) * (c.y - p.y) - (q.y - p.y) * (c.x - p.x);
		const float s = cs >= 0.f ? 1.f : -1.f;

		std::vector<ImVec2> outside = cham_clip_half(piece, p, q, -s);
		if (!outside.empty()) out.push_back(std::move(outside));
		piece = cham_clip_half(piece, p, q, s);
	}
}

static void cham_outline_outside(ImDrawList* dl, const ImVec2& a, const ImVec2& b,
	const std::vector<std::vector<ImVec2>>& polys, int skip, ImU32 color)
{
	std::vector<std::pair<float, float>> covered;
	for (int i = 0; i < (int)polys.size(); ++i)
	{
		if (i == skip) continue;
		float t0 = 0.f, t1 = 0.f;
		if (cham_seg_inside(a, b, polys[i], t0, t1))
			covered.emplace_back(t0, t1);
	}

	auto lerp_pt = [&](float t) {
		return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
	};

	if (covered.empty())
	{
		dl->AddLine(a, b, color, 1.f);
		return;
	}

	std::sort(covered.begin(), covered.end());
	std::vector<std::pair<float, float>> merged;
	for (auto& seg : covered)
	{
		if (merged.empty() || seg.first > merged.back().second)
			merged.push_back(seg);
		else
			merged.back().second = (std::max)(merged.back().second, seg.second);
	}

	float cur = 0.f;
	for (auto& m : merged)
	{
		if (m.first > cur) dl->AddLine(lerp_pt(cur), lerp_pt(m.first), color, 1.f);
		cur = (std::max)(cur, m.second);
	}
	if (cur < 1.f)
		dl->AddLine(lerp_pt(cur), b, color, 1.f);
}

static bool part_box_live(const rbx::part_t& part, math::vector3& pos,
	math::vector3& size, math::matrix3& rot)
{
	if (!part.address) return false;

	try
	{
		rbx::primitive_t prim = part.get_primitive();
		if (!prim.address) return false;

		const math::vector3 p = prim.get_position();
		const math::vector3 s = prim.get_size();
		if (!plausible_point(p) || !plausible_size(s)) return false;

		math::matrix3 r = prim.get_rotation();
		if (!plausible_rotation(r)) r = math::matrix3::identity();

		pos  = p;
		size = s;
		rot  = r;
		return true;
	}
	catch (...) { return false; }
}

static void draw_part_chams(const cache::entity_t& entity,
	const math::matrix4& view, const math::vector2& dims,
	ImDrawList* draw, const float* highlight, float hit_fade, bool corpse_mode)
{
	if (!draw) return;

	int mode = settings::visuals::nchams_mode;
	if (mode < 0) mode = 0;
	if (mode > 3) mode = 3;

	const bool want_hit = !corpse_mode && settings::visuals::hit_chams &&
		hit_fade > 0.001f && (mode == 2 || mode == 3);
	if (!settings::visuals::nchams && !want_hit) return;

	struct part_corners_t { ImVec2 pt[8]; bool full; };
	std::vector<part_corners_t> parts;
	parts.reserve(entity.parts.size());

	for (auto& kv : entity.parts)
	{
		if (kv.first == "HumanoidRootPart") continue;

		if (!is_valid_r15_part(kv.first) && !is_r6_part(kv.first)) continue;

		math::vector3 pos{}, size{};
		math::matrix3 rot = math::matrix3::identity();
		if (!part_box_live(kv.second, pos, size, rot)) continue;

		part_corners_t pc{};
		pc.full = true;
		const math::vector3 half{ size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
		for (int i = 0; i < 8; ++i)
		{
			const math::vector3 local{
				k_cham_corners[i].x * half.x,
				k_cham_corners[i].y * half.y,
				k_cham_corners[i].z * half.z };
			const math::vector3 world = pos + rot * local;

			math::vector2 s{};

			if (!game::visengine.world_to_screen(world, s, dims, view)) { pc.full = false; break; }
			pc.pt[i] = ImVec2(s.x, s.y);
		}

		if (pc.full) parts.push_back(pc);
	}

	if (parts.empty()) return;

	auto col_of = [](const float* c4, float a_mul) -> ImU32 {
		return IM_COL32((int)(c4[0] * 255.f), (int)(c4[1] * 255.f), (int)(c4[2] * 255.f),
			(int)(std::min(1.f, std::max(0.f, c4[3] * a_mul)) * 255.f));
	};

	const float* corpse_col = settings::visuals::corpse_color;
	const float* oc_ptr = corpse_mode ? corpse_col : settings::visuals::nchams_outline_color;
	const float* fc_ptr = corpse_mode ? corpse_col : settings::visuals::nchams_fill_color;

	ImU32 outline_col = col_of(oc_ptr, 1.f);
	ImU32 fill_col    = col_of(fc_ptr, corpse_mode ? 0.55f : 1.f);

	const bool override_hue = !corpse_mode && highlight != nullptr;
	if (override_hue)
	{
		outline_col = IM_COL32((int)(highlight[0] * 255.f), (int)(highlight[1] * 255.f),
			(int)(highlight[2] * 255.f), (int)(highlight[3] * 255.f));
		fill_col = IM_COL32((int)(highlight[0] * 255.f), (int)(highlight[1] * 255.f),
			(int)(highlight[2] * 255.f), 110);
	}

	auto glow_core = [&]() {
		if (settings::visuals::nchams_glow)
		{
			const int ga = (int)(std::clamp(settings::visuals::nchams_glow_opacity / 100.f, 0.f, 1.f) * 110.f);
			for (const auto& pc : parts)
			{

				ImRect r(pc.pt[0], pc.pt[0]);
				for (int i = 1; i < 8; ++i)
				{
					r.Min.x = std::min(r.Min.x, pc.pt[i].x);
					r.Min.y = std::min(r.Min.y, pc.pt[i].y);
					r.Max.x = std::max(r.Max.x, pc.pt[i].x);
					r.Max.y = std::max(r.Max.y, pc.pt[i].y);
				}
				const float pad = std::max(1.5f, settings::visuals::nchams_glow_size);
				draw->AddRect({r.Min.x - pad, r.Min.y - pad}, {r.Max.x + pad, r.Max.y + pad},
					IM_COL32(255, 255, 255, ga), 0.f, 0, pad * 2.f);
			}
		}

		if (settings::visuals::nchams_inner_fill && !corpse_mode)
		{

			const float k = std::clamp(settings::visuals::nchams_inner_fill_scale / 100.f, 0.05f, 1.f);
			const ImU32 core = col_of(fc_ptr, 1.f);
			for (const auto& pc : parts)
			{
				std::vector<ImVec2> pts(pc.pt, pc.pt + 8);
				std::vector<ImVec2> hull = cham_convex_hull(std::move(pts));
				if (hull.size() < 3) continue;

				ImVec2 c(0.f, 0.f);
				for (const ImVec2& v : hull) { c.x += v.x; c.y += v.y; }
				c.x /= (float)hull.size();
				c.y /= (float)hull.size();
				for (ImVec2& v : hull)
					v = ImVec2(c.x + (v.x - c.x) * k, c.y + (v.y - c.y) * k);

				draw->AddConvexPolyFilled(hull.data(), (int)hull.size(), core);
			}
		}
	};

	const ImDrawListFlags old_flags = draw->Flags;
	draw->Flags |= ImDrawListFlags_AntiAliasedLines;

	if (mode == 0)
	{
		glow_core();
		for (const auto& pc : parts)
			for (const auto& e : k_cham_edges)
				draw->AddLine(pc.pt[e[0]], pc.pt[e[1]], outline_col, 1.f);

		draw->Flags = old_flags;
		return;
	}

	if (mode == 1)
	{
		glow_core();
		for (const auto& pc : parts)
		{
			std::vector<ImVec2> pts(pc.pt, pc.pt + 8);
			std::vector<ImVec2> hull = cham_convex_hull(std::move(pts));
			if (hull.size() >= 3)
				draw->AddConvexPolyFilled(hull.data(), (int)hull.size(), fill_col);
		}
		for (const auto& pc : parts)
			for (const auto& e : k_cham_edges)
				draw->AddLine(pc.pt[e[0]], pc.pt[e[1]], outline_col, 1.f);

		draw->Flags = old_flags;
		return;
	}

	std::vector<std::vector<ImVec2>> hulls;
	hulls.reserve(parts.size());
	for (const auto& pc : parts)
	{
		std::vector<ImVec2> pts(pc.pt, pc.pt + 8);
		std::vector<ImVec2> h = cham_convex_hull(std::move(pts));
		if (h.size() >= 3) hulls.push_back(std::move(h));
	}

	std::vector<std::vector<ImVec2>> clipped;
	clipped.reserve(hulls.size() * 2);
	for (int i = 0; i < (int)hulls.size(); ++i)
	{
		std::vector<std::vector<ImVec2>> pieces{ hulls[i] };
		for (int j = 0; j < i && !pieces.empty(); ++j)
		{
			std::vector<std::vector<ImVec2>> next;
			for (auto& piece : pieces)
				cham_subtract(std::move(piece), hulls[j], next);
			pieces = std::move(next);
		}
		for (auto& piece : pieces)
			if (piece.size() >= 3)
				clipped.push_back(std::move(piece));
	}

	if (clipped.empty())
	{
		draw->Flags = old_flags;
		return;
	}

	if (mode == 3)
	{
		glow_core();

		namespace schams = Cheat::Visuals::ShaderChams;

		int style = settings::visuals::nchams_shader_style;
		if (style < 0 || style >= schams::StyleNameCount()) style = 0;

		const float* override_col = corpse_mode ? corpse_col : highlight;

		schams::DrawFill(draw, clipped, (float)ImGui::GetTime(), style,
			false, false, override_col);

		if (want_hit)
		{
			const float* hc = settings::visuals::hit_chams_color;
			const float hit_ov[4] = { hc[0], hc[1], hc[2], hc[3] * hit_fade };
			schams::DrawFill(draw, clipped, (float)ImGui::GetTime(), style,
				false, false, hit_ov);
		}

		ImU32 sh_outline;
		if (corpse_mode)  sh_outline = schams::OutlineColor(style, false, corpse_col);
		else if (override_hue) sh_outline = outline_col;
		else              sh_outline = schams::OutlineColor(style, false);

		for (int i = 0; i < (int)hulls.size(); ++i)
		{
			const auto& hull = hulls[i];
			const int n = (int)hull.size();
			if (n < 2) continue;
			for (int e = 0; e < n; ++e)
				cham_outline_outside(draw, hull[e], hull[(e + 1) % n], hulls, i, sh_outline);
		}

		draw->Flags = old_flags;
		return;
	}

	draw->Flags &= ~ImDrawListFlags_AntiAliasedFill;
	glow_core();
	for (const auto& piece : clipped)
		draw->AddConvexPolyFilled(piece.data(), (int)piece.size(), fill_col);

	if (want_hit)
	{
		const float* hc = settings::visuals::hit_chams_color;
		const ImU32 hit_col = IM_COL32((int)(hc[0] * 255.f), (int)(hc[1] * 255.f),
			(int)(hc[2] * 255.f), (int)(std::min(1.f, hc[3] * hit_fade) * 255.f));
		for (const auto& piece : clipped)
			draw->AddConvexPolyFilled(piece.data(), (int)piece.size(), hit_col);
	}

	draw->Flags = old_flags | ImDrawListFlags_AntiAliasedLines;

	for (int i = 0; i < (int)hulls.size(); ++i)
	{
		const auto& hull = hulls[i];
		const int n = (int)hull.size();
		if (n < 2) continue;
		for (int e = 0; e < n; ++e)
			cham_outline_outside(draw, hull[e], hull[(e + 1) % n], hulls, i, outline_col);
	}

	draw->Flags = old_flags;
}

struct HitState { float last_hp; float timer; };
static std::unordered_map<std::uint64_t, HitState> g_hit_states;

struct FrozenChams {
	std::vector<std::pair<std::string, std::vector<ImVec2>>> part_hulls;
	float timer = 0.f;
};
static std::unordered_map<std::uint64_t, FrozenChams> g_frozen;

struct BonePair { const char* a; const char* b; };
static const BonePair k_r15[] = {
	{"Head","UpperTorso"},{"UpperTorso","LowerTorso"},
	{"UpperTorso","LeftUpperArm"},{"LeftUpperArm","LeftLowerArm"},{"LeftLowerArm","LeftHand"},
	{"UpperTorso","RightUpperArm"},{"RightUpperArm","RightLowerArm"},{"RightLowerArm","RightHand"},
	{"LowerTorso","LeftUpperLeg"},{"LeftUpperLeg","LeftLowerLeg"},{"LeftLowerLeg","LeftFoot"},
	{"LowerTorso","RightUpperLeg"},{"RightUpperLeg","RightLowerLeg"},{"RightLowerLeg","RightFoot"},
};
static const BonePair k_r6[] = {
	{"Head","Torso"},{"Torso","Left Arm"},{"Torso","Right Arm"},
	{"Torso","Left Leg"},{"Torso","Right Leg"},
};

struct rig_frame_t
{
	bool          valid{ false };
	math::vector3 centre{};
	math::vector3 forward{ 0.f, 0.f, -1.f };
	math::vector3 right{ 1.f, 0.f, 0.f };
	float         top{ 0.f };
	float         bottom{ 0.f };
	float         height{ 5.f };
	float         half_width{ 0.8f };
	std::string   root_name;
};

static const rbx::part_t* find_body_part(const cache::entity_t& entity, std::string& name_out);
static bool rig_frame(const cache::entity_t& entity, rig_frame_t& out);

enum skel_slot_t : int
{
	SK_HEAD, SK_CHEST, SK_HIP,
	SK_SH_L, SK_SH_R, SK_HIP_L, SK_HIP_R,
	SK_ELB_L, SK_ELB_R, SK_HAND_L, SK_HAND_R,
	SK_KNEE_L, SK_KNEE_R, SK_FOOT_L, SK_FOOT_R,
	SK_SLOTS
};

static const char* const k_skel_slot_names[SK_SLOTS] = {
	"$head", "$chest", "$hip",
	"$shL", "$shR", "$hipL", "$hipR",
	"$elbL", "$elbR", "$handL", "$handR",
	"$kneeL", "$kneeR", "$footL", "$footR",
};

static int skel_slot_for(const char* key)
{
	for (int i = 0; i < (int)SK_SLOTS; ++i)
		if (std::strcmp(k_skel_slot_names[i], key) == 0)
			return i;
	return -1;
}

struct skel_ease_t
{
	ImVec2        pt[SK_SLOTS]{};
	std::uint64_t ok_ms[SK_SLOTS]{};
	std::uint64_t last_ms{};
};
static std::unordered_map<std::uint64_t, skel_ease_t> g_skel_ease;

static constexpr std::uint64_t k_skel_hold_ms = 250;
static constexpr float         k_skel_ease_rate = 35.f;
static constexpr float         k_skel_snap_px = 140.f;

static int build_generic_skeleton(const cache::entity_t& entity,
	std::unordered_map<std::string, ImVec2>& bmap,
	std::vector<BonePair>& out_pairs,
	const math::vector2& dims, const math::matrix4& view)
{

	rig_frame_t rig{};
	if (!rig_frame(entity, rig))
		return 0;

	const math::vector3 pos    = rig.centre;
	const math::vector3 up     = math::vector3{ 0.f, 1.f, 0.f };
	const math::vector3 right  = rig.right;
	const float         half   = rig.height * 0.5f;
	const float         halfw  = rig.half_width;

	math::matrix3 root_rot{};
	root_rot.m[0] = -rig.forward.z; root_rot.m[1] = 0.f;            root_rot.m[2] = -rig.forward.x;
	root_rot.m[3] =  0.f;           root_rot.m[4] = 1.f;            root_rot.m[5] =  0.f;
	root_rot.m[6] =  rig.forward.x; root_rot.m[7] = 0.f;            root_rot.m[8] = -rig.forward.z;

	skel_ease_t& ease = g_skel_ease[entity.instance.address];
	const std::uint64_t ease_now = geometry_now_ms();
	float ease_dt = 0.f;
	if (ease.last_ms && ease_now > ease.last_ms)
		ease_dt = std::min((float)(ease_now - ease.last_ms) / 1000.f, 0.1f);
	ease.last_ms = ease_now;

	auto put = [&](const char* key, const math::vector3& w, ImVec2& out) -> bool
	{
		const int slot = skel_slot_for(key);
		if (slot < 0)
			return false;

		math::vector2 s{};
		const bool projected =
			game::visengine.world_to_screen(w, s, dims, view) &&
			std::isfinite(s.x) && std::isfinite(s.y);

		if (!projected)
		{
			if (ease.ok_ms[slot] && (ease_now - ease.ok_ms[slot]) <= k_skel_hold_ms)
			{
				out = ease.pt[slot];
				return true;
			}
			return false;
		}

		const ImVec2 fresh{ s.x, s.y };
		if (!ease.ok_ms[slot])
		{
			ease.pt[slot] = fresh;
		}
		else
		{
			const float jump = std::max(std::fabs(fresh.x - ease.pt[slot].x),
			                            std::fabs(fresh.y - ease.pt[slot].y));

			const float a = op1::no_smoothing_enabled() ? 1.f
				: (jump > k_skel_snap_px) ? 1.f : 1.f - expf(-ease_dt * k_skel_ease_rate);
			ease.pt[slot].x += (fresh.x - ease.pt[slot].x) * a;
			ease.pt[slot].y += (fresh.y - ease.pt[slot].y) * a;
		}
		ease.ok_ms[slot] = ease_now;
		out = ease.pt[slot];
		return true;
	};

	struct skel_anim_t
	{
		std::uint64_t ms{ 0 };
		math::vector3 pos{};
		float         phase{ 0.f };
		bool          seen{ false };
	};
	static std::unordered_map<std::uint64_t, skel_anim_t> s_anim;

	skel_anim_t& anim = s_anim[entity.instance.address];
	const std::uint64_t now = geometry_now_ms();
	float dt = (anim.seen && now > anim.ms) ? (float)(now - anim.ms) / 1000.f : 0.f;
	dt = std::clamp(dt, 0.f, 0.1f);

	float speed = 0.f, vvel = 0.f;
	if (anim.seen && dt > 0.f)
	{
		const math::vector3 d = pos - anim.pos;
		if (helper::finite3(d))
		{
			speed = sqrtf(d.x * d.x + d.z * d.z) / dt;
			vvel  = d.y / dt;
		}
	}
	anim.seen = true;
	anim.pos  = pos;
	anim.ms   = now;

	const bool airborne = (entity.state == 3 || entity.state == 5) || vvel > 3.5f || vvel < -4.5f;

	const float swing_cfg = std::clamp(settings::visuals::skeleton_swing, 0.f, 2.f);
	const float gait      = settings::visuals::skeleton_animate
		? std::min(1.f, speed / 18.f) * swing_cfg
		: 0.f;

	if (gait > 0.01f)
		anim.phase += std::min(speed, 60.f) * 0.12f * 3.14159265f * dt;
	if (anim.phase > 6.28318531f * 1000.f)
		anim.phase -= 6.28318531f * 1000.f;

	const float sw = sinf(anim.phase) * gait;

	const float idle   = 1.f - std::min(1.f, gait);
	const float idle_t = (float)(now % 5000ull) / 5000.f * 6.28318531f;
	const float breath = idle * sinf(idle_t) * 0.022f * half;
	const float idle_arm_sway = idle * sinf(idle_t * 0.5f) * 0.05f * half;

	const float bob = fabsf(sinf(anim.phase)) * gait * 0.10f * half + breath;

	math::vector3 fwd = root_rot.forward();
	if (!helper::finite3(fwd) || fwd.length() < 0.5f)
		fwd = math::vector3{ 0.f, 0.f, 1.f };
	fwd = fwd.normalized();

	const float stride = half * 0.55f * (0.25f + 0.75f * std::min(1.f, speed / 18.f)) * swing_cfg;

	ImVec2 p{};

	if (put("$head",  pos + up * (half * 0.84f + bob), p)) bmap["$head"]  = p;
	if (put("$chest", pos + up * (half * 0.40f + bob), p)) bmap["$chest"] = p;
	if (put("$hip",   pos - up * (half * 0.06f + bob), p)) bmap["$hip"]   = p;

	static const BonePair k_spine[] = {
		{ "$head",  "$chest" },
		{ "$chest", "$hip"   },
	};
	for (auto& bone : k_spine)
		out_pairs.push_back(bone);

	const float shoulder = halfw;
	const float hip_span = halfw * 0.45f;

	const math::vector3 chest_w = pos + up * (half * 0.40f + bob);
	const math::vector3 hip_w   = pos - up * (half * 0.06f + bob);

	auto add_point = [&](const char* key, const math::vector3& world) -> bool
	{
		ImVec2 pt{};
		if (!put(key, world, pt)) return false;
		bmap[key] = pt;
		return true;
	};

	const bool have_shoulders = add_point("$shL", chest_w + right * shoulder)
		&& add_point("$shR", chest_w - right * shoulder);
	const bool have_hips = add_point("$hipL", hip_w + right * hip_span)
		&& add_point("$hipR", hip_w - right * hip_span);

	if (have_shoulders)
	{
		out_pairs.push_back({ "$chest", "$shL" });
		out_pairs.push_back({ "$chest", "$shR" });
	}
	if (have_hips)
	{
		out_pairs.push_back({ "$hip", "$hipL" });
		out_pairs.push_back({ "$hip", "$hipR" });
	}
	if (have_shoulders && have_hips)
	{
		out_pairs.push_back({ "$shL", "$hipL" });
		out_pairs.push_back({ "$shR", "$hipR" });
	}

	const float arm_drop = airborne ? half * 0.20f : half * 0.52f;
	const float arm_out  = halfw * 1.05f;
	const float arm_swing = (airborne ? stride * 0.25f : -sw * stride * 0.8f) + idle_arm_sway;

	const math::vector3 handL_w = chest_w + right * arm_out - up * arm_drop + fwd * arm_swing;
	const math::vector3 handR_w = chest_w - right * arm_out - up * arm_drop - fwd * arm_swing;

	const math::vector3 shL_w = chest_w + right * shoulder;
	const math::vector3 shR_w = chest_w - right * shoulder;

	const math::vector3 elbL_w = (shL_w + handL_w) * 0.5f + right * (halfw * 0.25f) - fwd * (halfw * 0.15f);
	const math::vector3 elbR_w = (shR_w + handR_w) * 0.5f - right * (halfw * 0.25f) - fwd * (halfw * 0.15f);

	const bool have_arms = add_point("$elbL", elbL_w) && add_point("$elbR", elbR_w)
		&& add_point("$handL", handL_w) && add_point("$handR", handR_w);

	if (have_arms)
	{
		if (have_shoulders)
		{
			out_pairs.push_back({ "$shL", "$elbL" });
			out_pairs.push_back({ "$shR", "$elbR" });
		}
		out_pairs.push_back({ "$elbL", "$handL" });
		out_pairs.push_back({ "$elbR", "$handR" });
	}

	const float foot_drop = std::max(half * 0.5f, pos.y - rig.bottom);

	const float foot_span = have_hips ? hip_span : halfw * 0.55f;
	const math::vector3 foot_w = pos - up * (foot_drop - bob);

	const float stepL = airborne ? -stride * 0.6f :  sw * stride;
	const float stepR = airborne ? -stride * 0.6f : -sw * stride;

	const math::vector3 footL_w = foot_w + right * foot_span + fwd * stepL;
	const math::vector3 footR_w = foot_w - right * foot_span + fwd * stepR;

	const math::vector3 kneeL_w = (hip_w + footL_w) * 0.5f + fwd * (stride * 0.35f);
	const math::vector3 kneeR_w = (hip_w + footR_w) * 0.5f + fwd * (stride * 0.35f);

	const bool have_feet = add_point("$footL", footL_w) && add_point("$footR", footR_w);
	const bool have_knees = add_point("$kneeL", kneeL_w) && add_point("$kneeR", kneeR_w);

	if (have_feet)
	{
		if (have_knees)
		{
			out_pairs.push_back({ have_hips ? "$hipL" : "$hip", "$kneeL" });
			out_pairs.push_back({ have_hips ? "$hipR" : "$hip", "$kneeR" });
			out_pairs.push_back({ "$kneeL", "$footL" });
			out_pairs.push_back({ "$kneeR", "$footR" });
		}
		else
		{
			out_pairs.push_back({ have_hips ? "$hipL" : "$hip", "$footL" });
			out_pairs.push_back({ have_hips ? "$hipR" : "$hip", "$footR" });
		}
	}

	return (int)out_pairs.size();
}

struct cached_part_t
{
	std::uint64_t seen_ms{ 0 };
	std::uint64_t rot_ms{ 0 };
	std::uint64_t size_ms{ 0 };
	std::uint64_t prim_ms{ 0 };
	std::uint64_t pos_ms{ 0 };
	std::uint64_t attempt_ms{ 0 };
	std::uintptr_t primitive{ 0 };
	math::vector3 size{};
	math::vector3 position{};
	math::matrix3 rotation = math::matrix3::identity();
	int           rejected{ 0 };
	bool          size_ok{ false };
	bool          have_position{ false };
	bool          hull{ false };
	bool          classified{ false };
};

static std::unordered_map<std::uintptr_t, cached_part_t> g_part_geom;

struct last_centre_t
{
	math::vector3 pos{};
	std::uint64_t ms{};
};
static std::unordered_map<std::uint64_t, last_centre_t> g_last_centre;
static constexpr std::uint64_t k_centre_hold_ms = 1000;

static constexpr std::uint64_t k_size_ttl_ms   = 1000;
static constexpr std::uint64_t k_prims_ttl_ms  = 500;
static constexpr std::uint64_t k_rot_ttl_ms    = 25;
static constexpr std::uint64_t k_pos_ttl_ms    = 250;
static constexpr std::uint64_t k_entry_ttl_ms  = 3000;
static constexpr std::uint64_t k_pos_min_ms    = 4;
static constexpr float         k_max_jump      = 64.f;

static bool plausible_point(const math::vector3& v)
{
	if (!helper::finite3(v)) return false;
	if (std::fabs(v.x) > 1.0e6f || std::fabs(v.z) > 1.0e6f) return false;
	if (v.y < -5000.f || v.y > 30000.f) return false;
	return true;
}

static bool plausible_size(const math::vector3& v)
{
	if (!helper::finite3(v)) return false;
	if (std::fabs(v.x) > 2048.f || std::fabs(v.y) > 2048.f || std::fabs(v.z) > 2048.f) return false;
	return v.x != 0.f || v.y != 0.f || v.z != 0.f;
}

static bool plausible_rotation(const math::matrix3& r)
{
	return helper::finite3(r.right()) && helper::finite3(r.up()) && helper::finite3(r.forward());
}

static void prune_part_geometry()
{
	static std::uint64_t s_last_ms = 0;
	const std::uint64_t now = geometry_now_ms();
	if (now - s_last_ms < 1000) return;
	s_last_ms = now;

	for (auto it = g_part_geom.begin(); it != g_part_geom.end(); )
	{
		if (now - it->second.seen_ms > k_entry_ttl_ms)
			it = g_part_geom.erase(it);
		else
			++it;
	}

	if (g_hit_states.size() > 128)    g_hit_states.clear();
	if (g_health_smooth.size() > 128) g_health_smooth.clear();
	if (g_last_centre.size() > 128)   g_last_centre.clear();
}

static cached_part_t& resolve_part_geometry(const std::string& name, const rbx::part_t& part)
{
	cached_part_t& known = g_part_geom[(std::uintptr_t)part.address];
	const std::uint64_t now = geometry_now_ms();
	known.seen_ms = now;

	if (known.primitive == 0 || (now - known.prim_ms) > k_prims_ttl_ms)
	{
		known.prim_ms = now;

		std::uintptr_t prim = 0;
		try { prim = part.get_primitive().address; } catch (...) { prim = 0; }

		if (prim == 0)
		{
			known.primitive     = 0;
			known.size_ok       = false;
			known.have_position = false;
			known.rejected      = 0;
			return known;
		}

		if (prim != known.primitive)
		{

			known.primitive     = prim;
			known.size_ok       = false;
			known.have_position = false;
			known.rejected      = 0;
		}
	}

	if (!known.classified)
	{
		known.classified = true;
		if (name != "HumanoidRootPart")
		{
			try { known.hull = helper::looks_like_hull(name, part); }
			catch (...) { known.hull = false; }

			if (known.hull && op1::collision_boxes_enabled() && op1::is_hull_name(name))
				known.hull = false;
		}
	}

	if (known.primitive)
	{
		rbx::primitive_t prim{ known.primitive };

		if (!known.size_ok || (now - known.size_ms) > k_size_ttl_ms)
		{
			known.size_ms = now;
			try
			{
				const math::vector3 sz = prim.get_size();
				if (plausible_size(sz)) { known.size = sz; known.size_ok = true; }
			}
			catch (...) {}
		}

		if ((now - known.attempt_ms) >= k_pos_min_ms)
		{
			known.attempt_ms = now;
			try
			{
				const math::vector3 p = prim.get_position();
				const float         step = known.have_position ? (p - known.position).length() : 0.f;

				if (!plausible_point(p))
				{

				}
				else if (known.have_position && step > k_max_jump && known.rejected < 6)
				{
					++known.rejected;
				}
				else
				{
					known.rejected = 0;
					known.position = p;
					known.have_position = true;
					known.pos_ms = now;
				}
			}
			catch (...) {}
		}

		if ((now - known.rot_ms) >= k_rot_ttl_ms)
		{
			known.rot_ms = now;
			try
			{
				const math::matrix3 r = prim.get_rotation();
				if (plausible_rotation(r)) known.rotation = r;
			}
			catch (...) {}
		}
	}

	return known;
}

static bool cached_part_usable(const cached_part_t& g)
{
	if (!g.size_ok || !g.have_position) return false;
	if (!plausible_point(g.position) || !plausible_size(g.size)) return false;
	return (geometry_now_ms() - g.pos_ms) <= k_pos_ttl_ms;
}

static bool fits_avatar(const helper::world_box_t& b)
{
	if (!b.valid) return false;
	const math::vector3 span = b.max - b.min;

	return span.x <= 16.f && span.y <= 16.f && span.z <= 16.f;
}

static bool get_cached_geometry(const std::string& name, const rbx::part_t& part, geom_view_t& out)
{
	const cached_part_t& known = resolve_part_geometry(name, part);
	if (!cached_part_usable(known))
		return false;

	out.pos    = known.position;
	out.size   = known.size;
	out.rot    = plausible_rotation(known.rotation) ? known.rotation : math::matrix3::identity();
	out.half_h = cache::part_half_height(out.size, out.rot);
	out.volume = out.size.x * out.size.y * out.size.z;
	out.hull   = known.hull;
	out.ok     = true;
	return true;
}

static const rbx::part_t* find_body_part(const cache::entity_t& entity, std::string& name_out)
{
	auto named = entity.parts.find("HumanoidRootPart");
	if (named != entity.parts.end() && named->second.address)
	{
		name_out = named->first;
		return &named->second;
	}

	const rbx::part_t* best_part = nullptr;
	float best_volume = 0.f;

	for (int pass = 0; pass < 2; ++pass)
	{
		for (auto& [nm, part] : entity.parts)
		{
			if (!part.address) continue;

			geom_view_t g{};
			if (!get_cached_geometry(nm, part, g)) continue;
			if (pass == 0 && g.hull) continue;

			if (g.volume > best_volume)
			{
				best_volume = g.volume;
				best_part   = &part;
				name_out    = nm;
			}
		}
		if (best_part)
			break;
	}

	return best_part;
}

static bool rig_frame(const cache::entity_t& entity, rig_frame_t& out)
{
	std::string root_name;
	const rbx::part_t* root_part = find_body_part(entity, root_name);
	if (!root_part)
		return false;

	geom_view_t root{};
	if (!get_cached_geometry(root_name, *root_part, root))
		return false;

	const math::vector3 root_pos = root.pos;

	const bool body_only = op1::has_animated_body(entity);

	math::vector3 anchor_pos = root_pos;
	if (body_only)
	{
		math::vector3 sum{};
		int n = 0;
		for (auto& [nm, part] : entity.parts)
		{
			if (!part.address || op1::is_collider_body(nm)) continue;

			geom_view_t g{};
			if (!get_cached_geometry(nm, part, g)) continue;
			sum += g.pos;
			++n;
		}
		if (n > 0)
			anchor_pos = { sum.x / (float)n, sum.y / (float)n, sum.z / (float)n };
	}

	float top = -FLT_MAX, bottom = FLT_MAX;
	float widest = 0.f;

	for (int pass = 0; pass < 2 && top < bottom; ++pass)
	{
		top    = -FLT_MAX;
		bottom = FLT_MAX;

		for (auto& [nm, part] : entity.parts)
		{
			if (!part.address) continue;
			if (body_only && op1::is_collider_body(nm)) continue;

			geom_view_t g{};
			if (!get_cached_geometry(nm, part, g)) continue;
			if (pass == 0 && g.hull) continue;

			const math::vector3 d = g.pos - anchor_pos;
			if (sqrtf(d.x*d.x + d.y*d.y + d.z*d.z) > 12.f) continue;

			const bool swings = body_only && op1::is_swinging_limb(nm);

			if (!swings)
			{
				if (g.pos.y + g.half_h > top)    top    = g.pos.y + g.half_h;
				if (g.pos.y - g.half_h < bottom) bottom = g.pos.y - g.half_h;
			}

			if (!g.hull)
			{
				const float lateral = sqrtf(d.x*d.x + d.z*d.z) + std::max(g.size.x, g.size.z) * 0.5f;
				if (lateral > widest) widest = lateral;
			}
		}
	}

	const float min_height = body_only ? 0.8f : 2.f;

	float height = top - bottom;
	if (!std::isfinite(height) || height < min_height || height > 12.f)
	{		height = root.half_h * 2.f;
		if (!std::isfinite(height) || height < min_height || height > 12.f)
			height = body_only ? 1.2f : 5.f;
		top = (body_only ? anchor_pos.y : root_pos.y) + height * 0.5f;
	}

	out.top    = top;
	out.bottom = top - height;
	out.height = height;
	out.centre = math::vector3{ anchor_pos.x, (out.top + out.bottom) * 0.5f, anchor_pos.z };

	math::vector3 fwd = root.rot.forward();
	fwd.y = 0.f;
	if (!helper::finite3(fwd) || fwd.length() < 0.5f)
		fwd = math::vector3{ 0.f, 0.f, -1.f };
	fwd = fwd.normalized();

	out.forward = fwd;
	out.right   = math::vector3{ -fwd.z, 0.f, fwd.x };

	if (settings::visuals::box_mode == 0)
	{

		out.half_width = std::clamp(height * 0.275f, 0.35f, 1.8f);
	}
	else
	{

		out.half_width = std::clamp(std::max(height * 0.17f, widest * 0.5f), 0.35f, 1.8f);
	}
	out.root_name  = root_name;
	out.valid      = true;
	return true;
}

struct draw_stats_t
{
	std::uint64_t frames{ 0 };
	std::uint64_t drawn{ 0 };
	std::uint64_t no_volume{ 0 };
	std::uint64_t no_parts{ 0 };
	std::uint64_t nogeom{ 0 };
	std::uint64_t nofit{ 0 };
	std::uint64_t held{ 0 };
	std::uint64_t self{ 0 };
	std::uint64_t behind{ 0 };
	std::uint64_t tiny{ 0 };
	std::uint64_t team{ 0 };
	std::uint64_t dead{ 0 };
	std::uint64_t knock{ 0 };
	std::uint64_t range{ 0 };
	std::uint64_t visible{ 0 };
	std::string   name;
};

static std::unordered_map<std::uint64_t, draw_stats_t> g_draw_stats;

struct box_smooth_t
{
	float w{ 0.f }, h{ 0.f };
	bool  seen{ false };
};
static std::unordered_map<std::uint64_t, box_smooth_t> g_box_smooth;
static constexpr float k_box_smooth_rate = 25.f;
static constexpr float k_box_snap_px     = 400.f;

struct last_box_t
{
	float         left{}, top{}, right{}, bottom{};
	std::uint64_t ms{};
};
static std::unordered_map<std::uint64_t, last_box_t> g_last_box;
static constexpr std::uint64_t k_box_hold_ms = 250;

static void select_esp_font(ImFont*& font, float& size)
{
	font = ImGui::GetFont();
	size = ImGui::GetFontSize();

	const float want = settings::visuals::esp_font_size > 4.f
		? settings::visuals::esp_font_size
		: size;

	ImFont* picked = nullptr;
	switch (settings::visuals::esp_font)
	{
	case 0:  picked = aether::fonts::body();    break;
	case 1:  picked = aether::fonts::caption(); break;
	case 2:  picked = aether::fonts::medium();  break;
	case 3:  picked = aether::fonts::heading(); break;

	case 4:  picked = aether::fonts::esp_imgui();   break;

	case 5:  picked = aether::fonts::esp_display(want); break;
	case 6:  picked = aether::fonts::esp_clear(want);   break;

	case 7:  picked = aether::fonts::esp_inter();       break;
	default: picked = nullptr;                 break;
	}
	if (picked) font = picked;
	size = want;
}

static bool find_head_world(const cache::entity_t& entity, math::vector3& out)
{
	static const char* const k_head_names[] = { "Head", "head", "HitboxHead" };
	for (const char* n : k_head_names)
	{
		auto it = entity.parts.find(n);
		if (it == entity.parts.end() || !it->second.address) continue;

		try
		{
			rbx::primitive_t prim = it->second.get_primitive();
			if (prim.address)
			{
				const math::vector3 p = prim.get_position();
				if (plausible_point(p)) { out = p; return true; }
			}
		}
		catch (...) {}

		geom_view_t g{};
		if (get_cached_geometry(n, it->second, g)) { out = g.pos; return true; }
	}

	{
		rig_frame_t frame{};
		if (rig_frame(entity, frame) && frame.valid)
		{
			out = frame.centre;
			out.y = frame.top - std::max(0.4f, frame.height * 0.08f);
			return true;
		}
	}

	auto hrp = entity.parts.find("HumanoidRootPart");
	if (hrp != entity.parts.end() && hrp->second.address)
	{
		try
		{
			rbx::primitive_t prim = hrp->second.get_primitive();
			if (prim.address)
			{
				math::vector3 p = prim.get_position();
				if (plausible_point(p)) { p.y += 1.5f; out = p; return true; }
			}
		}
		catch (...) {}
	}

	out = { 0.f, 0.f, 0.f };
	return false;
}

static void draw_china_hat(ImDrawList* draw, const math::vector3& head,
	const math::matrix4& view, const math::vector2& dims, const float* col4)
{
	if (!draw || !col4) return;

	const float hat_h = std::max(0.1f, settings::visuals::china_hat_height);
	const float hat_r = std::max(0.1f, settings::visuals::china_hat_radius);
	constexpr int k_hat_segs = 48;

	auto project = [&](const math::vector3& p, ImVec2& out) -> bool {
		math::vector2 s{};
		if (!game::visengine.world_to_screen(p, s, dims, view)) return false;
		if (!std::isfinite(s.x) || !std::isfinite(s.y)) return false;
		out = { s.x, s.y };
		return true;
	};

	ImVec2 apex{};
	if (!project({ head.x, head.y + hat_h + 0.15f, head.z }, apex)) return;
	if (!std::isfinite(apex.x) || !std::isfinite(apex.y)) return;

	ImVec2 base[k_hat_segs];
	bool all_ok = true;
	for (int i = 0; i < k_hat_segs; ++i)
	{
		const float ang = 6.2831853f * (float)i / (float)k_hat_segs;
		ImVec2 sp{};
		if (!project({ head.x + hat_r * cosf(ang), head.y + 0.2f, head.z + hat_r * sinf(ang) }, sp))
		{
			all_ok = false;
			base[i] = ImVec2(-9999.f, -9999.f);
			continue;
		}
		base[i] = sp;
	}

	const ImU32 fill = IM_COL32((int)(col4[0] * 255.f), (int)(col4[1] * 255.f),
		(int)(col4[2] * 255.f), (int)(col4[3] * 255.f));
	const ImU32 base_col = IM_COL32((int)(col4[0] * 255.f), (int)(col4[1] * 255.f),
		(int)(col4[2] * 255.f), (int)(col4[3] * 0.6f * 255.f));
	const ImU32 outline = IM_COL32(0, 0, 0, 100);

	ImDrawListFlags fl = draw->Flags;
	draw->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines;

	const float soft = 2.f;
	for (int i = 0; i < k_hat_segs; ++i)
	{
		const int nxt = (i + 1) % k_hat_segs;
		if (base[i].x < -9000.f || base[nxt].x < -9000.f) continue;
		const float ang = 6.2831853f * (float)i / (float)k_hat_segs;
		const ImVec2 apex_off{ apex.x + cosf(ang) * soft, apex.y + sinf(ang) * soft };
		draw->AddTriangleFilled(apex_off, base[i], base[nxt], fill);
	}
	if (all_ok)
		draw->AddConvexPolyFilled(base, k_hat_segs, base_col);
	for (int i = 0; i < k_hat_segs; ++i)
	{
		const int nxt = (i + 1) % k_hat_segs;
		if (base[i].x < -9000.f || base[nxt].x < -9000.f) continue;
		draw->AddLine(base[i], base[nxt], outline, 1.2f);
	}

	draw->Flags = fl;
}

static bool find_look_world(const cache::entity_t& entity, math::vector3& out)
{
	static const char* const k_names[] = { "Head", "head", "HumanoidRootPart" };
	for (const char* n : k_names)
	{
		auto it = entity.parts.find(n);
		if (it == entity.parts.end() || !it->second.address) continue;

		try
		{
			rbx::primitive_t prim = it->second.get_primitive();
			if (!prim.address) continue;

			const math::matrix3 rot = prim.get_rotation();
			const math::vector3 f   = rot.forward();
			if (!math::finite(f)) continue;

			const float len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
			if (!(len > 0.001f)) continue;

			out = { f.x / len, f.y / len, f.z / len };
			return true;
		}
		catch (...) {}
	}

	out = { 0.f, 0.f, 0.f };
	return false;
}

static const char* humanoid_state_word(int hstate)
{
	static const char* const states[] = {
		"FallingDown", "Ragdoll", "GettingUp", "Jumping", "Swimming",
		"Freefall", "Flying", "Landed", "Running", "Unknown",
		"RunningNoPhysics", "StrafingNoPhysics", "Climbing", "Seated",
		"PlatformStanding", "Dead", "Physics", "Unknown", "None"
	};
	if (hstate >= 0 && hstate < (int)(sizeof(states) / sizeof(states[0])))
		return states[hstate];
	return "Unknown";
}

static const char* entity_state_word(const cache::entity_t& entity)
{
	if (entity.state == 8)
	{
		auto hd_it = entity.parts.find("Head");
		if (hd_it != entity.parts.end() && hd_it->second.address)
		{
			try
			{
				math::vector3 vel = memory->read<math::vector3>(
					hd_it->second.get_primitive().address + Offsets::Primitive::AssemblyLinearVelocity);
				if (vel.length() < 0.1f) return "Idle";
			}
			catch (...) {}
		}
	}
	return humanoid_state_word(entity.state);
}

static std::string rig_word(const cache::entity_t& entity)
{
	if (entity.rig_type == 1) return "R15";
	if (entity.rig_type == 0) return "R6";
	char buf[24];
	std::snprintf(buf, sizeof(buf), "Rig %u", (unsigned)entity.rig_type);
	return buf;
}

static void draw_head_dot(ImDrawList* draw, const math::vector3& head,
	const math::matrix4& view, const math::vector2& dims, float alpha)
{
	if (!draw || alpha <= 0.f) return;

	math::vector2 s{};
	if (!game::visengine.world_to_screen(head, s, dims, view)) return;
	if (!std::isfinite(s.x) || !std::isfinite(s.y)) return;

	const float r = std::max(0.5f, settings::visuals::head_dot_size);
	const float* c = settings::visuals::head_dot_color;

	if (settings::visuals::head_dot_outline)
	{
		const float* o = settings::visuals::head_dot_outline_color;
		draw->AddCircleFilled(ImVec2(s.x, s.y), r + 1.4f,
			helper::color_with_opacity(o[0], o[1], o[2], o[3], alpha), 20);
	}
	draw->AddCircleFilled(ImVec2(s.x, s.y), r,
		helper::color_with_opacity(c[0], c[1], c[2], c[3], alpha), 20);
}

static void draw_look_direction(ImDrawList* draw, const math::vector3& head,
	const math::vector3& dir, const math::matrix4& view, const math::vector2& dims,
	float alpha)
{
	if (!draw || alpha <= 0.f) return;

	const float len = std::max(0.1f, settings::visuals::look_direction_length);
	const math::vector3 tip{ head.x + dir.x * len, head.y + dir.y * len, head.z + dir.z * len };

	math::vector2 a{}, b{};
	if (!game::visengine.world_to_screen(head, a, dims, view)) return;
	if (!game::visengine.world_to_screen(tip,  b, dims, view)) return;
	if (!std::isfinite(a.x) || !std::isfinite(a.y) ||
		!std::isfinite(b.x) || !std::isfinite(b.y)) return;

	const float* c  = settings::visuals::look_direction_color;
	const float  th = std::max(0.5f, settings::visuals::look_direction_thickness);
	const ImU32  col = helper::color_with_opacity(c[0], c[1], c[2], c[3], alpha);

	draw->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), IM_COL32(0, 0, 0, (int)(200.f * alpha)), th + 1.4f);
	draw->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), col, th);
	draw->AddCircleFilled(ImVec2(b.x, b.y), th + 1.4f, col, 16);
}

void esp::run()
{

	static int s_skipped_team = 0, s_skipped_dead = 0, s_skipped_knock = 0;
	static int s_skipped_range = 0, s_skipped_visible = 0, s_drawn_boxes = 0;

	static std::uint64_t s_dm_seen = 0;
	if (game::datamodel.address && game::datamodel.address != s_dm_seen)
	{
		if (s_dm_seen)
		{
			g_hit_states.clear();
			g_health_smooth.clear();

			g_part_geom.clear();
			g_draw_stats.clear();
			g_last_box.clear();
			g_last_centre.clear();
			g_skel_ease.clear();
		}
		s_dm_seen = game::datamodel.address;
	}

	prune_part_geometry();
	if (g_draw_stats.size() > 128) g_draw_stats.clear();
	if (g_box_smooth.size() > 128) g_box_smooth.clear();
	if (g_last_box.size() > 128)   g_last_box.clear();
	if (g_skel_ease.size() > 128)  g_skel_ease.clear();

	if (!game::visengine.address || !game::workspace.address)
	{
		static auto s_guard_at = std::chrono::steady_clock::now() - std::chrono::seconds(6);
		auto gnow = std::chrono::steady_clock::now();
		if (gnow - s_guard_at >= std::chrono::seconds(5))
		{
			s_guard_at = gnow;

			debug::log("esp", "blocked | visengine 0x%llx | workspace 0x%llx",
				(unsigned long long)game::visengine.address,
				(unsigned long long)game::workspace.address);
		}
		return;
	}

	float fi = settings::visuals::fade_in_speed;
	float fo = settings::visuals::fade_out_speed;
	g_fade.box       = fade_tick(g_fade.box,       settings::visuals::box,          fi, fo);
	g_fade.box_fill  = fade_tick(g_fade.box_fill,  settings::visuals::box_fill,     fi, fo);
	g_fade.name      = fade_tick(g_fade.name,       settings::visuals::name || settings::visuals::avatar, fi, fo);
	g_fade.healthbar = fade_tick(g_fade.healthbar, settings::visuals::healthbar,   fi, fo);
	g_fade.arrows      = fade_tick(g_fade.arrows,      settings::visuals::offscreen_arrows, fi, fo);
	g_fade.tracer      = fade_tick(g_fade.tracer,      settings::visuals::tracer,           fi, fo);
	g_fade.china_hat   = fade_tick(g_fade.china_hat,   settings::visuals::china_hat,        fi, fo);
	g_fade.head_dot    = fade_tick(g_fade.head_dot,    settings::visuals::head_dot,         fi, fo);
	g_fade.look_dir    = fade_tick(g_fade.look_dir,    settings::visuals::look_direction,   fi, fo);
	g_fade.rig_state   = fade_tick(g_fade.rig_state,   settings::visuals::rigtype,          fi, fo);
	g_fade.state_tag   = fade_tick(g_fade.state_tag,   settings::visuals::state,            fi, fo);
	g_fade.health_text = fade_tick(g_fade.health_text, settings::visuals::health_text,      fi, fo);
	g_fade.health_pct= fade_tick(g_fade.health_pct,settings::visuals::health_percent, fi, fo);
	g_fade.distance  = fade_tick(g_fade.distance,  settings::visuals::distance,    fi, fo);
	g_fade.tool      = fade_tick(g_fade.tool,       settings::visuals::tool,        fi, fo);
	g_fade.flags     = fade_tick(g_fade.flags,     settings::visuals::flags,       fi, fo);
	g_fade.chams     = fade_tick(g_fade.chams,     settings::visuals::chams,       fi, fo);

	rbx::visualengine_t::update_window_offset();

	if (!settings::visuals::enable_enemies && !settings::visuals::view_hitbox)
	{
		aimbot::draw_fov(ImGui::GetBackgroundDrawList(), game::visengine.get_dimensions());
		return;
	}

	if (settings::visuals::chams && g_fade.chams > 0.f)
	{
		const math::vector2 dims_now = game::visengine.get_dimensions();
		const math::matrix4 view_now = game::visengine.get_viewmatrix();

		math::vector3 eye{};
		const std::uint64_t cam = rbx::current_camera_address();
		if (cam)
		{
			const math::vector3 p = rbx::camera_t(cam).get_position();
			if (math::finite(p))
				eye = p;
		}

		mesh::chams_begin_frame(view_now, dims_now, eye);
	}

	math::vector2 dims = game::visengine.get_dimensions();
	math::matrix4 view = game::visengine.get_viewmatrix();

	std::vector<cache::entity_t> snap;
	std::string local_team_name;
	{
		std::lock_guard<std::mutex> lk(cache::mtx);
		snap = cache::cached_players;
		local_team_name = cache::cached_local_team_name;
	}

	std::uint64_t local_addr = 0;

	{
		std::lock_guard<std::mutex> lk(cache::mtx);
		local_addr = cache::cached_local_player.instance.address;
	}
	if (!local_addr)
	{
		if (game::local_player.address)
			local_addr = game::local_player.address;
		else if (game::players.address)
			local_addr = memory->read<std::uint64_t>(game::players.address + Offsets::Player::LocalPlayer);
	}

	{
		static auto s_esp_diag = std::chrono::steady_clock::now() - std::chrono::seconds(6);
		auto enow = std::chrono::steady_clock::now();
		if (enow - s_esp_diag >= std::chrono::seconds(5))
		{
			s_esp_diag = enow;

			math::vector2 probe{ -1.f, -1.f };
			{
				const std::uint64_t cam = rbx::current_camera_address();
				if (cam)
				{
					try
					{
						const math::vector3 cp = memory->read<math::vector3>(cam + Offsets::Camera::Position);
						const math::matrix3 cr = memory->read<math::matrix3>(cam + Offsets::Camera::Rotation);
						if (math::finite(cp) && math::finite(cr.forward()))
							game::visengine.world_to_screen(cp + cr.forward().normalized() * 10.f, probe, dims, view);
					}
					catch (...) { probe = { -1.f, -1.f }; }
				}
			}

			int enemy_count = 0, team_ents = 0, corpse_ents = 0;
			for (const auto& e : snap)
			{
				if (!e.instance.address || e.instance.address == local_addr) continue;
				if (cache::is_teammate(e)) { ++team_ents;   continue; }
				if (entity_is_dead(e))     { ++corpse_ents; continue; }
				++enemy_count;
			}

			char cham_status[224];
			mesh::dx::shader::StatusText(cham_status, sizeof cham_status);

			debug::log("esp", "run | enemies=%d team=%d%scorpses=%d hitbox=%d box=%d name=%d ents=%zu drawn=%d local=0x%llx dims=%.0fx%.0f proj=%s@0x%llx projcheck=%.0f,%.0f (want %.0f,%.0f) chams=%d %s checks team=%d dead=%d knock=%d vis=%d wg=%d | skips team=%d dead=%d knock=%d range=%d vis=%d",
				enemy_count, team_ents,
				local_team_name.empty() ? "(no team) " : ("('" + local_team_name + "') ").c_str(),
				corpse_ents,
				settings::visuals::view_hitbox ? 1 : 0,
				settings::visuals::box ? 1 : 0,
				(settings::visuals::name || settings::visuals::avatar) ? 1 : 0,
				snap.size(),
				s_drawn_boxes,
				(unsigned long long)local_addr,
				dims.x, dims.y,
				rbx::visualengine_t::using_native_projection() ? "engine" : "camera",
				(unsigned long long)rbx::visualengine_t::native_projection_offset(),
				probe.x, probe.y,
				rbx::visualengine_t::window_offset_x() + dims.x * 0.5f,
				rbx::visualengine_t::window_offset_y() + dims.y * 0.5f,
				settings::visuals::chams ? 1 : 0,
				cham_status,
				settings::visuals::team_check ? 1 : 0,
				settings::visuals::dead_check ? 1 : 0,
				settings::visuals::knock_check ? 1 : 0,
				settings::visuals::visible_check ? 1 : 0,
				worldgeo::box_count(),
				s_skipped_team, s_skipped_dead, s_skipped_knock, s_skipped_range, s_skipped_visible);

			s_skipped_team = 0; s_skipped_dead = 0; s_skipped_knock = 0;
			s_skipped_range = 0; s_skipped_visible = 0; s_drawn_boxes = 0;

			char unstable[1024]{};
			int  unstable_len = 0;
			for (auto& [addr, stats] : g_draw_stats)
			{
				if (stats.drawn >= stats.frames || unstable_len >= (int)sizeof(unstable) - 96)
					continue;

				const int written = std::snprintf(unstable + unstable_len,
					sizeof(unstable) - unstable_len,
					" [%s %llu/%llu nv=%llu ng=%llu nf=%llu hl=%llu np=%llu slf=%llu beh=%llu tiny=%llu tm=%llu dd=%llu kn=%llu rg=%llu vs=%llu]",
					stats.name.empty() ? "?" : stats.name.c_str(),
					(unsigned long long)stats.drawn, (unsigned long long)stats.frames,
					(unsigned long long)stats.no_volume, (unsigned long long)stats.nogeom,
					(unsigned long long)stats.nofit, (unsigned long long)stats.held,
					(unsigned long long)stats.no_parts, (unsigned long long)stats.self,
					(unsigned long long)stats.behind, (unsigned long long)stats.tiny,
					(unsigned long long)stats.team, (unsigned long long)stats.dead,
					(unsigned long long)stats.knock, (unsigned long long)stats.range,
					(unsigned long long)stats.visible);
				if (written > 0)
					unstable_len += written;
			}

			if (unstable_len > 0)
				debug::log("esp", "frames lost (drawn/total, nv=no box this frame, ng=no usable part geometry, nf=body would not project, hl=drew the held rectangle, np=rig unresolved, slf=local player hidden, beh=behind camera, tiny=sub-pixel, tm/dd/kn/rg=filters):%s", unstable);

			for (auto it = g_draw_stats.begin(); it != g_draw_stats.end(); )
				it = g_draw_stats.erase(it);
		}
	}

	math::vector3 local_pos{};
	{

		rbx::part_t  hrp_part{};
		bool         have_hrp = false;
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			auto it = cache::cached_local_player.parts.find("HumanoidRootPart");
			if (it != cache::cached_local_player.parts.end() && it->second.address)
			{
				hrp_part = it->second;
				have_hrp = true;
			}
		}
		if (have_hrp)
		{
			try {
				rbx::primitive_t prim = hrp_part.get_primitive();
				if (prim.address) local_pos = prim.get_position();
			} catch (...) {}
		}
		else if (local_addr)
		{
			try {

				rbx::instance_t inst(local_addr);
				rbx::instance_t rig = inst;
				if (inst.get_class_name() == "Player")
					rig = rbx::player_t(local_addr).get_model_instance();
				if (rig.address)
				{
					rbx::instance_t hrp_inst = rig.find_first_child("HumanoidRootPart");
					if (hrp_inst.address)
					{
						rbx::primitive_t prim = rbx::part_t(hrp_inst.address).get_primitive();
						if (prim.address) local_pos = prim.get_position();
					}
				}
			} catch (...) {}
		}
	}

	const math::vector3 eye = worldgeo::eye_position(local_pos);

	static auto s_last = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	float dt = std::min(std::chrono::duration<float>(now - s_last).count(), 0.1f);
	s_last = now;
	for (auto& [a, hs] : g_hit_states)
		hs.timer = std::max(0.f, hs.timer - dt);

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
	g_offscreen.clear();

	for (auto& entity : snap)
	{
		if (!entity.instance.address) continue;
		bool is_local = (entity.instance.address == local_addr);

		draw_stats_t& st = g_draw_stats[entity.instance.address];
		st.frames++;
		if (!entity.name.empty()) st.name = entity.name;

		if (is_local && !settings::visuals::localplayer) { ++st.self; continue; }
		if (!is_local && !settings::visuals::enable_enemies && !settings::visuals::view_hitbox) continue;
		if (!is_local && settings::visuals::knock_check && is_player_knocked(entity)) { ++s_skipped_knock; ++st.knock; continue; }
		const bool friend_of_mine = !is_local && friends::is_friend(entity.instance.address);
		if (!is_local && !friend_of_mine && settings::visuals::team_check && cache::is_teammate(entity)) { ++s_skipped_team; ++st.team; continue; }

		const bool is_dead_ent = entity_is_dead(entity);
		const bool corpse_mode = !is_local && is_dead_ent && settings::visuals::body_corpse;
		if (!is_local && is_dead_ent && !corpse_mode)
		{ ++s_skipped_dead; ++st.dead; continue; }

		const bool aim_target =
			(entity.instance.address != 0) &&
			(targets::is_target(entity.instance.address) ||
				aimbot::player.instance.address == entity.instance.address ||
				silentaim::state.target.instance.address == entity.instance.address);

		const float* highlight = friend_of_mine || aim_target
			? highlight_colour(entity.instance.address, aim_target)
			: nullptr;

		math::vector3 centre{};
		bool have_centre = false;
		std::string centre_name;
		{
			const rbx::part_t* root_part = find_body_part(entity, centre_name);
			if (root_part)
			{
				geom_view_t root{};
				if (get_cached_geometry(centre_name, *root_part, root))
				{
					centre = root.pos;
					have_centre = true;
				}
			}
		}

		{
			const std::uint64_t now = geometry_now_ms();
			if (have_centre)
			{
				g_last_centre[entity.instance.address] = { centre, now };
			}
			else
			{
				auto last = g_last_centre.find(entity.instance.address);
				if (last != g_last_centre.end() && (now - last->second.ms) <= k_centre_hold_ms)
				{
					centre      = last->second.pos;
					have_centre = true;
				}
			}
		}

		constexpr float k_max_part_dist = 8.f;

		if (settings::visuals::distance_check && settings::visuals::max_distance > 0.f &&
			!is_local && have_centre && !corpse_mode)
		{
			const math::vector3 d = centre - local_pos;
			float dist = sqrtf(d.x*d.x + d.y*d.y + d.z*d.z);

			if (settings::visuals::distance_unit == 1) dist *= 0.28f;
			if (dist > settings::visuals::max_distance)
			{
				++s_skipped_range;
				++st.range;
				continue;
			}
		}

		if (settings::visuals::visible_check && !is_local && have_centre)
		{
			if (!worldgeo::is_visible(eye, centre))
			{
				++s_skipped_visible;
				++st.visible;
				continue;
			}
		}

		bool hrp_behind = false;
		if (have_centre)
		{
			math::vector4 clip = view.multiply({ centre.x, centre.y, centre.z, 1.f });
			hrp_behind = (clip.w <= 0.f);
		}

		float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = -FLT_MAX;
		bool  box_valid = false;
		bool  had_geometry = false;
		bool  used_hold = false;

		{
			struct part_geom_t
			{
				math::vector3 pos{};
				math::matrix3 rot{};
				math::vector3 size{};
				bool          hull{ false };
			};

			part_geom_t geoms[24]{};
			int         geom_count = 0;

			for (auto& [pname, part] : entity.parts)
			{
				if (!part.address || geom_count >= 24) continue;

				const cached_part_t& known = resolve_part_geometry(pname, part);
				if (!cached_part_usable(known)) continue;

				if (have_centre && pname != "HumanoidRootPart" &&
					(known.position - centre).length() > k_max_part_dist)
					continue;

				part_geom_t& g = geoms[geom_count];
				g.pos  = known.position;
				g.size = known.size;
				g.rot  = plausible_rotation(known.rotation) ? known.rotation : math::matrix3::identity();

				g.hull = known.hull;

				++geom_count;
			}

			helper::world_box_t volume{};
			int used = 0;
			for (int pass = 0; pass < 2 && used == 0; ++pass)
			{

				for (int i = 0; i < geom_count; ++i)
				{
					if (pass == 0 && geoms[i].hull) continue;

					helper::world_box_t trial = volume;
					helper::include_oriented(trial, geoms[i].pos, geoms[i].rot, geoms[i].size);
					if (!fits_avatar(trial)) continue;

					volume = trial;
					++used;
				}
			}

			had_geometry = (used > 0);

			const float ox = rbx::visualengine_t::window_offset_x();
			const float oy = rbx::visualengine_t::window_offset_y();
			const float half_w = dims.x * 0.5f;
			const float half_h = dims.y * 0.5f;

			auto project_volume = [&](const helper::world_box_t& v) -> int
			{
				if (!v.valid) return 0;

				int landed = 0;
				for (int i = 0; i < 8; ++i)
				{
					math::vector3 w{
						(i & 1) ? v.max.x : v.min.x,
						(i & 2) ? v.max.y : v.min.y,
						(i & 4) ? v.max.z : v.min.z
					};

					math::vector2 s{};
					if (!game::visengine.world_to_screen(w, s, dims, view)) continue;
					if (!std::isfinite(s.x) || !std::isfinite(s.y)) continue;

					const float cx = std::clamp(s.x, ox - half_w, ox + dims.x + half_w);
					const float cy = std::clamp(s.y, oy - half_h, oy + dims.y + half_h);

					box_valid = true;
					left   = std::min(left,   cx);
					top    = std::min(top,    cy);
					right  = std::max(right,  cx);
					bottom = std::max(bottom, cy);
					++landed;
				}
				return landed;
			};

			project_volume(volume);

			if (!box_valid && have_centre)
			{
				for (float scale : { 0.5f, 0.25f })
				{
					helper::world_box_t shrunk{};
					shrunk.valid = true;
					shrunk.min = {
						centre.x + (volume.min.x - centre.x) * scale,
						centre.y + (volume.min.y - centre.y) * scale,
						centre.z + (volume.min.z - centre.z) * scale
					};
					shrunk.max = {
						centre.x + (volume.max.x - centre.x) * scale,
						centre.y + (volume.max.y - centre.y) * scale,
						centre.z + (volume.max.z - centre.z) * scale
					};

					if (project_volume(shrunk)) break;
				}
			}
		}

		if (settings::visuals::box_mode == 0 && have_centre)
		{
			rig_frame_t rig{};
			if (rig_frame(entity, rig) && rig.valid)
			{
				math::vector2 ts{}, bs{};
				const math::vector3 top_w{ rig.centre.x, rig.top,    rig.centre.z };
				const math::vector3 bot_w{ rig.centre.x, rig.bottom, rig.centre.z };

				if (game::visengine.world_to_screen(top_w, ts, dims, view) &&
					game::visengine.world_to_screen(bot_w, bs, dims, view) &&
					std::isfinite(ts.x) && std::isfinite(ts.y) &&
					std::isfinite(bs.y))
				{
					const float static_h = bs.y - ts.y;
					if (static_h > 4.f)
					{
						const float static_w = static_h * 0.55f;
						left   = ts.x - static_w * 0.5f;
						right  = ts.x + static_w * 0.5f;
						top    = ts.y;
						bottom = bs.y;
						box_valid    = true;
						had_geometry = true;
					}
				}
			}
		}

		draw_hitbox_3d(entity, view, dims, draw, local_addr);

		if (settings::visuals::offscreen_arrows && g_fade.arrows > 0.f && !is_local &&
			have_centre && !corpse_mode && (!box_valid || hrp_behind))
		{

			math::vector4 clip = view.multiply({ centre.x, centre.y, centre.z, 1.f });
			if (clip.w > -1e9f && clip.w < 1e9f &&
				(fabsf(clip.x) > 1e-4f || fabsf(clip.y) > 1e-4f))
			{

				const float sgn = (clip.w > 0.f) ? 1.f : -1.f;

				char info_buf[160]{};
				int  used = 0;
				auto append = [&](const char* part) {
					if (!part || !part[0] || used >= (int)sizeof(info_buf) - 4) return;
					const int n = std::snprintf(info_buf + used, sizeof(info_buf) - used,
						used > 0 ? " | %s" : "%s", part);
					if (n > 0 && used + n < (int)sizeof(info_buf)) used += n;
				};
				if (settings::visuals::arrow_info[0])
				{
					const std::string& shown = (settings::visuals::name_display_type == 0 &&
						!entity.display_name.empty()) ? entity.display_name : entity.name;
					append(shown.c_str());
				}
				{
					const math::vector3 dd = centre - local_pos;
					const float adist = sqrtf(dd.x*dd.x + dd.y*dd.y + dd.z*dd.z);
					if (settings::visuals::arrow_info[1])
					{
						char dbuf[32];
						if (settings::visuals::distance_unit == 1)
							std::snprintf(dbuf, sizeof(dbuf), "%.0fm", adist * 0.28f);
						else
							std::snprintf(dbuf, sizeof(dbuf), "%.0f", adist);
						append(dbuf);
					}
				}
				if (settings::visuals::arrow_info[2] && entity.humanoid.address)
				{
					try {
						char hbuf[32];
						std::snprintf(hbuf, sizeof(hbuf), "%.0f",
							rbx::humanoid_t(entity.humanoid.address).get_health());
						append(hbuf);
					} catch (...) {}
				}
				if (settings::visuals::arrow_info[3] && !entity.tool_name.empty())
					append(entity.tool_name.c_str());

				const arrow_tint_t tint = aim_target ? arrow_aim
					: (friend_of_mine ? arrow_friend : arrow_base);

				g_offscreen.push_back({ clip.x * sgn, -clip.y * sgn, tint, std::string(info_buf) });
			}
		}

		if (hrp_behind) { ++st.behind; continue; }
		if (!box_valid || left >= right || top >= bottom)
		{

			++st.no_volume;
			if (entity.parts.empty())     ++st.no_parts;
			else if (!had_geometry)       ++st.nogeom;
			else                          ++st.nofit;

			const std::uint64_t hold_now = geometry_now_ms();
			auto held = g_last_box.find(entity.instance.address);
			if (have_centre && held != g_last_box.end() &&
				(hold_now - held->second.ms) <= k_box_hold_ms)
			{
				const float held_w = held->second.right - held->second.left;
				const float held_h = held->second.bottom - held->second.top;

				bool  placed = false;
				float place_x = 0.f, place_y = 0.f;

				if (held_w > 0.f && held_h > 0.f)
				{
					const math::vector4 clip =
						view.multiply({ centre.x, centre.y, centre.z, 1.f });

					if (clip.w > 1.0e-4f)
					{
						const float nx = clip.x / clip.w;
						const float ny = clip.y / clip.w;
						if (std::isfinite(nx) && std::isfinite(ny))
						{
							const float ox = rbx::visualengine_t::window_offset_x();
							const float oy = rbx::visualengine_t::window_offset_y();

							place_x = std::clamp(dims.x * 0.5f * nx + dims.x * 0.5f + ox,
								ox - held_w * 0.5f, ox + dims.x + held_w * 0.5f);
							place_y = std::clamp(-dims.y * 0.5f * ny + dims.y * 0.5f + oy,
								oy - held_h * 0.5f, oy + dims.y + held_h * 0.5f);
							placed = true;
						}
					}
				}

				if (placed)
				{
					left   = place_x - held_w * 0.5f;
					right  = place_x + held_w * 0.5f;
					top    = place_y - held_h * 0.5f;
					bottom = place_y + held_h * 0.5f;
				}
				else
				{
					left   = held->second.left;
					top    = held->second.top;
					right  = held->second.right;
					bottom = held->second.bottom;
				}

				used_hold = true;
				++st.held;
			}
			else
			{
				continue;
			}
		}

		if (right - left < 1.5f || bottom - top < 1.5f) { ++st.tiny; continue; }

		++s_drawn_boxes;
		++st.drawn;

		float box_left  = roundf(left);
		float box_right = roundf(right);
		float box_top   = roundf(top);
		float box_bot   = roundf(bottom);

		if (entity.instance.address && dt > 0.f)
		{
			box_smooth_t& sm = g_box_smooth[entity.instance.address];
			const float fresh_w = box_right - box_left;
			const float fresh_h = box_bot   - box_top;

			if (!sm.seen)
			{
				sm.w = fresh_w;
				sm.h = fresh_h;
				sm.seen = true;
			}
			else
			{
				const float jump = std::max(std::fabs(fresh_w - sm.w), std::fabs(fresh_h - sm.h));
				const float a = (jump > k_box_snap_px) ? 1.f : 1.f - expf(-dt * k_box_smooth_rate);

				sm.w += (fresh_w - sm.w) * a;
				sm.h += (fresh_h - sm.h) * a;
			}

			box_right = roundf(box_left + sm.w);
			box_bot   = roundf(box_top  + sm.h);
		}

		if (!used_hold)
		{
			last_box_t& slot = g_last_box[entity.instance.address];
			slot.left   = box_left;
			slot.top    = box_top;
			slot.right  = box_right;
			slot.bottom = box_bot;
			slot.ms     = geometry_now_ms();
		}

		float box_w     = box_right - box_left;
		float box_h     = box_bot - box_top;
		float box_cx    = box_left + box_w * 0.5f;

		ImVec2 c1(box_left, box_top);
		ImVec2 c2(box_w, box_h);

		float entity_hp = 0.f, entity_maxhp = 100.f;
		if (entity.humanoid.address) {
			rbx::humanoid_t h = entity.humanoid;
			entity_hp    = h.get_health();
			entity_maxhp = h.get_max_health();
		}

		if (!std::isfinite(entity_maxhp) || entity_maxhp <= 0.f || entity_maxhp > 1e6f)
			entity_maxhp = 100.f;
		if (!std::isfinite(entity_hp) || entity_hp < 0.f)
			entity_hp = 0.f;
		entity_hp = std::min(entity_hp, entity_maxhp);

		float hit_flash = 0.f;
		{
			auto& hs = g_hit_states[entity.instance.address];
			float max_dur = std::max(0.01f, std::max(
				settings::visuals::hit_skeleton_duration,
				settings::visuals::hit_chams_duration));

			hs.last_hp = entity_hp;
			hit_flash  = effects::own_hit_fade(entity.instance.address, max_dur);
			hs.timer   = hit_flash * max_dur;
			hs.last_hp = entity_hp;
			hit_flash  = std::min(1.f, hs.timer / max_dur);
		}

		float hp_pct = entity_hp / entity_maxhp;
		if (entity.instance.address)
		{
			auto& hs = g_health_smooth[entity.instance.address];
			if (!hs.seen) { hs.hp = entity_hp; hs.seen = true; }
			hs.hp += (entity_hp - hs.hp) * 0.15f;
			hs.hp  = std::max(0.f, std::min(hs.hp, entity_maxhp));
			hp_pct = hs.hp / entity_maxhp;
		}

		ImFont*  font      = nullptr;
		float    font_size = 0.f;
		select_esp_font(font, font_size);

		if (settings::visuals::box_fill && g_fade.box_fill > 0.f)
		{
			ImRect fr(c1.x, c1.y, c1.x + c2.x, c1.y + c2.y);

			if (settings::visuals::box_fill_mode == 1)
			{
				boxfill::draw(draw, 0, fr.Min, fr.Max,
					settings::visuals::box_fill_image_alpha * g_fade.box_fill, false);
			}
			else if (settings::visuals::box_gradient)
			{

				auto& gt = settings::visuals::box_gradient_top;
				auto& gb = settings::visuals::box_gradient_bottom;
				const ImU32 top = helper::color_with_opacity(gt[0], gt[1], gt[2], gt[3], g_fade.box_fill);
				const ImU32 bot = helper::color_with_opacity(gb[0], gb[1], gb[2], gb[3], g_fade.box_fill);
				draw->AddRectFilledMultiColor(fr.Min, fr.Max, top, top, bot, bot);
			}
			else
			{
				auto& fc = settings::visuals::box_fill_color;
				const ImU32 col = helper::color_with_opacity(fc[0],fc[1],fc[2],fc[3], g_fade.box_fill);
				draw->AddRectFilled(fr.Min, fr.Max, col);
			}
		}

		float actual_box_left  = box_left;
		float actual_box_right = box_right;
		if (settings::visuals::box && g_fade.box > 0.f)
		{

			const float* bc = corpse_mode ? settings::visuals::corpse_color : settings::visuals::box_color;
			const float* box_src = (!corpse_mode && highlight) ? highlight : bc;
			ImU32 col = helper::color_with_opacity(box_src[0], box_src[1], box_src[2], box_src[3], g_fade.box);
			if (settings::visuals::box_type == 0)
			{
				helper::box(c1, c2, col, settings::visuals::box_thickness);
				actual_box_left  = c1.x;
				actual_box_right = c1.x + c2.x;
			}
			else
			{
				ImRect rb(c1.x, c1.y, c1.x + c2.x, c1.y + c2.y);
				helper::corner_box(draw, rb.Min, rb.Max, col, settings::visuals::box_thickness);
				actual_box_left  = roundf(rb.Min.x);
				actual_box_right = roundf(rb.Max.x);
			}
		}

		if (!is_local && !corpse_mode && settings::visuals::tracer && g_fade.tracer > 0.f)
		{
			const float ccx = rbx::visualengine_t::window_offset_x() + dims.x * 0.5f;
			const float ccy = rbx::visualengine_t::window_offset_y() + dims.y * 0.5f;
			const float cty = rbx::visualengine_t::window_offset_y();
			const float cby = rbx::visualengine_t::window_offset_y() + dims.y;

			ImVec2 from{};
			switch (settings::visuals::tracer_origin)
			{
			case 1:  from = { ccx, ccy }; break;
			case 2:
			{
				POINT pt{};
				from = GetCursorPos(&pt) ? ImVec2((float)pt.x, (float)pt.y) : ImVec2(ccx, ccy);
				break;
			}
			case 3:  from = { ccx, cty }; break;
			default: from = { ccx, cby }; break;
			}
			const ImVec2 to = { box_cx, box_bot };
			auto& tc = settings::visuals::tracer_color;
			const float* trace_src = highlight ? highlight : tc;
			const ImU32 col = helper::color_with_opacity(trace_src[0], trace_src[1], trace_src[2],
				trace_src[3], g_fade.tracer);
			draw->AddLine(from, to, IM_COL32(0, 0, 0, 200), 2.5f);
			draw->AddLine(from, to, col, 1.0f);
		}

		if (!corpse_mode && settings::visuals::china_hat && g_fade.china_hat > 0.f)
		{
			math::vector3 head_pos{};
			if (find_head_world(entity, head_pos))
				draw_china_hat(draw, head_pos, view, dims, settings::visuals::china_hat_color);
		}

		if (!corpse_mode && (settings::visuals::head_dot || settings::visuals::look_direction))
		{
			math::vector3 head_pos{};
			if (find_head_world(entity, head_pos))
			{
				if (settings::visuals::head_dot && g_fade.head_dot > 0.f)
					draw_head_dot(draw, head_pos, view, dims, g_fade.head_dot);

				if (settings::visuals::look_direction && g_fade.look_dir > 0.f)
				{
					math::vector3 dir{};
					if (find_look_world(entity, dir))
						draw_look_direction(draw, head_pos, dir, view, dims, g_fade.look_dir);
				}
			}
		}

		if (!corpse_mode && settings::visuals::healthbar && g_fade.healthbar > 0.f && entity.humanoid.address)
		{
			float pct = hp_pct;

			float r, g, b;
			if (settings::visuals::health_based_healthbar)
			{
				if (pct > 0.6f)      { r=0.f;  g=1.f;    b=0.f; }
				else if (pct >= 0.5f){ r=1.f;  g=0.647f; b=0.f; }
				else if (pct <= 0.2f){ r=1.f;  g=0.f;    b=0.f; }
				else                  { r=1.f;  g=0.647f; b=0.f; }
			}
			else
			{
				r = settings::visuals::healthbar_color[0];
				g = settings::visuals::healthbar_color[1];
				b = settings::visuals::healthbar_color[2];
			}

			const float bar_alpha = std::clamp(settings::visuals::healthbar_opacity / 100.f, 0.f, 1.f);
			float tr = 255.f * g_fade.healthbar;
			float bar_h = box_h + 2.f;

			float bar_tp = std::max(0.f, box_top - 2.f);
			float bar_bt = std::min(dims.y, box_bot + 2.f);
			if (bar_bt > bar_tp + 2.f)
			{
				if (settings::visuals::healthbar_enhanced)
				{

					const float w   = std::clamp(settings::visuals::healthbar_width, 2.f, 24.f);
					const float x   = box_left - w - 5.f;
					const float top = bar_tp;
					const float bot = bar_bt;
					const float rnd = w * 0.5f;

					const ImU32 track = IM_COL32(10, 10, 12, (int)(tr * 0.85f * bar_alpha));
					const ImU32 edge  = IM_COL32(255, 255, 255, (int)(tr * 0.20f * bar_alpha));

					if (settings::visuals::healthbar_glow)
						draw->AddRectFilled({x - 3.f, top - 2.f}, {x + w + 3.f, bot + 2.f},
							IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(tr * 0.18f * bar_alpha)), rnd + 3.f);

					draw->AddRectFilled({x, top}, {x + w, bot}, track, rnd);
					draw->AddRect({x, top}, {x + w, bot}, edge, rnd, 0, 1.f);

					if (pct > 0.f)
					{
						const float room   = bot - top - 2.f;
						const float fill_h = std::max(1.f, room * pct);
						const float fy0    = bot - 1.f - fill_h;
						const float fy1    = bot - 1.f;
						const float irnd   = std::max(0.f, rnd - 1.f);
						const ImU32 fill_col = IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(tr * bar_alpha));
						draw->AddRectFilled({x + 1.f, fy0}, {x + w - 1.f, fy1}, fill_col, irnd);

						draw->AddRectFilled({x + 1.f, fy0}, {x + w - 1.f, fy0 + std::min(2.f, fill_h)},
							IM_COL32(255, 255, 255, (int)(tr * 0.7f * bar_alpha)), irnd);
					}
				}
				else
				{
					draw->AddRectFilled({box_left-7.f, bar_tp}, {box_left-3.f, bar_bt},
						IM_COL32(0,0,0,(int)(tr * bar_alpha)));
					if (pct > 0.f)
					{
						float fill = std::min(bar_h * pct, bar_bt - bar_tp);
						draw->AddRectFilled(
							{box_left-6.f, bar_bt - 1.f - fill},
							{box_left-4.f, bar_bt - 1.f},
							IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(tr * bar_alpha)));
					}
				}
			}
		}

		if (!corpse_mode && settings::visuals::health_text && g_fade.health_text > 0.f && entity.humanoid.address)
		{

			char buf[16];
			std::snprintf(buf, sizeof(buf), "%.0f", entity_hp);
			ImVec2 tsz = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, buf);
			auto& hc = settings::visuals::health_text_color;
			ImU32 col = helper::apply_opacity(
				ImGui::ColorConvertFloat4ToU32({hc[0],hc[1],hc[2],hc[3]}), g_fade.health_text);
			helper::draw_text_outlined(draw, font, font_size,
				{ box_left - 7.f - tsz.x * 0.5f, box_bot - tsz.y - 1.f }, col, buf);
		}

		if (!corpse_mode && settings::visuals::health_percent && g_fade.health_pct > 0.f && entity.humanoid.address)
		{
			float pct = hp_pct;
			int hc = (int)std::ceil(pct * 100.f);
			if (hc != (int)entity_maxhp)
			{
				char buf[8]; std::snprintf(buf, sizeof(buf), "%d", hc);
				ImVec2 tsz = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, buf);

				float bar_h  = box_h + 2.f;
				float bar_bt = std::min(dims.y, box_bot + 2.f);
				float fill   = std::min(bar_h * pct, std::max(0.f, bar_bt - std::max(0.f, box_top - 2.f)));
				float bar_tp = std::max(0.f, bar_bt - 1.f - fill);
				auto& hpc = settings::visuals::health_percent_color;
				ImU32 col = helper::apply_opacity(
					ImGui::ColorConvertFloat4ToU32({hpc[0],hpc[1],hpc[2],hpc[3]}),
					g_fade.health_pct);
				helper::draw_text_outlined(draw, font, font_size,
					{box_left - 7.f - tsz.x * 0.5f, bar_tp}, col, buf);
			}
		}

		bool want_name   = settings::visuals::name;
		bool want_avatar = settings::visuals::avatar;

		if (corpse_mode)
		{

			const char* corpse_name = "x_x";
			float tw = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, corpse_name).x;
			auto& nc = settings::visuals::corpse_color;
			ImU32 col = IM_COL32((int)(nc[0]*255.f),(int)(nc[1]*255.f),(int)(nc[2]*255.f),(int)(nc[3]*255.f));
			helper::draw_text_outlined(draw, font, font_size,
				{ box_cx - tw * 0.5f, box_top - font_size - 5.f }, col, corpse_name);
		}

		if (!corpse_mode && (want_name || want_avatar) && g_fade.name > 0.f)
		{
			std::string nm = (settings::visuals::name_display_type == 0 && !entity.display_name.empty())
				? entity.display_name : entity.name;
			if (nm.empty()) nm = "Unknown";

			float text_w  = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, nm.c_str()).x;
			float actual_w = text_w + (float)std::max(0,(int)nm.size()-1) * 1.f;
			float name_x  = box_cx - actual_w * 0.5f;
			float name_y  = box_top - font_size - 5.f;

			if (want_avatar)
			{

				float av_sz = font_size * 2.4f;
				float av_x  = name_x - av_sz - 5.f;
				float av_y  = name_y + (font_size - av_sz) * 0.5f;

				const ImU32 av_tint = helper::apply_opacity(IM_COL32(255, 255, 255, 255), g_fade.name);
				avatar::draw(draw, entity.user_id, {av_x, av_y}, {av_x + av_sz, av_y + av_sz}, av_tint);
			}

			if (want_name)
			{
				if (settings::visuals::blend)
				{
					ImU32 cs = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({
						settings::visuals::name_color_blend_start[0],
						settings::visuals::name_color_blend_start[1],
						settings::visuals::name_color_blend_start[2],
						settings::visuals::name_color_blend_start[3]}), g_fade.name);
					ImU32 ce = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({
						settings::visuals::name_color_blend_end[0],
						settings::visuals::name_color_blend_end[1],
						settings::visuals::name_color_blend_end[2],
						settings::visuals::name_color_blend_end[3]}), g_fade.name);
					helper::draw_text_blended(draw, font, font_size, {name_x, name_y}, cs, ce, nm.c_str());
				}
				else
				{
					auto& nc = settings::visuals::name_color;
					ImU32 col = helper::apply_opacity(
						ImGui::ColorConvertFloat4ToU32({nc[0],nc[1],nc[2],nc[3]}), g_fade.name);
					helper::draw_text_outlined(draw, font, font_size, {name_x, name_y}, col, nm.c_str());
				}

				if (settings::visuals::name_sheen && g_fade.name > 0.02f)
				{

					const float ph = sheen::phase_smooth(settings::visuals::name_sheen_speed,
						(float)(entity.instance.address & 0xFFu) * 0.017f, 0.28f);
					sheen::draw(draw, font, font_size, {name_x, name_y},
						IM_COL32(255, 255, 255, 0), nm.c_str(), ph,
						settings::visuals::name_sheen_width,
						settings::visuals::name_sheen_strength * g_fade.name,
						sheen::titanium(), 6);
				}
			}
		}

		float below_y = box_bot + 5.f;

		if (!corpse_mode && settings::visuals::distance && g_fade.distance > 0.f)
		{
			auto hrp_it = entity.parts.find("HumanoidRootPart");
			if (hrp_it != entity.parts.end() && hrp_it->second.address)
			{
				try {
					math::vector3 tp = hrp_it->second.get_primitive().get_position();
					math::vector3 d = {tp.x-local_pos.x, tp.y-local_pos.y, tp.z-local_pos.z};
					float dist = sqrtf(d.x*d.x + d.y*d.y + d.z*d.z);
					if (settings::visuals::distance_measurement == 1) dist *= 0.28f;
					char buf[32];
					std::snprintf(buf, sizeof(buf),
						settings::visuals::distance_measurement == 1 ? "%.1fm" : "%.1f", dist);
					float tw = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, buf).x;
					float aw = tw + (float)std::max(0,(int)std::strlen(buf)-1) * 1.f;
					auto& dc = settings::visuals::distance_color;
					ImU32 col = helper::apply_opacity(
						ImGui::ColorConvertFloat4ToU32({dc[0],dc[1],dc[2],dc[3]}), g_fade.distance);
					if (settings::visuals::blend) {
						ImU32 cs = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({settings::visuals::name_color_blend_start[0],settings::visuals::name_color_blend_start[1],settings::visuals::name_color_blend_start[2],settings::visuals::name_color_blend_start[3]}), g_fade.distance);
						ImU32 ce = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({settings::visuals::name_color_blend_end[0],settings::visuals::name_color_blend_end[1],settings::visuals::name_color_blend_end[2],settings::visuals::name_color_blend_end[3]}), g_fade.distance);
						helper::draw_text_blended(draw, font, font_size, {box_cx - aw*.5f, below_y}, cs, ce, buf);
					} else {
						helper::draw_text_outlined(draw, font, font_size, {box_cx - aw*.5f, below_y}, col, buf);
					}
					below_y += font_size + 2.f;
				} catch (...) {}
			}
		}

		if (!corpse_mode && settings::visuals::tool && g_fade.tool > 0.f)
		{
			std::string tn = entity.tool_name;
			if (tn.empty() && entity.instance.address) {
				try {

					bool is_player = rbx::instance_t(entity.instance.address).get_class_name() == "Player";
					if (is_player)
					{
						rbx::player_t pl(entity.instance.address);
						rbx::model_instance_t mdl = pl.get_model_instance();
						if (mdl.address) {
							rbx::instance_t ti = mdl.find_first_child_by_class("Tool");
							if (ti.address) tn = ti.get_name();
						}
					}
				} catch (...) {}
			}
			if (!tn.empty()) {
				float tw = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, tn.c_str()).x;
				float aw = tw + (float)std::max(0,(int)tn.size()-1) * 1.f;
				auto& tc = settings::visuals::tool_color;
				ImU32 col = helper::apply_opacity(
					ImGui::ColorConvertFloat4ToU32({tc[0],tc[1],tc[2],tc[3]}), g_fade.tool);
				if (settings::visuals::blend) {
					ImU32 cs = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({settings::visuals::name_color_blend_start[0],settings::visuals::name_color_blend_start[1],settings::visuals::name_color_blend_start[2],settings::visuals::name_color_blend_start[3]}), g_fade.tool);
					ImU32 ce = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({settings::visuals::name_color_blend_end[0],settings::visuals::name_color_blend_end[1],settings::visuals::name_color_blend_end[2],settings::visuals::name_color_blend_end[3]}), g_fade.tool);
					helper::draw_text_blended(draw, font, font_size, {box_cx - aw*.5f, below_y}, cs, ce, tn.c_str());
				} else {
					helper::draw_text_outlined(draw, font, font_size, {box_cx - aw*.5f, below_y}, col, tn.c_str());
				}
			}
		}

		if (!corpse_mode && entity.humanoid.address &&
			(settings::visuals::rigtype || settings::visuals::state) &&
			(g_fade.rig_state > 0.f || g_fade.state_tag > 0.f))
		{
			auto& fc = settings::visuals::flags_state_colour;

			auto tag = [&](const std::string& text, float alpha)
			{
				float tw = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, text.c_str()).x;
				float aw = tw + (float)std::max(0, (int)text.size() - 1) * 1.f;
				ImU32 col = helper::apply_opacity(
					ImGui::ColorConvertFloat4ToU32({fc[0], fc[1], fc[2], fc[3]}), alpha);
				helper::draw_text_outlined(draw, font, font_size,
					{box_cx - aw * 0.5f, below_y}, col, text.c_str());
				below_y += font_size + 2.f;
			};

			if (settings::visuals::rigtype && g_fade.rig_state > 0.f)
				tag("Rig  " + rig_word(entity), g_fade.rig_state);

			if (settings::visuals::state && g_fade.state_tag > 0.f)
				tag(std::string("State  ") + entity_state_word(entity), g_fade.state_tag);
		}

		if (!corpse_mode && settings::visuals::flags && g_fade.flags > 0.f && entity.humanoid.address)
		{
			try {

				const char* state_str = entity_state_word(entity);
				char fbuf[64];
				std::snprintf(fbuf, sizeof(fbuf), "[%s]", state_str);

				float flag_x = box_right + 5.f;
				auto& fc = settings::visuals::flags_state_colour;
				ImU32 col = helper::apply_opacity(
					ImGui::ColorConvertFloat4ToU32({fc[0],fc[1],fc[2],fc[3]}), g_fade.flags);
				if (settings::visuals::blend) {
					ImU32 cs = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({settings::visuals::name_color_blend_start[0],settings::visuals::name_color_blend_start[1],settings::visuals::name_color_blend_start[2],settings::visuals::name_color_blend_start[3]}), g_fade.flags);
					ImU32 ce = helper::apply_opacity(ImGui::ColorConvertFloat4ToU32({settings::visuals::name_color_blend_end[0],settings::visuals::name_color_blend_end[1],settings::visuals::name_color_blend_end[2],settings::visuals::name_color_blend_end[3]}), g_fade.flags);
					helper::draw_text_blended(draw, font, font_size, {flag_x, box_top}, cs, ce, fbuf);
				} else {
					helper::draw_text_outlined(draw, font, font_size, {flag_x, box_top}, col, fbuf);
				}
			} catch (...) {}
		}

		if (!corpse_mode && settings::visuals::movement_status && entity.humanoid.address)
		{

			std::string mtext = "unknown";
			try
			{
				const int st = entity.state;
				switch (st)
				{
				case 0: case 1: mtext = "ragdolled"; break;
				case 3: case 5: case 6: mtext = "in air"; break;
				case 4:  mtext = "swimming"; break;
				case 12: mtext = "climbing"; break;
				case 13: mtext = "sitting"; break;
				case 14: mtext = "platform standing"; break;
				default:
				{
					math::vector3 vel{};
					auto mvit = entity.parts.find("HumanoidRootPart");
					if (mvit != entity.parts.end() && mvit->second.address)
					{
						rbx::primitive_t mprim = mvit->second.get_primitive();
						if (mprim.address)
							vel = memory->read<math::vector3>(mprim.address + Offsets::Primitive::AssemblyLinearVelocity);
					}
					float lat = std::sqrt(vel.x*vel.x + vel.z*vel.z);
					mtext = lat > 0.5f ? "walking" : "idle";
				}
				break;
				}
			}
			catch (...) {}

			float tw = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, mtext.c_str()).x;
			float aw = tw + (float)std::max(0, (int)mtext.size() - 1) * 1.f;
			auto& mc = settings::visuals::flags_state_colour;
			ImU32 col = helper::apply_opacity(
				ImGui::ColorConvertFloat4ToU32({mc[0],mc[1],mc[2],mc[3]}), 1.f);
			helper::draw_text_outlined(draw, font, font_size, {box_cx - aw*0.5f, below_y}, col, mtext.c_str());
			below_y += font_size + 2.f;
		}

		bool do_skel     = settings::visuals::skeleton;
		bool do_hit_skel = !corpse_mode && settings::visuals::hit_skeleton && hit_flash > 0.f;
		if (do_skel || do_hit_skel)
		{
			std::unordered_map<std::string, ImVec2> bmap;

			const bool named_rig = entity.parts.count("UpperTorso") > 0 ||
			                       entity.parts.count("Torso") > 0;

			if (named_rig)
			{
				for (auto& [nm, part] : entity.parts)
				{
					if (!part.address) continue;
					rbx::primitive_t prim = part.get_primitive();
					if (!prim.address) continue;
					math::vector2 s{};
					if (game::visengine.world_to_screen(prim.get_position(), s, dims, view))
						bmap[nm] = {s.x, s.y};
				}
			}

			const BonePair* pairs = nullptr; int cnt = 0;
			std::vector<BonePair> generic;
			if (entity.parts.count("UpperTorso") > 0) { pairs = k_r15; cnt = (int)(sizeof(k_r15)/sizeof(k_r15[0])); }
			else if (named_rig) { pairs = k_r6; cnt = (int)(sizeof(k_r6)/sizeof(k_r6[0])); }
			else
			{

				cnt = build_generic_skeleton(entity, bmap, generic, dims, view);
				pairs = generic.empty() ? nullptr : generic.data();
			}

			if (pairs)
			{
				ImDrawListFlags old = draw->Flags;
				draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
				float thick = settings::visuals::skeleton_thickness;

				auto lerp_col = [](const float* base, const float* hit, float t) -> ImU32 {
					return IM_COL32(
						(int)((base[0]+(hit[0]-base[0])*t)*255.f),
						(int)((base[1]+(hit[1]-base[1])*t)*255.f),
						(int)((base[2]+(hit[2]-base[2])*t)*255.f),
						(int)((base[3]+(hit[3]-base[3])*t)*255.f));
				};

				static const float white[4]{1,1,1,1};
				const float* base_c = do_skel
					? (corpse_mode ? settings::visuals::corpse_color : settings::visuals::skeleton_color)
					: white;
				const float* hit_c  = settings::visuals::hit_skeleton_color;

				ImU32 line_col = do_hit_skel
					? lerp_col(do_skel ? base_c : hit_c, hit_c, hit_flash)
					: IM_COL32((int)(base_c[0]*255),(int)(base_c[1]*255),(int)(base_c[2]*255),(int)(base_c[3]*255));

				for (int i = 0; i < cnt; i++)
				{
					auto ia = bmap.find(pairs[i].a), ib = bmap.find(pairs[i].b);
					if (ia == bmap.end() || ib == bmap.end()) continue;
					if (do_skel && settings::visuals::skeleton_outline) {
						auto& oc = settings::visuals::skeleton_outline_color;
						draw->AddLine(ia->second, ib->second,
							IM_COL32((int)(oc[0]*255),(int)(oc[1]*255),(int)(oc[2]*255),255),
							thick + 2.f);
					}
					draw->AddLine(ia->second, ib->second, line_col,
						thick + (do_hit_skel ? hit_flash * 1.5f : 0.f));
				}
				draw->Flags = old;
			}
		}

		bool do_chams     = !corpse_mode && settings::visuals::chams && g_fade.chams > 0.f;
		bool do_hit_chams = !corpse_mode && settings::visuals::hit_chams && hit_flash > 0.f;
		if (do_chams || do_hit_chams)
		{
			auto blend_c4 = [&](const float* base, const float* hit, float t) -> ImU32 {
				return IM_COL32(
					(int)((base[0]+(hit[0]-base[0])*t)*255.f),
					(int)((base[1]+(hit[1]-base[1])*t)*255.f),
					(int)((base[2]+(hit[2]-base[2])*t)*255.f),
					(int)((base[3]+(hit[3]-base[3])*t)*255.f));
			};
			static const float white4[4]{1,1,1,0.3f};
			const float* fb = do_chams ? settings::visuals::chams_fill_color : white4;
			const float* hf = settings::visuals::hit_chams_color;

			const float cham_alpha = do_chams ? g_fade.chams : (do_hit_chams ? hit_flash : 0.f);
			const ImU32 fill_col = do_hit_chams
				? helper::apply_opacity(blend_c4(fb, hf, hit_flash), cham_alpha)
				: helper::color_with_opacity(fb[0],fb[1],fb[2],fb[3], g_fade.chams);

			const bool full_detail = have_centre
				? (centre - local_pos).length() < 120.f
				: true;

			mesh::chams_draw(draw, entity.parts, entity.instance.address,
				view, dims, fill_col, full_detail);
		}

		draw_part_chams(entity, view, dims, draw, highlight, hit_flash, corpse_mode);
	}

	if (settings::visuals::offscreen_arrows && g_fade.arrows > 0.f && !g_offscreen.empty())
	{
		const ImVec2 center(rbx::visualengine_t::window_offset_x() + dims.x * 0.5f,
		                    rbx::visualengine_t::window_offset_y() + dims.y * 0.5f);
		const float max_rx = std::max(40.f, dims.x * 0.5f - 18.f);
		const float max_ry = std::max(40.f, dims.y * 0.5f - 18.f);
		const float rmax   = std::max(max_rx, max_ry);
		float radius = std::max(40.f, settings::visuals::arrow_radius);
		if (radius > rmax) radius = rmax;
		const float size  = std::max(6.f, settings::visuals::arrow_size);
		ImFont* arrow_font = nullptr;
		float   arrow_fs   = 0.f;
		select_esp_font(arrow_font, arrow_fs);
		auto& ac = settings::visuals::arrow_color;
		const float alpha = std::clamp(ac[3] * g_fade.arrows, 0.f, 1.f);
		const ImU32 base_col = IM_COL32((int)(ac[0]*255.f),(int)(ac[1]*255.f),(int)(ac[2]*255.f),(int)(alpha*255.f));
		const auto tint_col = [&](const float* c) -> ImU32
		{
			return IM_COL32((int)(c[0]*255.f), (int)(c[1]*255.f), (int)(c[2]*255.f),
				(int)(alpha * std::clamp(c[3], 0.f, 1.f) * 255.f));
		};
		const ImU32 aim_col    = tint_col(settings::visuals::aim_target_color);
		const ImU32 friend_col = tint_col(settings::visuals::friend_color);

		for (auto& a : g_offscreen)
		{
			const float len = sqrtf(a.dx*a.dx + a.dy*a.dy);
			if (len < 1e-4f) continue;
			const float dx = a.dx / len, dy = a.dy / len;

			float tip_x = center.x + dx * radius;
			float tip_y = center.y + dy * radius;
			const float nx = (tip_x - center.x) / max_rx;
			const float ny = (tip_y - center.y) / max_ry;
			const float el = nx * nx + ny * ny;
			if (el > 1.f)
			{
				const float sc = 1.f / sqrtf(el);
				tip_x = center.x + (tip_x - center.x) * sc;
				tip_y = center.y + (tip_y - center.y) * sc;
			}

			const ImVec2 tip(tip_x, tip_y);
			const float px = -dy, py = dx;
			const ImVec2 pa(tip.x - dx * size + px * size * 0.55f, tip.y - dy * size + py * size * 0.55f);
			const ImVec2 pb(tip.x - dx * size - px * size * 0.55f, tip.y - dy * size - py * size * 0.55f);
			const ImU32 col = a.tint == arrow_aim ? aim_col
				: (a.tint == arrow_friend ? friend_col : base_col);
			draw->AddTriangleFilled(tip, pa, pb, col);
			draw->AddTriangle(tip, pa, pb, IM_COL32(0, 0, 0, 200), 1.0f);

			if (!a.info.empty())
			{
				const ImVec2 tsz = arrow_font->CalcTextSizeA(arrow_fs, FLT_MAX, 0.f, a.info.c_str());
				ImVec2 tp(tip.x + dx * (size * 0.35f + 4.f) - tsz.x * 0.5f,
					tip.y + dy * (size * 0.35f + 4.f) - tsz.y * 0.5f);
				tp.x = std::clamp(tp.x, 2.f, std::max(2.f, dims.x - tsz.x - 2.f));
				tp.y = std::clamp(tp.y, 2.f, std::max(2.f, dims.y - tsz.y - 2.f));
				helper::draw_text_outlined(draw, arrow_font, arrow_fs,
					{ std::floor(tp.x), std::floor(tp.y) }, col, a.info.c_str());
			}
		}
	}

	aimbot::draw_fov(draw, dims);
}

bool esp::screen_anchor(std::uint64_t instance, float& cx, float& top)
{
	if (!instance)
		return false;

	const float font = settings::visuals::esp_font_size > 4.f
		? (float)settings::visuals::esp_font_size
		: ImGui::GetFontSize();

	float clear = 0.f;
	if (settings::visuals::name)
		clear = font + 5.f;
	if (settings::visuals::avatar)
		clear = std::max(clear, font * 1.7f + 5.f);

	auto it = g_last_box.find(instance);
	if (it != g_last_box.end())
	{
		const std::uint64_t now = geometry_now_ms();
		if (now - it->second.ms <= k_box_hold_ms)
		{
			cx  = (it->second.left + it->second.right) * 0.5f;
			top = it->second.top - clear;
			return true;
		}
	}

	math::vector3 head{};
	math::vector3 head_size{};
	bool have = false;
	{
		std::lock_guard<std::mutex> lk(cache::mtx);
		for (const auto& e : cache::cached_players)
		{
			if (e.instance.address != instance)
				continue;
			auto pit = e.parts.find("Head");
			if (pit != e.parts.end() && pit->second.address)
			{
				try
				{
					rbx::primitive_t prim = pit->second.get_primitive();
					if (prim.address)
					{
						head      = prim.get_position();
						head_size = prim.get_size();
						have      = true;
					}
				}
				catch (...) {}
			}
			break;
		}
	}
	if (!have || !game::visengine.address)
		return false;

	const math::vector2 dims = game::visengine.get_dimensions();
	const math::matrix4 view = game::visengine.get_viewmatrix();

	math::vector2 sp{};
	if (!game::visengine.world_to_screen(head, sp, dims, view))
		return false;

	float top_y = sp.y - 34.f;
	if (std::isfinite(head_size.y) && head_size.y > 0.2f && head_size.y < 8.f)
	{
		math::vector2 head_top{};
		if (game::visengine.world_to_screen(
			{ head.x, head.y + head_size.y * 0.5f, head.z }, head_top, dims, view) &&
			std::isfinite(head_top.y))
		{
			top_y = head_top.y;
		}
	}

	cx  = sp.x;
	top = top_y - clear;
	return true;
}

bool esp::aim_anchor(std::uint64_t instance, float& cx, float& cy)
{
	if (!instance || !game::visengine.address)
		return false;

	static const char* const k_points[] = { "Head", "HitboxHead", "HumanoidRootPart" };

	math::vector3 world{};
	bool have = false;
	{
		std::lock_guard<std::mutex> lk(cache::mtx);
		for (const auto& e : cache::cached_players)
		{
			if (e.instance.address != instance)
				continue;

			for (const char* name : k_points)
			{
				auto pit = e.parts.find(name);
				if (pit == e.parts.end() || !pit->second.address)
					continue;

				try
				{
					rbx::primitive_t prim = pit->second.get_primitive();
					if (prim.address)
					{
						const math::vector3 p = prim.get_position();
						if (math::finite(p)) { world = p; have = true; }
					}
				}
				catch (...) {}

				if (have)
					break;
			}
			break;
		}
	}

	if (!have)
		return false;

	math::vector2 sp{};
	if (!game::visengine.world_to_screen(world, sp, game::visengine.get_dimensions(),
		game::visengine.get_viewmatrix()))
		return false;

	cx = sp.x;
	cy = sp.y;
	return true;
}
