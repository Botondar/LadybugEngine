#include "VulkanRenderer.hpp"

vulkan VK;
platform_api Platform;

#define ReturnOnFailure() \
if (Result != VK_SUCCESS) { \
    UnhandledError("Renderer init error"); \
    return nullptr; \
}

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

static VkDescriptorType DescriptorTypeTable[Descriptor_Count] =
{
    [Descriptor_Sampler]                = VK_DESCRIPTOR_TYPE_SAMPLER,
    [Descriptor_ImageSampler]           = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    [Descriptor_SampledImage]           = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    [Descriptor_StorageImage]           = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    [Descriptor_UniformTexelBuffer]     = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
    [Descriptor_StorageTexelBuffer]     = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    [Descriptor_UniformBuffer]          = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    [Descriptor_StorageBuffer]          = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    [Descriptor_DynamicUniformBuffer]   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    [Descriptor_DynamicStorageBuffer]   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
    [Descriptor_InlineUniformBlock]     = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
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

internal VkSamplerCreateInfo SamplerStateToVulkanSamplerInfo(sampler_state Sampler)
{
    f32 AnisotropyTable[Anisotropy_Count] = { 1.0f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
    VkSamplerCreateInfo Info = 
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = FilterTable[Sampler.MagFilter],
        .minFilter = FilterTable[Sampler.MinFilter],
        .mipmapMode = MipFilterTable[Sampler.MipFilter],
        .addressModeU = WrapTable[Sampler.WrapU],
        .addressModeV = WrapTable[Sampler.WrapU],
        .addressModeW = WrapTable[Sampler.WrapU],
        .mipLodBias = 0.0f,
        .anisotropyEnable = (Sampler.Anisotropy != Anisotropy_None),
        .maxAnisotropy = AnisotropyTable[Sampler.Anisotropy],
        .compareEnable = Sampler.EnableComparison,
        .compareOp = CompareOpTable[Sampler.Comparison],
        .minLod = Sampler.MinLOD,
        .maxLod = Sampler.MaxLOD,
        .borderColor = BorderTable[Sampler.Border],
        .unnormalizedCoordinates = Sampler.EnableUnnormalizedCoordinates,
    };
    return(Info);
}

#include "RenderDevice.cpp"
#include "Frame.cpp"
#include "Geometry.cpp"
#include "RenderTarget.cpp"
#include "TextureManager.cpp"
#include "Pipelines.cpp"

internal VkResult 
ResizeRenderTargets(renderer* Renderer, b32 Forced);

internal VkResult 
CreatePipelines(renderer* Renderer);

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

internal VkMemoryRequirements
GetImageMemoryRequirements(VkDevice Device, const VkImageCreateInfo* ImageInfo, VkImageAspectFlagBits Aspects)
{
    VkMemoryRequirements Result = {};

    VkMemoryRequirements2 MemoryRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkDeviceImageMemoryRequirements Query = 
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
        .pNext = nullptr,
        .pCreateInfo = ImageInfo,
        .planeAspect = Aspects,
    };

    vkGetDeviceImageMemoryRequirements(Device, &Query, &MemoryRequirements);
    Result = MemoryRequirements.memoryRequirements;

    return(Result);
}

internal VkResult CreateDedicatedBuffer(
    VkBufferUsageFlags Usage, u32 MemoryTypes, 
    umm Size, VkBuffer* pBuffer, VkDeviceMemory* pMemory)
{
    VkResult Result = VK_SUCCESS;

    VkBufferCreateInfo BufferInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = Size,
        .usage = Usage|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &BufferInfo);
    u32 MemoryTypeIndex;
    if (BitScanForward(&MemoryTypeIndex, MemoryTypes & MemoryRequirements.memoryTypeBits))
    {
        VkBuffer Buffer = VK_NULL_HANDLE;
        if ((Result = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Buffer)) == VK_SUCCESS)
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
        vkDestroyBuffer(VK.Device, Buffer, nullptr);
    }
    else
    {
        Result = VK_ERROR_UNKNOWN; // TODO
    }
    return Result;
}

internal b32 
PushBuffer(gpu_memory_arena* Arena, VkDeviceSize Size, VkBufferUsageFlags Usage, VkBuffer* Buffer, void** Mapping /*= nullptr*/)
{
    b32 Result = false;

    VkBufferCreateInfo Info = 
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

    VkDeviceBufferMemoryRequirements Query = 
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
        .pNext = nullptr,
        .pCreateInfo = &Info,
    };

    VkMemoryRequirements2 MemoryRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    vkGetDeviceBufferMemoryRequirements(VK.Device, &Query, &MemoryRequirements);
    VkMemoryRequirements* Requirements = &MemoryRequirements.memoryRequirements;

    if ((Requirements->memoryTypeBits & (1 << Arena->MemoryTypeIndex)) != 0)
    {
        u64 MemoryOffset = Align(Arena->MemoryAt, Requirements->alignment);
        if ((MemoryOffset + Requirements->size) <= Arena->Size)
        {
            VkResult ErrorCode = vkCreateBuffer(VK.Device, &Info, nullptr, Buffer);
            if (ErrorCode == VK_SUCCESS)
            {
                ErrorCode = vkBindBufferMemory(VK.Device, *Buffer, Arena->Memory, MemoryOffset);
                if (ErrorCode == VK_SUCCESS)
                {
                    Arena->MemoryAt = MemoryOffset + Requirements->size;
                    if (Mapping)
                    {
                        *Mapping = OffsetPtr(Arena->Mapping, MemoryOffset);
                    }
                    Result = true;
                }
            }
        }
        else
        {
            UnimplementedCodePath;
        }
    }

    return(Result);
}

internal VkResult 
CreatePipelines(renderer* Renderer, memory_arena* Scratch)
{
    VkResult Result = VK_SUCCESS;

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
        memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Scratch);
    
        pipeline_with_layout* Pipeline = Renderer->Pipelines + PipelineIndex;
        vkDestroyPipeline(VK.Device, Pipeline->Pipeline, nullptr);
        vkDestroyPipelineLayout(VK.Device, Pipeline->Layout, nullptr);

        const pipeline_info* Info = PipelineInfos + PipelineIndex;
        
        if (Info->Layout.DescriptorSetCount)
        {
            VkDescriptorSetLayout SetLayouts[MaxDescriptorSetCount] = {};
            for (u32 SystemSetIndex = 0; SystemSetIndex < SystemSet_Count; SystemSetIndex++)
            {
                SetLayouts[SystemSetIndex] = Renderer->SetLayouts[SystemSetIndex];
            }
            for (u32 SetLayoutIndex = 0; SetLayoutIndex < Info->Layout.DescriptorSetCount; SetLayoutIndex++)
            {
                SetLayouts[SetLayoutIndex + Set_User0] = Renderer->SetLayouts[Info->Layout.DescriptorSets[SetLayoutIndex]];
            }

            VkPushConstantRange PushConstantRange = 
            {
                .stageFlags = VK_SHADER_STAGE_ALL,
                .offset = 0,
                .size = MinPushConstantSize,
            };

            VkPipelineLayoutCreateInfo PipelineLayoutInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = SystemSet_Count + Info->Layout.DescriptorSetCount,
                .pSetLayouts = SetLayouts,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &PushConstantRange,
            };
            Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Pipeline->Layout);
            Assert(Result == VK_SUCCESS);
            if (Result != VK_SUCCESS) return(Result);
        }
        else
        {
            Pipeline->Layout = Renderer->SystemPipelineLayout;
        }

        u64 NameSize = PathSize;
        CopyZStringToBuffer(Name, Info->Name, &NameSize);
        char* Extension = Path + (MaxPathSize - NameSize);
    
        if (Info->Type == PipelineType_Compute)
        {
            u64 Size = NameSize;
            CopyZStringToBuffer(Extension, ".cs", &Size);
            buffer Bin = Platform.LoadEntireFile(Path, Scratch);
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
                if (Result != VK_SUCCESS) return(Result);

                VkComputePipelineCreateInfo PipelineInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
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
                if (Result != VK_SUCCESS) return(Result);
            }
            else
            {
                return(VK_ERROR_UNKNOWN);
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
                buffer Bin = Platform.LoadEntireFile(Path, Scratch);
                if (Bin.Size)
                {
                    VkShaderModuleCreateInfo ModuleInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = Bin.Size,
                        .pCode = (u32*)Bin.Data,
                    };
                    Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &StageInfo->module);
                    if (Result != VK_SUCCESS) return(Result);
                }
                else
                {
                    return(VK_ERROR_UNKNOWN);
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
                buffer Bin = Platform.LoadEntireFile(Path, Scratch);
                if (Bin.Size)
                {
                    VkShaderModuleCreateInfo ModuleInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = Bin.Size,
                        .pCode = (u32*)Bin.Data,
                    };
                    Result = vkCreateShaderModule(VK.Device, &ModuleInfo, nullptr, &StageInfo->module);
                    if (Result != VK_SUCCESS) return(Result);
                }
                else
                {
                    return(VK_ERROR_UNKNOWN);
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
                .flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT ,
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
            if (Result != VK_SUCCESS) return(Result);

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
    
        RestoreArena(Scratch, Checkpoint);
    }

    return(Result);
}

extern "C" Signature_GetDeviceName(GetDeviceName)
{
    const char* Result = Renderer->Vulkan.DeviceProps.deviceName;
    return(Result);
}

