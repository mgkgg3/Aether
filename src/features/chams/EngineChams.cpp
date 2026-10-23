#define NOMINMAX
#include "EngineChams.h"

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <settings.h>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <cache/cache.h>
#include <game/game.h>
#include <debug/DebugConsole.h>

// In-client chams: the client draws the characters with a different shader
// program, so the effect is part of the frame the game renders rather than
// something drawn over it.
//
// The route is Roblox-Chams-Material-POC's, and this file has been through three
// versions of it, each one because the previous version crashed a client:
//
//   1. the first version guessed at the material's own bytes (fill mode, a flags
//      word, a param word) from a table of magic numbers - it did nothing;
//   2. the second version did the swap, but found the scene by walking the whole
//      address space for the render cluster's vtable and writing into every
//      match.  That is what took the client down (HANDOFF.md,
//      tmp/crash_analysis.txt): a freed cluster keeps its vtable bytes, so a
//      "match" is not a live object, and writing into one corrupts whatever the
//      allocator has put there since.  It also read every page of a 3 GB client
//      every 1.5 s, which is why the game loaded slowly with chams on;
//   3. what is here now: the scene is asked for the clusters it is drawing
//      (visual engine -> the scene -> fast cluster entity managers -> node
//      array -> cluster -> entities), and a block is only ever written when its
//      shader program is one the client's own shader manager calls a render pass.
//
// Two things the POC's README does not say, and both cost a client to learn:
//
//   * the manager's offsets are build specific.  Its +0xB98 is +0xBA0 here, its
//     scene updater +0xB40 still resolves through +0xB40 but its cluster manager
//     +0x588 is elsewhere, and its fill mode byte is not where it says.  Every
//     one of those is read off the live client instead: tools/shader_probe.py,
//     tools/chams_swap_probe.py and tools/scene_route_probe.py;
//   * the keys are not material names.  Each one is
//     "<vs>VS<flags>,<fs>FS<flags>," - one vertex/pixel pair for one render
//     pass, with the flags describing the vertex format that pass is set up for.
//     A swap is a substitution that keeps the flags and changes the family:
//     writing a program built for a different vertex format into a pass is how
//     a chams becomes a crash, and this file leaves such a pass alone.
//
// And two things it does say, which is why the restore path looks the way it
// does:
//
//   * a raw write does not bump the program's reference count, so a program that
//     outlives the manager it came from is a pointer into freed memory;
//   * the list goes stale on a map or character reload, and has to be read again.
//
// Hence: the manager and the scene routes are re-read when they change, the
// originals are written back only while the block still holds the program this
// feature put there (so a block the client has rebuilt is never written at all),
// and a program is only handed to the engine when the shared pointer being
// written really looks like one.

namespace engine_chams
{
	namespace
	{
		using uptr = std::uintptr_t;

		constexpr float k_rig_reach_sq = 6.f * 6.f;

		// The scene, as Roblox-Chams-Material-POC describes it and as
		// tools/scene_route_probe.py confirmed on this build: the visual engine
		// reaches a scene, the scene holds fast cluster entity managers, each
		// manager is an array of 0x1000 nodes of 0x20 bytes, and a node's +0x08
		// is a fast cluster whose +0x48 is a vector of render clusters.
		constexpr uptr k_node_stride      = 0x20;
		constexpr uptr k_node_count       = 0x1000;
		constexpr uptr k_node_cluster     = 0x08;
		constexpr uptr k_cluster_entities = 0x48;
		constexpr uptr k_node_bytes       = k_node_stride * k_node_count;
		constexpr uptr k_max_entity_bytes = 0x8000;

		// Where this build keeps them, as (visual engine field, offset in the
		// object that field points at).  The POC's +0xB40/+0x588 is one build
		// behind and is not what is used here; these are the routes the probe
		// measured, and they are only a fast path - if none of them validates (a
		// client update), the feature sweeps the visual engine for the same
		// shape, which is a one-off cost rather than a per-frame one.
		struct route_hint_t { uptr ve_field; uptr manager; };
		constexpr route_hint_t k_route_hints[] =
		{
			{ 0xCB8, 0x140 }, { 0xCB8, 0x118 }, { 0xCB8, 0x130 }, { 0xCB8, 0x148 },
			{ 0xBD0, 0x068 }, { 0xBD0, 0x070 },
			{ 0xB88, 0x3F8 }, { 0xB88, 0x3D8 }, { 0xB88, 0x3B0 },
			{ 0xC50, 0xDB8 }, { 0xC50, 0xCF8 },
			{ 0xB70, 0x14D8 }, { 0xAF0, 0x1208 }, { 0xB40, 0x808 },
		};

		// One material technique block, as this feature left it.
		struct block_backup_t
		{
			std::uint8_t   fillmode = 0;
			std::uint32_t  matflags = 0;
			std::uint32_t  param = 0;
			std::uint32_t  flags2 = 0;
			std::uint32_t  color = 0;
			std::uintptr_t program = 0;            // the pair that was there before
			std::uintptr_t program_control = 0;
			std::uintptr_t written_program = 0;    // what was written instead (0: nothing)
			std::uint32_t  written_param = 0;      // the tint this feature asked for
			bool           swapped = false;
		};

		struct program_t
		{
			uptr object = 0;
			uptr control = 0;
		};

		// The shader program list, read once per manager (and re-read when the
		// client rebuilds it, which is what a DataModel change or a map reload
		// does).
		struct manager_t
		{
			bool built = false;
			uptr manager = 0;             // the manager itself, so a rebuild is visible
			uptr head = 0;                // and its list head
			std::size_t programs = 0;
			// "<vs flags>|<fs flags>" -> the program, per family.  Keyed on the
			// flags rather than on the whole key because the two halves of a key
			// do not always name the same family: Neon's pixel shader is called
			// DefaultUnifiedNeonFFlagRenderMobileNeonHighlightFixFS, and a
			// substitution on the name would miss every one of them.
			std::unordered_map<std::string, std::unordered_map<std::string, program_t>> by_family;
			// program object -> its key, so a block's current program can be
			// turned back into the family and flags it was built for.
			std::unordered_map<uptr, std::string> key_of;
		};

		manager_t g_manager;

		// The scene: every cluster manager the feature found, and the buffers the
		// walk reuses so a tick allocates as little as possible.
		std::vector<uptr> g_routes;
		std::vector<unsigned char> g_node_buf;

		// The sweep's own reads: the visual engine's field block, and the block a
		// field points at.  Each is read whole rather than pointer by pointer, so
		// one pass over the object is hundreds of reads instead of half a million.
		std::vector<unsigned char> g_sweep_ve;
		std::vector<unsigned char> g_sweep_holder;

		struct record_t
		{
			uptr block = 0;
			uptr cluster = 0;
			uptr program = 0;
		};

