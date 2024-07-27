//
// Utility
//
struct mesh_data
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

inline m4 TRSToM4(trs_transform Transform);
inline trs_transform M4ToTRS(m4 M);

//
// Skin and animation
//
enum armature_type : u32
{
    Armature_Undefined = 0,

    Armature_Mixamo,
};

// TODO(boti): Rename skin to armature
// NOTE(boti): Skin joints must not precede their parents in the array
struct skin
{
    armature_type Type;
    u32 JointCount;
    m4 InverseBindMatrices[R_MaxJointCount];
    // NOTE(boti): The bind-pose transforms are all parent-relative and not global
    trs_transform BindPose[R_MaxJointCount];
    u32 JointParents[R_MaxJointCount];
};

struct animation_key_frame
{
    trs_transform JointTransforms[R_MaxJointCount];
};

struct joint_mask
{
    u64 Bits[R_MaxJointCount / 64];
};

inline b32 JointMaskIndexFromJointIndex(u32 JointIndex, u32* ArrayIndex, u32* BitIndex);

struct animation
{
    u32 SkinID;
    u32 KeyFrameCount;
    f32 MinTimestamp;
    f32 MaxTimestamp;
    f32* KeyFrameTimestamps;
    animation_key_frame* KeyFrames;

    joint_mask ActiveJoints;
};

inline b32 IsJointActive(animation* Animation, u32 JointIndex);

enum mixamo_joint : u32
{
    Mixamo_Root = 0,

    Mixamo_Torso = Mixamo_Root,
    Mixamo_Spine,
    Mixamo_Spine1,
    Mixamo_Spine2,
    Mixamo_Neck,
    Mixamo_Head,
    Mixamo_HeadTop,
    Mixamo_LeftEye,
    Mixamo_RightEye,
    Mixamo_LeftShoulder,
    Mixamo_LeftArm,
    Mixamo_LeftForeArm,
    Mixamo_LeftHand,
    Mixamo_LeftHandThumb1,
    Mixamo_LeftHandThumb2,
    Mixamo_LeftHandThumb3,
    Mixamo_LeftHandThumb4,
    Mixamo_LeftHandIndex1,
    Mixamo_LeftHandIndex2,
    Mixamo_LeftHandIndex3,
    Mixamo_LeftHandIndex4,
    Mixamo_LeftHandMiddle1,
    Mixamo_LeftHandMiddle2,
    Mixamo_LeftHandMiddle3,
    Mixamo_LeftHandMiddle4,
    Mixamo_LeftHandRing1,
    Mixamo_LeftHandRing2,
    Mixamo_LeftHandRing3,
    Mixamo_LeftHandRing4,
    Mixamo_LeftHandPinky1,
    Mixamo_LeftHandPinky2,
    Mixamo_LeftHandPinky3,
    Mixamo_LeftHandPinky4,
    Mixamo_RightShoulder,
    Mixamo_RightArm,
    Mixamo_RightForeArm,
    Mixamo_RightHand,
    Mixamo_RightHandThumb1,
    Mixamo_RightHandThumb2,
    Mixamo_RightHandThumb3,
    Mixamo_RightHandThumb4,
    Mixamo_RightHandIndex1,
    Mixamo_RightHandIndex2,
    Mixamo_RightHandIndex3,
    Mixamo_RightHandIndex4,
    Mixamo_RightHandMiddle1,
    Mixamo_RightHandMiddle2,
    Mixamo_RightHandMiddle3,
    Mixamo_RightHandMiddle4,
    Mixamo_RightHandRing1,
    Mixamo_RightHandRing2,
    Mixamo_RightHandRing3,
    Mixamo_RightHandRing4,
    Mixamo_RightHandPinky1,
    Mixamo_RightHandPinky2,
    Mixamo_RightHandPinky3,
    Mixamo_RightHandPinky4,
    Mixamo_LeftHip,
    Mixamo_LeftKnee,
    Mixamo_LeftFoot,
    Mixamo_LeftToeBase,
    Mixamo_LeftToeEnd,
    Mixamo_RightHip,
    Mixamo_RightKnee,
    Mixamo_RightFoot,
    Mixamo_RightToeBase,
    Mixamo_RightToeEnd,

    Mixamo_Count,
};

