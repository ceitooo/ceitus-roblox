/*                          jingohok
/*                  best dumper? you need
/*                  contact me not jonah
/*                  jonah is a loser im boss
/*                  skid v300
/*                  https://discord.gg/r6vb93eEmq

/*  Roblox Version  : version-4310300497aa4917
/*  Total Offsets   : 709
*/

#pragma once
#include <cstdint>
#include <string>

namespace Offsets {
    inline std::string ClientVersion = "version-4310300497aa4917";

    namespace Adornment {
        inline uintptr_t Adornee = 0xB8;
        inline uintptr_t Effect = 0xF8;
        inline uintptr_t FillColor = 0xD0;
        inline uintptr_t LineThickness = 0xF0;
        inline uintptr_t ModelModifier = 0x100;
        inline uintptr_t OutlineColor = 0xDC;
        inline uintptr_t Prop = 0xB0;
        inline uintptr_t ReservedId = 0xF4;
    }

    namespace AirProperties {
        inline uintptr_t AirDensity = 0x18;
        inline uintptr_t GlobalWind = 0x3C;
    }

    namespace Alloc {
        inline uintptr_t Malloc = 0x17E3C30; // not updated
    }

    namespace Animator {
        inline uintptr_t ActiveAnimations = 0xAB0;
    }

    namespace Atmosphere {
        inline uintptr_t Color = 0xB8;
        inline uintptr_t Decay = 0xC4;
        inline uintptr_t Density = 0xD0;
        inline uintptr_t Glare = 0xD4;
        inline uintptr_t Haze = 0xD8;
        inline uintptr_t Offset = 0xDC;
    }

    namespace Attachment {
        inline uintptr_t Position = 0xC4;
    }

    namespace Attribute {
        inline uintptr_t Key = 0x0;
        inline uintptr_t Size = 0x58;
        inline uintptr_t TypeIdRva = 0x87948E4;
        inline uintptr_t TypeIdRvaNew = 0x87949D4;
        inline uintptr_t Value = 0x8;
    }

    namespace AttributesMap {
        inline uintptr_t Attributes = 0x10;
        inline uintptr_t Length = 0x0;
    }

    namespace BasePart {
        inline uintptr_t CastShadow = 0x135;
        inline uintptr_t Color3 = 0x1A8;
        inline uintptr_t Locked = 0x136;
        inline uintptr_t Massless = 0x137;
        inline uintptr_t Primitive = 0x188;
        inline uintptr_t Reflectance = 0x10C;
        inline uintptr_t Shape = 0x1B8;
        inline uintptr_t Transparency = 0x130;
    }

    namespace Beam {
        inline uintptr_t Attachment0 = 0x160;
        inline uintptr_t Attachment1 = 0x160;
        inline uintptr_t Brightness = 0x180;
        inline uintptr_t CurveSize0 = 0x184;
        inline uintptr_t CurveSize1 = 0x188;
        inline uintptr_t LightEmission = 0x18C;
        inline uintptr_t LightInfluence = 0x190;
        inline uintptr_t TextureLength = 0x19C;
        inline uintptr_t TextureSpeed = 0x1A4;
        inline uintptr_t Width0 = 0x1A8;
        inline uintptr_t Width1 = 0x1AC;
        inline uintptr_t ZOffset = 0x1B0;
    }

    namespace BloomEffect {
        inline uintptr_t Enabled = 0x30;
        inline uintptr_t Intensity = 0xB8;
        inline uintptr_t Size = 0xBC;
        inline uintptr_t Threshold = 0xC0;
    }

    namespace BlurEffect {
        inline uintptr_t Enabled = 0x30;
        inline uintptr_t Size = 0xB8;
    }

    namespace ByteCode {
        inline uintptr_t Pointer = 0x10;
        inline uintptr_t Size = 0x28;
    }

    namespace CachedItem {
        inline uintptr_t FileMeshData = 0x40;
    }

    namespace Camera {
        inline uintptr_t CFrame = 0xD8;
        inline uintptr_t CameraSubject = 0xC8;
        inline uintptr_t CameraType = 0x138;
        inline uintptr_t FieldOfView = 0x140;
        inline uintptr_t Position = 0xFC;
        inline uintptr_t Rotation = 0xD8;
        inline uintptr_t Viewport = 0x28C;
        inline uintptr_t ViewportInt16 = 0x28C;
        inline uintptr_t ViewportSize = 0x2CC;
    }

    namespace CharacterMesh {
        inline uintptr_t BaseTextureId = 0xC8;
        inline uintptr_t BodyPart = 0x148;
        inline uintptr_t MeshId = 0xF8;
        inline uintptr_t OverlayTextureId = 0x128;
    }

    namespace Chat {
        inline uintptr_t IsFocused = 0x154;
    }

    namespace ClassDescriptor {
        inline uintptr_t ClassName = 0x8;
        inline uintptr_t Creator = 0x230;
        inline uintptr_t EventDescriptors = 0x88;
        inline uintptr_t FunctionDescriptors = 0xD0;
        inline uintptr_t PropertyDescriptors = 0x40;
    }

    namespace ClickDetector {
        inline uintptr_t MaxActivationDistance = 0xE8;
        inline uintptr_t MouseIcon = 0xC8;
    }

    namespace Clothing {
        inline uintptr_t Color3 = 0x100;
        inline uintptr_t Template = 0x100;
    }

    namespace ColorCorrectionEffect {
        inline uintptr_t Brightness = 0xC4;
        inline uintptr_t Contrast = 0xC8;
        inline uintptr_t Enabled = 0x30;
        inline uintptr_t TintColor = 0xB0;
    }

