
#include "AetherExplorer.h"
#include "AetherIcons.h"
#include "../AetherWidgets.h"
#include "../menu/AetherMenu.h"
#include "../menu/SyncMenu.h"
#include <imgui/addons/imgui_addons.h>

#include <features/explorer/Explorer.h>
#include <debug/DebugConsole.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>
#include <game/game_context.h>
#include <game/game_structure.h>
#include <settings.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <cctype>
#include <cstdlib>
#include <cfloat>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <ShlObj.h>
#include <functional>
#include <filesystem>

namespace
{
	using namespace aether;

	ImVec4 border_outer(0.13f, 0.13f, 0.13f, 1.f);
	ImVec4 border_inner(0.18f, 0.18f, 0.18f, 1.f);
	ImVec4 glass_title (0.078f, 0.078f, 0.086f, 0.85f);
	ImVec4 glass_body  (0.055f, 0.055f, 0.063f, 0.78f);
	ImVec4 row_accent  (1.f, 1.f, 1.f, 0.35f);
	ImVec4 row_text    (0.85f, 0.85f, 0.88f, 1.f);
	ImVec4 sep_col     (1.f, 1.f, 1.f, 0.10f);
	ImVec4 header_rule (1.f, 1.f, 1.f, 0.30f);
	ImVec4 btn_face    (0.f, 0.f, 0.f, 0.f);

