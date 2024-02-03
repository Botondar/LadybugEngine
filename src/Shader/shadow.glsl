#version 460 core

#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

SetBindingLayout(PerFrame, InstanceBuffer, scalar)
buffer InstanceBuffer
{
    instance_data Instances[];
};

layout(push_constant) uniform PushConstants
{
    uint CascadeIndex;
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
    gl_Position = PerFrame.CascadeViewProjections[CascadeIndex] * (Instance.Transform * vec4(aP, 1.0));
}

#elif defined(FS)

SetBinding(Sampler, NamedSamplers) uniform sampler Samplers[Sampler_Count];
SetBinding(Bindless, Textures) uniform texture2D Textures[];

void main()
{
#if 1
    instance_data Instance = Instances[InstanceIndex];
    if (texture(sampler2D(Textures[Instance.Material.DiffuseID], Samplers[Sampler_Default]), TexCoord).a < R_AlphaTestThreshold)
    {
        discard;
    }
#endif
}

#endif