#define NOMINMAX
#include "noclip.h"

#include <settings.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <keybind/keybind.h>
#include <keybind/live_state.h>
#include <game/game.h>
#include <game/game_context.h>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex>

void movement::noclip::run()
{
	static std::unordered_map<uintptr_t, bool> originalCanCollide;
	static uintptr_t lastCharacterAddress = 0;

	keybind kb("noclip");

	for (;;)
	{
		kb.key  = settings::noclip::keybind;
		kb.type = static_cast<keybind::c_keybind_type>(settings::noclip::activation_mode);
		kb.update();

		bool shouldActivate = settings::noclip::enabled &&
			(settings::noclip::keybind == 0 || kb.enabled);

		live_state::set(live_state::kNoclip, shouldActivate);

		if (!shouldActivate)
		{
			if (!originalCanCollide.empty())
			{
				cache::entity_t local;
				{
					std::lock_guard<std::mutex> lk(cache::mtx);
					local = cache::cached_local_player;
				}

				if (local.instance.address)
				{
					for (auto& [name, part] : local.parts)
					{
						if (!part.address) continue;
						rbx::primitive_t prim = part.get_primitive();
						if (!prim.address) continue;

						auto it = originalCanCollide.find(part.address);
						bool restore = (it != originalCanCollide.end()) ? it->second : true;
						try { prim.set_can_collide(restore); } catch (...) {}
					}
				}
				originalCanCollide.clear();
			}

			lastCharacterAddress = 0;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		cache::entity_t local;
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			local = cache::cached_local_player;
		}

		if (local.parts.empty())
		{
			try
			{
				std::uint64_t chr = game_ctx::resolve_local_character();
				if (chr)
				{
					rbx::model_instance_t mdl{ chr };
					for (auto& ch : mdl.get_children())
					{
						if (!ch.address) continue;
						std::string cls;
						try { cls = ch.get_class_name(); } catch (...) { continue; }
						if (cls.find("Part") == std::string::npos &&
							cls != "SpawnLocation" && cls != "Seat")
							continue;
						rbx::part_t pt{ ch.address };
						local.parts[ch.get_name()] = pt;
					}
				}
			}
			catch (...) {}
		}

		if (local.parts.empty())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (local.instance.address != lastCharacterAddress)
		{
			lastCharacterAddress = local.instance.address;
			originalCanCollide.clear();
		}

		for (auto& [name, part] : local.parts)
		{
			if (!part.address) continue;
			rbx::primitive_t prim = part.get_primitive();
			if (!prim.address) continue;

			try
			{
				if (originalCanCollide.find(part.address) == originalCanCollide.end())
					originalCanCollide[part.address] = prim.get_can_collide();

				prim.set_can_collide(false);
			}
			catch (...) {}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
