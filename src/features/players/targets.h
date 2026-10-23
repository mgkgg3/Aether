#pragma once

#include <cstdint>
#include <mutex>

// The player the user pinned with the Players tab target button.
//
// This is deliberately not the aimbot's sticky target. That one is transient: it
// is dropped whenever the key is released, the entity leaves the cache, or a
// validity read fails, so it could never hold a player the aim features were
// asked to focus on. The pin lives here instead, survives all of that, and is
// read from the feature threads while the UI thread writes it.
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
