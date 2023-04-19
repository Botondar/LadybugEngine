#include "lbfnt.hpp"

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <usp10.h>

#include <cinttypes>
#include <cassert>
#include <cstdio>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "usp10.lib")

#define USE_DEBUG_VIS 1

static void DebugVis(
    const lbfnt_font_info* Font,
    u32 CharCount, const lbfnt_charmap* CharMap,
    u32 GlyphCount, const lbfnt_glyph* Glyphs,
    int Width, int Height, void* Pixels);

int main(int ArgCount, char** Args)
{
    if (ArgCount > 1)
    {
        if (strcmp(Args[1], "--enumerate-fonts") == 0)
        {
            assert(!"Unimplemented code path");
            return 0;
        }
    }
    else
    {
        // TODO(boti): print usage
    }

    //
    // GDI init
    //

    HDC DeviceContext = CreateCompatibleDC(nullptr);

    int32_t Width = 1024;
    int32_t Height = 512;
    uint16_t BitsPerPixel = 32;
    
    BITMAPINFO BitmapInfo = 
    {
        .bmiHeader = 
        {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = Width,
            .biHeight = Height,
            .biPlanes = 1,
            .biBitCount = BitsPerPixel,
            .biCompression = BI_RGB,
            .biXPelsPerMeter = 0,
            .biYPelsPerMeter = 0,
            .biClrUsed = 0,
            .biClrImportant = 0,
        },
    };
    void* SrcPixels = nullptr;
    HBITMAP Bitmap = CreateDIBSection(DeviceContext, &BitmapInfo, DIB_RGB_COLORS, &SrcPixels, nullptr, 0);
    SelectObject(DeviceContext, Bitmap);

    int GdiFontHeight = 32;
    HFONT Font = CreateFontW(GdiFontHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, 
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FIXED_PITCH, 
        L"Liberation Mono");

    SelectObject(DeviceContext, Font);

    //
    // Direct2D init
    //

    ID2D1Factory* D2DFactory = nullptr;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&D2DFactory));
    if (FAILED(hr))
    {
        fprintf(stderr, "Failed to initialize Direct2D (0x%x)\n", hr);
        return -1;
    }

    //
    // DWrite init
    //

    IDWriteFactory* DWriteFactory = nullptr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&DWriteFactory);
    if (FAILED(hr))
    {
        fprintf(stderr, "Failed to initialize DirectWrite (0x%x)\n", hr);
        return -1;
    }

    IDWriteGdiInterop* GdiInterop = nullptr;
    DWriteFactory->GetGdiInterop(&GdiInterop);

    IDWriteBitmapRenderTarget* RenderTarget = nullptr;
    GdiInterop->CreateBitmapRenderTarget(DeviceContext, Width, Height, &RenderTarget);

    IDWriteRenderingParams* RenderingParams = nullptr;
    DWriteFactory->CreateCustomRenderingParams(1.0f, 1.0f, 0.0f, DWRITE_PIXEL_GEOMETRY_RGB, DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, &RenderingParams);
    //DWriteFactory->CreateRenderingParams(&RenderingParams);

    IDWriteFontFace* FontFaceBase = nullptr;
    GdiInterop->CreateFontFaceFromHdc(DeviceContext, &FontFaceBase);

    IDWriteFontFace1* FontFace = nullptr;
    FontFaceBase->QueryInterface(&FontFace);

    DWRITE_FONT_METRICS1 FontMetrics = {};
    FontFace->GetMetrics(&FontMetrics);
    UINT16 GlyphCount = FontFace->GetGlyphCount();

    UINT16* GlyphIndices = new UINT16[GlyphCount];
    for (UINT16 i = 0; i < GlyphCount; i++)
    {
        GlyphIndices[i] = i;
    }

    DWRITE_GLYPH_METRICS* GlyphMetrics = new DWRITE_GLYPH_METRICS[GlyphCount];
    FontFace->GetDesignGlyphMetrics(GlyphIndices, GlyphCount, GlyphMetrics);

    constexpr UINT32 CharacterCount = 256;

    UINT16 GlyphIndex = 0;
    UINT32 Codepoints[CharacterCount];
    UINT16 GlyphMap[CharacterCount];
    for (UINT32 i = 0; i < CharacterCount; i++)
    {
        Codepoints[i] = i;
    }
    FontFace->GetGlyphIndices(Codepoints, CharacterCount, GlyphMap);

    // Gather the unique glyphs that we'll actually need for our character map
    UINT32 UniqueGlyphCount = 0;
    UINT16 UniqueGlyphIndices[CharacterCount];
    for (UINT32 i = 0; i < CharacterCount; i++)
    {
        UINT16 Glyph = GlyphMap[i];
        bool IsGlyphInList = false;
        UINT32 InsertIndex = UniqueGlyphCount;
        for (UINT32 SearchIndex = 0; SearchIndex < UniqueGlyphCount; SearchIndex++)
        {
            if (UniqueGlyphIndices[SearchIndex] == Glyph)
            {
                IsGlyphInList = true;
                break;
            }
            else if (UniqueGlyphIndices[SearchIndex] > Glyph)
            {
                InsertIndex = SearchIndex;
                break;
            }
        }

        if (!IsGlyphInList)
        {
            memmove(UniqueGlyphIndices + InsertIndex + 1, UniqueGlyphIndices + InsertIndex, (UniqueGlyphCount - InsertIndex) * sizeof(UINT16));
            UniqueGlyphIndices[InsertIndex] = Glyph;
            UniqueGlyphCount++;
        }
    }

    // Bind glyphs and codepoints together for the lbfnt file
    lbfnt_charmap CharMap[CharacterCount];
    for (UINT32 i = 0; i < CharacterCount; i++)
    {
        lbfnt_charmap* Char = CharMap + i;
        Char->Codepoint = Codepoints[i];
        Char->GlyphIndex = 0xFFFFFFFFu;
        UINT16 GlyphIndex = GlyphMap[i];
        for (UINT32 j = 0; j < UniqueGlyphCount; j++)
        {
            if (UniqueGlyphIndices[j] == GlyphMap[Codepoints[i]])
            {
                Char->GlyphIndex = j;
            }
        }
    }

    FLOAT RasterHeight = 32.0f;
    FLOAT UnitScale = 1.0f / (FLOAT)FontMetrics.designUnitsPerEm;

    FLOAT GlyphBoxLeft = UnitScale * FontMetrics.glyphBoxLeft;
    FLOAT GlyphBoxRight = UnitScale * FontMetrics.glyphBoxRight;
    FLOAT GlyphBoxTop = UnitScale * FontMetrics.glyphBoxTop;
    FLOAT GlyphBoxBottom = UnitScale * FontMetrics.glyphBoxBottom;

    FLOAT GlyphOffsetX = -GlyphBoxLeft;
    FLOAT GlyphOffsetY = -GlyphBoxBottom;
    FLOAT GlyphWidth = GlyphBoxRight - GlyphBoxLeft;
    FLOAT GlyphHeight = GlyphBoxTop - GlyphBoxBottom;

    lbfnt_font_info FontInfo = {};
    FontInfo.RasterHeight = 32.0f;
    FontInfo.Ascent = UnitScale * FontMetrics.ascent;
    FontInfo.Descent = UnitScale * FontMetrics.descent;
    FontInfo.BaselineDistance = FontInfo.Ascent + FontInfo.Descent + UnitScale * FontMetrics.lineGap; // FIXME
