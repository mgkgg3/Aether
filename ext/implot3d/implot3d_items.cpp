
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "implot3d.h"
#include "implot3d_internal.h"

#ifndef IMGUI_DISABLE

#define SQRT_1_2 0.70710678118f
#define SQRT_3_2 0.86602540378f

#ifndef IMPLOT3D_NO_FORCE_INLINE
    #ifdef _MSC_VER
        #define IMPLOT3D_INLINE __forceinline
    #elif defined(__GNUC__)
        #define IMPLOT3D_INLINE inline __attribute__((__always_inline__))
    #elif defined(__CLANG__)
        #if __has_attribute(__always_inline__)
            #define IMPLOT3D_INLINE inline __attribute__((__always_inline__))
        #else
            #define IMPLOT3D_INLINE inline
        #endif
    #else
        #define IMPLOT3D_INLINE inline
    #endif
#else
    #define IMPLOT3D_INLINE inline
#endif

#define IMPLOT3D_NORMALIZE2F(VX, VY)                                                                                                                 \
    do {                                                                                                                                             \
        float d2 = VX * VX + VY * VY;                                                                                                                \
        if (d2 > 0.0f) {                                                                                                                             \
            float inv_len = ImRsqrt(d2);                                                                                                             \
            VX *= inv_len;                                                                                                                           \
            VY *= inv_len;                                                                                                                           \
        }                                                                                                                                            \
    } while (0)

IMPLOT3D_INLINE void GetLineRenderProps(const ImDrawList3D& draw_list_3d, float& half_weight, ImVec2& tex_uv0, ImVec2& tex_uv1) {
    const bool aa = ImPlot3D::ImHasFlag(draw_list_3d._Flags, ImDrawListFlags_AntiAliasedLines) &&
                    ImPlot3D::ImHasFlag(draw_list_3d._Flags, ImDrawListFlags_AntiAliasedLinesUseTex);
    if (aa) {
        ImVec4 tex_uvs = draw_list_3d._SharedData->TexUvLines[(int)(half_weight * 2)];
        tex_uv0 = ImVec2(tex_uvs.x, tex_uvs.y);
        tex_uv1 = ImVec2(tex_uvs.z, tex_uvs.w);
        half_weight += 1;
    } else {
        tex_uv0 = tex_uv1 = draw_list_3d._SharedData->TexUvWhitePixel;
    }
}

#ifdef IMPLOT3D_CUSTOM_NUMERIC_TYPES
#define IMPLOT3D_NUMERIC_TYPES IMPLOT3D_CUSTOM_NUMERIC_TYPES
#else
#define IMPLOT3D_NUMERIC_TYPES (ImS8)(ImU8)(ImS16)(ImU16)(ImS32)(ImU32)(ImS64)(ImU64)(float)(double)
#endif

#define _CAT(x, y) _CAT_(x, y)
#define _CAT_(x, y) x##y
#define _INSTANTIATE_FOR_NUMERIC_TYPES(chain) _CAT(_INSTANTIATE_FOR_NUMERIC_TYPES_1 chain, _END)
#define _INSTANTIATE_FOR_NUMERIC_TYPES_1(T) INSTANTIATE_MACRO(T) _INSTANTIATE_FOR_NUMERIC_TYPES_2
#define _INSTANTIATE_FOR_NUMERIC_TYPES_2(T) INSTANTIATE_MACRO(T) _INSTANTIATE_FOR_NUMERIC_TYPES_1
#define _INSTANTIATE_FOR_NUMERIC_TYPES_1_END
#define _INSTANTIATE_FOR_NUMERIC_TYPES_2_END
#define CALL_INSTANTIATE_FOR_NUMERIC_TYPES() _INSTANTIATE_FOR_NUMERIC_TYPES(IMPLOT3D_NUMERIC_TYPES)

namespace ImPlot3D {

static const float ITEM_HIGHLIGHT_LINE_SCALE = 2.0f;
static const float ITEM_HIGHLIGHT_MARK_SCALE = 1.25f;

template <typename T> int Stride(const ImPlot3DSpec& spec) { return spec.Stride == IMPLOT3D_AUTO ? sizeof(T) : spec.Stride; }

bool BeginItem(const char* label_id, const ImPlot3DSpec& spec, const ImVec4& item_col, ImPlot3DMarker item_mkr) {
    ImPlot3DContext& gp = *GImPlot3D;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != nullptr, "PlotX() needs to be called between BeginPlot() and EndPlot()!");

    SetupLock();

    ImPlot3DStyle& style = gp.Style;
    ImPlot3DNextItemData& n = gp.NextItemData;
    ImPlot3DSpec& s = n.Spec;
    s = spec;

    bool just_created;
    ImPlot3DItem* item = RegisterOrGetItem(label_id, spec.Flags, &just_created);

    gp.CurrentItem = item;

    if (!IsColorAuto(item_col)) {
        item->Color = ImGui::ColorConvertFloat4ToU32(item_col);
    } else if (just_created) {
        item->Color = NextColormapColorU32();
    }

    if (item_mkr != ImPlot3DMarker_Invalid) {
        if (item_mkr != ImPlot3DMarker_Auto) {
            item->Marker = item_mkr;
        } else if (just_created && item_mkr == ImPlot3DMarker_Auto) {
            item->Marker = NextMarker();
        } else if (item_mkr == ImPlot3DMarker_Auto && item->Marker == ImPlot3DMarker_None) {
            item->Marker = NextMarker();
        }
    }

    ImVec4 item_color = ImGui::ColorConvertU32ToFloat4(item->Color);
    n.IsAutoLine = IsColorAuto(s.LineColor);
    n.IsAutoFill = IsColorAuto(s.FillColor);
    s.LineColor = IsColorAuto(s.LineColor) ? item_color : s.LineColor;
    s.FillColor = IsColorAuto(s.FillColor) ? item_color : s.FillColor;
    s.MarkerLineColor = IsColorAuto(s.MarkerLineColor) ? s.LineColor : s.MarkerLineColor;
    s.MarkerFillColor = IsColorAuto(s.MarkerFillColor) ? s.LineColor : s.MarkerFillColor;

    s.LineWeight = s.LineWeight < 0.0f ? style.LineWeight : s.LineWeight;
    s.Marker = s.Marker < 0 ? style.Marker : s.Marker;
    s.MarkerSize = s.MarkerSize < 0.0f ? style.MarkerSize : s.MarkerSize;
    s.FillAlpha = s.FillAlpha < 0 ? gp.Style.FillAlpha : s.FillAlpha;

    s.FillColor.w *= s.FillAlpha;
    s.MarkerFillColor.w *= s.FillAlpha;

    n.RenderLine = s.LineColor.w > 0 && s.LineWeight > 0;
    n.RenderFill = s.FillColor.w > 0;
    n.RenderMarkerLine = s.LineColor.w > 0 && s.LineWeight > 0;
    n.RenderMarkerFill = s.FillColor.w > 0;

    if (!item->Show) {
        EndItem();
        return false;
    } else {

        if (item->LegendHovered) {
            if (!ImHasFlag(gp.CurrentItems->Legend.Flags, ImPlot3DLegendFlags_NoHighlightItem)) {
                s.LineWeight *= ITEM_HIGHLIGHT_LINE_SCALE;
                s.MarkerSize *= ITEM_HIGHLIGHT_MARK_SCALE;
            }
        }
    }

    return true;
}

template <typename _Getter> bool BeginItemEx(const char* label_id, const _Getter& getter, const ImPlot3DSpec& spec,
                                             const ImVec4& item_col = IMPLOT3D_AUTO_COL, ImPlot3DMarker item_mkr = ImPlot3DMarker_Invalid) {
    if (BeginItem(label_id, spec, item_col, item_mkr)) {
        ImPlot3DContext& gp = *GImPlot3D;
        ImPlot3DPlot& plot = *gp.CurrentPlot;
        if (plot.FitThisFrame && !ImHasFlag(spec.Flags, ImPlot3DItemFlags_NoFit)) {
            for (int i = 0; i < getter.Count; i++)
                plot.ExtendFit(getter(i));
        }
        return true;
    }
    return false;
}

void EndItem() {
    ImPlot3DContext& gp = *GImPlot3D;
    gp.NextItemData.Reset();
    gp.CurrentItem = nullptr;
}

ImPlot3DItem* RegisterOrGetItem(const char* label_id, ImPlot3DItemFlags flags, bool* just_created) {
    ImPlot3DContext& gp = *GImPlot3D;
    ImPlot3DItemGroup& Items = *gp.CurrentItems;
    ImGuiID id = Items.GetItemID(label_id);
    if (just_created != nullptr)
        *just_created = Items.GetItem(id) == nullptr;
    ImPlot3DItem* item = Items.GetOrAddItem(id);

    if (item->SeenThisFrame)
        return item;
    item->SeenThisFrame = true;

    int idx = Items.GetItemIndex(item);
    item->ID = id;
    if (!ImHasFlag(flags, ImPlot3DItemFlags_NoLegend) && ImGui::FindRenderedTextEnd(label_id, nullptr) != label_id) {
        Items.Legend.Indices.push_back(idx);
        item->NameOffset = Items.Legend.Labels.size();
        Items.Legend.Labels.append(label_id, label_id + strlen(label_id) + 1);
    }
    return item;
}

ImPlot3DItem* GetCurrentItem() {
    ImPlot3DContext& gp = *GImPlot3D;
    return gp.CurrentItem;
}

