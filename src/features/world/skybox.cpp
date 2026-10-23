#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "skybox.h"
#include "SkyPresets.h"
#include "world_effects.h"

#include <game/game.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <settings.h>
#include <debug/DebugConsole.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

// The skybox presets, applied to the place's own Sky.
//
// What the two catalogs are: 67 skies, each six face ContentIds, an optional sun
// and moon texture, and the star count and the two angular sizes.  What the
// client keeps them in: the Sky instance under Lighting, at the offsets in
// `Offsets::Sky`, and those offsets were read back off a live client rather than
// taken on faith (Lighting+0x1b8 is the Sky, its six faces are the six `Content`
// strings at 0xe8..0x1d8, SunTextureId is at
// 0x208, MoonTextureId at 0xb8, and the numbers are SunAngularSize 0x22c,
// MoonAngularSize 0x234, StarCount 0x238, CelestialBodiesShown 0x240).
//
// The rotation is the one field that is *not* the three floats the name implies:
// SunAngularSize sits at 0x22c, immediately behind 0x228, so a Vector3 written
// there would overwrite the sun's own angular size with the rotation's middle
// component.  The world tab's own sky rows write the single float at 0x228 for
// that reason, and so does this file.
//
// The one interesting decision in this file is still the faces, and it is two
// decisions.
//
// First, where a face is.  A face is a `Content`: an MSVC std::string with
// sixteen bytes of source type and flag immediately *before* it, thirty in all -
// which is the 0x30 stride between the faces.  Every offset the catalog gives is
// the string itself, so its object runs from 0x10 before that offset to 0x20
// past it.  The last face is the arithmetic that proves the reading: the text of
// SunTextureId (0x208) runs to 0x228 with its 0x10 of metadata just in front of
// it, and 0x228 is exactly where the rotation float starts - SunAngularSize
// (0x22c), MoonAngularSize (0x234), StarCount (0x238) and CelestialBodiesShown
// (0x240) then fall in with no overlap at all.  Read the other way round -
// metadata at the offset, text 0x10 past it - that last face would run to 0x238
// and swallow all of them.
//
// Second, what may be written there.  A string can only be written inside the
// buffer the client already allocated for that field.  Handing the field a block
// of our own would be a block the client's own free is asked to release, and its
// heap never handed it out - the free either corrupts the heap or takes the
// client down when the Sky is destroyed on a map change.  Growing the field with
// VirtualAllocEx was tried and is what this file used to do; it is not safe, so
// it is gone.  Instead the value is composed to fit what is already there: the
// full "rbxassetid://NNN" where the field has the room for it, and the bare id
// (which Roblox's own Content parser still accepts) where it does not, which is
// the fifteen bytes of small-string buffer an unset face has.  A face whose
// buffer fits neither is counted as refused rather than forced.
//
// A place with no Sky under Lighting at all is the one case this cannot cover:
// making an instance means calling the client's own constructor, the tree has no
// instance-call path compiled in (see the note in tick()), and the status line
// says so instead of pretending the preset landed.

namespace skybox
{
	namespace
	{
		using sky_clock = std::chrono::steady_clock;

		std::uint64_t g_sky = 0;
		std::uint64_t g_lighting = 0;
		std::uint64_t g_datamodel = 0;

		bool g_applied = false;
		bool g_warned_no_sky = false;
		bool g_warned_no_room = false;

		int  g_written = 0;
		int  g_refused = 0;

		char g_status[224] = "idle";

		void release_now();

		// ---- the Content block ----
		//
		// MSVC's std::string is 32 bytes: the characters themselves when the size
		// is 15 or fewer (the small-string buffer), otherwise a pointer to a heap
		// buffer, then the size and the capacity.  The offset in the table is the
		// string, and the Content's own metadata is the 0x10 in front of it.

		struct text_read
		{
			bool        ok = false;
			std::size_t len = 0;
			char        text[96]{};
		};

