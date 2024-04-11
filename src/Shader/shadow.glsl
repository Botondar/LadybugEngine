#include "common.glsli"

SetBindingLayout(PerFrame, Constants, scalar)
uniform PerFrameBlock
{
    per_frame PerFrame;
};

layout(buffer_reference, scalar)
buffer instance_buffer
{
    instance_data Instances[];
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

layout(location = Attrib_Position) in vec3 aP;
layout(location = Attrib_TexCoord) in vec2 aTexCoord;

void main()
{
    instance_buffer InstanceBuffer = instance_buffer(PerFrame.InstanceBufferAddress);
    instance_data Instance = InstanceBuffer.Instances[gl_InstanceIndex];
    gl_Position = ViewProjection * v4(TransformPoint(Instance.Transform, aP), 1.0);
#if ShaderVariant_AlphaTest
    TexCoord = aTexCoord;
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
    instance_buffer InstanceBuffer = instance_buffer(PerFrame.InstanceBufferAddress);
    instance_data Instance = InstanceBuffer.Instances[InstanceIndex];
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