#version 460 core

#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar) 
uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(push_constant) uniform PushConstants
{
    mat4 Transform;
    uint iSrcColor;
};

interpolant(0) vec3 P;
interpolant(1) vec3 N;
interpolant(2) vec3 SrcColor;

#if defined(VS)

layout(location = 0) in vec3 aP;
layout(location = 1) in vec3 aN;

void main()
{
    P = (PerFrame.ViewTransform * (Transform * vec4(aP, 1.0))).xyz;
    // TODO(boti): determine whether we'll need Inv/Transpose here
    N = normalize((PerFrame.ViewTransform * (Transform * vec4(aN, 0.0))).xyz);
    SrcColor = UnpackRGBA8(iSrcColor).rgb;
    gl_Position = PerFrame.ProjectionTransform * vec4(P, 1.0);
    
}

#elif defined(FS)

layout(location = 0) out vec4 OutColor;

void main()
{
    vec3 V = normalize(-P);
#if 1
    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    float ShineFactor = smoothstep(0.0, 1.0, pow(NdotV, 16.0));
    float DarkenFactor = smoothstep(0.2, 0.8, NdotV);
    vec3 Color = mix(0.5*SrcColor, SrcColor, DarkenFactor);
    Color = mix(Color, vec3(1.0, 1.0, 1.0), ShineFactor);

    OutColor = vec4(Color, 1.0);
#else
    OutColor = vec4(0.5 * N + vec3(0.5), 1.0);
#endif
}

#endif