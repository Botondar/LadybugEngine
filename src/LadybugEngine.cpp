#include "LadybugEngine.hpp"

#include "LadybugLib/LBLibBuild.cpp"

#include "Font.cpp"
#include "Asset.cpp"
#include "World.cpp"
#include "Editor.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_DXT_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable:4244)
#include "stb/stb_image.h"
#pragma warning(pop)
#include "stb/stb_image_resize.h"
#include "stb/stb_dxt.h"

platform_api Platform;

extern "C"
void Game_UpdateAndRender(game_memory* Memory, game_io* GameIO)
{
    Platform = Memory->PlatformAPI;
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

        GameState = Memory->GameState = PushStruct<game_state>(&BootstrapArena);
        GameState->TotalArena = BootstrapArena;
        constexpr umm TransientArenaSize = MiB(512);
        GameState->TransientArena = InitializeArena(TransientArenaSize, PushSize(&GameState->TotalArena, TransientArenaSize, 64));

        GameState->Renderer = CreateRenderer(&GameState->TotalArena, &GameState->TransientArena);
        if (!GameState->Renderer)
        {
            GameIO->bQuitRequested = true;
            GameIO->QuitMessage = "Renderer creation failed";
            return;
        }
        RenderFrame = BeginRenderFrame(GameState->Renderer, &GameState->TransientArena, GameIO->OutputWidth, GameIO->OutputHeight);

        constexpr umm AssetArenaSize = GiB(1);
        memory_arena AssetArena = InitializeArena(AssetArenaSize, PushSize(&GameState->TotalArena, AssetArenaSize, KiB(4)));
        assets* Assets = GameState->Assets = PushStruct<assets>(&AssetArena);
        Assets->Arena = AssetArena;
        InitializeAssets(Assets, RenderFrame, &GameState->TransientArena);

        game_world* World = GameState->World = PushStruct<game_world>(&GameState->TotalArena);

        InitEditor(GameState, &GameState->TransientArena);

        GameState->PostProcessParams.SSAO.Intensity = ssao_params::DefaultIntensity;
        GameState->PostProcessParams.SSAO.MaxDistance = ssao_params::DefaultMaxDistance;
        GameState->PostProcessParams.SSAO.TangentTau = ssao_params::DefaultTangentTau;
        GameState->PostProcessParams.Bloom.FilterRadius = bloom_params::DefaultFilterRadius;
        GameState->PostProcessParams.Bloom.InternalStrength = bloom_params::DefaultInternalStrength;
        GameState->PostProcessParams.Bloom.Strength = bloom_params::DefaultStrength;

        World->Camera.P = { 0.0f, 0.0f, 0.0f };
        World->Camera.FieldOfView = ToRadians(80.0f);
        World->Camera.NearZ = 0.1f;
        World->Camera.FarZ = 50.0f;//1000.0f;
        World->Camera.Yaw = 0.5f * Pi;
    }

    ResetArena(&GameState->TransientArena);

    if (GameIO->bIsMinimized)
    {
        return;
    }

    if (GameIO->dt >= 0.3f)
    {
        GameIO->dt = 0.0f;
    }

    if (!RenderFrame)
    {
        RenderFrame = BeginRenderFrame(GameState->Renderer, &GameState->TransientArena, GameIO->OutputWidth, GameIO->OutputHeight);
    }
    RenderFrame->ImmediateTextureID = GameState->Assets->DefaultFontTextureID;
    RenderFrame->ParticleTextureID = GameState->Assets->ParticleArrayID;

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
        m4 Transform = M4(1e-2f, 0.0f, 0.0f, 0.0f,
                          0.0f, 1e-2f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1e-2f, 0.0f,
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
    RenderFrame->PostProcess = GameState->PostProcessParams;

    if (!IsEmpty(&GameState->Assets->TextureQueue))
    {
        ProcessTextureQueueEntry(&GameState->Assets->TextureQueue, RenderFrame, &GameState->TransientArena);
    }

    if (IsEmpty(&GameState->Assets->TextureQueue))
    {
        UpdateAndRenderWorld(GameState->World, GameState->Assets, RenderFrame, GameIO, 
                             &GameState->TransientArena, GameState->Editor.DrawLights);
    }
    EndRenderFrame(RenderFrame);

    GameState->FrameID++;
}