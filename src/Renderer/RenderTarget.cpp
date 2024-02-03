internal bool 
CreateRenderTargetHeap(render_target_heap* Heap, u64 MemorySize)
{
    bool Result = false;

    VkImageCreateInfo ImageInfos[] = 
    {
        // RT LDR
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .extent = { 1280u, 720u, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
        // RT HDR
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = FormatTable[HDR_FORMAT],
            .extent = { 1280u, 720u, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
        // Depth-stencil
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = FormatTable[DEPTH_FORMAT],
            .extent = { 1280u, 720u, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
    };

    // Create dummy images and query their memory type requirements
    u32 MemoryTypeCandidates = VK.GPUMemTypes;
    for (u32 i = 0; i < CountOf(ImageInfos); i++)
    {
        VkImage Image = VK_NULL_HANDLE;
        if (vkCreateImage(VK.Device, ImageInfos + i, nullptr, &Image) != VK_SUCCESS)
        {
            return false;
        }
        
        VkMemoryRequirements MemoryRequirements = {};
        vkGetImageMemoryRequirements(VK.Device, Image, &MemoryRequirements);
        MemoryTypeCandidates &= MemoryRequirements.memoryTypeBits;
        vkDestroyImage(VK.Device, Image, nullptr);
    }

    u32 MemoryType = 0;
    if (BitScanForward(&MemoryType, MemoryTypeCandidates))
    {
        
        VkMemoryAllocateInfo AllocInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = MemorySize,
            .memoryTypeIndex = MemoryType,
        };

        if (vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Heap->Memory) == VK_SUCCESS)
        {
            Heap->MemoryType = MemoryType;
            Heap->MemorySize = MemorySize;
            Heap->Offset = 0;

            Result = true;
        }
    }

    return Result;
}

internal render_target* 
PushRenderTarget(const char* Name, render_target_heap* Heap, VkFormat Format, VkImageUsageFlags Usage, u32 MaxMipCount /*= 1*/)
{
    render_target* Result = nullptr;

    Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT; // HACK(boti)

    if (Heap->RenderTargetCount < Heap->MaxRenderTargetCount)
    {
        Result = Heap->RenderTargets + Heap->RenderTargetCount++;
        *Result =
        {
            .Name = Name,
            .Image = VK_NULL_HANDLE,
            .View = VK_NULL_HANDLE,
            .MipCount = 0,
            .MipViews = {},
            .MaxMipCount = MaxMipCount ? MaxMipCount : R_MaxMipCount,
            .Format = Format,
            .Usage = Usage,
        };
    }
    return Result;
}

internal bool 
ResizeRenderTargets(render_target_heap* Heap, u32 Width, u32 Height)
{
    bool Result = true;

    // TODO(boti): Add support for stencil formats

    Heap->Offset = 0;
    u32 MaxMipCount = GetMaxMipCount(Width, Height);
    for (u32 RenderTargetIndex = 0; RenderTargetIndex < Heap->RenderTargetCount; RenderTargetIndex++)
    {
        if (!Result) break;

        render_target* RT = Heap->RenderTargets + RenderTargetIndex;
        vkDestroyImageView(VK.Device, RT->View, nullptr);
        for (u32 Mip = 0; Mip < RT->MipCount; Mip++)
        {
            vkDestroyImageView(VK.Device, RT->MipViews[Mip], nullptr);
        }
        vkDestroyImage(VK.Device, RT->Image, nullptr);

        RT->MipCount = Min(MaxMipCount, RT->MaxMipCount);

        bool IsDepthFormat = false;
        if (RT->Format == VK_FORMAT_D16_UNORM ||
            RT->Format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
            RT->Format == VK_FORMAT_D32_SFLOAT ||
            RT->Format == VK_FORMAT_S8_UINT ||
            RT->Format == VK_FORMAT_D16_UNORM_S8_UINT ||
            RT->Format == VK_FORMAT_D24_UNORM_S8_UINT ||
            RT->Format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            IsDepthFormat = true;
        }

        VkImageCreateInfo ImageInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = RT->Format,
            .extent = { Width, Height, 1 },
            .mipLevels = RT->MipCount,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = RT->Usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        if (vkCreateImage(VK.Device, &ImageInfo, nullptr, &RT->Image) == VK_SUCCESS)
        {
            VkMemoryRequirements MemoryRequirements = {};
            vkGetImageMemoryRequirements(VK.Device, RT->Image, &MemoryRequirements);

            if ((MemoryRequirements.memoryTypeBits & (1u << Heap->MemoryType)) != 0)
            {
                u64 Offset = Align(Heap->Offset, MemoryRequirements.alignment);
                if (Offset + MemoryRequirements.size <= Heap->MemorySize)
                {
                    if (vkBindImageMemory(VK.Device, RT->Image, Heap->Memory, Offset) == VK_SUCCESS)
                    {
                        Heap->Offset = Offset + MemoryRequirements.size;

                        VkImageAspectFlags AspectFlags = IsDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT ;
                        for (u32 Mip = 0; Mip < RT->MipCount; Mip++)
                        {
                            VkImageViewCreateInfo ViewInfo = 
                            {
                                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                .pNext = nullptr,
                                .flags = 0,
                                .image = RT->Image,
                                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                .format = RT->Format,
                                .components = {},
                                .subresourceRange = 
                                {
                                    .aspectMask = AspectFlags,
                                    .baseMipLevel = Mip,
                                    .levelCount = 1,
                                    .baseArrayLayer = 0,
                                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                },
                            };

                            if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, RT->MipViews + Mip) != VK_SUCCESS)
                            {
                                Result = false;
                            }
                        }

                        VkImageViewCreateInfo ViewInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .image = RT->Image,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = RT->Format,
                            .components = {},
                            .subresourceRange = 
                            {
                                .aspectMask = AspectFlags,
                                .baseMipLevel = 0,
                                .levelCount = VK_REMAINING_MIP_LEVELS,
                                .baseArrayLayer = 0,
                                .layerCount = VK_REMAINING_ARRAY_LAYERS,
                            },
                        };

                        if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, &RT->View) == VK_SUCCESS)
                        {
                            VkDebugUtilsObjectNameInfoEXT NameInfo = 
                            {
                                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                .pNext = nullptr,
                                .objectType = VK_OBJECT_TYPE_IMAGE,
                                .objectHandle = (u64)RT->Image,
                                .pObjectName = RT->Name,

                            };
                            vkSetDebugUtilsObjectNameEXT(VK.Device, &NameInfo);
                        }
                        else
                        {
                            Result = false;
                        }
                    }
                    else
                    {
                        Result = false;
                    }
                }
                else
                {
                    Result = false;
                }
            }
            else
            {
                Result = false;
            }
        }
        else
        {
            Result = false;
        }
    }
    return Result;
}