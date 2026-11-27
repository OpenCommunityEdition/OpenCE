/* Normalize only terminal drawing, leaving map fonts and HUD text intact. */
#include "cseries.h"
#include "text/font_group.h"
#include "rasterizer/rasterizer.h"
#include "mcc_cache.h"
#include "mcc_hud_draw.h"

static real mcc_terminal_scale(long index)
{
    struct font_header const *font;
    real height;
    if (!mcc_cache_tags_loaded() || index==NONE) return 1.0f;
    font=font_definition_get(index);
    height=font->ascending_height+font->descending_height+font->leading_height;
    /* The native terminal's nominal line is 15 pixels at 480p. MCC maps
     * sometimes package a 33-pixel terminal font. Do not upscale small fonts. */
    return height>15.0f ? 15.0f/height : 1.0f;
}

short mcc_terminal_line_height(long font, short height)
{
    real scale=mcc_terminal_scale(font);
    return scale==1.0f ? height : (short)(height*scale+0.5f);
}

boolean mcc_terminal_draw(long font, rectangle2d const *bounds, char const *text)
{
    real scale=mcc_terminal_scale(font);
    real right,bottom;
    rectangle2d layout;
    if (scale==1.0f) return FALSE;
    layout=*bounds;
    right=layout.x0+(bounds->x1-bounds->x0)/scale;
    bottom=layout.y0+(bounds->y1-bounds->y0)/scale+1.0f;
    /* Map font metrics must not overflow the engine's short rectangles. */
    layout.x1=(short)(right>32767.0f ? 32767.0f : right< -32768.0f ? -32768.0f : right);
    layout.y1=(short)(bottom>32767.0f ? 32767.0f : bottom< -32768.0f ? -32768.0f : bottom);
    rasterizer_text_set_scale(scale,(real)bounds->x0,(real)bounds->y0);
    rasterizer_draw_string(&layout,NULL,NULL,0,text);
    rasterizer_text_set_scale(1.0f,0.0f,0.0f);
    return TRUE;
}
