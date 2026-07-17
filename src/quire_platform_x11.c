#include "quire_platform.h"

#include "quire_log.h"

#include <stdlib.h>
#include <inttypes.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Maps each of X11's 8 modifier bit positions (ShiftMapIndex..Mod5MapIndex)
// to the QuireModifier it semantically represents on this system, or
// QUIRE_MOD_NONE if the slot is unbound or doesn't correspond to any
// QuireModifier the application tracks. Built once via XGetModifierMapping and refreshed
// on MappingNotify.
typedef struct
{
    QuireModifier semantic[8];
} QuireModifierMap;

struct QuirePlatform
{
    Display *display;
    Window window;
    i32 screen;
    Atom WMDelete;
    u32 width, height;
    QuirePixelFormat pixelFormat;
    XImage *image;
    QuireModifierMap modifierMap;
};

// ============ KEY / MODIFIER TRANSLATION ============

// Translates an X11 KeySym into a platform-agnostic QuireKey.
//
// Printable keysyms below 0x80 line up with ASCII/Latin-1 already, so they
// pass through unchanged (matching QuireKey's convention of reusing ASCII
// codes for printable keys). Named/non-printable keysyms are mapped
// individually since X11's keysym values don't correspond to QuireKey's.
static QUIRE_CONST QuireKey TranslateKeySym(KeySym keysym)
{
    if (keysym >= 0x20 && keysym <= 0x7E)
        return (QuireKey)keysym;

    switch (keysym)
    {
    case XK_Escape:
        return QUIRE_KEY_ESCAPE;
    case XK_Return:
    case XK_KP_Enter:
        return QUIRE_KEY_ENTER;
    case XK_Tab:
        return QUIRE_KEY_TAB;
    case XK_BackSpace:
        return QUIRE_KEY_BACKSPACE;
    case XK_Insert:
        return QUIRE_KEY_INSERT;
    case XK_Delete:
        return QUIRE_KEY_DELETE;

    case XK_Left:
        return QUIRE_KEY_LEFT;
    case XK_Right:
        return QUIRE_KEY_RIGHT;
    case XK_Up:
        return QUIRE_KEY_UP;
    case XK_Down:
        return QUIRE_KEY_DOWN;

    case XK_Home:
        return QUIRE_KEY_HOME;
    case XK_End:
        return QUIRE_KEY_END;
    case XK_Page_Up:
        return QUIRE_KEY_PAGE_UP;
    case XK_Page_Down:
        return QUIRE_KEY_PAGE_DOWN;

    case XK_Caps_Lock:
        return QUIRE_KEY_CAPS_LOCK;
    case XK_Num_Lock:
        return QUIRE_KEY_NUM_LOCK;

    case XK_Shift_L:
        return QUIRE_KEY_SHIFT_LEFT;
    case XK_Shift_R:
        return QUIRE_KEY_SHIFT_RIGHT;
    case XK_Control_L:
        return QUIRE_KEY_CONTROL_LEFT;
    case XK_Control_R:
        return QUIRE_KEY_CONTROL_RIGHT;
    case XK_Alt_L:
        return QUIRE_KEY_ALT_LEFT;
    case XK_Alt_R:
        return QUIRE_KEY_ALT_RIGHT;
    case XK_Super_L:
        return QUIRE_KEY_SUPER_LEFT;
    case XK_Super_R:
        return QUIRE_KEY_SUPER_RIGHT;

    case XK_F1:
        return QUIRE_KEY_F1;
    case XK_F2:
        return QUIRE_KEY_F2;
    case XK_F3:
        return QUIRE_KEY_F3;
    case XK_F4:
        return QUIRE_KEY_F4;
    case XK_F5:
        return QUIRE_KEY_F5;
    case XK_F6:
        return QUIRE_KEY_F6;
    case XK_F7:
        return QUIRE_KEY_F7;
    case XK_F8:
        return QUIRE_KEY_F8;
    case XK_F9:
        return QUIRE_KEY_F9;
    case XK_F10:
        return QUIRE_KEY_F10;
    case XK_F11:
        return QUIRE_KEY_F11;
    case XK_F12:
        return QUIRE_KEY_F12;

    default:
        return QUIRE_KEY_UNKNOWN;
    }
}

