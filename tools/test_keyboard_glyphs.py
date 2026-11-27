"""Production keyboard prompt selection, semantic mapping and drawing contracts."""
import re
import pytest
from pathlib import Path

from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def code(path):
    return re.sub(r'^#include .*$', '', (ROOT / path).read_text(encoding='utf-8'), flags=re.M)


@pytest.mark.parametrize("android", [False, True])
def test_manual_auto_activity_disconnect_and_split_screen(tmp_path, android):
    run_c(tmp_path, ("#define HALO_ANDROID 1\n" if android else "") + r'''
#include <assert.h>
#include <string.h>
static int automatic=1, enabled;
static char const *manual="classic";
int config_boolean(char const *n){if(!strcmp(n,"display.keyboard_glyphs"))return enabled;assert(!strcmp(n,"input.auto_button_detection"));return automatic;}
char const *config_string(char const *n){(void)n;return manual;}
''' + code('port/linux/src/keyboard_prompts.c') + r'''
int main(void){
 unsigned char keys[512]={0};
 assert(!keyboard_prompt_enabled(0));enabled=1;
#ifdef HALO_ANDROID
 assert(!keyboard_prompt_enabled(0));
#else
 assert(keyboard_prompt_enabled(0));
#endif
 keyboard_prompt_keys(keys,512,0x100);
 assert(keyboard_prompt_active(0)&&!keyboard_prompt_active(1));
 assert(keyboard_prompt_enabled(0)&&!keyboard_prompt_enabled(1));
 keyboard_prompt_pad(0,0,0,1);assert(!keyboard_prompt_enabled(0));
 keys[4]=1;keyboard_prompt_keys(keys,512,0);assert(keyboard_prompt_enabled(0));
 keyboard_prompt_pad(1,1,1,1);assert(keyboard_prompt_enabled(0));
 keyboard_prompt_pad(0,1,0,1);assert(!keyboard_prompt_enabled(0));
 keyboard_prompt_keys(keys,512,0);assert(!keyboard_prompt_enabled(0)); /* held key is not fresh input */
 keyboard_prompt_keys(keys,512,2);assert(keyboard_prompt_enabled(0));
 keyboard_prompt_pad(0,0,1,1);assert(!keyboard_prompt_enabled(0));
 keyboard_prompt_keys(keys,512,2);assert(!keyboard_prompt_enabled(0)); /* held mouse too */
 keyboard_prompt_keys(keys,512,0x100);assert(keyboard_prompt_enabled(0));
 keyboard_prompt_pad(0,0,1,1);assert(keyboard_prompt_enabled(0)); /* no stick jitter */
 keyboard_prompt_pad(0,0,0,1);keyboard_prompt_pad(0,0,1,1);assert(!keyboard_prompt_enabled(0));
 keyboard_prompt_pad(0,0,0,0);assert(keyboard_prompt_enabled(0));
 keyboard_prompt_touch();assert(!keyboard_prompt_enabled(0));
 keyboard_prompt_pad(0,0,0,0);assert(!keyboard_prompt_enabled(0));
 keyboard_prompt_pad(0,1,0,1);keyboard_prompt_pad(0,0,0,0);assert(!keyboard_prompt_enabled(0));
 keyboard_prompt_keys(keys,512,0x100);assert(keyboard_prompt_enabled(0));
 enabled=0;assert(!keyboard_prompt_enabled(0));
 automatic=0;assert(!keyboard_prompt_enabled(0));manual="keyboard";
 assert(keyboard_prompt_enabled(0)&&keyboard_prompt_enabled(3));
 keyboard_prompt_pad(0,1,1,1);assert(keyboard_prompt_enabled(0));
 automatic=1;assert(!keyboard_prompt_enabled(0));enabled=1;assert(!keyboard_prompt_enabled(0));
 keys[5]=1;keyboard_prompt_keys(keys,512,0);assert(keyboard_prompt_enabled(0));automatic=0;
 assert(!keyboard_prompt_enabled(-1)&&!keyboard_prompt_enabled(4));
 manual="ps_modern";assert(!keyboard_prompt_enabled(0));return 0;
}
''')


