#pragma once

inline bool IsSpace(char C);
inline bool IsDigit(char C);
inline bool IsAlpha(char C);

inline bool CopyZStringToBuffer(char* Dst, const char* Src, u64* SizePtr);

struct string
{
    u64 Length;
    char* String;
};

// Checks string with zero-terminated string for equality
// Only returns true if the strings also have equal length
inline bool StringEquals(string String, const char* ZString);

// TODO(boti): There's no need to have this separate from string
struct string_view
{
    const char* At;
    u64 Size;
};

inline char Peek(string_view* View);
inline char Advance(string_view* View);

inline u64 EatWhitespace(string_view* View);
inline bool MatchZAndAdvance(string_view* View, const char* ZString);

struct filepath
{
    static constexpr u32 MaxCount = 320;
    u32 Count;
    u32 NameOffset;
    u32 NameCount;
    // NOTE(boti): The extension includes the dot '.'
    u32 ExtensionOffset; 
    // NOTE(boti): This buffer is null-terminated, so it can be safely passed to system functions
    char Path[MaxCount]; 
};

inline b32 MakeFilepathFromZ(filepath* Path, const char* String);

// NOTE(boti): Extension string must contain the .
inline b32 OverwriteExtension(filepath* Path, const char* Extension);
inline b32 OverwriteExtension(filepath* Path, string Extension);

// NOTE(boti): Overwriting the name and extension with subpath is a valid operation,
// but the path will keep the NameOffset at the _original_ position,
// in order to allow overwriting the same path with multiple different subpaths
inline b32 OverwriteNameAndExtension(filepath* Path, const char* NameAndExtension);
inline b32 OverwriteExtension(filepath* Path, string NameAndExtension);

inline b32 OverwriteSubpath_(filepath* Path, u32 Offset, const char* String);
inline b32 OverwriteSubpath_(filepath* Path, u32 Offset, string String);

//
// Implementation
//

inline bool IsSpace(char C)
{
    bool Result = 
        (C == ' ') ||
        (C == '\t') ||
        (C == '\n') ||
        (C == '\r');
    return Result;
}

inline bool IsDigit(char C)
{
    bool Result = (C >= '0') && (C <= '9');
    return Result;
}

inline bool IsAlpha(char C)
{
    bool Result = ((C >= 'a') && (C <= 'z')) || ((C >= 'A') && (C <= 'Z'));
    return(Result);
}

inline bool CopyZStringToBuffer(char* Dst, const char* Src, u64* SizePtr)
{
    bool Result = false;

    if (SizePtr)
    {
        u64 Size = *SizePtr;
        u64 Count = Size;
        char* DstAt = Dst;
        const char* SrcAt = Src;
        while (Count--)
        {
            char Ch = *Src++;
            *Dst++ = Ch;
            if (Ch == 0) 
            {
                Result = true;
                break;
            }
        }

        if (!Result)
        {
            Dst[Size - 1] = 0;
        }
        *SizePtr = Count + 1;
    }

    return(Result);
}

inline bool StringEquals(string String, const char* ZString)
{
    bool Result = false;

    if (ZString)
    {
        u64 i;
        for (i = 0; i < String.Length; i++)
        {
            if (ZString[i] != String.String[i])
            {
                break;
            }
            else if (ZString[i] == 0)
            {
                break;
            }
        }

        if (i == String.Length)
        {
            Result = (ZString[i] == 0);
        }
    }
    else
    {
        Result = (String.Length == 0);
    }

    return Result;
}

inline char Peek(string_view* View)
{
    char Result = 0;
    if (View->Size)
    {
        Result = *View->At;
    }
    return Result;
}

inline char Advance(string_view* View)
{
    char Result = 0;
    if (View->Size)
    {
        Result = *View->At++;
        View->Size--;
    }
    return Result;
}

inline u64 EatWhitespace(string_view* View)
{
    u64 Result = 0;

    while (IsSpace(Peek(View)))
    {
        Advance(View);
        Result++;
    }

    return Result;
}

inline bool MatchZAndAdvance(string_view* View, const char* ZString)
{
    bool Result = true;

    const char* Start = View->At;
    u64 StartSize = View->Size;

    for (u64 i = 0; ZString[i]; i++)
    {
        if (Advance(View) != ZString[i])
        {
            Result = false;
            View->At = Start;
            View->Size = StartSize;
            break;
        }
    }
    return Result;
}

inline b32 MakeFilepathFromZ(filepath* Path, const char* String)
{
    b32 Result = true;

    for (Path->Count = 0; Path->Count < Path->MaxCount; Path->Count++)
    {
        if ((Path->Path[Path->Count] = String[Path->Count]) == 0)
        {
            break;
        }
    }

    if (Path->Count != Path->MaxCount)
    {
        // NOTE(boti): Find extension substring
        Path->ExtensionOffset = Path->Count; // NOTE(boti): this is considered to be no file extension
        for (u32 Index = Path->Count - 1; Index < Path->Count; Index--)
        {
            if (Path->Path[Index] == '.')
            {
                Path->ExtensionOffset = Index;
                break;
            }
        }

        // NOTE(boti): Find filename substring
        Path->NameOffset = 0;
        Path->NameCount = Path->ExtensionOffset;
        for (u32 Index = Path->ExtensionOffset; Index < Path->Count; Index--)
        {
            if (Path->Path[Index] == '/' || Path->Path[Index] == '\\')
            {
                Path->NameOffset = Index + 1;
                Path->NameCount = Path->ExtensionOffset - Path->NameOffset;
                break;
            }
        }
    }
    else
    {
        *Path = {};
        Result = false;
#if DEVELOPER
        UnhandledError("Path creation failed");
#endif
    }
    return(Result);
}

inline b32 OverwriteSubpath_(filepath* Path, u32 Offset, const char* String)
{
    b32 Result = true;

    const char* At = String;
    for (Path->Count = Offset; Path->Count < Path->MaxCount; Path->Count++)
    {
        if ((Path->Path[Path->Count] = *At++) == 0)
        {
            break;
        }
    }

    if (Path->Count == Path->MaxCount)
    {
        Result = false;
#if DEVELOPER
        UnhandledError("Path overwrite failed");
#endif
    }
    return(Result);
}

inline b32 OverwriteSubpath_(filepath* Path, u32 Offset, string String)
{
    b32 Result = true;

    if (String.Length < (Path->MaxCount - Path->Count))
    {
        Path->Count = Offset + String.Length;
        char* Dst = Path->Path + Offset;
        while (String.Length--)
        {
            *Dst++ = *String.String++;
        }
        Path->Path[Path->Count] = 0;
    }
    else
    {
        Result = false;
#if DEVELOPER
        UnhandledError("Path overwrite failed");
#endif
    }

    return(Result);
}

inline b32 OverwriteExtension(filepath* Path, const char* Extension)
{
    b32 Result = OverwriteSubpath_(Path, Path->ExtensionOffset, Extension);
    return(Result);
}
inline b32 OverwriteNameAndExtension(filepath* Path, const char* NameAndExtension)
{
    b32 Result = OverwriteSubpath_(Path, Path->NameOffset, NameAndExtension);
    return(Result);
}

inline b32 OverwriteExtension(filepath* Path, string Extension)
{
    b32 Result = OverwriteSubpath_(Path, Path->ExtensionOffset, Extension);
    return(Result);
}
inline b32 OverwriteNameAndExtension(filepath* Path, string NameAndExtension)
{
    b32 Result = OverwriteSubpath_(Path, Path->NameOffset, NameAndExtension);
    return(Result);
}
