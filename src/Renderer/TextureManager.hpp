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

constexpr umm TexturePageSize       = KiB(64);
constexpr umm SmallTexturePageSize  = KiB(4);

struct renderer_texture
{
    VkImage         ImageHandle;
    VkImageView     ViewHandle;
    texture_info    Info;
    texture_flags   Flags;
    u32             MipResidencyMask;

    umm             PageIndex;
    umm             PageCount;
};

struct texture_cache
{
    umm SmallPageCount;
    u64* SmallPageUsage;

    umm PageCount;
    u64* PageUsage;

    umm UsedPageCount;
};

internal umm 
FindFreePageRange(texture_cache* Cache, umm PageCount);

internal void 
MarkPagesAsUsed(texture_cache* Cache, umm FirstPage, umm PageCount);

internal void
MarkPagesAsFree(texture_cache* Cache, umm FirstPage, umm PageCount);

// TODO(boti): The device memory should be a part of the cache in the future
internal b32
AllocateImage(texture_cache* Cache, VkDeviceMemory Memory, VkImage Image);

struct texture_manager
{
    static constexpr u32 MaxSpecialTextureCount = 2048u;

    VkDescriptorSetLayout DescriptorSetLayout;

    gpu_memory_arena    PersistentArena;
    // TODO(boti): Remove the cache arena and move the memory object to the cache
    gpu_memory_arena    CacheArena;
    texture_cache       Cache;

    gpu_memory_arena    DescriptorArena;
    VkBuffer            DescriptorBuffer;
    VkDeviceAddress     DescriptorAddress;
    void*               DescriptorMapping;
    umm                 TextureTableOffset;

    u32                 TextureCount;
    renderer_texture    Textures[R_MaxTextureCount];

    u32                 SpecialTextureCount;
    renderer_texture    SpecialTextures[MaxSpecialTextureCount];
};

// TODO(boti): Rework this API, it's horrible

internal bool 
CreateTextureManager(texture_manager* Manager, memory_arena* Arena, u64 MemorySize, u32 MemoryTypes, VkDescriptorSetLayout* SetLayouts);

internal renderer_texture* 
GetTexture(texture_manager* Manager, renderer_texture_id ID);

internal b32 IsTextureSpecial(renderer_texture_id ID);

internal renderer_texture_id
AllocateTexture(texture_manager* Manager, texture_flags Flags, const texture_info* Info, renderer_texture_id Placeholder);

internal b32
AllocateTexture(texture_manager* Manager, renderer_texture_id ID, texture_info Info);