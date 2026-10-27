"""Production controller identity, preferences, pending menu state and glyph layout."""
import json
from pathlib import Path
import re

import pytest
from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / 'port/linux/include/controller_buttons.h').read_text()


def without_includes(source):
    return '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))


@pytest.mark.parametrize('android', [False, True])
def test_identity_preferences_disconnect_and_unknown(tmp_path, android):
    code = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef enum {SDL_GAMEPAD_TYPE_UNKNOWN,SDL_GAMEPAD_TYPE_STANDARD,
 SDL_GAMEPAD_TYPE_XBOX360,SDL_GAMEPAD_TYPE_XBOXONE,SDL_GAMEPAD_TYPE_PS3,
 SDL_GAMEPAD_TYPE_PS4,SDL_GAMEPAD_TYPE_PS5,SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO,
 SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT,SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT,
 SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR} SDL_GamepadType;
typedef struct {SDL_GamepadType type;int id;} SDL_Gamepad;
static SDL_GamepadType SDL_GetGamepadType(SDL_Gamepad *p){return p->type;}
static char const *SDL_GetGamepadName(SDL_Gamepad *p){return "Test controller";}
static unsigned SDL_GetGamepadID(SDL_Gamepad *p){return p->id;}
static unsigned short SDL_GetGamepadVendor(SDL_Gamepad *p){return 123;}
static unsigned short SDL_GetGamepadProduct(SDL_Gamepad *p){return 456;}
static int automatic;
static char const *manual="ps_modern",*xbox="menu_gems",*ps="ps_gems",*sw="switch_modern";
static int config_boolean(char const *s){assert(!strcmp(s,"input.auto_button_detection"));return automatic;}
static char const *config_string(char const *s){
 if(!strcmp(s,"display.hud_button_style"))return manual;
 if(!strcmp(s,"display.hud_button_style_xbox"))return xbox;
 if(!strcmp(s,"display.hud_button_style_ps"))return ps;
 assert(!strcmp(s,"display.hud_button_style_switch"));return sw;
}
'''
    if android:
        code += '#define HALO_ARM64_GUEST 1\n'
    code += HEADER + without_includes((ROOT / 'port/linux/src/controller_buttons.c').read_text()) + r'''
