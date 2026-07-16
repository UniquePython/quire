#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 400
#define HEIGHT 200

#define PIXEL_SIZE 64

void blit_glyph(
    uint8_t *target,
    int target_w,
    int target_h,
    FT_Bitmap *glyph_bitmap,
    int pen_x,
    int pen_y,
    int bitmap_left,
    int bitmap_top,
    uint8_t fg_r,
    uint8_t fg_g,
    uint8_t fg_b,
    uint8_t bg_r,
    uint8_t bg_g,
    uint8_t bg_b)
{
    for (int y = 0; y < glyph_bitmap->rows; y++)
    {
        for (int x = 0; x < glyph_bitmap->width; x++)
        {
            unsigned char coverage =
                glyph_bitmap->buffer[y * glyph_bitmap->pitch + x];

            /*
             * FreeType's origin is the baseline.
             *
             * bitmap_top is the distance above the baseline,
             * so we subtract it.
             */
            int target_x = pen_x + bitmap_left + x;
            int target_y = pen_y - bitmap_top + y;

            if (target_x < 0 ||
                target_x >= target_w ||
                target_y < 0 ||
                target_y >= target_h)
            {
                continue;
            }

            uint8_t *pixel =
                target + (target_y * target_w + target_x) * 3;

            pixel[0] =
                (fg_r * coverage + bg_r * (255 - coverage)) / 255;

            pixel[1] =
                (fg_g * coverage + bg_g * (255 - coverage)) / 255;

            pixel[2] =
                (fg_b * coverage + bg_b * (255 - coverage)) / 255;
        }
    }
}

void write_ppm(
    const char *filename,
    uint8_t *buffer,
    int width,
    int height)
{
    FILE *file = fopen(filename, "wb");

    if (!file)
    {
        perror("fopen");
        return;
    }

    fprintf(file,
            "P6\n%d %d\n255\n",
            width,
            height);

    fwrite(
        buffer,
        1,
        width * height * 3,
        file);

    fclose(file);
}

int testmain(void)
{
    uint8_t *buffer = malloc(WIDTH * HEIGHT * 3);

    if (!buffer)
    {
        fprintf(stderr, "malloc failed\n");
        return EXIT_FAILURE;
    }

    /*
     * Fill framebuffer with dark background.
     */
    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
        buffer[i * 3 + 0] = 20;
        buffer[i * 3 + 1] = 20;
        buffer[i * 3 + 2] = 30;
    }

    FT_Library library;

    if (FT_Init_FreeType(&library))
    {
        fprintf(stderr, "FreeType init failed\n");
        free(buffer);
        return EXIT_FAILURE;
    }

    FT_Face face;

    const char *font =
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf";

    if (FT_New_Face(library, font, 0, &face))
    {
        fprintf(stderr, "Font load failed\n");
        FT_Done_FreeType(library);
        free(buffer);
        return EXIT_FAILURE;
    }

    FT_Set_Pixel_Sizes(
        face,
        0,
        PIXEL_SIZE);

    FT_Load_Char(
        face,
        'A',
        FT_LOAD_RENDER);

    FT_GlyphSlot glyph = face->glyph;

    /*
     * Draw at baseline position:
     *
     * x = 100
     * y = 100
     */
    blit_glyph(
        buffer,
        WIDTH,
        HEIGHT,
        &glyph->bitmap,

        100,
        100,

        glyph->bitmap_left,
        glyph->bitmap_top,

        255, 255, 255, // foreground
        20, 20, 30     // background
    );

    write_ppm(
        "glyph.ppm",
        buffer,
        WIDTH,
        HEIGHT);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    free(buffer);

    printf("Wrote glyph.ppm\n");

    return EXIT_SUCCESS;
}