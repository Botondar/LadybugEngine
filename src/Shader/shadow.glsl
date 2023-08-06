#version 460 core

#include "common.glsli"

layout(set = 2, binding = 0) uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(push_constant) uniform PushConstants
{
    mat4 ModelTransform;
    material Material;
    uint CascadeIndex;
};

#if defined(VS)

layout(location = 0) in vec3 aP;
layout(location = 4) in vec2 aTexCoord;

layout(location = 0) out vec2 TexCoord;

void main()
{
    gl_Position = PerFrame.CascadeViewProjection * (ModelTransform * vec4(aP, 1.0));
    if (CascadeIndex > 0)
    {
        gl_Position.xyz = PerFrame.CascadeScales[CascadeIndex - 1] * gl_Position.xyz + PerFrame.CascadeOffsets[CascadeIndex - 1];
    }
    TexCoord = aTexCoord;
}

#elif defined(FS)

layout(set = 0, binding = 0) uniform sampler Sampler;
layout(set = 1, binding = 0) uniform texture2D Textures[];

layout(location = 0) in vec2 TexCoord;

void main()
{
#if 1
    if (texture(sampler2D(Textures[Material.DiffuseID], Sampler), TexCoord).a < ALPHA_TEST_THRESHOLD)
    {
        discard;
    }
#endif
}

#endif