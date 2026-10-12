#include "pch.h"
#include "PlayerHandler.h"
#include "core/roblox/classes/Classes.h"
#include <thread>
#include <chrono>
#include "core/roblox/offsets/Offsets.h"
#include "core/memory/Memory.h"
#include "core/globals/Globals.h"
#include "core/console/Console.h"
#include "features/visuals/RaycastEngine.h"
#include "features/visuals/MeshCache.h"
#include "features/visuals/HavocWorldEsp.h"
#include "features/games/PhantomForces.h"
#include "features/games/ApocalypseRising.h"
#include "app/Settings.h"
#include <algorithm>
#include <mutex>
#include <utility>
#include <unordered_set>

#undef GetClassName

namespace Cheat {
    std::unordered_map<std::uint64_t, PlayerCache> PlayerHandler::playerCache;
    std::thread PlayerHandler::cacheThread;
    std::atomic<bool> PlayerHandler::shouldRun = false;
    std::mutex PlayerHandler::cacheMutex;

    std::unordered_map<std::uint64_t, PlayerCache> PlayerHandler::previousSnapshot;
    std::unordered_map<std::uint64_t, PlayerCache> PlayerHandler::staging;
    std::thread             PlayerHandler::watchdogThread;
    std::atomic<std::uint64_t> PlayerHandler::tickCount{ 0 };
    std::atomic<std::uint32_t> PlayerHandler::lastTickMs{ 0 };
    std::atomic<std::uint32_t> PlayerHandler::tickIntervalMs{ 250 };
    std::atomic<const char*>   PlayerHandler::phase{ "idle" };
}

