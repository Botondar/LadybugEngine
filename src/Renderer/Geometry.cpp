enum class geometry_buffer_type : u32 
{
    Vertex,
    Index,
};

internal bool CreateGeometryMemory(geometry_memory* Memory, geometry_buffer_type Type)
{
    bool Result = false;

    VkBufferUsageFlags Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if      (Type == geometry_buffer_type::Vertex) Usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    else if (Type == geometry_buffer_type::Index)  Usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    else InvalidCodePath;

    VkBufferCreateInfo BufferInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT|VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT,
        .size = geometry_memory::MaxGPUAllocationCount * geometry_memory::GPUBlockSize,
        .usage = Usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkResult ErrCode = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Buffer);
    if (ErrCode == VK_SUCCESS)
    {
        VkMemoryRequirements MemoryRequirements = {};
        vkGetBufferMemoryRequirements(VK.Device, Buffer, &MemoryRequirements);
        u32 MemoryTypes = MemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
        u32 MemoryTypeIndex = 0;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            *Memory = {};
            Memory->MemoryTypeIndex = MemoryTypeIndex;
            Memory->Buffer = Buffer;
            DListInitialize(&Memory->FreeBlocks);
            DListInitialize(&Memory->UsedBlocks);

            Buffer = VK_NULL_HANDLE;

            Result = true;
        }
        else
        {
            UnhandledError("No suitable memory type for geometry memory");
        }

        vkDestroyBuffer(VK.Device, Buffer, nullptr);
    }
    else
    {
        UnhandledError("Failed to create geometry memory");
    }

    return Result;
}

lbfn bool CreateGeometryBuffer(u64 MaxBlockCount, memory_arena* Arena, geometry_buffer* GeometryBuffer)
{
    bool Result = false;
    *GeometryBuffer = {};

    bool VertexBufferResult = CreateGeometryMemory(&GeometryBuffer->VertexMemory, geometry_buffer_type::Vertex);
    bool IndexBufferResult  = CreateGeometryMemory(&GeometryBuffer->IndexMemory, geometry_buffer_type::Index);

    if (VertexBufferResult && IndexBufferResult)
    {
        geometry_buffer_block* BlockPool = PushArray<geometry_buffer_block>(Arena, MaxBlockCount);
        if (BlockPool)
        {
            for (u64 i = 0; i < MaxBlockCount; i++)
            {
                BlockPool[i].ByteSize = 0;
                BlockPool[i].ByteOffset = 0;
                if (i < MaxBlockCount - 1)
                {
                    BlockPool[i + 0].Next = BlockPool + (i + 1);
                    BlockPool[i + 1].Prev = BlockPool + (i + 0);
                }
                else
                {
                    BlockPool[i].Next = BlockPool + 0;
                    BlockPool[0].Prev = BlockPool + i;
                }
            }

            GeometryBuffer->MaxBlockCount = MaxBlockCount;
            GeometryBuffer->BlockPool = BlockPool;
            Result = true;
        }
    }
    return Result;
}

internal bool AllocateGPUBlocks(geometry_memory* Memory, geometry_buffer_block*& BlockPool)
{
    bool Result = false;

    if (Memory->AllocationCount < Memory->MaxGPUAllocationCount)
    {
        VkMemoryAllocateInfo AllocInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = Memory->GPUBlockSize,
            .memoryTypeIndex = Memory->MemoryTypeIndex,
        };
        VkDeviceMemory DeviceMemory = VK_NULL_HANDLE;
        VkResult AllocResult = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &DeviceMemory);
        if (AllocResult == VK_SUCCESS)
        {
            VkSparseMemoryBind MemoryBind = 
            {
                .resourceOffset = Memory->MemorySize,
                .size = Memory->GPUBlockSize,
                .memory = DeviceMemory,
                .memoryOffset = 0,
                .flags = 0,
            };
            VkSparseBufferMemoryBindInfo BufferBind = 
            {
                .buffer = Memory->Buffer,
                .bindCount = 1,
                .pBinds = &MemoryBind,
            };
            VkBindSparseInfo BindInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .bufferBindCount = 1,
                .pBufferBinds = &BufferBind,
                .imageOpaqueBindCount = 0,
                .pImageOpaqueBinds = nullptr,
                .imageBindCount = 0,
                .pImageBinds = nullptr,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = nullptr,
            };
            VkResult BindResult = vkQueueBindSparse(VK.GraphicsQueue, 1, &BindInfo, VK_NULL_HANDLE);
            if (BindResult == VK_SUCCESS)
            {
                geometry_buffer_block* Block = nullptr;

                geometry_buffer_block* FreeBlock = Memory->FreeBlocks.Next;
                if ((FreeBlock != &Memory->FreeBlocks) && 
                    Memory->MemorySize == (FreeBlock->ByteOffset + FreeBlock->ByteSize))
                {
                    Block = FreeBlock;
                }
                else
                {
                    Block = FreeListAllocate(BlockPool);
                    *Block = {};
                    Block->ByteOffset = Memory->MemorySize;
                    DListInsert(&Memory->FreeBlocks, Block);
                }

                if (Block) 
                {
                    Block->ByteSize += Memory->GPUBlockSize;

                    Memory->MemorySize += Memory->GPUBlockSize;
                    Memory->MemoryBlocks[Memory->AllocationCount++] = DeviceMemory;
                    DeviceMemory = VK_NULL_HANDLE;

                    Result = true;
                }
                else
                {
                    UnhandledError("Out of vertex buffer block pool");
                }
            }
            else
            {
                UnhandledError("Failed to bind GPU vertex memory");
            }
            vkQueueWaitIdle(VK.GraphicsQueue);

            vkFreeMemory(VK.Device, DeviceMemory, nullptr);
        }
        else
        {
            UnhandledError("Failed to allocate GPU vertex memory");
        }
    }
    else
    {
        UnhandledError("Out of GPU vertex memory");
    }
    return Result;
}