// Classifies a single X11 modifier bit position (ShiftMapIndex..Mod5MapIndex)
// by inspecting the keysyms bound to the keycodes the server has assigned to
// it. ShiftMapIndex/LockMapIndex/ControlMapIndex are fixed by the X11
// protocol; Mod1..Mod5 carry no fixed meaning; convention differs across
// window managers, remapped keyboards (e.g. xmodmap/setxkbmap swaps), and
// even some non-PC keyboards, so each is inspected via its bound keysyms
// rather than assumed.
static QUIRE_PURE QuireModifier ClassifyModifierSlot(Display *display, const XModifierKeymap *modmap, i32 slotIndex)
{
    switch (slotIndex)
    {
    case ShiftMapIndex:
        return QUIRE_MOD_SHIFT;
    case ControlMapIndex:
        return QUIRE_MOD_CONTROL;
    case LockMapIndex:
        return QUIRE_MOD_CAPS_LOCK;
    default:
        break;
    }

    // Mod1MapIndex..Mod5MapIndex: inspect every keycode bound to this slot
    // and classify by the keysyms attached to it. A single modifier bit can
    // have multiple keycodes bound (e.g. both Alt_L and Alt_R), so any match
    // among them is sufficient.
    for (i32 keySlot = 0; keySlot < modmap->max_keypermod; ++keySlot)
    {
        KeyCode keycode = modmap->modifiermap[(usize)slotIndex * (usize)modmap->max_keypermod + (usize)keySlot];
        if (keycode == 0)
            continue;

        // XKeycodeToKeysym is deprecated in favor of XGetKeyboardMapping;
        // fetch this single keycode's row of keysyms (shifted levels, etc.)
        // and check all of them rather than just level 0.
        i32 keysymsPerKeycode = 0;
        KeySym *keysyms = XGetKeyboardMapping(display, keycode, 1, &keysymsPerKeycode);
        if (keysyms == NULL)
            continue;

        for (i32 level = 0; level < keysymsPerKeycode; ++level)
        {
            switch (keysyms[level])
            {
            case XK_Alt_L:
            case XK_Alt_R:
                XFree(keysyms);
                return QUIRE_MOD_ALT;

            case XK_Super_L:
            case XK_Super_R:
            // Some window managers/keyboards report Super as "Meta" instead.
            case XK_Meta_L:
            case XK_Meta_R:
                XFree(keysyms);
                return QUIRE_MOD_SUPER;

            case XK_Num_Lock:
                XFree(keysyms);
                return QUIRE_MOD_NUM_LOCK;

            default:
                break;
            }
        }

        XFree(keysyms);
    }

    return QUIRE_MOD_NONE;
}

// Human-readable name for a single QuireModifier flag, for debug logging of
// the resolved modifier map. Only meant for the flags ClassifyModifierSlot
// can return for a Mod1..Mod5 slot.
static QUIRE_CONST const char *ModifierSemanticName(QuireModifier modifier)
{
    switch (modifier)
    {
    case QUIRE_MOD_ALT:
        return "Alt";
    case QUIRE_MOD_SUPER:
        return "Super";
    case QUIRE_MOD_NUM_LOCK:
        return "NumLock";
    case QUIRE_MOD_NONE:
        return "(none)";
    default:
        return "(unknown)";
    }
}

// Queries the server's current modifier mapping and caches, for each of the
// 8 modifier bit positions, which QuireModifier (if any) it represents. Call
// once at startup and again whenever the server reports the mapping changed
// (MappingNotify), since remapping tools like xmodmap/setxkbmap can move
// Alt/Super/NumLock to a different Mod slot at runtime.
static void RebuildModifierMap(QuirePlatform *restrict platform)
{
    for (i32 i = 0; i < 8; ++i)
        platform->modifierMap.semantic[i] = QUIRE_MOD_NONE;

    XModifierKeymap *modmap = XGetModifierMapping(platform->display);
    if (modmap == NULL)
    {
        LOG_ERROR("XGetModifierMapping failed; modifier translation may be incomplete");
        return;
    }

    for (i32 slot = 0; slot < 8; ++slot)
        platform->modifierMap.semantic[slot] = ClassifyModifierSlot(platform->display, modmap, slot);

    XFreeModifiermap(modmap);

    const char *mod1 = ModifierSemanticName(platform->modifierMap.semantic[Mod1MapIndex]);
    const char *mod2 = ModifierSemanticName(platform->modifierMap.semantic[Mod2MapIndex]);
    const char *mod3 = ModifierSemanticName(platform->modifierMap.semantic[Mod3MapIndex]);
    const char *mod4 = ModifierSemanticName(platform->modifierMap.semantic[Mod4MapIndex]);
    const char *mod5 = ModifierSemanticName(platform->modifierMap.semantic[Mod5MapIndex]);

    LOG_DEBUG("Modifier map: Mod1=%s Mod2=%s Mod3=%s Mod4=%s Mod5=%s", mod1, mod2, mod3, mod4, mod5);
}