    namespace ColorGradingEffect {
        inline uintptr_t Enabled = 0x30;
        inline uintptr_t TonemapperPreset = 0xB8;
    }

    namespace Creator {
        inline uintptr_t MapEnd = 0x844F058;
        inline uintptr_t MapStart = 0x844F050;
    }

    namespace DataModel {
        inline uintptr_t CreatorId = 0x188;
        inline uintptr_t GameId = 0x190;
        inline uintptr_t GameLoaded = 0x5E0;
        inline uintptr_t JobId = 0x120;
        inline uintptr_t PlaceId = 0x198;
        inline uintptr_t ServerIP = 0x5C8;
        inline uintptr_t ToRenderView1 = 0x1D0;
        inline uintptr_t ToRenderView2 = 0x8;
        inline uintptr_t ToRenderView3 = 0x28;
        inline uintptr_t Workspace = 0x160;
    }

    namespace DepthOfFieldEffect {
        inline uintptr_t Enabled = 0x30;
        inline uintptr_t FarIntensity = 0xB8;
        inline uintptr_t FocusDistance = 0xBC;
        inline uintptr_t InFocusRadius = 0xC0;
        inline uintptr_t NearIntensity = 0xC4;
    }

    namespace Descriptor {
        inline uintptr_t Name = 0x8;
    }

    namespace DragDetector {
        inline uintptr_t MaxActivationDistance = 0xE8;
        inline uintptr_t MaxDragAngle = 0x2A8;
        inline uintptr_t MaxForce = 0x2AC;
        inline uintptr_t MaxTorque = 0x2B0;
        inline uintptr_t MinDragAngle = 0x2B4;
        inline uintptr_t Responsiveness = 0x2C0;
    }

    namespace FakeDataModel {
        inline uintptr_t Pointer = 0x8E42C98;
        inline uintptr_t RealDataModel = 0x1F8;
    }

    namespace FastClusterEntity {
        inline uintptr_t AlphaByte = 0x14;
        inline uintptr_t BBoxMaxX = 0xA4;
        inline uintptr_t BBoxMaxY = 0xA8;
        inline uintptr_t BBoxMaxZ = 0xAC;
        inline uintptr_t BBoxMinX = 0x98;
        inline uintptr_t BBoxMinY = 0x9C;
        inline uintptr_t BBoxMinZ = 0xA0;
        inline uintptr_t ContextPtr = 0x8;
        inline uintptr_t DecalMaterialPtr = 0x48;
        inline uintptr_t MaterialPtr = 0x20;
        inline uintptr_t PrimitiveIndexArrayPtr = 0x80;
        inline uintptr_t RenderQueueId = 0x10;
        inline uintptr_t TechniqueArrayPtr = 0x70;
        inline uintptr_t VTableRva = 0x6B3EC18;

        namespace Context {
            inline uintptr_t PrimitivePoolPtr = 0x1A0;
        }

        namespace PrimitivePool {
            inline uintptr_t ArrayBase = 0x20;
        }

        namespace PrimitiveRecord {
            inline uintptr_t Stride = 0x30;
            inline uintptr_t Translation = 0x24;
        }
    }

    namespace FileMeshData {
        inline uintptr_t AabbMax = 0x18C;
        inline uintptr_t AabbMin = 0x180;
        inline uintptr_t Faces = 0x30;
        inline uintptr_t FacesEnd = 0x38;
        inline uintptr_t Vertices = 0x0;
        inline uintptr_t VerticesEnd = 0x8;
    }

    namespace Fire {
        inline uintptr_t FireProximityPrompt = 0x3102650;
    }

    namespace FunctionDescriptor {
        inline uintptr_t Function = 0x80;
    }

    namespace Functions {
        inline uintptr_t Clone = 0x1642CE0;
        inline uintptr_t Destroy = 0x1642D00;
        inline uintptr_t FindPartOnRay = 0xECE7A0;
        inline uintptr_t FindPartOnRayWithIgnoreList = 0xECE820;
        inline uintptr_t FindPartOnRayWithWhitelist = 0xECE8B0;
        inline uintptr_t FireServer = 0xCAC950;
        inline uintptr_t Print = 0x1CAB4B0;
        inline uintptr_t RaisePropertyChanged = 0xF7C720;
        inline uintptr_t Raycast = 0xEC5D60;
        inline uintptr_t SetParent = 0xEC0090;
        inline uintptr_t SetParentInternal = 0x1CF3A00;
        inline uintptr_t SetParent_User = 0x1CD37F0;
        inline uintptr_t Shapecast = 0xEC7720;
    }

    namespace GuiBase2D {
        inline uintptr_t AbsolutePosition = 0x108;
        inline uintptr_t AbsoluteRotation = 0xE8;
        inline uintptr_t AbsoluteSize = 0x114;
    }

    namespace GuiObject {
        inline uintptr_t Active = 0x5A8;
        inline uintptr_t AnchorPoint = 0x54C;
        inline uintptr_t AutomaticSize = 0x560;
        inline uintptr_t BackgroundColor3 = 0x540;
        inline uintptr_t BackgroundTransparency = 0x564;
        inline uintptr_t BorderColor3 = 0x54C;
        inline uintptr_t BorderMode = 0x568;
        inline uintptr_t BorderSizePixel = 0x56C;
        inline uintptr_t ClipsDescendants = 0x5A9;
        inline uintptr_t GuiState = 0x578;
        inline uintptr_t Interactable = 0x5AB;
        inline uintptr_t LayoutOrder = 0x57C;
        inline uintptr_t Position = 0x510;
        inline uintptr_t Rotation = 0xE8;
        inline uintptr_t Selectable = 0x5AC;
        inline uintptr_t SelectionOrder = 0x598;
        inline uintptr_t Size = 0x530;
        inline uintptr_t SizeConstraint = 0x5A0;
        inline uintptr_t Visible = 0x5AD;
        inline uintptr_t ZIndex = 0x5A4;
    }

    namespace Highlight {
        inline uintptr_t Adornee = 0xB8;
        inline uintptr_t DepthMode = 0xE0;
        inline uintptr_t Effect = 0xF8;
        inline uintptr_t Enabled = 0xF4;
        inline uintptr_t FillColor = 0xC8;
        inline uintptr_t FillColor_User = 0xD0;
        inline uintptr_t FillTransparency = 0xE4;
        inline uintptr_t LineThickness = 0xF0;
        inline uintptr_t ModelModifier = 0x100;
        inline uintptr_t OutlineColor = 0xD4;
        inline uintptr_t OutlineColor_User = 0xDC;
        inline uintptr_t OutlineTransparency = 0xEC;
        inline uintptr_t Prop = 0xB0;
        inline uintptr_t ReservedId = 0xF4;
    }

    namespace HopperBin {
        inline uintptr_t BinType = 0x468;
    }

    namespace Humanoid {
        inline uintptr_t AutoJumpEnabled = 0x1D4;
        inline uintptr_t AutoRotate = 0x1D5;
        inline uintptr_t AutomaticScalingEnabled = 0x1D6;
        inline uintptr_t BreakJointsOnDeath = 0xC5;
        inline uintptr_t CameraOffset = 0x128;
        inline uintptr_t DisplayDistanceType = 0x180;
        inline uintptr_t DisplayName = 0xB8;
        inline uintptr_t EvaluateStateMachine = 0x1D8;
        inline uintptr_t Health = 0x190;
        inline uintptr_t HealthDisplayDistance = 0x188;
        inline uintptr_t HealthDisplayType = 0x18C;
        inline uintptr_t HipHeight = 0x194;
        inline uintptr_t HumanoidRootPart = 0x470;
        inline uintptr_t HumanoidState = 0x8B8;
        inline uintptr_t HumanoidStateID = 0x20;
        inline uintptr_t IsWalking = 0x95F;
        inline uintptr_t Jump = 0x1DA;
        inline uintptr_t JumpHeight = 0x1A0;
        inline uintptr_t JumpPower = 0x1A4;
        inline uintptr_t MaxHealth = 0x1A8;
        inline uintptr_t MaxSlopeAngle = 0x1AC;
        inline uintptr_t MoveDirection = 0x140;
        inline uintptr_t MoveToPart = 0x118;
        inline uintptr_t MoveToPoint = 0x164;
        inline uintptr_t NameDisplayDistance = 0x1B0;
        inline uintptr_t NameOcclusion = 0x1B4;
        inline uintptr_t PlatformStand = 0x1DC;
        inline uintptr_t PlatformStatePointer = 0x0;
        inline uintptr_t RequiresNeck = 0x1DD;
        inline uintptr_t RigType = 0x1C0;
        inline uintptr_t SeatPart = 0x108;
        inline uintptr_t Sit = 0x1DD;
        inline uintptr_t TargetPoint = 0x14C;
        inline uintptr_t UseJumpPower = 0x1E0;
        inline uintptr_t WalkSpeed = 0x1D0;
        inline uintptr_t WalkSpeedCheck = 0x3B4;
        inline uintptr_t WalkToPoint = 0x164;
        inline uintptr_t Walkspeed = 0x1D0;
        inline uintptr_t WalkspeedCheck = 0x3B4;
    }

    namespace ICreator {
        inline uintptr_t Create = 0x0;
    }

    namespace InputObject {
        inline uintptr_t MousePosition = 0xD4;
    }

    namespace Instance {
        inline uintptr_t AttributeContainer = 0x40;
        inline uintptr_t AttributeList = 0x10;
        inline uintptr_t AttributeToNext = 0x58;
        inline uintptr_t AttributeToValue = 0x18;
        inline uintptr_t ChildrenEnd = 0x8;
        inline uintptr_t ChildrenStart = 0x78;
        inline uintptr_t ChildrenStride = 0x10;
        inline uintptr_t ClassByName = 0x46F6BFE;
        inline uintptr_t ClassDescriptor = 0x18;
        inline uintptr_t ClassName = 0x8;
        inline uintptr_t ComponentMap = 0x38;
        inline uintptr_t Creator_create = 0x0;
        inline uintptr_t Creator_isCreatable = 0x10;
        inline uintptr_t FromExisting = 0x417C6B0;
        inline uintptr_t Name = 0x8;
        inline uintptr_t NameContainer = 0x70;
        inline uintptr_t New = 0x360A4D0;
        inline uintptr_t Parent = 0x68;
        inline uintptr_t SetParent = 0x7E4470;
        inline uintptr_t This = 0x8;
        inline uintptr_t WhJobNopSlot = 0x10;
        inline uintptr_t ClassBase = 0x1B0;
    }

