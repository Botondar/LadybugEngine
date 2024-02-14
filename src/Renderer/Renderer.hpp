#pragma once

#include <LadybugLib/Core.hpp>
#include <LadybugLib/Intrinsics.hpp>
#include <LadybugLib/String.hpp>

// HACK(boti): These are part of the high-level rendering API,
// but we need them in ShaderInterop so they're defined here at the top for now
struct renderer_texture_id
{
    u32 Value;
};

union rgba8
{
    u32 Color;
    struct
    {
        u8 R;
        u8 G;
        u8 B;
        u8 A;
    };
};
static_assert(sizeof(rgba8) == sizeof(u32));
#include <Shader/ShaderInterop.h>

static_assert(sizeof(per_frame) <= KiB(64));

//
// Config
//
constexpr u32 R_MaxFramesInFlight = 2u;

constexpr u32 R_MaxRenderTargetSizeX = 3840;
constexpr u32 R_MaxRenderTargetSizeY = 2160;
constexpr u32 R_MaxTileCountX = CeilDiv(R_MaxRenderTargetSizeX, R_TileSizeX);
constexpr u32 R_MaxTileCountY = CeilDiv(R_MaxRenderTargetSizeY, R_TileSizeY);

constexpr u64 R_RenderTargetMemorySize      = MiB(320);
constexpr u64 R_TextureMemorySize           = MiB(1024llu);
constexpr u64 R_ShadowMapMemorySize         = MiB(256);
constexpr u32 R_MaxShadowCascadeCount       = 4;
constexpr u32 R_ShadowResolution            = 2048u; // TODO(boti): Rename, this only applies to the cascades
constexpr u32 R_PointShadowResolution       = 256u;
constexpr u64 R_VertexBufferMaxBlockCount   = (1llu << 18);
constexpr u64 R_SkinningBufferSize          = MiB(128);

//
// Limits
//
constexpr u32 MaxVertexBindingCount = 16;
constexpr u32 MaxVertexAttribCount = 32;
constexpr u32 MaxColorAttachmentCount = 8;
constexpr u32 MaxDescriptorSetCount = 32;
constexpr u32 MaxDescriptorSetLayoutBindingCount = 32;
constexpr u32 MinPushConstantSize = 128;

//
// Low-level rendering
//

constexpr f32 GlobalMaxLOD = 1000.0f;

typedef flags32 format_flags;
enum format_flag_bits : format_flags
{
    FormatFlag_None = 0,

    FormatFlag_BlockCompressed = (1u << 0),
};

enum format : u32
{
    Format_Undefined = 0,

    Format_R8_UNorm,
    Format_R8_UInt,
    Format_R8_SRGB,
    Format_R8G8_UNorm,
    Format_R8G8_UInt,
    Format_R8G8_SRGB,
    Format_R8G8B8_UNorm,
    Format_R8G8B8_UInt,
    Format_R8G8B8_SRGB,
    Format_R8G8B8A8_UNorm,
    Format_R8G8B8A8_UInt,
    Format_R8G8B8A8_SRGB,

    Format_B8G8R8A8_SRGB,

    Format_R16_UNorm,
    Format_R16_UInt,
    Format_R16_Float,
    Format_R16G16_UNorm,
    Format_R16G16_UInt,
    Format_R16G16_Float,
    Format_R16G16B16_UNorm,
    Format_R16G16B16_UInt,
    Format_R16G16B16_Float,
    Format_R16G16B16A16_UNorm,
    Format_R16G16B16A16_UInt,
    Format_R16G16B16A16_Float,

    Format_R32_UInt,
    Format_R32_Float,
    Format_R32G32_UInt,
    Format_R32G32_Float,
    Format_R32G32B32_UInt,
    Format_R32G32B32_Float,
    Format_R32G32B32A32_UInt,
    Format_R32G32B32A32_Float,

    Format_R11G11B10_Float,

    // NOTE(boti): Stencil always UInt, D32 always float, D(<32) always UNorm
    Format_D16,
    Format_D24X8,
    Format_D32,
    Format_S8,
    Format_D16S8,
    Format_D24S8,
    Format_D32S8, // NOTE(boti): This is padded to 64-bits (D32S8X24)
    
    Format_BC1_RGB_UNorm,
    Format_BC1_RGB_SRGB,
    Format_BC1_RGBA_UNorm,
    Format_BC1_RGBA_SRGB,
    Format_BC2_UNorm,
    Format_BC2_SRGB,
    Format_BC3_UNorm,
    Format_BC3_SRGB,
    Format_BC4_UNorm,
    Format_BC4_SNorm,
    Format_BC5_UNorm,
    Format_BC5_SNorm,
    Format_BC6_UFloat,
    Format_BC6_SFloat,
    Format_BC7_UNorm,
    Format_BC7_SRGB,

    Format_Count,
};

enum blend_op : u32
{
    BlendOp_Add = 0,
    BlendOp_Subtract,
    BlendOp_ReverseSubtract,
    BlendOp_Min,
    BlendOp_Max,

    BlendOp_Count,
};

enum blend_factor : u32
{
    Blend_Zero = 0,
    Blend_One,
    Blend_SrcColor,
    Blend_InvSrcColor,
    Blend_DstColor,
    Blend_InvDstColor,
    Blend_SrcAlpha,
    Blend_InvSrcAlpha,
    Blend_DstAlpha,
    Blend_InvDstAlpha,
    Blend_ConstantColor,
    Blend_InvConstantColor,
    Blend_ConstantAlpha,
    Blend_InvConstantAlpha,

    Blend_Count,
};

enum compare_op : u32
{
    Compare_Never = 0,
    Compare_Always,
    Compare_Equal,
    Compare_NotEqual,
    Compare_Less,
    Compare_LessEqual,
    Compare_Greater,
    Compare_GreaterEqual,

    Compare_Count,
};

enum tex_filter : u32
{
    Filter_Nearest = 0,
    Filter_Linear,

    Filter_Count,
};

enum tex_wrap : u32
{
    Wrap_Repeat = 0,
    Wrap_RepeatMirror,
    Wrap_ClampToEdge,
    Wrap_ClampToBorder,

