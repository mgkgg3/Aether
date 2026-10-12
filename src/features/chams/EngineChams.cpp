#define NOMINMAX
#include "EngineChams.h"

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <settings.h>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <cache/cache.h>
#include <game/game.h>
#include <debug/DebugConsole.h>

namespace engine_chams
{
	namespace
	{
		using uptr = std::uintptr_t;

		constexpr float k_rig_reach_sq = 6.f * 6.f;

		constexpr uptr k_alt_vtable_rva = 0x6D5CE38ull;

		struct layer_backup_t
		{
			std::uint8_t  fillmode = 0;
			std::uint32_t matflags = 0;
			std::uint32_t param = 0;
			std::uint32_t flags2 = 0;
			std::uint32_t color = 0;
		};

		std::mutex g_mtx;
		std::unordered_map<uptr, std::uint32_t>  g_saved;
		std::unordered_map<uptr, std::uint8_t>   g_alpha;
		std::unordered_map<uptr, layer_backup_t> g_layers;
		std::unordered_map<uptr, std::vector<uptr>> g_ent_layers;
		std::unordered_map<uptr, int>            g_miss;

		uptr g_vt  = 0;
		uptr g_vt2 = 0;
		int  g_applied_style = -1;

		std::thread       g_thread;
		std::atomic<bool> g_run{ false };

		bool valid(uptr a)
		{
			return a >= 0x10000ull && a < 0x7fffffffffffull && (a & 3ull) == 0ull;
		}

		bool vt_match(uptr ent)
		{
			const uptr v = memory->read<uptr>(ent);
			if (!v) return false;
			return v == g_vt || (g_vt2 != 0 && v == g_vt2);
		}

		bool writable(uptr a, std::size_t size)
		{
			if (!valid(a)) return false;

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

		bool ent_alive(uptr ent)
		{
			if (!g_vt || !valid(ent) || !valid(ent + 8)) return false;
			if (!vt_match(ent)) return false;
			return writable(ent + Offsets::FastClusterEntity::RenderQueueId, sizeof(std::uint32_t));
		}

		bool ent_known(uptr ent)
		{
			if (!g_vt || !valid(ent)) return false;
			return vt_match(ent);
		}

		bool layer_alive(uptr layer)
		{
			return writable(layer + Offsets::MaterialLayer::FillModeByte, 1) &&
				writable(layer + Offsets::MaterialLayer::ColorData, sizeof(std::uint32_t));
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
			namespace RQ = Offsets::RenderQueue;
			switch (style)
			{
			case 5: return RQ::Glass;
			case 6: return RQ::GlassTint;
			case 7: return RQ::Transparent;
			case 8: return RQ::OnTopWithDepth;
			default: return RQ::AlwaysOnTop;
			}
		}

		void drop_layers_locked(uptr ent)
		{
			auto lit = g_ent_layers.find(ent);
			if (lit == g_ent_layers.end()) return;
			for (uptr layer : lit->second) g_layers.erase(layer);
			g_ent_layers.erase(lit);
		}

		void restore_layers_locked(uptr ent)
		{
			auto lit = g_ent_layers.find(ent);
			if (lit == g_ent_layers.end()) return;

			const bool alive = ent_alive(ent);
			for (uptr layer : lit->second)
			{
				auto bit = g_layers.find(layer);
				if (bit == g_layers.end()) continue;

				if (alive && layer_alive(layer))
				{
					layer_backup_t& b = bit->second;
					memory->write<std::uint8_t>(layer + Offsets::MaterialLayer::FillModeByte, b.fillmode);
					if (writable(layer + Offsets::MaterialLayer::MatFlags, 4))
						memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::MatFlags, b.matflags);
					if (writable(layer + Offsets::MaterialLayer::Param, 4))
						memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::Param, b.param);
					if (writable(layer + Offsets::MaterialLayer::Flags2, 4))
						memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::Flags2, b.flags2);
					memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::ColorData, b.color);
				}

