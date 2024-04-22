//
// Internal interface
//
internal mesh_data CreateCubeMesh(memory_arena* Arena);
internal mesh_data CreateSphereMesh(memory_arena* Arena);
internal mesh_data CreateArrowMesh(memory_arena* Arena);

internal loaded_image LoadTIFF(memory_arena* Arena, buffer File);

//
// Utility
//

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
                    Result.ChannelCount = (u32)ChannelCount;
                    Result.BitDepthPerChannel = 8;
                    memcpy(Result.Data, ImageData, ImageSize);
                }

                stbi_image_free(ImageData);
            }
        } break;
        case ImageFile_TIF:
        {
            Result = LoadTIFF(Arena, FileData);
        } break;
        default:
        {
            // TODO(boti): logging
        } break;
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
        TIFF_StripDecompressedByteCount = 0x117u, // NOTE(boti): Per strip
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
    u32 DataOffset = 0;
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
                if (Entry.Count == 1)
                {
                    DataOffset = Entry.OffsetOrValue;
                }
                else
                {
                    UnimplementedCodePath;
                }
            } break;
            case TIFF_StripExtentY:
            {
                // TODO(boti): Currently ignored because we only support single strip images
            } break;
            case TIFF_StripDecompressedByteCount:
            {
            } break;
        }
    }

    if (ChannelCount == 0)
    {
        ChannelCount = ChannelCountFromBitDepth;
    }

    if (ChannelCount != ChannelCountFromBitDepth)
    {
        UnhandledError("TIFF is weird");
    }

    Verify(Extent.X != 0 && Extent.Y != 0);
    Verify(ChannelCount != 0);
    Verify(DataOffset != 0);

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

        umm ImageSize = (umm)Extent.X * Extent.Y * ChannelCount * (BitDepth / 8);
        if (DataOffset + ImageSize <= File.Size)
        {
            Result.Data = PushArray(Arena, 0, u8, ImageSize);
            if (Result.Data)
            {
                Result.Extent = Extent;
                Result.ChannelCount = ChannelCount;
                Result.BitDepthPerChannel = 8; // HACK(boti): See below, we're doing the conversion here
                umm DstSize = (umm)Extent.X * Extent.Y * ChannelCount * (Result.BitDepthPerChannel / 8);
                u8* DstAt = (u8*)Result.Data;
                u8* SrcImage = (u8*)OffsetPtr(File.Data, DataOffset);
                u8* SrcAt = SrcImage;
                for (u32 Y = 0; Y < Extent.Y; Y++)
                {
                    for (u32 X = 0; X < Extent.X; X++)
                    {
                        // HACK(boti): We know the usage we're gonna be called with from process entry...
                        if (BitDepth == 16 && ChannelCount == 3)
                        {
                            u16 R = *(u16*)SrcAt;
                            SrcAt += 2;
                            u16 G = *(u16*)SrcAt;
                            SrcAt += 2;
                            u16 B = *(u16*)SrcAt;
                            SrcAt += 2;

                            if (IsBigEndian)
                            {
                                R = EndianFlip16(R);
                                G = EndianFlip16(G);
                                B = EndianFlip16(B);
                            }

                            *DstAt++ = (u8)(R >> 8);
                            *DstAt++ = (u8)(G >> 8);
                            *DstAt++ = (u8)(B >> 8);
                        }
                        else
                        {
                            UnimplementedCodePath;
                        }
                    }
                }
            }
        }
        else
        {
            UnhandledError("Corrupt TIFF file (image data out of bounds)");
        }
    }
    else
    {
        UnimplementedCodePath;
    }

    return(Result);
}

//
// Texture queue
//
lbfn void AssetThread(void* Param)
{
    assets* Assets = (assets*)Param;
    texture_queue* Queue = &Assets->TextureQueue;

    for (;;)
    {
        b32 WeShouldSleep = ProcessEntry(Queue);
        while (!WeShouldSleep)
        {
            WeShouldSleep = ProcessEntry(Queue);
        }
        u32 WaitMS = 0xFFFFFFFFu;
        Platform.WaitForSemaphore(Queue->Semaphore, WaitMS);
    }
}

lbfn umm GetNextEntryOffset(texture_queue* Queue, umm TotalSize, umm Begin)
{
    umm End = Begin + TotalSize;
    if ((End % Queue->RingBufferSize) < (Begin % Queue->RingBufferSize))
    {
        Begin = Align(Begin, Queue->RingBufferSize);
    }
    return(Begin);
}

lbfn void AddEntry(texture_queue* Queue, texture* Texture, texture_type Type, b32 AlphaEnabled, const filepath* Path)
{
    // TODO(boti): Spin-wait here?
    if (Queue->CompletionGoal - Queue->CompletionCount < Queue->MaxEntryCount)
    {
        u32 Index = AtomicLoadAndIncrement(&Queue->CompletionGoal) % Queue->MaxEntryCount;
        Queue->Entries[Index] =
        {
            .Texture = Texture,
            .TextureType = Type,
            .AlphaEnabled = AlphaEnabled,
            .Path = *Path,
            .ReadyToTransfer = false,
        };
        Platform.ReleaseSemaphore(Queue->Semaphore, 1, nullptr);
    }
    else
    {
        UnhandledError("Texture queue full");
    }
}

lbfn b32 IsEmpty(texture_queue* Queue)
{
    b32 Result = (Queue->CompletionCount == Queue->CompletionGoal);
    return(Result);
}

lbfn b32 ProcessEntry(texture_queue* Queue)
{
    b32 Result = false;
    if (Queue->ProcessingCount < Queue->CompletionGoal)
    {
        texture_queue_entry* Entry = Queue->Entries + (Queue->ProcessingCount++ % Queue->MaxEntryCount);

        memory_arena* Scratch = &Queue->Scratch;
        memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Scratch);

        int Alpha = 0;
        switch (Entry->TextureType)
        {
            case TextureType_Diffuse:
            {
                if (Entry->AlphaEnabled)
                {
                    Alpha = 1;
                    Entry->Info.Format = Format_BC3_SRGB;
                }
                else
                {
                    Entry->Info.Format = Format_BC1_RGB_SRGB;
                }
            } break;
            // TODO(boti): Transmission should be BC4 (or packed into some channel of another texture),
            // encoding it as BC5 is a temporary hack to get thing running
            case TextureType_Transmission: 
            case TextureType_Normal:
            {
                Entry->Info.Format = Format_BC5_UNorm;
            } break;
            case TextureType_Material:
            {
                Entry->Info.Format = Format_BC3_UNorm;
            } break;
            InvalidDefaultCase;
        }
        
        buffer ImageFile = Platform.LoadEntireFile(Entry->Path.Path, Scratch);
        loaded_image Image = LoadImage(Scratch, ImageFile);

        // TODO(boti): Currently all images get converted to 8bpc at load time,
        // but if we ever support higher bit depths, this function will need to get updated as well
        Assert(Image.BitDepthPerChannel == 8);

        u8* SrcImage = (u8*)Image.Data;
        Entry->Info.Extent = { Image.Extent.X, Image.Extent.Y, 1 };
        Entry->Info.ArrayCount = 1;
        if (SrcImage)
        {
            Entry->Info.MipCount = GetMaxMipCount(Entry->Info.Extent.X, Entry->Info.Extent.Y);
            format_byterate ByteRate = FormatByterateTable[Entry->Info.Format];
            u64 TotalMipChainSize = GetMipChainSize(Entry->Info.Extent.X, Entry->Info.Extent.Y, Entry->Info.MipCount, 1, ByteRate);
            umm MipChainBegin = GetNextEntryOffset(Queue, TotalMipChainSize, Queue->RingBufferWriteAt);
            while (((MipChainBegin + TotalMipChainSize) - Queue->RingBufferReadAt) >= Queue->RingBufferSize)
            {
                SpinWait;
            }

            u8* MipChain = Queue->RingBufferMemory + (MipChainBegin % Queue->RingBufferSize);
            Queue->RingBufferWriteAt = MipChainBegin + TotalMipChainSize;

            u32 BlockSize = 16 * ByteRate.Numerator / ByteRate.Denominator;

            u8* DownscaleBuffer = (u8*)PushSize_(Scratch, 0, Entry->Info.Extent.X * Entry->Info.Extent.Y, 64);
            u8* SrcAt = SrcImage;
            u8* DstAt = MipChain;
            for (u32 MipIndex = 0; MipIndex < Entry->Info.MipCount; MipIndex++)
            {
                u32 ExtentX = Max(Entry->Info.Extent.X >> MipIndex, 1u);
                u32 ExtentY = Max(Entry->Info.Extent.Y >> MipIndex, 1u);

                if (MipIndex != 0)
                {
                    u32 PrevExtentX = Max(Entry->Info.Extent.X >> (MipIndex - 1), 1u);
                    u32 PrevExtentY = Max(Entry->Info.Extent.Y >> (MipIndex - 1), 1u);

                    switch (Entry->TextureType)
                    {
                        case TextureType_Diffuse:
                        {
                            stbir_resize_uint8_srgb(
                                (u8*)SrcAt, PrevExtentX, PrevExtentY, 0, 
                                (u8*)DownscaleBuffer, ExtentX, ExtentY, 0,
                                (stbir_pixel_layout)Image.ChannelCount);
                        } break;

                        case TextureType_Transmission:
                        case TextureType_Normal:
                        case TextureType_Material:
                        {
                            stbir_resize_uint8_linear(
                                (u8*)SrcAt, PrevExtentX, PrevExtentY, 0, 
                                (u8*)DownscaleBuffer, ExtentX, ExtentY, 0,
                                (stbir_pixel_layout)Image.ChannelCount);
                        } break;
                        InvalidDefaultCase;
                    }
                    SrcAt = DownscaleBuffer;
                }

                for (u32 Y = 0; Y < ExtentX; Y += 4)
                {
                    for (u32 X = 0; X < ExtentY; X += 4)
                    {
                        switch (Entry->Info.Format)
                        {
                            case Format_BC3_SRGB: Alpha = 1;
                            case Format_BC3_UNorm: Alpha = 1;
                            case Format_BC1_RGB_SRGB:
                            {
                                Assert(Image.ChannelCount == 3 || Image.ChannelCount == 4);

                                u8 Block[4*4][4];
                                for (u32 BlockY = 0; BlockY < 4; BlockY++)
                                {
                                    for (u32 BlockX = 0; BlockX < 4; BlockX++)
                                    {
                                        umm Offset = ((X + BlockX) + (Y + BlockY) * ExtentX) * Image.ChannelCount;
                                        u8* Src = SrcAt + Offset;
                                        Block[BlockX + 4 * BlockY][0] = Src[0];
                                        Block[BlockX + 4 * BlockY][1] = Src[1];
                                        Block[BlockX + 4 * BlockY][2] = Src[2];
                                        Block[BlockX + 4 * BlockY][3] = (Image.ChannelCount == 4) ? Src[3] : 0xFFu;
                                    }
                                }
                                stb_compress_dxt_block(DstAt, (u8*)Block, Alpha, STB_DXT_HIGHQUAL);
                            } break;
                            case Format_BC5_UNorm:
                            {
                                Assert(Image.ChannelCount >= 2);

                                u8 Block[4*4][2];
                                for (u32 BlockY = 0; BlockY < 4; BlockY++)
                                {
                                    for (u32 BlockX = 0; BlockX < 4; BlockX++)
                                    {
                                        umm Offset = ((X + BlockX) + (Y + BlockY) * ExtentX) * Image.ChannelCount;
                                        u8* Src = SrcAt + Offset;
                                        Block[BlockX + 4*BlockY][0] = Src[0];
                                        Block[BlockX + 4*BlockY][1] = Src[1];
                                    }
                                }
                                stb_compress_bc5_block(DstAt, (u8*)Block);
                            } break;
                            InvalidDefaultCase;
                        }

                        DstAt += BlockSize;
                    }
                }
            }
            Entry->ReadyToTransfer = true;
        }
        else 
        {
            UnhandledError("Failed to load image");
        }
        RestoreArena(Scratch, Checkpoint);
    }
    else
    {
        Result = true;
    }
    return(Result);
}

