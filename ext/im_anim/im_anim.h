
#pragma once

#define IMANIM_VERSION          "1.0.0"
#define IMANIM_VERSION_NUM      10000
#define IMANIM_VERSION_MAJOR    1
#define IMANIM_VERSION_MINOR    0
#define IMANIM_VERSION_PATCH    0
#include "imgui.h"
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#ifndef IM_ANIM_API
#define IM_ANIM_API
#endif

#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 19200
#define IM_ANIM_PRE_19200_COMPATIBILITY
#endif

#ifndef IAM_PI
static float const IAM_PI = 3.1415926535897932384626433832795f;
#endif
#ifndef IAM_2PI
static float const IAM_2PI = IAM_PI * 2.0f;
#endif

enum iam_ease_type {
	iam_ease_linear = 0,
	iam_ease_in_quad,  iam_ease_out_quad,  iam_ease_in_out_quad,
	iam_ease_in_cubic, iam_ease_out_cubic, iam_ease_in_out_cubic,
	iam_ease_in_quart, iam_ease_out_quart, iam_ease_in_out_quart,
	iam_ease_in_quint, iam_ease_out_quint, iam_ease_in_out_quint,
	iam_ease_in_sine,  iam_ease_out_sine,  iam_ease_in_out_sine,
	iam_ease_in_expo,  iam_ease_out_expo,  iam_ease_in_out_expo,
	iam_ease_in_circ,  iam_ease_out_circ,  iam_ease_in_out_circ,
	iam_ease_in_back,  iam_ease_out_back,  iam_ease_in_out_back,
	iam_ease_in_elastic, iam_ease_out_elastic, iam_ease_in_out_elastic,
	iam_ease_in_bounce,  iam_ease_out_bounce,  iam_ease_in_out_bounce,
	iam_ease_steps,
	iam_ease_cubic_bezier,
	iam_ease_spring,
	iam_ease_custom
};

enum iam_policy {
	iam_policy_crossfade = 0,
	iam_policy_cut,
	iam_policy_queue,
	iam_policy_additive,
	iam_policy_multiply
};

enum iam_color_space {
	iam_col_srgb = 0,
	iam_col_srgb_linear,
	iam_col_hsv,
	iam_col_oklab,
	iam_col_oklch
};

enum iam_anchor_space {
	iam_anchor_window_content = 0,
	iam_anchor_window,
	iam_anchor_viewport,
	iam_anchor_last_item
};

struct iam_ease_desc {
	int		type;
	float	p0, p1, p2, p3;
};

typedef float (*iam_ease_fn)(float t);

IM_ANIM_API void iam_update_begin_frame();
IM_ANIM_API void iam_gc(unsigned int max_age_frames = 600);
IM_ANIM_API void iam_pool_clear();
IM_ANIM_API void iam_reserve(int cap_float, int cap_vec2, int cap_vec4, int cap_int, int cap_color);
IM_ANIM_API void iam_set_ease_lut_samples(int count);

IM_ANIM_API void  iam_set_global_time_scale(float scale);
IM_ANIM_API float iam_get_global_time_scale();

IM_ANIM_API void iam_set_lazy_init(bool enable);
IM_ANIM_API bool iam_is_lazy_init_enabled();

IM_ANIM_API void iam_register_custom_ease(int slot, iam_ease_fn fn);
IM_ANIM_API iam_ease_fn iam_get_custom_ease(int slot);

IM_ANIM_API void iam_show_unified_inspector(bool* p_open = nullptr);
IM_ANIM_API void iam_show_debug_timeline(ImGuiID instance_id);

IM_ANIM_API void iam_profiler_enable(bool enable);
IM_ANIM_API bool iam_profiler_is_enabled();
IM_ANIM_API void iam_profiler_begin_frame();
IM_ANIM_API void iam_profiler_end_frame();
IM_ANIM_API void iam_profiler_begin(const char* name);
IM_ANIM_API void iam_profiler_end();

struct iam_drag_opts {
	ImVec2 snap_grid;
	ImVec2* snap_points;
	int snap_points_count;
	float snap_duration;
	float overshoot;
	int ease_type;

	iam_drag_opts() : snap_grid(0, 0), snap_points(nullptr), snap_points_count(0),
		snap_duration(0.2f), overshoot(0), ease_type(iam_ease_out_cubic) {}
};

struct iam_drag_feedback {
	ImVec2 position;
	ImVec2 offset;
	ImVec2 velocity;
	bool is_dragging;
	bool is_snapping;
	float snap_progress;
};

IM_ANIM_API iam_drag_feedback iam_drag_begin(ImGuiID id, ImVec2 pos);
IM_ANIM_API iam_drag_feedback iam_drag_update(ImGuiID id, ImVec2 pos, float dt);
IM_ANIM_API iam_drag_feedback iam_drag_release(ImGuiID id, ImVec2 pos, iam_drag_opts const& opts, float dt);
IM_ANIM_API void iam_drag_cancel(ImGuiID id);

