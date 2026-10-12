#include "rescan.h"

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <algorithm>

#include <Windows.h>

#include <memory/memory.h>
#include <debug/DebugConsole.h>
#include <game/game_context.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <game/game.h>

namespace AutoRescan
{
    std::atomic<bool>   active{ true };
    std::mutex          updateMutex;
    std::atomic<bool>   needsRescan{ false };
    std::atomic<bool>   forceRescan{ false };
    std::atomic<uint64_t>  lastGameId{ 0 };
    std::atomic<uint64_t>  lastPlaceId{ 0 };
    std::atomic<uintptr_t> lastWorkspace{ 0 };
    std::atomic<uintptr_t> lastPlayers{ 0 };
    std::atomic<uintptr_t> lastDataModel{ 0 };
    std::atomic<uintptr_t> lastLocalPlayer{ 0 };
    std::atomic<DWORD>  lastProcessId{ 0 };

    constexpr int LOBBY_SCAN_MS      = 800;
    constexpr int LOADING_SCAN_MS    = 300;
    constexpr int INGAME_SCAN_MS     = 600;
    constexpr int TRANSITION_SCAN_MS = 200;
    constexpr int ERROR_RECOVERY_MS  = 1500;
    constexpr int MEMORY_CHECK_MS    = 3000;
    constexpr int PROCESS_CHECK_MS   = 2000;

    std::atomic<int>  consecutiveSuccesses{ 0 };
    std::atomic<int>  consecutiveFailures{ 0 };
    std::atomic<bool> inTransition{ false };
    std::atomic<bool> memoryHealthy{ true };
    std::atomic<std::chrono::steady_clock::time_point> lastStateChange;
    std::atomic<std::chrono::steady_clock::time_point> lastMemoryCheck;
}

struct GameState
{
    uint64_t  gameId      = 0;
    uint64_t  placeId     = 0;
    uintptr_t workspace   = 0;
    uintptr_t players     = 0;
    uintptr_t datamodel   = 0;
    uintptr_t localplayer = 0;
    DWORD     processId   = 0;

    enum class StateType {
        INVALID, LOBBY_EMPTY, LOBBY_SEARCHING, LOADING, IN_GAME, DISCONNECTING
    };

    StateType GetStateType() const
    {
        if (processId == 0 || datamodel == 0)       return StateType::INVALID;
        if (gameId == 0 && placeId == 0)            return StateType::LOBBY_EMPTY;
        if (gameId == 0 || placeId <= 1000)         return StateType::LOBBY_SEARCHING;
        if (workspace == 0 || players == 0)         return StateType::LOADING;
        if (gameId != 0 && placeId > 1000 &&
            (workspace == 0 || localplayer == 0))   return StateType::DISCONNECTING;
        return StateType::IN_GAME;
    }

    bool IsValid() const { return GetStateType() != StateType::INVALID; }

    bool HasChanged(const GameState& o) const {
        return gameId != o.gameId || placeId != o.placeId ||
               workspace != o.workspace || players != o.players ||
               datamodel != o.datamodel || localplayer != o.localplayer ||
               processId != o.processId;
    }
    bool HasCriticalChange(const GameState& o) const {
        return gameId != o.gameId || placeId != o.placeId ||
               processId != o.processId || datamodel != o.datamodel;
    }
    bool RequiresImmediateRescan(const GameState& o) const {
        return processId != o.processId ||
               (datamodel == 0 && o.datamodel != 0) ||
               (datamodel != 0 && o.datamodel == 0) ||
               (gameId == 0 && o.gameId != 0) ||
               (gameId != 0 && o.gameId == 0);
    }
};

struct SmartCache
{
    GameState   cachedState;
    std::chrono::steady_clock::time_point lastUpdate;
    std::chrono::steady_clock::time_point lastFullScan;
    std::mutex  cacheMutex;
    bool        isValid    = false;
    int         cacheHits  = 0;
    int         totalQueries = 0;

    std::chrono::milliseconds getCacheValidityDuration(GameState::StateType t) const {
        switch (t) {
        case GameState::StateType::LOBBY_EMPTY:    return std::chrono::milliseconds(1500);
        case GameState::StateType::LOBBY_SEARCHING:return std::chrono::milliseconds(800);
        case GameState::StateType::LOADING:        return std::chrono::milliseconds(300);		case GameState::StateType::IN_GAME:        return std::chrono::milliseconds(400);
        case GameState::StateType::DISCONNECTING:  return std::chrono::milliseconds(200);
        default:                                   return std::chrono::milliseconds(400);
        }
    }
    float getCacheHitRate() const {
        return totalQueries > 0 ? (float)cacheHits / totalQueries : 0.f;
    }
} g_smartCache;

