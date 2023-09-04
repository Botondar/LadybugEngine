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
        LocalX.x, LocalY.x, LocalZ.x, 0.0f,
        LocalX.y, LocalY.y, LocalZ.y, 0.0f,
        LocalX.z, LocalY.z, LocalZ.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    m4 Translation = M4(
        1.0f, 0.0f, 0.0f, Camera->P.x,
        0.0f, 1.0f, 0.0f, Camera->P.y,
        0.0f, 0.0f, 1.0f, Camera->P.z,
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
        Right.x, -Up.x, Forward.x, 0.0f,
        Right.y, -Up.y, Forward.y, 0.0f,
        Right.z, -Up.z, Forward.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    m4 Translation = M4(
        1.0f, 0.0f, 0.0f, Camera->P.x,
        0.0f, 1.0f, 0.0f, Camera->P.y,
        0.0f, 0.0f, 1.0f, Camera->P.z,
        0.0f, 0.0f, 0.0f, 1.0f);

    m4 Result = Translation * BasisTransform;
    return Result;
}

lbfn f32 SampleNoise(noise2* Noise, v2 P)
{
    v2 P0 = { Floor(P.x), Floor(P.y) };
    v2 dP = P - P0;

    u32 X = (u32)(s32)P0.x;
    u32 Y = (u32)(s32)P0.y;

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
    f32 U = Fade(dP.x);
    f32 V = Fade(dP.y);
    f32 X0 = Lerp(D00, D10, U);
    f32 X1 = Lerp(D01, D11, U);
    f32 Result = Lerp(X0, X1, V);
    return(Result);
}

internal mesh_data 
GenerateTerrainChunk(height_field* Field, memory_arena* Arena)
{
    mesh_data Result = {};

    Result.Box = {}; // TODO(boti)

    Result.VertexCount = Field->TexelCountX*Field->TexelCountY;
    Result.VertexData = PushArray<vertex>(Arena, Result.VertexCount);

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

            v3 PXn = { P.x - dS, P.y, SampleHeight(Field, X - 1, Y) };
            v3 PXp = { P.x + dS, P.y, SampleHeight(Field, X + 1, Y) };
            v3 PYn = { P.x, P.y - dS, SampleHeight(Field, X, Y - 1) };
            v3 PYp = { P.x, P.y + dS, SampleHeight(Field, X, Y + 1) };

            v3 T = NOZ(PXp - PXn);
            v3 B = NOZ(PYp - PYn);
            v3 N = NOZ(Cross(T, B));

            *VertexAt++ = 
            {
                .P = P,
                .N = N, 
                .T = { T.x, T.y, T.z, 1.0f },
                .TexCoord = { 0.0f, 0.0f },
                .Color = PackRGBA8(0xFF, 0xFF, 0xFF),
            };
        }
    }

    u32 QuadCountX = Field->TexelCountX - 1;
    u32 QuadCountY = Field->TexelCountY - 1;
    Result.IndexCount = 6 * QuadCountX*QuadCountY;
    Result.IndexData = PushArray<vert_index>(Arena, Result.IndexCount);
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