enum iam_wave_type {
	iam_wave_sine = 0,
	iam_wave_triangle,
	iam_wave_sawtooth,
	iam_wave_square
};
IM_ANIM_API float  iam_oscillate(ImGuiID id, float amplitude, float frequency, int wave_type, float phase, float dt);
IM_ANIM_API int    iam_oscillate_int(ImGuiID id, int amplitude, float frequency, int wave_type, float phase, float dt);
IM_ANIM_API ImVec2 iam_oscillate_vec2(ImGuiID id, ImVec2 amplitude, ImVec2 frequency, int wave_type, ImVec2 phase, float dt);
IM_ANIM_API ImVec4 iam_oscillate_vec4(ImGuiID id, ImVec4 amplitude, ImVec4 frequency, int wave_type, ImVec4 phase, float dt);
IM_ANIM_API ImVec4 iam_oscillate_color(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, int wave_type, float phase, int color_space, float dt);

IM_ANIM_API float  iam_shake(ImGuiID id, float intensity, float frequency, float decay_time, float dt);
IM_ANIM_API int    iam_shake_int(ImGuiID id, int intensity, float frequency, float decay_time, float dt);
IM_ANIM_API ImVec2 iam_shake_vec2(ImGuiID id, ImVec2 intensity, float frequency, float decay_time, float dt);
IM_ANIM_API ImVec4 iam_shake_vec4(ImGuiID id, ImVec4 intensity, float frequency, float decay_time, float dt);
IM_ANIM_API ImVec4 iam_shake_color(ImGuiID id, ImVec4 base_color, ImVec4 intensity, float frequency, float decay_time, int color_space, float dt);
IM_ANIM_API float  iam_wiggle(ImGuiID id, float amplitude, float frequency, float dt);
IM_ANIM_API int    iam_wiggle_int(ImGuiID id, int amplitude, float frequency, float dt);
IM_ANIM_API ImVec2 iam_wiggle_vec2(ImGuiID id, ImVec2 amplitude, float frequency, float dt);
IM_ANIM_API ImVec4 iam_wiggle_vec4(ImGuiID id, ImVec4 amplitude, float frequency, float dt);
IM_ANIM_API ImVec4 iam_wiggle_color(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, int color_space, float dt);
IM_ANIM_API void   iam_trigger_shake(ImGuiID id);

IM_ANIM_API float iam_eval_preset(int type, float t);

IM_ANIM_API float  iam_tween_float(ImGuiID id, ImGuiID channel_id, float target, float dur, iam_ease_desc const& ez, int policy, float dt, float init_value = 0.0f);
IM_ANIM_API ImVec2 iam_tween_vec2(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_desc const& ez, int policy, float dt, ImVec2 init_value = ImVec2(0, 0));
IM_ANIM_API ImVec4 iam_tween_vec4(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_desc const& ez, int policy, float dt, ImVec4 init_value = ImVec4(0, 0, 0, 0));
IM_ANIM_API int    iam_tween_int(ImGuiID id, ImGuiID channel_id, int target, float dur, iam_ease_desc const& ez, int policy, float dt, int init_value = 0);
IM_ANIM_API ImVec4 iam_tween_color(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt, ImVec4 init_value = ImVec4(1, 1, 1, 1));

IM_ANIM_API ImVec2 iam_anchor_size(int space);

IM_ANIM_API float  iam_tween_float_rel(ImGuiID id, ImGuiID channel_id, float percent, float px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, int axis, float dt);
IM_ANIM_API ImVec2 iam_tween_vec2_rel(ImGuiID id, ImGuiID channel_id, ImVec2 percent, ImVec2 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);
IM_ANIM_API ImVec4 iam_tween_vec4_rel(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);
IM_ANIM_API ImVec4 iam_tween_color_rel(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int color_space, int anchor_space, float dt);

typedef float  (*iam_float_resolver)(void* user);
typedef ImVec2 (*iam_vec2_resolver)(void* user);
typedef ImVec4 (*iam_vec4_resolver)(void* user);
typedef ImVec4 (*iam_color_resolver)(void* user);
typedef int    (*iam_int_resolver)(void* user);

IM_ANIM_API float  iam_tween_float_resolved(ImGuiID id, ImGuiID channel_id, iam_float_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);
IM_ANIM_API ImVec2 iam_tween_vec2_resolved(ImGuiID id, ImGuiID channel_id, iam_vec2_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);
IM_ANIM_API ImVec4 iam_tween_vec4_resolved(ImGuiID id, ImGuiID channel_id, iam_vec4_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);
IM_ANIM_API ImVec4 iam_tween_color_resolved(ImGuiID id, ImGuiID channel_id, iam_color_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt);
IM_ANIM_API int    iam_tween_int_resolved(ImGuiID id, ImGuiID channel_id, iam_int_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);

IM_ANIM_API void iam_rebase_float(ImGuiID id, ImGuiID channel_id, float new_target, float dt);
IM_ANIM_API void iam_rebase_vec2(ImGuiID id, ImGuiID channel_id, ImVec2 new_target, float dt);
IM_ANIM_API void iam_rebase_vec4(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt);
IM_ANIM_API void iam_rebase_color(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt);
IM_ANIM_API void iam_rebase_int(ImGuiID id, ImGuiID channel_id, int new_target, float dt);

