#pragma once

#if 0
struct tile_size_entry
{
    u32 BitDepth;
    v3u TileSize;
    v3u TileSize3;
};

// TODO(boti): Assert standard sparse sizes
static const tile_size_entry StandardTileSizes[] = 
{
    { 8,    { 256, 256, 1 }, { 64, 32, 32 } },
    { 16,   { 256, 128, 1 }, { 32, 32, 32 } },
    { 32,   { 128, 128, 1 }, { 32, 32, 16 } },
    { 64,   { 128,  64, 1 }, { 32, 16, 16 } },
    { 128,  {  64,  64, 1 }, { 16, 16, 16 } },
};

/*
512MB / 64k -> 8192

Mip inclusive:
 128 x  128 =    1 | 4540 ---    0
 256 x  256 =    2 |  256 --- 4540
 512 x  512 =    6 |   64 --- 5052
1024 x 1024 =   22 |   16 --- 5436
2048 x 2048 =  278 |    4 --- 5788
4096 x 4096 = 1302 |    1 --- 6890

Mip exclusive (requires sparse binding/residency):
 128 x  128 =    1 | 4608 --- 0
 256 x  256 =    1 | 1024 --- 4608
 512 x  512 =    4 |  256 --- 5632
1024 x 1024 =   16 |   16 --- 6656
2048 x 2048 =  256 |    4 --- 6912
4096 x 4096 = 1024 |    1 --- 7168
*/
#endif

constexpr umm TexturePageSize = KiB(64);

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
    u32 MipResidencyMask[R_MaxTextureCount];

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