extern "C" Signature_CreateRenderer(CreateRenderer)
{
    Platform = *PlatformAPI;

    renderer* Renderer = PushStruct(Arena, 0, renderer);
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

        for (u32 SurfaceFormatIndex = 0; SurfaceFormatIndex < SurfaceFormatCount; SurfaceFormatIndex++)
        {
            if (SurfaceFormats[SurfaceFormatIndex].format == VK_FORMAT_R8G8B8A8_SRGB ||
                SurfaceFormats[SurfaceFormatIndex].format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                Renderer->SurfaceFormat = SurfaceFormats[SurfaceFormatIndex];
            }
        }

        Renderer->SwapchainImageCount = 2;
        // HACK(boti):
        for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
        {
            Renderer->Frames[FrameIndex].Backend = Renderer->BackendFrames + FrameIndex;
        }
        Result = ResizeRenderTargets(Renderer, false);
        ReturnOnFailure();
    }

    for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
    {
        VkFenceCreateInfo ImageFenceInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        Result = vkCreateFence(VK.Device, &ImageFenceInfo, nullptr, &Renderer->ImageAcquiredFences[FrameIndex]);
        ReturnOnFailure();

        VkSemaphoreCreateInfo SemaphoreInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        Result = vkCreateSemaphore(VK.Device, &SemaphoreInfo, nullptr, &Renderer->ImageAcquiredSemaphores[FrameIndex]);
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
            Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->CmdPools[FrameIndex]);
            ReturnOnFailure();
        
            for (u32 BufferIndex = 0; BufferIndex < backend_render_frame::MaxCmdBufferCount; BufferIndex++)
            {
                VkCommandBufferAllocateInfo BufferInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .commandPool = Renderer->CmdPools[FrameIndex],
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1,
                };
                Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->CmdBuffers[FrameIndex][BufferIndex]);
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
            Result = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->ComputeCmdPools[FrameIndex]);
            ReturnOnFailure();
        
            VkCommandBufferAllocateInfo BufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = Renderer->ComputeCmdPools[FrameIndex],
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            Result = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->ComputeCmdBuffers[FrameIndex]);
            ReturnOnFailure();
        }
        
    }

    // Create samplers
    {
        for (u32 Index = 0; Index < Sampler_Count; Index++)
        {
            VkSamplerCreateInfo Info = SamplerStateToVulkanSamplerInfo(SamplerInfos[Index]);
            Result = vkCreateSampler(VK.Device, &Info, nullptr, &Renderer->Samplers[Index]);
            Assert(Result == VK_SUCCESS);
        }
    }

    // Create descriptor set layouts
    {
        for (u32 Index = 0; Index < SetLayout_Count; Index++)
        {
            const descriptor_set_layout_info* Info = SetLayoutInfos + Index;

            VkDescriptorSetLayoutBinding Bindings[MaxDescriptorSetLayoutBindingCount] = {};
            VkDescriptorBindingFlags BindingFlags[MaxDescriptorSetLayoutBindingCount] = {};

            VkDescriptorSetLayoutBindingFlagsCreateInfo FlagsInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
                .pNext = nullptr,
                .bindingCount = Info->BindingCount,
                .pBindingFlags = BindingFlags,
            };

            VkDescriptorSetLayoutCreateInfo CreateInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = &FlagsInfo,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
                .bindingCount = Info->BindingCount,
                .pBindings = Bindings,
            };

            for (u32 BindingIndex = 0; BindingIndex < Info->BindingCount; BindingIndex++)
            {
                const descriptor_set_binding* Binding = Info->Bindings + BindingIndex;
                Bindings[BindingIndex].binding = Binding->Binding;
                Bindings[BindingIndex].descriptorType = DescriptorTypeTable[Binding->Type];
                Bindings[BindingIndex].descriptorCount = Binding->DescriptorCount;
                Bindings[BindingIndex].stageFlags = PipelineStagesToVulkan(Binding->Stages);
                Bindings[BindingIndex].pImmutableSamplers = nullptr;
                if (Binding->Flags & DescriptorFlag_PartiallyBound)
                {
                    BindingFlags[BindingIndex] |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                }
                if (Binding->Flags & DescriptorFlag_Bindless)
                {
                    // HACK(boti): see texture manager;
                    // TODO(boti) [update]: I don't see a reason why this can't just be part of the set layout?
                    Bindings[BindingIndex].descriptorCount = texture_manager::MaxTextureCount;
                    BindingFlags[BindingIndex] |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                }
            }

            Result = vkCreateDescriptorSetLayout(VK.Device, &CreateInfo, nullptr, &Renderer->SetLayouts[Index]);
            Assert(Result == VK_SUCCESS);
        }
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
    Renderer->TextureManager.DescriptorSetLayout = Renderer->SetLayouts[SetLayout_Bindless];
    if (!CreateTextureManager(&Renderer->TextureManager, R_TextureMemorySize, VK.GPUMemTypes, Renderer->SetLayouts))
    {
        Result = VK_ERROR_INITIALIZATION_FAILED;
        ReturnOnFailure();
    }

    // TODO(boti): Packed samplers should be folded into the sampler descriptor set
    #if 0
    for (u32 PackedSamplerID = 0; PackedSamplerID < packed_sampler::MaxSamplerCount; PackedSamplerID++)
    {
        packed_sampler PackedSampler = { .Value = PackedSamplerID };
        VkSamplerCreateInfo SamplerInfo = SamplerStateToVulkanSamplerInfo(UnpackSampler(PackedSampler));
        Result = vkCreateSampler(VK.Device, &SamplerInfo, nullptr, Renderer->TextureManager.PackedSamplers + PackedSamplerID);
        ReturnOnFailure();
    }

    {
        VkDescriptorSetAllocateInfo AllocInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = Renderer->TextureManager.DescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &Renderer->SetLayouts[SetLayout_PackedSamplers],
        };
        Result = vkAllocateDescriptorSets(VK.Device, &AllocInfo, &Renderer->TextureManager.PackedSamplerDescriptorSet);
        ReturnOnFailure();

        for (u32 SamplerID = 0; SamplerID < packed_sampler::MaxSamplerCount; SamplerID++)
        {
            VkDescriptorImageInfo Info = 
            {
                .sampler = Renderer->TextureManager.PackedSamplers[SamplerID],
                .imageView = VK_NULL_HANDLE,
                .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            VkWriteDescriptorSet Write = 
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = Renderer->TextureManager.PackedSamplerDescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = SamplerID,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                .pImageInfo = &Info,
            };
            vkUpdateDescriptorSets(VK.Device, 1, &Write, 0, nullptr);
        }
    }
    #endif

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
                    .allocationSize = R_MaxFramesInFlight * BufferInfo.size,
                    .memoryTypeIndex = MemoryType,
                };
                Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->StagingMemory);
                ReturnOnFailure();

                Result = vkMapMemory(VK.Device, Renderer->StagingMemory, 0, VK_WHOLE_SIZE, 0, &Renderer->StagingMemoryMapping);
                for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
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
            
            u64 MemorySize = MiB(64);
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
                .allocationSize = MemorySize,
                .memoryTypeIndex = MemoryTypeIndex,
            };
            VkDeviceMemory Memory = VK_NULL_HANDLE;
            Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Memory);
            ReturnOnFailure();
            void* Mapping = nullptr;
            Result = vkMapMemory(VK.Device, Memory, 0, VK_WHOLE_SIZE, 0, &Mapping);
            ReturnOnFailure();

            Renderer->BARMemory = 
            {
                .Memory = Memory,
                .Size = MemorySize,
                .MemoryAt = 0,
                .MemoryTypeIndex = MemoryTypeIndex,
                .Mapping = Mapping,
            };
        }

        // Per-frame descriptor buffer
        {
            umm SamplerBufferSize = KiB(16);
            umm ResourceBufferSize = KiB(128);
            for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, ResourceBufferSize, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            Renderer->PerFrameResourceDescriptorBuffers + FrameIndex,
                                            Renderer->PerFrameResourceDescriptorMappings + FrameIndex);
                if (!PushResult)
                {
                    UnhandledError("Failed to push sampler descriptor buffer");
                    return(0);
                }

            }
            for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, SamplerBufferSize, VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            Renderer->PerFrameSamplerDescriptorBuffers + FrameIndex,
                                            Renderer->PerFrameSamplerDescriptorMappings + FrameIndex);
                if (!PushResult)
                {
                    UnhandledError("Failed to push resource descriptor buffer");
                    return(0);
                }
            }
        }

        // Vertex stack
        {
            umm Size = MiB(2);
            for (u32 i = 0; i < R_MaxFramesInFlight; i++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, Size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                           Renderer->PerFrameVertex2DBuffers + i, 
                           Renderer->PerFrameVertex2DMappings + i);
                Renderer->Frames[i].MaxVertex2DCount = Size / sizeof(vertex_2d);
                if (!PushResult)
                {
                    return(0);
                }
            }
        }

        // Joint buffers
        {
            u64 BufferSizePerFrame = render_frame::MaxJointCount * sizeof(m4);
            for (u32 i = 0; i < R_MaxFramesInFlight; i++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, BufferSizePerFrame, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            Renderer->PerFrameJointBuffers + i, 
                                            Renderer->PerFrameJointBufferMappings + i);
                if (!PushResult)
                {
                    return(0);
                }
            }
        }

        // Particle buffers
        {
            u64 BufferSizePerFrame = render_frame::MaxParticleCount * sizeof(render_particle);
            for (u32 i = 0; i < R_MaxFramesInFlight; i++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, BufferSizePerFrame, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            Renderer->PerFrameParticleBuffers + i, 
                                            Renderer->PerFrameParticleBufferMappings + i);
                if (!PushResult)
                {
                    return(0);
                }
            }
        }

        // Per frame uniform data
        {
            constexpr u64 BufferSize = KiB(64);
            for (u32 i = 0; i < R_MaxFramesInFlight; i++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, BufferSize,  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            Renderer->PerFrameUniformBuffers + i,
                                            Renderer->PerFrameUniformBufferMappings + i);
                if (!PushResult)
                {
                    return(0);
                }
            }
            ReturnOnFailure();
        }

        // Per frame descriptors
        for (u32 i = 0; i < R_MaxFramesInFlight; i++)
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

    Renderer->SkinningMemorySize = R_SkinningBufferSize;
    Result = CreateDedicatedBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK.GPUMemTypes, 
                                   Renderer->SkinningMemorySize, &Renderer->SkinningBuffer, &Renderer->SkinningMemory);
    ReturnOnFailure();
    
    Renderer->LightBufferMemorySize = R_MaxLightCount * sizeof(light);
    Result = CreateDedicatedBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK.GPUMemTypes,
                                   Renderer->LightBufferMemorySize, &Renderer->LightBuffer, &Renderer->LightBufferMemory);
    ReturnOnFailure();
    
    Renderer->TileMemorySize = sizeof(screen_tile) * R_MaxTileCountX * R_MaxTileCountY;
    Result = CreateDedicatedBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK.GPUMemTypes,
                                   Renderer->TileMemorySize, &Renderer->TileBuffer, &Renderer->TileMemory);
    ReturnOnFailure();

    Renderer->InstanceMemorySize = MiB(64);
    Result = CreateDedicatedBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK.GPUMemTypes,
                                   Renderer->InstanceMemorySize, &Renderer->InstanceBuffer, &Renderer->InstanceMemory);
    ReturnOnFailure();

    Renderer->DrawMemorySize = MiB(128);
    Result = CreateDedicatedBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK.GPUMemTypes,
                                   Renderer->DrawMemorySize, &Renderer->DrawBuffer, &Renderer->DrawMemory);
    ReturnOnFailure();

    // Shadow storage
    {
        VkFormat ShadowFormat = FormatTable[SHADOW_FORMAT];
        u32 MemoryTypeIndex = 0;
        {
            VkImageCreateInfo DummyImageInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = ShadowFormat,
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

        // Directional cascaded shadow maps
        {
            Renderer->ShadowCascadeCount = R_MaxShadowCascadeCount;
            VkImageCreateInfo ImageInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = ShadowFormat,
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
                    .format = ShadowFormat,
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

        // Point shadow maps
        for (u32 ShadowMapIndex = 0; ShadowMapIndex < R_MaxShadowCount; ShadowMapIndex++)
        {
            point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowMapIndex;

            VkImageCreateInfo ImageInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = ShadowFormat,
                .extent = { R_PointShadowResolution, R_PointShadowResolution, 1 },
                .mipLevels = 1,
                .arrayLayers = 6,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            Result = vkCreateImage(VK.Device, &ImageInfo, nullptr, &ShadowMap->Image);
            ReturnOnFailure();

            VkMemoryRequirements MemoryRequirements = {};
            vkGetImageMemoryRequirements(VK.Device, ShadowMap->Image, &MemoryRequirements);
            if ((1u << MemoryTypeIndex) & MemoryRequirements.memoryTypeBits)
            {
                umm Offset = Align(Renderer->ShadowMemoryOffset, MemoryRequirements.alignment);
                if (Offset + MemoryRequirements.size < Renderer->ShadowMemorySize)
                {
                    Result = vkBindImageMemory(VK.Device, ShadowMap->Image, Renderer->ShadowMemory, Offset);
                    ReturnOnFailure();
                    Renderer->ShadowMemoryOffset = Offset + MemoryRequirements.size;
                }
                else
                {
                    UnhandledError("Out of shadow memory");
                    return(0);
                }
            }
            else
            {
                UnhandledError("Invalid point shadow map memory type");
                return(0);
            }

            VkImageViewCreateInfo ViewInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = ShadowMap->Image,
                .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
                .format = ShadowFormat,
                .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 6,
                },
            };
            Result = vkCreateImageView(VK.Device, &ViewInfo, nullptr, &ShadowMap->CubeView);
            ReturnOnFailure();

            for (u32 LayerIndex = 0; LayerIndex < 6; LayerIndex++)
            {
                VkImageViewCreateInfo LayerViewInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = ShadowMap->Image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = ShadowFormat,
                    .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
                    .subresourceRange = 
                    {
                        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = LayerIndex,
                        .layerCount = 1,
                    },
                };
                Result = vkCreateImageView(VK.Device, &LayerViewInfo, nullptr, &ShadowMap->LayerViews[LayerIndex]);
                ReturnOnFailure();
            }
        }
    }

    // Create system pipeline layout
    {
        VkPushConstantRange PushConstantRange = 
        {
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0,
            .size = MinPushConstantSize,
        };
        VkPipelineLayoutCreateInfo Info = 
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = SystemSet_Count,
            .pSetLayouts = Renderer->SetLayouts,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &PushConstantRange,
        };

        Result = vkCreatePipelineLayout(VK.Device, &Info, nullptr, &Renderer->SystemPipelineLayout);
        ReturnOnFailure();
    }
    Result = CreatePipelines(Renderer, Scratch);
    ReturnOnFailure();
    return(Renderer);
}

