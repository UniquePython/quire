#include "quire_platform.h"

#include "quire_log.h"

#include <stdlib.h>
#include <inttypes.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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

// ============ LIFECYCLE HELPERS ============

static bool OpenDisplay(QuirePlatform *restrict platform, char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
{
    platform->display = XOpenDisplay(NULL);
    if (platform->display == NULL)
    {
        QuireSetError(errorBuffer, "Failed to open X Display");
        return false;
    }

    LOG_DEBUG("Opened X Display");

    platform->screen = (i32)DefaultScreen(platform->display);
    LOG_DEBUG("Using default screen %" PRIi32, platform->screen);

    return true;
}

static bool CreateWindow(
    QuirePlatform *restrict platform,
    u32 width,
    u32 height,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
{
    Window rootWindow = RootWindow(platform->display, platform->screen);
    LOG_DEBUG("Using root window: %" PRIu64, (u64)rootWindow);

    platform->window = XCreateSimpleWindow(
        platform->display,
        rootWindow,
        0,
        0,
        (unsigned int)width,
        (unsigned int)height,
        1,
        BlackPixel(platform->display, platform->screen),
        WhitePixel(platform->display, platform->screen));

    if (platform->window == 0)
    {
        QuireSetError(errorBuffer, "Failed to create Window");
        return false;
    }
    LOG_DEBUG("Created window (%" PRIu32 "x%" PRIu32 ")", width, height);

    platform->width = width;
    platform->height = height;

    return true;
}

static bool SetupWindow(
    QuirePlatform *restrict platform,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
{
    static const long EVENT_TYPES = KeyPressMask | ExposureMask | StructureNotifyMask;

    XSelectInput(platform->display, platform->window, EVENT_TYPES);
    LOG_DEBUG("Selected input events: KeyPress, Expose, StructureNotify");

    XMapWindow(platform->display, platform->window);
    LOG_DEBUG("Mapped window");

    platform->WMDelete = XInternAtom(platform->display, "WM_DELETE_WINDOW", False);

    if (XSetWMProtocols(platform->display, platform->window, &platform->WMDelete, 1) == 0)
    {
        QuireSetError(errorBuffer, "Failed to register WM_DELETE_WINDOW protocol");
        return false;
    }
    LOG_DEBUG("Registered WM_DELETE_WINDOW protocol");

    return true;
}

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

static bool QueryPixelFormat(
    QuirePlatform *restrict platform,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
{
    Visual *visual = DefaultVisual(platform->display, platform->screen);
    DeriveShiftAndBits(visual->red_mask, &platform->pixelFormat.redShift, &platform->pixelFormat.redBits);
    DeriveShiftAndBits(visual->green_mask, &platform->pixelFormat.greenShift, &platform->pixelFormat.greenBits);
    DeriveShiftAndBits(visual->blue_mask, &platform->pixelFormat.blueShift, &platform->pixelFormat.blueBits);
    LOG_DEBUG(
        "Pixel masks: R=0x%08lX G=0x%08lX B=0x%08lX",
        visual->red_mask,
        visual->green_mask,
        visual->blue_mask);

    i32 formatCount = 0;
    XPixmapFormatValues *formats = XListPixmapFormats(platform->display, &formatCount);
    if (formats == NULL)
    {
        QuireSetError(errorBuffer, "Failed to query pixmap formats");
        return false;
    }

    i32 depth = DefaultDepth(platform->display, platform->screen);
    bool found = false;
    for (i32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].depth == depth)
        {
            platform->pixelFormat.bytesPerPixel = (u32)((formats[i].bits_per_pixel + 7) / 8);
            found = true;
            break;
        }
    }
    XFree(formats);

    if (!found)
    {
        QuireSetError(errorBuffer, "No pixmap format found for depth %d", depth);
        return false;
    }
    LOG_DEBUG(
        "Pixel format: "
        "R%" PRIu8 "@%" PRIu8
        " G%" PRIu8 "@%" PRIu8
        " B%" PRIu8 "@%" PRIu8
        ", %" PRIu32 " bytes/pixel",

        platform->pixelFormat.redBits,
        platform->pixelFormat.redShift,

        platform->pixelFormat.greenBits,
        platform->pixelFormat.greenShift,

        platform->pixelFormat.blueBits,
        platform->pixelFormat.blueShift,

        platform->pixelFormat.bytesPerPixel);

    return true;
}

// ============ LIFECYCLE ============

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

    if (!OpenDisplay(*platform, errorBuffer))
        goto failure;

    if (!CreateWindow(*platform, width, height, errorBuffer))
        goto failure;

    if (!SetupWindow(*platform, errorBuffer))
        goto failure;

    if (!QueryPixelFormat(*platform, errorBuffer))
        goto failure;

    return true;

failure:
    QuirePlatformDestroy(platform);
    return false;
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

// ============ EVENTS ============

void QuirePlatformWaitForEvent(QuirePlatform *platform, u32 timeoutMilliseconds)
{
    int fd = ConnectionNumber(platform->display);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv;
    tv.tv_sec = timeoutMilliseconds / 1000;
    tv.tv_usec = (timeoutMilliseconds % 1000) * 1000;

    if (select(fd + 1, &fds, NULL, NULL, &tv) == -1)
        LOG_DEBUG("select() failed (errno=%d): %s", errno, strerror(errno));
}

QuirePlatformResult QuirePlatformPollEvent(
    QuirePlatform *restrict platform,
    QuireEvent *restrict event,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
{
    (void)errorBuffer;

    while (XPending(platform->display) > 0)
    {
        XEvent xevent;
        XNextEvent(platform->display, &xevent);

        switch (xevent.type)
        {
        case ClientMessage:
            if ((Atom)xevent.xclient.data.l[0] == platform->WMDelete)
            {
                event->type = QUIRE_EVENT_CLOSE;
                return QUIRE_PLATFORM_OK;
            }
            break;

        case Expose:
            if (xevent.xexpose.count != 0)
                break;

            event->type = QUIRE_EVENT_REDRAW;
            return QUIRE_PLATFORM_OK;

        case ConfigureNotify:
            if (((u32)xevent.xconfigure.height != platform->height) || ((u32)xevent.xconfigure.width != platform->width))
            {
                platform->height = (u32)xevent.xconfigure.height;
                platform->width = (u32)xevent.xconfigure.width;

                event->type = QUIRE_EVENT_RESIZE;
                event->as.resize.height = platform->height;
                event->as.resize.width = platform->width;

                return QUIRE_PLATFORM_OK;
            }
            break;

        case KeyPress:
        {
            char buffer[8] = {0};
            KeySym keysym;
            int count = XLookupString(&xevent.xkey, buffer, sizeof(buffer), &keysym, NULL);

            bool hasCommandModifier = (xevent.xkey.state & ControlMask) != 0;

            if (!hasCommandModifier && count > 0)
            {
                // plain typing: emit TEXT
                event->type = QUIRE_EVENT_TEXT;
                event->as.text.length = (usize)count;
                memcpy(event->as.text.text, buffer, (usize)count);
                return QUIRE_PLATFORM_OK;
            }

            // either a command modifier was held, or XLookupString gave us
            // no text (arrows, Escape, Backspace, etc.) — emit KEY
            event->type = QUIRE_EVENT_KEY;
            event->as.key.key = (u32)keysym;
            event->as.key.pressed = true;
            event->as.key.modifiers = (u32)xevent.xkey.state;
            return QUIRE_PLATFORM_OK;
        }
        }
    }

    return QUIRE_PLATFORM_NO_EVENT;
}

// ============ RENDERING ============

QuirePixelFormat QuirePlatformGetPixelFormat(const QuirePlatform *platform)
{
    return platform->pixelFormat;
}

u32 QuirePlatformGetWidth(const QuirePlatform *platform)
{
    return platform->width;
}

u32 QuirePlatformGetHeight(const QuirePlatform *platform)
{
    return platform->height;
}
