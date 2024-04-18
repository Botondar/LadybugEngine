
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

//
// GUI
//

enum gui_flow
{
    Flow_Vertical = 0,
    Flow_Horizontal,
};

struct gui_context
{
    render_frame* Frame;
    font* Font;

    rgba8 DefaultForegroundColor;
    rgba8 DefaultBackgroundColor;
    rgba8 SelectedBackgroundColor;
    rgba8 HotBackgroundColor;
    rgba8 ActiveBackgroundColor;

    v2 MouseP;
    v2 MousedP;
    platform_input_key MouseLeft;
    platform_input_key MouseRight;

    v2 CurrentP;
    font_layout_type CurrentLayout;
    gui_flow CurrentFlow;
    b32 IsRightAligned;
    f32 Margin;

    u32 LastElementID;
    u32 HotID;
    u32 ActiveID;

    static constexpr umm TextBufferSize = 1024;
    char TextBuffer[TextBufferSize];
};

internal gui_context BeginGUI(render_frame* Frame, font* Font, game_io* IO);
internal u32 TextGUI(gui_context* Context, f32 Size, const char* Text, ...);
internal u32 ButtonGUI(gui_context* Context, f32 Size, const char* Text, ...);
internal u32 F32Slider(gui_context* Context, f32 Size, const char* Name, f32* Value, f32 DefaultValue, f32 MinValue, f32 MaxValue, f32 Step);