internal VkResult ResizeRenderTargets(renderer* Renderer, b32 Forced)
{
    VkResult Result = VK_SUCCESS;

    VkSurfaceCapabilitiesKHR SurfaceCaps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VK.PhysicalDevice, Renderer->Surface, &SurfaceCaps);

    b32 ExtentChanged = 
        (Renderer->SurfaceExtent.width != SurfaceCaps.currentExtent.width) ||
        (Renderer->SurfaceExtent.height != SurfaceCaps.currentExtent.height);
    if (ExtentChanged || Forced)
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

                constexpr VkImageUsageFlagBits DepthStencil = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                constexpr VkImageUsageFlagBits Color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                constexpr VkImageUsageFlagBits Sampled = VK_IMAGE_USAGE_SAMPLED_BIT;
                constexpr VkImageUsageFlagBits Storage = VK_IMAGE_USAGE_STORAGE_BIT;
                
                Renderer->DepthBuffer           = PushRenderTarget("Depth",         &Renderer->RenderTargetHeap, FormatTable[DEPTH_FORMAT], DepthStencil|Sampled, 1);
                Renderer->StructureBuffer       = PushRenderTarget("Structure",     &Renderer->RenderTargetHeap, FormatTable[STRUCTURE_BUFFER_FORMAT], Color|Sampled, 1);
                Renderer->HDRRenderTarget       = PushRenderTarget("HDR",           &Renderer->RenderTargetHeap, FormatTable[HDR_FORMAT], Color|Sampled|Storage, 0);
                Renderer->BloomTarget           = PushRenderTarget("Bloom",         &Renderer->RenderTargetHeap, FormatTable[HDR_FORMAT], Color|Sampled|Storage, 0);
                Renderer->OcclusionBuffers[0]   = PushRenderTarget("OcclusionRaw",  &Renderer->RenderTargetHeap, FormatTable[SSAO_FORMAT], Color|Sampled|Storage, 1);
                Renderer->OcclusionBuffers[1]   = PushRenderTarget("Occlusion",     &Renderer->RenderTargetHeap, FormatTable[SSAO_FORMAT], Color|Sampled|Storage, 1);
                Renderer->VisibilityBuffer      = PushRenderTarget("Visibility",    &Renderer->RenderTargetHeap, FormatTable[VISIBILITY_FORMAT], Color|Sampled, 1);
            }

            if (!ResizeRenderTargets(&Renderer->RenderTargetHeap, Renderer->SurfaceExtent.width, Renderer->SurfaceExtent.height))
            {
                UnhandledError("Failed to resize render targets");
                Result = VK_ERROR_UNKNOWN;
            }

            VkPresentModeKHR PresentMode = 
                //VK_PRESENT_MODE_FIFO_KHR
                //VK_PRESENT_MODE_MAILBOX_KHR
                VK_PRESENT_MODE_IMMEDIATE_KHR
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

#undef ReturnOnFailure

internal void 
DrawList(render_frame* Frame, VkCommandBuffer CmdBuffer, draw_list* List)
{
    vkCmdBindIndexBuffer(CmdBuffer, Frame->Renderer->GeometryBuffer.IndexMemory.Buffer, 0, VK_INDEX_TYPE_UINT32);

    umm CurrentOffset = 0;
    for (u32 Group = 0; Group < DrawGroup_Count; Group++)
    {
        VkBuffer VertexBuffer = VK_NULL_HANDLE;
        switch (Group)
        {
            case DrawGroup_Opaque:
            {
                VertexBuffer = Frame->Renderer->GeometryBuffer.VertexMemory.Buffer;
            } break;
            case DrawGroup_Skinned:
            {
                VertexBuffer = Frame->Renderer->SkinningBuffer;
            } break;
            InvalidDefaultCase;
        }

        VkDeviceSize ZeroOffset = 0;
        vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &VertexBuffer, &ZeroOffset);
        vkCmdDrawIndexedIndirect(CmdBuffer, Frame->Renderer->DrawBuffer, List->DrawBufferOffset + CurrentOffset, List->DrawGroupDrawCounts[Group], sizeof(VkDrawIndexedIndirectCommand));
        CurrentOffset += List->DrawGroupDrawCounts[Group] * sizeof(VkDrawIndexedIndirectCommand);
    }
}

//
// Rendering interface implementation
//

extern "C" Signature_AllocateGeometry(AllocateGeometry)
{
    geometry_buffer_allocation Result = AllocateVertexBuffer(&Renderer->GeometryBuffer, VertexCount, IndexCount);
    return(Result);
}

