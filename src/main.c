#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>

// ======== CONFIGURATION CONSTANTS ========

#define PIXEL_SIZE 32

// ======== ENTRY POINT ========

int main(void)
{
    FT_Library library;

    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        fprintf(stderr, "[ERROR] Failed to initialize FreeType.\n");
        return EXIT_FAILURE;
    }

    printf("[LOG] FreeType initialized.\n");

    FT_Face face;

    const char *font = "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf";

    error = FT_New_Face(
        library,
        font,
        0,
        &face);

    if (error)
    {
        fprintf(stderr, "[ERROR] Couldn't load font: %s\n", font);
        FT_Done_FreeType(library);
        return EXIT_FAILURE;
    }

    printf("[LOG] Loaded: %s\n", face->family_name);
    printf("[LOG] Style : %s\n", face->style_name);

    error = FT_Set_Pixel_Sizes(face, 0, PIXEL_SIZE);

    if (error)
    {
        fprintf(stderr, "[ERROR] Couldn't set pixel size.\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return EXIT_FAILURE;
    }

    error = FT_Load_Char(face, 'A', FT_LOAD_RENDER);

    if (error)
    {
        fprintf(stderr, "[ERROR] Couldn't load glyph.\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return EXIT_FAILURE;
    }

    FT_GlyphSlot glyph = face->glyph;

    printf("[LOG] Bitmap width : %d\n", glyph->bitmap.width);
    printf("[LOG] Bitmap rows  : %d\n", glyph->bitmap.rows);

    printf("[LOG] Bearing X    : %d\n", glyph->bitmap_left);
    printf("[LOG] Bearing Y    : %d\n", glyph->bitmap_top);

    printf("[LOG] Advance X    : %ld\n", glyph->advance.x);
    printf("[LOG] Advance Y    : %ld\n", glyph->advance.y);

    printf("[LOG] Advance (px) = %ld\n", glyph->advance.x >> 6);
    printf("[LOG] Advance (py) = %ld\n", glyph->advance.y >> 6);

    FT_Bitmap *bmp = &glyph->bitmap;

    for (unsigned int y = 0; y < bmp->rows; y++)
    {
        for (unsigned int x = 0; x < bmp->width; x++)
        {
            unsigned char p =
                bmp->buffer[(int)y * bmp->pitch + (int)x];

            putchar(p > 128 ? '#' : ' ');
        }

        putchar('\n');
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return EXIT_SUCCESS;
}