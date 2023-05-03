#include "Editor.hpp"

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

internal mesh CreateArrowMesh(memory_arena* Arena);

lbfn void InitEditor(game_state* Game, memory_arena* Arena)
{
    Game->Editor.IsEnabled = false;
    Game->Editor.SelectedInstanceIndex = INVALID_INDEX_U32;
    Game->Editor.Gizmo.Type = Gizmo_Translate;
    Game->Editor.Gizmo.Selection = INVALID_INDEX_U32;

    assets* Assets = Game->Assets;

    mesh Mesh = CreateArrowMesh(Arena);
    geometry_buffer_allocation Allocation = UploadVertexData(Game->Renderer,
                                                             Mesh.VertexCount, Mesh.VertexData, 
                                                             Mesh.IndexCount, Mesh.IndexData);
    Assert(Assets->MeshCount < Assets->MaxMeshCount);
    u32 MeshID = Game->Editor.GizmoMeshID = Assets->MeshCount++;
    Game->Editor.GizmoMeshBox = Mesh.Box; // TODO(boti): Why is the bounding box stored separately here?
    Assets->Meshes[MeshID] = Allocation;
    Assets->MeshBoxes[MeshID] = {};
    Assets->MeshMaterialIndices[MeshID] = 0;
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
        u32 SelectedIndex = INVALID_INDEX_U32;
        {
            f32 tMax = 1e7f;
            for (u32 i = 0; i < World->InstanceCount; i++)
            {
                mesh_instance* Instance = World->Instances + i;
                m4 Transform = Instance->Transform;
                mmbox Box = Assets->MeshBoxes[Instance->MeshID];
                v3 BoxP = 0.5f * (Box.Min + Box.Max);
                v3 HalfExtent = 0.5f * (Box.Max - Box.Min);

                f32 t = 0.0f;
                if (IntersectRayBox(Ray, BoxP, HalfExtent, Transform, tMax, &t))
                {
                    SelectedIndex = i;
                    tMax = t;
                }
            }
        }
        Editor->SelectedInstanceIndex = SelectedIndex;
    }

    // Intersect the ray with gizmos
    if (Editor->SelectedInstanceIndex != INVALID_INDEX_U32)
    {
        mesh_instance* Instance = World->Instances + Game->Editor.SelectedInstanceIndex;
        m4 Transform = Instance->Transform;
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
            mmbox Box = Editor->GizmoMeshBox;
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

            Instance->Transform.P.x += TranslationAmount*Axes[Editor->Gizmo.Selection].x;
            Instance->Transform.P.y += TranslationAmount*Axes[Editor->Gizmo.Selection].y;
            Instance->Transform.P.z += TranslationAmount*Axes[Editor->Gizmo.Selection].z;

            IO->Mouse.dP = {}; // Don't propagate the mouse dP to the game
        }
    }
}

