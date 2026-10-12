#pragma once

#include <cstdint>

namespace Cheat {
namespace Features {
namespace CallGate {

bool Install(const char* method_name = nullptr);
void Remove();
bool Ready();

bool Invoke(std::uint64_t fn,
            std::uint64_t a0, std::uint64_t a1,
            std::uint64_t a2, std::uint64_t a3,
            std::uint64_t* out_ret, unsigned timeout_ms = 3000);

std::uint64_t Scratch();

std::uint64_t Calls();

std::uint64_t SlotAddress();
int LastFail();

}
}
}
