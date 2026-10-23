#include "cache.h"
#include "bounded_walk.h"
#include <thread>
#include <unordered_set>
#include <game/game.h>
#include <game/game_context.h>
#include <game/game_structure.h>
#include <features/opone/OperationOne.h>
#include <sdk/offsets.h>
#include <sdk/sdk.h>
#include <memory/memory.h>
#include <debug/DebugConsole.h>

static float read_number_value(rbx::instance_t container, const char* child_name)
{
	if (!container.address) return -1.f;
	rbx::instance_t child = container.find_first_child(child_name);
	if (!child.address) return -1.f;
	float v = memory->read<float>(child.address + Offsets::Misc::Value);
	return v;
}

static float find_armor(rbx::instance_t player_inst, rbx::model_instance_t character)
{
	{
		rbx::instance_t ls = player_inst.find_first_child("leaderstats");
		if (ls.address)
		{
			float v = read_number_value(ls, "Armor");
			if (v >= 0.f) return v;
		}
	}
	{
		float v = read_number_value(character, "ArmorValue");
		if (v >= 0.f) return v;
	}
	{
		float v = read_number_value(character, "Armor");
		if (v >= 0.f) return v;
	}
	return -1.f;
}

static std::string read_display_name(std::uintptr_t player_addr)
{
	try
	{
		std::uint64_t p = memory->read<std::uint64_t>(player_addr + Offsets::Player::DisplayName);
		if (!p || p == ~0ull)
			return "";
		std::string s = memory->read_string(p);
		if (s.empty() || s == "Unknown" || s == "invalid_str")
			return "";
		return s;
	}
	catch (...) { return ""; }
}

static constexpr int k_collect_max_depth   = 4;

static constexpr int k_collect_node_budget = 256;

static void collect_parts_recursive(rbx::instance_t root,
	std::unordered_map<std::string, rbx::part_t>& out)
{
	if (!root.address) return;
	try
	{
		cache::walk_bounded(
			root, k_collect_max_depth, k_collect_node_budget,
			[](const rbx::instance_t& node)
			{
				try { return node.get_children(); }
				catch (...) { return std::vector<rbx::instance_t>{}; }
			},
			[&out](const rbx::instance_t& ch, int )
			{
				if (!ch.address) return;
				std::string cls;
				try { cls = ch.get_class_name(); } catch (...) { return; }
				if (cls.find("Part") == std::string::npos &&
					cls != "SpawnLocation" && cls != "Seat" && cls != "VehicleSeat")
					return;
				std::string nm;
				try { nm = ch.get_name(); } catch (...) { return; }
				if (!nm.empty())
					out[nm] = rbx::part_t(ch.address);
			});
	}
	catch (...) {}
}

static bool has_in_match_marker(std::uint64_t model_address)
{
	if (!model_address) return false;

	if (!op1::marked_model(model_address))
		return false;

	if (op1::active())
		return op1::model_in_workspace(model_address);

	return true;
}

static std::uint64_t resolve_character_unique(const std::string& name, int* out_score = nullptr)
{
	if (out_score) *out_score = -1;
	if (name.empty() || !game::workspace.address)
		return 0;

	std::uint64_t best = 0;
	int best_score = -1;
	int best_count = 0;

	try
	{
		rbx::instance_t ws(game::workspace.address);
		for (auto& ch : ws.get_children())
		{
			if (!ch.address) continue;

			std::string nm;
			try { nm = ch.get_name(); } catch (...) { continue; }
			if (nm != name) continue;

			std::uint64_t humanoid = 0;
			bool rig = false;
			try
			{
				if (ch.get_class_name() == "Model")
				{
					rbx::model_instance_t m(ch.address);
					humanoid = m.find_first_child("Humanoid").address;
					rig = humanoid != 0
						&& m.find_first_child("HumanoidRootPart").address != 0;
				}
			}
			catch (...) { rig = false; }
			if (!rig) continue;

			bool live = false;
			try { live = rbx::humanoid_t(humanoid).get_health() > 0.f; } catch (...) { live = false; }

			const bool marked = has_in_match_marker(ch.address);
			const int  score  = (marked && live) ? 4 : marked ? 2 : live ? 1 : 0;

			if (score > best_score)
			{
				best_score = score;
				best       = ch.address;
				best_count = 1;
			}
			else if (score == best_score)
			{
				++best_count;
			}
		}
	}
	catch (...) { return 0; }

	if (best_score < 0 || best_count != 1)
		return 0;

	if (out_score) *out_score = best_score;
	return best;
}

