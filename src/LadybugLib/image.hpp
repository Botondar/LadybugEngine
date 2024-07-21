#pragma once

typedef flags32 format_flags;
enum format_flag_bits : format_flags
{
    FormatFlag_None = 0,

    FormatFlag_BlockCompressed = (1u << 0),
};

struct format_info
{
    u32 ByteRateNumerator;
    u32 ByteRateDenominator;
    u32 ChannelCount;
    format_flags Flags;
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

    Format_B8G8R8A8_SRGB,

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

enum image_file_type : u32
{
    ImageFile_Undefined = 0,

    ImageFile_BMP,
    ImageFile_TIF,
    ImageFile_PNG,
    ImageFile_JPG,
    ImageFile_DDS,

    ImageFile_Count,
};

//
// DDS
//

constexpr u32 DDSMagic = 0x20534444;

typedef flags32 dds_flags;
enum dds_flag_bits : dds_flags
{
    DDSFlag_None = 0,

    DDSFlag_Caps        = (1u << 0),
    DDSFlag_Height      = (1u << 1),
    DDSFlag_Width       = (1u << 2),
    DDSFlag_Pitch       = (1u << 3),

    DDSFlag_PixelFormat = (1u << 12),
    DDSFlag_MipMapCount = (1u << 17),
    DDSFlag_LinearSize  = (1u << 19),
    DDSFlag_Depth       = (1u << 23)
};

typedef flags32 dds_caps;
enum dds_caps_bits : dds_caps
{
    DDSCaps_None = 0,

    DDSCaps_Complex     = (1u << 3),
    DDSCaps_Texture     = (1u << 12),
    DDSCaps_MipMap      = (1u << 22),
};

typedef flags32 dds_caps2;
enum dds_caps2_bits : dds_caps2
{
    DDSCaps2_None = 0,

    DDSCaps2_Cubemap            = (1u << 9),
    DDSCaps2_CubemapPositiveX   = (1u << 10),
    DDSCaps2_CubemapNegativeX   = (1u << 11),
    DDSCaps2_CubemapPositiveY   = (1u << 12),
    DDSCaps2_CubemapNegativeY   = (1u << 13),
    DDSCaps2_CubemapPositiveZ   = (1u << 14),
    DDSCaps2_CubemapNegativeZ   = (1u << 15),
    DDSCaps2_Volume             = (1u << 21),
};

// NOTE(boti): caps3 and 4 are for future extensions
typedef flags32 dds_caps3;
typedef flags32 dds_caps4;

typedef flags32 dds_pixel_format_flags;
enum dds_pixel_format_flag_bits : dds_pixel_format_flags
{
    DDSPixelFormat_None = 0,

    DDSPixelFormat_AlphaPixels      = (1u << 0),
    DDSPixelFormat_Alpha            = (1u << 1),
    DDSPixelFormat_FourCC           = (1u << 2),
    DDSPixelFormat_UncompressedRGB  = (1u << 6), // NOTE(boti): RGB masks contain valid data
    DDSPixelFormat_YUV              = (1u << 9),
    DDSPixelFormat_Luminance        = (1u << 17),
};

enum dds_fourcc : u32
{
    DDS_DXT1 = '1TXD',
    DDS_DXT2 = '2TXD',
    DDS_DXT3 = '3TXD',
    DDS_DXT4 = '4TXD',
    DDS_DXT5 = '5TXD',
    DDS_DX10 = '01XD',
};

struct dds_pixel_format
{
    u32                     StructureSize;
    dds_pixel_format_flags  Flags;
    dds_fourcc              FourCC;
    u32                     BitsPerChannel;
    u32                     BitMaskR;
    u32                     BitMaskG;
    u32                     BitMaskB;
    u32                     BitMaskA;
};
static_assert(sizeof(dds_pixel_format) == 32);

struct dds_header
{
    u32                 HeaderSize;
    dds_flags           Flags;
    u32                 Height;
    u32                 Width;
    u32                 PitchOrLinearSize;
    u32                 Depth;
    u32                 MipMapCount;
    u32                 Reserved1[11];
    dds_pixel_format    PixelFormat;
    u32                 Caps;
    u32                 Caps2;
    u32                 Caps3;
    u32                 Caps4;
    u32                 Reserved2;
};

enum dxgi_format : u32
{
    DXGIFormat_Undefined = 0,

    DXGIFormat_RGBA32_Typeless,
    DXGIFormat_RGBA32_Float,
    DXGIFormat_RGBA32_UInt,
    DXGIFormat_RGBA32_SInt,

    DXGIFormat_RGB32_Typeless,
    DXGIFormat_RGB32_Float,
    DXGIFormat_RGB32_UInt,
    DXGIFormat_RGB32_SInt,

    DXGIFormat_RGBA16_Typeless,
    DXGIFormat_RGBA16_Float,
    DXGIFormat_RGBA16_UNorm,
    DXGIFormat_RGBA16_UInt,
    DXGIFormat_RGBA16_SNorm,
    DXGIFormat_RGBA16_SInt,

    DXGIFormat_RG32_Typeless,
    DXGIFormat_RG32_Float,
    DXGIFormat_RG32_UInt,
    DXGIFormat_RG32_SInt,

    DXGIFormat_R32G8X24_Typeless,
    DXGIFormat_D32_Float_S8X24_UInt,
    DXGIFormat_R32_Float_X8X24_Typeless,
    DXGIFormat_X32_Typeless_G8X24_UInt,

    DXGIFormat_RGB10A2_Typeless,
    DXGIFormat_RGB10A2_UNorm,
    DXGIFormat_RGB10A2_UInt,
    DXGIFormat_R11G11B10_Float,

    DXGIFormat_RGBA8_Typeless,
    DXGIFormat_RGBA8_UNorm,
    DXGIFormat_RGBA8_UNorm_SRGB,
    DXGIFormat_RGBA8_UInt,
    DXGIFormat_RGBA8_SNorm,
    DXGIFormat_RGBA8_SInt,

    DXGIFormat_RG16_Typeless,
    DXGIFormat_RG16_Float,
    DXGIFormat_RG16_UNorm,
    DXGIFormat_RG16_UInt,
    DXGIFormat_RG16_SNorm,
    DXGIFormat_RG16_SInt,

    DXGIFormat_R32_Typeless,
    DXGIFormat_D32_Float,
    DXGIFormat_R32_Float,
    DXGIFormat_R32_UInt,
    DXGIFormat_R32_SInt,
    
    DXGIFormat_R24G8_Typeless,
    DXGIFormat_D24_UNorm_S8_UInt,
    DXGIFormat_R24_UNorm_X8_Typeless,
    DXGIFormat_X24_Typeless_G8_UInt,

    DXGIFormat_RG8_Typeless,
    DXGIFormat_RG8_UNorm,
    DXGIFormat_RG8_UInt,
    DXGIFormat_RG8_SNorm,
    DXGIFormat_RG8_SInt,

    DXGIFormat_R16_Typeless,
    DXGIFormat_R16_Float,
    DXGIFormat_D16_UNorm,
    DXGIFormat_R16_UNorm,
    DXGIFormat_R16_UInt,
    DXGIFormat_R16_SNorm,
    DXGIFormat_R16_SInt,

    DXGIFormat_R8_Typeless,
    DXGIFormat_R8_UNorm,
    DXGIFormat_R8_UInt,
    DXGIFormat_R8_SNorm,
    DXGIFormat_R8_SInt,
    DXGIFormat_A8_UNorm,
    DXGIFormat_R1_UNorm,

    DXGIFormat_RGB9E5,
    DXGIFormat_RG8_BG8_UNorm,
    DXGIFormat_GR8_GB8_UNorm,

    DXGIFormat_BC1_Typeless,
    DXGIFormat_BC1_UNorm,
    DXGIFormat_BC1_UNorm_SRGB,

    DXGIFormat_BC2_Typeless,
    DXGIFormat_BC2_UNorm,
    DXGIFormat_BC2_UNorm_SRGB,

    DXGIFormat_BC3_Typeless,
    DXGIFormat_BC3_UNorm,
    DXGIFormat_BC3_UNorm_SRGB,

    DXGIFormat_BC4_Typeless,
    DXGIFormat_BC4_UNorm,
    DXGIFormat_BC4_SNorm,

    DXGIFormat_BC5_Typeless,
    DXGIFormat_BC5_UNorm,
    DXGIFormat_BC5_SNorm,

    DXGIFormat_B5G6R5_UNorm,
    DXGIFormat_B5G5R5A1_UNorm,
    DXGIFormat_BGRA8_UNorm,
    DXGIFormat_BGRX8_UNorm,

    DXGIFormat_RGB10_XR_Bias_A2_UNorm,

    DXGIFormat_BGRA8_Typeless,
    DXGIFormat_BGRA8_UNorm_SRGB,
    DXGIFormat_BGRX8_Typeless,
    DXGIFormat_BGRX8_UNorm_SRGB,

