/*  Roblox Version  : version-9ac8595712b447a7
/*  Updated At      : 22/10/2026
/*  Source          : theo's offsets dump (merged with legacy entries)
*/

#pragma once
#include <cstdint>
#include <string>

namespace Offsets {
    inline std::string ClientVersion = "version-9ac8595712b447a7";

    namespace Adornment {
        inline uintptr_t Adornee       = 0xb8;
        inline uintptr_t Effect        = 0xf8;
        inline uintptr_t FillColor     = 0xd0;
        inline uintptr_t LineThickness = 0xf0;
        inline uintptr_t ModelModifier = 0x100;
        inline uintptr_t OutlineColor  = 0xdc;
        inline uintptr_t Prop          = 0xb0;
        inline uintptr_t ReservedId    = 0xf4;
    }

    namespace AirProperties {
        inline uintptr_t AirDensity = 0x18;
        inline uintptr_t GlobalWind = 0x3c;
    }

    namespace Alloc {
        inline uintptr_t Malloc = 0x17e3c30; // not updated
    }

    namespace Animator {
        inline uintptr_t ActiveAnimations = 0x900;
    }

    namespace Atmosphere {
        inline uintptr_t Color   = 0xa0;
        inline uintptr_t Decay   = 0xac;
        inline uintptr_t Density = 0xb8;
        inline uintptr_t Glare   = 0xbc;
        inline uintptr_t Haze    = 0xc0;
        inline uintptr_t Offset  = 0xc4;
    }

    namespace Attachment {
        inline uintptr_t Position = 0xac;
    }

    namespace Attribute {
        inline uintptr_t Key          = 0x0;
        inline uintptr_t Size         = 0x58;
        inline uintptr_t TypeIdRva    = 0x87948e4;
        inline uintptr_t TypeIdRvaNew = 0x87949d4;
        inline uintptr_t Value        = 0x8;
    }

    namespace AttributesMap {
        inline uintptr_t Attributes = 0x10;
        inline uintptr_t Length     = 0x0;
    }

    namespace BasePart {
        inline uintptr_t CastShadow   = 0x120;
        inline uintptr_t Color3       = 0x12;
        inline uintptr_t Locked       = 0x121;
        inline uintptr_t Massless     = 0x122;
        inline uintptr_t Primitive    = 0x178;
        inline uintptr_t Reflectance  = 0xf4;
        inline uintptr_t Shape        = 0x1a0;
        inline uintptr_t Transparency = 0x118;
    }

    namespace Beam {
        inline uintptr_t Attachment0    = 0x148;
        inline uintptr_t Attachment1    = 0x158;
        inline uintptr_t Brightness     = 0x168;
        inline uintptr_t CurveSize0     = 0x16c;
        inline uintptr_t CurveSize1     = 0x170;
        inline uintptr_t LightEmission  = 0x174;
        inline uintptr_t LightInfluence = 0x178;
        inline uintptr_t TextureLength  = 0x184;
        inline uintptr_t TextureSpeed   = 0x18c;
        inline uintptr_t Width0         = 0x190;
        inline uintptr_t Width1         = 0x194;
        inline uintptr_t ZOffset        = 0x198;
        inline uintptr_t Texture        = 0x128;
    }

    namespace BloomEffect {
        inline uintptr_t Enabled   = 0x98;
        inline uintptr_t Intensity = 0xa0;
        inline uintptr_t Size      = 0xa4;
        inline uintptr_t Threshold = 0xa8;
    }

    namespace BlurEffect {
        inline uintptr_t Enabled = 0x98;
        inline uintptr_t Size    = 0xa0;
    }

    namespace ByteCode {
        inline uintptr_t Pointer = 0x10;
        inline uintptr_t Size    = 0x28;
    }

    namespace CachedItem {
        inline uintptr_t FileMeshData = 0x40;
    }

    namespace Camera {
        inline uintptr_t CFrame          = 0xc8;
        inline uintptr_t CameraSubject   = 0xb0;
        inline uintptr_t CameraType      = 0x120;
        inline uintptr_t FieldOfView     = 0x128;
        inline uintptr_t Position        = 0xe4; // Camera position (236 dec)
        inline uintptr_t Rotation        = 0xc0;
        inline uintptr_t Viewport        = 0x284;
        inline uintptr_t ViewportInt16   = 0x28c;
        inline uintptr_t ViewportSize    = 0x2c4;
        inline uintptr_t ImagePlaneDepth = 0x2cc;
    }

    namespace CharacterMesh {
        inline uintptr_t BaseTextureId    = 0xb0;
        inline uintptr_t BodyPart         = 0x130;
        inline uintptr_t MeshId           = 0xe0;
        inline uintptr_t OverlayTextureId = 0x110;
    }

    namespace Chat {
        inline uintptr_t IsFocused = 0x154;
    }

    namespace ClassDescriptor {
        inline uintptr_t ClassName           = 0x8;
        inline uintptr_t Creator             = 0x230;
        inline uintptr_t EventDescriptors    = 0x88;
        inline uintptr_t FunctionDescriptors = 0xd0;
        inline uintptr_t PropertyDescriptors = 0x40;
    }

    namespace ClickDetector {
        inline uintptr_t MaxActivationDistance = 0xd0;
        inline uintptr_t MouseIcon             = 0xb8;
    }

    namespace Clothing {
        inline uintptr_t Color3   = 0x108;
        inline uintptr_t Template = 0xe8;
    }

    namespace ColorCorrectionEffect {
        inline uintptr_t Brightness = 0xac;
        inline uintptr_t Contrast   = 0xb0;
        inline uintptr_t Enabled    = 0x98;
        inline uintptr_t TintColor  = 0xa0;
    }

    namespace ColorGradingEffect {
        inline uintptr_t Enabled          = 0x98;
        inline uintptr_t TonemapperPreset = 0xa0;
    }

    namespace Creator {
        inline uintptr_t MapEnd   = 0x844f058;
        inline uintptr_t MapStart = 0x844f050;
    }

