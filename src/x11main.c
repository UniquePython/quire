#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// ======== CONFIGURATION CONSTANTS ========

// WINDOW CONFIG

#define WINDOW_X_POS 0
#define WINDOW_Y_POS 0

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 600

#define WINDOW_BORDER_WIDTH 1

#define WINDOW_BORDER_COLOR BlackPixel
#define WINDOW_BG_COLOR WhitePixel

// EVENT CONFIG

#define EVENT_TYPES (KeyPressMask | ExposureMask | StructureNotifyMask)

// ======== ENTRY POINT ========

int x11main(void)
{
    Display *display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "[ERROR] Unable to open X display.\n");
        fflush(stderr);
        return EXIT_FAILURE;
    }

    int screen = DefaultScreen(display);
    Window rootWindow = RootWindow(display, screen);

    Window window = XCreateSimpleWindow(
        display, rootWindow,
        WINDOW_X_POS, WINDOW_Y_POS,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        WINDOW_BORDER_WIDTH,
        WINDOW_BORDER_COLOR(display, screen),
        WINDOW_BG_COLOR(display, screen));

    XSelectInput(display, window, EVENT_TYPES);

    XMapWindow(display, window);

    Atom WMDelete = XInternAtom(display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(display, window, &WMDelete, 1);

    XEvent event;
    bool running = true;
    while (running)
    {
        XNextEvent(display, &event);

        switch (event.type)
        {
        case KeyPress:
        {
            KeySym key = XLookupKeysym(&event.xkey, 0);

            switch (key)
            {
            case XK_Escape:
                printf("[LOG] Escape Key pressed.\n");
                running = false;
                printf("[LOG] Closing...\n");
                fflush(stdout);
                break;

            case XK_Left:
                printf("[LOG] Left Key pressed.\n");
                fflush(stdout);
                break;

            case XK_Right:
                printf("[LOG] Right Key pressed.\n");
                fflush(stdout);
                break;
            }
            break;
        }

        case Expose:
            printf("[LOG] Expose event fired.\n");
            fflush(stdout);
            break;

        case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == WMDelete)
                running = false;
            break;
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return EXIT_SUCCESS;
}
