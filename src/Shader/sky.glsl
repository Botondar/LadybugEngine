#version 460 core

#include "common.glsli"

layout(set = 0, binding = 0, scalar) uniform PerFrameBlock
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

layout(set = 1, binding = 0) uniform sampler2DArrayShadow ShadowSampler;

layout(location = 0) out vec4 Out0;

vec3 CIEClearSky(vec3 V, bool DoSun)
{
    vec3 Up = PerFrame.ViewTransform[2].xyz;//Viewvec3(0.0, 0.0, 1.0)

    vec3 SkyV = dot(V, Up) >= 0.0 ? V : reflect(V, Up);
    float CosGamma = dot(V, PerFrame.SunV);
    float CosS = max(dot(PerFrame.SunV, Up), 0.0);
    float CosTheta = dot(SkyV, Up);

    float Gamma = acos(CosGamma);
    float S = acos(CosS);
    float SinGamma = sqrt(1.0 - CosGamma*CosGamma);

    float Numerator = (1.0 - exp(-0.32 / CosTheta)) * (0.91 + 10.0 * exp(-3.0 * Gamma) + 0.45 * CosGamma * CosGamma);
    float Denominator = 0.2738509 * (0.91 + 10.0 * exp(-3.0 * S) + 0.45 * CosS * CosS);

    float Luminance = Numerator / max(Denominator, 1e-4);

    float SunLuminance = GetLuminance(PerFrame.SunL);

    vec3 SkyColor = 0.1 * SunLuminance * vec3(0.2, 0.5, 1.0);
    float SunWidth = 0.015;

    vec3 Result = max(SkyColor * Luminance, vec3(0.0));

    if (DoSun)
    {
        float SunGamma = acos(dot(V, PerFrame.SunV));
        Result = mix(PerFrame.SunL, Result, clamp(abs(SunGamma) / SunWidth, 0.0, 1.0));
    }

    return Result;
}

vec3 TraceAtmosphere(vec3 V)
{
    const float EarthR = 6371.0e3;
    const float AtmosphereR = 6371.0e3 + 40.0e3;
    const float Hr = 7994.0;
    const float Hm = 1200.0;
    const vec3 BetaR0 = vec3(3.8e-6, 13.5e-6, 33.1e-6);
    const vec3 BetaM0 = vec3(21e-6);

    const uint PrimarySampleCount = 8;
    const uint SecondarySampleCount = 8;

    
    for (uint PrimarySampleIndex = 0; PrimarySampleIndex < PrimarySampleCount; PrimarySampleIndex++)
    {

    }

    vec3 Result = CIEClearSky(V, true);
    return Result;
}

void main()
{
    vec3 V = normalize(vec3(2.0 * UV - vec2(1.0), 1.0) * vec3(PerFrame.AspectRatio / PerFrame.FocalLength, 1.0 / PerFrame.FocalLength, 1.0));
    //V = V * mat3(PerFrame.View);
#if 1
    vec3 Lo = CIEClearSky(V, false);
#else
    vec3 Lo = TraceAtmosphere(V);
#endif
    // NOTE(boti): We assume the fog dissipates at some distance...
    Lo = ApplyFog(Lo, PerFrame.Ambience, PerFrame.ConstantFogDensity, 1000.0f);
    Lo += (0.9 * MieBetaFactor) * PerFrame.SunL * CalculateAtmosphere(PerFrame.FarZ * V, PerFrame, ShadowSampler);
    Out0 = vec4(Lo, 1.0);
}

#endif