		std::vector<uptr>      g_clusters;    // this tick's clusters, sorted and deduped
		std::vector<record_t>  g_records;     // their blocks that hold a render pass
		// family|flags -> a program the scene is drawing with right now.  This is
		// the set a target is taken from first, because a program the client is
		// already rendering with is one it has prepared.
		std::unordered_map<std::string, program_t> g_in_use;
		// program -> 1 ready, 2 not.  The shape checks are remote reads, and the
		// same program is a candidate for every pass of its flag pair.
		std::unordered_map<uptr, unsigned char> g_gate_cache;

		std::mutex g_mtx;
		std::unordered_map<uptr, block_backup_t> g_blocks;
		std::unordered_map<uptr, std::vector<uptr>> g_ent_blocks;   // cluster -> its blocks

		uptr g_base = 0;
		uptr g_vt = 0;
		uptr g_vt2 = 0;
		int  g_applied_style = -1;

		// What the last pass over the scene wrote, for the rig (tmp/chams_rig.cpp)
		// and for the menu's own status.
		constexpr int k_max_styles = 16;

		struct tick_stats_t
		{
			int programs = 0;
			int families = 0;
			int routes = 0;
			int entities = 0;
			int checked = 0;
			int skipped = 0;
			int tinted = 0;
			int swapped = 0;
			int unknown = 0;
			int no_target = 0;
			int not_ready = 0;
			uptr sample_block = 0;
			uptr sample_program = 0;
			int reach[k_max_styles] = {};
			int swapped_of[k_max_styles] = {};
		};

		tick_stats_t g_tick;        // the walk's own counters, on the chasm thread
		tick_stats_t g_published;   // what the menu and the rig are shown, under the lock

		std::thread       g_thread;
		std::atomic<bool> g_run{ false };

		bool in_range(uptr a)
		{
			return a >= 0x10000ull && a < 0x7fffffffffffull;
		}

		// Pointers only.  Fields are addressed at byte offsets - the material's
		// fill mode is at +0x11 - so the alignment test belongs on the things
		// that *are* pointers, and a field address must not be put through it.
		bool valid(uptr a)
		{
			return in_range(a) && (a & 7ull) == 0ull;
		}

		bool vt_match(uptr ent)
		{
			const uptr v = memory->read<uptr>(ent);
			if (!v) return false;
			return v == g_vt || (g_vt2 != 0 && v == g_vt2);
		}

		bool writable(uptr a, std::size_t size)
		{
			if (!in_range(a) || !in_range(a + size)) return false;

			HANDLE proc = memory ? memory->get_process_handle() : nullptr;
			if (!proc) return false;

			MEMORY_BASIC_INFORMATION mbi{};
			if (VirtualQueryEx(proc, reinterpret_cast<LPCVOID>(a), &mbi, sizeof(mbi)) == 0)
				return false;

			if (mbi.State != MEM_COMMIT) return false;
			if (reinterpret_cast<uptr>(mbi.BaseAddress) + mbi.RegionSize < a + size) return false;

			const DWORD p = mbi.Protect;
			return p == PAGE_READWRITE || p == PAGE_WRITECOPY ||
				p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
		}

		// refresh target: vtable only.  IsWritable sometimes lies and that used to
		// switch chams off for an entity that was perfectly reachable.
		bool cluster_known(uptr ent)
		{
			if (!g_vt || !valid(ent)) return false;
			return vt_match(ent);
		}

		bool block_alive(uptr block)
		{
			return writable(block + Offsets::MaterialLayer::FillModeByte, 1) &&
				writable(block + Offsets::MaterialLayer::ColorData, sizeof(std::uint32_t));
		}

		// ---- the shared pointer we are about to write ------------------------

		// A std::shared_ptr is two words: the object and a control block.  Before
		// either is written into a technique block, the control block is checked
		// to be a control block - its vftable is a pointer into the client's own
		// image and its use count is a small positive number.
		//
		// This is not decoration.  The program pair is the one write the engine
		// acts on immediately, and a tagged or uninitialised value in either word
		// is exactly what the crash dumps of 2026-10-15 show: the engine's release
		// path reading a "control block" that is really two packed integers and
		// faulting on the first field it touches (HANDOFF.md, tmp/crash_analysis.txt).
		bool pair_ok(uptr object, uptr control)
		{
			if (!valid(object) || !valid(control)) return false;

			const uptr vft = memory->read<uptr>(control);
			if (!vft || vft < g_base || vft >= g_base + 0x20000000ull)
				return false;

			const std::int32_t uses = memory->read<std::int32_t>(control + 8);
			if (uses <= 0 || uses > 0x100000)
				return false;

			return true;
		}

		// Is this program one the engine can be handed for a pass?
		//
		// When a technique block's program changes, the client runs a small update
		// path over the new object (RobloxPlayerBeta.exe+0x2810950 on the build the
		// dump came from): it reads +0x40 and +0x50, and dereferences each at +0x48,
		// on one branch, and +0x60 the same way on the other - which of the two a
		// pass takes is decided by a byte of the material.
		//
		// So the fields that must hold pointers are the pair the update path always
		// touches, and +0x60 only has to hold one when it holds anything at all.
		// Requiring all three is what measuring this client said not to do:
		// tools/program_gate_probe.py shows +0x60 is *zero* for every program in the
		// manager, the ones the client is drawing with included, so requiring it
		// rejects the lot and the feature silently stops swapping - while a program
		// that carries the 0xF sentinel the crash dump is about (HANDOFF.md,
		// tmp/crash_analysis.txt) is still refused, because a sentinel is not zero.
		bool engine_ready_program_uncached(uptr program)
		{
			if (!valid(program))
				return false;

			const uptr always[2] = {
				memory->read<uptr>(program + 0x40),
				memory->read<uptr>(program + 0x50),
			};

			for (uptr slot : always)
			{
				if (!valid(slot))
					return false;
				if (!valid(memory->read<uptr>(slot + 0x48)))
					return false;
			}

			const uptr other = memory->read<uptr>(program + 0x60);
			if (other != 0)
			{
				if (!valid(other))
					return false;
				if (!valid(memory->read<uptr>(other + 0x48)))
					return false;
			}

			return true;
		}

		bool program_ready(uptr program)
		{
			if (!valid(program))
				return false;

			auto it = g_gate_cache.find(program);
			if (it != g_gate_cache.end())
				return it->second == 1;

			const bool ok = engine_ready_program_uncached(program);
			g_gate_cache.emplace(program, (unsigned char)(ok ? 1 : 2));
			return ok;
		}

		// ---- the styles ------------------------------------------------------