namespace {

bool PartPtrValid(const std::shared_ptr<Cheat::Instance>& p)
{
    return p && g_Memory.IsValid(p->address);
}

bool AnyBodyPartValid(const Cheat::PlayerCache& c)
{
    return PartPtrValid(c.head) || PartPtrValid(c.humanoidRootPart) ||
           PartPtrValid(c.upperTorso) || PartPtrValid(c.lowerTorso) ||
           PartPtrValid(c.leftUpperArm) || PartPtrValid(c.rightUpperArm) ||
           PartPtrValid(c.leftUpperLeg) || PartPtrValid(c.rightUpperLeg) ||
           PartPtrValid(c.leftHand) || PartPtrValid(c.rightHand) ||
           PartPtrValid(c.leftFoot) || PartPtrValid(c.rightFoot);
}

void ClearBodyParts(Cheat::PlayerCache& cache)
{
    cache.head = cache.humanoidRootPart = cache.upperTorso = cache.lowerTorso = nullptr;
    cache.leftUpperArm = cache.leftLowerArm = cache.leftHand = nullptr;
    cache.rightUpperArm = cache.rightLowerArm = cache.rightHand = nullptr;
    cache.leftUpperLeg = cache.leftLowerLeg = cache.leftFoot = nullptr;
    cache.rightUpperLeg = cache.rightLowerLeg = cache.rightFoot = nullptr;
    cache.humanoid = nullptr;
    cache.toolName.clear();
    cache.isR6 = false;
}

void CopyBodyParts(Cheat::PlayerCache& dst, const Cheat::PlayerCache& src)
{
    dst.head = src.head;
    dst.humanoidRootPart = src.humanoidRootPart;
    dst.upperTorso = src.upperTorso;
    dst.lowerTorso = src.lowerTorso;
    dst.leftUpperArm = src.leftUpperArm;
    dst.leftLowerArm = src.leftLowerArm;
    dst.leftHand = src.leftHand;
    dst.rightUpperArm = src.rightUpperArm;
    dst.rightLowerArm = src.rightLowerArm;
    dst.rightHand = src.rightHand;
    dst.leftUpperLeg = src.leftUpperLeg;
    dst.leftLowerLeg = src.leftLowerLeg;
    dst.leftFoot = src.leftFoot;
    dst.rightUpperLeg = src.rightUpperLeg;
    dst.rightLowerLeg = src.rightLowerLeg;
    dst.rightFoot = src.rightFoot;
    if (!dst.humanoid)
        dst.humanoid = src.humanoid;
    dst.isR6 = src.isR6;
}

bool PopulatePartsFromChildren(const std::vector<Cheat::Instance>& parts, Cheat::PlayerCache& cache)
{
    std::uint64_t hum_addr = 0;

    for (const auto& part : parts)
    {
        std::string cls = part.GetClassName();
        if (cls == "Accessory")
            continue;
        if (cls == "Tool")
        {
            cache.toolName = part.GetName();
            continue;
        }

        if (cls == "Humanoid")
        {
            cache.humanoid = std::make_shared<Cheat::Instance>(part);
            hum_addr = part.address;
        }

        std::string nm = part.GetName();

        if (nm == "Head")
        {
            cache.head = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "HumanoidRootPart")
        {
            cache.humanoidRootPart = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "UpperTorso")
        {
            cache.upperTorso = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "LowerTorso")
        {
            cache.lowerTorso = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "LeftUpperArm")
        {
            cache.leftUpperArm = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "LeftLowerArm")
        {
            cache.leftLowerArm = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "LeftHand")
        {
            cache.leftHand = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "RightUpperArm")
        {
            cache.rightUpperArm = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "RightLowerArm")
        {
            cache.rightLowerArm = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "RightHand")
        {
            cache.rightHand = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "LeftUpperLeg")
        {
            cache.leftUpperLeg = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "LeftLowerLeg")
        {
            cache.leftLowerLeg = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "LeftFoot")
        {
            cache.leftFoot = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "RightUpperLeg")
        {
            cache.rightUpperLeg = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "RightLowerLeg")
        {
            cache.rightLowerLeg = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "RightFoot")
        {
            cache.rightFoot = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "Torso")
        {
            cache.upperTorso = std::make_shared<Cheat::Instance>(part);
            cache.isR6 = true;
        }

        else if (nm == "Left Arm")
        {
            cache.leftUpperArm = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "Right Arm")
        {
            cache.rightUpperArm = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "Left Leg")
        {
            cache.leftUpperLeg = std::make_shared<Cheat::Instance>(part);
        }

        else if (nm == "Right Leg")
        {
            cache.rightUpperLeg = std::make_shared<Cheat::Instance>(part);
        }
    }

    if (!cache.humanoidRootPart && hum_addr)
    {
        std::uint64_t root = Cheat::Humanoid(hum_addr).GetRootPartAddress();
        if (g_Memory.IsValid(root))
            cache.humanoidRootPart = std::make_shared<Cheat::Instance>(root);
    }

    return AnyBodyPartValid(cache) || hum_addr != 0;
}

bool PopulateParts(std::uint64_t characterAddress, Cheat::PlayerCache& cache)
{
    if (!g_Memory.IsValid(characterAddress))
        return false;

    auto kids = Cheat::Instance(characterAddress).GetChildren();
    bool ok = PopulatePartsFromChildren(kids, cache);

    for (const auto& child : kids)
    {
        std::string cls = child.GetClassName();
        if (cls == "Model" || cls == "Folder" || cls == "Configuration")
            ok = PopulatePartsFromChildren(child.GetChildren(), cache) || ok;
    }
    return ok;
}

bool IsPlayerDead(const Cheat::PlayerCache& cache)
{
    if (cache.is_corpse)
        return true;
    if (!PartPtrValid(cache.humanoid))
        return false;

    Cheat::Humanoid hum(cache.humanoid->address);

    if (hum.GetStateId() == 15)
        return true;
    return hum.GetHealth() <= 0.f;
}

bool RefreshCorpseFromCharacter(Cheat::PlayerCache& cache, std::uint64_t character)
{
    if (!g_Memory.IsValid(character))
        return false;

    Cheat::PlayerCache tmp = cache;
    ClearBodyParts(tmp);
    tmp.character = character;
    if (!PopulateParts(character, tmp) || !AnyBodyPartValid(tmp))
        return false;

    CopyBodyParts(cache, tmp);
    cache.character = character;
    cache.humanoid = tmp.humanoid;
    cache.toolName = std::move(tmp.toolName);
    cache.isR6 = tmp.isR6;
    return true;
}

bool RetainCorpseEntry(Cheat::PlayerCache& out, const Cheat::PlayerCache& prev)
{
    out = prev;
    out.is_corpse = true;

    if (RefreshCorpseFromCharacter(out, prev.character))
        return AnyBodyPartValid(out);

    if (AnyBodyPartValid(prev))
    {
        CopyBodyParts(out, prev);
        return true;
    }

    return false;
}

std::uint64_t LocalCharacterAddress()
{
    if (Cheat::Games::PhantomForces::IsActivePlace()) {
        const std::uint64_t pf = Cheat::Games::PhantomForces::LocalCharacter();
        if (g_Memory.IsValid(pf))
            return pf;
    }

    if (!Cheat::Globals::Players || !g_Memory.IsValid(Cheat::Globals::Players->address))
        return 0;

    std::uint64_t local = g_Memory.Read<std::uint64_t>(
        Cheat::Globals::Players->address + Offsets::Player::LocalPlayer);
    if (!g_Memory.IsValid(local))
        return 0;

    return g_Memory.Read<std::uint64_t>(local + Offsets::Player::ModelInstance);
}

bool IsIgnoredService(const std::string& cls)
{
    return cls == "ReplicatedStorage" || cls == "ServerStorage" ||
           cls == "ReplicatedFirst" || cls == "StarterPlayer" ||
           cls == "StarterPack" || cls == "StarterGui" ||
           cls == "CoreGui" || cls == "Chat" ||
           cls == "MaterialService" || cls == "Lighting" ||
           cls == "SoundService" || cls == "TextChatService";
}

bool IsLeafPartClass(const std::string& cls)
{
    return cls == "Part" || cls == "MeshPart" || cls == "UnionOperation" ||
           cls == "NegateOperation" || cls == "IntersectOperation" ||
           cls == "TrussPart" || cls == "WedgePart" || cls == "CornerWedgePart" ||
           cls == "Seat" || cls == "VehicleSeat" || cls == "SpawnLocation" ||
           cls == "Terrain";
}

std::uint64_t FindOwningPlayerAddress(std::uint64_t character_address)
{
    if (!g_Memory.IsValid(character_address))
        return 0;
    if (!Cheat::Globals::Players || !g_Memory.IsValid(Cheat::Globals::Players->address))
        return 0;

    for (const auto& player : Cheat::Globals::Players->GetChildren()) {
        if (!g_Memory.IsValid(player.address))
            continue;
        const std::uint64_t model = g_Memory.Read<std::uint64_t>(
            player.address + Offsets::Player::ModelInstance);
        if (model == character_address)
            return player.address;
    }
    return 0;
}

bool CharacterOwnedByPlayer(std::uint64_t character_address)
{
    return FindOwningPlayerAddress(character_address) != 0;
}

void AddCharacter(const Cheat::Instance& model, Cheat::PlayerCache&& cache,
                  std::uint64_t localChar,
                  std::unordered_map<std::uint64_t, Cheat::PlayerCache>& target)
{
    if (model.address == localChar)
        return;

    cache.name = model.GetName();
    if (cache.name.empty())
        cache.name = "unknown";

    if (cache.humanoid)
    {
        std::string dn = Cheat::Humanoid(cache.humanoid->address).GetDisplayName();
        if (dn.empty() || dn == "Unknown")
            cache.displayName = cache.name;

        else
            cache.displayName = dn;
    }

    else
    {
        cache.displayName = cache.name;
    }

    cache.character = model.address;
    cache.team_folder = Cheat::PlayerHandler::ResolveTeamFolder(model.address);

    const std::uint64_t player_addr = FindOwningPlayerAddress(model.address);
    cache.is_player = player_addr != 0;
    if (player_addr)
    {
        cache.player_address = player_addr;
        cache.user_id = Cheat::Player(player_addr).GetUserId();

        std::string pdn = Cheat::Player(player_addr).GetDisplayName();
        if (!pdn.empty() && pdn != "Unknown")
            cache.displayName = pdn;
    }

    target[model.address] = std::move(cache);
}

void ScanNode(const Cheat::Instance& node, int depth, int& budget,
              std::uint64_t localChar,
              std::unordered_map<std::uint64_t, Cheat::PlayerCache>& target)
{

    if (depth > 12 || budget <= 0)
        return;

    auto kids = node.GetChildren();

    {
        Cheat::PlayerCache probe(node.address);
        if (PopulatePartsFromChildren(kids, probe)) {
            AddCharacter(node, std::move(probe), localChar, target);
            return;
        }
    }

    for (const auto& child : kids) {
        if (--budget <= 0)
            return;
        if (IsLeafPartClass(child.GetClassName()))
            continue;
        ScanNode(child, depth + 1, budget, localChar, target);
    }
}

void ScanTree(const Cheat::Instance& root, bool skipStorageServices,
              std::uint64_t localChar,
              std::unordered_map<std::uint64_t, Cheat::PlayerCache>& target)
{
    int budget = 40000;
    if (!skipStorageServices)
    {
        ScanNode(root, 0, budget, localChar, target);
        return;
    }

    for (const auto& service : root.GetChildren())
    {
        if (budget <= 0)
            break;
        if (IsIgnoredService(service.GetClassName()))
            continue;
        ScanNode(service, 1, budget, localChar, target);
    }
}

void CacheFromWorkspace(std::unordered_map<std::uint64_t, Cheat::PlayerCache>& target)
{
    const std::uint64_t localChar = LocalCharacterAddress();

    if (Cheat::Globals::Workspace && g_Memory.IsValid(Cheat::Globals::Workspace->address))
        ScanTree(*Cheat::Globals::Workspace, false, localChar, target);

    if (target.empty() && g_Memory.IsValid(Cheat::Globals::InstanceDataModel.address))
        ScanTree(Cheat::Globals::InstanceDataModel, true, localChar, target);
}

bool ChildNamedExists(const Cheat::Instance& parent, const char* name)
{
    auto c = parent.FindFirstChild(name);
    return c && g_Memory.IsValid(c->address);
}

int CountDirectHumanoidModels(const Cheat::Instance& root)
{
    int n = 0;
    for (const auto& child : root.GetChildren()) {
        if (child.GetClassName() != "Model")
            continue;
        if (ChildNamedExists(child, "Humanoid") || ChildNamedExists(child, "HumanoidRootPart"))
            ++n;
    }
    return n;
}

bool IsCharactersRootCandidate(const Cheat::Instance& node)
{
    const std::string cls = node.GetClassName();
    if (cls != "Model" && cls != "Folder")
        return false;

    const std::string name = node.GetName();
    if (name == "Buildings" || name == "Camera" || name == "Terrain" ||
        name == "Ignored" || name == "Debris" || name == "Map" ||
        name == "Effects" || name == "Sounds" || name == "Loots" ||
        name == "CurrentCamera")
        return false;

    const int hum = CountDirectHumanoidModels(node);
    if (hum >= 2)
        return true;

    if (hum >= 1 && ChildNamedExists(node, "NPCs"))
        return true;
    return false;
}

std::uint64_t FindCharactersRootAddress()
{
    static std::uint64_t cached = 0;
    static ULONGLONG next_scan = 0;
    const ULONGLONG now = GetTickCount64();

    if (cached && g_Memory.IsValid(cached)) {
        Cheat::Instance root(cached);
        if (CountDirectHumanoidModels(root) >= 1 || ChildNamedExists(root, "NPCs"))
            return cached;
        cached = 0;
    }

    if (now < next_scan)
        return cached;
    next_scan = now + 1500;

    if (!Cheat::Globals::Workspace || !g_Memory.IsValid(Cheat::Globals::Workspace->address))
        return 0;

    for (const auto& child : Cheat::Globals::Workspace->GetChildren()) {
        if (!IsCharactersRootCandidate(child))
            continue;
        cached = child.address;
        return cached;
    }
    return 0;
}

bool LooksLikeBotModel(const Cheat::Instance& model)
{
    const bool active = ChildNamedExists(model, "ActiveScripts");
    const bool weld_link = ChildNamedExists(model, "WeldObjectsLink");
    const bool player_bits =
        ChildNamedExists(model, "MovementAnticheat") ||
        ChildNamedExists(model, "FakeCam") ||
        ChildNamedExists(model, "Animate");

    if (player_bits && !active)
        return false;
    if (active || weld_link)
        return true;
    return false;
}

void MergeStreamedBots(std::unordered_map<std::uint64_t, Cheat::PlayerCache>& target)
{
    if (!Cheat::Visuals::HavocWorldEsp::IsActivePlace())
        return;

    std::uint64_t root_addr = FindCharactersRootAddress();
    if (!root_addr)
        return;

    std::unordered_set<std::uint64_t> known;
    known.reserve(target.size() * 2 + 8);
    for (const auto& [_, cache] : target)
    {
        if (cache.character)
            known.insert(cache.character);
    }

    std::uint64_t localChar = LocalCharacterAddress();
    Cheat::Instance root(root_addr);

    for (const auto& child : root.GetChildren())
    {
        if (child.GetClassName() != "Model")
            continue;
        if (child.address == localChar)
            continue;
        if (known.count(child.address))
            continue;
        if (CharacterOwnedByPlayer(child.address))
            continue;
        if (!LooksLikeBotModel(child))
            continue;

        Cheat::PlayerCache cache(child.address);
        if (!PopulateParts(child.address, cache))
            continue;
        if (!cache.humanoidRootPart && !cache.head)
            continue;

        AddCharacter(child, std::move(cache), localChar, target);

        auto it = target.find(child.address);
        if (it != target.end())
            it->second.is_player = false;
    }
}

std::uint64_t FindServiceByClass(const Cheat::Instance& dm, const char* cls)
{
    for (const auto& child : dm.GetChildren())
        if (child.GetClassName() == cls)
            return child.address;
    return 0;
}

bool LooksLikeDataModel(std::uint64_t dm)
{
    if (!dm)
        return false;
    Cheat::Instance inst(dm);
    if (inst.GetClassName() == "DataModel")
        return true;
    const std::string name = inst.GetName();
    return name == "Game" || name == "Ugc" || name == "LuaApp";
}

std::uint64_t ResolveDataModel()
{
    uintptr_t base = g_Memory.GetModuleBase();
    if (!base)
        return 0;

    std::uint64_t front = g_Memory.Read<std::uint64_t>(base + Offsets::FakeDataModel::Pointer);
    if (front)
    {
        Cheat::Globals::FrontDataModel = front;
        std::uint64_t dm = g_Memory.Read<std::uint64_t>(front + Offsets::FakeDataModel::RealDataModel);
        if (LooksLikeDataModel(dm))
            return dm;
        if (LooksLikeDataModel(front))
            return front;
    }

    std::uint64_t ve = g_Memory.Read<std::uint64_t>(base + Offsets::VisualEngine::Pointer);
    if (ve)
    {
        std::uint64_t fake = g_Memory.Read<std::uint64_t>(ve + Offsets::VisualEngine::FakeDataModel);
        if (fake)
        {
            std::uint64_t dm = g_Memory.Read<std::uint64_t>(fake + Offsets::FakeDataModel::RealDataModel);
            if (LooksLikeDataModel(dm))
                return dm;

            dm = g_Memory.Read<std::uint64_t>(fake + Offsets::RenderJob::RealDataModel);
            if (LooksLikeDataModel(dm))
                return dm;
            if (LooksLikeDataModel(fake))
                return fake;
        }
    }

    static ULONGLONG s_last_diag = 0;
    ULONGLONG now = GetTickCount64();
    if (now - s_last_diag > 5000)
    {
        s_last_diag = now;
        Cheat::Console::Clear();
        Cheat::Console::Log(Cheat::Console::Color::Yellow, "waiting for datamodel");
        Cheat::Console::Ptr(Cheat::Console::Color::Yellow, "Front DM", front);
        Cheat::Console::Ptr(Cheat::Console::Color::Magenta, "VisualEngine", ve);
    }
    return 0;
}

void RefreshGlobals()
{
    std::uint64_t dm = ResolveDataModel();
    if (!dm)
        return;

    bool changed = false;
    if (dm != Cheat::Globals::InstanceDataModel.address)
    {
        Cheat::Globals::InstanceDataModel.address = dm;
        Cheat::Globals::Workspace = nullptr;
        Cheat::Globals::Players = nullptr;
        changed = true;
    }

    if (!Cheat::Globals::Workspace)
    {
        std::uint64_t ws = FindServiceByClass(Cheat::Globals::InstanceDataModel, "Workspace");
        if (ws)
        {
            Cheat::Globals::Workspace = std::make_shared<Cheat::Workspace>(ws);
            changed = true;
        }
    }
    if (!Cheat::Globals::Players)
    {
        std::uint64_t pl = FindServiceByClass(Cheat::Globals::InstanceDataModel, "Players");
        if (pl)
        {
            Cheat::Globals::Players = std::make_shared<Cheat::Players>(pl);
            changed = true;
        }
    }

    if (changed && Cheat::Globals::Workspace && Cheat::Globals::Players)
    {
        Cheat::Console::Clear();
        Cheat::Console::DumpWorld();
    }
}

}

