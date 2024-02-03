#include "TextureManager.hpp"

constexpr u32 SpecialTextureBit = (1u << 31);

internal bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes)
{
    VkResult Result = VK_SUCCESS;

    VkImageCreateInfo ImageInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent = { 1024, 1024, 1 },
        .mipLevels = 10,
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
    Result = vkCreateImage(VK.Device, &ImageInfo, nullptr, &Image);
    if (Result != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(VK.Device, Image, &MemoryRequirements);
    MemoryTypes &= MemoryRequirements.memoryTypeBits;

    u32 MemoryTypeIndex = 0;
    if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
    {
        Manager->MemorySize = MemorySize;

        VkMemoryAllocateInfo MemAllocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = Manager->MemorySize,
            .memoryTypeIndex = MemoryTypeIndex,
        };
        Result = vkAllocateMemory(VK.Device, &MemAllocInfo, nullptr, &Manager->Memory);
        if (Result != VK_SUCCESS)
        {
            return false;
        }

        VkDescriptorPoolSize PoolSizes[] = 
        {
            {
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = Manager->MaxTextureCount,
            },
        };

        VkDescriptorPoolCreateInfo PoolInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = CountOf(PoolSizes),
            .pPoolSizes = PoolSizes,
        };
        Result = vkCreateDescriptorPool(VK.Device, &PoolInfo, nullptr, &Manager->DescriptorPool);
        if (Result != VK_SUCCESS)
        {
            return false;
        }
        VkDescriptorSetAllocateInfo DescriptorInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = Manager->DescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &Manager->DescriptorSetLayout,
        };
        Result = vkAllocateDescriptorSets(VK.Device, &DescriptorInfo, &Manager->DescriptorSet);
        if (Result != VK_SUCCESS)
        {
            return false;
        }
    }
    else
    {
        UnhandledError("No suitable memory type for textures");
    }

    vkDestroyImage(VK.Device, Image, nullptr);

    return (Result == VK_SUCCESS);
}

internal VkImage* GetImage(texture_manager* Manager, renderer_texture_id ID)
{
    VkImage* Result = nullptr;
    if (IsValid(ID))
    {
        if (ID.Value & SpecialTextureBit)
        {
            u32 Index = ID.Value & (~SpecialTextureBit);
            Result = Manager->SpecialImages + Index;
        }
        else
        {
            Result = Manager->Images + ID.Value;
        }
    }
    return Result;
}

internal VkImageView* GetImageView(texture_manager* Manager, renderer_texture_id ID)
{
    VkImageView* Result = nullptr;
    if (IsValid(ID))
    {
        if (ID.Value & SpecialTextureBit)
        {
            u32 Index = ID.Value & (~SpecialTextureBit);
            Result = Manager->SpecialImageViews + Index;
        }
        else
        {
            Result = Manager->ImageViews + ID.Value;
        }
    }
    return(Result);
}

internal renderer_texture_id
AllocateTextureName(texture_manager* Manager, texture_flags Flags)
{
    renderer_texture_id Result = { U32_MAX };

    if (Flags & TextureFlag_Special)
    {
        if (Manager->SpecialTextureCount < Manager->MaxSpecialTextureCount)
        {
            u32 Index = Manager->SpecialTextureCount++;
            Result = { Index | SpecialTextureBit };
        }
        else
        {
            UnhandledError("Out of special textures");
        }
    }
    else
    {
        if (Manager->TextureCount < Manager->MaxTextureCount)
        {
            u32 Index = Manager->TextureCount++;
            Result = { Index };
        }
        else
        {
            UnhandledError("Out of textures");
        }
    }

    return(Result);
}

