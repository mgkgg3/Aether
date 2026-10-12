#define NOMINMAX
#include <Windows.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>
#include <features/opone/OperationOne.h>

std::string rbx::nameable_t::get_name() const
{



	uintptr_t name_container = memory->read<uintptr_t>(this->address + Offsets::Instance::NameContainer);

	if (!name_container || name_container == ~0ull)
	{
		return "invalid_str";
	}

	return memory->read_string(name_container + Offsets::Instance::Name);
}

std::string rbx::nameable_t::get_class_name() const
{
	std::uint64_t class_descriptor = memory->read<std::uint64_t>(this->address + Offsets::Instance::ClassDescriptor);
	std::uint64_t class_name = memory->read<std::uint64_t>(class_descriptor + Offsets::Instance::ClassName);

	if (class_name)
	{
		return memory->read_string(class_name);
	}

	return "unknown";
}

std::vector<rbx::instance_t> rbx::interface_t::get_children() const
{
	const rbx::instance_t* base = static_cast<const rbx::instance_t*>(this);
	std::vector<rbx::instance_t> children;
	if (!base->address) return children;








	std::uint64_t start{ memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart) };
	std::uint64_t end{ memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd) };

	children.reserve(32);

	for (std::uint64_t instance = memory->read<std::uint64_t>(start);
	     instance != end && children.size() < 4096;
	     instance += sizeof(std::shared_ptr<void*>))
	{
		children.emplace_back(memory->read<std::uint64_t>(instance));
	}

	return children;
}

rbx::instance_t rbx::interface_t::find_first_child(std::string_view str) const
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_name() == str)
		{
			return child;
		}
	}

	return {};
}

rbx::instance_t rbx::interface_t::find_first_child_by_class(std::string_view str) const
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_class_name() == str)
		{
			return child;
		}
	}

	return {};
}

void rbx::instance_t::set_instance_double(double v)
{
	memory->write<double>(this->address + Offsets::Misc::Value, v);
}

rbx::model_instance_t rbx::player_t::get_model_instance() const
{
	return { memory->read<std::uint64_t>(this->address + Offsets::Player::ModelInstance) };
}

std::uint64_t rbx::player_t::get_team() const
{
	if (!this->address) return 0;

	std::uint64_t team = 0;
	try { team = memory->read<std::uint64_t>(this->address + Offsets::Player::Team); }
	catch (...) { return 0; }
	if (!team) return 0;





	try
	{
		if (rbx::instance_t(team).get_class_name() == "Team")
			return team;
	}
	catch (...) {}
	return 0;
}

std::uint32_t rbx::player_t::get_team_color() const
{
	if (!this->address) return 0;
	try { return memory->read<std::uint32_t>(this->address + Offsets::Player::TeamColor); }
	catch (...) { return 0; }
}

std::uint8_t rbx::humanoid_t::get_rig_type() const
{
	return { memory->read<std::uint8_t>(this->address + Offsets::Humanoid::RigType) };
}

float rbx::humanoid_t::get_health() const
{
	return memory->read<float>(this->address + Offsets::Humanoid::Health);
}

float rbx::humanoid_t::get_max_health() const
{
	return memory->read<float>(this->address + Offsets::Humanoid::MaxHealth);
}

int rbx::humanoid_t::get_state() const
{












	const std::uint64_t state_machine =
		memory->read<std::uint64_t>(this->address + Offsets::Humanoid::HumanoidState);

	if (state_machine > 0x10000ull && state_machine != ~0ull)
	{
		const int state = (int)memory->read<std::uint8_t>(
			state_machine + Offsets::Humanoid::HumanoidStateID);
		if (state >= 0 && state <= 18)
			return state;
	}

	return -1;
}

math::matrix3 rbx::humanoid_t::get_cframe_rotation() const
{
	std::uint64_t hrp = memory->read<std::uint64_t>(this->address + Offsets::Humanoid::HumanoidRootPart);
	if (!hrp) return {};
	std::uint64_t prim = memory->read<std::uint64_t>(hrp + Offsets::BasePart::Primitive);
	if (!prim) return {};
	return memory->read<math::matrix3>(prim + Offsets::Primitive::Rotation);
}

void rbx::humanoid_t::set_jump_power(float power)
{
	memory->write<float>(this->address + Offsets::Humanoid::JumpPower, power);
}

