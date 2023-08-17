#include "VulkanRenderer.hpp"

vulkan VK;

//#define ReturnOnFailure() if (Result != VK_SUCCESS) return Result
#define ReturnOnFailure() if (Result != VK_SUCCESS) return nullptr

static VkFormat FormatTable[Format_Count] = 
{
    [Format_Undefined]          = VK_FORMAT_UNDEFINED,
    [Format_R8_UNorm]           = VK_FORMAT_R8_UNORM,
    [Format_R8_UInt]            = VK_FORMAT_R8_UINT,
    [Format_R8_SRGB]            = VK_FORMAT_R8_SRGB,
    [Format_R8G8_UNorm]         = VK_FORMAT_R8G8_UNORM,
    [Format_R8G8_UInt]          = VK_FORMAT_R8G8_UINT,
    [Format_R8G8_SRGB]          = VK_FORMAT_R8G8_SRGB,
    [Format_R8G8B8_UNorm]       = VK_FORMAT_R8G8B8_UNORM,
    [Format_R8G8B8_UInt]        = VK_FORMAT_R8G8B8_UINT,
    [Format_R8G8B8_SRGB]        = VK_FORMAT_R8G8B8_SRGB,
    [Format_R8G8B8A8_UNorm]     = VK_FORMAT_R8G8B8A8_UNORM,
    [Format_R8G8B8A8_UInt]      = VK_FORMAT_R8G8B8A8_UINT,
    [Format_R8G8B8A8_SRGB]      = VK_FORMAT_R8G8B8A8_SRGB,
    [Format_R16_UNorm]          = VK_FORMAT_R16_UNORM,
    [Format_R16_UInt]           = VK_FORMAT_R16_UINT,
    [Format_R16_Float]          = VK_FORMAT_R16_SFLOAT,
    [Format_R16G16_UNorm]       = VK_FORMAT_R16G16_UNORM,
    [Format_R16G16_UInt]        = VK_FORMAT_R16G16_UINT,
    [Format_R16G16_Float]       = VK_FORMAT_R16G16_SFLOAT,
    [Format_R16G16B16_UNorm]    = VK_FORMAT_R16G16B16_UNORM,
    [Format_R16G16B16_UInt]     = VK_FORMAT_R16G16B16_UINT,
    [Format_R16G16B16_Float]    = VK_FORMAT_R16G16B16_SFLOAT,
    [Format_R16G16B16A16_UNorm] = VK_FORMAT_R16G16B16A16_UNORM,
    [Format_R16G16B16A16_UInt]  = VK_FORMAT_R16G16B16A16_UINT,
    [Format_R16G16B16A16_Float] = VK_FORMAT_R16G16B16A16_SFLOAT,
    [Format_R32_UInt]           = VK_FORMAT_R32_UINT,
    [Format_R32_Float]          = VK_FORMAT_R32_SFLOAT,
    [Format_R32G32_UInt]        = VK_FORMAT_R32G32_UINT,
    [Format_R32G32_Float]       = VK_FORMAT_R32G32_SFLOAT,
    [Format_R32G32B32_UInt]     = VK_FORMAT_R32G32B32_UINT,
    [Format_R32G32B32_Float]    = VK_FORMAT_R32G32B32_SFLOAT,
    [Format_R32G32B32A32_UInt]  = VK_FORMAT_R32G32B32A32_UINT,
    [Format_R32G32B32A32_Float] = VK_FORMAT_R32G32B32A32_SFLOAT,
    [Format_R11G11B10_Float]    = VK_FORMAT_B10G11R11_UFLOAT_PACK32,
    [Format_D16]                = VK_FORMAT_D16_UNORM,
    [Format_D24X8]              = VK_FORMAT_X8_D24_UNORM_PACK32,
    [Format_D32]                = VK_FORMAT_D32_SFLOAT,
    [Format_S8]                 = VK_FORMAT_S8_UINT,
    [Format_D24S8]              = VK_FORMAT_D24_UNORM_S8_UINT,
    [Format_D32S8]              = VK_FORMAT_D32_SFLOAT_S8_UINT,
    [Format_BC1_RGB_UNorm]      = VK_FORMAT_BC1_RGB_UNORM_BLOCK,
    [Format_BC1_RGB_SRGB]       = VK_FORMAT_BC1_RGB_SRGB_BLOCK,
    [Format_BC1_RGBA_UNorm]     = VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
    [Format_BC1_RGBA_SRGB]      = VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
    [Format_BC2_UNorm]          = VK_FORMAT_BC2_UNORM_BLOCK,
    [Format_BC2_SRGB]           = VK_FORMAT_BC2_SRGB_BLOCK,
    [Format_BC3_UNorm]          = VK_FORMAT_BC3_UNORM_BLOCK,
    [Format_BC3_SRGB]           = VK_FORMAT_BC3_SRGB_BLOCK,
    [Format_BC4_UNorm]          = VK_FORMAT_BC4_UNORM_BLOCK,
    [Format_BC4_SNorm]          = VK_FORMAT_BC4_SNORM_BLOCK,
    [Format_BC5_UNorm]          = VK_FORMAT_BC5_UNORM_BLOCK,
    [Format_BC5_SNorm]          = VK_FORMAT_BC5_SNORM_BLOCK,
    [Format_BC6_UFloat]         = VK_FORMAT_BC6H_UFLOAT_BLOCK,
    [Format_BC6_SFloat]         = VK_FORMAT_BC6H_SFLOAT_BLOCK,
    [Format_BC7_UNorm]          = VK_FORMAT_BC7_UNORM_BLOCK,
    [Format_BC7_SRGB]           = VK_FORMAT_BC7_SRGB_BLOCK,
};

static VkPrimitiveTopology TopologyTable[Topology_Count] = 
{
    [Topology_Undefined]                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,

    [Topology_PointList]                = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    [Topology_LineList]                 = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    [Topology_LineStrip]                = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
    [Topology_TriangleList]             = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    [Topology_TriangleStrip]            = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    [Topology_TriangleFan]              = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
    [Topology_LineListAdjacency]        = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
    [Topology_LineStripAdjacency]       = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
    [Topology_TriangleListAdjacency]    = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
    [Topology_TriangleStripAdjacency]   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
    [Topology_PatchList]                = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
};

static VkFilter FilterTable[Filter_Count] = 
{
    [Filter_Nearest] = VK_FILTER_NEAREST,
    [Filter_Linear]  = VK_FILTER_LINEAR,
};

static VkSamplerMipmapMode MipFilterTable[Filter_Count] = 
{
    [Filter_Nearest] = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    [Filter_Linear]  = VK_SAMPLER_MIPMAP_MODE_LINEAR,
};

static VkSamplerAddressMode WrapTable[Wrap_Count] = 
{
    [Wrap_Repeat]           = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    [Wrap_RepeatMirror]     = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    [Wrap_ClampToEdge]      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    [Wrap_ClampToBorder]    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
};

static VkBorderColor BorderTable[Border_Count] = 
{
    [Border_Black] = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    [Border_White] = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
};

static VkBlendOp BlendOpTable[BlendOp_Count] = 
{
    [BlendOp_Add] = VK_BLEND_OP_ADD,
    [BlendOp_Subtract] = VK_BLEND_OP_SUBTRACT,
    [BlendOp_ReverseSubtract] = VK_BLEND_OP_REVERSE_SUBTRACT,
    [BlendOp_Min] = VK_BLEND_OP_MIN,
    [BlendOp_Max] = VK_BLEND_OP_MAX,
};

static VkBlendFactor BlendFactorTable[Blend_Count] = 
{
    [Blend_Zero]                = VK_BLEND_FACTOR_ZERO,
    [Blend_One]                 = VK_BLEND_FACTOR_ONE,
    [Blend_SrcColor]            = VK_BLEND_FACTOR_SRC_COLOR,
    [Blend_InvSrcColor]         = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    [Blend_DstColor]            = VK_BLEND_FACTOR_DST_COLOR,
    [Blend_InvDstColor]         = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    [Blend_SrcAlpha]            = VK_BLEND_FACTOR_SRC_ALPHA,
    [Blend_InvSrcAlpha]         = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    [Blend_DstAlpha]            = VK_BLEND_FACTOR_DST_ALPHA,
    [Blend_InvDstAlpha]         = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    [Blend_ConstantColor]       = VK_BLEND_FACTOR_CONSTANT_COLOR,
    [Blend_InvConstantColor]    = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    [Blend_ConstantAlpha]       = VK_BLEND_FACTOR_CONSTANT_ALPHA,
    [Blend_InvConstantAlpha]    = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
};

static VkCompareOp CompareOpTable[Compare_Count] = 
{
    [Compare_Never]         = VK_COMPARE_OP_NEVER,
    [Compare_Always]        = VK_COMPARE_OP_ALWAYS,
    [Compare_Equal]         = VK_COMPARE_OP_EQUAL,
    [Compare_NotEqual]      = VK_COMPARE_OP_NOT_EQUAL,
    [Compare_Less]          = VK_COMPARE_OP_LESS,
    [Compare_LessEqual]     = VK_COMPARE_OP_LESS_OR_EQUAL,
    [Compare_Greater]       = VK_COMPARE_OP_GREATER,
    [Compare_GreaterEqual]  = VK_COMPARE_OP_GREATER_OR_EQUAL,
};

internal VkShaderStageFlags PipelineStagesToVulkan(flags32 Stages)
{
    VkShaderStageFlags Result = 0;
    if (Stages == PipelineStage_All)
    {
        Result = VK_SHADER_STAGE_ALL;
    }
    else if (Stages == PipelineStage_AllGfx)
    {
        Result = VK_SHADER_STAGE_ALL_GRAPHICS;
    }
    else 
    {
        if (HasFlag(Stages, PipelineStage_VS)) Result |= VK_SHADER_STAGE_VERTEX_BIT;
        if (HasFlag(Stages, PipelineStage_PS)) Result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (HasFlag(Stages, PipelineStage_HS)) Result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (HasFlag(Stages, PipelineStage_DS)) Result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (HasFlag(Stages, PipelineStage_GS)) Result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (HasFlag(Stages, PipelineStage_CS)) Result |= VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return(Result);
}

internal VkMemoryRequirements 
GetBufferMemoryRequirements(VkDevice Device, const VkBufferCreateInfo* BufferInfo)
{
    VkMemoryRequirements Result = {};

    VkMemoryRequirements2 MemoryRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkDeviceBufferMemoryRequirements Query = 
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
        .pNext = nullptr,
        .pCreateInfo = BufferInfo,
    };
    vkGetDeviceBufferMemoryRequirements(Device, &Query, &MemoryRequirements);
    Result = MemoryRequirements.memoryRequirements;

    return(Result);
}

internal VkResult ResizeRenderTargets(renderer* Renderer);

internal VkResult CreateAndAllocateBuffer(VkBufferUsageFlags Usage, u32 MemoryTypes, size_t Size, 
                                          VkBuffer* pBuffer, VkDeviceMemory* pMemory)
{
    VkResult Result = VK_SUCCESS;

    VkBufferCreateInfo BufferInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = Size,
        .usage = Usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkBuffer Buffer = VK_NULL_HANDLE;
    if ((Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Buffer)) == VK_SUCCESS)
    {
        VkMemoryRequirements MemoryRequirements = {};
        vkGetBufferMemoryRequirements(VK.Device, Buffer, &MemoryRequirements);
        Assert(MemoryRequirements.size == Size);
        MemoryTypes &= MemoryRequirements.memoryTypeBits;

        u32 MemoryTypeIndex;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = Size,
                .memoryTypeIndex = MemoryTypeIndex,
            };

            VkDeviceMemory Memory = VK_NULL_HANDLE;
            if ((Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Memory)) == VK_SUCCESS)
            {
                if ((Result = vkBindBufferMemory(VK.Device, Buffer, Memory, 0)) == VK_SUCCESS)
                {
                    *pBuffer = Buffer;
                    Buffer = VK_NULL_HANDLE;
                    *pMemory = Memory;
                    Memory = VK_NULL_HANDLE;
                }
            }

            vkFreeMemory(VK.Device, Memory, nullptr);
        }
    }

    vkDestroyBuffer(VK.Device, Buffer, nullptr);
    return Result;
}

const char* GetDeviceName(renderer* Renderer)
{
    const char* Result = Renderer->Vulkan.DeviceProps.deviceName;
    return(Result);
}