		text_read read_text(std::uint64_t block)
		{
			text_read out;

			try
			{
				const std::uint64_t size = memory->read<std::uint64_t>(block + 0x10);
				if (size > 4096) return out;

				out.len = (std::size_t)size;
				out.ok  = true;

				const std::size_t n = (std::min)(out.len, sizeof out.text - 1);

				if (size <= 15)
				{
					for (std::size_t i = 0; i < n; ++i)
						out.text[i] = memory->read<char>(block + i);
				}
				else
				{
					const std::uint64_t ptr = memory->read<std::uint64_t>(block);
					if (ptr < 0x10000) { out.ok = false; return out; }
					for (std::size_t i = 0; i < n; ++i)
						out.text[i] = memory->read<char>(ptr + i);
				}

				out.text[n] = 0;
			}
			catch (...) { out.ok = false; }

			return out;
		}

		// A string written into the client's own Content field.
		//
		// Only ever inside memory the client already owns.  A face the place
		// never set is the fifteen-byte small-string buffer, so the text has to
		// fit those fifteen bytes or the write is refused; that is why the bare
		// id form exists in target_text().  A field the place did set has a heap
		// buffer behind it, and a value that fits that buffer is written through
		// the client's own pointer.
		bool write_text(std::uint64_t block, const char* text)
		{
			const std::size_t len = text ? std::strlen(text) : 0;
			if (len > 96) return false;

			try
			{
				const std::uint64_t size = memory->read<std::uint64_t>(block + 0x10);
				const std::uint64_t cap  = memory->read<std::uint64_t>(block + 0x18);
				if (size > 4096 || cap > 4096) return false;   // not a string: leave it alone

				if (len <= 15)
				{
					for (int i = 0; i < 16; ++i)
						memory->write<char>(block + i, i < (int)len ? text[i] : 0);

					// Back to the small-string form: the capacity goes with it, or
					// the client would later free a pointer that is now text.
					memory->write<std::uint64_t>(block + 0x10, (std::uint64_t)len);
					memory->write<std::uint64_t>(block + 0x18, 15);
					return true;
				}

				// Longer than the small-string buffer: the field has to already
				// have a heap buffer with room.  Never allocate one of ours and
				// store it here - the client's own free would be asked to release
				// a block its heap never handed out.
				if (cap < (std::uint64_t)len) return false;

				const std::uint64_t ptr = memory->read<std::uint64_t>(block);
				if (ptr < 0x10000) return false;

				for (std::size_t i = 0; i < len; ++i)
					memory->write<char>(ptr + i, text[i]);
				memory->write<char>(ptr + len, 0);

				memory->write<std::uint64_t>(block + 0x10, (std::uint64_t)len);
				return true;
			}
			catch (...) { return false; }
		}

		// What one field should hold, chosen to fit the buffer the field already
		// has.  The URL form is the canonical one; the bare id is the fallback
		// for a face the place never set, whose whole buffer is fifteen bytes -
		// Roblox's Content parser still resolves a numeric id there, and every
		// face in the catalog is fifteen characters or shorter.  A value that
		// starts with rbxasset:// or http resolves on its own and is not an
		// asset id, so it is passed through untouched.
		std::string target_text(const char* id, std::uint64_t text)
		{
			if (!id || !id[0]) return std::string();

			std::uint64_t cap = 0;
			try
			{
				const std::uint64_t size = memory->read<std::uint64_t>(text + 0x10);
				cap = memory->read<std::uint64_t>(text + 0x18);
				if (size > 4096 || cap > 4096) return std::string();
			}
			catch (...) { return std::string(); }

			const std::string raw = id;
			if (raw.rfind("rbxassetid://", 0) == 0 || raw.rfind("rbxasset://", 0) == 0 ||
				raw.rfind("http://", 0) == 0 || raw.rfind("https://", 0) == 0)
				return raw;

			const std::string url = "rbxassetid://" + raw;
			if (url.size() <= cap) return url;
			if (raw.size() <= 15)  return raw;
			return std::string();
		}

		// ---- finding the Sky ----

		std::uint64_t find_service(const char* class_name, const char* name)
		{
			if (!game::datamodel.address) return 0;

			try
			{
				std::vector<rbx::instance_t> kids = game::datamodel.get_children();

				for (rbx::instance_t& c : kids)
				{
					std::string cls;
					try { cls = c.get_class_name(); } catch (...) { continue; }
					if (cls == class_name) return c.address;
				}

				for (rbx::instance_t& c : kids)
				{
					std::string nm;
					try { nm = c.get_name(); } catch (...) { continue; }
					while (!nm.empty() && (nm.back() == ' ' || nm.back() == '\t')) nm.pop_back();
					if (nm == name) return c.address;
				}
			}
			catch (...) {}

			return 0;
		}

		// The Sky the writes go to.  Re-resolved when the place changes and when
		// the instance we had goes away: a map change builds a new Lighting, and a
		// stale Sky is a write into freed memory.
		std::uint64_t resolve_sky()
		{
			const std::uint64_t dm = game::datamodel.address;
			if (!dm) { g_sky = 0; g_lighting = 0; g_datamodel = 0; return 0; }

			if (g_datamodel != dm)
			{
				// A new place means the old Sky is on its way out: put its own
				// strings and numbers back before dropping the address.  This is
				// the best moment available to stop the client freeing a face
				// buffer that came from this process rather than its own heap.
				if (g_applied && g_sky) release_now();

				g_datamodel = dm;
				g_lighting  = 0;
				g_sky       = 0;
			}

			if (g_sky)
			{
				try
				{
					if (rbx::instance_t(g_sky).get_class_name() == "Sky") return g_sky;
				}
				catch (...) {}
				g_sky = 0;
			}

			if (!g_lighting || g_lighting != find_service("Lighting", "Lighting"))
				g_lighting = find_service("Lighting", "Lighting");
			if (!g_lighting) return 0;

			// `Lighting.Sky` is the field the engine itself reads; the child walk
			// stays as the fallback for a client where that field is not the one
			// this offsets table thinks it is.
			std::uint64_t sky = 0;
			try { sky = memory->read<std::uint64_t>(g_lighting + Offsets::Lighting::Sky); }
			catch (...) { sky = 0; }

			if (sky)
			{
				try { if (rbx::instance_t(sky).get_class_name() != "Sky") sky = 0; }
				catch (...) { sky = 0; }
			}

			if (!sky)
			{
				try { sky = rbx::instance_t(g_lighting).find_first_child_by_class("Sky").address; }
				catch (...) { sky = 0; }
			}

			if (sky)
			{
				g_sky = sky;

				std::string nm;
				try { nm = rbx::instance_t(g_sky).get_name(); } catch (...) {}
				debug::ok("skybox: sky found 0x%llx%s%s", (unsigned long long)g_sky,
					nm.empty() ? "" : " name=", nm.c_str());
			}

			return g_sky;
		}

		// ---- what we wrote, so it can be put back ----

		struct field_backup
		{
			bool        have = false;
			std::uint64_t address = 0;
			std::uint32_t bytes = 0;
			unsigned char data[16]{};
		};

		struct text_backup
		{
			bool        have = false;
			std::uint64_t block = 0;   // the std::string, not the Content
			std::string text;
		};

		field_backup g_num[3];      // stars, sun angle, moon angle
		field_backup g_orient;      // SkyboxOrientation, three floats
		field_backup g_celestial;   // CelestialBodiesShown
		text_backup  g_faces[6];    // Bk, Dn, Ft, Lf, Rt, Up
		text_backup  g_tex[2];      // SunTextureId, MoonTextureId