std::uint64_t Cheat::PlayerHandler::ResolveTeamFolder(std::uint64_t character_address)
{
    if (!g_Memory.IsValid(character_address))
        return 0;

    auto parent = Instance(character_address).GetParent();
    if (!parent || !g_Memory.IsValid(parent->address))
        return 0;

    if (Globals::Workspace && parent->address == Globals::Workspace->address)
        return 0;

    const std::string cls = parent->GetClassName();
    if (cls == "Workspace" || cls == "Players" || cls == "DataModel" ||
        cls == "Camera" || cls == "Terrain")
        return 0;

    if (cls != "Folder" && cls != "Model" && cls != "Configuration")
        return 0;

    return parent->address;
}

std::uint64_t Cheat::PlayerHandler::LocalTeamFolder()
{
    if (Games::PhantomForces::IsActivePlace()) {
        const std::uint64_t pf = Games::PhantomForces::LocalTeamFolder();
        if (pf)
            return pf;
    }
    return ResolveTeamFolder(LocalCharacterAddress());
}

bool Cheat::PlayerHandler::IsTeammate(const PlayerCache& cache, std::uint64_t local_team_folder)
{
    if (Games::PhantomForces::IsActivePlace())
        return Games::PhantomForces::IsTeammate(cache, local_team_folder);

    if (!local_team_folder || !cache.team_folder)
        return false;
    if (!cache.is_player)
        return false;
    return cache.team_folder == local_team_folder;
}