IM_ANIM_API ImVec4 iam_get_blended_color(ImVec4 a_srgb, ImVec4 b_srgb, float t, int color_space);

inline iam_ease_desc iam_ease_preset(int type) { iam_ease_desc e = { type, 0,0,0,0 }; return e; }
inline iam_ease_desc iam_ease_bezier(float x1, float y1, float x2, float y2) { iam_ease_desc e = { iam_ease_cubic_bezier, x1,y1,x2,y2 }; return e; }
inline iam_ease_desc iam_ease_steps_desc(int steps, int mode) { iam_ease_desc e = { iam_ease_steps, (float)steps,(float)mode,0,0 }; return e; }
inline iam_ease_desc iam_ease_back(float overshoot) { iam_ease_desc e = { iam_ease_out_back, overshoot,0,0,0 }; return e; }
inline iam_ease_desc iam_ease_elastic(float amplitude, float period) { iam_ease_desc e = { iam_ease_out_elastic, amplitude, period,0,0 }; return e; }
inline iam_ease_desc iam_ease_spring_desc(float mass, float stiffness, float damping, float v0) { iam_ease_desc e = { iam_ease_spring, mass, stiffness, damping, v0 }; return e; }
inline iam_ease_desc iam_ease_custom_fn(int slot) { iam_ease_desc e = { iam_ease_custom, (float)slot,0,0,0 }; return e; }

IM_ANIM_API void iam_scroll_to_y(float target_y, float duration, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));
IM_ANIM_API void iam_scroll_to_x(float target_x, float duration, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));
IM_ANIM_API void iam_scroll_to_top(float duration = 0.3f, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));
IM_ANIM_API void iam_scroll_to_bottom(float duration = 0.3f, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));

struct iam_ease_per_axis {
	iam_ease_desc x;
	iam_ease_desc y;
	iam_ease_desc z;
	iam_ease_desc w;

	iam_ease_per_axis()
		: x(iam_ease_preset(iam_ease_linear)), y(iam_ease_preset(iam_ease_linear)),
		  z(iam_ease_preset(iam_ease_linear)), w(iam_ease_preset(iam_ease_linear)) {}

	iam_ease_per_axis(iam_ease_desc all)
		: x(all), y(all), z(all), w(all) {}

	iam_ease_per_axis(iam_ease_desc ex, iam_ease_desc ey)
		: x(ex), y(ey), z(iam_ease_preset(iam_ease_linear)), w(iam_ease_preset(iam_ease_linear)) {}

	iam_ease_per_axis(iam_ease_desc ex, iam_ease_desc ey, iam_ease_desc ez, iam_ease_desc ew)
		: x(ex), y(ey), z(ez), w(ew) {}
};

IM_ANIM_API ImVec2 iam_tween_vec2_per_axis(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_per_axis const& ez, int policy, float dt);
IM_ANIM_API ImVec4 iam_tween_vec4_per_axis(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_per_axis const& ez, int policy, float dt);
IM_ANIM_API ImVec4 iam_tween_color_per_axis(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_per_axis const& ez, int policy, int color_space, float dt);

enum iam_path_segment_type {
	iam_seg_line = 0,
	iam_seg_quadratic_bezier,
	iam_seg_cubic_bezier,
	iam_seg_catmull_rom
};

IM_ANIM_API ImVec2 iam_bezier_quadratic(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t);
IM_ANIM_API ImVec2 iam_bezier_cubic(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t);
IM_ANIM_API ImVec2 iam_catmull_rom(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension = 0.5f);

IM_ANIM_API ImVec2 iam_bezier_quadratic_deriv(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t);
IM_ANIM_API ImVec2 iam_bezier_cubic_deriv(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t);
IM_ANIM_API ImVec2 iam_catmull_rom_deriv(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension = 0.5f);

struct iam_path_data;

class IM_ANIM_API iam_path {
public:
	static iam_path begin(ImGuiID path_id, ImVec2 start);

	iam_path& line_to(ImVec2 end);
	iam_path& quadratic_to(ImVec2 ctrl, ImVec2 end);
	iam_path& cubic_to(ImVec2 ctrl1, ImVec2 ctrl2, ImVec2 end);
	iam_path& catmull_to(ImVec2 end, float tension = 0.5f);
	iam_path& close();

	void end();

	ImGuiID id() const { return m_path_id; }

private:
	iam_path(ImGuiID path_id) : m_path_id(path_id) {}
	ImGuiID m_path_id;
};

IM_ANIM_API bool   iam_path_exists(ImGuiID path_id);
IM_ANIM_API float  iam_path_length(ImGuiID path_id);
IM_ANIM_API ImVec2 iam_path_evaluate(ImGuiID path_id, float t);
IM_ANIM_API ImVec2 iam_path_tangent(ImGuiID path_id, float t);
IM_ANIM_API float  iam_path_angle(ImGuiID path_id, float t);

IM_ANIM_API ImVec2 iam_tween_path(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt);
IM_ANIM_API float  iam_tween_path_angle(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt);

