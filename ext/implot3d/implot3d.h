
#pragma once
#include "imgui.h"
#ifndef IMGUI_DISABLE

#ifndef IMPLOT3D_API
#define IMPLOT3D_API
#endif

#define IMPLOT3D_VERSION "0.5 WIP"
#define IMPLOT3D_VERSION_NUM 500
#define IMPLOT3D_AUTO -1
#define IMPLOT3D_AUTO_COL ImVec4(0, 0, 0, -1)
#define IMPLOT3D_TMP template <typename T> IMPLOT3D_API

struct ImPlot3DContext;
struct ImPlot3DStyle;
struct ImPlot3DPoint;
struct ImPlot3DRay;
struct ImPlot3DPlane;
struct ImPlot3DBox;
struct ImPlot3DRange;
struct ImPlot3DQuat;

typedef int ImPlot3DCond;
typedef int ImPlot3DCol;
typedef int ImPlot3DStyleVar;
typedef int ImPlot3DMarker;
typedef int ImPlot3DLocation;
typedef int ImAxis3D;
typedef int ImPlane3D;
typedef int ImPlot3DScale;
typedef int ImPlot3DColormap;
typedef int ImPlot3DProp;

typedef int ImPlot3DFlags;
typedef int ImPlot3DItemFlags;
typedef int ImPlot3DScatterFlags;
typedef int ImPlot3DLineFlags;
typedef int ImPlot3DTriangleFlags;
typedef int ImPlot3DQuadFlags;
typedef int ImPlot3DSurfaceFlags;
typedef int ImPlot3DMeshFlags;
typedef int ImPlot3DImageFlags;
typedef int ImPlot3DDummyFlags;
typedef int ImPlot3DLegendFlags;
typedef int ImPlot3DAxisFlags;

#if !defined(IMGUI_HAS_TEXTURES) && !defined(IMPLOT3D_NO_IMTEXTUREREF)
typedef ImTextureID ImTextureRef;
#endif

enum ImPlot3DProp_ {
    ImPlot3DProp_LineColor,
    ImPlot3DProp_LineColors,
    ImPlot3DProp_LineWeight,
    ImPlot3DProp_FillColor,
    ImPlot3DProp_FillColors,
    ImPlot3DProp_FillAlpha,
    ImPlot3DProp_Marker,
    ImPlot3DProp_MarkerSize,
    ImPlot3DProp_MarkerSizes,
    ImPlot3DProp_MarkerLineColor,
    ImPlot3DProp_MarkerLineColors,
    ImPlot3DProp_MarkerFillColor,
    ImPlot3DProp_MarkerFillColors,
    ImPlot3DProp_Offset,
    ImPlot3DProp_Stride,
    ImPlot3DProp_Flags
};

enum ImPlot3DFlags_ {
    ImPlot3DFlags_None = 0,
    ImPlot3DFlags_NoTitle = 1 << 0,
    ImPlot3DFlags_NoLegend = 1 << 1,
    ImPlot3DFlags_NoMouseText = 1 << 2,
    ImPlot3DFlags_NoClip = 1 << 3,
    ImPlot3DFlags_NoMenus = 1 << 4,
    ImPlot3DFlags_Equal = 1 << 5,
    ImPlot3DFlags_NoRotate = 1 << 6,
    ImPlot3DFlags_NoPan = 1 << 7,
    ImPlot3DFlags_NoZoom = 1 << 8,
    ImPlot3DFlags_NoInputs = 1 << 9,
    ImPlot3DFlags_CanvasOnly = ImPlot3DFlags_NoTitle | ImPlot3DFlags_NoLegend | ImPlot3DFlags_NoMouseText,
};

enum ImPlot3DCond_ {
    ImPlot3DCond_None = ImGuiCond_None,
    ImPlot3DCond_Always = ImGuiCond_Always,
    ImPlot3DCond_Once = ImGuiCond_Once,
};

enum ImPlot3DCol_ {

    ImPlot3DCol_TitleText,
    ImPlot3DCol_InlayText,
    ImPlot3DCol_FrameBg,
    ImPlot3DCol_PlotBg,
    ImPlot3DCol_PlotBorder,

    ImPlot3DCol_LegendBg,
    ImPlot3DCol_LegendBorder,
    ImPlot3DCol_LegendText,

