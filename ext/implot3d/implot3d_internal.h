
#pragma once

#ifndef IMPLOT3D_VERSION
#include "implot3d.h"
#endif

#ifndef IMGUI_DISABLE
#include "imgui_internal.h"

#define IMPLOT3D_LABEL_FORMAT "%g"

#define IMPLOT3D_LABEL_MAX_SIZE 32

namespace ImPlot3D {

static inline double ImLog10(double x) { return log10(x); }

static inline double ImSinh(double x) { return sinh(x); }

static inline double ImAsinh(double x) { return asinh(x); }

template <typename TSet, typename TFlag> static inline bool ImHasFlag(TSet set, TFlag flag) { return (set & flag) == flag; }

template <typename TSet, typename TFlag> static inline void ImFlipFlag(TSet& set, TFlag flag) { ImHasFlag(set, flag) ? set &= ~flag : set |= flag; }
template <typename T> static inline T ImRemap01(T x, T x0, T x1) { return (x1 - x0) ? ((x - x0) / (x1 - x0)) : 0; }

static inline int ImPosMod(int l, int r) { return (l % r + r) % r; }

static inline bool ImNan(double val) { return isnan(val); }

static inline bool ImNanOrInf(double val) { return !(val >= -DBL_MAX && val <= DBL_MAX) || ImNan(val); }

static inline double ImConstrainNan(double val) { return ImNan(val) ? 0 : val; }

static inline double ImConstrainInf(double val) {
    const double max_val = DBL_MAX * 0.5;
    return val >= max_val ? max_val : val <= -max_val ? -max_val : val;
}

static inline bool ImAlmostEqual(double v1, double v2, int ulp = 2) {
    return ImAbs(v1 - v2) < DBL_EPSILON * ImAbs(v1 + v2) * ulp || ImAbs(v1 - v2) < DBL_MIN;
}

static inline ImU32 ImAlphaU32(ImU32 col, float alpha) { return col & ~((ImU32)((1.0f - alpha) * 255) << IM_COL32_A_SHIFT); }

static inline ImU32 ImMixU32(ImU32 a, ImU32 b, ImU32 s) {
#ifdef IMPLOT3D_MIX64
    const ImU32 af = 256 - s;
    const ImU32 bf = s;
    const ImU64 al = (a & 0x00ff00ff) | (((ImU64)(a & 0xff00ff00)) << 24);
    const ImU64 bl = (b & 0x00ff00ff) | (((ImU64)(b & 0xff00ff00)) << 24);
    const ImU64 mix = (al * af + bl * bf);
    return ((mix >> 32) & 0xff00ff00) | ((mix & 0xff00ff00) >> 8);
#else
    const ImU32 af = 256 - s;
    const ImU32 bf = s;
    const ImU32 al = (a & 0x00ff00ff);
    const ImU32 ah = (a & 0xff00ff00) >> 8;
    const ImU32 bl = (b & 0x00ff00ff);
    const ImU32 bh = (b & 0xff00ff00) >> 8;
    const ImU32 ml = (al * af + bl * bf);
    const ImU32 mh = (ah * af + bh * bf);
    return (mh & 0xff00ff00) | ((ml & 0xff00ff00) >> 8);
#endif
}

template <typename T> void FillRange(ImVector<T>& buffer, int n, T vmin, T vmax) {
    buffer.resize(n);
    T step = (vmax - vmin) / (n - 1);
    for (int i = 0; i < n; ++i) {
        buffer[i] = vmin + i * step;
    }
}

}

enum ImPlot3DMarkerInternal_ {
    ImPlot3DMarker_Invalid = -3,
};

struct ImPlot3DTicker;

typedef void (*ImPlot3DLocator)(ImPlot3DTicker& ticker, const ImPlot3DRange& range, float pixels, ImPlot3DFormatter formatter, void* formatter_data);

struct ImDrawList3D {

    struct ImTextureBufferItem {
        ImTextureRef TexRef;
        unsigned int VtxIdx;
    };

