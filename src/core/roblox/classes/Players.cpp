#include "pch.h"
#include "Classes.h"
#include "core/memory/Memory.h"
#include "core/roblox/offsets/Offsets.h"

std::vector<std::shared_ptr<Cheat::Instance>> Cheat::Players::GetPlayers() const
{
	std::vector<std::shared_ptr<Cheat::Instance>> out;

	for (auto& child : GetChildren())
	{
		out.push_back(std::make_shared<Cheat::Instance>(child.address));
	}

	return out;
}
