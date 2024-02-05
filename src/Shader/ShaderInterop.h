#if defined(__cplusplus)

#   define CONSTEXPR constexpr
#   define StaticAssert(expr) static_assert(expr)

struct v4u8
{
    u8 E[4];

    u8& operator[](u32 Index) { return E[Index]; }
};

#else

#   define CONSTEXPR const
#   define StaticAssert(expr)


#   define PI 3.14159265359

#   define flags32  uint
#   define u32      uint
#   define s32      int
#   define f32      float
#   define v2       vec2
#   define v2u      uvec2
#   define v2s      ivec2
#   define v3       vec3
#   define v4       vec4
#   define v4u8     u8vec4
#   define m2       mat2
#   define m3       mat3
#   define m4       mat4

// NOTE(boti): GLSL has the row/column counts backwards...
#   define m3x4     mat4x3

#   define rgba8               u32
#   define renderer_texture_id u32

#endif

//
// Config
//
#define R_AlphaTestThreshold 0.5f

#define R_TileSizeX             16
#define R_TileSizeY             16
#define R_MaxLightCountPerTile  255
#define R_LuminanceThreshold    1e-2f
#define R_MaxLightCount         16384
#define R_MaxShadowCount        64
#define R_MaxMipCount           16

#define Attrib_Position         0
#define Attrib_Normal           1
#define Attrib_TangentSign      2
#define Attrib_TexCoord         3
#define Attrib_Color            4
#define Attrib_JointWeights     5
#define Attrib_JointIndices     6

#define Set_PerFrame            0
#define Set_Sampler             1
#define Set_Bindless            2
#define SystemSet_Count         3

#define Set_User0               SystemSet_Count

// PerFrame set bindings
#define Binding_PerFrame_Constants                  0
#define Binding_PerFrame_LightBuffer                1
#define Binding_PerFrame_TileBuffer                 2
#define Binding_PerFrame_InstanceBuffer             3
#define Binding_PerFrame_DrawBuffer                 4
#define Binding_PerFrame_IndexBuffer                5
#define Binding_PerFrame_VertexBuffer               6
#define Binding_PerFrame_SkinnedVertexBuffer        7
#define Binding_PerFrame_VisibilityImage            8
#define Binding_PerFrame_OcclusionImage             9
#define Binding_PerFrame_StructureImage             10
#define Binding_PerFrame_OcclusionRawStorageImage   11
#define Binding_PerFrame_OcclusionStorageImage      12
#define Binding_PerFrame_OcclusionRawImage          13
// TODO(boti): Remove this once we have the mip views in place
#define Binding_PerFrame_HDRColorStorageImage       14
#define Binding_PerFrame_HDRColorImage              15
#define Binding_PerFrame_BloomImage                 16
#define Binding_PerFrame_CascadedShadow             17
#define Binding_PerFrame_HDRMipStorageImages        18
#define Binding_PerFrame_BloomMipStorageImages      19
// NOTE(boti): HDR and Bloom general images are workarounds
// for the image layout being baked into the set
// TODO(boti): Enhanced barriers are need for this in D3D12 (same-queue read + read/write)
#define Binding_PerFrame_HDRColorImageGeneral       20
#define Binding_PerFrame_BloomImageGeneral          21
#define Binding_PerFrame_PointShadows               22
#define Binding_PerFrame_JointBuffer                23
// TODO(boti): Maybe these are fine here, but I'd really rather have them in their own descriptor set
#define Binding_PerFrame_ParticleBuffer             24
#define Binding_PerFrame_ParticleTexture            25
#define Binding_PerFrame_TextureUI                  26

#define Binding_PerFrame_Count                      27

// Sampler set bindings
#define Binding_Sampler_NamedSamplers               0

// TODO(boti): Move this back to an enum once we have C++ metageneration going on
#define Sampler_Default                             0
#define Sampler_LinearUnnormalized                  1
#define Sampler_Shadow                              2
#define Sampler_LinearBlackBorder                   3
#define Sampler_LinearEdgeClamp                     4
#define Sampler_NoFilter                            5

