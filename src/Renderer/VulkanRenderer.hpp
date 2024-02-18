#include "Renderer/Renderer.hpp"
#include "Renderer/Pipelines.hpp"

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

internal VkMemoryRequirements 
GetBufferMemoryRequirements(VkDevice Device, const VkBufferCreateInfo* BufferInfo);

internal VkMemoryRequirements
GetImageMemoryRequirements(VkDevice Device, const VkImageCreateInfo* ImageInfo, VkImageAspectFlagBits Aspects);

inline VkDeviceAddress 
GetBufferDeviceAddress(VkDevice Device, VkBuffer Buffer);

struct gpu_memory_arena
{
    VkDeviceMemory Memory;
    u64 Size;
    u64 MemoryAt;
    u32 MemoryTypeIndex;
    void* Mapping;
};

typedef flags32 gpu_memory_arena_flags;
enum gpu_memory_arena_flag_bits : gpu_memory_arena_flags
{
    GpuMemoryFlag_None      = 0,
    GpuMemoryFlag_Mapped    = (1 << 0),
};

internal gpu_memory_arena 
CreateGPUArena(umm Size, u32 MemoryTypeIndex, gpu_memory_arena_flags Flags);

internal b32 
PushBuffer(gpu_memory_arena* Arena, 
           VkDeviceSize Size, 
           VkBufferUsageFlags Usage, 
           VkBuffer* Buffer, 
           void** Mapping = nullptr);

extern m3 GlobalCubeFaceBases[Layer_Count];

#include "Renderer/RenderDevice.hpp"
#include "Renderer/RenderTarget.hpp"
#include "Renderer/Geometry.hpp"
#include "Renderer/TextureManager.hpp"
#include "Renderer/Frame.hpp"
#include "Platform.hpp"

extern vulkan VK;

static format RenderTargetFormatTable[RTFormat_Count]
{
    [RTFormat_Undefined]    = Format_Undefined,

    [RTFormat_Depth]        = Format_D32,
    [RTFormat_HDR]          = Format_R16G16B16A16_Float,
    [RTFormat_Structure]    = Format_R16G16B16A16_Float,
    [RTFormat_Visibility]   = Format_R32G32_UInt,
    [RTFormat_Occlusion]    = Format_R8_UNorm,
    [RTFormat_Shadow]       = Format_D16,
    [RTFormat_Swapchain]    = Format_Undefined, // NOTE(boti): Actually filled at runtime
};

struct draw_list
{
    u32 DrawGroupDrawCounts[DrawGroup_Count];
    umm DrawBufferOffset;
};

struct point_shadow_map
{
    VkImage Image;
    VkImageView CubeView;
    VkImageView LayerViews[6];
};

struct pipeline_with_layout
{
    VkPipeline Pipeline;
    VkPipelineLayout Layout;
};

struct backend_render_frame
{
    VkCommandPool CmdPool;
    static constexpr u32 MaxCmdBufferCount = 16;
    u32 CmdBufferAt;
    VkCommandBuffer CmdBuffers[MaxCmdBufferCount];

    VkCommandPool ComputeCmdPool;
    VkCommandBuffer ComputeCmdBuffer;

    VkFence ImageAcquiredFence;
    VkSemaphore ImageAcquiredSemaphore;

    u64 FrameFinishedCounter;

    u32 SwapchainImageIndex;
    VkImage SwapchainImage;
    VkImageView SwapchainImageView;

    VkBuffer StagingBuffer;
    VkBuffer UniformBuffer;
    VkBuffer ParticleBuffer;
    VkBuffer JointBuffer;
    VkBuffer Vertex2DBuffer;
};

struct renderer
{
    vulkan Vulkan;

    VkSurfaceFormatKHR SurfaceFormat;
    VkSurfaceKHR Surface;

    static constexpr u32 MaxSwapchainImageCount = 8;
    u32 SwapchainImageCount;
    VkSwapchainKHR Swapchain;

    VkImage         SwapchainImages[MaxSwapchainImageCount];
    VkImageView     SwapchainImageViews[MaxSwapchainImageCount];

    VkAllocationCallbacks* Allocator;

    VkExtent2D SurfaceExtent;

    geometry_buffer GeometryBuffer;
    texture_manager TextureManager;
    gpu_memory_arena BARMemory;
    render_target_heap RenderTargetHeap;
    render_target* DepthBuffer;
    render_target* StructureBuffer;
    render_target* HDRRenderTarget;
    render_target* BloomTarget;
    render_target* OcclusionBuffers[2];
    render_target* VisibilityBuffer;

    VkPipelineLayout SystemPipelineLayout;

    gpu_memory_arena ShadowArena;
    VkImage CascadeMap;
    VkImageView CascadeArrayView;
    VkImageView CascadeViews[R_MaxShadowCount];
    point_shadow_map PointShadowMaps;

    struct render_debug
    {
        u64 ResizeCount;
    } Debug;

    VkCommandPool TransferCmdPool;
    VkCommandBuffer TransferCmdBuffer;

