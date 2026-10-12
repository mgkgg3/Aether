#pragma once

#include <string>
#include <vector>

namespace aether_spotify
{
	struct Track
	{
		std::wstring path;
		std::string  name;
	};

	std::vector<Track>& tracks();

	void refresh_tracks();

	void play(int index);
	void pause();
	void stop();
	void set_volume(float v);
	void tick(bool menu_open);

	bool is_playing();
	int  current_index();

	namespace cfg
	{
		inline bool  menu_music   = false;
		inline bool  background   = false;
		inline float volume       = 0.4f;
		inline int    selected     = -1;
	}
}
