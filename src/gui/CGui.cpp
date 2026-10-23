#include "CGui.h"

#include <debug/DebugConsole.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/addons/imgui_addons.h>
#include <imgui/misc/freetype/imgui_freetype.h>

#include <gui/AetherFonts.h>

#include <gui/AetherWidgets.h>
#include <gui/AetherIcons.h>
#include <gui/AetherBackground.h>
#include <gui/explorer/AetherExplorer.h>
#include <gui/menu/SyncBridge.h>
#include <gui/menu/SyncMenu.h>
#include <gui/menu/UiSound.h>
#include <features/misc/UiFx.h>
#include <features/mesh/MeshChams.h>
#include <features/mesh/MeshDxShader.h>
#include <features/esp/Avatar.h>
#include <features/esp/esp.h>
#include <features/esp/BoxFill.h>
#include <features/aimbot/Telemetry.h>
#include <features/misc/Sonar.h>
#include <game/game_context.h>
#define APP_NAME "aether"

static void apply_theme(int idx);

#include <settings.h>
#include <features/aimbot/aimbot.h>
#include <features/silentaim/silentaim.h>
#include <features/players/targets.h>
#include <features/misc/exploits.h>
#include <features/misc/SkyShader.h>
#include <features/misc/SkyVideo.h>
#include <game/game.h>
#include <render/render.h>
#include <render/SurfaceTex.h>
#include <keybind/keybind.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <memory/memory.h>

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <mutex>
#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>
#include <mmsystem.h>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "winmm.lib")

namespace cfg
{
    static std::string cfg_dir()
    {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
            return std::string(path) + "\\Aether\\configs\\";
        return ".\\configs\\";
    }
    static void ensure_dir() { std::filesystem::create_directories(cfg_dir()); }
    static void refresh_list()
    {
        sync::state().cfg_names.clear();
        ensure_dir();
        for (auto& e : std::filesystem::directory_iterator(cfg_dir()))
            if (e.path().extension() == ".cfg")
                sync::state().cfg_names.push_back(e.path().stem().string());
        if (sync::state().cfg_selected >= (int)sync::state().cfg_names.size())
            sync::state().cfg_selected = 0;
    }

    static void write_float(std::ostream& s, const char* k, float v)  { s << k << "=" << v << "\n"; }
    static void write_bool (std::ostream& s, const char* k, bool  v)  { s << k << "=" << (v?1:0) << "\n"; }
    static void write_int  (std::ostream& s, const char* k, int   v)  { s << k << "=" << v << "\n"; }
    static void write_col4 (std::ostream& s, const char* k, float* c) { s << k << "=" << c[0]<<","<<c[1]<<","<<c[2]<<","<<c[3] << "\n"; }
    static void write_col3 (std::ostream& s, const char* k, float* c) { s << k << "=" << c[0]<<","<<c[1]<<","<<c[2] << "\n"; }
    static void write_str  (std::ostream& s, const char* k, const char* v) { s << k << "=" << (v ? v : "") << "\n"; }

