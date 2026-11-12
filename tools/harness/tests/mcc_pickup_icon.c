#include "harness.h"
#include <wchar.h>
typedef unsigned long pixel32;
typedef struct { short x, y; } point2d;
typedef struct { short x0, x1, y0, y1; } rectangle2d;
typedef struct { real x0, x1, y0, y1; } real_rectangle2d;
#include "definition.inc"
struct bitmap_group { unsigned short flags; } test_group;
struct bitmap_data { short width, height; } test_bitmap = {128, 64};
static struct { struct { struct { void *address; short count; } button_icons; } messaging; } *hud_globals;
static int loaded=1, players=1, replacement, draws, buttons;
static point2d drawn_point;
static real drawn_scale;
static pixel32 drawn_color;
#define mcc_cache_tags_loaded() loaded
#define bitmap_group_get(index) (&test_group)
#define local_player_count() players
#define config_boolean(name) FALSE
#define controller_glyph_draw_with_release(a,b,c,d,e,f) FALSE
#define mcc_hud_button_index(icon) 0
#define mcc_hud_button_bitmap(index) (replacement ? &test_bitmap : NULL)
#define draw_unicode_string_capital_middle(cursor,text) 80
#define mcc_hud_button_draw(a,b,c,d,e,f) (++buttons)
#define _hud_anchor_bottom_left 2
static void hud_draw_bitmap_direct(struct bitmap_data const *bitmap, int anchor,
    point2d const *point, real_rectangle2d const *clip, real scale, real theta,
    pixel32 color, boolean mirror)
{
    ++draws; drawn_point=*point; drawn_scale=scale; drawn_color=color;
}
#include "under_test.inc"

int main(int argc, char **argv)
{
    char const *mode=argv[1];
    rectangle2d cursor={100,200,50,100};
    struct icon_hud_element_definition icon={0};
    real_rectangle2d clipped={0.25f,0.75f,0,1};
    real_rectangle2d *clip=NULL;
    real expected_scale=0.25f;
    int expected_x=104, expected_y=116, expected_cursor=138;
    boolean menu=FALSE;
    test_group.flags=0x10;
    icon.offset.x=16; icon.offset.y=-64; icon.width_offset=8;
    if (!strcmp(mode,"plain")) { test_group.flags=0; expected_scale=0.5f; expected_x=108; expected_y=132; expected_cursor=176; }
    if (!strcmp(mode,"highres")) test_group.flags=0x80;
    if (!strcmp(mode,"split")) { players=2; expected_scale=0.1875f; expected_x=103; expected_y=112; expected_cursor=128; }
    if (!strcmp(mode,"signed")) { icon.offset.x=-3; icon.offset.y=3; expected_x=100; expected_y=100; expected_cursor=134; }
    if (!strcmp(mode,"clip")) { clip=&clipped; expected_cursor=122; }
    if (!strcmp(mode,"absolute")) { icon.flags=4; expected_cursor=106; }
    if (!strcmp(mode,"menu")) { menu=TRUE; expected_x=117; expected_y=88; expected_cursor=157; }
    if (!strcmp(mode,"button")) replacement=1;
    if (!strcmp(mode,"legacy") || !strcmp(mode,"transition")) {
        loaded=0;
        CHECK(!mcc_hud_icon_draw(0,&test_bitmap,NULL,&cursor,123,&icon,FALSE),"legacy owns drawing");
        CHECK(!draws && cursor.x0==100 && cursor.y1==100,"legacy cursor untouched");
        loaded=1;
    }
    CHECK(mcc_hud_icon_draw(0,&test_bitmap,clip,&cursor,123,&icon,menu),"MCC owns drawing");
    if (replacement) { CHECK(buttons==1 && draws==0,"replacement button owns layout"); return 0; }
    CHECK(draws==1 && drawn_point.x==expected_x && drawn_point.y==expected_y,"authored icon position");
    CHECK(drawn_scale==expected_scale && drawn_color==123,"texture scale and colour");
    CHECK(cursor.x0==expected_cursor,"text advance");
    if (!strcmp(mode,"transition")) {
        loaded=0; draws=0; cursor.x0=100;
        CHECK(!mcc_hud_icon_draw(0,&test_bitmap,NULL,&cursor,123,&icon,FALSE),"legacy after MCC");
        CHECK(!draws && cursor.x0==100,"no stale MCC placement");
    }
    return 0;
}
