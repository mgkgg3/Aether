#include "pch.h"
#include "Classes.h"
#include "core/memory/Memory.h"
#include "core/roblox/offsets/Offsets.h"
#undef GetClassName

std::string Cheat::Instance::GetName() const
{
	if (!g_Memory.IsValid(this->address))
	{
		return "Unknown";
	}

	const std::uint64_t container = g_Memory.Read<std::uint64_t>(
		this->address + Offsets::Instance::NameContainer);
	if (!g_Memory.IsValid(container))
	{
		return "Unknown";
	}

	return g_Memory.ReadString(container + Offsets::Instance::Name);
}

std::string Cheat::Instance::GetClassName() const
{
	if (!g_Memory.IsValid(this->address))
	{
		return "Unknown";
	}

	const std::uint64_t desc = g_Memory.Read<std::uint64_t>(
		this->address + Offsets::Instance::ClassDescriptor);
	if (!g_Memory.IsValid(desc))
	{
		return "Unknown";
	}

	const std::uint64_t cls = g_Memory.Read<std::uint64_t>(
		desc + Offsets::Instance::ClassName);
	if (!g_Memory.IsValid(cls))
	{
		return "Unknown";
	}

	return g_Memory.ReadString(cls);
}

bool Cheat::Instance::HasChildren() const
{
	if (!address || !g_Memory.IsValid(address))
		return false;

	std::uint64_t kids = g_Memory.Read<std::uint64_t>(address + Offsets::Instance::ChildrenStart);
	if (!g_Memory.IsValid(kids))
		return false;

	std::uint64_t start = g_Memory.Read<std::uint64_t>(kids);
	std::uint64_t end = g_Memory.Read<std::uint64_t>(kids + Offsets::Instance::ChildrenEnd);
	if (!g_Memory.IsValid(start) || !g_Memory.IsValid(end) || start >= end)
		return false;

	for (std::uint64_t ptr = start; ptr < end; ptr += 16)
	{
		std::uint64_t child = g_Memory.Read<std::uint64_t>(ptr);
		if (g_Memory.IsValid(child))
			return true;
	}
	return false;
}

std::vector<Cheat::Instance> Cheat::Instance::GetChildren() const
{
	if (!address || !g_Memory.IsValid(address))
	{
		return {};
	}

	std::uint64_t kids = g_Memory.Read<std::uint64_t>(address + Offsets::Instance::ChildrenStart);
	if (!g_Memory.IsValid(kids))
	{
		return {};
	}

	std::uint64_t start = g_Memory.Read<std::uint64_t>(kids);
	std::uint64_t end = g_Memory.Read<std::uint64_t>(kids + Offsets::Instance::ChildrenEnd);

	if (!g_Memory.IsValid(start) || !g_Memory.IsValid(end) || start >= end)
	{
		return {};
	}

	std::vector<Cheat::Instance> out;
	out.reserve(32);

	int count = 0;
	for (std::uint64_t ptr = start; ptr < end && count < 10000; ptr += 16, ++count)
	{
		std::uint64_t child = g_Memory.Read<std::uint64_t>(ptr);
		if (g_Memory.IsValid(child))
		{
			out.emplace_back(child);
		}
	}

	return out;
}

std::shared_ptr<Cheat::Instance> Cheat::Instance::FindFirstChild(std::string child) const
{
	for (auto& c : GetChildren())
	{
		if (c.GetName() == child)
		{
			return std::make_shared<Cheat::Instance>(c);
		}
	}

	return nullptr;
}

std::shared_ptr<Cheat::Instance> Cheat::Instance::GetParent() const
{
	if (!g_Memory.IsValid(address))
	{
		return nullptr;
	}

	std::uint64_t parent = g_Memory.Read<std::uint64_t>(address + Offsets::Instance::Parent);
	if (!g_Memory.IsValid(parent))
	{
		return nullptr;
	}

	return std::make_shared<Cheat::Instance>(parent);
}