    Wrap_Count,
};

enum tex_anisotropy : u32
{
    Anisotropy_None = 0,

    Anisotropy_1,
    Anisotropy_2,
    Anisotropy_4,
    Anisotropy_8,
    Anisotropy_16,

    Anisotropy_Count,
};

enum tex_border : u32
{
    Border_Black = 0,
    Border_White = 1,

    Border_Count,
};

struct sampler_state
{
    tex_filter MagFilter;
    tex_filter MinFilter;
    tex_filter MipFilter;
    tex_wrap WrapU;
    tex_wrap WrapV;
    tex_wrap WrapW;
    tex_anisotropy Anisotropy;
    b32 EnableComparison;
    compare_op Comparison;
    f32 MinLOD;
    f32 MaxLOD;
    tex_border Border;
    b32 EnableUnnormalizedCoordinates;
};

// NOTE(boti): Packed samplers are only used by materials, which means:
//  - Anisotropy is always 16x
//  - Comparison is always disabled
//  - Min/Max LOD is always [0, GlobalMax]
//  - Border is always white
//  - Coords are always normalized
union packed_sampler
{
    static constexpr u32 BitsUsed = 9;
    static constexpr u32 MaxSamplerCount = (1u << (BitsUsed + 1));

    u32 Value;
    struct
    {
        tex_filter MagFilter : 1;
        tex_filter MinFilter : 1;
        tex_filter MipFilter : 1;
        tex_wrap WrapU : 2;
        tex_wrap WrapV : 2;
        tex_wrap WrapW : 2;
    };
};

inline sampler_state UnpackSampler(packed_sampler Sampler);

// TODO(boti): This is binary compatible with Vulkan for now,
// but with mutable descriptor types it could be collapsed down to the D3D12 model?
enum descriptor_type : u32
{
    Descriptor_Sampler = 0,
    Descriptor_ImageSampler = 1, // TODO(boti): Remove combined image sampler
    Descriptor_SampledImage = 2,
    Descriptor_StorageImage = 3,
    Descriptor_UniformTexelBuffer = 4,
    Descriptor_StorageTexelBuffer = 5,
    Descriptor_UniformBuffer = 6,
    Descriptor_StorageBuffer = 7,
    Descriptor_DynamicUniformBuffer = 8,
    Descriptor_DynamicStorageBuffer = 9,
//  Descriptor_InputAttachment, 
    Descriptor_InlineUniformBlock = 11,

    Descriptor_Count,
};

typedef flags32 descriptor_flags;
enum descriptor_flag_bits : descriptor_flags
{
    DescriptorFlag_None = 0,

    DescriptorFlag_PartiallyBound   = (1 << 0),
    DescriptorFlag_Bindless         = (1 << 1),
};

enum pipeline_type : u32
{
    PipelineType_Graphics = 0,
    PipelineType_Compute,

    PipelineType_Count,
};

typedef flags32 pipeline_stages;
enum pipeline_stage_bits : pipeline_stages
{
    PipelineStage_None = 0,

    PipelineStage_VS = (1 << 0),
    PipelineStage_PS = (1 << 1),
    PipelineStage_HS = (1 << 2),
    PipelineStage_DS = (1 << 3),
    PipelineStage_GS = (1 << 4),

    PipelineStage_CS = (1 << 5),

    PipelineStage_Count = 6, // NOTE(boti): This has to be kept up to date !!!

    PipelineStage_AllGfx = PipelineStage_VS|PipelineStage_PS|PipelineStage_DS|PipelineStage_HS|PipelineStage_GS,
    PipelineStage_All = PipelineStage_AllGfx|PipelineStage_CS,
};


enum topology_type : u32
{
    Topology_Undefined = 0,

    Topology_PointList,
    Topology_LineList,
    Topology_LineStrip,
    Topology_TriangleList,
    Topology_TriangleStrip,
    Topology_TriangleFan,
    Topology_LineListAdjacency,
    Topology_LineStripAdjacency,
    Topology_TriangleListAdjacency,
    Topology_TriangleStripAdjacency,
    Topology_PatchList,

    Topology_Count,
};

struct vertex_attrib
{
    u32 Index;
    u32 Binding;
    format Format;
    u32 ByteOffset;
};

struct vertex_binding
{
    u32 Stride;
    u32 InstanceStepRate; // NOTE(boti): 0 is per-vertex, >0 is instanced
};

struct vertex_state
{
    topology_type Topology;
    b32 EnablePrimitiveRestart;
    u32 BindingCount;
    u32 AttribCount;
    vertex_binding Bindings[MaxVertexBindingCount];
    vertex_attrib Attribs[MaxVertexAttribCount];
};

enum fill_mode : u32
{
    Fill_Solid = 0,
    Fill_Line,
    Fill_Point,
};

enum cull_flags : u32
{
    Cull_None = 0,
    Cull_Back = (1 << 0),
    Cull_Front = (1 << 1),
};

struct descriptor_set_binding
{
    u32 Binding;
    descriptor_type Type;
    u32 DescriptorCount;
    pipeline_stages Stages;
    descriptor_flags Flags;
};

struct descriptor_set_layout_info
{
    u32 BindingCount;
    descriptor_set_binding Bindings[MaxDescriptorSetLayoutBindingCount];
};

struct pipeline_layout_info
{
    u32 DescriptorSetCount;
    u32 DescriptorSets[MaxDescriptorSetCount];
};

typedef flags32 rasterizer_flags;
enum rasterizer_flag_bits : rasterizer_flags
{
    RS_Flags_None = 0,

    RS_DepthClampEnable = (1 << 0),
    RS_DiscardEnable    = (1 << 1),
    RS_DepthBiasEnable  = (1 << 2),
    RS_FrontCW          = (1 << 3),
};

struct rasterizer_state
{
    rasterizer_flags Flags;
    fill_mode Fill;
    cull_flags CullFlags;

    f32 DepthBiasConstantFactor;
    f32 DepthBiasClamp;
    f32 DepthBiasSlopeFactor;
};

typedef flags32 depth_stencil_flags;
enum depth_stencil_flag_bits : depth_stencil_flags
{
    DS_Flags_None = 0,

