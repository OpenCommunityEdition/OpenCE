"""Run the actual UI text-box and porting draw functions against a recording renderer.

Uses the existing NDK to build a freestanding wasm test (tools/wasm_test.py runs
it). No Android device is needed for the main menu's picture swap, the pause
menu's own text style, the title typeface's glyph layout, alpha/fade and
background geometry checks. GPU presentation still requires a device.
"""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import wasm_test
import re, shutil, subprocess

root = Path(__file__).resolve().parent.parent
out = root / 'build/porting-ui-tests'
out.mkdir(parents=True, exist_ok=True)
ndk = Path('C:/Android/ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/bin')

def function(path, name):
    source = (root / path).read_text()
    for match in reversed(list(re.finditer(r'\b'+name+r'\s*\(', source))):
        start = source.rfind('\n', 0, match.start())+1
        # Include the return type on the previous line if necessary.
        if source[start:match.start()].strip() == '':
            start = source.rfind('\n', 0, start-1)+1
        brace = source.find('{', match.end())
        semicolon = source.find(';', match.end())
        if brace < 0 or (semicolon >= 0 and semicolon < brace): continue
        depth = 1; end = brace+1
        while depth:
            depth += (source[end] == '{')-(source[end] == '}'); end += 1
        return source[start:end]
    raise ValueError(name)