//
// Assets
//
lbfn b32 InitializeAssets(assets* Assets, render_frame* Frame, memory_arena* Scratch)
{
    b32 Result = true;

    // Texture cache
    {
        umm CacheSize = GiB(4);
        Assets->TextureCache = InitializeArena(CacheSize, PushSize_(&Assets->Arena, 0, CacheSize, KiB(4)));
    }

    // Texture queue
    {
        texture_queue* Queue = &Assets->TextureQueue;
        Queue->Semaphore = Platform.CreateSemaphore(0, 1);
        umm ScratchSize = MiB(384);
        Queue->Scratch = InitializeArena(ScratchSize, PushSize_(&Assets->Arena, 0, ScratchSize, KiB(4)));

        Queue->RingBufferSize = MiB(128);
        Queue->RingBufferMemory = (u8*)PushSize_(&Assets->Arena, 0, Queue->RingBufferSize, KiB(4));

        Platform.CreateThread(&AssetThread, Assets, L"AssetThread");
    }

    // Default textures
    {
        texture_info Info =
        {
            .Extent = { 1, 1, 1 },
            .MipCount = 1,
            .ArrayCount = 1,
            .Format = Format_R8G8B8A8_SRGB,
            .Swizzle = {},
        };

        // NOTE(boti): We want the the null texture to be some sensible default
        Assert(Assets->TextureCount == 0);
        Assets->WhitenessID = Assets->TextureCount++;
        texture* Whiteness = Assets->Textures + Assets->WhitenessID;

        Whiteness->RendererID = Platform.AllocateTexture(Frame->Renderer, TextureFlag_PersistentMemory, &Info, InvalidRendererTextureID);
        Assets->DefaultTextures[TextureType_Diffuse] = Assets->WhitenessID;
        Assets->DefaultTextures[TextureType_Material] = Assets->WhitenessID;
        Assets->DefaultTextures[TextureType_Transmission] = Assets->WhitenessID;

        u32 Texel = 0xFFFFFFFFu;
        TransferTexture(Frame, Whiteness->RendererID, Info, AllTextureSubresourceRange(), &Texel);
        Whiteness->IsLoaded = true;
    }

    {
        texture_info Info =
        {
            .Extent = { 1, 1, 1 },
            .MipCount = 1,
            .ArrayCount = 1,
            .Format = Format_R8G8_UNorm,
            .Swizzle = {},
        };

        Assets->DefaultTextures[TextureType_Normal] = Assets->TextureCount++;
        texture* DefaultNormalTexture = Assets->Textures + Assets->DefaultTextures[TextureType_Normal];
        DefaultNormalTexture->RendererID = Platform.AllocateTexture(Frame->Renderer, TextureFlag_PersistentMemory, &Info, InvalidRendererTextureID);
        u16 Texel = 0x8080u;
        TransferTexture(Frame, DefaultNormalTexture->RendererID, Info, AllTextureSubresourceRange(), &Texel);
        DefaultNormalTexture->IsLoaded = true;
    }

    // Null skin
    {
        skin* NullSkin = Assets->Skins + Assets->SkinCount++;
        NullSkin->JointCount = 0;
    }

    // Null animation
    {
        animation* NullAnimation = Assets->Animations + Assets->AnimationCount++;
        NullAnimation->SkinID = 0;
        NullAnimation->KeyFrameCount = 1;
        NullAnimation->MinTimestamp = 0.0f;
        NullAnimation->MaxTimestamp = 0.0f;
        NullAnimation->KeyFrameTimestamps = PushArray(&Assets->Arena, 0, f32, 1);
        NullAnimation->KeyFrameTimestamps[0] = 0.0f;
        NullAnimation->KeyFrames = PushArray(&Assets->Arena, 0, animation_key_frame, 1);
        for (u32 JointIndex = 0; JointIndex < R_MaxJointCount; JointIndex++)
        {
            NullAnimation->KeyFrames[0].JointTransforms[JointIndex] = 
            {
                .Rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
                .Position = { 0.0f, 0.0f, 0.0f},
                .Scale = { 1.0f, 1.0f, 1.0f },
            };
        }
        NullAnimation->ActiveJoints = {};
    }

    // Particle textures
    {
        Assets->ParticleArrayID = Assets->TextureCount++;
        texture* ParticleArray = Assets->Textures + Assets->ParticleArrayID;
        ParticleArray->RendererID = Platform.AllocateTexture(Frame->Renderer, TextureFlag_Special|TextureFlag_PersistentMemory, nullptr, InvalidRendererTextureID);
        // TODO(boti): For now we know that the texture pack we're using is 512x512, 
        // but we may want to figure out some way for the user code to pack texture arrays/atlases dynamically
        texture_info Info =
        {
            .Extent = { 512, 512, 1 },
            .MipCount = 1,
            .ArrayCount = Particle_COUNT,
            .Format = Format_R8_UNorm,
            .Swizzle = { Swizzle_One, Swizzle_One, Swizzle_One, Swizzle_R },
        };

        umm ImageSize = Info.Extent.X * Info.Extent.Y;
        umm MemorySize = ImageSize * Particle_COUNT;
        void* Memory = PushSize_(Scratch, 0, MemorySize, 0x100);

        u8* MemoryAt = (u8*)Memory;
        for (u32 ParticleIndex = 0; ParticleIndex < Particle_COUNT; ParticleIndex++)
        {
            memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Scratch);

            buffer FileData = Platform.LoadEntireFile(ParticlePaths[ParticleIndex], Scratch);
            loaded_image Image = LoadImage(Scratch, FileData);

            if (Image.Data && 
                (Image.Extent.X == Info.Extent.X) && 
                (Image.Extent.Y == Info.Extent.Y) &&
                Image.BitDepthPerChannel == 8)
            {
                u8* SrcAt = (u8*)Image.Data;

                // HACK(boti): We know that we're using the black background particles, so the R channel is the alpha
                const u32 TargetChannel = 0;
                for (u32 Y = 0; Y  < Image.Extent.Y; Y++)
                {
                    for (u32 X = 0; X < Image.Extent.X; X++)
                    {
                        *MemoryAt++ = SrcAt[TargetChannel];
                        SrcAt += Image.ChannelCount;
                    }
                }
            }
            else
            {
                UnimplementedCodePath;
            }

            RestoreArena(Scratch, Checkpoint);
        }

        TransferTexture(Frame, ParticleArray->RendererID, Info, AllTextureSubresourceRange(), Memory);
        ParticleArray->IsLoaded = true;
    }

    LoadDebugFont(Scratch, Assets, Frame, "data/liberation-mono.lbfnt");

    auto AddMesh = [Assets, Frame](mesh_data MeshData) -> u32
    {
        u32 Result = U32_MAX;
        if (Assets->MeshCount < Assets->MaxMeshCount)
        {
            Result = Assets->MeshCount++;
            mesh* Mesh = Assets->Meshes + Result;

            Mesh->Allocation = Platform.AllocateGeometry(Frame->Renderer, MeshData.VertexCount, MeshData.IndexCount);
            Mesh->BoundingBox = MeshData.Box;
            Mesh->MaterialID = 0;
            TransferGeometry(Frame, Mesh->Allocation, MeshData.VertexData, MeshData.IndexData);
        }
        return(Result);
    };

    // Default meshes
    Assets->ArrowMeshID = AddMesh(CreateArrowMesh(Scratch));
    Assets->SphereMeshID = AddMesh(CreateSphereMesh(Scratch));
    Assets->CubeMeshID = AddMesh(CreateCubeMesh(Scratch));

    return(Result);
}