internal v2 AdvanceCursor(gui_context* Context, f32 Size, mmrect2 Rect);

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

    gui_context Context = BeginGUI(Frame, Font, IO);
    Context.ActiveID = Editor->ActiveMenuID;

    if (WasPressed(IO->Keys[SC_R]))
    {
        Game->RenderConfig.ShadingMode = (shading_mode)((Game->RenderConfig.ShadingMode + 1) % ShadingMode_Count);
    }

    // NOTE(boti): we always print the frametime, regardless of whether the debug UI is active or not
    {
        Context.CurrentLayout = font_layout_type::Top;
        Context.CurrentFlow = Flow_Vertical;
        Context.IsRightAligned = true;

        Context.CurrentP = { (f32)Frame->OutputExtent.X, 0.0f };
        TextGUI(&Context, 24.0f, "%.2fms", 1000.0f * IO->dt);
        TextGUI(&Context, 20.0f, "%s", Platform.GetDeviceName(Frame->Renderer));
        TextGUI(&Context, 20.0f, "Output@%ux%u", Frame->OutputExtent.X, Frame->OutputExtent.Y);
        v2u RenderExtent = Frame->RenderExtent;
        if (RenderExtent.X == 0 || RenderExtent.Y == 0)
        {
            RenderExtent = Frame->OutputExtent;
        }
        TextGUI(&Context, 20.0f, "Rendering@%ux%u", RenderExtent.X, RenderExtent.Y);
    }

    if (!Editor->IsEnabled) 
    {
        return;
    }

    if (WasPressed(IO->Keys[SC_L]))
    {
        Editor->DrawLights = !Editor->DrawLights;
    }

    Context.Margin = 10.0f;
    f32 TextSize = 32.0f;
    font_layout_type Layout = font_layout_type::Top;
    Context.CurrentP = { 0.0f, 0.0f };
    Context.CurrentFlow = Flow_Horizontal;
    Context.IsRightAligned = false;

    u32 CPUMenuID = ButtonGUI(&Context, TextSize, "CPU");
    u32 GPUMenuID = ButtonGUI(&Context, TextSize, "GPU");
    u32 RenderMenuID = ButtonGUI(&Context, TextSize, "Render");
    u32 ReloadShadersButtonID = ButtonGUI(&Context, TextSize, "Reload Shaders");

    if (WasReleased(Context.MouseLeft))
    {
        if (Context.ActiveID != 0)
        {
            if (Context.ActiveID == Context.HotID)
            {
                if (Context.ActiveID == ReloadShadersButtonID)
                {
                    Frame->ReloadShaders = true;
                }
                else
                {
                    Editor->SelectedMenuID = Context.ActiveID;
                }
            }
            Context.ActiveID = 0;
        }
    }
    else if (!IO->Keys[SC_MouseLeft].bIsDown)
    {
        // NOTE(boti): throw away the active menu item in case we got into an inconsistent state
        Editor->ActiveMenuID = 0;
    }

    Context.CurrentP.X = 0.0f;
    Context.CurrentP.Y += TextSize * Font->BaselineDistance; // TODO(boti): this is kind of a hack
    TextSize = 24.0f;

    if (Editor->SelectedMenuID == CPUMenuID)
    {
        Context.CurrentFlow = Flow_Vertical;
        for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            constexpr size_t BufferSize = 256;
            char Buffer[BufferSize];
            snprintf(Buffer, BufferSize, "Select Entity%03u", EntityIndex);
            u32 SelectButtonID = ButtonGUI(&Context, TextSize, Buffer);
            if (SelectButtonID == Context.HotID && WasPressed(Context.MouseLeft))
            {
                Editor->SelectedEntityID = { EntityIndex };
            }
            
        }
    }
    else if (Editor->SelectedMenuID == GPUMenuID)
    {
        Context.CurrentFlow = Flow_Vertical;

        render_stats* Stats = &Frame->Stats;
        TextGUI(&Context, TextSize, "Total memory usage: %llu/%llu MB", Stats->TotalMemoryUsed >> 20, Stats->TotalMemoryAllocated >> 20);
        for (u32 Index = 0; Index < Stats->EntryCount; Index++)
        {
            render_stat_entry* Entry = Stats->Entries + Index;
            TextGUI(&Context, TextSize, "%s memory: %llu/%llu MB", Entry->Name, Entry->UsedSize >> 20, Entry->AllocationSize >> 20);
        }
    }
    else if (Editor->SelectedMenuID == RenderMenuID)
    {
        // HACK(boti): The horizontal flow here makes the button and text _within_ the widget behave correctly,
        // but that's not the actual flow of this menu item...
        Context.CurrentFlow = Flow_Horizontal;

        F32Slider(&Context, TextSize,
                  "Exposure", &Game->RenderConfig.Exposure,
                  Game->DefaultExposure, 1e-3f, 10.0f, 1e-3f);

        F32Slider(&Context, TextSize, 
                  "Bloom filter radius", &Game->RenderConfig.BloomFilterRadius, 
                  Game->RenderConfig.DefaultBloomFilterRadius, 0.0f, 1.0f, 1e-4f);
        F32Slider(&Context, TextSize, 
                  "Bloom internal strength", &Game->RenderConfig.BloomInternalStrength, 
                  Game->RenderConfig.DefaultBloomInternalStrength, 0.0f, 1.0f, 1e-3f);
        F32Slider(&Context, TextSize, 
                  "Bloom strength", &Game->RenderConfig.BloomStrength, 
                  Game->RenderConfig.DefaultBloomStrength, 0.0f, 1.0f, 1e-3f);

        F32Slider(&Context, TextSize, 
                  "SSAO intensity", &Game->RenderConfig.SSAOIntensity, 
                  Game->RenderConfig.DefaultSSAOIntensity, 0.0f, 20.0f, 1e-2f);
        F32Slider(&Context, TextSize, 
                  "SSAO max distance", &Game->RenderConfig.SSAOMaxDistance, 
                  Game->RenderConfig.DefaultSSAOMaxDistance, 1e-3f, 10.0f, 1e-3f);
        F32Slider(&Context, TextSize, 
                  "SSAO tangent tau", &Game->RenderConfig.SSAOTangentTau, 
                  Game->RenderConfig.DefaultSSAOTangentTau, 0.0f, 1.0f, 1e-3f);

        F32Slider(&Context, TextSize,
                  "ConstantFogDensity", &Game->ConstantFogDensity,
                  0.0f, 0.0f, F32_MAX_NORMAL, 1e-1f);

        F32Slider(&Context, TextSize,
                  "LinearFogDensityAtBottom", &Game->LinearFogDensityAtBottom,
                  0.0f, 0.0f, F32_MAX_NORMAL, 1e-1f);
        F32Slider(&Context, TextSize,
                  "LinearFogMinZ", &Game->LinearFogMinZ,
                  0.0f, -F32_MAX_NORMAL, Game->LinearFogMaxZ, 1e-1f);
        F32Slider(&Context, TextSize,
                  "LinearFogMaxZ", &Game->LinearFogMaxZ,
                  0.0f, Game->LinearFogMinZ, +F32_MAX_NORMAL, 1e-1f);
    }

    // TODO(boti): This shouldn't be done this way, 
    // all of this is calculated by the world update
    f32 AspectRatio = (f32)Frame->OutputExtent.X / (f32)Frame->OutputExtent.Y;
    f32 g = 1.0f / Tan(0.5f * World->Camera.FieldOfView);
    // Construct a ray the intersects the pixel where the cursor's at
    m4 CameraTransform = GetTransform(&World->Camera);
    m4 ViewTransform = AffineOrthonormalInverse(CameraTransform);
    ray Ray;
    {
        v2 P = IO->Mouse.P;
        P.X /= (f32)Frame->OutputExtent.X;
        P.Y /= (f32)Frame->OutputExtent.Y;
        P.X = 2.0f * P.X - 1.0f;
        P.Y = 2.0f * P.Y - 1.0f;
        P.X *= AspectRatio;

        v3 V = Normalize(v3{ P.X, P.Y, g });

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
                mesh* Mesh = Assets->Meshes + Entity->Pieces[0].MeshID; // HACK(boti): we need to loop through all the pieces here
                mmbox Box = Mesh->BoundingBox;
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

            u32 PlaybackID = ++Context.LastElementID;
            
            v2 ScreenExtent = { (f32)Frame->OutputExtent.X, (f32)Frame->OutputExtent.Y };
            f32 OutlineSize = 1.0f;
            f32 MinX = 0.1f * ScreenExtent.X;
            f32 MaxX = 0.9f * ScreenExtent.X;
            f32 MaxY = ScreenExtent.Y - 60.0f;
            f32 MinY = ScreenExtent.Y - 140.0f;
            PushRect(Frame, { MinX, MinY }, { MaxX, MaxY }, {}, {}, PackRGBA8(0x00, 0x00, 0x00, 0xC0));
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
                Context.HotID = PlaybackID;
                if (WasPressed(Context.MouseLeft))
                {
                    Context.ActiveID = PlaybackID;
                }
            }
            
            if (Context.ActiveID == PlaybackID)
            {
                Entity->AnimationCounter = Clamp(MaxTimestamp * (IO->Mouse.P.X - MinX) / ExtentX, 0.0f, MaxTimestamp);
                if (Context.MouseLeft.bIsDown == false)
                {
                    Context.ActiveID = 0;
                }
            }
        }

        m4 Transform = Entity->Transform;
        v3 InstanceP = Transform.P.XYZ;

        if (Editor->Gizmo.IsGlobal)
        {
            Transform = M4(1.0f, 0.0f, 0.0f, InstanceP.X,
                           0.0f, 1.0f, 0.0f, InstanceP.Y,
                           0.0f, 0.0f, 1.0f, InstanceP.Z,
                           0.0f, 0.0f, 0.0f, 1.0f);
        }

        v3 Axes[3] = {};
        // Orthogonalize the transform, we don't want the gizmos size to change with the scale
        for (u32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
        {
            Axes[AxisIndex] = NOZ(Transform.C[AxisIndex].XYZ);
            Transform.C[AxisIndex].X = Axes[AxisIndex].X;
            Transform.C[AxisIndex].Y = Axes[AxisIndex].Y;
            Transform.C[AxisIndex].Z = Axes[AxisIndex].Z;
        }

        mmbox Box = Assets->Meshes[Assets->ArrowMeshID].BoundingBox;
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

            v2 ScreenAxis = { ScreenLine.Direction.X, ScreenLine.Direction.Y };

            // TODO(boti): Translate by the actual amount that the user dragged along the axis (i.e. reproject to world space)
            constexpr f32 TranslationSpeed = 1e-2f;
            f32 TranslationAmount = TranslationSpeed * Dot(IO->Mouse.dP, ScreenAxis);

            Entity->Transform.P.X += TranslationAmount*Axes[Editor->Gizmo.Selection].X;
            Entity->Transform.P.Y += TranslationAmount*Axes[Editor->Gizmo.Selection].Y;
            Entity->Transform.P.Z += TranslationAmount*Axes[Editor->Gizmo.Selection].Z;

            IO->Mouse.dP = {}; // Don't propagate the mouse dP to the game
        }

        // Render
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
            DrawWidget3D(Frame, Assets->Meshes[Assets->ArrowMeshID].Allocation,
                         CurrentTransform, Color);
        }
    }

    Editor->ActiveMenuID = Context.ActiveID;

    if (Editor->ActiveMenuID)
    {
        IO->Mouse.dP = { 0.0f, 0.0f };
    }
}

