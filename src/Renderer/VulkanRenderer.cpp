#include "VulkanRenderer.hpp"

vulkan VK;
platform_api Platform;

#include "profiler.cpp"

#include "RenderDevice.cpp"
#include "Geometry.cpp"
#include "RenderTarget.cpp"
#include "TextureManager.cpp"
#include "Pipelines.cpp"
#include "rhi_vulkan.cpp"

internal VkResult 
ResizeSwapchain(renderer* Renderer, u32 FrameID, b32 Forced);

internal VkResult 
CreatePipelines(renderer* Renderer);

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

internal VkResult
CreateDedicatedImage(const VkImageCreateInfo* Info, u32 MemoryTypes, VkImage* pImage, VkDeviceMemory* pMemory)
{
    VkResult Result = VK_SUCCESS;

    VkMemoryRequirements MemoryRequirements = GetImageMemoryRequirements(VK.Device, Info, VK_IMAGE_ASPECT_COLOR_BIT);
    u32 MemoryTypeIndex;
    if (BitScanForward(&MemoryTypeIndex, MemoryTypes & MemoryRequirements.memoryTypeBits))
    {
        VkImage Image = VK_NULL_HANDLE;
        if ((Result = vkCreateImage(VK.Device, Info, nullptr, &Image)) == VK_SUCCESS)
        {
            VkMemoryAllocateInfo AllocInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = MemoryRequirements.size,
                .memoryTypeIndex = MemoryTypeIndex,
            };

            VkDeviceMemory Memory = VK_NULL_HANDLE;
            if ((Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Memory)) == VK_SUCCESS)
            {
                if ((Result = vkBindImageMemory(VK.Device, Image, Memory, 0)) == VK_SUCCESS)
                {
                    *pImage = Image;
                    *pMemory = Memory;
                    Image = VK_NULL_HANDLE;
                    Memory = VK_NULL_HANDLE;
                }
                vkFreeMemory(VK.Device, Memory, nullptr);
            }

            vkDestroyImage(VK.Device, Image, nullptr);
        }
    }
    else
    {
        Result = VK_ERROR_UNKNOWN;
    }

    return(Result);
}

internal gpu_memory_arena 
CreateGPUArena(umm Size, u32 MemoryTypeIndex, gpu_memory_arena_flags Flags)
{
    gpu_memory_arena Result = {};

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
    VkResult ErrorCode = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Memory);
    if (ErrorCode == VK_SUCCESS)
    {
        b32 MapSuccessful = true;
        if (Flags & GpuMemoryFlag_Mapped)
        {
            ErrorCode = vkMapMemory(VK.Device, Memory, 0, VK_WHOLE_SIZE, 0, &Result.Mapping);
            MapSuccessful = (ErrorCode == VK_SUCCESS);
        }

        if (MapSuccessful)
        {
            Result.Memory = Memory;
            Result.Size = Size;
            Result.MemoryAt = 0;
            Result.MemoryTypeIndex = MemoryTypeIndex;
            Memory = VK_NULL_HANDLE;
        }
        else
        {
            UnhandledError("Failed to map memory for memory mapped GPU arena");
        }
        vkFreeMemory(VK.Device, Memory, nullptr);
    }
    else
    {
        UnhandledError("Failed to allocate memory for GPU arena");
    }

    return(Result);
}

internal void ResetGPUArena(gpu_memory_arena* Arena)
{
    Arena->MemoryAt = 0;
}

internal b32 
PushImage(gpu_memory_arena* Arena, VkImage Image)
{
    b32 Result = false;

    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(VK.Device, Image, &MemoryRequirements);
    Assert(MemoryRequirements.memoryTypeBits & (1u << Arena->MemoryTypeIndex));

    umm EffectiveOffset = Align(Arena->MemoryAt, MemoryRequirements.alignment);
    if ((EffectiveOffset + MemoryRequirements.size) <= Arena->Size)
    {
        VkResult ErrorCode = vkBindImageMemory(VK.Device, Image, Arena->Memory, EffectiveOffset);
        if (ErrorCode == VK_SUCCESS)
        {
            Arena->MemoryAt = EffectiveOffset + MemoryRequirements.size;
            Result = true;
        }
        else
        {
            UnhandledError("Failed to bind image to memory");
        }
    }
    else
    {
        //UnhandledError("GPU arena full");
    }

    return(Result);
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
        .usage = Usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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
        if (Pipeline->Layout != Renderer->SystemPipelineLayout)
        {
            vkDestroyPipelineLayout(VK.Device, Pipeline->Layout, nullptr);
        }

        const pipeline_info* Info = PipelineInfos + PipelineIndex;
        
        if (Info->Layout.DescriptorSetCount)
        {
            VkDescriptorSetLayout SetLayouts[MaxDescriptorSetCount] = {};
            for (u32 SystemSetIndex = 0; SystemSetIndex < Set_Count; SystemSetIndex++)
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
                .setLayoutCount = Set_Count + Info->Layout.DescriptorSetCount,
                .pSetLayouts = SetLayouts,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &PushConstantRange,
            };
            Result = vkCreatePipelineLayout(VK.Device, &PipelineLayoutInfo, nullptr, &Pipeline->Layout);
            Verify(Result == VK_SUCCESS);
            if (Result != VK_SUCCESS) return(Result);
        }
        else
        {
            Pipeline->Layout = Renderer->SystemPipelineLayout;
        }

        // NOTE(boti): The shader name is always used from the current pipeline (not the parent)
        u64 NameSize = PathSize;
        CopyZStringToBuffer(Name, Info->Name, &NameSize);
        char* Extension = Path + (MaxPathSize - NameSize);
        
        pipeline_info VariantInfo = {};
        if (Info->Inheritance.ParentID)
        {
            const pipeline_info* ParentInfo = PipelineInfos + Info->Inheritance.ParentID;
            Assert(ParentInfo->Type == Info->Type);
            VariantInfo = *ParentInfo;
            if (Info->Inheritance.OverrideProps & PipelineProp_Cull)
            {
                VariantInfo.RasterizerState.CullFlags = Info->RasterizerState.CullFlags;
            }
            if (Info->Inheritance.OverrideProps & PipelineProp_DepthClamp)
            {
                VariantInfo.RasterizerState.Flags &= ~RS_DepthClampEnable;
                VariantInfo.RasterizerState.Flags |= (Info->RasterizerState.Flags & RS_DepthClampEnable);
            }
            if (Info->Inheritance.OverrideProps & PipelineProp_DepthBias)
            {
                VariantInfo.RasterizerState.Flags &= ~RS_DepthBiasEnable;
                VariantInfo.RasterizerState.Flags |= (Info->RasterizerState.Flags & RS_DepthBiasEnable);
                VariantInfo.RasterizerState.DepthBiasConstantFactor = Info->RasterizerState.DepthBiasConstantFactor;
                VariantInfo.RasterizerState.DepthBiasSlopeFactor = Info->RasterizerState.DepthBiasSlopeFactor;
                VariantInfo.RasterizerState.DepthBiasClamp = Info->RasterizerState.DepthBiasClamp;
            }
            Info = &VariantInfo;
        }

        switch (Info->Type)
        {
            case PipelineType_Compute:
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
                    
                    if (Result == VK_SUCCESS)
                    {
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
                        vkDestroyShaderModule(VK.Device, Module, nullptr);
                    }
                }
                else
                {
                    UnhandledError("Failed to load shader file");
                    Result = VK_ERROR_UNKNOWN;
                }
            } break;
            case PipelineType_Graphics:
            {
                Assert(!HasFlag(Info->EnabledStages, ShaderStage_CS));
                // Only VS and PS is supported for now
                if (Info->EnabledStages & ~(ShaderStage_VS|ShaderStage_PS)) 
                {
                    UnimplementedCodePath;
                }
                if (HasFlag(Info->DepthStencilState.Flags, DS_StencilTestEnable))
                {
                    UnimplementedCodePath;
                }
    
                VkPipelineShaderStageCreateInfo StageInfos[ShaderStage_Count] = {};
                u32 StageCount = 0;
    
                if (Info->EnabledStages & ShaderStage_VS)
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
                if (Info->EnabledStages & ShaderStage_PS)
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
    
                VkPipelineVertexInputStateCreateInfo VertexInputState = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .vertexBindingDescriptionCount = 0,
                    .pVertexBindingDescriptions = nullptr,
                    .vertexAttributeDescriptionCount = 0,
                    .pVertexAttributeDescriptions = nullptr,
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
                    ColorAttachmentFormats[ColorAttachmentIndex] = FormatTable[RenderTargetFormatTable[Info->ColorAttachments[ColorAttachmentIndex]]];
                }
    
                VkPipelineRenderingCreateInfo DynamicRendering = 
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .pNext = nullptr,
                    .viewMask = 0,
                    .colorAttachmentCount = Info->ColorAttachmentCount,
                    .pColorAttachmentFormats = ColorAttachmentFormats,
                    .depthAttachmentFormat = FormatTable[RenderTargetFormatTable[Info->DepthAttachment]],
                    .stencilAttachmentFormat = FormatTable[RenderTargetFormatTable[Info->StencilAttachment]],
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

                for (u32 StageIndex = 0; StageIndex < StageCount; StageIndex++)
                {
                    VkPipelineShaderStageCreateInfo* Stage = StageInfos + StageIndex;
                    vkDestroyShaderModule(VK.Device, Stage->module, nullptr);
                }
            } break;
            InvalidDefaultCase;
        }
        
        if (Result == VK_SUCCESS)
        {
            SetObjectName(VK.Device, Pipeline->Pipeline, Info->Name);
        }
        else
        {
            UnhandledError("Failed to create pipeline");
        }

        RestoreArena(Scratch, Checkpoint);
    }

    return(Result);
}

internal b32 
PushDeletionEntry(gpu_deletion_queue* Queue, u32 FrameID, vulkan_handle_type Type, void* Handle)
{
    b32 Result = false;
    if (Queue->EntryWriteAt - Queue->EntryReadAt < Queue->MaxEntryCount)
    {
        u32 Index = (Queue->EntryWriteAt++) % Queue->MaxEntryCount;
        Queue->Entries[Index] = { .Type = Type, .Handle = Handle };
        Queue->FrameEntryWriteAt[FrameID] = Queue->EntryWriteAt;
        Result = true;
    }
    else
    {
        UnhandledError("Texture deletion queue full");
    }
    return(Result);
}

internal void 
ProcessDeletionEntries(gpu_deletion_queue* Queue, u32 FrameID)
{
    for (; Queue->EntryReadAt < Queue->FrameEntryWriteAt[FrameID]; Queue->EntryReadAt++)
    {
        u32 Index = Queue->EntryReadAt % Queue->MaxEntryCount;
        deletion_entry* Entry = Queue->Entries + Index;
        switch (Entry->Type)
        {
            case VulkanHandle_Memory:
            {
                vkFreeMemory(VK.Device, Entry->Memory, nullptr);
            } break;
            case VulkanHandle_Buffer:
            {
                vkDestroyBuffer(VK.Device, Entry->Buffer, nullptr);
            } break;
            case VulkanHandle_Image:
            {
                vkDestroyImage(VK.Device, Entry->Image, nullptr);
            } break;
            case VulkanHandle_ImageView:
            {
                vkDestroyImageView(VK.Device, Entry->ImageView, nullptr);
            } break;
            case VulkanHandle_Swapchain:
            {
                vkDestroySwapchainKHR(VK.Device, Entry->Swapchain, nullptr);
            } break;
            InvalidDefaultCase;
        }
    }
    Queue->FrameEntryWriteAt[FrameID] = Queue->EntryWriteAt;
}

