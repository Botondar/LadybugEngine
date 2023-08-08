internal mesh CreateSphereMesh(memory_arena* Arena);
internal mesh CreateArrowMesh(memory_arena* Arena);

// NOTE(boti): This is pretty much the only place where we include something locally, because in the future Nvtt should be removed
// from the runtime
#include <nvtt/nvtt.h>

struct NvttOutputHandler : public nvtt::OutputHandler
{
    u64 MemorySize;
    u64 MemoryAt;
    u8* Memory;

    NvttOutputHandler(u64 MemorySize, void* Memory) :
        MemorySize(MemorySize),
    MemoryAt(0),
    Memory((u8*)Memory)
    { 
    }

    void beginImage(int Size, int Width, int Height, int Depth, int Face, int Mip) override
    {
    }

    bool writeData(const void* Data, int iSize) override
    {
        bool Result = false;

        u64 Size = (u64)iSize;
        if (MemorySize - MemoryAt >= Size)
        {
            memcpy(Memory + MemoryAt, Data, Size);
            MemoryAt += Size;
            Result = true;
        }
        return Result;
    }

    void endImage() override
    {
    }
};

lbfn b32 InitializeAssets(assets* Assets, renderer* Renderer, memory_arena* Scratch)
{
    b32 Result = true;

    // Default textures
    {
        u32 Value = 0xFFFFFFFFu;
        texture_id Whiteness = PushTexture(Renderer, TextureFlag_None, 1, 1, 1, 1, Format_R8G8B8A8_SRGB, {}, &Value);

        Assets->DefaultDiffuseID = Whiteness;
        Assets->DefaultMetallicRoughnessID = Whiteness;
    }
    {
        u16 Value = 0x8080u;
        Assets->DefaultNormalID = PushTexture(Renderer, TextureFlag_None, 1, 1, 1, 1, Format_R8G8_UNorm, {}, &Value);
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
        NullAnimation->KeyFrameTimestamps = PushArray<f32>(Assets->Arena, 1);
        NullAnimation->KeyFrameTimestamps[0] = 0.0f;
        NullAnimation->KeyFrames = PushArray<animation_key_frame>(Assets->Arena, 1);
        for (u32 JointIndex = 0; JointIndex < skin::MaxJointCount; JointIndex++)
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
        // TODO(boti): For now we know that the texture pack we're using is 512x512, 
        // but we may want to figure out some way for the user code to pack texture arrays/atlases dynamically
        u32 ParticleWidth = 512;
        u32 ParticleHeight = 512;
        u32 ParticleCount = Particle_COUNT;

        umm ImageSize = ParticleWidth * ParticleHeight;
        umm MemorySize = ImageSize * ParticleCount;
        void* Memory = PushSize(Scratch, MemorySize, 0x100);

        u8* MemoryAt = (u8*)Memory;
        for (u32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
        {
            s32 Width, Height, ChannelCount;
            u8* Texels = stbi_load(ParticlePaths[ParticleIndex], &Width, &Height, &ChannelCount, 1);
            if (Texels)
            {
                Assert(((u32)Width == ParticleWidth) && ((u32)Height == ParticleHeight));
                memcpy(MemoryAt, Texels, ImageSize);
            }
            else
            {
                UnhandledError("Failed to load particle texture");
            }
            stbi_image_free(Texels);
            MemoryAt += ImageSize;
        }

        Assets->ParticleArrayID = PushTexture(
            Renderer, TextureFlag_Special,
            ParticleWidth, ParticleHeight, 1, ParticleCount, 
            Format_R8_UNorm, { Swizzle_One, Swizzle_One, Swizzle_One, Swizzle_R },
            Memory);
        if (!IsValid(Assets->ParticleArrayID))
        {
            UnhandledError("Failed to create particles texture");
        }
    }

    LoadDebugFont(Scratch, Assets, Renderer, "data/liberation-mono.lbfnt");

    // Default meshes
    {
        mesh ArrowMesh = CreateArrowMesh(Scratch);
        geometry_buffer_allocation ArrowAllocation = UploadVertexData(Renderer,
                                                                      ArrowMesh.VertexCount, ArrowMesh.VertexData, 
                                                                      ArrowMesh.IndexCount, ArrowMesh.IndexData);

        Assert(Assets->MeshCount < Assets->MaxMeshCount);
        Assets->ArrowMeshID = Assets->MeshCount++;
        Assets->Meshes[Assets->ArrowMeshID] = ArrowAllocation;
        Assets->MeshBoxes[Assets->ArrowMeshID] = ArrowMesh.Box;
        Assets->MeshMaterialIndices[Assets->ArrowMeshID] = 0;
    }

    {
        mesh SphereMesh = CreateSphereMesh(Scratch);
        geometry_buffer_allocation SphereAllocation = UploadVertexData(Renderer,
                                                                       SphereMesh.VertexCount, SphereMesh.VertexData,
                                                                       SphereMesh.IndexCount, SphereMesh.IndexData);

        Assert(Assets->MeshCount < Assets->MaxMeshCount);
        Assets->SphereMeshID = Assets->MeshCount++;
        Assets->Meshes[Assets->SphereMeshID] = SphereAllocation;
        Assets->MeshBoxes[Assets->SphereMeshID] = SphereMesh.Box;
        Assets->MeshMaterialIndices[Assets->SphereMeshID] = 0;
    }

    return(Result);
}

static void LoadDebugFont(memory_arena* Arena, assets* Assets, renderer* Renderer, const char* Path)
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
                if (Font->Glyphs[i].P0.x > Font->Glyphs[i].P1.x)
                {
                    Font->Glyphs[i].P0 = { 0.0f, 0.0f };
                    Font->Glyphs[i].P1 = { 0.0f, 0.0f };
                }
            }

            Assets->DefaultFontTextureID = PushTexture(Renderer, TextureFlag_None,
                                                       FontFile->Bitmap.Width, FontFile->Bitmap.Height, 1, 1,
                                                       Format_R8_UNorm, 
                                                       {
                                                           .R = Swizzle_One,
                                                           .G = Swizzle_One,
                                                           .B = Swizzle_One,
                                                           .A = Swizzle_R,
                                                       },
                                                       FontFile->Bitmap.Bitmap);
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

