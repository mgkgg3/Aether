#pragma once
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// Refreshed from the 2026-10-21 dump (version-9ac8595712b447a7).
//
// Every entry below that the dump still emits was rewritten from it in place;
// the values that moved are the client's own (most member offsets came down by
// 8 and most RVAs moved with the image).  The dump is narrower than this header,
// so 569 names it does not emit - Luau/LuaVm internals, reflection tables, the
// D3D11/sky hooks, the mesh-cache walk, some part and GUI fields - kept their
// previous values.  They compile, but nothing here proves them against this
// client: check one before trusting a feature that reads it.
// ---------------------------------------------------------------------------

namespace Offsets {
    inline std::string ClientVersion = "version-9ac8595712b447a7";

    namespace AirProperties {
         inline constexpr uintptr_t AirDensity = 0x18;
         inline constexpr uintptr_t GlobalWind = 0x3c;
    }

    namespace AnimationTrack {
         inline constexpr uintptr_t Animation = 0xA0;
         inline constexpr uintptr_t Animator = 0xF8;
         inline constexpr uintptr_t IsPlaying = 0xA10;
         inline constexpr uintptr_t Looped = 0xCD;
         inline constexpr uintptr_t Speed = 0xBC;
         inline constexpr uintptr_t TimePosition = 0xC0;
    }

    namespace Animator {
    inline constexpr uintptr_t ActiveAnimations = 0x900;
    }

    namespace Atmosphere {
    inline constexpr uintptr_t Color = 0xA0;
         inline constexpr uintptr_t Decay = 0xAC;
    inline constexpr uintptr_t Density = 0xB8;
    inline constexpr uintptr_t Glare = 0xBC;
    inline constexpr uintptr_t Haze = 0xC0;
    inline constexpr uintptr_t Offset = 0xC4;
    }

    namespace Attachment {
    inline constexpr uintptr_t Position = 0xAC;
    }

    namespace BasePart {
    inline constexpr uintptr_t CastShadow = 0x120;
    inline constexpr uintptr_t Color3 = 0x183;
    inline constexpr uintptr_t Locked = 0x121;
    inline constexpr uintptr_t Massless = 0x122;
    inline constexpr uintptr_t Primitive = 0x178;
    inline constexpr uintptr_t Reflectance = 0xF4;
         inline constexpr uintptr_t Shape = 0x1A0;
    inline constexpr uintptr_t Transparency = 0x118;
    inline constexpr uintptr_t ClusterNode = 0x180;
    }

    namespace Beam {
    inline constexpr uintptr_t Attachment0 = 0x148;
    inline constexpr uintptr_t Attachment1 = 0x158;
    inline constexpr uintptr_t Brightness = 0x168;
    inline constexpr uintptr_t CurveSize0 = 0x16C;
    inline constexpr uintptr_t CurveSize1 = 0x170;
    inline constexpr uintptr_t LightEmission = 0x174;
    inline constexpr uintptr_t LightInfluence = 0x178;
    inline constexpr uintptr_t Texture = 0x128;
    inline constexpr uintptr_t TextureLength = 0x184;
    inline constexpr uintptr_t TextureSpeed = 0x18C;
    inline constexpr uintptr_t Width0 = 0x190;
    inline constexpr uintptr_t Width1 = 0x194;
    inline constexpr uintptr_t ZOffset = 0x198;
    }

    namespace BloomEffect {
    inline constexpr uintptr_t Enabled = 0x98;
    inline constexpr uintptr_t Intensity = 0xA0;
    inline constexpr uintptr_t Size = 0xA4;
    inline constexpr uintptr_t Threshold = 0xA8;
    }

    namespace BlurEffect {
    inline constexpr uintptr_t Enabled = 0x98;
    inline constexpr uintptr_t Size = 0xA0;
    }

    namespace BoundFuncDescriptor {
         inline constexpr uintptr_t Function = 0x90;
         inline constexpr uintptr_t Name = 0x8;
    }

    namespace Btools {
         inline constexpr uintptr_t MouseCommandWorkspace = 0x50;
         inline constexpr uintptr_t ToolAllocationSize = 0x100;
         inline constexpr uintptr_t WorkspaceCurrentCommand = 0x878;
         inline constexpr uintptr_t WorkspaceStickyCommand = 0x880;
    }

    namespace ByteCode {
         inline constexpr uintptr_t Pointer = 0x10;
         inline constexpr uintptr_t Size = 0x28;
         inline constexpr uintptr_t PointerLegacy = 0x10;
    }

    namespace Bytecode {
         inline constexpr uintptr_t BytecodeVerificationTableA = 0xd22f50;
         inline constexpr uintptr_t LuaVMLoad = 0x41a3610;
    }

    namespace CachedItem {
         inline constexpr uintptr_t FileMeshData = 0x40;
    }

    namespace Camera {
    inline constexpr uintptr_t CFrame = 0xC0;
    inline constexpr uintptr_t CameraSubject = 0xB0;
    inline constexpr uintptr_t CameraType = 0x120;
    inline constexpr uintptr_t FieldOfView = 0x128;
    inline constexpr uintptr_t ImagePlaneDepth = 0x2CC;
    inline constexpr uintptr_t Position = 0xE4;
    inline constexpr uintptr_t Rotation = 0xC0;
         // From the 2026-10-21 dump (same ClientVersion): the viewport pair sits
         // 0x40 apart, at 0x284 and 0x2c4.  The old values (0x28c/0x2cc) were one
         // ImagePlaneDepth read apart from the truth, so a viewport write landed
         // on the float plane depth and the mouse silent aim never moved.
         inline constexpr uintptr_t Viewport = 0x284;
         inline constexpr uintptr_t ViewportSize = 0x2c4;
         inline constexpr uintptr_t ViewportInt16 = 0x284;
    }

    namespace CharacterMesh {
    inline constexpr uintptr_t BaseTextureId = 0xB0;
    inline constexpr uintptr_t BodyPart = 0x130;
    inline constexpr uintptr_t MeshId = 0xE0;
    inline constexpr uintptr_t OverlayTextureId = 0x110;
    }

    namespace ClassDescriptor {
         inline constexpr uintptr_t ClassName = 0x8;
         inline constexpr uintptr_t EventDescriptors = 0x88;
         inline constexpr uintptr_t FunctionDescriptors = 0xd0;
         inline constexpr uintptr_t PropertyDescriptors = 0x40;
         inline constexpr uintptr_t Creator = 0x230;
    }

    namespace ClickDetector {
    inline constexpr uintptr_t MaxActivationDistance = 0xD0;
    inline constexpr uintptr_t MouseIcon = 0xB0;
    inline constexpr uintptr_t CursorIcon = 0xB0;
    }

    namespace ClientBuild {
         inline constexpr uintptr_t EntityDrawRva = 0x399f220;
         inline constexpr uintptr_t EntitySubmitRva = 0x399f770;
    inline constexpr uintptr_t ImageSize = 0x94B6000;
         inline constexpr uintptr_t MergeFlag = 0x888c668;
    inline constexpr uintptr_t Timestamp = 0x741C8904;
    }

    namespace Clothing {
         inline constexpr uintptr_t Color3 = 0x108;
    inline constexpr uintptr_t Template = 0xE8;
    }

    namespace ColorCorrectionEffect {
    inline constexpr uintptr_t Brightness = 0xAC;
    inline constexpr uintptr_t Contrast = 0xB0;
    inline constexpr uintptr_t Enabled = 0x98;
         inline constexpr uintptr_t TintColor = 0xA0;
    }

    namespace ColorGradingEffect {
    inline constexpr uintptr_t Enabled = 0x98;
    inline constexpr uintptr_t TonemapperPreset = 0xA0;
    }

    namespace Coroutine {
    inline constexpr uintptr_t create = 0x59EA830;
    inline constexpr uintptr_t finally = 0x59EC700;
    inline constexpr uintptr_t isyieldable = 0x59EB0E0;
    inline constexpr uintptr_t running = 0x59EB070;
    inline constexpr uintptr_t status = 0x59E8DF0;
    inline constexpr uintptr_t wrap = 0x59EADA0;
    inline constexpr uintptr_t yield = 0x59EB010;
    }

    namespace DataModel {
         // Same dump, same build.  The previous block was shifted one field up:
         // 0x180 read the place id and called it the game id, which is why the
         // startup log printed "place 0" while the rescan line showed a real id.
         inline constexpr uintptr_t CreatorId = 0x170;
         inline constexpr uintptr_t GameId = 0x178;
    inline constexpr uintptr_t GameLoaded = 0x5C8;
    inline constexpr uintptr_t JobId = 0x108;
         inline constexpr uintptr_t PlaceId = 0x180;
         inline constexpr uintptr_t PlaceVersion = 0x19c;
         inline constexpr uintptr_t PrimitiveCount = 0x410;
         inline constexpr uintptr_t ScriptContext = 0x440;
    inline constexpr uintptr_t ServerIP = 0x5B0;
    inline constexpr uintptr_t ToRenderView1 = 0x1B8;
         inline constexpr uintptr_t ToRenderView2 = 0x8;
         inline constexpr uintptr_t ToRenderView3 = 0x28;
    inline constexpr uintptr_t Workspace = 0x148;
         inline constexpr uintptr_t VTableRva = 0x6F8EDB0;
    }

    namespace DepthOfFieldEffect {
    inline constexpr uintptr_t Enabled = 0x98;
    inline constexpr uintptr_t FarIntensity = 0xA0;
    inline constexpr uintptr_t FocusDistance = 0xA4;
    inline constexpr uintptr_t InFocusRadius = 0xA8;
    inline constexpr uintptr_t NearIntensity = 0xAC;
    }

    namespace Descriptor {
    inline constexpr uintptr_t Name = 0x0;
    }

    namespace DeviceD3D11Gfx {
         inline constexpr uintptr_t Context = 0x178;
         inline constexpr uintptr_t ContextObj = 0x1b0;
         inline constexpr uintptr_t ContextObjAlt = 0x268;
         inline constexpr uintptr_t ContextObjDrawRva = 0x2812ee0;
         inline constexpr uintptr_t ContextObjDrawSlot = 0x1c;
         inline constexpr uintptr_t ContextObjLegacy = 0x1f8;
         inline constexpr uintptr_t ContextObjLinkScan = 0x570;
         inline constexpr uintptr_t ContextObjToContext = 0x18;
         inline constexpr uintptr_t ContextObjVTableLen = 0x27;
    inline constexpr uintptr_t ContextObjVTableRva = 0x6FFA798;
         inline constexpr uintptr_t ContextPtr = 0x178;
         inline constexpr uintptr_t DevicePtr = 0x178;
         inline constexpr uintptr_t DevicePtrLegacy = 0x228;
         inline constexpr uintptr_t SwapChainPtr = 0x80;
    inline constexpr uintptr_t VTableRva = 0x6FFA390;
    }

    namespace DragDetector {
    inline constexpr uintptr_t ActivatedCursorIcon = 0x1A8;
    inline constexpr uintptr_t CursorIcon = 0xB0;
    inline constexpr uintptr_t MaxActivationDistance = 0xD0;
    inline constexpr uintptr_t MaxDragAngle = 0x290;
    inline constexpr uintptr_t MaxDragTranslation = 0x254;
    inline constexpr uintptr_t MaxForce = 0x294;
    inline constexpr uintptr_t MaxTorque = 0x298;
    inline constexpr uintptr_t MinDragAngle = 0x29C;
    inline constexpr uintptr_t MinDragTranslation = 0x260;
    inline constexpr uintptr_t ReferenceInstance = 0x1D8;
    inline constexpr uintptr_t Responsiveness = 0x2A8;
    }