		void backup_field(field_backup& b, std::uint64_t address, std::uint32_t bytes)
		{
			if (b.have || !address || bytes > sizeof b.data) return;

			try
			{
				for (std::uint32_t i = 0; i < bytes; ++i)
					b.data[i] = memory->read<unsigned char>(address + i);
				b.address = address;
				b.bytes   = bytes;
				b.have    = true;
			}
			catch (...) {}
		}

		// `moved` says whether this call is what changed the field: that is what
		// decides whether the client has to be told the lighting moved at all.
		template <typename T>
		bool write_field(field_backup& b, std::uint64_t address, const T& value, bool* moved = nullptr)
		{
			if (moved) *moved = false;

			try
			{
				const T now = memory->read<T>(address);
				if (std::memcmp(&now, &value, sizeof(T)) == 0)
				{
					// Already what it should be.  Backing it up now would be wrong:
					// what is there is ours, not the place's.
					return true;
				}

				backup_field(b, address, (std::uint32_t)sizeof(T));
				memory->write<T>(address, value);
				if (moved) *moved = true;
				return true;
			}
			catch (...) { return false; }
		}

		void restore_field(field_backup& b)
		{
			if (!b.have) return;

			try
			{
				for (std::uint32_t i = 0; i < b.bytes; ++i)
					memory->write<unsigned char>(b.address + i, b.data[i]);
			}
			catch (...) {}

			b.have = false;
		}

		void restore_text(text_backup& b)
		{
			if (!b.have) return;

			// The buffer is the one the field already had, so the original fits by
			// construction: it was in there before we wrote over it.
			write_text(b.block, b.text.c_str());

			b.have = false;
			b.text.clear();
		}

		// ---- one preset, field by field ----

		// One string field that has to change, worked out before anything is
		// written, so the whole set is known before the first write lands.
		struct pending_t
		{
			text_backup*  backup = nullptr;
			std::uint64_t block  = 0;   // the std::string
			std::string   text;
		};

		// The fields of one preset that do not already hold what it asks for.
		// `same` counts the ones that do, so the status line reports the whole
		// preset rather than only the part that moved, and `refused` already
		// counts the fields whose own buffer cannot hold either form.
		int gather_content(const char* const* ids, const std::uintptr_t* offsets, int count,
			text_backup* slots, pending_t* out, int max, int& same, int& refused)
		{
			int n = 0;

			for (int i = 0; i < count && n < max; ++i)
			{
				const std::uint64_t block = g_sky + offsets[i];
				const std::string want = target_text(ids[i], block);
				if (want.empty())
				{
					if (ids[i] && ids[i][0])
						++refused;
					continue;
				}

				const text_read now = read_text(block);

				// Already ours.  Writing it again is a write the client does not
				// need, and this runs five times a second.
				if (now.ok && now.len == want.size() &&
					std::memcmp(now.text, want.c_str(), now.len) == 0)
				{
					++same;
					continue;
				}

				out[n].backup = &slots[i];
				out[n].block  = block;
				out[n].text   = want;
				++n;
			}

			return n;
		}

		int write_content(const pending_t* p, int n, int& written, int& refused)
		{
			int moved = 0;

			for (int i = 0; i < n; ++i)
			{
				text_backup& b = *p[i].backup;

				if (!b.have)
				{
					// The original is read now, before the write: this is the value
					// that goes back on release.  A field that cannot be read is
					// still ours to clear - it goes back as nothing rather than
					// keeping what we put there.
					const text_read now = read_text(p[i].block);

					b.have  = true;
					b.block = p[i].block;
					if (now.ok) b.text.assign(now.text, now.len);
				}

				if (write_text(p[i].block, p[i].text.c_str()))
				{
					++written;
					++moved;
				}
				else ++refused;
			}

			return moved;
		}

