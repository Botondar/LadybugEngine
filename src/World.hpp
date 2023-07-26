#pragma once

struct entity_id
{
    u32 Value;
};

inline b32 IsValid(entity_id ID);

enum entity_type : u32
{
    Entity_Undefined = 0,

    Entity_StaticMesh,
    Entity_SkinnedMesh,

    Entity_COUNT,
};

struct entity_reference
{
    entity_type Type;
    entity_id ID;
};

inline b32 IsValid(entity_reference Ref);

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

struct mesh_instance
{
    u32 MeshID;
    m4 Transform;
};

struct skinned_mesh_instance
{
    u32 MeshID;
    u32 SkinID;
    u32 CurrentAnimationID;
    b32 DoAnimation;
    f32 AnimationCounter;
    m4 Transform;
};

struct game_world
{
    b32 IsLoaded;
    camera Camera;
    v3 SunL;
    v3 SunV;

    static constexpr u32 MaxInstanceCount = 1u << 21;
    u32 InstanceCount;
    u32 SkinnedInstanceCount;
    mesh_instance Instances[MaxInstanceCount];
    skinned_mesh_instance SkinnedInstances[MaxInstanceCount];
};

//
// Implementation
//

inline b32 IsValid(entity_id ID)
{
    b32 Result = (ID.Value != U32_MAX);
    return(Result);
}

inline b32 IsValid(entity_reference Ref)
{
    b32 Result = (Ref.Type != Entity_Undefined) && IsValid(Ref.ID);
    return(Result);
}
