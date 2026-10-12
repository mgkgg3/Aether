#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include <cache/cache.h>
#include <sdk/math/math.h>

namespace op1
{

	void detect();

	bool active();

	std::uint64_t place_id();
	std::uint64_t game_id();
	const char*   place_name();
	const char*   detection_source();

	bool aliases_enabled();
	bool bodies_enabled();
	bool collision_boxes_enabled();
	bool engine_matrix_enabled();
	bool multipoint_los_enabled();
	bool live_health_enabled();
	bool no_smoothing_enabled();
	int  sticky_ms();

	bool marked_model(std::uint64_t model);

	bool model_in_workspace(std::uint64_t model);

	bool attach_viewmodel(cache::entity_t& entity, const math::vector3& anchor,
		std::unordered_set<std::uint64_t>& claimed);

	int body_part_count(const cache::entity_t& entity);

	bool parts_contain_body(
		const std::unordered_map<std::string, rbx::part_t>& parts);

	bool has_animated_body(const cache::entity_t& entity);

	bool is_collider_body(const std::string& name);

	const std::vector<const char*>& body_parts();

	enum class bone : int
	{
		head = 0, torso, left_arm, right_arm, left_leg, right_leg, root, count
	};

	const std::vector<const char*>& bone_aliases(bone b);

	const rbx::part_t* find_bone(const cache::entity_t& entity, bone b,
		std::string* found_name = nullptr);

	const rbx::part_t* find_part(const cache::entity_t& entity, const char* name,
		std::string* found = nullptr);

	bool is_hull_name(const std::string& name);

	bool is_swinging_limb(const std::string& name);

	bool head_mesh_fallback();

	bool corpse(const cache::entity_t& entity);

	const std::vector<const char*>& box_primary();
	const std::vector<const char*>& box_fallback();

	const std::vector<const char*>& los_points();

	bool part_position(const rbx::part_t& part, math::vector3& out);
	bool part_geometry(const rbx::part_t& part, math::vector3& pos,
		math::vector3* size, math::matrix3* rot);

	bool live_position(const rbx::part_t& part, math::vector3& out);

	bool head_point(const cache::entity_t& entity, math::vector3& out);
	bool chest_point(const cache::entity_t& entity, math::vector3& out);

	bool aim_point(const cache::entity_t& entity, bone b, math::vector3& out);

	bool visible_any(const cache::entity_t& entity, const math::vector3& from,
		const math::vector3& primary);

	bool read_health(std::uint64_t humanoid, float& health, float& max_health);

	void publish_health(std::uint64_t humanoid);

	void publish_model_source(const char* source);
	void publish_cache_stats(int entities, int held_lists, int unresolved);
	void publish_corpses(int count);
	void publish_matrix_source(const char* source);

	struct status_t
	{
		bool        detected{ false };
		bool        enabled{ false };
		bool        forced{ false };
		const char* place{ nullptr };
		const char* source{ nullptr };
		const char* model{ nullptr };
		const char* matrix{ nullptr };
		int         entities{ 0 };
		int         held{ 0 };
		int         unresolved{ 0 };
		int         bodies{ 0 };
		int         viewmodels{ 0 };
		int         body_parts{ 0 };
		int         corpses{ 0 };
		float       health{ 0.f };
		float       max_health{ 0.f };
		bool        health_valid{ false };

		float       alt_health{ 0.f };
		float       alt_max_health{ 0.f };
		bool        alt_health_valid{ false };
	};

	status_t status();

	void publish_animated_bodies(int count);
}
