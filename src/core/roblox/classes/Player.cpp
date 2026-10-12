#include "pch.h"
#include "Classes.h"
#include "core/memory/Memory.h"
#include "core/globals/Globals.h"
#include "core/roblox/offsets/Offsets.h"

std::string Cheat::Player::GetDisplayName() const
{
	if (!g_Memory.IsValid(address))
	{
		return "Unknown";
	}

	return g_Memory.ReadString(address + Offsets::Player::DisplayName);
}

std::int64_t Cheat::Player::GetUserId() const
{
	if (!g_Memory.IsValid(address))
		return 0;

	return g_Memory.Read<std::int64_t>(address + Offsets::Player::UserId);
}

std::int32_t Cheat::Player::GetAccountAge() const
{
	if (!g_Memory.IsValid(address))
	{
		return 0;
	}

	return g_Memory.Read<std::int32_t>(address + Offsets::Player::AccountAge);
}

float Cheat::Player::GetMaxZoomDistance() const
{
	if (!g_Memory.IsValid(address))
	{
		return 0.f;
	}

	return g_Memory.Read<float>(address + Offsets::Player::MaxZoomDistance);
}

float Cheat::Player::GetMinZoomDistance() const
{
	if (!g_Memory.IsValid(address))
	{
		return 0.f;
	}

	return g_Memory.Read<float>(address + Offsets::Player::MinZoomDistance);
}

bool Cheat::Player::IsLocalPlayer() const
{
	if (!g_Memory.IsValid(address))
	{
		return false;
	}

	if (!Cheat::Globals::Players || !g_Memory.IsValid(Cheat::Globals::Players->address))
	{
		return false;
	}

	std::uint64_t local = g_Memory.Read<std::uint64_t>(
		Cheat::Globals::Players->address + Offsets::Player::LocalPlayer);
	return local == address;
}

std::uint64_t Cheat::Player::GetTeam() const
{
	if (!g_Memory.IsValid(address))
	{
		return 0;
	}

	std::uint64_t team = g_Memory.Read<std::uint64_t>(address + Offsets::Player::Team);
	if (!g_Memory.IsValid(team))
	{
		return 0;
	}

	return team;
}

std::uint64_t Cheat::Player::GetCharacterAddress() const
{
	if (!g_Memory.IsValid(address))
	{
		return 0;
	}

	return g_Memory.Read<std::uint64_t>(address + Offsets::Player::ModelInstance);
}

std::shared_ptr<Cheat::Instance> Cheat::Player::GetCharacter() const
{
	std::uint64_t ch = GetCharacterAddress();
	if (!g_Memory.IsValid(ch))
	{
		return nullptr;
	}

	return std::make_shared<Cheat::Instance>(ch);
}