				g_layers.erase(bit);
			}
			g_ent_layers.erase(lit);
		}

		void drop_entity_locked(uptr ent)
		{
			auto sit = g_saved.find(ent);
			if (sit != g_saved.end() && ent_alive(ent))
				memory->write<std::uint32_t>(ent + Offsets::FastClusterEntity::RenderQueueId, sit->second);

			auto ait = g_alpha.find(ent);
			if (ait != g_alpha.end() &&
				writable(ent + Offsets::FastClusterEntity::AlphaByte, 1))
				memory->write<std::uint8_t>(ent + Offsets::FastClusterEntity::AlphaByte, ait->second);

			restore_layers_locked(ent);

			g_saved.erase(ent);
			g_alpha.erase(ent);
			g_miss.erase(ent);
		}

		void apply_layers_real(uptr ent, std::uint8_t fill, std::uint32_t param,
			std::uint32_t flags2, std::uint32_t color)
		{
			if (!ent_alive(ent)) return;

			uptr arr = memory->read<uptr>(ent + Offsets::FastClusterEntity::TechniqueArrayPtr);
			if (!valid(arr)) return;

			uptr begin = memory->read<uptr>(arr + Offsets::TechniqueArray::BeginOffset);
			uptr end   = memory->read<uptr>(arr + Offsets::TechniqueArray::EndOffset);
			if (!valid(begin) || end <= begin) return;

			const std::size_t bytes = (std::size_t)(end - begin);
			if (bytes > 64ull * 1024ull) return;

			const std::size_t count = bytes / Offsets::MaterialLayer::Stride;
			if (count == 0 || count > 256) return;

			for (std::size_t i = 0; i < count; ++i)
			{
				uptr layer = begin + i * Offsets::MaterialLayer::Stride;

				if (memory->read<std::uint8_t>(layer + Offsets::MaterialLayer::FillModeByte) == fill &&
					memory->read<std::uint32_t>(layer + Offsets::MaterialLayer::Param) == param &&
					memory->read<std::uint32_t>(layer + Offsets::MaterialLayer::Flags2) == flags2 &&
					memory->read<std::uint32_t>(layer + Offsets::MaterialLayer::ColorData) == color)
					continue;

				if (!layer_alive(layer)) continue;

				{
					std::lock_guard<std::mutex> lk(g_mtx);
					if (!g_layers.count(layer))
					{
						layer_backup_t b{};
						b.fillmode = memory->read<std::uint8_t>(layer + Offsets::MaterialLayer::FillModeByte);
						b.matflags = memory->read<std::uint32_t>(layer + Offsets::MaterialLayer::MatFlags);
						b.param    = memory->read<std::uint32_t>(layer + Offsets::MaterialLayer::Param);
						b.flags2   = memory->read<std::uint32_t>(layer + Offsets::MaterialLayer::Flags2);
						b.color    = memory->read<std::uint32_t>(layer + Offsets::MaterialLayer::ColorData);
						g_layers[layer] = b;
						g_ent_layers[ent].push_back(layer);
					}
				}

				memory->write<std::uint8_t>(layer + Offsets::MaterialLayer::FillModeByte, fill);
				if (writable(layer + Offsets::MaterialLayer::MatFlags, 4))
					memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::MatFlags, 0);
				if (writable(layer + Offsets::MaterialLayer::Param, 4))
					memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::Param, param);
				if (writable(layer + Offsets::MaterialLayer::Flags2, 4))
					memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::Flags2, flags2);
				memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::ColorData, color);
			}
		}

		bool apply_style_layers(uptr ent, int style, int color_idx)
		{
			const std::uint32_t cp = color_param(color_idx);
			switch (style)
			{
			case 1: apply_layers_real(ent, 0, cp,              0u,  0xFFFFFFFFu); return true;
			case 2: apply_layers_real(ent, 1, color_param(6),  0u,  0xFFFFFFFFu); return true;
			case 3: apply_layers_real(ent, 0, cp,              15u, 0xFFFFFFFFu); return true;
			case 4: apply_layers_real(ent, 1, cp,              7u,  0xFFFFFFFFu); return true;
			case 5: apply_layers_real(ent, 0, cp,              15u, 0xFFFFFFFFu); return true;
			case 6: apply_layers_real(ent, 0, cp,              0u,  0xFFFFFFFFu); return true;
			case 7: apply_layers_real(ent, 0, cp,              0u,  0xFFFFFFFFu); return true;
			case 8: apply_layers_real(ent, 0, cp,              0u,  0xFFFFFFFFu); return true;
			case 9: apply_layers_real(ent, 1, cp,              15u, 0xFFFFFFFFu); return true;
			default: return false;
			}
		}

		bool style_uses_picker(int style)
		{
			return style >= 0 && style <= 9 && style != 2;
		}

		int effective_color_idx(int style)
		{
			const int idx = style_uses_picker(style)
				? nearest_color_idx(settings::visuals::engine_chams_color)
				: 6;
			return (idx < 0) ? 0 : (idx > 6 ? 6 : idx);
		}

		void restore_all_locked()
		{
			std::unordered_map<uptr, std::uint32_t>  saved;
			std::unordered_map<uptr, std::uint8_t>   alphas;
			std::unordered_map<uptr, std::vector<uptr>> ent_layers;
			std::unordered_map<uptr, layer_backup_t> layers;
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				saved.swap(g_saved);
				alphas.swap(g_alpha);
				ent_layers.swap(g_ent_layers);
				layers.swap(g_layers);
				g_miss.clear();
				g_applied_style = -1;
			}

			for (const auto& [ent, id] : saved)
			{
				if (!ent_alive(ent)) continue;
				memory->write<std::uint32_t>(ent + Offsets::FastClusterEntity::RenderQueueId, id);

				auto ait = alphas.find(ent);
				if (ait != alphas.end() &&
					writable(ent + Offsets::FastClusterEntity::AlphaByte, 1))
					memory->write<std::uint8_t>(ent + Offsets::FastClusterEntity::AlphaByte, ait->second);

				auto lit = ent_layers.find(ent);
				if (lit == ent_layers.end()) continue;

				for (uptr layer : lit->second)
				{
					auto bit = layers.find(layer);
					if (bit == layers.end() || !layer_alive(layer)) continue;

					layer_backup_t& b = bit->second;
					memory->write<std::uint8_t>(layer + Offsets::MaterialLayer::FillModeByte, b.fillmode);
					if (writable(layer + Offsets::MaterialLayer::MatFlags, 4))
						memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::MatFlags, b.matflags);
					if (writable(layer + Offsets::MaterialLayer::Param, 4))
						memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::Param, b.param);
					if (writable(layer + Offsets::MaterialLayer::Flags2, 4))
						memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::Flags2, b.flags2);
					memory->write<std::uint32_t>(layer + Offsets::MaterialLayer::ColorData, b.color);
				}
			}
		}

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

		bool is_local_entity(uptr ent, const std::vector<math::vector3>& local_a,
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

		void apply_entity(uptr ent, int style, int color_idx)
		{
			if (!ent_known(ent)) return;

			const uptr rq = ent + Offsets::FastClusterEntity::RenderQueueId;
			if (!writable(rq, sizeof(std::uint32_t))) return;

			{
				std::lock_guard<std::mutex> lk(g_mtx);
				if (!g_saved.count(ent))
				{
					g_saved[ent] = memory->read<std::uint32_t>(rq);
					g_alpha[ent] = memory->read<std::uint8_t>(
						ent + Offsets::FastClusterEntity::AlphaByte);
				}
			}

			memory->write<std::uint32_t>(rq, style_queue(style));

			if (apply_style_layers(ent, style, color_idx)) return;

			std::lock_guard<std::mutex> lk(g_mtx);
			if (g_ent_layers.count(ent)) restore_layers_locked(ent);
		}

		void refresh_known()
		{
			const int style     = settings::visuals::engine_chams_style;
			const int color_idx = effective_color_idx(style);

			std::vector<uptr> ents;
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				ents.reserve(g_saved.size());
				for (const auto& kv : g_saved) ents.push_back(kv.first);
			}

			if (g_applied_style >= 1 && g_applied_style <= 9 && (style < 1 || style > 9))
			{
				for (uptr ent : ents)
				{
					if (!ent_known(ent)) continue;
					std::lock_guard<std::mutex> lk(g_mtx);
					restore_layers_locked(ent);
				}
			}

			for (uptr ent : ents)
			{
				if (!ent_known(ent))
				{
					std::lock_guard<std::mutex> lk(g_mtx);
					int& n = g_miss[ent];
					if (++n >= 25)
					{
						drop_layers_locked(ent);
						g_saved.erase(ent);
						g_alpha.erase(ent);
						g_miss.erase(ent);
					}
					continue;
				}

				{
					std::lock_guard<std::mutex> lk(g_mtx);
					g_miss[ent] = 0;
				}

				apply_entity(ent, style, color_idx);
			}

			g_applied_style = style;
		}

		void scan_once(uptr vt)
		{
			HANDLE proc = memory ? memory->get_process_handle() : nullptr;
			if (!proc) return;

			g_vt = vt;

			const int style     = settings::visuals::engine_chams_style;
			const int color_idx = effective_color_idx(style);

			const bool skip_local = !settings::visuals::localplayer;

			std::vector<math::vector3> local_a, other_a;
			const bool have_local = local_anchors(local_a);
			other_anchors(other_a);

			int hits = 0, applied = 0, skipped_local = 0;

			SYSTEM_INFO si{};
			GetSystemInfo(&si);

			const uptr max_va = reinterpret_cast<uptr>(si.lpMaximumApplicationAddress);
			uptr addr = reinterpret_cast<uptr>(si.lpMinimumApplicationAddress);

			static std::vector<std::uint8_t> buf;
			constexpr SIZE_T k_chunk = 4u << 20;

			MEMORY_BASIC_INFORMATION mbi{};
			while (addr < max_va)
			{
				if (VirtualQueryEx(proc, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == 0)
					break;

				const bool readable =
					mbi.Protect == PAGE_READWRITE ||
					mbi.Protect == PAGE_EXECUTE_READWRITE ||
					mbi.Protect == PAGE_WRITECOPY ||
					mbi.Protect == PAGE_EXECUTE_WRITECOPY;

				const bool ok = mbi.State == MEM_COMMIT && mbi.Type != MEM_IMAGE && readable;

				if (ok && mbi.RegionSize > 0 &&
					mbi.RegionSize <= 512ull * 1024ull * 1024ull)
				{
					const uptr base = reinterpret_cast<uptr>(mbi.BaseAddress);
					SIZE_T left = mbi.RegionSize;

					while (left >= 16)
					{
						const SIZE_T want = left < k_chunk ? left : k_chunk;
						if (buf.size() < want) buf.resize(want);

						SIZE_T got = 0;
						if (!ReadProcessMemory(proc, reinterpret_cast<LPCVOID>(base + (mbi.RegionSize - left)),
								buf.data(), want, &got) || got < 16)
							break;

						for (SIZE_T i = 0; i + 16 <= got; i += 8)
						{
							const uptr cand = *reinterpret_cast<const uptr*>(buf.data() + i);
							if (cand != vt && (g_vt2 == 0 || cand != g_vt2)) continue;

							const uptr node = *reinterpret_cast<const uptr*>(buf.data() + i + 8);
							if (!valid(node)) continue;

							const uptr ent = base + (mbi.RegionSize - left) + i;

							++hits;

							if (skip_local && have_local && is_local_entity(ent, local_a, other_a))
							{
								++skipped_local;
								continue;
							}

							apply_entity(ent, style, color_idx);
							++applied;
						}

						left -= got;
						if (got < want) break;
					}
				}

				const uptr next = reinterpret_cast<uptr>(mbi.BaseAddress) + mbi.RegionSize;
				if (next <= addr) break;
				addr = next;
			}

			static auto s_log = std::chrono::steady_clock::now();
			const auto now = std::chrono::steady_clock::now();
			if (now - s_log > std::chrono::seconds(10))
			{
				s_log = now;
				debug::log("chams", "scan vt=0x%llx alt=0x%llx hits=%d applied=%d skipped_local=%d style=%d",
					(unsigned long long)vt, (unsigned long long)g_vt2, hits, applied, skipped_local, style);
			}
		}

		bool feature_on()
		{
			return settings::visuals::engine_chams && memory && memory->get_process_id() != 0;
		}

		void loop()
		{
			bool was_on = false;
			int  tick = 0;
			auto last_scan = std::chrono::steady_clock::now() - std::chrono::seconds(10);
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
							g_saved.clear();
							g_alpha.clear();
							g_layers.clear();
							g_ent_layers.clear();
							g_miss.clear();
							g_applied_style = -1;
						}
						was_on = true;
						tick = 0;
						last_scan = std::chrono::steady_clock::now() - std::chrono::seconds(10);
						debug::log("chams", "chams on");
					}

					if (game::datamodel.address && game::datamodel.address != dm_seen)
					{
						if (dm_seen) restore_all_locked();
						dm_seen = game::datamodel.address;
						debug::log("chams", "datamodel changed - restored and rescanning");
					}

					const uptr base = memory->get_module_address();
					if (base)
					{
						g_vt  = base + Offsets::FastClusterEntity::VTableRva;
						g_vt2 = base + k_alt_vtable_rva;
					}

					refresh_known();

					const auto now = std::chrono::steady_clock::now();
					if (base && (now - last_scan) > std::chrono::milliseconds(1500))
					{
						last_scan = now;
						scan_once(g_vt);
					}

					++tick;
				}
				else if (was_on)
				{
					restore_all_locked();
					was_on = false;
					tick = 0;
					debug::log("chams", "chams off - render queues restored");
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(on ? 15 : 200));
			}

			if (was_on)
				restore_all_locked();
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
		restore_all_locked();
	}

	bool active()
	{
		return g_run.load() && feature_on();
	}
}
