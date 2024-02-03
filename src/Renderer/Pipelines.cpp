
vertex_state InputState_vertex = 
{
    .Topology = Topology_TriangleList,
    .EnablePrimitiveRestart = false,
    // TODO(boti): We should figure out some way to not have to hard-code these.
    .BindingCount = 1,
    .AttribCount = 5,
    .Bindings = 
    {
        { sizeof(vertex), 0 },
    },
    .Attribs = 
    {
        {
            .Index = Attrib_Position,
            .Binding = 0,
            .Format = Format_R32G32B32_Float,
            .ByteOffset = OffsetOf(vertex, P),
        },
        {
            .Index = Attrib_Normal,
            .Binding = 0,
            .Format = Format_R32G32B32_Float,
            .ByteOffset = OffsetOf(vertex, N),
        },
        {
            .Index = Attrib_TangentSign,
            .Binding = 0,
            .Format = Format_R32G32B32A32_Float,
            .ByteOffset = OffsetOf(vertex, T),
        },
        {
            .Index = Attrib_TexCoord,
            .Binding = 0,
            .Format = Format_R32G32_Float,
            .ByteOffset = OffsetOf(vertex, TexCoord),
        },
        {
            .Index = Attrib_Color,
            .Binding = 0,
            .Format = Format_R8G8B8A8_UNorm,
            .ByteOffset = OffsetOf(vertex, Color),
        },
    },
};

vertex_state InputState_skinned =
{
    .Topology = Topology_TriangleList,
    .EnablePrimitiveRestart = false,
    .BindingCount = 1,
    .AttribCount = 7,
    .Bindings = { { sizeof(vertex), 0 } },
    .Attribs = 
    {
        {
            .Index = Attrib_Position,
            .Binding = 0,
            .Format = Format_R32G32B32_Float,
            .ByteOffset = OffsetOf(vertex, P),
        },
        {
            .Index = Attrib_Normal,
            .Binding = 0,
            .Format = Format_R32G32B32_Float,
            .ByteOffset = OffsetOf(vertex, N),
        },
        {
            .Index = Attrib_TangentSign,
            .Binding = 0,
            .Format = Format_R32G32B32A32_Float,
            .ByteOffset = OffsetOf(vertex, T),
        },
        {
            .Index = Attrib_TexCoord,
            .Binding = 0,
            .Format = Format_R32G32_Float,
            .ByteOffset = OffsetOf(vertex, TexCoord),
        },
        {
            .Index = Attrib_Color,
            .Binding = 0,
            .Format = Format_R8G8B8A8_UNorm,
            .ByteOffset = OffsetOf(vertex, Color),
        },
        {
            .Index = Attrib_JointWeights,
            .Binding = 0,
            .Format = Format_R32G32B32A32_Float,
            .ByteOffset = OffsetOf(vertex, Weights),
        },
        {
            .Index = Attrib_JointIndices,
            .Binding = 0,
            .Format = Format_R8G8B8A8_UInt,
            .ByteOffset = OffsetOf(vertex, Joints),
        },
    },
};


