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

lbfn void InitEditor(game_state* Game, memory_arena* Arena)
{
    Game->Editor.IsEnabled = false;
    Game->Editor.SelectedEntityID = { INVALID_INDEX_U32 };
    Game->Editor.Gizmo.Type = Gizmo_Translate;
    Game->Editor.Gizmo.Selection = INVALID_INDEX_U32;
}

lbfn void UpdateEditor(game_state* Game, game_io* IO)
{
    editor_state* Editor = &Game->Editor;
    if (WasPressed(IO->Keys[SC_Backtick]))
    {
        Editor->IsEnabled = !Editor->IsEnabled;
    }

    if (!Editor->IsEnabled) return;

    if (WasPressed(IO->Keys[SC_G]))
    {
        Editor->Gizmo.IsGlobal = !Editor->Gizmo.IsGlobal;
    }

    if (WasPressed(IO->Keys[SC_L]))
    {
        Editor->DrawLights = !Editor->DrawLights;
    }

    game_world* World = Game->World;
    assets* Assets = Game->Assets;

    f32 AspectRatio = (f32)Game->Renderer->SurfaceExtent.width / (f32)Game->Renderer->SurfaceExtent.height;
    f32 g = 1.0f / Tan(0.5f * World->Camera.FieldOfView);
    // Construct a ray the intersects the pixel where the cursor's at
    m4 CameraTransform = GetTransform(&World->Camera);
    m4 ViewTransform = AffineOrthonormalInverse(CameraTransform);
    ray Ray;
    {
        v2 P = IO->Mouse.P;
        P.x /= (f32)Game->Renderer->SurfaceExtent.width;
        P.y /= (f32)Game->Renderer->SurfaceExtent.height;
        P.x = 2.0f * P.x - 1.0f;
        P.y = 2.0f * P.y - 1.0f;
        P.x *= AspectRatio;

        v3 V = Normalize(v3{ P.x, P.y, g });

        Ray = { World->Camera.P, TransformDirection(CameraTransform, V) };
    }

    // Intersect the ray with the scene
    if (WasPressed(IO->Keys[SC_MouseRight]))
    {
        entity_id SelectedEntityID = { U32_MAX };

        f32 tMax = 1e7f;
        for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            entity* Entity = World->Entities + EntityIndex;
            m4 Transform = Entity->Transform;

            if (HasFlag(Entity->Flags, EntityFlag_Mesh))
            {
                mmbox Box = Assets->MeshBoxes[Entity->MeshID];
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
                v3 HalfExtent = v3{ 0.2f, 0.2f, 0.2f };
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

    // Intersect the ray with gizmos
    if (IsValid(Editor->SelectedEntityID))
    {
        m4* InstanceTransform = nullptr;
        entity* Entity = World->Entities + Editor->SelectedEntityID.Value;
        InstanceTransform = &Entity->Transform;

        m4 Transform = *InstanceTransform;
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

        if (!IO->Keys[SC_MouseLeft].bIsDown)
        {
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

            u32 SelectedGizmoIndex = INVALID_INDEX_U32;
            f32 tMax = 1e7f;
            for (u32 GizmoIndex = 0; GizmoIndex < 3; GizmoIndex++)
            {
                m4 CurrentTransform = Transform * GizmoTransforms[GizmoIndex];
                f32 t = 0.0f;
                if (IntersectRayBox(Ray, BoxP, HalfExtent, CurrentTransform, tMax, &t))
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

            InstanceTransform->P.x += TranslationAmount*Axes[Editor->Gizmo.Selection].x;
            InstanceTransform->P.y += TranslationAmount*Axes[Editor->Gizmo.Selection].y;
            InstanceTransform->P.z += TranslationAmount*Axes[Editor->Gizmo.Selection].z;

            IO->Mouse.dP = {}; // Don't propagate the mouse dP to the game
        }
    }
}