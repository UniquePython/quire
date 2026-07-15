#include "quire_platform.h"

#include "quire_log.h"

#include <stdlib.h>
#include <inttypes.h>

#include <X11/Xlib.h>

struct QuirePlatform
{
    Display *display;
    Window window;
    i32 screen;
    Atom WMDelete;
    u32 width, height;
    QuirePixelFormat pixelFormat;
    XImage *image;
};

bool QuirePlatformCreate(
    QuirePlatform **restrict platform,
    u32 width,
    u32 height,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
{
    *platform = calloc(1, sizeof(**platform));
    if (*platform == NULL)
    {
        QuireSetError(errorBuffer, "Failed to allocate QuirePlatform");
        return false;
    }

    (*platform)->display = XOpenDisplay(NULL);
    if ((*platform)->display == NULL)
    {
        QuirePlatformDestroy(platform);
        QuireSetError(errorBuffer, "Failed to open X Display");
        return false;
    }
    LOG_DEBUG("Opened X Display");

    (*platform)->screen = (i32)DefaultScreen((*platform)->display);
    LOG_DEBUG("Using default screen %" PRIi32, (*platform)->screen);

    Window rootWindow = RootWindow((*platform)->display, (*platform)->screen);
    LOG_DEBUG("Using root window: %" PRIu64, (u64)rootWindow);

    (*platform)->window = XCreateSimpleWindow(
        (*platform)->display,
        rootWindow,
        0,
        0,
        (unsigned int)width,
        (unsigned int)height,
        1,
        BlackPixel((*platform)->display, (*platform)->screen),
        WhitePixel((*platform)->display, (*platform)->screen));

    if ((*platform)->window == 0)
    {
        QuirePlatformDestroy(platform);
        QuireSetError(errorBuffer, "Failed to create Window");
        return false;
    }
    LOG_DEBUG("Created window (%" PRIu32 "x%" PRIu32 ")", width, height);

    static const long EVENT_TYPES = KeyPressMask | ExposureMask | StructureNotifyMask;

    XSelectInput((*platform)->display, (*platform)->window, EVENT_TYPES);
    LOG_DEBUG("Selected input events: KeyPress, Expose, StructureNotify");

    XMapWindow((*platform)->display, (*platform)->window);
    LOG_DEBUG("Mapped window");

    (*platform)->WMDelete = XInternAtom((*platform)->display, "WM_DELETE_WINDOW", False);

    if (XSetWMProtocols((*platform)->display, (*platform)->window, &(*platform)->WMDelete, 1) == 0)
    {
        QuirePlatformDestroy(platform);
        QuireSetError(errorBuffer, "Failed to register WM_DELETE_WINDOW protocol");
        return false;
    }
    LOG_DEBUG("Registered WM_DELETE_WINDOW protocol");

    return true;
}

void QuirePlatformDestroy(QuirePlatform **restrict platform)
{
    if (platform == NULL || *platform == NULL)
        return;

    if ((*platform)->window != 0)
        XDestroyWindow((*platform)->display, (*platform)->window);

    if ((*platform)->display != NULL)
        XCloseDisplay((*platform)->display);

    free(*platform);
    *platform = NULL;
}