void rbx::humanoid_t::write_gravity(float gravity)
{
	if (!game::datamodel.address) return;
	std::uint64_t workspace = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
	if (!workspace) return;
	std::uint64_t world = memory->read<std::uint64_t>(workspace + Offsets::Workspace::World);
	if (!world) return;
	memory->write<float>(world + Offsets::World::Gravity, gravity);
}

void rbx::humanoid_t::write_tickrate(float tickrate)
{
	if (!game::workspace.address) return;
	std::uint64_t world = memory->read<std::uint64_t>(game::workspace.address + Offsets::Workspace::World);
	if (!world) return;
	memory->write<float>(world + Offsets::World::worldStepsPerSec, tickrate);
}

rbx::primitive_t rbx::part_t::get_primitive() const
{
	return { memory->read<std::uint64_t>(this->address + Offsets::BasePart::Primitive) };
}

math::vector3 rbx::primitive_t::get_size() const
{
	return memory->read<math::vector3>(this->address + Offsets::Primitive::Size);
}

void rbx::primitive_t::set_size(const math::vector3& size)
{
	memory->write<math::vector3>(this->address + Offsets::Primitive::Size, size);
}

math::vector3 rbx::primitive_t::get_position() const
{
	return memory->read<math::vector3>(this->address + Offsets::Primitive::Position);
}

math::matrix3 rbx::primitive_t::get_rotation() const
{
	return memory->read<math::matrix3>(this->address + Offsets::Primitive::Rotation);
}

bool rbx::primitive_t::get_can_collide() const
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;

	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::Primitive::Flags);
	return (flags & Offsets::PrimitiveFlags::CanCollide) != 0;
}

bool rbx::primitive_t::set_can_collide(bool enable)
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;

	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::Primitive::Flags);

	if (enable)
		flags |= Offsets::PrimitiveFlags::CanCollide;
	else
		flags &= ~Offsets::PrimitiveFlags::CanCollide;

	memory->write<std::uint8_t>(primitive + Offsets::Primitive::Flags, flags);
	return enable;
}

math::vector2 rbx::visualengine_t::get_dimensions() const
{
	return memory->read<math::vector2>(this->address + Offsets::VisualEngine::Dimensions);
}






























namespace
{
	struct cam_snapshot_t
	{
		bool          valid{ false };
		math::vector3 pos{};
		math::vector3 right{};
		math::vector3 up{};
		math::vector3 fwd{};
		float         tan_x{ 1.f };
		float         tan_y{ 1.f };
	};

	constexpr float k_default_fov = 1.22173f;
	bool build_cam_snapshot(cam_snapshot_t& out)
	{
		out = {};
		const std::uint64_t cam = rbx::current_camera_address();
		if (!cam) return false;

		const math::vector3 pos = memory->read<math::vector3>(cam + Offsets::Camera::Position);
		if (!math::finite(pos)) return false;
		if (std::fabs(pos.x) > 1e7f || std::fabs(pos.y) > 1e7f || std::fabs(pos.z) > 1e7f) return false;

		const math::matrix3 rot = memory->read<math::matrix3>(cam + Offsets::Camera::Rotation);
		math::vector3 f = rot.forward(), u = rot.up(), r = rot.right();
		if (!math::finite(f) || !math::finite(u) || !math::finite(r)) return false;
		if (f.length() < 0.5f || u.length() < 0.5f || r.length() < 0.5f) return false;











		const math::vector3 fr = f.normalized(), ur = u.normalized(), rr = r.normalized();
		if (std::fabs(fr.dot(ur)) > 0.05f || std::fabs(fr.dot(rr)) > 0.05f ||
			std::fabs(ur.dot(rr)) > 0.05f)
			return false;

		float fov = memory->read<float>(cam + Offsets::Camera::FieldOfView);
		if (!(fov > 0.05f && fov < 3.10f)) fov = k_default_fov;



		math::vector2 dims = memory->read<math::vector2>(
			game::visengine.address + Offsets::VisualEngine::Dimensions);
		float aspect = 1.7778f;
		if (dims.x > 16.f && dims.y > 16.f && std::isfinite(dims.x) && std::isfinite(dims.y))
			aspect = dims.x / dims.y;

		out.pos   = pos;
		out.fwd   = f.normalized();
		out.up    = u.normalized();
		out.right = r.normalized();
		out.tan_y = std::tan(fov * 0.5f);
		out.tan_x = out.tan_y * aspect;
		out.valid = true;
		return true;
	}