namespace {

struct sp_t
{
	std::uint64_t ptr;
	std::uint64_t ctrl;
};

bool kids_remove(std::uint64_t parent, std::uint64_t child)
{
	if (!g_Memory.IsValid(parent) || !g_Memory.IsValid(child))
	{
		return false;
	}

	std::uint64_t cow = g_Memory.Read<std::uint64_t>(
		parent + Offsets::Instance::ChildrenStart);
	if (!g_Memory.IsValid(cow))
	{
		return true;
	}

	std::uint64_t first = g_Memory.Read<std::uint64_t>(cow);
	std::uint64_t last = g_Memory.Read<std::uint64_t>(
		cow + Offsets::Instance::ChildrenEnd);
	if (!g_Memory.IsValid(first) || last <= first)
	{
		return true;
	}

	const std::size_t stride = sizeof(sp_t);
	std::size_t n = (std::size_t)(last - first) / stride;
	if (n > 10000)
	{
		return false;
	}

	for (std::size_t i = 0; i < n; ++i)
	{
		sp_t e = g_Memory.Read<sp_t>(first + i * stride);
		if (e.ptr != child)
		{
			continue;
		}

		if (i + 1 < n)
		{
			sp_t tail = g_Memory.Read<sp_t>(first + (n - 1) * stride);
			g_Memory.Write<sp_t>(first + i * stride, tail);
		}

		g_Memory.Write<std::uint64_t>(
			cow + Offsets::Instance::ChildrenEnd, last - stride);
		return true;
	}

	return true;
}

bool kids_add(std::uint64_t parent, std::uint64_t child)
{
	if (!g_Memory.IsValid(parent) || !g_Memory.IsValid(child))
	{
		return false;
	}

	std::uint64_t cow = g_Memory.Read<std::uint64_t>(
		parent + Offsets::Instance::ChildrenStart);
	if (!g_Memory.IsValid(cow))
	{
		return false;
	}

	sp_t entry{};
	entry.ptr = child;
	entry.ctrl = g_Memory.Read<std::uint64_t>(child + 0x10);

	std::uint64_t first = g_Memory.Read<std::uint64_t>(cow);
	std::uint64_t last = g_Memory.Read<std::uint64_t>(
		cow + Offsets::Instance::ChildrenEnd);
	std::uint64_t cap = g_Memory.Read<std::uint64_t>(cow + 0x10);

	const std::size_t stride = sizeof(sp_t);

	if (g_Memory.IsValid(first) && last > first)
	{
		std::size_t n = (std::size_t)(last - first) / stride;
		if (n > 10000)
		{
			return false;
		}

		for (std::size_t i = 0; i < n; ++i)
		{
			sp_t e = g_Memory.Read<sp_t>(first + i * stride);
			if (e.ptr == child)
			{
				return true;
			}
		}
	}

	bool room = g_Memory.IsValid(first) && g_Memory.IsValid(cap) &&
		(last + stride) <= cap && last >= first;

	if (room)
	{
		g_Memory.Write<sp_t>(last, entry);
		g_Memory.Write<std::uint64_t>(
			cow + Offsets::Instance::ChildrenEnd, last + stride);
		return true;
	}

	std::size_t old_n = 0;
	if (g_Memory.IsValid(first) && last > first)
	{
		old_n = (std::size_t)(last - first) / stride;
	}

	std::size_t new_n = old_n + 1;
	std::size_t new_cap = new_n * 2;
	if (new_cap < 4)
	{
		new_cap = 4;
	}

	std::uint64_t nf = g_Memory.Alloc(new_cap * stride, PAGE_READWRITE);
	if (!nf)
	{
		return false;
	}

	if (old_n > 0)
	{
		std::vector<sp_t> old(old_n);
		g_Memory.ReadRaw(first, old.data(), old_n * stride);
		g_Memory.WriteRaw(nf, old.data(), old_n * stride);
	}

	g_Memory.Write<sp_t>(nf + old_n * stride, entry);
	g_Memory.Write<std::uint64_t>(cow, nf);
	g_Memory.Write<std::uint64_t>(
		cow + Offsets::Instance::ChildrenEnd, nf + new_n * stride);
	g_Memory.Write<std::uint64_t>(cow + 0x10, nf + new_cap * stride);

	return true;
}

}

bool Cheat::Instance::SetParent(std::uint64_t parent_addr) const
{
	if (!g_Memory.IsValid(address))
	{
		return false;
	}

	if (parent_addr == address)
	{
		return false;
	}

	std::uint64_t cur = g_Memory.Read<std::uint64_t>(
		address + Offsets::Instance::Parent);
	if (cur == parent_addr)
	{
		return true;
	}

	if (g_Memory.IsValid(cur))
	{
		kids_remove(cur, address);
	}

	if (g_Memory.IsValid(parent_addr))
	{
		if (!kids_add(parent_addr, address))
		{
			return false;
		}
	}

	g_Memory.Write<std::uint64_t>(
		address + Offsets::Instance::Parent, parent_addr);
	return true;
}
