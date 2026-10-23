#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "RaycastSilent.h"

#include <memory/memory.h>
#include <sdk/offsets.h>
#include <debug/DebugConsole.h>

#include <Windows.h>
#include <vector>
#include <cstring>
#include <chrono>
#include <cstddef>

#ifndef CFG_CALL_TARGET_VALID
#define CFG_CALL_TARGET_VALID 0x00000001
#endif

namespace raycast_silent
{
	namespace
	{
		constexpr std::uintptr_t desc_rva      = Offsets::Raycast::RaycastBoundDesc;
		constexpr std::uintptr_t bound_fn_off  = Offsets::Raycast::RaycastBoundFn;
		constexpr std::size_t    k_stub_bytes  = 0x200;

#pragma pack(push, 4)
		struct RaycastState
		{
			std::uint32_t active   = 0;
			std::uint32_t reserved = 0;
			float         target_x = 0.f;
			float         target_y = 0.f;
			float         target_z = 0.f;
			float         scale    = 1.f;
			std::uint64_t calls    = 0;
		};
#pragma pack(pop)

		static_assert(offsetof(RaycastState, active)   == 0x00, "active");
		static_assert(offsetof(RaycastState, target_x) == 0x08, "target");
		static_assert(offsetof(RaycastState, calls)    == 0x18, "calls");

		struct Hook
		{
			std::uintptr_t thunk       = 0;
			std::uintptr_t state       = 0;
			std::uintptr_t original    = 0;
			std::uintptr_t module_base = 0;
			bool           thunk_owned = false;
			bool           installed   = false;
			bool           active      = false;
		};

		Hook g_hook{};
		std::chrono::steady_clock::time_point g_last_fail{};

		bool ok_addr(std::uintptr_t a)
		{
			return a >= 0x10000ull && a < 0x00007FFFFFFFFFFFull;
		}

		bool w_mem(std::uintptr_t a, const void* d, std::size_t s)
		{
			if (!ok_addr(a) || !d || !s || !memory->get_process_handle())
				return false;
			return memory->write_raw(a, d, s) == s;
		}

		std::size_t pg()
		{
			static const std::size_t p = [] {
				SYSTEM_INFO i{};
				GetSystemInfo(&i);
				return i.dwPageSize ? static_cast<std::size_t>(i.dwPageSize) : 0x1000u;
			}();
			return p;
		}

		DWORD query_protect(std::uintptr_t a)
		{
			MEMORY_BASIC_INFORMATION mbi{};
			if (!VirtualQueryEx(memory->get_process_handle(),
				reinterpret_cast<void*>(a), &mbi, sizeof(mbi)))
				return 0;
			return mbi.Protect;
		}

		bool is_executable_protect(DWORD p)
		{
			const DWORD x = p & 0xFF;
			return x == PAGE_EXECUTE || x == PAGE_EXECUTE_READ ||
			       x == PAGE_EXECUTE_READWRITE || x == PAGE_EXECUTE_WRITECOPY;
		}

		bool protect_remote(std::uintptr_t address, std::size_t size, DWORD protection,
		                    DWORD* old_protect = nullptr)
		{
			if (!ok_addr(address) || !size || !memory->get_process_handle())
				return false;

			const std::uintptr_t page_mask = ~(static_cast<std::uintptr_t>(pg()) - 1);
			const std::uintptr_t base = address & page_mask;
			const std::uintptr_t end  = (address + size + pg() - 1) & page_mask;
			const std::size_t    span = static_cast<std::size_t>(end - base);

			using NtProtectFn = LONG(WINAPI*)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
			static NtProtectFn nt_protect = []() -> NtProtectFn {
				HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
				if (!ntdll) return nullptr;
				return reinterpret_cast<NtProtectFn>(
					GetProcAddress(ntdll, "NtProtectVirtualMemory"));
			}();

			auto try_one = [&](DWORD req) -> bool {
				DWORD old = 0;
				if (VirtualProtectEx(memory->get_process_handle(),
					reinterpret_cast<void*>(base), span, req, &old)) {
					if (old_protect) *old_protect = old;
					return true;
				}
				if (!nt_protect) return false;
				PVOID  nt_base = reinterpret_cast<void*>(base);
				SIZE_T nt_size = span;
				ULONG  nt_old  = 0;
				const LONG st = nt_protect(
					memory->get_process_handle(), &nt_base, &nt_size, req, &nt_old);
				if (st >= 0) {
					if (old_protect) *old_protect = static_cast<DWORD>(nt_old);
					return true;
				}
				return false;
			};

			if (try_one(protection)) return true;
			if (protection == PAGE_EXECUTE_READWRITE && try_one(PAGE_EXECUTE_WRITECOPY))
				return true;
			if (protection == PAGE_READWRITE && try_one(PAGE_WRITECOPY))
				return true;
			return false;
		}

