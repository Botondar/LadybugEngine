
lbfn VkDescriptorSet PushDescriptorSet(render_frame* Frame, VkDescriptorSetLayout Layout)
{
    VkDescriptorSet Set = VK_NULL_HANDLE;

    VkDescriptorSetAllocateInfo AllocInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = Frame->DescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &Layout,
    };

    VkResult Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, &Set);
    if (Result != VK_SUCCESS)
    {
        UnhandledError("Failed to push descriptor set");
    }

    return Set;
}
lbfn VkDescriptorSet PushBufferDescriptor(render_frame* Frame, 
                                          VkDescriptorSetLayout Layout,
                                          VkDescriptorType Type,
                                          VkBuffer Buffer, u64 Offset, u64 Size)
{
    VkDescriptorSet Set = PushDescriptorSet(Frame, Layout);
    if (Set)
    {
        VkDescriptorBufferInfo Info = { Buffer, Offset, Size };
        VkWriteDescriptorSet Write = 
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = Set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1 ,
            .descriptorType = Type,
            .pBufferInfo = &Info,
        };

        vkUpdateDescriptorSets(VK.Device, 1, &Write, 0, nullptr);
    }
    else
    {
        UnhandledError("Failed to push buffer descriptor set");
    }
    return Set;
}
lbfn VkDescriptorSet PushImageDescriptor(render_frame* Frame, 
                                         VkDescriptorSetLayout Layout,
                                         VkDescriptorType Type,
                                         VkImageView View, VkImageLayout ImageLayout)
{
    VkDescriptorSet Set = PushDescriptorSet(Frame, Layout);
    if (Set)
    {
        VkDescriptorImageInfo Info = { VK_NULL_HANDLE, View, ImageLayout };
        VkWriteDescriptorSet Write = 
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = Set,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = Type,
            .pImageInfo = &Info,
        };
        vkUpdateDescriptorSets(VK.Device, 1, &Write, 0, nullptr);
    }
    else
    {
        UnhandledError("Failed to push image descriptor set");
    }
    return Set;
}


lbfn void PushRect(render_frame* Frame, v2 P1, v2 P2, v2 UV1, v2 UV2, rgba8 Color)
{
    ui_vertex VertexData[] = 
    {
        { { P1.x, P1.y }, { UV1.x, UV1.y }, Color },
        { { P2.x, P1.y }, { UV2.x, UV1.y }, Color },
        { { P2.x, P2.y }, { UV2.x, UV2.y }, Color },
        { { P1.x, P1.y }, { UV1.x, UV1.y }, Color },
        { { P2.x, P2.y }, { UV2.x, UV2.y }, Color },
        { { P1.x, P2.y }, { UV1.x, UV2.y }, Color },
    };

    if (BumpBuffer_(&Frame->DrawBuffer, sizeof(VkDrawIndirectCommand)) && 
        BumpBuffer_(&Frame->VertexStack, sizeof(VertexData)))
    {
        Frame->DrawCount++;
        *Frame->Commands++ = 
        {
            .vertexCount = CountOf(VertexData),
            .instanceCount = 1,
            .firstVertex = Frame->VertexCount,
            .firstInstance = 0,
        };

        memcpy(Frame->Vertices, VertexData, sizeof(VertexData));
        Frame->Vertices += CountOf(VertexData);
        Frame->VertexCount += CountOf(VertexData);
    }
}

//
// Immediate-mode rendering
//

lbfn void RenderImmediates(render_frame* Frame, 
                           VkPipeline Pipeline, VkPipelineLayout PipelineLayout,
                           VkDescriptorSet DescriptorSet)
{
    vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

    VkViewport Viewport = 
    {
        0.0f, 0.0f,
        (f32)Frame->RenderExtent.width, (f32)Frame->RenderExtent.height,
        0.0f, 0.0f,
    };
    VkRect2D Scissor = { { 0, 0 }, Frame->RenderExtent };
    vkCmdSetViewport(Frame->CmdBuffer, 0, 1, &Viewport);
    vkCmdSetScissor(Frame->CmdBuffer, 0, 1, &Scissor);

    vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);

    VkDeviceSize VBOffset = 0;
    vkCmdBindVertexBuffers(Frame->CmdBuffer, 0, 1, &Frame->VertexStack.Buffer, &VBOffset);

    m4 Transform = M4(
        2.0f / Frame->RenderExtent.width, 0.0f, 0.0f, -1.0f,
        0.0f, 2.0f / Frame->RenderExtent.height, 0.0f, -1.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    vkCmdPushConstants(Frame->CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Transform), &Transform);
    vkCmdDrawIndirect(Frame->CmdBuffer, Frame->DrawBuffer.Buffer, 0, Frame->DrawCount, sizeof(VkDrawIndirectCommand));
}