internal const char* MixamoJointNamePrefix = "mixamorig:";
internal const char* MixamoJointNames[Mixamo_Count] =
{
    [Mixamo_Torso]              = "mixamorig:Hips",
    [Mixamo_Spine]              = "mixamorig:Spine",
    [Mixamo_LeftHip]            = "mixamorig:LeftUpLeg",
    [Mixamo_RightHip]           = "mixamorig:RightUpLeg",
    [Mixamo_Spine1]             = "mixamorig:Spine1",
    [Mixamo_LeftKnee]           = "mixamorig:LeftLeg",
    [Mixamo_RightKnee]          = "mixamorig:RightLeg",
    [Mixamo_Spine2]             = "mixamorig:Spine2",
    [Mixamo_LeftFoot]           = "mixamorig:LeftFoot",
    [Mixamo_RightFoot]          = "mixamorig:RightFoot",
    [Mixamo_Neck]               = "mixamorig:Neck",
    [Mixamo_LeftShoulder]       = "mixamorig:LeftShoulder",
    [Mixamo_RightShoulder]      = "mixamorig:RightShoulder",
    [Mixamo_LeftToeBase]        = "mixamorig:LeftToeBase",
    [Mixamo_RightToeBase]       = "mixamorig:RightToeBase",
    [Mixamo_Head]               = "mixamorig:Head",
    [Mixamo_LeftArm]            = "mixamorig:LeftArm",
    [Mixamo_RightArm]           = "mixamorig:RightArm",
    [Mixamo_LeftToeEnd]         = "mixamorig:LeftToe_End",
    [Mixamo_RightToeEnd]        = "mixamorig:RightToe_End",
    [Mixamo_HeadTop]            = "mixamorig:HeadTop_End",
    [Mixamo_LeftEye]            = "mixamorig:LeftEye",
    [Mixamo_RightEye]           = "mixamorig:RightEye",
    [Mixamo_LeftForeArm]        = "mixamorig:LeftForeArm",
    [Mixamo_RightForeArm]       = "mixamorig:RightForeArm",
    [Mixamo_LeftHand]           = "mixamorig:LeftHand",
    [Mixamo_RightHand]          = "mixamorig:RightHand",
    [Mixamo_LeftHandThumb1]     = "mixamorig:LeftHandThumb1",
    [Mixamo_LeftHandIndex1]     = "mixamorig:LeftHandIndex1",
    [Mixamo_LeftHandMiddle1]    = "mixamorig:LeftHandMiddle1",
    [Mixamo_LeftHandRing1]      = "mixamorig:LeftHandRing1",
    [Mixamo_LeftHandPinky1]     = "mixamorig:LeftHandPinky1",
    [Mixamo_RightHandThumb1]    = "mixamorig:RightHandThumb1",
    [Mixamo_RightHandIndex1]    = "mixamorig:RightHandIndex1",
    [Mixamo_RightHandMiddle1]   = "mixamorig:RightHandMiddle1",
    [Mixamo_RightHandRing1]     = "mixamorig:RightHandRing1",
    [Mixamo_RightHandPinky1]    = "mixamorig:RightHandPinky1",
    [Mixamo_LeftHandThumb2]     = "mixamorig:LeftHandThumb2",
    [Mixamo_LeftHandIndex2]     = "mixamorig:LeftHandIndex2",
    [Mixamo_LeftHandMiddle2]    = "mixamorig:LeftHandMiddle2",
    [Mixamo_LeftHandRing2]      = "mixamorig:LeftHandRing2",
    [Mixamo_LeftHandPinky2]     = "mixamorig:LeftHandPinky2",
    [Mixamo_RightHandThumb2]    = "mixamorig:RightHandThumb2",
    [Mixamo_RightHandIndex2]    = "mixamorig:RightHandIndex2",
    [Mixamo_RightHandMiddle2]   = "mixamorig:RightHandMiddle2",
    [Mixamo_RightHandRing2]     = "mixamorig:RightHandRing2",
    [Mixamo_RightHandPinky2]    = "mixamorig:RightHandPinky2",
    [Mixamo_LeftHandThumb3]     = "mixamorig:LeftHandThumb3",
    [Mixamo_LeftHandIndex3]     = "mixamorig:LeftHandIndex3",
    [Mixamo_LeftHandMiddle3]    = "mixamorig:LeftHandMiddle3",
    [Mixamo_LeftHandRing3]      = "mixamorig:LeftHandRing3",
    [Mixamo_LeftHandPinky3]     = "mixamorig:LeftHandPinky3",
    [Mixamo_RightHandThumb3]    = "mixamorig:RightHandThumb3",
    [Mixamo_RightHandIndex3]    = "mixamorig:RightHandIndex3",
    [Mixamo_RightHandMiddle3]   = "mixamorig:RightHandMiddle3",
    [Mixamo_RightHandRing3]     = "mixamorig:RightHandRing3",
    [Mixamo_RightHandPinky3]    = "mixamorig:RightHandPinky3",
    [Mixamo_LeftHandThumb4]     = "mixamorig:LeftHandThumb4",
    [Mixamo_LeftHandIndex4]     = "mixamorig:LeftHandIndex4",
    [Mixamo_LeftHandMiddle4]    = "mixamorig:LeftHandMiddle4",
    [Mixamo_LeftHandRing4]      = "mixamorig:LeftHandRing4",
    [Mixamo_LeftHandPinky4]     = "mixamorig:LeftHandPinky4",
    [Mixamo_RightHandThumb4]    = "mixamorig:RightHandThumb4",
    [Mixamo_RightHandIndex4]    = "mixamorig:RightHandIndex4",
    [Mixamo_RightHandMiddle4]   = "mixamorig:RightHandMiddle4",
    [Mixamo_RightHandRing4]     = "mixamorig:RightHandRing4",
    [Mixamo_RightHandPinky4]    = "mixamorig:RightHandPinky4",
};

