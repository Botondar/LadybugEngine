#include "TextureManager.hpp"

constexpr u32 SpecialTextureBit = (1u << 31);

internal bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes)
{
    VkResult Result = VK_SUCCESS;

    VkSamplerCreateInfo SamplerInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 4.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    Result = vkCreateSampler(VK.Device, &SamplerInfo, nullptr, &Manager->Sampler);
    if (Result != VK_SUCCESS)
    {
        return false;
    }

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
            // Set 0
            {
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = 1,
            },
            // Set 1
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
            .maxSets = 2,
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
            .descriptorSetCount = CountOf(Manager->DescriptorSetLayouts),
            .pSetLayouts = Manager->DescriptorSetLayouts,
        };
        Result = vkAllocateDescriptorSets(VK.Device, &DescriptorInfo, Manager->DescriptorSets);
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

internal VkImage GetImage(texture_manager* Manager, texture_id ID)
{
    VkImage Result = VK_NULL_HANDLE;
    if (IsValid(ID))
    {
        if (ID.Value & SpecialTextureBit)
        {
            u32 Index = ID.Value & (~SpecialTextureBit);
            Result = Manager->SpecialImages[Index];
        }
        else
        {
            Result = Manager->Images[ID.Value];
        }
    }
    return Result;
}

internal VkImageView GetImageView(texture_manager* Manager, texture_id ID)
{
    VkImageView Result = VK_NULL_HANDLE;
    if (IsValid(ID))
    {
        if (ID.Value & SpecialTextureBit)
        {
            u32 Index = ID.Value & (~SpecialTextureBit);
            Result = Manager->SpecialImageViews[Index];
        }
        else
        {
            Result = Manager->ImageViews[ID.Value];
        }
    }
    return(Result);
}

internal texture_id 
CreateTexture2D(texture_manager* Manager, texture_flags Flags,
                u32 Width, u32 Height, u32 MipCount, u32 ArrayCount,
                VkFormat Format, texture_swizzle Swizzle)
{
    texture_id Result = { U32_MAX };
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

    if (Manager->TextureCount < Manager->MaxTextureCount)
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
        VkImage Image = VK_NULL_HANDLE;
        if (vkCreateImage(VK.Device, &ImageInfo, nullptr, &Image) == VK_SUCCESS)
        {
            VkMemoryRequirements MemoryRequirements = {};
            vkGetImageMemoryRequirements(VK.Device, Image, &MemoryRequirements);

            Manager->MemoryOffset = Align(Manager->MemoryOffset, MemoryRequirements.alignment);
            if (Manager->MemoryOffset + MemoryRequirements.size < Manager->MemorySize)
            {
                size_t Offset = Manager->MemoryOffset;
                Manager->MemoryOffset += MemoryRequirements.size;

                if (vkBindImageMemory(VK.Device, Image, Manager->Memory, Offset) == VK_SUCCESS)
                {
                    VkImageViewCreateInfo ViewInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .image = Image,
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

                    VkImageView ImageView = VK_NULL_HANDLE;
                    if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, &ImageView) == VK_SUCCESS)
                    {
                        if (HasFlag(Flags, TextureFlag_Special))
                        {
                            u32 Index = Manager->SpecialTextureCount++;
                            Result = { SpecialTextureBit | Index };
                            Manager->SpecialImages[Index] = Image;
                            Manager->SpecialImageViews[Index] = ImageView;
                        }
                        else
                        {
                            Assert(ArrayCount == 1);

                            u32 Index = Manager->TextureCount++;
                            VkDescriptorImageInfo DescriptorImage = 
                            {
                                .sampler = VK_NULL_HANDLE,
                                .imageView = ImageView,
                                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                            };

                            VkWriteDescriptorSet DescriptorWrite = 
                            {
                                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .pNext = nullptr,
                                .dstSet = Manager->DescriptorSets[1],
                                .dstBinding = 0,
                                .dstArrayElement = Index,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                .pImageInfo = &DescriptorImage,
                            };
                            vkUpdateDescriptorSets(VK.Device, 1, &DescriptorWrite, 0, nullptr);

                            Manager->Images[Index] = Image;
                            Manager->ImageViews[Index] = ImageView;
                            Result = { Index };
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

    return Result;
}