IM_ANIM_API void   iam_path_build_arc_lut(ImGuiID path_id, int subdivisions = 64);
IM_ANIM_API bool   iam_path_has_arc_lut(ImGuiID path_id);

IM_ANIM_API float  iam_path_distance_to_t(ImGuiID path_id, float distance);
IM_ANIM_API ImVec2 iam_path_evaluate_at_distance(ImGuiID path_id, float distance);
IM_ANIM_API float  iam_path_angle_at_distance(ImGuiID path_id, float distance);
IM_ANIM_API ImVec2 iam_path_tangent_at_distance(ImGuiID path_id, float distance);

struct iam_morph_opts {
	int   samples;
	bool  match_endpoints;
	bool  use_arc_length;

	iam_morph_opts() : samples(64), match_endpoints(true), use_arc_length(true) {}
};

IM_ANIM_API ImVec2 iam_path_morph(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());

IM_ANIM_API ImVec2 iam_path_morph_tangent(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());

IM_ANIM_API float  iam_path_morph_angle(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());

IM_ANIM_API ImVec2 iam_tween_path_morph(ImGuiID id, ImGuiID channel_id, ImGuiID path_a, ImGuiID path_b,
                            float target_blend, float dur, iam_ease_desc const& path_ease,
                            iam_ease_desc const& morph_ease, int policy, float dt,
                            iam_morph_opts const& opts = iam_morph_opts());

IM_ANIM_API float  iam_get_morph_blend(ImGuiID id, ImGuiID channel_id);

enum iam_text_path_align {
	iam_text_align_start = 0,
	iam_text_align_center,
	iam_text_align_end
};

struct iam_text_path_opts {
	ImVec2 origin;
	float offset;
	float letter_spacing;
	int   align;
	bool  flip_y;
	ImU32 color;
	ImFont* font;
	float font_scale;

	iam_text_path_opts() : origin(0, 0), offset(0), letter_spacing(0), align(iam_text_align_start),
	                       flip_y(false), color(IM_COL32_WHITE), font(nullptr), font_scale(1.0f) {}
};

IM_ANIM_API void iam_text_path(ImGuiID path_id, const char* text, iam_text_path_opts const& opts = iam_text_path_opts());

IM_ANIM_API void iam_text_path_animated(ImGuiID path_id, const char* text, float progress, iam_text_path_opts const& opts = iam_text_path_opts());

IM_ANIM_API float iam_text_path_width(const char* text, iam_text_path_opts const& opts = iam_text_path_opts());

IM_ANIM_API void iam_transform_quad(ImVec2* quad, ImVec2 center, float angle_rad, ImVec2 translation);

IM_ANIM_API void iam_make_glyph_quad(ImVec2* quad, ImVec2 pos, float angle_rad, float glyph_width, float glyph_height, float baseline_offset = 0.0f);

enum iam_text_stagger_effect {
	iam_text_fx_none = 0,
	iam_text_fx_fade,
	iam_text_fx_scale,
	iam_text_fx_slide_up,
	iam_text_fx_slide_down,
	iam_text_fx_slide_left,
	iam_text_fx_slide_right,
	iam_text_fx_rotate,
	iam_text_fx_bounce,
	iam_text_fx_wave,
	iam_text_fx_typewriter
};

struct iam_text_stagger_opts {
	ImVec2 pos;
	int    effect;
	float  char_delay;
	float  char_duration;
	float  effect_intensity;
	iam_ease_desc ease;
	ImU32  color;
	ImFont* font;
	float  font_scale;
	float  letter_spacing;

	iam_text_stagger_opts()
		: pos(0, 0), effect(iam_text_fx_fade), char_delay(0.05f), char_duration(0.3f),
		  effect_intensity(20.0f), ease(iam_ease_preset(iam_ease_out_cubic)),
		  color(IM_COL32_WHITE), font(nullptr), font_scale(1.0f), letter_spacing(0.0f) {}
};

IM_ANIM_API void iam_text_stagger(ImGuiID id, const char* text, float progress, iam_text_stagger_opts const& opts = iam_text_stagger_opts());

IM_ANIM_API float iam_text_stagger_width(const char* text, iam_text_stagger_opts const& opts = iam_text_stagger_opts());

IM_ANIM_API float iam_text_stagger_duration(const char* text, iam_text_stagger_opts const& opts = iam_text_stagger_opts());

enum iam_noise_type {
	iam_noise_perlin = 0,
	iam_noise_simplex,
	iam_noise_value,
	iam_noise_worley
};

struct iam_noise_opts {
	int   type;
	int   octaves;
	float persistence;
	float lacunarity;
	int   seed;

	iam_noise_opts()
		: type(iam_noise_perlin), octaves(4), persistence(0.5f), lacunarity(2.0f), seed(0) {}
};

IM_ANIM_API float  iam_noise_2d(float x, float y, iam_noise_opts const& opts = iam_noise_opts());
IM_ANIM_API float  iam_noise_3d(float x, float y, float z, iam_noise_opts const& opts = iam_noise_opts());

