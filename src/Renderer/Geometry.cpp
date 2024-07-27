enum class geometry_buffer_type : u32 
{
    Vertex,
    Index,
};

internal bool CreateGeometryMemory(geometry_memory* Memory, geometry_buffer_type Type)
{
    bool Result = false;

    VkBufferUsageFlags Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    u32 Stride = 0;
    switch (Type)
    {
        case geometry_buffer_type::Vertex:
        {
            Stride = sizeof(vertex);
        } break;
        case geometry_buffer_type::Index:
        {
            Stride = sizeof(vert_index);
            Usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        } break;
        InvalidDefaultCase;
    }

    Memory->BlockCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = Memory->GPUBlockSize,
        .usage = Usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &Memory->BlockCreateInfo);
    u32 MemoryTypes = MemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
    u32 MemoryTypeIndex = 0;
    if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
    {
        Memory->MemoryTypeIndex = MemoryTypeIndex;
        Memory->Stride = Stride;
        DList_Initialize(&Memory->FreeBlocks, Next, Prev);
        DList_Initialize(&Memory->UsedBlocks, Next, Prev);
    
        Result = true;
    }
    else
    {
        UnhandledError("No suitable memory type for geometry memory");
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
        geometry_buffer_block* BlockPool = PushArray(Arena, 0, geometry_buffer_block, MaxBlockCount);
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
        VkBuffer Buffer = VK_NULL_HANDLE;
        VkResult ErrorCode = vkCreateBuffer(VK.Device, &Memory->BlockCreateInfo, nullptr, &Buffer);
        if (ErrorCode == VK_SUCCESS)
        {
            VkMemoryAllocateFlagsInfo AllocFlags = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
                .pNext = nullptr,
                .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
                .deviceMask = 0,
            };
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = &AllocFlags,
                .allocationSize = Memory->GPUBlockSize,
                .memoryTypeIndex = Memory->MemoryTypeIndex,
            };
            VkDeviceMemory DeviceMemory = VK_NULL_HANDLE;
            ErrorCode = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &DeviceMemory);
            if (ErrorCode == VK_SUCCESS)
            {
                ErrorCode = vkBindBufferMemory(VK.Device, Buffer, DeviceMemory, 0);
                if (ErrorCode == VK_SUCCESS)
                {
                    u32 Index = Memory->AllocationCount++;
                    Memory->MemoryBlocks[Index] = DeviceMemory;
                    Memory->MemoryAddresses[Index] = GetBufferDeviceAddress(VK.Device, Buffer);
                    Memory->Buffers[Index] = Buffer;

                    DeviceMemory = VK_NULL_HANDLE;
                    Buffer = VK_NULL_HANDLE;

                    geometry_buffer_block* FreeBlock = BlockPool;
                    BlockPool = BlockPool->Next;
                    BlockPool->Prev = FreeBlock->Prev;
                    BlockPool->Prev->Next = BlockPool;

                    Memory->FreeBlocks.Next = Memory->FreeBlocks.Prev = FreeBlock;
                    FreeBlock->Prev = FreeBlock->Next = &Memory->FreeBlocks;

                    FreeBlock->Count = Memory->GPUBlockSize / Memory->Stride;
                    // HACK(boti): We should just go back to using byte offsets/counts
                    FreeBlock->Offset = (u32)CeilDiv(((umm)Index * Memory->GPUBlockSize), (umm)Memory->Stride);
                    Memory->MaxCount += Memory->GPUBlockSize / Memory->Stride;

                    Result = true;
                }

                vkFreeMemory(VK.Device, DeviceMemory, nullptr);
            }
            else
            {
                UnhandledError("Failed to allocate GPU vertex memory");
            }

            vkDestroyBuffer(VK.Device, Buffer, nullptr);
        }
        else
        {
            UnhandledError("Failed to create GPU geometry buffer block");
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
        DList_Remove(FreeBlock, Next, Prev);

        if (FreeBlock->Count > Count)
        {
            geometry_buffer_block* NextFreeBlock = BlockPool->Next; 
            if (NextFreeBlock)
            {
                DList_Remove(NextFreeBlock, Next, Prev);
                NextFreeBlock->Count = FreeBlock->Count - Count;
                NextFreeBlock->Offset = FreeBlock->Offset + Count;
                FreeBlock->Count = Count;
                DList_InsertFront(&Memory->FreeBlocks, NextFreeBlock, Next, Prev);

                Assert(((FreeBlock->Offset + FreeBlock->Count) <= NextFreeBlock->Offset) || 
                       ((NextFreeBlock->Offset + NextFreeBlock->Count) <= FreeBlock->Offset));
            }
            else
            {
                UnhandledError("Out of geometry buffer block pool");
            }
        }

        DList_InsertFront(&Memory->UsedBlocks, FreeBlock, Next, Prev);
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
        DList_Remove(Allocation.VertexBlock, Next, Prev);
        DList_InsertFront(&GB->VertexMemory.FreeBlocks, Allocation.VertexBlock, Next, Prev);
        GB->VertexMemory.CountInUse -= Allocation.VertexBlock->Count;
        GB->VertexMemory.BlocksInUse--;
    }

    if (Allocation.IndexBlock)
    {
        DList_Remove(Allocation.IndexBlock, Next, Prev);
        DList_InsertFront(&GB->IndexMemory.FreeBlocks, Allocation.IndexBlock, Next, Prev);
        GB->IndexMemory.CountInUse -= Allocation.IndexBlock->Count;
        GB->IndexMemory.BlocksInUse--;
    }
}

internal b32 VerifyMemoryIntegrity(geometry_memory* Memory)
{
    b32 Result = true;

    for (geometry_buffer_block* Block = Memory->UsedBlocks.Next; Block != &Memory->UsedBlocks; Block = Block->Next)
    {
        for (geometry_buffer_block* Comparand = Block->Next; Comparand != &Memory->UsedBlocks; Comparand = Comparand->Next)
        {
            b32 Check = ((Block->Offset + Block->Count) <= Comparand->Offset) || ((Comparand->Offset + Comparand->Count) <= Block->Offset);
            Result &= Check;
            Verify(Check);
        }
    }
    return(Result);
}

internal b32 VerifyGeometryIntegrity(geometry_buffer* GeometryBuffer)
{
    b32 Result = true;
    Result &= VerifyMemoryIntegrity(&GeometryBuffer->VertexMemory);
    Result &= VerifyMemoryIntegrity(&GeometryBuffer->IndexMemory);
    return(Result);
}