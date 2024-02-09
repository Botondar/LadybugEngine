#pragma once

struct texture_manager
{
    static constexpr u32 MaxSpecialTextureCount = 2048u;

    VkDescriptorSetLayout DescriptorSetLayout;

    umm MemorySize;
    VkDeviceMemory Memory;
    umm MemoryOffset;

    VkDeviceMemory DescriptorMemory;
    umm DescriptorBufferSize;
    VkBuffer DescriptorBuffer;
    VkDeviceAddress DescriptorDeviceAddress;
    umm TextureTableOffset;
    void* DescriptorMapping;
    

    // TODO(boti): Move this to the renderer
    VkSampler PackedSamplers[packed_sampler::MaxSamplerCount];

    u32 TextureCount;
    VkImage Images[R_MaxTextureCount];
    VkImageView ImageViews[R_MaxTextureCount];
    texture_info TextureInfos[R_MaxTextureCount];
    u32 TextureMipResidency[R_MaxTextureCount];

    u32 SpecialTextureCount;
    VkImage SpecialImages[MaxSpecialTextureCount];
    VkImageView SpecialImageViews[MaxSpecialTextureCount];
    texture_info SpecialTextureInfos[MaxSpecialTextureCount];
};

// TODO(boti): Rework this API, it's horrible

internal bool 
CreateTextureManager(texture_manager* Manager, u64 MemorySize, u32 MemoryTypes, VkDescriptorSetLayout* SetLayouts);

internal VkImage* GetImage(texture_manager* Manager, renderer_texture_id ID);
internal VkImageView* GetImageView(texture_manager* Manager, renderer_texture_id ID);
internal texture_info GetTextureInfo(texture_manager* Manager, renderer_texture_id ID);

internal b32 IsTextureSpecial(renderer_texture_id ID);

internal renderer_texture_id
AllocateTexture(texture_manager* Manager, texture_flags Flags, const texture_info* Info, renderer_texture_id Placeholder);

internal b32
AllocateTexture(texture_manager* Manager, renderer_texture_id ID, texture_info Info);