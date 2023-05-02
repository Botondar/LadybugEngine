#pragma once

/*
TODO(boti):
    - Have our own enums for all Vulkan the enums used here
    - The UI/Blit pipelines currently use the depth buffer because they get rendered in 
      the same render-pass as the gizmos. They should probably be moved to a different render-pass.
*/

enum sampler : u32
{
    Sampler_None = 0,

    Sampler_Default,
    Sampler_RenderTargetUnnormalized,
    Sampler_Shadow,
    Sampler_RenderTargetNormalized,
    Sampler_RenderTargetNormalizedClampToEdge,

    Sampler_Count,
};

enum descriptor_set_layout : u32
{
    SetLayout_None = 0,

    SetLayout_PerFrameUniformData,
    SetLayout_BindlessTexturesPS,
    SetLayout_SampledRenderTargetPS,
    SetLayout_DefaultSamplerPS,
    SetLayout_ShadowPS,
    SetLayout_SampledRenderTargetNormalized_PS_CS,
    SetLayout_StorageImage_CS,
    SetLayout_Bloom,
    SetLayout_SSAO,
    SetLayout_SSAOBlur,
    SetLayout_Blit,
    SetLayout_BloomUpsample,
    SetLayout_SingleCombinedTexturePS,

    SetLayout_Count,
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
    Pipeline_Quad,

    Pipeline_Count,
};

extern input_assembler_state InputState_vertex;

extern const VkSamplerCreateInfo SamplerInfos[Sampler_Count];
extern const descriptor_set_layout_info SetLayoutInfos[SetLayout_Count];
extern const pipeline_info PipelineInfos[Pipeline_Count];