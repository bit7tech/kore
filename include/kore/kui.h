//----------------------------------------------------------------------------------------------------------------------
// C-based user interface library
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/kore.h>

//----------------------------------------------------------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------------------------------------------------------

typedef struct  
{
    int x;
    int y;
}
Point;

typedef struct  
{
    int cx;
    int cy;
}
Size;

typedef struct  
{
    Point origin;
    Size size;
}
Rect;

typedef struct  
{
    int         handle;
    String      title;
    Rect        bounds;
    Size        imageSize;      // Image is stretched to window size
    u32*        image;          // 2D bitmap of image in RGBA format
}
Window;

typedef struct 
{
    int         type;
}
WindowEvent;

//----------------------------------------------------------------------------------------------------------------------
// Window API
//----------------------------------------------------------------------------------------------------------------------

void windowApply(Window* window);
void windowUpdate(Window* window);
void windowDone(Window* window);
bool windowPoll(Window* window, WindowEvent* event);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Win32
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    Window      window;       // Current state

#if K_OS_WIN32
    HWND        win32Handle;
    BITMAPINFO  bitmapInfo;
#endif
}
WindowInfo;

Pool(WindowInfo) g_windows = 0;

internal WindowInfo* _windowGet(int handle)
{
    if (0 == handle)
    {
        // Allocate a new handle
        i64 i = poolIndexOf(g_windows, poolAcquire(g_windows));
        g_windows[i].window.handle = (int)i;
        return &g_windows[i];
    }
    else
    {
        return &g_windows[handle];
    }
}

internal void _windowDestroy(WindowInfo* info)
{
    poolRecycle(g_windows, poolIndexOf(g_windows, info));
}

//----------------------------------------------------------------------------------------------------------------------
// Win32 window creation
//----------------------------------------------------------------------------------------------------------------------

#if K_OS_WIN32
typedef struct
{
    int     handle;
}
WindowCreateInfo;

ATOM g_windowClassAtom = 0;

internal void _windowResizeImage(WindowInfo* wci, int width, int height)
{
    wci->bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    wci->bitmapInfo.bmiHeader.biWidth = width;
    wci->bitmapInfo.bmiHeader.biHeight = -height;
    wci->bitmapInfo.bmiHeader.biPlanes = 1;
    wci->bitmapInfo.bmiHeader.biBitCount = 32;
    wci->bitmapInfo.bmiHeader.biClrImportant = BI_RGB;
}

internal LRESULT CALLBACK _windowProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    if (msg == WM_CREATE)
    {
        CREATESTRUCTA* cs = (CREATESTRUCTA *)l;
        WindowCreateInfo* wci = (WindowCreateInfo *)cs->lpCreateParams;
        WindowInfo* info = _windowGet(wci->handle);
        info->win32Handle = wnd;
        SetWindowLongA(wnd, 0, (LONG)wci->handle);

        // Initialise the associated image
        _windowResizeImage(info, info->window.imageSize.cx, info->window.imageSize.cy);
    }
    else
    {
        WindowInfo* info = &g_windows[GetWindowLongA(wnd, 0)];

        switch (msg)
        {
        case WM_SIZE:
            if (info)
            {
                info->window.bounds.size.cx = LOWORD(l);
                info->window.bounds.size.cy = HIWORD(l);
            }
            break;

        case WM_PAINT:
            if (info)
            {
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(wnd, &ps);
                StretchDIBits(dc,
                    0, 0, info->window.bounds.size.cx, info->window.bounds.size.cy,
                    0, 0, info->window.imageSize.cx, info->window.imageSize.cy,
                    info->window.image, &info->bitmapInfo,
                    DIB_RGB_COLORS, SRCCOPY);
                EndPaint(wnd, &ps);
            }
            break;

        case WM_CLOSE:
            DestroyWindow(wnd);
            break;

        case WM_DESTROY:
            info->win32Handle = INVALID_HANDLE_VALUE;
            break;

        default:
            return DefWindowProcA(wnd, msg, w, l);
        }
    }

    return 0;
}

internal WindowInfo* _windowCreate(Window* wnd)
{
    WindowInfo* info = _windowGet(0);
    wnd->handle = info->window.handle;

    if (!g_windowClassAtom)
    {
        WNDCLASSEXA wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &_windowProc;
        wc.hInstance = GetModuleHandleA(0);
        wc.hIcon = wc.hIconSm = LoadIconA(0, IDI_APPLICATION);
        wc.hCursor = LoadCursorA(0, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = "k_bitmap_window";

        g_windowClassAtom = RegisterClassExA(&wc);
    }

    RECT r = { wnd->bounds.origin.x, wnd->bounds.origin.y, wnd->bounds.size.cx, wnd->bounds.size.cy };
    int style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    AdjustWindowRect(&r, style, FALSE);

    WindowCreateInfo wci;
    wci.handle = wnd->handle;

    info->win32Handle = CreateWindowA("k_bitmap_window", wnd->title, style, r.left, r.top,
        r.right - r.left, r.bottom - r.top, 0, 0, GetModuleHandle(0), &wci);

    return info;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
// Apply
//----------------------------------------------------------------------------------------------------------------------

void windowApply(Window* window)
{
    WindowInfo* info = 0;

    if (window->handle == 0)
    {
        // We need to create the window
        info = _windowCreate(window);
    }
    else
    {
        // TODO: Changes
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------------------------------------------

void windowUpdate(Window* window)
{
    // TODO: update window to current state
}

//----------------------------------------------------------------------------------------------------------------------
// Done
//----------------------------------------------------------------------------------------------------------------------

void windowDone(Window* window)
{
    if (window->handle)
    {
        WindowInfo* info = _windowGet(window->handle);
#if K_OS_WIN32
        SendMessageA(info->win32Handle, WM_DESTROY, 0, 0);
#endif
        window->handle = 0;
        _windowDestroy(info);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Poll
//----------------------------------------------------------------------------------------------------------------------

bool windowPoll(Window* window, WindowEvent* event)
{
    if (window->handle)
    {
        MSG msg;
        WindowInfo* info = _windowGet(window->handle);

#if K_OS_WIN32
        if (PeekMessageA(&msg, info->win32Handle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);

            // TODO: Initialise event

            return YES;
        }
#endif
    }

    return NO;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
