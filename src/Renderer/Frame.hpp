#pragma once

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