    static void save(const std::string& name)
    {
        ensure_dir();
        std::ofstream f(cfg_dir() + name + ".cfg");
        if (!f) return;

        write_bool(f,"vis.box",          settings::visuals::box);
        write_int(f,"vis.box_type",      settings::visuals::box_type);
        write_int(f,"vis.box_style",     settings::visuals::box_style);
        write_float(f,"vis.box_corner",  settings::visuals::box_corner_scale);

        write_int(f,"vis.box_mode",      settings::visuals::box_mode);
        write_col4(f,"vis.box_color",    settings::visuals::box_color);
        write_bool(f,"vis.box_fill",     settings::visuals::box_fill);
        write_int (f,"vis.box_fill_mode", settings::visuals::box_fill_mode);
        write_str (f,"vis.box_fill_path",settings::visuals::box_fill_image_path);
        write_bool(f,"vis.box_fill_done",settings::visuals::box_fill_image_loaded);
        write_float(f,"vis.box_fill_a",   settings::visuals::box_fill_image_alpha);
        write_bool(f,"vis.box_fill_nobg",settings::visuals::box_fill_remove_bg);
        write_col4(f,"vis.box_fill_col", settings::visuals::box_fill_color);
        write_bool(f,"vis.box_gradient", settings::visuals::box_gradient);
        write_col4(f,"vis.box_grad_top", settings::visuals::box_gradient_top);
        write_col4(f,"vis.box_grad_bot", settings::visuals::box_gradient_bottom);
        write_bool(f,"vis.enable_enemies",settings::visuals::enable_enemies);
        write_bool(f,"vis.knock_check",  settings::visuals::knock_check);
        write_bool(f,"vis.team_check",   settings::visuals::team_check);
        write_bool(f,"vis.dead_check",   settings::visuals::dead_check);
        write_bool(f,"vis.vis_check",    settings::visuals::visible_check);
        write_bool(f,"vis.box_body_only",settings::visuals::box_body_only);
        write_bool(f,"vis.name",         settings::visuals::name);
        write_int(f,"vis.name_disp",     settings::visuals::name_display_type);
        write_col4(f,"vis.name_col",     settings::visuals::name_color);
        write_bool(f,"vis.blend",        settings::visuals::blend);
        write_col4(f,"vis.blend_start",  settings::visuals::name_color_blend_start);
        write_col4(f,"vis.blend_end",    settings::visuals::name_color_blend_end);
        write_bool(f,"vis.avatar",       settings::visuals::avatar);
        write_bool(f,"vis.distance",     settings::visuals::distance);
        write_int(f,"vis.dist_meas",     settings::visuals::distance_measurement);
        write_col4(f,"vis.dist_col",     settings::visuals::distance_color);
        write_float(f,"vis.max_dist",    settings::visuals::max_distance);
        write_bool(f,"vis.arrows",        settings::visuals::offscreen_arrows);
        write_col4(f,"vis.arrows_col",    settings::visuals::arrow_color);
        write_float(f,"vis.arrow_size",   settings::visuals::arrow_size);
        write_float(f,"vis.arrow_radius", settings::visuals::arrow_radius);
        write_int (f,"vis.arrow_style",   settings::visuals::arrow_style);
        write_bool(f,"vis.arrow_fov_r",   settings::visuals::arrow_use_fov_radius);
        write_int (f,"vis.hat_style",     settings::visuals::china_hat_style);
        write_bool(f,"vis.arrow_inf_n",   settings::visuals::arrow_info[0]);
        write_bool(f,"vis.arrow_inf_d",   settings::visuals::arrow_info[1]);
        write_bool(f,"vis.arrow_inf_h",   settings::visuals::arrow_info[2]);
        write_bool(f,"vis.arrow_inf_t",   settings::visuals::arrow_info[3]);
        write_bool(f,"vis.tracer",        settings::visuals::tracer);
        write_int(f,"vis.tracer_origin",  settings::visuals::tracer_origin);
        write_int (f,"vis.tracer_end",   settings::visuals::tracer_end);
        write_float(f,"vis.tracer_th",    settings::visuals::tracer_thickness);
        write_bool(f,"vis.tracer_outline",settings::visuals::tracer_outline);
        write_int (f,"vis.tracer_style", settings::visuals::tracer_style);
        write_col4(f,"vis.tracer_col",    settings::visuals::tracer_color);
        write_bool(f,"vis.china_hat",     settings::visuals::china_hat);
        write_col4(f,"vis.hat_col",       settings::visuals::china_hat_color);
        write_float(f,"vis.hat_h",        settings::visuals::china_hat_height);
        write_float(f,"vis.hat_r",        settings::visuals::china_hat_radius);
        write_bool(f,"vis.body_corpse",   settings::visuals::body_corpse);
        write_col4(f,"vis.corpse_col",    settings::visuals::corpse_color);
        write_bool(f,"vis.health_text",   settings::visuals::health_text);
        write_col4(f,"vis.htext_col",     settings::visuals::health_text_color);
        write_float(f,"vis.box_thick",    settings::visuals::box_thickness);
        write_bool(f,"vis.dist_check",    settings::visuals::distance_check);
        write_int(f,"vis.dist_unit",      settings::visuals::distance_unit);
        write_col4(f,"vis.aim_col",       settings::visuals::aim_target_color);
        write_col4(f,"vis.friend_col",    settings::visuals::friend_color);
        write_bool(f,"vis.tool",         settings::visuals::tool);
        write_col4(f,"vis.tool_col",     settings::visuals::tool_color);
        write_bool(f,"vis.flags",        settings::visuals::flags);
        write_col4(f,"vis.flags_col",    settings::visuals::flags_state_colour);
        write_bool(f,"vis.rigtype",      settings::visuals::rigtype);
        write_bool(f,"vis.state",        settings::visuals::state);
        write_bool(f,"vis.healthbar",    settings::visuals::healthbar);
        write_col4(f,"vis.hb_col",       settings::visuals::healthbar_color);
        write_bool(f,"vis.hb_based",     settings::visuals::health_based_healthbar);
        write_bool(f,"vis.health_pct",   settings::visuals::health_percent);
        write_col4(f,"vis.health_pct_col",settings::visuals::health_percent_color);
        write_float(f,"vis.bar_size",    settings::visuals::healthbar_size);
        write_bool(f,"vis.skeleton",     settings::visuals::skeleton);
        write_bool(f,"vis.skel_anim",    settings::visuals::skeleton_animate);
        write_float(f,"vis.skel_swing",  settings::visuals::skeleton_swing);
        write_bool(f,"vis.skel_outline", settings::visuals::skeleton_outline);
        write_float(f,"vis.skel_thick",  settings::visuals::skeleton_thickness);
        write_col4(f,"vis.skel_col",     settings::visuals::skeleton_color);
        write_col4(f,"vis.skel_out_col", settings::visuals::skeleton_outline_color);
        write_bool(f,"vis.chams",        settings::visuals::chams);

        write_bool (f,"vis.nchams",        settings::visuals::nchams);
        write_int  (f,"vis.nchams_mode",   settings::visuals::nchams_mode);
        write_col4 (f,"vis.nchams_out",    settings::visuals::nchams_outline_color);
        write_col4 (f,"vis.nchams_fcol",   settings::visuals::nchams_fill_color);
        write_int  (f,"vis.nchams_style",  settings::visuals::nchams_shader_style);
        write_bool (f,"vis.eng_chams",    settings::visuals::engine_chams);
        write_int  (f,"vis.eng_style",    settings::visuals::engine_chams_style);
        write_col4 (f,"vis.eng_col",      settings::visuals::engine_chams_color);
        write_col4(f,"vis.chams_fill",   settings::visuals::chams_fill_color);
        write_int(f,"vis.mesh_shader",   settings::visuals::mesh_shader_mode);
        write_col4(f,"vis.mesh_occl",    settings::visuals::mesh_occluded_color);
        write_int(f,"vis.esp_font",      settings::visuals::esp_font);
        write_float(f,"vis.esp_font_size",settings::visuals::esp_font_size);

        write_bool (f,"vis.hb_enh",      settings::visuals::healthbar_enhanced);
        write_float(f,"vis.hb_width",    settings::visuals::healthbar_width);
        write_float(f,"vis.hb_opacity",  settings::visuals::healthbar_opacity);
        write_bool (f,"vis.hb_glow",     settings::visuals::healthbar_glow);
        write_int  (f,"vis.hb_style",    settings::visuals::healthbar_style);
        write_bool (f,"vis.armorbar",    settings::visuals::armorbar);
        write_col4 (f,"vis.armorbar_col",settings::visuals::armorbar_color);
        write_bool (f,"vis.nchams_pulse", settings::visuals::nchams_pulse);
        write_float(f,"vis.nchams_pulse_s", settings::visuals::nchams_pulse_speed);
        write_bool (f,"vis.nchams_glow", settings::visuals::nchams_glow);
        write_float(f,"vis.nchams_glow_o",settings::visuals::nchams_glow_opacity);
        write_float(f,"vis.nchams_glow_s",settings::visuals::nchams_glow_size);
        write_bool (f,"vis.nchams_core", settings::visuals::nchams_inner_fill);
        write_float(f,"vis.nchams_core_s",settings::visuals::nchams_inner_fill_scale);
        write_float(f,"vis.fade_in",     settings::visuals::fade_in_speed);
        write_float(f,"vis.fade_out",    settings::visuals::fade_out_speed);
        write_bool(f,"vis.view_hitbox",  settings::visuals::view_hitbox);
        write_col4(f,"vis.vhb_col",      settings::visuals::view_hitbox_color);
        write_bool(f,"vis.localplayer",  settings::visuals::localplayer);

        write_bool (f,"vis.headdot",      settings::visuals::head_dot);
        write_float(f,"vis.headdot_size", settings::visuals::head_dot_size);
        write_col4 (f,"vis.headdot_col",  settings::visuals::head_dot_color);
        write_bool (f,"vis.headdot_out",  settings::visuals::head_dot_outline);
        write_col4 (f,"vis.headdot_ocol", settings::visuals::head_dot_outline_color);
        write_bool (f,"vis.lookdir",      settings::visuals::look_direction);
        write_float(f,"vis.lookdir_len",  settings::visuals::look_direction_length);
        write_float(f,"vis.lookdir_th",   settings::visuals::look_direction_thickness);
        write_col4 (f,"vis.lookdir_col",  settings::visuals::look_direction_color);
        write_bool(f,"vis.hit_skeleton", settings::visuals::hit_skeleton);
        write_col4(f,"vis.hit_skel_col", settings::visuals::hit_skeleton_color);
        write_float(f,"vis.hit_skel_dur",settings::visuals::hit_skeleton_duration);
        write_bool(f,"vis.hit_chams",    settings::visuals::hit_chams);
        write_col4(f,"vis.hit_chams_col",settings::visuals::hit_chams_color);
        write_float(f,"vis.hit_chams_dur",settings::visuals::hit_chams_duration);
        write_bool(f,"vis.hit_freeze",   settings::visuals::hit_freeze);
        write_bool(f,"vis.movement_status",settings::visuals::movement_status);
        write_bool (f,"vis.radar",       settings::visuals::radar_enabled);
        write_float(f,"vis.radar_size",  settings::visuals::radar_size);
        write_int  (f,"vis.radar_range", settings::visuals::radar_range);
        write_int  (f,"vis.radar_pos",   settings::visuals::radar_position);
        write_int  (f,"vis.radar_shape", settings::visuals::radar_shape);
        write_float(f,"vis.radar_op",    settings::visuals::radar_opacity);
        write_bool (f,"vis.radar_rot",   settings::visuals::radar_rotate);
        write_bool (f,"vis.radar_grid",  settings::visuals::radar_grid);
        write_bool (f,"vis.radar_labels",settings::visuals::radar_labels);
        write_bool (f,"vis.radar_sweep", settings::visuals::radar_sweep);
        write_bool (f,"vis.radar_names", settings::visuals::radar_names);
        write_bool (f,"vis.radar_hp",    settings::visuals::radar_health);
        write_bool (f,"vis.radar_dead",  settings::visuals::radar_dead);
        write_bool (f,"vis.radar_walls", settings::visuals::radar_show_walls);
        write_col4 (f,"vis.radar_wall_col", settings::visuals::radar_wall_color);
        write_col4 (f,"vis.radar_local_col",settings::visuals::radar_local_color);
        write_col4 (f,"vis.radar_team_col", settings::visuals::radar_team_color);
        write_col4 (f,"vis.radar_enemy_col",settings::visuals::radar_enemy_color);

        write_bool (f,"fx.hitmarker",      settings::effects::hitmarker);
        write_float(f,"fx.hm_size",        settings::effects::hitmarker_size);
        write_float(f,"fx.hm_dur",         settings::effects::hitmarker_duration);
        write_col4 (f,"fx.hm_col",         settings::effects::hitmarker_color);
        write_col4 (f,"fx.hm_head_col",    settings::effects::hitmarker_head_color);
        write_bool (f,"fx.hitdata",        settings::effects::hitdata);
        write_bool (f,"fx.hd_damage",      settings::effects::hitdata_damage);
        write_bool (f,"fx.hd_health",      settings::effects::hitdata_health);
        write_bool (f,"fx.hd_distance",    settings::effects::hitdata_distance);
        write_float(f,"fx.hd_size",        settings::effects::hitdata_size);
        write_float(f,"fx.hd_dur",         settings::effects::hitdata_duration);
        write_bool (f,"fx.hitsound",       settings::effects::hitsound);
        write_int  (f,"fx.hs_index",       settings::effects::hitsound_index);
        write_float(f,"fx.hs_vol",         settings::effects::hitsound_volume);
        write_bool (f,"fx.crosshair",      settings::effects::crosshair);
        write_bool (f,"fx.cross_lock",     settings::effects::crosshair_lock);
        write_float(f,"fx.cross_len",      settings::effects::crosshair_length);
        write_float(f,"fx.cross_gap",      settings::effects::crosshair_gap);
        write_float(f,"fx.cross_thick",    settings::effects::crosshair_thickness);
        write_bool (f,"fx.cross_spin",     settings::effects::crosshair_spin);
        write_float(f,"fx.cross_spin_spd", settings::effects::crosshair_spin_speed);
        write_bool (f,"fx.cross_outline",  settings::effects::crosshair_outline);
        write_bool (f,"fx.cross_dot",      settings::effects::crosshair_dot);
        write_float(f,"fx.cross_dot_size", settings::effects::crosshair_dot_size);
        write_col4 (f,"fx.cross_col",      settings::effects::crosshair_color);
        write_col4 (f,"fx.cross_out_col",  settings::effects::crosshair_outline_color);
        write_bool (f,"fx.cross_str",      settings::effects::crosshair_stretch);
        write_float(f,"fx.cross_str_amt",  settings::effects::crosshair_stretch_amount);
        write_float(f,"fx.cross_str_spd",  settings::effects::crosshair_stretch_speed);

        write_bool (f,"fx.tracer",         settings::effects::tracers);
        write_float(f,"fx.tracer_dur",     settings::effects::tracer_duration);
        write_float(f,"fx.tracer_thick",   settings::effects::tracer_thickness);
        write_col4 (f,"fx.tracer_col",     settings::effects::tracer_color);
        write_bool (f,"fx.tracer_rainbow", settings::effects::tracer_rainbow);
        write_float(f,"fx.tracer_rspd",    settings::effects::tracer_rainbow_speed);
        write_bool (f,"fx.tracer_fade",    settings::effects::tracer_fade);
        write_bool (f,"fx.tracer_outline", settings::effects::tracer_outline);
        write_float(f,"fx.tracer_glow",    settings::effects::tracer_glow);
        write_bool (f,"fx.tracer_impact",  settings::effects::tracer_impact);
        write_float(f,"fx.tracer_imp_sz",  settings::effects::tracer_impact_size);
        write_bool (f,"fx.tracer_tool",    settings::effects::tracer_from_tool);
        write_int  (f,"fx.tracer_mode",    settings::effects::tracer_mode);
        write_float(f,"fx.tracer_len",     settings::effects::tracer_length);

        write_bool(f,"hbx.enabled",      settings::hitbox_expander::enabled);
        write_float(f,"hbx.size_x",      settings::hitbox_expander::size_x);
        write_float(f,"hbx.size_y",      settings::hitbox_expander::size_y);
        write_float(f,"hbx.size_z",      settings::hitbox_expander::size_z);
        write_bool(f,"hbx.knock_check",  settings::hitbox_expander::knock_check);

        write_bool(f,"tb.enabled",       settings::triggerbot::enabled);
        write_int(f,"tb.keybind",        settings::triggerbot::keybind);
        write_int(f,"tb.actmode",        settings::triggerbot::activation_mode);
        write_float(f,"tb.delay_ms",     settings::triggerbot::delay_ms);
        write_float(f,"tb.fov",          settings::triggerbot::fov);
        write_bool(f,"tb.draw_fov",      settings::triggerbot::draw_fov);
        write_int (f,"tb.target_part",  settings::triggerbot::target_part);
        write_bool(f,"tb.continuous",   settings::triggerbot::continuous);
        write_bool(f,"tb.visibility",   settings::triggerbot::visibility_check);
        write_bool(f,"tb.gun_check",    settings::triggerbot::gun_check);
        write_bool(f,"tb.aim_check",    settings::triggerbot::aim_check);
        write_bool(f,"tb.skip_firing",  settings::triggerbot::skip_while_firing);
        write_bool(f,"tb.knock_check",   settings::triggerbot::knock_check);
        write_bool(f,"tb.team_check",   settings::triggerbot::team_check);

        write_bool(f,"sp360.enabled",    settings::spin360::enabled);
        write_int(f,"sp360.keybind",     settings::spin360::keybind);
        write_int(f,"sp360.actmode",     settings::spin360::activation_mode);
        write_float(f,"sp360.speed",     settings::spin360::speed);

        write_bool(f,"ab.enabled",       settings::aimbot::enabled);
        write_int(f,"ab.keybind",        settings::aimbot::keybind);
        write_int(f,"ab.actmode",        settings::aimbot::activation_mode);
        write_int(f,"ab.mode",           settings::aimbot::mode);
        write_int(f,"ab.target_part",    settings::aimbot::target_part);
        write_bool(f,"ab.air_part",      settings::aimbot::air_part_enabled);
        write_int(f,"ab.air_part_id",    settings::aimbot::air_part);
        write_float(f,"ab.fov",          settings::aimbot::fov);
        write_bool(f,"ab.use_fov",       settings::aimbot::use_fov);
        write_bool(f,"ab.draw_fov",      settings::aimbot::draw_fov);
        write_col4(f,"ab.fov_col",       settings::aimbot::fov_circle_colour);
        write_col4(f,"ab.fov_out_col",   settings::aimbot::fov_outline_colour);
        write_bool(f,"ab.smoothing",     settings::aimbot::smoothing);
        write_bool(f,"ab.instant_lock",  settings::aimbot::instant_lock);
        write_float(f,"ab.smooth_x",     settings::aimbot::smoothingx);
        write_float(f,"ab.smooth_y",     settings::aimbot::smoothingy);
        write_int(f,"ab.smooth_style",   settings::aimbot::smoothing_style);

        write_int(f,"ab.priority",       settings::aimbot::priority);
        write_bool(f,"ab.prediction",    settings::aimbot::enable_prediction);
        write_float(f,"ab.pred_x",       settings::aimbot::prediction_x);
        write_float(f,"ab.pred_y",       settings::aimbot::prediction_y);
        write_bool(f,"ab.air_pred",      settings::aimbot::air_prediction_enabled);
        write_float(f,"ab.air_pred_x",   settings::aimbot::air_prediction_x);
        write_float(f,"ab.air_pred_y",   settings::aimbot::air_prediction_y);
        write_bool(f,"ab.teamcheck",     settings::aimbot::teamcheck);
        write_bool(f,"ab.knock_check",   settings::aimbot::knock_check);
        write_bool(f,"ab.dead_check",    settings::aimbot::dead_check);
        write_bool(f,"ab.vis_check",     settings::aimbot::visible_check);
        write_bool(f,"ab.sticky",        settings::aimbot::sticky_aim);
        write_bool(f,"ab.health_check",  settings::aimbot::health_check_enabled);
        write_float(f,"ab.min_health",   settings::aimbot::min_health);
        write_bool(f,"ab.tracer",        settings::aimbot::tracer);
        write_int (f,"ab.tracer_origin", settings::aimbot::tracer_origin);
        write_col4(f,"ab.tracer_col",    settings::aimbot::tracer_color);
        write_bool(f,"ab.offset",        settings::aimbot::offset_enabled);
        write_float(f,"ab.offset_x",     settings::aimbot::offset_x);
        write_float(f,"ab.offset_y",     settings::aimbot::offset_y);

        write_bool(f,"sa.enabled",       settings::silentaim::enabled);
        write_int(f,"sa.keybind",        settings::silentaim::keybind);
        write_int(f,"sa.actmode",        settings::silentaim::activation_mode);
        write_int(f,"sa.method",         settings::silentaim::method);
        write_int(f,"sa.target_part",    settings::silentaim::target_part);
        write_float(f,"sa.fov",          settings::silentaim::fov);
        write_bool(f,"sa.use_fov",       settings::silentaim::use_fov);
        write_bool(f,"sa.draw_fov",      settings::silentaim::draw_fov);
        write_bool(f,"sa.prediction",    settings::silentaim::enable_prediction);
        write_float(f,"sa.pred_x",       settings::silentaim::prediction_x);
        write_float(f,"sa.pred_y",       settings::silentaim::prediction_y);
        write_bool(f,"sa.sticky",        settings::silentaim::sticky_aim);
        write_bool(f,"sa.use_ab_target", settings::silentaim::use_aimbot_target);
        write_bool(f,"sa.teamcheck",     settings::silentaim::teamcheck);
        write_bool(f,"sa.knock_check",   settings::silentaim::knock_check);
        write_bool(f,"sa.dead_check",    settings::silentaim::dead_check);
        write_bool(f,"sa.vis_check",     settings::silentaim::visible_check);
        write_bool(f,"sa.health_check",  settings::silentaim::health_check_enabled);
        write_float(f,"sa.min_health",   settings::silentaim::min_health);
        write_bool(f,"sa.draw_dot",      settings::silentaim::draw_target_dot);
        write_col4(f,"sa.dot_col",       settings::silentaim::target_dot_color);
        write_float(f,"sa.dot_size",     settings::silentaim::target_dot_size);
        write_bool(f,"vl.aimline",      settings::visuals::aim_line);
        write_int (f,"vl.aimline_sty",  settings::visuals::aim_line_style);
        write_col4(f,"vl.aimline_col",  settings::visuals::aim_line_color);
        write_float(f,"vl.aimline_th",  settings::visuals::aim_line_thickness);
        write_bool(f,"vl.aimline_ctr",  settings::visuals::aim_line_centre);
        write_bool(f,"vl.aimline_sa",   settings::visuals::aim_line_silent);
        write_col4(f,"sa.fov_col",       settings::silentaim::fov_circle_colour);
        write_col4(f,"sa.fov_out_col",   settings::silentaim::fov_outline_colour);
        write_bool(f,"sa.autopred",      settings::silentaim_autopred::enabled);
        write_float(f,"sa.autopred_str", settings::silentaim_autopred::strength);
        write_bool(f,"sa.mouse_spoof",   settings::mouse_spoof::enabled);

        write_bool(f,"sa.hit_chance_en", settings::silentaim::hit_chance_enabled);
        write_float(f,"sa.hit_chance",   settings::silentaim::hit_chance);
        write_float(f,"sa.hit_chance_fr",settings::silentaim::hit_chance_fire_rate);
        write_bool(f,"sa.curve_tracer",  settings::silentaim::draw_curve_tracer);
        write_col4(f,"sa.ct_col",        settings::silentaim::curve_tracer_color);
        write_float(f,"sa.ct_thick",     settings::silentaim::curve_tracer_thickness);
        write_bool (f,"sa.ct_outline",   settings::silentaim::curve_tracer_outline);
        write_int  (f,"sa.ct_target",    settings::silentaim::curve_tracer_target);

        write_bool (f,"op1.enable",      settings::op1::enable);
        write_bool (f,"op1.force",       settings::op1::force);
        write_bool (f,"op1.aliases",     settings::op1::part_aliases);
        write_bool (f,"op1.body",        settings::op1::animated_body);
        write_bool (f,"op1.coll_boxes",  settings::op1::collision_boxes);
        write_bool (f,"op1.engine_mat",  settings::op1::engine_matrix);
        write_bool (f,"op1.multi_los",   settings::op1::multipoint_los);
        write_bool (f,"op1.live_health", settings::op1::live_health);
        write_bool (f,"op1.no_smooth",   settings::op1::no_smoothing);
        write_int  (f,"op1.sticky_ms",   settings::op1::sticky_ms);

        write_bool(f,"mv.speed_en",      settings::movement::speedhack::enabled);
        write_int(f,"mv.speed_kb",       settings::movement::speedhack::keybind);
        write_int(f,"mv.speed_mode",     settings::movement::speedhack::activation_mode);
        write_int(f,"mv.speed_type",     settings::movement::speedhack::mode);
        write_float(f,"mv.speed_val",    settings::movement::speedhack::speed);
        write_bool(f,"mv.fly_en",        settings::movement::flyhack::enabled);
        write_int(f,"mv.fly_kb",         settings::movement::flyhack::keybind);
        write_int(f,"mv.fly_mode",       settings::movement::flyhack::activation_mode);
        write_int(f,"mv.fly_type",       settings::movement::flyhack::mode);
        write_float(f,"mv.fly_val",      settings::movement::flyhack::speed);
        write_bool(f,"mv.tickrate",      settings::movement::tickrate::enabled);
        write_float(f,"mv.tickrate_val", settings::movement::tickrate::value);

        write_bool(f,"rage.spin",        settings::spinbot::enabled);
        write_bool(f,"rage.rapidfire",   settings::rage::rapidfire);

        write_bool(f,"misc.noclip",      settings::noclip::enabled);
        write_int(f,"misc.noclip_kb",    settings::noclip::keybind);
        write_int(f,"misc.noclip_mode",  settings::noclip::activation_mode);
        write_bool(f,"misc.nojumpcd",    settings::nojumpcooldown::enabled);

        write_bool(f,"world.time_on",    settings::world::lighting::time_on);
        write_bool(f,"world.bright_on",  settings::world::lighting::brightness_on);
        write_bool(f,"world.exp_on",     settings::world::lighting::exposure_on);
        write_bool(f,"world.amb_on",     settings::world::lighting::ambient_on);
        write_bool(f,"world.out_on",     settings::world::lighting::outdoor_on);
        write_bool(f,"world.fog_on",     settings::world::lighting::fog_on);
        write_bool(f,"world.shift_on",   settings::world::lighting::shift_on);
        write_bool(f,"world.env_on",     settings::world::lighting::env_on);
        write_bool(f,"world.no_shadow",  settings::world::lighting::no_shadow);
        write_float(f,"world.brightness",settings::world::lighting::brightness);
        write_float(f,"world.exposure",  settings::world::lighting::exposure_compensation);
        write_float(f,"world.clock",     settings::world::lighting::clock_time);
        write_float(f,"world.fog_start", settings::world::lighting::fog_start);
        write_float(f,"world.fog_end",   settings::world::lighting::fog_end);
        write_col3(f,"world.ambient",    settings::world::lighting::ambient);
        write_col3(f,"world.outdoor_amb",settings::world::lighting::outdoor_ambient);
        write_col3(f,"world.fog_col",    settings::world::lighting::fog_color);
        write_col3(f,"world.shift_top",  settings::world::lighting::color_shift_top);
        write_col3(f,"world.shift_bot",  settings::world::lighting::color_shift_bottom);
        write_float(f,"world.diffuse",   settings::world::lighting::env_diffuse_scale);
        write_float(f,"world.specular",  settings::world::lighting::env_specular_scale);
        write_bool(f,"world.atm_en",     settings::world::atmosphere::enabled);
        write_col3(f,"world.atm_col",    settings::world::atmosphere::color);
        write_col3(f,"world.atm_decay",  settings::world::atmosphere::decay);
        write_float(f,"world.density",   settings::world::atmosphere::density);
        write_float(f,"world.offset",    settings::world::atmosphere::offset);
        write_float(f,"world.haze",      settings::world::atmosphere::haze);
        write_float(f,"world.glare",     settings::world::atmosphere::glare);

        write_bool(f,"pfx.bloom",        settings::world::postfx::bloom::enabled);
        write_float(f,"pfx.bloom_int",   settings::world::postfx::bloom::intensity);
        write_float(f,"pfx.bloom_sz",    settings::world::postfx::bloom::size);
        write_float(f,"pfx.bloom_thr",   settings::world::postfx::bloom::threshold);
        write_bool(f,"pfx.cc",           settings::world::postfx::colorcorrect::enabled);
        write_col3(f,"pfx.cc_tint",      settings::world::postfx::colorcorrect::tint);
        write_float(f,"pfx.cc_br",       settings::world::postfx::colorcorrect::brightness);
        write_float(f,"pfx.cc_con",      settings::world::postfx::colorcorrect::contrast);
        write_bool(f,"pfx.dof",          settings::world::postfx::dof::enabled);
        write_float(f,"pfx.dof_fd",      settings::world::postfx::dof::focus_distance);
        write_float(f,"pfx.dof_fr",      settings::world::postfx::dof::in_focus_radius);
        write_float(f,"pfx.dof_fi",      settings::world::postfx::dof::far_intensity);
        write_float(f,"pfx.dof_ni",      settings::world::postfx::dof::near_intensity);
        write_bool(f,"pfx.sr",           settings::world::postfx::sunrays::enabled);
        write_float(f,"pfx.sr_int",      settings::world::postfx::sunrays::intensity);
        write_float(f,"pfx.sr_spd",      settings::world::postfx::sunrays::spread);

        write_bool(f,"shd.vig",          settings::world::shaders::vignette);
        write_float(f,"shd.vig_str",     settings::world::shaders::vignette_strength);
        write_float(f,"shd.vig_rad",     settings::world::shaders::vignette_radius);
        write_bool(f,"shd.chrab",        settings::world::shaders::chromatic_aberration);
        write_float(f,"shd.chr_str",     settings::world::shaders::chromatic_strength);
        write_bool(f,"shd.grain",        settings::world::shaders::film_grain);
        write_float(f,"shd.grain_str",   settings::world::shaders::film_grain_strength);
        write_bool(f,"shd.glossy",       settings::world::shaders::glossy);
        write_float(f,"shd.glos_spc",    settings::world::shaders::glossy_specular);
        write_float(f,"shd.glos_dif",    settings::world::shaders::glossy_diffuse);
        write_bool(f,"shd.glos_shim",    settings::world::shaders::glossy_shimmer);
        write_bool(f,"shd.contrast",     settings::world::shaders::contrast);
        write_float(f,"shd.con_val",     settings::world::shaders::contrast_value);
        write_float(f,"shd.bri_val",     settings::world::shaders::brightness_value);
        write_bool(f,"preset.night",     settings::world::presets::night);
        write_bool(f,"preset.sunset",    settings::world::presets::sunset);
        write_bool(f,"preset.rainbow",   settings::world::presets::rainbow);
        write_bool(f,"preset.fogpulse",  settings::world::presets::fog_pulse);

        write_int(f,"world.snow_mode",    settings::world::effects_3d::snow_mode);
        write_float(f,"world.snow_int",   settings::world::effects_3d::snow_intensity);
        write_float(f,"world.snow_spd",   settings::world::effects_3d::snow_speed);
        write_col3(f,"world.snow_col",    settings::world::effects_3d::snow_color);
        write_int(f,"world.rain_mode",    settings::world::effects_3d::rain_mode);
        write_int(f,"world.storm_mode",   settings::world::effects_3d::storm_mode);
        write_float(f,"world.rain_int",   settings::world::effects_3d::rain_intensity);
        write_float(f,"world.rain_spd",   settings::world::effects_3d::rain_speed);
        write_col3(f,"world.rain_col",    settings::world::effects_3d::rain_color);
        write_float(f,"world.wind_x",     settings::world::effects_3d::wind_x);
        write_float(f,"world.wind_z",     settings::world::effects_3d::wind_z);
        write_bool(f,"world.ripples",     settings::world::effects_3d::ripples);
        write_float(f,"world.ripple_sz",  settings::world::effects_3d::ripple_size);
        write_bool(f,"world.rain_snd",    settings::world::effects_3d::rain_sound);
        write_float(f,"world.rain_vol",   settings::world::effects_3d::rain_volume);

        write_bool (f,"world.light_on",   settings::world::lighting::light_on);
        write_col3 (f,"world.light_col",  settings::world::lighting::light_color);
        write_col3 (f,"world.light_dir",  settings::world::lighting::light_dir);
        write_bool (f,"world.sky",        settings::world::sky_slots::enabled);
        write_float(f,"world.sky_sun",    settings::world::sky_slots::sun_angular);
        write_float(f,"world.sky_moon",   settings::world::sky_slots::moon_angular);
        write_col3 (f,"world.sky_orient", settings::world::sky_slots::orientation);
        write_bool (f,"world.terrain",    settings::world::terrain::enabled);
        write_float(f,"world.grass_len",  settings::world::terrain::grass_length);
        write_col3 (f,"world.grass_col",  settings::world::terrain::grass_color);
        write_col3 (f,"world.water_col",  settings::world::terrain::water_color);
        write_float(f,"world.water_refl", settings::world::terrain::water_reflectance);
        write_float(f,"world.water_trans",settings::world::terrain::water_transparency);
        write_bool (f,"world.colorgrade", settings::world::postfx::colorgrade::enabled);
        write_int  (f,"world.tonemapper", settings::world::postfx::colorgrade::tonemapper);

        write_int(f,"ui.theme",          settings::theme::index);
        write_int(f,"ui.menu_key",       sync::state().menu_key);
        write_bool(f,"ui.dim",           sync::state().dim_background);
        write_float(f,"ui.glass",        sync::state().glass_transparency);
        write_bool(f,"ui.wm_fps",        sync::state().wm_fps);
        write_bool(f,"ui.wm_time",       sync::state().wm_time);
        write_bool(f,"ui.wm_show",       sync::state().wm_show);
        write_bool(f,"ui.wm_hide",       settings::menu::hide_watermark);
        write_bool(f,"ui.ignore_place",  settings::menu::ignore_unsupported_place);
        write_bool (f,"ui.keybind_list", sync::state().kb_show);
        write_bool (f,"ui.kb_mode",      sync::state().kb_mode);
        write_bool (f,"ui.kb_active",    sync::state().kb_only_active);
        write_float(f,"ui.kb_x",         sync::state().kb_x);
        write_float(f,"ui.kb_y",         sync::state().kb_y);
        write_bool (f,"ui.th_show",      sync::state().th_show);
        write_bool (f,"ui.th_avatar",    sync::state().th_avatar);
        write_bool (f,"ui.th_name",      sync::state().th_name);
        write_bool (f,"ui.th_health",    sync::state().th_health);
        write_bool (f,"ui.th_tool",      sync::state().th_tool);
        write_float(f,"ui.th_x",         sync::state().th_x);
        write_float(f,"ui.th_y",         sync::state().th_y);
        write_float(f,"ui.th_scale",     sync::state().th_scale);

        write_bool(f,"ui.custom_accent", settings::appearance::custom_accent);
        write_float(f,"ui.accent_spread", settings::appearance::accent_spread);
        write_col4(f,"ui.accent",        settings::appearance::accent);
        write_bool(f,"ui.win_gradient",  settings::appearance::window_gradient);
        write_bool(f,"ui.win_shadow",    settings::appearance::window_shadow);
        write_bool(f,"ui.accent_glow",   settings::appearance::accent_glow);
        write_float(f,"ui.glow",         settings::appearance::glow_strength);
        write_float(f,"ui.rounding",     settings::appearance::rounding);
        write_float(f,"ui.panel_alpha",  settings::appearance::panel_alpha);
        write_float(f,"ui.dock_alpha",   settings::appearance::dock_alpha);
        write_float(f,"ui.frost",        settings::appearance::glass_sheen);
        write_bool (f,"ui.opaque",       settings::appearance::no_transparency);
        write_float(f,"ui.panel_contrast",settings::appearance::panel_contrast);
        write_bool (f,"ui.aa",           settings::appearance::antialiasing);
        write_float(f,"ui.aa_q",         settings::appearance::aa_quality);
        write_bool (f,"bd.on",            settings::backdrop::enabled);
        // New key: the default moved from 65 to 75, and a key that already
        // holds 65 in an existing config would keep the old value forever.
        write_float(f,"bd.blur75",        settings::backdrop::blur);
        write_float(f,"bd.tint",          settings::backdrop::tint);
        write_bool (f,"bd.tintonly",      settings::backdrop::tint_only);
        write_float(f,"bd.pad",           settings::backdrop::capture_pad);
        write_float(f,"bd.cap",           settings::backdrop::capture_cap);
        write_bool (f,"bd.status",        settings::backdrop::status_line);
        write_bool(f,"ui.nav_labels",    settings::appearance::nav_labels);
        write_bool(f,"ui.switches",      settings::appearance::switches);
        write_int(f,"ui.set_tab",        settings::appearance::settings_tab);
        write_bool(f,"ui.smooth_scroll",  settings::appearance::smooth_scroll);
        write_float(f,"ui.scroll_speed",  settings::appearance::scroll_speed);
        write_bool(f,"ui.page_anim",      settings::appearance::page_animation);
        write_bool(f,"ui.widget_anim",    settings::appearance::widget_animations);

        write_bool(f,"ui.bg_enabled",    settings::appearance::bg_enabled);
        write_int(f,"ui.bg_effect",      settings::appearance::bg_effect);
        write_float(f,"ui.bg_density",   settings::appearance::bg_density);
        write_float(f,"ui.bg_speed",     settings::appearance::bg_speed);
        write_float(f,"ui.bg_size",      settings::appearance::bg_size);
        write_float(f,"ui.bg_opacity",   settings::appearance::bg_opacity);
        write_col4(f,"ui.bg_color",      settings::appearance::bg_color);
        write_float(f,"ui.bg_wind",      settings::appearance::bg_wind);
        write_float(f,"ui.bg_glow",      settings::appearance::bg_glow);
        write_bool(f,"ui.bg_parallax",   settings::appearance::bg_parallax);
        write_bool(f,"ui.bg_menu_only",  settings::appearance::bg_menu_only);

        // Which way the falling elements fall, and how they are drawn.  These
        // were rows in the Config page with no key behind them - a rain angle
        // chosen in the menu was gone at the next launch.
        write_int (f,"ui.bg_dir",       settings::appearance::bg_dir);
        write_float(f,"ui.bg_angle",     settings::appearance::bg_angle);
        write_float(f,"ui.bg_streak",    settings::appearance::bg_streak);
        write_float(f,"ui.bg_spread",   settings::appearance::bg_spread);
        write_float(f,"ui.bg_width",    settings::appearance::bg_width);

        write_bool (f,"ui.enhance",      settings::appearance::enhance);
        write_float(f,"ui.enhance_size", settings::appearance::enhance_size);
        write_float(f,"ui.panel_width", settings::appearance::panel_width);
        write_bool (f,"ui.test_enhance", settings::appearance::test_enhance);


        write_bool(f,"sky.enabled",      settings::skybox::enabled);
        write_int(f,"sky.shader",        settings::skybox::shader);
        write_int(f,"sky.video",         settings::skybox::video);

        write_bool (f,"hm.greet",        settings::home::greeting);
        write_bool (f,"hm.clock",        settings::home::clock);
        write_int  (f,"hm.clockf",       settings::home::clock_format);
        write_bool (f,"hm.avatar",       settings::home::avatar);
        write_bool (f,"hm.avring",       settings::home::avatar_ring);
        write_bool (f,"hm.avglow",       settings::home::avatar_glow);
        write_bool (f,"hm.cards",        settings::home::cards);
        write_bool (f,"hm.count",        settings::home::count_up);
        write_float(f,"hm.stagger",      settings::home::stagger);
        write_bool (f,"hm.act",          settings::home::activity);
        write_bool (f,"hm.act_fill",     settings::home::activity_fill);
        write_bool (f,"hm.act_sm",       settings::home::activity_smooth);
        write_bool (f,"hm.act_grid",     settings::home::activity_grid);
        write_bool (f,"hm.act_hov",      settings::home::activity_hover);
        write_bool (f,"hm.near",         settings::home::nearby);
        write_int  (f,"hm.near_rows",    settings::home::nearby_rows);
        write_bool (f,"hm.near_faces",   settings::home::nearby_faces);
        write_bool (f,"hm.near_bars",    settings::home::nearby_bars);

        write_bool (f,"dk.on",           settings::docks::enabled);
        write_float(f,"dk.opacity",      settings::docks::opacity);
        write_float(f,"dk.scale",        settings::docks::scale);
        write_bool (f,"dk.shadow",       settings::docks::shadow);
        write_bool (f,"dk.snap",         settings::docks::snap);
        write_float(f,"dk.snap_px",      settings::docks::snap_px);
        write_float(f,"dk.snap_gap",     settings::docks::snap_gap);
        write_bool (f,"dk.lift",         settings::docks::hover_lift);
        write_bool (f,"dk.fold",         settings::docks::fold_on_click);
        write_bool (f,"dk.anim",         settings::docks::animate);
        write_float(f,"dk.anim_s",       settings::docks::anim_speed);
        write_bool (f,"dk.titles",       settings::docks::show_titles);
        write_bool (f,"dk.grip",         settings::docks::show_grip);
        for (int i = 0; i < settings::docks::kCount; ++i)
        {
            char k[32];
            snprintf(k, sizeof k, "dk.open.%d", i); write_bool (f, k, settings::docks::open[i]);
            snprintf(k, sizeof k, "dk.fold.%d", i); write_bool (f, k, settings::docks::folded[i]);
            snprintf(k, sizeof k, "dk.x.%d",    i); write_float(f, k, settings::docks::x[i]);
            snprintf(k, sizeof k, "dk.y.%d",    i); write_float(f, k, settings::docks::y[i]);
        }

        write_bool (f,"viz.on",          settings::viz::enabled);
        write_int  (f,"viz.mode",        settings::viz::mode);
        write_int  (f,"viz.window",      settings::viz::window);
        write_bool (f,"viz.grid",        settings::viz::grid);
        write_bool (f,"viz.glow",        settings::viz::glow);
        write_float(f,"viz.thick",       settings::viz::thickness);
        write_bool (f,"viz.band",        settings::viz::band);
        write_bool (f,"viz.readouts",    settings::viz::readouts);
        write_bool (f,"viz.hover",       settings::viz::hover_value);

        write_bool (f,"viz.demo",        settings::viz::demo_switch);
        write_float(f,"viz.demo_s",      settings::viz::demo_speed);
        write_float(f,"viz.demo_x",      settings::viz::demo_x);
        write_float(f,"viz.demo_y",      settings::viz::demo_y);
        write_int  (f,"viz.pin",         settings::viz::pin_style);
        write_col4 (f,"viz.col_sm",      settings::viz::smooth_color);
        write_col4 (f,"viz.col_raw",     settings::viz::raw_color);
        write_col4 (f,"viz.col_err",     settings::viz::error_color);

        // Everything below used to live only in memory: these features came
        // back at their defaults on every launch. Kept in one block so the next
        // feature added has somewhere obvious to be written.
        // visuals
        write_bool (f,"vis.trail",settings::visuals::trail);
        write_int  (f,"vis.trail_style",settings::visuals::trail_style);
        write_int  (f,"vis.trail_color_mode",settings::visuals::trail_color_mode);
        write_col4(f,"vis.trail_color",settings::visuals::trail_color);
        write_col4(f,"vis.trail_head_color",settings::visuals::trail_head_color);
        write_float(f,"vis.trail_thickness",settings::visuals::trail_thickness);
        write_float(f,"vis.trail_duration",settings::visuals::trail_duration);
        write_int  (f,"vis.trail_max_points",settings::visuals::trail_max_points);
        write_float(f,"vis.trail_update_interval",settings::visuals::trail_update_interval);
        write_float(f,"vis.trail_min_movement",settings::visuals::trail_min_movement);
        write_bool (f,"vis.trail_only_moving",settings::visuals::trail_only_moving);
        write_int  (f,"vis.trail_follow",settings::visuals::trail_follow);
        write_bool (f,"vis.trail_taper",settings::visuals::trail_taper);
        write_bool (f,"vis.trail_glow",settings::visuals::trail_glow);
        write_float(f,"vis.trail_glow_intensity",settings::visuals::trail_glow_intensity);
        write_int  (f,"vis.trail_glow_layers",settings::visuals::trail_glow_layers);
        write_bool (f,"vis.trail_fade",settings::visuals::trail_fade);
        write_float(f,"vis.trail_fade_start",settings::visuals::trail_fade_start);
        write_bool (f,"vis.trail_smooth",settings::visuals::trail_smooth);
        write_int  (f,"vis.trail_spline_segments",settings::visuals::trail_spline_segments);
        write_bool (f,"vis.trail_head_spark",settings::visuals::trail_head_spark);
        write_bool (f,"vis.trail_height_fall",settings::visuals::trail_height_fall);
        write_float(f,"vis.trail_height_drop",settings::visuals::trail_height_drop);
        write_bool (f,"vis.trail_team_check",settings::visuals::trail_team_check);
        write_bool (f,"vis.name_sheen",settings::visuals::name_sheen);
        write_float(f,"vis.name_sheen_speed",settings::visuals::name_sheen_speed);
        write_float(f,"vis.name_sheen_width",settings::visuals::name_sheen_width);
        write_float(f,"vis.name_sheen_strength",settings::visuals::name_sheen_strength);
        write_bool (f,"vis.esp_sheen_all",settings::visuals::esp_sheen_all);
        write_float(f,"vis.healthbar_padding",settings::visuals::healthbar_padding);
        // effects
        write_int  (f,"fx.hitmarker_style",settings::effects::hitmarker_style);
        write_int  (f,"fx.tracer_style",settings::effects::tracer_style);
        write_bool (f,"fx.hitmarker_shockwave",settings::effects::hitmarker_shockwave);
        write_float(f,"fx.hitmarker_shockwave_size",settings::effects::hitmarker_shockwave_size);
        write_bool (f,"fx.hitmarker_sparks",settings::effects::hitmarker_sparks);
        write_int  (f,"fx.hitmarker_spark_count",settings::effects::hitmarker_spark_count);
        write_float(f,"fx.hitmarker_spark_speed",settings::effects::hitmarker_spark_speed);
        write_bool (f,"fx.hitmarker_flash",settings::effects::hitmarker_flash);
        write_bool (f,"fx.hitmarker_streak",settings::effects::hitmarker_streak);
        write_float(f,"fx.hitmarker_streak_window",settings::effects::hitmarker_streak_window);
        write_bool (f,"fx.hitmarker_crit_pulse",settings::effects::hitmarker_crit_pulse);
        write_float(f,"fx.hitmarker_screen_flash",settings::effects::hitmarker_screen_flash);
        write_bool (f,"fx.hitmarker_outline",settings::effects::hitmarker_outline);
        write_float(f,"fx.hitmarker_glow",settings::effects::hitmarker_glow);
        write_float(f,"fx.hitmarker_pop",settings::effects::hitmarker_pop);
        write_float(f,"fx.crosshair_len_x",settings::effects::crosshair_len_x);
        write_float(f,"fx.crosshair_len_y",settings::effects::crosshair_len_y);
        write_float(f,"fx.crosshair_taper",settings::effects::crosshair_taper);
        write_bool (f,"fx.crosshair_spin_windup",settings::effects::crosshair_spin_windup);
        write_int  (f,"fx.crosshair_style",settings::effects::crosshair_style);
        write_float(f,"fx.crosshair_glow",settings::effects::crosshair_glow);
        write_bool (f,"fx.crosshair_rainbow",settings::effects::crosshair_rainbow);
        write_float(f,"fx.crosshair_rainbow_speed",settings::effects::crosshair_rainbow_speed);
        write_bool (f,"fx.crosshair_hit_pulse",settings::effects::crosshair_hit_pulse);
        write_float(f,"fx.crosshair_hit_pulse_size",settings::effects::crosshair_hit_pulse_size);
        write_bool (f,"fx.crosshair_dynamic",settings::effects::crosshair_dynamic);
        write_float(f,"fx.crosshair_dynamic_amount",settings::effects::crosshair_dynamic_amount);
        // sonar
        write_bool (f,"son.enabled",settings::sonar::enabled);
        write_int  (f,"son.keybind",settings::sonar::keybind);
        write_int  (f,"son.activation_mode",settings::sonar::activation_mode);
        write_int  (f,"son.style",settings::sonar::style);
        write_float(f,"son.range",settings::sonar::range);
        write_float(f,"son.sweep_speed",settings::sonar::sweep_speed);
        write_bool (f,"son.sweep_back",settings::sonar::sweep_back);
        write_float(f,"son.beam_width",settings::sonar::beam_width);
        write_col4(f,"son.color",settings::sonar::color);
        write_col4(f,"son.ping_color",settings::sonar::ping_color);
        write_bool (f,"son.glow",settings::sonar::glow);
        write_float(f,"son.glow_strength",settings::sonar::glow_strength);
        write_bool (f,"son.ping_rings",settings::sonar::ping_rings);
        write_float(f,"son.ping_life",settings::sonar::ping_life);
        write_float(f,"son.ping_radius",settings::sonar::ping_radius);
        write_int  (f,"son.info_mode",settings::sonar::info_mode);
        write_float(f,"son.info_size",settings::sonar::info_size);
        write_bool (f,"son.latch",settings::sonar::latch);
        write_float(f,"son.hold_time",settings::sonar::hold_time);
        write_bool (f,"son.ping_sound",settings::sonar::ping_sound);
        write_float(f,"son.ping_volume",settings::sonar::ping_volume);
        write_float(f,"son.beam_reach",settings::sonar::beam_reach);
        write_float(f,"son.beam_taper",settings::sonar::beam_taper);
        write_float(f,"son.tail_length",settings::sonar::tail_length);
        write_float(f,"son.tail_strength",settings::sonar::tail_strength);
        write_bool (f,"son.grid",settings::sonar::grid);
        write_int  (f,"son.grid_rings",settings::sonar::grid_rings);
        write_float(f,"son.grid_alpha",settings::sonar::grid_alpha);
        write_bool (f,"son.grid_labels",settings::sonar::grid_labels);
        write_int  (f,"son.ping_style",settings::sonar::ping_style);
        write_bool (f,"son.ping_ticks",settings::sonar::ping_ticks);
        write_bool (f,"son.ping_spark",settings::sonar::ping_spark);
        write_bool (f,"son.ping_drop",settings::sonar::ping_drop);
        write_bool (f,"son.readout_plate",settings::sonar::readout_plate);
        write_bool (f,"son.los_only",settings::sonar::los_only);
        write_int  (f,"son.color_mode",settings::sonar::color_mode);
        write_float(f,"son.hue_speed",settings::sonar::hue_speed);
        write_bool (f,"son.hide_with_menu",settings::sonar::hide_with_menu);
        // misc_ui
        write_bool (f,"uifx.enabled",settings::misc_ui::enabled);
        write_float(f,"uifx.intensity",settings::misc_ui::intensity);
        write_bool (f,"uifx.cursor_aura",settings::misc_ui::cursor_aura);
        write_float(f,"uifx.cursor_aura_size",settings::misc_ui::cursor_aura_size);
        write_bool (f,"uifx.bloom_soft",settings::misc_ui::bloom_soft);
        write_bool (f,"uifx.cursor_trail",settings::misc_ui::cursor_trail);
        write_float(f,"uifx.cursor_trail_length",settings::misc_ui::cursor_trail_length);
        write_bool (f,"uifx.click_shockwave",settings::misc_ui::click_shockwave);
        write_int  (f,"uifx.click_style",settings::misc_ui::click_style);
        write_bool (f,"uifx.click_spark",settings::misc_ui::click_spark);
        write_bool (f,"uifx.hover_sheen",settings::misc_ui::hover_sheen);
        write_float(f,"uifx.hover_sheen_speed",settings::misc_ui::hover_sheen_speed);
        write_float(f,"uifx.hover_sheen_strength",settings::misc_ui::hover_sheen_strength);
        write_bool (f,"uifx.sheen_soft",settings::misc_ui::sheen_soft);
        write_int  (f,"uifx.tint",settings::misc_ui::tint);
        write_bool (f,"uifx.hover_tint",settings::misc_ui::hover_tint);
        write_bool (f,"uifx.hover_lift",settings::misc_ui::hover_lift);
        write_bool (f,"uifx.press_squish",settings::misc_ui::press_squish);
        write_bool (f,"uifx.focus_ring",settings::misc_ui::focus_ring);
        write_bool (f,"uifx.chip_pop",settings::misc_ui::chip_pop);
        write_bool (f,"uifx.toggle_overshoot",settings::misc_ui::toggle_overshoot);
        write_bool (f,"uifx.panel_rim",settings::misc_ui::panel_rim);
        write_float(f,"uifx.panel_rim_size",settings::misc_ui::panel_rim_size);
        write_bool (f,"uifx.nav_breath",settings::misc_ui::nav_breath);
        write_bool (f,"uifx.indicator_glow",settings::misc_ui::indicator_glow);
        write_bool (f,"uifx.stagger",settings::misc_ui::stagger);
        write_float(f,"uifx.stagger_step",settings::misc_ui::stagger_step);
        write_int  (f,"uifx.cascade_mode",settings::misc_ui::cascade_mode);
        write_int  (f,"uifx.cascade_mode_view",settings::misc_ui::cascade_mode_view);
        write_bool (f,"uifx.cascade_pages",settings::misc_ui::cascade_pages);
        write_bool (f,"uifx.cascade_views",settings::misc_ui::cascade_views);
        write_bool (f,"uifx.cascade_hairline",settings::misc_ui::cascade_hairline);
        write_float(f,"uifx.cascade_time",settings::misc_ui::cascade_time);
        write_float(f,"uifx.cascade_rise",settings::misc_ui::cascade_rise);
        write_float(f,"uifx.cascade_drift",settings::misc_ui::cascade_drift);
        write_bool (f,"uifx.slider_hd",settings::misc_ui::slider_hd);
        write_float(f,"uifx.slider_hd_size",settings::misc_ui::slider_hd_size);
        write_bool (f,"uifx.tooltip_fade",settings::misc_ui::tooltip_fade);
        write_bool (f,"uifx.edge_fade",settings::misc_ui::edge_fade);
        write_int  (f,"uifx.menu_font",settings::misc_ui::menu_font);
        write_bool (f,"uifx.first_pop",settings::misc_ui::first_pop);
        write_bool (f,"uifx.open_flash",settings::misc_ui::open_flash);
        write_float(f,"uifx.open_flash_strength",settings::misc_ui::open_flash_strength);
        write_bool (f,"uifx.click_sounds",settings::misc_ui::click_sounds);
        write_float(f,"uifx.click_volume",settings::misc_ui::click_volume);
        // ui21
        write_bool (f,"ui21.enabled",settings::ui21::enabled);
        write_bool (f,"ui21.glass_layers",settings::ui21::glass_layers);
        write_float(f,"ui21.glass_depth",settings::ui21::glass_depth);
        write_bool (f,"ui21.spotlight_border",settings::ui21::spotlight_border);
        write_float(f,"ui21.spotlight_size",settings::ui21::spotlight_size);
        write_bool (f,"ui21.gradient_heading",settings::ui21::gradient_heading);
        write_bool (f,"ui21.odometer",settings::ui21::odometer);
        write_bool (f,"ui21.dot_grid",settings::ui21::dot_grid);
        write_float(f,"ui21.dot_grid_spacing",settings::ui21::dot_grid_spacing);
        write_col4(f,"ui21.accent_2",settings::ui21::accent_2);
        write_bool (f,"ui21.noise",settings::ui21::noise);
        write_float(f,"ui21.noise_amount",settings::ui21::noise_amount);
        write_bool (f,"ui21.light_rays",settings::ui21::light_rays);
        write_float(f,"ui21.light_strength",settings::ui21::light_rays_strength);
        write_float(f,"ui21.light_spread",settings::ui21::light_rays_spread);
        write_bool (f,"ui21.aurora",settings::ui21::aurora);
        write_float(f,"ui21.aurora_strength",settings::ui21::aurora_strength);
        write_bool (f,"ui21.panel_glow",settings::ui21::panel_glow);
        write_bool (f,"ui21.card_glow",settings::ui21::card_glow);
        write_float(f,"ui21.card_glow_strength",settings::ui21::card_glow_strength);
        write_bool (f,"ui21.panel_shadow",settings::ui21::panel_shadow);
        write_float(f,"ui21.panel_shadow_strength",settings::ui21::panel_shadow_strength);
        write_bool (f,"ui21.top_sheen",settings::ui21::top_sheen);
        write_float(f,"ui21.top_sheen_speed",settings::ui21::top_sheen_speed);
        // home
        write_bool (f,"home.card_icons",settings::home::card_icons);
        // docks
        write_bool (f,"dk.behind_menu",settings::docks::behind_menu);
        // viz
        write_float(f,"viz.scale",settings::viz::scale);
        // aimbot
        write_bool (f,"ab.fov_fill",settings::aimbot::fov_fill);
        write_col4(f,"ab.fov_fill_colour",settings::aimbot::fov_fill_colour);
        write_float(f,"ab.fov_fill_opacity",settings::aimbot::fov_fill_opacity);
        write_float(f,"ab.fov_th",settings::aimbot::fov_thickness);
        write_bool (f,"ab.fov_outline",settings::aimbot::fov_outline);
        write_float(f,"ab.tr_th",settings::aimbot::tracer_thickness);
        write_bool (f,"ab.tr_outline",settings::aimbot::tracer_outline);
        write_int (f,"ab.tr_style",settings::aimbot::tracer_style);
        write_int (f,"ab.fov_style",settings::aimbot::fov_style);
        // silentaim
        write_bool (f,"sa.fov_fill",settings::silentaim::fov_fill);
        write_col4(f,"sa.fov_fill_colour",settings::silentaim::fov_fill_colour);
        write_float(f,"sa.fov_fill_opacity",settings::silentaim::fov_fill_opacity);
        write_float(f,"sa.fov_th",settings::silentaim::fov_thickness);
        write_bool (f,"sa.fov_outline",settings::silentaim::fov_outline);
        write_int  (f,"sa.fov_style",settings::silentaim::fov_style);
        write_bool (f,"sa.spoof_mouse",settings::silentaim::spoof_mouse);
        write_bool (f,"sa.guncheck",settings::silentaim::guncheck);
        write_int  (f,"sa.priorities",settings::silentaim::priorities);
        // triggerbot
        write_col4(f,"tb.fov_circle_colour",settings::triggerbot::fov_circle_colour);
        write_col4(f,"tb.fov_outline_colour",settings::triggerbot::fov_outline_colour);
        write_float(f,"tb.fov_th",settings::triggerbot::fov_thickness);
        write_bool (f,"tb.fov_outline",settings::triggerbot::fov_outline);
        write_int  (f,"tb.fov_style",settings::triggerbot::fov_style);
        write_int  (f,"vis.skeleton_style",settings::visuals::skeleton_style);
        write_bool (f,"vis.skel_joints", settings::visuals::skeleton_joints);
        write_float(f,"vis.skel_joint_size", settings::visuals::skeleton_joint_size);
        write_int  (f,"vis.skel_segs",   settings::visuals::skeleton_vis_segments);
        write_col4 (f,"vis.skel_hidden", settings::visuals::skeleton_hidden_color);
        write_int  (f,"vis.radar_style",settings::visuals::radar_style);
        write_bool (f,"tb.fov_fill",settings::triggerbot::fov_fill);
        write_col4(f,"tb.fov_fill_colour",settings::triggerbot::fov_fill_colour);
        write_float(f,"tb.fov_fill_opacity",settings::triggerbot::fov_fill_opacity);
        write_bool (f,"tb.dead_check",settings::triggerbot::dead_check);
        // walkspeed
        write_bool (f,"ws.enabled",settings::walkspeed::enabled);
        write_float(f,"ws.speed",settings::walkspeed::speed);
        write_int  (f,"ws.keybind",settings::walkspeed::keybind);
        write_int  (f,"ws.keybind_mode",settings::walkspeed::keybind_mode);
        // streamproof
        write_bool (f,"spf.enabled",settings::streamproof::enabled);
        // skybox
        write_str (f,"sky.video_file",settings::skybox::video_file);
        write_bool (f,"sky.pre_on",   settings::skybox::preset_on);
        write_int  (f,"sky.pre",      settings::skybox::preset);
        write_bool (f,"sky.pre_faces",settings::skybox::preset_faces);
        write_bool (f,"sky.pre_sm",   settings::skybox::preset_sun_moon);
        write_bool (f,"sky.pre_stars",settings::skybox::preset_stars);
        write_bool (f,"sky.stars",    settings::skybox::stars_on);
        write_int  (f,"sky.starc",    settings::skybox::stars_count);
        write_bool (f,"sky.pre_cel",  settings::skybox::hide_celestial);
        write_bool (f,"sky.pre_ori",  settings::skybox::orientation_on);
        write_col3 (f,"sky.pre_oriv", settings::skybox::orientation);
        // exploits
        write_bool (f,"exp.low_gravity",settings::exploits::low_gravity);
        write_float(f,"exp.gravity_value",settings::exploits::gravity_value);
        write_bool (f,"exp.tp_cursor",settings::exploits::tp_cursor);
        write_int  (f,"exp.tp_cursor_key",settings::exploits::tp_cursor_key);
        write_float(f,"exp.tp_cursor_dist",settings::exploits::tp_cursor_dist);
        // source pack
        write_bool (f,"fu.antifling",settings::fusion::antifling);
        write_float(f,"fu.fling_lin",settings::fusion::antifling_lin);
        write_float(f,"fu.fling_ang",settings::fusion::antifling_ang);
        write_bool (f,"fu.jumppower",settings::fusion::jumppower);
        write_int  (f,"fu.jump_key",settings::fusion::jumppower_key);
        write_int  (f,"fu.jump_mode",settings::fusion::jumppower_mode);
        write_float(f,"fu.jump_value",settings::fusion::jumppower_value);
        write_bool (f,"fu.hipheight",settings::fusion::hipheight);
        write_float(f,"fu.hip_value",settings::fusion::hipheight_value);
        write_bool (f,"fu.freecam",settings::fusion::freecam);
        write_int  (f,"fu.free_key",settings::fusion::freecam_key);
        write_int  (f,"fu.free_mode",settings::fusion::freecam_mode);
        write_float(f,"fu.free_speed",settings::fusion::freecam_speed);
        write_float(f,"fu.free_sprint",settings::fusion::freecam_sprint);
        write_bool (f,"fu.free_revert",settings::fusion::freecam_revert);
        write_bool (f,"fu.orbit",settings::fusion::orbit);
        write_int  (f,"fu.orbit_key",settings::fusion::orbit_key);
        write_int  (f,"fu.orbit_act",settings::fusion::orbit_activation);
        write_int  (f,"fu.orbit_target",settings::fusion::orbit_target);
        write_float(f,"fu.orbit_radius",settings::fusion::orbit_radius);
        write_float(f,"fu.orbit_speed",settings::fusion::orbit_speed);
        write_float(f,"fu.orbit_height",settings::fusion::orbit_height);
        write_bool (f,"fu.orbit_spec",settings::fusion::orbit_spectate);
        write_bool (f,"fu.headless",settings::fusion::headless);
        write_bool (f,"fu.korblox",settings::fusion::korblox);
        write_bool (f,"fu.spectate",settings::fusion::spectate);
        write_int  (f,"fu.spec_key",settings::fusion::spectate_cycle_key);
        write_bool (f,"fu.flyabove",settings::fusion::flyabove);
        write_float(f,"fu.flyabove_h",settings::fusion::flyabove_height);
        write_bool (f,"fu.hitnotes",settings::fusion::hit_notifications);
        write_float(f,"fu.hitnotes_s",settings::fusion::hit_notification_seconds);
        write_bool (f,"fu.fps",settings::fusion::show_fps);
        // source pack
        write_bool (f,"fu.norecoil",settings::fusion::norecoil);
        write_int  (f,"fu.nr_key",settings::fusion::norecoil_key);
        write_float(f,"fu.nr_x",settings::fusion::norecoil_x);
        write_float(f,"fu.nr_y",settings::fusion::norecoil_y);
        write_float(f,"fu.nr_strength",settings::fusion::norecoil_strength);
        // theme
        write_col4(f,"theme.accent",settings::theme::accent);
        write_col4(f,"theme.bg_one",settings::theme::bg_one);
        write_col4(f,"theme.bg_two",settings::theme::bg_two);
        write_col4(f,"theme.stroke",settings::theme::stroke);
        write_col4(f,"theme.outline",settings::theme::outline);
        write_col4(f,"theme.text_active",settings::theme::text_active);
        write_col4(f,"theme.text_inactive",settings::theme::text_inactive);
        // menu
        write_bool (f,"ui.vsync",settings::menu::vsync);
        write_bool (f,"ui.performance_mode",settings::menu::performance_mode);
        write_int  (f,"ui.fps_limit",settings::menu::fps_limit);
        // settings
        write_bool (f,"app.hide_console",settings::settings::hide_console);
    }

