#ifndef QUIRE_EVENT_H_
#define QUIRE_EVENT_H_

#include "quire_types.h"

typedef enum
{
    QUIRE_EVENT_KEY,
    QUIRE_EVENT_TEXT,
    QUIRE_EVENT_MOUSE_BUTTON,
    QUIRE_EVENT_MOUSE_MOVE,
    QUIRE_EVENT_SCROLL,
    QUIRE_EVENT_RESIZE,
    QUIRE_EVENT_REDRAW,
    QUIRE_EVENT_CLOSE,
} QuireEventType;

// Platform-agnostic key identifiers.
//
// Printable keys are mapped onto their ASCII codes (e.g. QUIRE_KEY_A == 'A',
// QUIRE_KEY_0 == '0'), so no translation table is needed for the common case
// and callers can compare against char literals directly. Non-printable/named
// keys live past the ASCII range (0x7F) so the two spaces never collide.
// Platform backends are responsible for translating their native keycodes
// into this enum.
typedef enum
{
    QUIRE_KEY_UNKNOWN = 0,

    // Printable keys share their ASCII code, e.g.:
    // QUIRE_KEY_SPACE = ' ' (0x20)
    // QUIRE_KEY_0..QUIRE_KEY_9 = '0'..'9' (0x30-0x39)
    // QUIRE_KEY_A..QUIRE_KEY_Z = 'A'..'Z' (0x41-0x5A)

    QUIRE_KEY_NAMED_START = 0x80,

    QUIRE_KEY_ESCAPE = QUIRE_KEY_NAMED_START,
    QUIRE_KEY_ENTER,
    QUIRE_KEY_TAB,
    QUIRE_KEY_BACKSPACE,
    QUIRE_KEY_INSERT,
    QUIRE_KEY_DELETE,

    QUIRE_KEY_LEFT,
    QUIRE_KEY_RIGHT,
    QUIRE_KEY_UP,
    QUIRE_KEY_DOWN,

    QUIRE_KEY_HOME,
    QUIRE_KEY_END,
    QUIRE_KEY_PAGE_UP,
    QUIRE_KEY_PAGE_DOWN,

    QUIRE_KEY_CAPS_LOCK,
    QUIRE_KEY_NUM_LOCK,

    QUIRE_KEY_SHIFT_LEFT,
    QUIRE_KEY_SHIFT_RIGHT,
    QUIRE_KEY_CONTROL_LEFT,
    QUIRE_KEY_CONTROL_RIGHT,
    QUIRE_KEY_ALT_LEFT,
    QUIRE_KEY_ALT_RIGHT,
    QUIRE_KEY_SUPER_LEFT,
    QUIRE_KEY_SUPER_RIGHT,

    QUIRE_KEY_F1,
    QUIRE_KEY_F2,
    QUIRE_KEY_F3,
    QUIRE_KEY_F4,
    QUIRE_KEY_F5,
    QUIRE_KEY_F6,
    QUIRE_KEY_F7,
    QUIRE_KEY_F8,
    QUIRE_KEY_F9,
    QUIRE_KEY_F10,
    QUIRE_KEY_F11,
    QUIRE_KEY_F12,
} QuireKey;

typedef enum
{
    QUIRE_MOUSE_BUTTON_UNKNOWN = 0,
    QUIRE_MOUSE_BUTTON_LEFT,
    QUIRE_MOUSE_BUTTON_MIDDLE,
    QUIRE_MOUSE_BUTTON_RIGHT,
} QuireMouseButton;

// Platform-agnostic modifier flags, combined with bitwise OR.
// Platform backends translate their native modifier state into this set.
typedef enum
{
    QUIRE_MOD_NONE = 0,
    QUIRE_MOD_SHIFT = 1u << 0,
    QUIRE_MOD_CONTROL = 1u << 1,
    QUIRE_MOD_ALT = 1u << 2,
    QUIRE_MOD_SUPER = 1u << 3,
    QUIRE_MOD_CAPS_LOCK = 1u << 4,
    QUIRE_MOD_NUM_LOCK = 1u << 5,
} QuireModifier;

typedef struct
{
    QuireEventType type;
    union
    {
        struct
        {
            QuireKey key;
            bool pressed;
            QuireModifier modifiers;
        } key;
        struct
        {
            char text[8];
            usize length;
        } text;
        struct
        {
            i16 x, y;
            QuireMouseButton button;
            bool pressed;
            QuireModifier modifiers;
        } mouseButton;
        struct
        {
            i16 x, y;
            QuireModifier modifiers;
        } mouseMove;
        struct
        {
            i16 x, y;
            i32 deltaX, deltaY;
            QuireModifier modifiers;
        } scroll;
        struct
        {
            u16 width, height;
        } resize;
    } as;
} QuireEvent;

#endif