def test_semantic_actions_live_rebinds_secondary_unbound_and_custom_icons(tmp_path):
    source = (ROOT/'port/linux/game/keyboard_glyphs.c').read_text(encoding='utf-8')
    header = (ROOT/'port/linux/include/halo_keyboard.h').read_text(encoding='utf-8')
    actions = re.search(r'enum halo_keyboard_action.*?;', header, re.S).group()
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define NONE -1
typedef unsigned int pixel32;
typedef struct {short x0,y0,x1,y1;}rectangle2d;
static struct {short local_player_index;} test_player={0};
static int enabled=1,seen=-1,calls,absent;
static char const *binding="E";static char drawn[64];
static long local_player_get_player_index(short p){return p==3?-1:p;}
#define player_get(p) (&test_player)
static int keyboard_prompt_enabled(short port){return enabled&&port==0;}
static int halo_keyboard_binding_label(int action,int slot,char *out,unsigned int size){
 (void)size;seen=action;if(absent==2 || (absent==1&&slot==0))return 0;strcpy(out,binding);return 1;
}
static void keyboard_glyph_draw(char const *s,rectangle2d *r,pixel32 c,int menu){assert(!menu);
 (void)c;strcpy(drawn,s);r->x0+=24;calls++;
}
''' + actions + function(source,'keyboard_hud_action') + function(source,'keyboard_hud_prompt') + r'''
int main(void){
 rectangle2d r={0};int i;
 assert(keyboard_hud_prompt(0,18,&r,0xffffffff)&&seen==HALO_KEYBOARD_ACTION&&!strcmp(drawn,"E"));
 binding="F";assert(keyboard_hud_prompt(0,18,&r,0xffffffff)&&!strcmp(drawn,"F"));
 absent=1;binding="Mouse Right";assert(keyboard_hud_prompt(0,20,&r,0xffffffff)&&seen==HALO_KEYBOARD_FIRE&&!strcmp(drawn,"Mouse Right"));
 absent=2;assert(keyboard_hud_prompt(0,22,&r,0xffffffff)&&!strcmp(drawn,"Unbound"));absent=0;
 assert(keyboard_hud_prompt(0,13,&r,0xffffffff)&&seen==HALO_KEYBOARD_SCOREBOARD);
 assert(keyboard_hud_prompt(0,12,&r,0xffffffff)&&seen==HALO_KEYBOARD_PAUSE);
 assert(keyboard_hud_prompt(0,28,&r,0xffffffff)&&!strcmp(drawn,"Enter"));
 calls=0;assert(keyboard_hud_prompt(0,30,&r,0xffffffff)&&calls==4);
 calls=0;for(i=32;i<256;i++)assert(!keyboard_hud_prompt(0,i,&r,0xffffffff));
 for(i=0;i<12;i++)assert(!keyboard_hud_prompt(0,i,&r,0xffffffff));
 assert(!calls);enabled=0;assert(!keyboard_hud_prompt(0,18,&r,0xffffffff));enabled=1;
 test_player.local_player_index=1;assert(!keyboard_hud_prompt(0,18,&r,0xffffffff));
 assert(!keyboard_hud_prompt(3,18,&r,0xffffffff)&&!keyboard_hud_prompt(-1,18,&r,0xffffffff));return 0;
}
''')


def test_keycap_visible_ink_centering_padding_and_state(tmp_path):
    source = (ROOT/'port/linux/game/keyboard_glyphs.c').read_text(encoding='utf-8')
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <wchar.h>
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define MAX(a,b) ((a)>(b)?(a):(b))
typedef float real;typedef unsigned int pixel32;
typedef struct {short x0,y0,x1,y1;}rectangle2d;
typedef struct {float x0,y0,x1,y1;}real_rectangle2d;
typedef struct {float alpha,red,green,blue;}real_argb_color;
static real_argb_color current={.5f,.3f,.4f,.5f};static int texts,symbol;
static float bx,by,bw,bh;static pixel32 drawn_edge;
static void draw_string_get_color(real_argb_color *c){*c=current;}
static void draw_string_set_color(real_argb_color const *c){current=*c;}
static short initial_indent=2,paragraph_indent=3;
static void draw_string_get_indents(short *a,short *b){*a=initial_indent;*b=paragraph_indent;}
static void draw_string_set_indents(short a,short b){initial_indent=a;paragraph_indent=b;}
static void rasterizer_unicode_string_ink_bounds(rectangle2d const *r,wchar_t const *s,real_rectangle2d *ink){
 ink->x0=r->x0+2.25f;ink->x1=ink->x0+(float)wcslen(s)*7;
 ink->y0=r->y0+3.5f;ink->y1=r->y0+16.5f;
 if(wcslen(s)>1)ink->y1+=2; /* descender still centers inside its cap */
}
static int keyboard_glyph_background(int s,float x,float y,float w,float h,pixel32 c){symbol=s;bx=x;by=y;bw=w;bh=h;drawn_edge=c;assert((c>>24)==128);return 1;}
static void rasterizer_draw_unicode_string_offset(rectangle2d const *r,wchar_t const *s,float x,float y){
 real_rectangle2d ink;rasterizer_unicode_string_ink_bounds(r,s,&ink);texts++;
 assert(ink.x0+x-bx>=5.9f&&bx+bw-ink.x1-x>=5.9f);
 assert(ink.x0+ink.x1+2*x==2*bx+bw);
 assert(ink.y0+ink.y1+2*y==2*by+bh);
}
static void rasterizer_draw_unicode_string_scaled_offset(rectangle2d const *r,wchar_t const *s,float scale,float x,float y){assert(0);}
''' + function(source,'keyboard_glyph_draw_padded') + function(source,'keyboard_glyph_draw') + r'''
int main(void){
 rectangle2d r={10,20,600,100};short single;
 keyboard_glyph_draw("F",&r,0x80ffffff,0);single=r.x0-10;
 assert(single>=28&&texts==1&&by+bh*.5f==30.0f&&drawn_edge==0x80a8d9ff);
 assert(current.alpha==.5f&&current.red==.3f&&initial_indent==2&&paragraph_indent==3);
 r.x0=10;keyboard_glyph_draw("Space",&r,0x80ffffff,0);assert(r.x0-10>single);
 texts=0;keyboard_glyph_draw("Mouse Left",&r,0x80ffffff,0);assert(symbol==7);
 keyboard_glyph_draw("Wheel Up",&r,0x80ffffff,0);assert(symbol==10);
 keyboard_glyph_draw("Right",&r,0x80ffffff,0);assert(symbol==5&&!texts);
 keyboard_glyph_draw("Mouse 4",&r,0x80ffffff,0);assert(texts==1);keyboard_glyph_draw("Enter",&r,0x80ffffff,1);assert(drawn_edge==0x802896ff);return 0;
}
''')


