
#include "WorldGeo.h"

#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>
#include <settings.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace worldgeo
{
namespace
{
	constexpr float kCell        = 16.f;
	constexpr int   kMaxDepth    = 9;
	constexpr int   kFastDepth   = 4;
	constexpr float kFastMinDim  = 2.f;
	constexpr float kFullMinDim  = 0.02f;
	constexpr int   kMaxCells    = 512;
	constexpr int   kMaxTests    = 2048;
	constexpr int   kRebuildMs   = 10000;

	struct Box
	{
		float px = 0.f, py = 0.f, pz = 0.f;

		float r[9] = { 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f };
		float sx = 0.f, sy = 0.f, sz = 0.f;

		// The primitive the box came from.  It is kept so a ray that reaches
		// this box can ask where the part is *now* instead of trusting a
		// snapshot that can be ten seconds old.
		std::uint64_t prim = 0;
	};

	struct Snapshot
	{
		std::vector<Box>                                  boxes;
		std::unordered_map<std::uint64_t, std::vector<int>> grid;
		std::uint64_t workspace = 0;
		std::chrono::steady_clock::time_point built{};
	};

	std::mutex                        s_mtx;
	std::shared_ptr<const Snapshot>   s_snap;
	std::atomic<bool>                 s_thread_started{ false };
	std::atomic<bool>                 s_force{ false };
	std::atomic<bool>                 s_running{ true };

	std::shared_ptr<const Snapshot> get_snap()
	{
		std::lock_guard<std::mutex> lk(s_mtx);
		return s_snap;
	}

	void set_snap(std::shared_ptr<const Snapshot> s)
	{
		std::lock_guard<std::mutex> lk(s_mtx);
		s_snap = std::move(s);
	}

	inline std::int64_t cell_key(int cx, int cy, int cz)
	{
		return (static_cast<std::int64_t>(cx) * 73856093LL) ^
		       (static_cast<std::int64_t>(cy) * 19349663LL) ^
		       (static_cast<std::int64_t>(cz) * 83492791LL);
	}

	bool is_part_class(const std::string& cls)
	{
		return cls == "Part" || cls == "MeshPart" || cls == "UnionOperation" ||
		       cls == "WedgePart" || cls == "CornerWedgePart" || cls == "TrussPart";
	}

	void walk(std::uint64_t addr, int depth, std::vector<Box>& out,
	          bool ignore_collide, int max_depth, float min_dim)
	{
		if (!addr || depth > max_depth || !s_running.load(std::memory_order_relaxed))
			return;

		rbx::instance_t inst{ addr };
		const std::string cls = inst.get_class_name();
		if (cls.empty() || cls == "unknown")
			return;
		if (cls == "Camera")
			return;

		if (cls == "Model" && inst.find_first_child_by_class("Humanoid").address)
			return;

		if (depth <= 1 && cls == "Folder")
		{
			const std::string nm = inst.get_name();
			if (nm == "Players" || nm == "Ignore")
				return;
		}

		if (is_part_class(cls))
		{
			rbx::part_t part{ addr };
			rbx::primitive_t prim = part.get_primitive();
			if (prim.address)
			{
				bool solid = ignore_collide;
				if (!solid)
					solid = prim.get_can_collide();

				if (solid)
				{
					const math::vector3 pos = prim.get_position();
					const math::matrix3 rot = prim.get_rotation();
					const math::vector3 sz  = prim.get_size();

					const float max_dim = (std::max)(sz.x, (std::max)(sz.y, sz.z));

					if (!(rot.m[6] == 0.f && rot.m[7] == 0.f && rot.m[8] == 0.f) &&
					    max_dim >= min_dim &&
					    sz.x > 0.02f && sz.y > 0.02f && sz.z > 0.02f &&
					    sz.x < 2048.f && sz.y < 2048.f && sz.z < 2048.f)
					{
						Box b{};
						b.px = pos.x; b.py = pos.y; b.pz = pos.z;
						for (int i = 0; i < 9; ++i) b.r[i] = rot.m[i];
						b.sx = sz.x; b.sy = sz.y; b.sz = sz.z;
						b.prim = prim.address;
						out.push_back(b);
					}
				}
			}
			return;
		}

		if (cls == "Workspace" || cls == "Folder" || cls == "Model")
		{
			for (const rbx::instance_t& c : inst.get_children())
				walk(c.address, depth + 1, out, ignore_collide, max_depth, min_dim);
		}
	}

	void publish(std::vector<Box>&& boxes, std::uint64_t workspace)
	{
		if (boxes.empty() || !s_running.load(std::memory_order_relaxed))
			return;

		auto snap = std::make_shared<Snapshot>();
		snap->boxes     = std::move(boxes);
		snap->workspace = workspace;
		snap->built     = std::chrono::steady_clock::now();
		snap->grid.reserve(snap->boxes.size() * 2);

		for (int i = 0; i < (int)snap->boxes.size(); ++i)
		{
			const Box& b = snap->boxes[i];
			const float hx = b.sx * 0.5f, hy = b.sy * 0.5f, hz = b.sz * 0.5f;
			const float ex = std::fabs(b.r[0]) * hx + std::fabs(b.r[1]) * hy + std::fabs(b.r[2]) * hz;
			const float ey = std::fabs(b.r[3]) * hx + std::fabs(b.r[4]) * hy + std::fabs(b.r[5]) * hz;
			const float ez = std::fabs(b.r[6]) * hx + std::fabs(b.r[7]) * hy + std::fabs(b.r[8]) * hz;

			const int x0 = (int)std::floor((b.px - ex) / kCell);
			const int x1 = (int)std::floor((b.px + ex) / kCell);
			const int y0 = (int)std::floor((b.py - ey) / kCell);
			const int y1 = (int)std::floor((b.py + ey) / kCell);
			const int z0 = (int)std::floor((b.pz - ez) / kCell);
			const int z1 = (int)std::floor((b.pz + ez) / kCell);

			for (int cx = x0; cx <= x1; ++cx)
				for (int cy = y0; cy <= y1; ++cy)
					for (int cz = z0; cz <= z1; ++cz)
						snap->grid[cell_key(cx, cy, cz)].push_back(i);
		}

		set_snap(std::move(snap));
	}

	float ray_box_t(const Box& b, float ox, float oy, float oz,
	                float dx, float dy, float dz, float max_t)
	{
		const float px = ox - b.px, py = oy - b.py, pz = oz - b.pz;

		const float lo_x = b.r[0] * px + b.r[3] * py + b.r[6] * pz;
		const float lo_y = b.r[1] * px + b.r[4] * py + b.r[7] * pz;
		const float lo_z = b.r[2] * px + b.r[5] * py + b.r[8] * pz;
		const float ld_x = b.r[0] * dx + b.r[3] * dy + b.r[6] * dz;
		const float ld_y = b.r[1] * dx + b.r[4] * dy + b.r[7] * dz;
		const float ld_z = b.r[2] * dx + b.r[5] * dy + b.r[8] * dz;

		float tmin = 0.f, tmax = max_t;
		const float o[3] = { lo_x, lo_y, lo_z };
		const float d[3] = { ld_x, ld_y, ld_z };
		const float h[3] = { b.sx * 0.5f, b.sy * 0.5f, b.sz * 0.5f };

		for (int i = 0; i < 3; ++i)
		{
			if (std::fabs(d[i]) < 1e-9f)
			{
				if (o[i] < -h[i] || o[i] > h[i])
					return -1.f;
				continue;
			}
			float t1 = (-h[i] - o[i]) / d[i];
			float t2 = ( h[i] - o[i]) / d[i];
			if (t1 > t2) std::swap(t1, t2);
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			if (tmin > tmax)
				return -1.f;
		}
		return tmin;
	}

	bool finite_matrix(const math::matrix3& m)
	{
		for (int i = 0; i < 9; ++i)
			if (!std::isfinite(m.m[i]))
				return false;
		return true;
	}

	// Does this snapshot box stand in the way of the ray, *now*?
	//
	// The snapshot is rebuilt every ten seconds and the map is not frozen in
	// between: doors slide shut, elevators move, crates get carried.  Only the
	// boxes the ray actually reaches are re-read - a handful per ray - and the
	// test is then made against the part's live position and rotation.  That is
	// the difference between a vis check that tracks a moving map and one that
	// reports walls that are no longer there.  A part that stopped colliding
	// stops occluding for the same reason.
	bool live_hit(const Box& b, float ox, float oy, float oz,
	              float dx, float dy, float dz, float min_t, float limit_t, float& out)
	{
		// `limit_t` is the nearest blocker found so far: a box further away than
		// that cannot change the answer, and skipping it here is what keeps the
		// live read to the handful of boxes that matter.
		const float snapshot_hit = ray_box_t(b, ox, oy, oz, dx, dy, dz, limit_t);
		if (snapshot_hit <= min_t)
			return false;

		if (!b.prim)
		{
			out = snapshot_hit;
			return true;
		}

		Box live = b;
		try
		{
			const std::uint8_t flags = memory->read<std::uint8_t>(b.prim + Offsets::Primitive::Flags);
			if (!(flags & Offsets::PrimitiveFlags::CanCollide))
				return false;

			const math::vector3 pos = memory->read<math::vector3>(b.prim + Offsets::Primitive::Position);
			if (!math::finite(pos) || (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f))
			{
				out = snapshot_hit;
				return true;
			}

			live.px = pos.x; live.py = pos.y; live.pz = pos.z;

			// A part that has not moved since the walk has not turned either, so
			// the snapshot's rotation stands and a third read is skipped.  Most
			// of the map is nailed down; only what actually moved pays for it.
			const float moved = std::fabs(pos.x - b.px) + std::fabs(pos.y - b.py) +
			                    std::fabs(pos.z - b.pz);
			if (moved < 0.05f)
			{
				out = snapshot_hit;
				return true;
			}

			const math::matrix3 rot = memory->read<math::matrix3>(b.prim + Offsets::Primitive::Rotation);
			if (finite_matrix(rot))
				for (int i = 0; i < 9; ++i) live.r[i] = rot.m[i];
		}
		catch (...)
		{
			out = snapshot_hit;
			return true;
		}

		const float now_hit = ray_box_t(live, ox, oy, oz, dx, dy, dz, limit_t);
		if (now_hit <= min_t)
			return false;

		out = now_hit;
		return true;
	}

	// Distance to the first surface at or past min_t along a real direction, or
	// max_t when the ray stays clear. Hits closer than min_t are ignored: those are
	// the wall the camera is pressed against, which never counts as a blocker.
	float march(const Snapshot& snap, float ox, float oy, float oz,
	            float dx, float dy, float dz, float min_t, float max_t)
	{
		if (max_t <= min_t)
			return max_t;

		int cx = (int)std::floor(ox / kCell);
		int cy = (int)std::floor(oy / kCell);
		int cz = (int)std::floor(oz / kCell);

		const int step_x = (dx > 0.f) ? 1 : -1;
		const int step_y = (dy > 0.f) ? 1 : -1;
		const int step_z = (dz > 0.f) ? 1 : -1;

		const float t_delta_x = (dx != 0.f) ? std::fabs(kCell / dx) : 1e9f;
		const float t_delta_y = (dy != 0.f) ? std::fabs(kCell / dy) : 1e9f;
		const float t_delta_z = (dz != 0.f) ? std::fabs(kCell / dz) : 1e9f;

		auto axis_t = [](float origin, float d, int c) -> float {
			if (d == 0.f)
				return 1e9f;
			const float edge = (d > 0.f) ? ((c + 1) * kCell) : (c * kCell);
			return (edge - origin) / d;
		};
		float t_max_x = axis_t(ox, dx, cx);
		float t_max_y = axis_t(oy, dy, cy);
		float t_max_z = axis_t(oz, dz, cz);

		float best = max_t;
		float t = 0.f;
		int cells = 0;
		int tests = 0;

		while (best > min_t && t <= max_t && cells < kMaxCells && tests < kMaxTests)
		{
			auto it = snap.grid.find(cell_key(cx, cy, cz));
			if (it != snap.grid.end())
			{
				for (int idx : it->second)
				{
					if (++tests > kMaxTests)
						break;
					float hit = 0.f;
					if (live_hit(snap.boxes[(std::size_t)idx], ox, oy, oz, dx, dy, dz,
					             min_t, best, hit) && hit < best)
						best = hit;
				}
			}

			if (t_max_x < t_max_y && t_max_x < t_max_z)
			{
				t = t_max_x; t_max_x += t_delta_x; cx += step_x;
			}
			else if (t_max_y < t_max_z)
			{
				t = t_max_y; t_max_y += t_delta_y; cy += step_y;
			}
			else
			{
				t = t_max_z; t_max_z += t_delta_z; cz += step_z;
			}
			++cells;
		}

		return best;
	}

	// One line, one snapshot, answered without taking the mutex again.
	bool visible_in(const Snapshot& snap, const math::vector3& cam, const math::vector3& target)
	{
		if (!math::finite(cam) || !math::finite(target))
			return true;

		float dx = target.x - cam.x;
		float dy = target.y - cam.y;
		float dz = target.z - cam.z;

		const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

		if (dist < 1.f)
			return true;

		dx /= dist; dy /= dist; dz /= dist;

		// Anything hit between the camera and the target blocks the view; the
		// march stops a stud short of both ends, so a wall you are pressed
		// against and the target's own surface do not count.
		return march(snap, cam.x, cam.y, cam.z, dx, dy, dz, 1.f, dist - 1.f) >= dist - 1.f;
	}

	void run()
	{
		while (s_running.load(std::memory_order_relaxed))
		{
			const std::uint64_t ws = game::workspace.address;

			if (ws && wants_data())
			{
				auto cur = get_snap();
				const bool forced  = s_force.exchange(false);
				const bool stale   = !cur ||
				                     cur->workspace != ws ||
				                     cur->boxes.empty() ||
				                     (std::chrono::steady_clock::now() - cur->built) >
				                         std::chrono::milliseconds(kRebuildMs);

				if (forced || stale)
				{

					if (!cur || cur->boxes.empty() || cur->workspace != ws)
					{
						std::vector<Box> fast;
						fast.reserve(1024);
						try { walk(ws, 0, fast, false, kFastDepth, kFastMinDim); }
						catch (...) {  }
						if (!fast.empty())
							publish(std::move(fast), ws);
					}

					if (!s_running.load(std::memory_order_relaxed))
						break;

					std::vector<Box> boxes;
					boxes.reserve(4096);
					try { walk(ws, 0, boxes, false, kMaxDepth, kFullMinDim); }
					catch (...) {  }

					if (boxes.size() < 20 && s_running.load(std::memory_order_relaxed))
					{
						boxes.clear();
						try { walk(ws, 0, boxes, true, kMaxDepth, kFullMinDim); }
						catch (...) {  }
					}
					publish(std::move(boxes), ws);
				}
			}

			for (int i = 0; i < 50 && s_running.load(std::memory_order_relaxed); ++i)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if (s_force.load(std::memory_order_relaxed))
					break;
				if (!wants_data())
					continue;

				auto cur = get_snap();
				if (!cur || cur->workspace != game::workspace.address ||
				    cur->boxes.empty() ||
				    (std::chrono::steady_clock::now() - cur->built) >
				        std::chrono::milliseconds(kRebuildMs))
					break;
			}
		}
	}
}

