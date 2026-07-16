#ifndef QUIRE_LOG_H_
#define QUIRE_LOG_H_

#include "quire_attributes.h"

#ifndef VERBOSITY
#define VERBOSITY 3 // 0 = silent, 1 = ERROR, 2 = ERROR+INFO, 3 = ERROR+INFO+DEBUG
#endif

void QuireLogInit(void);

#if VERBOSITY >= 1
void QuireLogError(const char *file, int line, const char *func, const char *fmt, ...) QUIRE_PRINTF(4, 5);
#define LOG_ERROR(...) QuireLogError(__FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif

#if VERBOSITY >= 2
void QuireLogInfo(const char *fmt, ...) QUIRE_PRINTF(1, 2);
#define LOG_INFO(...) QuireLogInfo(__VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if VERBOSITY >= 3
void QuireLogDebug(const char *file, int line, const char *func, const char *fmt, ...) QUIRE_PRINTF(4, 5);
#define LOG_DEBUG(...) QuireLogDebug(__FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif