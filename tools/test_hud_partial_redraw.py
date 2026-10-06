"""Exercise production partial HUD compositing without game assets or a profile.

Checks the seat-icon alignment, every preserved label texel against an
independent bilinear reference, rectangular Morton layouts, and fallbacks.
An optional Xbox map checks the stock CRCs and icon/label separation too.
"""
import argparse
import json
import os
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

import numpy as np
from PIL import Image

from hud_assets import decode_bitmap, level0_size, morton_order

ROOT = Path(__file__).resolve().parent.parent


def block(text, marker):
    start = text.index(marker)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


HARNESS = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hud_hires.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); exit(1); } } while (0)
COMPOSE
int main(int argc, char **argv) {
 unsigned int w=atoi(argv[3]),h=atoi(argv[4]),scale=atoi(argv[5]);
 struct hud_hires_embedded e={.original_width=w,.original_height=h,.replace={1,1,w-1,h-1}};
 unsigned char *source=malloc(w*h),*pixels=malloc(w*h*scale*scale*4);
 FILE *f=fopen(argv[1],"rb");CHECK(f);CHECK(fread(source,1,w*h,f)==w*h);fclose(f);
 memset(pixels,17,w*h*scale*scale*4);
 CHECK(hud_hires_preserve_ay8(pixels,w*scale,h*scale,source,&e));
 f=fopen(argv[2],"wb");CHECK(f);CHECK(fwrite(pixels,1,w*h*scale*scale*4,f)==w*h*scale*scale*4);fclose(f);
 unsigned char sentinel[64];memset(sentinel,93,sizeof(sentinel));
 CHECK(!hud_hires_preserve_ay8(sentinel,w*scale,h*scale,NULL,&e));
 CHECK(!hud_hires_preserve_ay8(sentinel,w*scale+1,h*scale,source,&e));
 CHECK(!hud_hires_preserve_ay8(sentinel,w*16,h*16,source,&e));
 e.replace[2]=w+1;CHECK(!hud_hires_preserve_ay8(sentinel,w*scale,h*scale,source,&e));
 e.replace[2]=e.replace[0];CHECK(!hud_hires_preserve_ay8(sentinel,w*scale,h*scale,source,&e));
 e.original_width=3;CHECK(!hud_hires_preserve_ay8(sentinel,w*scale,h*scale,source,&e));
 e.original_width=0;CHECK(!hud_hires_preserve_ay8(sentinel,w*scale,h*scale,source,&e));
 e.original_height=0;CHECK(hud_hires_preserve_ay8(sentinel,w*scale,h*scale,NULL,&e));
 for(int i=0;i<64;i++)CHECK(sentinel[i]==93);
 free(source);free(pixels);return 0;
}
'''

GL_HARNESS = r'''
#define _CRT_SECURE_NO_WARNINGS
#define __HALO_LINUX_PLATFORM_H
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
typedef unsigned long DWORD, D3DCOLOR; typedef int BOOL;
#define D3DRS_PS_MAX 256
#define TRUE 1
#define FALSE 0
#include "xgpu.h"
#include "hud_hires.h"
#define GL_DEFINE(name) __typeof__(name) name;
GL_FUNCTIONS(GL_DEFINE)
#undef GL_DEFINE
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); exit(1); } } while (0)
static int enabled=1,selected; static unsigned long revision;
int config_boolean(const char *name) { return enabled; }
unsigned long config_changes(void) { return revision; }
void platform_log(const char *format, ...) { }
void xgpu_gl_state_invalidate(void) { }
long hud_hires_asset_at(unsigned long address,long w,long h) {return w==128 && h==32 ? selected : -1;}
ASSETS
PRODUCTION
int main(int argc,char **argv) {
 CHECK(SDL_Init(SDL_INIT_VIDEO));SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,5);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 SDL_Window *window=SDL_CreateWindow("Seat HUD",16,16,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);CHECK(window);
 SDL_GLContext context=SDL_GL_CreateContext(window);CHECK(context);
#define LOAD(name) name=(__typeof__(name))SDL_GL_GetProcAddress(#name);CHECK(name);
 GL_FUNCTIONS(LOAD)