//
// Texture
//

struct texture
{
    renderer_texture_id RendererID;
    b32 IsLoaded;
    texture_info Info;
    void* Memory;
};

enum texture_data_type : u32
{
    TextureData_Undefined = 0,

    TextureData_Albedo,
    TextureData_Normal,
    TextureData_Roughness,
    TextureData_Metallic,
    TextureData_Occlusion,
    TextureData_Height,
    TextureData_Transmission,

    TextureData_Count,
};
static_assert(TextureData_Count < 32);

// TODO(boti): Occlusion and height could be packed into the same BC5 texture,
// but right now that would greatly complicate texture loader interface.
// Really mip generation/BC compression should be done as a preprocess step, not at runtime...
enum texture_type : u32
{
    TextureType_Albedo, // NOTE(boti): Also includes coverage when alpha is present
    TextureType_Normal,
    TextureType_RoMe,
    TextureType_Occlusion,
    TextureType_Height,
    TextureType_Transmission,

    TextureType_Count,
};

enum texture_channel : u32
{
    TextureChannel_Undefined = 0,

    TextureChannel_R,
    TextureChannel_G,
    TextureChannel_B,
    TextureChannel_A,
};

inline u32 IndexFromChannel(texture_channel Channel)
{
    Assert(Channel);
    u32 Result = (u32)Channel - 1;
    return(Result);
}

struct texture_load_op
{
    texture_type Type;
    filepath Path;

    struct
    {
        texture_channel RoughnessChannel;
        texture_channel MetallicChannel;
    } RoughnessMetallic;
};

struct texture_queue_entry
{
    texture* Texture;

    // Input parameters for processing
    texture_load_op Op;

    // Output parameters for transfer
    volatile b32 ReadyToTransfer;
    umm TotalSize;
    texture_info Info;
};

struct texture_queue
{
    platform_semaphore Semaphore;
    memory_arena Scratch;

    static constexpr u32 MaxEntryCount = 1u << 14;
    u32 ProcessingCount;
    volatile u32 CompletionCount;
    volatile u32 CompletionGoal;
    texture_queue_entry Entries[MaxEntryCount];

    umm RingBufferSize;
    volatile umm RingBufferWriteAt;
    volatile umm RingBufferReadAt;
    u8* RingBufferMemory;
};

lbfn b32 IsEmpty(texture_queue* Queue);
lbfn void AddEntry(texture_queue* Queue, texture* Texture, texture_load_op Op);
lbfn b32 ProcessEntry(texture_queue* Queue);
// NOTE(boti): Wraps around when Begin+TotalSize > MemorySize
lbfn umm GetNextEntryOffset(texture_queue* Queue, umm TotalSize, umm Begin);

//
// Material
//
enum transparency_mode : u32
{
    Transparency_Opaque = 0,
    Transparency_AlphaTest,
    Transparency_AlphaBlend,

