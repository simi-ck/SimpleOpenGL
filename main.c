#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;      
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;

typedef float f32;

#include <Windows.h>
#include <gl/GL.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef HGLRC (wglCreateContextAttribsARBProc)(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (wglChoosePixelFormatARBProc)(HDC hDC, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
 
static wglChoosePixelFormatARBProc *wglChoosePixelFormatARB = NULL;
static wglCreateContextAttribsARBProc *wglCreateContextAttribsARB = NULL;


#define DUMMY_CLASS_NAME L"dummy_wnd_class"
#define REAL_CLASS_NAME L"real_wnd_class"

b32 should_choose = false;


static LRESULT window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CLOSE:
            should_choose = true;
        break;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

i32 pixel_format = 0;


int main() {
    HINSTANCE module_handle = GetModuleHandleW(NULL);
    //dummy window -> load extensions
    
    {
        WNDCLASSW wnd_class = {
            .lpszClassName = DUMMY_CLASS_NAME,
            .hInstance = module_handle,
            .lpfnWndProc = DefWindowProcW
        };
        RegisterClassW(&wnd_class);
        
        HWND dummy_wnd = CreateWindowW(
            DUMMY_CLASS_NAME, L"", WS_OVERLAPPEDWINDOW, 
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
            NULL, NULL, NULL, NULL);

        HDC dummy_dc = GetDC(dummy_wnd);

        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .cColorBits = 24
        };

        i32 pf = ChoosePixelFormat(dummy_dc, &pfd);
        DescribePixelFormat(dummy_dc, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        SetPixelFormat(dummy_dc, pf, &pfd);
        
        HGLRC dummy_gl_context = wglCreateContext(dummy_dc);
        wglMakeCurrent(dummy_dc, dummy_gl_context);

        wglCreateContextAttribsARB = (wglCreateContextAttribsARBProc *)wglGetProcAddress("wglCreateContextAttribsARB");
        wglChoosePixelFormatARB = (wglChoosePixelFormatARBProc *)wglGetProcAddress("wglChoosePixelFormatARB");
        
        i32 pf_attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_COLOR_BITS_ARB, 24,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            0
        };

        u32 num_formats = 0;
        wglChoosePixelFormatARB(dummy_dc, pf_attribs, NULL, 1, &pixel_format, &num_formats);

        
        // Load in opengl functions
        wglMakeCurrent(dummy_dc, NULL);
        wglDeleteContext(dummy_gl_context);
        ReleaseDC(dummy_wnd, dummy_dc);
        DestroyWindow(dummy_wnd);
        UnregisterClassW(DUMMY_CLASS_NAME, module_handle);
    }

    WNDCLASSW wnd_class = {
        .lpszClassName = REAL_CLASS_NAME,
        .hInstance = module_handle,
        .lpfnWndProc = DefWindowProcW,
        .hCursor = LoadCursorW(0, IDC_ARROW)
    };
    RegisterClassW(&wnd_class);
    
    HWND window = CreateWindowW(
        REAL_CLASS_NAME, L"WGL Window", WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 800, 
        NULL, NULL, NULL, NULL);
    
    HDC dc = GetDC(window);

    PIXELFORMATDESCRIPTOR pfd = {0};
    DescribePixelFormat(dc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    SetPixelFormat(dc, pixel_format, &pfd);

    i32 context_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        0
    };
    
    HGLRC gl_context = wglCreateContextAttribsARB(dc, 0, context_attribs);
    wglMakeCurrent(dc, gl_context);

    ShowWindow(window, SW_SHOW);

    while (!should_choose) {
        MSG msg = {0};
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        //Drawing
        SwapBuffers(dc);
    }

    wglMakeCurrent(dc, NULL);
    wglDeleteContext(gl_context); 
    ReleaseDC(window, dc);
    DestroyWindow(window);
    UnregisterClassW(REAL_CLASS_NAME, module_handle);

    return 0;
}