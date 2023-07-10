#version 460 core

#include "common.glsli"

layout(set = 0, binding = 0, std140) 
    uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(push_constant) 
uniform PushConstants
{
    mat4 ModelTransform;
    material Material;
};

#if defined(VS)

layout(set = 1, binding = 0, std140) 
uniform Skin
{
    m4 Joints[1024];
};

invariant gl_Position;

layout(location = 0) in vec3 aP;
layout(location = 1) in vec3 aN;
layout(location = 2) in vec4 aT;
layout(location = 4) in vec2 aTexCoord;
layout(location = 5) in vec4 aColor;
layout(location = 6) in vec4 aWeights;
layout(location = 7) in uvec4 aJoints;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 N;

void main()
{
    TexCoord = aTexCoord;

    m4 J0 = Joints[aJoints[0]];
    m4 J1 = Joints[aJoints[1]];
    m4 J2 = Joints[aJoints[2]];
    m4 J3 = Joints[aJoints[3]];
    v3 P = 
        aWeights[0] * TransformPoint(J0, aP) + 
        aWeights[1] * TransformPoint(J1, aP) +
        aWeights[2] * TransformPoint(J2, aP) +
        aWeights[3] * TransformPoint(J3, aP);
    v3 N0 = 
        aWeights[0] * TransformDirection(J0, aN) +
        aWeights[1] * TransformDirection(J1, aN) +
        aWeights[2] * TransformDirection(J2, aN) +
        aWeights[3] * TransformDirection(J3, aN);
    gl_Position = PerFrame.ViewProjection * (ModelTransform * vec4(P, 1.0));
    N = normalize((PerFrame.View * (ModelTransform * vec4(N0, 0.0))).xyz);
}

#elif defined(FS)

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 N;

layout(location = 0) out vec4 Target0;

void main()
{
    f32 NdotL = dot(N, PerFrame.SunV) + 0.5;
    Target0 = vec4(NdotL.xxx, 1.0);
}

#endif