    namespace EngineWallCheck {
         inline constexpr uintptr_t BoundFnOffset = 0x90;
         inline constexpr uintptr_t KernelVtableRva = 0x70b22d0;
    inline constexpr uintptr_t RaycastDescriptorRva = 0x86E0D50;
         inline constexpr uintptr_t RaycastWorkerRva = 0x8f6754b;
    inline constexpr uintptr_t RealWrapperRva = 0xD96A60;
         inline constexpr uintptr_t StepWorldRva = 0x72a9d0;
    }

    namespace FFlags {
    inline constexpr uintptr_t EnableLoadModule = 0x8A21BF0;
         inline constexpr uintptr_t GetFFlag = 0x49f8d70;
    inline constexpr uintptr_t NextGenReplicatorEnabledWrite4 = 0x86D0BD8;
    inline constexpr uintptr_t PhysicsSenderMaxBandwidthBps = 0x819B498;
    inline constexpr uintptr_t PhysicsSenderMaxBandwidthBpsScaling = 0x819B4C0;
    inline constexpr uintptr_t RenderFastClusterOcclusionCulling4 = 0x8A18A08;
         inline constexpr uintptr_t SetFFlag = 0x49f8270;
    }

    namespace FakeDataModel {
    inline constexpr uintptr_t Pointer = 0x8F59810;
         inline constexpr uintptr_t RealDataModel = 0x1f8;
    }

    namespace FastCluster {
         inline constexpr uintptr_t ClusterNode = 0x188;
    inline constexpr uintptr_t MeshEntityVTableRva = 0x70674B0;
    inline constexpr uintptr_t VTableRva = 0x7066130;
         inline constexpr uintptr_t VTableRvaAlt = 0x6f2ada0;
    inline constexpr uintptr_t VTableRvaSub = 0x7066268;
    inline constexpr uintptr_t VTableRvaSubAlt = 0x70663F8;
         inline constexpr uintptr_t BindingSubobject = 0x90;
         inline constexpr uintptr_t DecalMaterialPtr = 0x48;
         inline constexpr uintptr_t EntityAlphaByte = 0x14;
         inline constexpr uintptr_t EntityBBoxMax = 0xA4;
         inline constexpr uintptr_t EntityBBoxMin = 0x98;
         inline constexpr uintptr_t EntityBegin = 0x48;
         inline constexpr uintptr_t EntityEnd = 0x50;
         inline constexpr uintptr_t EntityVTableRva = 0x6F2A0F0;
         inline constexpr uintptr_t MaterialColor = 0x24;
         inline constexpr uintptr_t MaterialCullMode = 0x10;
         inline constexpr uintptr_t MaterialFillMode = 0x11;
         inline constexpr uintptr_t MaterialFlags = 0x20;
         inline constexpr uintptr_t MaterialMatFlags = 0x18;
         inline constexpr uintptr_t MaterialParam = 0x1C;
         inline constexpr uintptr_t MaterialPtr = 0x20;
         inline constexpr uintptr_t MaterialStride = 0x88;
         inline constexpr uintptr_t PartRecordStride = 0x18;
         inline constexpr uintptr_t PartRecords = 0x138;
         inline constexpr uintptr_t PartRecordsEnd = 0x140;
         inline constexpr uintptr_t RenderQueueId = 0x10;
         inline constexpr uintptr_t TechniqueArrayPtr = 0x70;
         inline constexpr uintptr_t TechniqueEnd = 0x8;
         inline constexpr uintptr_t TechniqueStride = 0x88;
         inline constexpr uintptr_t TransformsView = 0x198;
    }

    namespace FastClusterBinding {
         inline constexpr uintptr_t Owner = 0x60;
         inline constexpr uintptr_t Subobject = 0x90;
    inline constexpr uintptr_t VTableRva = 0x7067390;
         inline constexpr uintptr_t Part = 0x10;
         inline constexpr uintptr_t Primitive = 0x8;
    }

    namespace FastClusterEntity {
         inline constexpr uintptr_t AlphaByte = 0x14;
         inline constexpr uintptr_t BBoxMaxX = 0xa4;
         inline constexpr uintptr_t BBoxMaxY = 0xa8;
         inline constexpr uintptr_t BBoxMaxZ = 0xac;
         inline constexpr uintptr_t BBoxMinX = 0x98;
         inline constexpr uintptr_t BBoxMinY = 0x9c;
         inline constexpr uintptr_t BBoxMinZ = 0xa0;
         inline constexpr uintptr_t CollectedPasses = 0x18;
         inline constexpr uintptr_t ContextPtr = 0x8;
         inline constexpr uintptr_t DecalMaterialPtr = 0x48;
         inline constexpr uintptr_t Highlight = 0xb4;
         inline constexpr uintptr_t MaterialPtr = 0x20;
         inline constexpr uintptr_t PrimitiveIndexArrayPtr = 0x80;
         inline constexpr uintptr_t RenderQueueId = 0x10;
         inline constexpr uintptr_t TechniqueArrayPtr = 0x70;
    inline constexpr uintptr_t VTableRva = 0x7067468;
         namespace Context {
              inline constexpr uintptr_t PrimitivePoolPtr = 0x198;
         }
         namespace PrimitivePool {
              inline constexpr uintptr_t ArrayBase = 0x20;
         }
         namespace PrimitiveRecord {
    inline constexpr uintptr_t Row0 = 0x0;
    inline constexpr uintptr_t Row1 = 0xC;
    inline constexpr uintptr_t Row2 = 0x18;
    inline constexpr uintptr_t Stride = 0x30;
    inline constexpr uintptr_t Translation = 0x24;
         }
         inline constexpr uintptr_t Cluster = 0x8;
         inline constexpr uintptr_t DrawMethodSlot = 0x2;
         inline constexpr uintptr_t MeshVTableRva = 0x6D70D30;
         inline constexpr uintptr_t NextEntity = 0x28;
         inline constexpr uintptr_t PrimitivePtr = 0x8;
         inline constexpr uintptr_t Stride = 0x28;
         inline constexpr uintptr_t VertexBuffer = 0x50;
    }

    namespace FileMeshData {
         inline constexpr uintptr_t AABBMax = 0x18c;
         inline constexpr uintptr_t AABBMin = 0x180;
         inline constexpr uintptr_t Faces = 0x30;
         inline constexpr uintptr_t FacesEnd = 0x38;
         inline constexpr uintptr_t VerticesEnd = 0x8;
         inline constexpr uintptr_t AabbMax = 0x18C;
         inline constexpr uintptr_t AabbMin = 0x180;
         inline constexpr uintptr_t Vertices = 0x0;
    }

    namespace Fire {
         inline constexpr uintptr_t FireProximityPrompt = 0x30d1720;
         inline constexpr uintptr_t Enabled = 0xD0;
         inline constexpr uintptr_t Heat = 0xC8;
         inline constexpr uintptr_t Size = 0xCC;
    }

    namespace Frame {
    inline constexpr uintptr_t FramePositionOffsetX = 0x4FC;
    inline constexpr uintptr_t FramePositionOffsetY = 0x504;
    }

    namespace FunctionDescriptor {
         inline constexpr uintptr_t Function = 0x80;
    }

    namespace Functions {
         inline constexpr uintptr_t CastArgs = 0x7bff10;
    inline constexpr uintptr_t Clone = 0x1545AB0;
    inline constexpr uintptr_t Destroy = 0x1545AD0;
         inline constexpr uintptr_t FireServer = 0x39ae800;
    inline constexpr uintptr_t Print = 0x43078C0;
    inline constexpr uintptr_t Shapecast = 0xD98800;
         inline constexpr uintptr_t WaitForChild = 0x3b349ed;
         inline constexpr uintptr_t FindPartOnRay = 0xE0FDE0;
         inline constexpr uintptr_t FindPartOnRayWithIgnoreList = 0xE0FE60;
         inline constexpr uintptr_t FindPartOnRayWithWhitelist = 0xE0FEF0;
         inline constexpr uintptr_t RaisePropertyChanged = 0xEC1B36;
         inline constexpr uintptr_t Raycast = 0xE074B0;
         inline constexpr uintptr_t SetParent = 0xDD7B83;
         inline constexpr uintptr_t SetParentInternal = 0x1D76F10;
    inline constexpr uintptr_t GlobalCollectGarbage = 0x4305C30;
    inline constexpr uintptr_t GlobalDelay = 0x43064C0;
    inline constexpr uintptr_t GlobalElapsedTime = 0x4306CB0;
    inline constexpr uintptr_t GlobalGetFenv = 0x430A350;
    inline constexpr uintptr_t GlobalLoadstring = 0x4306D30;
    inline constexpr uintptr_t GlobalPluginManager = 0x43073C0;
    inline constexpr uintptr_t GlobalPrint = 0x43078C0;
    inline constexpr uintptr_t GlobalRequire = 0x43079D0;
    inline constexpr uintptr_t GlobalSetFenv = 0x430A480;
    inline constexpr uintptr_t GlobalSettings = 0x4307EE0;
    inline constexpr uintptr_t GlobalSpawn = 0x43083E0;
    inline constexpr uintptr_t GlobalStats = 0x4308BE0;
    inline constexpr uintptr_t GlobalTick = 0x4309140;
    inline constexpr uintptr_t GlobalTime = 0x43091C0;
    inline constexpr uintptr_t GlobalUserSettings = 0x43093C0;
    inline constexpr uintptr_t GlobalVersion = 0x4309850;
    inline constexpr uintptr_t GlobalWait = 0x4309930;
    inline constexpr uintptr_t GlobalWarn = 0x4309B30;
    inline constexpr uintptr_t PrintIdentity = 0x43078E0;
         inline constexpr uintptr_t SetFFlag = 0x498CE30;
    }

    namespace GeometryD3D11 {
    inline constexpr uintptr_t VTableRva = 0x6FFAA80;
    }

    namespace GuiBase2D {
    inline constexpr uintptr_t AbsolutePosition = 0xF4;
    inline constexpr uintptr_t AbsoluteRotation = 0xD0;
    inline constexpr uintptr_t AbsoluteSize = 0x69;
    }

    namespace GuiObject {
    inline constexpr uintptr_t BackgroundColor3 = 0x528;
    inline constexpr uintptr_t BackgroundTransparency = 0x534;
    inline constexpr uintptr_t BorderColor3 = 0x534;
    inline constexpr uintptr_t Image = 0x9D8;
         inline constexpr uintptr_t LayoutOrder = 0x564;
    inline constexpr uintptr_t Position = 0x4F8;
    inline constexpr uintptr_t RichText = 0xBD0;
    inline constexpr uintptr_t Rotation = 0xD0;
    // This dump: the enabled/visible flags are not next to the layout fields
    // in this build.  Visible is what the silent aim's gun check reads, so with
    // the old 0x54c it was reading an unrelated byte and could refuse to fire.
    inline constexpr uintptr_t ScreenGui_Enabled = 0x4AC;
    inline constexpr uintptr_t Size = 0x518;
    inline constexpr uintptr_t Text = 0xE38;
    inline constexpr uintptr_t TextColor3 = 0xEE8;
    inline constexpr uintptr_t Visible = 0x595;
         inline constexpr uintptr_t ZIndex = 0x58C;
         inline constexpr uintptr_t Active = 0x598;
         inline constexpr uintptr_t AnchorPoint = 0x548;
         inline constexpr uintptr_t AutomaticSize = 0x550;
         inline constexpr uintptr_t BorderMode = 0x558;
         inline constexpr uintptr_t BorderSizePixel = 0x55C;
         inline constexpr uintptr_t ClipsDescendants = 0x599;
         inline constexpr uintptr_t GuiState = 0x568;
         inline constexpr uintptr_t Interactable = 0x59B;
         inline constexpr uintptr_t Selectable = 0x59C;
         inline constexpr uintptr_t SelectionOrder = 0x588;
         inline constexpr uintptr_t SizeConstraint = 0x590;
    }

    namespace Highlight {
         inline constexpr uintptr_t Adornee = 0xa8;
         inline constexpr uintptr_t DepthMode = 0xd0;
         inline constexpr uintptr_t Enabled = 0xd8;
         inline constexpr uintptr_t FillColor = 0xb8;
         inline constexpr uintptr_t FillTransparency = 0xd4;
         inline constexpr uintptr_t OutlineColor = 0xc4;
         inline constexpr uintptr_t OutlineTransparency = 0xdc;
         inline constexpr uintptr_t VTableRva = 0x6F505E8;
    }

    namespace HighlightManager {
         inline constexpr uintptr_t ContextActive = 0x420;
         inline constexpr uintptr_t ContextDepth = 0x20;
         inline constexpr uintptr_t ContextFill = 0x520;
         inline constexpr uintptr_t ContextOutline = 0x1520;
         inline constexpr uintptr_t ContextStride = 0x2520;
         inline constexpr uintptr_t Contexts = 0x100;
         inline constexpr uintptr_t FallbackActive = 0x538;
         inline constexpr uintptr_t FallbackDepth = 0x138;
         inline constexpr uintptr_t FallbackFill = 0x638;
         inline constexpr uintptr_t FallbackOutline = 0x1638;
         inline constexpr uintptr_t GlobalActive = 0x2aac;
         inline constexpr uintptr_t GlobalDepth = 0x26ac;
         inline constexpr uintptr_t GlobalFill = 0x2bac;
         inline constexpr uintptr_t GlobalNext = 0x26a8;
         inline constexpr uintptr_t GlobalOutline = 0x3bac;
         inline constexpr uintptr_t Manager = 0xc10;
         inline constexpr uintptr_t MergeFlag = 0x888c668;
    }

    namespace HopperBin {
         inline constexpr std::uint32_t BinType = 1112u;
         inline constexpr uintptr_t VTableRva = 0x6F4E260;
    }

    namespace Humanoid {
    inline constexpr uintptr_t AutoJumpEnabled = 0x1C8;
    inline constexpr uintptr_t AutoRotate = 0x1C9;
    inline constexpr uintptr_t AutomaticScalingEnabled = 0x1CA;
    inline constexpr uintptr_t BreakJointsOnDeath = 0x1CB;
         inline constexpr uintptr_t CameraOffset = 0x110;
         inline constexpr uintptr_t DisplayDistanceType = 0x174;
    inline constexpr uintptr_t DisplayName = 0xA0;
    inline constexpr uintptr_t EvaluateStateMachine = 0x1CC;
    inline constexpr uintptr_t FloorMaterial = 0x178;
    inline constexpr uintptr_t Health = 0x184;
    inline constexpr uintptr_t HealthDisplayDistance = 0x17C;
         inline constexpr uintptr_t HealthDisplayType = 0x180;
    // 0x188 is a float in the dump; the old 0x75 was a byte offset, so a
    // hip-height write of four bytes was landing on the rig type's neighbours.
    inline constexpr uintptr_t HipHeight = 0x188;
    inline constexpr uintptr_t HumanoidRootPart = 0x460;
         inline constexpr uintptr_t HumanoidState = 0x8A8;
         inline constexpr uintptr_t HumanoidStateID = 0x20;
         inline constexpr uintptr_t IsWalking = 0xA27;
    inline constexpr uintptr_t Jump = 0x1CF;
    inline constexpr uintptr_t JumpHeight = 0x194;
    inline constexpr uintptr_t JumpPower = 0x198;
    inline constexpr uintptr_t MaxHealth = 0x19C;
    inline constexpr uintptr_t MaxSlopeAngle = 0x1A0;
    inline constexpr uintptr_t MoveDirection = 0x134;
    inline constexpr uintptr_t MoveToPoint = 0x158;
    inline constexpr uintptr_t NameDisplayDistance = 0x1A4;
    inline constexpr uintptr_t NameOcclusion = 0x1A8;
    inline constexpr uintptr_t PlatformStand = 0x1D1;
    inline constexpr uintptr_t RequiresNeck = 0x1D2;
    inline constexpr uintptr_t RigType = 0x1B4;
    inline constexpr uintptr_t SeatPart = 0xF0;
    inline constexpr uintptr_t Sit = 0x1D2;
    inline constexpr uintptr_t TargetPoint = 0x140;
    inline constexpr uintptr_t UseJumpPower = 0x1D5;
    inline constexpr uintptr_t Walkspeed = 0x1C4;
    inline constexpr uintptr_t WalkspeedCheck = 0x39C;
    inline constexpr uintptr_t MoveToPart = 0x100;
         inline constexpr uintptr_t PlatformStatePointer = 0x5AB900CC;
    inline constexpr uintptr_t WalkSpeed = 0x1C4;
         inline constexpr uintptr_t WalkSpeedCheck = 0x3A4;
         inline constexpr uintptr_t WalkTimer = 0x3F0;
         inline constexpr uintptr_t WalkToPoint = 0x1C;
         inline constexpr uintptr_t VTableRva = 0x6E89E20;
    }

    namespace Instance {
         inline constexpr uintptr_t ChildrenEnd = 0x8;
    inline constexpr uintptr_t ChildrenStart = 0x70;
         inline constexpr uintptr_t ClassBase = 0x1b0;
         inline constexpr uintptr_t ClassDescriptor = 0x18;
    inline constexpr uintptr_t FindFirstAncestorWhichIsA = 0x1545DF0;
    inline constexpr uintptr_t FindFirstChild = 0x1545B50;
    inline constexpr uintptr_t FindFirstChildOfClass = 0x1D521E0;
    inline constexpr uintptr_t GetAttribute = 0x1D5BBD0;
    inline constexpr uintptr_t GetChildren = 0xF2DD30;
         inline constexpr uintptr_t Index = 0x412b7d0;
    inline constexpr uintptr_t Name = 0x0;
    inline constexpr uintptr_t NameContainer = 0x68;
         inline constexpr uintptr_t New = 0x42c96a0;
    inline constexpr uintptr_t Parent = 0x60;
         inline constexpr uintptr_t This = 0x8;
         inline constexpr uintptr_t ChildrenStride = 0x10;
         inline constexpr uintptr_t ClassName = 0x8;
         inline constexpr uintptr_t AttributeContainer = 0x38;
         inline constexpr uintptr_t AttributeToNext = 0x58;
         inline constexpr uintptr_t AttributeToValue = 0x18;
         inline constexpr uintptr_t ComponentMap = 0x38;
         inline constexpr uintptr_t Creator_isCreatable = 0x10;
         inline constexpr uintptr_t PushInstance = 0x417A040;
         inline constexpr uintptr_t VTableRva = 0x6EA66F8;
    }

    namespace LRUHolder {
         inline constexpr uintptr_t MemEnforcedLRUCache = 0x20;
    }

    namespace LRUNode {
         inline constexpr uintptr_t AssetID = 0x10;
         inline constexpr uintptr_t CachedItem = 0x40;
         inline constexpr uintptr_t Next = 0x0;
    }

    namespace Lighting {
    inline constexpr uintptr_t Ambient = 0xB8;
    inline constexpr uintptr_t Brightness = 0x100;
    inline constexpr uintptr_t ClockTime = 0xB0;
    inline constexpr uintptr_t ColorShift_Bottom = 0xD0;
    inline constexpr uintptr_t ColorShift_Top = 0xC4;
    inline constexpr uintptr_t EnvironmentDiffuseScale = 0x104;
    inline constexpr uintptr_t EnvironmentSpecularScale = 0x108;
    inline constexpr uintptr_t ExposureCompensation = 0x10C;
         inline constexpr uintptr_t FogColor = 0xDC;
    inline constexpr uintptr_t FogEnd = 0x114;
    inline constexpr uintptr_t FogStart = 0x118;
    inline constexpr uintptr_t GeographicLatitude = 0x11C;
    inline constexpr uintptr_t GlobalShadows = 0x12C;
    inline constexpr uintptr_t GradientBottom = 0x178;
    inline constexpr uintptr_t GradientTop = 0x138;
    inline constexpr uintptr_t LightColor = 0x144;
         inline constexpr uintptr_t LightDirection = 0x150;
    inline constexpr uintptr_t MoonPosition = 0x16C;
    inline constexpr uintptr_t OutdoorAmbient = 0xE8;
    inline constexpr uintptr_t Sky = 0x1B0;
    inline constexpr uintptr_t Source = 0x15C;
    inline constexpr uintptr_t SunPosition = 0x160;
         inline constexpr uintptr_t Atmosphere = 0x1C8;
         inline constexpr uintptr_t ShadowSoftness = 0xE8;
         inline constexpr uintptr_t VTableRva = 0x6EF80B0;
    }

    namespace LocalScript {
         inline constexpr uintptr_t ByteCode = 0x180;
    inline constexpr uintptr_t GUID = 0xB8;
    inline constexpr uintptr_t Hash = 0x188;
    }

    namespace Luau {
         inline constexpr uintptr_t Capability = 0x26585a0;
         inline constexpr uintptr_t GetGlobalState = 0x41acdb0;
         inline constexpr uintptr_t LuaVMLoad = 0x9f6d00;
         inline constexpr uintptr_t dumpgco = 0x265b350;
         inline constexpr uintptr_t ktable = 0x411b7d0;
    inline constexpr uintptr_t luaB_assert = 0x26DBC90;
    inline constexpr uintptr_t luaB_error = 0x26D7430;
    inline constexpr uintptr_t luaB_gcinfo = 0x26DA5E0;
    inline constexpr uintptr_t luaB_getfenv = 0x26D89E0;
    inline constexpr uintptr_t luaB_getmetatable = 0x26D7E60;
    inline constexpr uintptr_t luaB_newproxy = 0x26DC880;
    inline constexpr uintptr_t luaB_next = 0x26DB1E0;
    inline constexpr uintptr_t luaB_print = 0x26D6FD0;
    inline constexpr uintptr_t luaB_rawequal = 0x26D8CC0;
    inline constexpr uintptr_t luaB_rawget = 0x26D8DD0;
    inline constexpr uintptr_t luaB_rawlen = 0x26DA540;
    inline constexpr uintptr_t luaB_rawset = 0x26D9860;
    inline constexpr uintptr_t luaB_select = 0x26DBD40;
    inline constexpr uintptr_t luaB_setfenv = 0x26D8AA0;
    inline constexpr uintptr_t luaB_setmetatable = 0x26D7FE0;
    inline constexpr uintptr_t luaB_tonumber = 0x26D70A0;
    inline constexpr uintptr_t luaB_tostring = 0x26DC820;
    inline constexpr uintptr_t luaB_type = 0x26DA600;
    inline constexpr uintptr_t luaB_typeof = 0x26DAC00;
         inline constexpr uintptr_t luaC_init = 0x265de50;
         inline constexpr uintptr_t luaC_step = 0x2644300;
         inline constexpr uintptr_t luaD_Throw = 0x262ddc0;
         inline constexpr uintptr_t luaF_FreeProto = 0xd195a0;
         inline constexpr uintptr_t luaG_runerror = 0x7bf930;
         inline constexpr uintptr_t luaH_clone = 0x2694180;
         inline constexpr uintptr_t luaH_dummynode = 0x2619a00;
         inline constexpr uintptr_t luaL_argerror = 0x2623300;
         inline constexpr uintptr_t luaL_checktype = 0x2622820;
         inline constexpr uintptr_t luaM_newgco = 0x265dce0;
         inline constexpr uintptr_t luaM_visitgco = 0x66992cb;
         inline constexpr uintptr_t luaO_nilobject = 0x26199c8;
         inline constexpr uintptr_t luaO_pushfstring = 0x2657710;
         inline constexpr uintptr_t lua_checkstack = 0xcb8d00;
         inline constexpr uintptr_t lua_error = 0x261d640;
         inline constexpr uintptr_t lua_newstate = 0x26210c0;
         inline constexpr uintptr_t lua_pushfstring = 0x261ad70;
         inline constexpr uintptr_t luau_execute = 0x266c81c;
         inline constexpr uintptr_t print = 0x1ce58f0;
         inline constexpr uintptr_t pseudo2addr = 0x2619930;
    inline constexpr uintptr_t tclone = 0x26ECB50;
    }

