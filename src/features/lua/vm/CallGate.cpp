#include "pch.h"
#include "CallGate.h"
#include "Reflect.h"
#include "core/console/Console.h"
#include "core/memory/Memory.h"
#include "core/roblox/offsets/Offsets.h"

#include <cstring>
#include <string>
#include <vector>

namespace Cheat {
namespace Features {
namespace CallGate {
namespace {

constexpr std::uintptr_t st_pending = 0x00;
constexpr std::uintptr_t st_done    = 0x04;
constexpr std::uintptr_t st_fn      = 0x08;
constexpr std::uintptr_t st_a0      = 0x10;
constexpr std::uintptr_t st_a1      = 0x18;
constexpr std::uintptr_t st_a2      = 0x20;
constexpr std::uintptr_t st_a3      = 0x28;
constexpr std::uintptr_t st_ret     = 0x30;
constexpr std::uintptr_t st_calls   = 0x38;
constexpr std::uintptr_t st_tid     = 0x40;
constexpr std::uintptr_t st_scratch = 0x100;

constexpr std::size_t stub_bytes = 0x120;

constexpr std::size_t stub_orig_off = 0x1F;

struct Gate
{
	bool          installed = false;
	std::uintptr_t slot = 0;
	std::uintptr_t orig = 0;
	std::uintptr_t stub = 0;
	std::uintptr_t state = 0;
	bool          stub_is_cave = false;
	std::size_t   cand = 0;
	std::string   method;
};

const char* const k_candidates[] = {
	"IsA", "FindFirstChild", "GetChildren", "WaitForChild",
	"FindFirstChildOfClass", "GetDescendants", "GetAttribute", "Clone",
};

constexpr std::size_t k_candidate_count =
	sizeof(k_candidates) / sizeof(k_candidates[0]);

Gate g_gate;
int  g_last_fail = 0;

bool g_cold[k_candidate_count]{};

std::size_t page_sz()
{
	static std::size_t v = []() -> std::size_t {
		SYSTEM_INFO si{};
		GetSystemInfo(&si);
		return si.dwPageSize ? si.dwPageSize : 0x1000;
	}();
	return v;
}

bool is_exec_protect(DWORD p)
{
	const DWORD b = p & 0xFF;
	return b == PAGE_EXECUTE || b == PAGE_EXECUTE_READ ||
	       b == PAGE_EXECUTE_READWRITE || b == PAGE_EXECUTE_WRITECOPY;
}

DWORD query_protect(std::uintptr_t a)
{
	MEMORY_BASIC_INFORMATION mbi{};
	if (!VirtualQueryEx(g_Memory.GetHandle(), (void*)a, &mbi, sizeof(mbi)))
		return 0;
	if (mbi.State != MEM_COMMIT)
		return 0;
	return mbi.Protect;
}

bool write_protected(std::uintptr_t addr, const void* data, std::size_t size)
{
	if (!addr || !data || !size)
		return false;

	DWORD old = 0;
	const bool changed = g_Memory.Protect(addr, size, PAGE_EXECUTE_READWRITE, &old);
	const bool wrote = g_Memory.WriteRaw(addr, data, size) == size;
	if (changed)
		g_Memory.Protect(addr, size, old, nullptr);
	return wrote;
}

bool mark_cfg(std::uintptr_t t)
{
	HMODULE h = GetModuleHandleA("kernelbase.dll");
	if (!h)
		h = GetModuleHandleA("kernel32.dll");
	if (!h)
		return false;

	FARPROC proc = GetProcAddress(h, "SetProcessValidCallTargets");
	if (!proc)
		return false;

	struct Info { ULONG_PTR Offset; ULONG Flags; } info{};
	info.Offset = t & (page_sz() - 1);
	info.Flags = CFG_CALL_TARGET_VALID;

	using Fn = BOOL(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, void*);
	return ((Fn)proc)(g_Memory.GetHandle(),
		(void*)(t & ~((std::uintptr_t)page_sz() - 1)),
		page_sz(), 1, &info) != 0;
}

bool module_range(const wchar_t* name, std::uintptr_t* out_base, std::size_t* out_size)
{
	const std::uintptr_t base = g_Memory.GetModuleBase(name);
	if (!base)
		return false;

	const auto lfanew = g_Memory.Read<std::int32_t>(base + 0x3C);
	if (lfanew <= 0 || lfanew > 0x1000)
		return false;

	const auto img = g_Memory.Read<std::uint32_t>(base + lfanew + 0x50);
	if (img < 0x1000 || img > 0x20000000u)
		return false;

	*out_base = base;
	*out_size = img;
	return true;
}

template <typename Cb>
void walk_committed(std::uintptr_t from, std::uintptr_t to, bool want_exec, Cb cb)
{
	std::vector<std::uint8_t> buf;
	std::uintptr_t addr = from;
	MEMORY_BASIC_INFORMATION mbi{};

	while (addr < to &&
	       VirtualQueryEx(g_Memory.GetHandle(), (void*)addr, &mbi, sizeof(mbi)))
	{
		const auto rb = (std::uintptr_t)mbi.BaseAddress;
		const auto rs = (std::size_t)mbi.RegionSize;
		const std::uintptr_t next = rb + rs;
		if (next <= addr)
			break;

		const bool usable = mbi.State == MEM_COMMIT &&
			!(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
			is_exec_protect(mbi.Protect) == want_exec;
		if (usable)
		{
			const std::uintptr_t s = (rb > from) ? rb : from;
			const std::uintptr_t e = (next < to) ? next : to;
			if (e > s)
			{
				buf.resize((std::size_t)(e - s));
				if (g_Memory.ReadRaw(s, buf.data(), buf.size()) == buf.size())
					cb(s, buf);
			}
		}

		addr = next;
	}
}

std::uintptr_t find_bound_desc(std::uintptr_t base, std::size_t size,
                               std::uint64_t name)
{
	if (!name)
		return 0;

	std::uintptr_t found = 0;
	walk_committed(base, base + size, false,
		[&](std::uintptr_t at, const std::vector<std::uint8_t>& b)
		{
			if (found || b.size() < 16)
				return;

			for (std::size_t i = 8; i + 8 <= b.size(); i += 8)
			{
				std::uint64_t v = 0;
				std::memcpy(&v, b.data() + i, 8);
				if (v != name)
					continue;

				const std::uintptr_t desc = at + i - 8;

				const auto vt = g_Memory.Read<std::uint64_t>(desc);
				if (vt < base || vt >= base + size)
					continue;

				const auto fn = g_Memory.Read<std::uint64_t>(
					desc + Offsets::WorldRoot::RaycastBoundFn);
				if (fn && is_exec_protect(query_protect((std::uintptr_t)fn)))
				{
					found = desc;
					return;
				}
			}
		});

	return found;
}

std::uintptr_t find_exec_cave(std::size_t need)
{
	static const wchar_t* pref[] = {
		L"winsta.dll", L"win32u.dll", L"uxtheme.dll", L"dwmapi.dll",
		L"msctf.dll", L"TextInputFramework.dll", L"CoreMessaging.dll",
	};

	for (auto* name : pref)
	{
		std::uintptr_t mb = 0;
		std::size_t ms = 0;
		if (!module_range(name, &mb, &ms))
			continue;

		std::uintptr_t hit = 0;
		walk_committed(mb + 0x1000, mb + ms, true,
			[&](std::uintptr_t at, const std::vector<std::uint8_t>& b)
			{
				if (hit || b.size() < need)
					return;

				std::size_t run = 0;
				for (std::size_t i = 0; i < b.size(); ++i)
				{
					if (b[i] != 0xCC && b[i] != 0x00)
					{
						run = 0;
						continue;
					}

					if (++run < need + 0x10)
						continue;

					const std::uintptr_t start = at + i + 1 - run;
					hit = (start + 0x0F) & ~(std::uintptr_t)0x0F;
					return;
				}
			});

		if (hit)
			return hit;
	}

	return 0;
}

void emit(std::vector<std::uint8_t>& c, std::initializer_list<std::uint8_t> b)
{
	c.insert(c.end(), b);
}

void emit_u32(std::vector<std::uint8_t>& c, std::uint32_t v)
{
	const auto* b = (const std::uint8_t*)&v;
	c.insert(c.end(), b, b + 4);
}

void emit_u64(std::vector<std::uint8_t>& c, std::uint64_t v)
{
	const auto* b = (const std::uint8_t*)&v;
	c.insert(c.end(), b, b + 8);
}

std::vector<std::uint8_t> build_stub(std::uintptr_t state, std::uintptr_t orig)
{
	std::vector<std::uint8_t> c;

	emit(c, { 0x49, 0xBA });
	emit_u64(c, state);
	emit(c, { 0xF0, 0x49, 0xFF, 0x42, 0x38 });
	emit(c, { 0x41, 0x83, 0x3A, 0x00 });
	emit(c, { 0x0F, 0x85 });
	const std::size_t fix_slow = c.size();
	emit_u32(c, 0);

	const std::size_t pass = c.size();
	emit(c, { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 });
	emit_u64(c, orig);

	const std::size_t slow = c.size();

	emit(c, { 0x49, 0x8B, 0x42, 0x40 });
	emit(c, { 0x48, 0x85, 0xC0 });
	emit(c, { 0x74, 0x00 });
	const std::size_t fix_take = c.size() - 1;
	emit(c, { 0x65, 0x4C, 0x8B, 0x1C, 0x25 });
	emit_u32(c, 0x48);
	emit(c, { 0x4C, 0x39, 0xD8 });
	emit(c, { 0x0F, 0x85 });
	const std::size_t fix_pass1 = c.size();
	emit_u32(c, 0);

	const std::size_t take = c.size();
	emit(c, { 0xB8, 0x01, 0x00, 0x00, 0x00 });
	emit(c, { 0x45, 0x31, 0xDB });
	emit(c, { 0xF0, 0x45, 0x0F, 0xB1, 0x1A });
	emit(c, { 0x0F, 0x85 });
	const std::size_t fix_pass2 = c.size();
	emit_u32(c, 0);

	emit(c, { 0x48, 0x81, 0xEC });
	emit_u32(c, 0x88);
	emit(c, { 0x4C, 0x89, 0x94, 0x24 });
	emit_u32(c, 0x80);
	emit(c, { 0x48, 0x89, 0x4C, 0x24, 0x20 });
	emit(c, { 0x48, 0x89, 0x54, 0x24, 0x28 });
	emit(c, { 0x4C, 0x89, 0x44, 0x24, 0x30 });
	emit(c, { 0x4C, 0x89, 0x4C, 0x24, 0x38 });
	emit(c, { 0x0F, 0x11, 0x44, 0x24, 0x40 });
	emit(c, { 0x0F, 0x11, 0x4C, 0x24, 0x50 });
	emit(c, { 0x0F, 0x11, 0x54, 0x24, 0x60 });
	emit(c, { 0x0F, 0x11, 0x5C, 0x24, 0x70 });

	emit(c, { 0x49, 0x8B, 0x42, 0x08 });
	emit(c, { 0x49, 0x8B, 0x4A, 0x10 });
	emit(c, { 0x49, 0x8B, 0x52, 0x18 });
	emit(c, { 0x4D, 0x8B, 0x42, 0x20 });
	emit(c, { 0x4D, 0x8B, 0x4A, 0x28 });
	emit(c, { 0xFF, 0xD0 });

	emit(c, { 0x4C, 0x8B, 0x94, 0x24 });
	emit_u32(c, 0x80);
	emit(c, { 0x49, 0x89, 0x42, 0x30 });
	emit(c, { 0x41, 0xC7, 0x42, 0x04 });
	emit_u32(c, 1);

	emit(c, { 0x0F, 0x10, 0x44, 0x24, 0x40 });
	emit(c, { 0x0F, 0x10, 0x4C, 0x24, 0x50 });
	emit(c, { 0x0F, 0x10, 0x54, 0x24, 0x60 });
	emit(c, { 0x0F, 0x10, 0x5C, 0x24, 0x70 });
	emit(c, { 0x48, 0x8B, 0x4C, 0x24, 0x20 });
	emit(c, { 0x48, 0x8B, 0x54, 0x24, 0x28 });
	emit(c, { 0x4C, 0x8B, 0x44, 0x24, 0x30 });
	emit(c, { 0x4C, 0x8B, 0x4C, 0x24, 0x38 });
	emit(c, { 0x48, 0x81, 0xC4 });
	emit_u32(c, 0x88);
	emit(c, { 0xE9 });
	const std::size_t fix_pass3 = c.size();
	emit_u32(c, 0);

	auto rel = [&](std::size_t at, std::size_t target)
	{
		const std::int32_t v = (std::int32_t)((std::ptrdiff_t)target -
		                                      (std::ptrdiff_t)(at + 4));
		std::memcpy(c.data() + at, &v, 4);
	};

	rel(fix_slow, slow);
	rel(fix_pass1, pass);
	rel(fix_pass2, pass);
	rel(fix_pass3, pass);
	c[fix_take] = (std::uint8_t)(take - (fix_take + 1));
	return c;
}

}

bool Ready() { return g_gate.installed; }
int LastFail() { return g_last_fail; }
std::uint64_t SlotAddress() { return g_gate.slot; }
std::uint64_t Scratch() { return g_gate.state ? g_gate.state + st_scratch : 0; }

std::uint64_t Calls()
{
	if (!g_gate.state)
		return 0;
	return g_Memory.Read<std::uint64_t>(g_gate.state + st_calls);
}

static std::uint64_t unwrap_stale(std::uint64_t fn, std::uintptr_t base,
                                  std::size_t size)
{
	std::uint8_t head[2]{};
	if (g_Memory.ReadRaw((std::uintptr_t)fn, head, sizeof(head)) != sizeof(head))
		return 0;

	if (head[0] != 0x49 || head[1] != 0xBA)
		return 0;

	std::uint8_t jmp[2]{};
	if (g_Memory.ReadRaw((std::uintptr_t)fn + stub_orig_off - 6, jmp, sizeof(jmp))
		!= sizeof(jmp))
		return 0;

	if (jmp[0] != 0xFF || jmp[1] != 0x25)
		return 0;

	const auto orig = g_Memory.Read<std::uint64_t>(
		(std::uintptr_t)fn + stub_orig_off);
	if (orig < base || orig >= base + size)
		return 0;

	return orig;
}

static bool install_impl(const char* method_name, std::size_t from)
{
	if (g_gate.installed)
		return true;

	g_last_fail = 0;

	std::uintptr_t base = 0;
	std::size_t size = 0;
	if (!module_range(L"RobloxPlayerBeta.exe", &base, &size))
	{
		g_last_fail = 1;
		return false;
	}

	const bool forced = method_name && method_name[0];

	const char* method = nullptr;
	std::uintptr_t slot = 0;
	std::uint64_t orig = 0;
	std::size_t picked = from;

	for (std::size_t i = from; i < k_candidate_count; ++i)
	{
		if (!forced && g_cold[i])
			continue;

		const char* try_name = forced ? method_name : k_candidates[i];

		const std::uint64_t name = Reflect::Name(base, try_name);
		const std::uintptr_t desc = name ? find_bound_desc(base, size, name) : 0;
		if (desc)
		{
			const std::uintptr_t s = desc + Offsets::WorldRoot::RaycastBoundFn;
			auto fn = g_Memory.Read<std::uint64_t>(s);

			if (fn < base || fn >= base + size)
				fn = unwrap_stale(fn, base, size);

			if (fn)
			{
				method = try_name;
				slot = s;
				orig = fn;
				picked = i;
				break;
			}
		}

		if (forced)
			break;
	}

	if (!slot || !orig)
	{
		g_last_fail = 2;
		return false;
	}

	const std::uintptr_t state = g_Memory.Alloc(page_sz(), PAGE_READWRITE);
	if (!state)
	{
		g_last_fail = 4;
		return false;
	}

	std::vector<std::uint8_t> zero(page_sz(), 0);
	g_Memory.WriteRaw(state, zero.data(), zero.size());

	bool cave = true;
	std::uintptr_t stub = find_exec_cave(stub_bytes);
	if (!stub)
	{
		cave = false;
		stub = g_Memory.Alloc(page_sz(), PAGE_EXECUTE_READWRITE);
	}

	if (!stub)
	{
		g_Memory.Free(state);
		g_last_fail = 5;
		return false;
	}

	const auto code = build_stub(state, (std::uintptr_t)orig);
	if (code.size() > stub_bytes || !write_protected(stub, code.data(), code.size()))
	{
		if (!cave)
			g_Memory.Free(stub);
		g_Memory.Free(state);
		g_last_fail = 6;
		return false;
	}

	mark_cfg(stub);

	if (!g_Memory.Write<std::uint64_t>(slot, (std::uint64_t)stub))
	{
		if (!cave)
			g_Memory.Free(stub);
		g_Memory.Free(state);
		g_last_fail = 7;
		return false;
	}

	g_gate.installed = true;
	g_gate.slot = slot;
	g_gate.orig = (std::uintptr_t)orig;
	g_gate.stub = stub;
	g_gate.state = state;
	g_gate.stub_is_cave = cave;
	g_gate.cand = picked;
	g_gate.method = method;
	return true;
}

bool Install(const char* method_name)
{
	const bool was = g_gate.installed;
	const bool ok = install_impl(method_name, 0);
	if (!was)
	{
		Console::DumpGate(ok, g_gate.method.c_str(), g_gate.slot, g_gate.orig,
			g_gate.stub, g_gate.state, g_gate.stub_is_cave, g_last_fail);
	}
	return ok;
}

void Remove()
{
	if (!g_gate.installed)
		return;

	g_Memory.Write<std::uint64_t>(g_gate.slot, (std::uint64_t)g_gate.orig);
	if (!g_gate.stub_is_cave && g_gate.stub)
		g_Memory.Free(g_gate.stub);
	if (g_gate.state)
		g_Memory.Free(g_gate.state);

	g_gate = Gate{};
}

static bool invoke_once(std::uint64_t fn, std::uint64_t a0, std::uint64_t a1,
                        std::uint64_t a2, std::uint64_t a3,
                        std::uint64_t* out_ret, unsigned timeout_ms,
                        bool* out_cold)
{
	if (out_ret)
		*out_ret = 0;
	if (out_cold)
		*out_cold = false;

	if (!g_gate.installed || !fn)
	{
		g_last_fail = 10;
		return false;
	}

	const std::uintptr_t s = g_gate.state;
	const auto hits_before = g_Memory.Read<std::uint64_t>(s + st_calls);
	g_Memory.Write<std::uint32_t>(s + st_done, 0);
	g_Memory.Write<std::uint64_t>(s + st_ret, 0);
	g_Memory.Write<std::uint64_t>(s + st_fn, fn);
	g_Memory.Write<std::uint64_t>(s + st_a0, a0);
	g_Memory.Write<std::uint64_t>(s + st_a1, a1);
	g_Memory.Write<std::uint64_t>(s + st_a2, a2);
	g_Memory.Write<std::uint64_t>(s + st_a3, a3);

	if (!g_Memory.Write<std::uint32_t>(s + st_pending, 1))
	{
		g_last_fail = 11;
		return false;
	}

	const DWORD deadline = GetTickCount() + timeout_ms;
	while (GetTickCount() < deadline)
	{
		if (g_Memory.Read<std::uint32_t>(s + st_done) == 1)
		{
			if (out_ret)
				*out_ret = g_Memory.Read<std::uint64_t>(s + st_ret);
			g_last_fail = 0;
			return true;
		}

		if (!g_Memory.IsAlive())
			break;

		Sleep(1);
	}

	g_Memory.Write<std::uint32_t>(s + st_pending, 0);

	const auto hits_after = g_Memory.Read<std::uint64_t>(s + st_calls);
	if (out_cold)
		*out_cold = hits_after == hits_before;

	Console::GateTimeout(g_gate.method.c_str(), hits_after);
	g_last_fail = 12;
	return false;
}

bool Invoke(std::uint64_t fn, std::uint64_t a0, std::uint64_t a1,
            std::uint64_t a2, std::uint64_t a3,
            std::uint64_t* out_ret, unsigned timeout_ms)
{
	for (std::size_t tries = 0; tries < k_candidate_count; ++tries)
	{
		bool cold = false;
		if (invoke_once(fn, a0, a1, a2, a3, out_ret, timeout_ms, &cold))
			return true;

		if (!cold || !g_gate.installed)
			return false;

		g_cold[g_gate.cand] = true;
		Remove();

		Sleep(15);
		if (!install_impl(nullptr, 0))
			return false;

		Console::DumpGate(true, g_gate.method.c_str(), g_gate.slot, g_gate.orig,
			g_gate.stub, g_gate.state, g_gate.stub_is_cave, 0);
	}

	return false;
}

}
}
}
