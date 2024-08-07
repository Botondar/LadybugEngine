#include "Win_LadybugEngine.hpp"

#include "profiler.cpp"

#include <cstdarg>
#include <cstdio>
#include <cwchar>

internal const wchar_t*     Win_WndClassName = L"lb_wndclass";
internal const wchar_t*     Win_WindowTitle = L"LadybugEngine";
internal HINSTANCE          WinInstance;
internal HWND               WinWindow;
internal profiler           GlobalProfiler;

internal const char*        GameDLLName         = "game-tmp.dll";
internal const char*        GameDLLFilename     = "build/game.dll";
internal const char*        GameDLLTempFilename = "build/game-tmp.dll";

internal void Win_DebugPrint(const char* Format, ...);

struct game_dll
{
    HMODULE Module;
    HANDLE File;
    FILETIME LastWriteTime;

    game_update_and_render* UpdateAndRender;
};

internal b32 LoadGameCode(game_dll* DLL)
{
    b32 Result = true;

    DLL->File = CreateFileA(GameDLLFilename, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (DLL->File != INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION Info = {};
        if (GetFileInformationByHandle(DLL->File, &Info))
        {
            DLL->LastWriteTime = Info.ftLastWriteTime;
            if (CopyFileA(GameDLLFilename, GameDLLTempFilename, FALSE))
            {
                DLL->Module = LoadLibraryA(GameDLLName);
                if (DLL->Module)
                {
                    DLL->UpdateAndRender = (game_update_and_render*)GetProcAddress(DLL->Module, Game_UpdateAndRenderFunctionName);
                    Result = (DLL->UpdateAndRender != nullptr);
                }
            }
            else
            {
                DWORD ErrorCode = GetLastError();
                Win_DebugPrint("Failed to copy temp dll: %u\n", ErrorCode);
                Result = false;
            }

        }
        else
        {
            Result = false;
        }
    }
    else
    {
        Result = false;
    }

    return(Result);
}

internal b32 TryReloadGameCode(game_dll* DLL)
{
    b32 Result = true;

    BY_HANDLE_FILE_INFORMATION Info = {};
    if (GetFileInformationByHandle(DLL->File, &Info))
    {
        if (Info.ftLastWriteTime.dwLowDateTime != DLL->LastWriteTime.dwLowDateTime || 
            Info.ftLastWriteTime.dwHighDateTime != DLL->LastWriteTime.dwHighDateTime)
        {
            // NOTE(boti): Currently we don't have any threads running game code that can cross
            // frame boundaries - otherwise this would crash
            FreeLibrary(DLL->Module);
            DLL->UpdateAndRender = nullptr;

            u32 MaxTryCount = 4096;
            u32 Counter = 0;
            for (; Counter < MaxTryCount; Counter++)
            {
                if (CopyFileA(GameDLLFilename, GameDLLTempFilename, FALSE))
                {
                    break;
                }
            }

            if (Counter != MaxTryCount)
            {
                DLL->Module = LoadLibraryA(GameDLLName);
                if (DLL->Module)
                {
                    DLL->LastWriteTime = Info.ftLastWriteTime;

                    DLL->UpdateAndRender = (game_update_and_render*)GetProcAddress(DLL->Module, Game_UpdateAndRenderFunctionName);
                    Result = (DLL->UpdateAndRender != nullptr);
                }
                else
                {
                    Result = false;
                }
            }
            else
            {
                DWORD ErrorCode = GetLastError();
                Win_DebugPrint("Failed to copy temp dll: %u\n", ErrorCode);
                Result = false;
            }
        }
    }
    else
    {
        Result = false;
    }

    return(Result);
}

internal void Win_DebugPrint(const char* Format, ...)
{
    constexpr size_t BuffSize = 1llu << 16;
    char Buff[BuffSize];

    va_list ArgList;
    va_start(ArgList, Format);

    vsnprintf(Buff, BuffSize, Format, ArgList);
    OutputDebugStringA(Buff);

    va_end(ArgList);
}

internal VkSurfaceKHR Win_CreateVulkanSurface(VkInstance Instance)
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

internal b32 Win_ProtectPage(void* Address, umm Size, b32 DoProtect)
{
    DWORD Flags = DoProtect ? PAGE_NOACCESS : PAGE_READWRITE;
    b32 Result = VirtualProtect(Address, Size, Flags, nullptr);
    return(Result);
}


//
// IO
//

internal platform_file Win_OpenFile(const char* Path)
{
    platform_file Result = {};
    Result.Handle = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (Result.Handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize = {};
        BOOL FileSizeResult = GetFileSizeEx(Result.Handle, &FileSize);
        if (FileSizeResult)
        {
            Result.ByteCount = (umm)FileSize.QuadPart;
            Result.IsValid = true;
        }
    }
    return(Result);
}

internal buffer Win_ReadFileContents(platform_file File, memory_arena* Arena)
{
    buffer Result = {};
    if (File.IsValid)
    {
        if (File.ByteCount > 0xFFFFFFFFu)
        {
            UnimplementedCodePath;
        }

        memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Arena);
        if (void* Memory = PushSize_(Arena, 0, File.ByteCount, 64))
        {
            DWORD BytesRead = 0;
            if (ReadFile(File.Handle, Memory, (DWORD)File.ByteCount, &BytesRead, nullptr))
            {
                Result.Size = File.ByteCount;
                Result.Data = Memory;
            }
            else
            {
                RestoreArena(Arena, Checkpoint);
            }
        }
    }
    return(Result);
}