    namespace MaterialColors {
         inline constexpr uintptr_t Asphalt = 0x30;
         inline constexpr uintptr_t Basalt = 0x27;
         inline constexpr uintptr_t Brick = 0xf;
         inline constexpr uintptr_t Cobblestone = 0x33;
         inline constexpr uintptr_t Concrete = 0xc;
         inline constexpr uintptr_t CrackedLava = 0x2d;
         inline constexpr uintptr_t Glacier = 0x1b;
         inline constexpr uintptr_t Grass = 0x6;
         inline constexpr uintptr_t Ground = 0x2a;
         inline constexpr uintptr_t Ice = 0x36;
         inline constexpr uintptr_t LeafyGrass = 0x39;
         inline constexpr uintptr_t Limestone = 0x3f;
         inline constexpr uintptr_t Mud = 0x24;
         inline constexpr uintptr_t Pavement = 0x42;
         inline constexpr uintptr_t Rock = 0x18;
         inline constexpr uintptr_t Salt = 0x3c;
         inline constexpr uintptr_t Sand = 0x12;
         inline constexpr uintptr_t Sandstone = 0x21;
         inline constexpr uintptr_t Slate = 0x9;
         inline constexpr uintptr_t Snow = 0x1e;
         inline constexpr uintptr_t WoodPlanks = 0x15;
    }

    namespace MaterialLayer {
    // 0x26, not 0x24: `../offsets/offsets (1).h` (version-11f155f3450f4c8e,
    // the dump the rest of this table is versioned against) puts the colour
    // word six bytes into the layer, and engine chams read and write it.  At
    // 0x24 the write lands on the padding in front of it and the part keeps
    // its own colour, which looks exactly like "engine chams does nothing".
    inline constexpr uintptr_t ColorData               = 0x26;
         inline constexpr uintptr_t CullMode = 0x10;
         inline constexpr uintptr_t FillModeByte = 0x11;
         inline constexpr uintptr_t Flags2 = 0x20;
         inline constexpr uintptr_t MatFlags = 0x18;
         inline constexpr uintptr_t Param = 0x1c;
    inline constexpr uintptr_t Stride = 0x88;
         // The technique block's shader program, as a shared pointer: the object
         // here and its control block eight bytes on.  This is the field the
         // chams swap writes - see features/chams/EngineChams.cpp, and the POC
         // it comes from (Roblox-Chams-Material-POC, "technique block +0x28").
         inline constexpr uintptr_t ShaderProgram = 0x28;
    }

    namespace MemEnforcedLRUCache {
         inline constexpr uintptr_t Head = 0x8;
    }

    namespace MeshContentProvider {
         inline constexpr uintptr_t LRUHolder = 0xC0;
         inline constexpr uintptr_t LruHolder = 0xC8;
         inline constexpr uintptr_t VTableRva = 0x6E56260;
    }

    namespace MeshPart {
    inline constexpr uintptr_t MeshId = 0x2F8;
    inline constexpr uintptr_t Texture = 0x328;
    inline constexpr uintptr_t TextureId = 0x328;
    inline constexpr uintptr_t MeshSize = 0x210;
    }

    namespace Misc {
    inline constexpr uintptr_t Adornee = 0xD8;
    inline constexpr uintptr_t AnimationId = 0xA8;
    inline constexpr uintptr_t PrintIdentity = 0x43078E0;
         inline constexpr uintptr_t RequireBypass = 0xa81;
         inline constexpr uintptr_t StringLength = 0x10;
    inline constexpr uintptr_t Value = 0xA0;
         inline constexpr uintptr_t BooleanType = 0x6E09978;
         inline constexpr uintptr_t EnableLoadModule = 0x869DBC8;
         inline constexpr uintptr_t FLogHookPtr = 0x8A948E0;
         inline constexpr uintptr_t LockViolationInstanceCrash = 0x8749848;
         inline constexpr uintptr_t LockViolationScriptCrash = 0x869D9D8;
         inline constexpr uintptr_t TLS_IDENTITYSTRUCT = 0x835AD38;
         inline constexpr uintptr_t vftFuncScriptSlot0 = 0x6DD78A0;
         inline constexpr uintptr_t vftFuncScriptSlot1 = 0x6DD7A00;
         inline constexpr uintptr_t vftMessageScriptSlot = 0x6DD7810;
         inline constexpr uintptr_t vftNetworkStream = 0x6CEB2D8;
         inline constexpr uintptr_t vftWaitScriptSlot = 0x6DD7660;
    }

    namespace Model {
    inline constexpr uintptr_t PrimaryPart = 0x240;
    inline constexpr uintptr_t Scale = 0x12C;
    }

    namespace ModuleScript {
         inline constexpr uintptr_t ByteCode = 0x128;
    inline constexpr uintptr_t GUID = 0xB8;
    inline constexpr uintptr_t Hash = 0x348;
         inline constexpr uintptr_t HashLookUp = 0x72657a0;
         inline constexpr uintptr_t IsCoreScript = 0x158;
         inline constexpr uintptr_t IsRobloxScript = 0x9a;
         inline constexpr uintptr_t VTableRva = 0x6F96518;
    }

    namespace Motor6D {
         inline constexpr uintptr_t ChildName = 0x118;
         inline constexpr uintptr_t ParentName = 0x108;
    }

    namespace Mouse {
         inline constexpr uintptr_t FireProximityPrompt = 0x30d1720;
         inline constexpr uintptr_t HitGetImpl = 0x98;
         inline constexpr uintptr_t HitGetter = 0x8;
         inline constexpr uintptr_t MouseProp = 0xA0;
         inline constexpr uintptr_t PropertyCount = 0x48;
         inline constexpr uintptr_t PropertyList = 0x40;
         inline constexpr uintptr_t PropertyName = 0x8;
    }

    namespace MouseService {
    inline constexpr uintptr_t InputObject = 0xE8;
    inline constexpr uintptr_t InputObject2 = 0xF8;
    inline constexpr uintptr_t MousePosition = 0xBC;
         inline constexpr uintptr_t VTableRva = 0x6F36B20;
    }

    namespace ParticleEmitter {
    inline constexpr uintptr_t Acceleration = 0x1C8;
    inline constexpr uintptr_t Brightness = 0x204;
    inline constexpr uintptr_t Drag = 0x208;
    inline constexpr uintptr_t Lifetime = 0x1DC;
    inline constexpr uintptr_t LightEmission = 0x220;
    inline constexpr uintptr_t LightInfluence = 0x224;
    inline constexpr uintptr_t Rate = 0x230;
    inline constexpr uintptr_t RotSpeed = 0x1E4;
    inline constexpr uintptr_t Rotation = 0x1EC;
    inline constexpr uintptr_t Speed = 0x1F4;
    inline constexpr uintptr_t SpreadAngle = 0x1FC;
    inline constexpr uintptr_t Texture = 0x1A8;
    inline constexpr uintptr_t TimeScale = 0x244;
    inline constexpr uintptr_t VelocityInheritance = 0x248;
    inline constexpr uintptr_t ZOffset = 0x24C;
         inline constexpr uintptr_t Enabled = 0x258;
         inline constexpr uintptr_t Size = 0xB8;
         inline constexpr uintptr_t Squash = 0xF8;
         inline constexpr uintptr_t Transparency = 0x138;
    }

    namespace Player {
         inline constexpr uintptr_t AccountAge = 0x354;
    inline constexpr uintptr_t CameraMode = 0x368;
         // 0x120 is DisplayName in the dump, which is exactly what the old
         // 0x120 "LocalPlayer" was reading: a string pointer, never a Player.
         inline constexpr uintptr_t DisplayName = 0x120;
    inline constexpr uintptr_t HealthDisplayDistance = 0x38C;
         // LocalPlayer is a field of the Players service, not of a Player.  Kept
         // here because a dozen call sites read it through this namespace; the
         // static_assert at the bottom of the file pins it to Players::LocalPlayer
         // so the two can never disagree again.
         inline constexpr uintptr_t LocalPlayer = 0x118;
    inline constexpr uintptr_t LocaleId = 0x100;
    inline constexpr uintptr_t MaxZoomDistance = 0x360;
    inline constexpr uintptr_t MinZoomDistance = 0x364;
    inline constexpr uintptr_t ModelInstance = 0x290;
    inline constexpr uintptr_t NameDisplayDistance = 0x39C;
    inline constexpr uintptr_t Team = 0x2D0;
         inline constexpr uintptr_t TeamColor = 0x3A8;
    inline constexpr uintptr_t UserId = 0xB8;
         inline constexpr uintptr_t Character = 0x298;
    inline constexpr uintptr_t Mouse = 0x1210;
         inline constexpr uintptr_t VTableRva = 0x6F5B9C0;
    }

    namespace Primitive {
         inline constexpr uintptr_t AssemblyAngularVelocity = 0xec;
         inline constexpr uintptr_t AssemblyLinearVelocity = 0xe0;
    inline constexpr uintptr_t Flags = 0x309;
         inline constexpr uintptr_t Material = 0x24e;
         inline constexpr uintptr_t Owner = 0x218;
         inline constexpr uintptr_t Position = 0xd4;
         inline constexpr uintptr_t Rotation = 0xb0;
         inline constexpr uintptr_t Size = 0x1c4;
         inline constexpr uintptr_t Validate = 0x6;
         inline constexpr uintptr_t CFrame = 0xB0;
         inline constexpr uintptr_t Geometry = 0x210;
         inline constexpr uintptr_t Orientation = 0xB0;
         inline constexpr uintptr_t Part = 0x218;
    inline constexpr uintptr_t PrimitiveFlags = 0x309;
    }

    namespace PrimitiveFlags {
         inline constexpr uintptr_t Anchored = 0x2;
         inline constexpr uintptr_t CanCollide = 0x8;
         inline constexpr uintptr_t CanQuery = 0x20;
         inline constexpr uintptr_t CanTouch = 0x10;
    }