internal mesh CreateArrowMesh(memory_arena* Arena)
{
    mesh Result = {};

    rgba8 White = { 0xFFFFFFFFu };

    constexpr f32 Radius = 0.05f;
    constexpr f32 Height = 0.8f;
    constexpr f32 TipRadius = 0.1f;
    constexpr f32 TipHeight = 0.2f;

    // NOTE(boti): Not a real bounding box, but should be better for clicking.
    Result.Box = { { -Radius, -Radius, 0.0f }, { +Radius, +Radius, TipHeight } };

    u32 RadiusDivisionCount = 16;

    u32 CapVertexCount = 1 + RadiusDivisionCount;
    u32 CylinderVertexCount = 2 * RadiusDivisionCount;
    u32 TipCapVertexCount = 2 * RadiusDivisionCount;
    u32 TipVertexCount = TipCapVertexCount + 2*RadiusDivisionCount;
    Result.VertexCount = CapVertexCount + CylinderVertexCount + TipVertexCount;

    u32 CapIndexCount = 3 * RadiusDivisionCount;
    u32 CylinderIndexCount = 2 * 3 * RadiusDivisionCount;
    u32 TipIndexCount = 2 * 3 * RadiusDivisionCount + 3 * RadiusDivisionCount;
    Result.IndexCount = CapIndexCount + CylinderIndexCount + TipIndexCount;

    Result.VertexData = PushArray<vertex>(Arena, Result.VertexCount);
    Result.IndexData = PushArray<u32>(Arena, Result.IndexCount);

    u32* IndexAt = Result.IndexData;
    vertex* VertexAt = Result.VertexData;

    // Bottom cap
    {
        v3 N = { 0.0f, 0.0f, -1.0f };
        v4 T = { 1.0f, 0.0f, 0.0f, 1.0f };
        // Origin
        *VertexAt++ = { { 0.0f, 0.0f, 0.0f }, N, T, {}, White };
        u32 BaseIndex = 1;

        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            v3 P = { Radius * Cos(Angle), Radius * Sin(Angle), 0.0f };
            *VertexAt++ = { P, N, T, {}, White };

            *IndexAt++ = 0;
            *IndexAt++ = ((i + 1) % RadiusDivisionCount) + BaseIndex;
            *IndexAt++ = i + BaseIndex;
        }
    }

    // Cylinder
    {
        u32 BaseIndex = CapVertexCount;
        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            f32 CosA = Cos(Angle);
            f32 SinA = Sin(Angle);

            v3 P = { Radius * CosA, Radius * SinA, 0.0f };
            v3 N = { CosA, SinA, 0.0f };
            v4 T = { -SinA, CosA, 0.0f, 1.0f };

            *VertexAt++ = { P, N, T, {}, White };
            *VertexAt++ = { P + v3{ 0.0f, 0.0f, Height }, N, T, {}, White };

            u32 i00 = 2*i + 0 + BaseIndex;
            u32 i01 = 2*i + 1 + BaseIndex;
            u32 i10 = 2*((i + 1) % RadiusDivisionCount) + 0 + BaseIndex;
            u32 i11 = 2*((i + 1) % RadiusDivisionCount) + 1 + BaseIndex;

            *IndexAt++ = i00;
            *IndexAt++ = i11;
            *IndexAt++ = i01;
                        
            *IndexAt++ = i00;
            *IndexAt++ = i10;
            *IndexAt++ = i11;
        }
    }

    // Tip
    {
        u32 BaseIndex = CapVertexCount + CylinderVertexCount;
        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            f32 CosA = Cos(Angle);
            f32 SinA = Sin(Angle);

            v3 P1 = { Radius * CosA, Radius * SinA, Height };
            v3 P2 = { TipRadius * CosA, TipRadius * SinA, Height  };

            v3 N = { 0.0f, 0.0f, -1.0f };
            v4 T = { 1.0f, 0.0f, 0.0f, 1.0f };
            *VertexAt++ = { P1, N, T, {}, White };
            *VertexAt++ = { P2, N, T, {}, White };

            u32 i00 = 2*i + 0 + BaseIndex;
            u32 i01 = 2*i + 1 + BaseIndex;
            u32 i10 = 2*((i + 1) % RadiusDivisionCount) + 0 + BaseIndex;
            u32 i11 = 2*((i + 1) % RadiusDivisionCount) + 1 + BaseIndex;

            *IndexAt++ = i00;
            *IndexAt++ = i11;
            *IndexAt++ = i01;
                        
            *IndexAt++ = i00;
            *IndexAt++ = i10;
            *IndexAt++ = i11;
        }

        BaseIndex += TipCapVertexCount;

        f32 Slope = TipRadius / TipHeight;
        for (u32 i = 0; i < RadiusDivisionCount; i++)
        {
            f32 Angle = (2.0f * Pi * i) / RadiusDivisionCount;
            f32 CosA = Cos(Angle);
            f32 SinA = Sin(Angle);

            f32 CosT = Cos(2.0f * Pi * (i + 0.5f) / RadiusDivisionCount);
            f32 SinT = Sin(2.0f * Pi * (i + 0.5f) / RadiusDivisionCount);

            v3 P = { TipRadius * CosA, TipRadius * SinA, Height };
            v3 N = Normalize(v3{ CosA, SinA, Slope });
            v3 TipP = { 0.0f, 0.0f, Height + TipHeight };
            v3 TipN = Normalize(v3{ CosT, SinT, Slope });

            v4 T = { -SinA, CosA, 0.0f, 1.0f };

            *VertexAt++ = { P, N, T, {}, White };
            *VertexAt++ = { TipP, TipN, T, {}, White };

            *IndexAt++ = 2 * i + 0 + BaseIndex;
            *IndexAt++ = (2 * ((i + 1) % RadiusDivisionCount)) + 0 + BaseIndex;
            *IndexAt++ = 2 * i + 1 + BaseIndex;
        }
    }

    return(Result);
}