#ifndef QUIRE_EVENT_H_
#define QUIRE_EVENT_H_

#include "quire_types.h"

typedef enum
{
    QUIRE_EVENT_KEY,
    QUIRE_EVENT_TEXT,
    QUIRE_EVENT_MOUSE_BUTTON,
    QUIRE_EVENT_MOUSE_MOVE,
    QUIRE_EVENT_RESIZE,
    QUIRE_EVENT_REDRAW,
    QUIRE_EVENT_CLOSE,
} QuireEventType;

typedef struct
{
    QuireEventType type;
    union
    {
        struct
        {
            u32 key;
            bool pressed;
            u32 modifiers;
        } key;
        struct
        {
            char text[8];
            usize length;
        } text;
        struct
        {
            i32 x, y;
            u32 button;
            bool pressed;
            u32 modifiers;
        } mouseButton;
        struct
        {
            i32 x, y;
            u32 modifiers;
        } mouseMove;
        struct
        {
            u32 width, height;
        } resize;
    } as;
} QuireEvent;

#endif