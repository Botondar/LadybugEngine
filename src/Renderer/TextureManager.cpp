#include "TextureManager.hpp"

lbfn texture_byte_rate GetByteRate(VkFormat Format)
{
    texture_byte_rate Result = { 0, 1, false };
    switch (Format)
    {
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
            Result = { 4, 1, false };
            break;
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
            Result = { 2, 1, false };
            break;
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
            Result = { 1, 2, true };
            break;
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
            Result = { 1, 1, true };
            break;
        default:
            UnimplementedCodePath;
            break;
    }
    return Result;
}

lbfn u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, texture_byte_rate ByteRate)
{
    u64 Result = 0;

    for (u32 Mip = 0; Mip < MipCount; Mip++)
    {
        u32 CurrentWidth = Max(Width >> Mip, 1u);
        u32 CurrentHeight = Max(Height >> Mip, 1u);
        if (ByteRate.IsBlock)
        {
            CurrentWidth = Align(CurrentWidth, 4u);
            CurrentHeight = Align(CurrentHeight, 4u);
        }

        Result += ((u64)CurrentWidth * (u64)CurrentHeight * ByteRate.Numerator) / ByteRate.Denominator;
    }

    return Result;
}

lbfn bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes)
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

lbfn VkImage GetImage(texture_manager* Manager, texture_id ID)
{
    VkImage Result = VK_NULL_HANDLE;
    if (ID.ID != U32_MAX)
    {
        Result = Manager->Images[ID.ID];
    }
    return Result;
}

lbfn texture_id CreateTexture(texture_manager* Manager, 
                                  u32 Width, u32 Height, u32 MipCount, 
                                  VkFormat Format)
{
    texture_id Result = { U32_MAX };

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
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = Format,
                        .components = {},
                        .subresourceRange = 
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = VK_REMAINING_MIP_LEVELS,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                    };

                    VkImageView ImageView = VK_NULL_HANDLE;
                    if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, &ImageView) == VK_SUCCESS)
                    {
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
                            .dstArrayElement = Manager->TextureCount,
                            .descriptorCount = 1,
                            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                            .pImageInfo = &DescriptorImage,
                        };
                        vkUpdateDescriptorSets(VK.Device, 1, &DescriptorWrite, 0, nullptr);

                        Manager->Images[Manager->TextureCount] = Image;
                        Manager->ImageViews[Manager->TextureCount] = ImageView;
                        Result = { Manager->TextureCount++ };
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