const sampler_state SamplerInfos[Sampler_Count] = 
{
    [Sampler_Default] = 
    {
        .MagFilter = Filter_Linear,
        .MinFilter = Filter_Linear,
        .MipFilter = Filter_Linear,
        .WrapU = Wrap_Repeat,
        .WrapV = Wrap_Repeat,
        .WrapW = Wrap_Repeat,
        .Anisotropy = Anisotropy_16,
        .EnableComparison = false,
        .Comparison = Compare_Always,
        .MinLOD = 0.0f,
        .MaxLOD = GlobalMaxLOD,
        .Border = Border_White,
        .EnableUnnormalizedCoordinates = false,
    },
    [Sampler_RenderTargetUnnormalized] = 
    {
        .MagFilter = Filter_Linear,
        .MinFilter = Filter_Linear,
        .MipFilter = Filter_Nearest,
        .WrapU = Wrap_ClampToEdge,
        .WrapV = Wrap_ClampToEdge,
        .WrapW = Wrap_ClampToEdge,
        .Anisotropy = Anisotropy_None,
        .EnableComparison = false,
        .Comparison = Compare_Always,
        .MinLOD = 0.0f,
        .MaxLOD = 0.0f,
        .Border = Border_Black,
        .EnableUnnormalizedCoordinates = true,
    },
    [Sampler_Shadow] = 
    {
        .MagFilter = Filter_Linear,
        .MinFilter = Filter_Linear,
        .MipFilter = Filter_Nearest,
        .WrapU = Wrap_ClampToBorder,
        .WrapV = Wrap_ClampToBorder,
        .WrapW = Wrap_ClampToBorder,
        .Anisotropy = Anisotropy_None,
        .EnableComparison = true,
        .Comparison = Compare_LessEqual,
        .MinLOD = 0.0f,
        .MaxLOD = 0.0f,
        .Border = Border_White,
        .EnableUnnormalizedCoordinates = false,
    },
    [Sampler_RenderTargetNormalized] = 
    {
        .MagFilter = Filter_Linear,
        .MinFilter = Filter_Linear,
        .MipFilter = Filter_Nearest,
        .WrapU = Wrap_ClampToBorder,
        .WrapV = Wrap_ClampToBorder,
        .WrapW = Wrap_ClampToBorder,
        .Anisotropy = Anisotropy_None,
        .EnableComparison = false,
        .Comparison = Compare_Always,
        .MinLOD = 0.0f,
        .MaxLOD = GlobalMaxLOD,
        .Border = Border_Black,
        .EnableUnnormalizedCoordinates = false,
    },

    [Sampler_RenderTargetNormalizedClampToEdge] = 
    {
        .MagFilter = Filter_Linear,
        .MinFilter = Filter_Linear,
        .MipFilter = Filter_Nearest,
        .WrapU = Wrap_ClampToEdge,
        .WrapV = Wrap_ClampToEdge,
        .WrapW = Wrap_ClampToEdge,
        .Anisotropy = Anisotropy_None,
        .EnableComparison = false,
        .Comparison = Compare_Always,
        .MinLOD = 0.0f,
        .MaxLOD = GlobalMaxLOD,
        .Border = Border_Black,
        .EnableUnnormalizedCoordinates = false,
    },

    [Sampler_NoFilter] = 
    {
        .MagFilter = Filter_Nearest,
        .MinFilter = Filter_Nearest,
        .MipFilter = Filter_Nearest,
        .WrapU = Wrap_ClampToEdge,
        .WrapV = Wrap_ClampToEdge,
        .WrapW = Wrap_ClampToEdge,
        .Anisotropy = Anisotropy_None,
        .EnableComparison = false,
        .Comparison = Compare_Always,
        .MinLOD = 0.0f,
        .MaxLOD = GlobalMaxLOD,
        .Border = Border_Black,
        .EnableUnnormalizedCoordinates = false,
    },
};


