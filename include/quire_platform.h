#ifndef QUIRE_PLATFORM_H_
#define QUIRE_PLATFORM_H_

#include "quire_types.h"
#include "quire_error.h"
#include "quire_attributes.h"

#include "quire_event.h"

typedef struct QuirePlatform QuirePlatform;

// ============ LIFECYCLE ============

QUIRE_WARN_UNUSED_RESULT bool QuirePlatformCreate(
    QuirePlatform **restrict platform,
    u32 width, u32 height,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE]);

void QuirePlatformDestroy(QuirePlatform **restrict platform);

typedef struct
{
    u8 redBits;
    u8 greenBits;
    u8 blueBits;

    u8 redShift;
    u8 greenShift;
    u8 blueShift;

    u32 bytesPerPixel;
} QuirePixelFormat;

// ============ EVENTS ============

typedef enum
{
    QUIRE_PLATFORM_OK,
    QUIRE_PLATFORM_NO_EVENT,
} QuirePlatformResult;

void QuirePlatformWaitForEvent(QuirePlatform *platform, u32 timeoutMilliseconds);

QUIRE_WARN_UNUSED_RESULT QuirePlatformResult QuirePlatformPollEvent(
    QuirePlatform *restrict platform,
    QuireEvent *restrict event);

// Returns a static, human-readable name for a QuireKey (e.g. "F1", "Escape",
// "A"), primarily intended for logging/debugging. Never returns NULL; unknown
// or unmapped values yield "Unknown".
QUIRE_CONST const char *QuireKeyName(QuireKey key);

// ============ RENDERING ============

QUIRE_PURE QuirePixelFormat QuirePlatformGetPixelFormat(const QuirePlatform *platform);

QUIRE_PURE u32 QuirePlatformGetWidth(const QuirePlatform *platform);

QUIRE_PURE u32 QuirePlatformGetHeight(const QuirePlatform *platform);

QUIRE_WARN_UNUSED_RESULT bool QuirePlatformPresent(
    QuirePlatform *restrict platform,
    const u8 *restrict pixels,
    char errorBuffer[restrict QUIRE_ERROR_BUFFER_SIZE]);

#endif