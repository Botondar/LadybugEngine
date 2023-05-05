
vertex_state InputState_vertex = 
{
    .Topology = Topology_TriangleList,
    .EnablePrimitiveRestart = false,
    // TODO(boti): We should figure out some way to not have to hard-code these.
    //             The problem is that we can't use CountOf, since arrays are defined after the counts
    .BindingCount = 1,
    .AttribCount = 5,
    .Bindings = 
    {
        { sizeof(vertex), 0 },
    },
    .Attribs = 
    {
        {
            .Index = 0,
            .Binding = 0,
            .Format = Format_R32G32B32_Float,
            .ByteOffset = OffsetOf(vertex, P),
        },
        {
            .Index = 1,
            .Binding = 0,
            .Format = Format_R32G32B32_Float,
            .ByteOffset = OffsetOf(vertex, N),
        },
        {
            .Index = 2,
            .Binding = 0,
            .Format = Format_R32G32B32A32_Float,
            .ByteOffset = OffsetOf(vertex, T),
        },
        {
            .Index = 4,
            .Binding = 0,
            .Format = Format_R32G32_Float,
            .ByteOffset = OffsetOf(vertex, TexCoord),
        },
        {
            .Index = 5,
            .Binding = 0,
            .Format = Format_R8G8B8A8_UNorm,
            .ByteOffset = OffsetOf(vertex, Color),
        },
    },
};


const VkSamplerCreateInfo SamplerInfos[Sampler_Count] = 
{
    [Sampler_None] = {},

    [Sampler_Default] = 
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
        .maxAnisotropy = 16.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    },
    [Sampler_RenderTargetUnnormalized] = 
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
    [Sampler_Shadow] = 
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
    [Sampler_RenderTargetNormalized] = 
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

    [Sampler_RenderTargetNormalizedClampToEdge] = 
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
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    },
};


const descriptor_set_layout_info SetLayoutInfos[SetLayout_Count] = 
{
    [SetLayout_None] = {},

    [SetLayout_PerFrameUniformData] = 
    {
        .Flags = SetLayoutFlag_UpdateAfterBind,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
                .ImmutableSampler = Sampler_None,
            },
        },
    },
    [SetLayout_BindlessTexturesPS] = 
    {
        .Flags = SetLayoutFlag_UpdateAfterBind|SetLayoutFlag_Bindless,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .DescriptorCount = 0, // NOTE(boti): actual count is implied by the descriptor pool size for bindless
                .Stages = PipelineStage_PS,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    [SetLayout_SampledRenderTargetPS] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
        },
    },

    [SetLayout_DefaultSamplerPS] =
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS,
                .ImmutableSampler = Sampler_Default,
            },
        },
    },

    [SetLayout_ShadowPS] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS|PipelineStage_CS,
                .ImmutableSampler = Sampler_Shadow,
            },
        },
    },

    [SetLayout_SampledRenderTargetNormalized_PS_CS] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS|PipelineStage_CS,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
        },
    },

    [SetLayout_StorageImage_CS] =
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    [SetLayout_Bloom] =
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_RenderTargetNormalizedClampToEdge,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_None,
            },
        },
    },
    [SetLayout_SSAO] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    [SetLayout_SSAOBlur] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 3,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 2,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    [SetLayout_Blit] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS,
                .ImmutableSampler = Sampler_RenderTargetNormalizedClampToEdge,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS,
                .ImmutableSampler = Sampler_RenderTargetNormalizedClampToEdge,
            },
        },
    },

    [SetLayout_BloomUpsample] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 3,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
            {
                .Binding = 2,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    [SetLayout_SingleCombinedTexturePS] = 
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS,
                .ImmutableSampler = Sampler_Default,
            },
        },
    },
};

// TODO(boti): Get the material visible to this portion of the code so we don't have to hard code this !!!!!
constexpr u32 MaterialStructSize = sizeof(v3) + 2 * sizeof(rgba8) + 3*sizeof(u32);

