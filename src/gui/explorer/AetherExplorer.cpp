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

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cfloat>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <ShlObj.h>
#include <functional>
#include <filesystem>

namespace
{
	using namespace aether;

	// The panel's material, resolved once per frame from the menu's own theme,
	// so the explorer cannot drift from the rest of the panel.
	ImVec4 border_outer(0.13f, 0.13f, 0.13f, 1.f);
	ImVec4 border_inner(0.18f, 0.18f, 0.18f, 1.f);
	ImVec4 glass_title (0.078f, 0.078f, 0.086f, 0.62f);
	ImVec4 glass_body  (0.055f, 0.055f, 0.063f, 0.52f);
	ImVec4 row_accent  (1.f, 1.f, 1.f, 0.35f);
	ImVec4 row_text    (0.85f, 0.85f, 0.88f, 1.f);
	ImVec4 sep_col     (1.f, 1.f, 1.f, 0.10f);
	ImVec4 header_rule (1.f, 1.f, 1.f, 0.30f);

	inline ImVec4 u32_to_vec4(ImU32 c)
	{
		return ImVec4(
			(float)((c >> IM_COL32_R_SHIFT) & 0xFF) / 255.f,
			(float)((c >> IM_COL32_G_SHIFT) & 0xFF) / 255.f,
			(float)((c >> IM_COL32_B_SHIFT) & 0xFF) / 255.f,
			(float)((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.f);
	}

	inline ImU32 vec4_to_u32(const ImVec4& c)
	{
		return ImGui::GetColorU32(c);
	}

	inline ImU32 with_alpha(ImVec4 c, float a)
	{
		c.w = a;
		return ImGui::GetColorU32(c);
	}

	inline void refresh_glass_colors()
	{
		const sync::Material m = sync::material();

		border_outer = ImVec4(1.f, 1.f, 1.f, 0.06f);
		border_inner = u32_to_vec4(m.hairline);

		// The cards are glass, not plates: the theme's surface with most of its
		// alpha taken out, so the blur reads through them.  No Transparency puts
		// it back, because that setting is the promise of a solid panel.
		const float see = settings::appearance::no_transparency ? 1.f : 0.58f;

		glass_body = u32_to_vec4(m.surface);
		glass_body.w *= see;

		glass_title = glass_body;
		glass_title.x = ImMin(1.f, glass_title.x + 0.012f);
		glass_title.y = ImMin(1.f, glass_title.y + 0.012f);
		glass_title.z = ImMin(1.f, glass_title.z + 0.016f);
		glass_title.w = ImMin(1.f, glass_title.w * 1.25f + 0.04f);

		row_accent  = u32_to_vec4(m.accent);
		row_text    = u32_to_vec4(m.text);
		sep_col     = u32_to_vec4(m.hairline);
		header_rule = u32_to_vec4(m.rule);
	}

	struct ScopedWidgetStyle
	{
		ScopedWidgetStyle()
		{
			const sync::Material m = sync::material();

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding,    7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,  10.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 8.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 4.f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(7.f, 5.f));
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 8.f);

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
			ImGui::PopStyleColor(19);
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

	struct workspace_state
	{
		std::uint64_t selected = 0;
		ctx_target    ctx;
		ImVec2        ctx_pos;
		prop_target   prop;
		bool          prop_open = false;
		char          search[128] = "";
	};

	workspace_state g_ws;

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

	void select_node(std::uint64_t addr, const std::string& name, const std::string& cls, const std::string& path, int children)
	{
		g_ws.selected = addr;
		fill_prop(g_ws.prop, addr, name, cls, path, children);
		g_ws.prop_open = true;
	}

	// ---------------------------------------------------------------------
	// Cards
	//
	// The workspace is three of them, all the same shape: a 26 px header with
	// the icon, the title and a short piece of meta on the right, then the body.
	// They are drawn by hand rather than with ImGui windows so that the header,
	// the fill and the hairline cannot disagree with the rest of the panel.

	constexpr float k_header_h = 27.f;
	constexpr float k_card_rad = 11.f;

	void card_frame(ImVec2 p0, ImVec2 p1)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(p0, p1, ImGui::GetColorU32(glass_body), k_card_rad);
		dl->AddRect(p0, p1, ImGui::GetColorU32(sep_col), k_card_rad, 0, 1.f);
	}

	float card_header(ImDrawList* dl, ImVec2 p0, ImVec2 p1, icons::Id icon,
		const char* title, const char* meta)
	{
		icons::draw(dl, icon, ImVec2(p0.x + 17.f, p0.y + k_header_h * 0.5f), 14.f,
			ImGui::GetColorU32(nm::style().accent), 0.f);

		nm::text_at(ImVec2(p0.x + 29.f, p0.y + (k_header_h - nm::text_h(nm::style().body_medium)) * 0.5f),
			title, nm::style().body_medium, nm::style().text);

		if (meta && *meta)
		{
			const float mw = nm::text_w(meta, nm::style().caption);
			nm::text_at(ImVec2(p1.x - 13.f - mw,
					p0.y + (k_header_h - nm::text_h(nm::style().caption)) * 0.5f),
				meta, nm::style().caption, nm::style().text_dim);
		}

		dl->AddLine(ImVec2(p0.x + 1.f, p0.y + k_header_h),
			ImVec2(p1.x - 1.f, p0.y + k_header_h), ImGui::GetColorU32(sep_col), 1.f);
		return p0.y + k_header_h;
	}

	// A control that belongs to the card rather than to ImGui's default look:
	// a faint fill, a hairline, and a press that sinks by a pixel.
	bool soft_button(const char* id, const char* label, ImVec2 min, ImVec2 size, bool primary)
	{
		ImGui::PushID(id);
		ImGui::SetCursorScreenPos(min);
		ImGui::InvisibleButton("##b", size);
		const bool hovered = ImGui::IsItemHovered();
		const bool held    = ImGui::IsItemActive();
		const bool clicked = ImGui::IsItemClicked();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const float r = 7.f;
		const ImVec2 max(min.x + size.x, min.y + size.y);

		const ImVec4 accent = nm::style().accent;
		ImU32 fill = primary ? with_alpha(accent, held ? 0.26f : hovered ? 0.20f : 0.14f)
		                     : ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, held ? 0.12f : hovered ? 0.09f : 0.05f));
		dl->AddRectFilled(min, max, fill, r);
		dl->AddRect(min, max, ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, primary ? 0.14f : 0.08f)), r);

		const ImVec2 ts = ImGui::CalcTextSize(label);
		dl->AddText(ImVec2(min.x + (size.x - ts.x) * 0.5f, min.y + (size.y - ts.y) * 0.5f),
			ImGui::GetColorU32(primary ? accent : row_text), label);

		ImGui::PopID();
		return clicked;
	}

	// ---------------------------------------------------------------------
	// The tree

	void render_node(Node& node, int depth)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		constexpr float row_h  = 22.f;
		constexpr float indent = 14.f;
		constexpr float icon_s = 14.f;

		const bool has_children = node.loaded ? !node.children.empty() : node.has_kids;
		const bool selected = (g_ws.selected == node.address);

		const ImVec2 row_pos = ImGui::GetCursorScreenPos();
		const float  avail_w = ImGui::GetContentRegionAvail().x;
		const ImVec2 row_max(row_pos.x + avail_w, row_pos.y + row_h);
		const float  arrow_x = row_pos.x + (float)depth * indent + 3.f;
		const float  arrow_w = indent;

		ImGui::PushID((void*)(uintptr_t)node.address);
		ImGui::Selectable("##row", selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(avail_w, row_h));
		const bool clicked   = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		const bool right_clk = ImGui::IsItemClicked(ImGuiMouseButton_Right);
		const bool hovered   = ImGui::IsItemHovered();
		ImGui::PopID();

		if (clicked)
		{
			const float mx = ImGui::GetIO().MousePos.x;
			const bool on_arrow = has_children && mx >= arrow_x && mx < arrow_x + arrow_w;
			if (on_arrow)
			{
				node.open = !node.open;
				if (node.open)
					load_children(node);
			}
			else
			{
				select_node(node.address, node.name, node.cls, build_path(node.address),
					node.loaded ? (int)node.children.size() : -1);
			}
		}
		if (right_clk)
		{
			g_ws.selected = node.address;
			open_ctx(g_ws.ctx, node.address, node.name, node.cls, build_path(node.address), g_ws.ctx_pos);
		}

		if (hovered && !selected)
			dl->AddRectFilled(ImVec2(row_pos.x + 1.f, row_pos.y + 1.f),
				ImVec2(row_max.x - 2.f, row_max.y - 1.f),
				ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.05f)), 6.f);

		if (selected)
		{
			dl->AddRectFilled(ImVec2(row_pos.x + 1.f, row_pos.y + 1.f),
				ImVec2(row_max.x - 2.f, row_max.y - 1.f), with_alpha(row_accent, 0.13f), 6.f);
			dl->AddRectFilled(ImVec2(row_pos.x + 1.f, row_pos.y + 5.f),
				ImVec2(row_pos.x + 2.5f, row_max.y - 5.f), with_alpha(row_accent, 0.65f), 1.f);
		}

		if (has_children)
			ImGui::RenderArrow(dl, ImVec2(arrow_x + 1.f, row_pos.y + row_h * 0.5f - 4.f),
				ImGui::GetColorU32(ImVec4(row_text.x, row_text.y, row_text.z, node.open ? 0.85f : 0.5f)),
				node.open ? ImGuiDir_Down : ImGuiDir_Right, 0.75f);

		const float icon_x = row_pos.x + (float)depth * indent + indent + 3.f;
		aether_icons::draw_icon(dl, node.cls, ImVec2(icon_x, row_pos.y + (row_h - icon_s) * 0.5f), icon_s);

		const float text_x = icon_x + icon_s + 6.f;
		const float text_y = row_pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f;
		const char* label = node.name.empty() ? "?" : node.name.c_str();

		dl->AddText(ImVec2(text_x, text_y),
			ImGui::GetColorU32(selected ? ImVec4(1.f, 1.f, 1.f, 1.f) : row_text), label);

		if (value_base::is_class(node.cls))
		{
			std::string val = value_base::format(node.address, node.cls);
			if (!val.empty())
			{
				const float lx = text_x + ImGui::CalcTextSize(label).x + 7.f;
				if (lx < row_max.x - 20.f)
					nm::text_at(ImVec2(lx, text_y), ("= " + val).c_str(), nm::style().caption,
						ImVec4(0.55f, 0.75f, 0.58f, 0.95f));
			}
		}

		ImGui::SetCursorScreenPos(ImVec2(row_pos.x, row_pos.y + row_h));

		if (node.open && node.loaded)
		{
			constexpr int max_depth = 40;
			int shown = (int)node.children.size();
			if (shown > 800) shown = 800;

			if (depth + 1 <= max_depth)
			{
				for (int i = 0; i < shown; ++i)
					render_node(node.children[i], depth + 1);
			}

			if ((int)node.children.size() > shown || depth + 1 > max_depth)
			{
				nm::text_at(ImVec2(row_pos.x + (float)(depth + 1) * indent + 4.f, row_pos.y + row_h + 3.f),
					depth + 1 > max_depth ? "... deeper than the tree will go - use search"
					                      : "... the rest is in search",
					nm::style().caption, ImVec4(row_text.x, row_text.y, row_text.z, 0.45f));
				ImGui::SetCursorScreenPos(ImVec2(row_pos.x, row_pos.y + row_h + 19.f));
			}
		}
	}

	void render_search_row(const SearchResult& r, int idx)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		constexpr float row_h  = 22.f;
		constexpr float icon_s = 14.f;

		const bool selected = (g_ws.selected == r.address);
		const ImVec2 row_pos = ImGui::GetCursorScreenPos();
		const float  avail_w = ImGui::GetContentRegionAvail().x;
		const ImVec2 row_max(row_pos.x + avail_w, row_pos.y + row_h);

		ImGui::PushID(idx);
		const bool clicked = ImGui::Selectable("##srow", selected, 0, ImVec2(avail_w, row_h));
		const bool right_clk = ImGui::IsItemClicked(ImGuiMouseButton_Right);
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		if (clicked)
			select_node(r.address, r.name, r.cls, r.path, -1);
		if (right_clk)
		{
			g_ws.selected = r.address;
			open_ctx(g_ws.ctx, r.address, r.name, r.cls, r.path, g_ws.ctx_pos);
		}

		if (hovered && !selected)
			dl->AddRectFilled(ImVec2(row_pos.x + 1.f, row_pos.y + 1.f),
				ImVec2(row_max.x - 2.f, row_max.y - 1.f),
				ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.05f)), 6.f);
		if (selected)
			dl->AddRectFilled(ImVec2(row_pos.x + 1.f, row_pos.y + 1.f),
				ImVec2(row_max.x - 2.f, row_max.y - 1.f), with_alpha(row_accent, 0.13f), 6.f);

		const float icon_x = row_pos.x + 7.f;
		aether_icons::draw_icon(dl, r.cls, ImVec2(icon_x, row_pos.y + (row_h - icon_s) * 0.5f), icon_s);

		const float text_x = icon_x + icon_s + 6.f;
		const float text_y = row_pos.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f;
		const char* label = r.name.empty() ? "?" : r.name.c_str();

		dl->AddText(ImVec2(text_x, text_y),
			ImGui::GetColorU32(selected ? ImVec4(1.f, 1.f, 1.f, 1.f) : row_text), label);

		// The path is the only thing that tells two "Part"s apart in a search,
		// so it is always shown, quietly, after the name.
		const float lx = text_x + ImGui::CalcTextSize(label).x + 8.f;
		if (lx < row_max.x - 30.f)
			nm::text_clipped(ImVec2(lx, text_y), r.path.c_str(), nm::style().caption,
				ImVec4(row_text.x, row_text.y, row_text.z, 0.42f), row_max.x - lx - 8.f);

		ImGui::SetCursorScreenPos(ImVec2(row_pos.x, row_pos.y + row_h));
	}

	// ---------------------------------------------------------------------
	// Properties
	//
	// Inline in the workspace rather than a second window floating beside it:
	// a 400 px card anchored to the side of a 700 px page either covered the
	// page or fell off the screen, and neither is a place to read a value.

	void properties_info_row(ImDrawList* dl, ImVec2 pos, float w, const char* label, const std::string& value)
	{
		const float th = nm::text_h(nm::style().caption);
		nm::text_at(ImVec2(pos.x + 12.f, pos.y), label, nm::style().caption,
			ImVec4(row_text.x, row_text.y, row_text.z, 0.42f));
		nm::text_clipped(ImVec2(pos.x + 74.f, pos.y), value.c_str(), nm::style().caption,
			row_text, w - 86.f);
		(void)th;
	}

	void draw_properties_card(ImVec2 p0, ImVec2 p1)
	{
		card_frame(p0, p1);
		ImDrawList* dl = ImGui::GetWindowDrawList();

		char meta[48] = "";
		if (g_ws.prop_open && g_ws.prop.address)
			snprintf(meta, sizeof meta, "%d children", g_ws.prop.children);

		const float body_y = card_header(dl, p0, p1, icons::Id::Sliders, "Properties", meta);

		ImGui::SetCursorScreenPos(ImVec2(p0.x + 1.f, body_y + 1.f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::BeginChild("##ex_props", ImVec2(p1.x - p0.x - 2.f, p1.y - body_y - 2.f),
			ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

		// Everything below is drawn on the child's list, not the page's: the child
		// carries the card's clip rect, and a hand-drawn row that escapes it would
		// paint over the card under it.
		dl = ImGui::GetWindowDrawList();

		if (!g_ws.prop_open || !g_ws.prop.address)
		{
			nm::text_at(ImVec2(p0.x + 13.f, body_y + 14.f), "Pick an instance in the tree",
				nm::style().caption, ImVec4(row_text.x, row_text.y, row_text.z, 0.38f));
			nm::text_at(ImVec2(p0.x + 13.f, body_y + 30.f), "or search for one above.",
				nm::style().caption, ImVec4(row_text.x, row_text.y, row_text.z, 0.26f));
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			return;
		}

		static std::uint64_t last_addr = 0;
		static char val_buf[256] = "";
		static char val_x[32] = "", val_y[32] = "", val_z[32] = "";
		static char obj_buf[32] = "";
		static bool val_bool = false;
		static char hum_hp[32] = "", hum_ws[32] = "", hum_jp[32] = "";

		const prop_target& target = g_ws.prop;
		const bool is_hum   = (target.cls == "Humanoid");
		const bool is_value = value_base::is_class(target.cls);

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
					snprintf(obj_buf, sizeof(obj_buf), "0x%llX",
						(unsigned long long)value_base::get_object(target.address));
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

		char addr_buf[40];
		snprintf(addr_buf, sizeof addr_buf, "0x%llX", (unsigned long long)target.address);

		const float row_h = 17.f;
		float y = body_y + 9.f;

		nm::text_clipped(ImVec2(p0.x + 13.f, y),
			target.name.empty() ? "?" : target.name.c_str(),
			nm::style().body_medium, nm::style().text, p1.x - p0.x - 26.f);
		y += 19.f;

		nm::text_clipped(ImVec2(p0.x + 13.f, y), target.cls.c_str(),
			nm::style().caption, ImVec4(row_text.x, row_text.y, row_text.z, 0.55f),
			p1.x - p0.x - 26.f);
		y += 15.f;

		properties_info_row(dl, ImVec2(p0.x, y), p1.x - p0.x, "path", target.path);
		y += row_h;

		properties_info_row(dl, ImVec2(p0.x, y), p1.x - p0.x, "address", addr_buf);
		y += row_h + 4.f;

		// The editors.  They are ImGui widgets because a text field is a text
		// field; everything around them is drawn, so the card does not look
		// like a form dropped into a panel.
		auto edit_row = [&](const char* label, char* buf, int buf_sz, auto apply) {
			nm::text_at(ImVec2(p0.x + 13.f, y + 4.f), label, nm::style().caption,
				ImVec4(row_text.x, row_text.y, row_text.z, 0.55f));
			ImGui::SetCursorScreenPos(ImVec2(p0.x + 74.f, y));
			ImGui::PushItemWidth(p1.x - p0.x - 86.f);
			const bool committed = ImGui::InputText(label, buf, buf_sz, ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::PopItemWidth();
			if (committed)
				apply();
			y += 23.f;
		};

		if (y < p1.y - 40.f)
		{
			dl->AddLine(ImVec2(p0.x + 12.f, y), ImVec2(p1.x - 12.f, y), ImGui::GetColorU32(sep_col), 1.f);
			y += 7.f;

			if (is_value)
			{
				if (target.cls == "BoolValue")
				{
					nm::text_at(ImVec2(p0.x + 13.f, y + 4.f), "value", nm::style().caption,
						ImVec4(row_text.x, row_text.y, row_text.z, 0.55f));
					ImGui::SetCursorScreenPos(ImVec2(p0.x + 74.f, y));
					if (ImAdd::CheckBox("##pv_bool", &val_bool))
						value_base::set_bool(target.address, val_bool);
					y += 23.f;
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
					properties_info_row(dl, ImVec2(p0.x, y), p1.x - p0.x, "current", obj_buf);
					y += row_h;
					edit_row("address", obj_buf, sizeof(obj_buf), [&]() {
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
				edit_row("jump", hum_jp, sizeof(hum_jp), [&]() {
					memory->write<float>(target.address + Offsets::Humanoid::JumpPower, (float)atof(hum_jp));
				});
			}
			else
			{
				nm::text_at(ImVec2(p0.x + 13.f, y), "No editable fields for this class",
					nm::style().caption, ImVec4(row_text.x, row_text.y, row_text.z, 0.34f));
			}
		}

		// No scroll here on purpose: the body's rows are positions, not ImGui
		// items, so a scroll would move the widgets and leave the text behind.
		// The card is sized for the widest editor it can show instead.
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	// ---------------------------------------------------------------------
	// Game structure
	//
	// The reference trees give this its own card and let you point the cheat at
	// a container by name; this one also lists what was found, so a model can be
	// applied by clicking it, and typed by hand when detection guesses wrong.

	char gs_players_buf[64]   = "Players";
	char gs_character_buf[64] = "Character";
	bool gs_manual      = false;
	bool gs_auto_apply  = true;
	bool gs_detected    = false;
	std::uint64_t gs_place = 0;
	int  gs_picked      = -1;

	void gs_apply()
	{
		game_structure::players_name_override   = gs_players_buf;
		game_structure::character_name_override = gs_character_buf;
		game_structure::use_overrides           = gs_manual;
		game_structure::auto_apply.store(gs_auto_apply);
		game_structure::scan_requested.store(true);
	}

	void gs_rescan()
	{
		game_structure::info_t found = game_structure::detect();
		{
			std::lock_guard<std::mutex> lk(game_structure::mtx);
			game_structure::g_info = found;
		}
		gs_detected = true;
		snprintf(gs_players_buf, sizeof(gs_players_buf), "%s", found.players_name.c_str());
		snprintf(gs_character_buf, sizeof(gs_character_buf), "%s", found.character_name.c_str());
		gs_apply();
	}

	void draw_structure_card(ImVec2 p0, ImVec2 p1)
	{
		card_frame(p0, p1);
		ImDrawList* dl = ImGui::GetWindowDrawList();

		// The cache thread walks the structure on attach; take its answer rather
		// than running a second scan on the UI thread.  Rescan is the only path
		// that ever blocks.
		if (game_ctx::in_place && game_ctx::place_id != 0 && game_ctx::place_id != gs_place)
		{
			gs_place = game_ctx::place_id;
			gs_detected = false;
			gs_picked = -1;
		}

		game_structure::info_t cur;
		{
			std::lock_guard<std::mutex> lk(game_structure::mtx);
			cur = game_structure::g_info;
		}

		if (!gs_detected && game_ctx::in_place && (cur.valid || cur.players_container || cur.character_count > 0))
		{
			gs_detected = true;
			if (!cur.players_name.empty())
				snprintf(gs_players_buf, sizeof(gs_players_buf), "%s", cur.players_name.c_str());
			if (!cur.character_name.empty())
				snprintf(gs_character_buf, sizeof(gs_character_buf), "%s", cur.character_name.c_str());
		}

		char meta[48];
		if (!game_ctx::in_place)        snprintf(meta, sizeof meta, "waiting for a place");
		else if (!gs_detected)          snprintf(meta, sizeof meta, "scanning");
		else if (gs_manual)             snprintf(meta, sizeof meta, "manual");
		else if (gs_auto_apply)         snprintf(meta, sizeof meta, "applied");
		else                            snprintf(meta, sizeof meta, "detected");

		const float body_y = card_header(dl, p0, p1, icons::Id::Sitemap, "Game Structure", meta);

		// A coloured dot is the whole status: green when the paths are in use,
		// blue when they are hand-written, amber when detection is idle.
		{
			ImU32 dot = IM_COL32(120, 120, 128, 200);
			if (gs_detected) dot = gs_manual ? IM_COL32(120, 190, 255, 255)
			                                 : (gs_auto_apply ? IM_COL32(120, 220, 140, 255)
			                                                  : IM_COL32(240, 200, 110, 255));
			dl->AddCircleFilled(ImVec2(p0.x + 13.f, body_y + 12.f), 2.6f, dot, 10);
		}

		const float pad = 13.f;
		const float btn_h = 21.f;

		// Rescan sits in the header row, right-aligned and quiet.
		if (soft_button("##gs_rescan", "Rescan",
				ImVec2(p1.x - pad - 58.f, body_y + 5.f), ImVec2(58.f, btn_h), false))
			gs_rescan();

		float y = body_y + 22.f;

		// Two stats per row rather than a label column: at this width a
		// "label : value" line spends half its ink on the label.
		auto stat = [&](float x, float w, const char* label, const std::string& value, ImU32 col) {
			nm::text_at(ImVec2(x, y), label, nm::style().caption,
				ImVec4(row_text.x, row_text.y, row_text.z, 0.38f));
			nm::text_clipped(ImVec2(x, y + 11.f), value.c_str(), nm::style().caption,
				ImGui::ColorConvertU32ToFloat4(col), w);
		};

		const float half = (p1.x - p0.x - pad * 2.f - 10.f) * 0.5f;
		const std::string players_v = gs_detected
			? (cur.players_path.empty() ? cur.players_name : cur.players_path)
			: "-";
		const std::string rigs_v = gs_detected
			? (std::to_string(cur.character_count) + " found")
			: "-";
		stat(p0.x + pad, half, "players", players_v, IM_COL32(225, 225, 230, 255));
		stat(p0.x + pad + half + 10.f, half, "rigs", rigs_v,
			cur.character_count > 0 ? IM_COL32(150, 220, 160, 255) : IM_COL32(190, 190, 196, 255));
		y += 27.f;

		const std::string rig_v = gs_detected ? (cur.rig.empty() ? "-" : cur.rig) : "-";
		const std::string parts_v = gs_detected
			? (std::to_string(cur.part_count) + " parts")
			: "-";
		stat(p0.x + pad, half, "rig", rig_v, IM_COL32(205, 205, 212, 255));
		stat(p0.x + pad + half + 10.f, half, "parts", parts_v, IM_COL32(205, 205, 212, 255));
		y += 27.f;

		// --- the model list -------------------------------------------------
		dl->AddLine(ImVec2(p0.x + pad, y), ImVec2(p1.x - pad, y), ImGui::GetColorU32(sep_col), 1.f);
		y += 6.f;

		nm::text_at(ImVec2(p0.x + pad, y), "CHARACTER MODEL", nm::style().caption,
			ImVec4(row_text.x, row_text.y, row_text.z, 0.34f));

		const float list_top = y + 13.f;
		// The override block below owns the last 84 px, and it must not be
		// overlapped whatever the card's height is: the list takes what is left.
		const float list_h = ImClamp((p1.y - 103.f) - list_top, 22.f, 112.f);

		ImGui::SetCursorScreenPos(ImVec2(p0.x + pad, list_top));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::BeginChild("##gs_models", ImVec2(p1.x - p0.x - pad * 2.f, list_h),
			ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

		const bool have_list = gs_detected && !cur.character_models.empty();
		if (!have_list)
		{
			nm::text_at(ImVec2(p0.x + pad + 1.f, list_top + 4.f),
				gs_detected ? "no rigs in this place" : "nothing detected yet",
				nm::style().caption, ImVec4(row_text.x, row_text.y, row_text.z, 0.32f));
		}
		else
		{
			for (int i = 0; i < (int)cur.character_models.size(); ++i)
			{
				const auto& m = cur.character_models[(size_t)i];
				const ImVec2 rp = ImGui::GetCursorScreenPos();
				const float rw = ImGui::GetContentRegionAvail().x;
				const bool picked = (gs_picked == i);

				ImGui::PushID(i);
				const bool clicked = ImGui::Selectable("##m", picked, 0, ImVec2(rw, 19.f));
				const bool hovered = ImGui::IsItemHovered();
				ImGui::PopID();

				if (hovered && !picked)
					dl->AddRectFilled(rp, ImVec2(rp.x + rw, rp.y + 19.f),
						ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.05f)), 5.f);
				if (picked)
				{
					dl->AddRectFilled(rp, ImVec2(rp.x + rw, rp.y + 19.f), with_alpha(row_accent, 0.13f), 5.f);
					dl->AddRectFilled(ImVec2(rp.x, rp.y + 5.f), ImVec2(rp.x + 1.5f, rp.y + 14.f),
						with_alpha(row_accent, 0.65f), 1.f);
				}

				aether_icons::draw_icon(dl, "Model", ImVec2(rp.x + 6.f, rp.y + 2.5f), 13.f);
				nm::text_clipped(ImVec2(rp.x + 24.f, rp.y + 3.f), m.name.c_str(), nm::style().caption,
					picked ? ImVec4(1.f, 1.f, 1.f, 1.f) : row_text, rw - 78.f);

				char parts[24];
				snprintf(parts, sizeof parts, "%d parts", m.rig_parts);
				nm::text_at(ImVec2(rp.x + rw - 50.f, rp.y + 3.f), parts, nm::style().caption,
					ImVec4(row_text.x, row_text.y, row_text.z, 0.38f));

				if (clicked)
				{
					gs_picked = i;
					snprintf(gs_character_buf, sizeof(gs_character_buf), "%s", m.name.c_str());
					gs_manual = true;
					gs_apply();
				}
			}
		}

		aether::ui::smooth_scroll(settings::appearance::smooth_scroll, settings::appearance::scroll_speed);
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		// --- the manual paths ----------------------------------------------
		float my = p1.y - 84.f;

		nm::text_at(ImVec2(p0.x + pad, my), "OVERRIDE", nm::style().caption,
			ImVec4(row_text.x, row_text.y, row_text.z, 0.34f));
		my += 13.f;

		auto field = [&](const char* id, const char* hint, char* buf, int cap, float w) {
			ImGui::SetCursorScreenPos(ImVec2(p0.x + pad, my));
			ImGui::PushID(id);
			ImGui::PushItemWidth(w);
			const bool changed = ImGui::InputTextWithHint("##f", hint, buf, cap);
			ImGui::PopItemWidth();
			ImGui::PopID();
			return changed;
		};

		const float field_w = p1.x - p0.x - pad * 2.f;
		const bool pl_changed = field("players", "players container", gs_players_buf, sizeof(gs_players_buf), field_w);
		my += 23.f;
		const bool ch_changed = field("model", "character model", gs_character_buf, sizeof(gs_character_buf), field_w);
		my += 24.f;

		if (pl_changed || ch_changed)
		{
			gs_manual = true;
			gs_apply();
		}

		const float half_btn = (field_w - 8.f) * 0.5f;
		if (soft_button("##gs_applybtn", "Apply paths",
				ImVec2(p0.x + pad, my), ImVec2(half_btn, 21.f), true))
		{
			gs_manual = true;
			gs_apply();
		}

		// The auto switch is the quiet one: it says whether detected paths are
		// used without asking, and it is a pill because a checkbox at this size
		// is a speck.
		{
			const ImVec2 sw(p0.x + pad + half_btn + 8.f, my);
			ImGui::SetCursorScreenPos(sw);
			ImGui::InvisibleButton("##gs_auto", ImVec2(field_w - half_btn - 8.f, 21.f));
			const bool clicked = ImGui::IsItemClicked();
			const bool hovered = ImGui::IsItemHovered();
			if (clicked)
			{
				gs_auto_apply = !gs_auto_apply;
				gs_apply();
			}

			const float pw = 26.f, ph = 14.f;
			const ImVec2 a(sw.x + 2.f, sw.y + (21.f - ph) * 0.5f);
			const ImVec2 b(a.x + pw, a.y + ph);
			dl->AddRectFilled(a, b, gs_auto_apply ? with_alpha(row_accent, 0.55f)
			                                     : ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, hovered ? 0.14f : 0.09f)),
				ph * 0.5f);
			const float kx = gs_auto_apply ? b.x - ph * 0.5f - 1.5f : a.x + ph * 0.5f + 1.5f;
			dl->AddCircleFilled(ImVec2(kx, a.y + ph * 0.5f), ph * 0.5f - 2.f,
				IM_COL32(246, 246, 250, 245), 12);

			nm::text_at(ImVec2(b.x + 8.f, sw.y + 4.f), "auto-apply", nm::style().caption,
				ImVec4(row_text.x, row_text.y, row_text.z, gs_auto_apply ? 0.85f : 0.45f));
		}
	}

	// ---------------------------------------------------------------------
	// The workspace

	void draw_workspace(ImVec2 wp, ImVec2 ws)
	{
		refresh_glass_colors();
		ensure_root();

		const float gap       = 8.f;
		const float toolbar_h = 30.f;

		// The page's own inner width, not the size it was handed: the content
		// window has 14 px of padding on each side, and filling to the passed
		// width put the right-hand cards under that padding - visible as a card
		// whose border was cut off at the panel edge.
		ImGui::SetCursorScreenPos(wp);
		const float full_w = ImGui::GetContentRegionAvail().x;
		const float ws_x   = full_w > 120.f ? full_w : ws.x;

		// ---- the search field, full width, on its own line ---------------
		ImGui::SetCursorScreenPos(wp);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.f, 1.f, 1.f, 0.04f));
		ImGui::PushStyleColor(ImGuiCol_Border, sep_col);
		ImGui::SetNextItemWidth(ws_x);
		const bool changed = ImGui::InputTextWithHint("##explorer_search",
			"Search the datamodel - instance name or class", g_ws.search, sizeof(g_ws.search));
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(2);

		{
			ImDrawList* sdl = ImGui::GetWindowDrawList();
			const ImVec2 rmin = ImGui::GetItemRectMin();
			const ImVec2 rmax = ImGui::GetItemRectMax();
			const float ico = 14.f;
			icons::draw(sdl, icons::Id::Search,
				ImVec2(rmax.x - ico * 0.5f - 9.f, (rmin.y + rmax.y) * 0.5f), ico,
				ImGui::GetColorU32(g_ws.search[0] ? ImGuiCol_Text : ImGuiCol_TextDisabled), 0.f);
		}

		if (ImGui::IsItemDeactivatedAfterEdit() && g_ws.search[0])
			start_search(g_ws.search);
		if (changed && g_ws.search[0] == '\0' && g_search_on)
			stop_search();
		if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false) && g_ws.search[0])
			start_search(g_ws.search);

		const float body_y = wp.y + toolbar_h + gap;
		const float body_h = ws.y - toolbar_h - gap;

		// Below this there is no room for a second column that is worth reading,
		// so the tree takes the page and the cards step aside instead of being
		// squeezed into a strip with a scrollbar.
		const bool roomy = (body_h >= 245.f) && (ws_x >= 585.f);

		float right_w = ImClamp(ws_x * 0.40f, 244.f, 330.f);
		if (ws_x - right_w - gap < 240.f)
			right_w = ImMax(200.f, ws_x - gap - 240.f);
		const float left_w = roomy ? (ws_x - right_w - gap) : ws_x;

		// ---- datamodel ----------------------------------------------------
		const ImVec2 l0(wp.x, body_y);
		const ImVec2 l1(wp.x + left_w, body_y + body_h);
		card_frame(l0, l1);

		{
			ImDrawList* dl = ImGui::GetWindowDrawList();
			char meta[48];
			if (g_search_on)
			{
				if (g_searching.load()) snprintf(meta, sizeof meta, "searching");
				else snprintf(meta, sizeof meta, "%d found", (int)g_search_results.size());
			}
			else
			{
				int roots = g_root.loaded ? (int)g_root.children.size() : 0;
				if (g_root.loaded) snprintf(meta, sizeof meta, "%d services", roots);
				else               snprintf(meta, sizeof meta, "reading");
			}

			const float body_top = card_header(dl, l0, l1, icons::Id::Tree, "Datamodel", meta);

			ImGui::SetCursorScreenPos(ImVec2(l0.x + 1.f, body_top + 1.f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.f, 5.f));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::BeginChild("##explorer_tree", ImVec2(l1.x - l0.x - 2.f, l1.y - body_top - 2.f),
				ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

			ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.f, 0.f, 0.f, 0.f));

			if (g_search_on)
			{
				std::lock_guard<std::mutex> lk(g_search_mtx);
				if (g_searching.load())
					nm::text_at(ImVec2(ImGui::GetCursorScreenPos().x + 4.f, ImGui::GetCursorScreenPos().y + 3.f),
						"searching...", nm::style().caption,
						ImVec4(row_text.x, row_text.y, row_text.z, 0.45f));
				else if (g_search_results.empty())
					nm::text_at(ImVec2(ImGui::GetCursorScreenPos().x + 4.f, ImGui::GetCursorScreenPos().y + 3.f),
						"no matches", nm::style().caption,
						ImVec4(row_text.x, row_text.y, row_text.z, 0.45f));

				ImGuiListClipper clipper;
				clipper.Begin((int)g_search_results.size(), 22.f);
				while (clipper.Step())
				{
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
						render_search_row(g_search_results[(size_t)i], i);
				}
			}
			else if (g_root.address)
			{
				if (!g_root.loaded)
					load_children(g_root);
				for (auto& c : g_root.children)
					render_node(c, 0);
			}
			else
			{
				nm::text_at(ImVec2(ImGui::GetCursorScreenPos().x + 4.f, ImGui::GetCursorScreenPos().y + 3.f),
					"not attached - no datamodel yet", nm::style().caption,
					ImVec4(row_text.x, row_text.y, row_text.z, 0.45f));
			}

			ImGui::PopStyleColor(3);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_PopupBg, glass_title);
			ImGui::PushStyleColor(ImGuiCol_Border, border_outer);
			ImGui::SetNextWindowPos(g_ws.ctx_pos, ImGuiCond_Appearing);
			if (ImGui::BeginPopup("##ex_ctx_menu"))
			{
				if (g_ws.ctx.valid)
				{
					ImDrawList* pdl = ImGui::GetWindowDrawList();
					constexpr float row_h = 24.f;
					constexpr float pad_x = 11.f;
					const float w = 174.f;
					const ImVec2 pwp = ImGui::GetWindowPos();

					float ht = 8.f;
					const float name_h = ImGui::GetTextLineHeight();
					const char* nm = g_ws.ctx.name.empty() ? "?" : g_ws.ctx.name.c_str();
					nm::text_clipped(ImVec2(pwp.x + pad_x, pwp.y + ht), nm,
						nm::style().caption, nm::style().text, w - pad_x * 2.f);
					nm::text_clipped(ImVec2(pwp.x + pad_x, pwp.y + ht + name_h + 2.f),
						g_ws.ctx.cls.c_str(), nm::style().caption,
						ImVec4(row_text.x, row_text.y, row_text.z, 0.45f), w - pad_x * 2.f);
					const float ctx_header_h = ht + name_h * 2.f + 2.f + 8.f;
					pdl->AddLine(ImVec2(pwp.x, pwp.y + ctx_header_h),
						ImVec2(pwp.x + w, pwp.y + ctx_header_h), ImGui::GetColorU32(sep_col));
					ImGui::SetCursorPos(ImVec2(0.f, ctx_header_h));

					auto ctx_row = [&](const char* label) -> bool {
						ImGui::PushID(label);
						const ImVec2 rp = ImGui::GetCursorScreenPos();
						const ImVec2 rmax(rp.x + w, rp.y + row_h);
						const bool row_clicked = ImGui::Selectable("##r", false, 0, ImVec2(w, row_h));
						const bool hov = ImGui::IsItemHovered();
						ImGui::PopID();
						if (hov)
							pdl->AddRectFilled(rp, rmax, ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.07f)));
						nm::text_at(ImVec2(rp.x + pad_x, rp.y + (row_h - ImGui::GetTextLineHeight()) * 0.5f),
							label, nm::style().caption, row_text);
						return row_clicked;
					};

					if (ctx_row("properties"))
					{
						fill_prop(g_ws.prop, g_ws.ctx.address, g_ws.ctx.name, g_ws.ctx.cls,
							g_ws.ctx.path, g_ws.ctx.children);
						g_ws.prop_open = true;
						ImGui::CloseCurrentPopup();
					}
					if (ctx_row("copy name"))
					{
						ImGui::SetClipboardText(g_ws.ctx.name.c_str());
						ImGui::CloseCurrentPopup();
					}
					if (ctx_row("copy class"))
					{
						ImGui::SetClipboardText(g_ws.ctx.cls.c_str());
						ImGui::CloseCurrentPopup();
					}
					if (ctx_row("copy path"))
					{
						ImGui::SetClipboardText(g_ws.ctx.path.c_str());
						ImGui::CloseCurrentPopup();
					}
					if (ctx_row("copy address"))
					{
						char buf[32];
						snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)g_ws.ctx.address);
						ImGui::SetClipboardText(buf);
						ImGui::CloseCurrentPopup();
					}
					if (ctx_row("teleport here") && g_ws.ctx.address)
					{
						try {
							rbx::part_t dst(g_ws.ctx.address);
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
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}

		// ---- the right column ---------------------------------------------
		if (!roomy)
			return;

		const float rx = wp.x + left_w + gap;
		// Half each, with the structure card held at the height its own rows need:
		// below this its model list and override fields start fighting, and the
		// properties card pays for it with a clipped editor.
		const float struct_h = ImClamp(body_h * 0.50f, 250.f, ImMax(250.f, body_h - 210.f));

		draw_structure_card(ImVec2(rx, body_y), ImVec2(rx + right_w, body_y + struct_h));
		draw_properties_card(ImVec2(rx, body_y + struct_h + gap),
			ImVec2(rx + right_w, body_y + body_h));
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
	g_ws = workspace_state{};
}

void page(ImVec2 pos, ImVec2 size)
{
	if (size.x < 120.f || size.y < 80.f)
		return;

	ScopedWidgetStyle widget_style;
	draw_workspace(pos, size);
}

}
