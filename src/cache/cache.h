#pragma once
#include <string>
#include <mutex>
#include <atomic>
#include <cmath>
#include <unordered_map>

#include <sdk/sdk.h>
#include <sdk/math/math.h>

namespace cache
{
	inline std::mutex mtx;






	inline std::atomic<std::uint64_t> tick_count{ 0 };
	inline std::atomic<const char*>   phase{ "idle" };

	struct part_data_t
	{
		rbx::part_t     part;
		std::string     name;
		math::vector3   position;
		math::vector3   size;
		math::matrix3   rotation;
	};

	struct entity_t final
	{
		rbx::instance_t instance;
		std::string     name;
		std::string     display_name;




		std::int64_t    user_id{ 0 };

		std::uint8_t    rig_type;

		rbx::humanoid_t humanoid;
		std::unordered_map<std::string, rbx::part_t> parts;
		std::string     tool_name;




		std::uint64_t   team{ 0 };






		std::string     team_name;
		std::uint32_t   team_color{ 0 };
		float           health{ 0.f };
		float           max_health{ 0.f };



		int             state{ -1 };
		bool            knocked{ false };

		float           armor     = -1.f;
		float           max_armor = 100.f;
	};

	inline cache::entity_t cached_local_player;
	inline std::vector<cache::entity_t> cached_players;






	inline std::atomic<std::uint64_t> cached_local_team{ 0 };





	inline std::string cached_local_team_name;











	inline float part_half_height(const math::vector3& size, const math::matrix3& rot)
	{
		return 0.5f * (std::fabs(rot.m[3]) * size.x +
		               std::fabs(rot.m[4]) * size.y +
		               std::fabs(rot.m[5]) * size.z);
	}







	bool is_teammate(const entity_t& e);















	bool is_dead(const entity_t& e);

	void run();
}
