#include "Renderer/Renderer.hpp"
#include "Renderer/Pipelines.hpp"

#include <vulkan/vulkan.h>

PFN_vkCmdBeginDebugUtilsLabelEXT        vkCmdBeginDebugUtilsLabelEXT_;
PFN_vkCmdEndDebugUtilsLabelEXT          vkCmdEndDebugUtilsLabelEXT_;
//#define vkCmdBeginDebugUtilsLabelEXT    vkCmdBeginDebugUtilsLabelEXT_
#define vkCmdEndDebugUtilsLabelEXT      vkCmdEndDebugUtilsLabelEXT_

inline void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer CmdBuffer, const char* Label);

// TODO(boti): Maybe we should just have an indirected enum for this too?
constexpr format DEPTH_FORMAT = Format_D32;
constexpr format HDR_FORMAT = Format_R16G16B16A16_Float;
constexpr format STRUCTURE_BUFFER_FORMAT = Format_R16G16B16A16_Float;
constexpr format SSAO_FORMAT = Format_R8_UNorm;
constexpr format SHADOW_FORMAT = Format_D16;
// HACK(boti): The swapchain format is unknown at compile time, so we use this special value to refer to it
constexpr format SWAPCHAIN_FORMAT = (format)0xFFFFFFFF;

struct gpu_memory_arena
{
    VkDeviceMemory Memory;
    u64 Size;
    u64 MemoryAt;
    u32 MemoryTypeIndex;
    void* Mapping;
};

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

    VkDescriptorPool DescriptorPool;
    VkDescriptorSet UniformDescriptorSet;

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
    VkBuffer SkinnedMeshVB;
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

    struct render_debug
    {
        u64 ResizeCount;
    } Debug;

    // Shadows
    struct
    {
        size_t ShadowMemorySize;
        VkDeviceMemory ShadowMemory;
        size_t ShadowMemoryOffset;

        u32 ShadowCascadeCount;
        VkImage ShadowMap;
        VkImageView ShadowView;
        VkImageView ShadowCascadeViews[R_MaxShadowCascadeCount];

        point_shadow_map PointShadowMaps[R_MaxShadowCount];
    };

    VkCommandPool TransferCmdPool;
    VkCommandBuffer TransferCmdBuffer;

    //
    // Per frame stuff
    // 
    VkCommandPool CmdPools[2];
    VkCommandBuffer CmdBuffers[2][backend_render_frame::MaxCmdBufferCount];

    VkCommandPool ComputeCmdPools[2];
    VkCommandBuffer ComputeCmdBuffers[2];

    static constexpr u32 MaxPerFrameDescriptorSetCount = 1024;
    VkDescriptorPool PerFrameDescriptorPool[2];

    // NOTE(boti): These are all allocated from BAR memory
    // TODO(boti): These memory mappings should probably just be stored in render_frame directly
    VkBuffer    PerFrameUniformBuffers[R_MaxFramesInFlight];
    void*       PerFrameUniformBufferMappings[R_MaxFramesInFlight];
    VkBuffer    PerFrameJointBuffers[R_MaxFramesInFlight];
    void*       PerFrameJointBufferMappings[R_MaxFramesInFlight];
    VkBuffer    PerFrameParticleBuffers[R_MaxFramesInFlight];
    void*       PerFrameParticleBufferMappings[R_MaxFramesInFlight];
    VkBuffer    PerFrameVertex2DBuffers[R_MaxFramesInFlight];
    void*       PerFrameVertex2DMappings[R_MaxFramesInFlight];

    VkDeviceMemory StagingMemory;
    void* StagingMemoryMapping;
    VkBuffer StagingBuffers[R_MaxFramesInFlight];

    umm SkinningMemorySize;
    VkDeviceMemory SkinningMemory;
    VkBuffer SkinningBuffer;

    umm LightBufferMemorySize;
    VkDeviceMemory LightBufferMemory;
    VkBuffer LightBuffer;

    umm TileMemorySize;
    VkDeviceMemory TileMemory;
    VkBuffer TileBuffer;

    VkSemaphore ImageAcquiredSemaphores[R_MaxFramesInFlight];
    VkFence ImageAcquiredFences[R_MaxFramesInFlight];
    VkSemaphore TimelineSemaphore;
    u64 TimelineSemaphoreCounter;
    VkSemaphore ComputeTimelineSemaphore;
    u64 ComputeTimelineSemaphoreCounter;

    //
    // Pipelines, pipeline layouts and associated descriptor set layouts
    //
    pipeline_with_layout Pipelines[Pipeline_Count];
    VkDescriptorSetLayout SetLayouts[SetLayout_Count];
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