void BustItemCache() {
    ImPlot3DContext& gp = *GImPlot3D;
    for (int p = 0; p < gp.Plots.GetBufSize(); ++p) {
        ImPlot3DPlot& plot = *gp.Plots.GetByIndex(p);
        plot.Items.Reset();
    }
}

IMPLOT3D_INLINE void PrimLine(ImDrawList3D& draw_list_3d, const ImVec2& P1, const ImVec2& P2, float half_weight, ImU32 col, const ImVec2& tex_uv0,
                              const ImVec2& tex_uv1, double z) {
    float dx = P2.x - P1.x;
    float dy = P2.y - P1.y;
    IMPLOT3D_NORMALIZE2F(dx, dy);
    dx *= half_weight;
    dy *= half_weight;
    draw_list_3d._VtxWritePtr[0].pos.x = P1.x + dy;
    draw_list_3d._VtxWritePtr[0].pos.y = P1.y - dx;
    draw_list_3d._VtxWritePtr[0].uv = tex_uv0;
    draw_list_3d._VtxWritePtr[0].col = col;
    draw_list_3d._VtxWritePtr[1].pos.x = P2.x + dy;
    draw_list_3d._VtxWritePtr[1].pos.y = P2.y - dx;
    draw_list_3d._VtxWritePtr[1].uv = tex_uv0;
    draw_list_3d._VtxWritePtr[1].col = col;
    draw_list_3d._VtxWritePtr[2].pos.x = P2.x - dy;
    draw_list_3d._VtxWritePtr[2].pos.y = P2.y + dx;
    draw_list_3d._VtxWritePtr[2].uv = tex_uv1;
    draw_list_3d._VtxWritePtr[2].col = col;
    draw_list_3d._VtxWritePtr[3].pos.x = P1.x - dy;
    draw_list_3d._VtxWritePtr[3].pos.y = P1.y + dx;
    draw_list_3d._VtxWritePtr[3].uv = tex_uv1;
    draw_list_3d._VtxWritePtr[3].col = col;
    draw_list_3d._VtxWritePtr += 4;
    draw_list_3d._IdxWritePtr[0] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
    draw_list_3d._IdxWritePtr[1] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 1);
    draw_list_3d._IdxWritePtr[2] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);
    draw_list_3d._IdxWritePtr[3] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
    draw_list_3d._IdxWritePtr[4] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);
    draw_list_3d._IdxWritePtr[5] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 3);
    draw_list_3d._IdxWritePtr += 6;
    draw_list_3d._VtxCurrentIdx += 4;
    draw_list_3d._ZWritePtr[0] = z;
    draw_list_3d._ZWritePtr[1] = z;
    draw_list_3d._ZWritePtr += 2;
}

double GetPointDepth(ImPlot3DPoint p) {
    ImPlot3DContext& gp = *GImPlot3D;
    ImPlot3DPlot& plot = *gp.CurrentPlot;

    if (ImHasFlag(plot.Axes[0].Flags, ImPlot3DAxisFlags_Invert))
        p.x = -p.x;
    if (ImHasFlag(plot.Axes[1].Flags, ImPlot3DAxisFlags_Invert))
        p.y = -p.y;
    if (ImHasFlag(plot.Axes[2].Flags, ImPlot3DAxisFlags_Invert))
        p.z = -p.z;

    ImPlot3DPoint p_rot = plot.Rotation * p;
    return p_rot.z;
}

struct RendererBase {
    RendererBase(int prims, int idx_consumed, int vtx_consumed) : Prims(prims), IdxConsumed(idx_consumed), VtxConsumed(vtx_consumed) {}
    const unsigned int Prims;
    const unsigned int IdxConsumed;
    const unsigned int VtxConsumed;
};

template <class _Getter, class _GetterColor, class _GetterSize> struct RendererMarkersFill : RendererBase {
    RendererMarkersFill(const _Getter& getter, const _GetterColor& col_getter, const _GetterSize& size_getter, const ImVec2* marker, int count)
        : RendererBase(getter.Count, (count - 2) * 3, count), Getter(getter), ColGetter(col_getter), SizeGetter(size_getter), Marker(marker),
          Count(count) {}

    void Init(ImDrawList3D& draw_list_3d) const { UV = draw_list_3d._SharedData->TexUvWhitePixel; }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {
        ImPlot3DPoint p_plot = Getter(prim);
        if (!cull_box.Contains(p_plot))
            return false;
        ImVec2 p = PlotToPixels(p_plot);
        ImU32 col = ColGetter(prim);
        float size = SizeGetter(prim);

        for (int i = 0; i < Count; i++) {
            draw_list_3d._VtxWritePtr[0].pos.x = p.x + Marker[i].x * size;
            draw_list_3d._VtxWritePtr[0].pos.y = p.y + Marker[i].y * size;
            draw_list_3d._VtxWritePtr[0].uv = UV;
            draw_list_3d._VtxWritePtr[0].col = col;
            draw_list_3d._VtxWritePtr++;
        }

        for (int i = 2; i < Count; i++) {

            draw_list_3d._IdxWritePtr[0] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
            draw_list_3d._IdxWritePtr[1] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + i - 1);
            draw_list_3d._IdxWritePtr[2] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + i);
            draw_list_3d._IdxWritePtr += 3;

            draw_list_3d._ZWritePtr[0] = GetPointDepth(p_plot);
            draw_list_3d._ZWritePtr++;
        }

        draw_list_3d._VtxCurrentIdx += (ImDrawIdx)Count;
        return true;
    }
    const _Getter& Getter;
    const _GetterColor ColGetter;
    const _GetterSize SizeGetter;
    const ImVec2* Marker;
    const int Count;
    mutable ImVec2 UV;
};

