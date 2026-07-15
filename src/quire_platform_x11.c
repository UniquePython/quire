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

static void DeriveShiftAndBits(unsigned long mask, u8 *restrict shift, u8 *restrict bits)
{
    *shift = 0;
    *bits = 0;

    if (mask == 0)
        return;

    while ((mask & 1UL) == 0)
    {
        ++(*shift);
        mask >>= 1;
    }

    while ((mask & 1UL) != 0)
    {
        ++(*bits);
        mask >>= 1;
    }
}

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

    Visual *visual = DefaultVisual((*platform)->display, (*platform)->screen);
    DeriveShiftAndBits(visual->red_mask, &(*platform)->pixelFormat.redShift, &(*platform)->pixelFormat.redBits);
    DeriveShiftAndBits(visual->green_mask, &(*platform)->pixelFormat.greenShift, &(*platform)->pixelFormat.greenBits);
    DeriveShiftAndBits(visual->blue_mask, &(*platform)->pixelFormat.blueShift, &(*platform)->pixelFormat.blueBits);
    LOG_DEBUG(
        "Pixel masks: R=0x%08lX G=0x%08lX B=0x%08lX", visual->red_mask, visual->green_mask, visual->blue_mask);

    i32 formatCount = 0;
    XPixmapFormatValues *formats = XListPixmapFormats((*platform)->display, &formatCount);
    if (formats == NULL)
    {
        QuirePlatformDestroy(platform);
        QuireSetError(errorBuffer, "Failed to query pixmap formats");
        return false;
    }

    i32 depth = DefaultDepth((*platform)->display, (*platform)->screen);
    bool found = false;
    for (i32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].depth == depth)
        {
            (*platform)->pixelFormat.bytesPerPixel = (u32)((formats[i].bits_per_pixel + 7) / 8);
            found = true;
            break;
        }
    }
    XFree(formats);

    if (!found)
    {
        QuirePlatformDestroy(platform);
        QuireSetError(errorBuffer, "No pixmap format found for depth %d", depth);
        return false;
    }
    LOG_DEBUG(
        "Pixel format: "
        "R%" PRIu8 "@%" PRIu8
        " G%" PRIu8 "@%" PRIu8
        " B%" PRIu8 "@%" PRIu8
        ", %" PRIu32 " bytes/pixel",

        (*platform)->pixelFormat.redBits,
        (*platform)->pixelFormat.redShift,

        (*platform)->pixelFormat.greenBits,
        (*platform)->pixelFormat.greenShift,

        (*platform)->pixelFormat.blueBits,
        (*platform)->pixelFormat.blueShift,

        (*platform)->pixelFormat.bytesPerPixel);

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
