#include "game_structure.h"

#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <sdk/math/math.h>
#include <memory/memory.h>
#include <game/game.h>
#include <debug/DebugConsole.h>

#include <algorithm>
#include <chrono>
#include <atomic>
#include <cfloat>
#include <map>
#include <set>

namespace game_structure
{

	static bool is_part_class(const std::string& cls)
	{
		if (cls.find("Part") != std::string::npos)
			return true;
		return cls == "SpawnLocation" || cls == "Seat" || cls == "VehicleSeat";
	}

	static const char* k_r6_parts[] = {
		"Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg"
	};
	static const char* k_r15_parts[] = {
		"Head", "UpperTorso", "LowerTorso", "HumanoidRootPart",
		"LeftUpperArm", "LeftLowerArm", "LeftHand",
		"RightUpperArm", "RightLowerArm", "RightHand",
		"LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
		"RightUpperLeg", "RightLowerLeg", "RightFoot"
	};

	static bool is_rig_part(const std::string& n)
	{
		for (auto p : k_r6_parts)  if (n == p) return true;
		for (auto p : k_r15_parts) if (n == p) return true;
		return false;
	}

	static int count_rig_parts(rbx::instance_t model)
	{
		if (!model.address) return 0;
		int count = 0;
		try
		{
			for (auto& ch : model.get_children())
			{
				if (!ch.address) continue;
				std::string cls = ch.get_class_name();
				if (is_part_class(cls) && is_rig_part(ch.get_name()))
					++count;
			}
		}
		catch (...) {}
		return count;
	}

	static bool has_humanoid(rbx::instance_t model)
	{
		if (!model.address) return false;
		try { return model.find_first_child("Humanoid").address != 0; }
		catch (...) { return false; }
	}

	template <typename Fn>
	static void walk(rbx::instance_t root, int depth, int max_depth, int& budget, Fn&& fn)
	{
		if (!root.address || depth > max_depth || budget <= 0) return;
		try
		{
			for (auto& ch : root.get_children())
			{
				if (!ch.address || budget <= 0) continue;
				--budget;
				fn(ch);
				walk(ch, depth + 1, max_depth, budget, fn);
			}
		}
		catch (...) {}
	}

	static std::string describe_location(rbx::instance_t inst)
	{
		if (!inst.address) return "";
		std::string loc = "?";
		try
		{
			rbx::instance_t parent{ memory->read<std::uint64_t>(inst.address + Offsets::Instance::Parent) };
			if (parent.address)
				loc = parent.get_name();
			else
				loc = "(root)";
		}
		catch (...) {}
		return loc.empty() ? "?" : loc;
	}

	static std::string read_player_name(std::uint64_t addr)
	{
		try
		{
			rbx::instance_t p(addr);
			std::string n = p.get_name();
			if (!n.empty() && n != "invalid_str") return n;
		}
		catch (...) {}
		return "";
	}

