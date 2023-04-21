#version 460 core

float GetLuminance(in vec3 Color)
{
    vec3 Factors = vec3(0.212639, 0.715169, 0.072192);
    float Result = dot(Factors, Color);
    return Result;
}

vec3 SetLuminance(in vec3 Color, in vec3 TargetLuminance)
{
    float CurrentLuminance = GetLuminance(Color);
    vec3 Result = Color * (TargetLuminance / CurrentLuminance);
    return Result;
}

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

layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 Out0;

void main()
{
    vec3 Sample = textureLod(Texture, TexCoord, 0).rgb;
#if 0
    float Exposure = 0.05;
    Sample = vec3(1.0) - exp(-Sample * Exposure);
#elif 0
    Sample = Sample / (vec3(1.0) + Sample);
#elif 1
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    Sample =  clamp((Sample*(a*Sample+b))/(Sample*(c*Sample+d)+e), vec3(0.0), vec3(1.0));
    //Sample = Sample*Sample;
#endif
    Out0 = vec4(Sample, 1.0);
}

#endif