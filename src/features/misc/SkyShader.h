#pragma once

#include <cstdint>

namespace skyshader
{

	// Entry `slot` of the client's own device (context=false) or immediate context
	// (context=true) vtable, borrowed from a device created locally and remapped
	// onto the client's copy of d3d11.dll. Zero when it cannot be worked out.
	std::uint64_t remote_d3d_fn(bool context, int slot);


	void Tick();

	void Stop();

	void Dump();

	void run();
}
