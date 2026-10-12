#define NOMINMAX
#include "Sonar.h"

#include <imgui/imgui_internal.h>

#include <Windows.h>

#include <settings.h>
#include <cache/cache.h>
#include <game/game.h>
#include <features/worldgeo/WorldGeo.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>

#undef min
#undef max

namespace sonar
{
	namespace
	{

		struct ping_t
		{
			ImVec2 pos{};
			float  age{ 0.f };
			float  life{ 1.1f };
			float  radius{ 22.f };
			float  dist{ 0.f };
			ImVec2 feet{};
			bool   have_feet{ false };
			bool   visible{ true };
			char   text[96]{};
		};

		struct sweep_t
		{
			float angle{ 0.f };
			float last{ 0.f };
			float glow{ 0.f };
			float ring_r{ 0.f };
		};

		std::vector<ping_t> g_pings;
		sweep_t             g_sweep;
		std::uint64_t       g_latched = 0;
		float               g_latched_until = 0.f;
		bool                g_toggle_on = false;
		bool                g_toggle_was_down = false;
		std::uint64_t       g_dm = 0;

		float clock()
		{
			using namespace std::chrono;
			static const steady_clock::time_point t0 = steady_clock::now();
			return duration<float>(steady_clock::now() - t0).count();
		}

		bool key_down(int vk)
		{
			return vk != 0 && (GetAsyncKeyState(vk) & 0x8000) != 0;
		}

		bool resolve_active()
		{
			namespace sc = settings::sonar;
			switch (sc::activation_mode)
			{
			case 0:
			{
				const bool down = key_down(sc::keybind);
				if (down && !g_toggle_was_down)
					g_toggle_on = !g_toggle_on;
				g_toggle_was_down = down;
				return g_toggle_on;
			}
			case 2:
				return true;
			default:
				return key_down(sc::keybind);
			}
		}

		ImU32 pack(const float* c, float a)
		{
			const float k = ImClamp(a, 0.f, 1.f);
			return IM_COL32((int)(c[0] * 255.f), (int)(c[1] * 255.f), (int)(c[2] * 255.f),
				(int)(c[3] * 255.f * k));
		}

		ImU32 with_alpha(ImU32 c, float k)
		{
			const int a = (int)(((c >> IM_COL32_A_SHIFT) & 0xFF) * ImClamp(k, 0.f, 1.f) + 0.5f);
			return (c & ~IM_COL32_A_MASK) | ((ImU32)ImClamp(a, 0, 255) << IM_COL32_A_SHIFT);
		}

		ImU32 hsv_u32(float hue, float sat, float val, float alpha)
		{
			hue -= floorf(hue);
			const ImVec4 c = ImColor::HSV(hue, ImClamp(sat, 0.f, 1.f), ImClamp(val, 0.f, 1.f));
			return IM_COL32((int)(c.x * 255.f + 0.5f), (int)(c.y * 255.f + 0.5f), (int)(c.z * 255.f + 0.5f),
				(int)(ImClamp(alpha, 0.f, 1.f) * 255.f + 0.5f));
		}

		ImU32 heat_u32(float t, float alpha)
		{
			return hsv_u32(0.02f + (1.f - ImClamp(t, 0.f, 1.f)) * 0.52f, 0.70f, 1.f, alpha);
		}

		void beam_wedge(ImDrawList* dl, ImVec2 hub, float ang, float len,
			float w0, float w1, ImU32 col)
		{
			if (len <= 0.5f || (w0 <= 0.05f && w1 <= 0.05f))
				return;
			const float cs = cosf(ang), sn = sinf(ang);
			const ImVec2 n(-sn, cs);
			const ImVec2 b(hub.x + cs * len, hub.y + sn * len);
			dl->AddQuadFilled(ImVec2(hub.x + n.x * w0, hub.y + n.y * w0),
				ImVec2(b.x + n.x * w1, b.y + n.y * w1),
				ImVec2(b.x - n.x * w1, b.y - n.y * w1),
				ImVec2(hub.x - n.x * w0, hub.y - n.y * w0), col);
		}

