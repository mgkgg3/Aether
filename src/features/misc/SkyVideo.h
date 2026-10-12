#pragma once

#include <string>
#include <vector>

namespace SkyVideo
{

	inline constexpr int k_w = 320;
	inline constexpr int k_h = 180;

	void Ensure();

	std::vector<std::string> List();

	void Play(const std::string& name);

	bool Frame(const void** px, int* w, int* h);
}