internal void Win_CloseFile(platform_file File)
{
    if (File.IsValid)
    {
        CloseHandle(File.Handle);
    }
}

internal buffer Win_LoadEntireFile(const char* Path, memory_arena* Arena)
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

struct win_io_request
{
    umm ByteOffset;
    umm ByteCount;
    void* Dst;
    platform_file File;

    b32 IsReady; // NOTE(boti): This signals whether the IOThread can start processing the request
};

struct io_queue
{
    u64 CurrentCompletion;
    u64 CompletionGoal;

    win_ticket_mutex Mutex;
    HANDLE Semaphore;

    static constexpr u32 MaxRequestCount = 2048;
    win_io_request RequestBuffer[MaxRequestCount];
};

internal u64 Win_GetIOCompletion(io_queue* Queue)
{
    u64 Result = AtomicLoad(&Queue->CurrentCompletion);
    return(Result);
}

internal u64 Win_PushIORequest(io_queue* Queue, platform_file File, umm ByteOffset, umm ByteCount, void* Dst)
{
    for (;;)
    {
        u64 CurrentCompletion = AtomicLoad(&Queue->CurrentCompletion);
        u64 CompletionGoal = AtomicLoad(&Queue->CompletionGoal);

        if ((CompletionGoal - CurrentCompletion) < Queue->MaxRequestCount)
        {
            break;
        }
        else
        {
            SpinWait;
        }
    }

    u64 Goal = AtomicLoadAndIncrement(&Queue->CompletionGoal);
    u64 Index = Goal % Queue->MaxRequestCount;
    Queue->RequestBuffer[Index] = 
    {
        .ByteOffset = ByteOffset,
        .ByteCount = ByteCount,
        .Dst = Dst,
        .File = File,
    };
    b32 OldValue = AtomicExchange(&Queue->RequestBuffer[Index].IsReady, 1);
    Assert(OldValue == 0);

    ReleaseSemaphore(Queue->Semaphore, 1, nullptr);

    return(Goal);
}

