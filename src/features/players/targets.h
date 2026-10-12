#pragma once

#include <cstdint>
#include <mutex>

namespace targets
{
	inline std::mutex    mtx;
	inline std::uint64_t pinned = 0;

	inline std::uint64_t get()
	{
		std::lock_guard<std::mutex> lk(mtx);
		return pinned;
	}

	inline void set(std::uint64_t instance)
	{
		std::lock_guard<std::mutex> lk(mtx);
		pinned = instance;
	}

	inline void clear()
	{
		set(0);
	}

	inline bool is_target(std::uint64_t instance)
	{
		if (!instance)
			return false;

		std::lock_guard<std::mutex> lk(mtx);
		return pinned == instance;
	}
}