internal b32
AllocateTexture(texture_manager* Manager, renderer_texture_id ID, texture_info Info)
{
    b32 Result = false;

    if (IsValid(ID))
    {
        VkImage* Image = GetImage(Manager, ID);
        VkImageView* View = GetImageView(Manager, ID);

        VkImageCreateInfo ImageInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D, // TODO(boti): Maybe this should be passed in as well?
            .format = FormatTable[Info.Format],
            .extent = { Info.Width, Info.Height, Info.Depth },
            .mipLevels = Info.MipCount,
            .arrayLayers = Info.ArrayCount,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        if (vkCreateImage(VK.Device, &ImageInfo, nullptr, Image) == VK_SUCCESS)
        {
            VkMemoryRequirements MemoryRequirements = {};
            vkGetImageMemoryRequirements(VK.Device, *Image, &MemoryRequirements);

            umm MemoryOffset = Align(Manager->MemoryOffset, MemoryRequirements.alignment);
            if (MemoryOffset + MemoryRequirements.size < Manager->MemorySize)
            {
                if (vkBindImageMemory(VK.Device, *Image, Manager->Memory, MemoryOffset) == VK_SUCCESS)
                {
                    Manager->MemoryOffset = MemoryOffset + MemoryRequirements.size;

                    auto SwizzleToVulkan = [](texture_swizzle_type Type) -> VkComponentSwizzle 
                    {
                        VkComponentSwizzle Result = VK_COMPONENT_SWIZZLE_IDENTITY;
                        switch (Type)
                        {
                            case Swizzle_Identity: Result = VK_COMPONENT_SWIZZLE_IDENTITY; break;
                            case Swizzle_Zero: Result = VK_COMPONENT_SWIZZLE_ZERO; break;
                            case Swizzle_One: Result = VK_COMPONENT_SWIZZLE_ONE; break;
                            case Swizzle_R: Result = VK_COMPONENT_SWIZZLE_R; break;
                            case Swizzle_G: Result = VK_COMPONENT_SWIZZLE_G; break;
                            case Swizzle_B: Result = VK_COMPONENT_SWIZZLE_B; break;
                            case Swizzle_A: Result = VK_COMPONENT_SWIZZLE_A; break;
                        }
                        return(Result);
                    };

                    VkImageViewCreateInfo ViewInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .image = *Image,
                        .viewType = (Info.ArrayCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                        .format = FormatTable[Info.Format],
                        .components = 
                        {
                            SwizzleToVulkan(Info.Swizzle.R),
                            SwizzleToVulkan(Info.Swizzle.G),
                            SwizzleToVulkan(Info.Swizzle.B),
                            SwizzleToVulkan(Info.Swizzle.A),
                        },
                        .subresourceRange = 
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = VK_REMAINING_MIP_LEVELS,
                            .baseArrayLayer = 0,
                            .layerCount = VK_REMAINING_ARRAY_LAYERS,
                        },
                    };

                    if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, View) == VK_SUCCESS)
                    {
                        b32 IsSpecial = (ID.Value & SpecialTextureBit) != 0;
                        if (!IsSpecial)
                        {
                            Assert(Info.ArrayCount == 1);
                            VkDescriptorImageInfo DescriptorImage = 
                            {
                                .sampler = VK_NULL_HANDLE,
                                .imageView = *View,
                                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                            };

                            VkWriteDescriptorSet DescriptorWrite = 
                            {
                                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .pNext = nullptr,
                                .dstSet = Manager->DescriptorSet,
                                .dstBinding = 0,
                                .dstArrayElement = ID.Value,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                .pImageInfo = &DescriptorImage,
                            };
                            vkUpdateDescriptorSets(VK.Device, 1, &DescriptorWrite, 0, nullptr);
                        }

                        Result = true;
                    }
                    else
                    {
                        UnhandledError("Failed to create image view");
                    }
                }
                else
                {
                    UnhandledError("Failed to bind image memory");
                }
            }
            else
            {
                UnhandledError("Out of texture memory");
            }
        }
        else
        {
            UnhandledError("Failed to create texture");
        }
    }

    return(Result);
}