bool wants_data()
{

	const int snow = settings::world::effects_3d::snow_mode;
	const int rain = settings::world::effects_3d::rain_mode;

	return settings::visuals::visible_check
	    || settings::aimbot::visible_check
	    || settings::silentaim::visible_check
	    || settings::triggerbot::visibility_check
	    || snow >= 3 || rain >= 3;
}

void start()
{
	bool expected = false;
	if (!s_thread_started.compare_exchange_strong(expected, true))
		return;
	s_running.store(true, std::memory_order_relaxed);
	std::thread(run).detach();
}

void stop()
{
	s_running.store(false, std::memory_order_relaxed);
}

void invalidate()
{
	set_snap(nullptr);
}

int box_count()
{
	auto s = get_snap();
	return s ? (int)s->boxes.size() : 0;
}

void force_reparse()
{
	s_force.store(true, std::memory_order_relaxed);
}

math::vector3 eye_position(const math::vector3& fallback)
{
	try
	{
		const std::uint64_t cam = rbx::current_camera_address();
		if (!cam)
			return fallback;

		const math::vector3 pos = memory->read<math::vector3>(cam + Offsets::Camera::Position);

		if (math::finite(pos) && (pos.x != 0.f || pos.y != 0.f || pos.z != 0.f))
			return pos;
	}
	catch (...) {}
	return fallback;
}