    ImVector<ImDrawIdx> IdxBuffer;
    ImVector<ImDrawVert> VtxBuffer;
    ImVector<double> ZBuffer;
    unsigned int _VtxCurrentIdx;
    ImDrawVert* _VtxWritePtr;
    ImDrawIdx* _IdxWritePtr;
    double* _ZWritePtr;
    ImDrawListFlags _Flags;
    ImVector<ImTextureBufferItem> _TextureBuffer;
    ImDrawListSharedData* _SharedData;

    ImDrawList3D() {
        _Flags = ImDrawListFlags_None;
        _SharedData = nullptr;
        ResetBuffers();
    }

    void PrimReserve(int idx_count, int vtx_count);
    void PrimUnreserve(int idx_count, int vtx_count);

    void SetTexture(ImTextureRef tex_ref);
    void ResetTexture();

    void SortedMoveToImGuiDrawList();

    void ResetBuffers() {
        IdxBuffer.clear();
        VtxBuffer.clear();
        ZBuffer.clear();
        _VtxCurrentIdx = 0;
        _VtxWritePtr = VtxBuffer.Data;
        _IdxWritePtr = IdxBuffer.Data;
        _ZWritePtr = ZBuffer.Data;
        _TextureBuffer.clear();
        ResetTexture();
    }

    constexpr static unsigned int MaxIdx() { return sizeof(ImDrawIdx) == 2 ? 65535 : 4294967295; }
};

struct ImPlot3DNextItemData {
    ImPlot3DSpec Spec;
    bool RenderLine;
    bool RenderFill;
    bool RenderMarkerLine;
    bool RenderMarkerFill;
    bool IsAutoFill;
    bool IsAutoLine;
    bool Hidden;

    ImPlot3DNextItemData() { Reset(); }

    void Reset() {
        Spec = ImPlot3DSpec();
        RenderLine = false;
        RenderFill = false;
        RenderMarkerLine = true;
        RenderMarkerFill = true;
        IsAutoFill = true;
        IsAutoLine = true;
        Hidden = false;
    }
};

struct ImPlot3DColormapData {
    ImVector<ImU32> Keys;
    ImVector<int> KeyCounts;
    ImVector<int> KeyOffsets;
    ImVector<ImU32> Tables;
    ImVector<int> TableSizes;
    ImVector<int> TableOffsets;
    ImGuiTextBuffer Text;
    ImVector<int> TextOffsets;
    ImVector<bool> Quals;
    ImGuiStorage Map;
    int Count;

    ImPlot3DColormapData() { Count = 0; }

    int Append(const char* name, const ImU32* keys, int count, bool qual) {
        if (GetIndex(name) != -1)
            return -1;
        KeyOffsets.push_back(Keys.size());
        KeyCounts.push_back(count);
        Keys.reserve(Keys.size() + count);
        for (int i = 0; i < count; ++i)
            Keys.push_back(keys[i]);
        TextOffsets.push_back(Text.size());
        Text.append(name, name + strlen(name) + 1);
        Quals.push_back(qual);
        ImGuiID id = ImHashStr(name);
        int idx = Count++;
        Map.SetInt(id, idx);
        _AppendTable(idx);
        return idx;
    }

    void _AppendTable(ImPlot3DColormap cmap) {
        int key_count = GetKeyCount(cmap);
        const ImU32* keys = GetKeys(cmap);
        int off = Tables.size();
        TableOffsets.push_back(off);
        if (IsQual(cmap)) {
            Tables.reserve(key_count);
            for (int i = 0; i < key_count; ++i)
                Tables.push_back(keys[i]);
            TableSizes.push_back(key_count);
        } else {
            int max_size = 255 * (key_count - 1) + 1;
            Tables.reserve(off + max_size);

            for (int i = 0; i < key_count - 1; ++i) {
                for (int s = 0; s < 255; ++s) {
                    ImU32 a = keys[i];
                    ImU32 b = keys[i + 1];
                    ImU32 c = ImPlot3D::ImMixU32(a, b, (ImU32)s);

                    Tables.push_back(c);

                }
            }
            ImU32 c = keys[key_count - 1];

            Tables.push_back(c);

            TableSizes.push_back(max_size);
        }
    }

