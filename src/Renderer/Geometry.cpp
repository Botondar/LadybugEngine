enum class geometry_buffer_type : u32 
{
    Vertex,
    Index,
};

internal bool CreateGeometryMemory(geometry_memory* Memory, geometry_buffer_type Type)
{
    bool Result = false;

    VkBufferUsageFlags Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    u32 Stride = 0;
    switch (Type)
    {
        case geometry_buffer_type::Vertex:
        {
            Stride = sizeof(vertex);
            Usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        } break;
        case geometry_buffer_type::Index:
        {
            Stride = sizeof(vert_index);
            Usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        } break;
        InvalidDefaultCase;
    }

    VkBufferCreateInfo BufferInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
#if VB_DISABLE_SPARSE
        .flags = 0,
#else
        .flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT|VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT,
#endif
        .size = geometry_memory::GPUBlockSize * geometry_memory::MaxGPUAllocationCount,
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
            Memory->Stride = Stride;
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

internal bool CreateGeometryBuffer(u64 MaxBlockCount, memory_arena* Arena, geometry_buffer* GeometryBuffer)
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
                BlockPool[i].Count = 0;
                BlockPool[i].Offset = 0;
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
#if VB_DISABLE_SPARSE
            if (vkBindBufferMemory(VK.Device, Memory->Buffer, DeviceMemory, 0) == VK_SUCCESS)
            {
                geometry_buffer_block* FreeBlock = BlockPool;
                BlockPool = BlockPool->Next;
                BlockPool->Prev = FreeBlock->Prev;
                BlockPool->Prev->Next = BlockPool;

                Memory->FreeBlocks.Next = Memory->FreeBlocks.Prev = FreeBlock;
                FreeBlock->Prev = FreeBlock->Next = &Memory->FreeBlocks;

                FreeBlock->Count = Memory->GPUBlockSize / Memory->Stride;
                FreeBlock->Offset = 0;
                Memory->MaxCount += Memory->GPUBlockSize / Memory->Stride;

                Result = true;
            }
#else
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
#endif
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

internal geometry_buffer_block* FindCandidateBlock(geometry_memory* Memory, u32 Count)
{
    geometry_buffer_block* Result = nullptr;

    for (geometry_buffer_block* Candidate = Memory->FreeBlocks.Next;
         Candidate != &Memory->FreeBlocks;
         Candidate = Candidate->Next)
    {
        if (Count <= Candidate->Count)
        {
            Result = Candidate;
            break;
        }
    }
    return Result;
}

internal geometry_buffer_block* AllocateSubBuffer(geometry_memory* Memory, u32 Count, geometry_buffer_block*& BlockPool)
{
    geometry_buffer_block* Result = nullptr;

    while (Memory->MaxCount - Memory->CountInUse < Count)
    {
        if (!AllocateGPUBlocks(Memory, BlockPool))
        {
            UnhandledError("Out of geometry memory");
            return Result;
        }
    }

    geometry_buffer_block* FreeBlock = FindCandidateBlock(Memory, Count);
    if (FreeBlock)
    {
        DListRemove(FreeBlock);

        if (FreeBlock->Count > Count)
        {
            geometry_buffer_block* NextFreeBlock = FreeListAllocate(BlockPool);
            NextFreeBlock->Count = FreeBlock->Count - Count;
            NextFreeBlock->Offset = FreeBlock->Offset + Count;
            FreeBlock->Count = Count;
            DListInsert(&Memory->FreeBlocks, NextFreeBlock);
        }

        DListInsert(&Memory->UsedBlocks, FreeBlock);
        Memory->CountInUse += FreeBlock->Count;
        Memory->BlocksInUse++;

        Result = FreeBlock;
    }
    else
    {
        UnhandledError("Failed to allocate vertex buffer block");
    }

    return Result;
}

internal geometry_buffer_allocation AllocateVertexBuffer(geometry_buffer* GB, u32 VertexCount, u32 IndexCount)
{
    geometry_buffer_allocation Result = {};

    if (VertexCount)
    {
        Result.VertexBlock = AllocateSubBuffer(&GB->VertexMemory, VertexCount, GB->BlockPool); 

        if (IndexCount)
        {
            Result.IndexBlock = AllocateSubBuffer(&GB->IndexMemory, IndexCount, GB->BlockPool);
        }
    }

    return Result;
}

internal void DeallocateVertexBuffer(geometry_buffer* GB, geometry_buffer_allocation Allocation)
{
    if (Allocation.VertexBlock)
    {
        DListRemove(Allocation.VertexBlock);
        DListInsert(&GB->VertexMemory.FreeBlocks, Allocation.VertexBlock);
        GB->VertexMemory.CountInUse -= Allocation.VertexBlock->Count;
        GB->VertexMemory.BlocksInUse--;
    }

    if (Allocation.IndexBlock)
    {
        DListRemove(Allocation.IndexBlock);
        DListInsert(&GB->IndexMemory.FreeBlocks, Allocation.IndexBlock);
        GB->IndexMemory.CountInUse -= Allocation.IndexBlock->Count;
        GB->IndexMemory.BlocksInUse--;
    }
}