    namespace Lighting {
        inline uintptr_t Ambient = 0xD0;
        inline uintptr_t Atmosphere = 0x1D8;
        inline uintptr_t Brightness = 0x118;
        inline uintptr_t ClockTime = 0xC8;
        inline uintptr_t ColorShift_Bottom = 0xDC;
        inline uintptr_t ColorShift_Top = 0xE8;
        inline uintptr_t EnvironmentDiffuseScale = 0x11C;
        inline uintptr_t EnvironmentSpecularScale = 0x120;
        inline uintptr_t ExposureCompensation = 0x124;
        inline uintptr_t FogColor = 0xF4;
        inline uintptr_t FogEnd = 0x12C;
        inline uintptr_t FogStart = 0x130;
        inline uintptr_t GeographicLatitude = 0x134;
        inline uintptr_t GradientBottom = 0x138;
        inline uintptr_t GradientTop = 0x150;
        inline uintptr_t LightColor = 0x15C;
        inline uintptr_t LightDirection = 0x168;
        inline uintptr_t MoonPosition = 0x184;
        inline uintptr_t OutdoorAmbient = 0x100;
        inline uintptr_t ShadowSoftness = 0x13C;
        inline uintptr_t Sky = 0x1C8;
        inline uintptr_t Source = 0x174;
        inline uintptr_t SunPosition = 0x178;
    }

    namespace LightingParameters {
        inline uintptr_t GeographicLatitude = 0x134;
        inline uintptr_t LightColor = 0x15C;
        inline uintptr_t LightDirection = 0x168;
        inline uintptr_t SkyAmbient = 0x150;
        inline uintptr_t SkyAmbient2 = 0x138;
        inline uintptr_t Source = 0x174;
        inline uintptr_t TrueMoonPosition = 0x184;
        inline uintptr_t TrueSunPosition = 0x178;
    }

    namespace LocalScript {
        inline uintptr_t ByteCode = 0x190;
        inline uintptr_t Bytecode = 0x190;
        inline uintptr_t Hash = 0x1A0;
        inline uintptr_t GUID = 0xD0;
    }

    namespace LruHolder {
        inline uintptr_t MemEnforcedLRUCache = 0x20;
    }

    namespace LruNode {
        inline uintptr_t CachedItem = 0x38;
        inline uintptr_t MeshId = 0x10;
        inline uintptr_t Next = 0x0;
    }

    namespace LuaState {
        inline uintptr_t Base = 0x28;
        inline uintptr_t Global = 0x20;
        inline uintptr_t Top = 0x8;
        inline uintptr_t TypeTag = 0x0;
    }

    namespace Luau {
        inline uintptr_t lua_getglobal = 0x0;
        inline uintptr_t require_impl = 0x0;
    }

    namespace LuauGlobal {
        inline uintptr_t GCthreshold = 0x48;
        inline uintptr_t currentwhite = 0x58;
        inline uintptr_t dummynode = 0x6CF3AF8;
        inline uintptr_t gcopages = 0x2F0;
        inline uintptr_t gcopages_end = 0x2F8;
        inline uintptr_t gcopages_large = 0x2F0;
        inline uintptr_t gcpause = 0x38;
        inline uintptr_t gcstate = 0x59;
        inline uintptr_t gcstepmul = 0x3C;
        inline uintptr_t gcstepsize = 0x40;
        inline uintptr_t gray = 0x10;
        inline uintptr_t grayagain = 0x18;
        inline uintptr_t page_next_all = 0x8;
        inline uintptr_t page_next_free = 0x18;
        inline uintptr_t strt_hash = 0x0;
        inline uintptr_t strt_size = 0xC;
        inline uintptr_t totalbytes = 0x50;
        inline uintptr_t weak = 0x20;
    }

    namespace LuauObject {
        inline uintptr_t marked = 0x2;
        inline uintptr_t page_block = 0x24;
        inline uintptr_t page_data = 0x40;
        inline uintptr_t page_next = 0x8;
        inline uintptr_t page_size = 0x20;
        inline uintptr_t table_array = 0x28;
        inline uintptr_t table_gclist = 0x20;
        inline uintptr_t table_lsz = 0x7;
        inline uintptr_t table_node = 0x18;
        inline uintptr_t table_sizearray = 0x8;
        inline uintptr_t tt = 0xC;
    }

    namespace MaterialColors {
        inline uintptr_t Asphalt = 0x30;
        inline uintptr_t Basalt = 0x27;
        inline uintptr_t Brick = 0xF;
        inline uintptr_t Cobblestone = 0x33;
        inline uintptr_t Concrete = 0xC;
        inline uintptr_t CrackedLava = 0x2D;
        inline uintptr_t Glacier = 0x1B;
        inline uintptr_t Grass = 0x6;
        inline uintptr_t Ground = 0x2A;
        inline uintptr_t Ice = 0x36;
        inline uintptr_t LeafyGrass = 0x39;
        inline uintptr_t Limestone = 0x3F;
        inline uintptr_t Mud = 0x24;
        inline uintptr_t Pavement = 0x42;
        inline uintptr_t Rock = 0x18;
        inline uintptr_t Salt = 0x3C;
        inline uintptr_t Sand = 0x12;
        inline uintptr_t Sandstone = 0x21;
        inline uintptr_t Slate = 0x9;
        inline uintptr_t Snow = 0x1E;
        inline uintptr_t WoodPlanks = 0x15;
    }

    namespace MaterialLayer {
        inline uintptr_t ColorData = 0x24;
        inline uintptr_t FillModeByte = 0x11;
        inline uintptr_t Flags2 = 0x20;
        inline uintptr_t MatFlags = 0x18;
        inline uintptr_t Param = 0x1C;
        inline uintptr_t Stride = 0x88;
    }

    namespace MemEnforcedLRUCache {
        inline uintptr_t Head = 0x8;
    }

