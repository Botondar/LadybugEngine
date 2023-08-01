#pragma once

inline bool IsValid(texture_id ID) { return ID.Value != U32_MAX; }

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

// NOTE(boti): Normally textures get put into the bindless descriptor heap,
// specifying a texture as "Special" prevents this, and it will be the user code's
// responsibility to manage its descriptor
typedef flags32 texture_flags;
enum texture_flag_bits : texture_flags
{
    TextureFlag_None = 0,
    TextureFlag_Special = (1 << 0),
};

lbfn format_byterate GetByteRate(VkFormat Format);
lbfn u64 GetMipChainSize(u32 Width, u32 Height, u32 MipCount, u32 ArrayCount, format_byterate ByteRate);

lbfn bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes);
lbfn VkImage GetImage(texture_manager* Manager, texture_id ID);
lbfn texture_id CreateTexture2D(texture_manager* Manager, texture_flags Flags,
                                u32 Width, u32 Height, u32 MipCount, u32 ArrayCount,
                                VkFormat Format, texture_swizzle Swizzle);