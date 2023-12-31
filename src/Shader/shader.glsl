#version 460

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

layout(location = Attrib_Position)      in vec3 aP;
layout(location = Attrib_Normal)        in vec3 aN;
layout(location = Attrib_TangentSign)   in vec4 aT;
layout(location = Attrib_TexCoord)      in vec2 aTexCoord;
layout(location = Attrib_Color)         in vec4 aColor;

layout(location = 0) out vec3 P;
layout(location = 1) out vec2 TexCoord;
layout(location = 2) out vec3 N;
layout(location = 3) out vec3 T;
layout(location = 4) out vec3 B;
layout(location = 5) out vec3 ShadowP;
layout(location = 6) out vec3 CascadeBlends;

void main()
{
    vec3 WorldP = TransformPoint(ModelTransform, aP);
    TexCoord = aTexCoord;
    P = TransformPoint(PerFrame.ViewTransform, WorldP);
    N = normalize(mat3(PerFrame.ViewTransform) * (aN * inverse(mat3(ModelTransform))));
    T = normalize((mat3(PerFrame.ViewTransform) * (mat3(ModelTransform) * aT.xyz)));
    B = normalize(cross(T, N)) * aT.w;

    ShadowP = TransformPoint(PerFrame.CascadeViewProjections[0], WorldP);
    ShadowP.xy = 0.5 * ShadowP.xy + v2(0.5);
    CascadeBlends = GetCascadeBlends(PerFrame.CascadeMinDistances, PerFrame.CascadeMaxDistances, P.z);

    gl_Position = PerFrame.ViewProjectionTransform * v4(WorldP, 1.0);
}

#elif defined(FS)

layout(set = 0, binding = 0) uniform sampler Sampler;
layout(set = 1, binding = 0) uniform texture2D Textures[];
layout(set = 3, binding = 0) uniform sampler2D OcclusionBuffer;
layout(set = 4, binding = 0) uniform sampler2D StructureBuffer;
layout(set = 5, binding = 0) uniform sampler2DArrayShadow ShadowSampler;

layout(set = 6, binding = 0, scalar) 
readonly buffer LightBuffer
{
    light Lights[];
};
layout(set = 7, binding = 0, scalar)
readonly buffer TileBuffer
{
    screen_tile Tiles[];
};

layout(set = 8, binding = 0) uniform samplerCubeShadow PointShadows[];

layout(location = 0) in vec3 P;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 InN;
layout(location = 3) in vec3 InT;
layout(location = 4) in vec3 InB;
layout(location = 5) in vec3 ShadowP;
layout(location = 6) in vec3 ShadowBlends;

layout(location = 0) out vec4 Out0;

