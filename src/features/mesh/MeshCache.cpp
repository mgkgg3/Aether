#include "MeshCache.h"

#include <cmath>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <sdk/sdk.h>
#include <game/game.h>

namespace mesh
{
	namespace
	{

		std::uint64_t find_provider()
		{
			const std::uint64_t dm = game::datamodel.address;
			if (!valid_ptr(dm))
				return 0;

			for (const auto& child : rbx::instance_t(dm).get_children())
			{
				if (!valid_ptr(child.address))
					continue;
				if (child.get_class_name() == "MeshContentProvider")
					return child.address;
			}
			return 0;
		}

		bool looks_like_mesh_data(std::uint64_t md, int& vtx_count, int& fac_count,
			std::uint64_t& vtx_start, std::uint64_t& fac_start)
		{
			if (!valid_ptr(md))
				return false;

			vtx_start = memory->read<std::uint64_t>(md + Offsets::FileMeshData::Vertices);
			const std::uint64_t vtx_end =
				memory->read<std::uint64_t>(md + Offsets::FileMeshData::VerticesEnd);
			fac_start = memory->read<std::uint64_t>(md + Offsets::FileMeshData::Faces);
			const std::uint64_t fac_end =
				memory->read<std::uint64_t>(md + Offsets::FileMeshData::FacesEnd);

			vtx_count = (vtx_end > vtx_start)
				? (int)((vtx_end - vtx_start) / sizeof(vertex_t)) : 0;
			fac_count = (fac_end > fac_start)
				? (int)((fac_end - fac_start) / sizeof(face_t)) : 0;

			return vtx_count > 0 && vtx_count < 50000 &&
			       fac_count > 0 && fac_count < 100000 &&
			       valid_ptr(vtx_start) && valid_ptr(fac_start);
		}

		std::uint64_t node_mesh_data(std::uint64_t node, int& vtx_count, int& fac_count,
			std::uint64_t& vtx_start, std::uint64_t& fac_start)
		{
			std::uint64_t p = memory->read<std::uint64_t>(node + 0x40);
			if (valid_ptr(p))
			{
				std::uint64_t md = memory->read<std::uint64_t>(p + 0x40);
				if (looks_like_mesh_data(md, vtx_count, fac_count, vtx_start, fac_start))
					return md;
				if (looks_like_mesh_data(p, vtx_count, fac_count, vtx_start, fac_start))
					return p;
			}

			p = memory->read<std::uint64_t>(node + Offsets::LRUNode::CachedItem);
			if (valid_ptr(p))
			{
				const std::uint64_t md =
					memory->read<std::uint64_t>(p + Offsets::CachedItem::FileMeshData);
				if (looks_like_mesh_data(md, vtx_count, fac_count, vtx_start, fac_start))
					return md;
				if (looks_like_mesh_data(p, vtx_count, fac_count, vtx_start, fac_start))
					return p;
			}

			return 0;
		}
	}

	void collect_keys(const std::string& raw, const std::string& cleaned,
		std::vector<std::string>& keys)
	{
		auto add = [&](const std::string& k)
		{
			if (k.empty())
				return;
			for (const auto& e : keys)
				if (e == k)
					return;
			keys.push_back(k);
		};

		add(cleaned);
		add(raw);
		if (raw.rfind("rbxasset://", 0) == 0)
			add(raw.substr(11));
		if (raw.rfind("rbxassetid://", 0) == 0)
			add(raw.substr(13));
	}

	std::string clean_asset_id(const std::string& raw)
	{
		if (raw.empty() || raw == "Unknown")
			return {};

		if (raw.rfind("rbxassetid://", 0) == 0)
			return raw.substr(13);

		const auto q = raw.find("?id=");
		if (q != std::string::npos && q + 4 < raw.size())
		{
			std::string id = raw.substr(q + 4);
			const auto end = id.find_first_of("& \n\r\t");
			if (end != std::string::npos)
				id.resize(end);
			return id;
		}

		const auto pos = raw.find_last_of("=/");
		if (pos != std::string::npos && pos + 1 < raw.size())
			return raw.substr(pos + 1);

		return raw;
	}

	cache_t& cache_t::get()
	{
		static cache_t inst;
		return inst;
	}