renderer* CreateRenderer(memory_arena* Arena, memory_arena* TempArena)
{
    renderer* Renderer = PushStruct<renderer>(Arena);
    if (!Renderer) return nullptr;
    VkResult Result = InitializeVulkan(&Renderer->Vulkan);
    ReturnOnFailure();
    VK = Renderer->Vulkan;

    {
        VkCommandPoolCreateInfo PoolInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = VK.GraphicsQueueIdx,
        };
        Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->TransferCmdPool);
        ReturnOnFailure();

        VkCommandBufferAllocateInfo BufferInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = Renderer->TransferCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->TransferCmdBuffer);
        ReturnOnFailure();
    }

    // Surface
    {
        Renderer->Surface = Platform.CreateVulkanSurface(VK.Instance);
        if (!Renderer->Surface) return nullptr;

        constexpr u32 MaxSurfaceFormatCount = 32;
        u32 SurfaceFormatCount = MaxSurfaceFormatCount;
        VkSurfaceFormatKHR SurfaceFormats[MaxSurfaceFormatCount];
        Result = vkGetPhysicalDeviceSurfaceFormatsKHR(VK.PhysicalDevice, Renderer->Surface, &SurfaceFormatCount, SurfaceFormats);
        ReturnOnFailure();

        for (u32 i = 0; i < SurfaceFormatCount; i++)
        {
            if (SurfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                SurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                Renderer->SurfaceFormat = SurfaceFormats[i];
            }
        }

        Renderer->SwapchainImageCount = Renderer->MaxSwapchainImageCount;
        // HACK(boti):
        for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
        {
            Renderer->Frames[i].Backend = Renderer->BackendFrames + i;
        }
        Result = ResizeRenderTargets(Renderer);
        ReturnOnFailure();
    }

    for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
    {
        VkFenceCreateInfo ImageFenceInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        Result = vkCreateFence(VK.Device, &ImageFenceInfo, nullptr, &Renderer->ImageAcquiredFences[i]);
        ReturnOnFailure();

        VkSemaphoreCreateInfo SemaphoreInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        Result = vkCreateSemaphore(VK.Device, &SemaphoreInfo, nullptr, &Renderer->ImageAcquiredSemaphores[i]);
        ReturnOnFailure();

        VkSemaphoreTypeCreateInfo SemaphoreTypeInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = 0,
        };
        VkSemaphoreCreateInfo TimelineSemaphoreInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &SemaphoreTypeInfo,
            .flags = 0,
        };
        Result = vkCreateSemaphore(VK.Device, &TimelineSemaphoreInfo, nullptr, &Renderer->TimelineSemaphore);
        ReturnOnFailure();
        Renderer->TimelineSemaphoreCounter = 0;

        Result = vkCreateSemaphore(VK.Device, &TimelineSemaphoreInfo, nullptr, &Renderer->ComputeTimelineSemaphore);
        ReturnOnFailure();
        Renderer->ComputeTimelineSemaphoreCounter = 0;

        // Graphics command pool + buffers
        {
            VkCommandPoolCreateInfo PoolInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = VK.GraphicsQueueIdx,
            };
            Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->CmdPools[i]);
            ReturnOnFailure();
        
            for (u32 BufferIndex = 0; BufferIndex < backend_render_frame::MaxCmdBufferCount; BufferIndex++)
            {
                VkCommandBufferAllocateInfo BufferInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .commandPool = Renderer->CmdPools[i],
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1,
                };
                Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->CmdBuffers[i][BufferIndex]);
                ReturnOnFailure();
            }
        }
        
        // Compute command pool + buffers
        {
            VkCommandPoolCreateInfo PoolInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = VK.ComputeQueueIdx,
            };
            Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->ComputeCmdPools[i]);
            ReturnOnFailure();
        
            VkCommandBufferAllocateInfo BufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = Renderer->ComputeCmdPools[i],
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->ComputeCmdBuffers[i]);
            ReturnOnFailure();
        }
        
    }

    // Create samplers
    {
        Renderer->Samplers[Sampler_None] = VK_NULL_HANDLE;
        for (u32 Index = 1; Index < Sampler_Count; Index++)
        {
            const sampler_state* Sampler = SamplerInfos + Index;
            f32 AnisotropyTable[Anisotropy_Count] = { 1.0f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
            VkSamplerCreateInfo Info = 
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = FilterTable[Sampler->MagFilter],
                .minFilter = FilterTable[Sampler->MinFilter],
                .mipmapMode = MipFilterTable[Sampler->MipFilter],
                .addressModeU = WrapTable[Sampler->WrapU],
                .addressModeV = WrapTable[Sampler->WrapU],
                .addressModeW = WrapTable[Sampler->WrapU],
                .mipLodBias = 0.0f,
                .anisotropyEnable = (Sampler->Anisotropy != Anisotropy_None),
                .maxAnisotropy = AnisotropyTable[Sampler->Anisotropy],
                .compareEnable = Sampler->EnableComparison,
                .compareOp = CompareOpTable[Sampler->Comparison],
                .minLod = Sampler->MinLOD,
                .maxLod = Sampler->MaxLOD,
                .borderColor = BorderTable[Sampler->Border],
                .unnormalizedCoordinates = Sampler->EnableUnnormalizedCoordinates,
            };
            Result = vkCreateSampler(VK.Device, &Info, nullptr, &Renderer->Samplers[Index]);
            Assert(Result == VK_SUCCESS);
        }
    }

    // Create descriptor set layouts
    {
        Renderer->SetLayouts[SetLayout_None] = VK_NULL_HANDLE;
        for (u32 Index = 1; Index < SetLayout_Count; Index++)
        {
            const descriptor_set_layout_info* Info = SetLayoutInfos + Index;

            VkDescriptorSetLayoutBinding Bindings[MaxDescriptorSetLayoutBindingCount] = {};
            for (u32 Binding = 0; Binding < Info->BindingCount; Binding++)
            {
                Bindings[Binding].binding = Info->Bindings[Binding].Binding;
                Bindings[Binding].descriptorType = (VkDescriptorType)Info->Bindings[Binding].Type;
                Bindings[Binding].descriptorCount = Info->Bindings[Binding].DescriptorCount;
                Bindings[Binding].stageFlags = PipelineStagesToVulkan(Info->Bindings[Binding].Stages);
                Bindings[Binding].pImmutableSamplers = &Renderer->Samplers[Info->Bindings[Binding].ImmutableSampler];
            }
            VkDescriptorSetLayoutCreateInfo CreateInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = Info->BindingCount,
                .pBindings = Bindings,
            };

            VkDescriptorBindingFlags BindlessFlags = 0;
            VkDescriptorSetLayoutBindingFlagsCreateInfo BindlessInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
                .pNext = nullptr,
                .bindingCount = 1,
                .pBindingFlags = &BindlessFlags,
            };

            if (HasFlag(Info->Flags, SetLayoutFlag_UpdateAfterBind))
            {
                CreateInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
                BindlessFlags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
            }

            if (HasFlag(Info->Flags, SetLayoutFlag_Bindless))
            {
                BindlessFlags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                Bindings[0].descriptorCount = 1 << 18; // HACK(boti): see texture manager
                CreateInfo.pNext = &BindlessInfo;
            }

            Result = vkCreateDescriptorSetLayout(VK.Device, &CreateInfo, nullptr, &Renderer->SetLayouts[Index]);
            Assert(Result == VK_SUCCESS);
        }
    }
    
    // Staging Buffer
    {
        size_t MemorySize = MiB(128);
        if ((Result = CreateAndAllocateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK.SharedMemTypes, MemorySize, 
                &Renderer->StagingBuffer.Buffer, &Renderer->StagingBuffer.Memory)) == VK_SUCCESS)
        {
            Renderer->StagingBuffer.Size = MemorySize;
            Result = vkMapMemory(VK.Device, Renderer->StagingBuffer.Memory, 
                                 0, Renderer->StagingBuffer.Size, 0, 
                                 &Renderer->StagingBuffer.Mapping);
            ReturnOnFailure();
        }
        ReturnOnFailure();
    }

    // Vertex Buffer
    {
        if (!CreateGeometryBuffer(R_VertexBufferMaxBlockCount, Arena, &Renderer->GeometryBuffer))
        {
            Result = VK_ERROR_INITIALIZATION_FAILED;
            ReturnOnFailure();
        }
    }

    // Texture Manager
    // TODO(boti): remove this
    Renderer->TextureManager.DescriptorSetLayouts[0] = Renderer->SetLayouts[SetLayout_DefaultSamplerPS];
    Renderer->TextureManager.DescriptorSetLayouts[1] = Renderer->SetLayouts[SetLayout_BindlessTexturesPS];
    if (!CreateTextureManager(&Renderer->TextureManager, R_TextureMemorySize, VK.GPUMemTypes))
    {
        Result = VK_ERROR_INITIALIZATION_FAILED;
        ReturnOnFailure();
    }
    
    // Per frame
    {
        // Per-frame staging buffer
        {
            VkBufferCreateInfo BufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = MiB(128),
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
            };

            VkDeviceBufferMemoryRequirements Query = 
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
                .pNext = nullptr,
                .pCreateInfo = &BufferInfo,
            };
            VkMemoryRequirements2 MemoryRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
            vkGetDeviceBufferMemoryRequirements(VK.Device, &Query, &MemoryRequirements);
            VkMemoryRequirements* Requirements = &MemoryRequirements.memoryRequirements;
            Assert((Requirements->alignment & BufferInfo.size) == 0);

            u32 MemoryType = 0;
            u8 ScanResult = BitScanForward(&MemoryType, Requirements->memoryTypeBits & VK.SharedMemTypes);
            if (ScanResult)
            {
                VkMemoryAllocateInfo AllocInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .allocationSize = Renderer->SwapchainImageCount * BufferInfo.size,
                    .memoryTypeIndex = MemoryType,
                };
                Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->StagingMemory);
                ReturnOnFailure();

                Result = vkMapMemory(VK.Device, Renderer->StagingMemory, 0, VK_WHOLE_SIZE, 0, &Renderer->StagingMemoryMapping);
                for (u32 FrameIndex = 0; FrameIndex < Renderer->SwapchainImageCount; FrameIndex++)
                {
                    Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Renderer->StagingBuffers[FrameIndex]);
                    ReturnOnFailure();

                    umm Offset = FrameIndex * BufferInfo.size;
                    Result = vkBindBufferMemory(VK.Device, Renderer->StagingBuffers[FrameIndex], Renderer->StagingMemory, Offset);
                    ReturnOnFailure();

                    Renderer->Frames[FrameIndex].StagingBufferBase = OffsetPtr(Renderer->StagingMemoryMapping, Offset);
                    Renderer->Frames[FrameIndex].StagingBufferSize = BufferInfo.size;
                    Renderer->Frames[FrameIndex].StagingBufferAt = 0;
                }
            }
            else
            {
                return(0);
            }
        }

        // BAR memory
        {
            u32 MemoryTypeIndex = 0;
            BitScanForward(&MemoryTypeIndex, VK.BARMemTypes);
            
            Renderer->BARMemoryByteSize = MiB(64);
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = Renderer->BARMemoryByteSize,
                .memoryTypeIndex = MemoryTypeIndex,
            };
            Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->BARMemory);
            ReturnOnFailure();
            Result = vkMapMemory(VK.Device, Renderer->BARMemory, 0, VK_WHOLE_SIZE, 0, &Renderer->BARMemoryMapping);
            ReturnOnFailure();

            Renderer->BARMemoryByteOffset = 0;
        }

        auto PushBARMemory = [Renderer](VkDeviceSize Size, VkDeviceSize Alignment) -> VkDeviceSize
        {
            VkDeviceSize Result = Align(Renderer->BARMemoryByteOffset, Alignment);
            Assert(Result + Size <= Renderer->BARMemoryByteSize);
            Renderer->BARMemoryByteOffset = Result + Size;
            return(Result);
        };

        auto PushBARBuffer = 
        [Renderer, PushBARMemory]
        (VkBuffer* Buffers, void** Mappings, u32 Count, VkDeviceSize Size, VkBufferUsageFlags Usage) -> VkResult
        {
            VkResult Result = VK_SUCCESS;

            VkBufferCreateInfo BufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = Size,
                .usage = Usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
            };

            for (u32 Index = 0; Index < Count; Index++)
            {
                Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, Buffers + Index);
                if (Result != VK_SUCCESS) break;

                VkMemoryRequirements MemoryRequirements = {};
                vkGetBufferMemoryRequirements(VK.Device, Buffers[Index], &MemoryRequirements);
                VkDeviceSize Offset = PushBARMemory(MemoryRequirements.size, MemoryRequirements.alignment);
                Result = vkBindBufferMemory(VK.Device, Buffers[Index], Renderer->BARMemory, Offset);
                if (Result != VK_SUCCESS) break;

                Mappings[Index] = OffsetPtr(Renderer->BARMemoryMapping, Offset);
            }
            return(Result);
        };

        // Vertex stack
        {
            umm Size = MiB(8);
            PushBARBuffer(Renderer->PerFrameVertex2DBuffers, Renderer->PerFrameVertex2DMappings,
                          Renderer->SwapchainImageCount, Size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            ReturnOnFailure();
            for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
            {
                Renderer->Frames[i].MaxVertex2DCount = Size / sizeof(vertex_2d);
            }
        }

        // Per frame uniform data
        {
            constexpr u64 BufferSize = KiB(64);
            Result = PushBARBuffer(Renderer->PerFrameUniformBuffers, Renderer->PerFrameUniformBufferMappings,
                                   Renderer->SwapchainImageCount, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            ReturnOnFailure();
        }

        // Joint buffers
        {
            u64 BufferSizePerFrame = render_frame::MaxJointCount * sizeof(m4);
            Result = PushBARBuffer(Renderer->PerFrameJointBuffers, Renderer->PerFrameJointBufferMappings,
                                   Renderer->SwapchainImageCount, BufferSizePerFrame, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            ReturnOnFailure();
        }

        // Particle buffers
        {
            u64 BufferSizePerFrame = render_frame::MaxParticleCount * sizeof(render_particle);
            Result = PushBARBuffer(Renderer->PerFrameParticleBuffers, Renderer->PerFrameParticleBufferMappings,
                                   Renderer->SwapchainImageCount, BufferSizePerFrame, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }

        // Per frame descriptors
        for (u32 i = 0; i < 2; i++)
        {
            VkDescriptorPoolSize PoolSizes[] = 
            {
                {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = Renderer->MaxPerFrameDescriptorSetCount,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = Renderer->MaxPerFrameDescriptorSetCount,
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = Renderer->MaxPerFrameDescriptorSetCount,
                },
            };

            VkDescriptorPoolCreateInfo PoolInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                .maxSets = 256,
                .poolSizeCount = CountOf(PoolSizes),
                .pPoolSizes = PoolSizes,
            };

            Result = vkCreateDescriptorPool(VK.Device, &PoolInfo, nullptr, &Renderer->PerFrameDescriptorPool[i]);
            ReturnOnFailure();
        }
    }

    // Skinning buffer
    {
        VkBufferCreateInfo BufferInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = R_SkinningBufferSize,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &BufferInfo);
        u32 MemoryTypes = MemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
        u32 MemoryTypeIndex = 0;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = MemoryRequirements.size,
                .memoryTypeIndex = MemoryTypeIndex,
            };

            Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->SkinningMemory);
            ReturnOnFailure();
            Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Renderer->SkinningBuffer);
            ReturnOnFailure();
            Result = vkBindBufferMemory(VK.Device, Renderer->SkinningBuffer, Renderer->SkinningMemory, 0);
            ReturnOnFailure();
            Renderer->SkinningMemorySize = MemoryRequirements.size;
        }
        else
        {
            return(0);
        }
    }
    
    // Light buffer
    {
        VkBufferCreateInfo BufferInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = R_MaxLightCount * sizeof(light),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &BufferInfo);
        u32 MemoryTypes = MemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
        u32 MemoryTypeIndex = 0;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = MemoryRequirements.size,
                .memoryTypeIndex = MemoryTypeIndex,
            };

            Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->LightBufferMemory);
            ReturnOnFailure();
            Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Renderer->LightBuffer);
            ReturnOnFailure();
            Result = vkBindBufferMemory(VK.Device, Renderer->LightBuffer, Renderer->LightBufferMemory, 0);
            ReturnOnFailure();
            Renderer->LightBufferMemorySize = MemoryRequirements.size;
        }
        else
        {
            return(0);
        }
    }

    // Tile buffer
    {
        VkBufferCreateInfo BufferInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = sizeof(screen_tile) * R_MaxTileCountX * R_MaxTileCountY,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &BufferInfo);
        u32 MemoryTypes = MemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
        u32 MemoryTypeIndex = 0;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = MemoryRequirements.size,
                .memoryTypeIndex = MemoryTypeIndex,
            };

            Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->TileMemory);
            ReturnOnFailure();
            Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Renderer->TileBuffer);
            ReturnOnFailure();
            Result = vkBindBufferMemory(VK.Device, Renderer->TileBuffer, Renderer->TileMemory, 0);
            ReturnOnFailure();
            Renderer->TileMemorySize = MemoryRequirements.size;
        }
        else
        {
            return(0);
        }
    }

    // Shadow storage
    {
        u32 MemoryTypeIndex = 0;
        {
            VkImageCreateInfo DummyImageInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = FormatTable[SHADOW_FORMAT],
                .extent = { R_ShadowResolution, R_ShadowResolution, 1 },
                .mipLevels = 1,
                .arrayLayers = R_MaxShadowCascadeCount,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            VkImage DummyImage = VK_NULL_HANDLE;
            Result = vkCreateImage(VK.Device, &DummyImageInfo, nullptr, &DummyImage);
            ReturnOnFailure();

            VkMemoryRequirements DummyMemoryRequirements;
            vkGetImageMemoryRequirements(VK.Device, DummyImage, &DummyMemoryRequirements);

            u32 MemoryTypes = DummyMemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
            if (!BitScanForward(&MemoryTypeIndex, MemoryTypes))
            {
                UnhandledError("No suitable memory type for shadow maps");
                return(nullptr);
            }

            vkDestroyImage(VK.Device, DummyImage, nullptr);
        }

        Renderer->ShadowMemorySize = R_ShadowMapMemorySize;
        VkMemoryAllocateInfo AllocInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = Renderer->ShadowMemorySize,
            .memoryTypeIndex = MemoryTypeIndex,
        };
        Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->ShadowMemory);
        ReturnOnFailure();

        Renderer->ShadowCascadeCount = R_MaxShadowCascadeCount;
        VkImageCreateInfo ImageInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = FormatTable[SHADOW_FORMAT],
            .extent = { R_ShadowResolution, R_ShadowResolution, 1 },
            .mipLevels = 1,
            .arrayLayers = R_MaxShadowCascadeCount,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        
        Result = vkCreateImage(VK.Device, &ImageInfo, nullptr, &Renderer->ShadowMap);
        ReturnOnFailure();
        
        VkMemoryRequirements MemoryRequirements;
        vkGetImageMemoryRequirements(VK.Device, Renderer->ShadowMap, &MemoryRequirements);
        
        size_t Offset = Align(Renderer->ShadowMemoryOffset, MemoryRequirements.alignment);
        if (Offset + MemoryRequirements.size <= Renderer->ShadowMemorySize)
        {
            Result = vkBindImageMemory(VK.Device, Renderer->ShadowMap, Renderer->ShadowMemory, Offset);
            ReturnOnFailure();
        
            Renderer->ShadowMemoryOffset = Offset + MemoryRequirements.size;
        }
        else
        {
            return(nullptr);
        }
        
        VkImageViewCreateInfo ViewInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = Renderer->ShadowMap,
            .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            .format = FormatTable[SHADOW_FORMAT],
            .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        
        Result = vkCreateImageView(VK.Device, &ViewInfo, nullptr,
                                   &Renderer->ShadowView);
        ReturnOnFailure();
        
        for (u32 Cascade = 0; Cascade < Renderer->ShadowCascadeCount; Cascade++)
        {
            VkImageViewCreateInfo CascadeViewInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = Renderer->ShadowMap,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = FormatTable[SHADOW_FORMAT],
                .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = Cascade,
                    .layerCount = 1,
                },
            };
        
            Result = vkCreateImageView(VK.Device, &CascadeViewInfo, nullptr, 
                                       &Renderer->ShadowCascadeViews[Cascade]);
            ReturnOnFailure();
        }
    }
    
    // Pipelines
    {
        Renderer->Pipelines[Pipeline_None] = { VK_NULL_HANDLE, VK_NULL_HANDLE };

        constexpr u64 MaxPathSize = 512;
        char Path[MaxPathSize];
        u64 PathSize = MaxPathSize;
        CopyZStringToBuffer(Path, "build/", &PathSize);
        char* Name = Path + (MaxPathSize - PathSize);

        auto FormatToVulkan = [](format Format) -> VkFormat
        {
            VkFormat Result = VK_FORMAT_UNDEFINED;
            if (Format < Format_Count)
            {
                Result = FormatTable[Format];
            }
            else
            {
                InvalidCodePath;
            }
            return(Result);
        };

        for (u32 PipelineIndex = 1; PipelineIndex < Pipeline_Count; PipelineIndex++)
        {
            memory_arena_checkpoint Checkpoint = ArenaCheckpoint(TempArena);

            pipeline_with_layout* Pipeline = Renderer->Pipelines + PipelineIndex;
            const pipeline_info* Info = PipelineInfos + PipelineIndex;

            VkDescriptorSetLayout SetLayouts[MaxDescriptorSetCount] = {};
            for (u32 SetLayoutIndex = 0; SetLayoutIndex < Info->Layout.DescriptorSetCount; SetLayoutIndex++)
            {
                SetLayouts[SetLayoutIndex] = Renderer->SetLayouts[Info->Layout.DescriptorSets[SetLayoutIndex]];
            }

            VkPushConstantRange PushConstantRanges[MaxPushConstantRangeCount] = {};
            for (u32 Index = 0; Index < Info->Layout.PushConstantRangeCount; Index++)
            {
                const push_constant_range* Range = Info->Layout.PushConstantRanges + Index;
                PushConstantRanges[Index] = 
                {
                    .stageFlags = PipelineStagesToVulkan(Range->Stages),
                    .offset = Range->ByteOffset,
                    .size = Range->ByteSize,
                };
            }

            VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = Info->Layout.DescriptorSetCount,
                .pSetLayouts = SetLayouts,
                .pushConstantRangeCount = Info->Layout.PushConstantRangeCount,
                .pPushConstantRanges = PushConstantRanges,
            };
            Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Pipeline->Layout);
            Assert(Result == VK_SUCCESS);

            u64 NameSize = PathSize;
            CopyZStringToBuffer(Name, Info->Name, &NameSize);
            char* Extension = Path + (MaxPathSize - NameSize);

            if (Info->Type == PipelineType_Compute)
            {
                u64 Size = NameSize;
                CopyZStringToBuffer(Extension, ".cs", &Size);
                buffer Bin = Platform.LoadEntireFile(Path, TempArena);
                if (Bin.Size)
                {
                    VkShaderModuleCreateInfo ModuleInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .codeSize = Bin.Size,
                        .pCode = (u32*)Bin.Data,
                    };
                    VkShaderModule Module = VK_NULL_HANDLE;
                    Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &Module);
                    Assert(Result == VK_SUCCESS);
                    ReturnOnFailure();

                    VkComputePipelineCreateInfo PipelineInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .stage = 
                        {
                            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                            .module = Module,
                            .pName = "main",
                            .pSpecializationInfo = nullptr,
                        },
                        .layout = Pipeline->Layout,
                        .basePipelineHandle = VK_NULL_HANDLE,
                        .basePipelineIndex = 0,
                    };
                    Result = vkCreateComputePipelines(VK.Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline->Pipeline);
                    Assert(Result == VK_SUCCESS);
                    vkDestroyShaderModule(VK.Device, Module, nullptr);
                    ReturnOnFailure();
                }
                else
                {
                    return(nullptr);
                }
            }
            else if (Info->Type == PipelineType_Graphics)
            {
                Assert(!HasFlag(Info->EnabledStages, PipelineStage_CS));
                // Only VS and PS is supported for now
                if (Info->EnabledStages & ~(PipelineStage_VS|PipelineStage_PS)) 
                {
                    UnimplementedCodePath;
                }
                if (HasFlag(Info->DepthStencilState.Flags, DS_StencilTestEnable))
                {
                    UnimplementedCodePath;
                }

                VkPipelineShaderStageCreateInfo StageInfos[PipelineStage_Count] = {};
                u32 StageCount = 0;

                if (Info->EnabledStages & PipelineStage_VS)
                {
                    VkPipelineShaderStageCreateInfo* StageInfo = StageInfos + StageCount++;
                    StageInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    StageInfo->stage = VK_SHADER_STAGE_VERTEX_BIT;
                    StageInfo->pName = "main";

                    u64 Size = NameSize;
                    CopyZStringToBuffer(Extension, ".vs", &Size);
                    buffer Bin = Platform.LoadEntireFile(Path, TempArena);
                    if (Bin.Size)
                    {
                        VkShaderModuleCreateInfo ModuleInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                            .codeSize = Bin.Size,
                            .pCode = (u32*)Bin.Data,
                        };
                        Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &StageInfo->module);
                        ReturnOnFailure();
                    }
                    else
                    {
                        return(nullptr);
                    }
                }
                if (Info->EnabledStages & PipelineStage_PS)
                {
                    VkPipelineShaderStageCreateInfo* StageInfo = StageInfos + StageCount++;
                    StageInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    StageInfo->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                    StageInfo->pName = "main";

                    u64 Size = NameSize;
                    CopyZStringToBuffer(Extension, ".fs", &Size);
                    buffer Bin = Platform.LoadEntireFile(Path, TempArena);
                    if (Bin.Size)
                    {
                        VkShaderModuleCreateInfo ModuleInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                            .codeSize = Bin.Size,
                            .pCode = (u32*)Bin.Data,
                        };
                        Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &StageInfo->module);
                        ReturnOnFailure();
                    }
                    else
                    {
                        return(nullptr);
                    }
                }

                VkVertexInputBindingDescription VertexBindings[MaxVertexBindingCount] = {};
                VkVertexInputAttributeDescription VertexAttribs[MaxVertexAttribCount] = {};
                for (u32 BindingIndex = 0; BindingIndex < Info->InputAssemblerState.BindingCount; BindingIndex++)
                {
                    const vertex_binding* Binding = Info->InputAssemblerState.Bindings + BindingIndex;
                    if (Binding->InstanceStepRate > 1)
                    {
                        UnimplementedCodePath;
                    }
                    VertexBindings[BindingIndex].binding = BindingIndex;
                    VertexBindings[BindingIndex].stride = Binding->Stride;
                    VertexBindings[BindingIndex].inputRate = Binding->InstanceStepRate == 0 ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
                }

                for (u32 AttribIndex = 0; AttribIndex < Info->InputAssemblerState.AttribCount; AttribIndex++)
                {
                    const vertex_attrib* Attrib = Info->InputAssemblerState.Attribs + AttribIndex;
                    VertexAttribs[AttribIndex].location = Attrib->Index;
                    VertexAttribs[AttribIndex].binding = Attrib->Binding;
                    VertexAttribs[AttribIndex].format = FormatToVulkan(Attrib->Format);
                    VertexAttribs[AttribIndex].offset = Attrib->ByteOffset;
                }

                VkPipelineVertexInputStateCreateInfo VertexInputState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .vertexBindingDescriptionCount = Info->InputAssemblerState.BindingCount,
                    .pVertexBindingDescriptions = VertexBindings,
                    .vertexAttributeDescriptionCount = Info->InputAssemblerState.AttribCount,
                    .pVertexAttributeDescriptions = VertexAttribs,
                };

                VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .topology = TopologyTable[Info->InputAssemblerState.Topology],
                    .primitiveRestartEnable = Info->InputAssemblerState.EnablePrimitiveRestart,
                };

                VkViewport Viewport = {};
                VkRect2D Scissor = {};
                VkPipelineViewportStateCreateInfo ViewportState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .viewportCount = 1,
                    .pViewports = &Viewport,
                    .scissorCount = 1,
                    .pScissors = &Scissor,
                };

                VkPolygonMode PolygonMode;
                switch (Info->RasterizerState.Fill)
                {
                    case Fill_Solid: PolygonMode = VK_POLYGON_MODE_FILL; break;
                    case Fill_Line: PolygonMode = VK_POLYGON_MODE_LINE; break;
                    case Fill_Point: PolygonMode = VK_POLYGON_MODE_POINT; break;
                    InvalidDefaultCase;
                }
                VkPipelineRasterizationStateCreateInfo RasterizationState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .depthClampEnable = HasFlag(Info->RasterizerState.Flags, RS_DepthClampEnable),
                    .rasterizerDiscardEnable = HasFlag(Info->RasterizerState.Flags, RS_DiscardEnable),
                    .polygonMode = PolygonMode,
                    .cullMode = Info->RasterizerState.CullFlags,
                    .frontFace = HasFlag(Info->RasterizerState.Flags, RS_FrontCW) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE,
                    .depthBiasEnable = HasFlag(Info->RasterizerState.Flags, RS_DepthBiasEnable),
                    .depthBiasConstantFactor = Info->RasterizerState.DepthBiasConstantFactor,
                    .depthBiasClamp = Info->RasterizerState.DepthBiasClamp,
                    .depthBiasSlopeFactor = Info->RasterizerState.DepthBiasSlopeFactor,
                    .lineWidth = 1.0f,
                };

                VkPipelineMultisampleStateCreateInfo MultisampleState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                    .sampleShadingEnable = VK_FALSE,
                    .minSampleShading = 0.0f,
                    .pSampleMask = nullptr,
                    .alphaToCoverageEnable = VK_FALSE,
                    .alphaToOneEnable = VK_FALSE,
                };

                VkPipelineDepthStencilStateCreateInfo DepthStencilState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .depthTestEnable = HasFlag(Info->DepthStencilState.Flags, DS_DepthTestEnable),
                    .depthWriteEnable = HasFlag(Info->DepthStencilState.Flags, DS_DepthWriteEnable),
                    .depthCompareOp = CompareOpTable[Info->DepthStencilState.DepthCompareOp],
                    .depthBoundsTestEnable = HasFlag(Info->DepthStencilState.Flags, DS_DepthBoundsTestEnable),
                    .stencilTestEnable = HasFlag(Info->DepthStencilState.Flags, DS_StencilTestEnable),
                    .front = {},
                    .back = {},
                    .minDepthBounds = Info->DepthStencilState.MinDepthBounds,
                    .maxDepthBounds = Info->DepthStencilState.MaxDepthBounds,
                };

                VkPipelineColorBlendAttachmentState BlendAttachments[MaxColorAttachmentCount] = {};
                for (u32 BlendAttachmentIndex = 0; BlendAttachmentIndex < Info->BlendAttachmentCount; BlendAttachmentIndex++)
                {
                    const blend_attachment* Src = Info->BlendAttachments + BlendAttachmentIndex;
                    BlendAttachments[BlendAttachmentIndex] = 
                    {
                        .blendEnable = Src->BlendEnable,
                        .srcColorBlendFactor = BlendFactorTable[Src->SrcColor],
                        .dstColorBlendFactor = BlendFactorTable[Src->DstColor],
                        .colorBlendOp = BlendOpTable[Src->ColorOp],
                        .srcAlphaBlendFactor = BlendFactorTable[Src->SrcAlpha],
                        .dstAlphaBlendFactor = BlendFactorTable[Src->DstAlpha],
                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT,
                    };
                }

                VkPipelineColorBlendStateCreateInfo BlendState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .logicOpEnable = VK_FALSE,
                    .logicOp = VK_LOGIC_OP_CLEAR,
                    .attachmentCount = Info->BlendAttachmentCount,
                    .pAttachments = BlendAttachments,
                    .blendConstants = { 1.0f, 1.0f, 1.0f, 1.0f },
                };

                VkDynamicState DynamicStates[] = 
                {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                };
                VkPipelineDynamicStateCreateInfo DynamicState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .dynamicStateCount = CountOf(DynamicStates),
                    .pDynamicStates = DynamicStates,
                };

                VkFormat ColorAttachmentFormats[MaxColorAttachmentCount] = {};
                for (u32 ColorAttachmentIndex = 0; ColorAttachmentIndex < Info->ColorAttachmentCount; ColorAttachmentIndex++)
                {
                    format Format = Info->ColorAttachments[ColorAttachmentIndex];
                    if (Format == SWAPCHAIN_FORMAT) 
                    {
                        ColorAttachmentFormats[ColorAttachmentIndex] = Renderer->SurfaceFormat.format;
                    }
                    else
                    {
                        ColorAttachmentFormats[ColorAttachmentIndex] = FormatTable[Format];
                    }
                }

                VkPipelineRenderingCreateInfo DynamicRendering = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .pNext = nullptr,
                    .viewMask = 0,
                    .colorAttachmentCount = Info->ColorAttachmentCount,
                    .pColorAttachmentFormats = ColorAttachmentFormats,
                    .depthAttachmentFormat = FormatTable[Info->DepthAttachment],
                    .stencilAttachmentFormat = FormatTable[Info->StencilAttachment],
                };

                VkGraphicsPipelineCreateInfo PipelineCreateInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                    .pNext = &DynamicRendering,
                    .flags = 0,
                    .stageCount = StageCount,
                    .pStages = StageInfos,
                    .pVertexInputState = &VertexInputState,
                    .pInputAssemblyState = &InputAssemblyState,
                    .pTessellationState = nullptr,
                    .pViewportState = &ViewportState,
                    .pRasterizationState = &RasterizationState,
                    .pMultisampleState = &MultisampleState,
                    .pDepthStencilState = &DepthStencilState,
                    .pColorBlendState = &BlendState,
                    .pDynamicState = &DynamicState,
                    .layout = Pipeline->Layout,
                    .renderPass = VK_NULL_HANDLE,
                    .subpass = 0,
                    .basePipelineHandle = VK_NULL_HANDLE,
                    .basePipelineIndex = 0,
                };

                Result = vkCreateGraphicsPipelines(VK.Device, nullptr, 1, &PipelineCreateInfo, nullptr, &Pipeline->Pipeline);
                Assert(Result == VK_SUCCESS);

                for (u32 StageIndex = 0; StageIndex < StageCount; StageIndex++)
                {
                    VkPipelineShaderStageCreateInfo* Stage = StageInfos + StageIndex;
                    vkDestroyShaderModule(VK.Device, Stage->module, nullptr);
                }
            }
            else
            {
                UnhandledError("Unrecognized pipeline type");
            }

            RestoreArena(TempArena, Checkpoint);
        }
    }

    return(Renderer);
}

