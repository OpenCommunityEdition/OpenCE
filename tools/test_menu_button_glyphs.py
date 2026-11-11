"""Production menu rendering: opt-in boundaries, fitting, fallback and state."""
import re
from pathlib import Path
from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_menu_glyph_policy_fitting_and_state(tmp_path):
    source = (ROOT/'port/linux/game/menu_button_glyphs.c').read_text()
    source = re.sub(r'^#include .*$', '', source, flags=re.M)
    renderer = (ROOT/'port/linux/game/keyboard_glyphs.c').read_text()
    constants = '\n'.join(re.findall(r'^#define KEYBOARD_GLYPH_COMPACT_.*$', (ROOT/'port/linux/game/keyboard_glyphs.h').read_text(), re.M))
    source = constants + '\n' + ''.join(function(renderer, name) for name in ('keyboard_glyph_draw_padded', 'keyboard_glyph_draw', 'keyboard_glyph_draw_compact')) + source
    run_c(tmp_path, r"""
#include <assert.h>
#include <string.h>
#include <wchar.h>
#define TRUE 1
#define FALSE 0
#define NONE -1
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
typedef int boolean;typedef float real;typedef unsigned long pixel32;
typedef struct {short x0,y0,x1,y1;}rectangle2d;
typedef struct {real x0,y0,x1,y1;}real_rectangle2d;
typedef struct {real alpha,red,green,blue;}real_argb_color;
struct bitmap_data {int dummy;};
struct dynamic_screen_vertex {struct {real x,y;}position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
 struct bitmap_data *map[3];struct {real i,j;}map_texture_scale[3],map_scale[3];int framebuffer_blend_function;
};
static int enabled=1,keyboard=1,style_choice=7,fail,backgrounds,labels,quads;
static short observed_port;
static char const *menus="pc";
static struct bitmap_data bitmap;
static long current_font=20;
static short current_style=2,current_justification=1,initial_indent=3,paragraph_indent=4;
static unsigned long current_flags=5;
static real_argb_color current_color={.5f,.2f,.3f,.4f};
static real bx,by,bw,bh,tx,ty,ts;
static wchar_t last_text[32];
static struct dynamic_screen_vertex drawn[4];
int config_boolean(char const *n){assert(!strcmp(n,"display.menu_button_glyphs"));return enabled;}
char const *config_string(char const *n){assert(!strcmp(n,"display.menus"));return menus;}
static int keyboard_prompt_enabled(short p){observed_port=p;return keyboard;}
static int controller_button_style_for_port(short p){observed_port=p;return style_choice;}
static long tag_loaded(long tag,char const *name){assert(!strcmp(name,"ui\\small_ui"));return fail==1?NONE:9;}
static long draw_string_get_font(void){return current_font;}
static void draw_string_get_color(real_argb_color *c){*c=current_color;}
static void draw_string_set_color(real_argb_color const *c){current_color=*c;}
static void draw_string_get_format(short *s,short *j,unsigned long *f){*s=current_style;*j=current_justification;*f=current_flags;}
static void draw_string_get_indents(short *i,short *p){*i=initial_indent;*p=paragraph_indent;}
static void draw_string_set_draw_mode(long f,short s,short j,unsigned long flags,real_argb_color const *c){
 current_font=f;current_style=s;current_justification=j;current_flags=flags;current_color=*c;}
static void draw_string_set_indents(short i,short p){initial_indent=i;paragraph_indent=p;}
static void draw_string_set_format(short s,short j,unsigned long f){current_style=s;current_justification=j;current_flags=f;}
static real text_left,text_right;static int text_runs;
static void rasterizer_draw_unicode_string_offset(rectangle2d const *r,wchar_t const *s,real x,real y){
 if(current_font==9){labels++;wcscpy(last_text,s);tx=x;ty=y+r->y0;ts=1;}
 else {if(!text_runs)text_left=x+2.5f;text_right=x+2.5f+(real)wcslen(s)*6;text_runs++;assert(!y);}}
static void rasterizer_draw_unicode_string_scaled_offset(rectangle2d const *r,wchar_t const *s,real scale,real x,real y){
 assert(current_font==9);labels++;wcscpy(last_text,s);tx=x;ty=y+r->y0*scale;ts=scale;}

static void rasterizer_unicode_string_ink_bounds(rectangle2d const *r,wchar_t const *s,real_rectangle2d *ink){
 assert((current_font==9||current_font==20)&&current_style==NONE&&!current_justification&&!current_flags&&!initial_indent&&!paragraph_indent);
 ink->x0=2.5f;ink->x1=2.5f+(real)wcslen(s)*6;ink->y0=r->y0+4;ink->y1=r->y0+14;
 if(fail==3)ink->x1=ink->x0;}
static int keyboard_glyph_background(int s,real x,real y,real w,real h,pixel32 color){
 assert(!s&&(color==0x802896ffUL||color==0x7f2896ffUL));bx=x;by=y;bw=w;bh=h;backgrounds++;return fail!=2;}
static short draw_unicode_string_capital_middle(rectangle2d const *r,wchar_t const *s){return 100;}
struct bitmap_data *hud_hires_button_bitmap(short s,short slot){assert(s<3&&slot>=0&&slot<4);return fail?NULL:&bitmap;}
static struct bitmap_data *controller_glyph_bitmap(short s,short slot,real_rectangle2d *uv){
 assert(s>=3&&s<=10&&slot>=0&&slot<4);uv->x0=uv->y0=.25f;uv->x1=.75f;uv->y1=.875f;return fail?NULL:&bitmap;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *p,
 struct dynamic_screen_vertex *v){assert(p->map[0]==&bitmap&&!p->framebuffer_blend_function);memcpy(drawn,v,sizeof(drawn));quads++;}
""" + (ROOT/'port/linux/game/hud_button_layout.h').read_text() + source + r"""
static void restored(void){
 assert(current_font==20&&current_style==2&&current_justification==1&&current_flags==5);
 assert(initial_indent==3&&paragraph_indent==4&&current_color.alpha==.5f&&current_color.red==.2f);}
int main(void){
 rectangle2d box={100,80,120,106},clip={0,0,640,480},cursor;
 short b,s;int before;
 assert(!wcscmp(menu_button_glyph_join_hint(0,0),L""));
 assert(!wcscmp(menu_button_glyph_join_hint(1,0),L"SPACE: join"));
 enabled=0;assert(!menu_button_glyph_draw(0,0,&box,0,0x80ffffff));
 assert(!wcscmp(menu_button_glyph_join_hint(0,0),L"ENTER: select    ESC: back"));
 enabled=1;menus="xbox";assert(!menu_button_glyph_draw(0,0,&box,0,0x80ffffff));
 menus="pc";assert(!menu_button_glyph_draw(-1,0,&box,0,0));assert(!menu_button_glyph_draw(4,0,&box,0,0));
 assert(!menu_button_glyph_draw(0,-1,&box,0,0));assert(!menu_button_glyph_draw(0,4,&box,0,0));
 assert(!backgrounds&&!quads&&!labels);
 for(b=0;b<4;b++){
  assert(menu_button_glyph_draw(b,2,&box,&clip,0x80ffffff));assert(observed_port==2);restored();
  assert(bx+bw==120&&bh==20&&by==83&&bw==(b?30:42));
  assert(ts>0&&ts<=1&&tx+2.5f*ts>=bx+2.9f&&ty+4*ts>=by);
  assert(tx+(2.5f+(real)wcslen(last_text)*6)*ts<=bx+bw-2.9f&&ty+14*ts<=by+bh);
 }
 assert(!wcscmp(last_text,L"Tab"));
 box.y1=96;assert(menu_button_glyph_draw(0,0,&box,&clip,0x80ffffff));assert(bh==20);restored();
 box.x1=116;box.y1=100;
 assert(menu_button_glyph_draw(0,0,&box,&clip,0x80ffffff));
 assert(bw==33.5f&&bh==20&&ts==.85f);restored();
 assert(tx+2.5f*ts>=bx+3.99f&&tx+32.5f*ts<=bx+bw-3.99f);
 assert(ty+4*ts>=by+5&&ty+14*ts<=by+bh-5);
 assert(menu_button_glyph_compact_key_width(0)==34);
 assert(menu_button_glyph_draw(1,0,&box,&clip,0x80ffffff));restored();
 assert(ts==.85f&&bw>23.29f&&bw<23.31f);
 assert(tx+2.5f*ts>=bx+3.99f&&tx+20.5f*ts<=bx+bw-3.99f);
 box.x1=120;
 box.y1=106;clip.x0=105;before=backgrounds;assert(!menu_button_glyph_draw(0,0,&box,&clip,0x80ffffff));assert(backgrounds==before);
 clip.x0=0;fail=1;assert(!menu_button_glyph_draw(1,0,&box,&clip,0x80ffffff));restored();
 fail=2;assert(menu_button_glyph_draw(1,0,&box,&clip,0x80ffffff));restored();fail=0;
 cursor=box;assert(menu_button_glyph_inline(0,1,&cursor));assert(cursor.x0==146&&bx==104&&bw==42);restored();
 assert(!menu_button_glyph_inline(18,1,&cursor));assert(cursor.x0==146);
 keyboard=0;assert(!wcscmp(menu_button_glyph_join_hint(1,0),L"SPACE: join    ENTER: select    ESC: back"));
 for(s=0;s<11;s++)for(b=0;b<4;b++){
  style_choice=s;assert(menu_button_glyph_draw(b,3,&box,&clip,0x80ffffff));assert(observed_port==3);
  assert(drawn[0].position.x>=100&&drawn[2].position.x<=120&&drawn[0].position.y>=80&&drawn[2].position.y<=106);
  assert(drawn[0].color==0x80ffffff&&drawn[2].texture_coordinates.x<=1&&drawn[2].texture_coordinates.y<=1);
 }
 fail=1;before=quads;assert(!menu_button_glyph_draw(0,0,&box,0,0));assert(quads==before);
 fail=0;box.x1=355;box.y1=110;
 for(keyboard=0;keyboard<2;keyboard++){
  text_runs=0;assert(menu_button_glyph_join_draw(L"Press %a-button to Join",0,&box));restored();
  assert(text_runs==2&&(text_left+text_right)*.5f==227.5f);
 }
 assert(!menu_button_glyph_join_draw(L"No icon",0,&box));
 assert(!menu_button_glyph_join_draw(L"Press %a-button %b-button",0,&box));
 menus="xbox";assert(!menu_button_glyph_join_draw(L"Press %a-button to Join",0,&box));restored();
 return 0;
}
""")


