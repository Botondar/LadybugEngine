#include "TextureManager.hpp"

constexpr u32 SpecialTextureBit = (1u << 31);

internal u32 GetTextureIndex(renderer_texture_id ID)
{
    u32 Result = ID.Value & (~SpecialTextureBit);
    return(Result);
}

internal b32 IsTextureSpecial(renderer_texture_id ID)
{
    b32 Result = (ID.Value & SpecialTextureBit) != 0;
    return(Result);
}

// TODO(boti): rewrite cache usage with bit ops

internal umm 
FindFreePageRange(texture_cache* Cache, umm PageCount)
{
    umm Result = U64_MAX;

    umm FoundPageCount = 0;
    umm CandidatePage = 0;
    for (umm PageIndex = 0; PageIndex < Cache->PageCount; PageIndex++)
    {
        umm PageGroupIndex = PageIndex / 64;
        umm InGroupIndex = PageIndex % 64;
        u64 Mask = 1 << InGroupIndex;
        if ((Cache->PageUsage[PageGroupIndex] & Mask) == 0)
        {
            FoundPageCount++;
            if (FoundPageCount == PageCount)
            {
                Result = CandidatePage;
                break;
            }
        }
        else
        {
            FoundPageCount = 0;
            CandidatePage = PageIndex + 1;
        }
    }
    return(Result);
}

internal void
MarkPagesAsUsed(texture_cache* Cache, umm FirstPage, umm PageCount)
{
    Cache->UsedPageCount += PageCount;
    umm PageGroupIndex = FirstPage / 64;
    umm InGroupIndex = FirstPage % 64;
    for (umm PageIndex = 0; PageIndex < PageCount; PageIndex++)
    {
        Assert((Cache->PageUsage[PageGroupIndex] & (1 << InGroupIndex)) == 0);
        Cache->PageUsage[PageGroupIndex] |= (1 << InGroupIndex);
        if (++InGroupIndex == 64)
        {
            InGroupIndex = 0;
            PageGroupIndex++;
        }
    }
}

internal b32
AllocateImage(texture_cache* Cache, VkDeviceMemory Memory, VkImage Image, umm* OutPageIndex, umm* OutPageCount)
{
    b32 Result = false;

    VkMemoryRequirements MemoryRequirements;
    vkGetImageMemoryRequirements(VK.Device, Image, &MemoryRequirements);
    Assert(MemoryRequirements.alignment <= TexturePageSize);
    
    umm PageCount = CeilDiv(MemoryRequirements.size, TexturePageSize);
    umm PageIndex = FindFreePageRange(Cache, PageCount);
    if (PageIndex != U64_MAX)
    {
        if (vkBindImageMemory(VK.Device, Image, Memory, PageIndex * TexturePageSize) == VK_SUCCESS)
        {
            MarkPagesAsUsed(Cache, PageIndex, PageCount);
            if (OutPageIndex) *OutPageIndex = PageIndex;
            if (OutPageCount) *OutPageCount = PageCount;
            Result = true;
        }
        else
        {
            UnhandledError("Failed to bind image memory");
        }
    }

    return(Result);
}

internal bool CreateTextureManager(texture_manager* Manager, memory_arena* Arena, u64 MemorySize, u32 MemoryTypes, VkDescriptorSetLayout* SetLayouts)
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
    u32 MemoryTypeIndex, DescriptorMemoryTypeIndex;
    if (BitScanForward(&MemoryTypeIndex, MemoryTypes) &&
        BitScanForward(&DescriptorMemoryTypeIndex, VK.BARMemTypes))
    {
        vkGetDescriptorSetLayoutBindingOffsetEXT(VK.Device, SetLayouts[Set_Bindless], Binding_Bindless_Textures, &Manager->TextureTableOffset);
        umm DescriptorBufferSize = 0;
        vkGetDescriptorSetLayoutSizeEXT(VK.Device, SetLayouts[Set_Bindless], &DescriptorBufferSize);
        Manager->DescriptorArena    = CreateGPUArena(DescriptorBufferSize, DescriptorMemoryTypeIndex, GpuMemoryFlag_Mapped);

        Manager->PersistentArena    = CreateGPUArena(MiB(32), MemoryTypeIndex, GpuMemoryFlag_None);
        Manager->CacheArena         = CreateGPUArena(MemorySize, MemoryTypeIndex, GpuMemoryFlag_None);

        Manager->Cache.PageCount = CeilDiv(MemorySize, TexturePageSize);
        umm PageUsageCount = CeilDiv(Manager->Cache.PageCount, 64);
        Manager->Cache.PageUsage = PushArray(Arena, MemPush_Clear, u64, PageUsageCount);

        Manager->Cache.UsedPageCount = 0;

        if (Manager->DescriptorArena.Memory && Manager->PersistentArena.Memory && Manager->CacheArena.Memory)
        {
            if (PushBuffer(&Manager->DescriptorArena, DescriptorBufferSize,
                           VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                           &Manager->DescriptorBuffer, &Manager->DescriptorMapping))
            {
                Manager->DescriptorAddress = GetBufferDeviceAddress(VK.Device, Manager->DescriptorBuffer);
            }
            else
            {
                // TODO(boti)
                Result = VK_ERROR_UNKNOWN;
            }
        }
        else
        {
            UnhandledError("No suitable memory type for textures");
            Result = VK_ERROR_UNKNOWN;
        }
    }
    else
    {
        UnhandledError("No suitable memory type for textures or descriptor buffer");
        Result = VK_ERROR_UNKNOWN;
    }

    return (Result == VK_SUCCESS);
}

