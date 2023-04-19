#include "VulkanRenderer.hpp"

#define ReturnOnFailure() if (Result != VK_SUCCESS) return Result

internal VkResult CreateAndAllocateBuffer(VkBufferUsageFlags Usage, u32 MemoryTypes, size_t Size, 
                                          VkBuffer* pBuffer, VkDeviceMemory* pMemory)
{
    VkResult Result = VK_SUCCESS;

    VkBufferCreateInfo BufferInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = Size,
        .usage = Usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkBuffer Buffer = VK_NULL_HANDLE;
    if ((Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Buffer)) == VK_SUCCESS)
    {
        VkMemoryRequirements MemoryRequirements = {};
        vkGetBufferMemoryRequirements(VK.Device, Buffer, &MemoryRequirements);
        Assert(MemoryRequirements.size == Size);
        MemoryTypes &= MemoryRequirements.memoryTypeBits;

        u32 MemoryTypeIndex;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = Size,
                .memoryTypeIndex = MemoryTypeIndex,
            };

            VkDeviceMemory Memory = VK_NULL_HANDLE;
            if ((Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Memory)) == VK_SUCCESS)
            {
                if ((Result = vkBindBufferMemory(VK.Device, Buffer, Memory, 0)) == VK_SUCCESS)
                {
                    *pBuffer = Buffer;
                    Buffer = VK_NULL_HANDLE;
                    *pMemory = Memory;
                    Memory = VK_NULL_HANDLE;
                }
            }

            vkFreeMemory(VK.Device, Memory, nullptr);
        }
    }

    vkDestroyBuffer(VK.Device, Buffer, nullptr);
    return Result;
}

internal VkResult CreateComputeShader(const char* Path, memory_arena* TempArena, VkPipelineLayout PipelineLayout);

