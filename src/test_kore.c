//----------------------------------------------------------------------------------------------------------------------
// Kore testing
//----------------------------------------------------------------------------------------------------------------------

#define K_IMPLEMENTATION
#include <kore/kore.h>
#include <kore/kgl.h>
#include <kore/kui.h>
#include <kore/parser.h>


//----------------------------------------------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------------------------------------------

int kmain(int argc, char** argv)
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

    return 0;
}

