//----------------------------------------------------------------------------------------------------------------------
// C-based user interface library
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/kore.h>

//----------------------------------------------------------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------------------------------------------------------

#define K_CREATE_HANDLE (-1)
#define K_DESTROYED_HANDLE (-2)

typedef struct Window  
{
    int                 handle;         // Initialise this with K_CREATE_HANDLE on first pass to windowApply().
    String              title;
    Rect                bounds;
    bool                fullscreen;     // Set to YES for fullscreen
#if K_OPENGL
    bool                opengl;         // Set to YES to set up an OpenGL
#endif
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
#define K_EVENT_CHAR    (5)

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
        char ch;                // Character input.
    };
}
WindowEvent;

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

//----------------------------------------------------------------------------------------------------------------------
// Keyboard API
//----------------------------------------------------------------------------------------------------------------------

// Some convenience routines for checking keys in events
bool keyPressed(const WindowEvent* ev, int key);          // YES if a particular key was pressed with no shifts.
bool keyShiftPressed(const WindowEvent* ev, int key);     // YES if a particular key was pressed with just SHIFT.
bool keyCtrlPressed(const WindowEvent* ev, int key);      // YES if a particular key was pressed with just CTRL.
bool keyAltPressed(const WindowEvent* ev, int key);       // YES if a particular key was pressed with just ALT.

#define     KEY_BACKSPACE   0x08
#define     KEY_TAB         0x09
#define     KEY_ENTER       0x0d
#define     KEY_PAUSE       0x13
#define     KEY_ESCAPE      0x1b
#define     KEY_SPACE       0x20
#define     KEY_PAGEUP      0x21
#define     KEY_PAGEDOWN    0x22
#define     KEY_END         0x23
#define     KEY_HOME        0x24
#define     KEY_LEFT        0x25
#define     KEY_UP          0x26
#define     KEY_RIGHT       0x27
#define     KEY_DOWN        0x28
#define     KEY_PRTSC       0x2c
#define     KEY_INSERT      0x2d
#define     KEY_DELETE      0x2e
#define     KEY_0           0x30
#define     KEY_1           0x31
#define     KEY_2           0x32
#define     KEY_3           0x33
#define     KEY_4           0x34
#define     KEY_5           0x35
#define     KEY_6           0x36
#define     KEY_7           0x37
#define     KEY_8           0x38
#define     KEY_9           0x39
#define     KEY_A           0x41
#define     KEY_B           0x42
#define     KEY_C           0x43
#define     KEY_D           0x44
#define     KEY_E           0x45
#define     KEY_F           0x46
#define     KEY_G           0x47
#define     KEY_H           0x48
#define     KEY_I           0x49
#define     KEY_J           0x4a
#define     KEY_K           0x4b
#define     KEY_L           0x4c
#define     KEY_M           0x4d
#define     KEY_N           0x4e
#define     KEY_O           0x4f
#define     KEY_P           0x50
#define     KEY_Q           0x51
#define     KEY_R           0x52
#define     KEY_S           0x53
#define     KEY_T           0x54
#define     KEY_U           0x55
#define     KEY_V           0x56
#define     KEY_W           0x57
#define     KEY_X           0x58
#define     KEY_Y           0x59
#define     KEY_Z           0x5a
#define     KEY_NUM0        0x60
#define     KEY_NUM1        0x61
#define     KEY_NUM2        0x62
#define     KEY_NUM3        0x63
#define     KEY_NUM4        0x64
#define     KEY_NUM5        0x65
#define     KEY_NUM6        0x66
#define     KEY_NUM7        0x67
#define     KEY_NUM8        0x68
#define     KEY_NUM9        0x69
#define     KEY_MULTIPLY    0x6a
#define     KEY_ADD         0x6b
#define     KEY_SUBTRACT    0x6d
#define     KEY_POINT       0x6e
#define     KEY_DIVIDE      0x6f
#define     KEY_F1          0x70
#define     KEY_F2          0x71
#define     KEY_F3          0x72
#define     KEY_F4          0x73
#define     KEY_F5          0x74
#define     KEY_F6          0x75
#define     KEY_F7          0x76
#define     KEY_F8          0x77
#define     KEY_F9          0x78
#define     KEY_F10         0x79
#define     KEY_F11         0x7a
#define     KEY_F12         0x7b
#define     KEY_F13         0x7c
#define     KEY_F14         0x7d
#define     KEY_F15         0x7e
#define     KEY_F16         0x7f
#define     KEY_F17         0x80
#define     KEY_F18         0x81
#define     KEY_F19         0x82
#define     KEY_F20         0x83
#define     KEY_F21         0x84
#define     KEY_F22         0x85
#define     KEY_F23         0x86
#define     KEY_F24         0x87
#define     KEY_NUMLOCK     0x90
#define     KEY_SCROLLLOCK  0x91

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

#if K_OPENGL
#   include <kore/kgl.h>
#endif

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
#if K_OPENGL
    HDC         dc;             // Device context for window
    HGLRC       openGL;         // OpenGL context
