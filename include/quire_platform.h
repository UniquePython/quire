#ifndef QUIRE_PLATFORM_H_
#define QUIRE_PLATFORM_H_

#include "quire_types.h"
#include "quire_error.h"

#include "quire_event.h"

typedef struct QuirePlatform QuirePlatform;

// ============ LIFECYCLE ============

bool QuirePlatformCreate(
    QuirePlatform **restrict platform,
    u32 width, u32 height,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE]);

void QuirePlatformDestroy(QuirePlatform *platform);

typedef struct
{
    u8 redBits;
    u8 greenBits;
    u8 blueBits;

    u8 redShift;
    u8 greenShift;
    u8 blueShift;

    u32 bytesPerPixel;
    u32 stride;
} QuirePixelFormat;

// ============ EVENT ============

typedef enum
{
    QUIRE_PLATFORM_OK,
    QUIRE_PLATFORM_NO_EVENT,
    QUIRE_PLATFORM_ERROR
} QuirePlatformResult;

bool QuirePlatformWaitForEvent(QuirePlatform *platform, u32 timeout);

QuirePlatformResult QuirePlatformPollEvent(
    QuirePlatform *restrict platform,
    QuireEvent *restrict event,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE]);

// ============ RENDERING ============

QuirePixelFormat QuirePlatformGetPixelFormat(const QuirePlatform *platform);

u32 QuirePlatformGetWidth(const QuirePlatform *platform);

u32 QuirePlatformGetHeight(const QuirePlatform *platform);

bool QuirePlatformPresent(
    QuirePlatform *restrict platform,
    const u8 *restrict pixels,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE]);

#endif