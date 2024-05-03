#include "common.glsli"

interpolant(0) vec2 TexCoord;
interpolant(1) vec4 Color;

layout(push_constant) uniform PushConstants
{
    mat4 Transform;
    vertex_2d_buffer_r VertexBuffer;
    renderer_texture_id TextureID;
};

#ifdef VS

void main()
{
    gl_Position = Transform * vec4(VertexBuffer.Data[gl_VertexIndex].P, 0.0, 1.0);
    TexCoord = VertexBuffer.Data[gl_VertexIndex].TexCoord;
    Color = UnpackRGBA8(VertexBuffer.Data[gl_VertexIndex].Color);
}
#else

SetBinding(Bindless, Textures) uniform texture2D Textures[];
SetBinding(Sampler, NamedSamplers) uniform sampler Samplers[Sampler_Count];

layout(location = 0) out vec4 Out0;

void main()
{
    vec4 Sample = texture(sampler2D(Textures[TextureID], Samplers[Sampler_LinearEdgeClamp]), TexCoord);
    Out0 = Color * Sample;
}

#endif