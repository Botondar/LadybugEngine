#include <LadybugLib/Core.hpp>
#include <LadybugLib/Intrinsics.hpp>
#include <LadybugLib/String.hpp>
#include <LadybugLib/JSON.hpp>
#include <LadybugLib/glTF.hpp>
#include <LadybugLib/image.hpp>

#include "stb/stb_image_resize2.h"
#include "stb/stb_dxt.h"

internal buffer LoadEntireFile(const char* Path, memory_arena* Arena);
internal b32 WriteEntireFile(const char* Path, umm Size, void* Memory);

// TODO(boti): I really don't like having this defined both here, and in the RHI, but at the same time
// tools should really only be pulling in the core lib...
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

u32 SwizzleToChannelIndex(texture_swizzle_type Swizzle)
{
    u32 Result = 0;
    switch (Swizzle)
    {
        case Swizzle_R: Result = 0; break;
        case Swizzle_G: Result = 1; break;
        case Swizzle_B: Result = 2; break;
        case Swizzle_A: Result = 3; break;
        InvalidDefaultCase;
    }
    return(Result);
}

inline u32 PackSwizzle(texture_swizzle_type R, texture_swizzle_type G, texture_swizzle_type B, texture_swizzle_type A)
{
    u32 Result = ((u32)R << 24) | ((u32)G << (16)) | ((u32)B << 8) | ((u32)A);
    return(Result);
}

typedef flags32 image_usage_flags;
enum image_usage_flag_bits : image_usage_flags
{
    ImageUsage_None = 0,

    ImageUsage_Albedo           = (1u << 0),
    ImageUsage_Normal           = (1u << 1),
    ImageUsage_RoMe             = (1u << 2),
    ImageUsage_Occlusion        = (1u << 3),
    ImageUsage_Height           = (1u << 4),
    ImageUsage_Transmission     = (1u << 5),
    ImageUsage_Emission         = (1u << 6),
};

struct image_entry
{
    image_usage_flags Usage;

    texture_swizzle_type RoughnessSwizzle;
    texture_swizzle_type MetallicSwizzle;

    filepath SrcPath;
    filepath DstPath;
};

internal void ResizeImage(void* OldData, v2u OldExtent, 
                           void* NewData, v2u NewExtent, 
                           image_usage_flags Usage, u32 ChannelCount,
                           f32 AlphaThreshold, f32 TargetAlphaCoverage)
{
    if (Usage == ImageUsage_Albedo)
    {
        stbir_pixel_layout PixelLayout = (ChannelCount == 4) ? STBIR_RGBA_PM : STBIR_RGB;
        stbir_resize_uint8_srgb(
            (u8*)OldData, OldExtent.X, OldExtent.Y, 0,
            (u8*)NewData, NewExtent.X, NewExtent.Y, 0,
            PixelLayout);

        if (ChannelCount == 4)
        {
            RescaleAlphaForCoverage(NewExtent, (u8*)NewData, AlphaThreshold, TargetAlphaCoverage);
        }
    }
    else
    {
        stbir_resize_uint8_linear(
            (u8*)OldData, OldExtent.X, OldExtent.Y, 0,
            (u8*)NewData, NewExtent.X, NewExtent.Y, 0,
            (stbir_pixel_layout)ChannelCount);
    }
}

internal void BCCompressBlock(u8* Dst, u8* Src, format Format)
{
    switch(Format)
    {
        case Format_BC1_RGB_UNorm:
        case Format_BC1_RGB_SRGB:
        {
            stb_compress_dxt_block(Dst, Src, 0, STB_DXT_HIGHQUAL);
        } break;
        case Format_BC3_UNorm:
        case Format_BC3_SRGB:
        {
            stb_compress_dxt_block(Dst, Src, 1, STB_DXT_HIGHQUAL);
        } break;
        case Format_BC4_UNorm:
        {
            stb_compress_bc4_block(Dst, Src);
        } break;
        case Format_BC5_UNorm:
        {
            stb_compress_bc5_block(Dst, Src);
        } break;
        InvalidDefaultCase;
    }
}