internal renderer_texture* GetTexture(texture_manager* Manager, renderer_texture_id ID)
{
    renderer_texture* Result = nullptr;

    if (IsValid(ID))
    {
        u32 Index = GetTextureIndex(ID);
        if (IsTextureSpecial(ID))
        {
            Assert(Index < Manager->MaxSpecialTextureCount);
            Result = Manager->SpecialTextures + Index;
        }
        else
        {
            Assert(Index < R_MaxTextureCount);
            Result = Manager->Textures + Index;
        }
    }

    return(Result);
}

internal renderer_texture_id 
AllocateNextID(texture_manager* Manager, texture_flags Flags)
{
    renderer_texture_id Result = InvalidRendererTextureID;

    if (Flags & TextureFlag_Special)
    {
        if (Manager->SpecialTextureCount < Manager->MaxSpecialTextureCount)
        {
            Result = { SpecialTextureBit | Manager->SpecialTextureCount++ };
        }
    }
    else
    {
        if (Manager->TextureCount < R_MaxTextureCount)
        {
            Result = { Manager->TextureCount++ };
        }
    }

    return(Result);
}

internal renderer_texture_id
AllocateTexture(texture_manager* Manager, texture_flags Flags, const texture_info* Info, renderer_texture_id Placeholder)
{
    renderer_texture_id Result = AllocateNextID(Manager, Flags);
    if (IsValid(Result))
    {
        renderer_texture* Texture = GetTexture(Manager, Result);
        Assert(Texture);
        Assert(Texture->ImageHandle == VK_NULL_HANDLE);
        Assert(Texture->ViewHandle == VK_NULL_HANDLE);

        Texture->Info = {};
        Texture->Flags = Flags;
        if (Info)
        {
            Texture->Info = *Info;
            if (AllocateTexture(Manager, Result, Texture->Info))
            {
            }
            else
            {
                // TODO(boti): Check for placeholder here - if there's one we can ignore this case and keep using the placeholder
            }
        }

        if (IsValid(Placeholder) && !IsTextureSpecial(Result))
        {
            renderer_texture* PlaceholderTex = GetTexture(Manager, Placeholder);
            Assert(PlaceholderTex);

            umm DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize;
            VkDescriptorImageInfo DescriptorImage = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = PlaceholderTex->ViewHandle,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkDescriptorGetInfoEXT DescriptorInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .data = { .pSampledImage = &DescriptorImage },
            };
            umm DescriptorOffset = Manager->TextureTableOffset + Result.Value*DescriptorSize;
            vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Manager->DescriptorMapping, DescriptorOffset));
        }
    }
    else
    {
        UnhandledError("Failed to allocate texture ID");
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
        renderer_texture* Texture = GetTexture(Manager, ID);

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
        VkImage Image = VK_NULL_HANDLE;
        VkResult ErrorCode = vkCreateImage(VK.Device, &ImageInfo, nullptr, &Image);
        if (ErrorCode == VK_SUCCESS)
        {
            umm PageIndex = 0;
            umm PageCount = 0;
            b32 PushResult = false;
            if (Texture->Flags & TextureFlag_PersistentMemory)
            {
                PushResult = PushImage(&Manager->PersistentArena, Image);
            }
            else
            {
                PushResult = AllocateImage(&Manager->Cache, Manager->CacheArena.Memory, Image, &PageIndex, &PageCount);
            }
            
            if (PushResult)
            {
                auto SwizzleToVulkan = [](texture_swizzle_type Swizzle) -> VkComponentSwizzle
                {
                    VkComponentSwizzle Result = VK_COMPONENT_SWIZZLE_IDENTITY;
                    switch (Swizzle)
                    {
                        case Swizzle_Identity: Result = VK_COMPONENT_SWIZZLE_IDENTITY; break;
                        case Swizzle_Zero: Result = VK_COMPONENT_SWIZZLE_ZERO; break;
                        case Swizzle_One: Result = VK_COMPONENT_SWIZZLE_ONE; break;
                        case Swizzle_R: Result = VK_COMPONENT_SWIZZLE_R; break;
                        case Swizzle_G: Result = VK_COMPONENT_SWIZZLE_G; break;
                        case Swizzle_B: Result = VK_COMPONENT_SWIZZLE_B; break;
                        case Swizzle_A: Result = VK_COMPONENT_SWIZZLE_A; break;
                        InvalidDefaultCase;
                    }
                    return(Result);
                };

                VkImageViewCreateInfo ViewInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = Image,
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
                
                VkImageView View = VK_NULL_HANDLE;
                if (vkCreateImageView(VK.Device, &ViewInfo, nullptr, &View) == VK_SUCCESS)
                {
                    Texture->ImageHandle = Image;
                    Texture->ViewHandle = View;
                    Texture->PageIndex = PageIndex;
                    Texture->PageCount = PageCount;
                    Result = true;
                }
                else
                {
                    UnhandledError("Failed to create image view");
                }
            }
            else
            {
                //UnhandledError("Failed to push image");
            }
        }
        else
        {
            UnhandledError("Failed to create image");
        }
    }
    else
    {
        UnhandledError("Invalid ID in allocate texture");
    }

    return(Result);
}