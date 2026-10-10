"""MCC model and meter flags select channels per consumer of shared bitmaps."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def model_masks(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for MCC shader channel tests")
    bitmap = (ROOT / "port/linux/game/mcc_bitmaps.c").read_text()
    header = (ROOT / "source/bitmaps/bitmap_group.h").read_text()
    declarations = "\n".join(structure(header, name) for name in [
        "bitmap_data", "bitmap_group_sprite", "bitmap_group_sequence", "bitmap_group"])
    declarations += "\n" + "\n".join(structure(bitmap, name) for name in [
        "mcc_texture", "mcc_bitmap_clone", "mcc_textures"])
    routines = "\n".join(function(bitmap, name) for name in [
        "mcc_bitmap_tag", "mcc_bitmap_block", "mcc_bitmap_mark_one", "mcc_bitmap_clone_block",
        "mcc_bitmap_specialize", "mcc_bitmap_mark", "mcc_meter_usage", "mcc_bitmap_model_usage",
        "mcc_bitmap_bytes", "mcc_pixel", "mcc_channels", "mcc_bitmap_decode"])
    source = r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef float real;
union point2d {struct {short x,y;};uint32_t n;};
typedef struct {float x0,x1,y0,y1;} real_rectangle2d;
typedef struct {float x,y;} real_point2d;
struct tag_block {long count;void *address,*definition;};
struct tag_data {long size,flags,file_offset;void *address,*definition;};
struct meter_hud_element_definition {struct {uint32_t index;} meter_bitmap;short sequence_index;unsigned char meter_flags;};
#define BITMAP_GROUP_TAG 'bitm'
#define NONE -1
static uint32_t global_vector_palette[256];
struct mcc_runtime {unsigned char *tags,*tag_index;uint32_t used;struct {uint32_t tag_count;} report;};
static void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t p,uint32_t size) {
    uintptr_t start=(uintptr_t)r->tags;
    return p>=start && p-start<=r->used && size<=r->used-(p-start) ? (void *)(uintptr_t)p : NULL;
}
static void *mcc_runtime_allocate(struct mcc_runtime *r,uint32_t size) {
    uint32_t at=(r->used+15)&~15u;
    if(size>65536-at)return NULL;
    r->used=at+size;return r->tags+at;
}
#define BCDEC_STATIC
#define BCDEC_IMPLEMENTATION
#include "bcdec.h"
''' + declarations + "\n" + routines + r'''
static void shader_init(unsigned char *shader,uint16_t flags,uint32_t handle) {
    static unsigned const offsets[]={0xB0,0xC8,0xE8,0x170};unsigned i;
    memset(shader,0,0x174);memcpy(shader+0x28,&flags,sizeof(flags));
    for(i=0;i<4;i++)memcpy(shader+offsets[i],&handle,sizeof(handle));
}
int main(int argc,char **argv) {
    struct mcc_runtime runtime={0};struct mcc_textures textures={0};
    struct bitmap_group *original;struct bitmap_data *descriptor,original_descriptor;
    unsigned char xbox[0x174],gearbox[0x174],repeated[0x174],missing[0x174];
    uint32_t *entry,source=0xE1740000,cloned,decoded[16];unsigned i;
    /* A red-only opaque BC1 block: specular reflection, with no color
     * change. Misreading alpha as change color instead tints all texels. */
    unsigned char dxt1[8]={0,0xF8,0,0,0,0,0,0};
    unsigned char rgba[4]={10,20,30,40};
    struct meter_hud_element_definition xbox_meter={{source},NONE,0x3F},gearbox_meter={{source},NONE,0x1F};
    int reverse;
    if(argc!=2)return 1;reverse=atoi(argv[1]);
    runtime.tags=calloc(1,65536);runtime.tag_index=runtime.tags;runtime.used=512;
    runtime.report.tag_count=1;
    original=mcc_runtime_allocate(&runtime,sizeof(*original));
    descriptor=mcc_runtime_allocate(&runtime,sizeof(*descriptor));
    original->bitmaps.count=1;original->bitmaps.address=descriptor;
    descriptor->width=descriptor->height=4;descriptor->depth=1;
    descriptor->format=14;descriptor->pixels_offset=2048;descriptor->pixels_size=8;
    original_descriptor=*descriptor;
    entry=(uint32_t *)runtime.tag_index;entry[0]='bitm';entry[3]=source;entry[5]=(uint32_t)original;
    textures.capacity=8;textures.tag_capacity=8;textures.count=1;
    textures.items=calloc(8,sizeof(*textures.items));textures.clones=calloc(8,sizeof(*textures.clones));
    textures.items[0].bitmap=descriptor;textures.items[0].handle=source;
    shader_init(xbox,0x7F,source);shader_init(gearbox,0x3F,source);shader_init(repeated,0,source);
    if(!mcc_bitmap_model_usage(&runtime,&textures,reverse ? gearbox : xbox) ||
       !mcc_bitmap_model_usage(&runtime,&textures,reverse ? xbox : gearbox))return 2;
    cloned=*(uint32_t *)(gearbox+0xC8);
    if(cloned==source || runtime.report.tag_count!=2 || textures.count!=2)return 3;
    if(*(uint32_t *)(xbox+0xC8)!=source || textures.items[0].channels!=1 || textures.items[1].channels!=2)return 4;
    if(memcmp(&original_descriptor,descriptor,sizeof(*descriptor)) ||
       textures.items[1].bitmap==descriptor || memcmp(textures.items[1].bitmap,descriptor,sizeof(*descriptor)))return 5;
    if(*(uint16_t *)(xbox+0x28)!=0x7F || *(uint16_t *)(gearbox+0x28)!=0x3F)return 6;
    if(!mcc_bitmap_model_usage(&runtime,&textures,repeated) || *(uint32_t *)(repeated+0xC8)!=cloned ||
       runtime.report.tag_count!=2 || textures.count!=2)return 7;
    /* Base, detail and cube textures remain ordinary in both shaders. */
    if(*(uint32_t *)(gearbox+0xB0)!=source || *(uint32_t *)(gearbox+0xE8)!=source ||
       *(uint32_t *)(gearbox+0x170)!=source)return 8;
    mcc_bitmap_decode(dxt1,decoded,4,4,14,textures.items[0].channels);
    for(i=0;i<16;i++)if(decoded[i]!=0xFFFF0000u)return 9;
    mcc_bitmap_decode(rgba,decoded,1,1,11,textures.items[1].channels);
    if(decoded[0]!=0x1E0A1428u)return 10;
    shader_init(missing,0x40,0xFFFFFFFFu);
    if(!mcc_bitmap_model_usage(&runtime,&textures,missing) || runtime.report.tag_count!=2)return 11;
    shader_init(missing,0,0xFFFFFFFFu);
    if(!mcc_bitmap_model_usage(&runtime,&textures,missing) || runtime.report.tag_count!=2)return 12;
    /* Neither channel path can silently accept a stale/non-bitmap handle. */
    shader_init(missing,0x40,source+3);
    if(mcc_bitmap_model_usage(&runtime,&textures,missing))return 13;
    shader_init(missing,0,source^0x10000u);
    if(mcc_bitmap_model_usage(&runtime,&textures,missing))return 14;
    /* A bitmap can simultaneously serve two meter conventions and a
     * material mask without any of these consumers overwriting another. */
    if(!mcc_meter_usage(&runtime,&textures,reverse ? &gearbox_meter : &xbox_meter) ||
       !mcc_meter_usage(&runtime,&textures,reverse ? &xbox_meter : &gearbox_meter))return 15;
    if(xbox_meter.meter_bitmap.index!=source || gearbox_meter.meter_bitmap.index==source ||
       gearbox_meter.meter_bitmap.index==cloned || textures.count!=3 || runtime.report.tag_count!=3 ||
       textures.items[0].channels!=1 || textures.items[1].channels!=2 || textures.items[2].channels!=4)return 16;
    if(xbox_meter.meter_flags!=0x3F || gearbox_meter.meter_flags!=0x1F ||
       memcmp(&original_descriptor,descriptor,sizeof(*descriptor)))return 17;
    /* Transparent outside-shape pixel has nonzero fill order. Preserve
     * alpha zero in Xbox order, while an unflagged meter must swap it. */
    rgba[0]=rgba[1]=rgba[2]=128;rgba[3]=0;
    mcc_bitmap_decode(rgba,decoded,1,1,11,textures.items[0].channels);
    if(decoded[0]!=0x00808080u)return 18;
    mcc_bitmap_decode(rgba,decoded,1,1,11,textures.items[2].channels);
    if(decoded[0]!=0x80000000u)return 19;
    xbox_meter.meter_bitmap.index=gearbox_meter.meter_bitmap.index=0xFFFFFFFFu;
    if(!mcc_meter_usage(&runtime,&textures,&xbox_meter) || !mcc_meter_usage(&runtime,&textures,&gearbox_meter) ||
       runtime.report.tag_count!=3)return 20;
    free(textures.clones);free(textures.items);free(runtime.tags);return 0;
}
'''
    work = tmp_path_factory.mktemp("mcc-model-masks")
    path = work / "masks.c"
    path.write_text(source)
    output = work / ("masks.exe" if sys.platform == "win32" else "masks")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror",
               "-Wno-multichar", "-Wno-unused-function", "-I", str(ROOT / "port/third_party/bcdec"),
               str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command[1:1] = ["-m32", "-fsanitize=undefined", "-fsanitize-trap=undefined"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


@pytest.mark.parametrize("reversed_consumers", [0, 1])
def test_mcc_model_channel_order_is_per_shader(model_masks, reversed_consumers):
    result = subprocess.run([str(model_masks), str(reversed_consumers)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
