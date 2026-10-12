#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Cheat {
namespace Features {
namespace ScriptBytecode {

bool IsScriptClass(const std::string& cls);

bool ReadRaw(std::uint64_t script_addr, const std::string& class_name, std::vector<std::uint8_t>& out);

bool Normalize(const std::vector<std::uint8_t>& raw, std::vector<std::uint8_t>& out, std::string* status = nullptr);

bool Decompress(const std::vector<std::uint8_t>& raw, std::vector<std::uint8_t>& out);

bool ReadDecompressed(std::uint64_t script_addr, const std::string& class_name, std::vector<std::uint8_t>& out);

std::string Decompile(const std::vector<std::uint8_t>& raw_or_luau, const char* chunk_name = "script");

std::string DecompileViaFission(const std::vector<std::uint8_t>& luau_bc);

bool EnsureFissionServer();

}
}
}