    static void load(const std::string& name)
    {
        std::ifstream f(cfg_dir() + name + ".cfg");
        if (!f) return;
        std::string line;
        while (std::getline(f, line))
        {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);

            bool  bv   = (val == "1");
            float fval = 0.f; try { fval = (float)std::stod(val); } catch(...) {}
            int   ival = 0;   try { ival = std::stoi(val); }         catch(...) {}

            auto parse_c4 = [&](float* c) {
                float v[4]={1,1,1,1}; int i=0;
                std::stringstream ss(val); std::string tok;
                while(std::getline(ss,tok,',') && i<4) v[i++]=(float)std::stod(tok);
                c[0]=v[0];c[1]=v[1];c[2]=v[2];c[3]=v[3];
            };
            auto parse_c3 = [&](float* c) {
                float v[3]={1,1,1}; int i=0;
                std::stringstream ss(val); std::string tok;
                while(std::getline(ss,tok,',') && i<3) v[i++]=(float)std::stod(tok);
                c[0]=v[0];c[1]=v[1];c[2]=v[2];
            };

            try {

            if      (key=="vis.box")           settings::visuals::box=bv;
            else if (key=="vis.box_type")      settings::visuals::box_type=ival;
            else if (key=="vis.box_style")     settings::visuals::box_style=ival;
            else if (key=="vis.box_corner")    settings::visuals::box_corner_scale=fval;
            else if (key=="vis.box_mode")      settings::visuals::box_mode=ival;
            else if (key=="vis.box_color")     parse_c4(settings::visuals::box_color);
            else if (key=="vis.box_fill")      settings::visuals::box_fill=bv;
            else if (key=="vis.box_fill_mode") settings::visuals::box_fill_mode=ival;
            else if (key=="vis.box_fill_path") snprintf(settings::visuals::box_fill_image_path, sizeof settings::visuals::box_fill_image_path, "%s", val.c_str());
            else if (key=="vis.box_fill_done") settings::visuals::box_fill_image_loaded=bv;
            else if (key=="vis.box_fill_a")    settings::visuals::box_fill_image_alpha=fval;
            else if (key=="vis.box_fill_nobg")settings::visuals::box_fill_remove_bg=bv;
            else if (key=="vis.box_fill_col")  parse_c4(settings::visuals::box_fill_color);
            else if (key=="vis.box_gradient")  settings::visuals::box_gradient=bv;
            else if (key=="vis.box_grad_top")  parse_c4(settings::visuals::box_gradient_top);
            else if (key=="vis.box_grad_bot")  parse_c4(settings::visuals::box_gradient_bottom);
            else if (key=="vis.enable_enemies")settings::visuals::enable_enemies=bv;
            else if (key=="vis.knock_check")   settings::visuals::knock_check=bv;
            else if (key=="vis.team_check")    settings::visuals::team_check=bv;
            else if (key=="vis.dead_check")    settings::visuals::dead_check=bv;
            else if (key=="vis.vis_check")     settings::visuals::visible_check=bv;
            else if (key=="vis.box_body_only") settings::visuals::box_body_only=bv;
            else if (key=="vis.name")          settings::visuals::name=bv;
            else if (key=="vis.name_disp")     settings::visuals::name_display_type=ival;
            else if (key=="vis.name_col")      parse_c4(settings::visuals::name_color);
            else if (key=="vis.blend")         settings::visuals::blend=bv;
            else if (key=="vis.blend_start")   parse_c4(settings::visuals::name_color_blend_start);
            else if (key=="vis.blend_end")     parse_c4(settings::visuals::name_color_blend_end);
            else if (key=="vis.avatar")        settings::visuals::avatar=bv;
            else if (key=="vis.distance")      settings::visuals::distance=bv;
            else if (key=="vis.dist_meas")     settings::visuals::distance_measurement=ival;
            else if (key=="vis.dist_col")      parse_c4(settings::visuals::distance_color);
            else if (key=="vis.max_dist")      settings::visuals::max_distance=fval;
            else if (key=="vis.arrows")        settings::visuals::offscreen_arrows=bv;
            else if (key=="vis.arrows_col")    parse_c4(settings::visuals::arrow_color);
            else if (key=="vis.arrow_size")    settings::visuals::arrow_size=fval;
            else if (key=="vis.arrow_radius")  settings::visuals::arrow_radius=fval;
            else if (key=="vis.arrow_inf_n")   settings::visuals::arrow_info[0]=bv;
            else if (key=="vis.arrow_inf_d")   settings::visuals::arrow_info[1]=bv;
            else if (key=="vis.arrow_inf_h")   settings::visuals::arrow_info[2]=bv;
            else if (key=="vis.arrow_inf_t")   settings::visuals::arrow_info[3]=bv;
            else if (key=="vis.tracer")        settings::visuals::tracer=bv;
            else if (key=="vis.tracer_origin") settings::visuals::tracer_origin=ival;
            else if (key=="vis.tracer_end")    settings::visuals::tracer_end=ival;
            else if (key=="vis.tracer_th")     settings::visuals::tracer_thickness=fval;
            else if (key=="vis.tracer_outline")settings::visuals::tracer_outline=bv;
            else if (key=="vis.tracer_style")  settings::visuals::tracer_style=ival;
            else if (key=="vis.arrow_style")   settings::visuals::arrow_style=ival;
            else if (key=="vis.arrow_fov_r")   settings::visuals::arrow_use_fov_radius=bv;
            else if (key=="vis.hat_style")     settings::visuals::china_hat_style=ival;
            else if (key=="vis.skeleton_style")settings::visuals::skeleton_style=ival;
            else if (key=="vis.skel_joints")   settings::visuals::skeleton_joints=bv;
            else if (key=="vis.skel_joint_size")settings::visuals::skeleton_joint_size=fval;
            else if (key=="vis.skel_segs")     settings::visuals::skeleton_vis_segments=ival;
            else if (key=="vis.skel_hidden")   parse_c4(settings::visuals::skeleton_hidden_color);
            else if (key=="vis.radar_style")   settings::visuals::radar_style=ival;
            else if (key=="vis.tracer_col")    parse_c4(settings::visuals::tracer_color);
            else if (key=="vis.china_hat")     settings::visuals::china_hat=bv;
            else if (key=="vis.hat_col")       parse_c4(settings::visuals::china_hat_color);
            else if (key=="vis.hat_h")         settings::visuals::china_hat_height=fval;
            else if (key=="vis.hat_r")         settings::visuals::china_hat_radius=fval;
            else if (key=="vis.body_corpse")   settings::visuals::body_corpse=bv;
            else if (key=="vis.corpse_col")    parse_c4(settings::visuals::corpse_color);
            else if (key=="vis.health_text")   settings::visuals::health_text=bv;
            else if (key=="vis.htext_col")     parse_c4(settings::visuals::health_text_color);
            else if (key=="vis.box_thick")     settings::visuals::box_thickness=fval;
            else if (key=="vis.dist_check")    settings::visuals::distance_check=bv;
            else if (key=="vis.dist_unit")     settings::visuals::distance_unit=ival;
            else if (key=="vis.aim_col")       parse_c4(settings::visuals::aim_target_color);
            else if (key=="vis.friend_col")    parse_c4(settings::visuals::friend_color);
            else if (key=="vis.tool")          settings::visuals::tool=bv;
            else if (key=="vis.tool_col")      parse_c4(settings::visuals::tool_color);
            else if (key=="vis.flags")         settings::visuals::flags=bv;
            else if (key=="vis.flags_col")     parse_c4(settings::visuals::flags_state_colour);
            else if (key=="vis.rigtype")       settings::visuals::rigtype=bv;
            else if (key=="vis.state")         settings::visuals::state=bv;
            else if (key=="vis.healthbar")     settings::visuals::healthbar=bv;
            else if (key=="vis.hb_col")        parse_c4(settings::visuals::healthbar_color);
            else if (key=="vis.hb_based")      settings::visuals::health_based_healthbar=bv;
            else if (key=="vis.health_pct")    settings::visuals::health_percent=bv;
            } catch(...) {}

            try {
            if      (key=="vis.health_pct_col")parse_c4(settings::visuals::health_percent_color);
            else if (key=="vis.bar_size")      settings::visuals::healthbar_size=fval;
            else if (key=="vis.skeleton")      settings::visuals::skeleton=bv;
            else if (key=="vis.skel_anim")     settings::visuals::skeleton_animate=bv;
            else if (key=="vis.skel_swing")    settings::visuals::skeleton_swing=fval;
            else if (key=="vis.skel_outline")  settings::visuals::skeleton_outline=bv;
            else if (key=="vis.skel_thick")    settings::visuals::skeleton_thickness=fval;
            else if (key=="vis.skel_col")      parse_c4(settings::visuals::skeleton_color);
            else if (key=="vis.skel_out_col")  parse_c4(settings::visuals::skeleton_outline_color);
            else if (key=="vis.chams")         settings::visuals::chams=bv;
            else if (key=="vis.eng_chams")     settings::visuals::engine_chams=bv;
            else if (key=="vis.eng_style")     settings::visuals::engine_chams_style=ival;
            else if (key=="vis.eng_col")       parse_c4(settings::visuals::engine_chams_color);
            else if (key=="vis.chams_fill")    parse_c4(settings::visuals::chams_fill_color);
            else if (key=="vis.mesh_shader")   settings::visuals::mesh_shader_mode=ival;
            else if (key=="vis.mesh_occl")     parse_c4(settings::visuals::mesh_occluded_color);
            else if (key=="vis.esp_font")      settings::visuals::esp_font=ival;
            else if (key=="vis.esp_font_size") settings::visuals::esp_font_size=fval;
            else if (key=="vis.fade_in")       settings::visuals::fade_in_speed=fval;
            else if (key=="vis.fade_out")      settings::visuals::fade_out_speed=fval;
            else if (key=="vis.view_hitbox")   settings::visuals::view_hitbox=bv;
            else if (key=="vis.vhb_col")       parse_c4(settings::visuals::view_hitbox_color);
            else if (key=="vis.localplayer")   settings::visuals::localplayer=bv;
            else if (key=="vis.hit_skeleton")  settings::visuals::hit_skeleton=bv;
            else if (key=="vis.hit_skel_col")  parse_c4(settings::visuals::hit_skeleton_color);
            else if (key=="vis.hit_skel_dur")  settings::visuals::hit_skeleton_duration=fval;
            else if (key=="vis.hit_chams")     settings::visuals::hit_chams=bv;
            else if (key=="vis.hit_chams_col") parse_c4(settings::visuals::hit_chams_color);
            else if (key=="vis.hit_chams_dur") settings::visuals::hit_chams_duration=fval;
            else if (key=="vis.hit_freeze")    settings::visuals::hit_freeze=bv;
            else if (key=="vis.movement_status") settings::visuals::movement_status=bv;
            else if (key=="vis.radar")         settings::visuals::radar_enabled=bv;
            else if (key=="vis.radar_size")    settings::visuals::radar_size=fval;
            else if (key=="vis.radar_range")   settings::visuals::radar_range=ival;
            else if (key=="vis.radar_pos")     settings::visuals::radar_position=ival;
            else if (key=="vis.radar_shape")   settings::visuals::radar_shape=ival;
            else if (key=="vis.radar_op")      settings::visuals::radar_opacity=fval;
            else if (key=="vis.radar_rot")     settings::visuals::radar_rotate=bv;
            else if (key=="vis.radar_grid")    settings::visuals::radar_grid=bv;
            else if (key=="vis.radar_labels")  settings::visuals::radar_labels=bv;
            else if (key=="vis.radar_sweep")   settings::visuals::radar_sweep=bv;
            else if (key=="vis.radar_names")   settings::visuals::radar_names=bv;
            else if (key=="vis.radar_hp")      settings::visuals::radar_health=bv;
            else if (key=="vis.radar_dead")    settings::visuals::radar_dead=bv;
            else if (key=="vis.radar_walls")   settings::visuals::radar_show_walls=bv;
            else if (key=="vis.radar_wall_col") parse_c4(settings::visuals::radar_wall_color);
            else if (key=="vis.radar_local_col")parse_c4(settings::visuals::radar_local_color);
            else if (key=="vis.radar_team_col") parse_c4(settings::visuals::radar_team_color);
            else if (key=="vis.radar_enemy_col")parse_c4(settings::visuals::radar_enemy_color);

            } catch(...) {}

            try {
            if      (key=="hbx.enabled")       settings::hitbox_expander::enabled=bv;
            else if (key=="hbx.size_x")        settings::hitbox_expander::size_x=fval;
            else if (key=="hbx.size_y")        settings::hitbox_expander::size_y=fval;
            else if (key=="hbx.size_z")        settings::hitbox_expander::size_z=fval;
            else if (key=="hbx.knock_check")   settings::hitbox_expander::knock_check=bv;

            else if (key=="tb.enabled")        settings::triggerbot::enabled=bv;
            else if (key=="tb.keybind")        settings::triggerbot::keybind=ival;
            else if (key=="tb.actmode")        settings::triggerbot::activation_mode=ival;
            else if (key=="tb.delay_ms")       settings::triggerbot::delay_ms=fval;
            else if (key=="tb.fov")            settings::triggerbot::fov=fval;
            else if (key=="tb.draw_fov")       settings::triggerbot::draw_fov=bv;
            else if (key=="tb.knock_check")    settings::triggerbot::knock_check=bv;
            else if (key=="tb.team_check")     settings::triggerbot::team_check=bv;
            else if (key=="tb.target_part")    settings::triggerbot::target_part=ival;
            else if (key=="tb.continuous")     settings::triggerbot::continuous=bv;
            else if (key=="tb.visibility")     settings::triggerbot::visibility_check=bv;
            else if (key=="tb.gun_check")      settings::triggerbot::gun_check=bv;
            else if (key=="tb.aim_check")      settings::triggerbot::aim_check=bv;
            else if (key=="tb.skip_firing")    settings::triggerbot::skip_while_firing=bv;

            else if (key=="sp360.enabled")     settings::spin360::enabled=bv;
            else if (key=="sp360.keybind")     settings::spin360::keybind=ival;
            else if (key=="sp360.actmode")     settings::spin360::activation_mode=ival;
            else if (key=="sp360.speed")       settings::spin360::speed=fval;
            } catch(...) {}

            try {
            if      (key=="vis.hb_enh")        settings::visuals::healthbar_enhanced=bv;
            else if (key=="vis.hb_width")      settings::visuals::healthbar_width=fval;
            else if (key=="vis.hb_opacity")    settings::visuals::healthbar_opacity=fval;
            else if (key=="vis.hb_glow")       settings::visuals::healthbar_glow=bv;
            else if (key=="vis.hb_style")      settings::visuals::healthbar_style=ival;
            else if (key=="vis.armorbar")      settings::visuals::armorbar=bv;
            else if (key=="vis.armorbar_col")  parse_c4(settings::visuals::armorbar_color);
            else if (key=="vis.nchams_glow")   settings::visuals::nchams_glow=bv;
            else if (key=="vis.nchams_glow_o") settings::visuals::nchams_glow_opacity=fval;
            else if (key=="vis.nchams_glow_s") settings::visuals::nchams_glow_size=fval;
            else if (key=="vis.nchams_core")   settings::visuals::nchams_inner_fill=bv;
            else if (key=="vis.nchams_core_s") settings::visuals::nchams_inner_fill_scale=fval;

            else if (key=="vis.headdot")       settings::visuals::head_dot=bv;
            else if (key=="vis.headdot_size")  settings::visuals::head_dot_size=fval;
            else if (key=="vis.headdot_col")   parse_c4(settings::visuals::head_dot_color);
            else if (key=="vis.headdot_out")   settings::visuals::head_dot_outline=bv;
            else if (key=="vis.headdot_ocol")  parse_c4(settings::visuals::head_dot_outline_color);
            else if (key=="vis.lookdir")       settings::visuals::look_direction=bv;
            else if (key=="vis.lookdir_len")   settings::visuals::look_direction_length=fval;
            else if (key=="vis.lookdir_th")    settings::visuals::look_direction_thickness=fval;
            else if (key=="vis.lookdir_col")   parse_c4(settings::visuals::look_direction_color);
            } catch(...) {}

            try {
            if      (key=="fx.hitmarker")      settings::effects::hitmarker=bv;
            else if (key=="fx.hm_size")        settings::effects::hitmarker_size=fval;
            else if (key=="fx.hm_dur")         settings::effects::hitmarker_duration=fval;
            else if (key=="fx.hm_col")         parse_c4(settings::effects::hitmarker_color);
            else if (key=="fx.hm_head_col")    parse_c4(settings::effects::hitmarker_head_color);
            else if (key=="fx.hitdata")        settings::effects::hitdata=bv;
            else if (key=="fx.hd_damage")      settings::effects::hitdata_damage=bv;
            else if (key=="fx.hd_health")      settings::effects::hitdata_health=bv;
            else if (key=="fx.hd_distance")    settings::effects::hitdata_distance=bv;
            else if (key=="fx.hd_size")        settings::effects::hitdata_size=fval;
            else if (key=="fx.hd_dur")         settings::effects::hitdata_duration=fval;
            else if (key=="fx.hitsound")       settings::effects::hitsound=bv;
            else if (key=="fx.hs_index")       settings::effects::hitsound_index=ival;
            else if (key=="fx.hs_vol")         settings::effects::hitsound_volume=fval;
            else if (key=="fx.crosshair")      settings::effects::crosshair=bv;
            else if (key=="fx.cross_lock")     settings::effects::crosshair_lock=bv;
            else if (key=="fx.cross_len")      settings::effects::crosshair_length=fval;
            else if (key=="fx.cross_gap")      settings::effects::crosshair_gap=fval;
            else if (key=="fx.cross_thick")    settings::effects::crosshair_thickness=fval;
            else if (key=="fx.cross_spin")     settings::effects::crosshair_spin=bv;
            else if (key=="fx.cross_spin_spd") settings::effects::crosshair_spin_speed=fval;
            else if (key=="fx.cross_outline")  settings::effects::crosshair_outline=bv;
            else if (key=="fx.cross_dot")      settings::effects::crosshair_dot=bv;
            else if (key=="fx.cross_dot_size") settings::effects::crosshair_dot_size=fval;
            else if (key=="fx.cross_col")      parse_c4(settings::effects::crosshair_color);
            else if (key=="fx.cross_out_col")  parse_c4(settings::effects::crosshair_outline_color);
            else if (key=="fx.cross_str")      settings::effects::crosshair_stretch=bv;
            else if (key=="fx.cross_str_amt")  settings::effects::crosshair_stretch_amount=fval;
            else if (key=="fx.cross_str_spd")  settings::effects::crosshair_stretch_speed=fval;
            else if (key=="fx.tracer")         settings::effects::tracers=bv;
            else if (key=="fx.tracer_fade")    settings::effects::tracer_fade=bv;
            else if (key=="fx.tracer_outline") settings::effects::tracer_outline=bv;
            else if (key=="fx.tracer_impact")  settings::effects::tracer_impact=bv;
            else if (key=="fx.tracer_tool")    settings::effects::tracer_from_tool=bv;
            else if (key=="fx.tracer_mode")    settings::effects::tracer_mode=ival;
            else if (key=="fx.tracer_len")     settings::effects::tracer_length=fval;
            else if (key=="fx.tracer_rainbow") settings::effects::tracer_rainbow=bv;
            else if (key=="fx.tracer_dur")     settings::effects::tracer_duration=fval;
            else if (key=="fx.tracer_thick")   settings::effects::tracer_thickness=fval;
            else if (key=="fx.tracer_rspd")    settings::effects::tracer_rainbow_speed=fval;
            else if (key=="fx.tracer_glow")    settings::effects::tracer_glow=fval;
            else if (key=="fx.tracer_imp_sz")  settings::effects::tracer_impact_size=fval;
            else if (key=="fx.tracer_col")     parse_c4(settings::effects::tracer_color);
            } catch(...) {}

            try {

            if      (key=="ab.enabled")      settings::aimbot::enabled=bv;
            else if (key=="ab.keybind")      settings::aimbot::keybind=ival;
            else if (key=="ab.actmode")      settings::aimbot::activation_mode=ival;
            else if (key=="ab.mode")         settings::aimbot::mode=ival;
            else if (key=="ab.target_part")  settings::aimbot::target_part=ival;
            else if (key=="ab.air_part")     settings::aimbot::air_part_enabled=bv;
            else if (key=="ab.air_part_id")  settings::aimbot::air_part=ival;
            else if (key=="ab.fov")          settings::aimbot::fov=fval;
            else if (key=="ab.use_fov")      settings::aimbot::use_fov=bv;
            else if (key=="ab.draw_fov")     settings::aimbot::draw_fov=bv;
            else if (key=="ab.fov_col")      parse_c4(settings::aimbot::fov_circle_colour);
            else if (key=="ab.fov_out_col")  parse_c4(settings::aimbot::fov_outline_colour);
            else if (key=="ab.smoothing")    settings::aimbot::smoothing=bv;
            else if (key=="ab.instant_lock") settings::aimbot::instant_lock=bv;
            else if (key=="ab.smooth_x")     settings::aimbot::smoothingx=fval;
            else if (key=="ab.smooth_y")     settings::aimbot::smoothingy=fval;
            else if (key=="ab.smooth_style") settings::aimbot::smoothing_style=ival;
            else if (key=="ab.priority")     settings::aimbot::priority=ival;
            else if (key=="ab.prediction")   settings::aimbot::enable_prediction=bv;
            else if (key=="ab.pred_x")       settings::aimbot::prediction_x=fval;
            else if (key=="ab.pred_y")       settings::aimbot::prediction_y=fval;
            else if (key=="ab.air_pred")     settings::aimbot::air_prediction_enabled=bv;
            else if (key=="ab.air_pred_x")   settings::aimbot::air_prediction_x=fval;
            else if (key=="ab.air_pred_y")   settings::aimbot::air_prediction_y=fval;
            else if (key=="ab.teamcheck")    settings::aimbot::teamcheck=bv;
            else if (key=="ab.knock_check")  settings::aimbot::knock_check=bv;
            else if (key=="ab.dead_check")   settings::aimbot::dead_check=bv;
            else if (key=="ab.vis_check")    settings::aimbot::visible_check=bv;
            else if (key=="ab.sticky")       settings::aimbot::sticky_aim=bv;
            else if (key=="ab.health_check") settings::aimbot::health_check_enabled=bv;
            else if (key=="ab.min_health")   settings::aimbot::min_health=fval;
            else if (key=="ab.tracer")        settings::aimbot::tracer=bv;
            else if (key=="ab.tracer_origin") settings::aimbot::tracer_origin=ival;
            else if (key=="ab.tracer_col")    parse_c4(settings::aimbot::tracer_color);
            else if (key=="ab.tr_th")         settings::aimbot::tracer_thickness=fval;
            else if (key=="ab.tr_outline")    settings::aimbot::tracer_outline=bv;
            else if (key=="ab.tr_style")      settings::aimbot::tracer_style=ival;
            else if (key=="ab.fov_style")     settings::aimbot::fov_style=ival;
            else if (key=="sa.fov_style")     settings::silentaim::fov_style=ival;
            else if (key=="tb.fov_style")     settings::triggerbot::fov_style=ival;
            else if (key=="fu.antifling")     settings::fusion::antifling=bv;
            else if (key=="fu.fling_lin")     settings::fusion::antifling_lin=fval;
            else if (key=="fu.fling_ang")     settings::fusion::antifling_ang=fval;
            else if (key=="fu.jumppower")     settings::fusion::jumppower=bv;
            else if (key=="fu.jump_key")      settings::fusion::jumppower_key=ival;
            else if (key=="fu.jump_mode")     settings::fusion::jumppower_mode=ival;
            else if (key=="fu.jump_value")    settings::fusion::jumppower_value=fval;
            else if (key=="fu.hipheight")     settings::fusion::hipheight=bv;
            else if (key=="fu.hip_value")     settings::fusion::hipheight_value=fval;
            else if (key=="fu.freecam")       settings::fusion::freecam=bv;
            else if (key=="fu.free_key")      settings::fusion::freecam_key=ival;
            else if (key=="fu.free_mode")     settings::fusion::freecam_mode=ival;
            else if (key=="fu.free_speed")    settings::fusion::freecam_speed=fval;
            else if (key=="fu.free_sprint")   settings::fusion::freecam_sprint=fval;
            else if (key=="fu.free_revert")   settings::fusion::freecam_revert=bv;
            else if (key=="fu.orbit")         settings::fusion::orbit=bv;
            else if (key=="fu.orbit_key")     settings::fusion::orbit_key=ival;
            else if (key=="fu.orbit_act")     settings::fusion::orbit_activation=ival;
            else if (key=="fu.orbit_target")  settings::fusion::orbit_target=ival;
            else if (key=="fu.orbit_radius")  settings::fusion::orbit_radius=fval;
            else if (key=="fu.orbit_speed")   settings::fusion::orbit_speed=fval;
            else if (key=="fu.orbit_height")  settings::fusion::orbit_height=fval;
            else if (key=="fu.orbit_spec")    settings::fusion::orbit_spectate=bv;
            else if (key=="fu.headless")      settings::fusion::headless=bv;
            else if (key=="fu.korblox")       settings::fusion::korblox=bv;
            else if (key=="fu.spectate")      settings::fusion::spectate=bv;
            else if (key=="fu.spec_key")      settings::fusion::spectate_cycle_key=ival;
            else if (key=="fu.flyabove")      settings::fusion::flyabove=bv;
            else if (key=="fu.flyabove_h")    settings::fusion::flyabove_height=fval;
            else if (key=="fu.hitnotes")      settings::fusion::hit_notifications=bv;
            else if (key=="fu.hitnotes_s")    settings::fusion::hit_notification_seconds=fval;
            else if (key=="fu.fps")           settings::fusion::show_fps=bv;
            else if (key=="fu.norecoil")      settings::fusion::norecoil=bv;
            else if (key=="fu.nr_key")        settings::fusion::norecoil_key=ival;
            else if (key=="fu.nr_x")          settings::fusion::norecoil_x=fval;
            else if (key=="fu.nr_y")          settings::fusion::norecoil_y=fval;
            else if (key=="fu.nr_strength")   settings::fusion::norecoil_strength=fval;
            else if (key=="ab.fov_th")        settings::aimbot::fov_thickness=fval;
            else if (key=="ab.fov_outline")   settings::aimbot::fov_outline=bv;
            else if (key=="ab.offset")       settings::aimbot::offset_enabled=bv;
            else if (key=="ab.offset_x")     settings::aimbot::offset_x=fval;
            else if (key=="ab.offset_y")     settings::aimbot::offset_y=fval;
            } catch(...) {}

            try {

            if      (key=="sa.enabled")       settings::silentaim::enabled=bv;
            else if (key=="sa.keybind")       settings::silentaim::keybind=ival;
            else if (key=="sa.actmode")       settings::silentaim::activation_mode=ival;
            else if (key=="sa.method")        settings::silentaim::method=ival;
            else if (key=="sa.target_part")   settings::silentaim::target_part=ival;
            else if (key=="sa.fov")           settings::silentaim::fov=fval;
            else if (key=="sa.use_fov")       settings::silentaim::use_fov=bv;
            else if (key=="sa.draw_fov")      settings::silentaim::draw_fov=bv;
            else if (key=="sa.prediction")    settings::silentaim::enable_prediction=bv;
            else if (key=="sa.pred_x")        settings::silentaim::prediction_x=fval;
            else if (key=="sa.pred_y")        settings::silentaim::prediction_y=fval;
            else if (key=="sa.sticky")        settings::silentaim::sticky_aim=bv;
            else if (key=="sa.use_ab_target") settings::silentaim::use_aimbot_target=bv;
            else if (key=="sa.teamcheck")     settings::silentaim::teamcheck=bv;
            else if (key=="sa.knock_check")   settings::silentaim::knock_check=bv;
            else if (key=="sa.dead_check")    settings::silentaim::dead_check=bv;
            else if (key=="sa.vis_check")     settings::silentaim::visible_check=bv;
            else if (key=="sa.health_check")  settings::silentaim::health_check_enabled=bv;
            else if (key=="sa.min_health")    settings::silentaim::min_health=fval;
            else if (key=="sa.draw_dot")      settings::silentaim::draw_target_dot=bv;
            else if (key=="sa.dot_col")       parse_c4(settings::silentaim::target_dot_color);
            else if (key=="sa.dot_size")      settings::silentaim::target_dot_size=fval;
            else if (key=="vl.aimline")       settings::visuals::aim_line=bv;
            else if (key=="vl.aimline_sty")   settings::visuals::aim_line_style=ival;
            else if (key=="vl.aimline_col")   parse_c4(settings::visuals::aim_line_color);
            else if (key=="vl.aimline_th")    settings::visuals::aim_line_thickness=fval;
            else if (key=="vl.aimline_ctr")   settings::visuals::aim_line_centre=bv;
            else if (key=="vl.aimline_sa")    settings::visuals::aim_line_silent=bv;
            else if (key=="sa.fov_col")       parse_c4(settings::silentaim::fov_circle_colour);
            else if (key=="sa.fov_out_col")   parse_c4(settings::silentaim::fov_outline_colour);
            else if (key=="sa.fov_th")        settings::silentaim::fov_thickness=fval;
            else if (key=="sa.fov_outline")   settings::silentaim::fov_outline=bv;
            else if (key=="sa.autopred")      settings::silentaim_autopred::enabled=bv;
            else if (key=="sa.autopred_str")  settings::silentaim_autopred::strength=fval;
            else if (key=="sa.mouse_spoof")   settings::mouse_spoof::enabled=bv;
            else if (key=="sa.hit_chance_en") settings::silentaim::hit_chance_enabled=bv;
            else if (key=="sa.hit_chance")    settings::silentaim::hit_chance=fval;
            else if (key=="sa.hit_chance_fr") settings::silentaim::hit_chance_fire_rate=fval;
            else if (key=="sa.curve_tracer")   settings::silentaim::draw_curve_tracer=bv;
            else if (key=="op1.enable")        settings::op1::enable=bv;
            else if (key=="op1.force")         settings::op1::force=bv;
            else if (key=="op1.aliases")       settings::op1::part_aliases=bv;
            else if (key=="op1.body")          settings::op1::animated_body=bv;
            else if (key=="op1.coll_boxes")    settings::op1::collision_boxes=bv;
            else if (key=="op1.engine_mat")    settings::op1::engine_matrix=bv;
            else if (key=="op1.multi_los")     settings::op1::multipoint_los=bv;
            else if (key=="op1.live_health")   settings::op1::live_health=bv;
            else if (key=="op1.no_smooth")     settings::op1::no_smoothing=bv;
            else if (key=="op1.sticky_ms")     settings::op1::sticky_ms=ival;
            else if (key=="sa.ct_col")         parse_c4(settings::silentaim::curve_tracer_color);
            else if (key=="sa.ct_outline")     settings::silentaim::curve_tracer_outline=bv;
            else if (key=="sa.ct_target")      settings::silentaim::curve_tracer_target=ival;
            else if (key=="sa.ct_thick")       settings::silentaim::curve_tracer_thickness=fval;
            } catch(...) {}

            try {

            if      (key=="hbx.enabled")      settings::hitbox_expander::enabled=bv;
            else if (key=="hbx.size_x")       settings::hitbox_expander::size_x=fval;
            else if (key=="hbx.size_y")       settings::hitbox_expander::size_y=fval;
            else if (key=="hbx.size_z")       settings::hitbox_expander::size_z=fval;
            else if (key=="hbx.knock_check")  settings::hitbox_expander::knock_check=bv;
            else if (key=="tb.enabled")       settings::triggerbot::enabled=bv;
            else if (key=="tb.keybind")       settings::triggerbot::keybind=ival;
            else if (key=="tb.actmode")       settings::triggerbot::activation_mode=ival;
            else if (key=="tb.delay_ms")      settings::triggerbot::delay_ms=fval;
            else if (key=="tb.fov")           settings::triggerbot::fov=fval;
            else if (key=="tb.draw_fov")      settings::triggerbot::draw_fov=bv;
            else if (key=="tb.fov_th")        settings::triggerbot::fov_thickness=fval;
            else if (key=="tb.fov_outline")   settings::triggerbot::fov_outline=bv;
            else if (key=="tb.knock_check")   settings::triggerbot::knock_check=bv;
            else if (key=="tb.team_check")    settings::triggerbot::team_check=bv;
            else if (key=="tb.target_part")   settings::triggerbot::target_part=ival;
            else if (key=="tb.continuous")    settings::triggerbot::continuous=bv;
            else if (key=="tb.visibility")    settings::triggerbot::visibility_check=bv;
            else if (key=="tb.gun_check")     settings::triggerbot::gun_check=bv;
            else if (key=="tb.aim_check")     settings::triggerbot::aim_check=bv;
            else if (key=="tb.skip_firing")   settings::triggerbot::skip_while_firing=bv;
            else if (key=="sp360.enabled")    settings::spin360::enabled=bv;
            else if (key=="sp360.keybind")    settings::spin360::keybind=ival;
            else if (key=="sp360.actmode")    settings::spin360::activation_mode=ival;
            else if (key=="sp360.speed")      settings::spin360::speed=fval;
            else if (key=="mv.speed_en")      settings::movement::speedhack::enabled=bv;
            else if (key=="mv.speed_kb")      settings::movement::speedhack::keybind=ival;
            else if (key=="mv.speed_mode")    settings::movement::speedhack::activation_mode=ival;
            else if (key=="mv.speed_type")    settings::movement::speedhack::mode=ival;
            else if (key=="mv.speed_val")     settings::movement::speedhack::speed=fval;
            else if (key=="mv.fly_en")        settings::movement::flyhack::enabled=bv;
            else if (key=="mv.fly_kb")        settings::movement::flyhack::keybind=ival;
            else if (key=="mv.fly_mode")      settings::movement::flyhack::activation_mode=ival;
            else if (key=="mv.fly_type")      settings::movement::flyhack::mode=ival;
            else if (key=="mv.fly_val")       settings::movement::flyhack::speed=fval;
            else if (key=="mv.tickrate")      settings::movement::tickrate::enabled=bv;
            else if (key=="mv.tickrate_val")  settings::movement::tickrate::value=fval;
            else if (key=="rage.spin")        settings::spinbot::enabled=bv;
            else if (key=="rage.rapidfire")   settings::rage::rapidfire=bv;
            else if (key=="misc.noclip")      settings::noclip::enabled=bv;
            else if (key=="misc.noclip_kb")   settings::noclip::keybind=ival;
            else if (key=="misc.noclip_mode") settings::noclip::activation_mode=ival;
            else if (key=="misc.nojumpcd")    settings::nojumpcooldown::enabled=bv;
            } catch(...) {}

            try {

            if      (key=="world.time_on")    settings::world::lighting::time_on=bv;
            else if (key=="world.bright_on")  settings::world::lighting::brightness_on=bv;
            else if (key=="world.exp_on")     settings::world::lighting::exposure_on=bv;
            else if (key=="world.amb_on")     settings::world::lighting::ambient_on=bv;
            else if (key=="world.out_on")     settings::world::lighting::outdoor_on=bv;
            else if (key=="world.fog_on")     settings::world::lighting::fog_on=bv;
            else if (key=="world.shift_on")   settings::world::lighting::shift_on=bv;
            else if (key=="world.env_on")     settings::world::lighting::env_on=bv;
            else if (key=="world.no_shadow")  settings::world::lighting::no_shadow=bv;
            else if (key=="world.brightness")  settings::world::lighting::brightness=fval;
            else if (key=="world.exposure")    settings::world::lighting::exposure_compensation=fval;
            else if (key=="world.clock")       settings::world::lighting::clock_time=fval;
            else if (key=="world.fog_start")   settings::world::lighting::fog_start=fval;
            else if (key=="world.fog_end")     settings::world::lighting::fog_end=fval;
            else if (key=="world.ambient")     parse_c3(settings::world::lighting::ambient);
            else if (key=="world.outdoor_amb") parse_c3(settings::world::lighting::outdoor_ambient);
            else if (key=="world.fog_col")     parse_c3(settings::world::lighting::fog_color);
            else if (key=="world.shift_top")   parse_c3(settings::world::lighting::color_shift_top);
            else if (key=="world.shift_bot")   parse_c3(settings::world::lighting::color_shift_bottom);
            else if (key=="world.diffuse")     settings::world::lighting::env_diffuse_scale=fval;
            else if (key=="world.specular")    settings::world::lighting::env_specular_scale=fval;
            else if (key=="world.atm_en")      settings::world::atmosphere::enabled=bv;
            else if (key=="world.atm_col")     parse_c3(settings::world::atmosphere::color);
            else if (key=="world.atm_decay")   parse_c3(settings::world::atmosphere::decay);
            else if (key=="world.density")     settings::world::atmosphere::density=fval;
            else if (key=="world.offset")      settings::world::atmosphere::offset=fval;
            else if (key=="world.haze")        settings::world::atmosphere::haze=fval;
            else if (key=="world.glare")       settings::world::atmosphere::glare=fval;

            else if (key=="pfx.bloom")         settings::world::postfx::bloom::enabled=bv;
            else if (key=="pfx.bloom_int")     settings::world::postfx::bloom::intensity=fval;
            else if (key=="pfx.bloom_sz")      settings::world::postfx::bloom::size=fval;
            else if (key=="pfx.bloom_thr")     settings::world::postfx::bloom::threshold=fval;
            else if (key=="pfx.cc")            settings::world::postfx::colorcorrect::enabled=bv;
            else if (key=="pfx.cc_tint")       parse_c3(settings::world::postfx::colorcorrect::tint);
            else if (key=="pfx.cc_br")         settings::world::postfx::colorcorrect::brightness=fval;
            else if (key=="pfx.cc_con")        settings::world::postfx::colorcorrect::contrast=fval;
            else if (key=="pfx.dof")           settings::world::postfx::dof::enabled=bv;
            else if (key=="pfx.dof_fd")        settings::world::postfx::dof::focus_distance=fval;
            else if (key=="pfx.dof_fr")        settings::world::postfx::dof::in_focus_radius=fval;
            else if (key=="pfx.dof_fi")        settings::world::postfx::dof::far_intensity=fval;
            else if (key=="pfx.dof_ni")        settings::world::postfx::dof::near_intensity=fval;
            else if (key=="pfx.sr")            settings::world::postfx::sunrays::enabled=bv;
            else if (key=="pfx.sr_int")        settings::world::postfx::sunrays::intensity=fval;
            else if (key=="pfx.sr_spd")        settings::world::postfx::sunrays::spread=fval;

            else if (key=="shd.vig")           settings::world::shaders::vignette=bv;
            else if (key=="shd.vig_str")       settings::world::shaders::vignette_strength=fval;
            else if (key=="shd.vig_rad")       settings::world::shaders::vignette_radius=fval;
            else if (key=="shd.chrab")         settings::world::shaders::chromatic_aberration=bv;
            else if (key=="shd.chr_str")       settings::world::shaders::chromatic_strength=fval;
            else if (key=="shd.grain")         settings::world::shaders::film_grain=bv;
            else if (key=="shd.grain_str")     settings::world::shaders::film_grain_strength=fval;
            else if (key=="shd.glossy")        settings::world::shaders::glossy=bv;
            else if (key=="shd.glos_spc")      settings::world::shaders::glossy_specular=fval;
            else if (key=="shd.glos_dif")      settings::world::shaders::glossy_diffuse=fval;
            else if (key=="shd.glos_shim")     settings::world::shaders::glossy_shimmer=bv;
            else if (key=="shd.contrast")      settings::world::shaders::contrast=bv;
            else if (key=="shd.con_val")       settings::world::shaders::contrast_value=fval;
            else if (key=="shd.bri_val")       settings::world::shaders::brightness_value=fval;
            else if (key=="preset.night")      settings::world::presets::night=bv;
            else if (key=="preset.sunset")     settings::world::presets::sunset=bv;
            else if (key=="preset.rainbow")    settings::world::presets::rainbow=bv;
            else if (key=="preset.fogpulse")   settings::world::presets::fog_pulse=bv;

            else if (key=="world.snow")        settings::world::effects_3d::snow_mode = bv ? 1 : 0;
            else if (key=="world.snow_mode")   settings::world::effects_3d::snow_mode=ival;
            else if (key=="world.snow_int")    settings::world::effects_3d::snow_intensity=fval;
            else if (key=="world.snow_spd")    settings::world::effects_3d::snow_speed=fval;
            else if (key=="world.snow_col")    parse_c3(settings::world::effects_3d::snow_color);
            else if (key=="world.rain")        settings::world::effects_3d::rain_mode = bv ? 1 : 0;
            else if (key=="world.rain_mode")   settings::world::effects_3d::rain_mode=ival;
            else if (key=="world.storm")       settings::world::effects_3d::storm_mode = bv ? 1 : 0;
            else if (key=="world.storm_mode")  settings::world::effects_3d::storm_mode=ival;
            else if (key=="world.rain_int")    settings::world::effects_3d::rain_intensity=fval;
            else if (key=="world.rain_spd")    settings::world::effects_3d::rain_speed=fval;
            else if (key=="world.rain_col")    parse_c3(settings::world::effects_3d::rain_color);
            else if (key=="world.wind_x")      settings::world::effects_3d::wind_x=fval;
            else if (key=="world.wind_z")      settings::world::effects_3d::wind_z=fval;
            else if (key=="world.ripples")     settings::world::effects_3d::ripples=bv;
            else if (key=="world.ripple_sz")   settings::world::effects_3d::ripple_size=fval;
            else if (key=="world.rain_snd")    settings::world::effects_3d::rain_sound=bv;
            else if (key=="world.rain_vol")    settings::world::effects_3d::rain_volume=fval;
            } catch(...) {}

            try {
            if      (key=="ui.theme")          { settings::theme::index=ival; apply_theme(settings::theme::index); aether::glass::apply(ImGui::GetStyle()); }
            else if (key=="ui.menu_key")       sync::state().menu_key=ival;
            else if (key=="ui.dim")            sync::state().dim_background=bv;
            else if (key=="ui.glass")          sync::state().glass_transparency=fval;
            else if (key=="ui.wm_fps")         sync::state().wm_fps=bv;
            else if (key=="ui.wm_time")        sync::state().wm_time=bv;
            else if (key=="ui.wm_show")        sync::state().wm_show=bv;
            else if (key=="ui.wm_hide")       settings::menu::hide_watermark=bv;
            else if (key=="ui.ignore_place")  settings::menu::ignore_unsupported_place=bv;
            else if (key=="ui.keybind_list")   sync::state().kb_show=bv;
            else if (key=="ui.kb_mode")        sync::state().kb_mode=bv;
            else if (key=="ui.kb_active")      sync::state().kb_only_active=bv;
            else if (key=="ui.kb_x")           sync::state().kb_x=fval;
            else if (key=="ui.kb_y")           sync::state().kb_y=fval;
            else if (key=="ui.th_show")        sync::state().th_show=bv;
            else if (key=="ui.th_avatar")      sync::state().th_avatar=bv;
            else if (key=="ui.th_name")        sync::state().th_name=bv;
            else if (key=="ui.th_health")      sync::state().th_health=bv;
            else if (key=="ui.th_tool")        sync::state().th_tool=bv;
            else if (key=="ui.th_x")           sync::state().th_x=fval;
            else if (key=="ui.th_y")           sync::state().th_y=fval;
            else if (key=="ui.th_scale")       sync::state().th_scale=fval;

            else if (key=="ui.custom_accent")  settings::appearance::custom_accent=bv;
            else if (key=="ui.accent_spread")  settings::appearance::accent_spread=fval;
            else if (key=="ui.accent")         parse_c4(settings::appearance::accent);
            else if (key=="ui.win_gradient")   settings::appearance::window_gradient=bv;
            else if (key=="ui.win_shadow")     settings::appearance::window_shadow=bv;
            else if (key=="ui.accent_glow")    settings::appearance::accent_glow=bv;
            else if (key=="ui.glow")           settings::appearance::glow_strength=fval;
            else if (key=="ui.rounding")       settings::appearance::rounding=fval;
            else if (key=="ui.panel_alpha")    settings::appearance::panel_alpha=fval;
            else if (key=="ui.dock_alpha")     settings::appearance::dock_alpha=fval;
            else if (key=="ui.frost")          settings::appearance::glass_sheen=fval;
            else if (key=="ui.opaque")         settings::appearance::no_transparency=bv;
            else if (key=="ui.panel_contrast")  settings::appearance::panel_contrast=fval;
            else if (key=="ui.aa")             settings::appearance::antialiasing=bv;
            else if (key=="ui.aa_q")           settings::appearance::aa_quality=fval;
            else if (key=="bd.on")             settings::backdrop::enabled=bv;
            else if (key=="bd.blur75")         settings::backdrop::blur=fval;
            else if (key=="bd.tint")           settings::backdrop::tint=fval;
            else if (key=="bd.tintonly")       settings::backdrop::tint_only=bv;
            else if (key=="bd.pad")            settings::backdrop::capture_pad=fval;
            else if (key=="bd.cap")            settings::backdrop::capture_cap=fval;
            else if (key=="bd.status")         settings::backdrop::status_line=bv;
            else if (key=="ui.nav_labels")    settings::appearance::nav_labels=bv;
            else if (key=="ui.switches")       settings::appearance::switches=bv;
            else if (key=="ui.set_tab")        settings::appearance::settings_tab=ival;
            else if (key=="ui.smooth_scroll")  settings::appearance::smooth_scroll=bv;
            else if (key=="ui.scroll_speed")   settings::appearance::scroll_speed=fval;
            else if (key=="ui.page_anim")      settings::appearance::page_animation=bv;
            else if (key=="ui.widget_anim")    settings::appearance::widget_animations=bv;

            else if (key=="ui.particles")      settings::appearance::bg_enabled=bv;
            else if (key=="ui.bg_enabled")     settings::appearance::bg_enabled=bv;
            else if (key=="ui.bg_effect")      settings::appearance::bg_effect=ival;
            else if (key=="ui.bg_density")     settings::appearance::bg_density=fval;
            else if (key=="ui.bg_speed")       settings::appearance::bg_speed=fval;
            else if (key=="ui.bg_size")        settings::appearance::bg_size=fval;
            else if (key=="ui.bg_opacity")     settings::appearance::bg_opacity=fval;
            else if (key=="ui.bg_color")       parse_c4(settings::appearance::bg_color);
            else if (key=="ui.bg_wind")        settings::appearance::bg_wind=fval;
            else if (key=="ui.bg_glow")        settings::appearance::bg_glow=fval;
            else if (key=="ui.bg_parallax")    settings::appearance::bg_parallax=bv;
            else if (key=="ui.bg_menu_only")   settings::appearance::bg_menu_only=bv;
            else if (key=="ui.bg_dir")         settings::appearance::bg_dir=ival;
            else if (key=="ui.bg_angle")       settings::appearance::bg_angle=fval;
            else if (key=="ui.bg_streak")      settings::appearance::bg_streak=fval;
            else if (key=="ui.bg_spread")      settings::appearance::bg_spread=fval;
            else if (key=="ui.bg_width")       settings::appearance::bg_width=fval;
            else if (key=="ui.enhance")        settings::appearance::enhance=bv;
            else if (key=="ui.enhance_size")   settings::appearance::enhance_size=fval;
            else if (key=="ui.panel_width")    settings::appearance::panel_width=fval;
            else if (key=="ui.test_enhance")   settings::appearance::test_enhance=bv;


            else if (key=="sky.enabled")       settings::skybox::enabled=bv;
            else if (key=="sky.shader")        settings::skybox::shader=ival;
            else if (key=="sky.video")       { settings::skybox::video=ival; settings::skybox::video_file[0]='\0'; }
            } catch(...) {}

            try {
            if      (key=="vis.nchams")        settings::visuals::nchams=bv;
            else if (key=="vis.nchams_mode")   settings::visuals::nchams_mode=ival;
            else if (key=="vis.nchams_out")    parse_c4(settings::visuals::nchams_outline_color);
            else if (key=="vis.nchams_fcol")   parse_c4(settings::visuals::nchams_fill_color);
            else if (key=="vis.nchams_style")  settings::visuals::nchams_shader_style=ival;
            else if (key=="vis.nchams_pulse")  settings::visuals::nchams_pulse=bv;
            else if (key=="vis.nchams_pulse_s")settings::visuals::nchams_pulse_speed=fval;
            else if (key=="vis.nchams_box")  { settings::visuals::nchams=bv; settings::visuals::nchams_mode=0; }
            else if (key=="vis.nchams_box_col")   parse_c4(settings::visuals::nchams_outline_color);
            else if (key=="vis.nchams_fill") { settings::visuals::nchams=bv; settings::visuals::nchams_mode=1; }
            else if (key=="vis.nchams_fill_col")  parse_c4(settings::visuals::nchams_fill_color);
            else if (key=="vis.nchams_clip") { settings::visuals::nchams=bv; settings::visuals::nchams_mode=2; }
            else if (key=="vis.nchams_clip_col")  parse_c4(settings::visuals::nchams_outline_color);
            } catch(...) {}

            try {
            if      (key=="world.light")       settings::world::lighting::light_on=bv;
            else if (key=="world.light_on")    settings::world::lighting::light_on=bv;
            else if (key=="world.light_col")   parse_c3(settings::world::lighting::light_color);
            else if (key=="world.light_dir")   parse_c3(settings::world::lighting::light_dir);
            else if (key=="world.sky")         settings::world::sky_slots::enabled=bv;
            else if (key=="world.sky_sun")     settings::world::sky_slots::sun_angular=fval;
            else if (key=="world.sky_moon")    settings::world::sky_slots::moon_angular=fval;
            else if (key=="world.sky_orient")  parse_c3(settings::world::sky_slots::orientation);
            else if (key=="world.terrain")     settings::world::terrain::enabled=bv;
            else if (key=="world.grass_len")   settings::world::terrain::grass_length=fval;
            else if (key=="world.grass_col")   parse_c3(settings::world::terrain::grass_color);
            else if (key=="world.water_col")   parse_c3(settings::world::terrain::water_color);
            else if (key=="world.water_refl")  settings::world::terrain::water_reflectance=fval;
            else if (key=="world.water_trans") settings::world::terrain::water_transparency=fval;
            else if (key=="world.colorgrade")  settings::world::postfx::colorgrade::enabled=bv;
            else if (key=="world.tonemapper")  settings::world::postfx::colorgrade::tonemapper=ival;
            } catch(...) {}

            try {
            if      (key=="hm.greet")   settings::home::greeting=bv;
            else if (key=="hm.clock")   settings::home::clock=bv;
            else if (key=="hm.clockf")  settings::home::clock_format=ival;
            else if (key=="hm.avatar")  settings::home::avatar=bv;
            else if (key=="hm.avring")  settings::home::avatar_ring=bv;
            else if (key=="hm.avglow")  settings::home::avatar_glow=bv;
            else if (key=="hm.cards")   settings::home::cards=bv;
            else if (key=="hm.count")   settings::home::count_up=bv;
            else if (key=="hm.stagger") settings::home::stagger=fval;
            else if (key=="hm.act")     settings::home::activity=bv;
            else if (key=="hm.act_fill")settings::home::activity_fill=bv;
            else if (key=="hm.act_sm")  settings::home::activity_smooth=bv;
            else if (key=="hm.act_grid")settings::home::activity_grid=bv;
            else if (key=="hm.act_hov") settings::home::activity_hover=bv;
            else if (key=="hm.near")    settings::home::nearby=bv;
            else if (key=="hm.near_rows")settings::home::nearby_rows=ival;
            else if (key=="hm.near_faces")settings::home::nearby_faces=bv;
            else if (key=="hm.near_bars")settings::home::nearby_bars=bv;
            else if (key=="dk.on")      settings::docks::enabled=bv;
            else if (key=="dk.opacity") settings::docks::opacity=fval;
            else if (key=="dk.scale")   settings::docks::scale=fval;
            else if (key=="dk.shadow")  settings::docks::shadow=bv;
            else if (key=="dk.snap")    settings::docks::snap=bv;
            else if (key=="dk.snap_px") settings::docks::snap_px=fval;
            else if (key=="dk.snap_gap")settings::docks::snap_gap=fval;
            else if (key=="dk.lift")    settings::docks::hover_lift=bv;
            else if (key=="dk.fold")    settings::docks::fold_on_click=bv;
            else if (key=="dk.anim")    settings::docks::animate=bv;
            else if (key=="dk.anim_s")  settings::docks::anim_speed=fval;
            else if (key=="dk.titles")  settings::docks::show_titles=bv;
            else if (key=="dk.grip")    settings::docks::show_grip=bv;
            else if (key=="viz.on")     settings::viz::enabled=bv;
            else if (key=="viz.mode")   settings::viz::mode=ival;
            else if (key=="viz.window") settings::viz::window=ival;
            else if (key=="viz.grid")   settings::viz::grid=bv;
            else if (key=="viz.glow")   settings::viz::glow=bv;
            else if (key=="viz.thick")  settings::viz::thickness=fval;
            else if (key=="viz.band")   settings::viz::band=bv;
            else if (key=="viz.readouts")settings::viz::readouts=bv;
            else if (key=="viz.hover")  settings::viz::hover_value=bv;
            else if (key=="viz.demo")   settings::viz::demo_switch=bv;
            else if (key=="viz.demo_s") settings::viz::demo_speed=fval;
            else if (key=="viz.demo_x") settings::viz::demo_x=fval;
            else if (key=="viz.demo_y") settings::viz::demo_y=fval;
            else if (key=="viz.pin")    settings::viz::pin_style=ival;
            else if (key=="viz.col_sm") parse_c4(settings::viz::smooth_color);
            else if (key=="viz.col_raw")parse_c4(settings::viz::raw_color);
            else if (key=="viz.col_err")parse_c4(settings::viz::error_color);
            else
            {

                const std::size_t dot = key.rfind('.');
                if (dot != std::string::npos && key.compare(0, 3, "dk.") == 0)
                {
                    const std::string rest = key.substr(3);
                    const std::size_t sep = rest.find_last_of('.');
                    const std::string field = (sep == std::string::npos) ? rest : rest.substr(0, sep);
                    const int idx = (sep == std::string::npos) ? -1 : std::stoi(rest.substr(sep + 1));
                    if (idx >= 0 && idx < settings::docks::kCount)
                    {
                        if      (field == "open") settings::docks::open[idx] = bv;
                        else if (field == "fold") settings::docks::folded[idx] = bv;
                        else if (field == "x")    settings::docks::x[idx] = fval;
                        else if (field == "y")    settings::docks::y[idx] = fval;
                    }
                }
            }
            } catch(...) {}

            // visuals
            try {
            if      (key=="vis.trail") settings::visuals::trail=bv;
            else if (key=="vis.trail_style") settings::visuals::trail_style=ival;
            else if (key=="vis.trail_color_mode") settings::visuals::trail_color_mode=ival;
            else if (key=="vis.trail_color") parse_c4(settings::visuals::trail_color);
            else if (key=="vis.trail_head_color") parse_c4(settings::visuals::trail_head_color);
            else if (key=="vis.trail_thickness") settings::visuals::trail_thickness=fval;
            else if (key=="vis.trail_duration") settings::visuals::trail_duration=fval;
            else if (key=="vis.trail_max_points") settings::visuals::trail_max_points=ival;
            else if (key=="vis.trail_update_interval") settings::visuals::trail_update_interval=fval;
            else if (key=="vis.trail_min_movement") settings::visuals::trail_min_movement=fval;
            else if (key=="vis.trail_only_moving") settings::visuals::trail_only_moving=bv;
            else if (key=="vis.trail_follow") settings::visuals::trail_follow=ival;
            else if (key=="vis.trail_taper") settings::visuals::trail_taper=bv;
            else if (key=="vis.trail_glow") settings::visuals::trail_glow=bv;
            else if (key=="vis.trail_glow_intensity") settings::visuals::trail_glow_intensity=fval;
            else if (key=="vis.trail_glow_layers") settings::visuals::trail_glow_layers=ival;
            else if (key=="vis.trail_fade") settings::visuals::trail_fade=bv;
            else if (key=="vis.trail_fade_start") settings::visuals::trail_fade_start=fval;
            else if (key=="vis.trail_smooth") settings::visuals::trail_smooth=bv;
            else if (key=="vis.trail_spline_segments") settings::visuals::trail_spline_segments=ival;
            else if (key=="vis.trail_head_spark") settings::visuals::trail_head_spark=bv;
            else if (key=="vis.trail_height_fall") settings::visuals::trail_height_fall=bv;
            else if (key=="vis.trail_height_drop") settings::visuals::trail_height_drop=fval;
            else if (key=="vis.trail_team_check") settings::visuals::trail_team_check=bv;
            else if (key=="vis.name_sheen") settings::visuals::name_sheen=bv;
            else if (key=="vis.name_sheen_speed") settings::visuals::name_sheen_speed=fval;
            else if (key=="vis.name_sheen_width") settings::visuals::name_sheen_width=fval;
            else if (key=="vis.name_sheen_strength") settings::visuals::name_sheen_strength=fval;
            else if (key=="vis.esp_sheen_all") settings::visuals::esp_sheen_all=bv;
            else if (key=="vis.healthbar_padding") settings::visuals::healthbar_padding=fval;
            } catch(...) {}

            // effects
            try {
            if      (key=="fx.hitmarker_style") settings::effects::hitmarker_style=ival;
            if      (key=="fx.tracer_style")     settings::effects::tracer_style=ival;
            else if (key=="fx.hitmarker_shockwave") settings::effects::hitmarker_shockwave=bv;
            else if (key=="fx.hitmarker_shockwave_size") settings::effects::hitmarker_shockwave_size=fval;
            else if (key=="fx.hitmarker_sparks") settings::effects::hitmarker_sparks=bv;
            else if (key=="fx.hitmarker_spark_count") settings::effects::hitmarker_spark_count=ival;
            else if (key=="fx.hitmarker_spark_speed") settings::effects::hitmarker_spark_speed=fval;
            else if (key=="fx.hitmarker_flash") settings::effects::hitmarker_flash=bv;
            else if (key=="fx.hitmarker_streak") settings::effects::hitmarker_streak=bv;
            else if (key=="fx.hitmarker_streak_window") settings::effects::hitmarker_streak_window=fval;
            else if (key=="fx.hitmarker_crit_pulse") settings::effects::hitmarker_crit_pulse=bv;
            else if (key=="fx.hitmarker_screen_flash") settings::effects::hitmarker_screen_flash=fval;
            else if (key=="fx.hitmarker_outline") settings::effects::hitmarker_outline=bv;
            else if (key=="fx.hitmarker_glow") settings::effects::hitmarker_glow=fval;
            else if (key=="fx.hitmarker_pop") settings::effects::hitmarker_pop=fval;
            else if (key=="fx.crosshair_len_x") settings::effects::crosshair_len_x=fval;
            else if (key=="fx.crosshair_len_y") settings::effects::crosshair_len_y=fval;
            else if (key=="fx.crosshair_taper") settings::effects::crosshair_taper=fval;
            else if (key=="fx.crosshair_spin_windup") settings::effects::crosshair_spin_windup=bv;
            else if (key=="fx.crosshair_style") settings::effects::crosshair_style=ival;
            else if (key=="fx.crosshair_glow") settings::effects::crosshair_glow=fval;
            else if (key=="fx.crosshair_rainbow") settings::effects::crosshair_rainbow=bv;
            else if (key=="fx.crosshair_rainbow_speed") settings::effects::crosshair_rainbow_speed=fval;
            else if (key=="fx.crosshair_hit_pulse") settings::effects::crosshair_hit_pulse=bv;
            else if (key=="fx.crosshair_hit_pulse_size") settings::effects::crosshair_hit_pulse_size=fval;
            else if (key=="fx.crosshair_dynamic") settings::effects::crosshair_dynamic=bv;
            else if (key=="fx.crosshair_dynamic_amount") settings::effects::crosshair_dynamic_amount=fval;
            } catch(...) {}

            // sonar
            try {
            if      (key=="son.enabled") settings::sonar::enabled=bv;
            else if (key=="son.keybind") settings::sonar::keybind=ival;
            else if (key=="son.activation_mode") settings::sonar::activation_mode=ival;
            else if (key=="son.style") settings::sonar::style=ival;
            else if (key=="son.range") settings::sonar::range=fval;
            else if (key=="son.sweep_speed") settings::sonar::sweep_speed=fval;
            else if (key=="son.sweep_back") settings::sonar::sweep_back=bv;
            else if (key=="son.beam_width") settings::sonar::beam_width=fval;
            else if (key=="son.color") parse_c4(settings::sonar::color);
            else if (key=="son.ping_color") parse_c4(settings::sonar::ping_color);
            else if (key=="son.glow") settings::sonar::glow=bv;
            else if (key=="son.glow_strength") settings::sonar::glow_strength=fval;
            else if (key=="son.ping_rings") settings::sonar::ping_rings=bv;
            else if (key=="son.ping_life") settings::sonar::ping_life=fval;
            else if (key=="son.ping_radius") settings::sonar::ping_radius=fval;
            else if (key=="son.info_mode") settings::sonar::info_mode=ival;
            else if (key=="son.info_size") settings::sonar::info_size=fval;
            else if (key=="son.latch") settings::sonar::latch=bv;
            else if (key=="son.hold_time") settings::sonar::hold_time=fval;
            else if (key=="son.ping_sound") settings::sonar::ping_sound=bv;
            else if (key=="son.ping_volume") settings::sonar::ping_volume=fval;
            else if (key=="son.beam_reach") settings::sonar::beam_reach=fval;
            else if (key=="son.beam_taper") settings::sonar::beam_taper=fval;
            else if (key=="son.tail_length") settings::sonar::tail_length=fval;
            else if (key=="son.tail_strength") settings::sonar::tail_strength=fval;
            else if (key=="son.grid") settings::sonar::grid=bv;
            else if (key=="son.grid_rings") settings::sonar::grid_rings=ival;
            else if (key=="son.grid_alpha") settings::sonar::grid_alpha=fval;
            else if (key=="son.grid_labels") settings::sonar::grid_labels=bv;
            else if (key=="son.ping_style") settings::sonar::ping_style=ival;
            else if (key=="son.ping_ticks") settings::sonar::ping_ticks=bv;
            else if (key=="son.ping_spark") settings::sonar::ping_spark=bv;
            else if (key=="son.ping_drop") settings::sonar::ping_drop=bv;
            else if (key=="son.readout_plate") settings::sonar::readout_plate=bv;
            else if (key=="son.los_only") settings::sonar::los_only=bv;
            else if (key=="son.color_mode") settings::sonar::color_mode=ival;
            else if (key=="son.hue_speed") settings::sonar::hue_speed=fval;
            else if (key=="son.hide_with_menu") settings::sonar::hide_with_menu=bv;
            } catch(...) {}

            // misc_ui
            try {
            if      (key=="uifx.enabled") settings::misc_ui::enabled=bv;
            else if (key=="uifx.intensity") settings::misc_ui::intensity=fval;
            else if (key=="uifx.cursor_aura") settings::misc_ui::cursor_aura=bv;
            else if (key=="uifx.cursor_aura_size") settings::misc_ui::cursor_aura_size=fval;
            else if (key=="uifx.bloom_soft") settings::misc_ui::bloom_soft=bv;
            else if (key=="uifx.cursor_trail") settings::misc_ui::cursor_trail=bv;
            else if (key=="uifx.cursor_trail_length") settings::misc_ui::cursor_trail_length=fval;
            else if (key=="uifx.click_shockwave") settings::misc_ui::click_shockwave=bv;
            else if (key=="uifx.click_style") settings::misc_ui::click_style=ival;
            else if (key=="uifx.click_spark") settings::misc_ui::click_spark=bv;
            else if (key=="uifx.hover_sheen") settings::misc_ui::hover_sheen=bv;
            else if (key=="uifx.hover_sheen_speed") settings::misc_ui::hover_sheen_speed=fval;
            else if (key=="uifx.hover_sheen_strength") settings::misc_ui::hover_sheen_strength=fval;
            else if (key=="uifx.sheen_soft") settings::misc_ui::sheen_soft=bv;
            else if (key=="uifx.tint") settings::misc_ui::tint=ival;
            else if (key=="uifx.hover_tint") settings::misc_ui::hover_tint=bv;
            else if (key=="uifx.hover_lift") settings::misc_ui::hover_lift=bv;
            else if (key=="uifx.press_squish") settings::misc_ui::press_squish=bv;
            else if (key=="uifx.focus_ring") settings::misc_ui::focus_ring=bv;
            else if (key=="uifx.chip_pop") settings::misc_ui::chip_pop=bv;
            else if (key=="uifx.toggle_overshoot") settings::misc_ui::toggle_overshoot=bv;
            else if (key=="uifx.panel_rim") settings::misc_ui::panel_rim=bv;
            else if (key=="uifx.panel_rim_size") settings::misc_ui::panel_rim_size=fval;
            else if (key=="uifx.nav_breath") settings::misc_ui::nav_breath=bv;
            else if (key=="uifx.indicator_glow") settings::misc_ui::indicator_glow=bv;
            else if (key=="uifx.stagger") settings::misc_ui::stagger=bv;
            else if (key=="uifx.stagger_step") settings::misc_ui::stagger_step=fval;
            else if (key=="uifx.cascade_mode") settings::misc_ui::cascade_mode=ival;
            else if (key=="uifx.cascade_mode_view") settings::misc_ui::cascade_mode_view=ival;
            else if (key=="uifx.cascade_pages") settings::misc_ui::cascade_pages=bv;
            else if (key=="uifx.cascade_views") settings::misc_ui::cascade_views=bv;
            else if (key=="uifx.cascade_hairline") settings::misc_ui::cascade_hairline=bv;
            else if (key=="uifx.cascade_time") settings::misc_ui::cascade_time=fval;
            else if (key=="uifx.cascade_rise") settings::misc_ui::cascade_rise=fval;
            else if (key=="uifx.cascade_drift") settings::misc_ui::cascade_drift=fval;
            else if (key=="uifx.slider_hd") settings::misc_ui::slider_hd=bv;
            else if (key=="uifx.slider_hd_size") settings::misc_ui::slider_hd_size=fval;
            else if (key=="uifx.tooltip_fade") settings::misc_ui::tooltip_fade=bv;
            else if (key=="uifx.edge_fade") settings::misc_ui::edge_fade=bv;
            else if (key=="uifx.menu_font") settings::misc_ui::menu_font=ival;
            else if (key=="uifx.first_pop") settings::misc_ui::first_pop=bv;
            else if (key=="uifx.open_flash") settings::misc_ui::open_flash=bv;
            else if (key=="uifx.open_flash_strength") settings::misc_ui::open_flash_strength=fval;
            else if (key=="uifx.click_sounds") settings::misc_ui::click_sounds=bv;
            else if (key=="uifx.click_volume") settings::misc_ui::click_volume=fval;
            } catch(...) {}

            // ui21
            try {
            if      (key=="ui21.enabled") settings::ui21::enabled=bv;
            else if (key=="ui21.glass_layers") settings::ui21::glass_layers=bv;
            else if (key=="ui21.glass_depth") settings::ui21::glass_depth=fval;
            else if (key=="ui21.spotlight_border") settings::ui21::spotlight_border=bv;
            else if (key=="ui21.spotlight_size") settings::ui21::spotlight_size=fval;
            else if (key=="ui21.gradient_heading") settings::ui21::gradient_heading=bv;
            else if (key=="ui21.odometer") settings::ui21::odometer=bv;
            else if (key=="ui21.dot_grid") settings::ui21::dot_grid=bv;
            else if (key=="ui21.dot_grid_spacing") settings::ui21::dot_grid_spacing=fval;
            else if (key=="ui21.accent_2") parse_c4(settings::ui21::accent_2);
            else if (key=="ui21.noise") settings::ui21::noise=bv;
            else if (key=="ui21.noise_amount") settings::ui21::noise_amount=fval;
            else if (key=="ui21.light_rays") settings::ui21::light_rays=bv;
            else if (key=="ui21.light_strength") settings::ui21::light_rays_strength=fval;
            else if (key=="ui21.light_spread") settings::ui21::light_rays_spread=fval;
            else if (key=="ui21.aurora") settings::ui21::aurora=bv;
            else if (key=="ui21.aurora_strength") settings::ui21::aurora_strength=fval;
            else if (key=="ui21.panel_glow") settings::ui21::panel_glow=bv;
            else if (key=="ui21.card_glow") settings::ui21::card_glow=bv;
            else if (key=="ui21.card_glow_strength") settings::ui21::card_glow_strength=fval;
            else if (key=="ui21.panel_shadow") settings::ui21::panel_shadow=bv;
            else if (key=="ui21.panel_shadow_strength") settings::ui21::panel_shadow_strength=fval;
            else if (key=="ui21.top_sheen") settings::ui21::top_sheen=bv;
            else if (key=="ui21.top_sheen_speed") settings::ui21::top_sheen_speed=fval;
            } catch(...) {}

            // home
            try {
            if      (key=="home.card_icons") settings::home::card_icons=bv;
            } catch(...) {}

            // docks
            try {
            if      (key=="dk.behind_menu") settings::docks::behind_menu=bv;
            } catch(...) {}

            // viz
            try {
            if      (key=="viz.scale") settings::viz::scale=fval;
            } catch(...) {}

            // aimbot
            try {
            if      (key=="ab.fov_fill") settings::aimbot::fov_fill=bv;
            else if (key=="ab.fov_fill_colour") parse_c4(settings::aimbot::fov_fill_colour);
            else if (key=="ab.fov_fill_opacity") settings::aimbot::fov_fill_opacity=fval;
            } catch(...) {}

            // silentaim
            try {
            if      (key=="sa.fov_fill") settings::silentaim::fov_fill=bv;
            else if (key=="sa.fov_fill_colour") parse_c4(settings::silentaim::fov_fill_colour);
            else if (key=="sa.fov_fill_opacity") settings::silentaim::fov_fill_opacity=fval;
            else if (key=="sa.spoof_mouse") settings::silentaim::spoof_mouse=bv;
            else if (key=="sa.guncheck") settings::silentaim::guncheck=bv;
            else if (key=="sa.priorities") settings::silentaim::priorities=ival;
            } catch(...) {}

            // triggerbot
            try {
            if      (key=="tb.fov_circle_colour") parse_c4(settings::triggerbot::fov_circle_colour);
            else if (key=="tb.fov_outline_colour") parse_c4(settings::triggerbot::fov_outline_colour);
            else if (key=="tb.fov_fill") settings::triggerbot::fov_fill=bv;
            else if (key=="tb.fov_fill_colour") parse_c4(settings::triggerbot::fov_fill_colour);
            else if (key=="tb.fov_fill_opacity") settings::triggerbot::fov_fill_opacity=fval;
            else if (key=="tb.dead_check") settings::triggerbot::dead_check=bv;
            } catch(...) {}

            // walkspeed
            try {
            if      (key=="ws.enabled") settings::walkspeed::enabled=bv;
            else if (key=="ws.speed") settings::walkspeed::speed=fval;
            else if (key=="ws.keybind") settings::walkspeed::keybind=ival;
            else if (key=="ws.keybind_mode") settings::walkspeed::keybind_mode=ival;
            } catch(...) {}

            // streamproof
            try {
            if      (key=="spf.enabled") settings::streamproof::enabled=bv;
            } catch(...) {}

            // skybox
            try {
            if      (key=="sky.video_file") snprintf(settings::skybox::video_file, sizeof settings::skybox::video_file, "%s", val.c_str());
            } catch(...) {}

            // skybox presets
            //
            // Flat statements instead of eight more branches of the if/else
            // chain above: MSVC counts an if/else chain against its nesting
            // limit, this file is already at it, and more levels of it are what
            // C1061 was complaining about.
            try {
            if (key=="sky.pre_on")    settings::skybox::preset_on=bv;
            if (key=="sky.pre")       settings::skybox::preset=ival;
            if (key=="sky.pre_faces") settings::skybox::preset_faces=bv;
            if (key=="sky.pre_sm")    settings::skybox::preset_sun_moon=bv;
            if (key=="sky.pre_stars") settings::skybox::preset_stars=bv;
            if (key=="sky.stars")     settings::skybox::stars_on=bv;
            if (key=="sky.starc")     settings::skybox::stars_count=ival;
            if (key=="sky.pre_cel")   settings::skybox::hide_celestial=bv;
            if (key=="sky.pre_ori")   settings::skybox::orientation_on=bv;
            if (key=="sky.pre_oriv")  parse_c3(settings::skybox::orientation);
            } catch(...) {}

            // exploits
            try {
            if      (key=="exp.low_gravity") settings::exploits::low_gravity=bv;
            else if (key=="exp.gravity_value") settings::exploits::gravity_value=fval;
            else if (key=="exp.tp_cursor") settings::exploits::tp_cursor=bv;
            else if (key=="exp.tp_cursor_key") settings::exploits::tp_cursor_key=ival;
            else if (key=="exp.tp_cursor_dist") settings::exploits::tp_cursor_dist=fval;
            } catch(...) {}

            // theme
            try {
            if      (key=="theme.accent") parse_c4(settings::theme::accent);
            else if (key=="theme.bg_one") parse_c4(settings::theme::bg_one);
            else if (key=="theme.bg_two") parse_c4(settings::theme::bg_two);
            else if (key=="theme.stroke") parse_c4(settings::theme::stroke);
            else if (key=="theme.outline") parse_c4(settings::theme::outline);
            else if (key=="theme.text_active") parse_c4(settings::theme::text_active);
            else if (key=="theme.text_inactive") parse_c4(settings::theme::text_inactive);
            } catch(...) {}

            // menu
            try {
            if      (key=="ui.vsync") settings::menu::vsync=bv;
            else if (key=="ui.performance_mode") settings::menu::performance_mode=bv;
            else if (key=="ui.fps_limit") settings::menu::fps_limit=ival;
            } catch(...) {}

            // settings
            try {
            if      (key=="app.hide_console") settings::settings::hide_console=bv;
            } catch(...) {}
        }

