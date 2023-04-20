#pragma once

/*
TODO(boti):
    - Have our own enums for all Vulkan the enums used here
    - The UI/Blit pipelines currently use the depth buffer because they get rendered in 
      the same render-pass as the gizmos. They should probably be moved to a different render-pass.
*/

struct pipeline_layout_info
{
    u32 PushConstantRangeCount;
    u32 DescriptorSetCount;
    VkPushConstantRange PushConstantRanges[MaxPushConstantRangeCount];
    descriptor_set_layout DescriptorSets[MaxDescriptorSetCount];
};

enum pipeline_stage : u32
{
    PipelineStage_None = 0,

    PipelineStage_VS = (1 << 0),
    PipelineStage_PS = (1 << 1),
    PipelineStage_DS = (1 << 2),
    PipelineStage_HS = (1 << 3),
    PipelineStage_GS = (1 << 4),

    PipelineStage_CS = (1 << 5),

    GfxPipelineStage_Count = 6, // NOTE(boti): This has to be kept up to date!
};
typedef u32 pipeline_stages;

// NOTE(boti): binding index is implicit from what index this structure is at in input_assembler_state
struct vertex_input_binding
{
    u32 Stride;
    b32 IsPerInstance;
};

struct vertex_input_attrib
{
    u32 Index;
    u32 BindingIndex;
    VkFormat Format;
    u32 Offset;
};

struct input_assembler_state
{
    VkPrimitiveTopology PrimitiveTopology;
    b32 EnablePrimitiveRestart;
    u32 BindingCount;
    u32 AttribCount;
    vertex_input_binding Bindings[MaxVertexInputBindingCount];
    vertex_input_attrib Attribs[MaxVertexInputAttribCount];
};

enum rasterizer_flags : flags32
{
    RS_Flags_None = 0,

    RS_DepthClampEnable = (1 << 0),
    RS_DiscardEnable = (1 << 1),
    RS_DepthBiasEnable = (1 << 2),
};

struct rasterizer_state
{
    flags32 Flags;
    VkPolygonMode PolygonMode;
    VkCullModeFlags CullFlags;
    VkFrontFace FrontFace;

    f32 DepthBiasConstantFactor;
    f32 DepthBiasClamp;
    f32 DepthBiasSlopeFactor;
};

enum depth_stencil_flags : flags32
{
    DS_Flags_None = 0,

    DS_DepthTestEnable = (1 << 0),
    DS_DepthWriteEnable = (1 << 1),
    DS_DepthBoundsTestEnable = (1 << 2),
    DS_StencilTestEnable = (1 << 3),
};

struct depth_stencil_state
{
    flags32 Flags;

    VkCompareOp DepthCompareOp;
    VkStencilOpState StencilFront;
    VkStencilOpState StencilBack;
    f32 MinDepthBounds;
    f32 MaxDepthBounds;
};

struct blend_attachment
{
    b32 BlendEnable;
    VkBlendFactor SrcColorFactor;
    VkBlendFactor DstColorFactor;
    VkBlendOp ColorOp;
    VkBlendFactor SrcAlphaFactor;
    VkBlendFactor DstAlphaFactor;
    VkBlendOp AlphaOp;
};

enum pipeline_type : u32
{
    PipelineType_Graphics = 0,
    PipelineType_Compute,

    PipelineType_Count,
};

struct graphics_pipeline_info
{
    const char* Name;
    pipeline_type Type;

    pipeline_layout_info Layout;

    // Gfx pipeline specific
    flags32 EnabledStages;

    input_assembler_state InputAssemblerState;
    rasterizer_state RasterizerState;
    depth_stencil_state DepthStencilState;
    u32 BlendAttachmentCount;
    blend_attachment BlendAttachments[MaxColorAttachmentCount];

    u32 ColorAttachmentCount;
    VkFormat ColorAttachments[MaxColorAttachmentCount];
    VkFormat DepthAttachment;
    VkFormat StencilAttachment;
};

static input_assembler_state InputState_vertex = 
{
    .PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .EnablePrimitiveRestart = false,
    // TODO(boti): We should figure out some way to not have to hard-code these.
    //             The problem is that we can't use CountOf, since arrays are defined after the counts
    .BindingCount = 1,
    .AttribCount = 5,
    .Bindings = 
    {
        { sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX },
    },
    .Attribs = 
    {
        {
            .Index = 0,
            .BindingIndex = 0,
            .Format = VK_FORMAT_R32G32B32_SFLOAT,
            .Offset = OffsetOf(vertex, P),
        },
        {
            .Index = 1,
            .BindingIndex = 0,
            .Format = VK_FORMAT_R32G32B32_SFLOAT,
            .Offset = OffsetOf(vertex, N),
        },
        {
            .Index = 2,
            .BindingIndex = 0,
            .Format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .Offset = OffsetOf(vertex, T),
        },
        {
            .Index = 4,
            .BindingIndex = 0,
            .Format = VK_FORMAT_R32G32_SFLOAT,
            .Offset = OffsetOf(vertex, TexCoord),
        },
        {
            .Index = 5,
            .BindingIndex = 0,
            .Format = VK_FORMAT_R8G8B8A8_UNORM,
            .Offset = OffsetOf(vertex, Color),
        },
    },
};

