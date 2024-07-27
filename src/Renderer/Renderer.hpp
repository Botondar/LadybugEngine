#pragma once

#include <LadybugLib/Core.hpp>
#include <LadybugLib/Intrinsics.hpp>
#include <LadybugLib/String.hpp>
#include <LadybugLib/image.hpp>

// HACK(boti): These are part of the high-level rendering API,
// but we need them in ShaderInterop so they're defined here at the top for now
struct renderer_texture_id
{
    u32 Value;
};

struct material_sampler_id
{
    u32 Value;
};

union rgba8
{
    u32 Color;
    struct
    {
        u8 R;
        u8 G;
        u8 B;
        u8 A;
    };
};
static_assert(sizeof(rgba8) == sizeof(u32));
#include <Shader/ShaderInterop.h>

static_assert(sizeof(per_frame) <= KiB(64));

//
// Config
//
constexpr u32 R_MaxFramesInFlight = 2u;

constexpr u64 R_RenderTargetMemorySize      = MiB(512);
constexpr u64 R_TextureMemorySize           = MiB(512);
constexpr u64 R_ShadowMapMemorySize         = MiB(256);
constexpr u32 R_MaxShadowCascadeCount       = 4;
constexpr u32 R_ShadowResolution            = 1536; //2048u; // TODO(boti): Rename, this only applies to the cascades
constexpr u32 R_PointShadowResolution       = 256u;
constexpr u64 R_VertexBufferMaxBlockCount   = (1llu << 18);
constexpr u32 R_MaxJointCount               = 256u;

constexpr f32 R_MaxLOD = 1000.0f;

#include "rhi.hpp"

union frustum
{
    v4 Planes[6];
    struct
    {
        v4 Left;
        v4 Right;
        v4 Top;
        v4 Bottom;
        v4 Near;
        v4 Far;
    };
};

inline frustum GetClipSpaceFrustum();

inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box);
inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box, m4 Transform);
inline b32 IntersectFrustumSphere(const frustum* Frustum, v3 P, f32 r);

//
// Render API
//

inline bool IsValid(renderer_texture_id ID) { return ID.Value != 0; }

inline f32 GetLuminance(v3 RGB);
inline v3 SetLuminance(v3 RGB, f32 Luminance);

typedef flags32 texture_flags;
enum texture_flag_bits : texture_flags
{
    TextureFlag_None                = 0,

    TextureFlag_PersistentMemory    = (1u << 0),
};

struct texture_request
{
    renderer_texture_id TextureID;
    u32 MipMask;
};

// Shading mode for the primary opaque pass
enum shading_mode : u32
{
    ShadingMode_Forward = 0,
    ShadingMode_Visibility,

    ShadingMode_Count,
};

struct render_config
{
    shading_mode ShadingMode;
    debug_view_mode DebugViewMode;
    
    f32 Exposure;
    f32 ParallaxScale; // TODO(boti): Move parallax scale to material

    f32 SSAOIntensity;
    f32 SSAOMaxDistance;
    f32 SSAOTangentTau;

    f32 BloomFilterRadius;
    f32 BloomInternalStrength;
    f32 BloomStrength;

    static constexpr f32 DefaultParallaxScale = 0.1f;

    static constexpr f32 DefaultSSAOIntensity = 8.0f;
    static constexpr f32 DefaultSSAOMaxDistance = 0.5f;
    static constexpr f32 DefaultSSAOTangentTau = 0.03125f;

    static constexpr f32 DefaultBloomFilterRadius = 0.005f;
    static constexpr f32 DefaultBloomInternalStrength = 1.0f;
    static constexpr f32 DefaultBloomStrength = 0.04f;
};

struct geometry_buffer_block
{
    u32 Count;
    u32 Offset;

    geometry_buffer_block* Next;
    geometry_buffer_block* Prev;
};

struct geometry_buffer_allocation
{
    geometry_buffer_block* VertexBlock;
    geometry_buffer_block* IndexBlock;
};

typedef u32 vert_index;

struct vertex_skin8
{
    v4 Weights;
    u8 Joints[4];
};
typedef vertex_skin8 vertex_skin;