internal VkResult ResizeRenderTargets(renderer* Renderer)
{
    VkResult Result = VK_SUCCESS;

    VkSurfaceCapabilitiesKHR SurfaceCaps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VK.PhysicalDevice, Renderer->Surface, &SurfaceCaps);

    if (Renderer->SurfaceExtent.width != SurfaceCaps.currentExtent.width ||
        Renderer->SurfaceExtent.height != SurfaceCaps.currentExtent.height)
    {
        Renderer->SurfaceExtent = SurfaceCaps.currentExtent;
        if (Renderer->SurfaceExtent.width != 0 && Renderer->SurfaceExtent.height != 0)
        {
            if (Renderer->Swapchain)
            {
                Renderer->Debug.ResizeCount++;
                vkDeviceWaitIdle(VK.Device);

                for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                {
                    vkDestroyImageView(VK.Device, Renderer->SwapchainImageViews[i], nullptr);
                    Renderer->SwapchainImageViews[i] = VK_NULL_HANDLE;
                }
            }
            else
            {
                if (!CreateRenderTargetHeap(&Renderer->RenderTargetHeap, R_RenderTargetMemorySize))
                {
                    return VK_ERROR_INITIALIZATION_FAILED;
                }

                for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                {
                    render_frame* Frame = Renderer->Frames + i;
                    constexpr VkImageUsageFlagBits DepthStencil = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                    constexpr VkImageUsageFlagBits Color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                    constexpr VkImageUsageFlagBits Sampled = VK_IMAGE_USAGE_SAMPLED_BIT;
                    constexpr VkImageUsageFlagBits Storage = VK_IMAGE_USAGE_STORAGE_BIT;
                    
                    if (i == 0)
                    {
                        Frame->Backend->DepthBuffer = PushRenderTarget(&Renderer->RenderTargetHeap, FormatTable[DEPTH_FORMAT], DepthStencil|Sampled, 1);
                        Frame->Backend->StructureBuffer = PushRenderTarget(&Renderer->RenderTargetHeap, FormatTable[STRUCTURE_BUFFER_FORMAT], Color|Sampled, 1);
                        Frame->Backend->HDRRenderTargets[0] = PushRenderTarget(&Renderer->RenderTargetHeap, FormatTable[HDR_FORMAT], Color|Sampled|Storage, 0);
                        Frame->Backend->HDRRenderTargets[1] = PushRenderTarget(&Renderer->RenderTargetHeap, FormatTable[HDR_FORMAT], Color|Sampled|Storage, 0);
                        Frame->Backend->OcclusionBuffers[0] = PushRenderTarget(&Renderer->RenderTargetHeap, FormatTable[SSAO_FORMAT], Color|Sampled|Storage, 1);
                        Frame->Backend->OcclusionBuffers[1] = PushRenderTarget(&Renderer->RenderTargetHeap, FormatTable[SSAO_FORMAT], Color|Sampled|Storage, 1);
                    }
                    else
                    {
                        Frame->Backend->DepthBuffer = Renderer->Frames[0].Backend->DepthBuffer;
                        Frame->Backend->StructureBuffer = Renderer->Frames[0].Backend->StructureBuffer;
                        Frame->Backend->HDRRenderTargets[0] = Renderer->Frames[0].Backend->HDRRenderTargets[0];
                        Frame->Backend->HDRRenderTargets[1] = Renderer->Frames[0].Backend->HDRRenderTargets[1];
                        Frame->Backend->OcclusionBuffers[0] = Renderer->Frames[0].Backend->OcclusionBuffers[0];
                        Frame->Backend->OcclusionBuffers[1] = Renderer->Frames[0].Backend->OcclusionBuffers[1];
                    }
                }
            }

            if (!ResizeRenderTargets(&Renderer->RenderTargetHeap, Renderer->SurfaceExtent.width, Renderer->SurfaceExtent.height))
            {
                UnhandledError("Failed to resize render targets");
                Result = VK_ERROR_UNKNOWN;
            }

            VkPresentModeKHR PresentMode = 
                VK_PRESENT_MODE_FIFO_KHR
                //VK_PRESENT_MODE_MAILBOX_KHR
                //VK_PRESENT_MODE_IMMEDIATE_KHR
                ;

            VkSwapchainCreateInfoKHR CreateInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0/*|VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR*/,
                .surface = Renderer->Surface,
                .minImageCount = Renderer->SwapchainImageCount,
                .imageFormat = Renderer->SurfaceFormat.format,
                .imageColorSpace = Renderer->SurfaceFormat.colorSpace,
                .imageExtent = SurfaceCaps.currentExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &VK.GraphicsQueueIdx,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = PresentMode,
                .clipped = VK_FALSE,
                .oldSwapchain = Renderer->Swapchain,
            };
            Result = vkCreateSwapchainKHR(VK.Device, &CreateInfo, nullptr, &Renderer->Swapchain);
            if (Result == VK_SUCCESS)
            {
                Result = vkGetSwapchainImagesKHR(VK.Device, Renderer->Swapchain, &Renderer->SwapchainImageCount, Renderer->SwapchainImages);
                if (Result == VK_SUCCESS)
                {
                    for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                    {
                        VkImageViewCreateInfo ViewInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .image = Renderer->SwapchainImages[i],
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = Renderer->SurfaceFormat.format,
                            .components = {},
                            .subresourceRange = 
                            {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            },
                        };

                        Result = vkCreateImageView(VK.Device, &ViewInfo, nullptr, &Renderer->SwapchainImageViews[i]);
                        if (Result != VK_SUCCESS)
                        {
                            break;
                        }
                    }
                }
            }

            if (CreateInfo.oldSwapchain)
            {
                vkDestroySwapchainKHR(VK.Device, CreateInfo.oldSwapchain, nullptr);
            }
        }
    }
    return Result;
}