	math::matrix4 make_view_projection(const cam_snapshot_t& cs)
	{
		math::matrix4 m{};
		const float kx = 1.f / cs.tan_x;
		const float ky = 1.f / cs.tan_y;

		m.m[0][0] = cs.right.x * kx; m.m[0][1] = cs.right.y * kx; m.m[0][2] = cs.right.z * kx;
		m.m[0][3] = -cs.right.dot(cs.pos) * kx;
		m.m[1][0] = cs.up.x * ky;    m.m[1][1] = cs.up.y * ky;    m.m[1][2] = cs.up.z * ky;
		m.m[1][3] = -cs.up.dot(cs.pos) * ky;



		m.m[2][0] = -cs.fwd.x;       m.m[2][1] = -cs.fwd.y;       m.m[2][2] = -cs.fwd.z;
		m.m[2][3] =  cs.fwd.dot(cs.pos);
		m.m[3][0] =  cs.fwd.x;       m.m[3][1] =  cs.fwd.y;       m.m[3][2] =  cs.fwd.z;
		m.m[3][3] = -cs.fwd.dot(cs.pos);
		return m;
	}

	bool project_ndc(const math::matrix4& m, const math::vector3& p, math::vector2& ndc)
	{
		math::vector4 c = m.multiply({ p.x, p.y, p.z, 1.f });
		if (!math::finite(c)) return false;
		if (!(c.w > 1e-3f)) return false;
		ndc = { c.x / c.w, c.y / c.w };
		return std::isfinite(ndc.x) && std::isfinite(ndc.y);
	}












	bool describe_view_projection(const math::matrix4& m, const cam_snapshot_t& cs,
		float aspect, float& out_tan_x, float& out_tan_y)
	{
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				if (!std::isfinite(m.m[r][c]) || std::fabs(m.m[r][c]) > 1e7f)
					return false;

		math::vector2 ndc;

		if (!project_ndc(m, cs.pos + cs.fwd * 12.f, ndc)) return false;
		if (std::fabs(ndc.x) > 0.02f || std::fabs(ndc.y) > 0.02f) return false;
		if (!project_ndc(m, cs.pos + cs.fwd * 90.f, ndc)) return false;
		if (std::fabs(ndc.x) > 0.02f || std::fabs(ndc.y) > 0.02f) return false;

		if (!project_ndc(m, cs.pos + cs.fwd * 20.f + cs.right * 10.f, ndc)) return false;
		if (!(ndc.x > 0.02f)) return false;
		out_tan_x = 10.f / (20.f * ndc.x);

		if (!project_ndc(m, cs.pos + cs.fwd * 20.f + cs.up * 10.f, ndc)) return false;
		if (!(ndc.y > 0.02f)) return false;
		out_tan_y = 10.f / (20.f * ndc.y);

		if (!(out_tan_x > 0.02f && out_tan_x < 6.f)) return false;
		if (!(out_tan_y > 0.02f && out_tan_y < 6.f)) return false;

		const float ratio = out_tan_x / out_tan_y;
		if (std::fabs(ratio - aspect) > aspect * 0.15f) return false;

		return true;
	}

	std::atomic<std::uintptr_t> s_proj_object{ 0 };
	std::atomic<std::uintptr_t> s_proj_offset{ 0 };
	std::atomic<float>          s_cal_tan_x{ 0.f };
	std::atomic<float>          s_cal_tan_y{ 0.f };
	std::atomic<bool>           s_native_active{ false };
	std::atomic<bool>           s_scan_busy{ false };
	std::atomic<std::uint64_t>  s_scan_ms{ 0 };
	std::atomic<int>            s_scan_attempts{ 0 };

	auto tick_ms = [] { return (std::uint64_t)GetTickCount64(); };
















	constexpr std::uint64_t k_last_basis_ms = 400;

	std::mutex          s_last_cam_mtx;
	cam_snapshot_t      s_last_cam{};
	std::atomic<std::uint64_t> s_last_cam_ms{ 0 };

	void remember_camera_basis(const cam_snapshot_t& cs)
	{
		std::lock_guard<std::mutex> lk(s_last_cam_mtx);
		s_last_cam = cs;
		s_last_cam_ms.store(tick_ms(), std::memory_order_relaxed);
	}

	bool recall_camera_basis(cam_snapshot_t& out)
	{
		std::lock_guard<std::mutex> lk(s_last_cam_mtx);
		if (!s_last_cam.valid)
			return false;

		const std::uint64_t at = s_last_cam_ms.load(std::memory_order_relaxed);
		if (at == 0 || tick_ms() - at > k_last_basis_ms)
			return false;

		out = s_last_cam;
		return true;
	}