float CalculateShadow(vec3 CascadeCoord0, in vec3 CascadeBlends)
{
    vec3 CascadeCoord1 = PerFrame.CascadeScales[0] * CascadeCoord0 + PerFrame.CascadeOffsets[0];
    vec3 CascadeCoord2 = PerFrame.CascadeScales[1] * CascadeCoord0 + PerFrame.CascadeOffsets[1];
    vec3 CascadeCoord3 = PerFrame.CascadeScales[2] * CascadeCoord0 + PerFrame.CascadeOffsets[2];

    vec2 ShadowMapSize = vec2(textureSize(ShadowSampler, 0).xy);
    vec2 TexelSize = 1.0 / ShadowMapSize;

    bool BeyondCascade2 = (CascadeBlends.y >= 0.0);
    bool BeyondCascade3 = (CascadeBlends.z >= 0.0);

    v2 ShadowP1 = (BeyondCascade2) ? CascadeCoord2.xy : CascadeCoord0.xy;
    v2 ShadowP2 = (BeyondCascade3) ? CascadeCoord3.xy : CascadeCoord1.xy;
    float Depth1 = (BeyondCascade2) ? CascadeCoord2.z : CascadeCoord0.z;
    float Depth2 = (BeyondCascade3) ? clamp(CascadeCoord3.z, 0.0, 1.0) : CascadeCoord1.z;

    v3 Blends = clamp(CascadeBlends, v3(0.0), v3(1.0));
    float Blend = (BeyondCascade2) ? Blends.y - Blends.z : 1.0 - Blends.x;

    v4 P1 = v4(ShadowP1.xy, 2.0 * float(BeyondCascade2),       Depth1);
    v4 P2 = v4(ShadowP2.xy, 2.0 * float(BeyondCascade3) + 1.0, Depth2);

    float Shadow1 = 0.0;
    float Shadow2 = 0.0;

#if 1
    float Scale = 4.0 * TexelSize.x;
    Shadow1 = SampleShadowPoisson(ShadowSampler, P1, Scale, 64);
    Shadow2 = SampleShadowPoisson(ShadowSampler, P2, Scale, 64);
#else
    v2 Jitter = v2(1.0 * (3.0/16.0), 3.0 * (3.0 / 16.0)) * TexelSize.x;
    Shadow1 = SampleShadowJitter4(ShadowSampler, P1, Jitter);
    Shadow2 = SampleShadowJitter4(ShadowSampler, P2, Jitter);
#endif

    float Result = mix(Shadow2, Shadow1, Blend);
    return(Result);
}