		std::string short_name(const cache::entity_t& e)
		{
			std::string n = e.display_name.empty() ? e.name : e.display_name;
			if (n.size() > 18)
				n.resize(18);
			return n;
		}
	}

	bool active()
	{
		if (!settings::sonar::enabled)
			return false;
		return resolve_active();
	}

	std::uint64_t latched()
	{
		return g_latched;
	}

	snapshot_t snapshot()
	{
		snapshot_t s{};
		s.active    = active();
		s.sweep_deg = g_sweep.angle;
		s.glow      = g_sweep.glow;

		const ImVec2 ds = ImGui::GetIO().DisplaySize;
		const ImVec2 mid(ds.x * 0.5f, ds.y * 0.5f);

		float best = -1.f;
		for (const ping_t& p : g_pings)
		{
			if (p.age >= p.life)
				continue;
			const float fresh = 1.f - (p.age / ImMax(0.001f, p.life));
			if (fresh <= best)
				continue;
			best = fresh;
			s.has_ping = true;
			s.distance = p.dist;
			snprintf(s.name, sizeof s.name, "%s", p.text);
			s.bearing_deg = atan2f(p.pos.y - mid.y, p.pos.x - mid.x) * 57.29578f;
		}
		return s;
	}

	void reset()
	{
		g_pings.clear();
		g_latched = 0;
		g_latched_until = 0.f;
		g_sweep = sweep_t{};
	}

	void draw(ImDrawList* dl, const math::vector2& dims, const math::matrix4& view)
	{
		namespace sc = settings::sonar;

		static std::uint64_t s_dm = 0;
		if (game::datamodel.address && game::datamodel.address != s_dm && s_dm)
			reset();
		s_dm = game::datamodel.address;

		if (g_latched && clock() > g_latched_until)
			g_latched = 0;

		const float now = clock();
		for (auto it = g_pings.begin(); it != g_pings.end(); )
			it = ((it->age += ImMin(0.05f, ImMax(0.f, now - g_sweep.last))) >= it->life) ? g_pings.erase(it) : it + 1;

		const bool on = settings::sonar::enabled &&
			(!sc::hide_with_menu || !ImGui::GetIO().WantTextInput) && dl && game::visengine.address;
		const bool running = on && resolve_active();
		if (!running)
		{
			g_sweep.last = now;
			return;
		}

		float dt = now - g_sweep.last;
		if (!(dt > 0.f) || dt > 0.25f)
			dt = 1.f / 60.f;
		g_sweep.last = now;

		const int style = ImClamp(sc::style, 0, 3);
		const bool backwards = sc::sweep_speed < 0.f;
		const float speed = ImMax(6.f, std::fabs(sc::sweep_speed));

		if (style == 0 || style == 3)
		{
			float a = g_sweep.angle + (backwards ? -speed : speed) * dt;
			if (sc::sweep_back)
			{

				if (a > 360.f) a = 720.f - a;
				if (a < 0.f)   a = -a;
				a = std::fmod(a, 720.f);
				if (a > 360.f) a = 720.f - a;
			}
			else
			{
				a = std::fmod(a, 360.f);
				if (a < 0.f) a += 360.f;
			}
			g_sweep.angle = a;
		}
		else if (style == 2)
		{
			g_sweep.ring_r += speed * 3.2f * dt;
		}
		else
		{

			POINT pt{};
			if (GetCursorPos(&pt))
			{
				const ImVec2 d(dims.x * 0.5f, dims.y * 0.5f);
				const float dx = (float)pt.x - d.x, dy = (float)pt.y - d.y;
				g_sweep.angle = std::fmod(std::atan2(dy, dx) * 57.29578f, 360.f);
				if (g_sweep.angle < 0.f) g_sweep.angle += 360.f;
			}
		}

		g_sweep.glow = ImMax(0.f, g_sweep.glow - dt * 2.2f);

		std::vector<cache::entity_t> snap;
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			snap = cache::cached_players;
		}

		const ImVec2 centre(dims.x * 0.5f, dims.y * 0.5f);
		const float range = ImMax(20.f, sc::range);
		const float half_angle = 3.2f;
		const float sweep_rad = g_sweep.angle * 0.0174533f;
		const float sweep_x = cosf(sweep_rad), sweep_y = sinf(sweep_rad);

