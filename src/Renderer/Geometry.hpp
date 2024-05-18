struct geometry_memory
{
    static constexpr u32 MaxGPUAllocationCount = 1;
    static constexpr u64 GPUBlockSize = MiB(256);

    u32 MemoryTypeIndex;
    u32 AllocationCount;

    VkBuffer Buffer;

    u32 MaxCount;
    u32 CountInUse;
    u32 Stride;

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

internal bool CreateGeometryBuffer(u64 MaxBlockCount, memory_arena* Arena, geometry_buffer* Buffer);
internal geometry_buffer_allocation AllocateVertexBuffer(geometry_buffer* GB, u32 VertexCount, u32 IndexCount);
internal void DeallocateVertexBuffer(geometry_buffer* GB, geometry_buffer_allocation Allocation);