    DS_DepthTestEnable = (1 << 0),
    DS_DepthWriteEnable = (1 << 1),
    DS_DepthBoundsTestEnable = (1 << 2),
    DS_StencilTestEnable = (1 << 3),
};

struct depth_stencil_state
{
    depth_stencil_flags Flags;
    compare_op DepthCompareOp;
    f32 MinDepthBounds;
    f32 MaxDepthBounds;
};

struct blend_attachment
{
    b32 BlendEnable;
    blend_factor SrcColor;
    blend_factor DstColor;
    blend_op ColorOp;
    blend_factor SrcAlpha;
    blend_factor DstAlpha;
    blend_op AlphaOp;
};

enum render_target_format : u32
{
    RTFormat_Undefined = 0,
    
    RTFormat_Depth,
    RTFormat_HDR,
    RTFormat_Structure,
    RTFormat_Visibility,
    RTFormat_Occlusion,
    RTFormat_Shadow,
    RTFormat_Swapchain,

    RTFormat_Count,
};

struct pipeline_info
{
    const char* Name;
    pipeline_type Type;

    pipeline_layout_info Layout;

    //
    // Graphics pipeline specific
    //
    pipeline_stages EnabledStages;

    vertex_state InputAssemblerState;
    rasterizer_state RasterizerState;
    depth_stencil_state DepthStencilState;
    u32 BlendAttachmentCount;
    blend_attachment BlendAttachments[MaxColorAttachmentCount];

    u32 ColorAttachmentCount;
    render_target_format ColorAttachments[MaxColorAttachmentCount];
    render_target_format DepthAttachment;
    render_target_format StencilAttachment;
};

enum cube_layer : u32
{
    Layer_PositiveX = 0,
    Layer_NegativeX,
    Layer_PositiveY,
    Layer_NegativeY,
    Layer_PositiveZ,
    Layer_NegativeZ,

    Layer_Count,
};

enum texture_swizzle_type : u8
{
    Swizzle_Identity = 0,
    Swizzle_Zero,
    Swizzle_One,
    Swizzle_R,
    Swizzle_G,
    Swizzle_B,
    Swizzle_A,
};

struct texture_swizzle
{
    texture_swizzle_type R, G, B, A;
};

struct format_byterate
{
    u32 Numerator;
    u32 Denominator;
    format_flags Flags;
};

struct texture_info
{
    u32 Width;
    u32 Height;
    u32 Depth;
    u32 MipCount;
    u32 ArrayCount;
    format Format;
    texture_swizzle Swizzle;
};

// NOTE(boti): Set the counts to U32_MAX to indicate all mips/arrays
struct texture_subresource_range
{
    u32 BaseMip;
    u32 MipCount;
    u32 BaseArray;
    u32 ArrayCount;
};

inline texture_subresource_range AllTextureSubresourceRange()
{
    texture_subresource_range Range = { 0, U32_MAX, 0, U32_MAX };
    return(Range);
}

inline b32 AreTextureInfosSameFormat(texture_info A, texture_info B)
{
    b32 Result = 
        (A.Width == B.Width) &&
        (A.Height == B.Height) &&
        (A.Depth == B.Depth) &&
        (A.MipCount == B.MipCount) &&
        (A.ArrayCount == B.ArrayCount) &&
        (A.Format == B.Format);
    return(Result);
}

//
// High-level renderer
//

union frustum
{
    v4 Planes[6];
    struct
    {
        v4 Left;
        v4 Right;
        v4 Top;
        v4 Bottom;
        v4 Near;
        v4 Far;
    };
};

inline frustum GetClipSpaceFrustum();

inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box);
inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box, m4 Transform);
inline b32 IntersectFrustumSphere(const frustum* Frustum, v3 P, f32 r);

//
// Render API
//

constexpr renderer_texture_id InvalidRendererTextureID = { U32_MAX };
inline bool IsValid(renderer_texture_id ID) { return ID.Value != InvalidRendererTextureID.Value; }

inline f32 GetLuminance(v3 RGB);
inline v3 SetLuminance(v3 RGB, f32 Luminance);

// NOTE(boti): Normally textures get put into the bindless descriptor heap,
// specifying a texture as "Special" prevents this, and it will be the user code's
// responsibility to manage its descriptor
typedef flags32 texture_flags;
enum texture_flag_bits : texture_flags
{
    TextureFlag_None                = 0,

    TextureFlag_Special             = (1u << 0),
    TextureFlag_PersistentMemory    = (1u << 1),
};

struct texture_request
{
    renderer_texture_id TextureID;
    u32 MipMask;
};

// Shading mode for the primary opaque pass
enum shading_mode : u32
{
    ShadingMode_Forward = 0,
    ShadingMode_Visibility,

    ShadingMode_Count,
};

struct render_config
{
    shading_mode ShadingMode;
    debug_view_mode DebugViewMode;
    f32 Exposure;

    f32 SSAOIntensity;
    f32 SSAOMaxDistance;
    f32 SSAOTangentTau;

    f32 BloomFilterRadius;
    f32 BloomInternalStrength;
    f32 BloomStrength;

    static constexpr f32 DefaultSSAOIntensity = 8.0f;
    static constexpr f32 DefaultSSAOMaxDistance = 0.5f;
    static constexpr f32 DefaultSSAOTangentTau = 0.03125f;

    static constexpr f32 DefaultBloomFilterRadius = 0.005f;
    static constexpr f32 DefaultBloomInternalStrength = 1.0f;
    static constexpr f32 DefaultBloomStrength = 0.04f;
};

enum draw_group : u32
{
    DrawGroup_Opaque = 0,
    DrawGroup_Skinned,

    DrawGroup_Count,
};

typedef u32 vert_index;

struct vertex_skin8
{
    v4 Weights;
    u8 Joints[4];
};
typedef vertex_skin8 vertex_skin;

struct vertex_2d
{
    v2 P;
    v2 TexCoord;
    rgba8 Color;
};

enum billboard_mode : u32
{
    Billboard_ViewAligned = 0, // Billboards facing the camera directly
    Billboard_ZAligned = 1, // Billboards aligned to the vertical axis (but rotated horizontally)
};