#if 0
    FontInfo.GlyphBoxLeft = GlyphBoxLeft;
    FontInfo.GlyphBoxRight = GlyphBoxRight;
    FontInfo.GlyphBoxTop = GlyphBoxTop;
    FontInfo.GlyphBoxBottom = GlyphBoxBottom;
    FontInfo.GlyphWidth = (GlyphBoxRight - GlyphBoxLeft);
    FontInfo.GlyphHeight = (GlyphBoxTop - GlyphBoxBottom);
#endif
    lbfnt_glyph* UniqueGlyphs = new lbfnt_glyph[UniqueGlyphCount];

    FLOAT CurrentX = 0;
    FLOAT CurrentY = RasterHeight * GlyphHeight;
    for (UINT32 i = 0; i < UniqueGlyphCount; i++)
    {
        UINT16 GlyphIndex = UniqueGlyphIndices[i];
        const DWRITE_GLYPH_METRICS* CurrentGlyphMetrics = GlyphMetrics + GlyphIndex;
        lbfnt_glyph* Glyph = UniqueGlyphs + i;
        Glyph->AdvanceX = UnitScale * CurrentGlyphMetrics->advanceWidth;
        Glyph->OriginY = UnitScale * CurrentGlyphMetrics->verticalOriginY;
#if 0
        Glyph->LeftBearing = UnitScale * CurrentGlyphMetrics->leftSideBearing;
        Glyph->RightBearing = UnitScale * CurrentGlyphMetrics->rightSideBearing;
        Glyph->TopBearing = UnitScale * CurrentGlyphMetrics->topSideBearing;
        Glyph->BottomBearing = UnitScale * CurrentGlyphMetrics->bottomSideBearing;
#endif

        FLOAT RunAdvance = 0.0f;
        DWRITE_GLYPH_OFFSET RunOffset = 
        {
            .advanceOffset = 0.0f,
            .ascenderOffset = 0.0f,
        };

        DWRITE_GLYPH_RUN GlyphRun = 
        {
            .fontFace = FontFace,
            .fontEmSize = RasterHeight,
            .glyphCount = 1,
            .glyphIndices = &GlyphIndex,
            .glyphAdvances = &RunAdvance,
            .glyphOffsets = &RunOffset,
            .isSideways = FALSE,
            .bidiLevel = 0,
        };

        ID2D1PathGeometry* PathGeometry = nullptr;
        D2DFactory->CreatePathGeometry(&PathGeometry);
        ID2D1GeometrySink* GeometrySink = nullptr;
        PathGeometry->Open(&GeometrySink);

        FontFace->GetGlyphRunOutline(RasterHeight, 
            GlyphRun.glyphIndices, GlyphRun.glyphAdvances, 
            GlyphRun.glyphOffsets, GlyphRun.glyphCount, 
            FALSE, FALSE, GeometrySink);
        GeometrySink->Close();

        // This transform should normalize the bounds to the em square
        D2D1_MATRIX_3X2_F Transform = 
        {
            1.0f / RasterHeight, 0.0f,
            0.0f, 1.0f / RasterHeight,
            0.0f, 0.0f,
        };

        D2D1_RECT_F GlyphBounds;
        PathGeometry->GetBounds(Transform, &GlyphBounds);

        Glyph->BoundsLeft = GlyphBounds.left;
        Glyph->BoundsTop = GlyphBounds.top;
        Glyph->BoundsRight = GlyphBounds.right;
        Glyph->BoundsBottom = GlyphBounds.bottom;

        PathGeometry->Release();

        RECT CharacterRect = {};
        RenderTarget->DrawGlyphRun(CurrentX, CurrentY, DWRITE_MEASURING_MODE_NATURAL, &GlyphRun, RenderingParams, RGB(0xFF, 0xFF,0xFF), &CharacterRect);
        
        Glyph->u0 = CharacterRect.left / (FLOAT)Width;
        Glyph->v0 = 1.0f - CharacterRect.top / (FLOAT)Height;
        Glyph->u1 = CharacterRect.right / (FLOAT)Width;
        Glyph->v1 = 1.0f - CharacterRect.bottom / (FLOAT)Height;

        CurrentX += RasterHeight * GlyphWidth;
        if (CurrentX + RasterHeight * GlyphWidth > (FLOAT)Width)
        {
            CurrentX = 0;
            CurrentY += RasterHeight * GlyphHeight;
        }
    }
    
    BitBlt(DeviceContext, 0, 0, Width, Height, RenderTarget->GetMemoryDC(), 0, 0, SRCCOPY);
    SetPixel(DeviceContext, 0, Height - 1, RGB(0xFF, 0xFF, 0xFF));

    size_t FileSize = sizeof(lbfnt_file) + (size_t)Width * (size_t)Height;
    lbfnt_file* FileData = (lbfnt_file*)malloc(FileSize);
    FileData->FileTag = LBFNT_FILE_TAG;
    FileData->Version = LBFNT_CURRENT_VERSION;
    FileData->FontInfo = FontInfo;
    memcpy(FileData->CharacterMap, CharMap, sizeof(CharMap));
    FileData->GlyphCount = UniqueGlyphCount;
    ZeroMemory(FileData->Glyphs, sizeof(FileData->Glyphs));
    memcpy(FileData->Glyphs, UniqueGlyphs, sizeof(lbfnt_glyph) * (size_t)UniqueGlyphCount);

    FileData->Bitmap.Width = (u32)Width;
    FileData->Bitmap.Height = (u32)Height;
    u8* Dest = FileData->Bitmap.Bitmap;
    for (int y = 0; y < Height; y++)
    {
        for (int x = 0; x < Width; x++)
        {
            u32 SrcPixel = ((u32*)SrcPixels)[x + y*Width];

            u32 R = SrcPixel & 0xFF; // NOTE(boti): R, G, B should be equal
            *Dest++ = (u8)R;
        }
    }

