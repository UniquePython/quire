#ifndef QUIRE_LOG_H_
#define QUIRE_LOG_H_

#include "quire_attributes.h"

#include "quire_types.h"

#ifndef VERBOSITY
#define VERBOSITY 4 // 0 = silent, 1 = ERROR, 2 = ERROR+WARNING, 3 = ERROR+WARNING+INFO, 4 = ERROR+WARNING+INFO+DEBUG
#endif

void QuireLogInit(void);

#if VERBOSITY >= 1
void QuireLogError(const char *file, i32 line, const char *func, const char *fmt, ...) QUIRE_PRINTF(4, 5);
#define LOG_ERROR(...) QuireLogError(__FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif

#if VERBOSITY >= 2
void QuireLogWarning(const char *fmt, ...) QUIRE_PRINTF(1, 2);
#define LOG_WARN(...) QuireLogWarning(__VA_ARGS__)
#else
#define LOG_WARN(...) ((void)0)
#endif

#if VERBOSITY >= 3
void QuireLogInfo(const char *fmt, ...) QUIRE_PRINTF(1, 2);
#define LOG_INFO(...) QuireLogInfo(__VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if VERBOSITY >= 4
void QuireLogDebug(const char *file, i32 line, const char *func, const char *fmt, ...) QUIRE_PRINTF(4, 5);
#define LOG_DEBUG(...) QuireLogDebug(__FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif