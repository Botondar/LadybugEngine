#version 460 core

#include "common.glsli"

#if defined(VS)

layout(location = 0) out vec2 TexCoord;

void main()
{
    vec2 VertexData[] = 
    {
        vec2(-1.0, -1.0),
        vec2(+3.0, -1.0),
        vec2(-1.0, +3.0),
    };

    vec2 Vertex = VertexData[gl_VertexIndex];

    gl_Position = vec4(Vertex, 0.0, 1.0);
    TexCoord = 0.5*Vertex + vec2(0.5);
}

#elif defined(FS)

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

SetBindingLayout(PerFrame, TileBuffer, scalar)
readonly buffer TileBuffer
{
    screen_tile Tiles[];
};

SetBinding(PerFrame, HDRColorImage) uniform texture2D HDRColorImage;
SetBinding(PerFrame, BloomImage) uniform texture2D BloomImage;

SetBinding(PerFrame, OcclusionImage) uniform texture2D OcclusionImage;
SetBinding(PerFrame, StructureImage) uniform texture2D StructureImage;
SetBinding(PerFrame, VisibilityImage) uniform utexture2D VisibilityImage;

SetBinding(Sampler, NamedSamplers) uniform sampler Samplers[Sampler_Count];

layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 Out0;

v3 ACESFilm(v3 S)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    v3 Numerator = S * (a * S + b);
    v3 Denom = S * (c * S + d) + e;
    v3 Result = clamp(Numerator / Denom, vec3(0.0), vec3(1.0));
    return(Result);
}

/*
    https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
    MIT License - Copyright MJP and David Neubelt
*/
v3 ACESFilm2(v3 S)
{
    m3 InM = m3(
        v3(0.59719, 0.07600, 0.02840),
        v3(0.35458, 0.90834, 0.01566),
        v3(0.04823, 0.01566, 0.83777));

    S = InM * S;

    v3 a = S * (S + 0.0245786) - 0.000090537;
    v3 b = S * (0.983729 + 0.4329510) + 0.238081;
    S = a / b;

    m3 OutM = m3(
        v3(1.60475, -0.10208, -0.00327),
        v3(-0.53108, 1.10813, -0.07276),
        v3(-0.07367, -0.00605, 1.07602));
    S = OutM * S;
    S = clamp(S, v3(0.0), vec3(1.0));

    return(S);
}

void main()
{
    if (PerFrame.DebugViewMode == DebugView_None)
    {
        f32 Exposure = PerFrame.Exposure;

    // Incredibly silly dynamic exposure
#if 0
        {
            v3 AverageColor = textureLod(sampler2D(HDRColorImage, Samplers[Sampler_LinearEdgeClamp]), v2(0.0), 1000.0).rgb;
            f32 AverageLuminance = GetLuminance(AverageColor);
            f32 TargetLuminance = 0.4;
            Exposure = TargetLuminance / AverageLuminance;
        }
#endif

        vec3 Sample = texelFetch(HDRColorImage, v2s(gl_FragCoord.xy), 0).rgb;
        vec3 SampleBloom = texelFetch(BloomImage, v2s(gl_FragCoord.xy), 0).rgb;
        Sample = mix(Sample, SampleBloom, PerFrame.BloomStrength);

        vec3 Exposed = Sample * Exposure;
#if 0
        v3 Tonemapped = vec3(1.0) - exp(-Exposed);
#elif 0
        v3 Tonemapped = Exposed / (vec3(1.0) + Exposed);
#elif 1
        v3 Tonemapped = ACESFilm2(Exposed);
#endif
        Out0 = vec4(Tonemapped, 1.0);
    }
    else if (PerFrame.DebugViewMode == DebugView_LightOccupancy)
    {
        v2u TileID = v2u(gl_FragCoord.xy) / v2u(R_TileSizeX, R_TileSizeY);
        uint TileIndex = TileID.x + TileID.y * PerFrame.TileCount.x;

        uint LightCount = Tiles[TileIndex].LightCount;
        f32 Occupancy = f32(LightCount) / f32(R_MaxLightCountPerTile);
        
        v3 Colors[4] = v3[4](
            v3(0.0, 0.0, 1.0),
            v3(0.0, 1.0, 0.0),
            v3(1.0, 1.0, 0.0),
            v3(1.0, 0.0, 0.0));

        f32 fIndex = f32(Colors.length()) * Occupancy;
        f32 Factor = fract(fIndex);
        uint Index = uint(floor(fIndex));

        Out0 = v4(mix(Colors[Index], Colors[Index + 1], Factor), 1.0);
    }
    else if (PerFrame.DebugViewMode == DebugView_SSAO)
    {
        f32 Occlusion = texelFetch(OcclusionImage, v2s(gl_FragCoord.xy), 0).r;
        Out0 = v4(Occlusion.xxx, 1.0);
    }
    else if (PerFrame.DebugViewMode == DebugView_Structure)
    {
        v3 Structure = StructureDecode(texelFetch(StructureImage, v2s(gl_FragCoord.xy), 0));
        Structure.xy = (1.0 / 0.03) * Structure.xy + v2(0.5);
        Structure.z = PerFrame.ProjectionTransform[2][2] + PerFrame.ProjectionTransform[2][3] / Structure.z;
        Out0 = v4(Structure, 1.0);
    }
    else if (PerFrame.DebugViewMode == DebugView_InstanceID || PerFrame.DebugViewMode == DebugView_TriangleID)
    {
        v2u InstanceTriangleID = texelFetch(VisibilityImage, v2s(gl_FragCoord.xy), 0).xy;
        uint ID = (PerFrame.DebugViewMode == DebugView_InstanceID) ? InstanceTriangleID.x : InstanceTriangleID.y;
        uint HashID = HashPCG(ID);

        v3u Color8 = v3u(0xFFu) & v3u(HashID >> 16, HashID >> 8, HashID);
        v3 Color = (1.0 / 255.0) * v3(Color8);
        Out0 = v4(Color, 1.0);
    }
}

#endif