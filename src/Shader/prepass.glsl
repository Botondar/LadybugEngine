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

interpolant(0) vec2 TexCoord;
interpolant(1) vec3 ViewP;
interpolant(2) flat uint InstanceIndex;

#if defined(VS)

invariant gl_Position;

layout(location = Attrib_Position) in vec3 aP;
layout(location = Attrib_TexCoord) in vec2 aTexCoord;

void main()
{
    instance_data Instance = Instances[gl_InstanceIndex];
    precise v3 WorldP  = TransformPoint(Instance.Transform, aP);
    ViewP = TransformPoint(PerFrame.ViewTransform, WorldP);
    TexCoord = aTexCoord;
    InstanceIndex = gl_InstanceIndex;
    gl_Position = PerFrame.ViewProjectionTransform * vec4(WorldP, 1.0);
}

#elif defined(FS)

layout(set = 0, binding = 0) uniform sampler Sampler;
layout(set = 1, binding = 0) uniform texture2D Textures[];

layout(location = 0) out v2u VisibilityOut;
layout(location = 1) out vec4 StructureOut;

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
    instance_data Instance = Instances[InstanceIndex];
    VisibilityOut = v2u(InstanceIndex, gl_PrimitiveID);
    StructureOut = StructureEncode(ViewP.z);
    vec4 Albedo = texture(sampler2D(Textures[Instance.Material.DiffuseID], Sampler), TexCoord);
    if (Albedo.a < R_AlphaTestThreshold)
    {
        discard;
    }
}

#endif