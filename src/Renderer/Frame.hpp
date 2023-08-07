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
// Immediate-mode rendering
//
void PushRect(render_frame* Frame, 
                   v2 P1, v2 P2, 
                   v2 UV1, v2 UV2, 
                   rgba8 Color);
mmrect2 PushText(render_frame* Frame, const char* Text, const font* Font, 
             f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);

mmrect2 PushTextWithShadow(render_frame* Frame, const char* Text, const font* Font, f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);

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