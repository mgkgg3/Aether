#pragma once
#include <memory>

#include <d3d11.h>
#include <Windows.h>

#include <gui/CGui.h>

struct detail_t {
	HWND window = nullptr;
	WNDCLASSEX window_class = {};
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* device_context = nullptr;
	ID3D11RenderTargetView* render_target_view = nullptr;
	IDXGISwapChain* swap_chain = nullptr;
};

class render_t {
public:
	render_t();
	~render_t();

	bool running = true;

	void start_render();
	void render_menu();
	void render_visuals();
	void end_render();

	bool create_device();
	bool create_window();

















	void trace(const char* phase, double ms);

	int get_ping_ms();

	std::unique_ptr<detail_t> detail = std::make_unique<detail_t>();
	std::unique_ptr<CGui> gui = std::make_unique<CGui>();

private:
	void destroy_device();
	void destroy_window();
};

inline std::unique_ptr<render_t> render = std::make_unique<render_t>();