static std::mutex g_instanceMutex;

static bool QuickMemoryCheck()
{
    auto now      = std::chrono::steady_clock::now();
    auto lastChk  = AutoRescan::lastMemoryCheck.load();
    auto elapsed  = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChk);

    if (elapsed < std::chrono::milliseconds(AutoRescan::MEMORY_CHECK_MS))
        return AutoRescan::memoryHealthy.load();

    HANDLE h = memory->get_process_handle();
    if (!h || h == INVALID_HANDLE_VALUE) {
        AutoRescan::memoryHealthy = false;
        return false;
    }
    DWORD  probe; SIZE_T rd;
    bool ok = ReadProcessMemory(h, (LPCVOID)memory->get_module_address(),
                                &probe, sizeof(DWORD), &rd) != FALSE;
    AutoRescan::memoryHealthy = ok;
    AutoRescan::lastMemoryCheck = now;
    return ok;
}

static GameState CaptureCurrentState()
{
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
        g_smartCache.totalQueries++;
        if (g_smartCache.isValid) {
            auto validity = g_smartCache.getCacheValidityDuration(
                g_smartCache.cachedState.GetStateType());
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - g_smartCache.lastUpdate);
            if (age < validity && AutoRescan::memoryHealthy.load()) {
                g_smartCache.cacheHits++;
                return g_smartCache.cachedState;
            }
        }
    }

    GameState state;
    try {
        if (!QuickMemoryCheck()) {
            std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
            g_smartCache.isValid = false;
            return state;
        }

        state.processId = memory->get_process_id();
        std::lock_guard<std::mutex> il(g_instanceMutex);

        bool fullScan = false;
        {
            std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - g_smartCache.lastFullScan);
            fullScan = (age > std::chrono::milliseconds(5000)) || !g_smartCache.isValid;
        }

        std::uint64_t fdm = memory->read<std::uint64_t>(
            memory->get_module_address() + Offsets::FakeDataModel::Pointer);
        rbx::instance_t dm{ memory->read<std::uint64_t>(
            fdm + Offsets::FakeDataModel::RealDataModel) };

        if (dm.address)
            game::datamodel = dm;
        state.datamodel   = dm.address;

        if (dm.address) {
            state.gameId  = memory->read<uint64_t>(dm.address + Offsets::DataModel::GameId);
            state.placeId = memory->read<uint64_t>(dm.address + Offsets::DataModel::PlaceId);
        }

        if (fullScan || state.gameId != 0 || state.placeId != 0) {
            rbx::instance_t ws  = dm.find_first_child_by_class("Workspace");
            rbx::instance_t pl  = dm.find_first_child_by_class("Players");
            state.workspace     = ws.address ? ws.address : game::workspace.address;
            state.players       = pl.address ? pl.address : game::players.address;
            if (pl.address)
                state.localplayer = memory->read<uintptr_t>(
                    pl.address + Offsets::Player::LocalPlayer);
            std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
            g_smartCache.lastFullScan = now;
        } else {
            std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
            if (g_smartCache.isValid) {
                state.workspace   = g_smartCache.cachedState.workspace;
                state.players     = g_smartCache.cachedState.players;
                state.localplayer = g_smartCache.cachedState.localplayer;
            }
        }

        {
            std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
            g_smartCache.cachedState = state;
            g_smartCache.lastUpdate  = now;
            g_smartCache.isValid     = true;
        }
    }
    catch (...) {
        std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
        g_smartCache.isValid = false;
        AutoRescan::consecutiveFailures++;
    }
    return state;
}

