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
};

#if defined(VS)

invariant gl_Position;

layout(location = 0) in vec3 aP;
layout(location = 4) in vec2 aTexCoord;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 ViewP;

void main()
{
    v3 WorldP  = TransformPoint(ModelTransform, aP);
    ViewP = TransformPoint(PerFrame.View, WorldP);
    TexCoord = aTexCoord;
    gl_Position = PerFrame.ViewProjection * vec4(WorldP, 1.0);
}

#elif defined(FS)

layout(set = 0, binding = 0) uniform sampler Sampler;
layout(set = 1, binding = 0) uniform texture2D Textures[];

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 ViewP;

layout(location = 0) out vec4 StructureOut;

vec4 StructureEncode(in float z)
{
    float h = uintBitsToFloat(floatBitsToUint(z) & 0xFFFFE000u);
    float dx = dFdxFine(z);
    float dy = dFdyFine(z);
    vec4 Result = vec4(dx, dy, h, z - h);
    return Result;
}

void main()
{
    StructureOut = StructureEncode(ViewP.z);
    vec4 Albedo = texture(sampler2D(Textures[Material.DiffuseID], Sampler), TexCoord);
    if (Albedo.a < R_AlphaTestThreshold)
    {
        discard;
    }
}

#endif