    namespace Property {
         inline constexpr uintptr_t GetPropertyData = 0x7c47e0;
    }

    namespace PropertyDescriptor {
         inline constexpr uintptr_t GetSetImpl = 0x98;
         inline constexpr uintptr_t TType = 0x70;
    }

    namespace ProximityPrompt {
    inline constexpr uintptr_t ActionText = 0x98;
    inline constexpr uintptr_t Enabled = 0x11E;
         inline constexpr uintptr_t GamepadKeyCode = 0x104;
    inline constexpr uintptr_t HoldDuration = 0x108;
         inline constexpr uintptr_t KeyCode = 0x10C;
    inline constexpr uintptr_t MaxActivationDistance = 0x110;
    inline constexpr uintptr_t ObjectText = 0xB8;
    inline constexpr uintptr_t RequiresLineOfSight = 0x11F;
         inline constexpr uintptr_t KeyboardKeyCode = 0x114;
    }

    namespace RakNet {
         inline constexpr uintptr_t RakNetReliabilityLayerSend = 0xba0160;
         inline constexpr uintptr_t RakNetSend = 0x2852d00;
         inline constexpr uintptr_t RakPeerProcessNetworkPacket = 0x28532c0;
         inline constexpr uintptr_t RakPeerReceive = 0x848900;
    inline constexpr uintptr_t RakPeerVirtualTable = 0x6FFB0C8;
         inline constexpr uintptr_t RaknetReportNetworkError = 0x47b8110;
         inline constexpr uintptr_t SendPacketsToSelf = 0x2852f50;
         inline constexpr uintptr_t UpdateNetworkLoop = 0x495fae0;
         inline constexpr uintptr_t convertItemEnumToString = 0x4819f10;
    }

    namespace RbxTransport {
         inline constexpr uintptr_t AppDestructor = 0x2155f90;
    inline constexpr uintptr_t AppRTTI = 0x8448790;
         inline constexpr uintptr_t AppReceive = 0x2150b30;
         inline constexpr uintptr_t AppReceiveMaxBytes = 0x2150c30;
         inline constexpr uintptr_t AppSend = 0x214fe00;
    inline constexpr uintptr_t AppVTable = 0x6FEDBC8;
         inline constexpr uintptr_t ClientChannelConfig = 0x3a87510;
         inline constexpr uintptr_t ClientDestructor = 0x3a86c50;
    inline constexpr uintptr_t ClientRTTI = 0x84B5418;
         inline constexpr uintptr_t ClientReceive = 0x3a87330;
         inline constexpr uintptr_t ClientSend = 0x3a86cc0;
    inline constexpr uintptr_t ClientVTable = 0x7067F18;
         inline constexpr uintptr_t FailureFallbackToRbxTransport = 0x47b75b5;
         inline constexpr uintptr_t StreamClose = 0x214fe80;
         inline constexpr uintptr_t UseRbxTransportClient = 0x3a6cc65;
         inline constexpr uintptr_t AppRtti = 0x745E7E0;
         inline constexpr uintptr_t ClientRtti = 0x74FA7C8;
    }

    namespace RemoteEvent {
    inline constexpr uintptr_t BaseVTableRva = 0x7057398;
    inline constexpr uintptr_t DescBaseVTableRva = 0x7057130;
         inline constexpr uintptr_t FireClient = 0x3449d10;
         inline constexpr uintptr_t FireServer = 0x3449b70;
    inline constexpr uintptr_t FireServerRva = 0x74E390;
         inline constexpr uintptr_t FireServerVtableSlot = 0x4b;
         inline constexpr uintptr_t HookSlot7 = 0x7;
    inline constexpr uintptr_t HookSlot7RemoteEventFn = 0x35B5FF0;
         inline constexpr uintptr_t InstanceParentOffset = 0x68;
    inline constexpr uintptr_t InstanceVTableRva = 0x6FE27D8;
    inline constexpr uintptr_t RemoteFuncVTableRva = 0x705CAF8;
    inline constexpr uintptr_t VTableRva = 0x705CD60;
         inline constexpr uintptr_t VtableSlotSignalFire = 0x2;
    }

    namespace RenderJob {
         inline constexpr uintptr_t FakeDataModel = 0x38;
         inline constexpr uintptr_t RealDataModel = 0x1f0;
         inline constexpr uintptr_t RenderView = 0x1e0;
    }

    namespace RenderQueue {
         inline constexpr std::uint32_t AlwaysOnTop = 13u;
         inline constexpr std::uint32_t AlwaysOnTopAdorns = 14u;
         inline constexpr std::uint32_t Decals = 2u;
         inline constexpr std::uint32_t Glass = 8u;
         inline constexpr std::uint32_t GlassTint = 7u;
         inline constexpr std::uint32_t OnTopReadOnlyDepth = 12u;
         inline constexpr std::uint32_t OnTopWithDepth = 11u;
         inline constexpr std::uint32_t Opaque = 0u;
         inline constexpr std::uint32_t OpaqueAdorns = 4u;
         inline constexpr std::uint32_t OpaqueCasters = 3u;
         inline constexpr std::uint32_t OpaqueWithAlpha = 5u;
         inline constexpr std::uint32_t Screen = 15u;
         inline constexpr std::uint32_t ScreenOnTopOfBlur = 16u;
         inline constexpr std::uint32_t Terrain = 1u;
         inline constexpr std::uint32_t Transparent = 9u;
         inline constexpr std::uint32_t TransparentCasters = 10u;
         inline constexpr std::uint32_t Water = 6u;
    }

    namespace RenderView {
         inline constexpr uintptr_t DeviceD3D11 = 0x8;
         inline constexpr uintptr_t LightingValid = 0x278;
         inline constexpr uintptr_t SkyValid = 0x2dc;
         inline constexpr uintptr_t VisualEngine = 0x18;
         inline constexpr uintptr_t VTableRva = 0x6F25BE0;
    }

    namespace RunService {
         inline constexpr uintptr_t HeartbeatFPS = 0xC0;
    inline constexpr uintptr_t HeartbeatTask = 0x8A08;
    }

    namespace Script {
         inline constexpr uintptr_t ByteCode = 0x180;
    inline constexpr uintptr_t GUID = 0xB8;
    inline constexpr uintptr_t Hash = 0x188;
    }

    namespace ScriptContext {
    inline constexpr uintptr_t Resume = 0x43004B0;
         inline constexpr uintptr_t RequireBypass = 0xB8;
         inline constexpr uintptr_t LuaStateAlt = 0x650;
         // This build moved the game's lua_State out of the dump's slot: the
         // wrapper route (VmWrapper + 0x650) and 0x650/0x1B0 directly all read
         // as non-states, while sc + 0x750 holds one that LuaGc's
         // LooksLikeLuaState accepts (live: L=0x1b606e8e480, totalbytes 60404864,
         // gcstate 0).  LuaGc tries this slot first and the rest as fallbacks.
         inline constexpr uintptr_t LuaStateCurrent = 0x750;
         inline constexpr uintptr_t ToResume = 0x8F0;
         inline constexpr uintptr_t VTableRva = 0x6F93730;
         inline constexpr uintptr_t VmEncryptedLuaState = 0x1B0;
         inline constexpr uintptr_t VmWrapper = 0x1A0;
         inline constexpr uintptr_t VmWrapper2 = 0xDF0;
         inline constexpr uintptr_t VmWrapperBig = 0x1A0;
    }

    namespace Seat {
    inline constexpr uintptr_t Occupant = 0x200;
    }

    namespace ShaderManager {
    inline constexpr uintptr_t VTableRva = 0x7064978;
         // The shader program list the visual engine holds, keyed by name: the
         // route Roblox-Chams-Material-POC's README describes.  Its offsets are
         // for another client build, so they were checked rather than copied:
         // tools/shader_probe.py reads the live list and reports where it really
         // is, and what each key really says.
         //
         // Measured on 0.743 (7a0c6d10fcb34a2c): the manager is at +0xBA0, not
         // the README's +0xB98 - one field off, which is why the probe sweeps
         // instead of trusting it - and the list holds 321 programs, 320 of them
         // named.  The keys are not material names: each is
         // "<vs>VS<flags>,<fs>FS<flags>,", one vertex/pixel pair per render pass.
         inline constexpr uintptr_t Manager = 0xBA0;
         inline constexpr uintptr_t ListHead = 0x80;      // circular; next at +0x00
         inline constexpr uintptr_t NodeNext = 0x00;
         inline constexpr uintptr_t NodeKey = 0x10;       // std::string: buffer, size at +0x20
         inline constexpr uintptr_t NodeProgram = 0x30;   // shared_ptr: object, control at +0x38
    }

    namespace Sky {
         inline constexpr uintptr_t CubeTextures = 0x2ea7f60;
         inline constexpr uintptr_t DrawSkyEnv = 0x377be00;
    inline constexpr uintptr_t MoonAngularSize = 0x22C;
    inline constexpr uintptr_t MoonTextureId = 0xB0;
         inline constexpr uintptr_t SkyGroup = 0x8004a08;
    inline constexpr uintptr_t SkyboxBk = 0xE0;
    inline constexpr uintptr_t SkyboxDn = 0x110;
    inline constexpr uintptr_t SkyboxFt = 0x140;
    inline constexpr uintptr_t SkyboxLf = 0x170;
    inline constexpr uintptr_t SkyboxOrientation = 0x220;
    inline constexpr uintptr_t SkyboxRt = 0x1A0;
    inline constexpr uintptr_t SkyboxUp = 0x1D0;
         inline constexpr uintptr_t StarCount = 0x230;
    inline constexpr uintptr_t SunAngularSize = 0x224;
    inline constexpr uintptr_t SunTextureId = 0x200;
         inline constexpr uintptr_t CelestialBodiesShown = 0x240;
         inline constexpr uintptr_t DrawAdv = 0x3747BC0;
         inline constexpr uintptr_t DrawCubeCallB = 0x0;
         inline constexpr uintptr_t VTableRva = 0x6F4ECC8;
    }

    namespace Sound {
    inline constexpr uintptr_t IsPlaying = 0x128;
    inline constexpr uintptr_t Looped = 0x125;
    inline constexpr uintptr_t PlaybackSpeed = 0x104;
    inline constexpr uintptr_t RollOffMaxDistance = 0x108;
    inline constexpr uintptr_t RollOffMinDistance = 0x10C;
    inline constexpr uintptr_t SoundGroup = 0xD0;
    inline constexpr uintptr_t SoundId = 0xB0;
    inline constexpr uintptr_t Volume = 0x118;
    }

    namespace SpawnLocation {
    inline constexpr uintptr_t AllowTeamChangeOnTouch = 0x1D8;
    inline constexpr uintptr_t Enabled = 0x1D9;
         inline constexpr uintptr_t ForcefieldDuration = 0x1D0;
    inline constexpr uintptr_t Neutral = 0x1DA;
         inline constexpr uintptr_t TeamColor = 0x1D4;
         inline constexpr uintptr_t Duration = 0x1D8;
    }

    namespace SpecialMesh {
    inline constexpr uintptr_t MeshId = 0xE0;
    inline constexpr uintptr_t Scale = 0xAC;
         inline constexpr uintptr_t Offset = 0xA8;
         inline constexpr uintptr_t TextureId = 0x118;
    }

