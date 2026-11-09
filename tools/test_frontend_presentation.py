"""Production layout, input-source and binding contracts, independent of maps."""
from pathlib import Path
import re

from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def source(name):
    text = (ROOT / name).read_text().replace('#include "frontend_network_layout.inl"',
        (ROOT / 'port/linux/game/frontend_network_layout.inl').read_text())
    return re.sub(r'^#include .*$', '', text, flags=re.M)


def test_description_reflow_joins_native_split_words(tmp_path):
    implementation = function((ROOT / 'port/linux/game/frontend_layout.c').read_text(), 'frontend_menu_reflow_text')
    run_c(tmp_path, r'''
#include <assert.h>
#include <wchar.h>
''' + implementation + r'''
int main(void) {
    wchar_t text[256] = L"|cReactivate the wea-\r\npon at the heart of\r\nHalo...and learn the truth.";
    wchar_t marked[64] = L"|lwea-|npon|nready|r";
    wchar_t plain[64] = L"Co-op campaign |";
    wchar_t empty[1] = {0};
    frontend_menu_reflow_text(text);
    assert(!wcscmp(text,L"Reactivate the weapon at the heart of Halo...and learn the truth."));
    frontend_menu_reflow_text(text);
    assert(!wcscmp(text,L"Reactivate the weapon at the heart of Halo...and learn the truth."));
    frontend_menu_reflow_text(marked);assert(!wcscmp(marked,L"weapon ready"));
    frontend_menu_reflow_text(plain);assert(!wcscmp(plain,L"Co-op campaign |"));
    frontend_menu_reflow_text(empty);assert(!empty[0]);
    return 0;
}
''')


def test_wide_geometry_resize_and_scoped_text_clip(tmp_path):
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#define NONE -1
static long tag_loaded(long group,char const *name){(void)group;(void)name;return 9;}
static int available=1,screen=852;
int frontend_menu_available(void) {return available;}
char const *tag_get_name(long tag) {return tag==1?"pc\\modern\\help":"pc\\main_menu\\main_menu";}
long halo_screen_width(void) {return screen;}
''' + source('port/linux/game/frontend_layout.c') + r'''
int main(void) {
    int sizes[]={640,768,852,1120,1706,640},i,extra,rows,origin;
    short l,r,shift,cl,cr;
    for(i=0;i<6;i++) {
        screen=sizes[i];extra=frontend_layout_width(screen)-640;rows=extra*3/5;origin=-extra/2;
        l=0;r=640;shift=0;
        assert(frontend_layout_geometry("pc\\modern\\settings",screen,&l,&r,&shift));
        assert(l==-(screen-640)/2&&r-l==screen&&shift==0);
        l=0;r=364;assert(frontend_layout_geometry("pc\\modern\\row_style",screen,&l,&r,&shift));
        assert(shift==origin&&r==364+rows);
        /* Hit targets receive this same definition and accumulated origin. */
        assert(24+shift>=-(screen-640)/2&&24+shift+r<=400+origin+rows);
        l=0;r=152;assert(frontend_layout_geometry("pc\\modern\\spinner_style",screen,&l,&r,&shift));
        assert(shift==rows-rows/3&&r==152+rows/3);
        l=412;r=610;assert(frontend_layout_geometry("pc\\modern\\help",screen,&l,&r,&shift));
        assert(shift==origin+rows&&r-l==198+extra-rows);
        cl=0;cr=640;frontend_menu_clip_text(-1000,2000,&cl,&cr);assert(cl==0&&cr==640);
        frontend_menu_text_begin(1);frontend_menu_clip_text(-1000,2000,&cl,&cr);
        assert(cl==-(screen-640)/2&&cr==cl+screen);frontend_menu_text_end();
        cl=0;cr=640;frontend_menu_text_begin(2);frontend_menu_clip_text(-1000,2000,&cl,&cr);assert(cl==0&&cr==640);
        l=0;r=640;shift=0;assert(!frontend_layout_geometry("ui\\shell\\main_menu",screen,&l,&r,&shift));
        assert(l==0&&r==640&&shift==0);
        assert(!frontend_layout_geometry("pc\\modern\\entry",screen,&l,&r,&shift));
        l=0;r=272;shift=0;
        assert(frontend_layout_geometry("pc\\modern\\multiplayer\\row_host",screen,&l,&r,&shift));
        assert(shift==origin&&r==272+extra*9/20);
        l=0;r=302;shift=0;
        assert(frontend_layout_geometry("pc\\modern\\host\\help_panel",screen,&l,&r,&shift));
        assert(shift==origin+extra*9/20&&r==302+extra-extra*9/20);
    }
    available=0;cl=0;cr=640;frontend_menu_text_begin(1);frontend_menu_clip_text(-1000,2000,&cl,&cr);
    assert(cl==0&&cr==640);assert(!frontend_menu_geometry(1,&cl,&cr,&shift));
    return 0;
}
''')


def test_option_font_is_uniform_across_rows_pages_and_resize(tmp_path):
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#define NONE -1
static int available=1,screen=640,fonts_loaded=1;
static char const *current_name;
static long tag_loaded(long group,char const *name){
    (void)group;
    if(!fonts_loaded)return NONE;
    return !strcmp(name,"ui\\small_ui")?10:20;
}
int frontend_menu_available(void){return available;}
char const *tag_get_name(long tag){(void)tag;return current_name;}
long halo_screen_width(void){return screen;}
''' + source('port/linux/game/frontend_layout.c') + r'''
int main(void){
    char const *names[]={"pc\\modern\\interface\\label_auto",
        "pc\\modern\\interface\\label_manual","pc\\modern\\interface\\spinner_manual",
        "pc\\modern\\lighting\\label_glow","pc\\modern\\browser\\master_servers_value"};
    int sizes[]={640,768,831,832,852,1120,1706,640},i,j;
    for(i=0;i<8;i++){
        screen=sizes[i];
        for(j=0;j<5;j++){
            current_name=names[j];
            assert(frontend_menu_font(1,7)==(screen<832?10:20));
        }
    }
    current_name="pc\\modern\\help_title";assert(frontend_menu_font(1,7)==7);
    current_name="pc\\main_menu\\label_auto";assert(frontend_menu_font(1,7)==7);
    current_name=names[0];available=0;assert(frontend_menu_font(1,7)==7);
    available=1;fonts_loaded=0;assert(frontend_menu_font(1,7)==7);
    current_name=NULL;assert(frontend_menu_font(1,7)==7);
    return 0;
}
''')


def test_input_edges_hotplug_touch_and_teardown(tmp_path):
    run_c(tmp_path, '#include <assert.h>\n#include <string.h>\n' + source('port/linux/src/frontend_input.c') + r'''
int main(void) {
    unsigned char keys[512]={0};int i;
    frontend_input_enable(1);assert(frontend_input_device()==-1);
    frontend_input_pad(0,0,1);assert(frontend_input_device()==-1);
    frontend_input_pad(2,1,1);assert(frontend_input_device()==2);
    keys[4]=1;frontend_input_keys(keys,512);assert(frontend_input_device()==-1);
    frontend_input_pad(2,1,1);assert(frontend_input_device()==-1); /* held is not new activity */
    frontend_input_pad(2,0,1);frontend_input_pad(2,2,1);assert(frontend_input_device()==2);
    frontend_input_keys(keys,512);assert(frontend_input_device()==2);
    frontend_input_pointer(0,0);assert(frontend_input_device()==2);
    frontend_input_pointer(1,1);assert(frontend_input_device()==-2);
    frontend_input_pad(3,4,1);assert(frontend_input_device()==3);
    frontend_input_pad(3,0,0);assert(frontend_input_device()==-2);
    frontend_input_pad(3,0,1);assert(frontend_input_device()==-2); /* reconnect idle */
    frontend_input_pad(-1,1,1);frontend_input_pad(4,1,1);assert(frontend_input_device()==-2);
    frontend_input_pointer(0,1);assert(frontend_input_device()==-1);
    for(i=0;i<100;i++) {
        frontend_input_enable(0);frontend_input_pad(0,1,1);frontend_input_pointer(1,1);
        assert(frontend_input_device()==-1);frontend_input_enable(1);
        frontend_input_pad(0,1,1);assert(frontend_input_device()==0);
    }
    return 0;
}
''')


