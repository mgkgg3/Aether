
#include "MeshChams.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include <game/game.h>
#include <memory/memory.h>
#include <features/opone/OperationOne.h>
#include <sdk/offsets.h>

#include "MeshCache.h"
#include "MeshDxShader.h"
#include "MeshMath.h"
#include "MeshParser.h"

using namespace Mesh;

namespace mesh
{
	namespace
	{

		bool w2s(const math::matrix4& view, const math::vector2& dims,
			const Vector3& p, Vector2& out)
		{
			math::vector2 s{};
			if (!game::visengine.world_to_screen(math::vector3{ p.x, p.y, p.z }, s, dims, view))
				return false;
			out.x = s.x;
			out.y = s.y;
			return true;
		}

		bool part_frame_data(std::uint64_t part, Vector3& pos, Matrix4x4& rot, Vector3& sz)
		{
			if (!valid_ptr(part))
				return false;

			const std::uint64_t prim =
				memory->read<std::uint64_t>(part + Offsets::BasePart::Primitive);
			if (!valid_ptr(prim))
				return false;

			const math::vector3 p =
				memory->read<math::vector3>(prim + Offsets::Primitive::Position);
			const math::vector3 s =
				memory->read<math::vector3>(prim + Offsets::Primitive::Size);
			if (!math::finite(p) || !math::finite(s))
				return false;

			const math::matrix3 r =
				memory->read<math::matrix3>(prim + Offsets::Primitive::Rotation);

			pos = { p.x, p.y, p.z };
			sz  = { s.x, s.y, s.z };
			rot = Matrix4x4(
				r.m[0], r.m[1], r.m[2], 0.f,
				r.m[3], r.m[4], r.m[5], 0.f,
				r.m[6], r.m[7], r.m[8], 0.f,
				0.f, 0.f, 0.f, 1.f);
			return true;
		}

		std::string read_mesh_id_at(std::uint64_t addr)
		{
			if (!valid_ptr(addr))
				return {};

			std::string s = memory->read_string(addr);
			if (!s.empty() && s != "Unknown")
				return s;

			const std::uint64_t p = memory->read<std::uint64_t>(addr);
			if (!valid_ptr(p))
				return {};

			s = memory->read_string(p);
			if (s == "Unknown")
				return {};
			return s;
		}