struct render_particle
{
    v3 P;
    u32 TextureIndex;
    v4 Color;
    v2 HalfExtent;
};

struct particle_cmd
{
    u32 FirstParticle;
    u32 ParticleCount;
    billboard_mode Mode;
};

// NOTE(boti): binary-compatible with Vulkan/D3D12
struct draw_indirect_cmd
{
    u32 VertexCount;
    u32 InstanceCount;
    u32 VertexOffset;
    u32 InstanceOffset;
};

// NOTE(boti): binary-compatible with Vulkan/D3D12
struct draw_indirect_index_cmd
{
    u32 IndexCount;
    u32 InstanceCount;
    u32 IndexOffset;
    u32 VertexOffset;
    u32 InstanceOffset;
};

struct draw_cmd
{
    draw_indirect_index_cmd Base;
    m4 Transform;
    mmbox BoundingBox;
    renderer_material Material;
};

struct skinning_cmd
{
    u32 SrcVertexOffset;
    u32 DstVertexOffset;
    u32 VertexCount;
    u32 PoseOffset;
};

struct draw_widget3d_cmd
{
    draw_indirect_index_cmd Base;
    m4 Transform;
    rgba8 Color;
};

struct geometry_buffer_block
{
    u32 Count;
    u32 Offset;

    geometry_buffer_block* Next;
    geometry_buffer_block* Prev;
};

struct geometry_buffer_allocation
{
    geometry_buffer_block* VertexBlock;
    geometry_buffer_block* IndexBlock;
};

struct renderer;
struct backend_render_frame;

struct render_stat_entry
{
    const char* Name;
    umm UsedSize;
    umm AllocationSize;
};

struct render_stats
{
    umm TotalMemoryUsed;
    umm TotalMemoryAllocated;

    static constexpr u32 MaxEntryCount = 1024u;
    u32 EntryCount;
    render_stat_entry Entries[MaxEntryCount];
};

enum transfer_op_type : u32
{
    TransferOp_Undefined = 0,

    TransferOp_Texture,
    TransferOp_Geometry,
};

struct transfer_texture
{
    renderer_texture_id TargetID;
    texture_info Info;
    texture_subresource_range SubresourceRange;
};

struct transfer_geometry
{
    geometry_buffer_allocation Dest;
};

struct transfer_op
{
    transfer_op_type Type;
    umm SourceOffset;
    union
    {
        transfer_texture Texture;
        transfer_geometry Geometry;
    };
};

struct render_frame
{
    renderer* Renderer;
    backend_render_frame* Backend;
    memory_arena* Arena; // NOTE(boti): filled by the game

    // NOTE(boti): Texture upload requests _from_ the renderer _to_ the game
    u32 TextureRequestCount;
    texture_request* TextureRequests;

    render_stats Stats;

    b32 ReloadShaders;

    u32 FrameID;
    u32 RenderWidth;
    u32 RenderHeight;

    renderer_texture_id ImmediateTextureID;
    renderer_texture_id ParticleTextureID; // TODO(boti): Move this to the particle command?

    render_config Config;

    // Environment
    struct
    {
        v3 SunL;
        v3 SunV; // World-space sun direction

        f32 ConstantFogDensity;
        f32 LinearFogDensityAtBottom;
        f32 LinearFogMinZ;
        f32 LinearFogMaxZ;
    };

    m4 CameraTransform;
    f32 CameraFocalLength;
    f32 CameraFarPlane;
    f32 CameraNearPlane;

    // Backend-calculated/cached values
    // TODO(boti): move these out of the API
    struct
    {
        frustum CameraFrustum;
        m4 ViewTransform;
        m4 ProjectionTransform;
        m4 InverseProjectionTransform;
    };

    umm StagingBufferSize;
    umm StagingBufferAt;
    void* StagingBufferBase;

    // Limits
    static constexpr u32 MaxTransferOpCount         = (1u << 15);
    static constexpr u32 MaxParticleCount           = (1u << 18);
    static constexpr u32 MaxJointCount              = (1u << 17);
    u32 MaxLightCount;
    u32 MaxShadowCount;
    u32 MaxDrawCmdCount;
    u32 MaxSkinnedDrawCmdCount;
    u32 MaxSkinningCmdCount;
    u32 MaxParticleDrawCmdCount;
    u32 MaxDrawWidget3DCmdCount;
    u32 MaxVertex2DCount;
    // TODO(boti): There's no need to have the skinned vertex count in the frontend, the backend
    // can just keep track of everything when executing the skinning cmds
    u32 MaxSkinnedVertexCount;
    u32 SkinnedMeshVertexCount;

    u32 JointBufferAlignment;

    u32 TransferOpCount;
    transfer_op TransferOps[MaxTransferOpCount];

    u32 LightCount;
    light* Lights;

    u32 ShadowCount;
    u32* Shadows;

    u32 DrawCmdCount;
    draw_cmd* DrawCmds;
    
    u32 SkinnedDrawCmdCount;
    draw_cmd* SkinnedDrawCmds;

    u32 JointCount;
    m4* JointMapping;

    u32 SkinningCmdCount;
    skinning_cmd* SkinningCmds;

    u32 ParticleDrawCmdCount;
    particle_cmd* ParticleDrawCmds;

    u32 ParticleCount;
    render_particle* Particles;

    u32 DrawWidget3DCmdCount;
    draw_widget3d_cmd* DrawWidget3DCmds;

    u32 Vertex2DCount;
    vertex_2d* Vertex2DArray;

    // TODO(boti): Remove these from the API (should be backend only)
    void* UniformData; // GPU-backed frame_uniform_data
    per_frame Uniforms;
};

#define Signature_CreateRenderer(name)      renderer*                   name(struct platform_api* PlatformAPI, memory_arena* Arena, memory_arena* Scratch)
#define Signature_GetDeviceName(name)       const char*                 name(renderer* Renderer)
#define Signature_AllocateGeometry(name)    geometry_buffer_allocation  name(renderer* Renderer, u32 VertexCount, u32 IndexCount)
/* NOTE(boti): Passing nullptr as Info is allowed as a way to allocate a texture _name_ only.
 * Such a name can't be used as a placeholder until it has been uploaded with some data (after a frame boundary).
 */ 
