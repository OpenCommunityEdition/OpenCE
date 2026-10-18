/* Exact shared CPU font/atlas functions plus their explicit native lifecycle.
   This CPU-only variant omits GL upload; engine_text_gl.c supplies it optionally. */
#include "engine_text.h"
#include <stdarg.h>
#include <stdio.h>
/* The glyph atlas at 1024x1024 (4 MB as RGBA) rather than 2048x2048 (16 MB):
   a G4 laptop's GPU may have only 32 MB, and at the sizes these Macs draw
   text, the glyphs fit (a full atlas is emptied and filled again). */
#define ATLAS_SIZE 1024
#include "mac_text_body.h"
static float mac_text_scale=1.0f;
int mac_text_set_pixel_scale(float scale)
{
    if(!(scale>0.0f) || scale>32.0f)return 0;
    mac_text_scale=scale;return 1;
}
float mac_text_pixel_scale(void){return mac_text_scale;}
void mac_text_log(const char *format,...)
{
    va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);fputc('\n',stderr);
}
const unsigned char *mac_text_atlas(size_t *bytes,unsigned int *width,unsigned int *height)
{
    if(bytes)*bytes=atlas?ATLAS_SIZE*ATLAS_SIZE:0;
    if(width)*width=atlas?ATLAS_SIZE:0;
    if(height)*height=atlas?ATLAS_SIZE:0;
    return atlas;
}
void mac_text_dispose(void)
{
    if(atlas && reset_hook)reset_hook();
    free(atlas);atlas=NULL;
    memset(fonts,0,sizeof(fonts));font_count=0;
    memset(glyphs,0,sizeof(glyphs));atlas_scale=0;
    pack_x=pack_y=pack_row_height=0;dirty_top=ATLAS_SIZE;dirty_bottom=0;
    placeholder_data=placeholder_width=placeholder_height=0;
    reset_hook=NULL;mac_text_scale=1.0f;
}
