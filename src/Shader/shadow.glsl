#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(push_constant) 
uniform PushConstants
{
    m4 ViewProjection;
};

interpolant(0) vec2 TexCoord;

#if ShaderVariant_AlphaTest
interpolant(1) flat uint InstanceIndex;
#endif

#if defined(VS)

void main()
{
    instance_buffer_r InstanceBuffer = instance_buffer_r(PerFrame.InstanceBufferAddress);
    instance_data Instance = InstanceBuffer.Data[gl_InstanceIndex];

    bool IsSkinned = gl_InstanceIndex >= PerFrame.DrawGroupOffsets[DrawGroup_Skinned];
    vertex_buffer_r VertexBuffer = vertex_buffer_r(IsSkinned ? PerFrame.SkinningBufferAddress : PerFrame.VertexBufferAddress);

    gl_Position = ViewProjection * v4(TransformPoint(Instance.Transform, VertexBuffer.Data[gl_VertexIndex].P), 1.0);
#if ShaderVariant_AlphaTest
    TexCoord = VertexBuffer.Data[gl_VertexIndex].TexCoord;
    InstanceIndex = gl_InstanceIndex;
#endif
    
}

#elif defined(FS)

#if ShaderVariant_AlphaTest
SetBindingLayout(Static, MipFeedbackBuffer, scalar)
coherent buffer MipFeedbackBuffer
{
    uint MipFeedbacks[];
};

SetBinding(Sampler, MaterialSamplers) uniform sampler MatSamplers[R_MaterialSamplerCount];
SetBinding(Bindless, Textures) uniform texture2D Textures[];

void main()
{
    instance_buffer_r InstanceBuffer = instance_buffer_r(PerFrame.InstanceBufferAddress);
    instance_data Instance = InstanceBuffer.Data[InstanceIndex];
    v4 Albedo = texture(sampler2D(Textures[Instance.Material.DiffuseID], MatSamplers[Instance.Material.DiffuseSamplerID]), TexCoord);
    uint MipBucket = GetMipBucketFromDerivatives(dFdxFine(TexCoord), dFdyFine(TexCoord));
    atomicOr(MipFeedbacks[Instance.Material.DiffuseID], MipBucket);
    if (Albedo.a < Instance.Material.AlphaThreshold)
    {
        discard;
    }
}

#else

layout(early_fragment_tests) in;

void main()
{
}

#endif

#endif