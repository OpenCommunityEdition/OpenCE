"""Live high-res toggles: run the production cache and font-selection code."""
from pathlib import Path
from harness import function
from test_hud_button_style import run_c
ROOT=Path(__file__).resolve().parents[1]


def test_only_committed_affected_textures_are_refreshed(tmp_path):
    hud=(ROOT/'port/linux/src/hud_hires.c').read_text()
    texture=(ROOT/'port/linux/src/xbox_textures.c').read_text()
    code=r'''
#include <assert.h>
#include <string.h>
#include <stdlib.h>
static unsigned long revision;
static int hud_on=1,text_on=1;
static unsigned long config_changes(void){return revision;}
static int config_boolean(char const *s){return strstr(s,"high_res_hud")?hud_on:text_on;}
static struct {int title;} hud_hires_embedded[2]={{0},{1}};
static long hud_hires_asset_count(void){return 2;}
static long hud_hires_asset_at(unsigned long a,long w,long h){return w==32 && h==32 && a<2?(long)a:-1;}
#define TEXTURE_BUCKET_COUNT 4
#define TEXTURE_IDLE_FRAMES 1800
struct texture_entry {
 struct texture_entry *next; unsigned long address,generation,last_used_frame;
 struct {unsigned long width,height;} description;
 unsigned texture,hud_upscale;
};
static struct texture_entry *texture_buckets[TEXTURE_BUCKET_COUNT];
static unsigned long texture_frame,texture_drop_serial;
static unsigned long recent_textures[8];
static int invalidated,deleted;
static void glDeleteTextures(int n,unsigned const *t){(void)n;(void)t;deleted++;}
static void xgpu_gl_state_invalidate(void){invalidated++;}
''' + function(hud,'hud_hires_settings_changed')+'\n'+function(hud,'hud_hires_bitmap_affected')+'\n'+function(texture,'xgpu_texture_cache_begin_frame')+r'''
int main(void){
 struct texture_entry h={0},t={0},world={0};int i;
 h.address=0;t.address=1;world.address=99;
 h.description.width=t.description.width=world.description.width=32;
 h.description.height=t.description.height=world.description.height=32;
 texture_buckets[0]=&h;h.next=&t;t.next=&world;
 h.generation=t.generation=world.generation=42;
 xgpu_texture_cache_begin_frame();assert(h.generation==42 && t.generation==42);
 for(i=0;i<20;i++){
  /* An unrelated saved setting (or an uncommitted menu edit) is harmless. */
  revision++;xgpu_texture_cache_begin_frame();assert(h.generation==42 && t.generation==42);
  recent_textures[0]=99;hud_on=!hud_on;revision++;
  xgpu_texture_cache_begin_frame();assert(h.generation==0 && t.generation==42 && world.generation==42);
  assert(recent_textures[0]==0);h.generation=42;
  text_on=!text_on;revision++;xgpu_texture_cache_begin_frame();
  assert(t.generation==0 && h.generation==42 && world.generation==42);t.generation=42;
 }
 assert(invalidated==40 && deleted==0);
 /* Map address reuse is classified against the new map's tags, not old choices. */
 h.address=99;hud_on=!hud_on;revision++;xgpu_texture_cache_begin_frame();assert(h.generation==42);
 return 0;
}
'''
    run_c(tmp_path,code)


def test_text_can_start_disabled_and_toggle_without_caching_none(tmp_path):
    text=(ROOT/'port/linux/src/text_hires.c').read_text()
    source=(ROOT/'source/rasterizer/rasterizer_text.c').read_text()
    code=r'''
#include <assert.h>
#include <string.h>
typedef float real;typedef unsigned short word;typedef unsigned char byte;
#define NONE -1
#define TEXT_HIRES_FIT_LETTERS 52
#define MAXIMUM_HIRES_TEXT_FONTS 4
static int enabled,calls;static unsigned long revision;
static unsigned long config_changes(void){return revision;}
static int config_boolean(char const *s){(void)s;return enabled;}
struct font_header{struct{void *address;}pixels;};
struct font_character{int character_width,bitmap_height,bitmap_width,pixels_offset;};
static byte ink[]={255};static struct font_header font={{ink}};
static struct font_character capital={5,1,1,0};
static struct {struct font_header *font;real oversample;long hires_font;}hires_text_fonts[4];
static short hires_text_next_font;
static struct font_header *font_definition_get(long n){(void)n;return &font;}
static struct font_character *font_get_character_by_ascii_code(struct font_header *f,word c){(void)f;(void)c;return &capital;}
static char const *tag_get_name(long n){(void)n;return "ui\\small_ui";}
static long text_hires_font(char const *n,float h,short const *w,float o){(void)n;(void)h;(void)w;(void)o;calls++;return 7;}
''' + function(text,'text_hires_enabled')+'\n'+function(source,'hires_text_font_get')+r'''
int main(void){int i;
 assert(hires_text_font_get(1,1)==NONE && calls==0);
 for(i=0;i<20;i++){
  enabled=1;revision++;assert(hires_text_font_get(1,1)==7);
  assert(hires_text_font_get(1,1)==7);
  enabled=0;revision++;assert(hires_text_font_get(1,1)==NONE);
 }
 assert(calls==1); /* No atlas/font rebuild on a toggle. */
 enabled=1;revision++;assert(hires_text_font_get(1,.75f)==7 && calls==2);
 memset(hires_text_fonts,0,sizeof(hires_text_fonts));hires_text_next_font=0;
 assert(hires_text_font_get(1,1)==7 && calls==3);
 return 0;
}
'''
    run_c(tmp_path,code)


def test_menu_changes_commit_only_on_save(tmp_path):
    source=(ROOT/'port/linux/game/menu_functions.c').read_text()
    code=r'''
#include <assert.h>
#include <string.h>
#include <stddef.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
static int committed=1,writes;
struct widget_instance {int definition_tag_index;struct{struct{short selected_index,number_of_items;}list;}parameters;};
struct pc_menu_setting {char const *setting;short loaded_index,value_count;char const *values[2];};
static struct pc_menu_setting setting={"display.high_res_hud",-1,2,{"true","false"}};
static struct pc_menu_setting *pc_menu_setting_get(int i){(void)i;return &setting;}
static int spinner_item(struct widget_instance *w){(void)w;return 0;}
static int setting_text(char const *s,char *out,size_t n,int def){(void)s;(void)n;(void)def;strcpy(out,committed?"true":"false");return 1;}
static int setting_value_index(struct pc_menu_setting *s,char const *v){(void)s;return strcmp(v,"true")!=0;}
static int config_write(char const *s,char const *v){(void)s;committed=!strcmp(v,"true");writes++;return 1;}
static void platform_log(char const *s,...){(void)s;}
'''+'\n'+function(source,'setting_load')+'\n'+function(source,'setting_save')+r'''
int main(void){struct widget_instance w={0};w.parameters.list.number_of_items=2;
 assert(setting_load(&w));w.parameters.list.selected_index=1;
 assert(committed && writes==0); /* Selecting No does not commit. */
 assert(setting_load(&w));assert(w.parameters.list.selected_index==0 && committed); /* Reopen after Cancel. */
 w.parameters.list.selected_index=1;assert(setting_save(&w));assert(!committed && writes==1);
 assert(setting_save(&w));assert(writes==1);
 w.parameters.list.selected_index=0;assert(!committed);assert(setting_save(&w));assert(committed && writes==2);
 return 0;}
'''
    run_c(tmp_path,code)