    void RebuildTables() {
        Tables.resize(0);
        TableSizes.resize(0);
        TableOffsets.resize(0);
        for (int i = 0; i < Count; ++i)
            _AppendTable(i);
    }

    inline bool IsQual(ImPlot3DColormap cmap) const { return Quals[cmap]; }
    inline const char* GetName(ImPlot3DColormap cmap) const { return cmap < Count ? Text.Buf.Data + TextOffsets[cmap] : nullptr; }
    inline ImPlot3DColormap GetIndex(const char* name) const {
        ImGuiID key = ImHashStr(name);
        return Map.GetInt(key, -1);
    }

    inline const ImU32* GetKeys(ImPlot3DColormap cmap) const { return &Keys[KeyOffsets[cmap]]; }
    inline int GetKeyCount(ImPlot3DColormap cmap) const { return KeyCounts[cmap]; }
    inline ImU32 GetKeyColor(ImPlot3DColormap cmap, int idx) const { return Keys[KeyOffsets[cmap] + idx]; }
    inline void SetKeyColor(ImPlot3DColormap cmap, int idx, ImU32 value) {
        Keys[KeyOffsets[cmap] + idx] = value;
        RebuildTables();
    }

    inline const ImU32* GetTable(ImPlot3DColormap cmap) const { return &Tables[TableOffsets[cmap]]; }
    inline int GetTableSize(ImPlot3DColormap cmap) const { return TableSizes[cmap]; }
    inline ImU32 GetTableColor(ImPlot3DColormap cmap, int idx) const { return Tables[TableOffsets[cmap] + idx]; }

    inline ImU32 LerpTable(ImPlot3DColormap cmap, float t) const {
        int off = TableOffsets[cmap];
        int siz = TableSizes[cmap];
        int idx = Quals[cmap] ? ImClamp((int)(siz * t), 0, siz - 1) : (int)((siz - 1) * t + 0.5f);
        return Tables[off + idx];
    }
};

struct ImPlot3DItem {
    ImGuiID ID;
    ImU32 Color;
    ImPlot3DMarker Marker;
    int NameOffset;
    bool Show;
    bool LegendHovered;
    bool SeenThisFrame;

    ImPlot3DItem() {
        ID = 0;
        Color = IM_COL32_WHITE;
        Marker = ImPlot3DMarker_None;
        NameOffset = -1;
        Show = true;
        LegendHovered = false;
        SeenThisFrame = false;
    }
    ~ImPlot3DItem() { ID = 0; }
};

struct ImPlot3DLegend {
    ImPlot3DLegendFlags Flags;
    ImPlot3DLegendFlags PreviousFlags;
    ImPlot3DLocation Location;
    ImPlot3DLocation PreviousLocation;
    ImVec2 Scroll;
    ImVector<int> Indices;
    ImGuiTextBuffer Labels;
    ImRect Rect;
    ImRect RectClamped;
    bool Hovered;
    bool Held;

    ImPlot3DLegend() {
        PreviousFlags = Flags = ImPlot3DLegendFlags_None;
        Hovered = Held = false;
        PreviousLocation = Location = ImPlot3DLocation_NorthWest;
        Scroll = ImVec2(0, 0);
    }

    void Reset() {
        Indices.shrink(0);
        Labels.Buf.shrink(0);
    }
};

struct ImPlot3DItemGroup {
    ImGuiID ID;
    ImPool<ImPlot3DItem> ItemPool;
    ImPlot3DLegend Legend;
    int ColormapIdx;
    ImPlot3DMarker MarkerIdx;

    ImPlot3DItemGroup() {
        ID = 0;
        ColormapIdx = 0;
        MarkerIdx = 0;
    }