def test_detached_help_retains_viewport_clip(tmp_path):
    text = (ROOT / 'source/interface/ui_widget.c').read_text()
    integration = text.split('/* port: a local modern definition supplies identical drawing and pointer bounds. */', 1)[1]
    integration = integration.split('if (!use_nifty_plasma_fx', 1)[0]
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
struct ui_widget_definition {rectangle2d bounds,list_footer_bounds;struct {long index;}text_font;};
struct widget_instance {int type;void *parent;long definition_tag_index;};
enum {_ui_widget_type_container,_ui_widget_type_text_box,_ui_widget_type_spinner_list};
static char const *current_name;
static int frontend_menu_available(void){return 1;}
static char const *tag_get_name(long tag){return current_name;}
static long halo_screen_width(void){return 852;}
#define NONE -1
static long tag_loaded(long group,char const *name){(void)group;(void)name;return 9;}
''' + source('port/linux/game/frontend_layout.c') + r'''
static void check(int type,short left,short right,short expected_right){
 struct widget_instance w={0,0,1},*widget=&w;
 struct ui_widget_definition original={{left,0,right,480}},*definition=&original;
 rectangle2d incoming={-106,0,746,480},*clip_rect=&incoming;
 struct {short x,y;}offset={0,0};
''' + integration.replace('frontend_layout = frontend_menu_geometry', 'w.type = type; frontend_layout = frontend_menu_geometry') + r'''
 assert(clip_rect->x1==expected_right);
 assert(original.bounds.x0==left&&original.bounds.x1==right);
}
int main(void){
 current_name="pc\\modern\\settings";check(_ui_widget_type_container,0,640,746);
 /* Extended descriptions have no parent but their local bounds are not a viewport. */
 current_name="pc\\modern\\help";check(_ui_widget_type_text_box,396,596,746);
 return 0;
}
''')


def test_panel_caps_preserve_shape_plasma_and_scope(tmp_path):
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
typedef float real;
struct dynamic_screen_vertex {struct {real x,y;}position,texture_coordinates;unsigned long color;};
struct rasterizer_dynamic_screen_geometry_parameters {int plasma;};
static int available=1,calls;
static char const *name="pc\\modern\\row_style";
static struct dynamic_screen_vertex drawn[3][4];
static struct rasterizer_dynamic_screen_geometry_parameters *expected;
static int frontend_menu_available(void){return available;}
static char const *tag_get_name(long tag){return name;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *p,struct dynamic_screen_vertex *v){
 assert(p==expected&&p->plasma==1&&calls<3);memcpy(drawn[calls++],v,sizeof(drawn[0]));
}
''' + source('port/linux/game/frontend_panel.c') + r'''
int main(void){
 struct rasterizer_dynamic_screen_geometry_parameters p={1};
 struct dynamic_screen_vertex v[4]={{{-80,100},{0,0},0x55ffffff},{{256,100},{1,0},0x55ffffff},
                                  {{256,120},{1,1},0x55ffffff},{{-80,120},{0,1},0x55ffffff}};
 int widths[]={336,463,720},i,j,k;
 expected=&p;
 assert(!frontend_panel_draw(&p,v,336));
 for(i=0;i<3;i++){
  calls=0;v[1].position.x=v[2].position.x=v[0].position.x+widths[i];
  frontend_panel_begin(1);assert(frontend_panel_draw(&p,v,336)&&calls==3);
  assert(drawn[0][1].position.x-drawn[0][0].position.x==18);
  assert(drawn[2][1].position.x-drawn[2][0].position.x==18);
  assert(drawn[0][0].position.x==v[0].position.x&&drawn[2][1].position.x==v[1].position.x);
  assert(drawn[0][1].texture_coordinates.x==18.0f/336);
  for(j=0;j<2;j++){
   assert(drawn[j][1].position.x==drawn[j+1][0].position.x);
   assert(drawn[j][1].texture_coordinates.x==drawn[j+1][0].texture_coordinates.x);
  }
  for(j=0;j<3;j++)for(k=0;k<4;k++){
   assert(drawn[j][k].position.y==v[k].position.y);
   assert(drawn[j][k].texture_coordinates.y==v[k].texture_coordinates.y);
   assert(drawn[j][k].color==v[k].color);
  }
  frontend_panel_end();assert(!frontend_panel_draw(&p,v,336));
 }
 name="pc\\modern\\help_panel";calls=0;frontend_panel_begin(1);assert(frontend_panel_draw(&p,v,232));
 name="ui\\shell\\main_menu";frontend_panel_begin(1);assert(!frontend_panel_draw(&p,v,336));
 name="pc\\modern\\title";frontend_panel_begin(1);assert(!frontend_panel_draw(&p,v,336));
 name="pc\\modern\\row_style";available=0;frontend_panel_begin(1);assert(!frontend_panel_draw(&p,v,336));
 available=1;frontend_panel_begin(1);assert(!frontend_panel_draw(&p,v,24));
 v[1].position.x=v[0].position.x+24;assert(!frontend_panel_draw(&p,v,336));
 return 0;
}
''')