        // The fill image lives on the device, not in the file, so a restored path
        // has to be pushed to it again before the box fill can draw anything.
        if (sync::hooks().fill_image)
            settings::visuals::box_fill_image_loaded =
                sync::hooks().fill_image(settings::visuals::box_fill_image_path,
                                         settings::visuals::box_fill_remove_bg);
    }
}

namespace preview
{
    static bool part_at(const cache::entity_t& e, const char* name, math::vector3& out)
    {
        const auto it = e.parts.find(name);
        if (it == e.parts.end())
            return false;

        const rbx::primitive_t prim = it->second.get_primitive();
        if (!prim.address)
            return false;

        out = prim.get_position();
        return true;
    }

    static bool build(const cache::entity_t& e, sync::PreviewRig& out)
    {

        const bool r15 = e.parts.count("UpperTorso") > 0;

        const char* head_name   = "Head";
        const char* chest_name  = r15 ? "UpperTorso" : "Torso";
        const char* pelvis_name = r15 ? "LowerTorso" : "Torso";
        const char* l_arm       = r15 ? "LeftUpperArm"  : "Left Arm";
        const char* l_hand      = r15 ? "LeftHand"      : "Left Arm";
        const char* r_arm       = r15 ? "RightUpperArm" : "Right Arm";
        const char* r_hand      = r15 ? "RightHand"     : "Right Arm";
        const char* l_foot      = r15 ? "LeftFoot"      : "Left Leg";
        const char* r_foot      = r15 ? "RightFoot"     : "Right Leg";

        math::vector3 head{}, chest{}, pelvis{}, la{}, ra{}, lh{}, rh{}, lf{}, rf{};
        if (!part_at(e, head_name, head))   return false;
        if (!part_at(e, chest_name, chest)) return false;
        if (!part_at(e, pelvis_name, pelvis)) return false;

        if (!part_at(e, l_arm,  la)) la = chest;
        if (!part_at(e, l_hand, lh)) lh = la;
        if (!part_at(e, r_arm,  ra)) ra = chest;
        if (!part_at(e, r_hand, rh)) rh = ra;
        if (!part_at(e, l_foot, lf)) lf = pelvis;
        if (!part_at(e, r_foot, rf)) rf = pelvis;

        auto put = [&](sync::PreviewJoint slot, const math::vector3& v)
        {
            const int i = (int)slot;
            out.joints[i][0] = v.x;
            out.joints[i][1] = v.y;
            out.joints[i][2] = v.z;
        };

        put(sync::PreviewJoint::Head,  head);
        put(sync::PreviewJoint::Neck,  (head + chest) * 0.5f);
        put(sync::PreviewJoint::Chest, chest);
        put(sync::PreviewJoint::Pelvis, pelvis);

        put(sync::PreviewJoint::LegsBase, pelvis + (pelvis - chest) * 0.5f);
        put(sync::PreviewJoint::LeftShoulder,  la);
        put(sync::PreviewJoint::LeftHand,      lh);
        put(sync::PreviewJoint::RightShoulder, ra);
        put(sync::PreviewJoint::RightHand,     rh);
        put(sync::PreviewJoint::LeftFoot,      lf);
        put(sync::PreviewJoint::RightFoot,     rf);

        for (int i = 0; i < sync::kPreviewJointCount; ++i)
        {
            if (!std::isfinite(out.joints[i][0]) ||
                !std::isfinite(out.joints[i][1]) ||
                !std::isfinite(out.joints[i][2]))
                return false;
        }

        const std::string& shown = e.display_name.empty() ? e.name : e.display_name;
        snprintf(out.name, sizeof out.name, "%s", shown.c_str());
        out.health   = e.health > 0.f ? e.health : 100.f;
        out.rig_type = r15 ? 1 : 0;

        out.state = e.state;
        snprintf(out.tool, sizeof out.tool, "%s", e.tool_name.empty() ? "" : e.tool_name.c_str());

        out.user_id  = e.user_id;
        out.valid    = true;
        return true;
    }

