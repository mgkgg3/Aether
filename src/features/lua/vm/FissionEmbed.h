#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace Cheat {
namespace Features {
namespace FissionEmbed {

std::string DecompileLuauBytecode(const std::uint8_t* data, std::size_t n);

void UnescapeLuaByteEscapes(std::string& src);

}
}
}