extern "C" Signature_CreateRenderer(CreateRenderer)
{
    renderer_init_result Result = {};

    Platform = *PlatformAPI;

    memory_arena_checkpoint InitCheckpoint = ArenaCheckpoint(Arena);

    Result.Renderer = PushStruct(Arena, 0, renderer);
    if (!Result.Renderer) 
    {
        Result.ErrorMessage = "Failed to allocate renderer from arena";
        return(Result);
    }
    renderer* Renderer = Result.Renderer;

    #define ReturnOnFailure(msg)                    \
        if (Result.ErrorCode != VK_SUCCESS) {       \
            Result.Renderer = nullptr;              \
            Result.ErrorMessage = msg;              \
            RestoreArena(Arena, InitCheckpoint);    \
            return(Result);                         \
        }

    #define ReturnWithFailure(err_code, msg)        \
        Result.Renderer = nullptr;                  \
        Result.ErrorCode = err_code;                \
        Result.ErrorMessage = msg;                  \
        RestoreArena(Arena, InitCheckpoint);        \
        return(Result)

    Result.ErrorCode = InitializeVulkan(&Renderer->Vulkan);
    ReturnOnFailure("Failed to initialize Vulkan instance or device");
    VK = Renderer->Vulkan;

    // Create samplers
    {
        for (u32 Index = 0; Index < Sampler_Count; Index++)
        {
            VkSamplerCreateInfo Info = SamplerStateToVulkanSamplerInfo(SamplerInfos[Index]);
            Result.ErrorCode = vkCreateSampler(VK.Device, &Info, nullptr, &Renderer->Samplers[Index]);
            ReturnOnFailure("Failed to create builtin sampler");
        }

        for (u32 Index = 0; Index < R_MaterialSamplerCount; Index++)
        {
            material_sampler_id ID = { Index };
            tex_wrap WrapU, WrapV, WrapW;
            if (GetWrapFromMaterialSampler(ID, &WrapU, &WrapV, &WrapW))
            {
                VkSamplerCreateInfo Info = 
                {
                    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .magFilter = VK_FILTER_LINEAR,
                    .minFilter = VK_FILTER_LINEAR,
                    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                    .addressModeU = WrapTable[WrapU],
                    .addressModeV = WrapTable[WrapV],
                    .addressModeW = WrapTable[WrapW],
                    .mipLodBias = 0.0f,
                    .anisotropyEnable = VK_TRUE,
                    .maxAnisotropy = AnisotropyTable[MaterialSamplerAnisotropy],
                    .compareEnable = VK_FALSE,
                    .compareOp = VK_COMPARE_OP_ALWAYS,
                    .minLod = 0.0f,
                    .maxLod = R_MaxLOD,
                    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                    .unnormalizedCoordinates = VK_FALSE,
                };
                Result.ErrorCode = vkCreateSampler(VK.Device, &Info, nullptr, Renderer->MaterialSamplers + Index);
                ReturnOnFailure("Failed to create material sampler");
            }
            else
            {
                InvalidCodePath;
            }
        }
    }

    // Create descriptor set layouts
    {
        for (u32 Index = 0; Index < Set_Count; Index++)
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
                Bindings[BindingIndex].stageFlags = ShaderStagesToVulkan(Binding->Stages);
                Bindings[BindingIndex].pImmutableSamplers = nullptr;
                if (Binding->Flags & DescriptorFlag_PartiallyBound)
                {
                    BindingFlags[BindingIndex] |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                }
                if (Binding->Flags & DescriptorFlag_Bindless)
                {
                    // HACK(boti): see texture manager;
                    // TODO(boti) [update]: I don't see a reason why this can't just be part of the set layout?
                    Bindings[BindingIndex].descriptorCount = R_MaxTextureCount;
                    BindingFlags[BindingIndex] |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                }
            }

            Result.ErrorCode = vkCreateDescriptorSetLayout(VK.Device, &CreateInfo, nullptr, &Renderer->SetLayouts[Index]);
            ReturnOnFailure("Failed to create descriptor set layout");
        }
    }

    // Profiler query
    for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
    {
        VkQueryPoolCreateInfo QueryPoolInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queryType = VK_QUERY_TYPE_TIMESTAMP,
            .queryCount = 2 * (FrameStage_Count + 1), // NOTE(boti): Begin/End pair for each stage + entire frame
            .pipelineStatistics = 0,
        };
        Result.ErrorCode = vkCreateQueryPool(VK.Device, &QueryPoolInfo, nullptr, &Renderer->PerformanceQueryPools[FrameIndex]);
        ReturnOnFailure("Failed to create performance query pool");
        vkResetQueryPool(VK.Device, Renderer->PerformanceQueryPools[FrameIndex], 0, QueryPoolInfo.queryCount);
    }

    // Vertex Buffer
    {
        if (!CreateGeometryBuffer(R_VertexBufferMaxBlockCount, Arena, &Renderer->GeometryBuffer))
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to create geometry buffer");
        }
    }

    {
        u32 DeviceMemoryIndex = 0;
        u8 ScanResult = BitScanForward(&DeviceMemoryIndex, VK.GPUMemTypes);
        Assert(ScanResult);
        Renderer->DeviceArena = CreateGPUArena(MiB(128), DeviceMemoryIndex, 0);

        gpu_memory_arena* GpuArena = &Renderer->DeviceArena;
        
        Renderer->MipFeedbackMemorySize = R_MaxTextureCount * sizeof(u32);
        b32 PushResult = PushBuffer(GpuArena, Renderer->MipFeedbackMemorySize, 
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                    &Renderer->MipFeedbackBuffer);
        if (!PushResult)
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push MipFeedback buffer");
        }
        
        Renderer->PerFrameBufferSize = MiB(96);
        PushResult = PushBuffer(GpuArena, Renderer->PerFrameBufferSize,
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT|
                                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT|
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                &Renderer->PerFrameBuffer);
        if (!PushResult)
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push PerFrame buffer");
        }
        Renderer->PerFrameBufferAddress = GetBufferDeviceAddress(VK.Device, Renderer->PerFrameBuffer);
        SetObjectName(VK.Device, Renderer->PerFrameBuffer, "PerFrameBuffer");
    }

    // Shadow storage
    {
        VkImageCreateInfo DummyImageInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = FormatTable[RenderTargetFormatTable[RTFormat_Shadow]],
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
        VkMemoryRequirements MemoryRequirements = GetImageMemoryRequirements(VK.Device, &DummyImageInfo, VK_IMAGE_ASPECT_DEPTH_BIT);
        u32 MemoryTypes = VK.GPUMemTypes & MemoryRequirements.memoryTypeBits;
        
        u32 MemoryTypeIndex = 0;
        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
        {
            Renderer->ShadowArena = CreateGPUArena(R_ShadowMapMemorySize, MemoryTypeIndex, GpuMemoryFlag_None);
            if (Renderer->ShadowArena.Memory)
            {
                SetObjectName(VK.Device, Renderer->ShadowArena.Memory, "ShadowMemory");

                VkImageCreateInfo CascadeImageInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = FormatTable[RenderTargetFormatTable[RTFormat_Shadow]],
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
                Result.ErrorCode = vkCreateImage(VK.Device, &CascadeImageInfo, nullptr, &Renderer->CascadeMap);
                ReturnOnFailure("Failed to create shadow cascade image");

                b32 PushResult = PushImage(&Renderer->ShadowArena, Renderer->CascadeMap);
                if (PushResult)
                {
                    VkImageViewCreateInfo ViewInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .image = Renderer->CascadeMap,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                        .format = FormatTable[RenderTargetFormatTable[RTFormat_Shadow]],
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
        
                    Result.ErrorCode = vkCreateImageView(VK.Device, &ViewInfo, nullptr, &Renderer->CascadeArrayView);
                    ReturnOnFailure("Failed to create shadow cascade view");
        
                    for (u32 Cascade = 0; Cascade < R_MaxShadowCascadeCount; Cascade++)
                    {
                        VkImageViewCreateInfo CascadeViewInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .image = Renderer->CascadeMap,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = FormatTable[RenderTargetFormatTable[RTFormat_Shadow]],
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
        
                        Result.ErrorCode = vkCreateImageView(VK.Device, &CascadeViewInfo, nullptr, &Renderer->CascadeViews[Cascade]);
                        ReturnOnFailure("Failed to create shadow cascade layer view");
                    }
                }
                else
                {
                    ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push shadow cascade image");
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
                        .format = FormatTable[RenderTargetFormatTable[RTFormat_Shadow]],
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

                    Result.ErrorCode = vkCreateImage(VK.Device, &ImageInfo, nullptr, &ShadowMap->Image);
                    ReturnOnFailure("Failed to create shadow image");

                    PushResult = PushImage(&Renderer->ShadowArena, ShadowMap->Image);
                    if (PushResult)
                    {
                        VkImageViewCreateInfo ViewInfo = 
                        {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .image = ShadowMap->Image,
                            .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
                            .format = FormatTable[RenderTargetFormatTable[RTFormat_Shadow]],
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
                        Result.ErrorCode = vkCreateImageView(VK.Device, &ViewInfo, nullptr, &ShadowMap->CubeView);
                        ReturnOnFailure("Failed to create shadow image view");

                        for (u32 LayerIndex = 0; LayerIndex < 6; LayerIndex++)
                        {
                            VkImageViewCreateInfo LayerViewInfo = 
                            {
                                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                .pNext = nullptr,
                                .flags = 0,
                                .image = ShadowMap->Image,
                                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                .format = FormatTable[RenderTargetFormatTable[RTFormat_Shadow]],
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
                            Result.ErrorCode = vkCreateImageView(VK.Device, &LayerViewInfo, nullptr, &ShadowMap->LayerViews[LayerIndex]);
                            ReturnOnFailure("Failed to create shadow image layer view");
                        }
                    }
                    else
                    {
                        ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push shadow image");
                    }
                }
            }
            else
            {
                ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to allocate shadow storage");
            }
        }
        else
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to find suitable memory type for shadow storage");
        }
    }

    // Environment BRDF image
    {
        Renderer->EnvironmentBRDFResolution = { 128, 128 };
        VkImageCreateInfo Info = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R16G16_SFLOAT,
            .extent = { Renderer->EnvironmentBRDFResolution.X, Renderer->EnvironmentBRDFResolution.Y, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        Result.ErrorCode = CreateDedicatedImage(&Info, VK.GPUMemTypes, &Renderer->EnvironmentBRDFImage, &Renderer->EnvironmentBRDFMemory);
        ReturnOnFailure("Failed to create BRDF integration LUT image");

        VkImageViewCreateInfo ViewInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = Renderer->EnvironmentBRDFImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R16G16_SFLOAT,
            .components = {},
            .subresourceRange= 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };

        Result.ErrorCode = vkCreateImageView(VK.Device, &ViewInfo, nullptr, &Renderer->EnvironmentBRDFImageView);
        ReturnOnFailure("Failed to create BRDF integration LUT image view");
    }

    // Sky image
    {
        Renderer->SkyResolution = { 256, 256 };
        VkImageCreateInfo Info =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_B10G11R11_UFLOAT_PACK32,
            .extent = { Renderer->SkyResolution.X, Renderer->SkyResolution.Y, 1 },
            .mipLevels = 1,
            .arrayLayers = 6,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        Result.ErrorCode = CreateDedicatedImage(&Info, VK.GPUMemTypes, &Renderer->SkyImage, &Renderer->SkyImageMemory);
        ReturnOnFailure("FailedToCreate sky image");
    }

    // BAR memory
    u32 BARMemoryTypeIndex = 0;
    b32 BARMemoryTypeIndexFound = BitScanForward(&BARMemoryTypeIndex, VK.BARMemTypes);
    if (BARMemoryTypeIndexFound)
    {
        Renderer->BARMemory = CreateGPUArena(MiB(64), BARMemoryTypeIndex, true);
        if (Renderer->BARMemory.Memory)
        {
            SetObjectName(VK.Device, Renderer->BARMemory.Memory, "BAR");
        }
        else
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to create BAR memory");
        }
    }
    else
    {
        ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to find suitable memory type for BAR memory");
    }

    // Persistent descriptor buffers
    // TODO(boti): Assert buffer size <=> SetLayout size here
    {
        umm StaticResourceBufferSize = KiB(64);
        b32 PushResult = PushBuffer(&Renderer->BARMemory, StaticResourceBufferSize, 
                                    VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                    &Renderer->StaticResourceDescriptorBuffer,
                                    &Renderer->StaticResourceDescriptorMapping);
        if (!PushResult)
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push static resource descriptor buffer");
        }
    
        
        umm SamplerBufferSize = KiB(16);
        PushResult = PushBuffer(&Renderer->BARMemory, SamplerBufferSize, 
                                VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                &Renderer->SamplerDescriptorBuffer,
                                &Renderer->SamplerDescriptorMapping);
        if (!PushResult)
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push sampler descriptor buffer");
        }
    }

    // Sampler descriptor upload
    {
        const umm DescriptorSize = VK.DescriptorBufferProps.samplerDescriptorSize;

        VkDeviceSize NamedSamplersOffset = 0;
        vkGetDescriptorSetLayoutBindingOffsetEXT(VK.Device, Renderer->SetLayouts[Set_Sampler], Binding_Sampler_NamedSamplers, &NamedSamplersOffset);
        for (u32 SamplerIndex = 0; SamplerIndex < Sampler_Count; SamplerIndex++)
        {
            VkDescriptorGetInfoEXT DescriptorInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .data = { .pSampler = &Renderer->Samplers[SamplerIndex] },
            };

            vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, 
                               OffsetPtr(Renderer->SamplerDescriptorMapping, NamedSamplersOffset + SamplerIndex*DescriptorSize));
        }

        VkDeviceSize MaterialSamplersOffset = 0;
        vkGetDescriptorSetLayoutBindingOffsetEXT(VK.Device, Renderer->SetLayouts[Set_Sampler], Binding_Sampler_MaterialSamplers, &MaterialSamplersOffset);
        for (u32 MaterialSamplerIndex = 0; MaterialSamplerIndex < R_MaterialSamplerCount; MaterialSamplerIndex++)
        {
            VkDescriptorGetInfoEXT DescriptorInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .data = { .pSampler = &Renderer->MaterialSamplers[MaterialSamplerIndex] },
            };
            vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize,
                               OffsetPtr(Renderer->SamplerDescriptorMapping, MaterialSamplersOffset + MaterialSamplerIndex*DescriptorSize));
        }
    }

    // Static descriptor upload (except RTs)
    {
        descriptor_write Writes[] = 
        {
            {
                .Type = Descriptor_StorageBuffer,
                .Binding = Binding_Static_MipFeedbackBuffer,
                .BaseIndex = 0,
                .Count = 1,
                .Buffers = { { Renderer->MipFeedbackBuffer, 0, Renderer->MipFeedbackMemorySize } },
            },
            {
                .Type = Descriptor_SampledImage,
                .Binding = Binding_Static_CascadedShadow,
                .BaseIndex = 0,
                .Count = 1,
                .Images = { { Renderer->CascadeArrayView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },
            },
            {
                .Type = Descriptor_SampledImage,
                .Binding = Binding_Static_PointShadows,
                .BaseIndex = 0,
                .Count = R_MaxShadowCount,
                .Images = {}, // NOTE(boti): Filled after this array def.
            },
            {
                .Type = Descriptor_SampledImage,
                .Binding = Binding_Static_BRDFLutTexture,
                .BaseIndex = 0,
                .Count = 1,
                .Images = { { Renderer->EnvironmentBRDFImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },
            },
            {
                .Type = Descriptor_StorageImage,
                .Binding = Binding_Static_BRDFLutImage,
                .BaseIndex = 0,
                .Count = 1,
                .Images = { { Renderer->EnvironmentBRDFImageView, VK_IMAGE_LAYOUT_GENERAL } },
            },
        };

        for (u32 WriteIndex = 0; WriteIndex < CountOf(Writes); WriteIndex++)
        {
            descriptor_write* Write = Writes + WriteIndex;
            if (Write->Binding == Binding_Static_PointShadows)
            {
                for (u32 ShadowIndex = 0; ShadowIndex < R_MaxShadowCount; ShadowIndex++)
                {
                    Write->Images[ShadowIndex].View = Renderer->PointShadowMaps[ShadowIndex].CubeView;
                    Write->Images[ShadowIndex].Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
            }
        }

        UpdateDescriptorBuffer(CountOf(Writes), Writes, Renderer->SetLayouts[Set_Static], Renderer->StaticResourceDescriptorMapping);
    }

    // Surface
    {
        Renderer->Surface = Platform.CreateVulkanSurface(VK.Instance);
        if (!Renderer->Surface) 
        {
            ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to create Vulkan surface");
        }

        constexpr u32 MaxSurfaceFormatCount = 32;
        u32 SurfaceFormatCount = MaxSurfaceFormatCount;
        VkSurfaceFormatKHR SurfaceFormats[MaxSurfaceFormatCount];
        Result.ErrorCode = vkGetPhysicalDeviceSurfaceFormatsKHR(VK.PhysicalDevice, Renderer->Surface, &SurfaceFormatCount, SurfaceFormats);
        ReturnOnFailure("Failed to query physical device surface formats");

        for (u32 SurfaceFormatIndex = 0; SurfaceFormatIndex < SurfaceFormatCount; SurfaceFormatIndex++)
        {
            if (SurfaceFormats[SurfaceFormatIndex].format == VK_FORMAT_R8G8B8A8_SRGB ||
                SurfaceFormats[SurfaceFormatIndex].format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                Renderer->SurfaceFormat = SurfaceFormats[SurfaceFormatIndex];
            }
        }

        switch (Renderer->SurfaceFormat.format)
        {
            case VK_FORMAT_R8G8B8A8_SRGB: 
            {
                RenderTargetFormatTable[RTFormat_Swapchain] = Format_R8G8B8A8_SRGB;
            } break;
            case VK_FORMAT_B8G8R8A8_SRGB: 
            {
                RenderTargetFormatTable[RTFormat_Swapchain] = Format_B8G8R8A8_SRGB;
            } break;
            default:
            {
                UnimplementedCodePath;
            } break;
        };

        Renderer->SwapchainImageCount = 2;
        Result.ErrorCode = ResizeSwapchain(Renderer, 0, false);
        ReturnOnFailure("Failed to create Vulkan swapchain");
    }

    if (CreateRenderTargetHeap(&Renderer->RenderTargetHeap, R_RenderTargetMemorySize, Renderer->SetLayouts[Set_Static], Renderer->StaticResourceDescriptorMapping))
    {
        render_target_heap* Heap = &Renderer->RenderTargetHeap;
        constexpr VkImageUsageFlagBits DepthStencil = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        constexpr VkImageUsageFlagBits Color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        constexpr VkImageUsageFlagBits Sampled = VK_IMAGE_USAGE_SAMPLED_BIT;
        constexpr VkImageUsageFlagBits Storage = VK_IMAGE_USAGE_STORAGE_BIT;

        Renderer->DepthBuffer               = PushRenderTarget("Depth",         Heap, FormatTable[RenderTargetFormatTable[RTFormat_Depth]],      DepthStencil|Sampled,   1,
                                                               U32_MAX, U32_MAX, U32_MAX, U32_MAX);
        Renderer->StructureBuffer           = PushRenderTarget("Structure",     Heap, FormatTable[RenderTargetFormatTable[RTFormat_Structure]],  Color|Sampled,          1,
                                                               Binding_Static_StructureImage, U32_MAX, U32_MAX, U32_MAX);
        Renderer->HDRRenderTarget           = PushRenderTarget("HDR",           Heap, FormatTable[RenderTargetFormatTable[RTFormat_HDR]],        Color|Sampled|Storage,  0,
                                                               Binding_Static_HDRColorImage, U32_MAX, Binding_Static_HDRColorImageGeneral, Binding_Static_HDRMipStorageImages);
        Renderer->TransparentRenderTarget   = PushRenderTarget("Transparent",   Heap, FormatTable[RenderTargetFormatTable[RTFormat_HDR]],        Color|Sampled,          0,
                                                               Binding_Static_TransparentImage, U32_MAX, U32_MAX, U32_MAX);
        Renderer->BloomTarget               = PushRenderTarget("Bloom",         Heap, FormatTable[RenderTargetFormatTable[RTFormat_HDR]],        Color|Sampled|Storage,  0,
                                                               Binding_Static_BloomImage, U32_MAX, Binding_Static_BloomImageGeneral, Binding_Static_BloomMipStorageImages);
        Renderer->OcclusionBuffers[0]       = PushRenderTarget("OcclusionRaw",  Heap, FormatTable[RenderTargetFormatTable[RTFormat_Occlusion]],  Color|Sampled|Storage,  1,
                                                               Binding_Static_OcclusionRawImage, Binding_Static_OcclusionRawStorageImage, U32_MAX, U32_MAX);
        Renderer->OcclusionBuffers[1]       = PushRenderTarget("Occlusion",     Heap, FormatTable[RenderTargetFormatTable[RTFormat_Occlusion]],  Color|Sampled|Storage,  1,
                                                               Binding_Static_OcclusionImage, Binding_Static_OcclusionStorageImage, U32_MAX, U32_MAX);
        Renderer->VisibilityBuffer          = PushRenderTarget("Visibility",    Heap, FormatTable[RenderTargetFormatTable[RTFormat_Visibility]], Color|Sampled,          1,
                                                               Binding_Static_VisibilityImage, U32_MAX, U32_MAX, U32_MAX);
    }
    else
    {
        ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to create render target heap");
    }

    for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
    {
        VkFenceCreateInfo ImageFenceInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        Result.ErrorCode = vkCreateFence(VK.Device, &ImageFenceInfo, nullptr, &Renderer->ImageAcquiredFences[FrameIndex]);
        ReturnOnFailure("Failed to create image acquire fence");

        VkSemaphoreCreateInfo SemaphoreInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        Result.ErrorCode = vkCreateSemaphore(VK.Device, &SemaphoreInfo, nullptr, &Renderer->ImageAcquiredSemaphores[FrameIndex]);
        ReturnOnFailure("Failed to create image acquire semaphore");

        Result.ErrorCode = vkCreateSemaphore(VK.Device, &SemaphoreInfo, nullptr, &Renderer->RenderFinishedSemaphores[FrameIndex]);
        ReturnOnFailure("Failed to render finished semaphore");

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
        Result.ErrorCode = vkCreateSemaphore(VK.Device, &TimelineSemaphoreInfo, nullptr, &Renderer->TimelineSemaphore);
        ReturnOnFailure("Failed to create timeline semaphore");
        Renderer->TimelineSemaphoreCounter = 0;

        Result.ErrorCode = vkCreateSemaphore(VK.Device, &TimelineSemaphoreInfo, nullptr, &Renderer->ComputeTimelineSemaphore);
        ReturnOnFailure("Failed to create compute timeline semaphore");
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
            Result.ErrorCode = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->CmdPools[FrameIndex]);
            ReturnOnFailure("Failed to create graphics command pool");
        
            VkCommandBufferAllocateInfo PrimaryBufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = Renderer->CmdPools[FrameIndex],
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = Renderer->MaxCmdBufferCountPerFrame,
            };
            Result.ErrorCode = vkAllocateCommandBuffers(VK.Device, &PrimaryBufferInfo, Renderer->CmdBuffers[FrameIndex]);
            ReturnOnFailure("Failed to allocate graphics primary command buffers");

            VkCommandBufferAllocateInfo SecondaryBufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = Renderer->CmdPools[FrameIndex],
                .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                .commandBufferCount = Renderer->MaxSecondaryCmdBufferCountPerFrame,
            };
            Result.ErrorCode = vkAllocateCommandBuffers(VK.Device, &SecondaryBufferInfo, Renderer->SecondaryCmdBuffers[FrameIndex]);
            ReturnOnFailure("Failed to allocate graphics secondary command buffers");
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
            Result.ErrorCode = vkCreateCommandPool(VK.Device, &PoolInfo, nullptr, &Renderer->ComputeCmdPools[FrameIndex]);
            ReturnOnFailure("Failed to create compute command pool");
        
            VkCommandBufferAllocateInfo BufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = Renderer->ComputeCmdPools[FrameIndex],
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            Result.ErrorCode = vkAllocateCommandBuffers(VK.Device, &BufferInfo, &Renderer->ComputeCmdBuffers[FrameIndex]);
            ReturnOnFailure("Failed to allocate compute command buffers");
        }
        
    }

    // Texture Manager
    // TODO(boti): remove this
    Renderer->TextureManager.DescriptorSetLayout = Renderer->SetLayouts[Set_Bindless];
    if (!CreateTextureManager(&Renderer->TextureManager, Arena, R_TextureMemorySize, VK.GPUMemTypes, Renderer->SetLayouts))
    {
        ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to create texture manager");
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
                .size = MiB(512),
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
            };

            VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &BufferInfo);

            u32 MemoryType = 0;
            if (BitScanForward(&MemoryType, MemoryRequirements.memoryTypeBits & VK.TransferMemTypes))
            {
                VkMemoryAllocateInfo AllocInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .allocationSize = R_MaxFramesInFlight * BufferInfo.size,
                    .memoryTypeIndex = MemoryType,
                };
                Result.ErrorCode = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->StagingMemory);
                ReturnOnFailure("Failed to allocate staging memory");

                SetObjectName(VK.Device, Renderer->StagingMemory, "Staging");

                Result.ErrorCode = vkMapMemory(VK.Device, Renderer->StagingMemory, 0, VK_WHOLE_SIZE, 0, &Renderer->StagingMemoryMapping);
                for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
                {
                    Result.ErrorCode = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Renderer->StagingBuffers[FrameIndex]);
                    ReturnOnFailure("Failed to create staging buffer");

                    umm Offset = FrameIndex * BufferInfo.size;
                    Result.ErrorCode = vkBindBufferMemory(VK.Device, Renderer->StagingBuffers[FrameIndex], Renderer->StagingMemory, Offset);
                    ReturnOnFailure("Failed to bind staging buffer to staging memory");

                    Renderer->Frames[FrameIndex].StagingBuffer.Base = OffsetPtr(Renderer->StagingMemoryMapping, Offset);
                    Renderer->Frames[FrameIndex].StagingBuffer.Size = BufferInfo.size;
                    Renderer->Frames[FrameIndex].StagingBuffer.At   = 0;
                }
            }
            else
            {
                ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to find suitable memory type for staging memory");
            }
        }

        // Per-frame (texture mip) readback buffer
        {
            VkBufferCreateInfo BufferInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = R_MaxTextureCount * sizeof(u32),
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
            };

            VkMemoryRequirements MemoryRequirements = GetBufferMemoryRequirements(VK.Device, &BufferInfo);

            u32 MemoryType = 0;
            if (BitScanForward(&MemoryType, MemoryRequirements.memoryTypeBits & VK.ReadbackMemTypes))
            {
                VkMemoryAllocateInfo AllocInfo = 
                {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .allocationSize = R_MaxFramesInFlight * BufferInfo.size,
                    .memoryTypeIndex = MemoryType,
                };
                Result.ErrorCode = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->MipReadbackMemory);
                ReturnOnFailure("Failed to allocate mip readback memory");

                SetObjectName(VK.Device, Renderer->MipReadbackMemory, "MipReadback");

                Result.ErrorCode = vkMapMemory(VK.Device, Renderer->MipReadbackMemory, 0, VK_WHOLE_SIZE, 0, &Renderer->MipReadbackMapping);
                for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
                {
                    Result.ErrorCode = vkCreateBuffer(VK.Device, &BufferInfo, nullptr, &Renderer->MipReadbackBuffers[FrameIndex]);
                    ReturnOnFailure("Failed to create mip readback buffer");

                    umm Offset = FrameIndex * BufferInfo.size;
                    Result.ErrorCode = vkBindBufferMemory(VK.Device, Renderer->MipReadbackBuffers[FrameIndex], Renderer->MipReadbackMemory, Offset);
                    ReturnOnFailure("Failed to bind mip readback buffer to mip readback memory");
                    Renderer->MipReadbackMappings[FrameIndex] = OffsetPtr(Renderer->MipReadbackMapping, Offset);
                }
            }
            else
            {
                ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to find suitable memory type for mip readback");
            }
        }

        // Per-frame descriptor buffer
        {
            umm ResourceBufferSize = KiB(64);
            for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, ResourceBufferSize, 
                                            VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            Renderer->PerFrameResourceDescriptorBuffers + FrameIndex,
                                            Renderer->PerFrameResourceDescriptorMappings + FrameIndex);
                if (!PushResult)
                {
                    ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push per-frame resource descriptor buffer");
                }
            }
        }

        // Per frame uniform data
        {
            constexpr u64 BufferSize = KiB(64);
            for (u32 i = 0; i < R_MaxFramesInFlight; i++)
            {
                b32 PushResult = PushBuffer(&Renderer->BARMemory, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            Renderer->PerFrameUniformBuffers + i,
                                            Renderer->PerFrameUniformBufferMappings + i);
                if (!PushResult)
                {
                    ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push per-frame uniform buffer");
                }
            }
        }

        // Per frame buffer
        {
            for (u32 FrameIndex = 0; FrameIndex < R_MaxFramesInFlight; FrameIndex++)
            {
                VkBufferUsageFlags Usage = 
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT|
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
                b32 PushResult = PushBuffer(&Renderer->BARMemory, Renderer->BARBufferSize, Usage,
                                            Renderer->BARBuffers + FrameIndex,
                                            Renderer->BARBufferMappings + FrameIndex);
                if (PushResult)
                {
                    Renderer->BARBufferAddresses[FrameIndex] = GetBufferDeviceAddress(VK.Device, Renderer->BARBuffers[FrameIndex]);
                }
                else
                {
                    ReturnWithFailure(VK_ERROR_UNKNOWN, "Failed to push per-frame BAR buffer");
                }
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
            .setLayoutCount = Set_Count,
            .pSetLayouts = Renderer->SetLayouts,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &PushConstantRange,
        };

        Result.ErrorCode = vkCreatePipelineLayout(VK.Device, &Info, nullptr, &Renderer->SystemPipelineLayout);
        ReturnOnFailure("Failed to create system pipeline layout");
    }

    // Pipelines
    Result.ErrorCode = CreatePipelines(Renderer, Scratch);
    ReturnOnFailure("Failed to create pipelines");

    // Gather info
    Result.Info.DeviceName = Renderer->Vulkan.DeviceProps.properties.deviceName;
    static_assert(sizeof(Renderer->Vulkan.Vulkan11Props.deviceLUID) == sizeof(device_luid));
    memcpy(&Result.Info.DeviceLUID, Renderer->Vulkan.Vulkan11Props.deviceLUID, sizeof(device_luid));

    #undef ReturnOnFailure
    #undef ReturnWithFailure

    return(Result);
}

