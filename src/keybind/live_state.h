#pragma once
#include <atomic>

namespace live_state
{
    enum slot : int
    {
        kAimbot = 0,
        kSilentAim,
        kTriggerbot,
        kSpeed,
        kFly,
        kNoclip,
        kSpin360,
        kWalkspeed,
        kSonar,
        kCount
    };

    inline std::atomic<bool> g_live[kCount] = {};

    inline void set(int s, bool on)
    {
        if (s >= 0 && s < kCount) g_live[s].store(on, std::memory_order_relaxed);
    }

    inline bool get(int s)
    {
        return s >= 0 && s < kCount && g_live[s].load(std::memory_order_relaxed);
    }

    inline void clear()
    {
        for (int i = 0; i < kCount; ++i) g_live[i].store(false, std::memory_order_relaxed);
    }
}