    int GetItemCount() const { return ItemPool.GetBufSize(); }
    ImGuiID GetItemID(const char* label_id) { return ImGui::GetID(label_id); }
    ImPlot3DItem* GetItem(ImGuiID id) { return ItemPool.GetByKey(id); }
    ImPlot3DItem* GetItem(const char* label_id) { return GetItem(GetItemID(label_id)); }
    ImPlot3DItem* GetOrAddItem(ImGuiID id) { return ItemPool.GetOrAddByKey(id); }
    ImPlot3DItem* GetItemByIndex(int i) { return ItemPool.GetByIndex(i); }
    int GetItemIndex(ImPlot3DItem* item) { return ItemPool.GetIndex(item); }
    int GetLegendCount() const { return Legend.Indices.size(); }
    ImPlot3DItem* GetLegendItem(int i) { return ItemPool.GetByIndex(Legend.Indices[i]); }
    const char* GetLegendLabel(int i) { return Legend.Labels.Buf.Data + GetLegendItem(i)->NameOffset; }
    void Reset() {
        ItemPool.Clear();
        Legend.Reset();
        ColormapIdx = 0;
        MarkerIdx = 0;
    }
};

struct ImPlot3DTick {
    double PlotPos;
    bool Major;
    bool ShowLabel;
    ImVec2 LabelSize;
    int TextOffset;
    int Idx;

    ImPlot3DTick(double value, bool major, bool show_label) {
        PlotPos = value;
        Major = major;
        ShowLabel = show_label;
        TextOffset = -1;
    }
};

struct ImPlot3DTicker {
    ImVector<ImPlot3DTick> Ticks;
    ImGuiTextBuffer TextBuffer;

    ImPlot3DTicker() { Reset(); }

    ImPlot3DTick& AddTick(double value, bool major, bool show_label, const char* label) {
        ImPlot3DTick tick(value, major, show_label);
        if (show_label && label != nullptr) {
            tick.TextOffset = TextBuffer.size();
            TextBuffer.append(label, label + strlen(label) + 1);
            tick.LabelSize = ImGui::CalcTextSize(TextBuffer.Buf.Data + tick.TextOffset);
        }
        return AddTick(tick);
    }

    ImPlot3DTick& AddTick(double value, bool major, bool show_label, ImPlot3DFormatter formatter, void* data) {
        ImPlot3DTick tick(value, major, show_label);
        if (show_label && formatter != nullptr) {
            char buff[IMPLOT3D_LABEL_MAX_SIZE];
            tick.TextOffset = TextBuffer.size();
            formatter(tick.PlotPos, buff, sizeof(buff), data);
            TextBuffer.append(buff, buff + strlen(buff) + 1);
            tick.LabelSize = ImGui::CalcTextSize(TextBuffer.Buf.Data + tick.TextOffset);
        }
        return AddTick(tick);
    }

    inline ImPlot3DTick& AddTick(ImPlot3DTick tick) {
        tick.Idx = Ticks.size();
        Ticks.push_back(tick);
        return Ticks.back();
    }

    const char* GetText(int idx) const { return TextBuffer.Buf.Data + Ticks[idx].TextOffset; }

    const char* GetText(const ImPlot3DTick& tick) const { return GetText(tick.Idx); }

    void Reset() {
        Ticks.shrink(0);
        TextBuffer.Buf.shrink(0);
    }

    int TickCount() const { return Ticks.Size; }
};

struct ImPlot3DAxis {

    ImPlot3DAxisFlags Flags;
    ImPlot3DAxisFlags PreviousFlags;

    ImPlot3DRange Range;
    ImPlot3DCond RangeCond;
    double NDCScale;
    ImPlot3DScale Scale;

    ImGuiTextBuffer Label;

    ImPlot3DTicker Ticker;
    ImPlot3DFormatter Formatter;
    void* FormatterData;
    ImPlot3DLocator Locator;
    bool ShowDefaultTicks;

    ImPlot3DTransform TransformForward;
    ImPlot3DTransform TransformInverse;
    void* TransformData;
    ImPlot3DRange ScaledRange;

    bool FitThisFrame;
    ImPlot3DRange FitExtents;

    ImPlot3DRange ConstraintRange;
    ImPlot3DRange ConstraintZoom;

    bool Hovered;
    bool Held;

    ImU32 ColorBg;
    ImU32 ColorHov;
    ImU32 ColorAct;

    ImPlot3DAxis() {
        PreviousFlags = Flags = ImPlot3DAxisFlags_None;

        Range.Min = 0.0;
        Range.Max = 1.0;
        RangeCond = ImPlot3DCond_None;

        NDCScale = 1.0;
        Scale = ImPlot3DScale_Linear;
        TransformForward = TransformInverse = nullptr;
        TransformData = nullptr;

        Formatter = nullptr;
        FormatterData = nullptr;
        Locator = nullptr;
        ShowDefaultTicks = true;

        FitThisFrame = true;
        FitExtents = ImPlot3DRange(HUGE_VAL, -HUGE_VAL);

        ConstraintRange = ImPlot3DRange(-INFINITY, INFINITY);
        ConstraintZoom = ImPlot3DRange(DBL_MIN, INFINITY);

        Hovered = false;
        Held = false;

        ColorBg = ColorHov = ColorAct = IM_COL32_BLACK_TRANS;
    }

    inline void Reset() {
        RangeCond = ImPlot3DCond_None;

        Scale = ImPlot3DScale_Linear;
        TransformForward = TransformInverse = nullptr;
        TransformData = nullptr;

        Ticker.Reset();
        Formatter = nullptr;
        FormatterData = nullptr;
        Locator = nullptr;
        ShowDefaultTicks = true;

        FitExtents = ImPlot3DRange(HUGE_VAL, -HUGE_VAL);

        ConstraintRange = ImPlot3DRange(-INFINITY, INFINITY);
        ConstraintZoom = ImPlot3DRange(DBL_MIN, INFINITY);
    }

    inline void SetRange(double v1, double v2) {
        v1 = ImPlot3D::ImConstrainNan(ImPlot3D::ImConstrainInf(v1));
        v2 = ImPlot3D::ImConstrainNan(ImPlot3D::ImConstrainInf(v2));
        Range.Min = ImMin(v1, v2);
        Range.Max = ImMax(v1, v2);
        Constrain();
        UpdateTransformCache();
    }

    inline bool SetMin(double _min, bool force = false) {
        if (!force && IsLockedMin())
            return false;
        _min = ImPlot3D::ImConstrainNan(ImPlot3D::ImConstrainInf(_min));

        if (_min < ConstraintRange.Min)
            _min = ConstraintRange.Min;
        double zoom = Range.Max - _min;
        if (zoom < ConstraintZoom.Min)
            _min = Range.Max - ConstraintZoom.Min;
        if (zoom > ConstraintZoom.Max)
            _min = Range.Max - ConstraintZoom.Max;

        if (_min >= Range.Max)
            return false;

        Range.Min = _min;
        UpdateTransformCache();
        return true;
    }

    inline bool SetMax(double _max, bool force = false) {
        if (!force && IsLockedMax())
            return false;
        _max = ImPlot3D::ImConstrainNan(ImPlot3D::ImConstrainInf(_max));

        if (_max > ConstraintRange.Max)
            _max = ConstraintRange.Max;
        double zoom = _max - Range.Min;
        if (zoom < ConstraintZoom.Min)
            _max = Range.Min + ConstraintZoom.Min;
        if (zoom > ConstraintZoom.Max)
            _max = Range.Min + ConstraintZoom.Max;

        if (_max <= Range.Min)
            return false;

        Range.Max = _max;
        UpdateTransformCache();
        return true;
    }

    inline void Constrain() {
        Range.Min = ImPlot3D::ImConstrainNan(ImPlot3D::ImConstrainInf(Range.Min));
        Range.Max = ImPlot3D::ImConstrainNan(ImPlot3D::ImConstrainInf(Range.Max));
        if (Range.Min < ConstraintRange.Min)
            Range.Min = ConstraintRange.Min;
        if (Range.Max > ConstraintRange.Max)
            Range.Max = ConstraintRange.Max;
        double zoom = Range.Size();
        if (zoom < ConstraintZoom.Min) {
            double delta = (ConstraintZoom.Min - zoom) * 0.5;
            Range.Min -= delta;
            Range.Max += delta;
        }
        if (zoom > ConstraintZoom.Max) {
            double delta = (zoom - ConstraintZoom.Max) * 0.5;
            Range.Min += delta;
            Range.Max -= delta;
        }
        if (Range.Max <= Range.Min)
            Range.Max = Range.Min + DBL_EPSILON;
    }

    inline void UpdateTransformCache() {
        if (TransformForward != nullptr) {
            ScaledRange.Min = TransformForward(Range.Min, TransformData);
            ScaledRange.Max = TransformForward(Range.Max, TransformData);
        } else {
            ScaledRange.Min = Range.Min;
            ScaledRange.Max = Range.Max;
        }
    }

    inline double PlotToNDC(double plt) const {
        if (TransformForward != nullptr) {
            double s = TransformForward(plt, TransformData);
            return (s - ScaledRange.Min) / (ScaledRange.Max - ScaledRange.Min);
        }
        return (plt - Range.Min) / (Range.Max - Range.Min);
    }

    inline double NDCToPlot(double t) const {
        if (TransformInverse != nullptr) {
            double s = t * (ScaledRange.Max - ScaledRange.Min) + ScaledRange.Min;
            return TransformInverse(s, TransformData);
        }
        return Range.Min + t * (Range.Max - Range.Min);
    }

    inline bool IsRangeLocked() const { return RangeCond == ImPlot3DCond_Always; }
    inline bool IsLockedMin() const { return IsRangeLocked() || ImPlot3D::ImHasFlag(Flags, ImPlot3DAxisFlags_LockMin); }
    inline bool IsLockedMax() const { return IsRangeLocked() || ImPlot3D::ImHasFlag(Flags, ImPlot3DAxisFlags_LockMax); }
    inline bool IsLocked() const { return IsLockedMin() && IsLockedMax(); }
    inline bool IsInputLockedMin() const { return IsLockedMin() || IsAutoFitting(); }
    inline bool IsInputLockedMax() const { return IsLockedMax() || IsAutoFitting(); }
    inline bool IsInputLocked() const { return IsLocked() || IsAutoFitting(); }

    inline bool IsPanLocked(bool increasing) {
        if (ImPlot3D::ImHasFlag(Flags, ImPlot3DAxisFlags_PanStretch)) {
            return IsInputLocked();
        } else {
            if (IsLockedMin() || IsLockedMax() || IsAutoFitting())
                return false;
            if (increasing)
                return Range.Max == ConstraintRange.Max;
            else
                return Range.Min == ConstraintRange.Min;
        }
    }

    inline void SetLabel(const char* label) {
        Label.Buf.shrink(0);
        if (label && ImGui::FindRenderedTextEnd(label, nullptr) != label)
            Label.append(label, label + strlen(label) + 1);
    }

    inline const char* GetLabel() const { return Label.Buf.Data; }

    inline double NDCSize() const {

        return NDCScale;
    }

    inline void SetAspect(double units_per_ndc_unit) {
        double new_size = units_per_ndc_unit * NDCSize();
        double delta = (new_size - Range.Size()) * 0.5;
        if (IsLocked())
            return;
        else if (IsLockedMin() && !IsLockedMax())
            SetRange(Range.Min, Range.Max + 2 * delta);
        else if (!IsLockedMin() && IsLockedMax())
            SetRange(Range.Min - 2 * delta, Range.Max);
        else
            SetRange(Range.Min - delta, Range.Max + delta);
    }