lbfn mmrect2 PushText(render_frame* Frame, const char* Text, const font* Font, 
                      f32 Size, v2 P, rgba8 Color, 
                      font_layout_type Layout /*= font_layout_type::Baseline*/)
{
    v2 CurrentP = P;
    if (Layout == font_layout_type::Top)
    {
        CurrentP.y += Size * Font->Ascent;
    }
    else if (Layout == font_layout_type::Bottom)
    {
        CurrentP.y -= Size * Font->Descent;
    }

    for (const char* At = Text; *At; At++)
    {
        const font_glyph* Glyph = Font->Glyphs + Font->CharMapping[*At].GlyphIndex;
        if (*At == '\n')
        {
            CurrentP.y += Size * Font->BaselineDistance;
            CurrentP.x = P.x;
        }
        else
        {
            PushRect(Frame,
                     CurrentP + Size * Glyph->P0,
                     CurrentP + Size * Glyph->P1,
                     Glyph->UV0, Glyph->UV1,
                     Color);
            CurrentP.x += Size * Glyph->AdvanceX;
        }
    }

    // TODO(boti): there's no need to call GetTextRect here, we should just calculate this ourselves in the loop
    mmrect2 Result = GetTextRect(Font, Text, Layout);
    Result.Min *= Size;
    Result.Max *= Size;
    Result.Min += P;
    Result.Max += P;
    return Result;
}

lbfn mmrect2 PushTextWithShadow(render_frame* Frame, const char* Text, const font* Font, 
                                f32 Size, v2 P, rgba8 Color, 
                                font_layout_type Layout /*= font_layout_type::Baseline*/)
{
    f32 ShadowOffset = 0.075f;
    PushText(Frame, Text, Font, Size, P + Size * ShadowOffset * v2{ 1.0f, 1.0f }, PackRGBA8(0x00, 0x00, 0x00), Layout);
    mmrect2 Result = PushText(Frame, Text, Font, Size, P, Color, Layout);
    return Result;
}

//
// Rendering
//
lbfn void BeginPrepass(render_frame* Frame)
{
    VkViewport Viewport = 
    {
        0.0f, 0.0f,
        (f32)Frame->RenderExtent.width, (f32)Frame->RenderExtent.height,
        0.0f, 1.0f,
    };

    VkRect2D Scissor = { { 0, 0 }, Frame->RenderExtent };

    vkCmdSetViewport(Frame->CmdBuffer, 0, 1, &Viewport);
    vkCmdSetScissor(Frame->CmdBuffer, 0, 1, &Scissor);

    VkImageMemoryBarrier2 BeginBarriers[] = 
    {
        // Structure buffer
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->StructureBuffer->Image,
                .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
        // Depth buffer
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->DepthBuffer->Image,
                .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
    };

    VkDependencyInfo BeginDependencyInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = CountOf(BeginBarriers),
        .pImageMemoryBarriers = BeginBarriers,
    };
    vkCmdPipelineBarrier2(Frame->CmdBuffer, &BeginDependencyInfo);

    VkRenderingAttachmentInfo ColorAttachments[] = 
    {
        // Structure buffer
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = Frame->StructureBuffer->View,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .color = { 0.0f, 0.0f, Frame->Uniforms.FarZ, 0.0f } },
        }
    };

    VkRenderingAttachmentInfo DepthAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Frame->DepthBuffer->View,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .depthStencil = { 1.0f, 0 } },
    };

    VkRenderingInfo RenderingInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = { .offset = { 0, 0 }, .extent = Frame->RenderExtent },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = CountOf(ColorAttachments),
        .pColorAttachments = ColorAttachments,
        .pDepthAttachment = &DepthAttachment,
        .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(Frame->CmdBuffer, &RenderingInfo);
}

lbfn void EndPrepass(render_frame* Frame)
{
    vkCmdEndRendering(Frame->CmdBuffer);
}