		int body_part_index(const std::string& name)
		{
			struct slot_t { int index; const char* names[5]; int count; };
			static const slot_t k_slots[] = {
				{ 1, { "Torso", "UpperTorso", "LowerTorso", "torso", nullptr }, 4 },
				{ 2, { "Left Arm", "LeftUpperArm", "LeftLowerArm", "LeftHand", "arm1" }, 5 },
				{ 3, { "Right Arm", "RightUpperArm", "RightLowerArm", "RightHand", "arm2" }, 5 },
				{ 4, { "Left Leg", "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "leg1" }, 5 },
				{ 5, { "Right Leg", "RightUpperLeg", "RightLowerLeg", "RightFoot", "leg2" }, 5 },
			};

			auto eq_ci = [](const std::string& a, const char* b)
			{
				std::size_t i = 0;
				for (; i < a.size() && b[i]; ++i)
				{
					const unsigned char ca = (unsigned char)a[i];
					const unsigned char cb = (unsigned char)b[i];
					if (std::tolower(ca) != std::tolower(cb)) return false;
				}
				return i == a.size() && b[i] == '\0';
			};

			for (const slot_t& slot : k_slots)
				for (int i = 0; i < slot.count; ++i)
					if (slot.names[i] && eq_ci(name, slot.names[i]))
						return slot.index;

			return -1;
		}

		std::string find_character_mesh_id(std::uint64_t part, int body_part)
		{
			if (body_part < 0 || !valid_ptr(part))
				return {};

			const std::uint64_t character =
				memory->read<std::uint64_t>(part + Offsets::Instance::Parent);
			if (!valid_ptr(character))
				return {};

			for (const auto& c : rbx::instance_t(character).get_children())
			{
				if (!valid_ptr(c.address))
					continue;
				if (c.get_class_name() != "CharacterMesh")
					continue;

				const int bp = memory->read<int>(c.address + Offsets::CharacterMesh::BodyPart);
				if (bp != body_part)
					continue;

				const std::string id =
					read_mesh_id_at(c.address + Offsets::CharacterMesh::MeshId);
				if (!id.empty())
					return id;
			}
			return {};
		}

		struct resolve_result
		{
			std::string mesh_id;
			std::string class_name;
			Vector3     scale{ 1.f, 1.f, 1.f };
			Vector3     offset{ 0.f, 0.f, 0.f };
			bool        fit_to_part{ false };
			bool        is_special{ false };
		};

		resolve_result resolve(const parser::entry_t& e)
		{
			resolve_result r{};
			if (!valid_ptr(e.part))
				return r;

			r.class_name = rbx::part_t(e.part).get_class_name();

			if (r.class_name == "MeshPart")
			{
				r.mesh_id = read_mesh_id_at(e.part + Offsets::MeshPart::MeshId);

				r.fit_to_part = true;
			}

			std::uint64_t special = 0;
			for (const auto& c : rbx::instance_t(e.part).get_children())
			{
				if (!valid_ptr(c.address))
					continue;
				const std::string cc = c.get_class_name();
				if (cc == "SpecialMesh" || cc == "FileMesh" ||
					cc == "CylinderMesh" || cc == "BlockMesh")
				{
					special = c.address;
					break;
				}
			}

			if (special)
			{
				r.is_special = true;
				if (r.mesh_id.empty())
					r.mesh_id = read_mesh_id_at(special + Offsets::SpecialMesh::MeshId);

				r.scale  = memory->read<Vector3>(special + Offsets::SpecialMesh::Scale);
				r.offset = memory->read<Vector3>(special + Offsets::SpecialMesh::Offset);

				auto clamp_sc = [](float& v)
				{
					if (!std::isfinite(v) || v < 1e-4f) v = 1.f;
					if (v > 50.f) v = 50.f;
				};
				clamp_sc(r.scale.x);
				clamp_sc(r.scale.y);
				clamp_sc(r.scale.z);

				r.fit_to_part = false;
			}

			if (r.mesh_id.empty() && e.what == parser::kind::body)
			{
				r.mesh_id = find_character_mesh_id(e.part, body_part_index(e.name));
				if (!r.mesh_id.empty())
					r.fit_to_part = true;
			}

			return r;
		}

		std::shared_ptr<const cached_mesh_t> lookup_mesh(const std::string& mesh_id)
		{
			if (mesh_id.empty())
				return nullptr;
			return cache_t::get().find(mesh_id);
		}

		bool is_worn_kind(parser::kind k)
		{
			return k == parser::kind::accessory || k == parser::kind::hair ||
			       k == parser::kind::face;
		}

		bool mesh_aabb(const cached_mesh_t& m, const Vector3& ms,
			float out_min[3], float out_max[3])
		{
			out_min[0] = out_min[1] = out_min[2] = FLT_MAX;
			out_max[0] = out_max[1] = out_max[2] = -FLT_MAX;

			const int n = (int)m.vertices.size();
			if (n <= 0)
				return false;

			int step = 1;
			if (n > 4000)
				step = (n + 3999) / 4000;

			for (int i = 0; i < n; i += step)
			{
				const float* p = m.vertices[i].pos;
				if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2]))
					continue;

				const float px = p[0] * ms.x, py = p[1] * ms.y, pz = p[2] * ms.z;
				out_min[0] = (std::min)(out_min[0], px); out_max[0] = (std::max)(out_max[0], px);
				out_min[1] = (std::min)(out_min[1], py); out_max[1] = (std::max)(out_max[1], py);
				out_min[2] = (std::min)(out_min[2], pz); out_max[2] = (std::max)(out_max[2], pz);
			}
			return out_min[0] <= out_max[0];
		}