    double GetAspect() const { return Range.Size() / NDCSize(); }

    bool HasLabel() const;
    bool HasGridLines() const;
    bool HasTickLabels() const;
    bool HasTickMarks() const;
    bool IsAutoFitting() const;
    void ExtendFit(double value);
    void ApplyFit();
};

struct ImPlot3DPlot {
    ImGuiID ID;
    ImPlot3DFlags Flags;
    ImPlot3DFlags PreviousFlags;
    ImGuiTextBuffer Title;
    bool JustCreated;
    bool Initialized;

    ImRect FrameRect;
    ImRect CanvasRect;
    ImRect PlotRect;

    ImPlot3DQuat InitialRotation;
    ImPlot3DQuat Rotation;
    ImPlot3DCond RotationCond;
    ImPlot3DAxis Axes[3];

    float AnimationTime;
    ImPlot3DQuat RotationAnimationEnd;

    bool SetupLocked;
    bool Hovered;
    bool Held;
    int HeldEdgeIdx;
    int HeldPlaneIdx;
    ImPlot3DPoint DragRotationAxis;

    bool FitThisFrame;

    ImPlot3DItemGroup Items;

    ImDrawList3D DrawList;

    bool ContextClick;
    bool OpenContextThisFrame;

    ImPlot3DPlot() {
        PreviousFlags = Flags = ImPlot3DFlags_None;
        JustCreated = true;
        Initialized = false;
        InitialRotation = ImPlot3DQuat(-0.513269, -0.212596, -0.318184, 0.76819);
        Rotation = ImPlot3DQuat(0.0, 0.0, 0.0, 1.0);
        RotationCond = ImPlot3DCond_None;
        for (int i = 0; i < 3; i++)
            Axes[i] = ImPlot3DAxis();
        AnimationTime = 0.0f;
        RotationAnimationEnd = Rotation;
        SetupLocked = false;
        Hovered = Held = false;
        HeldEdgeIdx = -1;
        HeldPlaneIdx = -1;
        DragRotationAxis = ImPlot3DPoint(0.0, 0.0, 0.0);
        FitThisFrame = true;
        ContextClick = false;
        OpenContextThisFrame = false;
    }

    inline void SetTitle(const char* title) {
        Title.Buf.shrink(0);
        if (title && ImGui::FindRenderedTextEnd(title, nullptr) != title)
            Title.append(title, title + strlen(title) + 1);
    }
    inline bool HasTitle() const { return !Title.empty() && !ImPlot3D::ImHasFlag(Flags, ImPlot3DFlags_NoTitle); }
    inline const char* GetTitle() const { return Title.Buf.Data; }
    inline bool IsRotationLocked() const { return RotationCond == ImPlot3DCond_Always; }

    void ExtendFit(const ImPlot3DPoint& point);

    ImPlot3DPoint RangeMin() const;

    ImPlot3DPoint RangeMax() const;

    ImPlot3DPoint RangeCenter() const;

    void SetRange(const ImPlot3DPoint& min, const ImPlot3DPoint& max);

    float GetViewScale() const;

    ImPlot3DPoint GetBoxScale() const;

    void ApplyEqualAspect(ImAxis3D ref_axis);
};

struct ImPlot3DContext {
    ImPool<ImPlot3DPlot> Plots;
    ImPlot3DPlot* CurrentPlot;
    ImPlot3DItemGroup* CurrentItems;
    ImPlot3DItem* CurrentItem;
    ImPlot3DNextItemData NextItemData;
    ImPlot3DStyle Style;
    ImVector<ImGuiColorMod> ColorModifiers;
    ImVector<ImGuiStyleMod> StyleModifiers;
    ImVector<ImPlot3DColormap> ColormapModifiers;
    ImPlot3DColormapData ColormapData;
};