    DXGIFormat_BC6H_Typeless,
    DXGIFormat_BC6H_UF16,
    DXGIFormat_BC6H_SF16,

    DXGIFormat_BC7_Typeless,
    DXGIFormat_BC7_UNorm,
    DXGIFormat_BC7_UNorm_SRGB,

    // TODO(boti): More types here!

    DXGIFormat_Count,
};

static_assert(DXGIFormat_BC7_UNorm_SRGB == 99);

enum dds_dimension : u32
{
    DDSDim_Texture1D = 2,
    DDSDim_Texture2D = 3,
    DDSDim_Texture3D = 4,
};

typedef flags32 dds_resource_flags;
enum dds_resource_flag_bits : dds_resource_flags
{
    DDSResourceFlag_None = 0,

    DDSResourceFlag_TextureCube = (1u << 2),
};

enum dds_alpha_mode : u32
{
    DDSAlpha_Undefined      = 0,
    DDSAlpha_Straight       = 1,
    DDSAlpha_Premultiplied  = 2,
    DDSAlpha_Opaque         = 3,
    DDSAlpha_Custom         = 4,
};

struct dds_header_dx10
{
    dxgi_format         Format;
    dds_dimension       Dimension;
    dds_resource_flags  ResourceFlags;
    u32                 ArrayCount;
    dds_alpha_mode      AlphaMode; // TODO(boti): Technically the upper 29 bits could be anything...
};

constexpr u32 DDSHeaderSize = 124;
static_assert(sizeof(dds_header) == DDSHeaderSize);

struct dds_file
{
    u32             Magic;
    dds_header      Header;
    dds_header_dx10 DX10Header;
    u8              Data[];
};

//
// Image functions
//

inline u32 GetMaxMipCount(u32 Width, u32 Height);
inline u32 GetMipChainTexelCount(u32 Width, u32 Height, u32 MaxMipCount = 0xFFFFFFFFu);
inline u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, u32 ArrayCount, format_info ByteRate);

lbfn image_file_type 
DetermineImageFileType(buffer FileData);

struct loaded_image
{
    v2u Extent;
    format Format;
    void* Data;
};

lbfn loaded_image
LoadImage(memory_arena* Arena, buffer FileData);

// Alpha test mipmap generation as described by Ignacio Castano
// NOTE(boti): Image must be RGBA8
lbfn f32 CalculateAlphaCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 AlphaScale);
lbfn f32 CalculateAlphaScaleForCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 TargetCoverage);
// NOTE(boti): Returns the scale used
lbfn f32 RescaleAlphaForCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 TargetCoverage);

//
// Implementation
//
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

inline u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, u32 ArrayCount, format_info ByteRate)
{
    u64 Result = 0;

    MipCount = Min(GetMaxMipCount(Width, Height), MipCount);
    for (u32 Mip = 0; Mip < MipCount; Mip++)
    {
        u32 CurrentWidth = Max(Width >> Mip, 1u);
        u32 CurrentHeight = Max(Height >> Mip, 1u);
        if (ByteRate.Flags & FormatFlag_BlockCompressed)
        {
            CurrentWidth = Align(CurrentWidth, 4u);
            CurrentHeight = Align(CurrentHeight, 4u);
        }

        Result += ((u64)CurrentWidth * (u64)CurrentHeight * ByteRate.ByteRateNumerator) / ByteRate.ByteRateDenominator;
    }
    Result *= ArrayCount;

    return Result;
}

internal format_info FormatInfoTable[Format_Count] = 
{
    [Format_Undefined]          = { 0, 0, 0, FormatFlag_None },

    [Format_R8_UNorm]           = { 1*1, 1, 1, FormatFlag_None },
    [Format_R8_UInt]            = { 1*1, 1, 1, FormatFlag_None },
    [Format_R8_SRGB]            = { 1*1, 1, 1, FormatFlag_None },
    [Format_R8G8_UNorm]         = { 2*1, 1, 2, FormatFlag_None },
    [Format_R8G8_UInt]          = { 2*1, 1, 2, FormatFlag_None },
    [Format_R8G8_SRGB]          = { 2*1, 1, 2, FormatFlag_None },
    [Format_R8G8B8_UNorm]       = { 3*1, 1, 3, FormatFlag_None },
    [Format_R8G8B8_UInt]        = { 3*1, 1, 3, FormatFlag_None },
    [Format_R8G8B8_SRGB]        = { 3*1, 1, 3, FormatFlag_None },
    [Format_R8G8B8A8_UNorm]     = { 4*1, 1, 4, FormatFlag_None },
    [Format_R8G8B8A8_UInt]      = { 4*1, 1, 4, FormatFlag_None },
    [Format_R8G8B8A8_SRGB]      = { 4*1, 1, 4, FormatFlag_None },
    [Format_B8G8R8A8_SRGB]      = { 4*1, 1, 4, FormatFlag_None },
    [Format_R16_UNorm]          = { 1*2, 1, 1, FormatFlag_None },
    [Format_R16_UInt]           = { 1*2, 1, 1, FormatFlag_None },
    [Format_R16_Float]          = { 1*2, 1, 1, FormatFlag_None },
    [Format_R16G16_UNorm]       = { 2*2, 1, 2, FormatFlag_None },
    [Format_R16G16_UInt]        = { 2*2, 1, 2, FormatFlag_None },
    [Format_R16G16_Float]       = { 2*2, 1, 2, FormatFlag_None },
    [Format_R16G16B16_UNorm]    = { 3*2, 1, 3, FormatFlag_None },
    [Format_R16G16B16_UInt]     = { 3*2, 1, 3, FormatFlag_None },
    [Format_R16G16B16_Float]    = { 3*2, 1, 3, FormatFlag_None },
    [Format_R16G16B16A16_UNorm] = { 4*2, 1, 4, FormatFlag_None },
    [Format_R16G16B16A16_UInt]  = { 4*2, 1, 4, FormatFlag_None },
    [Format_R16G16B16A16_Float] = { 4*2, 1, 4, FormatFlag_None },
    [Format_R32_UInt]           = { 1*4, 1, 1, FormatFlag_None },
    [Format_R32_Float]          = { 1*4, 1, 1, FormatFlag_None },
    [Format_R32G32_UInt]        = { 2*4, 1, 2, FormatFlag_None },
    [Format_R32G32_Float]       = { 2*4, 1, 2, FormatFlag_None },
    [Format_R32G32B32_UInt]     = { 3*4, 1, 3, FormatFlag_None },
    [Format_R32G32B32_Float]    = { 3*4, 1, 3, FormatFlag_None },
    [Format_R32G32B32A32_UInt]  = { 4*4, 1, 4, FormatFlag_None },
    [Format_R32G32B32A32_Float] = { 4*4, 1, 4, FormatFlag_None },

    [Format_R11G11B10_Float]    = { 4, 1, 3, FormatFlag_None },
    [Format_D16]                = { 2, 1, 1, FormatFlag_None },
    [Format_D24X8]              = { 4, 1, 2, FormatFlag_None },
    [Format_D32]                = { 4, 1, 1, FormatFlag_None },
    [Format_S8]                 = { 1, 1, 1, FormatFlag_None },
    [Format_D24S8]              = { 4, 1, 2, FormatFlag_None },
    [Format_D32S8]              = { 8, 1, 2, FormatFlag_None },

    [Format_BC1_RGB_UNorm]  = { 1, 2, 3, FormatFlag_BlockCompressed },
    [Format_BC1_RGB_SRGB]   = { 1, 2, 3, FormatFlag_BlockCompressed },
    [Format_BC1_RGBA_UNorm] = { 1, 2, 4, FormatFlag_BlockCompressed },
    [Format_BC1_RGBA_SRGB]  = { 1, 2, 4, FormatFlag_BlockCompressed },
    [Format_BC2_UNorm]      = { 1, 1, 4, FormatFlag_BlockCompressed },
    [Format_BC2_SRGB]       = { 1, 1, 4, FormatFlag_BlockCompressed },
    [Format_BC3_UNorm]      = { 1, 1, 4, FormatFlag_BlockCompressed },
    [Format_BC3_SRGB]       = { 1, 1, 4, FormatFlag_BlockCompressed },
    [Format_BC4_UNorm]      = { 1, 2, 1, FormatFlag_BlockCompressed },
    [Format_BC4_SNorm]      = { 1, 2, 1, FormatFlag_BlockCompressed },
    [Format_BC5_UNorm]      = { 1, 1, 2, FormatFlag_BlockCompressed },
    [Format_BC5_SNorm]      = { 1, 1, 2, FormatFlag_BlockCompressed },
    [Format_BC6_UFloat]     = { 1, 1, 3, FormatFlag_BlockCompressed },
    [Format_BC6_SFloat]     = { 1, 1, 3, FormatFlag_BlockCompressed },
    [Format_BC7_UNorm]      = { 1, 1, 4, FormatFlag_BlockCompressed },
    [Format_BC7_SRGB]       = { 1, 1, 4, FormatFlag_BlockCompressed },
};

