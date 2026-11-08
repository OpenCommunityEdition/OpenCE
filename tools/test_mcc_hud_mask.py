"""HUD screen-effect mask units and scoped ownership, using production code."""
from pathlib import Path

import pytest
from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('flags,factor', [(0, 1), (0x10, .5), (0x80, .5), (0x90, .5)])
def test_mask_units_and_lifecycle(tmp_path, flags, factor):
    source = (ROOT/'port/linux/game/mcc_hud_draw.c').read_text()
    routines = '\n'.join(function(source, name) for name in [
        'mcc_hud_canvas_scale', 'mcc_hud_bitmap_scale'])
    routines += '\n'.join(function(source, name) for name in [
        'mcc_hud_mask_begin', 'mcc_hud_mask_end', 'mcc_hud_mask_transform'])
    run_c(tmp_path, r'''
#include <assert.h>
#include <stddef.h>
typedef float real;
typedef struct {float i,j;} real_vector2d;
struct bitmap_data {long tag_index;unsigned short flags;};
struct bitmap_group {unsigned short flags;};
#define NONE -1
static int mcc=1,reads;
static struct bitmap_group group;
static struct bitmap_data const *mcc_hud_mask;
static real mcc_hud_mask_scale=1.0f;
static int mcc_cache_tags_loaded(void) {return mcc;}
static struct bitmap_group *bitmap_group_get(long index) {assert(index==7);reads++;return &group;}
''' + routines + r'''
static void check(struct bitmap_data const *b,float factor) {
    real_vector2d size={512,256},scale;
    scale.i=(b && (b->flags&16))?1:1.f/512;
    scale.j=(b && (b->flags&16))?1:1.f/256;
    mcc_hud_mask_transform(b,&size,&scale);
    assert(size.i==512*factor && size.j==256*factor);
    assert(scale.i==((b && (b->flags&16))?1.f/factor:1.f/(512*factor)));
    assert(scale.j==((b && (b->flags&16))?1.f/factor:1.f/(256*factor)));
}
int main(void) {
    struct bitmap_data mask={7,16},other={7,16};
    int linear;
    group.flags=FLAGS;
    for(linear=0;linear<2;linear++) {
        mask.flags=linear?16:0;
        check(&mask,1);
        mcc_hud_mask_begin(&mask);
        check(&mask,FACTOR);
        check(&other,1); /* Render-target fallback and unrelated cinematic maps. */
        check(NULL,1);
        mcc_hud_mask_end();check(&mask,1);
        mcc_hud_mask_begin(NULL);check(&mask,1);
    }
    mcc=0;reads=0;
    mcc_hud_mask_begin(&mask);check(&mask,1);
    assert(reads==0); /* Xbox/CE never look up an MCC bitmap. */
    mcc_hud_mask_end();mcc=1;
    mcc_hud_mask_begin(&mask);mcc=0;check(&mask,1);
    mcc_hud_mask_end();mcc=1;check(&mask,1); /* Unload/reused pointer. */
    assert(mask.tag_index==7 && mask.flags==16); /* Original bitmap unchanged. */
    return 0;
}
'''.replace('FLAGS', str(flags)).replace('FACTOR', str(float(factor))+'f'))


def test_mask_scope_reaches_renderer_and_ends_before_next_view():
    draw = function((ROOT/'source/interface/interface.c').read_text(), 'interface_draw_screen')
    begin = draw.index('mcc_hud_mask_begin(')
    end = draw.index('mcc_hud_mask_end(')
    assert begin < draw.index('rasterizer_screen_effect(&parameters)') < end
    assert 'return' not in draw[begin:end]
    renderer = (ROOT/'source/rasterizer/xbox/rasterizer_xbox_screen_effect.c').read_text()
    assert renderer.count('mcc_hud_mask_transform(') == 1
    assert renderer.index('custom_edition_hud_mask_transform(') < renderer.index('mcc_hud_mask_transform(')
