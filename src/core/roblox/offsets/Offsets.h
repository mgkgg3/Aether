#pragma once

#include <cstdint>
#include <string>

namespace Offsets {
    inline std::string ClientVersion = "version-02c37bc51a384b8f";

    namespace AirProperties {
         inline constexpr uintptr_t AirDensity = 0x18;
         inline constexpr uintptr_t GlobalWind = 0x3c;
    }

    namespace Alloc {
         inline constexpr uintptr_t Free = 0x5123fe0;
         inline constexpr uintptr_t Malloc = 0x5123f60;
    }

    namespace AnimationTrack {
         inline constexpr uintptr_t Animation = 0xa8;
         inline constexpr uintptr_t Animator = 0x100;
         inline constexpr uintptr_t IsPlaying = 0xa48;
         inline constexpr uintptr_t Looped = 0xd5;
         inline constexpr uintptr_t Speed = 0xc4;
         inline constexpr uintptr_t TimePosition = 0xc8;
    }

    namespace Animator {
         inline constexpr uintptr_t ActiveAnimations = 0xa80;
    }

    namespace Atmosphere {
         inline constexpr uintptr_t Color = 0xa8;
         inline constexpr uintptr_t Decay = 0xb4;
         inline constexpr uintptr_t Density = 0xc0;
         inline constexpr uintptr_t Glare = 0xc4;
         inline constexpr uintptr_t Haze = 0xc8;
         inline constexpr uintptr_t Offset = 0xcc;
    }

    namespace Attachment {
         inline constexpr uintptr_t Position = 0xb4;
    }

    namespace Attribute {
         inline constexpr uintptr_t Key = 0x0;
         inline constexpr uintptr_t Size = 0x58;
         inline constexpr uintptr_t TypeIdRva = 0x7e41a2c;
         inline constexpr uintptr_t Value = 0x8;
    }

    namespace AttributesMap {
         inline constexpr uintptr_t Attributes = 0x10;
         inline constexpr uintptr_t Length = 0x0;
    }

    namespace BasePart {
         inline constexpr uintptr_t CastShadow = 0x125;
         inline constexpr uintptr_t Color3 = 0x198;
         inline constexpr uintptr_t Locked = 0x126;
         inline constexpr uintptr_t Massless = 0x127;
         inline constexpr uintptr_t Primitive = 0x178;
         inline constexpr uintptr_t Reflectance = 0xfc;
         inline constexpr uintptr_t Shape = 0x1a8;
         inline constexpr uintptr_t Transparency = 0x120;
         inline constexpr uintptr_t ClusterNode = 0x180;
    }

    namespace Beam {
         inline constexpr uintptr_t Attachment0 = 0x150;
         inline constexpr uintptr_t Attachment1 = 0x160;
         inline constexpr uintptr_t Brightness = 0x170;
         inline constexpr uintptr_t CurveSize0 = 0x174;
         inline constexpr uintptr_t CurveSize1 = 0x178;
         inline constexpr uintptr_t LightEmission = 0x17c;
         inline constexpr uintptr_t LightInfluence = 0x180;
         inline constexpr uintptr_t Texture = 0x130;
         inline constexpr uintptr_t TextureLength = 0x18c;
         inline constexpr uintptr_t TextureSpeed = 0x194;
         inline constexpr uintptr_t Width0 = 0x198;
         inline constexpr uintptr_t Width1 = 0x19c;
         inline constexpr uintptr_t ZOffset = 0x1a0;
    }

    namespace BloomEffect {
         inline constexpr uintptr_t Enabled = 0xa0;
         inline constexpr uintptr_t Intensity = 0xa8;
         inline constexpr uintptr_t Size = 0xac;
         inline constexpr uintptr_t Threshold = 0xb0;
    }

    namespace BlurEffect {
         inline constexpr uintptr_t Enabled = 0xa0;
         inline constexpr uintptr_t Size = 0xa8;
    }

    namespace ByteCode {
         inline constexpr uintptr_t Pointer = 0x10;
         inline constexpr uintptr_t Size = 0x28;
    }

    namespace Camera {
         inline constexpr uintptr_t CameraSubject = 0xb8;
         inline constexpr uintptr_t CameraType = 0x128;
         inline constexpr uintptr_t FieldOfView = 0x130;
         inline constexpr uintptr_t ImagePlaneDepth = 0x2d4;
         inline constexpr uintptr_t Position = 0xec;
         inline constexpr uintptr_t Rotation = 0xc8;
         inline constexpr uintptr_t Viewport = 0x28c;
         inline constexpr uintptr_t ViewportSize = 0x2cc;
    }

    namespace CharacterMesh {
         inline constexpr uintptr_t BaseTextureId = 0xb8;
         inline constexpr uintptr_t BodyPart = 0x138;
         inline constexpr uintptr_t MeshId = 0xe8;
         inline constexpr uintptr_t OverlayTextureId = 0x118;
    }

    namespace Chat {
         inline constexpr uintptr_t IsFocused = 0x154;
    }

    namespace ClickDetector {
         inline constexpr uintptr_t MaxActivationDistance = 0xd8;
         inline constexpr uintptr_t MouseIcon = 0xb8;
    }

    namespace Clothing {
         inline constexpr uintptr_t Color3 = 0x110;
         inline constexpr uintptr_t Template = 0xf0;
    }

    namespace ColorCorrectionEffect {
         inline constexpr uintptr_t Brightness = 0xb4;
         inline constexpr uintptr_t Contrast = 0xb8;
         inline constexpr uintptr_t Enabled = 0xa0;
         inline constexpr uintptr_t TintColor = 0xa8;
    }

