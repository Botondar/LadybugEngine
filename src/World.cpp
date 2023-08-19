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

lbfn u32 MakeParticleSystem(game_world* World, entity_id ParentID, particle_system_type Type, mmbox Bounds)
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

        {
            World->AdHocLightBounds = { { -7.5f, -4.0f, 4.0f }, { +7.5f, -2.75f, 6.0f } };
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
                        //RandBetween(R, 0.01f, 0.01f),
                        RandBetween(R, 0.1f, 0.3f),
                    },
                };
            }
        }

        m4 YUpToZUp = M4(1.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, -1.0f, 0.0f,
                         0.0f, 1.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 1.0f);
        // Sponza scene
#if 1
        {
            m4 Transform = YUpToZUp;
            DEBUGLoadTestScene(Scratch, Assets, World, Frame->Renderer,
                               "data/Scenes/Sponza/Sponza.gltf", Transform);

            const light LightSources[] = 
            {
                // "Fire" lights on top of the metal hangers
                { { -4.95f, -1.15f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
                { { +3.90f, -1.15f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
                { { -4.95f, +1.75f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
                { { +3.90f, +1.75f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },

                // "Magic" lights on top of the wells
                { { +8.95f, +3.60f, 1.30f, 1.0f }, { 0.2f, 0.6f, 1.0f, 2.5f } },
                { { +8.95f, -3.20f, 1.30f, 1.0f }, { 0.6f, 0.2f, 1.0f, 2.5f } },
                { { -9.65f, +3.60f, 1.30f, 1.0f }, { 0.4f, 1.0f, 0.4f, 2.5f } },
                { { -9.65f, -3.20f, 1.30f, 1.0f }, { 0.2f, 0.6f, 1.0f, 2.5f } },
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
                        MakeParticleSystem(World, ID, ParticleSystem_Magic, Bounds);
                    }
                    else
                    {
                        mmbox Bounds = 
                        {
                            .Min = { -0.15f, -0.15f, -0.15f },
                            .Max = { +0.15f, +0.15f, +0.50f },
                        };
                        MakeParticleSystem(World, ID, ParticleSystem_Fire, Bounds);
                    }
                }
            }
        }
#else
        // Bathroom scene
        {
            m4 Transform = YUpToZUp;
            DEBUGLoadTestScene(Scratch, Assets, World, Frame->Renderer,
                               "data/Scenes/Bathroom/Bathroom.gltf", Transform);
        }
#endif

        // Animated fox
        {
            m4 Transform = YUpToZUp * M4(1e-2f, 0.0f, 0.0f, 0.0f,
                                         0.0f, 1e-2f, 0.0f, 0.0f,
                                         0.0f, 0.0f, 1e-2f, 0.0f,
                                         0.0f, 0.0f, 0.0f, 1.0f);
            DEBUGLoadTestScene(Scratch, Assets, World, Frame->Renderer, 
                               "data/Scenes/Fox/Fox.gltf", Transform);
        }

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

        if (IO->Keys[SC_Left].bIsDown)
        {
            Camera->Yaw += 1e-1f * dt;
        }
        if (IO->Keys[SC_Right].bIsDown)
        {
            Camera->Yaw -= 1e-1f * dt;
        }

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
        f32 SpeedMul = IO->Keys[SC_LeftShift].bIsDown ? 2.5f : 1.0f;
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
        World->SunL = 2.5f * v3{ 10.0f, 7.0f, 3.0f };
        World->SunV = Normalize(v3{ -3.0f, 2.5f, 12.0f });
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
                            .Rotation = Normalize(Lerp(CurrentTransform->Rotation, NextTransform->Rotation, BlendFactor)),
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
                geometry_buffer_allocation MeshAllocation = Assets->Meshes[Piece->MeshID];
                u32 MaterialID = Assets->MeshMaterialIndices[Piece->MeshID];
                
                u32 VertexOffset = TruncateU64ToU32(MeshAllocation.VertexBlock->ByteOffset / sizeof(vertex));
                u32 VertexCount = TruncateU64ToU32(MeshAllocation.VertexBlock->ByteSize / sizeof(vertex));
                u32 IndexOffset = TruncateU64ToU32(MeshAllocation.IndexBlock->ByteOffset / sizeof(vert_index));
                u32 IndexCount = TruncateU64ToU32(MeshAllocation.IndexBlock->ByteSize / sizeof(vert_index));

                if (DrawSkinned)
                {
                    DrawSkinnedMesh(Frame, VertexOffset, VertexCount, IndexOffset, IndexCount,
                                    Entity->Transform, Assets->Materials[MaterialID],
                                    JointCount, Pose);
                }
                else
                {
                    DrawMesh(Frame, VertexOffset, VertexCount, IndexOffset, IndexCount,
                             Entity->Transform, Assets->Materials[MaterialID]);
                }

            }
        }

        if (Entity->Flags & EntityFlag_LightSource)
        {
            AddLight(Frame, { Entity->Transform.P, Entity->LightEmission });
            if (DrawLights)
            {
                geometry_buffer_allocation Mesh = Assets->Meshes[Assets->SphereMeshID];
                u32 VertexOffset = Mesh.VertexBlock->ByteOffset / sizeof(vertex);
                u32 VertexCount = Mesh.VertexBlock->ByteSize / sizeof(vertex);
                u32 IndexOffset = Mesh.IndexBlock->ByteOffset / sizeof(vert_index);
                u32 IndexCount = Mesh.IndexBlock->ByteSize / sizeof(vert_index);

                f32 S = World->LightProxyScale;
                m4 Transform = Entity->Transform * M4(S, 0.0f, 0.0f, 0.0f,
                                                      0.0f, S, 0.0f, 0.0f,
                                                      0.0f, 0.0f, S, 0.0f,
                                                      0.0f, 0.0f, 0.0f, 1.0f);
                DrawWidget3D(Frame, VertexOffset, VertexCount, IndexOffset, IndexCount, Transform, PackRGBA(Entity->LightEmission));
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
                        v3 ParticleP = 
                        {
                            RandBetween(&World->EffectEntropy, Bounds.Min.x, Bounds.Max.x), 
                            RandBetween(&World->EffectEntropy, Bounds.Min.y, Bounds.Max.y), 
                            Bounds.Min.z,
                        };
                        ParticleSystem->Particles[ParticleSystem->NextParticle] = 
                        {
                            .P = BaseP + ParticleP,
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

                        v3 ParticleP = { 0.0f, 0.0f, Bounds.Min.z };
                        ParticleSystem->Particles[ParticleSystem->NextParticle] = 
                        {
                            .P = ParticleP + BaseP,
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
    for (u32 LightIndex = 0; LightIndex < World->AdHocLightCount; LightIndex++)
    {
        light* Light = World->AdHocLights + LightIndex;
#if 0
        v3 dP = { RandBilateral(&World->EffectEntropy), RandBilateral(&World->EffectEntropy), RandBilateral(&World->EffectEntropy) };
        dP = RandBetween(&World->EffectEntropy, 1.5f, 4.5f) * dP;
#else
        v3 dP = { 0.0f, 0.0f, 0.0f };
#endif
        Light->P.xyz += dP * dt;
        Light->P.x = Clamp(Light->P.x, World->AdHocLightBounds.Min.x, World->AdHocLightBounds.Max.x);
        Light->P.y = Clamp(Light->P.y, World->AdHocLightBounds.Min.y, World->AdHocLightBounds.Max.y);
        Light->P.z = Clamp(Light->P.z, World->AdHocLightBounds.Min.z, World->AdHocLightBounds.Max.z);

        AddLight(Frame, *Light);
        if (Cmd)
        {
            Frame->Particles[Cmd->FirstParticle + LightIndex] = 
            {
                .P = Light->P.xyz,
                .TextureIndex = Particle_Star07,
                .Color = { Light->E.x, Light->E.y, Light->E.z, 5.0f },
                .HalfExtent = { 0.1f, 0.1f },
            };
        }
    }
}