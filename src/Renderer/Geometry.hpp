#pragma once

// NOTE(boti): RenderDoc can't handle sparse buffers, 
//             so this option is here for debugging.
//             Note that allocations will fail once you exceed the 256MiB limit.
#define VB_DISABLE_SPARSE 1

struct geometry_buffer_block
{
    u64 ByteSize;
    u64 ByteOffset;

    geometry_buffer_block* Next;
    geometry_buffer_block* Prev;
};

struct geometry_memory
{
#if VB_DISABLE_SPARSE
    static constexpr u32 MaxGPUAllocationCount = 1;
    static constexpr u64 GPUBlockSize = MiB(256);
#else
    static constexpr u32 MaxGPUAllocationCount = 96;
    static constexpr u64 GPUBlockSize = MiB(16);
#endif

    u32 MemoryTypeIndex;
    u32 AllocationCount;

    VkBuffer Buffer;

    u64 MemorySize;
    u64 MemoryInUse;

    u64 BlocksInUse;
    geometry_buffer_block FreeBlocks;
    geometry_buffer_block UsedBlocks;

    VkDeviceMemory MemoryBlocks[MaxGPUAllocationCount];
};

struct geometry_buffer
{
    u64 MaxBlockCount;
    geometry_buffer_block* BlockPool;

    geometry_memory VertexMemory;
    geometry_memory IndexMemory;
};

struct geometry_buffer_allocation
{
    geometry_buffer_block* VertexBlock;
    geometry_buffer_block* IndexBlock;
};

lbfn bool CreateGeometryBuffer(u64 MaxBlockCount, memory_arena* Arena, geometry_buffer* Buffer);
lbfn geometry_buffer_allocation AllocateVertexBuffer(geometry_buffer* GB, u32 VertexCount, u32 IndexCount);
lbfn void DeallocateVertexBuffer(geometry_buffer* GB, geometry_buffer_allocation Allocation);