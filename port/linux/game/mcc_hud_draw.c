/* The MCC canvas is 960p, while native UI/HUD coordinates are 480p.
 * Keep full-resolution texture samples and convert only screen geometry. */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "game/players.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "interface/hud_draw.h"
#include "text/draw_string.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.h"
#include "mcc_cache.h"
#include "mcc_hud_draw.h"
#include <string.h>

long halo_screen_width(void);
long hud_hires_asset_count(void);
char const *hud_hires_asset_tag(long asset);
void hud_hires_register_placeholder(long asset, unsigned long const *texture);
void xgpu_mcc_texture_unbind(unsigned long const *resource);

static struct {
    struct bitmap_data *bitmap;
    long asset;
    boolean attempted;
} mcc_buttons[6];

void mcc_hud_glyphs_dispose(void)
{
    long i;
    for (i=0;i<6;i++) if (mcc_buttons[i].bitmap) {
        hud_hires_register_placeholder(mcc_buttons[i].asset,NULL);
        xgpu_mcc_texture_unbind(mcc_buttons[i].bitmap->hardware_format);
        bitmap_delete(mcc_buttons[i].bitmap);
    }
    memset(mcc_buttons,0,sizeof(mcc_buttons));
}

/* Identify the controller entry, not its sequence: custom weapon/pickup
 * icons can use the same sequence numbers and must retain their artwork. */
static short mcc_hud_button_index(struct icon_hud_element_definition const *icon)
{
    short i;
    struct icon_hud_element_definition const *buttons;
    if (!hud_globals) return NONE;
    buttons=hud_globals->messaging.button_icons.address;
    if (!buttons) return NONE;
    for (i=0;i<6 && i<hud_globals->messaging.button_icons.count;i++)
        if (icon==buttons+i) return i;
    return NONE;
}

static struct bitmap_data const *mcc_hud_button_bitmap(short index)
{
    static char const *names[]={"ui\\shell\\bitmaps\\a_butn","ui\\shell\\bitmaps\\b_butn",
        "ui\\shell\\bitmaps\\x_butn","ui\\shell\\bitmaps\\y_butn",
        "mcc\\buttons\\black","mcc\\buttons\\white"};
    long asset;
    struct bitmap_data *bitmap;
    if (index<0 || index>=6) return NULL;
    if (mcc_buttons[index].attempted) return mcc_buttons[index].bitmap;
    mcc_buttons[index].attempted=TRUE;
    for (asset=0;asset<hud_hires_asset_count();asset++) {
        if (strcmp(hud_hires_asset_tag(asset),names[index])) continue;
        /* Reuse OpenCE's embedded hand-drawn buttons. The placeholder has
         * the original 32px layout, independent of the MCC sprite sheet. */
        bitmap=bitmap_2d_new(32,32,0,11);
        if (!bitmap) return NULL;
        if (!rasterizer_bitmap_new(bitmap)) {bitmap_delete(bitmap);return NULL;}
        hud_hires_register_placeholder(asset,bitmap->hardware_format);
        mcc_buttons[index].asset=asset;
        mcc_buttons[index].bitmap=bitmap;
        return bitmap;
    }
    return NULL;
}

/* Menu artwork has straight alpha; the gameplay HUD's multiply-add blend
 * would expose RGB stored under transparent pixels around the button. */
static void mcc_hud_button_draw(struct bitmap_data const *bitmap, point2d const *point,
    real scale, pixel32 color)
{
    struct rasterizer_dynamic_screen_geometry_parameters parameters;
    struct dynamic_screen_vertex vertices[4];
    short i;
    memset(&parameters,0,sizeof(parameters));
    parameters.map[0]=(struct bitmap_data *)bitmap;
    parameters.map_texture_scale[0].i=parameters.map_texture_scale[0].j=1.0f;
    parameters.map_scale[0].i=parameters.map_scale[0].j=1.0f;
    parameters.framebuffer_blend_function=0; /* alpha blend */
    for (i=0;i<4;i++) {
        boolean right=((i+1)&2)!=0, bottom=i>1;
        vertices[i].position.x=point->x+(right ? 21.0f*scale : 0.0f);
        vertices[i].position.y=point->y-(bottom ? 0.0f : 26.0f*scale);
        vertices[i].texture_coordinates.x=right ? 21.0f/32.0f : 0.0f;
        vertices[i].texture_coordinates.y=bottom ? 26.0f/32.0f : 0.0f;
        vertices[i].color=(color&0xFF000000UL)|0xFFFFFFUL;
    }
    rasterizer_psuedo_dynamic_screen_quad_draw(&parameters,vertices);
}