static bool held_parts_belong_to(
	const std::unordered_map<std::string, rbx::part_t>& held, std::uint64_t character)
{
	if (!character) return false;

	auto root = held.find("HumanoidRootPart");
	if (root == held.end() || !root->second.address) return false;

	try
	{
		return memory->read<std::uint64_t>(root->second.address + Offsets::Instance::Parent) == character;
	}
	catch (...) { return false; }
}

	static void read_humanoid(cache::entity_t& entity)
	{
		if (!entity.humanoid.address) return;
		try
		{
			rbx::humanoid_t h(entity.humanoid.address);

			float hp = 0.f, mx = 0.f;
			if (op1::live_health_enabled() && op1::read_health(entity.humanoid.address, hp, mx))
			{
				entity.health     = hp;
				entity.max_health = mx;
			}
			else
			{
				entity.health     = h.get_health();
				entity.max_health = h.get_max_health();
			}

			entity.state = h.get_state();
		}
		catch (...) {}
	}

static int read_ko_direct(std::uintptr_t player_addr)
{
	try
	{
		rbx::player_t p(player_addr);
		rbx::model_instance_t mdl = p.get_model_instance();
		if (!mdl.address) return -1;

		rbx::instance_t be = mdl.find_first_child("BodyEffects");
		if (!be.address) return -1;

		rbx::instance_t ko = be.find_first_child("K.O");
		if (!ko.address || ko.get_class_name() != "BoolValue") return -1;

		return memory->read<bool>(ko.address + Offsets::Misc::Value) ? 1 : 0;
	}
	catch (...) { return -1; }
}

static inline void set_phase(const char* p)
{
	cache::phase.store(p, std::memory_order_relaxed);
}

static void stall_watchdog()
{
	std::uint64_t seen = cache::tick_count.load(std::memory_order_relaxed);
	int stalled = 0;
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		const std::uint64_t now = cache::tick_count.load(std::memory_order_relaxed);
		if (now != seen)
		{
			seen = now;
			stalled = 0;
			continue;
		}

		++stalled;
		if (stalled == 3 || stalled % 10 == 0)
			debug::log("cache", "STALLED %ds | phase=%s | ticks=%llu", stalled,
				cache::phase.load(std::memory_order_relaxed),
				(unsigned long long)now);
	}
}

bool cache::is_teammate(const entity_t& e)
{
	// The Team instance is what decides this. Colour and name-hash fallbacks used to
	// stand in when it could not be read, but a teamless player still reports a
	// default colour, so those fallbacks answered 'teammate' for whole lobbies.
	// Address only, and an unknown team means 'not a teammate'.
	const std::uint64_t mine = cached_local_team.load(std::memory_order_relaxed);
	if (!mine || !e.team)
		return false;

	return e.team == mine;
}

bool cache::is_dead(const entity_t& e)
{

	if (!e.humanoid.address)
		return false;

	if (e.state == 15)
		return true;

	if (!std::isfinite(e.health) || !std::isfinite(e.max_health))
		return false;

	return e.max_health > 0.f && e.health <= 0.f;
}

