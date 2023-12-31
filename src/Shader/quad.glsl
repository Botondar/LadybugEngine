#version 460 core

#extension GL_EXT_scalar_block_layout : require

#include "common.glsli"

#define Billboard_ViewAligned 0
#define Billboard_ZAligned 1

layout(set = 0, binding = 0, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

#if defined(VS)
layout(push_constant) 
uniform PushConstants
{
    uint BillboardMode;
};

struct particle
{
    v3 P;
    uint TextureIndex;
    v4 Color;
    v2 HalfExtent;
};

layout(scalar, set = 1, binding = 0) 
buffer VertexBlock
{
    particle Particles[];
};

layout(location = 0) out v2 TexCoord;
layout(location = 1) out v4 ParticleColor;
layout(location = 2) out flat uint ParticleTexture;
layout(location = 3) out v3 ViewP;

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

    v3 X = PerFrame.CameraTransform[0].xyz;
    v3 Y = PerFrame.CameraTransform[1].xyz;
    if (BillboardMode == Billboard_ZAligned)
    {
        Y = v3(0.0, 0.0, 1.0);
    }
    v3 P = Particle.HalfExtent.x * BaseP.x * X + Particle.HalfExtent.y * BaseP.y * Y + Particle.P;
    P = TransformPoint(PerFrame.ViewTransform, P);

    TexCoord = 0.5 * (BaseP.xy + vec2(1.0, 1.0));
    ParticleColor = Particle.Color;
    ParticleTexture = Particle.TextureIndex;
    ViewP = P;

    if (GetLuminance(ParticleColor.rgb) > 0.0)
    {
        gl_Position = PerFrame.ProjectionTransform * vec4(P, 1.0);
    }
    else
    {
        // NOTE(boti): We're essentially doing particle culling here,
        // the idea is that when a particle is fully transparent
        // we want the rasterizer to take care of that, 
        // otherwise the GPU would choke on the fill-rate.
        gl_Position = v4(0.0, 0.0, 0.0, 0.0);
    }
}
#else

layout(set = 2, binding = 0) uniform sampler2D StructureBuffer;
layout(set = 3, binding = 0) uniform sampler2DArray Texture;

layout(location = 0) in v2 TexCoord;
layout(location = 1) in v4 ParticleColor;
layout(location = 2) in flat uint ParticleTexture;
layout(location = 3) in v3 ViewP;

layout(location = 0) out v4 Target0;

void main()
{
    f32 Depth = StructureDecode(textureLod(StructureBuffer, gl_FragCoord.xy, 0)).z;
    v4 SampleColor = texture(Texture, vec3(TexCoord, float(ParticleTexture)));

    f32 FarFade = clamp(2.0 * (Depth - ViewP.z), 0.0, 1.0);
    f32 NearFade = clamp(ViewP.z - (PerFrame.NearZ + 0.01), 0.0, 1.0);
    f32 Fade = min(FarFade, NearFade);
    f32 d = length(ViewP);
    f32 FogFactor = exp(-PerFrame.ConstantFogDensity * MieBetaFactor * d);
    Target0 = v4(ParticleColor.xyz * SampleColor.xyz, FogFactor * Fade * ParticleColor.w * SampleColor.w);
}
#endif