		// The style list, and the measurement behind it.
		//
		// A pass can only be given a program that carries the *same* flag pair as
		// the one already in it, so which families are worth offering is a question
		// with an answer in the client, not a matter of taste.
		// tools/chams_swap_probe.py walks the live materials and counts the families
		// that can take them; tools/scene_route_probe.py counts the ones the scene
		// is drawing with, which is the set a prepared program comes from, and the
		// menu's "reach" per style is that count.
		//
		// Style 0 swaps nothing at all: the render queue still lifts the character
		// through walls, but its materials are untouched.  That was the feature's
		// old default, and it stays the default.
		const char* const k_families[] = {
			nullptr,                                    // as-is, through walls only
			"DefaultUnifiedPlastic",
			"DefaultUnifiedNeon",
			"DefaultUnifiedDetiled",
			"DefaultUnifiedTiled",
			"DefaultUnifiedFlatTransparent",
			"DefaultUnifiedFlatOpaque",
			"DefaultUnifiedGlass",
			"DefaultUnifiedForcefield",
			"DefaultUnifiedPlasticReflection",
		};
		constexpr int k_family_count()
		{
			return (int)(sizeof(k_families) / sizeof(k_families[0]));
		}

		// What the menu shows.  The names say the family, because that is what the
		// style really is - the old list (Ghost, Wireframe, Hologram) named the fill
		// mode and the param word this file used to guess at.
		const char* const k_style_names[] = {
			"Through Walls (no swap)",
			"Plastic",
			"Neon",
			"Detiled",
			"Tiled",
			"Flat Transparent",
			"Flat Opaque",
			"Glass",
			"Forcefield",
			"Plastic Reflection",
		};
		static_assert(sizeof(k_style_names) / sizeof(k_style_names[0]) ==
			(std::size_t)(sizeof(k_families) / sizeof(k_families[0])),
			"style names and families must line up");

		const char* family_for_style(int style)
		{
			if (style < 0) style = 0;
			if (style >= k_family_count()) style = 0;
			return k_families[style];
		}

		// ---- the shader program list ---------------------------------------

		// The manager's node key is a std::string: sixteen byte buffer, then the
		// size.  Short strings live in the buffer, long ones on the heap.
		std::string read_key(uptr at)
		{
			unsigned char buf[16]{};
			if (memory->read_raw(at, buf, sizeof(buf)) != sizeof(buf))
				return std::string();

			std::uint64_t size = 0;
			if (memory->read_raw(at + 0x10, &size, sizeof(size)) != sizeof(size))
				return std::string();
			if (size == 0 || size > 512)
				return std::string();

			if (size <= 15)
				return std::string(reinterpret_cast<const char*>(buf), (std::size_t)size);

			uptr text = 0;
			std::memcpy(&text, buf, sizeof(text));
			if (!valid(text))
				return std::string();

			std::string out;
			out.resize((std::size_t)size);
			if (memory->read_raw(text, out.data(), out.size()) != out.size())
				return std::string();
			return out;
		}

		// "<name>VS<vsflags>,<name>FS<fsflags>," -> "vsflags|fsflags", or empty
		// for a key that is not a render pass of that shape.
		bool flag_pair(const std::string& key, std::string& out)
		{
			const std::size_t comma = key.find(',');
			if (comma == std::string::npos)
				return false;

			const std::string vs = key.substr(0, comma);
			std::size_t fs_end = key.find(',', comma + 1);
			if (fs_end == std::string::npos)
				fs_end = key.size();
			const std::string fs = key.substr(comma + 1, fs_end - comma - 1);

			const std::size_t vd = vs.find_last_not_of("0123456789");
			const std::size_t fd = fs.find_last_not_of("0123456789");
			if (vd + 1 >= vs.size() || fd + 1 >= fs.size())
				return false;

			out = vs.substr(vd + 1) + "|" + fs.substr(fd + 1);
			return true;
		}

		// The family a key belongs to: everything before its "VS".
		std::string family_of(const std::string& key)
		{
			const std::size_t vs = key.find("VS");
			if (vs == std::string::npos || vs == 0)
				return std::string();
			return key.substr(0, vs);
		}

		void forget_manager()
		{
			g_manager.built = false;
			g_manager.manager = 0;
			g_manager.head = 0;
			g_manager.programs = 0;
			g_manager.by_family.clear();
			g_manager.key_of.clear();
			g_gate_cache.clear();
		}

		bool build_manager()
		{
			if (g_manager.built)
				return true;

			if (!g_base)
				return false;

			const uptr ve = memory->read<uptr>(g_base + Offsets::VisualEngine::Pointer);
			if (!valid(ve))
				return false;

			const uptr manager = memory->read<uptr>(ve + Offsets::ShaderManager::Manager);
			if (!valid(manager))
				return false;

			const uptr head = memory->read<uptr>(manager + Offsets::ShaderManager::ListHead);
			if (!valid(head))
				return false;

			std::unordered_map<std::string, std::unordered_map<std::string, program_t>> by_family;
			std::unordered_map<uptr, std::string> key_of;

			uptr node = head;
			std::size_t count = 0;
			for (int i = 0; i < 4096; ++i)
			{
				if (!valid(node))
					break;

				const std::string key = read_key(node + Offsets::ShaderManager::NodeKey);
				const uptr program = memory->read<uptr>(node + Offsets::ShaderManager::NodeProgram);
				const uptr control = memory->read<uptr>(node + Offsets::ShaderManager::NodeProgram + 8);

				if (!key.empty() && valid(program) && valid(control))
				{
					++count;

					const std::string family = family_of(key);
					std::string flags;
					if (!family.empty() && flag_pair(key, flags))
					{
						auto& per_flags = by_family[family];
						// First one wins: a manager is built once and a key that
						// repeats does not change what the pass is.
						if (!per_flags.count(flags))
							per_flags.emplace(flags, program_t{ program, control });
					}

					if (!key_of.count(program))
						key_of.emplace(program, key);
				}

				const uptr next = memory->read<uptr>(node + Offsets::ShaderManager::NodeNext);
				if (!valid(next) || next == head)
					break;
				node = next;
			}

			if (by_family.empty())
			{
				debug::warn("[chams] shader manager at %p read as empty - not swapping programs",
					(void*)manager);
				return false;
			}

			g_manager.by_family.swap(by_family);
			g_manager.key_of.swap(key_of);
			g_manager.manager = manager;
			g_manager.head = head;
			g_manager.programs = count;
			g_manager.built = true;

			debug::log("chams", "shader manager: %zu program(s) in %zu families, %zu with a render-pass key",
				count, g_manager.by_family.size(), g_manager.key_of.size());
			return true;
		}

		// The manager is rebuilt on a map or character reload, and the README is
		// explicit that the programs saved before that are stale afterwards.  A
		// manager that is not the one the list was read from means every saved
		// program is a pointer into memory the client has freed, so the state is
		// dropped without writing anything back.
		bool manager_changed()
		{
			if (!g_base || !g_manager.built)
				return false;

			const uptr ve = memory->read<uptr>(g_base + Offsets::VisualEngine::Pointer);
			if (!valid(ve))
				return false;

			const uptr manager = memory->read<uptr>(ve + Offsets::ShaderManager::Manager);
			if (!valid(manager))
				return false;

			return manager != g_manager.manager;
		}

		// ---- the scene -------------------------------------------------------

		// Sweep state, so the search for the scene is a few milliseconds at a time
		// rather than one long stall: a place with no scene in it yet (the client
		// sitting on its own menus, or in a loading screen when the feature came
		// on) is exactly the case where the routes are not there to be found, and
		// the search has to be able to pick up where it left off.
		//
		// The sweep is the fallback, not the plan.  The measured routes are what
		// this feature runs on, and a session that found one of them never sweeps:
		// a log from a real session shows 4 routes from the measured list and 64
		// after the sweep, and the 60 the sweep added are the ones whose only
		// evidence is a pointer that looks like a manager.
		bool g_hints_tried = false;
		bool g_sweep_done = false;
		uptr g_sweep_off1 = 0;
		uptr g_sweep_off2 = 0;
		unsigned long long g_sweep_retry_at = 0;   // a scene that was not there yet is looked for again

		bool vector_sane(uptr begin, uptr end, uptr& count)
		{
			if (!valid(begin) || end <= begin)
				return false;
			const uptr bytes = end - begin;
			if (bytes > k_max_entity_bytes || (bytes % 8) != 0)
				return false;
			count = bytes / 8;
			return count > 0;
		}

		// Is this a fast cluster entity manager at all?  Sixteen nodes with a
		// plausible cluster whose entity vector looks like a vector is enough to
		// be worth the full read; the full read is 128 KB, so a sweep cannot
		// afford to do it for every pointer pair in the visual engine.
		bool manager_shaped(uptr mgr)
		{
			unsigned char buf[k_node_stride * 16]{};
			if (memory->read_raw(mgr, buf, sizeof(buf)) != sizeof(buf))
				return false;

			for (uptr i = 0; i < sizeof(buf); i += k_node_stride)
			{
				uptr cluster = 0;
				std::memcpy(&cluster, buf + i + k_node_cluster, sizeof(cluster));
				if (!valid(cluster))
					continue;

				const uptr begin = memory->read<uptr>(cluster + k_cluster_entities);
				const uptr end   = memory->read<uptr>(cluster + k_cluster_entities + 8);
				uptr n = 0;
				if (vector_sane(begin, end, n))
					return true;
			}
			return false;
		}

		// Every render cluster under this manager, by vtable.  `want` of 0 means
		// all of them.
		int collect_clusters(uptr mgr, std::vector<uptr>& out, int want)
		{
			if (g_node_buf.size() < k_node_bytes)
				g_node_buf.resize(k_node_bytes);

			if (memory->read_raw(mgr, g_node_buf.data(), k_node_bytes) != k_node_bytes)
				return 0;

			int found = 0;
			for (uptr i = 0; i < k_node_bytes; i += k_node_stride)
			{
				uptr cluster = 0;
				std::memcpy(&cluster, g_node_buf.data() + i + k_node_cluster, sizeof(cluster));
				if (!valid(cluster))
					continue;
				if (!cluster_known(cluster))
					continue;

				out.push_back(cluster);
				++found;
				if (want > 0 && found >= want)
					break;
			}
			return found;
		}

		// A candidate is only a route once a cluster under it is really a render
		// cluster, by vtable: a pointer that merely looks like a manager is not the
		// scene, and a write to it would be a write to something nobody asked about.
		bool consider_route(uptr mgr)
		{
			if (!valid(mgr))
				return false;
			if (std::find(g_routes.begin(), g_routes.end(), mgr) != g_routes.end())
				return false;
			if (!manager_shaped(mgr))
				return false;

			std::vector<uptr> probe;
			if (collect_clusters(mgr, probe, 1) == 0)
				return false;

			g_routes.push_back(mgr);
			return true;
		}

		// Where the scene is on this build.
		//
		// The measured routes are tried first, which is the whole cost on a client
		// that has been measured.  When none of them validates - a client update,
		// or a build whose fields moved - the visual engine is swept for the same
		// shape, in slices: a few milliseconds per tick, resuming where the last
		// slice stopped.  Routes are usable the moment they are found, so the chams
		// works during the sweep rather than after it.
		void resolve_routes(int budget_ms = 25)
		{
			if (!g_base)
				return;

			// A client sitting on its own menus, or in a loading screen, has no
			// scene to find: the sweep finishing empty is not the same answer as the
			// offsets moving, so it is looked for again in a moment.
			if (g_sweep_done && g_routes.empty())
			{
				const unsigned long long now = ::GetTickCount64();
				if (now < g_sweep_retry_at)
					return;
				g_sweep_done = false;
				g_hints_tried = false;   // the fields the hints read fill in as a place loads
				g_sweep_off1 = 0;
				g_sweep_off2 = 0;
			}

			const uptr ve = memory->read<uptr>(g_base + Offsets::VisualEngine::Pointer);
			if (!valid(ve))
				return;

			if (!g_hints_tried)
			{
				g_hints_tried = true;
				const std::size_t before = g_routes.size();
				for (const route_hint_t& hint : k_route_hints)
				{
					const uptr holder = memory->read<uptr>(ve + hint.ve_field);
					if (!valid(holder))
						continue;
					consider_route(memory->read<uptr>(holder + hint.manager));
				}

				if (g_routes.size() != before)
				{
					// The measured routes answered, which is the whole cost on every
					// session that has been measured: no sweep, so nothing is written
					// into a manager whose only evidence is a pointer that looks like
					// one, and no pass over the visual engine while the game loads.
					g_sweep_done = true;
					debug::log("chams", "scene: %zu cluster manager(s) from the measured routes",
						g_routes.size());
					return;
				}
			}

			if (g_sweep_done || g_routes.size() >= 64)
				return;

			const auto start = std::chrono::steady_clock::now();

			// Both blocks are read whole.  A remote read per candidate is what made
			// this expensive enough to feel: 512 fields and up to 768 offsets under
			// each of them, every one of them a call into the kernel, and the game
			// loads with the feature on.
			g_sweep_ve.resize(0x1000);
			g_sweep_holder.resize(0x1800);

			if (memory->read_raw(ve, g_sweep_ve.data(), g_sweep_ve.size()) != g_sweep_ve.size())
				return;

			for (; g_sweep_off1 < 0x1000; g_sweep_off1 += 8)
			{
				uptr holder = 0;
				std::memcpy(&holder, g_sweep_ve.data() + g_sweep_off1, sizeof(holder));

				if (valid(holder) &&
					memory->read_raw(holder, g_sweep_holder.data(), g_sweep_holder.size()) ==
						g_sweep_holder.size())
				{
					for (; g_sweep_off2 < 0x1800; g_sweep_off2 += 8)
					{
						uptr candidate = 0;
						std::memcpy(&candidate, g_sweep_holder.data() + g_sweep_off2,
							sizeof(candidate));
						consider_route(candidate);

						if (std::chrono::steady_clock::now() - start >
							std::chrono::milliseconds(budget_ms))
						{
							g_sweep_off2 += 8;
							return;
						}
					}
				}
				g_sweep_off2 = 0;
				if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(budget_ms))
				{
					g_sweep_off1 += 8;
					return;
				}
			}

