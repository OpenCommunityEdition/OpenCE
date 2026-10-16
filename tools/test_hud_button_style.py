"""Exercise pending menu edits and both production glyph paths without maps."""
import json
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def run_c(tmp_path, code):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    source = tmp_path / 'style.c'
    binary = tmp_path / 'style.exe'
    source.write_text(code)
    target = ['--target=i686-pc-windows-msvc'] if sys.platform == 'win32' else []
    result = subprocess.run([compiler, *target, '-std=gnu89', '-O1', str(source), '-o', str(binary)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(binary)], check=True, timeout=10)


def test_pending_visibility_spacing_and_focus(tmp_path):
    body = function((ROOT / 'port/linux/game/menu_functions.c').read_text(), 'hud_button_style_row_show')
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
typedef int boolean;
struct widget_instance {
    char const *name; short vertical_offset; int visible,definition_tag_index;
    struct widget_instance *child,*next,*parent,*focused_child;
    struct {struct {short selected_index;} list;} parameters;
};
struct pc_menu_setting {int value_count;char const *values[2];};
static struct widget_instance spinner;
static struct pc_menu_setting setting={2,{"true","false"}};
static struct pc_menu_setting *pc_menu_setting_get(int id) {(void)id;return &setting;}
static struct widget_instance *named(struct widget_instance *list,char const *name,int n) {
    struct widget_instance *p; (void)n;
    if (!strcmp(name,"high_res_hud_spinner")) return &spinner;
    for(p=list->child;p;p=p->next) if(!strcmp(p->name,name)) return p;
    return 0;
}
''' + body + r'''
int main(void) {
    struct widget_instance list={0},hud={0},style={0},text={0},aa={0},bar={0}; int i;
    list.child=&hud;hud.next=&style;style.next=&text;text.next=&aa;aa.next=&bar;
    hud.name="op_high_res_hud";style.name="op_hud_button_style";text.name="op_high_res_text";
    aa.name="op_anti_aliasing";bar.name="button_bar";spinner.parent=&hud;
    style.visible=1;text.vertical_offset=133;aa.vertical_offset=163;bar.vertical_offset=414;
    for(i=0;i<100;i++) {
        list.focused_child=&style;spinner.parameters.list.selected_index=1;
        hud_button_style_row_show(&list);hud_button_style_row_show(&list);
        assert(!style.visible && text.vertical_offset==103 && aa.vertical_offset==133);
        assert(list.focused_child==&hud && list.parameters.list.selected_index==0);
        spinner.parameters.list.selected_index=0;
        hud_button_style_row_show(&list);hud_button_style_row_show(&list);
        assert(style.visible && text.vertical_offset==133 && aa.vertical_offset==163);
        assert(bar.vertical_offset==414);
    }
    list.child=&bar;hud_button_style_row_show(&list);assert(bar.vertical_offset==414);
    return 0;
}
''')


@pytest.mark.parametrize('mcc', [False, True])
def test_six_button_styles_toggle_and_dispose(tmp_path, mcc):
    path = ROOT / 'port/linux/game' / ('mcc_hud_draw.c' if mcc else 'hud_hires_tags.c')
    source = path.read_text()
    start = source.index('static struct' if mcc else '/* Original Xbox controller faces')
    end = source.index('/* Identify the controller entry' if mcc else '/* ---------- private code */', start)
    state = source[start:end]
    names = ['mcc_hud_button_bitmap'] if mcc else ['hud_hires_button_draw']
    routines = '\n'.join(function(source, name) for name in names)
    tags = [a['tag'] for a in json.loads((ROOT/'port/assets/buttons/buttons.json').read_text())['assets']
            if a['name'] in ['a_butn__0','b_butn__0','x_butn__0','y_butn__0','black_butn__0','white_butn__0',
                             'mcc_black_butn__0','mcc_white_butn__0'] or 'classic' in a]
    prelude = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean; typedef float real; typedef unsigned long pixel32;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
#define TRUE 1
#define FALSE 0
#define csmemset memset
struct bitmap_data {unsigned long *hardware_format;};
struct icon_hud_element_definition {int unused;};
struct {struct {struct {int count;void *address;}button_icons;}messaging;} globals,*hud_globals=&globals;
struct dynamic_screen_vertex {struct {float x,y;}position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
    struct bitmap_data *map[3];struct {float i,j;}map_texture_scale[3],map_scale[3];int framebuffer_blend_function;
};
static int enabled=1,gems,live,last_asset=-1,draws;
static int config_boolean(char const *key) {(void)key;return enabled;}
static char const *config_string(char const *key) {(void)key;return gems?"gems":"classic";}
static int local_player_count(void) {return 1;}
static int draw_unicode_string_capital_middle(rectangle2d *r,void const *s) {(void)r;(void)s;return 100;}
static struct bitmap_data *bitmap_2d_new(int w,int h,int m,int f) {
    struct bitmap_data *b=calloc(1,sizeof(*b));assert(w==32&&h==32&&!m&&f==11);live++;return b;
}
static int rasterizer_bitmap_new(struct bitmap_data *b) {b->hardware_format=(unsigned long *)b;return 1;}
static void bitmap_delete(struct bitmap_data *b) {free(b);live--;}
static void xgpu_mcc_texture_unbind(unsigned long const *t) {(void)t;}
static void hud_hires_register_placeholder(long a,unsigned long const *t) {if(t)last_asset=(int)a;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *p,
    struct dynamic_screen_vertex *v) {
    assert(!p->framebuffer_blend_function);assert(v[2].position.x-v[0].position.x==21);
    assert(v[2].position.y-v[0].position.y==26);draws++;
}
'''
    prelude += 'static char const *tags[]={' + ','.join(json.dumps(t) for t in tags) + '};\n'
    prelude += 'static long hud_hires_asset_count(void) {return sizeof(tags)/sizeof(*tags);}\n'
    prelude += 'static char const *hud_hires_asset_tag(long i) {return tags[i];}\n'
    call = 'assert(mcc_hud_button_bitmap((short)i));' if mcc else 'assert(hud_hires_button_draw(&buttons[i],&cursor,0xffffffff));'
    dispose = 'mcc_hud_glyphs_dispose' if mcc else 'hud_buttons_forget'
    check_off = (r'''enabled=0;gems=0;assert(mcc_hud_button_bitmap(0));
        assert(!strstr(tags[last_asset],"classic"));''' if mcc else r'''
        enabled=0;assert(!hud_hires_button_draw(&buttons[0],&cursor,0xffffffff));
        enabled=1;assert(!hud_hires_button_draw(&unrelated,&cursor,0xffffffff));''')
    run_c(tmp_path, prelude + state + routines + r'''
int main(void) {
    struct icon_hud_element_definition buttons[6],unrelated;rectangle2d cursor={0};int i,j;
    globals.messaging.button_icons.count=6;globals.messaging.button_icons.address=buttons;
    for(j=0;j<30;j++) {gems=j%2;for(i=0;i<6;i++) {
        CALL
        assert((strstr(tags[last_asset],"classic")!=0)==!gems);
        CALL
        assert(live==i+1);
    }}
    OFF
    DISPOSE();DISPOSE();assert(live==0);return 0;
}
'''.replace('CALL',call).replace('OFF',check_off).replace('DISPOSE',dispose))
