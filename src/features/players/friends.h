#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>

namespace friends
{
	inline std::mutex                         mtx;
	inline std::unordered_set<std::uint64_t>  list;

	inline bool is_friend(std::uint64_t instance)
	{
		if (!instance)
			return false;

		std::lock_guard<std::mutex> lk(mtx);
		return list.count(instance) != 0;
	}

	inline bool is_skipped(std::uint64_t instance, std::uint64_t target)
	{
		if (!instance || instance == target)
			return false;

		return is_friend(instance);
	}

	inline bool toggle(std::uint64_t instance)
	{
		if (!instance)
			return false;

		std::lock_guard<std::mutex> lk(mtx);
		if (list.erase(instance) != 0)
			return false;

		list.insert(instance);
		return true;
	}

	inline void clear()
	{
		std::lock_guard<std::mutex> lk(mtx);
		list.clear();
	}
}
