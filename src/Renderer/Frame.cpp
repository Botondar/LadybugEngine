
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

internal VkDescriptorSet 
PushImageDescriptor(render_frame* Frame, VkDescriptorSetLayout Layout, renderer_texture_id ID)
{
    texture_manager* TextureManager = &Frame->Renderer->TextureManager;
    VkDescriptorSet Set = PushImageDescriptor(Frame, Layout,
                                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              *GetImageView(TextureManager, ID),
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return(Set);
}

//
// Rendering
//

internal void BeginPrepass(render_frame* Frame, VkCommandBuffer CmdBuffer)
{
    VkDebugUtilsLabelEXT BeginLabel = 
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = "Prepass",
        .color = {},
    };
    vkCmdBeginDebugUtilsLabelEXT(CmdBuffer, &BeginLabel);

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
            .image = Frame->Backend->StructureBuffer->Image,
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
            .image = Frame->Backend->DepthBuffer->Image,
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
        // Structure buffer
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->Backend->StructureBuffer->View,
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
        .imageView = Frame->Backend->DepthBuffer->View,
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
            .image = Frame->Backend->ShadowMap,
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
        .imageView = Frame->Backend->ShadowCascadeViews[CascadeIndex],
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

internal void 
RenderSSAO(render_frame* Frame,
           VkCommandBuffer CmdBuffer,
           ssao_params Params,
           VkPipeline Pipeline, VkPipelineLayout PipelineLayout, 
           VkDescriptorSetLayout SetLayout,
           VkPipeline BlurPipeline, VkPipelineLayout BlurPipelineLayout,
           VkDescriptorSetLayout BlurSetLayout)
{
    VkDebugUtilsLabelEXT SSAOLabel = 
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = "SSAO",
        .color = {},
    };
    vkCmdBeginDebugUtilsLabelEXT(CmdBuffer, &SSAOLabel);

    // Transition depth + G-buffers
    VkImageMemoryBarrier2 GBufferReadBarriers[] = 
    {
        // Structure buffer
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK.GraphicsQueueIdx,
            .dstQueueFamilyIndex = VK.ComputeQueueIdx,
            .image = Frame->Backend->StructureBuffer->Image,
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
    vkCmdPipelineBarrier2(CmdBuffer, &GBufferReadDependency);

    // Calculate AO
    {
        constexpr u32 GroupSizeX = 8;
        constexpr u32 GroupSizeY = 8;
        u32 DispatchX = CeilDiv(Frame->RenderWidth, GroupSizeX);
        u32 DispatchY = CeilDiv(Frame->RenderHeight, GroupSizeY);

        // Allocate the descriptor set
        VkDescriptorSet SSAODescriptorSet = VK_NULL_HANDLE;
        {
            VkDescriptorSetAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Frame->Backend->DescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &SetLayout,
            };
            VkResult Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, &SSAODescriptorSet);
            Assert(Result == VK_SUCCESS);

            VkDescriptorImageInfo StructureBufferInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->Backend->StructureBuffer->View,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorImageInfo SSAOImageInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->Backend->OcclusionBuffers[0]->View,
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
            .srcQueueFamilyIndex = VK.GraphicsQueueIdx,
            .dstQueueFamilyIndex = VK.ComputeQueueIdx,
            .image = Frame->Backend->OcclusionBuffers[0]->Image,
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

        vkCmdPipelineBarrier2(CmdBuffer, &SSAOBeginDependency);

        vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
        f32 PushConstants[3] = { Params.Intensity, 1.0f / Params.MaxDistance, Params.TangentTau };
        vkCmdPushConstants(CmdBuffer, PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), PushConstants);
        VkDescriptorSet SSAODescriptorSets[] = 
        {
            SSAODescriptorSet,
            Frame->Backend->UniformDescriptorSet,
        };
        vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout, 
                                0, CountOf(SSAODescriptorSets), SSAODescriptorSets, 0, nullptr);

        vkCmdDispatch(CmdBuffer, DispatchX, DispatchY, 1);

        VkDescriptorSet SSAOBlurDescriptorSet = VK_NULL_HANDLE;
        {
            VkDescriptorSetAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Frame->Backend->DescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &BlurSetLayout,
            };
            VkResult Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, &SSAOBlurDescriptorSet);
            Assert(Result == VK_SUCCESS);

            VkDescriptorImageInfo StructureBufferInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->Backend->StructureBuffer->View,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorImageInfo SSAOImageInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->Backend->OcclusionBuffers[0]->View,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorImageInfo SSAOOutImageInfo = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Frame->Backend->OcclusionBuffers[1]->View,
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
                .image = Frame->Backend->OcclusionBuffers[0]->Image,
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
                .srcQueueFamilyIndex = VK.GraphicsQueueIdx,
                .dstQueueFamilyIndex = VK.ComputeQueueIdx,
                .image = Frame->Backend->OcclusionBuffers[1]->Image,
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

        vkCmdPipelineBarrier2(CmdBuffer, &BlurDependency);

        vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, BlurPipeline);

        VkDescriptorSet SSAOBlurDescriptorSets[] = 
        {
            SSAOBlurDescriptorSet,
        };
        vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, BlurPipelineLayout, 
                                0, CountOf(SSAOBlurDescriptorSets), SSAOBlurDescriptorSets, 0, nullptr);

        vkCmdDispatch(CmdBuffer, DispatchX, DispatchY, 1);
    }

    vkCmdEndDebugUtilsLabelEXT(CmdBuffer);
}

