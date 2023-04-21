
input_assembler_state InputState_vertex = 
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


const VkSamplerCreateInfo SamplerInfos[Sampler_Count] = 
{
    {},

    // Default
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 4.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    },
    // RenderTargetUnnormalized
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_TRUE,
    },
    // Shadow
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_TRUE,
        .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    },
    // RenderTargetNormalized
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    },
};


const descriptor_set_layout_info SetLayoutInfos[SetLayout_Count] = 
{
    {},

    // PerFrameUniformData
    {
        .Flags = SetLayoutFlag_UpdateAfterBind,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_ALL,
                .ImmutableSampler = Sampler_None,
            },
        },
    },
    // BindlessTexturesPS
    {
        .Flags = SetLayoutFlag_UpdateAfterBind|SetLayoutFlag_Bindless,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .DescriptorCount = 0, // NOTE(boti): actual count is implied by the descriptor pool size for bindless
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    // SampledRenderTargetPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
        },
    },

    // DefaultSamplerPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .ImmutableSampler = Sampler_Default,
            },
        },
    },

    // ShadowPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_Shadow,
            },
        },
    },

    // SampledRenderTargetNormalizedPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
        },
    },

    // StorageImage_CS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    // Bloom
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },
    // SSAO
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    //SetLayout_SSAOBlur
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 3,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 2,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },
};

// TODO(boti): Get the material visible to this portion of the code so we don't have to hard code this !!!!!
constexpr u32 MaterialStructSize = sizeof(v3) + 2 * sizeof(rgba8) + 3*sizeof(u32);

const pipeline_info PipelineInfos[Pipeline_Count] = 
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

    // Pipeline_Quad
    {
        .Name = "quad",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 0,
            .PushConstantRanges = 
            { 
                {
                    .stageFlags = VK_SHADER_STAGE_ALL,
                    .offset = 0,
                    .size = 2 * sizeof(v2) + sizeof(v3),
                },
            },
            .DescriptorSets = {},
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
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = VK_FORMAT_UNDEFINED,

    }
};