def test_supersampled_shapes_cache_failure_and_cleanup(tmp_path):
    source=(ROOT/'port/linux/game/keyboard_glyph_art.c').read_text(encoding='utf-8')
    run_c(tmp_path,r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define KEYBOARD_ART_SIZE 128
#define KEYBOARD_ART_COUNT 13
typedef float real;typedef unsigned int pixel32;
struct bitmap_data {unsigned int *pixels,*hardware_format;};
static struct bitmap_data *keyboard_art[13];static int keyboard_art_attempted[13];
static int allocations,releases,fail;
static struct bitmap_data *bitmap_2d_new(short w,short h,int m,int f){
 struct bitmap_data *b=calloc(1,sizeof(*b));assert(w==128&&h==128&&!m&&f==11);b->pixels=calloc(w*h,4);allocations++;return b;}
static void *bitmap_2d_address(struct bitmap_data *b,short x,short y,int m){assert(!m);return b->pixels+y*128+x;}
static void bitmap_rebuild(struct bitmap_data *b){if(!fail)b->hardware_format=b->pixels;}
static void bitmap_delete(struct bitmap_data *b){free(b->pixels);free(b);releases++;}
''' + ''.join(function(source,n) for n in ('keyboard_inside','keyboard_art_sample','keyboard_glyphs_dispose','keyboard_art_get')) + r'''
int main(void){
 int i,soft=0;struct bitmap_data *b=keyboard_art_get(1);
 assert(b&&keyboard_art_get(1)==b&&allocations==1);
 for(i=0;i<128*128;i++)if((b->pixels[i]>>24)>0&&(b->pixels[i]>>24)<255)soft++;
 assert(soft>100); /* subpixel coverage, not binary stair steps */
 assert(!keyboard_art_get(-1)&&!keyboard_art_get(13));
 fail=1;assert(!keyboard_art_get(3)&&!keyboard_art_get(3)&&allocations==2&&releases==1);
 keyboard_glyphs_dispose();keyboard_glyphs_dispose();assert(releases==2);
 fail=0;assert(keyboard_art_get(3)&&allocations==3);keyboard_glyphs_dispose();assert(releases==3);return 0;
}
''')


def test_optional_hook_precedes_controller_translation_and_preserves_custom_paths():
    text = (ROOT/'source/interface/hud_messaging.c').read_text(encoding='utf-8')
    start = text.index('case _hud_message_type_icon:')
    hook = text.index('keyboard_hud_prompt(', start)
    assert hook < text.index('icon_index = preferences.game_control_to_xbox_buttons', start)
    config = (ROOT/'port/linux/src/port_config.c').read_text(encoding='utf-8')
    assert '"display.keyboard_glyphs", _config_boolean, "false"' in config


def test_score_hint_only_rewrites_known_token_with_room(tmp_path):
    source = (ROOT/'port/linux/game/keyboard_glyphs.c').read_text(encoding='utf-8')
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#include <wchar.h>
#define NONE -1
static int enabled=1;
static struct {short local_player_index;} test_player={0};
#define player_get(p) (&test_player)
static int keyboard_prompt_enabled(short p){return enabled&&p==0;}
''' + function(source,'keyboard_score_hint') + r'''
int main(void){
 wchar_t text[80]=L"Hold BACK for score";
 keyboard_score_hint(0,text,80);assert(!wcscmp(text,L"Hold %back-button for score"));
 wcscpy(text,L"Hold BACK for score");keyboard_score_hint(0,text,20);assert(!wcscmp(text,L"Hold BACK for score"));
 wcscpy(text,L"Hold %s for score");keyboard_score_hint(0,text,80);assert(!wcscmp(text,L"Hold %back-button for score"));
 wcscpy(text,L"Unrecognized authored wording");keyboard_score_hint(0,text,80);assert(!wcscmp(text,L"Unrecognized authored wording"));
 enabled=0;wcscpy(text,L"Hold BACK for score");keyboard_score_hint(0,text,80);assert(!wcscmp(text,L"Hold BACK for score"));
 return 0;
}
''')