extern "C" Signature_BeginRenderFrame(BeginRenderFrame)
{
    u32 FrameID = (u32)(Renderer->CurrentFrameID++ % R_MaxFramesInFlight);
    render_frame* Frame = Renderer->Frames + FrameID;
    Frame->Renderer = Renderer;
    Frame->Arena = Arena;

    Frame->ReloadShaders = false;

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

    // Reset buffers
    {
        Frame->StagingBufferAt = 0;
        Frame->Backend->StagingBuffer = Renderer->StagingBuffers[FrameID];
        Frame->TransferOpCount = 0;

        Frame->MaxLightCount = R_MaxLightCount;
        Frame->LightCount = 0;
        Frame->Lights = PushArray(Arena, 0, light, Frame->MaxLightCount);

        Frame->MaxShadowCount = R_MaxShadowCount;
        Frame->ShadowCount = 0;
        Frame->Shadows = PushArray(Arena, 0, u32, Frame->MaxShadowCount);

        Frame->MaxDrawCmdCount = 1u << 20;
        Frame->DrawCmdCount = 0;
        Frame->DrawCmds = PushArray(Arena, 0, draw_cmd, Frame->MaxDrawCmdCount);
    
        Frame->MaxSkinnedDrawCmdCount = 1u << 20;
        Frame->SkinnedDrawCmdCount = 0;
        Frame->SkinnedDrawCmds = PushArray(Arena, 0, draw_cmd, Frame->MaxSkinnedDrawCmdCount);

        Frame->MaxSkinningCmdCount = Frame->MaxSkinnedDrawCmdCount;
        Frame->SkinningCmdCount = 0;
        Frame->SkinningCmds = PushArray(Arena, 0, skinning_cmd, Frame->MaxSkinningCmdCount);

        Frame->MaxParticleDrawCmdCount = 8192u;
        Frame->ParticleDrawCmdCount = 0;
        Frame->ParticleDrawCmds = PushArray(Arena, 0, particle_cmd, Frame->MaxParticleDrawCmdCount);

        Frame->MaxDrawWidget3DCmdCount = 1u << 16;
        Frame->DrawWidget3DCmdCount = 0;
        Frame->DrawWidget3DCmds = PushArray(Arena, 0, draw_widget3d_cmd, Frame->MaxDrawWidget3DCmdCount);

        Frame->Backend->UniformBuffer = Renderer->PerFrameUniformBuffers[FrameID];
        Frame->UniformData = Renderer->PerFrameUniformBufferMappings[FrameID];

        Frame->Backend->Vertex2DBuffer = Renderer->PerFrameVertex2DBuffers[FrameID];

        Frame->Vertex2DCount = 0;
        Frame->Vertex2DArray = (vertex_2d*)Renderer->PerFrameVertex2DMappings[FrameID];

        Frame->ParticleCount = 0;
        Frame->Backend->ParticleBuffer = Renderer->PerFrameParticleBuffers[FrameID];
        Frame->Particles = (render_particle*)Renderer->PerFrameParticleBufferMappings[FrameID];

        Frame->JointCount = 0;
        Frame->Backend->JointBuffer = Renderer->PerFrameJointBuffers[FrameID];
        Frame->JointMapping = (m4*)Renderer->PerFrameJointBufferMappings[FrameID];
        Frame->JointBufferAlignment = TruncateU64ToU32(VK.ConstantBufferAlignment) / sizeof(m4);

        Frame->MaxSkinnedVertexCount = TruncateU64ToU32(R_SkinningBufferSize / sizeof(vertex));
        Frame->SkinnedMeshVertexCount = 0;
    }

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

    vkResetCommandPool(VK.Device, Frame->Backend->CmdPool, 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
    vkResetCommandPool(VK.Device, Frame->Backend->ComputeCmdPool, 0);

    return(Frame);
}

extern "C" Signature_EndRenderFrame(EndRenderFrame)
{
    renderer* Renderer = Frame->Renderer;
    if (Frame->ReloadShaders)
    {
        vkDeviceWaitIdle(Frame->Renderer->Vulkan.Device);
        CreatePipelines(Renderer, Frame->Arena);
    }

    // NOTE(boti): This is currently also the initial transfer and skinning cmd buffer
    VkCommandBuffer PrepassCmd = Frame->Backend->CmdBuffers[Frame->Backend->CmdBufferAt++];
    VkCommandBuffer RenderCmd = Frame->Backend->CmdBuffers[Frame->Backend->CmdBufferAt++];
    // NOTE(boti): Light binning, SSAO
    VkCommandBuffer PreLightCmd = Frame->Backend->ComputeCmdBuffer;
    VkCommandBuffer ShadowCmd = Frame->Backend->CmdBuffers[Frame->Backend->CmdBufferAt++];

    f32 AspectRatio = (f32)Frame->RenderWidth / (f32)Frame->RenderHeight;

    Frame->Uniforms.Ambience                    = 0.25f * v3{ 1.0f, 1.0f, 1.0f }; // TODO(boti): Expose this in the API
    Frame->Uniforms.Exposure                    = Frame->Config.Exposure;
    Frame->Uniforms.SSAOIntensity               = Frame->Config.SSAOIntensity;
    Frame->Uniforms.SSAOInverseMaxDistance      = 1.0f / Frame->Config.SSAOMaxDistance;
    Frame->Uniforms.SSAOTangentTau              = Frame->Config.SSAOTangentTau;
    Frame->Uniforms.BloomFilterRadius           = Frame->Config.BloomFilterRadius;
    Frame->Uniforms.BloomInternalStrength       = Frame->Config.BloomInternalStrength;
    Frame->Uniforms.BloomStrength               = Frame->Config.BloomStrength;
    Frame->Uniforms.ConstantFogDensity          = Frame->ConstantFogDensity;
    Frame->Uniforms.LinearFogDensityAtBottom    = Frame->LinearFogDensityAtBottom;
    Frame->Uniforms.LinearFogMinZ               = Frame->LinearFogMinZ;
    Frame->Uniforms.LinearFogMaxZ               = Frame->LinearFogMaxZ;

    // Calculate camera parameters
    {
        f32 n = Frame->CameraNearPlane;
        f32 f = Frame->CameraFarPlane;
        f32 r = 1.0f / (f - n);
        f32 s = AspectRatio;
        f32 g = Frame->CameraFocalLength;

        Frame->ViewTransform = AffineOrthonormalInverse(Frame->CameraTransform);

#if 1
        // Reverse Z
        Frame->ProjectionTransform = M4(
            g / s, 0.0f, 0.0f, 0.0f,
            0.0f, g, 0.0f, 0.0f,
            0.0f, 0.0f, -n*r, n*f*r,
            0.0f, 0.0f, 1.0f, 0.0f);
        Frame->InverseProjectionTransform = M4(
            s / g,  0.0f,       0.0f,           0.0f,
            0.0f,   1.0f / g,   0.0f,           0.0f,
            0.0f,   0.0f,       0.0f,           1.0f,
            0.0f,   0.0f,       1.0f / (n*f*r), 1.0f / f);
#else
        // Forward Z
        Frame->ProjectionTransform = M4(
            g / s, 0.0f, 0.0f, 0.0f,
            0.0f, g, 0.0f, 0.0f,
            0.0f, 0.0f, f * r, -f*n * r,
            0.0f, 0.0f, 1.0f, 0.0f);
        Frame->InverseProjectionTransform = M4(
            s / g, 0.0f,     0.0f,            0.0f,
            0.0f,  1.0f / g, 0.0f,            0.0f,
            0.0f,  0.0f,     0.0f,            1.0f,
            0.0f,  0.0f,     1.0f / (-f*n*r), 1.0f / n);
#endif

        f32 g2 = g*g;
        f32 mx = 1.0f / Sqrt(g2 + s*s);
        f32 my = 1.0f / Sqrt(g2 + 1.0f);
        f32 gmx = g*mx;
        f32 gmy = g*my;
        f32 smx = s*mx;
        Frame->CameraFrustum = 
        {
            .Left   = v4{ -gmx, 0.0f, smx, 0.0f } * Frame->ViewTransform,
            .Right  = v4{ +gmx, 0.0f, smx, 0.0f } * Frame->ViewTransform,
            .Top    = v4{ 0.0f, -gmy,  my, 0.0f } * Frame->ViewTransform,
            .Bottom = v4{ 0.0f, +gmy,  my, 0.0f } * Frame->ViewTransform,
            .Near   = v4{ 0.0f, 0.0f, +1.0f, -n } * Frame->ViewTransform,
            .Far    = v4{ 0.0f, 0.0f, -1.0f, +f } * Frame->ViewTransform,
        };

        Frame->Uniforms.CameraTransform = Frame->CameraTransform;
        Frame->Uniforms.ViewTransform = Frame->ViewTransform;
        Frame->Uniforms.ProjectionTransform = Frame->ProjectionTransform;
        Frame->Uniforms.InverseProjectionTransform = Frame->InverseProjectionTransform;
        Frame->Uniforms.ViewProjectionTransform = Frame->ProjectionTransform * Frame->ViewTransform;
        
        Frame->Uniforms.FocalLength = Frame->CameraFocalLength;
        Frame->Uniforms.AspectRatio = AspectRatio;
        Frame->Uniforms.NearZ = Frame->CameraNearPlane;
        Frame->Uniforms.FarZ = Frame->CameraFarPlane;
        
        Frame->Uniforms.SunV = TransformDirection(Frame->ViewTransform, Frame->SunV);
        Frame->Uniforms.SunL = Frame->SunL;
    }

    // Acquire image
    {
        vkWaitForFences(VK.Device, 1, &Frame->Backend->ImageAcquiredFence, VK_TRUE, UINT64_MAX);
        vkResetFences(VK.Device, 1, &Frame->Backend->ImageAcquiredFence);

        if (Frame->RenderWidth != Renderer->SurfaceExtent.width ||
            Frame->RenderHeight != Renderer->SurfaceExtent.height)
        {
            ResizeRenderTargets(Renderer, false);
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
                ResizeRenderTargets(Renderer, true);
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

    frustum* ShadowFrustums = PushArray(Frame->Arena, 0, frustum, R_MaxShadowCount);

    for (u32 ShadowIndex = 0; ShadowIndex < Frame->ShadowCount; ShadowIndex++)
    {
        light* Light = Frame->Lights + Frame->Shadows[ShadowIndex];
        v3 P = Light->P;

        f32 L = GetLuminance(Light->E);
        f32 R = Sqrt(Max((L / R_LuminanceThreshold), 0.0f));
        // TODO(boti): Get the far plane from the luminance and the light threshold
        f32 n = 0.05f;
        f32 f = R + 1e-6f;
        f32 r = 1.0f / (f - n);
        m4 Projection = M4(1.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, f*r, -f*n*r,
                           0.0f, 0.0f, 1.0f, 0.0f);
        m4 InverseProjection = M4(1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 0.0f, 1.0f,
                                  0.0f, 0.0f, -1.0f / (f*n*r), 1.0f / n);
        point_shadow_data* Shadow = Frame->Uniforms.PointShadows + ShadowIndex;
        Shadow->Near = n;
        Shadow->Far = f;

        for (u32 LayerIndex = 0; LayerIndex < Layer_Count; LayerIndex++)
        {
            m3 M = GlobalCubeFaceBases[LayerIndex];
            m4 InverseView = M4(M.X.X, M.Y.X, M.Z.X, P.X,
                                M.X.Y, M.Y.Y, M.Z.Y, P.Y,
                                M.X.Z, M.Y.Z, M.Z.Z, P.Z,
                                0.0f, 0.0f, 0.0f ,1.0f);
            m4 View = M4(M.X.X, M.X.Y, M.X.Z, -Dot(M.X, P),
                         M.Y.X, M.Y.Y, M.Y.Z, -Dot(M.Y, P),
                         M.Z.X, M.Z.Y, M.Z.Z, -Dot(M.Z, P),
                         0.0f, 0.0f, 0.0f, 1.0f);
            m4 ViewProjection = Projection * View;
            Shadow->ViewProjections[LayerIndex] = ViewProjection;

            m4 InverseViewProjection = InverseView * InverseProjection;

            frustum ClipSpaceFrustum = GetClipSpaceFrustum();
            frustum* Frustum = &ShadowFrustums[6 * ShadowIndex + LayerIndex];
            for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
            {
                Frustum->Planes[PlaneIndex] = ClipSpaceFrustum.Planes[PlaneIndex] * Projection * View;
            }
        }
    }

    //
    // Transfer
    //
    VkCommandBufferBeginInfo CmdBufferBegin = 
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkBeginCommandBuffer(PrepassCmd, &CmdBufferBegin);

    for (u32 TransferOpIndex = 0; TransferOpIndex < Frame->TransferOpCount; TransferOpIndex++)
    {
        transfer_op* Op = Frame->TransferOps + TransferOpIndex;
        switch (Op->Type)
        {
            case TransferOp_Texture:
            {
                texture_info* Info = &Op->Texture.Info;
                if (Info->Depth > 1)
                {
                    UnimplementedCodePath;
                }

                format_byterate ByteRate = FormatByterateTable[Info->Format];
                umm TotalSize = GetMipChainSize(Info->Width, Info->Height, Info->MipCount, Info->ArrayCount, ByteRate);

                u32 CopyCount = Info->MipCount * Info->ArrayCount;
                VkBufferImageCopy* Copies = PushArray(Frame->Arena, 0, VkBufferImageCopy, CopyCount);
                VkBufferImageCopy* CopyAt = Copies;

                u64 Offset = Op->SourceOffset;
                for (u32 ArrayIndex = 0; ArrayIndex < Info->ArrayCount; ArrayIndex++)
                {
                    for (u32 MipIndex = 0; MipIndex < Info->MipCount; MipIndex++)
                    {
                        u32 Width = Max(Info->Width >> MipIndex, 1u);
                        u32 Height = Max(Info->Height >> MipIndex, 1u);

                        u32 RowLength = 0;
                        u32 ImageHeight = 0;
                        u64 TexelCount;
                        if (ByteRate.Flags & FormatFlag_BlockCompressed)
                        {
                            RowLength = Align(Width, 4u);
                            ImageHeight = Align(Height, 4u);

                            TexelCount = (u64)RowLength * ImageHeight;
                        }
                        else
                        {
                            TexelCount = Width * Height;
                        }
                        u64 MipSize = TexelCount * ByteRate.Numerator / ByteRate.Denominator;

                        CopyAt->bufferOffset = Offset,
                        CopyAt->bufferRowLength = RowLength,
                        CopyAt->bufferImageHeight = ImageHeight,
                        CopyAt->imageSubresource = 
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .mipLevel = MipIndex,
                            .baseArrayLayer = ArrayIndex,
                            .layerCount = 1,
                        },
                        CopyAt->imageOffset = { 0, 0, 0 },
                        CopyAt->imageExtent = { Width, Height, 1 },

                        Offset += MipSize;
                        CopyAt++;
                    }
                }

                VkImage* Image = GetImage(&Renderer->TextureManager, Op->Texture.TargetID);
                if (*Image == VK_NULL_HANDLE)
                {
                    AllocateTexture(&Renderer->TextureManager, Op->Texture.TargetID, *Info);
                }
                else if (AreTextureInfosSameFormat(Op->Texture.Info, GetTextureInfo(&Renderer->TextureManager, Op->Texture.TargetID)))
                {
                    // NOTE(boti): Nothing to do here, although in the future we might want to allocate memory here
                    // (currently that's always already done in this case)
                }
                else
                {
                    UnimplementedCodePath;
                }

                if (!IsTextureSpecial(Op->Texture.TargetID))
                {
                    umm DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize;
                    if ((Frame->StagingBufferAt + DescriptorSize) <= Frame->StagingBufferSize)
                    {
                        VkDescriptorImageInfo DescriptorImage = 
                        {
                            .sampler = VK_NULL_HANDLE,
                            .imageView = *GetImageView(&Renderer->TextureManager, Op->Texture.TargetID),
                            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                        };
                        VkDescriptorGetInfoEXT DescriptorInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                            .pNext = nullptr,
                            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                            .data = { .pSampledImage = &DescriptorImage },
                        };
                        vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Frame->StagingBufferBase, Frame->StagingBufferAt));

                        umm DescriptorOffset = Renderer->TextureManager.TextureTableOffset + Op->Texture.TargetID.Value*DescriptorSize; // TODO(boti): standardize this
                        VkBufferCopy DescriptorCopy = 
                        {
                            .srcOffset = Frame->StagingBufferAt,
                            .dstOffset = DescriptorOffset,
                            .size = DescriptorSize,
                        };
                        vkCmdCopyBuffer(PrepassCmd, Frame->Backend->StagingBuffer, Renderer->TextureManager.DescriptorBuffer, 1, &DescriptorCopy);

                        Frame->StagingBufferAt += DescriptorSize;
                    }
                    else
                    {
                        UnimplementedCodePath;
                    }
                }

                VkImageMemoryBarrier2 BeginBarrier = 
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                    .srcAccessMask = VK_ACCESS_2_NONE,
                    .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = *Image,
                    .subresourceRange = 
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount = VK_REMAINING_ARRAY_LAYERS,
                    },
                };
                VkDependencyInfo BeginDependency = 
                {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext = nullptr,
                    .dependencyFlags = 0,
                    .imageMemoryBarrierCount = 1,
                    .pImageMemoryBarriers = &BeginBarrier,
                };
                vkCmdPipelineBarrier2(PrepassCmd, &BeginDependency);
                vkCmdCopyBufferToImage(PrepassCmd, Frame->Backend->StagingBuffer, *Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       CopyCount, Copies);

                VkImageMemoryBarrier2 EndBarrier = 
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = *Image,
                    .subresourceRange = 
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount = VK_REMAINING_ARRAY_LAYERS,
                    },
                };
                VkDependencyInfo EndDependency =
                {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext = nullptr,
                    .dependencyFlags = 0,
                    .imageMemoryBarrierCount = 1,
                    .pImageMemoryBarriers = &EndBarrier,
                };
                vkCmdPipelineBarrier2(PrepassCmd, &EndDependency);
            } break;

            case TransferOp_Geometry:
            {
                umm VertexByteCount = Op->Geometry.Dest.VertexBlock->Count * sizeof(vertex);
                umm VertexByteOffset = Op->Geometry.Dest.VertexBlock->Offset * sizeof(vertex);
                umm IndexByteCount = Op->Geometry.Dest.IndexBlock->Count * sizeof(vert_index);
                umm IndexByteOffset = Op->Geometry.Dest.IndexBlock->Offset * sizeof(vert_index);
                VkBuffer VertexBuffer = Renderer->GeometryBuffer.VertexMemory.Buffer;
                VkBuffer IndexBuffer = Renderer->GeometryBuffer.IndexMemory.Buffer;

                VkBufferMemoryBarrier2 Barrier = 
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = 0,
                    .srcAccessMask = 0,
                    .dstStageMask = 0,
                    .dstAccessMask = 0,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = VK_NULL_HANDLE,
                    .offset = 0,
                    .size = 0,
                };
                VkDependencyInfo Dependency = 
                {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext = nullptr,
                    .dependencyFlags = 0,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &Barrier,
                };

                if (VertexByteCount)
                {

                    Barrier.buffer = VertexBuffer;
                    Barrier.offset = VertexByteOffset;
                    Barrier.size = VertexByteCount;

                    Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
                    Barrier.srcAccessMask = VK_ACCESS_2_NONE;
                    Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                    Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                    vkCmdPipelineBarrier2(PrepassCmd, &Dependency);

                    VkBufferCopy Copy = 
                    {
                        .srcOffset = Op->SourceOffset,
                        .dstOffset = VertexByteOffset,
                        .size = VertexByteCount,
                    };
                    vkCmdCopyBuffer(PrepassCmd, Frame->Backend->StagingBuffer, VertexBuffer, 1, &Copy);

                    Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                    Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                    Barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                    Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT|VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
                    vkCmdPipelineBarrier2(PrepassCmd, &Dependency);
                }

                if (IndexByteCount)
                {
                    Barrier.buffer = IndexBuffer;
                    Barrier.offset = IndexByteOffset;
                    Barrier.size = IndexByteCount;

                    Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
                    Barrier.srcAccessMask = VK_ACCESS_2_NONE;
                    Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                    Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                    vkCmdPipelineBarrier2(PrepassCmd, &Dependency);
                    VkBufferCopy Copy = 
                    {
                        .srcOffset = Op->SourceOffset + VertexByteCount,
                        .dstOffset = IndexByteOffset,
                        .size = IndexByteCount,
                    };
                    vkCmdCopyBuffer(PrepassCmd, Frame->Backend->StagingBuffer, IndexBuffer, 1, &Copy);

                    Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                    Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                    Barrier.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
                    Barrier.dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT;
                    vkCmdPipelineBarrier2(PrepassCmd, &Dependency);
                }
            } break;
            InvalidDefaultCase;
        }
    }

    
    VkBufferMemoryBarrier2 EndTransferStageBufferMemoryBarriers[] = 
    {
        // Texture descriptor buffer
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = Renderer->TextureManager.DescriptorBuffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };

    VkDependencyInfo EndTransferStageDependency = 
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .bufferMemoryBarrierCount = CountOf(EndTransferStageBufferMemoryBarriers),
        .pBufferMemoryBarriers = EndTransferStageBufferMemoryBarriers,
    };
    vkCmdPipelineBarrier2(PrepassCmd, &EndTransferStageDependency);

    // Upload instance data
    umm DrawBufferAt = 0;
    {
        Frame->Uniforms.OpaqueDrawCount = Frame->DrawCmdCount;
        Frame->Uniforms.SkinnedDrawCount = Frame->SkinnedDrawCmdCount;

        u32 TotalDrawCount = Frame->DrawCmdCount + Frame->SkinnedDrawCmdCount;

        umm InstanceDataByteCount = (umm)TotalDrawCount * sizeof(instance_data);
        Assert(InstanceDataByteCount <= Renderer->InstanceMemorySize);
        Frame->StagingBufferAt = Align(Frame->StagingBufferAt, alignof(instance_data));
        Assert((Frame->StagingBufferAt + InstanceDataByteCount) <= Frame->StagingBufferSize);
        umm InstanceDataCopyOffset = Frame->StagingBufferAt;
        Frame->StagingBufferAt += InstanceDataByteCount;
        instance_data* InstanceDataAt = (instance_data*)OffsetPtr(Frame->StagingBufferBase, InstanceDataCopyOffset);

        umm DrawDataByteCount = (umm)TotalDrawCount * sizeof(VkDrawIndexedIndirectCommand);
        Assert(DrawDataByteCount <= Renderer->DrawMemorySize);
        Frame->StagingBufferAt = Align(Frame->StagingBufferAt, alignof(VkDrawIndexedIndirectCommand));
        Assert((Frame->StagingBufferAt + DrawDataByteCount) <= Frame->StagingBufferSize);
        umm DrawDataCopyOffset = Frame->StagingBufferAt;
        Frame->StagingBufferAt += DrawDataByteCount;
        VkDrawIndexedIndirectCommand* DrawDataAt = (VkDrawIndexedIndirectCommand*)OffsetPtr(Frame->StagingBufferBase, DrawDataCopyOffset);
        Frame->StagingBufferAt += InstanceDataByteCount;
        DrawBufferAt += DrawDataByteCount;

        for (u32 It = 0; It < Frame->DrawCmdCount; It++)
        {
            draw_cmd* Cmd = Frame->DrawCmds + It;
            Cmd->Base.InstanceOffset = It; // HACK(boti)
            *DrawDataAt++ = 
            {
                .indexCount     = Cmd->Base.IndexCount,
                .instanceCount  = Cmd->Base.InstanceCount,
                .firstIndex     = Cmd->Base.IndexOffset,
                .vertexOffset   = (s32)Cmd->Base.VertexOffset,
                .firstInstance  = Cmd->Base.InstanceOffset,
            };
            *InstanceDataAt++ = 
            {
                .Transform = Cmd->Transform,
                .Material = Cmd->Material,
            };
        }

        for (u32 It = 0; It < Frame->SkinnedDrawCmdCount; It++)
        {
            draw_cmd* Cmd = Frame->SkinnedDrawCmds + It;
            Cmd->Base.InstanceOffset = Frame->DrawCmdCount + It; // HACK(boti)
            *DrawDataAt++ = 
            {
                .indexCount     = Cmd->Base.IndexCount,
                .instanceCount  = Cmd->Base.InstanceCount,
                .firstIndex     = Cmd->Base.IndexOffset,
                .vertexOffset   = (s32)Cmd->Base.VertexOffset,
                .firstInstance  = Cmd->Base.InstanceOffset,
            };
            *InstanceDataAt++ = 
            {
                .Transform = Cmd->Transform,
                .Material = Cmd->Material,
            };
        }

        VkBufferCopy InstanceCopy = 
        {
            .srcOffset = InstanceDataCopyOffset,
            .dstOffset = 0,
            .size = InstanceDataByteCount,
        };
        
        VkBufferCopy DrawCopy =
        {
            .srcOffset = DrawDataCopyOffset,
            .dstOffset = 0,
            .size = TotalDrawCount * sizeof(VkDrawIndexedIndirectCommand),
        };

        VkBufferMemoryBarrier2 BeginBarriers[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Renderer->InstanceBuffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Renderer->DrawBuffer,
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
        vkCmdCopyBuffer(PrepassCmd, Frame->Backend->StagingBuffer, Renderer->InstanceBuffer, 1, &InstanceCopy);
        vkCmdCopyBuffer(PrepassCmd, Frame->Backend->StagingBuffer, Renderer->DrawBuffer, 1, &DrawCopy);

        VkBufferMemoryBarrier2 EndBarriers[] = 
        {
            {
                
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Renderer->InstanceBuffer,
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

    u32 TileCountX = CeilDiv(Frame->RenderWidth, R_TileSizeX);
    u32 TileCountY = CeilDiv(Frame->RenderHeight, R_TileSizeY);
    Frame->Uniforms.TileCount = { TileCountX, TileCountY };
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

        u32 ShadowAt = 0;
        for (u32 LightIndex = 0; LightIndex < Frame->LightCount; LightIndex++)
        {
            light* Light = Frame->Lights + LightIndex;

            u32 ShadowIndex = 0xFFFFFFFFu;
            if ((ShadowAt < Frame->ShadowCount) && (Frame->Shadows[ShadowAt] == LightIndex))
            {
                    ShadowIndex = ShadowAt++;
            }

            f32 L = GetLuminance(Light->E);
            f32 R = Sqrt(Max((L / R_LuminanceThreshold), 0.0f));
            if (IntersectFrustumSphere(&CameraFrustum, Light->P, R))
            {
                v3 P = TransformPoint(Frame->Uniforms.ViewTransform, Light->P);
                u32 DstIndex = Frame->Uniforms.LightCount++;
                LightBuffer[DstIndex] = 
                {
                    .P = P,
                    .ShadowIndex = ShadowIndex,
                    .E = Light->E,
                    .Flags = Light->Flags,
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

    frustum CascadeFrustums[R_MaxShadowCascadeCount];
    SetupSceneRendering(Frame, CascadeFrustums);

    //
    // Build draw lists and upload draw data
    //
    draw_list PrimaryDrawList = {};
    draw_list CascadeDrawLists[R_MaxShadowCascadeCount] = {};
    draw_list* ShadowDrawLists = PushArray(Frame->Arena, MemPush_Clear, draw_list, 6 * Frame->ShadowCount);
    u32 DrawListCount = 1 + R_MaxShadowCascadeCount + 6 * Frame->ShadowCount;

    {
        Frame->StagingBufferAt = Align(Frame->StagingBufferAt, alignof(VkDrawIndexedIndirectCommand));
        Assert(Frame->StagingBufferAt <= Frame->StagingBufferSize);
        umm MaxDrawCountPerDrawList = Frame->DrawCmdCount + Frame->SkinnedDrawCmdCount;
        umm MaxMemorySizePerDrawList = sizeof(VkDrawIndexedIndirectCommand) * MaxDrawCountPerDrawList;
        umm CopySrcAt = Frame->StagingBufferAt;
        umm TotalDrawCount = 0;
        umm DrawCopyOffset = DrawBufferAt;

        for (u32 DrawListIndex = 0; DrawListIndex < DrawListCount; DrawListIndex++)
        {
            frustum* Frustum = nullptr;
            draw_list* DrawList = nullptr;
            if (DrawListIndex == 0)
            {
                Frustum = &Frame->CameraFrustum;
                DrawList = &PrimaryDrawList;
            }
            else if ((DrawListIndex - 1) < R_MaxShadowCascadeCount)
            {
                u32 Index = DrawListIndex - 1;
                Frustum = CascadeFrustums + Index;
                DrawList = CascadeDrawLists + Index;
            }
            else
            {
                u32 Index = (DrawListIndex - 1 - R_MaxShadowCascadeCount);
                Frustum = ShadowFrustums + Index;
                DrawList = ShadowDrawLists + Index;
            }

            if (((Frame->StagingBufferAt + MaxMemorySizePerDrawList) > Frame->StagingBufferSize) ||
                ((DrawBufferAt + MaxMemorySizePerDrawList) > Renderer->DrawMemorySize))
            {
                UnimplementedCodePath;
                break;
            }

            DrawList->DrawBufferOffset = DrawBufferAt;
            VkDrawIndexedIndirectCommand* DstAt = (VkDrawIndexedIndirectCommand*)OffsetPtr(Frame->StagingBufferBase, Frame->StagingBufferAt);
            umm TotalDrawCountPerList = 0;
            for (u32 Group = 0; Group < DrawGroup_Count; Group++)
            {
                u32 CmdCount = 0;
                draw_cmd* FirstCmd = nullptr;
                switch (Group)
                {
                    case DrawGroup_Opaque: 
                    {
                        CmdCount = Frame->DrawCmdCount;
                        FirstCmd = Frame->DrawCmds;
                    } break;
                    case DrawGroup_Skinned: 
                    {
                        CmdCount = Frame->SkinnedDrawCmdCount;
                        FirstCmd = Frame->SkinnedDrawCmds;
                    } break;
                    InvalidDefaultCase;
                }
                
                for (u32 CmdIndex = 0; CmdIndex < CmdCount; CmdIndex++)
                {
                    draw_cmd* Cmd = FirstCmd + CmdIndex;
                    b32 IsVisible = true;

                    // NOTE(boti): We don't have correct BBox information about skinned meshes,
                    //             so those are never culled
                    if (Frustum && (Group != DrawGroup_Skinned))
                    {
                        IsVisible = IntersectFrustumBox(Frustum, Cmd->BoundingBox, Cmd->Transform);
                    }
            
                    if (IsVisible)
                    {
                        DrawList->DrawGroupDrawCounts[Group] += 1;
                        *DstAt++ = 
                        {
                            .indexCount     = Cmd->Base.IndexCount,
                            .instanceCount  = Cmd->Base.InstanceCount,
                            .firstIndex     = Cmd->Base.IndexOffset,
                            .vertexOffset   = (s32)Cmd->Base.VertexOffset,
                            .firstInstance  = Cmd->Base.InstanceOffset,
                        };
                    }
                }
                
                TotalDrawCountPerList += DrawList->DrawGroupDrawCounts[Group];
            }
            
            TotalDrawCount += TotalDrawCountPerList;
            umm MemorySize = TotalDrawCountPerList * sizeof(VkDrawIndexedIndirectCommand);
            DrawBufferAt += MemorySize;
            Frame->StagingBufferAt += MemorySize;
        }

        if (TotalDrawCount)
        {
            VkBufferCopy Copy =
            {
                .srcOffset = CopySrcAt,
                .dstOffset = DrawCopyOffset,
                .size = TotalDrawCount * sizeof(VkDrawIndexedIndirectCommand),
            };

            VkDependencyInfo BeginDependency = 
            {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = nullptr,
            };
            vkCmdPipelineBarrier2(PrepassCmd, &BeginDependency);

            vkCmdCopyBuffer(PrepassCmd, Frame->Backend->StagingBuffer, Renderer->DrawBuffer, 1, &Copy);

            VkBufferMemoryBarrier2 EndBarriers[] = 
            {
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
                    .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT|VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = Renderer->DrawBuffer,
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
    }

    // TODO(boti): Move these out of the frame, these descriptors are static
    {
        VkBuffer SamplerBuffer = Renderer->PerFrameSamplerDescriptorBuffers[Frame->FrameID];
        void* SamplerBufferMapping = Renderer->PerFrameSamplerDescriptorMappings[Frame->FrameID];

        umm SamplerDescriptorSize = VK.DescriptorBufferProps.samplerDescriptorSize;
        VkDeviceSize SamplerTableOffset = 0;
        vkGetDescriptorSetLayoutBindingOffsetEXT(VK.Device, Renderer->SetLayouts[SetLayout_Samplers], Binding_Sampler_NamedSamplers, &SamplerTableOffset);
        for (u32 SamplerIndex = 0; SamplerIndex < Sampler_Count; SamplerIndex++)
        {
            VkDescriptorGetInfoEXT DescriptorInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .data = { .pSampler = &Renderer->Samplers[SamplerIndex] },
            };

            vkGetDescriptorEXT(VK.Device, &DescriptorInfo, SamplerDescriptorSize, OffsetPtr(SamplerBufferMapping, SamplerTableOffset + SamplerIndex*SamplerDescriptorSize));
        }
    }

    {
        VkDescriptorBufferInfo ConstantsInfo = 
        {
            .buffer = Frame->Backend->UniformBuffer,
            .offset = 0,
            .range = KiB(64),
        };

        VkDescriptorBufferInfo LightBufferInfo = 
        {
            .buffer = Renderer->LightBuffer,
            .offset = 0,
            .range = Renderer->LightBufferMemorySize,
        };
        VkDescriptorBufferInfo TileBufferInfo = 
        {
            .buffer = Renderer->TileBuffer,
            .offset = 0,
            .range = Renderer->TileMemorySize,
        };
        VkDescriptorBufferInfo InstanceBufferInfo = 
        {
            .buffer = Renderer->InstanceBuffer,
            .offset = 0,
            .range = Renderer->InstanceMemorySize,
        };
        VkDescriptorBufferInfo DrawBufferInfo = 
        {
            .buffer = Renderer->DrawBuffer,
            .offset = 0,
            .range = Renderer->DrawMemorySize,
        };
        VkDescriptorBufferInfo IndexBufferInfo = 
        {
            .buffer = Renderer->GeometryBuffer.IndexMemory.Buffer,
            .offset = 0,
            .range = geometry_memory::GPUBlockSize,
        };
        VkDescriptorBufferInfo VertexBufferInfo = 
        {
            .buffer = Renderer->GeometryBuffer.VertexMemory.Buffer,
            .offset = 0,
            .range = geometry_memory::GPUBlockSize,
        };
        VkDescriptorBufferInfo SkinnedVertexBufferInfo = 
        {
            .buffer = Renderer->SkinningBuffer,
            .offset = 0,
            .range = Renderer->SkinningMemorySize,
        };
        VkDescriptorImageInfo VisibilityBufferInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->VisibilityBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo OcclusionBufferInfo =
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->OcclusionBuffers[1]->View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo StructureBufferInfo =
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->StructureBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo OcclusionRawStorageImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->OcclusionBuffers[0]->View,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkDescriptorImageInfo OcclusionStorageImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->OcclusionBuffers[1]->View,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkDescriptorImageInfo OcclusionRawImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->OcclusionBuffers[0]->View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo HDRColorStorageImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->HDRRenderTarget->MipViews[0],
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkDescriptorImageInfo HDRColorImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->HDRRenderTarget->View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo BloomImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->BloomTarget->View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo CascadedShadowImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->ShadowView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorImageInfo HDRMipStorageImageInfos[R_MaxMipCount];
        VkDescriptorImageInfo BloomMipStorageImageInfos[R_MaxMipCount];
        Assert(Renderer->HDRRenderTarget->MipCount == Renderer->BloomTarget->MipCount);
        for (u32 Mip = 0; Mip < Renderer->HDRRenderTarget->MipCount; Mip++)
        {
            HDRMipStorageImageInfos[Mip] = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Renderer->HDRRenderTarget->MipViews[Mip],
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            BloomMipStorageImageInfos[Mip] = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Renderer->BloomTarget->MipViews[Mip],
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
        }

        VkDescriptorImageInfo HDRColorImageGeneralInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->HDRRenderTarget->View,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkDescriptorImageInfo BloomImageGeneralInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = Renderer->BloomTarget->View,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkDescriptorImageInfo PointShadowInfos[R_MaxShadowCount] = {};
        for (u32 PointShadowIndex = 0; PointShadowIndex < R_MaxShadowCount; PointShadowIndex++)
        {
            PointShadowInfos[PointShadowIndex] = 
            {
                .sampler = VK_NULL_HANDLE,
                .imageView = Renderer->PointShadowMaps[PointShadowIndex].CubeView,
                .imageLayout= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
        }
        VkDescriptorBufferInfo JointBufferInfo = 
        {
            .buffer = Renderer->PerFrameJointBuffers[Frame->FrameID],
            .offset = 0,
            .range = render_frame::MaxJointCount * sizeof(m4), // TODO(boti): deduplicate (this is also in init)
        };
        VkDescriptorBufferInfo ParticleBufferInfo = 
        {
            .buffer = Renderer->PerFrameParticleBuffers[Frame->FrameID],
            .offset = 0,
            .range = render_frame::MaxParticleCount * sizeof(render_particle), // TODO(boti): deduplicate (this is also in init)
        };
        VkDescriptorImageInfo ParticleImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = *GetImageView(&Renderer->TextureManager, Frame->ParticleTextureID),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo UIImageInfo = 
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = *GetImageView(&Renderer->TextureManager, Frame->ImmediateTextureID),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet Writes[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_Constants,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &ConstantsInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_LightBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &LightBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_TileBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &TileBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_InstanceBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &InstanceBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_DrawBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &DrawBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_IndexBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &IndexBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_VertexBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &VertexBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_SkinnedVertexBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &SkinnedVertexBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_VisibilityImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &VisibilityBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_OcclusionImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &OcclusionBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_StructureImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &StructureBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_OcclusionRawStorageImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &OcclusionRawStorageImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_OcclusionStorageImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &OcclusionStorageImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_OcclusionRawImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &OcclusionRawImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_HDRColorStorageImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &HDRColorStorageImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_HDRColorImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &HDRColorImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_BloomImage,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &BloomImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_CascadedShadow,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &CascadedShadowImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_HDRMipStorageImages,
                .dstArrayElement = 0,
                .descriptorCount = Renderer->HDRRenderTarget->MipCount,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = HDRMipStorageImageInfos,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_BloomMipStorageImages,
                .dstArrayElement = 0,
                .descriptorCount = Renderer->BloomTarget->MipCount,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = BloomMipStorageImageInfos,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_HDRColorImageGeneral,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &HDRColorImageGeneralInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_BloomImageGeneral,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &BloomImageGeneralInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_PointShadows,
                .dstArrayElement = 0,
                .descriptorCount = R_MaxShadowCount,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = PointShadowInfos,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_JointBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &JointBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_ParticleBuffer,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &ParticleBufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_ParticleTexture,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &ParticleImageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = Binding_PerFrame_TextureUI,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &UIImageInfo,
            },
        };
        static_assert(CountOf(Writes) == Binding_PerFrame_Count);

        void* Mapping = Renderer->PerFrameResourceDescriptorMappings[Frame->FrameID];
        VkDeviceAddress ResourceDescriptorsDeviceAddress = GetBufferDeviceAddress(VK.Device, Renderer->PerFrameResourceDescriptorBuffers[Frame->FrameID]);

        // TODO(boti): Keep track of the maximum used address (max(CurrentOffset + DescriptorSize))
        // to allow us to use the rest of the buffer for the "push" or temporary descriptors
        for (u32 WriteIndex = 0; WriteIndex < CountOf(Writes); WriteIndex++)
        {
            VkWriteDescriptorSet* Write = Writes + WriteIndex;

            VkDeviceSize BaseOffset = 0;
            vkGetDescriptorSetLayoutBindingOffsetEXT(VK.Device, Renderer->SetLayouts[SetLayout_PerFrame], Write->dstBinding, &BaseOffset);
            
            for (u32 Element = 0; Element < Write->descriptorCount; Element++)
            {
                u32 EffectiveElement = Element + Write->dstArrayElement;

                const void* Data = nullptr;
                umm DescriptorSize = 0;
            
                VkDescriptorAddressInfoEXT BufferInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
                switch (Write->descriptorType)
                {
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize; goto IMAGE_CASE;
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: DescriptorSize = VK.DescriptorBufferProps.storageImageDescriptorSize; goto IMAGE_CASE;
                    IMAGE_CASE:
                    {
                        Data = Write->pImageInfo + Element;
                    } break;
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: DescriptorSize = VK.DescriptorBufferProps.storageBufferDescriptorSize; goto BUFFER_CASE;
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: DescriptorSize = VK.DescriptorBufferProps.uniformBufferDescriptorSize; goto BUFFER_CASE;
                    BUFFER_CASE:
                    {
                        BufferInfo.address = GetBufferDeviceAddress(VK.Device, Write->pBufferInfo[Element].buffer) + Write->pBufferInfo[Element].offset;
                        BufferInfo.range = Write->pBufferInfo[Element].range;
                        Data = &BufferInfo;
                    } break;
                    InvalidDefaultCase;
                }

                // NOTE(boti): descriptors live in their own dedicated buffer starting at offset 0 for now, but may not in the future
                VkDeviceSize Offset = 0 + BaseOffset + EffectiveElement*DescriptorSize;

                VkDescriptorGetInfoEXT DescriptorInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                    .pNext = nullptr,
                    .type = Write->descriptorType,
                    .data = { (VkSampler*)Data }, // HACK(boti): This is just a union of pointers, there really should be a void* in there...
                };
                vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Mapping, Offset));
            }
        }
    }

    auto BindSystemDescriptors = [=](VkCommandBuffer CmdBuffer)
    {
        VkDeviceAddress PerFrameResourceBufferAddress = GetBufferDeviceAddress(VK.Device, Renderer->PerFrameResourceDescriptorBuffers[Frame->FrameID]);
        VkDeviceAddress SamplerBufferAddress = GetBufferDeviceAddress(VK.Device, Renderer->PerFrameSamplerDescriptorBuffers[Frame->FrameID]);
        VkDescriptorBufferBindingInfoEXT DescriptorBufferBindings[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                .pNext = nullptr,
                .address = PerFrameResourceBufferAddress,
                .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT 
            },
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                .pNext = nullptr,
                .address = SamplerBufferAddress,
                .usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT 
            },
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                .pNext = nullptr,
                .address = Renderer->TextureManager.DescriptorDeviceAddress,
                .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT 
            },
        };
        vkCmdBindDescriptorBuffersEXT(CmdBuffer, CountOf(DescriptorBufferBindings), DescriptorBufferBindings);

        u32 DescriptorBufferIndices[SystemSet_Count] = { 0, 1, 2 };
        VkDeviceSize DescriptorBufferOffsets[SystemSet_Count] = { 0, 0, 0 };
        vkCmdSetDescriptorBufferOffsetsEXT(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Renderer->SystemPipelineLayout,
                                           0, CountOf(DescriptorBufferBindings), DescriptorBufferIndices, DescriptorBufferOffsets);
        vkCmdSetDescriptorBufferOffsetsEXT(CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Renderer->SystemPipelineLayout,
                                           0, CountOf(DescriptorBufferBindings), DescriptorBufferIndices, DescriptorBufferOffsets);
    };

    BindSystemDescriptors(PrepassCmd);

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

    //
    // Skinning
    //
    vkCmdBeginDebugUtilsLabelEXT(PrepassCmd, "Skinning");
    {
        pipeline_with_layout SkinningPipeline = Renderer->Pipelines[Pipeline_Skinning];
        vkCmdBindPipeline(PrepassCmd, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Pipeline);

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
                .buffer = Frame->Renderer->SkinningBuffer,
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
            u32 PushConstants[4] = 
            {
                Cmd->SrcVertexOffset,
                Cmd->DstVertexOffset,
                Cmd->VertexCount,
                (u32)(Cmd->PoseOffset / sizeof(m4)),
            };
            vkCmdPushConstants(PrepassCmd, SkinningPipeline.Layout, VK_SHADER_STAGE_ALL,
                               0, sizeof(PushConstants), PushConstants);

            vkCmdDispatch(PrepassCmd, CeilDiv(Cmd->VertexCount, Skin_GroupSizeX), 1, 1);
        }

        VkBufferMemoryBarrier2 EndBarriers[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_2_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Frame->Renderer->SkinningBuffer,
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
    vkCmdEndDebugUtilsLabelEXT(PrepassCmd);

    vkCmdSetViewport(PrepassCmd, 0, 1, &FrameViewport);
    vkCmdSetScissor(PrepassCmd, 0, 1, &FrameScissor);
    BeginPrepass(Frame, PrepassCmd);
    {
        pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Prepass];
        vkCmdBindPipeline(PrepassCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
        DrawList(Frame, PrepassCmd, &PrimaryDrawList);
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
    BindSystemDescriptors(PreLightCmd);

    //
    // SSAO
    //
    vkCmdBeginDebugUtilsLabelEXT(PreLightCmd, "SSAO");
    {
        // Occlusion
        {
            VkImageMemoryBarrier2 BeginBarriers[] = 
            {
                // Structure buffer
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK.GraphicsQueueIdx,
                    .dstQueueFamilyIndex = VK.ComputeQueueIdx,
                    .image = Frame->Renderer->StructureBuffer->Image,
                    .subresourceRange = 
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                },

                // Raw occlusion image
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                    .srcAccessMask = VK_ACCESS_2_NONE,
                    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .srcQueueFamilyIndex = VK.GraphicsQueueIdx,
                    .dstQueueFamilyIndex = VK.ComputeQueueIdx,
                    .image = Renderer->OcclusionBuffers[0]->Image,
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
            VkDependencyInfo BeginDependency = 
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
            vkCmdPipelineBarrier2(PreLightCmd, &BeginDependency);

            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_SSAO];
            vkCmdBindPipeline(PreLightCmd, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline.Pipeline);
            
            u32 DispatchX = CeilDiv(Frame->RenderWidth, SSAO_GroupSizeX);
            u32 DispatchY = CeilDiv(Frame->RenderHeight, SSAO_GroupSizeY);
            vkCmdDispatch(PreLightCmd, DispatchX, DispatchY, 1);
        }

        // Blur/filter
        {
            VkImageMemoryBarrier2 BlurBarriers[] = 
            {
                // Raw SSAO
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = Frame->Renderer->OcclusionBuffers[0]->Image,
                    .subresourceRange = 
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                },
                // Filtered SSAO
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext = nullptr,
                    .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                    .srcAccessMask = VK_ACCESS_2_NONE,
                    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .srcQueueFamilyIndex = VK.GraphicsQueueIdx,
                    .dstQueueFamilyIndex = VK.ComputeQueueIdx,
                    .image = Frame->Renderer->OcclusionBuffers[1]->Image,
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

            VkDependencyInfo BlurDependency = 
            {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .imageMemoryBarrierCount = CountOf(BlurBarriers),
                .pImageMemoryBarriers = BlurBarriers,
            };
            vkCmdPipelineBarrier2(PreLightCmd, &BlurDependency);
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_SSAOBlur];
            vkCmdBindPipeline(PreLightCmd, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline.Pipeline);

            u32 BlurDispatchX = CeilDiv(Frame->RenderWidth, SSAOBlur_GroupSizeX);
            u32 BlurDispatchY = CeilDiv(Frame->RenderHeight, SSAOBlur_GroupSizeY);
            vkCmdDispatch(PreLightCmd, BlurDispatchX, BlurDispatchY, 1);
        }
    }
    vkCmdEndDebugUtilsLabelEXT(PreLightCmd);

    //
    // Light binning
    //
    {
        vkCmdBeginDebugUtilsLabelEXT(PreLightCmd, "Light binning");

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
        vkCmdDispatch(PreLightCmd, CeilDiv(TileCountX, LightBin_GroupSizeX), CeilDiv(TileCountY, LightBin_GroupSizeY), 1);

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
        vkCmdEndDebugUtilsLabelEXT(PreLightCmd);
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
    BindSystemDescriptors(ShadowCmd);

    //
    // Cascaded shadows
    //
    {
        vkCmdBeginDebugUtilsLabelEXT(ShadowCmd, "CSM");

        pipeline_with_layout ShadowPipeline = Renderer->Pipelines[Pipeline_Shadow];
        vkCmdBindPipeline(ShadowCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowPipeline.Pipeline);

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
        for (u32 CascadeIndex = 0; CascadeIndex < R_MaxShadowCascadeCount; CascadeIndex++)
        {
            BeginCascade(Frame, ShadowCmd, CascadeIndex);

            vkCmdPushConstants(ShadowCmd, ShadowPipeline.Layout,
                               VK_SHADER_STAGE_ALL,
                               0, sizeof(u32), &CascadeIndex);
            DrawList(Frame, ShadowCmd, CascadeDrawLists + CascadeIndex);

            EndCascade(Frame, ShadowCmd);
        }

        vkCmdEndDebugUtilsLabelEXT(ShadowCmd);
    }

    //
    // Point shadows
    //
    {
        vkCmdBeginDebugUtilsLabelEXT(ShadowCmd, "Point shadows");

        VkImageMemoryBarrier2* Barriers = PushArray(Frame->Arena, 0, VkImageMemoryBarrier2, Frame->MaxShadowCount);
        for (u32 ShadowIndex = 0; ShadowIndex < Frame->ShadowCount; ShadowIndex++)
        {
            point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowIndex;
            Barriers[ShadowIndex] = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = ShadowMap->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 6,
                },
            };
        }

        VkDependencyInfo BeginShadow = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = Frame->ShadowCount,
            .pImageMemoryBarriers = Barriers,
        };
        vkCmdPipelineBarrier2(ShadowCmd, &BeginShadow);

        pipeline_with_layout ShadowPipeline = Renderer->Pipelines[Pipeline_ShadowAny];
        vkCmdBindPipeline(ShadowCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowPipeline.Pipeline);

        VkViewport ShadowViewport = 
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = (f32)R_PointShadowResolution,
            .height = (f32)R_PointShadowResolution,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        VkRect2D ShadowScissor = 
        {
            .offset = { 0, 0 },
            .extent = { R_PointShadowResolution, R_PointShadowResolution },
        };

        vkCmdSetViewport(ShadowCmd, 0, 1, &ShadowViewport);
        vkCmdSetScissor(ShadowCmd, 0, 1, &ShadowScissor);
        for (u32 ShadowIndex = 0; ShadowIndex < Frame->ShadowCount; ShadowIndex++)
        {
            u32 LightIndex = Frame->Shadows[ShadowIndex];
            light* Light = Frame->Lights + LightIndex;
            point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowIndex;

            for (u32 LayerIndex = 0; LayerIndex < Layer_Count; LayerIndex++)
            {
                VkRenderingAttachmentInfo DepthAttachment = 
                {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .pNext = nullptr,
                    .imageView = ShadowMap->LayerViews[LayerIndex],
                    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .resolveImageView = VK_NULL_HANDLE,
                    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = { .depthStencil = { 1.0f, 0 } },
                };
                VkRenderingInfo ShadowRendering = 
                {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .renderArea = { { 0, 0 }, { R_PointShadowResolution, R_PointShadowResolution } },
                    .layerCount = 1,
                    .viewMask = 0,
                    .colorAttachmentCount = 0,
                    .pColorAttachments = nullptr,
                    .pDepthAttachment = &DepthAttachment,
                    .pStencilAttachment = nullptr,
                };
                vkCmdBeginRendering(ShadowCmd, &ShadowRendering);

                u32 Index = 6*ShadowIndex + LayerIndex;
                vkCmdPushConstants(ShadowCmd, ShadowPipeline.Layout, VK_SHADER_STAGE_ALL,
                                   0, sizeof(Index), &Index);
                DrawList(Frame, ShadowCmd, ShadowDrawLists + Index);

                vkCmdEndRendering(ShadowCmd);
            }
        }

        for (u32 ShadowIndex = 0; ShadowIndex < Frame->ShadowCount; ShadowIndex++)
        {
            point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowIndex;
            Barriers[ShadowIndex] = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = ShadowMap->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 6,
                },
            };
        }
        for (u32 ShadowIndex = Frame->ShadowCount; ShadowIndex < Frame->MaxShadowCount; ShadowIndex++)
        {
            point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowIndex;
            Barriers[ShadowIndex] = 
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = ShadowMap->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 6,
                },
            };
        }

        VkDependencyInfo EndShadow = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = Frame->MaxShadowCount,
            .pImageMemoryBarriers = Barriers,
        };
        vkCmdPipelineBarrier2(ShadowCmd, &EndShadow);

        vkCmdEndDebugUtilsLabelEXT(ShadowCmd);
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
    BindSystemDescriptors(RenderCmd);

    vkCmdSetViewport(RenderCmd, 0, 1, &FrameViewport);
    vkCmdSetScissor(RenderCmd, 0, 1, &FrameScissor);

    //
    // Shading
    //

    VkRenderingAttachmentInfo ShadingColorAttachments[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Renderer->HDRRenderTarget->View,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {},
        },
    };
    
    VkRenderingAttachmentInfo ShadingDepthAttachment = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = Renderer->DepthBuffer->View,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {},
    };
    VkRenderingInfo ShadingRenderingInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = { { 0, 0 }, { Frame->RenderWidth, Frame->RenderHeight } },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = CountOf(ShadingColorAttachments),
        .pColorAttachments = ShadingColorAttachments,
        .pDepthAttachment = &ShadingDepthAttachment,
        .pStencilAttachment = nullptr,
    };

    vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "Opaque shading");
    {
        pipeline PipelineID = Pipeline_None;
        VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        VkImageLayout LayoutForShading = VK_IMAGE_LAYOUT_UNDEFINED;
        switch (Frame->Config.ShadingMode)
        {
            case ShadingMode_Visibility:
            {
                PipelineID = Pipeline_ShadingVisibility;
                BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
                LayoutForShading = VK_IMAGE_LAYOUT_GENERAL;
            } break;
            case ShadingMode_Forward:
            {
                PipelineID = Pipeline_ShadingForward;
                BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                LayoutForShading = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
            } break;
            InvalidDefaultCase;
        }

        VkImageMemoryBarrier2 BeginBarriers[] = 
        {
            // HDR Color
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = LayoutForShading,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Renderer->HDRRenderTarget->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },

            // Visibility buffer
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Renderer->VisibilityBuffer->Image,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },

            // Shadow cascades
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->Renderer->ShadowMap,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = Frame->Renderer->ShadowCascadeCount,
                },
            },

            // SSAO
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK.ComputeQueueIdx,
                .dstQueueFamilyIndex = VK.GraphicsQueueIdx,
                .image = Frame->Renderer->OcclusionBuffers[1]->Image,
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

        VkDependencyInfo BeginDependency = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = CountOf(BeginBarriers),
            .pImageMemoryBarriers = BeginBarriers,
        };

        vkCmdPipelineBarrier2(RenderCmd, &BeginDependency);

        pipeline_with_layout Pipeline = Renderer->Pipelines[PipelineID];
        vkCmdBindPipeline(RenderCmd, BindPoint, Pipeline.Pipeline);

        switch (Frame->Config.ShadingMode)
        {
            case ShadingMode_Visibility:
            {
                u32 DispatchX = CeilDiv(Frame->RenderWidth, ShadingVisibility_GroupSizeX);
                u32 DispatchY = CeilDiv(Frame->RenderHeight, ShadingVisibility_GroupSizeY);
                vkCmdDispatch(RenderCmd, DispatchX, DispatchY, 1);

                VkImageMemoryBarrier2 EndBarriers[] = 
                {
                    {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image = Renderer->HDRRenderTarget->Image,
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
                VkDependencyInfo EndDependency = 
                {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext = nullptr,
                    .dependencyFlags = 0,
                    .imageMemoryBarrierCount = CountOf(EndBarriers),
                    .pImageMemoryBarriers = EndBarriers,
                };
                vkCmdPipelineBarrier2(RenderCmd, &EndDependency);

                vkCmdBeginRendering(RenderCmd, &ShadingRenderingInfo);
            } break;
            case ShadingMode_Forward:
            {
                vkCmdBeginRendering(RenderCmd, &ShadingRenderingInfo);
                DrawList(Frame, RenderCmd, &PrimaryDrawList);
            } break;
            InvalidDefaultCase;
        }
    }
    vkCmdEndDebugUtilsLabelEXT(RenderCmd);

    // Render sky
    vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "Sky");
    {
        pipeline_with_layout SkyPipeline = Renderer->Pipelines[Pipeline_Sky];
        vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Pipeline);
        vkCmdDraw(RenderCmd, 3, 1, 0, 0);
    }
    vkCmdEndDebugUtilsLabelEXT(RenderCmd);

    // Particles
    vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "Particle");
    {
        pipeline_with_layout ParticlePipeline = Renderer->Pipelines[Pipeline_Quad];
        vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ParticlePipeline.Pipeline);

        for (u32 CmdIndex = 0; CmdIndex < Frame->ParticleDrawCmdCount; CmdIndex++)
        {
            particle_cmd* Cmd = Frame->ParticleDrawCmds + CmdIndex;
            u32 VertexCount = 6 * Cmd->ParticleCount;
            u32 FirstVertex = 6 * Cmd->FirstParticle;
            vkCmdPushConstants(RenderCmd, ParticlePipeline.Layout, VK_SHADER_STAGE_ALL, 
                               0, sizeof(Cmd->Mode), &Cmd->Mode);
            vkCmdDraw(RenderCmd, VertexCount, 1, FirstVertex, 0);
        }
    }
    vkCmdEndDebugUtilsLabelEXT(RenderCmd);

    vkCmdEndRendering(RenderCmd);

    RenderBloom(Frame, RenderCmd,
                Frame->Renderer->HDRRenderTarget,
                Frame->Renderer->BloomTarget,
                Renderer->Pipelines[Pipeline_BloomDownsample].Layout,
                Renderer->Pipelines[Pipeline_BloomDownsample].Pipeline,
                Renderer->Pipelines[Pipeline_BloomUpsample].Layout,
                Renderer->Pipelines[Pipeline_BloomUpsample].Pipeline);

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
            .imageView = Frame->Renderer->DepthBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .depthStencil = { 0.0f, 0 } },
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
        vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "Blit");
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Blit];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdDraw(RenderCmd, 3, 1, 0, 0);
        }
        vkCmdEndDebugUtilsLabelEXT(RenderCmd);

        vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "UI");

        // 3D widget render
        {
            const VkDeviceSize ZeroOffset = 0;
            vkCmdBindVertexBuffers(RenderCmd, 0, 1, &Renderer->GeometryBuffer.VertexMemory.Buffer, &ZeroOffset);
            vkCmdBindIndexBuffer(RenderCmd, Renderer->GeometryBuffer.IndexMemory.Buffer, ZeroOffset, VK_INDEX_TYPE_UINT32);

            pipeline_with_layout GizmoPipeline = Renderer->Pipelines[Pipeline_Gizmo];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Pipeline);

            for (u32 CmdIndex = 0; CmdIndex < Frame->DrawWidget3DCmdCount; CmdIndex++)
            {
                draw_widget3d_cmd* Cmd = Frame->DrawWidget3DCmds + CmdIndex;

                vkCmdPushConstants(RenderCmd, GizmoPipeline.Layout, VK_SHADER_STAGE_ALL,
                                   0, sizeof(Cmd->Transform), &Cmd->Transform);
                vkCmdPushConstants(RenderCmd, GizmoPipeline.Layout, VK_SHADER_STAGE_ALL,
                                   sizeof(Cmd->Transform), sizeof(Cmd->Color), &Cmd->Color);
                vkCmdDrawIndexed(RenderCmd, Cmd->Base.IndexCount, Cmd->Base.InstanceCount, Cmd->Base.IndexOffset, Cmd->Base.VertexOffset, Cmd->Base.InstanceCount);
            }
        }

        // 2D UI render
        {
            VkDeviceSize ZeroOffset = 0;
            vkCmdBindVertexBuffers(RenderCmd, 0, 1, &Frame->Backend->Vertex2DBuffer, &ZeroOffset);

            pipeline_with_layout UIPipeline = Renderer->Pipelines[Pipeline_UI];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, UIPipeline.Pipeline);

            m4 OrthoTransform = M4(2.0f / Frame->RenderWidth, 0.0f, 0.0f, -1.0f,
                                   0.0f, 2.0f / Frame->RenderHeight, 0.0f, -1.0f,
                                   0.0f, 0.0f, 1.0f, 0.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f);
            vkCmdPushConstants(RenderCmd, UIPipeline.Layout, VK_SHADER_STAGE_ALL, 
                               0, sizeof(OrthoTransform), &OrthoTransform);

            vkCmdDraw(RenderCmd, Frame->Vertex2DCount, 1, 0, 0);
        }

        vkCmdEndDebugUtilsLabelEXT(RenderCmd);

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
                Result = true;
            }
            return(Result);
        };

        AddEntry("BAR", Renderer->BARMemory.MemoryAt, Renderer->BARMemory.Size);
        AddEntry("RenderTarget", Renderer->RenderTargetHeap.Offset, Renderer->RenderTargetHeap.MemorySize);
        AddEntry("VertexBuffer", 
                 Renderer->GeometryBuffer.VertexMemory.CountInUse * Renderer->GeometryBuffer.VertexMemory.Stride, 
                 Renderer->GeometryBuffer.VertexMemory.MaxCount * Renderer->GeometryBuffer.VertexMemory.Stride);
        AddEntry("IndexBuffer", 
                 Renderer->GeometryBuffer.IndexMemory.CountInUse * Renderer->GeometryBuffer.IndexMemory.Stride, 
                 Renderer->GeometryBuffer.IndexMemory.MaxCount * Renderer->GeometryBuffer.IndexMemory.Stride);
        AddEntry("Texture", Renderer->TextureManager.MemoryOffset, Renderer->TextureManager.MemorySize);
        AddEntry("Shadow", Renderer->ShadowMemoryOffset, Renderer->ShadowMemorySize);
        AddEntry("Staging", Frame->StagingBufferAt, Frame->StagingBufferSize);
        AddEntry("LightBuffer", Frame->LightCount * sizeof(light), Renderer->LightBufferMemorySize);
        AddEntry("TileBuffer", TileCountX * TileCountY * sizeof(screen_tile), Renderer->TileMemorySize);
    }
}

