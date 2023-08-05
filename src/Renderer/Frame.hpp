#pragma once

struct render_frame
{
    renderer* Renderer;
    u32 RenderFrameID; // NOTE: FrameID % SwapchainImageCount

    VkExtent2D RenderExtent;

    VkSemaphore ImageAcquiredSemaphore;
    VkFence ImageAcquiredFence;
    VkFence RenderFinishedFence;

    VkCommandPool CmdPool;
    VkCommandBuffer CmdBuffer;

    u32 SwapchainImageIndex;
    VkImage SwapchainImage;
    VkImageView SwapchainImageView;

    render_target* DepthBuffer;
    render_target* StructureBuffer;
    render_target* HDRRenderTargets[2];
    render_target* OcclusionBuffers[2];

    u32 ShadowCascadeCount;
    VkImage ShadowMap;
    VkImageView ShadowMapView;
    VkImageView ShadowCascadeViews[R_MaxShadowCascadeCount];

    vulkan_buffer DrawBuffer;
    vulkan_buffer VertexStack;

    u32 DrawCount;
    VkDrawIndirectCommand* Commands;

    u32 VertexCount;
    ui_vertex* Vertices;

    VkBuffer PerFrameUniformBuffer;
    void* PerFrameUniformMemory;

    static constexpr u32 MaxDrawCmdCount = (1u << 22);
    u32 DrawCmdCount;
    draw_cmd DrawCmds[MaxDrawCmdCount];

    static constexpr u32 MaxSkinnedDrawCmdCount = (1u << 20);
    u32 SkinnedDrawCmdCount;
    draw_cmd SkinnedDrawCmds[MaxSkinnedDrawCmdCount];

    static constexpr u32 MaxParticleCount = (1u << 18);
    VkBuffer ParticleBuffer;
    u32 ParticleCount;
    render_particle* Particles;

    static constexpr u32 MaxParticleDrawCmdCount = 8192u;
    u32 ParticleDrawCmdCount;
    particle_cmd ParticleDrawCmds[MaxParticleDrawCmdCount];

    static constexpr u32 MaxSkinningCmdCount = MaxSkinnedDrawCmdCount;
    u32 SkinningCmdCount;
    skinning_cmd SkinningCmds[MaxSkinningCmdCount];

    static constexpr u32 MaxJointCount = (1u << 17);
    u32 JointCount;
    VkBuffer JointBuffer;
    m4* JointMapping;

    VkDescriptorPool DescriptorPool;

    frustum CameraFrustum;
    v3 SunV; // World space

    u32 SkinningBufferOffset;
    VkBuffer SkinningBuffer;

    /* Data that will stay the same for the entire duration of the frame
     * Also gets sent to the GPU
     */
    VkDescriptorSet UniformDescriptorSet;
    frame_uniform_data Uniforms;
};

VkDescriptorSet PushDescriptorSet(render_frame* Frame, VkDescriptorSetLayout Layout);
VkDescriptorSet PushBufferDescriptor(render_frame* Frame, 
                                     VkDescriptorSetLayout Layout,
                                     VkDescriptorType Type,
                                     VkBuffer Buffer, u64 Offset, u64 Size);
VkDescriptorSet PushImageDescriptor(render_frame* Frame, 
                                    VkDescriptorSetLayout Layout,
                                    VkDescriptorType Type,
                                    VkImageView View, VkImageLayout ImageLayout);
VkDescriptorSet PushImageDescriptor(render_frame* Frame, VkDescriptorSetLayout Layout, texture_id ID);

//
// Immediate-mode rendering
//

void RenderImmediates(render_frame* Frame, 
                           VkPipeline Pipeline, VkPipelineLayout PipelineLayout,
                           VkDescriptorSet DescriptorSet);

void PushRect(render_frame* Frame, 
                   v2 P1, v2 P2, 
                   v2 UV1, v2 UV2, 
                   rgba8 Color);
mmrect2 PushText(render_frame* Frame, const char* Text, const font* Font, 
             f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);

mmrect2 PushTextWithShadow(render_frame* Frame, const char* Text, const font* Font,                        f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);


//
// Rendering
//
void BeginPrepass(render_frame* Frame);
void EndPrepass(render_frame* Frame);

void BeginCascade(render_frame* Frame, u32 CascadeIndex);
void EndCascade(render_frame* Frame);

void RenderSSAO(render_frame* Frame,
                     ssao_params Params,
                     VkPipeline Pipeline, VkPipelineLayout PipelineLayout,
                     VkDescriptorSetLayout SetLayout,
                     VkPipeline BlurPipeline, VkPipelineLayout BlurPipelineLayout,
                     VkDescriptorSetLayout BlurSetLayout);

void BeginForwardPass(render_frame* Frame);
void EndForwardPass(render_frame* Frame);
void RenderBloom(render_frame* Frame,
                      bloom_params Params,
                      render_target* SrcRT,
                      render_target* DstRT,
                      VkPipelineLayout DownsamplePipelineLayout,
                      VkPipeline DownsamplePipeline, 
                      VkPipelineLayout UpsamplePipelineLayout,
                      VkPipeline UpsamplePipeline, 
                      VkDescriptorSetLayout DownsampleSetLayout,
                      VkDescriptorSetLayout UpsampleSetLayout);