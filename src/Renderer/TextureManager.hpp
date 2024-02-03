#pragma once

struct texture_manager
{
    static constexpr u32 MaxTextureCount = 1u << 18;
    static constexpr u32 MaxSpecialTextureCount = 2048u;

    VkDescriptorPool DescriptorPool;
    VkDescriptorSetLayout DescriptorSetLayout;
    VkDescriptorSet DescriptorSet; 
    VkDescriptorSet PackedSamplerDescriptorSet;

    u64 MemorySize;
    VkDeviceMemory Memory;
    u64 MemoryOffset;

    // TODO(boti): Move this to the renderer
    VkSampler PackedSamplers[packed_sampler::MaxSamplerCount];

    u32 TextureCount;
    VkImage Images[MaxTextureCount];
    VkImageView ImageViews[MaxTextureCount];

    u32 SpecialTextureCount;
    VkImage SpecialImages[MaxSpecialTextureCount];
    VkImageView SpecialImageViews[MaxSpecialTextureCount];
};

internal bool CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes);

internal VkImage* GetImage(texture_manager* Manager, renderer_texture_id ID);
internal VkImageView* GetImageView(texture_manager* Manager, renderer_texture_id ID);

internal renderer_texture_id
AllocateTextureName(texture_manager* Manager, texture_flags Flags);

internal b32
AllocateTexture(texture_manager* Manager, renderer_texture_id ID, texture_info Info);

internal renderer_texture_id 
CreateTexture2D(texture_manager* Manager, texture_flags Flags,
                u32 Width, u32 Height, u32 MipCount, u32 ArrayCount,
                VkFormat Format, texture_swizzle Swizzle);