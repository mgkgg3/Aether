#define NOMINMAX
#include "Effects.h"

#include <Windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>
#include <game/game.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <memory/memory.h>
#include <features/misc/HitSounds.h>
#include <features/aimbot/aimbot.h>
#include <features/silentaim/silentaim.h>

namespace effects
{
	namespace
	{

		namespace cfg = settings::effects;

		struct marker_t
		{
			math::vector3 pos{};
			float         age{ 0.f };
			float         life{ 0.55f };
			bool          head{ false };

			unsigned      seed{ 0u };
			float         damage{ 0.f };
		};

		struct hit_text_t
		{
			math::vector3 pos{};
			float         age{ 0.f };
			float         life{ 1.35f };
			char          text[48]{};
		};

		std::mutex             g_mtx;
		std::vector<marker_t>  g_markers;
		std::vector<hit_text_t> g_texts;

		struct watch_t { float hp{ -1.f }; std::uint64_t seen_ms{ 0 }; };
		std::unordered_map<std::uint64_t, watch_t> g_watch;

		constexpr std::uint64_t k_shot_window_ms = 450ull;
		constexpr std::uint64_t k_aim_hold_ms   = 850ull;
		constexpr std::uint64_t k_own_hit_keep_ms = 6000ull;

		constexpr float k_cross_x = 60.f;
		constexpr float k_cross_y = 80.f;

		std::uint64_t g_last_hit_ms = 0;
		bool          g_last_hit_head = false;
		float         g_last_hit_damage = 0.f;

		int           g_streak = 0;
		std::uint64_t g_streak_at = 0;

		float         g_flash = 0.f;

		float hit_age_s()
		{
			if (!g_last_hit_ms) return 1.0e9f;

			const std::uint64_t ms = (std::uint64_t)GetTickCount64();
			if (ms < g_last_hit_ms) return 1.0e9f;
			return (float)(ms - g_last_hit_ms) / 1000.f;
		}

		std::uint64_t g_lmb_at    = 0;
		bool          g_lmb_was   = false;
		std::uint64_t g_aim_addr  = 0;
		std::uint64_t g_aim_until = 0;
		std::unordered_map<std::uint64_t, std::uint64_t> g_own_hits;

		std::uint64_t now_ms() { return (std::uint64_t)GetTickCount64(); }

		bool recent_shot(std::uint64_t ms)
		{
			const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			if (down && !g_lmb_was) g_lmb_at = ms;
			g_lmb_was = down;
			return down || (g_lmb_at != 0 && ms - g_lmb_at <= k_shot_window_ms);
		}

		math::vector3 marker_origin(const cache::entity_t& e)
		{
			static const char* const k_head_names[] = { "Head", "head", "HitboxHead" };
			for (const char* n : k_head_names)
			{
				auto it = e.parts.find(n);
				if (it == e.parts.end() || !it->second.address) continue;
				try {
					rbx::primitive_t prim = it->second.get_primitive();
					if (prim.address) return prim.get_position();
				} catch (...) {}
			}

			auto hrp = e.parts.find("HumanoidRootPart");
			if (hrp != e.parts.end() && hrp->second.address)
			{
				try {
					rbx::primitive_t prim = hrp->second.get_primitive();
					if (prim.address)
					{
						math::vector3 p = prim.get_position();
						p.y += 1.5f;
						return p;
					}
				} catch (...) {}
			}
			return {};
		}

		bool plausible(const math::vector3& v)
		{
			return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
				std::fabs(v.x) < 1.0e6f && std::fabs(v.y) < 1.0e6f && std::fabs(v.z) < 1.0e6f;
		}

		ImU32 crosshair_base()
		{
			if (cfg::crosshair_rainbow)
			{
				const float hue = std::fmod((float)ImGui::GetTime() * std::max(0.01f, cfg::crosshair_rainbow_speed), 1.f);
				const ImVec4 c = ImColor::HSV(hue < 0.f ? hue + 1.f : hue, 0.62f, 1.f);
				return IM_COL32((int)(c.x * 255.f), (int)(c.y * 255.f), (int)(c.z * 255.f), 255);
			}

			ImU32 c = IM_COL32((int)(cfg::crosshair_color[0] * 255.f),
				(int)(cfg::crosshair_color[1] * 255.f), (int)(cfg::crosshair_color[2] * 255.f),
				(int)(cfg::crosshair_color[3] * 255.f));

			if (cfg::crosshair_hit_pulse)
			{
				const float age = hit_age_s();
				if (age < 0.22f)
				{
					const float k = 1.f - age / 0.22f;
					const ImVec4 hit(1.f, 0.94f, 0.72f, 1.f);
					const int r = (int)(((c >> IM_COL32_R_SHIFT) & 0xFF) + (255 - ((c >> IM_COL32_R_SHIFT) & 0xFF)) * k);
					const int g = (int)(((c >> IM_COL32_G_SHIFT) & 0xFF) + (int)(hit.y * 255.f - ((c >> IM_COL32_G_SHIFT) & 0xFF)) * k);
					const int b = (int)(((c >> IM_COL32_B_SHIFT) & 0xFF) + (int)(hit.z * 255.f - ((c >> IM_COL32_B_SHIFT) & 0xFF)) * k);
					const int a = (c >> IM_COL32_A_SHIFT) & 0xFF;
					c = IM_COL32(ImClamp(r, 0, 255), ImClamp(g, 0, 255), ImClamp(b, 0, 255), a);
				}
			}
			return c;
		}

