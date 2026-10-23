#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d11.h>
#include <imgui/imgui.h>

// The interface's light, as textures instead of geometry.
//
// Every soft shape the panel drew - the shafts over the lip, the glow behind
// the cursor, the drifting aurora, the highlight on a particle - used to be a
// triangle fan whose alpha fell from the centre vertex to a transparent rim.
// That is a real gradient, but it is a gradient with two problems a screen can
// see: the rim is a polygon, so a big fan has straight edges, and alpha is
// quantised to eight bits at the vertex, so a long falloff arrives in visible
// rings.  Over the frost, at low alpha, that is the "pixely" look.
//
// These textures are the same gradients sampled per pixel instead: smoothstep
// falloffs, an ordered dither baked into the alpha so eight-bit banding breaks
// up into grain instead of rings, and a linear-filtered upscale that no vertex
// count can outrun.  They are built once, on the device, and never change, so a
// glow costs one quad rather than forty triangles.
//
// The contour maps are the same idea for the topography background: a tileable
// height field reduced to anti-aliased iso-lines at build time, so the effect is
// one textured quad per layer per frame instead of thousands of line segments.
namespace surface
{
	bool init(ID3D11Device* device);
	void shutdown();
	bool ready();

	// White radial falloff: opaque at the centre, clear at the rim.  Stretch it
	// into an ellipse and a light shaft is the same texture.
	ImTextureID radial();

	// A soft band: a bell across its width and a bell down its length, so a
	// ribbon has no top edge, no bottom edge and no seam where it ends.
	ImTextureID ribbon();

	// Tileable iso-line maps at two spacings.  Both wrap cleanly, so a layer can
	// be scrolled by moving its UVs and nothing at the seam moves.
	ImTextureID topo_fine();
	ImTextureID topo_coarse();
}
