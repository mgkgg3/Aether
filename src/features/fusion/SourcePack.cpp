#define NOMINMAX
#include "SourcePack.h"

#include <settings.h>
#include <game/game.h>
#include <cache/cache.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <keybind/keybind.h>
#include <features/aimbot/aimbot.h>
#include <debug/DebugConsole.h>

#include <Windows.h>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fusion
{
	namespace
	{
		using clock_t_ = std::chrono::steady_clock;

		bool key_down(int vk)
		{
			return vk != 0 && (GetAsyncKeyState(vk) & 0x8000) != 0;
		}

		bool lmb_down()
		{
			return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		}

		std::uint64_t local_character()
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			return cache::cached_local_player.instance.address;
		}

		std::uint64_t local_humanoid()
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			return cache::cached_local_player.humanoid.address;
		}

		std::uint64_t local_root_part()
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			auto it = cache::cached_local_player.parts.find("HumanoidRootPart");
			if (it == cache::cached_local_player.parts.end())
				return 0;
			return it->second.address;
		}

		bool local_root_primitive(rbx::primitive_t& out)
		{
			std::lock_guard<std::mutex> lk(cache::mtx);
			auto it = cache::cached_local_player.parts.find("HumanoidRootPart");
			if (it == cache::cached_local_player.parts.end() || !it->second.address)
				return false;
			out = it->second.get_primitive();
			return out.address != 0;
		}

		// ---- anti-fling ------------------------------------------
		//
		// A fling is the character's assembly velocities leaving the band a
		// legal step can produce.  The reference saves the last quiet position
		// and, on the frame a fling is detected, pins the root back to it while
		// zeroing every part's velocity for a burst of writes - the physics
		// step has to be caught more than once because it re-applies the
		// velocity it computed before the first write landed.
		struct antifling_state_t
		{
			math::vector3 saved{};
			bool have_saved = false;
			std::uint64_t character = 0;
		};

		antifling_state_t g_fling{};

		void antifling_reset()
		{
			g_fling.have_saved = false;
			g_fling.character = 0;
		}

		void antifling_tick()
		{
			if (!settings::fusion::antifling)
			{
				antifling_reset();
				return;
			}

			const std::uint64_t character = local_character();
			if (!character)
			{
				antifling_reset();
				return;
			}

			if (character != g_fling.character)
			{
				g_fling.character = character;
				g_fling.have_saved = false;
			}

			rbx::primitive_t root{};
			if (!local_root_primitive(root))
				return;

			const math::vector3 lin = memory->read<math::vector3>(
				root.address + Offsets::Primitive::AssemblyLinearVelocity);
			const math::vector3 ang = memory->read<math::vector3>(
				root.address + Offsets::Primitive::AssemblyAngularVelocity);

			const float lin_sq = lin.x * lin.x + lin.y * lin.y + lin.z * lin.z;
			const float ang_sq = ang.x * ang.x + ang.y * ang.y + ang.z * ang.z;

			const float lin_trigger = settings::fusion::antifling_lin;
			const float ang_trigger = settings::fusion::antifling_ang;
			if (lin_trigger < 1.f) return;
			if (ang_trigger < 1.f) return;

			if (lin_sq < 100.f * 100.f && ang_sq < 50.f * 50.f)
			{
				const math::vector3 pos = root.get_position();
				if (std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z))
				{
					g_fling.saved = pos;
					g_fling.have_saved = true;
				}
				return;
			}

			if (lin_sq < lin_trigger * lin_trigger && ang_sq < ang_trigger * ang_trigger)
				return;

			const math::vector3 zero{ 0.f, 0.f, 0.f };

			// Every part, not only the root: a fling is applied to the whole
			// assembly, so zeroing the root alone leaves the limbs carrying it.
			std::vector<std::uint64_t> primitives;
			{
				std::lock_guard<std::mutex> lk(cache::mtx);
				primitives.reserve(cache::cached_local_player.parts.size());
				for (auto& kv : cache::cached_local_player.parts)
				{
					if (!kv.second.address) continue;
					rbx::primitive_t p = kv.second.get_primitive();
					if (p.address) primitives.push_back(p.address);
				}
			}

			for (int i = 0; i < 100; ++i)
			{
				if (g_fling.have_saved)
					memory->write<math::vector3>(root.address + Offsets::Primitive::Position,
						g_fling.saved);

				memory->write<math::vector3>(
					root.address + Offsets::Primitive::AssemblyLinearVelocity, zero);
				memory->write<math::vector3>(
					root.address + Offsets::Primitive::AssemblyAngularVelocity, zero);

				for (std::uint64_t prim : primitives)
				{
					memory->write<math::vector3>(
						prim + Offsets::Primitive::AssemblyLinearVelocity, zero);
					memory->write<math::vector3>(
						prim + Offsets::Primitive::AssemblyAngularVelocity, zero);
				}

				std::this_thread::yield();
			}

			g_fling.have_saved = false;
		}

		// ---- jump power ------------------------------------------
		//
		// The client keeps both a JumpPower and a UseJumpPower flag, and a game
		// that uses JumpHeight instead is why writing the power alone reads as
		// "nothing happened".  Both are restored together.
		struct jumppower_state_t
		{
			std::uint64_t humanoid = 0;
			float original_power = 50.f;
			bool  original_use = false;
			bool  captured = false;
			bool  active = false;
		};

		jumppower_state_t g_jump{};

		void jumppower_tick(keybind& kb)
		{
			kb.key  = settings::fusion::jumppower_key;
			kb.type = static_cast<keybind::c_keybind_type>(settings::fusion::jumppower_mode);
			kb.update();

			const bool want = settings::fusion::jumppower &&
				(settings::fusion::jumppower_key == 0 || kb.enabled);

			const std::uint64_t humanoid = local_humanoid();
			if (!humanoid)
			{
				g_jump = {};
				return;
			}

			if (humanoid != g_jump.humanoid)
			{
				g_jump = {};
				g_jump.humanoid = humanoid;
			}

			if (want)
			{
				if (!g_jump.captured)
				{
					g_jump.original_power = memory->read<float>(
						humanoid + Offsets::Humanoid::JumpPower);
					g_jump.original_use = memory->read<bool>(
						humanoid + Offsets::Humanoid::UseJumpPower);
					g_jump.captured = true;
				}

				const float value = settings::fusion::jumppower_value;
				memory->write<bool>(humanoid + Offsets::Humanoid::UseJumpPower, true);
				memory->write<float>(humanoid + Offsets::Humanoid::JumpPower, value);
				memory->write<float>(humanoid + Offsets::Humanoid::JumpHeight, value * 0.144f);
				g_jump.active = true;
			}
			else if (g_jump.captured)
			{
				memory->write<float>(humanoid + Offsets::Humanoid::JumpPower,
					g_jump.original_power);
				memory->write<bool>(humanoid + Offsets::Humanoid::UseJumpPower,
					g_jump.original_use);
				g_jump.captured = false;
				g_jump.active = false;
			}
		}

		// ---- hip height ------------------------------------------
		struct hipheight_state_t
		{
			std::uint64_t humanoid = 0;
			float original = 2.f;
			bool captured = false;
		};

		hipheight_state_t g_hip{};

		void hipheight_tick()
		{
			const std::uint64_t humanoid = local_humanoid();
			if (!humanoid)
			{
				g_hip = {};
				return;
			}

			if (humanoid != g_hip.humanoid)
			{
				g_hip = {};
				g_hip.humanoid = humanoid;
			}

			if (settings::fusion::hipheight)
			{
				if (!g_hip.captured)
				{
					g_hip.original = memory->read<float>(humanoid + Offsets::Humanoid::HipHeight);
					g_hip.captured = true;
				}
				memory->write<float>(humanoid + Offsets::Humanoid::HipHeight,
					settings::fusion::hipheight_value);
			}
			else if (g_hip.captured)
			{
				memory->write<float>(humanoid + Offsets::Humanoid::HipHeight, g_hip.original);
				g_hip.captured = false;
			}
		}

		// ---- freecam ---------------------------------------------
		//
		// CameraType 5 is the client's own Scriptable type: it stops the camera
		// script writing Position every frame, which is what makes a freecam
		// hold instead of snapping back.
		//
		// The field is written as one *byte*, not the reference's int.  In this
		// dump CameraType sits at 0x28f, immediately behind the 4-byte Viewport at
		// 0x28c, so an int write would run three bytes into whatever follows it.
		// The property is an enum token, which is a byte, and the value is
		// sanity-checked before it is touched: a token that is not a small number
		// means the offset is stale, and a stale offset is not somewhere to write.
		struct freecam_state_t
		{
			bool  captured = false;
			std::uint8_t original_type = 0;
			math::vector3 original_pos{};
			math::vector3 held{};
			bool held_valid = false;
			clock_t_::time_point last{};
			bool refused = false;
		};

		freecam_state_t g_free{};

		void freecam_release(std::uint64_t camera)
		{
			if (!g_free.captured) return;
			if (camera)
			{
				if (g_free.original_type != 0xFF)
					memory->write<std::uint8_t>(camera + Offsets::Camera::CameraType,
						g_free.original_type);
				if (settings::fusion::freecam_revert)
					memory->write<math::vector3>(camera + Offsets::Camera::Position,
						g_free.original_pos);
			}
			g_free = {};
		}

		void freecam_tick(keybind& kb)
		{
			kb.key  = settings::fusion::freecam_key;
			kb.type = static_cast<keybind::c_keybind_type>(settings::fusion::freecam_mode);
			kb.update();

			const bool want = settings::fusion::freecam &&
				(settings::fusion::freecam_key == 0 || kb.enabled);

			const std::uint64_t camera = rbx::current_camera_address();
			if (!want)
			{
				freecam_release(camera);
				return;
			}
			if (!camera) return;

			if (!g_free.captured)
			{
				const std::uint8_t token = memory->read<std::uint8_t>(
					camera + Offsets::Camera::CameraType);
				if (token > 12)
				{
					// Not a camera-type token - the field is something else in this
					// build.  Say so once, and carry on pinning the position instead:
					// a 2 ms write still outruns the camera script's per-frame one,
					// the hold is just less firm than it would be with the type set.
					if (!g_free.refused)
					{
						g_free.refused = true;
						debug::warn("freecam", "CameraType reads %u - offsets stale, position pin only",
							(unsigned)token);
					}
					g_free.original_type = 0xFF;   // 0xFF = "do not touch the type"
				}
				else
				{
					g_free.original_type = token;
				}

				g_free.original_pos  = memory->read<math::vector3>(camera + Offsets::Camera::Position);
				g_free.captured = true;
				g_free.last = clock_t_::now();
			}

			if (g_free.original_type != 0xFF)
				memory->write<std::uint8_t>(camera + Offsets::Camera::CameraType, 5);

			const math::matrix3 rotation = memory->read<math::matrix3>(camera + Offsets::Camera::Rotation);
			const math::vector3 position = memory->read<math::vector3>(camera + Offsets::Camera::Position);

			math::vector3 dir{};
			if (key_down('W')) dir.z -= 1.f;
			if (key_down('S')) dir.z += 1.f;
			if (key_down('A')) dir.x -= 1.f;
			if (key_down('D')) dir.x += 1.f;
			if (key_down(VK_SPACE) || key_down('E')) dir.y += 1.f;
			if (key_down(VK_LCONTROL) || key_down(VK_CONTROL) || key_down('Q')) dir.y -= 1.f;

			const clock_t_::time_point now = clock_t_::now();
			float dt = std::chrono::duration<float>(now - g_free.last).count();
			if (!(dt > 0.f) || dt > 0.1f) dt = 0.004f;
			g_free.last = now;

			// The position the freecam holds.  Reading it back and rewriting it
			// would be a no-op - the camera has already moved on by the time this
			// runs - so the held point is ours and every idle tick writes *that*.
			// Without it the script's own write wins whenever the type field could
			// not be set, and the camera creeps back to the character.
			if (dir.length() > 0.f)
			{
				dir = dir.normalized();
				const math::vector3 moved = rotation * dir;
				const float speed = key_down(VK_SHIFT) ? settings::fusion::freecam_sprint
					: settings::fusion::freecam_speed;
				g_free.held = position + moved * (speed * dt);
			}
			else if (!g_free.held_valid)
			{
				g_free.held = position;
			}

			g_free.held_valid = true;
			memory->write<math::vector3>(camera + Offsets::Camera::Position, g_free.held);
		}

		// ---- orbit -----------------------------------------------
		//
		// Writes the root's position on a circle around the chosen target.  The
		// velocity is zeroed every step for the same reason the source does it:
		// the physics step integrates the old velocity into the next position
		// and the orbit drifts off its radius.
		struct orbit_state_t
		{
			float angle = 0.f;
			bool  spectating = false;
			std::uint64_t original_subject = 0;
		};

		orbit_state_t g_orbit{};

		bool orbit_target(std::uint64_t& instance, math::vector3& pos)
		{
			std::lock_guard<std::mutex> lk(cache::mtx);

			const cache::entity_t* pick = nullptr;

			if (settings::fusion::orbit_target == 1 &&
				aimbot::player.instance.address != 0)
			{
				for (const auto& e : cache::cached_players)
					if (e.instance.address == aimbot::player.instance.address)
					{ pick = &e; break; }
			}

			if (!pick)
			{
				math::vector3 local{};
				if (auto lit = cache::cached_local_player.parts.find("HumanoidRootPart");
					lit != cache::cached_local_player.parts.end() && lit->second.address)
				{
					rbx::primitive_t lp = lit->second.get_primitive();
					if (lp.address)
						local = memory->read<math::vector3>(
							lp.address + Offsets::Primitive::Position);
				}

				float best = 1e9f;
				for (const auto& e : cache::cached_players)
				{
					if (e.instance.address == 0) continue;
					if (e.instance.address == cache::cached_local_player.instance.address) continue;
					auto it = e.parts.find("UpperTorso");
					if (it == e.parts.end()) it = e.parts.find("Torso");
					if (it == e.parts.end() || !it->second.address) continue;
					rbx::primitive_t prim = it->second.get_primitive();
					if (!prim.address) continue;
					const math::vector3 p = memory->read<math::vector3>(
						prim.address + Offsets::Primitive::Position);
					const float d = (p - local).length();
					if (d < best) { best = d; pick = &e; }
				}
			}

			if (!pick) return false;

			auto it = pick->parts.find("UpperTorso");
			if (it == pick->parts.end()) it = pick->parts.find("Torso");
			if (it == pick->parts.end() || !it->second.address) return false;

			rbx::primitive_t prim = it->second.get_primitive();
			if (!prim.address) return false;

			instance = pick->instance.address;
			pos = memory->read<math::vector3>(prim.address + Offsets::Primitive::Position);
			return true;
		}

		void orbit_release(std::uint64_t camera)
		{
			if (!g_orbit.spectating) return;
			if (camera)
				memory->write<std::uint64_t>(camera + Offsets::Camera::CameraSubject,
					g_orbit.original_subject);
			g_orbit.spectating = false;
			g_orbit.original_subject = 0;
		}

		void orbit_tick(keybind& kb)
		{
			kb.key  = settings::fusion::orbit_key;
			kb.type = static_cast<keybind::c_keybind_type>(settings::fusion::orbit_activation);
			kb.update();

			const bool want = settings::fusion::orbit &&
				(settings::fusion::orbit_key == 0 || kb.enabled);

			const std::uint64_t camera = rbx::current_camera_address();
			if (!want)
			{
				orbit_release(camera);
				return;
			}

			rbx::primitive_t root{};
			if (!local_root_primitive(root)) return;

			std::uint64_t target_instance = 0;
			math::vector3 target_pos{};
			if (!orbit_target(target_instance, target_pos)) return;

			const float speed = settings::fusion::orbit_speed;
			g_orbit.angle += speed * 0.05f;
			if (g_orbit.angle > 6.2831853f) g_orbit.angle -= 6.2831853f;

			const float radius = settings::fusion::orbit_radius;
			const math::vector3 next{
				target_pos.x + std::cos(g_orbit.angle) * radius,
				target_pos.y + settings::fusion::orbit_height,
				target_pos.z + std::sin(g_orbit.angle) * radius };

			memory->write<math::vector3>(root.address + Offsets::Primitive::Position, next);
			memory->write<math::vector3>(root.address + Offsets::Primitive::AssemblyLinearVelocity, {});
			memory->write<math::vector3>(root.address + Offsets::Primitive::AssemblyAngularVelocity, {});

			if (camera && settings::fusion::orbit_spectate)
			{
				if (!g_orbit.spectating)
				{
					g_orbit.original_subject = memory->read<std::uint64_t>(
						camera + Offsets::Camera::CameraSubject);
					g_orbit.spectating = true;
				}
				memory->write<std::uint64_t>(camera + Offsets::Camera::CameraSubject,
					target_instance);
			}
			else if (g_orbit.spectating)
			{
				orbit_release(camera);
			}
		}

		// ---- anti-recoil ------------------------------------
		//
		// The reference's own numbers: a fixed walk per 5 ms tick while the
		// fire button is down.  The remainder is carried between ticks so a
		// sub-count strength still moves the mouse instead of rounding to zero
		// forever.
		void norecoil_tick()
		{
			static float accum_x = 0.f;
			static float accum_y = 0.f;
			static clock_t_::time_point last_step{};
			static bool primed = false;

			// One step per 5 ms, which is the reference's own cadence.  The loop
			// runs faster than that and the strength is a per-step amount, so
			// without the gate the compensation is simply multiplied by however
			// often this happens to be called.
			const clock_t_::time_point now = clock_t_::now();
			if (primed &&
				std::chrono::duration_cast<std::chrono::milliseconds>(now - last_step).count() < 5)
				return;
			last_step = now;
			primed = true;

			const bool active = settings::fusion::norecoil &&
				(settings::fusion::norecoil_key == 0 || key_down(settings::fusion::norecoil_key)) &&
				lmb_down();

			if (!active)
			{
				accum_x = accum_y = 0.f;
				primed = false;
				return;
			}

			const float strength = settings::fusion::norecoil_strength;
			accum_x += settings::fusion::norecoil_x * strength;
			accum_y += settings::fusion::norecoil_y * strength;

			const int dx = (int)accum_x;
			const int dy = (int)accum_y;
			accum_x -= (float)dx;
			accum_y -= (float)dy;

			if (dx == 0 && dy == 0) return;

			INPUT input{};
			input.type = INPUT_MOUSE;
			input.mi.dx = dx;
			input.mi.dy = dy;
			input.mi.dwFlags = MOUSEEVENTF_MOVE;
			SendInput(1, &input, sizeof(input));
		}

		// ---- headless --------------------------------------------
		//
		// The source writes the head primitive's size to zero.  The size is
		// captured first and put back when the switch goes off, because a
		// zero-size head is not something the client repairs by itself.
		struct headless_state_t
		{
			std::uint64_t primitive = 0;
			math::vector3 original{};
			bool captured = false;
		};

		headless_state_t g_head{};

		void headless_release()
		{
			if (!g_head.captured) return;
			if (g_head.primitive)
				memory->write<math::vector3>(g_head.primitive + Offsets::Primitive::Size,
					g_head.original);
			g_head = {};
		}

		void headless_tick()
		{
			if (!settings::fusion::headless)
			{
				headless_release();
				return;
			}

			std::uint64_t head = 0;
			{
				std::lock_guard<std::mutex> lk(cache::mtx);
				auto it = cache::cached_local_player.parts.find("Head");
				if (it != cache::cached_local_player.parts.end())
					head = it->second.address;
			}
			if (!head) return;

			rbx::primitive_t prim = rbx::part_t{ head }.get_primitive();
			if (!prim.address) return;

			if (g_head.primitive != prim.address)
			{
				headless_release();
				g_head.primitive = prim.address;
				g_head.original = memory->read<math::vector3>(
					prim.address + Offsets::Primitive::Size);
				g_head.captured = true;
			}

			memory->write<math::vector3>(prim.address + Offsets::Primitive::Size, {});
		}
		// ---- korblox ------------------------------------------------
		//
		// Two parts of the local rig hidden, which is the silhouette the
		// reference gets out of the same two.  The original transparencies are
		// read before the first write and put back when the switch goes off or
		// the character is replaced - the source does not restore them, and a
		// character that respawned while it was on keeps the hole forever.
		struct korblox_state_t
		{
			std::uint64_t character{ 0 };
			std::unordered_map<std::uint64_t, float> original;
		};

		korblox_state_t g_korblox{};

		void korblox_release(bool restore)
		{
			if (restore)
				for (const auto& kv : g_korblox.original)
					if (kv.first)
						memory->write<float>(kv.first + Offsets::BasePart::Transparency, kv.second);
			g_korblox = {};
		}

		void korblox_tick()
		{
			if (!settings::fusion::korblox)
			{
				korblox_release(true);
				return;
			}

			std::uint64_t character = 0;
			std::vector<std::uint64_t> parts;
			{
				std::lock_guard<std::mutex> lk(cache::mtx);
				character = cache::cached_local_player.instance.address;
				for (const char* n : { "RightFoot", "RightLowerLeg" })
				{
					auto it = cache::cached_local_player.parts.find(n);
					if (it != cache::cached_local_player.parts.end() && it->second.address)
						parts.push_back(it->second.address);
				}
			}

			if (!character || parts.empty())
			{
				korblox_release(true);
				return;
			}

			// A new character means the old addresses belong to a rig that is
			// already gone: drop them without writing to freed memory.
			if (character != g_korblox.character)
			{
				korblox_release(false);
				g_korblox.character = character;
			}

			for (std::uint64_t part : parts)
			{
				if (!g_korblox.original.count(part))
					g_korblox.original[part] = memory->read<float>(
						part + Offsets::BasePart::Transparency);
				memory->write<float>(part + Offsets::BasePart::Transparency, 1.f);
			}
		}

		// ---- spectate --------------------------------------------
		//
		// CameraSubject is pointed at another player's root and CameraType is
		// forced to Scriptable, which is the whole of the reference's follower
		// camera.  The cycle key walks the roster; the originals go back when
		// the switch goes off, so the local camera is never left on another
		// body - a stale subject outlives the feature and is invisible until
		// the next respawn.
		struct spectate_state_t
		{
			bool          active{ false };
			std::uint8_t  saved_type{ 0xFF };
			std::uint64_t saved_subject{ 0 };
			std::uint64_t subject{ 0 };
			int           index{ -1 };
			bool          key_was{ false };
		};

		spectate_state_t g_spectate{};

		void spectate_release(std::uint64_t camera)
		{
			if (!g_spectate.active) return;
			if (camera)
			{
				memory->write<std::uint64_t>(camera + Offsets::Camera::CameraSubject,
					g_spectate.saved_subject);
				if (g_spectate.saved_type != 0xFF)
					memory->write<std::uint8_t>(camera + Offsets::Camera::CameraType,
						g_spectate.saved_type);
			}
			g_spectate = {};
		}

		void spectate_tick()
		{
			const std::uint64_t camera = rbx::current_camera_address();
			if (!settings::fusion::spectate)
			{
				spectate_release(camera);
				return;
			}
			if (!camera) return;

			const bool down = key_down(settings::fusion::spectate_cycle_key);
			if (down && !g_spectate.key_was)
				++g_spectate.index;
			g_spectate.key_was = down;

			std::uint64_t root = 0;
			{
				std::lock_guard<std::mutex> lk(cache::mtx);
				const std::size_t count = cache::cached_players.size();

				// A pinned aim target wins over the cycle: the body the aim is
				// holding is the body being watched.
				const std::uint64_t pinned = aimbot::player.instance.address;
				if (pinned)
				{
					for (const auto& e : cache::cached_players)
					{
						if (e.instance.address != pinned) continue;
						auto it = e.parts.find("HumanoidRootPart");
						if (it != e.parts.end()) root = it->second.address;
						break;
					}
				}

				if (!root && count)
				{
					if (g_spectate.index < 0 || (std::size_t)g_spectate.index >= count)
						g_spectate.index = 0;

					const auto& e = cache::cached_players[(std::size_t)g_spectate.index];
					if (e.instance.address != cache::cached_local_player.instance.address)
					{
						auto it = e.parts.find("HumanoidRootPart");
						if (it != e.parts.end()) root = it->second.address;
					}
				}
			}
			if (!root) return;

			if (!g_spectate.active)
			{
				g_spectate.saved_subject = memory->read<std::uint64_t>(
					camera + Offsets::Camera::CameraSubject);
				g_spectate.saved_type = memory->read<std::uint8_t>(
					camera + Offsets::Camera::CameraType);
				if (g_spectate.saved_type > 12) g_spectate.saved_type = 0xFF;
				g_spectate.active = true;
				g_spectate.subject = 0;
			}

			if (root != g_spectate.subject)
			{
				memory->write<std::uint64_t>(camera + Offsets::Camera::CameraSubject, root);
				g_spectate.subject = root;
			}
			memory->write<std::uint8_t>(camera + Offsets::Camera::CameraType, 4);
		}

		// ---- fly above ---------------------------------------------
		//
		// Rides the pinned target from twelve studs overhead.  Fifteen repeats
		// per tick, like the source: the physics step re-applies the position
		// it computed before the first write landed, so one write loses the
		// race on most frames.
		void flyabove_tick()
		{
			if (!settings::fusion::flyabove) return;

			rbx::primitive_t root{};
			if (!local_root_primitive(root)) return;

			std::uint64_t target_instance = 0;
			math::vector3 target_pos{};
			if (!orbit_target(target_instance, target_pos)) return;

			const math::vector3 next{
				target_pos.x,
				target_pos.y + std::max(1.f, settings::fusion::flyabove_height),
				target_pos.z };

			const math::matrix3 rot = memory->read<math::matrix3>(
				root.address + Offsets::Primitive::Rotation);

			for (int i = 0; i < 15; ++i)
			{
				memory->write<math::vector3>(root.address + Offsets::Primitive::Position, next);
				memory->write<math::matrix3>(root.address + Offsets::Primitive::Rotation, rot);
				memory->write<math::vector3>(root.address + Offsets::Primitive::AssemblyLinearVelocity, {});
				memory->write<math::vector3>(root.address + Offsets::Primitive::AssemblyAngularVelocity, {});
			}
		}
	}

	// ---- anti-AFK --------------------------------------------------
	//
	// The reference writes the party inactivity FFlag.  That needs a flag
	// address this build has no measured value for, and a wrong address is a
	// write into an object nobody has identified - so the idle clock is reset
	// the way Windows itself measures it instead: one pixel of cursor movement,
	// there and back.  The client counts it as input, the game cannot bind it,
	// and nothing in the process is written.
	void antiafk_tick()
	{
		if (!settings::fusion::antiafk) return;

		LASTINPUTINFO li{};
		li.cbSize = sizeof li;
		if (!GetLastInputInfo(&li)) return;

		const std::uint64_t idle_ms = GetTickCount64() - li.dwTime;
		const float limit_s = settings::fusion::antiafk_idle < 5.f
			? 5.f : settings::fusion::antiafk_idle;
		if (idle_ms < (std::uint64_t)(limit_s * 1000.f)) return;

		POINT p{};
		if (!GetCursorPos(&p)) return;
		SetCursorPos(p.x + 1, p.y);
		SetCursorPos(p.x, p.y);
	}

	// ---- peek return ------------------------------------------
	//
	// The simplest of the reference's three peek modes: the spot the character
	// stood on when the key went down is saved, and it is walked back to at the
	// set speed - on the key's release, or on the next shot - then stopped.  The
	// return is velocity writes repeated a few times per pass, because the
	// physics step re-applies the velocity it computed before the first write
	// landed (the same reason anti-fling bursts its writes).
	struct peek_state_t
	{
		bool          have_saved = false;
		bool          returning = false;
		bool          key_was = false;
		bool          lmb_was = false;
		math::vector3 saved{};
	};

	peek_state_t g_peek{};

	void peek_reset()
	{
		g_peek.have_saved = false;
		g_peek.returning = false;
		g_peek.key_was = false;
		g_peek.lmb_was = false;
	}

	void peek_tick()
	{
		if (!settings::fusion::peek_return || settings::fusion::peek_key == 0)
		{
			peek_reset();
			return;
		}

		rbx::primitive_t prim{};
		if (!local_root_primitive(prim))
		{
			peek_reset();
			return;
		}

		const bool key = key_down(settings::fusion::peek_key);
		const bool lmb = lmb_down();
		const bool lmb_edge = lmb && !g_peek.lmb_was;
		g_peek.lmb_was = lmb;

		if (settings::fusion::peek_save_on_hold && key && !g_peek.key_was)
		{
			g_peek.saved = memory->read<math::vector3>(prim.address + Offsets::Primitive::Position);
			g_peek.have_saved = true;
			g_peek.returning = false;
		}
		g_peek.key_was = key;

		if (g_peek.have_saved && !g_peek.returning)
		{
			const bool click_trigger = settings::fusion::peek_trigger == 1;
			if (click_trigger ? lmb_edge : !key)
				g_peek.returning = true;
		}

		if (!g_peek.returning || !g_peek.have_saved) return;

		const math::vector3 cur = memory->read<math::vector3>(
			prim.address + Offsets::Primitive::Position);
		const float dx = g_peek.saved.x - cur.x;
		const float dy = g_peek.saved.y - cur.y;
		const float dz = g_peek.saved.z - cur.z;
		const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

		if (dist <= 1.5f)
		{
			memory->write<math::vector3>(
				prim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
			g_peek.have_saved = false;
			g_peek.returning = false;
			return;
		}

		const float speed = settings::fusion::peek_speed < 5.f
			? 5.f : (settings::fusion::peek_speed > 400.f ? 400.f : settings::fusion::peek_speed);
		const float inv = speed / dist;
		const math::vector3 vel{ dx * inv, dy * inv, dz * inv };

		for (int i = 0; i < 4; ++i)
			memory->write<math::vector3>(
				prim.address + Offsets::Primitive::AssemblyLinearVelocity, vel);
	}

	// ---- Auto clicker ------------------------------------------------------
	//
	// A held key becomes clicks at the set rate.  The interval is timed from
	// GetTickCount64 rather than slept through, so a busy frame delays the next
	// click instead of shifting every click after it; the jitter, when it is
	// asked for, spreads the interval by a share of itself either way.
	std::uint64_t g_click_next = 0;
	std::uint32_t g_click_seed = 0x9e3779b9u;

	void autoclicker_tick()
	{
		if (!settings::fusion::autoclicker || settings::fusion::autoclicker_key == 0)
		{
			g_click_next = 0;
			return;
		}
		if (!key_down(settings::fusion::autoclicker_key))
		{
			g_click_next = 0;
			return;
		}

		const std::uint64_t now = GetTickCount64();
		if (g_click_next != 0 && now < g_click_next) return;

		INPUT in[2]{};
		in[0].type = INPUT_MOUSE;
		in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		in[1].type = INPUT_MOUSE;
		in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(2, in, sizeof(INPUT));

		float cps = settings::fusion::autoclicker_cps;
		if (cps < 1.f) cps = 1.f;
		if (cps > 30.f) cps = 30.f;

		float interval = 1000.f / cps;

		float jitter = settings::fusion::autoclicker_jitter;
		if (jitter < 0.f) jitter = 0.f;
		if (jitter > 0.9f) jitter = 0.9f;
		if (jitter > 0.f)
		{
			g_click_seed = g_click_seed * 1664525u + 1013904223u;
			const float unit = (float)((g_click_seed >> 8) & 0xFFFFu) / 65535.f; // 0..1
			interval *= 1.f + (unit * 2.f - 1.f) * jitter;
		}

		if (interval < 8.f) interval = 8.f;
		g_click_next = now + (std::uint64_t)interval;
	}

	// ---- Camera field of view ---------------------------------------------
	//
	// One float on the camera object, written every slow tick because the game
	// keeps pushing its own value back.  The value found on the first write is
	// kept and restored when the switch goes off - and re-read when the camera
	// is replaced, which happens on every respawn.
	struct camfov_state_t
	{
		std::uint64_t camera = 0;
		float         saved = 70.f;
		bool          have_saved = false;
	};

	camfov_state_t g_camfov{};

	void camfov_restore()
	{
		if (!g_camfov.have_saved) return;
		if (g_camfov.camera)
			memory->write<float>(
				g_camfov.camera + Offsets::Camera::FieldOfView, g_camfov.saved);
		g_camfov.have_saved = false;
		g_camfov.camera = 0;
	}

	void camfov_tick()
	{
		if (!settings::fusion::camera_fov)
		{
			camfov_restore();
			return;
		}

		const std::uint64_t camera = rbx::current_camera_address();
		if (!camera) return;

		if (g_camfov.camera != camera)
		{
			g_camfov.camera = camera;
			g_camfov.saved = memory->read<float>(
				camera + Offsets::Camera::FieldOfView);
			g_camfov.have_saved = true;
		}

		float want = settings::fusion::camera_fov_value;
		if (want < 1.f) want = 1.f;
		if (want > 120.f) want = 120.f;
		memory->write<float>(camera + Offsets::Camera::FieldOfView, want);
	}

	void run()
	{
		keybind jump_kb("fusion_jumppower");
		keybind free_kb("fusion_freecam");
		keybind orbit_kb("fusion_orbit");

		std::uint64_t last_slow = 0;

		for (;;)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(2));

			if (game::datamodel.address == 0)
			{
				antifling_reset();
				continue;
			}

			// The mouse features run at the loop's own rate; the memory sweeps
			// below are cheap enough to run at a fraction of it and the client
			// does not re-read them faster than that anyway.
			norecoil_tick();
			autoclicker_tick();
			freecam_tick(free_kb);

			const std::uint64_t now = GetTickCount64();
			if (now - last_slow < 8)
				continue;
			last_slow = now;

			antifling_tick();
			antiafk_tick();
			peek_tick();
			camfov_tick();
			jumppower_tick(jump_kb);
			hipheight_tick();
			orbit_tick(orbit_kb);
			korblox_tick();
			spectate_tick();
			flyabove_tick();
			headless_tick();
		}
	}

	void draw_overlay(ImDrawList* dl, const math::vector2& dims)
	{
		if (!dl || !settings::fusion::show_fps) return;
		if (game::visengine.address == 0) return;

		const float x = rbx::visualengine_t::window_offset_x() + 12.f;
		const float y = rbx::visualengine_t::window_offset_y() + 8.f;

		char text[32];
		std::snprintf(text, sizeof text, "%.0f fps", ImGui::GetIO().Framerate);

		ImFont* font = ImGui::GetFont();
		if (!font) return;
		const float fs = font->FontSize;

		dl->AddText(font, fs, ImVec2(x + 1.f, y + 1.f), IM_COL32(0, 0, 0, 200), text);
		dl->AddText(font, fs, ImVec2(x, y), IM_COL32(235, 235, 240, 235), text);
		(void)dims;
	}
}
