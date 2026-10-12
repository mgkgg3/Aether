#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <sdk/sdk.h>

namespace mesh
{
	namespace parser
	{
		enum class kind : std::uint8_t
		{
			body = 0,
			accessory = 1,
			face = 2,
			hair = 3,
			special = 5,
			other = 6
		};

		struct entry_t
		{
			std::uint64_t part{ 0 };
			std::string   name;
			kind          what{ kind::other };
		};

		std::vector<entry_t> collect(
			const std::unordered_map<std::string, rbx::part_t>& parts);

		const char* kind_name(kind k);
	}
}
