#pragma once

/*
    TODO(boti): 
      The purpose of this file kind of got folded into the renderer when we decided
    that most things should go through the frame, so maybe it's time to move these?
*/

internal VkDescriptorSet 
PushDescriptorSet(render_frame* Frame, VkDescriptorSetLayout Layout);

internal VkDescriptorSet 
PushBufferDescriptor(render_frame* Frame, 
                     VkDescriptorSetLayout Layout,
                     VkDescriptorType Type,
                     VkBuffer Buffer, u64 Offset, u64 Size);

internal VkDescriptorSet 
PushImageDescriptor(render_frame* Frame, 
                    VkDescriptorSetLayout Layout,
                    VkDescriptorType Type,
                    VkImageView View, VkImageLayout ImageLayout);

internal VkDescriptorSet 
PushImageDescriptor(render_frame* Frame, VkDescriptorSetLayout Layout, renderer_texture_id ID);

//
// Rendering
//
internal void BeginPrepass(render_frame* Frame, VkCommandBuffer CmdBuffer);
internal void EndPrepass(render_frame* Frame, VkCommandBuffer CmdBuffer);

internal void BeginCascade(render_frame* Frame, VkCommandBuffer CmdBuffer, u32 CascadeIndex);
internal void EndCascade(render_frame* Frame, VkCommandBuffer CmdBuffer);

internal void BeginForwardPass(render_frame* Frame, VkCommandBuffer CmdBuffer);
internal void EndForwardPass(render_frame* Frame, VkCommandBuffer CmdBuffer);

internal void 
RenderBloom(render_frame* Frame,
            VkCommandBuffer CmdBuffer,
            render_target* SrcRT,
            render_target* DstRT,
            VkPipelineLayout DownsamplePipelineLayout,
            VkPipeline DownsamplePipeline, 
            VkPipelineLayout UpsamplePipelineLayout,
            VkPipeline UpsamplePipeline, 
            VkDescriptorSetLayout DownsampleSetLayout,
            VkDescriptorSetLayout UpsampleSetLayout);