			g_sweep_done = true;
			g_sweep_retry_at = ::GetTickCount64() + 10000;
			debug::log("chams", "scene swept: %zu cluster manager(s)", g_routes.size());

			if (g_routes.empty())
				debug::warn("[chams] the scene did not resolve - there is nothing to colour");
		}

		void forget_routes()
		{
			g_routes.clear();
			g_hints_tried = false;
			g_sweep_done = false;
			g_sweep_off1 = 0;
			g_sweep_off2 = 0;
			g_sweep_retry_at = 0;
		}

		// One pass over the scene: the clusters the client is drawing, the blocks
		// they carry, and - from those blocks - the programs the client is
		// rendering with right now.
		void walk_scene()
		{
			g_clusters.clear();
			g_records.clear();
			g_in_use.clear();

			for (uptr mgr : g_routes)
				collect_clusters(mgr, g_clusters, 0);

			if (g_clusters.empty())
				return;

			// Two routes can reach the same cluster - the measured ones overlap -
			// and a cluster visited twice would be written twice per tick.
			std::sort(g_clusters.begin(), g_clusters.end());
			g_clusters.erase(std::unique(g_clusters.begin(), g_clusters.end()), g_clusters.end());

			for (uptr cluster : g_clusters)
			{
				const uptr material = memory->read<uptr>(
					cluster + Offsets::FastClusterEntity::TechniqueArrayPtr);
				if (!valid(material))
					continue;

				const uptr begin = memory->read<uptr>(material + Offsets::TechniqueArray::BeginOffset);
				const uptr end   = memory->read<uptr>(material + Offsets::TechniqueArray::EndOffset);
				if (!valid(begin) || end <= begin)
					continue;

				const uptr bytes = end - begin;
				if (bytes > 64ull * 1024ull)
					continue;

				const std::size_t count = (std::size_t)(bytes / Offsets::MaterialLayer::Stride);
				if (count == 0 || count > 64)
					continue;

				for (std::size_t i = 0; i < count; ++i)
				{
					const uptr block = begin + i * Offsets::MaterialLayer::Stride;
					const uptr program = memory->read<uptr>(
						block + Offsets::MaterialLayer::ShaderProgram);
					if (!program)
						continue;

					++g_tick.checked;

					// Only passes the client's own list calls a render pass are ever
					// touched.  A shadow, depth or highlight block has a program too,
					// and it is not one of those keys - writing a fill mode or a
					// colour into those is how a chams breaks the shadows it stands
					// in, and the block's own program is the only thing that says
					// which kind of pass it is.
					auto kit = g_manager.key_of.find(program);
					if (kit == g_manager.key_of.end())
					{
						++g_tick.unknown;
						continue;
					}

					g_records.push_back(record_t{ block, cluster, program });

					std::string family = family_of(kit->second);
					std::string flags;
					if (family.empty() || !flag_pair(kit->second, flags))
						continue;

					program_t pair;
					pair.object = program;
					pair.control = memory->read<uptr>(
						block + Offsets::MaterialLayer::ShaderProgram + 8);

					const std::string use_key = family + "|" + flags;
					auto ins = g_in_use.emplace(use_key, pair);
					if (!ins.second && !pair_ok(ins.first->second.object, ins.first->second.control))
						ins.first->second = pair;
				}
			}

			g_tick.entities = (int)g_clusters.size();
		}

		// A program the manager lists for this flag pair, that this client will not
		// accept: the two reasons a pass keeps its own program are worth telling
		// apart, because one is "this style has nothing for this pass" and the other
		// is "it has one and the engine is not ready for it".
		bool listed_but_unready(const std::string& flags, const char* family, uptr current)
		{
			if (!family)
				return false;

			auto fit = g_manager.by_family.find(family);
			if (fit == g_manager.by_family.end())
				return false;

			auto pit = fit->second.find(flags);
			if (pit == fit->second.end() || pit->second.object == current)
				return false;

			return !pair_ok(pit->second.object, pit->second.control) ||
				!program_ready(pit->second.object);
		}

		// The program a family uses for the pass this block is set up for.
		//
		// Two sources, and the order matters: a program the scene is *already*
		// drawing with is one the client has prepared, so it is used when there is
		// one; the manager's own list is the fallback, and only for a program that
		// passes the engine-ready shape test.  A program that is merely listed is
		// not necessarily prepared - that is what the first crash of 2026-10-15
		// was - so the fallback is the second choice, not the first.
		bool target_for(const std::string& flags, const char* family, const program_t& current, program_t& out)
		{
			const std::string key = std::string(family) + "|" + flags;

			auto iit = g_in_use.find(key);
			if (iit != g_in_use.end() && iit->second.object != current.object &&
				pair_ok(iit->second.object, iit->second.control) &&
				program_ready(iit->second.object))
			{
				out = iit->second;
				return true;
			}

			auto fit = g_manager.by_family.find(family);
			if (fit == g_manager.by_family.end())
				return false;

			auto pit = fit->second.find(flags);
			if (pit == fit->second.end())
				return false;

			if (pit->second.object == current.object)
				return false;

			if (!pair_ok(pit->second.object, pit->second.control))
				return false;

			if (!program_ready(pit->second.object))
				return false;

			out = pit->second;
			return true;
		}

		std::uint32_t color_param(int idx)
		{
			if (idx < 0) idx = 0;
			if (idx > 6) idx = 6;
			return (std::uint32_t)(idx + 1);
		}

		int nearest_color_idx(const float c[4])
		{
			static const float tab[7][3] = {
				{ 1.f, 0.f,  0.f }, { 0.f, 1.f,  0.f }, { 1.f, 0.5f, 0.f },
				{ 0.f, 0.5f, 1.f }, { 1.f, 0.f,  1.f }, { 0.f, 1.f,  1.f },
				{ 1.f, 1.f,  1.f },
			};

			float best = 1.0e30f;
			int   bi   = 6;
			for (int i = 0; i < 7; ++i)
			{
				const float dr = c[0] - tab[i][0];
				const float dg = c[1] - tab[i][1];
				const float db = c[2] - tab[i][2];
				const float d  = dr * dr + dg * dg + db * db;
				if (d < best) { best = d; bi = i; }
			}
			return bi;
		}

