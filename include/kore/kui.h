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
#define K_DESTROYED_HANDLE (-2)

typedef struct Window  
{
    int                 handle;         // Initialise this with K_CREATE_HANDLE on first pass to windowApply().
    String              title;
    Rect                bounds;
    bool                fullscreen;     // Set to YES for fullscreen
    bool                opengl;         // Set to YES to set up an OpenGL
    Size                imageSize;      // Image is stretched to window size.  If in OpenGL mode, this will determine the view.
    u32*                image;          // 2D bitmap of image in RGBA format.  Must be NULL if in OpenGL mode.

    bool                resizeable;     // YES if window can be resized.
    Size                sizeSnap;       // The size snap that allows the window to be resized to a grid.

    // Callbacks (set to 0 to do default behaviour).
    void                (*paintFunc) (const struct Window *);               // Used to paint the image.  If openGL, context is set and buffers are swapped afterwards.
    void                (*sizeFunc) (const struct Window *, int w, int h);  // This is called whenever the window is resized.
}
Window;

#define K_EVENT_NONE    (0)
#define K_EVENT_QUIT    (1)
#define K_EVENT_CLOSE   (2)
#define K_EVENT_SIZE    (3)
#define K_EVENT_KEY     (4)

typedef struct 
{
    int         type;           // One of the K_EVENT_??? defines.
    int         handle;         // Window handle creating the event.

    union {
        struct {
            int     key;        // The value of the key press (using K_KEY_xxx defines) or ASCII code.
            bool    down;       // YES if the key was pressed.
            bool    shift;      // YES if either shift key was pressed.
            bool    ctrl;       // YES if either ctrl key was pressed.
            bool    alt;        // YES if either alt key was pressed.
        } input;
    };
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

void windowInit(Window* window);            // Set a window to default parameters.
void windowApply(Window* window);           // Apply the parameters to a window or create a new one, and repaint.
void windowUpdate(Window* window);          // Update window structure to reflect current OS state of window
void windowRedraw(const Window* window);    // Redraw the window.
void windowDone(Window* window);            // Destroy window.
bool windowPoll(WindowEvent* event);        // Obtain events from the window.
void windowAddEvent(Window* window, const WindowEvent* event);  // Add an event to window event queue.
void windowAddGlobalEvent(const WindowEvent* event);            // Add an event not related to a window or referred
                                                                // window is destroyed.

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

#include <kore/kgl.h>

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
    i64         poolIndex;      // Used so that poolFor works.
    Window      window;         // Current state

#if K_OS_WIN32
    HWND        win32Handle;
    BITMAPINFO  bitmapInfo;
    HDC         dc;             // Device context for window
    HGLRC       openGL;         // OpenGL context
#endif

    Array(WindowEvent)  events;
    Rect        originalBounds;     // Used to store original bounds when fullscreen is activated.
}
WindowInfo;

Pool(WindowInfo) g_windows = 0;
int g_windowCount = 0;
Array(WindowEvent) g_globalEvents = 0;

//----------------------------------------------------------------------------------------------------------------------

internal WindowInfo* _windowGet(int handle)
{
    if (K_CREATE_HANDLE == handle)
    {
        // Allocate a new handle
        WindowInfo* info = poolAcquire(g_windows);
        info->window.handle = (int)poolIndexOf(g_windows, info);
        info->events = 0;
        ++g_windowCount;
        return info;
    }
    else if (K_DESTROYED_HANDLE == handle)
    {
        return 0;
    }
    else
    {
        return &g_windows[handle];
    }
}

//----------------------------------------------------------------------------------------------------------------------

