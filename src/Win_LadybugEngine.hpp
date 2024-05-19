#include "Platform.hpp"

#include <Windows.h>
#include <windowsx.h>
#include <Shellapi.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <Xinput.h>

// NOTE(boti): If there are any conflicting defines with the Platform.hpp code,
// those should be undefined here
#undef CreateSemaphore

enum win_service_msg
{
    WIN_SERVICE_MSG_UNDEFINED = WM_USER,

    CREATE_WINDOW_REMOTE,
    DESTROY_WINDOW_REMOTE,
    CAPTURE_MOUSE,
    RELEASE_MOUSE,
};

struct win_window_params
{
    DWORD ExStyle;
    LPCWSTR ClassName;
    LPCWSTR WindowName;
    DWORD Style;
    int X;
    int Y;
    int Width;
    int Height;
    HWND Parent;
    HMENU Menu;
    HINSTANCE Instance;
    LPVOID Param;
};

// TODO(boti): This could probably be cross-platform
struct win_ticket_mutex
{
    u64 LastTicket;
    u64 CurrentTicket;
};

inline void BeginTicketMutex(win_ticket_mutex* Mutex);
inline void EndTicketMutex(win_ticket_mutex* Mutex);

internal u32 Win_ScanCodeToKey(u32 ScanCode, bool bIsExtended);

typedef DWORD WINAPI FN_XInputGetState(DWORD UserIndex, XINPUT_STATE* State);
typedef DWORD WINAPI FN_XInputSetState(DWORD UserIndex, XINPUT_VIBRATION* Vibration);
typedef DWORD WINAPI FN_XInputGetCapabilities(DWORD UserIndex, DWORD Flags, XINPUT_CAPABILITIES* Caps);

struct win_xinput
{
    HMODULE Module;

    FN_XInputGetState* GetState;
    FN_XInputSetState* SetState;
    FN_XInputGetCapabilities* GetCapabilities;
};

internal bool Win_InitializeXInput(win_xinput* XInput);