void cache::run()
{
	static std::atomic<bool> s_watchdog_started{ false };
	if (!s_watchdog_started.exchange(true))
		std::thread(stall_watchdog).detach();

	while (true)
	{
		try
		{

			set_phase("detect_game");
			game_ctx::detect_game();

			set_phase("structure");
			game_structure::refresh();

			set_phase("local_character");
			game::local_character = { game_ctx::resolve_local_character() };

		std::uint64_t container_addr = game_structure::players_container_addr();
		if (!container_addr)
			container_addr = game::players.address;

		if (!game::local_player.address && container_addr && container_addr != game::players.address)
		{
			try
			{
				std::uint64_t lp = memory->read<std::uint64_t>(container_addr + Offsets::Player::LocalPlayer);
				if (lp)
				{
					rbx::instance_t lp_inst{ lp };
					if (lp_inst.get_class_name() == "Player")
						game::local_player = { lp };
				}
			}
			catch (...) {}
		}

		std::vector<rbx::player_t> players;
		set_phase("players");
		if (container_addr)
		{
			try { players = rbx::instance_t(container_addr).get_children<rbx::player_t>(); }
			catch (...) {}
		}

		if (players.empty() && container_addr != game::players.address && game::players.address)
		{
			try { players = game::players.get_children<rbx::player_t>(); }
			catch (...) {}
		}

		{
			static auto s_pl_at = std::chrono::steady_clock::now() - std::chrono::seconds(6);
			auto pl_now = std::chrono::steady_clock::now();
			if (pl_now - s_pl_at >= std::chrono::seconds(5))
			{
				s_pl_at = pl_now;
				debug::log("cache", "tick | container 0x%llx | players=%zu",
					(unsigned long long)container_addr, players.size());
			}
		}

		bool obfuscated = players.empty()
			&& game_structure::scan_requested.load()
			&& game_structure::auto_apply.load();
		std::vector<std::uint64_t> obf_chars;
		if (obfuscated)
		{
			obf_chars = game_structure::find_all_characters();
			obfuscated = !obf_chars.empty();

			if (obfuscated)
			{
				static bool s_reported_workspace_scan = false;
				if (!s_reported_workspace_scan)
				{
					s_reported_workspace_scan = true;
					debug::log("cache", "workspace-scan path active: %zu candidate rigs (Players service empty)",
						obf_chars.size());
				}
			}
		}

		std::uint64_t obf_local_char = 0;
		if (obfuscated)
		{
			static std::uint64_t s_cached_local = 0;
			static auto s_lc_at = std::chrono::steady_clock::now();
			auto n_lc = std::chrono::steady_clock::now();
			if (n_lc >= s_lc_at)
			{
				s_lc_at = n_lc + std::chrono::seconds(2);
				s_cached_local = game_structure::resolve_local_character_camera();
			}
			obf_local_char = s_cached_local;
			if (!obf_local_char && game::local_character.address)
				obf_local_char = game::local_character.address;
			if (obf_local_char)
			{

				game::local_character = { obf_local_char };
			}
		}

		std::string local_team_name;
		{
			std::uint64_t team = 0;
			if (game::local_player.address)
			{
				try { team = rbx::player_t(game::local_player.address).get_team(); }
				catch (...) { team = 0; }
			}

			if (team)
			{
				try
				{
					local_team_name = rbx::instance_t(team).get_name();
					if (local_team_name.size() >= 32)
						local_team_name.clear();
				}
				catch (...) { local_team_name.clear(); }
			}

			cache::cached_local_team.store(team, std::memory_order_relaxed);
		}

		std::vector<cache::entity_t> temp_cache;
		set_phase("collect_parts");

		int rejected_non_players = 0;

		int         unresolved_nochar = 0;
		int         unresolved_nowalk = 0;
		std::string unresolved_names;

		std::unordered_set<std::uint64_t> claimed_characters;
		int duplicate_characters = 0;

		std::unordered_set<std::uint64_t> claimed_bodies;
		int animated_bodies = 0;

		std::string held_names;

		const char* model_source = nullptr;

		int root_part_fallbacks = 0;

		int stale_model_instances = 0;
		int character_fallbacks = 0;

		int unmarked_characters = 0;

		static std::unordered_map<std::uint64_t, std::uint64_t> s_last_character;

		static std::unordered_map<std::uint64_t, std::uint64_t> s_last_character_ms;
		int held_model_locks = 0;

		static std::unordered_map<std::uint64_t, std::unordered_map<std::string, rbx::part_t>> s_held_parts;

		static std::unordered_map<std::uint64_t, std::unordered_map<std::string, rbx::part_t>> s_held_parts_by_player;
		int held_part_lists = 0;

		if (obfuscated)
		{

			for (std::uint64_t rig_addr : obf_chars)
			{
				rbx::model_instance_t rig{ rig_addr };
				cache::entity_t entity{};
				entity.instance = { rig_addr };
				try { entity.name = rig.get_name(); } catch (...) { entity.name = "?"; }

				bool real_rig = false;
				try
				{
					real_rig = rig.find_first_child("Humanoid").address != 0
						&& rig.find_first_child("HumanoidRootPart").address != 0;
				}
				catch (...) { real_rig = false; }

				if (!real_rig)
				{
					++rejected_non_players;
					continue;
				}

				if (rig.address)
				{
					collect_parts_recursive(rig, entity.parts);
					entity.humanoid  = { rig.find_first_child("Humanoid").address };
					entity.rig_type  = entity.humanoid.get_rig_type();

					if (entity.humanoid.address)
					{
						read_humanoid(entity);
					}
				}
				entity.max_armor = 100.f;
				temp_cache.push_back(entity);
			}
		}
		else
		{
			for (rbx::player_t& player : players)
			{

				std::string player_class;
				try { player_class = player.get_class_name(); } catch (...) { player_class.clear(); }
				if (!player_class.empty() && player_class != "Player")
				{
					++rejected_non_players;
					continue;
				}

				cache::entity_t entity{};

				entity.instance = { player.address };
				entity.name     = player.get_name();
				entity.display_name = read_display_name(player.address);

				try
				{
					const std::int64_t uid =
						memory->read<std::int64_t>(player.address + Offsets::Player::UserId);
					entity.user_id = uid > 0 ? uid : 0;
				}
				catch (...) { entity.user_id = 0; }

				struct character_candidate_t
				{
					std::uint64_t address{ 0 };
					bool          in_match{ false };
				};

				character_candidate_t candidates[4]{};
				int candidate_count = 0;

				auto add_candidate = [&](std::uint64_t address, int lookup_score = -1)
				{
					if (!address || candidate_count >= 4) return;
					for (int i = 0; i < candidate_count; ++i)
						if (candidates[i].address == address) return;

					candidates[candidate_count++] = {
						address,
						(lookup_score < 0) ? has_in_match_marker(address) : (lookup_score >= 2)
					};
				};

				std::uint64_t from_player = 0;
				try { from_player = player.get_model_instance().address; } catch (...) {}
				add_candidate(from_player);

				auto known_character = s_last_character.find(player.address);
				if (known_character != s_last_character.end())
					add_candidate(known_character->second);

				bool fresh_lock = false;
				if (op1::active() && known_character != s_last_character.end() && known_character->second)
				{
					auto held_at = s_last_character_ms.find(player.address);
					fresh_lock = held_at != s_last_character_ms.end() &&
						(GetTickCount64() - held_at->second) <= 1000;
				}

				int name_score = -1;
				if (!fresh_lock)
				{
					const std::uint64_t by_name = resolve_character_unique(entity.name, &name_score);
					add_candidate(by_name, name_score);

					if (!entity.display_name.empty() && entity.display_name != entity.name)
					{
						int display_score = -1;
						const std::uint64_t by_display =
							resolve_character_unique(entity.display_name, &display_score);
						add_candidate(by_display, display_score);
					}
				}

				rbx::model_instance_t model_instance{ 0 };

				for (int pass = 0; pass < 2 && !model_instance.address; ++pass)
				{
					for (int ci = 0; ci < candidate_count && !model_instance.address; ++ci)
					{
						if (pass == 0 && !candidates[ci].in_match) continue;

						const std::uint64_t candidate_address = candidates[ci].address;

						if (!claimed_characters.insert(candidate_address).second)
						{
							++duplicate_characters;
							continue;
						}

						rbx::model_instance_t candidate{ candidate_address };
						for (int attempt = 0; attempt < 3 && entity.parts.empty(); ++attempt)
						{
							if (attempt) std::this_thread::yield();
							try { collect_parts_recursive(candidate, entity.parts); } catch (...) {}
						}

						rbx::humanoid_t humanoid{ 0 };
						try { humanoid = { candidate.find_first_child("Humanoid").address }; } catch (...) {}

						if (entity.parts.empty() && !humanoid.address)
						{

							claimed_characters.erase(candidate_address);
							if (ci == 0) ++stale_model_instances;
							continue;
						}

						model_instance  = candidate;
						entity.humanoid = humanoid;
						if (ci != 0) ++character_fallbacks;
						if (!candidates[ci].in_match) ++unmarked_characters;

						if (!model_source)
							model_source = (ci == 0) ? "ModelInstance" :
							               (ci == 1) ? "sticky lock"   :
							               (ci == 2) ? "by username"   : "by display name";
					}
				}

				const std::uint64_t now_ms = (std::uint64_t)GetTickCount64();
				if (model_instance.address)
				{
					s_last_character[player.address]    = model_instance.address;
					s_last_character_ms[player.address] = now_ms;
				}
				else if (known_character != s_last_character.end())
				{
					const int hold_ms = op1::sticky_ms();
					auto held_at = s_last_character_ms.find(player.address);

					const bool still_held =
						hold_ms > 0 &&
						held_at != s_last_character_ms.end() &&
						now_ms - held_at->second <= (std::uint64_t)hold_ms;

					if (still_held)
						++held_model_locks;
					else
					{
						s_last_character.erase(known_character);
						if (held_at != s_last_character_ms.end())
							s_last_character_ms.erase(held_at);
					}
				}

				if (model_instance.address)
				{

					if (entity.parts.empty())
					{

						auto by_model  = s_held_parts.find(model_instance.address);
						auto by_player = s_held_parts_by_player.find(player.address);

						const std::unordered_map<std::string, rbx::part_t>* held = nullptr;

						if (by_model != s_held_parts.end() && !by_model->second.empty() &&
							held_parts_belong_to(by_model->second, model_instance.address))
							held = &by_model->second;
						else if (by_player != s_held_parts_by_player.end() && !by_player->second.empty() &&
							held_parts_belong_to(by_player->second, model_instance.address))
							held = &by_player->second;

						if (held)
						{
							entity.parts = *held;
							++held_part_lists;
							if (held_names.size() < 160 && !entity.name.empty())
							{
								held_names += entity.name;
								held_names += " ";
							}
						}
						else
						{

							if (by_model  != s_held_parts.end())           s_held_parts.erase(by_model);
							if (by_player != s_held_parts_by_player.end())  s_held_parts_by_player.erase(by_player);
						}
					}
					else
					{
						s_held_parts[model_instance.address]   = entity.parts;
						s_held_parts_by_player[player.address] = entity.parts;
					}

					try { entity.rig_type = entity.humanoid.get_rig_type(); } catch (...) {}

					if (entity.humanoid.address)
					{
						auto have_root = entity.parts.find("HumanoidRootPart");
						if (have_root == entity.parts.end() || !have_root->second.address)
						{
							std::uint64_t root = 0;
							try
							{
								root = memory->read<std::uint64_t>(
									entity.humanoid.address + Offsets::Humanoid::HumanoidRootPart);
							}
							catch (...) { root = 0; }

							bool is_part = false;
							if (root)
							{
								try { is_part = rbx::instance_t(root).get_class_name().find("Part") != std::string::npos; }
								catch (...) { is_part = false; }
							}

							if (is_part)
							{
								entity.parts["HumanoidRootPart"] = rbx::part_t(root);
								++root_part_fallbacks;
							}
						}
					}

					if (op1::bodies_enabled())
					{
						math::vector3 anchor{};
						auto hrp = entity.parts.find("HumanoidRootPart");
						if (hrp != entity.parts.end() && hrp->second.address)
						{
							try
							{
								rbx::primitive_t prim = hrp->second.get_primitive();
								if (prim.address) anchor = prim.get_position();
							}
							catch (...) {}
						}

						if (math::finite(anchor) &&
							op1::attach_viewmodel(entity, anchor, claimed_bodies))
							++animated_bodies;
					}

					entity.tool_name = "";
					try
					{
						for (rbx::instance_t& child : model_instance.get_children<rbx::instance_t>())
						{
							std::string cls = child.get_class_name();
							if (cls == "Tool" || cls == "HopperBin")
							{
								entity.tool_name = child.get_name();
								break;
							}
						}
					}
					catch (...) {}

					entity.armor     = find_armor(entity.instance, model_instance);
				}
				entity.max_armor = 100.f;

				read_humanoid(entity);

			try
			{
				rbx::player_t p(player.address);
				entity.team       = p.get_team();
				entity.team_color = p.get_team_color();
				if (entity.team)
					entity.team_name = rbx::instance_t(entity.team).get_name();
			}
			catch (...) {}

				temp_cache.push_back(entity);
			}
		}

		{
			static auto s_done_at = std::chrono::steady_clock::now() - std::chrono::seconds(6);
			auto done_now = std::chrono::steady_clock::now();
			if (done_now - s_done_at >= std::chrono::seconds(5))
			{
				s_done_at = done_now;
				debug::log("cache", "tick | collected=%zu", temp_cache.size());
			}
		}

		set_phase("diagnostics");
		static auto s_diag_at = std::chrono::steady_clock::now() - std::chrono::seconds(6);
		auto diag_now = std::chrono::steady_clock::now();
		if (diag_now - s_diag_at >= std::chrono::seconds(5))
		{
			s_diag_at = diag_now;
			if (!temp_cache.empty())
			{
				auto& f = temp_cache.front();
				std::uint64_t hrp = 0;
				auto hit = f.parts.find("HumanoidRootPart");
				if (hit != f.parts.end()) hrp = hit->second.address;

				int with_parts = 0, with_hum = 0, with_hrp = 0;

				for (auto& e : temp_cache)
				{
					if (!e.parts.empty()) continue;

					if (e.humanoid.address) ++unresolved_nowalk;
					else                    ++unresolved_nochar;

					if (unresolved_names.size() < 300 && !e.name.empty())
					{
						unresolved_names += e.name;
						unresolved_names += e.humanoid.address ? "(nowalk) " : "(nochar) ";
					}
				}
				int teammates = 0, dead = 0, named_team = 0;
				for (auto& e : temp_cache)
				{
					if (!e.parts.empty()) ++with_parts;
					if (e.humanoid.address) ++with_hum;
					if (e.parts.count("HumanoidRootPart")) ++with_hrp;
					if (e.team) ++named_team;
					if (cache::is_teammate(e)) ++teammates;
					if (cache::is_dead(e)) ++dead;
				}

				debug::log("cache", "%zu ents (players listed %zu, rejected non-players %d, held part lists %d%s, shared characters %d, root from Humanoid %d, stale model ptr %d, char from fallback %d, unmarked rigs %d) | first '%s' p%d h0x%llx st%d | cov parts=%d hum=%d hrp=%d | teams=%d teammates=%d (mine '%s') dead=%d | unresolved %d/%d: %s",
					temp_cache.size(), players.size(), rejected_non_players, held_part_lists,
					held_names.empty() ? "" : (" [" + held_names + "]").c_str(),
					duplicate_characters, root_part_fallbacks,
					stale_model_instances, character_fallbacks, unmarked_characters,
					f.name.c_str(), (int)f.parts.size(),
					(unsigned long long)f.humanoid.address, f.state,
					with_parts, with_hum, with_hrp, named_team, teammates,
					local_team_name.empty() ? "?" : local_team_name.c_str(), dead,
					unresolved_nochar, unresolved_nowalk,
					unresolved_names.empty() ? "-" : unresolved_names.c_str());
			}
			else
			{
				debug::log("cache", "0 entities | players listed %zu | rejected non-players %d | container 0x%llx | players svc 0x%llx",
					players.size(), rejected_non_players,
					(unsigned long long)container_addr, (unsigned long long)game::players.address);
			}
		}

		if (s_held_parts.size() > 64)
			s_held_parts.clear();
		if (s_held_parts_by_player.size() > 64)
			s_held_parts_by_player.clear();

		if (op1::active())
		{
			int unresolved = 0;
			int corpses    = 0;
			for (auto& e : temp_cache)
			{
				if (e.parts.empty()) ++unresolved;

				if (e.humanoid.address && std::isfinite(e.health) && std::isfinite(e.max_health) &&
					e.max_health > 0.f && e.health <= 0.f)
					++corpses;
			}

			op1::publish_cache_stats((int)temp_cache.size(), held_part_lists, unresolved);
			op1::publish_corpses(corpses);
			op1::publish_model_source(model_source ? model_source : "-");
			op1::publish_animated_bodies(animated_bodies);
		}

		set_phase("publish");
		{
			std::lock_guard<std::mutex> lock(mtx);

			if (temp_cache.empty() && !cached_players.empty() && game::datamodel.address)
			{
				static std::uint64_t s_last_kept_log = 0;
				const std::uint64_t now_ms = GetTickCount64();
				if (now_ms - s_last_kept_log > 4000)
				{
					s_last_kept_log = now_ms;
					debug::log("cache", "empty player enumeration - keeping the %zu entities already collected",
						cached_players.size());
				}
			}
			else
			{
				cached_players = std::move(temp_cache);
			}

			std::uint64_t want_local = obfuscated
				? (obf_local_char ? obf_local_char : game::local_character.address)
				: game::local_player.address;

			for (cache::entity_t& ent : cached_players)
			{
				if (ent.instance.address == want_local)
				{
					cached_local_player = ent;
					break;
				}
			}

			cache::cached_local_team_name = local_team_name;
		}

		if (op1::active())
			op1::publish_health(cache::cached_local_player.humanoid.address);

		set_phase("knock");
		if (obfuscated)
		{
			std::lock_guard<std::mutex> lk(game_ctx::ko_mtx);
			game_ctx::ko_status.clear();
		}
		else if (game_ctx::game_logic_allowed())
		{
			std::vector<std::pair<std::uintptr_t, bool>> fresh;
			fresh.reserve(players.size());

			for (auto& p : players)
			{
				int ko = read_ko_direct(p.address);
				if (ko >= 0)
					fresh.push_back({ p.address, ko != 0 });
			}

			{
				std::lock_guard<std::mutex> lk(game_ctx::ko_mtx);
				for (auto& [addr, st] : fresh)
					game_ctx::ko_status[addr] = st;

				for (auto it = game_ctx::ko_status.begin(); it != game_ctx::ko_status.end(); )
				{
					bool alive = false;
					for (auto& p : players)
						if (p.address == it->first) { alive = true; break; }
					if (!alive) it = game_ctx::ko_status.erase(it);
					else        ++it;
				}
			}
		}
		else
		{
			std::lock_guard<std::mutex> lk(game_ctx::ko_mtx);
			game_ctx::ko_status.clear();
		}

		cache::tick_count.fetch_add(1, std::memory_order_relaxed);

		set_phase("sleep");
		std::this_thread::sleep_for(std::chrono::milliseconds(40));
		}
		catch (...)
		{

			static auto s_err_at = std::chrono::steady_clock::now();
			auto err_now = std::chrono::steady_clock::now();
			if (err_now - s_err_at >= std::chrono::seconds(10))
			{
				s_err_at = err_now;
				debug::log("cache", "refresh failed, keeping last snapshot | phase=%s",
					cache::phase.load(std::memory_order_relaxed));
			}
		}
	}
}
