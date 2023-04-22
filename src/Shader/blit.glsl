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

layout(set = 0, binding = 0) uniform sampler2D Texture;
layout(set = 0, binding = 1) uniform sampler2D BloomTexture;

layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 Out0;

void main()
{
    vec3 Sample = textureLod(Texture, TexCoord, 0).rgb;
    vec3 SampleBloom = textureLod(BloomTexture, TexCoord, 0).rgb;

    float Exposure = 0.5;
#if 0
    Sample = vec3(1.0) - exp(-Sample * Exposure);
#elif 0
    Sample = Sample / (vec3(1.0) + Sample);
#elif 1
    Sample = Exposure * Sample;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    Sample =  clamp((Sample*(a*Sample+b))/(Sample*(c*Sample+d)+e), vec3(0.0), vec3(1.0));
    //Sample = Sample*Sample;
#endif
    float BloomStrength = 0.2;
    Out0 = vec4(mix(Sample, SampleBloom, BloomStrength), 1.0);
}

#endif