	inline ImVec4 u32_to_vec4(ImU32 c)
	{
		return ImVec4(
			(float)((c >> IM_COL32_R_SHIFT) & 0xFF) / 255.f,
			(float)((c >> IM_COL32_G_SHIFT) & 0xFF) / 255.f,
			(float)((c >> IM_COL32_B_SHIFT) & 0xFF) / 255.f,
			(float)((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.f);
	}

	inline void refresh_glass_colors()
	{
		const sync::Material m = sync::material();

		border_outer = ImVec4(1.f, 1.f, 1.f, 0.07f);
		border_inner = u32_to_vec4(m.hairline);

		glass_body  = u32_to_vec4(m.surface);

		glass_title = glass_body;
		glass_title.x = ImMin(1.f, glass_title.x + 0.012f);
		glass_title.y = ImMin(1.f, glass_title.y + 0.012f);
		glass_title.z = ImMin(1.f, glass_title.z + 0.016f);

		row_accent  = ImVec4(1.f, 1.f, 1.f, 0.14f);
		row_text    = u32_to_vec4(m.text);
		sep_col     = u32_to_vec4(m.hairline);
		header_rule = u32_to_vec4(m.rule);

		btn_face = ImVec4(1.f, 1.f, 1.f, 0.06f);
	}

	struct ScopedWidgetStyle
	{
		ScopedWidgetStyle()
		{
			const sync::Material m = sync::material();

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   8.f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding,    8.f);
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,  10.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 8.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 5.f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(8.f, 6.f));
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 9.f);

			ImGui::PushStyleColor(ImGuiCol_FrameBg,            ImVec4(1.f, 1.f, 1.f, 0.05f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,     ImVec4(1.f, 1.f, 1.f, 0.08f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive,      ImVec4(1.f, 1.f, 1.f, 0.11f));
			ImGui::PushStyleColor(ImGuiCol_Border,             u32_to_vec4(m.hairline));
			ImGui::PushStyleColor(ImGuiCol_Header,             ImVec4(1.f, 1.f, 1.f, 0.06f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered,      ImVec4(1.f, 1.f, 1.f, 0.09f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive,       ImVec4(1.f, 1.f, 1.f, 0.12f));
			ImGui::PushStyleColor(ImGuiCol_Button,             ImVec4(1.f, 1.f, 1.f, 0.06f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,      ImVec4(1.f, 1.f, 1.f, 0.10f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,       ImVec4(1.f, 1.f, 1.f, 0.14f));
			ImGui::PushStyleColor(ImGuiCol_CheckMark,          u32_to_vec4(m.text));
			ImGui::PushStyleColor(ImGuiCol_Text,               u32_to_vec4(m.text));
			ImGui::PushStyleColor(ImGuiCol_TextDisabled,       ImVec4(1.f, 1.f, 1.f, 0.35f));
			ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,     ImVec4(1.f, 1.f, 1.f, 0.16f));
			ImGui::PushStyleColor(ImGuiCol_Separator,          u32_to_vec4(m.hairline));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,        ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,      ImVec4(1.f, 1.f, 1.f, 0.14f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(1.f, 1.f, 1.f, 0.22f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImVec4(1.f, 1.f, 1.f, 0.30f));
			ImGui::PushStyleColor(ImGuiCol_PopupBg,            u32_to_vec4(m.surface));
		}

		~ScopedWidgetStyle()
		{
			ImGui::PopStyleColor(20);
			ImGui::PopStyleVar(9);
		}
	};

	struct ctx_target
	{
		std::uint64_t address = 0;
		std::string name;
		std::string cls;
		std::string path;
		int children = 0;
		bool valid = false;
	};

	struct prop_target
	{
		std::string name;
		std::string cls;
		std::string path;
		std::uint64_t address = 0;
		int children = 0;
	};

	void fill_prop(prop_target& out, std::uint64_t addr, const std::string& name, const std::string& cls, const std::string& path, int children)
	{
		out.name = name;
		out.cls = cls;
		out.path = path;
		out.address = addr;
		out.children = children;
		if (addr && children <= 0)
		{
			rbx::instance_t inst(addr);
			out.children = (int)inst.get_children().size();
		}
	}

	void open_ctx(ctx_target& ctx, std::uint64_t addr, const std::string& name, const std::string& cls, const std::string& path, ImVec2& ctx_pos)
	{
		ctx.address = addr;
		ctx.name = name;
		ctx.cls = cls;
		ctx.path = path;
		ctx.children = 0;
		if (addr)
		{
			rbx::instance_t inst(addr);
			ctx.children = (int)inst.get_children().size();
		}
		ctx.valid = true;
		ctx_pos = ImGui::GetMousePos();
		ImGui::OpenPopup("##ex_ctx_menu");
	}

	void select_node(std::uint64_t& selected_addr, prop_target& prop, bool& properties_open,
		std::uint64_t addr, const std::string& name, const std::string& cls, const std::string& path, int children)
	{
		selected_addr = addr;
		fill_prop(prop, addr, name, cls, path, children);
		properties_open = true;
	}

	void render_node(Node& node, int depth, std::uint64_t& selected_addr, ctx_target& ctx, ImVec2& ctx_pos,
		prop_target& prop, bool& properties_open)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		constexpr float row_h = 22.f;
		constexpr float indent = 15.f;
		constexpr float icon_size = 14.f;

		bool has_children = node.loaded ? !node.children.empty() : node.has_kids;
		bool selected = (selected_addr == node.address);

		ImVec2 row_pos = ImGui::GetCursorScreenPos();
		float avail_w = ImGui::GetContentRegionAvail().x;
		ImVec2 row_max(row_pos.x + avail_w, row_pos.y + row_h);
		float arrow_x = row_pos.x + depth * indent + 2.f;
		float arrow_max_x = arrow_x + indent + 4.f;

		ImGui::PushID((void*)(uintptr_t)node.address);
		ImGui::Selectable("##row", selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(avail_w, row_h));
		bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
		bool hovered = ImGui::IsItemHovered();
		bool dbl = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered;
		ImGui::PopID();

		if (clicked)
		{
			float mx = ImGui::GetIO().MousePos.x;
			bool on_arrow = has_children && mx >= arrow_x && mx < arrow_max_x;
			if (on_arrow)
			{
				node.open = !node.open;
				if (node.open)
					load_children(node);
				selected_addr = node.address;
			}
			else
			{
				select_node(selected_addr, prop, properties_open, node.address, node.name, node.cls,
					build_path(node.address), node.loaded ? (int)node.children.size() : -1);
				if (dbl && has_children)
				{
					node.open = !node.open;
					if (node.open)
						load_children(node);
				}
			}
		}
		if (right_clicked)
		{
			selected_addr = node.address;
			open_ctx(ctx, node.address, node.name, node.cls, build_path(node.address), ctx_pos);
		}

		if (hovered && !selected)
		{
			ImVec4 hov = row_text;
			hov.w = 0.07f;
			dl->AddRectFilled(row_pos, row_max, ImGui::GetColorU32(hov), 4.f);
		}
		if (selected)
		{

			ImVec4 sel = row_accent;
			sel.w = 0.15f;
			dl->AddRectFilled(row_pos, row_max, ImGui::GetColorU32(sel), 4.f);
			dl->AddRectFilled(ImVec2(row_pos.x + 1.f, row_pos.y + 3.f),
				ImVec2(row_pos.x + 3.f, row_max.y - 3.f), ImGui::GetColorU32(row_accent), 1.f);
		}

		if (has_children)
			ImGui::RenderArrow(dl, ImVec2(arrow_x, row_pos.y + row_h * 0.5f - 4.f),
				ImGui::GetColorU32(ImVec4(row_text.x, row_text.y, row_text.z, 0.55f)),
				node.open ? ImGuiDir_Down : ImGuiDir_Right, 0.8f);

		float icon_x = row_pos.x + depth * indent + indent + 4.f;
		aether_icons::draw_icon(dl, node.cls, ImVec2(icon_x, row_pos.y + (row_h - icon_size) * 0.5f), icon_size);

		float text_x = icon_x + icon_size + 5.f;
		float text_y = row_pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f;

		const ImVec4 tc = selected ? ImVec4(1.f, 1.f, 1.f, 1.f) : row_text;
		ImU32 text_col = ImGui::GetColorU32(tc);
		const char* label = node.name.empty() ? "?" : node.name.c_str();
		dl->AddText(ImVec2(text_x, text_y), text_col, label);

		if (value_base::is_class(node.cls))
		{
			std::string val = value_base::format(node.address, node.cls);
			if (!val.empty())
			{
				float lx = text_x + ImGui::CalcTextSize(label).x + 6.f;
				std::string suffix = "= " + val;
				widgets::text_outlined(dl, ImVec2(lx, text_y), IM_COL32(120, 160, 120, 220), suffix.c_str());
			}
		}

		ImGui::SetCursorScreenPos(ImVec2(row_pos.x, row_pos.y + row_h));

		if (node.open && node.loaded)
		{

			constexpr int k_max_depth = 40;
			int shown = (int)node.children.size();
			if (shown > 800) shown = 800;
			if (depth + 1 <= k_max_depth)
			{
				for (int i = 0; i < shown; ++i)
					render_node(node.children[i], depth + 1, selected_addr, ctx, ctx_pos, prop, properties_open);
			}
			if ((int)node.children.size() > shown || depth + 1 > k_max_depth)
			{
				ImGui::SetCursorPosX(depth * indent + 28.f);
				if (depth + 1 > k_max_depth)
					ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.f), "... tree too deep (use search)");
				else
					ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.f), "... %d more (use search)", (int)node.children.size() - shown);
			}
		}
	}

	void render_search_row(const SearchResult& r, int idx, std::uint64_t& selected_addr, ctx_target& ctx, ImVec2& ctx_pos,
		prop_target& prop, bool& properties_open)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		constexpr float row_h = 22.f;
		constexpr float icon_size = 14.f;

		bool selected = (selected_addr == r.address);
		ImVec2 row_pos = ImGui::GetCursorScreenPos();
		float avail_w = ImGui::GetContentRegionAvail().x;
		ImVec2 row_max(row_pos.x + avail_w, row_pos.y + row_h);

		ImGui::PushID(idx);
		bool clicked = ImGui::Selectable("##srow", selected, 0, ImVec2(avail_w, row_h));
		bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
		bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		if (clicked)
			select_node(selected_addr, prop, properties_open, r.address, r.name, r.cls, r.path, -1);
		if (right_clicked)
		{
			selected_addr = r.address;
			open_ctx(ctx, r.address, r.name, r.cls, r.path, ctx_pos);
		}