	bool looks_like_projection(const math::matrix4& m)
	{
		bool any_non_zero = false;
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
			{
				const float v = m.m[r][c];
				if (!std::isfinite(v) || std::fabs(v) > 1e9f)
					return false;
				if (v != 0.f)
					any_non_zero = true;
			}
		if (!any_non_zero)
			return false;

		const float rx = m.m[0][0], ry = m.m[0][1], rz = m.m[0][2];
		const float ux = m.m[1][0], uy = m.m[1][1], uz = m.m[1][2];
		const float wx = m.m[3][0], wy = m.m[3][1], wz = m.m[3][2];

		const float r_len = std::sqrt(rx * rx + ry * ry + rz * rz);
		const float u_len = std::sqrt(ux * ux + uy * uy + uz * uz);
		const float w_len = std::sqrt(wx * wx + wy * wy + wz * wz);

		if (!(w_len > 0.5f && w_len < 2.f))
			return false;
		if (!(r_len > 0.5f && r_len < 40.f) || !(u_len > 0.5f && u_len < 40.f))
			return false;


		const float dot = rx * ux + ry * uy + rz * uz;
		return std::fabs(dot) < r_len * u_len * 0.05f;
	}

	bool scan_block_at(std::uintptr_t base, const cam_snapshot_t& cs, float aspect,
		std::uintptr_t& out_off, math::matrix4& out_m)
	{
		if (!base) return false;



		constexpr std::size_t k_span = 0x2000;
		alignas(16) float block[16];

		for (std::size_t off = 0; off + sizeof(block) <= k_span; off += 4)
		{
			if (memory->read_raw(base + off, block, sizeof(block)) != sizeof(block))
				continue;

			math::matrix4 m{};
			std::memcpy(&m.m[0][0], block, sizeof(block));

			float tx = 0.f, ty = 0.f;
			if (!describe_view_projection(m, cs, aspect, tx, ty))
				continue;

			out_off = off;
			out_m   = m;
			s_cal_tan_x.store(tx, std::memory_order_relaxed);
			s_cal_tan_y.store(ty, std::memory_order_relaxed);
			return true;
		}
		return false;
	}




	bool scan_for_view_projection(std::uintptr_t ve)
	{
		cam_snapshot_t cs{};
		if (!build_cam_snapshot(cs))
			return false;

		const float aspect = cs.tan_x / cs.tan_y;




		std::uintptr_t windows[4]{ ve, 0, 0, 0 };
		std::uint32_t  count = 1;




		auto usable = [](std::uintptr_t p) { return p > 0x10000ull && p < 0x7fffffff0000ull; };

		if (usable(ve))
		{
			std::uintptr_t rv = memory->read<std::uintptr_t>(ve + Offsets::VisualEngine::RenderView);
			if (usable(rv)) windows[count++] = rv;
		}
		if (game::datamodel.address)
		{
			std::uintptr_t rv = memory->read<std::uintptr_t>(
				game::datamodel.address + Offsets::DataModel::ToRenderView1);
			rv = usable(rv) ? memory->read<std::uintptr_t>(rv + Offsets::DataModel::ToRenderView2) : 0;
			rv = usable(rv) ? memory->read<std::uintptr_t>(rv + Offsets::DataModel::ToRenderView3) : 0;
			if (usable(rv)) windows[count++] = rv;
		}

		for (std::uint32_t w = 0; w < count; ++w)
		{
			std::uintptr_t off = 0;
			math::matrix4 m{};
			if (scan_block_at(windows[w], cs, aspect, off, m))
			{
				s_proj_object.store(windows[w]);
				s_proj_offset.store(off);
				return true;
			}
		}
		return true;
	}





	bool resolve_native_view_projection(math::matrix4& out)
	{
		const std::uintptr_t obj = s_proj_object.load();
		const std::uintptr_t off = s_proj_offset.load();
		if (!obj || !off)
			return false;

		math::matrix4 m = memory->read<math::matrix4>(obj + off);

		cam_snapshot_t cs{};
		if (!build_cam_snapshot(cs))
		{



			if (!looks_like_projection(m))
				return false;
			out = m;
			return true;
		}

		float tx = 0.f, ty = 0.f;
		if (!describe_view_projection(m, cs, cs.tan_x / cs.tan_y, tx, ty))
		{


			s_proj_offset.store(0);
			s_proj_object.store(0);
			return false;
		}

		out = m;
		return true;
	}