	void cache_t::refresh(bool force)
	{
		const std::uint64_t now = GetTickCount64();
		if (last_refresh_)
		{
			const std::uint64_t gap = force ? 1000ull : 15000ull;
			if ((now - last_refresh_) < gap)
				return;
		}

		const std::uint64_t mcp = find_provider();
		if (!valid_ptr(mcp))
			return;

		static const std::uintptr_t k_cache_offs[] = {
			Offsets::MeshContentProvider::LRUHolder, 0xd8, 0xf0, 0xc8, 0xe0, 0xe8
		};

		std::uint64_t cache_obj = 0, lru = 0, sentinel = 0, node = 0;
		for (std::uintptr_t off : k_cache_offs)
		{
			const std::uint64_t c = memory->read<std::uint64_t>(mcp + off);
			if (!valid_ptr(c))
				continue;
			const std::uint64_t l = memory->read<std::uint64_t>(c + 0x20);
			if (!valid_ptr(l))
				continue;
			const std::uint64_t s = memory->read<std::uint64_t>(l + 0x08);
			if (!valid_ptr(s))
				continue;
			const std::uint64_t n = memory->read<std::uint64_t>(s);
			if (!valid_ptr(n) || n == s)
				continue;

			cache_obj = c;
			lru = l;
			sentinel = s;
			node = n;
			break;
		}
		(void)lru;

		if (!cache_obj || !node)
			return;

		last_refresh_ = now;

		std::unordered_map<std::string, std::shared_ptr<cached_mesh_t>> fresh;
		int remaining = 400;
		int consecutive_cached = 0;

		while (valid_ptr(node) && node != sentinel && remaining-- > 0)
		{
			const std::string raw_id = memory->read_string(node + Offsets::LRUNode::AssetID);
			const std::string id = clean_asset_id(raw_id);

			if (!id.empty() && fresh.find(id) == fresh.end())
			{
				std::shared_ptr<cached_mesh_t> existing;
				{
					std::lock_guard<std::mutex> lk(mtx_);
					const auto it = map_.find(id);
					if (it != map_.end())
						existing = it->second;
				}

				if (existing)
				{
					std::vector<std::string> keys;
					collect_keys(raw_id, id, keys);
					for (const auto& k : keys)
						fresh.emplace(k, existing);

					if (++consecutive_cached > 15)
						break;

					node = memory->read<std::uint64_t>(node + Offsets::LRUNode::Next);
					continue;
				}
				consecutive_cached = 0;

				int vtx_count = 0, fac_count = 0;
				std::uint64_t vtx_start = 0, fac_start = 0;
				const std::uint64_t md = node_mesh_data(node, vtx_count, fac_count,
					vtx_start, fac_start);

				if (md && vtx_count > 0)
				{
					auto entry = std::make_shared<cached_mesh_t>();
					entry->asset_id = id;
					entry->vertices.resize((std::size_t)vtx_count);
					entry->faces.resize((std::size_t)fac_count);

					const std::size_t vb = (std::size_t)vtx_count * sizeof(vertex_t);
					const std::size_t fb = (std::size_t)fac_count * sizeof(face_t);
					if (memory->read_raw(vtx_start, entry->vertices.data(), vb) == vb &&
						memory->read_raw(fac_start, entry->faces.data(), fb) == fb)
					{

						if (std::isfinite(entry->vertices[0].pos[0]))
						{
							std::vector<std::string> keys;
							collect_keys(raw_id, id, keys);
							for (const auto& k : keys)
								fresh.emplace(k, entry);
						}
					}
				}
			}

			node = memory->read<std::uint64_t>(node + Offsets::LRUNode::Next);
		}

		std::lock_guard<std::mutex> lk(mtx_);
		for (auto& kv : fresh)
			map_[kv.first] = std::move(kv.second);
	}

	std::shared_ptr<const cached_mesh_t> cache_t::find(const std::string& asset_id) const
	{
		if (asset_id.empty() || asset_id == "Unknown")
			return nullptr;

		std::vector<std::string> keys;
		collect_keys(asset_id, clean_asset_id(asset_id), keys);

		std::lock_guard<std::mutex> lk(mtx_);
		for (const auto& k : keys)
		{
			const auto it = map_.find(k);
			if (it == map_.end() || !it->second)
				continue;
			return it->second;
		}
		return nullptr;
	}

	std::size_t cache_t::count() const
	{
		std::lock_guard<std::mutex> lk(mtx_);
		return map_.size();
	}
}