#define Sampler_Count                               6

// Bindless bindings
#define Binding_Bindless_Textures                   0

#define light_flags flags32
#define LightFlag_None          0x00u
#define LightFlag_ShadowCaster  0x01u
#define LightFlag_Volumetric    0x02u

//
// Shader specific
//
#define LightBin_GroupSizeX             32
#define LightBin_GroupSizeY             1
#define LightBin_GroupSizeZ             1

#define SSAO_GroupSizeX                 8
#define SSAO_GroupSizeY                 8
#define SSAO_GroupSizeZ                 1

#define SSAOBlur_GroupSizeX             8
#define SSAOBlur_GroupSizeY             8
#define SSAOBlur_GroupSizeZ             1

#define Skin_GroupSizeX                 64
#define Skin_GroupSizeY                 1
#define Skin_GroupSizeZ                 1

#define BloomDownsample_GroupSizeX      8
#define BloomDownsample_GroupSizeY      8
#define BloomDownsample_GroupSizeZ      1

#define BloomUpsample_GroupSizeX        8
#define BloomUpsample_GroupSizeY        8
#define BloomUpsample_GroupSizeZ        1

#define ShadingVisibility_GroupSizeX    16
#define ShadingVisibility_GroupSizeY    16
#define ShadingVisibility_GroupSizeZ    1

#define LayoutGroupSize(name) layout(local_size_x = name##_GroupSizeX, local_size_y = name##_GroupSizeY, local_size_z = name##_GroupSizeZ) in
#define SetBinding(s, b) layout(set = Set_##s, binding = Binding_##s##_##b)
#define SetBindingLayout(s, b, l) layout(set = Set_##s, binding = Binding_##s##_##b, l)

//
// Data
//

struct vertex
{
    v3 P;
    v3 N;
    v4 T;
    v2 TexCoord;
    v4 Weights;
    v4u8 Joints;
    rgba8 Color;
};

struct light
{
    v3 P;
    u32 ShadowIndex;
    v3 E;
    light_flags Flags;
};

struct screen_tile
{
    u32 LightCount;
    u32 LightIndices[R_MaxLightCountPerTile];
};

struct point_shadow_data
{
    m4 ViewProjections[6];
    f32 Near;
    f32 Far;
};

struct per_frame
{
    m4 CameraTransform;
    m4 ViewTransform;
    m4 ProjectionTransform;
    m4 InverseProjectionTransform;
    m4 ViewProjectionTransform;

    m4 CascadeViewProjections[4];

    point_shadow_data PointShadows[R_MaxShadowCount];

    f32 CascadeMinDistances[4];
    f32 CascadeMaxDistances[4];
    v3 CascadeScales[3];
    v3 CascadeOffsets[3];

    f32 FocalLength;
    f32 AspectRatio;
    f32 NearZ;
    f32 FarZ;

    v3 SunV;
    v3 SunL;

    v2 ScreenSize;
    v2u TileCount;

    u32 OpaqueDrawCount;
    u32 SkinnedDrawCount;

    u32 LightCount;

    v3 Ambience;

    f32 Exposure;
    f32 SSAOIntensity;
    f32 SSAOInverseMaxDistance;
    f32 SSAOTangentTau;
    f32 BloomFilterRadius;
    f32 BloomInternalStrength;
    f32 BloomStrength;

    f32 ConstantFogDensity;
    f32 LinearFogDensityAtBottom;
    f32 LinearFogMinZ;
    f32 LinearFogMaxZ;
};
StaticAssert(sizeof(per_frame) <= KiB(64));

struct renderer_material
{
    v3 Emissive;
    rgba8 DiffuseColor;
    rgba8 BaseMaterial;
    renderer_texture_id DiffuseID;
    renderer_texture_id NormalID;
    renderer_texture_id MetallicRoughnessID;
};

struct instance_data
{
    m4 Transform;
    renderer_material Material;
};

struct draw_data
{
    u32 IndexCount;
    u32 InstanceCount;
    u32 IndexOffset;
    s32 VertexOffset;
    u32 InstanceOffset;
};