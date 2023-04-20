#pragma once

struct render_frame
{
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

    VkDescriptorPool DescriptorPool;

    /* Data that will stay the same for the entire duration of the frame
     * Also gets sent to the GPU
     */
    VkDescriptorSet UniformDescriptorSet;
    frame_uniform_data Uniforms;
};

lbfn VkDescriptorSet PushDescriptorSet(render_frame* Frame, VkDescriptorSetLayout Layout);
lbfn VkDescriptorSet PushBufferDescriptor(render_frame* Frame, 
                                          VkDescriptorSetLayout Layout,
                                          VkDescriptorType Type,
                                          VkBuffer Buffer, u64 Offset, u64 Size);
lbfn VkDescriptorSet PushImageDescriptor(render_frame* Frame, 
                                         VkDescriptorSetLayout Layout,
                                         VkDescriptorType Type,
                                         VkImageView View, VkImageLayout ImageLayout);

//
// Immediate-mode rendering
//

lbfn void RenderImmediates(render_frame* Frame, 
                           VkPipeline Pipeline, VkPipelineLayout PipelineLayout,
                           VkDescriptorSet DescriptorSet);

lbfn void PushRect(render_frame* Frame, 
                   v2 P1, v2 P2, 
                   v2 UV1, v2 UV2, 
                   rgba8 Color);
lbfn mmrect2 PushText(render_frame* Frame, const char* Text, const font* Font, 
             f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);

lbfn mmrect2 PushTextWithShadow(render_frame* Frame, const char* Text, const font* Font,                        f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);


//
// Rendering
//
lbfn void BeginPrepass(render_frame* Frame);
lbfn void EndPrepass(render_frame* Frame);

lbfn void BeginCascade(render_frame* Frame, u32 CascadeIndex);
lbfn void EndCascade(render_frame* Frame);

lbfn void RenderSSAO(render_frame* Frame,
                     VkPipeline Pipeline, VkPipelineLayout PipelineLayout,
                     VkDescriptorSetLayout SetLayout,
                     VkPipeline BlurPipeline, VkPipelineLayout BlurPipelineLayout,
                     VkDescriptorSetLayout BlurSetLayout);

lbfn void BeginForwardPass(render_frame* Frame);
lbfn void EndForwardPass(render_frame* Frame);

lbfn void RenderBloom(render_frame* Frame, render_target* RT,
                      VkPipelineLayout DownsamplePipelineLayout,
                      VkPipeline DownsamplePipeline, 
                      VkPipelineLayout UpsamplePipelineLayout,
                      VkPipeline UpsamplePipeline, 
                      VkDescriptorSetLayout DescriptorSetLayout);