// Adapted from https://github.com/harfbuzz/harfbuzz-tutorial/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <hb.h>
#include <hb-ft.h>
#include <cairo.h>
#include <cairo-ft.h>

#define FONT_SIZE 100
#define SPACING (FONT_SIZE * 64.0)
#define ROW_SIZE 32

// Pass only the font file as an argument
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: fontmap font-file.ttf\n");
        exit(1);
    }

    char* fontFile = argv[1];

    // All ascii characters which represent text
    char* ascii = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

    // Initialize FreeType and create FreeType font face.
    FT_Library library;
    FT_Face face;

    // Will not return zero on error
    if (FT_Init_FreeType (&library))
        abort();
    if (FT_New_Face(library, fontFile, 0, &face))
        abort();
    if (FT_Set_Char_Size(face, FONT_SIZE * 64, FONT_SIZE * 64, 0, 0))
        abort();

    // Create font
    hb_font_t *font;
    font = hb_ft_font_create(face, NULL);

    // Create buffer and populate.
    hb_buffer_t* buffer;
    buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, ascii, -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);

    // Shape the font
    hb_shape(font, buffer, NULL, 0);

    // Get glyph information and positions out of the buffer.
    unsigned int len = hb_buffer_get_length(buffer);
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, NULL);

    // Print the x advances of each character
    printf("Character advances:\n");
    for (unsigned int i = 0; i < len; i++)
    {
        hb_codepoint_t gid = info[i].codepoint;
        unsigned int cluster = info[i].cluster;
        double advance = pos[i].x_advance / 64.0;

        printf("%g, ", advance);
    }

    printf("\n");

    // Draw, using cairo
    double width = 0;
    double height = 0;

    height += ceil(len * 2.0 / ROW_SIZE) * FONT_SIZE;
    width += ROW_SIZE * FONT_SIZE;

    // Set up cairo surface
    cairo_surface_t *cairoSurface;
    cairoSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(width), ceil(height));
    cairo_t *cr;
    cr = cairo_create(cairoSurface);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);

    // Set up cairo font face
    cairo_font_face_t *cairoFace;
    cairoFace = cairo_ft_font_face_create_for_ft_face(face, 0);
    cairo_set_font_face(cr, cairoFace);
    cairo_set_font_size(cr, FONT_SIZE);

    // Set up baseline
    cairo_font_extents_t fontExtents;
    cairo_font_extents(cr, &fontExtents);
    double baseline = (FONT_SIZE - fontExtents.height) * 0.5 + fontExtents.ascent;
    cairo_translate(cr, 0, baseline);

    cairo_glyph_t *cairoGlyphs = cairo_glyph_allocate(len);
    double currentX = 0;
    double currentY = SPACING / 64.0 / 2;

    int count = 0;
    height = 0;
    for (unsigned int i = 0; i < len; i++)
    {
        cairoGlyphs[i].index = info[i].codepoint;
        cairoGlyphs[i].x = currentX + pos[i].x_offset / 64.0;
        cairoGlyphs[i].y = currentY + pos[i].y_offset / 64.0;

        count++;

        if (count >= ROW_SIZE)
        {
            count = 0;
            currentX = 0;
            height += 1;
            currentY += SPACING / 64.0 * 2;
        }
        else
            currentX += SPACING / 64.0;
    }

    cairo_show_glyphs(cr, cairoGlyphs, len);
    cairo_glyph_free(cairoGlyphs);

    cairo_surface_write_to_png(cairoSurface, "font.png");

    cairo_font_face_destroy(cairoFace);
    cairo_destroy(cr);
    cairo_surface_destroy(cairoSurface);

    hb_buffer_destroy(buffer);
    hb_font_destroy(font);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}