// TODO(boti): Pipe this through AllocateTexture
internal renderer_texture_id 
CreateTexture2D(texture_manager* Manager, texture_flags Flags,
                u32 Width, u32 Height, u32 MipCount, u32 ArrayCount,
                VkFormat Format, texture_swizzle Swizzle)
{
    Assert(ArrayCount < VK.DeviceProps.limits.maxImageArrayLayers);

    auto SwizzleToVulkan = [](texture_swizzle_type Type) -> VkComponentSwizzle 
    {
        VkComponentSwizzle Result = VK_COMPONENT_SWIZZLE_IDENTITY;
        switch (Type)
        {
            case Swizzle_Identity: Result = VK_COMPONENT_SWIZZLE_IDENTITY; break;
            case Swizzle_Zero: Result = VK_COMPONENT_SWIZZLE_ZERO; break;
            case Swizzle_One: Result = VK_COMPONENT_SWIZZLE_ONE; break;
            case Swizzle_R: Result = VK_COMPONENT_SWIZZLE_R; break;
            case Swizzle_G: Result = VK_COMPONENT_SWIZZLE_G; break;
            case Swizzle_B: Result = VK_COMPONENT_SWIZZLE_B; break;
            case Swizzle_A: Result = VK_COMPONENT_SWIZZLE_A; break;
        }
        return(Result);
    };

    renderer_texture_id ID = AllocateTextureName(Manager, Flags);

    if (IsValid(ID))
    {
        VkImageCreateInfo ImageInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = Format,
            .extent = { Width, Height, 1 },
            .mipLevels = MipCount,
            .arrayLayers = ArrayCount,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        VkImage* Image = GetImage(Manager, ID);
        if (vkCreateImage(VK.Device, &ImageInfo, nullptr, Image) == VK_SUCCESS)
        {
            VkMemoryRequirements MemoryRequirements = {};
            vkGetImageMemoryRequirements(VK.Device, *Image, &MemoryRequirements);

            Manager->MemoryOffset = Align(Manager->MemoryOffset, MemoryRequirements.alignment);
            if (Manager->MemoryOffset + MemoryRequirements.size < Manager->MemorySize)
            {
                size_t Offset = Manager->MemoryOffset;
                Manager->MemoryOffset += MemoryRequirements.size;

                if (vkBindImageMemory(VK.Device, *Image, Manager->Memory, Offset) == VK_SUCCESS)
                {
                    VkImageViewCreateInfo ViewInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .image = *Image,
                        .viewType = (ArrayCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                        .format = Format,
                        .components = 
                        {
                            SwizzleToVulkan(Swizzle.R),
                            SwizzleToVulkan(Swizzle.G),
                            SwizzleToVulkan(Swizzle.B),
                            SwizzleToVulkan(Swizzle.A),
                        },
                        .subresourceRange = 
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = VK_REMAINING_MIP_LEVELS,
                            .baseArrayLayer = 0,
                            .layerCount = VK_REMAINING_ARRAY_LAYERS,
                        },
                    };

                    VkImageView* ImageView = GetImageView(Manager, ID);
                    if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, ImageView) == VK_SUCCESS)
                    {
                        if (!HasFlag(Flags, TextureFlag_Special))
                        {
                            Assert(ArrayCount == 1);

                            VkDescriptorImageInfo DescriptorImage = 
                            {
                                .sampler = VK_NULL_HANDLE,
                                .imageView = *ImageView,
                                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                            };

                            VkWriteDescriptorSet DescriptorWrite = 
                            {
                                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .pNext = nullptr,
                                .dstSet = Manager->DescriptorSet,
                                .dstBinding = 0,
                                .dstArrayElement = ID.Value,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                .pImageInfo = &DescriptorImage,
                            };
                            vkUpdateDescriptorSets(VK.Device, 1, &DescriptorWrite, 0, nullptr);
                        }
                    }
                    else
                    {
                        UnhandledError("Failed to create image view");
                    }
                }
                else
                {
                    UnhandledError("Failed to bind image to memory");
                }
            }
            else
            {
                UnhandledError("Out of texture memory");
            }
        }
        else
        {
            UnhandledError("Failed to create image");
        }
    }

    return(ID);
}