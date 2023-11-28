#include "Win_LadybugEngine.hpp"

#include <cstdarg>
#include <cstdio>

internal const wchar_t* Win_WndClassName = L"lb_wndclass";
internal const wchar_t* Win_WindowTitle = L"LadybugEngine";
internal HINSTANCE WinInstance;
internal HWND WinWindow;

static void Win_DebugPrint(const char* Format, ...)
{
    constexpr size_t BuffSize = 512;
    char Buff[BuffSize];

    va_list ArgList;
    va_start(ArgList, Format);

    vsnprintf(Buff, BuffSize, Format, ArgList);
    OutputDebugStringA(Buff);

    va_end(ArgList);
}

static VkSurfaceKHR Win_CreateVulkanSurface(VkInstance Instance)
{
    VkSurfaceKHR Surface = VK_NULL_HANDLE;
    VkWin32SurfaceCreateInfoKHR CreateInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .hinstance = WinInstance,
        .hwnd = WinWindow,
    };
    vkCreateWin32SurfaceKHR(Instance, &CreateInfo, nullptr, &Surface);
    return(Surface);
}

static buffer Win_LoadEntireFile(const char* Path, memory_arena* Arena)
{
    buffer Result = {};

    HANDLE FileHandle = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            if (FileSize.QuadPart <= 0xFFFFFFFF)
            {
                u64 Size = (u64)FileSize.QuadPart;

                memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Arena);
                void* Memory = PushSize_(Arena, 0, Size, 64);
                if (Memory)
                {

                    DWORD BytesRead = 0;
                    if (ReadFile(FileHandle, Memory, (DWORD)Size, &BytesRead, nullptr))
                    {
                        Result.Size = Size;
                        Result.Data = Memory;
                    }
                    else
                    {
                        RestoreArena(Arena, Checkpoint);
                        DWORD Error = GetLastError();
                        UNREFERENCED_VARIABLE(Error);
                        UnhandledError("File read error");
                    }
                }
            }
        }

        CloseHandle(FileHandle);
    }
    return(Result);
}

internal counter Win_GetCounter()
{
    counter Result = {};
    LARGE_INTEGER Counter;
    QueryPerformanceCounter(&Counter);
    Result.Value = Counter.QuadPart;
    return(Result);
}

internal s64 Win_GetCounterFrequency()
{
    LARGE_INTEGER Result;
    QueryPerformanceFrequency(&Result);
    return Result.QuadPart;
}

internal f32 Win_ElapsedSeconds(counter Start, counter End)
{
    s64 Frequency = Win_GetCounterFrequency();
    f32 Result = (f32)(End.Value - Start.Value) / (f32)Frequency;
    return Result;
}

struct thread_params
{
    thread_procedure* Proc;
    void* Data;
};

internal __stdcall DWORD 
Win_ThreadEntry(void* Data)
{
    thread_params* Params = (thread_params*)Data;
    Params->Proc(Params->Data);
    return(0);
}

// NOTE(boti): Because we need to maintain 2 parameters per thread entry,
// and because we want to avoid the race condition of WinCreateThread returning before WinThreadEntry
// could read the data, we maintain all thread starting parameters here.
// TODO(boti): move this out of global storage
static constexpr u32 MaxThreadCount = 1024u;
static volatile u32 CurrentThreadCount = 0;
static thread_params ThreadParamStorage[MaxThreadCount];

internal void 
Win_CreateThread(thread_procedure* Proc, void* Data, const wchar_t* Name)
{
    u32 ThreadIndex = (u32)(_InterlockedIncrement((long*)&CurrentThreadCount) - 1);
    thread_params* Params = ThreadParamStorage + ThreadIndex;
    Params->Proc = Proc;
    Params->Data = Data;
    HANDLE ThreadHandle = CreateThread(nullptr, 0, &Win_ThreadEntry, Params, 0, nullptr);
    if (Name)
    {
        SetThreadDescription(ThreadHandle, Name);
    }
}

internal platform_semaphore 
Win_CreateSemaphore(u32 InitialCount, u32 MaxCount)
{
    platform_semaphore Semaphore = {};
    Semaphore.Handle = (void*)CreateSemaphoreA(nullptr, (LONG)InitialCount, (LONG)MaxCount, nullptr);
    return(Semaphore);
}