def test_renderer_ink_metrics_and_scoped_subpixel_offset(tmp_path):
    source=(ROOT/'source/rasterizer/rasterizer_text.c').read_text(encoding='utf-8')
    run_c(tmp_path,r'''
#include <assert.h>
#include <string.h>
#include <wchar.h>
#define NONE -1
#define FALSE 0
#define TRUE 1
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
typedef float real;typedef int boolean;typedef unsigned char byte;typedef unsigned short word;typedef unsigned int pixel32;
typedef struct {short x0,y0,x1,y1;}rectangle2d;
typedef struct {float x0,y0,x1,y1;}real_rectangle2d;
struct parse_string_state{int dummy;};
struct font_header {struct {byte *address;} pixels;};
struct font_character {short character,bitmap_origin_x,bitmap_origin_y,character_width,bitmap_width,pixels_offset;};
struct text_hires_glyph {float left,top,right,bottom,u0,v0,u1,v1,advance;};
static real_rectangle2d measured_ink;static long measured_hires_font;static boolean measured_has_ink;
static void *hires_text_atlas=(void *)1;static real text_scale=1,text_offset_x=2,text_offset_y=3;
static real text_scale_origin_x=17,text_scale_origin_y=23;
static int scaled;
static int covered=1;
static long draw_string_get_font(void){return 1;}
static long hires_text_font_get(long f,real s){assert(f==1&&s==1);return 9;}
static int text_hires_covers(long f,unsigned long c){assert(f==9);(void)c;return covered;}
static int text_hires_glyph(long f,unsigned long c,struct text_hires_glyph *g){
 assert(f==9);(void)c;g->left=1.25f;g->right=7.75f;g->top=-8.5f;g->bottom=.5f;g->advance=8;return 1;}
static void draw_unicode_string(void (*callback)(struct parse_string_state*,struct font_header*,struct font_character*,pixel32,short,short,short,short,short,short),rectangle2d const *r,void *p,void *clip,int h,wchar_t const *text){
 byte pixels[6]={0,1,0,0,0,1};struct font_header f={{pixels}};struct font_character c={72,2,10,10,3,0};
 (void)r;(void)p;(void)clip;(void)h;if(*text)callback(0,&f,&c,0,100,200,0,0,3,2);}
static void rasterizer_draw_unicode_string(rectangle2d const *r,void *a,void *b,int c,wchar_t const *t){
 (void)r;(void)a;(void)b;(void)c;(void)t;assert(text_offset_x==2.25f&&text_offset_y==2.5f);
 if(scaled)assert(text_scale==.85f&&text_scale_origin_x==0&&text_scale_origin_y==0);}
''' + ''.join(function(source,n) for n in ('rasterizer_measure_ink_character','rasterizer_unicode_string_ink_bounds','rasterizer_draw_unicode_string_offset','rasterizer_draw_unicode_string_scaled_offset')) + r'''
int main(void){rectangle2d r={0};real_rectangle2d ink;
 rasterizer_unicode_string_ink_bounds(&r,L"H",&ink);assert(ink.x0==104.25f&&ink.x1==110.75f&&ink.y0==201.5f&&ink.y1==210.5f);
 covered=0;rasterizer_unicode_string_ink_bounds(&r,L"H",&ink);assert(ink.x0==101&&ink.x1==103&&ink.y0==200&&ink.y1==202);
 rasterizer_unicode_string_ink_bounds(&r,L"",&ink);assert(ink.x0==0&&ink.x1==0);
 rasterizer_draw_unicode_string_offset(&r,L"H",.25f,-.5f);assert(text_offset_x==2&&text_offset_y==3);
 scaled=1;text_scale=1.5f;
 rasterizer_draw_unicode_string_scaled_offset(&r,L"H",.85f,.25f,-.5f);
 assert(text_scale==1.5f&&text_scale_origin_x==17&&text_scale_origin_y==23&&text_offset_x==2&&text_offset_y==3);
 rasterizer_draw_unicode_string_scaled_offset(&r,L"",.85f,.25f,-.5f);
 assert(text_scale==1.5f&&text_scale_origin_x==17&&text_scale_origin_y==23&&text_offset_x==2&&text_offset_y==3);return 0;}
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