lbfn VkResult CreateRenderer(vulkan_renderer* Renderer, 
                             memory_arena* Arena, 
                             memory_arena* TempArena)
{
    VkResult Result = VK_SUCCESS;

    {
        VkCommandPoolCreateInfo PoolInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = VK.GraphicsQueueIdx,
        };
        Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->CmdPools[0]);
        ReturnOnFailure();

        Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->CmdPools[1]);
        ReturnOnFailure();

        VkCommandBufferAllocateInfo BufferInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = Renderer->CmdPools[0],
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->CmdBuffers[0]);
        ReturnOnFailure();
        BufferInfo.commandPool = Renderer->CmdPools[1];
        Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->CmdBuffers[1]);
        ReturnOnFailure();
    }

    {
        VkCommandPoolCreateInfo PoolInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = VK.GraphicsQueueIdx,
        };
        Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->TransferCmdPool);
        ReturnOnFailure();

        VkCommandBufferAllocateInfo BufferInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = Renderer->TransferCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->TransferCmdBuffer);
        ReturnOnFailure();
    }

    // Surface
    {
        Renderer->Surface = Platform.CreateVulkanSurface(VK.Instance);
        if (!Renderer->Surface) return VK_ERROR_UNKNOWN;

        constexpr u32 MaxSurfaceFormatCount = 32;
        u32 SurfaceFormatCount = MaxSurfaceFormatCount;
        VkSurfaceFormatKHR SurfaceFormats[MaxSurfaceFormatCount];
        Result = vkGetPhysicalDeviceSurfaceFormatsKHR(VK.PhysicalDevice, Renderer->Surface, &SurfaceFormatCount, SurfaceFormats);
        ReturnOnFailure();

        for (u32 i = 0; i < SurfaceFormatCount; i++)
        {
            if (SurfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                SurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                Renderer->SurfaceFormat = SurfaceFormats[i];
            }
        }

        Renderer->SwapchainImageCount = Renderer->MaxSwapchainImageCount;
        Result = ResizeRenderTargets(Renderer);
        ReturnOnFailure();
    }

    for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
    {
        VkFenceCreateInfo ImageFenceInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        Result = vkCreateFence(VK.Device, &ImageFenceInfo, nullptr, &Renderer->ImageAcquiredFences[i]);
        ReturnOnFailure();

        VkFenceCreateInfo RenderFenceInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        Result = vkCreateFence(VK.Device, &RenderFenceInfo, nullptr, &Renderer->RenderFinishedFences[i]);
        ReturnOnFailure();

        VkSemaphoreCreateInfo ImageAcquiredSemaphoreInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        Result = vkCreateSemaphore(VK.Device, &ImageAcquiredSemaphoreInfo, nullptr, &Renderer->ImageAcquiredSemaphores[i]);
        ReturnOnFailure();
    }

    // Frame uniform descriptor
    {
        VkDescriptorSetLayoutBinding Bindings[] = 
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            },
        };

        VkDescriptorSetLayoutCreateInfo LayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = CountOf(Bindings),
            .pBindings = Bindings,
        };
        Result = vkCreateDescriptorSetLayout(VK.Device, &LayoutInfo, nullptr, &Renderer->FrameUniformDescriptorSetLayout);
        ReturnOnFailure();
    }

    // Font
    {
        // Font sampler
        {
            VkSamplerCreateInfo SamplerInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .mipLodBias = 0.0f,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 1.0f,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0.0f,
                .maxLod = 0.0f,
                .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
                .unnormalizedCoordinates = VK_FALSE,
            };
            Result = vkCreateSampler(VK.Device, &SamplerInfo, nullptr, &Renderer->FontSampler);
            ReturnOnFailure();
        }

        // Font Descriptor
        {
            VkDescriptorPoolSize PoolSize = 
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
            };

            VkDescriptorPoolCreateInfo PoolInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = &PoolSize,
            };
            Result = vkCreateDescriptorPool(VK.Device, &PoolInfo, nullptr, &Renderer->FontDescriptorPool);
            ReturnOnFailure();

            VkDescriptorSetLayoutBinding Binding = 
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = &Renderer->FontSampler,
            };
            VkDescriptorSetLayoutCreateInfo SetLayoutInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = 1,
                .pBindings = &Binding,
            };
            Result = vkCreateDescriptorSetLayout(VK.Device, &SetLayoutInfo, nullptr, &Renderer->FontDescriptorSetLayout);
            ReturnOnFailure();

            VkDescriptorSetAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Renderer->FontDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &Renderer->FontDescriptorSetLayout,
            };
            Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, &Renderer->FontDescriptorSet);
            ReturnOnFailure();
        }
    }

    // Staging Buffer
    {
        size_t MemorySize = MiB(128);
        if ((Result = CreateAndAllocateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK.SharedMemTypes, MemorySize, 
                &Renderer->StagingBuffer.Buffer, &Renderer->StagingBuffer.Memory)) == VK_SUCCESS)
        {
            Renderer->StagingBuffer.Size = MemorySize;
            
            Result = vkMapMemory(VK.Device, Renderer->StagingBuffer.Memory, 
                                 0, Renderer->StagingBuffer.Size, 0, 
                                 &Renderer->StagingBuffer.Mapping);
            ReturnOnFailure();
        }
        ReturnOnFailure();
    }

    // Vertex Buffer
    {
        if (!CreateGeometryBuffer(R_VertexBufferMaxBlockCount, Arena, &Renderer->GeometryBuffer))
        {
            Result = VK_ERROR_INITIALIZATION_FAILED;
            ReturnOnFailure();
        }
    }

    // Texture Manager
    if (!CreateTextureManager(&Renderer->TextureManager, R_TextureMemorySize, VK.GPUMemTypes))
    {
        Result = VK_ERROR_INITIALIZATION_FAILED;
        ReturnOnFailure();
    }

    
    // Per frame
    {
        // Draw buffer
        for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
        {
            VkBufferUsageFlags Usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            size_t Size = MiB(8);
            Result = CreateAndAllocateBuffer(Usage, VK.SharedMemTypes, Size, 
                                             &Renderer->DrawBuffers[i].Buffer, &Renderer->DrawBuffers[i].Memory);
            if (Result == VK_SUCCESS)
            {
                Renderer->DrawBuffers[i].Size = Size;
                Result = vkMapMemory(VK.Device, Renderer->DrawBuffers[i].Memory, 0, VK_WHOLE_SIZE, 0, &Renderer->DrawBuffers[i].Mapping);
                ReturnOnFailure();
            }
            else
            {
                return Result;
            }
        }

        // Vertex stack
        for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
        {
            VkBufferUsageFlags Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            size_t Size = MiB(32);
            Result = CreateAndAllocateBuffer(Usage, VK.SharedMemTypes, Size, 
                                             &Renderer->VertexStacks[i].Buffer, &Renderer->VertexStacks[i].Memory);
            if (Result == VK_SUCCESS)
            {
                Renderer->VertexStacks[i].Size = Size;
                Result = vkMapMemory(VK.Device, Renderer->VertexStacks[i].Memory, 0, VK_WHOLE_SIZE, 0, &Renderer->VertexStacks[i].Mapping);
                ReturnOnFailure();
            }
            else
            {
                return Result;
            }
        }

        // Per frame uniform data
        {
            constexpr u64 BufferSize = KiB(64);
            u32 MemoryTypes = VK.BARMemTypes;
            for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
            {
                VkBufferCreateInfo BufferInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .size = BufferSize,
                    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = 0,
                    .pQueueFamilyIndices = nullptr,
                };
                Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Renderer->PerFrameUniformBuffers[i]);
                ReturnOnFailure();

                VkMemoryRequirements MemoryRequirements = {};
                vkGetBufferMemoryRequirements(VK.Device, Renderer->PerFrameUniformBuffers[i], &MemoryRequirements);
                Assert((BufferSize % MemoryRequirements.alignment) == 0);

                MemoryTypes &= MemoryRequirements.memoryTypeBits;
            }

            
            u32 MemoryTypeIndex = 0;
            if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
            {
                VkMemoryAllocateInfo AllocInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .allocationSize =  Renderer->SwapchainImageCount * BufferSize,
                    .memoryTypeIndex = MemoryTypeIndex,
                };
                Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->PerFrameUniformMemory);
                ReturnOnFailure();

                Result = vkMapMemory(VK.Device, Renderer->PerFrameUniformMemory, 
                                     0, VK_WHOLE_SIZE, 
                                     0, &Renderer->PerFrameUniformMappingBase);
                ReturnOnFailure();

                for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                {
                    u64 Offset = i * BufferSize;
                    Result = vkBindBufferMemory(VK.Device, Renderer->PerFrameUniformBuffers[i], 
                                                Renderer->PerFrameUniformMemory, Offset);
                    ReturnOnFailure();

                    Renderer->PerFrameUniformBufferMappings[i] = (void*)((u8*)Renderer->PerFrameUniformMappingBase + Offset);
                }
            }
            else
            {
                UnhandledError("No memory type for uniform buffers");
            }
        }

        // Per frame descriptors
        for (u32 i = 0; i < 2; i++)
        {
            VkDescriptorPoolSize PoolSizes[] = 
            {
                {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = Renderer->MaxPerFrameDescriptorSetCount,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = Renderer->MaxPerFrameDescriptorSetCount,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = Renderer->MaxPerFrameDescriptorSetCount,
                },
            };

            VkDescriptorPoolCreateInfo PoolInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                .maxSets = 256,
                .poolSizeCount = CountOf(PoolSizes),
                .pPoolSizes = PoolSizes,
            };

            Result = vkCreateDescriptorPool(VK.Device, &PoolInfo, nullptr, &Renderer->PerFrameDescriptorPool[i]);
            ReturnOnFailure();
        }
    }

    // Shadow pipeline
    {
        u32 MemoryTypeIndex = 0;
        {
            VkImageCreateInfo DummyImageInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = SHADOW_FORMAT,
                .extent = { R_ShadowResolution, R_ShadowResolution, 1 },
                .mipLevels = 1,
                .arrayLayers = R_MaxShadowCascadeCount,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            VkImage DummyImage = VK_NULL_HANDLE;
            Result = vkCreateImage(VK.Device, &DummyImageInfo, nullptr, &DummyImage);
            ReturnOnFailure();

            VkMemoryRequirements DummyMemoryRequirements;
            vkGetImageMemoryRequirements(VK.Device, DummyImage, &DummyMemoryRequirements);

            u32 MemoryTypes = DummyMemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
            if (!BitScanForward(&MemoryTypeIndex, MemoryTypes))
            {
                UnhandledError("No suitable memory type for shadow maps");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            vkDestroyImage(VK.Device, DummyImage, nullptr);
        }

        Renderer->ShadowMemorySize = R_ShadowMapMemorySize;
        VkMemoryAllocateInfo AllocInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = Renderer->ShadowMemorySize,
            .memoryTypeIndex = MemoryTypeIndex,
        };
        Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->ShadowMemory);
        ReturnOnFailure();

        Renderer->ShadowCascadeCount = R_MaxShadowCascadeCount;
        for (u32 FrameIndex = 0; FrameIndex < Renderer->SwapchainImageCount; FrameIndex++)
        {
            VkImageCreateInfo ImageInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = SHADOW_FORMAT,
                .extent = { R_ShadowResolution, R_ShadowResolution, 1 },
                .mipLevels = 1,
                .arrayLayers = R_MaxShadowCascadeCount,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            Result = vkCreateImage(VK.Device, &ImageInfo, nullptr, &Renderer->ShadowMaps[FrameIndex]);
            ReturnOnFailure();

            VkMemoryRequirements MemoryRequirements;
            vkGetImageMemoryRequirements(VK.Device, Renderer->ShadowMaps[FrameIndex], &MemoryRequirements);

            size_t Offset = Align(Renderer->ShadowMemoryOffset, MemoryRequirements.alignment);
            if (Offset + MemoryRequirements.size <= Renderer->ShadowMemorySize)
            {
                Result = vkBindImageMemory(VK.Device, Renderer->ShadowMaps[FrameIndex], Renderer->ShadowMemory, Offset);
                ReturnOnFailure();

                Renderer->ShadowMemoryOffset = Offset + MemoryRequirements.size;
            }
            else
            {
                return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            }

            VkImageViewCreateInfo ViewInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = Renderer->ShadowMaps[FrameIndex],
                .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .format = SHADOW_FORMAT,
                .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                },
            };

            Result = vkCreateImageView(VK.Device, &ViewInfo, nullptr,
                                       &Renderer->ShadowViews[FrameIndex]);
            ReturnOnFailure();

            for (u32 Cascade = 0; Cascade < Renderer->ShadowCascadeCount; Cascade++)
            {
                VkImageViewCreateInfo CascadeViewInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = Renderer->ShadowMaps[FrameIndex],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = SHADOW_FORMAT,
                    .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
                    .subresourceRange = 
                    {
                        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = Cascade,
                        .layerCount = 1,
                    },
                };

                Result = vkCreateImageView(VK.Device, &CascadeViewInfo, nullptr, 
                                           &Renderer->ShadowCascadeViews[FrameIndex][Cascade]);
                ReturnOnFailure();
            }
        }

        VkSamplerCreateInfo SamplerInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.0f,
            .compareEnable = VK_TRUE,
            .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };

        Result = vkCreateSampler(VK.Device, &SamplerInfo, nullptr, &Renderer->ShadowSampler);
        ReturnOnFailure();

        VkDescriptorSetLayoutBinding Bindings[] = 
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,
                .pImmutableSamplers = &Renderer->ShadowSampler,
            },
        };

        VkDescriptorSetLayoutCreateInfo SetLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = CountOf(Bindings),
            .pBindings = Bindings,
        };
        Result = vkCreateDescriptorSetLayout(VK.Device, &SetLayoutInfo, nullptr, &Renderer->ShadowDescriptorSetLayout);
        ReturnOnFailure();

        VkDescriptorSetLayout SetLayouts[] = 
        {
            Renderer->TextureManager.DescriptorSetLayouts[0],
            Renderer->TextureManager.DescriptorSetLayouts[1],
            Renderer->FrameUniformDescriptorSetLayout,
        };

        VkPushConstantRange PushConstants[] = 
        {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(push_constants) + sizeof(u32),
            },
        };

        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = CountOf(SetLayouts),
            .pSetLayouts = SetLayouts,
            .pushConstantRangeCount = CountOf(PushConstants),
            .pPushConstantRanges = PushConstants,
        };
        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Renderer->ShadowPipelineLayout);
        ReturnOnFailure();

        Result = CreateGraphicsPipeline(TempArena,
                                        &ShadowRenderPipeline::Info, &ShadowRenderPipeline::RenderPass,
                                        "build/shadow",
                                        Renderer->ShadowPipelineLayout,
                                        &Renderer->ShadowPipeline);
        ReturnOnFailure();
    }

    // Common sampler for post-processing passes
    {
        VkSamplerCreateInfo SamplerInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            .unnormalizedCoordinates = VK_TRUE,
        };
        Result = vkCreateSampler(VK.Device, &SamplerInfo, nullptr, &Renderer->RenderTargetSampler);
    }

    // Simple pipeline
    {
        ReturnOnFailure();
        VkDescriptorSetLayoutBinding Bindings[] = 
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = &Renderer->RenderTargetSampler,
            },
        };
        VkDescriptorSetLayoutCreateInfo SetLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = CountOf(Bindings),
            .pBindings = Bindings,
        };
        Result = vkCreateDescriptorSetLayout(VK.Device, &SetLayoutInfo, nullptr, &Renderer->GBufferDescriptorSetLayout);
        ReturnOnFailure();

        VkPushConstantRange PushConstantRanges[] = 
        {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(push_constants),
            },
        };

        const VkDescriptorSetLayout DescriptorSetLayouts[] = 
        {
            Renderer->TextureManager.DescriptorSetLayouts[0],
            Renderer->TextureManager.DescriptorSetLayouts[1],
            Renderer->FrameUniformDescriptorSetLayout,
            Renderer->GBufferDescriptorSetLayout,
            Renderer->GBufferDescriptorSetLayout,
            Renderer->ShadowDescriptorSetLayout,
        };

        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = CountOf(DescriptorSetLayouts),
            .pSetLayouts = DescriptorSetLayouts,
            .pushConstantRangeCount = CountOf(PushConstantRanges),
            .pPushConstantRanges = PushConstantRanges,
        };

        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, Renderer->Allocator, &Renderer->PipelineLayout);
        ReturnOnFailure();

        Result = CreateGraphicsPipeline(
            TempArena, 
            &SimpleRenderPipeline::Info, &SimpleRenderPipeline::RenderPassInfo, 
            "build/shader",
            Renderer->PipelineLayout, &Renderer->Pipeline);
        ReturnOnFailure();
    }

    // Prepass pipeline
    {
        Result = CreateGraphicsPipeline(
            TempArena,
            &PrepassRenderPipeline::Info, &PrepassRenderPipeline::RenderPass,
            "build/prepass",
            Renderer->PipelineLayout, &Renderer->PrepassPipeline);
        ReturnOnFailure();
    }

    // SSAO pipeline
    {
        // Descriptor set layout
        {
            VkDescriptorSetLayoutBinding Bindings[] = 
            {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = &Renderer->RenderTargetSampler,
                },
                {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = VK_NULL_HANDLE,
                },
            };

            VkDescriptorSetLayoutCreateInfo SetLayoutInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = CountOf(Bindings),
                .pBindings = Bindings,
            };

            Result = vkCreateDescriptorSetLayout(VK.Device, &SetLayoutInfo, nullptr, &Renderer->SSAO.SetLayout);
            ReturnOnFailure();
        }

        buffer CS = Platform.LoadEntireFile("build/ssao.cs", TempArena);

        VkShaderModuleCreateInfo ModuleInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = CS.Size,
            .pCode = (u32*)CS.Data,
        };

        VkShaderModule Module = VK_NULL_HANDLE;
        Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &Module);
        ReturnOnFailure();

        VkDescriptorSetLayout DescriptorSetLayouts[] = 
        {
            Renderer->SSAO.SetLayout,
            Renderer->FrameUniformDescriptorSetLayout,
        };

        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = CountOf(DescriptorSetLayouts),
            .pSetLayouts = DescriptorSetLayouts,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr, // TODO
        };

        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Renderer->SSAO.Layout);
        ReturnOnFailure();

        VkComputePipelineCreateInfo Info = 
        {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = Module,
                .pName = "main",
                .pSpecializationInfo = nullptr,
            },
            .layout = Renderer->SSAO.Layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
        };

        Result = vkCreateComputePipelines(VK.Device, VK_NULL_HANDLE, 1, &Info, nullptr, &Renderer->SSAO.Pipeline);
        ReturnOnFailure();

        vkDestroyShaderModule(VK.Device, Module, nullptr);
    }

    // SSAO blur pipeline
    {
        // Descriptor set layout
        {
            VkDescriptorSetLayoutBinding Bindings[] = 
            {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = &Renderer->RenderTargetSampler,
                },
                {
                    .binding = 1, 
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = &Renderer->RenderTargetSampler,
                },
                {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = VK_NULL_HANDLE,
                },
            };

            VkDescriptorSetLayoutCreateInfo SetLayoutInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = CountOf(Bindings),
                .pBindings = Bindings,
            };

            Result = vkCreateDescriptorSetLayout(VK.Device, &SetLayoutInfo, nullptr, &Renderer->SSAO.BlurSetLayout);
            ReturnOnFailure();
        }

        buffer CS = Platform.LoadEntireFile("build/ssao_blur.cs", TempArena);

        VkShaderModuleCreateInfo ModuleInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = CS.Size,
            .pCode = (u32*)CS.Data,
        };

        VkShaderModule Module = VK_NULL_HANDLE;
        Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &Module);
        ReturnOnFailure();

        VkDescriptorSetLayout DescriptorSetLayouts[] = 
        {
            Renderer->SSAO.BlurSetLayout,
        };

        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = CountOf(DescriptorSetLayouts),
            .pSetLayouts = DescriptorSetLayouts,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };

        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Renderer->SSAO.BlurLayout);
        ReturnOnFailure();

        VkComputePipelineCreateInfo Info = 
        {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = Module,
                .pName = "main",
                .pSpecializationInfo = nullptr,
            },
            .layout = Renderer->SSAO.BlurLayout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
        };

        Result = vkCreateComputePipelines(VK.Device, VK_NULL_HANDLE, 1, &Info, nullptr, &Renderer->SSAO.BlurPipeline);
        ReturnOnFailure();

        vkDestroyShaderModule(VK.Device, Module, nullptr);
    }

    // UI Pipeline
    {
        VkPushConstantRange PushConstants = 
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(m4),
        };

        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &Renderer->FontDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &PushConstants,
        };
        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Renderer->UIPipelineLayout);
        ReturnOnFailure();

        render_pass_info RenderPassInfo = 
        {
            .ColorAttachmentCount = 1,
            .ColorAttachments = { Renderer->SurfaceFormat.format },
            .DepthAttachment = DEPTH_FORMAT,
            .StencilAttachment = VK_FORMAT_UNDEFINED,
        };
        Result = CreateGraphicsPipeline(
            TempArena,
            &UIRenderPipeline::Info, &RenderPassInfo, 
            "build/ui",
            Renderer->UIPipelineLayout, &Renderer->UIPipeline);
        ReturnOnFailure();
    }

    // Sky pipeline
    {
        VkPipelineLayoutCreateInfo LayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &Renderer->FrameUniformDescriptorSetLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };
        Result = vkCreatePipelineLayout(VK.Device, &LayoutInfo, nullptr, &Renderer->SkyPipelineLayout);
        ReturnOnFailure();

        Result = CreateGraphicsPipeline(TempArena, 
                                        &SkyRenderPipeline::Info, &SkyRenderPipeline::RenderPassInfo, "build/sky", 
                                        Renderer->SkyPipelineLayout, &Renderer->SkyPipeline);
        ReturnOnFailure();
    }

    // Bloom pipeline
    {
        VkSamplerCreateInfo SamplerInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor = {},
            .unnormalizedCoordinates = VK_FALSE,
        };
        Result = vkCreateSampler(VK.Device, &SamplerInfo, nullptr, &Renderer->Bloom.Sampler);
        ReturnOnFailure();

        VkDescriptorSetLayoutBinding Bindings[] = 
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .pImmutableSamplers = &Renderer->Bloom.Sampler,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .pImmutableSamplers = nullptr,
            },
        };
        VkDescriptorSetLayoutCreateInfo SetLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = CountOf(Bindings),
            .pBindings = Bindings,
        };
        Result = vkCreateDescriptorSetLayout(VK.Device, &SetLayoutInfo, nullptr, &Renderer->Bloom.SetLayout);
        ReturnOnFailure();

        VkPushConstantRange PushConstants = 
        {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = 4,
        };
        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &Renderer->Bloom.SetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &PushConstants,
        };

        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Renderer->Bloom.Layout);
        ReturnOnFailure();

        // Downsample
        {
            buffer CS = Platform.LoadEntireFile("build/downsample_bloom.cs", TempArena);
            Assert(CS.Data);

            VkShaderModuleCreateInfo ShaderInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = CS.Size,
                .pCode = (u32*)CS.Data,
            };

            VkShaderModule Module = VK_NULL_HANDLE;
            Result = vkCreateShaderModule(VK.Device, &ShaderInfo, nullptr, &Module);
            ReturnOnFailure();

            VkComputePipelineCreateInfo PipelineInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                    .module = Module,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
                },
                .layout = Renderer->Bloom.Layout,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = 0,
            };

            Result = vkCreateComputePipelines(VK.Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, 
                                              &Renderer->Bloom.DownsamplePipeline);
            ReturnOnFailure();

            vkDestroyShaderModule(VK.Device, Module, nullptr);
        }

        // Upsample
        {
            buffer CS = Platform.LoadEntireFile("build/upsample_bloom.cs", TempArena);
            Assert(CS.Data);

            VkShaderModuleCreateInfo ShaderInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = CS.Size,
                .pCode = (u32*)CS.Data,
            };

            VkShaderModule Module = VK_NULL_HANDLE;
            Result = vkCreateShaderModule(VK.Device, &ShaderInfo, nullptr, &Module);
            ReturnOnFailure();

            VkComputePipelineCreateInfo PipelineInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                    .module = Module,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
                },
                .layout = Renderer->Bloom.Layout,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = 0,
            };

            Result = vkCreateComputePipelines(VK.Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, 
                                              &Renderer->Bloom.UpsamplePipeline);
            ReturnOnFailure();

            vkDestroyShaderModule(VK.Device, Module, nullptr);
        }
    }

    // Gizmo pipeline
    {
        VkPushConstantRange PushConstants = 
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(m4) + sizeof(u32),
        };
        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &Renderer->FrameUniformDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &PushConstants,
        };

        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Renderer->GizmoPipelineLayout);
        ReturnOnFailure();

        render_pass_info RenderPassInfo = 
        {
            .ColorAttachmentCount = 1,
            .ColorAttachments = { Renderer->SurfaceFormat.format },
            .DepthAttachment = DEPTH_FORMAT,
            .StencilAttachment = VK_FORMAT_UNDEFINED,
        };
        Result = CreateGraphicsPipeline(
            TempArena,
            &GizmoRenderPipeline::Info, &RenderPassInfo,
            "build/gizmo",
            Renderer->GizmoPipelineLayout, &Renderer->GizmoPipeline);
        ReturnOnFailure();
    }

    // Blit pipeline
    {
        // Descriptor
        {
            VkSamplerCreateInfo SamplerInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .mipLodBias = 0.0f,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 1.0f,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0.0f,
                .maxLod = VK_LOD_CLAMP_NONE,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                .unnormalizedCoordinates = VK_FALSE,
            };
            Result = vkCreateSampler(VK.Device, &SamplerInfo, nullptr, &Renderer->BlitSampler);
            ReturnOnFailure();

            VkDescriptorSetLayoutBinding Binding = 
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = &Renderer->BlitSampler,
            };
            VkDescriptorSetLayoutCreateInfo LayoutInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
                .bindingCount = 1,
                .pBindings = &Binding,
            };
            Result = vkCreateDescriptorSetLayout(VK.Device, &LayoutInfo, nullptr, &Renderer->BlitDescriptorSetLayout);
            ReturnOnFailure();
        }

        VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &Renderer->BlitDescriptorSetLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };
        Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Renderer->BlitPipelineLayout);
        ReturnOnFailure();

        render_pass_info RenderPassInfo = 
        {
            .ColorAttachmentCount = 1,
            .ColorAttachments = { Renderer->SurfaceFormat.format },
            .DepthAttachment = DEPTH_FORMAT,
            .StencilAttachment = VK_FORMAT_UNDEFINED,
        };
        Result = CreateGraphicsPipeline(
            TempArena,
            &BlitRenderPipeline::Info, &RenderPassInfo, 
            "build/blit",
            Renderer->BlitPipelineLayout, &Renderer->BlitPipeline);
        ReturnOnFailure();
    }

    // Pipelines
    {
        Renderer->Pipelines[Pipeline_None] = { VK_NULL_HANDLE, VK_NULL_HANDLE };

        constexpr u64 MaxPathSize = 512;
        char Path[MaxPathSize];
        u64 PathSize = MaxPathSize;
        CopyZStringToBuffer(Path, "build/", &PathSize);
        char* Name = Path + (MaxPathSize - PathSize);

        for (u32 PipelineIndex = 1; PipelineIndex < Pipeline_Count; PipelineIndex++)
        {
            memory_arena_checkpoint Checkpoint = ArenaCheckpoint(TempArena);

            pipeline_with_layout* Pipeline = Renderer->Pipelines + PipelineIndex;
            const graphics_pipeline_info* Info = PipelineInfos + PipelineIndex;

            // HACK(boti): we're currently piggy backing off the old way of creating pipelines
            switch (PipelineIndex)
            {
                case Pipeline_Simple:
                {
                    Pipeline->Layout = Renderer->PipelineLayout;
                } break;
                case Pipeline_Prepass:
                {
                    Pipeline->Layout = Renderer->PipelineLayout;
                } break;
                case Pipeline_Shadow:
                {
                    Pipeline->Layout = Renderer->ShadowPipelineLayout;
                } break;
                case Pipeline_Gizmo:
                {
                    Pipeline->Layout = Renderer->GizmoPipelineLayout;
                } break;
                case Pipeline_Sky:
                {
                    Pipeline->Layout = Renderer->SkyPipelineLayout;
                } break;
                case Pipeline_UI:
                {
                    Pipeline->Layout = Renderer->UIPipelineLayout;
                } break;
                case Pipeline_Blit:
                {
                    Pipeline->Layout = Renderer->BlitPipelineLayout;
                } break;
            }

            u64 NameSize = PathSize;
            CopyZStringToBuffer(Name, Info->Name, &NameSize);
            char* Extension = Path + (MaxPathSize - NameSize);

            // Only VS and PS is supported for now
            if (Info->EnabledStages & ~(GfxPipelineStage_VS|GfxPipelineStage_PS)) UnimplementedCodePath;

            VkPipelineShaderStageCreateInfo StageInfos[GfxPipelineStage_Count] = {};
            u32 StageCount = 0;

            if (Info->EnabledStages & GfxPipelineStage_VS)
            {
                VkPipelineShaderStageCreateInfo* StageInfo = StageInfos + StageCount++;
                StageInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                StageInfo->stage = VK_SHADER_STAGE_VERTEX_BIT;
                StageInfo->pName = "main";

                u64 Size = NameSize;
                CopyZStringToBuffer(Extension, ".vs", &Size);
                buffer Bin = Platform.LoadEntireFile(Path, TempArena);
                if (Bin.Size)
                {
                    VkShaderModuleCreateInfo ModuleInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = Bin.Size,
                        .pCode = (u32*)Bin.Data,
                    };
                    Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &StageInfo->module);
                    ReturnOnFailure();
                }
                else
                {
                    return VK_ERROR_INITIALIZATION_FAILED;
                }
            }
            if (Info->EnabledStages & GfxPipelineStage_PS)
            {
                VkPipelineShaderStageCreateInfo* StageInfo = StageInfos + StageCount++;
                StageInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                StageInfo->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                StageInfo->pName = "main";

                u64 Size = NameSize;
                CopyZStringToBuffer(Extension, ".fs", &Size);
                buffer Bin = Platform.LoadEntireFile(Path, TempArena);
                if (Bin.Size)
                {
                    VkShaderModuleCreateInfo ModuleInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = Bin.Size,
                        .pCode = (u32*)Bin.Data,
                    };
                    Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &StageInfo->module);
                    ReturnOnFailure();
                }
                else
                {
                    return VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            VkVertexInputBindingDescription VertexBindings[MaxVertexInputBindingCount] = {};
            VkVertexInputAttributeDescription VertexAttribs[MaxVertexInputAttribCount] = {};
            for (u32 BindingIndex = 0; BindingIndex < Info->InputAssemblerState.BindingCount; BindingIndex++)
            {
                const vertex_input_binding* Binding = Info->InputAssemblerState.Bindings + BindingIndex;
                VertexBindings[BindingIndex].binding = BindingIndex;
                VertexBindings[BindingIndex].stride = Binding->Stride;
                VertexBindings[BindingIndex].inputRate = Binding->IsPerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            }

            for (u32 AttribIndex = 0; AttribIndex < Info->InputAssemblerState.AttribCount; AttribIndex++)
            {
                const vertex_input_attrib* Attrib = Info->InputAssemblerState.Attribs + AttribIndex;
                VertexAttribs[AttribIndex].location = Attrib->Index;
                VertexAttribs[AttribIndex].binding = Attrib->BindingIndex;
                VertexAttribs[AttribIndex].format = Attrib->Format;
                VertexAttribs[AttribIndex].offset = Attrib->Offset;
            }

            VkPipelineVertexInputStateCreateInfo VertexInputState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .vertexBindingDescriptionCount = Info->InputAssemblerState.BindingCount,
                .pVertexBindingDescriptions = VertexBindings,
                .vertexAttributeDescriptionCount = Info->InputAssemblerState.AttribCount,
                .pVertexAttributeDescriptions = VertexAttribs,
            };

            VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .topology = Info->InputAssemblerState.PrimitiveTopology,
                .primitiveRestartEnable = Info->InputAssemblerState.EnablePrimitiveRestart,
            };

            VkViewport Viewport = {};
            VkRect2D Scissor = {};
            VkPipelineViewportStateCreateInfo ViewportState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .viewportCount = 1,
                .pViewports = &Viewport,
                .scissorCount = 1,
                .pScissors = &Scissor,
            };

            VkPipelineRasterizationStateCreateInfo RasterizationState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext = nullptr,
                .depthClampEnable = HasFlag(Info->RasterizerState.Flags, RS_DepthClampEnable),
                .rasterizerDiscardEnable = HasFlag(Info->RasterizerState.Flags, RS_DiscardEnable),
                .polygonMode = Info->RasterizerState.PolygonMode,
                .cullMode = Info->RasterizerState.CullFlags,
                .frontFace = Info->RasterizerState.FrontFace,
                .depthBiasEnable = HasFlag(Info->RasterizerState.Flags, RS_DepthBiasEnable),
                .depthBiasConstantFactor = Info->RasterizerState.DepthBiasConstantFactor,
                .depthBiasClamp = Info->RasterizerState.DepthBiasClamp,
                .depthBiasSlopeFactor = Info->RasterizerState.DepthBiasConstantFactor,
                .lineWidth = 1.0f,
            };

            VkPipelineMultisampleStateCreateInfo MultisampleState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
                .minSampleShading = 0.0f,
                .pSampleMask = nullptr,
                .alphaToCoverageEnable = VK_FALSE,
                .alphaToOneEnable = VK_FALSE,
            };

            VkPipelineDepthStencilStateCreateInfo DepthStencilState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .depthTestEnable = HasFlag(Info->DepthStencilState.Flags, DS_DepthTestEnable),
                .depthWriteEnable = HasFlag(Info->DepthStencilState.Flags, DS_DepthWriteEnable),
                .depthCompareOp = Info->DepthStencilState.DepthCompareOp,
                .depthBoundsTestEnable = HasFlag(Info->DepthStencilState.Flags, DS_DepthBoundsTestEnable),
                .stencilTestEnable = HasFlag(Info->DepthStencilState.Flags, DS_StencilTestEnable),
                .front = Info->DepthStencilState.StencilFront,
                .back = Info->DepthStencilState.StencilBack,
                .minDepthBounds = Info->DepthStencilState.MinDepthBounds,
                .maxDepthBounds = Info->DepthStencilState.MaxDepthBounds,
            };

            VkPipelineColorBlendAttachmentState BlendAttachments[MaxColorAttachmentCount] = {};
            for (u32 BlendAttachmentIndex = 0; BlendAttachmentIndex < Info->BlendAttachmentCount; BlendAttachmentIndex++)
            {
                const blend_attachment* Src = Info->BlendAttachments + BlendAttachmentIndex;
                BlendAttachments[BlendAttachmentIndex] = 
                {
                    .blendEnable = Src->BlendEnable,
                    .srcColorBlendFactor = Src->SrcColorFactor,
                    .dstColorBlendFactor = Src->DstColorFactor,
                    .colorBlendOp = Src->ColorOp,
                    .srcAlphaBlendFactor = Src->SrcAlphaFactor,
                    .dstAlphaBlendFactor = Src->DstAlphaFactor,
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT,
                };
            }

            VkPipelineColorBlendStateCreateInfo BlendState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_CLEAR,
                .attachmentCount = Info->BlendAttachmentCount,
                .pAttachments = BlendAttachments,
                .blendConstants = { 1.0f, 1.0f, 1.0f, 1.0f },
            };

            VkDynamicState DynamicStates[] = 
            {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
            };
            VkPipelineDynamicStateCreateInfo DynamicState = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .dynamicStateCount = CountOf(DynamicStates),
                .pDynamicStates = DynamicStates,
            };

            VkFormat ColorAttachmentFormats[MaxColorAttachmentCount] = {};
            for (u32 ColorAttachmentIndex = 0; ColorAttachmentIndex < Info->ColorAttachmentCount; ColorAttachmentIndex++)
            {
                VkFormat Format = Info->ColorAttachments[ColorAttachmentIndex];
                if (Format == SWAPCHAIN_FORMAT) Format = Renderer->SurfaceFormat.format;
                ColorAttachmentFormats[ColorAttachmentIndex] = Format;
            }

            VkPipelineRenderingCreateInfo DynamicRendering = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .pNext = nullptr,
                .viewMask = 0,
                .colorAttachmentCount = Info->ColorAttachmentCount,
                .pColorAttachmentFormats = ColorAttachmentFormats,
                .depthAttachmentFormat = Info->DepthAttachment,
                .stencilAttachmentFormat = Info->StencilAttachment
            };

            VkGraphicsPipelineCreateInfo PipelineCreateInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &DynamicRendering,
                .flags = 0,
                .stageCount = StageCount,
                .pStages = StageInfos,
                .pVertexInputState = &VertexInputState,
                .pInputAssemblyState = &InputAssemblyState,
                .pTessellationState = nullptr,
                .pViewportState = &ViewportState,
                .pRasterizationState = &RasterizationState,
                .pMultisampleState = &MultisampleState,
                .pDepthStencilState = &DepthStencilState,
                .pColorBlendState = &BlendState,
                .pDynamicState = &DynamicState,
                .layout = Pipeline->Layout,
                .renderPass = VK_NULL_HANDLE,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = 0,
            };

            Result = vkCreateGraphicsPipelines(VK.Device, nullptr, 1, &PipelineCreateInfo, nullptr, &Pipeline->Pipeline);
            Assert(Result == VK_SUCCESS);

            for (u32 StageIndex = 0; StageIndex < StageCount; StageIndex++)
            {
                VkPipelineShaderStageCreateInfo* Stage = StageInfos + StageIndex;
                vkDestroyShaderModule(VK.Device, Stage->module, nullptr);
            }

            RestoreArena(TempArena, Checkpoint);
        }
    }

    return Result;
}