		std::uint32_t style_queue(int style)
		{
			(void)style;
			// Every style draws on top now: that is what makes the effect
			// visible through walls, and it is the same write the feature's old
			// default style made.  The old table varied it per style, which made
			// "style" mean two different things at once.
			return Offsets::RenderQueue::AlwaysOnTop;
		}

		// ---- writing, and putting it back ------------------------------------

		bool still_ours(uptr block, const block_backup_t& b)
		{
			if (!block_alive(block))
				return false;

			if (b.swapped)
				return memory->read<uptr>(block + Offsets::MaterialLayer::ShaderProgram) == b.written_program;

			// A tint-only block: the fill mode, the param and the colour are what
			// this feature asked for.  Anything else means the client has rebuilt
			// the material, and a rebuilt material is not written back to.
			return memory->read<std::uint8_t>(block + Offsets::MaterialLayer::FillModeByte) == 0 &&
				memory->read<std::uint32_t>(block + Offsets::MaterialLayer::Param) == b.written_param &&
				memory->read<std::uint32_t>(block + Offsets::MaterialLayer::ColorData) == 0xFFFFFFFFu;
		}

		void restore_block(uptr block, const block_backup_t& b)
		{
			if (!still_ours(block, b))
				return;

			if (b.swapped && writable(block + Offsets::MaterialLayer::ShaderProgram, 16) &&
				pair_ok(b.program, b.program_control))
			{
				// The program goes back first and on its own: it is the one write
				// the engine acts on, and a saved program from a rebuilt manager is
				// a pointer into freed memory - which is why the pair is checked
				// here too.
				memory->write<uptr>(block + Offsets::MaterialLayer::ShaderProgram, b.program);
				memory->write<uptr>(block + Offsets::MaterialLayer::ShaderProgram + 8, b.program_control);
			}

			memory->write<std::uint8_t>(block + Offsets::MaterialLayer::FillModeByte, b.fillmode);
			if (writable(block + Offsets::MaterialLayer::MatFlags, 4))
				memory->write<std::uint32_t>(block + Offsets::MaterialLayer::MatFlags, b.matflags);
			if (writable(block + Offsets::MaterialLayer::Param, 4))
				memory->write<std::uint32_t>(block + Offsets::MaterialLayer::Param, b.param);
			if (writable(block + Offsets::MaterialLayer::Flags2, 4))
				memory->write<std::uint32_t>(block + Offsets::MaterialLayer::Flags2, b.flags2);
			memory->write<std::uint32_t>(block + Offsets::MaterialLayer::ColorData, b.color);
		}

		void restore_blocks_locked(uptr ent)
		{
			auto bit = g_ent_blocks.find(ent);
			if (bit == g_ent_blocks.end()) return;

			for (uptr block : bit->second)
			{
				auto sit = g_blocks.find(block);
				if (sit == g_blocks.end()) continue;
				restore_block(block, sit->second);
				g_blocks.erase(sit);
			}
			g_ent_blocks.erase(bit);
		}

		void restore_everything()
		{
			std::vector<uptr> ents;
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				ents.reserve(g_ent_blocks.size());
				for (const auto& kv : g_ent_blocks) ents.push_back(kv.first);
			}

