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
        .MaxLOD = R_MaxLOD,
        .Border = Border_White,
        .EnableUnnormalizedCoordinates = false,
    },
    [Sampler_LinearUnnormalized] = 
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
    [Sampler_LinearBlackBorder] = 
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
        .MaxLOD = R_MaxLOD,
        .Border = Border_Black,
        .EnableUnnormalizedCoordinates = false,
    },

    [Sampler_LinearEdgeClamp] = 
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
        .MaxLOD = R_MaxLOD,
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
        .MaxLOD = R_MaxLOD,
        .Border = Border_Black,
        .EnableUnnormalizedCoordinates = false,
    },
};

const descriptor_set_layout_info SetLayoutInfos[Set_Count] = 
{
    [Set_Static] = 
    {
        .BindingCount = Binding_Static_Count,
        .Bindings = 
        {   
            // Shadow
            [Binding_Static_CascadedShadow] = 
            {
                .Binding = Binding_Static_CascadedShadow,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_PointShadows] =
            {
                .Binding = Binding_Static_PointShadows,
                .Type = Descriptor_ImageSampler,
                .DescriptorCount = R_MaxShadowCount,
                .Stages = ShaderStage_All,
                .Flags = DescriptorFlag_PartiallyBound,
            },

            // Mip feedback
            [Binding_Static_MipFeedbackBuffer] =
            {
                .Binding = Binding_Static_MipFeedbackBuffer,
                .Type = Descriptor_StorageBuffer,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
                .Flags = 0,
            },

            // Render targets
            [Binding_Static_VisibilityImage] = 
            {
                .Binding = Binding_Static_VisibilityImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_OcclusionImage] = 
            {
                .Binding = Binding_Static_OcclusionImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_StructureImage] = 
            {
                .Binding = Binding_Static_StructureImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_OcclusionRawStorageImage] =
            {
                .Binding = Binding_Static_OcclusionRawStorageImage,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_OcclusionStorageImage] =
            {
                .Binding = Binding_Static_OcclusionStorageImage,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_OcclusionRawImage] =
            {
                .Binding = Binding_Static_OcclusionRawImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_HDRColorImage] =
            {
                .Binding = Binding_Static_HDRColorImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_BloomImage] =
            {
                .Binding = Binding_Static_BloomImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_HDRMipStorageImages] = 
            {
                .Binding = Binding_Static_HDRMipStorageImages,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = R_MaxMipCount,
                .Stages = ShaderStage_All,
                .Flags = DescriptorFlag_PartiallyBound,
            },
            [Binding_Static_BloomMipStorageImages] = 
            {
                .Binding = Binding_Static_BloomMipStorageImages,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = R_MaxMipCount,
                .Stages = ShaderStage_All,
                .Flags = DescriptorFlag_PartiallyBound,
            },
            [Binding_Static_HDRColorImageGeneral] =
            {
                .Binding = Binding_Static_HDRColorImageGeneral,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_BloomImageGeneral] =
            {
                .Binding = Binding_Static_BloomImageGeneral,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_TransparentImage] =
            {
                .Binding = Binding_Static_TransparentImage,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },

            // Misc
            [Binding_Static_SkyTexture] =
            {
                .Binding = Binding_Static_SkyTexture,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_SkyImage] =
            {
                .Binding = Binding_Static_SkyImage,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_BRDFLutImage] =
            {
                .Binding = Binding_Static_BRDFLutImage,
                .Type = Descriptor_StorageImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_Static_BRDFLutTexture] =
            {
                .Binding = Binding_Static_BRDFLutTexture,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
        },
    },

    [Set_Sampler] = 
    {
        .BindingCount = Binding_Sampler_Count,
        {
            {
                .Binding = Binding_Sampler_NamedSamplers,
                .Type = Descriptor_Sampler,
                .DescriptorCount = Sampler_Count,
                .Stages = ShaderStage_All,
            },
            {
                .Binding = Binding_Sampler_MaterialSamplers,
                .Type = Descriptor_Sampler,
                .DescriptorCount = R_MaterialSamplerCount,
                .Stages = ShaderStage_All,
            },
        },
    },

    [Set_Bindless] = 
    {
        .BindingCount = Binding_Bindless_Count,
        .Bindings = 
        {
            {
                .Binding = Binding_Bindless_Textures,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 0, // NOTE(boti): actual count is implied by the descriptor pool size for bindless
                .Stages = ShaderStage_All,
                .Flags = DescriptorFlag_Bindless,
            },
        },
    },

    [Set_PerFrame] = 
    {
        .BindingCount = Binding_PerFrame_Count,
        .Bindings = 
        {
            [Binding_PerFrame_Constants] = 
            {
                .Binding = Binding_PerFrame_Constants,
                .Type = Descriptor_UniformBuffer,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
            },
            [Binding_PerFrame_ParticleTexture] =
            {
                .Binding = Binding_PerFrame_ParticleTexture,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
                .Flags = 0,
            },
            [Binding_PerFrame_TextureUI] =
            {
                .Binding = Binding_PerFrame_TextureUI,
                .Type = Descriptor_SampledImage,
                .DescriptorCount = 1,
                .Stages = ShaderStage_All,
                .Flags = 0,
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
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
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
        .ColorAttachments = { RTFormat_HDR },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_Prepass] =
    {
        .Name = "prepass",
        .Type = PipelineType_Graphics,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
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
        .ColorAttachments = { RTFormat_Visibility, RTFormat_Structure },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_ShadowCascade] = 
    {
        .Name = "shadow",
        .Type = PipelineType_Graphics,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
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
        .DepthAttachment = RTFormat_Shadow,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_Shadow] =
    {
        .Name = "shadow",
        .Type = PipelineType_Graphics,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
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
        .DepthAttachment = RTFormat_Shadow,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_Gizmo] =
    {
        .Name = "gizmo",
        .Type = PipelineType_Graphics,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
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
        .ColorAttachments = { RTFormat_Swapchain },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_Sky] = 
    {
        .Name = "sky",
        .Type = PipelineType_Graphics,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
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
        .ColorAttachments = { RTFormat_HDR },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_UI] = 
    {
        .Name = "ui",
        .Type = PipelineType_Graphics,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
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
        .ColorAttachments = { RTFormat_Swapchain },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_Blit] =
    {
        .Name = "blit",
        .Type = PipelineType_Graphics,
        .Inheritance = 
        {
            .ParentID = Pipeline_None,
            .OverrideProps = PipelineProp_None,
        },
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = 
        {
            .Topology = Topology_TriangleList,
            .EnablePrimitiveRestart = false,
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
        .ColorAttachments = { RTFormat_Swapchain },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },

    [Pipeline_BloomDownsample] =
    {
        .Name = "bloom_downsample",
        .Type = PipelineType_Compute,
        .Inheritance = 
        {
            .ParentID = Pipeline_None,
            .OverrideProps = PipelineProp_None,
        },
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
        .Inheritance = 
        {
            .ParentID = Pipeline_None,
            .OverrideProps = PipelineProp_None,
        },
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
        .Inheritance = 
        {
            .ParentID = Pipeline_None,
            .OverrideProps = PipelineProp_None,
        },
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
        .Inheritance = 
        {
            .ParentID = Pipeline_None,
            .OverrideProps = PipelineProp_None,
        },
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
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = 
        {
            .Topology = Topology_TriangleList,
            .EnablePrimitiveRestart = false,
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
        .ColorAttachments = { RTFormat_HDR },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },
    
    [Pipeline_Skinning] = 
    {
        .Name = "skin",
        .Type = PipelineType_Compute,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_CS,
    },
    [Pipeline_LightBinning] =
    {
        .Name = "light_bin",
        .Type = PipelineType_Compute,
        .Inheritance = 
        {
            .ParentID = Pipeline_None,
            .OverrideProps = PipelineProp_None,
        },
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
        .Inheritance = 
        {
            .ParentID = Pipeline_None,
            .OverrideProps = PipelineProp_None,
        },
        .Layout = 
        {
            .DescriptorSetCount = 0,
            .DescriptorSets = {},
        },
    },

    [Pipeline_EnvironmentBRDF] =
    {
        .Name = "environment_brdf",
        .Type = PipelineType_Compute,
        .Inheritance = {},
        .Layout = {},
    },

    [Pipeline_Prepass_AlphaTest] =
    {
        .Name = "prepass_alphatest",
        .Type = PipelineType_Graphics,
        .Inheritance =
        {
            .ParentID = Pipeline_Prepass,
            .OverrideProps = PipelineProp_Cull,
        },
        .RasterizerState =
        {
            .CullFlags = Cull_None,
        },
    },

    [Pipeline_ShadingForward_AlphaTest] =
    {
        .Name = "shading_forward",
        .Type = PipelineType_Graphics,
        .Inheritance = 
        {
            .ParentID = Pipeline_ShadingForward,
            .OverrideProps = PipelineProp_Cull,
        },
        .RasterizerState =
        {
            .CullFlags = Cull_None,
        },
    },

    [Pipeline_Shadow_AlphaTest] = 
    {
        .Name = "shadow_alphatest",
        .Type = PipelineType_Graphics,
        .Inheritance = 
        {
            .ParentID = Pipeline_Shadow,
            .OverrideProps = PipelineProp_None,
        },
    },

    [Pipeline_ShadowCascade_AlphaTest] = 
    {
        .Name = "shadow_alphatest",
        .Type = PipelineType_Graphics,
        .Inheritance =
        {
            .ParentID = Pipeline_ShadowCascade,
            .OverrideProps = PipelineProp_None,
        },
    },

    [Pipeline_ShadingForward_Transparent] =
    {
        .Name = "shading_transparent",
        .Type = PipelineType_Graphics,
        .Inheritance = {},
        .Layout = {},
        .EnabledStages = ShaderStage_VS|ShaderStage_PS,
        .InputAssemblerState = { .Topology = Topology_TriangleList },
        .RasterizerState = 
        {
            .Flags = RS_FrontCW,
            .Fill = Fill_Solid,
            .CullFlags = Cull_Back,
        },
        .DepthStencilState = 
        {
            .Flags = DS_DepthTestEnable,
            .DepthCompareOp = Compare_GreaterEqual,
            .MinDepthBounds = 0.0f,
            .MaxDepthBounds = 1.0f,
        },
        .BlendAttachmentCount = 1,
        .BlendAttachments = 
        {
            { 
                .BlendEnable = true,
                .SrcColor = Blend_SrcAlpha,
                .DstColor = Blend_Src1Color,
                .ColorOp = BlendOp_Add,
                .SrcAlpha = Blend_Zero,
                .DstAlpha = Blend_One,
                .AlphaOp = BlendOp_Add,
            }
        },
        .ColorAttachmentCount = 1,
        .ColorAttachments = { RTFormat_HDR },
        .DepthAttachment = RTFormat_Depth,
        .StencilAttachment = RTFormat_Undefined,
    },
};