void main()
{
    v3 Lo = vec3(0.0);
    v3 V = normalize(-P);

    uint TileX = uint(gl_FragCoord.x) / R_TileSizeX;
    uint TileY = uint(gl_FragCoord.y) / R_TileSizeY;
    uint TileIndex = TileX + TileY * PerFrame.TileCount.x;

    {
        vec4 BaseColor = UnpackRGBA8(Material.DiffuseColor);
        vec4 BaseMetallicRoughness = UnpackRGBA8(Material.BaseMaterial);
        vec4 Albedo = BaseColor * texture(sampler2D(Textures[Material.DiffuseID], Sampler), TexCoord);
        {
            float ScreenSpaceOcclusion = textureLod(OcclusionBuffer, gl_FragCoord.xy, 0).r;
            vec3 Ambient = PerFrame.Ambience * Albedo.rgb * ScreenSpaceOcclusion;
            Lo += Ambient + Material.Emissive;
        }

        vec3 Normal;
        Normal.xy = texture(sampler2D(Textures[Material.NormalID], Sampler), TexCoord).xy;
        Normal.xy = 2.0 * Normal.xy - vec2(1.0);
        Normal.z = sqrt(1.0 - dot(Normal.xy, Normal.xy));
        vec4 MetallicRoughness = texture(sampler2D(Textures[Material.MetallicRoughnessID], Sampler), TexCoord);
        float Roughness = MetallicRoughness.g * BaseMetallicRoughness.g;
        float Metallic = MetallicRoughness.b * BaseMetallicRoughness.b;

        mat3 TBN = mat3(
            normalize(InT),
            normalize(InB),
            normalize(InN));
        vec3 N = TBN * Normal;

        vec3 F0 = mix(vec3(0.04), Albedo.rgb, Metallic);
        vec3 DiffuseBase = (1.0 - Metallic) * (vec3(1.0) - F0) * Albedo.rgb;

        float SunShadow = CalculateShadow(ShadowP, ShadowBlends);
        Lo += CalculateOutgoingLuminance(SunShadow * PerFrame.SunL, PerFrame.SunV, N, V,
                                         DiffuseBase, F0, Roughness);

        for (uint i = 0; i < Tiles[TileIndex].LightCount; i++)
        {
            uint LightIndex = Tiles[TileIndex].LightIndices[i];
            v3 dP = Lights[LightIndex].P.xyz - P;
            v3 E = Lights[LightIndex].E;
            float DistSq = dot(dP, dP);
            float InvDistSq = 1.0 / DistSq;
            v3 L = dP * sqrt(InvDistSq);
            E = E * InvDistSq;

            // NOTE(boti): When there are a lot of light sources close together,
            // their cumulative effect can be seen on quite far away texels from the source,
            // which produces "blocky" artefacts when only certain portions of a screen tile
            // pass the binning check.
            // To counteract this, we set the luminance of those lights to 0,
            // but a better way in the future would probably be to have an attenuation
            // that actually falls to 0.
            if (GetLuminance(E) < R_LuminanceThreshold)
            {
                E = vec3(0.0);
            }

            uint ShadowIndex = Lights[LightIndex].ShadowIndex;
            float Shadow = 1.0;
            if (ShadowIndex != 0xFFFFFFFFu)
            {
                v3 SampleP = TransformDirection(PerFrame.CameraTransform, -dP);
                v3 AbsP = abs(SampleP);
                f32 MaxXY = max(AbsP.x, AbsP.y);
                f32 Depth = max(MaxXY, AbsP.z);

                ivec2 ShadowResolution = textureSize(PointShadows[ShadowIndex], 0);
                f32 TexelSize = 1.0 / float(ShadowResolution.x);
                f32 Offset = 2.0 * TexelSize;

                f32 n = PerFrame.PointShadows[ShadowIndex].Near;
                f32 f = PerFrame.PointShadows[ShadowIndex].Far;
                f32 r = 1.0 / (f - n);
                f32 ProjDepth = f*r - f*n*r / Depth;
                Shadow = texture(PointShadows[ShadowIndex], v4(SampleP, ProjDepth));

                f32 dXY = (MaxXY > AbsP.z) ? Offset : 0.0;
                f32 dX = (AbsP.x > AbsP.y) ? dXY : 0.0;
                v2 OffsetXY = v2(Offset - dX, dX);
                v2 OffsetYZ = v2(Offset - dXY, dXY);

                // TODO(boti): Something's wrong here, we're getting seams
                // at the edge of the cube faces
                v3 Limit = v3(Depth);
                Limit.xy -= OffsetXY * (1.0 / 512.0);
                Limit.yz -= OffsetYZ * (1.0 / 512.0);

                SampleP.xy -= OffsetXY;
                SampleP.yz -= OffsetYZ;
                Shadow += texture(PointShadows[ShadowIndex], v4(clamp(SampleP, -Limit, +Limit), ProjDepth));
                SampleP.xy += 2.0 * OffsetXY;
                Shadow += texture(PointShadows[ShadowIndex], v4(clamp(SampleP, -Limit, +Limit), ProjDepth));
                SampleP.yz += 2.0 * OffsetYZ;
                Shadow += texture(PointShadows[ShadowIndex], v4(clamp(SampleP, -Limit, +Limit), ProjDepth));
                SampleP.xy -= 2.0 * OffsetXY;
                Shadow += texture(PointShadows[ShadowIndex], v4(clamp(SampleP, -Limit, +Limit), ProjDepth));
                Shadow = 0.2 * Shadow;

#if 0
                // NOTE(boti): This is the code path for volumetric point lights
                float MaxDistance = 6.0;
                int VolCount = 16;
                float W = MaxDistance * (1.0 / float(VolCount));
                float di = AtmosphereNoise(0.5 * gl_FragCoord.xy);
                float g = 0.76;
                float g2 = g*g;

                float VolLo = 0.0;
                for (int i = 0; i < VolCount; i++)
                {
                    float t = MaxDistance * ((i + di) / float(VolCount));
                    v3 CurrentP = -V * t;
                    if (CurrentP.z >= P.z - 1e-2)
                    {
                        break;
                    }

                    v3 VolL = Lights[LightIndex].P.xyz - CurrentP;
                    v3 VolP = TransformDirection(PerFrame.CameraTransform, -VolL);
                    float VolDepth = max(max(abs(VolP.x), abs(VolP.y)), abs(VolP.z));
                    VolDepth = f*r - f*n*r / VolDepth;
                    float VolShadow = texture(PointShadows[ShadowIndex], v4(VolP, VolDepth));

                    float Attenuation = clamp(1.0 / sqrt(dot(VolL, VolL)), 0.0, 1.0);

                    VolL = normalize(VolL);
                    float Phase = PhaseMie(g, g2, dot(VolL, -V));
                    VolLo += 0.20 * VolShadow * Attenuation * Phase * W;
                }

                Lo += VolLo * Lights[LightIndex].E.xyz * Lights[LightIndex].E.w;
#endif
            }

            Lo += Shadow * CalculateOutgoingLuminance(Shadow * E, L, N, V,
                                                      DiffuseBase, F0, Roughness);
        }
    }

#if 0
    {
        uint StepCount = 64;
        f32 InvStepCount = 1.0 / float(StepCount);
        f32 MaxDistance = length(P);
        f32 InvMaxDistance = 1.0 / MaxDistance;
        f32 StepDistance = MaxDistance / float(StepCount);

        f32 MieExtinction = (1.0 / 0.9) * MieBetaFactor;
        f32 MieAbsorption = MieExtinction - MieBetaFactor;

        f32 CosSun = dot(V, PerFrame.SunV);
        const f32 g = 0.0;
        const f32 g2 = g*g;
        f32 PhaseIn = PhaseMie(g, g2, CosSun);
        f32 PhaseOut = max(1.0 - PhaseMie(g, g2, 1.0), 0.0);

        v3 UpDirection = PerFrame.ViewTransform[2].xyz;

        uint SunCascadeIndex = 0;
        m4 CameraToSun = PerFrame.CascadeViewProjections[0] * PerFrame.CameraTransform;


        Lo = Lo * exp(-PerFrame.ConstantFogDensity * MieBetaFactor * 1.111111 * MaxDistance);
        for (uint Step = 0; Step < StepCount; Step++)
        {
            f32 t = (float(Step) + 0.5) * InvStepCount;
            v3 CurrentP = P * t;

            f32 Height = dot(CurrentP, UpDirection);

            while ((SunCascadeIndex < 3) && (CurrentP.z > PerFrame.CascadeMaxDistances[SunCascadeIndex]))
            {
                SunCascadeIndex++;
            }

            vec3 SunShadowP = TransformPoint(CameraToSun, CurrentP);
            vec4 SunShadowCoord = vec4(0.5 * SunShadowP.xy + vec2(0.5), float(SunCascadeIndex), SunShadowP.z);
            if (SunCascadeIndex > 0)
            {
                // FIXME
                SunShadowCoord.xyw = PerFrame.CascadeScales[SunCascadeIndex - 1] * SunShadowCoord.xyw + PerFrame.CascadeOffsets[SunCascadeIndex - 1];
            }
            float SunShadow = texture(ShadowSampler, SunShadowCoord);

            f32 CurrentHeight = TransformPoint(PerFrame.CameraTransform, CurrentP).z;
            f32 tFog = clamp(Ratio1(CurrentHeight - PerFrame.LinearFogMinZ, PerFrame.LinearFogMaxZ - PerFrame.LinearFogMinZ), 0.0, 1.0);
            f32 Density = (1.0 - tFog) * PerFrame.LinearFogDensityAtBottom * MieBetaFactor;
            
            f32 Extinction = PhaseOut * exp(Density * StepDistance * t);
            f32 InScatterAmbience = 1.0 - exp(-Density * StepDistance);
            f32 InScatterSun = PhaseIn * (1.0 - exp(-Density * StepDistance));

            Lo += Extinction * (InScatterAmbience * PerFrame.Ambience + 0.0 * InScatterSun * SunShadow * PerFrame.SunL);
        }
    }
#endif
    Out0 = vec4(Lo, 1.0);
    //f32 LightOccupancy = float(Tiles[TileIndex].LightCount) / float( R_MaxLightCountPerTile);
    //Out0 = vec4(LightOccupancy.rrr, 1.0);
}

#endif