prefix = r'''
typedef unsigned short wchar_t;
typedef float real;
typedef unsigned short word;
typedef int boolean;
#define NULL ((void*)0)
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define TEST_FLAG(a,b) ((a)&(1<<(b)))
#define NUMBER_OF_TEXT_JUSTIFICATIONS 3
#define _text_box_flashing_text_bit 0
#define _error_silent 0
#define FONT_GROUP_TAG 1
#define BITMAP_GROUP_TAG 2
#define SECONDS_PER_MILLISECOND 0.001f
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PIN(v,a,b) MIN(MAX(v,a),b)
#define _ui_audio_feedback_cursor 0
typedef struct {short y0,x0,y1,x1;} rectangle2d;
typedef struct {short x,y;} point2d;
typedef struct {real alpha,red,green,blue;} real_argb_color;
typedef unsigned long pixel32;
typedef struct {real x,y;} real_point2d;
#define NUMBER_OF_POINTS_PER_RECTANGLE 4
struct tag_reference {long index;};
struct tag_block {long count; void *address;};
struct ui_widget_search_and_replace_reference {char search_string[32]; short replace_function;};
struct ui_widget_definition {
 struct tag_reference text_label_string_list,text_font,background_bitmap;
 struct tag_block search_and_replace_functions;
 rectangle2d bounds; short horizontal_offset,vertical_offset,string_list_index,justification;
 word text_box_flags; real_argb_color text_color;
};
struct widget_instance {
 struct widget_instance *parent; long definition_tag_index;
 struct widget_instance *child,*next;
 real alpha_modifier; boolean visible;
 struct {struct {wchar_t *text;short string_list_index;} text_box;} parameters;
};
static struct ui_widget_definition template;
'''+(root/'port/linux/include/halo_porting_ui.h').read_text()+r'''
'''+(root/'port/linux/include/halo_porting_font.h').read_text()+r'''
'''+(root/'port/linux/include/halo_ui_pointer.h').read_text()+r'''
static struct halo_porting_menu ui_porting_menu;
static int ui_porting_context,ui_porting_count,ui_porting_revision;
static int ui_porting_scroll,ui_porting_scroll_max;
static int ui_porting_slider_action;
static rectangle2d ui_porting_slider_bounds, ui_porting_credit_bounds;
static boolean ui_porting_credit_visible, ui_porting_main_focus;
static struct widget_instance *ui_porting_item_text;
static rectangle2d ui_porting_item_text_bounds;
static int focused;
int ui_mouse_widget_has_focus(struct widget_instance *widget){return focused;}
#define UI_PORTING_TOP 112
#define UI_PORTING_BOTTOM 404
#define UI_PORTING_SCROLL_LEFT 590
#define UI_PORTING_SCROLL_RIGHT 626
static rectangle2d ui_porting_bounds[HALO_PORTING_ROWS];
static int ui_porting_actions[HALO_PORTING_ROWS];
static struct widget_instance fixture;
static struct widget_instance *ui_widget_port_error_text_box;
static wchar_t *ui_widget_port_error_text;
#define SPINNER_EXTRA_DESCRIPTION_BASE 1000
static wchar_t const *kills_to_win_extra_descriptions[] = {0};
struct ui_mouse_target {int kind; struct widget_instance *widget; rectangle2d bounds;};
#define _ui_mouse_target_item 0
static struct ui_mouse_target ui_mouse_targets[4];
static int ui_mouse_target_count, last_action, last_value, last_justification, last_font;
struct widget_instance *ui_mouse_menu(void){return &fixture;}
struct widget_instance *ui_porting_find_style(struct widget_instance *root){return root;}
struct widget_instance *ui_porting_find_text(struct widget_instance *root){return root;}
int ui_porting_is_quit(struct widget_instance *widget){return widget->definition_tag_index==99;}
void host_porting_action(int revision,int action,int value){last_action=action;last_value=value;}
void ui_play_audio_feedback_sound(int sound){}
void draw_quad(rectangle2d *bounds,unsigned long color){}
static struct {long current_system_milliseconds;} widget_globals;
static void *widget_memory_pool;
static int glyphs,percent,draw_percent,background_fixture,background_drawn;
static real_argb_color current_color;
static rectangle2d background_bounds, text_bounds;
struct bitmap_data {int width;};
/* the PC menus' art (tools/porting_assets.py): present, or the Xbox's menus */
static int art;
static int quads,last_frame;
static pixel32 quad_color;
static real quad_left,quad_right,quad_top,quad_bottom,quad_v0,quad_v1;
struct dynamic_screen_vertex {real_point2d position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
 struct bitmap_data *map[3]; struct {real i,j;} map_texture_scale[3],map_scale[3];
};
void rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters *p,struct dynamic_screen_vertex *v){
 if(!p->map[0]||p->map_scale[0].i!=1||p->map_texture_scale[0].j!=1)return;
 if(!quads){quad_left=v[0].position.x;quad_right=v[1].position.x;quad_top=v[0].position.y;quad_bottom=v[2].position.y;}
 if(v[0].position.x<quad_left)quad_left=v[0].position.x;
 if(v[1].position.x>quad_right)quad_right=v[1].position.x;
 if(v[0].position.y<quad_top)quad_top=v[0].position.y;
 if(v[2].position.y>quad_bottom)quad_bottom=v[2].position.y;
 quad_v0=v[0].texture_coordinates.y;quad_v1=v[2].texture_coordinates.y;
 quad_color=v[0].color;++quads;
}
pixel32 real_argb_color_to_pixel32(real_argb_color const*c){
 return ((pixel32)(c->alpha*255+0.5f)<<24)|((pixel32)(c->red*255+0.5f)<<16)|((pixel32)(c->green*255+0.5f)<<8)|(pixel32)(c->blue*255+0.5f);
}
void csmemset(void*d,int v,unsigned long n){unsigned char*a=d;while(n--)*a++=(unsigned char)v;}
int ui_widget_port_text_wrap(wchar_t *s,rectangle2d *b,short w){return 1;}
void draw_unicode_string_compute_bounds(rectangle2d *b,wchar_t *s,rectangle2d *t,rectangle2d *c){*t=*b;*c=*b;}
wchar_t *ustrncpy(wchar_t *d,wchar_t const *s,unsigned long n){unsigned long i=0;while(i<n&&s[i]){d[i]=s[i];++i;}return d;}
static struct bitmap_data blue;
void *memcpy(void *d,const void *s,unsigned long n) {char *a=d;const char*b=s;while(n--)*a++=*b++;return d;}
unsigned long ustrlen(wchar_t const*s){unsigned long n=0;while(s[n])++n;return n;}
char *strstr(char const*s,char const*q){for(;*s;++s){unsigned long n=0;while(q[n]&&q[n]==s[n])++n;if(!q[n])return (char*)s;}return NULL;}
int equal(char const*a,char const*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
wchar_t *ascii_to_wide(char const*s,wchar_t*d,unsigned long size){unsigned long n=0;while(s[n])++n;if(size<2*n+2)return NULL;for(unsigned long i=0;i<=n;++i)d[i]=s[i];return d;}
void *pool_resize_pointer(void*p,void*q,unsigned long n,char const*f,unsigned long l){return NULL;}
void csmemcpy(void*d,void const*s,unsigned long n){memcpy(d,s,n);}
wchar_t *unicode_string_list_get_string(long a,short b){return NULL;}
wchar_t *ui_widget_search_and_replace_invoke(struct widget_instance*w,short f){return NULL;}
long search_and_replace(wchar_t*a,wchar_t*b,wchar_t**c){return 0;}
void error(int level,char const*s){}
double cos(double x){return 0;}
int string_has_icons_to_draw(wchar_t*s){return 0;}
struct ui_widget_definition *ui_widget_definition_get(long tag){return &template;}
real_argb_color get_ui_argb_white(void){real_argb_color c={1,1,1,1};return c;}
void draw_string_set_draw_mode(long f,short s,short j,unsigned long flags,real_argb_color const*c){current_color=*c;last_justification=j;last_font=f;}
void draw_string_set_tab_stops(short const*p,short n){}
void draw_string_set_indents(short a,short b){}
void rasterizer_text_set_ui_scale(int x,int y,int p){percent=p;if(p!=100)draw_percent=p;}
int strcmp(char const*a,char const*b){while(*a&&*a==*b){++a;++b;}return *a-*b;}
char *strrchr(char const*s,int c){char *last=NULL;for(;*s;++s)if(*s==c)last=(char*)s;return last;}
int strncmp(char const*a,char const*b,unsigned long n){while(n&&*a&&*a==*b){++a;++b;--n;}return n?*a-*b:0;}
char const *tag_get_name(long tag){return tag==42?"ui\\shell\\pause_game\\pause_dialog_bkd":"ui\\shell\\pause_game\\button_key";}
void rasterizer_draw_unicode_string(rectangle2d const*b,rectangle2d const*c,point2d*p,short h,wchar_t const*s){
 if(current_color.alpha>0 && b->y0+17<b->y1 && b->x0<b->x1 && c->x0<c->x1 && c->y0<c->y1){glyphs+=(int)ustrlen(s);text_bounds=*b;}
}
void draw_string_and_hack_in_icons(rectangle2d*b,rectangle2d*c,point2d*p,short h,wchar_t const*s,boolean v){}
long halo_screen_width(void){return 1068;}
long tag_loaded(int group,char const*name){
 if(group==FONT_GROUP_TAG)return 1;
 if(background_fixture==0&&equal(name,"ui\\shell\\bitmaps\\blue"))return 10;
 if(background_fixture==1&&equal(name,"ui\\shell\\bitmaps\\pausebox_center"))return 11;
 if(art&&equal(name,HALO_PORTING_FONT_TAG))return 20;
 if(art&&equal(name,HALO_PORTING_ITEM_TAG))return 21;
 return NONE;
}
long ui_porting_item_bitmap(void){return tag_loaded(BITMAP_GROUP_TAG,HALO_PORTING_ITEM_TAG);}
struct bitmap_data *bitmap_group_get_bitmap_from_sequence(long t,int s,int f){if(t==20)last_frame=f;return &blue;}
void draw_bitmap_in_rect(struct bitmap_data*b,rectangle2d*r,rectangle2d*s,rectangle2d*c,unsigned long color,void*p,boolean plain){
 background_bounds=*r;background_drawn=plain;
}
'''
code = prefix + '\n' + '\n'.join([
    function('source/interface/ui_widget.c','widget_instance_get_cumulative_alpha_modifier'),
    function('source/interface/ui_widget.c','widget_instance_render_text_box'),
    function('source/interface/ui_widget_porting.h','ui_porting_text'),
    function('source/interface/ui_widget_porting.h','ui_porting_glyph'),
    function('source/interface/ui_widget_porting.h','ui_porting_glyph_quad'),
    function('source/interface/ui_widget_porting.h','ui_porting_glyph_text'),
    function('source/interface/ui_widget_porting.h','ui_porting_label'),
    function('source/interface/ui_widget_porting.h','ui_porting_adjust_widget'),
    function('source/interface/ui_widget_porting.h','ui_porting_background'),
    function('source/interface/ui_widget_porting.h','ui_porting_pointer'),
    function('source/interface/ui_widget_porting.h','ui_porting_render'),
]) + r'''
int run_tests(void){
 struct widget_instance parent={0},style={0};
 rectangle2d clip={0,0,480,640},row={356,192,389,448};
 real_argb_color blue_item={0.6f,0,128.f/255.f,1},cyan={1,0.2f,0.4f,0.6f};
 template.text_label_string_list.index=NONE;template.text_font.index=7;template.justification=0;
 template.text_color.alpha=0;style.parent=&parent;style.alpha_modifier=0;style.visible=0;

 /* The Xbox's menus have no picture: the main menu's label is capitals of text. */
 art=0;ui_porting_context=1;ui_porting_menu.page=0;glyphs=0;draw_percent=100;
 ui_porting_text(&style,"PORTING OPTIONS",row,&clip,140,&blue_item);
 if(glyphs!=15||current_color.alpha!=0.6f||percent!=100||last_justification!=2)return 1;
 if(draw_percent!=140||current_color.red!=0||current_color.green!=128.f/255.f||current_color.blue!=1)return 14;
 fixture.definition_tag_index=99;fixture.parent=&parent;
 ui_mouse_target_count=1;ui_mouse_targets[0].kind=_ui_mouse_target_item;
 ui_mouse_targets[0].widget=&fixture;ui_mouse_targets[0].bounds=row;
 ui_porting_menu.revision=5;glyphs=0;quads=0;
 ui_porting_render(&fixture,&clip);
 if(ui_porting_count!=1||ui_porting_bounds[0].y0!=356||ui_porting_bounds[0].y1!=389||glyphs!=15||quads)return 16;
 focused=1;glyphs=0;ui_porting_render(&fixture,&clip);focused=0;
 if(glyphs!=30||current_color.red!=1||current_color.blue!=1||current_color.alpha!=1)return 21;

 /* The PC menus: Quit draws our picture itself, twice as wide about its middle,
    with its own frames for the selected glow; nothing is drawn over it. */
 art=1;
 {struct ui_widget_definition copy,*def=&template;point2d offset={192,391};
  template.bounds.x0=0;template.bounds.x1=256;template.bounds.y0=0;template.bounds.y1=33;
  template.background_bitmap.index=5;
  if(!ui_porting_adjust_widget(&fixture,&def,&copy,&offset)||def!=&copy)return 22;
  if(copy.background_bitmap.index!=21||copy.bounds.x0!=-128||copy.bounds.x1!=384||copy.bounds.y1!=33)return 23;
  if(template.background_bitmap.index!=5||template.bounds.x1!=256)return 24; /* the tag is untouched */
  if(offset.x!=192||offset.y!=391)return 25;
  parent.definition_tag_index=1;def=&template;
  if(!ui_porting_adjust_widget(&style,&def,&copy,&offset)||def!=&template)return 26; /* only Quit */
 }
 glyphs=0;quads=0;ui_porting_render(&fixture,&clip);
 if(ui_porting_count!=1||ui_porting_bounds[0].y0!=356||glyphs||quads)return 27;
 struct halo_ui_pointer main_tap={0};main_tap.left_clicks=1;main_tap.click_x=320;main_tap.click_y=370;
 if(!ui_porting_pointer(&main_tap)||last_action!=1)return 17;
 ui_mouse_target_count=0;fixture.parent=NULL;

 /* The pause menu: the frame grows, and PORTING OPTIONS has the items' own
    font, colour, justification and 112% size, set where their text sits. */
 ui_porting_context=2;
 struct widget_instance background={0};struct ui_widget_definition copy,*def=&template;point2d offset={320,240};
 background.parent=&parent;background.definition_tag_index=42;template.bounds.y1=200;
 if(!ui_porting_adjust_widget(&background,&def,&copy,&offset)||copy.bounds.y1!=(200+52)*112/100)return 15;
 template.text_color=cyan;template.justification=1;
 {struct widget_instance item={0},text={0};rectangle2d item_bounds={200,180,227,460};
  item.parent=&parent;item.definition_tag_index=1;text.parent=&item;text.definition_tag_index=1;
  text.alpha_modifier=0;
  ui_mouse_target_count=1;ui_mouse_targets[0].kind=_ui_mouse_target_item;
  ui_mouse_targets[0].widget=&item;ui_mouse_targets[0].bounds=item_bounds;
  ui_porting_item_text=&text;
  ui_porting_item_text_bounds.x0=190;ui_porting_item_text_bounds.x1=450;
  ui_porting_item_text_bounds.y0=203;ui_porting_item_text_bounds.y1=225;
  fixture.parent=NULL;glyphs=0;quads=0;draw_percent=100;last_font=0;
  ui_porting_render(&fixture,&clip);
  if(ui_porting_count!=1||ui_porting_actions[0]!=1||glyphs!=15||quads)return 2;
  if(ui_porting_bounds[0].y0!=231||ui_porting_bounds[0].y1!=261||ui_porting_bounds[0].x0!=180)return 28;
  if(draw_percent!=112||last_font!=7||last_justification!=1)return 29;
  if(current_color.alpha!=1||current_color.red!=0.2f||current_color.green!=0.4f||current_color.blue!=0.6f)return 30;
  if(text_bounds.x0!=190||text_bounds.y0!=234)return 31; /* 10 right of and 3 below its row's corner */
  ui_mouse_target_count=0;ui_porting_item_text=NULL;
 }
 template.text_color.alpha=0;glyphs=0;
 ui_porting_text(&style,"PORTING OPTIONS",row,&clip,112,NULL);
 if(glyphs!=15||current_color.alpha!=1)return 32; /* a tag whose text shows only in its plasma pass */

 /* Its screens keep the pause menu's font, in their own colours. */
 template.text_color=cyan;
 ui_porting_menu.page=6;row.x0=70;row.x1=570;row.y0=112;row.y1=172;glyphs=0;quads=0;last_font=0;
 ui_porting_label(&style,"INVINCIBILITY [ON]",row,&clip,TRUE,FALSE,FALSE);
 if(glyphs!=18||quads||current_color.alpha!=1||current_color.green!=1||last_font!=7||draw_percent!=160)return 3;
 ui_porting_label(&style,"GYROSCOPE: UNAVAILABLE",row,&clip,FALSE,FALSE,FALSE);
 if(current_color.alpha!=0.45f||percent!=100||last_justification!=2)return 4;
 for(background_fixture=0;background_fixture<2;++background_fixture){
  background_drawn=0;ui_porting_background();
  if(!background_drawn||background_bounds.x0!=-214||background_bounds.x1!=854||background_bounds.y0!=0||background_bounds.y1!=480)return 5;
 }
 row.y0=60;row.y1=108;
 ui_porting_label(&style,"GENERAL",row,&clip,TRUE,TRUE,FALSE);
 if(current_color.red!=0.35f||current_color.green!=0.8f)return 6;

 /* From the main menu the screens are set in its typeface: a picture a letter. */
 ui_porting_context=1;glyphs=0;quads=0;
 ui_porting_label(&style,"GENERAL",row,&clip,TRUE,TRUE,FALSE);
 if(glyphs||quads!=7||quad_color!=0xff59ccff)return 33;
 if(quad_left<70||quad_right>570||quad_top<60||quad_bottom>108)return 34;
 {real middle=(quad_left+quad_right)*0.5f;if(middle<318||middle>322)return 35;} /* centred */
 if(last_frame!=11)return 36; /* L */
 quads=0;ui_porting_label(&style,"general",row,&clip,TRUE,TRUE,FALSE);
 if(quads!=7||last_frame!=11)return 37; /* small letters draw their capitals */
 row.y0=112;row.y1=172;quads=0;
 ui_porting_label(&style,"FOV: 70 DEGREES",row,&clip,TRUE,FALSE,TRUE);
 if(quads!=13||quad_color!=0xffd9f0ff||quad_bottom>112+52)return 38; /* above its slider; spaces draw nothing */
 quads=0;ui_porting_label(&style,"RUMBLE: UNAVAILABLE",row,&clip,FALSE,FALSE,FALSE);
 if(quads!=18||(quad_color>>24)!=0x73)return 39;
 quads=0;ui_porting_label(&style,"JETPACK [ON]",row,&clip,TRUE,FALSE,FALSE);
 if(quad_color!=0xff40ff73)return 40;
 quads=0;ui_porting_label(&style,"ACTIVE CAMOUFLAGE (LOCAL PLAYER) [OFF] AND A VERY LONG TAIL OF TEXT",row,&clip,TRUE,FALSE,FALSE);
 if(!quads||quad_left<70||quad_right>570)return 41; /* too long: smaller, still inside its row */
 {rectangle2d viewport={112,0,404,640},low={380,70,440,570};quads=0;
  ui_porting_label(&style,"BOTTOM",low,&viewport,TRUE,FALSE,FALSE);
  if(!quads||quad_bottom>404||quad_v1>=1.0f||quad_v0!=0.0f)return 42; /* cut at the viewport, with its texture */
  low.y0=420;low.y1=480;quads=0;ui_porting_label(&style,"BELOW",low,&viewport,TRUE,FALSE,FALSE);
  if(quads)return 43;
 }
 art=0;quads=0;glyphs=0;
 ui_porting_label(&style,"GENERAL",row,&clip,TRUE,FALSE,FALSE);
 if(quads||glyphs!=7)return 44; /* the Xbox's menus: the menu's font */
 art=1;

 ui_porting_context=2;
 ui_porting_menu.count=65;ui_porting_menu.revision=8;
 for(int i=0;i<64;++i){memcpy(ui_porting_menu.rows[i].text,"FIRE",5);ui_porting_menu.rows[i].action=1000+i;ui_porting_menu.rows[i].value=-1;}
 ui_porting_render(&fixture,&clip);
 if(ui_porting_scroll_max!=3548||ui_porting_count!=6)return 7;
 if(ui_porting_actions[ui_porting_count-1]!=90||ui_porting_bounds[ui_porting_count-1].y0!=420)return 8;
 struct halo_ui_pointer pointer={0};pointer.scroll_pixels=32767;last_action=0;
 if(!ui_porting_pointer(&pointer)||ui_porting_scroll!=3548||last_action)return 9;
 ui_porting_render(&fixture,&clip);
 pointer.scroll_pixels=0;pointer.left_clicks=1;pointer.click_x=100;pointer.click_y=365;
 ui_porting_pointer(&pointer);
 if(last_action!=1063)return 10;
 pointer.click_y=445;ui_porting_pointer(&pointer);
 if(last_action!=90)return 11;
 pointer.left_clicks=0;pointer.scroll_drag=1;pointer.x=608;pointer.y=112;last_action=0;
 ui_porting_pointer(&pointer);
 if(ui_porting_scroll!=0||last_action)return 12;
 pointer.scroll_drag=0;pointer.scroll_pixels=-32767;ui_porting_pointer(&pointer);
 if(ui_porting_scroll!=0)return 13;
 ui_porting_menu.page=4;ui_porting_menu.count=2;ui_porting_scroll=ui_porting_scroll_max=0;
 ui_porting_menu.rows[0].action=40;ui_porting_menu.rows[0].value=467;
 memcpy(ui_porting_menu.rows[0].text,"LOOK SENSITIVITY",17);
 ui_porting_render(&fixture,&clip);
 pointer=(struct halo_ui_pointer){0};pointer.left_clicks=1;pointer.click_x=70;pointer.click_y=150;
 ui_porting_pointer(&pointer);if(last_action!=40||last_value!=0)return 18;
 ++ui_porting_menu.revision;pointer.scroll_drag=1;pointer.x=640;pointer.y=20;
 ui_porting_pointer(&pointer);if(last_value!=1000)return 19;
 pointer.x=0;ui_porting_pointer(&pointer);if(last_value!=0)return 20;

 /* The whole screen from the main menu, with no menu font to fall back on. */
 ui_porting_context=1;ui_porting_menu.page=3;ui_porting_menu.count=3;
 memcpy(ui_porting_menu.title,"GENERAL",8);
 memcpy(ui_porting_menu.rows[0].text,"EDIT IN-GAME UI",16);ui_porting_menu.rows[0].action=14;ui_porting_menu.rows[0].value=-1;
 memcpy(ui_porting_menu.rows[1].text,"RUMBLE: ON",11);ui_porting_menu.rows[1].action=30;ui_porting_menu.rows[1].value=-1;
 quads=0;glyphs=0;ui_porting_render(&fixture,&clip);
 if(glyphs||quads!=7+13+9+4||ui_porting_count!=3||ui_porting_actions[0]!=14)return 45;
 return 0;
}
'''
(out/'test.c').write_text(code)
subprocess.run([str(ndk/'clang.exe'),'--target=wasm32','-DHALO_ANDROID=1','-std=c99','-fshort-wchar','-ffreestanding','-fno-builtin','-c',str(out/'test.c'),'-o',str(out/'test.o')],check=True)
subprocess.run([str(ndk/'ld.lld.exe'),'-flavor','wasm','--no-entry','--export=run_tests',str(out/'test.o'),'-o',str(out/'test.wasm')],check=True)
wasm_test.check(out/'test.wasm', 'UI', 'Main menu picture, pause menu style, title typeface glyphs, bitmap bounds, 64-control scrolling and fixed Back passed')
