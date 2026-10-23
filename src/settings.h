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
		// How the frame is drawn, named by the tree it came from:
		//   0 Full      (Aether)      one clean rect with a black underlay
		//   1 Cornered  (Aether)      eight flags, solid black edge
		//   2 Shadowed      a drop shadow and a navy plate under the frame
		//   3 Double          black / colour / black band on every edge
		//   4 Corner Cut flags whose leg length follows the box height
		inline int  box_type{ 0 };
		// the corner length fraction; only used by entry 4.
		inline float box_corner_scale{ 0.24f };

		inline int  box_mode{ 0 };
		inline float box_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool  engine_chams{ false };
		// Style 1 rather than 0: style 0 swaps no program at all, so the default look
		// would be the character's own materials and the feature would read as
		// doing nothing.  Style 1 is a family the client was measured to have.
		inline int   engine_chams_style{ 1 };
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
		// 0 chevron (Aether), 1 arrow, 2 pointer, 3 ring
		//.  Every entry is the same idea drawn the way that source
		// drew it, so an arrow looks like the menu that named it.
		inline int   arrow_style{ 0 };
		// the arrows ride just outside the FOV circle instead of on a
		// radius of their own; that is the "orbit" entry's only difference.
		inline bool  arrow_use_fov_radius{ false };
		inline float arrow_color[4]{ 1.f, 1.f, 1.f, 0.95f };
		inline float arrow_size{ 14.f };
		inline float arrow_radius{ 200.f };
		inline bool  arrow_info[4]{ true, false, false, false };

		inline bool  tracer{ false };
		inline int   tracer_origin{ 0 };

		// Where the line lands on the target: the bottom of the box (the feet),
		// the middle of it, or the top (the head).  Feet is the classic one, and
		// the one a top-down view reads correctly - the head sits above the body
		// on screen, so a line to it appears to hang over nothing.
		inline int   tracer_end{ 0 };

		// The colour line's width in pixels.  The outline, when it is on, is the
		// same line one pixel wider underneath, so the pair reads as one stroke
		// with a solid edge rather than a dark band with a thin line in it.
		inline float tracer_thickness{ 1.4f };
		inline bool  tracer_outline{ true };

		// 0 = the plain line this has always been.  1 = the smooth style: one
		// tapered band with round caps and a fade toward the target, drawn as a
		// single shape so there is no join anywhere along it.		// 0 plain, 1 smooth (Aether), 2 beam, 3 laser.
		inline int  tracer_style{ 0 };

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
		// 0 Cone (Aether), 1 Ribbed (a spinning brim with ribs down to the
		// apex), 2 Pinwheel (alternating segment shade).
		inline int   china_hat_style{ 0 };
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

		// The line - and the aim card - follow the silent aim's own target while
		// there is no aimbot target.  Without this the line needs the aimbot (or
		// a pinned player) before it draws anything at all.
		inline bool  aim_line_silent{ false };

		inline float healthbar_width{ 6.f };
		inline bool  healthbar_glow    { false };
		// 0 Aether (tall bar left of the box), 1 framed (track with a gradient
		// fill and a bright head), 2 pill (rounded with a black lip), 3 flat
		// (solid bar with a black backing).
		inline int   healthbar_style{ 0 };
		// The armour bar: drawn above the health bar with the same shape.
		inline bool  armorbar{ false };
		inline float armorbar_color[4]{ 0.38f, 0.62f, 1.f, 1.f };

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

		// Two switches that change what the existing box *means* rather than
		// adding another shape: its colour comes from the player's health, or
		// from their team's own colour, instead of one fixed value for everyone.
		inline bool  box_health_color{ false };
		inline bool  box_team_color{ false };

		inline bool  box_body_only{ true };
		// The box's finish, layered on top of whatever shape Box Type draws:
		//   0 Plain      (Aether)     one box, nothing else
		//   1 Shadow        a solid black copy two pixels down-right
		//   2 Double          a thinner copy inset by three pixels
		// The corners are their own Box Type entry, not a finish.
		inline int   box_style{ 0 };

		inline bool  skeleton{ false };
		// The glow rig: the limb chains, one step wider and faint underneath.
		// Kept as a value because configs store it; the menu no longer offers a
		// choice of rig.
		inline int   skeleton_style{ 0 };
		// Unused by the glow rig, kept for the config keys older builds wrote.
		inline bool  skeleton_joints{ true };
		inline float skeleton_joint_size{ 2.4f };
		inline int   skeleton_vis_segments{ 6 };
		inline float skeleton_hidden_color[4]{ 0.45f, 0.46f, 0.52f, 1.f };

		inline bool  skeleton_animate{ true };
		inline float skeleton_swing{ 1.f };
		inline bool skeleton_outline{ false };
		inline float skeleton_thickness{ 1.5f };
		inline float skeleton_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float skeleton_outline_color[4]{ 0.f, 0.f, 0.f, 1.f };

		inline bool  chams{ false };
		inline float chams_fill_color[4]{ 1.f, 0.f, 0.f, 1.f };

		inline bool  nchams{ false };
		// 0 Box edges, 1 Box filled, 2 Hull union, 3 Shader fill (Aether),
		// 4 Chains (one silhouette for the whole body, black underlay, optional
		// pulse), 5 Rig (a hull per part, filled, no union).
		inline int   nchams_mode{ 0 };
		inline bool  nchams_pulse{ false };
		inline float nchams_pulse_speed{ 2.f };
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
		// 0 compass (Aether), 1 bearing, 2 directional.
		inline int   radar_style{ 0 };
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
		// Only ring on a headshot.  The reference plays on every hit; this is
		// the filter that keeps a body shot from sounding like a crit.
		inline bool  hitsound_head_only{ false };
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

		// Bullet tracers: a streak from where the shot came from to where it
		// landed, spawned by the same health-drop detection the hitmarker already
		// runs on - a shot is a target's health falling while it is under the
		// crosshair (or under an active aim target), so the line is only drawn
		// for shots that actually connected.
		inline bool  tracers{ false };
		inline float tracer_duration{ 1.60f };
		inline float tracer_thickness{ 1.8f };
		inline float tracer_color[4]{ 1.f, 0.88f, 0.42f, 1.f };
		inline bool  tracer_rainbow{ false };
		inline float tracer_rainbow_speed{ 0.35f };
		inline bool  tracer_fade{ true };
		inline bool  tracer_outline{ true };
		inline float tracer_glow{ 0.45f };

		// The little cross at the far end, where the bullet landed.
		inline bool  tracer_impact{ true };
		inline float tracer_impact_size{ 5.f };

		// Draw from the muzzle of the held tool when it can be found, so the
		// streak leaves the gun; off (or with no tool) it leaves the camera.
		inline bool  tracer_from_tool{ true };

		// 0 streak (Aether), 1 beam, 2 laser, 3 dots
		//.  The bullet tracer's own shape, independent of the aim and
		// ESP tracer lines.
		inline int   tracer_style{ 0 };

		// How long the every-shot streak is, in screen pixels.  It is drawn from
		// the muzzle towards the crosshair, so this is how far along that line it
		// reaches - the one number that decides whether a first-person tracer is
		// visible at all, since the muzzle itself is usually just off screen.
		inline float tracer_length{ 150.f };

		// 0 draws a streak only for a shot that connected: the health drop the
		// hitmarker reads confirms both that the shot was ours and where it landed,
		// so the line is the bullet's actual path.  1 - the default - draws one for
		// every shot fired, as a muzzle streak that runs from the gun towards the
		// crosshair, so the effect is visible whether or not it hit anything.  It
		// is the default because the connected-shot line needs a target to lose
		// health on *our* shot: a tracer that only appears once a fight is going
		// well reads as a broken tracer, not as a hitmarker.
		inline int   tracer_mode{ 1 };
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

		// Off: a 2 px lit segment that followed the cursor around the plate's edge
		// is a glow on the outside of the panel, which is the one thing a clean
		// frosted surface does not have.  Still switchable in Settings > Interface.
		inline bool  panel_rim{ false };
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

		// The panel's own clicks.  Quiet on purpose, and off is a first-class
		// answer: a sound the user did not ask for is worse than no sound.
		inline bool  click_sounds{ true };
		inline float click_volume{ 40.f };
	}

	// The optional surface layer, borrowed from the current crop of web UI
	// components.  Everything here is off until it is asked for, and the master
	// switch is off too: the shipped panel is the clean dark plate, and these are
	// the extra lights on top of it.
	namespace ui21
	{
		// Everything in this group is off until it is asked for - the master
		// switch included.  With it off the panel is exactly the panel it was
		// before this pack existed; with it on, each effect is still its own
		// switch, so turning the pack on is not a light show the user did not
		// ask for.
		inline bool  enabled{ false };
		inline bool  glass_layers{ false };
		inline float glass_depth{ 1.f };
		inline bool  spotlight_border{ false };
		inline float spotlight_size{ 1.f };
		inline bool  gradient_heading{ false };
		inline bool  odometer{ false };
		inline bool  dot_grid{ false };
		inline float dot_grid_spacing{ 22.f };
		inline float accent_2[4]{ 0.62f, 0.40f, 1.f, 1.f };
		inline bool  noise{ false };
		inline float noise_amount{ 0.5f };

		// Volumetric shafts from the panel's top edge, like a soft light rig above
		// a sheet of glass.  Off by default - it is the loudest thing in here.
		inline bool  light_rays{ false };
		inline float light_rays_strength{ 0.55f };
		inline float light_rays_spread{ 1.00f };

		// Two or three very broad colour fields drifting behind the content.
		inline bool  aurora{ false };
		inline float aurora_strength{ 0.45f };

		// A wider, softer halo under the plates than the hairline border gives.
		// The panel's own light: one broad wash off the top edge and a fainter
		// counter-wash at the bottom.  On by default, main menu only - the
		// floating cards draw their own material - and its strength follows the
		// Accent Spread slider, whether or not the accent is a custom one.
		inline bool  panel_glow{ true };
		inline bool  card_glow{ false };
		inline float card_glow_strength{ 0.55f };

		// A very soft dark falloff under the plate, so it sits on the scene
		// instead of being pasted onto it.  Off with the rest of the pack.
		inline bool  panel_shadow{ false };
		inline float panel_shadow_strength{ 0.55f };

		// A single slow highlight travelling along the panel's top edge.
		inline bool  top_sheen{ false };
		inline float top_sheen_speed{ 0.10f };
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
		// 0 ring (Aether), 1 dots, 2 dynamic, 3 pulse
		//.  The ring is the shape every other feature was tuned
		// against; the three below it are the same circle drawn by the source
		// named in each entry.
		inline int   fov_style{ 0 };
		inline float fov_circle_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_outline_colour[4]{ 0.f, 0.f, 0.f, 1.f };

		// The width of the colour ring, in pixels.  The ring used to be a 1.4 px
		// line with a 2.4 px dark line centred on it, which ate half the colour
		// and read as a thick smudged circle however the colour was set.
		inline float fov_thickness{ 1.5f };
		inline bool  fov_outline{ true };

		inline bool  fov_fill{ false };
		inline float fov_fill_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_fill_opacity{ 12.f };

		inline bool  smoothing{ false };
		inline float smoothingx{ 10.f };
		inline float smoothingy{ 10.f };
		inline int   smoothing_style{ 0 };

		inline bool  instant_lock{ false };

		// Milliseconds between mouse steps. 0 follows the overlay's frame limit
		// (`menu::fps_limit`), which is the honest measure of how often the game applies
		// the counts we send - see `mouse_send_interval_ms()` in features/aimbot.

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

		// The line's own width, and the outline as the same line one pixel wider
		// underneath it.  The pair used to be hardcoded at 1.25 px over 2.6 px.
		inline float tracer_thickness{ 1.25f };
		inline bool  tracer_outline{ true };		// 0 plain, 1 smooth (Aether), 2 beam, 3 laser.
		inline int  tracer_style{ 0 };

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
		inline int   fov_style{ 0 };
		inline float fov_thickness{ 1.5f };
		inline bool  fov_outline{ true };

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

		// Attach FOV to Target: the circle follows the body being aimed at
		// instead of the pointer.  It is on by default because that is
		// what the ring has always done here; the switch exists so it can stop.
		inline bool  attach_fov_to_target{ true };

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
		// A touch thinner than it used to be: with the outline under it the curve
		// reads its own width, so 2 px of colour over 2 px of edge was mud.
		inline float curve_tracer_thickness{ 1.6f };

		// Solid black under the curve, the same shape one pixel wider - off is
		// the bare colour on whatever is behind it.
		inline bool  curve_tracer_outline{ true };

		// Where the curve points.  0 - the default - is the silent aim's own
		// target, which is what it has always followed.  1 is the nearest enemy
		// to the cursor, so the curve can be drawn with silent aim off (or while
		// it is on but has not picked anybody up yet).
		inline int   curve_tracer_target{ 0 };
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
		inline int   activation_mode{ 1 };
		inline int   target_part{ 0 };
		inline float delay_ms{ 80.f };

		// Aimbot Target Only: the trigger will not fire on a body the aim is
		// not holding, so a click under the dot cannot land on whoever
		// else drifted through it.
		inline bool  aimbot_target_only{ false };
		inline float fov{ 20.f };
		inline bool  draw_fov{ false };
		inline int   fov_style{ 0 };
		inline float fov_circle_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_outline_colour[4]{ 0.f, 0.f, 0.f, 1.f };
		inline float fov_thickness{ 1.5f };
		inline bool  fov_outline{ true };

		inline bool  fov_fill{ false };
		inline float fov_fill_colour[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float fov_fill_opacity{ 12.f };
		inline bool  continuous{ false };
		inline bool  visibility_check{ false };
		inline bool  gun_check{ false };
		// Scope check: fire only while the right button is held.  It is a
		// toggle of its own rather than something baked into the trigger, so
		// a player who aims with another key can turn the requirement off.
		inline bool  aim_check{ false };
		// Stand down while the user is holding fire themselves: a click
		// injected over their burst arrives as a release and cuts it.
		// Turning this off restores the old click-anyway behaviour.
		inline bool  skip_while_firing{ true };
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
		// Hides the overlay from screenshots, recordings and screen shares.
		// Off by default: the menu is meant to be seen, and a capture of it is
		// the only way to show a bug in it. Config > Settings > Captures.
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

		// ---- the preset catalog ----
		//
		// The skybox catalog as one list:
		// every preset is six face ContentIds plus the sun and moon textures and
		// the three celestial numbers, and picking one puts the place's own Sky
		// instance into that sky.  Each half is its own switch because they fail
		// differently: the numbers are plain fields in the Sky and always land,
		// while a face is a Content string that can only be written where the
		// place already keeps one of its own.
		inline bool  preset_on{ false };
		inline int   preset{ 0 };              // index into sky_presets::k_presets
		inline bool  preset_faces{ true };     // the six faces
		inline bool  preset_sun_moon{ true };  // SunTextureId / MoonTextureId
		inline bool  preset_stars{ true };     // StarCount and the two angular sizes

		// Stars on their own, with no preset behind them.  The catalog has this
		// as its own switch - "stars at night" is something a user wants while
		// keeping the place's sky - and StarCount is a plain field of the Sky
		// rather than a face string, so it lands wherever a Sky exists.
		inline bool  stars_on{ false };
		inline int   stars_count{ 3000 };
		inline bool  orientation_on{ false };
		inline float orientation[3]{};         // degrees, the Sky's own rotation

		// the "Rotation": the sky turns on its own.  It is the same single
		// float the slider above writes - the client keeps one angle there, with the
		// sun's angular size immediately behind it - so the two share the field, and
		// the switch wins over the slider while it is on.
		inline bool  rotate_on{ false };
		inline float rotate_speed{ 12.f };     // degrees per second, either way
		inline bool  hide_celestial{ false };  // CelestialBodiesShown = false
		inline bool  log_skipped{ true };
	}

	namespace exploits
	{
		inline bool  low_gravity{ false };
		inline float gravity_value{ 8.f };

		inline bool  tp_cursor{ false };
		inline int   tp_cursor_key{ 'F' };
		inline float tp_cursor_dist{ 80.f };
	}

	// Features taken from the three trees under `../Sources`.  Each block says
	// which one, because "why does this behave differently from the rest of the
	// tab" is the question a ported feature always raises.  The code lives in
	// features/fusion/SourcePack.cpp; the tabs name the source on the row.
	namespace fusion
	{
		// ---- -------------------------------------------------------
		inline bool  antifling{ false };
		inline float antifling_lin{ 300.f };   // studs/s that count as a fling
		inline float antifling_ang{ 200.f };   // rad/s that count as a fling

		inline bool  jumppower{ false };
		inline int   jumppower_key{ 0 };
		inline int   jumppower_mode{ 0 };
		inline float jumppower_value{ 50.f };

		inline bool  hipheight{ false };
		inline float hipheight_value{ 2.f };

		inline bool  freecam{ false };
		inline int   freecam_key{ 0 };
		inline int   freecam_mode{ 0 };
		inline float freecam_speed{ 40.f };
		inline float freecam_sprint{ 90.f };
		inline bool  freecam_revert{ true };

		inline bool  orbit{ false };
		inline int   orbit_key{ 0 };
		inline int   orbit_activation{ 0 };
		inline int   orbit_target{ 0 };
		inline float orbit_radius{ 6.f };
		inline float orbit_speed{ 2.f };
		inline float orbit_height{ 0.f };
		inline bool  orbit_spectate{ false };

		inline bool  headless{ false };

		// ---- korblox (local visual) --------------------------------
		// Hides the right foot and lower leg, which is the silhouette the
		// reference gets from those two parts.  It hides the parts rather than
		// swapping the upper leg's MeshId: that write needs a string allocated
		// inside the client and the original heap pointer kept alive, which is a
		// lot of machinery for a cosmetic - and a bad place to be wrong.
		inline bool  korblox{ false };

		inline bool  show_fps{ false };

		// ---- ---------------------------------------------------
		// Recoil compensation: while the fire button is held the mouse is walked
		// down (and sideways, if asked) by a share of the shot's kick.  It is a
		// mouse mover rather than a memory write, so it works in every game -
		// and it is the one feature here that a player can feel.
		inline bool  norecoil{ false };
		inline int   norecoil_key{ 0 };
		inline float norecoil_x{ 0.f };
		inline float norecoil_y{ 1.f };
		inline float norecoil_strength{ 1.f };

		// ---- spectate ---------------------------------------------
		// Follower camera: CameraSubject is pointed at a player's root and
		// CameraType forced to Scriptable.  The cycle key walks the roster,
		// exactly as the reference does with its target index.
		inline bool  spectate{ false };
		inline int   spectate_cycle_key{ 'Z' };

		// ---- fly above -------------------------------------------
		// Rides the pinned target from the Players tab (or the nearest player
		// when nothing is pinned), twelve studs overhead, by writing the root
		// position.  Fifteen repeats a tick, like the source, because the
		// physics step re-applies the position it computed before the first
		// write landed.
		inline bool  flyabove{ false };
		inline float flyabove_height{ 12.f };

		// ---- hit notifications ---------------------------------
		// The damage toast: a slide-in card per landed hit with the damage and
		// the part that was hit.  It is separate from the hitmarker - that one
		// is a cross at the impact, this one is a log.
		inline bool  hit_notifications{ false };
		inline float hit_notification_seconds{ 3.f };

		// ---- anti-AFK -----------------------------------------------
		// The reference writes the party inactivity FFlag.  That needs a flag
		// address this build has no measured value for, and a wrong address is
		// a write into an unknown object - so the idle timer is reset the way
		// Windows itself measures it: one pixel of cursor movement, there and
		// back, which the client cannot see and the game cannot bind.
		inline bool  antiafk{ false };
		inline float antiafk_idle{ 45.f };      // seconds of no input first

		// ---- peek return ---------------------------------------
		// The ideal-peek's simplest mode: the spot the character stood on when
		// the key went down is saved, and it is walked back to - on the key's
		// release, or on the next shot - at the set speed, then stopped dead.
		inline bool  peek_return{ false };
		inline int   peek_key{ 0 };
		inline float peek_speed{ 60.f };        // studs/s of the return
		inline int   peek_trigger{ 0 };         // 0 key release, 1 on left click
		inline bool  peek_save_on_hold{ true }; // save when the key goes down

		// ---- Auto clicker ---------------------------------------------------
		// A held key turns into clicks at the set rate.  The interval is not
		// sent through a sleep: the loop is 2 ms and the next click is timed
		// from GetTickCount64, so a busy frame delays a click instead of
		// shifting every one after it.
		inline bool  autoclicker{ false };
		inline int   autoclicker_key{ 0 };
		inline float autoclicker_cps{ 12.f };
		inline float autoclicker_jitter{ 0.f }; // +/- share of the interval

		// ---- Camera field of view -------------------------------------------
		// Writes the camera's own FOV each slow tick.  The value it had is kept
		// and put back when the switch goes off, or when the camera object is
		// replaced under it.
		inline bool  camera_fov{ false };
		inline float camera_fov_value{ 70.f };
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

		// How far the custom accent reaches *beyond* the parts that are always
		// the accent (the nav pill, the switches, the sliders).  0 leaves the top
		// bar, the rules, the row hover and the text exactly as the theme has
		// them; 1 tints all of those, so the surface reads as one colour wash.
		// The controls that are already accent stay the accent whatever this is.
		inline float accent_spread{ 0.35f };

		inline bool  window_gradient{ true };
		inline bool  window_shadow{ true };
		inline bool  accent_glow{ true };
		inline float glow_strength{ 0.65f };
		inline float rounding{ 8.f };

		inline float panel_alpha{ 0.86f };
		inline float dock_alpha{ 0.78f };

		// One opaque panel: the theme's own dark fill at full alpha, with the
		// glass left out of it entirely.  A clean dark plate instead of a window
		// the game shows through - and the capture behind it is skipped, because
		// nothing of the desktop shows through to blur.  Off by default: the
		// panel is translucent until it is asked to be solid.
		// Preview build: on by default, for the same reason.
		inline bool  no_transparency{ true };

		// The menu's own surface sheen - how much white the plates and the wells
		// are lifted by.  It is *not* the backdrop: the backdrop's frost pass
		// (a desaturating veil over the captured image) is gone for good, and
		// this is the only thing left that name used to cover.
		inline float glass_sheen{ 0.30f };

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

		// Which way the falling elements fall - rain, matrix and the like.
		// `bg_dir` is the preset (Down / Diagonal Left / Diagonal Right /
		// Sideways Right), and it writes `bg_angle`; the angle is the one the
		// simulation and the streak are both built from, so a drop and the line
		// it draws always lean the same way.  Negative leans left, and a custom
		// angle leaves the preset reading as Custom.
		inline int   bg_dir{ 0 };
		inline float bg_angle{ 16.f };

		// How long a rain streak is drawn.  It is the *persistence* of the drop:
		// the length follows the speed it is actually falling at, so slowing the
		// element down shortens the streaks with it instead of leaving a slow
		// drizzle made of the same long sticks.  This slider scales that.
		inline float bg_streak{ 1.0f };

		// How far each drop may lean off the chosen direction, in degrees.  0 is a
		// comb of perfectly parallel lines; a few degrees is weather.  The lean is
		// applied per drop at spawn and drives its velocity *and* its streak, so a
		// drop and the line it draws still agree about which way it is going.
		inline float bg_spread{ 4.0f };

		// The rain streak's width, as a multiple of the element size.
		inline float bg_width{ 1.0f };

		// The overlay's own anti-aliasing - the ESP boxes, the FOV ring, the
		// tracers, the skeleton and the curves the aim draws.  Off by default:
		// the shipped rasterisation is the one every feature was tuned against,
		// and smoothing the overlay's strokes costs a little of the overlay's
		// own frame.
		//
		// On, a stroke is placed on the pixel *centre* rather than on the pixel
		// edge - so a 1 px line lands on one column of pixels instead of half on
		// each of two - the filled shapes take ImGui's anti-aliased fill, and the
		// rings and discs are drawn a step finer with a feathered rim, because a
		// quad strip cannot be anti-aliased by ImDrawList at all.
		inline bool  antialiasing{ false };

		// How far into that smoothing the shapes go: the ring and disc segment
		// counts, the width of the feathered rim, and how finely the curves the
		// overlay draws are tessellated.  1 is the balanced setting.
		inline float aa_quality{ 1.00f };

		// Enhance: one switch over the whole surface language.  The tight,
		// near-rectangular surface the reference is - smaller controls, closer
		// rows, every radius pulled in a step, feature groups separated by a rule
		// instead of by whitespace alone, and every change of it animated rather
		// than snapped.  Off by default: this is the whole panel's look, and the
		// one it ships with is the flat, roomier one.
		inline bool  enhance{ false };

		// A nudge to the panel's overall size on top of that, and nothing else -
		// the tightness is Enhance's own.  1 is the panel it already is.
		inline float enhance_size{ 1.00f };

		// How wide the panel is, on top of its scale.  1 is the shipped width;
		// below it the content column comes in without shrinking the type, above
		// it the line grows.  Kept separate from Enhance because a narrow panel
		// and a small one are two different things - the reference is a tall
		// column, not a shrunk window.
		inline float panel_width{ 1.00f };

		// Test Enhance: one step in from the shipped layout, with nothing else
		// changed.  The left column loses the dead space either side of the nav
		// (a narrower sidebar, a smaller pad, a thinner divider gap) and the whole
		// panel comes down a little, so the same rows sit in less room instead of
		// in a second layout.  Off by default, like Enhance, and animated.
		// Preview build: on by default so the compact surface can be compared with
		// the reference without setting it up first.  It is still one switch.
		inline bool  test_enhance{ true };

		// The plate's own contrast: how far the surface is lifted above pure
		// black.  No Transparency uses this fill at full alpha.
		inline float panel_contrast{ 1.00f };
	}

	namespace menu
	{
		inline bool vsync{ false };
		inline bool performance_mode{ false };

		inline int fps_limit{ 240 };

		inline bool hide_watermark{ false };
		inline bool ignore_unsupported_place{ false };
	}

	// The backdrop is one capture of the desktop, down-scaled hard and boxed
	// into a small frost image the panel draws behind itself.  It costs a copy of
	// the *panel's rectangle* per captured frame - not the desktop - and nothing
	// at all while the menu is shut.  Switched off until asked for.
	namespace backdrop
	{
		// Off by default, as it was in the merged tree: the capture is a per-frame
		// cost and an effect that changes the whole panel's material, so it is the
		// user's switch, not something that arrives switched on.
		inline bool  enabled{ false };

		// The merged tree's values, restored.  These two numbers *are* the look:
		// blur 65 asks for a wide frost, and tint 100 is the darkening that makes
		// the frost read as glass rather than as a smeared screenshot.  They were
		// lowered here in one round and the result was exactly the report - the
		// blur barely visible and the tint doing nothing.
		inline float blur{ 75.f };
		inline float tint{ 100.f };

		// How far outside the panel, the card and the plates the capture reaches,
		// in pixels.  Zero - the default - is the cap: only the rectangles that
		// sample the image are captured, so a frame of the effect costs a quarter
		// of the screen instead of all of it and the game behind the panel keeps
		// its frame rate.  Raise it only if the blur's edge is visibly clipped.
		inline float capture_pad{ 0.f };

		// Tint only: what is behind the panel is darkened but never blurred.
		// Not one blur pass runs, so the background stays sharp and a captured
		// frame costs a fraction of the blurred one - it is the cheap end of the
		// effect, and it is what "the tint without the smudge" is for: text over
		// a readable background instead of over a frost.  The Blur slider is
		// bypassed while it is on; the Tint slider still drives it.
		inline bool  tint_only{ false };

		// The most of the output the blur may copy, as a percentage of its area.
		// 0 is the off switch: the capture is never started, so the panel, the
		// card and the plates are drawn without a frost image and nothing behind
		// the menu pays for a frame nobody samples.  100 - the default - copies
		// the panels' own rectangle whole.
		inline float capture_cap{ 100.f };

		inline bool  status_line{ false };
	}

	namespace settings
	{
		inline bool hide_console{ false };
		inline bool should_unload{ false };
	}
}