		void fit_scale_to_part(const cached_mesh_t& m, Vector3& ms, const Vector3& sz)
		{
			if (sz.x <= 0.01f || sz.y <= 0.01f || sz.z <= 0.01f)
				return;

			float mn[3], mx[3];
			if (!mesh_aabb(m, ms, mn, mx))
				return;

			const float ax = mx[0] - mn[0], ay = mx[1] - mn[1], az = mx[2] - mn[2];
			if (ax < 1e-5f || ay < 1e-5f || az < 1e-5f)
				return;

			const float rx = sz.x / ax, ry = sz.y / ay, rz = sz.z / az;
			if (rx > 0.88f && rx < 1.12f && ry > 0.88f && ry < 1.12f && rz > 0.88f && rz < 1.12f)
				return;

			auto apply = [](float& cur, float ratio)
			{
				if (ratio < 0.001f) ratio = 0.001f;
				if (ratio > 500.f) ratio = 500.f;
				cur *= ratio;
			};
			apply(ms.x, rx);
			apply(ms.y, ry);
			apply(ms.z, rz);
		}

		void recenter_offset(const cached_mesh_t& m, const Vector3& ms, Vector3& off)
		{
			float mn[3], mx[3];
			if (!mesh_aabb(m, ms, mn, mx))
				return;
			off.x -= (mn[0] + mx[0]) * 0.5f;
			off.y -= (mn[1] + mx[1]) * 0.5f;
			off.z -= (mn[2] + mx[2]) * 0.5f;
		}

		void sanity_fit_if_huge(const cached_mesh_t& m, Vector3& ms, const Vector3& sz, bool is_acc)
		{
			float mn[3], mx[3];
			if (!mesh_aabb(m, ms, mn, mx))
				return;

			const float ext = (std::max)(mx[0] - mn[0], (std::max)(mx[1] - mn[1], mx[2] - mn[2]));
			const float part_ext = (std::max)(sz.x, (std::max)(sz.y, sz.z));
			const bool huge = ext > 40.f || (is_acc && part_ext > 0.05f && ext > part_ext * 5.f);
			if (huge)
				fit_scale_to_part(m, ms, sz);
		}

		bool is_classic_head_mesh(const std::string& id)
		{
			return id.find("heads/head.mesh") != std::string::npos ||
			       clean_asset_id(id) == "head.mesh";
		}

		void fit_classic_r6_head(const cached_mesh_t& m, Vector3& ms, const Vector3& sz,
			const Vector3& sm_scale)
		{
			float mn[3], mx[3];
			const Vector3 one{ 1.f, 1.f, 1.f };
			if (!mesh_aabb(m, one, mn, mx))
				return;

			const float ext = (std::max)(mx[0] - mn[0], (std::max)(mx[1] - mn[1], mx[2] - mn[2]));
			if (ext < 1e-5f)
				return;

			float sy = sm_scale.y;
			if (!std::isfinite(sy) || sy < 0.25f || sy > 3.f)
				sy = 1.25f;

			float target = (sz.y > 0.1f) ? (sz.y * sy) : sy;
			if (target < 0.2f) target = 0.2f;
			if (target > 3.f) target = 3.f;

			const float s = target / ext;
			ms = { s, s, s };
		}

