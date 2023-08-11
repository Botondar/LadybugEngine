
internal void PushRect(render_frame* Frame, v2 P1, v2 P2, v2 UV1, v2 UV2, rgba8 Color);
internal mmrect2 PushText(render_frame* Frame, const char* Text, const font* Font, f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);
internal mmrect2 PushTextWithShadow(render_frame* Frame, const char* Text, const font* Font, f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);

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
        v2 P = { (f32)Frame->RenderWidth - Width, 0.0f };
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
    u32 CurrentID = 1;
    u32 HotID = 0;

    if (IsValid(Game->Editor.SelectedEntityID))
    {
        entity* Entity = Game->World->Entities + Game->Editor.SelectedEntityID.Value;
        if (HasFlag(Entity->Flags, EntityFlag_Skin))
        {
            u32 PlaybackID = CurrentID++;

            v2 ScreenExtent = { (f32)Frame->RenderWidth, (f32)Frame->RenderHeight };
            f32 OutlineSize = 1.0f;
            f32 MinX = 0.1f * ScreenExtent.x;
            f32 MaxX = 0.9f * ScreenExtent.x;
            f32 MaxY = ScreenExtent.y - 60.0f;
            f32 MinY = ScreenExtent.y - 140.0f;
            PushRect(Frame, { MinX, MinY - OutlineSize }, { MaxX, MinY + OutlineSize }, {}, {}, PackRGBA8(0xFF, 0xFF, 0xFF));
            PushRect(Frame, { MinX, MaxY - OutlineSize }, { MaxX, MaxY + OutlineSize }, {}, {}, PackRGBA8(0xFF, 0xFF, 0xFF));
            PushRect(Frame, { MinX - OutlineSize, MinY}, { MinX + OutlineSize, MaxY }, {}, {}, PackRGBA8(0xFF, 0xFF, 0xFF));
            PushRect(Frame, { MaxX - OutlineSize, MinY}, { MaxX + OutlineSize, MaxY }, {}, {}, PackRGBA8(0xFF, 0xFF, 0xFF));

            animation* Animation = Game->Assets->Animations + Entity->CurrentAnimationID;
            f32 MaxTimestamp = Animation->KeyFrameTimestamps[Animation->KeyFrameCount - 1];
            f32 ExtentX = (MaxX - MinX);
            f32 PlayX = MinX + ExtentX * Entity->AnimationCounter / MaxTimestamp;
            f32 IndicatorSize = 5.0f;
            PushRect(Frame, { PlayX - IndicatorSize, MinY }, { PlayX + IndicatorSize, MaxY }, {}, {}, PackRGBA8(0xFF, 0xFF, 0xFF));

            for (u32 KeyFrameIndex = 0; KeyFrameIndex < Animation->KeyFrameCount; KeyFrameIndex++)
            {
                f32 Timestamp = Animation->KeyFrameTimestamps[KeyFrameIndex];
                f32 X = MinX + ExtentX * (Timestamp / MaxTimestamp);
                PushRect(Frame, { X - 0.5f, MinY }, { X + 0.5f, MaxY }, {}, {}, PackRGBA8(0xFF, 0xFF, 0x00));
            }

            if (PointRectOverlap(GameIO->Mouse.P, { { MinX, MinY}, { MaxX, MaxY } }))
            {
                HotID = PlaybackID;
                if (WasPressed(GameIO->Keys[SC_MouseLeft]))
                {
                    Game->Debug.ActiveMenuID = PlaybackID;
                }
            }

            if (Game->Debug.ActiveMenuID == PlaybackID)
            {
                Entity->AnimationCounter = Clamp(MaxTimestamp * (GameIO->Mouse.P.x - MinX) / ExtentX, 0.0f, MaxTimestamp);
                MouseInputUsed = true;

                if (GameIO->Keys[SC_MouseLeft].bIsDown == false)
                {
                    Game->Debug.ActiveMenuID = 0;
                }
            }
        }
    }

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

            if (Game->Debug.ActiveMenuID == 0)
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
        if (Game->Debug.ActiveMenuID != 0)
        {
            if (Game->Debug.ActiveMenuID == HotID)
            {
                Game->Debug.SelectedMenuID = Game->Debug.ActiveMenuID;
            }
            Game->Debug.ActiveMenuID = 0;
        }
    }
    else if (!GameIO->Keys[SC_MouseLeft].bIsDown)
    {
        // NOTE(boti): throw away the active menu item in case we got into an inconsistent state
        Game->Debug.ActiveMenuID = 0;
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
        render_stats* Stats = &Frame->Stats;
        for (u32 Index = 0; Index < Stats->EntryCount; Index++)
        {
            render_stat_entry* Entry = Stats->Entries + Index;
            TextLine("%s memory: %llu/%llu MB", Entry->Name, Entry->UsedSize >> 20, Entry->AllocationSize >> 20);
        }
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

internal void PushRect(render_frame* Frame, v2 P1, v2 P2, v2 UV1, v2 UV2, rgba8 Color)
{
    vertex_2d VertexData[] = 
    {
        { { P1.x, P1.y }, { UV1.x, UV1.y }, Color },
        { { P2.x, P1.y }, { UV2.x, UV1.y }, Color },
        { { P2.x, P2.y }, { UV2.x, UV2.y }, Color },
        { { P1.x, P1.y }, { UV1.x, UV1.y }, Color },
        { { P2.x, P2.y }, { UV2.x, UV2.y }, Color },
        { { P1.x, P2.y }, { UV1.x, UV2.y }, Color },
    };

    DrawTriangleList2D(Frame, CountOf(VertexData), VertexData);
}

internal mmrect2 PushText(render_frame* Frame, const char* Text, const font* Font, 
                          f32 Size, v2 P, rgba8 Color, 
                          font_layout_type Layout /*= font_layout_type::Baseline*/)
{
    v2 CurrentP = P;
    if (Layout == font_layout_type::Top)
    {
        CurrentP.y += Size * Font->Ascent;
    }
    else if (Layout == font_layout_type::Bottom)
    {
        CurrentP.y -= Size * Font->Descent;
    }

    for (const char* At = Text; *At; At++)
    {
        const font_glyph* Glyph = Font->Glyphs + Font->CharMapping[*At].GlyphIndex;
        if (*At == '\n')
        {
            CurrentP.y += Size * Font->BaselineDistance;
            CurrentP.x = P.x;
        }
        else
        {
            PushRect(Frame,
                     CurrentP + Size * Glyph->P0,
                     CurrentP + Size * Glyph->P1,
                     Glyph->UV0, Glyph->UV1,
                     Color);
            CurrentP.x += Size * Glyph->AdvanceX;
        }
    }

    // TODO(boti): there's no need to call GetTextRect here, we should just calculate this ourselves in the loop
    mmrect2 Result = GetTextRect(Font, Text, Layout);
    Result.Min *= Size;
    Result.Max *= Size;
    Result.Min += P;
    Result.Max += P;
    return Result;
}

internal mmrect2 PushTextWithShadow(render_frame* Frame, const char* Text, const font* Font, 
                                    f32 Size, v2 P, rgba8 Color, 
                                    font_layout_type Layout /*= font_layout_type::Baseline*/)
{
    f32 ShadowOffset = 0.075f;
    PushText(Frame, Text, Font, Size, P + Size * ShadowOffset * v2{ 1.0f, 1.0f }, PackRGBA8(0x00, 0x00, 0x00), Layout);
    mmrect2 Result = PushText(Frame, Text, Font, Size, P, Color, Layout);
    return Result;
}