namespace ImPlot3D {

#ifndef GImPlot3D
extern IMPLOT3D_API ImPlot3DContext* GImPlot3D;
#endif

IMPLOT3D_API void InitializeContext(ImPlot3DContext* ctx);
IMPLOT3D_API void ResetContext(ImPlot3DContext* ctx);

IMPLOT3D_API bool IsColorAuto(const ImVec4& col);
IMPLOT3D_API bool IsColorAuto(ImPlot3DCol idx);
IMPLOT3D_API ImVec4 GetAutoColor(ImPlot3DCol idx);
IMPLOT3D_API const char* GetStyleColorName(ImPlot3DCol idx);

static inline ImU32 CalcTextColor(const ImVec4& bg) {
    return (bg.x * 0.299f + bg.y * 0.587f + bg.z * 0.114f) > 0.5f ? IM_COL32_BLACK : IM_COL32_WHITE;
}
static inline ImU32 CalcTextColor(ImU32 bg) { return CalcTextColor(ImGui::ColorConvertU32ToFloat4(bg)); }

IMPLOT3D_API const ImPlot3DNextItemData& GetItemData();

IMPLOT3D_API ImU32 GetColormapColorU32(int idx, ImPlot3DColormap cmap);

IMPLOT3D_API ImU32 NextColormapColorU32();

IMPLOT3D_API void RenderColorBar(const ImU32* colors, int size, ImDrawList& DrawList, const ImRect& bounds, bool vert, bool reversed,
                                 bool continuous);

IMPLOT3D_API bool BeginItem(const char* label_id, const ImPlot3DSpec& spec = ImPlot3DSpec(), const ImVec4& item_col = IMPLOT3D_AUTO_COL,
                            ImPlot3DMarker item_mkr = ImPlot3DMarker_Invalid);
IMPLOT3D_API void EndItem();

IMPLOT3D_API ImPlot3DItem* RegisterOrGetItem(const char* label_id, ImPlot3DItemFlags flags, bool* just_created = nullptr);

IMPLOT3D_API ImPlot3DItem* GetCurrentItem();

IMPLOT3D_API void BustItemCache();

IMPLOT3D_API void AddTextRotated(ImDrawList* draw_list, ImVec2 pos, float angle, ImU32 col, const char* text_begin, const char* text_end = nullptr);

IMPLOT3D_API ImPlot3DPlot* GetCurrentPlot();

IMPLOT3D_API void BustPlotCache();

IMPLOT3D_API ImVec2 GetFramePos();
IMPLOT3D_API ImVec2 GetFrameSize();

IMPLOT3D_API ImPlot3DPoint PlotToNDC(const ImPlot3DPoint& point);
IMPLOT3D_API ImPlot3DPoint NDCToPlot(const ImPlot3DPoint& point);

IMPLOT3D_API ImVec2 NDCToPixels(const ImPlot3DPoint& point);

IMPLOT3D_API ImPlot3DRay PixelsToNDCRay(const ImVec2& pix);

IMPLOT3D_API ImPlot3DRay NDCRayToPlotRay(const ImPlot3DRay& ray);

IMPLOT3D_API void SetupLock();

static inline double TransformForward_Log10(double v, void*) { return ImLog10(v <= 0.0 ? DBL_MIN : v); }

static inline double TransformInverse_Log10(double v, void*) { return ImPow(10, v); }

static inline double TransformForward_SymLog(double v, void*) { return 2.0 * ImAsinh(v / 2.0); }

static inline double TransformInverse_SymLog(double v, void*) { return 2.0 * ImSinh(v / 2.0); }

int Formatter_Default(double value, char* buff, int size, void* data);

void Locator_Default(ImPlot3DTicker& ticker, const ImPlot3DRange& range, float pixels, ImPlot3DFormatter formatter, void* formatter_data);
void Locator_Log10(ImPlot3DTicker& ticker, const ImPlot3DRange& range, float pixels, ImPlot3DFormatter formatter, void* formatter_data);
void Locator_SymLog(ImPlot3DTicker& ticker, const ImPlot3DRange& range, float pixels, ImPlot3DFormatter formatter, void* formatter_data);

}

#endif
