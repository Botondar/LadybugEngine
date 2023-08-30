#pragma once

#include "Renderer/Renderer.hpp"
#include "Renderer/Pipelines.hpp"

#include <vulkan/vulkan.h>

//
// TODO(boti): Rework the relationship between SwapchainImageCount and the number of 
// frames in flight, we're in a mess right now
//

struct vulkan_buffer
{
    size_t Size;
    VkDeviceMemory Memory;
    VkBuffer Buffer;

    size_t Offset;
    void* Mapping;
};

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

#include "Renderer/RenderDevice.hpp"
#include "Renderer/RenderTarget.hpp"
#include "Renderer/Geometry.hpp"
#include "Renderer/TextureManager.hpp"
#include "Renderer/Frame.hpp"
#include "Platform.hpp"

extern vulkan VK;

struct screen_tile
{
    u32 LightCount;
    u32 LightIndices[R_MaxLightCountPerTile];
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

    VkDescriptorPool DescriptorPool;
    VkDescriptorSet UniformDescriptorSet;

    VkFence ImageAcquiredFence;
    VkSemaphore ImageAcquiredSemaphore;

    u64 FrameFinishedCounter;

    u32 SwapchainImageIndex;
    VkImage SwapchainImage;
    VkImageView SwapchainImageView;
    render_target* DepthBuffer;
    render_target* StructureBuffer;
    render_target* HDRRenderTargets[2];
    render_target* OcclusionBuffers[2];

    u32 ShadowCascadeCount;
    VkImage ShadowMap;
    VkImageView ShadowMapView;
    VkImageView ShadowCascadeViews[R_MaxShadowCascadeCount];

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

    static constexpr u32 MaxSwapchainImageCount = 2;
    u32 SwapchainImageCount;
    VkSwapchainKHR Swapchain;

    VkImage SwapchainImages[MaxSwapchainImageCount];
    VkImageView SwapchainImageViews[MaxSwapchainImageCount];

    VkAllocationCallbacks* Allocator;

    VkExtent2D SurfaceExtent;

    render_target_heap RenderTargetHeap;
    vulkan_buffer StagingBuffer;
    geometry_buffer GeometryBuffer;
    texture_manager TextureManager;
    gpu_memory_arena BARMemory;

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
    VkBuffer PerFrameUniformBuffers[MaxSwapchainImageCount];
    void* PerFrameUniformBufferMappings[MaxSwapchainImageCount];
    VkBuffer PerFrameJointBuffers[MaxSwapchainImageCount];
    void* PerFrameJointBufferMappings[MaxSwapchainImageCount];
    VkBuffer PerFrameParticleBuffers[MaxSwapchainImageCount];
    void* PerFrameParticleBufferMappings[MaxSwapchainImageCount];
    VkBuffer PerFrameVertex2DBuffers[MaxSwapchainImageCount];
    void* PerFrameVertex2DMappings[MaxSwapchainImageCount];

    VkDeviceMemory StagingMemory;
    void* StagingMemoryMapping;
    VkBuffer StagingBuffers[MaxSwapchainImageCount];

    umm SkinningMemorySize;
    VkDeviceMemory SkinningMemory;
    VkBuffer SkinningBuffer;

    umm LightBufferMemorySize;
    VkDeviceMemory LightBufferMemory;
    VkBuffer LightBuffer;

    umm TileMemorySize;
    VkDeviceMemory TileMemory;
    VkBuffer TileBuffer;

    VkSemaphore ImageAcquiredSemaphores[MaxSwapchainImageCount];
    VkFence ImageAcquiredFences[MaxSwapchainImageCount];
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
    render_frame Frames[MaxSwapchainImageCount];
    backend_render_frame BackendFrames[MaxSwapchainImageCount];
};

lbfn void SetupSceneRendering(render_frame* Frame);
