#pragma once

#include <d3d11.h>
#include <windows.h>

namespace backdrop
{
	struct params_t
	{

		float blur  = 45.f;
		float frost = 40.f;
	};

	bool start(ID3D11Device* device, ID3D11DeviceContext* context, HWND overlay_window);
	void stop();
	bool started();

	void set_params(const params_t& params);
	params_t params();

	void set_occlusion(const float* rects_xyxy, int count);

	void set_capture_has_self(bool in_capture);

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
}