IM_ANIM_API float  iam_noise_channel_float(ImGuiID id, float frequency, float amplitude, iam_noise_opts const& opts, float dt);
IM_ANIM_API ImVec2 iam_noise_channel_vec2(ImGuiID id, ImVec2 frequency, ImVec2 amplitude, iam_noise_opts const& opts, float dt);
IM_ANIM_API ImVec4 iam_noise_channel_vec4(ImGuiID id, ImVec4 frequency, ImVec4 amplitude, iam_noise_opts const& opts, float dt);
IM_ANIM_API ImVec4 iam_noise_channel_color(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, iam_noise_opts const& opts, int color_space, float dt);

IM_ANIM_API float  iam_smooth_noise_float(ImGuiID id, float amplitude, float speed, float dt);
IM_ANIM_API ImVec2 iam_smooth_noise_vec2(ImGuiID id, ImVec2 amplitude, float speed, float dt);
IM_ANIM_API ImVec4 iam_smooth_noise_vec4(ImGuiID id, ImVec4 amplitude, float speed, float dt);
IM_ANIM_API ImVec4 iam_smooth_noise_color(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float speed, int color_space, float dt);

IM_ANIM_API void iam_style_register(ImGuiID style_id, ImGuiStyle const& style);
IM_ANIM_API void iam_style_register_current(ImGuiID style_id);

IM_ANIM_API void iam_style_blend(ImGuiID style_a, ImGuiID style_b, float t, int color_space = iam_col_oklab);

IM_ANIM_API void iam_style_tween(ImGuiID id, ImGuiID target_style, float duration, iam_ease_desc const& ease, int color_space, float dt);

IM_ANIM_API void iam_style_blend_to(ImGuiID style_a, ImGuiID style_b, float t, ImGuiStyle* out_style, int color_space = iam_col_oklab);

IM_ANIM_API bool iam_style_exists(ImGuiID style_id);

IM_ANIM_API void iam_style_unregister(ImGuiID style_id);

struct iam_gradient {
	ImVector<float> positions;
	ImVector<ImVec4> colors;

	iam_gradient() {}

	iam_gradient& add(float position, ImVec4 color);
	iam_gradient& add(float position, ImU32 color) {
		return add(position, ImGui::ColorConvertU32ToFloat4(color));
	}

	int stop_count() const { return positions.Size; }

	ImVec4 sample(float t, int color_space = iam_col_oklab) const;

	static iam_gradient solid(ImVec4 color);
	static iam_gradient two_color(ImVec4 start, ImVec4 end);
	static iam_gradient three_color(ImVec4 start, ImVec4 mid, ImVec4 end);
};

IM_ANIM_API iam_gradient iam_gradient_lerp(iam_gradient const& a, iam_gradient const& b, float t, int color_space = iam_col_oklab);

IM_ANIM_API iam_gradient iam_tween_gradient(ImGuiID id, ImGuiID channel_id, iam_gradient const& target, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt);

enum iam_rotation_mode {
	iam_rotation_shortest = 0,
	iam_rotation_longest,
	iam_rotation_cw,
	iam_rotation_ccw,
	iam_rotation_direct
};

struct iam_transform {
	ImVec2 position;
	ImVec2 scale;
	float rotation;

	iam_transform() : position(0, 0), scale(1, 1), rotation(0) {}
	iam_transform(ImVec2 pos, float rot = 0, ImVec2 scl = ImVec2(1, 1))
		: position(pos), scale(scl), rotation(rot) {}

	static iam_transform identity() { return iam_transform(); }

	iam_transform operator*(iam_transform const& other) const;

	ImVec2 apply(ImVec2 point) const;

	iam_transform inverse() const;
};

IM_ANIM_API iam_transform iam_transform_lerp(iam_transform const& a, iam_transform const& b, float t, int rotation_mode = iam_rotation_shortest);

IM_ANIM_API iam_transform iam_tween_transform(ImGuiID id, ImGuiID channel_id, iam_transform const& target, float dur, iam_ease_desc const& ez, int policy, int rotation_mode, float dt);

IM_ANIM_API iam_transform iam_transform_from_matrix(float m00, float m01, float m10, float m11, float tx, float ty);

IM_ANIM_API void iam_transform_to_matrix(iam_transform const& t, float* out_matrix);

enum iam_direction {
	iam_dir_normal = 0,
	iam_dir_reverse,
	iam_dir_alternate
};

enum iam_channel_type {
	iam_chan_float = 0,
	iam_chan_vec2,
	iam_chan_vec4,
	iam_chan_int,
	iam_chan_color,
	iam_chan_float_rel,
	iam_chan_vec2_rel,
	iam_chan_vec4_rel,
	iam_chan_color_rel
};

enum iam_result {
	iam_ok = 0,
	iam_err_not_found,
	iam_err_bad_arg,
	iam_err_no_mem
};

struct iam_spring_params {
	float mass;
	float stiffness;
	float damping;
	float initial_velocity;
};

enum iam_variation_mode {
	iam_var_none = 0,
	iam_var_increment,
	iam_var_decrement,
	iam_var_multiply,
	iam_var_random,
	iam_var_random_abs,
	iam_var_pingpong,
	iam_var_callback
};