int main(void){
 SDL_Gamepad a={SDL_GAMEPAD_TYPE_PS5,7},b={SDL_GAMEPAD_TYPE_XBOXONE,9},c={SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO,11};
 struct controller_device info;int type;
 controller_device_update(0,&a);controller_device_update(1,&b);controller_device_update(2,&c);
 controller_device_get(0,&info);assert(info.family==CONTROLLER_PLAYSTATION && info.type==SDL_GAMEPAD_TYPE_PS5);
#ifndef HALO_ARM64_GUEST
 assert(info.instance==7 && info.vendor==123 && info.product==456 && !strcmp(info.name,"Test controller"));
#endif
 assert(controller_button_style_for_port(0)==7 && controller_button_style_for_port(1)==7);
 automatic=1;
 assert(controller_button_style_for_port(0)==5 && controller_button_style_for_port(1)==2 && controller_button_style_for_port(2)==10);
 ps="ps_dark_gems";assert(controller_button_style_for_port(0)==6);
 automatic=0;assert(controller_button_style_for_port(0)==7);automatic=1;
 controller_device_update(0,NULL);controller_device_get(0,&info);assert(!info.family && !info.name[0] && !info.instance);
 assert(controller_button_style_for_port(0)==7);controller_device_update(0,&a);assert(controller_button_style_for_port(0)==6);
 for(type=0;type<=10;type++){
  a.type=(SDL_GamepadType)type;controller_device_update(0,&a);controller_device_get(0,&info);
  assert(info.family==(type<2?0:type<4?1:type<7?2:3));
 }
 controller_device_get(-1,&info);assert(!info.family);controller_device_get(4,&info);assert(!info.family);
 ps="xbox_modern";a.type=SDL_GAMEPAD_TYPE_PS3;controller_device_update(0,&a);
 assert(controller_button_style_for_port(0)==4);
 assert(controller_button_style_index("dark_gems")==2 && controller_button_style_index("bad")==0);
 assert(controller_button_style_family(-1)==0 && controller_button_style_family(11)==0);
 return 0;
}
'''
    run_c(tmp_path, code)


def test_pending_menu_filter_visibility_and_focus(tmp_path):
    code = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define usnprintf swprintf
#define FALSE 0
typedef int boolean;
struct widget_instance {
 char const *name;short vertical_offset;int visible,definition_tag_index;
 struct widget_instance *child,*next,*parent,*focused_child;
 struct {struct {short selected_index;}list;}parameters;
};
struct pc_menu_setting {int value_count;char const *values[11];};
static struct pc_menu_setting settings[]={
 {2,{"true","false"}},
 {11,{"classic","gems","menu_gems","xbox_modern","ps_classic","ps_gems","ps_dark_gems","ps_modern","switch_classic","switch_gems","switch_modern"}},
 {4,{"classic","gems","menu_gems","xbox_modern"}},
 {4,{"ps_classic","ps_gems","ps_dark_gems","ps_modern"}},
 {3,{"switch_classic","switch_gems","switch_modern"}}};
static struct widget_instance automatic,labels[4],styles[4];static int family;
static struct pc_menu_setting *pc_menu_setting_get(int id){return &settings[id];}
static struct widget_instance *named(struct widget_instance *list,char const *name,int n){
 struct widget_instance *p;int i;
 char const *names[]={"hud_button_style_spinner","hud_button_style_xbox_spinner","hud_button_style_ps_spinner","hud_button_style_switch_spinner"};
 if(!strcmp(name,"auto_button_detection_spinner"))return &automatic;
 for(i=0;i<4;i++)if(!strcmp(name,names[i]))return &styles[i];
 for(p=list->child;p;p=p->next)if(!strcmp(p->name,name))return p;
 return 0;
}
static short controller_of(struct widget_instance *w){return 1;}
static void text_set(struct widget_instance *w,wchar_t const *s){assert(!wcscmp(s,L"BUTTON STYLE (P2):"));}
'''
    code += HEADER + function((ROOT / 'port/linux/src/controller_buttons.c').read_text(), 'controller_button_style_index') + r'''
void controller_device_get(short port,struct controller_device *device){assert(port==1);device->family=family;}
''' + (ROOT / 'port/linux/game/controller_button_menu.inl').read_text() + r'''
int main(void){
 struct widget_instance list={0},a={0},r[4]={{0}},bumper={0},bar={0};int i,j,k,repeat;
 char const *names[]={"op_hud_button_style","op_hud_button_style_xbox","op_hud_button_style_ps","op_hud_button_style_switch"};
 list.child=&a;a.next=&r[0];a.name="op_auto_button_detection";
 a.visible=1;automatic.parent=&a;
 for(i=0;i<4;i++){r[i].name=names[i];r[i].next=i<3?&r[i+1]:&bumper;r[i].visible=1;r[i].child=&labels[i];r[i].vertical_offset=103;styles[i].definition_tag_index=i+1;}
 bumper.name="op_hud_bumper_style";bumper.next=&bar;bumper.vertical_offset=133;
 bar.name="button_bar";bar.vertical_offset=414;
 for(repeat=0;repeat<40;repeat++)for(k=0;k<4;k++)for(j=0;j<settings[k+1].value_count;j++){
  family=k;automatic.parameters.list.selected_index=0;styles[k].parameters.list.selected_index=j;
  list.focused_child=&r[(k+1)%4];controller_button_rows_show(&list);
  for(i=0;i<4;i++)assert(r[i].visible==(i==k));assert(list.focused_child==&r[k]);
  assert(list.parameters.list.selected_index==1+k);
  assert(bumper.visible==(k<2&&j<3));
  list.focused_child=&bumper;controller_button_rows_show(&list);
  assert(list.focused_child==(bumper.visible?&bumper:&r[k]));
  list.focused_child=&r[k];
  automatic.parameters.list.selected_index=1;controller_button_rows_show(&list);
  assert(r[0].visible && list.focused_child==&r[0]);
  assert(styles[k].parameters.list.selected_index==j);
  assert(bumper.visible==(styles[0].parameters.list.selected_index<3));
  assert(a.visible && bumper.vertical_offset==133 && bar.vertical_offset==414);
 }
 styles[0].parameters.list.selected_index=-1;controller_button_rows_show(&list);assert(!bumper.visible);
 styles[0].parameters.list.selected_index=11;controller_button_rows_show(&list);assert(!bumper.visible);
 automatic.parameters.list.selected_index=0;family=99;controller_button_rows_show(&list);assert(r[0].visible);
 return 0;
}
'''
    run_c(tmp_path, code)


def test_artwork_mapping_transparency_labels_and_regeneration():
    import numpy as np
    from PIL import Image
    import controller_button_assets as assets
    entries = assets.sources()
    assert len(entries) == 62
    assert [e['source'] for e in entries[20:24]] == [f'switch/{b}-switch_simple.png' for b in 'BAYX']
    assert [e['label'] for e in entries if e.get('label')][:4] == ['R1','L1','L2','R2']
    for entry in entries:
        expected = assets.render(entry)
        actual = Image.open(assets.ASSETS / (entry['name'] + '.png'))
        assert np.array_equal(np.asarray(expected), np.asarray(actual)), entry['name']
        bounds = actual.getchannel('A').getbbox()
        assert bounds and bounds[0] > 0 and bounds[1] > 0 and bounds[2] < 512 and bounds[3] < 512