internal void
Win_WaitForSemaphore(platform_semaphore Semaphore, u32 TimeoutMS)
{
    WaitForSingleObjectEx((HANDLE)Semaphore.Handle, TimeoutMS, FALSE);
}

internal void
Win_ReleaseSemaphore(platform_semaphore Semaphore, u32 ReleaseCount, u32* PrevCount)
{
    ReleaseSemaphore((HANDLE)Semaphore.Handle, (LONG)ReleaseCount, (LONG*)PrevCount);
}

//
// Input
//
internal void Win_HandleKeyEvent(game_io* GameIO, u32 KeyCode, u32 ScanCode, bool bIsDown, bool bWasDown, s64 MessageTime)
{
    platform_input_key* Key = GameIO->Keys + ScanCode;

    bool IsTransition = bIsDown != bWasDown;
    Key->bIsDown = bIsDown;
    if (IsTransition)
    {
        Key->TransitionFlags |= KEYTRANSITION_TRANSITION;
    }
    // TODO(boti): Double press handling
}

lbfn u32 Win_ScanCodeToKey(u32 ScanCode, bool bIsExtended)
{
    u32 Result = 0;
    if (bIsExtended)
    {
        switch (ScanCode)
        {
            case SC_Num7: Result = SC_Home; break;
            case SC_Num8: Result = SC_Up; break;
            case SC_Num9: Result = SC_PgUp; break;
            case SC_Num4: Result = SC_Left; break;
            case SC_Num6: Result = SC_Right; break;
            case SC_Num1: Result = SC_End; break;
            case SC_Num2: Result = SC_Down; break;
            case SC_Num0: Result = SC_Insert; break;
            case SC_NumDel: Result = SC_Delete; break;
            case SC_NumMul: Result = SC_PrtScreen; break;

            case SC_Slash: Result = SC_NumDiv; break;
            case SC_Pause: Result = SC_NumLock; break;
            case SC_Enter: Result = SC_NumEnter; break;

            case SC_LeftControl: Result = SC_RightControl; break;
            case SC_LeftAlt: Result = SC_RightAlt; break;

            case SC_LeftWin:
            case SC_RightWin:
            case SC_Menu: Result = ScanCode; break;
        }
    }
    else
    {
        Result = ScanCode;
    }
    return Result;
}

//
// XInput
//
internal DWORD WINAPI XInputGetState_Stub(DWORD UserIndex, XINPUT_STATE* State)
{
    *State = {};
    return ERROR_DEVICE_NOT_CONNECTED;
}

internal DWORD WINAPI XInputSetState_Stub(DWORD UserIndex, XINPUT_VIBRATION* Vibration)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

internal DWORD WINAPI XInputGetCapabilities_Stub(DWORD UserIndex, DWORD Flags, XINPUT_CAPABILITIES* Caps)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

internal bool Win_InitializeXInput(win_xinput* XInput)
{
    bool Result = false;

    if ((XInput->Module = LoadLibraryA("xinput1_4.dll")) == nullptr)
    {
        if ((XInput->Module = LoadLibraryA("xinput9_1_0.dll")) == nullptr)
        {
            XInput->Module = LoadLibraryA("xinput1_3.dll");
        }
    }

    XInput->GetState = &XInputGetState_Stub;
    XInput->SetState = &XInputSetState_Stub;
    XInput->GetCapabilities = &XInputGetCapabilities_Stub;

    if (XInput->Module)
    {
        XInput->GetState = (FN_XInputGetState*)GetProcAddress(XInput->Module, "XInputGetState");
        XInput->SetState = (FN_XInputSetState*)GetProcAddress(XInput->Module, "XInputSetState");
        XInput->GetCapabilities = (FN_XInputGetCapabilities*)GetProcAddress(XInput->Module, "XInputGetCapabilities");

        if (XInput->GetState && XInput->SetState && XInput->GetCapabilities)
        {
            Result = true;
        }
        else
        {
            FreeLibrary(XInput->Module);
            XInput->Module = nullptr;

            XInput->GetState = &XInputGetState_Stub;
            XInput->SetState = &XInputSetState_Stub;
            XInput->GetCapabilities = &XInputGetCapabilities_Stub;
        }
    }

    return Result;
}

