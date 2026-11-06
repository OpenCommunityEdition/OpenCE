"""Run the real MCC presentation against deterministic font/GPU services."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


def test_mcc_menu_draw_and_lifecycle(tmp_path):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("C compiler required")
    preamble = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "mcc_preview_image.h"
typedef int boolean;
typedef unsigned long pixel32;
typedef union rectangle2d {struct {short y0,x0,y1,x1;};short v[4];} rectangle2d;
typedef struct {float alpha,red,green,blue;} real_argb_color;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define SHORT_MAX 32767
#define MCC_MAPS_FIRST_DISPLAY_INDEX 0x1000
#define MCC_MAPS_MAXIMUM 1024
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
struct bitmap_data {short width,height;uintptr_t base_address;int hardware_format;};
struct rasterizer_dynamic_screen_geometry_parameters {int unused;};
static int live,loads,draws,text_draws,current_font,fail_image,portrait;
static rectangle2d drawn,clipped,drawn_texels;
static wchar_t drawn_text[512];
static const char *tag_name="ui\\shell\\bitmaps\\mp_map_grafix";
static const char *tag_get_name(long tag) {(void)tag;return tag_name;}
static long tag_loaded(long kind,const char *name) {(void)kind;(void)name;return 2;}
static rectangle2d *set_rectangle2d(rectangle2d *r,short x,short y,short x1,short y1) {r->x0=x;r->y0=y;r->x1=x1;r->y1=y1;return r;}
static const char *mcc_maps_level_name(short i) {static char level[64];if(i<0||i>=8)return NULL;snprintf(level,sizeof(level),"mcc_maps\\pkg\\level%d",i);return level;}
static boolean mcc_maps_campaign(short i) {return i%2;}
void mcc_preview_image_free(struct mcc_preview_image *im) {free(im->pixels);memset(im,0,sizeof(*im));}
int mcc_preview_image_load(const char *level,int campaign,struct mcc_preview_image *im) {
 unsigned i;(void)level;(void)campaign;loads++;if(fail_image)return 0;
 im->width=portrait?292:668;im->height=portrait?668:292;im->pixels=malloc(668*292*4);for(i=0;i<668*292;i++)im->pixels[i]=i;return 1;
}
static struct bitmap_data *bitmap_2d_new(short w,short h,short m,short f) {
 struct bitmap_data *b=calloc(1,sizeof(*b));assert(f==11&&!m);b->width=w;b->height=h;b->base_address=(uintptr_t)calloc(w*h,4);live++;return b;
}
static void bitmap_rebuild(struct bitmap_data *b) {b->hardware_format=1;}
static void bitmap_delete(struct bitmap_data *b) {free((void *)b->base_address);free(b);live--;}
static void draw_bitmap_in_rect(struct bitmap_data *b,rectangle2d *r,rectangle2d *t,rectangle2d *c,pixel32 color,struct rasterizer_dynamic_screen_geometry_parameters *p,boolean no_plasma) {
 (void)b;(void)color;(void)p;(void)no_plasma;draws++;drawn=*r;clipped=*c;drawn_texels=*t;
}
static void draw_string_set_draw_mode(long f,short s,short j,unsigned long flags,const real_argb_color *c) {(void)s;(void)j;(void)flags;(void)c;current_font=(int)f;}
static void draw_unicode_string_compute_bounds(const rectangle2d *b,const wchar_t *s,rectangle2d *ink,rectangle2d *cursor) {
 int width=current_font==2?6:10,height=current_font==2?12:22,x=b->x0,y=b->y0,right=x;
 for(;*s;s++){if(*s=='\r'){y+=height;x=b->x0;}else x+=width;right=MAX(x,right);}
 set_rectangle2d(ink,b->x0,b->y0,(short)right,(short)(y+height));set_rectangle2d(cursor,(short)x,(short)y,(short)x,(short)(y+height));
}
static void rasterizer_draw_unicode_string(const rectangle2d *b,const rectangle2d *c,void *cursor,short adjust,const wchar_t *text) {
 rectangle2d ink,pos;(void)cursor;(void)adjust;text_draws++;drawn=*b;clipped=*c;wcscpy(drawn_text,text);
 draw_unicode_string_compute_bounds(b,text,&ink,&pos);assert(ink.x1<=b->x1);assert(ink.y1<=b->y1);
}
'''
    code = preamble
    for name in ("mcc_menu_preview.c", "mcc_menu_text.c"):
        code += re.sub(r'^#include[^\n]*\n', '', (ROOT / "port/linux/game" / name).read_text(), flags=re.M)
    # Compile the production pure fit routine, with no decoder dependency.
    from harness import function
    code += function((ROOT / "port/linux/src/mcc_preview_image.c").read_text(), "mcc_preview_fit")
    code += r'''
int main(void) {
 rectangle2d box,clip;real_argb_color color={1,1,1,1};int i;wchar_t row_text[64];
 wcscpy(row_text,L"Short title");mcc_menu_text_row(row_text);assert(!wcscmp(row_text,L"Short title"));
 wcscpy(row_text,L"A long MCC title which needs to fit inside the map list row");
 mcc_menu_text_row(row_text);assert(width_of(row_text)<=310&&wcsstr(row_text,L"..."));
 assert(mcc_menu_preview_index(0,0)==NONE);assert(mcc_menu_preview_index(0,0x4000)==NONE);
 assert(mcc_menu_preview_index(0,0x6000)==NONE);assert(mcc_menu_preview_index(0,0x1000)==0);
 tag_name="unrelated";assert(mcc_menu_preview_index(0,0x1000)==NONE);tag_name="ui\\shell\\bitmaps\\mp_map_grafix";
 set_rectangle2d(&box,419,87,559,201);set_rectangle2d(&clip,420,90,550,199);
 assert(mcc_menu_preview_draw(0,&box,&clip,0,NULL));assert(live==1&&loads==1&&draws==1);
 assert(drawn.x0>=box.x0&&drawn.x1<=box.x1&&drawn.y0>=box.y0&&drawn.y1<=box.y1);
 assert(drawn.x1-drawn.x0==140&&drawn.y1-drawn.y0==114);
 assert(clipped.x0==420&&clipped.x1==550&&clipped.y0==90&&clipped.y1==199);
 assert(drawn_texels.x1==512&&drawn_texels.y1==416);
 assert(mcc_menu_preview_draw(0,&box,NULL,0,NULL));assert(loads==1);
 tag_name="pc\\mcc_xbox\\card_0\\picture";
 assert(mcc_menu_preview_draw(0,&box,&clip,0,NULL));
 assert(!memcmp(&drawn,&box,sizeof(box)));assert(drawn_texels.x0==0&&drawn_texels.x1==512);
 assert(drawn_texels.y0==0&&drawn_texels.y1==416);
 assert(*(uint32_t *)(uintptr_t)previews[0].bitmap->base_address==155); /* actual center pixels, not a left crop */
 assert(clipped.x0==420&&clipped.x1==550&&clipped.y0==90&&clipped.y1==199);
 tag_name="ui\\shell\\bitmaps\\mp_map_grafix";
 for(i=1;i<8;i++) assert(mcc_menu_preview_draw((short)i,&box,NULL,0,NULL));
 assert(live==4&&loads==8);mcc_menu_preview_clear();assert(live==0);
 fail_image=1;assert(!mcc_menu_preview_draw(0,&box,NULL,0,NULL));assert(loads==9);
 assert(!mcc_menu_preview_draw(0,&box,NULL,0,NULL));assert(loads==9);mcc_menu_preview_clear();
 fail_image=0;assert(mcc_menu_preview_draw(0,&box,NULL,0,NULL));assert(loads==10);mcc_menu_preview_clear();assert(live==0);
 portrait=1;assert(mcc_menu_preview_draw(0,&box,NULL,0,NULL));
 assert(!memcmp(&drawn,&box,sizeof(box))&&drawn_texels.x1==512&&drawn_texels.y1==416);
 assert(*(uint32_t *)(uintptr_t)previews[0].bitmap->base_address==215*292); /* centered vertical crop */
 mcc_menu_preview_clear();assert(live==0);
 tag_name="pc\\main_menu\\mp_map_right_data";set_rectangle2d(&box,417,258,563,394);
 assert(!mcc_menu_text_draw(0,1,L"Xbox",1,0,&color,&box,NULL));assert(!text_draws);
 assert(!mcc_menu_text_draw(0,0x4000,L"CE",1,0,&color,&box,NULL));assert(!text_draws);
 assert(mcc_menu_text_draw(0,0x1000,L"Steam Workshop\r\nHalo CE - A level with a long title and description",1,0,&color,&box,NULL));
 assert(wcschr(drawn_text,'\r')&&text_draws==1);
 tag_name="pc\\main_menu\\mp_map_right_name";set_rectangle2d(&box,417,204,563,247);
 assert(mcc_menu_text_draw(0,0x1000,L"A VERY LONG MCC LEVEL TITLE WITH LOTS OF WORDS",1,0,&color,&box,NULL));
 assert(current_font==2);assert(wcsstr(drawn_text,L"...")==NULL);
 assert(mcc_menu_text_draw(0,0x1000,L"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ",1,0,&color,&box,NULL));
 assert(wcsstr(drawn_text,L"...")!=NULL);
 tag_name="pc\\modern\\campaign\\mcc_folder_name";
 assert(mcc_menu_text_draw(0,NONE,L"Workshop folder with a long package title",1,0,&color,&box,NULL));
 tag_name="pc\\mcc_xbox\\card_0\\mcc_folder_help";set_rectangle2d(&box,83,258,227,394);
 assert(mcc_menu_text_draw(0,NONE,L"A long description must not cover the earned completion badges at the bottom of the native Xbox level card even when every difficulty has been completed.",2,0,&color,&box,NULL));
 assert(drawn.y0==258&&drawn.y1==341&&clipped.y1==341&&wcsstr(drawn_text,L"..."));
 puts("MCC preview bounds, cache lifecycle, text wrapping and Xbox/CE isolation passed");return 0;
}
'''
    source=tmp_path/"menu.c"; source.write_text(code)
    exe=tmp_path/("menu.exe" if sys.platform=="win32" else "menu")
    flags=["--target=i686-pc-windows-msvc","-fuse-ld=lld","-D_CRT_SECURE_NO_WARNINGS"] if sys.platform=="win32" else []
    result=subprocess.run([compiler,*flags,"-std=gnu11","-Wall","-Wextra","-Werror","-Wno-multichar","-I",str(ROOT/"port/linux/src"),str(source),"-o",str(exe)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
    assert result.returncode==0,result.stderr


def test_authored_thumbnail_boxes_and_hooks():
    import xml.etree.ElementTree as ET
    widgets=[]
    for path in (ROOT/"port/assets/menus").rglob("*.xml"):
        for widget in ET.parse(path).getroot().iter("widget"):
            if widget.get("name","").split("/")[-1] in ("mp_map_right_pic","lobby_map_pic"):
                widgets.append(widget)
    assert len(widgets)>=6
    assert all((w.get("width"),w.get("height"))==("140","114") for w in widgets)
    ui=(ROOT/"source/interface/ui_widget.c").read_text()
    assert 'mcc_menu_preview_draw(mcc_picture_index, &bounds, clip' in ui
    assert 'mcc_menu_text_draw(widget->definition_tag_index' in ui
    for file in ("mcc_maps.c","menu_tags.c"):
        assert 'mcc_menu_preview_clear();' in (ROOT/"port/linux/game"/file).read_text()
