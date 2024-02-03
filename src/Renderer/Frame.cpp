
internal VkDescriptorSet PushDescriptorSet(render_frame* Frame, VkDescriptorSetLayout Layout)
{
    VkDescriptorSet Set = VK_NULL_HANDLE;

    VkDescriptorSetAllocateInfo AllocInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = Frame->Backend->DescriptorPool,
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

internal VkDescriptorSet 
PushBufferDescriptor(render_frame* Frame, 
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

internal VkDescriptorSet 
PushImageDescriptor(render_frame* Frame, 
                    VkDescriptorSetLayout Layout,
                    VkDescriptorType Type,
                    VkImageView View, 
                    VkImageLayout ImageLayout,
                    VkSampler Sampler)
{
    VkDescriptorSet Set = PushDescriptorSet(Frame, Layout);
    if (Set)
    {
        VkDescriptorImageInfo Info = { Sampler, View, ImageLayout };
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

internal VkDescriptorSet 
PushImageDescriptor(render_frame* Frame, VkDescriptorSetLayout Layout, renderer_texture_id ID, VkSampler Sampler)
{
    texture_manager* TextureManager = &Frame->Renderer->TextureManager;
    VkDescriptorSet Set = PushImageDescriptor(Frame, Layout,
                                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              *GetImageView(TextureManager, ID),
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                              Sampler);
    return(Set);
}

//
// Rendering
//

internal void BeginPrepass(render_frame* Frame, VkCommandBuffer CmdBuffer)
{
    vkCmdBeginDebugUtilsLabelEXT(CmdBuffer, "Prepass");

    VkImageMemoryBarrier2 BeginBarriers[] = 
    {
        // Visibility buffer
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
            .image = Frame->Renderer->VisibilityBuffer->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
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
            .image = Frame->Renderer->StructureBuffer->Image,
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
            .image = Frame->Renderer->DepthBuffer->Image,
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
    vkCmdPipelineBarrier2(CmdBuffer, &BeginDependencyInfo);

    VkRenderingAttachmentInfo ColorAttachments[] = 
    {
        // Visibility buffer
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->Renderer->VisibilityBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color.uint32 = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00u, 0x00u } },
        },
        // Structure buffer
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->Renderer->StructureBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = { 0.0f, 0.0f, Frame->Uniforms.FarZ, 0.0f } },
        },
    };

    VkRenderingAttachmentInfo DepthAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Frame->Renderer->DepthBuffer->View,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .depthStencil = { 0.0f, 0 } },
    };

    VkRenderingInfo RenderingInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = { .offset = { 0, 0 }, .extent = { Frame->RenderWidth, Frame->RenderHeight } },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = CountOf(ColorAttachments),
        .pColorAttachments = ColorAttachments,
        .pDepthAttachment = &DepthAttachment,
        .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(CmdBuffer, &RenderingInfo);
}

internal void EndPrepass(render_frame* Frame, VkCommandBuffer CmdBuffer)
{
    vkCmdEndRendering(CmdBuffer);
    vkCmdEndDebugUtilsLabelEXT(CmdBuffer);
}

internal void BeginCascade(render_frame* Frame, VkCommandBuffer CmdBuffer, u32 CascadeIndex)
{
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
            .image = Frame->Renderer->ShadowMap,
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

    vkCmdPipelineBarrier2(CmdBuffer, &BeginDependency);

    VkRenderingAttachmentInfo DepthAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Frame->Renderer->ShadowCascadeViews[CascadeIndex],
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
            
    vkCmdBeginRendering(CmdBuffer, &RenderingInfo);
}

internal void EndCascade(render_frame* Frame, VkCommandBuffer CmdBuffer)
{
    vkCmdEndRendering(CmdBuffer);
}

