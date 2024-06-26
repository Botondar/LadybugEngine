#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

interpolant(0) vec3 P;
interpolant(1) vec2 TexCoord;
interpolant(2) vec3 TriN;
interpolant(3) vec3 TriT;
interpolant(4) f32 TangentSign;
interpolant(5) vec3 ShadowP;
interpolant(6) vec3 CascadeBlends;
interpolant(7) flat uint InstanceIndex;


#if defined(VS)

invariant gl_Position;

void main()
{
    instance_buffer_r InstanceBuffer = instance_buffer_r(PerFrame.InstanceBufferAddress);
    instance_data Instance = InstanceBuffer.Data[gl_InstanceIndex];

    vertex_buffer_r VertexBuffer = vertex_buffer_r(Instance.VertexBufferAddress);

    precise v3 WorldP   = TransformPoint(Instance.Transform, VertexBuffer.Data[gl_VertexIndex].P);
    TexCoord            = VertexBuffer.Data[gl_VertexIndex].TexCoord;
    TriN                = normalize(mat3(PerFrame.ViewTransform) * (VertexBuffer.Data[gl_VertexIndex].N * inverse(mat3(Instance.Transform))));
    v4 Tangent          = VertexBuffer.Data[gl_VertexIndex].T;
    TriT                = normalize((mat3(PerFrame.ViewTransform) * (mat3(Instance.Transform) * Tangent.xyz)));
    TangentSign         = Tangent.w;

    P = TransformPoint(PerFrame.ViewTransform, WorldP);

    // NOTE(boti): This works because orthographic projection is an affine transform
    ShadowP = TransformPoint(PerFrame.CascadeViewProjections[0], WorldP);
    ShadowP.xy = 0.5 * ShadowP.xy + v2(0.5);
    CascadeBlends = GetCascadeBlends(PerFrame.CascadeMinDistances, PerFrame.CascadeMaxDistances, P.z);

    InstanceIndex = gl_InstanceIndex;

    gl_Position = PerFrame.ViewProjectionTransform * v4(WorldP, 1.0);
}

#elif defined(FS)

#ifndef ShaderVariant_Transmission
layout(early_fragment_tests) in;
#endif

SetBindingLayout(Static, MipFeedbackBuffer, scalar)
coherent buffer MipFeedbackBuffer
{
    uint MipFeedbacks[];
};

SetBinding(Static, BRDFLutTexture) uniform texture2D BRDFLut;

SetBinding(Static, StructureImage) uniform texture2D StructureImage;
SetBinding(Static, OcclusionImage) uniform texture2D OcclusionImage;

SetBinding(Static, CascadedShadow) uniform texture2DArray CascadedShadow;
SetBinding(Static, PointShadows) uniform textureCube PointShadows[];

SetBinding(Sampler, NamedSamplers) uniform sampler Samplers[Sampler_Count];
SetBinding(Sampler, MaterialSamplers) uniform sampler MatSamplers[R_MaterialSamplerCount];
SetBinding(Bindless, Textures) uniform texture2D Textures[];

layout(location = 0, index = 0) out v4 Out0;
#if ShaderVariant_Transmission
layout(location = 0, index = 1) out v4 SourceTransmission;
#endif