internal void _windowDestroy(WindowInfo* info)
{
    arrayDone(info->events);
    poolRecycle(g_windows, poolIndexOf(g_windows, info));
    stringDone(&info->window.title);
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

internal RECT _windowCalcRect(Window* wnd, int style)
{
    RECT r = { 0, 0, wnd->bounds.size.cx, wnd->bounds.size.cy };
    AdjustWindowRect(&r, style, FALSE);
    r.right += -r.left + wnd->bounds.origin.x;
    r.bottom += -r.top + wnd->bounds.origin.y;
    r.left = wnd->bounds.origin.x;
    r.top = wnd->bounds.origin.y;

    return r;
}

internal void _windowFullScreen(WindowInfo* info)
{
    if (info->win32Handle)
    {
        SetWindowRgn(info->win32Handle, 0, FALSE);

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        info->originalBounds = info->window.bounds;

        DEVMODE mode;
        EnumDisplaySettings(0, 0, &mode);
        mode.dmBitsPerPel = 32;
        mode.dmPelsWidth = screenWidth;
        mode.dmPelsHeight = screenHeight;
        mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        long result = ChangeDisplaySettings(&mode, CDS_FULLSCREEN);

        if (result == DISP_CHANGE_SUCCESSFUL)
        {
            DWORD style = GetWindowLong(info->win32Handle, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_THICKFRAME);
            SetWindowLong(info->win32Handle, GWL_STYLE, style);

            // Move the window to 0, 0
            SetWindowPos(info->win32Handle, 0, 0, 0, screenWidth, screenHeight, SWP_NOZORDER);
            InvalidateRect(info->win32Handle, 0, TRUE);
        }
    }

    info->window.fullscreen = YES;
}

internal void _windowNoFullScreen(WindowInfo* info)
{
    ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
    info->window.fullscreen = NO;
    info->window.bounds = info->originalBounds;
    
    DWORD style = GetWindowLong(info->win32Handle, GWL_STYLE);
    style |= WS_CAPTION;
    if (info->window.resizeable) style |= WS_THICKFRAME;
    SetWindowLong(info->win32Handle, GWL_STYLE, style);

    RECT rc = _windowCalcRect(&info->window, style);
    
    SetWindowPos(info->win32Handle, 0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    InvalidateRect(info->win32Handle, 0, TRUE);
}

internal void _windowResizeImage(WindowInfo* wci, int width, int height)
{
    if (wci->window.image)
    {
        K_ZERO(wci->bitmapInfo.bmiHeader);
        wci->bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        wci->bitmapInfo.bmiHeader.biWidth = width;
        wci->bitmapInfo.bmiHeader.biHeight = -height;
        wci->bitmapInfo.bmiHeader.biPlanes = 1;
        wci->bitmapInfo.bmiHeader.biBitCount = 32;
        wci->bitmapInfo.bmiHeader.biClrImportant = BI_RGB;
        wci->window.imageSize.cx = width;
        wci->window.imageSize.cy = height;
    }
}

internal void renderOpenGL(HWND wnd, WindowInfo* info)
{
    if (info)
    {
        wglMakeCurrent(info->dc, info->openGL);

        if (info->window.paintFunc)
        {
            info->window.paintFunc(&info->window);
        }
        else
        {
            glClearColor(1, 0, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        SwapBuffers(info->dc);
    }
}

internal void resizeWindow(HWND wnd, WindowInfo* info, int newWidth, int newHeight)
{
    if (info)
    {
        info->window.bounds.size.cx = newWidth;
        info->window.bounds.size.cy = newHeight;

        if (info->window.fullscreen)
        {
            _windowFullScreen(info);
        }

        if (info->window.opengl && info->openGL)
        {
            glViewport(0, 0, newWidth, newHeight);
        }

        if (info->window.sizeFunc)
        {
            info->window.sizeFunc(&info->window, newWidth, newHeight);
        }
    }
}

internal LRESULT CALLBACK _windowProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    if (msg == WM_CREATE)
    {
        CREATESTRUCTA* cs = (CREATESTRUCTA *)l;
        WindowCreateInfo* wci = (WindowCreateInfo *)cs->lpCreateParams;
        WindowInfo* info = _windowGet(wci->handle);
        info->win32Handle = wnd;
        info->dc = 0;
        info->openGL = 0;
        SetWindowLongA(wnd, 0, (LONG)wci->handle);

        // Initialise the associated image
        _windowResizeImage(info, info->window.imageSize.cx, info->window.imageSize.cy);
    }
    else
    {
        int handle = (int)GetWindowLongA(wnd, 0);
        WindowInfo* info = &g_windows[handle];
        WindowEvent ev = { 0 };
        ev.handle = handle;

        switch (msg)
        {
        case WM_SIZE:
            resizeWindow(wnd, info, LOWORD(l), HIWORD(l));
            ev.type = K_EVENT_SIZE;
            windowAddEvent(&info->window, &ev);
            break;

        case WM_SIZING:
            {
                RECT* rc = (RECT *)l;
                resizeWindow(wnd, info, rc->right - rc->left, rc->bottom - rc->top);
                ev.type = K_EVENT_SIZE;
                windowAddEvent(&info->window, &ev);
            }
            break;

        case WM_MOVE:
            if (info)
            {
                int x = LOWORD(l);
                int y = HIWORD(l);
                RECT rc = { x, y, x, y };
                int style = GetWindowLong(wnd, GWL_STYLE);
                AdjustWindowRect(&rc, style, NO);

                info->window.bounds.origin.x = rc.left;
                info->window.bounds.origin.y = rc.top;
            }
            break;

        case WM_PAINT:
            if (info)
            {
                if (!info->openGL && info->window.image)
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
                else if (info->openGL)
                {
                    renderOpenGL(wnd, info);
                }
            }
            break;

        case WM_CLOSE:
            windowDone(&info->window);
            ev.type = K_EVENT_CLOSE;
            windowAddGlobalEvent(&ev);
            break;

        case WM_DESTROY:
            if (info->openGL)
            {
                wglDeleteContext(info->openGL);
                DeleteDC(info->dc);
                info->openGL = 0;
                info->dc = 0;
            }
            if (info->window.fullscreen) _windowNoFullScreen(info);
            info->window.handle = K_DESTROYED_HANDLE;
            break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
            ev.type = K_EVENT_KEY;
            ev.input.key = (int)w;
            ev.input.down = K_BOOL(msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            ev.input.shift = K_BOOL(HIBYTE(GetKeyState(VK_SHIFT)));
            ev.input.ctrl = K_BOOL(HIBYTE(GetKeyState(VK_CONTROL)));
            ev.input.alt = K_BOOL(HIBYTE(GetKeyState(VK_MENU)));
            windowAddEvent(&info->window, &ev);
            break;

        case WM_MENUCHAR:
            // This is handled to stop the beep that occurs when ALT+key is pressed.
            return MAKELRESULT(0, MNC_CLOSE);

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

    int style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    if (wnd->resizeable)
    {
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }
    RECT r = _windowCalcRect(wnd, style);

    WindowCreateInfo wci;
    wci.handle = wnd->handle;

    info->win32Handle = CreateWindowA("k_bitmap_window", wnd->title, style, r.left, r.top,
        r.right - r.left, r.bottom - r.top, 0, 0, GetModuleHandle(0), &wci);

    if (info->win32Handle && wnd->opengl)
    {
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),                                  // nSize
            1,                                                              // nVersion
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,     // dwFlags
            PFD_TYPE_RGBA,                                                  // iPixelType
            32,                                                             // cColorBits
            0, 0,                                                           // cRedBits, cRedShift
            0, 0,                                                           // cGreenBits, cGreenShift
            0, 0,                                                           // cBlueBits, cBlueShift
            0, 0,                                                           // cAlphaBits, cAlphaShift
            0,                                                              // cAccumBits
            0, 0, 0, 0,                                                     // cAccumRed/Green/Blue/AlphaBits
            24,                                                             // cDepthBits
            8,                                                              // cStencilBits
            0,                                                              // cAuxBuffers
            PFD_MAIN_PLANE,                                                 // iLayerType
            0,                                                              // bReserved
            0,                                                              // dwLayerMask
            0,                                                              // dwVisibleMask
            0,                                                              // dwDamageMask
        };

        info->dc = GetDC(info->win32Handle);
        int pixelFormat = ChoosePixelFormat(info->dc, &pfd);
        SetPixelFormat(info->dc, pixelFormat, &pfd);

        info->openGL = wglCreateContext(info->dc);
        wglMakeCurrent(info->dc, info->openGL);
        glInit();
    }

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
    window->fullscreen = NO;
    window->imageSize.cx = 0;
    window->imageSize.cy = 0;
    window->image = 0;
    window->resizeable = NO;
    window->sizeSnap.cx = 1;
    window->sizeSnap.cy = 1;
    window->paintFunc = 0;
    window->sizeFunc = 0;
}

//----------------------------------------------------------------------------------------------------------------------
// Redraw
//----------------------------------------------------------------------------------------------------------------------

void windowRedraw(const Window* window)
{
    if (window->handle != K_CREATE_HANDLE && window->handle != K_DESTROYED_HANDLE)
    {
        WindowInfo* info = _windowGet(window->handle);
        InvalidateRect(info->win32Handle, 0, 0);
    }
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

        // Check to see if we haven't destroyed this window.
        if (info)
        {
            //
            // Deal with fullscreen
            //
            if (window->fullscreen != info->window.fullscreen)
            {
                if (window->fullscreen) _windowFullScreen(info);
                else _windowNoFullScreen(info);
            }

            //
            // Deal with image changes
            //
            // #todo: Create a new image when changing from null to a pointer.
            if (info->window.image)
            {
                if ((info->window.imageSize.cx != window->imageSize.cx) ||
                    (info->window.imageSize.cy != window->imageSize.cy))
                {
                    _windowResizeImage(info, window->imageSize.cx, window->imageSize.cy);
                }
                info->window.image = window->image;
            }

            //
            // Update the callbacks
            //
            info->window.paintFunc = window->paintFunc;
            info->window.sizeFunc = window->sizeFunc;

            InvalidateRect(info->win32Handle, 0, 0);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------------------------------------------

void windowUpdate(Window* window)
{
    WindowInfo* info = _windowGet(window->handle);
    if (info)
    {
        *window = info->window;
    }
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
        if (info)
        {
            if (info->win32Handle != INVALID_HANDLE_VALUE)
            {
                SendMessageA(info->win32Handle, WM_DESTROY, 0, 0);
            }
        }
#endif
        window->handle = K_DESTROYED_HANDLE;
        if (info) _windowDestroy(info);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Poll
//----------------------------------------------------------------------------------------------------------------------

bool windowPoll(WindowEvent* event)
{
    event->type = K_EVENT_NONE;

    // Get the events from the OS - this will queue up events on windows or the global queue.
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
    }
#endif

    // Scan the known windows for any events.
    poolFor(g_windows) {
        WindowInfo* info = &g_windows[i];
        if (arrayCount(info->events) > 0)
        {
            *event = info->events[0];
            arrayDelete(info->events, 0);
            return YES;
        }
    }

    // Scan the global events.
    if (arrayCount(g_globalEvents) > 0)
    {
        *event = g_globalEvents[0];
        arrayDelete(g_globalEvents, 0);
        if (arrayCount(g_globalEvents) == 0)
        {
            arrayDone(g_globalEvents);
            g_globalEvents = 0;
        }
        return YES;
    }

    return NO;
}

//----------------------------------------------------------------------------------------------------------------------

void windowAddEvent(Window* window, const WindowEvent* event)
{
    K_ASSERT(window->handle != K_CREATE_HANDLE);
    WindowInfo* info = _windowGet(window->handle);
    *arrayExpand(info->events, 1) = *event;
}

//----------------------------------------------------------------------------------------------------------------------

void windowAddGlobalEvent(const WindowEvent* event)
{
    *arrayExpand(g_globalEvents, 1) = *event;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