internal VkResult ResizeSwapchain(renderer* Renderer, u32 FrameID, b32 Forced)
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

                for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                {
                    PushDeletionEntry(&Renderer->DeletionQueue, FrameID, Renderer->SwapchainImageViews[i]);
                    Renderer->SwapchainImageViews[i] = VK_NULL_HANDLE;
                }
                PushDeletionEntry(&Renderer->DeletionQueue, FrameID, Renderer->SwapchainDepthImageView);
                PushDeletionEntry(&Renderer->DeletionQueue, FrameID, Renderer->SwapchainDepthImage);

                PushDeletionEntry(&Renderer->DeletionQueue, FrameID, Renderer->SwapchainDepthMemory);

                PushDeletionEntry(&Renderer->DeletionQueue, FrameID, Renderer->Swapchain);
            }

            VkPresentModeKHR PresentMode = 
                //VK_PRESENT_MODE_FIFO_KHR
                //VK_PRESENT_MODE_MAILBOX_KHR
                VK_PRESENT_MODE_IMMEDIATE_KHR
                ;

            VkSwapchainCreateInfoKHR CreateInfo = 
            {
                .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext                  = nullptr,
                .flags                  = 0/*|VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR*/,
                .surface                = Renderer->Surface,
                .minImageCount          = Renderer->SwapchainImageCount,
                .imageFormat            = Renderer->SurfaceFormat.format,
                .imageColorSpace        = Renderer->SurfaceFormat.colorSpace,
                .imageExtent            = SurfaceCaps.currentExtent,
                .imageArrayLayers       = 1,
                .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount  = 1,
                .pQueueFamilyIndices    = &VK.GraphicsQueueIdx,
                .preTransform           = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode            = PresentMode,
                .clipped                = VK_FALSE,
                .oldSwapchain           = Renderer->Swapchain,
            };
            Result = vkCreateSwapchainKHR(VK.Device, &CreateInfo, nullptr, &Renderer->Swapchain);
            if (Result == VK_SUCCESS)
            {
                Result = vkGetSwapchainImagesKHR(VK.Device, Renderer->Swapchain, &Renderer->SwapchainImageCount, Renderer->SwapchainImages);
                if (Result == VK_SUCCESS)
                {
                    for (u32 i = 0; i < Renderer->SwapchainImageCount; i++)
                    {
                        SetObjectName(VK.Device, Renderer->SwapchainImages[i], "SwapchainImage");

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

                    VkImageCreateInfo DepthImageInfo = 
                    {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .imageType = VK_IMAGE_TYPE_2D,
                        .format = FormatTable[RenderTargetFormatTable[RTFormat_Depth]],
                        .extent = { Renderer->SurfaceExtent.width, Renderer->SurfaceExtent.height, 1 },
                        .mipLevels = 1,
                        .arrayLayers = 1,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .tiling = VK_IMAGE_TILING_OPTIMAL,
                        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                        .queueFamilyIndexCount = 0,
                        .pQueueFamilyIndices = nullptr,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    };

                    Result = vkCreateImage(VK.Device, &DepthImageInfo, nullptr, &Renderer->SwapchainDepthImage);
                    if (Result == VK_SUCCESS)
                    {
                        SetObjectName(VK.Device, Renderer->SwapchainDepthImage, "SwapchainDepthImage");

                        VkMemoryRequirements MemoryRequirements = {};
                        vkGetImageMemoryRequirements(VK.Device, Renderer->SwapchainDepthImage, &MemoryRequirements);
                        u32 MemoryTypes = MemoryRequirements.memoryTypeBits & VK.GPUMemTypes;
                        u32 MemoryTypeIndex;
                        if (BitScanForward(&MemoryTypeIndex, MemoryTypes))
                        {
                            VkMemoryAllocateInfo AllocInfo = 
                            {
                                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                .pNext = nullptr,
                                .allocationSize = MemoryRequirements.size,
                                .memoryTypeIndex = MemoryTypeIndex,
                            };
                            Result = vkAllocateMemory(VK.Device, &AllocInfo, nullptr, &Renderer->SwapchainDepthMemory);
                            if (Result == VK_SUCCESS)
                            {
                                Result = vkBindImageMemory(VK.Device, Renderer->SwapchainDepthImage, Renderer->SwapchainDepthMemory, 0);
                                if (Result == VK_SUCCESS)
                                {
                                    VkImageViewCreateInfo DepthViewInfo = 
                                    {
                                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                        .pNext = nullptr,
                                        .flags = 0,
                                        .image = Renderer->SwapchainDepthImage,
                                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                        .format = FormatTable[RenderTargetFormatTable[RTFormat_Depth]],
                                        .components = {},
                                        .subresourceRange = 
                                        {
                                            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                            .baseMipLevel = 0,
                                            .levelCount = VK_REMAINING_MIP_LEVELS,
                                            .baseArrayLayer = 0,
                                            .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                        },
                                    };
                                    Result = vkCreateImageView(VK.Device, &DepthViewInfo, nullptr, &Renderer->SwapchainDepthImageView);
                                }
                            }
                        }
                        else
                        {
                            UnhandledError("No suitable memory type for swapchain depth image");
                        }
                    }
                    else
                    {
                        UnhandledError("Failed to create swapchain depth image");
                    }
                }
                else
                {
                    UnhandledError("Failed to get swapchain images");
                }
            }
            else
            {
                UnhandledError("Failed to (re)create swapchain");
            }
        }
    }
    return Result;
}

internal void 
DrawList(render_frame* Frame, VkCommandBuffer CmdBuffer, pipeline* Pipelines, draw_list* List)
{
    // NOTE(boti): We only support a single buffer for index memory
    vkCmdBindIndexBuffer(CmdBuffer, Frame->Renderer->GeometryBuffer.IndexMemory.Buffers[0], 0, VK_INDEX_TYPE_UINT32);

    umm CurrentOffset = 0;
    for (u32 Group = 0; Group < DrawGroup_Count; Group++)
    {
        u32 DrawCount = List->DrawGroupDrawCounts[Group];
        if (DrawCount == 0)
        {
            continue;
        }

        umm DrawBufferOffset = List->DrawBufferOffset + CurrentOffset;
        CurrentOffset += DrawCount * sizeof(VkDrawIndexedIndirectCommand);
        if (Pipelines[Group] == Pipeline_None)
        {
            continue;
        }

        vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Frame->Renderer->Pipelines[Pipelines[Group]].Pipeline);
        vkCmdDrawIndexedIndirect(CmdBuffer, List->DrawBuffer, DrawBufferOffset, DrawCount, sizeof(VkDrawIndexedIndirectCommand));
    }
}

internal void BeginFrameStage(VkCommandBuffer CmdBuffer, frame_stage_type Stage, VkQueryPool PerfQueries, frame_stage* Stages)
{
    TimedFunction(Platform.Profiler);

    vkCmdWriteTimestamp2(CmdBuffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, PerfQueries, 2 * Stage);

    vkCmdBeginDebugUtilsLabelEXT(CmdBuffer, FrameStageNames[Stage]);

    VkDependencyInfo DependencyInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .memoryBarrierCount         = Stages[Stage].GlobalMemoryBarrierCount,
        .pMemoryBarriers            = Stages[Stage].GlobalMemoryBarriers,
        .bufferMemoryBarrierCount   = Stages[Stage].BufferMemoryBarrierCount,
        .pBufferMemoryBarriers      = Stages[Stage].BufferMemoryBarriers,
        .imageMemoryBarrierCount    = Stages[Stage].ImageMemoryBarrierCount,
        .pImageMemoryBarriers       = Stages[Stage].ImageMemoryBarriers,
    };
    vkCmdPipelineBarrier2(CmdBuffer, &DependencyInfo);
}
internal void EndFrameStage(VkCommandBuffer CmdBuffer, frame_stage_type Stage, VkQueryPool PerfQueries, frame_stage* Stages)
{
    vkCmdEndDebugUtilsLabelEXT(CmdBuffer);

    vkCmdWriteTimestamp2(CmdBuffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, PerfQueries, 2 * Stage + 1);
}

internal b32 PushBeginBarrier(frame_stage* Stage, const VkImageMemoryBarrier2* Barrier)
{
    b32 Result = false;
    if (Stage->ImageMemoryBarrierCount < Stage->MaxImageMemoryBarrierCount)
    {
        Stage->ImageMemoryBarriers[Stage->ImageMemoryBarrierCount++] = *Barrier;
        Result = true;
    }
    return(Result);
}

internal b32 PushBeginBarrier(frame_stage* Stage, const VkBufferMemoryBarrier2* Barrier)
{
    b32 Result = false;
    if (Stage->BufferMemoryBarrierCount < Stage->MaxBufferMemoryBarrierCount)
    {
        Stage->BufferMemoryBarriers[Stage->BufferMemoryBarrierCount++] = *Barrier;
        Result = true;
    }
    return(Result);
}

internal b32 PushBeginBarrier(frame_stage* Stage, const VkMemoryBarrier2* Barrier)
{
    b32 Result = false;
    if (Stage->GlobalMemoryBarrierCount < Stage->MaxGlobalMemoryBarrierCount)
    {
        Stage->GlobalMemoryBarriers[Stage->GlobalMemoryBarrierCount++] = *Barrier;
        Result = true;
    }
    return(Result);
}

internal VkCommandBuffer
BeginCommandBuffer(renderer* Renderer, u32 FrameID, begin_cb_flags Flags, pipeline Pipeline)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    if (Flags & BeginCB_Secondary)
    {
        Assert(Renderer->SecondaryCmdBufferAt < Renderer->MaxSecondaryCmdBufferCountPerFrame);
        CommandBuffer = Renderer->SecondaryCmdBuffers[FrameID][Renderer->SecondaryCmdBufferAt++];
    }
    else
    {

        Assert(Renderer->CmdBufferAt < Renderer->MaxCmdBufferCountPerFrame);
        CommandBuffer = Renderer->CmdBuffers[FrameID][Renderer->CmdBufferAt++];
    }

    
    const pipeline_info* PipelineInfo = PipelineInfos + Pipeline;
    Assert(PipelineInfo->Type == PipelineType_Graphics);
    VkFormat ColorAttachmentFormats[MaxColorAttachmentCount];
    for (u32 FormatIndex = 0; FormatIndex < PipelineInfo->ColorAttachmentCount; FormatIndex++)
    {
        ColorAttachmentFormats[FormatIndex] = FormatTable[RenderTargetFormatTable[PipelineInfo->ColorAttachments[FormatIndex]]];
    }

    VkCommandBufferInheritanceRenderingInfo RenderingInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewMask = 0,
        .colorAttachmentCount = PipelineInfo->ColorAttachmentCount,
        .pColorAttachmentFormats = ColorAttachmentFormats,
        .depthAttachmentFormat = FormatTable[RenderTargetFormatTable[PipelineInfo->DepthAttachment]],
        .stencilAttachmentFormat = FormatTable[RenderTargetFormatTable[PipelineInfo->StencilAttachment]],
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkCommandBufferInheritanceInfo InheritanceInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .pNext = &RenderingInfo,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .framebuffer = VK_NULL_HANDLE,
        .occlusionQueryEnable = VK_FALSE,
        .queryFlags = 0,
        .pipelineStatistics = 0,
    };

    VkCommandBufferBeginInfo BeginInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = &InheritanceInfo,
    };
    if (Pipeline != Pipeline_None)
    {
        BeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

    VkDeviceAddress StaticResourceBufferAddress = GetBufferDeviceAddress(VK.Device, Renderer->StaticResourceDescriptorBuffer);
    VkDeviceAddress SamplerBufferAddress = GetBufferDeviceAddress(VK.Device, Renderer->SamplerDescriptorBuffer);
    VkDeviceAddress PerFrameResourceBufferAddress = GetBufferDeviceAddress(VK.Device, Renderer->PerFrameResourceDescriptorBuffers[FrameID]);
    VkDescriptorBufferBindingInfoEXT DescriptorBufferBindings[] = 
    {
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .pNext = nullptr,
            .address = StaticResourceBufferAddress,
            .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .pNext = nullptr,
            .address = SamplerBufferAddress,
            .usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .pNext = nullptr,
            .address = Renderer->TextureManager.DescriptorAddress,
            .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .pNext = nullptr,
            .address = PerFrameResourceBufferAddress,
            .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
        },
    };
    vkCmdBindDescriptorBuffersEXT(CommandBuffer, CountOf(DescriptorBufferBindings), DescriptorBufferBindings);

    u32 DescriptorBufferIndices[Set_Count] = { 0, 1, 2, 3 };
    VkDeviceSize DescriptorBufferOffsets[Set_Count] = { 0, 0, 0, 0 };
    vkCmdSetDescriptorBufferOffsetsEXT(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Renderer->SystemPipelineLayout,
                                       0, CountOf(DescriptorBufferBindings), DescriptorBufferIndices, DescriptorBufferOffsets);
    vkCmdSetDescriptorBufferOffsetsEXT(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Renderer->SystemPipelineLayout,
                                       0, CountOf(DescriptorBufferBindings), DescriptorBufferIndices, DescriptorBufferOffsets);

    return(CommandBuffer);
}

