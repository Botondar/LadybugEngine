#pragma once

#include <LadybugLib/Core.hpp>
#include <LadybugLib/Intrinsics.hpp>
#include <LadybugLib/String.hpp>
#include "Font.hpp"
#include "Platform.hpp"

//
// Config
//
constexpr u64 R_RenderTargetMemorySize  = MiB(320);
constexpr u64 R_TextureMemorySize       = MiB(1024llu);
constexpr u64 R_ShadowMapMemorySize     = MiB(256);
constexpr u32 R_MaxShadowCascadeCount   = 4;
constexpr u32 R_ShadowResolution        = 2048;
constexpr u64 R_VertexBufferMaxBlockCount = (1llu << 18);

//
// Limits
//
constexpr u32 MaxVertexBindingCount = 16;
constexpr u32 MaxVertexAttribCount = 32;
constexpr u32 MaxColorAttachmentCount = 8;
constexpr u32 MaxDescriptorSetCount = 256;
constexpr u32 MaxPushConstantRangeCount = 64;
constexpr u32 MaxDescriptorSetLayoutBindingCount = 32;


//
// Low-level rendering
//

constexpr f32 GlobalMaxLOD = 1000.0f;

enum compare_op : u32
{
    Compare_Never = 0,
    Compare_Less,
    Compare_Equal,
    Compare_LessEqual,
    Compare_Greater,
    Compare_NotEqual,
    Compare_GreaterEqual,
    Compare_Always,
};

enum tex_filter : u32
{
    Filter_Nearest = 0,
    Filter_Linear = 1,
};

enum tex_wrap : u32
{
    Wrap_Repeat = 0,
    Wrap_RepeatMirror = 1,
    Wrap_ClampToEdge = 2,
    Wrap_ClampToBorder = 3,
};

enum tex_anisotropy : u32
{
    Anisotropy_None = 0,

    Anisotropy_1,
    Anisotropy_2,
    Anisotropy_4,
    Anisotropy_8,
    Anisotropy_16,

    Anisotropy_Count,
};

enum tex_border : u32
{
    Border_Black = 0,
    Border_White = 1,
};

struct sampler_state
{
    tex_filter MagFilter;
    tex_filter MinFilter;
    tex_filter MipFilter;
    tex_wrap WrapU;
    tex_wrap WrapV;
    tex_wrap WrapW;
    tex_anisotropy Anisotropy;
    b32 EnableComparison;
    compare_op Comparison;
    f32 MinLOD;
    f32 MaxLOD;
    tex_border Border;
    b32 EnableUnnormalizedCoordinates;
};

// TODO(boti): This is binary compatible with Vulkan for now,
// but with mutable descriptor types it could be collapsed down to the D3D12 model?
enum descriptor_type : u32
{
    Descriptor_Sampler = 0,
    Descriptor_ImageSampler = 1,
    Descriptor_SampledImage = 2,
    Descriptor_StorageImage = 3,
    Descriptor_UniformTexelBuffer = 4,
    Descriptor_StorageTexelBuffer = 5,
    Descriptor_UniformBuffer = 6,
    Descriptor_StorageBuffer = 7,
    Descriptor_DynamicUniformBuffer = 8,
    Descriptor_DynamicStorageBuffer = 9,
    //  Descriptor_InputAttachment, 
    Descriptor_InlineUniformBlock = 11,
};

enum descriptor_set_layout_flags : flags32
{
    SetLayoutFlag_None = 0,

    SetLayoutFlag_UpdateAfterBind = (1 << 0),
    SetLayoutFlag_Bindless = (2 << 0),
};


enum pipeline_type : u32
{
    PipelineType_Graphics = 0,
    PipelineType_Compute,

    PipelineType_Count,
};

enum pipeline_stage : flags32
{
    PipelineStage_None = 0,

    PipelineStage_VS = (1 << 0),
    PipelineStage_PS = (1 << 1),
    PipelineStage_HS = (1 << 2),
    PipelineStage_DS = (1 << 3),
    PipelineStage_GS = (1 << 4),

    PipelineStage_CS = (1 << 5),

    PipelineStage_Count = 6, // NOTE(boti): This has to be kept up to date !!!

    PipelineStage_AllGfx = PipelineStage_VS|PipelineStage_PS|PipelineStage_DS|PipelineStage_HS|PipelineStage_GS,
    PipelineStage_All = PipelineStage_AllGfx|PipelineStage_CS,
};

enum texture_swizzle_type : u8
{
    Swizzle_Identity = 0,
    Swizzle_Zero,
    Swizzle_One,
    Swizzle_R,
    Swizzle_G,
    Swizzle_B,
    Swizzle_A,
};

struct texture_swizzle
{
    texture_swizzle_type R, G, B, A;
};

struct format_byterate
{
    u32 Numerator;
    u32 Denominator;
    b32 IsBlock;
};

enum format : u32
{
    Format_Undefined = 0,

    Format_R8_UNorm,
    Format_R8_UInt,
    Format_R8_SRGB,
    Format_R8G8_UNorm,
    Format_R8G8_UInt,
    Format_R8G8_SRGB,
    Format_R8G8B8_UNorm,
    Format_R8G8B8_UInt,
    Format_R8G8B8_SRGB,
    Format_R8G8B8A8_UNorm,
    Format_R8G8B8A8_UInt,
    Format_R8G8B8A8_SRGB,

    Format_R16_UNorm,
    Format_R16_UInt,
    Format_R16_Float,
    Format_R16G16_UNorm,
    Format_R16G16_UInt,
    Format_R16G16_Float,
    Format_R16G16B16_UNorm,
    Format_R16G16B16_UInt,
    Format_R16G16B16_Float,
    Format_R16G16B16A16_UNorm,
    Format_R16G16B16A16_UInt,
    Format_R16G16B16A16_Float,

    Format_R32_UInt,
    Format_R32_Float,
    Format_R32G32_UInt,
    Format_R32G32_Float,
    Format_R32G32B32_UInt,
    Format_R32G32B32_Float,
    Format_R32G32B32A32_UInt,
    Format_R32G32B32A32_Float,

    Format_R11G11B10_Float,

    // NOTE(boti): Stencil always UInt, D32 always float, D(<32) always UNorm
    Format_D16,
    Format_D24X8,
    Format_D32,
    Format_S8,
    Format_D16S8,
    Format_D24S8,
    Format_D32S8, // NOTE(boti): This is padded to 64-bits (D32S8X24)
    
    Format_BC1_RGB_UNorm,
    Format_BC1_RGB_SRGB,
    Format_BC1_RGBA_UNorm,
    Format_BC1_RGBA_SRGB,
    Format_BC2_UNorm,
    Format_BC2_SRGB,
    Format_BC3_UNorm,
    Format_BC3_SRGB,
    Format_BC4_UNorm,
    Format_BC4_SNorm,
    Format_BC5_UNorm,
    Format_BC5_SNorm,
    Format_BC6_UFloat,
    Format_BC6_SFloat,
    Format_BC7_UNorm,
    Format_BC7_SRGB,

    Format_Count,
};

enum topology_type : u32
{
    Topology_Undefined = 0,

    Topology_PointList,
    Topology_LineList,
    Topology_LineStrip,
    Topology_TriangleList,
    Topology_TriangleStrip,
    Topology_TriangleFan,
    Topology_LineListAdjacency,
    Topology_LineStripAdjacency,
    Topology_TriangleListAdjacency,
    Topology_TriangleStripAdjacency,
    Topology_PatchList,

    Topology_Count,
};

struct push_constant_range
{
    flags32 Stages;
    u32 Size;
    u32 Offset;
};

struct vertex_attrib
{
    u32 Index;
    u32 Binding;
    format Format;
    u32 ByteOffset;
};

struct vertex_binding
{
    u32 Stride;
    u32 InstanceStepRate; // NOTE(boti): 0 is per-vertex, >0 is instanced
};

struct vertex_state
{
    topology_type Topology;
    b32 EnablePrimitiveRestart;
    u32 BindingCount;
    u32 AttribCount;
    vertex_binding Bindings[MaxVertexBindingCount];
    vertex_attrib Attribs[MaxVertexAttribCount];
};

enum fill_mode : u32
{
    Fill_Solid = 0,
    Fill_Line,
    Fill_Point,
};

enum cull_flags : u32
{
    Cull_None = 0,
    Cull_Back = (1 << 0),
    Cull_Front = (1 << 1),
};

static format_byterate FormatByterateTable[Format_Count] = 
{
    [Format_Undefined]          = { 0, 0, false },

    [Format_R8_UNorm]           = { 1*1, 1, false },
    [Format_R8_UInt]            = { 1*1, 1, false },
    [Format_R8_SRGB]            = { 1*1, 1, false },
    [Format_R8G8_UNorm]         = { 2*1, 1, false },
    [Format_R8G8_UInt]          = { 2*1, 1, false },
    [Format_R8G8_SRGB]          = { 2*1, 1, false },
    [Format_R8G8B8_UNorm]       = { 3*1, 1, false },
    [Format_R8G8B8_UInt]        = { 3*1, 1, false },
    [Format_R8G8B8_SRGB]        = { 3*1, 1, false },
    [Format_R8G8B8A8_UNorm]     = { 4*1, 1, false },
    [Format_R8G8B8A8_UInt]      = { 4*1, 1, false },
    [Format_R8G8B8A8_SRGB]      = { 4*1, 1, false },
    [Format_R16_UNorm]          = { 1*2, 1, false },
    [Format_R16_UInt]           = { 1*2, 1, false },
    [Format_R16_Float]          = { 1*2, 1, false },
    [Format_R16G16_UNorm]       = { 2*2, 1, false },
    [Format_R16G16_UInt]        = { 2*2, 1, false },
    [Format_R16G16_Float]       = { 2*2, 1, false },
    [Format_R16G16B16_UNorm]    = { 3*2, 1, false },
    [Format_R16G16B16_UInt]     = { 3*2, 1, false },
    [Format_R16G16B16_Float]    = { 3*2, 1, false },
    [Format_R16G16B16A16_UNorm] = { 4*2, 1, false },
    [Format_R16G16B16A16_UInt]  = { 4*2, 1, false },
    [Format_R16G16B16A16_Float] = { 4*2, 1, false },
    [Format_R32_UInt]           = { 1*4, 1, false },
    [Format_R32_Float]          = { 1*4, 1, false },
    [Format_R32G32_UInt]        = { 2*4, 1, false },
    [Format_R32G32_Float]       = { 2*4, 1, false },
    [Format_R32G32B32_UInt]     = { 3*4, 1, false },
    [Format_R32G32B32_Float]    = { 3*4, 1, false },
    [Format_R32G32B32A32_UInt]  = { 4*4, 1, false },
    [Format_R32G32B32A32_Float] = { 4*4, 1, false },

    [Format_R11G11B10_Float]    = { 4, 1, false},
    [Format_D16]                = { 2, 1, false },
    [Format_D24X8]              = { 4, 1, false },
    [Format_D32]                = { 4, 1, false },
    [Format_S8]                 = { 1, 1, false },
    [Format_D24S8]              = { 4, 1, false },
    [Format_D32S8]              = { 8, 1, false },

    [Format_BC1_RGB_UNorm]  = { 1, 2, true },
    [Format_BC1_RGB_SRGB]   = { 1, 2, true },
    [Format_BC1_RGBA_UNorm] = { 1, 2, true },
    [Format_BC1_RGBA_SRGB]  = { 1, 2, true },
    [Format_BC2_UNorm]      = { 1, 1, true },
    [Format_BC2_SRGB]       = { 1, 1, true },
    [Format_BC3_UNorm]      = { 1, 1, true },
    [Format_BC3_SRGB]       = { 1, 1, true },
    [Format_BC4_UNorm]      = { 1, 1, true },
    [Format_BC4_SNorm]      = { 1, 1, true },
    [Format_BC5_UNorm]      = { 2, 1, true },
    [Format_BC5_SNorm]      = { 2, 1, true },
    [Format_BC6_UFloat]     = { 1, 1, true },
    [Format_BC6_SFloat]     = { 1, 1, true },
    [Format_BC7_UNorm]      = { 1, 1, true },
    [Format_BC7_SRGB]       = { 1, 1, true },
};

//
// High-level renderer
//

struct ssao_params
{
    f32 Intensity;
    f32 MaxDistance;
    f32 TangentTau;

    static constexpr f32 DefaultIntensity = 8.0f;
    static constexpr f32 DefaultMaxDistance = 0.5f;
    static constexpr f32 DefaultTangentTau = 0.03125;
};

struct bloom_params
{
    f32 FilterRadius;
    f32 InternalStrength;
    f32 Strength;

    static constexpr f32 DefaultFilterRadius = 0.005f;
    static constexpr f32 DefaultInternalStrength = 0.4f;
    static constexpr f32 DefaultStrength = 0.3f;
};

struct post_process_params
{
    ssao_params SSAO;
    bloom_params Bloom;
};

struct texture_id
{
    u32 Value;
};

union frustum
{
    v4 Planes[6];
    struct
    {
        v4 Left;
        v4 Right;
        v4 Top;
        v4 Bottom;
        v4 Near;
        v4 Far;
    };
};

struct render_camera
{
    m4 CameraTransform;
    m4 ViewTransform; // aka Inverse camera transform

