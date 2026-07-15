#include "quire_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

static bool gUseColor = false;

#define QUIRE_COLOR_RESET "\033[0m"
#define QUIRE_COLOR_BOLD "\033[1m"

#define QUIRE_COLOR_RED "\033[31m"
#define QUIRE_COLOR_YELLOW "\033[33m"
#define QUIRE_COLOR_CYAN "\033[36m"

void QuireLogInit(void)
{
    gUseColor = isatty(fileno(stdout)) && isatty(fileno(stderr));
}

static void QuireLogWrite(
    FILE *restrict stream,
    const char *restrict level,
    const char *restrict color,
    const char *restrict fmt,
    va_list args)
{
    if (gUseColor)
        fprintf(stream, "%s%s[%s]%s ", QUIRE_COLOR_BOLD, color, level, QUIRE_COLOR_RESET);
    else
        fprintf(stream, "[%s] ", level);

    vfprintf(stream, fmt, args);
    fprintf(stream, "\n");
    fflush(stream);
}

void QuireLogError(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    QuireLogWrite(stderr, "ERROR", QUIRE_COLOR_RED, fmt, args);
    va_end(args);
}

void QuireLogInfo(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    QuireLogWrite(stdout, "INFO", QUIRE_COLOR_YELLOW, fmt, args);
    va_end(args);
}

void QuireLogDebug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    QuireLogWrite(stdout, "DEBUG", QUIRE_COLOR_CYAN, fmt, args);
    va_end(args);
}