internal void BeginForwardPass(render_frame* Frame, VkCommandBuffer CmdBuffer)
{
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
            .image = Frame->Backend->HDRRenderTargets[0]->Image,
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
            .image = Frame->Backend->ShadowMap,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = Frame->Backend->ShadowCascadeCount,
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
            .srcQueueFamilyIndex = VK.ComputeQueueIdx,
            .dstQueueFamilyIndex = VK.GraphicsQueueIdx,
            .image = Frame->Backend->OcclusionBuffers[1]->Image,
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
    vkCmdPipelineBarrier2(CmdBuffer, &BeginDependencyInfo);

    VkRenderingAttachmentInfo ColorAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Frame->Backend->HDRRenderTargets[0]->MipViews[0],
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
        .imageView = Frame->Backend->DepthBuffer->View,
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
        .renderArea = { .offset = { 0, 0 }, .extent = { Frame->RenderWidth, Frame->RenderHeight } },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ColorAttachment,
        .pDepthAttachment = &DepthAttachment,
        .pStencilAttachment = nullptr,
    };
    vkCmdBeginRendering(CmdBuffer, &RenderingInfo);
}

internal void EndForwardPass(render_frame* Frame, VkCommandBuffer CmdBuffer)
{
    vkCmdEndRendering(CmdBuffer);
}

