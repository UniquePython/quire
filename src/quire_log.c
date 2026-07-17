#define _POSIX_C_SOURCE 200809L

#include "quire_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

static bool gUseColorStdout = false;
static bool gUseColorStderr = false;

#define QUIRE_COLOR_RESET "\033[0m"
#define QUIRE_COLOR_BOLD "\033[1m"

#define QUIRE_COLOR_RED "\033[31m"
#define QUIRE_COLOR_MAGENTA "\033[35m"
#define QUIRE_COLOR_YELLOW "\033[33m"
#define QUIRE_COLOR_CYAN "\033[36m"
#define QUIRE_COLOR_GRAY "\033[90m"

void QuireLogInit(void)
{
    gUseColorStdout = isatty(fileno(stdout));
    gUseColorStderr = isatty(fileno(stderr));
}

// Writes one log line: "[LEVEL] message (file:line, in func)".
//
// The message always comes first so it has visual priority; the location is
// a secondary, dimmed annex meant for when the reader actually needs to jump
// to the source, not for every-glance scanning. Pass file == NULL to omit
// the location entirely (used by QuireLogInfo).
static void QuireLogWrite(
    FILE *restrict stream,
    bool useColor,
    bool flush,
    const char *restrict level,
    const char *restrict color,
    const char *restrict file,
    i32 line,
    const char *restrict func,
    const char *restrict fmt,
    va_list args)
{
    if (useColor)
        fprintf(stream, "%s%s[%s]%s ", QUIRE_COLOR_BOLD, color, level, QUIRE_COLOR_RESET);
    else
        fprintf(stream, "[%s] ", level);

    vfprintf(stream, fmt, args);

    if (file != NULL)
    {
        if (useColor)
            fprintf(stream, " %s(%s:%d, in %s)%s", QUIRE_COLOR_GRAY, file, line, func, QUIRE_COLOR_RESET);
        else
            fprintf(stream, " (%s:%d, in %s)", file, line, func);
    }

    fprintf(stream, "\n");

    if (flush)
        fflush(stream);
}

#if VERBOSITY >= 1
void QuireLogError(const char *file, i32 line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    QuireLogWrite(stderr, gUseColorStderr, true, "ERROR", QUIRE_COLOR_RED, file, line, func, fmt, args);
    va_end(args);
}
#endif

#if VERBOSITY >= 2
void QuireLogWarning(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    QuireLogWrite(stdout, gUseColorStdout, false, "WARNING", QUIRE_COLOR_MAGENTA, NULL, 0, NULL, fmt, args);
    va_end(args);
}
#endif

#if VERBOSITY >= 3
void QuireLogInfo(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    QuireLogWrite(stdout, gUseColorStdout, false, "INFO", QUIRE_COLOR_YELLOW, NULL, 0, NULL, fmt, args);
    va_end(args);
}
#endif

#if VERBOSITY >= 4
void QuireLogDebug(const char *file, i32 line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    QuireLogWrite(stdout, gUseColorStdout, false, "DEBUG", QUIRE_COLOR_CYAN, file, line, func, fmt, args);
    va_end(args);
}
#endif
