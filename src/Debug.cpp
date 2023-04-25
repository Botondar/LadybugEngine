#include "Debug.hpp"

#if 0
lbfn void DEBUGRenderVBUsage(vulkan_renderer* Renderer, v2 P, v2 Size)
{
    f32 Scale =  Size.x / Renderer->GeometryBuffer.MemorySize;
    for (geometry_buffer_block* Block = Renderer->GeometryBuffer.FreeVertexBlocks.Next; 
        Block != &Renderer->GeometryBuffer.FreeVertexBlocks; 
        Block = Block->Next)
    {
        v2 P1 = { P.x + Scale * Block->ByteOffset, P.y };
        v2 P2 = { P.x + Scale * (Block->ByteOffset + Block->ByteSize), P.y + Size.y };
        v2 UV = { 0.0f, 0.0f };
        PushRect(Renderer, P1, P2, UV, UV, PackRGBA8(0x00, 0xAA, 0x00));
    }

    for (geometry_buffer_block* Block = Renderer->GeometryBuffer.UsedVertexBlocks.Next; 
        Block != &Renderer->GeometryBuffer.UsedVertexBlocks; 
        Block = Block->Next)
    {
        v2 P1 = { P.x + Scale * Block->ByteOffset, P.y };
        v2 P2 = { P.x + Scale * (Block->ByteOffset + Block->ByteSize), P.y + Size.y };
        v2 UV = { 0.0f, 0.0f };
        PushRect(Renderer, P1, P2, UV, UV, PackRGBA8(0xAA, 0x00, 0x00));
    }
}
#endif