    ImPlot3DCol_AxisText,
    ImPlot3DCol_AxisGrid,
    ImPlot3DCol_AxisTick,
    ImPlot3DCol_AxisBg,
    ImPlot3DCol_AxisBgHovered,
    ImPlot3DCol_AxisBgActive,
    ImPlot3DCol_COUNT,
};

enum ImPlot3DStyleVar_ {

    ImPlot3DStyleVar_LineWeight,
    ImPlot3DStyleVar_Marker,
    ImPlot3DStyleVar_MarkerSize,
    ImPlot3DStyleVar_FillAlpha,

    ImPlot3DStyleVar_PlotDefaultSize,
    ImPlot3DStyleVar_PlotMinSize,
    ImPlot3DStyleVar_PlotPadding,
    ImPlot3DStyleVar_LabelPadding,
    ImPlot3DStyleVar_ViewScaleFactor,

    ImPlot3DStyleVar_LegendPadding,
    ImPlot3DStyleVar_LegendInnerPadding,
    ImPlot3DStyleVar_LegendSpacing,
    ImPlot3DStyleVar_COUNT
};

enum ImPlot3DMarker_ {
    ImPlot3DMarker_None = -2,
    ImPlot3DMarker_Auto = -1,
    ImPlot3DMarker_Circle,
    ImPlot3DMarker_Square,
    ImPlot3DMarker_Diamond,
    ImPlot3DMarker_Up,
    ImPlot3DMarker_Down,
    ImPlot3DMarker_Left,
    ImPlot3DMarker_Right,
    ImPlot3DMarker_Cross,
    ImPlot3DMarker_Plus,
    ImPlot3DMarker_Asterisk,
    ImPlot3DMarker_COUNT
};

enum ImPlot3DItemFlags_ {
    ImPlot3DItemFlags_None = 0,
    ImPlot3DItemFlags_NoLegend = 1 << 0,
    ImPlot3DItemFlags_NoFit = 1 << 1,
};

enum ImPlot3DScatterFlags_ {
    ImPlot3DScatterFlags_None = 0,
    ImPlot3DScatterFlags_NoLegend = ImPlot3DItemFlags_NoLegend,
    ImPlot3DScatterFlags_NoFit = ImPlot3DItemFlags_NoFit,
};

enum ImPlot3DLineFlags_ {
    ImPlot3DLineFlags_None = 0,
    ImPlot3DLineFlags_NoLegend = ImPlot3DItemFlags_NoLegend,
    ImPlot3DLineFlags_NoFit = ImPlot3DItemFlags_NoFit,
    ImPlot3DLineFlags_Segments = 1 << 10,
    ImPlot3DLineFlags_Loop = 1 << 11,
    ImPlot3DLineFlags_SkipNaN = 1 << 12,
};

enum ImPlot3DTriangleFlags_ {
    ImPlot3DTriangleFlags_None = 0,
    ImPlot3DTriangleFlags_NoLegend = ImPlot3DItemFlags_NoLegend,
    ImPlot3DTriangleFlags_NoFit = ImPlot3DItemFlags_NoFit,
    ImPlot3DTriangleFlags_NoLines = 1 << 10,
    ImPlot3DTriangleFlags_NoFill = 1 << 11,
    ImPlot3DTriangleFlags_NoMarkers = 1 << 12,
};

enum ImPlot3DQuadFlags_ {
    ImPlot3DQuadFlags_None = 0,
    ImPlot3DQuadFlags_NoLegend = ImPlot3DItemFlags_NoLegend,
    ImPlot3DQuadFlags_NoFit = ImPlot3DItemFlags_NoFit,
    ImPlot3DQuadFlags_NoLines = 1 << 10,
    ImPlot3DQuadFlags_NoFill = 1 << 11,
    ImPlot3DQuadFlags_NoMarkers = 1 << 12,
};

enum ImPlot3DSurfaceFlags_ {
    ImPlot3DSurfaceFlags_None = 0,
    ImPlot3DSurfaceFlags_NoLegend = ImPlot3DItemFlags_NoLegend,
    ImPlot3DSurfaceFlags_NoFit = ImPlot3DItemFlags_NoFit,
    ImPlot3DSurfaceFlags_NoLines = 1 << 10,
    ImPlot3DSurfaceFlags_NoFill = 1 << 11,
    ImPlot3DSurfaceFlags_NoMarkers = 1 << 12,
};

