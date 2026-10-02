#!/usr/bin/env python3
"""Exercise the production alpha-mask decoder with partial mip chains."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
source = (root / "port/linux/game/raytrace_world.c").read_text()
registry = re.search(r"static struct\s*\{[^}]*\} masks;", source).group(0)
start = source.index("boolean halo_ray_tracing_mask(")
brace = source.index("{", start)
end, depth = brace + 1, 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1
decoder = source[start:end]
fixture = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int boolean;
typedef struct { float x,y; } real_point2d;
struct bitmap_data { long width,height,mipmap_count; };
#define RAY_MASKS 256
#define TRUE 1
#define FALSE 0
#define MAX(a,b) ((a)>(b)?(a):(b))
static float last_lod;
static long reads;
static boolean level_bitmap_readable(struct bitmap_data *b) { return TRUE; }
static unsigned long bitmap_2d_get_pixel(struct bitmap_data *b, real_point2d *p, float lod) {
    assert(isfinite(lod) && lod>=0 && lod<=1);
    assert(p->x>=0 && p->x<=1 && p->y>=0 && p->y<=1);
    last_lod=lod; reads++;
    return 0x73ffffffUL;
}
'''
checks = r'''
static void check(long width,long height,long mips,int expected,long ew,long eh) {
    struct bitmap_data b={width,height,mips};
    const unsigned char *pixels=NULL;
    long index=-1,w=0,h=0;
    unsigned long generation=0;
    memset(&masks,0,sizeof(masks));
    masks.bitmaps[0]=&b;masks.count=1;masks.generation=42;
    reads=0;last_lod=-1;
    assert(halo_ray_tracing_mask(&index,&pixels,&w,&h,&generation)==expected);
    assert(generation==42);
    if(expected) {
        assert(index==0 && w==ew && h==eh && reads==w*h);
        assert(pixels[0]==0x73 && pixels[w*h-1]==0x73);
        assert(!halo_ray_tracing_mask(&index,&pixels,&w,&h,&generation));
    } else assert(reads==0);
}
int main(void) {
    check(256,256,0,0,0,0);
    check(256,256,1,1,128,128);assert(last_lod==0);
    check(256,256,2,1,128,128);
    check(256,256,3,1,128,128);
    check(1024,1024,3,1,128,128);assert(last_lod==0);
    check(1024,512,3,1,128,64);assert(last_lod==0);
    check(1024,1024,2,0,0,0);
    check(16,16,0,1,16,16);assert(last_lod==1);
    puts("PASS: alpha mask final mip, partial chains, no mip and rectangular textures");
}
'''
with tempfile.TemporaryDirectory(prefix="halo-mask-lod-") as folder:
    c = Path(folder) / "test.c"
    binary = Path(folder) / "test"
    c.write_text(fixture + registry + decoder + checks)
    subprocess.run(["clang", "-std=c11", "-O2", "-fsanitize=undefined,float-divide-by-zero", str(c), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