lbfn void BeginCascade(render_frame* Frame, u32 CascadeIndex)
{
    VkViewport Viewport = 
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (f32)R_ShadowResolution,
        .height = (f32)R_ShadowResolution,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    VkRect2D Scissor = 
    {
        .offset = { 0, 0 },
        .extent = { R_ShadowResolution, R_ShadowResolution },
    };
    vkCmdSetViewport(Frame->CmdBuffer, 0, 1, &Viewport);
    vkCmdSetScissor(Frame->CmdBuffer, 0, 1, &Scissor);

    VkImageMemoryBarrier2 BeginBarriers[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->ShadowMap,
                .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = CascadeIndex,
                .layerCount = 1,
            },
        },
    };

    VkDependencyInfo BeginDependency = 
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .imageMemoryBarrierCount = CountOf(BeginBarriers),
        .pImageMemoryBarriers = BeginBarriers,
    };

    vkCmdPipelineBarrier2(Frame->CmdBuffer, &BeginDependency);

    VkRenderingAttachmentInfo DepthAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Frame->ShadowCascadeViews[CascadeIndex],
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .depthStencil = { 1.0, 0 } },
    };

    VkRenderingInfo RenderingInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = { { 0, 0 } , { R_ShadowResolution, R_ShadowResolution } },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 0,
        .pColorAttachments = nullptr,
        .pDepthAttachment = &DepthAttachment,
        .pStencilAttachment = nullptr,
    };
            
    vkCmdBeginRendering(Frame->CmdBuffer, &RenderingInfo);
}

lbfn void EndCascade(render_frame* Frame)
{
    vkCmdEndRendering(Frame->CmdBuffer);
}

lbfn void RenderSSAO(render_frame* Frame, 
                     VkPipeline Pipeline, VkPipelineLayout PipelineLayout, 
                     VkDescriptorSetLayout SetLayout,
                     VkPipeline BlurPipeline, VkPipelineLayout BlurPipelineLayout,
                     VkDescriptorSetLayout BlurSetLayout)
{
    // Transition depth + G-buffers
    VkImageMemoryBarrier2 GBufferReadBarriers[] = 
    {
        // Structure buffer
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->StructureBuffer->Image,
                .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
        // Depth buffer
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->DepthBuffer->Image,
                .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
    };

    VkDependencyInfo GBufferReadDependency = 
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = CountOf(GBufferReadBarriers),
        .pImageMemoryBarriers = GBufferReadBarriers,
    };
    vkCmdPipelineBarrier2(Frame->CmdBuffer, &GBufferReadDependency);

    // Calculate AO
    {
        u32 DispatchX = CeilDiv(Frame->RenderExtent.width, 8);
        u32 DispatchY = CeilDiv(Frame->RenderExtent.height, 8);

        // Allocate the descriptor set
        VkDescriptorSet SSAODescriptorSet = VK_NULL_HANDLE;
        {
            VkDescriptorSetAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Frame->DescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &SetLayout,
            };
            VkResult Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, &SSAODescriptorSet);
            Assert(Result == VK_SUCCESS);

            VkDescriptorImageInfo StructureBufferInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->StructureBuffer->View,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorImageInfo SSAOImageInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->OcclusionBuffers[0]->View,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            VkWriteDescriptorSet Writes[] = 
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstSet = SSAODescriptorSet,
                        .dstBinding = 0,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &StructureBufferInfo,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstSet = SSAODescriptorSet,
                        .dstBinding = 1,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .pImageInfo = &SSAOImageInfo,
                },
            };
            vkUpdateDescriptorSets(VK.Device, CountOf(Writes), Writes, 0, nullptr);
        }

        VkImageMemoryBarrier2 SSAOBeginBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Frame->OcclusionBuffers[0]->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        VkDependencyInfo SSAOBeginDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &SSAOBeginBarrier,
        };

        vkCmdPipelineBarrier2(Frame->CmdBuffer, &SSAOBeginDependency);

        vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);

        VkDescriptorSet SSAODescriptorSets[] = 
        {
            SSAODescriptorSet,
            Frame->UniformDescriptorSet,
        };
        vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout, 
                                0, CountOf(SSAODescriptorSets), SSAODescriptorSets, 0, nullptr);

        vkCmdDispatch(Frame->CmdBuffer, DispatchX, DispatchY, 1);

        VkDescriptorSet SSAOBlurDescriptorSet = VK_NULL_HANDLE;
        {
            VkDescriptorSetAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Frame->DescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &BlurSetLayout,
            };
            VkResult Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, &SSAOBlurDescriptorSet);
            Assert(Result == VK_SUCCESS);

            VkDescriptorImageInfo StructureBufferInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->StructureBuffer->View,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorImageInfo SSAOImageInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->OcclusionBuffers[0]->View,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorImageInfo SSAOOutImageInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->OcclusionBuffers[1]->View,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            VkWriteDescriptorSet Writes[] = 
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = SSAOBlurDescriptorSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &StructureBufferInfo,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = SSAOBlurDescriptorSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &SSAOImageInfo,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = SSAOBlurDescriptorSet,
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &SSAOOutImageInfo,
                },
            };
            vkUpdateDescriptorSets(VK.Device, CountOf(Writes), Writes, 0, nullptr);
        }

        VkImageMemoryBarrier2 BlurBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->OcclusionBuffers[0]->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->OcclusionBuffers[1]->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
        };

        VkDependencyInfo BlurDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = CountOf(BlurBarriers),
            .pImageMemoryBarriers = BlurBarriers,
        };

        vkCmdPipelineBarrier2(Frame->CmdBuffer, &BlurDependency);

        vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, BlurPipeline);

        VkDescriptorSet SSAOBlurDescriptorSets[] = 
        {
            SSAOBlurDescriptorSet,
        };
        vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, BlurPipelineLayout, 
                                0, CountOf(SSAOBlurDescriptorSets), SSAOBlurDescriptorSets, 0, nullptr);

        vkCmdDispatch(Frame->CmdBuffer, DispatchX, DispatchY, 1);
    }
}

