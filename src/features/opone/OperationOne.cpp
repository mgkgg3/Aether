#define NOMINMAX
#include "OperationOne.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <windows.h>

#include <game/game.h>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <settings.h>
#include <debug/DebugConsole.h>
#include <features/worldgeo/WorldGeo.h>

namespace op1
{
	namespace
	{

		constexpr std::uint64_t k_place_operation_one = 72920620366355ull;
		constexpr std::uint64_t k_game_operation_one  = 8307114974ull;
		constexpr std::uint64_t k_place_rivals        = 17625359962ull;
		constexpr std::uint64_t k_place_bedwars       = 6872274481ull;

		constexpr std::uint64_t k_layout_retry_ms = 1000;
		constexpr std::uint64_t k_layout_forget   = 2;
		constexpr int           k_workspace_scan  = 400;

		std::atomic<bool>          s_active{ false };
		std::atomic<std::uint64_t> s_place{ 0 };
		std::atomic<std::uint64_t> s_game{ 0 };
		std::atomic<const char*>   s_source{ nullptr };

		bool s_forced = false;
		bool s_forced_checked = false;

		std::atomic<const char*> s_model_source{ "-" };
		std::atomic<const char*> s_matrix_source{ "-" };
		std::atomic<int>   s_entities{ 0 };
		std::atomic<int>   s_held{ 0 };
		std::atomic<int>   s_unresolved{ 0 };
		std::atomic<float> s_health{ 0.f };
		std::atomic<float> s_max_health{ 0.f };
		std::atomic<bool>  s_health_valid{ false };
		std::atomic<float> s_alt_health{ 0.f };
		std::atomic<float> s_alt_max_health{ 0.f };
		std::atomic<bool>  s_alt_health_valid{ false };
		std::atomic<int>   s_bodies{ 0 };
		std::atomic<int>   s_viewmodels{ 0 };

		std::atomic<int>   s_body_parts{ 0 };
		std::atomic<int>   s_corpses{ 0 };

		bool ci_equal(const std::string& a, const char* b)
		{
			if (!b) return false;
			std::size_t i = 0;
			for (; i < a.size() && b[i]; ++i)
			{
				const unsigned char ca = (unsigned char)a[i];
				const unsigned char cb = (unsigned char)b[i];
				if (std::tolower(ca) != std::tolower(cb))
					return false;
			}
			return i == a.size() && b[i] == '\0';
		}

		rbx::instance_t ci_find_child(const rbx::instance_t& parent, const char* name)
		{
			if (!parent.address || !name) return {};
			try
			{
				const rbx::instance_t exact = parent.find_first_child(name);
				if (exact.address) return exact;

				for (rbx::instance_t& ch : parent.get_children())
				{
					if (!ch.address) continue;
					std::string nm;
					try { nm = ch.get_name(); } catch (...) { continue; }
					if (ci_equal(nm, name)) return ch;
				}
			}
			catch (...) {}
			return {};
		}

		bool is_hull_substr(const std::string& lowered)
		{
			static const char* const k_needles[] = {
				"collision", "hitbox", "hull", "capsule", "nocol"
			};
			for (const char* n : k_needles)
				if (lowered.find(n) != std::string::npos)
					return true;
			return false;
		}

