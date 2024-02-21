#pragma once

struct render_target
{
    const char* Name;
    VkImage Image;
    VkImageView View;
    u32 MipCount;
    VkImageView MipViews[R_MaxMipCount];

    u32 MaxMipCount;
    VkFormat Format;
    VkImageUsageFlags Usage;
};

struct render_target_heap
{
    gpu_memory_arena Arena;

    static constexpr u32 MaxRenderTargetCount = 256;
    u32 RenderTargetCount;
    render_target RenderTargets[MaxRenderTargetCount];
};

internal bool CreateRenderTargetHeap(render_target_heap* Heap, u64 MemorySize);

// NOTE(boti): MaxMipCount = 0 means unlimited mips (up to resolution)
internal render_target* PushRenderTarget(const char* Name, render_target_heap* Heap, VkFormat Format, VkImageUsageFlags Usage, u32 MaxMipCount = 1);
internal bool ResizeRenderTargets(render_target_heap* Heap, u32 Width, u32 Height);