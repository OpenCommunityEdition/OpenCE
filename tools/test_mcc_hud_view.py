"""A player's native unit HUD supplies a shared corner origin, not new offsets."""
from pathlib import Path

from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_view_frame_preserves_offsets_custom_anchors_and_other_players(tmp_path):
    source = (ROOT/'port/linux/game/mcc_hud_draw.c').read_text()
    routines = '\n'.join(function(source, name) for name in [
        'mcc_hud_view_begin', 'mcc_hud_view_end', 'mcc_hud_frame_inset', 'mcc_hud_anchor_point'])
    run_c(tmp_path, r'''
#include <assert.h>
#include <stddef.h>
#include <string.h>
typedef float real;
typedef int boolean;
typedef struct {short x,y;} point2d;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
struct bitmap_data {short width,height;point2d registration_point;};
struct hud_placement_definition {point2d offset;short pad;};
struct unit_datum {long definition_index;};
struct unit_definition {int unused;};
struct unit_hud_interface_definition {struct {short corner,pad;} absolute_placement;};
#define NONE -1
#define FALSE 0
#define TRUE 1
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MCC_HUD_NATIVE_FRAME 0x4d53
static boolean mcc_native_view;
static short mcc_weapon_frame=NONE;
static int loaded=1,reads,players=1,valid=1;
static long active=9;
static struct unit_datum unit={8};
static struct unit_definition definition;
static struct unit_hud_interface_definition hud;
static int mcc_cache_tags_loaded(void) {return loaded;}
static struct unit_datum const *unit_try_and_get(long i) {reads++;return i==7?&unit:NULL;}
static struct unit_definition const *unit_definition_get(long i) {assert(i==8);return &definition;}
static long unit_definition_get_active_hud_index(struct unit_definition const *d,int split) {
    assert(d==&definition && split==(players>1));return active;
}
static int local_player_count(void) {return players;}
static void *tag_get(long group,long i) {assert(group=='unhi' && i==9);return &hud;}
static int mcc_cache_contains(void const *p,unsigned size) {assert(p==&hud && size==sizeof(hud));return valid;}
static long halo_screen_width(void) {return 840;}
''' + routines + r'''
int main(void) {
    struct hud_placement_definition p={{19,65},0},original;
    rectangle2d window={63,36,777,444},viewport={0,0,840,480};
    point2d point;
    int anchor,split;
    original=p;
    for(split=0;split<2;split++) {
        players=split+1;
        hud.absolute_placement.pad=MCC_HUD_NATIVE_FRAME;
        mcc_hud_view_begin(7);
        mcc_weapon_frame=FALSE; /* Customized ballistic panel; message uses NONE. */
        for(anchor=0;anchor<4;anchor++) {
            assert(!mcc_hud_anchor_point(anchor,&p,NULL,1,&window,&viewport,&point));
            mcc_weapon_frame=NONE;
            assert(!mcc_hud_anchor_point(anchor,&p,NULL,1,&window,&viewport,&point));
        }
        assert(!memcmp(&p,&original,sizeof(p))); /* No digit/message offset rewrite. */
        assert(!mcc_hud_anchor_point(4,&p,NULL,1,&window,&viewport,&point));
        for(anchor=5;anchor<9;anchor++)
            assert(mcc_hud_anchor_point(anchor,&p,NULL,1,&window,&viewport,&point));
        mcc_hud_view_end();
        assert(mcc_hud_anchor_point(0,&p,NULL,1,&window,&viewport,&point));
        assert(point.x==40 && point.y==77);
        /* Unknown custom unit layouts retain the component policy. */
        hud.absolute_placement.pad=0;mcc_hud_view_begin(7);
        assert(!mcc_native_view);
        assert(mcc_hud_anchor_point(0,&p,NULL,1,&window,&viewport,&point));
        p.pad=MCC_HUD_NATIVE_FRAME;
        assert(!mcc_hud_anchor_point(0,&p,NULL,1,&window,&viewport,&point));
        p=original;mcc_hud_view_end();
    }
    hud.absolute_placement.pad=MCC_HUD_NATIVE_FRAME;
    mcc_hud_view_begin(7);mcc_hud_view_begin(NONE);assert(!mcc_native_view);
    active=NONE;mcc_hud_view_begin(7);assert(!mcc_native_view);active=9;
    valid=0;mcc_hud_view_begin(7);assert(!mcc_native_view);valid=1;
    loaded=0;reads=0;mcc_hud_view_begin(7);assert(!mcc_native_view && !reads);
    assert(!mcc_hud_anchor_point(0,&p,NULL,1,&window,&viewport,&point));
    mcc_hud_view_end();loaded=1;assert(!mcc_native_view);
    return 0;
}
''')


def test_scope_encloses_weapon_and_message_draws():
    draw = function((ROOT/'source/interface/hud.c').read_text(), 'hud_draw_screen')
    begin = draw.index('mcc_hud_view_begin(')
    end = draw.index('mcc_hud_view_end(')
    for call in ('hud_render_weapon_interface(', 'hud_render_unit_interface(', 'hud_messaging_update('):
        assert begin < draw.index(call) < end
    assert 'return' not in draw[begin:end]