lbfn VkResult ResizeRenderTargets(vulkan_renderer* Renderer)
{
    VkResult Result = VK_SUCCESS;

    VkSurfaceCapabilitiesKHR SurfaceCaps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VK.PhysicalDevice, Renderer->Surface, &SurfaceCaps);

    if (Renderer->SurfaceExtent.width != SurfaceCaps.currentExtent.width ||
        Renderer->SurfaceExtent.height != SurfaceCaps.currentExtent.height)
    {
        Renderer->SurfaceExtent = SurfaceCaps.currentExtent;
        if (Renderer->SurfaceExtent.width != 0 && Renderer->SurfaceExtent.height != 0)
        {
            if (Renderer->Swapchain)
            {
                Renderer->Debug.ResizeCount++;
                vkDeviceWaitIdle(VK.Device);

                for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                {
                    vkDestroyImageView(VK.Device, Renderer->SwapchainImageViews[i], nullptr);
                    Renderer->SwapchainImageViews[i] = VK_NULL_HANDLE;
                }
            }
            else
            {
                if (!CreateRenderTargetHeap(&Renderer->RenderTargetHeap, R_RenderTargetMemorySize))
                {
                    return VK_ERROR_INITIALIZATION_FAILED;
                }

                for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                {
                    render_frame* Frame = Renderer->Frames + i;
                    constexpr VkImageUsageFlagBits DepthStencil = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                    constexpr VkImageUsageFlagBits Color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                    constexpr VkImageUsageFlagBits Sampled = VK_IMAGE_USAGE_SAMPLED_BIT;
                    constexpr VkImageUsageFlagBits Storage = VK_IMAGE_USAGE_STORAGE_BIT;
                    
#if 1
                    if (i == 0)
                    {
                        Frame->DepthBuffer = PushRenderTarget(&Renderer->RenderTargetHeap, DEPTH_FORMAT, DepthStencil|Sampled, 1);
                        Frame->StructureBuffer = PushRenderTarget(&Renderer->RenderTargetHeap, STRUCTURE_BUFFER_FORMAT, Color|Sampled, 1);
                        Frame->HDRRenderTargets[0] = PushRenderTarget(&Renderer->RenderTargetHeap, HDR_FORMAT, Color|Sampled|Storage, 0);
                        Frame->HDRRenderTargets[1] = PushRenderTarget(&Renderer->RenderTargetHeap, HDR_FORMAT, Color|Sampled|Storage, 0);
                        Frame->OcclusionBuffers[0] = PushRenderTarget(&Renderer->RenderTargetHeap, SSAO_FORMAT, Color|Sampled|Storage, 1);
                        Frame->OcclusionBuffers[1] = PushRenderTarget(&Renderer->RenderTargetHeap, SSAO_FORMAT, Color|Sampled|Storage, 1);
                    }
                    else
                    {
                        Frame->DepthBuffer = Renderer->Frames[0].DepthBuffer;
                        Frame->StructureBuffer = Renderer->Frames[0].StructureBuffer;
                        Frame->HDRRenderTargets[0] = Renderer->Frames[0].HDRRenderTargets[0];
                        Frame->HDRRenderTargets[1] = Renderer->Frames[0].HDRRenderTargets[1];
                        Frame->OcclusionBuffers[0] = Renderer->Frames[0].OcclusionBuffers[0];
                        Frame->OcclusionBuffers[1] = Renderer->Frames[0].OcclusionBuffers[1];
                    }
#else
                    Frame->DepthBuffer = PushRenderTarget(&Renderer->RenderTargetHeap, DEPTH_FORMAT, DepthStencil|Sampled, 1);
                    Frame->StructureBuffer = PushRenderTarget(&Renderer->RenderTargetHeap, STRUCTURE_BUFFER_FORMAT, Color|Sampled, 1);
                    Frame->HDRRenderTargets[0] = PushRenderTarget(&Renderer->RenderTargetHeap, HDR_FORMAT, Color|Sampled|Storage, 0);
                    Frame->HDRRenderTargets[1] = PushRenderTarget(&Renderer->RenderTargetHeap, HDR_FORMAT, Color|Sampled|Storage, 0);
                    Frame->OcclusionBuffers[0] = PushRenderTarget(&Renderer->RenderTargetHeap, SSAO_FORMAT, Color|Sampled|Storage, 1);
                    Frame->OcclusionBuffers[1] = PushRenderTarget(&Renderer->RenderTargetHeap, SSAO_FORMAT, Color|Sampled|Storage, 1);
#endif
                }
            }

            if (!ResizeRenderTargets(&Renderer->RenderTargetHeap, Renderer->SurfaceExtent.width, Renderer->SurfaceExtent.height))
            {
                UnhandledError("Failed to resize render targets");
                Result = VK_ERROR_UNKNOWN;
            }

            VkPresentModeKHR PresentMode = 
                //VK_PRESENT_MODE_FIFO_KHR
                VK_PRESENT_MODE_MAILBOX_KHR
                //VK_PRESENT_MODE_IMMEDIATE_KHR
                ;

            VkSwapchainCreateInfoKHR CreateInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0/*|VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR*/,
                .surface = Renderer->Surface,
                .minImageCount = Renderer->SwapchainImageCount,
                .imageFormat = Renderer->SurfaceFormat.format,
                .imageColorSpace = Renderer->SurfaceFormat.colorSpace,
                .imageExtent = SurfaceCaps.currentExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &VK.GraphicsQueueIdx,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = PresentMode,
                .clipped = VK_FALSE,
                .oldSwapchain = Renderer->Swapchain,
            };
            Result = vkCreateSwapchainKHR(VK.Device, &CreateInfo, nullptr, &Renderer->Swapchain);
            if (Result == VK_SUCCESS)
            {
                Result = vkGetSwapchainImagesKHR(VK.Device, Renderer->Swapchain, &Renderer->SwapchainImageCount, Renderer->SwapchainImages);
                if (Result == VK_SUCCESS)
                {
                    for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                    {
                        VkImageViewCreateInfo ViewInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .image = Renderer->SwapchainImages[i],
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = Renderer->SurfaceFormat.format,
                            .components = {},
                            .subresourceRange = 
                            {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            },
                        };

                        Result = vkCreateImageView(VK.Device, &ViewInfo, nullptr, &Renderer->SwapchainImageViews[i]);
                        if (Result != VK_SUCCESS)
                        {
                            break;
                        }
                    }
                }
            }

            if (CreateInfo.oldSwapchain)
            {
                vkDestroySwapchainKHR(VK.Device, CreateInfo.oldSwapchain, nullptr);
            }
        }
    }
    return Result;
}

