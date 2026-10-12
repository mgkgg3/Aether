#include "Spotify.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <mmsystem.h>
#include <ShlObj.h>
#include <filesystem>
#include <mutex>
#include <algorithm>

#include <debug/DebugConsole.h>

#pragma comment(lib, "winmm.lib")

namespace aether_spotify
{
	static std::mutex s_mtx;
	static bool       s_paused = false;

	std::vector<Track>& tracks()
	{
		static std::vector<Track> t;
		return t;
	}

	void refresh_tracks()
	{
		auto& list = tracks();
		list.clear();

		char base[MAX_PATH];
		if (FAILED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, base)))
			return;

		std::string dir = std::string(base) + "\\Aether\\music\\";
		std::error_code ec;
		std::filesystem::create_directories(dir, ec);

		for (auto& e : std::filesystem::directory_iterator(dir, ec))
		{
			std::string ext = e.path().extension().string();
			for (char& c : ext) c = (char)tolower((unsigned char)c);
			if (ext != ".mp3" && ext != ".wav" && ext != ".ogg" &&
				ext != ".flac" && ext != ".wma" && ext != ".m4a")
				continue;

			list.push_back({ e.path().wstring(), e.path().stem().string() });
		}

		std::sort(list.begin(), list.end(),
			[](const Track& a, const Track& b) { return a.name < b.name; });

		debug::log("spotify", "%zu track(s) in %s", list.size(), dir.c_str());
	}

	static std::wstring current_alias_path()
	{
		auto& list = tracks();
		if (cfg::selected < 0 || cfg::selected >= (int)list.size())
			return {};
		return list[cfg::selected].path;
	}

	void play(int index)
	{
		std::lock_guard<std::mutex> lk(s_mtx);

		auto& list = tracks();
		if (index >= 0 && index < (int)list.size())
			cfg::selected = index;

		if (cfg::selected < 0 && !list.empty())
			cfg::selected = 0;

		std::wstring path = current_alias_path();
		if (path.empty())
		{
			debug::warn("spotify: no track selected");
			return;
		}

		mciSendStringW(L"close aethermusic", nullptr, 0, nullptr);

		std::wstring cmd = L"open \"" + path + L"\" type mpegvideo alias aethermusic";
		MCIERROR err = mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
		if (err)
		{
			wchar_t buf[128];
			mciGetErrorStringW(err, buf, 128);

			cmd = L"open \"" + path + L"\" alias aethermusic";
			err = mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
			if (err)
			{
				mciGetErrorStringW(err, buf, 128);
				char u8[256];
				WideCharToMultiByte(CP_UTF8, 0, buf, -1, u8, sizeof u8, nullptr, nullptr);
				debug::error("spotify: open failed (%s). Try re-encoding to plain MP3/WAV.", u8);
				return;
			}
		}

		err = mciSendStringW(L"play aethermusic", nullptr, 0, nullptr);
		if (err)
		{
			wchar_t buf[128];
			mciGetErrorStringW(err, buf, 128);
			debug::error("spotify: play failed: %ls", buf);
			return;
		}

		s_paused = false;

		wchar_t mode[32] = L"?";
		mciSendStringW(L"status aethermusic mode", mode, 32, nullptr);

				char u8mode[64]; WideCharToMultiByte(CP_UTF8, 0, mode, -1, u8mode, sizeof u8mode, nullptr, nullptr);
		debug::log("spotify", "play '%s' -> device mode: %s",
			list[cfg::selected].name.c_str(), u8mode);

		char vcmd[64];
		snprintf(vcmd, sizeof vcmd, "setaudio aethermusic volume to %d",
			(int)(aether_spotify::cfg::volume * 1000.f));
		mciSendStringA(vcmd, nullptr, 0, nullptr);
	}

	void pause()
	{
		std::lock_guard<std::mutex> lk(s_mtx);
		if (s_paused) return;
		mciSendStringW(L"pause aethermusic", nullptr, 0, nullptr);
		s_paused = true;
	}

	void stop()
	{
		std::lock_guard<std::mutex> lk(s_mtx);
		mciSendStringW(L"close aethermusic", nullptr, 0, nullptr);
		cfg::selected = -1;
		s_paused = false;
	}

	void set_volume(float v)
	{
		cfg::volume = v;

		char cmd[64];
		snprintf(cmd, sizeof cmd, "setaudio aethermusic volume to %d", (int)(v * 1000.f));
		mciSendStringA(cmd, nullptr, 0, nullptr);
	}

	bool is_playing()
	{
		return cfg::selected >= 0 && !s_paused;
	}

	int current_index() { return cfg::selected; }

	void tick(bool menu_open)
	{

		if (!cfg::menu_music || cfg::background)
			return;

		static bool s_was_menu_open = false;

		if (menu_open && !s_was_menu_open && s_paused && cfg::selected >= 0)
			play(-1);

		if (!menu_open && s_was_menu_open && is_playing())
			pause();

		s_was_menu_open = menu_open;
	}
}