    namespace MeshContentProvider {
        inline uintptr_t AssetID = 0x10;
        inline uintptr_t Cache = 0xD8;
        inline uintptr_t LRUCache = 0x20;
        inline uintptr_t LruHolder = 0xD8;
        inline uintptr_t MeshData = 0x40;
        inline uintptr_t ToMeshData = 0x40;
    }

    namespace MeshData {
        inline uintptr_t FaceEnd = 0x38;
        inline uintptr_t FaceStart = 0x30;
        inline uintptr_t VertexEnd = 0x8;
        inline uintptr_t VertexStart = 0x0;
    }

    namespace MeshPart {
        inline uintptr_t MeshId = 0x310;
        inline uintptr_t Texture = 0x338;
        inline uintptr_t TextureId = 0x340;
    }

    namespace Misc {
        inline uintptr_t StringLength = 0x10;
        inline uintptr_t Value = 0xB8;
        inline uintptr_t Adornee = 0xF0;
    }

    namespace Model {
        inline uintptr_t PrimaryPart = 0x258;
        inline uintptr_t Scale = 0x144;
    }

    namespace ModuleScript {
        inline uintptr_t ByteCode = 0x138;
        inline uintptr_t Bytecode = 0x138;
        inline uintptr_t Hash = 0x148;
        inline uintptr_t GUID = 0xD0;
        inline uintptr_t IsCoreScript = 0x168;
        inline uintptr_t IsRobloxScript = 0x168;
    }

    namespace MouseService {
        inline uintptr_t InputObject = 0xF0;
        inline uintptr_t InputObject2 = 0x100;
        inline uintptr_t MousePosition = 0xD4;
        inline uintptr_t SensitivityPointer = 0x0;
    }

    namespace ParticleEmitter {
        inline uintptr_t Brightness = 0x21C;
        inline uintptr_t Drag = 0x220;
        inline uintptr_t LightEmission = 0x238;
        inline uintptr_t LightInfluence = 0x23C;
        inline uintptr_t Rate = 0x248;
        inline uintptr_t Texture = 0x1C0;
        inline uintptr_t TimeScale = 0x25C;
        inline uintptr_t VelocityInheritance = 0x260;
        inline uintptr_t ZOffset = 0x264;
    }

    namespace Player {
        inline uintptr_t AccountAge = 0x35C;
        inline uintptr_t Character = 0x298;
        inline uintptr_t DisplayName = 0x138;
        inline uintptr_t HealthDisplayDistance = 0x394;
        inline uintptr_t LocalPlayer = 0x130;
        inline uintptr_t LocaleId = 0x748;
        inline uintptr_t MaxZoomDistance = 0x368;
        inline uintptr_t MinZoomDistance = 0x36C;
        inline uintptr_t ModelInstance = 0x298;
        inline uintptr_t NameDisplayDistance = 0x3A4;
        inline uintptr_t Team = 0x2D8;
        inline uintptr_t TeamColor = 0x3B0;
        inline uintptr_t UserId = 0xD0;
    }

    namespace PlayerConfigurer {
        inline uintptr_t Pointer = 0x0;
    }

    namespace Players {
        inline uintptr_t LocalPlayer = 0x130;
    }

    namespace Primitive {
        inline uintptr_t AssemblyAngularVelocity = 0xEC;
        inline uintptr_t AssemblyLinearVelocity = 0xE0;
        inline uintptr_t CFrame = 0xC8;
        inline uintptr_t Flags = 0x1BE;
        inline uintptr_t Material = 0x0;
        inline uintptr_t Orientation = 0xC8;
        inline uintptr_t Part = 0x210;
        inline uintptr_t Position = 0xD4;
        inline uintptr_t PrimitiveFlags = 0x1BE;
        inline uintptr_t Rotation = 0xB0;
        inline uintptr_t Size = 0x1C4;
        inline uintptr_t Validate = 0x6;
        inline uintptr_t Owner = 0x218;
    }

    namespace PrimitiveFlags {
        inline uintptr_t Anchored = 0x2;
        inline uintptr_t CanCollide = 0x8;
        inline uintptr_t CanQuery = 0x20;
        inline uintptr_t CanTouch = 0x10;
    }

    namespace PropertyDescriptor {
        inline uintptr_t GetSetImpl = 0x90;
        inline uintptr_t TType = 0x68;
    }

    namespace ProximityPrompt {
        inline uintptr_t ActionText = 0xB0;
        inline uintptr_t Enabled = 0x136;
        inline uintptr_t HoldDuration = 0x120;
        inline uintptr_t KeyboardKeyCode = 0x124;
        inline uintptr_t MaxActivationDistance = 0x128;
        inline uintptr_t ObjectText = 0xD0;
        inline uintptr_t RequiresLineOfSight = 0x137;
    }

    namespace Reflection {
        inline uintptr_t ClassDescCreatable = 0x10;
        inline uintptr_t ClassDescFlags = 0x1BC;
        inline uintptr_t CreatorTable = 0x61CE910;
        inline uintptr_t EntryValue = 0x8;
        inline uintptr_t NameRegistry = 0x80E2C90;
        inline uintptr_t NameTable = 0x50;
        inline uintptr_t TableEmpty = 0x20;
        inline uintptr_t TableEnd = 0x8;
        inline uintptr_t TableStart = 0x0;
        inline uintptr_t TableStride = 0x10;
    }