internal void
EndCommandBuffer(VkCommandBuffer CB)
{
    vkEndCommandBuffer(CB);
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
    TimedFunction(Platform.Profiler);

    u32 FrameID = (u32)(Renderer->CurrentFrameID % R_MaxFramesInFlight);
    render_frame* Frame = Renderer->Frames + FrameID;
    Frame->Renderer = Renderer;
    Frame->Arena = Arena;

    Frame->ReloadShaders = false;
    Frame->FrameID = FrameID;
    Frame->RenderExtent = RenderExtent;
    Frame->OutputExtent = { Renderer->SurfaceExtent.width, Renderer->SurfaceExtent.height };

    // TODO(boti): See renderer struct
    Renderer->CmdBufferAt = 0;
    Renderer->SecondaryCmdBufferAt = 0;

    // Reset buffers
    {
        Frame->StagingBuffer.At = 0;

        Frame->BARBufferSize = Renderer->BARBufferSize;
        Frame->BARBufferAt = 0;
        Frame->BARBufferBase = Renderer->BARBufferMappings[FrameID];

        Frame->CommandCount = 0;
        Frame->Commands = PushArray(Arena, 0, render_command, Frame->MaxCommandCount);
        for (u32 GroupIndex = 0; GroupIndex < DrawGroup_Count; GroupIndex++)
        {
            Frame->DrawGroupDrawCounts[GroupIndex] = 0;
        }
        Frame->LastBatch2D = nullptr;

        Frame->LightCount = 0;
        Frame->ShadowCount = 0;
        
        Frame->UniformData = Renderer->PerFrameUniformBufferMappings[FrameID];
    }
    
    Frame->Uniforms = {};

    {
        TimedBlock(Platform.Profiler, "Wait for GPU previous frame");
        VkSemaphoreWaitInfo WaitInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &Renderer->TimelineSemaphore,
            .pValues = &Renderer->FrameFinishedCounters[FrameID],
        };
        vkWaitSemaphores(VK.Device, &WaitInfo, U64_MAX);

        vkResetCommandPool(VK.Device, Renderer->CmdPools[FrameID], 0/*|VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT*/);
        vkResetCommandPool(VK.Device, Renderer->ComputeCmdPools[FrameID], 0);
    }
    ProcessDeletionEntries(&Renderer->DeletionQueue, FrameID);

    // Perf readback
    {
        struct query_timestamp
        {
            u64 Timestamp;
            u32 Availability;
        };

        query_timestamp Timestamps[2 * (FrameStage_Count + 1)] = {};
        VkResult QueryResult = vkGetQueryPoolResults(VK.Device, Renderer->PerformanceQueryPools[FrameID], 
                                                     0, CountOf(Timestamps), sizeof(Timestamps), Timestamps, sizeof(*Timestamps), 
                                                     VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        f64 InvFrequency = VK.TimestampPeriod / (1000.0*1000.0*1000.0);

        u64 FrameDeltaTSC = Timestamps[2*FrameStage_Count + 1].Timestamp - Timestamps[2*FrameStage_Count + 0].Timestamp;
        f32 FrameTime = (f32)(FrameDeltaTSC * InvFrequency);

        render_stats* Stats = &Frame->Stats;
        Stats->FrameTime = FrameTime;
        Assert(FrameStage_Count <= Stats->MaxPerfEntryCount);
        Stats->PerfEntryCount = Min((u32)FrameStage_Count, Stats->MaxPerfEntryCount);
        for (u32 Stage = 0; Stage < FrameStage_Count; Stage++)
        {
            render_stat_perf_entry* Entry = Stats->PerfEntries + Stage;
            Entry->Name = FrameStageNames[Stage];
            u64 DeltaTSC = Timestamps[2*Stage + 1].Timestamp - Timestamps[2*Stage + 0].Timestamp;
            Entry->Time = (f32)(DeltaTSC * InvFrequency);
        }
    }

    // Mip readback
    {
        TimedBlock(Platform.Profiler, "MipReadback");

        texture_manager* Manager = &Renderer->TextureManager;
        
        u32 SampledTextureCount = 0;
        u32* SampledTextureMips = PushArray(Frame->Arena, 0, u32, Manager->TextureCount);
        u32* SampledTextureIndices = PushArray(Frame->Arena, 0, u32, Manager->TextureCount);

        u32* MipFeedbacks = (u32*)Renderer->MipReadbackMappings[FrameID];
        for (u32 TextureIndex = 0; TextureIndex < R_MaxTextureCount; TextureIndex++)
        {
            if (TextureIndex >= Manager->TextureCount)
            {
                break;
            }

            u32 MipFeedback = MipFeedbacks[TextureIndex];
            renderer_texture* Texture = Manager->Textures + TextureIndex;
            if (!(Texture->Flags & TextureFlag_PersistentMemory))
            {
                Texture->LastMipAccess = MipFeedback;
                if (MipFeedback)
                {
                    u32 Index = SampledTextureCount++;
                    SampledTextureMips[Index] = MipFeedbacks[TextureIndex];
                    SampledTextureIndices[Index] = TextureIndex;
                }
            }
        }
        
        Frame->TextureRequestCount = 0;
        Frame->TextureRequests = PushArray(Frame->Arena, 0, texture_request, SampledTextureCount);
        for (u32 RequestCandidateIndex = 0; RequestCandidateIndex < SampledTextureCount; RequestCandidateIndex++)
        {
            u32 SampledMips = SampledTextureMips[RequestCandidateIndex];
            // NOTE(boti): Pad the request with the low resolution mips
            SampledMips = SetBitsBelowHighInclusive(SampledMips);

            u32 TextureIndex = SampledTextureIndices[RequestCandidateIndex];
            u32 CurrentMips = Manager->Textures[TextureIndex].MipResidencyMask;
            // NOTE(boti): Only request mips that are not present
            u32 RequestedMips = SampledMips & (~CurrentMips);
            if (RequestedMips)
            {
                Frame->TextureRequests[Frame->TextureRequestCount++] = { { TextureIndex }, RequestedMips };
            }
        }
    }

    return(Frame);
}

