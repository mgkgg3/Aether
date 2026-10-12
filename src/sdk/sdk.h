#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include "math/math.h"

namespace rbx
{
	struct instance_t;
	struct primitive_t;
	struct model_instance_t;

	struct addressable_t
	{
		std::uint64_t address;

		addressable_t() : address(0) {}
		addressable_t(std::uint64_t address) : address(address) {}
	};

	struct nameable_t : public addressable_t
	{
		using addressable_t::addressable_t;

		std::string get_name() const;
		std::string get_class_name() const;
	};

	struct interface_t
	{
		template <typename T>
		std::vector<T> get_children() const;

		std::vector<rbx::instance_t> get_children() const;
		rbx::instance_t find_first_child(std::string_view str) const;
		rbx::instance_t find_first_child_by_class(std::string_view str) const;
	};

	struct instance_t : public nameable_t, public interface_t
	{
		using nameable_t::nameable_t;

		void set_instance_double(double v);
	};

	struct player_t final : public instance_t
	{
		using instance_t::instance_t;

		rbx::model_instance_t get_model_instance() const;






		std::uint64_t get_team() const;

		std::uint32_t get_team_color() const;
	};

	struct model_instance_t final : public instance_t
	{
		using instance_t::instance_t;
	};

	struct humanoid_t final : public addressable_t
	{
		using addressable_t::addressable_t;

		std::uint8_t get_rig_type() const;
		float get_health() const;
		float get_max_health() const;
		int get_state() const;
		math::matrix3 get_cframe_rotation() const;
		void set_jump_power(float power);
		static void write_gravity(float gravity);
		static void write_tickrate(float tickrate);
	};

	struct part_t : public instance_t
	{
		using instance_t::instance_t;

		rbx::primitive_t get_primitive() const;
	};

	struct primitive_t final : public addressable_t
	{
		using addressable_t::addressable_t;

		math::vector3 get_size() const;
		void set_size(const math::vector3& size);
		math::vector3 get_position() const;
		math::matrix3 get_rotation() const;
		bool get_can_collide() const;
		bool set_can_collide(bool enable);
	};

	struct visualengine_t final : public addressable_t
	{
		math::vector2 get_dimensions() const;









		math::matrix4 get_viewmatrix() const;



		static bool using_native_projection();




		static std::uintptr_t native_projection_offset();

		static void update_window_offset();
		static float window_offset_x();
		static float window_offset_y();
		bool world_to_screen(const math::vector3& world, math::vector2& out, const math::vector2& dims, const math::matrix4& view) const;
	};



	std::uint64_t current_camera_address();

	struct vector2int16 final
	{
		int16_t x{ 0 };
		int16_t y{ 0 };
	};

	struct camera_t final : public instance_t
	{
		using instance_t::instance_t;

		math::vector3 get_position() const;
		math::matrix3 get_rotation() const;

		float get_field_of_view() const;
		void write_rotation(const math::matrix3& rotation);
		void set_viewport(const vector2int16& val);
	};

	vector2int16 camera_calculate_viewport(const math::vector2& target, const math::vector2& size, const math::vector2& mouse);

	struct c_silent_help final : public addressable_t
	{
		using addressable_t::addressable_t;

		static std::uint64_t cached_input_object;
		void initialize_mouse_service(std::uint64_t address);
		void write_mouse_position(std::uint64_t address, float x, float y);
	};
}

template <typename T>
std::vector<T> rbx::interface_t::get_children() const
{
	const rbx::instance_t* base = static_cast<const rbx::instance_t*>(this);

	std::uint64_t start = memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart);
	std::uint64_t end = memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd);

	std::vector<T> children;
	children.reserve(32);







	for (std::uint64_t instance = memory->read<std::uint64_t>(start);
	     instance != end && children.size() < 4096;
	     instance += sizeof(std::shared_ptr<void*>))
	{
		children.emplace_back(memory->read<std::uint64_t>(instance));
	}

	return children;
}