static bool PerformSmartRescan(GameState::StateType expectedState)
{
    try {
        static uint64_t  last_datamodel = 0;
        static uint64_t  last_visengine = 0;
        static uint64_t  last_workspace = 0;
        static uint64_t  last_players   = 0;
        static uint64_t  last_gameid    = 0;
        static auto lastProcessCheck    = std::chrono::steady_clock::now();

        auto now = std::chrono::steady_clock::now();
        auto timeSinceProc = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastProcessCheck);

        if (timeSinceProc > std::chrono::milliseconds(AutoRescan::PROCESS_CHECK_MS)) {
            DWORD curPID = memory->find_process_id("RobloxPlayerBeta.exe");
            if (curPID != memory->get_process_id() || curPID == 0) {
                if (curPID == 0) return false;
                if (!memory->attach_to_process("RobloxPlayerBeta.exe")) return false;

                if (!memory->find_module_address("RobloxPlayerBeta.exe"))
                    return false;
                debug::warn("re-attached to Roblox (pid %u), base 0x%llx",
                    (unsigned)curPID, (unsigned long long)memory->get_module_address());
            }
            lastProcessCheck = now;
        }

        if (!QuickMemoryCheck()) return false;

        std::lock_guard<std::mutex> il(g_instanceMutex);

        std::uint64_t fdm = memory->read<std::uint64_t>(
            memory->get_module_address() + Offsets::FakeDataModel::Pointer);
        rbx::instance_t dm{ memory->read<std::uint64_t>(
            fdm + Offsets::FakeDataModel::RealDataModel) };

        if (!dm.address) return false;
        game::datamodel = dm;

        uint64_t gameid  = memory->read<uint64_t>(dm.address + Offsets::DataModel::GameId);
        uint64_t placeid = memory->read<uint64_t>(dm.address + Offsets::DataModel::PlaceId);

        bool needAll = (expectedState == GameState::StateType::IN_GAME   ||
                        expectedState == GameState::StateType::LOADING    ||
                        gameid != 0 || placeid != 0);

        if (needAll) {
            rbx::instance_t ws_new = dm.find_first_child_by_class("Workspace");
            rbx::instance_t pl_new = dm.find_first_child_by_class("Players");

            if (ws_new.address) game::workspace = ws_new;
            if (pl_new.address) game::players   = pl_new;

            if (game::players.address) {
                std::uint64_t lp_addr = memory->read<std::uint64_t>(
                    game::players.address + Offsets::Player::LocalPlayer);

                if (lp_addr)
                    game::local_player = { lp_addr };

                if (game::local_player.address)
                    game::local_character = { game_ctx::resolve_local_character() };
            }
        }

        {
            std::uint64_t ve = memory->read<std::uint64_t>(
                memory->get_module_address() + Offsets::VisualEngine::Pointer);

            if (ve)
                game::visengine = { ve };
        }

        bool changed = (last_datamodel != game::datamodel.address ||
                        last_visengine != game::visengine.address  ||
                        last_workspace != game::workspace.address  ||
                        last_players   != game::players.address    ||
                        last_gameid    != gameid);

        if (changed) {
            debug::log("rescan",
                "dm=0x%llx ve=0x%llx ws=0x%llx pl=0x%llx gid=%llu",
                game::datamodel.address, game::visengine.address,
                game::workspace.address, game::players.address, gameid);
        }

        last_datamodel = game::datamodel.address;
        last_visengine = game::visengine.address;
        last_workspace = game::workspace.address;
        last_players   = game::players.address;
        last_gameid    = gameid;

        AutoRescan::consecutiveSuccesses++;
        AutoRescan::consecutiveFailures = 0;
        return true;
    }
    catch (...) {
        AutoRescan::consecutiveFailures++;
        AutoRescan::consecutiveSuccesses = 0;
        return false;
    }
}

