
constexpr u32 MaxColorAttachmentCount               = 8;
constexpr u32 MaxDescriptorSetCount                 = 32;
constexpr u32 MaxDescriptorSetLayoutBindingCount    = 32;
constexpr u32 MinPushConstantSize                   = 128;

struct device_luid
{
    u64 Value;
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
    Blend_Src1Color,
    Blend_InvSrc1Color,
    Blend_Src1Alpha,
    Blend_InvSrc1Alpha,

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

constexpr tex_anisotropy MaterialSamplerAnisotropy = Anisotropy_16;

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

typedef flags32 shader_stages;
enum shader_stage_bits : shader_stages
{
    ShaderStage_None = 0,

    ShaderStage_VS = (1 << 0),
    ShaderStage_PS = (1 << 1),
    ShaderStage_HS = (1 << 2),
    ShaderStage_DS = (1 << 3),
    ShaderStage_GS = (1 << 4),

    ShaderStage_CS = (1 << 5),

    ShaderStage_Count = 6, // NOTE(boti): This has to be kept up to date !!!

    ShaderStage_AllGfx = ShaderStage_VS|ShaderStage_PS|ShaderStage_DS|ShaderStage_HS|ShaderStage_GS,
    ShaderStage_All = ShaderStage_AllGfx|ShaderStage_CS,
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

struct vertex_state
{
    topology_type Topology;
    b32 EnablePrimitiveRestart;
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
    shader_stages Stages;
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

typedef flags64 pipeline_properties;
enum pipeline_property_bits : pipeline_properties
{
    PipelineProp_None       = 0,

    PipelineProp_Cull       = (1u << 0),
    PipelineProp_DepthClamp = (1u << 1),
    PipelineProp_DepthBias  = (1u << 2),
};

struct pipeline_inheritance_info
{
    u32 ParentID;
    pipeline_property_bits OverrideProps;
};

struct pipeline_info
{
    const char* Name;
    pipeline_type Type;

    pipeline_inheritance_info Inheritance;

    pipeline_layout_info Layout;

    //
    // Graphics pipeline specific
    //
    shader_stages EnabledStages;

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
    CubeLayer_PositiveX = 0,
    CubeLayer_NegativeX,
    CubeLayer_PositiveY,
    CubeLayer_NegativeY,
    CubeLayer_PositiveZ,
    CubeLayer_NegativeZ,

    CubeLayer_Count,
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

struct texture_info
{
    v3u Extent;
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
        (A.Extent.X == B.Extent.X) &&
        (A.Extent.Y == B.Extent.Y) &&
        (A.Extent.Z == B.Extent.Z) &&
        (A.MipCount == B.MipCount) &&
        (A.ArrayCount == B.ArrayCount) &&
        (A.Format == B.Format);
    return(Result);
}

constexpr material_sampler_id 
GetMaterialSamplerID(tex_wrap WrapU, tex_wrap WrapV, tex_wrap WrapW)
{
    Assert((WrapU < Wrap_Count) && (WrapV < Wrap_Count) && (WrapW < Wrap_Count));
    material_sampler_id Result = { (u32)WrapU | ((u32)WrapV << 2) | ((u32)WrapW << 4)};
    return(Result);
}

inline b32 
GetWrapFromMaterialSampler(material_sampler_id ID, tex_wrap* WrapU, tex_wrap* WrapV, tex_wrap* WrapW)
{
    b32 Result = true;
    if (ID.Value < R_MaterialSamplerCount)
    {
        constexpr u32 Mask = Wrap_Count - 1;
        constexpr u32 Shift = 2;
        *WrapU = (tex_wrap)((ID.Value >> (0*Shift)) & Mask);
        *WrapV = (tex_wrap)((ID.Value >> (1*Shift)) & Mask);
        *WrapW = (tex_wrap)((ID.Value >> (2*Shift)) & Mask);
    }
    else
    {
        Result = false;
    }
    return(Result);
}