#define Signature_AllocateTexture(name)     renderer_texture_id         name(renderer* Renderer, texture_flags Flags, const texture_info* Info, renderer_texture_id Placeholder)
#define Signature_BeginRenderFrame(name)    render_frame*               name(renderer* Renderer, memory_arena* Arena, v2u OutputExtent)
#define Signature_EndRenderFrame(name)      void                        name(render_frame* Frame)

typedef Signature_CreateRenderer(create_renderer);
typedef Signature_GetDeviceName(get_device_name);
typedef Signature_AllocateGeometry(allocate_geometry);
typedef Signature_AllocateTexture(allocate_texture);
typedef Signature_BeginRenderFrame(begin_render_frame);
typedef Signature_EndRenderFrame(end_render_frame);

//
// Frame rendering
//
inline b32 
TransferTexture(render_frame* Frame, renderer_texture_id ID, 
                texture_info Info, texture_subresource_range Range,
                const void* Data);
inline b32 TransferGeometry(render_frame* Frame, geometry_buffer_allocation Allocation,
                            const void* VertexData, const void* IndexData);

inline b32 
DrawMesh(render_frame* Frame,
         geometry_buffer_allocation Allocation, 
         m4 Transform,
         mmbox BoundingBox,
         renderer_material Material);

inline b32 
DrawSkinnedMesh(render_frame* Frame,
                geometry_buffer_allocation Allocation,
                m4 Transform,
                mmbox BoundingBox,
                renderer_material Material,
                u32 JointCount, m4* Pose);

inline b32 
DrawWidget3D(render_frame* Frame,
             geometry_buffer_allocation Allocation,
             m4 Transform, rgba8 Color);
inline b32 AddLight(render_frame* Frame, v3 P, v3 E, light_flags Flags);

inline b32 DrawTriangleList2D(render_frame* Frame, u32 VertexCount, vertex_2d* VertexArray);

//
// Helpers
//
inline constexpr rgba8 PackRGBA8(u32 R, u32 G, u32 B, u32 A = 0xFF);
inline rgba8 PackRGBA(v4 Color);
inline u32 GetMaxMipCount(u32 Width, u32 Height);
inline u32 GetMipChainTexelCount(u32 Width, u32 Height, u32 MaxMipCount = 0xFFFFFFFFu);
inline u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, u32 ArrayCount, format_byterate ByteRate);
inline umm GetPackedTexture2DMipOffset(const texture_info* Info, u32 Mip);
inline texture_subresource_range SubresourceFromMipMask(u32 Mask, texture_info Info);

static format_byterate FormatByterateTable[Format_Count] = 
{
    [Format_Undefined]          = { 0, 0, FormatFlag_None },

    [Format_R8_UNorm]           = { 1*1, 1, FormatFlag_None },
    [Format_R8_UInt]            = { 1*1, 1, FormatFlag_None },
    [Format_R8_SRGB]            = { 1*1, 1, FormatFlag_None },
    [Format_R8G8_UNorm]         = { 2*1, 1, FormatFlag_None },
    [Format_R8G8_UInt]          = { 2*1, 1, FormatFlag_None },
    [Format_R8G8_SRGB]          = { 2*1, 1, FormatFlag_None },
    [Format_R8G8B8_UNorm]       = { 3*1, 1, FormatFlag_None },
    [Format_R8G8B8_UInt]        = { 3*1, 1, FormatFlag_None },
    [Format_R8G8B8_SRGB]        = { 3*1, 1, FormatFlag_None },
    [Format_R8G8B8A8_UNorm]     = { 4*1, 1, FormatFlag_None },
    [Format_R8G8B8A8_UInt]      = { 4*1, 1, FormatFlag_None },
    [Format_R8G8B8A8_SRGB]      = { 4*1, 1, FormatFlag_None },
    [Format_B8G8R8A8_SRGB]      = { 4*1, 1, FormatFlag_None },
    [Format_R16_UNorm]          = { 1*2, 1, FormatFlag_None },
    [Format_R16_UInt]           = { 1*2, 1, FormatFlag_None },
    [Format_R16_Float]          = { 1*2, 1, FormatFlag_None },
    [Format_R16G16_UNorm]       = { 2*2, 1, FormatFlag_None },
    [Format_R16G16_UInt]        = { 2*2, 1, FormatFlag_None },
    [Format_R16G16_Float]       = { 2*2, 1, FormatFlag_None },
    [Format_R16G16B16_UNorm]    = { 3*2, 1, FormatFlag_None },
    [Format_R16G16B16_UInt]     = { 3*2, 1, FormatFlag_None },
    [Format_R16G16B16_Float]    = { 3*2, 1, FormatFlag_None },
    [Format_R16G16B16A16_UNorm] = { 4*2, 1, FormatFlag_None },
    [Format_R16G16B16A16_UInt]  = { 4*2, 1, FormatFlag_None },
    [Format_R16G16B16A16_Float] = { 4*2, 1, FormatFlag_None },
    [Format_R32_UInt]           = { 1*4, 1, FormatFlag_None },
    [Format_R32_Float]          = { 1*4, 1, FormatFlag_None },
    [Format_R32G32_UInt]        = { 2*4, 1, FormatFlag_None },
    [Format_R32G32_Float]       = { 2*4, 1, FormatFlag_None },
    [Format_R32G32B32_UInt]     = { 3*4, 1, FormatFlag_None },
    [Format_R32G32B32_Float]    = { 3*4, 1, FormatFlag_None },
    [Format_R32G32B32A32_UInt]  = { 4*4, 1, FormatFlag_None },
    [Format_R32G32B32A32_Float] = { 4*4, 1, FormatFlag_None },

    [Format_R11G11B10_Float]    = { 4, 1, FormatFlag_None },
    [Format_D16]                = { 2, 1, FormatFlag_None },
    [Format_D24X8]              = { 4, 1, FormatFlag_None },
    [Format_D32]                = { 4, 1, FormatFlag_None },
    [Format_S8]                 = { 1, 1, FormatFlag_None },
    [Format_D24S8]              = { 4, 1, FormatFlag_None },
    [Format_D32S8]              = { 8, 1, FormatFlag_None },

    [Format_BC1_RGB_UNorm]  = { 1, 2, FormatFlag_BlockCompressed },
    [Format_BC1_RGB_SRGB]   = { 1, 2, FormatFlag_BlockCompressed },
    [Format_BC1_RGBA_UNorm] = { 1, 2, FormatFlag_BlockCompressed },
    [Format_BC1_RGBA_SRGB]  = { 1, 2, FormatFlag_BlockCompressed },
    [Format_BC2_UNorm]      = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC2_SRGB]       = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC3_UNorm]      = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC3_SRGB]       = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC4_UNorm]      = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC4_SNorm]      = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC5_UNorm]      = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC5_SNorm]      = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC6_UFloat]     = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC6_SFloat]     = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC7_UNorm]      = { 1, 1, FormatFlag_BlockCompressed },
    [Format_BC7_SRGB]       = { 1, 1, FormatFlag_BlockCompressed },
};