// NOTE(boti): Even when using overlapped IO or IOCP, 
// Windows might decide to synchronously copy the file from its file cache,
// so we use our own thread instead
internal DWORD Win_IOThread(void* Params)
{
    io_queue* Queue = (io_queue*)Params;

    for (;;)
    {
        u64 CurrentCompletion = AtomicLoad(&Queue->CurrentCompletion);
        if (CurrentCompletion != AtomicLoad(&Queue->CompletionGoal))
        {
            u64 Index = CurrentCompletion % Queue->MaxRequestCount;
            win_io_request* Request = Queue->RequestBuffer + Index;
            while (!AtomicLoad(&Request->IsReady))
            {
                SpinWait;
            }

            // Read barrier here

            if (Request->File.IsValid)
            {
                OVERLAPPED Overlapped = {};
                Overlapped.Offset = (DWORD)Request->ByteOffset;
                Overlapped.OffsetHigh = (DWORD)(Request->ByteOffset >> 32);
                
                Assert(Request->ByteCount <= 0xFFFFFFFFu);
                DWORD BytesRead = 0;
                BOOL ReadResult = ReadFile(Request->File.Handle, Request->Dst, (DWORD)Request->ByteCount, &BytesRead, &Overlapped);
                if (!ReadResult)
                {
                    DWORD ErrorCode = GetLastError();
                    Win_DebugPrint("[IOThread] ReadFile failed: %u", ErrorCode);
                }
            }

            AtomicExchange(&Request->IsReady, 0);
            AtomicLoadAndIncrement(&Queue->CurrentCompletion);
        }
        else
        {
            WaitForSingleObject(Queue->Semaphore, INFINITE);
        }
    }

    return(0);
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

struct work_entry
{
    work_procedure* Proc;
    void*           Data;
};

struct work_queue
{
    static constexpr u32 MaxEntryCount = 4096;
    u32         ReadAt;
    u32         WriteAt;
    u32         Completion;
    u32         CompletionGoal;
    work_entry  Entries[MaxEntryCount];

    win_ticket_mutex Mutex;
    HANDLE WorkerSemaphore;
};

internal b32 Win_IsWorkQueueEmpty(work_queue* Queue)
{
    u32 Goal = AtomicLoad(&Queue->CompletionGoal);
    b32 Result = (Goal == AtomicLoad(&Queue->Completion));
    return(Result);
}

internal void Win_AddWorkEntry(work_queue* Queue, work_procedure* Proc, void* Data)
{
    BeginTicketMutex(&Queue->Mutex);

    u32 EntryID = Queue->WriteAt++;
    Assert(EntryID - Queue->ReadAt < Queue->MaxEntryCount);

    u32 Index = EntryID % Queue->MaxEntryCount;
    Queue->Entries[Index] = { Proc, Data };
    Queue->CompletionGoal++;

    ReleaseSemaphore(Queue->WorkerSemaphore, 1, nullptr);

    EndTicketMutex(&Queue->Mutex);
}

internal b32 Win_GetNextWorkEntry(work_queue* Queue, work_entry* Entry)
{
    b32 Result = true;
    BeginTicketMutex(&Queue->Mutex);

    if (Queue->ReadAt < Queue->WriteAt)
    {
        u32 EntryID = Queue->ReadAt++;
        u32 EntryIndex = EntryID % Queue->MaxEntryCount;
        memcpy(Entry, Queue->Entries + EntryIndex, sizeof(*Entry));
    }
    else
    {
        Result = false;
    }

    EndTicketMutex(&Queue->Mutex);
    return(Result);
}

internal void Win_CompleteAllWork(work_queue* Queue, thread_context* ThreadContext)
{
    TimedFunctionMT(&GlobalProfiler, ThreadContext->ThreadID);

    for (;;)
    {
        work_entry Entry;
        b32 EntryResult = Win_GetNextWorkEntry(Queue, &Entry);
        if (EntryResult)
        {
            if (Entry.Proc)
            {
                Entry.Proc(ThreadContext, Entry.Data);
            }
            AtomicLoadAndIncrement(&Queue->Completion);
        }
        else
        {
            break;
        }
    }

    while (!Win_IsWorkQueueEmpty(Queue))
    {
        SpinWait;
    }
}

struct worker_init_info
{
    thread_context ThreadContext;

    work_queue* Queue;
};

internal DWORD Win_WorkerThread(void* Params)
{
    worker_init_info* WorkerInfo = (worker_init_info*)Params;

    thread_context ThreadContext = WorkerInfo->ThreadContext;
    work_queue* Queue            = WorkerInfo->Queue;

    {
        wchar_t ThreadName[256];
        _snwprintf(ThreadName, CountOf(ThreadName), L"LadybugWorker%03", ThreadContext.ThreadID);
        SetThreadDescription(GetCurrentThread(), ThreadName);
    }
    for (;;)
    {
        work_entry Entry;
        b32 EntryResult = Win_GetNextWorkEntry(Queue, &Entry);
        if (EntryResult)
        {
            if (Entry.Proc)
            {
                Entry.Proc(&ThreadContext, Entry.Data);
            }
            AtomicLoadAndIncrement(&Queue->Completion);
        }
        else
        {
            WaitForSingleObject(Queue->WorkerSemaphore, INFINITE);
        }
    }
}

inline void BeginTicketMutex(win_ticket_mutex* Mutex)
{
    u64 TicketValue = AtomicLoadAndIncrement(&Mutex->LastTicket);
    while (TicketValue != AtomicLoad(&Mutex->CurrentTicket))
    {
        SpinWait;
    }
}
inline void EndTicketMutex(win_ticket_mutex* Mutex)
{
    AtomicLoadAndIncrement(&Mutex->CurrentTicket);
}

//
// Audio
//

internal DWORD Win_AudioThread(void* Params)
{
    HRESULT HResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(HResult))
    {
        UnhandledError("CoInitializeEx failed in Win_AudioThread");
    }

    IMMDeviceEnumerator*    DeviceEnumerator    = nullptr;
    IMMDevice*              Device              = nullptr;
    IAudioClient*           AudioClient         = nullptr;
    WAVEFORMATEXTENSIBLE*   DeviceFormat        = nullptr;
    IAudioRenderClient*     RenderClient        = nullptr;

    HResult = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&DeviceEnumerator));
    if (FAILED(HResult))
    {
        UnhandledError("CoCreateInstance failed in Win_AudioThread");
    }

    HResult = DeviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eRender, ERole::eConsole, &Device);
    if (FAILED(HResult))
    {
        UnhandledError("IMMDeviceEnumerator::GetDefaultAudioEndpoint failed in Win_AudioThread");
    }

    HResult = Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&AudioClient);
    if (FAILED(HResult))
    {
        UnhandledError("IMMDevice::Activate failed in Win_AudioThread");
    }

    constexpr u32 RequestChannelCount   = 2;
    constexpr u32 RequestSampleRate     = 48000;
    constexpr u32 RequestBitsPerSample  = 32;
    constexpr u32 RequestBytesPerSample = RequestBitsPerSample / 8;
    constexpr u32 RequestBlockAlign     = RequestChannelCount * RequestBytesPerSample;

    WAVEFORMATEXTENSIBLE RequestFormat = 
    {
        .Format = 
        {
            .wFormatTag         = WAVE_FORMAT_EXTENSIBLE,
            .nChannels          = RequestChannelCount,
            .nSamplesPerSec     = RequestSampleRate,
            .nAvgBytesPerSec    = RequestChannelCount * RequestBytesPerSample * RequestSampleRate,
            .nBlockAlign        = RequestBlockAlign,
            .wBitsPerSample     = RequestBitsPerSample,
            .cbSize             = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX),
        },
        .Samples        = { .wValidBitsPerSample = 32 },
        .dwChannelMask  = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,
        .SubFormat      = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,
    };

    WAVEFORMATEXTENSIBLE* ClosestFormat = nullptr;
    HResult = AudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX*)&RequestFormat, (WAVEFORMATEX**)&ClosestFormat);
    // TODO(boti): Fallback to closest format, handle errors separately to S_FALSE
    if (HResult != S_OK)
    {
        UnhandledError("No suitable audio format");
    }

    constexpr REFERENCE_TIME Milliseconds = 10 * 1000;
    constexpr REFERENCE_TIME BufferDuration = 8 * Milliseconds;
    HResult = AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, BufferDuration, 0, (WAVEFORMATEX*)&RequestFormat, nullptr);
    if (FAILED(HResult))
    {
        UnhandledError("IAudioClient::Initialize failed in Win_AudioThread");
    }

    HANDLE AudioEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (!AudioEvent)
    {
        UnhandledError("CreateEventA failed in Win_AudioThread");
    }

    HResult = AudioClient->SetEventHandle(AudioEvent);
    if (FAILED(HResult))
    {
        UnhandledError("IAudioClient::SetEventHandle failed in Win_AudioThread");
    }

    HResult = AudioClient->GetService(IID_PPV_ARGS(&RenderClient));
    if (FAILED(HResult))
    {
        UnhandledError("IAudioClient::GetService failed in Win_AudioThread");
    }

    u32 BufferSize = 0;
    AudioClient->GetBufferSize(&BufferSize);

    union audio_sample
    {
        f32 Samples[2];
        struct
        {
            f32 Left;
            f32 Right;
        };
    };

    AudioClient->Start();
    f64 CurrentTime = 0.0;
    f64 dtPerSample = 1.0 / RequestSampleRate;
    for (;;)
    {
        WaitForSingleObjectEx(AudioEvent, INFINITE, FALSE);

        u32 Padding = 0;
        AudioClient->GetCurrentPadding(&Padding);
        u32 SampleCount = BufferSize - Padding;

        u8* DstBuffer = nullptr;
        HResult = RenderClient->GetBuffer(SampleCount, &DstBuffer);
        if (SUCCEEDED(HResult))
        {
            #if 0
            audio_sample* Dst = (audio_sample*)DstBuffer;

            u32 CountRemaining = SampleCount;
            while (CountRemaining--)
            {
                constexpr f64 Freq = 2.0 * Pi * 220.0;
                constexpr f64 Amp = 2e-1;
                f64 Sample0_64 = Amp * Sin(Freq * CurrentTime);
                f64 Sample1_64 = Amp * Sin((5.0 / 4.0) * Freq * CurrentTime);
                CurrentTime += dtPerSample;

                f32 Sample0 = (f32)Sample0_64;
                f32 Sample1 = (f32)Sample1_64;

                f32 SampleL = 0.7 * Sample0 + 0.3 * Sample1;
                f32 SampleR = 0.3 * Sample0 + 0.7 * Sample1;
                *Dst++ = { SampleL, SampleR };
            }
            #else
            u32 ByteCount = SampleCount * RequestChannelCount * RequestBytesPerSample;
            memset(DstBuffer, 0, ByteCount);
            #endif

            RenderClient->ReleaseBuffer(SampleCount, 0);
        }
    }
    return(0);
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