const descriptor_set_layout_info SetLayoutInfos[SetLayout_Count] = 
{
    [SetLayout_PerFrame] = 
    {
        .BindingCount = Binding_PerFrame_Count,
        .Bindings = 
        {
            [Binding_PerFrame_Constants] = 
            {
                .Binding = Binding_PerFrame_Constants,
                .Type = Descriptor_UniformBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_LightBuffer] = 
            {
                .Binding = Binding_PerFrame_LightBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_TileBuffer] = 
            {
                .Binding = Binding_PerFrame_TileBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_InstanceBuffer] = 
            {
                .Binding = Binding_PerFrame_InstanceBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_DrawBuffer] = 
            {
                .Binding = Binding_PerFrame_DrawBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_IndexBuffer] = 
            {
                .Binding = Binding_PerFrame_IndexBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_VertexBuffer] = 
            {
                .Binding = Binding_PerFrame_VertexBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_SkinnedVertexBuffer] = 
            {
                .Binding = Binding_PerFrame_SkinnedVertexBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_VisibilityImage] = 
            {
                .Binding = Binding_PerFrame_VisibilityImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_OcclusionImage] = 
            {
                .Binding = Binding_PerFrame_OcclusionImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_StructureImage] = 
            {
                .Binding = Binding_PerFrame_StructureImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_OcclusionRawStorageImage] =
            {
                .Binding = Binding_PerFrame_OcclusionRawStorageImage,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_OcclusionStorageImage] =
            {
                .Binding = Binding_PerFrame_OcclusionStorageImage,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_OcclusionRawImage] =
            {
                .Binding = Binding_PerFrame_OcclusionRawImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_HDRColorStorageImage] =
            {
                .Binding = Binding_PerFrame_HDRColorStorageImage,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_HDRColorImage] =
            {
                .Binding = Binding_PerFrame_HDRColorImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_BloomImage] =
            {
                .Binding = Binding_PerFrame_BloomImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_CascadedShadow] = 
            {
                .Binding = Binding_PerFrame_CascadedShadow,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_HDRMipStorageImages] = 
            {
                .Binding = Binding_PerFrame_HDRMipStorageImages,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = R_MaxMipCount,
                .Stages = PipelineStage_All,
                .Flags = DescriptorFlag_PartiallyBound,
            },
            [Binding_PerFrame_BloomMipStorageImages] = 
            {
                .Binding = Binding_PerFrame_BloomMipStorageImages,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = R_MaxMipCount,
                .Stages = PipelineStage_All,
                .Flags = DescriptorFlag_PartiallyBound,
            },
            [Binding_PerFrame_HDRColorImageGeneral] = 
            {
                .Binding = Binding_PerFrame_HDRColorImageGeneral,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
            [Binding_PerFrame_BloomImageGeneral] = 
            {
                .Binding = Binding_PerFrame_BloomImageGeneral,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
        },
    },

    [SetLayout_Bindless] = 
    {
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 0, // NOTE(boti): actual count is implied by the descriptor pool size for bindless
                .Stages = PipelineStage_All,
                .Flags = DescriptorFlag_Bindless,
            },
        },
    },

    [SetLayout_Samplers] = 
    {
        .BindingCount = 1,
        {
            {
                .Binding = Binding_Sampler_NamedSamplers,
                .Type = Descriptor_Sampler,
                .DescriptorCount = Sampler_Count,
                .Stages = PipelineStage_All,
            },
        },
    },

    [SetLayout_CascadeShadow] = 
    {
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_ImageSampler,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
        },
    },
    
    [SetLayout_Bloom] =
    {
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_ImageSampler,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
            },
            {
                .Binding = 1,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
            },
        },
    },

    [SetLayout_BloomUpsample] = 
    {
        .BindingCount = 3,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_ImageSampler,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
            },
            {
                .Binding = 1,
                .Type = Descriptor_ImageSampler,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
            },
            {
                .Binding = 2,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = PipelineStage_CS,
            },
        },
    },

    [SetLayout_SingleCombinedTexturePS] = 
    {
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_ImageSampler,
                .DescriptorCount = 1,
                .Stages = PipelineStage_PS,
            },
        },
    },

    [SetLayout_ParticleBuffer] = 
    {
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
        },
    },

    [SetLayout_PoseTransform] = 
    {
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_DynamicUniformBuffer,
                .DescriptorCount = 1,
                .Stages = PipelineStage_All,
            },
        },
    },
    
    [SetLayout_PointShadows] =
    {
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_ImageSampler,
                .DescriptorCount = R_MaxShadowCount,
                .Stages = PipelineStage_All,
            },
        },
    },
    
    [SetLayout_PackedSamplers] = 
    {
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = Descriptor_Sampler,
                .DescriptorCount = packed_sampler::MaxSamplerCount,
                .Stages = PipelineStage_All,
            },
        },
    },
};