void main()
{
    instance_buffer_r   InstanceBuffer  = instance_buffer_r(PerFrame.InstanceBufferAddress);
    light_buffer_r      LightBuffer     = light_buffer_r(PerFrame.LightBufferAddress);
    tile_buffer_r       TileBuffer      = tile_buffer_r(PerFrame.TileBufferAddress);

    v3 Lo = vec3(0.0);
    v3 V = normalize(-P);
    f32 Alpha = 1.0;

    uint TileIndex;
    {
        uint TileX = uint(gl_FragCoord.x) / R_TileSizeX;
        uint TileY = uint(gl_FragCoord.y) / R_TileSizeY;
        TileIndex = TileX + TileY * PerFrame.TileCount.x;
    }
    
    {
        instance_data Instance = InstanceBuffer.Data[InstanceIndex];

        v2 UV = TexCoord;
        // Parallax
        #if 0
        {
            const uint StepCount = 4;
            v3 TangentV = normalize(v3(dot(TriT, V), dot(TriB, V), dot(TriN, V)));
            v2 ParallaxV = PerFrame.ParallaxScale * (1.0 / StepCount) * TangentV.xy;
            for (uint Step = 0; Step < StepCount; Step++)
            {
                f32 Height = 2.0 * texture(sampler2D(Textures[Instance.Material.HeightID], MatSamplers[Instance.Material.AlbedoSamplerID]), UV).r - 1.0;
                v3 N = UnpackSurfaceNormal01(texture(sampler2D(Textures[Instance.Material.NormalID], MatSamplers[Instance.Material.NormalSamplerID]), UV).xy);
                v2 dUV = N.z * Height * ParallaxV;
                UV += dUV;
            }
        }
        #endif


#if ShaderVariant_Transmission
        f32 Transmission = Instance.Material.Transmission * texture(sampler2D(Textures[Instance.Material.TransmissionID], MatSamplers[Instance.Material.TransmissionSamplerID]), UV).r;
#endif
        v4 BaseAlbedo = UnpackRGBA8(Instance.Material.BaseAlbedo);
        v4 Albedo = BaseAlbedo * texture(sampler2D(Textures[Instance.Material.AlbedoID], MatSamplers[Instance.Material.AlbedoSamplerID]), UV);
        v4 MetallicRoughness = texture(sampler2D(Textures[Instance.Material.MetallicRoughnessID], MatSamplers[Instance.Material.MetallicRoughnessSamplerID]), UV);
        MetallicRoughness *= UnpackRGBA8(Instance.Material.BaseMaterial);
        v3 N = UnpackSurfaceNormal01(texture(sampler2D(Textures[Instance.Material.NormalID], MatSamplers[Instance.Material.NormalSamplerID]), UV).xy);
        v3 TriB = cross(TriN, TriT) * TangentSign;
        N = normalize(TriT) * N.x + normalize(TriB) * N.y + normalize(TriN) * N.z;

        Alpha = Albedo.a;

        // Desired mip level feedback
        {
            uint MipBucket = GetMipBucketFromDerivatives(dFdxFine(UV), dFdyFine(UV));
            atomicOr(MipFeedbacks[Instance.Material.AlbedoID], MipBucket);
            atomicOr(MipFeedbacks[Instance.Material.NormalID], MipBucket);
            atomicOr(MipFeedbacks[Instance.Material.MetallicRoughnessID], MipBucket);
            atomicOr(MipFeedbacks[Instance.Material.OcclusionID], MipBucket);
            atomicOr(MipFeedbacks[Instance.Material.HeightID], MipBucket);
#if ShaderVariant_Transmission
            atomicOr(MipFeedbacks[Instance.Material.TransmissionID], MipBucket);
#endif
        }

        vec3 F0 = mix(vec3(0.04), Albedo.rgb, MetallicRoughness.b);
        vec3 DiffuseBase = (1.0 - MetallicRoughness.b) * (vec3(1.0) - F0) * Albedo.rgb;

        Lo += Instance.Material.Emissive;
        {
            f32 Occlusion = texelFetch(OcclusionImage, v2s(gl_FragCoord.xy), 0).r;
            v2 EnvBRDF = textureLod(sampler2D(BRDFLut, Samplers[Sampler_LinearEdgeClamp]), v2(MetallicRoughness.g, Clamp01(dot(N, V))), 0.0).rg;
            Lo += Occlusion * PerFrame.Ambience * (DiffuseBase + F0 * EnvBRDF.x + EnvBRDF.y);
        }

        f32 SunShadow = CalculateCascadedShadow(
            CascadedShadow, Samplers[Sampler_Shadow], 
            ShadowP, CascadeBlends,
            PerFrame.CascadeScales, PerFrame.CascadeOffsets);
#if ShaderVariant_Transmission
        Lo += CalculateOutgoingLuminanceTransmission(
            SunShadow * PerFrame.SunL, PerFrame.SunV, N, V,
            DiffuseBase, F0, MetallicRoughness.g, Transmission);

        SourceTransmission = v4(mix(v3(1.0), Transmission * DiffuseBase, Albedo.a), 1.0);
#else
        Lo += CalculateOutgoingLuminance(
            SunShadow * PerFrame.SunL, PerFrame.SunV, N, V,
            DiffuseBase, F0, MetallicRoughness.g);
#endif

        for (uint LightIndex = 0; LightIndex < TileBuffer.Data[TileIndex].LightCount; LightIndex++)
        {
            light Light = LightBuffer.Data[TileBuffer.Data[TileIndex].LightIndices[LightIndex]];
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
#if ShaderVariant_Transmission
            Lo += CalculateOutgoingLuminanceTransmission(
                Shadow * E, normalize(dP), N, V,
                DiffuseBase, F0, MetallicRoughness.g, Transmission);
#else
            Lo += CalculateOutgoingLuminance(
                Shadow * E, normalize(dP), N, V,
                DiffuseBase, F0, MetallicRoughness.g);
#endif
        }
    }

#if 1
    f32 Atmosphere = CalculateAtmosphere(
        gl_FragCoord.xy, 
        P, 
        PerFrame.SunV,
        PerFrame.CascadeViewProjections[0] * PerFrame.CameraTransform,
        PerFrame.CascadeScales,
        PerFrame.CascadeOffsets,
        PerFrame.CascadeMaxDistances,
        CascadedShadow,
        Samplers[Sampler_Shadow]);
    Lo += AtmosphereIntensity * Atmosphere * PerFrame.SunL;
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

    Lo = NanTo0(Lo);
    Out0 = vec4(Lo, Alpha);
}

#endif