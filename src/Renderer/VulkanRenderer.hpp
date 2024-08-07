/*
 * TODO list:
 * - Cleanup subresource handling
 * - Make texture_info always store the info about entire image, not the currently present mip level
 * - Make the texture cache only discard images when more space is needed (Thresholding?)
 * - Unify shadow rendering
 * - Shader variants and draw group for alpha tested rendering
 * - Add mip feedback to alpha-tested prepass and shadow passes
 */

#include "Renderer/Renderer.hpp"
#include "Renderer/Pipelines.hpp"
#include "Renderer/rhi_vulkan.hpp"

struct gpu_memory_arena
{
    VkDeviceMemory Memory;
    umm Size;
    umm MemoryAt;
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

internal void ResetGPUArena(gpu_memory_arena* Arena);

internal b32 
PushImage(gpu_memory_arena* Arena, VkImage Image);

internal b32 
PushBuffer(gpu_memory_arena* Arena, 
           VkDeviceSize Size, 
           VkBufferUsageFlags Usage, 
           VkBuffer* Buffer, 
           void** Mapping = nullptr);

extern m3 GlobalCubeFaceBases[CubeLayer_Count];

enum vulkan_handle_type : u32
{
    VulkanHandle_Undefined = 0,
    VulkanHandle_Memory,
    VulkanHandle_Buffer,
    VulkanHandle_Image,
    VulkanHandle_ImageView,
    VulkanHandle_Swapchain,
};

struct deletion_entry
{
    vulkan_handle_type Type;
    union
    {
        void*           Handle;
        VkDeviceMemory  Memory;
        VkBuffer        Buffer;
        VkImage         Image;
        VkImageView     ImageView;
        VkSwapchainKHR  Swapchain;
    };
};

struct gpu_deletion_queue
{
    static constexpr u32 MaxEntryCount = 4096;
    u32 EntryWriteAt;
    u32 EntryReadAt;
    u32 FrameEntryWriteAt[R_MaxFramesInFlight];

    deletion_entry Entries[MaxEntryCount];
};

internal void 
ProcessDeletionEntries(gpu_deletion_queue* Queue, u32 FrameID);

internal b32 
PushDeletionEntry(gpu_deletion_queue* Queue, u32 FrameID, vulkan_handle_type Type, void* Handle);

inline b32 PushDeletionEntry(gpu_deletion_queue* Queue, u32 FrameID, VkDeviceMemory Memory)
{ return PushDeletionEntry(Queue, FrameID, VulkanHandle_Memory, Memory); }
inline b32 PushDeletionEntry(gpu_deletion_queue* Queue, u32 FrameID, VkBuffer Buffer)
{ return PushDeletionEntry(Queue, FrameID, VulkanHandle_Buffer, Buffer); }
inline b32 PushDeletionEntry(gpu_deletion_queue* Queue, u32 FrameID, VkImage Image)
{ return PushDeletionEntry(Queue, FrameID, VulkanHandle_Image, Image); }
inline b32 PushDeletionEntry(gpu_deletion_queue* Queue, u32 FrameID, VkImageView ImageView)
{ return PushDeletionEntry(Queue, FrameID, VulkanHandle_ImageView, ImageView); }
inline b32 PushDeletionEntry(gpu_deletion_queue* Queue, u32 FrameID, VkSwapchainKHR Swapchain)
{ return PushDeletionEntry(Queue, FrameID, VulkanHandle_Swapchain, Swapchain); }

#include "Renderer/RenderDevice.hpp"
#include "Renderer/RenderTarget.hpp"
#include "Renderer/Geometry.hpp"
#include "Renderer/TextureManager.hpp"
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

enum frame_stage_type : u32
{
    FrameStage_Upload,
    FrameStage_Skinning,
    FrameStage_Prepass,
    FrameStage_SSAO,
    FrameStage_LightBinning,
    FrameStage_CascadedShadow,
    FrameStage_Shadows,
    FrameStage_Shading,
    FrameStage_Bloom,
    FrameStage_BlitAndGUI,
    FrameStage_Readback,