enum billboard_mode : u32
{
    Billboard_ViewAligned = 0, // Billboards facing the camera directly
    Billboard_ZAligned = 1, // Billboards aligned to the vertical axis (but rotated horizontally)
};

struct render_particle
{
    v3 P;
    u32 TextureIndex;
    v4 Color;
    v2 HalfExtent;
};

struct particle_batch
{
    u32 Count;
    billboard_mode Mode;
};

struct draw_batch_2d
{
    u32 VertexCount;
};

typedef flags32 draw_flags;
enum draw_flag_bits : draw_flags
{
    Draw_None    = 0,

    Draw_Skinned = (1u << 0),
};

struct draw_command
{
    draw_group Group;
    draw_flags Flags;
    mmbox BoundingBox;
    renderer_material Material;
    m4 Transform;
    geometry_buffer_allocation Geometry;
};

struct draw_widget3d_cmd
{
    geometry_buffer_allocation Geometry;
    m4 Transform;
    rgba8 Color;
};

enum transfer_op_type : u32
{
    TransferOp_Undefined = 0,

    TransferOp_Texture,
    TransferOp_Geometry,
};

struct transfer_texture
{
    renderer_texture_id TargetID;
    texture_info Info;
    texture_subresource_range SubresourceRange;
};

struct transfer_geometry
{
    geometry_buffer_allocation Dest;
};

struct transfer_op
{
    transfer_op_type Type;
    union
    {
        transfer_texture Texture;
        transfer_geometry Geometry;
    };
};

enum render_command_type : u32
{
    RenderCommand_NOP = 0,

    RenderCommand_Transfer,
    RenderCommand_Light,
    RenderCommand_Draw,
    RenderCommand_ParticleBatch,
    RenderCommand_Widget3D,
    RenderCommand_Batch2D,

    RenderCommand_Count,
};

struct render_command
{
    render_command_type Type;
    umm BARBufferAt;
    umm BARBufferSize;
    umm StagingBufferAt;
    umm StagingBufferSize;

    union
    {
        transfer_op         Transfer;
        light               Light;
        draw_command        Draw;
        particle_batch      ParticleBatch;
        draw_widget3d_cmd   Widget3D;
        draw_batch_2d       Batch2D;
    };
};

struct staging_buffer
{
    umm     Size;
    umm     At;
    void*   Base;
};

struct render_stat_mem_entry
{
    const char* Name;
    umm UsedSize;
    umm AllocationSize;
};

struct render_stat_perf_entry
{
    const char* Name;
    f32 Time;
};

struct render_stats
{
    umm TotalMemoryUsed;
    umm TotalMemoryAllocated;

    f32 FrameTime;

    static constexpr u32 MaxMemoryEntryCount = 1024u;
    u32 MemoryEntryCount;
    render_stat_mem_entry MemoryEntries[MaxMemoryEntryCount];

    static constexpr u32 MaxPerfEntryCount = 32u;
    u32 PerfEntryCount;
    render_stat_perf_entry PerfEntries[MaxPerfEntryCount];
};

struct renderer;

struct renderer_info
{
    const char* DeviceName;
    device_luid DeviceLUID;
};

struct renderer_init_result
{
    renderer* Renderer;

    // NOTE(boti): Extra information if Renderer is null
    u64 ErrorCode;
    const char* ErrorMessage;

    renderer_info Info;
};

struct render_frame
{
    renderer* Renderer;
    memory_arena* Arena; // NOTE(boti): filled by the game

    // NOTE(boti): Texture upload requests _from_ the renderer _to_ the game
    u32 TextureRequestCount;
    texture_request* TextureRequests;

    render_stats Stats;

    b32 ReloadShaders;

    u32 FrameID;
    v2u RenderExtent;
    v2u OutputExtent;

    renderer_texture_id ImmediateTextureID;
    renderer_texture_id ParticleTextureID; // TODO(boti): Move this to the particle command?

    render_config Config;

    // Environment
    struct
    {
        v3 SunL;
        v3 SunV; // World-space sun direction

        f32 ConstantFogDensity;
        f32 LinearFogDensityAtBottom;
        f32 LinearFogMinZ;
        f32 LinearFogMaxZ;
    };