    Transparency_Count,
};

struct material
{
    u32 AlbedoID;
    u32 NormalID;
    u32 MetallicRoughnessID;
    u32 OcclusionID;
    u32 HeightID;
    material_sampler_id AlbedoSamplerID;
    material_sampler_id NormalSamplerID;
    material_sampler_id MetallicRoughnessSamplerID;
    transparency_mode Transparency;
    rgba8 Albedo;
    rgba8 MetallicRoughness;
    v3 Emission;
    f32 AlphaThreshold;
    b32 TransmissionEnabled;
    u32 TransmissionID;
    material_sampler_id TransmissionSamplerID;
    f32 Transmission;
};

//
// Mesh
//
struct mesh
{
    geometry_buffer_allocation Allocation;
    mmbox BoundingBox;
    u32 MaterialID;
};

struct model
{
    static constexpr u32 MaxMeshCount = 256;
    u32 MeshCount;
    u32 Meshes[MaxMeshCount];
};

enum default_mesh : u32
{
    DefaultMesh_Cube = 0,
    DefaultMesh_Sphere,
    DefaultMesh_Arrow,
    DefaultMesh_Pyramid,

    DefaultMesh_Count,
};

//
// Assets
//

struct assets
{
    memory_arena Arena;
    memory_arena TextureCache;
    texture_queue TextureQueue;

    u32 WhitenessID;
    u32 HalfGrayID;
    u32 DefaultTextures[TextureType_Count];

    u32 ParticleArrayID;

    u32 DefaultFontTextureID;
    font DefaultFont;

    u32 DefaultMeshIDs[DefaultMesh_Count];
    static constexpr u32 MaxTreeModelCount = 16;
    u32 TreeModelCount;
    u32 TreeModels[MaxTreeModelCount];

    static constexpr u32 MaxTextureCount = 1u << 20;
    static constexpr u32 MaxMeshCount = 1u << 16;
    static constexpr u32 MaxModelCount = 1u << 16;
    static constexpr u32 MaxMaterialCount = 1u << 14;
    static constexpr u32 MaxSkinCount = 1u << 14;
    static constexpr u32 MaxAnimationCount = 1u << 16;
    u32 TextureCount;
    u32 MeshCount;
    u32 ModelCount;
    u32 MaterialCount;
    u32 SkinCount;
    u32 AnimationCount;

    texture Textures[MaxTextureCount];
    mesh Meshes[MaxMeshCount];
    model Models[MaxModelCount];
    material Materials[MaxMaterialCount];
    skin Skins[MaxSkinCount];
    animation Animations[MaxAnimationCount];
};

inline mesh* GetDefaultMesh(assets* Assets, u32 Mesh);

struct texture_set
{
    u32 IDs[TextureType_Count];
};

struct texture_set_entry
{
    const char* Path;
    texture_channel RoughnessChannel;
    texture_channel MetallicChannel;
};

lbfn texture_set DEBUGLoadTextureSet(assets* Assets, texture_set_entry Entries[TextureType_Count]);

lbfn b32 InitializeAssets(assets* Assets, render_frame* Frame, memory_arena* Scratch);
lbfn void UpdateAssets(assets* Assets);
lbfn void ProcessTextureRequests(assets* Assets, render_frame* Frame);

lbfn void LoadDebugFont(memory_arena* Arena, assets* Assets, render_frame* Frame, const char* Path);

typedef flags32 debug_load_flags;
enum debug_load_flag_bits : debug_load_flags
{
    DEBUGLoad_None                  = 0,

    DEBUGLoad_AddNodesAsEntities    = (1u << 0),
};

lbfn void DEBUGLoadTestScene(memory_arena* Scratch,
                             assets* Assets, 
                             struct game_world* World, 
                             render_frame* Frame,
                             debug_load_flags LoadFlags,
                             const char* ScenePath, 
                             m4 BaseTransform);

lbfn void AssetThread(void* Param);

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


//
// Implementation
//

inline mesh* GetDefaultMesh(assets* Assets, u32 Mesh)
{
    Assert(Mesh < DefaultMesh_Count);

    mesh* Result = Assets->Meshes + Assets->DefaultMeshIDs[Mesh];
    return(Result);
}

