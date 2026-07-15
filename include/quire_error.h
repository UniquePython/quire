#ifndef QUIRE_ERROR_H_
#define QUIRE_ERROR_H_

#ifndef QUIRE_ERROR_BUFFER_SIZE
#define QUIRE_ERROR_BUFFER_SIZE 2048
#endif

void QuireSetError(char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE], const char *restrict fmt, ...);

#define QUIRE_ERROR_BUFFER(name) char name[QUIRE_ERROR_BUFFER_SIZE] = {0}

#endif