internal b32 ProcessImage(memory_arena* Arena, image_entry* Entry)
{
    b32 Result = 1;

    memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Arena);

    buffer FileBuffer = LoadEntireFile(Entry->SrcPath.Path, Arena);
    if (FileBuffer.Data)
    {
        loaded_image Image = LoadImage(Arena, FileBuffer);

        if (Image.Data)
        {
            format TargetFormat = Format_Undefined;
            dxgi_format DXGIFormat = DXGIFormat_Undefined;
            u32 Swizzle = 0;
            u32 ChannelsOfInterest[4] = { 0, 1, 2, 3 };
            switch (Entry->Usage)
            {
                case ImageUsage_Albedo:
                {
                    // TODO(boti): BC1 for textures without alpha
                    TargetFormat = Format_BC3_SRGB;
                    DXGIFormat = DXGIFormat_BC3_UNorm_SRGB;
                } break;
                case ImageUsage_Normal:
                {
                    TargetFormat = Format_BC5_UNorm;
                    DXGIFormat = DXGIFormat_BC5_UNorm;
                    ChannelsOfInterest[0] = 0;
                    ChannelsOfInterest[1] = 1;
                } break;
                case ImageUsage_RoMe:
                {
                    if (Entry->RoughnessSwizzle == Swizzle_One || Entry->RoughnessSwizzle == Swizzle_Zero)
                    {
                        // Texture is Metallic only, roughness is constant
                        TargetFormat = Format_BC4_UNorm;
                        DXGIFormat = DXGIFormat_BC4_UNorm;
                        ChannelsOfInterest[0] = SwizzleToChannelIndex(Entry->MetallicSwizzle);
                        Swizzle = PackSwizzle(Swizzle_B, Entry->RoughnessSwizzle, Swizzle_Identity, Swizzle_Identity);
                    }
                    else if (Entry->MetallicSwizzle == Swizzle_One || Entry->MetallicSwizzle == Swizzle_Zero)
                    {
                        // Texture is Roughness only, metallic is constant
                        TargetFormat = Format_BC4_UNorm;
                        DXGIFormat = DXGIFormat_BC4_UNorm;
                        ChannelsOfInterest[0] = SwizzleToChannelIndex(Entry->RoughnessSwizzle);
                        Swizzle = PackSwizzle(Swizzle_G, Swizzle_Identity, Entry->MetallicSwizzle, Swizzle_Identity);
                    }
                    else
                    {
                        // Both Roughness and Metallic are present
                        TargetFormat = Format_BC5_UNorm;
                        DXGIFormat = DXGIFormat_BC5_UNorm;
                        ChannelsOfInterest[0] = SwizzleToChannelIndex(Entry->RoughnessSwizzle);
                        ChannelsOfInterest[1] = SwizzleToChannelIndex(Entry->MetallicSwizzle);
                        Swizzle = PackSwizzle(Swizzle_G, Swizzle_B, Swizzle_Identity, Swizzle_Identity);
                    }
                    
                } break;
                case ImageUsage_Occlusion:
                {
                    TargetFormat = Format_BC4_UNorm;
                    DXGIFormat = DXGIFormat_BC4_UNorm;
                    ChannelsOfInterest[0] = 0;
                } break;
                InvalidDefaultCase;
            }

            f32 AlphaThreshold = 1.0f; // TODO(boti): This should come from the entry
            b32 DoAlphaCoverageRescale = (Entry->Usage == ImageUsage_Albedo) && ((Image.Format == Format_R8G8B8A8_UNorm) || (Image.Format == Format_R8G8B8A8_SRGB));
            f32 AlphaCoverage = 1.0f;            
            if (DoAlphaCoverageRescale)
            {
                AlphaCoverage = CalculateAlphaCoverage(Image.Extent, (u8*)Image.Data, AlphaThreshold, 1.0f);
            }

            // TODO(boti): Once we hammer out the edge cases there shouldn't be a need for rescaling to Pow2

            v2u PowerOf2Extent = { CeilPowerOf2(Image.Extent.X), CeilPowerOf2(Image.Extent.Y) };
            format_info SrcFormatInfo = FormatInfoTable[Image.Format];
            umm RescaleBufferSize = (umm)Image.Extent.X * Image.Extent.Y * SrcFormatInfo.ChannelCount * SrcFormatInfo.ByteRateNumerator / SrcFormatInfo.ByteRateDenominator;
            u8* RescaleBuffer = (u8*)PushSize_(Arena, 0, RescaleBufferSize, 64);

            if (PowerOf2Extent.X != Image.Extent.X || PowerOf2Extent.Y != Image.Extent.Y)
            {
                ResizeImage(Image.Data, Image.Extent, RescaleBuffer, PowerOf2Extent, Entry->Usage, SrcFormatInfo.ChannelCount, AlphaThreshold, AlphaCoverage);
                Image.Extent = PowerOf2Extent;
                Image.Data = RescaleBuffer;

                if (DoAlphaCoverageRescale)
                {
                    RescaleAlphaForCoverage(Image.Extent, (u8*)Image.Data, AlphaThreshold, AlphaCoverage);
                }
            }

            u8* SrcImage = (u8*)Image.Data;

            u32 MipCount = GetMaxMipCount(Image.Extent.X, Image.Extent.Y);
            format_info DstFormatInfo = FormatInfoTable[TargetFormat];

            umm TotalMipChainSize = GetMipChainSize(Image.Extent.X, Image.Extent.Y, MipCount, 1, DstFormatInfo);
            dds_file* DstFile = (dds_file*)PushSize_(Arena, 0, sizeof(dds_file) + TotalMipChainSize, 64);

            memset(DstFile, 0, sizeof(*DstFile));
            DstFile->Magic                              = DDSMagic;
            DstFile->Header.HeaderSize                  = sizeof(dds_header);
            DstFile->Header.Flags                       = DDSFlag_Caps|DDSFlag_Height|DDSFlag_Width|DDSFlag_PixelFormat|DDSFlag_MipMapCount;
            DstFile->Header.Height                      = Image.Extent.Y;
            DstFile->Header.Width                       = Image.Extent.X;
            DstFile->Header.PitchOrLinearSize           = 0; // TODO(boti): Is this required?
            DstFile->Header.MipMapCount                 = MipCount;
            DstFile->Header.EngineFourCC                = LadybugDDSMagic;
            DstFile->Header.Swizzle                     = Swizzle;
            DstFile->Header.PixelFormat.StructureSize   = sizeof(dds_pixel_format);
            DstFile->Header.PixelFormat.Flags           = DDSPixelFormat_FourCC;
            DstFile->Header.PixelFormat.FourCC          = DDS_DX10;
            DstFile->Header.Caps                        = DDSCaps_Complex|DDSCaps_Texture|DDSCaps_MipMap;
            DstFile->Header.Caps2                       = DDSCaps2_None;
            DstFile->DX10Header.Format                  = DXGIFormat;
            DstFile->DX10Header.Dimension               = DDSDim_Texture2D;
            DstFile->DX10Header.ResourceFlags           = DDSResourceFlag_None;
            DstFile->DX10Header.ArrayCount              = 1;
            DstFile->DX10Header.AlphaMode               = DDSAlpha_Undefined; // TODO(boti)


            u8* DstBuffer = DstFile->Data;
            u8* DstBlock = DstBuffer;
            umm DstBlockSize = 4 * 4 * DstFormatInfo.ByteRateNumerator / DstFormatInfo.ByteRateDenominator;

            //
            // Rescale and compress
            //
            for (u32 Mip = 0; Mip < MipCount; Mip++)
            {
                v2u CurrentExtent = { Image.Extent.X >> Mip, Image.Extent.Y >> Mip };
                if (Mip)
                {
                    v2u OldExtent = { Image.Extent.X >> (Mip - 1), Image.Extent.Y >> (Mip - 1) };
                    ResizeImage(SrcImage, OldExtent, RescaleBuffer, CurrentExtent, Entry->Usage, SrcFormatInfo.ChannelCount, AlphaThreshold, AlphaCoverage);
                    if (DoAlphaCoverageRescale)
                    {
                        RescaleAlphaForCoverage(CurrentExtent, RescaleBuffer, AlphaThreshold, AlphaCoverage);
                    }
                    SrcImage = RescaleBuffer;
                }

                // TODO(boti): Handle 2x2 and 1x1 mips correctly
                for (u32 Y = 0; Y < CurrentExtent.Y; Y += 4)
                {
                    for (u32 X = 0; X < CurrentExtent.X; X += 4)
                    {
                        u8 Block[16*4];
                        u8* DstAt = Block;
                        for (u32 YY = Y; YY < (Y + 4); YY++)
                        {
                            u8* SrcAt = SrcImage + (X + YY * CurrentExtent.X) * SrcFormatInfo.ChannelCount;
                            for (u32 XX = X; XX < (X + 4); XX++)
                            {
                                switch (TargetFormat)
                                {
                                    case Format_BC1_RGB_UNorm:
                                    case Format_BC1_RGB_SRGB:
                                    case Format_BC3_UNorm:
                                    case Format_BC3_SRGB:
                                    {
                                        *DstAt++ = SrcAt[0];
                                        *DstAt++ = SrcAt[1];
                                        *DstAt++ = SrcAt[2];
                                        *DstAt++ = (SrcFormatInfo.ChannelCount == 4) ? SrcAt[3] : 0xFFu;
                                    } break;
                                    case Format_BC4_UNorm:
                                    {
                                        *DstAt++ = SrcAt[ChannelsOfInterest[0]];
                                    } break;
                                    case Format_BC5_UNorm:
                                    {
                                        *DstAt++ = SrcAt[ChannelsOfInterest[0]];
                                        *DstAt++ = SrcAt[ChannelsOfInterest[1]];
                                    } break;
                                    InvalidDefaultCase;
                                }

                                SrcAt += SrcFormatInfo.ChannelCount;
                            }
                        }

                        BCCompressBlock(DstBlock, Block, TargetFormat);
                        DstBlock += DstBlockSize;
                    }
                }
            }

            //
            // Write to disk
            //
            Result = WriteEntireFile(Entry->DstPath.Path, sizeof(dds_file) + TotalMipChainSize, DstFile);
        }
        else
        {
            Result = 0;
        }
    }
    else
    {
        Result = 0;
    }

    RestoreArena(Arena, Checkpoint);
    return(Result);
}

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#include "stb/stb_image_resize2.h"
#pragma clang diagnostic pop

