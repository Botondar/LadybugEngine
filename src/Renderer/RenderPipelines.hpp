#pragma once

/*
TODO(boti): The goal here is to make this header backend agnostic in the future:
    - Have our own enums for all Vulkan the enums used here
*/

// NOTE(boti): these constants probably shouldn't be here
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
constexpr VkFormat HDR_FORMAT = VK_FORMAT_B10G11R11_UFLOAT_PACK32; //VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat STRUCTURE_BUFFER_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat SSAO_FORMAT = VK_FORMAT_R8_UNORM;
constexpr VkFormat SHADOW_FORMAT = VK_FORMAT_D16_UNORM; // TODO(boti): is this enough?
// HACK(boti): The swapchain format is unknown at compile time, so we use this special value to refer to it
constexpr VkFormat SWAPCHAIN_FORMAT = (VkFormat)0xFFFFFFFF;

constexpr u32 MaxVertexInputBindingCount = 16;
constexpr u32 MaxVertexInputAttribCount = 32;
constexpr u32 MaxColorAttachmentCount = 8;

enum graphics_pipeline_stage : u32
{
    GfxPipelineStage_None = 0,

    GfxPipelineStage_VS = (1 << 0),
    GfxPipelineStage_PS = (1 << 1),
    GfxPipelineStage_DS = (1 << 2),
    GfxPIpelineStage_HS = (1 << 3),
    GfxPipelineStage_GS = (1 << 4),

    GfxPipelineStage_Count = 5, // NOTE(boti): This has to be kept up to date!
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

struct graphics_pipeline_info
{
    const char* Name;
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

enum graphics_pipeline_type : u32
{
    Pipeline_None = 0,

    Pipeline_Simple,
    Pipeline_Prepass,
    Pipeline_Shadow,
    Pipeline_Gizmo,
    Pipeline_Sky,
    Pipeline_UI,
    Pipeline_Blit,

    Pipeline_Count,
};

static const graphics_pipeline_info PipelineInfos[Pipeline_Count] = 
{
    // None pipeline
    {},

    // Simple
    {
        .Name = "shader",
        .EnabledStages = GfxPipelineStage_VS|GfxPipelineStage_PS,
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
        .EnabledStages = GfxPipelineStage_VS|GfxPipelineStage_PS,
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
        .EnabledStages = GfxPipelineStage_VS|GfxPipelineStage_PS,
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
        .EnabledStages = GfxPipelineStage_VS|GfxPipelineStage_PS,
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
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    },

    // Sky
    {
        .Name = "sky",
        .EnabledStages = GfxPipelineStage_VS|GfxPipelineStage_PS,
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
        .EnabledStages = GfxPipelineStage_VS|GfxPipelineStage_PS,
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
        .EnabledStages = GfxPipelineStage_VS|GfxPipelineStage_PS,
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
};



/*
typedef struct VkGraphicsPipelineCreateInfo {
    VkStructureType                                  sType;
    const void*                                      pNext;
    VkPipelineCreateFlags                            flags;
    uint32_t                                         stageCount;
    const VkPipelineShaderStageCreateInfo*           pStages;
    const VkPipelineVertexInputStateCreateInfo*      pVertexInputState;
    const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
    const VkPipelineTessellationStateCreateInfo*     pTessellationState; // can be null
    const VkPipelineViewportStateCreateInfo*         pViewportState;
    const VkPipelineRasterizationStateCreateInfo*    pRasterizationState;
    const VkPipelineMultisampleStateCreateInfo*      pMultisampleState;
    const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;
    const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;
    const VkPipelineDynamicStateCreateInfo*          pDynamicState;
    VkPipelineLayout                                 layout;
    VkRenderPass                                     renderPass;
    uint32_t                                         subpass;
    VkPipeline                                       basePipelineHandle;
    int32_t                                          basePipelineIndex;
} VkGraphicsPipelineCreateInfo;
*/

/*
 Pipeline helper struct so that we don't have to live in pointer hell when pre-defining pipelines
 NOTE(boti):
 - Stages is missing. When creating a pipeline, the stages should be passed in separately
 - Tessellation state is missing. It may be added in the future if we want to support it.
 - Viewport state is missing. We're always using dynamic viewports so the render pass create helper can take care of that.
 - Multisample state is missing. We don't plan on using multisampling for AA.
 - Blend state is specified directly through the attachments. In the future we may want to expose logic ops.
 - Dynamic state is missing. We may want to expose it, but for now it always creates dynamic viewport + scissor states.
*/
struct render_pipeline_info
{
    VkPipelineVertexInputStateCreateInfo VertexInputState;
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
    VkPipelineRasterizationStateCreateInfo RasterizationState;
    VkPipelineDepthStencilStateCreateInfo DepthStencilState;
    u32 BlendAttachmentCount;
    VkPipelineColorBlendAttachmentState BlendAttachments[8];
};

struct render_pass_info
{
    static constexpr u32 MaxColorAttachmentCount = 8;
    u32 ColorAttachmentCount;
    VkFormat ColorAttachments[MaxColorAttachmentCount];
    VkFormat DepthAttachment;
    VkFormat StencilAttachment;
};


// Use this render pipeline as a reference
namespace DefaultRenderPipeline
{
    static const render_pipeline_info Info = 
    {
        .VertexInputState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        },
        .InputAssemblyState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        },
        .RasterizationState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 0.0f,
        },
        .DepthStencilState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_ALWAYS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .back = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}

namespace SimpleRenderPipeline
{
    static const render_pass_info RenderPassInfo = 
    {
        .ColorAttachmentCount = 1,
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    };