bool ground_y_at(float x, float z, float from_y, float& out_y)
{
	auto snap = get_snap();
	if (!snap || snap->boxes.empty() || snap->grid.empty())
		return false;

	const int cx = (int)std::floor(x / kCell);
	const int cz = (int)std::floor(z / kCell);
	int cy = (int)std::floor(from_y / kCell);

	float best  = -1e30f;
	bool  found = false;

	constexpr int kMaxColumnCells = 24;
	for (int i = 0; i < kMaxColumnCells; ++i, --cy)
	{
		auto it = snap->grid.find(cell_key(cx, cy, cz));
		if (it == snap->grid.end())
			continue;

		for (int idx : it->second)
		{
			const Box& b = snap->boxes[(std::size_t)idx];

			const float top = b.py + std::fabs(b.r[4] * b.sy * 0.5f) +
				std::fabs(b.r[1] * b.sx * 0.5f) + std::fabs(b.r[7] * b.sz * 0.5f);
			if (top > from_y + 1.f || top <= best)
				continue;

			const float hx = std::fabs(b.r[0] * b.sx * 0.5f) + std::fabs(b.r[3] * b.sy * 0.5f) + std::fabs(b.r[6] * b.sz * 0.5f) + 0.5f;
			const float hz = std::fabs(b.r[2] * b.sx * 0.5f) + std::fabs(b.r[5] * b.sy * 0.5f) + std::fabs(b.r[8] * b.sz * 0.5f) + 0.5f;
			if (x < b.px - hx || x > b.px + hx || z < b.pz - hz || z > b.pz + hz)
				continue;

			best  = top;
			found = true;
		}
	}

	if (found)
		out_y = best;
	return found;
}