typedef float  (*iam_variation_float_fn)(int index, void* user);
typedef int    (*iam_variation_int_fn)(int index, void* user);
typedef ImVec2 (*iam_variation_vec2_fn)(int index, void* user);
typedef ImVec4 (*iam_variation_vec4_fn)(int index, void* user);

struct iam_variation_float {
	int                     mode;
	float                   amount;
	float                   min_clamp;
	float                   max_clamp;
	unsigned int            seed;
	iam_variation_float_fn  callback;
	void*                   user;
};

struct iam_variation_int {
	int                   mode;
	int                   amount;
	int                   min_clamp;
	int                   max_clamp;
	unsigned int          seed;
	iam_variation_int_fn  callback;
	void*                 user;
};

struct iam_variation_vec2 {
	int                    mode;
	ImVec2                 amount;
	ImVec2                 min_clamp;
	ImVec2                 max_clamp;
	unsigned int           seed;
	iam_variation_vec2_fn  callback;
	void*                  user;
	iam_variation_float    x, y;
};

struct iam_variation_vec4 {
	int                    mode;
	ImVec4                 amount;
	ImVec4                 min_clamp;
	ImVec4                 max_clamp;
	unsigned int           seed;
	iam_variation_vec4_fn  callback;
	void*                  user;
	iam_variation_float    x, y, z, w;
};

struct iam_variation_color {
	int                    mode;
	ImVec4                 amount;
	ImVec4                 min_clamp;
	ImVec4                 max_clamp;
	int                    color_space;
	unsigned int           seed;
	iam_variation_vec4_fn  callback;
	void*                  user;
	iam_variation_float    r, g, b, a;
};