    static bool bridge(sync::PreviewRig& out)
    {

        cache::entity_t chosen{};
        bool  have_chosen    = false;
        bool  chosen_is_local = false;
        float chosen_dist    = 0.f;

        cache::entity_t fallback{};
        bool  have_fallback  = false;
        float fallback_dist  = 0.f;
        {
            std::lock_guard<std::mutex> lk(cache::mtx);

            const std::uint64_t local = cache::cached_local_player.instance.address;

            math::vector3 from{};
            bool have_from = false;
            {
                const auto it = cache::cached_local_player.parts.find("Head");
                if (it != cache::cached_local_player.parts.end())
                {
                    const rbx::primitive_t prim = it->second.get_primitive();
                    if (prim.address)
                    {
                        const math::vector3 p = prim.get_position();
                        if (math::finite(p))
                        {
                            from = p;
                            have_from = true;
                        }
                    }
                }
            }

            const cache::entity_t* best = nullptr;
            float best_dist = 0.f;

            for (const cache::entity_t& e : cache::cached_players)
            {
                if (!e.instance.address || e.instance.address == local)
                    continue;
                if (cache::is_dead(e))
                    continue;

                math::vector3 head{};
                if (!part_at(e, "Head", head))
                    continue;

                const float d = have_from ? head.distance(from) : 0.f;
                if (!best || d < best_dist)
                {
                    best = &e;
                    best_dist = d;
                }
            }

            if (best)
            {
                fallback = *best;
                fallback_dist = best_dist;
                have_fallback = true;
            }

            if (local)
            {
                chosen = cache::cached_local_player;
                chosen_is_local = true;
                have_chosen = true;
            }
        }

        const auto label_distance = [](float studs)
        {

            return settings::visuals::distance_measurement == 1 ? studs * 0.28f : studs;
        };

        if (have_chosen && build(chosen, out))
        {
            out.distance = 0.f;
            return true;
        }

        if (have_fallback && build(fallback, out))
        {
            out.distance = label_distance(fallback_dist);
            return true;
        }

        return false;
    }
}