extern "C" Signature_EndRenderFrame(EndRenderFrame)
{
    TimedFunction(Platform.Profiler);

    renderer* Renderer = Frame->Renderer;
    if (Frame->ReloadShaders)
    {
        vkDeviceWaitIdle(Renderer->Vulkan.Device);
        CreatePipelines(Renderer, Frame->Arena);
    }

    // Acquire image
    u32 SwapchainImageIndex = U32_MAX;
    {
        TimedBlock(Platform.Profiler, "Acquire swapchain image");

        vkWaitForFences(VK.Device, 1, &Renderer->ImageAcquiredFences[Frame->FrameID], VK_TRUE, UINT64_MAX);
        vkResetFences(VK.Device, 1, &Renderer->ImageAcquiredFences[Frame->FrameID]);
        ResizeSwapchain(Renderer, Frame->FrameID, false);

        for (;;)
        {
            VkResult ImageAcquireResult = vkAcquireNextImageKHR(VK.Device, Renderer->Swapchain, 0, 
                                                                Renderer->ImageAcquiredSemaphores[Frame->FrameID], 
                                                                Renderer->ImageAcquiredFences[Frame->FrameID], 
                                                                &SwapchainImageIndex);
            if (ImageAcquireResult == VK_SUCCESS)
            {
                Assert(SwapchainImageIndex != U32_MAX);
                break;
            }
            else if (ImageAcquireResult == VK_SUBOPTIMAL_KHR)
            {
                UnimplementedCodePath;
                ResizeSwapchain(Renderer, Frame->FrameID, true);
            }
            else if (ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
            {
                Assert(SwapchainImageIndex == U32_MAX);
                ResizeSwapchain(Renderer, Frame->FrameID, true);
            }
            else if (ImageAcquireResult == VK_TIMEOUT || ImageAcquireResult == VK_NOT_READY)
            {
                UnimplementedCodePath;
            }
            else
            {
                UnhandledError("vkAcquireNextImage error");
                return;
            }
        }
    }

    Frame->OutputExtent = { Renderer->SurfaceExtent.width, Renderer->SurfaceExtent.height };
    if (Frame->OutputExtent.X == 0 || Frame->OutputExtent.Y == 0)
    {
        return;
    }
    if (Frame->RenderExtent.X == 0 || Frame->RenderExtent.Y == 0)
    {
        Frame->RenderExtent = Frame->OutputExtent;
    }

    if (Frame->RenderExtent.X != Renderer->RenderTargetHeap.CurrentExtent.X ||
        Frame->RenderExtent.Y != Renderer->RenderTargetHeap.CurrentExtent.Y)
    {
        ResizeRenderTargets(&Renderer->RenderTargetHeap, &Renderer->DeletionQueue, Frame->FrameID, Frame->RenderExtent);
    }

    Frame->Uniforms.RenderExtent = Frame->RenderExtent;
    Frame->Uniforms.OutputExtent = Frame->OutputExtent;

    frame_stage* FrameStages = PushArray(Frame->Arena, 0, frame_stage, FrameStage_Count);
    for (u32 FrameStageIndex = 0; FrameStageIndex < FrameStage_Count; FrameStageIndex++)
    {
        FrameStages[FrameStageIndex].GlobalMemoryBarrierCount = 0;
        FrameStages[FrameStageIndex].BufferMemoryBarrierCount = 0;
        FrameStages[FrameStageIndex].ImageMemoryBarrierCount = 0;
    }

    // NOTE(boti): This is currently also the initial transfer and skinning cmd buffer
    VkCommandBuffer PrepassCmd = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_None, Pipeline_None);
    VkCommandBuffer RenderCmd = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_None, Pipeline_None);
    // NOTE(boti): Light binning, SSAO
    VkCommandBuffer PreLightCmd = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_None, Pipeline_None);
    VkCommandBuffer ShadowCmd = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_None, Pipeline_None);

    f32 AspectRatio = (f32)Frame->RenderExtent.X / (f32)Frame->RenderExtent.Y;

    // NOTE(boti): We only support a single index buffer
    Frame->Uniforms.IndexBufferAddress = Renderer->GeometryBuffer.IndexMemory.MemoryAddresses[0];

    Frame->Uniforms.Ambience                    = 0.5f * v3{ 1.0f, 1.0f, 1.0f }; // TODO(boti): Expose this in the API
    Frame->Uniforms.Exposure                    = Frame->Config.Exposure;
    Frame->Uniforms.ParallaxScale               = Frame->Config.ParallaxScale;
    Frame->Uniforms.DebugViewMode               = Frame->Config.DebugViewMode;
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
        // Infinite Reverse Z
        Frame->ProjectionTransform = M4(
            g / s,  0.0f,   0.0f, 0.0f,
            0.0f,   g,      0.0f, 0.0f,
            0.0f,   0.0f,   0.0f, n,
            0.0f,   0.0f,   1.0f, 0.0f);
        Frame->InverseProjectionTransform = M4(
            s / g,  0.0f,       0.0f,           0.0f,
            0.0f,   1.0f / g,   0.0f,           0.0f,
            0.0f,   0.0f,       0.0f,           1.0f,
            0.0f,   0.0f,       1.0f / n,       0.0f);
        #elif 0
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
            .Far    = v4{ 0.0f, 0.0f,  0.0f, 0.0f } * Frame->ViewTransform,
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

    VkCommandBuffer UploadCB = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_Secondary, Pipeline_None);

    // HACK(boti): Kind of a hacky way of doing init time stuff
    // The reason we're doing it this way is so that we don't allocate a command buffer/pool at init time
    if (Renderer->CurrentFrameID == 0)
    {
        // Clear null texture
        VkImageMemoryBarrier2 BeginBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Renderer->TextureManager.Textures[0].ImageHandle,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };

        PushBeginBarrier(&FrameStages[FrameStage_Upload], &BeginBarrier);

        VkClearColorValue ClearColor = {};
        vkCmdClearColorImage(UploadCB, BeginBarrier.image, BeginBarrier.newLayout, &ClearColor, 1, &BeginBarrier.subresourceRange);

        VkImageMemoryBarrier2 EndBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Renderer->TextureManager.Textures[0].ImageHandle,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };

        PushBeginBarrier(&FrameStages[FrameStage_Upload + 1], &EndBarrier);
    }

    //
    // Discard unused mip levels
    //
    // TODO(boti): Build an actual LRU and only discard mips when we can't fit new ones anymore
    umm PageUsageLimit = Renderer->TextureManager.Cache.PageCount - (Renderer->TextureManager.Cache.PageCount / 8);
    if (Renderer->TextureManager.Cache.UsedPageCount > PageUsageLimit)
    {
        TimedBlock(Platform.Profiler, "Mip discard");

        texture_manager* Manager = &Renderer->TextureManager;
        for (u32 TextureIndex = 0; TextureIndex < Manager->TextureCount; TextureIndex++)
        {
            renderer_texture* Texture = Manager->Textures + TextureIndex;
            if (!(Texture->Flags & TextureFlag_PersistentMemory) && Texture->MipResidencyMask)
            {
                constexpr u32 AlwaysKeptMips = 0xFF;
                u32 LastMipAccess = SetBitsBelowHighInclusive(Texture->LastMipAccess);

                u32 UsedMips = Texture->MipResidencyMask & (LastMipAccess | AlwaysKeptMips);
                u32 DiscardMips = (~UsedMips) & Texture->MipResidencyMask;
                if (DiscardMips)
                {
                    // TODO(boti): We have a bunch of these asserts in the codebase, 
                    // should we just make it a rule that streamed textures can't be arrays?
                    if (Texture->Info.ArrayCount != 1)
                    {
                        UnimplementedCodePath;
                    }

                    u32 CurrentMipCount = CountSetBits(Texture->MipResidencyMask);
                    u32 MipCountToDiscard = CountSetBits(DiscardMips);
                    if (CurrentMipCount == MipCountToDiscard)
                    {
                        renderer_texture* Placeholder = GetTexture(Manager, Texture->PlaceholderID);
                        umm DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize;
                        if ((Frame->StagingBuffer.At + DescriptorSize) <= Frame->StagingBuffer.Size)
                        {
                            VkDescriptorImageInfo DescriptorImage = 
                            {
                                .sampler = VK_NULL_HANDLE,
                                .imageView = Placeholder->ViewHandle,
                                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                            };
                            VkDescriptorGetInfoEXT DescriptorInfo = 
                            {
                                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                                .pNext = nullptr,
                                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                .data = { .pSampledImage = &DescriptorImage },
                            };
                            vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Frame->StagingBuffer.Base, Frame->StagingBuffer.At));
                                
                            umm DescriptorOffset = Manager->TextureTableOffset + TextureIndex*DescriptorSize;
                            VkBufferCopy DescriptorCopy = 
                            {
                                .srcOffset = Frame->StagingBuffer.At,
                                .dstOffset = DescriptorOffset,
                                .size = DescriptorSize,
                            };
                            vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Renderer->TextureManager.DescriptorBuffer, 1, &DescriptorCopy);
                                
                            Frame->StagingBuffer.At += DescriptorSize;
                                
                            MarkPagesAsFree(&Manager->Cache, Texture->PageIndex, Texture->PageCount);
                            Texture->PageIndex = 0;
                            Texture->PageCount = 0;
                            Texture->MipResidencyMask = 0;
                            PushDeletionEntry(&Renderer->DeletionQueue, Frame->FrameID, Texture->ImageHandle);
                            PushDeletionEntry(&Renderer->DeletionQueue, Frame->FrameID, Texture->ViewHandle);
                            Texture->ImageHandle = VK_NULL_HANDLE;
                            Texture->ViewHandle = VK_NULL_HANDLE;
                        }
                        else
                        {
                            UnhandledError("Out of staging memory (for texture descriptor)");
                        }
                    }
                    else
                    {
                        // NOTE(boti): Because we're always discarding a contiguous range we can just use the bit count to iterate
                        texture_info Info = 
                        {
                            .Extent = 
                            {
                                Max(Texture->Info.Extent.X >> MipCountToDiscard, 1u),
                                Max(Texture->Info.Extent.Y >> MipCountToDiscard, 1u),
                                1,
                            },
                            .MipCount = CurrentMipCount - MipCountToDiscard,
                            .ArrayCount = Texture->Info.ArrayCount,
                            .Format = Texture->Info.Format,
                            .Swizzle = Texture->Info.Swizzle,
                        };

                        VkImageCreateInfo ImageInfo = TextureInfoToVulkan(Info);
                        VkImage ImageHandle = VK_NULL_HANDLE;
                        VkResult ErrorCode = vkCreateImage(VK.Device, &ImageInfo, nullptr, &ImageHandle);
                        if (ErrorCode == VK_SUCCESS)
                        {
                            umm PageIndex, PageCount;
                            if (AllocateImage(&Manager->Cache, ImageHandle, &PageIndex, &PageCount))
                            {
                                VkImageViewCreateInfo ViewInfo = 
                                {
                                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                    .pNext = nullptr,
                                    .flags = 0,
                                    .image = ImageHandle,
                                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                    .format = ImageInfo.format,
                                    .components = SwizzleToVulkan(Info.Swizzle),
                                    .subresourceRange = 
                                    {
                                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                        .baseMipLevel = 0,
                                        .levelCount = VK_REMAINING_MIP_LEVELS,
                                        .baseArrayLayer = 0,
                                        .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                    },
                                };
                                VkImageView ViewHandle = VK_NULL_HANDLE;
                                ErrorCode = vkCreateImageView(VK.Device, &ViewInfo, nullptr, &ViewHandle);
                                if (ErrorCode == VK_SUCCESS)
                                {
                                    VkImageMemoryBarrier2 BeginBarriers[] = 
                                    {
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
                                            .image = ImageHandle,
                                            .subresourceRange = 
                                            {
                                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                .baseMipLevel = 0,
                                                .levelCount = VK_REMAINING_MIP_LEVELS,
                                                .baseArrayLayer = 0,
                                                .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                            },
                                        },
                                        {
                                            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                            .pNext = nullptr,
                                            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                                            .srcAccessMask = VK_ACCESS_2_NONE,
                                            .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                            .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                            .image = Texture->ImageHandle,
                                            .subresourceRange = 
                                            {
                                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                .baseMipLevel = 0,
                                                .levelCount = VK_REMAINING_MIP_LEVELS,
                                                .baseArrayLayer = 0,
                                                .layerCount = VK_REMAINING_ARRAY_LAYERS,
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

                                    vkCmdPipelineBarrier2(UploadCB, &BeginDependency);

                                    constexpr u32 MaxCopyCount = 32;
                                    VkImageCopy CopyRegions[MaxCopyCount];
                                    for (u32 DstMipIndex = 0; DstMipIndex < Info.MipCount; DstMipIndex++)
                                    {
                                        u32 SrcMipIndex = DstMipIndex + MipCountToDiscard;
                                        CopyRegions[DstMipIndex] = 
                                        {
                                            .srcSubresource = 
                                            {
                                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                .mipLevel = SrcMipIndex,
                                                .baseArrayLayer = 0,
                                                .layerCount = 1,
                                            },
                                            .srcOffset = { 0, 0, 0 },
                                            .dstSubresource = 
                                            {
                                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                .mipLevel = DstMipIndex,
                                                .baseArrayLayer = 0,
                                                .layerCount = 1,
                                            },
                                            .dstOffset = { 0, 0, 0 },
                                            .extent = 
                                            {
                                                .width = Max(Info.Extent.X >> DstMipIndex, 1u),
                                                .height = Max(Info.Extent.Y >> DstMipIndex, 1u),
                                                .depth = 1,
                                            },
                                        };
                                    }
                                    
                                    vkCmdCopyImage(UploadCB, 
                                                   Texture->ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                   ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   Info.MipCount, CopyRegions);

                                    VkImageMemoryBarrier2 EndBarrier = 
                                    {
                                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                        .pNext = nullptr,
                                        .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                                        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                        .image = ImageHandle,
                                        .subresourceRange = 
                                        {
                                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                            .baseMipLevel = 0,
                                            .levelCount = VK_REMAINING_MIP_LEVELS,
                                            .baseArrayLayer = 0,
                                            .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                        },
                                    };
                                    PushBeginBarrier(&FrameStages[FrameStage_Prepass], &EndBarrier);

                                    // TODO(boti): Deduplicate descriptor creation code
                                    umm DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize;
                                    if ((Frame->StagingBuffer.At + DescriptorSize) <= Frame->StagingBuffer.Size)
                                    {
                                        VkDescriptorImageInfo DescriptorImage = 
                                        {
                                            .sampler = VK_NULL_HANDLE,
                                            .imageView = ViewHandle,
                                            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                                        };
                                        VkDescriptorGetInfoEXT DescriptorInfo = 
                                        {
                                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                                            .pNext = nullptr,
                                            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                            .data = { .pSampledImage = &DescriptorImage },
                                        };
                                        vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Frame->StagingBuffer.Base, Frame->StagingBuffer.At));
                                
                                        umm DescriptorOffset = Manager->TextureTableOffset + TextureIndex*DescriptorSize;
                                        VkBufferCopy DescriptorCopy = 
                                        {
                                            .srcOffset = Frame->StagingBuffer.At,
                                            .dstOffset = DescriptorOffset,
                                            .size = DescriptorSize,
                                        };
                                        vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Renderer->TextureManager.DescriptorBuffer, 1, &DescriptorCopy);
                                        Frame->StagingBuffer.At += DescriptorSize;
                                
                                        MarkPagesAsFree(&Manager->Cache, Texture->PageIndex, Texture->PageCount);

                                        Texture->PageIndex = PageIndex;
                                        Texture->PageCount = PageCount;
                                        Texture->MipResidencyMask = UsedMips;
                                        Texture->Info = Info;
                                        PushDeletionEntry(&Renderer->DeletionQueue, Frame->FrameID, Texture->ImageHandle);
                                        PushDeletionEntry(&Renderer->DeletionQueue, Frame->FrameID, Texture->ViewHandle);
                                        Texture->ImageHandle = ImageHandle;
                                        Texture->ViewHandle = ViewHandle;
                                    }
                                    else
                                    {
                                        UnhandledError("Out of staging memory (for texture descriptor)");
                                    }
                                }
                                else
                                {
                                    UnhandledError("Failed to create image view when discarding unused mips");
                                }
                            }
                            else
                            {
                                // TODO(boti): Actual logging
                                Platform.DebugPrint("Failed to allocate memory when discarding unused mip levels\n");
                                
                            }
                        }
                        else
                        {
                            // TODO(boti): Actual logging
                            Platform.DebugPrint("Failed to create image when discarding unused mip levels\n");
                        }
                    }
                }
            }
        }
    }
    
    struct gpu_buffer_range
    {
        u64 Address;
        u64 ByteCount;
        u64 ByteOffset;
    };

    u64 PerFrameBufferAt = 0;
    auto PushPerFrame = [&PerFrameBufferAt, Renderer](u64 ByteCount, u64 Alignment = 256) -> gpu_buffer_range
    {
        gpu_buffer_range Result = {};

        u64 EffectiveAt = Alignment ? Align(PerFrameBufferAt, Alignment) : PerFrameBufferAt;
        if (EffectiveAt + ByteCount <= Renderer->PerFrameBufferSize)
        {
            Result.Address = Renderer->PerFrameBufferAddress + EffectiveAt;
            Result.ByteCount = ByteCount;
            Result.ByteOffset = EffectiveAt;

            PerFrameBufferAt = EffectiveAt + ByteCount;
        }
        else
        {
            // TODO(boti): dynamic allocation
            UnimplementedCodePath;
        }

        return(Result);
    };

    u32 TileCountX = CeilDiv(Frame->RenderExtent.X, R_TileSizeX);
    u32 TileCountY = CeilDiv(Frame->RenderExtent.Y, R_TileSizeY);
    Frame->Uniforms.TileCount = { TileCountX, TileCountY };

    gpu_buffer_range TileBufferRange = PushPerFrame(TileCountX * TileCountY * sizeof(screen_tile));
    Frame->Uniforms.TileBufferAddress = TileBufferRange.Address;

    frustum CascadeFrustums[R_MaxShadowCascadeCount];
    SetupSceneRendering(Frame, CascadeFrustums);

    //
    // Process commands
    //
    draw_list PrimaryDrawList = {};
    draw_list CascadeDrawLists[R_MaxShadowCascadeCount] = {};
    draw_list* ShadowDrawLists = PushArray(Frame->Arena, MemPush_Clear, draw_list, 6 * R_MaxShadowCount);
    u32 DrawListCount = 1 + R_MaxShadowCascadeCount;

    u32 ShadowCount = 0;
    frustum* ShadowFrustums = PushArray(Frame->Arena, 0, frustum, R_MaxShadowCount);

    VkViewport PrimaryViewport =
    {
        0.0f, 0.0f,
        (f32)Frame->RenderExtent.X, (f32)Frame->RenderExtent.Y,
        0.0f, 1.0f,
    };
    VkRect2D PrimaryScissor = { { 0, 0 }, { Frame->RenderExtent.X, Frame->RenderExtent.Y } };

    VkViewport OutputViewport =
    {
        0.0f, 0.0f,
        (f32)Frame->OutputExtent.X, (f32)Frame->OutputExtent.Y,
        0.0f, 1.0f,
    };
    VkRect2D OutputScissor = { { 0, 0 }, { Frame->OutputExtent.X, Frame->OutputExtent.Y } };

    f32 OutputAspect = OutputViewport.width / OutputViewport.height;

    VkViewport BlitViewport = {};
    BlitViewport.minDepth = 0.0f;
    BlitViewport.maxDepth = 1.0f;
    if (AspectRatio < OutputAspect)
    {
        BlitViewport.width = Round(AspectRatio * Frame->OutputExtent.Y);
        BlitViewport.height = Frame->OutputExtent.Y;
        BlitViewport.x = Floor(0.5f * (Frame->OutputExtent.X - BlitViewport.width));
        BlitViewport.y = 0.0f;
    }
    else
    {
        BlitViewport.width = Frame->OutputExtent.X;
        BlitViewport.height = Round((1.0f / AspectRatio) * Frame->OutputExtent.X);
        BlitViewport.x = 0.0f;
        BlitViewport.y = Floor(0.5f * (Frame->OutputExtent.Y - BlitViewport.height));
    }
    VkRect2D BlitScissor = 
    { 
        { (s32)BlitViewport.x, (s32)BlitViewport.y }, 
        { (u32)BlitViewport.width, (u32)BlitViewport.height }
    };

    VkCommandBuffer SkinningCB = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_Secondary, Pipeline_None);
    vkCmdBindPipeline(SkinningCB, VK_PIPELINE_BIND_POINT_COMPUTE, Renderer->Pipelines[Pipeline_Skinning].Pipeline);

    VkCommandBuffer ParticleCB = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_Secondary, Pipeline_Quad);
    vkCmdBeginDebugUtilsLabelEXT(ParticleCB, "Particles");
    vkCmdSetViewport(ParticleCB, 0, 1, &PrimaryViewport);
    vkCmdSetScissor(ParticleCB, 0, 1, &PrimaryScissor);
    vkCmdBindPipeline(ParticleCB, VK_PIPELINE_BIND_POINT_GRAPHICS, Renderer->Pipelines[Pipeline_Quad].Pipeline);
        
    VkCommandBuffer Widget3DCB = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_Secondary, Pipeline_Gizmo);
    {
        vkCmdBeginDebugUtilsLabelEXT(Widget3DCB, "3D GUI");
        vkCmdSetViewport(Widget3DCB, 0, 1, &BlitViewport);
        vkCmdSetScissor(Widget3DCB, 0, 1, &BlitScissor);

        vkCmdBindPipeline(Widget3DCB, VK_PIPELINE_BIND_POINT_GRAPHICS, Renderer->Pipelines[Pipeline_Gizmo].Pipeline);
        vkCmdBindIndexBuffer(Widget3DCB, Renderer->GeometryBuffer.IndexMemory.Buffers[0], 0, VK_INDEX_TYPE_UINT32);
    }

    VkCommandBuffer GuiCB = BeginCommandBuffer(Renderer, Frame->FrameID, BeginCB_Secondary, Pipeline_UI);
    {
        vkCmdBeginDebugUtilsLabelEXT(GuiCB, "2D GUI");
        vkCmdSetViewport(GuiCB, 0, 1, &OutputViewport);
        vkCmdSetScissor(GuiCB, 0, 1, &OutputScissor);

        vkCmdBindPipeline(GuiCB, VK_PIPELINE_BIND_POINT_GRAPHICS, Renderer->Pipelines[Pipeline_UI].Pipeline);
    }

    {
        memcpy(Frame->Uniforms.DrawGroupCounts, Frame->DrawGroupDrawCounts, sizeof(Frame->DrawGroupDrawCounts));
        memset(Frame->Uniforms.DrawGroupOffsets, 0, sizeof(Frame->Uniforms.DrawGroupOffsets));
        u32 DrawGroupOffsets[DrawGroup_Count] = {};
        for (u32 GroupIndex = 1; GroupIndex < DrawGroup_Count; GroupIndex++)
        {
            u32 Offset = DrawGroupOffsets[GroupIndex - 1] + Frame->DrawGroupDrawCounts[GroupIndex - 1];
            DrawGroupOffsets[GroupIndex] = Offset;
            Frame->Uniforms.DrawGroupOffsets[GroupIndex] = Offset;
        }
        u32 InstanceCount = 0;
        for (u32 GroupIndex = 0; GroupIndex < DrawGroup_Count; GroupIndex++)
        {
            InstanceCount += Frame->DrawGroupDrawCounts[GroupIndex];
        }
        instance_data*                  Instances           = PushArray(Frame->Arena, 0, instance_data, InstanceCount);
        mmbox*                          BoundingBoxes       = PushArray(Frame->Arena, 0, mmbox, InstanceCount);
        m4*                             Transforms          = PushArray(Frame->Arena, 0, m4, InstanceCount);
        VkDrawIndexedIndirectCommand*   IndirectCommands    = PushArray(Frame->Arena, 0, VkDrawIndexedIndirectCommand, InstanceCount);

        umm LightDataOffset = 0;
        u32 LightDataAt = 0;
        light* LightData = nullptr;
        {
            LightDataOffset = Align(Frame->StagingBuffer.At, alignof(light));
            umm LightDataSize = Frame->LightCount * sizeof(light);
            if (LightDataOffset + LightDataSize <= Frame->StagingBuffer.Size)
            {
                LightData = (light*)OffsetPtr(Frame->StagingBuffer.Base, LightDataOffset);
                Frame->StagingBuffer.At = LightDataOffset + LightDataSize;
            }
            else
            {
                UnhandledError("Not enough memory in staging buffer for light data");
            }
        }

        {
            TimedBlock(Platform.Profiler, "ProcessRenderCommands");
            for (u32 CommandIndex = 0; CommandIndex < Frame->CommandCount; CommandIndex++)
            {
                render_command* Command = Frame->Commands + CommandIndex;
                switch (Command->Type)
                {
                    case RenderCommand_Transfer:
                    {
                        transfer_op* Op = &Command->Transfer;
                        switch (Op->Type)
                        {
                            case TransferOp_Texture:
                            {
                                if (!IsValid(Op->Texture.TargetID))
                                {
                                    break;
                                }

                                texture_info* Info = &Op->Texture.Info;
                                texture_subresource_range* Range = &Op->Texture.SubresourceRange;
                                if (Info->Extent.Z > 1)
                                {
                                    UnimplementedCodePath;
                                }

                                format_info ByteRate = FormatInfoTable[Info->Format];
                                umm TotalSize = GetMipChainSize(Info->Extent.X, Info->Extent.Y, Info->MipCount, Info->ArrayCount, ByteRate);

                                u32 CopyCount = Info->MipCount * Info->ArrayCount;
                                VkBufferImageCopy* Copies = PushArray(Frame->Arena, 0, VkBufferImageCopy, CopyCount);
                                VkBufferImageCopy* CopyAt = Copies;

                                umm Offset = Command->StagingBufferAt;
                                for (u32 ArrayIndex = 0; ArrayIndex < Info->ArrayCount; ArrayIndex++)
                                {
                                    for (u32 MipIndex = 0; MipIndex < Info->MipCount; MipIndex++)
                                    {
                                        v2u Extent = 
                                        {
                                            Max(Info->Extent.X >> MipIndex, 1u),
                                            Max(Info->Extent.Y >> MipIndex, 1u),
                                        };

                                        u32 RowLength = 0;
                                        u32 ImageHeight = 0;
                                        u64 TexelCount;
                                        if (ByteRate.Flags & FormatFlag_BlockCompressed)
                                        {
                                            RowLength = Align(Extent.X, 4u);
                                            ImageHeight = Align(Extent.Y, 4u);

                                            TexelCount = (u64)RowLength * ImageHeight;
                                        }
                                        else
                                        {
                                            TexelCount = Extent.X * Extent.Y;
                                        }
                                        u64 MipSize = TexelCount * ByteRate.ByteRateNumerator / ByteRate.ByteRateDenominator;

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
                                        CopyAt->imageExtent = { Extent.X, Extent.Y, 1 },

                                        Offset += MipSize;
                                        CopyAt++;
                                    }
                                }

                                b32 IsAllocated = false;
                                renderer_texture* Texture = GetTexture(&Renderer->TextureManager, Op->Texture.TargetID);
                                Assert(Texture);
                                if (Texture->ImageHandle == VK_NULL_HANDLE)
                                {
                                    IsAllocated = AllocateTexture(&Renderer->TextureManager, Op->Texture.TargetID, *Info);
                                }
                                else if (AreTextureInfosSameFormat(Op->Texture.Info, Texture->Info))
                                {
                                    // NOTE(boti): Nothing to do here, although in the future we might want to allocate memory here
                                    // (currently that's always already done in this case)
                                    IsAllocated = true;
                                }
                                else
                                {
                                    // TODO(boti): Move the deletion entry push to the texture manager
                                    VkImage OldImageHandle = Texture->ImageHandle;
                                    VkImageView OldViewHandle = Texture->ViewHandle;
                                    IsAllocated = AllocateTexture(&Renderer->TextureManager, Op->Texture.TargetID, *Info);
                                    if (IsAllocated)
                                    {
                                        PushDeletionEntry(&Renderer->DeletionQueue, Frame->FrameID, OldImageHandle);
                                        PushDeletionEntry(&Renderer->DeletionQueue, Frame->FrameID, OldViewHandle);
                                    }
                                }

                                if (IsAllocated)
                                {
                                    umm DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize;
                                    if ((Frame->StagingBuffer.At + DescriptorSize) <= Frame->StagingBuffer.Size)
                                    {
                                        VkDescriptorImageInfo DescriptorImage = 
                                        {
                                            .sampler = VK_NULL_HANDLE,
                                            .imageView = Texture->ViewHandle,
                                            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                                        };
                                        VkDescriptorGetInfoEXT DescriptorInfo = 
                                        {
                                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                                            .pNext = nullptr,
                                            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                            .data = { .pSampledImage = &DescriptorImage },
                                        };
                                        vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Frame->StagingBuffer.Base, Frame->StagingBuffer.At));
                                
                                        umm DescriptorOffset = Renderer->TextureManager.TextureTableOffset + Op->Texture.TargetID.Value*DescriptorSize; // TODO(boti): standardize this
                                        VkBufferCopy DescriptorCopy = 
                                        {
                                            .srcOffset = Frame->StagingBuffer.At,
                                            .dstOffset = DescriptorOffset,
                                            .size = DescriptorSize,
                                        };
                                        vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Renderer->TextureManager.DescriptorBuffer, 1, &DescriptorCopy);
                                
                                        Frame->StagingBuffer.At += DescriptorSize;
                                
                                        u32 MipBucket;
                                        BitScanReverse(&MipBucket, Max(Info->Extent.X, Info->Extent.Y));
                                        u32 MipMask = (1 << (MipBucket + 1)) - 1;
                                        Texture->MipResidencyMask = MipMask;
                                        Texture->Info = Op->Texture.Info;
                                    }
                                    else
                                    {
                                        UnimplementedCodePath;
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
                                        .image = Texture->ImageHandle,
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
                                    vkCmdPipelineBarrier2(UploadCB, &BeginDependency);
                                    vkCmdCopyBufferToImage(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Texture->ImageHandle, 
                                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
                                        .image = Texture->ImageHandle,
                                        .subresourceRange = 
                                        {
                                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                            .baseMipLevel = 0,
                                            .levelCount = VK_REMAINING_MIP_LEVELS,
                                            .baseArrayLayer = 0,
                                            .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                        },
                                    };
                                    PushBeginBarrier(&FrameStages[FrameStage_Prepass], &EndBarrier);
                                }
                                else
                                {
                                    // TODO(boti): Logging
                                    //UnimplementedCodePath;
                                }
                            } break;

                            case TransferOp_Geometry:
                            {
                                umm VertexByteCount = Op->Geometry.Dest.VertexBlock->Count * sizeof(vertex);
                                umm VertexByteOffset = Op->Geometry.Dest.VertexBlock->Offset * sizeof(vertex);
                                umm IndexByteCount = Op->Geometry.Dest.IndexBlock->Count * sizeof(vert_index);
                                umm IndexByteOffset = Op->Geometry.Dest.IndexBlock->Offset * sizeof(vert_index);

                                u32 BlockIndex = VertexByteOffset / Renderer->GeometryBuffer.VertexMemory.GPUBlockSize;
                                VertexByteOffset = VertexByteOffset % Renderer->GeometryBuffer.VertexMemory.GPUBlockSize;
                                VkBuffer VertexBuffer = Renderer->GeometryBuffer.VertexMemory.Buffers[BlockIndex];
                                VkBuffer IndexBuffer = Renderer->GeometryBuffer.IndexMemory.Buffers[0];

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
                                    vkCmdPipelineBarrier2(UploadCB, &Dependency);

                                    VkBufferCopy Copy = 
                                    {
                                        .srcOffset = Command->StagingBufferAt,
                                        .dstOffset = VertexByteOffset,
                                        .size = VertexByteCount,
                                    };
                                    vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], VertexBuffer, 1, &Copy);

                                    Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                                    Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                                    Barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                                    Barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                                    PushBeginBarrier(&FrameStages[FrameStage_Skinning], &Barrier);
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
                                    vkCmdPipelineBarrier2(UploadCB, &Dependency);
                                    VkBufferCopy Copy = 
                                    {
                                        .srcOffset = Command->StagingBufferAt + VertexByteCount,
                                        .dstOffset = IndexByteOffset,
                                        .size = IndexByteCount,
                                    };
                                    vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], IndexBuffer, 1, &Copy);

                                    Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                                    Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                                    Barrier.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                                    Barrier.dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT|VK_ACCESS_2_SHADER_READ_BIT;
                                    PushBeginBarrier(&FrameStages[FrameStage_Prepass], &Barrier);
                                }
                            } break;
                            InvalidDefaultCase;
                        }
                    } break;

                    case RenderCommand_Light:
                    {
                        light* Light = &Command->Light;

                        f32 L = GetLuminance(Light->E);
                        f32 R = Sqrt(Max(L / R_LuminanceThreshold, 0.0f));

                        if (LightDataAt <= R_MaxLightCount /*&& IntersectFrustumSphere(&Frame->CameraFrustum, Light->P, R)*/)
                        {
                            Frame->Uniforms.LightCount++;
                            u32 ShadowIndex = 0xFFFFFFFFu;
                            if ((Light->Flags & LightFlag_ShadowCaster) && ShadowCount <= R_MaxShadowCount)
                            {
                                ShadowIndex = ShadowCount++;
                                point_shadow_data* Shadow = Frame->Uniforms.PointShadows + ShadowIndex;

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
                                Shadow->Near = n;
                                Shadow->Far = f;
                                DrawListCount += 6;
                                for (u32 LayerIndex = 0; LayerIndex < CubeLayer_Count; LayerIndex++)
                                {
                                    m3 M = GlobalCubeFaceBases[LayerIndex];
                                    m4 InverseView = M4(M.X.X, M.Y.X, M.Z.X, Light->P.X,
                                                        M.X.Y, M.Y.Y, M.Z.Y, Light->P.Y,
                                                        M.X.Z, M.Y.Z, M.Z.Z, Light->P.Z,
                                                        0.0f, 0.0f, 0.0f ,1.0f);
                                    m4 View = M4(M.X.X, M.X.Y, M.X.Z, -Dot(M.X, Light->P),
                                                 M.Y.X, M.Y.Y, M.Y.Z, -Dot(M.Y, Light->P),
                                                 M.Z.X, M.Z.Y, M.Z.Z, -Dot(M.Z, Light->P),
                                                 0.0f, 0.0f, 0.0f, 1.0f);
                                    m4 ViewProjection = Projection * View;
                                    Shadow->ViewProjections[LayerIndex] = ViewProjection;

                                    frustum ClipSpaceFrustum = GetClipSpaceFrustum();
                                    frustum* Frustum = &ShadowFrustums[6 * ShadowIndex + LayerIndex];
                                    for (u32 PlaneIndex = 0; PlaneIndex < CountOf(Frustum->Planes); PlaneIndex++)
                                    {
                                        // TODO(boti): Try mul w/ ViewProjection after porting
                                        Frustum->Planes[PlaneIndex] = ClipSpaceFrustum.Planes[PlaneIndex] * Projection * View;
                                    }
                                }
                            }

                            v3 P = TransformPoint(Frame->Uniforms.ViewTransform, Light->P);
                            LightData[LightDataAt++] =
                            {
                                .P = P,
                                .ShadowIndex = ShadowIndex,
                                .E = Light->E,
                                .Flags = Light->Flags,
                            };
                        }
                    } break;

                    case RenderCommand_Draw:
                    {
                        draw_command* Draw = &Command->Draw;

                        u32 ID = DrawGroupOffsets[Draw->Group]++;
                        BoundingBoxes[ID] = Draw->BoundingBox;
                        Transforms[ID] = Draw->Transform;

                        umm SourceByteOffset = Draw->Geometry.VertexBlock->Offset * sizeof(vertex);
                        u64 SourceVertexBufferAddress = GetDeviceAddress(&Renderer->GeometryBuffer.VertexMemory, SourceByteOffset);
                        u64 DrawVertexBufferAddress = SourceVertexBufferAddress;

                        if (Draw->Flags & Draw_Skinned)
                        {
                            u32 VertexCount = Draw->Geometry.VertexBlock->Count;
                            gpu_buffer_range VertexBufferRange = PushPerFrame(VertexCount * sizeof(vertex), 16);
                            DrawVertexBufferAddress = VertexBufferRange.Address;
                        
                            struct skinning_constants
                            {
                                VkDeviceAddress SkinAddress;
                                VkDeviceAddress SrcAddress;
                                VkDeviceAddress DstAddress;
                                u32 VertexCount;
                            } Push;
                            Push.SkinAddress = Renderer->BARBufferAddresses[Frame->FrameID] + Command->BARBufferAt;
                            Push.SrcAddress = SourceVertexBufferAddress;
                            Push.DstAddress = VertexBufferRange.Address;
                            Push.VertexCount = VertexCount;
                            vkCmdPushConstants(SkinningCB, Renderer->Pipelines[Pipeline_Skinning].Layout, VK_SHADER_STAGE_ALL,
                                               0, sizeof(Push), &Push);
                            vkCmdDispatch(SkinningCB, CeilDiv(VertexCount, Skin_GroupSizeX), 1, 1);

                            // HACK(boti): We'd either need to generate the bboxes when skinning (which requires GPU culling),
                            // or have some guarantee that the bbox from the mesh is conservative enough to accomodate
                            // all animations, neither of which we currently have,
                            // so instead we massively overestimate the bbox, essentially turning frustum culling off for skinned meshes.
                            BoundingBoxes[ID] = 
                            {
                                .Min = { -1000000.0f, -1000000.0f, -1000000.0f },
                                .Max = { +1000000.0f, +1000000.0f, +1000000.0f }, 
                            };
                        }

                        IndirectCommands[ID] = 
                        {
                            .indexCount = Draw->Geometry.IndexBlock->Count,
                            .instanceCount = 1,
                            .firstIndex = Draw->Geometry.IndexBlock->Offset,
                            .vertexOffset = 0,
                            .firstInstance = ID,
                        };

                        Instances[ID] = 
                        {
                            .Transform = Draw->Transform,
                            .VertexBufferAddress = DrawVertexBufferAddress,
                            .Material = Draw->Material,
                        };
                    } break;
                    case RenderCommand_ParticleBatch:
                    {
                        struct
                        {
                            VkDeviceAddress ParticleAddress;
                            billboard_mode Mode;
                            renderer_texture_id TextureID;
                        } Push;
                        Push.ParticleAddress = Renderer->BARBufferAddresses[Frame->FrameID] + Command->BARBufferAt;
                        Push.Mode = Command->ParticleBatch.Mode;
                        Push.TextureID = Frame->ParticleTextureID;
                        vkCmdPushConstants(ParticleCB, Renderer->Pipelines[Pipeline_Quad].Layout, VK_SHADER_STAGE_ALL, 
                                           0, sizeof(Push), &Push);
                        vkCmdDraw(ParticleCB, 6 * Command->ParticleBatch.Count, 1, 0, 0);
                    } break;
                    case RenderCommand_Widget3D:
                    {
                        geometry_buffer_allocation* Geometry = &Command->Widget3D.Geometry;
                        umm VertexByteOffset = Geometry->VertexBlock->Offset * sizeof(vertex);

                        struct
                        {
                            m4 Transform;
                            u64 VertexBufferAddress;
                            rgba8 Color;
                        } Push;
                        Push.Transform = Command->Widget3D.Transform;
                        Push.VertexBufferAddress = GetDeviceAddress(&Renderer->GeometryBuffer.VertexMemory, VertexByteOffset);
                        Push.Color = Command->Widget3D.Color;
                        vkCmdPushConstants(Widget3DCB, Renderer->Pipelines[Pipeline_Gizmo].Layout, VK_SHADER_STAGE_ALL,
                                           0, sizeof(Push), &Push);
                        vkCmdDrawIndexed(Widget3DCB, Geometry->IndexBlock->Count, 1, Geometry->IndexBlock->Offset, 0, 0);
                    } break;
                    case RenderCommand_Batch2D:
                    {
                        struct
                        {
                            m4 Transform;
                            u64 VertexBufferAddress;
                            renderer_texture_id TextureID;
                        } Push;
                        Push.Transform = M4(
                            2.0f / Frame->OutputExtent.X, 0.0f, 0.0f, -1.0f,
                            0.0f, 2.0f / Frame->OutputExtent.Y, 0.0f, -1.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f);
                        Push.VertexBufferAddress    = Renderer->BARBufferAddresses[Frame->FrameID] + Command->BARBufferAt;
                        Push.TextureID              = Frame->ImmediateTextureID;
                        vkCmdPushConstants(GuiCB, Renderer->Pipelines[Pipeline_UI].Layout, VK_SHADER_STAGE_ALL, 0, sizeof(Push), &Push);
                        vkCmdDraw(GuiCB, Command->Batch2D.VertexCount, 1, 0, 0);
                    } break;
                    InvalidDefaultCase;
                }
            }
        }

        {
            TimedBlock(Platform.Profiler, "Upload");

            // Upload lights
            gpu_buffer_range LightBufferRange = PushPerFrame(Frame->Uniforms.LightCount * sizeof(light));
            Frame->Uniforms.LightBufferAddress = LightBufferRange.Address;
            if (LightBufferRange.ByteCount)
            {
                VkBufferCopy LightBufferCopy = 
                {
                    .srcOffset = LightDataOffset,
                    .dstOffset = LightBufferRange.ByteOffset,
                    .size = LightBufferRange.ByteCount,
                };
                vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Renderer->PerFrameBuffer, 1, &LightBufferCopy);
            }

            // Upload instance data
            gpu_buffer_range InstanceBufferRange = PushPerFrame(InstanceCount * sizeof(instance_data));
            Frame->Uniforms.InstanceBufferAddress = InstanceBufferRange.Address;
            if (InstanceBufferRange.ByteCount)
            {
                Frame->StagingBuffer.At = Align(Frame->StagingBuffer.At, alignof(instance_data));
                if (Frame->StagingBuffer.At + InstanceBufferRange.ByteCount <= Frame->StagingBuffer.Size)
                {
                    memcpy(OffsetPtr(Frame->StagingBuffer.Base, Frame->StagingBuffer.At), Instances, InstanceBufferRange.ByteCount);
                    VkBufferCopy Copy = 
                    {
                        .srcOffset = Frame->StagingBuffer.At,
                        .dstOffset = InstanceBufferRange.ByteOffset,
                        .size = InstanceBufferRange.ByteCount,
                    };
                    vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Renderer->PerFrameBuffer, 1, &Copy);

                    Frame->StagingBuffer.At += InstanceBufferRange.ByteCount;
                }
                else
                {
                    UnhandledError("Out of staging memory when uploading instance data");
                }
            }

            // Upload (global) draw data
            gpu_buffer_range GlobalDrawBufferRange = PushPerFrame(InstanceCount * sizeof(VkDrawIndexedIndirectCommand));
            Frame->Uniforms.DrawBufferAddress = GlobalDrawBufferRange.Address;
            if (GlobalDrawBufferRange.ByteCount)
            {
                Frame->StagingBuffer.At = Align(Frame->StagingBuffer.At, alignof(VkDrawIndexedIndirectCommand));
                if (Frame->StagingBuffer.At + GlobalDrawBufferRange.ByteCount <= Frame->StagingBuffer.Size)
                {
                    memcpy(OffsetPtr(Frame->StagingBuffer.Base, Frame->StagingBuffer.At), IndirectCommands, GlobalDrawBufferRange.ByteCount);
                    VkBufferCopy Copy = 
                    {
                        .srcOffset = Frame->StagingBuffer.At,
                        .dstOffset = GlobalDrawBufferRange.ByteOffset,
                        .size = GlobalDrawBufferRange.ByteCount,
                    };
                    vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Renderer->PerFrameBuffer, 1, &Copy);

                    Frame->StagingBuffer.At += GlobalDrawBufferRange.ByteCount;
                }
                else
                {
                    UnhandledError("Out of staging memory when uploading draw data");
                }
            }
        }

        struct draw_list_work_params
        {
            draw_list*                      DrawList;
            VkDrawIndexedIndirectCommand*   CopyDst;
            umm                             CopyDstOffset;
            frustum*                        Frustum;
            
            // Scene info
            u32*                            DrawGroupOffsets;
            mmbox*                          BoundingBoxes;
            m4*                             Transforms;
            VkDrawIndexedIndirectCommand*   IndirectCommands;
            u32                             InstanceCount;
            
            // NOTE(boti): Filled by worker
            u32 TotalDrawCount;

            u32 Padding[14];
        };
        static_assert(sizeof(draw_list_work_params) % 64 == 0);

        draw_list_work_params* WorkParams = (draw_list_work_params*)PushSize_(Frame->Arena, MemPush_Clear, sizeof(draw_list_work_params) * DrawListCount, 64);
        auto FrustumCullDrawList = [](thread_context* ThreadContext, void* Params_)
        {
            TimedFunctionMT(Platform.Profiler, ThreadContext->ThreadID);

            draw_list_work_params* Params = (draw_list_work_params*)Params_;
            VkDrawIndexedIndirectCommand* At = Params->CopyDst;

            u32 CurrentGroupIndex = 0;
            for (u32 InstanceIndex = 0; InstanceIndex < Params->InstanceCount; InstanceIndex++)
            {
                while (InstanceIndex >= Params->DrawGroupOffsets[CurrentGroupIndex])
                {
                    CurrentGroupIndex++;
                }

                b32 IsVisible = true;
                if (Params->Frustum)
                {
                    IsVisible = IntersectFrustumBox(Params->Frustum, Params->BoundingBoxes[InstanceIndex], Params->Transforms[InstanceIndex]);
                }

                if (IsVisible)
                {
                    Params->TotalDrawCount++;
                    Params->DrawList->DrawGroupDrawCounts[CurrentGroupIndex]++;
                    *At++ = Params->IndirectCommands[InstanceIndex];
                }
            }
        };

        Frame->StagingBuffer.At = Align(Frame->StagingBuffer.At, alignof(VkDrawIndexedIndirectCommand));
        Assert(Frame->StagingBuffer.At <= Frame->StagingBuffer.Size);
        umm MaxMemorySizePerDrawList = InstanceCount * sizeof(VkDrawIndexedIndirectCommand);
        for (u32 DrawListIndex = 0; DrawListIndex < DrawListCount; DrawListIndex++)
        {
            draw_list_work_params* Params = WorkParams + DrawListIndex;
            Params->DrawGroupOffsets    = DrawGroupOffsets;
            Params->BoundingBoxes       = BoundingBoxes;
            Params->Transforms          = Transforms;
            Params->IndirectCommands    = IndirectCommands;
            Params->InstanceCount       = InstanceCount;

            if (DrawListIndex == 0)
            {
                Params->Frustum = &Frame->CameraFrustum;
                Params->DrawList = &PrimaryDrawList;
            }
            else if ((DrawListIndex - 1) < R_MaxShadowCascadeCount)
            {
                u32 Index = DrawListIndex - 1;
                Params->Frustum = CascadeFrustums + Index;
                Params->DrawList = CascadeDrawLists + Index;
            }
            else
            {
                u32 Index = (DrawListIndex - 1 - R_MaxShadowCascadeCount);
                Params->Frustum = ShadowFrustums + Index;
                Params->DrawList = ShadowDrawLists + Index;
            }

            if ((Frame->StagingBuffer.At + MaxMemorySizePerDrawList) > Frame->StagingBuffer.Size)
            {
                UnimplementedCodePath;
                break;
            }

            Params->CopyDstOffset   = Frame->StagingBuffer.At;
            Params->CopyDst         = (VkDrawIndexedIndirectCommand*)OffsetPtr(Frame->StagingBuffer.Base, Frame->StagingBuffer.At);
            Frame->StagingBuffer.At += MaxMemorySizePerDrawList;

            Platform.AddWorkEntry(Platform.Queue, FrustumCullDrawList, Params);
        }

        Platform.CompleteAllWork(Platform.Queue, ThreadContext);

        for (u32 DrawListIndex = 0; DrawListIndex < DrawListCount; DrawListIndex++)
        {
            draw_list_work_params* Params = WorkParams + DrawListIndex;
            gpu_buffer_range DrawBufferRange = PushPerFrame(Params->TotalDrawCount * sizeof(VkDrawIndexedIndirectCommand));
            if (DrawBufferRange.ByteCount)
            {
                Params->DrawList->DrawBuffer = Renderer->PerFrameBuffer;
                Params->DrawList->DrawBufferOffset = DrawBufferRange.ByteOffset;

                VkBufferCopy Copy =
                {
                    .srcOffset = Params->CopyDstOffset,
                    .dstOffset = DrawBufferRange.ByteOffset,
                    .size = DrawBufferRange.ByteCount,
                };

                vkCmdCopyBuffer(UploadCB, Renderer->StagingBuffers[Frame->FrameID], Params->DrawList->DrawBuffer, 1, &Copy);
            }
        }
    }
    vkCmdEndDebugUtilsLabelEXT(ParticleCB);
    EndCommandBuffer(ParticleCB);
    vkCmdEndDebugUtilsLabelEXT(Widget3DCB);
    EndCommandBuffer(Widget3DCB);
    vkCmdEndDebugUtilsLabelEXT(GuiCB);
    EndCommandBuffer(GuiCB);
    EndCommandBuffer(SkinningCB);

    // Clear mip feedback buffer
    {
        // NOTE(boti): This barrier is here because of a false positive,
        // currently we're also waiting for the previous frame render to finish w/ a semaphore
        VkMemoryBarrier2 BeginBarrier =
        {
            .sType          = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext          = nullptr,
            .srcStageMask   = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
            .srcAccessMask  = VK_ACCESS_2_TRANSFER_READ_BIT,
            .dstStageMask   = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
            .dstAccessMask  = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        };
        PushBeginBarrier(&FrameStages[FrameStage_Upload], &BeginBarrier);

        vkCmdFillBuffer(UploadCB, Renderer->MipFeedbackBuffer, 0, VK_WHOLE_SIZE, 0);
    }

    EndCommandBuffer(UploadCB);

    // Upload uniform data
    memcpy(Frame->UniformData, &Frame->Uniforms, sizeof(Frame->Uniforms));

    // Per-frame descriptor update
    {
        descriptor_write PerFrameWrites[] = 
        {
            {
                .Type = Descriptor_UniformBuffer,
                .Binding = Binding_PerFrame_Constants,
                .BaseIndex = 0,
                .Count = 1,
                .Buffers = { { Renderer->PerFrameUniformBuffers[Frame->FrameID], 0, KiB(64) } },
            },
            {
                .Type = Descriptor_SampledImage,
                .Binding = Binding_PerFrame_ParticleTexture,
                .BaseIndex = 0,
                .Count = 1,
                .Images = { { GetTexture(&Renderer->TextureManager, Frame->ParticleTextureID)->ViewHandle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },
            },
            {
                .Type = Descriptor_SampledImage,
                .Binding = Binding_PerFrame_TextureUI,
                .BaseIndex = 0,
                .Count = 1,
                .Images = { { GetTexture(&Renderer->TextureManager, Frame->ImmediateTextureID)->ViewHandle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },
            },
        };
        static_assert(CountOf(PerFrameWrites) == Binding_PerFrame_Count);
        UpdateDescriptorBuffer(CountOf(PerFrameWrites), PerFrameWrites, Renderer->SetLayouts[Set_PerFrame], Renderer->PerFrameResourceDescriptorMappings[Frame->FrameID]);
    }

    //
    // Frame
    //
    vkCmdResetQueryPool(PrepassCmd, Renderer->PerformanceQueryPools[Frame->FrameID], 0, 2 * (FrameStage_Count + 1));
    vkCmdWriteTimestamp2(PrepassCmd, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, Renderer->PerformanceQueryPools[Frame->FrameID], 2 * FrameStage_Count);

    //
    // Upload
    //
    BeginFrameStage(PrepassCmd, FrameStage_Upload, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
        vkCmdExecuteCommands(PrepassCmd, 1, &UploadCB);
    }
    EndFrameStage(PrepassCmd, FrameStage_Upload, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    //
    // Skinning
    //
    BeginFrameStage(PrepassCmd, FrameStage_Skinning, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
        vkCmdExecuteCommands(PrepassCmd, 1, &SkinningCB);

        // NOTE(boti): This isn't the clearest place to handle both the upload and skinning barriers,
        // the skinning doesn't overlap with the upload, so this is where that should happen
        VkMemoryBarrier2 EndBarrier =
        {
            .sType          = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext          = nullptr,
            .srcStageMask   = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask  = VK_ACCESS_2_SHADER_WRITE_BIT|VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask   = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask  = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT|VK_ACCESS_2_SHADER_READ_BIT|VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT,
        };
        PushBeginBarrier(&FrameStages[FrameStage_Prepass], &EndBarrier);
    }
    EndFrameStage(PrepassCmd, FrameStage_Skinning, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    //
    // Init time
    //

    // Environemnt BRDF Lut
    if (!Renderer->EnvironmentBRDFReady)
    {
        VkImageMemoryBarrier2 BeginBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Renderer->EnvironmentBRDFImage,
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

        pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_EnvironmentBRDF];
        vkCmdBindPipeline(PrepassCmd, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline.Pipeline);

        u32 SampleCount = 4096;
        vkCmdPushConstants(PrepassCmd, Pipeline.Layout, VK_SHADER_STAGE_ALL, 0, sizeof(SampleCount), &SampleCount);
        vkCmdDispatch(PrepassCmd,
                      CeilDiv(Renderer->EnvironmentBRDFResolution.X, EnvironmentBRDF_GroupSizeX),
                      CeilDiv(Renderer->EnvironmentBRDFResolution.Y, EnvironmentBRDF_GroupSizeY),
                      1);

        VkImageMemoryBarrier2 EndBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Renderer->EnvironmentBRDFImage,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_Shading], &EndBarrier);

        Renderer->EnvironmentBRDFReady = true;
    }

    vkCmdSetViewport(PrepassCmd, 0, 1, &PrimaryViewport);
    vkCmdSetScissor(PrepassCmd, 0, 1, &PrimaryScissor);

    //
    // Prepass
    //
    {
        VkImageMemoryBarrier2 BeginBarriers[] = 
        {
            // Visibility buffer
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                #if 1
                .srcStageMask           = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                #else
                .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask          = VK_ACCESS_2_NONE,
                #endif
                .dstStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = Renderer->VisibilityBuffer->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            },
            // Structure buffer
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                #if 1
                .srcStageMask           = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                #else
                .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask          = VK_ACCESS_2_NONE,
                #endif
                .dstStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = Renderer->StructureBuffer->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            },
            // Depth buffer
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                #if 1
                .srcStageMask           = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask          = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                #else
                .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask          = VK_ACCESS_2_NONE,
                #endif
                .dstStageMask           = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .dstAccessMask          = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout              = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = Renderer->DepthBuffer->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            },
        };
        for (u32 BarrierIndex = 0; BarrierIndex < CountOf(BeginBarriers); BarrierIndex++)
        {
            PushBeginBarrier(&FrameStages[FrameStage_Prepass], BeginBarriers + BarrierIndex);
        }
    }
    BeginFrameStage(PrepassCmd, FrameStage_Prepass, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
        VkRenderingAttachmentInfo ColorAttachments[] = 
        {
            // Visibility buffer
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = Renderer->VisibilityBuffer->View,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .color.uint32 = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00u, 0x00u } },
            },
            // Structure buffer
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = Renderer->StructureBuffer->View,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .color = { 0.0f, 0.0f, Frame->Uniforms.FarZ, 0.0f } },
            },
        };

        VkRenderingAttachmentInfo DepthAttachment = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Renderer->DepthBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .depthStencil = { 0.0f, 0 } },
        };

        VkRenderingInfo RenderingInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = { .offset = { 0, 0 }, .extent = { Frame->RenderExtent.X, Frame->RenderExtent.Y } },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = CountOf(ColorAttachments),
            .pColorAttachments = ColorAttachments,
            .pDepthAttachment = &DepthAttachment,
            .pStencilAttachment = nullptr,
        };

        vkCmdBeginRendering(PrepassCmd, &RenderingInfo);

        pipeline Pipelines[DrawGroup_Count] = 
        {
            [DrawGroup_Opaque]      = Pipeline_Prepass,
            [DrawGroup_AlphaTest]   = Pipeline_Prepass_AlphaTest,
            [DrawGroup_Transparent] = Pipeline_None,
        };
        DrawList(Frame, PrepassCmd, Pipelines, &PrimaryDrawList);

        vkCmdEndRendering(PrepassCmd);

        VkImageMemoryBarrier2 StructureImageBarrier =
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext                  = nullptr,
            .srcStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstAccessMask          = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex    = VK.GraphicsQueueIdx,
            .dstQueueFamilyIndex    = VK.ComputeQueueIdx,
            .image                  = Renderer->StructureBuffer->Image,
            .subresourceRange = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        
        VkImageMemoryBarrier2 VisibilityImageBarrier =
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext                  = nullptr,
            .srcStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = Renderer->VisibilityBuffer->Image,
            .subresourceRange = 
            {
                .aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel       = 0,
                .levelCount         = 1,
                .baseArrayLayer     = 0,
                .layerCount         = 1,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_SSAO], &StructureImageBarrier);
        PushBeginBarrier(&FrameStages[FrameStage_Shading], &VisibilityImageBarrier);
    }
    EndFrameStage(PrepassCmd, FrameStage_Prepass, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    EndCommandBuffer(PrepassCmd);

    u64 PrepassCounter = ++Renderer->TimelineSemaphoreCounter;
    {
        TimedBlock(Platform.Profiler, "Submit Prepass");
        VkSemaphoreSubmitInfo PrepassWaits[] =
        {
            {
                .sType      = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext      = nullptr,
                .semaphore  = Renderer->TimelineSemaphore,
                .value      = Renderer->FrameFinishedCounters[(Frame->FrameID + 1) % R_MaxFramesInFlight],
                .stageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            },
        };

        VkCommandBufferSubmitInfo PrepassCmdBuffers[] = 
        {
            {
                .sType          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext          = nullptr,
                .commandBuffer  = PrepassCmd,
                .deviceMask     = 0,
            },
        };
        VkSemaphoreSubmitInfo PrepassSignals[] = 
        {
            {
                .sType      = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext      = nullptr,
                .semaphore  = Renderer->TimelineSemaphore,
                .value      = PrepassCounter,
                .stageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            },
        };
        VkSubmitInfo2 SubmitPrepass = 
        {
            .sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext                      = nullptr,
            .waitSemaphoreInfoCount     = CountOf(PrepassWaits),
            .pWaitSemaphoreInfos        = PrepassWaits,
            .commandBufferInfoCount     = CountOf(PrepassCmdBuffers),
            .pCommandBufferInfos        = PrepassCmdBuffers,
            .signalSemaphoreInfoCount   = CountOf(PrepassSignals),
            .pSignalSemaphoreInfos      = PrepassSignals,
        };
        vkQueueSubmit2(VK.GraphicsQueue, 1, &SubmitPrepass, nullptr);
    }

    //
    // SSAO
    //
    {
        VkImageMemoryBarrier2 Barrier =
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
        };
        PushBeginBarrier(&FrameStages[FrameStage_SSAO], &Barrier);
    }

    BeginFrameStage(PreLightCmd, FrameStage_SSAO, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
        // Occlusion
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_SSAO];
            vkCmdBindPipeline(PreLightCmd, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline.Pipeline);
            
            u32 DispatchX = CeilDiv(Frame->RenderExtent.X, SSAO_GroupSizeX);
            u32 DispatchY = CeilDiv(Frame->RenderExtent.Y, SSAO_GroupSizeY);
            vkCmdDispatch(PreLightCmd, DispatchX, DispatchY, 1);
        }

        // Blur/filter
        {
            VkImageMemoryBarrier2 BlurBarriers[] = 
            {
                // Raw SSAO
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .srcAccessMask          = VK_ACCESS_2_SHADER_WRITE_BIT,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
                    .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .image                  = Renderer->OcclusionBuffers[0]->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
                // Filtered SSAO
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                    .srcAccessMask          = VK_ACCESS_2_NONE,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dstAccessMask          = VK_ACCESS_2_SHADER_WRITE_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                    .srcQueueFamilyIndex    = VK.GraphicsQueueIdx,
                    .dstQueueFamilyIndex    = VK.ComputeQueueIdx,
                    .image                  = Renderer->OcclusionBuffers[1]->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
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

            u32 BlurDispatchX = CeilDiv(Frame->RenderExtent.X, SSAOBlur_GroupSizeX);
            u32 BlurDispatchY = CeilDiv(Frame->RenderExtent.Y, SSAOBlur_GroupSizeY);
            vkCmdDispatch(PreLightCmd, BlurDispatchX, BlurDispatchY, 1);
        }

        VkImageMemoryBarrier2 EndBarrier =
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext                  = nullptr,
            .srcStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask          = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask           = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex    = VK.ComputeQueueIdx,
            .dstQueueFamilyIndex    = VK.GraphicsQueueIdx,
            .image                  = Renderer->OcclusionBuffers[1]->Image,
            .subresourceRange = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_Shading], &EndBarrier);
    }
    EndFrameStage(PreLightCmd, FrameStage_SSAO, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    //
    // Light binning
    //
    BeginFrameStage(PreLightCmd, FrameStage_LightBinning, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    // NOTE(boti): This should always pass...
    if (TileBufferRange.ByteCount)
    {
        pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_LightBinning];
        vkCmdBindPipeline(PreLightCmd, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline.Pipeline);

        v2u DispatchXY = { CeilDiv(Frame->RenderExtent.X, LightBin_GroupSizeX), CeilDiv(Frame->RenderExtent.Y, LightBin_GroupSizeY) };
        vkCmdDispatch(PreLightCmd, DispatchXY.X, DispatchXY.Y, 1);

        VkBufferMemoryBarrier2 EndBarrier = 
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = Renderer->PerFrameBuffer,
            .offset = TileBufferRange.ByteOffset,
            .size = TileBufferRange.ByteCount,
        };
        PushBeginBarrier(&FrameStages[FrameStage_Shading], &EndBarrier);
    }
    EndFrameStage(PreLightCmd, FrameStage_LightBinning, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    EndCommandBuffer(PreLightCmd);

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
        .sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                      = nullptr,
        .flags                      = 0,
        .waitSemaphoreInfoCount     = CountOf(PreLightWaits),
        .pWaitSemaphoreInfos        = PreLightWaits,
        .commandBufferInfoCount     = CountOf(PreLightCmdBuffers),
        .pCommandBufferInfos        = PreLightCmdBuffers,
        .signalSemaphoreInfoCount   = CountOf(PreLightSignals),
        .pSignalSemaphoreInfos      = PreLightSignals,
    };
    vkQueueSubmit2(VK.ComputeQueue, 1, &SubmitPreLight, nullptr);

    //
    // Cascaded shadows
    //
    {
        VkImageMemoryBarrier2 Barrier = 
        {
            
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Frame->Renderer->CascadeMap,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = R_MaxShadowCascadeCount,
            },   
        };
        PushBeginBarrier(&FrameStages[FrameStage_CascadedShadow], &Barrier);
    }

    BeginFrameStage(ShadowCmd, FrameStage_CascadedShadow, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
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
            VkRenderingAttachmentInfo DepthAttachment = 
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = Frame->Renderer->CascadeViews[CascadeIndex],
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .depthStencil = { 1.0, 0 } },
            };

            VkRenderingInfo RenderingInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderArea = { { 0, 0 } , { R_ShadowResolution, R_ShadowResolution } },
                .layerCount = 1,
                .viewMask = 0,
                .colorAttachmentCount = 0,
                .pColorAttachments = nullptr,
                .pDepthAttachment = &DepthAttachment,
                .pStencilAttachment = nullptr,
            };
            
            vkCmdBeginRendering(ShadowCmd, &RenderingInfo);

            m4 ViewProjection = Frame->Uniforms.CascadeViewProjections[CascadeIndex];
            vkCmdPushConstants(ShadowCmd, Renderer->SystemPipelineLayout,
                               VK_SHADER_STAGE_ALL,
                               0, sizeof(ViewProjection), &ViewProjection);

            pipeline Pipelines[DrawGroup_Count] = 
            {
                [DrawGroup_Opaque]      = Pipeline_ShadowCascade,
                [DrawGroup_AlphaTest]   = Pipeline_ShadowCascade_AlphaTest,
                [DrawGroup_Transparent] = Pipeline_None,
            };
            DrawList(Frame, ShadowCmd, Pipelines, CascadeDrawLists + CascadeIndex);

            vkCmdEndRendering(ShadowCmd);

            VkMemoryBarrier Barrier = 
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT,
            };
            vkCmdPipelineBarrier(ShadowCmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 
                                 1, &Barrier, 0, nullptr, 0, nullptr);
        }

        VkImageMemoryBarrier2 Barrier =
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
            .image = Renderer->CascadeMap,
            .subresourceRange = 
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = R_MaxShadowCascadeCount,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_Shading], &Barrier);
    }
    EndFrameStage(ShadowCmd, FrameStage_CascadedShadow, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    //
    // Point shadows
    //
    for (u32 ShadowIndex = 0; ShadowIndex < ShadowCount; ShadowIndex++)
    {
        point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowIndex;
        VkImageMemoryBarrier2 Barrier = 
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext                  = nullptr,
            .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask          = VK_ACCESS_2_NONE,
            .dstStageMask           = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask          = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout              = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = ShadowMap->Image,
            .subresourceRange = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 6,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_Shadows], &Barrier);
    }

    BeginFrameStage(ShadowCmd, FrameStage_Shadows, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
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
        for (u32 ShadowIndex = 0; ShadowIndex < ShadowCount; ShadowIndex++)
        {
            point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowIndex;

            for (u32 LayerIndex = 0; LayerIndex < CubeLayer_Count; LayerIndex++)
            {
                VkMemoryBarrier Barrier = 
                {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT,
                };
                vkCmdPipelineBarrier(ShadowCmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 
                                     1, &Barrier, 0, nullptr, 0, nullptr);

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

                m4 ViewProjection = Frame->Uniforms.PointShadows[ShadowIndex].ViewProjections[LayerIndex];
                vkCmdPushConstants(ShadowCmd, Renderer->SystemPipelineLayout, VK_SHADER_STAGE_ALL,
                                   0, sizeof(ViewProjection), &ViewProjection);

                pipeline Pipelines[DrawGroup_Count] = 
                {
                    [DrawGroup_Opaque]      = Pipeline_Shadow,
                    [DrawGroup_AlphaTest]   = Pipeline_Shadow_AlphaTest,
                    [DrawGroup_Transparent] = Pipeline_None,
                };
                u32 Index = 6*ShadowIndex + LayerIndex;
                DrawList(Frame, ShadowCmd, Pipelines, ShadowDrawLists + Index);

                vkCmdEndRendering(ShadowCmd);
            }
        }

        for (u32 ShadowIndex = 0; ShadowIndex < ShadowCount; ShadowIndex++)
        {
            point_shadow_map* ShadowMap = Renderer->PointShadowMaps + ShadowIndex;
            VkImageMemoryBarrier2 Barrier =
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                .srcStageMask           = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask          = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .dstStageMask           = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = ShadowMap->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 6,
                },
            };
            PushBeginBarrier(&FrameStages[FrameStage_Shading], &Barrier);
        }
    }
    EndFrameStage(ShadowCmd, FrameStage_Shadows, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    EndCommandBuffer(ShadowCmd);
    
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
        .sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                      = nullptr,
        .flags                      = 0,
        .waitSemaphoreInfoCount     = CountOf(ShadowWaits),
        .pWaitSemaphoreInfos        = ShadowWaits,
        .commandBufferInfoCount     = CountOf(ShadowCmdBuffers),
        .pCommandBufferInfos        = ShadowCmdBuffers,
        .signalSemaphoreInfoCount   = CountOf(ShadowSignals),
        .pSignalSemaphoreInfos      = ShadowSignals,
    };
    vkQueueSubmit2(VK.GraphicsQueue, 1, &SubmitShadow, nullptr);
    
    vkCmdSetViewport(RenderCmd, 0, 1, &PrimaryViewport);
    vkCmdSetScissor(RenderCmd, 0, 1, &PrimaryScissor);

    //
    // Shading
    //
    {
        // HDR Color
        // NOTE(boti): Synchronization validation complains about a WAW hazard on subsequent frames from the previous frame's bloom pass,
        // but that can't be correct, since the bloom pass only ever _reads_ mip 0.
        VkImageMemoryBarrier2 HDRColorBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask           = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .srcAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
            .dstStageMask           = (Frame->Config.ShadingMode == ShadingMode_Forward) ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask          = (Frame->Config.ShadingMode == ShadingMode_Forward) ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT : VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout              = (Frame->Config.ShadingMode == ShadingMode_Forward) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = Renderer->HDRRenderTarget->Image,
            .subresourceRange = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_Shading], &HDRColorBarrier);
    }

    VkRenderingAttachmentInfo ShadingColorAttachments[] = 
    {
        {
            .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext              = nullptr,
            .imageView          = Renderer->HDRRenderTarget->View,
            .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode        = VK_RESOLVE_MODE_NONE,
            .resolveImageView   = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp             = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue         = {},
        },
    };
    
    VkRenderingAttachmentInfo ShadingDepthAttachment = 
    {
        .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext              = nullptr,
        .imageView          = Renderer->DepthBuffer->View,
        .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .resolveMode        = VK_RESOLVE_MODE_NONE,
        .resolveImageView   = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp             = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue         = {},
    };
    VkRenderingInfo ShadingRenderingInfo = 
    {
        .sType                  = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext                  = nullptr,
        .flags                  = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
        .renderArea             = { { 0, 0 }, { Frame->RenderExtent.X, Frame->RenderExtent.Y } },
        .layerCount             = 1,
        .viewMask               = 0,
        .colorAttachmentCount   = CountOf(ShadingColorAttachments),
        .pColorAttachments      = ShadingColorAttachments,
        .pDepthAttachment       = &ShadingDepthAttachment,
        .pStencilAttachment     = nullptr,
    };

    BeginFrameStage(RenderCmd, FrameStage_Shading, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "Opaque");
    {
        pipeline PipelineID = Pipeline_None;
        VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        switch (Frame->Config.ShadingMode)
        {
            case ShadingMode_Visibility:
            {
                PipelineID = Pipeline_ShadingVisibility;
                BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            } break;
            case ShadingMode_Forward:
            {
                PipelineID = Pipeline_ShadingForward;
                BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            } break;
            InvalidDefaultCase;
        }

        pipeline_with_layout Pipeline = Renderer->Pipelines[PipelineID];
        vkCmdBindPipeline(RenderCmd, BindPoint, Pipeline.Pipeline);

        switch (Frame->Config.ShadingMode)
        {
            case ShadingMode_Visibility:
            {
                u32 DispatchX = CeilDiv(Frame->RenderExtent.X, ShadingVisibility_GroupSizeX);
                u32 DispatchY = CeilDiv(Frame->RenderExtent.Y, ShadingVisibility_GroupSizeY);
                vkCmdDispatch(RenderCmd, DispatchX, DispatchY, 1);

                VkImageMemoryBarrier2 EndBarriers[] = 
                {
                    {
                        .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .pNext                  = nullptr,
                        .srcStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                        .srcAccessMask          = VK_ACCESS_2_SHADER_WRITE_BIT,
                        .dstStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .dstAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
                        .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
                        .newLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                        .image                  = Renderer->HDRRenderTarget->Image,
                        .subresourceRange = 
                        {
                            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel   = 0,
                            .levelCount     = 1,
                            .baseArrayLayer = 0,
                            .layerCount     = 1,
                        },
                    },
                };
                VkDependencyInfo EndDependency = 
                {
                    .sType                      = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext                      = nullptr,
                    .dependencyFlags            = 0,
                    .imageMemoryBarrierCount    = CountOf(EndBarriers),
                    .pImageMemoryBarriers       = EndBarriers,
                };
                vkCmdPipelineBarrier2(RenderCmd, &EndDependency);

                vkCmdBeginRendering(RenderCmd, &ShadingRenderingInfo);
            } break;
            case ShadingMode_Forward:
            {
                vkCmdBeginRendering(RenderCmd, &ShadingRenderingInfo);
                pipeline Pipelines[DrawGroup_Count] = 
                {
                    [DrawGroup_Opaque]      = Pipeline_ShadingForward,
                    [DrawGroup_AlphaTest]   = Pipeline_ShadingForward_AlphaTest,
                    [DrawGroup_Transparent] = Pipeline_None,
                };
                DrawList(Frame, RenderCmd, Pipelines, &PrimaryDrawList);
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

    vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "Transparent");
    {
        pipeline Pipelines[DrawGroup_Count] =
        {
            [DrawGroup_Opaque]      = Pipeline_None,
            [DrawGroup_AlphaTest]   = Pipeline_None,
            [DrawGroup_Transparent] = Pipeline_ShadingForward_Transparent,
        };
        DrawList(Frame, RenderCmd, Pipelines, &PrimaryDrawList);
    }
    vkCmdEndDebugUtilsLabelEXT(RenderCmd);

    vkCmdExecuteCommands(RenderCmd, 1, &ParticleCB);

    vkCmdEndRendering(RenderCmd);

    EndFrameStage(RenderCmd, FrameStage_Shading, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    //
    // Mip usage copy
    //
    {
        VkBufferMemoryBarrier2 BeginBarriers[] = 
        {
            {
                .sType                  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                .srcStageMask           = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask          = VK_ACCESS_2_SHADER_READ_BIT|VK_ACCESS_2_SHADER_WRITE_BIT,
                .dstStageMask           = VK_PIPELINE_STAGE_2_COPY_BIT,
                .dstAccessMask          = VK_ACCESS_2_TRANSFER_READ_BIT,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .buffer                 = Renderer->MipFeedbackBuffer,
                .offset                 = 0,
                .size                   = VK_WHOLE_SIZE,
            },
        };
        VkDependencyInfo BeginDependency = 
        {
            .sType                      = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext                      = nullptr,
            .dependencyFlags            = 0,
            .bufferMemoryBarrierCount   = CountOf(BeginBarriers),
            .pBufferMemoryBarriers      = BeginBarriers,
        };
        vkCmdPipelineBarrier2(RenderCmd, &BeginDependency);

        VkBufferCopy Copy = 
        {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = Renderer->MipFeedbackMemorySize,
        };
        vkCmdCopyBuffer(RenderCmd, Renderer->MipFeedbackBuffer, Renderer->MipReadbackBuffers[Frame->FrameID], 1, &Copy);
    }

    //
    // Bloom
    //
    BeginFrameStage(RenderCmd, FrameStage_Bloom, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
        v2u Extent = Frame->RenderExtent;

        pipeline_with_layout DownsamplePipeline = Renderer->Pipelines[Pipeline_BloomDownsample];
        pipeline_with_layout UpsamplePipeline = Renderer->Pipelines[Pipeline_BloomUpsample];

        constexpr u32 MaxBloomMipCount = 64u;//9u;
        u32 BloomMipCount = Min(Renderer->HDRRenderTarget->MipCount, MaxBloomMipCount);
        VkImageMemoryBarrier2 BeginBarriers[] = 
        {
            // HDR Mip 0
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                .srcStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
                .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask          = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = Renderer->HDRRenderTarget->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            },

            // HDR mips
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                // NOTE(boti): Synchronization validation w/ version 1.3.280 complains that the rest of
                // the mips don't have the src stage/access masks set, but that seems like a false positive?
                #if 1
                .srcStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
                #else
                .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask          = VK_ACCESS_2_NONE,
                #endif
                .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask          = VK_ACCESS_2_SHADER_WRITE_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = Renderer->HDRRenderTarget->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 1,
                    .levelCount     = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            },
        };

        VkDependencyInfo BeginDependencyInfo = 
        {
            .sType                      = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext                      = nullptr,
            .dependencyFlags            = 0,
            .imageMemoryBarrierCount    = CountOf(BeginBarriers),
            .pImageMemoryBarriers       = BeginBarriers,
        };
        vkCmdPipelineBarrier2(RenderCmd, &BeginDependencyInfo); 

        // NOTE(boti): Downsampling always goes down to the 1x1 mip, in case someone wants the average luminance of the scene
        vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_COMPUTE, DownsamplePipeline.Pipeline);
        for (u32 Mip = 1; Mip < Renderer->HDRRenderTarget->MipCount; Mip++)
        {
            vkCmdPushConstants(RenderCmd, DownsamplePipeline.Layout, VK_SHADER_STAGE_ALL, 
                               0, sizeof(Mip), &Mip);

            u32 DispatchX = CeilDiv(Extent.X, BloomDownsample_GroupSizeX);
            u32 DispatchY = CeilDiv(Extent.Y, BloomDownsample_GroupSizeY);
            vkCmdDispatch(RenderCmd, DispatchX, DispatchY, 1);

            Extent.X = Max(Extent.X >> 1, 1u);
            Extent.Y = Max(Extent.Y >> 1, 1u);

            VkImageMemoryBarrier2 MipBarrier = 
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                .srcStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask          = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = Renderer->HDRRenderTarget->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = Mip,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
            VkDependencyInfo Barrier = 
            {
                .sType                      = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext                      = nullptr,
                .dependencyFlags            = 0,
                .imageMemoryBarrierCount    = 1,
                .pImageMemoryBarriers       = &MipBarrier,
            };

            vkCmdPipelineBarrier2(RenderCmd, &Barrier);
        }

        // Copy highest mip to the bloom chain
        {
            u32 MipIndex = BloomMipCount - 1;

            VkImageMemoryBarrier2 CopyBeginImageBarriers[] = 
            {
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                    .srcAccessMask          = VK_ACCESS_2_NONE,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .dstAccessMask          = VK_ACCESS_2_TRANSFER_READ_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
                    .newLayout              = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .image                  = Renderer->HDRRenderTarget->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = MipIndex,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                    .srcAccessMask          = VK_ACCESS_2_NONE,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .dstAccessMask          = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout              = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .image                  = Renderer->BloomTarget->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = MipIndex,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
            };
            VkDependencyInfo CopyBeginBarrier = 
            {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .imageMemoryBarrierCount = CountOf(CopyBeginImageBarriers),
                .pImageMemoryBarriers = CopyBeginImageBarriers,
            };
            vkCmdPipelineBarrier2(RenderCmd, &CopyBeginBarrier);

            VkImageCopy CopyRegion = 
            {
                .srcSubresource = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = MipIndex,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
                .srcOffset = { 0, 0, 0 },
                .dstSubresource = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = MipIndex,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
                .dstOffset = { 0, 0, 0 },
                .extent = 
                {
                    .width  = Max(Frame->RenderExtent.X >> MipIndex, 1u),
                    .height = Max(Frame->RenderExtent.Y >> MipIndex, 1u),
                    .depth  = 1,
                },
            };

            vkCmdCopyImage(RenderCmd,
                           Renderer->HDRRenderTarget->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           Renderer->BloomTarget->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &CopyRegion);

            VkImageMemoryBarrier2 CopyEndImageBarriers[] = 
            {
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .srcAccessMask          = VK_ACCESS_2_TRANSFER_READ_BIT,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                    .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .image                  = Renderer->HDRRenderTarget->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = MipIndex,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                    .srcAccessMask          = VK_ACCESS_2_NONE,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                    .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
                    .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .image                  = Renderer->HDRRenderTarget->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = MipIndex,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .srcAccessMask          = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dstAccessMask          = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                    .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .image                  = Renderer->BloomTarget->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = MipIndex,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
                {
                    .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext                  = nullptr,
                    .srcStageMask           = VK_PIPELINE_STAGE_2_COPY_BIT,
                    .srcAccessMask          = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dstAccessMask          = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                    .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                    .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                    .image                  = Renderer->BloomTarget->Image,
                    .subresourceRange = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = MipIndex,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
            };
            VkDependencyInfo CopyEndBarrier = 
            {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .imageMemoryBarrierCount = CountOf(CopyEndImageBarriers),
                .pImageMemoryBarriers = CopyEndImageBarriers,
            };
            vkCmdPipelineBarrier2(RenderCmd, &CopyEndBarrier);
        }

        vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_COMPUTE, UpsamplePipeline.Pipeline);
        for (u32 Index = 1; Index < BloomMipCount; Index++)
        {
            u32 Mip = BloomMipCount - Index - 1;
            vkCmdPushConstants(RenderCmd, UpsamplePipeline.Layout, VK_SHADER_STAGE_ALL,
                               0, sizeof(Mip), &Mip);
            Extent.X = Max(Frame->RenderExtent.X >> Mip, 1u);
            Extent.Y = Max(Frame->RenderExtent.Y >> Mip, 1u);

            u32 DispatchX = CeilDiv(Extent.X, BloomUpsample_GroupSizeX);
            u32 DispatchY = CeilDiv(Extent.Y, BloomUpsample_GroupSizeY);
            vkCmdDispatch(RenderCmd, DispatchX, DispatchY, 1);

            VkImageMemoryBarrier2 End = 
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext                  = nullptr,
                .srcStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask          = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT|VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = Renderer->BloomTarget->Image,
                .subresourceRange = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = Mip,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
    
            VkDependencyInfo EndDependency = 
            {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &End,
            };
            vkCmdPipelineBarrier2(RenderCmd, &EndDependency);
        }

        VkImageMemoryBarrier2 End = 
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext                  = nullptr,
            .srcStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask          = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT|VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            .dstStageMask           = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            .dstAccessMask          = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = Renderer->BloomTarget->Image,
            .subresourceRange = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_BlitAndGUI], &End);
    }
    EndFrameStage(RenderCmd, FrameStage_Bloom, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);

    //
    // Blit + UI
    //
    {
        VkImageMemoryBarrier2 SwapchainBarrier =
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext                  = nullptr,
            .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask          = VK_ACCESS_2_NONE,
            .dstStageMask           = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask          = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = Renderer->SwapchainImages[SwapchainImageIndex],
            .subresourceRange = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        VkImageMemoryBarrier2 SwapchainDepthBarrier = 
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext                  = nullptr,
            #if 1
            .srcStageMask           = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask          = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            #else
            .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask          = VK_ACCESS_2_NONE,
            #endif
            .dstStageMask           = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask          = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout              = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = Renderer->SwapchainDepthImage,
            .subresourceRange = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        PushBeginBarrier(&FrameStages[FrameStage_BlitAndGUI], &SwapchainBarrier);
        PushBeginBarrier(&FrameStages[FrameStage_BlitAndGUI], &SwapchainDepthBarrier);
    }

    BeginFrameStage(RenderCmd, FrameStage_BlitAndGUI, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    {
        VkRenderingAttachmentInfo ColorAttachment = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Renderer->SwapchainImageViews[SwapchainImageIndex],
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
            .imageView = Renderer->SwapchainDepthImageView,
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
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .renderArea = { { 0, 0 }, { Frame->OutputExtent.X, Frame->OutputExtent.Y } },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachment,
            .pStencilAttachment = nullptr,
        };
        vkCmdBeginRendering(RenderCmd, &RenderingInfo);

        vkCmdSetViewport(RenderCmd, 0, 1, &BlitViewport);
        vkCmdSetScissor(RenderCmd, 0, 1, &BlitScissor);

        // Blit
        vkCmdBeginDebugUtilsLabelEXT(RenderCmd, "Blit");
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Blit];
            vkCmdBindPipeline(RenderCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdDraw(RenderCmd, 3, 1, 0, 0);
        }
        vkCmdEndDebugUtilsLabelEXT(RenderCmd);

        VkCommandBuffer PostBlitCBs[] = { Widget3DCB, GuiCB };
        vkCmdExecuteCommands(RenderCmd, CountOf(PostBlitCBs), PostBlitCBs);

        vkCmdEndRendering(RenderCmd);
    }
    EndFrameStage(RenderCmd, FrameStage_BlitAndGUI, Renderer->PerformanceQueryPools[Frame->FrameID], FrameStages);
    

    VkImageMemoryBarrier2 EndBarriers[] = 
    {
        // Swapchain
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Renderer->SwapchainImages[SwapchainImageIndex],
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

    vkCmdWriteTimestamp2(RenderCmd, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, Renderer->PerformanceQueryPools[Frame->FrameID], 2 * FrameStage_Count + 1);

    EndCommandBuffer(RenderCmd);

    // Submit + Present
    {
        TimedBlock(Platform.Profiler, "SubmitAndPresent");

        u64 RenderCounter = ++Renderer->TimelineSemaphoreCounter;
        Renderer->FrameFinishedCounters[Frame->FrameID] = RenderCounter;
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
                .sType      = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext      = nullptr,
                .semaphore  = Renderer->TimelineSemaphore,
                .value      = ShadowCounter,
                .stageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            },
            {
                .sType      = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext      = nullptr,
                .semaphore  = Renderer->ComputeTimelineSemaphore,
                .value      = PreLightCounter,
                .stageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            },
            {
                .sType      = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext      = nullptr,
                .semaphore  = Renderer->ImageAcquiredSemaphores[Frame->FrameID],
                .value      = 0,
                .stageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            },
        };
        VkSemaphoreSubmitInfo RenderSignals[] = 
        {
            {
                .sType      = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext      = nullptr,
                .semaphore  = Renderer->TimelineSemaphore,
                .value      = RenderCounter,
                .stageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            },
            {
                .sType      = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext      = nullptr,
                .semaphore  = Renderer->RenderFinishedSemaphores[Frame->FrameID],
                .value      = 0,
                .stageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            },
        };
        VkSubmitInfo2 SubmitRender = 
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .waitSemaphoreInfoCount     = CountOf(RenderWaits),
            .pWaitSemaphoreInfos        = RenderWaits,
            .commandBufferInfoCount     = CountOf(RenderCmdBuffers),
            .pCommandBufferInfos        = RenderCmdBuffers,
            .signalSemaphoreInfoCount   = CountOf(RenderSignals),
            .pSignalSemaphoreInfos      = RenderSignals,
        };
        vkQueueSubmit2(VK.GraphicsQueue, 1, &SubmitRender, nullptr);

        VkPresentInfoKHR PresentInfo = 
        {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext              = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &Renderer->RenderFinishedSemaphores[Frame->FrameID],
            .swapchainCount     = 1,
            .pSwapchains        = &Renderer->Swapchain,
            .pImageIndices      = &SwapchainImageIndex,
            .pResults           = nullptr,
        };
        vkQueuePresentKHR(VK.GraphicsQueue, &PresentInfo);
    }