geometry_buffer_allocation UploadVertexData(renderer* Renderer, 
                                            u32 VertexCount, const vertex* VertexData,
                                            u32 IndexCount, const vert_index* IndexData)
{
    geometry_buffer_allocation Allocation = AllocateVertexBuffer(&Renderer->GeometryBuffer, VertexCount, IndexCount);

    if (VertexCount)
    {
        Assert(Allocation.VertexBlock);

        void* Mapping = Renderer->StagingBuffer.Mapping;
        memcpy(OffsetPtr(Mapping, Renderer->StagingBuffer.Offset), VertexData, Allocation.VertexBlock->ByteSize);

        VkBufferCopy VertexRegion = 
        {
            .srcOffset = Renderer->StagingBuffer.Offset,
            .dstOffset = Allocation.VertexBlock->ByteOffset,
            .size = Allocation.VertexBlock->ByteSize,
        };
        // TODO(boti): support for multiple concurrent uploads
        //Renderer->StagingBuffer.Offset += Size;

        VkCommandBufferBeginInfo BeginInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        vkBeginCommandBuffer(Renderer->TransferCmdBuffer, &BeginInfo);
        vkCmdCopyBuffer(Renderer->TransferCmdBuffer, 
                        Renderer->StagingBuffer.Buffer, 
                        Renderer->GeometryBuffer.VertexMemory.Buffer, 
                        1, &VertexRegion);
        if (IndexCount)
        {
            Assert(Allocation.IndexBlock);
            u64 IndexOffset = Allocation.VertexBlock->ByteSize;

            memcpy(OffsetPtr(Mapping, Renderer->StagingBuffer.Offset + IndexOffset), IndexData, Allocation.IndexBlock->ByteSize);

            VkBufferCopy IndexRegion = 
            {
                .srcOffset = Renderer->StagingBuffer.Offset + IndexOffset,
                .dstOffset = Allocation.IndexBlock->ByteOffset,
                .size = Allocation.IndexBlock->ByteSize,
            };

            vkCmdCopyBuffer(Renderer->TransferCmdBuffer,
                            Renderer->StagingBuffer.Buffer,
                            Renderer->GeometryBuffer.IndexMemory.Buffer,
                            1, &IndexRegion);
        }

        vkEndCommandBuffer(Renderer->TransferCmdBuffer);

        VkSubmitInfo SubmitInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &Renderer->TransferCmdBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        vkQueueSubmit(VK.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VK.GraphicsQueue);

        vkResetCommandPool(VK.Device, Renderer->TransferCmdPool, 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    }

    return Allocation;
}

#undef ReturnOnFailure

texture_id PushTexture(renderer* Renderer, texture_flags Flags,
                       u32 Width, u32 Height, u32 MipCount, u32 ArrayCount,
                       format InFormat, texture_swizzle Swizzle, 
                       const void* Data)
{
    texture_id ID = { INVALID_INDEX_U32 };

    VkFormat Format = FormatTable[InFormat];

    texture_manager* TextureManager = &Renderer->TextureManager;
    ID = CreateTexture2D(TextureManager, Flags, Width, Height, MipCount, ArrayCount, Format, Swizzle);
    if (IsValid(ID))
    {
        VkImage Image = GetImage(TextureManager, ID);

        format_byterate ByteRate = FormatByterateTable[InFormat];
        u64 MemorySize = GetMipChainSize(Width, Height, MipCount, ArrayCount, ByteRate);

        vulkan_buffer* StagingBuffer = &Renderer->StagingBuffer;
        Assert(MemorySize <= StagingBuffer->Size);

        VkCommandBufferBeginInfo BeginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        vkBeginCommandBuffer(Renderer->TransferCmdBuffer, &BeginInfo);

        VkImageMemoryBarrier BeginBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = MipCount,
                .baseArrayLayer = 0,
                .layerCount = ArrayCount,
            },
        };
        vkCmdPipelineBarrier(Renderer->TransferCmdBuffer, 
                             VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &BeginBarrier);

        u8* Mapping = (u8*)StagingBuffer->Mapping;
        memcpy(Mapping, Data, MemorySize);

        u64 CopyOffset = 0;
        for (u32 ArrayIndex = 0; ArrayIndex < ArrayCount; ArrayIndex++)
        {
            for (u32 Mip = 0; Mip < MipCount; Mip++)
            {
                u32 CurrentWidth = Max(Width >> Mip, 1u);
                u32 CurrentHeight = Max(Height >> Mip, 1u);

                u32 RowLength = 0;
                u32 ImageHeight = 0;
                u64 TexelCount;
                u64 MipSize;
                if (ByteRate.IsBlock)
                {
                    RowLength = Align(CurrentWidth, 4u);
                    ImageHeight = Align(CurrentHeight, 4u);

                    TexelCount = (u64)RowLength * ImageHeight;
                }
                else
                {
                    TexelCount = CurrentWidth * CurrentHeight;
                }
                MipSize = TexelCount * ByteRate.Numerator / ByteRate.Denominator;

                VkBufferImageCopy CopyRegion = 
                {
                    .bufferOffset = CopyOffset,
                    .bufferRowLength = RowLength,
                    .bufferImageHeight = ImageHeight,
                    .imageSubresource = 
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = Mip,
                        .baseArrayLayer = ArrayIndex,
                        .layerCount = 1,
                    },
                    .imageOffset = { 0, 0, 0 },
                    .imageExtent = { CurrentWidth, CurrentHeight, 1 },
                };
                vkCmdCopyBufferToImage(Renderer->TransferCmdBuffer, StagingBuffer->Buffer, Image, 
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);
                CopyOffset += MipSize;
            }
        }
        VkImageMemoryBarrier EndBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Image,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = MipCount,
                .baseArrayLayer = 0,
                .layerCount = ArrayCount,
            },
        };
        vkCmdPipelineBarrier(Renderer->TransferCmdBuffer, 
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &EndBarrier);
        vkEndCommandBuffer(Renderer->TransferCmdBuffer);

        VkSubmitInfo SubmitInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &Renderer->TransferCmdBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        vkQueueSubmit(VK.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VK.GraphicsQueue);

        vkResetCommandPool(VK.Device, Renderer->TransferCmdPool, 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    }
    
    return(ID);
}