namespace players_bridge
{

    static bool entity_origin(const cache::entity_t& e, math::vector3& out)
    {
        static const char* const k_order[] = { "HumanoidRootPart", "Head", "Torso", "UpperTorso" };
        for (const char* name : k_order)
            if (preview::part_at(e, name, out))
                return true;

        for (const auto& entry : e.parts)
            if (preview::part_at(e, entry.first.c_str(), out))
                return true;

        return false;
    }

    static bool page_ok(std::uint64_t address, std::size_t size)
    {
        if (!address) return false;
        HANDLE proc = memory ? memory->get_process_handle() : nullptr;
        if (!proc) return false;

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(proc, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0)
            return false;
        if (mbi.State != MEM_COMMIT) return false;
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
        if (reinterpret_cast<std::uint64_t>(mbi.BaseAddress) + mbi.RegionSize < address + size)
            return false;
        return true;
    }

    static void fill(sync::PlayerRow& row, const cache::entity_t& e, bool local, const math::vector3& from,
        bool have_from)
    {
        row = sync::PlayerRow{};
        row.valid        = e.instance.address != 0;
        row.local        = local;
        row.instance     = e.instance.address;
        row.user_id      = e.user_id;
        row.health       = e.health;
        row.max_health   = e.max_health > 0.f ? e.max_health : 100.f;
        row.rig_type     = e.rig_type ? 1 : 0;
        row.teammate     = local ? false : cache::is_teammate(e);
        row.dead         = cache::is_dead(e);
        row.distance     = -1.f;

        snprintf(row.name, sizeof row.name, "%s", e.name.c_str());
        snprintf(row.display_name, sizeof row.display_name, "%s",
            e.display_name.empty() ? e.name.c_str() : e.display_name.c_str());
        snprintf(row.team, sizeof row.team, "%s",
            e.team_name.empty() ? "" : e.team_name.c_str());
        snprintf(row.tool, sizeof row.tool, "%s",
            e.tool_name.empty() ? "" : e.tool_name.c_str());

        row.enemy = !local && !row.teammate && e.team != 0 && cache::cached_local_team != 0;

        row.account_age_days = -1;

        if (have_from)
        {
            math::vector3 here{};
            if (entity_origin(e, here))
                row.distance = here.distance(from);
        }
    }

