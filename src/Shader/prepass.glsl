#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

SetBindingLayout(Static, InstanceBuffer, scalar)
readonly buffer InstanceBuffer
{
    instance_data Instances[];
};

interpolant(0) vec3 ViewP;
interpolant(1) flat uint InstanceIndex;
#if ShaderVariant_AlphaTest
interpolant(2) vec2 TexCoord;
#endif

#if defined(VS)

invariant gl_Position;

layout(location = Attrib_Position) in vec3 aP;
layout(location = Attrib_TexCoord) in vec2 aTexCoord;

void main()
{
    instance_data Instance = Instances[gl_InstanceIndex];
    precise v3 WorldP  = TransformPoint(Instance.Transform, aP);
    ViewP = TransformPoint(PerFrame.ViewTransform, WorldP);
    gl_Position = PerFrame.ViewProjectionTransform * vec4(WorldP, 1.0);
    InstanceIndex = gl_InstanceIndex;

#if ShaderVariant_AlphaTest
    TexCoord = aTexCoord;
#endif
}

#elif defined(FS)

#if ShaderVariant_AlphaTest
SetBinding(Sampler, MaterialSamplers) uniform sampler MatSamplers[R_MaterialSamplerCount];
SetBinding(Bindless, Textures) uniform texture2D Textures[];
#else
layout(early_fragment_tests) in;
#endif

layout(location = 0) out v2u VisibilityOut;
layout(location = 1) out vec4 StructureOut;

vec4 StructureEncode(in float z)
{
    float h = uintBitsToFloat(floatBitsToUint(z) & 0xFFFFE000u);
    float dx = dFdxFine(z);
    float dy = dFdyFine(z);
    vec4 Result = vec4(dx, dy, h, z - h);
    return Result;
}

void main()
{
    instance_data Instance = Instances[InstanceIndex];
    VisibilityOut = v2u(InstanceIndex, gl_PrimitiveID);
    StructureOut = StructureEncode(ViewP.z);
#if ShaderVariant_AlphaTest
    vec4 Albedo = texture(sampler2D(Textures[Instance.Material.DiffuseID], MatSamplers[Instance.Material.DiffuseSamplerID]), TexCoord);
    if (Albedo.a < Instance.Material.AlphaThreshold)
    {
        discard;
    }
#endif
}

#endif