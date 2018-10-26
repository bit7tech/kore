//----------------------------------------------------------------------------------------------------------------------
// Konsole C-based API
// Copyright (C)2018 Matt Davies, all rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/kore.h>

//----------------------------------------------------------------------------------------------------------------------
// Basic control
//----------------------------------------------------------------------------------------------------------------------

// Create a console window.
void consoleOpen();

// Wait for a key press.
void consolePause();

// Enable ANSI escape codes.
void consoleEnableANSIColours();

//----------------------------------------------------------------------------------------------------------------------
// Screen colours
//----------------------------------------------------------------------------------------------------------------------

typedef enum
{
    EC_BLACK,
    EC_BLUE,
    EC_GREEN,
    EC_CYAN,
    EC_RED,
    EC_MAGENTA,
    EC_YELLOW,
    EC_LTGREY,
    EC_DKGREY,
    EC_LTBLUE,
    EC_LTGREEN,
    EC_LTCYAN,
    EC_LTRED,
    EC_LTMAGENTA,
    EC_LTYELLOW,
    EC_WHITE,
}
Colour;

inline u8 colour(Colour ink, Colour paper) { return paper << 4 | ink; }

//----------------------------------------------------------------------------------------------------------------------
// Screen buffer
//----------------------------------------------------------------------------------------------------------------------

// The character supported by the text API.
typedef char kchar;

typedef struct _Screen
{
    int     width;
    int     height;
    kchar*  text;
    u8*     attr;
    int     cursorX;
    int     cursorY;
    String  title;
}
Screen, *ScreenRef;

// Set the screen structure to the current status of the console.  Set everything to 0 for initialisation.
void consoleScreenUpdate(ScreenRef screen);

// Apply the screen structure to the console.
void consoleScreenApply(ScreenRef screen);

// Set a new size for the screen structure.  Will truncate or wipe accordingly.
void consoleScreenResize(ScreenRef screen, int newWidth, int newHeight, u8 expandColour);

// Toggle fullscreen.  Will also update the Screen structure.
void consoleToggleFullScreen(ScreenRef screen);

//----------------------------------------------------------------------------------------------------------------------
// Screen drawing
//----------------------------------------------------------------------------------------------------------------------

void consoleScreenClear(ScreenRef scr, u8 colour);
void consoleScreenWrite(ScreenRef scr, int x, int y, const kchar* str);
void consoleScreenWriteRange(ScreenRef scr, int x, int y, const char* start, const kchar* end);
void consoleScreenWriteChar(ScreenRef scr, int x, int y, kchar c);
void consoleScreenRect(ScreenRef scr, int x, int y, int width, int height, u8 colour);

//----------------------------------------------------------------------------------------------------------------------
// Input
//----------------------------------------------------------------------------------------------------------------------

#if K_OS_WIN32
// Allocate a buffer and set variables pointed to outLineRef and outNumRef to receive input of a single line from
// a file (which can be stdin).  Returns the number of characters read or -1 if EOF.
i64 getLine(char** outLineRef, i64* outNumRef, FILE* stream);
#endif

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// I M P L E M E N T A T I O N
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// C O N S O L E   C O N T R O L 
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

void consoleOpen()
{
#if K_OS_WIN32
    AllocConsole();
    SetConsoleTitleA("Debug Window");

    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((intptr_t)handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    freopen("CONOUT$", "w", stdout);

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((intptr_t)handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 0);
    freopen("CONIN$", "r", stdin);
#endif

    consoleEnableANSIColours();
}

void consolePause()
{
    printf("\n\033[33;1mPress any key...\033[0m\n\n");
    _getch();
}

void consoleEnableANSIColours()
{
#if K_OS_WIN32
    DWORD mode;
    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(handle_out, &mode);
    mode |= 0x4;
    SetConsoleMode(handle_out, mode);
#endif
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// S C R E E N   C O N T R O L
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

HANDLE gConsole = 0;

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenResize(ScreenRef screen, int newWidth, int newHeight, u8 expandColour)
{
    i64 newSize = newWidth * newHeight;
    if (screen->text)
    {
        i64 oldSize = screen->width * screen->height;
        kchar* newText = K_ALLOC(newSize * sizeof(kchar));
        u8* newAttr = K_ALLOC(newSize * sizeof(u8));

        i64 row;
        kchar* ot = screen->text;
        u8* oa = screen->attr;
        kchar* nt = newText;
        u8* na = newAttr;
        for (row = 0; row < K_MIN(screen->height, newHeight); ++row)
        {
            i64 col;
            for (col = 0; col < K_MIN(screen->width, newWidth); ++col)
            {
                *nt++ = *ot++;
                *na++ = *oa++;
            }
            if (col < newWidth)
            {
                // Still some more columns to render
                *nt++ = ' ';
                *na++ = expandColour;
            }
            else
            {
                // Width shrunk, jump to the next row of the original data.
                ot += (screen->width - newWidth);
                oa += (screen->width - newWidth);
            }
        }
        if (row < newHeight)
        {
            // More rows to render.
            for (; row < newHeight; ++row)
            {
                for (i64 col = 0; col < newWidth; ++col)
                {
                    *nt++ = ' ';
                    *na++ = expandColour;
                }
            }
        }
        K_FREE(screen->text, oldSize * sizeof(kchar));
        K_FREE(screen->attr, oldSize * sizeof(u8));
        screen->text = newText;
        screen->attr = newAttr;
        screen->width = newWidth;
        screen->height = newHeight;
    }
    else
    {
        screen->width = newWidth;
        screen->height = newHeight;
        screen->text = (kchar *)K_ALLOC(newSize * sizeof(kchar));
        screen->attr = (u8 *)K_ALLOC(newSize * sizeof(u8));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void consoleToggleFullScreen(ScreenRef screen)
{
    SendMessageA(GetConsoleWindow(), WM_SYSKEYDOWN, VK_RETURN, 0x20000000);
    consoleScreenUpdate(screen);
}

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenUpdate(ScreenRef screen)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(gConsole, &info);
    int currentWidth = info.srWindow.Right - info.srWindow.Left;
    int currentHeight = info.srWindow.Bottom - info.srWindow.Top;
    int size = currentWidth * currentHeight;

    if (screen->text == 0 || screen->width != currentWidth || screen->height != currentHeight)
    {
        consoleScreenResize(screen, currentWidth, currentHeight, colour(EC_LTGREY, EC_BLACK));
    }

    CHAR_INFO* ci = K_ALLOC(size * sizeof(CHAR_INFO));
    COORD bufferSize = { currentWidth, currentHeight };
    COORD bufferXY = { 0, 0 };
    
    ReadConsoleOutput(gConsole, ci, bufferSize, bufferXY, &info.srWindow);

    i64 i = 0;
    for (i64 row = 0; row < currentHeight; ++row)
    {
        for (i64 col = 0; col < currentWidth; ++col)
        {
            screen->text[i] = (kchar)ci[i].Char.AsciiChar;
            screen->attr[i] = (u8)ci[i].Attributes;
            ++i;
        }
    }
    K_FREE(ci, size * sizeof(CHAR_INFO));

    screen->cursorX = (int)info.dwCursorPosition.X;
    screen->cursorY = (int)info.dwCursorPosition.Y;
}

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenApply(ScreenRef scr)
{
    CONSOLE_SCREEN_BUFFER_INFOEX info = { 0 };
    info.cbSize = sizeof(info);
    GetConsoleScreenBufferInfoEx(gConsole, &info);
    int currentWidth = info.srWindow.Right - info.srWindow.Left;
    int currentHeight = info.srWindow.Bottom - info.srWindow.Top;

    if (currentWidth != scr->width || currentHeight != scr->height)
    {
        COORD sz = { scr->width, scr->height };

        info.dwSize.X = scr->width;
        info.dwSize.Y = scr->height;
        info.dwCursorPosition.X = scr->cursorX;
        info.dwCursorPosition.Y = scr->cursorY;
        info.srWindow.Left = 0;
        info.srWindow.Top = 0;
        info.srWindow.Right = scr->width;
        info.srWindow.Bottom = scr->height;
        SetConsoleScreenBufferInfoEx(gConsole, &info);

        //SetConsoleScreenBufferSize(gConsole, sz);
        currentWidth = scr->width;
        currentHeight = scr->height;
    }

    COORD cursorPos = { scr->cursorX, scr->cursorY };
    SetConsoleCursorPosition(gConsole, cursorPos);
    SetConsoleTitle(scr->title);

    K_ASSERT(currentWidth == scr->width, "No resizing yet.");
    K_ASSERT(currentHeight == scr->height, "No resizing yet.");

    int size = currentWidth * currentHeight;
    CHAR_INFO* data = K_ALLOC(size * sizeof(CHAR_INFO));
    for (int i = 0; i < size; ++i)
    {
        data[i].Char.UnicodeChar = (WCHAR)scr->text[i];
        data[i].Attributes = (DWORD)scr->attr[i];
    }

    COORD bufferSize = { currentWidth, currentHeight };
    COORD bufferCoord = { 0, 0 };
    SMALL_RECT writeRegion = {
        info.srWindow.Left,
        info.srWindow.Top,
        info.srWindow.Left + currentWidth,
        info.srWindow.Top + currentHeight
    };
    WriteConsoleOutput(gConsole, data, bufferSize, bufferCoord, &writeRegion);
    K_FREE(data, size * sizeof(CHAR_INFO));
}

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenDone(ScreenRef scr)
{
    i64 size = scr->width * scr->height;
    K_FREE(scr->text, size * sizeof(kchar));
    K_FREE(scr->attr, size * sizeof(u8));
    scr->text = 0;
    scr->attr = 0;
    scr->width = 0;
    scr->height = 0;
    stringDone(&scr->title);
}

//----------------------------------------------------------------------------------------------------------------------

void consoleSave()
{
    gConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, 0, CONSOLE_TEXTMODE_BUFFER, 0);
    SetConsoleActiveScreenBuffer(gConsole);
}

//----------------------------------------------------------------------------------------------------------------------

internal void consoleRestore()
{
    SetConsoleActiveScreenBuffer(0);
    CloseHandle(gConsole);
    gConsole = 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// S C R E E N   O U T P U T
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

void consoleScreenClear(ScreenRef scr, u8 colour)
{
    int count = scr->width * scr->height;
    for (int i = 0; i < count; ++i)
    {
        scr->text[i] = ' ';
        scr->attr[i] = colour;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenWrite(ScreenRef scr, int x, int y, const kchar* str)
{
    int len = (int)strlen(str);
    consoleScreenWriteRange(scr, x, y, str, str + len);
}

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenWriteRange(ScreenRef scr, int x, int y, const kchar* start, const kchar* end)
{
    if (start && (end > start) && (x >= 0) && (y >= 0) && (x < scr->width) && (y < scr->height))
    {
        i64 len = end - start;
        len = K_MIN(scr->width - x, len);
        kchar* p = &scr->text[y * scr->width + x];
        for (int i = 0; i < len; ++i, ++start) *p++ = *start > ' ' && *start < 128 ? *start : ' ';
    }
}

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenWriteChar(ScreenRef scr, int x, int y, kchar c)
{
    scr->text[y * scr->width + x] = c;
}

//----------------------------------------------------------------------------------------------------------------------

void consoleScreenRect(ScreenRef scr, int x, int y, int width, int height, u8 colour)
{
    int x0 = (x < 0) ? (width += x, 0) : x;
    int y0 = (y < 0) ? (height += y, 0) : y;
    int x1 = K_MIN(x0 + width, scr->width);
    int y1 = K_MIN(y0 + height, scr->height);

    u8* p = &scr->attr[y0 * scr->width + x0];
    for (int row = y0; row < y1; ++row)
    {
        for (int col = x0; col < x1; ++col)
        {
            *p++ = colour;
        }
        p += (scr->width - x1 + x0);
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// I N P U T
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#if K_OS_WIN32
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#ifndef SIZE_MAX
# define SIZE_MAX ((i64)0x7fffffffffffffffll)
#endif
#ifndef SSIZE_MAX
# define SSIZE_MAX ((i64)0x3fffffffffffffffll)
#endif
#if !HAVE_FLOCKFILE
# undef flockfile
# define flockfile(x) ((void) 0)
#endif
#if !HAVE_FUNLOCKFILE
# undef funlockfile
# define funlockfile(x) ((void) 0)
#endif

i64 getDelim(char **outLineRef, i64 *outNumRef, int delimiter, FILE *fp)
{
    i64 result;
    i64 cur_len = 0;

    if (outLineRef == NULL || outNumRef == NULL || fp == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    flockfile(fp);

    if (*outLineRef == NULL || *outNumRef == 0)
    {
        *outNumRef = 120;
        *outLineRef = (char *)K_ALLOC(*outNumRef);
        if (*outLineRef == NULL)
        {
            result = -1;
            goto unlock_return;
        }
    }

    for (;;)
    {
        int i;

        i = getc(fp);
        if (i == EOF)
        {
            result = -1;
            break;
        }

        /* Make enough space for len+1 (for final NUL) bytes.  */
        if (cur_len + 1 >= *outNumRef)
        {
            i64 needed_max =
                SSIZE_MAX < SIZE_MAX ? (i64)SSIZE_MAX + 1 : SIZE_MAX;
            i64 needed = 2 * *outNumRef + 1;   /* Be generous. */
            char *new_lineptr;

            if (needed_max < needed)
                needed = needed_max;
            if (cur_len + 1 >= needed)
            {
                result = -1;
                goto unlock_return;
            }

            new_lineptr = (char *)K_REALLOC(*outLineRef, *outNumRef, needed);
            if (new_lineptr == NULL)
            {
                result = -1;
                goto unlock_return;
            }

            *outLineRef = new_lineptr;
            *outNumRef = needed;
        }

        (*outLineRef)[cur_len] = i;
        cur_len++;

        if (i == delimiter)
            break;
    }
    (*outLineRef)[cur_len] = '\0';
    result = cur_len ? cur_len : result;

unlock_return:
    funlockfile(fp);
    return result;
}

i64 getLine(char** outLineRef, i64* outNumRef, FILE* stream)
{
    return getDelim(outLineRef, outNumRef, '\n', stream);
}

#endif

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION

