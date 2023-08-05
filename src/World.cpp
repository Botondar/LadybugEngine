#include "World.hpp"

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
                ParticleSystem->ParticleCount = ParticleSystem->MaxParticleCount;
                
            } break;
            case ParticleSystem_Fire:
            {
                ParticleSystem->Mode = Billboard_ViewAligned;
                ParticleSystem->ParticleHalfExtent = { 0.15f, 0.15f };
                ParticleSystem->ParticleCount = ParticleSystem->MaxParticleCount;
                ParticleSystem->EmissionRate = 1.0f / 30.0f;
            } break;
            InvalidDefaultCase;
        }
    }
    return(Result);
}

lbfn void UpdateAndRenderWorld(game_world* World, assets* Assets, render_frame* Frame, game_io* IO)
{
    f32 dt = IO->dt;
    World->SunL = 2.5f * v3{ 10.0f, 7.0f, 3.0f }; // Intensity
    World->SunV = Normalize(v3{ -3.0f, 2.5f, 12.0f }); // Direction (towards the sun)
    
    // Camera update
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
    }

    frame_uniform_data* Uniforms = &Frame->Uniforms;
    for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        entity* Entity = World->Entities + EntityIndex;
        if (Entity->Flags & EntityFlag_Mesh)
        {
            mmbox BoundingBox = Assets->MeshBoxes[Entity->MeshID];
            geometry_buffer_allocation MeshAllocation = Assets->Meshes[Entity->MeshID];
            u32 MaterialID = Assets->MeshMaterialIndices[Entity->MeshID];
                
            u32 VertexOffset = TruncateU64ToU32(MeshAllocation.VertexBlock->ByteOffset / sizeof(vertex));
            u32 VertexCount = TruncateU64ToU32(MeshAllocation.VertexBlock->ByteSize / sizeof(vertex));
            u32 IndexOffset = TruncateU64ToU32(MeshAllocation.IndexBlock->ByteOffset / sizeof(vert_index));
            u32 IndexCount = TruncateU64ToU32(MeshAllocation.IndexBlock->ByteSize / sizeof(vert_index));

            if (Entity->Flags & EntityFlag_Skin)
            {
                u32 JointCount = 0;
                m4 Pose[skin::MaxJointCount] = {};

                // Animation
                {
                    skin* Skin = Assets->Skins + Entity->SkinID;
                    JointCount = Skin->JointCount;
                    animation* Animation = Assets->Animations + Entity->CurrentAnimationID;
                    Assert(Animation->SkinID == Entity->SkinID);
                    if (Entity->DoAnimation)
                    {
                        Entity->AnimationCounter += dt;
                        f32 LastKeyFrameTimestamp = Animation->KeyFrameTimestamps[Animation->KeyFrameCount - 1];
                        while (Entity->AnimationCounter >= LastKeyFrameTimestamp)
                        {
                            Entity->AnimationCounter -= LastKeyFrameTimestamp;
                        }
                    }

                    u32 NextKeyFrameIndex;
                    for (NextKeyFrameIndex = 0; NextKeyFrameIndex < Animation->KeyFrameCount; NextKeyFrameIndex++)
                    {
                        if (Entity->AnimationCounter < Animation->KeyFrameTimestamps[NextKeyFrameIndex])
                        {
                            break;
                        }
                    }

                    u32 KeyFrameIndex = NextKeyFrameIndex - 1;
                    f32 Timestamp0 = Animation->KeyFrameTimestamps[KeyFrameIndex];
                    f32 Timestamp1 = Animation->KeyFrameTimestamps[NextKeyFrameIndex];
                    f32 KeyFrameDelta = Timestamp1 - Timestamp0;
                    f32 BlendStart = Entity->AnimationCounter - Timestamp0;
                    f32 BlendFactor = Ratio0(BlendStart, KeyFrameDelta);

                    animation_key_frame* CurrentFrame = Animation->KeyFrames + KeyFrameIndex;
                    animation_key_frame* NextFrame = Animation->KeyFrames + NextKeyFrameIndex;
                    for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                    {
                        trs_transform* CurrentTransform = CurrentFrame->JointTransforms + JointIndex;
                        trs_transform* NextTransform = NextFrame->JointTransforms + JointIndex;
                        trs_transform Transform = 
                        {
                            .Rotation = Normalize(Lerp(CurrentTransform->Rotation, NextTransform->Rotation, BlendFactor)),
                            .Position = Lerp(CurrentTransform->Position, NextTransform->Position, BlendFactor),
                            .Scale = Lerp(CurrentTransform->Scale, NextTransform->Scale, BlendFactor),
                        };

                        m4 Matrix = TRSToM4(Transform);
                        u32 ParentIndex = Skin->JointParents[JointIndex];
                        if (ParentIndex != U32_MAX)
                        {
                            Pose[JointIndex] = Pose[ParentIndex] * Matrix;
                        }
                        else
                        {
                            Pose[JointIndex] = Matrix;
                        }
                    }

                    // NOTE(boti): This _cannot_ be folded into the above loop, because the parent transforms must not contain
                    // the inverse bind transform when propagating the transforms down the hierarchy
                    for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                    {
                        Pose[JointIndex] = Pose[JointIndex] * Skin->InverseBindMatrices[JointIndex];
                    }
                }

                RenderSkinnedMesh(Frame, VertexOffset, VertexCount, IndexOffset, IndexCount,
                                  Entity->Transform, Assets->Materials[MaterialID],
                                  JointCount, Pose);
            }
            else
            {
                RenderMesh(Frame, VertexOffset, VertexCount, IndexOffset, IndexCount,
                           Entity->Transform, Assets->Materials[MaterialID]);
            }
        }

        if (Entity->Flags & EntityFlag_LightSource)
        {
            AddLight(Frame, { Entity->Transform.P, Entity->LightEmission });
        }
    }

    for (u32 ParticleSystemIndex = 0; ParticleSystemIndex < World->ParticleSystemCount; ParticleSystemIndex++)
    {
        particle_system* ParticleSystem = World->ParticleSystems + ParticleSystemIndex;
        v2 ParticleSize = { 1.0f, 1.0f };

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
                            .Alpha = 1.0f,
                            .dAlpha = 0.0f,
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
                            .Alpha = 1.0f,
                            .dAlpha = -1.25f,
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
            Particle->Alpha += Particle->dAlpha * dt;

            if (Cmd)
            {
                if (Frame->ParticleCount < Frame->MaxParticleCount)
                {
                    Cmd->ParticleCount++;
                    f32 Alpha = Max(Particle->Alpha, 0.0f);
                    Frame->Particles[Frame->ParticleCount++] = 
                    {
                        .P = Particle->P,
                        .TextureIndex = Particle->TextureIndex,
                        .Color = { Color.x, Color.y, Color.z, Alpha * Color.w },
                        .HalfExtent = ParticleSystem->ParticleHalfExtent,
                    };
                }
            }
        }
    }
}