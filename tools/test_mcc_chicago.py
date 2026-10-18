"""Check MCC-only black-neutral blending and preservation of legacy draws."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


def test_chicago_blend(tmp_path):
    from harness import function
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    adapter = function((ROOT / "port/linux/game/mcc_chicago.inl").read_text(),
                       "mcc_chicago_blend")
    code = r'''
#include <stdio.h>
#include <string.h>
typedef int boolean;
enum { D3DRS_DESTBLEND, D3DBLEND_ONE=1,
       _framebuffer_blend_function_double_multiply=2,
       NUMBER_OF_SHADER_TRANSPARENT_MAPS=4 };
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
struct shader_transparent_chicago {
 unsigned short extra_flags;short framebuffer_blend_function;
 struct {int count;} maps;
};
struct pixel_shader_definition {unsigned int rgb_inputs[8];};
static int loaded, writes, destination;
static int mcc_cache_tags_loaded(void) {return loaded;}
static void SetRenderStateSmart(int state,int value) {
 (void)state;writes++;destination=value;
}
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}} while (0)
''' + adapter + r'''
int main(void) {
 struct shader_transparent_chicago shader={4,2,{4}};
 struct pixel_shader_definition original={{1,2,3,4,0x0C14A034}},pixel;
 int mode,flag;
 /* A reserved bit in Xbox/CE is not an opt-in. */
 pixel=original;mcc_chicago_blend(&shader,&pixel,1);
 CHECK(!writes && !memcmp(&pixel,&original,sizeof(pixel)));
 loaded=1;
 for(mode=0;mode<8;mode++) for(flag=0;flag<8;flag++) {
  shader.framebuffer_blend_function=mode;shader.extra_flags=flag;
  pixel=original;writes=0;mcc_chicago_blend(&shader,&pixel,1);
  if(mode==2 && (flag&4)) {
   CHECK(writes==1 && destination==D3DBLEND_ONE);
   CHECK(pixel.rgb_inputs[4]==0x0C140000);
   CHECK(!memcmp(pixel.rgb_inputs,original.rgb_inputs,4*sizeof(unsigned int)));
  } else CHECK(!writes && !memcmp(&pixel,&original,sizeof(pixel)));
 }
 shader.framebuffer_blend_function=2;shader.extra_flags=4;
 pixel=original;writes=0;mcc_chicago_blend(&shader,&pixel,0);
 CHECK(writes==1 && !memcmp(&pixel,&original,sizeof(pixel)));
 /* Unloading MCC does not leave adapter state behind. */
 loaded=0;writes=0;pixel=original;mcc_chicago_blend(&shader,&pixel,1);
 CHECK(!writes && !memcmp(&pixel,&original,sizeof(pixel)));
 return 0;
}
'''
    path = tmp_path / "chicago.c"
    path.write_text(code)
    binary = tmp_path / ("chicago.exe" if os.name == "nt" else "chicago")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    result = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