enum ImPlot3DMeshFlags_ {
    ImPlot3DMeshFlags_None = 0,
    ImPlot3DMeshFlags_NoLegend = ImPlot3DItemFlags_NoLegend,
    ImPlot3DMeshFlags_NoFit = ImPlot3DItemFlags_NoFit,
    ImPlot3DMeshFlags_NoLines = 1 << 10,
    ImPlot3DMeshFlags_NoFill = 1 << 11,
    ImPlot3DMeshFlags_NoMarkers = 1 << 12,
};

enum ImPlot3DImageFlags_ {
    ImPlot3DImageFlags_None = 0,
    ImPlot3DImageFlags_NoLegend = ImPlot3DItemFlags_NoLegend,
    ImPlot3DImageFlags_NoFit = ImPlot3DItemFlags_NoFit,
};

enum ImPlot3DDummyFlags_ {
    ImPlot3DDummyFlags_None = 0
};

enum ImPlot3DLegendFlags_ {
    ImPlot3DLegendFlags_None = 0,
    ImPlot3DLegendFlags_NoButtons = 1 << 0,
    ImPlot3DLegendFlags_NoHighlightItem = 1 << 1,
    ImPlot3DLegendFlags_Horizontal = 1 << 2,
};

enum ImPlot3DLocation_ {
    ImPlot3DLocation_Center = 0,
    ImPlot3DLocation_North = 1 << 0,
    ImPlot3DLocation_South = 1 << 1,
    ImPlot3DLocation_West = 1 << 2,
    ImPlot3DLocation_East = 1 << 3,
    ImPlot3DLocation_NorthWest = ImPlot3DLocation_North | ImPlot3DLocation_West,
    ImPlot3DLocation_NorthEast = ImPlot3DLocation_North | ImPlot3DLocation_East,
    ImPlot3DLocation_SouthWest = ImPlot3DLocation_South | ImPlot3DLocation_West,
    ImPlot3DLocation_SouthEast = ImPlot3DLocation_South | ImPlot3DLocation_East
};

enum ImPlot3DAxisFlags_ {
    ImPlot3DAxisFlags_None = 0,
    ImPlot3DAxisFlags_NoLabel = 1 << 0,
    ImPlot3DAxisFlags_NoGridLines = 1 << 1,
    ImPlot3DAxisFlags_NoTickMarks = 1 << 2,
    ImPlot3DAxisFlags_NoTickLabels = 1 << 3,
    ImPlot3DAxisFlags_LockMin = 1 << 4,
    ImPlot3DAxisFlags_LockMax = 1 << 5,
    ImPlot3DAxisFlags_AutoFit = 1 << 6,
    ImPlot3DAxisFlags_Invert = 1 << 7,
    ImPlot3DAxisFlags_PanStretch = 1 << 8,
    ImPlot3DAxisFlags_Lock = ImPlot3DAxisFlags_LockMin | ImPlot3DAxisFlags_LockMax,
    ImPlot3DAxisFlags_NoDecorations = ImPlot3DAxisFlags_NoLabel | ImPlot3DAxisFlags_NoGridLines | ImPlot3DAxisFlags_NoTickLabels,
};

enum ImAxis3D_ {
    ImAxis3D_X = 0,
    ImAxis3D_Y,
    ImAxis3D_Z,
    ImAxis3D_COUNT,
};

enum ImPlane3D_ {
    ImPlane3D_YZ = 0,
    ImPlane3D_XZ,
    ImPlane3D_XY,
    ImPlane3D_COUNT,
};

enum ImPlot3DScale_ {
    ImPlot3DScale_Linear = 0,
    ImPlot3DScale_Log10,
    ImPlot3DScale_SymLog,
};

enum ImPlot3DColormap_ {
    ImPlot3DColormap_Deep = 0,
    ImPlot3DColormap_Dark = 1,
    ImPlot3DColormap_Pastel = 2,
    ImPlot3DColormap_Paired = 3,
    ImPlot3DColormap_Viridis = 4,
    ImPlot3DColormap_Plasma = 5,
    ImPlot3DColormap_Hot = 6,
    ImPlot3DColormap_Cool = 7,
    ImPlot3DColormap_Pink = 8,
    ImPlot3DColormap_Jet = 9,
    ImPlot3DColormap_Twilight = 10,
    ImPlot3DColormap_RdBu = 11,
    ImPlot3DColormap_BrBG = 12,
    ImPlot3DColormap_PiYG = 13,
    ImPlot3DColormap_Spectral = 14,
    ImPlot3DColormap_Greys = 15,
};

struct ImPlot3DSpec {
    ImVec4 LineColor = IMPLOT3D_AUTO_COL;
    ImU32* LineColors = nullptr;
    float LineWeight = 1.0f;
    ImVec4 FillColor = IMPLOT3D_AUTO_COL;
    ImU32* FillColors = nullptr;
    float FillAlpha = IMPLOT3D_AUTO;
    ImPlot3DMarker Marker = ImPlot3DMarker_Auto;
    float MarkerSize = IMPLOT3D_AUTO;
    float* MarkerSizes = nullptr;
    ImVec4 MarkerLineColor = IMPLOT3D_AUTO_COL;
    ImU32* MarkerLineColors = nullptr;
    ImVec4 MarkerFillColor = IMPLOT3D_AUTO_COL;
    ImU32* MarkerFillColors = nullptr;
    int Offset = 0;
    int Stride = IMPLOT3D_AUTO;
    ImPlot3DItemFlags Flags =
        ImPlot3DItemFlags_None;

    ImPlot3DSpec() {}

    template <typename... Args> ImPlot3DSpec(Args... args) {
        static_assert((sizeof...(Args)) % 2 == 0, "Odd number of arguments! You must provide (ImPlot3DProp,value) pairs!");
        SetProp(args...);
    }

    template <typename Arg, typename... Args> void SetProp(ImPlot3DProp prop, Arg arg, Args... args) {
        static_assert((sizeof...(Args)) % 2 == 0, "Odd number of arguments! You must provide (ImPlot3DProp,value) pairs!");
        SetProp(prop, arg);
        SetProp(args...);
    }

    template <typename T> void SetProp(ImPlot3DProp prop, T v) {
        switch (prop) {
            case ImPlot3DProp_LineColor: LineColor = ImGui::ColorConvertU32ToFloat4((ImU32)v); return;
            case ImPlot3DProp_LineWeight: LineWeight = (float)v; return;
            case ImPlot3DProp_FillColor: FillColor = ImGui::ColorConvertU32ToFloat4((ImU32)v); return;
            case ImPlot3DProp_FillAlpha: FillAlpha = (float)v; return;
            case ImPlot3DProp_Marker: Marker = (ImPlot3DMarker)v; return;
            case ImPlot3DProp_MarkerSize: MarkerSize = (float)v; return;
            case ImPlot3DProp_MarkerLineColor: MarkerLineColor = ImGui::ColorConvertU32ToFloat4((ImU32)v); return;
            case ImPlot3DProp_MarkerFillColor: MarkerFillColor = ImGui::ColorConvertU32ToFloat4((ImU32)v); return;
            case ImPlot3DProp_Offset: Offset = (int)v; return;
            case ImPlot3DProp_Stride: Stride = (int)v; return;
            case ImPlot3DProp_Flags: Flags = (ImPlot3DItemFlags)v; return;
            default: break;
        }
        IM_ASSERT(0 && "User provided an ImPlot3DProp which cannot be set from scalar value!");
    }

    void SetProp(ImPlot3DProp prop, ImU32* v) {
        switch (prop) {
            case ImPlot3DProp_LineColors: LineColors = v; return;
            case ImPlot3DProp_FillColors: FillColors = v; return;
            case ImPlot3DProp_MarkerLineColors: MarkerLineColors = v; return;
            case ImPlot3DProp_MarkerFillColors: MarkerFillColors = v; return;
            default: break;
        }
        IM_ASSERT(0 && "User provided an ImPlot3DProp which cannot be set from ImU32* value!");
    }

    void SetProp(ImPlot3DProp prop, float* v) {
        switch (prop) {
            case ImPlot3DProp_MarkerSizes: MarkerSizes = v; return;
            default: break;
        }
        IM_ASSERT(0 && "User provided an ImPlot3DProp which cannot be set from float* value!");
    }

    void SetProp(ImPlot3DProp prop, const ImVec4& v) {
        switch (prop) {
            case ImPlot3DProp_LineColor: LineColor = v; return;
            case ImPlot3DProp_FillColor: FillColor = v; return;
            case ImPlot3DProp_MarkerLineColor: MarkerLineColor = v; return;
            case ImPlot3DProp_MarkerFillColor: MarkerFillColor = v; return;
            default: break;
        }
        IM_ASSERT(0 && "User provided an ImPlot3DProp which cannot be set from ImVec4 value!");
    }
};

typedef int (*ImPlot3DFormatter)(double value, char* buff, int size, void* user_data);

typedef double (*ImPlot3DTransform)(double value, void* user_data);