internal void RenderBloom(
    render_frame* Frame,
    VkCommandBuffer CmdBuffer,
    render_target* SrcRT,
    render_target* DstRT,
    VkPipelineLayout DownsamplePipelineLayout,
    VkPipeline DownsamplePipeline, 
    VkPipelineLayout UpsamplePipelineLayout,
    VkPipeline UpsamplePipeline, 
    VkDescriptorSetLayout DownsampleSetLayout,
    VkDescriptorSetLayout UpsampleSetLayout)
{
    vkCmdBeginDebugUtilsLabelEXT(CmdBuffer, "Bloom");

    u32 Width = Frame->RenderWidth;
    u32 Height = Frame->RenderHeight;

    constexpr u32 MaxBloomMipCount = 64u;//9u;
    u32 BloomMipCount = Min(SrcRT->MipCount, MaxBloomMipCount);
    VkImageMemoryBarrier2 BeginBarriers[] = 
    {
        // HDR Mip 0
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = SrcRT->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
        // HDR mips
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
            .image = SrcRT->Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 1,
                .levelCount = VK_REMAINING_MIP_LEVELS,
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
    vkCmdPipelineBarrier2(CmdBuffer, &BeginDependencyInfo); 

    // NOTE(boti): Downsampling always goes down to the 1x1 mip, in case someone wants the average luminance of the scene
    b32 DoKarisAverage = 1;
    vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, DownsamplePipeline);
    for (u32 Mip = 1; Mip < SrcRT->MipCount; Mip++)
    {
        vkCmdPushConstants(CmdBuffer, DownsamplePipelineLayout, VK_SHADER_STAGE_ALL, 
                           0, sizeof(Mip), &Mip);

        u32 DispatchX = CeilDiv(Width, BloomDownsample_GroupSizeX);
        u32 DispatchY = CeilDiv(Height, BloomDownsample_GroupSizeY);
        vkCmdDispatch(CmdBuffer, DispatchX, DispatchY, 1);

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
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = SrcRT->Image,
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

        vkCmdPipelineBarrier2(CmdBuffer, &Barrier);
    }

    // Copy highest mip to the bloom chain
    {
        u32 MipIndex = BloomMipCount - 1;

        VkImageMemoryBarrier2 CopyBeginImageBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = SrcRT->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = MipIndex,
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
                .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = DstRT->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = MipIndex,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
        };
        VkDependencyInfo CopyBeginBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = CountOf(CopyBeginImageBarriers),
            .pImageMemoryBarriers = CopyBeginImageBarriers,
        };
        vkCmdPipelineBarrier2(CmdBuffer, &CopyBeginBarrier);

        VkImageCopy CopyRegion = 
        {
            .srcSubresource = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = MipIndex,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffset = { 0, 0, 0 },
            .dstSubresource = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = MipIndex,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffset = { 0, 0, 0 },
            .extent = 
            {
                .width = Max(Frame->RenderWidth >> MipIndex, 1u),
                .height = Max(Frame->RenderHeight >> MipIndex, 1u),
                .depth = 1,
            },
        };

        vkCmdCopyImage(CmdBuffer,
                       SrcRT->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       DstRT->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &CopyRegion);

        VkImageMemoryBarrier2 CopyEndImageBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = SrcRT->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = MipIndex,
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
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = SrcRT->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = MipIndex,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = DstRT->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = MipIndex,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = DstRT->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = MipIndex,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
        };
        VkDependencyInfo CopyEndBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = CountOf(CopyEndImageBarriers),
            .pImageMemoryBarriers = CopyEndImageBarriers,
        };
        vkCmdPipelineBarrier2(CmdBuffer, &CopyEndBarrier);
    }

    vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, UpsamplePipeline);
    for (u32 Index = 1; Index < BloomMipCount; Index++)
    {
        u32 Mip = BloomMipCount - Index - 1;
        vkCmdPushConstants(CmdBuffer, UpsamplePipelineLayout, VK_SHADER_STAGE_ALL,
                           0, sizeof(Mip), &Mip);
        Width = Max(Frame->RenderWidth >> Mip, 1u);
        Height = Max(Frame->RenderHeight >> Mip, 1u);

        u32 DispatchX = CeilDiv(Width, BloomUpsample_GroupSizeX);
        u32 DispatchY = CeilDiv(Height, BloomUpsample_GroupSizeY);
        vkCmdDispatch(CmdBuffer, DispatchX, DispatchY, 1);

        VkImageMemoryBarrier2 End = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT|VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = DstRT->Image,
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
        vkCmdPipelineBarrier2(CmdBuffer, &EndDependency);
    }

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
        .image = DstRT->Image,
        .subresourceRange = 
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
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
    vkCmdPipelineBarrier2(CmdBuffer, &EndDependency);

    vkCmdEndDebugUtilsLabelEXT(CmdBuffer);
}
