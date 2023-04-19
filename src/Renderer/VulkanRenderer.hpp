#pragma once

#include "LadybugLib/Core.hpp"
#include "Renderer/Renderer.hpp"

//
// Config
//

constexpr u64 R_RenderTargetMemorySize  = MiB(320);
constexpr u64 R_TextureMemorySize       = MiB(1024llu);
constexpr u64 R_ShadowMapMemorySize     = MiB(256);
constexpr u32 R_MaxShadowCascadeCount   = 4;
constexpr u32 R_ShadowResolution        = 2048;
constexpr u64 R_VertexBufferMaxBlockCount = (1llu << 18);

enum descriptor_set : u32
{
    DescriptorSet_None = 0,

    DescriptorSet_PerFrameUniformData,

    DescriptorSet_Count,
};

#include <vulkan/vulkan.h>

#include "Renderer/RenderPipelines.hpp"

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
#include "Renderer/Shadow.hpp"
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

struct vulkan_renderer
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
#if 0
        VkImage ShadowMap;
        VkImageView ShadowView;
        VkImageView ShadowCascadeViews[R_MaxShadowCascadeCount];
#else
        VkImage ShadowMaps[MaxSwapchainImageCount];
        VkImageView ShadowViews[MaxSwapchainImageCount];
        VkImageView ShadowCascadeViews[MaxSwapchainImageCount][R_MaxShadowCascadeCount];
#endif

        VkSampler ShadowSampler;
        VkDescriptorSetLayout ShadowDescriptorSetLayout;
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
    VkDescriptorSetLayout FrameUniformDescriptorSetLayout;

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

    bloom_desc Bloom;
    ssao_desc SSAO;

    VkSampler RenderTargetSampler;    

    VkDescriptorSetLayout GBufferDescriptorSetLayout; // NOTE(boti): G-buffers needed for rendering

    VkDescriptorSetLayout BlitDescriptorSetLayout;
    VkSampler BlitSampler;

    //
    // Fonts
    //

    // TODO(boti): remove these once we have texture handling in place
    VkDeviceMemory FontImageMemory;
    VkImage FontImage;
    VkImageView FontImageView;
    VkSampler FontSampler;
    
    VkDescriptorPool FontDescriptorPool;
    VkDescriptorSetLayout FontDescriptorSetLayout;
    VkDescriptorSet FontDescriptorSet;

    u64 CurrentFrameID;
    render_frame Frames[MaxSwapchainImageCount];
};

lbfn VkResult CreateRenderer(vulkan_renderer* Renderer, 
                             memory_arena* Arena, 
                             memory_arena* TempArena);

lbfn VkResult ResizeRenderTargets(vulkan_renderer* Renderer);

lbfn geometry_buffer_allocation UploadVertexData(vulkan_renderer* Renderer, 
                                                 u32 VertexCount, const vertex* VertexData,
                                                 u32 IndexCount, const vert_index* IndexData);
lbfn texture_id PushTexture(vulkan_renderer* Renderer, u32 Width, u32 Height, u32 MipCount, const void* Data, VkFormat Format);

lbfn void CreateDebugFontImage(vulkan_renderer* Renderer, u32 Width, u32 Height, const void* Texels);

//
// Render API
//

// NOTE(boti): returns the current swapchain image index
lbfn render_frame* BeginRenderFrame(vulkan_renderer* Renderer);