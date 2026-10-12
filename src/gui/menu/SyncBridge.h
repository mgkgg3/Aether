#pragma once

#include <imgui/imgui.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sync
{
	bool gate_wants_mouse();
	inline constexpr const char* kBrandName = "aether";


	struct State
	{
		int  menu_key       = 0x2D;





		bool dim_background = true;




































		bool  place_notice_pending = false;
		bool  place_notice_done    = false;
		bool  place_ignore_last    = false;
		bool  roblox_missing       = false;
		int   roblox_missing_seconds = 0;
		bool  roblox_found         = false;
		char  client_version[64]{};
		unsigned long long place_id = 0;












		float glass_transparency = 45.f;


		bool wm_show = true;
		bool wm_fps  = true;
		bool wm_time = true;










		bool  kb_show    = true;
		bool  kb_mode    = true;
		bool  kb_only_active = false;
		float kb_x = 16.f, kb_y = 150.f;








		bool  th_show   = true;
		bool  th_avatar = true;
		bool  th_name   = true;
		bool  th_health = true;
		bool  th_tool   = true;
		float th_x = 16.f, th_y = 320.f;
		float th_scale = 1.f;



		std::vector<std::string> cfg_names;
		int cfg_selected = 0;
	};













	struct Backdrop
	{
		ImTextureID image = (ImTextureID)0;
		float tex_w = 0.f, tex_h = 0.f;
		float desktop_x = 0.f, desktop_y = 0.f;






		float image_scale = 1.f;








		bool capture_has_self = true;

		bool ready() const { return image != (ImTextureID)0 && tex_w > 0.f && tex_h > 0.f; }




		ImVec2 uv(float wx, float wy, float window_x, float window_y) const
		{
			const float s = image_scale > 0.05f ? image_scale : 1.f;
			return ImVec2(((window_x + wx) * s - desktop_x) / tex_w,
			              ((window_y + wy) * s - desktop_y) / tex_h);
		}
	};

	inline Backdrop& backdrop()
	{
		static Backdrop b;
		return b;
	}














	struct Occlusion
	{
		int   n = 0;
		float rect[8][4]{};
		void add(float x0, float y0, float x1, float y1)
		{
			if (n >= 8 || !(x1 > x0) || !(y1 > y0))
				return;



			for (int i = 0; i < n; ++i)
				if (rect[i][0] == x0 && rect[i][1] == y0 && rect[i][2] == x1 && rect[i][3] == y1)
					return;
			rect[n][0] = x0; rect[n][1] = y0; rect[n][2] = x1; rect[n][3] = y1;
			++n;
		}
	};

	inline Occlusion& occlusion()
	{
		static Occlusion o;
		return o;
	}

	inline State& state()
	{
		static State s;
		return s;
	}







	enum class PreviewJoint : int
	{
		Head = 0, Neck, Chest, Pelvis, LegsBase,
		LeftShoulder, LeftHand, RightShoulder, RightHand,
		LeftFoot, RightFoot,
		COUNT
	};

	inline constexpr int kPreviewJointCount = (int)PreviewJoint::COUNT;

	struct PreviewRig
	{
		bool  valid = false;
		char  name[64]{};
		float health = 100.f;
		float distance = 0.f;
		int   rig_type = 0;



		int   state = -1;
		char  tool[48]{};


		long long user_id = 0;
		float joints[kPreviewJointCount][3]{};
	};



	struct PlayerRow
	{
		long long user_id = 0;
		std::uint64_t instance = 0;
		char  name[64]{};
		char  display_name[64]{};
		char  team[32]{};

		char  tool[48]{};

		int   account_age_days = -1;
		float health = 0.f;
		float max_health = 100.f;
		float distance = 0.f;
		int   rig_type = 0;
		bool  teammate = false;


		bool  enemy = false;
		bool  dead = false;
		bool  local = false;
		bool  valid = false;
	};



	enum class PlayerStatus : int
	{
		Dead = 0, Friend, Teammate, Self, Neutral, Enemy
	};

	inline const char* PlayerStatusName(PlayerStatus s)
	{
		switch (s)
		{
		case PlayerStatus::Dead:     return "dead";
		case PlayerStatus::Friend:   return "friend";
		case PlayerStatus::Teammate: return "teammate";
		case PlayerStatus::Self:     return "you";
		case PlayerStatus::Enemy:    return "enemy";
		default:                     return "neutral";
		}
	}

	inline constexpr int kMaxPlayerRows = 128;










	struct SessionInfo
	{
		unsigned long long place_id = 0;
		unsigned long long game_id  = 0;
		char  place_name[64]{};
		char  game_name[64]{};
		float uptime_s = 0.f;
		float ping_ms  = -1.f;
		int   players  = 0;
		int   max_players = 0;
		long long local_user_id = 0;
		char  local_name[64]{};
		char  local_handle[64]{};
		bool  attached = false;
	};












	struct SonarInfo
	{
		bool  active   = false;
		bool  has_ping = false;
		char  name[64]{};
		float distance = 0.f;
		float health   = 0.f;
		float bearing  = 0.f;
		float sweep    = 0.f;
		float glow     = 0.f;
	};

	struct AimSample
	{
		float t = 0.f;
		float raw_yaw = 0.f, raw_pitch = 0.f;
		float step_yaw = 0.f, step_pitch = 0.f;
		float smoothing_x = 1.f, smoothing_y = 1.f;
		float dist = 0.f;
		bool  primary = false;
	};

	struct Hooks
	{
		void (*cfg_refresh)() = nullptr;
		bool (*cfg_save)(const char* name) = nullptr;
		bool (*cfg_load)(const char* name) = nullptr;
		bool (*cfg_delete)(const char* name) = nullptr;

		int  (*theme_count)() = nullptr;
		const char* (*theme_name)(int) = nullptr;
		void (*theme_apply)(int) = nullptr;



		bool (*preview_rig)(PreviewRig& out) = nullptr;



		bool (*preview_avatar)(ImDrawList* dl, long long user_id,
			ImVec2 min, ImVec2 max, ImU32 tint) = nullptr;


		bool (*player_avatar)(ImDrawList* dl, long long user_id,
			ImVec2 min, ImVec2 max, ImU32 tint) = nullptr;





		bool (*player_avatar_rounded)(ImDrawList* dl, long long user_id,
			ImVec2 min, ImVec2 max, ImU32 tint, float rounding) = nullptr;




		bool (*session)(SessionInfo& out) = nullptr;








		bool (*sonar_info)(SonarInfo& out) = nullptr;


		int (*players_snapshot)(void* out, int max) = nullptr;




		int (*player_account_age)(unsigned long long instance) = nullptr;



		bool (*player_spectate)(unsigned long long instance, bool on) = nullptr;


		bool (*player_teleport)(unsigned long long instance) = nullptr;



		void (*player_target)(unsigned long long instance) = nullptr;




		unsigned long long (*aim_target_instance)() = nullptr;







		bool (*player_hud_anchor)(unsigned long long instance, float& cx, float& top) = nullptr;

		bool (*player_aim_anchor)(unsigned long long instance, float& cx, float& cy) = nullptr;



		bool (*chams_pass)() = nullptr;

		bool (*fill_image)(const char* path, bool remove_bg) = nullptr;

		void (*shutdown)() = nullptr;





		void (*explorer_page)(ImVec2 pos, ImVec2 size) = nullptr;




		void (*lua_run)(const char* source) = nullptr;



		bool (*local_player)(char* name, int n, char* handle, int m) = nullptr;
	};

	inline Hooks& hooks()
	{
		static Hooks h;
		return h;
	}
}
