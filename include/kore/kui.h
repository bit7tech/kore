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

#define K_CREATE_HANDLE (-1)

typedef struct  
{
    int         handle;         // Initialise this with K_CREATE_HANDLE on first pass to windowApply().
    String      title;
    Rect        bounds;
    Size        imageSize;      // Image is stretched to window size
    u32*        image;          // 2D bitmap of image in RGBA format
    Size        sizeSnap;       // The size snap that allows the window to be resized to a grid.
}
Window;

#define K_EVENT_NONE    0
#define K_EVENT_QUIT    1

typedef struct 
{
    int         type;           // One of the K_EVENT_??? defines.
    int         handle;         // Window handle creating the event.
}
WindowEvent;

//----------------------------------------------------------------------------------------------------------------------
// Bounds API
//----------------------------------------------------------------------------------------------------------------------

Point pointMake(int x, int y);
Size sizeMake(int cx, int cy);
Rect rectMake(Point p, Size size);

//----------------------------------------------------------------------------------------------------------------------
// Window API
//----------------------------------------------------------------------------------------------------------------------

void windowInit(Window* window);        // Set a window to default parameters.
void windowApply(Window* window);       // Apply the parameters to a window or create a new one.
void windowUpdate(Window* window);      // Update window structure to reflect current OS state of window and repaint.
void windowDone(Window* window);        // Destroy window.
bool windowPoll(WindowEvent* event);    // Obtain events from the window.

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Bounds API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

Point pointMake(int x, int y)
{
    Point p = { x, y };
    return p;
}

Size sizeMake(int cx, int cy)
{
    Size s = { cx, cy };
    return s;
}

Rect rectMake(Point p, Size size)
{
    Rect r = { p, size };
    return r;
}

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
int g_windowCount = 0;
int g_createdWindowCount = 0;

internal WindowInfo* _windowGet(int handle)
{
    if (K_CREATE_HANDLE == handle)
    {
        // Allocate a new handle
        i64 i = poolAcquire(g_windows);
        g_windows[i].window.handle = (int)i;
        ++g_windowCount;
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
    if (--g_windowCount == 0)
    {
        poolDone(g_windows);
        PostQuitMessage(0);
    }
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
    K_ZERO(wci->bitmapInfo.bmiHeader);
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
        ++g_createdWindowCount;

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
            //info->win32Handle = INVALID_HANDLE_VALUE;
            info->window.handle = K_CREATE_HANDLE;
            if (0 == --g_createdWindowCount) PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(wnd, msg, w, l);
        }
    }

    return 0;
}

internal WindowInfo* _windowCreate(Window* wnd)
{
    WindowInfo* info = _windowGet(K_CREATE_HANDLE);
    wnd->handle = info->window.handle;
    info->window = *wnd;

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

    RECT r = { wnd->bounds.origin.x, wnd->bounds.origin.y,
        wnd->bounds.origin.x + wnd->bounds.size.cx,
        wnd->bounds.origin.y + wnd->bounds.size.cy };
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
// Init
//----------------------------------------------------------------------------------------------------------------------

void windowInit(Window* window)
{
    window->handle = K_CREATE_HANDLE;
    window->title = 0;
    window->bounds.origin.x = 10;
    window->bounds.origin.y = 10;
    window->bounds.size.cx = 800;
    window->bounds.size.cy = 600;
    window->imageSize.cx = 0;
    window->imageSize.cy = 0;
    window->image = 0;
    window->sizeSnap.cx = 1;
    window->sizeSnap.cy = 1;
}

//----------------------------------------------------------------------------------------------------------------------
// Apply
//----------------------------------------------------------------------------------------------------------------------

void windowApply(Window* window)
{
    WindowInfo* info = 0;

    if (window->handle == K_CREATE_HANDLE)
    {
        // We need to create the window
        info = _windowCreate(window);
    }
    else
    {
        // TODO: Changes
        WindowInfo* info = _windowGet(window->handle);
        InvalidateRect(info->win32Handle, 0, 0);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------------------------------------------

void windowUpdate(Window* window)
{
    WindowInfo* info = _windowGet(window->handle);
    *window = info->window;
}

//----------------------------------------------------------------------------------------------------------------------
// Done
//----------------------------------------------------------------------------------------------------------------------

void windowDone(Window* window)
{
    if (window->handle != K_CREATE_HANDLE)
    {
        WindowInfo* info = _windowGet(window->handle);
#if K_OS_WIN32
        if (info->win32Handle != INVALID_HANDLE_VALUE)
        {
            SendMessageA(info->win32Handle, WM_DESTROY, 0, 0);
        }
#endif
        window->handle = K_CREATE_HANDLE;
        _windowDestroy(info);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Poll
//----------------------------------------------------------------------------------------------------------------------

bool windowPoll(WindowEvent* event)
{
#if K_OS_WIN32
    MSG msg;
    if (PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE))
    {
        if (GetClassLongA(msg.hwnd, GCW_ATOM) == g_windowClassAtom)
        {
            event->handle = GetWindowLongA(msg.hwnd, 0);
        }
        if (!GetMessageA(&msg, 0, 0, 0))
        {
            event->type = K_EVENT_QUIT;
            return YES;
        };

        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        switch (msg.message)
        {
        default:
            event->type = K_EVENT_NONE;
            break;
        }

        return YES;
    }
#endif

    return NO;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
