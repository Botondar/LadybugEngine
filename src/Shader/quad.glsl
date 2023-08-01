#version 460 core

#extension GL_EXT_scalar_block_layout : require

#include "common.glsli"

layout(set = 0, binding = 0) 
uniform PerFrameBlock
{
    per_frame PerFrame;
};

#if defined(VS)

struct particle
{
    v3 P;
    v2 HalfExtent;
    v3 Color;
};

layout(scalar, set = 1, binding = 0) 
buffer VertexBlock
{
    particle Particles[];
};

layout(location = 0) out v2 TexCoord;
layout(location = 1) out v3 ParticleColor;

void main()
{
    v3 BaseVertices[4] = 
    {
        vec3(-1.0, +1.0, 0.0),
        vec3(+1.0, +1.0, 0.0),
        vec3(-1.0, -1.0, 0.0),
        vec3(+1.0, -1.0, 0.0),
    };
    uint IndexData[6] = 
    {
        0, 1, 2,
        1, 3, 2,
    };

    uint GlobalIndex = gl_VertexIndex / 6;
    uint LocalIndex = gl_VertexIndex % 6;

    v3 BaseP = BaseVertices[IndexData[LocalIndex]];
    particle Particle = Particles[GlobalIndex];

    v3 P = v3(Particle.HalfExtent.xy, 1.0) * BaseP + TransformPoint(PerFrame.View, Particle.P);

    TexCoord = 0.5 * (BaseP.xy + vec2(1.0, 1.0));
    ParticleColor = Particle.Color;
    gl_Position = PerFrame.Projection * vec4(P, 1.0);
}
#else

layout(set = 2, binding = 0) uniform sampler2DArray Texture;

layout(location = 0) in v2 TexCoord;
layout(location = 1) in v3 ParticleColor;

layout(location = 0) out v4 Target0;

void main()
{
    v4 SampleColor = texture(Texture, vec3(TexCoord, 0));
    
    Target0 = vec4(SampleColor.rgb * ParticleColor.rgb, SampleColor.a);
}
#endif