//
// Rendering interface implementation
//

render_frame* BeginRenderFrame(renderer* Renderer, memory_arena* Arena, u32 OutputWidth, u32 OutputHeight)
{
    u32 FrameID = (u32)(Renderer->CurrentFrameID++ % Renderer->SwapchainImageCount);
    render_frame* Frame = Renderer->Frames + FrameID;
    Frame->Renderer = Renderer;
    Frame->Arena = Arena;

    Frame->Backend->SwapchainImageIndex = INVALID_INDEX_U32;
    Frame->FrameID = FrameID;

    Frame->Backend->ImageAcquiredFence = Renderer->ImageAcquiredFences[FrameID];
    Frame->Backend->ImageAcquiredSemaphore = Renderer->ImageAcquiredSemaphores[FrameID];

    Frame->Backend->CmdPool = Renderer->CmdPools[FrameID];
    Frame->Backend->CmdBufferAt = 0;
    for (u32 Index = 0; Index < Frame->Backend->MaxCmdBufferCount; Index++)
    {
        Frame->Backend->CmdBuffers[Index] = Renderer->CmdBuffers[FrameID][Index];
    }
    Frame->Backend->ComputeCmdPool = Renderer->ComputeCmdPools[FrameID];
    Frame->Backend->ComputeCmdBuffer = Renderer->ComputeCmdBuffers[FrameID];

    Frame->Backend->DescriptorPool = Renderer->PerFrameDescriptorPool[FrameID];    

    Frame->Backend->ShadowCascadeCount = Renderer->ShadowCascadeCount;
    Frame->Backend->ShadowMap = Renderer->ShadowMap;
    Frame->Backend->ShadowMapView = Renderer->ShadowView;
    for (u32 i = 0; i < Frame->Backend->ShadowCascadeCount; i++)
    {
        Frame->Backend->ShadowCascadeViews[i] = Renderer->ShadowCascadeViews[i];
    }

    Frame->LightCount = 0;

    Frame->Backend->UniformBuffer = Renderer->PerFrameUniformBuffers[FrameID];
    Frame->UniformData = Renderer->PerFrameUniformBufferMappings[FrameID];

    Frame->StagingBufferAt = 0;
    Frame->Backend->StagingBuffer = Renderer->StagingBuffers[FrameID];

    Frame->Backend->Vertex2DBuffer = Renderer->PerFrameVertex2DBuffers[FrameID];

    Frame->Vertex2DCount = 0;
    Frame->Vertex2DArray = (vertex_2d*)Renderer->PerFrameVertex2DMappings[FrameID];

    Frame->DrawCmdCount = 0;
    Frame->SkinnedDrawCmdCount = 0;
    Frame->SkinningCmdCount = 0;

    Frame->ParticleCount = 0;
    Frame->Backend->ParticleBuffer = Renderer->PerFrameParticleBuffers[FrameID];
    Frame->Particles = (render_particle*)Renderer->PerFrameParticleBufferMappings[FrameID];

    Frame->ParticleDrawCmdCount = 0;

    Frame->JointCount = 0;
    Frame->Backend->JointBuffer = Renderer->PerFrameJointBuffers[FrameID];
    Frame->JointMapping = (m4*)Renderer->PerFrameJointBufferMappings[FrameID];
    Frame->JointBufferAlignment = TruncateU64ToU32(VK.ConstantBufferAlignment) / sizeof(m4);

    Frame->MaxSkinnedVertexCount = TruncateU64ToU32(R_SkinningBufferSize / sizeof(vertex));
    Frame->SkinnedMeshVertexCount = 0;
    Frame->Backend->SkinnedMeshVB = Renderer->SkinningBuffer;

    Frame->DrawWidget3DCmdCount = 0;

    Frame->RenderWidth = Renderer->SurfaceExtent.width;
    Frame->RenderHeight = Renderer->SurfaceExtent.height;
    
    Frame->Uniforms = {};
    Frame->Uniforms.ScreenSize = { (f32)Frame->RenderWidth, (f32)Frame->RenderHeight };

    VkSemaphoreWaitInfo WaitInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0,
        .semaphoreCount = 1,
        .pSemaphores = &Renderer->TimelineSemaphore,
        .pValues = &Frame->Backend->FrameFinishedCounter,
    };
    vkWaitSemaphores(VK.Device, &WaitInfo, U64_MAX);
    //vkWaitForFences(VK.Device, 1, &Frame->Backend->RenderFinishedFence, VK_TRUE, UINT64_MAX);
    //vkResetFences(VK.Device, 1, &Frame->Backend->RenderFinishedFence);

    vkResetCommandPool(VK.Device, Frame->Backend->CmdPool, 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    vkResetCommandPool(VK.Device, Frame->Backend->ComputeCmdPool, 0);

    return(Frame);
}