    static bool find_entity(unsigned long long instance, cache::entity_t& out)
    {
        if (!instance) return false;
        std::lock_guard<std::mutex> lk(cache::mtx);
        for (const cache::entity_t& e : cache::cached_players)
        {
            if (e.instance.address == instance) { out = e; return true; }
        }
        if (cache::cached_local_player.instance.address == instance)
        {
            out = cache::cached_local_player;
            return true;
        }
        return false;
    }

    static bool entity_anchor(const cache::entity_t& e, math::vector3& out)
    {
        static const char* const k_order[] = { "HumanoidRootPart", "Torso", "UpperTorso", "Head" };
        rbx::primitive_t prim;
        for (const char* name : k_order)
        {
            const auto it = e.parts.find(name);
            if (it == e.parts.end()) continue;
            prim = it->second.get_primitive();
            if (!prim.address) continue;
            out = prim.get_position();
            return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
        }
        return false;
    }

    static bool local_anchor(rbx::primitive_t& out)
    {
        std::lock_guard<std::mutex> lk(cache::mtx);
        static const char* const k_order[] = { "HumanoidRootPart", "Torso", "UpperTorso", "Head" };
        for (const char* name : k_order)
        {
            const auto it = cache::cached_local_player.parts.find(name);
            if (it == cache::cached_local_player.parts.end()) continue;
            out = it->second.get_primitive();
            if (out.address) return true;
        }
        return false;
    }

    static bool teleport_to(unsigned long long instance)
    {
        cache::entity_t e{};
        if (!find_entity(instance, e)) return false;

        math::vector3 to{};
        if (!entity_anchor(e, to)) return false;

        rbx::primitive_t mine;
        if (!local_anchor(mine)) return false;

        if (!std::isfinite(to.x) || !std::isfinite(to.y) || !std::isfinite(to.z))
            return false;
        if (std::fabs(to.x) > 1.0e7f || std::fabs(to.y) > 1.0e7f || std::fabs(to.z) > 1.0e7f)
            return false;

        to.y += 1.5f;

        if (!page_ok(mine.address + Offsets::Primitive::Position, sizeof(math::vector3)))
            return false;
        if (!page_ok(mine.address + Offsets::Primitive::AssemblyLinearVelocity, sizeof(math::vector3)))
            return false;

        memory->write<math::vector3>(mine.address + Offsets::Primitive::Position, to);
        memory->write<math::vector3>(mine.address + Offsets::Primitive::AssemblyLinearVelocity, {});
        return true;
    }

    static unsigned long long g_cam_prev_subject = 0;
    static std::int32_t       g_cam_prev_type    = -1;
    static bool               g_cam_saved        = false;

    static std::uint64_t current_camera()
    {
        if (!game::workspace.address) return 0;
        const std::uint64_t cam = memory->read<std::uint64_t>(
            game::workspace.address + Offsets::Workspace::CurrentCamera);
        return page_ok(cam, 8) ? cam : 0;
    }

    static bool spectate(unsigned long long instance, bool on)
    {
        const std::uint64_t cam = current_camera();
        if (!cam) return false;

        if (!on)
        {
            if (!g_cam_saved) return false;
            if (g_cam_prev_subject && page_ok(cam + Offsets::Camera::CameraSubject, 8))
                memory->write<std::uint64_t>(cam + Offsets::Camera::CameraSubject, g_cam_prev_subject);
            if (g_cam_prev_type >= 0 && page_ok(cam + Offsets::Camera::CameraType, 4))
                memory->write<std::int32_t>(cam + Offsets::Camera::CameraType, g_cam_prev_type);
            g_cam_saved = false;
            g_cam_prev_subject = 0;
            g_cam_prev_type = -1;
            return true;
        }

        cache::entity_t e{};
        if (!find_entity(instance, e)) return false;

        const std::uint64_t hum = e.humanoid.address;
        if (!hum || !page_ok(hum, 8)) return false;
        if (!page_ok(cam + Offsets::Camera::CameraSubject, 8) ||
            !page_ok(cam + Offsets::Camera::CameraType, 4)) return false;

        if (!g_cam_saved)
        {
            g_cam_prev_subject = memory->read<std::uint64_t>(cam + Offsets::Camera::CameraSubject);
            g_cam_prev_type    = memory->read<std::int32_t>(cam + Offsets::Camera::CameraType);
            g_cam_saved        = true;
        }

        memory->write<std::uint64_t>(cam + Offsets::Camera::CameraSubject, hum);
        memory->write<std::int32_t>(cam + Offsets::Camera::CameraType, 5);
        return true;
    }

    static void set_target(unsigned long long instance)
    {
        // The pin lives in its own store, so it outlives the aim features dropping
        // their transient holds and needs no entity in the cache to be set. The
        // aimbot picks it up on its next tick; no setting is touched here.
        targets::set((std::uint64_t)instance);
    }

    struct roster_t
    {
        std::uint64_t tick = ~0ull;
        int           count = 0;
        sync::PlayerRow rows[sync::kMaxPlayerRows];
    };
    static roster_t g_roster;

    static int snapshot(void* raw, int max)
    {
        sync::PlayerRow* rows = static_cast<sync::PlayerRow*>(raw);
        if (!rows || max <= 0)
            return 0;

        const std::uint64_t tick_now = cache::tick_count.load(std::memory_order_relaxed);
        if (tick_now != g_roster.tick)
        {
            std::lock_guard<std::mutex> lk(cache::mtx);

            const std::uint64_t tick = cache::tick_count.load(std::memory_order_relaxed);

            math::vector3 from{};
            const bool have_from = entity_origin(cache::cached_local_player, from);
            const std::uint64_t local_addr = cache::cached_local_player.instance.address;

            int count = 0;
            if (local_addr && count < sync::kMaxPlayerRows)
                fill(g_roster.rows[count++], cache::cached_local_player, true, from, have_from);

            for (const cache::entity_t& e : cache::cached_players)
            {
                if (count >= sync::kMaxPlayerRows)
                    break;
                if (!e.instance.address || e.instance.address == local_addr)
                    continue;
                fill(g_roster.rows[count++], e, false, from, have_from);
            }

            g_roster.count = count;
            g_roster.tick  = tick;
        }

        const int n = g_roster.count < max ? g_roster.count : max;
        std::memcpy(rows, g_roster.rows, (std::size_t)n * sizeof(sync::PlayerRow));
        return n;
    }

    static int account_age(unsigned long long instance)
    {
        if (!instance)
            return -1;

        static std::unordered_map<unsigned long long, int> s_ages;
        if (s_ages.size() > 512)
            s_ages.clear();

        const auto it = s_ages.find(instance);
        if (it != s_ages.end())
            return it->second;

        int age = -1;
        const std::uint64_t at = (std::uint64_t)instance + Offsets::Player::AccountAge;
        if (page_ok(at, 4))
            age = memory->read<std::int32_t>(at);

        s_ages.emplace(instance, age);
        return age;
    }
}

namespace cfg
{
    static void bridge_refresh()
    {
        refresh_list();
    }

    static bool bridge_save(const char* name)
    {
        if (!name || !name[0]) return false;
        save(name);
        refresh_list();
        return std::filesystem::exists(cfg_dir() + std::string(name) + ".cfg");
    }

    static bool bridge_load(const char* name)
    {
        if (!name || !name[0]) return false;
        load(name);
        return true;
    }

    static bool bridge_delete(const char* name)
    {
        if (!name || !name[0]) return false;
        std::error_code ec;
        const bool removed = std::filesystem::remove(cfg_dir() + std::string(name) + ".cfg", ec);
        refresh_list();
        return removed && !ec;
    }
}

static ImVec2 s_probe_pos{}, s_probe_size{};

static bool ui_probe_enabled()
{

    static const bool on = [] {
        char buf[8]{};
        const DWORD n = GetEnvironmentVariableA("AETHER_UI_PROBE", buf, sizeof(buf));
        return n > 0 && buf[0] != '0';
    }();
    return on;
}

static void ui_probe_rect(const ImVec2& pos, const ImVec2& size)
{
    s_probe_pos  = pos;
    s_probe_size = size;
}


static void keep_on_screen()
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 p = ImGui::GetWindowPos();
    const ImVec2 s = ImGui::GetWindowSize();

    ImVec2 q = p;
    if (q.x + s.x > display.x) q.x = display.x - s.x;
    if (q.y + s.y > display.y) q.y = display.y - s.y;
    if (q.x < 0.f) q.x = 0.f;
    if (q.y < 0.f) q.y = 0.f;

    if (q.x != p.x || q.y != p.y)
        ImGui::SetWindowPos(q);
}

struct theme_palette_t
{
    const char* name;
    float accent[3];
    float bg[3];
    float surface[3];
    float border[3];
    float text[3];
    float text_dim[3];
};

static const theme_palette_t k_themes[] =
{
     { "Midnight Blue", { .035f,.314f,.866f }, { .047f,.047f,.055f }, { .075f,.075f,.090f }, { .158f,.158f,.181f }, { .784f,.784f,.784f }, { .533f,.533f,.533f } },
     { "Forest Green",  { .165f,.456f,.275f }, { .047f,.047f,.055f }, { .075f,.075f,.090f }, { .158f,.158f,.181f }, { .784f,.784f,.784f }, { .533f,.533f,.533f } },
     { "Purple",        { .444f,.130f,.745f }, { .047f,.047f,.055f }, { .075f,.075f,.090f }, { .158f,.158f,.181f }, { .784f,.784f,.784f }, { .533f,.533f,.533f } },
     { "Hot Pink",      { .745f,.130f,.501f }, { .047f,.047f,.055f }, { .075f,.075f,.090f }, { .158f,.158f,.181f }, { .784f,.784f,.784f }, { .533f,.533f,.533f } },
     { "Titanium White",{ 1.f,1.f,1.f },       { .047f,.047f,.055f }, { .075f,.075f,.090f }, { .200f,.200f,.220f }, { .900f,.900f,.900f }, { .650f,.650f,.650f } },
     { "Mint",          { .631f,1.f,.814f },   { .047f,.047f,.055f }, { .075f,.075f,.090f }, { .158f,.158f,.181f }, { .784f,.784f,.784f }, { .533f,.533f,.533f } },
     { "Hazard",        { .556f,.718f,.227f }, { .047f,.047f,.055f }, { .075f,.075f,.090f }, { .158f,.158f,.181f }, { .784f,.784f,.784f }, { .533f,.533f,.533f } },
     { "Ocean Cyan",    { .137f,.745f,.854f }, { .045f,.055f,.065f }, { .065f,.075f,.085f }, { .110f,.130f,.150f }, { .780f,.930f,1.f },  { .530f,.630f,.730f } },
     { "Canada",        { .850f,.050f,.050f }, { .060f,.060f,.060f }, { .090f,.090f,.090f }, { .160f,.160f,.160f }, { 1.f,1.f,1.f },      { .700f,.700f,.700f } },
     { "Steel",         { .290f,.540f,.690f }, { .030f,.040f,.050f }, { .060f,.080f,.100f }, { .110f,.130f,.150f }, { .920f,.950f,1.f },  { .600f,.700f,.800f } },
     { "Skyline Blue",  { .200f,.550f,.950f }, { .030f,.040f,.060f }, { .060f,.070f,.090f }, { .100f,.120f,.150f }, { .850f,.900f,1.f },  { .550f,.600f,.700f } },
     { "Cherry Blossom",{ 1.f,.670f,.830f },   { .060f,.050f,.060f }, { .090f,.070f,.090f }, { .130f,.110f,.130f }, { 1.f,.800f,.900f }, { .700f,.500f,.600f } },
     { "Atlanta",       { .604f,.498f,.675f }, { .141f,.141f,.184f }, { .165f,.165f,.220f }, { .208f,.208f,.263f }, { .784f,.784f,.784f }, { .533f,.533f,.533f } },
     { "Vaporwave",     { 1.000f,.360f,.860f },{ .075f,.055f,.125f }, { .120f,.085f,.195f }, { .215f,.160f,.315f }, { .950f,.900f,1.000f },{ .620f,.545f,.750f } },
     { "Cyberpunk",     { 1.000f,.830f,.020f },{ .035f,.045f,.065f }, { .065f,.085f,.115f }, { .125f,.165f,.215f }, { .900f,.960f,1.000f },{ .545f,.615f,.700f } },
     { "Sunset",        { 1.000f,.450f,.250f },{ .070f,.050f,.070f }, { .110f,.080f,.100f }, { .200f,.140f,.160f }, { 1.000f,.900f,.850f },{ .700f,.580f,.550f } },
     { "Nord",          { .530f,.750f,.820f }, { .120f,.140f,.170f }, { .160f,.190f,.230f }, { .240f,.280f,.330f }, { .850f,.890f,.920f },{ .560f,.620f,.680f } },
     { "Amoled",        { .000f,.900f,.650f }, { .000f,.000f,.000f }, { .040f,.040f,.050f }, { .100f,.100f,.120f }, { .920f,.920f,.940f },{ .450f,.450f,.480f } },
     { "Gruvbox",       { .980f,.600f,.200f }, { .160f,.150f,.130f }, { .210f,.200f,.170f }, { .300f,.280f,.240f }, { .920f,.860f,.700f },{ .600f,.560f,.480f } },
     { "Rosewood",      { .830f,.420f,.520f }, { .100f,.080f,.090f }, { .150f,.120f,.130f }, { .240f,.190f,.200f }, { .950f,.880f,.880f },{ .620f,.540f,.550f } },
     { "Toxic",         { .600f,1.000f,.200f },{ .030f,.050f,.030f }, { .060f,.090f,.060f }, { .130f,.190f,.120f }, { .880f,.980f,.860f },{ .500f,.620f,.480f } },
     { "Ice",           { .550f,.830f,1.000f },{ .050f,.070f,.100f }, { .090f,.120f,.160f }, { .160f,.210f,.280f }, { .900f,.950f,1.000f },{ .550f,.630f,.720f } },
     { "Copper",        { .850f,.550f,.320f }, { .070f,.060f,.050f }, { .110f,.090f,.080f }, { .200f,.170f,.140f }, { .960f,.910f,.860f },{ .620f,.560f,.500f } },
     { "Neon Noir",     { 1.000f,.160f,.620f },{ .030f,.020f,.045f }, { .070f,.050f,.100f }, { .145f,.100f,.200f }, { .950f,.900f,.990f },{ .560f,.480f,.660f } },
     { "Jade",          { .180f,.850f,.620f }, { .020f,.060f,.050f }, { .050f,.115f,.090f }, { .100f,.210f,.175f }, { .880f,.980f,.940f },{ .480f,.660f,.600f } },
     { "Monochrome",    { .920f,.920f,.920f }, { .040f,.040f,.045f }, { .090f,.090f,.100f }, { .200f,.200f,.215f }, { .960f,.960f,.970f },{ .550f,.550f,.570f } },
     { "Bloodmoon",     { .950f,.160f,.160f }, { .050f,.020f,.025f }, { .110f,.040f,.055f }, { .220f,.080f,.100f }, { 1.000f,.900f,.900f },{ .660f,.460f,.480f } },
     { "Exterium",      { .902f,.525f,.878f }, { .051f,.055f,.063f }, { .075f,.075f,.075f }, { .137f,.137f,.137f }, { 1.000f,1.000f,1.000f },{ .588f,.588f,.588f } },
};

static const char* const k_theme_names_csv =
    "Midnight Blue\0Forest Green\0Purple\0Hot Pink\0Titanium White\0"
    "Mint\0Hazard\0Ocean Cyan\0Canada\0Steel\0Skyline Blue\0Cherry Blossom\0Atlanta\0"
    "Vaporwave\0Cyberpunk\0Sunset\0Nord\0Amoled\0Gruvbox\0Rosewood\0Toxic\0Ice\0Copper\0"
    "Neon Noir\0Jade\0Monochrome\0Bloodmoon\0Exterium\0";

static const int k_theme_count = (int)(sizeof(k_themes) / sizeof(k_themes[0]));

static ImVec4 theme_scale(const ImVec4& c, float f)
{
    return ImVec4(ImClamp(c.x * f, 0.f, 1.f), ImClamp(c.y * f, 0.f, 1.f), ImClamp(c.z * f, 0.f, 1.f), c.w);
}