    namespace ColorGradingEffect {
         inline constexpr uintptr_t Enabled = 0xa0;
         inline constexpr uintptr_t TonemapperPreset = 0xa8;
    }

    namespace DataModel {
         inline constexpr uintptr_t CreatorId = 0x178;
         inline constexpr uintptr_t GameId = 0x180;
         inline constexpr uintptr_t GameLoaded = 0x5d0;
         inline constexpr uintptr_t JobId = 0x110;
         inline constexpr uintptr_t PlaceId = 0x188;
         inline constexpr uintptr_t PlaceVersion = 0x1a4;
         inline constexpr uintptr_t PrimitiveCount = 0x418;
         inline constexpr uintptr_t ScriptContext = 0x440;
         inline constexpr uintptr_t ServerIP = 0x5b8;
         inline constexpr uintptr_t ToRenderView1 = 0x1c0;
         inline constexpr uintptr_t ToRenderView2 = 0x8;
         inline constexpr uintptr_t ToRenderView3 = 0x28;
         inline constexpr uintptr_t Workspace = 0x150;
    }

    namespace DepthOfFieldEffect {
         inline constexpr uintptr_t Enabled = 0xa0;
         inline constexpr uintptr_t FarIntensity = 0xa8;
         inline constexpr uintptr_t FocusDistance = 0xac;
         inline constexpr uintptr_t InFocusRadius = 0xb0;
         inline constexpr uintptr_t NearIntensity = 0xb4;
    }

    namespace DragDetector {
         inline constexpr uintptr_t ActivatedCursorIcon = 0x1b0;
         inline constexpr uintptr_t CursorIcon = 0xb8;
         inline constexpr uintptr_t MaxActivationDistance = 0xd8;
         inline constexpr uintptr_t MaxDragAngle = 0x298;
         inline constexpr uintptr_t MaxDragTranslation = 0x25c;
         inline constexpr uintptr_t MaxForce = 0x29c;
         inline constexpr uintptr_t MaxTorque = 0x2a0;
         inline constexpr uintptr_t MinDragAngle = 0x2a4;
         inline constexpr uintptr_t MinDragTranslation = 0x268;
         inline constexpr uintptr_t ReferenceInstance = 0x1e0;
         inline constexpr uintptr_t Responsiveness = 0x2b0;
    }

    namespace FakeDataModel {
         inline constexpr uintptr_t Pointer = 0x8bcfd50;
         inline constexpr uintptr_t RealDataModel = 0x1f8;
    }

    namespace FastClusterEntity {
         inline constexpr uintptr_t AlphaByte = 0x14;
         inline constexpr uintptr_t BBoxMaxX = 0xA4;
         inline constexpr uintptr_t BBoxMaxY = 0xA8;
         inline constexpr uintptr_t BBoxMaxZ = 0xAC;
         inline constexpr uintptr_t BBoxMinX = 0x98;
         inline constexpr uintptr_t BBoxMinY = 0x9C;
         inline constexpr uintptr_t BBoxMinZ = 0xA0;
         inline constexpr uintptr_t ContextPtr = 0x08;
         inline constexpr uintptr_t DecalMaterialPtr = 0x48;
         inline constexpr uintptr_t MaterialPtr = 0x20;
         inline constexpr uintptr_t PrimitiveIndexArrayPtr = 0x80;
         inline constexpr uintptr_t RenderQueueId = 0x10;
         inline constexpr uintptr_t TechniqueArrayPtr = 0x70;
         inline constexpr uintptr_t VTableRva = 0x6d70ce8;

         namespace Context {
              inline constexpr uintptr_t PrimitivePoolPtr = 0x1A0;
         }

         namespace PrimitivePool {
              inline constexpr uintptr_t ArrayBase = 0x20;
         }

         namespace PrimitiveRecord {
              inline constexpr uintptr_t Stride = 48;
              inline constexpr uintptr_t Translation = 36;
         }
    }

    namespace GuiBase2D {
         inline constexpr uintptr_t AbsolutePosition = 0xfc;
         inline constexpr uintptr_t AbsoluteRotation = 0xd8;
         inline constexpr uintptr_t AbsoluteSize = 0x100;
    }

    namespace GuiObject {
         inline constexpr uintptr_t BackgroundColor3 = 0x530;
         inline constexpr uintptr_t BackgroundTransparency = 0x53c;
         inline constexpr uintptr_t BorderColor3 = 0x53c;
         inline constexpr uintptr_t Image = 0x9a8;
         inline constexpr uintptr_t LayoutOrder = 0x56c;
         inline constexpr uintptr_t Position = 0x500;
         inline constexpr uintptr_t RichText = 0xba0;
         inline constexpr uintptr_t Rotation = 0xd8;
         inline constexpr uintptr_t ScreenGui_Enabled = 0x4b4;
         inline constexpr uintptr_t Size = 0x520;
         inline constexpr uintptr_t Text = 0xe08;
         inline constexpr uintptr_t TextColor3 = 0xeb8;
         inline constexpr uintptr_t Visible = 0x59d;
         inline constexpr uintptr_t ZIndex = 0x594;
    }

    namespace Humanoid {
         inline constexpr uintptr_t AutoJumpEnabled = 0x1c4;
         inline constexpr uintptr_t AutomaticScalingEnabled = 0x1c6;
         inline constexpr uintptr_t AutoRotate = 0x1c5;
         inline constexpr uintptr_t BreakJointsOnDeath = 0x1c7;
         inline constexpr uintptr_t CameraOffset = 0x118;
         inline constexpr uintptr_t DisplayDistanceType = 0x170;
         inline constexpr uintptr_t DisplayName = 0xa8;
         inline constexpr uintptr_t EvaluateStateMachine = 0x1c8;
         inline constexpr uintptr_t FloorMaterial = 0x174;
         inline constexpr uintptr_t Health = 0x180;
         inline constexpr uintptr_t HealthDisplayDistance = 0x178;
         inline constexpr uintptr_t HealthDisplayType = 0x17c;
         inline constexpr uintptr_t HipHeight = 0x184;
         inline constexpr uintptr_t HumanoidRootPart = 0x460;
         inline constexpr uintptr_t HumanoidState = 0x8a8;
         inline constexpr uintptr_t HumanoidStateID = 0x20;
         inline constexpr uintptr_t IsWalking = 0xa27;
         inline constexpr uintptr_t Jump = 0x1ca;
         inline constexpr uintptr_t JumpHeight = 0x190;
         inline constexpr uintptr_t JumpPower = 0x194;
         inline constexpr uintptr_t MaxHealth = 0x198;
         inline constexpr uintptr_t MaxSlopeAngle = 0x19c;
         inline constexpr uintptr_t MoveDirection = 0x130;
         inline constexpr uintptr_t MoveToPart = 0x108;
         inline constexpr uintptr_t MoveToPoint = 0x154;
         inline constexpr uintptr_t NameDisplayDistance = 0x1a0;
         inline constexpr uintptr_t NameOcclusion = 0x1a4;
         inline constexpr uintptr_t PlatformStand = 0x1cc;
         inline constexpr uintptr_t PlatformStatePointer = 0xcb84497c;
         inline constexpr uintptr_t RequiresNeck = 0x1cd;
         inline constexpr uintptr_t RigType = 0x1b0;
         inline constexpr uintptr_t SeatPart = 0xf8;
         inline constexpr uintptr_t Sit = 0x1cd;
         inline constexpr uintptr_t TargetPoint = 0x13c;
         inline constexpr uintptr_t UseJumpPower = 0x1d0;
         inline constexpr uintptr_t Walkspeed = 0x1c0;
         inline constexpr uintptr_t WalkspeedCheck = 0x39c;
         inline constexpr uintptr_t WalkTimer = 0x408;
    }

    namespace Instance {
         inline constexpr uintptr_t AttributeContainer = 0x48;
         inline constexpr uintptr_t AttributeList = 0x18;
         inline constexpr uintptr_t AttributeToNext = 0x58;
         inline constexpr uintptr_t AttributeToValue = 0x18;
         inline constexpr uintptr_t ChildrenEnd = 0x8;
         inline constexpr uintptr_t ChildrenStart = 0x78;
         inline constexpr uintptr_t ClassBase = 0x1b0;
         inline constexpr uintptr_t ClassByName = 0x507a230;
         inline constexpr uintptr_t ClassDescriptor = 0x18;
         inline constexpr uintptr_t ClassName = 0x8;
         inline constexpr uintptr_t ComponentMap = 0x38;
         inline constexpr uintptr_t Creator_create = 0x0;
         inline constexpr uintptr_t Creator_isCreatable = 0x10;
         inline constexpr uintptr_t CreatorFromClass = 0x4c32e60;
         inline constexpr uintptr_t FromExisting = 0x1e013c0;
         inline constexpr uintptr_t Name = 0x8;
         inline constexpr uintptr_t New = 0x1e00d00;
         inline constexpr uintptr_t Parent = 0x68;
         inline constexpr uintptr_t PushToLua = 0x1d48ca0;
         inline constexpr uintptr_t SetParent = 0x4a8cf20;
         inline constexpr uintptr_t This = 0x8;
         inline constexpr uintptr_t WhJobNopSlot = 0x10;
         inline constexpr uintptr_t WhJobVftable = 0x60424d8;
         inline constexpr uintptr_t ChildrenStride = 0x10;
         inline constexpr uintptr_t NameContainer = 0x70;
    }

    namespace Lighting {
         inline constexpr uintptr_t Ambient = 0xc0;
         inline constexpr uintptr_t Brightness = 0x108;
         inline constexpr uintptr_t ClockTime = 0xb8;
         inline constexpr uintptr_t ColorShift_Bottom = 0xd8;
         inline constexpr uintptr_t ColorShift_Top = 0xcc;
         inline constexpr uintptr_t EnvironmentDiffuseScale = 0x10c;
         inline constexpr uintptr_t EnvironmentSpecularScale = 0x110;
         inline constexpr uintptr_t ExposureCompensation = 0x114;
         inline constexpr uintptr_t FogColor = 0xe4;
         inline constexpr uintptr_t FogEnd = 0x11c;
         inline constexpr uintptr_t FogStart = 0x120;
         inline constexpr uintptr_t GeographicLatitude = 0x124;
         inline constexpr uintptr_t GlobalShadows = 0x134;
         inline constexpr uintptr_t GradientBottom = 0x180;
         inline constexpr uintptr_t GradientTop = 0x140;
         inline constexpr uintptr_t LightColor = 0x14c;
         inline constexpr uintptr_t LightDirection = 0x158;
         inline constexpr uintptr_t MoonPosition = 0x174;
         inline constexpr uintptr_t OutdoorAmbient = 0xf0;
         inline constexpr uintptr_t Sky = 0x1b8;
         inline constexpr uintptr_t Atmosphere = 0x1c8;
         inline constexpr uintptr_t ShadowSoftness = 0xe8;
         inline constexpr uintptr_t Source = 0x164;
         inline constexpr uintptr_t SunPosition = 0x168;
    }