bool is_visible(const math::vector3& cam, const math::vector3& target)
{
	auto snap = get_snap();

	if (!snap || snap->boxes.empty() || snap->grid.empty())
		return true;

	return visible_in(*snap, cam, target);
}

bool is_visible_any(const math::vector3& cam, const math::vector3* points, int count)
{
	if (!points || count <= 0)
		return true;

	auto snap = get_snap();

	if (!snap || snap->boxes.empty() || snap->grid.empty())
		return true;

	for (int i = 0; i < count; ++i)
	{
		if (!math::finite(points[i]))
			continue;
		if (visible_in(*snap, cam, points[i]))
			return true;
	}

	return false;
}

bool is_body_visible(const math::vector3& cam, const math::vector3& head, const math::vector3& root)
{
	if (!math::finite(cam) || !math::finite(head) || !math::finite(root))
		return true;

	auto snap = get_snap();

	if (!snap || snap->boxes.empty() || snap->grid.empty())
		return true;

	const math::vector3 column = head - root;
	const float         height = column.length();

	// A crouched, flattened or broken body: one point is all there is to test.
	if (height < 0.5f)
		return visible_in(*snap, cam, head);

	// The order a peek uncovers in, head first.  A body out in the open
	// answers on the first ray, which is the common case and keeps this cheap.
	if (visible_in(*snap, cam, head - column * 0.06f)) return true;  // head
	if (visible_in(*snap, cam, head - column * 0.34f)) return true;  // chest
	if (visible_in(*snap, cam, head - column * 0.70f)) return true;  // hips

	// Either shoulder at chest height: a body leaning out from cover is an arm
	// and nothing else, and the centre of it is still behind the wall.  The
	// side has to be horizontal and across the view: the body column is
	// vertical, so it has no side of its own to take.
	const math::vector3 to_body{ head.x - cam.x, 0.f, head.z - cam.z };
	const float         to_body_len = to_body.length();

	math::vector3 side{ 1.f, 0.f, 0.f };
	if (to_body_len > 1e-3f)
		side = { -to_body.z / to_body_len, 0.f, to_body.x / to_body_len };

	const math::vector3 shoulder = head - column * 0.30f;
	const math::vector3 offset   = side * (height * 0.18f);

	if (visible_in(*snap, cam, shoulder + offset)) return true;
	if (visible_in(*snap, cam, shoulder - offset)) return true;

	return false;
}

