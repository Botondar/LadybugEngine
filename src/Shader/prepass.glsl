#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

interpolant(0) vec3 ViewP;
interpolant(1) flat uint InstanceIndex;
#if ShaderVariant_AlphaTest
interpolant(2) vec2 TexCoord;
#endif

#if defined(VS)

invariant gl_Position;

void main()
{
    instance_buffer_r InstanceBuffer = instance_buffer_r(PerFrame.InstanceBufferAddress);
    instance_data Instance = InstanceBuffer.Data[gl_InstanceIndex];

    vertex_buffer_r VertexBuffer = vertex_buffer_r(Instance.VertexBufferAddress);
    precise v3 WorldP = TransformPoint(Instance.Transform, VertexBuffer.Data[gl_VertexIndex].P);
    ViewP = TransformPoint(PerFrame.ViewTransform, WorldP);
    gl_Position = PerFrame.ViewProjectionTransform * vec4(WorldP, 1.0);
    InstanceIndex = gl_InstanceIndex;

#if ShaderVariant_AlphaTest
    TexCoord = VertexBuffer.Data[gl_VertexIndex].TexCoord;
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
    instance_buffer_r InstanceBuffer = instance_buffer_r(PerFrame.InstanceBufferAddress);
    instance_data Instance = InstanceBuffer.Data[InstanceIndex];
    VisibilityOut = v2u(InstanceIndex, gl_PrimitiveID);
    StructureOut = StructureEncode(ViewP.z);
#if ShaderVariant_AlphaTest
    vec4 Albedo = texture(sampler2D(Textures[Instance.Material.AlbedoID], MatSamplers[Instance.Material.AlbedoSamplerID]), TexCoord);
    if (Albedo.a < Instance.Material.AlphaThreshold)
    {
        discard;
    }
#endif
}

#endif