internal void PushRect(render_frame* Frame, v2 P1, v2 P2, v2 UV1, v2 UV2, rgba8 Color)
{
    vertex_2d VertexData[] = 
    {
        { { P1.X, P1.Y }, { UV1.X, UV1.Y }, Color },
        { { P2.X, P1.Y }, { UV2.X, UV1.Y }, Color },
        { { P2.X, P2.Y }, { UV2.X, UV2.Y }, Color },
        { { P1.X, P1.Y }, { UV1.X, UV1.Y }, Color },
        { { P2.X, P2.Y }, { UV2.X, UV2.Y }, Color },
        { { P1.X, P2.Y }, { UV1.X, UV2.Y }, Color },
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
        CurrentP.Y += Size * Font->Ascent;
    }
    else if (Layout == font_layout_type::Bottom)
    {
        CurrentP.Y -= Size * Font->Descent;
    }

    for (const char* At = Text; *At; At++)
    {
        const font_glyph* Glyph = Font->Glyphs + Font->CharMapping[*At].GlyphIndex;
        if (*At == '\n')
        {
            CurrentP.Y += Size * Font->BaselineDistance;
            CurrentP.X = P.X;
        }
        else
        {
            PushRect(Frame,
                     CurrentP + Size * Glyph->P0,
                     CurrentP + Size * Glyph->P1,
                     Glyph->UV0, Glyph->UV1,
                     Color);
            CurrentP.X += Size * Glyph->AdvanceX;
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

//
// Gui
//

internal gui_context BeginGUI(render_frame* Frame, font* Font, game_io* IO)
{
    gui_context Context = 
    {
        .Frame = Frame,
        .Font = Font,

        .DefaultForegroundColor = PackRGBA8(0xFF, 0xFF, 0xFF, 0xFF),
        .DefaultBackgroundColor = PackRGBA8(0x00, 0x00, 0xFF, 0xAA),
        .SelectedBackgroundColor = PackRGBA8(0xAA, 0x00, 0xFF, 0xAA),
        .HotBackgroundColor = PackRGBA8(0xFF, 0x00 ,0xFF, 0xAA),
        .ActiveBackgroundColor = PackRGBA8(0xFF, 0x00, 0xFF, 0xFF),

        .MouseP = IO->Mouse.P,
        .MousedP = IO->Mouse.dP,
        .MouseLeft = IO->Keys[SC_MouseLeft],
        .MouseRight = IO->Keys[SC_MouseRight],

        .CurrentP = { 0.0f, 0.0f },
        .CurrentLayout = font_layout_type::Baseline,
        .CurrentFlow = Flow_Vertical,
        .IsRightAligned = false,
        .Margin = 0.0f,

        .LastElementID = 0,
    };
    return(Context);
}

internal v2 AdvanceCursor(gui_context* Context, f32 Size, mmrect2 Rect)
{
    v2 P = Context->CurrentP;
    switch (Context->CurrentFlow)
    {
        case Flow_Vertical:
        {
            P.Y += Size * Context->Font->BaselineDistance;
        } break;
        case Flow_Horizontal:
        {
            P.X = Context->IsRightAligned ? Rect.Min.X : Rect.Max.X;
        } break;
        InvalidDefaultCase;
    }
    return(P);
}

internal u32 TextGUI(gui_context* Context, f32 Size, const char* Format, ...)
{
    u32 ID = ++Context->LastElementID;

    va_list ArgList;
    va_start(ArgList, Format);
    vsnprintf(Context->TextBuffer, Context->TextBufferSize, Format, ArgList);
    
    mmrect2 Rect = GetTextRect(Context->Font, Context->TextBuffer, Context->CurrentLayout);
    Rect.Min *= Size;
    Rect.Max *= Size;
    Rect.Min += Context->CurrentP;
    Rect.Max += Context->CurrentP;
    f32 Width = Rect.Max.X - Rect.Min.X;
    v2 P = Context->CurrentP;
    if (Context->IsRightAligned)
    {
        P.X -= Width;
    }
    PushTextWithShadow(Context->Frame, Context->TextBuffer, Context->Font, Size, P, 
                       Context->DefaultForegroundColor, Context->CurrentLayout);
    Context->CurrentP = AdvanceCursor(Context, Size, Rect);
    return(ID);
}

internal u32 ButtonGUI(gui_context* Context, f32 Size, const char* Format, ...)
{
    u32 ID = ++Context->LastElementID;

    va_list ArgList;
    va_start(ArgList, Format);
    vsnprintf(Context->TextBuffer, Context->TextBufferSize, Format, ArgList);

    mmrect2 Rect = GetTextRect(Context->Font, Context->TextBuffer, Context->CurrentLayout);
    Rect.Min *= Size;
    Rect.Max *= Size;
    Rect.Min += Context->CurrentP;
    Rect.Max += Context->CurrentP + v2{ 2.0f * Context->Margin, 0.0f };
    bool IsHot = PointRectOverlap(Context->MouseP, Rect);

    rgba8 Color = Context->DefaultBackgroundColor;
    if (IsHot)
    {
        Context->HotID = ID;
        Color = Context->HotBackgroundColor;
        if (WasPressed(Context->MouseLeft))
        {
            Context->ActiveID = ID;
        }
    }

    if (Context->ActiveID == ID)
    {
        Color = Context->ActiveBackgroundColor;
    }
    //else if (Context->SelectedID == ID)
    //{
    //Color = Context->SelectedBackgroundColor;
    //}

    PushRect(Context->Frame, Rect.Min, Rect.Max, { 0.0f, 0.0f }, { 0.0f, 0.0f }, Color);
    PushTextWithShadow(Context->Frame, Context->TextBuffer, Context->Font, 
                       Size, { Context->CurrentP.X + Context->Margin, Context->CurrentP.Y },
                       Context->DefaultForegroundColor,
                       Context->CurrentLayout);
    Context->CurrentP = AdvanceCursor(Context, Size, Rect);
    return(ID);
}

internal u32 F32Slider(gui_context* Context, f32 Size, const char* Name, f32* Value, f32 DefaultValue, f32 MinValue, f32 MaxValue, f32 Step)
{
    // HACK(boti): We store the initial position here, so that we can reset the X coordinate when moving to a new line...
    v2 StartP = Context->CurrentP;
    u32 ResetButtonID = ButtonGUI(Context, Size, " ");
    if (ResetButtonID == Context->HotID && WasPressed(Context->MouseLeft))
    {
        *Value = DefaultValue;
    }

    snprintf(Context->TextBuffer, Context->TextBufferSize, "%s = %f", Name, *Value);
    mmrect2 TextRect = GetTextRect(Context->Font, Context->TextBuffer, Context->CurrentLayout);
    TextRect.Min *= Size;
    TextRect.Max *= Size;
    TextRect.Min += Context->CurrentP;
    TextRect.Max += Context->CurrentP;

    u32 RectID = ++Context->LastElementID;
    b32 IsHot = PointRectOverlap(Context->MouseP, TextRect);
    if (IsHot)
    {
        if (WasPressed(Context->MouseLeft))
        {
            Context->ActiveID = RectID;
        }
    }
    if (Context->ActiveID == RectID)
    {
        *Value += Step * Context->MousedP.X;
        *Value = Clamp(*Value, MinValue, MaxValue);
    }

    rgba8 TextColor = Context->DefaultForegroundColor;
    if ((IsHot && (Context->ActiveID == 0)) ||
        Context->ActiveID == RectID)
    {
        TextColor = PackRGBA8(0xFF, 0xFF, 0x00, 0xFF);
    }

    PushTextWithShadow(Context->Frame, Context->TextBuffer, Context->Font, Size, Context->CurrentP, TextColor, Context->CurrentLayout);
    Context->CurrentP.X = StartP.X;
    Context->CurrentP.Y = TextRect.Max.Y;
    return(RectID);
}
