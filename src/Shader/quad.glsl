#version 460 core

layout(push_constant) uniform PushConstants
{
    vec2 P;
    vec2 HalfExtent;
    vec3 Color;
};

#if defined(VS)
void main()
{
    vec2 VertexData[4] = 
    {
        vec2(-1.0, +1.0),
        vec2(+1.0, +1.0),
        vec2(-1.0, -1.0),
        vec2(+1.0, -1.0),
    };
    uint IndexData[6] = 
    {
        0, 1, 2, 
        1, 3, 2,
    };

    vec2 Vertex = VertexData[IndexData[gl_VertexIndex]];
    gl_Position = vec4(HalfExtent * Vertex + P, 0.0, 1.0);
}
#else
layout(location = 0) out vec4 Target0;

void main()
{
    Target0 = vec4(Color, 1.0);
}
#endif