    //
    // Per frame stuff
    // 
    VkCommandPool CmdPools[2];
    VkCommandBuffer CmdBuffers[2][backend_render_frame::MaxCmdBufferCount];

    VkCommandPool ComputeCmdPools[2];
    VkCommandBuffer ComputeCmdBuffers[2];

    VkSemaphore ImageAcquiredSemaphores[R_MaxFramesInFlight];
    VkFence ImageAcquiredFences[R_MaxFramesInFlight];
    VkSemaphore TimelineSemaphore;
    u64 TimelineSemaphoreCounter;
    VkSemaphore ComputeTimelineSemaphore;
    u64 ComputeTimelineSemaphoreCounter;

    // NOTE(boti): These are all allocated from BAR memory
    VkBuffer    PerFrameUniformBuffers[R_MaxFramesInFlight];
    void*       PerFrameUniformBufferMappings[R_MaxFramesInFlight];
    VkBuffer    PerFrameJointBuffers[R_MaxFramesInFlight];
    void*       PerFrameJointBufferMappings[R_MaxFramesInFlight];
    VkBuffer    PerFrameParticleBuffers[R_MaxFramesInFlight];
    void*       PerFrameParticleBufferMappings[R_MaxFramesInFlight];
    VkBuffer    PerFrameVertex2DBuffers[R_MaxFramesInFlight];
    void*       PerFrameVertex2DMappings[R_MaxFramesInFlight];
    VkBuffer    PerFrameResourceDescriptorBuffers[R_MaxFramesInFlight];
    void*       PerFrameResourceDescriptorMappings[R_MaxFramesInFlight];

    VkDeviceMemory StagingMemory;
    void* StagingMemoryMapping;
    VkBuffer StagingBuffers[R_MaxFramesInFlight];

    VkBuffer DesiredMipReadbackBuffers[R_MaxFramesInFlight];
    void* DesiredMipReadbackMappings[R_MaxFramesInFlight];

    VkDeviceMemory DesiredMipReadbackMemory;
    void* DesiredMipReadbackMapping;

    //
    // Persistent
    //
    VkBuffer        SamplerDescriptorBuffer;
    void*           SamplerDescriptorMapping;
    VkBuffer        StaticResourceDescriptorBuffer;
    void*           StaticResourceDescriptorMapping;

    umm             DesiredMipMemorySize;
    VkDeviceMemory  DesiredMipMemory;
    VkBuffer        DesiredMipBuffer;

    umm             SkinningMemorySize;
    VkDeviceMemory  SkinningMemory;
    VkBuffer        SkinningBuffer;

    umm             LightBufferMemorySize;
    VkDeviceMemory  LightBufferMemory;
    VkBuffer        LightBuffer;

    umm             TileMemorySize;
    VkDeviceMemory  TileMemory;
    VkBuffer        TileBuffer;

    umm             InstanceMemorySize;
    VkDeviceMemory  InstanceMemory;
    VkBuffer        InstanceBuffer;

    umm             DrawMemorySize;
    VkDeviceMemory  DrawMemory;
    VkBuffer        DrawBuffer;

    //
    // Pipelines, pipeline layouts and associated descriptor set layouts
    //
    pipeline_with_layout Pipelines[Pipeline_Count];
    VkDescriptorSetLayout SetLayouts[Set_Count];
    VkSampler Samplers[Sampler_Count];

    u64 CurrentFrameID;
    render_frame Frames[R_MaxFramesInFlight];
    backend_render_frame BackendFrames[R_MaxFramesInFlight];
};

internal void 
SetupSceneRendering(render_frame* Frame, frustum* CascadeFrustums);

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

// NOTE(boti): See Vulkan spec. 16.5.4. Table 17. to find where these transforms come from
m3 GlobalCubeFaceBases[Layer_Count]
{
    // +X
    {
        .X = {  0.0f,  0.0f, -1.0f },
        .Y = {  0.0f, -1.0f,  0.0f },
        .Z = { +1.0f,  0.0f,  0.0f },
    },
    // -X
    {
        .X = {  0.0f,  0.0f, +1.0f },
        .Y = {  0.0f, -1.0f,  0.0f },
        .Z = { -1.0f,  0.0f,  0.0f },
    },
    // +Y
    {
        .X = { +1.0f,  0.0f,  0.0f },
        .Y = {  0.0f,  0.0f, +1.0f },
        .Z = {  0.0f, +1.0f,  0.0f },
    },
    // -Y
    {
        .X = { +1.0f,  0.0f,  0.0f },
        .Y = {  0.0f,  0.0f, -1.0f },
        .Z = {  0.0f, -1.0f,  0.0f },
    },
    // +Z
    {
        .X = { +1.0f,  0.0f,  0.0f },
        .Y = {  0.0f, -1.0f,  0.0f },
        .Z = {  0.0f,  0.0f, +1.0f },
    },
    // -Z
    {
        .X = { -1.0f,  0.0f,  0.0f },
        .Y = {  0.0f, -1.0f,  0.0f },
        .Z = {  0.0f,  0.0f, -1.0f },
    },
};