    namespace ReflectionType {
        inline uintptr_t AdReward = 0x60;
        inline uintptr_t AnimTrackMetadata = 0x66;
        inline uintptr_t AnimTrackPlayState = 0x65;
        inline uintptr_t AnimTrackWeight = 0x67;
        inline uintptr_t AnimationContext = 0x54;
        inline uintptr_t AnimationMask = 0x4D;
        inline uintptr_t AnimationMaskModifier = 0x5B;
        inline uintptr_t AnimationPose = 0x4E;
        inline uintptr_t Array = 0x24;
        inline uintptr_t ArticulatedJoint = 0x53;
        inline uintptr_t AssetContentMap = 0x61;
        inline uintptr_t Axes = 0x16;
        inline uintptr_t BinaryString = 0x1E;
        inline uintptr_t Bool = 0x1;
        inline uintptr_t BrickColor = 0x1C;
        inline uintptr_t Buffer = 0x56;
        inline uintptr_t CSGPropertyData = 0x48;
        inline uintptr_t CatalogSearchParams = 0x46;
        inline uintptr_t CellId = 0x19;
        inline uintptr_t ClipEvaluator = 0x4F;
        inline uintptr_t CollectionHandle = 0x20;
        inline uintptr_t Color3 = 0x11;
        inline uintptr_t Color3uint8 = 0x12;
        inline uintptr_t ColorSequence = 0x2A;
        inline uintptr_t ColorSequenceKeypoint = 0x2B;
        inline uintptr_t Connection = 0x30;
        inline uintptr_t Content = 0x5C;
        inline uintptr_t ContentId = 0x31;
        inline uintptr_t CoordinateFrame = 0x10;
        inline uintptr_t DateTime = 0x40;
        inline uintptr_t DebugTable = 0x45;
        inline uintptr_t DescribedBase = 0x32;
        inline uintptr_t Dictionary = 0x25;
        inline uintptr_t DockWidgetPluginGuiInfo = 0x38;
        inline uintptr_t Double = 0x5;
        inline uintptr_t Enum = 0x21;
        inline uintptr_t EventInstance = 0x36;
        inline uintptr_t Faces = 0x15;
        inline uintptr_t FacsReplicationData = 0x5A;
        inline uintptr_t Float = 0x4;
        inline uintptr_t FloatCurveKey = 0x3C;
        inline uintptr_t Font = 0x4A;
        inline uintptr_t Function = 0x29;
        inline uintptr_t GenericFunction = 0x28;
        inline uintptr_t GuidData = 0x1A;
        inline uintptr_t Instance = 0x8;
        inline uintptr_t InstanceRef = 0x51;
        inline uintptr_t Instances = 0x9;
        inline uintptr_t Int = 0x2;
        inline uintptr_t Int64 = 0x3;
        inline uintptr_t Integer = 0x57;
        inline uintptr_t LazyTable = 0x44;
        inline uintptr_t Map = 0x26;
        inline uintptr_t NetAssetHandle = 0x5D;
        inline uintptr_t NetAssetRef = 0x5E;
        inline uintptr_t Null = 0x0;
        inline uintptr_t NumberRange = 0x2C;
        inline uintptr_t NumberSequence = 0x2D;
        inline uintptr_t NumberSequenceKeypoint = 0x2E;
        inline uintptr_t Object = 0x5F;
        inline uintptr_t OpenCloudModel = 0x50;
        inline uintptr_t OptionalCoordinateFrame = 0x47;
        inline uintptr_t OverlapParams = 0x43;
        inline uintptr_t Path2DControlPoint = 0x58;
        inline uintptr_t PathWaypoint = 0x3B;
        inline uintptr_t PhysicalProperties = 0x1B;
        inline uintptr_t PluginDrag = 0x39;
        inline uintptr_t Property = 0x22;
        inline uintptr_t ProtectedString = 0x7;
        inline uintptr_t Random = 0x3A;
        inline uintptr_t Ray = 0xA;
        inline uintptr_t RaycastParams = 0x41;
        inline uintptr_t RaycastResult = 0x42;
        inline uintptr_t Rect2D = 0xF;
        inline uintptr_t RefType = 0x33;
        inline uintptr_t Region3 = 0x17;
        inline uintptr_t Region3int16 = 0x18;
        inline uintptr_t ReplicationPV = 0x59;
        inline uintptr_t RotationCurveKey = 0x3D;
        inline uintptr_t ScopedInstanceIdentity = 0x68;
        inline uintptr_t Secret = 0x55;
        inline uintptr_t SecurityCapabilities = 0x52;
        inline uintptr_t SharedString = 0x3F;
        inline uintptr_t SharedTable = 0x4B;
        inline uintptr_t SharedTableIterator = 0x4C;
        inline uintptr_t SlimReplicationData = 0x62;
        inline uintptr_t String = 0x6;
        inline uintptr_t Surface = 0x1F;
        inline uintptr_t SystemAddress = 0x1D;
        inline uintptr_t Tuple = 0x23;
        inline uintptr_t TweenInfo = 0x37;
        inline uintptr_t UDim = 0x13;
        inline uintptr_t UDim2 = 0x14;
        inline uintptr_t UniqueId = 0x49;
        inline uintptr_t User = 0x63;
        inline uintptr_t ValueCurveKey = 0x3E;
        inline uintptr_t Variant = 0x27;
        inline uintptr_t Vector2 = 0xB;
        inline uintptr_t Vector2int16 = 0xD;
        inline uintptr_t Vector3 = 0xC;
        inline uintptr_t Vector3int16 = 0xE;
        inline uintptr_t WebViewParams = 0x64;
    }

