#ifndef QUIRE_LOG_H_
#define QUIRE_LOG_H_

#ifndef VERBOSITY
#define VERBOSITY 3 // 0 = silent, 1 = ERROR, 2 = ERROR+INFO, 3 = ERROR+INFO+DEBUG
#endif

void QuireLogInit(void);

#if VERBOSITY >= 1
void QuireLogError(const char *fmt, ...);
#define LOG_ERROR(...) QuireLogError(__VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif

#if VERBOSITY >= 2
void QuireLogInfo(const char *fmt, ...);
#define LOG_INFO(...) QuireLogInfo(__VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if VERBOSITY >= 3
void QuireLogDebug(const char *fmt, ...);
#define LOG_DEBUG(...) QuireLogDebug(__VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif