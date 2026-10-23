#include "game_context.h"

#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>
#include <debug/DebugConsole.h>
#include <features/opone/OperationOne.h>

#include <chrono>
#include <thread>

namespace game_ctx
{
	std::mutex                               ko_mtx;
	std::unordered_map<std::uintptr_t, bool> ko_status;

	static std::uint64_t read_place_id()
	{
		if (!game::datamodel.address)
			return 0;
		try
		{
			return memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::PlaceId);
		}
		catch (...) { return 0; }
	}

	const char* place_name()
	{
		switch (place_id)
		{
		case 17625359962ull:  return "Rivals";
		case 72920620366355ull: return "";
		case 6872274481ull:   return "BedWars";
		case 2788229376ull:   return "";
		case 1385324719ull:   return "";
		default:              return nullptr;
		}
	}

	bool place_supported()
	{
		return place_name() != nullptr || is_operation_one() || game_type != GameType::Unknown;
	}

	void detect_game()
	{

		op1::detect();

		std::uint64_t pid_now = read_place_id();
		if (pid_now && pid_now != place_id)
		{
			debug::info("placeId -> %llu", (unsigned long long)pid_now);
		}
		if (pid_now)
			place_id = pid_now;

		// "In a place" needs the id to be readable right now, but a single failed
		// read should not drop the client back to the home screen - so absence has
		// to persist for a moment before the flag follows it.
		static auto s_id_missing_at = std::chrono::steady_clock::time_point{};
		if (pid_now)
		{
			s_id_missing_at = std::chrono::steady_clock::time_point{};
			in_place        = true;
		}
		else if (in_place)
		{
			const auto now = std::chrono::steady_clock::now();
			if (s_id_missing_at.time_since_epoch().count() == 0)
				s_id_missing_at = now;
			if (now - s_id_missing_at >= std::chrono::seconds(1))
			{
				in_place        = false;
				s_id_missing_at = std::chrono::steady_clock::time_point{};
			}
		}

		GameType t = GameType::Unknown;
		switch (place_id)
		{
		case 2788229376:
		case 1385324719:
			t = GameType::Blocky;
			break;
		default:
			t = GameType::Unknown;
			break;
		}

		if (op1::active())
			t = GameType::OperationOne;

		game_id = op1::game_id();

		if (t != game_type)
		{
			game_type = t;
			debug::ok("game context -> %s",
				t == GameType::Blocky      ? "" :
				t == GameType::OperationOne ? "" : "Universal");
		}
	}

	std::uintptr_t resolve_local_character()
	{
		if (!game::local_player.address)
			return 0;

		std::uint64_t mdl = memory->read<std::uint64_t>(
			game::local_player.address + Offsets::Player::ModelInstance);
		if (mdl)
			return mdl;

		static auto s_last_try = std::chrono::steady_clock::time_point{};
		auto now = std::chrono::steady_clock::now();
		if (s_last_try.time_since_epoch().count() != 0 &&
			now - s_last_try < std::chrono::milliseconds(500))
			return game::local_character.address;
		s_last_try = now;

		try
		{
			rbx::player_t     lp(game::local_player.address);
			rbx::instance_t   ws(game::workspace.address);
			if (!ws.address) return game::local_character.address;

			std::string pname = lp.get_name();
			if (pname.empty()) return game::local_character.address;

			for (auto& c : ws.get_children())
			{
				if (!c.address) continue;
				if (c.get_name() == pname)
				{
					debug::log("ctx", "local character via fallback -> '%s' @ 0x%llX",
						pname.c_str(), (unsigned long long)c.address);
					return c.address;
				}
			}
		}
		catch (...) {}

		return game::local_character.address;
	}

	KnockState get_knock(std::uintptr_t player_addr)
	{
		if (!game_logic_allowed())
			return KnockState::No;

		if (!player_addr)
			return KnockState::Unknown;

		std::lock_guard<std::mutex> lk(ko_mtx);
		auto it = ko_status.find(player_addr);
		if (it == ko_status.end())
			return KnockState::Unknown;
		return it->second ? KnockState::Yes : KnockState::No;
	}
}