lbfn VkResult CreateGraphicsPipeline(memory_arena* Arena,
                                       const render_pipeline_info* BaseInfo, 
                                       const render_pass_info* RenderPassInfo,
                                       const char* ShaderName,
                                       VkPipelineLayout PipelineLayout,
                                       VkPipeline* Pipeline)
{
    VkResult Result = VK_SUCCESS;

    constexpr size_t PathBufferSize = 256;
    char PathBuffer[PathBufferSize];
    char* PathBufferAt = PathBuffer;
    size_t RemainingPathBufferSize = PathBufferSize;
    strncpy(PathBufferAt, ShaderName, RemainingPathBufferSize);
    {
        size_t Length = strlen(PathBuffer);
        RemainingPathBufferSize -= Length;
        PathBufferAt += Length;
    }

    strncpy(PathBufferAt, ".vs", RemainingPathBufferSize);
    buffer VSBin = Platform.LoadEntireFile(PathBuffer, Arena);
    strncpy(PathBufferAt, ".fs", RemainingPathBufferSize);
    buffer FSBin = Platform.LoadEntireFile(PathBuffer, Arena);

    Assert(VSBin.Data && FSBin.Data);


    VkShaderModuleCreateInfo VSInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = VSBin.Size,
        .pCode = (uint32_t*)VSBin.Data,
    };
    VkShaderModule VS = VK_NULL_HANDLE;
    Result = vkCreateShaderModule(VK.Device, &VSInfo, nullptr, &VS);
    ReturnOnFailure();

    VkShaderModuleCreateInfo FSInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = FSBin.Size,
        .pCode = (uint32_t*)FSBin.Data,
    };
    VkShaderModule FS = VK_NULL_HANDLE;
    Result = vkCreateShaderModule(VK.Device, &FSInfo, nullptr, &FS);
    ReturnOnFailure();

    VkPipelineShaderStageCreateInfo ShaderStages[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = VS,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = FS,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    };

    VkViewport Viewport = {};
    VkRect2D Scissor = {};
    VkPipelineViewportStateCreateInfo ViewportState = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &Viewport,
        .scissorCount = 1,
        .pScissors = &Scissor,
    };

    VkPipelineMultisampleStateCreateInfo MultisampleState = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo BlendState = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_CLEAR,
        .attachmentCount = BaseInfo->BlendAttachmentCount,
        .pAttachments = BaseInfo->BlendAttachments,
        .blendConstants = { 1.0f, 1.0f, 1.0f, 1.0f },
    };
    
    VkDynamicState DynamicStates[] = 
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo DynamicState = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = CountOf(DynamicStates),
        .pDynamicStates = DynamicStates,
    };

    VkPipelineRenderingCreateInfo DynamicRendering = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = RenderPassInfo->ColorAttachmentCount,
        .pColorAttachmentFormats = RenderPassInfo->ColorAttachments,
        .depthAttachmentFormat = RenderPassInfo->DepthAttachment,
        .stencilAttachmentFormat = RenderPassInfo->StencilAttachment,
    };

    VkGraphicsPipelineCreateInfo PipelineInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &DynamicRendering,
        .flags = 0,
        .stageCount = CountOf(ShaderStages),
        .pStages = ShaderStages,
        .pVertexInputState = &BaseInfo->VertexInputState,
        .pInputAssemblyState = &BaseInfo->InputAssemblyState,
        .pTessellationState = nullptr,
        .pViewportState = &ViewportState,
        .pRasterizationState = &BaseInfo->RasterizationState,
        .pMultisampleState = &MultisampleState,
        .pDepthStencilState = &BaseInfo->DepthStencilState,
        .pColorBlendState = &BlendState,
        .pDynamicState = &DynamicState,
        .layout = PipelineLayout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    Result = vkCreateGraphicsPipelines(VK.Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, Pipeline);

    vkDestroyShaderModule(VK.Device, VS, nullptr);
    vkDestroyShaderModule(VK.Device, FS, nullptr);
    return Result;
}

lbfn geometry_buffer_allocation UploadVertexData(vulkan_renderer* Renderer, 
                                                     u32 VertexCount, const vertex* VertexData,
                                                     u32 IndexCount, const vert_index* IndexData)
{
    geometry_buffer_allocation Allocation = AllocateVertexBuffer(&Renderer->GeometryBuffer, VertexCount, IndexCount);

    if (VertexCount)
    {
        Assert(Allocation.VertexBlock);

        void* Mapping = Renderer->StagingBuffer.Mapping;
        memcpy(OffsetPtr(Mapping, Renderer->StagingBuffer.Offset), VertexData, Allocation.VertexBlock->ByteSize);

        VkBufferCopy VertexRegion = 
        {
            .srcOffset = Renderer->StagingBuffer.Offset,
            .dstOffset = Allocation.VertexBlock->ByteOffset,
            .size = Allocation.VertexBlock->ByteSize,
        };
        // TODO(boti): support for multiple concurrent uploads
        //Renderer->StagingBuffer.Offset += Size;

        VkCommandBufferBeginInfo BeginInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        vkBeginCommandBuffer(Renderer->TransferCmdBuffer, &BeginInfo);
        vkCmdCopyBuffer(Renderer->TransferCmdBuffer, 
                        Renderer->StagingBuffer.Buffer, 
                        Renderer->GeometryBuffer.VertexMemory.Buffer, 
                        1, &VertexRegion);
        if (IndexCount)
        {
            Assert(Allocation.IndexBlock);
            u64 IndexOffset = Allocation.VertexBlock->ByteSize;

            memcpy(OffsetPtr(Mapping, Renderer->StagingBuffer.Offset + IndexOffset), IndexData, Allocation.IndexBlock->ByteSize);

            VkBufferCopy IndexRegion = 
            {
                .srcOffset = Renderer->StagingBuffer.Offset + IndexOffset,
                .dstOffset = Allocation.IndexBlock->ByteOffset,
                .size = Allocation.IndexBlock->ByteSize,
            };

            vkCmdCopyBuffer(Renderer->TransferCmdBuffer, 
                            Renderer->StagingBuffer.Buffer, 
                            Renderer->GeometryBuffer.IndexMemory.Buffer,
                            1, &IndexRegion);
        }

        vkEndCommandBuffer(Renderer->TransferCmdBuffer);

        VkSubmitInfo SubmitInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &Renderer->TransferCmdBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        vkQueueSubmit(VK.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VK.GraphicsQueue);

        vkResetCommandPool(VK.Device, Renderer->TransferCmdPool, 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    }

    return Allocation;
}

#undef ReturnOnFailure

lbfn void CreateDebugFontImage(vulkan_renderer* Renderer, u32 Width, u32 Height, const void* Texels)
{
    VkImageCreateInfo ImageInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8_UNORM,
        .extent = { Width, Height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage Image = VK_NULL_HANDLE;
    if (vkCreateImage(VK.Device, &ImageInfo, nullptr, &Image) == VK_SUCCESS)
    {
        VkMemoryRequirements MemoryRequirements = {};
        vkGetImageMemoryRequirements(VK.Device, Image, &MemoryRequirements);
        u32 MemoryTypes = MemoryRequirements.memoryTypeBits & VK.GPUMemTypes;

        u32 MemoryTypeIndex = 0;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = MemoryRequirements.size,
                .memoryTypeIndex = MemoryTypeIndex,
            };

            VkDeviceMemory Memory = VK_NULL_HANDLE;
            if (vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Memory) == VK_SUCCESS)
            {
                if (vkBindImageMemory(VK.Device, Image, Memory, 0) == VK_SUCCESS)
                {
                    VkImageViewCreateInfo ViewInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .image = Image,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = VK_FORMAT_R8_UNORM,
                        .components = 
                        {
                            VK_COMPONENT_SWIZZLE_ONE,
                            VK_COMPONENT_SWIZZLE_ONE,
                            VK_COMPONENT_SWIZZLE_ONE,
                            VK_COMPONENT_SWIZZLE_R,
                        },
                        .subresourceRange = 
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                    };

                    VkImageView ImageView = VK_NULL_HANDLE;
                    if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, &ImageView) == VK_SUCCESS)
                    {
                        size_t BitmapMemorySize = (size_t)Width * Height;
                        memcpy(Renderer->StagingBuffer.Mapping, Texels, BitmapMemorySize);
                        
                        VkCommandBuffer CmdBuffer = Renderer->TransferCmdBuffer;
                        VkCommandBufferBeginInfo BeginInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                            .pNext = nullptr,
                            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                            .pInheritanceInfo = nullptr,
                        };

                        vkBeginCommandBuffer(CmdBuffer, &BeginInfo);

                        VkBufferImageCopy Region = 
                        {
                            .bufferOffset = 0,
                            .bufferRowLength = 0,
                            .bufferImageHeight = 0,
                            .imageSubresource = 
                            {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .mipLevel = 0,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            },
                            .imageOffset = { 0, 0, 0 },
                            .imageExtent = { Width, Height, 1 },
                        };

                        VkImageMemoryBarrier BeginBarrier = 
                        {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                            .pNext = nullptr,
                            .srcAccessMask = 0,
                            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                            .image = Image,
                            .subresourceRange = 
                            {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            },
                        };
                        vkCmdPipelineBarrier(CmdBuffer, 
                            VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                            0, 
                            0, nullptr,
                            0, nullptr,
                            1, &BeginBarrier);
                        vkCmdCopyBufferToImage(CmdBuffer, Renderer->StagingBuffer.Buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

                        VkImageMemoryBarrier EndBarrier = 
                        {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                            .pNext = nullptr,
                            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                            .image = Image,
                            .subresourceRange = 
                            {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            },
                        };
                        vkCmdPipelineBarrier(CmdBuffer,
                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 
                            0, nullptr,
                            0, nullptr,
                            1, &EndBarrier);

                        vkEndCommandBuffer(CmdBuffer);

                        VkSubmitInfo SubmitInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .pNext = nullptr,
                            .waitSemaphoreCount = 0,
                            .pWaitSemaphores = nullptr,
                            .pWaitDstStageMask = nullptr,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &CmdBuffer,
                            .signalSemaphoreCount = 0,
                            .pSignalSemaphores = nullptr,
                        };
                        vkQueueSubmit(VK.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
                        vkQueueWaitIdle(VK.GraphicsQueue);

                        VkDescriptorImageInfo ImageDescriptor = 
                        {
                            .sampler = Renderer->FontSampler,
                            .imageView = ImageView,
                            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                        };
                        VkWriteDescriptorSet WriteDescriptorSet = 
                        {
                            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                            .pNext = nullptr,
                            .dstSet = Renderer->FontDescriptorSet,
                            .dstBinding = 0,
                            .dstArrayElement = 0,
                            .descriptorCount = 1,
                            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .pImageInfo = &ImageDescriptor,
                        };
                        vkUpdateDescriptorSets(VK.Device, 1, &WriteDescriptorSet, 0, nullptr);

                        Renderer->FontImageMemory = Memory;
                        Renderer->FontImage = Image;
                        Renderer->FontImageView = ImageView;
                    }
                    else
                    {
                        UnhandledError("Failed to create font image view");
                    }
                }
                else
                {
                    UnhandledError("Failed to bind font image memory");
                }
            }
            else
            {
                UnhandledError("Failed to allocate font image memory");
            }
        }
        else
        {
            UnhandledError("Failed to find suitable memory type for font image");
        }
    }
    else
    {
        UnhandledError("Failed to create font image");
    }
}