		const math::vector3 eye = worldgeo::eye_position(math::vector3{});

		for (const cache::entity_t& e : snap)
		{
			if (!e.instance.address || !e.humanoid.address)
				continue;

			math::vector3 world{};
			bool have = false;
			static const char* const k_points[] = { "Head", "HitboxHead", "HumanoidRootPart" };
			for (const char* n : k_points)
			{
				auto it = e.parts.find(n);
				if (it == e.parts.end() || !it->second.address)
					continue;
				try
				{
					rbx::primitive_t prim = it->second.get_primitive();
					if (!prim.address)
						continue;
					world = prim.get_position();
					have = true;
					break;
				}
				catch (...) {}
			}
			if (!have)
				continue;

			math::vector2 sp{};
			if (!game::visengine.world_to_screen(world, sp, dims, view))
				continue;
			if (sp.x < -dims.x || sp.y < -dims.y || sp.x > dims.x * 2.f || sp.y > dims.y * 2.f)
				continue;

			math::vector3 d{ world.x - eye.x, world.y - eye.y, world.z - eye.z };
			const float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
			if (dist > range)
				continue;

			float err = 0.f;
			if (style == 2)
			{
				const float dx = sp.x - centre.x, dy = sp.y - centre.y;
				err = std::fabs(std::sqrt(dx * dx + dy * dy) - g_sweep.ring_r);
				if (err > 18.f)
					continue;
			}
			else
			{
				const float dx = sp.x - centre.x, dy = sp.y - centre.y;
				const float len = std::sqrt(dx * dx + dy * dy);
				if (len < 6.f)
					continue;
				const float ux = dx / len, uy = dy / len;

				const float cross = sweep_x * uy - sweep_y * ux;
				const float dot = sweep_x * ux + sweep_y * uy;
				if (dot <= 0.f)
					continue;
				err = std::fabs(asinf(ImClamp(cross, -1.f, 1.f))) * 57.29578f;
				if (err > half_angle)
					continue;
			}

			const bool visible = worldgeo::is_visible(eye, world);

			if (sc::los_only && !visible)
				continue;

			ImVec2 feet{};
			bool have_feet = false;
			{
				auto rit = e.parts.find("HumanoidRootPart");
				if (rit != e.parts.end() && rit->second.address)
				{
					try
					{
						rbx::primitive_t prim = rit->second.get_primitive();
						if (prim.address)
						{
							math::vector3 foot = prim.get_position();
							foot.y -= 3.f;
							math::vector2 fp{};
							if (game::visengine.world_to_screen(foot, fp, dims, view))
							{
								feet = ImVec2(fp.x, fp.y);
								have_feet = true;
							}
						}
					}
					catch (...) {}
				}
			}

			bool existing = false;
			for (ping_t& p : g_pings)
			{
				const float px = p.pos.x - sp.x, py = p.pos.y - sp.y;
				if (px * px + py * py < 36.f)
				{
					p.age = 0.f;
					p.visible = visible;
					existing = true;
					break;
				}
			}
			if (!existing)
			{
				ping_t p;
				p.pos = ImVec2(sp.x, sp.y);
				p.life = ImMax(0.2f, sc::ping_life);
				p.radius = ImClamp(sc::ping_radius, 6.f, 120.f);
				p.dist = dist;
				p.feet = feet;
				p.have_feet = have_feet;
				p.visible = visible;

				std::string label;
				if (sc::info_mode == 1)
					label = short_name(e);
				else if (sc::info_mode == 2)
				{
					char buf[96];
					std::snprintf(buf, sizeof buf, "%s  %.0f",
						short_name(e).c_str(),
						settings::visuals::distance_unit == 1 ? dist * 0.28f : dist);
					label = buf;
				}
				else if (sc::info_mode == 3)
				{
					char buf[96];
					std::snprintf(buf, sizeof buf, "%s  %.0f  %.0f hp",
						short_name(e).c_str(),
						settings::visuals::distance_unit == 1 ? dist * 0.28f : dist,
						ImMax(0.f, e.health));
					label = buf;
				}
				std::snprintf(p.text, sizeof p.text, "%s", label.c_str());

				g_pings.push_back(p);
				if (g_pings.size() > 24)
					g_pings.erase(g_pings.begin());

				g_sweep.glow = 1.f;
				if (sc::latch && e.instance.address)
				{
					g_latched = e.instance.address;
					g_latched_until = now + ImMax(0.2f, sc::hold_time);
				}
			}
		}