	info_t detect()
	{
		info_t out{};

		{
			if (game::players.address)
			{
				try
				{
					rbx::instance_t pl(game::players.address);
					std::string nm = pl.get_name();
					if (nm.empty() || nm == "invalid_str") nm = "Players";
					out.players_container = game::players.address;
					out.players_name      = nm;
					out.players_path      = "game." + nm;
				}
				catch (...) {}
			}

			std::uint64_t best = 0;
			int best_count = 0;
			std::string best_name, best_loc;
			auto inspect = [&](rbx::instance_t& inst)
			{
				if (!inst.address) return;
				std::string cls;
				try { cls = inst.get_class_name(); } catch (...) { return; }
				if (cls == "Player") return;
				int players = 0;
				try
				{
					for (auto& ch : inst.get_children())
					{
						if (!ch.address) continue;
						try { if (ch.get_class_name() == "Player") ++players; } catch (...) {}
					}
				}
				catch (...) { return; }
				if (players > best_count)
				{
					best_count = players;
					best = inst.address;
					try { best_name = inst.get_name(); } catch (...) { best_name = ""; }
					best_loc = describe_location(inst);
				}
			};

			if (game::datamodel.address)
			{
				try
				{
					rbx::instance_t dm(game::datamodel.address);
					int budget = 400;
					for (auto& ch : dm.get_children()) { inspect(ch); walk(ch, 1, 2, budget, inspect); }
				}
				catch (...) {}
			}
			if (game::workspace.address)
			{
				try
				{
					rbx::instance_t ws(game::workspace.address);
					int budget = 800;
					for (auto& ch : ws.get_children()) { inspect(ch); walk(ch, 1, 1, budget, inspect); }
				}
				catch (...) {}
			}

			if (best && best != out.players_container)
			{
				out.players_container = best;
				out.players_name      = best_name;
				out.players_path      = best_loc.empty() ? "?" : best_loc + "." + best_name;
				out.custom            = true;
			}
			else if (!out.players_container)
			{
				out.players_container = best;
				out.players_name      = best_name;
				out.players_path      = best_loc.empty() ? "?" : best_loc + "." + best_name;
			}
			if (out.players_name.empty()) out.players_name = "Players";
		}

		std::set<std::string> player_names;
		if (out.players_container)
		{
			try
			{
				rbx::instance_t cont(out.players_container);
				for (auto& ch : cont.get_children())
				{
					if (!ch.address) continue;
					try { if (ch.get_class_name() == "Player") player_names.insert(ch.get_name()); } catch (...) {}
				}
			}
			catch (...) {}
		}

		std::vector<character_model_t> models;
		std::set<std::string> part_names;
		{
			std::vector<std::uint64_t> chars = find_all_characters();
			for (auto& addr : chars)
			{
				rbx::instance_t m{ addr };
				std::string name;
				try { name = m.get_name(); } catch (...) {}
				int rig = count_rig_parts(m);
				character_model_t cm;
				cm.name = name;
				cm.address = addr;
				cm.rig_parts = rig;
				models.push_back(cm);

				try
				{
					for (auto& pc : m.get_children())
					{
						if (!pc.address) continue;
						try
						{
							if (is_part_class(pc.get_class_name()))
							{
								std::string pn = pc.get_name();
								if (is_rig_part(pn)) part_names.insert(pn);
							}
						}
						catch (...) {}
					}
				}
				catch (...) {}
			}
		}

		out.character_models = std::move(models);
		out.character_count  = (int)out.character_models.size();

		{
			std::map<std::string, int> name_counts;
			for (auto& m : out.character_models) ++name_counts[m.name];
			int best_n = 1;
			for (auto& [n, c] : name_counts)
				if (c > best_n) { best_n = c; out.character_name = n; }
			if (out.character_name.empty())
				out.character_name = "Character";
		}

		if (!out.players_container ||
			(out.players_container != game::players.address) ||
			(!out.character_models.empty() && player_names.empty()))
			out.custom = true;

		{
			out.parts.assign(part_names.begin(), part_names.end());
			out.part_count = (int)out.parts.size();
			int r6 = 0, r15 = 0;
			for (auto& p : out.parts)
			{
				for (auto pp : k_r6_parts)  if (p == pp) ++r6;
				for (auto pp : k_r15_parts) if (p == pp) ++r15;
			}
			if (r15 >= 6)      out.rig = "R15";
			else if (r6 >= 4)  out.rig = "R6";
			else               out.rig = out.part_count > 0 ? "custom" : "none";
		}

		{
			out.health_source = "Humanoid";
			if (out.character_count == 0 && !game::local_character.address)
				out.health_source = "none (no character found)";
		}

		using_obfuscated_models.store(out.character_count > 0);

		out.valid = true;
		return out;
	}

	info_t refresh()
	{
		if (!scan_requested.load())
			return g_info;

		static auto s_last = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		if (now - s_last < std::chrono::seconds(2) && g_info.valid)
			return g_info;
		s_last = now;

		info_t fresh = detect();
		{
			std::lock_guard<std::mutex> lk(mtx);
			g_info = fresh;
		}
		return fresh;
	}

	static bool looks_like_character(rbx::instance_t inst)
	{
		if (!inst.address) return false;
		try
		{
			bool humanoid = false;
			bool has_root = false;
			int  rig_parts = 0;
			for (auto& ch : inst.get_children())
			{
				if (!ch.address) continue;
				std::string cls;
				try { cls = ch.get_class_name(); } catch (...) { continue; }
				std::string nm;
				try { nm = ch.get_name(); } catch (...) { continue; }
				if (cls == "Humanoid") { humanoid = true; continue; }
				if (is_part_class(cls))
				{
					if (nm == "HumanoidRootPart") has_root = true;
					if (is_rig_part(nm)) ++rig_parts;
				}
			}

			if (humanoid && (has_root || rig_parts >= 4))
				return true;

			if (has_root && rig_parts >= 2)
				return true;
		}
		catch (...) {}
		return false;
	}

	static void collect_characters(rbx::instance_t root, int depth,
		std::vector<std::uint64_t>& out, int& budget)
	{
		if (!root.address || depth > 8 || budget <= 0) return;
		try
		{
			std::string cls;
			try { cls = root.get_class_name(); } catch (...) { cls = ""; }

			if (cls != "Part" && !is_part_class(cls) && cls != "Script" &&
				cls != "LocalScript" && cls != "ModuleScript" && cls != "Camera") {
				if (looks_like_character(root))
				{
					out.push_back(root.address);
					if ((int)out.size() >= 200) { budget = 0; return; }
					return;
				}
			}
			--budget;
			for (auto& ch : root.get_children())
			{
				if (!ch.address) continue;
				collect_characters(ch, depth + 1, out, budget);
				if (budget <= 0) return;
			}
		}
		catch (...) {}
	}

	std::vector<std::uint64_t> find_all_characters()
	{
		std::vector<std::uint64_t> out;
		if (!game::workspace.address) return out;
		int budget = 1200;
		collect_characters(rbx::instance_t(game::workspace.address), 0, out, budget);
		return out;
	}

	static std::uint64_t camera_subject_part()
	{
		if (!game::workspace.address) return 0;
		try
		{
			std::uint64_t cam = memory->read<std::uint64_t>(game::workspace.address + Offsets::Workspace::CurrentCamera);
			if (!cam) return 0;
			return memory->read<std::uint64_t>(cam + Offsets::Camera::CameraSubject);
		}
		catch (...) { return 0; }
	}