    namespace DataModel {
        inline uintptr_t CreatorId      = 0x170;
        inline uintptr_t GameId         = 0x178;
        inline uintptr_t GameLoaded     = 0x5c8;
        inline uintptr_t JobId          = 0x108;
        inline uintptr_t PlaceId        = 0x180;
        inline uintptr_t ServerIP       = 0x5b0;
        inline uintptr_t ToRenderView1  = 0x1b8;
        inline uintptr_t ToRenderView2  = 0x8;
        inline uintptr_t ToRenderView3  = 0x28;
        inline uintptr_t Workspace      = 0x148;
        inline uintptr_t PlaceVersion   = 0x19c;
        inline uintptr_t PrimitiveCount = 0x410;
        inline uintptr_t ScriptContext  = 0x440;
    }

    namespace DepthOfFieldEffect {
        inline uintptr_t Enabled       = 0x98;
        inline uintptr_t FarIntensity  = 0xa0;
        inline uintptr_t FocusDistance = 0xa4;
        inline uintptr_t InFocusRadius = 0xa8;
        inline uintptr_t NearIntensity = 0xac;
    }

    namespace Descriptor {
        inline uintptr_t Name = 0x8;
    }

    namespace DragDetector {
        inline uintptr_t MaxActivationDistance = 0xd0;
        inline uintptr_t MaxDragAngle          = 0x290;
        inline uintptr_t MaxForce              = 0x294;
        inline uintptr_t MaxTorque             = 0x298;
        inline uintptr_t MinDragAngle          = 0x29c;
        inline uintptr_t Responsiveness        = 0x2a8;
        inline uintptr_t ActivatedCursorIcon   = 0x1b0;
        inline uintptr_t CursorIcon            = 0xb8;
        inline uintptr_t MaxDragTranslation    = 0x25c;
        inline uintptr_t MinDragTranslation    = 0x268;
        inline uintptr_t ReferenceInstance     = 0x1e0;
    }

    namespace FakeDataModel {
        inline uintptr_t Pointer       = 0x8f59810;
        inline uintptr_t RealDataModel = 0x1f8;
    }

    namespace FastClusterEntity {
        inline uintptr_t AlphaByte              = 0x14;
        inline uintptr_t BBoxMaxX               = 0xa4;
        inline uintptr_t BBoxMaxY               = 0xa8;
        inline uintptr_t BBoxMaxZ               = 0xac;
        inline uintptr_t BBoxMinX               = 0x98;
        inline uintptr_t BBoxMinY               = 0x9c;
        inline uintptr_t BBoxMinZ               = 0xa0;
        inline uintptr_t ContextPtr             = 0x8;
        inline uintptr_t DecalMaterialPtr       = 0x48;
        inline uintptr_t MaterialPtr            = 0x20;
        inline uintptr_t PrimitiveIndexArrayPtr = 0x80;
        inline uintptr_t RenderQueueId          = 0x10;
        inline uintptr_t TechniqueArrayPtr      = 0x70;
        inline uintptr_t VTableRva              = 0x6b3ec18;
    }

    namespace FastClusterEntity::Context {
        inline uintptr_t PrimitivePoolPtr = 0x1a0;
    }

    namespace FastClusterEntity::PrimitivePool {
        inline uintptr_t ArrayBase = 0x20;
    }

    namespace FastClusterEntity::PrimitiveRecord {
        inline uintptr_t Stride      = 0x30;
        inline uintptr_t Translation = 0x24;
    }

    namespace FileMeshData {
        inline uintptr_t AabbMax     = 0x18c;
        inline uintptr_t AabbMin     = 0x180;
        inline uintptr_t Faces       = 0x30;
        inline uintptr_t FacesEnd    = 0x38;
        inline uintptr_t Vertices    = 0x0;
        inline uintptr_t VerticesEnd = 0x8;
        inline uintptr_t AABBMax     = 0x18c;
        inline uintptr_t AABBMin     = 0x180;
    }

    namespace Fire {
        inline uintptr_t FireProximityPrompt = 0x3102650;
    }

    namespace FunctionDescriptor {
        inline uintptr_t Function = 0x80;
    }

    namespace Functions {
        inline uintptr_t Clone                       = 0x1642ce0;
        inline uintptr_t Destroy                     = 0x1642d00;
        inline uintptr_t FindPartOnRay               = 0xece7a0;
        inline uintptr_t FindPartOnRayWithIgnoreList = 0xece820;
        inline uintptr_t FindPartOnRayWithWhitelist  = 0xece8b0;
        inline uintptr_t FireServer                  = 0xcac950;
        inline uintptr_t Print                       = 0x1cab4b0;
        inline uintptr_t RaisePropertyChanged        = 0xf7c720;
        inline uintptr_t Raycast                     = 0xec5d60;
        inline uintptr_t SetParent                   = 0xec0090;
        inline uintptr_t SetParentInternal           = 0x1cf3a00;
        inline uintptr_t SetParent_User              = 0x1cd37f0;
        inline uintptr_t Shapecast                   = 0xec7720;
    }

    namespace GuiBase2D {
        inline uintptr_t AbsolutePosition = 0xf4;
        inline uintptr_t AbsoluteRotation = 0xd0;
        inline uintptr_t AbsoluteSize     = 0x0;
    }

    namespace GuiObject {
        inline uintptr_t Active                 = 0x5a8;
        inline uintptr_t AnchorPoint            = 0x54c;
        inline uintptr_t AutomaticSize          = 0x560;
        inline uintptr_t BackgroundColor3       = 0x528;
        inline uintptr_t BackgroundTransparency = 0x534;
        inline uintptr_t BorderColor3           = 0x534;
        inline uintptr_t BorderMode             = 0x568;
        inline uintptr_t BorderSizePixel        = 0x56c;
        inline uintptr_t ClipsDescendants       = 0x5a9;
        inline uintptr_t GuiState               = 0x578;
        inline uintptr_t Interactable           = 0x5ab;
        inline uintptr_t LayoutOrder            = 0x564;
        inline uintptr_t Position               = 0x4f8;
        inline uintptr_t Rotation               = 0xd0;
        inline uintptr_t Selectable             = 0x5ac;
        inline uintptr_t SelectionOrder         = 0x598;
        inline uintptr_t Size                   = 0x518;
        inline uintptr_t SizeConstraint         = 0x5a0;
        inline uintptr_t Visible                = 0x595;
        inline uintptr_t ZIndex                 = 0x58c;
        inline uintptr_t Image                  = 0x9a8;
        inline uintptr_t RichText               = 0xba0;
        inline uintptr_t ScreenGui_Enabled      = 0x4b4;
        inline uintptr_t Text                   = 0xe08;
        inline uintptr_t TextColor3             = 0xeb8;
    }

    namespace Highlight {
        inline uintptr_t Adornee             = 0xb8;
        inline uintptr_t DepthMode           = 0xe0;
        inline uintptr_t Effect              = 0xf8;
        inline uintptr_t Enabled             = 0xf4;
        inline uintptr_t FillColor           = 0xc8;
        inline uintptr_t FillColor_User      = 0xd0;
        inline uintptr_t FillTransparency    = 0xe4;
        inline uintptr_t LineThickness       = 0xf0;
        inline uintptr_t ModelModifier       = 0x100;
        inline uintptr_t OutlineColor        = 0xd4;
        inline uintptr_t OutlineColor_User   = 0xdc;
        inline uintptr_t OutlineTransparency = 0xec;
        inline uintptr_t Prop                = 0xb0;
        inline uintptr_t ReservedId          = 0xf4;
    }

    namespace HopperBin {
        inline uintptr_t BinType = 0x468;
    }

    namespace Humanoid {
        inline uintptr_t AutoJumpEnabled         = 0x1c8;
        inline uintptr_t AutoRotate              = 0x1c9;
        inline uintptr_t AutomaticScalingEnabled = 0x1ca;
        inline uintptr_t BreakJointsOnDeath      = 0x1cb;
        inline uintptr_t CameraOffset            = 0x110;
        inline uintptr_t DisplayDistanceType     = 0x174;
        inline uintptr_t DisplayName             = 0xa0;
        inline uintptr_t EvaluateStateMachine    = 0x1cc;
        inline uintptr_t Health                  = 0x184;
        inline uintptr_t HealthDisplayDistance   = 0x17c;
        inline uintptr_t HealthDisplayType       = 0x180;
        inline uintptr_t HipHeight               = 0x188;
        inline uintptr_t HumanoidRootPart        = 0x460;
        inline uintptr_t HumanoidState           = 0x8a8;
        inline uintptr_t HumanoidStateID         = 0x20;
        inline uintptr_t IsWalking               = 0xa27;
        inline uintptr_t Jump                    = 0x1cf;
        inline uintptr_t JumpHeight              = 0x194;
        inline uintptr_t JumpPower               = 0x198;
        inline uintptr_t MaxHealth               = 0x19c;
        inline uintptr_t MaxSlopeAngle           = 0x1a0;
        inline uintptr_t MoveDirection           = 0x134;
        inline uintptr_t MoveToPart              = 0x100;
        inline uintptr_t MoveToPoint             = 0x158;
        inline uintptr_t NameDisplayDistance     = 0x1a4;
        inline uintptr_t NameOcclusion           = 0x1a8;
        inline uintptr_t PlatformStand           = 0x1d1;
        inline uintptr_t PlatformStatePointer    = 0xa739b13a;
        inline uintptr_t RequiresNeck            = 0x1d2;
        inline uintptr_t RigType                 = 0x1b4;
        inline uintptr_t SeatPart                = 0xf0;
        inline uintptr_t Sit                     = 0x1d2;
        inline uintptr_t TargetPoint             = 0x140;
        inline uintptr_t UseJumpPower            = 0x1d5;
        inline uintptr_t WalkSpeed               = 0x1c0;
        inline uintptr_t WalkSpeedCheck          = 0x39c;
        inline uintptr_t WalkToPoint             = 0x154;
        inline uintptr_t Walkspeed               = 0x1c4;
        inline uintptr_t WalkspeedCheck          = 0x39c;
        inline uintptr_t FloorMaterial           = 0x180;
        inline uintptr_t WalkTimer               = 0x3f0;
    }

    namespace ICreator {
        inline uintptr_t Create = 0x0;
    }

    namespace InputObject {
        inline uintptr_t MousePosition = 0xd4;
    }

    namespace Instance {
        inline uintptr_t AttributeContainer  = 0x40;
        inline uintptr_t AttributeList       = 0x10;
        inline uintptr_t AttributeToNext     = 0x58;
        inline uintptr_t AttributeToValue    = 0x18;
        inline uintptr_t ChildrenEnd         = 0x8;
        inline uintptr_t ChildrenStart       = 0x70; // 120
        inline uintptr_t ChildrenStride      = 0x10;
        inline uintptr_t ClassByName         = 0x46f6bfe;
        inline uintptr_t ClassDescriptor     = 0x18;
        inline uintptr_t ClassName           = 0x8;
        inline uintptr_t ComponentMap        = 0x38;
        inline uintptr_t Creator_create      = 0x0;
        inline uintptr_t Creator_isCreatable = 0x10;
        inline uintptr_t FromExisting        = 0x417c6b0;
        inline uintptr_t Name                = 0x8;
        inline uintptr_t NameContainer       = 0x68; // 112
        inline uintptr_t New                 = 0x360a4d0;
        inline uintptr_t Parent              = 0x60;
        inline uintptr_t SetParent           = 0x7e4470;
        inline uintptr_t This                = 0x8;
        inline uintptr_t WhJobNopSlot        = 0x10;
        inline uintptr_t ClassBase           = 0x1b0;
    }

    namespace Lighting {
        inline uintptr_t Ambient                  = 0xb8;
        inline uintptr_t Atmosphere               = 0x1d8;
        inline uintptr_t Brightness               = 0x100;
        inline uintptr_t ClockTime                = 0xb0;
        inline uintptr_t ColorShift_Bottom        = 0xd0;
        inline uintptr_t ColorShift_Top           = 0xc4;
        inline uintptr_t EnvironmentDiffuseScale  = 0x104;
        inline uintptr_t EnvironmentSpecularScale = 0x108;
        inline uintptr_t ExposureCompensation     = 0x10c;
        inline uintptr_t FogColor                 = 0xdc;
        inline uintptr_t FogEnd                   = 0x114;
        inline uintptr_t FogStart                 = 0x118;
        inline uintptr_t GeographicLatitude       = 0x11c;
        inline uintptr_t GradientBottom           = 0x178;
        inline uintptr_t GradientTop              = 0x138;
        inline uintptr_t LightColor               = 0x144;
        inline uintptr_t LightDirection           = 0x150;
        inline uintptr_t MoonPosition             = 0x16c;
        inline uintptr_t OutdoorAmbient           = 0xe8;
        inline uintptr_t ShadowSoftness           = 0x13c;
        inline uintptr_t Sky                      = 0x1b0;
        inline uintptr_t Source                   = 0x15c;
        inline uintptr_t SunPosition              = 0x160;
        inline uintptr_t GlobalShadows            = 0x134;
    }

    namespace LightingParameters {
        inline uintptr_t GeographicLatitude = 0x134;
        inline uintptr_t LightColor         = 0x15c;
        inline uintptr_t LightDirection     = 0x168;
        inline uintptr_t SkyAmbient         = 0x150;
        inline uintptr_t SkyAmbient2        = 0x138;
        inline uintptr_t Source             = 0x174;
        inline uintptr_t TrueMoonPosition   = 0x184;
        inline uintptr_t TrueSunPosition    = 0x178;
    }

    namespace LocalScript {
        inline uintptr_t ByteCode = 0x0;
        inline uintptr_t Bytecode = 0x190;
        inline uintptr_t Hash     = 0x190;
        inline uintptr_t GUID     = 0xc0;
    }

    namespace LruHolder {
        inline uintptr_t MemEnforcedLRUCache = 0x20;
    }

    namespace LruNode {
        inline uintptr_t CachedItem = 0x40;
        inline uintptr_t MeshId     = 0x10;
        inline uintptr_t Next       = 0x0;
            inline uintptr_t AssetID = 0x10;
    }

    namespace LuaState {
        inline uintptr_t Base    = 0x28;
        inline uintptr_t Global  = 0x20;
        inline uintptr_t Top     = 0x8;
        inline uintptr_t TypeTag = 0x0;
    }

    namespace Luau {
        inline uintptr_t lua_getglobal = 0x0;
        inline uintptr_t require_impl  = 0x0;
    }

    namespace LuauGlobal {
        inline uintptr_t GCthreshold    = 0x48;
        inline uintptr_t currentwhite   = 0x58;
        inline uintptr_t dummynode      = 0x6cf3af8;
        inline uintptr_t gcopages       = 0x2f0;
        inline uintptr_t gcopages_end   = 0x2f8;
        inline uintptr_t gcopages_large = 0x2f0;
        inline uintptr_t gcpause        = 0x38;
        inline uintptr_t gcstate        = 0x59;
        inline uintptr_t gcstepmul      = 0x3c;
        inline uintptr_t gcstepsize     = 0x40;
        inline uintptr_t gray           = 0x10;
        inline uintptr_t grayagain      = 0x18;
        inline uintptr_t page_next_all  = 0x8;
        inline uintptr_t page_next_free = 0x18;
        inline uintptr_t strt_hash      = 0x0;
        inline uintptr_t strt_size      = 0xc;
        inline uintptr_t totalbytes     = 0x50;
        inline uintptr_t weak           = 0x20;
    }

    namespace LuauObject {
        inline uintptr_t marked          = 0x2;
        inline uintptr_t page_block      = 0x24;
        inline uintptr_t page_data       = 0x40;
        inline uintptr_t page_next       = 0x8;
        inline uintptr_t page_size       = 0x20;
        inline uintptr_t table_array     = 0x28;
        inline uintptr_t table_gclist    = 0x20;
        inline uintptr_t table_lsz       = 0x7;
        inline uintptr_t table_node      = 0x18;
        inline uintptr_t table_sizearray = 0x8;
        inline uintptr_t tt              = 0xc;
    }

    namespace MaterialColors {
        inline uintptr_t Asphalt     = 0x30;
        inline uintptr_t Basalt      = 0x27;
        inline uintptr_t Brick       = 0xf;
        inline uintptr_t Cobblestone = 0x33;
        inline uintptr_t Concrete    = 0xc;
        inline uintptr_t CrackedLava = 0x2d;
        inline uintptr_t Glacier     = 0x1b;
        inline uintptr_t Grass       = 0x6;
        inline uintptr_t Ground      = 0x2a;
        inline uintptr_t Ice         = 0x36;
        inline uintptr_t LeafyGrass  = 0x39;
        inline uintptr_t Limestone   = 0x3f;
        inline uintptr_t Mud         = 0x24;
        inline uintptr_t Pavement    = 0x42;
        inline uintptr_t Rock        = 0x18;
        inline uintptr_t Salt        = 0x3c;
        inline uintptr_t Sand        = 0x12;
        inline uintptr_t Sandstone   = 0x21;
        inline uintptr_t Slate       = 0x9;
        inline uintptr_t Snow        = 0x1e;
        inline uintptr_t WoodPlanks  = 0x15;
    }

    namespace MaterialLayer {
        inline uintptr_t ColorData    = 0x24;
        inline uintptr_t FillModeByte = 0x11;
        inline uintptr_t Flags2       = 0x20;
        inline uintptr_t MatFlags     = 0x18;
        inline uintptr_t Param        = 0x1c;
        inline uintptr_t Stride       = 0x88;
    }

    namespace MemEnforcedLRUCache {
        inline uintptr_t Head = 0x8;
    }

    namespace MeshContentProvider {
        inline uintptr_t AssetID    = 0x10;
        inline uintptr_t Cache      = 0xd8;
        inline uintptr_t LRUCache   = 0x20;
        inline uintptr_t LruHolder  = 0xc0;
        inline uintptr_t MeshData   = 0x40;
        inline uintptr_t ToMeshData = 0x40;
        inline uintptr_t LRUHolder  = 0xc8;
    }

    namespace MeshData {
        inline uintptr_t FaceEnd     = 0x38;
        inline uintptr_t FaceStart   = 0x30;
        inline uintptr_t VertexEnd   = 0x8;
        inline uintptr_t VertexStart = 0x0;
    }

    namespace MeshPart {
        inline uintptr_t MeshId    = 0x2f8;
        inline uintptr_t Texture   = 0x328;
        inline uintptr_t TextureId = 0x330;
    }

    namespace Misc {
        inline uintptr_t StringLength = 0x10;
        inline uintptr_t Value        = 0xa0;
        inline uintptr_t Adornee      = 0xd8;
        inline uintptr_t AnimationId  = 0xa8;
    }

    namespace Model {
        inline uintptr_t PrimaryPart = 0x240;
        inline uintptr_t Scale       = 0x12c;
    }

    namespace ModuleScript {
        inline uintptr_t ByteCode       = 0x0;
        inline uintptr_t Bytecode       = 0x138;
        inline uintptr_t Hash           = 0x348;
        inline uintptr_t GUID           = 0xb8;
        inline uintptr_t IsCoreScript   = 0x0;
        inline uintptr_t IsRobloxScript = 0x168;
    }

    namespace MouseService {
        inline uintptr_t InputObject        = 0xe8;
        inline uintptr_t InputObject2       = 0xf8;
        inline uintptr_t MousePosition      = 0xbc;
        inline uintptr_t SensitivityPointer = 0x0;
    }

    namespace ParticleEmitter {
        inline uintptr_t Brightness          = 0x204;
        inline uintptr_t Drag                = 0x208;
        inline uintptr_t LightEmission       = 0x220;
        inline uintptr_t LightInfluence      = 0x224;
        inline uintptr_t Rate                = 0x230;
        inline uintptr_t Texture             = 0x1a8;
        inline uintptr_t TimeScale           = 0x244;
        inline uintptr_t VelocityInheritance = 0x248;
        inline uintptr_t ZOffset             = 0x24c;
        inline uintptr_t Acceleration        = 0x1c8;
        inline uintptr_t Lifetime            = 0x1dc;
        inline uintptr_t RotSpeed            = 0x1e4;
        inline uintptr_t Rotation            = 0x1ec;
        inline uintptr_t Speed               = 0x1f4;
        inline uintptr_t SpreadAngle         = 0x1fc;
    }

    namespace Player {
        inline uintptr_t AccountAge            = 0x354;
        inline uintptr_t Character             = 0x288;
        inline uintptr_t DisplayName           = 0x120;
        inline uintptr_t HealthDisplayDistance = 0x38c;
        inline uintptr_t LocalPlayer           = 0x118;
        inline uintptr_t LocaleId              = 0x100;
        inline uintptr_t MaxZoomDistance       = 0x360;
        inline uintptr_t MinZoomDistance       = 0x364;
        inline uintptr_t ModelInstance         = 0x290;
        inline uintptr_t NameDisplayDistance   = 0x39c;
        inline uintptr_t Team                  = 0x2d0;
        inline uintptr_t TeamColor             = 0x3a8;
        inline uintptr_t UserId                = 0xb8;
        inline uintptr_t CameraMode            = 0x370;
        inline uintptr_t Mouse                 = 0x1218;
    }

    namespace PlayerConfigurer {
        inline uintptr_t Pointer = 0x0;
    }

    namespace Players {
        inline uintptr_t LocalPlayer = 0x120;
    }

    namespace Primitive {
        inline uintptr_t AssemblyAngularVelocity = 0xec;
        inline uintptr_t AssemblyLinearVelocity  = 0xe0;
        inline uintptr_t CFrame                  = 0xc8;
        inline uintptr_t Flags                   = 0x1be;
        inline uintptr_t Material                = 0x0;
        inline uintptr_t Orientation             = 0xc8;
        inline uintptr_t Part                    = 0x210;
        inline uintptr_t Position                = 0xd4;
        inline uintptr_t PrimitiveFlags          = 0x1b6;
        inline uintptr_t Rotation                = 0xb0;
        inline uintptr_t Size                    = 0x1c4;
        inline uintptr_t Validate                = 0x6;
        inline uintptr_t Owner                   = 0x218;
    }

    namespace PrimitiveFlags {
        inline uintptr_t Anchored   = 0x2;
        inline uintptr_t CanCollide = 0x8;
        inline uintptr_t CanQuery   = 0x20;
        inline uintptr_t CanTouch   = 0x10;
    }

    namespace PropertyDescriptor {
        inline uintptr_t GetSetImpl = 0x90;
        inline uintptr_t TType      = 0x68;
    }

    namespace ProximityPrompt {
        inline uintptr_t ActionText            = 0x98;
        inline uintptr_t Enabled               = 0x11e;
        inline uintptr_t HoldDuration          = 0x108;
        inline uintptr_t KeyboardKeyCode       = 0x10c;
        inline uintptr_t MaxActivationDistance = 0x110;
        inline uintptr_t ObjectText            = 0xb8;
        inline uintptr_t RequiresLineOfSight   = 0x11f;
        inline uintptr_t GamepadKeyCode        = 0x10c;
        inline uintptr_t KeyCode               = 0x114;
    }

    namespace Reflection {
        inline uintptr_t ClassDescCreatable = 0x10;
        inline uintptr_t ClassDescFlags     = 0x1bc;
        inline uintptr_t CreatorTable       = 0x61ce910;
        inline uintptr_t EntryValue         = 0x8;
        inline uintptr_t NameRegistry       = 0x80e2c90;
        inline uintptr_t NameTable          = 0x50;
        inline uintptr_t TableEmpty         = 0x20;
        inline uintptr_t TableEnd           = 0x8;
        inline uintptr_t TableStart         = 0x0;
        inline uintptr_t TableStride        = 0x10;
    }

    namespace ReflectionType {
        inline uintptr_t AdReward                = 0x60;
        inline uintptr_t AnimTrackMetadata       = 0x66;
        inline uintptr_t AnimTrackPlayState      = 0x65;
        inline uintptr_t AnimTrackWeight         = 0x67;
        inline uintptr_t AnimationContext        = 0x54;
        inline uintptr_t AnimationMask           = 0x4d;
        inline uintptr_t AnimationMaskModifier   = 0x5b;
        inline uintptr_t AnimationPose           = 0x4e;
        inline uintptr_t Array                   = 0x24;
        inline uintptr_t ArticulatedJoint        = 0x53;
        inline uintptr_t AssetContentMap         = 0x61;
        inline uintptr_t Axes                    = 0x16;
        inline uintptr_t BinaryString            = 0x1e;
        inline uintptr_t Bool                    = 0x1;
        inline uintptr_t BrickColor              = 0x1c;
        inline uintptr_t Buffer                  = 0x56;
        inline uintptr_t CSGPropertyData         = 0x48;
        inline uintptr_t CatalogSearchParams     = 0x46;
        inline uintptr_t CellId                  = 0x19;
        inline uintptr_t ClipEvaluator           = 0x4f;
        inline uintptr_t CollectionHandle        = 0x20;
        inline uintptr_t Color3                  = 0x11;
        inline uintptr_t Color3uint8             = 0x12;
        inline uintptr_t ColorSequence           = 0x2a;
        inline uintptr_t ColorSequenceKeypoint   = 0x2b;
        inline uintptr_t Connection              = 0x30;
        inline uintptr_t Content                 = 0x5c;
        inline uintptr_t ContentId               = 0x31;
        inline uintptr_t CoordinateFrame         = 0x10;
        inline uintptr_t DateTime                = 0x40;
        inline uintptr_t DebugTable              = 0x45;
        inline uintptr_t DescribedBase           = 0x32;
        inline uintptr_t Dictionary              = 0x25;
        inline uintptr_t DockWidgetPluginGuiInfo = 0x38;
        inline uintptr_t Double                  = 0x5;
        inline uintptr_t Enum                    = 0x21;
        inline uintptr_t EventInstance           = 0x36;
        inline uintptr_t Faces                   = 0x15;
        inline uintptr_t FacsReplicationData     = 0x5a;
        inline uintptr_t Float                   = 0x4;
        inline uintptr_t FloatCurveKey           = 0x3c;
        inline uintptr_t Font                    = 0x4a;
        inline uintptr_t Function                = 0x29;
        inline uintptr_t GenericFunction         = 0x28;
        inline uintptr_t GuidData                = 0x1a;
        inline uintptr_t Instance                = 0x8;
        inline uintptr_t InstanceRef             = 0x51;
        inline uintptr_t Instances               = 0x9;
        inline uintptr_t Int                     = 0x2;
        inline uintptr_t Int64                   = 0x3;
        inline uintptr_t Integer                 = 0x57;
        inline uintptr_t LazyTable               = 0x44;
        inline uintptr_t Map                     = 0x26;
        inline uintptr_t NetAssetHandle          = 0x5d;
        inline uintptr_t NetAssetRef             = 0x5e;
        inline uintptr_t Null                    = 0x0;
        inline uintptr_t NumberRange             = 0x2c;
        inline uintptr_t NumberSequence          = 0x2d;
        inline uintptr_t NumberSequenceKeypoint  = 0x2e;
        inline uintptr_t Object                  = 0x5f;
        inline uintptr_t OpenCloudModel          = 0x50;
        inline uintptr_t OptionalCoordinateFrame = 0x47;
        inline uintptr_t OverlapParams           = 0x43;
        inline uintptr_t Path2DControlPoint      = 0x58;
        inline uintptr_t PathWaypoint            = 0x3b;
        inline uintptr_t PhysicalProperties      = 0x1b;
        inline uintptr_t PluginDrag              = 0x39;
        inline uintptr_t Property                = 0x22;
        inline uintptr_t ProtectedString         = 0x7;
        inline uintptr_t Random                  = 0x3a;
        inline uintptr_t Ray                     = 0xa;
        inline uintptr_t RaycastParams           = 0x41;
        inline uintptr_t RaycastResult           = 0x42;
        inline uintptr_t Rect2D                  = 0xf;
        inline uintptr_t RefType                 = 0x33;
        inline uintptr_t Region3                 = 0x17;
        inline uintptr_t Region3int16            = 0x18;
        inline uintptr_t ReplicationPV           = 0x59;
        inline uintptr_t RotationCurveKey        = 0x3d;
        inline uintptr_t ScopedInstanceIdentity  = 0x68;
        inline uintptr_t Secret                  = 0x55;
        inline uintptr_t SecurityCapabilities    = 0x52;
        inline uintptr_t SharedString            = 0x3f;
        inline uintptr_t SharedTable             = 0x4b;
        inline uintptr_t SharedTableIterator     = 0x4c;
        inline uintptr_t SlimReplicationData     = 0x62;
        inline uintptr_t String                  = 0x6;
        inline uintptr_t Surface                 = 0x1f;
        inline uintptr_t SystemAddress           = 0x1d;
        inline uintptr_t Tuple                   = 0x23;
        inline uintptr_t TweenInfo               = 0x37;
        inline uintptr_t UDim                    = 0x13;
        inline uintptr_t UDim2                   = 0x14;
        inline uintptr_t UniqueId                = 0x49;
        inline uintptr_t User                    = 0x63;
        inline uintptr_t ValueCurveKey           = 0x3e;
        inline uintptr_t Variant                 = 0x27;
        inline uintptr_t Vector2                 = 0xb;
        inline uintptr_t Vector2int16            = 0xd;
        inline uintptr_t Vector3                 = 0xc;
        inline uintptr_t Vector3int16            = 0xe;
        inline uintptr_t WebViewParams           = 0x64;
    }

    namespace RenderJob {
        inline uintptr_t FakeDataModel = 0x38;
        inline uintptr_t FrameDt       = 0xc0;
        inline uintptr_t FrameDtAlt    = 0xb8;
        inline uintptr_t RealDataModel = 0x1f0;
        inline uintptr_t RenderView    = 0x1e0;
    }

