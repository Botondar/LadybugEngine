#include "TextureManager.hpp"

constexpr u32 SpecialTextureBit = (1u << 31);

internal b32 IsTextureSpecial(renderer_texture_id ID)
{
    b32 Result = (ID.Value & SpecialTextureBit) != 0;
    return(Result);
}

internal bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes, VkDescriptorSetLayout* SetLayouts)
{
    VkResult Result = VK_SUCCESS;

    {
        VkImageCreateInfo DummyImageInfo = 
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

        VkMemoryRequirements MemoryRequirements = GetImageMemoryRequirements(VK.Device, &DummyImageInfo, VK_IMAGE_ASPECT_COLOR_BIT);
        MemoryTypes &= MemoryRequirements.memoryTypeBits;
    }

    // TODO(boti): Error handling cleanup
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
        if (Result == VK_SUCCESS)
        {
            vkGetDescriptorSetLayoutBindingOffsetEXT(VK.Device, SetLayouts[SetLayout_Bindless], Binding_Bindless_Textures, &Manager->TextureTableOffset);
            vkGetDescriptorSetLayoutSizeEXT(VK.Device, SetLayouts[SetLayout_Bindless], &Manager->DescriptorBufferSize);

            VkBufferCreateInfo DescriptorBufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = Manager->DescriptorBufferSize,
                .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
            };
            Result = vkCreateBuffer(VK.Device, &DescriptorBufferInfo, nullptr, &Manager->DescriptorBuffer);
            Assert(Result == VK_SUCCESS);

            u32 DescriptorBufferMemoryTypes = VK.BARMemTypes;
            {
                VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &DescriptorBufferInfo);
                DescriptorBufferMemoryTypes &= MemoryRequirements.memoryTypeBits;

                u32 DescriptorMemoryType = 0;
                if (BitScanForward(&DescriptorMemoryType, DescriptorBufferMemoryTypes))
                {
                    VkMemoryAllocateFlagsInfo AllocFlags = 
                    {
                        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
                        .pNext = nullptr,
                        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
                        .deviceMask = 0,
                    };
                    VkMemoryAllocateInfo AllocInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                        .pNext = &AllocFlags,
                        .allocationSize = Manager->DescriptorBufferSize,
                        .memoryTypeIndex = DescriptorMemoryType,
                    };
                    Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Manager->DescriptorMemory);
                    if (Result == VK_SUCCESS)
                    {
                        Result = vkBindBufferMemory(VK.Device, Manager->DescriptorBuffer, Manager->DescriptorMemory, 0);
                        Assert(Result == VK_SUCCESS);

                        Result = vkMapMemory(VK.Device, Manager->DescriptorMemory, 0, VK_WHOLE_SIZE, 0, &Manager->DescriptorMapping);
                        Assert(Result == VK_SUCCESS);

                        VkBufferDeviceAddressInfo DeviceAddressInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                            .pNext = nullptr,
                            .buffer = Manager->DescriptorBuffer,
                        };
                        Manager->DescriptorDeviceAddress = vkGetBufferDeviceAddress(VK.Device, &DeviceAddressInfo);
                    }
                    else
                    {
                        return(false);
                    }
                }
            }
        }
        else
        {
            return(false);
        }
    }
    else
    {
        UnhandledError("No suitable memory type for textures");
    }

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

internal texture_info GetTextureInfo(texture_manager* Manager, renderer_texture_id ID)
{
    texture_info Info = {};
    if (IsValid(ID))
    {
        if (ID.Value & SpecialTextureBit)
        {
            u32 Index = ID.Value & (~SpecialTextureBit);
            Info = Manager->SpecialTextureInfos[Index];
        }
        else
        {
            Info = Manager->TextureInfos[ID.Value];
        }
    }
    return(Info);
}

internal renderer_texture_id
AllocateTexture(texture_manager* Manager, texture_flags Flags, const texture_info* Info, renderer_texture_id Placeholder)
{
    renderer_texture_id Result = { U32_MAX };

    u32 MaxCount = 0;
    u32* Count = nullptr;
    VkImage* Images = nullptr;
    VkImageView* Views = nullptr;
    texture_info* Infos = nullptr;

    if (Flags & TextureFlag_Special)
    {
        MaxCount = Manager->MaxSpecialTextureCount;
        Count = &Manager->SpecialTextureCount;
        Images = Manager->SpecialImages;
        Views = Manager->SpecialImageViews;
        Infos = Manager->SpecialTextureInfos;
    }
    else
    {
        MaxCount = Manager->MaxTextureCount;
        Count = &Manager->TextureCount;
        Images = Manager->Images;
        Views = Manager->ImageViews;
        Infos = Manager->TextureInfos;
    }

    if (*Count < MaxCount)
    {
        u32 Index = (*Count)++;
        Result = { Index };
        if (Flags & TextureFlag_Special)
        {
            Result.Value |= SpecialTextureBit;
        }

        if (Info)
        {
            Infos[Index] = *Info;
            // TODO(boti): We should really just create the VkImage and VkImageView here instead of allocating memory
            AllocateTexture(Manager, Result, *Info);
        }
        else
        {
            Infos[Index] = {};
        }

        if (IsValid(Placeholder) && !(Flags & TextureFlag_Special))
        {
            umm DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize;
            VkDescriptorImageInfo DescriptorImage = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = *GetImageView(Manager, Placeholder),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorGetInfoEXT DescriptorInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .data = { .pSampledImage = &DescriptorImage },
            };
            umm DescriptorOffset = Manager->TextureTableOffset + Index*DescriptorSize;
            vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Manager->DescriptorMapping, DescriptorOffset));
        }
    }
    else
    {
        UnhandledError("Out of textures");
    }

    return(Result);
}

internal b32
AllocateTexture(texture_manager* Manager, renderer_texture_id ID, texture_info Info)
{
    b32 Result = false;

    if (IsValid(ID))
    {
        u32 Index = ID.Value & (~SpecialTextureBit);
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