		void apply_visual_fit(const parser::entry_t& e, const resolve_result& rr,
			const cached_mesh_t& m, Vector3& ms, Vector3& off, const Vector3& sz)
		{
			const bool is_acc = is_worn_kind(e.what);

			if (e.name == "Head" && is_classic_head_mesh(rr.mesh_id))
			{
				fit_classic_r6_head(m, ms, sz, rr.scale);
				if (!std::isfinite(off.x) || !std::isfinite(off.y) || !std::isfinite(off.z) ||
					std::fabs(off.x) > 2.f || std::fabs(off.y) > 2.f || std::fabs(off.z) > 2.f)
					off = { 0.f, 0.f, 0.f };
				recenter_offset(m, ms, off);
				return;
			}

			if (rr.is_special)
				return;

			if (rr.fit_to_part)
			{
				fit_scale_to_part(m, ms, sz);
				if (!is_acc)
					recenter_offset(m, ms, off);
				return;
			}

			if (!is_acc)
				sanity_fit_if_huge(m, ms, sz, false);
		}

		Matrix4x4 make_world(const Vector3& pos, const Matrix4x4& rot,
			const Vector3& ms, const Vector3& off)
		{
			Matrix4x4 w{};
			w.m[0][0] = rot.m[0][0] * ms.x;
			w.m[0][1] = rot.m[0][1] * ms.y;
			w.m[0][2] = rot.m[0][2] * ms.z;
			w.m[0][3] = pos.x + rot.m[0][0] * off.x + rot.m[0][1] * off.y + rot.m[0][2] * off.z;
			w.m[1][0] = rot.m[1][0] * ms.x;
			w.m[1][1] = rot.m[1][1] * ms.y;
			w.m[1][2] = rot.m[1][2] * ms.z;
			w.m[1][3] = pos.y + rot.m[1][0] * off.x + rot.m[1][1] * off.y + rot.m[1][2] * off.z;
			w.m[2][0] = rot.m[2][0] * ms.x;
			w.m[2][1] = rot.m[2][1] * ms.y;
			w.m[2][2] = rot.m[2][2] * ms.z;
			w.m[2][3] = pos.z + rot.m[2][0] * off.x + rot.m[2][1] * off.y + rot.m[2][2] * off.z;
			w.m[3][0] = 0.f;
			w.m[3][1] = 0.f;
			w.m[3][2] = 0.f;
			w.m[3][3] = 1.f;
			return w;
		}

		void draw_box_fallback(ImDrawList* dl, const Vector3& pos, const Matrix4x4& rot,
			const Vector3& sz, const math::matrix4& view, const math::vector2& dims, ImU32 fill)
		{
			if (sz.x < 0.01f && sz.y < 0.01f && sz.z < 0.01f)
				return;

			const Vector3 h{ sz.x * 0.5f, sz.y * 0.5f, sz.z * 0.5f };
			const Vector3 lc[8] = {
				{ -h.x, -h.y, -h.z }, { -h.x, -h.y,  h.z },
				{ -h.x,  h.y, -h.z }, { -h.x,  h.y,  h.z },
				{  h.x, -h.y, -h.z }, {  h.x, -h.y,  h.z },
				{  h.x,  h.y, -h.z }, {  h.x,  h.y,  h.z },
			};

			ImVec2 sp[8];
			bool   sv[8]{};
			bool   any = false;
			for (int i = 0; i < 8; ++i)
			{
				const Vector3 wc{
					pos.x + rot.m[0][0] * lc[i].x + rot.m[0][1] * lc[i].y + rot.m[0][2] * lc[i].z,
					pos.y + rot.m[1][0] * lc[i].x + rot.m[1][1] * lc[i].y + rot.m[1][2] * lc[i].z,
					pos.z + rot.m[2][0] * lc[i].x + rot.m[2][1] * lc[i].y + rot.m[2][2] * lc[i].z,
				};
				Vector2 sc;
				sv[i] = w2s(view, dims, wc, sc);
				sp[i] = sv[i] ? ImVec2(sc.x, sc.y) : ImVec2(-9999.f, -9999.f);
				any = any || sv[i];
			}
			if (!any)
				return;

			static const int tris[12][3] = {
				{ 0, 1, 3 }, { 0, 3, 2 }, { 4, 6, 7 }, { 4, 7, 5 },
				{ 0, 4, 5 }, { 0, 5, 1 }, { 2, 3, 7 }, { 2, 7, 6 },
				{ 0, 2, 6 }, { 0, 6, 4 }, { 1, 5, 7 }, { 1, 7, 3 },
			};
			for (const auto& t : tris)
			{
				if (!sv[t[0]] || !sv[t[1]] || !sv[t[2]])
					continue;

				const ImVec2 a = sp[t[0]], b = sp[t[1]], c = sp[t[2]];

				const float cr = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
				if (cr < 0.15f)
					continue;

				dl->AddTriangleFilled(a, b, c, fill);
			}
		}