//
// Implementation
//

inline sampler_state UnpackSampler(packed_sampler Sampler)
{
    sampler_state Result =
    {
        .MagFilter = Sampler.MagFilter,
        .MinFilter = Sampler.MinFilter,
        .MipFilter = Sampler.MipFilter,
        .WrapU = Sampler.WrapU,
        .WrapV = Sampler.WrapV,
        .WrapW = Sampler.WrapW,
        .Anisotropy = Anisotropy_16,
        .EnableComparison = false,
        .Comparison = Compare_Always,
        .MinLOD = 0.0f,
        .MaxLOD = GlobalMaxLOD,
        .Border = Border_White,
        .EnableUnnormalizedCoordinates = false,
    };
    return(Result);
}

inline constexpr rgba8 PackRGBA8(u32 R, u32 G, u32 B, u32 A /*= 0xFF*/)
{
    rgba8 Result = 
    {
        .R = (u8)R,
        .G = (u8)G,
        .B = (u8)B,
        .A = (u8)A,
    };
    return Result;
}

inline rgba8 PackRGBA(v4 Color)
{
    rgba8 Result = 
    {
        .R = (u8)Round(Clamp(Color.X, 0.0f, 1.0f) * 255.0f),
        .G = (u8)Round(Clamp(Color.Y, 0.0f, 1.0f) * 255.0f),
        .B = (u8)Round(Clamp(Color.Z, 0.0f, 1.0f) * 255.0f),
        .A = (u8)Round(Clamp(Color.W, 0.0f, 1.0f) * 255.0f),
    };
    return Result;
}

inline rgba8 PackRGBA(v3 Color)
{
    rgba8 Result = 
    {
        .R = (u8)Round(Clamp(Color.X, 0.0f, 1.0f) * 255.0f),
        .G = (u8)Round(Clamp(Color.Y, 0.0f, 1.0f) * 255.0f),
        .B = (u8)Round(Clamp(Color.Z, 0.0f, 1.0f) * 255.0f),
        .A = 0xFFu,
    };
    return Result;
}

inline f32 GetLuminance(v3 RGB)
{
    f32 Result = Dot(RGB, v3{ 0.212639f, 0.715169f, 0.072192f });
    return(Result);
}
inline v3 SetLuminance(v3 RGB, f32 Luminance)
{
    f32 CurrentLuminance = GetLuminance(RGB);
    v3 Result = RGB * Ratio0(Luminance, CurrentLuminance);
    return(Result);
}

inline u32 GetMaxMipCount(u32 Width, u32 Height)
{
    u32 Result = 0;
    if (BitScanReverse(&Result, Max(Width, Height)))
    {
        Result += 1;
    }
    else
    {
        // NOTE(boti): The x64/MSDN spec says that the result is undefined if value is 0,
        //             so we kind of have to make sure that the result will also be 0 in that case.
        Result = 0; 
    }
    return Result;
}

// NOTE(boti): Returns the mip count of a mip chain when the lowest resolution mip level
//             must be at least 2x2
inline u32 GetMaxMipCountGreaterThanOne(u32 Width, u32 Height)
{
    u32 Result = 0;
    if (!BitScanReverse(&Result, Min(Width, Height)))
    {
        Result = 0;
    }
    return Result;
}

inline u32 GetMipChainTexelCount(u32 Width, u32 Height, u32 MaxMipCount /*= 0xFFFFFFFFu*/)
{
    u32 Result = 0;
    u32 MipCount = Min(GetMaxMipCount(Width, Height), MaxMipCount);

    u32 CurrentSize = Width * Height;

    for (u32 i = 0; i < MipCount; i++)
    {
        Result += CurrentSize;
        CurrentSize = Max(CurrentSize / 4, 1u);
    }

    return Result;
}

inline u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, u32 ArrayCount, format_byterate ByteRate)
{
    u64 Result = 0;

    for (u32 Mip = 0; Mip < MipCount; Mip++)
    {
        u32 CurrentWidth = Max(Width >> Mip, 1u);
        u32 CurrentHeight = Max(Height >> Mip, 1u);
        if (ByteRate.Flags & FormatFlag_BlockCompressed)
        {
            CurrentWidth = Align(CurrentWidth, 4u);
            CurrentHeight = Align(CurrentHeight, 4u);
        }

        Result += ((u64)CurrentWidth * (u64)CurrentHeight * ByteRate.Numerator) / ByteRate.Denominator;
    }
    Result *= ArrayCount;

    return Result;
}

inline umm GetPackedTexture2DMipOffset(const texture_info* Info, u32 Mip)
{
    umm Result = 0;

    // TODO(boti): This is really just the same code as GetMipChainSize...
    // NOTE(boti): MipCount from info is ignored, 
    // we return the offset even if that mip level is not present in the texture
    u32 MipCount = GetMaxMipCount(Info->Width, Info->Height);
    if (Mip < MipCount)
    {
        format_byterate Byterate = FormatByterateTable[Info->Format];

        for (u32 CurrentMip = 0; CurrentMip < Mip; CurrentMip++)
        {
            u32 Width = Max(Info->Width >> CurrentMip, 1u);
            u32 Height = Max(Info->Height >> CurrentMip, 1u);
            if (Byterate.Flags & FormatFlag_BlockCompressed)
            {
                Width = Align(Width, 4u);
                Height = Align(Height, 4u);
            }
            Result += ((umm)Width * (umm)Height * Byterate.Numerator) / Byterate.Denominator;
        }
    }

    return(Result);
}

