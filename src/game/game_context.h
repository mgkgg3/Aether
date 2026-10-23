#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace game_ctx
{
	enum class GameType : int
	{
		Unknown = 0,
		Blocky,

		OperationOne,
	};

	inline GameType    game_type = GameType::Unknown;
	inline std::uint64_t place_id = 0;

	// True while the client is actually inside a placed experience. The id above is
	// sticky on purpose (a failed read must not clear it), so it is not enough on its
	// own to answer "is the player in a place right now".
	inline bool in_place = false;

	void detect_game();

	inline bool is_blocky()          { return game_type == GameType::Blocky; }
	inline bool game_logic_allowed()    { return is_blocky(); }
	inline bool is_operation_one()    { return game_type == GameType::OperationOne; }

	inline std::uint64_t game_id = 0;
	const char* place_name();

	// An experience the offsets already recognise, ie one the features are known to
	// work in. Used to decide whether the unsupported-place notice applies.
	bool place_supported();

	std::uintptr_t resolve_local_character();

	extern std::mutex                               ko_mtx;
	extern std::unordered_map<std::uintptr_t, bool> ko_status;

	enum class KnockState : int { Unknown = -1, No = 0, Yes = 1 };

	KnockState get_knock(std::uintptr_t player_addr);
}
