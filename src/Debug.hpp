#pragma once

typedef const char* debug_guid;

#if DEVELOPER
#define DEBUG_GUID(name) debug_guid Name
#else
#define DEBUG_GUID(...)
#endif

struct debug_state
{
    b32 IsEnabled;
    u32 SelectedMenuID;
    u32 ActiveMenuID;
};

struct render_frame;

lbfn void DEBUGRenderVBUsage(struct vulkan_renderer* Renderer, v2 P, v2 Size);
lbfn void DoDebugUI(struct game_state* GameState, struct game_io* GameIO, render_frame* Frame);