#if 1
    HANDLE File = CreateFileA("output.lbfnt", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File != INVALID_HANDLE_VALUE)
    {
        assert(FileSize <= 0xFFFFFFFFu);
        DWORD BytesWritten;
        if (WriteFile(File, FileData, (DWORD)FileSize, &BytesWritten, nullptr))
        {

        }
        else
        {
            fprintf(stderr, "Failed to write output file data\n");
        }

        CloseHandle(File);
    }
    else
    {
        fprintf(stderr, "Couldn't create output file\n");
    }
#else
    HANDLE File = CreateFileA("output.bmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File != INVALID_HANDLE_VALUE)
    {
        uint32_t ImageSize = (uint32_t)(Width * Height * BitsPerPixel / 8);
        uint32_t FileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + ImageSize;
        BITMAPFILEHEADER FileHeader = 
        {
            .bfType = 'MB',
            .bfSize = FileSize,
            .bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER),
        };

        BITMAPINFOHEADER InfoHeader = BitmapInfo.bmiHeader;
        //InfoHeader.biBitCount = 8;

        DWORD BytesWritten;
        if (WriteFile(File, &FileHeader, sizeof(FileHeader), &BytesWritten, nullptr))
        {
            if (WriteFile(File, &InfoHeader, sizeof(InfoHeader), &BytesWritten, nullptr))
            {
                if (WriteFile(File, SrcPixels, ImageSize, &BytesWritten, nullptr))
                {

                }
                else
                {
                    fprintf(stderr, "Couldn't write bitmap image data\n");
                }
            }
            else
            {
                fprintf(stderr, "Couldn't write bitmap info header\n");
            }
        }
        else
        {
            fprintf(stderr, "Couldn't write bitmap file header\n");
        }

        CloseHandle(File);
    }
    else
    {
        fprintf(stderr, "Couldn't create output file\n");
    }