		const float beam_alpha = ImClamp(sc::color[3], 0.f, 1.f);
		const float ping_alpha = ImClamp(sc::ping_color[3], 0.f, 1.f);

		ImU32 beam = pack(sc::color, 1.f);
		ImU32 hot = pack(sc::ping_color, 1.f);
		if (sc::color_mode == 1)
		{
			const float hue = now * ImMax(0.005f, sc::hue_speed);
			beam = hsv_u32(hue, 0.55f, 1.f, beam_alpha);
			hot = hsv_u32(hue + 0.5f, 0.70f, 1.f, ping_alpha);
		}

		const float k = ImClamp(sc::glow_strength, 0.f, 1.f);
		const float width = ImMax(0.4f, sc::beam_width);
		const float taper = ImClamp(sc::beam_taper, 0.f, 0.95f);
		const float reach = std::sqrt(dims.x * dims.x + dims.y * dims.y) * 0.5f
			* ImClamp(sc::beam_reach, 0.2f, 2.5f);

		ImFont* font = ImGui::GetFont();
		const float font_size = ImClamp(sc::info_size, 8.f, 26.f);

		if (sc::grid)
		{
			const float ga = ImClamp(sc::grid_alpha, 0.f, 1.f) * (0.45f + 0.55f * k);
			const int rings = ImClamp(sc::grid_rings, 0, 8);
			for (int i = 1; i <= rings; ++i)
			{
				const float r = reach * (float)i / (float)rings;
				dl->AddCircle(centre, r, with_alpha(beam, ga * 0.42f), 128, 1.f);
				if (sc::grid_labels && font)
				{
					char buf[32];
					const float studs = range * ((float)i / (float)rings);
					std::snprintf(buf, sizeof buf, "%.0f",
						settings::visuals::distance_unit == 1 ? studs * 0.28f : studs);
					dl->AddText(font, 11.f,
						ImVec2(centre.x + r * 0.7071f + 4.f, centre.y + r * 0.7071f - 14.f),
						with_alpha(beam, ga * 0.75f), buf);
				}
			}

			if (rings > 0)
			{
				const float inner = reach * 0.14f;
				const float outer = reach * 0.20f;
				for (int i = 0; i < 4; ++i)
				{
					const float a = (float)i * 1.5707963f;
					const ImVec2 o(cosf(a), sinf(a));
					dl->AddLine(ImVec2(centre.x + o.x * inner, centre.y + o.y * inner),
						ImVec2(centre.x + o.x * outer, centre.y + o.y * outer),
						with_alpha(beam, ga * 0.55f), 1.f);
				}
			}
		}