    f32 FocalLength;
    f32 NearZ;
    f32 FarZ;
};

union rgba8
{
    u32 Color;
    struct
    {
        u8 R;
        u8 G;
        u8 B;
        u8 A;
    };
};
static_assert(sizeof(rgba8) == sizeof(u32));

typedef u32 vert_index;

struct vertex
{
    v3 P;
    v3 N;
    v4 T;
    v2 TexCoord;
    rgba8 Color;
};

struct ui_vertex
{
    v2 P;
    v2 TexCoord;
    rgba8 Color;
};

struct material
{
    v3 Emissive;
    rgba8 DiffuseColor;
    rgba8 BaseMaterial;
    texture_id DiffuseID;
    texture_id NormalID;
    texture_id MetallicRoughnessID;
};

// TODO(boti): Rename this. we'd probably want pipeline-specific push constant types
struct alignas(4) push_constants
{
    m4 Transform;
    material Material;
};

inline constexpr rgba8 PackRGBA8(u32 R, u32 G, u32 B, u32 A = 0xFF);
inline rgba8 PackRGBA(v4 Color);
inline u32 GetMaxMipCount(u32 Width, u32 Height);
inline u32 GetMipChainTexelCount(u32 Width, u32 Height, u32 MaxMipCount = 0xFFFFFFFFu);

//
// Render API
//
struct frame_uniform_data
{
    m4 CameraTransform;
    m4 ViewTransform;
    m4 ProjectionTransform;
    m4 ViewProjectionTransform;

    m4 CascadeViewProjection;
    f32 CascadeMinDistances[4];
    f32 CascadeMaxDistances[4];
    v4 CascadeScales[3];
    v4 CascadeOffsets[3];

    f32 FocalLength;
    f32 AspectRatio;
    f32 NearZ;
    f32 FarZ;

    v3 CameraP;
    f32 Padding0;

    v3 SunV; // NOTE(boti): view space
    f32 Padding1;
    v3 SunL;
    f32 Padding2;

    v2 ScreenSize;
};

struct render_frame;
struct renderer;

render_frame* BeginRenderFrame(renderer* Renderer, u32 OutputWidth, u32 OutputHeight);
void EndRenderFrame(render_frame* Frame);

void SetRenderCamera(render_frame* Frame, const render_camera* Camera);
void SetLights(render_frame* Frame, v3 SunDirection, v3 SunLuminance);

void BeginSceneRendering(render_frame* Frame);
void EndSceneRendering(render_frame* Frame);

#include "VulkanRenderer.hpp"

//
// Implementation
//
inline constexpr rgba8 PackRGBA8(u32 R, u32 G, u32 B, u32 A /*= 0xFF*/)
{
    rgba8 Result = 
    {
        .R = (u8)R,
        .G = (u8)G,
        .B = (u8)B,
        .A = (u8)A,
    };
    return Result;
}

inline rgba8 PackRGBA(v4 Color)
{
    rgba8 Result = 
    {
        .R = (u8)Round(Color.x * 255.0f),
        .G = (u8)Round(Color.y * 255.0f),
        .B = (u8)Round(Color.z * 255.0f),
        .A = (u8)Round(Color.w * 255.0f),
    };
    return Result;
}

inline u32 GetMaxMipCount(u32 Width, u32 Height)
{
    u32 Result = 0;
    if (BitScanReverse(&Result, Max(Width, Height)))
    {
        Result += 1;
    }
    else
    {
        // NOTE(boti): The x64/MSDN spec says that the result is undefined if value is 0,
        //             so we kind of have to make sure that the result will also be 0 in that case.
        Result = 0; 
    }
    return Result;
}

// NOTE(boti): Returns the mip count of a mip chain when the lowest resolution mip level
//             must be at least 2x2
inline u32 GetMaxMipCountGreaterThanOne(u32 Width, u32 Height)
{
    u32 Result = 0;
    if (!BitScanReverse(&Result, Min(Width, Height)))
    {
        Result = 0;
    }
    return Result;
}

inline u32 GetMipChainTexelCount(u32 Width, u32 Height, u32 MaxMipCount /*= 0xFFFFFFFFu*/)
{
    u32 Result = 0;
    u32 MipCount = Min(GetMaxMipCount(Width, Height), MaxMipCount);

    u32 CurrentSize = Width * Height;

    for (u32 i = 0; i < MipCount; i++)
    {
        Result += CurrentSize;
        CurrentSize = Max(CurrentSize / 4, 1u);
    }

    return Result;
}