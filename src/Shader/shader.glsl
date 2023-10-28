#version 460

#include "common.glsli"

layout(set = 2, binding = 0, scalar) uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(push_constant) uniform PushConstants
{
    mat4 ModelTransform;
    material Material;
};

#if defined(VS)

invariant gl_Position;

layout(location = 0) in vec3 aP;
layout(location = 1) in vec3 aN;
layout(location = 2) in vec4 aT;
layout(location = 4) in vec2 aTexCoord;
layout(location = 5) in vec4 aColor;

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
    P = TransformPoint(PerFrame.View, WorldP);
    N = normalize(mat3(PerFrame.View) * (aN * inverse(mat3(ModelTransform))));
    T = normalize((mat3(PerFrame.View) * (mat3(ModelTransform) * aT.xyz)));
    B = normalize(cross(T, N)) * aT.w;

    ShadowP = TransformPoint(PerFrame.CascadeViewProjections[0], WorldP);
    ShadowP.xy = 0.5 * ShadowP.xy + v2(0.5);
    CascadeBlends = GetCascadeBlends(PerFrame.CascadeMinDistances, PerFrame.CascadeMaxDistances, P.z);

    gl_Position = PerFrame.ViewProjection * v4(WorldP, 1.0);
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

    float Scale = 4.0 * TexelSize.x;
#if 1
    // Poisson filter
    const v2 PoissonOffsets[64] = v2[]
    (
        v2(-0.5119625000, -0.4827938),
        v2(-0.2171264000, -0.4768726),
        v2(-0.7552931000, -0.2426507),
        v2(-0.7136765000, -0.4496614),
        v2(-0.5938849000, -0.6895654),
        v2(-0.3148003000, -0.7047654),
        v2(-0.4221500000, -0.2024607),
        v2(-0.9466816000, -0.2014508),
        v2(-0.8409063000, -0.03465778),
        v2(-0.6517572000, -0.07476326),
        v2(-0.1041822000, -0.02521214),
        v2(-0.3042712000, -0.02195431),
        v2(-0.5082307000, +0.1079806),
        v2(-0.0842987700, -0.2316298),
        v2(-0.9879128000, +0.1113683),
        v2(-0.3859636000, +0.3363545),
        v2(-0.1925334000, +0.1787288),
        v2(+0.0032561820, +0.138135),
        v2(-0.8706837000, +0.3010679),
        v2(-0.6982038000, +0.1904326),
        v2(+0.1975043000, +0.2221317),
        v2(+0.1507788000, +0.4204168),
        v2(+0.3514056000, +0.09865579),
        v2(+0.1558783000, -0.08460935),
        v2(-0.0684978000, +0.4461993),
        v2(+0.3780522000, +0.3478679),
        v2(+0.3956799000, -0.1469177),
        v2(+0.5838975000, +0.1054943),
        v2(+0.6155105000, +0.3245716),
        v2(+0.3928624000, -0.4417621),
        v2(+0.1749884000, -0.4202175),
        v2(+0.6813727000, -0.2424808),
        v2(-0.6707711000, +0.4912741),
        v2(+0.0005130528, -0.8058334),
        v2(+0.0270301300, -0.6010728),
        v2(-0.1658188000, -0.9695674),
        v2(+0.4060591000, -0.7100726),
        v2(+0.7713396000, -0.4713659),
        v2(+0.5732120000, -0.5154400),
        v2(-0.3448896000, -0.9046497),
        v2(+0.1268544000, -0.9874692),
        v2(+0.7418533000, -0.6667366),
        v2(+0.3492522000, +0.5924662),
        v2(+0.5679897000, +0.5343465),
        v2(+0.5663417000, +0.7708698),
        v2(+0.7375497000, +0.6691415),
        v2(+0.2271994000, -0.6163502),
        v2(+0.2312844000, +0.8725659),
        v2(+0.4216993000, +0.9002838),
        v2(+0.4262091000, -0.9013284),
        v2(+0.2001408000, -0.808381),
        v2(+0.1493940000, +0.6650763),
        v2(-0.0964037600, +0.9843736),
        v2(+0.7682328000, -0.07273844),
        v2(+0.0414658400, +0.8313184),
        v2(+0.9705266000, -0.1143304),
        v2(+0.9670017000, +0.1293385),
        v2(+0.9015037000, -0.3306949),
        v2(-0.5085648000, +0.7534177),
        v2(+0.9055501000, +0.3758393),
        v2(+0.7599946000, +0.1809109),
        v2(-0.2483695000, +0.7942952),
        v2(-0.4241052000, +0.5581087),
        v2(-0.1020106000, +0.6724468)
    );

    int TapCount = 64;
    for (int i = 0; i < TapCount; i++)
    {
        v2 Offset = Scale * PoissonOffsets[i];
        Shadow1 += texture(ShadowSampler, P1 + v4(Offset, 0.0, 0.0));
        Shadow2 += texture(ShadowSampler, P2 + v4(Offset, 0.0, 0.0));
    }
    float InverseSumWeight = 1.0 / float(TapCount);

#elif 0
    // Disk filter
#if 1
    const int FilterHalfSize = 3;
    const float W[7][7] =
    {
        { 0.0,0.0,0.5,1.0,0.5,0.0,0.0 },
        { 0.0,1.0,1.0,1.0,1.0,1.0,0.0 },
        { 0.5,1.0,1.0,1.0,1.0,1.0,0.5 },
        { 1.0,1.0,1.0,1.0,1.0,1.0,1.0 },
        { 0.5,1.0,1.0,1.0,1.0,1.0,0.5 },
        { 0.0,1.0,1.0,1.0,1.0,1.0,0.0 },
        { 0.0,0.0,0.5,1.0,0.5,0.0,0.0 }
    };
#else
    const int FilterHalfSize = 2;
    const float W[5][5] =
    {
        { 0.0, 0.5, 1.0, 0.5, 0.0 },
        { 0.5, 1.0, 1.0, 1.0, 0.5 },
        { 1.0, 1.0, 1.0, 1.0, 1.0 },
        { 0.5, 1.0, 1.0, 1.0, 0.5 },
        { 0.0, 0.5, 1.0, 0.5, 0.0 },
    };
#endif

    float SumWeight = 0.0;
    for (int y = -FilterHalfSize; y <= +FilterHalfSize; y++)
    {
        for (int x = -FilterHalfSize; x <= +FilterHalfSize; x++)
        {
            float Weight = W[y + FilterHalfSize][x + FilterHalfSize];

            Shadow1 += Weight * texture(ShadowSampler, P1 + Scale * v4(float(x), float(y), 0.0, 0.0));
            Shadow2 += Weight * texture(ShadowSampler, P2 + Scale * v4(float(x), float(y), 0.0, 0.0));

            SumWeight += Weight;
        }
    }
    float InverseSumWeight = 1.0 / SumWeight;
#else
    // 4-tap jagged filter
    float InverseSumWeight = 1.0 / 4.0;
    float d = 3.0 / 16.0;
    float o = d * TexelSize.x;
    float s1 = 1.0;
    float s2 = 3.0;
    v2 SampleOffsets[4] = 
    {
        (d * Scale) * v2(-s1, -s2),
        (d * Scale) * v2(+s2, -s1),
        (d * Scale) * v2(+s1, +s2),
        (d * Scale) * v2(-s2, +s1),
    };

    Shadow1 += texture(ShadowSampler, P1 + v4(SampleOffsets[0], 0.0, 0.0));
    Shadow1 += texture(ShadowSampler, P1 + v4(SampleOffsets[1], 0.0, 0.0));
    Shadow1 += texture(ShadowSampler, P1 + v4(SampleOffsets[2], 0.0, 0.0));
    Shadow1 += texture(ShadowSampler, P1 + v4(SampleOffsets[3], 0.0, 0.0));

    Shadow2 += texture(ShadowSampler, P2 + v4(SampleOffsets[0], 0.0, 0.0));
    Shadow2 += texture(ShadowSampler, P2 + v4(SampleOffsets[1], 0.0, 0.0));
    Shadow2 += texture(ShadowSampler, P2 + v4(SampleOffsets[2], 0.0, 0.0));
    Shadow2 += texture(ShadowSampler, P2 + v4(SampleOffsets[3], 0.0, 0.0));
#endif

    float Result = InverseSumWeight * mix(Shadow2, Shadow1, Blend);
    return(Result);
}

