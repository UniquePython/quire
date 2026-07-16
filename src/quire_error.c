#include "quire_error.h"

#include <stdarg.h>
#include <stdio.h>

void QuireSetError(
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE],
    const char *restrict fmt,
    ...)
{
    if (errorBuffer == NULL || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(errorBuffer, QUIRE_ERROR_BUFFER_SIZE, fmt, args);
    va_end(args);
}
