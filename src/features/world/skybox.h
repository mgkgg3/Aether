#pragma once

#include <cstdint>

// The skybox presets: the two catalogs in `SkyPresets.h` applied to the place's
// own Sky instance.
//
// This is the one part of the world tab that is not an effect - it does not draw
// anything, it writes what the client draws its sky from, and the client is what
// renders it.  So it is a small state machine over three kinds of write:
//
//   * the three celestial numbers (StarCount, SunAngularSize, MoonAngularSize),
//     the Sky's rotation and whether it shows the sun and moon at all - plain
//     fields at the offsets `Offsets::Sky` names, so they always land.  StarCount
//     is also on its own row: "stars at night" is a thing a user wants while
//     keeping the place's own sky, and it is the same write;
//   * the six face ContentIds and the sun and moon textures - strings, and the
//     one part of this that can be refused (see the note in the .cpp);
//   * the restore, which puts back exactly what was there, per field, so a
//     preset applied over a place's own sky can be taken off again.
//
// A preset is applied while the row is on and taken off when it is not, so the
// feature has no "apply once" state to get stuck in.  Everything the feature
// owns is released when both rows are off, and the fields each row writes are
// backed up once, so the stars row and a preset's own star count never fight
// over the same value.

namespace skybox
{
	// The thread.  Started from main.cpp like every other feature's.
	void run();

	// True while a preset is applied and the values are ours to put back.
	bool applied();

	// How many fields of the last apply landed, and how many were refused.  The
	// menu shows them because "the sky did not change" has two very different
	// causes and this is the only place that knows which one happened.
	int  written();
	int  refused();

	// One line describing the last apply, for the log.
	const char* status();

	// The Sky instance the writes go to, 0 when the place has none yet.
	std::uint64_t sky_address();

	// Take everything back now, whatever the switch says: the unload path and the
	// reset button both need it.
	void release();
}