enum pipeline : u32
{
    Pipeline_None = 0,

    Pipeline_Simple,
    Pipeline_Prepass,
    Pipeline_Shadow,
    Pipeline_Gizmo,
    Pipeline_Sky,
    Pipeline_UI,
    Pipeline_Blit,
    Pipeline_BloomDownsample,
    Pipeline_BloomUpsample,
    Pipeline_SSAO,
    Pipeline_SSAOBlur,

    Pipeline_Count,
};

// TODO(boti): Get the material visible to this portion of the code so we don't have to hard code this !!!!!
constexpr u32 MaterialStructSize = sizeof(v3) + 2 * sizeof(rgba8) + 3*sizeof(u32);

static const graphics_pipeline_info PipelineInfos[Pipeline_Count] = 
{
    // None pipeline
    {},

    // Simple
    {
        .Name = "shader",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 6,
            .PushConstantRanges = 
            {
                { 
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 
                    .offset = 0, 
                    .size = sizeof(m4) + MaterialStructSize 
                },
            },
            .DescriptorSets = 
            {
                SetLayout_DefaultSamplerPS, // Texture sampler
                SetLayout_BindlessTexturesPS, // Textures
                SetLayout_PerFrameUniformData,
                SetLayout_SampledRenderTargetPS, // Occlusion buffer
                SetLayout_SampledRenderTargetPS, // Structure buffer
                SetLayout_ShadowPS, // Shadow map
            },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .PolygonMode = VK_POLYGON_MODE_FILL,
            .CullFlags = VK_CULL_MODE_NONE, // TODO
            .FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .DepthBiasConstantFactor = 0.0f,
            .DepthBiasClamp = 0.0f,
            .DepthBiasSlopeFactor = 0.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable,
            .DepthCompareOp = VK_COMPARE_OP_EQUAL,
            .StencilFront = {},
            .StencilBack = {},
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false }, },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // Prepass
    {
        .Name = "prepass",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 3,
            .PushConstantRanges = 
            {
                { 
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 
                    .offset = 0, 
                    .size = sizeof(m4) + MaterialStructSize 
                },
            },
            .DescriptorSets = 
            {
                SetLayout_DefaultSamplerPS,
                SetLayout_BindlessTexturesPS,
                SetLayout_PerFrameUniformData,
            },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .PolygonMode = VK_POLYGON_MODE_FILL,
            .CullFlags = VK_CULL_MODE_NONE, // TODO
            .FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .DepthBiasConstantFactor = 0.0f,
            .DepthBiasClamp = 0.0f,
            .DepthBiasSlopeFactor = 0.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable|DS_DepthWriteEnable,
            .DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .StencilFront = {},
            .StencilBack = {},
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false } },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { STRUCTURE_BUFFER_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // Shadow
    {
        .Name = "shadow",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 3,
            .PushConstantRanges = 
            {
                { 
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 
                    .offset = 0, 
                    .size = sizeof(m4) + MaterialStructSize + sizeof(u32) 
                },
            },
            .DescriptorSets = 
            {
                SetLayout_DefaultSamplerPS,
                SetLayout_BindlessTexturesPS,
                SetLayout_PerFrameUniformData,
            },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_DepthClampEnable|RS_DepthBiasEnable,
            .PolygonMode = VK_POLYGON_MODE_FILL,
            .CullFlags = VK_CULL_MODE_NONE,
            .FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .DepthBiasConstantFactor = 0.0f, // 1.0f / 4096.0f,
            .DepthBiasClamp = 1.0f / 128.0f,
            .DepthBiasSlopeFactor = 3.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable|DS_DepthWriteEnable,
            .DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .StencilFront = {},
            .StencilBack = {},
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 0,
        //.BlendAttachments,
        .ColorAttachmentCount = 0,
        //.ColorAttachments,
        .DepthAttachment = SHADOW_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // Gizmo
    {
        .Name = "gizmo",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 1,
            .PushConstantRanges = 
            {
                { 
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 
                    .offset = 0, 
                    .size = sizeof(m4) + sizeof(u32) },
            },
            .DescriptorSets = { SetLayout_PerFrameUniformData },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .PolygonMode = VK_POLYGON_MODE_FILL,
            .CullFlags = VK_CULL_MODE_NONE,
            .FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .DepthBiasConstantFactor = 0.0f,
            .DepthBiasClamp = 0.0f,
            .DepthBiasSlopeFactor = 0.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable|DS_DepthWriteEnable,
            .DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .StencilFront = {},
            .StencilBack = {},
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 0.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false } },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { SWAPCHAIN_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // Sky
    {
        .Name = "sky",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 0,
            .DescriptorSetCount = 1,
            .PushConstantRanges = {},
            .DescriptorSets = { SetLayout_PerFrameUniformData },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .PolygonMode = VK_POLYGON_MODE_FILL,
            .CullFlags = VK_CULL_MODE_NONE,
            .FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .DepthBiasConstantFactor = 0.0f,
            .DepthBiasClamp = 0.0f,
            .DepthBiasSlopeFactor = 0.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable,
            .DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .StencilFront = {},
            .StencilBack = {},
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },

        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false } },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // UI
    {
        .Name = "ui",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 1,
            .PushConstantRanges = 
            { 
                { 
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0, 
                    .size = sizeof(m4) 
                },
            },
            .DescriptorSets = { SetLayout_SampledRenderTargetNormalized_PS_CS }, // TODO(boti): probably we should rename this
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = 
        {
            .PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .EnablePrimitiveRestart = false,
            .BindingCount = 1,
            .AttribCount = 3,
            .Bindings = { { .Stride = sizeof(ui_vertex), .IsPerInstance = false } },
            .Attribs = 
            {
                {
                    .Index = 0,
                    .BindingIndex = 0,
                    .Format = VK_FORMAT_R32G32_SFLOAT,
                    .Offset = OffsetOf(ui_vertex, P),
                },
                {
                    .Index = 1,
                    .BindingIndex = 0,
                    .Format = VK_FORMAT_R32G32_SFLOAT,
                    .Offset = OffsetOf(ui_vertex, TexCoord),
                },
                {
                    .Index = 2,
                    .BindingIndex = 0,
                    .Format = VK_FORMAT_R8G8B8A8_UNORM,
                    .Offset = OffsetOf(ui_vertex, Color),
                },
            },
        },
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .PolygonMode = VK_POLYGON_MODE_FILL,
            .CullFlags = VK_CULL_MODE_NONE,
            .FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        },
        .DepthStencilState = 
        {
            .Flags = DS_Flags_None,
            .StencilFront = {},
            .StencilBack = {},
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 0.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .BlendEnable = true,
                .SrcColorFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .DstColorFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .ColorOp = VK_BLEND_OP_ADD,
                .SrcAlphaFactor = VK_BLEND_FACTOR_ONE,
                .DstAlphaFactor = VK_BLEND_FACTOR_ZERO,
                .AlphaOp = VK_BLEND_OP_ADD,
            },
        },

        .ColorAttachmentCount = 1,
        .ColorAttachments = { SWAPCHAIN_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // Blit
    {
        .Name = "blit",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 0,
            .DescriptorSetCount = 1,
            .PushConstantRanges = {},
            .DescriptorSets = { SetLayout_SampledRenderTargetNormalized_PS_CS },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = 
        {
            .PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .EnablePrimitiveRestart = false,
            .BindingCount = 0,
            .AttribCount = 0,
        },
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .PolygonMode = VK_POLYGON_MODE_FILL,
            .CullFlags = VK_CULL_MODE_NONE,
            .FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        },
        .DepthStencilState = 
        {
            .Flags = DS_Flags_None,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false } },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { SWAPCHAIN_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // Bloom downsample
    {
        .Name = "downsample_bloom",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 1,
            .PushConstantRanges = 
            {
                {
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .offset = 0,
                    .size = sizeof(b32),
                },
            },
            .DescriptorSets = { SetLayout_Bloom },
        },
    },

    // Pipeline_BloomUpsample
    {
        .Name = "upsample_bloom",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .PushConstantRangeCount = 0,
            .DescriptorSetCount = 1,
            .PushConstantRanges = { },
            .DescriptorSets = { SetLayout_Bloom },
        },
    },

    // Pipeline_SSAO
    {
        .Name = "ssao",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .PushConstantRangeCount = 0,
            .DescriptorSetCount = 2,
            .PushConstantRanges = {},
            .DescriptorSets = 
            {
                SetLayout_SSAO,
                SetLayout_PerFrameUniformData,
            },
        },
    },
    // Pipeline_SSAOBlur
    {
        .Name = "ssao_blur",
        .Type = PipelineType_Compute,
        .Layout =
        {
            .PushConstantRangeCount = 0,
            .DescriptorSetCount = 1,
            .PushConstantRanges = {},
            .DescriptorSets = { SetLayout_SSAOBlur },
        },
    },
};