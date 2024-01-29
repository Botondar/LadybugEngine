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

layout(set = 0, binding = 0) uniform sampler2D Sampler;

layout(location = 0) out vec4 Out0;

void main()
{
    vec4 Sample = texture(Sampler, TexCoord);
    Out0 = Color * Sample;
}

#endif