void EndRenderFrame(render_frame* Frame)
{
    auto DrawMeshes = [](render_frame* Frame, VkCommandBuffer CmdBuffer, VkPipelineLayout PipelineLayout)
    {
        const VkDeviceSize ZeroOffset = 0;
        vkCmdBindIndexBuffer(CmdBuffer, Frame->Renderer->GeometryBuffer.IndexMemory.Buffer, ZeroOffset, VK_INDEX_TYPE_UINT32);
        vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &Frame->Renderer->GeometryBuffer.VertexMemory.Buffer, &ZeroOffset);

        for (u32 CmdIndex = 0; CmdIndex < Frame->DrawCmdCount; CmdIndex++)
        {
            draw_cmd* Cmd = Frame->DrawCmds + CmdIndex;
            push_constants PushConstants = 
            {
                .Transform = Cmd->Transform,
                .Material = Cmd->Material,
            };

            vkCmdPushConstants(CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(PushConstants), &PushConstants);
            vkCmdDrawIndexed(CmdBuffer, Cmd->Base.IndexCount, Cmd->Base.InstanceCount, Cmd->Base.IndexOffset, Cmd->Base.VertexOffset, Cmd->Base.InstanceOffset);

        }

        vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &Frame->Backend->SkinnedMeshVB, &ZeroOffset);
        for (u32 CmdIndex = 0; CmdIndex < Frame->SkinnedDrawCmdCount; CmdIndex++)
        {
            draw_cmd* Cmd = Frame->SkinnedDrawCmds + CmdIndex;
            push_constants PushConstants = 
            {
                .Transform = Cmd->Transform,
                .Material = Cmd->Material,
            };
            vkCmdPushConstants(CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(PushConstants), &PushConstants);
            vkCmdDrawIndexed(CmdBuffer, Cmd->Base.IndexCount, 1, Cmd->Base.IndexOffset, Cmd->Base.VertexOffset, 0);
        }
    };

    renderer* Renderer = Frame->Renderer;
    // NOTE(boti): This is currently also the initial transfer and skinning cmd buffer
    VkCommandBuffer PrepassCmd = Frame->Backend->CmdBuffers[Frame->Backend->CmdBufferAt++];
    VkCommandBuffer RenderCmd = Frame->Backend->CmdBuffers[Frame->Backend->CmdBufferAt++];
    // NOTE(boti): Light binning, SSAO
    VkCommandBuffer PreLightCmd = Frame->Backend->ComputeCmdBuffer;
    VkCommandBuffer ShadowCmd = Frame->Backend->CmdBuffers[Frame->Backend->CmdBufferAt++];

    vkResetDescriptorPool(VK.Device, Frame->Backend->DescriptorPool, 0);
    Frame->Backend->UniformDescriptorSet = PushDescriptorSet(Frame, Renderer->SetLayouts[SetLayout_PerFrameUniformData]);

    // Acquire image
    {
        vkWaitForFences(VK.Device, 1, &Frame->Backend->ImageAcquiredFence, VK_TRUE, UINT64_MAX);
        vkResetFences(VK.Device, 1, &Frame->Backend->ImageAcquiredFence);

        if (Frame->RenderWidth != Renderer->SurfaceExtent.width ||
            Frame->RenderHeight != Renderer->SurfaceExtent.height)
        {
            ResizeRenderTargets(Renderer);
        }

        for (;;)
        {
            VkResult ImageAcquireResult = vkAcquireNextImageKHR(VK.Device, Renderer->Swapchain, 0, 
                                                                Frame->Backend->ImageAcquiredSemaphore, 
                                                                Frame->Backend->ImageAcquiredFence, 
                                                                &Frame->Backend->SwapchainImageIndex);
            if (ImageAcquireResult == VK_SUCCESS || 
                ImageAcquireResult == VK_TIMEOUT ||
                ImageAcquireResult == VK_NOT_READY)
            {
                Assert(Frame->Backend->SwapchainImageIndex != INVALID_INDEX_U32);
                break;
            }
            else if (ImageAcquireResult == VK_SUBOPTIMAL_KHR ||
                ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
            {
                ResizeRenderTargets(Renderer);
                Frame->RenderWidth = Renderer->SurfaceExtent.width;
                Frame->RenderHeight = Renderer->SurfaceExtent.height;
            }
            else
            {
                UnhandledError("vkAcquireNextImage error");
                return;
            }
        }
        Frame->Backend->SwapchainImage = Renderer->SwapchainImages[Frame->Backend->SwapchainImageIndex];
        Frame->Backend->SwapchainImageView = Renderer->SwapchainImageViews[Frame->Backend->SwapchainImageIndex];
    }

    VkCommandBufferBeginInfo CmdBufferBegin = 
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkBeginCommandBuffer(PrepassCmd, &CmdBufferBegin);

    u32 TileCountX = CeilDiv(Frame->RenderWidth, R_TileSizeX);
    u32 TileCountY = CeilDiv(Frame->RenderHeight, R_TileSizeY);
    Frame->Uniforms.TileCountX = TileCountX;
    Frame->Uniforms.TileCountY = TileCountY;
    {
        f32 s = (f32)Frame->RenderWidth / (f32)Frame->RenderHeight;
        frustum CameraFrustum = Frame->CameraFrustum;

        u32 LightBufferOffset = Frame->StagingBufferAt;
        light* LightBuffer = nullptr;
        if (Frame->StagingBufferAt + Frame->LightCount * sizeof(light) < Frame->StagingBufferSize)
        {
            LightBuffer = (light*)OffsetPtr(Frame->StagingBufferBase, LightBufferOffset);
            Frame->StagingBufferAt += Frame->LightCount * sizeof(light);
        }
        else
        {
            UnimplementedCodePath;
        }

        for (u32 LightIndex = 0; LightIndex < Frame->LightCount; LightIndex++)
        {
            light* Light = Frame->Lights + LightIndex;

            f32 L = Light->E.w * Max(Max(Light->E.x, Light->E.y), Light->E.z);
            f32 R = Sqrt(Max((L / R_LuminanceThreshold), 0.0f));
            if (IntersectFrustumSphere(&CameraFrustum, Light->P.xyz, R))
            {
                v4 P = Frame->Uniforms.ViewTransform * Light->P;
                u32 DstIndex = Frame->Uniforms.LightCount++;
                LightBuffer[DstIndex] = 
                {
                    .P = P,
                    .E = Light->E,
                };

                if (Frame->Uniforms.LightCount == R_MaxLightCount)
                {
                    break;
                }
            }
        }

        VkBufferMemoryBarrier2 BeginBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Renderer->LightBuffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };
        VkDependencyInfo BeginDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(BeginBarriers),
            .pBufferMemoryBarriers = BeginBarriers,
        };
        vkCmdPipelineBarrier2(PrepassCmd, &BeginDependency);

        VkBufferCopy LightBufferCopy = 
        {
            .srcOffset = LightBufferOffset,
            .dstOffset = 0,
            .size = Frame->Uniforms.LightCount * sizeof(light),
        };
        if (LightBufferCopy.size > 0)
        {
            vkCmdCopyBuffer(PrepassCmd, Frame->Backend->StagingBuffer, Renderer->LightBuffer, 1, &LightBufferCopy);
        }

        VkBufferMemoryBarrier2 EndBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Renderer->LightBuffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };
        VkDependencyInfo EndDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(EndBarriers),
            .pBufferMemoryBarriers = EndBarriers,
        };
        vkCmdPipelineBarrier2(PrepassCmd, &EndDependency);
    }

    SetupSceneRendering(Frame);

    VkDescriptorSet LightBufferDescriptorSet = PushBufferDescriptor(Frame, Renderer->SetLayouts[SetLayout_StructuredBuffer], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                                    Renderer->LightBuffer, 0, VK_WHOLE_SIZE);
    VkDescriptorSet TileBufferDescriptorSet = PushBufferDescriptor(Frame, Renderer->SetLayouts[SetLayout_StructuredBuffer], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                                   Renderer->TileBuffer, 0, VK_WHOLE_SIZE);

    VkDescriptorSet OcclusionDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_SampledRenderTargetPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->Backend->OcclusionBuffers[1]->View,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); 

    VkDescriptorSet StructureBufferDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_SampledRenderTargetPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->Backend->StructureBuffer->View,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); 

    VkDescriptorSet ShadowDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_ShadowPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->Backend->ShadowMapView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkViewport FrameViewport = 
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (f32)Frame->RenderWidth,
        .height = (f32)Frame->RenderHeight,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    VkRect2D FrameScissor = 
    {
        .offset = { 0, 0 },
        .extent = { Frame->RenderWidth, Frame->RenderHeight },
    };


    // Skinning
    {
        VkDescriptorSet JointDescriptorSet = 
            PushBufferDescriptor(Frame, 
                                 Renderer->SetLayouts[SetLayout_PoseTransform], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 
                                 Frame->Backend->JointBuffer, 0, VK.MaxConstantBufferByteSize); 

        VkDescriptorSet SkinningBuffersDescriptorSet = PushDescriptorSet(Frame, Renderer->SetLayouts[SetLayout_Skinning]);

        VkDescriptorBufferInfo DescriptorBufferInfos[] = 
        {
            {
                .buffer = Frame->Renderer->GeometryBuffer.VertexMemory.Buffer,
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            },
            {
                .buffer = Frame->Backend->SkinnedMeshVB,
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            },
        };
        VkWriteDescriptorSet DescriptorWrites[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = SkinningBuffersDescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = DescriptorBufferInfos + 0,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = SkinningBuffersDescriptorSet,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = DescriptorBufferInfos + 1,
            },
        };
        vkUpdateDescriptorSets(VK.Device, CountOf(DescriptorWrites), DescriptorWrites, 0, nullptr);

        pipeline_with_layout SkinningPipeline = Renderer->Pipelines[Pipeline_Skinning];
        vkCmdBindPipeline(PrepassCmd, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Pipeline);
        vkCmdBindDescriptorSets(PrepassCmd, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Layout,
                                0, 1, &SkinningBuffersDescriptorSet,
                                0, nullptr);

        VkBufferMemoryBarrier2 BeginBarriers[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Frame->Backend->SkinnedMeshVB,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };

        VkDependencyInfo BeginDependencies = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(BeginBarriers),
            .pBufferMemoryBarriers = BeginBarriers,
        };
        vkCmdPipelineBarrier2(PrepassCmd, &BeginDependencies);

        for (u32 SkinningCmdIndex = 0; SkinningCmdIndex < Frame->SkinningCmdCount; SkinningCmdIndex++)
        {
            skinning_cmd* Cmd = Frame->SkinningCmds + SkinningCmdIndex;
            vkCmdBindDescriptorSets(PrepassCmd, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Layout,
                                    1, 1, &JointDescriptorSet,
                                    1, &Cmd->PoseOffset);

            u32 PushConstants[3] = 
            {
                Cmd->SrcVertexOffset,
                Cmd->DstVertexOffset,
                Cmd->VertexCount,
            };
            vkCmdPushConstants(PrepassCmd, SkinningPipeline.Layout, VK_SHADER_STAGE_ALL,
                               0, sizeof(PushConstants), PushConstants);


            constexpr u32 SkinningGroupSize = 64;
            vkCmdDispatch(PrepassCmd, CeilDiv(Cmd->VertexCount, SkinningGroupSize), 1, 1);
        }

        VkBufferMemoryBarrier2 EndBarriers[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Frame->Backend->SkinnedMeshVB,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };

        VkDependencyInfo EndDependencies = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(EndBarriers),
            .pBufferMemoryBarriers = EndBarriers,
        };
        vkCmdPipelineBarrier2(PrepassCmd, &EndDependencies);
    }

    const VkDescriptorSet DescriptorSets[] = 
    {
        Renderer->TextureManager.DescriptorSets[0], // Samplers
        Renderer->TextureManager.DescriptorSets[1], // Images
        Frame->Backend->UniformDescriptorSet,
        OcclusionDescriptorSet,
        StructureBufferDescriptorSet,
        ShadowDescriptorSet,
        LightBufferDescriptorSet,
        TileBufferDescriptorSet,
    };

    vkCmdSetViewport(PrepassCmd, 0, 1, &FrameViewport);
    vkCmdSetScissor(PrepassCmd, 0, 1, &FrameScissor);
    BeginPrepass(Frame, PrepassCmd);
    {
        pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Prepass];
        vkCmdBindPipeline(PrepassCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
        vkCmdBindDescriptorSets(PrepassCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout, 
                                0, 3, DescriptorSets,
                                0, nullptr);
        DrawMeshes(Frame, PrepassCmd, Pipeline.Layout);
    }
    EndPrepass(Frame, PrepassCmd);

    vkEndCommandBuffer(PrepassCmd);

    u64 PrepassCounter = ++Renderer->TimelineSemaphoreCounter;
    VkCommandBufferSubmitInfo PrepassCmdBuffers[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = PrepassCmd,
            .deviceMask = 0,
        },
    };
    VkSemaphoreSubmitInfo PrepassSignals[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = Renderer->TimelineSemaphore,
            .value = PrepassCounter,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
    };
    VkSubmitInfo2 SubmitPrepass = 
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .waitSemaphoreInfoCount = 0,
        .pWaitSemaphoreInfos = nullptr,
        .commandBufferInfoCount = CountOf(PrepassCmdBuffers),
        .pCommandBufferInfos = PrepassCmdBuffers,
        .signalSemaphoreInfoCount = CountOf(PrepassSignals),
        .pSignalSemaphoreInfos = PrepassSignals,
    };
    vkQueueSubmit2(VK.GraphicsQueue, 1, &SubmitPrepass, nullptr);

    vkBeginCommandBuffer(PreLightCmd, &CmdBufferBegin);

    RenderSSAO(Frame, PreLightCmd,
               Frame->PostProcess.SSAO,
               Renderer->Pipelines[Pipeline_SSAO].Pipeline, 
               Renderer->Pipelines[Pipeline_SSAO].Layout, 
               Renderer->SetLayouts[SetLayout_SSAO],
               Renderer->Pipelines[Pipeline_SSAOBlur].Pipeline, 
               Renderer->Pipelines[Pipeline_SSAOBlur].Layout, 
               Renderer->SetLayouts[SetLayout_SSAOBlur]);

    // Light binning
    {
        VkBufferMemoryBarrier2 BeginBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Renderer->TileBuffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };
        VkDependencyInfo BeginDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(BeginBarriers),
            .pBufferMemoryBarriers = BeginBarriers,
        };
        vkCmdPipelineBarrier2(PreLightCmd, &BeginDependency);

        pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_LightBinning];
        vkCmdBindPipeline(PreLightCmd, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline.Pipeline);

        VkDescriptorSet BinningDescriptorSets[] = 
        {
            Frame->Backend->UniformDescriptorSet,
            LightBufferDescriptorSet,
            TileBufferDescriptorSet,
        };
        vkCmdBindDescriptorSets(PreLightCmd, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline.Layout, 
                                0, CountOf(BinningDescriptorSets), BinningDescriptorSets, 0, nullptr);

        vkCmdDispatch(PreLightCmd, CeilDiv(TileCountX, 32), TileCountY, 1);

        VkBufferMemoryBarrier2 EndBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Renderer->TileBuffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };
        VkDependencyInfo EndDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(EndBarriers),
            .pBufferMemoryBarriers = EndBarriers,
        };
        vkCmdPipelineBarrier2(PreLightCmd, &EndDependency);
    }

    vkEndCommandBuffer(PreLightCmd);

    u64 PreLightCounter = ++Renderer->ComputeTimelineSemaphoreCounter;
    VkCommandBufferSubmitInfo PreLightCmdBuffers[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = PreLightCmd,
            .deviceMask = 0,
        },
    };
    VkSemaphoreSubmitInfo PreLightWaits[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = Renderer->TimelineSemaphore,
            .value = PrepassCounter,
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        },
    };
    VkSemaphoreSubmitInfo PreLightSignals[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = Renderer->ComputeTimelineSemaphore,
            .value = PreLightCounter,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
    };
    VkSubmitInfo2 SubmitPreLight = 
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0,
        .waitSemaphoreInfoCount = CountOf(PreLightWaits),
        .pWaitSemaphoreInfos = PreLightWaits,
        .commandBufferInfoCount = CountOf(PreLightCmdBuffers),
        .pCommandBufferInfos = PreLightCmdBuffers,
        .signalSemaphoreInfoCount = CountOf(PreLightSignals),
        .pSignalSemaphoreInfos = PreLightSignals,
    };
    vkQueueSubmit2(VK.ComputeQueue, 1, &SubmitPreLight, nullptr);

    vkBeginCommandBuffer(ShadowCmd, &CmdBufferBegin);
    VkViewport ShadowViewport = 
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (f32)R_ShadowResolution,
        .height = (f32)R_ShadowResolution,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    VkRect2D ShadowScissor = 
    {
        .offset = { 0, 0 },
        .extent = { R_ShadowResolution, R_ShadowResolution },
    };
    vkCmdSetViewport(ShadowCmd, 0, 1, &ShadowViewport);
    vkCmdSetScissor(ShadowCmd, 0, 1, &ShadowScissor);
    for (u32 Cascade = 0; Cascade < R_MaxShadowCascadeCount; Cascade++)
    {
        BeginCascade(Frame, ShadowCmd, Cascade);

        pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Shadow];
        vkCmdBindPipeline(ShadowCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
        vkCmdPushConstants(ShadowCmd, Pipeline.Layout,
                           VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                           sizeof(push_constants), sizeof(u32), &Cascade);
        vkCmdBindDescriptorSets(ShadowCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                0, 3, DescriptorSets, 0, nullptr);
        DrawMeshes(Frame, ShadowCmd, Pipeline.Layout);

        EndCascade(Frame, ShadowCmd);
    }
    vkEndCommandBuffer(ShadowCmd);

    u64 ShadowCounter = ++Renderer->TimelineSemaphoreCounter;
    VkCommandBufferSubmitInfo ShadowCmdBuffers[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = ShadowCmd,
            .deviceMask = 0,
        },
    };
    VkSemaphoreSubmitInfo ShadowWaits[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = Renderer->TimelineSemaphore,
            .value = PrepassCounter,
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        },
    };
    VkSemaphoreSubmitInfo ShadowSignals[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = Renderer->TimelineSemaphore,
            .value = ShadowCounter,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
    };
    VkSubmitInfo2 SubmitShadow = 
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0,
        .waitSemaphoreInfoCount = CountOf(ShadowWaits),
        .pWaitSemaphoreInfos = ShadowWaits,
        .commandBufferInfoCount = CountOf(ShadowCmdBuffers),
        .pCommandBufferInfos = ShadowCmdBuffers,
        .signalSemaphoreInfoCount = CountOf(ShadowSignals),
        .pSignalSemaphoreInfos = ShadowSignals,
    };
    vkQueueSubmit2(VK.GraphicsQueue, 1, &SubmitShadow, nullptr);

    vkBeginCommandBuffer(RenderCmd, &CmdBufferBegin);
    vkCmdSetViewport(RenderCmd, 0, 1, &FrameViewport);
    vkCmdSetScissor(RenderCmd, 0, 1, &FrameScissor);
    BeginForwardPass(Frame, RenderCmd);
    {

        pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Simple];
        vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
        vkCmdBindDescriptorSets(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout, 
                                0, CountOf(DescriptorSets), DescriptorSets,
                                0, nullptr);

        DrawMeshes(Frame, RenderCmd, Pipeline.Layout);

        // Render sky
