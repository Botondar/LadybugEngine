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
};

struct entity
{
    entity_flags Flags;
    m4 Transform;

    // EntityFlag_Mesh
    u32 MeshID;

    // EntityFlag_Skin
    u32 SkinID;
    u32 CurrentAnimationID;
    b32 DoAnimation;
    f32 AnimationCounter;
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
    camera Camera;
    v3 SunL;
    v3 SunV;

    static constexpr u32 MaxEntityCount = (1u << 21);
    u32 EntityCount;
    entity Entities[MaxEntityCount];
};

//
// Implementation
//

inline b32 IsValid(entity_id ID)
{
    b32 Result = (ID.Value != U32_MAX);
    return(Result);
}

