#version 460

#include "common.glsli"

#ifdef VS

layout(location = 0) in vec3 aP;
layout(location = 2) in vec3 aColor;

layout(location = 0) out vec3 Color;

void main()
{
    gl_Position = vec4(aP, 1.0);
    Color = aColor;
}

#else

layout(location = 0) in vec3 Color;

layout(location = 0) out vec4 Target0;

void main()
{
    Target0 = vec4(Color, 1.0);
}

#endif