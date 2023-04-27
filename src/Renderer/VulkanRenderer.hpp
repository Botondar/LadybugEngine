#pragma once

#include <vulkan/vulkan.h>

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

struct pipeline_layout_info
{
    u32 PushConstantRangeCount;
    u32 DescriptorSetCount;
    VkPushConstantRange PushConstantRanges[MaxPushConstantRangeCount];
    u32 DescriptorSets[MaxDescriptorSetCount];
};


enum pipeline_type : u32
{
    PipelineType_Graphics = 0,
    PipelineType_Compute,

    PipelineType_Count,
};

enum pipeline_stage : flags32
{
    PipelineStage_None = 0,

    PipelineStage_VS = (1 << 0),
    PipelineStage_PS = (1 << 1),
    PipelineStage_DS = (1 << 2),
    PipelineStage_HS = (1 << 3),
    PipelineStage_GS = (1 << 4),

    PipelineStage_CS = (1 << 5),

    PipelineStage_Count = 6, // NOTE(boti): This has to be kept up to date !!!
};

// NOTE(boti): binding index is implicit from what index this structure is at in input_assembler_state
struct vertex_input_binding
{
    u32 Stride;
    b32 IsPerInstance;
};

struct vertex_input_attrib
{
    u32 Index;
    u32 BindingIndex;
    VkFormat Format;
    u32 Offset;
};

struct input_assembler_state
{
    VkPrimitiveTopology PrimitiveTopology;
    b32 EnablePrimitiveRestart;
    u32 BindingCount;
    u32 AttribCount;
    vertex_input_binding Bindings[MaxVertexInputBindingCount];
    vertex_input_attrib Attribs[MaxVertexInputAttribCount];
};

enum rasterizer_flags : flags32
{
    RS_Flags_None = 0,

    RS_DepthClampEnable = (1 << 0),
    RS_DiscardEnable = (1 << 1),
    RS_DepthBiasEnable = (1 << 2),
};

struct rasterizer_state
{
    flags32 Flags;
    VkPolygonMode PolygonMode;
    VkCullModeFlags CullFlags;
    VkFrontFace FrontFace;

    f32 DepthBiasConstantFactor;
    f32 DepthBiasClamp;
    f32 DepthBiasSlopeFactor;
};

enum depth_stencil_flags : flags32
{
    DS_Flags_None = 0,

    DS_DepthTestEnable = (1 << 0),
    DS_DepthWriteEnable = (1 << 1),
    DS_DepthBoundsTestEnable = (1 << 2),
    DS_StencilTestEnable = (1 << 3),
};

struct depth_stencil_state
{
    flags32 Flags;

    VkCompareOp DepthCompareOp;
    VkStencilOpState StencilFront;
    VkStencilOpState StencilBack;
    f32 MinDepthBounds;
    f32 MaxDepthBounds;
};

struct blend_attachment
{
    b32 BlendEnable;
    VkBlendFactor SrcColorFactor;
    VkBlendFactor DstColorFactor;
    VkBlendOp ColorOp;
    VkBlendFactor SrcAlphaFactor;
    VkBlendFactor DstAlphaFactor;
    VkBlendOp AlphaOp;
};

struct pipeline_info
{
    const char* Name;
    pipeline_type Type;

    pipeline_layout_info Layout;

    // Gfx pipeline specific
    flags32 EnabledStages;

    input_assembler_state InputAssemblerState;
    rasterizer_state RasterizerState;
    depth_stencil_state DepthStencilState;
    u32 BlendAttachmentCount;
    blend_attachment BlendAttachments[MaxColorAttachmentCount];

    u32 ColorAttachmentCount;
    VkFormat ColorAttachments[MaxColorAttachmentCount];
    VkFormat DepthAttachment;
    VkFormat StencilAttachment;
};

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

VkResult CreateRenderer(renderer* Renderer, 
                        memory_arena* Arena, 
                        memory_arena* TempArena);

geometry_buffer_allocation UploadVertexData(renderer* Renderer, 
                                                 u32 VertexCount, const vertex* VertexData,
                                                 u32 IndexCount, const vert_index* IndexData);
texture_id PushTexture(renderer* Renderer, u32 Width, u32 Height, u32 MipCount, const void* Data, VkFormat Format);

void CreateDebugFontImage(renderer* Renderer, u32 Width, u32 Height, const void* Texels);