float open_radius(const math::vector3& fallback_eye, float max_radius)
{
	if (max_radius <= 1.f)
		return 0.f;

	auto snap = get_snap();

	// No geometry to measure against: hand back the full size rather than a tiny
	// volume, so weather looks the same as it did before the snapshot exists.
	if (!snap || snap->boxes.empty() || snap->grid.empty())
		return max_radius;

	math::vector3 eye   = fallback_eye;
	math::vector3 fwd{};
	math::vector3 right{};
	math::vector3 up{};
	bool have_frame = false;

	try
	{
		const std::uint64_t cam = rbx::current_camera_address();
		if (cam)
		{
			const math::vector3 pos = memory->read<math::vector3>(cam + Offsets::Camera::Position);
			if (math::finite(pos) && (pos.x != 0.f || pos.y != 0.f || pos.z != 0.f))
				eye = pos;

			const math::matrix3 rot = rbx::camera_t(cam).get_rotation();
			fwd   = rot.forward();
			right = rot.right();
			up    = rot.up();

			have_frame = math::finite(fwd) && fwd.length() > 0.001f &&
			             math::finite(right) && math::finite(up);
		}
	}
	catch (...)
	{
		have_frame = false;
	}

	if (!have_frame)
		return max_radius;

	// A 3x3 grid of rays across the view cone, then the low third of what they
	// hit: one open direction - a doorway, the far end of a corridor - cannot hold
	// the volume at full size while the rest of the view is a wall.
	constexpr int   kProbes = 9;
	constexpr float kSpread = 0.45f;

	float hits[kProbes]{};
	int   count = 0;

	for (int iy = -1; iy <= 1; ++iy)
	{
		for (int ix = -1; ix <= 1; ++ix)
		{
			math::vector3 dir = fwd + right * ((float)ix * kSpread) +
			                          up    * ((float)iy * kSpread * 0.6f);

			const float len = dir.length();
			if (len < 0.0001f)
				continue;
			dir = dir * (1.f / len);

			// A probe shorter than a stud means the camera is inside geometry, which
			// is not something to size the volume from - treat it as open.
			const float hit = march(*snap, eye.x, eye.y, eye.z, dir.x, dir.y, dir.z, 1.f, max_radius);
			hits[count++] = (hit > 1.f) ? hit : max_radius;
		}
	}

	if (count == 0)
		return max_radius;

	std::sort(hits, hits + count);
	return hits[count / 3];
}

}