// TODO(boti): remove the renderer from here
internal void DEBUGLoadTestScene(memory_arena* Scratch, assets* Assets, game_world* World, renderer* Renderer, const char* ScenePath, m4 BaseTransform)
{
    constexpr u64 PathBuffSize = 256;
    char PathBuff[PathBuffSize];
    u64 PathDirectoryLength = 0;
    for (u64 i = 0; ScenePath[i]; i++)
    {
        if (ScenePath[i] == '/' || ScenePath[i] == '\\')
        {
            PathDirectoryLength = i + 1;
        }
    }
    strncpy(PathBuff, ScenePath, PathDirectoryLength);

    u64 FilenameBuffSize = PathBuffSize - PathDirectoryLength;
    char* Filename = PathBuff + PathDirectoryLength;

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
    u32 BaseMeshIndex = Assets->MeshCount;
    u32 BaseMaterialIndex = Assets->MaterialCount;

    buffer* Buffers = PushArray<buffer>(Scratch, GLTF.BufferCount, MemPush_Clear);
    for (u32 BufferIndex = 0; BufferIndex < GLTF.BufferCount; BufferIndex++)
    {
        string URI = GLTF.Buffers[BufferIndex].URI;
        if (URI.Length <= FilenameBuffSize)
        {
            strncpy(Filename, URI.String, URI.Length);
            Filename[URI.Length] = 0;
            Buffers[BufferIndex] = Platform.LoadEntireFile(PathBuff, Scratch);
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

    enum class texture_type : u32
    {
        Diffuse,
        Normal,
        Material,
    };

    auto LoadAndUploadTexture = [&](u32 TextureIndex, texture_type Type, gltf_alpha_mode AlphaMode) -> texture_id
    {
        texture_id Result = { 0 };

        memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Scratch);
        if (TextureIndex < GLTF.TextureCount)
        {
            gltf_texture* Texture = GLTF.Textures + TextureIndex;
            if (Texture->ImageIndex >= GLTF.ImageCount) UnhandledError("Invalid glTF image index");

            gltf_image* Image = GLTF.Images + Texture->ImageIndex;
            if (Image->BufferViewIndex != U32_MAX) UnimplementedCodePath;

            if (Image->URI.Length > FilenameBuffSize) UnhandledError("Invalid glTF image path");

            strncpy(Filename, Image->URI.String, Image->URI.Length);
            Filename[Image->URI.Length] = 0;

            nvtt::Surface Surface;
            bool HasAlpha = false;
            if (Surface.load(PathBuff, &HasAlpha, false, nullptr))
            {
#if 1
                constexpr int ResolutionLimit = 2048;
                while (Surface.width() > ResolutionLimit || Surface.height() > ResolutionLimit)
                {
                    Surface.buildNextMipmap(nvtt::MipmapFilter_Box);
                }
#endif

                int Width = Surface.width();
                int Height = Surface.height();
                int MipCount = Surface.countMipmaps(1);

                nvtt::Context Ctx(true);
                nvtt::CompressionOptions CompressionOptions;
                nvtt::OutputOptions OutputOptions;

                nvtt::Format CompressFormat = nvtt::Format_RGB;
                format Format = Format_Undefined;
                switch (Type)
                {
                    case texture_type::Diffuse:
                    {
                        if (AlphaMode == GLTF_ALPHA_MODE_OPAQUE)
                        {
                            Format = Format_BC1_RGB_SRGB;
                            CompressFormat = nvtt::Format_BC1;
                        }
                        else
                        {
                            Format =  Format_BC3_SRGB; 
                            CompressFormat = nvtt::Format_BC3;
                        }
                    } break;
                    case texture_type::Normal:
                    {
                        Format = Format_BC5_UNorm; 
                        CompressFormat = nvtt::Format_BC5;
                    } break;
                    case texture_type::Material:
                    {
                        Format = Format_BC3_UNorm; 
                        CompressFormat = nvtt::Format_BC3;
                    } break;
                }

                CompressionOptions.setFormat(CompressFormat);
                CompressionOptions.setQuality(nvtt::Quality_Fastest);

                u64 Size = (u64)Ctx.estimateSize(Surface, MipCount, CompressionOptions);
                void* Texels = PushSize(Scratch, Size, 64);
                NvttOutputHandler OutputHandler(Size, Texels);
                OutputOptions.setOutputHandler(&OutputHandler);

                for (int Mip = 0; Mip < MipCount; Mip++)
                {
                    bool CompressionResult = Ctx.compress(Surface, 0, Mip, CompressionOptions, OutputOptions);
                    if (!CompressionResult)
                    {
                        UnhandledError("Failed to compress mip level");
                    }

                    if (Mip < MipCount - 1)
                    {
                        Surface.buildNextMipmap(nvtt::MipmapFilter_Box);
                    }
                }

                Result = PushTexture(Renderer, TextureFlag_None, (u32)Width, (u32)Height, (u32)MipCount, 1, Format, {}, Texels);
            }
            else
            {
                UnhandledError("Couldn't load image");
            }
        }
        else
        {
            //Assert(!"WARNING: default texture");
        }

        RestoreArena(Scratch, Checkpoint);
        return Result;
    };

    texture_id* TextureTable = PushArray<texture_id>(Scratch, GLTF.TextureCount);
    for (u32 i = 0; i < GLTF.TextureCount; i++)
    {
        TextureTable[i].Value = INVALID_INDEX_U32;
    }

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

            Material->Emissive = SrcMaterial->EmissiveFactor;
            Material->DiffuseColor = PackRGBA(SrcMaterial->BaseColorFactor);
            Material->BaseMaterial = PackRGBA(v4{ 1.0f, SrcMaterial->RoughnessFactor, SrcMaterial->MetallicFactor, 1.0f });
            Material->DiffuseID = Assets->DefaultDiffuseID;
            Material->NormalID = Assets->DefaultNormalID;
            Material->MetallicRoughnessID = Assets->DefaultMetallicRoughnessID;

            if (SrcMaterial->BaseColorTexture.TextureIndex != U32_MAX)
            {
                texture_id* Texture = TextureTable + SrcMaterial->BaseColorTexture.TextureIndex;
                if (Texture->Value == INVALID_INDEX_U32)
                {
                    *Texture = LoadAndUploadTexture(SrcMaterial->BaseColorTexture.TextureIndex, texture_type::Diffuse, SrcMaterial->AlphaMode);
                }
                Material->DiffuseID = *Texture;
            }
            if (SrcMaterial->NormalTexture.TextureIndex != U32_MAX)
            {
                texture_id* Texture = TextureTable + SrcMaterial->NormalTexture.TextureIndex;
                if (Texture->Value == INVALID_INDEX_U32)
                {
                    *Texture = LoadAndUploadTexture(SrcMaterial->NormalTexture.TextureIndex, texture_type::Normal, SrcMaterial->AlphaMode);
                }
                Material->NormalID = *Texture;
            }
            if (SrcMaterial->MetallicRoughnessTexture.TextureIndex != U32_MAX)
            {
                texture_id* Texture = TextureTable + SrcMaterial->MetallicRoughnessTexture.TextureIndex;
                if (Texture->Value == INVALID_INDEX_U32)
                {
                    *Texture = LoadAndUploadTexture(SrcMaterial->MetallicRoughnessTexture.TextureIndex, texture_type::Material, SrcMaterial->AlphaMode);
                }
                Material->MetallicRoughnessID = *Texture;
            }
        }
        else
        {
            UnhandledError("Out of material pool");
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

            vertex* VertexData = PushArray<vertex>(Scratch, VertexCount);
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

                Verify(WeightsAccessor->BufferView < GLTF.BufferViewCount);
                gltf_buffer_view* WeightsView = GLTF.BufferViews + WeightsAccessor->BufferView;
                Verify(WeightsView->BufferIndex < GLTF.BufferCount);
                buffer* WeightsBuffer = Buffers + WeightsView->BufferIndex;
                Verify(WeightsAccessor->ByteOffset + WeightsView->Offset + VertexCount * WeightsView->Stride <= WeightsBuffer->Size);
                void* WeightsAt = OffsetPtr(WeightsBuffer->Data, WeightsView->Offset + WeightsAccessor->ByteOffset);

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

                    JointsAt = OffsetPtr(JointsAt, JointsView->Stride);
                    WeightsAt = OffsetPtr(WeightsAt, WeightsView->Stride);
                }
            }

            u32 IndexCount = 0;
            u32* IndexData = nullptr;
            if (Primitive->IndexBufferIndex == U32_MAX)
            {
                IndexCount = VertexCount;
                IndexData = PushArray<u32>(Scratch, IndexCount);
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
                IndexData = PushArray<u32>(Scratch, IndexCount);
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
                v3* Tangents   = PushArray<v3>(Scratch, VertexCount, MemPush_Clear);
                v3* Bitangents = PushArray<v3>(Scratch, VertexCount, MemPush_Clear);
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

                    f32 InvDetT = 1.0f / (dT1.x * dT2.y - dT1.y * dT2.x);

                    v3 Tangent = 
                    {
                        (dP1.x * dT2.y - dP2.x * dT1.y) * InvDetT,
                        (dP1.y * dT2.y - dP2.y * dT1.y) * InvDetT,
                        (dP1.z * dT2.y - dP2.z * dT1.y) * InvDetT,
                    };

                    v3 Bitangent = 
                    {
                        (dP1.x * dT2.x - dP2.x * dT1.x) * InvDetT,
                        (dP1.y * dT2.x - dP2.y * dT1.x) * InvDetT,
                        (dP1.z * dT2.x - dP2.z * dT1.x) * InvDetT,
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
                    f32 w = Dot(B, Bitangents[i]) < 0.0f ? -1.0f : 1.0f;
                    V->T = { T.x, T.y, T.z, w };
                }
            }

            if (Assets->MeshCount < Assets->MaxMeshCount)
            {
                u32 MeshID = Assets->MeshCount++;
                Assets->Meshes[MeshID] = UploadVertexData(Renderer,
                                                          VertexCount, VertexData,
                                                          IndexCount, IndexData);
                Assets->MeshBoxes[MeshID] = Box;
                Assets->MeshMaterialIndices[MeshID] = Primitive->MaterialIndex + BaseMaterialIndex;
            }
            else
            {
                UnhandledError("Out of mesh pool memory");
            }

            RestoreArena(Scratch, Checkpoint);
        }
    }

    u32 BaseSkinIndex = Assets->SkinCount;
    for (u32 SkinIndex = 0; SkinIndex < GLTF.SkinCount; SkinIndex++)
    {
        gltf_skin* Skin = GLTF.Skins + SkinIndex;
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
            if (Skin->JointCount <= skin::MaxJointCount)
            {
                SkinAsset->JointCount = Skin->JointCount;
                u32 Stride = View->Stride ? View->Stride : sizeof(m4);

                {
                    u8* SrcAt = (u8*)Buffer->Data + View->Offset + Accessor->ByteOffset;
                    m4* DstAt = SkinAsset->InverseBindMatrices;
                    u32 Count = SkinAsset->JointCount;
                    while (Count--)
                    {
                        *DstAt++ = *(m4*)SrcAt;
                        SrcAt += Stride;
                    }
                }

                for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                {
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
                            // TODO(boti): Decompose the matrix here?
                            SkinAsset->BindPose[JointIndex] = 
                            {
                                .Rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
                                .Position = { 0.0f, 0.0f, 0.0f },
                                .Scale = { 1.0f, 1.0f, 1.0f },
                            };
                        }
                    }
                    else
                    {
                        UnhandledError("Invalid joint node index in glTF skin");
                    }
                }

                // NOTE(boti): We clear the joints here to later verify that the joint node has a single parent
                for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                {
                    SkinAsset->JointParents[JointIndex] = U32_MAX;
                }

                // Build the joint hierarchy
                // TODO(boti): We need to handle the case where the joints don't form a closed hierarchy 
                // (i.e. bones are parented to nodes not part of the skeleton)
                for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                {
                    u32 NodeIndex = Skin->JointIndices[JointIndex];
                    gltf_node* Node = GLTF.Nodes + NodeIndex;
                    for (u32 ChildIt = 0; ChildIt < Node->ChildrenCount; ChildIt++)
                    {
                        u32 ChildIndex = Node->Children[ChildIt];
                        for (u32 JointIt = 0; JointIt < Skin->JointCount; JointIt++)
                        {
                            if (Skin->JointIndices[JointIt] == ChildIndex)
                            {
                                Verify(SkinAsset->JointParents[JointIt] == U32_MAX);
                                SkinAsset->JointParents[JointIt] = JointIndex;

                                // NOTE(boti): It makes thing easier if the joints are in order,
                                // so we currently only handle that case
                                Assert(JointIndex < JointIt);
                                break;
                            }
                        }
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

        f32* KeyFrameTimestamps = PushArray<f32>(Scratch, MaxKeyFrameCount);
        u32 KeyFrameCount = 1;
        KeyFrameTimestamps[0] = 0.0f;
        for (u32 SamplerIndex = 0; SamplerIndex < Animation->SamplerCount; SamplerIndex++)
        {
            gltf_animation_sampler* Sampler = Animation->Samplers + SamplerIndex;
            gltf_accessor* TimestampAccessor = GLTF.Accessors + Sampler->InputAccessorIndex;
            gltf_buffer_view* View = GLTF.BufferViews + TimestampAccessor->BufferView;
            buffer* Buffer = Buffers + View->BufferIndex;

            void* At = OffsetPtr(Buffer->Data, View->Offset + TimestampAccessor->ByteOffset);
            u32 Count = TimestampAccessor->Count;
            while (Count--)
            {
                f32 Timestamp = *(f32*)At;
                Assert(Timestamp >= 0.0f);
                At = OffsetPtr(At, View->Stride);
                
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
            AnimationAsset->KeyFrameTimestamps = PushArray<f32>(Assets->Arena, KeyFrameCount);
            AnimationAsset->KeyFrames = PushArray<animation_key_frame>(Assets->Arena, KeyFrameCount);
            memcpy(AnimationAsset->KeyFrameTimestamps, KeyFrameTimestamps, KeyFrameCount * sizeof(f32));

            // TODO(boti): We need to rework the import here to actually figure out which joints are active
            // during an animation and which ones aren't, and set the mask appropriately!
            // Currently when a joint is missing from an animation, the bind-pose is broadcast through all key-frames
            // of the animation.
            memset(&AnimationAsset->ActiveJoints, 0xFF, sizeof(AnimationAsset->ActiveJoints));

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

                    gltf_buffer_view* TransformView = GLTF.BufferViews + TransformAccessor->BufferView;
                    buffer* TransformBuffer = Buffers + TransformView->BufferIndex;
                    void* SamplerTransformAt = OffsetPtr(TransformBuffer->Data, TransformView->Offset + TransformAccessor->ByteOffset);
                    
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
                            SamplerTimestampAt = OffsetPtr(SamplerTimestampAt, TimestampView->Stride);
                            SamplerTransformAt = OffsetPtr(SamplerTransformAt, TransformView->Stride);
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

    m4* ParentTransforms = PushArray<m4>(Scratch, GLTF.NodeCount);
    u32* NodeParents = PushArray<u32>(Scratch, GLTF.NodeCount);

    u32 NodeQueueCount = 0;
    u32* NodeQueue = PushArray<u32>(Scratch, GLTF.NodeCount);
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

            if (World->EntityCount + Mesh->PrimitiveCount <= World->MaxEntityCount)
            {
                if (Node->SkinIndex == U32_MAX)
                {
                    for (u32 PrimitiveIndex = 0; PrimitiveIndex < Mesh->PrimitiveCount; PrimitiveIndex++)
                    {
                        World->Entities[World->EntityCount++] = 
                        {
                            .Flags = EntityFlag_Mesh,
                            .Transform = NodeTransform,
                            .MeshID = BaseMeshIndex + (MeshOffset + PrimitiveIndex),
                        };
                    }
                }
                else
                {
                    if (Mesh->PrimitiveCount != 1)
                    {
                        UnimplementedCodePath;
                    }

                    u32 MeshID = BaseMeshIndex + MeshOffset;
                    u32 SkinID = BaseSkinIndex + Node->SkinIndex;

                    // NOTE(boti): set default animation (for debugging!!!)
                    u32 AnimationID = U32_MAX;
                    for (u32 AnimationIt = 0; AnimationIt < Assets->AnimationCount; AnimationIt++)
                    {
                        animation* Animation = Assets->Animations + AnimationIt;
                        if (Animation->SkinID == SkinID)
                        {
                            AnimationID = AnimationIt;
                            break;
                        }
                    }

                    World->Entities[World->EntityCount++] = 
                    {
                        .Flags = EntityFlag_Mesh|EntityFlag_Skin,
                        .Transform = NodeTransform,
                        .MeshID = MeshID,
                        .SkinID = SkinID,
                        .CurrentAnimationID = AnimationID,
                        .DoAnimation = true, // NOTE(boti): for debug, default should be false
                        .AnimationCounter = 0.0f,
                        
                    };
                }
            }
            else
            {
                UnhandledError("Out of entity pool");
            }
        }
    }
}


internal mesh CreateSphereMesh(memory_arena* Arena)
{
    mesh Result = {};

    Result.Box = { { -1.0f, -1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f } };

    constexpr u32 XCount = 16;
    constexpr u32 YCount = 16;

    Result.VertexCount = XCount * YCount;
    Result.VertexData = PushArray<vertex>(Arena, Result.VertexCount);

    Result.IndexCount = 6 * Result.VertexCount;
    Result.IndexData = PushArray<u32>(Arena, Result.IndexCount);

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
                .T = { T.x, T.y, T.z, 1.0f }, 
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

internal mesh CreateArrowMesh(memory_arena* Arena)
{
    mesh Result = {};

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

    Result.VertexData = PushArray<vertex>(Arena, Result.VertexCount);
    Result.IndexData = PushArray<u32>(Arena, Result.IndexCount);

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