internal LRESULT CALLBACK MainWindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_MENUCHAR:
        {
            Result = MNC_CLOSE << 16;
        } break;
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;
        default:
        {
            Result = DefWindowProcW(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

internal DWORD WINAPI Win_MainThread(void* pParams)
{
    HWND ServiceWindow = (HWND)pParams;

    WNDCLASSEXW WindowClass = 
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_OWNDC,
        .lpfnWndProc = &MainWindowProc,
        .hInstance = GetModuleHandleW(nullptr),
        .hCursor = LoadCursorA(nullptr, IDC_ARROW),
        .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
        .lpszClassName = Win_WndClassName,
    };

    if (!RegisterClassExW(&WindowClass))
    {
        return (DWORD)-1;
    }

    game_io GameIO = {};

    // Create window
    {
        DWORD WindowStyle = WS_OVERLAPPEDWINDOW;

        HMONITOR PrimaryMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO MonitorInfo = { sizeof(MONITORINFO) };
        GetMonitorInfo(PrimaryMonitor, &MonitorInfo);

        int MonitorWidth = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
        int MonitorHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;

        int InitialWindowWidth = 1280;
        int InitialWindowHeight = 720;
        if (MonitorWidth < 1280 || MonitorHeight < 720)
        {
            MessageBoxA(nullptr, "Tiny monitor", "LadybugEngine", MB_OK|MB_ICONERROR);
            return((DWORD)-1);
        }
        else if (MonitorWidth <= 1600 || MonitorHeight <= 900)
        {
            InitialWindowWidth = 1280;
            InitialWindowHeight = 720;
        }
        else if (MonitorWidth <= 1920 || MonitorHeight <= 1080)
        {
            InitialWindowWidth = 1600;
            InitialWindowHeight = 900;
        }
        else if (MonitorWidth <= 2560 || MonitorHeight <= 1440)
        {
            InitialWindowWidth = 1920;
            InitialWindowHeight = 1080;
        }

        GameIO.OutputWidth = InitialWindowWidth;
        GameIO.OutputHeight = InitialWindowHeight;

        RECT WindowRect;
        WindowRect.left = (MonitorWidth - InitialWindowWidth) / 2;
        WindowRect.top = (MonitorHeight - InitialWindowHeight) / 2;
        WindowRect.right = WindowRect.left + InitialWindowWidth;
        WindowRect.bottom = WindowRect.top + InitialWindowHeight;

        AdjustWindowRect(&WindowRect, WindowStyle, FALSE);

        win_window_params Params = 
        {
            .ExStyle = WS_EX_ACCEPTFILES,
            .ClassName = Win_WndClassName,
            .WindowName = Win_WindowTitle,
            .Style = WindowStyle|WS_VISIBLE,
            .X = WindowRect.left,
            .Y = WindowRect.top,
            .Width = WindowRect.right - WindowRect.left,
            .Height = WindowRect.bottom - WindowRect.top,
            .Parent = nullptr,
            .Menu = nullptr,
            .Instance = nullptr,
            .Param = nullptr,
        };

        WinWindow = (HWND)SendMessageW(ServiceWindow, CREATE_WINDOW_REMOTE, 0, (LPARAM)&Params);
    }

    if (!WinWindow)
    {
        return (DWORD)-1;
    }
    ShowWindow(WinWindow, SW_SHOW);

    game_update_and_render* Game_UpdateAndRender = nullptr;
    HMODULE GameDLL = LoadLibraryA("game.dll");
    if (GameDLL)
    {
        Game_UpdateAndRender = (game_update_and_render*)GetProcAddress(GameDLL, "Game_UpdateAndRender");
    }
    else
    {
        DWORD ErrorCode = GetLastError();
        constexpr DWORD MaxErrorLength = 1024;
        wchar_t ErrorString[MaxErrorLength];
        FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, ErrorCode, 0, ErrorString, MaxErrorLength, nullptr);
        MessageBoxW(nullptr, ErrorString, L"LadybugEngine", MB_OK|MB_ICONERROR);
        return (DWORD)-1;
    }

    game_memory GameMemory = {};
    GameMemory.Size = GiB(6);
    GameMemory.Memory = VirtualAlloc(nullptr, GameMemory.Size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    GameMemory.PlatformAPI.DebugPrint = &Win_DebugPrint;
    GameMemory.PlatformAPI.GetCounter = &Win_GetCounter;
    GameMemory.PlatformAPI.ElapsedSeconds = &Win_ElapsedSeconds;
    GameMemory.PlatformAPI.LoadEntireFile = &Win_LoadEntireFile;
    GameMemory.PlatformAPI.CreateVulkanSurface = &Win_CreateVulkanSurface;
    GameMemory.PlatformAPI.CreateThread = &Win_CreateThread;
    GameMemory.PlatformAPI.CreateSemaphore = &Win_CreateSemaphore;
    GameMemory.PlatformAPI.WaitForSemaphore = &Win_WaitForSemaphore;
    GameMemory.PlatformAPI.ReleaseSemaphore = &Win_ReleaseSemaphore;

    win_xinput XInput = {};
    Win_InitializeXInput(&XInput);
    {
        POINT P = {};
        GetCursorPos(&P);
        ScreenToClient(WinWindow, &P);
        GameIO.Mouse.P = { (f32)P.x, (f32)P.y };
    }

    for (;;)
    {
        counter FrameStartCounter = Win_GetCounter();

        GameIO.Mouse.dP = { 0.0f, 0.0f };
        for (u32 Key = 0; Key < ScanCode_Count; Key++)
        {
            GameIO.Keys[Key].TransitionFlags = 0;
        }

        MSG Message = {};
        while (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE))
        {
            switch (Message.message)
            {
                case WM_SIZE:
                {
                    if (Message.wParam == SIZE_MINIMIZED)
                    {
                        GameIO.bIsMinimized = true;
                    }
                    else if (Message.wParam == SIZE_RESTORED || Message.wParam == SIZE_MAXIMIZED)
                    {
                        GameIO.bIsMinimized = false;
                        GameIO.OutputWidth = LOWORD(Message.lParam);
                        GameIO.OutputHeight = HIWORD(Message.lParam);
                    }
                } break;

                //
                // Mouse
                //
                case WM_MOUSEMOVE:
                {
                    s32 X = GET_X_LPARAM(Message.lParam);
                    s32 Y = GET_Y_LPARAM(Message.lParam);

                    v2 P = { (f32)X, (f32)Y };
                    GameIO.Mouse.dP += P - GameIO.Mouse.P;
                    GameIO.Mouse.P = P;
                } break;
                case WM_LBUTTONDOWN:
                {
                    u32 KeyCode = VK_LBUTTON;
                    u32 ScanCode = SC_MouseLeft;
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, CAPTURE_MOUSE, 0, (LPARAM)WinWindow);
                } break;
                case WM_RBUTTONDOWN:
                {
                    u32 KeyCode = VK_RBUTTON;
                    u32 ScanCode = SC_MouseRight;
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, CAPTURE_MOUSE, 0, (LPARAM)WinWindow);
                } break;
                case WM_MBUTTONDOWN:
                {
                    u32 KeyCode = VK_MBUTTON;
                    u32 ScanCode = SC_MouseMiddle;
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, CAPTURE_MOUSE, 0, (LPARAM)WinWindow);
                } break;
                case WM_XBUTTONDOWN:
                {
                    u32 KeyCode = 0;
                    u32 ScanCode = 0;
                    if (HIWORD(Message.wParam) & XBUTTON1)
                    {
                        KeyCode = VK_XBUTTON1;
                        ScanCode = SC_MouseX1;
                    }
                    else if (HIWORD(Message.wParam) & XBUTTON2)
                    {
                        KeyCode = VK_XBUTTON2;
                        ScanCode = SC_MouseX2;
                    }
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, CAPTURE_MOUSE, 0, (LPARAM)WinWindow);
                } break;

                case WM_LBUTTONUP:
                {
                    u32 KeyCode = VK_LBUTTON;
                    u32 ScanCode = SC_MouseLeft;
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, RELEASE_MOUSE, 0, 0);
                } break;
                case WM_RBUTTONUP:
                {
                    u32 KeyCode = VK_RBUTTON;
                    u32 ScanCode = SC_MouseRight;
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, RELEASE_MOUSE, 0, 0);
                } break;
                case WM_MBUTTONUP:
                {
                    u32 KeyCode = VK_MBUTTON;
                    u32 ScanCode = SC_MouseMiddle;
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, RELEASE_MOUSE, 0, 0);
                } break;
                case WM_XBUTTONUP:
                {
                    u32 KeyCode = 0;
                    u32 ScanCode = 0;
                    if (HIWORD(Message.wParam) & XBUTTON1)
                    {
                        KeyCode = VK_XBUTTON1;
                        ScanCode = SC_MouseX1;
                    }
                    else if (HIWORD(Message.wParam) & XBUTTON2)
                    {
                        KeyCode = VK_XBUTTON2;
                        ScanCode = SC_MouseX2;
                    }
                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
                    SendMessageW(ServiceWindow, RELEASE_MOUSE, 0, 0);
                } break;

                //
                // Keyboard
                //
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP:
                {
                    s64 MessageTime = (s64)GetMessageTime();
                    u32 KeyCode = (u32)Message.wParam;

                    // TODO(boti): proper left/right extended key handling
                    if (KeyCode == VK_SHIFT)    KeyCode = VK_LSHIFT;
                    if (KeyCode == VK_CONTROL)  KeyCode = VK_LCONTROL;
                    if (KeyCode == VK_MENU)     KeyCode = VK_LMENU;

                    u32 ScanCode    = (Message.lParam >> 16) & 0xFF;
                    b32 IsExtended  = (Message.lParam & (1 << 24)) != 0;
                    b32 IsDown      = (Message.lParam & (1 << 31)) == 0;
                    b32 WasDown     = (Message.lParam & (1 << 30)) != 0;
                    b32 IsAltDown   = (Message.lParam & (1 << 29)) != 0;

                    b32 WasPressed = IsDown && !WasDown;
                    if (WasPressed && IsAltDown && (KeyCode == VK_RETURN))
                    {
                        MONITORINFO MonitorInfo = { sizeof(MONITORINFO) };
                        GetMonitorInfo(MonitorFromWindow(WinWindow, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo);

                        s32 MonitorX = MonitorInfo.rcMonitor.left;
                        s32 MonitorY = MonitorInfo.rcMonitor.top;
                        s32 MonitorWidth = MonitorInfo.rcMonitor.right- MonitorInfo.rcMonitor.left;
                        s32 MonitorHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;

                        DWORD WindowStyle = GetWindowLong(WinWindow, GWL_STYLE);
                        if (WindowStyle & WS_OVERLAPPEDWINDOW)
                        {
                            WindowStyle &= ~WS_OVERLAPPEDWINDOW;
                            SetWindowLong(WinWindow, GWL_STYLE, WindowStyle);
                            SetWindowPos(WinWindow, HWND_TOP,
                                         MonitorX, MonitorY, MonitorWidth, MonitorHeight,
                                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                        }
                        else
                        {
                            WindowStyle |= WS_OVERLAPPEDWINDOW;
                            SetWindowLong(WinWindow, GWL_STYLE, WindowStyle);

                            // TODO(boti): Cache the actual resolution when going fullscreen
                            s32 WindowWidth = 1920;
                            s32 WindowHeight = 1080;
                            s32 WindowX = (MonitorWidth - WindowWidth) / 2;
                            s32 WindowY = (MonitorHeight - WindowHeight) / 2;
                            RECT WindowRect = { WindowX, WindowY, WindowX + WindowWidth, WindowY + WindowHeight };
                            AdjustWindowRect(&WindowRect, WindowStyle, FALSE);

                            SetWindowPos(WinWindow, HWND_TOP, 
                                         WindowRect.left, WindowRect.top, 
                                         WindowRect.right - WindowRect.left, 
                                         WindowRect.bottom - WindowRect.top,
                                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                        }
                    }

                    ScanCode = Win_ScanCodeToKey(ScanCode, IsExtended);

                    Win_HandleKeyEvent(&GameIO, KeyCode, ScanCode, IsDown, WasDown, MessageTime);
                } break;

                case WM_DROPFILES:
                {
                    HDROP Drop = (HDROP)Message.wParam;
                    UINT FileCount = DragQueryFileA(Drop, 0xFFFFFFFFu, nullptr, 0);
                    if (FileCount == 1)
                    {
                        if (DragQueryFileA(Drop, 0, GameIO.DroppedFilename, GameIO.DroppedFilenameLength))
                        {
                            GameIO.bHasDroppedFile = true;
                        }
                    }
                    else
                    {
                        UnimplementedCodePath;
                    }
                    DragFinish(Drop);
                } break;
                case WM_QUIT:
                {
                    GameIO.bQuitRequested = true;
                } break;
            }
        }

        Game_UpdateAndRender(&GameMemory, &GameIO);
        if (GameIO.bQuitRequested) break;

        counter FrameEndCounter = Win_GetCounter();
        GameIO.dt = Win_ElapsedSeconds(FrameStartCounter, FrameEndCounter);

        {
            constexpr size_t BuffSize = 64;
            wchar_t Buff[BuffSize];
            _snwprintf(Buff, BuffSize, L"%s [%5.1f FPS | %5.2f ms]\n", Win_WindowTitle, 1.0f / GameIO.dt, 1000.0f * GameIO.dt);

            SetWindowTextW(WinWindow, Buff);
        }
    }

    if (GameIO.QuitMessage)
    {
        MessageBoxA(nullptr, GameIO.QuitMessage, "LadybugEngine", MB_OK|MB_ICONERROR);
    }

    return(0);
}