#if 0
    b32 GeometryIntegrity = VerifyGeometryIntegrity(&Renderer->GeometryBuffer);
    if (!GeometryIntegrity)
    {
        UnhandledError("Geometry is messed up!");
    }
#endif

    // Verify texture allocation integrity
#if 0
    {
        texture_manager* Manager = &Renderer->TextureManager;
        v2u* IndexCountPairs = PushArray(Frame->Arena, 0, v2u, Manager->TextureCount);

        for (u32 Index = 0; Index < Manager->TextureCount; Index++)
        {
            IndexCountPairs[Index].X = TruncateU64ToU32(Manager->Textures[Index].PageIndex);
            IndexCountPairs[Index].Y = TruncateU64ToU32(Manager->Textures[Index].PageCount);
        }

        v2u* Src = IndexCountPairs;
        v2u* Dst = PushArray(Frame->Arena, 0, v2u, Manager->TextureCount);
        for (u32 BucketIndex = 0; BucketIndex < 4; BucketIndex++)
        {
            u32 BucketOffsets[256] = {};
            for (u32 It = 0; It < Manager->TextureCount; It++)
            {
                u32 Key = (Src[It].X >> (BucketIndex * 8)) & 0xFF;
                BucketOffsets[Key]++;
            }

            u32 TotalCount = 0;
            for (u32 It = 0; It < 256; It++)
            {
                u32 CurrentCount = BucketOffsets[It];
                BucketOffsets[It] = TotalCount;
                TotalCount += CurrentCount;
            }

            for (u32 It = 0; It < Manager->TextureCount; It++)
            {
                u32 Key = (Src[It].X >> (BucketIndex * 8)) & 0xFF;
                u32 DstIt = BucketOffsets[Key]++;

                Dst[DstIt] = Src[It];
            }

            {
                v2u* Tmp = Src;
                Src = Dst;
                Dst = Tmp;
            }
        }

        for (u32 It = 1; It < Manager->TextureCount; It++)
        {
            Assert(IndexCountPairs[It - 1].X <= IndexCountPairs[It].X);
            u32 PrevEnd = IndexCountPairs[It - 1].X + IndexCountPairs[It - 1].Y;
            if (IndexCountPairs[It].Y > 0)
            {
                Assert(PrevEnd <= IndexCountPairs[It].X);
            }
        }
    }
