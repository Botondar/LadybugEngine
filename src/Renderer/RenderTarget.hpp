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

    u32 Binding;
    u32 StorageBinding;
    u32 BindingGeneral; // NOTE(boti): Sampled access in general layout
    u32 MipBinding;
};

struct render_target_heap
{
    gpu_memory_arena Arena;

    static constexpr u32 MaxRenderTargetCount = 32;
    v2u CurrentExtent;
    u32 RenderTargetCount;
    render_target RenderTargets[MaxRenderTargetCount];

    VkDescriptorSetLayout SetLayout;
    void* DescriptorBuffer;
};

internal bool CreateRenderTargetHeap(render_target_heap* Heap, u64 MemorySize, VkDescriptorSetLayout SetLayout, void* DescriptorBuffer);

// NOTE(boti): MaxMipCount = 0 means unlimited mips (up to resolution)
internal render_target* PushRenderTarget(
    const char* Name, 
    render_target_heap* Heap,
    VkFormat Format, 
    VkImageUsageFlags Usage,
    u32 MaxMipCount,
    u32 Binding,
    u32 StorageBinding,
    u32 BindingGeneral,
    u32 StorageMipBinding);
internal bool ResizeRenderTargets(render_target_heap* Heap, v2u Extent);