#define STB_DXT_IMPLEMENTATION
#include "stb/stb_dxt.h"

#include <cstdio>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <Windows.h>

#undef LoadImage

internal buffer LoadEntireFile(const char* Path, memory_arena* Arena)
{
    buffer Result = {};

    HANDLE File = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize = {};
        GetFileSizeEx(File, &FileSize);

        memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Arena);
        if (void* Memory = PushSize_(Arena, 0, (umm)FileSize.QuadPart, 64))
        {
            DWORD BytesRead = 0;
            if (ReadFile(File, Memory, (DWORD)FileSize.QuadPart, &BytesRead, nullptr))
            {
                Result.Size = (umm)FileSize.QuadPart;
                Result.Data = Memory;
            }
            else
            {
                RestoreArena(Arena, Checkpoint);
            }
        }

        CloseHandle(File);
    }

    return(Result);
}

internal b32 WriteEntireFile(const char* Path, umm Size, void* Memory)
{
    b32 Result = false;

    HANDLE File = CreateFileA(Path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten = 0;
        if (WriteFile(File, Memory, (DWORD)Size, &BytesWritten, nullptr))
        {
            Result = ((umm)BytesWritten == Size);
        }

        CloseHandle(File);
    }

    return(Result);
}

// TODO(boti): Arg handling cleanup + help/man
// - Arg to specify what data is contained in which channel
int main(int ArgCount, char** Args)
{
    if (ArgCount < 3)
    {
        fprintf(stdout, "Usage: lbasset <dst-directory> <src-file>");
        return(1);
    }

    // NOTE(boti): There's a separate enum for the arguments, because we want the user
    // to be able to pass in roughness only (or metallic only) textures
    // and have the asset processor set up the proper swizzling
    enum image_type : u32
    {
        Image_Undefined = 0,

        Image_Albedo,
        Image_Normal,
        Image_Roughness,
        Image_RoughnessMetallic,
        Image_Occlusion,

        Image_Count,
    };

    image_type ImageType = Image_Undefined;

    if      (strcmp(Args[1], "--albedo")    == 0) ImageType = Image_Albedo;
    else if (strcmp(Args[1], "--normal")    == 0) ImageType = Image_Normal;
    else if (strcmp(Args[1], "--rome")      == 0) ImageType = Image_RoughnessMetallic;
    else if (strcmp(Args[1], "--roughness") == 0) ImageType = Image_Roughness;
    else if (strcmp(Args[1], "--occlusion") == 0) ImageType = Image_Occlusion;

    filepath DstDirectory;
    MakeFilepathFromZ(&DstDirectory, Args[ArgCount - 2]);
    filepath SrcFilePath;
    MakeFilepathFromZ(&SrcFilePath, Args[ArgCount - 1]);

    memory_arena Arena_ = {};
    {
        constexpr umm ArenaSize = GiB(1);
        void* ArenaMemory = VirtualAlloc(nullptr, ArenaSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        if (ArenaMemory)
        {
            Arena_ = InitializeArena(ArenaSize, ArenaMemory);
        }
        else
        {
            fprintf(stderr, "Failed to allocate scratch memory");
            return(1);
        }
    }
    memory_arena* Arena = &Arena_;

    u32 ImagesToProcessCount = 0;
    image_entry* ImagesToProcess = nullptr;

    if (ImageType)
    {
        ImagesToProcessCount = 1;
        ImagesToProcess = PushArray(Arena, MemPush_Clear, image_entry, ImagesToProcessCount);

        image_entry* Entry = ImagesToProcess;
        switch (ImageType)
        {
            case Image_Albedo:
            {
                Entry->Usage = ImageUsage_Albedo;
            } break;
            case Image_Normal:
            {
                Entry->Usage = ImageUsage_Normal;
            } break;
            case Image_Roughness:
            {
                Entry->Usage = ImageUsage_RoMe;
                Entry->RoughnessSwizzle = Swizzle_R;
                Entry->MetallicSwizzle = Swizzle_Zero;
            } break;
            case Image_RoughnessMetallic:
            {
                Entry->Usage = ImageUsage_RoMe;
                Entry->RoughnessSwizzle = Swizzle_R;
                Entry->MetallicSwizzle = Swizzle_B;
            } break;
            case Image_Occlusion:
            {
                Entry->Usage = ImageUsage_Occlusion;
            } break;
            InvalidDefaultCase;
        }
        Entry->SrcPath = SrcFilePath;
        Entry->DstPath = DstDirectory;
        OverwriteNameAndExtension(&Entry->DstPath, { Entry->SrcPath.NameCount, Entry->SrcPath.Path + Entry->SrcPath.NameOffset });
        FindFilepathExtensionAndName(&Entry->DstPath, 0);
        OverwriteExtension(&Entry->DstPath, ".dds");
    }
    else
    {
        buffer SourceFile = LoadEntireFile(SrcFilePath.Path, Arena);

        // TODO(boti): Figure out a way to handle glTF occlusion-roughness-metallic textures properly:
        // We want to store the occlusion separately from RoMe, which means we have to find a different name if occlusion exists
        if (json_element* Root = ParseJSON(SourceFile.Data, SourceFile.Size, Arena))
        {
            gltf GLTF = {};
            if (ParseGLTF(&GLTF, Root, Arena))
            {
                image_usage_flags* ImageUsageFlags = PushArray(Arena, MemPush_Clear, image_usage_flags, GLTF.ImageCount);

                for (u32 MaterialIndex = 0; MaterialIndex < GLTF.MaterialCount; MaterialIndex++)
                {
                    gltf_material* Material = GLTF.Materials + MaterialIndex;
                    #define AddUsage(tex, usage) \
                    if (tex.TextureIndex != U32_MAX) ImageUsageFlags[tex.TextureIndex] |= usage

                    AddUsage(Material->BaseColorTexture,            ImageUsage_Albedo);
                    AddUsage(Material->MetallicRoughnessTexture,    ImageUsage_RoMe);
                    AddUsage(Material->NormalTexture,               ImageUsage_Normal);
                    AddUsage(Material->OcclusionTexture,            ImageUsage_Occlusion);
                    AddUsage(Material->TransmissionTexture,         ImageUsage_Transmission);
                    AddUsage(Material->EmissiveTexture,             ImageUsage_Emission);
                    #undef AddUsage
                }

                ImagesToProcess = PushArray(Arena, MemPush_Clear, image_entry, GLTF.ImageCount);
                for (u32 ImageIndex = 0; ImageIndex < GLTF.ImageCount; ImageIndex++)
                {
                    gltf_image* Image = GLTF.Images + ImageIndex;
                    image_usage_flags Usage = ImageUsageFlags[ImageIndex];

                    u32 UsageCount = CountSetBits(Usage);
                    if (UsageCount == 1)
                    {
                        image_entry* Entry = ImagesToProcess + ImagesToProcessCount++;

                        Entry->Usage = Usage;
                        // NOTE(boti): Set up glTF ORM channel order
                        if (Usage == ImageUsage_RoMe)
                        {
                            Entry->RoughnessSwizzle = Swizzle_G;
                            Entry->MetallicSwizzle = Swizzle_B;
                        }
                        Entry->SrcPath = SrcFilePath;
                        OverwriteNameAndExtension(&Entry->SrcPath, Image->URI);
                        Entry->DstPath = DstDirectory;
                        OverwriteNameAndExtension(&Entry->DstPath, Image->URI);
                        FindFilepathExtensionAndName(&Entry->DstPath, 0);
                        OverwriteExtension(&Entry->DstPath, ".dds");
                    }
                    else if (UsageCount == 0)
                    {
                        fprintf(stderr, "Skipping unused image %.*s\n", (u32)Image->URI.Length, Image->URI.String);
                    }
                    else
                    {
                        fprintf(stderr, "[WARNING] lbasset can't handle textures with multiple usage types! (skipping %.*s)\n",
                                (u32)Image->URI.Length, Image->URI.String);
                    }
                }
            }
            else
            {
                fprintf(stderr, "Failed to parse glTF\n");
            }
        }
        else
        {
            fprintf(stderr, "Failed to parse JSON (%s)\n", SrcFilePath.Path);
        }
    }

    // TODO(boti): Multithreading (if we even want...)
    fprintf(stdout, "Processing %u images...\n", ImagesToProcessCount);
    for (u32 ImageIndex = 0; ImageIndex < ImagesToProcessCount; ImageIndex++)
    {
        image_entry* Entry = ImagesToProcess + ImageIndex;
        if (ProcessImage(Arena, Entry))
        {
            fprintf(stdout, "%s -> %s\n", Entry->SrcPath.Path, Entry->DstPath.Path);
        }
        else
        {
            fprintf(stderr, "Failed to process image %s\n", Entry->SrcPath.Path);
        }
    }

    fprintf(stdout, "Done!\n");

    return(0);
}

#include <LadybugLib/LBLibBuild.cpp>