#pragma once

struct render_target
{
    VkImage Image;
    VkImageView View;
    static constexpr u32 GlobalMaxMipCount = 16;
    u32 MipCount;
    VkImageView MipViews[GlobalMaxMipCount];

    u32 MaxMipCount;
    VkFormat Format;
    VkImageUsageFlags Usage;
};

struct render_target_heap
{
    u64 MemorySize;
    VkDeviceMemory Memory;
    u32 MemoryType;

    u64 Offset;

    static constexpr u32 MaxRenderTargetCount = 256;
    u32 RenderTargetCount;
    render_target RenderTargets[MaxRenderTargetCount];
};

internal bool CreateRenderTargetHeap(render_target_heap* Heap, u64 MemorySize);

// NOTE(boti): MaxMipCount = 0 means unlimited mips (up to resolution)
internal render_target* PushRenderTarget(render_target_heap* Heap, VkFormat Format, VkImageUsageFlags Usage, u32 MaxMipCount = 1);
internal bool ResizeRenderTargets(render_target_heap* Heap, u32 Width, u32 Height);