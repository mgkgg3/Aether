#pragma once

#include <cstdint>

namespace Cheat {
namespace Features {
namespace InstanceCreate {

bool New(const char* className, std::uint64_t parent, std::uint64_t* out_addr);
bool SetParent(std::uint64_t inst, std::uint64_t parent);

bool SetString(std::uint64_t field, const char* text);

bool SetContent(std::uint64_t string_field, const char* text);

int LastFail();

}
}
}