/* MCC child anchors are 0=inherited, 1..9=the nine absolute anchors.
 * The field occupies offset 6 in a weapon HUD element header (native pad).
 * Resolve into caller-owned storage, never into the shared parent tag. */
struct hud_absolute_placement_definition const *mcc_hud_child_placement(
    struct hud_absolute_placement_definition const *parent, void const *element,
    struct hud_absolute_placement_definition *storage)
{
    unsigned short child;
    if (!mcc_cache_tags_loaded()) return parent;
    memcpy(&child,(unsigned char const *)element+6,sizeof(child));
    if (!child || child>9) return parent;
    *storage=*parent;
    storage->corner=(short)(child-1);
    return storage;
}

real mcc_hud_canvas_scale(void)
{
    return mcc_cache_tags_loaded() ? 0.5f : 1.0f;
}

real mcc_hud_bitmap_scale(long bitmap_group_index)
{
    struct bitmap_group const *group;
    if (!mcc_cache_tags_loaded() || bitmap_group_index==NONE) return 1.0f;
    group=bitmap_group_get(bitmap_group_index);
    return group->flags&(0x10|0x80) ? 0.25f : 0.5f;
}

boolean mcc_hud_icon_draw(long bitmap_group_index, struct bitmap_data const *bitmap,
    real_rectangle2d const *clip, rectangle2d *cursor, pixel32 color,
    struct icon_hud_element_definition const *icon, boolean menu)
{
    real viewport_scale, pixel_scale, offset_scale, advance;
    point2d point;
    if (!mcc_cache_tags_loaded()) return FALSE;
    viewport_scale=!menu && local_player_count()>1 ? 0.75f : 1.0f;
    if (!menu) {
        struct bitmap_data const *button=mcc_hud_button_bitmap(mcc_hud_button_index(icon));
        if (button) {
            /* Crop transparent padding from the native 32px art. A 16x20
             * icon shares the current font's capital center, and its text
             * advance uses the text fragments' three-pixel kerning rather than
             * the map's modern-controller cell width. */
            real button_scale=viewport_scale*(20.0f/26.0f);
            point.x=(short)(cursor->x0+2.0f*viewport_scale);
            point.y=(short)(draw_unicode_string_capital_middle(cursor,L"")+10.0f*viewport_scale);
            mcc_hud_button_draw(button,&point,button_scale,color);
            cursor->x0=(short)(point.x+21.0f*button_scale-3.0f*viewport_scale);
            return TRUE;
        }
    }
    pixel_scale=viewport_scale*mcc_hud_bitmap_scale(bitmap_group_index);
    /* Icon offsets and extra text advances are font-relative, unlike the
     * HUD placement anchors. MCC tags retain the native values here. */
    offset_scale=viewport_scale;
    point.x=(short)(cursor->x0+icon->offset.x*offset_scale+(menu ? 1.0f : 0.0f));
    point.y=(short)(cursor->y1-icon->offset.y*offset_scale-(menu ? 2.0f : 0.0f));
    if (menu) {
        /* A map's HUD icon baseline may be custom-authored for its messages.
         * Native pause labels use the selected UI font instead. Align their
         * glyphs to its capitals, without resizing or changing the font. The
         * empty string measures this same line without introducing wrapping. */
        real height=(clip ? clip->y1-clip->y0 : 1.0f)*bitmap->height*pixel_scale;
        point.y=(short)(draw_unicode_string_capital_middle(cursor,L"")+height*0.5f);
    }
    hud_draw_bitmap_direct(bitmap,_hud_anchor_bottom_left,&point,clip,pixel_scale,0.0f,
        icon->flags&2 ? icon->color : color,FALSE);
    advance=icon->width_offset*offset_scale;
    if (!(icon->flags&4)) advance+=(clip ? clip->x1-clip->x0 : 1.0f)*bitmap->width*pixel_scale;
    cursor->x0=(short)(point.x+advance);
    return TRUE;
}

