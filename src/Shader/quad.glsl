#version 460 core

#include "common.glsli"

layout(set = 0, binding = 0) uniform PerFrameBlock
{
    per_frame PerFrame;
};

#if defined(VS)

layout(set = 1, binding = 0) buffer VertexBlock
{
    vec3 VertexData[];
};

layout(location = 0) out vec2 TexCoord;

void main()
{
    vec3 BaseVertices[4] = 
    {
        vec3(-1.0, +1.0, 0.0),
        vec3(+1.0, +1.0, 0.0),
        vec3(-1.0, -1.0, 0.0),
        vec3(+1.0, -1.0, 0.0),
    };
    uint IndexData[6] = 
    {
        0, 1, 2,
        1, 3, 2,
    };

    uint GlobalIndex = gl_VertexIndex / 6;
    uint LocalIndex = gl_VertexIndex % 6;

    vec3 BaseVertex = BaseVertices[IndexData[LocalIndex]];
    vec3 Vertex = BaseVertex + TransformPoint(PerFrame.View, VertexData[GlobalIndex]);

    TexCoord = 0.5 * (BaseVertex.xy + vec2(1.0, 1.0));
    gl_Position = PerFrame.Projection * vec4(Vertex, 1.0);
}
#else

layout(set = 2, binding = 0) uniform sampler2DArray Texture;

layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 Target0;

void main()
{
    v4 Color = texture(Texture, vec3(TexCoord, 0));
    Target0 = vec4(0.8, 1.0, 0.4, 10.0 * Color.a);
}
#endif