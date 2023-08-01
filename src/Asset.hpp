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

enum particle_texture : u32
{
    Particle_Circle01,
    Particle_Circle02,
    Particle_Circle03,
    Particle_Circle04,
    Particle_Circle05,
    Particle_Dirt01,
    Particle_Dirt02,
    Particle_Dirt03,
    Particle_Fire01,
    Particle_Fire02,
    Particle_Flame01,
    Particle_Flame02,
    Particle_Flame03,
    Particle_Flame04,
    Particle_Flame05,
    Particle_Flame06,
    Particle_Flare01,
    Particle_Light01,
    Particle_Light02,
    Particle_Light03,
    Particle_Magic01,
    Particle_Magic02,
    Particle_Magic03,
    Particle_Magic04,
    Particle_Magic05,
    Particle_Muzzle01,
    Particle_Muzzle02,
    Particle_Muzzle03,
    Particle_Muzzle04,
    Particle_Muzzle05,
    Particle_Scorch01,
    Particle_Scorch02,
    Particle_Scorch03,
    Particle_Scratch01,
    Particle_Slash01,
    Particle_Slash02,
    Particle_Slash03,
    Particle_Slash04,
    Particle_Smoke01,
    Particle_Smoke02,
    Particle_Smoke03,
    Particle_Smoke04,
    Particle_Smoke05,
    Particle_Smoke06,
    Particle_Smoke07,
    Particle_Smoke08,
    Particle_Smoke09,
    Particle_Smoke10,
    Particle_Spark01,
    Particle_Spark02,
    Particle_Spark03,
    Particle_Spark04,
    Particle_Spark05,
    Particle_Spark06,
    Particle_Spark07,
    Particle_Star01,
    Particle_Star02,
    Particle_Star03,
    Particle_Star04,
    Particle_Star05,
    Particle_Star06,
    Particle_Star07,
    Particle_Star08,
    Particle_Star09,
    Particle_Symbol01,
    Particle_Symbol02,
    Particle_Trace01,
    Particle_Trace02,
    Particle_Trace03,
    Particle_Trace04,
    Particle_Trace05,
    Particle_Trace06,
    Particle_Trace07,
    Particle_Twirl01,
    Particle_Twirl02,
    Particle_Twirl03,
    Particle_Window01,
    Particle_Window02,
    Particle_Window03,
    Particle_Window04,

    Particle_COUNT,
};
extern const char* ParticlePaths[Particle_COUNT];

struct assets
{
    memory_arena* Arena;

    texture_id DefaultDiffuseID;
    texture_id DefaultNormalID;
    texture_id DefaultMetallicRoughnessID;

    texture_id DefaultFontTextureID;
    font DefaultFont;

    texture_id ParticleArrayID;

    u32 SphereMeshID;

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