internal format DXGIFormatTable[DXGIFormat_Count] =
{
    [DXGIFormat_Undefined]                  = Format_Undefined,
    [DXGIFormat_RGBA32_Typeless]            = Format_Undefined,
    [DXGIFormat_RGBA32_Float]               = Format_R32G32B32A32_Float,
    [DXGIFormat_RGBA32_UInt]                = Format_R32G32B32A32_UInt,
    [DXGIFormat_RGBA32_SInt]                = Format_Undefined,
    [DXGIFormat_RGB32_Typeless]             = Format_Undefined,
    [DXGIFormat_RGB32_Float]                = Format_R32G32B32_Float,
    [DXGIFormat_RGB32_UInt]                 = Format_R32G32B32_UInt,
    [DXGIFormat_RGB32_SInt]                 = Format_Undefined,
    [DXGIFormat_RGBA16_Typeless]            = Format_Undefined,
    [DXGIFormat_RGBA16_Float]               = Format_R16G16B16A16_Float,
    [DXGIFormat_RGBA16_UNorm]               = Format_R16G16B16A16_UNorm,
    [DXGIFormat_RGBA16_UInt]                = Format_R16G16B16A16_UInt,
    [DXGIFormat_RGBA16_SNorm]               = Format_Undefined,
    [DXGIFormat_RGBA16_SInt]                = Format_Undefined,
    [DXGIFormat_RG32_Typeless]              = Format_Undefined,
    [DXGIFormat_RG32_Float]                 = Format_R32G32_Float,
    [DXGIFormat_RG32_UInt]                  = Format_R32G32_UInt,
    [DXGIFormat_RG32_SInt]                  = Format_Undefined,
    [DXGIFormat_R32G8X24_Typeless]          = Format_D32S8,
    [DXGIFormat_D32_Float_S8X24_UInt]       = Format_D32S8,
    [DXGIFormat_R32_Float_X8X24_Typeless]   = Format_D32S8,
    [DXGIFormat_X32_Typeless_G8X24_UInt]    = Format_D32S8,
    [DXGIFormat_RGB10A2_Typeless]           = Format_Undefined,
    [DXGIFormat_RGB10A2_UNorm]              = Format_Undefined,
    [DXGIFormat_RGB10A2_UInt]               = Format_Undefined,
    [DXGIFormat_R11G11B10_Float]            = Format_R11G11B10_Float,
    [DXGIFormat_RGBA8_Typeless]             = Format_R8G8B8A8_UNorm,
    [DXGIFormat_RGBA8_UNorm]                = Format_R8G8B8A8_UNorm,
    [DXGIFormat_RGBA8_UNorm_SRGB]           = Format_R8G8B8A8_SRGB,
    [DXGIFormat_RGBA8_UInt]                 = Format_R8G8B8A8_UInt,
    [DXGIFormat_RGBA8_SNorm]                = Format_Undefined,
    [DXGIFormat_RGBA8_SInt]                 = Format_Undefined,
    [DXGIFormat_RG16_Typeless]              = Format_Undefined,
    [DXGIFormat_RG16_Float]                 = Format_R16G16_Float,
    [DXGIFormat_RG16_UNorm]                 = Format_R16G16_UNorm,
    [DXGIFormat_RG16_UInt]                  = Format_R16G16_UInt,
    [DXGIFormat_RG16_SNorm]                 = Format_Undefined,
    [DXGIFormat_RG16_SInt]                  = Format_Undefined,
    [DXGIFormat_R32_Typeless]               = Format_R32_Float,
    [DXGIFormat_D32_Float]                  = Format_D32,
    [DXGIFormat_R32_Float]                  = Format_R32_Float,
    [DXGIFormat_R32_UInt]                   = Format_R32_UInt,
    [DXGIFormat_R32_SInt]                   = Format_Undefined,
    [DXGIFormat_R24G8_Typeless]             = Format_D24S8,
    [DXGIFormat_D24_UNorm_S8_UInt]          = Format_D24S8,
    [DXGIFormat_R24_UNorm_X8_Typeless]      = Format_D24X8,
    [DXGIFormat_X24_Typeless_G8_UInt]       = Format_Undefined,
    [DXGIFormat_RG8_Typeless]               = Format_R8G8_UNorm,
    [DXGIFormat_RG8_UNorm]                  = Format_R8G8_UNorm,
    [DXGIFormat_RG8_UInt]                   = Format_R8G8_UInt,
    [DXGIFormat_RG8_SNorm]                  = Format_Undefined,
    [DXGIFormat_RG8_SInt]                   = Format_Undefined,
    [DXGIFormat_R16_Typeless]               = Format_R16_UNorm,
    [DXGIFormat_R16_Float]                  = Format_R16_Float,
    [DXGIFormat_D16_UNorm]                  = Format_D16,
    [DXGIFormat_R16_UNorm]                  = Format_R16_UNorm,
    [DXGIFormat_R16_UInt]                   = Format_R16_UInt,
    [DXGIFormat_R16_SNorm]                  = Format_Undefined,
    [DXGIFormat_R16_SInt]                   = Format_Undefined,
    [DXGIFormat_R8_Typeless]                = Format_R8_UNorm,
    [DXGIFormat_R8_UNorm]                   = Format_R8_UNorm,
    [DXGIFormat_R8_UInt]                    = Format_R8_UInt,
    [DXGIFormat_R8_SNorm]                   = Format_Undefined,
    [DXGIFormat_R8_SInt]                    = Format_Undefined,
    [DXGIFormat_A8_UNorm]                   = Format_R8_UNorm,
    [DXGIFormat_R1_UNorm]                   = Format_Undefined,
    [DXGIFormat_RGB9E5]                     = Format_Undefined,
    [DXGIFormat_RG8_BG8_UNorm]              = Format_Undefined,
    [DXGIFormat_GR8_GB8_UNorm]              = Format_Undefined,
    [DXGIFormat_BC1_Typeless]               = Format_BC1_RGBA_UNorm,
    [DXGIFormat_BC1_UNorm]                  = Format_BC1_RGBA_UNorm,
    [DXGIFormat_BC1_UNorm_SRGB]             = Format_BC1_RGBA_SRGB,
    [DXGIFormat_BC2_Typeless]               = Format_BC2_UNorm,
    [DXGIFormat_BC2_UNorm]                  = Format_BC2_UNorm,
    [DXGIFormat_BC2_UNorm_SRGB]             = Format_BC2_SRGB,
    [DXGIFormat_BC3_Typeless]               = Format_BC3_UNorm,
    [DXGIFormat_BC3_UNorm]                  = Format_BC3_UNorm,
    [DXGIFormat_BC3_UNorm_SRGB]             = Format_BC3_SRGB,
    [DXGIFormat_BC4_Typeless]               = Format_BC4_UNorm,
    [DXGIFormat_BC4_UNorm]                  = Format_BC4_UNorm,
    [DXGIFormat_BC4_SNorm]                  = Format_BC4_SNorm,
    [DXGIFormat_BC5_Typeless]               = Format_BC5_UNorm,
    [DXGIFormat_BC5_UNorm]                  = Format_BC5_UNorm,
    [DXGIFormat_BC5_SNorm]                  = Format_BC5_SNorm,
    [DXGIFormat_B5G6R5_UNorm]               = Format_Undefined,
    [DXGIFormat_B5G5R5A1_UNorm]             = Format_Undefined,
    [DXGIFormat_BGRA8_UNorm]                = Format_Undefined,
    [DXGIFormat_BGRX8_UNorm]                = Format_Undefined,
    [DXGIFormat_RGB10_XR_Bias_A2_UNorm]     = Format_Undefined,
    [DXGIFormat_BGRA8_Typeless]             = Format_Undefined,
    [DXGIFormat_BGRA8_UNorm_SRGB]           = Format_B8G8R8A8_SRGB,
    [DXGIFormat_BGRX8_Typeless]             = Format_Undefined,
    [DXGIFormat_BGRX8_UNorm_SRGB]           = Format_Undefined,
    [DXGIFormat_BC6H_Typeless]              = Format_BC6_UFloat,
    [DXGIFormat_BC6H_UF16]                  = Format_BC6_UFloat,
    [DXGIFormat_BC6H_SF16]                  = Format_BC6_SFloat,
    [DXGIFormat_BC7_Typeless]               = Format_BC7_UNorm,
    [DXGIFormat_BC7_UNorm]                  = Format_BC7_UNorm,
    [DXGIFormat_BC7_UNorm_SRGB]             = Format_BC7_SRGB,
};