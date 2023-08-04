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

        v3 CenterP = 0.5f * (Bounds.Max + Bounds.Min);
        v3 HalfExtent = 0.5f * (Bounds.Max - Bounds.Min);

        switch (Type)
        {
            case ParticleSystem_Undefined:
            {
                // Ignored
            } break;
            case ParticleSystem_Magic:
            {
                ParticleSystem->ParticleCount = 128;
                for (u32 ParticleIndex = 0; ParticleIndex < ParticleSystem->ParticleCount; ParticleIndex++)
                {
                    v3 BaseP = { RandUnilateral(&World->EffectEntropy), RandUnilateral(&World->EffectEntropy), RandUnilateral(&World->EffectEntropy) };
                    v3 P = 2.0f * Hadamard(HalfExtent, BaseP) + Bounds.Min;
                    f32 dPz = 2.0f * RandUnilateral(&World->EffectEntropy) + 0.25f;
                    ParticleSystem->Particles[ParticleIndex] = 
                    {
                        .P = P,
                        .dP = { 0.0f, 0.0f, dPz },
                        .Alpha = 1.0f,
                        .TextureIndex = Particle_Trace02,
                    };
                }
            } break;
            case ParticleSystem_Fire:
            {
                ParticleSystem->ParticleCount = 128;
            } break;
            InvalidDefaultCase;
        }
    }
    return(Result);
}