void Cheat::PlayerHandler::UpdateCache(const Instance& player,
                                       std::unordered_map<std::uint64_t, PlayerCache>& target)
{
    if (!g_Memory.IsValid(player.address))
        return;

    std::uint64_t char_addr = g_Memory.Read<std::uint64_t>(
        player.address + Offsets::Player::ModelInstance);
    if (!g_Memory.IsValid(char_addr))
        return;

    PlayerCache cache(player.address);
    cache.name = player.GetName();
    if (cache.name.empty())
        return;

    cache.character = char_addr;
    cache.team_folder = ResolveTeamFolder(char_addr);
    cache.player_address = player.address;
    cache.user_id = Player(player.address).GetUserId();
    cache.is_player = true;
    PopulateParts(char_addr, cache);

    cache.displayName = Player(player.address).GetDisplayName();
    if ((cache.displayName.empty() || cache.displayName == "Unknown") && cache.humanoid)
        cache.displayName = Humanoid(cache.humanoid->address).GetDisplayName();
    if (cache.displayName.empty() || cache.displayName == "Unknown")
        cache.displayName = cache.name;

    target[player.address] = std::move(cache);
}

Cheat::PlayerCache Cheat::PlayerHandler::GetCachedPlayer(std::uint64_t address)
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = playerCache.find(address);
    if (it != playerCache.end())
        return it->second;
    return {};
}

