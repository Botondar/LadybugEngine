#include "LadybugEngine.hpp"

#include "LadybugLib/LBLibBuild.cpp"

#include "Font.cpp"
#include "Asset.cpp"
#include "World.cpp"
#include "Editor.cpp"
#include "profiler.cpp"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#include "stb/stb_image_resize2.h"
#pragma clang diagnostic pop

#define STB_DXT_IMPLEMENTATION
#include "stb/stb_dxt.h"

platform_api Platform;

extern "C"
void Game_UpdateAndRender(thread_context* ThreadContext, game_memory* Memory, game_io* GameIO)
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
        RenderFrame = Platform.BeginRenderFrame(GameState->Renderer, ThreadContext, &GameState->TransientArena, GameIO->OutputExtent);

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
        RenderFrame = Platform.BeginRenderFrame(GameState->Renderer, ThreadContext, &GameState->TransientArena, Resolution);
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
#if 1
        m4 Transform = YUpToZUp;
#else
        m4 Transform = M4(1e-1f, 0.0f, 0.0f, 0.0f,
                          0.0f, 1e-1f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1e-1f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f) * YUpToZUp;
#endif
        DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, RenderFrame,
                           DEBUGLoad_AddNodesAsEntities,
                           GameIO->DroppedFilename, Transform);
        GameIO->bHasDroppedFile = false;
    }
    
    if (GameIO->Keys[SC_LeftAlt].bIsDown && GameIO->Keys[SC_F4].bIsDown)
    {
        GameIO->bQuitRequested = true;
    }
    
    GameState->PerfDataLog[GameState->FrameID % GameState->MaxPerfDataCount] = GameIO->ProfileDeltaTime;

    UpdateEditor(GameState, GameIO, RenderFrame);
    RenderFrame->Config = GameState->RenderConfig;
    RenderFrame->Config.DebugViewMode = DebugView_None;
    RenderFrame->ConstantFogDensity = GameState->ConstantFogDensity;
    RenderFrame->LinearFogDensityAtBottom = GameState->LinearFogDensityAtBottom;
    RenderFrame->LinearFogMinZ = GameState->LinearFogMinZ;
    RenderFrame->LinearFogMaxZ = GameState->LinearFogMaxZ;

    UpdateAssets(GameState->Assets);
    ProcessTextureRequests(GameState->Assets, RenderFrame);

    UpdateAndRenderWorld(GameState->World, GameState->Assets, RenderFrame, GameIO, 
                         &GameState->TransientArena, GameState->Editor.DebugFlags);
    Platform.EndRenderFrame(RenderFrame, ThreadContext);

    GameState->FrameID++;
}

ProfilerOverflowGuard;