def test_menu_generated_and_default_on():
    import port_settings
    for name, lines in port_settings.settings_files().items():
        assert (ROOT / 'port/assets/menus/ce' / name).read_text() == '\n'.join(lines)
    # Dynamic captions must not be overwritten or resized to the static string
    # by ui_widget's text update before the next frame.
    import xml.etree.ElementTree as ET
    prefix = 'main_menu.settings_select.player_setup.player_profile_edit'
    menu = ET.parse(ROOT / f'port/assets/menus/ce/{prefix}.controller_appearance.xml')
    labels = [w for w in menu.getroot().findall('widget')
              if '/hud_button_style' in w.get('name', '') and w.get('name', '').endswith('_label')]
    assert len(labels) == 4
    assert all('string_list' not in w.attrib and 'string_index' in w.attrib for w in labels)
    rows = port_settings.SCREENS['controller_appearance']['rows']
    assert len(next(r[2] for r in rows if r[1] == 'display.hud_button_style')) == 11
    assert next(r[2] for r in rows if r[1] == 'display.hud_bumper_style') == [('ORIGINAL XBOX', 'original'), ('MODERN XBOX', 'modern')]
    parent = ET.parse(ROOT / f'port/assets/menus/ce/{prefix}.gamepad_setup.xml')
    assert any('controller_appearance_screen' in e.get('open', '') for e in parent.iter('on'))
    assert not any(r[1] in ('input.auto_button_detection', 'display.hud_bumper_style') or r[1].startswith('display.hud_button_style')
                   for r in port_settings.SCREENS['video_settings/graphics']['rows'])
    assert f'ce/{prefix}.controller_appearance.xml' in json.loads((ROOT / 'port/assets/menus/menus.json').read_text())['files']
    config = (ROOT / 'port/linux/src/port_config.c').read_text()
    assert re.search(r'"input.auto_button_detection", _config_boolean, "true"', config)
    assert r'"display.hud_bumper_style", _config_string, "\"original\""' in config


def test_real_glyph_geometry_opacity_cache_and_teardown(tmp_path):
    source = (ROOT / 'port/linux/game/controller_glyphs.c').read_text()
    metadata = (ROOT / 'port/linux/game/controller_glyph_assets.h').read_text()
    source = source.replace('#include "controller_glyph_assets.h"', metadata)
    source = without_includes(source)
    code = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define NONE (-1)
#define TRUE 1
#define FALSE 0
typedef int boolean;typedef float real;typedef unsigned long pixel32;
typedef struct {short x0,y0,x1,y1;}rectangle2d;
struct bitmap_data {unsigned long *hardware_format;};
struct icon_hud_element_definition {short frame_rate;unsigned flags;pixel32 color;};
struct dynamic_screen_vertex {struct {real x,y;}position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
 struct bitmap_data *map[3];struct {real i,j;}map_texture_scale[3],map_scale[3];int framebuffer_blend_function;
};
static struct {short local_player_index;}render;
static struct player_datum {short local_player_index;}player;
static long local_player_get_player_index(short p){return p==2?NONE:123;}
static struct player_datum *player_get(long p){assert(p==123);return &player;}
static short observed_port;
static int controller_button_style_for_port(short p){observed_port=p;return 4;}
static char const *bumper_style="original";
static char const *config_string(char const *s){assert(!strcmp(s,"display.hud_bumper_style"));return bumper_style;}
static int players=1,allocations,live,draws,fail;static long ticks,last_asset;
static struct dynamic_screen_vertex drawn[4];
static long game_time_get(void){return ticks;}
static int local_player_count(void){return players;}
static real draw_unicode_string_capital_middle(rectangle2d *r,void const *s){return 100;}
static struct bitmap_data *bitmap_2d_new(int w,int h,int m,int f){
 struct bitmap_data *b;if(fail==1)return NULL;assert(w==64&&h==64&&!m&&f==11);
 b=calloc(1,sizeof(*b));allocations++;live++;return b;
}
static int rasterizer_bitmap_new(struct bitmap_data *b){b->hardware_format=(unsigned long*)b;return fail!=2;}
static void bitmap_delete(struct bitmap_data *b){free(b);live--;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *p,
 struct dynamic_screen_vertex *v){assert(p->framebuffer_blend_function==0);memcpy(drawn,v,sizeof(drawn));draws++;}
'''
    code += (ROOT / 'port/linux/game/hud_button_layout.h').read_text() + source + r'''