    m4 CameraTransform;
    f32 CameraFocalLength;
    f32 CameraFarPlane;
    f32 CameraNearPlane;

    // Backend-calculated/cached values
    // TODO(boti): move these out of the API
    struct
    {
        frustum CameraFrustum;
        m4 ViewTransform;
        m4 ProjectionTransform;
        m4 InverseProjectionTransform;
    };

    umm BARBufferSize;
    umm BARBufferAt;
    void* BARBufferBase;

    staging_buffer StagingBuffer;

    // Limits
    static constexpr u32 MaxCommandCount    = (1u << 17);

    u32             CommandCount;
    render_command* Commands;
    render_command* LastBatch2D;

    u32             DrawGroupDrawCounts[DrawGroup_Count];
    u32             LightCount;
    u32             ShadowCount;

    // TODO(boti): Remove these from the API (should be backend only)
    void* UniformData; // GPU-backed frame_uniform_data
    per_frame Uniforms;
};

struct thread_context;

#define Signature_CreateRenderer(name)      renderer_init_result        name(struct platform_api* PlatformAPI, memory_arena* Arena, memory_arena* Scratch)
#define Signature_AllocateGeometry(name)    geometry_buffer_allocation  name(renderer* Renderer, u32 VertexCount, u32 IndexCount)
/* NOTE(boti): Passing nullptr as Info is allowed as a way to allocate a texture _name_ only.
 * Such a name can't be used as a placeholder until it has been uploaded with some data (after a frame boundary).
 */ 
#define Signature_AllocateTexture(name)     renderer_texture_id         name(renderer* Renderer, texture_flags Flags, const texture_info* Info, renderer_texture_id Placeholder)
#define Signature_BeginRenderFrame(name)    render_frame*               name(renderer* Renderer, thread_context* ThreadContext, memory_arena* Arena, v2u RenderExtent)
#define Signature_EndRenderFrame(name)      void                        name(render_frame* Frame, thread_context* ThreadContext)

typedef Signature_CreateRenderer(create_renderer);
typedef Signature_AllocateGeometry(allocate_geometry);
typedef Signature_AllocateTexture(allocate_texture);
typedef Signature_BeginRenderFrame(begin_render_frame);
typedef Signature_EndRenderFrame(end_render_frame);

//
// Frame rendering
//
inline void* 
Transfer(staging_buffer* StagingBuffer, umm Size, umm Alignment);

inline b32 
TransferTexture(render_frame* Frame, renderer_texture_id ID, 
                texture_info Info, texture_subresource_range Range,
                const void* Data);
inline b32 TransferGeometry(render_frame* Frame, geometry_buffer_allocation Allocation,
                            const void* VertexData, const void* IndexData);

inline b32 
DrawMesh(render_frame* Frame,
         draw_group Group,
         geometry_buffer_allocation Allocation, 
         m4 Transform,
         mmbox BoundingBox,
         renderer_material Material,
         u32 JointCount, m4* Pose);

inline b32
DrawWidget3D(render_frame* Frame,
             geometry_buffer_allocation Allocation,
             m4 Transform, rgba8 Color);
inline b32 AddLight(render_frame* Frame, v3 P, v3 E, light_flags Flags);

inline b32 DrawTriangleList2D(render_frame* Frame, u32 VertexCount, vertex_2d* VertexArray);

inline render_command*
MakeParticleBatch(render_frame* Frame, u32 MaxParticleCount);

inline render_command*
PushParticle(render_frame* Frame, render_command* Batch, render_particle Particle);

//
// Internal public interface
//
inline render_command*
PushCommand_(render_frame* Frame, render_command_type Type, 
             umm BARAlignment, umm BARUsage,
             umm StagingAlignment, umm StagingUsage);

//
// Helpers
//
inline constexpr rgba8 PackRGBA8(u32 R, u32 G, u32 B, u32 A = 0xFF);
inline rgba8 PackRGBA(v4 Color);
inline umm GetPackedTexture2DMipOffset(const texture_info* Info, u32 Mip);
inline texture_subresource_range SubresourceFromMipMask(u32 Mask, texture_info Info);

//
// Implementation
//