    namespace LocalScript {
         inline constexpr uintptr_t ByteCode = 0x190;
         inline constexpr uintptr_t GUID = 0xc0;
         inline constexpr uintptr_t Hash = 0x190;
    }

    namespace LuaState {
         inline constexpr uintptr_t Base = 0x40;
         inline constexpr uintptr_t Global = 0x28;
         inline constexpr uintptr_t Top = 0x38;
         inline constexpr uintptr_t TypeTag = 0x0;
    }

    namespace Luau {
         inline constexpr uintptr_t collectgarbage_wrap = 0x1e76340;
         inline constexpr uintptr_t loadstring = 0x1e76ad0;
         inline constexpr uintptr_t lua_gc = 0x4b61f60;
         inline constexpr uintptr_t lua_getfield = 0x4b622b0;
         inline constexpr uintptr_t lua_getglobal = 0x0;
         inline constexpr uintptr_t lua_getmetatable = 0x4b623c0;
         inline constexpr uintptr_t lua_gettop = 0x4b625f0;
         inline constexpr uintptr_t lua_pcall = 0x4b69c60;
         inline constexpr uintptr_t lua_pushboolean = 0x4b62fb0;
         inline constexpr uintptr_t lua_pushlstring = 0x4b63370;
         inline constexpr uintptr_t lua_pushnil = 0x4b63430;
         inline constexpr uintptr_t lua_pushnumber = 0x4b63490;
         inline constexpr uintptr_t lua_pushstring = 0x4b63510;
         inline constexpr uintptr_t lua_pushvalue = 0x4b63740;
         inline constexpr uintptr_t lua_rawget = 0x4b63a40;
         inline constexpr uintptr_t lua_rawgetfield = 0x4b63ae0;
         inline constexpr uintptr_t lua_rawset = 0x4b63f90;
         inline constexpr uintptr_t lua_setfield = 0x4b64870;
         inline constexpr uintptr_t lua_settop = 0x4b64bc0;
         inline constexpr uintptr_t lua_tolstring = 0x4b64ff0;
         inline constexpr uintptr_t luaC_fullgc = 0x4b6cb50;
         inline constexpr uintptr_t luaC_step = 0x4b6cd20;
         inline constexpr uintptr_t luaC_step_work = 0x4b6c800;
         inline constexpr uintptr_t luaL_getmetafield = 0x4b69650;
         inline constexpr uintptr_t pcall_wrap = 0x4b7e4b0;
         inline constexpr uintptr_t print_wrap = 0x1e771f0;
         inline constexpr uintptr_t require = 0x1e772e0;
         inline constexpr uintptr_t require_impl = 0x0;
    }

    namespace LuauGlobal {
         inline constexpr uintptr_t currentwhite = 0x48;
         inline constexpr uintptr_t dummynode = 0x6bc02c8;
         inline constexpr uintptr_t gcopages = 0x90;
         inline constexpr uintptr_t gcopages_end = 0x78;
         inline constexpr uintptr_t gcopages_large = 0xa0;
         inline constexpr uintptr_t gcopages_sizeclass = 0xa8;
         inline constexpr uintptr_t gcpause = 0x18;
         inline constexpr uintptr_t gcstate = 0x49;
         inline constexpr uintptr_t gcstepmul = 0x14;
         inline constexpr uintptr_t gcstepsize = 0x10;
         inline constexpr uintptr_t GCthreshold = 0x0;
         inline constexpr uintptr_t gray = 0x28;
         inline constexpr uintptr_t grayagain = 0x30;
         inline constexpr uintptr_t page_next_all = 0x8;
         inline constexpr uintptr_t page_next_free = 0x18;
         inline constexpr uintptr_t strt_hash = 0x40;
         inline constexpr uintptr_t strt_size = 0x38;
         inline constexpr uintptr_t totalbytes = 0x8;
         inline constexpr uintptr_t weak = 0x20;
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
         inline constexpr uintptr_t ColorData = 0x24;
         inline constexpr uintptr_t FillModeByte = 0x11;
         inline constexpr uintptr_t Flags2 = 0x20;
         inline constexpr uintptr_t MatFlags = 0x18;
         inline constexpr uintptr_t Param = 0x1C;
         inline constexpr uintptr_t Stride = 136;
    }

    namespace MeshContentProvider {
         inline constexpr uintptr_t AssetID = 0x10;
         inline constexpr uintptr_t Cache = 0xd8;
         inline constexpr uintptr_t LRUCache = 0x20;
         inline constexpr uintptr_t MeshData = 0x40;
         inline constexpr uintptr_t ToMeshData = 0x40;
         inline constexpr uintptr_t LRUHolder = 0xc8;
    }

    namespace MeshData {
         inline constexpr uintptr_t FaceEnd = 0x38;
         inline constexpr uintptr_t FaceStart = 0x30;
         inline constexpr uintptr_t VertexEnd = 0x8;
         inline constexpr uintptr_t VertexStart = 0x0;
    }
         inline constexpr uintptr_t TextureId = 0x730;

    namespace MeshPart {
         inline constexpr uintptr_t MeshId = 0x300;
         inline constexpr uintptr_t Texture = 0x330;
    }

    namespace Misc {
         inline constexpr uintptr_t Adornee = 0xe0;
         inline constexpr uintptr_t AnimationId = 0xb0;
         inline constexpr uintptr_t StringLength = 0x10;
         inline constexpr uintptr_t Value = 0xa8;
    }

