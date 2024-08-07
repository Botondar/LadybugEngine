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


#   define PI           3.14159265359
#   define OneOverPi    0.31830988618

#   define flags32  uint
#   define u32      uint
#   define s32      int
#   define f32      float
#   define u64      uint64_t
#   define v2       vec2
#   define v2u      uvec2
#   define v2s      ivec2
#   define v3       vec3
#   define v3u      uvec3
#   define v3s      ivec3
#   define v4       vec4
#   define v4u8     u8vec4
#   define m2       mat2
#   define m3       mat3
#   define m4       mat4

// NOTE(boti): GLSL has the row/column counts backwards...
#   define m3x4     mat4x3

#   define rgba8               u32
#   define renderer_texture_id u32
#   define material_sampler_id u32

#endif

//
// Config
//
#define R_TileSizeX             16
#define R_TileSizeY             16
#define R_MaxLightCountPerTile  255
#define R_LuminanceThreshold    1e-2f
#define R_MaxLightCount         16384
#define R_MaxTextureCount       262144u
#define R_MaxShadowCount        64
#define R_MaxMipCount           16

// NOTE(boti): Material samplers are really just 
#define R_MaterialSamplerCount  64

#define Attrib_Position         0
#define Attrib_Normal           1
#define Attrib_TangentSign      2
#define Attrib_TexCoord         3
#define Attrib_Color            4
#define Attrib_JointWeights     5
#define Attrib_JointIndices     6

#define Set_Static              0
#define Set_Sampler             1
#define Set_Bindless            2
#define Set_PerFrame            3

#define Set_Count               4

#define Set_User0               Set_Count

// Static set bindings
#define Binding_Static_MipFeedbackBuffer            0
#define Binding_Static_CascadedShadow               1
#define Binding_Static_PointShadows                 2
#define Binding_Static_VisibilityImage              3
#define Binding_Static_OcclusionImage               4
#define Binding_Static_StructureImage               5
#define Binding_Static_OcclusionRawStorageImage     6
#define Binding_Static_OcclusionStorageImage        7
#define Binding_Static_OcclusionRawImage            8
#define Binding_Static_HDRColorImage                9
#define Binding_Static_BloomImage                   10
// NOTE(boti): HDR and Bloom general images are workarounds
// for the image layout being baked into the set
// TODO(boti): Enhanced barriers are need for this in D3D12 (same-queue read + read/write)
#define Binding_Static_HDRColorImageGeneral         11
#define Binding_Static_BloomImageGeneral            12
#define Binding_Static_HDRMipStorageImages          13
#define Binding_Static_BloomMipStorageImages        14
#define Binding_Static_TransparentImage             15
#define Binding_Static_SkyTexture                   16
#define Binding_Static_SkyImage                     17
#define Binding_Static_BRDFLutImage                 18
#define Binding_Static_BRDFLutTexture               19

#define Binding_Static_Count                        20

// Sampler set bindings
#define Binding_Sampler_NamedSamplers               0
#define Binding_Sampler_MaterialSamplers            1

#define Binding_Sampler_Count                       2

// Bindless bindings
#define Binding_Bindless_Textures                   0

#define Binding_Bindless_Count                      1

// PerFrame set bindings
#define Binding_PerFrame_Constants                  0
#define Binding_PerFrame_ParticleTexture            1
#define Binding_PerFrame_TextureUI                  2

#define Binding_PerFrame_Count                      3

// Named sampler binding indices
#define Sampler_Default                             0
#define Sampler_LinearUnnormalized                  1
#define Sampler_Shadow                              2
#define Sampler_LinearBlackBorder                   3
#define Sampler_LinearEdgeClamp                     4
#define Sampler_NoFilter                            5

#define Sampler_Count                               6

//
// Shader specific
//
#define LightBin_GroupSizeX             R_TileSizeX
#define LightBin_GroupSizeY             R_TileSizeY
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

#define EnvironmentBRDF_GroupSizeX      16
#define EnvironmentBRDF_GroupSizeY      16
#define EnvironmentBRDF_GroupSizeZ      1

#define Sky_GroupSizeX                  8
#define Sky_GroupSizeY                  8
#define Sky_GroupSizeZ                  1

#define LayoutGroupSize(name) layout(local_size_x = name##_GroupSizeX, local_size_y = name##_GroupSizeY, local_size_z = name##_GroupSizeZ) in
#define SetBinding(s, b) layout(set = Set_##s, binding = Binding_##s##_##b)
#define SetBindingLayout(s, b, l) layout(set = Set_##s, binding = Binding_##s##_##b, l)

//
// Data
//

#define light_flags flags32
#define LightFlag_None          0x00u
#define LightFlag_ShadowCaster  0x01u
#define LightFlag_Volumetric    0x02u

#define draw_group u32
#define DrawGroup_Opaque            0
#define DrawGroup_AlphaTest         1
#define DrawGroup_Transparent       2

#define DrawGroup_Count             3

StaticAssert(DrawGroup_Count < 32);

#define debug_view_mode u32
#define DebugView_None              0
#define DebugView_LightOccupancy    1
#define DebugView_InstanceID        2
#define DebugView_TriangleID        3
#define DebugView_SSAO              4
#define DebugView_Structure         5

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

struct vertex_2d
{
    v2 P;
    v2 TexCoord;
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

    u64 IndexBufferAddress;
    u64 LightBufferAddress;
    u64 TileBufferAddress;
    u64 InstanceBufferAddress;
    u64 DrawBufferAddress;

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

    v2u RenderExtent;
    v2u OutputExtent;
    v2u TileCount;

    u32 DrawGroupCounts[DrawGroup_Count];
    u32 DrawGroupOffsets[DrawGroup_Count];

    u32 LightCount;

    v3 Ambience;

    debug_view_mode DebugViewMode;

    f32 Exposure;
    f32 ParallaxScale;
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
    renderer_texture_id AlbedoID;
    renderer_texture_id NormalID;
    renderer_texture_id MetallicRoughnessID;
    // HACK(boti): Occlusion and height are currently just using the albedo sampler
    renderer_texture_id OcclusionID; 
    renderer_texture_id HeightID;
    renderer_texture_id TransmissionID;
    // TODO(boti): These sampler IDs could just be packed into a single u32
    material_sampler_id AlbedoSamplerID;
    material_sampler_id NormalSamplerID;
    material_sampler_id MetallicRoughnessSamplerID;
    material_sampler_id TransmissionSamplerID;

    f32 AlphaThreshold;
    f32 Transmission;
    rgba8 BaseAlbedo;
    rgba8 BaseMaterial;
    v3 Emissive;
};

struct instance_data
{
    m4 Transform;
    u64 VertexBufferAddress; 
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