def test_menu_toggle_defaults_off_and_generator_owns_screens():
    config=(ROOT/'port/linux/src/port_config.c').read_text()
    assert '"display.menu_button_glyphs", _config_boolean, "false"' in config
    from port_settings import settings_files
    for name,lines in settings_files().items():
        if 'controller_appearance' in name:
            assert (ROOT/'port/assets/menus/ce'/name).read_text()=='\n'.join(lines)
            assert 'MENU GLYPHS:' in '\n'.join(lines)


def test_legend_expansion_moves_text_and_hit_target_together(tmp_path):
    source=(ROOT/'port/linux/game/menu_functions.c').read_text()
    run_c(tmp_path,r"""
#include <assert.h>
#include <string.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define NONE -1
#define TRUE 1
typedef int boolean;
typedef struct {short x0,y0,x1,y1;}rectangle2d;
struct widget_instance {struct widget_instance *parent,*child,*next;short horizontal_offset;long definition_tag_index;};
static int active=1,small=0;
static char const *names[]={"ui\\shell\\main_menu\\button_key","ui\\shell\\main_menu\\b_butn",
 "ui\\shell\\main_menu\\=back","ui\\shell\\main_menu\\a_butn","ui\\shell\\main_menu\\=accept","unrelated"};
static char const *tag_get_name(long t){return names[t];}
static short controller_of(struct widget_instance *w){return 0;}
static int menu_button_glyph_keyboard_active(short p){assert(!p);return active;}
static short menu_button_glyph_key_width(short b){return b?30:38;}
static short menu_button_glyph_compact_key_width(short b){return b?22:30;}
static void pc_menu_widget_text_bounds(long t,rectangle2d *b,short *o){assert(t>=1&&t<=3);b->x0=0;b->x1=t==2?63:small?16:21;*o=0;}
static int mcc_pause_owns(long t){return t==6;}
static short mcc_pause_key_offset(long parent,long child,short width,short back){assert(parent==6&&width==30&&back==22);return child>=3?9:-2;}
static short mcc_pause_key_button(long parent,long child){assert(parent==6);return child==1?1:child==3?0:NONE;}
"""+function(source,'menu_functions_glyph_button')+function(source,'menu_functions_glyph_offset')+r"""
int main(void){
 struct widget_instance parent={0},b={0},back={0},a={0},accept={0};
 parent.child=&b;b.next=&back;back.next=&a;a.next=&accept;
 b.parent=back.parent=a.parent=accept.parent=&parent;
 b.definition_tag_index=1;back.definition_tag_index=2;a.definition_tag_index=3;accept.definition_tag_index=4;
 b.horizontal_offset=371;back.horizontal_offset=393;a.horizontal_offset=471;accept.horizontal_offset=493;
 assert(menu_functions_glyph_offset(&b)==-17&&menu_functions_glyph_offset(&back)==-17);
 assert(!menu_functions_glyph_offset(&a)&&!menu_functions_glyph_offset(&accept));
 active=0;assert(!menu_functions_glyph_offset(&b));active=1;
 parent.definition_tag_index=5;assert(!menu_functions_glyph_offset(&b));
 parent.definition_tag_index=0;small=1;
 names[0]="ui\\shell\\main_menu\\button_key_sm";
 names[1]="ui\\shell\\main_menu\\b_butn_sm";names[3]="ui\\shell\\main_menu\\a_butn_sm";
 b.horizontal_offset=0;back.horizontal_offset=16;a.horizontal_offset=90;accept.horizontal_offset=107;
 assert(menu_functions_glyph_offset(&b)==-2&&menu_functions_glyph_offset(&back)==-2);
 assert(menu_functions_glyph_offset(&a)==9&&menu_functions_glyph_offset(&accept)==9);
 /* The imported caps remain inside the 226-wide panel, whose key starts at 25.
    The 63-wide CANCEL label ends before Enter; SELECT remains inside the box. */
 assert(25+16-22+menu_functions_glyph_offset(&b)>=0);
 assert(back.horizontal_offset+menu_functions_glyph_offset(&back)+63+8<=a.horizontal_offset+16+menu_functions_glyph_offset(&a)-30);
 assert(25+accept.horizontal_offset+menu_functions_glyph_offset(&accept)+63<=226);
 /* Confirmation dialogs retain the unsuffixed parent name and small slots. */
 names[0]="ui\\shell\\error\\confirm_buttons";
 a.horizontal_offset=78;accept.horizontal_offset=94;
 assert(back.horizontal_offset+menu_functions_glyph_offset(&back)+63+8<=a.horizontal_offset+16+menu_functions_glyph_offset(&a)-30);
 active=0;assert(!menu_functions_glyph_offset(&a));active=1;
 parent.definition_tag_index=6;assert(menu_functions_glyph_offset(&a)==9);
 assert(menu_functions_glyph_button(&a,NONE)==0);
 assert(menu_functions_glyph_button(&b,NONE)==1);
 assert(menu_functions_glyph_button(&back,NONE)==NONE);
 assert(menu_functions_glyph_button(&a,2)==2);
 active=0;assert(menu_functions_glyph_button(&a,NONE)==NONE);
 return 0;
}
""")