		bool write_protected(std::uintptr_t address, const void* data, std::size_t size)
		{
			if (!ok_addr(address) || !data || !size) return false;
			DWORD old = 0;
			const bool changed = protect_remote(address, size, PAGE_EXECUTE_READWRITE, &old);
			const bool wrote = w_mem(address, data, size);
			if (changed)
				protect_remote(address, size, old, nullptr);
			return wrote;
		}

		bool mark_cfg(std::uintptr_t t)
		{
			auto resolve = []() -> FARPROC {
				const char* mods[] = {
					"kernelbase.dll", "kernel32.dll",
					"api-ms-win-core-memory-l1-1-3.dll"
				};
				for (auto* m : mods) {
					HMODULE h = GetModuleHandleA(m);
					if (!h) h = LoadLibraryA(m);
					if (!h) continue;
					if (auto p = GetProcAddress(h, "SetProcessValidCallTargets"))
						return p;
				}
				return nullptr;
			};

			auto proc = resolve();
			if (!proc) return false;

			struct Info { ULONG_PTR Offset; ULONG Flags; } info{};
			info.Offset = t & (pg() - 1);
			info.Flags  = CFG_CALL_TARGET_VALID;

			using Fn = BOOL(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, void*);
			SetLastError(0);
			const BOOL ok = reinterpret_cast<Fn>(proc)(
				memory->get_process_handle(),
				reinterpret_cast<void*>(t & ~(static_cast<std::uintptr_t>(pg()) - 1)),
				pg(), 1, &info);
			return ok != 0;
		}

		void append_u64(std::vector<std::uint8_t>& c, std::uint64_t v)
		{
			const auto* b = reinterpret_cast<const std::uint8_t*>(&v);
			c.insert(c.end(), b, b + 8);
		}

		void patch_rel32(std::vector<std::uint8_t>& c, std::size_t o, std::size_t t)
		{
			const std::int32_t v = static_cast<std::int32_t>(
				static_cast<std::ptrdiff_t>(t) - static_cast<std::ptrdiff_t>(o + 4));
			std::memcpy(c.data() + o, &v, 4);
		}