#if 1
        {
            pipeline_with_layout SkyPipeline = Renderer->Pipelines[Pipeline_Sky];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Pipeline);
            vkCmdBindDescriptorSets(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Layout, 
                                    0, 1, &Frame->Backend->UniformDescriptorSet, 
                                    0, nullptr);
            vkCmdDraw(RenderCmd, 3, 1, 0, 0);
        }
#endif

        // Particles
        {
            VkImageView ParticleView = GetImageView(&Renderer->TextureManager, Frame->ParticleTextureID);
            VkDescriptorSet TextureSet = PushDescriptorSet(Frame, Renderer->SetLayouts[SetLayout_SingleCombinedTexturePS]);
            VkDescriptorImageInfo DescriptorImage = 
            {
                .sampler = Renderer->Samplers[Sampler_Default],
                .imageView = ParticleView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkWriteDescriptorSet TextureWrite = 
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = TextureSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &DescriptorImage,
            };
            vkUpdateDescriptorSets(VK.Device, 1, &TextureWrite, 0, nullptr);

            VkDescriptorSet ParticleBufferDescriptor = 
                PushBufferDescriptor(Frame, Renderer->SetLayouts[SetLayout_ParticleBuffer], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
                                     Frame->Backend->ParticleBuffer, 0, VK_WHOLE_SIZE);
            VkDescriptorSet ParticleDescriptorSets[] = 
            {
                Frame->Backend->UniformDescriptorSet,
                ParticleBufferDescriptor,
                StructureBufferDescriptorSet,
                TextureSet,
            };

            pipeline_with_layout ParticlePipeline = Renderer->Pipelines[Pipeline_Quad];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ParticlePipeline.Pipeline);
            vkCmdBindDescriptorSets(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ParticlePipeline.Layout,
                                    0, CountOf(ParticleDescriptorSets), ParticleDescriptorSets, 
                                    0, nullptr);

            for (u32 CmdIndex = 0; CmdIndex < Frame->ParticleDrawCmdCount; CmdIndex++)
            {
                particle_cmd* Cmd = Frame->ParticleDrawCmds + CmdIndex;
                u32 VertexCount = 6 * Cmd->ParticleCount;
                u32 FirstVertex = 6 * Cmd->FirstParticle;
                vkCmdPushConstants(RenderCmd, ParticlePipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 
                                   0, sizeof(Cmd->Mode), &Cmd->Mode);
                vkCmdDraw(RenderCmd, VertexCount, 1, FirstVertex, 0);
            }
        }
    }
    EndForwardPass(Frame, RenderCmd);
    

    RenderBloom(Frame, RenderCmd,
                Frame->PostProcess.Bloom,
                Frame->Backend->HDRRenderTargets[0],
                Frame->Backend->HDRRenderTargets[1],
                Renderer->Pipelines[Pipeline_BloomDownsample].Layout,
                Renderer->Pipelines[Pipeline_BloomDownsample].Pipeline,
                Renderer->Pipelines[Pipeline_BloomUpsample].Layout,
                Renderer->Pipelines[Pipeline_BloomUpsample].Pipeline,
                Renderer->SetLayouts[SetLayout_Bloom],
                Renderer->SetLayouts[SetLayout_BloomUpsample]);

    // Blit + UI
    {
        VkImageMemoryBarrier2 BeginBarriers[] = 
        {
            // Swapchain image
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->Backend->SwapchainImage,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
        };

        VkDependencyInfo BeginDependencyInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = CountOf(BeginBarriers),
            .pImageMemoryBarriers = BeginBarriers,
        };
        vkCmdPipelineBarrier2(RenderCmd, &BeginDependencyInfo); 

        VkRenderingAttachmentInfo ColorAttachment = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->Backend->SwapchainImageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = { { 0.0f, 0.0f, 0.0f, 0.0f } } },
        };

        VkRenderingAttachmentInfo DepthAttachment = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->Backend->DepthBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .depthStencil = { 1.0f, 0 } },
        };

        VkRenderingInfo RenderingInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = { { 0, 0 }, { Frame->RenderWidth, Frame->RenderHeight } },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachment,
            .pStencilAttachment = nullptr,
        };
        vkCmdBeginRendering(RenderCmd, &RenderingInfo);
        
        // Blit
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Blit];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdPushConstants(RenderCmd, Pipeline.Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 
                               0, sizeof(f32), &Frame->PostProcess.Bloom.Strength);

            VkDescriptorSet BlitDescriptorSet = VK_NULL_HANDLE;
            VkDescriptorSetAllocateInfo BlitSetInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Frame->Backend->DescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &Renderer->SetLayouts[SetLayout_Blit],
            };
            VkResult Result = vkAllocateDescriptorSets(VK.Device, &BlitSetInfo, &BlitDescriptorSet);
            Assert(Result == VK_SUCCESS);

            VkDescriptorImageInfo ImageInfos[2] = 
            {
                {
                    .sampler = VK_NULL_HANDLE,
                    .imageView = Frame->Backend->HDRRenderTargets[0]->MipViews[0],
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
                {
                    .sampler = VK_NULL_HANDLE,
                    .imageView = Frame->Backend->HDRRenderTargets[1]->MipViews[0],
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
            };
            VkWriteDescriptorSet SetWrites[2] = 
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = BlitDescriptorSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = ImageInfos + 0,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = BlitDescriptorSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = ImageInfos + 1,
                },
            };
            vkUpdateDescriptorSets(VK.Device, CountOf(SetWrites), SetWrites, 0, nullptr);

            vkCmdBindDescriptorSets(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                    0, 1, &BlitDescriptorSet, 0, nullptr);
            vkCmdDraw(RenderCmd, 3, 1, 0, 0);
        }

        // 3D widget render
        {
            const VkDeviceSize ZeroOffset = 0;
            vkCmdBindVertexBuffers(RenderCmd, 0, 1, &Renderer->GeometryBuffer.VertexMemory.Buffer, &ZeroOffset);
            vkCmdBindIndexBuffer(RenderCmd, Renderer->GeometryBuffer.IndexMemory.Buffer, ZeroOffset, VK_INDEX_TYPE_UINT32);

            pipeline_with_layout GizmoPipeline = Renderer->Pipelines[Pipeline_Gizmo];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Pipeline);
            vkCmdBindDescriptorSets(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Layout,
                                    0, 1, &Frame->Backend->UniformDescriptorSet, 
                                    0, nullptr);

            for (u32 CmdIndex = 0; CmdIndex < Frame->DrawWidget3DCmdCount; CmdIndex++)
            {
                draw_widget3d_cmd* Cmd = Frame->DrawWidget3DCmds + CmdIndex;

                vkCmdPushConstants(RenderCmd, GizmoPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0, sizeof(Cmd->Transform), &Cmd->Transform);
                vkCmdPushConstants(RenderCmd, GizmoPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                                   sizeof(Cmd->Transform), sizeof(Cmd->Color), &Cmd->Color);
                vkCmdDrawIndexed(RenderCmd, Cmd->Base.IndexCount, Cmd->Base.InstanceCount, Cmd->Base.IndexOffset, Cmd->Base.VertexOffset, Cmd->Base.InstanceCount);
            }
        }

        {
            VkDeviceSize ZeroOffset = 0;
            vkCmdBindVertexBuffers(RenderCmd, 0, 1, &Frame->Backend->Vertex2DBuffer, &ZeroOffset);

            pipeline_with_layout UIPipeline = Renderer->Pipelines[Pipeline_UI];
            VkDescriptorSetLayout SetLayout = Frame->Renderer->SetLayouts[SetLayout_SingleCombinedTexturePS];
            VkDescriptorSet ImmediateDescriptorSet = PushImageDescriptor(Frame, SetLayout, Frame->ImmediateTextureID);

            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, UIPipeline.Pipeline);
            vkCmdBindDescriptorSets(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, UIPipeline.Layout, 
                                    0, 1, &ImmediateDescriptorSet, 0, nullptr);

            m4 OrthoTransform = M4(2.0f / Frame->RenderWidth, 0.0f, 0.0f, -1.0f,
                                   0.0f, 2.0f / Frame->RenderHeight, 0.0f, -1.0f,
                                   0.0f, 0.0f, 1.0f, 0.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f);

            vkCmdPushConstants(RenderCmd, UIPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 
                               0, sizeof(OrthoTransform), &OrthoTransform);
            vkCmdDraw(RenderCmd, Frame->Vertex2DCount, 1, 0, 0);
        }

        vkCmdEndRendering(RenderCmd);
    }

    VkImageMemoryBarrier2 EndBarriers[] = 
    {
        // Swapchain
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Frame->Backend->SwapchainImage,
            .subresourceRange = 
            {

                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
    };

    VkDependencyInfo EndDependencyInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = CountOf(EndBarriers),
        .pImageMemoryBarriers = EndBarriers,
    };
    vkCmdPipelineBarrier2(RenderCmd, &EndDependencyInfo);

    vkEndCommandBuffer(RenderCmd);

    // Submit + Present
    {
        u64 RenderCounter = ++Renderer->TimelineSemaphoreCounter;
        Frame->Backend->FrameFinishedCounter  = RenderCounter;
        VkCommandBufferSubmitInfo RenderCmdBuffers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext = nullptr,
                .commandBuffer = RenderCmd,
                .deviceMask = 0,
            },
        };
        VkSemaphoreSubmitInfo RenderWaits[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = Renderer->TimelineSemaphore,
                .value = ShadowCounter,
                .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            },
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = Renderer->ComputeTimelineSemaphore,
                .value = PreLightCounter,
                .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            },
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = Frame->Backend->ImageAcquiredSemaphore,
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            },
        };
        VkSemaphoreSubmitInfo RenderSignals[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = Renderer->TimelineSemaphore,
                .value = RenderCounter,
                .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            },
        };
        VkSubmitInfo2 SubmitRender = 
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .waitSemaphoreInfoCount = CountOf(RenderWaits),
            .pWaitSemaphoreInfos = RenderWaits,
            .commandBufferInfoCount = CountOf(RenderCmdBuffers),
            .pCommandBufferInfos = RenderCmdBuffers,
            .signalSemaphoreInfoCount = CountOf(RenderSignals),
            .pSignalSemaphoreInfos = RenderSignals,
        };
        vkQueueSubmit2(VK.GraphicsQueue, 1, &SubmitRender, nullptr);

        VkPresentInfoKHR PresentInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .swapchainCount = 1,
            .pSwapchains = &Frame->Renderer->Swapchain,
            .pImageIndices = &Frame->Backend->SwapchainImageIndex,
            .pResults = nullptr,
        };
        vkQueuePresentKHR(VK.GraphicsQueue, &PresentInfo);
    }

    // Collect stats
    {
        render_stats* Stats = &Frame->Stats;
        Stats->TotalMemoryUsed = 0;
        Stats->TotalMemoryAllocated = 0;
        Stats->EntryCount = 0;

        auto AddEntry = [Stats](const char* Name, umm UsedSize, umm TotalSize) -> b32
        {
            b32 Result = false;

            Stats->TotalMemoryUsed += UsedSize;
            Stats->TotalMemoryAllocated += TotalSize;

            if (Stats->EntryCount < Stats->MaxEntryCount)
            {
                render_stat_entry* Entry = Stats->Entries + Stats->EntryCount++;
                Entry->Name = Name;
                Entry->UsedSize = UsedSize;
                Entry->AllocationSize = TotalSize;
            }
            return(Result);
        };

        AddEntry("RenderTarget", Renderer->RenderTargetHeap.Offset, Renderer->RenderTargetHeap.MemorySize);
        AddEntry("BAR", Renderer->BARMemoryByteOffset, Renderer->BARMemoryByteSize);
        AddEntry("VertexBuffer", Renderer->GeometryBuffer.VertexMemory.MemoryInUse, Renderer->GeometryBuffer.VertexMemory.MemorySize);
        AddEntry("IndexBuffer", Renderer->GeometryBuffer.IndexMemory.MemoryInUse, Renderer->GeometryBuffer.IndexMemory.MemorySize);
        AddEntry("Texture", Renderer->TextureManager.MemoryOffset, Renderer->TextureManager.MemorySize);
        AddEntry("Shadow", Renderer->ShadowMemoryOffset, Renderer->ShadowMemorySize);
        AddEntry("Staging", Frame->StagingBufferAt, Frame->StagingBufferSize);
        AddEntry("LightBuffer", Frame->LightCount * sizeof(light), Renderer->LightBufferMemorySize);
        AddEntry("TileBuffer", TileCountX * TileCountY * sizeof(screen_tile), Renderer->TileMemorySize);
    }
}