    namespace StatsItem {
    inline constexpr uintptr_t Value = 0x98;
    }

    namespace SunRaysEffect {
    inline constexpr uintptr_t Enabled = 0x98;
    inline constexpr uintptr_t Intensity = 0xA0;
    inline constexpr uintptr_t Spread = 0xA4;
    }

    namespace SurfaceAppearance {
         inline constexpr uintptr_t AlphaMode = 0x1D8;
         inline constexpr uintptr_t Color = 0x1C0;
    inline constexpr uintptr_t ColorMap = 0xB0;
    inline constexpr uintptr_t EmissiveMaskContent = 0xE0;
    inline constexpr uintptr_t EmissiveStrength = 0x1DC;
         inline constexpr uintptr_t EmissiveTint = 0x1CC;
    inline constexpr uintptr_t MetalnessMap = 0x110;
    inline constexpr uintptr_t NormalMap = 0x140;
    inline constexpr uintptr_t RoughnessMap = 0x170;
    }

    namespace Task {
         inline constexpr uintptr_t TaskDelay = 0x43246d0;
         inline constexpr uintptr_t TaskDesynchronize = 0x4323210;
         inline constexpr uintptr_t TaskSynchronize = 0x4322da0;
         inline constexpr uintptr_t TaskWait = 0x43249d0;
         inline constexpr uintptr_t TaskCancel = 0x43234B0;
         inline constexpr uintptr_t TaskDefer = 0x4322730;
         inline constexpr uintptr_t TaskSpawn = 0x4322C10;
    }

    namespace TaskScheduler {
         inline constexpr uintptr_t JobEnd = 0xd0;
         inline constexpr uintptr_t JobName = 0x18;
         inline constexpr uintptr_t JobStart = 0xc8;
         inline constexpr uintptr_t MaxFPS = 0xb0;
    inline constexpr uintptr_t Pointer = 0x8F026A8;
    inline constexpr uintptr_t StepFunction = 0x2A740B0;
         inline constexpr uintptr_t MaxFps = 0xB0;
    }

    namespace Team {
         inline constexpr uintptr_t BrickColor = 0xA0;
         inline constexpr uintptr_t TeamColor = 0xA8;
    }

    namespace TechniqueArray {
         inline constexpr uintptr_t BeginOffset = 0x0;
         inline constexpr uintptr_t EndOffset = 0x8;
    inline constexpr uintptr_t EntryStride = 0x88;
    }

    namespace Terrain {
    inline constexpr uintptr_t GrassLength = 0x1D8;
    inline constexpr uintptr_t MaterialColors = 0x4A0;
    inline constexpr uintptr_t WaterColor = 0x1C8;
    inline constexpr uintptr_t WaterReflectance = 0x1E0;
    inline constexpr uintptr_t WaterTransparency = 0x1E4;
    inline constexpr uintptr_t WaterWaveSize = 0x1E8;
    inline constexpr uintptr_t WaterWaveSpeed = 0x1EC;
    }

    namespace TextureManager2 {
         inline constexpr uintptr_t VTableRva = 0x6f28da0;
    }

    namespace Textures {
    inline constexpr uintptr_t Decal_Texture = 0x1C8;
    inline constexpr uintptr_t Texture_Texture = 0x1C8;
    }

    namespace Tool {
    inline constexpr uintptr_t CanBeDropped = 0x4A0;
    inline constexpr uintptr_t Enabled = 0x4A1;
    inline constexpr uintptr_t Grip = 0x494;
    inline constexpr uintptr_t ManualActivationOnly = 0x4A2;
    inline constexpr uintptr_t RequiresHandle = 0x4A3;
    inline constexpr uintptr_t TextureId = 0x348;
    inline constexpr uintptr_t Tooltip = 0x450;
         inline constexpr uintptr_t GripForward = 0x490;
         inline constexpr uintptr_t GripPos = 0x49C;
         inline constexpr uintptr_t GripRight = 0x478;
         inline constexpr uintptr_t GripUp = 0x484;
    }

    namespace Types {
         inline constexpr uintptr_t AllTypes = 0x8d5c788;
    }

    namespace UnionOperation {
         inline constexpr uintptr_t AssetId = 0x2F8;
    }

    namespace UserInputService {
         inline constexpr uintptr_t WindowInputState = 0x2A8;
    }

    namespace Value {
    inline constexpr uintptr_t Value = 0xA0;
    }

    namespace VehicleSeat {
    inline constexpr uintptr_t MaxSpeed = 0x210;
    inline constexpr uintptr_t SteerFloat = 0x214;
    inline constexpr uintptr_t ThrottleFloat = 0x218;
    inline constexpr uintptr_t Torque = 0x21C;
    inline constexpr uintptr_t TurnSpeed = 0x220;
         inline constexpr uintptr_t Occupant = 0x1F8;
    }

    namespace ViewBase {
    inline constexpr uintptr_t VTableRva = 0x7063668;
    }

    namespace VisualEngine {
         inline constexpr uintptr_t Dimensions = 0xb10;
         inline constexpr uintptr_t FakeDataModel = 0xaf0;
    inline constexpr uintptr_t Pointer = 0x89DC1A0;
    inline constexpr uintptr_t RenderView = 0xC40;
         inline constexpr uintptr_t ViewMatrix = 0x1b0;
         inline constexpr uintptr_t VTableRva = 0x6F26338;
    }

    namespace Weld {
         inline constexpr uintptr_t Part0 = 0x100;
         inline constexpr uintptr_t Part1 = 0x110;
    }

    namespace WeldConstraint {
         inline constexpr uintptr_t Part0 = 0xA0;
         inline constexpr uintptr_t Part1 = 0xB0;
    }

    namespace WindowInputState {
         inline constexpr uintptr_t CapsLock = 0x40;
         inline constexpr uintptr_t CurrentTextBox = 0x48;
    }

    namespace Workspace {
    inline constexpr uintptr_t CurrentCamera = 0x4A0;
    inline constexpr uintptr_t DistributedGameTime = 0x4C0;
         inline constexpr uintptr_t ReadOnlyGravity = 0x9c8;
    inline constexpr uintptr_t World = 0x3F8;
    }

    namespace World {
         inline constexpr uintptr_t AirProperties = 0x238;
         inline constexpr uintptr_t FallenPartsDestroyHeight = 0x214;
         inline constexpr uintptr_t Gravity = 0x220;
    inline constexpr uintptr_t Primitives = 0x2A8;
         inline constexpr uintptr_t worldStepsPerSec = 0x860;
         inline constexpr uintptr_t WorldSteps = 0x888;
    }

    namespace WorldRoot {
    inline constexpr uintptr_t FindPartOnRayBoundFnRva = 0xD9F8B0;
    inline constexpr uintptr_t FindPartOnRayDescriptorRva = 0x86E1790;
    inline constexpr uintptr_t FindPartOnRayWithIgnoreListBoundFnRva = 0xD9F930;
    inline constexpr uintptr_t FindPartOnRayWithIgnoreListDescriptorRva = 0x86E1850;
    inline constexpr uintptr_t FindPartOnRayWithWhitelistBoundFnRva = 0xD9F9C0;
    inline constexpr uintptr_t FindPartOnRayWithWhitelistDescriptorRva = 0x86E1910;
    inline constexpr uintptr_t RaycastBoundFnRva = 0xD96A60;
    inline constexpr uintptr_t RaycastDescriptorRva = 0x86E0D50;
    inline constexpr uintptr_t RaycastDescriptorVtableRva = 0x6F44F58;
         inline constexpr uintptr_t RaycastBoundDesc = RaycastDescriptorRva;
         inline constexpr uintptr_t RaycastBoundFn = 0x90;
         inline constexpr uintptr_t VTableRva = 0x6DDCE30;
    }

    namespace Raycast {
         inline constexpr uintptr_t RaycastBoundDesc = 0x86E0D50;
    inline constexpr uintptr_t RaycastBoundFn   = 0x90;
         inline constexpr uintptr_t RaycastBoundDescJatos   = 0x8364BB0;   // jatos dumper, 2026-10-14
         inline constexpr uintptr_t RaycastBoundDescFallback = 0x8197D90;
         inline constexpr uintptr_t RaycastBoundDescProven = 0x8089F20;
    }
}

namespace Offsets {

    // Raycast lives in the hand-written block at the bottom of this file: the
    // stub picks its slot from a list at install time, so the dumper's single
    // value is one candidate among several rather than the answer.  It is
    // deliberately empty here so the dump cannot reintroduce a duplicate name.
    static_assert(RenderView::LightingValid != 0,
        "RenderView::LightingValid came back 0 from a dump refresh - restore the proven value, do not write to offset 0");
    static_assert(RenderView::SkyValid != 0,
        "RenderView::SkyValid came back 0 from a dump refresh - restore the proven value");
    // The slot WorldRoot::RaycastBound's function pointer lives in: the client
    // keeps a descriptor at `desc` and the pointer 0x90 bytes into it.  That
    // address moves with every client build, and the dumps do not agree with
    // each other - two dumps taken the same afternoon name two different
    // descriptors - so it is not one constant here.  The stub tries each of
    // these in order and keeps the first whose slot holds a function inside the
    // client's own image, which is a check the wrong address cannot pass.

    namespace Creator {
         inline constexpr uintptr_t MapEnd = 0x83DEC00;
         inline constexpr uintptr_t MapStart = 0x83DEBF8;
         inline constexpr uintptr_t isCreatable = 0x10;
    }

    namespace DeviceContextD3D11 {
         inline constexpr uintptr_t DrawRecordSlot = 0x1B;
         inline constexpr uintptr_t DrawSlot = 0x1C;
    }

    namespace DeviceD3D11 {
         inline constexpr uintptr_t Context = 0x180;
         inline constexpr uintptr_t ContextObj = 0x1B0;
         inline constexpr uintptr_t ContextPtr = 0x180;
         inline constexpr uintptr_t DevicePtr = 0x178;
         inline constexpr uintptr_t SwapChainPtr = 0x80;
    inline constexpr uintptr_t VTableRva = 0x6FFA390;
    }

    namespace FFlag {
         inline constexpr uintptr_t RenderFastClusterOcclusionCulling4 = 0x8693D50;
    }

    namespace ICreator {
         inline constexpr uintptr_t Create = 0x0;
    }

    namespace InputObject {
         inline constexpr uintptr_t MousePosition = 0x58;
         inline constexpr uintptr_t VTableRva = 0x6F359F0;
    }

    namespace InstancedBinding {
         inline constexpr uintptr_t Entity = 0x70;
         inline constexpr uintptr_t Part = 0x10;
         inline constexpr uintptr_t SpecialMeshVTableRva = 0x6F27C68;
         inline constexpr uintptr_t VTableRva = 0x6F27B10;
    }

    namespace InstancedBinding2 {
         inline constexpr uintptr_t Cluster = 0x68;
    }

    namespace InstancedCluster2 {
         inline constexpr uintptr_t VTableRva = 0x6D5ACB0;
    }

    namespace InstancedEntity {
         inline constexpr uintptr_t Bindings = 0x130;
         inline constexpr uintptr_t BindingsEnd = 0x138;
         inline constexpr uintptr_t InstanceData = 0x148;
         inline constexpr uintptr_t InstanceDataEnd = 0x150;
         inline constexpr uintptr_t InstanceStride = 0x70;
         inline constexpr uintptr_t MeshResource = 0x98;
         inline constexpr uintptr_t VTableRva = 0x6F27AC8;
    }

