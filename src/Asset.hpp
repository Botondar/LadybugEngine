#pragma once

struct mesh
{
    u32 VertexCount;
    u32 IndexCount;
    vertex* VertexData;
    vert_index* IndexData;
    mmbox Box;
};

struct trs_transform
{
    v4 Rotation;
    v3 Position;
    v3 Scale;
};

struct skin
{
    static constexpr u32 MaxJointCount = 256;
    u32 JointCount;
    m4 InverseBindMatrices[MaxJointCount];
    trs_transform BindPose[MaxJointCount];
    u32 JointParents[MaxJointCount];
};

struct animation_key_frame
{
    trs_transform JointTransforms[skin::MaxJointCount];
};

struct animation
{
    u32 SkinID;
    u32 KeyFrameCount;
    f32 MinTimestamp;
    f32 MaxTimestamp;
    f32* KeyFrameTimestamps;
    animation_key_frame* KeyFrames;
};

struct assets
{
    memory_arena* Arena;

    texture_id DefaultDiffuseID;
    texture_id DefaultNormalID;
    texture_id DefaultMetallicRoughnessID;

    texture_id DefaultFontTextureID;
    font DefaultFont;

    static constexpr u32 MaxMeshCount = 1u << 16;
    static constexpr u32 MaxMaterialCount = 1u << 14;
    static constexpr u32 MaxSkinCount = 1u << 14;
    static constexpr u32 MaxAnimationCount = 1u << 16;
    u32 MeshCount;
    u32 MaterialCount;
    u32 SkinCount;
    u32 AnimationCount;

    geometry_buffer_allocation Meshes[MaxMeshCount];
    mmbox MeshBoxes[MaxMeshCount];
    u32 MeshMaterialIndices[MaxMeshCount];

    material Materials[MaxMaterialCount];

    skin Skins[MaxSkinCount];

    animation Animations[MaxAnimationCount];
};

inline m4 TRSToM4(trs_transform Transform);

static void LoadDebugFont(memory_arena* Arena, assets* Assets, renderer* Renderer, const char* Path);
static void DEBUGLoadTestScene(memory_arena* Scratch, 
                               assets* Assets, struct game_world* World, renderer* Renderer,
                               const char* ScenePath, m4 BaseTransform);

inline m4 TRSToM4(trs_transform Transform)
{
    m4 Result;

    m4 S = M4(Transform.Scale.x, 0.0f, 0.0f, 0.0f,
              0.0f, Transform.Scale.y, 0.0f, 0.0f,
              0.0f, 0.0f, Transform.Scale.z, 0.0f,
              0.0f, 0.0f, 0.0f, 1.0f);
    m4 R = QuaternionToM4(Transform.Rotation);
    m4 T = M4(1.0f, 0.0f, 0.0f, Transform.Position.x,
              0.0f, 1.0f, 0.0f, Transform.Position.y,
              0.0f, 0.0f, 1.0f, Transform.Position.z,
              0.0f, 0.0f, 0.0f, 1.0f);

    Result = T * R * S;
    return(Result);
}