inline constexpr rgba8 PackRGBA8(u32 R, u32 G, u32 B, u32 A /*= 0xFF*/)
{
    rgba8 Result = 
    {
        .R = (u8)R,
        .G = (u8)G,
        .B = (u8)B,
        .A = (u8)A,
    };
    return Result;
}

inline rgba8 PackRGBA(v4 Color)
{
    rgba8 Result = 
    {
        .R = (u8)Round(Clamp(Color.X, 0.0f, 1.0f) * 255.0f),
        .G = (u8)Round(Clamp(Color.Y, 0.0f, 1.0f) * 255.0f),
        .B = (u8)Round(Clamp(Color.Z, 0.0f, 1.0f) * 255.0f),
        .A = (u8)Round(Clamp(Color.W, 0.0f, 1.0f) * 255.0f),
    };
    return Result;
}

inline rgba8 PackRGBA(v3 Color)
{
    rgba8 Result = 
    {
        .R = (u8)Round(Clamp(Color.X, 0.0f, 1.0f) * 255.0f),
        .G = (u8)Round(Clamp(Color.Y, 0.0f, 1.0f) * 255.0f),
        .B = (u8)Round(Clamp(Color.Z, 0.0f, 1.0f) * 255.0f),
        .A = 0xFFu,
    };
    return Result;
}

inline f32 GetLuminance(v3 RGB)
{
    f32 Result = Dot(RGB, v3{ 0.212639f, 0.715169f, 0.072192f });
    return(Result);
}
inline v3 SetLuminance(v3 RGB, f32 Luminance)
{
    f32 CurrentLuminance = GetLuminance(RGB);
    v3 Result = RGB * Ratio0(Luminance, CurrentLuminance);
    return(Result);
}

// NOTE(boti): Returns the mip count of a mip chain when the lowest resolution mip level
//             must be at least 2x2
inline u32 GetMaxMipCountGreaterThanOne(u32 Width, u32 Height)
{
    u32 Result = 0;
    if (!BitScanReverse(&Result, Min(Width, Height)))
    {
        Result = 0;
    }
    return Result;
}

inline umm GetPackedTexture2DMipOffset(const texture_info* Info, u32 Mip)
{
    umm Result = 0;

    // TODO(boti): This is really just the same code as GetMipChainSize...
    // NOTE(boti): MipCount from info is ignored, 
    // we return the offset even if that mip level is not present in the texture
    u32 MipCount = GetMaxMipCount(Info->Extent.X, Info->Extent.Y);
    if (Mip < MipCount)
    {
        format_info Byterate = FormatInfoTable[Info->Format];

        for (u32 CurrentMip = 0; CurrentMip < Mip; CurrentMip++)
        {
            u32 ExtentX = Max(Info->Extent.X >> CurrentMip, 1u);
            u32 ExtentY = Max(Info->Extent.Y >> CurrentMip, 1u);
            if (Byterate.Flags & FormatFlag_BlockCompressed)
            {
                ExtentX = Align(ExtentX, 4u);
                ExtentY = Align(ExtentY, 4u);
            }
            Result += ((umm)ExtentX * (umm)ExtentY * Byterate.ByteRateNumerator) / Byterate.ByteRateDenominator;
        }
    }

    return(Result);
}

inline texture_subresource_range SubresourceFromMipMask(u32 Mask, texture_info Info)
{
    texture_subresource_range Result = AllTextureSubresourceRange();

    if (Info.ArrayCount != 1) UnimplementedCodePath;

    u32 MostDetailedMipInMask;
    if (BitScanReverse(&MostDetailedMipInMask, Mask))
    {
        u32 MipCount = GetMaxMipCount(Info.Extent.X, Info.Extent.Y);
        u32 ResolutionFromMask = 1 << MostDetailedMipInMask;
        u32 MipCountFromMask = GetMaxMipCount(ResolutionFromMask, ResolutionFromMask);

        u32 MaxExtent = Max(Info.Extent.X, Info.Extent.Y);
        if (MipCountFromMask > MipCount) 
        {
            MipCountFromMask = MipCount;
        }
        else
        {
            while (MaxExtent && (ResolutionFromMask < MaxExtent))
            {
                Result.BaseMip++;
                MaxExtent >>= 1;
            }
        }

        u32 LeastDetailedMipInMask;
        BitScanForward(&LeastDetailedMipInMask, Mask);
        u32 LeastDetailedMipResolution = 1 << LeastDetailedMipInMask;
        if (LeastDetailedMipResolution > MaxExtent)
        {
            Result.MipCount = 0;
        }
        else
        {
            // TODO(boti): Handle all bits from the mask, not just the highest one
            Result.MipCount = Info.MipCount - Result.BaseMip;
        }
    }

    return(Result);
}

