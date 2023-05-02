#pragma once

inline bool IsValid(texture_id ID) { return ID.Value != U32_MAX; }

struct texture_byte_rate
{
    u32 Numerator;
    u32 Denominator;
    b32 IsBlock;
};

struct texture_manager
{
    static constexpr u32 MaxTextureCount = 1u << 18;

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
};

lbfn texture_byte_rate GetByteRate(VkFormat Format);
lbfn u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, texture_byte_rate ByteRate);

lbfn bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes);
lbfn VkImage GetImage(texture_manager* Manager, texture_id ID);
lbfn texture_id CreateTexture(texture_manager* Manager, 
                              u32 Width, u32 Height, u32 MipCount, 
                              VkFormat Format, swizzle Swizzle);