/* The four edge-center anchors are H1A additions (5..8). Keep them in
 * MCC tags and handle them here instead of changing the native enum. */
static boolean mcc_hud_edge(short anchor)
{
    return mcc_cache_tags_loaded() && anchor>=5 && anchor<=8;
}

static short mcc_hud_frame_inset(short gap,long maximum)
{
    return gap>4 ? (short)MIN(gap-gap/3,maximum) : 0;
}

boolean mcc_hud_anchor_point(short anchor, struct hud_placement_definition const *placement,
    struct bitmap_data const *bitmap, real scale, rectangle2d const *window,
    rectangle2d const *viewport, point2d *point)
{
    real x,y;
    rectangle2d frame;
    long horizontal_inset;
    if (!mcc_cache_tags_loaded() || anchor<0 || anchor>8 || anchor==4) return FALSE;
    horizontal_inset=halo_screen_width()/20;
    /* MCC's 2.5% title-safe frame versus the Xbox's 7.5%. Derive each
     * player's frame from its viewport; leave the small split-screen seam
     * inset alone. Cap the correction at 5% of the 480p screen so zoom
     * layouts retain their extra inset. Centre anchors use the native path. */
    frame=*window;
    frame.x0-=mcc_hud_frame_inset(window->x0-viewport->x0,horizontal_inset);
    frame.x1+=mcc_hud_frame_inset(viewport->x1-window->x1,horizontal_inset);
    frame.y0-=mcc_hud_frame_inset(window->y0-viewport->y0,24);
    frame.y1+=mcc_hud_frame_inset(viewport->y1-window->y1,24);
    if (anchor<4) {
        x=(anchor&1 ? frame.x1 : frame.x0)+placement->offset.x*scale*(anchor&1 ? -1 : 1);
        y=(anchor&2 ? frame.y1 : frame.y0)+placement->offset.y*scale*(anchor&2 ? -1 : 1);
    } else {
        x=(anchor==7 ? frame.x0 : anchor==8 ? frame.x1 : (frame.x0+frame.x1)*0.5f);
        y=(anchor==5 ? frame.y0 : anchor==6 ? frame.y1 : (frame.y0+frame.y1)*0.5f);
        x+=placement->offset.x*scale*(anchor==8 ? -1.0f : 1.0f);
        y+=placement->offset.y*scale*(anchor==6 ? -1.0f : 1.0f);
    }
    x-=viewport->x0;y-=viewport->y0;
    if (bitmap) {
        if (anchor<4) {
            x+=(bitmap->registration_point.x-(anchor&1 ? bitmap->width : 0))*scale;
            y+=(bitmap->registration_point.y-(anchor&2 ? bitmap->height : 0))*scale;
        } else {
            x+=(bitmap->registration_point.x-(anchor==8 ? bitmap->width : anchor<=6 ? bitmap->width*0.5f : 0))*scale;
            y+=(bitmap->registration_point.y-(anchor==6 ? bitmap->height : anchor>=7 ? bitmap->height*0.5f : 0))*scale;
        }
    }
    point->x=(short)x;point->y=(short)y;
    return TRUE;
}

boolean mcc_hud_anchor_bounds(short anchor, real width, real height, real_rectangle2d *bounds)
{
    if (!mcc_hud_edge(anchor)) return FALSE;
    bounds->x0=anchor==8 ? -width : anchor<=6 ? -width*0.5f : 0;
    bounds->y0=anchor==6 ? -height : anchor>=7 ? -height*0.5f : 0;
    bounds->x1=bounds->x0+width;bounds->y1=bounds->y0+height;
    return TRUE;
}

boolean mcc_hud_anchor_number(short anchor, real width, real origin, short *cursor)
{
    if (!mcc_hud_edge(anchor)) return FALSE;
    *cursor=(short)(origin+(anchor<=6 ? width*0.5f : anchor==7 ? width : 0));
    return TRUE;
}
