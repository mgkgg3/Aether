#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace game_structure
{

	struct character_model_t
	{
		std::string name;
		std::uint64_t address = 0;
		int rig_parts = 0;
	};

	struct info_t
	{
		bool valid = false;

		std::uint64_t players_container = 0;
		std::string   players_name;
		std::string   players_path;

		std::string   character_name = "Character";
		std::vector<character_model_t> character_models;
		int           character_count = 0;

		std::string   rig;
		int           part_count = 0;
		std::vector<std::string> parts;

		std::string   health_source;

		bool custom = false;
	};

	inline std::string players_name_override;
	inline std::string character_name_override;
	inline bool        use_overrides = false;

	inline std::atomic<bool> scan_requested{ false };
	inline std::atomic<bool> auto_apply{ false };

	inline std::mutex mtx;
	inline info_t     g_info;

	info_t detect();

	info_t refresh();

	std::uint64_t players_container_addr();

	std::uint64_t resolve_character(const std::string& player_name, std::uint64_t model_instance);

	std::vector<std::uint64_t> find_all_characters();

	inline std::atomic<bool> using_obfuscated_models{ false };

	std::uint64_t resolve_local_character_camera();
}
