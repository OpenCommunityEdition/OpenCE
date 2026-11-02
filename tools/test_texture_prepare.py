"""Real loading pass: budgets, family guards, palette exclusion and failure."""
import os
from pathlib import Path
import re
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('gles', [False, True])
def test_texture_preparation(tmp_path, gles):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    code = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define NONE -1
#define BITMAP_GROUP_TAG 1
#define _error_silent 0
struct bitmap_data {short width,height,depth,type,format,mipmap_count;unsigned flags;long pixels_size;};
struct tag_iterator {int done;};
struct bitmap_group {struct {long count;void *address;} bitmaps;};
static struct bitmap_data bitmaps[4];
static struct bitmap_group group;
static int enabled,mcc,server,null_renderer,loads,uploads,fail;
static unsigned long charged;
static long config_integer(char const *key) {(void)key;return enabled;}
static int config_boolean(char const *key) {assert(!strcmp(key,"debug.null_renderer"));return null_renderer;}
static int config_server_mode_enabled(void) {return server;}
static int mcc_cache_tags_loaded(void) {return mcc;}
static void tag_iterator_new(struct tag_iterator *it,long tag) {(void)tag;it->done=0;}
static long tag_iterator_next(struct tag_iterator *it) {return it->done++ ? NONE : 1;}
static struct bitmap_group *bitmap_group_get(long tag) {(void)tag;return &group;}
static unsigned long texture_prepare_charge(struct bitmap_data const *bitmap);
static void *_texture_cache_bitmap_get_hardware_format(struct bitmap_data *b,int block,int load) {
    assert(block && load && b->format!=17);loads++;charged+=texture_prepare_charge(b);return fail?NULL:b;
}
static int render_prepare_texture(void const *p) {assert(p);uploads++;return 1;}
static void error(int severity,char const *fmt,...) {(void)severity;(void)fmt;}
'''
    code += re.sub(r'^#include[^\n]*\n', '', (ROOT/'port/linux/game/texture_prepare.c').read_text(), flags=re.M)
    code += r'''
int main(void) {
    int i;
    for(i=0;i<4;i++) {bitmaps[i].width=bitmaps[i].height=256;bitmaps[i].depth=1;
        bitmaps[i].flags=128;bitmaps[i].format=11;bitmaps[i].pixels_size=262144;}
    group.bitmaps.address=bitmaps;group.bitmaps.count=4;
    texture_prepare_map();assert(!loads);
    enabled=1;mcc=1;texture_prepare_map();assert(!loads);mcc=0;
    server=1;texture_prepare_map();assert(!loads);server=0;
    null_renderer=1;texture_prepare_map();assert(!loads);null_renderer=0;
#ifndef HALO_GLES
    assert(texture_prepare_charge(&bitmaps[0])==262144);
    bitmaps[0].width=1;bitmaps[0].height=1024;bitmaps[0].pixels_size=1024;
    assert(texture_prepare_charge(&bitmaps[0])==65536);bitmaps[0]=bitmaps[1];
    bitmaps[0].type=2;assert(texture_prepare_charge(&bitmaps[0])==1572864);bitmaps[0].type=0;
    bitmaps[0].mipmap_count=1;assert(texture_prepare_charge(&bitmaps[0])==327680);bitmaps[0].mipmap_count=0;
    bitmaps[0].width=4096;bitmaps[0].height=4096;bitmaps[0].depth=512;
    assert(!texture_prepare_charge(&bitmaps[0]));bitmaps[0]=bitmaps[1];
    bitmaps[1].format=17;bitmaps[2].flags=0;bitmaps[3].width=-1;
    texture_prepare_map();assert(loads==1 && uploads==1 && charged==262144);
    for(i=1;i<4;i++)bitmaps[i]=bitmaps[0];
    bitmaps[0].mipmap_count=1;loads=uploads=0;charged=0;
    texture_prepare_map();assert(loads==3 && uploads==3 && charged<=1024*1024);
    loads=uploads=0;charged=0;fail=1;texture_prepare_map();assert(loads==3 && !uploads && charged<=1024*1024);
#else
    texture_prepare_map();assert(!loads && !uploads);
#endif
    return 0;
}
'''
    path = tmp_path/'prepare.c'
    path.write_text(code)
    binary = tmp_path/('prepare.exe' if os.name=='nt' else 'prepare')
    flags = ['--target=i686-pc-windows-msvc', '-fuse-ld=lld'] if os.name=='nt' else ['-m32']
    if gles: flags += ['-DHALO_GLES=1']
    result = subprocess.run([compiler,*flags,'-std=gnu89',str(path),'-o',str(binary)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    subprocess.run([str(binary)],check=True)
