#pragma once

#include <vulkan/vulkan.h>

struct vulkan_buffer
{
    size_t Size;
    VkDeviceMemory Memory;
    VkBuffer Buffer;

    size_t Offset;
    void* Mapping;
};

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

struct pipeline_with_layout
{
    VkPipeline Pipeline;
    VkPipelineLayout Layout;
};

struct backend_render_frame
{
    VkCommandPool CmdPool;
    VkCommandBuffer CmdBuffer;

    VkDescriptorPool DescriptorPool;
    VkDescriptorSet UniformDescriptorSet;

    VkSemaphore ImageAcquiredSemaphore;
    VkFence ImageAcquiredFence;
    VkFence RenderFinishedFence;

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
    VkBuffer Draw2DCmdBuffer;
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
    };

    VkCommandPool TransferCmdPool;
    VkCommandBuffer TransferCmdBuffer;

    //
    // Per frame stuff
    // 
    VkCommandPool CmdPools[2];
    VkCommandBuffer CmdBuffers[2];

    static constexpr u32 MaxPerFrameDescriptorSetCount = 1024;
    VkDescriptorPool PerFrameDescriptorPool[2];

    VkDeviceSize BARMemoryByteSize;
    VkDeviceSize BARMemoryByteOffset;
    VkDeviceMemory BARMemory;
    void* BARMemoryMapping;

    // NOTE(boti): These are all allocated from BAR memory
    // TODO(boti): These memory mappings should probably just be stored in render_frame directly
    VkBuffer PerFrameUniformBuffers[MaxSwapchainImageCount];
    void* PerFrameUniformBufferMappings[MaxSwapchainImageCount];
    VkBuffer PerFrameJointBuffers[MaxSwapchainImageCount];
    void* PerFrameJointBufferMappings[MaxSwapchainImageCount];
    VkBuffer PerFrameParticleBuffers[MaxSwapchainImageCount];
    void* PerFrameParticleBufferMappings[MaxSwapchainImageCount];
    VkBuffer PerFrameDraw2DCmdBuffers[MaxSwapchainImageCount];
    void* PerFrameDraw2DCmdBufferMappings[MaxSwapchainImageCount];
    VkBuffer PerFrameVertex2DBuffers[MaxSwapchainImageCount];
    void* PerFrameVertex2DMappings[MaxSwapchainImageCount];

    // TODO(boti): this probably doesn't need to be duplicated among frames
    // because the GPU timelines don't actually overlap.
    // In the same vein, using this as a ring-buffer should work.
    VkDeviceMemory SkinningMemory;
    VkBuffer SkinningBuffers[MaxSwapchainImageCount];

    VkDeviceMemory StagingMemory;
    void* StagingMemoryMapping;
    VkBuffer StagingBuffers[MaxSwapchainImageCount];

    VkDeviceMemory LightBufferMemory;
    VkBuffer LightBuffer;

    VkSemaphore ImageAcquiredSemaphores[MaxSwapchainImageCount];
    VkFence ImageAcquiredFences[MaxSwapchainImageCount];
    VkFence RenderFinishedFences[MaxSwapchainImageCount];

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

lbfn void BeginSceneRendering(render_frame* Frame);
lbfn void EndSceneRendering(render_frame* Frame);
