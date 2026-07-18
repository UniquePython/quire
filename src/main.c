#include "quire_error.h"
#include "quire_log.h"
#include "quire_platform.h"

#include <stdbool.h>
#include <stdlib.h>

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
            case QUIRE_EVENT_CLOSE:
                running = false;
                break;

            default:
                break;
            }
        }
    }

    QuirePlatformDestroy(&platform);

    return EXIT_SUCCESS;
}