const pipeline_info PipelineInfos[Pipeline_Count] = 
{
    // None pipeline
    [Pipeline_None] = {},

    [Pipeline_Simple] =
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
                    .Stages = PipelineStage_VS|PipelineStage_PS, 
                    .Size = sizeof(m4) + MaterialStructSize,
                    .Offset = 0, 
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
            .Fill = Fill_Solid,
            .CullFlags = Cull_None, // TODO
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

    [Pipeline_Prepass] =
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
                    .Stages = PipelineStage_VS|PipelineStage_PS, 
                    .Size = sizeof(m4) + MaterialStructSize,
                    .Offset = 0, 
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
            .Fill = Fill_Solid,
            .CullFlags = Cull_None, // TODO
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

    [Pipeline_Shadow] = 
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
                    .Stages = PipelineStage_VS|PipelineStage_PS, 
                    .Size = sizeof(m4) + MaterialStructSize + sizeof(u32),
                    .Offset = 0, 
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
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
            .DepthBiasConstantFactor = 1.0f / 4096.0f,
            .DepthBiasClamp = 1.0f / 64.0f,
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

    [Pipeline_Gizmo] =
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
                    .Stages = PipelineStage_VS|PipelineStage_PS, 
                    .Size = sizeof(m4) + sizeof(u32),
                    .Offset = 0, 
                },
            },
            .DescriptorSets = { SetLayout_PerFrameUniformData },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
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

    [Pipeline_Sky] = 
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
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
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

    [Pipeline_UI] = 
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
                    .Stages = PipelineStage_VS,
                    .Size = sizeof(m4),
                    .Offset = 0, 
                },
            },
            .DescriptorSets = { SetLayout_SingleCombinedTexturePS },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = 
        {
            .Topology = Topology_TriangleList,
            .EnablePrimitiveRestart = false,
            .BindingCount = 1,
            .AttribCount = 3,
            .Bindings = { { .Stride = sizeof(ui_vertex), .InstanceStepRate = 0 } },
            .Attribs = 
            {
                {
                    .Index = 0,
                    .Binding = 0,
                    .Format = Format_R32G32_Float,
                    .ByteOffset = OffsetOf(ui_vertex, P),
                },
                {
                    .Index = 1,
                    .Binding = 0,
                    .Format = Format_R32G32_Float,
                    .ByteOffset = OffsetOf(ui_vertex, TexCoord),
                },
                {
                    .Index = 2,
                    .Binding = 0,
                    .Format = Format_R8G8B8A8_UNorm,
                    .ByteOffset = OffsetOf(ui_vertex, Color),
                },
            },
        },
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
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

    [Pipeline_Blit] =
    {
        .Name = "blit",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 1,
            .PushConstantRanges = 
            {
                {
                    .Stages = PipelineStage_PS,
                    .Size = sizeof(f32),
                    .Offset = 0,
                },
            },
            .DescriptorSets = { SetLayout_Blit },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = 
        {
            .Topology = Topology_TriangleList,
            .EnablePrimitiveRestart = false,
            .BindingCount = 0,
            .AttribCount = 0,
        },
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
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

    [Pipeline_BloomDownsample] =
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
                    .Stages = PipelineStage_CS,
                    .Size = sizeof(b32),
                    .Offset = 0,
                },
            },
            .DescriptorSets = { SetLayout_Bloom },
        },
    },

    [Pipeline_BloomUpsample] =
    {
        .Name = "upsample_bloom",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 1,
            .PushConstantRanges = 
            { 
                {
                    .Stages = PipelineStage_CS,
                    .Size = 2 * sizeof(f32),
                    .Offset = 0,
                }
            },
            .DescriptorSets = { SetLayout_BloomUpsample },
        },
    },

    [Pipeline_SSAO] =
    {
        .Name = "ssao",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .PushConstantRangeCount = 1,
            .DescriptorSetCount = 2,
            .PushConstantRanges = 
            {
                {
                    .Stages = PipelineStage_CS,
                    .Size = 3 * sizeof(f32),
                    .Offset = 0,
                },
            },
            .DescriptorSets = 
            {
                SetLayout_SSAO,
                SetLayout_PerFrameUniformData,
            },
        },
    },
    [Pipeline_SSAOBlur] =
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

    [Pipeline_Quad] = 
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
                    .Stages = PipelineStage_All,
                    .Size = 2 * sizeof(v2) + sizeof(v3),
                    .Offset = 0,
                },
            },
            .DescriptorSets = {},
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = 
        {
            .Topology = Topology_TriangleList,
            .EnablePrimitiveRestart = false,
            .BindingCount = 0,
            .AttribCount = 0,
        },
        .RasterizerState = 
        {
            .Flags = RS_Flags_None,
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
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
    },
};