def test_keyboard_labels_read_actual_binding_cache(tmp_path):
    text = (ROOT / 'port/linux/src/xinput_sdl.c').read_text()
    body = function(text, 'halo_keyboard_binding_label')
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define NUMBER_OF_HALO_KEYBOARD_ACTIONS 2
#define MAXIMUM_BINDINGS 2
static int bindings[2][2]={{4,5},{-1,6}},reads;
static void bindings_read(void) {reads++;}
static void halo_input_name(int input,char *name,unsigned int size) {snprintf(name,size,"%s",input==4?"Z":input==5?"Mouse Left":"Wheel Up");}
static void halo_input_shown_name(char const *name,char *shown,unsigned int size) {snprintf(shown,size,"%s",!strcmp(name,"Z")?"Y":name);}
''' + body + r'''
int main(void) {
    char label[64];
    assert(halo_keyboard_binding_label(0,0,label,sizeof(label))&&!strcmp(label,"Y"));
    assert(halo_keyboard_binding_label(0,1,label,sizeof(label))&&!strcmp(label,"Mouse Left"));
    assert(!halo_keyboard_binding_label(1,0,label,sizeof(label))&&!label[0]);
    assert(halo_keyboard_binding_label(1,1,label,sizeof(label))&&!strcmp(label,"Wheel Up"));
    bindings[0][0]=5;assert(halo_keyboard_binding_label(0,0,label,sizeof(label))&&!strcmp(label,"Mouse Left"));
    assert(reads==5);assert(!halo_keyboard_binding_label(-1,0,label,sizeof(label))&&!label[0]);
    assert(!halo_keyboard_binding_label(0,2,label,sizeof(label))&&!label[0]);
    assert(!halo_keyboard_binding_label(0,0,0,0));return 0;
}
''')


def test_menu_actions_are_independent_of_gameplay_remaps(tmp_path):
    text = (ROOT / 'port/linux/game/frontend_prompts.c').read_text()
    header = (ROOT / 'port/linux/game/frontend_prompts.h').read_text()
    enum = re.search(r'enum frontend_prompt_action.*?;', header).group()
    run_c(tmp_path, '#include <assert.h>\n#include <string.h>\n' + enum + '\n' +
          function(text, 'frontend_prompt_button') + function(text, 'frontend_prompt_key') + r'''