    namespace RenderJob {
        inline uintptr_t FakeDataModel = 0x38;
        inline uintptr_t FrameDt = 0xC0;
        inline uintptr_t FrameDtAlt = 0xB8;
        inline uintptr_t RealDataModel = 0x1C8;
        inline uintptr_t RenderView = 0x1D0;
    }

    namespace RenderQueue {
        inline uintptr_t AlwaysOnTop = 0xD;
        inline uintptr_t AlwaysOnTopAdorns = 0xE;
        inline uintptr_t Decals = 0x2;
        inline uintptr_t Glass = 0x8;
        inline uintptr_t GlassTint = 0x7;
        inline uintptr_t OnTopReadOnlyDepth = 0xC;
        inline uintptr_t OnTopWithDepth = 0xB;
        inline uintptr_t Opaque = 0x0;
        inline uintptr_t OpaqueAdorns = 0x4;
        inline uintptr_t OpaqueCasters = 0x3;
        inline uintptr_t OpaqueWithAlpha = 0x5;
        inline uintptr_t Screen = 0xF;
        inline uintptr_t ScreenOnTopOfBlur = 0x10;
        inline uintptr_t Terrain = 0x1;
        inline uintptr_t Transparent = 0x9;
        inline uintptr_t TransparentCasters = 0xA;
        inline uintptr_t Water = 0x6;
    }

    namespace RenderView {
        inline uintptr_t DeviceD3D11 = 0x8;
        inline uintptr_t LightingValid = 0x278;
        inline uintptr_t SkyValid = 0x28D;
        inline uintptr_t SkyboxValid = 0x28D;
        inline uintptr_t VisualEngine = 0x10;
    }

    namespace RobloxString {
        inline uintptr_t Size = 0x10;
        inline uintptr_t SsoCapacity = 0xF;
    }

    namespace RunService {
        inline uintptr_t HeartbeatFPS = 0xC0;
        inline uintptr_t HeartbeatTask = 0xE0;
    }

    namespace Script {
        inline uintptr_t ByteCode = 0x190;
    }

    namespace ScriptContext {
        inline uintptr_t LuaState = 0x28;
        inline uintptr_t LuaState2 = 0x28;
        inline uintptr_t LuaStateAlt = 0xE8;
        inline uintptr_t RequireBypass = 0xBB4;
        inline uintptr_t VmEncryptedLuaState = 0xD0;
        inline uintptr_t VmWrapper = 0x220;
        inline uintptr_t VmWrapper2 = 0x528;
        inline uintptr_t VmWrapperBig = 0x440;
    }

    namespace Seat {
        inline uintptr_t Occupant = 0x218;
    }

    namespace Sky {
        inline uintptr_t MoonAngularSize = 0x244;
        inline uintptr_t MoonTextureId = 0xC8;
        inline uintptr_t SkyboxBk = 0xF8;
        inline uintptr_t SkyboxDn = 0x128;
        inline uintptr_t SkyboxFt = 0x158;
        inline uintptr_t SkyboxLf = 0x188;
        inline uintptr_t SkyboxOrientation = 0x238;
        inline uintptr_t SkyboxRt = 0x1B8;
        inline uintptr_t SkyboxUp = 0x1E8;
        inline uintptr_t StarCount = 0x248;
        inline uintptr_t SunAngularSize = 0x24C;
        inline uintptr_t SunTextureId = 0x218;
    }

    namespace Sound {
        inline uintptr_t IsPlaying = 0x140;
        inline uintptr_t Looped = 0x13D;
        inline uintptr_t PlaybackSpeed = 0x11C;
        inline uintptr_t RollOffMaxDistance = 0x120;
        inline uintptr_t RollOffMinDistance = 0x124;
        inline uintptr_t SoundId = 0xC8;
        inline uintptr_t Volume = 0x130;
    }

    namespace SpawnLocation {
        inline uintptr_t AllowTeamChangeOnTouch = 0x188;
        inline uintptr_t ForcefieldDuration = 0x180;
    }

    namespace SpecialMesh {
        inline uintptr_t MeshId = 0xF8;
        inline uintptr_t Offset = 0xB8;
        inline uintptr_t Scale = 0xC4;
        inline uintptr_t TextureId = 0x128;
    }

    namespace SunRaysEffect {
        inline uintptr_t Enabled = 0x30;
        inline uintptr_t Intensity = 0xB8;
        inline uintptr_t Spread = 0xBC;
    }

    namespace SurfaceAppearance {
        inline uintptr_t ColorMap = 0xC8;
        inline uintptr_t EmissiveStrength = 0x294;
        inline uintptr_t MetalnessMap = 0x128;
        inline uintptr_t NormalMap = 0x158;
        inline uintptr_t RoughnessMap = 0x188;
    }

    namespace TaskScheduler {
        inline uintptr_t JobEnd = 0xD0;
        inline uintptr_t JobName = 0x18;
        inline uintptr_t JobStart = 0xC8;
        inline uintptr_t MaxFPS = 0xB0;
        inline uintptr_t MaxFps = 0xB0;
        inline uintptr_t Pointer = 0x8BDD8E8;
    }

    namespace Team {
        inline uintptr_t BrickColor = 0xB8;
        inline uintptr_t TeamColor = 0xB8;
    }

    namespace TechniqueArray {
        inline uintptr_t BeginOffset = 0x0;
        inline uintptr_t EndOffset = 0x8;
    }