		struct fit_key_t
		{
			std::uint64_t part{ 0 };
			std::size_t   mesh_h{ 0 };
			int qx{ 0 }, qy{ 0 }, qz{ 0 };
			int flags{ 0 };

			bool operator==(const fit_key_t& o) const
			{
				return part == o.part && mesh_h == o.mesh_h && qx == o.qx &&
					qy == o.qy && qz == o.qz && flags == o.flags;
			}
		};

		struct fit_key_hash_t
		{
			std::size_t operator()(const fit_key_t& k) const
			{
				std::size_t h = (std::size_t)k.part ^ (k.mesh_h * 0x9e3779b97f4a7c15ull);
				h ^= ((std::size_t)k.qx << 1) ^ ((std::size_t)k.qy << 11) ^
				     ((std::size_t)k.qz << 21);
				h ^= (std::size_t)k.flags * 0x85ebca77u;
				return h;
			}
		};

		struct fit_val_t { Vector3 ms; Vector3 off; };

		fit_key_t make_fit_key(std::uint64_t part, const std::string& mesh_id,
			const Vector3& sz, const Vector3& scale, int flags)
		{
			fit_key_t k;
			k.part   = part;
			k.mesh_h = std::hash<std::string>{}(mesh_id);
			k.qx = (int)std::lround(sz.x * 100.f) ^ ((int)std::lround(scale.x * 100.f) << 16);
			k.qy = (int)std::lround(sz.y * 100.f) ^ ((int)std::lround(scale.y * 100.f) << 16);
			k.qz = (int)std::lround(sz.z * 100.f) ^ ((int)std::lround(scale.z * 100.f) << 16);
			k.flags = flags;
			return k;
		}

		constexpr std::uint64_t k_resolve_ttl_ms = 5000ull;
		constexpr std::uint64_t k_fit_ttl_ms     = 15000ull;
		constexpr std::size_t   k_max_resolve    = 2048;

		struct resolve_cache_t
		{
			resolve_result r;
			std::uint64_t  stamp{ 0 };
		};

		std::unordered_map<std::uint64_t, resolve_cache_t> s_resolve;
		std::unordered_map<fit_key_t, fit_val_t, fit_key_hash_t> s_fit;
		std::uint64_t s_prune_at = 0;

