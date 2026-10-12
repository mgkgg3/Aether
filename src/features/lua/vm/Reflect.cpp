#include "pch.h"
#include "Reflect.h"
#include "core/memory/Memory.h"
#include "core/roblox/offsets/Offsets.h"

#include <cstring>

namespace Cheat {
namespace Features {
namespace Reflect {
namespace {

struct Table
{
	std::uint64_t start = 0;
	std::uint64_t end = 0;
	std::uint64_t empty = 0;
};

bool KeyEquals(std::uint64_t key, const char* text, std::size_t len)
{
	char buf[128];
	if (len + 1 > sizeof(buf))
		return false;

	if (g_Memory.ReadRaw((std::uintptr_t)key, buf, len + 1) != len + 1)
		return false;

	return buf[len] == '\0' && std::memcmp(buf, text, len) == 0;
}

bool ReadTable(std::uintptr_t at, Table* out)
{
	out->start = g_Memory.Read<std::uint64_t>(at + Offsets::Reflection::TableStart);
	out->end = g_Memory.Read<std::uint64_t>(at + Offsets::Reflection::TableEnd);
	out->empty = g_Memory.Read<std::uint64_t>(at + Offsets::Reflection::TableEmpty);

	return out->start && out->end > out->start &&
	       (out->end - out->start) <= 0x400000;
}

}

std::uint64_t Name(std::uintptr_t base, const char* text)
{
	if (!text || !text[0])
		return 0;

	const auto reg = g_Memory.Read<std::uint64_t>(
		base + Offsets::Reflection::NameRegistry);
	if (!reg)
		return 0;

	Table t;
	if (!ReadTable((std::uintptr_t)reg + Offsets::Reflection::NameTable, &t))
		return 0;

	const std::size_t len = std::strlen(text);

	for (std::uint64_t e = t.start; e + Offsets::Reflection::TableStride <= t.end;
	     e += Offsets::Reflection::TableStride)
	{
		const auto key = g_Memory.Read<std::uint64_t>(e);
		if (!key || key == t.empty)
			continue;

		if (!KeyEquals(key, text, len))
			continue;

		return g_Memory.Read<std::uint64_t>(e + Offsets::Reflection::EntryValue);
	}

	return 0;
}

std::uint64_t Creator(std::uintptr_t base, std::uint64_t name)
{
	if (!name)
		return 0;

	Table t;
	if (!ReadTable(base + Offsets::Reflection::CreatorTable, &t))
		return 0;

	for (std::uint64_t e = t.start; e + Offsets::Reflection::TableStride <= t.end;
	     e += Offsets::Reflection::TableStride)
	{
		if (g_Memory.Read<std::uint64_t>(e) != name)
			continue;

		return g_Memory.Read<std::uint64_t>(e + Offsets::Reflection::EntryValue);
	}

	return 0;
}

}
}
}
