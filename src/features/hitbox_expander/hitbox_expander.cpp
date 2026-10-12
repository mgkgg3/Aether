#define NOMINMAX
#include "hitbox_expander.h"

#include <game/game.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <memory/memory.h>
#include <game/game_context.h>
#include <sdk/offsets.h>
#include <settings.h>

#include <thread>
#include <chrono>
#include <mutex>

static bool is_player_knocked_hbe(const cache::entity_t& player)
{

	return game_ctx::get_knock(player.instance.address) == game_ctx::KnockState::Yes;
}

void rbx::hitbox_expander::run()
{
	while (true)
	{
		try
		{
			if (!game::datamodel.address || !settings::hitbox_expander::enabled)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			if (settings::hitbox_expander::size_x <= 0.f ||
			    settings::hitbox_expander::size_y <= 0.f ||
			    settings::hitbox_expander::size_z <= 0.f)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			std::vector<cache::entity_t> snapshot;
			std::uint64_t local_addr = 0;
			std::string   local_name;
			std::uint64_t local_prim_addr = 0;
			{
				std::lock_guard<std::mutex> lock(cache::mtx);
				snapshot   = cache::cached_players;
				local_addr = cache::cached_local_player.instance.address;
				local_name = cache::cached_local_player.name;
				auto lit = cache::cached_local_player.parts.find("HumanoidRootPart");
				if (lit != cache::cached_local_player.parts.end() && lit->second.address)
					local_prim_addr = memory->read<std::uint64_t>(
						lit->second.address + Offsets::BasePart::Primitive);
			}

			for (auto& entity : snapshot)
			{
				if (!entity.instance.address) continue;
				if ((local_addr && entity.instance.address == local_addr) ||
				    (!local_name.empty() && entity.name == local_name)) continue;
				if (settings::hitbox_expander::knock_check && is_player_knocked_hbe(entity)) continue;

				auto pit = entity.parts.find("HumanoidRootPart");
				if (pit == entity.parts.end() || !pit->second.address) continue;

				std::uint64_t prim_addr = memory->read<std::uint64_t>(
					pit->second.address + Offsets::BasePart::Primitive);
				if (!prim_addr) continue;
				if (local_prim_addr && prim_addr == local_prim_addr) continue;

				math::vector3 new_size{
					settings::hitbox_expander::size_x,
					settings::hitbox_expander::size_y,
					settings::hitbox_expander::size_z
				};

				try {
					memory->write<math::vector3>(prim_addr + Offsets::Primitive::Size, new_size);
					std::uint8_t flags = memory->read<std::uint8_t>(prim_addr + Offsets::PrimitiveFlags::CanCollide);
					flags &= ~0x08;
					memory->write<std::uint8_t>(prim_addr + Offsets::PrimitiveFlags::CanCollide, flags);
				} catch (...) { continue; }
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
	}
}

void rbx::hitbox_expander::initialize() {}