static void LoadDebugFont(memory_arena* Arena, assets* Assets, render_frame* Frame, const char* Path)
{
    memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Arena);
    buffer FileBuffer = Platform.LoadEntireFile(Path, Arena);
    lbfnt_file* FontFile = (lbfnt_file*)FileBuffer.Data;

    if (FontFile)
    {
        // HACK(boti): When sampling with wrap+linear mode, UV 0,0 pulls in all 4 corner texels,
        // so we set those to the maximum alpha value here.
        {
            auto SetPixel = [&](u32 x, u32 y, u8 Value) { FontFile->Bitmap.Bitmap[x + y*FontFile->Bitmap.Width] = Value; };
            u32 x = FontFile->Bitmap.Width - 1;
            u32 y = FontFile->Bitmap.Height - 1;
            SetPixel(0, 0, 0xFFu);
            SetPixel(x, 0, 0xFFu);
            SetPixel(0, y, 0xFFu);
            SetPixel(x, y, 0xFFu);
        }
        
        if (FontFile->FileTag == LBFNT_FILE_TAG)
        {
            font* Font = &Assets->DefaultFont;
            Font->RasterHeight = FontFile->FontInfo.RasterHeight;
            Font->Ascent = FontFile->FontInfo.Ascent;
            Font->Descent = FontFile->FontInfo.Descent;
            Font->BaselineDistance = FontFile->FontInfo.BaselineDistance;

            for (u32 i = 0; i < Font->CharCount; i++)
            {
                Font->CharMapping[i] = 
                {
                    .Codepoint = FontFile->CharacterMap[i].Codepoint,
                    .GlyphIndex = FontFile->CharacterMap[i].GlyphIndex,
                };
            }

            Font->GlyphCount = FontFile->GlyphCount;
            Assert(Font->GlyphCount <= Font->MaxGlyphCount);

            for (u32 i = 0; i < Font->GlyphCount; i++)
            {
                lbfnt_glyph* Glyph = FontFile->Glyphs + i;
                Font->Glyphs[i] = 
                {
                    .UV0 = { Glyph->u0, Glyph->v0 },
                    .UV1 = { Glyph->u1, Glyph->v1 },
                    .P0 = { Glyph->BoundsLeft, Glyph->BoundsTop },
                    .P1 = { Glyph->BoundsRight, Glyph->BoundsBottom },
                    .AdvanceX = Glyph->AdvanceX,
                    .OriginY = Glyph->OriginY,
                };

                // TODO(boti): This is the way Direct2D specifies that the bounds are empty,
                //             we probably shouldn't be exporting the glyph bounds this way in the first place!
                if (Font->Glyphs[i].P0.X > Font->Glyphs[i].P1.X)
                {
                    Font->Glyphs[i].P0 = { 0.0f, 0.0f };
                    Font->Glyphs[i].P1 = { 0.0f, 0.0f };
                }
            }

            Assets->DefaultFontTextureID = Assets->TextureCount++;
            texture* DefaultFontTexture = Assets->Textures + Assets->DefaultFontTextureID;
            DefaultFontTexture->RendererID = Platform.AllocateTexture(Frame->Renderer, TextureFlag_Special|TextureFlag_PersistentMemory, nullptr, InvalidRendererTextureID);

            texture_info Info = 
            {
                .Extent = { FontFile->Bitmap.Width, FontFile->Bitmap.Height, 1 },
                .MipCount = 1,
                .ArrayCount = 1,
                .Format = Format_R8_UNorm,
                .Swizzle = { Swizzle_One, Swizzle_One, Swizzle_One, Swizzle_R },
            };
            TransferTexture(Frame, DefaultFontTexture->RendererID, Info, AllTextureSubresourceRange(), FontFile->Bitmap.Bitmap);
            DefaultFontTexture->IsLoaded = true;
        }
        else
        {
            UnhandledError("Invalid font file");
        }
    }
    else
    {
        UnimplementedCodePath;
    }

    RestoreArena(Arena, Checkpoint);
}