#endif

    // Collect stats
    {
        render_stats* Stats = &Frame->Stats;
        Stats->TotalMemoryUsed = 0;
        Stats->TotalMemoryAllocated = 0;
        Stats->MemoryEntryCount = 0;

        auto AddEntry = [Stats](const char* Name, umm UsedSize, umm TotalSize) -> b32
        {
            b32 Result = false;

            Stats->TotalMemoryUsed += UsedSize;
            Stats->TotalMemoryAllocated += TotalSize;

            if (Stats->MemoryEntryCount < Stats->MaxMemoryEntryCount)
            {
                render_stat_mem_entry* Entry = Stats->MemoryEntries + Stats->MemoryEntryCount++;
                Entry->Name = Name;
                Entry->UsedSize = UsedSize;
                Entry->AllocationSize = TotalSize;
                Result = true;
            }
            return(Result);
        };

        auto AddGPUArenaEntry = [Stats](const char* Name, gpu_memory_arena* Arena) -> b32
        {
            b32 Result = false;

            Stats->TotalMemoryUsed += Arena->MemoryAt;
            Stats->TotalMemoryAllocated += Arena->Size;

            if (Stats->MemoryEntryCount < Stats->MaxMemoryEntryCount)
            {
                render_stat_mem_entry* Entry = Stats->MemoryEntries + Stats->MemoryEntryCount++;
                Entry->Name = Name;
                Entry->UsedSize = Arena->MemoryAt;
                Entry->AllocationSize = Arena->Size;
                Result = true;
            }
            return(Result);
        };

        AddEntry("PerFrame", PerFrameBufferAt, Renderer->PerFrameBufferSize);
        AddEntry("MipFeedback", Renderer->TextureManager.TextureCount * sizeof(u32), Renderer->MipFeedbackMemorySize);
        AddEntry("BAR", Frame->BARBufferAt, Frame->BARBufferSize);
        AddGPUArenaEntry("RenderTarget", &Renderer->RenderTargetHeap.Arena);
        AddEntry("VertexBuffer", 
                 Renderer->GeometryBuffer.VertexMemory.CountInUse * Renderer->GeometryBuffer.VertexMemory.Stride, 
                 Renderer->GeometryBuffer.VertexMemory.MaxCount * Renderer->GeometryBuffer.VertexMemory.Stride);
        AddEntry("IndexBuffer", 
                 Renderer->GeometryBuffer.IndexMemory.CountInUse * Renderer->GeometryBuffer.IndexMemory.Stride, 
                 Renderer->GeometryBuffer.IndexMemory.MaxCount * Renderer->GeometryBuffer.IndexMemory.Stride);
        AddEntry("TextureCache", Renderer->TextureManager.Cache.UsedPageCount * TexturePageSize, Renderer->TextureManager.Cache.MemorySize);
        AddGPUArenaEntry("TexturePersist", &Renderer->TextureManager.PersistentArena);
        AddGPUArenaEntry("Shadow", &Renderer->ShadowArena);
        AddEntry("Staging", Frame->StagingBuffer.At, Frame->StagingBuffer.Size);
    }

    Renderer->CurrentFrameID++;
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
    
        f32 SplitFactor = 0.1f;
        f32 Splits[] = { 8.0f, 16.0f, 32.0f, 64.0f };
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
}

ProfilerOverflowGuard;