int main(void) {
    assert(frontend_prompt_button(FRONTEND_SELECT)==0);
    assert(frontend_prompt_button(FRONTEND_CANCEL)==1);
    assert(frontend_prompt_button(FRONTEND_STYLE)==3);
    assert(!strcmp(frontend_prompt_key(FRONTEND_SELECT),"Return"));
    assert(!strcmp(frontend_prompt_key(FRONTEND_CANCEL),"Escape"));
    assert(!strcmp(frontend_prompt_key(FRONTEND_STYLE),"Tab"));
    assert(frontend_prompt_button(99)==-1&&!frontend_prompt_key(99)[0]);return 0;
}
''')


def test_frontend_glyph_preferences_cache_failure_and_cleanup(tmp_path):
    code = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
typedef int boolean;typedef float real;typedef unsigned long pixel32;
typedef struct {short x0,y0,x1,y1;}rectangle2d;
#define csmemset memset
static void ce_hud_buttons_forget(void){}
struct bitmap_data {unsigned long *hardware_format;};
struct icon_hud_element_definition {int unused;};
struct dynamic_screen_vertex {struct {real x,y;}position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
 struct bitmap_data *map[3];struct {real i,j;}map_texture_scale[3],map_scale[3];int framebuffer_blend_function;
};
static int style,live,created,registered,fail,new_draws,draws;
static short observed_port;
static int controller_button_style_for_port(short p){observed_port=p;return style;}
static int controller_glyph_draw_menu(short s,short b,rectangle2d *r,pixel32 c){
 assert(s==style&&b>=0&&b<4);new_draws++;r->x0+=26;return 1;
}
static real draw_unicode_string_capital_middle(rectangle2d *r,void const *s){return 100;}
static struct bitmap_data *bitmap_2d_new(int w,int h,int m,int f){
 struct bitmap_data *b;assert(w==32&&h==32&&!m&&f==11);if(fail==1)return NULL;
 b=calloc(1,sizeof(*b));live++;created++;return b;
}
static int rasterizer_bitmap_new(struct bitmap_data *b){b->hardware_format=(unsigned long*)b;return fail!=2;}
static void bitmap_delete(struct bitmap_data *b){free(b);live--;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *p,struct dynamic_screen_vertex *v){
 assert(p->framebuffer_blend_function==0&&v[0].color==0x40ffffff);
 assert(v[2].position.x>v[0].position.x&&v[2].position.y>v[0].position.y);draws++;
}
'''
    code += (ROOT / 'port/linux/game/hud_button_layout.h').read_text()
    hud = (ROOT/'port/linux/game/hud_hires_tags.c').read_text()
    start = hud.index('static struct\n{\n\tstruct bitmap_data *bitmap;', hud.index('/* Original Xbox controller faces'))
    state = hud[start:hud.index('/* ---------- private code */', start)]
    code += 'long hud_hires_asset_count(void);\nchar const *hud_hires_asset_tag(long);\nvoid hud_hires_register_placeholder(long,unsigned long const *);\n'
    code += state + function(hud, 'hud_hires_button_bitmap')
    code += source('port/linux/game/frontend_glyphs.c') + r'''
long hud_hires_asset_count(void){return 12;}
char const *hud_hires_asset_tag(long a){static char const *names[]={
 "ui\\hud\\buttons\\classic_a","ui\\hud\\buttons\\classic_b","ui\\hud\\buttons\\classic_x","ui\\hud\\buttons\\classic_y",
 "ui\\hud\\buttons\\gem_a","ui\\hud\\buttons\\gem_b","ui\\hud\\buttons\\gem_x","ui\\hud\\buttons\\gem_y",
 "ui\\shell\\bitmaps\\a_butn","ui\\shell\\bitmaps\\b_butn","ui\\shell\\bitmaps\\x_butn","ui\\shell\\bitmaps\\y_butn"};return names[a];}
void hud_hires_register_placeholder(long a,unsigned long const *p){registered+=p?1:-1;}
int main(void){
 rectangle2d cursor={100,50,500,150};int repeat,before,button;
 for(repeat=0;repeat<10;repeat++)for(style=0;style<11;style++)for(button=0;button<4;button++) {
  cursor.x0=100;assert(frontend_glyph_draw(2,button,&cursor,0x40112233));assert(observed_port==2&&cursor.x0==126);
  if(style<3){before=created;assert(hud_hires_button_bitmap(style,button));assert(created==before);}
 }
 assert(live==4&&created==120&&registered==4&&draws==120&&new_draws==320);
 hud_buttons_forget();assert(!live&&!registered);hud_buttons_forget();assert(!live&&!registered);
 style=0;
 for(fail=1;fail<=2;fail++){
  before=created;assert(!frontend_glyph_draw(0,0,&cursor,0x40112233));assert(!live&&!registered);
  assert(!frontend_glyph_draw(0,0,&cursor,0x40112233));assert(created==before+(fail==2));hud_buttons_forget();
 }
 fail=0;assert(frontend_glyph_draw(0,0,&cursor,0x40112233));assert(live==1&&registered==1);
 assert(!frontend_glyph_draw(-1,0,&cursor,0));assert(!frontend_glyph_draw(4,0,&cursor,0));assert(!frontend_glyph_draw(0,4,&cursor,0));
 hud_buttons_forget();assert(!live&&!registered);return 0;
}
'''
    run_c(tmp_path, code)