		if (style == 0 || style == 3)
		{
			const bool cone = (style == 3);
			const float len = cone ? reach * 0.72f : reach * 1.05f;
			const float tail_deg = ImClamp(sc::tail_length, 0.f, 180.f);
			const float tail_str = ImClamp(sc::tail_strength, 0.f, 1.5f);

			{
				const int steps = (int)ImClamp(tail_deg / 2.2f, 0.f, 40.f);
				for (int i = steps; i >= 0; --i)
				{
					const float f = (float)i / ImMax(1.f, (float)steps);
					const float back = (float)i * (tail_deg / ImMax(1.f, (float)steps));
					const float ang = (g_sweep.angle - back) * 0.0174533f;
					const float a = (1.f - f) * tail_str * 0.55f;
					if (a <= 0.01f)
						continue;
					const float wid = (0.35f + 0.65f * (1.f - f));
					beam_wedge(dl, centre, ang, len, width * 0.5f * wid,
						width * 0.5f * wid * (1.f - taper),
						with_alpha(beam, a * (0.35f + 0.65f * k)));
				}
			}

			if (sc::glow)
			{
				const float g = k * (0.35f + 0.65f * g_sweep.glow);
				const float ang = g_sweep.angle * 0.0174533f;
				for (int i = 4; i >= 1; --i)
				{
					const float w = width * 0.5f + (float)i * 1.9f;
					beam_wedge(dl, centre, ang, len, w, w * (1.f - taper),
						with_alpha(beam, 0.10f * g / (float)i));
				}
			}

			beam_wedge(dl, centre, g_sweep.angle * 0.0174533f, len,
				width * 0.5f, width * 0.5f * (1.f - taper),
				with_alpha(beam, (cone ? 0.28f : 0.55f) * (0.55f + 0.45f * k)));

			dl->AddCircleFilled(centre, 3.4f, with_alpha(hot, 0.35f + 0.45f * g_sweep.glow), 20);
			dl->AddCircle(centre, 7.f, with_alpha(beam, 0.30f + 0.40f * g_sweep.glow), 28, 1.2f);
		}
		else if (style == 2)
		{
			const float r = g_sweep.ring_r;
			if (r > reach * 1.3f)
				g_sweep.ring_r = 0.f;
			if (r > 1.f)
			{
				dl->AddCircle(centre, r, with_alpha(beam, 0.42f * (0.4f + 0.6f * k)), 96, width);
				dl->AddCircle(centre, r * 0.86f, with_alpha(beam, 0.14f * k), 96, width * 2.6f);
			}
		}
		else
		{

			const float ang = g_sweep.angle * 0.0174533f;
			const ImVec2 tip(centre.x + cosf(ang) * reach * 0.34f, centre.y + sinf(ang) * reach * 0.34f);
			beam_wedge(dl, centre, ang, reach * 0.34f, width * 0.5f, width * 0.5f * (1.f - taper),
				with_alpha(beam, 0.50f * (0.4f + 0.6f * k)));

			const float br = 10.f + 3.f * sinf(now * 3.4f);
			dl->AddCircle(tip, br, with_alpha(hot, 0.55f * (0.5f + 0.5f * g_sweep.glow)), 32, 1.3f);
			for (int i = 0; i < 4; ++i)
			{
				const float a = (float)i * 1.5708f;
				const ImVec2 o(cosf(a), sinf(a));
				dl->AddLine(ImVec2(tip.x + o.x * br, tip.y + o.y * br),
					ImVec2(tip.x + o.x * (br + 6.f), tip.y + o.y * (br + 6.f)),
					with_alpha(beam, 0.45f), 1.4f);
			}
		}

		const int mark = ImClamp(sc::ping_style, 0, 3);

