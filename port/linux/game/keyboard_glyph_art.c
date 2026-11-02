/* Original supersampled prompt shapes. A small map-scoped cache avoids
 * integer-coordinate stair steps and keeps rounded ends at every key width. */
#include "cseries.h"
#include "keyboard_glyphs.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "rasterizer/rasterizer.h"
#include <string.h>

#define KEYBOARD_ART_SIZE 128
#define KEYBOARD_ART_COUNT 13
static struct bitmap_data *keyboard_art[KEYBOARD_ART_COUNT];
static int keyboard_art_attempted[KEYBOARD_ART_COUNT];

static int keyboard_inside(real x, real y, real inset, real radius)
{
    real dx = MAX(inset + radius - x, x - (32.0f - inset - radius));
    real dy = MAX(inset + radius - y, y - (32.0f - inset - radius));
    dx = MAX(dx, 0.0f); dy = MAX(dy, 0.0f);
    return dx * dx + dy * dy <= radius * radius;
}

static int keyboard_art_sample(int shape, real x, real y)
{
    real dx, dy;
    int body, inside;
    if (shape < 2)
        return shape ? keyboard_inside(x,y,1,5) && !keyboard_inside(x,y,2,4) : keyboard_inside(x,y,2,4);
    if (shape < 6)
    {
        dx = x - 16; dy = y - 16;
        if (shape == 3) dy = -dy;
        if (shape == 4) {real t = dx; dx = dy; dy = t;}
        if (shape == 5) {real t = dx; dx = dy; dy = -t;}
        return (dy >= -5 && dy <= 1 && dx >= -(dy+5) && dx <= dy+5) ||
            (dx >= -1.2f && dx <= 1.2f && dy >= 0 && dy <= 6);
    }
    /* Tall mouse silhouette with distinct button and wheel indications. */
    dx = MAX(12.0f-x,x-20.0f); dy = MAX(10.0f-y,y-22.0f);
    dx=MAX(dx,0);dy=MAX(dy,0);body=dx*dx+dy*dy<=25.0f;
    inside=dx*dx+dy*dy<=14.44f;
    if (body && (!inside || (y < 16 && x > 15.4f && x < 16.6f) || (y > 15.4f && y < 16.6f))) return 1;
    if (!inside) return 0;
    if (shape == 7) return x < 14.5f && y < 14.5f;
    if (shape == 8) return x > 17.5f && y < 14.5f;
    if (shape == 9) return x > 14.4f && x < 17.6f && y > 8 && y < 14;
    if (shape >= 10)
    {
        dx=x-16;dy=y-22;
        if (shape == 11) dy=-dy;
        if (shape == 12)
        {
            real distance = dy < 0 ? -dy : dy;
            return distance >= 1 && distance <= 4 && dx >= -(4-distance)*.7f && dx <= (4-distance)*.7f;
        }
        return dy >= -3 && dy <= 2 && dx >= -(dy+3)*0.65f && dx <= (dy+3)*0.65f;
    }
    return 0;
}

void keyboard_glyphs_dispose(void)
{
    int i;
    for(i=0;i<KEYBOARD_ART_COUNT;i++) if(keyboard_art[i]) bitmap_delete(keyboard_art[i]);
    memset(keyboard_art,0,sizeof(keyboard_art));
    memset(keyboard_art_attempted,0,sizeof(keyboard_art_attempted));
}

static struct bitmap_data *keyboard_art_get(int shape)
{
    struct bitmap_data *bitmap;
    int x,y,sx,sy,coverage;
    if(shape<0 || shape>=KEYBOARD_ART_COUNT) return NULL;
    if(keyboard_art_attempted[shape]) return keyboard_art[shape];
    keyboard_art_attempted[shape]=1;
    bitmap=bitmap_2d_new(KEYBOARD_ART_SIZE,KEYBOARD_ART_SIZE,0,11);
    if(!bitmap) return NULL;
    for(y=0;y<KEYBOARD_ART_SIZE;y++) for(x=0;x<KEYBOARD_ART_SIZE;x++)
    {
        coverage=0;
        for(sy=0;sy<4;sy++) for(sx=0;sx<4;sx++)
            coverage+=keyboard_art_sample(shape,(x+(sx+.5f)/4)*.25f,(y+(sy+.5f)/4)*.25f);
        *(pixel32 *)bitmap_2d_address(bitmap,(short)x,(short)y,0)=((unsigned long)(coverage*255/16)<<24)|0xffffffUL;
    }
    bitmap_rebuild(bitmap);
    if(!bitmap->hardware_format) {bitmap_delete(bitmap);return NULL;}
    keyboard_art[shape]=bitmap;
    return bitmap;
}

static void keyboard_art_quad(int shape, real x, real y, real width, real height, pixel32 color, int stretch)
{
    struct rasterizer_dynamic_screen_geometry_parameters p;
    struct dynamic_screen_vertex v[4];
    real xs[4],us[4]={0,.25f,.75f,1};
    int slice,i;
    memset(&p,0,sizeof(p));
    p.map[0]=keyboard_art_get(shape);
    if(!p.map[0]) return;
    p.map_texture_scale[0].i=p.map_texture_scale[0].j=1;
    p.map_scale[0].i=p.map_scale[0].j=1;
    xs[0]=x;xs[1]=x+height*.25f;xs[2]=x+width-height*.25f;xs[3]=x+width;
    for(slice=0;slice<(stretch?3:1);slice++)
    {
        for(i=0;i<4;i++)
        {
            int right=((i+1)&2)!=0,bottom=i>1;
            v[i].position.x=stretch?xs[slice+right]:x+right*width;
            v[i].position.y=y+bottom*height;
            v[i].texture_coordinates.x=stretch?us[slice+right]:(real)right;
            v[i].texture_coordinates.y=(real)bottom;
            v[i].color=color;
        }
        rasterizer_psuedo_dynamic_screen_quad_draw(&p,v);
    }
}

int keyboard_glyph_background(int symbol, real x, real y, real width, real height, pixel32 edge)
{
    pixel32 fill=(((edge>>24)*4/5)<<24)|0x06182aUL;
    if (!keyboard_art_get(0) || !keyboard_art_get(1) || (symbol >= 2 && !keyboard_art_get(symbol))) return 0;
    keyboard_art_quad(0,x,y,width,height,fill,1);
    keyboard_art_quad(1,x,y,width,height,edge,1);
    if(symbol>=2) keyboard_art_quad(symbol,x+(width-height)*.5f,y,height,height,edge,0);
    return 1;
}
