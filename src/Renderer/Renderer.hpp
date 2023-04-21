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

//
// Utils
//
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

    v3 SunV;
    f32 Padding1;
    v3 SunL;
    f32 Padding2;

    v2 ScreenSize;
};

struct render_frame;
struct renderer;

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