			for (uptr ent : ents)
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				restore_blocks_locked(ent);
			}
			g_applied_style = -1;
		}

		// Nothing is written once the feature is off, so the walk's own counters
		// have to say so: a menu (or the rig) that read the last tick's numbers
		// would report a scene it is no longer touching.
		void publish_off()
		{
			std::lock_guard<std::mutex> lk(g_mtx);
			g_published.entities = 0;
			g_published.checked = 0;
			g_published.skipped = 0;
			g_published.tinted = 0;
			g_published.swapped = 0;
			g_published.unknown = 0;
			g_published.no_target = 0;
			g_published.not_ready = 0;
			g_published.sample_block = 0;
			g_published.sample_program = 0;
			for (int i = 0; i < k_max_styles; ++i)
			{
				g_published.reach[i] = 0;
				g_published.swapped_of[i] = 0;
			}
		}

		// Forget everything without writing anything: for the case where the
		// client has rebuilt its shader manager, which makes every saved program
		// and every saved block address a pointer into memory it has reused.
		void drop_state()
		{
			std::lock_guard<std::mutex> lk(g_mtx);
			g_blocks.clear();
			g_ent_blocks.clear();
			g_applied_style = -1;
		}

		// ---- the local player's own rig --------------------------------------

		void push_part(const cache::entity_t& e, const char* name, std::vector<math::vector3>& out)
		{
			auto it = e.parts.find(name);
			if (it == e.parts.end() || !it->second.address) return;
			try
			{
				rbx::primitive_t prim = it->second.get_primitive();
				if (!prim.address) return;
				const math::vector3 p = prim.get_position();
				if (std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z))
					out.push_back(p);
			}
			catch (...) {}
		}

		bool local_anchors(std::vector<math::vector3>& out)
		{
			out.clear();
			std::lock_guard<std::mutex> lk(cache::mtx);
			push_part(cache::cached_local_player, "Head", out);
			push_part(cache::cached_local_player, "HumanoidRootPart", out);
			push_part(cache::cached_local_player, "UpperTorso", out);
			push_part(cache::cached_local_player, "LowerTorso", out);
			return !out.empty();
		}

		void other_anchors(std::vector<math::vector3>& out)
		{
			out.clear();
			std::lock_guard<std::mutex> lk(cache::mtx);
			for (auto& e : cache::cached_players)
			{
				push_part(e, "HumanoidRootPart", out);
				push_part(e, "Head", out);
			}
		}

		bool world_center(uptr ent, math::vector3& out)
		{
			namespace FC = Offsets::FastClusterEntity;

			const float minx = memory->read<float>(ent + FC::BBoxMinX);
			const float maxx = memory->read<float>(ent + FC::BBoxMaxX);
			if (!std::isfinite(minx) || !std::isfinite(maxx)) return false;

			const float miny = memory->read<float>(ent + FC::BBoxMinY);
			const float minz = memory->read<float>(ent + FC::BBoxMinZ);
			const float maxy = memory->read<float>(ent + FC::BBoxMaxY);
			const float maxz = memory->read<float>(ent + FC::BBoxMaxZ);

			const math::vector3 local((minx + maxx) * 0.5f, (miny + maxy) * 0.5f, (minz + maxz) * 0.5f);

			const uptr ctx = memory->read<uptr>(ent + FC::ContextPtr);
			if (!valid(ctx)) return false;

			const uptr pool = memory->read<uptr>(ctx + Offsets::FastClusterContext::PrimitivePoolPtr);
			if (!valid(pool)) return false;

			const uptr base = memory->read<uptr>(pool + Offsets::FastClusterRecord::PoolArrayBase);
			if (!valid(base)) return false;

			const uptr idx_ptr = memory->read<uptr>(ent + FC::PrimitiveIndexArrayPtr);
			if (!valid(idx_ptr)) return false;

			const std::uint32_t idx = memory->read<std::uint32_t>(idx_ptr);
			if (idx > 1000000u) return false;

			const uptr record = base + Offsets::FastClusterRecord::Stride * (uptr)idx;
			if (!valid(record)) return false;

			float m[9]{};
			if (memory->read_raw(record, m, sizeof(m)) != sizeof(m)) return false;

			const math::vector3 t = memory->read<math::vector3>(
				record + Offsets::FastClusterRecord::Translation);
			out.x = m[0] * local.x + m[1] * local.y + m[2] * local.z + t.x;
			out.y = m[3] * local.x + m[4] * local.y + m[5] * local.z + t.y;
			out.z = m[6] * local.x + m[7] * local.y + m[8] * local.z + t.z;
			return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
		}

		float min_dist_sq(const math::vector3& p, const std::vector<math::vector3>& anchors)
		{
			float best = 1.0e30f;
			for (const math::vector3& a : anchors)
			{
				const math::vector3 d = p - a;
				const float dsq = d.x * d.x + d.y * d.y + d.z * d.z;
				if (dsq < best) best = dsq;
			}
			return best;
		}

		// only used to recognise the local player's own rig - never to decide
		// whether an entity is worth colouring
		bool is_local_cluster(uptr ent, const std::vector<math::vector3>& local_a,
			const std::vector<math::vector3>& other_a)
		{
			if (local_a.empty()) return false;

			math::vector3 p{};
			if (!world_center(ent, p)) return false;

			const float dl = min_dist_sq(p, local_a);
			if (dl > k_rig_reach_sq) return false;

			if (!other_a.empty())
			{
				const float dout = min_dist_sq(p, other_a);
				if (dout + 0.35f < dl) return false;
			}
			return true;
		}

		// One block: the tint this feature has always written, then the swap on
		// top of it - if this style has a program for this pass's flag pair, if
		// the engine is ready for that program, and if the pair about to go in
		// really is a shared pointer.
		void apply_block(uptr cluster, uptr block, uptr program, const char* family,
			std::uint32_t cp, int style)
		{
			program_t current;
			current.object = program;
			current.control = memory->read<uptr>(
				block + Offsets::MaterialLayer::ShaderProgram + 8);

			auto kit = g_manager.key_of.find(program);
			if (kit == g_manager.key_of.end())
				return;

			std::string flags;
			if (!flag_pair(kit->second, flags))
				return;

			program_t target{};
			const bool have_target = family && target_for(flags, family, current, target);

			// The style's reach, counted even when this particular block is
			// already in the state the feature leaves: it says how much of the
			// scene a style can move, which is what the menu shows.
			if (have_target && style >= 0 && style < k_max_styles)
				++g_tick.reach[style];

			const bool swap = have_target && writable(block + Offsets::MaterialLayer::ShaderProgram, 16);

			// A block already in the state this feature leaves costs nothing:
			// materials are re-applied by the engine, so this is the common case
			// on every tick but the first.
			if (memory->read<std::uint8_t>(block + Offsets::MaterialLayer::FillModeByte) == 0 &&
				memory->read<std::uint32_t>(block + Offsets::MaterialLayer::Param) == cp &&
				memory->read<std::uint32_t>(block + Offsets::MaterialLayer::Flags2) == 0 &&
				memory->read<std::uint32_t>(block + Offsets::MaterialLayer::ColorData) == 0xFFFFFFFFu &&
				(!have_target || current.object == target.object))
			{
				++g_tick.skipped;
				return;
			}

			if (!block_alive(block))
				return;

			// What it looked like before this feature touched it.  Kept per
			// block, and only written back while the block still holds what was
			// written here.
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				if (!g_blocks.count(block))
				{
					block_backup_t b{};
					b.fillmode = memory->read<std::uint8_t>(block + Offsets::MaterialLayer::FillModeByte);
					b.matflags = memory->read<std::uint32_t>(block + Offsets::MaterialLayer::MatFlags);
					b.param    = memory->read<std::uint32_t>(block + Offsets::MaterialLayer::Param);
					b.flags2   = memory->read<std::uint32_t>(block + Offsets::MaterialLayer::Flags2);
					b.color    = memory->read<std::uint32_t>(block + Offsets::MaterialLayer::ColorData);
					b.program  = current.object;
					b.program_control = current.control;
					b.swapped  = swap;
					b.written_program = swap ? target.object : 0;
					b.written_param = cp;
					g_blocks[block] = b;
					g_ent_blocks[cluster].push_back(block);
				}
			}

			memory->write<std::uint8_t>(block + Offsets::MaterialLayer::FillModeByte, 0);
			if (writable(block + Offsets::MaterialLayer::MatFlags, 4))
				memory->write<std::uint32_t>(block + Offsets::MaterialLayer::MatFlags, 0);
			if (writable(block + Offsets::MaterialLayer::Param, 4))
				memory->write<std::uint32_t>(block + Offsets::MaterialLayer::Param, cp);
			if (writable(block + Offsets::MaterialLayer::Flags2, 4))
				memory->write<std::uint32_t>(block + Offsets::MaterialLayer::Flags2, 0);
			memory->write<std::uint32_t>(block + Offsets::MaterialLayer::ColorData, 0xFFFFFFFFu);
			++g_tick.tinted;

			if (swap)
			{
				// Both halves of the shared pointer, in the POC's order: the
				// object and then its control block.
				memory->write<uptr>(block + Offsets::MaterialLayer::ShaderProgram + 8, target.control);
				memory->write<uptr>(block + Offsets::MaterialLayer::ShaderProgram, target.object);

				{
					std::lock_guard<std::mutex> lk(g_mtx);
					auto sit = g_blocks.find(block);
					if (sit != g_blocks.end())
					{
						sit->second.swapped = true;
						sit->second.written_program = target.object;
					}
				}

				++g_tick.swapped;
				if (style >= 0 && style < k_max_styles)
					++g_tick.swapped_of[style];
				if (!g_tick.sample_block)
				{
					g_tick.sample_block   = block;
					g_tick.sample_program = target.object;
				}
			}
			else if (family)
			{
				// This pass keeps the program it had.  Counted, and split by why,
				// because the two reasons are different problems: the style has no
				// program for this flag pair, or it has one this client will not
				// accept as a shared pointer.
				if (listed_but_unready(flags, family, current.object))
					++g_tick.not_ready;
				else
					++g_tick.no_target;
			}
		}

		// ---- the loop --------------------------------------------------------

		bool feature_on()
		{
			return settings::visuals::engine_chams && memory && memory->get_process_id() != 0;
		}

		void tick_once()
		{
			const int style     = settings::visuals::engine_chams_style;
			const int color_idx = nearest_color_idx(settings::visuals::engine_chams_color);
			const std::uint32_t cp = color_param(color_idx);
			const char* family = family_for_style(style);
			const bool skip_local = !settings::visuals::localplayer;

			tick_stats_t fresh;
			fresh.programs = (int)g_manager.programs;
			fresh.families = (int)g_manager.by_family.size();
			fresh.routes   = (int)g_routes.size();
			g_tick = fresh;

			// The manager is rebuilt on a reload, and everything saved against the
			// old one points into memory the client has reused.  The state goes
			// without any write at all, and the list is read again.
			if (manager_changed())
			{
				drop_state();
				forget_manager();
				forget_routes();
				resolve_routes();
				debug::log("chams", "shader manager rebuilt - restored nothing, re-reading the list");
			}

			// The style changed: everything written for the old one goes back
			// first.  A program pointer that outlives the family it came from is
			// the case the POC warns about, and a style change is when it would
			// happen.
			if (g_applied_style != style)
			{
				restore_everything();
				g_applied_style = style;
			}

			walk_scene();

			if (g_clusters.empty())
			{
				// Nothing in the scene: every cluster this feature wrote belongs to
				// something the client has taken down, so it goes back - but only
				// where the write is still this feature's.
				restore_everything();
				return;
			}

			// The queue: the engine resets it, so it is written every pass, and a
			// cluster whose queue cannot be written is not touched at all.
			std::vector<math::vector3> local_a, other_a;
			const bool have_local = skip_local && local_anchors(local_a);
			if (skip_local)
				other_anchors(other_a);

			std::unordered_set<uptr> live;
			for (uptr cluster : g_clusters)
			{
				if (have_local && is_local_cluster(cluster, local_a, other_a))
					continue;                 // the local rig is not this feature's business

				const uptr rq = cluster + Offsets::FastClusterEntity::RenderQueueId;
				if (!writable(rq, sizeof(std::uint32_t)))
					continue;

				memory->write<std::uint32_t>(rq, style_queue(style));
				live.insert(cluster);
			}

			// The blocks, and then the clusters that vanished: a cluster the
			// client has stopped drawing has its material fields put back, so
			// nothing is left holding a program the client is about to free.
			for (const record_t& r : g_records)
			{
				if (!live.count(r.cluster))
					continue;
				apply_block(r.cluster, r.block, r.program, family, cp, style);
			}

			std::vector<uptr> gone;
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				gone.reserve(g_ent_blocks.size());
				for (const auto& kv : g_ent_blocks)
					if (!live.count(kv.first))
						gone.push_back(kv.first);
			}
			for (uptr ent : gone)
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				restore_blocks_locked(ent);
			}

			{
				std::lock_guard<std::mutex> lk(g_mtx);
				g_published = g_tick;
			}

			static auto s_log = std::chrono::steady_clock::now() - std::chrono::seconds(10);
			const auto now = std::chrono::steady_clock::now();
			if (now - s_log > std::chrono::seconds(10))
			{
				s_log = now;
				debug::log("chams",
					"scene=%d route(s) entities=%d checked=%d skipped=%d tinted=%d swapped=%d unknown=%d no_target=%d not_ready=%d style=%d",
					g_tick.routes, g_tick.entities, g_tick.checked, g_tick.skipped, g_tick.tinted,
					g_tick.swapped, g_tick.unknown, g_tick.no_target, g_tick.not_ready, style);
			}
		}

		void loop()
		{
			bool was_on = false;
			std::uint64_t dm_seen = 0;

			while (g_run.load(std::memory_order_relaxed))
			{
				const bool on = feature_on();

				if (on)
				{
					if (!was_on)
					{
						{
							std::lock_guard<std::mutex> lk(g_mtx);
							g_blocks.clear();
							g_ent_blocks.clear();
						}
						forget_manager();
						forget_routes();
						g_applied_style = -1;
						was_on = true;
						debug::log("chams", "chams on");
					}

					g_base = memory->get_module_address();
					if (g_base)
					{
						g_vt  = g_base + Offsets::FastClusterEntity::VTableRva;
						g_vt2 = 0;   // the measured vtable is the one this build has
					}

					if (game::datamodel.address && game::datamodel.address != dm_seen)
					{
						// A rejoin rebuilds the materials and the shader manager,
						// so nothing saved survives it: the originals go back only
						// where they are still this feature's write, and the list
						// and the scene are read again.
						if (dm_seen) restore_everything();
						forget_manager();
						forget_routes();
						dm_seen = game::datamodel.address;
						debug::log("chams", "datamodel changed - restored, re-reading the shader list and the scene");
					}

					resolve_routes();
					build_manager();
					tick_once();
				}
				else if (was_on)
				{
					restore_everything();
					forget_manager();
					forget_routes();
					publish_off();
					was_on = false;
					debug::log("chams", "chams off - materials restored");
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(on ? 15 : 200));
			}

			if (was_on)
			{
				restore_everything();
				publish_off();
			}
		}
	}

	void start()
	{
		if (g_run.exchange(true)) return;
		g_thread = std::thread(loop);
	}

	void stop()
	{
		if (!g_run.exchange(false)) return;
		if (g_thread.joinable()) g_thread.join();
	}

	void restore_all()
	{
		restore_everything();
	}

	bool active()
	{
		return g_run.load() && feature_on();
	}

	int style_count()
	{
		return k_family_count();
	}

	const char* style_name(int style)
	{
		if (style < 0) style = 0;
		if (style >= k_family_count()) style = k_family_count() - 1;
		return k_style_names[style];
	}

	const char* family_name(int style)
	{
		return family_for_style(style);
	}

	int style_reach(int style)
	{
		if (style < 0 || style >= k_max_styles) return 0;
		std::lock_guard<std::mutex> lk(g_mtx);
		return g_published.reach[style];
	}

	int style_swapped(int style)
	{
		if (style < 0 || style >= k_max_styles) return 0;
		std::lock_guard<std::mutex> lk(g_mtx);
		return g_published.swapped_of[style];
	}

	int programs_loaded()
	{
		return (int)g_manager.programs;
	}

	int routes_loaded()
	{
		return (int)g_routes.size();
	}

	stats_t stats()
	{
		stats_t s{};
		std::lock_guard<std::mutex> lk(g_mtx);
		s.programs = (int)g_manager.programs;
		s.families = (int)g_manager.by_family.size();
		s.routes   = (int)g_routes.size();
		s.entities = g_published.entities;
		s.checked  = g_published.checked;
		s.skipped  = g_published.skipped;
		s.tinted   = g_published.tinted;
		s.swapped  = g_published.swapped;
		s.unknown  = g_published.unknown;
		s.no_target = g_published.no_target;
		s.not_ready = g_published.not_ready;
		s.sample_block   = (unsigned long long)g_published.sample_block;
		s.sample_program = (unsigned long long)g_published.sample_program;
		return s;
	}
}
