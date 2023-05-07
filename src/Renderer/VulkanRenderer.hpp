#pragma once

#include <vulkan/vulkan.h>

#include "Renderer/Pipelines.hpp"

//
// Buffers
//
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

extern vulkan VK;

// Forward declarations
struct font;
struct render_pipeline_info;
struct render_pass_info;

struct pipeline_with_layout
{
    VkPipeline Pipeline;
    VkPipelineLayout Layout;
};

struct renderer
{
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

    VkDeviceMemory PerFrameUniformMemory;
    void* PerFrameUniformMappingBase;
    VkBuffer PerFrameUniformBuffers[MaxSwapchainImageCount];
    void* PerFrameUniformBufferMappings[MaxSwapchainImageCount];

    VkSemaphore ImageAcquiredSemaphores[MaxSwapchainImageCount];
    VkFence ImageAcquiredFences[MaxSwapchainImageCount];
    VkFence RenderFinishedFences[MaxSwapchainImageCount];

    vulkan_buffer DrawBuffers[MaxSwapchainImageCount];
    vulkan_buffer VertexStacks[MaxSwapchainImageCount];

    //
    // Pipelines, pipeline layouts and associated descriptor set layouts
    //
    pipeline_with_layout Pipelines[Pipeline_Count];
    VkDescriptorSetLayout SetLayouts[SetLayout_Count];
    VkSampler Samplers[Sampler_Count];

    u64 CurrentFrameID;
    render_frame Frames[MaxSwapchainImageCount];
};

VkResult CreateRenderer(renderer* Renderer, 
                        memory_arena* Arena, 
                        memory_arena* TempArena);

geometry_buffer_allocation UploadVertexData(renderer* Renderer, 
                                                 u32 VertexCount, const vertex* VertexData,
                                                 u32 IndexCount, const vert_index* IndexData);
texture_id PushTexture(renderer* Renderer, u32 Width, u32 Height, u32 MipCount, const void* Data, VkFormat Format, texture_swizzle Swizzle);
