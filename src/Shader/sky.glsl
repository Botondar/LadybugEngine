#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

interpolant(0) vec2 UV;

#if defined(VS)

const vec2 VertexData[] = 
{
    vec2(-1.0, -1.0),
    vec2(+3.0, -1.0),
    vec2(-1.0, +3.0),
};

void main()
{
    vec4 P = vec4(VertexData[gl_VertexIndex], 0.0, 1.0);
    UV = 0.5 * P.xy + vec2(0.5);
    gl_Position = P;
}

#elif defined(FS)

SetBinding(Static, CascadedShadow) uniform texture2DArray CascadedShadow;
SetBinding(Sampler, NamedSamplers) uniform sampler Samplers[Sampler_Count];

layout(location = 0) out vec4 Out0;

void main()
{
    vec3 V = normalize(vec3(2.0 * UV - vec2(1.0), 1.0) * vec3(PerFrame.AspectRatio / PerFrame.FocalLength, 1.0 / PerFrame.FocalLength, 1.0));

    vec3 Lo = CIEClearSky(V, PerFrame.ViewTransform[2].xyz, PerFrame.SunV, PerFrame.SunL, false);

    // NOTE(boti): We assume the fog dissipates at some distance...
    Lo = ApplyFog(Lo, PerFrame.Ambience, PerFrame.ConstantFogDensity, 1000.0f);
    f32 Atmosphere = CalculateAtmosphere(
        gl_FragCoord.xy,
        PerFrame.FarZ * V,
        PerFrame.SunV,
        PerFrame.CascadeViewProjections[0] * PerFrame.CameraTransform,
        PerFrame.CascadeScales,
        PerFrame.CascadeOffsets,
        PerFrame.CascadeMaxDistances,
        CascadedShadow,
        Samplers[Sampler_Shadow]);
    Lo += AtmosphereIntensity * Atmosphere * PerFrame.SunL;
    Out0 = vec4(Lo, 1.0);
}

#endif