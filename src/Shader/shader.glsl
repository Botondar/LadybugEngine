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
    vec4 WorldP = ModelTransform * vec4(aP, 1.0);
    TexCoord = aTexCoord;
    P = (PerFrame.View * WorldP).xyz;
    N = normalize(mat3(PerFrame.View) * (aN * inverse(mat3(ModelTransform))));
    T = normalize((mat3(PerFrame.View) * (mat3(ModelTransform) * aT.xyz)));
    B = normalize(cross(T, N)) * aT.w;

    ShadowP = (PerFrame.CascadeViewProjection * WorldP).xyz;
    CascadeBlends = GetCascadeBlends(PerFrame.CascadeMinDistances, 
                                     PerFrame.CascadeMaxDistances,
                                     P);
    gl_Position = PerFrame.ViewProjection * WorldP;
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


layout(location = 0) in vec3 P;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 InN;
layout(location = 3) in vec3 InT;
layout(location = 4) in vec3 InB;
layout(location = 5) in vec3 ShadowP;
layout(location = 6) in vec3 ShadowBlends;

layout(location = 0) out vec4 Out0;

float CalculateShadow(vec3 SP, in vec3 CascadeBlends)
{
#if 1
    vec3 CascadeCoord0 = vec3(0.5 * SP.xy + vec2(0.5), SP.z);
    vec3 CascadeCoord1 = PerFrame.CascadeScales[0] * SP + PerFrame.CascadeOffsets[0];
    vec3 CascadeCoord2 = PerFrame.CascadeScales[1] * SP + PerFrame.CascadeOffsets[1];
    vec3 CascadeCoord3 = PerFrame.CascadeScales[2] * SP + PerFrame.CascadeOffsets[2];

    CascadeCoord1.xy = 0.5 * CascadeCoord1.xy + vec2(0.5);
    CascadeCoord2.xy = 0.5 * CascadeCoord2.xy + vec2(0.5);
    CascadeCoord3.xy = 0.5 * CascadeCoord3.xy + vec2(0.5);
#else
    vec3 CascadeCoord0 = vec3(0.5 * SP0.xy + vec2(0.5), SP0.z);
    vec3 CascadeCoord1 = vec3(0.5 * SP1.xy + vec2(0.5), SP1.z);
    vec3 CascadeCoord2 = vec3(0.5 * SP2.xy + vec2(0.5), SP2.z);
    vec3 CascadeCoord3 = vec3(0.5 * SP3.xy + vec2(0.5), SP3.z);
#endif
    vec2 ShadowMapSize = vec2(textureSize(ShadowSampler, 0).xy);
    vec2 TexelSize = 1.0 / ShadowMapSize;    

    bool BeyondCascade2 = (CascadeBlends.y >= 0.0);
    bool BeyondCascade3 = (CascadeBlends.z >= 0.0);

    vec3 Blends = clamp(CascadeBlends, vec3(0.0), vec3(1.0));

    vec4 P1, P2;
    float Blend;
    if (BeyondCascade3)
    {
        P1 = vec4(CascadeCoord2.xy, 2.0, CascadeCoord2.z);
        P2 = vec4(CascadeCoord3.xy, 3.0, CascadeCoord3.z);
        Blend = Blends.z;
    }
    else if (BeyondCascade2)
    {
        P1 = vec4(CascadeCoord1.xy, 1.0, CascadeCoord1.z);
        P2 = vec4(CascadeCoord2.xy, 2.0, CascadeCoord2.z);
        Blend = Blends.y;
    }
    else
    {
        P1 = vec4(CascadeCoord0.xy, 0.0, CascadeCoord0.z);
        P2 = vec4(CascadeCoord1.xy, 1.0, CascadeCoord1.z);
        Blend = Blends.x;
    }

    float Shadow1 = 0.0;
    float Shadow2 = 0.0;

#if 0
    const float W[3][3] =
    {
        { 0.5, 1.0, 0.5 },
        { 1.0, 1.0, 1.0 },
        { 0.5, 1.0, 0.5 },
    };
    int MinW = -(W.length() / 2);
    int MaxW = -MinW;

    float SumWeight = 7.0;
    for (int y = MinW; y <= MaxW; y++)
    {
        for (int x = MinW; x <= MaxW; x++)
        {
            vec2 SampleOffset = vec2(float(x), float(y)) * TexelSize;
            float Weight = W[y + MaxW][x + MaxW];
            Shadow1 += Weight * texture(ShadowSampler, P1 + vec4(SampleOffset, 0.0, 0.0));
            Shadow2 += Weight * texture(ShadowSampler, P2 + vec4(SampleOffset, 0.0, 0.0));
            //SumWeight += Weight;
        }
    }
#else
    float SumWeight = 8.0;
    vec2 SampleOffset = 1.0 * TexelSize;
    Shadow1 += 4.0 * texture(ShadowSampler, P1);
    Shadow1 += texture(ShadowSampler, P1 + vec4(-SampleOffset.x, -SampleOffset.y, 0.0, 0.0));
    Shadow1 += texture(ShadowSampler, P1 + vec4(+SampleOffset.x, -SampleOffset.y, 0.0, 0.0));
    Shadow1 += texture(ShadowSampler, P1 + vec4(-SampleOffset.x, +SampleOffset.y, 0.0, 0.0));
    Shadow1 += texture(ShadowSampler, P1 + vec4(+SampleOffset.x, +SampleOffset.y, 0.0, 0.0));

    Shadow2 += 4.0 * texture(ShadowSampler, P2);
    Shadow2 += texture(ShadowSampler, P2 + vec4(-SampleOffset.x, -SampleOffset.y, 0.0, 0.0));
    Shadow2 += texture(ShadowSampler, P2 + vec4(+SampleOffset.x, -SampleOffset.y, 0.0, 0.0));
    Shadow2 += texture(ShadowSampler, P2 + vec4(-SampleOffset.x, +SampleOffset.y, 0.0, 0.0));
    Shadow2 += texture(ShadowSampler, P2 + vec4(+SampleOffset.x, +SampleOffset.y, 0.0, 0.0));
#endif
    float Result = mix(Shadow1, Shadow2, Blend) / SumWeight;
    return Result;
}

