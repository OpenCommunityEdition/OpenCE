"""Compile the real pause builder and resource importer with a fake renderer.

Guarded allocations cover generated pixels and the copied live tag table.
Failure injection checks every allocation and texture-creation boundary, so
partial imports cannot publish dangling tags or leak native texture headers.
"""
from pathlib import Path
import re
import shutil
import subprocess
import sys

import pytest

from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


def _compile(source, binary):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for the MCC pause runtime tests")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
               "-I", str(ROOT / "port/linux/game"), str(source), "-o", str(binary)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command[1:1] = ["-m32"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


@pytest.fixture(scope="module")
def runtime_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-pause-runtime")
    tags = (ROOT / "source/tag_files/tag_groups.h").read_text()
    bitmaps = (ROOT / "source/bitmaps/bitmap_group.h").read_text()
    strings = (ROOT / "source/text/text_group.h").read_text()
    types = "\n".join(structure(tags, name) for name in ["tag_block", "tag_reference", "tag_data"])
    types += "\n" + "\n".join(structure(bitmaps, name) for name in [
        "bitmap_data", "bitmap_group_sequence", "bitmap_group"])
    types += "\n" + "\n".join(structure(strings, name) for name in ["string_list", "string_list_entry"])
    # Keep both implementations intact; replace only their project headers
    # with the native structures and narrow host services declared below.
    implementations = "\n".join(re.sub(r'^#include "[^"\n]+"\s*$', "", (ROOT / name).read_text(), flags=re.M)
        for name in ["port/linux/game/mcc_pause.c", "port/linux/game/mcc_pause_runtime.c"])
    unbind = function((ROOT / "port/linux/src/d3d8_gl.c").read_text(), "xgpu_mcc_texture_unbind")
    source = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char boolean;
typedef unsigned short word;
typedef unsigned long DWORD;
typedef float real;
union point2d {struct {short x,y;} point;long value;};
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#include "mcc_pause.h"
#include "mcc_ui.h"
#include "mcc_pause_runtime.h"
''' + types + r'''
enum {D3DTSS_MAXSTAGES=4};
static struct {void *textures[D3DTSS_MAXSTAGES];} device;
''' + unbind + r'''
#define CHECK(c) do {if(!(c)){fprintf(stderr,"line %d: %s\n",__LINE__,#c);return 1;}}while(0)
struct allocation {struct allocation *next;size_t bytes;unsigned long pad[2];};
static struct allocation *allocations;
static unsigned malloc_calls,live_allocations,fail_malloc;
static unsigned gpu_calls,gpu_live,gpu_changed,gpu_deleted,fail_gpu;
static unsigned published,restored,logs;
static boolean active=TRUE,null_renderer;
static int connection;
static long large_font=0x11110000,small_font=0x11120001,live_count;
static void *live_table,*original_table;
static struct test_scenario {short type;} scenario,*global_scenario;
static struct {struct {boolean teams;} universal_variant;} variant;
enum {_game_connection_local,_game_connection_network_client,_game_connection_network_server};
static void *test_malloc(size_t bytes) {
    struct allocation *a;unsigned char *end;
    if(++malloc_calls==fail_malloc)return NULL;
    a=malloc(sizeof(*a)+bytes+16);if(!a)return NULL;
    a->next=allocations;a->bytes=bytes;a->pad[0]=a->pad[1]=0xcafebabe;allocations=a;
    end=(unsigned char *)(a+1)+bytes;memset(end,0xD7,16);live_allocations++;return a+1;
}
static void check_guards(void) {
    struct allocation *a;unsigned i;
    for(a=allocations;a;a=a->next) {
        assert(a->pad[0]==0xcafebabe&&a->pad[1]==0xcafebabe);
        for(i=0;i<16;i++)assert(((unsigned char *)(a+1))[a->bytes+i]==0xD7);
    }
}
static void test_free(void *p) {
    struct allocation **slot=&allocations,*a;
    assert(p);check_guards();
    while(*slot&&(*slot)+1!=p)slot=&(*slot)->next;
    assert(*slot);a=*slot;*slot=a->next;memset(p,0xEA,a->bytes);free(a);live_allocations--;
}
static boolean mcc_cache_tags_loaded(void){return active;}
void mcc_ui_settings_close(short controller){assert(controller==NONE);}
static short game_connection(void){return (short)connection;}
static void *global_network_game_server_get(void){return connection==_game_connection_network_server?&variant:NULL;}
#define game_engine_get_variant() (&variant)
static long tag_loaded(long group,char const *name) {
    assert(group=='font');
    if(!strcmp(name,"ui\\large_ui"))return large_font;
    if(!strcmp(name,"ui\\small_ui"))return small_font;
    assert(0);return NONE;
}
char const *config_string(char const *name){assert(!strcmp(name,"display.menus"));return "pc";}
void platform_log(char const *format,...){assert(format);logs++;}
void *cache_files_tag_instances(long *count){*count=live_count;return live_table;}
void cache_files_set_tag_instances(void *instances,long count) {
    assert(instances&&count>=2);
    if(instances==original_table)restored++;
    else {assert(live_table==original_table&&mcc_pause_tag_count()>24);published++;}
    live_table=instances;live_count=count;
}
static boolean rasterizer_bitmap_new(struct bitmap_data *bitmap) {
    assert(bitmap->width>0&&!(bitmap->width&(bitmap->width-1)));
    assert(bitmap->height>0&&!(bitmap->height&(bitmap->height-1)));
    assert(bitmap->format==11&&bitmap->depth==1&&bitmap->type==0&&bitmap->flags==1);
    assert(bitmap->pixels_size==bitmap->width*bitmap->height*4&&bitmap->base_address);
    if(++gpu_calls==fail_gpu)return FALSE;
    if(null_renderer)return TRUE;
    bitmap->hardware_format=test_malloc(16);
    if(!bitmap->hardware_format)return FALSE;
    device.textures[gpu_calls%3]=bitmap->hardware_format;
    gpu_live++;return TRUE;
}
static void rasterizer_bitmap_changed(struct bitmap_data *bitmap) {
    assert(bitmap->base_address&&(bitmap->hardware_format||null_renderer));check_guards();gpu_changed++;
}
static void rasterizer_bitmap_delete(struct bitmap_data *bitmap) {
    unsigned stage;
    assert(bitmap->hardware_format&&gpu_live);
    for(stage=0;stage<D3DTSS_MAXSTAGES;stage++)if(device.textures[stage]==bitmap->hardware_format) {
        fputs("texture header released while still bound\n",stderr);exit(88);
    }
    test_free(bitmap->hardware_format);bitmap->hardware_format=NULL;gpu_live--;gpu_deleted++;
}
#define malloc test_malloc
#define free test_free
''' + implementations + r'''
#undef malloc
#undef free
static struct mcc_pause_instance original[2];
static unsigned char original_copy[sizeof(original)];
static void setup(void) {
    memset(original,0,sizeof(original));
    original[0].group=original[1].group='font';original[0].handle=0x11110000;original[1].handle=0x11120001;
    original[0].name="ui\\large_ui";original[1].name="ui\\small_ui";
    original[0].data=&original[0];original[1].data=&original[1];
    memcpy(original_copy,original,sizeof(original));
    live_table=original;original_table=original;live_count=2;
    global_scenario=&scenario;scenario.type=0;
    device.textures[3]=&scenario; /* An unrelated texture identity must survive. */
}
static int clean(void) {
    mcc_pause_runtime_unload();
    CHECK(live_table==original_table&&live_count==2&&!live_allocations&&!gpu_live&&!mcc_pause_tag_count());
    CHECK(!device.textures[0]&&!device.textures[1]&&!device.textures[2]&&device.textures[3]==&scenario);
    CHECK(!memcmp(original_copy,original,sizeof(original)));return 0;
}
static int inventory(void) {
    long i,j;
    struct mcc_pause_instance *table=live_table;
    CHECK(live_table!=original_table&&live_count>2&&published==1);
    CHECK(!memcmp(original,table,sizeof(original)));
    for(i=2;i<live_count;i++) {
        CHECK(table[i].data&&table[i].name&&(table[i].handle&0xffff)==i);
        CHECK(table[i].parents[0]==NONE&&table[i].parents[1]==NONE);
        if(table[i].group=='DeLa')CHECK(mcc_pause_owns(table[i].handle));
        if(table[i].group=='bitm') {
            struct bitmap_group const *group=table[i].data;
            struct bitmap_group_sequence const *sequence=group->sequences.address;
            struct bitmap_data const *frames=group->bitmaps.address;
            CHECK(group->sequences.count==1&&sequence->first_bitmap_index==0&&
                sequence->bitmap_count==group->bitmaps.count&&group->bitmaps.count>=1&&group->bitmaps.count<=2);
            for(j=0;j<group->bitmaps.count;j++) {
                CHECK(frames[j].tag_index==table[i].handle&&frames[j].base_address);
                CHECK(frames[j].hardware_format||null_renderer);
            }
        } else CHECK(table[i].group=='DeLa'||table[i].group=='ustr');
    }
    return 0;
}
static int pixel_check(void) {
    long i,j;int x,y;
    for(i=0;i<pause_runtime.art_count;i++) {
        struct mcc_pause_art const *art=&pause_runtime.art[i];
        for(j=0;j<art->group.bitmaps.count;j++) {
            struct bitmap_data const *bitmap=&art->frames[j];
            unsigned long const *pixels=bitmap->base_address;
            for(y=0;y<bitmap->height;y++)for(x=0;x<bitmap->width;x++) {
                unsigned long color=pixels[y*bitmap->width+x];
                if(art->kind==MCC_PAUSE_ART_DIM)CHECK(color==0x80000000UL);
                else if(x>=art->width||y>=art->height)CHECK(!color);
                else if(art->kind==MCC_PAUSE_ART_HIGHLIGHT&&!j)CHECK(!color);
                else CHECK(!color||color==0xFF2896FFUL||color==0xC00A2649UL||color==0x60204A75UL);
            }
        }
    }
    check_guards();return 0;
}
int main(int argc,char **argv) {
    unsigned calls,textures,k;long old;
    CHECK(argc==2);setup();
    if(!strcmp(argv[1],"inactive")) {
        active=FALSE;CHECK(!mcc_pause_runtime_load());
        CHECK(!malloc_calls&&!gpu_calls&&!published&&!mcc_pause_runtime_active());
        CHECK(mcc_pause_runtime_screen(1,TRUE)==NONE);CHECK(!clean());return 0;
    }
    if(!strcmp(argv[1],"fonts")) {
        large_font=NONE;CHECK(mcc_pause_runtime_load());CHECK(mcc_pause.assets.font[0]==small_font);CHECK(!clean());
        large_font=original[0].handle;small_font=NONE;
        CHECK(mcc_pause_runtime_load());CHECK(mcc_pause.assets.small_font[0]==large_font);CHECK(!clean());
        large_font=small_font=NONE;
        CHECK(mcc_pause_runtime_load());CHECK(mcc_pause.assets.font[0]==original[0].handle);CHECK(!clean());
        original[0].group=original[1].group='bitm';memcpy(original_copy,original,sizeof(original));
        published=0;CHECK(!mcc_pause_runtime_load());CHECK(!published&&!live_allocations&&!gpu_live);CHECK(!clean());return 0;
    }
    if(!strcmp(argv[1],"table-bounds")) {
        live_count=-1;CHECK(!mcc_pause_runtime_load()&&!published&&!live_allocations);setup();
        live_count=63001;CHECK(!mcc_pause_runtime_load()&&!published&&!live_allocations);setup();
        live_table=NULL;CHECK(!mcc_pause_runtime_load()&&!published&&!live_allocations);setup();
        CHECK(!clean());return 0;
    }
    if(!strcmp(argv[1],"art-bounds")) {
        static short const invalid[][2]={{0,1},{1,0},{-1,1},{1,-1},{1025,1},{1,513}};
        long handle;
        for(k=0;k<NUMBEROF(invalid);k++) {
            CHECK(mcc_pause_art_add(invalid[k][0],invalid[k][1],MCC_PAUSE_ART_PANEL)==NONE);
            CHECK(!live_allocations&&!gpu_live);mcc_pause_runtime_unload();
        }
        handle=mcc_pause_art_add(1024,512,MCC_PAUSE_ART_HIGHLIGHT);CHECK(handle!=NONE);
        calls=malloc_calls;textures=gpu_calls;
        CHECK(mcc_pause_art_add(1024,512,MCC_PAUSE_ART_HIGHLIGHT)==handle&&malloc_calls==calls&&gpu_calls==textures);
        CHECK(!pixel_check());CHECK(!clean());
        for(k=0;k<64;k++)CHECK(mcc_pause_art_add((short)(10+k),10,MCC_PAUSE_ART_PANEL)!=NONE);
        CHECK(mcc_pause_art_add(100,11,MCC_PAUSE_ART_PANEL)==NONE);CHECK(!clean());return 0;
    }
    if(!strcmp(argv[1],"null-renderer"))null_renderer=TRUE;
    CHECK(mcc_pause_runtime_load());calls=malloc_calls;textures=gpu_calls;
    CHECK(!inventory());
    if(!strcmp(argv[1],"publish")) {
        CHECK(mcc_pause_runtime_load()&&published==1&&malloc_calls==calls&&gpu_calls==textures);
        CHECK(mcc_pause_runtime_screen(1,TRUE)!=NONE);
    } else if(!strcmp(argv[1],"pixels")||!strcmp(argv[1],"null-renderer")) {
        CHECK(!pixel_check());
    } else if(!strcmp(argv[1],"malloc-failure")) {
        CHECK(!clean());
        for(k=1;k<=calls;k++) {
            fail_malloc=k;malloc_calls=0;published=0;
            CHECK(!mcc_pause_runtime_load());CHECK(!published);CHECK(!clean());
        }
        fail_malloc=0;published=0;CHECK(mcc_pause_runtime_load());CHECK(!inventory());
    } else if(!strcmp(argv[1],"gpu-failure")) {
        CHECK(!clean());
        for(k=1;k<=textures;k++) {
            fail_gpu=k;gpu_calls=0;published=0;
            CHECK(!mcc_pause_runtime_load());CHECK(!published);CHECK(!clean());
        }
        fail_gpu=0;published=0;CHECK(mcc_pause_runtime_load());CHECK(!inventory());
    } else if(!strcmp(argv[1],"lifetime")) {
        old=mcc_pause_runtime_screen(1,TRUE);CHECK(!clean());CHECK(!mcc_pause_owns(old));
        CHECK(mcc_pause_runtime_screen(1,TRUE)==NONE);CHECK(!clean());
        published=0;CHECK(mcc_pause_runtime_load());CHECK(!inventory());
    } else if(!strcmp(argv[1],"selection")) {
        int type,conn,teams,count,first;
        for(type=0;type<2;type++)for(conn=0;conn<3;conn++)for(teams=0;teams<2;teams++)
        for(count=1;count<=4;count++)for(first=0;first<2;first++) {
            scenario.type=(short)type;connection=conn;variant.universal_variant.teams=(boolean)teams;
            CHECK(mcc_pause_runtime_screen((short)count,(boolean)first)==
                mcc_pause_select(type==0,conn!=0,conn==2,(boolean)(type==1&&teams),(short)count,(boolean)first));
        }
        global_scenario=NULL;CHECK(mcc_pause_runtime_screen(1,TRUE)==NONE);
    } else return 99;
    CHECK(!clean());return 0;
}
'''
    path = work / "pause_runtime_check.c"
    path.write_text(source)
    binary = work / ("pause_runtime_check.exe" if sys.platform == "win32" else "pause_runtime_check")
    _compile(path, binary)
    return binary


@pytest.mark.parametrize("case", ["inactive", "publish", "pixels", "null-renderer", "fonts", "table-bounds", "art-bounds",
                                 "malloc-failure", "gpu-failure", "lifetime", "selection"])
def test_mcc_pause_runtime(runtime_tool, case):
    result = subprocess.run([str(runtime_tool), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, f"{case}: exit {result.returncode}\n{result.stdout}{result.stderr}"


def test_bound_header_release_negative_control(runtime_tool, tmp_path):
    """The lifetime checks must reject the old release-without-unbind path."""
    source = (runtime_tool.parent / "pause_runtime_check.c").read_text()
    call = "xgpu_mcc_texture_unbind(bitmap->hardware_format);"
    assert source.count(call) == 1
    path = tmp_path / "bound_release.c"
    path.write_text(source.replace(call, "/* injected missing unbind */", 1))
    binary = tmp_path / ("bound_release.exe" if sys.platform == "win32" else "bound_release")
    _compile(path, binary)
    result = subprocess.run([str(binary), "lifetime"], capture_output=True, text=True, timeout=30)
    assert result.returncode == 88, result.stderr
    assert "texture header released while still bound" in result.stderr