template <class _Getter, class _GetterColor, class _GetterSize> struct RendererMarkersLine : RendererBase {
    RendererMarkersLine(const _Getter& getter, const _GetterColor& col_getter, const _GetterSize& size_getter, const ImVec2* marker, int count,
                        float weight)
        : RendererBase(getter.Count, count / 2 * 6, count / 2 * 4), Getter(getter), ColGetter(col_getter), SizeGetter(size_getter), Marker(marker),
          Count(count), HalfWeight(ImMax(1.0f, weight) * 0.5f) {}

    void Init(ImDrawList3D& draw_list_3d) const { GetLineRenderProps(draw_list_3d, HalfWeight, UV0, UV1); }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {
        ImPlot3DPoint p_plot = Getter(prim);
        if (!cull_box.Contains(p_plot))
            return false;
        ImVec2 p = PlotToPixels(p_plot);
        ImU32 col = ColGetter(prim);
        float size = SizeGetter(prim);
        for (int i = 0; i < Count; i = i + 2) {
            ImVec2 p1(p.x + Marker[i].x * size, p.y + Marker[i].y * size);
            ImVec2 p2(p.x + Marker[i + 1].x * size, p.y + Marker[i + 1].y * size);
            PrimLine(draw_list_3d, p1, p2, HalfWeight, col, UV0, UV1, GetPointDepth(p_plot));
        }
        return true;
    }

    const _Getter& Getter;
    const _GetterColor ColGetter;
    const _GetterSize SizeGetter;
    const ImVec2* Marker;
    const int Count;
    mutable float HalfWeight;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter, class _GetterColor> struct RendererLineStrip : RendererBase {
    RendererLineStrip(const _Getter& getter, const _GetterColor& col_getter, float weight)
        : RendererBase(getter.Count - 1, 6, 4), Getter(getter), ColGetter(col_getter), HalfWeight(ImMax(1.0f, weight) * 0.5f) {

        P1_plot = Getter(0);
    }

    void Init(ImDrawList3D& draw_list_3d) const { GetLineRenderProps(draw_list_3d, HalfWeight, UV0, UV1); }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {
        ImPlot3DPoint P2_plot = Getter(prim + 1);

        ImPlot3DPoint P1_clipped, P2_clipped;
        bool visible = cull_box.ClipLineSegment(P1_plot, P2_plot, P1_clipped, P2_clipped);

        if (visible) {

            ImVec2 P1_screen = PlotToPixels(P1_clipped);
            ImVec2 P2_screen = PlotToPixels(P2_clipped);

            PrimLine(draw_list_3d, P1_screen, P2_screen, HalfWeight, ColGetter(prim), UV0, UV1, GetPointDepth((P1_plot + P2_plot) * 0.5));
        }

        P1_plot = P2_plot;

        return visible;
    }

    const _Getter& Getter;
    const _GetterColor ColGetter;
    mutable float HalfWeight;
    mutable ImPlot3DPoint P1_plot;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter, class _GetterColor> struct RendererLineStripSkip : RendererBase {
    RendererLineStripSkip(const _Getter& getter, const _GetterColor& col_getter, float weight)
        : RendererBase(getter.Count - 1, 6, 4), Getter(getter), ColGetter(col_getter), HalfWeight(ImMax(1.0f, weight) * 0.5f) {

        P1_plot = Getter(0);
    }

    void Init(ImDrawList3D& draw_list_3d) const { GetLineRenderProps(draw_list_3d, HalfWeight, UV0, UV1); }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {

        ImPlot3DPoint P2_plot = Getter(prim + 1);
        bool visible = false;

        if (!ImNan(P1_plot.x) && !ImNan(P1_plot.y) && !ImNan(P1_plot.z) && !ImNan(P2_plot.x) && !ImNan(P2_plot.y) && !ImNan(P2_plot.z)) {

            ImPlot3DPoint P1_clipped, P2_clipped;
            visible = cull_box.ClipLineSegment(P1_plot, P2_plot, P1_clipped, P2_clipped);

            if (visible) {

                ImVec2 P1_screen = PlotToPixels(P1_clipped);
                ImVec2 P2_screen = PlotToPixels(P2_clipped);

                PrimLine(draw_list_3d, P1_screen, P2_screen, HalfWeight, ColGetter(prim), UV0, UV1, GetPointDepth((P1_plot + P2_plot) * 0.5));
            }
        }

        if (!ImNan(P2_plot.x) && !ImNan(P2_plot.y) && !ImNan(P2_plot.z))
            P1_plot = P2_plot;

        return visible;
    }

    const _Getter& Getter;
    const _GetterColor ColGetter;
    mutable float HalfWeight;
    mutable ImPlot3DPoint P1_plot;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter, class _GetterColor> struct RendererLineSegments : RendererBase {
    RendererLineSegments(const _Getter& getter, const _GetterColor& col_getter, float weight)
        : RendererBase(getter.Count / 2, 6, 4), Getter(getter), ColGetter(col_getter), HalfWeight(ImMax(1.0f, weight) * 0.5f) {}

    void Init(ImDrawList3D& draw_list_3d) const { GetLineRenderProps(draw_list_3d, HalfWeight, UV0, UV1); }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {

        ImPlot3DPoint P1_plot = Getter(prim * 2 + 0);
        ImPlot3DPoint P2_plot = Getter(prim * 2 + 1);

        if (!ImNan(P1_plot.x) && !ImNan(P1_plot.y) && !ImNan(P1_plot.z) && !ImNan(P2_plot.x) && !ImNan(P2_plot.y) && !ImNan(P2_plot.z)) {

            ImPlot3DPoint P1_clipped, P2_clipped;
            bool visible = cull_box.ClipLineSegment(P1_plot, P2_plot, P1_clipped, P2_clipped);

            if (visible) {

                ImVec2 P1_screen = PlotToPixels(P1_clipped);
                ImVec2 P2_screen = PlotToPixels(P2_clipped);

                PrimLine(draw_list_3d, P1_screen, P2_screen, HalfWeight, ColGetter(prim), UV0, UV1, GetPointDepth((P1_plot + P2_plot) * 0.5));
            }
            return visible;
        }

        return false;
    }

    const _Getter& Getter;
    const _GetterColor ColGetter;
    mutable float HalfWeight;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter, class _GetterColor> struct RendererTriangleFill : RendererBase {
    RendererTriangleFill(const _Getter& getter, const _GetterColor& col_getter)
        : RendererBase(getter.Count / 3, 3, 3), Getter(getter), ColGetter(col_getter) {}

    void Init(ImDrawList3D& draw_list_3d) const { UV = draw_list_3d._SharedData->TexUvWhitePixel; }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {
        ImPlot3DPoint p_plot[3];
        p_plot[0] = Getter(3 * prim);
        p_plot[1] = Getter(3 * prim + 1);
        p_plot[2] = Getter(3 * prim + 2);

        if (!cull_box.Contains(p_plot[0]) && !cull_box.Contains(p_plot[1]) && !cull_box.Contains(p_plot[2]))
            return false;

        ImVec2 p[3];
        p[0] = PlotToPixels(p_plot[0]);
        p[1] = PlotToPixels(p_plot[1]);
        p[2] = PlotToPixels(p_plot[2]);

        draw_list_3d._VtxWritePtr[0].pos.x = p[0].x;
        draw_list_3d._VtxWritePtr[0].pos.y = p[0].y;
        draw_list_3d._VtxWritePtr[0].uv = UV;
        draw_list_3d._VtxWritePtr[0].col = ColGetter(3 * prim);
        draw_list_3d._VtxWritePtr[1].pos.x = p[1].x;
        draw_list_3d._VtxWritePtr[1].pos.y = p[1].y;
        draw_list_3d._VtxWritePtr[1].uv = UV;
        draw_list_3d._VtxWritePtr[1].col = ColGetter(3 * prim + 1);
        draw_list_3d._VtxWritePtr[2].pos.x = p[2].x;
        draw_list_3d._VtxWritePtr[2].pos.y = p[2].y;
        draw_list_3d._VtxWritePtr[2].uv = UV;
        draw_list_3d._VtxWritePtr[2].col = ColGetter(3 * prim + 2);
        draw_list_3d._VtxWritePtr += 3;

        draw_list_3d._IdxWritePtr[0] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
        draw_list_3d._IdxWritePtr[1] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 1);
        draw_list_3d._IdxWritePtr[2] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);
        draw_list_3d._IdxWritePtr += 3;

        draw_list_3d._ZWritePtr[0] = GetPointDepth((p_plot[0] + p_plot[1] + p_plot[2]) / 3);
        draw_list_3d._ZWritePtr++;

        draw_list_3d._VtxCurrentIdx += 3;

        return true;
    }

    const _Getter& Getter;
    const _GetterColor ColGetter;
    mutable ImVec2 UV;
};

template <class _Getter, class _GetterColor> struct RendererQuadFill : RendererBase {
    RendererQuadFill(const _Getter& getter, const _GetterColor& col_getter)
        : RendererBase(getter.Count / 4, 6, 4), Getter(getter), ColGetter(col_getter) {}

    void Init(ImDrawList3D& draw_list_3d) const { UV = draw_list_3d._SharedData->TexUvWhitePixel; }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {
        ImPlot3DPoint p_plot[4];
        p_plot[0] = Getter(4 * prim);
        p_plot[1] = Getter(4 * prim + 1);
        p_plot[2] = Getter(4 * prim + 2);
        p_plot[3] = Getter(4 * prim + 3);

        if (!cull_box.Contains(p_plot[0]) && !cull_box.Contains(p_plot[1]) && !cull_box.Contains(p_plot[2]) && !cull_box.Contains(p_plot[3]))
            return false;

        ImVec2 p[4];
        p[0] = PlotToPixels(p_plot[0]);
        p[1] = PlotToPixels(p_plot[1]);
        p[2] = PlotToPixels(p_plot[2]);
        p[3] = PlotToPixels(p_plot[3]);

        draw_list_3d._VtxWritePtr[0].pos.x = p[0].x;
        draw_list_3d._VtxWritePtr[0].pos.y = p[0].y;
        draw_list_3d._VtxWritePtr[0].uv = UV;
        draw_list_3d._VtxWritePtr[0].col = ColGetter(4 * prim);

        draw_list_3d._VtxWritePtr[1].pos.x = p[1].x;
        draw_list_3d._VtxWritePtr[1].pos.y = p[1].y;
        draw_list_3d._VtxWritePtr[1].uv = UV;
        draw_list_3d._VtxWritePtr[1].col = ColGetter(4 * prim + 1);

        draw_list_3d._VtxWritePtr[2].pos.x = p[2].x;
        draw_list_3d._VtxWritePtr[2].pos.y = p[2].y;
        draw_list_3d._VtxWritePtr[2].uv = UV;
        draw_list_3d._VtxWritePtr[2].col = ColGetter(4 * prim + 2);

        draw_list_3d._VtxWritePtr[3].pos.x = p[3].x;
        draw_list_3d._VtxWritePtr[3].pos.y = p[3].y;
        draw_list_3d._VtxWritePtr[3].uv = UV;
        draw_list_3d._VtxWritePtr[3].col = ColGetter(4 * prim + 3);

        draw_list_3d._VtxWritePtr += 4;

        draw_list_3d._IdxWritePtr[0] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
        draw_list_3d._IdxWritePtr[1] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 1);
        draw_list_3d._IdxWritePtr[2] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);

        draw_list_3d._IdxWritePtr[3] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
        draw_list_3d._IdxWritePtr[4] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);
        draw_list_3d._IdxWritePtr[5] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 3);

        draw_list_3d._IdxWritePtr += 6;

        double z = GetPointDepth((p_plot[0] + p_plot[1] + p_plot[2] + p_plot[3]) / 4.0);
        draw_list_3d._ZWritePtr[0] = z;
        draw_list_3d._ZWritePtr[1] = z;
        draw_list_3d._ZWritePtr += 2;

        draw_list_3d._VtxCurrentIdx += 4;

        return true;
    }

    const _Getter& Getter;
    const _GetterColor ColGetter;
    mutable ImVec2 UV;
};

