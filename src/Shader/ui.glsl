#include "common.glsli"

interpolant(0) vec2 TexCoord;
interpolant(1) vec4 Color;

layout(push_constant) uniform PushConstants
{
    mat4 Transform;
    renderer_texture_id TextureID;
};

#ifdef VS

layout(location = 0) in vec2 aP;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

void main()
{
    gl_Position = Transform * vec4(aP, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
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