static void SmartChangeDetectionLoop()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    GameState             prevState;
    GameState::StateType  prevType      = GameState::StateType::INVALID;
    auto                  lastRescanTime = std::chrono::steady_clock::now();

    auto getSmartDelay = [&](GameState::StateType type, bool hasChanges, bool transition) -> int {
        int  fail    = AutoRescan::consecutiveFailures.load();
        if (fail > 3) return AutoRescan::ERROR_RECOVERY_MS;
        if (fail > 0) return 1000 + fail * 200;

        int   succ   = AutoRescan::consecutiveSuccesses.load();
        float bonus  = (std::min)(0.3f, succ * 0.05f);

        if (transition || hasChanges)
            return (int)(AutoRescan::TRANSITION_SCAN_MS * (1.f - bonus));

        int base;
        switch (type) {
        case GameState::StateType::LOBBY_EMPTY:    base = AutoRescan::LOBBY_SCAN_MS;    break;
        case GameState::StateType::LOBBY_SEARCHING:base = AutoRescan::LOBBY_SCAN_MS-200;break;
        case GameState::StateType::LOADING:        base = AutoRescan::LOADING_SCAN_MS;  break;
        case GameState::StateType::IN_GAME:        base = AutoRescan::INGAME_SCAN_MS;   break;
        case GameState::StateType::DISCONNECTING:  base = AutoRescan::LOADING_SCAN_MS;  break;
        default:                                   base = 600;                           break;
        }
        return (int)(base * (1.f - bonus));
    };

    while (AutoRescan::active.load()) {
        bool hadChanges = false;
        try {
            GameState             cur     = CaptureCurrentState();
            GameState::StateType  curType = cur.GetStateType();

            bool anyChange      = cur.HasChanged(prevState);
            bool critChange     = cur.HasCriticalChange(prevState);
            bool immediate      = cur.RequiresImmediateRescan(prevState);
            bool typeChanged    = (curType != prevType);
            bool forced         = AutoRescan::needsRescan.exchange(false) ||
                                  AutoRescan::forceRescan.exchange(false);
            hadChanges          = anyChange || typeChanged || forced || immediate;

            bool transition = typeChanged || immediate;
            if (transition) {
                AutoRescan::inTransition  = true;
                AutoRescan::lastStateChange = std::chrono::steady_clock::now();
            } else {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - AutoRescan::lastStateChange.load());
                if (elapsed > std::chrono::milliseconds(2000))
                    AutoRescan::inTransition = false;
            }

            if (anyChange || typeChanged || forced || immediate) {
                auto now     = std::chrono::steady_clock::now();
                auto sinceRs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - lastRescanTime);

                int cooldown = immediate || forced ? 50 :
                               critChange          ? 200 :
                               typeChanged         ? 300 : 500;

                if (sinceRs.count() >= cooldown || forced || immediate) {
                    std::lock_guard<std::mutex> lk(AutoRescan::updateMutex);
                    bool ok    = false;
                    int retries = immediate ? 3 : 2;
                    for (int r = 0; r < retries && !ok; r++) {
                        if (r) std::this_thread::sleep_for(
                            std::chrono::milliseconds(200 * r));
                        ok = PerformSmartRescan(curType);
                    }
                    if (ok) {
                        std::lock_guard<std::mutex> cl(g_smartCache.cacheMutex);
                        g_smartCache.isValid = false;
                        prevState = CaptureCurrentState();
                        prevType  = prevState.GetStateType();
                        lastRescanTime = now;
                    }
                }
            } else {
                if (cur.IsValid()) { prevState = cur; prevType = curType; }
            }
        }
        catch (...) { AutoRescan::consecutiveFailures++; }

        int delay = getSmartDelay(prevType, hadChanges,
                                  AutoRescan::inTransition.load());
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}

static void GameTransitionMonitor()
{
    GameState::StateType lastType = GameState::StateType::INVALID;
    auto stateChangeTime = std::chrono::steady_clock::now();

    while (AutoRescan::active.load()) {
        try {
            GameState            cur     = CaptureCurrentState();
            GameState::StateType curType = cur.GetStateType();

            if (curType != lastType) {
                stateChangeTime = std::chrono::steady_clock::now();
                lastType = curType;
                if (curType == GameState::StateType::LOADING   ||
                    curType == GameState::StateType::LOBBY_EMPTY ||
                    curType == GameState::StateType::DISCONNECTING)
                    AutoRescan::forceRescan = true;
            }

            auto now       = std::chrono::steady_clock::now();
            auto inState   = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - stateChangeTime);

            switch (curType) {
            case GameState::StateType::LOADING:
                if (inState.count() > 12000) {
                    AutoRescan::forceRescan = true; stateChangeTime = now; }
                break;
            case GameState::StateType::LOBBY_EMPTY:
                if (inState.count() > 5000) {
                    AutoRescan::needsRescan = true; stateChangeTime = now; }
                break;
            case GameState::StateType::DISCONNECTING:
                if (inState.count() > 8000) {
                    AutoRescan::forceRescan = true; stateChangeTime = now; }
                break;
            default: break;
            }
        }
        catch (...) {}

        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
}

bool InitializeStorage()
{
    return PerformSmartRescan(GameState::StateType::LOBBY_EMPTY);
}

void AutoRescanHandler()
{
    GameState init = CaptureCurrentState();
    AutoRescan::lastGameId    = init.gameId;
    AutoRescan::lastPlaceId   = init.placeId;
    AutoRescan::lastStateChange  = std::chrono::steady_clock::now();
    AutoRescan::lastMemoryCheck  = std::chrono::steady_clock::now();

    std::thread(SmartChangeDetectionLoop).detach();
    std::thread(GameTransitionMonitor).detach();

    while (AutoRescan::active.load())
        std::this_thread::sleep_for(std::chrono::seconds(2));
}

void StopAutoRescan()
{
    AutoRescan::active = false;
    std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
    g_smartCache.isValid = false;
}

void RequestRescan() { AutoRescan::needsRescan = true; }

void ForceRescan()
{
    AutoRescan::forceRescan = true;
    std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
    g_smartCache.isValid = false;
    AutoRescan::consecutiveFailures = 0;
}

float GetCacheEfficiency()
{
    std::lock_guard<std::mutex> lk(g_smartCache.cacheMutex);
    return g_smartCache.getCacheHitRate();
}
