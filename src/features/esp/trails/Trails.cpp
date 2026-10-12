#define NOMINMAX
#include "Trails.h"

#include <imgui/imgui_internal.h>

#include <Windows.h>

#include <settings.h>
#include <cache/cache.h>
#include <game/game.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#undef min
#undef max

namespace esp_trails
{
	namespace
	{

		struct pt_t
		{
			math::vector3 pos{};
			float         stamp{ 0.f };
		};

		std::mutex g_mtx;
		std::unordered_map<std::uint64_t, std::vector<pt_t>> g_hist;
		std::unordered_map<std::uint64_t, float>             g_last_add;
		std::unordered_map<std::uint64_t, math::vector3>     g_last_pos;
		std::uint64_t g_dm = 0;

		float now_s()
		{
			using namespace std::chrono;
			static const steady_clock::time_point t0 = steady_clock::now();
			return duration<float>(steady_clock::now() - t0).count();
		}

		bool plausible(const math::vector3& v)
		{
			return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
				std::fabs(v.x) < 1.0e6f && std::fabs(v.y) < 1.0e6f && std::fabs(v.z) < 1.0e6f;
		}

		bool sample_part(const cache::entity_t& e, const char* const* names, int count, math::vector3& out)
		{
			for (int i = 0; i < count; ++i)
			{
				auto it = e.parts.find(names[i]);
				if (it == e.parts.end() || !it->second.address)
					continue;
				try
				{
					rbx::primitive_t prim = it->second.get_primitive();
					if (!prim.address)
						continue;
					const math::vector3 p = prim.get_position();
					if (!plausible(p))
						continue;
					out = p;
					return true;
				}
				catch (...) {}
			}
			return false;
		}

		bool sample(const cache::entity_t& e, math::vector3& out)
		{
			static const char* const k_root[] = { "HumanoidRootPart", "UpperTorso", "Torso", "Head" };
			static const char* const k_head[] = { "Head", "HitboxHead" };
			static const char* const k_feet_l[] = { "LeftFoot", "Left Leg", "Foot", "LeftLeg" };
			static const char* const k_feet_r[] = { "RightFoot", "Right Leg", "RightLeg" };

			switch (settings::visuals::trail_follow)
			{
			case 1:
			{
				math::vector3 l{}, r{};
				const bool have_l = sample_part(e, k_feet_l, 4, l);
				const bool have_r = sample_part(e, k_feet_r, 3, r);
				if (have_l && have_r)
				{
					out = math::vector3((l.x + r.x) * 0.5f, (l.y + r.y) * 0.5f, (l.z + r.z) * 0.5f);
					return true;
				}
				out = have_l ? l : r;
				return have_l || have_r;
			}
			case 2:
				return sample_part(e, k_head, 2, out);
			default:
				return sample_part(e, k_root, 4, out);
			}
		}

		ImU32 pack_rgba(const float* c, float a)
		{
			const float k = ImClamp(a, 0.f, 1.f);
			return IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f), (int)(c[2] * 255.f),
				(int)(c[3] * 255.f * k));
		}

		ImU32 team_packed(std::uint32_t brick)
		{

			std::uint32_t h = brick ? brick : 1u;
			h ^= h >> 16; h *= 0x7feb352du;
			h ^= h >> 15; h *= 0x846ca68bu;
			h ^= h >> 16;
			const float hue = (float)(h % 1000u) / 1000.f;
			const ImVec4 c = ImColor::HSV(hue, 0.72f, 1.f);
			return IM_COL32((int)(c.x * 255.f), (int)(c.y * 255.f), (int)(c.z * 255.f), 235);
		}

		ImU32 mode_color(const cache::entity_t& e, float age01, float speed01)
		{
			ImU32 base;
			switch (settings::visuals::trail_color_mode)
			{
			case 1:
				base = team_packed(e.team_color);
				break;
			case 2:
			{
				const float hp = (e.max_health > 0.f) ? ImClamp(e.health / e.max_health, 0.f, 1.f) : 1.f;
				base = IM_COL32((int)((1.f - hp) * 255.f), (int)(hp * 255.f), 40, 255);
				break;
			}
			case 3:
			{

				const float hue = std::fmod((float)ImGui::GetTime() * 0.35f + age01 * 0.9f, 1.f);
				const ImVec4 c = ImColor::HSV(hue < 0.f ? hue + 1.f : hue, 0.70f, 1.f);
				base = IM_COL32((int)(c.x * 255.f), (int)(c.y * 255.f), (int)(c.z * 255.f), 255);
				break;
			}
			case 4:
			{

				const float t = ImClamp(speed01 * 1.6f, 0.f, 1.f);
				base = IM_COL32((int)((1.f - t) * 255.f), (int)(90.f + t * 140.f), (int)(60.f + t * 195.f), 255);
				break;
			}
			default:
				base = pack_rgba(settings::visuals::trail_color, 1.f);
				break;
			}
			return base;
		}

		inline ImU32 with_alpha(ImU32 c, float k)
		{
			const int a = (int)(((c >> IM_COL32_A_SHIFT) & 0xFF) * ImClamp(k, 0.f, 1.f) + 0.5f);
			return (c & ~IM_COL32_A_MASK) | ((ImU32)ImClamp(a, 0, 255) << IM_COL32_A_SHIFT);
		}

		math::vector3 catmull_rom(const math::vector3& p0, const math::vector3& p1,
			const math::vector3& p2, const math::vector3& p3, float t)
		{
			const float t2 = t * t;
			const float t3 = t2 * t;
			math::vector3 r;
			r.x = 0.5f * ((2.f * p1.x) + (-p0.x + p2.x) * t + (2.f * p0.x - 5.f * p1.x + 4.f * p2.x - p3.x) * t2 + (-p0.x + 3.f * p1.x - 3.f * p2.x + p3.x) * t3);
			r.y = 0.5f * ((2.f * p1.y) + (-p0.y + p2.y) * t + (2.f * p0.y - 5.f * p1.y + 4.f * p2.y - p3.y) * t2 + (-p0.y + 3.f * p1.y - 3.f * p2.y + p3.y) * t3);
			r.z = 0.5f * ((2.f * p1.z) + (-p0.z + p2.z) * t + (2.f * p0.z - 5.f * p1.z + 4.f * p2.z - p3.z) * t2 + (-p0.z + 3.f * p1.z - 3.f * p2.z + p3.z) * t3);
			return r;
		}

		struct vert_t
		{
			ImVec2 p{};
			float  life{ 1.f };
			float  scale{ 1.f };
			bool   ok{ false };
		};

		float pixel_scale(const math::vector3& world, const math::vector2& sp,
			const math::vector2& dims, const math::matrix4& view)
		{
			math::vector2 up{};
			if (!game::visengine.world_to_screen(
				math::vector3(world.x, world.y + 0.5f, world.z), up, dims, view))
				return 0.f;
			const float dx = up.x - sp.x, dy = up.y - sp.y;
			const float d = std::sqrt(dx * dx + dy * dy);
			return (d > 0.001f) ? d * 2.f : 0.f;
		}
	}

	int tracked()
	{
		std::lock_guard<std::mutex> lk(g_mtx);
		return (int)g_hist.size();
	}

	bool head_position(std::uint64_t instance, math::vector3& out)
	{
		std::lock_guard<std::mutex> lk(g_mtx);
		auto it = g_hist.find(instance);
		if (it == g_hist.end() || it->second.empty())
			return false;
		out = it->second.back().pos;
		return true;
	}

	void reset()
	{
		std::lock_guard<std::mutex> lk(g_mtx);
		g_hist.clear();
		g_last_add.clear();
		g_last_pos.clear();
	}

	void draw(ImDrawList* dl, const math::vector2& dims, const math::matrix4& view)
	{
		namespace tv = settings::visuals;

		static std::uint64_t s_dm = 0;
		if (game::datamodel.address && game::datamodel.address != s_dm && s_dm)
		{

			reset();
		}
		s_dm = game::datamodel.address;

		std::lock_guard<std::mutex> lk(g_mtx);

		if (!tv::trail)
		{

			g_hist.clear();
			g_last_add.clear();
			g_last_pos.clear();
			return;
		}
		if (!dl || !game::visengine.address)
			return;

		const float now = now_s();
		const float duration = ImMax(0.08f, tv::trail_duration);
		const float interval = ImMax(0.004f, tv::trail_update_interval);
		const int   max_points = ImClamp(tv::trail_max_points, 8, 512);

		std::vector<cache::entity_t> snap;
		math::vector3 local{};
		{

			std::lock_guard<std::mutex> cl(cache::mtx);
			snap = cache::cached_players;
			math::vector3 l{};
			auto hrp = cache::cached_local_player.parts.find("HumanoidRootPart");
			if (hrp != cache::cached_local_player.parts.end() && hrp->second.address)
			{
				try
				{
					rbx::primitive_t p = hrp->second.get_primitive();
					if (p.address) local = p.get_position();
				}
				catch (...) {}
			}
			(void)l;
		}

		for (const cache::entity_t& e : snap)
		{
			const std::uint64_t key = e.instance.address;
			if (!key || !e.humanoid.address)
				continue;

			if (settings::visuals::trail_team_check && cache::is_teammate(e))
			{
				g_hist.erase(key);
				continue;
			}

			math::vector3 pos{};
			if (!sample(e, pos))
				continue;

			std::vector<pt_t>& hist = g_hist[key];
			float& last_add = g_last_add[key];

			const float cut = now - duration;
			while (!hist.empty() && hist.front().stamp < cut)
				hist.erase(hist.begin());

			const bool moved = [&]
			{
				auto it = g_last_pos.find(key);
				if (it == g_last_pos.end())
					return true;
				const math::vector3 d = pos - it->second;
				return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z) >= tv::trail_min_movement;
			}();

			if (tv::trail_only_moving && hist.size() >= 2 && !moved)
			{

				hist.back().stamp = now;
				continue;
			}
			if (now - last_add < interval || !moved)
				continue;

			hist.push_back(pt_t{ pos, now });
			g_last_pos[key] = pos;
			last_add = now;

			while ((int)hist.size() > max_points)
				hist.erase(hist.begin());
		}

		if (g_hist.size() > 1)
		{
			for (auto it = g_hist.begin(); it != g_hist.end(); )
			{
				const bool alive = std::any_of(snap.begin(), snap.end(),
					[&](const cache::entity_t& e) { return e.instance.address == it->first; });
				if (!alive)
				{
					g_last_add.erase(it->first);
					g_last_pos.erase(it->first);
					it = g_hist.erase(it);
				}
				else ++it;
			}
		}

		const int style = ImClamp(tv::trail_style, 0, 4);
		const float glow_k = ImClamp(tv::trail_glow_intensity, 0.f, 2.f);
		const int   glow_layers = ImClamp(tv::trail_glow_layers, 0, 6);
		const float head_col[4] = { tv::trail_head_color[0], tv::trail_head_color[1],
			tv::trail_head_color[2], tv::trail_head_color[3] };

		ImDrawListFlags flags_backup = dl->Flags;
		dl->Flags |= ImDrawListFlags_AntiAliasedLines;

		for (const cache::entity_t& e : snap)
		{
			if (settings::visuals::trail_team_check && cache::is_teammate(e))
				continue;

			auto hit = g_hist.find(e.instance.address);
			if (hit == g_hist.end() || hit->second.size() < 2)
				continue;

			const std::vector<pt_t>& hist = hit->second;

			std::vector<math::vector3> curve;
			const bool spline = tv::trail_smooth && hist.size() >= 4;
			if (spline)
			{
				const int seg = ImClamp(tv::trail_spline_segments, 2, 16);
				curve.reserve((hist.size() - 1) * (size_t)seg + 1);
				for (size_t i = 0; i + 1 < hist.size(); ++i)
				{
					const math::vector3& p0 = (i == 0) ? hist[i].pos : hist[i - 1].pos;
					const math::vector3& p1 = hist[i].pos;
					const math::vector3& p2 = hist[i + 1].pos;
					const math::vector3& p3 = (i + 2 < hist.size()) ? hist[i + 2].pos : hist[i + 1].pos;
					for (int s = 0; s < seg; ++s)
						curve.push_back(catmull_rom(p0, p1, p2, p3, (float)s / (float)seg));
				}
				curve.push_back(hist.back().pos);
			}
			else
			{
				curve.reserve(hist.size());
				for (const pt_t& p : hist)
					curve.push_back(p.pos);
			}

			const size_t n = curve.size();
			std::vector<vert_t> vs(n);
			bool any = false;
			for (size_t i = 0; i < n; ++i)
			{
				vert_t& v = vs[i];

				size_t hi = spline ? (i / (size_t)ImClamp(tv::trail_spline_segments, 2, 16)) : i;
				if (hi >= hist.size()) hi = hist.size() - 1;
				const float age = now - hist[hi].stamp;
				v.life = ImClamp(1.f - age / duration, 0.f, 1.f);

				math::vector2 sp{};
				if (!game::visengine.world_to_screen(curve[i], sp, dims, view))
					continue;
				v.p = ImVec2(sp.x, sp.y);
				v.scale = pixel_scale(curve[i], sp, dims, view);
				v.ok = std::isfinite(v.p.x) && std::isfinite(v.p.y) &&
					std::fabs(v.p.x) < 20000.f && std::fabs(v.p.y) < 20000.f;
				any = any || v.ok;
			}
			if (!any)
				continue;

			float speed01 = 0.f;
			if (hist.size() >= 2)
			{
				const math::vector3 d = hist.back().pos - hist[hist.size() - 2].pos;
				const float dtp = ImMax(0.001f, hist.back().stamp - hist[hist.size() - 2].stamp);
				speed01 = ImClamp(std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z) / dtp / 60.f, 0.f, 1.f);
			}

			const ImU32 head = pack_rgba(head_col, 1.f);

			const float fade_at = tv::trail_fade ? ImClamp(tv::trail_fade_start, 0.f, 1.f) : 0.f;
			auto alpha_of = [&](float life) -> float
			{
				if (!tv::trail_fade) return life > 0.f ? 1.f : 0.f;
				if (life >= fade_at) return 1.f;
				const float k = (fade_at > 0.001f) ? life / fade_at : 0.f;
				return k * k * (3.f - 2.f * k);
			};

			std::vector<ImVec2> off(n);
			std::vector<float>  half(n), wid(n);
			for (size_t i = 0; i < n; ++i)
			{
				if (!vs[i].ok)
				{
					off[i] = ImVec2(0.f, 0.f);
					half[i] = 0.f;
					wid[i] = 0.f;
					continue;
				}

				const ImVec2 a = vs[i > 0 && vs[i - 1].ok ? i - 1 : i].p;
				const ImVec2 b = vs[(i + 1 < n && vs[i + 1].ok) ? i + 1 : i].p;
				float dx = b.x - a.x, dy = b.y - a.y;
				const float len = std::sqrt(dx * dx + dy * dy);
				if (len < 0.0001f) { dx = 1.f; dy = 0.f; }
				else { dx /= len; dy /= len; }
				off[i] = ImVec2(-dy, dx);

				const float taper = tv::trail_taper ? powf(ImClamp(vs[i].life, 0.f, 1.f), 0.75f) : 1.f;
				const float px = ImClamp(vs[i].scale * tv::trail_thickness * 0.085f, 0.55f, 30.f);
				half[i] = px * taper;
				wid[i] = px;
			}

			const ImU32 base = mode_color(e, 0.f, speed01);
			const bool per_segment_hue = (tv::trail_color_mode == 3);

			auto strip = [&](size_t i, float width_scale, float alpha_mul, ImU32 col)
			{
				if (!vs[i].ok || !vs[i + 1].ok)
					return;
				const float a0 = alpha_of(vs[i].life) * alpha_mul;
				const float a1 = alpha_of(vs[i + 1].life) * alpha_mul;
				if (a0 <= 0.004f && a1 <= 0.004f)
					return;
				const float h0 = half[i] * width_scale;
				const float h1 = half[i + 1] * width_scale;
				if (h0 <= 0.05f && h1 <= 0.05f)
					return;

				const ImVec2 p0(vs[i].p.x + off[i].x * h0, vs[i].p.y + off[i].y * h0);
				const ImVec2 p1(vs[i].p.x - off[i].x * h0, vs[i].p.y - off[i].y * h0);
				const ImVec2 p2(vs[i + 1].p.x - off[i + 1].x * h1, vs[i + 1].p.y - off[i + 1].y * h1);
				const ImVec2 p3(vs[i + 1].p.x + off[i + 1].x * h1, vs[i + 1].p.y + off[i + 1].y * h1);

				dl->AddQuadFilled(p0, p1, p2, p3, with_alpha(col, (a0 + a1) * 0.5f));
			};

			const float kMaxJump = std::sqrt(dims.x * dims.x + dims.y * dims.y) * 1.35f;
			auto sane = [&](size_t i) -> bool
			{
				if (!vs[i].ok || !vs[i + 1].ok)
					return false;
				const float dx = vs[i + 1].p.x - vs[i].p.x;
				const float dy = vs[i + 1].p.y - vs[i].p.y;
				return (dx * dx + dy * dy) <= kMaxJump * kMaxJump;
			};

			switch (style)
			{
			case 1:
				for (size_t i = 0; i + 1 < n; ++i)
				{
					if (!sane(i))
						continue;
					const float a = alpha_of(vs[i].life);
					if (a <= 0.004f)
						continue;
					const ImU32 c = per_segment_hue ? mode_color(e, 1.f - vs[i].life, speed01) : base;
					dl->AddLine(vs[i].p, vs[i + 1].p, with_alpha(c, a), ImMax(0.6f, wid[i] * 0.45f));
				}
				break;

			case 4:
				for (size_t i = 0; i + 1 < n; ++i)
				{
					if (!sane(i))
						continue;
					const float a = alpha_of(vs[i].life);
					if (a <= 0.004f)
						continue;
					const ImU32 c = per_segment_hue ? mode_color(e, 1.f - vs[i].life, speed01) : base;
					const float w = ImMax(0.7f, wid[i] * 0.5f);
					const ImVec2 o0(off[i].x * w * 0.55f, off[i].y * w * 0.55f);
					const ImVec2 o1(off[i + 1].x * w * 0.55f, off[i + 1].y * w * 0.55f);
					dl->AddLine(ImVec2(vs[i].p.x + o0.x, vs[i].p.y + o0.y),
						ImVec2(vs[i + 1].p.x + o1.x, vs[i + 1].p.y + o1.y),
						with_alpha(c, a * 0.85f), w);
					dl->AddLine(ImVec2(vs[i].p.x - o0.x, vs[i].p.y - o0.y),
						ImVec2(vs[i + 1].p.x - o1.x, vs[i + 1].p.y - o1.y),
						with_alpha(c, a * 0.85f), w);
					dl->AddLine(vs[i].p, vs[i + 1].p, with_alpha(c, a * 0.35f), w * 0.65f);
				}
				break;

			default:
			{
				const float taper_power = (style == 2) ? 1.45f : 1.f;

				if (tv::trail_glow && glow_layers > 0)
				{
					for (int g = glow_layers; g >= 1; --g)
					{
						const float ws = 1.f + (float)g * 0.85f;
						const float am = (glow_k * 0.42f) / (float)g;
						for (size_t i = 0; i + 1 < n; ++i)
						{
							if (!sane(i))
								continue;
							strip(i, ws, am, base);
						}
					}
				}

				for (size_t i = 0; i + 1 < n; ++i)
				{
					if (!sane(i))
						continue;
					const ImU32 c = per_segment_hue ? mode_color(e, 1.f - vs[i].life, speed01) : base;
					if (style == 2)
					{

						for (int k = 0; k < 2; ++k)
							strip(i, k == 0 ? 1.f : 0.45f, k == 0 ? 1.f : 0.75f, c);
					}
					else
					{
						strip(i, 1.f, 1.f, c);
					}
					if (style == 3)
					{

						const float a = alpha_of(vs[i].life);
						if (a > 0.02f)
							dl->AddLine(vs[i].p, vs[i + 1].p, with_alpha(head, a * 0.55f),
								ImMax(0.6f, wid[i] * 0.22f));
					}
				}
				(void)taper_power;
				break;
			}
			}

			const size_t h = n - 1;
			if (vs[h].ok && vs[h].life > 0.02f)
			{
				const float a = alpha_of(vs[h].life);
				const float r = ImClamp(vs[h].scale * 0.045f, 1.2f, 7.f);
				if (tv::trail_height_fall)
				{
					const float drop = ImClamp(vs[h].scale * ImMax(0.f, tv::trail_height_drop), 0.f, 60.f);
					if (drop > 0.6f)
						dl->AddLine(vs[h].p, ImVec2(vs[h].p.x, vs[h].p.y + drop),
							with_alpha(base, a * 0.32f), ImMax(0.6f, r * 0.5f));
				}
				if (tv::trail_head_spark)
				{
					dl->AddCircleFilled(vs[h].p, r * 1.9f, with_alpha(base, a * 0.20f), 18);
					dl->AddCircleFilled(vs[h].p, r, with_alpha(head, a * 0.95f), 18);
				}
			}
		}

		dl->Flags = flags_backup;
	}
}