		std::string narrow(const std::wstring& w)
		{
			if (w.empty()) return {};
			const int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
				nullptr, 0, nullptr, nullptr);
			if (n <= 0) return {};
			std::string out((std::size_t)n, '\0');
			::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), n,
				nullptr, nullptr);
			return out;
		}

		std::string lower_copy(const std::string& s)
		{
			std::string out = s;
			for (char& c : out) c = (char)std::tolower((unsigned char)c);
			return out;
		}

		void read_launch_args()
		{
			if (s_forced_checked) return;
			s_forced_checked = true;

			const std::string cmd = lower_copy(narrow(::GetCommandLineW()));
			if (cmd.empty()) return;

			auto value_of = [&](const char* flag) -> std::string
			{
				const std::size_t at = cmd.find(flag);
				if (at == std::string::npos) return {};
				std::size_t i = at + std::strlen(flag);
				while (i < cmd.size() && (cmd[i] == ' ' || cmd[i] == '=' || cmd[i] == '\t'))
					++i;
				std::size_t end = i;
				while (end < cmd.size() && cmd[end] != ' ' && cmd[end] != '\t')
					++end;
				return cmd.substr(i, end - i);
			};

			const std::string game_arg = value_of("--game");
			if (game_arg == "operation-one" || game_arg == "operationone" || game_arg == "op1")
				s_forced = true;

			const std::string place_arg = value_of("--place");
			if (!place_arg.empty())
			{
				const std::uint64_t pid = (std::uint64_t)std::strtoull(place_arg.c_str(), nullptr, 10);
				if (pid == k_place_operation_one) s_forced = true;
				else if (pid == k_place_rivals || pid == k_place_bedwars) s_forced = false;
			}
		}

		bool layout_says_operation_one()
		{
			if (!game::workspace.address) return false;
			try
			{
				rbx::instance_t ws(game::workspace.address);
				int scanned = 0;
				for (auto& ch : ws.get_children())
				{
					if (++scanned > k_workspace_scan) break;
					if (!ch.address) continue;

					std::string cls;
					try { cls = ch.get_class_name(); } catch (...) { continue; }
					if (cls != "Model") continue;

					rbx::instance_t model(ch.address);
					rbx::instance_t cfg = model.find_first_child("Configuration");
					if (!cfg.address) continue;
					if (cfg.find_first_child("Electronic").address)
						return true;
				}
			}
			catch (...) {}
			return false;
		}
	}

	void detect()
	{
		read_launch_args();

		const bool forced = s_forced || settings::op1::force;

		std::uint64_t place = 0, gameid = 0;
		if (game::datamodel.address)
		{
			try { place = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::PlaceId); }
			catch (...) { place = 0; }
			try { gameid = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::GameId); }
			catch (...) { gameid = 0; }
		}

		const std::uint64_t place_before = s_place.load(std::memory_order_relaxed);
		const std::uint64_t place_now    = place ? place : place_before;
		(void)place_before;
		if (place) s_place.store(place, std::memory_order_relaxed);
		if (gameid) s_game.store(gameid, std::memory_order_relaxed);

		const bool was_active = s_active.load(std::memory_order_relaxed);
		const char* was_source = s_source.load(std::memory_order_relaxed);

		bool active_now = was_active;
		const char* source = was_source;

		if (place_now == k_place_rivals || place_now == k_place_bedwars)
		{
			active_now = false;
			source     = nullptr;
		}
		else if (place_now == k_place_operation_one || gameid == k_game_operation_one)
		{
			active_now = true;
			source     = "ids";
		}
		else
		{

			static std::uint64_t s_last_layout_ms = 0;
			static int           s_layout_no      = 0;

			const std::uint64_t now = ::GetTickCount64();
			if (now - s_last_layout_ms >= k_layout_retry_ms)
			{
				s_last_layout_ms = now;
				if (layout_says_operation_one())
				{
					s_layout_no = 0;
					active_now  = true;
					source      = "layout";
				}
				else
				{
					++s_layout_no;

					const bool place_changed =
						place != 0 && place_before != 0 && place != place_before;
					const bool hold_ids = !place_changed && source &&
						std::strcmp(source, "ids") == 0;

					if (s_layout_no >= k_layout_forget && !hold_ids)
					{
						active_now = false;
						source     = nullptr;
					}
				}
			}

			if (!active_now && forced)
			{
				active_now = true;
				source     = settings::op1::force ? "forced" : "launch arg";
			}
		}

		s_source.store(source, std::memory_order_relaxed);
		s_active.store(active_now && settings::op1::enable, std::memory_order_relaxed);

		if (active_now != was_active)
		{
			const std::uint64_t p = s_place.load(std::memory_order_relaxed);
			debug::ok(": %s (place %llu, via %s)",
				active_now ? "detected - Op One path on" : "off - standard path",
				(unsigned long long)p, source ? source : "n/a");
		}
	}

	bool active()
	{
		return s_active.load(std::memory_order_relaxed);
	}

	std::uint64_t place_id() { return s_place.load(std::memory_order_relaxed); }
	std::uint64_t game_id()  { return s_game.load(std::memory_order_relaxed); }

	const char* place_name()
	{
		const std::uint64_t p = s_place.load(std::memory_order_relaxed);
		if (p == k_place_operation_one) return "";
		if (p == k_place_rivals)        return "RIVALS";
		if (p == k_place_bedwars)       return "BEDWARS";
		return nullptr;
	}

	const char* detection_source() { return s_source.load(std::memory_order_relaxed); }

	bool aliases_enabled()         { return active() && settings::op1::part_aliases; }
	bool bodies_enabled()          { return active() && settings::op1::animated_body; }
	bool collision_boxes_enabled() { return active() && settings::op1::collision_boxes; }
	bool engine_matrix_enabled()   { return active() && settings::op1::engine_matrix; }
	bool multipoint_los_enabled()  { return active() && settings::op1::multipoint_los; }
	bool live_health_enabled()     { return active() && settings::op1::live_health; }
	bool no_smoothing_enabled()    { return active() && settings::op1::no_smoothing; }
	bool head_mesh_fallback()      { return active() && settings::op1::animated_body; }

	bool corpse(const cache::entity_t& entity)
	{
		if (!active() || !settings::op1::live_health)
			return false;
		if (!entity.humanoid.address)
			return false;

		const bool zero = std::isfinite(entity.health) && std::isfinite(entity.max_health) &&
			entity.max_health > 0.f && entity.health <= 0.f;

		const bool alive = std::isfinite(entity.health) && std::isfinite(entity.max_health) &&
			entity.max_health > 0.f && entity.health > 0.f;

		struct dead_state_t
		{
			std::uint64_t humanoid{ 0 };
			std::uint64_t since{ 0 };
			bool          zero{ false };
			bool          dead{ false };
		};

		static thread_local std::unordered_map<std::uint64_t, dead_state_t> s_dead;
		if (s_dead.size() > 256)
			s_dead.clear();

		constexpr std::uint64_t k_corpse_dwell_ms = 250;

		const std::uint64_t now = ::GetTickCount64();
		dead_state_t& st = s_dead[entity.instance.address];

		if (st.humanoid != entity.humanoid.address)
		{

			st = dead_state_t{};
			st.humanoid = entity.humanoid.address;
		}

		if (alive)
		{

			st.since = now;
			st.zero  = false;
			st.dead  = false;
			return false;
		}

		if (!zero)
			return st.dead;

		if (st.since == 0 || !st.zero)
		{
			st.since = now;
			st.zero  = true;
		}

		if (now - st.since >= k_corpse_dwell_ms)
			st.dead = true;

		return st.dead;
	}

	int sticky_ms()
	{
		if (!active()) return 0;
		return settings::op1::sticky_ms < 0 ? 0 : settings::op1::sticky_ms;
	}

	namespace
	{

		const std::vector<const char*>& k_body_names()
		{
			static const std::vector<const char*> k_names{
				"head", "torso",
				"arm1", "arm2", "shoulder1", "shoulder2",
				"leg1", "leg2", "hip1", "hip2"
			};
			return k_names;
		}

		struct viewmodel_entry_t
		{
			std::uint64_t model{ 0 };
			math::vector3 centre{};
			bool          drawn{ true };
			int           parts{ 0 };
		};

		int count_body_children(const rbx::instance_t& model,
			rbx::instance_t& anchor, bool& head_avail)
		{
			anchor      = {};
			head_avail  = false;
			int         n = 0;
			rbx::instance_t first{};
			rbx::instance_t torso{};

			try
			{
				for (rbx::instance_t& ch : model.get_children())
				{
					if (!ch.address) continue;

					std::string nm;
					try { nm = ch.get_name(); } catch (...) { continue; }
					if (nm.empty() || nm.size() > 24) continue;

					bool is_body = false;
					for (const char* b : k_body_names())
						if (ci_equal(nm, b)) { is_body = true; break; }
					if (!is_body) continue;

					++n;
					if (!first.address) first = ch;
					if (!anchor.address && ci_equal(nm, "head"))
					{
						anchor     = ch;
						head_avail = true;
					}
					if (!torso.address && ci_equal(nm, "torso")) torso = ch;
				}
			}
			catch (...) {}

			if (!anchor.address) anchor = torso.address ? torso : first;
			return n;
		}

		rbx::instance_t viewmodel_folder()
		{
			if (!game::workspace.address) return {};

			rbx::instance_t ws(game::workspace.address);
			if (rbx::instance_t folder = ci_find_child(ws, "Viewmodels"); folder.address)
				return folder;

			try
			{
				int scanned = 0;
				for (rbx::instance_t& ch : ws.get_children())
				{
					if (++scanned > 64) break;
					if (!ch.address) continue;

					std::string cls;
					try { cls = ch.get_class_name(); } catch (...) { continue; }
					if (cls != "Model" && cls != "Folder") continue;

					int bodies = 0;
					for (rbx::instance_t& sub : ch.get_children())
					{
						if (!sub.address) continue;
						std::string sub_cls;
						try { sub_cls = sub.get_class_name(); } catch (...) { continue; }
						if (sub_cls != "Model") continue;

						rbx::instance_t anchor{};
						bool head_avail = false;
						if (count_body_children(sub, anchor, head_avail) >= 3 && ++bodies >= 2)
							return ch;
					}
				}
			}
			catch (...) {}
			return {};
		}

		const std::vector<viewmodel_entry_t>& viewmodel_list()
		{
			static std::vector<viewmodel_entry_t> s_list;
			static std::uint64_t s_at = 0;

			const std::uint64_t now = ::GetTickCount64();
			if (!s_list.empty() && now - s_at < 500) return s_list;
			s_at = now;

			std::vector<viewmodel_entry_t> fresh;
			try
			{
				const rbx::instance_t folder = viewmodel_folder();
				if (folder.address)
				{
					for (rbx::instance_t& ch : folder.get_children())
					{
						if (!ch.address || fresh.size() >= 32) continue;

						std::string cls;
						try { cls = ch.get_class_name(); } catch (...) { continue; }
						if (cls != "Model") continue;

						rbx::instance_t anchor{};
						bool head_avail = false;
						const int parts = count_body_children(ch, anchor, head_avail);

						if (parts < 2 || !anchor.address) continue;

						viewmodel_entry_t e{};
						e.model = ch.address;
						e.parts = parts;

						try
						{
							rbx::primitive_t prim = rbx::part_t(anchor.address).get_primitive();
							if (!prim.address) continue;
							e.centre = prim.get_position();
							if (!math::finite(e.centre)) continue;
						}
						catch (...) { continue; }

						try
						{
							const float t = memory->read<float>(anchor.address + Offsets::BasePart::Transparency);
							if (std::isfinite(t) && t >= -0.01f && t <= 1.01f)
								e.drawn = t < 0.99f;
						}
						catch (...) {}

						fresh.push_back(e);
					}
				}
			}
			catch (...) {}

			if (!fresh.empty())
				s_list.swap(fresh);

			s_viewmodels.store((int)s_list.size(), std::memory_order_relaxed);
			return s_list;
		}
	}

	const std::vector<const char*>& body_parts() { return k_body_names(); }

	namespace
	{

		constexpr int k_min_body_parts = 3;

		constexpr std::uint64_t k_body_hold_ms = 1500;

		int s_attach_parts_max = 0;
	}

	int body_part_count(const cache::entity_t& entity)
	{
		if (!bodies_enabled()) return 0;

		int n = 0;
		for (const char* name : k_body_names())
			if (find_part(entity, name) != nullptr)
				++n;
		return n;
	}

	bool parts_contain_body(const std::unordered_map<std::string, rbx::part_t>& parts)
	{
		if (!bodies_enabled() || parts.empty()) return false;

		int n = 0;
		for (const auto& kv : parts)
		{
			if (!kv.second.address) continue;
			for (const char* name : k_body_names())
				if (ci_equal(kv.first, name) && ++n >= k_min_body_parts)
					return true;
		}
		return false;
	}

	bool has_animated_body(const cache::entity_t& entity)
	{
		const int n = body_part_count(entity);
		if (n >= 4) return true;

		if (n < k_min_body_parts) return false;
		return find_part(entity, "head") != nullptr || find_part(entity, "torso") != nullptr;
	}

	bool attach_viewmodel(cache::entity_t& entity, const math::vector3& anchor,
		std::unordered_set<std::uint64_t>& claimed)
	{
		if (!bodies_enabled() || !math::finite(anchor)) return false;

		const std::vector<viewmodel_entry_t>& list = viewmodel_list();
		if (list.empty()) return false;

		constexpr float k_max_dist = 12.f;

		static std::unordered_map<std::uint64_t, std::uint64_t> s_last_body;
		static std::unordered_map<std::uint64_t, std::uint64_t> s_last_body_ms;

		static std::unordered_map<std::uint64_t,
			std::unordered_map<std::string, rbx::part_t>> s_parts_by_player;
		static std::unordered_map<std::uint64_t, std::uint64_t> s_parts_by_player_ms;
		if (s_parts_by_player.size() > 64)
		{
			s_parts_by_player.clear();
			s_parts_by_player_ms.clear();
		}

		const std::uint64_t now_ms = ::GetTickCount64();

		auto reuse_remembered = [&]() -> int
		{
			auto ms = s_parts_by_player_ms.find(entity.instance.address);
			if (ms == s_parts_by_player_ms.end() || now_ms - ms->second > k_body_hold_ms)
				return 0;

			auto remembered = s_parts_by_player.find(entity.instance.address);
			if (remembered == s_parts_by_player.end() || remembered->second.empty())
				return 0;

			int reused = 0;
			for (const auto& kv : remembered->second)
			{
				if (!kv.second.address) continue;
				entity.parts[kv.first] = kv.second;
				++reused;
			}
			return reused;
		};

		auto held = s_last_body.find(entity.instance.address);
		const std::uint64_t held_model =
			(held != s_last_body.end() && s_last_body_ms.count(entity.instance.address) &&
				now_ms - s_last_body_ms[entity.instance.address] <= 2000) ? held->second : 0;

		struct candidate_t { const viewmodel_entry_t* v; float score; };
		std::vector<candidate_t> ranked;
		ranked.reserve(list.size());

		for (const viewmodel_entry_t& v : list)
		{
			if (!v.model) continue;

			if (claimed.count(v.model)) continue;
			if (v.parts < k_min_body_parts) continue;

			const float d = (v.centre - anchor).length();
			if (d > k_max_dist) continue;

			const float score = (v.drawn ? 0.f : 60.f) + d
				- (float)std::min(v.parts, 10) * 0.75f
				+ (v.model == held_model ? -25.f : 0.f);

			ranked.push_back({ &v, score });
		}

		std::sort(ranked.begin(), ranked.end(),
			[](const candidate_t& a, const candidate_t& b) { return a.score < b.score; });

		const int attempts = std::min((int)ranked.size(), 3);

		for (int i = 0; i < attempts; ++i)
		{
			const std::uint64_t model_addr = ranked[i].v->model;

			rbx::model_instance_t model(model_addr);
			int copied = 0;
			try
			{
				for (rbx::instance_t& ch : model.get_children<rbx::instance_t>())
				{
					if (!ch.address) continue;

					std::string name;
					try { name = ch.get_name(); } catch (...) { continue; }
					if (name.empty() || name.size() > 16) continue;

					bool wanted = false;
					for (const char* n : k_body_names())
						if (ci_equal(name, n)) { wanted = true; name = n; break; }
					if (!wanted) continue;

					entity.parts[name] = rbx::part_t(ch.address);
					++copied;
				}
			}
			catch (...) {}

			if (copied < k_min_body_parts)
			{

				if (s_last_body.count(entity.instance.address) &&
					s_last_body[entity.instance.address] == model_addr)
				{
					s_last_body.erase(entity.instance.address);
					s_last_body_ms.erase(entity.instance.address);
				}
				continue;
			}

			claimed.insert(model_addr);
			s_last_body[entity.instance.address]    = model_addr;
			s_last_body_ms[entity.instance.address] = now_ms;

			{
				std::unordered_map<std::string, rbx::part_t> copy;
				for (const char* n : k_body_names())
				{
					auto it = entity.parts.find(n);
					if (it != entity.parts.end() && it->second.address)
						copy[n] = it->second;
				}
				if (!copy.empty())
				{
					s_parts_by_player_ms[entity.instance.address] = now_ms;
					s_parts_by_player[entity.instance.address] = std::move(copy);
				}
			}

			if (copied > s_attach_parts_max) s_attach_parts_max = copied;
			return true;
		}

		return reuse_remembered() > 0;
	}

	bool is_collider_body(const std::string& name)
	{
		if (name.empty()) return false;
		if (ci_equal(name, "HumanoidRootPart") || ci_equal(name, "legs"))
			return true;
		return is_hull_name(name);
	}

	bool marked_model(std::uint64_t model)
	{
		if (!model) return false;
		try
		{
			rbx::model_instance_t m(model);

			if (m.find_first_child("Electronic").address)
				return true;

			rbx::instance_t cfg = m.find_first_child("Configuration");
			if (cfg.address && cfg.find_first_child("Electronic").address)
				return true;
		}
		catch (...) {}
		return false;
	}

	bool model_in_workspace(std::uint64_t model)
	{
		if (!model || !game::workspace.address) return false;
		try
		{
			return memory->read<std::uint64_t>(model + Offsets::Instance::Parent)
				== game::workspace.address;
		}
		catch (...) { return false; }
	}

	const std::vector<const char*>& bone_aliases(bone b)
	{

		static const std::vector<const char*> k_head{ "Head", "head" };
		static const std::vector<const char*> k_torso{ "UpperTorso", "LowerTorso", "Torso", "torso" };
		static const std::vector<const char*> k_larm{ "Left Arm", "LeftUpperArm", "LeftLowerArm", "LeftHand", "arm1" };
		static const std::vector<const char*> k_rarm{ "Right Arm", "RightUpperArm", "RightLowerArm", "RightHand", "arm2" };
		static const std::vector<const char*> k_lleg{ "Left Leg", "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "leg1" };
		static const std::vector<const char*> k_rleg{ "Right Leg", "RightUpperLeg", "RightLowerLeg", "RightFoot", "leg2" };
		static const std::vector<const char*> k_root{ "HumanoidRootPart", "root" };
		static const std::vector<const char*> k_none{};

		switch (b)
		{
		case bone::head:      return k_head;
		case bone::torso:     return k_torso;
		case bone::left_arm:  return k_larm;
		case bone::right_arm: return k_rarm;
		case bone::left_leg:  return k_lleg;
		case bone::right_leg: return k_rleg;
		case bone::root:      return k_root;
		default:              return k_none;
		}
	}

	const rbx::part_t* find_part(const cache::entity_t& entity, const char* name,
		std::string* found)
	{
		if (!name || entity.parts.empty())
			return nullptr;

		auto exact = entity.parts.find(name);
		if (exact != entity.parts.end() && exact->second.address)
		{
			if (found) *found = exact->first;
			return &exact->second;
		}

		if (!aliases_enabled())
			return nullptr;

		for (auto& kv : entity.parts)
		{
			if (!kv.second.address) continue;
			if (!ci_equal(kv.first, name)) continue;
			if (found) *found = kv.first;
			return &kv.second;
		}
		return nullptr;
	}

	const rbx::part_t* find_bone(const cache::entity_t& entity, bone b,
		std::string* found_name)
	{
		for (const char* name : bone_aliases(b))
			if (const rbx::part_t* p = find_part(entity, name, found_name))
				return p;
		return nullptr;
	}

	bool is_hull_name(const std::string& name)
	{
		if (name.empty()) return false;
		if (name == "HumanoidRootPart" || name == "humanoidrootpart") return false;
		return is_hull_substr(lower_copy(name));
	}

	bool is_swinging_limb(const std::string& name)
	{
		if (name.empty()) return false;
		static const char* const k_limbs[] = { "arm1", "arm2", "shoulder1", "shoulder2" };
		for (const char* n : k_limbs)
			if (ci_equal(name, n))
				return true;
		return false;
	}

	const std::vector<const char*>& box_primary()
	{
		static const std::vector<const char*> k_primary{
			"HumanoidRootPart", "collision", "collision2", "collision3", "legs"
		};
		return k_primary;
	}

	const std::vector<const char*>& box_fallback()
	{
		static const std::vector<const char*> k_fallback{
			"head", "arm1", "arm2", "leg1", "leg2"
		};
		return k_fallback;
	}

	const std::vector<const char*>& los_points()
	{
		static const std::vector<const char*> k_points{
			"head", "HumanoidRootPart", "arm1", "arm2", "leg1", "leg2"
		};
		return k_points;
	}

	bool part_geometry(const rbx::part_t& part, math::vector3& pos,
		math::vector3* size, math::matrix3* rot)
	{
		if (!part.address) return false;
		try
		{
			rbx::primitive_t prim = part.get_primitive();
			if (!prim.address) return false;

			const math::vector3 p = prim.get_position();
			if (!math::finite(p)) return false;
			if (std::fabs(p.x) > 1.0e6f || std::fabs(p.z) > 1.0e6f) return false;
			if (p.y < -5000.f || p.y > 30000.f) return false;
			pos = p;

			if (size)
			{
				const math::vector3 s = prim.get_size();
				if (!math::finite(s) || s.x <= 0.f || s.y <= 0.f || s.z <= 0.f) return false;
				if (s.x > 2048.f || s.y > 2048.f || s.z > 2048.f) return false;
				*size = s;
			}
			if (rot)
			{
				const math::matrix3 r = prim.get_rotation();
				if (!math::finite(r.right()) || !math::finite(r.up()) || !math::finite(r.forward()))
					return false;
				*rot = r;
			}
			return true;
		}
		catch (...) { return false; }
	}

	bool part_position(const rbx::part_t& part, math::vector3& out)
	{
		return part_geometry(part, out, nullptr, nullptr);
	}

	namespace
	{

		struct live_pos_t
		{
			math::vector3 pos{};
			std::uint64_t ms{ 0 };
			int           rejected{ 0 };
			bool          ok{ false };
		};

		constexpr float         k_live_max_jump   = 4.f;
		constexpr float         k_live_deadband   = 0.02f;
		constexpr int           k_live_max_reject = 4;
		constexpr std::uint64_t k_live_hold_ms    = 150;

		bool live_point(const rbx::part_t& part, math::vector3& out)
		{
			if (!part.address) return false;

			static std::unordered_map<std::uintptr_t, live_pos_t> s_live;
			if (s_live.size() > 512) s_live.clear();

			live_pos_t& e = s_live[(std::uintptr_t)part.address];
			const std::uint64_t now = ::GetTickCount64();

			math::vector3 fresh{};
			if (part_position(part, fresh))
			{
				if (!e.ok)
				{
					e.pos      = fresh;
					e.ok       = true;
					e.rejected = 0;
					e.ms       = now;
				}
				else
				{
					const float step = (fresh - e.pos).length();

					if (step > k_live_max_jump && e.rejected < k_live_max_reject)
					{
						++e.rejected;
					}
					else
					{
						e.rejected = 0;
						if (step > k_live_deadband) e.pos = fresh;
						e.ms = now;
					}
				}

				out = e.pos;
				return math::finite(out);
			}

			if (e.ok && now - e.ms <= k_live_hold_ms)
			{
				out = e.pos;
				return math::finite(out);
			}
			return false;
		}
	}

	bool live_position(const rbx::part_t& part, math::vector3& out)
	{
		return live_point(part, out);
	}

	namespace
	{

		bool collision_cluster(const cache::entity_t& entity, math::vector3& centre,
			float& top)
		{
			static const char* const k_names[] = { "collision", "collision2", "collision3" };

			math::vector3 sum{};
			int   n   = 0;
			top       = -FLT_MAX;

			for (const char* nm : k_names)
			{
				const rbx::part_t* p = find_part(entity, nm);
				if (!p) continue;

				math::vector3 pos{}, size{};
				math::matrix3 rot = math::matrix3::identity();
				if (!part_geometry(*p, pos, &size, &rot)) continue;

				sum += pos;
				++n;

				const float half = cache::part_half_height(size, rot);
				if (pos.y + half > top) top = pos.y + half;
			}

			if (n == 0) return false;
			centre = { sum.x / (float)n, sum.y / (float)n, sum.z / (float)n };
			return math::finite(centre);
		}

		int body_extent(const cache::entity_t& entity, float& top, float& bottom,
			math::vector3& centre)
		{
			top    = -FLT_MAX;
			bottom =  FLT_MAX;
			if (!bodies_enabled()) return 0;

			math::vector3 sum{};
			int n = 0;

			for (const char* name : k_body_names())
			{
				const rbx::part_t* p = find_part(entity, name);
				if (!p) continue;

				math::vector3 pos{}, size{};
				math::matrix3 rot = math::matrix3::identity();
				if (!part_geometry(*p, pos, &size, &rot)) continue;

				const float half = cache::part_half_height(size, rot);
				if (pos.y + half > top)    top    = pos.y + half;
				if (pos.y - half < bottom) bottom = pos.y - half;
				sum += pos;
				++n;
			}

			if (n > 0)
				centre = { sum.x / (float)n, (top + bottom) * 0.5f, sum.z / (float)n };
			return n;
		}
	}

	bool head_point(const cache::entity_t& entity, math::vector3& out)
	{

		if (const rbx::part_t* head = find_bone(entity, bone::head))
			if (live_point(*head, out))
				return true;

		{
			float top = 0.f, bottom = 0.f;
			math::vector3 centre{};
			if (body_extent(entity, top, bottom, centre) >= k_min_body_parts)
			{
				out = { centre.x, top - 0.15f, centre.z };
				if (math::finite(out)) return true;
			}
		}

		math::vector3 centre{};
		float top = 0.f;
		if (collision_cluster(entity, centre, top))
		{
			out = { centre.x, top + 0.25f, centre.z };
			return math::finite(out);
		}

		if (const rbx::part_t* root = find_bone(entity, bone::root))
		{
			math::vector3 r{};
			if (part_position(*root, r))
			{
				out = { r.x, r.y + 2.5f, r.z };
				return math::finite(out);
			}
		}
		return false;
	}

	bool chest_point(const cache::entity_t& entity, math::vector3& out)
	{

		if (const rbx::part_t* torso = find_bone(entity, bone::torso))
			if (live_point(*torso, out))
				return true;

		{
			float top = 0.f, bottom = 0.f;
			math::vector3 centre{};
			if (body_extent(entity, top, bottom, centre) >= k_min_body_parts)
			{
				out = { centre.x, centre.y, centre.z };
				if (math::finite(out)) return true;
			}
		}

		math::vector3 centre{};
		float top = 0.f;
		if (collision_cluster(entity, centre, top))
		{
			out = { centre.x, centre.y - 0.45f, centre.z };
			return math::finite(out);
		}

		if (const rbx::part_t* root = find_bone(entity, bone::root))
		{
			math::vector3 r{};
			if (part_position(*root, r))
			{
				out = { r.x, r.y + 1.25f, r.z };
				return math::finite(out);
			}
		}
		return false;
	}

	bool aim_point(const cache::entity_t& entity, bone b, math::vector3& out)
	{
		if (!active())
			return false;

		switch (b)
		{
		case bone::head:
			return head_point(entity, out);
		case bone::torso:
			return chest_point(entity, out);
		default:
			break;
		}

		if (const rbx::part_t* p = find_bone(entity, b))
			return live_point(*p, out);
		return false;
	}

	bool visible_any(const cache::entity_t& entity, const math::vector3& from,
		const math::vector3& primary)
	{

		const math::vector3 eye = worldgeo::eye_position(from);

		if (math::finite(primary) && worldgeo::is_visible(eye, primary))
			return true;

		for (const char* nm : los_points())
		{
			const rbx::part_t* p = find_part(entity, nm);
			if (!p) continue;

			math::vector3 pos{};
			if (!live_point(*p, pos)) continue;
			if (worldgeo::is_visible(eye, pos))
				return true;
		}
		return false;
	}

	namespace
	{
		bool plausible_pair(float hp, float mx)
		{
			if (!std::isfinite(hp) || !std::isfinite(mx)) return false;
			if (mx <= 0.f || mx > 1.0e7f)              return false;
			if (hp < -0.001f)                          return false;
			if (hp > mx * 4.f + 1.f)                   return false;
			return true;
		}

		bool read_pair(std::uint64_t humanoid, std::uintptr_t health_off,
			std::uintptr_t max_off, float& hp, float& mx)
		{
			try
			{
				hp = memory->read<float>(humanoid + health_off);
				mx = memory->read<float>(humanoid + max_off);
			}
			catch (...) { hp = 0.f; mx = 0.f; }
			return plausible_pair(hp, mx);
		}
	}

	bool read_health(std::uint64_t humanoid, float& health, float& max_health)
	{
		if (!humanoid) return false;

		float hp = 0.f, mx = 0.f;
		if (read_pair(humanoid, Offsets::Humanoid::Health, Offsets::Humanoid::MaxHealth, hp, mx))
		{
			health = hp;
			max_health = mx;
			return true;
		}

		float gh = 0.f, gm = 0.f;
		if (read_pair(humanoid, 0x188, 0x1A8, gh, gm))
		{
			health = gh;
			max_health = gm;
			return true;
		}
		return false;
	}

	void publish_health(std::uint64_t humanoid)
	{
		float hp = 0.f, mx = 0.f;
		const bool ok = humanoid && read_health(humanoid, hp, mx);

		s_health.store(ok ? hp : 0.f, std::memory_order_relaxed);
		s_max_health.store(ok ? mx : 0.f, std::memory_order_relaxed);
		s_health_valid.store(ok, std::memory_order_relaxed);

		float gh = 0.f, gm = 0.f;
		const bool alt_ok = humanoid && read_pair(humanoid, 0x188, 0x1A8, gh, gm);
		s_alt_health.store(alt_ok ? gh : 0.f, std::memory_order_relaxed);
		s_alt_max_health.store(alt_ok ? gm : 0.f, std::memory_order_relaxed);
		s_alt_health_valid.store(alt_ok, std::memory_order_relaxed);
	}

	void publish_model_source(const char* source)
	{
		s_model_source.store(source ? source : "-", std::memory_order_relaxed);
	}

	void publish_cache_stats(int entities, int held_lists, int unresolved)
	{
		s_entities.store(entities, std::memory_order_relaxed);
		s_held.store(held_lists, std::memory_order_relaxed);
		s_unresolved.store(unresolved, std::memory_order_relaxed);
	}

	void publish_corpses(int count)
	{
		s_corpses.store(count, std::memory_order_relaxed);
	}

	void publish_animated_bodies(int count)
	{
		s_bodies.store(count, std::memory_order_relaxed);

		s_body_parts.store(s_attach_parts_max, std::memory_order_relaxed);
		s_attach_parts_max = 0;
	}

	void publish_matrix_source(const char* source)
	{
		s_matrix_source.store(source ? source : "-", std::memory_order_relaxed);
	}

	status_t status()
	{
		status_t s{};
		s.detected       = active();
		s.enabled        = settings::op1::enable;
		s.forced         = s_forced;
		s.place          = place_name();
		s.source         = detection_source();
		s.model          = s_model_source.load(std::memory_order_relaxed);
		s.matrix         = s_matrix_source.load(std::memory_order_relaxed);
		s.entities       = s_entities.load(std::memory_order_relaxed);
		s.held           = s_held.load(std::memory_order_relaxed);
		s.unresolved     = s_unresolved.load(std::memory_order_relaxed);
		s.bodies         = s_bodies.load(std::memory_order_relaxed);
		s.viewmodels     = s_viewmodels.load(std::memory_order_relaxed);
		s.body_parts     = s_body_parts.load(std::memory_order_relaxed);
		s.corpses        = s_corpses.load(std::memory_order_relaxed);
		s.health         = s_health.load(std::memory_order_relaxed);
		s.max_health     = s_max_health.load(std::memory_order_relaxed);
		s.health_valid   = s_health_valid.load(std::memory_order_relaxed);
		s.alt_health     = s_alt_health.load(std::memory_order_relaxed);
		s.alt_max_health = s_alt_max_health.load(std::memory_order_relaxed);
		s.alt_health_valid = s_alt_health_valid.load(std::memory_order_relaxed);
		return s;
	}
}