	bool read_documented_view_projection(math::matrix4& out)
	{
		const std::uintptr_t ve = game::visengine.address;
		if (!ve)
			return false;

		const math::matrix4 m = memory->read<math::matrix4>(ve + Offsets::VisualEngine::ViewMatrix);
		if (!looks_like_projection(m))
			return false;

		out = m;
		return true;
	}
}

bool rbx::visualengine_t::using_native_projection()
{
	return s_native_active.load(std::memory_order_relaxed);
}

std::uintptr_t rbx::visualengine_t::native_projection_offset()
{
	return s_proj_offset.load(std::memory_order_relaxed);
}

math::matrix4 rbx::visualengine_t::get_viewmatrix() const
{











	if (op1::engine_matrix_enabled())
	{
		auto note = [](const char* source) { op1::publish_matrix_source(source); };

		math::matrix4 native{};
		if (resolve_native_view_projection(native))
		{
			s_native_active.store(true, std::memory_order_relaxed);
			note("engine (discovered)");
			return native;
		}
		if (read_documented_view_projection(native))
		{
			s_native_active.store(true, std::memory_order_relaxed);
			note("engine (documented)");
			return native;
		}





		const std::uintptr_t ve = this->address ? this->address : game::visengine.address;

		static std::uintptr_t s_last_ve_op1 = 0;
		if (ve != s_last_ve_op1)
		{
			s_last_ve_op1 = ve;
			s_scan_attempts.store(0);
		}

		if (ve && s_scan_attempts.load() < 4)
		{
			const std::uint64_t now = tick_ms();
			if (now - s_scan_ms.load() >= 1500 && !s_scan_busy.exchange(true))
			{
				s_scan_ms.store(now);
				if (scan_for_view_projection(ve))
					s_scan_attempts.fetch_add(1);
				s_scan_busy.store(false);

				if (resolve_native_view_projection(native))
				{
					s_native_active.store(true, std::memory_order_relaxed);
					note("engine (probed)");
					return native;
				}
			}
		}
	}











	cam_snapshot_t cs{};
	if (build_cam_snapshot(cs))
	{
		remember_camera_basis(cs);
		s_native_active.store(false, std::memory_order_relaxed);
		if (op1::engine_matrix_enabled())
			op1::publish_matrix_source("camera (engine matrix unavailable)");
		return make_view_projection(cs);
	}




	if (recall_camera_basis(cs))
	{
		s_native_active.store(false, std::memory_order_relaxed);
		return make_view_projection(cs);
	}



	math::matrix4 native{};
	if (resolve_native_view_projection(native))
	{
		s_native_active.store(true, std::memory_order_relaxed);
		return native;
	}

	if (read_documented_view_projection(native))
	{
		s_native_active.store(true, std::memory_order_relaxed);
		return native;
	}






	const std::uintptr_t ve = this->address ? this->address : game::visengine.address;


	static std::uintptr_t s_last_ve = 0;
	if (ve != s_last_ve)
	{
		s_last_ve = ve;
		s_scan_attempts.store(0);
	}

	if (ve && s_scan_attempts.load() < 4)
	{
		const std::uint64_t now = tick_ms();
		if (now - s_scan_ms.load() >= 1500 && !s_scan_busy.exchange(true))
		{
			s_scan_ms.store(now);
			if (scan_for_view_projection(ve))
				s_scan_attempts.fetch_add(1);
			s_scan_busy.store(false);

			if (resolve_native_view_projection(native))
			{
				s_native_active.store(true, std::memory_order_relaxed);
				return native;
			}
		}
	}




	s_native_active.store(false, std::memory_order_relaxed);
	return native;
}

std::uint64_t rbx::current_camera_address()
{
	std::uint64_t ws = game::workspace.address;
	if (!ws && game::datamodel.address)
		ws = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace);
	if (!ws) return 0;

	std::uint64_t cam = memory->read<std::uint64_t>(ws + Offsets::Workspace::CurrentCamera);
	if (!cam || cam == ~0ull) return 0;
	return cam;
}

static float s_win_off_x = 0.f;
static float s_win_off_y = 0.f;

void rbx::visualengine_t::update_window_offset()
{
	HWND roblox_window = game::wnd;
	if (!roblox_window) { s_win_off_x = 0.f; s_win_off_y = 0.f; return; }

	if (IsIconic(roblox_window))
		return;

	POINT pt{ 0, 0 };
	if (ClientToScreen(roblox_window, &pt) &&
		pt.x > -30000 && pt.x < 30000 && pt.y > -30000 && pt.y < 30000)
	{
		s_win_off_x = (float)pt.x;
		s_win_off_y = (float)pt.y;
	}
}