internal void Win_ProcessInput(game_io* IO, HWND ServiceWindow)
{
    TimedFunction(&GlobalProfiler);

    IO->Mouse.dP = { 0.0f, 0.0f };
    for (u32 Key = 0; Key < ScanCode_Count; Key++)
    {
        IO->Keys[Key].TransitionFlags = 0;
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
                    IO->bIsMinimized = true;
                }
                else if (Message.wParam == SIZE_RESTORED || Message.wParam == SIZE_MAXIMIZED)
                {
                    IO->bIsMinimized = false;
                    IO->OutputExtent = { (u32)LOWORD(Message.lParam), (u32)HIWORD(Message.lParam) };
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
                IO->Mouse.dP += P - IO->Mouse.P;
                IO->Mouse.P = P;
            } break;
            case WM_LBUTTONDOWN:
            {
                u32 KeyCode = VK_LBUTTON;
                u32 ScanCode = SC_MouseLeft;
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
                SendMessageW(ServiceWindow, CAPTURE_MOUSE, 0, (LPARAM)WinWindow);
            } break;
            case WM_RBUTTONDOWN:
            {
                u32 KeyCode = VK_RBUTTON;
                u32 ScanCode = SC_MouseRight;
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
                SendMessageW(ServiceWindow, CAPTURE_MOUSE, 0, (LPARAM)WinWindow);
            } break;
            case WM_MBUTTONDOWN:
            {
                u32 KeyCode = VK_MBUTTON;
                u32 ScanCode = SC_MouseMiddle;
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
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
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, true, false, (s64)GetMessageTime());
                SendMessageW(ServiceWindow, CAPTURE_MOUSE, 0, (LPARAM)WinWindow);
            } break;

            case WM_LBUTTONUP:
            {
                u32 KeyCode = VK_LBUTTON;
                u32 ScanCode = SC_MouseLeft;
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
                SendMessageW(ServiceWindow, RELEASE_MOUSE, 0, 0);
            } break;
            case WM_RBUTTONUP:
            {
                u32 KeyCode = VK_RBUTTON;
                u32 ScanCode = SC_MouseRight;
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
                SendMessageW(ServiceWindow, RELEASE_MOUSE, 0, 0);
            } break;
            case WM_MBUTTONUP:
            {
                u32 KeyCode = VK_MBUTTON;
                u32 ScanCode = SC_MouseMiddle;
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
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
                Win_HandleKeyEvent(IO, KeyCode, ScanCode, false, true, (s64)GetMessageTime());
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

                Win_HandleKeyEvent(IO, KeyCode, ScanCode, IsDown, WasDown, MessageTime);
            } break;

            case WM_DROPFILES:
            {
                HDROP Drop = (HDROP)Message.wParam;
                UINT FileCount = DragQueryFileA(Drop, 0xFFFFFFFFu, nullptr, 0);
                if (FileCount == 1)
                {
                    if (DragQueryFileA(Drop, 0, IO->DroppedFilename, IO->DroppedFilenameLength))
                    {
                        IO->bHasDroppedFile = true;
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
                IO->bQuitRequested = true;
            } break;
        }
    }
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
        case WM_DEVICECHANGE:
        {
            Win_DebugPrint("WM_DEVICECHANGE\n");
        } break;
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

    //timeBeginPeriod(1);

    thread_context ThreadContext_ = {};
    thread_context* ThreadContext = &ThreadContext_;
    ThreadContext->ThreadID = 0;

    DWORD AudioThreadID = 0;
    HANDLE AudioThread = CreateThread(nullptr, 0, &Win_AudioThread, nullptr, 0, &AudioThreadID);
    if (AudioThread)
    {
        SetThreadDescription(AudioThread, L"AudioThread");
    }
    else
    {
        // TODO(boti): Not a critical failure
        UnhandledError("Failed to create audio thread");
    }

    io_queue IOQueue = {};
    IOQueue.Semaphore = CreateSemaphoreA(nullptr, 0, 1, nullptr);
    DWORD IOThreadID = 0;
    HANDLE IOThread = CreateThread(nullptr, 0, &Win_IOThread, &IOQueue, 0, &IOThreadID);
    if (IOThread)
    {
        SetThreadDescription(IOThread, L"IOThread");
    }
    else
    {
        UnhandledError("Failed to create IO thread");
    }

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

        v2u MonitorExtent = 
        {
            (u32)(MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left),
            (u32)(MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top),
        };

        v2u WindowMonitorExtentPairs[][2] = 
        {
            { { 1280,  720 }, { 1600,  900 } },
            { { 1600,  900 }, { 1920, 1080 } },
            { { 1920, 1080 }, { 2560, 1440 } },
            { { 2560, 1440 }, { 3840, 2160 } },
        };

        if ((MonitorExtent.X < WindowMonitorExtentPairs[0][1].X) ||
            (MonitorExtent.Y < WindowMonitorExtentPairs[0][1].Y))
        {
            MessageBoxA(nullptr, "Tiny monitor", "LadybugEngine", MB_OK|MB_ICONERROR);
            return((DWORD)-1);
        }

        v2u InitialWindowExtent = WindowMonitorExtentPairs[0][0];
        for (u32 Index = 1; Index < CountOf(WindowMonitorExtentPairs); Index++)
        {
            if ((WindowMonitorExtentPairs[Index][1].X <= MonitorExtent.X) ||
                (WindowMonitorExtentPairs[Index][1].Y <= MonitorExtent.Y))
            {
                InitialWindowExtent = WindowMonitorExtentPairs[Index][0];
            }
        }
        GameIO.OutputExtent = InitialWindowExtent;

        RECT WindowRect;
        WindowRect.left = ((s32)MonitorExtent.X - (s32)InitialWindowExtent.X) / 2;
        WindowRect.top  = ((s32)MonitorExtent.Y - (s32)InitialWindowExtent.Y) / 2;
        WindowRect.right    = WindowRect.left + (s32)InitialWindowExtent.X;
        WindowRect.bottom   = WindowRect.top  + (s32)InitialWindowExtent.Y;

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

    // Estimate rdtsc frequency
    u64 TSCFrequency = 1;
