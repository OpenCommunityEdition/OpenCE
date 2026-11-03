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
    target = ['--target=i686-pc-windows-msvc'] if sys.platform == 'win32' else ['-m32']
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
static void controller_button_rows_show(struct widget_instance *list) {(void)list;assert(0);}
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


def test_supplied_gems_preserve_transparency_and_fit_existing_bounds():
    import numpy as np
    from PIL import Image
    import button_assets
    reference = Image.open(button_assets.ASSETS/'a_butn__0.png').getchannel('A').getbbox()
    for entry in button_assets.GEM_BUTTONS:
        generated = button_assets.gameplay_gem(entry)
        committed = np.asarray(Image.open(button_assets.ASSETS/(entry['name']+'.png')))
        assert np.array_equal(generated, committed)
        bounds = Image.fromarray(generated).getchannel('A').getbbox()
        assert bounds[0] >= reference[0] and bounds[1] >= reference[1]
        assert bounds[2] <= reference[2] and bounds[3] <= reference[3]
        assert abs((bounds[0]+bounds[2])-(reference[0]+reference[2])) <= 1
        assert abs((bounds[1]+bounds[3])-(reference[1]+reference[3])) <= 1


def test_legacy_button_styles_toggle_and_dispose(tmp_path):
    path = ROOT / 'port/linux/game' / ('hud_hires_tags.c')
    source = path.read_text()
    start = source.index('/* Original Xbox controller faces')
    end = source.index('/* ---------- private code */', start)
    state = source[start:end]
    names = ['hud_hires_button_draw']
    routines = (ROOT/'port/linux/game/hud_button_layout.h').read_text() + '\n'.join(function(source, name) for name in names)
    tags = [a['tag'] for a in json.loads((ROOT/'port/assets/buttons/buttons.json').read_text())['assets']
            if a['name'] in ['a_butn__0','b_butn__0','x_butn__0','y_butn__0','black_butn__0','white_butn__0'] or 'classic' in a or 'source_png' in a]
    prelude = r'''
#define ce_hud_buttons_stock(icon, index) 1
static int ce,stock_sheet=1;
static int lowres_draws;
#define ce_hud_buttons_forget() ((void)0)
#define ce_hud_button_draw(icon,cursor,color) (++lowres_draws,1)
#define custom_edition_cache_tags_loaded() ce
#define custom_edition_cache_stock_tag(index) stock_sheet
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean; typedef float real; typedef unsigned long pixel32;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
#define TRUE 1
#define FALSE 0
#define csmemset memset
struct bitmap_data {unsigned long *hardware_format;};
struct icon_hud_element_definition {unsigned flags;pixel32 color;short frame_rate;};
struct {struct {struct {int count;void *address;}button_icons;struct {long index;}messaging_icons;}messaging;} globals,*hud_globals=&globals;
struct dynamic_screen_vertex {struct {float x,y;}position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
    struct bitmap_data *map[3];struct {float i,j;}map_texture_scale[3],map_scale[3];int framebuffer_blend_function;
};
static int enabled=1,gems,live,last_asset=-1,draws;
static short controller_glyph_style(void) {return (short)gems;}
#define controller_glyph_draw(style,index,icon,cursor,color) 0
#define controller_glyphs_dispose() ((void)0)
static pixel32 drawn_color;
static int drawn_blend;
/* Tutorial controls must depend on High-Res HUD, never High-Res Text. */
static int config_boolean(char const *key) {assert(!strcmp(key,"display.high_res_hud"));return enabled;}
static char const *config_string(char const *key) {(void)key;return gems==2?"menu_gems":gems?"gems":"classic";}
static long ticks;
static long game_time_get(void) {return ticks;}
static int local_player_count(void) {return 1;}
static int draw_unicode_string_capital_middle(rectangle2d *r,void const *s) {(void)r;(void)s;return 100;}
static struct bitmap_data *bitmap_2d_new(int w,int h,int m,int f) {
    struct bitmap_data *b=calloc(1,sizeof(*b));assert((w==32||w==64)&&h==w&&!m&&f==11);live++;return b;
}
static int rasterizer_bitmap_new(struct bitmap_data *b) {b->hardware_format=(unsigned long *)b;return 1;}
static void bitmap_delete(struct bitmap_data *b) {free(b);live--;}
static void hud_hires_register_placeholder(long a,unsigned long const *t) {if(t)last_asset=(int)a;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *p,
    struct dynamic_screen_vertex *v) {
    drawn_blend=p->framebuffer_blend_function;assert(drawn_blend==0||drawn_blend==7);assert(v[2].position.x-v[0].position.x>=21);
    assert(v[2].position.y-v[0].position.y>=26);drawn_color=v[0].color;draws++;
}
'''
    prelude += 'static char const *tags[]={' + ','.join(json.dumps(t) for t in tags) + '};\n'
    prelude += 'static long hud_hires_asset_count(void) {return sizeof(tags)/sizeof(*tags);}\n'
    prelude += 'static char const *hud_hires_asset_tag(long i) {return tags[i];}\n'
    call = r'''
        buttons[i].flags=2;buttons[i].color=0xff123456;
        assert(hud_hires_button_draw(&buttons[i],&cursor,0x7fabcdef));
        assert(drawn_color==0xffffffff);
        buttons[i].color=0x40123456;
        assert(hud_hires_button_draw(&buttons[i],&cursor,0x7fabcdef));
        assert(drawn_color==0x40ffffff);
        buttons[i].flags=0;
        assert(hud_hires_button_draw(&buttons[i],&cursor,0x7fabcdef));
        assert(drawn_color==0x7fffffff);
        assert(hud_hires_button_draw(&buttons[i],&cursor,0));
        assert(drawn_color==0x00ffffff);
        '''
    dispose = 'hud_buttons_forget'
    check_off = (r'''
        enabled=0;assert(!hud_hires_button_draw(&buttons[0],&cursor,0xffffffff));
        ce=1;assert(!hud_hires_button_draw(&buttons[0],&cursor,0xffffffff));
        stock_sheet=0;assert(hud_hires_button_draw(&buttons[0],&cursor,0xffffffff));
        assert(lowres_draws==1);
        for(i=1;i<18;i++) assert(hud_hires_button_draw(&buttons[i],&cursor,0xffffffff));
        assert(lowres_draws==18);
        enabled=1;assert(hud_hires_button_draw(&buttons[0],&cursor,0xffffffff));
        assert(lowres_draws==18);
        enabled=0;
        assert(!hud_hires_button_draw(&unrelated,&cursor,0xffffffff));
        ce=0;assert(!hud_hires_button_draw(&buttons[0],&cursor,0xffffffff));
        enabled=1;assert(!hud_hires_button_draw(&unrelated,&cursor,0xffffffff));''')
    run_c(tmp_path, prelude + state + routines + r'''
int main(void) {
    struct icon_hud_element_definition buttons[18]={0},unrelated;rectangle2d cursor={0};int i,j;
    globals.messaging.button_icons.count=18;globals.messaging.button_icons.address=buttons;
    for(j=0;j<30;j++) {gems=j%3;for(i=0;i<6;i++) {
        CALL
        assert((strstr(tags[last_asset],"classic")!=0)==!gems);
        assert((strstr(tags[last_asset],"gem_")!=0)==(gems==1 && i<4));
        CALL
        assert(live==i+1);
    }}
    DISPOSE();
    for(j=0;j<3;j++) {
        int control,frame,slot;gems=j;
        for(control=6;control<18;control++) {
            if(control>=8 && control<14) continue;
            for(frame=0;frame<3;frame++) {
                ticks=frame*10;buttons[control].frame_rate=10;
                slot=hud_button_control_slot((short)control,frame);
                CONTROL
                if (frame==0 && control<8) assert(!strcmp(tags[last_asset],hud_button_control_name((short)slot)));
            }
        }
        assert(live==8);DISPOSE();
    }
    OFF
    DISPOSE();DISPOSE();assert(live==0);return 0;
}
'''.replace('CALL',call).replace('CONTROL', 'assert(hud_hires_button_draw(&buttons[control],&cursor,0x7fabcdef));assert(drawn_color==0x7fabcdef);assert(drawn_blend==7);').replace('OFF',check_off).replace('DISPOSE',dispose))


