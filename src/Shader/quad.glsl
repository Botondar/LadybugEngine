#version 460 core

#include "common.glsli"

#define Billboard_ViewAligned 0
#define Billboard_ZAligned 1

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

interpolant(0) v2 TexCoord;
interpolant(1) v4 ParticleColor;
interpolant(2) flat uint ParticleTexture;
interpolant(3) v3 ViewP;

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

layout(set = Set_User0, binding = 0, scalar) 
readonly buffer VertexBlock
{
    particle Particles[];
};

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

SetBinding(PerFrame, StructureImage) uniform texture2D StructureImage;
layout(set = Set_User0 + 1, binding = 0) uniform sampler2DArray Texture;

layout(location = 0) out v4 Target0;

void main()
{
    f32 Depth = StructureDecode(texelFetch(StructureImage, v2s(gl_FragCoord.xy), 0)).z;
    v4 SampleColor = texture(Texture, vec3(TexCoord, float(ParticleTexture)));

    f32 FarFade = clamp(2.0 * (Depth - ViewP.z), 0.0, 1.0);
    f32 NearFade = clamp(ViewP.z - (PerFrame.NearZ + 0.01), 0.0, 1.0);
    f32 Fade = min(FarFade, NearFade);
    f32 d = length(ViewP);
    f32 FogFactor = exp(-PerFrame.ConstantFogDensity * MieBetaFactor * d);
    Target0 = v4(ParticleColor.xyz * SampleColor.xyz, FogFactor * Fade * ParticleColor.w * SampleColor.w);
}
#endif