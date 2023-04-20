#pragma once

#include "LadybugLib/Core.hpp"
#include "Renderer/Renderer.hpp"

//
// Formats
//

// TODO(boti): Maybe we should just have an indirected enum for this too?
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
constexpr VkFormat HDR_FORMAT = VK_FORMAT_B10G11R11_UFLOAT_PACK32; //VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat STRUCTURE_BUFFER_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat SSAO_FORMAT = VK_FORMAT_R8_UNORM;
constexpr VkFormat SHADOW_FORMAT = VK_FORMAT_D16_UNORM; // TODO(boti): is this enough?
// HACK(boti): The swapchain format is unknown at compile time, so we use this special value to refer to it
constexpr VkFormat SWAPCHAIN_FORMAT = (VkFormat)0xFFFFFFFF;

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
    SetLayout_SampledRenderTargetNormalized_PS_CS,
    SetLayout_StorageImage_CS,
    SetLayout_Bloom,
    SetLayout_SSAO,
    SetLayout_SSAOBlur,

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
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,
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
                .StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
        },
    },

    // StorageImage_CS
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 1,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    // Bloom
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetNormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },
    // SSAO
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 2,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
            },
        },
    },

    //SetLayout_SSAOBlur
    {
        .Flags = SetLayoutFlag_None,
        .BindingCount = 3,
        .Bindings = 
        {
            {
                .Binding = 0,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 1,
                .Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_RenderTargetUnnormalized,
            },
            {
                .Binding = 2,
                .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .DescriptorCount = 1,
                .StageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .ImmutableSampler = Sampler_None,
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