#pragma once

#include <d3d11.h>
#include <windows.h>

// The panel's frosted glass.  One desktop capture, down-scaled hard, blurred
// small and published as a texture the menu samples with bilinear filtering -
// the upscale is the frost.  Everything here is called from the render thread
// only, and an image that has been produced is never taken away while the
// module runs, so the glass cannot flicker back to nothing.
namespace backdrop
{
	struct params_t
	{
		// The blur is the whole material now.  "Frost" used to be a second pass -
		// a noise/grain veil over the small image - and it was the most expensive
		// thing the backdrop did: a full-image pass plus its own buffer, paid on
		// every captured frame, for a texture nobody could see under the tint.
		// Removing it is what took the lag out from behind the blur.
		float blur  = 30.f;

		// Darkens the captured image itself, so the panel's own fill does not
		// have to carry all of the contrast.  0 is off, 100 is at its darkest.
		float tint  = 0.f;

		// The most of the output the capture may copy, as a percentage of its
		// area.  0 turns the capture off: `wanted()` answers false, so the
		// caller never starts it and nothing on the desktop is duplicated.
		float capture_cap = 100.f;
	};

	bool start(ID3D11Device* device, ID3D11DeviceContext* context, HWND overlay_window);
	void stop();
	bool started();

	// False when the blur is at 0 (and the cap is off): there is nothing to
	// capture, so the caller should not pay for the duplication at all.
	bool wanted();

	void set_params(const params_t& params);
	params_t params();

	// Restrict the capture to a rectangle in *client* pixels: the panel and the
	// docks, nothing else.  Pixels outside it are never copied, mapped or
	// blurred, which is what makes a light blur cheap.  An empty rectangle (or
	// one covering most of the display) goes back to the full desktop.
	void set_region(float x0, float y0, float x1, float y1);

	bool update();
	bool ready();

	void* image();

	float image_w();
	float image_h();
	float origin_x();
	float origin_y();
	float image_scale();

	const char* status();
	const char* status_key();

	// CPU time of the last frame's copy + blur + veil, in milliseconds.
	float cost_ms();

	// How old the published image is, in milliseconds (-1 before the first one).
	// This is the number behind "the blur lags behind the panel".
	float image_age_ms();

	// CPU time of the last readback issue, whatever path it took.
	float readback_ms();

	// The read path is split across two ticks: the down-scale pass and its
	// staging copy are issued with the capture, and the map collects them on the
	// next frame, where they have landed.  Returns false only on the CPU fallback
	// path, which reads the block in the tick that copied it, and stalls doing so.
	bool  split_readback();
}