namespace ImPlot3D {

IMPLOT3D_API ImPlot3DContext* CreateContext();

IMPLOT3D_API void DestroyContext(ImPlot3DContext* ctx = nullptr);

IMPLOT3D_API ImPlot3DContext* GetCurrentContext();

IMPLOT3D_API void SetCurrentContext(ImPlot3DContext* ctx);

IMPLOT3D_API bool BeginPlot(const char* title_id, const ImVec2& size = ImVec2(-1, 0), ImPlot3DFlags flags = 0);
IMPLOT3D_API void EndPlot();

IMPLOT3D_API void SetupAxis(ImAxis3D axis, const char* label = nullptr, ImPlot3DAxisFlags flags = 0);

IMPLOT3D_API void SetupAxisLimits(ImAxis3D axis, double v_min, double v_max, ImPlot3DCond cond = ImPlot3DCond_Once);

IMPLOT3D_API void SetupAxisFormat(ImAxis3D axis, ImPlot3DFormatter formatter, void* data = nullptr);

IMPLOT3D_API void SetupAxisTicks(ImAxis3D axis, const double* values, int n_ticks, const char* const labels[] = nullptr, bool keep_default = false);

IMPLOT3D_API void SetupAxisTicks(ImAxis3D axis, double v_min, double v_max, int n_ticks, const char* const labels[] = nullptr,
                                 bool keep_default = false);

IMPLOT3D_API void SetupAxisScale(ImAxis3D axis, ImPlot3DScale scale);

IMPLOT3D_API void SetupAxisScale(ImAxis3D axis, ImPlot3DTransform forward, ImPlot3DTransform inverse, void* data = nullptr);

IMPLOT3D_API void SetupAxisLimitsConstraints(ImAxis3D axis, double v_min, double v_max);

IMPLOT3D_API void SetupAxisZoomConstraints(ImAxis3D axis, double zoom_min, double zoom_max);

IMPLOT3D_API void SetupAxes(const char* x_label, const char* y_label, const char* z_label, ImPlot3DAxisFlags x_flags = 0,
                            ImPlot3DAxisFlags y_flags = 0, ImPlot3DAxisFlags z_flags = 0);

IMPLOT3D_API void SetupAxesLimits(double x_min, double x_max, double y_min, double y_max, double z_min, double z_max,
                                  ImPlot3DCond cond = ImPlot3DCond_Once);

IMPLOT3D_API void SetupBoxRotation(double elevation, double azimuth, bool animate = false, ImPlot3DCond cond = ImPlot3DCond_Once);

IMPLOT3D_API void SetupBoxRotation(ImPlot3DQuat rotation, bool animate = false, ImPlot3DCond cond = ImPlot3DCond_Once);

IMPLOT3D_API void SetupBoxInitialRotation(double elevation, double azimuth);

IMPLOT3D_API void SetupBoxInitialRotation(ImPlot3DQuat rotation);

IMPLOT3D_API void SetupBoxScale(double x, double y, double z);

IMPLOT3D_API void SetupLegend(ImPlot3DLocation location, ImPlot3DLegendFlags flags = 0);

IMPLOT3D_TMP void PlotScatter(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_TMP void PlotLine(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_TMP void PlotTriangle(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_TMP void PlotQuad(const char* label_id, const T* xs, const T* ys, const T* zs, int count, const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_TMP void PlotSurface(const char* label_id, const T* xs, const T* ys, const T* zs, int x_count, int y_count, double scale_min = 0.0,
                              double scale_max = 0.0, const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_TMP void PlotMesh(const char* label_id, const T* vtx_xs, const T* vtx_ys, const T* vtx_zs, const unsigned int* idxs, int vtx_count,
                           int idx_count, const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_API void PlotImage(const char* label_id, ImTextureRef tex_ref, const ImPlot3DPoint& center, const ImPlot3DPoint& axis_u,
                            const ImPlot3DPoint& axis_v, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1),
                            const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_API void PlotImage(const char* label_id, ImTextureRef tex_ref, const ImPlot3DPoint& p0, const ImPlot3DPoint& p1, const ImPlot3DPoint& p2,
                            const ImPlot3DPoint& p3, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 0),
                            const ImVec2& uv2 = ImVec2(1, 1), const ImVec2& uv3 = ImVec2(0, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1),
                            const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_API void PlotText(const char* text, double x, double y, double z, double angle = 0.0, const ImVec2& pix_offset = ImVec2(0, 0));

IMPLOT3D_API void PlotDummy(const char* label_id, const ImPlot3DSpec& spec = ImPlot3DSpec());

IMPLOT3D_API ImVec2 PlotToPixels(const ImPlot3DPoint& point);
IMPLOT3D_API ImVec2 PlotToPixels(double x, double y, double z);

IMPLOT3D_API ImPlot3DRay PixelsToPlotRay(const ImVec2& pix);
IMPLOT3D_API ImPlot3DRay PixelsToPlotRay(double x, double y);

IMPLOT3D_API ImPlot3DPoint PixelsToPlotPlane(const ImVec2& pix, ImPlane3D plane, bool mask = true);
IMPLOT3D_API ImPlot3DPoint PixelsToPlotPlane(double x, double y, ImPlane3D plane, bool mask = true);

IMPLOT3D_API ImVec2 GetPlotRectPos();

IMPLOT3D_API ImVec2 GetPlotRectSize();

IMPLOT3D_API ImDrawList* GetPlotDrawList();

IMPLOT3D_API ImPlot3DStyle& GetStyle();
IMPLOT3D_API void SetStyle(const ImPlot3DStyle& style);

IMPLOT3D_API void StyleColorsAuto(ImPlot3DStyle* dst = nullptr);
IMPLOT3D_API void StyleColorsDark(ImPlot3DStyle* dst = nullptr);
IMPLOT3D_API void StyleColorsLight(ImPlot3DStyle* dst = nullptr);
IMPLOT3D_API void StyleColorsClassic(ImPlot3DStyle* dst = nullptr);

IMPLOT3D_API void PushStyleColor(ImPlot3DCol idx, ImU32 col);
IMPLOT3D_API void PushStyleColor(ImPlot3DCol idx, const ImVec4& col);

IMPLOT3D_API void PopStyleColor(int count = 1);

IMPLOT3D_API void PushStyleVar(ImPlot3DStyleVar idx, float val);

IMPLOT3D_API void PushStyleVar(ImPlot3DStyleVar idx, int val);

IMPLOT3D_API void PushStyleVar(ImPlot3DStyleVar idx, const ImVec2& val);

IMPLOT3D_API void PopStyleVar(int count = 1);

IMPLOT3D_API ImVec4 GetStyleColorVec4(ImPlot3DCol idx);
IMPLOT3D_API ImU32 GetStyleColorU32(ImPlot3DCol idx);

IMPLOT3D_API ImPlot3DMarker NextMarker();

IMPLOT3D_API ImPlot3DColormap AddColormap(const char* name, const ImVec4* cols, int size, bool qual = true);
IMPLOT3D_API ImPlot3DColormap AddColormap(const char* name, const ImU32* cols, int size, bool qual = true);

IMPLOT3D_API int GetColormapCount();

IMPLOT3D_API const char* GetColormapName(ImPlot3DColormap cmap);

IMPLOT3D_API ImPlot3DColormap GetColormapIndex(const char* name);

IMPLOT3D_API void PushColormap(ImPlot3DColormap cmap);

IMPLOT3D_API void PushColormap(const char* name);

IMPLOT3D_API void PopColormap(int count = 1);

IMPLOT3D_API ImVec4 NextColormapColor();

IMPLOT3D_API int GetColormapSize(ImPlot3DColormap cmap = IMPLOT3D_AUTO);

IMPLOT3D_API ImVec4 GetColormapColor(int idx, ImPlot3DColormap cmap = IMPLOT3D_AUTO);

IMPLOT3D_API ImVec4 SampleColormap(float t, ImPlot3DColormap cmap = IMPLOT3D_AUTO);

IMPLOT3D_API void ShowDemoWindow(bool* p_open = nullptr);

IMPLOT3D_API void ShowAllDemos();

IMPLOT3D_API void ShowStyleEditor(ImPlot3DStyle* ref = nullptr);

IMPLOT3D_API bool ShowStyleSelector(const char* label);

IMPLOT3D_API bool ShowColormapSelector(const char* label);

IMPLOT3D_API void ShowMetricsWindow(bool* p_popen = nullptr);

IMPLOT3D_API void ShowAboutWindow(bool* p_open = nullptr);

}

struct ImPlot3DPoint {
    double x, y, z;
    constexpr ImPlot3DPoint() : x(0.0), y(0.0), z(0.0) {}
    constexpr ImPlot3DPoint(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}

    double& operator[](size_t idx) {
        IM_ASSERT(idx == 0 || idx == 1 || idx == 2);
        return ((double*)(void*)(char*)this)[idx];
    }
    double operator[](size_t idx) const {
        IM_ASSERT(idx == 0 || idx == 1 || idx == 2);
        return ((const double*)(const void*)(const char*)this)[idx];
    }

    IMPLOT3D_API ImPlot3DPoint operator*(double rhs) const;
    IMPLOT3D_API ImPlot3DPoint operator/(double rhs) const;
    IMPLOT3D_API ImPlot3DPoint operator+(const ImPlot3DPoint& rhs) const;
    IMPLOT3D_API ImPlot3DPoint operator-(const ImPlot3DPoint& rhs) const;
    IMPLOT3D_API ImPlot3DPoint operator*(const ImPlot3DPoint& rhs) const;
    IMPLOT3D_API ImPlot3DPoint operator/(const ImPlot3DPoint& rhs) const;

    IMPLOT3D_API ImPlot3DPoint operator-() const;

    IMPLOT3D_API ImPlot3DPoint& operator*=(double rhs);
    IMPLOT3D_API ImPlot3DPoint& operator/=(double rhs);
    IMPLOT3D_API ImPlot3DPoint& operator+=(const ImPlot3DPoint& rhs);
    IMPLOT3D_API ImPlot3DPoint& operator-=(const ImPlot3DPoint& rhs);
    IMPLOT3D_API ImPlot3DPoint& operator*=(const ImPlot3DPoint& rhs);
    IMPLOT3D_API ImPlot3DPoint& operator/=(const ImPlot3DPoint& rhs);

    IMPLOT3D_API bool operator==(const ImPlot3DPoint& rhs) const;
    IMPLOT3D_API bool operator!=(const ImPlot3DPoint& rhs) const;

    IMPLOT3D_API double Dot(const ImPlot3DPoint& rhs) const;

    IMPLOT3D_API ImPlot3DPoint Cross(const ImPlot3DPoint& rhs) const;

    IMPLOT3D_API double Length() const;

    IMPLOT3D_API double LengthSquared() const;

    IMPLOT3D_API void Normalize();

    IMPLOT3D_API ImPlot3DPoint Normalized() const;

    IMPLOT3D_API friend ImPlot3DPoint operator*(double lhs, const ImPlot3DPoint& rhs);

    IMPLOT3D_API bool IsNaN() const;

#ifdef IMPLOT3D_POINT_CLASS_EXTRA
    IMPLOT3D_POINT_CLASS_EXTRA

#endif
};

struct ImPlot3DRay {
    ImPlot3DPoint Origin;
    ImPlot3DPoint Direction;
};

struct ImPlot3DPlane {
    ImPlot3DPoint Point;
    ImPlot3DPoint Normal;
};

struct ImPlot3DBox {
    ImPlot3DPoint Min;
    ImPlot3DPoint Max;

    constexpr ImPlot3DBox() : Min(ImPlot3DPoint()), Max(ImPlot3DPoint()) {}

    constexpr ImPlot3DBox(const ImPlot3DPoint& min, const ImPlot3DPoint& max) : Min(min), Max(max) {}

    IMPLOT3D_API void Expand(const ImPlot3DPoint& point);

    IMPLOT3D_API bool Contains(const ImPlot3DPoint& point) const;

    IMPLOT3D_API bool ClipLineSegment(const ImPlot3DPoint& p0, const ImPlot3DPoint& p1, ImPlot3DPoint& p0_clipped, ImPlot3DPoint& p1_clipped) const;
};

struct ImPlot3DRange {
    double Min;
    double Max;

    constexpr ImPlot3DRange() : Min(0.0), Max(0.0) {}
    constexpr ImPlot3DRange(double min, double max) : Min(min), Max(max) {}

    IMPLOT3D_API void Expand(double value);
    IMPLOT3D_API bool Contains(double value) const;
    double Size() const { return Max - Min; }
};

struct ImPlot3DQuat {
    double x, y, z, w;

    constexpr ImPlot3DQuat() : x(0.0), y(0.0), z(0.0), w(1.0) {}
    constexpr ImPlot3DQuat(double _x, double _y, double _z, double _w) : x(_x), y(_y), z(_z), w(_w) {}

    IMPLOT3D_API ImPlot3DQuat(double _angle, const ImPlot3DPoint& _axis);

    IMPLOT3D_API static ImPlot3DQuat FromTwoVectors(const ImPlot3DPoint& v0, const ImPlot3DPoint& v1);

    IMPLOT3D_API static ImPlot3DQuat FromElAz(double elevation, double azimuth);

    IMPLOT3D_API double Length() const;

    IMPLOT3D_API ImPlot3DQuat Normalized() const;

    IMPLOT3D_API ImPlot3DQuat Conjugate() const;

    IMPLOT3D_API ImPlot3DQuat Inverse() const;

    IMPLOT3D_API ImPlot3DQuat operator*(const ImPlot3DQuat& rhs) const;

    IMPLOT3D_API ImPlot3DQuat& Normalize();

    IMPLOT3D_API ImPlot3DPoint operator*(const ImPlot3DPoint& point) const;

    IMPLOT3D_API bool operator==(const ImPlot3DQuat& rhs) const;
    IMPLOT3D_API bool operator!=(const ImPlot3DQuat& rhs) const;

    IMPLOT3D_API static ImPlot3DQuat Slerp(const ImPlot3DQuat& q1, const ImPlot3DQuat& q2, double t);

    IMPLOT3D_API double Dot(const ImPlot3DQuat& rhs) const;

#ifdef IMPLOT3D_QUAT_CLASS_EXTRA
    IMPLOT3D_QUAT_CLASS_EXTRA

#endif
};

struct ImPlot3DStyle {

    float LineWeight;
    int Marker;
    float MarkerSize;
    float FillAlpha;

    ImVec2 PlotDefaultSize;
    ImVec2 PlotMinSize;
    ImVec2 PlotPadding;
    ImVec2 LabelPadding;
    float ViewScaleFactor;

    ImVec2 LegendPadding;
    ImVec2 LegendInnerPadding;
    ImVec2 LegendSpacing;

    ImVec4 Colors[ImPlot3DCol_COUNT];
    inline ImVec4 GetColor(ImPlot3DCol idx) const { return Colors[idx]; }
    inline void SetColor(ImPlot3DCol idx, const ImVec4& col) { Colors[idx] = col; }

    ImPlot3DColormap Colormap;

    IMPLOT3D_API ImPlot3DStyle();
    ImPlot3DStyle(const ImPlot3DStyle& other) = default;
    ImPlot3DStyle& operator=(const ImPlot3DStyle& other) = default;
};

namespace ImPlot3D {

constexpr int CUBE_VTX_COUNT = 8;
constexpr int CUBE_IDX_COUNT = 36;
extern ImPlot3DPoint cube_vtx[CUBE_VTX_COUNT];
extern unsigned int cube_idx[CUBE_IDX_COUNT];

constexpr int SPHERE_VTX_COUNT = 162;
constexpr int SPHERE_IDX_COUNT = 960;
extern ImPlot3DPoint sphere_vtx[SPHERE_VTX_COUNT];
extern unsigned int sphere_idx[SPHERE_IDX_COUNT];

constexpr int DUCK_VTX_COUNT = 254;
constexpr int DUCK_IDX_COUNT = 1428;
extern ImPlot3DPoint duck_vtx[DUCK_VTX_COUNT];
extern unsigned int duck_idx[DUCK_IDX_COUNT];

}

#ifndef IMPLOT3D_DISABLE_OBSOLETE_FUNCTIONS

#ifndef IMPLOT3D_DISABLE_OBSOLETE_WARNINGS
#if __cplusplus > 201402L
#define IMPLOT3D_DEPRECATED(method) [[deprecated]] method
#elif defined(__GNUC__) && !defined(__INTEL_COMPILER) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define IMPLOT3D_DEPRECATED(method) method __attribute__((deprecated))
#elif defined(_MSC_VER)
#define IMPLOT3D_DEPRECATED(method) __declspec(deprecated) method
#else
#define IMPLOT3D_DEPRECATED(method) method
#endif
#else
#define IMPLOT3D_DEPRECATED(method) method
#endif

namespace ImPlot3D {

IMPLOT3D_DEPRECATED(IMPLOT3D_API ImVec2 GetPlotPos());
IMPLOT3D_DEPRECATED(IMPLOT3D_API ImVec2 GetPlotSize());

IMPLOT3D_DEPRECATED(IMPLOT3D_API void PlotMesh(const char* label_id, const ImPlot3DPoint* vtx, const unsigned int* idxs, int vtx_count, int idx_count,
                                               const ImPlot3DSpec& spec = ImPlot3DSpec()));

}

#endif

#endif