float rbx::visualengine_t::window_offset_x() { return s_win_off_x; }
	float rbx::visualengine_t::window_offset_y() { return s_win_off_y; }

bool rbx::visualengine_t::world_to_screen(const math::vector3& world, math::vector2& out, const math::vector2& dims, const math::matrix4& view) const
{
	math::vector4 clip = view.multiply({ world.x, world.y, world.z, 1.0f });

	if (clip.w < 0.1f)
		return false;

	clip.x /= clip.w;
	clip.y /= clip.w;





	constexpr float k_ndc_max = 60.f;
	if (!(clip.x > -k_ndc_max && clip.x < k_ndc_max &&
	      clip.y > -k_ndc_max && clip.y < k_ndc_max))
		return false;

	out.x = (dims.x * 0.5f * clip.x) + (dims.x * 0.5f) + s_win_off_x;
	out.y = -(dims.y * 0.5f * clip.y) + (dims.y * 0.5f) + s_win_off_y;

	return true;
}

math::vector3 rbx::camera_t::get_position() const
{
	return memory->read<math::vector3>(this->address + Offsets::Camera::Position);
}

math::matrix3 rbx::camera_t::get_rotation() const
{
	return memory->read<math::matrix3>(this->address + Offsets::Camera::Rotation);
}

void rbx::camera_t::write_rotation(const math::matrix3& rotation)
{
	memory->write<math::matrix3>(this->address + Offsets::Camera::Rotation, rotation);
}

void rbx::camera_t::set_viewport(const rbx::vector2int16& val)
{
	if (!this->address) return;
	memory->write<rbx::vector2int16>(this->address + Offsets::Camera::Viewport, val);
}

float rbx::camera_t::get_field_of_view() const
{




	const float fov = memory->read<float>(this->address + Offsets::Camera::FieldOfView);
	if (fov > 0.05f && fov < 3.10f) return fov;
	return 1.22173f;
}

rbx::vector2int16 rbx::camera_calculate_viewport(const math::vector2& TargetPosition, const math::vector2& Size, const math::vector2& MousePosition)
{
	rbx::vector2int16 Result;

	double TargetY = static_cast<double>(TargetPosition.y);
	if (TargetY < 1.0)                                  TargetY = 1.0;
	if (TargetY > static_cast<double>(Size.y) - 1.0)   TargetY = static_cast<double>(Size.y) - 1.0;

	double Ratio = static_cast<double>(MousePosition.y) / TargetY;
	double VY    = static_cast<double>(Size.y) * Ratio;
	if (VY > 32767.0) VY = 32767.0;
	if (VY < 1.0)     VY = 1.0;

	Ratio = VY / static_cast<double>(Size.y);
	double VX = 2.0 * static_cast<double>(MousePosition.x)
	          - Ratio * (2.0 * static_cast<double>(TargetPosition.x) - static_cast<double>(Size.x));
	if (VX > 32767.0) VX = 32767.0;
	if (VX < 1.0)     VX = 1.0;

	Result.x = static_cast<int16_t>(std::round(VX));
	Result.y = static_cast<int16_t>(std::round(VY));
	return Result;
}

#include <xmmintrin.h>

std::uint64_t rbx::c_silent_help::cached_input_object = 0;

static std::uint64_t get_current_input_object(std::uint64_t base_address)
{
	return memory->read<std::uint64_t>(base_address + Offsets::MouseService::InputObject + sizeof(std::shared_ptr<void*>));
}

void rbx::c_silent_help::initialize_mouse_service(std::uint64_t address)
{
	cached_input_object = get_current_input_object(address);

	if (cached_input_object && cached_input_object != 0xFFFFFFFFFFFFFFFF)
	{
		const char* base = reinterpret_cast<const char*>(cached_input_object);
		_mm_prefetch(base + Offsets::MouseService::MousePosition, _MM_HINT_T0);
		_mm_prefetch(base + Offsets::MouseService::MousePosition + sizeof(math::vector2), _MM_HINT_T0);
	}
}

void rbx::c_silent_help::write_mouse_position(std::uint64_t address, float x, float y)
{
	cached_input_object = get_current_input_object(address);
	if (cached_input_object != 0 && cached_input_object != 0xFFFFFFFFFFFFFFFF)
	{
		memory->write<math::vector2>(cached_input_object + Offsets::MouseService::MousePosition, { x, y });
	}
}