		// The engine hands WorldRoot::RaycastBound the origin in r8 and the
		// direction in r9. The stub swaps that direction for the one from the
		// origin to our target, then tail-calls the original bound function so
		// the shot, not the camera, is redirected.
		std::vector<std::uint8_t> make_hook_thunk(std::uintptr_t state, std::uintptr_t orig)
		{
			std::vector<std::uint8_t> c;
			c.reserve(320);
			std::vector<std::size_t> inactive;

			auto je_inactive = [&] {
				c.insert(c.end(), { 0x0F, 0x84 });
				inactive.push_back(c.size());
				c.insert(c.end(), { 0, 0, 0, 0 });
			};
			auto jbe_inactive = [&] {
				c.insert(c.end(), { 0x0F, 0x86 });
				inactive.push_back(c.size());
				c.insert(c.end(), { 0, 0, 0, 0 });
			};

			c.insert(c.end(), { 0x48, 0x83, 0xEC, 0x68 });          // sub rsp, 0x68
			c.insert(c.end(), { 0x49, 0xBA });                      // mov r10, state
			append_u64(c, state);
			c.insert(c.end(), { 0x41, 0x83, 0x3A, 0x00 });          // cmp dword [r10], 0
			je_inactive();
			c.insert(c.end(), { 0x4D, 0x85, 0xC0 }); je_inactive();  // test r8, r8
			c.insert(c.end(), { 0x4D, 0x85, 0xC9 }); je_inactive();  // test r9, r9

			// delta = target - origin
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x10, 0x42, 0x08 });  // movss xmm0, [r10+8]
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x5C, 0x00 });        // subss xmm0, [r8]
			c.insert(c.end(), { 0xF3, 0x0F, 0x11, 0x44, 0x24, 0x40 });  // movss [rsp+40], xmm0
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x10, 0x4A, 0x0C });  // movss xmm1, [r10+12]
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x5C, 0x48, 0x04 });  // subss xmm1, [r8+4]
			c.insert(c.end(), { 0xF3, 0x0F, 0x11, 0x4C, 0x24, 0x44 });
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x10, 0x52, 0x10 });  // movss xmm2, [r10+16]
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x5C, 0x50, 0x08 });  // subss xmm2, [r8+8]
			c.insert(c.end(), { 0xF3, 0x0F, 0x11, 0x54, 0x24, 0x48 });

			// |delta|
			c.insert(c.end(), { 0x0F, 0x28, 0xD8 });                    // movaps xmm3, xmm0
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xDB });              // mulss xmm3, xmm3
			c.insert(c.end(), { 0x0F, 0x28, 0xE1 });                    // movaps xmm4, xmm1
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xE4 });
			c.insert(c.end(), { 0xF3, 0x0F, 0x58, 0xDC });
			c.insert(c.end(), { 0x0F, 0x28, 0xE2 });                    // movaps xmm4, xmm2
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xE4 });
			c.insert(c.end(), { 0xF3, 0x0F, 0x58, 0xDC });
			c.insert(c.end(), { 0xF3, 0x0F, 0x51, 0xDB });              // sqrtss xmm3, xmm3
			c.insert(c.end(), { 0x0F, 0x57, 0xED });                    // xorps xmm5, xmm5
			c.insert(c.end(), { 0x0F, 0x2E, 0xDD });                    // ucomiss xmm3, xmm5
			jbe_inactive();

			// |ray dir|
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x10, 0x21 });        // movss xmm4, [r9]
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xE4 });
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x10, 0x69, 0x04 });  // movss xmm5, [r9+4]
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xED });
			c.insert(c.end(), { 0xF3, 0x0F, 0x58, 0xE5 });
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x10, 0x69, 0x08 });  // movss xmm5, [r9+8]
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xED });
			c.insert(c.end(), { 0xF3, 0x0F, 0x58, 0xE5 });
			c.insert(c.end(), { 0xF3, 0x0F, 0x51, 0xE4 });              // sqrtss xmm4, xmm4
			c.insert(c.end(), { 0x0F, 0x57, 0xED });
			c.insert(c.end(), { 0x0F, 0x2E, 0xE5 });                    // ucomiss xmm4, xmm5
			jbe_inactive();

			// dir = normalize(delta) * |ray dir|
			c.insert(c.end(), { 0x0F, 0x28, 0xEC });                    // movaps xmm5, xmm4
			c.insert(c.end(), { 0xF3, 0x0F, 0x5E, 0xEB });              // divss xmm5, xmm3
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xC5 });              // mulss xmm0, xmm5
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xCD });              // mulss xmm1, xmm5
			c.insert(c.end(), { 0xF3, 0x0F, 0x59, 0xD5 });              // mulss xmm2, xmm5
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x11, 0x01 });        // movss [r9], xmm0
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x11, 0x49, 0x04 });  // movss [r9+4], xmm1
			c.insert(c.end(), { 0xF3, 0x41, 0x0F, 0x11, 0x51, 0x08 });  // movss [r9+8], xmm2
			c.insert(c.end(), { 0x49, 0xFF, 0x42, 0x18 });              // inc qword [r10+0x18]

			const std::size_t inactive_off = c.size();
			for (auto o : inactive)
				patch_rel32(c, o, inactive_off);

			c.insert(c.end(), { 0x48, 0x8B, 0x84, 0x24, 0x90, 0x00, 0x00, 0x00 });
			c.insert(c.end(), { 0x48, 0x89, 0x44, 0x24, 0x20 });
			c.insert(c.end(), { 0x48, 0xB8 });                          // mov rax, orig
			append_u64(c, orig);
			c.insert(c.end(), { 0xFF, 0xD0 });                          // call rax
			c.insert(c.end(), { 0x48, 0x83, 0xC4, 0x68 });              // add rsp, 0x68
			c.push_back(0xC3);                                          // ret
			return c;
		}

		bool region_is_padding(std::uintptr_t a, std::size_t n)
		{
			std::vector<std::uint8_t> buf(n);
			if (memory->read_raw(a, buf.data(), n) != n) return false;
			for (auto b : buf)
				if (b != 0xCC && b != 0x00 && b != 0x90)
					return false;
			return true;
		}

		bool read_val(std::uintptr_t a, void* d, std::size_t s)
		{
			return memory->read_raw(a, d, s) == s;
		}

		// Look for a run of padding inside an executable section of a loaded
		// module and hand back an aligned slot big enough for the stub.
		std::uintptr_t find_cave_in_module(std::uintptr_t module_base, std::size_t need,
		                                   std::uintptr_t min_offset, std::uintptr_t ignore)
		{
			if (!ok_addr(module_base) || !need) return 0;

			IMAGE_DOS_HEADER dos{};
			if (!read_val(module_base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE)
				return 0;

			IMAGE_NT_HEADERS64 nt{};
			const std::uintptr_t nt_addr = module_base + static_cast<std::uintptr_t>(dos.e_lfanew);
			if (!read_val(nt_addr, &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE)
				return 0;

			const std::uintptr_t section_base = nt_addr + offsetof(IMAGE_NT_HEADERS64, OptionalHeader)
				+ nt.FileHeader.SizeOfOptionalHeader;

			for (WORD i = 0; i < nt.FileHeader.NumberOfSections; ++i)
			{
				IMAGE_SECTION_HEADER section{};
				if (!read_val(section_base + static_cast<std::uintptr_t>(i) * sizeof(section),
				              &section, sizeof(section)))
					break;

				if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
					continue;

				const std::uintptr_t section_off = section.VirtualAddress;
				std::size_t section_size = section.Misc.VirtualSize
					? section.Misc.VirtualSize : section.SizeOfRawData;
				if (!section_off || section_size < need) continue;

				std::uintptr_t scan_off = section_off;
				if (scan_off < min_offset) scan_off = min_offset;
				if (scan_off >= section_off + section_size) continue;

				const std::uintptr_t scan_start = module_base + scan_off;
				const std::size_t scan_size = static_cast<std::size_t>(
					section_off + section_size - scan_off);

				std::vector<std::uint8_t> buf(scan_size);
				if (!read_val(scan_start, buf.data(), buf.size())) continue;

				std::size_t run_start = 0;
				std::size_t run_len = 0;
				for (std::size_t j = 0; j < buf.size(); ++j)
				{
					const std::uint8_t b = buf[j];
					if (b != 0x00 && b != 0xCC && b != 0x90) {
						run_len = 0;
						run_start = j + 1;
						continue;
					}
					++run_len;
					if (run_len < need) continue;

					const std::uintptr_t cand = scan_start + run_start;
					const std::uintptr_t aligned =
						(cand + 0x0F) & ~static_cast<std::uintptr_t>(0x0F);
					const std::size_t loss = static_cast<std::size_t>(aligned - cand);
					if (run_len < need + loss) continue;
					if (ignore && aligned == ignore) continue;
					if (g_hook.thunk && aligned == g_hook.thunk) continue;
					return aligned;
				}
			}
			return 0;
		}

		std::uintptr_t find_exec_cave(std::size_t need, std::uintptr_t ignore = 0)
		{
			static const wchar_t* k_pref[] = {
				L"winsta.dll", L"win32u.dll", L"uxtheme.dll", L"dwmapi.dll",
				L"msctf.dll", L"TextInputFramework.dll", L"CoreMessaging.dll",
				L"user32.dll",
			};

			for (std::size_t mi = 0; mi < sizeof(k_pref) / sizeof(k_pref[0]); ++mi)
			{
				const wchar_t* name = k_pref[mi];
				std::string narrow(name, name + wcslen(name));
				const std::uintptr_t mod = memory->find_module_address(narrow);
				if (!mod) continue;
				const std::uintptr_t min_off = (mi == 0) ? 0x2000u : 0x1000u;
				const std::uintptr_t cave = find_cave_in_module(mod, need, min_off, ignore);
				if (cave) return cave;
			}

			MEMORY_BASIC_INFORMATION mbi{};
			std::uintptr_t addr = 0;
			std::uintptr_t fallback_rwx = 0;

			while (VirtualQueryEx(memory->get_process_handle(),
				reinterpret_cast<void*>(addr), &mbi, sizeof(mbi)))
			{
				const auto base = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
				const auto size = static_cast<std::size_t>(mbi.RegionSize);
				addr = base + size;
				if (addr < base) break;

				if (mbi.State != MEM_COMMIT) continue;
				if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) continue;
				if (!is_executable_protect(mbi.Protect)) continue;
				if (size < need) continue;
				if (g_hook.thunk && base <= g_hook.thunk && g_hook.thunk < addr) continue;

				for (std::size_t off = 0; off + need <= size; off += 0x10)
				{
					const std::uintptr_t cand = base + off;
					if (ignore && cand == ignore) continue;
					if (region_is_padding(cand, need))
						return cand;
				}

				if (!fallback_rwx && mbi.Type == MEM_PRIVATE &&
					(mbi.Protect & 0xFF) == PAGE_EXECUTE_READWRITE &&
					size >= need + 0x40)
				{
					const std::uintptr_t cand = base + size - need;
					if (!ignore || cand != ignore)
						fallback_rwx = cand;
				}
			}

			return fallback_rwx;
		}

		std::uintptr_t alloc_exec_page()
		{
			const std::uintptr_t p = memory->alloc(pg(), PAGE_EXECUTE_READWRITE);
			if (!p) return 0;
			if (!is_executable_protect(query_protect(p))) {
				memory->free(p);
				return 0;
			}
			return p;
		}
	}

	bool ready()  { return g_hook.installed; }
	bool aiming() { return g_hook.active; }

	bool install()
	{
		if (g_hook.installed) return true;

		const std::uintptr_t base = memory->find_module_address("RobloxPlayerBeta.exe");
		if (!base) return false;

		const auto now = std::chrono::steady_clock::now();
		if (g_last_fail.time_since_epoch().count() != 0 &&
			now - g_last_fail < std::chrono::milliseconds(1500))
			return false;

		const std::uintptr_t slot = base + desc_rva + bound_fn_off;
		const std::uintptr_t fn   = memory->read<std::uintptr_t>(slot);
		if (!ok_addr(fn) || !is_executable_protect(query_protect(fn)))
		{
			g_last_fail = now;
			return false;
		}

		if (!g_hook.state) g_hook.state = memory->alloc(pg(), PAGE_READWRITE);
		if (!g_hook.state) { g_last_fail = now; return false; }

		const std::vector<std::uint8_t> thunk = make_hook_thunk(g_hook.state, fn);
		if (thunk.size() > k_stub_bytes) { g_last_fail = now; return false; }

		bool owned = false;
		std::uintptr_t stub = 0;
		std::uintptr_t ignore = 0;

		for (int attempt = 0; attempt < 8 && !stub; ++attempt)
		{
			const std::uintptr_t cand = find_exec_cave(k_stub_bytes, ignore);
			if (!cand) break;

			DWORD old_prot = 0;
			const bool prot_ok = protect_remote(cand, thunk.size(), PAGE_EXECUTE_READWRITE, &old_prot);
			if (!write_protected(cand, thunk.data(), thunk.size())) {
				if (prot_ok) protect_remote(cand, thunk.size(), old_prot, nullptr);
				ignore = cand;
				continue;
			}
			stub = cand;
			owned = false;
		}

		if (!stub) {
			stub = alloc_exec_page();
			owned = stub != 0;
			if (stub && !write_protected(stub, thunk.data(), thunk.size())) {
				memory->free(stub);
				stub = 0;
				owned = false;
			}
		}

		if (!stub) { g_last_fail = now; return false; }

		RaycastState empty{};
		if (!w_mem(g_hook.state, &empty, sizeof(empty))) {
			if (owned) memory->free(stub);
			g_last_fail = now;
			return false;
		}

		FlushInstructionCache(memory->get_process_handle(),
			reinterpret_cast<void*>(stub), thunk.size());
		mark_cfg(stub);

		if (!is_executable_protect(query_protect(stub))) {
			if (owned) memory->free(stub);
			g_last_fail = now;
			return false;
		}

		protect_remote(slot, 8, PAGE_READWRITE, nullptr);
		if (!write_protected(slot, &stub, sizeof(stub)) ||
			memory->read<std::uintptr_t>(slot) != stub)
		{
			if (owned) memory->free(stub);
			g_last_fail = now;
			return false;
		}

		g_hook.module_base = base;
		g_hook.original    = fn;
		g_hook.thunk       = stub;
		g_hook.thunk_owned = owned;
		g_hook.installed   = true;
		g_hook.active      = false;
		debug::ok("raycast hook installed (slot 0x%llx)", (unsigned long long)slot);
		return true;
	}

	void remove()
	{
		if (g_hook.installed && ok_addr(g_hook.original) && g_hook.module_base)
		{
			const std::uintptr_t slot = g_hook.module_base + desc_rva + bound_fn_off;
			write_protected(slot, &g_hook.original, sizeof(g_hook.original));
		}

		if (g_hook.thunk && !g_hook.thunk_owned)
		{
			std::vector<std::uint8_t> pad(k_stub_bytes, 0xCC);
			write_protected(g_hook.thunk, pad.data(), pad.size());
		}

		if (g_hook.thunk && g_hook.thunk_owned) memory->free(g_hook.thunk);
		if (g_hook.state) memory->free(g_hook.state);
		g_hook = {};
	}

	void ensure(bool want)
	{
		const std::uintptr_t base = memory->find_module_address("RobloxPlayerBeta.exe");
		static std::uintptr_t s_last_base = 0;

		if (g_hook.installed && base && s_last_base && base != s_last_base)
			remove();
		if (base) s_last_base = base;

		if (want) {
			if (!base) return;
			if (!g_hook.installed) install();
		} else if (g_hook.installed) {
			remove();
		}
	}

	void set_active(bool on, const math::vector3& world_target)
	{
		if (!on)
		{
			if (g_hook.active && g_hook.state)
			{
				std::uint32_t v = 0;
				w_mem(g_hook.state, &v, sizeof(v));
				g_hook.active = false;
			}
			return;
		}

		if (!g_hook.installed) return;

		RaycastState st{};
		st.active   = 1;
		st.target_x = world_target.x;
		st.target_y = world_target.y;
		st.target_z = world_target.z;
		st.scale    = 1.f;
		if (!w_mem(g_hook.state, &st, sizeof(st)))
			return;
		g_hook.active = true;
	}
}
