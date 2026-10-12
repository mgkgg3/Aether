#include <cstdint>
#include <chrono>
#include <exception>
#include <thread>

#include <windows.h>
#include <mmsystem.h>
#include <debug/DebugConsole.h>

#include <memory/memory.h>
#include <sdk/offsets.h>
#include <sdk/sdk.h>
#include <game/game.h>
#include <game/rescan/rescan.h>
#include <cache/cache.h>
#include <render/render.h>
#include <features/hitbox_expander/hitbox_expander.h>
#include <features/walkspeed/walkspeed.h>
#include <features/rage/rage.h>
#include <features/rage/rapidfire.h>
#include <features/rage/spin360.h>
#include <features/rage/triggerbot.h>
#include <features/movement/fly.h>
#include <features/movement/speed.h>
#include <features/movement/tickrate.h>
#include <features/movement/noclip.h>
#include <features/movement/jumpcooldown.h>
#include <features/misc/exploits.h>
#include <features/misc/SkyShader.h>
#include <features/aimbot/aimbot.h>
#include <features/silentaim/silentaim.h>
#include <features/worldgeo/WorldGeo.h>
#include <features/raycast/RaycastEngine.h>
#include <features/raycast/RaycastSilent.h>
#include <features/effects/Effects.h>
#include <features/chams/EngineChams.h>
#include <features/world/world_effects.h>
#include <gui/explorer/AetherExplorer.h>
#include <settings.h>

#pragma comment(lib, "winmm.lib")