std::unordered_map<std::uint64_t, Cheat::PlayerCache> Cheat::PlayerHandler::GetPlayerCache()
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    return playerCache;
}

static bool CacheConsumersActive()
{
    const auto& s = Cheat::g_Settings;

    if (s.esp.enabled || s.esp.preview || s.misc.esp_preview)
        return true;
    if (s.misc.players)
        return true;
    if (s.aim.type != 2)
        return true;

    return false;
}

void Cheat::PlayerHandler::CacheAllPlayers()
{
    if (!Globals::InstanceDataModel.address)
        return;

    std::unordered_map<std::uint64_t, PlayerCache>& fresh = staging;
    const std::unordered_map<std::uint64_t, PlayerCache>& previous = previousSnapshot;
    fresh.clear();

    if (Games::PhantomForces::IsActivePlace())
    {

        Games::PhantomForces::MergePlayers(fresh);
    }
    else
    {
        if (Globals::Players && g_Memory.IsValid(Globals::Players->address))
        {
            auto players = Globals::Players->GetChildren();
            fresh.reserve(players.size());
            for (const auto& player : players)
                UpdateCache(player, fresh);
        }

        bool any_parts = false;
        for (const auto& [addr, cache] : fresh)
        {
            if (cache.humanoidRootPart)
            {
                any_parts = true;
                break;
            }
        }

        if (!any_parts)
        {
            fresh.clear();
            CacheFromWorkspace(fresh);
        }
    }

    MergeStreamedBots(fresh);

    if (Games::ApocalypseRising::IsActivePlace())
        Games::ApocalypseRising::MergeEntities(fresh);

    if (g_Settings.esp.body_corpse)
    {
        std::unordered_map<std::uint64_t, bool> still_players;
        if (Globals::Players && g_Memory.IsValid(Globals::Players->address))
        {
            for (const auto& player : Globals::Players->GetChildren())
                still_players[player.address] = true;
        }

        for (auto& [addr, cur] : fresh)
        {
            const auto prev_it = previous.find(addr);
            const bool dead = IsPlayerDead(cur);

            if (dead)
            {
                cur.is_corpse = true;

                if (!RefreshCorpseFromCharacter(cur, cur.character))
                {
                    if (prev_it != previous.end())
                    {
                        if (!AnyBodyPartValid(cur) && AnyBodyPartValid(prev_it->second))
                            CopyBodyParts(cur, prev_it->second);
                        if (!g_Memory.IsValid(cur.character) && prev_it->second.character)
                            cur.character = prev_it->second.character;
                        RefreshCorpseFromCharacter(cur, cur.character);
                        if (!AnyBodyPartValid(cur) && AnyBodyPartValid(prev_it->second))
                            CopyBodyParts(cur, prev_it->second);
                    }
                }
            }

            else
            {
                cur.is_corpse = false;
            }
        }

        for (const auto& [addr, prev] : previous)
        {
            if (fresh.find(addr) != fresh.end())
                continue;
            if (!still_players.count(addr))
                continue;
            if (!prev.is_player && !prev.is_corpse)
                continue;

            PlayerCache corpse;
            if (RetainCorpseEntry(corpse, prev) && AnyBodyPartValid(corpse))
                fresh[addr] = std::move(corpse);
        }
    }

    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        playerCache.swap(fresh);
    }

    previousSnapshot.swap(fresh);
    fresh.clear();
}

