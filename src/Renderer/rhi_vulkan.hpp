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

internal VkShaderStageFlags ShaderStagesToVulkan(shader_stages Stages);
internal VkSamplerCreateInfo SamplerStateToVulkanSamplerInfo(sampler_state Sampler);

//
// Utility
//
internal VkMemoryRequirements 
GetBufferMemoryRequirements(VkDevice Device, const VkBufferCreateInfo* BufferInfo);

internal VkMemoryRequirements
GetImageMemoryRequirements(VkDevice Device, const VkImageCreateInfo* ImageInfo, VkImageAspectFlagBits Aspects);

inline VkDeviceAddress 
GetBufferDeviceAddress(VkDevice Device, VkBuffer Buffer);


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
    VkBufferDeviceAddressInfo Info = 
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = Buffer,
    };

    VkDeviceAddress Result = vkGetBufferDeviceAddress(Device, &Info);
    return(Result);
}