long hud_hires_asset_count(void){return NUMBEROF(controller_glyph_assets);}
char const *hud_hires_asset_tag(long a){return controller_glyph_assets[a].tag;}
void hud_hires_register_placeholder(long a,unsigned long const *p){if(p)last_asset=a;}
static void near(real a,real b){assert(a-b<0.001f && b-a<0.001f);}
int main(void){
 struct icon_hud_element_definition icon={10,0,0};rectangle2d cursor;
 int style,index,frame,i,before;real scale,x,y,w,h,cellw,cellh;short const *bounds;
 render.local_player_index=0;player.local_player_index=3;assert(controller_glyph_style()==4 && observed_port==3);
 render.local_player_index=2;controller_glyph_style();assert(observed_port==2);
 bumper_style="modern";
 for(players=1;players<=2;players++)for(style=0;style<11;style++)for(index=0;index<18;index++)for(frame=0;frame<3;frame++){
  cursor.x0=100;cursor.y0=50;cursor.x1=500;cursor.y1=150;ticks=frame*10;
  if((style<3 && index!=4 && index!=5) || (index>=8 && index<14)){before=draws;assert(!controller_glyph_draw(style,index,&icon,&cursor,0x7fffffff));assert(draws==before&&cursor.x0==100);continue;}
  assert(controller_glyph_draw(style,index,&icon,&cursor,0x7fffffff));assert(drawn[0].color==0x7fffffff);
  scale=players==1?1:.75f;
  bounds=controller_glyph_assets[controller_glyph_asset_index(style,index<6?index:hud_button_control_slot(index,frame))].bounds;
  w=drawn[2].position.x-drawn[0].position.x;h=drawn[2].position.y-drawn[0].position.y;
  x=drawn[0].position.x+(bounds[0]+bounds[2])*w/1024;
  y=drawn[0].position.y+(bounds[1]+bounds[3])*h/1024;
  cellw=(index<6?21:index<8?44:32)*scale;cellh=(index<6?26:index<8?33:34)*scale;
  near(x,104+(scale-1)*4+cellw/2);near(y,100);
  assert((bounds[2]-bounds[0])*w/512<=cellw+.001f && (bounds[3]-bounds[1])*h/512<=cellh+.001f);
  assert(cursor.x0==100+(short)((index<4?22:index<6?20:index<8?45:33)*scale));
  before=allocations;icon.flags=2;icon.color=0x40123456;
  assert(controller_glyph_draw(style,index,&icon,&cursor,0xffffffff));assert(drawn[0].color==0x40ffffff&&before==allocations);icon.flags=0;
 }
 assert(live==62 && allocations==62);controller_glyphs_dispose();assert(!live);
 controller_glyphs_dispose();assert(!live);
 /* Only black/RB and white/LB change. All three styles share the modern
  * artwork cache, including after preference changes and map teardown. */
 for(i=0;i<10;i++)for(style=0;style<3;style++)for(index=0;index<18;index++){
  bumper_style="original";assert(!controller_glyph_draw(style,index,&icon,&cursor,0));
  bumper_style="modern";before=draws;
  assert(controller_glyph_draw(style,index,&icon,&cursor,0)==(index==4||index==5));
  if(index==4||index==5){assert(draws==before+1);assert(controller_glyph_asset_index(style,index)==32+index-4);}
 }
 assert(live==2);controller_glyphs_dispose();assert(!live);
 bumper_style="invalid";assert(!controller_glyph_draw(0,4,&icon,&cursor,0));
 bumper_style="original";
 for(i=1;i<=2;i++){fail=i;assert(!controller_glyph_draw(4,0,&icon,&cursor,0));assert(!live);controller_glyphs_dispose();}
 fail=0;assert(controller_glyph_draw(4,0,&icon,&cursor,0));assert(drawn[0].color==0xffffff);
 for(style=-1;style<3;style++)assert(!controller_glyph_draw(style,0,&icon,&cursor,0));
 assert(!controller_glyph_draw(11,0,&icon,&cursor,0));controller_glyphs_dispose();assert(!live);
 return 0;
}
'''
    run_c(tmp_path, code)


def test_classic_letters_are_transparent_cutouts():
    import numpy as np
    from PIL import Image
    from scipy.ndimage import binary_fill_holes
    for letter in 'abxy':
        rgba = np.asarray(Image.open(ROOT / f'port/assets/buttons/classic_{letter}__0.png'))
        solid = rgba[..., 3] >= 128
        holes = binary_fill_holes(solid) & ~solid
        assert np.count_nonzero(holes & (rgba[..., 3] == 0)) > 500
        # No painted black letter remains on any of the coloured faces.
        assert np.min(np.max(rgba[..., :3][solid], axis=1)) > 100