		int gather_faces(const sky_presets::preset_t& p, pending_t* out, int max, int& same, int& refused)
		{
			static const std::uintptr_t offsets[6] =
			{
				Offsets::Sky::SkyboxBk, Offsets::Sky::SkyboxDn, Offsets::Sky::SkyboxFt,
				Offsets::Sky::SkyboxLf, Offsets::Sky::SkyboxRt, Offsets::Sky::SkyboxUp,
			};
			const char* ids[6] = { p.bk, p.dn, p.ft, p.lf, p.rt, p.up };

			return gather_content(ids, offsets, 6, g_faces, out, max, same, refused);
		}

		int gather_textures(const sky_presets::preset_t& p, pending_t* out, int max, int& same, int& refused)
		{
			static const std::uintptr_t offsets[2] =
			{
				Offsets::Sky::SunTextureId, Offsets::Sky::MoonTextureId,
			};
			const char* ids[2] = { p.sun, p.moon };

			return gather_content(ids, offsets, 2, g_tex, out, max, same, refused);
		}

		void apply_numbers(const sky_presets::preset_t& p, int& written, int& refused, bool& changed)
		{
			bool moved = false;

			if (p.stars >= 0)
			{
				if (write_field<std::int32_t>(g_num[0], g_sky + Offsets::Sky::StarCount, p.stars, &moved))
				{ ++written; changed |= moved; }
				else ++refused;
			}

			if (p.sun_ang >= 0.f)
			{
				if (write_field<float>(g_num[1], g_sky + Offsets::Sky::SunAngularSize, p.sun_ang, &moved))
				{ ++written; changed |= moved; }
				else ++refused;
			}

			if (p.moon_ang >= 0.f)
			{
				if (write_field<float>(g_num[2], g_sky + Offsets::Sky::MoonAngularSize, p.moon_ang, &moved))
				{ ++written; changed |= moved; }
				else ++refused;
			}
		}

		// The rotation, as the one float at 0x228 that the client actually keeps
		// here: SunAngularSize is at 0x22c, so the twelve-byte write a Vector3
		// would need lands on top of it - the sun's angular size would be the
		// rotation's Y.  One float, backed up as one float, is what the world
		// tab's own sky rows do for the same reason.
		void apply_orientation(bool& changed)
		{
			if (!settings::skybox::orientation_on) return;

			const float v = std::clamp(settings::skybox::orientation[0], -180.f, 180.f);

			try
			{
				const std::uint64_t at = g_sky + Offsets::Sky::SkyboxOrientation;

				if (std::fabs(memory->read<float>(at) - v) < 0.001f) return;

				backup_field(g_orient, at, 4);
				memory->write<float>(at, v);
				changed = true;
			}
			catch (...) {}
		}

		// The rotating sky - the "Rotation" toggle - with that
		// implementation's two faults fixed rather than carried:
		//
		//   * it writes a Vector3 at the orientation field, and SunAngularSize sits
		//     at 0x22c, four bytes behind the rotation float at 0x228.  The twelve
		//     bytes that write needs put the rotation's Y into the sun's angular
		//     size, so turning the sky on shrinks the sun to a speck.  Here the one
		//     float the client actually keeps is the one written, and the sun's own
		//     value is read and put straight back in the same pass.
		//   * its wrap is `rotY = (0.0f > 360.0f) ? 0.0f : rotY + speed` - a
		//     comparison of two literals, false every time, so the angle grows for
		//     as long as the feature is on and the sky starts to step once the float
		//     has no fractional precision left at that magnitude.  The angle here is
		//     wrapped every pass.
		struct rotate_state_t
		{
			float         angle = 0.f;
			std::uint64_t last_ms = 0;
		};

		rotate_state_t g_rotate{};

		void apply_rotate(bool& changed)
		{
			if (!settings::skybox::rotate_on)
			{
				g_rotate.last_ms = 0;
				return;
			}

			const std::uint64_t now = GetTickCount64();
			float dt = g_rotate.last_ms ? (float)(now - g_rotate.last_ms) / 1000.f : 0.f;
			g_rotate.last_ms = now;
			if (dt > 0.25f) dt = 0.25f;   // a hitch is a hitch, not a jump

			const float speed = std::clamp(settings::skybox::rotate_speed, -180.f, 180.f);
			g_rotate.angle += speed * dt;

			// Every pass, not once it is past a literal that is never past anything.
			if (g_rotate.angle >= 360.f || g_rotate.angle <= -360.f)
				g_rotate.angle = std::fmod(g_rotate.angle, 360.f);

			try
			{
				const std::uint64_t at = g_sky + Offsets::Sky::SkyboxOrientation;
				const std::uint64_t sun_at = g_sky + Offsets::Sky::SunAngularSize;

				// Read before the write: whatever is in the sun's own field - the
				// place's value, or the one a preset wrote earlier in this same pass -
				// is what goes back after it.
				const float sun = memory->read<float>(sun_at);

				if (std::fabs(memory->read<float>(at) - g_rotate.angle) < 0.0005f)
					return;

				backup_field(g_orient, at, 4);
				memory->write<float>(at, g_rotate.angle);
				memory->write<float>(sun_at, sun);
				changed = true;
			}
			catch (...) {}
		}

		void apply_celestial(bool& changed)
		{
			const std::uint64_t at = g_sky + Offsets::Sky::CelestialBodiesShown;
			const bool want = !settings::skybox::hide_celestial;

			try
			{
				if ((memory->read<unsigned char>(at) != 0) == want) return;
				backup_field(g_celestial, at, 1);
				memory->write<unsigned char>(at, want ? 1 : 0);
				changed = true;
			}
			catch (...) {}
		}

		// Stars on their own, with no preset behind them.  It writes the same field
		// a preset's star count writes and through the same backup slot - one
		// field, one owner - so the two can be on together without either of them
		// restoring a value the other one made.  The explicit switch wins: a preset
		// is applied first and the stars row overrides its count, which is what a
		// user who ticked both is asking for.
		//
		// `counted_by_preset` keeps the status honest when a preset wrote the field
		// this row owns: one field, one line in the count.
		void apply_stars(int& written, int& refused, bool& changed, bool counted_by_preset)
		{
			if (!settings::skybox::stars_on) return;

			const std::int32_t want =
				(std::int32_t)std::clamp(settings::skybox::stars_count, 0, 5000);

			bool moved = false;
			if (write_field<std::int32_t>(g_num[0], g_sky + Offsets::Sky::StarCount, want, &moved))
			{
				changed |= moved;
				if (moved || !counted_by_preset) ++written;
			}
			else ++refused;
		}

		// One pass over everything the feature owns.  `p` may be null: the star
		// switch, the rotation and the celestial toggle are not properties of a
		// preset, so they apply whether a preset is in play or not.
		void apply(const sky_presets::preset_t* p)
		{
			int written = 0, refused = 0;
			bool changed = false;

			pending_t pending[8];
			int np = 0;

			if (p)
			{
				if (settings::skybox::preset_faces)    np += gather_faces(*p, pending + np, 8 - np, written, refused);
				if (settings::skybox::preset_sun_moon) np += gather_textures(*p, pending + np, 8 - np, written, refused);
			}

			// `changed` is what the client is told about, so it counts writes that
			// actually landed - not fields that were merely planned.  A refused
			// face is not a reason to invalidate the lighting.
			changed |= write_content(pending, np, written, refused) > 0;

			if (p && settings::skybox::preset_stars)
				apply_numbers(*p, written, refused, changed);

			apply_stars(written, refused, changed,
				p && settings::skybox::preset_stars && p->stars >= 0);
			// One field, one writer: a rotating sky and a fixed angle are the same
			// float, so the switch that turns on its own wins while it is on.
			if (settings::skybox::rotate_on) apply_rotate(changed);
			else                             apply_orientation(changed);
			apply_celestial(changed);

			g_written = written;
			g_refused = refused;
			g_applied = true;

			if (p)
				std::snprintf(g_status, sizeof g_status, "%s%s: %d written, %d refused",
					p->name ? p->name : "?",
					settings::skybox::stars_on ? " + stars" : "", written, refused);
			else
				std::snprintf(g_status, sizeof g_status, "stars: %d written, %d refused", written, refused);

			// Only when something actually moved: this used to invalidate the
			// lighting sixteen times a second whether or not the sky had changed.
			//
			// Both flags: the lighting one is what rebuilds the environment the
			// renderer draws from, and the sky one is what stops it drawing the sky
			// it built before this write.  Without the second one the preset lands
			// in the instance and never reaches a frame - which is exactly what
			// "the presets do not apply" looked like.
			if (changed) world_effects::notify_sky_changed();
		}

		void release_now()
		{
			for (int i = 0; i < 3; ++i)  restore_field(g_num[i]);
			for (int i = 0; i < 6; ++i)  restore_text(g_faces[i]);
			for (int i = 0; i < 2; ++i)  restore_text(g_tex[i]);
			restore_field(g_orient);
			restore_field(g_celestial);

			if (g_applied)
			{
				world_effects::notify_sky_changed();
				debug::log("skybox", "restored | %s", g_status);
			}

			g_applied = false;
			g_written = 0;
			g_refused = 0;
			std::snprintf(g_status, sizeof g_status, "off");
		}

		void tick()
		{
			const bool on = settings::skybox::enabled;

			if (!on)
			{
				if (g_applied) release_now();
				return;
			}

			if (!game::datamodel.address) return;

			if (!resolve_sky())
			{
				if (!g_warned_no_sky)
				{
					g_warned_no_sky = true;
					// Warn, not log: this is the one answer that tells the user the preset
					// has nowhere to go, and a line that only reaches the log file is a
					// silent "the sky did not change".
					debug::warn("skybox: no Sky instance under Lighting in this place - a preset "
						"writes into the place's own Sky; making one needs the client's own "
						"constructor, which this build has no path to");
				}
				std::snprintf(g_status, sizeof g_status, "no Sky in this place");
				if (g_applied) release_now();
				return;
			}
			g_warned_no_sky = false;

			const int index = std::clamp(settings::skybox::preset, 0, sky_presets::k_count - 1);
			apply(&sky_presets::k_presets[index]);

			// Say once why a preset came up short, rather than every tick: a field
			// whose own buffer holds neither form is the one refusal a user can do
			// something about (the place has to set that face first).
			if (!g_refused) g_warned_no_room = false;
			else			if (!g_warned_no_room)
			{
				g_warned_no_room = true;
				debug::warn("skybox: %s - %d fields did not fit the buffer the place keeps them in "
					"(set those faces once from a script so the place owns a large enough string)",
					g_status, g_refused);
			}
		}
	}

	void run()
	{
		using namespace std::chrono_literals;

		// A config written before the Skybox switch existed has `preset_on` set
		// and `enabled` at its default: carry it over once so an old config does
		// not silently lose its sky.
		if (settings::skybox::preset_on)
			settings::skybox::enabled = true;

		std::this_thread::sleep_for(600ms);

		auto last_log = sky_clock::now();

		for (;;)
		{
			std::this_thread::sleep_for(60ms);

			try { tick(); }
			catch (...) {}

			// One line every five seconds while it is doing something: which sky it
			// found and how much of the preset the place took.
			const auto now = sky_clock::now();
			if (now - last_log >= std::chrono::seconds(5) && settings::skybox::enabled)
			{
				last_log = now;
				debug::log("skybox", "sky=0x%llx lighting=0x%llx applied=%d %s",
					(unsigned long long)g_sky, (unsigned long long)g_lighting,
					g_applied ? 1 : 0, g_status);
			}
		}
	}

	bool applied()        { return g_applied; }
	int  written()        { return g_written; }
	int  refused()        { return g_refused; }
	const char* status()  { return g_status; }
	std::uint64_t sky_address() { return g_sky; }
	void release()        { release_now(); }
}
