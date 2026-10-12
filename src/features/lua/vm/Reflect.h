#pragma once

#include <cstdint>

namespace Cheat {
namespace Features {
namespace Reflect {

std::uint64_t Name(std::uintptr_t base, const char* text);
std::uint64_t Creator(std::uintptr_t base, std::uint64_t name);

}
}
}