#undef LOAD
 unsigned char source[4096];FILE *f=fopen(argv[1],"rb");CHECK(f);CHECK(fread(source,1,4096,f)==4096);fclose(f);
 unsigned char *actual=malloc(1024*256*4);GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 for(selected=0;selected<3;selected++) {
  CHECK(hud_hires_override_find((unsigned long)source,128,32,4096)==selected);
  enabled=0;revision++;CHECK(hud_hires_override_find((unsigned long)source,128,32,4096)==-1);
  enabled=1;revision++;source[0]^=1;CHECK(hud_hires_override_find((unsigned long)source,128,32,4096)==-1);source[0]^=1;
  CHECK(hud_hires_override_find((unsigned long)source,64,32,4096)==-1);
  unsigned long levels=0;GLuint texture=hud_hires_override_texture(selected,source,&levels);CHECK(texture && levels==11);
  CHECK(hud_hires_override_texture(selected,NULL,&levels)==texture); /* cached: no guest pointer retained */
  unsigned long w,h;unsigned char *expected=png_decode((const unsigned char *)hud_hires_embedded[selected].png,hud_hires_embedded[selected].png_size,&w,&h);
  CHECK(expected && w==1024 && h==256);
  CHECK(hud_hires_preserve_ay8(expected,w,h,source,&hud_hires_embedded[selected]));
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
  CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
  glReadPixels(0,0,1024,256,GL_RGBA,GL_UNSIGNED_BYTE,actual);CHECK(!memcmp(actual,expected,1024*256*4));
  CHECK(glGetError()==GL_NO_ERROR);free(expected);
 }
 free(actual);SDL_GL_DestroyContext(context);SDL_DestroyWindow(window);SDL_Quit();
 puts("PASS: real PNG decode/GL upload/cache for both seat patches and unchanged full redraw; disabled, modified and size-mismatch guards");return 0;
}
'''


def test_upload(compiler, entries):
    original = (np.arange(4096)*37 % 256).astype(np.uint8)
    crc = zlib.crc32(original.tobytes())
    png = (ROOT/f'port/assets/hud/{entries[0]["name"]}.png').read_bytes()
    words = struct.unpack('<'+'I'*((len(png)+3)//4), png+b'\0'*(-len(png)%4))
    assets = 'static const unsigned int png[]={' + ','.join(map(str,words)) + '};\n'
    assets += 'const unsigned int hud_hires_embedded_count=3;\nconst struct hud_hires_embedded hud_hires_embedded[]={\n'
    for entry in [*entries, dict(replace=[0,0,0,0])]:
        w, h = (128,32) if any(entry['replace']) else (0,0)
        assets += '{.tag="seat",.width=1024,.height=256,.crc='+str(crc)+',.png=png,.png_size='+str(len(png))
        assets += ',.original_width='+str(w)+',.original_height='+str(h)+',.replace={' + ','.join(map(str,entry['replace']))+'}},\n'
    assets += '};\n'
    production = (ROOT/'port/linux/src/hud_hires.c').read_text()
    # Link the same zlib implementation that this checkout builds.
    if 'zlib_prefixed.h' in production:
        zlib_dir = ROOT/'port/third_party/zlib'
        zlib_includes = ['-I'+str(zlib_dir), '-DZ_PREFIX', '-Dz_errmsg=z_port_errmsg']
        zlib_names = ['adler32','crc32','inflate','inffast','inftrees','uncompr','zutil']
    else:
        zlib_dir = ROOT/'source/memory/zlib'
        zlib_includes = ['-I'+str(ROOT/'source')]
        zlib_names = ['adler32','crc32','inflate','infblock','infcodes','inffast','inftrees','infutil','uncompr','zutil']
    zlib_sources = [zlib_dir/(name+'.c') for name in zlib_names]
    env = dict(os.environ)
    if sys.platform == 'win32':
        sdl = next((ROOT/'build/windows/third_party').glob('SDL3-*/include')).parent
        includes = ['-I'+str(sdl/'include')]
        libraries = [str(sdl/'lib/x86/SDL3.lib')]
        env['PATH'] = str(sdl/'lib/x86') + os.pathsep + env['PATH']
    else:
        includes = subprocess.check_output(['pkg-config','--cflags','sdl3'],text=True).split()
        libraries = subprocess.check_output(['pkg-config','--libs','sdl3'],text=True).split()
    with tempfile.TemporaryDirectory(prefix='halo-seat-gl-') as directory:
        p = Path(directory)
        (p/'cseries.h').write_text('#include <stdlib.h>\n#define TRUE 1\n'
                                   '#define debug_malloc(n,zero,file,line) calloc(1,n)\n'
                                   '#define debug_free(p,file,line) free(p)\n')
        (p/'upload.c').write_text(GL_HARNESS.replace('ASSETS',assets).replace('PRODUCTION',production))
        (p/'original.raw').write_bytes(original.tobytes())
        subprocess.run([*compiler, '-I'+str(p), *includes, *zlib_includes, '-Wno-deprecated-non-prototype',
                        str(p/'upload.c'), *map(str,zlib_sources), *libraries, '-o',str(p/'upload.exe')],check=True)
        subprocess.run([str(p/'upload.exe'),str(p/'original.raw')],env=env,check=True,timeout=45)


def stock_bitmaps(path):
    data = path.read_bytes()
    u = lambda offset: struct.unpack_from('<I', data, offset)[0]
    tag_offset = u(16)
    if tag_offset >= len(data):
        data = data[:2048] + zlib.decompress(data[2048:])
    base = u(tag_offset) - 36
    offset = lambda address: address - base + tag_offset
    for i in range(u(tag_offset + 12)):
        tag = tag_offset + 36 + i * 32
        if data[tag:tag+4] != b'mtib':
            continue
        name = offset(u(tag+16))
        if data[name:data.index(b'\0', name)] != b'ui\\hud\\bitmaps\\combined\\hud_unit_backgrounds':
            continue
        group = offset(u(tag+20))
        for index in (9, 10):
            bitmap = offset(u(group+100)) + index*48
            w, h, _, _, fmt, flags = struct.unpack_from('<hhhhhH', data, bitmap+4)
            position, size = struct.unpack_from('<ii', data, bitmap+24)
            yield index, dict(width=w, height=h, format=fmt, flags=flags,
                              pixels=data[position:position+size])
        return
    raise AssertionError('No stock seat HUD bitmap group in this map')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--map', type=Path)
    args = parser.parse_args()
    entries = [e for e in json.loads((ROOT/'port/assets/hud/layout.json').read_text())['assets'] if 'replace' in e]
    assert {e['bitmap'] for e in entries} == {9, 10}
    meter = np.array(Image.open(ROOT/'port/assets/hud/hud_unit_meters__0.png'))
    for entry in entries:
        art = np.array(Image.open(ROOT/f'port/assets/hud/{entry["name"]}.png'))
        assert art.shape == (256, 1024, 4)
        assert np.array_equal(art[..., 0], art[..., 3]), 'AY8 redraw must be premultiplied'
        x0, y0, x1, y1 = [v*8 for v in entry['replace']]
        outside = art.copy(); outside[y0:y1, x0:x1] = 0
        assert not outside.any(), 'Patch would overwrite seat label or another region'
        outline = art[y0:y1, x0:x1, 3] > 8
        fill = meter[34*8+y0-80:34*8+y1-80, 122*8+x0:122*8+x1, 3] > 0
        assert outline.any() and np.all(fill[outline]), 'Outline must lie on its matching meter contour'
    if args.map:
        for index, bitmap in stock_bitmaps(args.map):
            entry = next(e for e in entries if e['bitmap'] == index)
            assert bitmap['format'] == 2 and not bitmap['flags'] & 16
            assert zlib.crc32(bitmap['pixels'][:level0_size(bitmap)]) == entry['crc']
            decoded = decode_bitmap(bitmap)
            x0, y0, x1, y1 = entry['replace']
            assert not decoded[:, x0:x1, 3][:y0].any() and not decoded[y1:, x0:x1, 3].any()
            assert decoded[:, :x0, 3].any(), 'Seat label must stay outside the replaced icon'
    source = (ROOT/'port/linux/src/hud_hires.c').read_text()
    unit = HARNESS.replace('COMPOSE', block(source, 'static int hud_hires_preserve_ay8('))
    compiler = [args.cc, '-std=gnu11', '-O2', '-fuse-ld=lld', '-I'+str(ROOT/'port/linux/src')]
    if sys.platform == 'win32':
        compiler += ['--target=i686-pc-windows-msvc']
    with tempfile.TemporaryDirectory(prefix='halo-seat-hud-') as directory:
        p = Path(directory)
        (p/'test.c').write_text(unit)
        subprocess.run([*compiler, str(p/'test.c'), '-o', str(p/'test.exe')], check=True)
        for w, h in ((4,4), (8,4), (4,8), (128,32)):
            original = (np.arange(w*h).reshape(h,w)*37 % 256).astype(np.uint8)
            swizzled = np.empty(w*h, np.uint8); swizzled[morton_order(w,h)] = original
            (p/'original.raw').write_bytes(swizzled.tobytes())
            for scale in (1,2,8):
                subprocess.run([str(p/'test.exe'), str(p/'original.raw'), str(p/'out.raw'),
                                str(w), str(h), str(scale)], check=True)
                actual = np.frombuffer((p/'out.raw').read_bytes(), np.uint8).reshape(h*scale,w*scale,4)
                x, y = np.meshgrid(np.maximum((np.arange(w*scale)+.5)/scale-.5,0),
                                   np.maximum((np.arange(h*scale)+.5)/scale-.5,0))
                x0, y0 = x.astype(int), y.astype(int)
                x1, y1 = np.minimum(x0+1,w-1), np.minimum(y0+1,h-1)
                fx, fy = x-x0, y-y0
                expected = np.floor(original[y0,x0]*(1-fx)*(1-fy)+original[y0,x1]*fx*(1-fy)+
                                    original[y1,x0]*(1-fx)*fy+original[y1,x1]*fx*fy+.5).astype(np.uint8)
                expected[scale:(h-1)*scale, scale:(w-1)*scale] = 17
                assert np.all(actual == expected[...,None]), (w,h,scale)
    print('PASS: two matching seat outlines; original labels preserved, 12 rectangular Morton/bilinear cases; invalid and full-redraw fallbacks')
    test_upload(compiler, entries)


if __name__ == '__main__':
    main()
