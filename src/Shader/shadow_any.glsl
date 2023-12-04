#version 460 core

#include "common.glsli"

layout(set = 2, binding = 0, scalar) uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(push_constant) uniform PushConstants
{
    mat4 ModelTransform;
    renderer_material Material;
    uint Index;
};

#if defined(VS)

layout(location = Attrib_Position) in vec3 aP;
layout(location = Attrib_TexCoord) in vec2 aTexCoord;

layout(location = 0) out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    uint ShadowIndex = Index / 6;
    uint LayerIndex = Index % 6;
    gl_Position = PerFrame.PointShadows[ShadowIndex].ViewProjections[LayerIndex] * (ModelTransform * vec4(aP, 1.0));
}

#elif defined(FS)

layout(set = 0, binding = 0) uniform sampler Sampler;
layout(set = 1, binding = 0) uniform texture2D Textures[];

layout(location = 0) in vec2 TexCoord;

void main()
{
#if 1
    if (texture(sampler2D(Textures[Material.DiffuseID], Sampler), TexCoord).a < R_AlphaTestThreshold)
    {
        discard;
    }
#endif
}

#endif