		for (const ping_t& p : g_pings)
		{
			const float t = ImClamp(p.age / ImMax(0.05f, p.life), 0.f, 1.f);
			const float fade = (1.f - t) * (1.f - t);

			const ImU32 col = (sc::color_mode == 2)
				? heat_u32(p.dist / ImMax(1.f, range), 1.f)
				: (p.visible ? hot : with_alpha(beam, 0.6f));
			const float a = fade * (p.visible ? 1.f : 0.45f);

			if (sc::ping_drop && p.have_feet)
			{
				const float dx = p.feet.x - p.pos.x, dy = p.feet.y - p.pos.y;
				if (dx * dx + dy * dy > 16.f)
					dl->AddLine(p.pos, p.feet, with_alpha(col, 0.32f * a), 1.f);
			}

			if (sc::ping_rings)
			{
				if (mark == 1)
				{
					const float r = p.radius * (1.35f - t * 0.85f);
					const float arm = ImMax(3.f, r * 0.45f);
					for (int i = 0; i < 4; ++i)
					{
						const float sx = (i & 1) ? 1.f : -1.f;
						const float sy = (i & 2) ? 1.f : -1.f;
						const ImVec2 c2(p.pos.x + sx * r, p.pos.y + sy * r);
						dl->AddLine(c2, ImVec2(c2.x - sx * arm, c2.y), with_alpha(col, 0.80f * a), 1.8f);
						dl->AddLine(c2, ImVec2(c2.x, c2.y - sy * arm), with_alpha(col, 0.80f * a), 1.8f);
					}
				}
				else if (mark == 2)
				{
					const float r = p.radius * (0.45f + 0.55f * t);
					const float rot = now * 1.4f + (float)(int)p.pos.x * 0.03f;
					ImVec2 pts[4];
					for (int i = 0; i < 4; ++i)
					{
						const float ang = rot + (float)i * 1.5707963f;
						pts[i] = ImVec2(p.pos.x + cosf(ang) * r, p.pos.y + sinf(ang) * r);
					}
					for (int i = 0; i < 4; ++i)
						dl->AddLine(pts[i], pts[(i + 1) & 3], with_alpha(col, 0.75f * a), 1.6f);
					dl->AddCircleFilled(p.pos, 2.2f + 2.6f * (1.f - t), with_alpha(col, 0.85f * a), 14);
				}
				else if (mark == 3)
				{
					const float h = p.radius * (0.7f + 0.6f * t);
					dl->AddLine(ImVec2(p.pos.x, p.pos.y + 6.f), ImVec2(p.pos.x, p.pos.y + 6.f + h),
						with_alpha(col, 0.65f * a), 1.6f);
					dl->AddCircleFilled(p.pos, 3.f + 2.f * (1.f - t), with_alpha(col, 0.90f * a), 16);
					dl->AddCircle(p.pos, p.radius * (0.3f + 0.7f * t), with_alpha(col, 0.35f * a), 32, 1.f);
				}
				else
				{
					const float r = p.radius * (0.35f + t * 1.35f);
					dl->AddCircle(p.pos, r, with_alpha(col, 0.60f * a), 40, 1.8f);
					dl->AddCircle(p.pos, r * 0.62f, with_alpha(col, 0.30f * a), 32, 1.1f);
					if (sc::ping_ticks)
					{

						for (int i = 0; i < 4; ++i)
						{
							const float ang = (float)i * 1.5708f + now * 0.9f;
							const ImVec2 o(cosf(ang), sinf(ang));
							dl->AddLine(ImVec2(p.pos.x + o.x * r, p.pos.y + o.y * r),
								ImVec2(p.pos.x + o.x * (r + 5.f), p.pos.y + o.y * (r + 5.f)),
								with_alpha(col, 0.55f * a), 1.4f);
						}
					}
					dl->AddCircleFilled(p.pos, 2.4f * (1.f - t * 0.4f), with_alpha(col, 0.9f * a), 12);
				}
			}

			if (sc::ping_spark && p.age < 0.16f)
			{
				const float st = p.age / 0.16f;
				const float sa = (1.f - st) * 0.80f * a;
				for (int i = 0; i < 6; ++i)
				{
					const float ang = (float)i * 1.0471975f + (float)(int)p.pos.y * 0.07f;
					const float d0 = p.radius * 0.35f + st * 14.f;
					const float d1 = d0 + 6.f + st * 10.f;
					dl->AddLine(ImVec2(p.pos.x + cosf(ang) * d0, p.pos.y + sinf(ang) * d0),
						ImVec2(p.pos.x + cosf(ang) * d1, p.pos.y + sinf(ang) * d1),
						with_alpha(col, sa), 1.3f);
				}
			}

			if (font && p.text[0] && a > 0.05f)
			{
				const ImVec2 ts = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, p.text);
				const float rise = t * 14.f;
				const ImVec2 pos(p.pos.x - ts.x * 0.5f, p.pos.y - p.radius - ts.y - 6.f - rise);

				if (sc::readout_plate)
				{
					const ImVec2 mn(pos.x - 6.f, pos.y - 3.f);
					const ImVec2 mx(pos.x + ts.x + 6.f, pos.y + ts.y + 3.f);
					dl->AddRectFilled(mn, mx, IM_COL32(8, 9, 12, (int)(150.f * a)), 5.f);
					dl->AddRect(mn, mx, with_alpha(col, 0.30f * a), 5.f, 0, 1.f);
				}

				dl->AddText(font, font_size, ImVec2(pos.x + 1.f, pos.y + 1.f),
					IM_COL32(0, 0, 0, (int)(190.f * a)), p.text);
				dl->AddText(font, font_size, pos,
					with_alpha(sc::color_mode == 2 ? col : (p.visible ? col : beam), a), p.text);
			}
		}
	}
}
