#pragma once
#define NOMINMAX
#include <Windows.h>
#include "keybind/keybind.h"
#include <vector>
#include <string>

namespace settings
{
	namespace visuals
	{
		inline bool enable_enemies{ false };

		inline bool box{ false };
		inline int  box_type{ 0 };

		inline int  box_mode{ 0 };
		inline float box_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool  engine_chams{ false };
		inline int   engine_chams_style{ 0 };
		inline float engine_chams_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool box_fill{ false };

		inline int  box_fill_mode{ 0 };
		inline char box_fill_image_path[260]{};
		inline bool  box_fill_image_loaded{ false };
		inline float box_fill_image_alpha{ 0.5f };
		inline bool box_fill_remove_bg{ false };
		inline float box_fill_color[4]{ 0.2f, 0.2f, 0.2f, 0.3f };

		inline bool box_gradient{ false };
		inline float box_gradient_top[4]{ 1.f, 1.f, 1.f, 0.15f };
		inline float box_gradient_bottom[4]{ 1.f, 1.f, 1.f, 0.05f };

		inline bool name{ false };

		inline int  name_display_type{ 0 };
		inline float name_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float name_color_blend_start[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float name_color_blend_end[4]{ 0.f, 0.f, 1.f, 1.f };
		inline bool blend{ false };

		inline bool avatar{ false };		inline bool  distance{ false };
		inline int  distance_measurement{ 0 };
		inline float distance_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float max_distance{ 0.f };

		inline bool  offscreen_arrows{ false };
		inline float arrow_color[4]{ 1.f, 1.f, 1.f, 0.95f };
		inline float arrow_size{ 14.f };
		inline float arrow_radius{ 200.f };
		inline bool  arrow_info[4]{ true, false, false, false };

		inline bool  tracer{ false };
		inline int   tracer_origin{ 0 };
		inline float tracer_color[4]{ 1.f, 1.f, 1.f, 0.85f };

		inline bool  trail{ false };
		inline int   trail_style{ 3 };
		inline int   trail_color_mode{ 0 };
		inline float trail_color[4]{ 0.42f, 0.78f, 1.f, 0.90f };
		inline float trail_head_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float trail_thickness{ 2.6f };
		inline float trail_duration{ 1.30f };
		inline int   trail_max_points{ 64 };
		inline float trail_update_interval{ 0.025f };
		inline float trail_min_movement{ 0.16f };
		inline bool  trail_only_moving{ true };
		inline int   trail_follow{ 0 };
		inline bool  trail_taper{ true };
		inline bool  trail_glow{ true };
		inline float trail_glow_intensity{ 0.60f };
		inline int   trail_glow_layers{ 3 };
		inline bool  trail_fade{ true };
		inline float trail_fade_start{ 0.40f };
		inline bool  trail_smooth{ true };
		inline int   trail_spline_segments{ 5 };
		inline bool  trail_head_spark{ true };
		inline bool  trail_height_fall{ true };
		inline float trail_height_drop{ 0.35f };
		inline bool  trail_team_check{ false };

		inline bool  name_sheen{ false };
		inline float name_sheen_speed{ 0.55f };
		inline float name_sheen_width{ 0.34f };
		inline float name_sheen_strength{ 0.85f };
		inline bool  esp_sheen_all{ false };

		inline bool  china_hat{ false };
		inline float china_hat_color[4]{ 1.f, 1.f, 1.f, 0.42f };
		inline float china_hat_height{ 1.0f };
		inline float china_hat_radius{ 1.5f };

		inline bool  body_corpse{ false };
		inline float corpse_color[4]{ 1.f, 1.f, 1.f, 0.90f };

		inline bool  health_text{ false };
		inline float health_text_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline float esp_font_size{ 13.f };

		inline float box_thickness{ 1.f };

		inline bool  distance_check{ false };
		inline int   distance_unit{ 0 };

		inline float aim_target_color[4]{ 0.f, 0.f, 0.f, 1.f };
		inline float friend_color[4]{ 0.42f, 0.85f, 0.46f, 1.f };

		inline bool  aim_line{ false };
		inline int   aim_line_style{ 0 };
		inline float aim_line_color[4]{ 1.f, 1.f, 1.f, 0.90f };
		inline float aim_line_thickness{ 1.5f };
		inline bool  aim_line_centre{ true };

		inline float healthbar_width{ 6.f };
		inline bool  healthbar_glow    { false };

		inline float healthbar_opacity{ 100.f };

		inline bool  healthbar_enhanced{ true };

		inline bool tool{ false };
		inline float tool_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool rigtype{ false };
		inline bool state{ false };

		inline bool flags{ false };
		inline float flags_state_colour[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool healthbar{ false };
		inline float healthbar_color[4]{ 0.f, 1.f, 0.f, 1.f };
		inline bool health_based_healthbar{ false };

		inline bool health_percent{ false };
		inline float health_percent_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float healthbar_size{ 4.0f };
		inline float healthbar_padding{ 1.0f };

		inline bool box_body_only{ true };

		inline bool skeleton{ false };

		inline bool  skeleton_animate{ true };
		inline float skeleton_swing{ 1.f };
		inline bool skeleton_outline{ false };
		inline float skeleton_thickness{ 1.5f };
		inline float skeleton_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float skeleton_outline_color[4]{ 0.f, 0.f, 0.f, 1.f };

		inline bool  chams{ false };
		inline float chams_fill_color[4]{ 1.f, 0.f, 0.f, 1.f };

		inline bool  nchams{ false };
		inline int   nchams_mode{ 0 };
		inline float nchams_outline_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float nchams_fill_color[4]{ 1.f, 0.25f, 0.25f, 0.55f };
		inline int   nchams_shader_style{ 0 };

		inline bool  nchams_glow{ false };
		inline float nchams_glow_opacity{ 45.f };
		inline float nchams_glow_size{ 3.f };

		inline bool  nchams_inner_fill{ false };
		inline float nchams_inner_fill_scale{ 45.f };
		inline int   mesh_shader_mode{ 0 };
		inline float mesh_occluded_color[4]{ 0.f, 1.f, 0.f, 1.f };

		inline int esp_font{ 7 };

		inline float fade_in_speed{ 5.0f };
		inline float fade_out_speed{ 5.0f };

		inline bool localplayer{ false };
		inline bool knock_check{ false };

		inline bool team_check{ true };
		inline bool dead_check{ true };

		inline bool visible_check{ false };

		inline bool view_hitbox{ false };
		inline float view_hitbox_color[4]{ 1.f, 0.f, 0.f, 1.f };

		inline bool hit_skeleton{ false };
		inline float hit_skeleton_color[4]{ 1.f, 0.2f, 0.f, 1.f };
		inline float hit_skeleton_duration{ 1.5f };

		inline bool hit_chams{ false };
		inline float hit_chams_color[4]{ 1.f, 0.1f, 0.f, 0.6f };
		inline float hit_chams_duration{ 1.5f };

		inline bool  hit_freeze{ false };

		inline bool  movement_status{ false };

		inline bool  radar_enabled{ false };
		inline float radar_size{ 130.f };
		inline int   radar_range{ 180 };
		inline int   radar_position{ 0 };
		inline int   radar_shape{ 0 };
		inline float radar_opacity{ 0.86f };
		inline bool  radar_rotate{ true };
		inline bool  radar_grid{ true };
		inline bool  radar_labels{ true };
		inline bool  radar_sweep{ true };
		inline bool  radar_names{ true };
		inline bool  radar_health{ true };
		inline bool  radar_dead{ false };
		inline bool  radar_show_walls{ true };
		inline float radar_wall_color[4]{ 0.43f, 0.46f, 0.52f, 0.69f };
		inline float radar_local_color[4]{ 0.31f, 0.78f, 1.f, 1.f };
		inline float radar_team_color[4]{ 0.42f, 0.85f, 0.50f, 1.f };
		inline float radar_enemy_color[4]{ 1.f, 0.31f, 0.31f, 1.f };

		inline bool  head_dot{ false };
		inline float head_dot_size{ 4.f };
		inline float head_dot_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool  head_dot_outline{ true };
		inline float head_dot_outline_color[4]{ 0.f, 0.f, 0.f, 1.f };

		inline bool  look_direction{ false };
		inline float look_direction_length{ 1.6f };
		inline float look_direction_thickness{ 1.6f };
		inline float look_direction_color[4]{ 1.f, 1.f, 0.35f, 1.f };

	}

	namespace effects
	{
		inline bool  hitmarker{ false };
		inline float hitmarker_size{ 1.0f };
		inline float hitmarker_duration{ 0.55f };
		inline float hitmarker_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float hitmarker_head_color[4]{ 1.f, 0.176f, 0.176f, 1.f };

		inline int   hitmarker_style{ 0 };

		inline bool  hitmarker_shockwave{ true };
		inline float hitmarker_shockwave_size{ 1.f };

		inline bool  hitmarker_sparks{ true };
		inline int   hitmarker_spark_count{ 10 };
		inline float hitmarker_spark_speed{ 1.f };

		inline bool  hitmarker_flash{ true };

		inline bool  hitmarker_streak{ false };
		inline float hitmarker_streak_window{ 1.6f };

		inline bool  hitmarker_crit_pulse{ true };

		inline float hitmarker_screen_flash{ 0.f };

		inline bool  hitmarker_outline{ true };

		inline float hitmarker_glow{ 0.55f };

		inline float hitmarker_pop{ 1.f };

		inline bool  hitdata{ false };
		inline bool  hitdata_damage{ true };
		inline bool  hitdata_health{ false };
		inline bool  hitdata_distance{ false };
		inline float hitdata_duration{ 1.35f };
		inline float hitdata_size{ 15.f };

		inline bool  hitsound{ false };
		inline int   hitsound_index{ 0 };
		inline float hitsound_volume{ 100.f };

		inline bool  crosshair{ false };
		inline bool  crosshair_lock{ false };
		inline float crosshair_length{ 8.f };

		inline float crosshair_len_x{ 1.f };
		inline float crosshair_len_y{ 1.f };

		inline float crosshair_taper{ 0.f };
		inline float crosshair_gap{ 4.f };
		inline float crosshair_thickness{ 1.f };

		inline bool  crosshair_spin{ false };
		inline float crosshair_spin_speed{ 90.f };

		inline bool  crosshair_spin_windup{ false };
		inline bool  crosshair_outline{ true };
		inline float crosshair_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float crosshair_outline_color[4]{ 0.f, 0.f, 0.f, 0.85f };
		inline bool  crosshair_dot{ false };
		inline float crosshair_dot_size{ 1.5f };

		inline int   crosshair_style{ 0 };

		inline float crosshair_glow{ 0.f };

		inline bool  crosshair_rainbow{ false };
		inline float crosshair_rainbow_speed{ 0.35f };

		inline bool  crosshair_hit_pulse{ true };
		inline float crosshair_hit_pulse_size{ 1.f };

		inline bool  crosshair_dynamic{ false };
		inline float crosshair_dynamic_amount{ 1.f };

		inline bool  crosshair_stretch{ false };
		inline float crosshair_stretch_amount{ 0.45f };
		inline float crosshair_stretch_speed{ 1.6f };
	}

	namespace sonar
	{
		inline bool  enabled{ false };
		inline int   keybind{ 0 };
		inline int   activation_mode{ 1 };

		inline int   style{ 0 };

		inline float range{ 600.f };
		inline float sweep_speed{ 60.f };
		inline bool  sweep_back{ false };
		inline float beam_width{ 1.6f };
		inline float color[4]{ 0.42f, 0.92f, 1.f, 0.95f };
		inline float ping_color[4]{ 1.f, 0.55f, 0.25f, 1.f };
		inline bool  glow{ true };
		inline float glow_strength{ 0.7f };

		inline bool  ping_rings{ true };
		inline float ping_life{ 1.1f };
		inline float ping_radius{ 22.f };

		inline int   info_mode{ 2 };
		inline float info_size{ 12.f };

		inline bool  latch{ true };
		inline float hold_time{ 1.4f };

		inline bool  ping_sound{ false };
		inline float ping_volume{ 60.f };

		inline float beam_reach{ 1.f };
		inline float beam_taper{ 0.35f };

		inline float tail_length{ 26.f };
		inline float tail_strength{ 0.6f };

		inline bool  grid{ true };
		inline int   grid_rings{ 3 };
		inline float grid_alpha{ 0.22f };
		inline bool  grid_labels{ false };

		inline int   ping_style{ 0 };
		inline bool  ping_ticks{ true };
		inline bool  ping_spark{ true };
		inline bool  ping_drop{ false };
		inline bool  readout_plate{ true };
		inline bool  los_only{ false };

		inline int   color_mode{ 0 };
		inline float hue_speed{ 0.06f };

		inline bool  hide_with_menu{ true };
	}

	namespace misc_ui
	{
		inline bool  enabled{ true };
		inline float intensity{ 1.f };

		inline bool  cursor_aura{ false };
		inline float cursor_aura_size{ 1.f };
		inline bool  bloom_soft{ true };
		inline bool  cursor_trail{ true };
		inline float cursor_trail_length{ 1.f };
		inline bool  click_shockwave{ true };

		inline int   click_style{ 2 };

		inline bool  click_spark{ false };

		inline bool  hover_sheen{ false };
		inline float hover_sheen_speed{ 1.f };

		inline float hover_sheen_strength{ 0.18f };
		inline bool  sheen_soft{ true };

		inline int   tint{ 0 };
		inline bool  hover_tint{ true };
		inline bool  hover_lift{ true };
		inline bool  press_squish{ true };
		inline bool  focus_ring{ true };
		inline bool  chip_pop{ true };
		inline bool  toggle_overshoot{ true };

		inline bool  panel_rim{ true };
		inline float panel_rim_size{ 1.f };
		inline bool  nav_breath{ true };
		inline bool  indicator_glow{ true };
		inline bool  stagger{ true };
		inline float stagger_step{ 0.022f };

		inline int   cascade_mode{ 0 };

		inline int   cascade_mode_view{ 1 };
		inline bool  cascade_pages{ true };
		inline bool  cascade_views{ true };
		inline bool  cascade_hairline{ true };
		inline float cascade_time{ 0.f };
		inline float cascade_rise{ 14.f };
		inline float cascade_drift{ 22.f };

		inline bool  slider_hd{ false };
		inline float slider_hd_size{ 1.4f };
		inline bool  tooltip_fade{ true };
		inline bool  edge_fade{ true };

		inline int   menu_font{ 0 };

		inline bool  first_pop{ true };
		inline bool  open_flash{ false };
		inline float open_flash_strength{ 0.6f };
	}

	namespace ui21
	{
		inline bool  enabled{ false };
		inline bool  glass_layers{ true };
		inline float glass_depth{ 1.f };
		inline bool  spotlight_border{ true };
		inline float spotlight_size{ 1.f };
		inline bool  gradient_heading{ true };
		inline bool  odometer{ true };
		inline bool  dot_grid{ false };
		inline float dot_grid_spacing{ 22.f };
		inline float accent_2[4]{ 0.62f, 0.40f, 1.f, 1.f };
		inline bool  noise{ true };
		inline float noise_amount{ 0.5f };
	}

	namespace home
	{
		inline bool  greeting{ true };
		inline bool  clock{ true };
		inline int   clock_format{ 1 };
		inline bool  avatar{ true };
		inline bool  avatar_ring{ true };
		inline bool  avatar_glow{ true };
		inline bool  cards{ true };
		inline bool  card_icons{ true };
		inline bool  activity{ true };
		inline bool  activity_fill{ true };
		inline bool  activity_smooth{ true };
		inline bool  activity_grid{ true };
		inline bool  activity_hover{ true };
		inline bool  nearby{ true };
		inline int   nearby_rows{ 7 };
		inline bool  nearby_faces{ true };
		inline bool  nearby_bars{ true };
		inline bool  count_up{ true };
		inline float stagger{ 0.055f };
	}

	namespace docks
	{

		enum Id : int { Session = 0, Nearby, Sonar, Count };
		inline constexpr int kCount = (int)Count;

		inline bool  enabled{ true };
		inline bool  open[kCount]   { false, false, false };

		inline float x[kCount]      { 260.f, 16.f, 300.f };
		inline float y[kCount]      { 232.f, 232.f, 232.f };
		inline bool  folded[kCount] { false, false, false };

		inline float opacity{ 1.f };
		inline float scale{ 1.f };
		inline bool  shadow{ true };
		inline bool  snap{ true };
		inline float snap_px{ 20.f };
		inline float snap_gap{ 10.f };
		inline bool  hover_lift{ true };
		inline bool  fold_on_click{ true };
		inline bool  animate{ true };
		inline float anim_speed{ 1.f };
		inline bool  show_titles{ true };
		inline bool  show_grip{ true };
		inline bool  behind_menu{ true };
	}

	namespace viz
	{
		inline bool  enabled{ false };

		inline int   mode{ 1 };
		inline int   window{ 1 };
		inline bool  grid{ true };
		inline bool  glow{ true };
		inline float thickness{ 1.8f };
		inline float scale{ 1.f };
		inline float smooth_color[4]{ 0.42f, 0.86f, 1.f, 1.f };
		inline float raw_color[4]{ 1.f, 0.62f, 0.32f, 0.9f };
		inline float error_color[4]{ 1.f, 0.34f, 0.42f, 1.f };
		inline bool  band{ true };
		inline bool  readouts{ true };
		inline bool  hover_value{ true };

		inline bool  demo_switch{ true };
		inline float demo_speed{ 2.6f };
		inline float demo_x{ 6.f };
		inline float demo_y{ 6.f };
		inline int   pin_style{ -1 };
	}

	namespace aimbot
	{
		inline bool enabled{ false };
		inline int  keybind{ 0 };
		inline int  activation_mode{ 1 };

		inline int  mode{ 0 };

		inline int  target_part{ 1 };
		inline bool air_part_enabled{ false };
		inline int  air_part{ 1 };

		inline float fov{ 150.f };
		inline bool  use_fov{ false };
		inline bool  draw_fov{ false };
		inline float fov_circle_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_outline_colour[4]{ 0.f, 0.f, 0.f, 1.f };

		inline bool  fov_fill{ false };
		inline float fov_fill_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_fill_opacity{ 12.f };

		inline bool  smoothing{ false };
		inline float smoothingx{ 10.f };
		inline float smoothingy{ 10.f };
		inline int   smoothing_style{ 0 };

		inline bool  instant_lock{ false };

		inline int   pace_ms{ 0 };

		inline bool  enable_prediction{ false };
		inline float prediction_x{ 10.f };
		inline float prediction_y{ 10.f };
		inline bool  air_prediction_enabled{ false };
		inline float air_prediction_x{ 10.f };
		inline float air_prediction_y{ 10.f };

		inline bool teamcheck{ false };
		inline bool knock_check{ false };
		inline bool dead_check{ true };
		inline bool visible_check{ false };
		inline bool sticky_aim{ false };

		inline bool  health_check_enabled{ false };
		inline float min_health{ 0.f };

		inline bool  offset_enabled{ false };
		inline float offset_x{ 0.f };
		inline float offset_y{ 0.f };

		inline bool  tracer{ false };
		inline int   tracer_origin{ 0 };
		inline float tracer_color[4]{ 1.f, 1.f, 1.f, 0.85f };

		inline int   priority{ 0 };
	}

	namespace silentaim
	{
		inline bool  enabled{ false };
		inline int   keybind{ 0 };
		inline int   activation_mode{ 1 };

		inline int   method{ 0 };

		inline int   target_part{ 1 };
		inline float fov{ 100.f };
		inline bool  use_fov{ false };
		inline bool  draw_fov{ false };

		inline bool  fov_fill{ false };
		inline float fov_fill_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_fill_opacity{ 12.f };

		inline bool  enable_prediction{ false };
		inline float prediction_x{ 10.f };
		inline float prediction_y{ 10.f };

		inline bool  sticky_aim{ false };
		inline bool  use_aimbot_target{ false };
		inline bool  spoof_mouse{ true };

		inline bool  teamcheck{ false };
		inline bool  guncheck{ false };
		inline bool  knock_check{ false };
		inline bool  dead_check{ true };
		inline bool  visible_check{ false };

		inline int   priorities{ 0 };
		inline bool  health_check_enabled{ false };
		inline float min_health{ 0.f };

		inline bool  draw_target_dot{ false };
		inline float target_dot_color[4]{ 1.f, 0.f, 0.f, 1.f };
		inline float target_dot_size{ 4.f };

		inline float fov_circle_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_outline_colour[4]{ 0.f, 0.f, 0.f, 1.f };

		inline bool  hit_chance_enabled{ false };
		inline float hit_chance{ 100.f };
		inline float hit_chance_fire_rate{ 10.f };

		inline bool  draw_curve_tracer{ false };
		inline float curve_tracer_color[4]{ 0.f, 1.f, 0.5f, 1.f };
		inline float curve_tracer_thickness{ 2.f };
	}

	namespace hitbox_expander
	{
		inline bool  enabled{ false };
		inline float size_x{ 2.2f };
		inline float size_y{ 2.2f };
		inline float size_z{ 1.2f };
		inline bool  knock_check{ false };
	}

	namespace triggerbot
	{
		inline bool  enabled{ false };
		inline int   keybind{ 0 };
		inline int   activation_mode{ 2 };
		inline int   target_part{ 0 };
		inline float delay_ms{ 80.f };
		inline float fov{ 20.f };
		inline bool  draw_fov{ false };
		inline float fov_circle_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_outline_colour[4]{ 0.f, 0.f, 0.f, 1.f };

		inline bool  fov_fill{ false };
		inline float fov_fill_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_fill_opacity{ 12.f };
		inline bool  continuous{ false };
		inline bool  visibility_check{ false };
		inline bool  gun_check{ false };
		inline bool  knock_check{ false };
		inline bool  team_check{ false };

		inline bool  dead_check{ true };
	}

	namespace spin360
	{
		inline bool  enabled{ false };
		inline int   keybind{ 0 };
		inline int   activation_mode{ 1 };
		inline float speed{ 10.0f };
	}

	namespace walkspeed
	{
		inline bool enabled{ false };
		inline float speed{ 16.0f };
		inline int keybind{ 0 };
		inline int keybind_mode{ 0 };
	}

	namespace movement
	{
		namespace speedhack
		{
			inline bool enabled{ false };
			inline int  mode{ 0 };
			inline float speed{ 50.f };
			inline int  keybind{ 0 };
			inline int  activation_mode{ 0 };
		}

		namespace flyhack
		{
			inline bool enabled{ false };
			inline int  mode{ 0 };
			inline float speed{ 50.f };
			inline int  keybind{ 0 };
			inline int  activation_mode{ 0 };
		}

		namespace tickrate
		{
			inline bool  enabled{ false };
			inline float value{ 240.f };
		}
	}

	namespace rage
	{
		inline bool rapidfire{ false };
	}

	namespace spinbot
	{
		inline bool enabled{ false };
	}

	namespace noclip
	{
		inline bool enabled{ false };
		inline int  keybind{ 0 };
		inline int  activation_mode{ 0 };
	}

	namespace nojumpcooldown
	{
		inline bool enabled{ false };
	}

	namespace streamproof
	{
		inline bool enabled{ false };
	}

	namespace mouse_spoof
	{
		inline bool enabled{ false };
	}

	namespace silentaim_autopred
	{
		inline bool enabled{ false };
		inline float strength{ 1.0f };
	}

	namespace op1
	{
		inline bool enable{ true };
		inline bool force{ false };

		inline bool part_aliases{ true };
		inline bool animated_body{ true };
		inline bool collision_boxes{ false };

		inline bool engine_matrix{ true };
		inline bool multipoint_los{ true };
		inline bool live_health{ true };
		inline bool no_smoothing{ true };

		inline int  sticky_ms{ 2500 };
	}

	namespace world
	{

		namespace lighting
		{

			inline bool  time_on{ false };
			inline float clock_time{ 14.f };

			inline bool  brightness_on{ false };
			inline float brightness{ 2.0f };

			inline bool  exposure_on{ false };
			inline float exposure_compensation{ 0.0f };

			inline bool  ambient_on{ false };
			inline float ambient[3]{ 0.7f, 0.7f, 0.7f };

			inline bool  outdoor_on{ false };
			inline float outdoor_ambient[3]{ 0.7f, 0.7f, 0.7f };

			inline bool  fog_on{ false };
			inline float fog_color[3]{ 0.75f, 0.75f, 0.75f };
			inline float fog_start{ 0.f };
			inline float fog_end{ 1000.f };

			inline bool  shift_on{ false };
			inline float color_shift_top[3]{ 0.f, 0.f, 0.f };
			inline float color_shift_bottom[3]{ 0.f, 0.f, 0.f };

			inline bool  env_on{ false };
			inline float env_diffuse_scale{ 1.0f };
			inline float env_specular_scale{ 1.0f };

			inline bool  no_shadow{ false };

			inline bool  light_on{ false };
			inline float light_color[3]{ 1.f, 1.f, 1.f };
			inline float light_dir[3]{ 0.f, -1.f, 0.f };
		}

		namespace atmosphere
		{
			inline bool  enabled{ false };
			inline float density{ 0.3f };
			inline float offset{ 0.f };
			inline float haze{ 0.f };
			inline float glare{ 0.f };
			inline float color[3]{ 0.78f, 0.83f, 0.90f };
			inline float decay[3]{ 0.30f, 0.35f, 0.40f };
		}

		namespace postfx
		{
			namespace bloom
			{
				inline bool  enabled{ false };
				inline float intensity{ 0.9f };
				inline float size{ 24.f };
				inline float threshold{ 0.95f };
			}
			namespace colorcorrect
			{
				inline bool  enabled{ false };
				inline float tint[3]{ 1.f, 1.f, 1.f };
				inline float brightness{ 0.f };
				inline float contrast{ 0.f };
			}
			namespace dof
			{
				inline bool  enabled{ false };
				inline float focus_distance{ 50.f };
				inline float in_focus_radius{ 10.f };
				inline float far_intensity{ 0.5f };
				inline float near_intensity{ 0.f };
			}

			namespace colorgrade
			{
				inline bool enabled{ false };
				inline int  tonemapper{ 0 };
			}

			namespace sunrays
			{
				inline bool  enabled{ false };
				inline float intensity{ 0.25f };
				inline float spread{ 1.0f };
			}
		}

		namespace shaders
		{
			inline bool  vignette{ false };
			inline float vignette_strength{ 0.6f };
			inline float vignette_radius{ 0.75f };

			inline bool  chromatic_aberration{ false };
			inline float chromatic_strength{ 2.5f };

			inline bool  film_grain{ false };
			inline float film_grain_strength{ 0.08f };

			inline bool  glossy{ false };
			inline float glossy_specular{ 2.0f };
			inline float glossy_diffuse{ 0.8f };
			inline bool  glossy_shimmer{ true };

			inline bool  contrast{ false };
			inline float contrast_value{ 1.3f };
			inline float brightness_value{ 0.0f };
		}

		namespace presets
		{
			inline bool night{ false };
			inline bool sunset{ false };
			inline bool rainbow{ false };
			inline bool fog_pulse{ false };
		}

		namespace effects_3d
		{

			inline int   snow_mode{ 0 };
			inline float snow_intensity{ 1.f };
			inline float snow_color[3]{ 0.90f, 0.95f, 1.00f };
			inline float snow_speed{ 1.f };

			inline int   rain_mode{ 0 };
			inline float rain_intensity{ 1.f };
			inline float rain_color[3]{ 0.78f, 0.85f, 1.00f };
			inline float rain_speed{ 1.f };

			inline float wind_x{ 0.f };
			inline float wind_z{ 0.f };

			inline bool  ripples{ true };
			inline float ripple_size{ 1.f };

			inline bool  rain_sound{ false };
			inline float rain_volume{ 0.7f };

			inline int   storm_mode{ 0 };
		}

		namespace sky_slots
		{
			inline bool  enabled{ false };
			inline float sun_angular{ 21.f };
			inline float moon_angular{ 11.f };
			inline float orientation[3]{ 0.f, 0.f, 0.f };
		}

		namespace terrain
		{
			inline bool  enabled{ false };
			inline float grass_length{ 0.2f };
			inline float grass_color[3]{ 1.f, 1.f, 1.f };
			inline float water_color[3]{ 1.f, 1.f, 1.f };
			inline float water_reflectance{ 1.f };
			inline float water_transparency{ 0.3f };
		}
	}

	namespace skybox
	{

		inline bool  enabled{ false };

		inline int   shader{ 0 };

		inline int   video{ 0 };
		inline char  video_file[260]{};
	}

	namespace exploits
	{
		inline bool  low_gravity{ false };
		inline float gravity_value{ 8.f };

		inline bool  tp_cursor{ false };
		inline int   tp_cursor_key{ 'F' };
		inline float tp_cursor_dist{ 80.f };
	}

	namespace theme
	{

		inline int index{ 4 };

		inline float accent[4]{ 1.f, 1.f, 1.f, 1.f };

		inline float bg_one[4]{ 0.056f, 0.056f, 0.067f, 1.f };
		inline float bg_two[4]{ 0.079f, 0.079f, 0.094f, 1.f };

		inline float stroke[4]{ 0.158f, 0.158f, 0.181f, 1.f };
		inline float outline[4]{ 0.141f, 0.141f, 0.188f, 1.f };

		inline float text_active[4]{ 0.784f, 0.784f, 0.784f, 1.f };
		inline float text_inactive[4]{ 0.533f, 0.533f, 0.533f, 1.f };
	}

	namespace appearance
	{

		inline bool  custom_accent{ false };
		inline float accent[4]{ 0.235f, 0.510f, 1.000f, 1.f };

		inline bool  window_gradient{ true };
		inline bool  window_shadow{ true };
		inline bool  accent_glow{ true };
		inline float glow_strength{ 0.65f };
		inline float rounding{ 8.f };

		inline float panel_alpha{ 0.86f };
		inline float dock_alpha{ 0.78f };

		inline float glass_frost{ 0.30f };

		inline bool  nav_labels{ true };

		inline bool  smooth_scroll{ true };
		inline float scroll_speed{ 14.f };
		inline bool  page_animation{ true };
		inline bool  widget_animations{ true };

		inline bool  switches{ true };

		inline int   settings_tab{ 0 };

		inline bool  bg_enabled{ true };
		inline int   bg_effect{ 1 };
		inline float bg_density{ 0.7f };
		inline float bg_speed{ 1.0f };
		inline float bg_size{ 1.0f };
		inline float bg_opacity{ 0.85f };
		inline float bg_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float bg_wind{ 0.20f };
		inline float bg_glow{ 0.55f };
		inline bool  bg_parallax{ true };
		inline bool  bg_menu_only{ false };
	}

	namespace menu
	{
		inline bool vsync{ false };
		inline bool performance_mode{ false };

		inline int fps_limit{ 240 };

		inline bool hide_watermark{ false };
		inline bool ignore_unsupported_place{ false };
	}

	namespace settings
	{
		inline bool hide_console{ false };
		inline bool should_unload{ false };
	}
}
