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

// Shader limits
constexpr u32 MaxDescriptorSetCount = 256;
constexpr u32 MaxPushConstantRangeCount = 64;

constexpr u32 MaxDescriptorSetLayoutBindingCount = 32;

enum sampler : u32
{
    Sampler_None = 0,

    Sampler_Default,
    Sampler_RenderTargetUnnormalized,
    Sampler_Shadow,
    Sampler_RenderTargetNormalized,

    Sampler_Count,
};

static const VkSamplerCreateInfo SamplerInfos[Sampler_Count] = 
{
    {},

    // Default
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 4.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    },
    // RenderTargetUnnormalized
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_TRUE,
    },
    // Shadow
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_TRUE,
        .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    },
    // RenderTargetNormalized
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    },
};

enum descriptor_set_layout : u32
{
    SetLayout_None = 0,

    SetLayout_PerFrameUniformData,
    SetLayout_BindlessTexturesPS,
    SetLayout_SampledRenderTargetPS,
    SetLayout_DefaultSamplerPS,
    SetLayout_ShadowPS,
    SetLayout_SampledRenderTargetNormalizedPS,

    SetLayout_Count,
};

enum descriptor_set_layout_flags : flags32
{
    SetLayoutFlag_None = 0,

    SetLayoutFlag_UpdateAfterBind = (1 << 0),
    SetLayoutFlag_Bindless = (2 << 0),
};

struct descriptor_set_binding
{
    u32 Binding;
    VkDescriptorType Type;
    u32 DescriptorCount;
    flags32 StageFlags;
    u32 ImmutableSampler; // NOTE(boti): for now we only allow single immutable samplers
};

struct descriptor_set_layout_info
{
    flags32 Flags; // descriptor_set_layout_flags
    u32 BindingCount;
    descriptor_set_binding Bindings[MaxDescriptorSetLayoutBindingCount];
};

static const descriptor_set_layout_info SetLayoutInfos[SetLayout_Count] = 
{
    {},

    // PerFrameUniformData
    {
        .Flags = SetLayoutFlag_UpdateAfterBind,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_ALL,
                .ImmutableSampler = Sampler_None,
            },
        },
    },
    // BindlessTexturesPS
    {
        .Flags = SetLayoutFlag_UpdateAfterBind|SetLayoutFlag_Bindless,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .DescriptorCount = 0, // NOTE(boti): actual count is implied by the descriptor pool size for bindless
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    // SampledRenderTargetPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
        },
    },

    // DefaultSamplerPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .ImmutableSampler = Sampler_Default,
            },
        },
    },

    // ShadowPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_VERTEX_BIT,
                .ImmutableSampler = Sampler_Shadow,
            },
        },
    },

    // SampledRenderTargetNormalizedPS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
        },
    },
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

    bloom_desc Bloom;
    ssao_desc SSAO;

    //
    // Fonts
    //

    // TODO(boti): remove these once we have texture handling in place
    VkDeviceMemory FontImageMemory;
    VkImage FontImage;
    VkImageView FontImageView;
    
    VkDescriptorPool FontDescriptorPool;
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