#include "LadybugEngine.hpp"

#include "LadybugLib/LBLibBuild.cpp"

#include "Font.cpp"
#include "Asset.cpp"
#include "World.cpp"
#include "Editor.cpp"
#include "profiler.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_DXT_IMPLEMENTATION

#pragma warning(push)
#pragma warning(disable:4244)
#include "stb/stb_image.h"
#pragma warning(pop)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#include "stb/stb_image_resize2.h"
#pragma clang diagnostic pop
#include "stb/stb_dxt.h"

platform_api Platform;

extern "C"
void Game_UpdateAndRender(game_memory* Memory, game_io* GameIO)
{
    Platform = Memory->PlatformAPI;

    TimedFunction(Platform.Profiler);

    if (GameIO->bQuitRequested)
    {
        // NOTE(boti): This is where handling game quit should be happening (saving the game, cleaning up if needed, etc.)
        return;
    }

    game_state* GameState = Memory->GameState;
    render_frame* RenderFrame = nullptr;
    if (!GameState)
    {
        memory_arena BootstrapArena = InitializeArena(Memory->Size, Memory->Memory);

        GameState = Memory->GameState = PushStruct(&BootstrapArena, 0, game_state);
        GameState->TotalArena = BootstrapArena;
        constexpr umm TransientArenaSize = MiB(512);
        GameState->TransientArena = InitializeArena(TransientArenaSize, PushSize_(&GameState->TotalArena, 0, TransientArenaSize, 64));

        renderer_init_result RendererInit = Platform.CreateRenderer(&Memory->PlatformAPI, &GameState->TotalArena, &GameState->TransientArena);
        GameState->Renderer = RendererInit.Renderer;
        if (RendererInit.Renderer)
        {
            GameState->RendererInfo = RendererInit.Info;
        }
        else
        {
            GameIO->bQuitRequested = true;
            GameIO->QuitMessage = RendererInit.ErrorMessage;
            return;
        }
        RenderFrame = Platform.BeginRenderFrame(GameState->Renderer, &GameState->TransientArena, GameIO->OutputExtent);

        constexpr umm AssetArenaSize = GiB(5);
        memory_arena AssetArena = InitializeArena(AssetArenaSize, PushSize_(&GameState->TotalArena, 0, AssetArenaSize, KiB(4)));
        assets* Assets = GameState->Assets = PushStruct(&AssetArena, 0, assets);
        Assets->Arena = AssetArena;
        InitializeAssets(Assets, RenderFrame, &GameState->TransientArena);

        game_world* World = GameState->World = PushStruct(&GameState->TotalArena, 0, game_world);
        World->Arena = &GameState->TotalArena;

        InitEditor(GameState, &GameState->TransientArena);

        GameState->RenderConfig.Exposure                = GameState->DefaultExposure;
        GameState->RenderConfig.SSAOIntensity           = render_config::DefaultSSAOIntensity;
        GameState->RenderConfig.SSAOMaxDistance         = render_config::DefaultSSAOMaxDistance;
        GameState->RenderConfig.SSAOTangentTau          = render_config::DefaultSSAOTangentTau;
        GameState->RenderConfig.BloomFilterRadius       = render_config::DefaultBloomFilterRadius;
        GameState->RenderConfig.BloomInternalStrength   = render_config::DefaultBloomInternalStrength;
        GameState->RenderConfig.BloomStrength           = render_config::DefaultBloomStrength;
    }

    if (GameIO->bIsMinimized)
    {
        return;
    }

    if (GameIO->dt >= 0.3f)
    {
        GameIO->dt = 0.3f;
    }

    if (!RenderFrame)
    {
        ResetArena(&GameState->TransientArena);
        v2u Resolution = { 0, 0 };
        //Resolution = { 1366, 768 };
        RenderFrame = Platform.BeginRenderFrame(GameState->Renderer, &GameState->TransientArena, Resolution);
    }
    RenderFrame->ImmediateTextureID = GameState->Assets->Textures[GameState->Assets->DefaultFontTextureID].RendererID;
    RenderFrame->ParticleTextureID = GameState->Assets->Textures[GameState->Assets->ParticleArrayID].RendererID;

    if (GameIO->bHasDroppedFile)
    {
        m4 YUpToZUp = M4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
#if 0
        m4 Transform = YUpToZUp;
#else
        m4 Transform = M4(1e-1f, 0.0f, 0.0f, 0.0f,
                          0.0f, 1e-1f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1e-1f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f) * YUpToZUp;
#endif
        DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, RenderFrame, GameIO->DroppedFilename, Transform);
        GameIO->bHasDroppedFile = false;
    }
    
    if (GameIO->Keys[SC_LeftAlt].bIsDown && GameIO->Keys[SC_F4].bIsDown)
    {
        GameIO->bQuitRequested = true;
    }
    
    UpdateEditor(GameState, GameIO, RenderFrame);
    RenderFrame->Config.ShadingMode = ShadingMode_Visibility;
    RenderFrame->Config = GameState->RenderConfig;
    RenderFrame->Config.DebugViewMode = DebugView_None;
    RenderFrame->ConstantFogDensity = GameState->ConstantFogDensity;
    RenderFrame->LinearFogDensityAtBottom = GameState->LinearFogDensityAtBottom;
    RenderFrame->LinearFogMinZ = GameState->LinearFogMinZ;
    RenderFrame->LinearFogMaxZ = GameState->LinearFogMaxZ;

    // Asset update
    {
        assets* Assets = GameState->Assets;
        texture_queue* Queue = &Assets->TextureQueue;

        for (u32 Completion = Queue->CompletionCount; Completion < Queue->CompletionGoal; Completion++)
        {
            u32 EntryIndex = Completion % Queue->MaxEntryCount;
            texture_queue_entry* Entry = Queue->Entries + EntryIndex;
            if (Entry->ReadyToTransfer)
            {
                umm TotalSize = GetMipChainSize(Entry->Info.Extent.X, Entry->Info.Extent.Y, 
                                                Entry->Info.MipCount, Entry->Info.ArrayCount,
                                                FormatByterateTable[Entry->Info.Format]);
                umm Begin = GetNextEntryOffset(Queue, TotalSize, Queue->RingBufferReadAt);

                Entry->Texture->Info = Entry->Info;
                Entry->Texture->Memory = PushSize_(&Assets->TextureCache, 0, TotalSize, 0);
                Assert(Entry->Texture->Memory);
                memcpy(Entry->Texture->Memory, Queue->RingBufferMemory + (Begin % Queue->RingBufferSize), TotalSize);

                Entry->Texture->IsLoaded = true;
                Queue->RingBufferReadAt = Begin + TotalSize;
                AtomicLoadAndIncrement(&Queue->CompletionCount);
            }
            else
            {
                break;
            }
        }
    }

    // Process texture requests
    {
        assets* Assets = GameState->Assets;
        for (u32 RequestIndex = 0; RequestIndex < RenderFrame->TextureRequestCount; RequestIndex++)
        {
            // TODO(boti): I don't think this is needed anymore, default textures go into persistent memory, 
            // so the renderer shouldn't request them?
            texture_request* Request = RenderFrame->TextureRequests + RequestIndex;

            b32 IsDefaultTexture = false;
            for (u32 TextureType = TextureType_Albedo; TextureType < TextureType_Count; TextureType++)
            {
                if (Request->TextureID.Value == Assets->Textures[Assets->DefaultTextures[TextureType]].RendererID.Value)
                {
                    IsDefaultTexture = true;
                    break;
                }
            }
            if (!IsDefaultTexture)
            {
                // TODO(boti): Put a user value in the renderer texture so we don't have to search for the texture every time
                texture* Texture = nullptr;
                u32 TextureID;
                for (TextureID = 0; TextureID < Assets->TextureCount; TextureID++)
                {
                    texture* It = Assets->Textures + TextureID;
                    if (Request->TextureID.Value == It->RendererID.Value)
                    {
                        Texture = It;
                        break;
                    }
                }

                if (Texture)
                {
                    if (Texture->IsLoaded && Texture->Memory)
                    {
                        texture_subresource_range Subresource = SubresourceFromMipMask(Request->MipMask, Texture->Info);
                        if (Subresource.MipCount)
                        {
                            if (Subresource.BaseArray != 0 || Subresource.ArrayCount != U32_MAX) UnimplementedCodePath;
                            umm Offset = GetPackedTexture2DMipOffset(&Texture->Info, Subresource.BaseMip);

                            v2u EffectiveExtent = 
                            {
                                Max(Texture->Info.Extent.X >> Subresource.BaseMip, 1u),
                                Max(Texture->Info.Extent.Y >> Subresource.BaseMip, 1u),
                            };
                            texture_info CopyInfo = 
                            {
                                .Extent = { EffectiveExtent.X, EffectiveExtent.Y, 1 },
                                .MipCount =  Min(GetMaxMipCount(EffectiveExtent.X, EffectiveExtent.Y), Subresource.MipCount),
                                .ArrayCount = 1,
                                .Format = Texture->Info.Format,
                                .Swizzle = Texture->Info.Swizzle,
                            };
                            TransferTexture(RenderFrame, Texture->RendererID, CopyInfo, AllTextureSubresourceRange(), OffsetPtr(Texture->Memory, Offset));
                        }
                    }
                }
            }
        }
    }
    
    UpdateAndRenderWorld(GameState->World, GameState->Assets, RenderFrame, GameIO, 
                         &GameState->TransientArena, GameState->Editor.DrawLights);
    Platform.EndRenderFrame(RenderFrame);

    GameState->FrameID++;
}

ProfilerOverflowGuard;