    namespace RenderQueue {
        inline uintptr_t AlwaysOnTop        = 0xd;
        inline uintptr_t AlwaysOnTopAdorns  = 0xe;
        inline uintptr_t Decals             = 0x2;
        inline uintptr_t Glass              = 0x8;
        inline uintptr_t GlassTint          = 0x7;
        inline uintptr_t OnTopReadOnlyDepth = 0xc;
        inline uintptr_t OnTopWithDepth     = 0xb;
        inline uintptr_t Opaque             = 0x0;
        inline uintptr_t OpaqueAdorns       = 0x4;
        inline uintptr_t OpaqueCasters      = 0x3;
        inline uintptr_t OpaqueWithAlpha    = 0x5;
        inline uintptr_t Screen             = 0xf;
        inline uintptr_t ScreenOnTopOfBlur  = 0x10;
        inline uintptr_t Terrain            = 0x1;
        inline uintptr_t Transparent        = 0x9;
        inline uintptr_t TransparentCasters = 0xa;
        inline uintptr_t Water              = 0x6;
    }

    namespace RenderView {
        inline uintptr_t DeviceD3D11   = 0x0;
        inline uintptr_t LightingValid = 0x0;
        inline uintptr_t SkyValid      = 0x0;
        inline uintptr_t SkyboxValid   = 0x28d;
        inline uintptr_t VisualEngine  = 0x0;
    }

    namespace RobloxString {
        inline uintptr_t Size        = 0x10;
        inline uintptr_t SsoCapacity = 0xf;
    }

    namespace RunService {
        inline uintptr_t HeartbeatFPS  = 0xc0;
        inline uintptr_t HeartbeatTask = 0x8a08;
    }

    namespace Script {
        inline uintptr_t ByteCode = 0x0;
        inline uintptr_t GUID     = 0xc0;
        inline uintptr_t Hash     = 0x190;
    }

    namespace ScriptContext {
        inline uintptr_t LuaState            = 0x28;
        inline uintptr_t LuaState2           = 0x28;
        inline uintptr_t LuaStateAlt         = 0xe8;
        inline uintptr_t RequireBypass       = 0x0;
        inline uintptr_t VmEncryptedLuaState = 0xd0;
        inline uintptr_t VmWrapper           = 0x220;
        inline uintptr_t VmWrapper2          = 0x528;
        inline uintptr_t VmWrapperBig        = 0x440;
    }

    namespace Seat {
        inline uintptr_t Occupant = 0x200;
    }

    namespace Sky {
        inline uintptr_t MoonAngularSize   = 0x22c;
        inline uintptr_t MoonTextureId     = 0xb0;
        inline uintptr_t SkyboxBk          = 0xe0;
        inline uintptr_t SkyboxDn          = 0x110;
        inline uintptr_t SkyboxFt          = 0x140;
        inline uintptr_t SkyboxLf          = 0x170;
        inline uintptr_t SkyboxOrientation = 0x220;
        inline uintptr_t SkyboxRt          = 0x1a0;
        inline uintptr_t SkyboxUp          = 0x1d0;
        inline uintptr_t StarCount         = 0x230;
        inline uintptr_t SunAngularSize    = 0x224;
        inline uintptr_t SunTextureId      = 0x200;
    }

    namespace Sound {
        inline uintptr_t IsPlaying          = 0x128;
        inline uintptr_t Looped             = 0x125;
        inline uintptr_t PlaybackSpeed      = 0x104;
        inline uintptr_t RollOffMaxDistance = 0x108;
        inline uintptr_t RollOffMinDistance = 0x10c;
        inline uintptr_t SoundId            = 0xb0;
        inline uintptr_t Volume             = 0x118;
        inline uintptr_t SoundGroup         = 0xd8;
    }

    namespace SpawnLocation {
        inline uintptr_t AllowTeamChangeOnTouch = 0x1d8;
        inline uintptr_t ForcefieldDuration     = 0x1d0;
        inline uintptr_t Enabled                = 0x1e1;
        inline uintptr_t Neutral                = 0x1e2;
        inline uintptr_t TeamColor              = 0x1dc;
    }

    namespace SpecialMesh {
        inline uintptr_t MeshId    = 0xe0;
        inline uintptr_t Offset    = 0xb8;
        inline uintptr_t Scale     = 0xac;
        inline uintptr_t TextureId = 0x128;
    }

    namespace SunRaysEffect {
        inline uintptr_t Enabled   = 0x98;
        inline uintptr_t Intensity = 0xa0;
        inline uintptr_t Spread    = 0xa4;
    }

    namespace SurfaceAppearance {
        inline uintptr_t ColorMap            = 0xb0;
        inline uintptr_t EmissiveStrength    = 0x1dc;
        inline uintptr_t MetalnessMap        = 0x110;
        inline uintptr_t NormalMap           = 0x140;
        inline uintptr_t RoughnessMap        = 0x170;
        inline uintptr_t AlphaMode           = 0x1d8;
        inline uintptr_t Color               = 0x1c0;
        inline uintptr_t EmissiveMaskContent = 0xe0;
        inline uintptr_t EmissiveTint        = 0x1cc;
    }

    namespace TaskScheduler {
        inline uintptr_t JobEnd   = 0xd0;
        inline uintptr_t JobName  = 0x18;
        inline uintptr_t JobStart = 0xc8;
        inline uintptr_t MaxFPS   = 0xb0;
        inline uintptr_t MaxFps   = 0xb0;
        inline uintptr_t Pointer  = 0x8f026a8;
    }

    namespace Team {
        inline uintptr_t BrickColor = 0xa0;
        inline uintptr_t TeamColor  = 0xa8;
    }

    namespace TechniqueArray {
        inline uintptr_t BeginOffset = 0x0;
        inline uintptr_t EndOffset   = 0x8;
    }

    namespace Terrain {
        inline uintptr_t GrassLength       = 0x1d8;
        inline uintptr_t MaterialColors    = 0x4a0;
        inline uintptr_t WaterColor        = 0x1c8;
        inline uintptr_t WaterReflectance  = 0x1e0;
        inline uintptr_t WaterTransparency = 0x1e4;
        inline uintptr_t WaterWaveSize     = 0x1e8;
        inline uintptr_t WaterWaveSpeed    = 0x1ec;
    }