		float crosshair_spread()
		{
			if (!cfg::crosshair_dynamic) return 0.f;
			static float s_dyn = 0.f;
			static bool  s_was = false;
			const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			if (down && !s_was) s_dyn = 1.f;
			s_was = down;
			const float target = down ? 1.f : 0.f;

			s_dyn += (target - s_dyn) * (target > s_dyn ? 0.55f : 0.045f);
			return ImClamp(s_dyn, 0.f, 1.f);
		}

		void draw_crosshair_at(ImDrawList* dl, const ImVec2& c)
		{
			const float spread = crosshair_spread();
			const float gap   = std::max(0.f, cfg::crosshair_gap) + spread * 14.f * std::max(0.f, cfg::crosshair_dynamic_amount);
			float breathe = 1.f;
			if (cfg::crosshair_stretch)
			{
				const float amp = ImClamp(cfg::crosshair_stretch_amount, 0.f, 0.95f);
				const float spd = std::max(0.01f, cfg::crosshair_stretch_speed);
				breathe = 1.f + amp * std::sin((float)ImGui::GetTime() * spd * 6.2831853f);
			}
			const float len   = std::max(1.f, cfg::crosshair_length) * breathe * (1.f - spread * 0.25f * std::max(0.f, cfg::crosshair_dynamic_amount));
			const float thick = std::max(1.f, cfg::crosshair_thickness);

			const ImU32 col = crosshair_base();
			const ImU32 outline = IM_COL32((int)(cfg::crosshair_outline_color[0] * 255.f),
				(int)(cfg::crosshair_outline_color[1] * 255.f),
				(int)(cfg::crosshair_outline_color[2] * 255.f),
				(int)(cfg::crosshair_outline_color[3] * 255.f));

			float spin_speed = cfg::crosshair_spin_speed;
			if (cfg::crosshair_spin && std::fabs(spin_speed) < 0.01f)
				spin_speed = 90.f;

			float angle_deg = 0.f;
			if (cfg::crosshair_spin)
			{
				const float now = (float)ImGui::GetTime();
				if (cfg::crosshair_spin_windup)
				{

					constexpr float T = 1.1f;
					angle_deg = spin_speed * (now - T * (1.f - expf(-now / T)));
				}
				else
				{
					angle_deg = now * spin_speed;
				}
				angle_deg = std::fmod(angle_deg, 360.f);
				if (angle_deg < 0.f) angle_deg += 360.f;
			}

			constexpr float pi = 3.14159265f;
			const float base = angle_deg * (pi / 180.f) - (pi * 0.5f);

			ImDrawListFlags backup = dl->Flags;
			dl->Flags |= ImDrawListFlags_AntiAliasedLines;

			float swell = 1.f;
			if (cfg::crosshair_hit_pulse)
			{
				const float age = hit_age_s();
				if (age < 0.22f)
					swell = 1.f + (1.f - age / 0.22f) * 0.38f * std::max(0.f, cfg::crosshair_hit_pulse_size);
			}
			const float gap_s = gap * swell;
			const float len_s = len * swell;

			const float sx = ImClamp(cfg::crosshair_len_x, 0.10f, 3.f);
			const float sy = ImClamp(cfg::crosshair_len_y, 0.10f, 3.f);
			const float len_x_s = len_s * sx;
			const float len_y_s = len_s * sy;
			const float len_d_s = len_s * 0.5f * (sx + sy);

			if (cfg::crosshair_glow > 0.01f)
			{
				const float g = ImClamp(cfg::crosshair_glow, 0.f, 1.f) * (0.6f + 0.4f * (swell - 1.f) * 2.6f);
				const ImU32 glow = (col & ~IM_COL32_A_MASK) | ((ImU32)(90.f * g) << IM_COL32_A_SHIFT);
				for (int arm = 0; arm < 4; ++arm)
				{
					const float a = base + (float)arm * (pi * 0.5f);
					const float cs = std::cos(a), sn = std::sin(a);
					const float l = (arm & 1) ? len_x_s : len_y_s;
					dl->AddLine(ImVec2(c.x + cs * gap_s * 0.85f, c.y + sn * gap_s * 0.85f),
						ImVec2(c.x + cs * (gap_s + l) * 1.12f, c.y + sn * (gap_s + l) * 1.12f),
						glow, thick + 7.f * g);
				}
			}

			auto wedge = [&](float angle, float from, float to, float w_in, float w_out, ImU32 cw)
			{
				const float cs = std::cos(angle), sn = std::sin(angle);
				const ImVec2 n(-sn, cs);
				const ImVec2 a(c.x + cs * from, c.y + sn * from);
				const ImVec2 b(c.x + cs * to, c.y + sn * to);
				dl->AddQuadFilled(ImVec2(a.x + n.x * w_in,  a.y + n.y * w_in),
					ImVec2(b.x + n.x * w_out, b.y + n.y * w_out),
					ImVec2(b.x - n.x * w_out, b.y - n.y * w_out),
					ImVec2(a.x - n.x * w_in,  a.y - n.y * w_in), cw);
			};

			const float taper = ImClamp(cfg::crosshair_taper, 0.f, 0.95f);

			auto arm = [&](float angle, float from, float to)
			{
				if (taper > 0.02f)
				{
					const float w_in  = thick * 0.5f;
					const float w_out = w_in * (1.f - taper);
					if (cfg::crosshair_outline)
						wedge(angle, from, to, w_in + 1.f, w_out + 0.8f, outline);
					wedge(angle, from, to, w_in, w_out, col);
					return;
				}

				const float cs = std::cos(angle), sn = std::sin(angle);
				const ImVec2 p0(c.x + cs * from, c.y + sn * from);
				const ImVec2 p1(c.x + cs * to, c.y + sn * to);
				if (cfg::crosshair_outline)
					dl->AddLine(p0, p1, outline, thick + 2.f);
				dl->AddLine(p0, p1, col, thick);
			};

			switch (ImClamp(cfg::crosshair_style, 0, 5))
			{
			case 1:
				for (int i = 0; i < 4; ++i)
					arm(base + (float)i * (pi * 0.5f) + pi * 0.25f, gap_s, gap_s + len_d_s);
				break;

			case 2:
				arm(base + pi * 0.5f, gap_s, gap_s + len_x_s);
				arm(base + pi, gap_s, gap_s + len_y_s);
				arm(base, gap_s, gap_s + len_y_s);
				break;

			case 3:
			{
				const float r = std::max(2.f, gap_s + len_d_s * 0.25f);
				if (cfg::crosshair_outline)
					dl->AddCircle(c, r, outline, 40, thick + 2.f);
				dl->AddCircle(c, r, col, 40, thick);
				break;
			}

			case 4:
			{
				const float wing = pi * 0.30f;
				for (int i = 0; i < 4; ++i)
				{
					const float a = base + (float)i * (pi * 0.5f);
					const float l = (i & 1) ? len_x_s : len_y_s;
					arm(a, gap_s + l, gap_s + l * 0.25f);
					arm(a + wing, gap_s + l * 1.02f, gap_s + l * 0.45f);
					arm(a - wing, gap_s + l * 1.02f, gap_s + l * 0.45f);
				}
				break;
			}

			case 5:
			{
				const float r = std::max(1.f, thick + 1.2f);
				for (int i = 0; i < 4; ++i)
				{
					const float a = base + (float)i * (pi * 0.5f);
					const ImVec2 p(c.x + std::cos(a) * gap_s, c.y + std::sin(a) * gap_s);
					if (cfg::crosshair_outline)
						dl->AddCircleFilled(p, r + 1.4f, outline, 16);
					dl->AddCircleFilled(p, r, col, 16);
				}
				break;
			}

			default:
				for (int i = 0; i < 4; ++i)
					arm(base + (float)i * (pi * 0.5f), gap_s,
						gap_s + ((i & 1) ? len_x_s : len_y_s));
				break;
			}

			if (cfg::crosshair_dot)
			{
				const float r = std::max(1.f, cfg::crosshair_dot_size) * swell;
				if (cfg::crosshair_outline)
					dl->AddCircleFilled(ImVec2(c.x, c.y), r + 1.f, outline, 16);
				dl->AddCircleFilled(ImVec2(c.x, c.y), r, col, 16);
			}

			dl->Flags = backup;
		}