std::int32_t main()
{
	debug::init_console();
	debug::set_quiet(true);

	{
		static const DWORD main_thread = GetCurrentThreadId();
		std::set_terminate([]
		{
			const DWORD tid = GetCurrentThreadId();
			debug::error("fatal", "unhandled exception on thread %lu - retiring it", (unsigned long)tid);
			if (tid == main_thread)
				std::abort();
			ExitThread(0);
		});
	}

	timeBeginPeriod(1);

	static const char* BINARY_NAME = "RobloxPlayerBeta.exe";

	if (!memory->find_process_id(BINARY_NAME))
	{
		debug::error("Roblox not found");
		for (int left = 8; left > 0; --left)
		{
			debug::status("roblox", "not running - closing in %d s", left);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		return 1;
	}

	if (!memory->attach_to_process(BINARY_NAME))
	{
		debug::error("unable to attach to Roblox!");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	if (!memory->find_module_address(BINARY_NAME))
	{
		debug::error("unable to find main module address!");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	debug::status("roblox", "attached (pid %u)", memory->get_process_id());
	debug::status("client", "%s", Offsets::ClientVersion.c_str());

	{
		char image[MAX_PATH]{};
		DWORD len = MAX_PATH;
		if (QueryFullProcessImageNameA(memory->get_process_handle(), 0, image, &len))
		{
			std::string path(image);
			const std::size_t at = path.find("version-");
			std::string running = (at == std::string::npos) ? std::string()
				: path.substr(at, path.find_first_of("\\/", at) - at);

			debug::info("client version -> %s", Offsets::ClientVersion.c_str());
			if (running.empty())
				debug::info("running client  -> unknown (no version folder in the image path)");
			else if (running == Offsets::ClientVersion)
				debug::ok("running client  -> %s (matches the offsets table)", running.c_str());
			else
				debug::info("running client  -> %s (offsets table is %s)",
					running.c_str(), Offsets::ClientVersion.c_str());
		}
		else
		{
			debug::info("client version -> %s", Offsets::ClientVersion.c_str());
		}
	}

	std::uint64_t fake_datamodel{ memory->read<std::uint64_t>(
		memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
	game::datamodel = rbx::instance_t(memory->read<std::uint64_t>(
		fake_datamodel + Offsets::FakeDataModel::RealDataModel));
	debug::info("datamodel -> 0x%llx", game::datamodel.address);

	{
		const std::uint64_t place = game::datamodel.address
			? memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::PlaceId)
			: 0ull;
		const bool supported = (place == 17625359962ull);
		debug::status("place", "%llu%s", (unsigned long long)place,
			supported ? " (Rivals)" : "");
		if (place && !supported)
			debug::warn("unsupported place - features may not work as intended");
	}

	game::visengine = { memory->read<std::uint64_t>(
		memory->get_module_address() + Offsets::VisualEngine::Pointer) };
	debug::info("visengine -> 0x%llx", game::visengine.address);

	game::workspace = { game::datamodel.find_first_child_by_class("Workspace") };
	debug::info("workspace -> 0x%llx", game::workspace.address);

	game::players = { game::datamodel.find_first_child_by_class("Players") };
	debug::info("players -> 0x%llx", game::players.address);

	game::local_player = { memory->read<std::uint64_t>(
		game::players.address + Offsets::Player::LocalPlayer) };
	debug::info("local_player -> 0x%llx", game::local_player.address);

	game::wnd = FindWindowA(nullptr, "Roblox");
	debug::info("roblox window -> 0x%p", (void*)game::wnd);

	debug::section("name method check (NameContainer)");
	{
		std::string dm_name = game::datamodel.get_name();
		std::string dm_class = game::datamodel.get_class_name();
		if (dm_class == "DataModel" && dm_name != "invalid_str" && !dm_name.empty())
			debug::ok("datamodel name/class -> '%s' / '%s'", dm_name.c_str(), dm_class.c_str());
		else
			debug::warn("datamodel name/class -> '%s' / '%s' (offsets may be stale)", dm_name.c_str(), dm_class.c_str());

		rbx::instance_t ws_check{ game::workspace.address };
		std::string ws_name = ws_check.get_name();
		std::string ws_cls = ws_check.get_class_name();
		debug::info("workspace instance -> '%s' / '%s'", ws_name.c_str(), ws_cls.c_str());
	}

	if (!InitializeStorage())
	{
		debug::warn("[rescan] InitializeStorage failed - continuing anyway");
	}

	debug::section("feature threads");

	std::thread(cache::run).detach();            debug::ok("cache thread running");
	std::thread(rbx::hitbox_expander::run).detach(); debug::ok("hitbox_expander thread running");
	std::thread(walkspeed::run).detach();        debug::ok("walkspeed thread running");
	rage::start_threads();                       debug::ok("rage threads started");
	std::thread(rage::rapidfire::run).detach();  debug::ok("rapidfire thread running");
	std::thread(rage::spin360::run).detach();    debug::ok("spin360 thread running");
	std::thread(rage::triggerbot::run).detach(); debug::ok("triggerbot thread running");
	std::thread(movement::fly::run).detach();    debug::ok("fly thread running");
	std::thread(movement::speed::run).detach();  debug::ok("speed thread running");
	movement::tickrate::run();                   debug::ok("tickrate applied");
	std::thread(movement::noclip::run).detach(); debug::ok("noclip thread running");
	std::thread(movement::jumpcooldown::run).detach(); debug::ok("jumpcooldown thread running");
	std::thread(exploits::run).detach();         debug::ok("exploits thread running");
	std::thread(aimbot::run).detach();           debug::ok("aimbot thread running");
	std::thread(world_effects::run).detach();    debug::ok("world_effects thread running");
	std::thread(effects::run).detach();          debug::ok("effects thread running (hit feedback)");

	engine_chams::start();                       debug::ok("engine chams thread running");
	std::thread(skyshader::run).detach();        debug::ok("skyshader thread running");

	std::thread(AutoRescanHandler).detach();     debug::ok("auto-rescan thread running");

	silentaim::run();                            debug::ok("silentaim active");
	aether_raycast::start();                       debug::ok("raycast engine started (LOS / radar)");
	worldgeo::start();                            debug::ok("world-geometry cache thread started (visible checks)");

	debug::section("render");

	if (!render->create_window())
	{
		debug::error("failed to create window");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}
	debug::ok("overlay window created");

	if (!render->create_device())
	{
		debug::error("failed to create device");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}
	debug::ok("D3D11 device created");

	debug::ok("workspace explorer ready");

	debug::section("main loop");

	debug::log("render", "frame loop thread tid %lu", (unsigned long)GetCurrentThreadId());
	while (true)
	{
		try
		{
			if (settings::settings::should_unload)
			{
				debug::info("unload requested, shutting down...");
				StopAutoRescan();
				aimbot::release_camera();
				aether_explorer::shutdown();
				raycast_silent::ensure(false);
				aether_raycast::stop();

				break;
			}

			render->start_render();
			render->render_menu();
			render->end_render();
		}
		catch (...)
		{

			static auto s_frame_err_at = std::chrono::steady_clock::now();
			auto fnow = std::chrono::steady_clock::now();
			if (fnow - s_frame_err_at >= std::chrono::seconds(10))
			{
				s_frame_err_at = fnow;
				debug::warn("main", "render frame threw, recovering");
			}
		}
	}

	debug::info("bye");
	return 0;
}