    namespace Model {
         inline constexpr uintptr_t PrimaryPart = 0x248;
         inline constexpr uintptr_t Scale = 0x134;
    }

    namespace ModuleScript {
         inline constexpr uintptr_t ByteCode = 0x138;
         inline constexpr uintptr_t GUID = 0xc0;
         inline constexpr uintptr_t Hash = 0x350;
         inline constexpr uintptr_t IsCoreScript = 0x0;
    }

    namespace MouseService {
         inline constexpr uintptr_t InputObject = 0xe0;
         inline constexpr uintptr_t InputObject2 = 0xf0;
         inline constexpr uintptr_t MousePosition = 0xc4;
         inline constexpr uintptr_t SensitivityPointer = 0x7fd51b8;
    }

    namespace ParticleEmitter {
         inline constexpr uintptr_t Acceleration = 0x1d0;
         inline constexpr uintptr_t Brightness = 0x20c;
         inline constexpr uintptr_t Drag = 0x210;
         inline constexpr uintptr_t Lifetime = 0x1e4;
         inline constexpr uintptr_t LightEmission = 0x228;
         inline constexpr uintptr_t LightInfluence = 0x22c;
         inline constexpr uintptr_t Rate = 0x238;
         inline constexpr uintptr_t Rotation = 0x1f4;
         inline constexpr uintptr_t RotSpeed = 0x1ec;
         inline constexpr uintptr_t Speed = 0x1fc;
         inline constexpr uintptr_t SpreadAngle = 0x204;
         inline constexpr uintptr_t Texture = 0x1b0;
         inline constexpr uintptr_t TimeScale = 0x24c;
         inline constexpr uintptr_t VelocityInheritance = 0x250;
         inline constexpr uintptr_t ZOffset = 0x254;
    }

    namespace Player {
         inline constexpr uintptr_t AccountAge = 0x34c;
         inline constexpr uintptr_t CameraMode = 0x360;
         inline constexpr uintptr_t DisplayName = 0x128;
         inline constexpr uintptr_t HealthDisplayDistance = 0x384;
         inline constexpr uintptr_t LocaleId = 0x108;
         inline constexpr uintptr_t LocalPlayer = 0x120;
         inline constexpr uintptr_t MaxZoomDistance = 0x358;
         inline constexpr uintptr_t MinZoomDistance = 0x35c;
         inline constexpr uintptr_t ModelInstance = 0x288;
         inline constexpr uintptr_t Character = 0x288;
         inline constexpr uintptr_t Mouse = 0x1208;
         inline constexpr uintptr_t NameDisplayDistance = 0x394;
         inline constexpr uintptr_t Team = 0x2c8;
         inline constexpr uintptr_t TeamColor = 0x3a0;
         inline constexpr uintptr_t UserId = 0xc0;
    }

    namespace PlayerConfigurer {
         inline constexpr uintptr_t Pointer = 0x0;
    }

    namespace PlayerMouse {
         inline constexpr uintptr_t Icon = 0xb8;
         inline constexpr uintptr_t Workspace = 0x140;
    }

    namespace Primitive {
         inline constexpr uintptr_t AssemblyAngularVelocity = 0xec;
         inline constexpr uintptr_t AssemblyLinearVelocity = 0xe0;
         inline constexpr uintptr_t Flags = 0x1b6;
         inline constexpr uintptr_t Material = 0x246;
         inline constexpr uintptr_t Owner = 0x210;
         inline constexpr uintptr_t CFrame = 0xb0;
         inline constexpr uintptr_t Orientation = 0xb0;
         inline constexpr uintptr_t Part = 0x210;
         inline constexpr uintptr_t Position = 0xd4;
         inline constexpr uintptr_t Properties = 0xa0;
         inline constexpr uintptr_t PropertyPosition = 0x90;
         inline constexpr uintptr_t Rotation = 0xb0;
         inline constexpr uintptr_t Size = 0x1bc;
         inline constexpr uintptr_t Validate = 0x6;
    }

    namespace PrimitiveFlags {
         inline constexpr uintptr_t Anchored = 0x2;
         inline constexpr uintptr_t CanCollide = 0x8;
         inline constexpr uintptr_t CanQuery = 0x20;
         inline constexpr uintptr_t CanTouch = 0x10;
    }

    namespace ProximityPrompt {
         inline constexpr uintptr_t ActionText = 0xa0;
         inline constexpr uintptr_t Enabled = 0x126;
         inline constexpr uintptr_t GamepadKeyCode = 0x10c;
         inline constexpr uintptr_t HoldDuration = 0x110;
         inline constexpr uintptr_t KeyCode = 0x114;
         inline constexpr uintptr_t MaxActivationDistance = 0x118;
         inline constexpr uintptr_t ObjectText = 0xc0;
         inline constexpr uintptr_t RequiresLineOfSight = 0x127;
    }

    namespace Reflection {
         inline constexpr uintptr_t ClassDescCreatable = 0x10;
         inline constexpr uintptr_t ClassDescFlags = 0x1bc;
         inline constexpr uintptr_t CreatorTable = 0x85eeae0;
         inline constexpr uintptr_t EntryValue = 0x8;
         inline constexpr uintptr_t NameRegistry = 0x822c9c0;
         inline constexpr uintptr_t NameTable = 0x50;
         inline constexpr uintptr_t TableEmpty = 0x20;
         inline constexpr uintptr_t TableEnd = 0x8;
         inline constexpr uintptr_t TableStart = 0x0;
         inline constexpr uintptr_t TableStride = 0x10;
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
         inline constexpr uintptr_t SkyValid = 0x28d;
         inline constexpr uintptr_t VisualEngine = 0x18;
    }

