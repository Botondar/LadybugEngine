#pragma once

struct texture_manager
{
    static constexpr u32 MaxTextureCount = 1u << 18;
    static constexpr u32 MaxSpecialTextureCount = 2048u;

    VkDescriptorPool DescriptorPool;
    VkDescriptorSetLayout DescriptorSetLayouts[2]; // NOTE(boti): 0 = sampler, 1 = texture
    VkDescriptorSet DescriptorSets[2]; 

    u64 MemorySize;
    VkDeviceMemory Memory;
    u64 MemoryOffset;

    VkSampler Sampler;

    u32 TextureCount;
    VkImage Images[MaxTextureCount];
    VkImageView ImageViews[MaxTextureCount];

    u32 SpecialTextureCount;
    VkImage SpecialImages[MaxSpecialTextureCount];
    VkImageView SpecialImageViews[MaxSpecialTextureCount];
};

u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, u32 ArrayCount, format_byterate ByteRate);

bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes);

VkImage GetImage(texture_manager* Manager, texture_id ID);
VkImageView GetImageView(texture_manager* Manager, texture_id ID);

texture_id CreateTexture2D(texture_manager* Manager, texture_flags Flags,
                                u32 Width, u32 Height, u32 MipCount, u32 ArrayCount,
                                VkFormat Format, texture_swizzle Swizzle);