"""Authored CE HD scaling, separate from Xbox redraws and MCC's canvas."""
from pathlib import Path
from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_draw_flags_and_known_map_numbers_stay_in_ce(tmp_path):
    source = (ROOT / 'port/linux/game/custom_edition_hud_draw.c').read_text()
    body = '\n'.join(function(source, name) for name in [
        'custom_edition_hud_bitmap_scale', 'custom_edition_hud_number_scale'])
    flags = (ROOT / 'port/linux/game/custom_edition_hud_scale.h').read_text()
    run_c(tmp_path, r'''
#include <assert.h>
typedef float real;
typedef int boolean;
#define FALSE 0
#define NONE -1
#define _custom_edition_behaviour_hud_number_scale 7
static int ce, legacy, reads;
static struct {unsigned short flags;} bitmap;
static int custom_edition_cache_tags_loaded(void) {return ce;}
static int custom_edition_cache_relies_on(int flag) {assert(flag==7);return legacy;}
#define bitmap_group_get(index) (reads++, &bitmap)
''' + flags + body + r'''
int main(void) {
    int p,b;
    const unsigned short bits[4]={0,0x10,0x80,0x90};
    const float expected[2][4]={{1,.5f,.5f,.25f},{.5f,.25f,.5f,.25f}};
    ce=1;
    for(p=0;p<2;p++) for(b=0;b<4;b++) {
        bitmap.flags=bits[b];
        assert(custom_edition_hud_number_scale(1,p*4)==expected[p][b]);
        assert(custom_edition_hud_bitmap_scale(1)==expected[0][b]);
    }
    /* Bigass-style metric correction already supplies the halving. */
    legacy=1;bitmap.flags=0x10;
    assert(custom_edition_hud_number_scale(1,4)==.5f);
    assert(custom_edition_hud_number_scale(1,0)==.5f);
    /* Xbox, MCC, unload/failed loads: never look up a CE bitmap. */
    ce=0;reads=0;
    assert(custom_edition_hud_number_scale(1,4)==1);
    assert(custom_edition_hud_bitmap_scale(1)==1);
    ce=1;
    assert(custom_edition_hud_number_scale(NONE,4)==1);
    assert(custom_edition_hud_bitmap_scale(NONE)==1);
    assert(reads==0);
    legacy=0;bitmap.flags=0;
    assert(custom_edition_hud_number_scale(1,0)==1);
    return 0;
}
''')


def test_mask_transform_scope_and_map_transitions(tmp_path):
    source = (ROOT / 'port/linux/game/custom_edition_hud_draw.c').read_text()
    body = source[source.index('static struct bitmap_data const *hud_mask;'):]
    run_c(tmp_path, r'''
#include <assert.h>
#include <stddef.h>
typedef float real;
typedef struct {float i,j;} real_vector2d;
struct bitmap_data {long tag_index;unsigned short flags;};
#define TEST_FLAG(v,b) ((v)&(1u<<(b)))
#define _bitmap_linear_bit 4
static int ce=1;
static float factor=.25f;
static int custom_edition_cache_tags_loaded(void) {return ce;}
static float custom_edition_hud_bitmap_scale(long index) {assert(index==7);return factor;}
''' + body + r'''
static void check(struct bitmap_data *bitmap,float width,float height,float x,float y) {
    real_vector2d size={512,256},scale={1,1};
    custom_edition_hud_mask_transform(bitmap,&size,&scale);
    assert(size.i==width && size.j==height && scale.i==x && scale.j==y);
}
int main(void) {
    struct bitmap_data mask={7,16},other={7,16};
    check(&mask,512,256,1,1);
    custom_edition_hud_mask_begin(&mask);
    check(&mask,128,64,4,4); /* Linear textures use pixel coordinates. */
    check(&other,512,256,1,1); /* Viewport fallback, other effects. */
    mask.flags=0;check(&mask,128,64,1.f/128,1.f/64);
    custom_edition_hud_mask_end();check(&mask,512,256,1,1);
    custom_edition_hud_mask_begin(NULL);check(&mask,512,256,1,1);
    ce=0;custom_edition_hud_mask_begin(&mask);check(&mask,512,256,1,1);
    custom_edition_hud_mask_end();
    ce=1;factor=.5f;mask.flags=16;
    custom_edition_hud_mask_begin(&mask);check(&mask,256,128,2,2);
    ce=0;check(&mask,512,256,1,1); /* Failed/unloaded CE guard. */
    custom_edition_hud_mask_end();
    ce=1;check(&mask,512,256,1,1); /* Reused bitmap address in next map. */
    return 0;
}
''')


def test_mask_draw_is_bracketed_and_mcc_number_path_is_preserved():
    draw = function((ROOT / 'source/interface/interface.c').read_text(), 'interface_draw_screen')
    assert draw.index('custom_edition_hud_mask_begin(') < draw.index('rasterizer_screen_effect(&parameters)')
    assert draw.index('rasterizer_screen_effect(&parameters)') < draw.index('custom_edition_hud_mask_end()')
    numbers = function((ROOT / 'source/interface/hud_draw.c').read_text(), 'hud_draw_numbers')
    assert 'if (custom_edition_cache_tags_loaded())' in numbers
    assert 'digit_scale *= mcc_hud_canvas_scale()' in numbers


def test_pickup_icon_draw_and_cursor_keep_authored_offsets(tmp_path):
    source = (ROOT / 'source/interface/hud_messaging.c').read_text()
    body = function(source, 'render_state_bitmap')
    run_c(tmp_path, r'''
#include <assert.h>
#include <stddef.h>
typedef float real;
typedef unsigned int pixel32;
typedef struct {short x,y;} point2d;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
typedef struct {float x0,y0,x1,y1;} real_rectangle2d;
struct bitmap_data {short width;};
struct icon_hud_element_definition {short frame_rate,sequence_index;point2d offset;unsigned short flags;pixel32 color;short width_offset;};
static struct {struct {struct {long index;} messaging_icons;} messaging;} globals={{{7}}},*hud_globals=&globals;
#define FALSE 0
#define TRUE 1
#define TEST_FLAG(v,b) ((v)&(1u<<(b)))
#define _hud_icon_use_color_bit 0
#define _hud_icon_absolute_width_bit 1
#define _hud_corner_bottom_left 0
static int mcc,button,owned,players=1,draws,reads,clipped;
static float drawn_scale,factor=.25f;
static point2d drawn_point;
static struct bitmap_data image={128};
static real_rectangle2d region={.25f,0,.75f,1};
static int mcc_cache_tags_loaded(void) {return mcc;}
static int hud_hires_button_draw(const struct icon_hud_element_definition *i,rectangle2d *b,pixel32 c) {return button;}
static int game_time_get(void) {return 0;}
static void hud_retrieve_bitmap_and_bounding_rect(long i,long s,long f,const struct bitmap_data **b,const real_rectangle2d **c) {*b=&image;*c=clipped?&region:NULL;}
static int _texture_cache_bitmap_get_hardware_format(struct bitmap_data *b,int a,int c) {return 1;}
static int mcc_hud_icon_draw(long i,const struct bitmap_data *b,const real_rectangle2d *c,rectangle2d *r,pixel32 p,const struct icon_hud_element_definition *icon,int f) {return owned;}
static const struct bitmap_data *hud_hires_sprite_bitmap(const struct bitmap_data *b,long s) {return b;}
static float custom_edition_hud_bitmap_scale(long i) {assert(i==7);reads++;return factor;}
static int local_player_count(void) {return players;}
static void hud_draw_bitmap_direct(const struct bitmap_data *b,int corner,const point2d *p,const real_rectangle2d *c,float scale,float rotation,pixel32 color,int linear) {draws++;drawn_scale=scale;drawn_point=*p;}
''' + body + r'''
int main(void) {
    struct icon_hud_element_definition icon={0,0,{8,12},0,0,4};
    rectangle2d bounds={0,0,640,480},cursor={100,0,640,80};
    render_state_bitmap(&bounds,&cursor,0,&icon);
    assert(draws==1 && drawn_scale==.25f && drawn_point.x==108 && drawn_point.y==68);
    assert(cursor.x0==144); /* 128*.25 + 4 + (100+8). */
    clipped=1;cursor.x0=100;render_state_bitmap(&bounds,&cursor,0,&icon);
    assert(cursor.x0==128); /* Sprite bounds scale, authored offset does not. */
    players=2;cursor.x0=100;render_state_bitmap(&bounds,&cursor,0,&icon);
    assert(drawn_scale==.1875f && drawn_point.x==106 && drawn_point.y==71 && cursor.x0==121);
    icon.flags=2;cursor.x0=100;render_state_bitmap(&bounds,&cursor,0,&icon);
    assert(cursor.x0==109); /* Absolute advance does not depend on bitmap flags. */
    draws=reads=0;button=1;render_state_bitmap(&bounds,&cursor,0,&icon);
    assert(draws==0 && reads==0); /* Stock replacement glyph owns its geometry. */
    button=0;mcc=owned=1;render_state_bitmap(&bounds,&cursor,0,&icon);
    assert(draws==0 && reads==0); /* MCC icon ownership remains first. */
    return 0;
}
''')
