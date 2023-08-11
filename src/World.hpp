#pragma once

struct entity_id
{
    u32 Value;
};

inline b32 IsValid(entity_id ID);

typedef flags32 entity_flags;
enum entity_flag_bits : entity_flags
{
    EntityFlag_None = 0,

    EntityFlag_Mesh = (1u << 0),
    EntityFlag_Skin = (1u << 1),
    EntityFlag_LightSource = (1u << 2),
};

struct entity_piece
{
    u32 MeshID;
};

struct entity
{
    entity_flags Flags;
    m4 Transform;

    // EntityFlag_Mesh
    static constexpr u32 MaxPieceCount = 256;
    u32 PieceCount;
    entity_piece Pieces[MaxPieceCount];

    // EntityFlag_Skin
    u32 SkinID;
    u32 CurrentAnimationID;
    b32 DoAnimation;
    f32 AnimationCounter;

    // EntityFlag_LightSource
    v4 LightEmission;
};

struct particle
{
    v3 P;
    v3 dP;
    v3 ddP;
    v4 Color;
    v4 dColor;
    u32 TextureIndex;
};

enum particle_system_type
{
    ParticleSystem_Undefined = 0,
    ParticleSystem_Magic,
    ParticleSystem_Fire,

    ParticleSystem_COUNT,
};

struct particle_system
{
    entity_id ParentID;
    particle_system_type Type;

    billboard_mode Mode;
    v2 ParticleHalfExtent;

    b32 CullOutOfBoundsParticles;
    f32 Counter;
    f32 EmissionRate;

    mmbox Bounds;
    u32 NextParticle;

    static constexpr u32 MaxParticleCount = 512;
    u32 ParticleCount;
    particle Particles[MaxParticleCount];
};

struct camera
{
    v3 P;
    f32 Yaw;
    f32 Pitch;
    f32 NearZ;
    f32 FarZ;
    f32 FieldOfView;
};

lbfn m4 GetLocalTransform(const camera* Camera);
lbfn m4 GetTransform(const camera* Camera);

struct game_world
{
    b32 IsLoaded;
    f32 LightProxyScale; // NOTE(boti): Sphere scale for editor selection and rendering

    camera Camera;
    v3 SunL;
    v3 SunV;

    entropy32 EffectEntropy; // NOTE(boti): for visual effects only

    static constexpr u32 MaxEntityCount = (1u << 18);
    u32 EntityCount;
    entity Entities[MaxEntityCount];

    static constexpr u32 MaxParticleSystemCount = 512u;
    u32 ParticleSystemCount;
    particle_system ParticleSystems[MaxParticleSystemCount];

    mmbox AdHocLightBounds;
    static constexpr u32 AdHocLightCount = 192u;
    light AdHocLights[AdHocLightCount];
};

lbfn u32 MakeParticleSystem(game_world* World, particle_system_type Type, entity_id ParentID, mmbox Bounds);

lbfn void UpdateAndRenderWorld(game_world* World, struct assets* Assets, render_frame* Frame, game_io* IO, memory_arena* Scratch, b32 DrawLights);

//
// Implementation
//
inline b32 IsValid(entity_id ID)
{
    b32 Result = (ID.Value != U32_MAX);
    return(Result);
}