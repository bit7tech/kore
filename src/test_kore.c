//----------------------------------------------------------------------------------------------------------------------
// Kore testing
//----------------------------------------------------------------------------------------------------------------------

#define K_IMPLEMENTATION
#include <kore/kore.h>
#include <kore/konsole.h>
#include <kore/kgl.h>
#include <kore/kui.h>
#include <kore/parser.h>

//----------------------------------------------------------------------------------------------------------------------
// Window test
//----------------------------------------------------------------------------------------------------------------------

void testWindow()
{
    Window wnd1 = {
        K_CREATE_HANDLE,
        "Test Window 1",                // title
        { { 50, 50 }, { 800, 600 } }    // bounds
    };
    Window wnd2 = {
        K_CREATE_HANDLE,
        "Test Window 2",                // title
        { { 200, 200 },{ 800, 600 } }   // bounds
    };

    consoleOpen();

    windowApply(&wnd1);
    windowApply(&wnd2);
    WindowEvent ev;

    for(;;)
    {
        while (windowPoll(&ev))
        {
            if (ev.type == K_EVENT_QUIT) goto quit;
        }
    }
    quit:

    windowUpdate(&wnd1);
    windowDone(&wnd1);
    windowDone(&wnd2);

    printf("Final window width = %d\n", wnd1.bounds.size.cx);
}

//----------------------------------------------------------------------------------------------------------------------
// Console test
//----------------------------------------------------------------------------------------------------------------------

void testConsole()
{
    consoleOpen();

    char* line = 0;
    i64 size = 0;

    printf("Input: ");
    i64 num = getLine(&line, &size, stdin);
    K_FREE(line, size);
}

void testFullConsole()
{
    consoleOpen();
    consoleSave();

    // Set up the console
    Screen scr = { 0 };
    scr.title = stringMake("Konsole Demo");
    consoleScreenUpdate(&scr);
    consoleScreenResize(&scr, 50, 20, colour(EC_WHITE, EC_BLACK));

    // Draw something and change the cursor
    consoleScreenClear(&scr, colour(EC_WHITE, EC_BLACK));

    for (int i = 0; i < 16; ++i)
    {
        for (int j = 0; j < 16; ++j)
        {
            consoleScreenWriteChar(&scr, j + 2, i + 2, i * 16 + j);
        }
    }
    static kchar hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i)
    {
        consoleScreenWriteChar(&scr, i + 2, 1, hex[i]);
        consoleScreenWriteChar(&scr, 1, i + 2, hex[i]);
    }
    consoleScreenRect(&scr, 2, 1, 16, 1, colour(EC_LTYELLOW, EC_BLACK));
    consoleScreenRect(&scr, 1, 2, 1, 16, colour(EC_LTRED, EC_BLACK));
    scr.cursorX = 0;
    scr.cursorY = 0;
    consoleScreenApply(&scr);

    // Clean up
    consoleScreenDone(&scr);
    consolePause();
    consoleRestore();
}

//----------------------------------------------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------------------------------------------

int kmain(int argc, char** argv)
{
    debugBreakOnAlloc(0);
    //testWindow();
    //testConsole();
    testFullConsole();
    return 0;
}