template <class _Getter> struct RendererQuadImage : RendererBase {
    RendererQuadImage(const _Getter& getter, ImTextureRef tex_ref, const ImVec2& uv0, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3,
                      ImU32 col)
        : RendererBase(getter.Count / 4, 6, 4), Getter(getter), TexRef(tex_ref), UV0(uv0), UV1(uv1), UV2(uv2), UV3(uv3), Col(col) {}

    void Init(ImDrawList3D& ) const {}

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {
        ImPlot3DPoint p_plot[4];
        p_plot[0] = Getter(4 * prim);
        p_plot[1] = Getter(4 * prim + 1);
        p_plot[2] = Getter(4 * prim + 2);
        p_plot[3] = Getter(4 * prim + 3);

        if (!cull_box.Contains(p_plot[0]) && !cull_box.Contains(p_plot[1]) && !cull_box.Contains(p_plot[2]) && !cull_box.Contains(p_plot[3]))
            return false;

        draw_list_3d.SetTexture(TexRef);

        ImVec2 p[4];
        p[0] = PlotToPixels(p_plot[0]);
        p[1] = PlotToPixels(p_plot[1]);
        p[2] = PlotToPixels(p_plot[2]);
        p[3] = PlotToPixels(p_plot[3]);

        draw_list_3d._VtxWritePtr[0].pos.x = p[0].x;
        draw_list_3d._VtxWritePtr[0].pos.y = p[0].y;
        draw_list_3d._VtxWritePtr[0].uv = UV0;
        draw_list_3d._VtxWritePtr[0].col = Col;

        draw_list_3d._VtxWritePtr[1].pos.x = p[1].x;
        draw_list_3d._VtxWritePtr[1].pos.y = p[1].y;
        draw_list_3d._VtxWritePtr[1].uv = UV1;
        draw_list_3d._VtxWritePtr[1].col = Col;

        draw_list_3d._VtxWritePtr[2].pos.x = p[2].x;
        draw_list_3d._VtxWritePtr[2].pos.y = p[2].y;
        draw_list_3d._VtxWritePtr[2].uv = UV2;
        draw_list_3d._VtxWritePtr[2].col = Col;

        draw_list_3d._VtxWritePtr[3].pos.x = p[3].x;
        draw_list_3d._VtxWritePtr[3].pos.y = p[3].y;
        draw_list_3d._VtxWritePtr[3].uv = UV3;
        draw_list_3d._VtxWritePtr[3].col = Col;

        draw_list_3d._VtxWritePtr += 4;

        draw_list_3d._IdxWritePtr[0] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
        draw_list_3d._IdxWritePtr[1] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 1);
        draw_list_3d._IdxWritePtr[2] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);

        draw_list_3d._IdxWritePtr[3] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
        draw_list_3d._IdxWritePtr[4] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);
        draw_list_3d._IdxWritePtr[5] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 3);

        draw_list_3d._IdxWritePtr += 6;

        double z = GetPointDepth((p_plot[0] + p_plot[1] + p_plot[2] + p_plot[3]) / 4.0);
        draw_list_3d._ZWritePtr[0] = z;
        draw_list_3d._ZWritePtr[1] = z;
        draw_list_3d._ZWritePtr += 2;

        draw_list_3d._VtxCurrentIdx += 4;

        draw_list_3d.ResetTexture();

        return true;
    }

    const _Getter& Getter;
    const ImTextureRef TexRef;
    const ImVec2 UV0, UV1, UV2, UV3;
    const ImU32 Col;
};

template <class _Getter> struct RendererSurfaceFill : RendererBase {
    RendererSurfaceFill(const _Getter& getter, int x_count, int y_count, ImU32 col, double scale_min, double scale_max)
        : RendererBase((x_count - 1) * (y_count - 1), 6, 4), Getter(getter), Min(0.), Max(0.), XCount(x_count), YCount(y_count), Col(col),
          ScaleMin(scale_min), ScaleMax(scale_max) {}

    void Init(ImDrawList3D& draw_list_3d) const {
        UV = draw_list_3d._SharedData->TexUvWhitePixel;

        const ImPlot3DNextItemData& n = GetItemData();
        if (n.IsAutoFill) {
            Min = DBL_MAX;
            Max = -DBL_MAX;
            for (int i = 0; i < Getter.Count; i++) {
                double z = Getter(i).z;
                Min = ImMin(Min, z);
                Max = ImMax(Max, z);
            }
        }
    }

    IMPLOT3D_INLINE bool Render(ImDrawList3D& draw_list_3d, const ImPlot3DBox& cull_box, int prim) const {
        int x = prim % (XCount - 1);
        int y = prim / (XCount - 1);

        ImPlot3DPoint p_plot[4];
        p_plot[0] = Getter(x + y * XCount);
        p_plot[1] = Getter(x + 1 + y * XCount);
        p_plot[2] = Getter(x + 1 + (y + 1) * XCount);
        p_plot[3] = Getter(x + (y + 1) * XCount);

        if (!cull_box.Contains(p_plot[0]) && !cull_box.Contains(p_plot[1]) && !cull_box.Contains(p_plot[2]) && !cull_box.Contains(p_plot[3]))
            return false;

        ImU32 cols[4] = {Col, Col, Col, Col};
        const ImPlot3DNextItemData& n = GetItemData();
        const int vtx_idx[4] = {x + y * XCount, x + 1 + y * XCount, x + 1 + (y + 1) * XCount, x + (y + 1) * XCount};
        if (n.Spec.FillColors != nullptr) {
            float alpha = n.Spec.FillAlpha;
            for (int i = 0; i < 4; i++) {
                ImU32 c = n.Spec.FillColors[vtx_idx[i]];
                ImVec4 cv = ImGui::ColorConvertU32ToFloat4(c);
                cv.w *= alpha;
                cols[i] = ImGui::ColorConvertFloat4ToU32(cv);
            }
        } else if (n.IsAutoFill) {
            float alpha = GImPlot3D->NextItemData.Spec.FillAlpha;
            double min = Min;
            double max = Max;
            if (ScaleMin != 0.0 || ScaleMax != 0.0) {
                min = ScaleMin;
                max = ScaleMax;
            }
            for (int i = 0; i < 4; i++) {
                ImVec4 col = SampleColormap((float)ImClamp(ImRemap01(p_plot[i].z, min, max), 0.0, 1.0));
                col.w *= alpha;
                cols[i] = ImGui::ColorConvertFloat4ToU32(col);
            }
        }

        ImVec2 p[4];
        p[0] = PlotToPixels(p_plot[0]);
        p[1] = PlotToPixels(p_plot[1]);
        p[2] = PlotToPixels(p_plot[2]);
        p[3] = PlotToPixels(p_plot[3]);

        draw_list_3d._VtxWritePtr[0].pos.x = p[0].x;
        draw_list_3d._VtxWritePtr[0].pos.y = p[0].y;
        draw_list_3d._VtxWritePtr[0].uv = UV;
        draw_list_3d._VtxWritePtr[0].col = cols[0];

        draw_list_3d._VtxWritePtr[1].pos.x = p[1].x;
        draw_list_3d._VtxWritePtr[1].pos.y = p[1].y;
        draw_list_3d._VtxWritePtr[1].uv = UV;
        draw_list_3d._VtxWritePtr[1].col = cols[1];

        draw_list_3d._VtxWritePtr[2].pos.x = p[2].x;
        draw_list_3d._VtxWritePtr[2].pos.y = p[2].y;
        draw_list_3d._VtxWritePtr[2].uv = UV;
        draw_list_3d._VtxWritePtr[2].col = cols[2];

        draw_list_3d._VtxWritePtr[3].pos.x = p[3].x;
        draw_list_3d._VtxWritePtr[3].pos.y = p[3].y;
        draw_list_3d._VtxWritePtr[3].uv = UV;
        draw_list_3d._VtxWritePtr[3].col = cols[3];

        draw_list_3d._VtxWritePtr += 4;

        draw_list_3d._IdxWritePtr[0] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
        draw_list_3d._IdxWritePtr[1] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 1);
        draw_list_3d._IdxWritePtr[2] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);

        draw_list_3d._IdxWritePtr[3] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx);
        draw_list_3d._IdxWritePtr[4] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 2);
        draw_list_3d._IdxWritePtr[5] = (ImDrawIdx)(draw_list_3d._VtxCurrentIdx + 3);

        draw_list_3d._IdxWritePtr += 6;

        draw_list_3d._ZWritePtr[0] = GetPointDepth((p_plot[0] + p_plot[1] + p_plot[2]) / 3.0);
        draw_list_3d._ZWritePtr[1] = GetPointDepth((p_plot[0] + p_plot[2] + p_plot[3]) / 3.0);
        draw_list_3d._ZWritePtr += 2;

        draw_list_3d._VtxCurrentIdx += 4;

        return true;
    }

    const _Getter& Getter;
    mutable ImVec2 UV;
    mutable double Min;
    mutable double Max;
    const int XCount;
    const int YCount;
    const ImU32 Col;
    const double ScaleMin;
    const double ScaleMax;
};

template <typename T> IMPLOT3D_INLINE T IndexData(const T* data, int idx, int count, int offset, int stride) {
    const int s = ((offset == 0) << 0) | ((stride == sizeof(T)) << 1);
    switch (s) {
        case 3: return data[idx];
        case 2: return data[(offset + idx) % count];
        case 1: return *(const T*)(const void*)((const unsigned char*)data + (size_t)((idx)) * stride);
        case 0: return *(const T*)(const void*)((const unsigned char*)data + (size_t)((offset + idx) % count) * stride);
        default: return T(0);
    }
}

template <typename T> struct IndexerIdx {
    IndexerIdx(const T* data, int count, int offset = 0, int stride = sizeof(T))
        : Data(data), Count(count), Offset(count ? ImPosMod(offset, count) : 0), Stride(stride) {}
    template <typename I> IMPLOT3D_INLINE double operator()(I idx) const { return (double)IndexData(Data, idx, Count, Offset, Stride); }
    const T* Data;
    int Count;
    int Offset;
    int Stride;
};

template <typename _IndexerX, typename _IndexerY, typename _IndexerZ> struct GetterXYZ {
    GetterXYZ(_IndexerX x, _IndexerY y, _IndexerZ z, int count) : IndexerX(x), IndexerY(y), IndexerZ(z), Count(count) {}
    template <typename I> IMPLOT3D_INLINE ImPlot3DPoint operator()(I idx) const { return ImPlot3DPoint(IndexerX(idx), IndexerY(idx), IndexerZ(idx)); }
    const _IndexerX IndexerX;
    const _IndexerY IndexerY;
    const _IndexerZ IndexerZ;
    const int Count;
};

template <typename _Getter> struct GetterLoop {
    GetterLoop(_Getter getter) : Getter(getter), Count(getter.Count + 1) {}
    template <typename I> IMPLOT3D_INLINE ImPlot3DPoint operator()(I idx) const {
        idx = idx % (Count - 1);
        return Getter(idx);
    }
    const _Getter Getter;
    const int Count;
};

template <typename _Getter> struct GetterTriangleLines {
    GetterTriangleLines(_Getter getter) : Getter(getter), Count(getter.Count * 2) {}
    template <typename I> IMPLOT3D_INLINE ImPlot3DPoint operator()(I idx) const {
        idx = ((idx % 6 + 1) / 2) % 3 + idx / 6 * 3;
        return Getter(idx);
    }
    const _Getter Getter;
    const int Count;
};

template <typename _Getter> struct GetterQuadLines {
    GetterQuadLines(_Getter getter) : Getter(getter), Count(getter.Count * 2) {}
    template <typename I> IMPLOT3D_INLINE ImPlot3DPoint operator()(I idx) const {
        idx = ((idx % 8 + 1) / 2) % 4 + idx / 8 * 4;
        return Getter(idx);
    }
    const _Getter Getter;
    const int Count;
};

template <typename _Getter> struct GetterSurfaceLines {
    GetterSurfaceLines(_Getter getter, int x_count, int y_count) : Getter(getter), XCount(x_count), YCount(y_count) {
        int horizontal_segments = (XCount - 1) * YCount;
        int vertical_segments = (YCount - 1) * XCount;
        int segments = horizontal_segments + vertical_segments;
        Count = segments * 2;
    }

    template <typename I> IMPLOT3D_INLINE ImPlot3DPoint operator()(I idx) const {

        int endpoint_i = (int)(idx % 2);
        int segment_i = (int)(idx / 2);

        int horizontal_segments = (XCount - 1) * YCount;

        int px, py;
        if (segment_i < horizontal_segments) {

            int row = segment_i / (XCount - 1);
            int col = segment_i % (XCount - 1);

            px = endpoint_i == 0 ? col : col + 1;
            py = row;
        } else {

            int seg_v = segment_i - horizontal_segments;
            int col = seg_v / (YCount - 1);
            int row = seg_v % (YCount - 1);

            px = col;
            py = row + endpoint_i;
        }

        return Getter(py * XCount + px);
    }

    const _Getter Getter;
    int Count;
    const int XCount;
    const int YCount;
};

struct Getter3DPoints {
    Getter3DPoints(const ImPlot3DPoint* points, int count) : Points(points), Count(count) {}
    template <typename I> IMPLOT3D_INLINE ImPlot3DPoint operator()(I idx) const { return Points[idx]; }
    const ImPlot3DPoint* Points;
    const int Count;
};

template <typename TGX, typename TGY, typename TGZ> struct GetterMeshTriangles {
    GetterMeshTriangles(TGX gx, TGY gy, TGZ gz, const unsigned int* idx, int idx_count)
        : Gx(gx), Gy(gy), Gz(gz), Idx(idx), TriCount(idx_count / 3), Count(idx_count) {}

    template <typename I> IMPLOT3D_INLINE ImPlot3DPoint operator()(I i) const {
        unsigned int vi = Idx[i];
        return ImPlot3DPoint(Gx(vi), Gy(vi), Gz(vi));
    }

    TGX Gx;
    TGY Gy;
    TGZ Gz;
    const unsigned int* Idx;
    int TriCount;
    int Count;
};

struct GetterConstColor {
    GetterConstColor(ImU32 col) : Col(col) {}
    IMPLOT3D_INLINE ImU32 operator()(int) const { return Col; }
    ImU32 Col;
};

struct GetterIdxColor {
    GetterIdxColor(const ImU32* data, int count, float alpha = 1.0f) : Data(data), Count(count), Alpha(alpha) {}
    IMPLOT3D_INLINE ImU32 operator()(int idx) const {
        ImU32 col = Data[idx];
        if (Alpha < 1.0f) {
            ImVec4 c = ImGui::ColorConvertU32ToFloat4(col);
            c.w *= Alpha;
            col = ImGui::ColorConvertFloat4ToU32(c);
        }
        return col;
    }
    const ImU32* Data;
    const int Count;
    const float Alpha;
};

struct GetterConstSize {
    GetterConstSize(float size) : Size(size) {}
    IMPLOT3D_INLINE float operator()(int) const { return Size; }
    float Size;
};

struct GetterIdxSize {
    GetterIdxSize(const float* data, int count) : Data(data), Count(count) {}
    IMPLOT3D_INLINE float operator()(int idx) const { return Data[idx]; }
    const float* Data;
    const int Count;
};

template <class _Renderer> void RenderPrimitivesEx(const _Renderer& renderer) {
    ImPlot3DPlot& plot = *GetCurrentPlot();
    ImDrawList3D& draw_list_3d = plot.DrawList;
    ImPlot3DBox cull_box;
    if (ImHasFlag(plot.Flags, ImPlot3DFlags_NoClip)) {
        cull_box.Min = ImPlot3DPoint(-HUGE_VAL, -HUGE_VAL, -HUGE_VAL);
        cull_box.Max = ImPlot3DPoint(HUGE_VAL, HUGE_VAL, HUGE_VAL);
    } else {
        cull_box.Min = plot.RangeMin();
        cull_box.Max = plot.RangeMax();
    }

    unsigned int prims_to_render = ImMin(renderer.Prims, (ImDrawList3D::MaxIdx() - draw_list_3d._VtxCurrentIdx) / renderer.VtxConsumed);

    draw_list_3d.PrimReserve(prims_to_render * renderer.IdxConsumed, prims_to_render * renderer.VtxConsumed);

    renderer.Init(draw_list_3d);

    int num_culled = 0;
    for (unsigned int i = 0; i < prims_to_render; i++)
        if (!renderer.Render(draw_list_3d, cull_box, i))
            num_culled++;

    draw_list_3d.PrimUnreserve(num_culled * renderer.IdxConsumed, num_culled * renderer.VtxConsumed);
}

template <template <class> class _Renderer, class _Getter, typename... Args> void RenderPrimitives(const _Getter& getter, Args... args) {
    RenderPrimitivesEx(_Renderer<_Getter>(getter, args...));
}

template <template <class, class> class _Renderer, class _Getter1, class _Getter2, typename... Args>
void RenderPrimitives2(const _Getter1& getter1, const _Getter2& getter2, Args... args) {
    RenderPrimitivesEx(_Renderer<_Getter1, _Getter2>(getter1, getter2, args...));
}

template <template <class, class, class> class _Renderer, class _Getter1, class _Getter2, class _Getter3, typename... Args>
void RenderPrimitives3(const _Getter1& getter1, const _Getter2& getter2, const _Getter3& getter3, Args... args) {
    RenderPrimitivesEx(_Renderer<_Getter1, _Getter2, _Getter3>(getter1, getter2, getter3, args...));
}

static const ImVec2 MARKER_FILL_CIRCLE[10] = {ImVec2(1.0f, 0.0f),
                                              ImVec2(0.809017f, 0.58778524f),
                                              ImVec2(0.30901697f, 0.95105654f),
                                              ImVec2(-0.30901703f, 0.9510565f),
                                              ImVec2(-0.80901706f, 0.5877852f),
                                              ImVec2(-1.0f, 0.0f),
                                              ImVec2(-0.80901694f, -0.58778536f),
                                              ImVec2(-0.3090171f, -0.9510565f),
                                              ImVec2(0.30901712f, -0.9510565f),
                                              ImVec2(0.80901694f, -0.5877853f)};
static const ImVec2 MARKER_FILL_SQUARE[4] = {ImVec2(SQRT_1_2, SQRT_1_2), ImVec2(SQRT_1_2, -SQRT_1_2), ImVec2(-SQRT_1_2, -SQRT_1_2),
                                             ImVec2(-SQRT_1_2, SQRT_1_2)};
static const ImVec2 MARKER_FILL_DIAMOND[4] = {ImVec2(1, 0), ImVec2(0, -1), ImVec2(-1, 0), ImVec2(0, 1)};
static const ImVec2 MARKER_FILL_UP[3] = {ImVec2(SQRT_3_2, 0.5f), ImVec2(0, -1), ImVec2(-SQRT_3_2, 0.5f)};
static const ImVec2 MARKER_FILL_DOWN[3] = {ImVec2(SQRT_3_2, -0.5f), ImVec2(0, 1), ImVec2(-SQRT_3_2, -0.5f)};
static const ImVec2 MARKER_FILL_LEFT[3] = {ImVec2(-1, 0), ImVec2(0.5, SQRT_3_2), ImVec2(0.5, -SQRT_3_2)};
static const ImVec2 MARKER_FILL_RIGHT[3] = {ImVec2(1, 0), ImVec2(-0.5, SQRT_3_2), ImVec2(-0.5, -SQRT_3_2)};
static const ImVec2 MARKER_LINE_CIRCLE[20] = {ImVec2(1.0f, 0.0f),
                                              ImVec2(0.809017f, 0.58778524f),
                                              ImVec2(0.809017f, 0.58778524f),
                                              ImVec2(0.30901697f, 0.95105654f),
                                              ImVec2(0.30901697f, 0.95105654f),
                                              ImVec2(-0.30901703f, 0.9510565f),
                                              ImVec2(-0.30901703f, 0.9510565f),
                                              ImVec2(-0.80901706f, 0.5877852f),
                                              ImVec2(-0.80901706f, 0.5877852f),
                                              ImVec2(-1.0f, 0.0f),
                                              ImVec2(-1.0f, 0.0f),
                                              ImVec2(-0.80901694f, -0.58778536f),
                                              ImVec2(-0.80901694f, -0.58778536f),
                                              ImVec2(-0.3090171f, -0.9510565f),
                                              ImVec2(-0.3090171f, -0.9510565f),
                                              ImVec2(0.30901712f, -0.9510565f),
                                              ImVec2(0.30901712f, -0.9510565f),
                                              ImVec2(0.80901694f, -0.5877853f),
                                              ImVec2(0.80901694f, -0.5877853f),
                                              ImVec2(1.0f, 0.0f)};
static const ImVec2 MARKER_LINE_SQUARE[8] = {ImVec2(SQRT_1_2, SQRT_1_2),   ImVec2(SQRT_1_2, -SQRT_1_2),  ImVec2(SQRT_1_2, -SQRT_1_2),
                                             ImVec2(-SQRT_1_2, -SQRT_1_2), ImVec2(-SQRT_1_2, -SQRT_1_2), ImVec2(-SQRT_1_2, SQRT_1_2),
                                             ImVec2(-SQRT_1_2, SQRT_1_2),  ImVec2(SQRT_1_2, SQRT_1_2)};
static const ImVec2 MARKER_LINE_DIAMOND[8] = {ImVec2(1, 0),  ImVec2(0, -1), ImVec2(0, -1), ImVec2(-1, 0),
                                              ImVec2(-1, 0), ImVec2(0, 1),  ImVec2(0, 1),  ImVec2(1, 0)};
static const ImVec2 MARKER_LINE_UP[6] = {ImVec2(SQRT_3_2, 0.5f),  ImVec2(0, -1),           ImVec2(0, -1),
                                         ImVec2(-SQRT_3_2, 0.5f), ImVec2(-SQRT_3_2, 0.5f), ImVec2(SQRT_3_2, 0.5f)};
static const ImVec2 MARKER_LINE_DOWN[6] = {ImVec2(SQRT_3_2, -0.5f),  ImVec2(0, 1),           ImVec2(0, 1), ImVec2(-SQRT_3_2, -0.5f),
                                           ImVec2(-SQRT_3_2, -0.5f), ImVec2(SQRT_3_2, -0.5f)};
static const ImVec2 MARKER_LINE_LEFT[6] = {ImVec2(-1, 0),          ImVec2(0.5, SQRT_3_2),  ImVec2(0.5, SQRT_3_2),
                                           ImVec2(0.5, -SQRT_3_2), ImVec2(0.5, -SQRT_3_2), ImVec2(-1, 0)};
static const ImVec2 MARKER_LINE_RIGHT[6] = {
    ImVec2(1, 0), ImVec2(-0.5, SQRT_3_2), ImVec2(-0.5, SQRT_3_2), ImVec2(-0.5, -SQRT_3_2), ImVec2(-0.5, -SQRT_3_2), ImVec2(1, 0)};
static const ImVec2 MARKER_LINE_ASTERISK[6] = {ImVec2(-SQRT_3_2, -0.5f), ImVec2(SQRT_3_2, 0.5f), ImVec2(-SQRT_3_2, 0.5f),
                                               ImVec2(SQRT_3_2, -0.5f),  ImVec2(0, -1),          ImVec2(0, 1)};
static const ImVec2 MARKER_LINE_PLUS[4] = {ImVec2(-1, 0), ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, 1)};
static const ImVec2 MARKER_LINE_CROSS[4] = {ImVec2(-SQRT_1_2, -SQRT_1_2), ImVec2(SQRT_1_2, SQRT_1_2), ImVec2(SQRT_1_2, -SQRT_1_2),
                                            ImVec2(-SQRT_1_2, SQRT_1_2)};

template <typename _Getter, typename _GetterFillColor, typename _GetterLineColor, typename _GetterSize>
void RenderMarkers(const _Getter& getter, ImPlot3DMarker marker, bool rend_fill, const _GetterFillColor& col_fill, bool rend_line,
                   const _GetterLineColor& col_line, const _GetterSize& size, float weight) {
    if (rend_fill) {
        switch (marker) {
            case ImPlot3DMarker_Circle: RenderPrimitives3<RendererMarkersFill>(getter, col_fill, size, MARKER_FILL_CIRCLE, 10); break;
            case ImPlot3DMarker_Square: RenderPrimitives3<RendererMarkersFill>(getter, col_fill, size, MARKER_FILL_SQUARE, 4); break;
            case ImPlot3DMarker_Diamond: RenderPrimitives3<RendererMarkersFill>(getter, col_fill, size, MARKER_FILL_DIAMOND, 4); break;
            case ImPlot3DMarker_Up: RenderPrimitives3<RendererMarkersFill>(getter, col_fill, size, MARKER_FILL_UP, 3); break;
            case ImPlot3DMarker_Down: RenderPrimitives3<RendererMarkersFill>(getter, col_fill, size, MARKER_FILL_DOWN, 3); break;
            case ImPlot3DMarker_Left: RenderPrimitives3<RendererMarkersFill>(getter, col_fill, size, MARKER_FILL_LEFT, 3); break;
            case ImPlot3DMarker_Right: RenderPrimitives3<RendererMarkersFill>(getter, col_fill, size, MARKER_FILL_RIGHT, 3); break;
        }
    }
    if (rend_line) {
        switch (marker) {
            case ImPlot3DMarker_Circle: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_CIRCLE, 20, weight); break;
            case ImPlot3DMarker_Square: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_SQUARE, 8, weight); break;
            case ImPlot3DMarker_Diamond: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_DIAMOND, 8, weight); break;
            case ImPlot3DMarker_Up: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_UP, 6, weight); break;
            case ImPlot3DMarker_Down: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_DOWN, 6, weight); break;
            case ImPlot3DMarker_Left: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_LEFT, 6, weight); break;
            case ImPlot3DMarker_Right: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_RIGHT, 6, weight); break;
            case ImPlot3DMarker_Asterisk: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_ASTERISK, 6, weight); break;
            case ImPlot3DMarker_Plus: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_PLUS, 4, weight); break;
            case ImPlot3DMarker_Cross: RenderPrimitives3<RendererMarkersLine>(getter, col_line, size, MARKER_LINE_CROSS, 4, weight); break;
        }
    }
}

template <typename _Getter> void RenderMarkers(const _Getter& getter, ImPlot3DMarker marker, float size, bool rend_fill, ImU32 col_fill,
                                               bool rend_line, ImU32 col_line, float weight) {
    RenderMarkers(getter, marker, rend_fill, GetterConstColor(col_fill), rend_line, GetterConstColor(col_line), GetterConstSize(size), weight);
}

template <typename _Getter> void RenderColoredMarkers(const _Getter& getter, const ImPlot3DNextItemData& n) {
    const ImPlot3DSpec& s = n.Spec;
    const ImU32 col_line = ImGui::GetColorU32(s.MarkerLineColor);
    const ImU32 col_fill = ImGui::GetColorU32(s.MarkerFillColor);
    if (s.MarkerSizes != nullptr) {
        GetterIdxSize size_getter(s.MarkerSizes, getter.Count);
        if (s.MarkerFillColors != nullptr && s.MarkerLineColors != nullptr) {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterIdxColor(s.MarkerFillColors, getter.Count, s.FillAlpha), n.RenderMarkerLine,
                          GetterIdxColor(s.MarkerLineColors, getter.Count), size_getter, s.LineWeight);
        } else if (s.MarkerFillColors != nullptr) {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterIdxColor(s.MarkerFillColors, getter.Count, s.FillAlpha), n.RenderMarkerLine,
                          GetterConstColor(col_line), size_getter, s.LineWeight);
        } else if (s.MarkerLineColors != nullptr) {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterConstColor(col_fill), n.RenderMarkerLine,
                          GetterIdxColor(s.MarkerLineColors, getter.Count), size_getter, s.LineWeight);
        } else {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterConstColor(col_fill), n.RenderMarkerLine, GetterConstColor(col_line),
                          size_getter, s.LineWeight);
        }
    } else {
        GetterConstSize size_getter(s.MarkerSize);
        if (s.MarkerFillColors != nullptr && s.MarkerLineColors != nullptr) {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterIdxColor(s.MarkerFillColors, getter.Count, s.FillAlpha), n.RenderMarkerLine,
                          GetterIdxColor(s.MarkerLineColors, getter.Count), size_getter, s.LineWeight);
        } else if (s.MarkerFillColors != nullptr) {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterIdxColor(s.MarkerFillColors, getter.Count, s.FillAlpha), n.RenderMarkerLine,
                          GetterConstColor(col_line), size_getter, s.LineWeight);
        } else if (s.MarkerLineColors != nullptr) {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterConstColor(col_fill), n.RenderMarkerLine,
                          GetterIdxColor(s.MarkerLineColors, getter.Count), size_getter, s.LineWeight);
        } else {
            RenderMarkers(getter, s.Marker, n.RenderMarkerFill, GetterConstColor(col_fill), n.RenderMarkerLine, GetterConstColor(col_line),
                          size_getter, s.LineWeight);
        }
    }
}

template <typename Getter> void PlotScatterEx(const char* label_id, const Getter& getter, const ImPlot3DSpec& spec) {
    if (BeginItemEx(label_id, getter, spec, spec.MarkerLineColor, spec.Marker)) {
        ImPlot3DContext& gp = *GImPlot3D;
        ImPlot3DNextItemData& n = gp.NextItemData;

        if (n.Spec.Marker == ImPlot3DMarker_None)
            n.Spec.Marker = ImPlot3DMarker_Circle;
        if (n.Spec.Marker != ImPlot3DMarker_None)
            RenderColoredMarkers(getter, n);
        EndItem();
    }
}

template <typename T> void PlotScatter(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec) {
    if (count < 1)
        return;
    int stride = Stride<T>(spec);
    GetterXYZ<IndexerIdx<T>, IndexerIdx<T>, IndexerIdx<T>> getter(IndexerIdx<T>(xs, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(ys, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(zs, count, spec.Offset, stride), count);
    return PlotScatterEx(label_id, getter, spec);
}

#define INSTANTIATE_MACRO(T)                                                                                                                         \
    template IMPLOT3D_API void PlotScatter<T>(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

template <typename _Getter> void PlotLineEx(const char* label_id, const _Getter& getter, const ImPlot3DSpec& spec) {
    if (BeginItemEx(label_id, getter, spec, spec.LineColor, spec.Marker)) {
        const ImPlot3DNextItemData& n = GetItemData();
        const ImPlot3DSpec& s = n.Spec;

        if (getter.Count >= 2 && n.RenderLine) {
            if (ImHasFlag(spec.Flags, ImPlot3DLineFlags_Segments)) {
                if (s.LineColors != nullptr) {
                    RenderPrimitives2<RendererLineSegments>(getter, GetterIdxColor(s.LineColors, getter.Count), s.LineWeight);
                } else {
                    RenderPrimitives2<RendererLineSegments>(getter, GetterConstColor(ImGui::GetColorU32(s.LineColor)), s.LineWeight);
                }
            } else if (ImHasFlag(spec.Flags, ImPlot3DLineFlags_Loop)) {
                if (s.LineColors != nullptr) {
                    if (ImHasFlag(spec.Flags, ImPlot3DLineFlags_SkipNaN))
                        RenderPrimitives2<RendererLineStripSkip>(GetterLoop<_Getter>(getter), GetterIdxColor(s.LineColors, getter.Count),
                                                                 s.LineWeight);
                    else
                        RenderPrimitives2<RendererLineStrip>(GetterLoop<_Getter>(getter), GetterIdxColor(s.LineColors, getter.Count), s.LineWeight);
                } else {
                    if (ImHasFlag(spec.Flags, ImPlot3DLineFlags_SkipNaN))
                        RenderPrimitives2<RendererLineStripSkip>(GetterLoop<_Getter>(getter), GetterConstColor(ImGui::GetColorU32(s.LineColor)),
                                                                 s.LineWeight);
                    else
                        RenderPrimitives2<RendererLineStrip>(GetterLoop<_Getter>(getter), GetterConstColor(ImGui::GetColorU32(s.LineColor)),
                                                             s.LineWeight);
                }
            } else {
                if (s.LineColors != nullptr) {
                    if (ImHasFlag(spec.Flags, ImPlot3DLineFlags_SkipNaN))
                        RenderPrimitives2<RendererLineStripSkip>(getter, GetterIdxColor(s.LineColors, getter.Count), s.LineWeight);
                    else
                        RenderPrimitives2<RendererLineStrip>(getter, GetterIdxColor(s.LineColors, getter.Count), s.LineWeight);
                } else {
                    if (ImHasFlag(spec.Flags, ImPlot3DLineFlags_SkipNaN))
                        RenderPrimitives2<RendererLineStripSkip>(getter, GetterConstColor(ImGui::GetColorU32(s.LineColor)), s.LineWeight);
                    else
                        RenderPrimitives2<RendererLineStrip>(getter, GetterConstColor(ImGui::GetColorU32(s.LineColor)), s.LineWeight);
                }
            }
        }

        if (s.Marker != ImPlot3DMarker_None)
            RenderColoredMarkers(getter, n);
        EndItem();
    }
}

IMPLOT3D_TMP void PlotLine(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec) {
    if (count < 2)
        return;
    int stride = Stride<T>(spec);
    GetterXYZ<IndexerIdx<T>, IndexerIdx<T>, IndexerIdx<T>> getter(IndexerIdx<T>(xs, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(ys, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(zs, count, spec.Offset, stride), count);
    return PlotLineEx(label_id, getter, spec);
}

#define INSTANTIATE_MACRO(T)                                                                                                                         \
    template IMPLOT3D_API void PlotLine<T>(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

template <typename _Getter> void PlotTriangleEx(const char* label_id, const _Getter& getter, const ImPlot3DSpec& spec) {
    if (BeginItemEx(label_id, getter, spec, spec.FillColor, spec.Marker)) {
        const ImPlot3DNextItemData& n = GetItemData();
        const ImPlot3DSpec& s = n.Spec;

        if (getter.Count >= 3 && n.RenderFill && !ImHasFlag(spec.Flags, ImPlot3DTriangleFlags_NoFill)) {
            if (s.FillColors != nullptr)
                RenderPrimitives2<RendererTriangleFill>(getter, GetterIdxColor(s.FillColors, getter.Count, s.FillAlpha));
            else
                RenderPrimitives2<RendererTriangleFill>(getter, GetterConstColor(ImGui::GetColorU32(s.FillColor)));
        }

        if (getter.Count >= 2 && n.RenderLine && !ImHasFlag(spec.Flags, ImPlot3DTriangleFlags_NoLines)) {
            if (s.LineColors != nullptr)
                RenderPrimitives2<RendererLineSegments>(GetterTriangleLines<_Getter>(getter), GetterIdxColor(s.LineColors, getter.Count),
                                                        s.LineWeight);
            else
                RenderPrimitives2<RendererLineSegments>(GetterTriangleLines<_Getter>(getter), GetterConstColor(ImGui::GetColorU32(s.LineColor)),
                                                        s.LineWeight);
        }

        if (s.Marker != ImPlot3DMarker_None && !ImHasFlag(spec.Flags, ImPlot3DTriangleFlags_NoMarkers))
            RenderColoredMarkers(getter, n);

        EndItem();
    }
}

IMPLOT3D_TMP void PlotTriangle(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec) {
    if (count < 3)
        return;
    int stride = Stride<T>(spec);
    GetterXYZ<IndexerIdx<T>, IndexerIdx<T>, IndexerIdx<T>> getter(IndexerIdx<T>(xs, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(ys, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(zs, count, spec.Offset, stride), count);
    return PlotTriangleEx(label_id, getter, spec);
}

#define INSTANTIATE_MACRO(T)                                                                                                                         \
    template IMPLOT3D_API void PlotTriangle<T>(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

template <typename _Getter> void PlotQuadEx(const char* label_id, const _Getter& getter, const ImPlot3DSpec& spec) {
    if (BeginItemEx(label_id, getter, spec, spec.FillColor, spec.Marker)) {
        const ImPlot3DNextItemData& n = GetItemData();
        const ImPlot3DSpec& s = n.Spec;

        if (getter.Count >= 4 && n.RenderFill && !ImHasFlag(spec.Flags, ImPlot3DQuadFlags_NoFill)) {
            if (s.FillColors != nullptr)
                RenderPrimitives2<RendererQuadFill>(getter, GetterIdxColor(s.FillColors, getter.Count, s.FillAlpha));
            else
                RenderPrimitives2<RendererQuadFill>(getter, GetterConstColor(ImGui::GetColorU32(s.FillColor)));
        }

        if (getter.Count >= 2 && n.RenderLine && !ImHasFlag(spec.Flags, ImPlot3DQuadFlags_NoLines)) {
            if (s.LineColors != nullptr)
                RenderPrimitives2<RendererLineSegments>(GetterQuadLines<_Getter>(getter), GetterIdxColor(s.LineColors, getter.Count), s.LineWeight);
            else
                RenderPrimitives2<RendererLineSegments>(GetterQuadLines<_Getter>(getter), GetterConstColor(ImGui::GetColorU32(s.LineColor)),
                                                        s.LineWeight);
        }

        if (s.Marker != ImPlot3DMarker_None && !ImHasFlag(spec.Flags, ImPlot3DQuadFlags_NoMarkers))
            RenderColoredMarkers(getter, n);

        EndItem();
    }
}

IMPLOT3D_TMP void PlotQuad(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec) {
    if (count < 3)
        return;
    int stride = Stride<T>(spec);
    GetterXYZ<IndexerIdx<T>, IndexerIdx<T>, IndexerIdx<T>> getter(IndexerIdx<T>(xs, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(ys, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(zs, count, spec.Offset, stride), count);
    return PlotQuadEx(label_id, getter, spec);
}

#define INSTANTIATE_MACRO(T)                                                                                                                         \
    template IMPLOT3D_API void PlotQuad<T>(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

template <typename _Getter> void PlotSurfaceEx(const char* label_id, const _Getter& getter, int x_count, int y_count, double scale_min,
                                               double scale_max, const ImPlot3DSpec& spec) {
    if (BeginItemEx(label_id, getter, spec, spec.FillColor, spec.Marker)) {
        const ImPlot3DNextItemData& n = GetItemData();
        const ImPlot3DSpec& s = n.Spec;

        if (getter.Count >= 4 && n.RenderFill && !ImHasFlag(spec.Flags, ImPlot3DSurfaceFlags_NoFill)) {
            const ImU32 col_fill = ImGui::GetColorU32(s.FillColor);
            RenderPrimitives<RendererSurfaceFill>(getter, x_count, y_count, col_fill, scale_min, scale_max);
        }

        if (getter.Count >= 2 && n.RenderLine && !ImHasFlag(spec.Flags, ImPlot3DSurfaceFlags_NoLines)) {
            if (s.LineColors != nullptr)
                RenderPrimitives2<RendererLineSegments>(GetterSurfaceLines<_Getter>(getter, x_count, y_count),
                                                        GetterIdxColor(s.LineColors, getter.Count), s.LineWeight);
            else
                RenderPrimitives2<RendererLineSegments>(GetterSurfaceLines<_Getter>(getter, x_count, y_count),
                                                        GetterConstColor(ImGui::GetColorU32(s.LineColor)), s.LineWeight);
        }

        if (s.Marker != ImPlot3DMarker_None && !ImHasFlag(spec.Flags, ImPlot3DSurfaceFlags_NoMarkers))
            RenderColoredMarkers(getter, n);

        EndItem();
    }
}

IMPLOT3D_TMP void PlotSurface(const char* label_id, const T* xs, const T* ys, const T* zs, int x_count, int y_count, double scale_min,
                              double scale_max, const ImPlot3DSpec& spec) {
    int count = x_count * y_count;
    if (count < 4)
        return;
    int stride = Stride<T>(spec);
    GetterXYZ<IndexerIdx<T>, IndexerIdx<T>, IndexerIdx<T>> getter(IndexerIdx<T>(xs, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(ys, count, spec.Offset, stride),
                                                                  IndexerIdx<T>(zs, count, spec.Offset, stride), count);
    return PlotSurfaceEx(label_id, getter, x_count, y_count, scale_min, scale_max, spec);
}

#define INSTANTIATE_MACRO(T)                                                                                                                         \
    template IMPLOT3D_API void PlotSurface<T>(const char* label_id, const T* xs, const T* ys, const T* zs, int x_count, int y_count,                 \
                                              double scale_min, double scale_max, const ImPlot3DSpec& spec);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

template <typename _VtxGetter, typename _TriGetter>
void PlotMeshEx(const char* label_id, const _VtxGetter& getter, const _TriGetter& getter_triangles, const ImPlot3DSpec& spec) {
    if (BeginItemEx(label_id, getter, spec, spec.FillColor, spec.Marker)) {
        const ImPlot3DNextItemData& n = GetItemData();
        const ImPlot3DSpec& s = n.Spec;

        if (getter.Count >= 3 && n.RenderFill && !ImHasFlag(spec.Flags, ImPlot3DMeshFlags_NoFill)) {
            if (s.FillColors != nullptr)
                RenderPrimitives2<RendererTriangleFill>(getter_triangles, GetterIdxColor(s.FillColors, getter_triangles.Count, s.FillAlpha));
            else
                RenderPrimitives2<RendererTriangleFill>(getter_triangles, GetterConstColor(ImGui::GetColorU32(s.FillColor)));
        }

        if (getter.Count >= 2 && n.RenderLine && !n.IsAutoLine && !ImHasFlag(spec.Flags, ImPlot3DMeshFlags_NoLines)) {
            if (s.LineColors != nullptr)
                RenderPrimitives2<RendererLineSegments>(GetterTriangleLines<_TriGetter>(getter_triangles),
                                                        GetterIdxColor(s.LineColors, getter_triangles.Count), s.LineWeight);
            else
                RenderPrimitives2<RendererLineSegments>(GetterTriangleLines<_TriGetter>(getter_triangles),
                                                        GetterConstColor(ImGui::GetColorU32(s.LineColor)), s.LineWeight);
        }

        if (s.Marker != ImPlot3DMarker_None && !ImHasFlag(spec.Flags, ImPlot3DMeshFlags_NoMarkers))
            RenderColoredMarkers(getter, n);

        EndItem();
    }
}

IMPLOT3D_TMP void PlotMesh(const char* label_id, const T* vtx_xs, const T* vtx_ys, const T* vtx_zs, const unsigned int* idxs, int vtx_count,
                           int idx_count, const ImPlot3DSpec& spec) {
    if (vtx_count < 3 || idx_count < 3)
        return;
    int stride = Stride<T>(spec);
    GetterXYZ<IndexerIdx<T>, IndexerIdx<T>, IndexerIdx<T>> getter(IndexerIdx<T>(vtx_xs, vtx_count, spec.Offset, stride),
                                                                  IndexerIdx<T>(vtx_ys, vtx_count, spec.Offset, stride),
                                                                  IndexerIdx<T>(vtx_zs, vtx_count, spec.Offset, stride), vtx_count);
    GetterMeshTriangles<IndexerIdx<T>, IndexerIdx<T>, IndexerIdx<T>> getter_triangles(
        IndexerIdx<T>(vtx_xs, vtx_count, spec.Offset, stride), IndexerIdx<T>(vtx_ys, vtx_count, spec.Offset, stride),
        IndexerIdx<T>(vtx_zs, vtx_count, spec.Offset, stride), idxs, idx_count);
    PlotMeshEx(label_id, getter, getter_triangles, spec);
}

#define INSTANTIATE_MACRO(T)                                                                                                                         \
    template IMPLOT3D_API void PlotMesh<T>(const char* label_id, const T* vtx_xs, const T* vtx_ys, const T* vtx_zs, const unsigned int* idxs,        \
                                           int vtx_count, int idx_count, const ImPlot3DSpec& spec);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

void PlotMesh(const char* label_id, const ImPlot3DPoint* vtx, const unsigned int* idxs, int vtx_count, int idx_count, const ImPlot3DSpec& spec) {
    ImPlot3DSpec s = spec;
    s.Offset = 0;
    s.Stride = (int)sizeof(ImPlot3DPoint);
    PlotMesh<double>(label_id, &vtx->x, &vtx->y, &vtx->z, idxs, vtx_count, idx_count, s);
}

IMPLOT3D_API void PlotImage(const char* label_id, ImTextureRef tex_ref, const ImPlot3DPoint& center, const ImPlot3DPoint& axis_u,
                            const ImPlot3DPoint& axis_v, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImPlot3DSpec& spec) {

    ImPlot3DPoint p0 = center - axis_u - axis_v;
    ImPlot3DPoint p1 = center + axis_u - axis_v;
    ImPlot3DPoint p2 = center + axis_u + axis_v;
    ImPlot3DPoint p3 = center - axis_u + axis_v;

    ImVec2 uv_0 = uv0;
    ImVec2 uv_1 = ImVec2(uv1.x, uv0.y);
    ImVec2 uv_2 = uv1;
    ImVec2 uv_3 = ImVec2(uv0.x, uv1.y);

    PlotImage(label_id, tex_ref, p0, p1, p2, p3, uv_0, uv_1, uv_2, uv_3, tint_col, spec);
}

IMPLOT3D_API void PlotImage(const char* label_id, ImTextureRef tex_ref, const ImPlot3DPoint& p0, const ImPlot3DPoint& p1, const ImPlot3DPoint& p2,
                            const ImPlot3DPoint& p3, const ImVec2& uv0, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3,
                            const ImVec4& tint_col, const ImPlot3DSpec& spec) {
    ImPlot3DContext& gp = *GImPlot3D;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != nullptr, "PlotImage() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();

    ImPlot3DPoint corners[4] = {p0, p1, p2, p3};
    Getter3DPoints getter(corners, 4);

    ImVec2 uv_0 = ImVec2(uv0.x, 1 - uv0.y);
    ImVec2 uv_1 = ImVec2(uv1.x, 1 - uv1.y);
    ImVec2 uv_2 = ImVec2(uv2.x, 1 - uv2.y);
    ImVec2 uv_3 = ImVec2(uv3.x, 1 - uv3.y);

    if (BeginItemEx(label_id, getter, spec, tint_col, spec.Marker)) {
        ImU32 tint_col32 = ImGui::ColorConvertFloat4ToU32(tint_col);
        GetCurrentItem()->Color = tint_col32;

        bool is_transparent = (tint_col32 & IM_COL32_A_MASK) == 0;
        if (!is_transparent)
            RenderPrimitives<RendererQuadImage>(getter, tex_ref, uv_0, uv_1, uv_2, uv_3, tint_col32);

        EndItem();
    }
}

void PlotText(const char* text, double x, double y, double z, double angle, const ImVec2& pix_offset) {
    ImPlot3DContext& gp = *GImPlot3D;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != nullptr, "PlotText() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    ImPlot3DPlot& plot = *gp.CurrentPlot;

    ImPlot3DBox cull_box;
    if (ImHasFlag(plot.Flags, ImPlot3DFlags_NoClip)) {
        cull_box.Min = ImPlot3DPoint(-HUGE_VAL, -HUGE_VAL, -HUGE_VAL);
        cull_box.Max = ImPlot3DPoint(HUGE_VAL, HUGE_VAL, HUGE_VAL);
    } else {
        cull_box.Min = plot.RangeMin();
        cull_box.Max = plot.RangeMax();
    }
    if (!cull_box.Contains(ImPlot3DPoint(x, y, z)))
        return;

    ImVec2 p = PlotToPixels(ImPlot3DPoint(x, y, z));
    p.x += pix_offset.x;
    p.y += pix_offset.y;
    AddTextRotated(GetPlotDrawList(), p, (float)angle, GetStyleColorU32(ImPlot3DCol_InlayText), text);
}

void PlotDummy(const char* label_id, const ImPlot3DSpec& spec) {

    ImVec4 item_col = spec.LineColor;
    if (IsColorAuto(item_col))
        item_col = spec.FillColor;
    if (IsColorAuto(item_col))
        item_col = spec.MarkerLineColor;
    if (IsColorAuto(item_col))
        item_col = spec.MarkerFillColor;
    if (BeginItem(label_id, spec, item_col, spec.Marker))
        EndItem();
}

}

#endif
