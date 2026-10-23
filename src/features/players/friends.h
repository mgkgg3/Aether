#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>

// Players the user marked as friends in the Players tab.
//
// Friends are read from the aim features' threads and written from the UI
// thread, so the set is mutex guarded. A friend is never picked up by aimbot,
// silent aim or the triggerbot, and is highlighted instead - unless the user
// explicitly targeted them, which outranks the friend flag everywhere.
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

	// True when the player should be passed over by the aim features. The explicit
	// target - whoever the user picked in the Players tab - is never skipped, so a
	// friend the user aimed at stays a valid target.
	inline bool is_skipped(std::uint64_t instance, std::uint64_t target)
	{
		if (!instance || instance == target)
			return false;

		return is_friend(instance);
	}

	// Returns true when the player is a friend afterwards.
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