lbfn void BeginForwardPass(render_frame* Frame)
{
    VkViewport Viewport = 
    {
        0.0f, 0.0f,
        (f32)Frame->RenderExtent.width, (f32)Frame->RenderExtent.height,
        0.0f, 1.0f,
    };
    VkRect2D Scissor = { { 0, 0 }, Frame->RenderExtent };

    vkCmdSetViewport(Frame->CmdBuffer, 0, 1, &Viewport);
    vkCmdSetScissor(Frame->CmdBuffer, 0, 1, &Scissor);

    VkImageMemoryBarrier2 BeginBarriers[] = 
    {
        // HDR RT
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Frame->HDRRenderTargets[0]->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },

        // Shadow cascades
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Frame->ShadowMap,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = Frame->ShadowCascadeCount,
            },
        },

        // SSAO
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Frame->OcclusionBuffers[1]->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
    };

    VkDependencyInfo BeginDependencyInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = CountOf(BeginBarriers),
        .pImageMemoryBarriers = BeginBarriers,
    };
    vkCmdPipelineBarrier2(Frame->CmdBuffer, &BeginDependencyInfo);

    VkRenderingAttachmentInfo ColorAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Frame->HDRRenderTargets[0]->MipViews[0],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .color = { 0.0f, 0.0f, 0.0f, 0.0f } },
    };

    VkRenderingAttachmentInfo DepthAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Frame->DepthBuffer->View,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .depthStencil = { 1.0f, 0 } },
    };

    VkRenderingInfo RenderingInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = { .offset = { 0, 0 }, .extent = Frame->RenderExtent },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ColorAttachment,
        .pDepthAttachment = &DepthAttachment,
        .pStencilAttachment = nullptr,
    };
    vkCmdBeginRendering(Frame->CmdBuffer, &RenderingInfo);
}

lbfn void EndForwardPass(render_frame* Frame)
{
    vkCmdEndRendering(Frame->CmdBuffer);
}

// TODO(boti): Figure out how to remove Renderer
//  - Renderer is needed for descriptor set layouts
lbfn void RenderBloom(render_frame* Frame, render_target* RT,
                      VkPipelineLayout PipelineLayout,
                      VkPipeline DownsamplePipeline, 
                      VkPipeline UpsamplePipeline, 
                      VkDescriptorSetLayout DescriptorSetLayout,
                      VkSampler Sampler)
{
    
    VkImageMemoryBarrier2 BeginBarriers[] = 
    {
        // HDR Mip 0
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = RT->Image,
                .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
#if 1
        // HDR Mip rest
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = RT->Image,
                .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 1,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
#endif
    };

    VkDependencyInfo BeginDependencyInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = CountOf(BeginBarriers),
        .pImageMemoryBarriers = BeginBarriers,
    };
    vkCmdPipelineBarrier2(Frame->CmdBuffer, &BeginDependencyInfo); 

    constexpr u32 MaxBloomMipCount = 100u;
    u32 BloomMipCount = Min(RT->MipCount, MaxBloomMipCount);
    VkDescriptorSetLayout SetLayouts[RT->GlobalMaxMipCount];
    for (u32 i = 0; i < BloomMipCount; i++)
    {
        SetLayouts[i] = DescriptorSetLayout;
    }
    // TODO(boti): Set for each downsample level
    VkDescriptorSetAllocateInfo AllocInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = Frame->DescriptorPool,
        .descriptorSetCount = BloomMipCount,
        .pSetLayouts = SetLayouts,
    };

    VkDescriptorSet DownsampleSets[RT->GlobalMaxMipCount] = {};
    VkResult Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, DownsampleSets);
    Assert(Result == VK_SUCCESS);

    VkDescriptorSet UpsampleSets[RT->GlobalMaxMipCount] = {};
    Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, UpsampleSets);
    Assert(Result == VK_SUCCESS);

    u32 Width = Frame->RenderExtent.width;
    u32 Height = Frame->RenderExtent.height;

    b32 DoKarisAverage = 1;
    vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, DownsamplePipeline);
    for (u32 Mip = 1; Mip < BloomMipCount; Mip++)
    {
        VkDescriptorImageInfo SourceImageInfo = 
        {
            .sampler = Sampler,
            .imageView = RT->MipViews[Mip - 1],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo DestImageInfo = 
        {
            .sampler = Sampler,
            .imageView = RT->MipViews[Mip],
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkWriteDescriptorSet Writes[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = DownsampleSets[Mip - 1],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &SourceImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = DownsampleSets[Mip - 1],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &DestImageInfo,
            },  
        };

        vkUpdateDescriptorSets(VK.Device, CountOf(Writes), Writes, 0, nullptr);
        vkCmdBindDescriptorSets(Frame->CmdBuffer, 
                                VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout,
                                0, 1, DownsampleSets + Mip - 1, 0, nullptr);
        vkCmdPushConstants(Frame->CmdBuffer, PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 
                           0, sizeof(DoKarisAverage), &DoKarisAverage);

        u32 DispatchX = CeilDiv(Width, 8);
        u32 DispatchY = CeilDiv(Height, 8);
        vkCmdDispatch(Frame->CmdBuffer, DispatchX, DispatchY, 1);

        DoKarisAverage = 0;
        Width = Max(Width >> 1, 1u);
        Height = Max(Height >> 1, 1u);

        VkImageMemoryBarrier2 MipBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = RT->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = Mip,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VkDependencyInfo Barrier = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &MipBarrier,
        };

        vkCmdPipelineBarrier2(Frame->CmdBuffer, &Barrier);
    }

    // TODO(boti): Rewrite this loop condition!
    //             It's just an (M, 0] loop, but we can't go to 0 inclusively with unsigned.
    vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, UpsamplePipeline);
    for (u32 Mip = BloomMipCount - 2; Mip < BloomMipCount; Mip--)
    {
        VkImageMemoryBarrier2 Begin = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT|VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = RT->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = Mip,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        VkDependencyInfo BeginDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &Begin,
        };
        vkCmdPipelineBarrier2(Frame->CmdBuffer, &BeginDependency);

        VkDescriptorImageInfo SourceImageInfo = 
        {
            .sampler = Sampler,
            .imageView = RT->MipViews[Mip + 1],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo DestImageInfo = 
        {
            .sampler = Sampler,
            .imageView = RT->MipViews[Mip],
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkWriteDescriptorSet Writes[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = UpsampleSets[Mip],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &SourceImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = UpsampleSets[Mip],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &DestImageInfo,
            },
        };

        vkUpdateDescriptorSets(VK.Device, CountOf(Writes), Writes, 0, nullptr);
        vkCmdBindDescriptorSets(Frame->CmdBuffer, 
                                VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout,
                                0, 1, UpsampleSets + Mip, 0, nullptr);

        Width = Max(Frame->RenderExtent.width >> Mip, 1u);
        Height = Max(Frame->RenderExtent.height >> Mip, 1u);

        u32 DispatchX = CeilDiv(Width, 8);
        u32 DispatchY = CeilDiv(Height, 8);
        vkCmdDispatch(Frame->CmdBuffer, DispatchX, DispatchY, 1);

        VkImageMemoryBarrier2 End = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT|VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = RT->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = Mip,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        VkDependencyInfo EndDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &End,
        };
        vkCmdPipelineBarrier2(Frame->CmdBuffer, &EndDependency);
    }
}
