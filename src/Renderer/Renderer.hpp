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
constexpr u32 MaxVertexInputBindingCount = 16;
constexpr u32 MaxVertexInputAttribCount = 32;
constexpr u32 MaxColorAttachmentCount = 8;
constexpr u32 MaxDescriptorSetCount = 256;
constexpr u32 MaxPushConstantRangeCount = 64;
constexpr u32 MaxDescriptorSetLayoutBindingCount = 32;

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

struct texture_id
{
    u32 Value;
};

enum swizzle_type : u8
{
    Swizzle_Identity = 0,
    Swizzle_Zero,
    Swizzle_One,
    Swizzle_R,
    Swizzle_G,
    Swizzle_B,
    Swizzle_A,
};

struct swizzle
{
    swizzle_type R, G, B, A;
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