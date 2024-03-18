#version 460 core

#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

SetBindingLayout(Static, InstanceBuffer, scalar)
buffer InstanceBuffer
{
    instance_data Instances[];
};

layout(push_constant) uniform PushConstants
{
    uint Index;
};

interpolant(0) vec2 TexCoord;
interpolant(1) flat uint InstanceIndex;

#if defined(VS)

layout(location = Attrib_Position) in vec3 aP;
layout(location = Attrib_TexCoord) in vec2 aTexCoord;

void main()
{
    instance_data Instance = Instances[gl_InstanceIndex];
    TexCoord = aTexCoord;
    InstanceIndex = gl_InstanceIndex;
    uint ShadowIndex = Index / 6;
    uint LayerIndex = Index % 6;
    m4 ViewProjection = PerFrame.PointShadows[ShadowIndex].ViewProjections[LayerIndex];
    gl_Position = ViewProjection * v4(TransformPoint(Instance.Transform, aP), 1.0);
}

#elif defined(FS)

SetBinding(Sampler, MaterialSamplers) uniform sampler MatSamplers[R_MaterialSamplerCount];
SetBinding(Bindless, Textures) uniform texture2D Textures[];

void main()
{
#if 1
    instance_data Instance = Instances[InstanceIndex];
    if (texture(sampler2D(Textures[Instance.Material.DiffuseID], MatSamplers[Instance.Material.DiffuseSamplerID]), TexCoord).a < Instance.Material.AlphaThreshold)
    {
        discard;
    }
#endif
}

#endif