const pipeline_info PipelineInfos[Pipeline_Count] = 
{
    // None pipeline
    [Pipeline_None] = {},

    [Pipeline_ShadingForward] =
    {
        .Name = "shading_forward",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 1,
            .DescriptorSets = 
            {
                SetLayout_PointShadows,
            },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_FrontCW,
            .Fill = Fill_Solid,
            .CullFlags = Cull_Back,
            .DepthBiasConstantFactor = 0.0f,
            .DepthBiasClamp = 0.0f,
            .DepthBiasSlopeFactor = 0.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable,
            .DepthCompareOp = Compare_Equal,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false }, },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_Prepass] =
    {
        .Name = "prepass",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_FrontCW,
            .Fill = Fill_Solid,
            .CullFlags = Cull_Back,
            .DepthBiasConstantFactor = 0.0f,
            .DepthBiasClamp = 0.0f,
            .DepthBiasSlopeFactor = 0.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable|DS_DepthWriteEnable,
            .DepthCompareOp = Compare_GreaterEqual,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 2,
        .BlendAttachments = { { .BlendEnable = false } },
        .ColorAttachmentCount = 2,
        .ColorAttachments = { VISIBILITY_FORMAT, STRUCTURE_BUFFER_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_Shadow] = 
    {
        .Name = "shadow",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_DepthClampEnable|RS_DepthBiasEnable,
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
            .DepthBiasConstantFactor = 2.0f / (f32)R_ShadowResolution,
            .DepthBiasClamp = 1.0f / 16.0f,
            .DepthBiasSlopeFactor = 4.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable|DS_DepthWriteEnable,
            .DepthCompareOp = Compare_LessEqual,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 0,
        //.BlendAttachments,
        .ColorAttachmentCount = 0,
        //.ColorAttachments,
        .DepthAttachment = SHADOW_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_ShadowAny] =
    {
        .Name = "shadow_any",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = InputState_vertex,
        .RasterizerState = 
        {
            .Flags = RS_DepthBiasEnable,
            .Fill = Fill_Solid,
            .CullFlags = Cull_None,
            .DepthBiasConstantFactor = 2.0f / 256.0f,
            .DepthBiasClamp = 1.0f / 24.0f,
            .DepthBiasSlopeFactor = 4.0f,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable|DS_DepthWriteEnable,
            .DepthCompareOp = Compare_LessEqual,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 0,
        //.BlendAttachments,
        .ColorAttachmentCount = 0,
        //.ColorAttachments,
        .DepthAttachment = SHADOW_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_Gizmo] =
    {
        .Name = "gizmo",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
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
            .DepthCompareOp = Compare_GreaterEqual,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false } },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { SWAPCHAIN_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_Sky] = 
    {
        .Name = "sky",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = 
        {
            .Topology = Topology_TriangleList,
            .EnablePrimitiveRestart = false,
            .BindingCount = 0,
            .AttribCount = 0,
            .Bindings = {},
            .Attribs = {},
        },
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
            .DepthCompareOp = Compare_GreaterEqual,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },

        .BlendAttachmentCount = 1,
        .BlendAttachments = { { .BlendEnable = false } },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_UI] = 
    {
        .Name = "ui",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 1,
            .DescriptorSets = { SetLayout_SingleCombinedTexturePS },
        },
        .EnabledStages = PipelineStage_VS|PipelineStage_PS,
        .InputAssemblerState = 
        {
            .Topology = Topology_TriangleList,
            .EnablePrimitiveRestart = false,
            .BindingCount = 1,
            .AttribCount = 3,
            .Bindings = { { .Stride = sizeof(vertex_2d), .InstanceStepRate = 0 } },
            .Attribs = 
            {
                {
                    .Index = 0,
                    .Binding = 0,
                    .Format = Format_R32G32_Float,
                    .ByteOffset = OffsetOf(vertex_2d, P),
                },
                {
                    .Index = 1,
                    .Binding = 0,
                    .Format = Format_R32G32_Float,
                    .ByteOffset = OffsetOf(vertex_2d, TexCoord),
                },
                {
                    .Index = 2,
                    .Binding = 0,
                    .Format = Format_R8G8B8A8_UNorm,
                    .ByteOffset = OffsetOf(vertex_2d, Color),
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
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 0.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            {
                .BlendEnable = true,
                .SrcColor = Blend_SrcAlpha,
                .DstColor = Blend_InvSrcAlpha,
                .ColorOp = BlendOp_Add,
                .SrcAlpha = Blend_One,
                .DstAlpha = Blend_Zero,
                .AlphaOp = BlendOp_Add,
            },
        },

        .ColorAttachmentCount = 1,
        .ColorAttachments = { SWAPCHAIN_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_Blit] =
    {
        .Name = "blit",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 0,
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
        .ColorAttachments = { SWAPCHAIN_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = Format_Undefined,
    },

    [Pipeline_BloomDownsample] =
    {
        .Name = "bloom_downsample",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
    },

    [Pipeline_BloomUpsample] =
    {
        .Name = "bloom_upsample",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
    },

    [Pipeline_SSAO] =
    {
        .Name = "ssao",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
    },
    [Pipeline_SSAOBlur] =
    {
        .Name = "ssao_blur",
        .Type = PipelineType_Compute,
        .Layout =
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
    },

    [Pipeline_Quad] = 
    {
        .Name = "quad",
        .Type = PipelineType_Graphics,
        .Layout = 
        {
            .DescriptorSetCount = 2,
            .DescriptorSets = 
            {
                SetLayout_ParticleBuffer,
                SetLayout_SingleCombinedTexturePS,
            },
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
            .Flags = DS_DepthTestEnable,
            .DepthCompareOp = Compare_Greater,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            { 
                .BlendEnable = true,
                .SrcColor = Blend_SrcAlpha,
                .DstColor = Blend_One,
                .ColorOp = BlendOp_Add,
                .SrcAlpha = Blend_Zero,
                .DstAlpha = Blend_One,
                .AlphaOp = BlendOp_Add,
            } 
        },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { HDR_FORMAT },
        .DepthAttachment = DEPTH_FORMAT,
        .StencilAttachment = Format_Undefined,
    },
    
    [Pipeline_Skinning] = 
    {
        .Name = "skin",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .DescriptorSetCount = 1,
            .DescriptorSets = 
            {
                SetLayout_PoseTransform,
            },
        },
        .EnabledStages = PipelineStage_CS,
    },
    [Pipeline_LightBinning] =
    {
        .Name = "light_bin",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
    },
    [Pipeline_ShadingVisibility] = 
    {
        .Name = "shading_visibility",
        .Type = PipelineType_Compute,
        .Layout = 
        {
            .DescriptorSetCount = 1,
            .DescriptorSets = 
            {
                SetLayout_PointShadows,
            },
        },
    },
};