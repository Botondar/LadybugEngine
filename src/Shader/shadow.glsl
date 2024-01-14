#version 460 core

#include "common.glsli"

layout(set = 2, binding = 0, scalar) 
uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(set = 3, binding = 0, scalar)
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

layout(set = 0, binding = 0) uniform sampler Sampler;
layout(set = 1, binding = 0) uniform texture2D Textures[];

void main()
{
#if 1
    instance_data Instance = Instances[InstanceIndex];
    if (texture(sampler2D(Textures[Instance.Material.DiffuseID], Sampler), TexCoord).a < R_AlphaTestThreshold)
    {
        discard;
    }
#endif
}

#endif