#pragma once

struct line
{
    v3 Direction;
    v3 Moment;
};

union plane
{
    struct 
    {
        v3 N;
        f32 d;
    };
    v4 Plane;
};

enum gizmo_type : u32
{
    Gizmo_Scale = 0,
    Gizmo_Rotate,
    Gizmo_Translate,
};

struct gizmo
{
    gizmo_type Type;
    b32 IsGlobal;
    u32 Selection; // Axis (0,1,2 or INVALID_INDEX)
    v3 P;
};

struct editor_state
{
    b32 IsEnabled;
    u32 SelectedMenuID;
    u32 ActiveMenuID;

    entity_id SelectedEntityID;
    gizmo Gizmo;

    b32 DrawLights;
};

lbfn void InitEditor(struct game_state* Game, memory_arena* Arena);
lbfn void UpdateEditor(struct game_state* Game, game_io* IO, render_frame* Frame);