lbfn b32 DoDebugUI(game_state* Game, game_io* GameIO, render_frame* Frame)
{
    b32 MouseInputUsed = false;

    font* Font = &Game->Assets->DefaultFont;

    rgba8 DefaultForegroundColor = PackRGBA8(0xFF, 0xFF, 0xFF, 0xFF);
    rgba8 DefaultBackgroundColor = PackRGBA8(0x00, 0x00, 0xFF, 0xAA);
    rgba8 SelectedBackgroundColor = PackRGBA8(0xAA, 0x00, 0xFF, 0xAA);
    rgba8 HotBackgroundColor = PackRGBA8(0xFF, 0x00 ,0xFF, 0xAA);
    rgba8 ActiveBackgroundColor = PackRGBA8(0xFF, 0x00, 0xFF, 0xFF);

    // NOTE(boti): we always print the frametime, regardless of whether the debug UI is active or not
    {
        font_layout_type Layout = font_layout_type::Top;
        f32 FrameTimeSize = 24.0f;

        constexpr size_t BuffSize = 128;
        char Buff[BuffSize];

        snprintf(Buff, BuffSize, "%.2fms", 1000.0f * GameIO->dt);
        mmrect2 Rect = GetTextRect(Font, Buff, Layout);
        Rect.Min *= FrameTimeSize;
        Rect.Max *= FrameTimeSize;
        f32 Width = Rect.Max.x - Rect.Min.x;
        v2 P = { (f32)Frame->RenderExtent.width - Width, 0.0f };
        PushTextWithShadow(Frame, Buff, Font, FrameTimeSize, P,  DefaultForegroundColor, Layout);
    }

    if (WasPressed(GameIO->Keys[SC_Backtick]))
    {
        Game->Debug.IsEnabled = !Game->Debug.IsEnabled;
    }

    if (!Game->Debug.IsEnabled)
    {
        return(MouseInputUsed);
    }

    if (WasPressed(GameIO->Keys[SC_F1]))
    {
        Game->Debug.SelectedMenuID = 0;
    }
    else if (WasPressed(GameIO->Keys[SC_F2]))
    {
        Game->Debug.SelectedMenuID = 1;
    }

    f32 Margin = 10.0f;

    f32 TextSize = 32.0f;
    font_layout_type Layout = font_layout_type::Top;

    v2 StartP = { 0.0f, 0.0f };
    v2 CurrentP = StartP;
    u32 CurrentID = 0;
    u32 HotID = INVALID_INDEX_U32;

    auto ResetX = [&]() { CurrentP.x = StartP.x; };

    auto DoButton = [&](const char* Text) -> u32
    {
        mmrect2 Rect = GetTextRect(Font, Text, Layout);
        Rect.Min *= TextSize;
        Rect.Max *= TextSize;
        Rect.Min += CurrentP;
        Rect.Max += CurrentP + v2{ 2.0f * Margin, 0.0f };
        bool IsHot = PointRectOverlap(GameIO->Mouse.P, Rect);
        
        rgba8 CurrentColor = DefaultBackgroundColor;
        if (IsHot)
        {
            HotID = CurrentID;
            if (WasPressed(GameIO->Keys[SC_MouseLeft]))
            {
                Game->Debug.ActiveMenuID = CurrentID;
                MouseInputUsed = true;
            }

            if (Game->Debug.ActiveMenuID == INVALID_INDEX_U32)
            {
                CurrentColor = HotBackgroundColor;
            }
        }

        if (Game->Debug.SelectedMenuID == CurrentID)
        {
            CurrentColor = SelectedBackgroundColor;
        }
        else if (Game->Debug.ActiveMenuID == CurrentID)
        {
            CurrentColor = ActiveBackgroundColor;
            MouseInputUsed = true;
        }
        PushRect(Frame, Rect.Min, Rect.Max, { 0.0f, 0.0f, }, { 0.0f, 0.0f }, CurrentColor);
        CurrentP.x += Margin;
        PushTextWithShadow(Frame, Text, Font, 
                           TextSize, CurrentP,
                           DefaultForegroundColor,
                           Layout);
        CurrentP.x = Rect.Max.x;
        return(CurrentID++);
    };

    u32 CPUMenuID = DoButton("CPU");
    u32 GPUMenuID = DoButton("GPU");
    u32 PostProcessMenuID = DoButton("PostProcess");

    if (WasReleased(GameIO->Keys[SC_MouseLeft]))
    {
        if (Game->Debug.ActiveMenuID != INVALID_INDEX_U32)
        {
            if (Game->Debug.ActiveMenuID == HotID)
            {
                Game->Debug.SelectedMenuID = Game->Debug.ActiveMenuID;
            }
            Game->Debug.ActiveMenuID = INVALID_INDEX_U32;
        }
    }
    else if (!GameIO->Keys[SC_MouseLeft].bIsDown)
    {
        // NOTE(boti): throw away the active menu item in case we got into an inconsistent state
        Game->Debug.ActiveMenuID = INVALID_INDEX_U32;
    }

    CurrentP.x = 0.0f;
    CurrentP.y += TextSize * Font->BaselineDistance; // TODO(boti): this is kind of a hack
    TextSize = 24.0f;

    auto TextLine = [&](const char* Format, ...)
    {
        constexpr size_t BuffSize = 128;
        char Buff[BuffSize];

        va_list ArgList;
        va_start(ArgList, Format);
        vsnprintf(Buff, BuffSize, Format, ArgList);
        va_end(ArgList);

        mmrect2 TextRect = PushTextWithShadow(Frame, Buff, Font, TextSize, CurrentP, DefaultForegroundColor, Layout);
        CurrentP.y = TextRect.Max.y;
    };

    if (Game->Debug.SelectedMenuID == CPUMenuID)
    {

    }
    else if (Game->Debug.SelectedMenuID == GPUMenuID)
    {
        vulkan_renderer* Renderer = Game->Renderer;

        TextLine("RenderTarget memory: %llu/%llu MB",
                     Renderer->RenderTargetHeap.Offset >> 20,
                     Renderer->RenderTargetHeap.MemorySize >> 20);
        TextLine("VertexBuffer memory: %llu/%llu MB",
                     Renderer->GeometryBuffer.VertexMemory.MemoryInUse >> 20,
                     Renderer->GeometryBuffer.VertexMemory.MemorySize >> 20);
        TextLine("IndexBuffer memory: %llu/%llu MB",
                     Renderer->GeometryBuffer.IndexMemory.MemoryInUse >> 20,
                     Renderer->GeometryBuffer.IndexMemory.MemorySize >> 20);
        TextLine("Texture memory: %llu/%llu MB",
                     Renderer->TextureManager.MemoryOffset >> 20,
                     Renderer->TextureManager.MemorySize >> 20);
        TextLine("Shadow memory: %llu/%llu MB",
                     Renderer->ShadowMemoryOffset >> 20,
                     Renderer->ShadowMemorySize >> 20);
    }
    else if (Game->Debug.SelectedMenuID == PostProcessMenuID)
    {
        auto F32Slider = [&](const char* Name, f32* Value, 
            f32 DefaultValue, f32 MinValue, f32 MaxValue, f32 Step) -> u32
        {
            u32 ResetButtonID = DoButton(" ");
            if (ResetButtonID == HotID && WasPressed(GameIO->Keys[SC_MouseLeft]))
            {
                *Value = DefaultValue;
                MouseInputUsed = true;
            }

            constexpr size_t BuffSize = 128;
            char Buff[BuffSize];
            snprintf(Buff, BuffSize, "%s = %f", Name, *Value);
            mmrect2 TextRect = GetTextRect(Font, Buff, Layout);
            TextRect.Min *= TextSize;
            TextRect.Max *= TextSize;
            TextRect.Min += CurrentP;
            TextRect.Max += CurrentP;
            rgba8 TextColor = DefaultForegroundColor;

            b32 IsHot = PointRectOverlap(GameIO->Mouse.P, TextRect);
            if (IsHot)
            {
                if (WasPressed(GameIO->Keys[SC_MouseLeft]))
                {
                    Game->Debug.ActiveMenuID = CurrentID;
                }
            }
            if (Game->Debug.ActiveMenuID == CurrentID)
            {
                *Value += Step * GameIO->Mouse.dP.x;
                *Value = Clamp(*Value, MinValue, MaxValue);
                MouseInputUsed = true;
            }

            if ((IsHot && (Game->Debug.ActiveMenuID == INVALID_INDEX_U32)) ||
                Game->Debug.ActiveMenuID == CurrentID)
            {
                TextColor = PackRGBA8(0xFF, 0xFF, 0x00, 0xFF);
            }


            PushTextWithShadow(Frame, Buff, Font, TextSize, CurrentP, TextColor, Layout);
            CurrentP.y = TextRect.Max.y;
            ResetX();
            return(CurrentID++);
        };
        F32Slider("Bloom filter radius", &Game->PostProcessParams.Bloom.FilterRadius, 
                  bloom_params::DefaultFilterRadius, 0.0f, 1.0f, 1e-4f);
        F32Slider("Bloom internal strength", &Game->PostProcessParams.Bloom.InternalStrength, 
                  bloom_params::DefaultInternalStrength, 0.0f, 1.0f, 1e-3f);
        F32Slider("Bloom strength", &Game->PostProcessParams.Bloom.Strength, 
                  bloom_params::DefaultStrength, 0.0f, 1.0f, 1e-3f);

        F32Slider("SSAO intensity", &Game->PostProcessParams.SSAO.Intensity, 
                  ssao_params::DefaultIntensity, 0.0f, 20.0f, 1e-2f);
        F32Slider("SSAO max distance", &Game->PostProcessParams.SSAO.MaxDistance, 
                  ssao_params::DefaultMaxDistance, 1e-3f, 10.0f, 1e-3f);
        F32Slider("SSAO tangent tau", &Game->PostProcessParams.SSAO.TangentTau, 
                  ssao_params::DefaultTangentTau, 0.0f, 1.0f, 1e-3f);
    }

    return(MouseInputUsed);
}