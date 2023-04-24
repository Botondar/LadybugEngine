#pragma once

#include "LadybugLib/Core.hpp"
#include <vulkan/vulkan.h>

// High resolution counter
struct counter 
{
    s64 Value;
};

#ifndef VULKAN_H_
typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
#endif

typedef void debug_print(const char* Format, ...);
typedef counter get_counter();
typedef f32 elapsed_seconds(counter Start, counter End);
typedef buffer load_entire_file(const char* Path, memory_arena* Arena);
typedef VkSurfaceKHR create_vulkan_surface(VkInstance Instance);

struct platform_api
{
    debug_print* DebugPrint;
    get_counter* GetCounter;
    elapsed_seconds* ElapsedSeconds;
    load_entire_file* LoadEntireFile;
    create_vulkan_surface* CreateVulkanSurface;
};

// Custom key table for layout-independent keys. Naming follows the US keyboard layout
enum scancode : u32
{
    //
    // Standard non-extended scancodes on a keyboard
    //
    SC_None = 0x00,
    SC_Esc = 0x01,
    SC_1 = 0x02,
    SC_2 = 0x03,
    SC_3 = 0x04,
    SC_4 = 0x05,
    SC_5 = 0x06,
    SC_6 = 0x07,
    SC_7 = 0x08,
    SC_8 = 0x09,
    SC_9 = 0x0A,
    SC_0 = 0x0B,
    SC_Minus = 0x0C,
    SC_Equal = 0x0D,
    SC_Backscape = 0x0E,
    SC_Tab = 0x0F,
    SC_Q = 0x10,
    SC_W = 0x11,
    SC_E = 0x12,
    SC_R = 0x13,
    SC_T = 0x14,
    SC_Y = 0x15,
    SC_U = 0x16,
    SC_I = 0x17,
    SC_O = 0x18,
    SC_P = 0x19,
    SC_OpenBracket = 0x1A,
    SC_CloseBracket = 0x1B,
    SC_Enter = 0x1C,
    SC_LeftControl = 0x1D,
    SC_A = 0x1E,
    SC_S = 0x1F,
    SC_D = 0x20,
    SC_F = 0x21,
    SC_G = 0x22,
    SC_H = 0x23,
    SC_J = 0x24,
    SC_K = 0x25,
    SC_L = 0x26,
    SC_Semicolon = 0x27,
    SC_Quote = 0x28,
    SC_Backtick = 0x29,
    SC_LeftShift = 0x2A,
    SC_Backslash = 0x2B,
    SC_Z = 0x2C,
    SC_X = 0x2D,
    SC_C = 0x2E,
    SC_V = 0x2F,
    SC_B = 0x30,
    SC_N = 0x31,
    SC_M = 0x32,
    SC_Comma = 0x33,
    SC_Period = 0x34,
    SC_Slash = 0x35,
    SC_RightShift = 0x36,
    SC_NumMul = 0x37,
    SC_LeftAlt = 0x38,
    SC_Space = 0x39,
    SC_CapsLock = 0x3A,
    SC_F1 = 0x3B,
    SC_F2 = 0x3C,
    SC_F3 = 0x3D,
    SC_F4 = 0x3E,
    SC_F5 = 0x3F,
    SC_F6 = 0x40,
    SC_F7 = 0x41,
    SC_F8 = 0x42,
    SC_F9 = 0x43,
    SC_F10 = 0x44,
    SC_Pause = 0x45,
    SC_ScrollLock = 0x46,
    SC_Num7 = 0x47,
    SC_Num8 = 0x48,
    SC_Num9 = 0x49,
    SC_NumMinus = 0x4A,
    SC_Num4 = 0x4B,
    SC_Num5 = 0x4C,
    SC_Num6 = 0x4D,
    SC_NumPlus = 0x4E,
    SC_Num1 = 0x4F,
    SC_Num2 = 0x50,
    SC_Num3 = 0x51,
    SC_Num0 = 0x52,
    SC_NumDel = 0x53,
    SC_AltGr = 0x54,
    // = 0x55
    // = 0x56
    SC_F11 = 0x57,
    SC_F12 = 0x58,
    SC_LeftWin = 0x5B,
    SC_RightWin = 0x5C,
    SC_Menu = 0x5D,

    //
    // Custom keys for extended scancodes so that we fit into 0xFF
    //
    SC_Insert, // Num0
    SC_Home, // Num7
    SC_PgUp, // Num9
    SC_Delete, // NumDel
    SC_End, // Num1
    SC_PgDown, // Num3

    SC_Up, // Num8
    SC_Right, // Num6
    SC_Down, // Num2
    SC_Left, // Num4

    SC_NumLock, // Pause/Break
    SC_NumEnter, // Enter
    SC_NumDiv, // Slash

    SC_PrtScreen, // NumMul

    SC_RightControl, // LeftControl
    SC_RightAlt, // LeftAlt
    SC_RightMenu, // Menu - unchanged

    //
    // Mouse buttons for unified mouse and keyboard input
    //
    SC_MouseLeft,
    SC_MouseRight,
    SC_MouseMiddle,
    SC_MouseX1,
    SC_MouseX2,


    // Count helper for arrays
    ScanCode_Count = 0xFF,
};

enum key_transition_flags : u32
{
    KEYTRANSITION_TRANSITION = 1 << 0,
    KEYTRANSITION_DOUBLE_PRESS = 1 << 1,
};

struct platform_input_key
{
    b32 bIsDown;
    u32 TransitionFlags;
};

inline b32 WasPressed(const platform_input_key& Key)
{
    b32 Result = Key.bIsDown && (Key.TransitionFlags & KEYTRANSITION_TRANSITION);
    return Result;
}

inline b32 WasReleased(const platform_input_key& Key)
{
    b32 Result = !Key.bIsDown && (Key.TransitionFlags & KEYTRANSITION_TRANSITION);
    return Result;
}

struct platform_mouse
{
    v2 P;
    v2 dP;
};

struct game_io
{
    f32 dt;

    u32 OutputWidth;
    u32 OutputHeight;

    bool bQuitRequested;
    bool bIsMinimized;
    bool bHasDroppedFile;

    platform_mouse Mouse;
    platform_input_key Keys[ScanCode_Count];

    static constexpr u32 DroppedFilenameLength = 256;
    char DroppedFilename[DroppedFilenameLength];
};

struct game_memory
{
    size_t Size;
    void* Memory;

    struct game_state* GameState;

    platform_api PlatformAPI;
};

extern platform_api Platform;

typedef void game_update_and_render(game_memory* Memory, game_io* GameIO);
//extern "C" void Game_UpdateAndRender(game_memory* Memory, game_io* GameIO);