lbfn void UpdateAndRenderWorld(game_world* World, assets* Assets, render_frame* Frame, game_io* IO, memory_arena* Scratch, b32 DrawLights)
{
    // Load debug scene
    if (!World->IsLoaded)
    {
        World->LightProxyScale = 1e-1f;
        World->EffectEntropy = { 0x13370420 };

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
        World->Camera.NearZ = 0.1f;
        World->Camera.FarZ = 250.0f;
        World->Camera.Yaw = 0.5f * Pi;

        // Terrain generation
        {
            // Height field
            {
                World->TerrainNoise = { 0 };
                World->HeightField.TexelCountX = 128;
                World->HeightField.TexelCountY = 128;
                World->HeightField.TexelsPerMeter = 4;
                u32 TexelCount = World->HeightField.TexelCountX * World->HeightField.TexelCountY;
                World->HeightField.HeightData = PushArray<f32>(World->Arena, TexelCount);

                for (u32 Y = 0; Y < World->HeightField.TexelCountY; Y++)
                {
                    for (u32 X = 0; X < World->HeightField.TexelCountX; X++)
                    {
                        u32 Index = X + Y*World->HeightField.TexelCountX;

                        f32 BaseScale = 1.0f / 2.0f;
                        v2 P = (BaseScale / World->HeightField.TexelsPerMeter) * v2{ (f32)X, (f32)Y };
                        f32 Height = 0.0f;

                        for (u32 OctaveIndex = 0; OctaveIndex < 8; OctaveIndex++)
                        {
                            f32 Mul = 1.0f / (f32)(1 << OctaveIndex);
                            Height += Mul * SampleNoise(&World->TerrainNoise, Mul*P);
                            World->HeightField.HeightData[Index] = Height;
                        }
                    }
                }
            }

            World->TerrainMaterialID = Assets->MaterialCount++;
            material* TerrainMaterial = Assets->Materials + World->TerrainMaterialID;
            TerrainMaterial->AlbedoID = Assets->DefaultDiffuseID;
            TerrainMaterial->NormalID = Assets->DefaultNormalID;
            TerrainMaterial->MetallicRoughnessID = Assets->DefaultMetallicRoughnessID;
            TerrainMaterial->Transparency = Transparency_Opaque;
            TerrainMaterial->Albedo = PackRGBA8(0x10, 0x10, 0x10);
            TerrainMaterial->MetallicRoughness = PackRGBA8(0xFF, 0xFF, 0x00);
            TerrainMaterial->Emission = { 0.0f, 0.0f, 0.0f };

            mesh_data TerrainMesh = GenerateTerrainChunk(&World->HeightField, Scratch);

            u32 ChunkMeshID = Assets->MeshCount++;
            mesh* Mesh = Assets->Meshes + ChunkMeshID;
            Mesh->Allocation = AllocateGeometry(Frame->Renderer, TerrainMesh.VertexCount, TerrainMesh.IndexCount);
            Mesh->BoundingBox = {};
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
        }

        {
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
                        RandBetween(R, Bounds.Min.x, Bounds.Max.x), 
                        RandBetween(R, Bounds.Min.y, Bounds.Max.y), 
                        RandBetween(R, Bounds.Min.z, Bounds.Max.z),
                        1.0f 
                    },
                    .E = 
                    { 
                        RandBetween(R, 0.1f, 1.0f), 
                        RandBetween(R, 0.1f, 1.0f), 
                        RandBetween(R, 0.1f, 1.0f), 
                        RandBetween(R, 0.1f, 0.3f),
                    },
                };
            }
        }

#if 0
        m4 YUpToZUp = M4(1.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, -1.0f, 0.0f,
                         0.0f, 1.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 1.0f);
        // Sponza scene
