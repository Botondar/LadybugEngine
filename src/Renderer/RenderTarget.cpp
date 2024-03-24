internal bool IsDepthFormat(VkFormat Format)
{
    bool Result = 
        Format == VK_FORMAT_D16_UNORM ||
        Format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
        Format == VK_FORMAT_D32_SFLOAT ||
        Format == VK_FORMAT_S8_UINT ||
        Format == VK_FORMAT_D16_UNORM_S8_UINT ||
        Format == VK_FORMAT_D24_UNORM_S8_UINT ||
        Format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    return(Result);
}

internal bool
CreateRenderTargetHeap(render_target_heap* Heap, u64 MemorySize, VkDescriptorSetLayout SetLayout, void* DescriptorBuffer)
{
    bool Result = false;

    Heap->CurrentExtent = { 0, 0 };
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
            .format = FormatTable[RenderTargetFormatTable[RTFormat_HDR]],
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
            .format = FormatTable[RenderTargetFormatTable[RTFormat_Depth]],
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

    u32 MemoryTypeCandidates = VK.GPUMemTypes;
    for (u32 i = 0; i < CountOf(ImageInfos); i++)
    {
        VkImageAspectFlagBits Aspect = IsDepthFormat(ImageInfos[i].format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        VkMemoryRequirements MemoryRequirements = GetImageMemoryRequirements(VK.Device, ImageInfos + i, Aspect);
        MemoryTypeCandidates &= MemoryRequirements.memoryTypeBits;
    }

    u32 MemoryType = 0;
    if (BitScanForward(&MemoryType, MemoryTypeCandidates))
    {
        Heap->Arena = CreateGPUArena(MemorySize, MemoryType, GpuMemoryFlag_None);
        if (Heap->Arena.Memory)
        {
            Heap->SetLayout = SetLayout;
            Heap->DescriptorBuffer = DescriptorBuffer;
            Result = true;
        }
    }

    return Result;
}

internal render_target* 
PushRenderTarget(
    const char* Name, 
    render_target_heap* Heap, 
    VkFormat Format, 
    VkImageUsageFlags Usage, 
    u32 MaxMipCount, 
    u32 Binding,
    u32 StorageBinding,
    u32 BindingGeneral,
    u32 MipBinding)
{
    render_target* Result = nullptr;

    Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT; // HACK(boti)

    if (Heap->RenderTargetCount < Heap->MaxRenderTargetCount)
    {
        Result = Heap->RenderTargets + Heap->RenderTargetCount++;
        *Result =
        {
            .Name               = Name,
            .Image              = VK_NULL_HANDLE,
            .View               = VK_NULL_HANDLE,
            .MipCount           = 0,
            .MipViews           = {},
            .MaxMipCount        = MaxMipCount ? MaxMipCount : R_MaxMipCount,
            .Format             = Format,
            .Usage              = Usage,
            .Binding            = Binding,
            .StorageBinding     = StorageBinding,
            .BindingGeneral     = BindingGeneral,
            .MipBinding         = MipBinding,
        };
    }
    return Result;
}

internal bool 
ResizeRenderTargets(render_target_heap* Heap, v2u Extent)
{
    bool Result = true;

    ResetGPUArena(&Heap->Arena);

    u32 MaxMipCount = GetMaxMipCount(Extent.X, Extent.Y);
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

        VkImageCreateInfo ImageInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = RT->Format,
            .extent = { Extent.X, Extent.Y, 1 },
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
            if (PushImage(&Heap->Arena, RT->Image))
            {
                VkImageAspectFlags AspectFlags = IsDepthFormat(RT->Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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
                    UnhandledError("Failed to create render target image view");
                }
            }
            else
            {
                Result = false;
                UnhandledError("Failed to push render target");
            }
        }
        else
        {
            Result = false;
            UnhandledError("Failed to create render target image");
        }
    }

    if (Result)
    {
        constexpr u32 MaxWriteCount = 4 * Heap->MaxRenderTargetCount;

        u32 WriteAt = 0;
        descriptor_write Writes[MaxWriteCount];
        for (u32 RenderTargetIndex = 0; RenderTargetIndex < Heap->RenderTargetCount; RenderTargetIndex++)
        {
            render_target* RenderTarget = Heap->RenderTargets + RenderTargetIndex;

            if (RenderTarget->Binding != U32_MAX)
            {
                Writes[WriteAt++] =
                {
                    .Type = Descriptor_SampledImage,
                    .Binding = RenderTarget->Binding,
                    .BaseIndex = 0,
                    .Count = 1,
                    .Images = { { RenderTarget->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },
                };
            }

            if (RenderTarget->StorageBinding != U32_MAX)
            {
                Writes[WriteAt++] = 
                {
                    .Type = Descriptor_StorageImage,
                    .Binding = RenderTarget->StorageBinding,
                    .BaseIndex = 0,
                    .Count = 1,
                    .Images = { { RenderTarget->View, VK_IMAGE_LAYOUT_GENERAL } },
                };
            }

            if (RenderTarget->BindingGeneral != U32_MAX)
            {
                Writes[WriteAt++] = 
                {
                    .Type = Descriptor_SampledImage,
                    .Binding = RenderTarget->BindingGeneral,
                    .BaseIndex = 0,
                    .Count = 1,
                    .Images = { { RenderTarget->View, VK_IMAGE_LAYOUT_GENERAL } },
                };
            }

            if (RenderTarget->MipBinding != U32_MAX)
            {
                u32 WriteIndex = WriteAt++;
                Writes[WriteIndex] = 
                {
                    .Type = Descriptor_StorageImage,
                    .Binding = RenderTarget->MipBinding,
                    .BaseIndex = 0,
                    .Count = RenderTarget->MipCount,
                };

                for (u32 Mip = 0; Mip < RenderTarget->MipCount; Mip++)
                {
                    Writes[WriteIndex].Images[Mip] = { RenderTarget->MipViews[Mip], VK_IMAGE_LAYOUT_GENERAL };
                }
            }
        }

        UpdateDescriptorBuffer(WriteAt, Writes, Heap->SetLayout, Heap->DescriptorBuffer);
    }

    Heap->CurrentExtent = Extent;
    return Result;
}