// Translates X11's XKeyEvent.state modifier bitmask into a platform-agnostic
// QuireModifier bitmask, using the platform's cached modifier map rather
// than assuming which of Mod1..Mod5 correspond to Alt/Super/NumLock.
static QUIRE_PURE QuireModifier TranslateModifiers(const QuirePlatform *platform, u32 state)
{
    static const u32 slotMasks[8] = {
        ShiftMask,
        LockMask,
        ControlMask,
        Mod1Mask,
        Mod2Mask,
        Mod3Mask,
        Mod4Mask,
        Mod5Mask,
    };

    u32 modifiers = QUIRE_MOD_NONE;

    for (i32 slot = 0; slot < 8; ++slot)
    {
        if ((state & slotMasks[slot]) == 0)
            continue;

        modifiers |= (u32)platform->modifierMap.semantic[slot];
    }

    return (QuireModifier)modifiers;
}

// ============ LIFECYCLE HELPERS ============

static QUIRE_WARN_UNUSED_RESULT bool OpenDisplay(QuirePlatform *restrict platform, char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
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

static QUIRE_WARN_UNUSED_RESULT bool CreateWindow(
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

static QUIRE_WARN_UNUSED_RESULT bool SetupWindow(
    QuirePlatform *restrict platform,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE])
{
    static const long EVENT_TYPES = KeyPressMask | ExposureMask | StructureNotifyMask |
                                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

    XSelectInput(platform->display, platform->window, EVENT_TYPES);
    LOG_DEBUG("Selected input events: KeyPress, Expose, StructureNotify, ButtonPress, ButtonRelease, PointerMotion");

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

static QUIRE_WARN_UNUSED_RESULT bool QueryPixelFormat(
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

    if (width == 0 || height == 0)
    {
        QuireSetError(errorBuffer, "Window dimensions must be greater than zero");
        return false;
    }

    *platform = calloc(1, sizeof(**platform));
    if (*platform == NULL)
    {
        QuireSetError(errorBuffer, "Failed to allocate QuirePlatform");
        return false;
    }

    if (!OpenDisplay(*platform, errorBuffer))
        goto failure;

    RebuildModifierMap(*platform);

    if (!CreateWindow(*platform, width, height, errorBuffer))
        goto failure;

    if (!SetupWindow(*platform, errorBuffer))
        goto failure;

    if (!QueryPixelFormat(*platform, errorBuffer))
        goto failure;

    LOG_INFO("Quire ready");

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
    {
        XDestroyWindow((*platform)->display, (*platform)->window);
        LOG_DEBUG("Destroyed window");
    }

    if ((*platform)->display != NULL)
    {
        XCloseDisplay((*platform)->display);
        LOG_DEBUG("Closed display");
    }

    free(*platform);
    *platform = NULL;

    LOG_INFO("Shutdown successful");
}

// ============ MOUSE TRANSLATION ============

// Backend-private: classifies which "kind" of thing an X11 button number
// represents. Not part of the platform-agnostic API — X11-specific button
// numbering (1-3 = real buttons, 4-7 = wheel-as-buttons convention) is an
// X11 implementation detail.
typedef enum
{
    QUIRE_X11_BUTTON_NONE,
    QUIRE_X11_BUTTON_REAL,
    QUIRE_X11_BUTTON_SCROLL,
} QuireX11ButtonKind;

static QUIRE_CONST QuireX11ButtonKind ClassifyXButton(u32 xButton)
{
    if (1 <= xButton && xButton <= 3)
        return QUIRE_X11_BUTTON_REAL;
    else if (4 <= xButton && xButton <= 7)
        return QUIRE_X11_BUTTON_SCROLL;
    else
        return QUIRE_X11_BUTTON_NONE;
}

// Assumes xButton is already known to be a real button (1-3).
static QUIRE_CONST QuireMouseButton TranslateMouseButton(u32 xButton)
{
    switch (xButton)
    {
    case 1:
        return QUIRE_MOUSE_BUTTON_LEFT;

    case 2:
        return QUIRE_MOUSE_BUTTON_MIDDLE;

    case 3:
        return QUIRE_MOUSE_BUTTON_RIGHT;

    default:
        return QUIRE_MOUSE_BUTTON_UNKNOWN;
    }
}

// Assumes xButton is already known to be a scroll button (4-7). Fills in
// the signed delta for whichever axis this wheel button represents.
static void ScrollDeltaForButton(u32 xButton, i32 *restrict deltaX, i32 *restrict deltaY)
{
    *deltaX = 0;
    *deltaY = 0;

    if (xButton == 4)
        *deltaY = 1;
    else if (xButton == 5)
        *deltaY = -1;
    else if (xButton == 6)
        *deltaX = 1;
    else if (xButton == 7)
        *deltaX = -1;
}

// ============ EVENTS ============

void QuirePlatformWaitForEvent(QuirePlatform *platform, u32 timeoutMilliseconds)
{
    i32 fd = ConnectionNumber(platform->display);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv;
    tv.tv_sec = timeoutMilliseconds / 1000;
    tv.tv_usec = (timeoutMilliseconds % 1000) * 1000;

    if (select(fd + 1, &fds, NULL, NULL, &tv) == -1)
        LOG_WARN("select() failed (errno=%d): %s", errno, strerror(errno));
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
                LOG_DEBUG("Close event recorded");
                return QUIRE_PLATFORM_OK;
            }
            break;

        case Expose:
            if (xevent.xexpose.count != 0)
                break;

            event->type = QUIRE_EVENT_REDRAW;
            LOG_DEBUG("Redraw event recorded");
            return QUIRE_PLATFORM_OK;

        case ConfigureNotify:
            if (((u32)xevent.xconfigure.height != platform->height) || ((u32)xevent.xconfigure.width != platform->width))
            {
                platform->height = (u32)xevent.xconfigure.height;
                platform->width = (u32)xevent.xconfigure.width;

                event->type = QUIRE_EVENT_RESIZE;
                event->as.resize.height = platform->height;
                event->as.resize.width = platform->width;

                LOG_DEBUG("Resize event recorded. New dimensions %" PRIu32 "x%" PRIu32, platform->width, platform->height);

                return QUIRE_PLATFORM_OK;
            }
            break;

        case MappingNotify:
            // Nonmaskable; delivered to every client regardless of the event
            // mask. Sent whenever the server's keyboard/modifier/pointer
            // mapping changes (e.g. the user ran xmodmap/setxkbmap, or
            // plugged in a different keyboard). Xlib's own KeyCode->KeySym
            // cache also needs refreshing for XLookupString/XKeycodeToKeysym
            // to keep working correctly.
            if (xevent.xmapping.request == MappingModifier || xevent.xmapping.request == MappingKeyboard)
            {
                LOG_DEBUG("Keyboard mapping refresh requested");
                XRefreshKeyboardMapping(&xevent.xmapping);
                RebuildModifierMap(platform);
                LOG_INFO("Keyboard mapping updated");
            }
            break;

        case KeyPress:
        {
            char buffer[sizeof(event->as.text.text)] = {0};
            KeySym keysym;
            i32 count = XLookupString(&xevent.xkey, buffer, sizeof(buffer), &keysym, NULL);

            QuireModifier modifiers = TranslateModifiers(platform, xevent.xkey.state);

            // Any modifier other than Shift/CapsLock/NumLock turns this into
            // a command-like key combo (e.g. Ctrl+S, Alt+F4, Super+Tab)
            // rather than plain typing, regardless of what XLookupString
            // produced. Shift/CapsLock/NumLock are excluded here since they
            // only affect which character is produced (e.g. Shift+A -> "A"),
            // not the key's role as a shortcut.
            static const QuireModifier COMMAND_MODIFIERS =
                (QuireModifier)(QUIRE_MOD_CONTROL | QUIRE_MOD_ALT | QUIRE_MOD_SUPER);
            bool hasCommandModifier = (modifiers & COMMAND_MODIFIERS) != 0;

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
            event->as.key.key = TranslateKeySym(keysym);
            event->as.key.pressed = true;
            event->as.key.modifiers = modifiers;
            return QUIRE_PLATFORM_OK;
        }

        case ButtonPress:
        {
            u32 xButton = (u32)xevent.xbutton.button;
            QuireX11ButtonKind xButtonKind = ClassifyXButton(xButton);

            switch (xButtonKind)
            {
            case QUIRE_X11_BUTTON_REAL:
            {
                QuireMouseButton mouseButton = TranslateMouseButton(xButton);

                event->type = QUIRE_EVENT_MOUSE_BUTTON;

                event->as.mouseButton.x = (i32)xevent.xbutton.x;
                event->as.mouseButton.y = (i32)xevent.xbutton.y;
                event->as.mouseButton.button = mouseButton;
                event->as.mouseButton.pressed = true;
                event->as.mouseButton.modifiers = TranslateModifiers(platform, xevent.xbutton.state);

                return QUIRE_PLATFORM_OK;
            }

            case QUIRE_X11_BUTTON_SCROLL:
            {
                i32 deltaX, deltaY;
                ScrollDeltaForButton(xButton, &deltaX, &deltaY);

                event->type = QUIRE_EVENT_SCROLL;

                event->as.scroll.x = (i32)xevent.xbutton.x;
                event->as.scroll.y = (i32)xevent.xbutton.y;
                event->as.scroll.deltaX = deltaX;
                event->as.scroll.deltaY = deltaY;
                event->as.scroll.modifiers = TranslateModifiers(platform, xevent.xbutton.state);

                return QUIRE_PLATFORM_OK;
            }

            case QUIRE_X11_BUTTON_NONE:
                LOG_WARN("Unknown mouse button %" PRIu32 " clicked", xButton);
                break;
            }

            break;
        }

        case ButtonRelease:
        {
            u32 xButton = (u32)xevent.xbutton.button;
            QuireX11ButtonKind xButtonKind = ClassifyXButton(xButton);

            switch (xButtonKind)
            {
            case QUIRE_X11_BUTTON_REAL:
            {
                QuireMouseButton mouseButton = TranslateMouseButton(xButton);

                event->type = QUIRE_EVENT_MOUSE_BUTTON;

                event->as.mouseButton.x = (i32)xevent.xbutton.x;
                event->as.mouseButton.y = (i32)xevent.xbutton.y;
                event->as.mouseButton.button = mouseButton;
                event->as.mouseButton.pressed = false;
                event->as.mouseButton.modifiers = TranslateModifiers(platform, xevent.xbutton.state);

                return QUIRE_PLATFORM_OK;
            }

            case QUIRE_X11_BUTTON_SCROLL:
                LOG_WARN("Unexpected scroll button %" PRIu32 " release ignored", xButton);
                break;

            case QUIRE_X11_BUTTON_NONE:
                LOG_WARN("Unknown mouse button %" PRIu32 " released", xButton);
                break;
            }

            break;
        }

        case MotionNotify:
        {
            XEvent motion = xevent;
            XEvent peek;
            u8 nEvents = 0;

            while (XPending(platform->display) > 0)
            {
                XPeekEvent(platform->display, &peek);

                if (peek.type != MotionNotify)
                    break;

                XNextEvent(platform->display, &motion);

                nEvents++;
            }

            event->type = QUIRE_EVENT_MOUSE_MOVE;

            event->as.mouseMove.x = (i32)motion.xmotion.x;
            event->as.mouseMove.y = (i32)motion.xmotion.y;
            event->as.mouseMove.modifiers = TranslateModifiers(platform, motion.xmotion.state);

            LOG_DEBUG("Mouse movement event recorded");
            LOG_DEBUG("%" PRIu8 " mouse movement events suppressed", nEvents);

            return QUIRE_PLATFORM_OK;
        }
        }

        return QUIRE_PLATFORM_NO_EVENT;
    }
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