inline m4 TRSToM4(trs_transform Transform)
{
    m4 Result;

    m4 S = M4(Transform.Scale.X, 0.0f, 0.0f, 0.0f,
              0.0f, Transform.Scale.Y, 0.0f, 0.0f,
              0.0f, 0.0f, Transform.Scale.Z, 0.0f,
              0.0f, 0.0f, 0.0f, 1.0f);
    m4 R = QuaternionToM4(Transform.Rotation);
    m4 T = M4(1.0f, 0.0f, 0.0f, Transform.Position.X,
              0.0f, 1.0f, 0.0f, Transform.Position.Y,
              0.0f, 0.0f, 1.0f, Transform.Position.Z,
              0.0f, 0.0f, 0.0f, 1.0f);

    Result = T * R * S;
    return(Result);
}

inline trs_transform M4ToTRS(m4 M)
{
    trs_transform Result = {};

    Result.Position = M.P.XYZ;
    Result.Scale.X = Sqrt(Dot(M.X.XYZ, M.X.XYZ));
    Result.Scale.Y = Sqrt(Dot(M.Y.XYZ, M.Y.XYZ));
    Result.Scale.Z = Sqrt(Dot(M.Z.XYZ, M.Z.XYZ));
    v3 X = M.X.XYZ * (1.0f / Result.Scale.X);
    v3 Y = M.Y.XYZ * (1.0f / Result.Scale.Y);
    v3 Z = M.Z.XYZ * (1.0f / Result.Scale.Z);

    // NOTE(boti): Enable this assert to check whether the matrix does reflection or not
#if 0
    f32 Det = X.X * (Y.Y*Z.Z - Y.Z*Z.Y) - Y.X * (X.Y*Z.Z - X.Z*Z.Y) + Z.X * (X.Y*Y.Z - X.Z*Y.Y);
    Assert(Det > 0.0f);
#endif

    f32 Sum = X.X + Y.Y + Z.Z;
    if (Sum > 0.0f)
    {
        Result.Rotation.W = 0.5f * Sqrt(Sum + 1.0f);
        f32 f = 0.25f / Result.Rotation.W;
        Result.Rotation.X = f * (Y.Z - Z.Y);
        Result.Rotation.Y = f * (Z.X - X.Z);
        Result.Rotation.Z = f * (X.Y - Y.X);
    }
    else if ((X.X> Y.Y) && (X.Z > Z.Z))
    {
        Result.Rotation.X = 0.5f * Sqrt(X.X - Y.Y - Z.Z + 1.0f);
        f32 f = 0.25f / Result.Rotation.X;
        Result.Rotation.X = f * (X.Y + Y.X);
        Result.Rotation.Y = f * (Y.Z + Z.Y);
        Result.Rotation.W = f * (Z.X - X.Z);
    }
    else if (Y.Y > Z.Z)
    {
        Result.Rotation.Y = 0.5f * Sqrt(Y.Y - X.X - Z.Z + 1.0f);
        f32 f = 0.25f / Result.Rotation.Y;
        Result.Rotation.X = f * (X.Y + Y.X);
        Result.Rotation.Z = f * (Y.Z + Z.Y);
        Result.Rotation.W = f * (Z.X - X.Z);
    }
    else
    {
        Result.Rotation.Z = 0.5f * Sqrt(Z.Z - X.X - Y.Y + 1.0f);
        f32 f = 0.25f / Result.Rotation.Z;
        Result.Rotation.X = f * (X.Z + Z.X);
        Result.Rotation.Y = f * (Y.Y + Z.Y);
        Result.Rotation.W = f * (X.Y - Y.X);
    }
    return(Result);
}

inline b32 JointMaskIndexFromJointIndex(u32 JointIndex, u32* ArrayIndex, u32* BitIndex)
{
    b32 Result = false;
    if (JointIndex < R_MaxJointCount)
    {
        *ArrayIndex = JointIndex / 64;
        *BitIndex = JointIndex % 64;
        Result = true;
    }
    return(Result);
}

inline b32 IsJointActive(animation* Animation, u32 JointIndex)
{
    b32 Result = false;
    u32 ArrayIndex, BitIndex;
    if (JointMaskIndexFromJointIndex(JointIndex, &ArrayIndex, &BitIndex))
    {
        u64 Mask = 1llu << BitIndex;
        Result = (Animation->ActiveJoints.Bits[ArrayIndex] & Mask) != 0;
    }
    return(Result);
}