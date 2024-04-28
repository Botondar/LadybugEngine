#include <vulkan/vulkan.h>

//
// VK_EXT_debug_utils
//
PFN_vkCmdBeginDebugUtilsLabelEXT        vkCmdBeginDebugUtilsLabelEXT_;
PFN_vkCmdEndDebugUtilsLabelEXT          vkCmdEndDebugUtilsLabelEXT_;
PFN_vkSetDebugUtilsObjectNameEXT        vkSetDebugUtilsObjectNameEXT_;

//#define vkCmdBeginDebugUtilsLabelEXT    vkCmdBeginDebugUtilsLabelEXT_
#define vkCmdEndDebugUtilsLabelEXT      vkCmdEndDebugUtilsLabelEXT_
#define vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT_

inline void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer CmdBuffer, const char* Label);

static VkResult SetObjectName(VkDevice Device, VkObjectType Type, u64 Handle, const char* Name);
inline VkResult SetObjectName(VkDevice Device, VkDeviceMemory Memory, const char* Name);

//
// VK_EXT_descriptor_buffer
//
PFN_vkCmdBindDescriptorBuffersEXT               vkCmdBindDescriptorBuffersEXT_;
PFN_vkCmdSetDescriptorBufferOffsetsEXT          vkCmdSetDescriptorBufferOffsetsEXT_;
PFN_vkGetDescriptorEXT                          vkGetDescriptorEXT_;
PFN_vkGetDescriptorSetLayoutBindingOffsetEXT    vkGetDescriptorSetLayoutBindingOffsetEXT_;
PFN_vkGetDescriptorSetLayoutSizeEXT             vkGetDescriptorSetLayoutSizeEXT_;

#define vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT_
#define vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT_
#define vkGetDescriptorEXT vkGetDescriptorEXT_
#define vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT_
#define vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT_

//
// Conversion
//
extern VkFormat             FormatTable[Format_Count];
extern VkPrimitiveTopology  TopologyTable[Topology_Count];
extern VkFilter             FilterTable[Filter_Count];
extern VkSamplerMipmapMode  MipFilterTable[Filter_Count];
extern VkSamplerAddressMode WrapTable[Wrap_Count];
extern VkBorderColor        BorderTable[Border_Count];
extern VkBlendOp            BlendOpTable[BlendOp_Count];
extern VkBlendFactor        BlendFactorTable[Blend_Count];
extern VkCompareOp          CompareOpTable[Compare_Count];
extern VkDescriptorType     DescriptorTypeTable[Descriptor_Count];
extern f32                  AnisotropyTable[Anisotropy_Count];


internal VkShaderStageFlags ShaderStagesToVulkan(shader_stages Stages);
internal VkSamplerCreateInfo SamplerStateToVulkanSamplerInfo(sampler_state Sampler);
internal VkImageCreateInfo TextureInfoToVulkan(texture_info Info);
internal VkComponentSwizzle SwizzleComponentToVulkan(texture_swizzle_type Swizzle);
internal VkComponentMapping SwizzleToVulkan(texture_swizzle Swizzle);

//
// Utility
//
internal VkMemoryRequirements 
GetBufferMemoryRequirements(VkDevice Device, const VkBufferCreateInfo* BufferInfo);

internal VkMemoryRequirements
GetImageMemoryRequirements(VkDevice Device, const VkImageCreateInfo* ImageInfo, VkImageAspectFlagBits Aspects);

inline VkDeviceAddress 
GetBufferDeviceAddress(VkDevice Device, VkBuffer Buffer);

struct descriptor_write_image
{
    VkImageView View;
    VkImageLayout Layout;
};

struct descriptor_write_buffer
{
    VkBuffer Buffer;
    VkDeviceSize Offset;
    VkDeviceSize Range;
};

struct descriptor_write
{
    descriptor_type Type;
    u32 Binding;
    u32 BaseIndex;
    u32 Count;
    static constexpr u32 MaxArrayCount = 64;
    union
    {
        descriptor_write_image Images[MaxArrayCount];
        descriptor_write_buffer Buffers[MaxArrayCount];
    };
};

internal void UpdateDescriptorBuffer(u32 WriteCount, const descriptor_write* Writes, 
                                     VkDescriptorSetLayout Layout, void* Buffer);


//
// Implementation
//
inline void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer CmdBuffer, const char* Label)
{
    VkDebugUtilsLabelEXT UtilsLabel = 
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = Label,
        .color = {},
    };
    vkCmdBeginDebugUtilsLabelEXT_(CmdBuffer, &UtilsLabel);
}

inline VkDeviceAddress 
GetBufferDeviceAddress(VkDevice Device, VkBuffer Buffer)
{
    VkDeviceAddress Result = 0;
    if (Buffer)
    {
        VkBufferDeviceAddressInfo Info = 
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = Buffer,
        };
        Result = vkGetBufferDeviceAddress(Device, &Info);
    }
    return(Result);
}

static VkResult SetObjectName(VkDevice Device, VkObjectType Type, u64 Handle, const char* Name)
{
    VkDebugUtilsObjectNameInfoEXT NameInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = Type,
        .objectHandle = Handle,
        .pObjectName = Name,
    };
    VkResult Result = vkSetDebugUtilsObjectNameEXT(Device, &NameInfo);
    return(Result);
}

inline VkResult SetObjectName(VkDevice Device, VkDeviceMemory Memory, const char* Name)
{
    return SetObjectName(Device, VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)Memory, Name);
}