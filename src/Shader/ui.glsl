#version 460
#ifdef VS
layout(push_constant) uniform PushConstants
{
    mat4 Transform;
};

layout(location = 0) in vec2 aP;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec4 Color;

void main()
{
    gl_Position = Transform * vec4(aP, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
}
#else

layout(binding = 0, set = 0) uniform sampler2D Sampler;

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec4 Color;

layout(location = 0) out vec4 Out0;

void main()
{
    Out0 = Color * texture(Sampler, TexCoord);
}

#endif