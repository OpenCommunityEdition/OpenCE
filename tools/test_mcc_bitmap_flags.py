"""Exercise MCC descriptor conversion with generated pixels, without map assets."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def bitmap_converter(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed for MCC bitmap conversion tests")
    bitmap = (ROOT / "port/linux/game/mcc_bitmaps.c").read_text()
    native = (ROOT / "source/bitmaps/bitmaps.c").read_text()
    swizzle = (ROOT / "source/rasterizer/rasterizer_swizzle.c").read_text()
    source = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define TRUE 1
#define FALSE 0
#define NONE -1
#define FLAG(bit) (1u<<(bit))
#define TEST_FLAG(value,bit) ((value)&FLAG(bit))
#define VALID_INDEX(index,count) ((index)>=0 && (index)<(count))
#define NUMBER_OF_BITMAP_FLAGS 9
#define NUMBER_OF_BITMAP_TYPES 3
#define NUMBER_OF_BITMAP_FORMATS 18
#define BITMAP_GROUP_TAG 0x6269746du
#define _bitmap_format_a8r8g8b8 11
#define _bitmap_has_power_of_two_dimensions_bit 0
#define _bitmap_compressed_bit 1
#define _bitmap_palettized_bit 2
#define _bitmap_swizzled_bit 3
#define _bitmap_linear_bit 4
#define _error_silent 0
#define error(...) ((void)0)
#define match_assert(file,line,condition) do {if(!(condition))abort();} while(0)
#define MCC_BITMAP_BASE 0x60000000u
#define MCC_BITMAP_LIMIT 0x1FFFFFFFu
#define MCC_BITMAP_ALLOCATION_LIMIT 0x08000000u
typedef unsigned char boolean;
struct bitmap_data {
    uint32_t signature;
    short width,height,depth,type,format;
    unsigned short flags;
    short mipmap_count;
    int32_t pixels_offset,pixels_size,tag_index,cache_block_index;
    void *hardware_format,*base_address;
};
struct bitmap_group {short usage;};
struct mcc_runtime {struct bitmap_group group;unsigned char source[350000];uint32_t size,reads;};
static short floor_log2(unsigned value) {short n=0;while(value>1){value>>=1;n++;}return n;}
static int bitmap_format_type_valid_width(short f,short t,short v) {(void)f;(void)t;return v>0 && v<=4096;}
static int bitmap_format_type_valid_height(short f,short t,short v) {return bitmap_format_type_valid_width(f,t,v);}
static int bitmap_format_type_valid_depth(short f,short t,short v) {(void)f;(void)t;return v==1;}
static void *mcc_bitmap_tag(struct mcc_runtime *r,uint32_t h,uint32_t g,uint32_t s) {
    (void)s;return h==7 && g==BITMAP_GROUP_TAG ? &r->group : NULL;
}
static int mcc_runtime_read(struct mcc_runtime *r,uint32_t offset,uint32_t size,void *out) {
    r->reads++;if(offset || size>r->size)return 0;memcpy(out,r->source,size);return 1;
}
static long rasterizer_xbox_bitmap_get_pixel_data_size(struct bitmap_data *b) {
    uint32_t bytes=0;unsigned level;
    for(level=0;level<=(unsigned)b->mipmap_count;level++) {
        unsigned w=MAX(b->width>>level,1),h=MAX(b->height>>level,1);
        bytes+=b->format==14 ? ((w+3)/4)*((h+3)/4)*8 : w*h*2;
    }
    return (bytes+127u)&~127u;
}
static uint32_t global_vector_palette[256];
#define BCDEC_STATIC
#define BCDEC_IMPLEMENTATION
#include "bcdec.h"
'''
    source += "\n" + structure(bitmap, "mcc_texture")
    source += "\n" + structure(bitmap, "mcc_bitmap_clone")
    source += "\n" + structure(bitmap, "mcc_textures")
    source += "\n" + function(native, "bitmap_verify")
    source += "\n" + function(swizzle, "rasterizer_xbox_bitmap_get_max_mipmap_count")
    source += "\n" + "\n".join(function(bitmap, name) for name in [
        "mcc_bitmap_bytes", "mcc_pixel", "mcc_channels", "mcc_bitmap_decode",
        "mcc_morton_axis", "mcc_bitmap_pack", "mcc_bitmap_lightmap_flags", "mcc_bitmap_complete_mips",
        "mcc_texture_prepare"])
    source += r'''
int main(int argc,char **argv) {
    struct bitmap_data bitmap={0},original;
    struct mcc_textures textures={0};
    struct mcc_texture texture={0};
    struct mcc_runtime runtime={0};
    unsigned i;int expect=1,converted,tail=0;
    static unsigned const order[8]={0,1,4,5,2,3,6,7};
    if(argc!=2)return 2;
    runtime.group.usage=4;runtime.size=16;
    bitmap.signature=BITMAP_GROUP_TAG;bitmap.width=4;bitmap.height=2;bitmap.depth=1;
    bitmap.format=6;bitmap.flags=0x1281;bitmap.pixels_size=16;
    texture.bitmap=&bitmap;texture.handle=7;
    for(i=0;i<16;i++)runtime.source[i]=(unsigned char)i;
    if(!strncmp(argv[1],"tail",4)) {
        tail=1;runtime.group.usage=0;runtime.size=349525;
        bitmap.width=1024;bitmap.height=512;bitmap.format=14;bitmap.flags=0x81;
        bitmap.mipmap_count=10;bitmap.pixels_size=349525;
        for(i=0;i<runtime.size;i++)runtime.source[i]=(unsigned char)i;
        if(!strcmp(argv[1],"tail-known")){}
        else if(!strcmp(argv[1],"tail-random-short")) {bitmap.pixels_size--;expect=0;}
        else if(!strcmp(argv[1],"tail-short-base")) {bitmap.pixels_size=262143;expect=0;}
        else if(!strcmp(argv[1],"tail-wrong-flags")) {bitmap.flags=0x83;expect=0;}
        else if(!strcmp(argv[1],"tail-wrong-format")) {bitmap.format=15;expect=0;}
        else return 3;
    }
    else if(!strcmp(argv[1],"known")){}
    else if(!strcmp(argv[1],"ordinary")) {bitmap.flags=0x81;runtime.group.usage=0;}
    else if(!strcmp(argv[1],"wrong-group")) {runtime.group.usage=0;expect=0;}
    else if(!strcmp(argv[1],"missing-group")) {texture.handle=8;expect=0;}
    else if(!strcmp(argv[1],"missing-environment")) {bitmap.flags&=~0x200;expect=0;}
    else if(!strcmp(argv[1],"unknown-flag")) {bitmap.flags|=0x400;expect=0;}
    else if(!strcmp(argv[1],"extra-pixels")) {bitmap.pixels_size=17;expect=0;}
    else if(!strcmp(argv[1],"short-pixels")) {bitmap.pixels_size=15;expect=0;}
    else if(!strcmp(argv[1],"bad-signature")) {bitmap.signature=0;expect=0;}
    else if(!strcmp(argv[1],"wrong-format")) {bitmap.format=8;expect=0;}
    else if(!strcmp(argv[1],"non-power-two")) {bitmap.width=3;bitmap.pixels_size=12;expect=0;}
    else if(!strcmp(argv[1],"mipmaps")) {bitmap.mipmap_count=1;bitmap.pixels_size=20;expect=0;}
    else return 3;
    original=bitmap;
    converted=mcc_texture_prepare(&runtime,&textures,&texture);
    if(converted!=expect)return 4;
    if(!expect) {
        if(runtime.reads || memcmp(&bitmap,&original,sizeof(bitmap)))return 5;
        return 0;
    }
    if(tail) {
        if(bitmap.flags!=0x83 || bitmap.format!=14 || bitmap.mipmap_count!=7 ||
            bitmap.pixels_size!=349568 || runtime.reads!=1 || memcmp(texture.pixels,runtime.source,349520))return 10;
        for(i=349520;i<(unsigned)bitmap.pixels_size;i++)if(texture.pixels[i])return 11;
        free(texture.pixels);return 0;
    }
    if(bitmap.flags!=0x89 || bitmap.format!=6 || bitmap.width!=4 || bitmap.height!=2 ||
        bitmap.pixels_size!=128 || textures.end!=128 || runtime.reads!=1 ||
        (uint32_t)bitmap.pixels_offset!=MCC_BITMAP_BASE || bitmap.tag_index!=7)return 6;
    for(i=0;i<8;i++)if(memcmp(texture.pixels+i*2,runtime.source+order[i]*2,2))return 7;
    for(i=16;i<128;i++)if(texture.pixels[i])return 8;
    if(!bitmap_verify(&bitmap,FALSE))return 9;
    free(texture.pixels);return 0;
}
'''
    work = tmp_path_factory.mktemp("mcc-bitmap-converter")
    path = work / "bitmap.c"
    path.write_text(source)
    output = work / ("bitmap.exe" if sys.platform == "win32" else "bitmap")
    command = [compiler, "-std=c99", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
               "-I", str(ROOT / "port/third_party/bcdec"), str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command += ["-fsanitize=undefined", "-fsanitize-trap=undefined"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


@pytest.mark.parametrize("case", ["known", "ordinary", "wrong-group", "missing-group", "missing-environment",
                                  "unknown-flag", "extra-pixels", "short-pixels", "bad-signature",
                                  "wrong-format", "non-power-two", "mipmaps"])
def test_mcc_lightmap_conversion(bitmap_converter, case):
    result = subprocess.run([str(bitmap_converter), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("case", ["tail-known", "tail-random-short", "tail-short-base",
                                  "tail-wrong-flags", "tail-wrong-format"])
def test_mcc_incomplete_dxt_tail(bitmap_converter, case):
    result = subprocess.run([str(bitmap_converter), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