static ImVec4 theme_blend(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

static void apply_theme(int idx)
{
    ImGuiStyle& ts = ImGui::GetStyle();

    if (idx < 0 || idx >= k_theme_count)
        idx = 0;
    const theme_palette_t& pal = k_themes[idx];

    auto v4 = [](const float c[3]) { return ImVec4(c[0], c[1], c[2], 1.f); };

    ImVec4 accent = v4(pal.accent);
    if (settings::appearance::custom_accent)
        accent = ImVec4(settings::appearance::accent[0], settings::appearance::accent[1],
                        settings::appearance::accent[2], 1.f);

    const ImVec4 bg       = v4(pal.bg);
    const ImVec4 surface  = v4(pal.surface);
    const ImVec4 border   = v4(pal.border);
    const ImVec4 text     = v4(pal.text);
    const ImVec4 text_dim = v4(pal.text_dim);

    const ImVec4 surface_lo = theme_scale(surface, 0.72f);
    const ImVec4 header     = theme_blend(surface, accent, 0.20f);
    const ImVec4 header_hi  = theme_blend(surface, accent, 0.34f);
    const ImVec4 header_lo  = theme_blend(surface, accent, 0.12f);

    ts.Colors[ImGuiCol_WindowBg]         = bg;
    ts.Colors[ImGuiCol_ChildBg]          = surface;
    ts.Colors[ImGuiCol_PopupBg]          = theme_blend(bg, surface, 0.35f);
    ts.Colors[ImGuiCol_Text]             = text;
    ts.Colors[ImGuiCol_TextDisabled]     = text_dim;

    const ImVec4 border_acc = theme_blend(border, accent, 0.22f);
    ts.Colors[ImGuiCol_Border]           = border_acc;
    ts.Colors[ImGuiCol_BorderShadow]     = ImVec4();
    ts.Colors[ImGuiCol_Separator]        = theme_blend(border, accent, 0.10f);
    ts.Colors[ImGuiCol_SeparatorHovered] = accent;
    ts.Colors[ImGuiCol_SeparatorActive]  = theme_scale(accent, 0.70f);

    ts.Colors[ImGuiCol_SliderGrab]       = accent;
    ts.Colors[ImGuiCol_SliderGrabActive] = theme_scale(accent, 0.72f);

    ts.Colors[ImGuiCol_Button]        = surface;
    ts.Colors[ImGuiCol_ButtonHovered] = theme_blend(surface, accent, 0.16f);
    ts.Colors[ImGuiCol_ButtonActive]  = theme_blend(surface, accent, 0.28f);

    ts.Colors[ImGuiCol_FrameBg]        = surface_lo;
    ts.Colors[ImGuiCol_FrameBgHovered] = theme_blend(surface_lo, accent, 0.16f);
    ts.Colors[ImGuiCol_FrameBgActive]  = theme_blend(surface_lo, accent, 0.26f);

    ts.Colors[ImGuiCol_Header]        = header;
    ts.Colors[ImGuiCol_HeaderHovered] = header_hi;
    ts.Colors[ImGuiCol_HeaderActive]  = header_lo;

    ts.Colors[ImGuiCol_TitleBg]          = theme_blend(bg, surface, 0.50f);
    ts.Colors[ImGuiCol_TitleBgActive]    = theme_blend(bg, surface, 0.70f);
    ts.Colors[ImGuiCol_TitleBgCollapsed] = bg;

    ts.Colors[ImGuiCol_CheckMark] = ImVec4(0.02f, 0.02f, 0.03f, 1.f);

    ts.Colors[ImGuiCol_ScrollbarBg]          = theme_blend(bg, surface, 0.14f);
    ts.Colors[ImGuiCol_ScrollbarGrab]        = theme_blend(surface, accent, 0.44f);
    ts.Colors[ImGuiCol_ScrollbarGrabHovered] = theme_blend(surface, accent, 0.70f);
    ts.Colors[ImGuiCol_ScrollbarGrabActive]  = accent;

    ts.Colors[ImGuiCol_PlotLines]     = accent;
    ts.Colors[ImGuiCol_PlotHistogram] = accent;
    ts.Colors[ImGuiCol_NavHighlight]  = accent;
    ts.Colors[ImGuiCol_ResizeGrip]        = ImVec4();
    ts.Colors[ImGuiCol_ResizeGripHovered] = ImVec4();
    ts.Colors[ImGuiCol_ResizeGripActive]  = ImVec4();

    const float r = ImClamp(settings::appearance::rounding, 0.f, 14.f);
    ts.WindowRounding    = r;
    ts.ChildRounding     = r * 0.72f;
    ts.FrameRounding     = r * 0.50f;
    ts.PopupRounding     = r * 0.65f;
    ts.GrabRounding      = r * 0.50f;
    ts.TabRounding       = r * 0.60f;

    ts.ScrollbarSize     = 7.f;
    ts.ScrollbarRounding = ImMax(2.f, r * 0.9f);
    ts.GrabRounding      = ImMax(ts.GrabRounding, 3.f);
    ts.GrabMinSize       = 14.f;

    settings::theme::index = idx;
    settings::theme::accent[0] = accent.x;      settings::theme::accent[1] = accent.y;      settings::theme::accent[2] = accent.z;      settings::theme::accent[3] = 1.f;
    settings::theme::bg_one[0] = bg.x;          settings::theme::bg_one[1] = bg.y;          settings::theme::bg_one[2] = bg.z;          settings::theme::bg_one[3] = 1.f;
    settings::theme::bg_two[0] = surface.x;     settings::theme::bg_two[1] = surface.y;     settings::theme::bg_two[2] = surface.z;     settings::theme::bg_two[3] = 1.f;
    settings::theme::stroke[0] = border_acc.x;  settings::theme::stroke[1] = border_acc.y;  settings::theme::stroke[2] = border_acc.z;  settings::theme::stroke[3] = 1.f;
    settings::theme::outline[0] = border_acc.x; settings::theme::outline[1] = border_acc.y; settings::theme::outline[2] = border_acc.z; settings::theme::outline[3] = 1.f;
    settings::theme::text_active[0] = text.x;   settings::theme::text_active[1] = text.y;   settings::theme::text_active[2] = text.z;   settings::theme::text_active[3] = 1.f;
    settings::theme::text_inactive[0] = text_dim.x; settings::theme::text_inactive[1] = text_dim.y; settings::theme::text_inactive[2] = text_dim.z; settings::theme::text_inactive[3] = 1.f;
}

static float g_bg_fade = 0.f;

static aether::bg::Config background_config()
{
    using namespace settings::appearance;

    aether::bg::Config c;
    c.effect   = bg_effect;
    c.density  = bg_density;
    c.speed    = bg_speed;
    c.size     = bg_size;
    c.opacity  = bg_opacity * ImClamp(g_bg_fade, 0.f, 1.f);
    c.color[0] = bg_color[0];
    c.color[1] = bg_color[1];
    c.color[2] = bg_color[2];
    c.color[3] = bg_color[3];
    c.wind     = bg_wind;
    c.glow     = bg_glow;
    c.parallax = bg_parallax;

    // The direction the falling elements fall.  The angle is the one the
    // simulation and the streaks are both built from (`bg_dir` is the UI's
    // preset, and writing a preset writes this), the spread is how far a single
    // drop may lean off it, and the streak is how long a drop trails behind
    // itself.
    c.angle    = bg_angle;
    c.streak   = bg_streak;
    c.spread   = bg_spread;
    c.width    = bg_width;

    // The material the element is seen through.  The blur and the tint are the
    // backdrop's own sliders and this file does not change them - it only hands
    // their state to the element, so a heavy frost softens what drifts over it
    // and a tinted capture does not get the light values meant for a clear one.
    // With the backdrop off (or No Transparency on) both are zero and every
    // element draws at the plain-plate values it always had.
    const bool frosted = settings::backdrop::enabled && !no_transparency;
    c.veil = frosted ? ImClamp(settings::backdrop::blur / 100.f, 0.f, 1.f) : 0.f;
    c.tint = frosted ? ImClamp(settings::backdrop::tint / 100.f, 0.f, 1.f) : 0.f;
    return c;
}

static void draw_background_elements(ImDrawList* dl, ImVec2 rect_min, ImVec2 rect_max)
{
    if (!settings::appearance::bg_enabled || !dl)
        return;

    const ImVec2 a(ImMin(rect_min.x, rect_max.x), ImMin(rect_min.y, rect_max.y));
    const ImVec2 b(ImMax(rect_min.x, rect_max.x), ImMax(rect_min.y, rect_max.y));

    aether::bg::render(dl, a, b, background_config());
}

static void draw_pane_background()
{
    if (!settings::appearance::bg_enabled || !settings::appearance::bg_menu_only)
        return;

    const ImVec2 p = ImGui::GetWindowPos();
    const ImVec2 s = ImGui::GetWindowSize();
    draw_background_elements(ImGui::GetWindowDrawList(), p, p + s);
}

CGui::CGui()
{
    m_hWnd           = 0;
    m_pDevice        = 0;
    m_pDeviceContext = 0;
    m_pTextEditor    = 0;
    m_pTextures      = 0;
    m_iCurrentPage   = 0;

    m_Tabs.push_back("Combat");
    m_Tabs.push_back("Esp");
    m_Tabs.push_back("Misc");
    m_Tabs.push_back("World");

    memset(m_bSelectedBones,    false, sizeof m_bSelectedBones);
    memset(m_bSelectedCategory, false, sizeof m_bSelectedCategory);
    m_bSelectedCategory[ImCategory_Main] = true;
}

CGui::~CGui() { surface::shutdown(); }

bool CGui::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* device_context)
{
    if (!hwnd || !device || !device_context) return false;

    m_hWnd           = hwnd;
    m_pDevice        = device;
    m_pDeviceContext = device_context;

    // The interface's light and its contour maps, built once on the device.  A
    // light is then one textured quad instead of a forty-triangle fan, and it
    // has no polygon rim and no eight-bit rings to show.
    if (!surface::init(device))
        debug::warn("surface textures could not be built; the interface falls back to fan gradients");

    m_pTextures = new CTextures();
    m_pTextures->Initialize(device);

    bool result = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.IniFilename = nullptr;
    GImGui->NavDisableHighlight = true;

    ImGui::StyleColorsDark();

    style.WindowRounding    = 4;
    style.ChildRounding     = 3;
    style.FrameRounding     = 2.5f;
    style.PopupRounding     = 2;
    style.ScrollbarRounding = 2;
    style.ButtonTextAlign   = {.5f,.5f};
    style.WindowTitleAlign  = {.5f,.5f};

    style.FramePadding      = {8.f,8.f};
    style.WindowPadding     = {10.f,10.f};
    style.ItemSpacing       = {10.f,5.f};
    style.ItemInnerSpacing  = {style.WindowPadding.x, 4.f};
    style.WindowBorderSize  = 1;
    style.FrameBorderSize   = 1;
    style.ScrollbarSize     = 7.f;
    style.GrabMinSize       = 1.f;
    style.DisabledAlpha     = 0.5f;

    style.Colors[ImGuiCol_WindowBg]  = ImAdd::HexToColorVec4(0x0c0c0c, 1.f);
    style.Colors[ImGuiCol_ChildBg]   = ImAdd::HexToColorVec4(0x121212, 1.f);
    style.Colors[ImGuiCol_PopupBg]   = ImAdd::HexToColorVec4(0x0d0d0d, 1.f);
    style.Colors[ImGuiCol_CheckMark] = style.Colors[ImGuiCol_ChildBg];
    style.Colors[ImGuiCol_Text]         = ImAdd::HexToColorVec4(0xb4b4b4, 1.f);
    style.Colors[ImGuiCol_TextDisabled] = ImAdd::HexToColorVec4(0x8c8c8c, 1.f);
    style.Colors[ImGuiCol_SliderGrab]       = ImAdd::HexToColorVec4(0x6b6ca0, 1.f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImAdd::HexToColorVec4(0x454665, 1.f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4();
    style.Colors[ImGuiCol_Border]    = ImAdd::HexToColorVec4(0x333333, 1.f);
    style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_Button]         = ImAdd::HexToColorVec4(0x252525, 1.f);
    style.Colors[ImGuiCol_ButtonHovered]  = ImAdd::HexToColorVec4(0x282828, 1.f);
    style.Colors[ImGuiCol_ButtonActive]   = ImAdd::HexToColorVec4(0x202020, 1.f);
    style.Colors[ImGuiCol_FrameBg]        = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_FrameBgHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_FrameBgActive]  = style.Colors[ImGuiCol_ButtonActive];
    style.Colors[ImGuiCol_Header]        = ImAdd::HexToColorVec4(0x2a2a2a, 1.f);
    style.Colors[ImGuiCol_HeaderHovered] = ImAdd::HexToColorVec4(0x3f3f3f, 1.f);
    style.Colors[ImGuiCol_HeaderActive]  = ImAdd::HexToColorVec4(0x191919, 1.f);
    style.Colors[ImGuiCol_ScrollbarBg]          = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImAdd::HexToColorVec4(0x414141, 1.f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = style.Colors[ImGuiCol_ScrollbarGrab];
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = style.Colors[ImGuiCol_ScrollbarGrab];
    style.Colors[ImGuiCol_ResizeGrip]        = ImVec4();
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4();
    style.Colors[ImGuiCol_ResizeGripActive]  = ImVec4();
    style.Colors[ImGuiCol_SeparatorActive]   = ImVec4();
    style.Colors[ImGuiCol_SeparatorHovered]  = ImVec4();

    aether::fonts::load(io.Fonts, aether::icons::all_codepoints(), aether::icons::nav_codepoints());

    result &= ImGui_ImplWin32_Init(hwnd);
    result &= ImGui_ImplDX11_Init(device, device_context);

    m_pTextEditor = new CTextEditor();
    m_pTextEditor->SetLanguageDefinition(CTextEditor::LanguageDefinitionId::Lua);
    m_pTextEditor->SetPalette(CTextEditor::PaletteId::Dark);

    apply_theme(settings::theme::index);
    aether::glass::apply(style);
    cfg::refresh_list();

    sync::init();
    sync::set_hwnd(hwnd);
    sync::hooks().cfg_refresh = &cfg::bridge_refresh;
    sync::hooks().cfg_save    = cfg::bridge_save;
    sync::hooks().cfg_load    = cfg::bridge_load;
    sync::hooks().cfg_delete  = cfg::bridge_delete;
    sync::hooks().theme_count = []() -> int { return k_theme_count; };
    sync::hooks().theme_name  = [](int index) -> const char* {
        return k_themes[ImClamp(index, 0, k_theme_count - 1)].name; };
    sync::hooks().theme_apply = [](int index) { apply_theme(index); };
    sync::hooks().preview_rig = &preview::bridge;

    sync::hooks().preview_avatar = +[](ImDrawList* dl, long long user_id,
        ImVec2 min, ImVec2 max, ImU32 tint) -> bool
    {
        return avatar::draw(dl, user_id, min, max, tint);
    };

    sync::hooks().player_avatar = +[](ImDrawList* dl, long long user_id,
        ImVec2 min, ImVec2 max, ImU32 tint) -> bool
    {
        return avatar::draw(dl, user_id, min, max, tint);
    };
    sync::hooks().players_snapshot    = &players_bridge::snapshot;
    sync::hooks().player_account_age  = &players_bridge::account_age;

    sync::hooks().player_avatar_rounded = +[](ImDrawList* dl, long long user_id,
        ImVec2 min, ImVec2 max, ImU32 tint, float rounding) -> bool
    {
        return avatar::draw_head(dl, user_id, min, max, tint, rounding);
    };

    sync::hooks().session = +[](sync::SessionInfo& out) -> bool
    {
        static std::uint64_t seen_tick = 0;
        static double last_change = 0.0;
        static double last_interval = 0.0;

        const std::uint64_t tick = cache::tick_count.load(std::memory_order_relaxed);
        const double now = ImGui::GetTime();
        if (tick != seen_tick)
        {
            if (seen_tick != 0) last_interval = (now - last_change) * 1000.0;
            seen_tick = tick;
            last_change = now;
        }

        out.place_id = game_ctx::place_id;
        out.game_id  = game_ctx::game_id;
        const char* pn = game_ctx::place_name();
        snprintf(out.place_name, sizeof out.place_name, "%s", pn ? pn : "");
        out.attached = (out.place_id != 0);
        out.uptime_s = (float)now;
        out.ping_ms  = (last_interval > 0.0) ? (float)last_interval : (float)((now - last_change) * 1000.0);

        std::lock_guard<std::mutex> lk(cache::mtx);
        out.players = (int)cache::cached_players.size();
        const cache::entity_t& me = cache::cached_local_player;

        // "Loaded" is the stricter answer, and it is the one the waiting card
        // needs.  A place id is readable the moment the client starts joining,
        // while the client is still on its own loading screen - so `attached`
        // is true for the whole of that wait, and a card gated on it never
        // appeared at all.  Neither is the Player instance: it exists before
        // the character does.  The body parts resolved under it are the first
        // moment there is something to draw a game from.
        out.loaded = out.attached && !me.parts.empty();
        out.local_user_id = (long long)me.user_id;
        snprintf(out.local_name, sizeof out.local_name, "%s",
            me.display_name.empty() ? me.name.c_str() : me.display_name.c_str());
        snprintf(out.local_handle, sizeof out.local_handle, "%s", me.name.c_str());
        if (!out.local_name[0]) snprintf(out.local_name, sizeof out.local_name, "not attached");
        return true;
    };

    sync::hooks().sonar_info = +[](sync::SonarInfo& out) -> bool
    {
        const sonar::snapshot_t s = sonar::snapshot();
        out.active   = s.active;
        out.has_ping = s.has_ping;
        snprintf(out.name, sizeof out.name, "%s", s.name);
        out.distance = s.distance;
        out.health   = s.health;
        out.bearing  = s.bearing_deg;
        out.sweep    = s.sweep_deg;
        out.glow     = s.glow;
        return true;
    };

    sync::hooks().player_spectate = +[](unsigned long long instance, bool on) -> bool
    {
        return players_bridge::spectate(instance, on);
    };
    sync::hooks().player_teleport = +[](unsigned long long instance) -> bool
    {
        return players_bridge::teleport_to(instance);
    };
    sync::hooks().player_target = +[](unsigned long long instance) -> void
    {
        players_bridge::set_target(instance);
    };

    sync::hooks().aim_target_instance = +[]() -> unsigned long long
    {
        // The pin first: it is what the players tab shows as held, and what the
        // aim line and the aim-target colour follow while it is set.
        const unsigned long long pin = (unsigned long long)targets::get();
        if (pin) return pin;

        const unsigned long long ab = (unsigned long long)aimbot::player.instance.address;
        if (ab) return ab;
        // No silent aim fallback. Its target flips as the cursor drifts, so the HUD
        // and the aim line following it read as jitter on a lock the aimbot does not
        // have; the silent aim keeps its own target dot and FOV circle.
        return 0ULL;
    };

    sync::hooks().fill_image = +[](const char* path, bool remove_bg) -> bool
    {
        if (!path || !path[0])
            return false;

        // Background removal re-keys the pixels, so the image has to be pushed to
        // the device again for the toggle to take effect.
        boxfill::set_remove_bg(remove_bg);
        return boxfill::load_path(path);
    };

    sync::hooks().player_hud_anchor = +[](unsigned long long instance, float& cx, float& top) -> bool
    {
        return esp::screen_anchor((std::uint64_t)instance, cx, top);
    };

    sync::hooks().player_aim_anchor = +[](unsigned long long instance, float& cx, float& cy) -> bool
    {
        return esp::aim_anchor((std::uint64_t)instance, cx, cy);
    };

    sync::hooks().chams_pass = +[]() -> bool { return mesh::dx::shader::PassAvailable(); };

    sync::hooks().explorer_page = +[](ImVec2 pos, ImVec2 size) -> void
    {
        aether_explorer::page(pos, size);
    };

    sync::hooks().local_player = +[](char* name, int n, char* handle, int m) -> bool
    {
        std::lock_guard<std::mutex> lk(cache::mtx);
        const cache::entity_t& me = cache::cached_local_player;
        if (!me.instance.address)
            return false;
        snprintf(name, n, "%s", me.display_name.empty() ? me.name.c_str() : me.display_name.c_str());
        snprintf(handle, m, "%s", me.name.c_str());
        return true;
    };

    return result;
}

void CGui::Shutdown()
{
    if (m_pTextEditor) { delete m_pTextEditor; m_pTextEditor = nullptr; }
    if (m_pTextures)   { delete m_pTextures;   m_pTextures   = nullptr; }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void CGui::Render(const char* title, POINTS size, bool* b_open)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    float HeaderHeight = ImGui::GetFontSize() + style.WindowPadding.y * 2;

    const auto gui_phase_at = [] { return std::chrono::steady_clock::now(); };
    auto gui_phase = [](const char* name, std::chrono::steady_clock::time_point& from)
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        render->trace(name, std::chrono::duration<double, std::milli>(now - from).count());
        from = now;
    };

    const auto gui_thread_cpu_ms = []() -> double
    {
        FILETIME creation{}, exit{}, kernel{}, user{};
        if (!GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user))
            return 0.0;
        const auto ticks = [](const FILETIME& f)
        {
            return (double)((((unsigned long long)f.dwHighDateTime) << 32) | f.dwLowDateTime);
        };
        return (ticks(kernel) + ticks(user)) / 10000.0;
    };

    std::chrono::steady_clock::time_point gui_mark = gui_phase_at();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImAdd::animations             = settings::appearance::widget_animations;
    aether::ui::animations_enabled = settings::appearance::widget_animations;
    ImAdd::use_switches           = settings::appearance::switches;

    static bool s_key_last = false;
    bool s_key_cur = (GetAsyncKeyState(sync::state().menu_key) & 0x8000) != 0;
    // The menu key is inert while the overlay's intro is on screen: the card is
    // what the user sees, and a toggle during it would close a panel that has not
    // opened yet.  Five seconds later the key is the menu's again.
    if (s_key_cur && !s_key_last && b_open && !io.WantCaptureKeyboard &&
        !sync::intro_hold())
        *b_open = !(*b_open);
    s_key_last = s_key_cur;

    {
        static float s_last_rounding = -1000.f;
        static bool  s_last_custom   = false;
        static float s_last_panel    = -1.f;
        static float s_last_dock     = -1.f;
        static bool  s_last_opaque   = false;
        static float s_last_spread   = -1.f;
        static float s_last_accent[4] = { -1.f, -1.f, -1.f, -1.f };

        // No Transparency is one switch over both alphas and the frost: the
        // panel is drawn at full alpha in the theme's own dark fill, with no
        // glass over it, which is what makes it a clean plate rather than a
        // window with a solid tint.
        const bool  opaque  = settings::appearance::no_transparency;
        const float panel_a = opaque ? 1.f : ImClamp(settings::appearance::panel_alpha, 0.30f, 1.f);
        const float dock_a  = opaque ? 1.f : ImClamp(settings::appearance::dock_alpha,  0.30f, 1.f);

        aether::glass::frost = opaque ? 0.f : ImClamp(settings::appearance::glass_sheen, 0.f, 1.f);

        const float* acc = settings::appearance::accent;
        const bool accent_moved =
            s_last_accent[0] != acc[0] || s_last_accent[1] != acc[1] ||
            s_last_accent[2] != acc[2] || s_last_accent[3] != acc[3];

        if (s_last_rounding != settings::appearance::rounding ||
            s_last_custom   != settings::appearance::custom_accent ||
            s_last_spread   != settings::appearance::accent_spread ||
            accent_moved)
        {
            s_last_rounding = settings::appearance::rounding;
            s_last_custom   = settings::appearance::custom_accent;
            s_last_spread   = settings::appearance::accent_spread;
            for (int i = 0; i < 4; ++i)
                s_last_accent[i] = settings::appearance::accent[i];
            apply_theme(settings::theme::index);

            aether::glass::window_alpha = panel_a;
            aether::glass::dock_alpha   = dock_a;
            aether::glass::apply(style);
        }

        if (s_last_panel != panel_a || s_last_dock != dock_a || s_last_opaque != opaque)
        {
            s_last_panel  = panel_a;
            s_last_dock   = dock_a;
            s_last_opaque = opaque;
            aether::glass::window_alpha = panel_a;
            aether::glass::dock_alpha   = dock_a;
            aether::glass::apply(style);
        }
    }

    const bool menu_open = (b_open && *b_open);

    {
        const float target = (settings::appearance::bg_enabled && menu_open) ? 1.f : 0.f;
        g_bg_fade += (target - g_bg_fade) * ImMin(1.f, io.DeltaTime * 6.5f);
        if (ImFabs(target - g_bg_fade) < 0.002f)
            g_bg_fade = target;

        if (g_bg_fade > 0.001f && !settings::appearance::bg_menu_only)
            draw_background_elements(ImGui::GetBackgroundDrawList(), ImVec2(0, 0), io.DisplaySize);
    }

    if (menu_open)
    {

        if (sync::state().dim_background)
        {
            const float fade = ImClamp(sync::fade(), 0.f, 1.f);
            const int veil = (int)(120.f * fade + 0.5f);
            if (veil > 0)
                ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0,0), io.DisplaySize, IM_COL32(0,0,0,veil));
        }
    }

    debug::set_console_visible(!settings::settings::hide_console);


    gui_phase("newframe", gui_mark);

    uifx::frame_begin();

    sync::set_open(b_open && *b_open);
    {
        const double menu_cpu0 = gui_thread_cpu_ms();
        const std::chrono::steady_clock::time_point menu_wall0 = gui_phase_at();

        sync::draw(io.DisplaySize);

        // The panel's click, in one place rather than inside every widget.
        //
        // HoveredId is the item ImGui had under the cursor when the button went
        // down, so this fires for controls and not for empty panel, and it does
        // not need the widgets to know a sound exists.  The click is also the
        // only event worth a sound: a hover that beeps turns the panel into a
        // Geiger counter.
        if (settings::misc_ui::click_sounds &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImGuiContext& g = *ImGui::GetCurrentContext();
            if (g.HoveredId != 0)
                ui_sound::play(ui_sound::kind::click, settings::misc_ui::click_volume);
        }

        uifx::paint_controls();
        uifx::draw_overlay(ImGui::GetForegroundDrawList(), b_open && *b_open);

        const double menu_cpu1 = gui_thread_cpu_ms();
        const std::chrono::steady_clock::time_point menu_wall1 = gui_phase_at();
        const double menu_wall = std::chrono::duration<double, std::milli>(menu_wall1 - menu_wall0).count();
        const double menu_cpu  = menu_cpu1 - menu_cpu0;

        const sync::Cost c = sync::cost();
        render->trace("sync.poll",   c.poll);
        render->trace("sync.panel",  c.panel);
        render->trace("sync.page",   c.page);
        render->trace("sync.xform",  c.xform);
        render->trace("sync.card",   c.card);
        render->trace("sync.count",  c.count);
        render->trace("sync.cpu",    menu_cpu);
        render->trace("sync.wall",   menu_wall);
        render->trace("sync.wait",   menu_wall - menu_cpu);
        render->trace("sync.worst",  sync::cost_take_worst_panel());
        render->trace("sync.drew",   c.open ? 1.0 : 0.0);
    }

    gui_phase("menu", gui_mark);

    render->render_visuals();

    gui_phase("visuals", gui_mark);

    mesh::chams_flush(render->detail->render_target_view);

    gui_phase("chams", gui_mark);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    gui_phase("imguidraw", gui_mark);
}

bool CGui::MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
}