internal geometry_buffer_block* FindCandidateBlock(geometry_memory* Memory, u64 ByteSize)
{
    geometry_buffer_block* Result = nullptr;

    for (geometry_buffer_block* Candidate = Memory->FreeBlocks.Next;
         Candidate != &Memory->FreeBlocks;
         Candidate = Candidate->Next)
    {
        if (ByteSize <= Candidate->ByteSize)
        {
            Result = Candidate;
            break;
        }
    }
    return Result;
}

internal void DefragmentGPUBuffer(geometry_memory* Memory, geometry_buffer_block* BlockPool)
{
    geometry_buffer_block* It = Memory->FreeBlocks.Next;
    while (It != &Memory->FreeBlocks)
    {
        geometry_buffer_block* Next = It->Next;
        if (Next != &Memory->FreeBlocks)
        {
            if (It->ByteOffset == (Next->ByteOffset + Next->ByteSize) ||
                Next->ByteOffset == (It->ByteOffset + It->ByteSize))
            {
                It->ByteOffset = Min(It->ByteOffset, Next->ByteOffset);
                It->ByteSize = It->ByteSize + Next->ByteSize;
                DListRemove(Next);
                FreeListFree(BlockPool, Next);
            }
            else
            {
                It = Next;
            }
        }
        else
        {
            It = Next;
        }
    }
}

internal geometry_buffer_block* AllocateSubBuffer(geometry_memory* Memory, u64 AllocationSize, geometry_buffer_block*& BlockPool)
{
    geometry_buffer_block* Result = nullptr;

    while (Memory->MemorySize - Memory->MemoryInUse < AllocationSize)
    {
        if (!AllocateGPUBlocks(Memory, BlockPool))
        {
            return Result;
        }
    }

    geometry_buffer_block* FreeBlock = FindCandidateBlock(Memory, AllocationSize);
    if (!FreeBlock)
    {
        DefragmentGPUBuffer(Memory, BlockPool);
        FreeBlock = FindCandidateBlock(Memory, AllocationSize);
    }

    if (FreeBlock)
    {
        DListRemove(FreeBlock);

        if (FreeBlock->ByteSize > AllocationSize)
        {
            geometry_buffer_block* NextFreeBlock = FreeListAllocate(BlockPool);
            NextFreeBlock->ByteSize = FreeBlock->ByteSize - AllocationSize;
            NextFreeBlock->ByteOffset = FreeBlock->ByteOffset + AllocationSize;
            FreeBlock->ByteSize = AllocationSize;
            DListInsert(&Memory->FreeBlocks, NextFreeBlock);
        }

        DListInsert(&Memory->UsedBlocks, FreeBlock);
        Memory->MemoryInUse += FreeBlock->ByteSize;
        Memory->BlocksInUse++;

        Result = FreeBlock;
    }
    else
    {
        UnhandledError("Failed to allocate vertex buffer block");
    }

    return Result;
}

lbfn geometry_buffer_allocation AllocateVertexBuffer(geometry_buffer* GB, u32 VertexCount, u32 IndexCount)
{
    geometry_buffer_allocation Result = {};

    if (VertexCount)
    {
        u64 VertexMemorySize = u64(VertexCount) * sizeof(vertex);
        Result.VertexBlock = AllocateSubBuffer(&GB->VertexMemory, VertexMemorySize, GB->BlockPool); 

        if (IndexCount)
        {
            u64 IndexMemorySize = u64(IndexCount) * sizeof(vert_index);
            Result.IndexBlock = AllocateSubBuffer(&GB->IndexMemory, IndexMemorySize, GB->BlockPool);
        }
    }

    return Result;
}

lbfn void DeallocateVertexBuffer(geometry_buffer* GB, geometry_buffer_allocation Allocation)
{
    if (Allocation.VertexBlock)
    {
        DListRemove(Allocation.VertexBlock);
        DListInsert(&GB->VertexMemory.FreeBlocks, Allocation.VertexBlock);
        GB->VertexMemory.MemoryInUse -= Allocation.VertexBlock->ByteSize;
        GB->VertexMemory.BlocksInUse--;
    }

    if (Allocation.IndexBlock)
    {
        DListRemove(Allocation.IndexBlock);
        DListInsert(&GB->IndexMemory.FreeBlocks, Allocation.IndexBlock);
        GB->IndexMemory.MemoryInUse -= Allocation.VertexBlock->ByteSize;
        GB->IndexMemory.BlocksInUse--;
    }
}