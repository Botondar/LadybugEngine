#pragma once

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
    SetLayout_ParticleBuffer,
    SetLayout_PoseTransform,
    SetLayout_Skinning, 

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
    Pipeline_Skinning,

    Pipeline_Count,
};

extern vertex_state InputState_vertex;

extern const sampler_state SamplerInfos[Sampler_Count];
extern const descriptor_set_layout_info SetLayoutInfos[SetLayout_Count];
extern const pipeline_info PipelineInfos[Pipeline_Count];