    FrameStage_Count,
};

static const char* FrameStageNames[FrameStage_Count] =
{
    [FrameStage_Upload]         = "Upload",
    [FrameStage_Skinning]       = "Skinning",
    [FrameStage_Prepass]        = "Prepass",
    [FrameStage_SSAO]           = "SSAO",
    [FrameStage_LightBinning]   = "LightBinning",
    [FrameStage_CascadedShadow] = "CSM",
    [FrameStage_Shadows]        = "Shadows",
    [FrameStage_Shading]        = "Shading",
    [FrameStage_Bloom]          = "Bloom",
    [FrameStage_BlitAndGUI]     = "BlitAndGUI",
    [FrameStage_Readback]       = "Readback",
};

// NOTE(boti): Stage barriers are executed at the beginning of a stage
struct frame_stage
{
    static constexpr u32 MaxGlobalMemoryBarrierCount    = 8;
    static constexpr u32 MaxBufferMemoryBarrierCount    = 64;
    static constexpr u32 MaxImageMemoryBarrierCount     = 64;
    
    u32 GlobalMemoryBarrierCount;
    u32 BufferMemoryBarrierCount;
    u32 ImageMemoryBarrierCount;

    VkMemoryBarrier2        GlobalMemoryBarriers[MaxGlobalMemoryBarrierCount];
    VkBufferMemoryBarrier2  BufferMemoryBarriers[MaxBufferMemoryBarrierCount];
    VkImageMemoryBarrier2   ImageMemoryBarriers[MaxImageMemoryBarrierCount];
};

internal void BeginFrameStage(VkCommandBuffer CmdBuffer, frame_stage_type Stage, VkQueryPool PerfQueries, frame_stage* Stages);
internal void EndFrameStage(VkCommandBuffer CmdBuffer, frame_stage_type Stage, VkQueryPool PerfQueries, frame_stage* Stages);

internal b32 PushBeginBarrier(frame_stage* Stage, const VkImageMemoryBarrier2* Barrier);
internal b32 PushBeginBarrier(frame_stage* Stage, const VkBufferMemoryBarrier2* Barrier);
internal b32 PushBeginBarrier(frame_stage* Stage, const VkMemoryBarrier2* Barrier);

struct draw_list
{
    VkBuffer DrawBuffer;
    umm DrawBufferOffset;
    u32 DrawGroupDrawCounts[DrawGroup_Count];
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

struct renderer
{
    vulkan Vulkan;

    VkQueryPool PerformanceQueryPools[R_MaxFramesInFlight];

    VkSurfaceFormatKHR SurfaceFormat;
    VkSurfaceKHR Surface;

    static constexpr u32 MaxSwapchainImageCount = 8;
    u32 SwapchainImageCount;
    VkSwapchainKHR Swapchain;

    VkImage         SwapchainImages[MaxSwapchainImageCount];
    VkImageView     SwapchainImageViews[MaxSwapchainImageCount];
    // NOTE(boti): Separate depth buffer for after-blit/GUI rendering
    // TODO(boti): We could just allocate the actual RT depth buffer at full resolution and only render to a portion of it
    VkImage         SwapchainDepthImage;
    VkImageView     SwapchainDepthImageView;
    VkDeviceMemory  SwapchainDepthMemory;

    VkAllocationCallbacks* Allocator;

    VkExtent2D SurfaceExtent;

    geometry_buffer GeometryBuffer;
    texture_manager TextureManager;
    gpu_memory_arena BARMemory;
    render_target_heap RenderTargetHeap;
    render_target* DepthBuffer;
    render_target* StructureBuffer;
    render_target* HDRRenderTarget;
    render_target* TransparentRenderTarget;
    render_target* BloomTarget;
    render_target* OcclusionBuffers[2];
    render_target* VisibilityBuffer;

    gpu_memory_arena DeviceArena;

    b32             EnvironmentBRDFReady;
    v2u             EnvironmentBRDFResolution;
    VkDeviceMemory  EnvironmentBRDFMemory;
    VkImage         EnvironmentBRDFImage;
    VkImageView     EnvironmentBRDFImageView;

    b32             SkyReady;
    v2u             SkyResolution;
    VkDeviceMemory  SkyImageMemory;
    VkImage         SkyImage;
    VkImageView     SkyImageView;

    VkPipelineLayout SystemPipelineLayout;

    gpu_memory_arena        ShadowArena;
    VkImage                 CascadeMap;
    VkImageView             CascadeArrayView;
    VkImageView             CascadeViews[R_MaxShadowCascadeCount];
    point_shadow_map        PointShadowMaps[R_MaxShadowCount];

    struct render_debug
    {
        u64 ResizeCount;
    } Debug;

    gpu_deletion_queue DeletionQueue;

    //
    // Per frame stuff
    // 