		bool crosshair_centre(ImVec2& out)
		{
			if (cfg::crosshair_lock)
			{
				const ImVec2 ds = ImGui::GetIO().DisplaySize;
				out = ImVec2(std::floor(ds.x * 0.5f) + 0.5f, std::floor(ds.y * 0.5f) + 0.5f);
				return true;
			}

			POINT pt{};
			if (!GetCursorPos(&pt)) return false;
			out = ImVec2((float)pt.x, (float)pt.y);
			return true;
		}

		void draw_crosshair(ImDrawList* dl)
		{
			if (!cfg::crosshair) return;

			ImVec2 c{};
			if (!crosshair_centre(c)) return;
			draw_crosshair_at(dl, c);
		}

		void draw_marker(ImDrawList* dl, const marker_t& m,
			const math::matrix4& view, const math::vector2& dims)
		{
			float t = m.age / std::max(0.01f, m.life);
			t = std::clamp(t, 0.f, 1.f);

			const float fade_in = std::clamp(t / 0.08f, 0.f, 1.f);
			const float a = fade_in * (1.f - t);

			math::vector2 sp{};
			if (!game::visengine.world_to_screen(m.pos, sp, dims, view)) return;

			math::vector2 side{};
			float base = 20.f;
			if (game::visengine.world_to_screen(
				{ m.pos.x + 0.55f, m.pos.y, m.pos.z }, side, dims, view))
			{
				const float dx = side.x - sp.x, dy = side.y - sp.y;
				base = std::max(4.f, std::sqrt(dx * dx + dy * dy));
			}
			base *= std::max(0.55f, cfg::hitmarker_size);

			const float pop = 1.f + (1.f - fade_in) * 0.55f * std::max(0.f, cfg::hitmarker_pop);

			const float crit = (m.head && cfg::hitmarker_crit_pulse) ? (1.f - fade_in) : 0.f;
			const float s = base * pop * (1.f + crit * 0.22f);
			const ImVec2 c(sp.x, sp.y);
			const int   mstyle = ImClamp(cfg::hitmarker_style, 0, 5);

			auto mul_a = [](ImU32 col, float alpha) -> ImU32 {
				int na = (int)((float)((col >> IM_COL32_A_SHIFT) & 0xFF) * alpha + 0.5f);
				na = na < 0 ? 0 : (na > 255 ? 255 : na);
				return (col & ~IM_COL32_A_MASK) | ((ImU32)na << IM_COL32_A_SHIFT);
			};
			auto col4 = [](const float* c4) -> ImU32 {
				return IM_COL32((int)(c4[0] * 255.f), (int)(c4[1] * 255.f),
					(int)(c4[2] * 255.f), (int)(c4[3] * 255.f));
			};

			const ImU32 acc = col4(m.head ? cfg::hitmarker_head_color
				: cfg::hitmarker_color);
			const ImU32 hi = IM_COL32(255, 255, 255, 255);
			const ImU32 shadow = IM_COL32(0, 0, 0, 180);

			if (cfg::hitmarker_glow > 0.01f)
			{
				const float g = ImClamp(cfg::hitmarker_glow, 0.f, 1.f);
				for (int i = 3; i >= 1; --i)
					dl->AddCircleFilled(c, s * (0.55f + (float)i * 0.30f), mul_a(acc, a * 0.10f * g / (float)i), 28);
			}

			if (mstyle == 0)
			{
			dl->AddCircle(c, s * (1.1f + t * 1.8f), mul_a(acc, a * 0.55f), 36, 1.5f);

			const ImVec2 d0(c.x, c.y - s * 0.55f);
			const ImVec2 d1(c.x + s * 0.55f, c.y);
			const ImVec2 d2(c.x, c.y + s * 0.55f);
			const ImVec2 d3(c.x - s * 0.55f, c.y);
			dl->AddQuad(ImVec2(d0.x + 1, d0.y + 1), ImVec2(d1.x + 1, d1.y + 1),
				ImVec2(d2.x + 1, d2.y + 1), ImVec2(d3.x + 1, d3.y + 1),
				mul_a(shadow, a), 1.5f);
			dl->AddQuad(d0, d1, d2, d3, mul_a(acc, a), 1.6f);

			const float gap = s * 0.22f;
			const float arm = s * 0.85f;
			const float th = 2.0f;
			auto arm_line = [&](ImVec2 a0, ImVec2 a1) {
				dl->AddLine(ImVec2(a0.x + 1, a0.y + 1), ImVec2(a1.x + 1, a1.y + 1),
					mul_a(shadow, a), th + 1.0f);
				dl->AddLine(a0, a1, mul_a(hi, a), th);
				dl->AddLine(a0, a1, mul_a(acc, a * 0.85f), th * 0.55f);
			};
			arm_line(ImVec2(c.x - arm, c.y - arm), ImVec2(c.x - gap, c.y - gap));
			arm_line(ImVec2(c.x + gap, c.y - gap), ImVec2(c.x + arm, c.y - arm));
			arm_line(ImVec2(c.x - arm, c.y + arm), ImVec2(c.x - gap, c.y + gap));
			arm_line(ImVec2(c.x + gap, c.y + gap), ImVec2(c.x + arm, c.y + arm));

			dl->AddCircleFilled(c, s * 0.12f * (1.0f - t * 0.5f), mul_a(hi, a * 0.9f), 12);
			}
			else
			{

				const float th = std::max(1.2f, s * 0.075f);
				switch (mstyle)
				{
				case 1:
				{
					const float l = s * 0.78f;
					for (int q = 0; q < 4; ++q)
					{
						const float sx = (q & 1) ? 1.f : -1.f;
						const float sy = (q & 2) ? 1.f : -1.f;
						dl->AddLine(ImVec2(c.x, c.y), ImVec2(c.x + sx * l, c.y + sy * l), mul_a(hi, a), th);
						dl->AddLine(ImVec2(c.x, c.y), ImVec2(c.x + sx * l * 0.62f, c.y + sy * l * 0.62f),
							mul_a(acc, a * 0.9f), th * 0.7f);
					}
					break;
				}

				case 2:
				{
					const float r = s * (1.5f - t * 0.75f);
					const float wing = s * 0.34f;
					for (int q = 0; q < 4; ++q)
					{
						const float ang = (float)q * 1.5707964f + 0.7853982f;
						const ImVec2 p(c.x + std::cos(ang) * r, c.y + std::sin(ang) * r);
						const float ax = -std::cos(ang), ay = -std::sin(ang);
						const float px = -ay, py = ax;
						dl->AddLine(ImVec2(p.x + px * wing, p.y + py * wing), p, mul_a(hi, a), th * 1.15f);
						dl->AddLine(p, ImVec2(p.x - px * wing, p.y - py * wing), mul_a(hi, a), th * 1.15f);
						dl->AddLine(ImVec2(p.x + px * wing * 0.6f, p.y + py * wing * 0.6f),
							ImVec2(p.x - px * wing * 0.6f, p.y - py * wing * 0.6f), mul_a(acc, a * 0.6f), th * 0.6f);
						(void)ax; (void)ay;
					}
					break;
				}

				case 3:
				{
					for (int q = 0; q < 3; ++q)
					{
						const float phase_t = ImClamp(t * 1.35f - (float)q * 0.16f, 0.f, 1.f);
						const float r = s * (1.9f - phase_t * 1.55f);
						const float aa = a * (1.f - phase_t * 0.55f);
						if (aa <= 0.01f) continue;
						dl->AddCircle(c, r, mul_a(q == 0 ? hi : acc, aa), 32, th * (q == 0 ? 1.2f : 0.9f));
					}
					break;
				}

				case 4:
				{
					const float spin = (float)ImGui::GetTime() * 0.9f;
					for (int q = 0; q < 8; ++q)
					{
						const float ang = spin + (float)q * 0.7853982f;
						const float l = s * ((q & 1) ? 0.55f : 1.05f) * (1.f - t * 0.35f);
						const float cs = std::cos(ang), sn = std::sin(ang);
						dl->AddLine(ImVec2(c.x + cs * l * 0.25f, c.y + sn * l * 0.25f),
							ImVec2(c.x + cs * l, c.y + sn * l),
							mul_a((q & 1) ? acc : hi, a), th * ((q & 1) ? 0.75f : 1.1f));
					}
					break;
				}

				default:
				{
					const float r = s * (1.85f - t * 1.05f);
					const float arm = s * 0.42f;
					for (int q = 0; q < 4; ++q)
					{
						const float sx = (q & 1) ? 1.f : -1.f;
						const float sy = (q & 2) ? 1.f : -1.f;
						const ImVec2 p(c.x + sx * r, c.y + sy * r);
						dl->AddLine(p, ImVec2(p.x - sx * arm, p.y), mul_a(hi, a), th * 1.2f);
						dl->AddLine(p, ImVec2(p.x, p.y - sy * arm), mul_a(hi, a), th * 1.2f);
						dl->AddLine(ImVec2(p.x - sx * arm * 0.55f, p.y), p, mul_a(acc, a * 0.55f), th * 0.6f);
					}
					break;
				}
				}
			}

			if (cfg::hitmarker_shockwave)
			{
				const float k = ImClamp(cfg::hitmarker_shockwave_size, 0.2f, 3.f);
				const float wr = s * (0.9f + t * 3.0f) * k;
				const float wa = a * (1.f - t) * 0.55f;
				if (wa > 0.01f)
				{
					dl->AddCircle(c, wr, mul_a(hi, wa), 40, 1.6f + 1.4f * (1.f - t));
					if (cfg::hitmarker_glow > 0.01f)
						dl->AddCircle(c, wr * 0.94f, mul_a(acc, wa * 0.4f), 40, 3.6f);
				}
			}

			if (cfg::hitmarker_sparks && cfg::hitmarker_spark_count > 0)
			{
				const int want = ImClamp(cfg::hitmarker_spark_count, 1, 40);
				const float sp_k = ImClamp(cfg::hitmarker_spark_speed, 0.2f, 3.f);
				unsigned s32 = m.seed ? m.seed : 0x9E3779B9u;
				const auto rnd = [&s32]() { s32 ^= s32 << 13; s32 ^= s32 >> 17; s32 ^= s32 << 5; return s32; };
				const float ease_out = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
				for (int i = 0; i < want; ++i)
				{
					const float ang = (float)(rnd() % 62832u) * 0.0001f;
					const float spd = (0.35f + (float)(rnd() % 1000u) * 0.0011f) * sp_k;
					const float cs = std::cos(ang), sn = std::sin(ang);
					const float d0 = s * (0.35f + ease_out * 2.6f * spd);
					const float d1 = d0 + s * (0.30f + 0.45f * (1.f - t)) * spd;
					const float aa = a * (1.f - t) * (0.55f + 0.45f * spd);
					if (aa <= 0.01f) continue;
					dl->AddLine(ImVec2(c.x + cs * d0, c.y + sn * d0), ImVec2(c.x + cs * d1, c.y + sn * d1),
						mul_a(i % 3 == 0 ? hi : acc, aa), 1.1f + 1.3f * spd);
				}
			}

			if (cfg::hitmarker_flash && t < 0.16f)
			{
				const float k = 1.f - t / 0.16f;
				dl->AddCircleFilled(c, s * 0.30f * k + 1.5f, mul_a(hi, k * a), 24);
			}

			if (cfg::hitmarker_streak && g_streak > 1 && t < 0.35f)
			{
				char chain[24];
				std::snprintf(chain, sizeof chain, "x%d", g_streak);
				ImFont* font = ImGui::GetFont();
				if (font)
				{
					const float fs = std::max(11.f, s * 0.5f);
					const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.f, chain);
					const ImVec2 p(c.x - ts.x * 0.5f, c.y + s * 1.25f + (1.f - t / 0.35f) * 5.f);
					dl->AddText(font, fs, ImVec2(p.x + 1.f, p.y + 1.f), IM_COL32(0, 0, 0, (int)(190.f * a)), chain);
					dl->AddText(font, fs, p, mul_a(acc, a), chain);
				}
			}
		}