void Cheat::PlayerHandler::CacheThreadLoop()
{

    static std::once_flag s_watchdog_once;
    std::call_once(s_watchdog_once, []
    {
        std::thread([]
        {
            std::uint64_t seen = tickCount.load(std::memory_order_relaxed);
            int stalled = 0;

            for (;;)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                const std::uint64_t now = tickCount.load(std::memory_order_relaxed);
                if (now != seen)
                {
                    seen = now;
                    stalled = 0;
                    continue;
                }

                ++stalled;
                if (stalled == 3 || stalled % 10 == 0)
                {
                    Cheat::Console::Log(Cheat::Console::Color::Red,
                        "cache STALLED %ds | phase=%s | ticks=%llu | last=%ums",
                        stalled, phase.load(std::memory_order_relaxed),
                        (unsigned long long)now,
                        (unsigned)lastTickMs.load(std::memory_order_relaxed));
                }
            }
        }).detach();
    });

    while (shouldRun.load())
    {
        const ULONGLONG tick_start = GetTickCount64();

        phase.store("globals", std::memory_order_relaxed);
        RefreshGlobals();

        phase.store("players", std::memory_order_relaxed);
        CacheAllPlayers();

        phase.store("raycast", std::memory_order_relaxed);
        Features::RaycastEngine::Tick();

        if (g_Settings.esp.enabled && g_Settings.esp.chams_mode == 4)
        {
            phase.store("mesh_cache", std::memory_order_relaxed);
            Visuals::MeshCache::Get().Refresh(false);
        }

        const int cost = (int)(GetTickCount64() - tick_start);
        lastTickMs.store((std::uint32_t)(cost < 0 ? 0 : cost), std::memory_order_relaxed);
        tickCount.fetch_add(1, std::memory_order_relaxed);

        int target = 250;
        if (CacheConsumersActive())
            target = 40;
        if (g_Settings.esp.enabled && g_Settings.esp.chams_mode == 4)
            target = (std::min)(target, 100);
        if (Features::RaycastEngine::WantsCache())
            target = (std::min)(target, 20);

        int sleep_ms = target - cost;
        if (sleep_ms < 5)
            sleep_ms = 5;

        tickIntervalMs.store((std::uint32_t)target, std::memory_order_relaxed);
        phase.store("idle", std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
}

void Cheat::PlayerHandler::ClearCache()
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    playerCache.clear();
}

std::size_t Cheat::PlayerHandler::GetPlayerCount()
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    return playerCache.size();
}

std::uint64_t Cheat::PlayerHandler::TickCount()
{
    return tickCount.load(std::memory_order_relaxed);
}

const char* Cheat::PlayerHandler::Phase()
{
    return phase.load(std::memory_order_relaxed);
}

int Cheat::PlayerHandler::LastTickMs()
{
    return (int)lastTickMs.load(std::memory_order_relaxed);
}

int Cheat::PlayerHandler::TickIntervalMs()
{
    return (int)tickIntervalMs.load(std::memory_order_relaxed);
}

void Cheat::PlayerHandler::StartCacheThread()
{
    if (shouldRun.load())
        return;
    shouldRun.store(true);
    cacheThread = std::thread(CacheThreadLoop);
}

void Cheat::PlayerHandler::StopCacheThread()
{
    if (!shouldRun.load())
        return;

    shouldRun.store(false);
    if (cacheThread.joinable())
        cacheThread.join();
}