void main()
{
#if 1
    float Shadow = CalculateShadow(ShadowP, ShadowBlends);
#else
    float Shadow = 1.0;
#endif

    vec4 BaseColor = UnpackRGBA8(Material.Diffuse);
    vec4 BaseMetallicRoughness = UnpackRGBA8(Material.MetallicRoughness);

    vec4 Albedo = texture(sampler2D(Textures[Material.DiffuseID], Sampler), TexCoord);
    Albedo.rgb *= BaseColor.rgb;

    vec3 Normal;
    Normal.xy = texture(sampler2D(Textures[Material.NormalID], Sampler), TexCoord).xy;
    Normal.xy = 2.0 * Normal.xy - vec2(1.0);
    Normal.z = sqrt(1.0 - dot(Normal.xy, Normal.xy));
    vec4 MetallicRoughness = texture(sampler2D(Textures[Material.MetallicRoughnessID], Sampler), TexCoord);
    float Roughness = MetallicRoughness.g * BaseMetallicRoughness.g;
    float Metallic = MetallicRoughness.b * BaseMetallicRoughness.b;

    float ScreenSpaceOcclusion = textureLod(OcclusionBuffer, gl_FragCoord.xy, 0).r;

#if 1
    mat3 TBN = mat3(
        normalize(InT),
        normalize(InB),
        normalize(InN));
    vec3 N = TBN * Normal;
#else
    vec3 N = normalize(InN);
#endif

    vec3 V = normalize(-P);
    vec3 F0 = mix(vec3(0.04), Albedo.rgb, Metallic);
    vec3 DiffuseBase = (1.0 - Metallic) * (vec3(1.0) - F0) * Albedo.rgb;
    float NdotV = max(dot(N, V), 0.0);

    vec3 Lo = vec3(0.0, 0.0, 0.0);
    {
        float NdotL = max(dot(PerFrame.SunV, N), 0.0);
        vec3 Diffuse = DiffuseBase * NdotL;

        vec3 H = normalize(PerFrame.SunV + V);
        float VdotH = max(dot(V, H), 0.0);
        vec3 Fresnel = FresnelSchlick(F0, VdotH);

        float NdotH = max(dot(N, H), 0.0);
        float Distribution = DistributionGGX(Roughness, NdotH);
        float Geometry = GeometryGGX(Roughness, NdotV, NdotL);

        float SpecularDenominator = max(4.0 * NdotL * NdotV, 1e-4);
        vec3 Specular = Distribution * Geometry * Fresnel / SpecularDenominator;

        Lo += Shadow * (Diffuse + Specular) * PerFrame.SunL;
    }

    uint TileX = uint(gl_FragCoord.x) / R_TileSizeX;
    uint TileY = uint(gl_FragCoord.y) / R_TileSizeY;
    uint TileIndex = TileX + TileY * PerFrame.TileCount.x;

    for (uint i = 0; i < Tiles[TileIndex].LightCount; i++)
    {
        uint LightIndex = Tiles[TileIndex].LightIndices[i];
        vec3 L = Lights[LightIndex].P.xyz - P;
        vec3 E = Lights[LightIndex].E.xyz * Lights[LightIndex].E.w;
        float DistSq = dot(L, L);
        float InvDistSq = 1.0 / DistSq;
        L = L * sqrt(InvDistSq);
        E = E * InvDistSq;

        // NOTE(boti): When there are a lot of light sources close together,
        // their cumulative effect can be seen on quite far away texels from the source,
        // which produces "blocky" artefacts when only certain portions of a screen tile
        // passes the binning check.
        // To counteract this, we set the luminance of those lights to 0,
        // but a better way in the future would probably be to have an attenuation
        // that actually falls to 0.
        if (max(E.x, max(E.y, E.z)) < R_LuminanceThreshold)
        {
            E = vec3(0.0);
        }

        float NdotL = max(dot(L, N), 0.0);
        vec3 Diffuse = DiffuseBase * NdotL;

        vec3 H = normalize(L + V);
        float VdotH = max(dot(V, H), 0.0);
        vec3 Fresnel = FresnelSchlick(F0, VdotH);

        float NdotH = max(dot(N, H), 0.0);
        float Distribution = DistributionGGX(Roughness, NdotH);
        float Geometry = GeometryGGX(Roughness, NdotV, NdotL);

        float SpecularDenominator = max(4.0 * NdotL * NdotV, 1e-4);
        vec3 Specular = Distribution * Geometry * Fresnel / SpecularDenominator;
        Lo += (Diffuse + Specular) * E;
    }

    float AmbientFactor = 3e-1;
    vec3 Ambient = AmbientFactor * Albedo.rgb * ScreenSpaceOcclusion;
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