internal void RenderBloom(
    render_frame* Frame,
    VkCommandBuffer CmdBuffer,
    bloom_params Params,
    render_target* SrcRT,
    render_target* DstRT,
    VkPipelineLayout DownsamplePipelineLayout,
    VkPipeline DownsamplePipeline, 
    VkPipelineLayout UpsamplePipelineLayout,
    VkPipeline UpsamplePipeline, 
    VkDescriptorSetLayout DownsampleSetLayout,
    VkDescriptorSetLayout UpsampleSetLayout)
{
    VkDebugUtilsLabelEXT BloomLabel = 
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = "Bloom",
        .color = {},
    };
    vkCmdBeginDebugUtilsLabelEXT(CmdBuffer, &BloomLabel);

    u32 Width = Frame->RenderWidth;
    u32 Height = Frame->RenderHeight;

    constexpr u32 MaxBloomMipCount = 64u;//9u;
    u32 BloomMipCount = Min(SrcRT->MipCount, MaxBloomMipCount);

    VkDescriptorSetLayout DownsampleSetLayouts[render_target::GlobalMaxMipCount];
    VkDescriptorSetLayout UpsampleSetLayouts[render_target::GlobalMaxMipCount];
    for (u32 i = 0; i < SrcRT->MipCount; i++)
    {
        DownsampleSetLayouts[i] = DownsampleSetLayout;
        UpsampleSetLayouts[i] = UpsampleSetLayout;
    }
    VkDescriptorSetAllocateInfo DownsampleAllocInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = Frame->Backend->DescriptorPool,
        .descriptorSetCount = SrcRT->MipCount,
        .pSetLayouts = DownsampleSetLayouts,
    };
    VkDescriptorSet DownsampleSets[render_target::GlobalMaxMipCount] = {};
    VkResult Result = vkAllocateDescriptorSets(VK.Device, &DownsampleAllocInfo, DownsampleSets);
    Assert(Result == VK_SUCCESS);

    VkDescriptorSetAllocateInfo UpsampleAllocInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = Frame->Backend->DescriptorPool,
        .descriptorSetCount = DstRT->MipCount, // TODO(boti): this only needs to be BloomMipCount
        .pSetLayouts = UpsampleSetLayouts,
    };
    VkDescriptorSet UpsampleSets[render_target::GlobalMaxMipCount] = {};
    Result = vkAllocateDescriptorSets(VK.Device, &UpsampleAllocInfo, UpsampleSets);
    Assert(Result == VK_SUCCESS);

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
    vkCmdPipelineBarrier2(CmdBuffer, &BeginDependencyInfo); 

    // NOTE(boti): Downsampling always goes down to the 1x1 mip, in case someone wants the average luminance of the scene
    b32 DoKarisAverage = 1;
    vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, DownsamplePipeline);
    for (u32 Mip = 1; Mip < SrcRT->MipCount; Mip++)
    {
        VkDescriptorImageInfo SourceImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = SrcRT->MipViews[Mip - 1],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo DestImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = SrcRT->MipViews[Mip],
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
        vkCmdBindDescriptorSets(CmdBuffer, 
                                VK_PIPELINE_BIND_POINT_COMPUTE, DownsamplePipelineLayout,
                                0, 1, DownsampleSets + Mip - 1, 0, nullptr);
        vkCmdPushConstants(CmdBuffer, DownsamplePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 
                           0, sizeof(DoKarisAverage), &DoKarisAverage);

        u32 DispatchX = CeilDiv(Width, 8);
        u32 DispatchY = CeilDiv(Height, 8);
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
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
                .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
                .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
                .dstAccessMask = VK_ACCESS_2_NONE,
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
                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
    f32 PushConstants[2] = { Params.FilterRadius, Params.InternalStrength };
    vkCmdPushConstants(CmdBuffer, UpsamplePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), PushConstants);
    for (u32 Index = 1; Index < BloomMipCount; Index++)
    {
        u32 Mip = BloomMipCount - Index - 1;
        VkImageMemoryBarrier2 Begin = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT|VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
        vkCmdPipelineBarrier2(CmdBuffer, &BeginDependency);

        VkDescriptorImageInfo DstMipPlus1ImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = DstRT->MipViews[Mip + 1],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo SrcImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = SrcRT->MipViews[Mip],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo DstImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = DstRT->MipViews[Mip],
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
                .pImageInfo = &DstMipPlus1ImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = UpsampleSets[Mip],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &SrcImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = UpsampleSets[Mip],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &DstImageInfo,
            },
        };

        vkUpdateDescriptorSets(VK.Device, CountOf(Writes), Writes, 0, nullptr);
        vkCmdBindDescriptorSets(CmdBuffer, 
                                VK_PIPELINE_BIND_POINT_COMPUTE, UpsamplePipelineLayout,
                                0, 1, UpsampleSets + Mip, 0, nullptr);

        Width = Max(Frame->RenderWidth >> Mip, 1u);
        Height = Max(Frame->RenderHeight >> Mip, 1u);

        u32 DispatchX = CeilDiv(Width, 8);
        u32 DispatchY = CeilDiv(Height, 8);
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
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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

    vkCmdEndDebugUtilsLabelEXT(CmdBuffer);
}