inline texture_subresource_range SubresourceFromMipMask(u32 Mask, texture_info Info)
{
    texture_subresource_range Result = AllTextureSubresourceRange();

    if (Info.ArrayCount != 1) UnimplementedCodePath;

    u32 MostDetailedMipInMask;
    if (BitScanReverse(&MostDetailedMipInMask, Mask))
    {
        u32 MipCount = GetMaxMipCount(Info.Width, Info.Height);
        u32 ResolutionFromMask = 1 << MostDetailedMipInMask;
        u32 MipCountFromMask = GetMaxMipCount(ResolutionFromMask, ResolutionFromMask);

        u32 MaxExtent = Max(Info.Width, Info.Height);
        if (MipCountFromMask > MipCount) 
        {
            MipCountFromMask = MipCount;
        }
        else
        {
            while (MaxExtent && (ResolutionFromMask < MaxExtent))
            {
                Result.BaseMip++;
                MaxExtent >>= 1;
            }
        }

        u32 LeastDetailedMipInMask;
        BitScanForward(&LeastDetailedMipInMask, Mask);
        u32 LeastDetailedMipResolution = 1 << LeastDetailedMipInMask;
        if (LeastDetailedMipResolution > MaxExtent)
        {
            Result.MipCount = 0;
        }
        else
        {
            // TODO(boti): Handle all bits from the mask, not just the highest one
            Result.MipCount = Info.MipCount - Result.BaseMip;
        }
    }

    return(Result);
}

inline b32 
TransferTexture(render_frame* Frame, renderer_texture_id ID, 
                texture_info Info, texture_subresource_range Range,
                const void* Data)
{
    b32 Result = true;

    if (Frame->TransferOpCount < Frame->MaxTransferOpCount)
    {
        format_byterate ByteRate = FormatByterateTable[Info.Format];
        umm TotalSize = GetMipChainSize(Info.Width, Info.Height, Info.MipCount, Info.ArrayCount, ByteRate);
        umm EffectiveOffset = Align(Frame->StagingBufferAt, 64);
        if (EffectiveOffset + TotalSize <= Frame->StagingBufferSize)
        {
            transfer_op* Op = Frame->TransferOps + Frame->TransferOpCount++;
            Op->Type = TransferOp_Texture;
            Op->SourceOffset = EffectiveOffset;
            Op->Texture.TargetID = ID;
            Op->Texture.Info = Info;
            Op->Texture.SubresourceRange = Range;
            memcpy(OffsetPtr(Frame->StagingBufferBase, EffectiveOffset), Data, TotalSize);
            Frame->StagingBufferAt = EffectiveOffset + TotalSize;
        }
        else
        {
            Result = false;
            UnhandledError("Out of staging memory");
        }
    }
    else
    {
        Result = false;
        UnhandledError("Out of transfer pool");
    }
    return(Result);
}

inline b32 TransferGeometry(render_frame* Frame, geometry_buffer_allocation Allocation,
                            const void* VertexData, const void* IndexData)
{
    b32 Result = true;

    if (Frame->TransferOpCount < Frame->MaxTransferOpCount)
    {
        umm VertexSize = (umm)Allocation.VertexBlock->Count * sizeof(vertex);
        umm IndexSize = (umm)Allocation.IndexBlock->Count * sizeof(vert_index);
        umm TotalSize = VertexSize + IndexSize;
        if (Frame->StagingBufferAt + TotalSize < Frame->StagingBufferSize)
        {
            transfer_op* Op = Frame->TransferOps + Frame->TransferOpCount++;
            Op->Type = TransferOp_Geometry;
            Op->SourceOffset = Frame->StagingBufferAt;
            Op->Geometry.Dest = Allocation;
            memcpy(OffsetPtr(Frame->StagingBufferBase, Frame->StagingBufferAt), VertexData, VertexSize);
            memcpy(OffsetPtr(Frame->StagingBufferBase, Frame->StagingBufferAt + VertexSize), IndexData, IndexSize);
            Frame->StagingBufferAt += TotalSize;
        }
        else
        {
            Result = false;
            UnhandledError("Out of staging memory");
        }
    }
    else
    {
        Result = false;
        UnhandledError("Out of transfer pool");
    }

    return(Result);
}

inline b32 
DrawMesh(render_frame* Frame, 
         geometry_buffer_allocation Allocation,
         m4 Transform,
         mmbox BoundingBox,
         renderer_material Material)
{
    b32 Result = false;
    if (Frame->DrawCmdCount < Frame->MaxDrawCmdCount)
    {
        Frame->DrawCmds[Frame->DrawCmdCount++] = 
        {
            .Base = 
            {
                .IndexCount = Allocation.IndexBlock->Count,
                .InstanceCount = 1,
                .IndexOffset = Allocation.IndexBlock->Offset,
                .VertexOffset = Allocation.VertexBlock->Offset,
                .InstanceOffset = 0,
            },
            .Transform = Transform,
            .BoundingBox = BoundingBox,
            .Material = Material,
        };
        Result = true;
    }
    return(Result);
}

inline b32 
DrawSkinnedMesh(render_frame* Frame,
                geometry_buffer_allocation Allocation,
                m4 Transform,
                mmbox BoundingBox,
                renderer_material Material,
                u32 JointCount, m4* Pose)
{
    b32 Result = false;

    if ((Frame->SkinningCmdCount < Frame->MaxSkinningCmdCount) &&
        (Frame->SkinnedDrawCmdCount < Frame->MaxSkinnedDrawCmdCount) &&
        (Frame->SkinnedMeshVertexCount + Allocation.VertexBlock->Count <= Frame->MaxSkinnedVertexCount) &&
        (Frame->JointCount + JointCount <= Frame->MaxJointCount))
    {
        u32 DstVertexOffset = Frame->SkinnedMeshVertexCount;
        Frame->SkinnedMeshVertexCount += Allocation.VertexBlock->Count;

        memcpy(Frame->JointMapping + Frame->JointCount, Pose, JointCount * sizeof(m4));
        u32 JointBufferOffset = Frame->JointCount * sizeof(m4);
        Frame->JointCount = Align(Frame->JointCount + JointCount, Frame->JointBufferAlignment);

        Frame->SkinningCmds[Frame->SkinningCmdCount++] =
        {
            .SrcVertexOffset = Allocation.VertexBlock->Offset,
            .DstVertexOffset = DstVertexOffset,
            .VertexCount = Allocation.VertexBlock->Count,
            .PoseOffset = JointBufferOffset,
        };

        Frame->SkinnedDrawCmds[Frame->SkinnedDrawCmdCount++] = 
        {
            .Base = 
            {
                .IndexCount = Allocation.IndexBlock->Count,
                .InstanceCount = 1,
                .IndexOffset = Allocation.IndexBlock->Offset,
                .VertexOffset = DstVertexOffset,
                .InstanceOffset = 0,
            },
            .Transform = Transform,
            .BoundingBox = BoundingBox,
            .Material = Material,
        };
        Result = true;
    }

    return(Result);
}

inline b32 
DrawWidget3D(render_frame* Frame,
             geometry_buffer_allocation Allocation,
             m4 Transform, rgba8 Color)
{
    b32 Result = false;
    if (Frame->DrawWidget3DCmdCount < Frame->MaxDrawWidget3DCmdCount)
    {
        Frame->DrawWidget3DCmds[Frame->DrawWidget3DCmdCount++] = 
        {
            .Base = 
            {
                .IndexCount = Allocation.IndexBlock->Count,
                .InstanceCount = 1,
                .IndexOffset = Allocation.IndexBlock->Offset,
                .VertexOffset = Allocation.VertexBlock->Offset,
                .InstanceOffset = 0,
            },
            .Transform = Transform,
            .Color = Color,
        };
        Result = true;
    }
    return(Result);
}

inline b32 AddLight(render_frame* Frame, v3 P, v3 E, light_flags Flags)
{
    b32 Result = false;
    if (Frame->LightCount < R_MaxLightCount)
    {
        u32 LightIndex = Frame->LightCount++;
        u32 ShadowIndex = 0xFFFFFFFF;
        if (Flags & LightFlag_ShadowCaster)
        {
            if (Frame->ShadowCount < Frame->MaxShadowCount)
            {
                ShadowIndex = Frame->ShadowCount++;
                Frame->Shadows[ShadowIndex] = LightIndex;
            }
        }

        Frame->Lights[LightIndex] = 
        {
            .P = P,
            .ShadowIndex = ShadowIndex,
            .E = E,
            .Flags = Flags,
        };

        Result = true;
    }
    return(Result);
}

inline b32 DrawTriangleList2D(render_frame* Frame, u32 VertexCount, vertex_2d* VertexArray)
{
    b32 Result = false;
    if (Frame->Vertex2DCount + VertexCount <= Frame->MaxVertex2DCount)
    {
        u32 VertexOffset = Frame->Vertex2DCount;
        memcpy(Frame->Vertex2DArray + VertexOffset, VertexArray, VertexCount * sizeof(vertex_2d));
        Frame->Vertex2DCount += VertexCount;
        Result = true;
    }

    return(Result);
}

inline frustum GetClipSpaceFrustum()
{
    frustum Result = 
    {
        .Left   = { -1.0f,  0.0f,   0.0f, +1.0f },
        .Right  = { +1.0f,  0.0f,   0.0f, +1.0f },
        .Top    = {  0.0f, -1.0f,   0.0f, +1.0f },
        .Bottom = {  0.0f, +1.0f,   0.0f, +1.0f },
        .Near   = {  0.0f,  0.0f,  +1.0f,  0.0f },
        .Far    = {  0.0f,  0.0f,  -1.0f, +1.0f },
    };
    return(Result);
}

inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box)
{
    b32 Result = true;

    v3 HalfExtent = 0.5f * (Box.Max - Box.Min);
    if (Dot(HalfExtent, HalfExtent) > 1e-6f)
    {
        v3 CenterP3 = 0.5f * (Box.Min + Box.Max);
        v4 CenterP = { CenterP3.X, CenterP3.Y, CenterP3.Z, 1.0f };

        for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
        {
            f32 EffectiveRadius = 
                Abs(Frustum->Planes[PlaneIndex].X * HalfExtent.X) + 
                Abs(Frustum->Planes[PlaneIndex].Y * HalfExtent.Y) +
                Abs(Frustum->Planes[PlaneIndex].Z * HalfExtent.Z);

            if (Dot(CenterP, Frustum->Planes[PlaneIndex]) < -EffectiveRadius)
            {
                Result = false;
                break;
            }
        }
    }
    return(Result);
}

inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box, m4 Transform)
{
    b32 Result = true;

    v3 P = TransformPoint(Transform, 0.5f * (Box.Max + Box.Min));
    v3 HalfExtent = 0.5f * (Box.Max - Box.Min);
    for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
    {
        v4 Plane = Frustum->Planes[PlaneIndex];
        f32 EffectiveRadius = 
            HalfExtent.X * Abs(Dot(Plane, Transform.X)) +
            HalfExtent.Y * Abs(Dot(Plane, Transform.Y)) +
            HalfExtent.Z * Abs(Dot(Plane, Transform.Z));
        if (Dot(Plane, v4{ P.X, P.Y, P.Z, 1.0f }) < -EffectiveRadius)
        {
            Result = false;
            break;
        }
    }

    return(Result);
}

inline b32 IntersectFrustumSphere(const frustum* Frustum, v3 Center, f32 r)
{
    b32 Result = true;
    v4 P = { Center.X, Center.Y, Center.Z, 1.0f };
    for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
    {
        if (Dot(Frustum->Planes[PlaneIndex], P) <= -r)
        {
            Result = false;
            break;
        }
    }
    return(Result);
}

inline b32 IntersectFrustumFrustum(const frustum* A, const frustum* B)
{
    b32 Result = true;
    UnimplementedCodePath;
    return(Result);
}