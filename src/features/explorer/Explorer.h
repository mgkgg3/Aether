#pragma once

#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>
#include <debug/DebugConsole.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cmath>

namespace aether
{

	inline bool valid_ptr(std::uint64_t p)
	{
		if (!p || p == ~0ull)
			return false;

		MEMORY_BASIC_INFORMATION mbi{};
		if (!VirtualQueryEx(memory->get_process_handle(), reinterpret_cast<LPCVOID>(p), &mbi, sizeof(mbi)))
			return false;

		if ((mbi.State & MEM_COMMIT) == 0)
			return false;

		constexpr DWORD readable =
			PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
			PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

		return (mbi.Protect & readable) != 0;
	}

	inline std::string to_lower(std::string s)
	{
		for (char& ch : s)
			ch = (char)std::tolower((unsigned char)ch);
		return s;
	}

	inline bool has_children(std::uint64_t addr)
	{
		std::uint64_t start = memory->read<std::uint64_t>(addr + Offsets::Instance::ChildrenStart);
		if (!valid_ptr(start))
			return false;
		std::uint64_t beg = memory->read<std::uint64_t>(start);
		std::uint64_t end = memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd);
		return beg != end;
	}

	struct Node
	{
		std::uint64_t address = 0;
		std::string   name;
		std::string   cls;
		std::vector<Node> children;
		bool loaded = false;
		bool open = false;
		bool has_kids = false;
	};

	inline Node   g_root;
	inline std::uint64_t g_root_addr = 0;

	inline int service_rank(const std::string& cls)
	{
		static const char* order[] = {
			"Workspace", "Players", "Lighting", "MaterialService",
			"ReplicatedFirst", "ReplicatedStorage", "ServerStorage",
			"ServerScriptService", "StarterGui", "StarterPack", "StarterPlayer",
			"SoundService", "Chat", "TextChatService", "Teams", "TeleportService",
			"TweenService", "RunService", "UserInputService", "GuiService",
			"HttpService", "MarketplaceService", "InsertService", "Debris",
		};
		for (int i = 0; i < (int)(sizeof(order) / sizeof(order[0])); ++i)
			if (cls == order[i])
				return i;
		return 1000;
	}

	inline bool service_less(const Node& a, const Node& b)
	{
		int ra = service_rank(a.cls);
		int rb = service_rank(b.cls);
		if (ra != rb)
			return ra < rb;
		return a.name < b.name;
	}

	inline void sort_children(Node& n)
	{
		if (n.cls != "DataModel")
			return;

		auto& v = n.children;
		std::stable_sort(v.begin(), v.end(), service_less);
	}

	inline void load_children(Node& n)
	{
		if (n.loaded)
			return;

		n.loaded = true;
		n.children.clear();

		rbx::instance_t inst(n.address);
		for (const auto& c : inst.get_children())
		{
			Node cn;
			cn.address = c.address;
			cn.name = c.get_name();
			cn.cls = c.get_class_name();
			cn.has_kids = has_children(c.address);
			n.children.push_back(std::move(cn));
		}
		n.has_kids = !n.children.empty();

		sort_children(n);

		if (n.cls == "DataModel")
		{
			for (auto& c : n.children)
			{
				if (c.cls == "Workspace")
				{
					c.open = true;
					load_children(c);
					break;
				}
			}
		}
	}

	inline std::uint64_t g_ws_addr = 0;

	inline void ensure_root()
	{
		std::uint64_t dm = game::datamodel.address;
		std::uint64_t ws = game::workspace.address;

		if (dm && dm != g_root_addr)
		{
			g_root_addr = dm;
			g_ws_addr = ws;
			g_root = Node{};
			g_root.address = dm;
			g_root.cls = "DataModel";
			g_root.name = "game";
			g_root.open = true;

			debug::log("explorer", "root -> DataModel @ 0x%llX", (unsigned long long)dm);
		}
		else if (dm && ws && ws != g_ws_addr)
		{

			g_ws_addr = ws;
			g_root = Node{};
			g_root.address = dm;
			g_root.cls = "DataModel";
			g_root.name = "game";
			g_root.open = true;

			debug::log("explorer", "workspace changed (0x%llX), tree refreshed",
				(unsigned long long)ws);
		}
		else if (!dm)
		{
			if (g_root_addr)
				debug::warn("datamodel lost, clearing tree");
			g_root_addr = 0;
			g_ws_addr = 0;
			g_root = Node{};
		}
		else if (!ws)
		{

			g_ws_addr = 0;
		}
	}

	inline std::string build_path(std::uint64_t address)
	{
		std::vector<std::string> parts;
		std::uint64_t cur = address;
		int guard = 64;

		while (cur && guard-- > 0)
		{
			rbx::instance_t node(cur);
			std::string nm = node.get_name();
			if (nm.empty())
				nm = "?";
			parts.push_back(nm);

			if (cur == g_root_addr)
				break;

			std::uint64_t parent = memory->read<std::uint64_t>(cur + Offsets::Instance::Parent);
			if (!valid_ptr(parent))
				break;

			cur = parent;
		}

		std::string out;
		for (auto it = parts.rbegin(); it != parts.rend(); ++it)
		{
			if (!out.empty())
				out += ".";
			out += *it;
		}

		return out;
	}

	struct SearchResult
	{
		std::uint64_t address = 0;
		std::string name;
		std::string cls;
		std::string path;
	};

	inline std::mutex g_search_mtx;
	inline std::vector<SearchResult> g_search_results;
	inline std::atomic<std::uint32_t> g_search_gen{ 0 };
	inline std::atomic<bool> g_searching{ false };
	inline bool g_search_on = false;

	inline void run_search(std::string query, std::uint64_t root_addr, std::uint32_t gen)
	{
		debug::log("explorer", "search '%s' started", query.c_str());

		std::string q = to_lower(query);
		std::vector<SearchResult> local;

		struct Frame
		{
			std::uint64_t addr;
			std::string path;
		};
		std::vector<Frame> stack;
		stack.push_back({ root_addr, "game" });

		int budget = 400000;
		while (!stack.empty())
		{
			if (g_search_gen.load() != gen)
				return;

			if (budget-- <= 0)
				break;

			Frame f = std::move(stack.back());
			stack.pop_back();

			rbx::instance_t inst(f.addr);
			for (const auto& child : inst.get_children())
			{
				if (g_search_gen.load() != gen)
					return;

				std::string name = child.get_name();
				std::string cls = child.get_class_name();

				if (to_lower(name).find(q) != std::string::npos ||
					to_lower(cls).find(q) != std::string::npos)
				{
					if ((int)local.size() < 1000)
					{
						SearchResult r;
						r.address = child.address;
						r.name = name;
						r.cls = cls;
						r.path = f.path + "." + name;
						local.push_back(std::move(r));
					}
				}

				if ((int)stack.size() < 400000)
					stack.push_back({ child.address, f.path + "." + name });
			}
		}

		if (g_search_gen.load() != gen)
			return;

		{
			std::lock_guard<std::mutex> lk(g_search_mtx);
			g_search_results = std::move(local);
		}
		g_searching.store(false);

		debug::log("explorer", "search '%s' done, %zu hits", query.c_str(), g_search_results.size());
	}

	inline void start_search(const char* query)
	{
		std::uint32_t gen = ++g_search_gen;
		g_searching.store(true);
		g_search_on = true;
		{
			std::lock_guard<std::mutex> lk(g_search_mtx);
			g_search_results.clear();
		}

		if (!g_root_addr)
		{
			g_searching.store(false);
			return;
		}

		std::thread(run_search, std::string(query), g_root_addr, gen).detach();
	}

	inline void stop_search()
	{
		++g_search_gen;
		g_searching.store(false);
		g_search_on = false;
		std::lock_guard<std::mutex> lk(g_search_mtx);
		g_search_results.clear();
	}

	namespace value_base
	{
		inline bool is_class(const std::string& cls)
		{
			return cls == "BoolValue" || cls == "IntValue" || cls == "NumberValue"
				|| cls == "StringValue" || cls == "ObjectValue" || cls == "Vector3Value";
		}

		inline std::uint64_t field(std::uint64_t addr)
		{
			return addr + Offsets::Misc::Value;
		}

		inline bool get_bool(std::uint64_t addr)
		{
			return memory->read<std::uint8_t>(field(addr)) != 0;
		}

		inline void set_bool(std::uint64_t addr, bool v)
		{
			memory->write<std::uint8_t>(field(addr), v ? 1 : 0);
		}

		inline std::int32_t get_int(std::uint64_t addr)
		{
			return memory->read<std::int32_t>(field(addr));
		}

		inline void set_int(std::uint64_t addr, std::int32_t v)
		{
			memory->write<std::int32_t>(field(addr), v);
		}

		inline double get_number(std::uint64_t addr)
		{
			return memory->read<double>(field(addr));
		}

		inline void set_number(std::uint64_t addr, double v)
		{
			memory->write<double>(field(addr), v);
		}

		inline std::string get_string(std::uint64_t addr)
		{
			return memory->read_string(field(addr));
		}

		inline bool set_string(std::uint64_t addr, const std::string& s)
		{
			if (s.size() >= 16)
				return false;

			char buf[24]{};
			std::memcpy(buf, s.data(), s.size());
			std::int32_t len = (std::int32_t)s.size();

			std::uint64_t w0 = 0, w1 = 0, w2 = 0;
			std::memcpy(&w0, buf, 8);
			std::memcpy(&w1, buf + 8, 8);
			std::memcpy(&w2, &len, 4);

			memory->write<std::uint64_t>(field(addr), w0);
			memory->write<std::uint64_t>(field(addr) + 8, w1);
			memory->write<std::uint64_t>(field(addr) + 16, w2);
			return true;
		}

		inline std::uint64_t get_object(std::uint64_t addr)
		{
			return memory->read<std::uint64_t>(field(addr));
		}

		inline void set_object(std::uint64_t addr, std::uint64_t obj)
		{
			memory->write<std::uint64_t>(field(addr), obj);
		}

		inline math::vector3 get_vector3(std::uint64_t addr)
		{
			return memory->read<math::vector3>(field(addr));
		}

		inline void set_vector3(std::uint64_t addr, const math::vector3& v)
		{
			memory->write<math::vector3>(field(addr), v);
		}

		inline std::string format(std::uint64_t addr, const std::string& cls)
		{
			if (!addr || !is_class(cls))
				return {};

			char buf[96]{};
			if (cls == "BoolValue")
				return get_bool(addr) ? "true" : "false";

			if (cls == "IntValue")
			{
				snprintf(buf, sizeof(buf), "%d", get_int(addr));
				return buf;
			}

			if (cls == "NumberValue")
			{
				snprintf(buf, sizeof(buf), "%.6g", get_number(addr));
				return buf;
			}

			if (cls == "StringValue")
			{
				std::string s = get_string(addr);
				if (s.size() > 40)
					s = s.substr(0, 40) + "...";
				return "\"" + s + "\"";
			}

			if (cls == "ObjectValue")
			{
				const std::uint64_t obj = get_object(addr);
				if (!valid_ptr(obj))
					return "nil";

				rbx::instance_t inst(obj);
				std::string nm = inst.get_name();
				std::string c = inst.get_class_name();
				if (nm.empty())
					nm = "?";
				return nm + " (" + c + ")";
			}

			if (cls == "Vector3Value")
			{
				math::vector3 v = get_vector3(addr);
				snprintf(buf, sizeof(buf), "%.3g, %.3g, %.3g", v.x, v.y, v.z);
				return buf;
			}

			return {};
		}
	}
}