    namespace Terrain {
        inline uintptr_t GrassLength = 0x1F0;
        inline uintptr_t MaterialColors = 0x4B8;
        inline uintptr_t WaterColor = 0x1E0;
        inline uintptr_t WaterReflectance = 0x1F8;
        inline uintptr_t WaterTransparency = 0x1FC;
        inline uintptr_t WaterWaveSize = 0x200;
        inline uintptr_t WaterWaveSpeed = 0x204;
    }

    namespace TextButton {
        inline uintptr_t AutoButtonColor = 0x9DC;
        inline uintptr_t ContentText = 0xE18;
        inline uintptr_t Font = 0x6;
        inline uintptr_t LineHeight = 0xF30;
        inline uintptr_t LocalizedText = 0xE18;
        inline uintptr_t MaxVisibleGraphemes = 0x114C;
        inline uintptr_t Modal = 0x9DD;
        inline uintptr_t RichText = 0x102E;
        inline uintptr_t Selected = 0x9DE;
        inline uintptr_t Text = 0xE18;
        inline uintptr_t TextColor3 = 0x1130;
        inline uintptr_t TextDirection = 0xFD0;
        inline uintptr_t TextScaled = 0xE01;
        inline uintptr_t TextSize = 0x1154;
        inline uintptr_t TextStrokeColor3 = 0x113C;
        inline uintptr_t TextStrokeTransparency = 0x1158;
        inline uintptr_t TextTransparency = 0x115C;
        inline uintptr_t TextTruncate = 0x1160;
        inline uintptr_t TextWrapped = 0x1028;
        inline uintptr_t TextXAlignment = 0x1164;
        inline uintptr_t TextYAlignment = 0xF78;
    }

    namespace TextLabel {
        inline uintptr_t ContentText = 0xB98;
        inline uintptr_t Font = 0x6;
        inline uintptr_t LineHeight = 0xCB0;
        inline uintptr_t LocalizedText = 0xB98;
        inline uintptr_t MaxVisibleGraphemes = 0xECC;
        inline uintptr_t RichText = 0xDAE;
        inline uintptr_t Text = 0xB98;
        inline uintptr_t TextColor3 = 0xEB0;
        inline uintptr_t TextDirection = 0xD50;
        inline uintptr_t TextScaled = 0xB81;
        inline uintptr_t TextSize = 0xED4;
        inline uintptr_t TextStrokeColor3 = 0xEBC;
        inline uintptr_t TextStrokeTransparency = 0xED8;
        inline uintptr_t TextTransparency = 0xEDC;
        inline uintptr_t TextTruncate = 0xEE0;
        inline uintptr_t TextWrapped = 0xDA8;
        inline uintptr_t TextXAlignment = 0xEE4;
        inline uintptr_t TextYAlignment = 0xCF8;
    }

    namespace Textures {
        inline uintptr_t Decal_Texture = 0x1E0;
        inline uintptr_t Texture_Texture = 0x1E0;
    }

    namespace Tool {
        inline uintptr_t CanBeDropped = 0x4B8;
        inline uintptr_t Enabled = 0x4B9;
        inline uintptr_t Grip = 0x488;
        inline uintptr_t GripForward = 0x4A0;
        inline uintptr_t GripPos = 0x4AC;
        inline uintptr_t GripRight = 0x488;
        inline uintptr_t GripUp = 0x494;
        inline uintptr_t ManualActivationOnly = 0x4BA;
        inline uintptr_t RequiresHandle = 0x4BB;
        inline uintptr_t Tooltip = 0x468;
    }

    namespace Types {
        inline uintptr_t AllTypes = 0x89482B8;
    }

    namespace UserInputService {
        inline uintptr_t WindowInputState = 0x2C0;
    }

    namespace Value {
        inline uintptr_t Value = 0xB8;
    }

    namespace VehicleSeat {
        inline uintptr_t MaxSpeed = 0x228;
        inline uintptr_t Occupant = 0x208;
        inline uintptr_t SteerFloat = 0x22C;
        inline uintptr_t ThrottleFloat = 0x230;
        inline uintptr_t Torque = 0x234;
        inline uintptr_t TurnSpeed = 0x238;
    }

    namespace VisualEngine {
        inline uintptr_t Dimensions = 0xB10;
        inline uintptr_t FakeDataModel = 0xAF0;
        inline uintptr_t Pointer = 0x846F768;
        inline uintptr_t RenderView = 0xC30;
        inline uintptr_t ViewMatrix = 0x1B0;
    }

    namespace Weld {
        inline uintptr_t Part0 = 0x118;
        inline uintptr_t Part1 = 0x128;
    }

    namespace WeldConstraint {
        inline uintptr_t Part0 = 0xB8;
        inline uintptr_t Part1 = 0xC8;
    }

    namespace WindowInputState {
        inline uintptr_t CapsLock = 0x40;
        inline uintptr_t CurrentTextBox = 0x48;
    }

    namespace Workspace {
        inline uintptr_t CurrentCamera = 0x4B8;
        inline uintptr_t ReadOnlyGravity = 0xA00;
        inline uintptr_t World = 0x410;
    }

    namespace World {
        inline uintptr_t AirProperties = 0x240;
        inline uintptr_t FallenPartsDestroyHeight = 0x208;
        inline uintptr_t Gravity = 0x228;
        inline uintptr_t Primitives = 0x2B0;
        inline uintptr_t WorldSteps = 0x728;
        inline uintptr_t worldStepsPerSec = 0x708;
    }

    namespace WorldRoot {
        inline uintptr_t RaycastBoundDesc = 0x80FA4C0;
        inline uintptr_t RaycastBoundFn = 0x80;
    }

}
