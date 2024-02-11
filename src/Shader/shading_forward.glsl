#version 460

#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

SetBindingLayout(PerFrame, InstanceBuffer, scalar)
readonly buffer InstanceBuffer
{
    instance_data Instances[];
};

interpolant(0) vec3 P;
interpolant(1) vec2 TexCoord;
interpolant(2) vec3 TriN;
interpolant(3) vec3 TriT;
interpolant(4) vec3 TriB;
interpolant(5) vec3 ShadowP;
interpolant(6) vec3 CascadeBlends;
interpolant(7) flat uint InstanceIndex;

#if defined(VS)

invariant gl_Position;

layout(location = Attrib_Position)      in vec3 aP;
layout(location = Attrib_Normal)        in vec3 aN;
layout(location = Attrib_TangentSign)   in vec4 aT;
layout(location = Attrib_TexCoord)      in vec2 aTexCoord;
layout(location = Attrib_Color)         in vec4 aColor;

void main()
{
    instance_data Instance = Instances[gl_InstanceIndex];
    precise vec3 WorldP = TransformPoint(Instance.Transform, aP);
    TexCoord = aTexCoord;
    P = TransformPoint(PerFrame.ViewTransform, WorldP);
    TriN = normalize(mat3(PerFrame.ViewTransform) * (aN * inverse(mat3(Instance.Transform))));
    TriT = normalize((mat3(PerFrame.ViewTransform) * (mat3(Instance.Transform) * aT.xyz)));
    TriB = normalize(cross(TriT, TriN)) * aT.w;

    // NOTE(boti): This works because orthographic projection is an affine transform
    ShadowP = TransformPoint(PerFrame.CascadeViewProjections[0], WorldP);
    ShadowP.xy = 0.5 * ShadowP.xy + v2(0.5);
    CascadeBlends = GetCascadeBlends(PerFrame.CascadeMinDistances, PerFrame.CascadeMaxDistances, P.z);

    InstanceIndex = gl_InstanceIndex;

    gl_Position = PerFrame.ViewProjectionTransform * v4(WorldP, 1.0);
}

#elif defined(FS)

layout(early_fragment_tests) in;

SetBindingLayout(PerFrame, LightBuffer, scalar)
readonly buffer LightBuffer
{
    light Lights[];
};
SetBindingLayout(PerFrame, TileBuffer, scalar)
readonly buffer TileBuffer
{
    screen_tile Tiles[];
};

SetBindingLayout(PerFrame, MipMaskBuffer, scalar)
buffer MipMaskBuffer
{
    uint MipMasks[];
};

SetBinding(PerFrame, StructureImage) uniform texture2D StructureImage;
SetBinding(PerFrame, OcclusionImage) uniform texture2D OcclusionImage;

SetBinding(PerFrame, CascadedShadow) uniform texture2DArray CascadedShadow;
SetBinding(PerFrame, PointShadows) uniform textureCube PointShadows[];

SetBinding(Sampler, NamedSamplers) uniform sampler Samplers[Sampler_Count];
SetBinding(Bindless, Textures) uniform texture2D Textures[];

layout(location = 0) out vec4 Out0;

void main()
{
    v3 Lo = vec3(0.0);
    v3 V = normalize(-P);

    uint TileIndex;
    {
        uint TileX = uint(gl_FragCoord.x) / R_TileSizeX;
        uint TileY = uint(gl_FragCoord.y) / R_TileSizeY;
        TileIndex = TileX + TileY * PerFrame.TileCount.x;
    }
    
    {
        instance_data Instance = Instances[InstanceIndex];

        v4 BaseColor = UnpackRGBA8(Instance.Material.DiffuseColor);
        v4 BaseMetallicRoughness = UnpackRGBA8(Instance.Material.BaseMaterial);
        v4 Albedo = BaseColor * texture(sampler2D(Textures[Instance.Material.DiffuseID], Samplers[Sampler_Default]), TexCoord);
        v4 MetallicRoughness = texture(sampler2D(Textures[Instance.Material.MetallicRoughnessID], Samplers[Sampler_Default]), TexCoord);
        f32 Roughness = MetallicRoughness.g * BaseMetallicRoughness.g;
        f32 Metallic = MetallicRoughness.b * BaseMetallicRoughness.b;
        v3 N = UnpackSurfaceNormal01(texture(sampler2D(Textures[Instance.Material.NormalID], Samplers[Sampler_Default]), TexCoord).xy);
        N = normalize(TriT) * N.x + normalize(TriB) * N.y + normalize(TriN) * N.z;

        // Desired mip level feedback
        {
            uint MipBucket = GetMipBucketFromDerivatives(dFdxFine(TexCoord), dFdxFine(TexCoord));
            atomicOr(MipMasks[Instance.Material.DiffuseID], MipBucket);
            atomicOr(MipMasks[Instance.Material.NormalID], MipBucket);
            atomicOr(MipMasks[Instance.Material.MetallicRoughnessID], MipBucket);
        }

        vec3 F0 = mix(vec3(0.04), Albedo.rgb, Metallic);
        vec3 DiffuseBase = (1.0 - Metallic) * (vec3(1.0) - F0) * Albedo.rgb;

        Lo += Instance.Material.Emissive;
        Lo += PerFrame.Ambience * Albedo.rgb * texelFetch(OcclusionImage, v2s(gl_FragCoord.xy), 0).r;

        f32 SunShadow = CalculateCascadedShadow(
            CascadedShadow, Samplers[Sampler_Shadow], 
            ShadowP, CascadeBlends,
            PerFrame.CascadeScales, PerFrame.CascadeOffsets);
        Lo += CalculateOutgoingLuminance(SunShadow * PerFrame.SunL, PerFrame.SunV, N, V,
                                         DiffuseBase, F0, Roughness);

        for (uint LightIndex = 0; LightIndex < Tiles[TileIndex].LightCount; LightIndex++)
        {
            light Light = Lights[Tiles[TileIndex].LightIndices[LightIndex]];
            v3 dP, E;
            CalculatePointLightParams(Light, P, dP, E);
            
            f32 Shadow = 1.0;
            if (Light.ShadowIndex != 0xFFFFFFFFu)
            {
                v3 ShadowP = TransformDirection(PerFrame.CameraTransform, -dP);
                Shadow = CalculatePointShadow(PointShadows[Light.ShadowIndex], Samplers[Sampler_Shadow], 
                                              ShadowP, 
                                              PerFrame.PointShadows[Light.ShadowIndex].Near, 
                                              PerFrame.PointShadows[Light.ShadowIndex].Far);
            }
            Lo += Shadow * CalculateOutgoingLuminance(Shadow * E, normalize(dP), N, V,
                                                      DiffuseBase, F0, Roughness);
        }
    }

#if 1
    Lo += AtmosphereIntensity * CalculateAtmosphere(gl_FragCoord.xy, P, PerFrame, CascadedShadow, Samplers[Sampler_Shadow]) * PerFrame.SunL;
#else
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
}

#endif