void SetRenderCamera(render_frame* Frame, const render_camera* CameraIn)
{
    render_camera* Camera = &Frame->Camera;
    *Camera = *CameraIn;

    f32 AspectRatio = (f32)Frame->RenderWidth / (f32)Frame->RenderHeight;

    f32 n = Camera->NearZ;
    f32 f = Camera->FarZ;
    f32 r = 1.0f / (f - n);
    f32 s = AspectRatio;
    f32 g = Camera->FocalLength;

    Camera->ProjectionTransform = M4(
        g / s, 0.0f, 0.0f, 0.0f,
        0.0f, g, 0.0f, 0.0f,
        0.0f, 0.0f, f * r, -f*n * r,
        0.0f, 0.0f, 1.0f, 0.0f);
    Camera->InverseProjectionTransform = M4(
        s / g, 0.0f,     0.0f,            0.0f,
        0.0f,  1.0f / g, 0.0f,            0.0f,
        0.0f,  0.0f,     0.0f,            1.0f,
        0.0f,  0.0f,     1.0f / (-f*n*r), 1.0f / n);

    // Calculate camera frustum
    {
        f32 g2 = g*g;
        f32 mx = 1.0f / Sqrt(g2 + s*s);
        f32 my = 1.0f / Sqrt(g2 + 1.0f);
        f32 gmx = g*mx;
        f32 gmy = g*my;
        f32 smx = s*mx;
        Frame->CameraFrustum = 
        {
            .Left   = v4{ -gmx, 0.0f, smx, 0.0f } * Camera->ViewTransform,
            .Right  = v4{ +gmx, 0.0f, smx, 0.0f } * Camera->ViewTransform,
            .Top    = v4{ 0.0f, -gmy, my, 0.0f } * Camera->ViewTransform,
            .Bottom = v4{ 0.0f, +gmy, my, 0.0f } * Camera->ViewTransform,
            .Near   = v4{ 0.0f, 0.0f, +1.0f, -n } * Camera->ViewTransform,
            .Far    = v4{ 0.0f, 0.0f, -1.0f, +f } * Camera->ViewTransform,
        };
    }

    Frame->Uniforms.CameraTransform = Camera->CameraTransform;
    Frame->Uniforms.ViewTransform = Camera->ViewTransform;
    Frame->Uniforms.ProjectionTransform = Camera->ProjectionTransform;
    Frame->Uniforms.ViewProjectionTransform = Camera->ProjectionTransform * Camera->ViewTransform;

    Frame->Uniforms.FocalLength = Camera->FocalLength;
    Frame->Uniforms.AspectRatio = AspectRatio;
    Frame->Uniforms.NearZ = Camera->NearZ;
    Frame->Uniforms.FarZ = Camera->FarZ;

    Frame->Uniforms.CameraP = Camera->CameraTransform.P.xyz;
}

internal void SetupSceneRendering(render_frame* Frame)
{
    // Shadow cascade setup
    {
        m4 SunTransform;
        v3 SunX, SunY, SunZ;
        {
            SunZ = -Frame->SunV;
            if (Abs(Dot(SunZ, v3{ 0.0f, 0.0f, 0.0f })) < (1.0f - 1e-3f))
            {
                SunY = Normalize(Cross(SunZ, v3{ 1.0f, 0.0f, 0.0f }));
                SunX = Cross(SunY, SunZ);
            }
            else
            {
                SunX = Normalize(Cross(SunZ, v3{ 0.0f, 0.0f, -1.0f }));
                SunY = Cross(SunZ, SunX);
            }

            SunTransform.X = v4{ SunX.x, SunX.y, SunX.z, 0.0f };
            SunTransform.Y = v4{ SunY.x, SunY.y, SunY.z, 0.0f };
            SunTransform.Z = v4{ SunZ.x, SunZ.y, SunZ.z, 0.0f };
            SunTransform.P = v4{ 0.0f, 0.0f, 0.0f, 1.0f };
        }

        m4 SunView = AffineOrthonormalInverse(SunTransform);
        m4 CameraToSun = SunView * Frame->Uniforms.CameraTransform;
    
        f32 Splits[] = { 5.0f, 10.0f, 20.0f, 40.0f };
        f32 NdTable[] = 
        { 
            0.0f,
            0.8f * Splits[0],
            0.8f * Splits[1],
            0.8f * Splits[2],
        };
        f32 FdTable[] = 
        { 
            1.2f * Splits[0], 
            1.2f * Splits[1],
            1.2f * Splits[2], 
            Splits[3], 
        };
        static_assert(CountOf(NdTable) == R_MaxShadowCascadeCount);
        static_assert(CountOf(FdTable) == R_MaxShadowCascadeCount);
#if 0
        for (u32 CascadeIndex = 0; CascadeIndex < R_MaxShadowCascadeCount; CascadeIndex++)
        {
            f32 StartPercent = Max((CascadeIndex - 0.1f) / R_MaxShadowCascadeCount, 0.0f);
            f32 EndPercent = (CascadeIndex + 1.0f) / R_MaxShadowCascadeCount;
            f32 NdLinear = Frame->Uniforms.NearZ * StartPercent;
            f32 FdLinear = Frame->Uniforms.FarZ * EndPercent;

            constexpr f32 PolyExp = 2.0f;
            f32 NdPoly = Frame->Uniforms.FarZ * Pow(StartPercent, PolyExp);
            f32 FdPoly = Frame->Uniforms.FarZ * Pow(EndPercent, PolyExp);
            constexpr f32 PolyFactor = 0.8f;
            NdTable[CascadeIndex] = Lerp(NdLinear, NdPoly, PolyFactor);
            FdTable[CascadeIndex] = Lerp(FdLinear, FdPoly, PolyFactor);
        }
#endif

        f32 s = Frame->Uniforms.AspectRatio;
        f32 g = Frame->Uniforms.FocalLength;

        m4 Cascade0InverseViewProjection;
        for (u32 CascadeIndex = 0; CascadeIndex < R_MaxShadowCascadeCount; CascadeIndex++)
        {
            f32 Nd = NdTable[CascadeIndex];
            f32 Fd = FdTable[CascadeIndex];

            f32 NdOverG = Nd / g;
            f32 FdOverG = Fd / g;

            v3 CascadeBoxP[8] = 
            {
                // Near points in camera space
                { -NdOverG * s, -NdOverG, Nd },
                { +NdOverG * s, -NdOverG, Nd },
                { +NdOverG * s, +NdOverG, Nd },
                { -NdOverG * s, +NdOverG, Nd },

                // Far points in camera space
                { -FdOverG * s, -FdOverG, Fd },
                { +FdOverG * s, -FdOverG, Fd },
                { +FdOverG * s, +FdOverG, Fd },
                { -FdOverG * s, +FdOverG, Fd },
            };

            v3 CascadeBoxMin = { +FLT_MAX, +FLT_MAX, +FLT_MAX };
            v3 CascadeBoxMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

            for (u32 i = 0; i < 8; i++)
            {
                v3 P = TransformPoint(CameraToSun, CascadeBoxP[i]);
                CascadeBoxMin = 
                {
                    Min(P.x, CascadeBoxMin.x),
                    Min(P.y, CascadeBoxMin.y),
                    Min(P.z, CascadeBoxMin.z),
                };
                CascadeBoxMax = 
                {
                    Max(P.x, CascadeBoxMax.x),
                    Max(P.y, CascadeBoxMax.y),
                    Max(P.z, CascadeBoxMax.z),
                };
            }

            f32 CascadeScale = Ceil(Max(VectorLength(CascadeBoxP[0] - CascadeBoxP[6]),
                                        VectorLength(CascadeBoxP[4] - CascadeBoxP[6])));
            f32 TexelSize = CascadeScale / (f32)R_ShadowResolution;

            v3 CascadeP = 
            {
                TexelSize * Floor((CascadeBoxMax.x + CascadeBoxMin.x) / (2.0f * TexelSize)),
                TexelSize * Floor((CascadeBoxMax.y + CascadeBoxMin.y) / (2.0f * TexelSize)),
                CascadeBoxMin.z,
            };

            m4 CascadeView = M4(SunX.x, SunX.y, SunX.z, -CascadeP.x,
                                SunY.x, SunY.y, SunY.z, -CascadeP.y,
                                SunZ.x, SunZ.y, SunZ.z, -CascadeP.z,
                                0.0f, 0.0f, 0.0f, 1.0f);
            m4 CascadeProjection = M4(2.0f / CascadeScale, 0.0f, 0.0f, 0.0f,
                                      0.0f, 2.0f / CascadeScale, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f / (CascadeBoxMax.z - CascadeBoxMin.z), 0.0f,
                                      0.0f, 0.0f, 0.0f, 1.0f);
            m4 CascadeViewProjection = CascadeProjection * CascadeView;

            if (CascadeIndex == 0)
            {
                m4 InverseProjection = CascadeProjection;
                for (u32 i = 0; i < 3; i++)
                {
                    InverseProjection.E[i][i] = 1.0f / InverseProjection.E[i][i];
                }
                Cascade0InverseViewProjection = AffineOrthonormalInverse(CascadeView) * InverseProjection;
                Frame->Uniforms.CascadeViewProjection = CascadeViewProjection;
            }
            else
            {
                m4 Cascade0ToI = CascadeViewProjection * Cascade0InverseViewProjection;

                Frame->Uniforms.CascadeScales[CascadeIndex - 1] = 
                {
                    Cascade0ToI.E[0][0],
                    Cascade0ToI.E[1][1],
                    Cascade0ToI.E[2][2],
                    0.0f,
                };
                Frame->Uniforms.CascadeOffsets[CascadeIndex - 1] = Cascade0ToI.P;
                Frame->Uniforms.CascadeOffsets[CascadeIndex - 1].w = 0.0f;
            }

            Frame->Uniforms.CascadeMinDistances[CascadeIndex] = Nd;
            Frame->Uniforms.CascadeMaxDistances[CascadeIndex] = Fd;
        }
    }

    // Upload uniform data
    memcpy(Frame->UniformData, &Frame->Uniforms, sizeof(Frame->Uniforms));
    {
         VkDescriptorBufferInfo Info = { Frame->Backend->UniformBuffer, 0, sizeof(Frame->Uniforms) };
         VkWriteDescriptorSet Write = 
         {
             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .pNext = nullptr,
             .dstSet = Frame->Backend->UniformDescriptorSet,
             .dstBinding = 0,
             .dstArrayElement = 0,
             .descriptorCount = 1 ,
             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
             .pBufferInfo = &Info,
         };
         vkUpdateDescriptorSets(VK.Device, 1, &Write, 0, nullptr);
    }
}