#if DEVELOPER
    {
        LARGE_INTEGER QPCFrequency;
        QueryPerformanceFrequency(&QPCFrequency);

        // TODO(boti): Check cpuid invariant tsc
        LARGE_INTEGER BeginQPC;
        QueryPerformanceCounter(&BeginQPC);
        u64 BeginTSC = ReadTSC();

        u64 DeltaQPC;
        u64 EndTSC;
        LARGE_INTEGER EndQPC;
        for (;;)
        {
            QueryPerformanceCounter(&EndQPC);
            DeltaQPC = (u64)(EndQPC.QuadPart - BeginQPC.QuadPart);
            
            if (DeltaQPC >= (u64)QPCFrequency.QuadPart)
            {
                EndTSC = ReadTSC();
                break;
            }
        }

        u64 DeltaTSC = EndTSC - BeginTSC;

        // TODO(boti): f64?
        TSCFrequency = DeltaTSC * (u64)QPCFrequency.QuadPart / DeltaQPC;

        Win_DebugPrint("Frequency estimate: %.2f Mhz\n", TSCFrequency / (1000.0 * 1000.0));
    }
#endif

    game_dll GameDLL = {};
    b32 LoadResult = LoadGameCode(&GameDLL);
    if (!LoadResult)
    {
        DWORD ErrorCode = GetLastError();
        constexpr DWORD MaxErrorLength = 1024;
        wchar_t ErrorString[MaxErrorLength];
        FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, ErrorCode, 0, ErrorString, MaxErrorLength, nullptr);
        MessageBoxW(nullptr, ErrorString, L"LadybugEngine", MB_OK|MB_ICONERROR);
        return (DWORD)-1;
    }

    game_memory GameMemory = {};
    GameMemory.Size = GiB(8);
    GameMemory.Memory = VirtualAlloc(nullptr, GameMemory.Size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    constexpr u32 WorkerCount = 3;
    work_queue WorkQueue = {};
    WorkQueue.WorkerSemaphore = CreateSemaphoreA(nullptr, 0, WorkerCount, nullptr);

    struct worker_info
    {
        HANDLE Handle;
        DWORD ThreadID;
    };
    worker_info Workers[WorkerCount];
    worker_init_info WorkerInitInfos[WorkerCount];

    for (u32 WorkerIndex = 0; WorkerIndex < WorkerCount; WorkerIndex++)
    {
        worker_info* Worker = Workers + WorkerIndex;
        worker_init_info* Init = WorkerInitInfos + WorkerIndex;
        Init->ThreadContext.ThreadID = WorkerIndex + 1;
        Init->Queue = &WorkQueue;
        Worker->Handle = CreateThread(nullptr, 0, &Win_WorkerThread, Init, 0, &Worker->ThreadID);
    }
    
    GameMemory.PlatformAPI.Profiler             = &GlobalProfiler;
    GameMemory.PlatformAPI.Queue                = &WorkQueue;
    GameMemory.PlatformAPI.IOQueue              = &IOQueue;
    GameMemory.PlatformAPI.DebugPrint           = &Win_DebugPrint;
    GameMemory.PlatformAPI.GetCounter           = &Win_GetCounter;
    GameMemory.PlatformAPI.ElapsedSeconds       = &Win_ElapsedSeconds;
    GameMemory.PlatformAPI.LoadEntireFile       = &Win_LoadEntireFile;
    GameMemory.PlatformAPI.CreateVulkanSurface  = &Win_CreateVulkanSurface;
    GameMemory.PlatformAPI.ProtectPage          = &Win_ProtectPage;
    GameMemory.PlatformAPI.AddWorkEntry         = &Win_AddWorkEntry;
    GameMemory.PlatformAPI.CompleteAllWork      = &Win_CompleteAllWork;
    GameMemory.PlatformAPI.OpenFile             = &Win_OpenFile;
    GameMemory.PlatformAPI.CloseFile            = &Win_CloseFile;
    GameMemory.PlatformAPI.ReadFileContents     = &Win_ReadFileContents;
    GameMemory.PlatformAPI.PushIORequest        = &Win_PushIORequest;
    GameMemory.PlatformAPI.GetIOCompletion      = &Win_GetIOCompletion;

    HMODULE RendererDLL = LoadLibraryA("vulkan_renderer.dll");
    if (RendererDLL)
    {
        GameMemory.PlatformAPI.CreateRenderer       = (create_renderer*)    GetProcAddress(RendererDLL, "CreateRenderer");
        GameMemory.PlatformAPI.AllocateGeometry     = (allocate_geometry*)  GetProcAddress(RendererDLL, "AllocateGeometry");
        GameMemory.PlatformAPI.AllocateTexture      = (allocate_texture*)   GetProcAddress(RendererDLL, "AllocateTexture");
        GameMemory.PlatformAPI.BeginRenderFrame     = (begin_render_frame*) GetProcAddress(RendererDLL, "BeginRenderFrame");
        GameMemory.PlatformAPI.EndRenderFrame       = (end_render_frame*)   GetProcAddress(RendererDLL, "EndRenderFrame");

    }
    else
    {
        return (DWORD)-1;
    }

    win_xinput XInput = {};
    Win_InitializeXInput(&XInput);
    {
        POINT P = {};
        GetCursorPos(&P);
        ScreenToClient(WinWindow, &P);
        GameIO.Mouse.P = { (f32)P.x, (f32)P.y };
    }

    //
    // Main loop
    //
    for (;;)
    {
        // Profiler processing
        {
            GameIO.ProfileDeltaTime = (f32)((GlobalProfiler.EndTSC - GlobalProfiler.BeginTSC) / (f64)TSCFrequency);

            // Debug profiler output
            if (0)
            {
                struct processed_profile_entry
                {
                    const char* Label;
                    u64 HitCount;
                    u64 InclusiveDeltaTSC;
                    u64 ExclusiveDeltaTSC;
                };

                processed_profile_entry CollatedEntries[LB_TranslationUnitCount][profiler::MaxEntryCount] = {};

                u64 TotalDelta = GlobalProfiler.EndTSC - GlobalProfiler.BeginTSC;
                for (u32 ThreadIndex = 0; ThreadIndex < WorkerCount + 1; ThreadIndex++)
                {
                    for (u32 TranslationUnit = 0; TranslationUnit < LB_TranslationUnitCount; TranslationUnit++)
                    {
                        for (u32 EntryIndex = 0; EntryIndex < GlobalProfiler.MaxEntryCount; EntryIndex++)
                        {
                            profile_entry* Entry = GlobalProfiler.Entries[ThreadIndex][TranslationUnit] + EntryIndex;
                            if (Entry->HitCount)
                            {
                                processed_profile_entry* ProcessedEntry = CollatedEntries[TranslationUnit] + EntryIndex;
                                ProcessedEntry->Label = Entry->Label;
                                ProcessedEntry->HitCount += Entry->HitCount;
                                ProcessedEntry->InclusiveDeltaTSC += Entry->InclusiveDeltaTSC;
                                ProcessedEntry->ExclusiveDeltaTSC += Entry->ExclusiveDeltaTSC;
                            }
                        }
                    }
                }

                Win_DebugPrint("===== Profiler =====\n");
                for (u32 TranslationUnit = 0; TranslationUnit < LB_TranslationUnitCount; TranslationUnit++)
                {
                    for (u32 EntryIndex = 0; EntryIndex < GlobalProfiler.MaxEntryCount; EntryIndex++)
                    {
                        processed_profile_entry* Entry = CollatedEntries[TranslationUnit] + EntryIndex;
                        if (Entry->HitCount)
                        {
                            f64 InclusivePercent = 100.0 * Entry->InclusiveDeltaTSC / TotalDelta;
                            f64 ExclusivePercent = 100.0 * Entry->ExclusiveDeltaTSC / TotalDelta;

                            if (Entry->InclusiveDeltaTSC != Entry->ExclusiveDeltaTSC)
                            {
                                Win_DebugPrint("%s[%llu]: %.2f%% (%.2f%%)\n",
                                               Entry->Label, Entry->HitCount, ExclusivePercent, InclusivePercent);
                            }
                            else
                            {
                                Win_DebugPrint("%s[%llu]: %.2f%%\n",
                                               Entry->Label, Entry->HitCount, ExclusivePercent);
                            }
                        }
                    }
                }
                Win_DebugPrint("====================\n");
            }
        }

        b32 ReloadResult = TryReloadGameCode(&GameDLL);
        if (!ReloadResult)
        {
            UnhandledError("Failed to reload Game DLL");
        }

        BeginProfiler(&GlobalProfiler);

        counter FrameStartCounter = Win_GetCounter();

        Win_ProcessInput(&GameIO, ServiceWindow);

        GameDLL.UpdateAndRender(ThreadContext, &GameMemory, &GameIO);
        if (GameIO.bQuitRequested) break;

        counter FrameEndCounter = Win_GetCounter();
        GameIO.dt = Win_ElapsedSeconds(FrameStartCounter, FrameEndCounter);

        EndProfiler(&GlobalProfiler);
    }

    if (GameIO.QuitMessage)
    {
        MessageBoxA(nullptr, GameIO.QuitMessage, "LadybugEngine", MB_OK|MB_ICONERROR);
    }

    Win_CompleteAllWork(&WorkQueue, ThreadContext);

    // TODO(boti): Wait for the IO thread to finish instead
    SuspendThread(IOThread);

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
        HANDLE WaitHandles[] = { MainThreadHandle };
        DWORD WaitResult = MsgWaitForMultipleObjects(CountOf(WaitHandles), WaitHandles, FALSE, INFINITE, QS_ALLINPUT);
        if (WaitResult == WAIT_OBJECT_0)
        {
            break;
        }
        else if (WaitResult == WAIT_FAILED)
        {
            // TODO(boti): Check for errors
            ExitProcess(-1);
        }
        else
        {
            MSG Message = {};
            while (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
                PostThreadMessageW(MainThreadID, Message.message, Message.wParam, Message.lParam);
            }
        }
    }
    ExitProcess(0);
}

ProfilerOverflowGuard;