// TODO(boti): There seem to be multiple places in here that might not handle the case where the buffer view stride is 0
internal void DEBUGLoadTestScene(memory_arena* Scratch, assets* Assets, game_world* World, render_frame* Frame, const char* ScenePath, m4 BaseTransform)
{
    filepath Filepath = {};
    b32 FilepathResult = MakeFilepathFromZ(&Filepath, ScenePath);
    Assert(FilepathResult);

    gltf GLTF = {};
    // JSON test parsing
    {
        buffer JSONBuffer = Platform.LoadEntireFile(ScenePath, Scratch);
        if (JSONBuffer.Data)
        {
            json_element* Root = ParseJSON(JSONBuffer.Data, JSONBuffer.Size, Scratch);
            if (Root)
            {
                bool GLTFParseResult = ParseGLTF(&GLTF, Root, Scratch);
                if (!GLTFParseResult)
                {
                    UnhandledError("Couldn't parse glTF file");
                    return;
                }
            }
            else
            {
                UnhandledError("Couldn't parse JSON file");
                return;
            }
        }
        else
        {
            UnhandledError("Couldn't load scene file");
            return;
        }
    }

    // NOTE(boti): Store the initial indices in the game so that we know what to offset the glTF indices by
    u32 BaseMeshIndex       = Assets->MeshCount;
    u32 BaseMaterialIndex   = Assets->MaterialCount;
    u32 BaseSkinIndex       = Assets->SkinCount;

    buffer* Buffers = PushArray(Scratch, MemPush_Clear, buffer, GLTF.BufferCount);
    for (u32 BufferIndex = 0; BufferIndex < GLTF.BufferCount; BufferIndex++)
    {
        string URI = GLTF.Buffers[BufferIndex].URI;
        if (OverwriteNameAndExtension(&Filepath, URI))
        {
            Buffers[BufferIndex] = Platform.LoadEntireFile(Filepath.Path, Scratch);
            if (!Buffers[BufferIndex].Data)
            {
                UnhandledError("Couldn't load glTF buffer");
            }
        }
        else
        {
            UnhandledError("glTF resource filename too long");
        }
    }

    struct loaded_gltf_image
    {
        u32 AssetID;
        u32 Usage;
    };

    loaded_gltf_image* ImageTable = PushArray(Scratch, MemPush_Clear, loaded_gltf_image, GLTF.ImageCount);

    for (u32 MaterialIndex = 0; MaterialIndex < GLTF.MaterialCount; MaterialIndex++)
    {
        gltf_material* SrcMaterial = GLTF.Materials + MaterialIndex;

        if (SrcMaterial->BaseColorTexture.TexCoordIndex != 0) UnimplementedCodePath;
        if (SrcMaterial->NormalTexture.TexCoordIndex != 0) UnimplementedCodePath;
        if (SrcMaterial->MetallicRoughnessTexture.TexCoordIndex != 0) UnimplementedCodePath;

        if (SrcMaterial->NormalTexture.Scale != 1.0f) UnimplementedCodePath;

        if (Assets->MaterialCount < Assets->MaxMaterialCount)
        {
            material* Material = Assets->Materials + Assets->MaterialCount++;

            Material->AlbedoID = Assets->DefaultTextures[TextureType_Diffuse];
            Material->NormalID = Assets->DefaultTextures[TextureType_Normal];
            Material->MetallicRoughnessID = Assets->DefaultTextures[TextureType_Material];
            Material->AlbedoSamplerID = { 0 };
            Material->NormalSamplerID = { 0 };
            Material->MetallicRoughnessSamplerID = { 0 };
            switch (SrcMaterial->AlphaMode)
            {
                case GLTF_ALPHA_MODE_OPAQUE:    Material->Transparency = Transparency_Opaque; break;
                case GLTF_ALPHA_MODE_MASK:      Material->Transparency = Transparency_AlphaTest; break;
                case GLTF_ALPHA_MODE_BLEND:     Material->Transparency = Transparency_AlphaBlend; break;

            }
            Material->Emission = SrcMaterial->EmissiveFactor;
            Material->AlphaThreshold = SrcMaterial->AlphaCutoff;
            Material->Albedo = PackRGBA(SrcMaterial->BaseColorFactor);
            Material->MetallicRoughness = PackRGBA(v4{ 1.0f, SrcMaterial->RoughnessFactor, SrcMaterial->MetallicFactor, 1.0f });
            Material->TransmissionEnabled = SrcMaterial->TransmissionEnabled;
            Material->Transmission = SrcMaterial->TransmissionFactor;

            auto ProcessTexture = [&GLTF, Assets, ImageTable](
                gltf_texture_info* TextureInfo,
                flags32 Usage,
                material_sampler_id* SamplerID, 
                u32* TextureID)
            {
                auto ConvertGLTFSampler = [](gltf_sampler* Sampler) -> material_sampler_id
                {
                    auto ConvertGLTFWrap = [](gltf_wrap Wrap) -> tex_wrap
                    {
                        tex_wrap Result = Wrap_Repeat;
                        switch (Wrap)
                        {
                            case GLTF_WRAP_REPEAT:          Result = Wrap_Repeat; break;
                            case GLTF_WRAP_CLAMP_TO_EDGE:   Result = Wrap_ClampToEdge; break;
                            case GLTF_WRAP_MIRRORED_REPEAT: Result = Wrap_RepeatMirror; break;
                                InvalidDefaultCase;
                        }
                        return(Result);
                    };

                    material_sampler_id Result = GetMaterialSamplerID(ConvertGLTFWrap(Sampler->WrapU), ConvertGLTFWrap(Sampler->WrapV), Wrap_Repeat);
                    return(Result);
                };

                if (TextureInfo->TextureIndex != U32_MAX)
                {
                    gltf_texture* GLTFTexture = GLTF.Textures + TextureInfo->TextureIndex;
                    gltf_sampler* Sampler = GLTF.Samplers + GLTFTexture->SamplerIndex;
                    *SamplerID = ConvertGLTFSampler(Sampler);
                    
                    if (GLTFTexture->ImageIndex >= GLTF.ImageCount)
                    {
                        UnhandledError("Corrupt glTF: texture image index out of bounds");
                    }
                    loaded_gltf_image* Image = ImageTable + GLTFTexture->ImageIndex;
                    if (!Image->AssetID)
                    {
                        if (Assets->TextureCount < Assets->MaxTextureCount)
                        {
                            Image->AssetID = Assets->TextureCount++;
                        }
                        else
                        {
                            UnimplementedCodePath;
                        }
                    }
                    *TextureID = Image->AssetID;
                    Image->Usage |= Usage;
                }
            };

            ProcessTexture(&SrcMaterial->BaseColorTexture, 
                           (1u << TextureData_Albedo),
                           &Material->AlbedoSamplerID, 
                           &Material->AlbedoID);
            ProcessTexture(&SrcMaterial->NormalTexture,
                           (1u << TextureData_Normal),
                           &Material->NormalSamplerID, 
                           &Material->NormalID);
            ProcessTexture(&SrcMaterial->MetallicRoughnessTexture,
                           (1u << TextureData_Metallic) | (1u << TextureData_Roughness),
                           &Material->MetallicRoughnessSamplerID, 
                           &Material->MetallicRoughnessID);
            ProcessTexture(&SrcMaterial->OcclusionTexture,
                           (1u << TextureData_Occlusion),
                           &Material->MetallicRoughnessSamplerID, // TODO(boti): rename
                           &Material->MetallicRoughnessID);
            ProcessTexture(&SrcMaterial->TransmissionTexture, 
                           (1u << TextureData_Transmission),
                           &Material->TransmissionSamplerID, 
                           &Material->TransmissionID);
        }
        else
        {
            UnhandledError("Out of material pool");
        }
    }

    for (u32 ImageIndex = 0; ImageIndex < GLTF.ImageCount; ImageIndex++)
    {
        loaded_gltf_image* Image = ImageTable + ImageIndex;
        gltf_image* GLTFImage = GLTF.Images + ImageIndex;
        if ((GLTFImage->URI.Length == 0) || (GLTFImage->BufferViewIndex != U32_MAX))
        {
            UnimplementedCodePath;
        }
        
        if (Image->AssetID && Image->Usage)
        {
            b32 DoUpload = true;
            b32 AlphaEnabled = false;
            texture_type Type = TextureType_Diffuse;

            if (Image->Usage == (1u << TextureData_Albedo))
            {
                AlphaEnabled = true;
                Type = TextureType_Diffuse;
            }
            else if (Image->Usage == (1u << TextureData_Normal))
            {
                Type = TextureType_Normal;
            }
            else if (Image->Usage == ((1u << TextureData_Metallic) | (1u << TextureData_Roughness)))
            {
                Type = TextureType_Material;
            }
            else if (Image->Usage == (1u << TextureData_Transmission)) 
            {
                Type = TextureType_Transmission;
            }
            else
            {
                DoUpload = false;
            }

            if (DoUpload)
            {
                renderer_texture_id Placeholder = Assets->Textures[Assets->DefaultTextures[Type]].RendererID;
                texture* Asset = Assets->Textures + Image->AssetID;
                Asset->RendererID = Platform.AllocateTexture(Frame->Renderer, TextureFlag_None, nullptr, Placeholder);
                Asset->IsLoaded = false;

                if (OverwriteNameAndExtension(&Filepath, GLTFImage->URI))
                {
                    AddEntry(&Assets->TextureQueue, Asset, Type, AlphaEnabled, &Filepath);
                }
                else
                {
                    UnhandledError("Invalid glTF image URI");
                }
            }
        }
    }

    for (u32 MeshIndex = 0; MeshIndex < GLTF.MeshCount; MeshIndex++)
    {
        gltf_mesh* Mesh = GLTF.Meshes + MeshIndex;
        for (u32 PrimitiveIndex = 0; PrimitiveIndex < Mesh->PrimitiveCount; PrimitiveIndex++)
        {
            memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Scratch);

            gltf_mesh_primitive* Primitive = Mesh->Primitives + PrimitiveIndex;
            if (Primitive->Topology != GLTF_TRIANGLES)
            {
                UnimplementedCodePath;
            }
            
            gltf_accessor* PAccessor = (Primitive->PositionIndex < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->PositionIndex : nullptr;
            gltf_accessor* NAccessor = (Primitive->NormalIndex < GLTF.AccessorCount)  ? GLTF.Accessors + Primitive->NormalIndex : nullptr;
            gltf_accessor* TAccessor = (Primitive->TangentIndex < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->TangentIndex : nullptr;
            gltf_accessor* TCAccessor = (Primitive->TexCoordIndex[0] < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->TexCoordIndex[0] : nullptr;
            gltf_accessor* JointsAccessor = (Primitive->JointsIndex < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->JointsIndex : nullptr;
            gltf_accessor* WeightsAccessor = (Primitive->WeightsIndex < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->WeightsIndex : nullptr;

            gltf_iterator ItP   = MakeGLTFAttribIterator(&GLTF, PAccessor, Buffers);
            gltf_iterator ItN   = MakeGLTFAttribIterator(&GLTF, NAccessor, Buffers);
            gltf_iterator ItT   = MakeGLTFAttribIterator(&GLTF, TAccessor, Buffers);
            gltf_iterator ItTC  = MakeGLTFAttribIterator(&GLTF, TCAccessor, Buffers);

            u32 VertexCount = (u32)ItP.Count;
            if (VertexCount == 0)
            {
                UnhandledError("Missing glTF position data");
            }

            if ((ItN.Count && (ItN.Count != VertexCount)) ||
                (ItT.Count && (ItT.Count!= VertexCount)) ||
                (ItTC.Count && (ItTC.Count!= VertexCount)))
            {
                UnhandledError("Inconsistent glTF vertex attribute counts");
            }

            if (!PAccessor)
            {
                UnhandledError("glTF No position data for mesh");
            }

            if (PAccessor->Type != GLTF_VEC3)
            {
                UnhandledError("Invalid glTF Position type");
            }

            mmbox Box = {};
            for (u32 i = 0; i < 3; i++)
            {
                Box.Min.E[i] = PAccessor->Min.EE[i];
                Box.Max.E[i] = PAccessor->Max.EE[i];
            }

            vertex* VertexData = PushArray(Scratch, 0, vertex, VertexCount);
            for (u32 i = 0; i < VertexCount; i++)
            {
                vertex* V = VertexData + i;
                V->P = ItP.Get<v3>();
                V->N = ItN.Get<v3>();
                V->T = ItT.Get<v4>();
                V->TexCoord = ItTC.Get<v2>();

                ++ItP;
                ++ItN;
                ++ItT;
                ++ItTC;
            }

            if (JointsAccessor || WeightsAccessor)
            {

                Verify(JointsAccessor && WeightsAccessor);
                Verify(JointsAccessor->Type == GLTF_VEC4 && WeightsAccessor->Type == GLTF_VEC4);

                Verify(JointsAccessor->BufferView < GLTF.BufferViewCount);
                gltf_buffer_view* JointsView = GLTF.BufferViews + JointsAccessor->BufferView;
                Verify(JointsView->BufferIndex < GLTF.BufferCount);
                buffer* JointsBuffer = Buffers + JointsView->BufferIndex;
                Verify(JointsAccessor->ByteOffset + JointsView->Offset + VertexCount * JointsView->Stride <= JointsBuffer->Size);
                void* JointsAt = OffsetPtr(JointsBuffer->Data, JointsView->Offset + JointsAccessor->ByteOffset);
                u32 JointsStride = JointsView->Stride;
                if (JointsStride == 0)
                {
                    JointsStride = GLTFGetDefaultStride(JointsAccessor);
                }

                Verify(WeightsAccessor->BufferView < GLTF.BufferViewCount);
                gltf_buffer_view* WeightsView = GLTF.BufferViews + WeightsAccessor->BufferView;
                Verify(WeightsView->BufferIndex < GLTF.BufferCount);
                buffer* WeightsBuffer = Buffers + WeightsView->BufferIndex;
                Verify(WeightsAccessor->ByteOffset + WeightsView->Offset + VertexCount * WeightsView->Stride <= WeightsBuffer->Size);
                void* WeightsAt = OffsetPtr(WeightsBuffer->Data, WeightsView->Offset + WeightsAccessor->ByteOffset);
                u32 WeightsStride = WeightsView->Stride;
                if (WeightsStride == 0)
                {
                    WeightsStride = GLTFGetDefaultStride(WeightsAccessor);
                }

                if (WeightsAccessor->ComponentType != GLTF_FLOAT)
                {
                    UnimplementedCodePath;
                }

                for (u32 i = 0; i < VertexCount; i++)
                {
                    vertex* Vertex = VertexData + i;
                    for (u32 JointIndex = 0; JointIndex < 4; JointIndex++)
                    {
                        switch (JointsAccessor->ComponentType)
                        {
                            case GLTF_UBYTE:
                            {
                                u8 Joint = ((u8*)JointsAt)[JointIndex];
                                Vertex->Joints[JointIndex] = Joint;
                            } break;
                            case GLTF_USHORT:
                            {
                                u16 Joint = ((u16*)JointsAt)[JointIndex];
                                if (Joint > 0xFF) UnimplementedCodePath;
                                Vertex->Joints[JointIndex] = (u8)Joint;
                            } break;
                            case GLTF_UINT:
                            {
                                u32 Joint = ((u32*)JointsAt)[JointIndex];
                                if (Joint > 0xFF) UnimplementedCodePath;
                                Vertex->Joints[JointIndex] = (u8)Joint;
                            } break;
                            default:
                            {
                                UnimplementedCodePath;
                            } break;
                        }
                    }

                    Vertex->Weights = *(v4*)WeightsAt;

                    JointsAt = OffsetPtr(JointsAt, JointsStride);
                    WeightsAt = OffsetPtr(WeightsAt, WeightsStride);
                }
            }

            u32 IndexCount = 0;
            u32* IndexData = nullptr;
            if (Primitive->IndexBufferIndex == U32_MAX)
            {
                IndexCount = VertexCount;
                IndexData = PushArray(Scratch, 0, u32, IndexCount);
                for (u32 i = 0; i < IndexCount; i++)
                {
                    IndexData[i] = i;
                }
            }
            else
            {
                gltf_accessor* IndexAccessor = GLTF.Accessors + Primitive->IndexBufferIndex;
                gltf_iterator ItIndex = MakeGLTFAttribIterator(&GLTF, IndexAccessor, Buffers);

                IndexCount = (u32)ItIndex.Count;
                IndexData = PushArray(Scratch, 0, u32, IndexCount);
                for (u32 i = 0; i < IndexCount; i++)
                {
                    switch (IndexAccessor->ComponentType)
                    {
                        case GLTF_USHORT:
                        case GLTF_SSHORT:
                        {
                            IndexData[i] = ItIndex.Get<u16>();
                        } break;
                        case GLTF_UINT:
                        case GLTF_SINT:
                        {
                            IndexData[i] = ItIndex.Get<u32>();
                        } break;
                        InvalidDefaultCase;
                    }
                    ++ItIndex;
                }
            }

            if (ItT.Count == 0)
            {
                v3* Tangents   = PushArray(Scratch, MemPush_Clear, v3, VertexCount);
                v3* Bitangents = PushArray(Scratch, MemPush_Clear, v3, VertexCount);
                for (u32 i = 0; i < IndexCount; i += 3)
                {
                    u32 Index0 = IndexData[i + 0];
                    u32 Index1 = IndexData[i + 1];
                    u32 Index2 = IndexData[i + 2];
                    vertex* V0 = VertexData + Index0;
                    vertex* V1 = VertexData + Index1;
                    vertex* V2 = VertexData + Index2;

                    v3 dP1 = V1->P - V0->P;
                    v3 dP2 = V2->P - V0->P;
                    
                    // NOTE(boti): Generate the normals here too, if they're not present
                    if (ItN.Count == 0)
                    {
                        v3 N = NOZ(Cross(dP1, dP2));
                        V0->N += N;
                        V1->N += N;
                        V2->N += N;
                    }

                    v2 dT1 = V1->TexCoord - V0->TexCoord;
                    v2 dT2 = V2->TexCoord - V0->TexCoord;

                    f32 InvDetT = 1.0f / (dT1.X * dT2.Y - dT1.Y * dT2.X);

                    v3 Tangent = 
                    {
                        (dP1.X * dT2.Y - dP2.X * dT1.Y) * InvDetT,
                        (dP1.Y * dT2.Y - dP2.Y * dT1.Y) * InvDetT,
                        (dP1.Z * dT2.Y - dP2.Z * dT1.Y) * InvDetT,
                    };

                    v3 Bitangent = 
                    {
                        (dP1.Z * dT2.X - dP2.X * dT1.X) * InvDetT,
                        (dP1.Y * dT2.X - dP2.Y * dT1.X) * InvDetT,
                        (dP1.Z * dT2.X - dP2.Z * dT1.X) * InvDetT,
                    };

                    Tangents[Index0] += Tangent;
                    Tangents[Index1] += Tangent;
                    Tangents[Index2] += Tangent;

                    Bitangents[Index0] += Bitangent;
                    Bitangents[Index1] += Bitangent;
                    Bitangents[Index2] += Bitangent;
                }

                for (u32 i = 0; i < VertexCount; i++)
                {
                    vertex* V = VertexData + i;

                    // NOTE(boti): This is only actually needed in case we had to generate the normals ourselves
                    V->N = NOZ(V->N);

                    v3 T = Tangents[i] - (V->N * Dot(V->N, Tangents[i]));
                    if (Dot(T, T) > 1e-7f)
                    {
                        T = Normalize(T);
                    }

                    v3 B = Cross(V->N, Tangents[i]);
                    f32 W = Dot(B, Bitangents[i]) < 0.0f ? -1.0f : 1.0f;
                    V->T = { T.X, T.Y, T.Z, W };
                }
            }

            if (Assets->MeshCount < Assets->MaxMeshCount)
            {
                u32 MeshID = Assets->MeshCount++;
                mesh* DstMesh = Assets->Meshes + MeshID;
                DstMesh->Allocation = Platform.AllocateGeometry(Frame->Renderer, VertexCount, IndexCount);
                DstMesh->BoundingBox = Box;
                DstMesh->MaterialID = Primitive->MaterialIndex + BaseMaterialIndex;
                TransferGeometry(Frame, DstMesh->Allocation, VertexData, IndexData);
            }
            else
            {
                UnhandledError("Out of mesh pool memory");
            }

            RestoreArena(Scratch, Checkpoint);
        }
    }

    for (u32 SkinIndex = 0; SkinIndex < GLTF.SkinCount; SkinIndex++)
    {
        gltf_skin* Skin = GLTF.Skins + SkinIndex;
        Assert(Skin->JointCount > 0);
        m4 ParentTransform = M4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        u32 RootJointNodeIndex = Skin->JointIndices[0];
        // NOTE(boti): Verify that the first joint node is actually the root
        for (u32 JointIndex = 1; JointIndex < Skin->JointCount; JointIndex++)
        {
            gltf_node* JointNode = GLTF.Nodes + Skin->JointIndices[JointIndex];
            for (u32 ChildIndex = 0; ChildIndex < JointNode->ChildrenCount; ChildIndex++)
            {
                if (RootJointNodeIndex == JointNode->Children[ChildIndex])
                {
                    UnimplementedCodePath;
                }
            }
        }

        Verify(Skin->InverseBindMatricesAccessorIndex < GLTF.AccessorCount);
        gltf_accessor* Accessor = GLTF.Accessors + Skin->InverseBindMatricesAccessorIndex;
        gltf_buffer_view* View  = GLTF.BufferViews + Accessor->BufferView;
        buffer* Buffer          = Buffers + View->BufferIndex;

        Verify(Accessor->IsSparse == false);
        Verify(Accessor->ComponentType == GLTF_FLOAT);
        Verify(Accessor->Type == GLTF_MAT4);

        if (Assets->SkinCount < Assets->MaxSkinCount)
        {
            skin* SkinAsset = Assets->Skins + Assets->SkinCount++;
            if (Skin->JointCount <= R_MaxJointCount)
            {
                SkinAsset->JointCount = Skin->JointCount;
                u32 InverseBindMatrixStride = View->Stride ? View->Stride : sizeof(m4);
                void* InverseBindMatrixAt = OffsetPtr(Buffer->Data, View->Offset + Accessor->ByteOffset);

                // NOTE(boti): We clear the joints here to later verify that the joint node has a single parent
                for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                {
                    SkinAsset->JointParents[JointIndex] = 0;
                }

                for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                {
                    m4 InverseBindTransform = *(m4*)InverseBindMatrixAt;
                    InverseBindMatrixAt = OffsetPtr(InverseBindMatrixAt, InverseBindMatrixStride);
                    SkinAsset->InverseBindMatrices[JointIndex] = InverseBindTransform;
                    u32 NodeIndex = Skin->JointIndices[JointIndex];
                    if (NodeIndex < GLTF.NodeCount)
                    {
                        gltf_node* Node = GLTF.Nodes + NodeIndex;
                        if (Node->IsTRS)
                        {
                            SkinAsset->BindPose[JointIndex] = 
                            {
                                .Rotation = Node->Rotation,
                                .Position = Node->Translation,
                                .Scale = Node->Scale,
                            };
                        }
                        else
                        {
                            SkinAsset->BindPose[JointIndex] = M4ToTRS(Node->Transform);
                        }

                        // Build the joint hierarchy
                        // TODO(boti): We need to handle the case where the joints don't form a closed hierarchy 
                        // (i.e. bones are parented to nodes not part of the skeleton)
                        for (u32 ChildIt = 0; ChildIt < Node->ChildrenCount; ChildIt++)
                        {
                            u32 ChildIndex = Node->Children[ChildIt];
                            for (u32 JointIt = 0; JointIt < Skin->JointCount; JointIt++)
                            {
                                if (Skin->JointIndices[JointIt] == ChildIndex)
                                {
                                    if (JointIndex >= JointIt)
                                    {
                                        // TODO(boti): Reorder the joints so that children don't precede their parents
                                        UnimplementedCodePath;
                                    }
                                    Verify(SkinAsset->JointParents[JointIt] == 0);
                                    SkinAsset->JointParents[JointIt] = JointIndex;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        UnhandledError("Invalid joint node index in glTF skin");
                    }
                }
            }
            else
            {
                UnhandledError("Too many joints in skin");
            }
        }
        else
        {
            UnhandledError("Out of skin pool memory");
        }
    }

    for (u32 AnimationIndex = 0; AnimationIndex < GLTF.AnimationCount; AnimationIndex++)
    {
        gltf_animation* Animation = GLTF.Animations + AnimationIndex;
        if (Animation->ChannelCount == 0)
        {
            continue;
        }

        // NOTE(boti): We only import animations that are tied to a specific skin
        u32 SkinIndex = U32_MAX;
        {
            u32 NodeIndex = Animation->Channels[0].Target.NodeIndex;
            for (u32 SkinIt = 0; SkinIt < GLTF.SkinCount; SkinIt++)
            {
                gltf_skin* Skin = GLTF.Skins + SkinIt;
                for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                {
                    if (NodeIndex == Skin->JointIndices[JointIndex])
                    {
                        SkinIndex = SkinIt;
                    }

                    if (SkinIndex != U32_MAX) break;
                }

                if (SkinIndex != U32_MAX) break;
            }
        }
        // Skip the current animation if it doesn't belong to any skin
        if (SkinIndex == U32_MAX) continue;
        gltf_skin* Skin = GLTF.Skins + SkinIndex;

        // NOTE(boti): We start the count at 1 because even if the animation doesn't have an explicit 0t
        // key frame, we'll want to add that
        u32 MaxKeyFrameCount = 1;
        for (u32 SamplerIndex = 0; SamplerIndex < Animation->SamplerCount; SamplerIndex++)
        {
            gltf_animation_sampler* Sampler = Animation->Samplers + SamplerIndex;
            Verify(Sampler->InputAccessorIndex < GLTF.AccessorCount);

            gltf_accessor* TimestampAccessor = GLTF.Accessors + Sampler->InputAccessorIndex;

            Verify((TimestampAccessor->ComponentType == GLTF_FLOAT) &&
                   (TimestampAccessor->Type == GLTF_SCALAR));

            MaxKeyFrameCount += TimestampAccessor->Count;
        }

        f32* KeyFrameTimestamps = PushArray(Scratch, 0, f32, MaxKeyFrameCount);
        u32 KeyFrameCount = 1;
        KeyFrameTimestamps[0] = 0.0f;
        for (u32 SamplerIndex = 0; SamplerIndex < Animation->SamplerCount; SamplerIndex++)
        {
            gltf_animation_sampler* Sampler = Animation->Samplers + SamplerIndex;
            gltf_accessor* TimestampAccessor = GLTF.Accessors + Sampler->InputAccessorIndex;
            gltf_buffer_view* View = GLTF.BufferViews + TimestampAccessor->BufferView;
            buffer* Buffer = Buffers + View->BufferIndex;

            u32 Stride = View->Stride;
            if (Stride == 0)
            {
                Stride = GLTFGetDefaultStride(TimestampAccessor);
            }

            void* At = OffsetPtr(Buffer->Data, View->Offset + TimestampAccessor->ByteOffset);
            u32 Count = TimestampAccessor->Count;
            while (Count--)
            {
                f32 Timestamp = *(f32*)At;
                Assert(Timestamp >= 0.0f);
                At = OffsetPtr(At, Stride);
                
                b32 AlreadyExists = false;
                for (u32 KeyFrameIndex = 0; KeyFrameIndex < KeyFrameCount; KeyFrameIndex++)
                {
                    if (Timestamp == KeyFrameTimestamps[KeyFrameIndex])
                    {
                        AlreadyExists = true;
                        break;
                    }
                    else if (Timestamp < KeyFrameTimestamps[KeyFrameIndex])
                    {
                        for (u32 It = KeyFrameCount; It > KeyFrameIndex; It--)
                        {
                            KeyFrameTimestamps[It] = KeyFrameTimestamps[It - 1];
                        }
                        KeyFrameTimestamps[KeyFrameIndex] = Timestamp;
                        KeyFrameCount++;
                    }
                }

                if (!AlreadyExists)
                {
                    KeyFrameTimestamps[KeyFrameCount++] = Timestamp;
                }
            }
        }

        if (Assets->AnimationCount < Assets->MaxAnimationCount)
        {
            animation* AnimationAsset = Assets->Animations + Assets->AnimationCount++;
            AnimationAsset->SkinID = BaseSkinIndex + SkinIndex;
            AnimationAsset->KeyFrameCount = KeyFrameCount;
            AnimationAsset->KeyFrameTimestamps = PushArray(&Assets->Arena, 0, f32, KeyFrameCount);
            AnimationAsset->KeyFrames = PushArray(&Assets->Arena, 0, animation_key_frame, KeyFrameCount);
            memcpy(AnimationAsset->KeyFrameTimestamps, KeyFrameTimestamps, KeyFrameCount * sizeof(f32));

            memset(&AnimationAsset->ActiveJoints, 0x00, sizeof(AnimationAsset->ActiveJoints));
            skin* SkinAsset = Assets->Skins + AnimationAsset->SkinID;
            for (u32 KeyFrameIndex = 0; KeyFrameIndex < KeyFrameCount; KeyFrameIndex++)
            {
                animation_key_frame* KeyFrame = AnimationAsset->KeyFrames + KeyFrameIndex;
                for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                {
                    KeyFrame->JointTransforms[JointIndex] = SkinAsset->BindPose[JointIndex];
                }
            }

            for (u32 ChannelIndex = 0; ChannelIndex < Animation->ChannelCount; ChannelIndex++)
            {
                gltf_animation_channel* Channel = Animation->Channels + ChannelIndex;
                u32 NodeIndex = Channel->Target.NodeIndex;
                if (NodeIndex == U32_MAX) continue;
            
                u32 JointIndex = U32_MAX;
                for (u32 JointIt = 0; JointIt < Skin->JointCount; JointIt++)
                {
                    if (NodeIndex == Skin->JointIndices[JointIt])
                    {
                        JointIndex = JointIt;
                        u32 ArrayIndex, BitIndex;
                        if (JointMaskIndexFromJointIndex(JointIndex, &ArrayIndex, &BitIndex))
                        {
                            AnimationAsset->ActiveJoints.Bits[ArrayIndex] |= (1 << BitIndex);
                        }
                        else
                        {
                            UnhandledError("Invalid joint index");
                        }
                        break;
                    }
                }
            
                if (JointIndex != U32_MAX)
                {
                    gltf_animation_sampler* Sampler = Animation->Samplers + Channel->SamplerIndex;
                    gltf_accessor* TimestampAccessor = GLTF.Accessors + Sampler->InputAccessorIndex;
                    gltf_accessor* TransformAccessor = GLTF.Accessors + Sampler->OutputAccessorIndex;
                    Verify(TransformAccessor->ComponentType == GLTF_FLOAT);
                    AnimationAsset->MinTimestamp = TimestampAccessor->Min.EE[0];
                    AnimationAsset->MaxTimestamp = TimestampAccessor->Max.EE[0];

                    gltf_buffer_view* TimestampView = GLTF.BufferViews + TimestampAccessor->BufferView;
                    buffer* TimestampBuffer = Buffers + TimestampView->BufferIndex;
                    void* SamplerTimestampAt = OffsetPtr(TimestampBuffer->Data, TimestampView->Offset + TimestampAccessor->ByteOffset);
                    u32 TimestampStride = TimestampView->Stride;
                    if (TimestampStride == 0)
                    {
                        TimestampStride = GLTFGetDefaultStride(TimestampAccessor);
                    }

                    gltf_buffer_view* TransformView = GLTF.BufferViews + TransformAccessor->BufferView;
                    buffer* TransformBuffer = Buffers + TransformView->BufferIndex;
                    void* SamplerTransformAt = OffsetPtr(TransformBuffer->Data, TransformView->Offset + TransformAccessor->ByteOffset);
                    u32 TransformStride = TransformView->Stride;
                    if (TransformStride == 0)
                    {
                        TransformStride = GLTFGetDefaultStride(TransformAccessor);
                    }

                    Verify(TimestampAccessor->Count > 0);
                    Verify(TimestampAccessor->Count == TransformAccessor->Count);
                    switch (Channel->Target.Path)
                    {
                        case GLTF_Rotation:
                        {
                            Verify(TransformAccessor->Type == GLTF_VEC4);
                        } break;
                        case GLTF_Scale:
                        case GLTF_Translation:
                        {
                            Verify(TransformAccessor->Type == GLTF_VEC3);
                        } break;
                        default:
                        {
                            UnimplementedCodePath;
                        } break;
                    }

                    // NOTE(boti): The initial transform of a joint is _always_ going to be the first element in the accessor,
                    // _regardless_ of whether the t=0 keyframe exists or not, because the transform is required
                    // to be clamped to the first one available by the glTF spec
                    {
                        trs_transform* Transform = AnimationAsset->KeyFrames[0].JointTransforms + JointIndex;
                        switch (Channel->Target.Path)
                        {
                            case GLTF_Rotation: Transform->Rotation = *(v4*)SamplerTransformAt; break;
                            case GLTF_Translation: Transform->Position = *(v3*)SamplerTransformAt; break;
                            case GLTF_Scale: Transform->Scale = *(v3*)SamplerTransformAt; break;
                            default:
                            {
                                UnimplementedCodePath;
                            } break;
                        }
                    }

                    for (u32 KeyFrameIndex = 1; KeyFrameIndex < KeyFrameCount; KeyFrameIndex++)
                    {
                        f32 Timestamp = *(f32*)SamplerTimestampAt;
                        f32 CurrentTime = AnimationAsset->KeyFrameTimestamps[KeyFrameIndex];
                        while (Timestamp < CurrentTime)
                        {
                            SamplerTimestampAt = OffsetPtr(SamplerTimestampAt, TimestampStride);
                            SamplerTransformAt = OffsetPtr(SamplerTransformAt, TransformStride);
                            Timestamp = *(f32*)SamplerTimestampAt;
                        }

                        trs_transform* Transform = AnimationAsset->KeyFrames[KeyFrameIndex].JointTransforms + JointIndex;
                        if (CurrentTime < Timestamp)
                        {
                            f32 PrevTime = AnimationAsset->KeyFrameTimestamps[KeyFrameIndex - 1];
                            f32 DeltaTime = Timestamp - PrevTime;
                            f32 BlendFactor = (CurrentTime - PrevTime) / DeltaTime;

                            trs_transform* PrevTransform = AnimationAsset->KeyFrames[KeyFrameIndex - 1].JointTransforms + JointIndex;
                            switch (Channel->Target.Path)
                            {
                                case GLTF_Rotation:
                                {
                                    v4 Rotation = *(v4*)SamplerTransformAt;
                                    Transform->Rotation = Normalize(Lerp(PrevTransform->Rotation, Rotation, BlendFactor));
                                } break;
                                case GLTF_Translation:
                                {
                                    v3 Translation = *(v3*)SamplerTransformAt;
                                    Transform->Position = Lerp(PrevTransform->Position, Translation, BlendFactor);
                                } break;
                                case GLTF_Scale:
                                {
                                    v3 Scale = *(v3*)SamplerTransformAt;
                                    Transform->Scale = Lerp(PrevTransform->Scale, Scale, BlendFactor);
                                } break;
                                InvalidDefaultCase;
                            }
                        }
                        else
                        {
                            switch (Channel->Target.Path)
                            {
                                case GLTF_Rotation:
                                {
                                    Transform->Rotation = *(v4*)SamplerTransformAt;
                                } break;
                                case GLTF_Translation:
                                {
                                    Transform->Position = *(v3*)SamplerTransformAt;
                                } break;
                                case GLTF_Scale:
                                {
                                    Transform->Scale = *(v3*)SamplerTransformAt;
                                } break;
                                InvalidDefaultCase;
                            }
                        }
                    }
                }
                else
                {
                    UnhandledError("glTF animation contains targets that don't belong to a single skin");
                }
            }
        }
        else
        {
            UnhandledError("Out of animation pool");
        }
    }

    if (GLTF.DefaultSceneIndex >= GLTF.SceneCount)
    {
        UnhandledError("glTF default scene index out of bounds");
    }

    gltf_scene* Scene = GLTF.Scenes + GLTF.DefaultSceneIndex;

    m4* ParentTransforms = PushArray(Scratch, 0, m4, GLTF.NodeCount);
    u32* NodeParents = PushArray(Scratch, 0, u32, GLTF.NodeCount);

    u32 NodeQueueCount = 0;
    u32* NodeQueue = PushArray(Scratch, 0, u32, GLTF.NodeCount);
    for (u32 It = 0; It < Scene->RootNodeCount; It++)
    {
        u32 NodeIndex = Scene->RootNodes[It];
        NodeParents[NodeIndex] = U32_MAX;
        ParentTransforms[NodeIndex] = BaseTransform;
        NodeQueue[NodeQueueCount++] = NodeIndex;
    }

    for (u32 It = 0; It < NodeQueueCount; It++)
    {
        u32 NodeIndex = NodeQueue[It];
        gltf_node* Node = GLTF.Nodes + NodeIndex;
        u32 Parent = NodeParents[NodeIndex];

        m4 LocalTransform = Node->Transform;
        if (Node->IsTRS)
        {
            trs_transform TRS
            {
                .Rotation = Node->Rotation,
                .Position = Node->Translation,
                .Scale = Node->Scale,
            };
            LocalTransform = TRSToM4(TRS);
        }

        m4 ParentTransform = ParentTransforms[NodeIndex];
        m4 NodeTransform = ParentTransform * LocalTransform;
        for (u32 ChildIt = 0; ChildIt < Node->ChildrenCount; ChildIt++)
        {
            u32 ChildIndex = Node->Children[ChildIt];
            ParentTransforms[ChildIndex] = NodeTransform;
            NodeQueue[NodeQueueCount++] = ChildIndex;
        }

        if (Node->MeshIndex != U32_MAX)
        {
            Assert(Node->MeshIndex < GLTF.MeshCount);
            gltf_mesh* Mesh = GLTF.Meshes + Node->MeshIndex;

            // NOTE(boti): Because we treat each primitive as a mesh, we manually
            // have to figure out what the base ID of said mesh is going to be in the asset storage
            u32 MeshOffset = 0;
            for (u32 i = 0; i < Node->MeshIndex; i++)
            {
                MeshOffset += GLTF.Meshes[i].PrimitiveCount;
            }

            if (World->EntityCount <= World->MaxEntityCount)
            {
                entity* Entity = World->Entities + World->EntityCount++;
                Entity->Flags = EntityFlag_Mesh;
                Entity->Transform = NodeTransform;

                Assert(Mesh->PrimitiveCount <= Entity->MaxPieceCount);
                for (u32 PrimitiveIndex = 0; PrimitiveIndex < Mesh->PrimitiveCount; PrimitiveIndex++)
                {
                    Entity->Pieces[Entity->PieceCount++] = 
                    { 
                        .MeshID = BaseMeshIndex + MeshOffset + PrimitiveIndex 
                    };
                }

                if (Node->SkinIndex != U32_MAX)
                {
                    Entity->Flags |= EntityFlag_Skin;
                    Entity->SkinID = BaseSkinIndex + Node->SkinIndex;
                    Entity->CurrentAnimationID = 0;
                    Entity->DoAnimation = false;
                    Entity->AnimationCounter = 0.0f;
                }
                Entity->LightEmission = {};
            }
        }
    }
}

lbfn texture_set 
DEBUGLoadTextureSet(assets* Assets, render_frame* Frame, const char* Paths[TextureType_Count])
{
    texture_set Result = {};

    for (u32 TextureType = 0; TextureType < TextureType_Count; TextureType++)
    {
        u32 DefaultTextureID = Assets->DefaultTextures[TextureType];
        const char* PathString = Paths[TextureType];
        if (PathString && Assets->TextureCount < Assets->MaxTextureCount)
        {
            u32 ID = Assets->TextureCount++;
            texture* Texture = Assets->Textures + ID;
            memset(Texture, 0, sizeof(texture));
            renderer_texture_id Placeholder = Assets->Textures[DefaultTextureID].RendererID;
            Texture->RendererID = Platform.AllocateTexture(Frame->Renderer, TextureFlag_None, nullptr, Placeholder);

            filepath Path = {};
            if (MakeFilepathFromZ(&Path, PathString))
            {
                AddEntry(&Assets->TextureQueue, Texture, (texture_type)TextureType, false, &Path);
                Result.IDs[TextureType] = ID;
            }
        }
        else
        {
            Result.IDs[TextureType] = DefaultTextureID;
        }
    }

    return(Result);
}

internal mesh_data CreateCubeMesh(memory_arena* Arena)
{
    mesh_data Result = {};
    Result.Box = { { -1.0f, -1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f } };

    vertex VertexData[] = 
    {
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { -1.0f, +1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, +1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },

        { { -1.0f, -1.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, -1.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { -1.0f, +1.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, +1.0f, +1.0f }, { 0.0f, 0.0f, +1.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },

        { { -1.0f, -1.0f, -1.0f }, { 0.0f, -1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, -1.0f, -1.0f }, { 0.0f, -1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { -1.0f, -1.0f, +1.0f }, { 0.0f, -1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, -1.0f, +1.0f }, { 0.0f, -1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },

        { { -1.0f, +1.0f, -1.0f }, { 0.0f, +1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, +1.0f, -1.0f }, { 0.0f, +1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { -1.0f, +1.0f, +1.0f }, { 0.0f, +1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, +1.0f, +1.0f }, { 0.0f, +1.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },

        { { -1.0f, -1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { -1.0f, +1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { -1.0f, -1.0f, +1.0f }, { -1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { -1.0f, +1.0f, +1.0f }, { -1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },

        { { +1.0f, -1.0f, -1.0f }, { +1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, +1.0f, -1.0f }, { +1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, -1.0f, +1.0f }, { +1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
        { { +1.0f, +1.0f, +1.0f }, { +1.0f, 0.0f, 0.0f }, {}, {}, {}, {}, { PackRGBA8(0xFF, 0xFF, 0xFF) } },
    };

    constexpr u32 IndexCount = 6 * 6;
    u32 IndexData[IndexCount] = {};
    for (u32 Face = 0; Face < 6; Face++)
    {
        IndexData[6*Face + 0] = 4*Face + 0;
        IndexData[6*Face + 1] = 4*Face + 1;
        IndexData[6*Face + 2] = 4*Face + 3;
        IndexData[6*Face + 3] = 4*Face + 0;
        IndexData[6*Face + 4] = 4*Face + 3;
        IndexData[6*Face + 5] = 4*Face + 2;
    }

    Result.VertexCount = CountOf(VertexData);
    Result.VertexData = PushArray(Arena, 0, vertex, Result.VertexCount);
    memcpy(Result.VertexData, VertexData, sizeof(VertexData));

    Result.IndexCount = IndexCount;
    Result.IndexData = PushArray(Arena, 0, u32, Result.IndexCount);
    memcpy(Result.IndexData, IndexData, sizeof(IndexData));

    return(Result);
}

internal mesh_data CreateSphereMesh(memory_arena* Arena)
{
    mesh_data Result = {};

    Result.Box = { { -1.0f, -1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f } };

    constexpr u32 XCount = 16;
    constexpr u32 YCount = 16;

    Result.VertexCount = XCount * YCount;
    Result.VertexData = PushArray(Arena, 0, vertex, Result.VertexCount);

    Result.IndexCount = 6 * Result.VertexCount;
    Result.IndexData = PushArray(Arena, 0, u32, Result.IndexCount);

    vertex* VertexAt = Result.VertexData;
    vert_index* IndexAt = Result.IndexData;
    for (u32 X = 0; X < XCount; X++)
    {
        for (u32 Y = 0; Y < YCount; Y++)
        {
            f32 U = (f32)X / (f32)XCount;
            f32 V = (f32)Y / (f32)YCount;
            
            f32 Phi = 2.0f * Pi * U;
            f32 Theta = Pi * V;

            f32 SinPhi = Sin(Phi);
            f32 CosPhi = Cos(Phi);
            f32 SinTheta = Sin(Theta);
            f32 CosTheta = Cos(Theta);

            v3 P = { CosPhi * SinTheta, SinPhi * SinTheta, CosTheta };
            v3 T = { -SinPhi * SinTheta, CosPhi * SinTheta, CosTheta }; // TODO(boti): verify
            v3 N = P;

            *VertexAt++ =
            {
                .P = P,
                .N = N,
                .T = { T.X, T.Y, T.Z, 1.0f }, 
                .TexCoord = { U, V },
                .Weights = {},
                .Joints = {},
                .Color = PackRGBA8(0xFF, 0xFF, 0xFF),
            };

            u32 X0 = X;
            u32 Y0 = Y;
            u32 X1 = (X + 1) % XCount;
            u32 Y1 = (Y + 1) % YCount;
            *IndexAt++ = X0 + Y0 * XCount;
            *IndexAt++ = X1 + Y0 * XCount;
            *IndexAt++ = X0 + Y1 * XCount;

            *IndexAt++ = X1 + Y0 * XCount;
            *IndexAt++ = X1 + Y1 * XCount;
            *IndexAt++ = X0 + Y1 * XCount;
        }
    }

    return(Result);
}

internal mesh_data CreateArrowMesh(memory_arena* Arena)
{
    mesh_data Result = {};

    rgba8 White = { 0xFFFFFFFFu };

    constexpr f32 Radius = 0.05f;
    constexpr f32 Height = 0.8f;
    constexpr f32 TipRadius = 0.1f;
    constexpr f32 TipHeight = 0.2f;

    // NOTE(boti): Not a real bounding box, but should be better for clicking.
    Result.Box = { { -Radius, -Radius, 0.0f }, { +Radius, +Radius, TipHeight } };

    u32 RadiusDivisionCount = 16;

    u32 CapVertexCount = 1 + RadiusDivisionCount;
    u32 CylinderVertexCount = 2 * RadiusDivisionCount;
    u32 TipCapVertexCount = 2 * RadiusDivisionCount;
    u32 TipVertexCount = TipCapVertexCount + 2*RadiusDivisionCount;
    Result.VertexCount = CapVertexCount + CylinderVertexCount + TipVertexCount;

    u32 CapIndexCount = 3 * RadiusDivisionCount;
    u32 CylinderIndexCount = 2 * 3 * RadiusDivisionCount;
    u32 TipIndexCount = 2 * 3 * RadiusDivisionCount + 3 * RadiusDivisionCount;
    Result.IndexCount = CapIndexCount + CylinderIndexCount + TipIndexCount;

    Result.VertexData = PushArray(Arena, 0, vertex, Result.VertexCount);
    Result.IndexData = PushArray(Arena, 0, u32, Result.IndexCount);

    u32* IndexAt = Result.IndexData;
    vertex* VertexAt = Result.VertexData;

    // Bottom cap
    {
        v3 N = { 0.0f, 0.0f, -1.0f };
        v4 T = { 1.0f, 0.0f, 0.0f, 1.0f };
        // Origin
        *VertexAt++ = { { 0.0f, 0.0f, 0.0f }, N, T, {}, {}, {}, White };
        u32 BaseIndex = 1;

        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            v3 P = { Radius * Cos(Angle), Radius * Sin(Angle), 0.0f };
            *VertexAt++ = { P, N, T, {}, {}, {}, White };

            *IndexAt++ = 0;
            *IndexAt++ = ((i + 1) % RadiusDivisionCount) + BaseIndex;
            *IndexAt++ = i + BaseIndex;
        }
    }

    // Cylinder
    {
        u32 BaseIndex = CapVertexCount;
        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            f32 CosA = Cos(Angle);
            f32 SinA = Sin(Angle);

            v3 P = { Radius * CosA, Radius * SinA, 0.0f };
            v3 N = { CosA, SinA, 0.0f };
            v4 T = { -SinA, CosA, 0.0f, 1.0f };

            *VertexAt++ = { P, N, T, {}, {}, {}, White };
            *VertexAt++ = { P + v3{ 0.0f, 0.0f, Height }, N, T, {}, {}, {}, White };

            u32 i00 = 2*i + 0 + BaseIndex;
            u32 i01 = 2*i + 1 + BaseIndex;
            u32 i10 = 2*((i + 1) % RadiusDivisionCount) + 0 + BaseIndex;
            u32 i11 = 2*((i + 1) % RadiusDivisionCount) + 1 + BaseIndex;

            *IndexAt++ = i00;
            *IndexAt++ = i11;
            *IndexAt++ = i01;
                        
            *IndexAt++ = i00;
            *IndexAt++ = i10;
            *IndexAt++ = i11;
        }
    }

    // Tip
    {
        u32 BaseIndex = CapVertexCount + CylinderVertexCount;
        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            f32 CosA = Cos(Angle);
            f32 SinA = Sin(Angle);

            v3 P1 = { Radius * CosA, Radius * SinA, Height };
            v3 P2 = { TipRadius * CosA, TipRadius * SinA, Height  };

            v3 N = { 0.0f, 0.0f, -1.0f };
            v4 T = { 1.0f, 0.0f, 0.0f, 1.0f };
            *VertexAt++ = { P1, N, T, {}, {}, {}, White };
            *VertexAt++ = { P2, N, T, {}, {}, {}, White };

            u32 i00 = 2*i + 0 + BaseIndex;
            u32 i01 = 2*i + 1 + BaseIndex;
            u32 i10 = 2*((i + 1) % RadiusDivisionCount) + 0 + BaseIndex;
            u32 i11 = 2*((i + 1) % RadiusDivisionCount) + 1 + BaseIndex;

            *IndexAt++ = i00;
            *IndexAt++ = i11;
            *IndexAt++ = i01;
                        
            *IndexAt++ = i00;
            *IndexAt++ = i10;
            *IndexAt++ = i11;
        }

        BaseIndex += TipCapVertexCount;

        f32 Slope = TipRadius / TipHeight;
        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            f32 CosA = Cos(Angle);
            f32 SinA = Sin(Angle);

            f32 CosT = Cos(2.0f * Pi * (i + 0.5f) / RadiusDivisionCount);
            f32 SinT = Sin(2.0f * Pi * (i + 0.5f) / RadiusDivisionCount);

            v3 P = { TipRadius * CosA, TipRadius * SinA, Height };
            v3 N = Normalize(v3{ CosA, SinA, Slope });
            v3 TipP = { 0.0f, 0.0f, Height + TipHeight };
            v3 TipN = Normalize(v3{ CosT, SinT, Slope });

            v4 T = { -SinA, CosA, 0.0f, 1.0f };

            *VertexAt++ = { P, N, T, {}, {}, {}, White };
            *VertexAt++ = { TipP, TipN, T, {}, {}, {}, White };

            *IndexAt++ = 2 * i + 0 + BaseIndex;
            *IndexAt++ = (2 * ((i + 1) % RadiusDivisionCount)) + 0 + BaseIndex;
            *IndexAt++ = 2 * i + 1 + BaseIndex;
        }
    }

    return(Result);
}

#define PARTICLE_BASE_PATH "data/kenney_particle-pack/PNG (Black background)/"
const char* ParticlePaths[Particle_COUNT] =
{
    [Particle_Circle01] = PARTICLE_BASE_PATH "circle_01.png",
    [Particle_Circle02] = PARTICLE_BASE_PATH "circle_02.png",
    [Particle_Circle03] = PARTICLE_BASE_PATH "circle_03.png",
    [Particle_Circle04] = PARTICLE_BASE_PATH "circle_04.png",
    [Particle_Circle05] = PARTICLE_BASE_PATH "circle_05.png",
    [Particle_Dirt01] = PARTICLE_BASE_PATH "dirt_01.png",
    [Particle_Dirt02] = PARTICLE_BASE_PATH "dirt_02.png",
    [Particle_Dirt03] = PARTICLE_BASE_PATH "dirt_03.png",
    [Particle_Fire01] = PARTICLE_BASE_PATH "fire_01.png",
    [Particle_Fire02] = PARTICLE_BASE_PATH "fire_02.png",
    [Particle_Flame01] = PARTICLE_BASE_PATH "flame_01.png",
    [Particle_Flame02] = PARTICLE_BASE_PATH "flame_02.png",
    [Particle_Flame03] = PARTICLE_BASE_PATH "flame_03.png",
    [Particle_Flame04] = PARTICLE_BASE_PATH "flame_04.png",
    [Particle_Flame05] = PARTICLE_BASE_PATH "flame_05.png",
    [Particle_Flame06] = PARTICLE_BASE_PATH "flame_06.png",
    [Particle_Flare01] = PARTICLE_BASE_PATH "flare_01.png",
    [Particle_Light01] = PARTICLE_BASE_PATH "light_01.png",
    [Particle_Light02] = PARTICLE_BASE_PATH "light_02.png",
    [Particle_Light03] = PARTICLE_BASE_PATH "light_03.png",
    [Particle_Magic01] = PARTICLE_BASE_PATH "magic_01.png",
    [Particle_Magic02] = PARTICLE_BASE_PATH "magic_02.png",
    [Particle_Magic03] = PARTICLE_BASE_PATH "magic_03.png",
    [Particle_Magic04] = PARTICLE_BASE_PATH "magic_04.png",
    [Particle_Magic05] = PARTICLE_BASE_PATH "magic_05.png",
    [Particle_Muzzle01] = PARTICLE_BASE_PATH "muzzle_01.png",
    [Particle_Muzzle02] = PARTICLE_BASE_PATH "muzzle_02.png",
    [Particle_Muzzle03] = PARTICLE_BASE_PATH "muzzle_03.png",
    [Particle_Muzzle04] = PARTICLE_BASE_PATH "muzzle_04.png",
    [Particle_Muzzle05] = PARTICLE_BASE_PATH "muzzle_05.png",
    [Particle_Scorch01] = PARTICLE_BASE_PATH "scorch_01.png",
    [Particle_Scorch02] = PARTICLE_BASE_PATH "scorch_02.png",
    [Particle_Scorch03] = PARTICLE_BASE_PATH "scorch_03.png",
    [Particle_Scratch01] = PARTICLE_BASE_PATH "scratch_01.png",
    [Particle_Slash01] = PARTICLE_BASE_PATH "slash_01.png",
    [Particle_Slash02] = PARTICLE_BASE_PATH "slash_02.png",
    [Particle_Slash03] = PARTICLE_BASE_PATH "slash_03.png",
    [Particle_Slash04] = PARTICLE_BASE_PATH "slash_04.png",
    [Particle_Smoke01] = PARTICLE_BASE_PATH "smoke_01.png",
    [Particle_Smoke02] = PARTICLE_BASE_PATH "smoke_02.png",
    [Particle_Smoke03] = PARTICLE_BASE_PATH "smoke_03.png",
    [Particle_Smoke04] = PARTICLE_BASE_PATH "smoke_04.png",
    [Particle_Smoke05] = PARTICLE_BASE_PATH "smoke_05.png",
    [Particle_Smoke06] = PARTICLE_BASE_PATH "smoke_06.png",
    [Particle_Smoke07] = PARTICLE_BASE_PATH "smoke_07.png",
    [Particle_Smoke08] = PARTICLE_BASE_PATH "smoke_08.png",
    [Particle_Smoke09] = PARTICLE_BASE_PATH "smoke_09.png",
    [Particle_Smoke10] = PARTICLE_BASE_PATH "smoke_10.png",
    [Particle_Spark01] = PARTICLE_BASE_PATH "spark_01.png",
    [Particle_Spark02] = PARTICLE_BASE_PATH "spark_02.png",
    [Particle_Spark03] = PARTICLE_BASE_PATH "spark_03.png",
    [Particle_Spark04] = PARTICLE_BASE_PATH "spark_04.png",
    [Particle_Spark05] = PARTICLE_BASE_PATH "spark_05.png",
    [Particle_Spark06] = PARTICLE_BASE_PATH "spark_06.png",
    [Particle_Spark07] = PARTICLE_BASE_PATH "spark_07.png",
    [Particle_Star01] = PARTICLE_BASE_PATH "star_01.png",
    [Particle_Star02] = PARTICLE_BASE_PATH "star_02.png",
    [Particle_Star03] = PARTICLE_BASE_PATH "star_03.png",
    [Particle_Star04] = PARTICLE_BASE_PATH "star_04.png",
    [Particle_Star05] = PARTICLE_BASE_PATH "star_05.png",
    [Particle_Star06] = PARTICLE_BASE_PATH "star_06.png",
    [Particle_Star07] = PARTICLE_BASE_PATH "star_07.png",
    [Particle_Star08] = PARTICLE_BASE_PATH "star_08.png",
    [Particle_Star09] = PARTICLE_BASE_PATH "star_09.png",
    [Particle_Symbol01] = PARTICLE_BASE_PATH "symbol_01.png",
    [Particle_Symbol02] = PARTICLE_BASE_PATH "symbol_02.png",
    [Particle_Trace01] = PARTICLE_BASE_PATH "trace_01.png",
    [Particle_Trace02] = PARTICLE_BASE_PATH "trace_02.png",
    [Particle_Trace03] = PARTICLE_BASE_PATH "trace_03.png",
    [Particle_Trace04] = PARTICLE_BASE_PATH "trace_04.png",
    [Particle_Trace05] = PARTICLE_BASE_PATH "trace_05.png",
    [Particle_Trace06] = PARTICLE_BASE_PATH "trace_06.png",
    [Particle_Trace07] = PARTICLE_BASE_PATH "trace_07.png",
    [Particle_Twirl01] = PARTICLE_BASE_PATH "twirl_01.png",
    [Particle_Twirl02] = PARTICLE_BASE_PATH "twirl_02.png",
    [Particle_Twirl03] = PARTICLE_BASE_PATH "twirl_03.png",
    [Particle_Window01] = PARTICLE_BASE_PATH "window_01.png",
    [Particle_Window02] = PARTICLE_BASE_PATH "window_02.png",
    [Particle_Window03] = PARTICLE_BASE_PATH "window_03.png",
    [Particle_Window04] = PARTICLE_BASE_PATH "window_04.png",
};
#undef PARTICLE_BASE_PATH