    namespace TextButton {
        inline uintptr_t AutoButtonColor        = 0x9dc;
        inline uintptr_t ContentText            = 0xe18;
        inline uintptr_t Font                   = 0x6;
        inline uintptr_t LineHeight             = 0xf30;
        inline uintptr_t LocalizedText          = 0xe18;
        inline uintptr_t MaxVisibleGraphemes    = 0x114c;
        inline uintptr_t Modal                  = 0x9dd;
        inline uintptr_t RichText               = 0x102e;
        inline uintptr_t Selected               = 0x9de;
        inline uintptr_t Text                   = 0xe18;
        inline uintptr_t TextColor3             = 0x1130;
        inline uintptr_t TextDirection          = 0xfd0;
        inline uintptr_t TextScaled             = 0xe01;
        inline uintptr_t TextSize               = 0x1154;
        inline uintptr_t TextStrokeColor3       = 0x113c;
        inline uintptr_t TextStrokeTransparency = 0x1158;
        inline uintptr_t TextTransparency       = 0x115c;
        inline uintptr_t TextTruncate           = 0x1160;
        inline uintptr_t TextWrapped            = 0x1028;
        inline uintptr_t TextXAlignment         = 0x1164;
        inline uintptr_t TextYAlignment         = 0xf78;
    }

    namespace TextLabel {
        inline uintptr_t ContentText            = 0xb98;
        inline uintptr_t Font                   = 0x6;
        inline uintptr_t LineHeight             = 0xcb0;
        inline uintptr_t LocalizedText          = 0xb98;
        inline uintptr_t MaxVisibleGraphemes    = 0xecc;
        inline uintptr_t RichText               = 0xdae;
        inline uintptr_t Text                   = 0xb98;
        inline uintptr_t TextColor3             = 0xeb0;
        inline uintptr_t TextDirection          = 0xd50;
        inline uintptr_t TextScaled             = 0xb81;
        inline uintptr_t TextSize               = 0xed4;
        inline uintptr_t TextStrokeColor3       = 0xebc;
        inline uintptr_t TextStrokeTransparency = 0xed8;
        inline uintptr_t TextTransparency       = 0xedc;
        inline uintptr_t TextTruncate           = 0xee0;
        inline uintptr_t TextWrapped            = 0xda8;
        inline uintptr_t TextXAlignment         = 0xee4;
        inline uintptr_t TextYAlignment         = 0xcf8;
    }

    namespace Textures {
        inline uintptr_t Decal_Texture   = 0x1c8;
        inline uintptr_t Texture_Texture = 0x1c8;
    }

    namespace Tool {
        inline uintptr_t CanBeDropped         = 0x4a0;
        inline uintptr_t Enabled              = 0x4a1;
        inline uintptr_t Grip                 = 0x494;
        inline uintptr_t GripForward          = 0x490;
        inline uintptr_t GripPos              = 0x49c;
        inline uintptr_t GripRight            = 0x47c;
        inline uintptr_t GripUp               = 0x484;
        inline uintptr_t ManualActivationOnly = 0x4a2;
        inline uintptr_t RequiresHandle       = 0x4a3;
        inline uintptr_t Tooltip              = 0x450;
        inline uintptr_t TextureId            = 0x350;
    }

    namespace Types {
        inline uintptr_t AllTypes = 0x89482b8;
    }

    namespace UserInputService {
        inline uintptr_t WindowInputState = 0x2a8;
    }

    namespace Value {
        inline uintptr_t Value = 0xb8;
    }

    namespace VehicleSeat {
        inline uintptr_t MaxSpeed      = 0x210;
        inline uintptr_t Occupant      = 0x208;
        inline uintptr_t SteerFloat    = 0x214;
        inline uintptr_t ThrottleFloat = 0x218;
        inline uintptr_t Torque        = 0x21c;
        inline uintptr_t TurnSpeed     = 0x220;
    }

    namespace VisualEngine {
        inline uintptr_t Dimensions    = 0xb10;
        inline uintptr_t FakeDataModel = 0xaf0;
        inline uintptr_t Pointer       = 0x89dc1a0;
        inline uintptr_t RenderView    = 0xc40;
        inline uintptr_t ViewMatrix    = 0x1b0;
    }

    namespace Weld {
        inline uintptr_t Part0 = 0x100;
        inline uintptr_t Part1 = 0x110;
    }

    namespace WeldConstraint {
        inline uintptr_t Part0 = 0xa0;
        inline uintptr_t Part1 = 0xb0;
    }

    namespace WindowInputState {
        inline uintptr_t CapsLock       = 0x40;
        inline uintptr_t CurrentTextBox = 0x48;
    }

    namespace Workspace {
        inline uintptr_t CurrentCamera       = 0x4a0;
        inline uintptr_t DistributedGameTime = 0x4c0;
        inline uintptr_t ReadOnlyGravity     = 0x9c8;
        inline uintptr_t World               = 0x3f8;
    }

    namespace World {
        inline uintptr_t AirProperties            = 0x238;
        inline uintptr_t FallenPartsDestroyHeight = 0x214;
        inline uintptr_t Gravity                  = 0x220;
        inline uintptr_t Primitives               = 0x2a8;
        inline uintptr_t worldStepsPerSec         = 0x860;
    }

    namespace WorldRoot {
        inline uintptr_t RaycastBoundDesc = 0x80fa4c0;
        inline uintptr_t RaycastBoundFn   = 0x80;
    }

    namespace AnimationTrack {
        inline uintptr_t Animation    = 0xa8;
        inline uintptr_t Animator     = 0x100;
        inline uintptr_t IsPlaying    = 0xa48;
        inline uintptr_t Looped       = 0xd5;
        inline uintptr_t Speed        = 0xc4;
        inline uintptr_t TimePosition = 0xc8;
    }

    namespace LRUHolder {
        inline uintptr_t MemEnforcedLRUCache = 0x20;
    }

    namespace LRUNode {
        inline uintptr_t AssetID    = 0x10;
        inline uintptr_t CachedItem = 0x38;
        inline uintptr_t Next       = 0x0;
    }

    namespace PlayerMouse {
        inline uintptr_t Icon      = 0xb8;
        inline uintptr_t Workspace = 0x140;
    }

    namespace StatsItem {
        inline uintptr_t Value = 0x180;
    }

    namespace UnionOperation {
        inline uintptr_t AssetId = 0x300;
    }

}