    namespace RunService {
         inline constexpr uintptr_t HeartbeatFPS = 0xe1;
         inline constexpr uintptr_t HeartbeatTask = 0x6338;
    }

    namespace Script {
         inline constexpr uintptr_t ByteCode = 0x190;
         inline constexpr uintptr_t GUID = 0xc0;
         inline constexpr uintptr_t Hash = 0x190;
    }

    namespace ScriptContext {
         inline constexpr uintptr_t LuaState = 0x28;
         inline constexpr uintptr_t LuaState2 = 0x28;
         inline constexpr uintptr_t LuaStateAlt = 0xe8;
         inline constexpr uintptr_t RequireBypass = 0x0;
         inline constexpr uintptr_t VmEncryptedLuaState = 0xd0;
         inline constexpr uintptr_t VmWrapper = 0x220;
         inline constexpr uintptr_t VmWrapper2 = 0x528;
         inline constexpr uintptr_t VmWrapperBig = 0x440;
    }

    namespace Seat {
         inline constexpr uintptr_t Occupant = 0x208;
    }

    namespace Sky {
         inline constexpr uintptr_t MoonAngularSize = 0x234;
         inline constexpr uintptr_t MoonTextureId = 0xb8;
         inline constexpr uintptr_t SkyboxBk = 0xe8;
         inline constexpr uintptr_t SkyboxDn = 0x118;
         inline constexpr uintptr_t SkyboxFt = 0x148;
         inline constexpr uintptr_t SkyboxLf = 0x178;
         inline constexpr uintptr_t SkyboxOrientation = 0x228;
         inline constexpr uintptr_t SkyboxRt = 0x1a8;
         inline constexpr uintptr_t SkyboxUp = 0x1d8;
         inline constexpr uintptr_t StarCount = 0x238;
         inline constexpr uintptr_t SunAngularSize = 0x22c;
         inline constexpr uintptr_t SunTextureId = 0x208;
    }

    namespace Sound {
         inline constexpr uintptr_t IsPlaying = 0x130;
         inline constexpr uintptr_t Looped = 0x12d;
         inline constexpr uintptr_t PlaybackSpeed = 0x10c;
         inline constexpr uintptr_t RollOffMaxDistance = 0x110;
         inline constexpr uintptr_t RollOffMinDistance = 0x114;
         inline constexpr uintptr_t SoundGroup = 0xd8;
         inline constexpr uintptr_t SoundId = 0xb8;
         inline constexpr uintptr_t Volume = 0x120;
    }

    namespace SpawnLocation {
         inline constexpr uintptr_t AllowTeamChangeOnTouch = 0x1e0;
         inline constexpr uintptr_t Enabled = 0x1e1;
         inline constexpr uintptr_t TextureId = 0x118;
         inline constexpr uintptr_t ForcefieldDuration = 0x1d8;
         inline constexpr uintptr_t Neutral = 0x1e2;
         inline constexpr uintptr_t TeamColor = 0x1dc;
    }

    namespace SpecialMesh {
         inline constexpr uintptr_t MeshId = 0xe8;
         inline constexpr uintptr_t Offset = 0xa8;
         inline constexpr uintptr_t Scale = 0xb4;
    }

    namespace StatsItem {
         inline constexpr uintptr_t Value = 0x5e0;
    }

    namespace SunRaysEffect {
         inline constexpr uintptr_t Enabled = 0xa0;
         inline constexpr uintptr_t Intensity = 0xa8;
         inline constexpr uintptr_t Spread = 0xac;
    }

    namespace SurfaceAppearance {
         inline constexpr uintptr_t AlphaMode = 0x1e0;
         inline constexpr uintptr_t Color = 0x1c8;
         inline constexpr uintptr_t ColorMap = 0xb8;
         inline constexpr uintptr_t EmissiveMaskContent = 0xe8;
         inline constexpr uintptr_t EmissiveStrength = 0x1e4;
         inline constexpr uintptr_t EmissiveTint = 0x1d4;
         inline constexpr uintptr_t MetalnessMap = 0x118;
         inline constexpr uintptr_t NormalMap = 0x148;
         inline constexpr uintptr_t RoughnessMap = 0x178;
    }

         inline constexpr uintptr_t TeamColor = 0xa8;
    namespace TaskScheduler {
         inline constexpr uintptr_t JobEnd = 0xd0;
         inline constexpr uintptr_t JobName = 0x18;
         inline constexpr uintptr_t JobStart = 0xc8;
         inline constexpr uintptr_t MaxFPS = 0xb0;
         inline constexpr uintptr_t Pointer = 0x8b79128;
    }

    namespace Team {
         inline constexpr uintptr_t BrickColor = 0xa8;
    }

    namespace TechniqueArray {
         inline constexpr uintptr_t BeginOffset = 0x0;
         inline constexpr uintptr_t EndOffset = 0x8;
         inline constexpr uintptr_t EntryStride = 136;
    }

    namespace Terrain {
         inline constexpr uintptr_t GrassLength = 0x1e0;
         inline constexpr uintptr_t MaterialColors = 0x4a8;
         inline constexpr uintptr_t WaterColor = 0x1d0;
         inline constexpr uintptr_t WaterReflectance = 0x1e8;
         inline constexpr uintptr_t WaterTransparency = 0x1ec;
         inline constexpr uintptr_t WaterWaveSize = 0x1f0;
         inline constexpr uintptr_t WaterWaveSpeed = 0x1f4;
    }