internal DWORD MainThreadID;

internal LRESULT CALLBACK Win_ServiceWindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case CREATE_WINDOW_REMOTE:
        {
            const win_window_params* Params = (const win_window_params*)LParam;
            Result = (LRESULT)CreateWindowExW(
                Params->ExStyle, Params->ClassName, Params->WindowName, Params->Style,
                Params->X, Params->Y, Params->Width, Params->Height,
                Params->Parent, Params->Menu, Params->Instance, Params->Param);
        } break;

        case DESTROY_WINDOW_REMOTE:
        {
            Result = (LRESULT)DestroyWindow((HWND)LParam);
        } break;

        case CAPTURE_MOUSE:
        {
            Result = (LRESULT)SetCapture((HWND)LParam);
        } break;

        case RELEASE_MOUSE:
        {
            Result = (LRESULT)ReleaseCapture();
        } break;

        case WM_SIZE:
        {
            wchar_t WindowClassName[256];
            int WindowClassNameLength = GetClassNameW(Window, WindowClassName, 256);
            if (lstrcmpW(WindowClassName, Win_WndClassName) == 0)
            {
                PostThreadMessageW(MainThreadID, Message, WParam, LParam);
            }
        } break;

        default:
        {
            Result = DefWindowProcW(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int CommandShow)
{
    SetThreadDescription(GetCurrentThread(), L"MessageThread");

    WNDCLASSEXW ServiceWindowClass = 
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .style = 0,
        .lpfnWndProc = &Win_ServiceWindowProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = Instance,
        .hIcon = nullptr,
        .hCursor = nullptr,
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = L"lb_srvcls",
        .hIconSm = nullptr,
    };

    if (!RegisterClassExW(&ServiceWindowClass))
    {
        return -1;
    }

    HWND ServiceWindow = CreateWindowExW(
        0, ServiceWindowClass.lpszClassName, L"lb_srvwnd", 0,
        0, 0, 1, 1,
        nullptr, nullptr, ServiceWindowClass.hInstance, nullptr);

    if (!ServiceWindow)
    {
        return -1;
    }

    HANDLE MainThreadHandle = CreateThread(nullptr, 0, &Win_MainThread, ServiceWindow, 0, &MainThreadID);
    SetThreadDescription(MainThreadHandle, L"MainThread");

    for (;;)
    {
        MSG Message;
        GetMessageW(&Message, 0, 0, 0);
        TranslateMessage(&Message);
        DispatchMessageW(&Message);

        PostThreadMessageW(MainThreadID, Message.message, Message.wParam, Message.lParam);

        if (WaitForSingleObject(MainThreadHandle, 0) == WAIT_OBJECT_0)
        {
            break;
        }
    }
    ExitProcess(0);
}