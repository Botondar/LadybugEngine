lbfn m4 GetLocalTransform(const camera* Camera)
{
    f32 SinYaw = Sin(Camera->Yaw);
    f32 CosYaw = Cos(Camera->Yaw);
    f32 SinPitch = Sin(Camera->Pitch);
    f32 CosPitch = Cos(Camera->Pitch);

    v3 LocalY = { 0.0f, 1.0f, 0.0f };
    v3 LocalZ = { -SinYaw * CosPitch, SinPitch, CosYaw * CosPitch };
    v3 LocalX = Normalize(Cross(LocalY, LocalZ));
    LocalY = Cross(LocalZ, LocalX);

    m4 BasisTransform = M4(
        LocalX.X, LocalY.X, LocalZ.X, 0.0f,
        LocalX.Y, LocalY.Y, LocalZ.Y, 0.0f,
        LocalX.Z, LocalY.Z, LocalZ.Z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    m4 Translation = M4(
        1.0f, 0.0f, 0.0f, Camera->P.X,
        0.0f, 1.0f, 0.0f, Camera->P.Y,
        0.0f, 0.0f, 1.0f, Camera->P.Z,
        0.0f, 0.0f, 0.0f, 1.0f);

    m4 Result = Translation * BasisTransform;
    return Result;
}

lbfn m4 GetTransform(const camera* Camera)
{
    f32 SinYaw = Sin(Camera->Yaw);
    f32 CosYaw = Cos(Camera->Yaw);
    f32 SinPitch = Sin(Camera->Pitch);
    f32 CosPitch = Cos(Camera->Pitch);

    v3 Up = { 0.0f, 0.0f, 1.0f };
    v3 Forward = { SinYaw * CosPitch, CosYaw * CosPitch, SinPitch };
    v3 Right = Normalize(Cross(Forward, Up));
    Up = Cross(Right, Forward);

    m4 BasisTransform = M4(
        Right.X, -Up.X, Forward.X, 0.0f,
        Right.Y, -Up.Y, Forward.Y, 0.0f,
        Right.Z, -Up.Z, Forward.Z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    m4 Translation = M4(
        1.0f, 0.0f, 0.0f, Camera->P.X,
        0.0f, 1.0f, 0.0f, Camera->P.Y,
        0.0f, 0.0f, 1.0f, Camera->P.Z,
        0.0f, 0.0f, 0.0f, 1.0f);

    m4 Result = Translation * BasisTransform;
    return Result;
}

lbfn f32 SampleNoise(noise2* Noise, v2 P)
{
    v2 P0 = { Floor(P.X), Floor(P.Y) };
    v2 dP = P - P0;

    u32 X = (u32)(s32)P0.X;
    u32 Y = (u32)(s32)P0.Y;

    // TODO(boti): Pull this out once we have a use for it elsewhere
    // NOTE(boti): Murmur3
    auto Hash = [](u32 Seed, u32 X, u32 Y) -> u32
    {
        u32 Result = Seed;

        u32 Values[2] = { X, Y };
        for (u32 Index = 0; Index < 2; Index++)
        {
            u32 K = 0xCC9E2D51u * Values[Index];
            // TODO(boti): Rotate intrinsics
            K = (K << 15) | (K >> (32 - 15));
            K *= 0x1B873593u;

            Result ^= K;
            Result = (Result << 13) | (Result >> (32 - 13));
            Result = 5u*Result + 0xE6546B64u;
        }

        // NOTE(boti): Final mix step
        Result ^= Result >> 16;
        Result *= 0x85EBCA6Bu;
        Result ^= Result >> 13;
        Result *= 0xC2B2AE35u;
        Result ^= Result >> 16;
        return(Result);
    };

    u32 Hash00 = Hash(Noise->Seed, X + 0, Y + 0);
    u32 Hash10 = Hash(Noise->Seed, X + 1, Y + 0);
    u32 Hash01 = Hash(Noise->Seed, X + 0, Y + 1);
    u32 Hash11 = Hash(Noise->Seed, X + 1, Y + 1);

    v2 GradTable[4]
    {
        { -1.0f, -1.0f },
        { +1.0f, -1.0f },
        { -1.0f, +1.0f },
        { +1.0f, +1.0f },

    };
    v2 Grad00 = GradTable[Hash00 & 3];
    v2 Grad10 = GradTable[Hash10 & 3];
    v2 Grad01 = GradTable[Hash01 & 3];
    v2 Grad11 = GradTable[Hash11 & 3];

    f32 D00 = Dot(dP, Grad00);
    f32 D10 = Dot(dP - v2{ 1.0f, 0.0f }, Grad10);
    f32 D01 = Dot(dP - v2{ 0.0f, 1.0f }, Grad01);
    f32 D11 = Dot(dP - v2{ 1.0f, 1.0f }, Grad11);

    auto Fade = [](f32 X) -> f32
    {
        f32 Result = X*X*X * ((6.0f*X - 15.0f)*X + 10.0f);
        return(Result);
    };
    f32 U = Fade(dP.X);
    f32 V = Fade(dP.Y);
    f32 X0 = Lerp(D00, D10, U);
    f32 X1 = Lerp(D01, D11, U);
    f32 Result = Lerp(X0, X1, V);
    return(Result);
}

internal mesh_data 
GenerateTerrainChunk(height_field* Field, memory_arena* Arena)
{
    mesh_data Result = {};

    Result.Box = { { +F32_MAX_NORMAL, +F32_MAX_NORMAL, +F32_MAX_NORMAL }, { -F32_MAX_NORMAL, -F32_MAX_NORMAL, -F32_MAX_NORMAL } };

    Result.VertexCount = Field->TexelCountX*Field->TexelCountY;
    Result.VertexData = PushArray(Arena, 0, vertex, Result.VertexCount);

    auto SampleHeight = [](height_field* Field, s32 X, s32 Y)
    {
        X = Clamp(X, 0, (s32)Field->TexelCountX - 1);
        Y = Clamp(Y, 0, (s32)Field->TexelCountY - 1);
        f32 Result = Field->HeightData[X + Y*Field->TexelCountX];
        return(Result);
    };

    f32 dS = 1.0f / Field->TexelsPerMeter;
    vertex* VertexAt = Result.VertexData;
    for (s32 Y = 0; Y < (s32)Field->TexelCountY; Y++)
    {
        for (s32 X = 0; X < (s32)Field->TexelCountX; X++)
        {
            v3 P = { dS*X, dS*Y, SampleHeight(Field, X, Y) };
            Result.Box.Min = 
            { 
                Min(Result.Box.Min.X, P.X),
                Min(Result.Box.Min.Y, P.Y),
                Min(Result.Box.Min.Z, P.Z),
            };
            Result.Box.Max = 
            { 
                Max(Result.Box.Max.X, P.X),
                Max(Result.Box.Max.Y, P.Y),
                Max(Result.Box.Max.Z, P.Z),
            };

            v3 PXn = { P.X - dS, P.Y, SampleHeight(Field, X - 1, Y) };
            v3 PXp = { P.X + dS, P.Y, SampleHeight(Field, X + 1, Y) };
            v3 PYn = { P.X, P.Y - dS, SampleHeight(Field, X, Y - 1) };
            v3 PYp = { P.X, P.Y + dS, SampleHeight(Field, X, Y + 1) };

            v3 T = NOZ(PXp - PXn);
            v3 B = NOZ(PYp - PYn);
            v3 N = NOZ(Cross(T, B));

            *VertexAt++ = 
            {
                .P = P,
                .N = N, 
                .T = { T.X, T.Y, T.Z, 1.0f },
                .TexCoord = { 0.5f * P.X, 0.5f * P.Y },
                .Color = PackRGBA8(0xFF, 0xFF, 0xFF),
            };
        }
    }

    u32 QuadCountX = Field->TexelCountX - 1;
    u32 QuadCountY = Field->TexelCountY - 1;
    Result.IndexCount = 6 * QuadCountX*QuadCountY;
    Result.IndexData = PushArray(Arena, 0, vert_index, Result.IndexCount);
    auto GetIndex = [=](u32 X, u32 Y) -> u32
    {
        u32 Result = X + Y*Field->TexelCountX;
        return(Result);
    };

    vert_index* IndexAt = Result.IndexData;
    for (u32 Y = 0; Y < QuadCountY; Y++)
    {
        for (u32 X = 0; X < QuadCountX; X++)
        {
            *IndexAt++ = GetIndex(X + 0, Y + 0);
            *IndexAt++ = GetIndex(X + 1, Y + 0);
            *IndexAt++ = GetIndex(X + 0, Y + 1);
            *IndexAt++ = GetIndex(X + 1, Y + 0);
            *IndexAt++ = GetIndex(X + 1, Y + 1);
            *IndexAt++ = GetIndex(X + 0, Y + 1);
        }
    }
    return(Result);
}

lbfn u32 
MakeParticleSystem(game_world* World, entity_id ParentID, particle_system_type Type, 
                   v3 EmitterOffset, mmbox Bounds)
{
    u32 Result = U32_MAX;
    if (World->ParticleSystemCount < World->MaxParticleSystemCount)
    {
        if (IsValid(ParentID))
        {
            Assert(ParentID.Value < World->EntityCount);
        }

        Result = World->ParticleSystemCount++;
        particle_system* ParticleSystem = World->ParticleSystems + Result;
        ParticleSystem->ParentID = ParentID;
        ParticleSystem->Type = Type;
        ParticleSystem->EmitterOffset = EmitterOffset;
        ParticleSystem->Bounds = Bounds;
        ParticleSystem->Counter = 0.0f;

        switch (Type)
        {
            case ParticleSystem_Undefined:
            {
                // Ignored
            } break;
            case ParticleSystem_Magic:
            {
                ParticleSystem->Mode = Billboard_ZAligned;
                ParticleSystem->ParticleHalfExtent = { 0.25f, 0.25f };
                ParticleSystem->EmissionRate = 1.0f / 144.0f;
                ParticleSystem->CullOutOfBoundsParticles = true;
                ParticleSystem->ParticleCount = ParticleSystem->MaxParticleCount;
                
            } break;
            case ParticleSystem_Fire:
            {
                ParticleSystem->Mode = Billboard_ViewAligned;
                ParticleSystem->ParticleHalfExtent = { 0.15f, 0.15f };
                ParticleSystem->ParticleCount = ParticleSystem->MaxParticleCount;
                ParticleSystem->CullOutOfBoundsParticles = false;
                ParticleSystem->EmissionRate = 1.0f / 30.0f;
            } break;
            InvalidDefaultCase;
        }
    }
    return(Result);
}

internal void 
DEBUGInitializeWorld(
    game_world* World, 
    assets* Assets, 
    render_frame* Frame, 
    memory_arena* Scratch,
    debug_scene_type Type,
    debug_scene_flags Flags)
{
    m4 YUpToZUp = M4(1.0f, 0.0f, 0.0f, 0.0f,
                     0.0f, 0.0f, -1.0f, 0.0f,
                     0.0f, 1.0f, 0.0f, 0.0f,
                     0.0f, 0.0f, 0.0f, 1.0f);

    switch (Type)
    {
        case DebugScene_None:
        {
            // Ignored
        } break;
        case DebugScene_TransmissionTest:
        {
            m4 Transform = YUpToZUp;
            DEBUGLoadTestScene(Scratch, Assets, World, Frame,
                               DEBUGLoad_AddNodesAsEntities,
                               "data/glTF-Sample-Assets/Models/TransmissionTest/glTF/TransmissionTest.gltf", Transform);
        } break;
        case DebugScene_Sponza:
        {
            m4 Transform = YUpToZUp;
            DEBUGLoadTestScene(Scratch, Assets, World, Frame,
                               DEBUGLoad_AddNodesAsEntities,
                               "data/glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf", Transform);
        } break;
        case DebugScene_Terrain:
        {
            // Load trees
            {
                #if 0
                const char* TreeFiles[] =
                {
                    "data/mighty_oak_trees/oaks.gltf",
                };
                #else
                const char* TreeFiles[] = 
                {
                    "data/orca/SpeedTree/european-linden/european-linden.gltf",
                    "data/orca/SpeedTree/japanese-maple/japanese-maple.gltf",
                    "data/orca/SpeedTree/red-maple-young/red-maple-young.gltf",
                    "data/orca/SpeedTree/white-oak/white-oak.gltf",
                };
                #endif

                for (u32 FileIndex = 0; FileIndex < CountOf(TreeFiles); FileIndex++)
                {
                    u32 BaseTreeIndex = Assets->ModelCount;
                    DEBUGLoadTestScene(Frame->Arena, Assets, World, Frame, 
                                       DEBUGLoad_None,
                                       TreeFiles[FileIndex],
                                       Identity4());

                    u32 TreeCount = Assets->ModelCount - BaseTreeIndex;
                    for (u32 TreeIndex = 0; TreeIndex < TreeCount; TreeIndex++)
                    {
                        if (Assets->TreeModelCount < Assets->MaxTreeModelCount)
                        {
                            Assets->TreeModels[Assets->TreeModelCount++] = BaseTreeIndex + TreeIndex;
                        }
                        else
                        {
                            Platform.DebugPrint("Too many trees!");
                        }
                    }
                }
            }

            // Height field
            {
                World->TerrainNoise = { 0 };
                World->HeightField.TexelCountX = 1024;
                World->HeightField.TexelCountY = 1024;
                World->HeightField.TexelsPerMeter = 4;
                u32 TexelCount = World->HeightField.TexelCountX * World->HeightField.TexelCountY;
                World->HeightField.HeightData = PushArray(World->Arena, 0, f32, TexelCount);

                for (u32 Y = 0; Y < World->HeightField.TexelCountY; Y++)
                {
                    for (u32 X = 0; X < World->HeightField.TexelCountX; X++)
                    {
                        u32 Index = X + Y*World->HeightField.TexelCountX;

                        f32 BaseFrequency = 1.0f / 256.0f;
                        f32 BaseAmplitude = 32.0f;
                        v2 P = (1.0f / World->HeightField.TexelsPerMeter) * v2{ (f32)X, (f32)Y };
                        f32 Height = 0.0f;

                        for (u32 OctaveIndex = 0; OctaveIndex < 12; OctaveIndex++)
                        {
                            f32 Mul = (f32)(1 << OctaveIndex);
                            f32 Frequency = BaseFrequency * Mul;
                            f32 Amplitude = BaseAmplitude / Mul;
                            Height += Amplitude * SampleNoise(&World->TerrainNoise, Frequency*P);
                            World->HeightField.HeightData[Index] = Height;
                        }
                    }
                }
            }

            #if 0
            texture_set_entry TextureSetEntries[TextureType_Count] =
            {
                [TextureType_Albedo]        = { "data/texture/TCom_Sand_Muddy2_2x2_4K_albedo.tif" },
                [TextureType_Normal]        = { "data/texture/TCom_Sand_Muddy2_2x2_4K_normal.tif" },
                [TextureType_RoMe]          = { "data/texture/TCom_Sand_Muddy2_2x2_4K_roughness.tif", TextureChannel_R, TextureChannel_Undefined },
                [TextureType_Occlusion]     = { "data/texture/TCom_Sand_Muddy2_2x2_4K_ao.tif" },
                [TextureType_Height]        = { "data/texture/TCom_Sand_Muddy2_2x2_4K_height.tif" },
                [TextureType_Transmission]  = {},
            };
            #elif 0
            texture_set_entry TextureSetEntries[TextureType_Count] =
            {
                [TextureType_Albedo]        = { "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_albedo.tif" },
                [TextureType_Normal]        = { "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_normal.tif" },
                [TextureType_RoMe]          = { "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_roughness.tif", TextureChannel_R, TextureChannel_Undefined },
                [TextureType_Occlusion]     = { "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_ao.tif" },
                [TextureType_Height]        = { "data/texture/TCom_Rock_CliffLayered_1.5x1.5_4K_height.tif" },
                [TextureType_Transmission]  = {},
            };
            #elif 0
            texture_set_entry TextureSetEntries[TextureType_Count] =
            {
                [TextureType_Albedo]        = { "data/texture/leafy-grass2-ue/leafy-grass2-albedo.png" },
                [TextureType_Normal]        = { "data/texture/leafy-grass2-ue/leafy-grass2-normal-dx.png" },
                [TextureType_RoMe]          = { "data/texture/leafy-grass2-ue/leafy-grass2-roughness.png", TextureChannel_R, TextureChannel_Undefined },
                [TextureType_Occlusion]     = { "data/texture/leafy-grass2-ue/leafy-grass2-ao.png" },
                [TextureType_Height]        = { "data/texture/leafy-grass2-ue/leafy-grass2-height.png" },
                [TextureType_Transmission]  = {},
            };
            #elif 1
            texture_set_entry TextureSetEntries[TextureType_Count] =
            {
                [TextureType_Albedo]        = { "data/texture/mixedmoss-ue4/mixedmoss-albedo2.png" },
                [TextureType_Normal]        = { "data/texture/mixedmoss-ue4/mixedmoss-normal2.png" },
                [TextureType_RoMe]          = { "data/texture/mixedmoss-ue4/mixedmoss-roughness.png", TextureChannel_R, TextureChannel_Undefined },
                [TextureType_Occlusion]     = { "data/texture/mixedmoss-ue4/mixedmoss-ao2.png" },
                [TextureType_Height]        = { "data/texture/mixedmoss-ue4/mixedmoss-height.png" },
                [TextureType_Transmission]  = {},
            };
            #endif
            texture_set TextureSet = DEBUGLoadTextureSet(Assets, Frame, TextureSetEntries);

            World->TerrainMaterialID = Assets->MaterialCount++;
            material* TerrainMaterial = Assets->Materials + World->TerrainMaterialID;
            TerrainMaterial->AlbedoID                   = TextureSet.IDs[TextureType_Albedo];
            TerrainMaterial->NormalID                   = TextureSet.IDs[TextureType_Normal];
            TerrainMaterial->MetallicRoughnessID        = TextureSet.IDs[TextureType_RoMe];
            TerrainMaterial->OcclusionID                = TextureSet.IDs[TextureType_Occlusion];
            TerrainMaterial->HeightID                   = TextureSet.IDs[TextureType_Height];
            TerrainMaterial->AlbedoSamplerID            = GetMaterialSamplerID(Wrap_Repeat, Wrap_Repeat, Wrap_Repeat);
            TerrainMaterial->NormalSamplerID            = GetMaterialSamplerID(Wrap_Repeat, Wrap_Repeat, Wrap_Repeat);
            TerrainMaterial->MetallicRoughnessSamplerID = GetMaterialSamplerID(Wrap_Repeat, Wrap_Repeat, Wrap_Repeat);
            TerrainMaterial->Transparency               = Transparency_Opaque;
            TerrainMaterial->Albedo                     = PackRGBA8(0xFF, 0xFF, 0xFF);
            TerrainMaterial->MetallicRoughness          = PackRGBA8(0xFF, 0xFF, 0x00);
            TerrainMaterial->Emission                   = { 0.0f, 0.0f, 0.0f };
            

            mesh_data TerrainMesh = GenerateTerrainChunk(&World->HeightField, Scratch);

            u32 ChunkMeshID = Assets->MeshCount++;
            mesh* Mesh = Assets->Meshes + ChunkMeshID;
            Mesh->Allocation = Platform.AllocateGeometry(Frame->Renderer, TerrainMesh.VertexCount, TerrainMesh.IndexCount);
            Mesh->BoundingBox = TerrainMesh.Box;
            Mesh->MaterialID = World->TerrainMaterialID;

            TransferGeometry(Frame, Mesh->Allocation, TerrainMesh.VertexData, TerrainMesh.IndexData);

            entity_id EntityID = { World->EntityCount++ };
            entity* TerrainEntity = World->Entities + EntityID.Value;
            TerrainEntity->Flags = EntityFlag_Mesh;
            f32 ExtentX = (f32)World->HeightField.TexelCountX / (f32)World->HeightField.TexelsPerMeter;
            f32 ExtentY = (f32)World->HeightField.TexelCountY / (f32)World->HeightField.TexelsPerMeter;
            TerrainEntity->Transform = M4(1.0f, 0.0f, 0.0f, -0.5f * ExtentX,
                                          0.0f, 1.0f, 0.0f, -0.5f * ExtentY,
                                          0.0f, 0.0f, 1.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);
            TerrainEntity->PieceCount = 1;
            TerrainEntity->Pieces[0].MeshID = ChunkMeshID;
            
            // Plant trees
            if (Assets->TreeModelCount)
            {
                v2 MinBounds = 
                {
                    TerrainEntity->Transform.P.X + TerrainMesh.Box.Min.X,
                    TerrainEntity->Transform.P.Y + TerrainMesh.Box.Min.Y,
                };

                v2 MaxBounds = 
                {
                    TerrainEntity->Transform.P.X + TerrainMesh.Box.Max.X,
                    TerrainEntity->Transform.P.Y + TerrainMesh.Box.Max.Y,
                };

                u32 TreeCountToGenerate = 2048;
                for (u32 TreeIndex = 0; TreeIndex < TreeCountToGenerate; TreeIndex++)
                {
                    u32 ModelIndex = RandU32(&World->GeneratorEntropy) % Assets->TreeModelCount;
                    model* Model = Assets->Models + Assets->TreeModels[ModelIndex];
                    v3 TreeMinP = { +F32_MAX_NORMAL, +F32_MAX_NORMAL, +F32_MAX_NORMAL };
                    v3 TreeMaxP = { -F32_MAX_NORMAL, -F32_MAX_NORMAL, -F32_MAX_NORMAL };
                    for (u32 MeshIndex = 0; MeshIndex < Model->MeshCount; MeshIndex++)
                    {
                        mesh* TreeMesh = Assets->Meshes + Model->Meshes[MeshIndex];
                        TreeMinP = Min(TreeMinP, TreeMesh->BoundingBox.Min);
                        TreeMaxP = Max(TreeMaxP, TreeMesh->BoundingBox.Max);
                    }

                    if (World->EntityCount < World->MaxEntityCount)
                    {
                        entity* Entity = World->Entities + World->EntityCount++;
                        memset(Entity, 0, sizeof(*Entity));
                        Entity->Flags = EntityFlag_Mesh;

                        v2 UV = { RandUnilateral(&World->GeneratorEntropy), RandUnilateral(&World->GeneratorEntropy) };
                        v3 P = 
                        { 
                            UV.X * (MaxBounds.X - MinBounds.X)  + MinBounds.X,
                            UV.Y * (MaxBounds.Y - MinBounds.Y)  + MinBounds.Y,
                            SampleHeight(&World->HeightField, UV) - 0.25f,
                        };

                        f32 Angle = 2.0f * Pi * RandUnilateral(&World->GeneratorEntropy);
                        f32 C = Cos(Angle);
                        f32 S = Sin(Angle);
                        Entity->Transform = M4(
                            +C,     -S, 0.0f, P.X,
                            +S,     +C, 0.0f, P.Y,
                            0.0f, 0.0f, 1.0f, P.Z,
                            0.0f, 0.0f, 0.0f, 1.0f);

                        for (u32 MeshIndex = 0; MeshIndex < Model->MeshCount; MeshIndex++)
                        {
                            if (Entity->PieceCount < Entity->MaxPieceCount)
                            {
                                Entity->Pieces[Entity->PieceCount++] = 
                                {
                                    .MeshID = Model->Meshes[MeshIndex],
                                    .OffsetP = { 0.0f, 0.0f, 0.0f },
                                };
                            }
                        }
                    }
                }
            }
        } break;
        InvalidDefaultCase;
    }

    if (Flags & DebugSceneFlag_SponzaAdHocLights)
    {
        World->AdHocLightCount = World->MaxAdHocLightCount;
        World->AdHocLightUpdateRate = 1.0f / 15.0f;
        World->AdHocLightCounter = World->AdHocLightUpdateRate;
            
        World->AdHocLightBounds = { { -8.0f, -4.5f, 3.5f }, { +7.5f, -2.0f, 6.5f } };
        mmbox Bounds = World->AdHocLightBounds;
        entropy32* R = &World->EffectEntropy;
        for (u32 LightIndex = 0; LightIndex < World->AdHocLightCount; LightIndex++)
        {
            World->AdHocLights[LightIndex] = 
            {
                .P = 
                {
                    RandBetween(R, Bounds.Min.X, Bounds.Max.X),
                    RandBetween(R, Bounds.Min.Y, Bounds.Max.Y),
                    RandBetween(R, Bounds.Min.Z, Bounds.Max.Z),
                },
                .E = SetLuminance(v3{ RandBetween(R, 0.1f, 1.0f), RandBetween(R, 0.1f, 1.0f), RandBetween(R, 0.1f, 1.0f) }, RandBetween(R, 0.1f, 0.3f)),
            };
        }
    }

    if (Flags & DebugSceneFlag_SponzaParticles)
    {
        const light LightSources[] = 
        {
            // "Fire" lights on top of the metal hangers
            { .P = { -4.95f, -1.15f, 1.40f }, .E = SetLuminance(v3{ 2.0f, 0.8f, 0.2f }, 3.0f) },
            { .P = { +3.90f, -1.15f, 1.40f }, .E = SetLuminance(v3{ 2.0f, 0.8f, 0.2f }, 3.0f) },
            { .P = { -4.95f, +1.75f, 1.40f }, .E = SetLuminance(v3{ 2.0f, 0.8f, 0.2f }, 3.0f) },
            { .P = { +3.90f, +1.75f, 1.40f }, .E = SetLuminance(v3{ 2.0f, 0.8f, 0.2f }, 3.0f) },

            // "Magic" lights on top of the wells
            { .P = { +8.95f, +3.60f, 1.30f }, .E = SetLuminance(v3{ 0.2f, 0.6f, 1.0f }, 2.5f) },
            { .P = { +8.95f, -3.20f, 1.30f }, .E = SetLuminance(v3{ 0.6f, 0.2f, 1.0f }, 2.5f) },
            { .P = { -9.65f, +3.60f, 1.30f }, .E = SetLuminance(v3{ 0.4f, 1.0f, 0.4f }, 2.5f) },
            { .P = { -9.65f, -3.20f, 1.30f }, .E = SetLuminance(v3{ 1.0f, 0.1f, 0.1f }, 7.5f) },
        };

        for (u32 LightIndex = 0; LightIndex < CountOf(LightSources); LightIndex++)
        {
            const light* Light = LightSources + LightIndex;
            if (World->EntityCount < World->MaxEntityCount)
            {
                entity_id ID = { World->EntityCount++ };
                World->Entities[ID.Value] = 
                {
                    .Flags = EntityFlag_LightSource,
                    .Transform = M4(1.0f, 0.0f, 0.0f, Light->P.X,
                                    0.0f, 1.0f, 0.0f, Light->P.Y,
                                    0.0f, 0.0f, 1.0f, Light->P.Z,
                                    0.0f, 0.0f, 0.0f, 1.0f),
                    .LightEmission = Light->E,
                };

                b32 IsMagicLight = (LightIndex >= 4);
                if (IsMagicLight)
                {
                    mmbox Bounds = 
                    {
                        .Min = { -0.3f, -0.3f, -0.25f },
                        .Max = { +0.3f, +0.3f, +1.75f },
                    };
                    MakeParticleSystem(World, ID, ParticleSystem_Magic, { 0.0f, 0.0f, -0.25f}, Bounds);
                }
                else
                {
                    mmbox Bounds = 
                    {
                        .Min = { -0.15f, -0.15f, -0.15f },
                        .Max = { +0.15f, +0.15f, +0.50f },
                    };
                    MakeParticleSystem(World, ID, ParticleSystem_Fire, { 0.0f, 0.0f, -0.35f }, Bounds);
                }
            }
        }
    }

    if (Flags & DebugSceneFlag_AnimatedFox)
    {
        m4 Transform = YUpToZUp * M4(1e-2f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1e-2f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1e-2f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 1.0f);
        DEBUGLoadTestScene(Scratch, Assets, World, Frame,
                           DEBUGLoad_AddNodesAsEntities,
                           "data/glTF-Sample-Assets/Models/Fox/glTF/Fox.gltf", Transform);
    }

    if (Flags & DebugSceneFlag_TransparentDragon)
    {
        m4 Transform = YUpToZUp;
        DEBUGLoadTestScene(Scratch, Assets, World, Frame,
                           DEBUGLoad_AddNodesAsEntities,
                           "data/glTF-Sample-Assets/Models/DragonAttenuation/glTF/DragonAttenuation.gltf", Transform);
    }
}

lbfn void UpdateAndRenderWorld(game_world* World, assets* Assets, render_frame* Frame, game_io* IO, memory_arena* Scratch, b32 DrawLights)
{
    TimedFunction(Platform.Profiler);

    if (!World->IsLoaded)
    {
        World->LightProxyScale = 1e-1f;
        World->EffectEntropy = { 0x13370420 };
        World->GeneratorEntropy = { 0x13370420 };

        Assert(World->EntityCount == 0);
        // Null entity
        World->Entities[World->EntityCount++] = 
        {
            .Flags = EntityFlag_None,
            .Transform = M4(1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f),
        };

        World->Camera.P = { 0.0f, 0.0f, 0.0f };
        World->Camera.FieldOfView = ToRadians(80.0f);
        World->Camera.NearZ = 0.03125f;
        World->Camera.FarZ = 250.0f;
        World->Camera.Yaw = 0.5f * Pi;

        // Load debug scene
        #if 0
        DEBUGInitializeWorld(World, Assets, Frame, Scratch,
                             DebugScene_Sponza, 
                             DebugSceneFlag_AnimatedFox|DebugSceneFlag_SponzaParticles|DebugSceneFlag_SponzaAdHocLights);
        #elif 0
        DEBUGInitializeWorld(World, Assets, Frame, Scratch,
                             DebugScene_TransmissionTest, 
                             0);
        #elif 1
        DEBUGInitializeWorld(World, Assets, Frame, Scratch,
                             DebugScene_Terrain,
                             DebugSceneFlag_None);
        #endif

        World->IsLoaded = true;
    }

    f32 dt = IO->dt;
    
    //
    // Camera update
    //
    {
        camera* Camera = &World->Camera;
        v3 MoveDirection = {};
        if (IO->Keys[SC_W].bIsDown) { MoveDirection.Z += 1.0f; }
        if (IO->Keys[SC_S].bIsDown) { MoveDirection.Z -= 1.0f; }
        if (IO->Keys[SC_D].bIsDown) { MoveDirection.X += 1.0f; }
        if (IO->Keys[SC_A].bIsDown) { MoveDirection.X -= 1.0f; }

        if (IO->Keys[SC_MouseLeft].bIsDown)
        {
            constexpr f32 MouseTurnSpeed = 1e-2f;
            Camera->Yaw -= -MouseTurnSpeed * IO->Mouse.dP.X;
            Camera->Pitch -= MouseTurnSpeed * IO->Mouse.dP.Y;
        }

        constexpr f32 PitchClamp = 0.5f * Pi - 1e-3f;
        Camera->Pitch = Clamp(Camera->Pitch, -PitchClamp, PitchClamp);

        m4 CameraTransform = GetTransform(Camera);
        v3 Forward = CameraTransform.Z.XYZ;
        v3 Right = CameraTransform.X.XYZ;

        constexpr f32 MoveSpeed = 1.0f;
        f32 SpeedMul = IO->Keys[SC_LeftShift].bIsDown ? 3.5f : 2.0f;
        Camera->dPTarget = (Right * MoveDirection.X + Forward * MoveDirection.Z) * SpeedMul * MoveSpeed;
        if (IO->Keys[SC_Space].bIsDown) Camera->dPTarget.Z += SpeedMul * MoveSpeed;
        if (IO->Keys[SC_LeftControl].bIsDown) Camera->dPTarget.Z -= SpeedMul * MoveSpeed;

        f32 PrecisionAfterT = 1.0f / 128.0f;
        f32 T = 0.150f;
        f32 Lambda = -T / Log2(PrecisionAfterT);

        Camera->dP = Lerp(Camera->dP, Camera->dPTarget, 1.0f - Exp2(-dt / Lambda));
        Camera->P += Camera->dP * dt;

        Frame->CameraTransform = GetTransform(Camera);
        Frame->CameraFocalLength = 1.0f / Tan(0.5f * Camera->FieldOfView);
        Frame->CameraNearPlane = Camera->NearZ;
        Frame->CameraFarPlane = Camera->FarZ;
    }

    // Sun update
    {
        World->SunL = 2.0f * v3{ 10.0f, 7.0f, 5.0f };
        #if 1
        World->SunV = Normalize(v3{ +4.25f, -1.0f, 8.0f });
        #else
        World->SunV = Normalize(v3{ +4.25f, -0.5f, 3.5f });
        #endif
        Frame->SunL = World->SunL;
        Frame->SunV = World->SunV;
    }

    //
    // Entity update
    //
    {
        TimedBlock(Platform.Profiler, "UpdateAndRenderEntities");

        for (u32 EntityIndex = 1; EntityIndex < World->EntityCount; EntityIndex++)
        {
            entity* Entity = World->Entities + EntityIndex;
            if (Entity->Flags & EntityFlag_Mesh)
            {
                u32 JointCount = 0;
                m4 Pose[R_MaxJointCount] = {};

                if (Entity->Flags & EntityFlag_Skin)
                {
                    Assert(Entity->SkinID < Assets->SkinCount);
                    skin* Skin = Assets->Skins + Entity->SkinID;
                    JointCount = Skin->JointCount;
                    for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                    {
                        Pose[JointIndex] = TRSToM4(Skin->BindPose[JointIndex]);
                    }

                    Assert(Entity->CurrentAnimationID < Assets->AnimationCount);
                    animation* Animation = Assets->Animations + Entity->CurrentAnimationID;
                    if (Entity->DoAnimation)
                    {
                        Entity->AnimationCounter += dt;
                        f32 LastKeyFrameTimestamp = Animation->KeyFrameTimestamps[Animation->KeyFrameCount - 1];
                        Entity->AnimationCounter = Modulo0(Entity->AnimationCounter, LastKeyFrameTimestamp);
                    }
                
                    u32 KeyFrameIndex = 0;
                    {
                        u32 MinIndex = 0;
                        u32 MaxIndex = Animation->KeyFrameCount - 1;
                        while (MinIndex <= MaxIndex)
                        {
                            u32 Index = (MinIndex + MaxIndex) / 2;
                            f32 t = Animation->KeyFrameTimestamps[Index];
                            if      (Entity->AnimationCounter < t) MaxIndex = Index - 1;
                            else if (Entity->AnimationCounter > t) MinIndex = Index + 1;
                            else break; 
                        }
                        KeyFrameIndex = MinIndex == 0 ? 0 : MinIndex - 1;
                    }
                
                    u32 NextKeyFrameIndex = (KeyFrameIndex + 1) % Animation->KeyFrameCount;
                    f32 Timestamp0 = Animation->KeyFrameTimestamps[KeyFrameIndex];
                    f32 Timestamp1 = Animation->KeyFrameTimestamps[NextKeyFrameIndex];
                    f32 KeyFrameDelta = Timestamp1 - Timestamp0;
                    f32 BlendStart = Entity->AnimationCounter - Timestamp0;
                    f32 BlendFactor = Ratio0(BlendStart, KeyFrameDelta);
                
                    animation_key_frame* CurrentFrame = Animation->KeyFrames + KeyFrameIndex;
                    animation_key_frame* NextFrame = Animation->KeyFrames + NextKeyFrameIndex;
                    for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                    {
                        if (IsJointActive(Animation, JointIndex))
                        {
                            trs_transform* CurrentTransform = CurrentFrame->JointTransforms + JointIndex;
                            trs_transform* NextTransform = NextFrame->JointTransforms + JointIndex;
                            trs_transform Transform = 
                            {
                                .Rotation = QLerp(CurrentTransform->Rotation, NextTransform->Rotation, BlendFactor),
                                .Position = Lerp(CurrentTransform->Position, NextTransform->Position, BlendFactor),
                                .Scale = Lerp(CurrentTransform->Scale, NextTransform->Scale, BlendFactor),
                            };
                
                            Pose[JointIndex] = TRSToM4(Transform);
                        }
                
                        u32 ParentIndex = Skin->JointParents[JointIndex];
                        if (ParentIndex != JointIndex)
                        {
                            Pose[JointIndex] = Pose[ParentIndex] * Pose[JointIndex];
                        }
                    }
                
                    // NOTE(boti): This _cannot_ be folded into the above loop, because the parent transforms must not contain
                    // the inverse bind transform when propagating the transforms down the hierarchy
                    for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                    {
                        Pose[JointIndex] = Pose[JointIndex] * Skin->InverseBindMatrices[JointIndex];
                    }
                }

                for (u32 PieceIndex = 0; PieceIndex < Entity->PieceCount; PieceIndex++)
                {
                    entity_piece* Piece = Entity->Pieces + PieceIndex;
                    mesh* Mesh = Assets->Meshes + Piece->MeshID;

                    material* Material = Assets->Materials + Mesh->MaterialID;
                    texture* AlbedoTexture              = Assets->Textures + Material->AlbedoID;
                    texture* NormalTexture              = Assets->Textures + Material->NormalID;
                    texture* MetallicRoughnessTexture   = Assets->Textures + Material->MetallicRoughnessID;
                    texture* OcclusionTexture           = Assets->Textures + Material->OcclusionID;
                    texture* HeightTexture              = Assets->Textures + Material->HeightID;
                    texture* TransmissionTexture        = Assets->Textures + Material->TransmissionID;

                    renderer_material RenderMaterial = 
                    {
                        .AlbedoID                   = AlbedoTexture->RendererID,
                        .NormalID                   = NormalTexture->RendererID,
                        .MetallicRoughnessID        = MetallicRoughnessTexture->RendererID,
                        .OcclusionID                = OcclusionTexture->RendererID,
                        .HeightID                   = HeightTexture->RendererID,
                        .TransmissionID             = TransmissionTexture->RendererID,
                        .AlbedoSamplerID            = Material->AlbedoSamplerID,
                        .NormalSamplerID            = Material->NormalSamplerID,
                        .MetallicRoughnessSamplerID = Material->MetallicRoughnessSamplerID,
                        .TransmissionSamplerID      = Material->TransmissionSamplerID,
                        .AlphaThreshold             = Material->AlphaThreshold,
                        .Transmission               = Material->Transmission,
                        .BaseAlbedo                 = Material->Albedo,
                        .BaseMaterial               = Material->MetallicRoughness,
                        .Emissive                   = Material->Emission,
                    };

                    draw_group TransparencyToDrawGroupTable[Transparency_Count] =
                    {
                        [Transparency_Opaque] = DrawGroup_Opaque,
                        [Transparency_AlphaTest] = DrawGroup_AlphaTest,
                        [Transparency_AlphaBlend] = DrawGroup_AlphaTest,
                    };

                    draw_group Group = TransparencyToDrawGroupTable[Material->Transparency];
                    if (Material->TransmissionEnabled)
                    {
                        Group = DrawGroup_Transparent;
                    }
                    m4 PieceTransform = Entity->Transform;
                    PieceTransform.P.XYZ += Piece->OffsetP;
                    DrawMesh(Frame, Group, Mesh->Allocation, PieceTransform, Mesh->BoundingBox, RenderMaterial, JointCount, Pose);
                }
            }

            if (Entity->Flags & EntityFlag_LightSource)
            {
                AddLight(Frame, Entity->Transform.P.XYZ, Entity->LightEmission, LightFlag_ShadowCaster);
                if (DrawLights)
                {
                    mesh* Mesh = Assets->Meshes + Assets->SphereMeshID;
                    f32 S = World->LightProxyScale;
                    m4 Transform = Entity->Transform * M4(S, 0.0f, 0.0f, 0.0f,
                                                          0.0f, S, 0.0f, 0.0f,
                                                          0.0f, 0.0f, S, 0.0f,
                                                          0.0f, 0.0f, 0.0f, 1.0f);
                    DrawWidget3D(Frame, Mesh->Allocation, Transform, PackRGBA(NOZ(Entity->LightEmission)));
                }
            }
        }
    }

    //
    // Particle system update
    //
    {
        TimedBlock(Platform.Profiler, "UpdateAndRenderParticleSystems");

        for (u32 ParticleSystemIndex = 0; ParticleSystemIndex < World->ParticleSystemCount; ParticleSystemIndex++)
        {
            particle_system* ParticleSystem = World->ParticleSystems + ParticleSystemIndex;
            v2 ParticleSize = { 1.0f, 1.0f };

            // TODO(boti): we should probably just pull in the entire parent transform
            v3 BaseP = { 0.0f, 0.0f, 0.0f };
            v3 Color = { 1.0f, 1.0f, 1.0f };
            if (IsValid(ParticleSystem->ParentID))
            {
                entity* Parent = World->Entities + ParticleSystem->ParentID.Value;
                BaseP = Parent->Transform.P.XYZ;
                if (Parent->Flags & EntityFlag_LightSource)
                {
                    Color = Parent->LightEmission;
                }
            }

            mmbox Bounds = ParticleSystem->Bounds;
            mmbox CullBounds = 
            {
                .Min = Bounds.Min + BaseP,
                .Max = Bounds.Max + BaseP,
            };
            if (ParticleSystem->EmissionRate > 0.0f)
            {
                ParticleSystem->Counter += dt;
                while (ParticleSystem->Counter > ParticleSystem->EmissionRate)
                {
                    ParticleSystem->Counter -= ParticleSystem->EmissionRate;
                    if (++ParticleSystem->NextParticle >= ParticleSystem->ParticleCount)
                    {
                        ParticleSystem->NextParticle -= ParticleSystem->ParticleCount;
                    }

                    switch (ParticleSystem->Type)
                    {
                        case ParticleSystem_Undefined:
                        {
                            // Ignored
                        } break;
                        case ParticleSystem_Magic:
                        {
                            v2 XY = 0.5f * Hadamard((Bounds.Max.XY - Bounds.Min.XY), RandInUnitCircle(&World->EffectEntropy));
                            v3 ParticleP = { XY.X, XY.Y, 0.0f };
                            ParticleSystem->Particles[ParticleSystem->NextParticle] = 
                            {
                                .P = BaseP + ParticleSystem->EmitterOffset + ParticleP,
                                .dP = { 0.0f, 0.0f, RandBetween(&World->EffectEntropy, 0.25f, 2.25f) },
                                .Color = Color,
                                .dColor = { 0.0f, 0.0f, 0.0f },
                                .TextureIndex = Particle_Trace02,
                            };
                        } break;
                        case ParticleSystem_Fire:
                        {
                            u32 FirstTexture = Particle_Flame01;
                            u32 OnePastLastTexture = Particle_Flame04 + 1;
                            u32 TextureCount = OnePastLastTexture - FirstTexture;

                            v3 ParticleP = { 0.0f, 0.0f, 0.0f };
                            ParticleSystem->Particles[ParticleSystem->NextParticle] = 
                            {
                                .P = ParticleP + ParticleSystem->EmitterOffset + BaseP,
                                .dP = 
                                { 
                                    0.3f * RandBilateral(&World->EffectEntropy),
                                    0.3f * RandBilateral(&World->EffectEntropy),
                                    RandBetween(&World->EffectEntropy, 0.25f, 1.20f) 
                                },
                                .ddP = { 0.5f, 0.2f, 0.0f },
                                .Color = Color,
                                .dColor = 6.0f * v3{ -1.00f, -1.25f, -1.00f },
                                .TextureIndex = FirstTexture + (RandU32(&World->EffectEntropy) % TextureCount),
                            };
                        } break;
                        InvalidDefaultCase;
                    }
                }
            }

            render_command* Cmd = MakeParticleBatch(Frame, 0);
            if (Cmd)
            {
                Cmd->ParticleBatch.Mode = ParticleSystem->Mode;
            }

            for (u32 It = 0; It < ParticleSystem->ParticleCount; It++)
            {
                particle* Particle = ParticleSystem->Particles + It;
                Particle->P += Particle->dP * dt;
                Particle->dP += Particle->ddP * dt;
                Particle->Color += Particle->dColor * dt;

                b32 CullParticle = false;
                if (ParticleSystem->CullOutOfBoundsParticles)
                {
                    CullParticle = 
                        Particle->P.X < CullBounds.Min.X || Particle->P.X >= CullBounds.Max.X ||
                        Particle->P.Y < CullBounds.Min.Y || Particle->P.Y >= CullBounds.Max.Y ||
                        Particle->P.Z < CullBounds.Min.Z || Particle->P.Z >= CullBounds.Max.Z;
                }

                if (!CullParticle)
                {
                    v4 PColor = 
                    {
                        Max(Particle->Color.X, 0.0f),
                        Max(Particle->Color.Y, 0.0f),
                        Max(Particle->Color.Z, 0.0f),
                        1.0,
                    };
                    Cmd = PushParticle(Frame, Cmd, 
                                       {
                                           .P = Particle->P,
                                           .TextureIndex = Particle->TextureIndex,
                                           .Color = PColor,
                                           .HalfExtent = ParticleSystem->ParticleHalfExtent,
                                       });
                }
            }
        }
    }

    // Ad-hoc lights
    if (1)
    {
        TimedBlock(Platform.Profiler, "UpdateAndRenderAdHocLights");

        render_command* Cmd = MakeParticleBatch(Frame, World->AdHocLightCount);
        if (Cmd)
        {
            Cmd->ParticleBatch.Mode = Billboard_ViewAligned;
        }

        b32 DoVelocityUpdate = false;
        World->AdHocLightCounter += dt;
        if (World->AdHocLightCounter >= World->AdHocLightUpdateRate)
        {
            DoVelocityUpdate = true;
        }
        World->AdHocLightCounter = Modulo0(World->AdHocLightCounter, World->AdHocLightUpdateRate);

        for (u32 LightIndex = 0; LightIndex < World->AdHocLightCount; LightIndex++)
        {
            light* Light = World->AdHocLights + LightIndex;

            v3 dP = World->AdHocLightdPs[LightIndex];
            if (DoVelocityUpdate)
            {
                dP = { RandBilateral(&World->EffectEntropy), RandBilateral(&World->EffectEntropy), RandBilateral(&World->EffectEntropy) };
                dP = RandBetween(&World->EffectEntropy, 1.5f, 4.5f) * dP;
                World->AdHocLightdPs[LightIndex] = dP;
            }

            Light->P += dP * dt;
            Light->P.X = Clamp(Light->P.X, World->AdHocLightBounds.Min.X, World->AdHocLightBounds.Max.X);
            Light->P.Y = Clamp(Light->P.Y, World->AdHocLightBounds.Min.Y, World->AdHocLightBounds.Max.Y);
            Light->P.Z = Clamp(Light->P.Z, World->AdHocLightBounds.Min.Z, World->AdHocLightBounds.Max.Z);

            AddLight(Frame, Light->P, Light->E, LightFlag_None);
            Cmd = PushParticle(Frame, Cmd, 
                               {
                                   .P = Light->P,
                                   .TextureIndex = Particle_Star06,
                                   .Color = { Light->E.X, Light->E.Y, Light->E.Z, 5.0f },
                                   .HalfExtent = { 0.1f, 0.1f },
                               });
        }
    }
}