lbfn texture_id PushTexture(vulkan_renderer* Renderer, u32 Width, u32 Height, u32 MipCount, const void* Data, VkFormat Format)
{
    texture_id Result = { INVALID_INDEX_U32 };

    texture_manager* TextureManager = &Renderer->TextureManager;

    texture_byte_rate ByteRate = GetByteRate(Format);

    Result = CreateTexture(TextureManager, Width, Height, MipCount, Format);
    if (IsValid(Result))
    {
        VkImage Image = GetImage(TextureManager, Result);
        u64 MemorySize = GetMipChainSize(Width, Height, MipCount, ByteRate);

        vulkan_buffer* StagingBuffer = &Renderer->StagingBuffer;
        Assert(MemorySize <= StagingBuffer->Size);

        VkCommandBufferBeginInfo BeginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        vkBeginCommandBuffer(Renderer->TransferCmdBuffer, &BeginInfo);

        VkImageMemoryBarrier BeginBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = MipCount,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        vkCmdPipelineBarrier(Renderer->TransferCmdBuffer, 
                             VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &BeginBarrier);

        u8* Mapping = (u8*)StagingBuffer->Mapping;
        memcpy(Mapping, Data, MemorySize);

        u64 CopyOffset = 0;
        for (u32 i = 0; i < MipCount; i++)
        {
            u32 CurrentWidth = Max(Width >> i, 1u);
            u32 CurrentHeight = Max(Height >> i, 1u);

            u32 RowLength = 0;
            u32 ImageHeight = 0;
            u64 TexelCount;
            u64 MipSize;
            if (ByteRate.IsBlock)
            {
                RowLength = Align(CurrentWidth, 4u);
                ImageHeight = Align(CurrentHeight, 4u);

                TexelCount = (u64)RowLength * ImageHeight;
            }
            else
            {
                TexelCount = CurrentWidth * CurrentHeight;
            }
            MipSize = TexelCount * ByteRate.Numerator / ByteRate.Denominator;

            VkBufferImageCopy CopyRegion = 
            {
                .bufferOffset = CopyOffset,
                .bufferRowLength = RowLength,
                .bufferImageHeight = ImageHeight,
                .imageSubresource = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = { CurrentWidth, CurrentHeight, 1 },
            };
            vkCmdCopyBufferToImage(Renderer->TransferCmdBuffer, StagingBuffer->Buffer, Image, 
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);

            CopyOffset += MipSize;
        }
        VkImageMemoryBarrier EndBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = MipCount,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        vkCmdPipelineBarrier(Renderer->TransferCmdBuffer, 
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &EndBarrier);
        vkEndCommandBuffer(Renderer->TransferCmdBuffer);

        VkSubmitInfo SubmitInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &Renderer->TransferCmdBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        vkQueueSubmit(VK.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VK.GraphicsQueue);

        vkResetCommandPool(VK.Device, Renderer->TransferCmdPool, 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    }
    
    return Result;
}

//
// Rendering interface implementation
//

internal bool BumpBuffer_(vulkan_buffer* Buffer, size_t Size)
{
    bool Result = false;
    if (Buffer->Offset + Size <= Buffer->Size)
    {
        Buffer->Offset += Size;
        Result = true;
    }
    else
    {
        UnhandledError("Vulkan buffer full");
    }
    return Result;
}

lbfn render_frame* BeginRenderFrame(vulkan_renderer* Renderer)
{
    u32 FrameID = (u32)(Renderer->CurrentFrameID++ % Renderer->SwapchainImageCount);
    render_frame* Frame = Renderer->Frames + FrameID;

    Frame->SwapchainImageIndex = INVALID_INDEX_U32;
    Frame->RenderFrameID = FrameID;

    Frame->ImageAcquiredSemaphore = Renderer->ImageAcquiredSemaphores[FrameID];
    Frame->ImageAcquiredFence = Renderer->ImageAcquiredFences[FrameID];
    Frame->RenderFinishedFence = Renderer->RenderFinishedFences[FrameID];

    Frame->CmdPool = Renderer->CmdPools[FrameID];
    Frame->CmdBuffer = Renderer->CmdBuffers[FrameID];

    vkWaitForFences(VK.Device, 1, &Frame->RenderFinishedFence, VK_TRUE, UINT64_MAX);
    vkResetFences(VK.Device, 1, &Frame->RenderFinishedFence);

    vkResetCommandPool(VK.Device, Frame->CmdPool, 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);

    Frame->DescriptorPool = Renderer->PerFrameDescriptorPool[FrameID];    

    Frame->ShadowCascadeCount = Renderer->ShadowCascadeCount;
    Frame->ShadowMap = Renderer->ShadowMaps[FrameID];
    Frame->ShadowMapView = Renderer->ShadowViews[FrameID];
    for (u32 i = 0; i < Frame->ShadowCascadeCount; i++)
    {
        Frame->ShadowCascadeViews[i] = Renderer->ShadowCascadeViews[FrameID][i];
    }

    Frame->PerFrameUniformBuffer = Renderer->PerFrameUniformBuffers[FrameID];
    Frame->PerFrameUniformMemory = Renderer->PerFrameUniformBufferMappings[FrameID];

    Frame->DrawBuffer = Renderer->DrawBuffers[FrameID];
    Frame->VertexStack = Renderer->VertexStacks[FrameID];

    Frame->DrawCount = 0;
    Frame->Commands = (VkDrawIndirectCommand*)Frame->DrawBuffer.Mapping;
    Frame->VertexCount = 0;
    Frame->Vertices = (ui_vertex*)Frame->VertexStack.Mapping;

    vkWaitForFences(VK.Device, 1, &Frame->ImageAcquiredFence, VK_TRUE, UINT64_MAX);
    vkResetFences(VK.Device, 1, &Frame->ImageAcquiredFence);

    for (;;)
    {
        VkResult ImageAcquireResult = vkAcquireNextImageKHR(VK.Device, Renderer->Swapchain, 0, 
                                                            Frame->ImageAcquiredSemaphore, 
                                                            Frame->ImageAcquiredFence, 
                                                            &Frame->SwapchainImageIndex);
        if (ImageAcquireResult == VK_SUCCESS || 
            ImageAcquireResult == VK_TIMEOUT ||
            ImageAcquireResult == VK_NOT_READY)
        {
            break;
        }
        else if (ImageAcquireResult == VK_SUBOPTIMAL_KHR ||
                 ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            ResizeRenderTargets(Renderer);
        }
        else
        {
            UnhandledError("vkAcquireNextImage error");
            return nullptr;
        }
    }

    Frame->RenderExtent = Renderer->SurfaceExtent;
    Frame->SwapchainImage = Renderer->SwapchainImages[Frame->SwapchainImageIndex];
    Frame->SwapchainImageView = Renderer->SwapchainImageViews[Frame->SwapchainImageIndex];

    vkResetDescriptorPool(VK.Device, Frame->DescriptorPool, 0);
    Frame->UniformDescriptorSet = PushDescriptorSet(Frame, Renderer->FrameUniformDescriptorSetLayout);

    Assert(Frame->SwapchainImageIndex != INVALID_INDEX_U32);
    return Frame;
}