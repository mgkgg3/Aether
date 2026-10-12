#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>

#include <cmath>
#include <cstdint>
#include <map>

#if defined(AETHER_ANIM)
#include <im_anim/im_anim.h>
#endif

namespace anim
{

	enum Ease : int
	{
		Linear = 0,
		OutQuad,
		OutCubic,
		InOutCubic,
		OutExpo,
		OutBack,
		InOutSine,
	};

	inline void begin_frame()
	{
#if defined(AETHER_ANIM)
		iam_update_begin_frame();
#endif
	}

	inline float dt()
	{
		const float d = ImGui::GetIO().DeltaTime;
		if (d <= 0.f) return 1.f / 60.f;
		return d > (1.f / 15.f) ? (1.f / 15.f) : d;
	}

	inline float ease(int type, float t)
	{
		if (t <= 0.f) return 0.f;
		if (t >= 1.f) return 1.f;
		switch (type)
		{
		case OutQuad:      return 1.f - (1.f - t) * (1.f - t);
		case OutCubic:     { const float u = 1.f - t; return 1.f - u * u * u; }
		case InOutCubic:   return t < 0.5f ? 4.f * t * t * t : 1.f - powf(-2.f * t + 2.f, 3.f) * 0.5f;
		case OutExpo:      return t >= 1.f ? 1.f : 1.f - powf(2.f, -10.f * t);
		case OutBack:      { const float c1 = 1.70158f, c3 = c1 + 1.f; const float u = t - 1.f;
		                     return 1.f + c3 * u * u * u + c1 * u * u; }
		case InOutSine:    return -(cosf(3.14159265f * t) - 1.f) * 0.5f;
		default:           return t;
		}
	}

#if defined(AETHER_ANIM)
	inline iam_ease_desc map_ease(int type)
	{
		switch (type)
		{
		case OutQuad:    return iam_ease_preset(iam_ease_out_quad);
		case OutCubic:   return iam_ease_preset(iam_ease_out_cubic);
		case InOutCubic: return iam_ease_preset(iam_ease_in_out_cubic);
		case OutExpo:    return iam_ease_preset(iam_ease_out_expo);
		case OutBack:    return iam_ease_back(1.7f);
		case InOutSine:  return iam_ease_preset(iam_ease_in_out_sine);
		default:         return iam_ease_preset(iam_ease_linear);
		}
	}
#endif

#if !defined(AETHER_ANIM)
	struct state_t
	{
		float from = 0.f, to = 0.f, t = 1.f, dur = 0.25f;
		int   ease = OutCubic;
		bool  used = false;
	};

	inline std::map<ImU64, state_t>& table()
	{
		static std::map<ImU64, state_t> t;
		return t;
	}

	inline ImU64 key(ImGuiID id, ImGuiID ch) { return ((ImU64)id << 32) | (ImU64)ch; }
#endif

	inline float tween(ImGuiID id, ImGuiID ch, float target, float dur, int ez, float d, float init = 0.f)
	{
#if defined(AETHER_ANIM)
		return iam_tween_float(id, ch, target, dur, map_ease(ez), iam_policy_crossfade, d, init);
#else
		if (dur <= 0.f) return target;
		state_t& s = table()[key(id, ch)];
		if (!s.used)
		{
			s.used = true; s.from = init; s.to = target; s.t = 1.f; s.dur = dur; s.ease = ez;
		}
		if (s.to != target)
		{
			s.from = s.from + (s.to - s.from) * ease(s.ease, s.t);
			s.to = target; s.t = 0.f; s.dur = dur; s.ease = ez;
		}
		s.t += d / s.dur;
		if (s.t > 1.f) s.t = 1.f;
		return s.from + (s.to - s.from) * ease(s.ease, s.t);
#endif
	}

	inline ImVec2 tween2(ImGuiID id, ImGuiID ch, ImVec2 target, float dur, int ez, float d, ImVec2 init = ImVec2(0, 0))
	{
#if defined(AETHER_ANIM)
		return iam_tween_vec2(id, ch, target, dur, map_ease(ez), iam_policy_crossfade, d, init);
#else
		return ImVec2(tween(id, ch, target.x, dur, ez, d, init.x),
		              tween(id, ch ^ 0x9E37u, target.y, dur, ez, d, init.y));
#endif
	}

	inline float count(ImGuiID id, float target, float d, float init = 0.f, float dur = 0.45f)
	{
		return tween(id, 0x0C0CE1u, target, dur, OutExpo, d, init);
	}

	inline float breath(float speed, float lo = 0.f, float hi = 1.f)
	{
		const float t = (float)ImGui::GetTime() * speed;
		return lo + (hi - lo) * (0.5f + 0.5f * sinf(t));
	}

	inline float follow(float cur, float target, float half_life, float d)
	{
		if (half_life <= 0.0001f) return target;
		const float k = 1.f - powf(0.5f, d / half_life);
		return cur + (target - cur) * k;
	}

	inline ImVec2 follow2(ImVec2 cur, ImVec2 target, float half_life, float d)
	{
		return ImVec2(follow(cur.x, target.x, half_life, d), follow(cur.y, target.y, half_life, d));
	}

	inline ImGuiID id(const void* tag) { return ImHashData(&tag, sizeof(tag), 0x0A17u); }
	inline ImGuiID ch(const char* tag)  { return ImHashStr(tag, 0, 0x0A17u); }
}