    static const VkVertexInputBindingDescription Bindings[] = 
    {
        {
            .binding = 0,
            .stride = sizeof(vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    static const VkVertexInputAttributeDescription Attribs[] = 
    {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = OffsetOf(vertex, P),
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = OffsetOf(vertex, N),
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = OffsetOf(vertex, T),
        },
        {
            .location = 4,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = OffsetOf(vertex, TexCoord),
        },
        {
            .location = 5,
            .binding = 0,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .offset = OffsetOf(vertex, Color),
        },
    };

    static const render_pipeline_info Info = 
    {
        .VertexInputState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = CountOf(Bindings),
            .pVertexBindingDescriptions = Bindings,
            .vertexAttributeDescriptionCount = CountOf(Attribs),
            .pVertexAttributeDescriptions = Attribs,
        },
        .InputAssemblyState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        },
        .RasterizationState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,//VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 0.0f,
        },
        .DepthStencilState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_EQUAL,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .back = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}

namespace PrepassRenderPipeline
{
    static const render_pass_info RenderPass
    {
        .ColorAttachmentCount = 1,
        .ColorAttachments = { STRUCTURE_BUFFER_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    };

    static const render_pipeline_info Info = 
    {
        .VertexInputState = SimpleRenderPipeline::Info.VertexInputState,
        .InputAssemblyState = SimpleRenderPipeline::Info.InputAssemblyState,
        .RasterizationState = SimpleRenderPipeline::Info.RasterizationState,
        .DepthStencilState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .back = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}


namespace ShadowRenderPipeline
{
    static const render_pass_info RenderPass
    {
        .ColorAttachmentCount = 0,
        .ColorAttachments = { VK_FORMAT_UNDEFINED },
        .DepthAttachment = SHADOW_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    };

    static const render_pipeline_info Info = 
    {
        .VertexInputState = SimpleRenderPipeline::Info.VertexInputState,
        .InputAssemblyState = SimpleRenderPipeline::Info.InputAssemblyState,
        .RasterizationState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_TRUE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_TRUE,
            .depthBiasConstantFactor = 0.0f,//1.0f / 4096.0f,
            .depthBiasClamp = 1.0f / 128.0f,
            .depthBiasSlopeFactor = 3.0f,
            .lineWidth = 0.0f,
        },
        .DepthStencilState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .back = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 0,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}

namespace GizmoRenderPipeline
{
    static const render_pipeline_info Info = 
    {
        .VertexInputState = SimpleRenderPipeline::Info.VertexInputState,
        .InputAssemblyState = SimpleRenderPipeline::Info.InputAssemblyState,
        .RasterizationState = SimpleRenderPipeline::Info.RasterizationState,
        .DepthStencilState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .back = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}

namespace SkyRenderPipeline
{
    static const render_pass_info RenderPassInfo = 
    {
        .ColorAttachmentCount = 1,
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,
    };

    static const render_pipeline_info Info = 
    {
        .VertexInputState = DefaultRenderPipeline::Info.VertexInputState,
        .InputAssemblyState = DefaultRenderPipeline::Info.InputAssemblyState,
        .RasterizationState = DefaultRenderPipeline::Info.RasterizationState,
        .DepthStencilState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .back = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}

namespace UIRenderPipeline
{
    static const VkVertexInputBindingDescription Bindings[] = 
    {
        {
            .binding = 0,
            .stride = sizeof(ui_vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    static const VkVertexInputAttributeDescription Attribs[] = 
    {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = OffsetOf(ui_vertex, P),
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = OffsetOf(ui_vertex, TexCoord),
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .offset = OffsetOf(ui_vertex, Color),
        },
    };

    static const render_pipeline_info Info = 
    {
        .VertexInputState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = CountOf(Bindings),
            .pVertexBindingDescriptions = Bindings,
            .vertexAttributeDescriptionCount = CountOf(Attribs),
            .pVertexAttributeDescriptions = Attribs,
        },
        .InputAssemblyState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        },
        .RasterizationState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 0.0f,
        },
        .DepthStencilState = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_ALWAYS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .back = 
            {
                .failOp = VK_STENCIL_OP_KEEP,
                .passOp = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .compareMask = 0,
                .writeMask = 0,
                .reference = 0,
            },
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}

namespace BlitRenderPipeline
{
    static const render_pipeline_info Info = 
    {
        .VertexInputState = DefaultRenderPipeline::Info.VertexInputState,
        .InputAssemblyState = DefaultRenderPipeline::Info.InputAssemblyState,
        .RasterizationState = DefaultRenderPipeline::Info.RasterizationState,
        .DepthStencilState = DefaultRenderPipeline::Info.DepthStencilState,
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 
                    VK_COLOR_COMPONENT_R_BIT|
                    VK_COLOR_COMPONENT_G_BIT|
                    VK_COLOR_COMPONENT_B_BIT|
                    VK_COLOR_COMPONENT_A_BIT,
            },
        },
    };
}