#endif

#if USE_DEBUG_VIS
    DebugVis(
        &FileData->FontInfo,
        CharacterCount, FileData->CharacterMap,
        FileData->GlyphCount, FileData->Glyphs,
        FileData->Bitmap.Width, FileData->Bitmap.Height, FileData->Bitmap.Bitmap);
#endif
    return 0;
}

//
// Debug visualization
//

#pragma comment(lib, "opengl32.lib")

#include <gl/GL.h>

static LRESULT __stdcall DebugVisWndProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

static void DebugVis(
    const lbfnt_font_info* Font,
    u32 CharCount, const lbfnt_charmap* CharMap,
    u32 GlyphCount, const lbfnt_glyph* Glyphs,
    int Width, int Height, void* Pixels)
{
    static const char* WindowClassName = "lbfnt_wndclass";

    WNDCLASSA WindowClass = 
    {
        .style = CS_OWNDC,
        .lpfnWndProc = &DebugVisWndProc,
        .hCursor = LoadCursorA(nullptr, IDC_ARROW),
        .lpszClassName = WindowClassName,
    };

    if (!RegisterClassA(&WindowClass))
    {
        return;
    }

    DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
    RECT WindowRect = { 0, 0, Width, Height };
    AdjustWindowRect(&WindowRect, WindowStyle, FALSE);

    HWND Window = CreateWindowA(WindowClassName, "lbfnt - DebugVis", WindowStyle,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                WindowRect.right - WindowRect.left,
                                WindowRect.bottom - WindowRect.top,
                                nullptr, nullptr, nullptr, nullptr);
    
    ShowWindow(Window, SW_SHOW);

    HDC DeviceContext = GetDC(Window);

    constexpr GLenum GL_UPPER_LEFT = 0x8CA2;
    constexpr GLenum GL_ZERO_TO_ONE = 0x935F;
    typedef void APIENTRY FN_glClipControl(GLenum Origin, GLenum Depth);
    FN_glClipControl* glClipControl = nullptr;

    HGLRC RenderingContext = nullptr;
    {
        PIXELFORMATDESCRIPTOR PixelFormatDesc = 
        {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_DRAW_TO_WINDOW,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 32,
            .cAlphaBits = 8,
        };

        int PixelFormat = ChoosePixelFormat(DeviceContext, &PixelFormatDesc);
        DescribePixelFormat(DeviceContext, PixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &PixelFormatDesc);
        SetPixelFormat(DeviceContext, PixelFormat, &PixelFormatDesc);

        RenderingContext = wglCreateContext(DeviceContext);
        wglMakeCurrent(DeviceContext, RenderingContext);

        glClipControl = (FN_glClipControl*)wglGetProcAddress("glClipControl");
    }

    glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    constexpr GLenum GL_R8 = 0x8229;
    constexpr GLenum GL_TEXTURE_SWIZZLE_R = 0x8E42;
    constexpr GLenum GL_TEXTURE_SWIZZLE_G = 0x8E43;
    constexpr GLenum GL_TEXTURE_SWIZZLE_B = 0x8E44;
    constexpr GLenum GL_TEXTURE_SWIZZLE_A = 0x8E45;
    constexpr GLenum GL_TEXTURE_SWIZZLE_RGBA = 0x8E46;
    GLuint TextureName;
    glGenTextures(1, &TextureName);
    glBindTexture(GL_TEXTURE_2D, TextureName);
    // NOTE(boti): for some reason the alpha channel seems to be completely ignored when using GL_RED/GL_R8 as the internal format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RED, GL_UNSIGNED_BYTE, Pixels);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    GLint TextureSwizzle[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, TextureSwizzle);

    bool bDrawAtlas = false;

    bool IsRunning = true;
    while (IsRunning)
    {
        MSG Message = {};
        while (PeekMessageA(&Message, nullptr, 0, 0, PM_REMOVE))
        {
            if (Message.message == WM_QUIT)
            {
                IsRunning = false;
            }

            if (Message.message == WM_KEYUP && Message.wParam == 'A')
            {
                bDrawAtlas = !bDrawAtlas;
            }

            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        }

        glViewport(0, 0, Width, Height);
        glClearColor(0.5f, 0.5f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        GLfloat Matrix[] = 
        {
            2.0f / Width, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / Height, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 1.0f,
        };
        glLoadMatrixf(Matrix);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        if (!bDrawAtlas)
        {
            glDisable(GL_TEXTURE_2D);
            glBegin(GL_LINES);

            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(0.0f, 0.5f * Height);
            glVertex2f((f32)Width, 0.5f * Height);

            glEnd();

            glEnable(GL_TEXTURE_2D);

            glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 1.0f, 1.0f);

            const char* Text = "abcdefghijklmnopqrstuvwxyz 1234567890";
            //const char* Text = "12";
            f32 TextSize = 64.0f;
            f32 x = 0.0f;
            f32 y = 0.5f * Height;
            f32 LayoutHeight = Font->Ascent + Font->Descent;

            for (const char* Ch = Text; *Ch; Ch++)
            {
                const lbfnt_glyph* Glyph = Glyphs + CharMap[*Ch].GlyphIndex;

                f32 USize = (Glyph->u1 - Glyph->u0) * Width;
                f32 VSize = (Glyph->v1 - Glyph->v0) * Height;

#if 1
                f32 x0 = x + TextSize * Glyph->BoundsLeft;
                f32 y0 = y + TextSize * Glyph->BoundsTop;
                f32 x1 = x + TextSize * Glyph->BoundsRight;
                f32 y1 = y + TextSize * Glyph->BoundsBottom;
#else
                f32 x0 = x + TextSize * Glyph->LeftBearing;
                f32 y0 = y - TextSize * (Font->Ascent - Glyph->TopBearing);
                f32 x1 = x + TextSize * (Glyph->AdvanceX - Glyph->RightBearing);
                f32 y1 = y + TextSize * (Font->Descent - Glyph->BottomBearing);
#endif
                glTexCoord2f(Glyph->u0, Glyph->v0);
                glVertex2f(x0, y0);
                glTexCoord2f(Glyph->u1, Glyph->v0);
                glVertex2f(x1, y0);
                glTexCoord2f(Glyph->u1, Glyph->v1);
                glVertex2f(x1, y1);
                glTexCoord2f(Glyph->u0, Glyph->v0);
                glVertex2f(x0, y0);
                glTexCoord2f(Glyph->u1, Glyph->v1);
                glVertex2f(x1, y1);
                glTexCoord2f(Glyph->u0, Glyph->v1);
                glVertex2f(x0, y1);

                x += TextSize * Glyph->AdvanceX;
            }

            glEnd();
        }
        else
        {
            glEnable(GL_TEXTURE_2D);
            //glMatrixMode(GL_PROJECTION);
            //glLoadIdentity();
            glBegin(GL_TRIANGLES);

            glColor3f(1.0f, 1.0f, 1.0f);
#if 0
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(-1.0f, -1.0f);
            glTexCoord2f(2.0f, 1.0f);
            glVertex2f(+3.0f, -1.0f);
            glTexCoord2f(0.0f, -1.0f);
            glVertex2f(-1.0f, +3.0f);
#else
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f((f32)Width, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f((f32)Width, (f32)Height);
            
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f((f32)Width, (f32)Height);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(0.0f, (f32)Height);
#endif
            glEnd();
        }
        SwapBuffers(DeviceContext);
    }
    
}

static LRESULT __stdcall DebugVisWndProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}