
//
// Render helpers
//
internal void PushRect(render_frame* Frame, v2 P1, v2 P2, v2 UV1, v2 UV2, rgba8 Color);
internal mmrect2 PushText(render_frame* Frame, const char* Text, const font* Font, f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);
internal mmrect2 PushTextWithShadow(render_frame* Frame, const char* Text, const font* Font, f32 Size, v2 P, rgba8 Color, font_layout_type Layout = font_layout_type::Baseline);

//
// 3D widget interaction helpers
//
internal line LineFromPoints(v3 P1, v3 P2);
internal line LineFromPointAndDirection(v3 Direction, v3 P);
internal line ProjectTo(line Line, plane Plane);
internal v3 ProjectTo(v3 P, line Line);
internal line AntiProjectTo(line Line, v3 P);

lbfn void InitEditor(game_state* Game, memory_arena* Arena)
{
    Game->Editor.IsEnabled = false;
    Game->Editor.SelectedEntityID = { 0 };
    Game->Editor.Gizmo.Type = Gizmo_Translate;
    Game->Editor.Gizmo.Selection = INVALID_INDEX_U32;
}

lbfn void UpdateEditor(game_state* Game, game_io* IO, render_frame* Frame)
{
    editor_state* Editor = &Game->Editor;
    game_world* World = Game->World;
    assets* Assets = Game->Assets;
    font* Font = &Assets->DefaultFont;

    if (WasPressed(IO->Keys[SC_Backtick]))
    {
        Editor->IsEnabled = !Editor->IsEnabled;
    }

    rgba8 DefaultForegroundColor = PackRGBA8(0xFF, 0xFF, 0xFF, 0xFF);
    rgba8 DefaultBackgroundColor = PackRGBA8(0x00, 0x00, 0xFF, 0xAA);
    rgba8 SelectedBackgroundColor = PackRGBA8(0xAA, 0x00, 0xFF, 0xAA);
    rgba8 HotBackgroundColor = PackRGBA8(0xFF, 0x00 ,0xFF, 0xAA);
    rgba8 ActiveBackgroundColor = PackRGBA8(0xFF, 0x00, 0xFF, 0xFF);

    // NOTE(boti): we always print the frametime, regardless of whether the debug UI is active or not
    {
        font_layout_type Layout = font_layout_type::Top;
        f32 FrameTimeSize = 24.0f;

        constexpr size_t BuffSize = 256;
        char Buff[BuffSize];

        f32 CurrentY = 0.0f;

        snprintf(Buff, BuffSize, "%.2fms", 1000.0f * IO->dt);
        mmrect2 Rect = GetTextRect(Font, Buff, Layout);
        Rect.Min *= FrameTimeSize;
        Rect.Max *= FrameTimeSize;
        f32 Width = Rect.Max.x - Rect.Min.x;
        v2 P = { (f32)Frame->RenderWidth - Width, CurrentY };
        PushTextWithShadow(Frame, Buff, Font, FrameTimeSize, P,  DefaultForegroundColor, Layout);
        CurrentY += FrameTimeSize * Font->BaselineDistance;

        const char* DeviceName = GetDeviceName(Frame->Renderer);
        f32 DeviceNameSize = 20.0f;
        Rect = GetTextRect(Font, DeviceName, Layout);
        Rect.Min *= DeviceNameSize;
        Rect.Max *= DeviceNameSize;
        Width = Rect.Max.x - Rect.Min.x;
        P = { (f32)Frame->RenderWidth - Width, CurrentY };
        PushTextWithShadow(Frame, DeviceName, Font, DeviceNameSize, P, DefaultForegroundColor, Layout);
        CurrentY += DeviceNameSize * Font->BaselineDistance;

        snprintf(Buff, BuffSize, "Rendering@%ux%u", Frame->RenderWidth, Frame->RenderHeight);
        Rect = GetTextRect(Font, Buff, Layout);
        Rect.Min *= DeviceNameSize;
        Rect.Max *= DeviceNameSize;
        Width = Rect.Max.x - Rect.Min.x;
        P = { (f32)Frame->RenderWidth - Width, CurrentY };
        PushTextWithShadow(Frame, Buff, Font, DeviceNameSize, P, DefaultForegroundColor, Layout);
    }

    if (!Editor->IsEnabled) 
    {
        return;
    }

    if (WasPressed(IO->Keys[SC_L]))
    {
        Editor->DrawLights = !Editor->DrawLights;
    }

    if (WasPressed(IO->Keys[SC_F1]))
    {
        Editor->SelectedMenuID = 0;
    }
    else if (WasPressed(IO->Keys[SC_F2]))
    {
        Editor->SelectedMenuID = 1;
    }

    f32 Margin = 10.0f;

    f32 TextSize = 32.0f;
    font_layout_type Layout = font_layout_type::Top;

    v2 StartP = { 0.0f, 0.0f };
    v2 CurrentP = StartP;
    u32 CurrentID = 1;
    u32 HotID = 0;

    auto ResetX = [&]() { CurrentP.x = StartP.x; };

    auto DoButton = [&](const char* Text) -> u32
    {
        mmrect2 Rect = GetTextRect(Font, Text, Layout);
        Rect.Min *= TextSize;
        Rect.Max *= TextSize;
        Rect.Min += CurrentP;
        Rect.Max += CurrentP + v2{ 2.0f * Margin, 0.0f };
        bool IsHot = PointRectOverlap(IO->Mouse.P, Rect);
        
        rgba8 CurrentColor = DefaultBackgroundColor;
        if (IsHot)
        {
            HotID = CurrentID;
            if (WasPressed(IO->Keys[SC_MouseLeft]))
            {
                Editor->ActiveMenuID = CurrentID;
            }

            if (Editor->ActiveMenuID == 0)
            {
                CurrentColor = HotBackgroundColor;
            }
        }

        if (Editor->SelectedMenuID == CurrentID)
        {
            CurrentColor = SelectedBackgroundColor;
        }
        else if (Editor->ActiveMenuID == CurrentID)
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
        return(CurrentID++);
    };

    u32 CPUMenuID = DoButton("CPU");
    u32 GPUMenuID = DoButton("GPU");
    u32 PostProcessMenuID = DoButton("PostProcess");

    if (WasReleased(IO->Keys[SC_MouseLeft]))
    {
        if (Editor->ActiveMenuID != 0)
        {
            if (Editor->ActiveMenuID == HotID)
            {
                Editor->SelectedMenuID = Editor->ActiveMenuID;
            }
            Editor->ActiveMenuID = 0;
        }
    }
    else if (!IO->Keys[SC_MouseLeft].bIsDown)
    {
        // NOTE(boti): throw away the active menu item in case we got into an inconsistent state
        Editor->ActiveMenuID = 0;
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

    if (Editor->SelectedMenuID == CPUMenuID)
    {
        for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            constexpr size_t BufferSize = 256;
            char Buffer[BufferSize];
            snprintf(Buffer, BufferSize, "Select Entity%03u", EntityIndex);
            u32 SelectButtonID = DoButton(Buffer);
            if (SelectButtonID == HotID && WasPressed(IO->Keys[SC_MouseLeft]))
            {
                Editor->SelectedEntityID = { EntityIndex };
            }
            CurrentP.x = StartP.x;
            CurrentP.y += TextSize;
            
        }
    }
    else if (Editor->SelectedMenuID == GPUMenuID)
    {
        render_stats* Stats = &Frame->Stats;

        TextLine("Total memory usage: %llu/%llu MB", Stats->TotalMemoryUsed >> 20, Stats->TotalMemoryAllocated >> 20);
        for (u32 Index = 0; Index < Stats->EntryCount; Index++)
        {
            render_stat_entry* Entry = Stats->Entries + Index;
            TextLine("%s memory: %llu/%llu MB", Entry->Name, Entry->UsedSize >> 20, Entry->AllocationSize >> 20);
        }
    }
    else if (Editor->SelectedMenuID == PostProcessMenuID)
    {
        auto F32Slider = [&](const char* Name, f32* Value, 
            f32 DefaultValue, f32 MinValue, f32 MaxValue, f32 Step) -> u32
        {
            u32 ResetButtonID = DoButton(" ");
            if (ResetButtonID == HotID && WasPressed(IO->Keys[SC_MouseLeft]))
            {
                *Value = DefaultValue;
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

            b32 IsHot = PointRectOverlap(IO->Mouse.P, TextRect);
            if (IsHot)
            {
                if (WasPressed(IO->Keys[SC_MouseLeft]))
                {
                    Editor->ActiveMenuID = CurrentID;
                }
            }
            if (Editor->ActiveMenuID == CurrentID)
            {
                *Value += Step * IO->Mouse.dP.x;
                *Value = Clamp(*Value, MinValue, MaxValue);
            }

            if ((IsHot && (Editor->ActiveMenuID == 0)) ||
                Editor->ActiveMenuID == CurrentID)
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

    // TODO(boti): This shouldn't be done this way, 
    // all of this is calculated by the world update
    f32 AspectRatio = (f32)Frame->RenderWidth / (f32)Frame->RenderHeight;
    f32 g = 1.0f / Tan(0.5f * World->Camera.FieldOfView);
    // Construct a ray the intersects the pixel where the cursor's at
    m4 CameraTransform = GetTransform(&World->Camera);
    m4 ViewTransform = AffineOrthonormalInverse(CameraTransform);
    ray Ray;
    {
        v2 P = IO->Mouse.P;
        P.x /= (f32)Frame->RenderWidth;
        P.y /= (f32)Frame->RenderHeight;
        P.x = 2.0f * P.x - 1.0f;
        P.y = 2.0f * P.y - 1.0f;
        P.x *= AspectRatio;

        v3 V = Normalize(v3{ P.x, P.y, g });

        Ray = { World->Camera.P, TransformDirection(CameraTransform, V) };
    }

    // Intersect the ray with the scene
    if (WasPressed(IO->Keys[SC_MouseRight]))
    {
        entity_id SelectedEntityID = { 0 };

        f32 tMax = 1e7f;
        for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            entity* Entity = World->Entities + EntityIndex;
            m4 Transform = Entity->Transform;

            if (HasFlag(Entity->Flags, EntityFlag_Mesh))
            {
                mmbox Box = Assets->MeshBoxes[Entity->Pieces[0].MeshID]; // HACK(boti)
                v3 BoxP = 0.5f * (Box.Min + Box.Max);
                v3 HalfExtent = 0.5f * (Box.Max - Box.Min);
        
                f32 t = 0.0f;
                if (IntersectRayBox(Ray, BoxP, HalfExtent, Transform, tMax, &t))
                {
                    SelectedEntityID.Value = EntityIndex;
                    tMax = t;
                }
            }
            else if (HasFlag(Entity->Flags, EntityFlag_LightSource))
            {
                v3 HalfExtent = v3{ World->LightProxyScale, World->LightProxyScale, World->LightProxyScale };
                f32 t = 0.0f;
                if (IntersectRayBox(Ray, v3{ 0.0f, 0.0f, 0.0f }, HalfExtent, Transform, tMax, &t))
                {
                    SelectedEntityID.Value = EntityIndex;
                    tMax = t;
                }
            }
        }
        Editor->SelectedEntityID = SelectedEntityID;
    }

    if (IsValid(Editor->SelectedEntityID))
    {
        entity* Entity = World->Entities + Editor->SelectedEntityID.Value;
        if (WasPressed(IO->Keys[SC_G]))
        {
            Editor->Gizmo.IsGlobal = !Editor->Gizmo.IsGlobal;
        }

        if (HasFlag(Entity->Flags, EntityFlag_Skin))
        {
            if (WasPressed(IO->Keys[SC_P]))
            {
                Entity->DoAnimation = !Entity->DoAnimation;
            }

            // Gather the animations for the entity's skin
            u32 AnimationCount = 0;
            u32 AnimationIDs[9] = { 0 };
            for (u32 AnimationIndex = 0; AnimationIndex < Assets->AnimationCount; AnimationIndex++)
            {
                animation* Animation = Assets->Animations + AnimationIndex;
                if (Animation->SkinID == Entity->SkinID)
                {
                    AnimationIDs[AnimationCount++] = AnimationIndex;
                    if (AnimationCount == CountOf(AnimationIDs))
                    {
                        break;
                    }
                }
            }

            if (WasPressed(IO->Keys[SC_0]))
            {
                Entity->CurrentAnimationID = 0;
                Entity->AnimationCounter = 0.0f;
            }

            for (u32 Scancode = SC_1; Scancode <= SC_9; Scancode++)
            {
                if (WasPressed(IO->Keys[Scancode]))
                {
                    u32 Index = Scancode - SC_1;
                    Entity->CurrentAnimationID = AnimationIDs[Index];
                    Entity->AnimationCounter = 0.0f;
                }
            }

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
            
            if (PointRectOverlap(IO->Mouse.P, { { MinX, MinY}, { MaxX, MaxY } }))
            {
                HotID = PlaybackID;
                if (WasPressed(IO->Keys[SC_MouseLeft]))
                {
                    Editor->ActiveMenuID = PlaybackID;
                }
            }
            
            if (Editor->ActiveMenuID == PlaybackID)
            {
                Entity->AnimationCounter = Clamp(MaxTimestamp * (IO->Mouse.P.x - MinX) / ExtentX, 0.0f, MaxTimestamp);
                if (IO->Keys[SC_MouseLeft].bIsDown == false)
                {
                    Editor->ActiveMenuID = 0;
                }
            }
        }

        m4 Transform = Entity->Transform;
        v3 InstanceP = Transform.P.xyz;

        if (Editor->Gizmo.IsGlobal)
        {
            Transform = M4(1.0f, 0.0f, 0.0f, InstanceP.x,
                           0.0f, 1.0f, 0.0f, InstanceP.y,
                           0.0f, 0.0f, 1.0f, InstanceP.z,
                           0.0f, 0.0f, 0.0f, 1.0f);
        }

        v3 Axes[3] = {};
        // Orthogonalize the transform, we don't want the gizmos size to change with the scale
        for (u32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
        {
            Axes[AxisIndex] = NOZ(Transform.C[AxisIndex].xyz);
            Transform.C[AxisIndex].x = Axes[AxisIndex].x;
            Transform.C[AxisIndex].y = Axes[AxisIndex].y;
            Transform.C[AxisIndex].z = Axes[AxisIndex].z;
        }

        mmbox Box = Assets->MeshBoxes[Assets->ArrowMeshID];
        v3 BoxP = 0.5f * (Box.Min + Box.Max);
        v3 HalfExtent = 0.5f * (Box.Max - Box.Min);

        m4 GizmoTransforms[3] = 
        {
            M4(0.0f, 0.0f, 1.0f, 0.0f,
               1.0f, 0.0f, 0.0f, 0.0f,
               0.0f, 1.0f, 0.0f, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f),
            M4(1.0f, 0.0f, 0.0f, 0.0f,
               0.0f, 0.0f, 1.0f, 0.0f,
               0.0f, -1.0f, 0.0f, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f),
            M4(1.0f, 0.0f, 0.0f, 0.0f,
               0.0f, 1.0f, 0.0f, 0.0f,
               0.0f, 0.0f, 1.0f, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f),
        };

        m4 Transforms[3] = 
        {
            Transform * GizmoTransforms[0],
            Transform * GizmoTransforms[1],
            Transform * GizmoTransforms[2],
        };

        if (!IO->Keys[SC_MouseLeft].bIsDown)
        {
            u32 SelectedGizmoIndex = INVALID_INDEX_U32;
            f32 tMax = 1e7f;
            for (u32 GizmoIndex = 0; GizmoIndex < 3; GizmoIndex++)
            {
                f32 t = 0.0f;
                if (IntersectRayBox(Ray, BoxP, HalfExtent, Transforms[GizmoIndex], tMax, &t))
                {
                    tMax = t;
                    SelectedGizmoIndex = GizmoIndex;
                }
            }
            Editor->Gizmo.Selection = SelectedGizmoIndex;
        }

        if (IO->Keys[SC_MouseLeft].bIsDown && 
            (Editor->Gizmo.Selection != INVALID_INDEX_U32))
        {
            line AxisLine = LineFromPoints(TransformPoint(ViewTransform, InstanceP), 
                                           TransformPoint(ViewTransform, InstanceP + Axes[Editor->Gizmo.Selection])) ;

            plane ProjectionPlane = { .Plane = { 0.0f, 0.0f, 1.0f, g } };
            line ScreenLine = ProjectTo(AxisLine, ProjectionPlane);

            v2 ScreenAxis = { ScreenLine.Direction.x, ScreenLine.Direction.y };

            // TODO(boti): Translate by the actual amount that the user dragged along the axis (i.e. reproject to world space)
            constexpr f32 TranslationSpeed = 1e-2f;
            f32 TranslationAmount = TranslationSpeed * Dot(IO->Mouse.dP, ScreenAxis);

            Entity->Transform.P.x += TranslationAmount*Axes[Editor->Gizmo.Selection].x;
            Entity->Transform.P.y += TranslationAmount*Axes[Editor->Gizmo.Selection].y;
            Entity->Transform.P.z += TranslationAmount*Axes[Editor->Gizmo.Selection].z;

            IO->Mouse.dP = {}; // Don't propagate the mouse dP to the game
        }

        // Render
        geometry_buffer_allocation Mesh = Assets->Meshes[Assets->ArrowMeshID];
        u32 VertexOffset = Mesh.VertexBlock->ByteOffset / sizeof(vertex);
        u32 VertexCount = Mesh.VertexBlock->ByteSize / sizeof(vertex);
        u32 IndexOffset = Mesh.IndexBlock->ByteOffset / sizeof(vert_index);
        u32 IndexCount = Mesh.IndexBlock->ByteSize / sizeof(vert_index);

        rgba8 Colors[3] = 
        {
            PackRGBA8(0xFF, 0x00, 0x00),
            PackRGBA8(0x00, 0xFF, 0x00),
            PackRGBA8(0x00, 0x00, 0xFF),
        };
        rgba8 SelectedColors[3] = 
        {
            PackRGBA8(0xFF, 0x80, 0x80),
            PackRGBA8(0x80, 0xFF, 0x80),
            PackRGBA8(0x80, 0x80, 0xFF),
        };

        for (u32 GizmoIndex = 0; GizmoIndex < 3; GizmoIndex++)
        {
            m4 CurrentTransform = Transforms[GizmoIndex];

            // TODO(boti): How come the transform for the actual raycasting does NOT WORK for rendering?!??!
            CurrentTransform =  CurrentTransform * M4(0.25f, 0.0f, 0.0, 0.0f,
                                                      0.0f, 0.25f, 0.0f, 0.0f,
                                                      0.0f, 0.0f, 0.25f, 0.0f,
                                                      0.0f, 0.0f, 0.0f, 1.0f);
            rgba8 Color = (GizmoIndex == Editor->Gizmo.Selection) ? 
                SelectedColors[GizmoIndex] : Colors[GizmoIndex];
            DrawWidget3D(Frame, VertexOffset, VertexCount, IndexOffset, IndexCount,
                         CurrentTransform, Color);
        }
    }

    if (Editor->ActiveMenuID)
    {
        IO->Mouse.dP = { 0.0f, 0.0f };
    }
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

internal line LineFromPoints(v3 P1, v3 P2)
{
    line Result;
    // TODO(boti): Do we want to normalize here?
    Result.Direction = P2 - P1;
    Result.Moment = Cross(P1, P2);
    return(Result);
}

internal line LineFromPointAndDirection(v3 Direction, v3 P)
{
    line Result;
    LineFromPoints(P, P + Direction);
    return(Result);
}

internal line ProjectTo(line Line, plane Plane)
{
    line Result;

    v3 V = Line.Direction;
    v3 M = Line.Moment;
    v3 N = Plane.N;
    f32 d = Plane.d;

    Result.Direction = Dot(N, N) * V - Dot(N, V) * N;
    Result.Moment = Dot(N, M) * N - d * Cross(N, V);
    return(Result);
};

internal v3 ProjectTo(v3 P, line Line)
{
    v3 Result;
    f32 InvWeight = 1.0f / Dot(Line.Direction, Line.Direction);
    Result = InvWeight * (Dot(Line.Direction, P) * Line.Direction + Cross(Line.Direction, Line.Moment));
    return(Result);
}

internal line AntiProjectTo(line Line, v3 P)
{
    line Result;
    Result.Direction = Line.Direction;
    Result.Moment = Cross(P, Line.Direction);
    return(Result);
}