		void prune_caches(std::uint64_t now)
		{
			if (now - s_prune_at <= 10000ull)
				return;
			s_prune_at = now;

			for (auto it = s_resolve.begin(); it != s_resolve.end(); )
			{
				if (now - it->second.stamp > k_resolve_ttl_ms)
					it = s_resolve.erase(it);
				else
					++it;
			}
			s_fit.clear();
		}
	}

	void chams_begin_frame(const math::matrix4& view, const math::vector2& dims,
		const math::vector3& camera)
	{
		Mesh::Matrix4x4 v{};
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				v.m[r][c] = view.m[r][c];

		// The pass works in the game's client space; the overlay is the monitor.
		dx::shader::SetOrigin(rbx::visualengine_t::window_offset_x(),
		                      rbx::visualengine_t::window_offset_y());

		if (dims.x > 1.f && dims.y > 1.f)
			dx::shader::Resize((unsigned)dims.x, (unsigned)dims.y);

		dx::shader::BeginFrame(v, { camera.x, camera.y, camera.z },
			(float)(GetTickCount64() / 1000.0));
	}

	void chams_flush(ID3D11RenderTargetView* rtv)
	{
		dx::shader::Flush(rtv);
	}

	void chams_draw(ImDrawList* dl,
		const std::unordered_map<std::string, rbx::part_t>& parts,
		std::uint64_t key,
		const math::matrix4& view, const math::vector2& dims,
		ImU32 fill_col, bool full_detail)
	{
		(void)key;
		if (!dl || parts.empty())
			return;

		const std::uint64_t now = GetTickCount64();
		prune_caches(now);
		if (s_resolve.size() > k_max_resolve)
			s_resolve.clear();

		const std::vector<parser::entry_t> entries = parser::collect(parts);
		if (entries.empty())
			return;

		bool has_named_body = false;
		for (const auto& e : entries)
			if (e.what == parser::kind::body)
			{
				has_named_body = true;
				break;
			}

		const ImDrawListFlags saved_flags = dl->Flags;
		dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;

		bool any_asset_missing = false;

		for (const auto& e : entries)
		{
			if (!valid_ptr(e.part))
				continue;

			if (!full_detail && has_named_body && is_worn_kind(e.what))
				continue;

			const bool use_shader = dx::shader::IsFrameValid();

			Vector3   pos{}, sz{};
			Matrix4x4 rot{};
			if (!part_frame_data(e.part, pos, rot, sz))
				continue;
			if (sz.x < 0.01f && sz.y < 0.01f && sz.z < 0.01f)
				continue;

			{
				const float cw = pos.x * view.m[3][0] + pos.y * view.m[3][1] +
				                 pos.z * view.m[3][2] + view.m[3][3];
				if (cw < 0.1f)
					continue;

				const float cx = pos.x * view.m[0][0] + pos.y * view.m[0][1] +
				                 pos.z * view.m[0][2] + view.m[0][3];
				const float cy = pos.x * view.m[1][0] + pos.y * view.m[1][1] +
				                 pos.z * view.m[1][2] + view.m[1][3];
				const float inv = 1.f / cw;
				const float nx = cx * inv, ny = cy * inv;
				if (nx < -2.f || nx > 2.f || ny < -2.f || ny > 2.f)
					continue;
			}

			const bool is_acc = is_worn_kind(e.what);

			resolve_result rr;
			{
				const auto it = s_resolve.find(e.part);
				if (it != s_resolve.end() && now - it->second.stamp < k_resolve_ttl_ms)
				{
					rr = it->second.r;
				}
				else
				{
					rr = resolve(e);
					if (!rr.mesh_id.empty())
						s_resolve[e.part] = { rr, now };
					else
						s_resolve.erase(e.part);
				}
			}

			std::shared_ptr<const cached_mesh_t> m = lookup_mesh(rr.mesh_id);

			const bool is_head = e.name.size() == 4 &&
				std::tolower((unsigned char)e.name[0]) == 'h' &&
				std::tolower((unsigned char)e.name[1]) == 'e' &&
				std::tolower((unsigned char)e.name[2]) == 'a' &&
				std::tolower((unsigned char)e.name[3]) == 'd';

			if ((!m || m->faces.empty()) && is_head)
			{
				m = lookup_mesh("rbxasset://avatar/heads/head.mesh");
				if (m && !m->faces.empty())
				{
					rr.mesh_id     = "rbxasset://avatar/heads/head.mesh";
					rr.fit_to_part = false;
					rr.scale       = { 1.f, 1.f, 1.f };
					rr.offset      = { 0.f, 0.f, 0.f };
				}
			}

			if (!m || m->faces.empty())
			{

				const bool head_draws_box = is_head && op1::head_mesh_fallback();
				if (!is_acc && (!is_head || head_draws_box))
				{
					if (use_shader)
						dx::shader::QueueBox(make_world(pos, rot, sz, Vector3{ 0.f, 0.f, 0.f }));
					else
						draw_box_fallback(dl, pos, rot, sz, view, dims, fill_col);
				}
				any_asset_missing = true;
				continue;
			}

			Vector3 ms  = rr.scale;
			Vector3 off = rr.offset;

			const int fit_flags = (is_acc ? 1 : 0) | (rr.is_special ? 2 : 0) |
			                      (rr.fit_to_part ? 4 : 0);
			const fit_key_t fk = make_fit_key(e.part, rr.mesh_id, sz, rr.scale, fit_flags);
			const auto fit_it = s_fit.find(fk);
			if (fit_it != s_fit.end())
			{
				ms  = fit_it->second.ms;
				off = fit_it->second.off;
			}
			else
			{
				apply_visual_fit(e, rr, *m, ms, off, sz);
				s_fit[fk] = { ms, off };
			}

			if (use_shader)
			{
				dx::shader::QueueMesh(rr.mesh_id, make_world(pos, rot, ms, off));
				continue;
			}

			const int vtx_count = (int)m->vertices.size();
			if (vtx_count <= 0 || vtx_count > 50000)
				continue;

			const int fac_total = (int)m->faces.size();
			if (fac_total <= 0)
				continue;

			int fac_stride = 1;
			const int fac_budget = 12000;
			if (fac_total > fac_budget)
				fac_stride = (fac_total + fac_budget - 1) / fac_budget;

			static std::vector<char>   need;
			static std::vector<ImVec2> screen;
			static std::vector<char>   ok;

			need.assign((std::size_t)vtx_count, 0);
			for (int i = 0; i < fac_total; i += fac_stride)
			{
				const face_t& f = m->faces[i];
				if (f.indices[0] < (std::uint32_t)vtx_count) need[f.indices[0]] = 1;
				if (f.indices[1] < (std::uint32_t)vtx_count) need[f.indices[1]] = 1;
				if (f.indices[2] < (std::uint32_t)vtx_count) need[f.indices[2]] = 1;
			}

			screen.resize((std::size_t)vtx_count);
			ok.assign((std::size_t)vtx_count, 0);

			const float ox = dims.x;
			const float oy = dims.y;

			for (int i = 0; i < vtx_count; ++i)
			{
				if (!need[i])
					continue;

				const float* p = m->vertices[i].pos;
				const float lx = p[0] * ms.x + off.x;
				const float ly = p[1] * ms.y + off.y;
				const float lz = p[2] * ms.z + off.z;

				const Vector3 wc{
					pos.x + rot.m[0][0] * lx + rot.m[0][1] * ly + rot.m[0][2] * lz,
					pos.y + rot.m[1][0] * lx + rot.m[1][1] * ly + rot.m[1][2] * lz,
					pos.z + rot.m[2][0] * lx + rot.m[2][1] * ly + rot.m[2][2] * lz,
				};

				Vector2 sc;
				if (!w2s(view, dims, wc, sc))
					continue;
				if (sc.x < -ox || sc.x > ox * 2.f || sc.y < -oy || sc.y > oy * 2.f)
					continue;

				screen[i] = ImVec2(sc.x, sc.y);
				ok[i] = 1;
			}

			for (int i = 0; i < fac_total; i += fac_stride)
			{
				const std::uint32_t i0 = m->faces[i].indices[0];
				const std::uint32_t i1 = m->faces[i].indices[1];
				const std::uint32_t i2 = m->faces[i].indices[2];
				if (i0 >= (std::uint32_t)vtx_count || i1 >= (std::uint32_t)vtx_count ||
					i2 >= (std::uint32_t)vtx_count)
					continue;
				if (!ok[i0] || !ok[i1] || !ok[i2])
					continue;

				dl->AddTriangleFilled(screen[i0], screen[i1], screen[i2], fill_col);
			}
		}

		dl->Flags = saved_flags;

		cache_t::get().refresh(any_asset_missing);
	}
}