		if (hovered && !selected)
		{
			ImVec4 hov = row_text;
			hov.w = 0.07f;
			dl->AddRectFilled(row_pos, row_max, ImGui::GetColorU32(hov), 4.f);
		}
		if (selected)
		{
			ImVec4 sel = row_accent;
			sel.w = 0.15f;
			dl->AddRectFilled(row_pos, row_max, ImGui::GetColorU32(sel), 4.f);
			dl->AddRectFilled(ImVec2(row_pos.x + 1.f, row_pos.y + 3.f),
				ImVec2(row_pos.x + 3.f, row_max.y - 3.f), ImGui::GetColorU32(row_accent), 1.f);
		}

		float icon_x = row_pos.x + 8.f;
		aether_icons::draw_icon(dl, r.cls, ImVec2(icon_x, row_pos.y + (row_h - icon_size) * 0.5f), icon_size);

		float text_x = icon_x + icon_size + 5.f;
		float text_y = row_pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f;
		const ImVec4 tc = selected ? ImVec4(1.f, 1.f, 1.f, 1.f) : row_text;
		ImU32 text_col = ImGui::GetColorU32(tc);
		const char* label = r.name.empty() ? "?" : r.name.c_str();
		dl->AddText(ImVec2(text_x, text_y), text_col, label);

		if (value_base::is_class(r.cls))
		{
			std::string val = value_base::format(r.address, r.cls);
			if (!val.empty())
			{
				float lx = text_x + ImGui::CalcTextSize(label).x + 6.f;
				std::string suffix = "= " + val;
				widgets::text_outlined(dl, ImVec2(lx, text_y), IM_COL32(120, 160, 120, 220), suffix.c_str());
			}
		}

