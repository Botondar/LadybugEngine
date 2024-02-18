//
// Utility
//
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

//
// Conversion
//
VkFormat FormatTable[Format_Count] = 
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
    
    [Format_B8G8R8A8_SRGB]      = VK_FORMAT_B8G8R8A8_SRGB,

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

VkPrimitiveTopology TopologyTable[Topology_Count] = 
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

VkFilter FilterTable[Filter_Count] = 
{
    [Filter_Nearest] = VK_FILTER_NEAREST,
    [Filter_Linear]  = VK_FILTER_LINEAR,
};

VkSamplerMipmapMode MipFilterTable[Filter_Count] = 
{
    [Filter_Nearest] = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    [Filter_Linear]  = VK_SAMPLER_MIPMAP_MODE_LINEAR,
};

VkSamplerAddressMode WrapTable[Wrap_Count] = 
{
    [Wrap_Repeat]           = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    [Wrap_RepeatMirror]     = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    [Wrap_ClampToEdge]      = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    [Wrap_ClampToBorder]    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
};

VkBorderColor BorderTable[Border_Count] = 
{
    [Border_Black] = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    [Border_White] = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
};

VkBlendOp BlendOpTable[BlendOp_Count] = 
{
    [BlendOp_Add] = VK_BLEND_OP_ADD,
    [BlendOp_Subtract] = VK_BLEND_OP_SUBTRACT,
    [BlendOp_ReverseSubtract] = VK_BLEND_OP_REVERSE_SUBTRACT,
    [BlendOp_Min] = VK_BLEND_OP_MIN,
    [BlendOp_Max] = VK_BLEND_OP_MAX,
};

VkBlendFactor BlendFactorTable[Blend_Count] = 
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

VkCompareOp CompareOpTable[Compare_Count] = 
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

VkDescriptorType DescriptorTypeTable[Descriptor_Count] =
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

internal VkShaderStageFlags ShaderStagesToVulkan(flags32 Stages)
{
    VkShaderStageFlags Result = 0;
    if (Stages == ShaderStage_All)
    {
        Result = VK_SHADER_STAGE_ALL;
    }
    else if (Stages == ShaderStage_AllGfx)
    {
        Result = VK_SHADER_STAGE_ALL_GRAPHICS;
    }
    else 
    {
        if (HasFlag(Stages, ShaderStage_VS)) Result |= VK_SHADER_STAGE_VERTEX_BIT;
        if (HasFlag(Stages, ShaderStage_PS)) Result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (HasFlag(Stages, ShaderStage_HS)) Result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (HasFlag(Stages, ShaderStage_DS)) Result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (HasFlag(Stages, ShaderStage_GS)) Result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (HasFlag(Stages, ShaderStage_CS)) Result |= VK_SHADER_STAGE_COMPUTE_BIT;
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

internal void UpdateDescriptorBuffer(u32 WriteCount, const descriptor_write* Writes, 
                                     VkDescriptorSetLayout Layout, void* Buffer)
{
    for (u32 WriteIndex = 0; WriteIndex < WriteCount; WriteIndex++)
    {
        const descriptor_write* Write = Writes + WriteIndex;

        VkDeviceSize BaseOffset = 0;
        vkGetDescriptorSetLayoutBindingOffsetEXT(VK.Device, Layout, Write->Binding, &BaseOffset);

        for (u32 Element = 0; Element < Write->Count; Element++)
        {
            u32 EffectiveElement = Element + Write->BaseIndex;

            const void* Data = nullptr;
            umm DescriptorSize = 0;
            
            VkDescriptorImageInfo ImageInfo = {};
            VkDescriptorAddressInfoEXT BufferInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
            switch (Write->Type)
            {
                case Descriptor_SampledImage: DescriptorSize = VK.DescriptorBufferProps.sampledImageDescriptorSize; goto IMAGE_CASE;
                case Descriptor_StorageImage: DescriptorSize = VK.DescriptorBufferProps.storageImageDescriptorSize; goto IMAGE_CASE;
                    IMAGE_CASE:
                    {
                        ImageInfo.sampler = VK_NULL_HANDLE,
                        ImageInfo.imageView = Write->Images[Element].View;
                        ImageInfo.imageLayout= Write->Images[Element].Layout;
                        Data = &ImageInfo;
                    } break;
                case Descriptor_StorageBuffer: DescriptorSize = VK.DescriptorBufferProps.storageBufferDescriptorSize; goto BUFFER_CASE;
                case Descriptor_UniformBuffer: DescriptorSize = VK.DescriptorBufferProps.uniformBufferDescriptorSize; goto BUFFER_CASE;
                    BUFFER_CASE:
                    {
                        BufferInfo.address = GetBufferDeviceAddress(VK.Device, Write->Buffers[Element].Buffer) + Write->Buffers[Element].Offset;
                        BufferInfo.range = Write->Buffers[Element].Range;
                        Data = &BufferInfo;
                    } break;
                InvalidDefaultCase;
            }

            VkDeviceSize Offset = BaseOffset + EffectiveElement*DescriptorSize;

            VkDescriptorGetInfoEXT DescriptorInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = DescriptorTypeTable[Write->Type],
                .data = { (VkSampler*)Data }, // HACK(boti): This is just a union of pointers, there really should be a void* in there...
            };
            vkGetDescriptorEXT(VK.Device, &DescriptorInfo, DescriptorSize, OffsetPtr(Buffer, Offset));
        }
    }
}