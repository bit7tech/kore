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
//     consoleScreenUpdate(&scr);
//     consoleScreenResize(&scr, 50, 20, colour(EC_WHITE, EC_BLACK));
    consoleToggleFullScreen(&scr);

    // Draw something and change the cursor
    consoleScreenClear(&scr, colour(EC_WHITE, EC_BLACK));
    consoleScreenWrite(&scr, 1, 1, "Hello, World!");
    consoleScreenRect(&scr, 1, 1, 13, 1, colour(EC_LTYELLOW, EC_RED));
    scr.cursorX = 14;
    scr.cursorY = 1;
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