inline render_command*
PushCommand_(render_frame* Frame, render_command_type Type, 
             umm BARAlignment, umm BARUsage,
             umm StagingAlignment, umm StagingUsage)
{
    render_command* Command = nullptr;

    umm AlignedBAR = BARAlignment ? Align(Frame->BARBufferAt, BARAlignment) : Frame->BARBufferAt;
    umm AlignedStaging = StagingAlignment ? Align(Frame->StagingBuffer.At, StagingAlignment) : Frame->StagingBuffer.At;
    if ((Frame->CommandCount < Frame->MaxCommandCount) && 
        (AlignedBAR + BARUsage <= Frame->BARBufferSize) &&
        (AlignedStaging + StagingUsage <= Frame->StagingBuffer.Size))
    {
        Command = Frame->Commands + Frame->CommandCount++;
        memset(Command, 0, sizeof(*Command));
        Command->Type = Type;

        Command->BARBufferAt        = AlignedBAR;
        Command->BARBufferSize      = BARUsage;
        Frame->BARBufferAt          = AlignedBAR + BARUsage;
        Command->StagingBufferAt    = AlignedStaging;
        Command->StagingBufferSize  = StagingUsage;
        Frame->StagingBuffer.At     = AlignedStaging + StagingUsage;
    }
    return(Command);
}

inline b32 
TransferTexture(render_frame* Frame, renderer_texture_id ID, 
                texture_info Info, texture_subresource_range Range,
                const void* Data)
{
    b32 Result = true;

    format_info ByteRate = FormatInfoTable[Info.Format];
    umm TotalSize = GetMipChainSize(Info.Extent.X, Info.Extent.Y, Info.MipCount, Info.ArrayCount, ByteRate);
    render_command* Command = PushCommand_(Frame, RenderCommand_Transfer, 0, 0, 64, TotalSize);
    if (Command)
    {
        Command->Transfer.Type = TransferOp_Texture;
        Command->Transfer.Texture.TargetID = ID;
        Command->Transfer.Texture.Info = Info;
        Command->Transfer.Texture.SubresourceRange = Range;
        memcpy(OffsetPtr(Frame->StagingBuffer.Base, Command->StagingBufferAt), Data, TotalSize);
    }
    else
    {
        UnhandledError("Failed to transfer texture");
        Result = false;
    }

    return(Result);
}

inline b32 TransferGeometry(render_frame* Frame, geometry_buffer_allocation Allocation,
                            const void* VertexData, const void* IndexData)
{
    b32 Result = true;

    umm VertexSize = (umm)Allocation.VertexBlock->Count * sizeof(vertex);
    umm IndexSize = (umm)Allocation.IndexBlock->Count * sizeof(vert_index);
    umm TotalSize = VertexSize + IndexSize;
    render_command* Command = PushCommand_(Frame, RenderCommand_Transfer, 0, 0, 16, TotalSize);
    if (Command)
    {
        Command->Transfer.Type = TransferOp_Geometry;
        Command->Transfer.Geometry.Dest = Allocation;
        memcpy(OffsetPtr(Frame->StagingBuffer.Base, Command->StagingBufferAt), VertexData, VertexSize);
        memcpy(OffsetPtr(Frame->StagingBuffer.Base, Command->StagingBufferAt + VertexSize), IndexData, IndexSize);
    }
    else
    {
        UnhandledError("Failed to transfer geometry");
        Result = false;
    }

    return(Result);
}

inline b32 
DrawMesh(render_frame* Frame,
         draw_group Group,
         geometry_buffer_allocation Allocation,
         m4 Transform,
         mmbox BoundingBox,
         renderer_material Material,
         u32 JointCount, m4* Pose)
{
    b32 Result = true;

    b32 IsSkinned = JointCount != 0;
    umm PoseByteCount = JointCount * sizeof(m4);

    render_command* Command = PushCommand_(Frame, RenderCommand_Draw, IsSkinned ? sizeof(m4) : 0, PoseByteCount, 0, 0);
    if (Command)
    {
        Command->Draw.Group = Group;
        if (IsSkinned)
        {
            Command->Draw.Flags |= Draw_Skinned;
            memcpy(OffsetPtr(Frame->BARBufferBase, Command->BARBufferAt), Pose, PoseByteCount);
        }
        Command->Draw.BoundingBox = BoundingBox;
        Command->Draw.Material = Material;
        Command->Draw.Transform = Transform;
        Command->Draw.Geometry = Allocation;

        Frame->DrawGroupDrawCounts[Command->Draw.Group]++;
    }
    else
    {
        Result = false;
    }
    return(Result);
}

inline b32 
DrawWidget3D(render_frame* Frame,
             geometry_buffer_allocation Allocation,
             m4 Transform, rgba8 Color)
{
    b32 Result = true;
    render_command* Command = PushCommand_(Frame, RenderCommand_Widget3D, 0, 0, 0, 0);
    if (Command)
    {
        Command->Widget3D.Geometry = Allocation;
        Command->Widget3D.Transform = Transform;
        Command->Widget3D.Color = Color;
    }
    else
    {
        Result = false;
    }
    return(Result);
}

inline b32 AddLight(render_frame* Frame, v3 P, v3 E, light_flags Flags)
{
    b32 Result = false;

    render_command* Command = PushCommand_(Frame, RenderCommand_Light, 0, 0, 0, 0);
    if (Command)
    {
        Command->Light.P = P;
        Command->Light.ShadowIndex = (Flags & LightFlag_ShadowCaster) ? Frame->ShadowCount++ : 0xFFFFFFFFu;
        Command->Light.E = E;
        Command->Light.Flags = Flags;

        Frame->LightCount++;
    }
    else
    {
        Result = false;
    }
    return(Result);
}

inline b32 DrawTriangleList2D(render_frame* Frame, u32 VertexCount, vertex_2d* VertexArray)
{
    b32 Result = true;

    constexpr umm BatchBlockByteCount = KiB(16);

    umm ByteCount = VertexCount * sizeof(vertex_2d);

    render_command* Command = Frame->LastBatch2D;
    if (Command)
    {
        Assert(Command->Type == RenderCommand_Batch2D);

        umm BytesUsed = Command->Batch2D.VertexCount * sizeof(vertex_2d);
        if (BytesUsed + ByteCount > Command->BARBufferSize)
        {
            // NOTE(boti): Try stretching to the last batch
            if (Frame->BARBufferAt == Command->BARBufferAt + Command->BARBufferSize)
            {
                umm NewBARSize = Align(ByteCount + BytesUsed, BatchBlockByteCount);
                if (Command->BARBufferAt + NewBARSize <= Frame->BARBufferSize)
                {
                    Command->BARBufferSize = NewBARSize;
                    Frame->BARBufferAt = Command->BARBufferAt + NewBARSize;
                }
                else
                {
                    Command = nullptr;
                }
            }
            else
            {
                Command = PushCommand_(Frame, RenderCommand_Batch2D, 16, Align(ByteCount, BatchBlockByteCount), 0, 0);
            }
        }
    }
    else
    {
        Command = PushCommand_(Frame, RenderCommand_Batch2D, 16, Align(ByteCount, BatchBlockByteCount), 0, 0);
    }

    if (Command)
    {
        umm BytesUsed = Command->Batch2D.VertexCount * sizeof(vertex_2d);
        memcpy(OffsetPtr(Frame->BARBufferBase, Command->BARBufferAt + BytesUsed), VertexArray, ByteCount);
        Command->Batch2D.VertexCount += VertexCount;

        Assert(Command->Batch2D.VertexCount * sizeof(vertex_2d) <= Command->BARBufferSize);
    }
    else
    {
        Result = false;
    }
    
    Frame->LastBatch2D = Command;
    return(Result);
}