	static std::uint64_t rig_root_part(std::uint64_t rig)
	{
		if (!rig) return 0;
		try
		{
			rbx::instance_t m{ rig };
			rbx::instance_t hrp = m.find_first_child("HumanoidRootPart");
			if (hrp.address) return hrp.address;

			for (auto& ch : m.get_children())
			{
				if (!ch.address) continue;
				try
				{
					if (is_rig_part(ch.get_name()) && ch.get_name() != "Head")
						return ch.address;
				} catch (...) {}
			}
		}
		catch (...) {}
		return 0;
	}

	std::uint64_t resolve_local_character_camera()
	{
		std::uint64_t subject = camera_subject_part();
		if (!subject) return 0;

		try
		{

			std::uint64_t cur = subject;
			for (int i = 0; i < 6 && cur; ++i)
			{
				rbx::instance_t inst{ cur };
				if (looks_like_character(inst))
					return cur;
				cur = memory->read<std::uint64_t>(cur + Offsets::Instance::Parent);
			}

			std::vector<std::uint64_t> chars = find_all_characters();
			rbx::instance_t subject_inst{ subject };
			bool subject_is_part = is_part_class(subject_inst.get_class_name());
			for (auto& rig : chars)
			{
				rbx::instance_t m{ rig };
				try
				{

					if (subject_is_part && m.find_first_child("HumanoidRootPart").address == subject)
						return rig;
					if (m.find_first_child("Head").address == subject)
						return rig;
				}
				catch (...) {}
			}
		}
		catch (...) {}

		return 0;
	}

	static std::uint64_t infer_local_character()
	{
		std::uint64_t subj = resolve_local_character_camera();
		if (subj) return subj;

		std::vector<std::uint64_t> chars = find_all_characters();
		if (chars.empty()) return 0;

		try
		{
			std::uint64_t cam = memory->read<std::uint64_t>(game::workspace.address + Offsets::Workspace::CurrentCamera);
			if (!cam) return 0;
			math::vector3 cam_pos = memory->read<math::vector3>(cam + Offsets::Camera::Position);

			std::uint64_t best = 0;
			float best_d = FLT_MAX;
			for (auto& rig : chars)
			{
				std::uint64_t hrp = rig_root_part(rig);
				if (!hrp) continue;
				rbx::primitive_t prim = rbx::part_t{ hrp }.get_primitive();
				if (!prim.address) continue;
				math::vector3 p = prim.get_position();
				float d = (p - cam_pos).length();
				if (d < best_d) { best_d = d; best = rig; }
			}
			return best;
		}
		catch (...) {}
		return 0;
	}

	static rbx::instance_t find_named_container(const std::string& name)
	{
		if (name.empty()) return {};

		if (game::datamodel.address)
		{
			try
			{
				rbx::instance_t dm(game::datamodel.address);
				for (auto& ch : dm.get_children())
				{
					if (!ch.address) continue;
					try { if (ch.get_name() == name) return ch; } catch (...) {}
				}
				for (auto& ch : dm.get_children())
				{
					if (!ch.address) continue;
					try { for (auto& g : ch.get_children()) if (g.address && g.get_name() == name) return g; } catch (...) {}
				}
			}
			catch (...) {}
		}
		if (game::workspace.address)
		{
			try
			{
				rbx::instance_t ws(game::workspace.address);
				for (auto& ch : ws.get_children())
				{
					if (!ch.address) continue;
					try { if (ch.get_name() == name) return ch; } catch (...) {}
				}
			}
			catch (...) {}
		}
		return {};
	}

	std::uint64_t players_container_addr()
	{

		if (use_overrides && !players_name_override.empty())
		{
			rbx::instance_t found = find_named_container(players_name_override);
			if (found.address) return found.address;
		}

		if (scan_requested.load() && auto_apply.load())
		{
			info_t cur;
			{
				std::lock_guard<std::mutex> lk(mtx);
				cur = g_info;
			}
			if (cur.valid && cur.players_container)
				return cur.players_container;
		}

		return game::players.address;
	}

	std::uint64_t resolve_character(const std::string& player_name, std::uint64_t model_instance)
	{
		if (model_instance) return model_instance;

		info_t cur;
		{
			std::lock_guard<std::mutex> lk(mtx);
			cur = g_info;
		}
		for (auto& m : cur.character_models)
		{
			if (!player_name.empty() && m.name == player_name)
				return m.address;
		}

		if (use_overrides && !character_name_override.empty())
		{
			for (auto& m : cur.character_models)
				if (m.name == character_name_override)
					return m.address;
		}

		if (game::workspace.address && !player_name.empty())
		{
			try
			{
				rbx::instance_t ws(game::workspace.address);
				for (auto& ch : ws.get_children())
				{
					if (!ch.address) continue;
					try { if (ch.get_name() == player_name) return ch.address; } catch (...) {}
				}
			}
			catch (...) {}
		}
		return 0;
	}
}