void main()
{
    vec4 BaseColor = UnpackRGBA8(Material.Diffuse);
    vec4 BaseMetallicRoughness = UnpackRGBA8(Material.MetallicRoughness);

    vec4 Albedo = texture(sampler2D(Textures[Material.DiffuseID], Sampler), TexCoord);
    Albedo.rgb *= BaseColor.rgb;

    // NOTE(boti): Light sources contribute to this term, to produce a fake GI effect
    vec3 AmbientTerm = vec3(0.25);

    vec3 Normal;
    Normal.xy = texture(sampler2D(Textures[Material.NormalID], Sampler), TexCoord).xy;
    Normal.xy = 2.0 * Normal.xy - vec2(1.0);
    Normal.z = sqrt(1.0 - dot(Normal.xy, Normal.xy));
    vec4 MetallicRoughness = texture(sampler2D(Textures[Material.MetallicRoughnessID], Sampler), TexCoord);
    float Roughness = MetallicRoughness.g * BaseMetallicRoughness.g;
    float Metallic = MetallicRoughness.b * BaseMetallicRoughness.b;

    float ScreenSpaceOcclusion = textureLod(OcclusionBuffer, gl_FragCoord.xy, 0).r;

    mat3 TBN = mat3(
        normalize(InT),
        normalize(InB),
        normalize(InN));
    vec3 N = TBN * Normal;

    vec3 V = normalize(-P);
    vec3 F0 = mix(vec3(0.04), Albedo.rgb, Metallic);
    vec3 DiffuseBase = (1.0 - Metallic) * (vec3(1.0) - F0) * Albedo.rgb;

    float SunShadow = CalculateShadow(ShadowP, ShadowBlends);
    vec3 Lo = CalculateOutgoingLuminance(SunShadow * PerFrame.SunL, PerFrame.SunV, N, V,
                                         DiffuseBase, F0, Roughness);

    uint TileX = uint(gl_FragCoord.x) / R_TileSizeX;
    uint TileY = uint(gl_FragCoord.y) / R_TileSizeY;
    uint TileIndex = TileX + TileY * PerFrame.TileCount.x;
    for (uint i = 0; i < Tiles[TileIndex].LightCount; i++)
    {
        uint LightIndex = Tiles[TileIndex].LightIndices[i];
        v3 dP = Lights[LightIndex].P.xyz - P;
        v3 E = Lights[LightIndex].E.xyz * Lights[LightIndex].E.w;
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
        if (max(E.x, max(E.y, E.z)) < R_LuminanceThreshold)
        {
            E = vec3(0.0);
        }

        uint ShadowIndex = uint(Lights[LightIndex].P.w);
        float Shadow = 1.0;
        if (ShadowIndex != 0xFF)
        {
            v3 SampleP = TransformDirection(PerFrame.CameraTransform, -dP);
            v3 AbsP = abs(SampleP);
            f32 MaxXY = max(AbsP.x, AbsP.y);
            f32 Depth = max(MaxXY, AbsP.z);

            ivec2 ShadowResolution = textureSize(PointShadows[ShadowIndex], 0);
            f32 TexelSize = 1.0 / float(ShadowResolution.x);
            f32 Offset = 2.0 * TexelSize;

            // HACK(boti): These will need to be stored and fetched from the per-frame uniform buffer
            f32 n = 0.05;
            f32 f = 10.0;
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
            Limit.xy -= OffsetXY * (1.0 / 1024.0);
            Limit.yz -= OffsetYZ * (1.0 / 1024.0);

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
            //AmbientTerm += 0.05 * E;

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

    vec3 Ambient = AmbientTerm * Albedo.rgb * ScreenSpaceOcclusion;
    Lo += Ambient + Material.Emissive;

    {
#if 0
        vec3 La = vec3(AmbientFactor);

        const vec3 BetaR0 = vec3(3.8e-6, 13.5e-6, 33.1e-6);
        const vec3 BetaM0 = vec3(21e-4);
        float Distance = length(P);

        const float g = 0.76;
        const float g2 = g*g;
        float Denom0 = 1.0 + g2 + 2*g/* * 1.0*/;
        float Denom = sqrt(Denom0) * Denom0;;
        float PhaseMieOut = 1.0 - 0.07957747 * ((1.0 - g) / Denom);

        // NOTE(boti): BetaM0 is the scattering coefficient of Mie scattering
        // 1.1111 * BetaM0 is the full extinction
        vec3 fMie = vec3(exp(-1.1111 * BetaM0 * (PhaseMieOut * Distance)));
        Lo = Lo * fMie + La * (vec3(1.0) - fMie);
#else
        Lo += 0.02 * PerFrame.SunL * CalculateAtmosphere(P, PerFrame, ShadowSampler);
#endif
    }
    
    Out0 = vec4(Lo, 1.0);
}

#endif