    namespace Textures {
         inline constexpr uintptr_t Decal_Texture = 0x1d0;
         inline constexpr uintptr_t Texture_Texture = 0x1d0;
    }

    namespace Tool {
         inline constexpr uintptr_t CanBeDropped = 0x4a8;
         inline constexpr uintptr_t Enabled = 0x4a9;
         inline constexpr uintptr_t Grip = 0x49c;
         inline constexpr uintptr_t ManualActivationOnly = 0x4aa;
         inline constexpr uintptr_t RequiresHandle = 0x4ab;
         inline constexpr uintptr_t TextureId = 0x350;
         inline constexpr uintptr_t Tooltip = 0x458;
    }

    namespace UnionOperation {
         inline constexpr uintptr_t AssetId = 0x300;
    }
         inline constexpr uintptr_t Occupant = 0x1f8;

    namespace UserInputService {
         inline constexpr uintptr_t WindowInputState = 0x2b0;
    }

    namespace VehicleSeat {
         inline constexpr uintptr_t MaxSpeed = 0x218;
         inline constexpr uintptr_t SteerFloat = 0x21c;
         inline constexpr uintptr_t ThrottleFloat = 0x220;
         inline constexpr uintptr_t Torque = 0x224;
         inline constexpr uintptr_t TurnSpeed = 0x228;
    }

    namespace VisualEngine {
         inline constexpr uintptr_t Dimensions = 0xb10;
         inline constexpr uintptr_t FakeDataModel = 0xaf0;
         inline constexpr uintptr_t Pointer = 0x8656e40;
         inline constexpr uintptr_t RenderView = 0xc30;
         inline constexpr uintptr_t ViewMatrix = 0x1b0;
    }

    namespace Weld {
         inline constexpr uintptr_t Part0 = 0x108;
         inline constexpr uintptr_t Part1 = 0x118;
    }

    namespace WeldConstraint {
         inline constexpr uintptr_t Part0 = 0xa8;
         inline constexpr uintptr_t Part1 = 0xb8;
    }

    namespace WindowInputState {
         inline constexpr uintptr_t CapsLock = 0x40;
         inline constexpr uintptr_t CurrentTextBox = 0x48;
    }

    namespace Workspace {
         inline constexpr uintptr_t WorldSteps = 0x748;
         inline constexpr uintptr_t CurrentCamera = 0x4a8;
         inline constexpr uintptr_t DistributedGameTime = 0x4c8;
         inline constexpr uintptr_t ReadOnlyGravity = 0x9c8;
         inline constexpr uintptr_t World = 0x400;
    }

    namespace World {
         inline constexpr uintptr_t AirProperties = 0x238;
         inline constexpr uintptr_t FallenPartsDestroyHeight = 0x218;
         inline constexpr uintptr_t Gravity = 0x224;
         inline constexpr uintptr_t Primitives = 0x2c8;
         inline constexpr uintptr_t worldStepsPerSec = 0x740;
    }

    namespace WorldRoot {
         inline constexpr uintptr_t RaycastBoundDesc = 0x8364bb0;
         inline constexpr uintptr_t RaycastBoundFn = 0x90;
    }

    namespace Raycast {
         inline constexpr uintptr_t RaycastBoundDesc = 0x8364bb0;
         inline constexpr uintptr_t RaycastBoundDescFallback = 0x8197D90;
         inline constexpr uintptr_t RaycastBoundDescProven = 0x8089F20;
         inline constexpr uintptr_t RaycastBoundFn = 0x90;
    }

    namespace CachedItem {
         inline constexpr uintptr_t FileMeshData = 0x40;
    }

    namespace FileMeshData {
         inline constexpr uintptr_t AABBMax = 0x18c;
         inline constexpr uintptr_t AABBMin = 0x180;
         inline constexpr uintptr_t Faces = 0x30;
         inline constexpr uintptr_t FacesEnd = 0x38;
         inline constexpr uintptr_t Vertices = 0x0;
         inline constexpr uintptr_t VerticesEnd = 0x8;
    }

    namespace LRUHolder {
         inline constexpr uintptr_t MemEnforcedLRUCache = 0x20;
    }

    namespace LRUNode {
         inline constexpr uintptr_t AssetID = 0x10;
         inline constexpr uintptr_t CachedItem = 0x40;
         inline constexpr uintptr_t Next = 0x0;
    }

    namespace MemEnforcedLRUCache {
         inline constexpr uintptr_t Head = 0x8;
    }

    namespace ClassDescriptor {
         inline constexpr uintptr_t ClassName = 0x8;
         inline constexpr uintptr_t Creator = 0x230;
         inline constexpr uintptr_t EventDescriptors = 0x88;
         inline constexpr uintptr_t FunctionDescriptors = 0xD0;
         inline constexpr uintptr_t PropertyDescriptors = 0x40;
    }

    namespace Creator {
         inline constexpr uintptr_t MapEnd = 0x85EEAE8;
         inline constexpr uintptr_t MapStart = 0x85EEAE0;
    }

    namespace Descriptor {
         inline constexpr uintptr_t Name = 0x8;
    }

    namespace DeviceD3D11Gfx {
         inline constexpr uintptr_t VTableRva = 0x6CF09B0;
    }

    namespace FastCluster {
         inline constexpr uintptr_t VTableRva = 0x6D5BC50;
         inline constexpr uintptr_t VTableRvaSub = 0x6D5BC70;
    }

    namespace FastClusterBinding {
         inline constexpr uintptr_t VTableRva = 0x6D5CD60;
    }

    namespace Fire {
         inline constexpr uintptr_t FireProximityPrompt = 0x3192070;
    }