#if 1
        {
            m4 Transform = YUpToZUp;
            DEBUGLoadTestScene(Scratch, Assets, World, Frame,
                               "data/Scenes/Sponza/Sponza.gltf", Transform);

            const light LightSources[] = 
            {
                // "Fire" lights on top of the metal hangers
                { { -4.95f, -1.15f, 1.40f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
                { { +3.90f, -1.15f, 1.40f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
                { { -4.95f, +1.75f, 1.40f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
                { { +3.90f, +1.75f, 1.40f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },

                // "Magic" lights on top of the wells
                { { +8.95f, +3.60f, 1.30f, 1.0f }, { 0.2f, 0.6f, 1.0f, 2.5f } },
                { { +8.95f, -3.20f, 1.30f, 1.0f }, { 0.6f, 0.2f, 1.0f, 2.5f } },
                { { -9.65f, +3.60f, 1.30f, 1.0f }, { 0.4f, 1.0f, 0.4f, 2.5f } },
                { { -9.65f, -3.20f, 1.30f, 1.0f }, { 1.0f, 0.1f, 0.1f, 7.5f } },
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
                        .Transform = M4(1.0f, 0.0f, 0.0f, Light->P.x,
                                        0.0f, 1.0f, 0.0f, Light->P.y,
                                        0.0f, 0.0f, 1.0f, Light->P.z,
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
#endif

        // Animated fox
        {
            m4 Transform = YUpToZUp * M4(1e-2f, 0.0f, 0.0f, 0.0f,
                                         0.0f, 1e-2f, 0.0f, 0.0f,
                                         0.0f, 0.0f, 1e-2f, 0.0f,
                                         0.0f, 0.0f, 0.0f, 1.0f);
            DEBUGLoadTestScene(Scratch, Assets, World, Frame, 
                               "data/Scenes/Fox/Fox.gltf", Transform);
        }
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
        if (IO->Keys[SC_W].bIsDown) { MoveDirection.z += 1.0f; }
        if (IO->Keys[SC_S].bIsDown) { MoveDirection.z -= 1.0f; }
        if (IO->Keys[SC_D].bIsDown) { MoveDirection.x += 1.0f; }
        if (IO->Keys[SC_A].bIsDown) { MoveDirection.x -= 1.0f; }

        if (IO->Keys[SC_MouseLeft].bIsDown)
        {
            constexpr f32 MouseTurnSpeed = 1e-2f;
            Camera->Yaw -= -MouseTurnSpeed * IO->Mouse.dP.x;
            Camera->Pitch -= MouseTurnSpeed * IO->Mouse.dP.y;
        }

        constexpr f32 PitchClamp = 0.5f * Pi - 1e-3f;
        Camera->Pitch = Clamp(Camera->Pitch, -PitchClamp, PitchClamp);

        m4 CameraTransform = GetTransform(Camera);
        v3 Forward = CameraTransform.Z.xyz;
        v3 Right = CameraTransform.X.xyz;

        constexpr f32 MoveSpeed = 1.0f;
        f32 SpeedMul = IO->Keys[SC_LeftShift].bIsDown ? 3.5f : 2.0f;
        Camera->P += (Right * MoveDirection.x + Forward * MoveDirection.z) * SpeedMul * MoveSpeed * dt;

        if (IO->Keys[SC_Space].bIsDown) { Camera->P.z += SpeedMul * MoveSpeed * dt; }
        if (IO->Keys[SC_LeftControl].bIsDown) { Camera->P.z -= SpeedMul * MoveSpeed * dt; }

        CameraTransform = GetTransform(Camera);
        m4 ViewTransform = AffineOrthonormalInverse(CameraTransform);
        render_camera RenderCamera = 
        {
            .CameraTransform = CameraTransform,
            .ViewTransform = ViewTransform,
            .FocalLength = 1.0f / Tan(0.5f * Camera->FieldOfView),
            .NearZ = Camera->NearZ,
            .FarZ = Camera->FarZ,
        };
        SetRenderCamera(Frame, &RenderCamera);
    }

    // Sun update
    {
        World->SunL = 2.0f * v3{ 10.0f, 7.0f, 5.0f };
        World->SunV = Normalize(v3{ +4.25f, -0.5f, 10.0f });
        Frame->SunV = World->SunV;
        Frame->Uniforms.SunV = TransformDirection(Frame->Uniforms.ViewTransform, World->SunV);
        Frame->Uniforms.SunL = World->SunL;
    }

    //
    // Entity update
    //
    for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        entity* Entity = World->Entities + EntityIndex;
        if (Entity->Flags & EntityFlag_Mesh)
        {
            b32 DrawSkinned = false;
            u32 JointCount = 0;
            m4 Pose[skin::MaxJointCount] = {};

            if (Entity->Flags & EntityFlag_Skin)
            {
                DrawSkinned = true;

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
                texture* AlbedoTexture = Assets->Textures + Material->AlbedoID;
                texture* NormalTexture = Assets->Textures + Material->NormalID;
                texture* MetallicRoughnessTexture = Assets->Textures + Material->MetallicRoughnessID;
                renderer_texture_id AlbedoID = AlbedoTexture->IsLoaded ? 
                    AlbedoTexture->RendererID : 
                    Assets->Textures[Assets->DefaultDiffuseID].RendererID;
                renderer_texture_id NormalID = NormalTexture->IsLoaded ?
                    NormalTexture->RendererID :
                    Assets->Textures[Assets->DefaultNormalID].RendererID;
                renderer_texture_id MetallicRoughnessID = MetallicRoughnessTexture->IsLoaded ?
                    MetallicRoughnessTexture->RendererID :
                    Assets->Textures[Assets->DefaultMetallicRoughnessID].RendererID;
                
                renderer_material RenderMaterial = 
                {
                    .Emissive = Material->Emission,
                    .DiffuseColor = Material->Albedo,
                    .BaseMaterial = Material->MetallicRoughness,
                    .DiffuseID = AlbedoID,
                    .NormalID = NormalID,
                    .MetallicRoughnessID = MetallicRoughnessID,
                };
                if (DrawSkinned)
                {
                    DrawSkinnedMesh(Frame, Mesh->Allocation, Entity->Transform, RenderMaterial, JointCount, Pose);
                }
                else
                {
                    DrawMesh(Frame, Mesh->Allocation, Entity->Transform, RenderMaterial);
                }

            }
        }

        if (Entity->Flags & EntityFlag_LightSource)
        {
            AddLight(Frame, { Entity->Transform.P, Entity->LightEmission }, LightFlag_ShadowCaster);
            if (DrawLights)
            {
                mesh* Mesh = Assets->Meshes + Assets->SphereMeshID;
                f32 S = World->LightProxyScale;
                m4 Transform = Entity->Transform * M4(S, 0.0f, 0.0f, 0.0f,
                                                      0.0f, S, 0.0f, 0.0f,
                                                      0.0f, 0.0f, S, 0.0f,
                                                      0.0f, 0.0f, 0.0f, 1.0f);
                DrawWidget3D(Frame, Mesh->Allocation, Transform, PackRGBA(Entity->LightEmission));
            }
        }
    }

    //
    // Particle system update
    //
    for (u32 ParticleSystemIndex = 0; ParticleSystemIndex < World->ParticleSystemCount; ParticleSystemIndex++)
    {
        particle_system* ParticleSystem = World->ParticleSystems + ParticleSystemIndex;
        v2 ParticleSize = { 1.0f, 1.0f };

        // TODO(boti): we should probably just pull in the entire parent transform
        v3 BaseP = { 0.0f, 0.0f, 0.0f };
        v4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        if (IsValid(ParticleSystem->ParentID))
        {
            entity* Parent = World->Entities + ParticleSystem->ParentID.Value;
            BaseP = Parent->Transform.P.xyz;
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
                        v2 XY = 0.5f * Hadamard((Bounds.Max.xy - Bounds.Min.xy), RandInUnitCircle(&World->EffectEntropy));
                        v3 ParticleP = { XY.x, XY.y, 0.0f };
                        ParticleSystem->Particles[ParticleSystem->NextParticle] = 
                        {
                            .P = BaseP + ParticleSystem->EmitterOffset + ParticleP,
                            .dP = { 0.0f, 0.0f, RandBetween(&World->EffectEntropy, 0.25f, 2.25f) },
                            .Color = Color,
                            .dColor = { 0.0f, 0.0f, 0.0f, 0.0f },
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
                            .Color = { Color },
                            .dColor = { -1.00f, -1.25f, -1.00f, -4.00f },
                            .TextureIndex = FirstTexture + (RandU32(&World->EffectEntropy) % TextureCount),
                        };
                    } break;
                    InvalidDefaultCase;
                }
            }
        }

        particle_cmd* Cmd = nullptr;
        if (Frame->ParticleDrawCmdCount < Frame->MaxParticleDrawCmdCount)
        {
            Cmd = Frame->ParticleDrawCmds + Frame->ParticleDrawCmdCount++;
            Cmd->FirstParticle = Frame->ParticleCount;
            Cmd->ParticleCount = 0;
            Cmd->Mode = ParticleSystem->Mode;
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
                    Particle->P.x < CullBounds.Min.x || Particle->P.x >= CullBounds.Max.x ||
                    Particle->P.y < CullBounds.Min.y || Particle->P.y >= CullBounds.Max.y ||
                    Particle->P.z < CullBounds.Min.z || Particle->P.z >= CullBounds.Max.z;
            }

            if (!CullParticle)
            {
                if (Cmd)
                {
                    if (Frame->ParticleCount < Frame->MaxParticleCount)
                    {
                        v4 PColor = 
                        {
                            Max(Particle->Color.x, 0.0f),
                            Max(Particle->Color.y, 0.0f),
                            Max(Particle->Color.z, 0.0f),
                            Max(Particle->Color.w, 0.0f),
                        };
                        Cmd->ParticleCount++;
                        Frame->Particles[Frame->ParticleCount++] = 
                        {
                            .P = Particle->P,
                            .TextureIndex = Particle->TextureIndex,
                            .Color = PColor,
                            .HalfExtent = ParticleSystem->ParticleHalfExtent,
                        };
                    }
                }
            }
        }
    }

    // Ad-hoc lights
    {
        particle_cmd* Cmd = nullptr;
        if ((Frame->ParticleDrawCmdCount < Frame->MaxParticleDrawCmdCount) && 
            (Frame->ParticleCount + World->AdHocLightCount <= Frame->MaxParticleCount))
        {
            Cmd = Frame->ParticleDrawCmds + Frame->ParticleDrawCmdCount++;
            Cmd->FirstParticle = Frame->ParticleCount;
            Cmd->ParticleCount = World->AdHocLightCount;
            Cmd->Mode = Billboard_ViewAligned;

            Frame->ParticleCount += World->AdHocLightCount;
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

            Light->P.xyz += dP * dt;
            Light->P.x = Clamp(Light->P.x, World->AdHocLightBounds.Min.x, World->AdHocLightBounds.Max.x);
            Light->P.y = Clamp(Light->P.y, World->AdHocLightBounds.Min.y, World->AdHocLightBounds.Max.y);
            Light->P.z = Clamp(Light->P.z, World->AdHocLightBounds.Min.z, World->AdHocLightBounds.Max.z);

            AddLight(Frame, *Light, LightFlag_None);
            if (Cmd)
            {
                Frame->Particles[Cmd->FirstParticle + LightIndex] = 
                {
                    .P = Light->P.xyz,
                    .TextureIndex = Particle_Star06,
                    .Color = { Light->E.x, Light->E.y, Light->E.z, 5.0f },
                    .HalfExtent = { 0.1f, 0.1f },
                };
            }
        }
    }
}