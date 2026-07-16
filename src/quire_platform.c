#include "quire_platform.h"

// Platform-agnostic QuireKey/QuireModifier helpers shared by all backends.
// This file must not depend on any windowing-system headers — backend-specific
// translation (e.g. KeySym -> QuireKey) lives in the respective
// quire_platform_<backend>.c file instead.

// One-character names for every printable QuireKey (0x20-0x7E), e.g.
// printableKeyNames['A' - 0x20] == "A". Built once at compile time so
// QuireKeyName never needs mutable state to return a printable key's name.
static const char printableKeyNames[0x7E - 0x20 + 1][2] = {
    " ",
    "!",
    "\"",
    "#",
    "$",
    "%",
    "&",
    "'",
    "(",
    ")",
    "*",
    "+",
    ",",
    "-",
    ".",
    "/",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    ":",
    ";",
    "<",
    "=",
    ">",
    "?",
    "@",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "[",
    "\\",
    "]",
    "^",
    "_",
    "`",
    "a",
    "b",
    "c",
    "d",
    "e",
    "f",
    "g",
    "h",
    "i",
    "j",
    "k",
    "l",
    "m",
    "n",
    "o",
    "p",
    "q",
    "r",
    "s",
    "t",
    "u",
    "v",
    "w",
    "x",
    "y",
    "z",
    "{",
    "|",
    "}",
    "~",
};

const char *QuireKeyName(QuireKey key)
{
    if (key == ' ')
        return "Space";

    if (key >= 0x20 && key <= 0x7E)
        return printableKeyNames[key - 0x20];

    switch (key)
    {
    case QUIRE_KEY_UNKNOWN:
        return "Unknown";

    case QUIRE_KEY_ESCAPE:
        return "Escape";
    case QUIRE_KEY_ENTER:
        return "Enter";
    case QUIRE_KEY_TAB:
        return "Tab";
    case QUIRE_KEY_BACKSPACE:
        return "Backspace";
    case QUIRE_KEY_INSERT:
        return "Insert";
    case QUIRE_KEY_DELETE:
        return "Delete";

    case QUIRE_KEY_LEFT:
        return "Left";
    case QUIRE_KEY_RIGHT:
        return "Right";
    case QUIRE_KEY_UP:
        return "Up";
    case QUIRE_KEY_DOWN:
        return "Down";

    case QUIRE_KEY_HOME:
        return "Home";
    case QUIRE_KEY_END:
        return "End";
    case QUIRE_KEY_PAGE_UP:
        return "PageUp";
    case QUIRE_KEY_PAGE_DOWN:
        return "PageDown";

    case QUIRE_KEY_CAPS_LOCK:
        return "CapsLock";
    case QUIRE_KEY_NUM_LOCK:
        return "NumLock";

    case QUIRE_KEY_SHIFT_LEFT:
        return "ShiftLeft";
    case QUIRE_KEY_SHIFT_RIGHT:
        return "ShiftRight";
    case QUIRE_KEY_CONTROL_LEFT:
        return "ControlLeft";
    case QUIRE_KEY_CONTROL_RIGHT:
        return "ControlRight";
    case QUIRE_KEY_ALT_LEFT:
        return "AltLeft";
    case QUIRE_KEY_ALT_RIGHT:
        return "AltRight";
    case QUIRE_KEY_SUPER_LEFT:
        return "SuperLeft";
    case QUIRE_KEY_SUPER_RIGHT:
        return "SuperRight";

    case QUIRE_KEY_F1:
        return "F1";
    case QUIRE_KEY_F2:
        return "F2";
    case QUIRE_KEY_F3:
        return "F3";
    case QUIRE_KEY_F4:
        return "F4";
    case QUIRE_KEY_F5:
        return "F5";
    case QUIRE_KEY_F6:
        return "F6";
    case QUIRE_KEY_F7:
        return "F7";
    case QUIRE_KEY_F8:
        return "F8";
    case QUIRE_KEY_F9:
        return "F9";
    case QUIRE_KEY_F10:
        return "F10";
    case QUIRE_KEY_F11:
        return "F11";
    case QUIRE_KEY_F12:
        return "F12";

    default:
        return "Unknown";
    }
}
