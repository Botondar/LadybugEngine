#include "image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable:4244)
#include <stb/stb_image.h>
#pragma warning(pop)

internal loaded_image LoadTIFF(memory_arena* Arena, buffer File);
internal loaded_image LoadDDS(memory_arena* Arena, buffer File);

lbfn image_file_type 
DetermineImageFileType(buffer FileData)
{
    image_file_type Result = ImageFile_Undefined;

    for (u32 Candidate = 1;
        (Result == ImageFile_Undefined) && (Candidate < ImageFile_Count);
        Candidate++)
    {
        switch (Candidate)
        {
            case ImageFile_BMP:
            {
                u8 RefTag[2] = { 0x42u, 0x4Du };
                if (FileData.Size >= sizeof(RefTag))
                {
                    if (memcmp(FileData.Data, RefTag, sizeof(RefTag)) == 0)
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
            case ImageFile_TIF:
            {
                if (FileData.Size >= 8)
                {
                    u16 ByteOrder = *(u16*)OffsetPtr(FileData.Data, 0);
                    if (ByteOrder == 0x4949u || ByteOrder == 0x4D4Du)
                    {
                        u16 Tag = *(u16*)OffsetPtr(FileData.Data, 2);
                        if (ByteOrder == 0x4D4Du)
                        {
                            Tag = (Tag << 8) | (Tag >> 8);
                        }
                        if (Tag == 42)
                        {
                            Result = (image_file_type)Candidate;
                        }
                    }
                }
            } break;
            case ImageFile_PNG:
            {
                u8 RefTag[8] = { 137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u };
                if (FileData.Size >= sizeof(RefTag))
                {
                    if (memcmp(FileData.Data, RefTag, sizeof(RefTag)) == 0)
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
            case ImageFile_JPG:
            {
                // TODO(boti): This tag consists of the JFIF begin marker and JPEG Start-Of-Image bytes, 
                // read the spec more carefully to figure out if we're doing the correct thing here
                u8 RefTag[2] = { 0xFFu, 0xD8u };
                if (FileData.Size >= sizeof(RefTag))
                {
                    if (memcmp(FileData.Data, RefTag, sizeof(RefTag)) == 0)
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
            case ImageFile_DDS:
            {
                if (FileData.Size >= DDSHeaderSize)
                {
                    dds_header* Header = (dds_header*)FileData.Data;
                    // TODO(boti): We probably could (and should) check more fields here to ensure that the file is actually .dds
                    dds_flags RequiredFlags = DDSFlag_Caps|DDSFlag_Height|DDSFlag_Width|DDSFlag_PixelFormat;
                    if ((Header->HeaderSize == DDSHeaderSize) && ((Header->Flags & RequiredFlags) == RequiredFlags))
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
        }
    }
    return(Result);
}

lbfn loaded_image
LoadImage(memory_arena* Arena, buffer FileData)
{
    loaded_image Result = {};

    image_file_type FileType = DetermineImageFileType(FileData);
    switch (FileType)
    {
        case ImageFile_BMP:
        case ImageFile_PNG:
        case ImageFile_JPG:
        {
            int Width, Height, ChannelCount;
            u8* ImageData = stbi_load_from_memory((const u8*)FileData.Data, (int)FileData.Size, &Width, &Height, &ChannelCount, 0);
            if (ImageData)
            {
                umm PixelCount = (umm)Width * (umm)Height;

                // Wasteful copy to get the same memory management guarantees when using stb_image
                umm ImageSize = PixelCount * ChannelCount;
                Result.Data = PushSize_(Arena, 0, ImageSize, 64);
                if (Result.Data)
                {
                    Result.Extent = { (u32)Width, (u32)Height };
                    switch(ChannelCount)
                    {
                        case 1: Result.Format = Format_R8_UNorm;        break;
                        case 2: Result.Format = Format_R8G8_UNorm;      break;
                        case 3: Result.Format = Format_R8G8B8_UNorm;    break;
                        case 4: Result.Format = Format_R8G8B8A8_UNorm;  break;
                        default:
                        {
                            // TODO(boti): Error case
                        } break;
                    }
                    memcpy(Result.Data, ImageData, ImageSize);
                }

                stbi_image_free(ImageData);
            }
        } break;
        case ImageFile_TIF:
        {
            Result = LoadTIFF(Arena, FileData);
        } break;
        case ImageFile_DDS:
        {
            Result = LoadDDS(Arena, FileData);
        } break;
        default:
        {
            // TODO(boti): logging
        } break;
    }

    return(Result);
}

internal loaded_image LoadDDS(memory_arena* Arena, buffer File)
{
    loaded_image Result = {};

    dds_header* Header          = (dds_header*)File.Data;
    dds_header_dx10* DX10Header = nullptr;
    void* ImageData             = nullptr;
    if (Header->PixelFormat.FourCC == DDS_DX10)
    {
        DX10Header = (dds_header_dx10*)OffsetPtr(File.Data, sizeof(dds_header));
        ImageData = OffsetPtr(File.Data, sizeof(dds_header) + sizeof(dds_header_dx10));
    }

    if (DX10Header && ImageData)
    {
        // TODO(boti): Handle pitch/linear size/ depth properly
        if ((Header->Flags & DDSFlag_Depth) || (DX10Header->Dimension != DDSDim_Texture2D))
        {
            UnimplementedCodePath;
        }
        
        v3u Extent      = { Header->Width, Header->Height, 1 };
        u32 ArrayCount  = DX10Header->ArrayCount;
        u32 MipCount    = 1;
        if (DX10Header->ResourceFlags & DDSResourceFlag_TextureCube)
        {
            ArrayCount *= 6; // DDS, you silly goose...
        }
        if (Header->Flags & DDSFlag_MipMapCount)
        {
            MipCount = Header->MipMapCount;
        }

        // TODO(boti): Don't use untrusted field for indexing
        format Format = DXGIFormatTable[DX10Header->Format];
        format_info FormatInfo = FormatInfoTable[Format];

        umm ImageDataSize = GetMipChainSize(Extent.X, Extent.Y, MipCount, ArrayCount, FormatInfo);
        if (sizeof(dds_header) + sizeof(dds_header_dx10) + ImageDataSize <= File.Size)
        {
            Result.Extent   = { Extent.X, Extent.Y };
            Result.Format   = Format;
            Result.Data     = PushSize_(Arena, 0, ImageDataSize, 64);
            memcpy(Result.Data, ImageData, ImageDataSize);
        }
        else
        {
            UnhandledError("Corrupt DDS file (image data out of bounds)");
        }
    }
    else
    {
        UnhandledError("Unsupported DDS file (missing DX10 header)");
    }

    return(Result);
}

internal loaded_image LoadTIFF(memory_arena* Arena, buffer File)
{
    loaded_image Result = {};

    static constexpr u16 ByteOrderLittleEndian  = 0x4949u;
    static constexpr u16 ByteOrderBigEndian     = 0x4D4Du;

    enum tiff_entry_type : u16
    {
        TIFFEntry_Undefined = 0,

        TIFFEntry_U8        = 1,
        TIFFEntry_ASCII     = 2, // NOTE(boti): NULL terminator included in count (of tiff_ifd_entry)
        TIFFEntry_U16       = 3,
        TIFFEntry_U32       = 4,
        TIFFEntry_U32x2     = 5, // NOTE(boti): Num/denom pair
        TIFFEntry_S8        = 6,
        TIFFEntry_X8        = 7, // NOTE(boti): 8-bit undefined
        TIFFEntry_S16       = 8,
        TIFFEntry_S32       = 9,
        TIFFEntry_S32x2     = 10, // NOTE(boti): Num/denom pair
        TIFFEntry_F32       = 11,
        TIFFEntry_F64       = 12,

        TIFFEntry_Count,
    };

    enum tiff_entry_tag : u16
    {
        TIFF_ExtentX            = 0x100u,
        TIFF_ExtentY            = 0x101u,
        TIFF_BitsPerSample      = 0x102u,
        TIFF_Compression        = 0x103u,
        TIFF_SamplesPerPixel    = 0x115u,

        // NOTE(boti): Image may be broken into "strips", which have to be assembled
        TIFF_StripByteOffsets           = 0x111u, // NOTE(boti): Per strip
        TIFF_StripExtentY               = 0x116u, // NOTE(boti): Single value that applies to all strips (except the last one, as StripExtentY*StripCount may not equal ExtentY)
        TIFF_StripCompressedByteCount   = 0x117u, // NOTE(boti): Per strip
    };

#pragma pack(push, 1)
    struct tiff_header
    {
        u16 ByteOrder;
        u16 Tag;
        u32 ImageFileDirectoryOffset;
    };

    struct tiff_ifd_entry
    {
        tiff_entry_tag  Tag;
        tiff_entry_type Type;
        u32             Count;
        u32             OffsetOrValue;
    };

    struct tiff_ifd
    {
        u16 EntryCount;
        tiff_ifd_entry Entries[1]; // NOTE(boti): At least one required by spec
    };

#pragma pack(pop)

    const u32 TiffStrideTable[TIFFEntry_Count] =
    {
        [TIFFEntry_Undefined]   = 0,

        [TIFFEntry_U8]          = 1,
        [TIFFEntry_ASCII]       = 1,
        [TIFFEntry_U16]         = 2,
        [TIFFEntry_U32]         = 4,
        [TIFFEntry_U32x2]       = 8,
        [TIFFEntry_S8]          = 1,
        [TIFFEntry_X8]          = 1,
        [TIFFEntry_S16]         = 2,
        [TIFFEntry_S32]         = 4,
        [TIFFEntry_S32x2]       = 8,
        [TIFFEntry_F32]         = 4,
        [TIFFEntry_F64]         = 8,
    };

    enum tiff_compression : u16
    {
        TIFFCompression_Undefined = 0,
        TIFFCompression_Uncompressed = 1,
    };

    static_assert(sizeof(tiff_header) == 8);
    static_assert(sizeof(tiff_ifd_entry) == 12);

    Assert(File.Size >= sizeof(tiff_header));
    tiff_header* Header = (tiff_header*)File.Data;

    Assert((Header->ByteOrder == ByteOrderLittleEndian) || (Header->ByteOrder == ByteOrderBigEndian));
    b32 IsBigEndian = (Header->ByteOrder == ByteOrderBigEndian);

    u16 Tag = IsBigEndian ? EndianFlip16(Header->Tag) : Header->Tag;
    Assert(Tag == 42);

    u32 ImageFileDirectoryOffset = IsBigEndian ? EndianFlip32(Header->ImageFileDirectoryOffset) : Header->ImageFileDirectoryOffset;
    if (ImageFileDirectoryOffset + sizeof(tiff_ifd) >= File.Size)
    {
        UnhandledError("Corrupt TIFF file (ImageFileDirectory offset out of bounds)");
    }

    tiff_ifd* ImageFileDirectory = (tiff_ifd*)OffsetPtr(File.Data, ImageFileDirectoryOffset);
    u16 EntryCount = IsBigEndian ? EndianFlip16(ImageFileDirectory->EntryCount) : ImageFileDirectory->EntryCount;

    Assert(EntryCount >= 1);
    if (ImageFileDirectoryOffset + EntryCount * sizeof(tiff_ifd_entry) + sizeof(u16) >= File.Size)
    {
        UnhandledError("Corrupt TIFF file (ImageFileDirectory netries out of bounds)");
    }

    v2u Extent = {};
    constexpr u32 MaxChannelCount = 4;
    u32 ChannelCount = 0;
    u32 ChannelCountFromBitDepth = 0;
    u32 ChannelBitDepths[MaxChannelCount] = {};
    u32 Compression = 0;

    struct tiff_strip
    {
        u32 SourceByteOffset;
        u32 CompressedByteCount;
    };

    u32 StripExtentY = 0;
    constexpr u32 MaxStripCount = 512; // TODO(boti): We'll want to be able to handle this dynamically
    u32 StripCount = 0;
    tiff_strip Strips[MaxStripCount];

    for (u32 EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++)
    {
        tiff_ifd_entry Entry = ImageFileDirectory->Entries[EntryIndex];
        if (IsBigEndian)
        {
            Entry.Tag           = (tiff_entry_tag)EndianFlip16((u16)Entry.Tag);
            Entry.Type          = (tiff_entry_type)EndianFlip16((u16)Entry.Type);
            Entry.Count         = EndianFlip32(Entry.Count);
            Entry.OffsetOrValue = EndianFlip32(Entry.OffsetOrValue); // TODO(boti): This isn't correct, if the value actually fits there are additional packing rules
        }

        if (Entry.Type == TIFFEntry_Undefined || Entry.Type >= TIFFEntry_Count)
        {
            continue;
        }

        switch (Entry.Tag)
        {
            case TIFF_ExtentX:
            {
                Verify(Entry.Type == TIFFEntry_U16 || Entry.Type == TIFFEntry_U32);
                Verify(Entry.Count == 1);
                Extent.X = Entry.OffsetOrValue;
            } break;
            case TIFF_ExtentY:
            {
                Verify(Entry.Type == TIFFEntry_U16 || Entry.Type == TIFFEntry_U32);
                Verify(Entry.Count == 1);
                Extent.Y = Entry.OffsetOrValue;
            } break;
            case TIFF_BitsPerSample:
            {
                Verify(Entry.Type == TIFFEntry_U16);
                ChannelCountFromBitDepth = Entry.Count;
                if (Entry.Count == 1)
                {
                    ChannelBitDepths[0] = Entry.OffsetOrValue;
                }
                else if (Entry.Count == 3)
                {
                    if (Entry.OffsetOrValue + Entry.Count * sizeof(u16) >= File.Size)
                    {
                        UnhandledError("Corrupt TIFF file (bits per sample data out of bounds)");
                    }

                    u16* ChannelAt = (u16*)OffsetPtr(File.Data, Entry.OffsetOrValue);
                    for (u32 ChannelIndex = 0; ChannelIndex < Entry.Count; ChannelIndex++)
                    {
                        u16 Value = *ChannelAt++;
                        ChannelBitDepths[ChannelIndex] = IsBigEndian ? EndianFlip16(Value) : Value;
                    }
                }
                else
                {
                    UnimplementedCodePath;
                }
            } break;
            case TIFF_Compression:
            {
                Verify(Entry.Type == TIFFEntry_U16);
                Verify(Entry.Count == 1);
                Compression = Entry.OffsetOrValue;
            } break;
            case TIFF_SamplesPerPixel:
            {
                Verify(Entry.Type == TIFFEntry_U16);
                Verify(Entry.Count == 1);
                ChannelCount = Entry.OffsetOrValue;
            } break;
            case TIFF_StripByteOffsets:
            {
                Verify(Entry.Type == TIFFEntry_U16 || Entry.Type == TIFFEntry_U32);
                u32 Stride = TiffStrideTable[Entry.Type];
                u32 TotalSize = Entry.Count * Stride;

                if (Entry.Count > MaxStripCount)
                {
                    UnimplementedCodePath;
                }

                if (StripCount)
                {
                    Verify(Entry.Count == StripCount);
                }
                else
                {
                    StripCount = Entry.Count;
                }

                if (Entry.Count == 1)
                {
                    Strips[0].SourceByteOffset = Entry.OffsetOrValue;
                }
                else if (TotalSize > sizeof(Entry.OffsetOrValue))
                {
                    if (Entry.OffsetOrValue + TotalSize > File.Size)
                    {
                        UnhandledError("Corrupt TIFF file (strip byte counts out of bounds)");
                    }

                    void* SrcAt = OffsetPtr(File.Data, Entry.OffsetOrValue);
                    for (u32 StripIndex = 0; StripIndex < Entry.Count; StripIndex++)
                    {
                        tiff_strip* Strip = Strips + StripIndex;
                        Strip->SourceByteOffset = 0;
                        if (IsBigEndian)
                        {
                            EndianFlip(&Strip->SourceByteOffset, SrcAt, Stride);
                        }
                        else
                        {
                            memcpy(&Strip->SourceByteOffset, SrcAt, Stride);
                        }
                        SrcAt = OffsetPtr(SrcAt, Stride);
                    }
                }
                else
                {
                    // TODO(boti): Implement support for packing 2 U16s into OffsetOrValue.
                    UnimplementedCodePath;
                }
            } break;
            case TIFF_StripExtentY:
            {
                Verify(Entry.Type == TIFFEntry_U16 || Entry.Type == TIFFEntry_U32);
                Verify(Entry.Count == 1);
                StripExtentY = Entry.OffsetOrValue;
            } break;
            case TIFF_StripCompressedByteCount:
            {
                Verify(Entry.Type == TIFFEntry_U16 || Entry.Type == TIFFEntry_U32);
                u32 Stride = TiffStrideTable[Entry.Type];
                u32 TotalSize = Entry.Count * Stride;

                if (Entry.Count > MaxStripCount)
                {
                    UnimplementedCodePath;
                }

                if (StripCount)
                {
                    Verify(StripCount == Entry.Count);
                }
                else
                {
                    StripCount = Entry.Count;
                }
                
                if (Entry.Count == 1)
                {
                    Strips[0].CompressedByteCount = Entry.OffsetOrValue;
                }
                else if (TotalSize > sizeof(Entry.OffsetOrValue))
                {
                    if (Entry.OffsetOrValue + TotalSize > File.Size)
                    {
                        UnhandledError("Corrupt TIFF file (strip byte counts out of bounds)");
                    }

                    void* SrcAt = OffsetPtr(File.Data, Entry.OffsetOrValue);
                    for (u32 StripIndex = 0; StripIndex < Entry.Count; StripIndex++)
                    {
                        tiff_strip* Strip = Strips + StripIndex;
                        Strip->CompressedByteCount = 0;
                        if (IsBigEndian)
                        {
                            EndianFlip(&Strip->CompressedByteCount, SrcAt, Stride);
                        }
                        else
                        {
                            memcpy(&Strip->CompressedByteCount, SrcAt, Stride);
                        }
                        SrcAt = OffsetPtr(SrcAt, Stride);
                    }
                }
                else
                {
                    // TODO(boti): Implement support for packing 2 U16s into OffsetOrValue.
                    UnimplementedCodePath;
                }
            } break;
        }
    }

    if (ChannelCount == 0)
    {
        ChannelCount = ChannelCountFromBitDepth;
    }

    if (ChannelCount != ChannelCountFromBitDepth)
    {
        UnhandledError("Corrupt TIFF file (channel count mismatch)");
    }

    Verify(Extent.X != 0 && Extent.Y != 0);
    Verify(ChannelCount != 0);
    Verify(StripCount != 0);
    Verify(StripExtentY != 0);

    if (Compression == TIFFCompression_Uncompressed)
    {
        u32 BitDepth = ChannelBitDepths[0];
        for (u32 ChannelIndex = 1; ChannelIndex < ChannelCount; ChannelIndex++)
        {
            if (BitDepth != ChannelBitDepths[ChannelIndex])
            {
                UnimplementedCodePath;
            }
        }
        if ((BitDepth % 8) != 0)
        {
            UnimplementedCodePath;
        }

        umm ImageSize = (umm)Extent.X * Extent.Y * ChannelCount * 1;
        Result.Extent = Extent;

        // HACK(boti): See below, we're doing the conversion here
        switch (ChannelCount)
        {
            case 1: Result.Format = Format_R8_UNorm;        break;
            case 2: Result.Format = Format_R8G8_UNorm;      break;
            case 3: Result.Format = Format_R8G8B8_UNorm;    break;
            case 4: Result.Format = Format_R8G8B8A8_UNorm;  break;
            default:
            {
                UnhandledError("Invalid TIFF channel count");
            } break;
        }
        Result.Data = PushArray(Arena, 0, u8, ImageSize);

        for (u32 StripIndex = 0; StripIndex < StripCount; StripIndex++)
        {
            tiff_strip* Strip = Strips + StripIndex;
            if (Strip->SourceByteOffset + Strip->CompressedByteCount > File.Size)
            {
                UnhandledError("Corrupt TIFF file | Strip data out of bounds");
            }

            u32 MinY = StripIndex * StripExtentY;
            u32 MaxY = Min(MinY + StripExtentY, Extent.Y);

            // TODO(boti): There's an out of bounds error here, we need to check if the strip byte count
            // lines up with the image extent
            u8* DstAt = (u8*)OffsetPtr(Result.Data, MinY * Extent.X * ChannelCount * 1);
            void* SrcAt = OffsetPtr(File.Data, Strip->SourceByteOffset);
            for (u32 Y = MinY; Y < MaxY; Y++)
            {
                for (u32 X = 0; X < Extent.X; X++)
                {
                    for (u32 ChannelIndex = 0; ChannelIndex < ChannelCount; ChannelIndex++)
                    {
                        u8 Value = 0;
                        switch (BitDepth)
                        {
                            case 8:
                            {
                                Value = *(u8*)SrcAt;
                            } break;
                            case 16:
                            {
                                u16 Value16 = *(u16*)SrcAt;
                                if (IsBigEndian)
                                {
                                    Value16 = EndianFlip16(Value16);
                                }
                                Value = (u8)(Value16 >> 8);
                            } break;
                            InvalidDefaultCase;
                        }

                        *DstAt++ = Value;
                        SrcAt = OffsetPtr(SrcAt, BitDepth / 8);
                    }
                }
            }
        }
    }
    else
    {
        UnimplementedCodePath;
    }

    // Y-flip
    for (u32 Y = 0; Y < Result.Extent.Y / 2; Y++)
    {
        u32 FlipY = Result.Extent.Y - Y - 1;
        u32 ByteCount = Result.Extent.X * ChannelCount * 1;
        
        u8* AAt = (u8*)OffsetPtr(Result.Data, Y * Result.Extent.X * ChannelCount * 1);
        u8* BAt = (u8*)OffsetPtr(Result.Data, FlipY * Result.Extent.X * ChannelCount * 1);
        while (ByteCount--)
        {
            u8 Temp = *AAt;
            *AAt++ = *BAt;
            *BAt++ = Temp;
        }
    }

    return(Result);
}

lbfn f32 CalculateAlphaCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 AlphaScale)
{
    f32 Coverage = 0.0f;

    u32 Count = Extent.X * Extent.Y;
    f32 Norm = 1.0f / (f32)Count;

    u8* At = Texels + 3;
    while (Count--)
    {
        f32 Alpha = Clamp(AlphaScale * (f32)(*At) / 255.0f, 0.0f, 1.0f);
        At += 4;

        if (Alpha >= AlphaThreshold)
        {
            Coverage += 1.0f;
        }
    }

    Coverage *= Norm;
    return(Coverage);
}

lbfn f32 CalculateAlphaScaleForCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 TargetCoverage)
{
    f32 MinScale = 0.0f;
    f32 MaxScale = 4.0f;
    f32 Scale = 1.0f;

    for (u32 Step = 0; Step < 8; Step++)
    {
        f32 CurrentCoverage = CalculateAlphaCoverage(Extent, Texels, AlphaThreshold, Scale);

        if      (CurrentCoverage < TargetCoverage) MinScale = Scale;
        else if (CurrentCoverage > TargetCoverage) MaxScale = Scale;
        else    break;

        Scale = 0.5f * (MinScale + MaxScale);
    }

    return(Scale);
}

lbfn f32 ScaleAlphaForCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 TargetCoverage)
{
    f32 Scale = CalculateAlphaScaleForCoverage(Extent, Texels, AlphaThreshold, TargetCoverage);

    u32 Count = Extent.X * Extent.Y;
    u8* At = Texels + 3;
    while (Count--)
    {
        f32 Alpha = (f32)(*At) / 255.0f;
        Alpha = Clamp(Scale * Alpha, 0.0f, 1.0f);
        u8 Alpha8 = (u8)Round(255.0f * Alpha);
        *At = Alpha8;
        At += 4;
    }

    return(Scale);
}