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
    int         m_handle;
    String      m_title;
    Rect        m_bounds;
}
Window;

//----------------------------------------------------------------------------------------------------------------------
// Window API
//----------------------------------------------------------------------------------------------------------------------

void windowApply(Window* window);
void windowUpdate(Window* window);
void windowDone(Window* window);

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
    Window      m_window;       // Current state
}
WindowInfo;

Pool(WindowInfo) g_windows = 0;

internal WindowInfo* windowGet(int handle)
{
    if (0 == handle)
    {
        // Allocate a new handle
        i64 i = poolIndexOf(g_windows, poolAcquire(g_windows));
        g_windows[i].m_window.m_handle = (int)i;
        return &g_windows[i];
    }
    else
    {
        return &g_windows[handle];
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
