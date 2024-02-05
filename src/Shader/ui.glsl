#version 460

#include "common.glsli"

interpolant(0) vec2 TexCoord;
interpolant(1) vec4 Color;

#ifdef VS
layout(push_constant) uniform PushConstants
{
    mat4 Transform;
};

layout(location = 0) in vec2 aP;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

void main()
{
    gl_Position = Transform * vec4(aP, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
}
#else

SetBinding(PerFrame, TextureUI) uniform texture2D Texture;
SetBinding(Sampler, NamedSamplers) uniform sampler Samplers[Sampler_Count];

layout(location = 0) out vec4 Out0;

void main()
{
    vec4 Sample = texture(sampler2D(Texture, Samplers[Sampler_LinearEdgeClamp]), TexCoord);
    Out0 = Color * Sample;
}

#endif