extern "C" Signature_AllocateTexture(AllocateTexture)
{
    renderer_texture_id Result = AllocateTexture(&Renderer->TextureManager, Flags, Info, Placeholder);
    return(Result);
}

internal void SetupSceneRendering(render_frame* Frame, frustum* CascadeFrustums)
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
        }

        m4 SunTransfrom = M4(
            SunX.X, SunY.X, SunZ.X, 0.0f,
            SunX.Y, SunY.Y, SunZ.Y, 0.0f,
            SunX.Z, SunY.Z, SunZ.Z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
        m4 SunView = M4(
            SunX.X, SunX.Y, SunX.Z, 0.0f,
            SunY.X, SunY.Y, SunY.Z, 0.0f,
            SunZ.X, SunZ.Y, SunZ.Z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
        m4 CameraToSun = SunView * Frame->CameraTransform;
    
        f32 SplitFactor = 0.25f;
        f32 Splits[] = { 4.0f, 8.0f, 16.0f, 30.0f };
        f32 NdTable[] = 
        { 
            0.0f,
            (1.0f - SplitFactor) * Splits[0],
            (1.0f - SplitFactor) * Splits[1],
            (1.0f - SplitFactor) * Splits[2],
        };
        f32 FdTable[] = 
        { 
            (1.0f + SplitFactor) * Splits[0], 
            (1.0f + SplitFactor) * Splits[1],
            (1.0f + SplitFactor) * Splits[2],
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

        f32 MinZ0;
        f32 MaxZ0;
        v3 CascadeP0;
        f32 CascadeScale0;
        for (u32 CascadeIndex = 0; CascadeIndex < R_MaxShadowCascadeCount; CascadeIndex++)
        {
            f32 Nd = NdTable[CascadeIndex];
            f32 Fd = FdTable[CascadeIndex];

            f32 NdOverG = Nd / g;
            f32 FdOverG = Fd / g;

            v3 CascadeBoxP[8] = 
            {
                // Near points in camera space
                { +NdOverG * s, +NdOverG, Nd },
                { +NdOverG * s, -NdOverG, Nd },
                { -NdOverG * s, -NdOverG, Nd },
                { -NdOverG * s, +NdOverG, Nd },

                // Far points in camera space
                { +FdOverG * s, +FdOverG, Fd },
                { +FdOverG * s, -FdOverG, Fd },
                { -FdOverG * s, -FdOverG, Fd },
                { -FdOverG * s, +FdOverG, Fd },
            };

            f32 CascadeScale = Ceil(Max(VectorLength(CascadeBoxP[0] - CascadeBoxP[6]),
                                        VectorLength(CascadeBoxP[4] - CascadeBoxP[6])));

            v3 CascadeBoxMin = { +FLT_MAX, +FLT_MAX, +FLT_MAX };
            v3 CascadeBoxMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

            for (u32 i = 0; i < 8; i++)
            {
                v3 P = TransformPoint(CameraToSun, CascadeBoxP[i]);
                CascadeBoxMin = 
                {
                    Min(P.X, CascadeBoxMin.X),
                    Min(P.Y, CascadeBoxMin.Y),
                    Min(P.Z, CascadeBoxMin.Z),
                };
                CascadeBoxMax = 
                {
                    Max(P.X, CascadeBoxMax.X),
                    Max(P.Y, CascadeBoxMax.Y),
                    Max(P.Z, CascadeBoxMax.Z),
                };
            }

            f32 TexelSize = CascadeScale / (f32)R_ShadowResolution;

            v3 CascadeP = 
            {
                TexelSize * Floor((CascadeBoxMax.X + CascadeBoxMin.X) / (2.0f * TexelSize)),
                TexelSize * Floor((CascadeBoxMax.Y + CascadeBoxMin.Y) / (2.0f * TexelSize)),
                CascadeBoxMin.Z,
            };

            m4 CascadeView = M4(SunX.X, SunX.Y, SunX.Z, -CascadeP.X,
                                SunY.X, SunY.Y, SunY.Z, -CascadeP.Y,
                                SunZ.X, SunZ.Y, SunZ.Z, -CascadeP.Z,
                                0.0f, 0.0f, 0.0f, 1.0f);
            m4 CascadeProjection = M4(2.0f / CascadeScale, 0.0f, 0.0f, 0.0f,
                                      0.0f, 2.0f / CascadeScale, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f / (CascadeBoxMax.Z - CascadeBoxMin.Z), 0.0f,
                                      0.0f, 0.0f, 0.0f, 1.0f);
            m4 InverseCascadeView = AffineOrthonormalInverse(CascadeView);
            m4 InverseCascadeProjection = M4(CascadeScale / 2.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, CascadeScale / 2.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, (CascadeBoxMax.Z - CascadeBoxMin.Z) / 1.0f, 0.0f,
                                             0.0f, 0.0f, 0.0f, 1.0f);

            m4 CascadeViewProjection = CascadeProjection * CascadeView;
            Frame->Uniforms.CascadeViewProjections[CascadeIndex] = CascadeViewProjection;
            Frame->Uniforms.CascadeMinDistances[CascadeIndex] = Nd;
            Frame->Uniforms.CascadeMaxDistances[CascadeIndex] = Fd;

            m4 InverseCascadeViewProjection = InverseCascadeView * InverseCascadeProjection;
            frustum ClipSpaceFrustum = GetClipSpaceFrustum();
            ClipSpaceFrustum.Near = {};
            for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
            {
                CascadeFrustums[CascadeIndex].Planes[PlaneIndex] = ClipSpaceFrustum.Planes[PlaneIndex] * CascadeProjection * CascadeView;
            }

            if (CascadeIndex == 0)
            {
                CascadeScale0 = CascadeScale;
                CascadeP0 = CascadeP;
                MinZ0 = CascadeBoxMin.Z;
                MaxZ0 = CascadeBoxMax.Z;
            }
            else
            {
                Frame->Uniforms.CascadeScales[CascadeIndex - 1] = 
                {
                    CascadeScale0 / CascadeScale,
                    CascadeScale0 / CascadeScale,
                    (MaxZ0 - MinZ0) / (CascadeBoxMax.Z - CascadeBoxMin.Z),
                };
                Frame->Uniforms.CascadeOffsets[CascadeIndex - 1] = 
                {
                    ((CascadeP0.X - CascadeP.X) / CascadeScale) - (CascadeScale0 / (2.0f * CascadeScale)) + 0.5f,
                    ((CascadeP0.Y - CascadeP.Y) / CascadeScale) - (CascadeScale0 / (2.0f * CascadeScale)) + 0.5f,
                    (CascadeP0.Z - CascadeP.Z) / (CascadeBoxMax.Z - CascadeBoxMin.Z),
                };
            }
        }
    }

    // Upload uniform data
    memcpy(Frame->UniformData, &Frame->Uniforms, sizeof(Frame->Uniforms));
}