		ImGui::SetCursorScreenPos(ImVec2(row_pos.x, row_pos.y + row_h));
	}

	void info_row(ImDrawList* dl, ImVec2 pos, float w, float h, const char* label, const std::string& value, bool sep)
	{
		widgets::text_outlined(dl, ImVec2(pos.x + 10.f, pos.y + (h - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(140, 140, 140, 255), label);
		widgets::text_outlined(dl, ImVec2(pos.x + 80.f, pos.y + (h - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(225, 225, 225, 255), value.c_str());
		if (sep)
			dl->AddLine(ImVec2(pos.x, pos.y + h), ImVec2(pos.x + w, pos.y + h), ImGui::GetColorU32(sep_col));
	}

	bool action_button(const char* label, ImVec2 min, ImVec2 max)
	{
		ImGui::PushID(label);
		ImGui::SetCursorScreenPos(min);
		ImGui::InvisibleButton("##act", ImVec2(max.x - min.x, max.y - min.y));
		bool hovered = ImGui::IsItemHovered();
		bool held = ImGui::IsItemActive();
		bool clicked = ImGui::IsItemClicked();

		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(min, max, ImGui::GetColorU32(btn_face), 4.f);
		if (held)
			draw->AddRectFilled(min, max, ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.10f)), 4.f);
		else if (hovered)
			draw->AddRectFilled(min, max, ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.06f)), 4.f);
		draw->AddRect(min, max, ImGui::GetColorU32(sep_col), 4.f);

		ImVec2 ts = ImGui::CalcTextSize(label);
		draw->AddText(ImVec2(min.x + (max.x - min.x - ts.x) * 0.5f, min.y + (max.y - min.y - ts.y) * 0.5f),
			ImGui::GetColorU32(row_text), label);

		ImGui::PopID();
		return clicked;
	}

	void render_properties(bool* open, ImVec2 anchor_pos, ImVec2 anchor_size, const prop_target& target)
	{
		if (!open || !*open)
			return;

		static std::uint64_t last_addr = 0;
		static char val_buf[256] = "";
		static char val_x[32] = "";
		static char val_y[32] = "";
		static char val_z[32] = "";
		static char obj_buf[32] = "";
		static bool val_bool = false;
		static char hum_hp[32] = "";
		static char hum_ws[32] = "";
		static char hum_jp[32] = "";

		refresh_glass_colors();

		bool is_hum = (target.cls == "Humanoid");
		bool is_value = value_base::is_class(target.cls);

		auto sync_value_bufs = [&]() {
			val_buf[0] = 0;
			val_x[0] = val_y[0] = val_z[0] = 0;
			obj_buf[0] = 0;
			val_bool = false;
			hum_hp[0] = hum_ws[0] = hum_jp[0] = 0;
			if (!target.address)
				return;
			if (is_value)
			{
				if (target.cls == "BoolValue")
					val_bool = value_base::get_bool(target.address);
				else if (target.cls == "IntValue")
					snprintf(val_buf, sizeof(val_buf), "%d", value_base::get_int(target.address));
				else if (target.cls == "NumberValue")
					snprintf(val_buf, sizeof(val_buf), "%.6g", value_base::get_number(target.address));
				else if (target.cls == "StringValue")
				{
					std::string s = value_base::get_string(target.address);
					if (s.size() >= sizeof(val_buf))
						s.resize(sizeof(val_buf) - 1);
					memcpy(val_buf, s.c_str(), s.size() + 1);
				}
				else if (target.cls == "ObjectValue")
				{
					snprintf(obj_buf, sizeof(obj_buf), "0x%llX",
						(unsigned long long)value_base::get_object(target.address));
				}
				else if (target.cls == "Vector3Value")
				{
					math::vector3 v = value_base::get_vector3(target.address);
					snprintf(val_x, sizeof(val_x), "%.6g", v.x);
					snprintf(val_y, sizeof(val_y), "%.6g", v.y);
					snprintf(val_z, sizeof(val_z), "%.6g", v.z);
				}
			}
			if (is_hum)
			{
				rbx::humanoid_t hu{ target.address };
				snprintf(hum_hp, sizeof(hum_hp), "%.1f", hu.get_health());
				snprintf(hum_ws, sizeof(hum_ws), "%.1f",
					memory->read<float>(target.address + Offsets::Humanoid::Walkspeed));
				snprintf(hum_jp, sizeof(hum_jp), "%.1f",
					memory->read<float>(target.address + Offsets::Humanoid::JumpPower));
			}
		};

		if (target.address != last_addr)
		{
			last_addr = target.address;
			sync_value_bufs();
		}

		constexpr float title_h = 26.f;
		constexpr float margin = 3.f;
		constexpr float gap = 6.f;
		constexpr float header_h = 22.f;
		constexpr float info_row_h = 24.f;
		constexpr float gap_y = 6.f;
		constexpr float prop_w = 400.f;
		constexpr float edit_row_h = 28.f;

		int info_rows = 4;
		float info_h = header_h + info_row_h * (float)info_rows;
		int value_rows = 0;
		if (is_value)
		{
			if (target.cls == "Vector3Value")      value_rows = 3;
			else if (target.cls == "ObjectValue")  value_rows = 2;
			else                                   value_rows = 1;
		}
		else if (is_hum)
			value_rows = 3;

		float value_h = value_rows > 0 ? (header_h + edit_row_h * (float)value_rows + 8.f) : 0.f;
		float fit_h = title_h + margin + info_h + margin;
		if (value_h > 0.f)
			fit_h += gap_y + value_h;

		ImVec2 prop_pos(anchor_pos.x + anchor_size.x + gap, anchor_pos.y);
		{
			const ImVec2 vp_min = ImGui::GetMainViewport()->WorkPos;
			const ImVec2 vp_max = ImVec2(vp_min.x + ImGui::GetMainViewport()->WorkSize.x,
				vp_min.y + ImGui::GetMainViewport()->WorkSize.y);
			if (prop_pos.x + prop_w > vp_max.x) prop_pos.x = vp_max.x - prop_w;
			if (prop_pos.x < vp_min.x)          prop_pos.x = vp_min.x;
			if (prop_pos.y < vp_min.y)          prop_pos.y = vp_min.y;
			if (prop_pos.y + fit_h > vp_max.y)  prop_pos.y = vp_max.y - fit_h;
			if (prop_pos.y < vp_min.y)          prop_pos.y = vp_min.y;
		}
		ImGui::SetNextWindowPos(prop_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(prop_w, fit_h), ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_Border, border_outer);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, glass_body);
		bool visible = ImGui::Begin("##aether_properties_window", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		if (!visible)
		{
			ImGui::End();
			return;
		}

		ImVec2 wp = ImGui::GetWindowPos();
		ImVec2 ws = ImGui::GetWindowSize();
		ImDrawList* draw = ImGui::GetWindowDrawList();

		aether::ui::frost_face(draw, wp, ImVec2(wp.x + ws.x, wp.y + ws.y),
			ImGui::GetStyle().WindowRounding, aether::glass::frost, ImGui::GetCurrentWindowRead()->ID);

		const ImVec4 accent = nm::style().accent;
		const float  title_r = ImGui::GetStyle().WindowRounding;
		draw->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + title_h), ImGui::GetColorU32(glass_title),
			title_r, ImDrawFlags_RoundCornersTop);
		draw->AddLine(ImVec2(wp.x + 1.f, wp.y + title_h - 0.5f),
			ImVec2(wp.x + ws.x - 1.f, wp.y + title_h - 0.5f), nm::rgb(255, 255, 255, 18), 1.f);

		const float pico = 16.f;
		const float pth  = nm::text_h(nm::style().body_medium);
		icons::draw(draw, icons::Id::Tree, ImVec2(wp.x + 13.f + pico * 0.5f, wp.y + title_h * 0.5f), pico,
			ImGui::GetColorU32(accent), 0.f);
		nm::text_clipped(ImVec2(wp.x + 13.f + pico + 7.f, wp.y + (title_h - pth) * 0.5f),
			target.name.empty() ? "Properties" : target.name.c_str(),
			nm::style().body_medium, nm::style().text, ws.x - (13.f + pico + 7.f) - 30.f);

		const float xsz = 18.f;
		const ImVec2 xmin(wp.x + ws.x - xsz - 4.f, wp.y + (title_h - xsz) * 0.5f);
		ImGui::SetCursorScreenPos(xmin);
		ImGui::InvisibleButton("##properties_close", ImVec2(xsz, xsz));
		bool xhov = ImGui::IsItemHovered();
		if (xhov)
			draw->AddRectFilled(xmin, ImVec2(xmin.x + xsz, xmin.y + xsz),
				ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.18f)), 4.f);
		if (ImGui::IsItemClicked())
			*open = false;
		icons::draw(draw, icons::Id::Close, ImVec2(xmin.x + xsz * 0.5f, xmin.y + xsz * 0.5f), xsz * 0.5f,
			xhov ? IM_COL32(255, 255, 255, 255) : IM_COL32(168, 168, 176, 255), 0.f);

		float body_top = title_h + margin;
		float body_h = ws.y - body_top - margin;
		float body_w = ws.x - margin * 2.f;

		ImGui::SetCursorPos(ImVec2(margin, body_top));
		ImGui::PushStyleColor(ImGuiCol_Border, border_inner);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, glass_body);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		ImGui::BeginChild("##properties_body", ImVec2(body_w, body_h), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleVar();

		char addr_buf[40];
		snprintf(addr_buf, sizeof(addr_buf), "0x%llX", (unsigned long long)target.address);

		int kids = target.children;
		char children_buf[16];
		snprintf(children_buf, sizeof(children_buf), "%d", kids);

		ImGui::SetCursorPos(ImVec2(0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_Border, border_inner);
		ImGui::BeginChild("##properties_info", ImVec2(body_w, info_h), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);
		{
			ImVec2 ip = ImGui::GetWindowPos();
			ImVec2 isz = ImGui::GetWindowSize();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			nm::text_at(ImVec2(ip.x + 10.f, ip.y + (header_h - nm::text_h(nm::style().body_medium)) * 0.5f),
				"Info", nm::style().body_medium, nm::style().text);
			dl->AddLine(ImVec2(ip.x, ip.y + header_h), ImVec2(ip.x + isz.x, ip.y + header_h), ImGui::GetColorU32(header_rule));

			float y = ip.y + header_h;
			info_row(dl, ImVec2(ip.x, y), isz.x, info_row_h, "name", target.name.empty() ? "?" : target.name, true);
			y += info_row_h;
			info_row(dl, ImVec2(ip.x, y), isz.x, info_row_h, "class", target.cls, true);
			y += info_row_h;
			info_row(dl, ImVec2(ip.x, y), isz.x, info_row_h, "path", target.path, true);
			y += info_row_h;
			info_row(dl, ImVec2(ip.x, y), isz.x, info_row_h, "address", addr_buf, false);
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		if (value_rows > 0)
		{
			ImGui::SetCursorPos(ImVec2(0.f, info_h + margin));
			ImGui::PushStyleColor(ImGuiCol_Border, border_inner);
			ImGui::BeginChild("##properties_value", ImVec2(body_w, value_h), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);
			{
				ImVec2 vp = ImGui::GetWindowPos();
				ImVec2 vsz = ImGui::GetWindowSize();
				ImDrawList* dl = ImGui::GetWindowDrawList();
				nm::text_at(ImVec2(vp.x + 10.f, vp.y + (header_h - nm::text_h(nm::style().body_medium)) * 0.5f),
					"Values", nm::style().body_medium, nm::style().text);
				dl->AddLine(ImVec2(vp.x, vp.y + header_h), ImVec2(vp.x + vsz.x, vp.y + header_h), ImGui::GetColorU32(header_rule));

				float y = header_h;

				auto edit_row = [&](const char* label, char* buf, int buf_sz, auto apply) {
					widgets::text_outlined(dl, ImVec2(vp.x + 10.f, vp.y + y + (edit_row_h - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(140, 140, 140, 255), label);
					ImGui::SetCursorScreenPos(ImVec2(vp.x + 80.f, vp.y + y + 3.f));
					ImGui::PushItemWidth(vsz.x - 90.f);
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.35f));
					bool committed = ImGui::InputText(label, buf, buf_sz, ImGuiInputTextFlags_EnterReturnsTrue);
					ImGui::PopStyleColor();
					ImGui::PopItemWidth();
					if (committed)
						apply();
					y += edit_row_h;
				};

				if (is_value)
				{
					if (target.cls == "BoolValue")
					{
						ImGui::SetCursorScreenPos(ImVec2(vp.x + 10.f, vp.y + y + 4.f));
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 140, 140, 255));
						ImGui::TextUnformatted("value");
						ImGui::PopStyleColor();
						ImGui::SetCursorScreenPos(ImVec2(vp.x + 80.f, vp.y + y + 3.f));

						if (ImAdd::CheckBox("##pv_bool", &val_bool))
							value_base::set_bool(target.address, val_bool);
						y += edit_row_h;
					}
					else if (target.cls == "IntValue" || target.cls == "NumberValue" || target.cls == "StringValue")
					{
						edit_row("value", val_buf, sizeof(val_buf), [&]() {
							if (target.cls == "IntValue")
								value_base::set_int(target.address, atoi(val_buf));
							else if (target.cls == "NumberValue")
								value_base::set_number(target.address, atof(val_buf));
							else
								value_base::set_string(target.address, val_buf);
						});
					}
					else if (target.cls == "ObjectValue")
					{
						info_row(dl, ImVec2(vp.x, vp.y + y), vsz.x, info_row_h, "current", obj_buf, true);
						y += info_row_h;
						edit_row("addr", obj_buf, sizeof(obj_buf), [&]() {
							std::uint64_t a = strtoull(obj_buf, nullptr, 16);
							if (a)
								value_base::set_object(target.address, a);
						});
					}
					else if (target.cls == "Vector3Value")
					{
						auto apply_v3 = [&]() {
							math::vector3 v{};
							v.x = (float)atof(val_x);
							v.y = (float)atof(val_y);
							v.z = (float)atof(val_z);
							value_base::set_vector3(target.address, v);
						};
						edit_row("X", val_x, sizeof(val_x), apply_v3);
						edit_row("Y", val_y, sizeof(val_y), apply_v3);
						edit_row("Z", val_z, sizeof(val_z), apply_v3);
					}
				}
				else if (is_hum)
				{
					edit_row("health", hum_hp, sizeof(hum_hp), [&]() {
						memory->write<float>(target.address + Offsets::Humanoid::Health, (float)atof(hum_hp));
					});
					edit_row("walkspeed", hum_ws, sizeof(hum_ws), [&]() {
						memory->write<float>(target.address + Offsets::Humanoid::Walkspeed, (float)atof(hum_ws));
					});
					edit_row("jumppower", hum_jp, sizeof(hum_jp), [&]() {
						memory->write<float>(target.address + Offsets::Humanoid::JumpPower, (float)atof(hum_jp));
					});
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();
		}

		ImGui::EndChild();
		ImGui::PopStyleColor(2);

		ImGui::End();
	}

	ID3D11Device* g_device = nullptr;

}

namespace aether_explorer
{

void init(ID3D11Device* device)
{
	g_device = device;
	aether_icons::icons_init(device);
}

void shutdown()
{
	stop_search();
	g_root = Node{};
	g_root_addr = 0;
}

static void draw_workspace(ImVec2 wp, ImVec2 ws)
{
	static std::uint64_t selected_addr = 0;
	static ctx_target ctx;
	static ImVec2 ctx_pos(0.f, 0.f);
	static bool properties_open = false;
	static prop_target prop_target;
	static char search_buf[128] = "";

	refresh_glass_colors();
	ensure_root();

	constexpr float title_h = 26.f;
	constexpr float margin = 3.f;

	float body_top = title_h + margin;
	float body_h = ws.y - body_top - margin;
	float body_w = ws.x - margin * 2.f;

	const sync::Material mat = sync::material();

	ImGui::SetCursorScreenPos(ImVec2(wp.x + margin, wp.y + body_top));
	ImGui::PushStyleColor(ImGuiCol_Border, border_inner);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, glass_body);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, mat.roundWindow);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
	ImGui::BeginChild("##explorer_body", ImVec2(body_w, body_h), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleVar(3);

	constexpr float search_pad = 6.f;
	constexpr float header_h = 22.f;

	ImGui::SetCursorPos(ImVec2(search_pad, search_pad + 2.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
	ImGui::PushItemWidth(body_w - search_pad * 2.f);
	bool changed = ImGui::InputTextWithHint("##explorer_search", "search name / class...",
		search_buf, sizeof(search_buf));
	{

		ImDrawList* sdl = ImGui::GetWindowDrawList();
		const ImVec2 rmin = ImGui::GetItemRectMin();
		const ImVec2 rmax = ImGui::GetItemRectMax();
		const float  ico = ImGui::GetFontSize();
		icons::draw(sdl, icons::Id::Search,
			ImVec2(rmax.x - ico * 0.5f - 6.f, (rmin.y + rmax.y) * 0.5f), ico,
			ImGui::GetColorU32(search_buf[0] ? ImGuiCol_Text : ImGuiCol_TextDisabled), 0.f);
	}
	ImGui::PopItemWidth();
	ImGui::PopStyleVar();

	if (ImGui::IsItemDeactivatedAfterEdit() && search_buf[0])
		start_search(search_buf);
	if (changed && search_buf[0] == '\0' && g_search_on)
		stop_search();
	if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false) && search_buf[0])
		start_search(search_buf);

	float search_bottom = search_pad + ImGui::GetItemRectSize().y;
	float panel_top = search_bottom + search_pad;
	float panel_w = body_w - search_pad * 2.f;

	constexpr float struct_h = 238.f;
	float struct_top = body_h - struct_h - search_pad;
	float panel_h = struct_top - panel_top - search_pad;
	if (panel_h < 60.f) panel_h = 60.f;

	ImGui::SetCursorPos(ImVec2(search_pad, panel_top));
	ImGui::PushStyleColor(ImGuiCol_Border, border_inner);
	ImGui::BeginChild("##explorer_panel", ImVec2(panel_w, panel_h), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);
	{
		ImVec2 pp = ImGui::GetWindowPos();
		ImVec2 psz = ImGui::GetWindowSize();
		ImDrawList* dl = ImGui::GetWindowDrawList();

		nm::text_at(ImVec2(pp.x + 10.f, pp.y + (header_h - nm::text_h(nm::style().body_medium)) * 0.5f),
			"Datamodel", nm::style().body_medium, nm::style().text);
		dl->AddLine(ImVec2(pp.x + 1.f, pp.y + header_h), ImVec2(pp.x + psz.x - 1.f, pp.y + header_h), ImGui::GetColorU32(header_rule));

		ImGui::SetCursorPos(ImVec2(0.f, header_h));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::BeginChild("##explorer_tree", ImVec2(psz.x, psz.y - header_h), ImGuiChildFlags_None);

		if (g_search_on)
		{
			std::lock_guard<std::mutex> lk(g_search_mtx);
			if (g_searching.load())
				ImGui::TextColored(row_text, "searching...");
			if (g_search_results.empty() && !g_searching.load())
				ImGui::TextColored(row_text, "no matches");

			ImGuiListClipper clipper;
			clipper.Begin((int)g_search_results.size(), 22.f);
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
					render_search_row(g_search_results[i], i, selected_addr, ctx, ctx_pos, prop_target, properties_open);
			}
		}
		else if (g_root.address)
		{
			if (!g_root.loaded)
				load_children(g_root);
			for (auto& c : g_root.children)
				render_node(c, 0, selected_addr, ctx, ctx_pos, prop_target, properties_open);
		}
		else
		{
			ImGui::TextColored(row_text, "not attached / no datamodel");
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, glass_title);
		ImGui::PushStyleColor(ImGuiCol_Border, border_outer);
		ImGui::SetNextWindowPos(ctx_pos, ImGuiCond_Appearing);
		if (ImGui::BeginPopup("##ex_ctx_menu"))
		{
			if (ctx.valid)
			{
				ImDrawList* pdl = ImGui::GetWindowDrawList();
				constexpr float row_h = 24.f;
				constexpr float pad_x = 10.f;
				constexpr float w = 170.f;
				ImVec2 pwp = ImGui::GetWindowPos();

				float header_top = 8.f;
				float name_h = ImGui::GetTextLineHeight();
				const char* nm = ctx.name.empty() ? "?" : ctx.name.c_str();
				widgets::text_outlined(pdl, ImVec2(pwp.x + pad_x, pwp.y + header_top), IM_COL32(235, 235, 235, 255), nm);
				widgets::text_outlined(pdl, ImVec2(pwp.x + pad_x, pwp.y + header_top + name_h + 2.f), IM_COL32(130, 130, 130, 255), ctx.cls.c_str());
				float ctx_header_h = header_top + name_h * 2.f + 2.f + 8.f;
				pdl->AddLine(ImVec2(pwp.x, pwp.y + ctx_header_h), ImVec2(pwp.x + w, pwp.y + ctx_header_h), ImGui::GetColorU32(border_inner));

				ImGui::SetCursorPos(ImVec2(0.f, ctx_header_h));

				auto ctx_row = [&](const char* label) -> bool {
					ImGui::PushID(label);
					ImVec2 rp = ImGui::GetCursorScreenPos();
					ImVec2 rmax(rp.x + w, rp.y + row_h);
					bool row_clicked = ImGui::Selectable("##r", false, 0, ImVec2(w, row_h));
					bool hov = ImGui::IsItemHovered();
					ImGui::PopID();
					if (hov)
						pdl->AddRectFilled(rp, rmax, ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.08f)));
					widgets::text_outlined(pdl, ImVec2(rp.x + pad_x, rp.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f), IM_COL32(220, 220, 220, 255), label);
					return row_clicked;
				};

				if (ctx_row("properties"))
				{
					fill_prop(prop_target, ctx.address, ctx.name, ctx.cls, ctx.path, ctx.children);
					properties_open = true;
					ImGui::CloseCurrentPopup();
				}
				if (ctx_row("copy name"))
				{
					ImGui::SetClipboardText(ctx.name.c_str());
					ImGui::CloseCurrentPopup();
				}
				if (ctx_row("copy class"))
				{
					ImGui::SetClipboardText(ctx.cls.c_str());
					ImGui::CloseCurrentPopup();
				}
				if (ctx_row("copy path"))
				{
					ImGui::SetClipboardText(ctx.path.c_str());
					ImGui::CloseCurrentPopup();
				}
				if (ctx_row("copy address"))
				{
					char buf[32];
					snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)ctx.address);
					ImGui::SetClipboardText(buf);
					ImGui::CloseCurrentPopup();
				}
				if (ctx_row("teleport here") && ctx.address)
				{
					try {
						rbx::part_t dst(ctx.address);
						rbx::primitive_t dprim = dst.get_primitive();
						if (dprim.address)
						{
							math::vector3 dest = dprim.get_position();
							dest.y += 3.f;

							std::uint64_t chr = game_ctx::resolve_local_character();
							if (chr)
							{
								rbx::model_instance_t mdl{ chr };
								rbx::instance_t hrp = mdl.find_first_child("HumanoidRootPart");
								if (hrp.address)
								{
									rbx::part_t hp(hrp.address);
									rbx::primitive_t hprim = hp.get_primitive();
									if (hprim.address)
									{
										memory->write<math::vector3>(
											hprim.address + Offsets::Primitive::Position, dest);
										memory->write<math::vector3>(
											hprim.address + Offsets::Primitive::AssemblyLinearVelocity, {});
									}
								}
							}
						}
					} catch (...) {}
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		aether::ui::smooth_scroll(settings::appearance::smooth_scroll, settings::appearance::scroll_speed);
		ImGui::EndChild();
		ImGui::PopStyleColor(3);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	{
		static char gs_players_buf[64]   = "Players";
		static char gs_character_buf[64] = "Character";
		static bool gs_manual            = false;
		static bool gs_auto_apply        = true;
		static bool gs_detected          = false;

		game_structure::info_t cur;

		auto apply_paths = [&]() {
			game_structure::players_name_override   = gs_players_buf;
			game_structure::character_name_override = gs_character_buf;
			game_structure::use_overrides           = gs_manual;
			game_structure::auto_apply.store(gs_auto_apply);

			game_structure::scan_requested.store(gs_manual || gs_detected);
		};

		auto run_scan = [&]() {
			gs_detected = true;
			game_structure::info_t found = game_structure::detect();
			{
				std::lock_guard<std::mutex> lk(game_structure::mtx);
				game_structure::g_info = found;
			}
			cur = found;
			snprintf(gs_players_buf, sizeof(gs_players_buf), "%s", found.players_name.c_str());
			snprintf(gs_character_buf, sizeof(gs_character_buf), "%s", found.character_name.c_str());

			game_structure::players_name_override   = gs_players_buf;
			game_structure::character_name_override = gs_character_buf;
			game_structure::use_overrides           = gs_manual;
			game_structure::scan_requested.store(true);
			game_structure::auto_apply.store(gs_auto_apply);
		};

		ImGui::SetCursorPos(ImVec2(search_pad, struct_top));
		ImGui::PushStyleColor(ImGuiCol_Border, border_inner);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, glass_body);
		ImGui::BeginChild("##explorer_structure", ImVec2(panel_w, struct_h), ImGuiChildFlags_Border, ImGuiWindowFlags_None);
		ImGui::PopStyleColor();
		{
			ImVec2 sp = ImGui::GetWindowPos();
			ImVec2 ssz = ImGui::GetWindowSize();
			ImDrawList* sdl = ImGui::GetWindowDrawList();
			nm::text_at(ImVec2(sp.x + 10.f, sp.y + (header_h - nm::text_h(nm::style().body_medium)) * 0.5f),
				"Game Structure", nm::style().body_medium, nm::style().text);
			sdl->AddLine(ImVec2(sp.x + 1.f, sp.y + header_h), ImVec2(sp.x + ssz.x - 1.f, sp.y + header_h),
				ImGui::GetColorU32(header_rule));

			{
				std::lock_guard<std::mutex> lk(game_structure::mtx);
				cur = game_structure::g_info;
			}

			const float row_h   = 17.f;
			const float pad_x   = 8.f;
			const float label_w = 62.f;
			const float cw      = ssz.x - pad_x * 2.f;
			const float val_w   = cw - label_w - 6.f;
			float y = header_h + 5.f;

			auto trunc_fit = [](const std::string& s, float max_w) -> std::string {
				if (s.empty() || ImGui::CalcTextSize(s.c_str()).x <= max_w)
					return s;
				std::string out = s;
				while (out.size() > 1)
				{
					out.pop_back();
					std::string t = out + "...";
					if (ImGui::CalcTextSize(t.c_str()).x <= max_w)
						return t;
				}
				return "...";
			};

			ImGui::SetCursorPos(ImVec2(pad_x, y));
			bool gs_clicked = ImGui::Button("detect", ImVec2(62.f, 20.f));
			if (gs_clicked)
				run_scan();
			ImGui::SameLine(0.f, 10.f);
			ImGui::SetCursorPosY(y + 3.f);
			{
				const char* st = "idle - press detect";
				ImVec4       sc(0.72f, 0.72f, 0.72f, 1.f);
				if (gs_detected)
				{
					if (gs_manual)          { st = "manual paths active";    sc = ImVec4(0.55f, 0.85f, 1.00f, 1.f); }
					else if (gs_auto_apply) { st = "applied - esp wired";    sc = ImVec4(0.45f, 0.95f, 0.55f, 1.f); }
					else                    { st = "detected - not applied"; sc = ImVec4(1.00f, 0.80f, 0.35f, 1.f); }
				}
				ImGui::TextColored(sc, "%s", st);
			}
			y += 25.f;

			auto kv_row = [&](const char* k, const std::string& v, ImU32 vcol) {
				ImGui::SetCursorPos(ImVec2(pad_x, y));
				ImGui::TextColored(ImVec4(0.62f, 0.66f, 0.72f, 1.f), "%s", k);
				ImGui::SameLine(pad_x + label_w);
				ImGui::PushStyleColor(ImGuiCol_Text, vcol);
				ImGui::TextUnformatted(trunc_fit(v, val_w).c_str());
				ImGui::PopStyleColor();
				y += row_h;
			};

			kv_row("players",
				gs_detected ? (cur.players_path.empty() ? cur.players_name : cur.players_path) : "-",
				gs_detected ? IM_COL32(225, 225, 225, 255) : IM_COL32(120, 120, 120, 255));

			std::string mv = gs_detected
				? (std::to_string(cur.character_count) + " rigs | " + cur.rig + " | " + std::to_string(cur.part_count) + " parts")
				: "-";
			kv_row("models", mv,
				(gs_detected && cur.character_count > 0) ? IM_COL32(150, 255, 170, 255) : IM_COL32(200, 200, 200, 255));

			kv_row("health",
				gs_detected ? (cur.health_source.empty() ? "-" : cur.health_source) : "-",
				IM_COL32(210, 210, 210, 255));
			y += 3.f;

			{
				ImGui::SetCursorPos(ImVec2(pad_x, y));
				bool toggled = ImGui::Selectable("##gs_autoApply", false,
					ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cw, row_h + 4.f));
				if (toggled)
				{
					gs_auto_apply = !gs_auto_apply;
					apply_paths();
				}
				ImVec2 rp = ImGui::GetItemRectMin();
				ImVec2 rs = ImGui::GetItemRectSize();
				float bh = 14.f;
				ImVec2 bmin(rp.x + 3.f, rp.y + (rs.y - bh) * 0.5f);
				ImVec2 bmax(bmin.x + bh, bmin.y + bh);
				sdl->AddRect(bmin - ImVec2(1, 1), bmax + ImVec2(1, 1), IM_COL32(235, 235, 235, 220));
				sdl->AddRectFilled(bmin, bmax,
					gs_auto_apply ? IM_COL32(40, 70, 48, 230) : IM_COL32(28, 28, 32, 230));
				if (gs_auto_apply)
				{
					sdl->AddLine(ImVec2(bmin.x + 2.f, bmin.y + 7.f),
						ImVec2(bmin.x + 5.5f, bmin.y + 10.5f), IM_COL32(90, 240, 130, 255), 2.2f);
					sdl->AddLine(ImVec2(bmin.x + 5.5f, bmin.y + 10.5f),
						ImVec2(bmin.x + 11.5f, bmin.y + 3.f), IM_COL32(90, 240, 130, 255), 2.2f);
				}
				widgets::text_outlined(sdl, ImVec2(bmax.x + 8.f, rp.y + (rs.y - ImGui::GetTextLineHeight()) * 0.5f),
					IM_COL32(215, 215, 215, 255), "auto-apply paths");
				const char* badge = gs_auto_apply ? "ON" : "OFF";
				widgets::text_outlined(sdl,
					ImVec2(sp.x + ssz.x - pad_x - ImGui::CalcTextSize(badge).x, rp.y + (rs.y - ImGui::GetTextLineHeight()) * 0.5f),
					gs_auto_apply ? IM_COL32(150, 255, 170, 255) : IM_COL32(160, 160, 160, 200), badge);
			}
			y += row_h + 9.f;

			ImGui::SetCursorPos(ImVec2(pad_x, y));
			ImGui::TextColored(ImVec4(0.55f, 0.58f, 0.64f, 1.f), "MANUAL PATHS");
			y += 15.f;

			ImGui::SetCursorPos(ImVec2(pad_x, y));
			ImGui::TextColored(ImVec4(0.62f, 0.66f, 0.72f, 1.f), "players");
			ImGui::SameLine(pad_x + label_w);
			ImGui::PushItemWidth(val_w);
			bool pl_edited = ImGui::InputText("##gs_players", gs_players_buf, sizeof(gs_players_buf));
			ImGui::PopItemWidth();
			y += 21.f;

			ImGui::SetCursorPos(ImVec2(pad_x, y));
			ImGui::TextColored(ImVec4(0.62f, 0.66f, 0.72f, 1.f), "char model");
			ImGui::SameLine(pad_x + label_w);
			ImGui::PushItemWidth(val_w);
			bool ch_edited = ImGui::InputText("##gs_char", gs_character_buf, sizeof(gs_character_buf));
			ImGui::PopItemWidth();
			if (pl_edited || ch_edited)
			{
				gs_manual = true;
				apply_paths();
			}
			y += 21.f;

			ImGui::SetCursorPos(ImVec2(pad_x, y));
			if (ImGui::Checkbox("use these paths", &gs_manual))
				apply_paths();
			y += row_h + 2.f;

			ImGui::SetCursorPos(ImVec2(pad_x, y));
			const char* hint = gs_manual ? "typed names override detection everywhere"
				: (gs_detected ? (gs_auto_apply ? "found layout feeds esp/aim automatically"
											    : "tick auto-apply to wire the layout in")
							   : "nothing scans until you press detect");
			ImGui::TextColored(ImVec4(0.60f, 0.62f, 0.66f, 1.f), "%s", trunc_fit(hint, cw).c_str());
		}
		aether::ui::smooth_scroll(settings::appearance::smooth_scroll, settings::appearance::scroll_speed);
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	ImGui::EndChild();
	ImGui::PopStyleColor(2);

	if (properties_open)
		render_properties(&properties_open, wp, ws, prop_target);
}

void page(ImVec2 pos, ImVec2 size)
{
	if (size.x < 120.f || size.y < 80.f)
		return;

	ScopedWidgetStyle widget_style;
	draw_workspace(pos, size);
}

}