    namespace FunctionDescriptor {
         inline constexpr uintptr_t Function = 0x80;
    }

    namespace Functions {
         inline constexpr uintptr_t Clone = 0x158D3A0;
         inline constexpr uintptr_t Destroy = 0x158D3C0;
         inline constexpr uintptr_t FindPartOnRay = 0xE0FDE0;
         inline constexpr uintptr_t FindPartOnRayWithIgnoreList = 0xE0FE60;
         inline constexpr uintptr_t FindPartOnRayWithWhitelist = 0xE0FEF0;
         inline constexpr uintptr_t FireServer = 0xBD8280;
         inline constexpr uintptr_t Print = 0x1D2EEB0;
         inline constexpr uintptr_t RaisePropertyChanged = 0xEC1B36;
         inline constexpr uintptr_t Raycast = 0xE074B0;
         inline constexpr uintptr_t SetParent = 0xE01B80;
         inline constexpr uintptr_t SetParentInternal = 0x1D76F10;
         inline constexpr uintptr_t Shapecast = 0xE08E70;
    }

    namespace GeometryD3D11 {
         inline constexpr uintptr_t VTableRva = 0x6CF1120;
    }

    namespace Highlight {
         inline constexpr uintptr_t Adornee = 0xA8;
         inline constexpr uintptr_t DepthMode = 0x9D;
         inline constexpr uintptr_t FillColor = 0xB8;
         inline constexpr uintptr_t FillTransparency = 0xD4;
         inline constexpr uintptr_t OutlineColor = 0xC4;
         inline constexpr uintptr_t OutlineTransparency = 0xDC;
    }

    namespace HopperBin {
         inline constexpr uintptr_t BinType = 0x458;
    }

    namespace ICreator {
         inline constexpr uintptr_t Create = 0x0;
    }

    namespace InputObject {
         inline constexpr uintptr_t MousePosition = 0x5C;
    }

    namespace InstancedBinding2 {
         inline constexpr uintptr_t Cluster = 0x68;
    }

    namespace InstancedCluster2 {
         inline constexpr uintptr_t VTableRva = 0x6D5ACB0;
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

    namespace Players {
         inline constexpr uintptr_t LocalPlayer = 0x120;
    }

    namespace PrimitivePool {
         inline constexpr uintptr_t ArrayBase = 0x20;
    }

    namespace PrimitiveRecord {
         inline constexpr uintptr_t Stride = 0x30;
         inline constexpr uintptr_t Translation = 0x24;
    }

    namespace PropertyDescriptor {
         inline constexpr uintptr_t GetSetImpl = 0x90;
         inline constexpr uintptr_t TType = 0x68;
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

    namespace SmoothClusterNode {
         inline constexpr uintptr_t VTableRva = 0x6D5CB98;
    }

    namespace TextButton {
         inline constexpr uintptr_t AutoButtonColor = 0x9CC;
         inline constexpr uintptr_t ContentText = 0x1070;
         inline constexpr uintptr_t LineHeight = 0xE0;
         inline constexpr uintptr_t LocalizedText = 0x1070;
         inline constexpr uintptr_t MaxVisibleGraphemes = 0xA8;
         inline constexpr uintptr_t Modal = 0x9CD;
         inline constexpr uintptr_t RichText = 0x6;
         inline constexpr uintptr_t Selected = 0x9CE;
         inline constexpr uintptr_t Text = 0x1070;
         inline constexpr uintptr_t TextColor3 = 0xF8C;
         inline constexpr uintptr_t TextDirection = 0x6;
         inline constexpr uintptr_t TextScaled = 0x6;
         inline constexpr uintptr_t TextSize = 0x1144;
         inline constexpr uintptr_t TextStrokeColor3 = 0xC;
         inline constexpr uintptr_t TextStrokeTransparency = 0xE0;
         inline constexpr uintptr_t TextTransparency = 0x20;
         inline constexpr uintptr_t TextTruncate = 0x6;
         inline constexpr uintptr_t TextWrapped = 0x6;
         inline constexpr uintptr_t TextXAlignment = 0xF64;
         inline constexpr uintptr_t TextYAlignment = 0xD;
    }

    namespace TextLabel {
         inline constexpr uintptr_t ContentText = 0xDF0;
         inline constexpr uintptr_t LineHeight = 0xE0;
         inline constexpr uintptr_t LocalizedText = 0xDF0;
         inline constexpr uintptr_t MaxVisibleGraphemes = 0xA8;
         inline constexpr uintptr_t RichText = 0xEDD;
         inline constexpr uintptr_t Text = 0xDF0;
         inline constexpr uintptr_t TextColor3 = 0xD0C;
         inline constexpr uintptr_t TextDirection = 0x6;
         inline constexpr uintptr_t TextScaled = 0x6;
         inline constexpr uintptr_t TextSize = 0xEC4;
         inline constexpr uintptr_t TextStrokeColor3 = 0xC;
         inline constexpr uintptr_t TextStrokeTransparency = 0xE0;
         inline constexpr uintptr_t TextTransparency = 0x20;
         inline constexpr uintptr_t TextTruncate = 0x6;
         inline constexpr uintptr_t TextWrapped = 0xEE0;
         inline constexpr uintptr_t TextXAlignment = 0xCE4;
         inline constexpr uintptr_t TextYAlignment = 0xD;
    }

    namespace Types {
         inline constexpr uintptr_t AllTypes = 0x8AEA748;
    }

    namespace Value {
         inline constexpr uintptr_t Value = 0xA8;
    }
}