def test_visible_face_bounds_match_xbox_and_keep_black_white(tmp_path):
    import numpy as np
    from PIL import Image
    cases = []
    for style, prefix in enumerate(("classic_", "gem_", "")):
        for index, button in enumerate("abxy"):
            name = (prefix + button if prefix else button + "_butn") + "__0.png"
            alpha = np.asarray(Image.open(ROOT / "port/assets/buttons" / name))[..., 3]
            ys, xs = np.where(alpha >= 128)
            cases.append((style, index, int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1))
    array = ",".join("{" + ",".join(map(str, case)) + "}" for case in cases)
    header = (ROOT / "port/linux/game/hud_button_layout.h").read_text()
    run_c(tmp_path, "#include <assert.h>\ntypedef float real;\n" + header +
          "\nstatic int cases[][6]={" + array + "};\n" + r'''
static void near(real a,real b) {assert(a-b<0.0001f && b-a<0.0001f);}
int main(void) {
    real scales[]={1.0f,0.75f,20.0f/26.0f,15.0f/26.0f};
    int i,j;
    for(i=0;i<12;i++) for(j=0;j<4;j++) {
        int *c=cases[i];real scale=scales[j];
        real x=104.0f,y=87.0f,w=21.0f*scale,h=26.0f*scale;
        hud_button_face_bounds((short)c[0],(short)c[1],&x,&y,&w,&h);
        near(x+c[2]*w/168.0f,104.0f);
        near(y+c[3]*h/208.0f,87.0f);
        near((c[4]-c[2])*w/168.0f,21.0f*scale);
        near((c[5]-c[3])*h/208.0f,26.0f*scale);
    }
    for(i=4;i<6;i++) {
        real x=104,y=87,w=21,h=26;
        hud_button_face_bounds(0,(short)i,&x,&y,&w,&h);
        assert(x==104 && y==87 && w==21 && h==26);
    }
    return 0;
}
''')


