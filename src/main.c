#include "quire_error.h"
#include "quire_log.h"
#include "quire_platform.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// Packs a single RGB pixel according to the platform's reported pixel
// format (arbitrary shift/bit-width per channel, not assumed to be a fixed
// 0xRRGGBB layout), and writes it into `dst` using exactly
// format.bytesPerPixel bytes, least-significant byte first.
static void PackPixel(u8 *restrict dst, QuirePixelFormat format, u8 r, u8 g, u8 b)
{
    u32 value =
        ((u32)r << format.redShift) |
        ((u32)g << format.greenShift) |
        ((u32)b << format.blueShift);

    for (u32 i = 0; i < format.bytesPerPixel; ++i)
        dst[i] = (u8)(value >> (8 * i));
}

// Fills `pixels` (width * height * format.bytesPerPixel bytes) with a solid
// color, purely to prove the QuirePlatformPresent pipeline actually gets
// correctly formatted pixels onto the screen. Not real rendering.
static void FillSolidColor(u8 *restrict pixels, u32 width, u32 height, QuirePixelFormat format, u8 r, u8 g, u8 b)
{
    for (u32 y = 0; y < height; ++y)
    {
        for (u32 x = 0; x < width; ++x)
        {
            u8 *dst = pixels + ((usize)y * width + x) * format.bytesPerPixel;
            PackPixel(dst, format, r, g, b);
        }
    }
}

int main(void)
{
    QuireLogInit();

    QUIRE_ERROR_BUFFER(errbuf);

    QuirePlatform *platform = NULL;

    if (!QuirePlatformCreate(&platform, 900, 600, errbuf))
    {
        LOG_ERROR("Failed to create platform: %s", errbuf);
        return EXIT_FAILURE;
    }

    QuirePixelFormat pixelFormat = QuirePlatformGetPixelFormat(platform);

    u32 pixelsWidth = QuirePlatformGetWidth(platform);
    u32 pixelsHeight = QuirePlatformGetHeight(platform);
    u8 *pixels = malloc((usize)pixelsWidth * pixelsHeight * pixelFormat.bytesPerPixel);

    if (pixels == NULL)
    {
        LOG_ERROR("Failed to allocate pixel buffer");
        QuirePlatformDestroy(&platform);
        return EXIT_FAILURE;
    }

    FillSolidColor(pixels, pixelsWidth, pixelsHeight, pixelFormat, 0x20, 0x40, 0x80);

    bool running = true;

    while (running)
    {
        QuirePlatformWaitForEvent(platform, 16);

        QuireEvent event;
        QuirePlatformResult result;

        while ((result = QuirePlatformPollEvent(platform, &event)) == QUIRE_PLATFORM_OK)
        {
            switch (event.type)
            {
            case QUIRE_EVENT_KEY:
                LOG_DEBUG(
                    "Key %s: %s (modifiers=0x%02X)",
                    event.as.key.pressed ? "pressed" : "released",
                    QuireKeyName(event.as.key.key),
                    (unsigned int)event.as.key.modifiers);
                break;

            case QUIRE_EVENT_TEXT:
                LOG_DEBUG("Text event recorded (%zu bytes)", event.as.text.length);
                break;

            case QUIRE_EVENT_MOUSE_BUTTON:
                LOG_DEBUG(
                    "Mouse button %u %s at (%d, %d) (modifiers=0x%02X)",
                    (unsigned int)event.as.mouseButton.button,
                    event.as.mouseButton.pressed ? "pressed" : "released",
                    event.as.mouseButton.x,
                    event.as.mouseButton.y,
                    (unsigned int)event.as.mouseButton.modifiers);
                break;

            case QUIRE_EVENT_MOUSE_MOVE:
                LOG_DEBUG(
                    "Mouse moved to (%d, %d) (modifiers=0x%02X)",
                    event.as.mouseMove.x,
                    event.as.mouseMove.y,
                    (unsigned int)event.as.mouseMove.modifiers);
                break;

            case QUIRE_EVENT_SCROLL:
                LOG_DEBUG(
                    "Scroll at (%d, %d): delta=(%d, %d) (modifiers=0x%02X)",
                    event.as.scroll.x,
                    event.as.scroll.y,
                    event.as.scroll.deltaX,
                    event.as.scroll.deltaY,
                    (unsigned int)event.as.scroll.modifiers);
                break;

            case QUIRE_EVENT_RESIZE:
            {
                LOG_INFO(
                    "Window resized to %" PRIu32 "x%" PRIu32,
                    event.as.resize.width,
                    event.as.resize.height);

                u8 *resized = realloc(
                    pixels,
                    (usize)event.as.resize.width * event.as.resize.height * pixelFormat.bytesPerPixel);

                if (resized == NULL)
                {
                    LOG_ERROR("Failed to reallocate pixel buffer for new size; keeping old buffer");
                    break;
                }

                pixels = resized;
                pixelsWidth = event.as.resize.width;
                pixelsHeight = event.as.resize.height;

                FillSolidColor(pixels, pixelsWidth, pixelsHeight, pixelFormat, 0x20, 0x40, 0x80);
                break;
            }

            case QUIRE_EVENT_REDRAW:
                LOG_DEBUG("Redraw requested");
                break;

            case QUIRE_EVENT_CLOSE:
                LOG_INFO("Close requested");
                running = false;
                break;
            }
        }

        if (!running)
            break;

        if (!QuirePlatformPresent(platform, pixels, errbuf))
        {
            LOG_ERROR("Failed to present frame: %s", errbuf);
            running = false;
        }
    }

    free(pixels);
    QuirePlatformDestroy(&platform);

    return EXIT_SUCCESS;
}