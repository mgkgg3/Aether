#pragma once

#include <imgui/imgui.h>

namespace sync
{

	bool init();

	void set_hwnd(void* hwnd);

	bool open();
	void set_open(bool value);

	float fade();

	void draw(ImVec2 display_size);

	struct Cost
	{
		double poll   = 0.0;
		double panel  = 0.0;
		double page   = 0.0;
		double xform  = 0.0;
		double card   = 0.0;
		double count  = 0.0;
		double docks  = 0.0;
		bool   open   = false;
	};

	Cost cost();

	double cost_take_worst_panel();

	struct Material
	{
		ImU32 surface     = 0;
		ImU32 card        = 0;
		ImU32 well        = 0;
		ImU32 hairline    = 0;
		ImU32 rule        = 0;
		ImU32 text        = 0;
		ImU32 textDim     = 0;
		ImU32 textFaint   = 0;
		ImU32 accent      = 0;
		float roundWindow = 0.f;
		float roundControl = 0.f;
	};

	Material material();
}