def test_manual_controller_prompts_follow_hud_gate_and_auto_source(tmp_path):
    text = (ROOT/'port/linux/game/frontend_prompts.c').read_text()
    header = (ROOT/'port/linux/game/frontend_prompts.h').read_text()
    enum = re.search(r'enum frontend_prompt_action.*?;',header).group()
    run_c(tmp_path, r"""
#include <assert.h>
#include <string.h>
#define TRUE 1
typedef struct {short x0,y0,x1,y1;}rectangle2d;
typedef unsigned long pixel32;
typedef struct {float alpha,red,green,blue;}real_argb_color;
static int hud=1,automatic=1,keyboard,device=-1,observed=99,calls;
static int config_boolean(char const *n){return !strcmp(n,"display.high_res_hud")?hud:automatic;}
static int keyboard_prompt_menu_enabled(int d){(void)d;return keyboard;}
static int frontend_menu_available(void){return 1;}
static char const *tag_get_name(long tag){(void)tag;return "pc\\modern\\prompts";}
static int frontend_input_device(void){return device;}
static void draw_string_get_color(real_argb_color *c){c->alpha=1.0f;}
static int frontend_glyph_draw(short d,short b,rectangle2d *r,pixel32 c){(void)d;(void)b;(void)r;(void)c;return 1;}
static void keyboard_glyph_draw(char const *s,rectangle2d *r,pixel32 c,int m){(void)s;(void)r;(void)c;(void)m;}
static void frontend_prompt_text(rectangle2d *r,char const *s){(void)r;(void)s;}
static void frontend_prompt_action(rectangle2d *r,int d,int a,char const *s,pixel32 c){(void)r;(void)a;(void)s;(void)c;observed=d;calls++;}
"""+enum+function(text,'frontend_prompts_draw')+r"""
int main(void){
    rectangle2d bounds={0,0,640,22};
    frontend_prompts_draw(1,&bounds);assert(observed==-1&&calls==3);
    automatic=0;frontend_prompts_draw(1,&bounds);assert(observed==0);
    hud=0;frontend_prompts_draw(1,&bounds);assert(observed==-1);
    hud=1;keyboard=1;frontend_prompts_draw(1,&bounds);assert(observed==-1);
    automatic=1;keyboard=0;device=2;frontend_prompts_draw(1,&bounds);assert(observed==2);
    device=-2;calls=0;frontend_prompts_draw(1,&bounds);assert(!calls);
    return 0;
}
""")


def test_keyboard_footer_draws_keycaps_for_manual_and_detected_input(tmp_path):
    text = (ROOT/'port/linux/game/frontend_prompts.c').read_text()
    header = (ROOT/'port/linux/game/frontend_prompts.h').read_text()
    enum = re.search(r'enum frontend_prompt_action.*?;', header).group()
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#define TRUE 1
typedef struct {short x0,y0,x1,y1;} rectangle2d;
typedef unsigned long pixel32;
typedef struct {float alpha,red,green,blue;} real_argb_color;
static int automatic=1,hud=1,device=-1,keycaps,pads,plain_keys,touch_text;
static char const *manual="classic";
static int config_boolean(char const *n){return !strcmp(n,"display.high_res_hud")?hud:!strcmp(n,"input.auto_button_detection")?automatic:0;}
static char const *config_string(char const *n){(void)n;return manual;}
static int frontend_menu_available(void){return 1;}
static char const *tag_get_name(long tag){(void)tag;return "pc\\modern\\prompts";}
static int frontend_input_device(void){return device;}
static void draw_string_get_color(real_argb_color *c){c->alpha=1.0f;}
static int frontend_glyph_draw(short d,short b,rectangle2d *r,pixel32 c){(void)d;(void)b;(void)r;(void)c;pads++;return 1;}
static void halo_input_shown_name(char const *s,char *out,unsigned int n){(void)n;strcpy(out,s);}
static void keyboard_glyph_draw(char const *s,rectangle2d *r,pixel32 c,int m){(void)s;(void)r;(void)c;assert(m);keycaps++;}
static void frontend_prompt_text(rectangle2d *r,char const *s){
 (void)r;if(!strcmp(s,"Return")||!strcmp(s,"Escape")||!strcmp(s,"Tab")||!strcmp(s,"Arrows: MOVE"))plain_keys++;
 if(strstr(s,"Tap "))touch_text++;
}
''' + enum + function((ROOT/'port/linux/src/keyboard_prompts.c').read_text(), 'keyboard_prompt_menu_enabled') +
          ''.join(function(text, n) for n in ('frontend_prompt_button','frontend_prompt_key','frontend_prompt_action','frontend_prompts_draw')) + r'''
int main(void){
 rectangle2d bounds={0,0,640,22};int i;
 for(hud=0;hud<2;hud++) {
  automatic=1;device=-1;manual="classic";keycaps=pads=plain_keys=0;
  frontend_prompts_draw(1,&bounds);assert(keycaps==(hud?7:0)&&!pads);if(hud)assert(!plain_keys);
  automatic=0;manual="keyboard";
  for(i=-2;i<4;i++) {device=i;keycaps=pads=plain_keys=0;
   frontend_prompts_draw(1,&bounds);assert(keycaps==(hud?7:0)&&!pads);if(hud)assert(!plain_keys);
  }
  automatic=1;device=2;keycaps=pads=0;
  frontend_prompts_draw(1,&bounds);assert(!keycaps&&pads==(hud?4:0));
  device=-2;keycaps=pads=touch_text=0;
  frontend_prompts_draw(1,&bounds);assert(!keycaps&&!pads&&touch_text==1);
 }
 return 0;
}
''')
