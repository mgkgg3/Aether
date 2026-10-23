#pragma once

namespace engine_chams
{

	void start();

	void stop();

	void restore_all();

	bool active();

	// The style list lives with the feature that measured it, in
	// features/chams/EngineChams.cpp: a style is a shader program family the
	// client actually has, and which families can take which pass is a count
	// taken from the live client (tools/chams_swap_probe.py,
	// tools/scene_route_probe.py), not a taste.  The menu reads the list from
	// here so there is one list rather than a copy in every page.
	int style_count();

	const char* style_name(int style);

	// The family a style swaps to, or nullptr for the as-is style.
	const char* family_name(int style);

	// How many passes of the scene the last pass over it could have given this
	// style's family, and how many it actually swapped.  This is what lets the
	// menu say a style reached nothing on this place instead of the style
	// looking broken.
	int style_reach(int style);
	int style_swapped(int style);

	// Programs read from the client's shader manager; 0 until it has been read.
	int programs_loaded();

	// Scene cluster managers the feature is walking; 0 until they are resolved.
	int routes_loaded();

	// What the last pass over the scene wrote.  This exists for the rig
	// (tmp/chams_rig.cpp), which runs the real feature against a real client and
	// has to be able to say what it did; the overlay reads only the first two
	// counters, and only to print them in the menu.
	struct stats_t
	{
		int programs;         // programs in the client's shader manager
		int families;         // families the manager offered a pass for
		int routes;           // scene cluster managers being walked
		int entities;         // render clusters reached this pass
		int checked;          // technique blocks the pass looked at
		int skipped;          // of those, already in the state this feature leaves
		int tinted;           // blocks whose material fields were written
		int swapped;          // of those, blocks whose program was replaced
		int unknown;          // blocks whose program is not a render pass of the manager
		int no_target;        // passes the style had no program for
		int not_ready;        // passes whose candidate program the engine is not ready for
		unsigned long long sample_block;    // first swapped block
		unsigned long long sample_program;  // the program written into it
	};

	stats_t stats();
}