    // TODO(boti): This is mutable, per-frame state that should be part of render_frame,
    // but it's internal use only, so I don't want to expose it in the API
    u32 CmdBufferAt;
    u32 SecondaryCmdBufferAt;

    static constexpr u32    MaxCmdBufferCountPerFrame           = 16;
    static constexpr u32    MaxSecondaryCmdBufferCountPerFrame  = 32;
    VkCommandPool           CmdPools[R_MaxFramesInFlight];
    VkCommandBuffer         CmdBuffers[R_MaxFramesInFlight][MaxCmdBufferCountPerFrame];
    VkCommandBuffer         SecondaryCmdBuffers[R_MaxFramesInFlight][MaxSecondaryCmdBufferCountPerFrame];

    VkCommandPool   ComputeCmdPools[R_MaxFramesInFlight];
    VkCommandBuffer ComputeCmdBuffers[R_MaxFramesInFlight];

    VkSemaphore     ImageAcquiredSemaphores[R_MaxFramesInFlight];
    VkFence         ImageAcquiredFences[R_MaxFramesInFlight];
    // TODO(boti): It's kind of ridiculous that we can't just use the timeline semaphore to sync w/ present...
    VkSemaphore     RenderFinishedSemaphores[R_MaxFramesInFlight];
    VkSemaphore     TimelineSemaphore;
    u64             TimelineSemaphoreCounter;
    VkSemaphore     ComputeTimelineSemaphore;
    u64             ComputeTimelineSemaphoreCounter;

    u64             FrameFinishedCounters[R_MaxFramesInFlight];

    // NOTE(boti): These are all allocated from BAR memory
    VkBuffer        PerFrameUniformBuffers[R_MaxFramesInFlight];
    void*           PerFrameUniformBufferMappings[R_MaxFramesInFlight];
    VkBuffer        PerFrameResourceDescriptorBuffers[R_MaxFramesInFlight];
    void*           PerFrameResourceDescriptorMappings[R_MaxFramesInFlight];

    static constexpr umm BARBufferSize = MiB(30);
    VkBuffer        BARBuffers[R_MaxFramesInFlight];
    void*           BARBufferMappings[R_MaxFramesInFlight];
    VkDeviceAddress BARBufferAddresses[R_MaxFramesInFlight];

    VkDeviceMemory  StagingMemory;
    void*           StagingMemoryMapping;
    VkBuffer        StagingBuffers[R_MaxFramesInFlight];

    VkDeviceMemory  MipReadbackMemory;
    void*           MipReadbackMapping;
    VkBuffer        MipReadbackBuffers[R_MaxFramesInFlight];
    void*           MipReadbackMappings[R_MaxFramesInFlight];

    //
    // Persistent
    //
    VkBuffer        SamplerDescriptorBuffer;
    void*           SamplerDescriptorMapping;
    VkBuffer        StaticResourceDescriptorBuffer;
    void*           StaticResourceDescriptorMapping;

    umm             MipFeedbackMemorySize;
    VkBuffer        MipFeedbackBuffer;

    umm             PerFrameBufferSize;
    u64             PerFrameBufferAddress;
    VkBuffer        PerFrameBuffer;

    //
    // Pipelines, pipeline layouts and associated descriptor set layouts
    //
    pipeline_with_layout Pipelines[Pipeline_Count];
    VkDescriptorSetLayout SetLayouts[Set_Count];
    VkSampler Samplers[Sampler_Count];
    VkSampler MaterialSamplers[R_MaterialSamplerCount];

    u64 CurrentFrameID;
    render_frame Frames[R_MaxFramesInFlight];
};

typedef flags32 begin_cb_flags;
enum begin_cb_flag_bits : begin_cb_flags
{
    BeginCB_None = 0,
    
    BeginCB_Secondary = (1u << 0),
};

// NOTE(boti): Currently we're using using the pipeline info to determine whether a secondary CB
// is inside a render pass and what that render pass is.
// This works, because the attachments in the pipelines don't differ from the pass.
internal VkCommandBuffer
BeginCommandBuffer(renderer* Renderer, u32 FrameID, begin_cb_flags Flags, pipeline Pipeline);

internal void
EndCommandBuffer(VkCommandBuffer CB);

internal void 
SetupSceneRendering(render_frame* Frame, frustum* CascadeFrustums);

//
// Implementation
//

// NOTE(boti): See Vulkan spec. 16.5.4. Table 17. to find where these transforms come from
m3 GlobalCubeFaceBases[CubeLayer_Count]
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