    namespace IntValue {
         inline constexpr uintptr_t Value = 0xA8;
    }

    namespace LightingParameters {
         inline constexpr uintptr_t GeographicLatitude = 0x124;
         inline constexpr uintptr_t LightColor = 0x14C;
         inline constexpr uintptr_t LightDirection = 0x158;
         inline constexpr uintptr_t SkyAmbient = 0x140;
         inline constexpr uintptr_t SkyAmbient2 = 0x128;
         inline constexpr uintptr_t Source = 0x164;
         inline constexpr uintptr_t TrueMoonPosition = 0x174;
         inline constexpr uintptr_t TrueSunPosition = 0x168;
    }

    namespace LruHolder {
         inline constexpr uintptr_t MemEnforcedLRUCache = 0x20;
    }

    namespace LruNode {
         inline constexpr uintptr_t CachedItem = 0x40;
         inline constexpr uintptr_t MeshId = 0x10;
         inline constexpr uintptr_t Next = 0x0;
    }

    namespace MeshData {
         inline constexpr uintptr_t FaceEnd = 0x38;
         inline constexpr uintptr_t FaceStart = 0x30;
         inline constexpr uintptr_t VertexEnd = 0x8;
         inline constexpr uintptr_t VertexStart = 0x0;
    }

    namespace MeshResource {
         inline constexpr uintptr_t RangeStride = 0x28;
         inline constexpr uintptr_t Ranges = 0xD0;
    }

    namespace NumberValue {
         inline constexpr uintptr_t Value = 0xA8;
    }

    namespace PlayerConfigurer {
    inline constexpr uintptr_t Pointer = 0x8E48500;
    }

    namespace PlayerMouse {
    inline constexpr uintptr_t Icon = 0xB0;
    inline constexpr uintptr_t Workspace = 0x138;
    }

    namespace Players {
    inline constexpr uintptr_t LocalPlayer = 0x118;
    }

    namespace Reflection {
         inline constexpr uintptr_t CreatorTable = 0x83DEBF8;
         inline constexpr uintptr_t NameRegistry = 0x827B210;
         inline constexpr uintptr_t ClassDescCreatable = 0x10;
         inline constexpr uintptr_t ClassDescFlags = 0x1BC;
         inline constexpr uintptr_t EntryValue = 0x8;
         inline constexpr uintptr_t KTable = 0x827B210;
         inline constexpr uintptr_t NameTable = 0x50;
         inline constexpr uintptr_t TableEmpty = 0x20;
         inline constexpr uintptr_t TableEnd = 0x8;
         inline constexpr uintptr_t TableStart = 0x0;
         inline constexpr uintptr_t TableStride = 0x10;
    }

    namespace ReflectionType {
         inline constexpr uintptr_t AdReward = 0x60;
         inline constexpr uintptr_t AnimTrackMetadata = 0x66;
         inline constexpr uintptr_t AnimTrackPlayState = 0x65;
         inline constexpr uintptr_t AnimTrackWeight = 0x67;
         inline constexpr uintptr_t AnimationContext = 0x54;
         inline constexpr uintptr_t AnimationMask = 0x4D;
         inline constexpr uintptr_t AnimationMaskModifier = 0x5B;
         inline constexpr uintptr_t AnimationPose = 0x4E;
         inline constexpr uintptr_t Array = 0x24;
         inline constexpr uintptr_t ArticulatedJoint = 0x53;
         inline constexpr uintptr_t AssetContentMap = 0x61;
         inline constexpr uintptr_t Axes = 0x16;
         inline constexpr uintptr_t BinaryString = 0x1E;
         inline constexpr uintptr_t Bool = 0x1;
         inline constexpr uintptr_t BrickColor = 0x1C;
         inline constexpr uintptr_t Buffer = 0x56;
         inline constexpr uintptr_t CSGPropertyData = 0x48;
         inline constexpr uintptr_t CatalogSearchParams = 0x46;
         inline constexpr uintptr_t CellId = 0x19;
         inline constexpr uintptr_t ClipEvaluator = 0x4F;
         inline constexpr uintptr_t CollectionHandle = 0x20;
         inline constexpr uintptr_t Color3 = 0x11;
         inline constexpr uintptr_t Color3uint8 = 0x12;
         inline constexpr uintptr_t ColorSequence = 0x2A;
         inline constexpr uintptr_t ColorSequenceKeypoint = 0x2B;
         inline constexpr uintptr_t Connection = 0x30;
         inline constexpr uintptr_t Content = 0x5C;
         inline constexpr uintptr_t ContentId = 0x31;
         inline constexpr uintptr_t CoordinateFrame = 0x10;
         inline constexpr uintptr_t DateTime = 0x40;
         inline constexpr uintptr_t DebugTable = 0x45;
         inline constexpr uintptr_t DescribedBase = 0x32;
         inline constexpr uintptr_t Dictionary = 0x25;
         inline constexpr uintptr_t DockWidgetPluginGuiInfo = 0x38;
         inline constexpr uintptr_t Double = 0x5;
         inline constexpr uintptr_t Enum = 0x21;
         inline constexpr uintptr_t EventInstance = 0x36;
         inline constexpr uintptr_t Faces = 0x15;
         inline constexpr uintptr_t FacsReplicationData = 0x5A;
         inline constexpr uintptr_t Float = 0x4;
         inline constexpr uintptr_t FloatCurveKey = 0x3C;
         inline constexpr uintptr_t Font = 0x4A;
         inline constexpr uintptr_t Function = 0x29;
         inline constexpr uintptr_t GenericFunction = 0x28;
         inline constexpr uintptr_t GuidData = 0x1A;
         inline constexpr uintptr_t Instance = 0x8;
         inline constexpr uintptr_t InstanceRef = 0x51;
         inline constexpr uintptr_t Instances = 0x9;
         inline constexpr uintptr_t Int = 0x2;
         inline constexpr uintptr_t Int64 = 0x3;
         inline constexpr uintptr_t Integer = 0x57;
         inline constexpr uintptr_t LazyTable = 0x44;
         inline constexpr uintptr_t Map = 0x26;
         inline constexpr uintptr_t NetAssetHandle = 0x5D;
         inline constexpr uintptr_t NetAssetRef = 0x5E;
         inline constexpr uintptr_t Null = 0x0;
         inline constexpr uintptr_t NumberRange = 0x2C;
         inline constexpr uintptr_t NumberSequence = 0x2D;
         inline constexpr uintptr_t NumberSequenceKeypoint = 0x2E;
         inline constexpr uintptr_t Object = 0x5F;
         inline constexpr uintptr_t OpenCloudModel = 0x50;
         inline constexpr uintptr_t OptionalCoordinateFrame = 0x47;
         inline constexpr uintptr_t OverlapParams = 0x43;
         inline constexpr uintptr_t Path2DControlPoint = 0x58;
         inline constexpr uintptr_t PathWaypoint = 0x3B;
         inline constexpr uintptr_t PhysicalProperties = 0x1B;
         inline constexpr uintptr_t PluginDrag = 0x39;
         inline constexpr uintptr_t Property = 0x22;
         inline constexpr uintptr_t ProtectedString = 0x7;
         inline constexpr uintptr_t Random = 0x3A;
         inline constexpr uintptr_t Ray = 0xA;
         inline constexpr uintptr_t RaycastParams = 0x41;
         inline constexpr uintptr_t RaycastResult = 0x42;
         inline constexpr uintptr_t Rect2D = 0xF;
         inline constexpr uintptr_t RefType = 0x33;
         inline constexpr uintptr_t Region3 = 0x17;
         inline constexpr uintptr_t Region3int16 = 0x18;
         inline constexpr uintptr_t ReplicationPV = 0x59;
         inline constexpr uintptr_t RotationCurveKey = 0x3D;
         inline constexpr uintptr_t ScopedInstanceIdentity = 0x68;
         inline constexpr uintptr_t Secret = 0x55;
         inline constexpr uintptr_t SecurityCapabilities = 0x52;
         inline constexpr uintptr_t SharedString = 0x3F;
         inline constexpr uintptr_t SharedTable = 0x4B;
         inline constexpr uintptr_t SharedTableIterator = 0x4C;
         inline constexpr uintptr_t SlimReplicationData = 0x62;
         inline constexpr uintptr_t String = 0x6;
         inline constexpr uintptr_t Surface = 0x1F;
         inline constexpr uintptr_t SystemAddress = 0x1D;
         inline constexpr uintptr_t Tuple = 0x23;
         inline constexpr uintptr_t TweenInfo = 0x37;
         inline constexpr uintptr_t UDim = 0x13;
         inline constexpr uintptr_t UDim2 = 0x14;
         inline constexpr uintptr_t UniqueId = 0x49;
         inline constexpr uintptr_t User = 0x63;
         inline constexpr uintptr_t ValueCurveKey = 0x3E;
         inline constexpr uintptr_t Variant = 0x27;
         inline constexpr uintptr_t Vector2 = 0xB;
         inline constexpr uintptr_t Vector2int16 = 0xD;
         inline constexpr uintptr_t Vector3 = 0xC;
         inline constexpr uintptr_t Vector3int16 = 0xE;
         inline constexpr uintptr_t WebViewParams = 0x64;
    }

    namespace RobloxString {
         inline constexpr uintptr_t Size = 0x10;
         inline constexpr uintptr_t SsoCapacity = 0xF;
    }

    namespace Smoke {
         inline constexpr uintptr_t Enabled = 0xC8;
         inline constexpr uintptr_t Opacity = 0xB8;
         inline constexpr uintptr_t RiseVelocity = 0xBC;
         inline constexpr uintptr_t Size = 0xC0;
    }

    namespace SmoothClusterNode {
         inline constexpr uintptr_t VTableRva = 0x6F29E10;
    }

    namespace TextButton {
         inline constexpr uintptr_t AutoButtonColor = 0x9E4;
         inline constexpr uintptr_t ContentText = 0xE20;
         inline constexpr uintptr_t LineHeight = 0xF38;
         inline constexpr uintptr_t LocalizedText = 0xE20;
         inline constexpr uintptr_t MaxVisibleGraphemes = 0x1154;
         inline constexpr uintptr_t Modal = 0x9E5;
         inline constexpr uintptr_t RichText = 0x1036;
         inline constexpr uintptr_t Selected = 0x9E6;
         inline constexpr uintptr_t Text = 0xE20;
         inline constexpr uintptr_t TextColor3 = 0x1138;
         inline constexpr uintptr_t TextDirection = 0xFD8;
         inline constexpr uintptr_t TextScaled = 0xE09;
         inline constexpr uintptr_t TextSize = 0x115C;
         inline constexpr uintptr_t TextStrokeColor3 = 0x1144;
         inline constexpr uintptr_t TextStrokeTransparency = 0x1160;
         inline constexpr uintptr_t TextTransparency = 0x1164;
         inline constexpr uintptr_t TextTruncate = 0x1168;
         inline constexpr uintptr_t TextWrapped = 0x1030;
         inline constexpr uintptr_t TextXAlignment = 0x116C;
         inline constexpr uintptr_t TextYAlignment = 0xF80;
    }