inline render_command*
MakeParticleBatch(render_frame* Frame, u32 MinParticleCount)
{
    constexpr umm MinBatchByteCount = KiB(16);
    umm ByteCount = Align(Max(MinBatchByteCount, MinParticleCount * sizeof(render_particle)), MinBatchByteCount);

    render_command* Command = PushCommand_(Frame, RenderCommand_ParticleBatch, 16, ByteCount, 0, 0);
    return(Command);
}

inline render_command*
PushParticle(render_frame* Frame, render_command* Batch, render_particle Particle)
{
    render_command* Command = Batch;

    if (Command)
    {
        Assert(Command->Type == RenderCommand_ParticleBatch);

        if ((Command->ParticleBatch.Count + 1) * sizeof(render_particle) >= Command->BARBufferSize)
        {
            Command = MakeParticleBatch(Frame, 1);
        }

        if (Command)
        {
            Command->ParticleBatch.Mode = Batch->ParticleBatch.Mode;
            memcpy(OffsetPtr(Frame->BARBufferBase, Command->BARBufferAt + Command->ParticleBatch.Count * sizeof(Particle)), &Particle, sizeof(Particle));
            Command->ParticleBatch.Count++;
        }
    }

    return(Command);
}

inline frustum GetClipSpaceFrustum()
{
    frustum Result = 
    {
        .Left   = { -1.0f,  0.0f,   0.0f, +1.0f },
        .Right  = { +1.0f,  0.0f,   0.0f, +1.0f },
        .Top    = {  0.0f, -1.0f,   0.0f, +1.0f },
        .Bottom = {  0.0f, +1.0f,   0.0f, +1.0f },
        .Near   = {  0.0f,  0.0f,  +1.0f,  0.0f },
        .Far    = {  0.0f,  0.0f,  -1.0f, +1.0f },
    };
    return(Result);
}

inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box)
{
    b32 Result = true;

    v3 HalfExtent = 0.5f * (Box.Max - Box.Min);
    if (Dot(HalfExtent, HalfExtent) > 1e-6f)
    {
        v3 CenterP3 = 0.5f * (Box.Min + Box.Max);
        v4 CenterP = { CenterP3.X, CenterP3.Y, CenterP3.Z, 1.0f };

        for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
        {
            f32 EffectiveRadius = 
                Abs(Frustum->Planes[PlaneIndex].X * HalfExtent.X) + 
                Abs(Frustum->Planes[PlaneIndex].Y * HalfExtent.Y) +
                Abs(Frustum->Planes[PlaneIndex].Z * HalfExtent.Z);

            if (Dot(CenterP, Frustum->Planes[PlaneIndex]) < -EffectiveRadius)
            {
                Result = false;
                break;
            }
        }
    }
    return(Result);
}

inline b32 IntersectFrustumBox(const frustum* Frustum, mmbox Box, m4 Transform)
{
    b32 Result = true;

    v3 P = TransformPoint(Transform, 0.5f * (Box.Max + Box.Min));
    v3 HalfExtent = 0.5f * (Box.Max - Box.Min);
    for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
    {
        v4 Plane = Frustum->Planes[PlaneIndex];
        f32 EffectiveRadius = 
            HalfExtent.X * Abs(Dot(Plane, Transform.X)) +
            HalfExtent.Y * Abs(Dot(Plane, Transform.Y)) +
            HalfExtent.Z * Abs(Dot(Plane, Transform.Z));
        if (Dot(Plane, v4{ P.X, P.Y, P.Z, 1.0f }) < -EffectiveRadius)
        {
            Result = false;
            break;
        }
    }

    return(Result);
}

inline b32 IntersectFrustumSphere(const frustum* Frustum, v3 Center, f32 r)
{
    b32 Result = true;
    v4 P = { Center.X, Center.Y, Center.Z, 1.0f };
    for (u32 PlaneIndex = 0; PlaneIndex < 6; PlaneIndex++)
    {
        if (Dot(Frustum->Planes[PlaneIndex], P) <= -r)
        {
            Result = false;
            break;
        }
    }
    return(Result);
}

inline b32 IntersectFrustumFrustum(const frustum* A, const frustum* B)
{
    b32 Result = true;
    UnimplementedCodePath;
    return(Result);
}
