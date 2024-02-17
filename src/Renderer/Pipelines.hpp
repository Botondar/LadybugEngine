#pragma once

enum pipeline : u32
{
    Pipeline_None = 0,

    Pipeline_ShadingForward,
    Pipeline_Prepass,
    Pipeline_Shadow, // TODO(boti): This is for cascades, rename
    Pipeline_ShadowAny, // TODO(boti): This is for generic shadows, rename
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

    Pipeline_Count,
};

extern vertex_state InputState_vertex;

extern const sampler_state SamplerInfos[Sampler_Count];
extern const descriptor_set_layout_info SetLayoutInfos[Set_Count];
extern const pipeline_info PipelineInfos[Pipeline_Count];