    namespace TextLabel {
         inline constexpr uintptr_t ContentText = 0xBA0;
         inline constexpr uintptr_t LineHeight = 0xCB8;
         inline constexpr uintptr_t LocalizedText = 0xBA0;
         inline constexpr uintptr_t MaxVisibleGraphemes = 0xED4;
         inline constexpr uintptr_t RichText = 0xDB6;
         inline constexpr uintptr_t Text = 0xBA0;
         inline constexpr uintptr_t TextColor3 = 0xEB8;
         inline constexpr uintptr_t TextDirection = 0xD58;
         inline constexpr uintptr_t TextScaled = 0xDAE;
         inline constexpr uintptr_t TextSize = 0xEDC;
         inline constexpr uintptr_t TextStrokeColor3 = 0xEC4;
         inline constexpr uintptr_t TextStrokeTransparency = 0xEE0;
         inline constexpr uintptr_t TextTransparency = 0xEE4;
         inline constexpr uintptr_t TextTruncate = 0xEE8;
         inline constexpr uintptr_t TextWrapped = 0xDB0;
         inline constexpr uintptr_t TextXAlignment = 0xEEC;
         inline constexpr uintptr_t TextYAlignment = 0xD00;
         inline constexpr uintptr_t VTableRva = 0x6F71020;
    }

    namespace TransformsView {
         inline constexpr uintptr_t PartStride = 0x38;
         inline constexpr uintptr_t Parts = 0x38;
         inline constexpr uintptr_t PartsEnd = 0x40;
         inline constexpr uintptr_t VTableRva = 0x6F2A008;
    }

    namespace TriangleMesh {
         inline constexpr uintptr_t Data = 0x20;
    }

    namespace TriangleMeshData {
    }

    namespace btConvexHullShape {
         inline constexpr uintptr_t PointCount = 0x20;
         inline constexpr uintptr_t Points = 0x28;
         inline constexpr uintptr_t Size = 0x80;
    }

    namespace btGImpactCompoundShape {
         inline constexpr uintptr_t HullsBegin = 0x30;
         inline constexpr uintptr_t HullsEnd = 0x38;
    }

    namespace FastClusterContext {
         inline constexpr uintptr_t PrimitivePoolPtr = FastClusterEntity::Context::PrimitivePoolPtr;
    }

    namespace FastClusterRecord {
         inline constexpr uintptr_t PoolArrayBase = 0x20;
         inline constexpr uintptr_t Stride = 48;
         inline constexpr uintptr_t Translation = 36;
    }

    namespace Alloc {
         inline constexpr uintptr_t Free = 0x178F5E0;
         inline constexpr uintptr_t Malloc = 0x799A40;
    }

    namespace Attribute {
         inline constexpr uintptr_t Key = 0x0;
         inline constexpr uintptr_t KeyString = 0x8;
         inline constexpr uintptr_t Size = 0x58;
         inline constexpr uintptr_t Type = 0x8;
         inline constexpr uintptr_t TypeIdRva = 0x8D5A030;
         inline constexpr uintptr_t TypeIdRvaNew = 0x8AA12D0;
         inline constexpr uintptr_t Value = 0x18;
    }

    namespace AttributeContainer {
         inline constexpr uintptr_t AttributeList = 0x8;
    }

    namespace AttributesMap {
         inline constexpr uintptr_t Attributes = 0x8;
         inline constexpr uintptr_t Length = 0x10;
    }

    namespace Lua {
         inline constexpr uintptr_t BytecodeVerificationTableA = 0x83DE9B0;
         inline constexpr uintptr_t DummyNode = 0x6500E48;
         inline constexpr uintptr_t GetGlobalState = 0x41921C0;
         inline constexpr uintptr_t KTable = 0x827B210;
         inline constexpr uintptr_t LuaVM_Load = 0x4188AF0;
         inline constexpr uintptr_t NilObject = 0x6504708;
         inline constexpr uintptr_t OpCodeTable = 0x6F7FC40;
         inline constexpr uintptr_t TypeNames = 0x6526340;
         inline constexpr uintptr_t dummynode = 0x6500E48;
    }

    namespace LuaB {
    inline constexpr uintptr_t LuaBAssert = 0x26DBC90;
    inline constexpr uintptr_t LuaBError = 0x26D7430;
    inline constexpr uintptr_t LuaBGcinfo = 0x26DA5E0;
    inline constexpr uintptr_t LuaBGetfenv = 0x26D89E0;
    inline constexpr uintptr_t LuaBGetmetatable = 0x26D7E60;
    inline constexpr uintptr_t LuaBNewproxy = 0x26DC880;
    inline constexpr uintptr_t LuaBNext = 0x26DB1E0;
    inline constexpr uintptr_t LuaBPrint = 0x26D6FD0;
    inline constexpr uintptr_t LuaBRawequal = 0x26D8CC0;
    inline constexpr uintptr_t LuaBRawget = 0x26D8DD0;
    inline constexpr uintptr_t LuaBRawlen = 0x26DA540;
    inline constexpr uintptr_t LuaBRawset = 0x26D9860;
    inline constexpr uintptr_t LuaBSelect = 0x26DBD40;
    inline constexpr uintptr_t LuaBSetfenv = 0x26D8AA0;
    inline constexpr uintptr_t LuaBSetmetatable = 0x26D7FE0;
    inline constexpr uintptr_t LuaBTonumber = 0x26D70A0;
    inline constexpr uintptr_t LuaBTostring = 0x26DC820;
    inline constexpr uintptr_t LuaBType = 0x26DA600;
    inline constexpr uintptr_t LuaBTypeof = 0x26DAC00;
    }

    namespace LuaCoroutine {
    inline constexpr uintptr_t Close = 0x59EB170;
    inline constexpr uintptr_t Create = 0x59EA830;
         inline constexpr uintptr_t IsYieldable = 0x5903E80;
    inline constexpr uintptr_t Running = 0x59EB070;
    inline constexpr uintptr_t Status = 0x59E8DF0;
    inline constexpr uintptr_t Wrap = 0x59EADA0;
    inline constexpr uintptr_t Yield = 0x59EB010;
    }

    namespace LuauGlobal {
         inline constexpr uintptr_t dummynode = 0x6500E48;
         inline constexpr uintptr_t nilobject = 0x6504708;
         inline constexpr uintptr_t page_next_all = 0x8;
         inline constexpr uintptr_t page_next_free = 0x18;
    }


    // ---- names added by the 2026-10-21 dump -------------------------------
    // These did not exist in this file before the refresh, so they could not be
    // rewritten in place the way every other value was.  They are grouped by
    // namespace, which reopens it: the dump's own layout is not this file's.

    inline constexpr uintptr_t ClusterNode = 0x188;

    namespace WorldRoot {
         inline constexpr uintptr_t ShapecastDescriptorRva = 0x86E0F90;
    }

    namespace Coroutine {
         inline constexpr uintptr_t close = 0x59EB170;
    }

    namespace FFlags {
         inline constexpr uintptr_t CLI52064IxpTxParam = 0x819BD20;
         inline constexpr uintptr_t DebugRomarkSpeechAssetId = 0x81B4800;
         inline constexpr uintptr_t R20UniverseAccessTime = 0x831A3731;
         inline constexpr uintptr_t RealNamesInDisplayNamesExperimentDefinition = 0xB94E6C41;
         inline constexpr uintptr_t IXPPerformanceControlMLThresholdThreshold = 0x81B6CB0;
         inline constexpr uintptr_t NewFriendRequestsBadgeLayer = 0xB3456F53;
         inline constexpr uintptr_t DebugLocalTeamTestRccHost = 0x81B5D50;
         inline constexpr uintptr_t DebugLocalTeamCreateRccHost = 0x81B5D20;
         inline constexpr uintptr_t AppChatNewChatInputBarIxpLayer = 0xB3456F53;
         inline constexpr uintptr_t BlackbirdUrl = 0x84597777;
         inline constexpr uintptr_t DomainAllowListKnownBlacklistRegex = 0x81AAD88;
         inline constexpr uintptr_t DebugRomarkPlayerTurnoverPattern = 0x815AF68;
         inline constexpr uintptr_t RCCMonitoring = 0xB9547245;
         inline constexpr uintptr_t rbxGreen = 0x7A123734;
         inline constexpr uintptr_t CLI52064IxpRxParam = 0x819BD50;
         inline constexpr uintptr_t CaptureServiceStartUploadUniverseId = 0x818AFB8;
         inline constexpr uintptr_t PaginatedCarouselExperimentLayer = 0xB3456F53;
         inline constexpr uintptr_t MeshStreamingLODTranscodeDefaultFormat = 0x8170B18;
         inline constexpr uintptr_t StudioPluginRelay_NewDocking = 0xAC546556;
         inline constexpr uintptr_t AssistantMeshGenImageGenModelOverride = 0xAF447563;
         inline constexpr uintptr_t V1MenuLanguageSelectionFeatureForcedUserIds = 0x7E123133;
         inline constexpr uintptr_t NewAddFriendsTileIXPLayer = 0xB3456F53;
         inline constexpr uintptr_t RobloxTelemetryCheckSumSalt = 0x8161FB0;
         inline constexpr uintptr_t StudioPublishService = 0xAC546556;
         inline constexpr uintptr_t rbxBlue = 0x831A3434;
         inline constexpr uintptr_t OpenTypeFeaturesWhitelist = 0x812F3D0;
         inline constexpr uintptr_t StudioPluginOTANew = 0xAC546556;
         inline constexpr uintptr_t NewChatTabExperimentLayerValue = 0xC1516853;
         inline constexpr uintptr_t ThumbnailGeneratorMaxUploadSizeBytes = 0x7A123032;
         inline constexpr uintptr_t R20UniverseId = 0x80193031;
         inline constexpr uintptr_t SocialCaptureIXPLayer = 0xB3456F53;
         inline constexpr uintptr_t Feature_AccessCookiesWithUrlEnabledSticky = 0x85183635;
         inline constexpr uintptr_t StudioQtitanDocking_NewDocking = 0xB8546157;
         inline constexpr uintptr_t LightstepToken = 0x815DE58;
         inline constexpr uintptr_t AltTelegrafAddress = 0x7B103533;
         inline constexpr uintptr_t AssetAccessController = 0xAC546556;
         inline constexpr uintptr_t DMChangeTrackerBufferSize = 0x7E1A3632;
         inline constexpr uintptr_t BootcampCLI217016Log = 0xB8546157;
         inline constexpr uintptr_t DMRecorderBatchOutputPath = 0x8120200;
         inline constexpr uintptr_t BootcampCLI214500Log = 0xAC546556;
    }

    namespace LuaCoroutine {
         inline constexpr uintptr_t Isyieldable = 0x59EB0E0;
    }

    namespace Instance {
         inline constexpr uintptr_t FindFirstChildWhichIsA = 0x1545B00;
    }

    namespace FFlagList {
         inline constexpr uintptr_t Pointer = 0x8E1B0D0;
         inline constexpr uintptr_t ToFlag = 0x30;
         inline constexpr uintptr_t ToValue = 0x20;
    }
}

// The local player lives in the Players service, not in a Player: 0x120 in the
// Player object is DisplayName in this build, and reading it as a player name
// produced a string pointer that matched no cached entity - which left the cache
// without a local player, the aim without a distance to aim from, and the ESP
// drawing its own box on no one.  Both names are still in use across the tree, so
// they are pinned together here instead of being allowed to drift apart again.
// (At the end of the file, because Players is declared after Player.)
static_assert(Offsets::Player::LocalPlayer == Offsets::Players::LocalPlayer,
    "Player::LocalPlayer and Players::LocalPlayer must name the same field");