def test_supplied_controls_and_animation():
    import numpy as np
    from PIL import Image
    import button_assets
    for entry in button_assets.CONTROL_BUTTONS:
        generated = button_assets.gameplay_control(entry)
        committed = np.asarray(Image.open(button_assets.ASSETS / (entry['name'] + '.png')))
        assert np.array_equal(generated, committed)
        assert np.all(generated[..., :3] <= generated[..., 3:4])
        assert np.all(generated[generated[...,3]==0, :3]==0)
        alpha = Image.fromarray(generated).getchannel('A')
        x, y, right, bottom = alpha.getbbox()
        width, height = entry['control_cell']
        assert 0 < x < right < width * 8
        assert 0 < y < bottom < height * 8
        assert abs(x + right - width * 8) <= 1
        assert abs(y + bottom - height * 8) <= 1


def test_dark_gems_label_keeps_existing_saved_value():
    import port_settings
    output = '\n'.join('\n'.join(lines) for lines in port_settings.settings_files().values())
    assert 'DARK GEMS' in output and 'MENU GEMS' not in output
    assert 'menu_gems' in output


def test_control_visible_bounds_and_animation_slots(tmp_path):
    import numpy as np
    from PIL import Image
    import button_assets
    cases=[]
    for slot,e in enumerate(button_assets.CONTROL_BUTTONS,6):
        a=np.asarray(Image.open(button_assets.ASSETS/(e['name']+'.png')))[...,3]
        ys,xs=np.where(a>=128)
        cases.append([slot,int(xs.min()),int(ys.min()),int(xs.max())+1,int(ys.max())+1])
    array=','.join('{'+','.join(map(str,c))+'}' for c in cases)
    header=(ROOT/'port/linux/game/hud_button_layout.h').read_text()
    run_c(tmp_path, '#include <assert.h>\ntypedef float real;\n'+header+
          '\nstatic int cases[][5]={'+array+'};\n'+r'''
static void near(real a,real b) {assert(a-b<0.0001f && b-a<0.0001f);}
int main(void) {
    int i,j;real scales[]={1,.75f,20.0f/26,15.0f/26};
    for(i=0;i<8;i++) for(j=0;j<4;j++) {
        int *c=cases[i];real scale=scales[j],x=104,y=80;
        real w=(i<2?44:32)*scale,h=(i<2?33:34)*scale;
        real cw=i<2?400:288,ch=i<2?280:320;
        hud_button_control_bounds((short)c[0],&x,&y,&w,&h);
        near(x+c[1]*w/cw,104);near(y+c[2]*h/ch,80);
        near((c[3]-c[1])*w/cw,(i<2?44:32)*scale);
        near((c[4]-c[2])*h/ch,(i<2?33:34)*scale);
    }
    for(i=0;i<30;i++) {
        assert(hud_button_control_slot(14,i)==8+2*(i%3));
        assert(hud_button_control_slot(15,i)==9+2*(i%3));
        assert(hud_button_control_slot(16,i)==8);
        assert(hud_button_control_slot(17,i)==9);
    }
    for(i=8;i<14;i++) assert(hud_button_control_slot((short)i,0)==-1);
    return 0;
}
''')


def test_texture_capacity_covers_all_embedded_button_assets():
    import re
    import embed_assets
    source=(ROOT/'port/linux/src/hud_hires.c').read_text()
    capacity=int(re.search(r'#define MAXIMUM_TEXTURES (\d+)',source)[1])
    entries=list(embed_assets.textures())
    assert len(entries)<=capacity
    assert any(asset['name']=='thumbstick-full_right__0' for _,asset,_ in entries)
