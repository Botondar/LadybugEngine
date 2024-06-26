#pragma once

enum pipeline : u32
{
    Pipeline_None = 0,

    Pipeline_ShadingForward,
    Pipeline_Prepass,
    Pipeline_ShadowCascade,
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
    Pipeline_LightBinning,
    Pipeline_ShadingVisibility,
    Pipeline_EnvironmentBRDF,

    Pipeline_Prepass_AlphaTest,
    Pipeline_ShadingForward_AlphaTest,
    Pipeline_Shadow_AlphaTest,
    Pipeline_ShadowCascade_AlphaTest,
    Pipeline_ShadingForward_Transparent,

    Pipeline_Count,
};

extern const sampler_state SamplerInfos[Sampler_Count];
extern const descriptor_set_layout_info SetLayoutInfos[Set_Count];
extern const pipeline_info PipelineInfos[Pipeline_Count];