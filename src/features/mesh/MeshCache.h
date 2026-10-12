#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mesh
{

	inline bool valid_ptr(std::uint64_t a)
	{
		return a >= 0x10000ull && a < 0x00007FFFFFFFFFFFull;
	}

#pragma pack(push, 1)

	struct vertex_t
	{
		float          pos[3];
		float          normal[3];
		float          uv[2];
		std::uint32_t  tangent;
		std::uint32_t  color;
	};
	static_assert(sizeof(vertex_t) == 40, "mesh vertex layout");

	struct face_t
	{
		std::uint32_t indices[3];
	};
	static_assert(sizeof(face_t) == 12, "mesh face layout");
#pragma pack(pop)

	struct cached_mesh_t
	{
		std::string            asset_id;
		std::vector<vertex_t>  vertices;
		std::vector<face_t>    faces;
	};

	class cache_t
	{
	public:
		static cache_t& get();

		void refresh(bool force = false);

		std::shared_ptr<const cached_mesh_t> find(const std::string& asset_id) const;

		std::size_t count() const;

	private:
		cache_t() = default;

		mutable std::mutex mtx_;
		std::unordered_map<std::string, std::shared_ptr<cached_mesh_t>> map_;
		std::uint64_t      last_refresh_{ 0 };
	};

	std::string clean_asset_id(const std::string& raw);

	void collect_keys(const std::string& raw, const std::string& cleaned,
		std::vector<std::string>& keys);
}