		void draw_hit_text(ImDrawList* dl, const hit_text_t& ht,
			const math::matrix4& view, const math::vector2& dims)
		{
			float t = ht.age / std::max(0.01f, ht.life);
			t = std::clamp(t, 0.f, 1.f);

			const float fade_in = std::clamp(t / 0.08f, 0.f, 1.f);
			const float fo = std::clamp((t - 0.6f) / 0.4f, 0.f, 1.f);
			const float a = fade_in * (1.f - fo);
			if (a <= 0.01f) return;

			math::vector3 wp = ht.pos;
			wp.y += t * 2.2f;

			math::vector2 sp{};
			if (!game::visengine.world_to_screen(wp, sp, dims, view)) return;

			const float fs = std::max(10.f, cfg::hitdata_size);
			ImFont* font = ImGui::GetFont();
			if (ImGui::GetCurrentContext() && ImGui::GetFontSize() > 0.f)
				font = ImGui::GetFont();
			const ImVec2 tsz = font->CalcTextSizeA(fs, FLT_MAX, 0.f, ht.text);
			const ImVec2 pos(sp.x - tsz.x * 0.5f, sp.y - tsz.y * 0.5f);

			const float ca[4] = { 1.f, 1.f, 1.f, a };
			dl->AddText(font, fs, ImVec2(pos.x + 1.f, pos.y + 1.f),
				IM_COL32(0, 0, 0, (int)(200 * a)), ht.text);
			dl->AddText(font, fs, pos,
				IM_COL32((int)(ca[0] * 255.f), (int)(ca[1] * 255.f), (int)(ca[2] * 255.f),
					(int)(ca[3] * 255.f)), ht.text);
		}
	}

	void reset()
	{
		std::lock_guard<std::mutex> lk(g_mtx);
		g_markers.clear();
		g_texts.clear();
		g_watch.clear();
		g_own_hits.clear();
		g_aim_addr  = 0;
		g_aim_until = 0;

		g_last_hit_ms = 0;
		g_last_hit_head = false;
		g_last_hit_damage = 0.f;
		g_streak = 0;
		g_streak_at = 0;
		g_flash = 0.f;
	}

	void play_hit_sound()
	{
		if (!settings::effects::hitsound) return;
		hitsounds::play(settings::effects::hitsound_index);
	}

	bool own_hit_recent(std::uint64_t addr, float window_s)
	{
		if (!addr) return false;

		const std::uint64_t ms = now_ms();
		const std::uint64_t win = (std::uint64_t)(window_s * 1000.f);

		std::lock_guard<std::mutex> lk(g_mtx);
		auto it = g_own_hits.find(addr);
		if (it == g_own_hits.end()) return false;
		return ms >= it->second && ms - it->second <= win;
	}

	float own_hit_fade(std::uint64_t addr, float duration_s)
	{
		if (!addr) return 0.f;

		const float dur = (duration_s > 0.05f) ? duration_s : 0.05f;
		const std::uint64_t ms = now_ms();

		std::lock_guard<std::mutex> lk(g_mtx);
		auto it = g_own_hits.find(addr);
		if (it == g_own_hits.end() || ms < it->second) return 0.f;

		const float age = (float)(ms - it->second) / 1000.f;
		if (age >= dur) return 0.f;
		return 1.f - age / dur;
	}

	void draw_crosshair_preview(ImDrawList* dl, const ImVec2& centre)
	{
		if (!dl) return;
		draw_crosshair_at(dl, centre);
	}

	void run()
	{
		auto last = std::chrono::steady_clock::now();

		for (;;)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(4));

			const auto now = std::chrono::steady_clock::now();
			const float dt = std::min(
				std::chrono::duration<float>(now - last).count(), 0.1f);
			last = now;

			{
				std::lock_guard<std::mutex> lk(g_mtx);
				for (auto it = g_markers.begin(); it != g_markers.end(); )
					it = (it->age += dt) >= it->life ? g_markers.erase(it) : it + 1;
				for (auto it = g_texts.begin(); it != g_texts.end(); )
					it = (it->age += dt) >= it->life ? g_texts.erase(it) : it + 1;
			}

			if (!settings::effects::hitmarker && !settings::effects::hitdata &&
				!settings::effects::hitsound &&
				!settings::visuals::hit_chams && !settings::visuals::hit_skeleton)
				continue;

			std::vector<cache::entity_t> snap;
			math::vector3 local{};
			{
				std::lock_guard<std::mutex> lk(cache::mtx);
				snap = cache::cached_players;
				auto hrp = cache::cached_local_player.parts.find("HumanoidRootPart");
				if (hrp != cache::cached_local_player.parts.end() && hrp->second.address)
				{
					try {
						rbx::primitive_t p = hrp->second.get_primitive();
						if (p.address) local = p.get_position();
					} catch (...) {}
				}
			}

			const std::uint64_t ms = now_ms();

			const std::uint64_t aim_now =
				aimbot::player.instance.address ? aimbot::player.instance.address :
				(aimbot::sticky_target.instance.address ? aimbot::sticky_target.instance.address :
					silentaim::state.target.instance.address);
			if (aim_now)
			{
				g_aim_addr  = aim_now;
				g_aim_until = ms + k_aim_hold_ms;
			}
			else if (g_aim_addr && ms > g_aim_until)
			{
				g_aim_addr = 0;
			}

			const bool shot = recent_shot(ms);
			const math::vector2 shot_dims = game::visengine.get_dimensions();
			const math::matrix4 shot_view = game::visengine.get_viewmatrix();

			for (auto& e : snap)
			{
				if (!e.instance.address || !e.humanoid.address) continue;
				if (!std::isfinite(e.health) || e.max_health <= 0.f) continue;

				watch_t& w = g_watch[e.instance.address];
				const float seen = e.health;
				w.seen_ms = ms;

				if (w.hp < 0.f || seen > w.hp + 0.5f)
				{
					w.hp = seen;
					continue;
				}

				const float damage = w.hp - seen;
				w.hp = seen;
				if (damage < 0.5f) continue;

				math::vector3 origin = marker_origin(e);
				if (!plausible(origin)) continue;

				bool mine = false;
				if (shot)
				{
					if (g_aim_addr != 0 && ms <= g_aim_until &&
						e.instance.address == g_aim_addr)
					{
						mine = true;
					}
					else
					{
						const float ccx = rbx::visualengine_t::window_offset_x() + shot_dims.x * 0.5f;
						const float ccy = rbx::visualengine_t::window_offset_y() + shot_dims.y * 0.5f;

						math::vector2 sp{};
						mine = game::visengine.world_to_screen(origin, sp, shot_dims, shot_view) &&
							std::fabs(sp.x - ccx) <= k_cross_x &&
							std::fabs(sp.y - ccy) <= k_cross_y;
					}
				}
				if (!mine) continue;

				{
					std::lock_guard<std::mutex> lk(g_mtx);
					g_own_hits[e.instance.address] = ms;
				}

				bool head = false;
				if (game::visengine.address)
				{
					const math::vector2 dims = game::visengine.get_dimensions();
					const math::matrix4 view = game::visengine.get_viewmatrix();
					const float ccx = rbx::visualengine_t::window_offset_x() + dims.x * 0.5f;
					const float ccy = rbx::visualengine_t::window_offset_y() + dims.y * 0.5f;
					math::vector2 sp{};
					if (game::visengine.world_to_screen(origin, sp, dims, view))
						head = std::fabs(sp.x - ccx) < 14.f &&
							std::fabs(sp.y - ccy) < 20.f;
				}

				{
					const std::uint64_t chain_window = (std::uint64_t)(std::max(0.2f,
						settings::effects::hitmarker_streak_window) * 1000.f);
					if (!g_streak_at || ms - g_streak_at > chain_window)
						g_streak = 1;
					else
						++g_streak;
					g_streak_at = ms;
					g_last_hit_ms = ms;
					g_last_hit_head = head;
					g_last_hit_damage = damage;
				}

				play_hit_sound();

				std::lock_guard<std::mutex> lk(g_mtx);

				if (settings::effects::hitmarker)
				{
					marker_t m;
					m.pos = origin;
					m.life = std::max(0.2f, settings::effects::hitmarker_duration);
					m.head = head;

					m.seed = (unsigned)(ms * 2654435761ull) ^ (unsigned)(e.instance.address & 0xFFFFFFFFu);
					m.damage = damage;
					g_markers.push_back(m);

					if (settings::effects::hitmarker_screen_flash > 0.001f)
						g_flash = ImMax(g_flash, ImClamp(settings::effects::hitmarker_screen_flash, 0.f, 1.f) *
							(head ? 1.5f : 1.0f));
					if (g_markers.size() > 32) g_markers.erase(g_markers.begin());
				}

				if (settings::effects::hitdata)
				{
					char lines[3][48]{};
					int count = 0;
					if (settings::effects::hitdata_damage)
						std::snprintf(lines[count++], 48, "-%.0f", damage);
					if (settings::effects::hitdata_health)
						std::snprintf(lines[count++], 48, "%.0f HP", std::max(0.f, seen));
					if (settings::effects::hitdata_distance)
					{
						const math::vector3 d{ origin.x - local.x, origin.y - local.y, origin.z - local.z };
						const float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
						const bool meters = settings::visuals::distance_unit == 1;
						std::snprintf(lines[count++], 48, meters ? "%.0fm" : "%.0f",
							meters ? dist * 0.28f : dist);
					}

					for (int i = 0; i < count; ++i)
					{
						hit_text_t t;
						t.pos = origin;
						t.pos.y += (float)i * 0.45f;
						t.life = std::max(0.35f, settings::effects::hitdata_duration);
						std::snprintf(t.text, sizeof(t.text), "%s", lines[i]);
						g_texts.push_back(t);
					}
					while (g_texts.size() > 36) g_texts.erase(g_texts.begin());
				}
			}

			for (auto it = g_watch.begin(); it != g_watch.end(); )
			{
				if (ms - it->second.seen_ms > 4000) it = g_watch.erase(it);
				else ++it;
			}

			{
				std::lock_guard<std::mutex> lk(g_mtx);
				for (auto it = g_own_hits.begin(); it != g_own_hits.end(); )
				{
					if (ms - it->second > k_own_hit_keep_ms) it = g_own_hits.erase(it);
					else ++it;
				}
			}

		}
	}

	void draw(ImDrawList* draw_list, const math::vector2& dims, const math::matrix4& view)
	{
		if (!draw_list) return;

		{
			const float age = hit_age_s();
			const float want = (age < 0.22f && settings::effects::hitmarker &&
				settings::effects::hitmarker_screen_flash > 0.001f) ? (1.f - age / 0.22f) : 0.f;
			g_flash = ImMax(want * ImClamp(settings::effects::hitmarker_screen_flash, 0.f, 1.f) *
				(g_last_hit_head ? 1.5f : 1.0f), g_flash - ImGui::GetIO().DeltaTime * 4.5f);
			if (g_flash > 0.01f)
			{
				const float k = ImClamp(g_flash, 0.f, 1.f);
				const int inner = (int)(150.f * k);
				const ImU32 edge = IM_COL32(255, 255, 255, (int)(26.f * k));
				const ImU32 clear = IM_COL32(255, 255, 255, 0);
				const float band = ImMax(24.f, dims.y * 0.22f);
				draw_list->AddRectFilledMultiColor(ImVec2(0.f, 0.f), ImVec2(dims.x, band),
					edge, edge, clear, clear);
				draw_list->AddRectFilledMultiColor(ImVec2(0.f, dims.y - band), ImVec2(dims.x, dims.y),
					clear, clear, edge, edge);
				draw_list->AddRectFilledMultiColor(ImVec2(0.f, band), ImVec2(58.f, dims.y - band),
					edge, clear, clear, edge);
				draw_list->AddRectFilledMultiColor(ImVec2(dims.x - 58.f, band), ImVec2(dims.x, dims.y - band),
					clear, edge, edge, clear);
				(void)inner;
			}
		}

		draw_crosshair(draw_list);

		std::lock_guard<std::mutex> lk(g_mtx);
		for (auto& m : g_markers)
			draw_marker(draw_list, m, view, dims);
		for (auto& t : g_texts)
			draw_hit_text(draw_list, t, view, dims);
	}
}