#endif
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
    RECT r = { 0, 0, wnd->bounds.w, wnd->bounds.h };
    AdjustWindowRect(&r, style, FALSE);
    r.right += -r.left + wnd->bounds.x;
    r.bottom += -r.top + wnd->bounds.y;
    r.left = wnd->bounds.x;
    r.top = wnd->bounds.y;

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
        wci->window.imageSize.w = width;
        wci->window.imageSize.h = height;
    }
}

#if K_OPENGL
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
#endif

internal void resizeWindow(HWND wnd, WindowInfo* info, int newWidth, int newHeight)
{
    if (info)
    {
        info->window.bounds.w = newWidth;
        info->window.bounds.h = newHeight;

        if (info->window.fullscreen)
        {
            _windowFullScreen(info);
        }

#if K_OPENGL
        if (info->window.opengl && info->openGL)
        {
            glViewport(0, 0, newWidth, newHeight);
        }
#endif

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
#if K_OPENGL
        info->dc = 0;
        info->openGL = 0;
#endif
        SetWindowLongA(wnd, 0, (LONG)wci->handle);

        // Initialise the associated image
        _windowResizeImage(info, info->window.imageSize.w, info->window.imageSize.h);
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

                info->window.bounds.x = rc.left;
                info->window.bounds.y = rc.top;
            }
            break;

        case WM_PAINT:
            if (info)
            {
#if K_OPENGL
                if (info->openGL)
                {
                    renderOpenGL(wnd, info);
                } else
#endif
                if (info->window.image)
                {
                    PAINTSTRUCT ps;
                    HDC dc = BeginPaint(wnd, &ps);
                    StretchDIBits(dc,
                        0, 0, info->window.bounds.w, info->window.bounds.h,
                        0, 0, info->window.imageSize.w, info->window.imageSize.h,
                        info->window.image, &info->bitmapInfo,
                        DIB_RGB_COLORS, SRCCOPY);
                    EndPaint(wnd, &ps);
                }
            }
            break;

        case WM_CLOSE:
            windowDone(&info->window);
            ev.type = K_EVENT_CLOSE;
            windowAddGlobalEvent(&ev);
            break;

        case WM_DESTROY:
#if K_OPENGL
            if (info->openGL)
            {
                wglDeleteContext(info->openGL);
                DeleteDC(info->dc);
                info->openGL = 0;
                info->dc = 0;
            }
#endif
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

        case WM_CHAR:
            if ((l & 0xa000) == 0)
            {
                ev.type = K_EVENT_CHAR;
                ev.ch = (char)w;
                windowAddEvent(&info->window, &ev);
            }
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

#if K_OPENGL
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
#endif // K_OPENGL

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
    window->bounds.x = 10;
    window->bounds.y = 10;
    window->bounds.w = 800;
    window->bounds.h = 600;
    window->fullscreen = NO;
    window->imageSize.w = 0;
    window->imageSize.h = 0;
    window->image = 0;
    window->resizeable = NO;
    window->sizeSnap.w = 1;
    window->sizeSnap.h = 1;
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
            // Deal with size changes
            //
            if (window->bounds.x != info->window.bounds.x ||
                window->bounds.y != info->window.bounds.y ||
                window->bounds.w != info->window.bounds.w ||
                window->bounds.h != info->window.bounds.h)
            {
                RECT rc = _windowCalcRect(window, GetWindowLongA(info->win32Handle, GWL_STYLE));
                SetWindowPos(info->win32Handle, 0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
                    SWP_NOACTIVATE | SWP_NOZORDER);

                info->window.bounds.x = window->bounds.x;
                info->window.bounds.y = window->bounds.y;
                info->window.bounds.w = window->bounds.w;
                info->window.bounds.h = window->bounds.h;
            }

            //
            // Deal with image changes
            //
            // #todo: Create a new image when changing from null to a pointer.
            if (info->window.image)
            {
                if ((info->window.imageSize.w != window->imageSize.w) ||
                    (info->window.imageSize.h != window->imageSize.h))
                {
                    _windowResizeImage(info, window->imageSize.w, window->imageSize.h);
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

bool keyPressed(const WindowEvent* ev, int key)
{
    return ev->input.down && !ev->input.shift && !ev->input.ctrl && !ev->input.alt && ev->input.key == key;
}

//----------------------------------------------------------------------------------------------------------------------

bool keyShiftPressed(const WindowEvent* ev, int key)
{
    return ev->input.down && ev->input.shift && !ev->input.ctrl && !ev->input.alt && ev->input.key == key;
}

//----------------------------------------------------------------------------------------------------------------------

bool keyCtrlPressed(const WindowEvent* ev, int key)
{
    return ev->input.down && !ev->input.shift && ev->input.ctrl && !ev->input.alt && ev->input.key == key;
}

//----------------------------------------------------------------------------------------------------------------------

bool keyAltPressed(const WindowEvent* ev, int key)
{
    return ev->input.down && !ev->input.shift && !ev->input.ctrl && ev->input.alt && ev->input.key == key;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