static inline iam_variation_float iam_varf_none(void) {
	iam_variation_float v = {iam_var_none, 0, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_inc(float amt) {
	iam_variation_float v = {iam_var_increment, amt, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_dec(float amt) {
	iam_variation_float v = {iam_var_decrement, amt, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_mul(float f) {
	iam_variation_float v = {iam_var_multiply, f, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_rand(float r) {
	iam_variation_float v = {iam_var_random, r, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_rand_abs(float r) {
	iam_variation_float v = {iam_var_random_abs, r, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_pingpong(float amt) {
	iam_variation_float v = {iam_var_pingpong, amt, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_fn(iam_variation_float_fn fn, void* user) {
	iam_variation_float v = {iam_var_callback, 0, -FLT_MAX, FLT_MAX, 0, fn, user};
	return v;
}
static inline iam_variation_float iam_varf_clamp(iam_variation_float v, float mn, float mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_float iam_varf_seed(iam_variation_float v, unsigned int s) {
	v.seed = s; return v;
}

static inline iam_variation_int iam_vari_none(void) {
	iam_variation_int v = {iam_var_none, 0, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_inc(int amt) {
	iam_variation_int v = {iam_var_increment, amt, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_dec(int amt) {
	iam_variation_int v = {iam_var_decrement, amt, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_rand(int r) {
	iam_variation_int v = {iam_var_random, r, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_fn(iam_variation_int_fn fn, void* user) {
	iam_variation_int v = {iam_var_callback, 0, INT_MIN, INT_MAX, 0, fn, user};
	return v;
}
static inline iam_variation_int iam_vari_clamp(iam_variation_int v, int mn, int mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_int iam_vari_seed(iam_variation_int v, unsigned int s) {
	v.seed = s; return v;
}

static inline iam_variation_vec2 iam_varv2_none(void) {
	iam_variation_vec2 v = {iam_var_none, {0,0}, {-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_inc(float x, float y) {
	iam_variation_vec2 v = {iam_var_increment, {x,y}, {-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_dec(float x, float y) {
	iam_variation_vec2 v = {iam_var_decrement, {x,y}, {-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_mul(float f) {
	iam_variation_vec2 v = {iam_var_multiply, {f,f}, {-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_rand(float x, float y) {
	iam_variation_vec2 v = {iam_var_random, {x,y}, {-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_fn(iam_variation_vec2_fn fn, void* user) {
	iam_variation_vec2 v = {iam_var_callback, {0,0}, {-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX}, 0, fn, user, {0}, {0}};
	return v;
}

static inline iam_variation_vec2 iam_varv2_axis(iam_variation_float vx, iam_variation_float vy) {
	iam_variation_vec2 v = {iam_var_none, {0,0}, {-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX}, 0, 0, 0, vx, vy};
	return v;
}
static inline iam_variation_vec2 iam_varv2_clamp(iam_variation_vec2 v, ImVec2 mn, ImVec2 mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_vec2 iam_varv2_seed(iam_variation_vec2 v, unsigned int s) {
	v.seed = s; return v;
}

static inline iam_variation_vec4 iam_varv4_none(void) {
	iam_variation_vec4 v = {iam_var_none, {0,0,0,0}, {-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_inc(float x, float y, float z, float w) {
	iam_variation_vec4 v = {iam_var_increment, {x,y,z,w}, {-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_dec(float x, float y, float z, float w) {
	iam_variation_vec4 v = {iam_var_decrement, {x,y,z,w}, {-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_mul(float f) {
	iam_variation_vec4 v = {iam_var_multiply, {f,f,f,f}, {-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_rand(float x, float y, float z, float w) {
	iam_variation_vec4 v = {iam_var_random, {x,y,z,w}, {-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX}, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_fn(iam_variation_vec4_fn fn, void* user) {
	iam_variation_vec4 v = {iam_var_callback, {0,0,0,0}, {-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX}, 0, fn, user, {0}, {0}, {0}, {0}};
	return v;
}

static inline iam_variation_vec4 iam_varv4_axis(iam_variation_float vx, iam_variation_float vy, iam_variation_float vz, iam_variation_float vw) {
	iam_variation_vec4 v = {iam_var_none, {0,0,0,0}, {-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX}, {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX}, 0, 0, 0, vx, vy, vz, vw};
	return v;
}
static inline iam_variation_vec4 iam_varv4_clamp(iam_variation_vec4 v, ImVec4 mn, ImVec4 mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_vec4 iam_varv4_seed(iam_variation_vec4 v, unsigned int s) {
	v.seed = s; return v;
}

static inline iam_variation_color iam_varc_none(void) {
	iam_variation_color v = {iam_var_none, {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_inc(float r, float g, float b, float a) {
	iam_variation_color v = {iam_var_increment, {r,g,b,a}, {0,0,0,0}, {1,1,1,1}, iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_dec(float r, float g, float b, float a) {
	iam_variation_color v = {iam_var_decrement, {r,g,b,a}, {0,0,0,0}, {1,1,1,1}, iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_mul(float f) {
	iam_variation_color v = {iam_var_multiply, {f,f,f,1}, {0,0,0,0}, {1,1,1,1}, iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_rand(float r, float g, float b, float a) {
	iam_variation_color v = {iam_var_random, {r,g,b,a}, {0,0,0,0}, {1,1,1,1}, iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_fn(iam_variation_vec4_fn fn, void* user) {
	iam_variation_color v = {iam_var_callback, {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, iam_col_oklab, 0, fn, user, {0}, {0}, {0}, {0}};
	return v;
}

static inline iam_variation_color iam_varc_channel(iam_variation_float vr, iam_variation_float vg, iam_variation_float vb, iam_variation_float va) {
	iam_variation_color v = {iam_var_none, {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, iam_col_oklab, 0, 0, 0, vr, vg, vb, va};
	return v;
}
static inline iam_variation_color iam_varc_space(iam_variation_color v, int space) {
	v.color_space = space; return v;
}
static inline iam_variation_color iam_varc_clamp(iam_variation_color v, ImVec4 mn, ImVec4 mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_color iam_varc_seed(iam_variation_color v, unsigned int s) {
	v.seed = s; return v;
}

struct iam_clip_data;
struct iam_instance_data;

typedef void (*iam_clip_callback)(ImGuiID inst_id, void* user_data);

typedef void (*iam_loop_callback)(ImGuiID inst_id, int loop_index, void* user_data);

typedef void (*iam_marker_callback)(ImGuiID inst_id, ImGuiID marker_id, float marker_time, void* user_data);

class IM_ANIM_API iam_clip {
public:

	static iam_clip begin(ImGuiID clip_id);

	iam_clip& key_float(ImGuiID channel, float time, float value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec2(ImGuiID channel, float time, ImVec2 value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec4(ImGuiID channel, float time, ImVec4 value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_int(ImGuiID channel, float time, int value, int ease_type = iam_ease_linear);
	iam_clip& key_color(ImGuiID channel, float time, ImVec4 value, int color_space = iam_col_oklab, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);

	iam_clip& key_float_var(ImGuiID channel, float time, float value, iam_variation_float const& var, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec2_var(ImGuiID channel, float time, ImVec2 value, iam_variation_vec2 const& var, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec4_var(ImGuiID channel, float time, ImVec4 value, iam_variation_vec4 const& var, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_int_var(ImGuiID channel, float time, int value, iam_variation_int const& var, int ease_type = iam_ease_linear);
	iam_clip& key_color_var(ImGuiID channel, float time, ImVec4 value, iam_variation_color const& var, int color_space = iam_col_oklab, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);

	iam_clip& key_float_spring(ImGuiID channel, float time, float target, iam_spring_params const& spring);

	iam_clip& key_float_rel(ImGuiID channel, float time, float percent, float px_bias, int anchor_space, int axis, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec2_rel(ImGuiID channel, float time, ImVec2 percent, ImVec2 px_bias, int anchor_space, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec4_rel(ImGuiID channel, float time, ImVec4 percent, ImVec4 px_bias, int anchor_space, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_color_rel(ImGuiID channel, float time, ImVec4 percent, ImVec4 px_bias, int color_space, int anchor_space, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);

	iam_clip& seq_begin();
	iam_clip& seq_end();
	iam_clip& par_begin();
	iam_clip& par_end();

	iam_clip& marker(float time, ImGuiID marker_id, iam_marker_callback cb, void* user = nullptr);
	iam_clip& marker(float time, iam_marker_callback cb, void* user = nullptr);

	iam_clip& set_loop(bool loop, int direction = iam_dir_normal, int loop_count = -1);
	iam_clip& set_loop_delay(float delay_seconds);
	iam_clip& set_delay(float delay_seconds);
	iam_clip& set_stagger(int count, float each_delay, float from_center_bias = 0.0f);
	iam_clip& set_stagger_ease(int ease_type);

	iam_clip& set_duration_var(iam_variation_float const& var);
	iam_clip& set_delay_var(iam_variation_float const& var);
	iam_clip& set_timescale_var(iam_variation_float const& var);

	iam_clip& on_begin(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_update(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_complete(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_loop(iam_loop_callback cb, void* user = nullptr);
	iam_clip& on_pause(iam_clip_callback cb, void* user = nullptr);

	void end();

	ImGuiID id() const { return m_clip_id; }

private:
	iam_clip(ImGuiID clip_id) : m_clip_id(clip_id) {}
	ImGuiID m_clip_id;
};

class IM_ANIM_API iam_instance {
public:
	iam_instance() : m_inst_id(0) {}
	iam_instance(ImGuiID inst_id) : m_inst_id(inst_id) {}

	void pause();
	void resume();
	void stop();
	void restart();
	void reset();
	void refresh();
	void destroy();
	void seek(float time);
	void set_time_scale(float scale);
	void set_weight(float weight);

	iam_instance& then(ImGuiID next_clip_id);
	iam_instance& then(ImGuiID next_clip_id, ImGuiID next_instance_id);
	iam_instance& then_delay(float delay);

	float time() const;
	float duration() const;
	bool is_playing() const;
	bool is_paused() const;

	bool get_float(ImGuiID channel, float* out) const;
	bool get_vec2(ImGuiID channel, ImVec2* out) const;
	bool get_vec4(ImGuiID channel, ImVec4* out) const;
	bool get_int(ImGuiID channel, int* out) const;
	bool get_color(ImGuiID channel, ImVec4* out, int color_space = iam_col_oklab) const;

	bool valid() const;
	operator bool() const { return valid(); }

	ImGuiID id() const { return m_inst_id; }

private:
	ImGuiID m_inst_id;
};

IM_ANIM_API void iam_clip_init(int initial_clip_cap = 256, int initial_inst_cap = 4096);
IM_ANIM_API void iam_clip_shutdown();

IM_ANIM_API void iam_clip_update(float dt);

IM_ANIM_API void iam_clip_gc(unsigned int max_age_frames = 600);

IM_ANIM_API iam_instance iam_play(ImGuiID clip_id, ImGuiID instance_id);

IM_ANIM_API iam_instance iam_get_instance(ImGuiID instance_id);

IM_ANIM_API float iam_clip_duration(ImGuiID clip_id);
IM_ANIM_API bool iam_clip_exists(ImGuiID clip_id);

IM_ANIM_API float iam_stagger_delay(ImGuiID clip_id, int index);
IM_ANIM_API iam_instance iam_play_stagger(ImGuiID clip_id, ImGuiID instance_id, int index);

enum iam_stagger_from {
	iam_stagger_first  = 0,
	iam_stagger_last,
	iam_stagger_center,
	iam_stagger_index
};

enum iam_stagger_axis {
	iam_stagger_both = 0,
	iam_stagger_x,
	iam_stagger_y
};

struct iam_stagger_grid_opts {
	int   cols;
	int   rows;
	int   from;
	int   from_index;
	int   axis;
	float delay;
	int   ease;
	float start_delay;

	iam_stagger_grid_opts() : cols(1), rows(1), from(iam_stagger_first),
	                          from_index(0), axis(iam_stagger_both),
	                          delay(0.05f), ease(iam_ease_linear), start_delay(0) {}
};

IM_ANIM_API float iam_stagger_grid_delay(int col, int row, iam_stagger_grid_opts const& opts);

IM_ANIM_API float iam_stagger_grid_delay_index(int index, iam_stagger_grid_opts const& opts);

IM_ANIM_API iam_instance iam_play_with_delay(ImGuiID clip_id, ImGuiID instance_id, float delay);

IM_ANIM_API void iam_layer_begin(ImGuiID instance_id);
IM_ANIM_API void iam_layer_add(iam_instance inst, float weight);
IM_ANIM_API void iam_layer_end(ImGuiID instance_id);
IM_ANIM_API bool iam_get_blended_float(ImGuiID instance_id, ImGuiID channel, float* out);
IM_ANIM_API bool iam_get_blended_vec2(ImGuiID instance_id, ImGuiID channel, ImVec2* out);
IM_ANIM_API bool iam_get_blended_vec4(ImGuiID instance_id, ImGuiID channel, ImVec4* out);
IM_ANIM_API bool iam_get_blended_int(ImGuiID instance_id, ImGuiID channel, int* out);

IM_ANIM_API iam_result iam_clip_save(ImGuiID clip_id, char const* path);
IM_ANIM_API iam_result iam_clip_load(char const* path, ImGuiID* out_clip_id);
