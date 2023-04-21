#include "Debug.hpp"

#if 0
internal void DEBUGRenderVBUsage(vulkan_renderer* Renderer, v2 P, v2 Size)
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

internal void DoDebugUI(game_state* Game, game_io* GameIO, render_frame* Frame)
{
    constexpr size_t BuffSize = 128;
    char Buff[BuffSize];

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
        return;
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

    v2 CurrentP = { 0.0f, 0.0f };
    u32 CurrentID = 0;
    u32 HotID = INVALID_INDEX_U32;

    // Menu
    {
        const char* Text = "CPU";
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
            CurrentColor = HotBackgroundColor;
            if (WasPressed(GameIO->Keys[SC_MouseLeft]))
            {
                Game->Debug.ActiveMenuID = CurrentID;
            }
        }

        if (Game->Debug.SelectedMenuID == CurrentID)
        {
            CurrentColor = SelectedBackgroundColor;
        }
        else if (Game->Debug.ActiveMenuID == CurrentID)
        {
            CurrentColor = ActiveBackgroundColor;
        }
        PushRect(Frame, Rect.Min, Rect.Max, { 0.0f, 0.0f, }, { 0.0f, 0.0f }, CurrentColor);
        CurrentP.x += Margin;
        PushTextWithShadow(Frame, Text, Font, 
                           TextSize, CurrentP,
                           DefaultForegroundColor,
                           Layout);
        CurrentP.x = Rect.Max.x;
        CurrentID++;
    }

    {
        const char* Text = "GPU";
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
            CurrentColor = HotBackgroundColor;
            if (WasPressed(GameIO->Keys[SC_MouseLeft]))
            {
                Game->Debug.ActiveMenuID = CurrentID;
            }
        }

        if (Game->Debug.SelectedMenuID == CurrentID)
        {
            CurrentColor = SelectedBackgroundColor;
        }
        else if (Game->Debug.ActiveMenuID == CurrentID)
        {
            CurrentColor = ActiveBackgroundColor;
        }
        PushRect(Frame, Rect.Min, Rect.Max, { 0.0f, 0.0f }, { 0.0f, 0.0f }, CurrentColor);
        CurrentP.x += Margin;
        PushTextWithShadow(Frame, Text, Font,
                           TextSize, CurrentP,
                           DefaultForegroundColor,
                           Layout);
        CurrentP.x = Rect.Max.x;
        CurrentID++;
    }

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

    switch (Game->Debug.SelectedMenuID)
    {
        case 0: // NOTE(boti): CPU
        {
        } break;
        case 1: // NOTE(boti): GPU
        {
            vulkan_renderer* Renderer = Game->Renderer;
            mmrect2 TextRect = {}; 

            snprintf(Buff, BuffSize, "RenderTarget memory: %llu/%llu MB",
                     Renderer->RenderTargetHeap.Offset >> 20,
                     Renderer->RenderTargetHeap.MemorySize >> 20);
            TextRect = PushTextWithShadow(Frame, Buff, Font, TextSize, CurrentP, DefaultForegroundColor, Layout);
            CurrentP.y = TextRect.Max.y;

#if 1
            snprintf(Buff, BuffSize, "VertexBuffer memory: %llu/%llu MB",
                     Renderer->GeometryBuffer.VertexMemory.MemoryInUse >> 20,
                     Renderer->GeometryBuffer.VertexMemory.MemorySize >> 20);
            TextRect = PushTextWithShadow(Frame, Buff, Font, TextSize, CurrentP, DefaultForegroundColor, Layout);
            CurrentP.y = TextRect.Max.y;

            snprintf(Buff, BuffSize, "IndexBuffer memory: %llu/%llu MB",
                     Renderer->GeometryBuffer.IndexMemory.MemoryInUse >> 20,
                     Renderer->GeometryBuffer.IndexMemory.MemorySize >> 20);
            TextRect = PushTextWithShadow(Frame, Buff, Font, TextSize, CurrentP, DefaultForegroundColor, Layout);
            CurrentP.y = TextRect.Max.y;
#endif

#if 0
            v2 VBUsageExtent = { 600.0f, 100.0f };
            DEBUGRenderVBUsage(Game->Renderer, CurrentP, VBUsageExtent);
            CurrentP.y += VBUsageExtent.y;
#endif
            snprintf(Buff, BuffSize, "Texture memory: %llu/%llu MB",
                     Renderer->TextureManager.MemoryOffset >> 20,
                     Renderer->TextureManager.MemorySize >> 20);
            TextRect = PushTextWithShadow(Frame, Buff, Font, TextSize, CurrentP, DefaultForegroundColor, Layout);
            CurrentP.y = TextRect.Max.y;

            snprintf(Buff, BuffSize, "Shadow memory: %llu/%llu MB",
                     Renderer->ShadowMemoryOffset >> 20,
                     Renderer->ShadowMemorySize >> 20);
            TextRect = PushTextWithShadow(Frame, Buff, Font, TextSize, CurrentP, DefaultForegroundColor, Layout);

        } break;
    }
}