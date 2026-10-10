"""MCC texture lifetime/pressure tests with real descriptors and a fake GPU.

Compile the actual private bridge and native descriptor/size routines. GPU
stubs record ownership and failures; no legacy staging allocator is linked.
"""
from pathlib import Path
import re
import shutil
import subprocess
import sys

import pytest

from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def bridge_tool(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for the MCC texture bridge tests")
    work = tmp_path_factory.mktemp("mcc-texture-bridge")
    native = (ROOT / "port/linux/src/xbox_textures.c").read_text()
    xgpu = (ROOT / "port/linux/src/xgpu.h").read_text()
    xdk = (ROOT / "port/include/xdk/xdk_d3d8.h").read_text()
    pdb = (ROOT / "port/include/xdk/xdk_pdb.h").read_text()
    macros = "\n".join(line for line in xdk.splitlines() if re.match(
        r"#define (D3DFORMAT_|D3DSIZE_|D3DCOMMON_TYPE_TEXTURE|D3DTEXTURE_(?:PITCH|CUBEFACE)_ALIGNMENT)", line))
    formats = "\n".join("#define " + name + " " + value for name, value in re.findall(
        r"\b(D3DFMT_\w+)\s*=\s*(0x[0-9a-fA-F]+|[0-9]+)\b", pdb))
    header = r'''
#ifndef TEST_XGPU_H
#define TEST_XGPU_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned long DWORD,D3DCOLOR;
typedef unsigned GLuint,GLenum;
typedef int BOOL,boolean;
#define TRUE 1
#define FALSE 0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_CUBE_MAP 0x8513
''' + macros + '\n' + formats + '\n' + structure(xgpu, "xgpu_texture_description") + r'''
void xgpu_texture_describe(DWORD,DWORD,struct xgpu_texture_description *);
unsigned long xgpu_texture_face_size(struct xgpu_texture_description const *);
void glGenTextures(int,GLuint *);
void glDeleteTextures(int,GLuint const *);
void xgpu_gl_state_invalidate(void);
void xgpu_mcc_texture_unbind(DWORD const *);
BOOL xgpu_mcc_texture_upload(GLuint,GLenum,struct xgpu_texture_description const *,void const *,unsigned long,D3DCOLOR const *);
#endif
'''
    (work / "xgpu.h").write_text(header)
    helpers = re.search(r"enum texel_kind\s*\{.*?\};", native, re.S).group()
    helpers += '\n' + structure(native, "format_information")
    helpers += '\n' + '\n'.join(function(native, name) for name in ["format_information",
        "kind_compressed", "floor_log2", "level_dimension", "xgpu_texture_describe", "level_bytes",
        "xgpu_texture_level_offset", "xgpu_texture_face_size"])
    bridge = (ROOT / "port/linux/src/mcc_texture_bridge.c").read_text()
    game = (ROOT / "port/linux/game/mcc_texture_cache.c").read_text()
    source = r'''
#include "xgpu.h"
#include <stdio.h>
static unsigned allocations,live_allocations,largest_allocation,fail_allocate;
static unsigned generated,deleted,uploads,unbound,invalidated,fail_upload,fail_generate;
static void const *last_pixels;
static unsigned long last_bytes;
static void *tracked_calloc(size_t count,size_t bytes) {
    void *p;
    if(fail_allocate)return NULL;
    if(count*bytes>largest_allocation)largest_allocation=(unsigned)(count*bytes);
    p=calloc(count,bytes);if(p){allocations++;live_allocations++;}return p;
}
static void tracked_free(void *p) {if(p)live_allocations--;free(p);}
void glGenTextures(int count,GLuint *p) {(void)count;*p=fail_generate?0:++generated;}
void glDeleteTextures(int count,GLuint const *p) {(void)count;if(!*p)exit(80);deleted++;}
void xgpu_gl_state_invalidate(void) {invalidated++;}
void xgpu_mcc_texture_unbind(DWORD const *p) {if(!p)exit(81);unbound++;}
BOOL xgpu_mcc_texture_upload(GLuint texture,GLenum target,struct xgpu_texture_description const *d,
    void const *pixels,unsigned long bytes,D3DCOLOR const *palette) {
    (void)texture;(void)target;(void)d;(void)palette;
    uploads++;last_pixels=pixels;last_bytes=bytes;return !fail_upload;
}
''' + helpers + r'''
#define calloc tracked_calloc
#define free tracked_free
''' + bridge + r'''
#undef calloc
#undef free
struct bitmap_data {short width,height,depth,type,format,mipmap_count;unsigned short flags;void *base_address;};
static int active=1;
static void const *owned;
static unsigned long owned_bytes;
static int mcc_cache_tags_loaded(void) {return active;}
static void const *mcc_cache_bitmap_pixels(struct bitmap_data const *b,unsigned long *bytes) {
    (void)b;*bytes=owned_bytes;return owned;
}
static unsigned rasterizer_xbox_bitmap_get_max_mipmap_count(struct bitmap_data *b) {return b->mipmap_count;}
static unsigned bitmap_mipmap_get_row_pitch(struct bitmap_data *b,int level) {(void)level;return b->width*4;}
''' + function(game, "mcc_texture_cache_get") + r'''
static DWORD format(unsigned logw,unsigned logh,unsigned logd,unsigned levels,unsigned type) {
    return (D3DFMT_A8R8G8B8<<D3DFORMAT_FORMAT_SHIFT)|(logw<<D3DFORMAT_USIZE_SHIFT)|
        (logh<<D3DFORMAT_VSIZE_SHIFT)|(logd<<D3DFORMAT_PSIZE_SHIFT)|(levels<<D3DFORMAT_MIPMAP_SHIFT)|
        ((type==1?3:2)<<D3DFORMAT_DIMENSION_SHIFT)|(type==2?D3DFORMAT_CUBEMAP:0);
}
static int get(DWORD const *resource,GLuint *texture,GLenum *target) {
    struct xgpu_texture_description d;
    return xgpu_mcc_texture_get(resource,NULL,texture,target,&d);
}
int main(int argc,char **argv) {
    unsigned char pixels[1024]={0};int owner=0,other=0;
    DWORD *header;GLuint texture=0;GLenum target=0;
    if(argc!=2)return 100;
    if(!strcmp(argv[1],"identity")) {
        DWORD copy[5];
        header=mcc_texture_bridge_register(&owner,format(2,2,0,1,0),0,pixels,sizeof(pixels));
        if(!header||header!=mcc_texture_bridge_find(&owner)||mcc_texture_bridge_find(&other))return 1;
        if(header!=mcc_texture_bridge_register(&owner,0,0,NULL,0)||allocations!=1)return 2;
        memcpy(copy,header,sizeof(copy));
        if(get(copy,&texture,&target)||get((DWORD const *)(uintptr_t)1,&texture,&target)||get(NULL,&texture,&target))return 3;
        if(!get(header,&texture,&target)||!texture||target!=GL_TEXTURE_2D||uploads!=1||last_pixels!=pixels)return 4;
        if(!get(header,&texture,&target)||uploads!=1)return 5;
    } else if(!strcmp(argv[1],"bounds")) {
        if(mcc_texture_bridge_register(NULL,format(2,2,0,1,0),0,pixels,sizeof(pixels))||
           mcc_texture_bridge_register(&owner,format(2,2,0,1,0),0,NULL,sizeof(pixels))||
           mcc_texture_bridge_register(&owner,format(2,2,0,1,0),0,pixels,63)||
           mcc_texture_bridge_register(&owner,format(2,2,0,1,2),0,pixels,767)||
           mcc_texture_bridge_register(&owner,format(13,2,0,1,0),0,pixels,0xffffffffUL)||
           mcc_texture_bridge_register(&owner,format(10,2,2,1,1),0,pixels,0xffffffffUL)||
           mcc_texture_bridge_register(&owner,format(2,2,0,14,0),0,pixels,0xffffffffUL))return 6;
        fail_allocate=1;
        if(mcc_texture_bridge_register(&owner,format(2,2,0,1,0),0,pixels,sizeof(pixels)))return 7;
        if(allocations||generated||uploads)return 8;
    } else if(!strcmp(argv[1],"failure")) {
        header=mcc_texture_bridge_register(&owner,format(2,2,0,1,0),0,pixels,sizeof(pixels));
        fail_generate=1;
        if(!get(header,&texture,&target)||texture||uploads||deleted)return 9;
        fail_generate=0;fail_upload=1;
        if(!get(header,&texture,&target)||texture||uploads!=1||deleted!=1||invalidated!=1)return 10;
        fail_upload=0;
        if(!get(header,&texture,&target)||!texture||uploads!=2||deleted!=1||last_bytes!=sizeof(pixels))return 11;
    } else if(!strcmp(argv[1],"dispose")) {
        header=mcc_texture_bridge_register(&owner,format(2,2,0,1,0),0,pixels,sizeof(pixels));
        if(!get(header,&texture,&target))return 12;
        mcc_texture_bridge_dispose();
        if(live_allocations||deleted!=1||unbound!=1||invalidated!=1||mcc_texture_bridge_find(&owner)||get(header,&texture,&target))return 13;
        header=mcc_texture_bridge_register(&owner,format(2,2,0,1,0),0,pixels,sizeof(pixels));
        if(!get(header,&texture,&target)||uploads!=2||generated!=2)return 14;
    } else if(!strcmp(argv[1],"palette")) {
        D3DCOLOR palette[256]={0};struct xgpu_texture_description d;
        DWORD f=(format(2,2,0,1,0)&~D3DFORMAT_FORMAT_MASK)|(D3DFMT_P8<<D3DFORMAT_FORMAT_SHIFT);
        header=mcc_texture_bridge_register(&owner,f,0,pixels,sizeof(pixels));
        if(!xgpu_mcc_texture_get(header,palette,&texture,&target,&d)||uploads!=1)return 24;
        if(!xgpu_mcc_texture_get(header,palette,&texture,&target,&d)||uploads!=1)return 25;
        palette[27]=0xff00ff00;
        if(!xgpu_mcc_texture_get(header,palette,&texture,&target,&d)||uploads!=2||deleted!=1)return 26;
        if(!get(header,&texture,&target)||uploads!=3||deleted!=2)return 27;
    } else if(!strcmp(argv[1],"descriptors")) {
        struct bitmap_data b[4]={{4,4,1,0,11,2,8,NULL},{3,2,1,0,11,0,16,NULL},
            {4,4,1,2,11,0,8,NULL},{2,2,2,1,11,1,8,NULL}};
        unsigned long expected_bytes[]={84,128,768,36};
        GLenum expected_targets[]={GL_TEXTURE_2D,GL_TEXTURE_2D,GL_TEXTURE_CUBE_MAP,GL_TEXTURE_3D};
        unsigned i;
        owned=pixels;owned_bytes=sizeof(pixels);
        if(mcc_texture_cache_get(&b[0],FALSE))return 15;
        active=0;if(mcc_texture_cache_get(&b[0],TRUE))return 16;active=1;
        for(i=0;i<4;i++) {
            struct xgpu_texture_description d;
            header=mcc_texture_cache_get(&b[i],TRUE);
            if(!header||b[i].base_address!=pixels||header!=mcc_texture_cache_get(&b[i],FALSE)||
                !xgpu_mcc_texture_get(header,NULL,&texture,&target,&d)||target!=expected_targets[i]||
                d.width!=(unsigned)b[i].width||d.height!=(unsigned)b[i].height||
                d.depth!=(unsigned)b[i].depth||d.levels!=(unsigned)b[i].mipmap_count+1||
                xgpu_texture_face_size(&d)*(d.cube_map?6:1)!=expected_bytes[i])return 17;
        }
    } else if(!strcmp(argv[1],"pressure")) {
        enum {count=13,chain_bytes=22369664};
        unsigned char *data[count];DWORD *headers[count];unsigned i,frame;
        for(i=0;i<count;i++) {
            data[i]=malloc(chain_bytes);if(!data[i])return 18;
            data[i][0]=(unsigned char)i;data[i][chain_bytes-1]=(unsigned char)(255-i);
            headers[i]=mcc_texture_bridge_register(&data[i],format(11,11,0,12,0),0,data[i],chain_bytes);
            if(!headers[i])return 19;
        }
        if(largest_allocation>2048 || allocations!=count || live_allocations!=count)return 20;
        for(frame=0;frame<3;frame++)for(i=0;i<count;i++) {
            unsigned n=frame&1?count-1-i:i;
            if(!get(headers[n],&texture,&target)||!texture||target!=GL_TEXTURE_2D||
                data[n][0]!=n||data[n][chain_bytes-1]!=255-n)return 21;
        }
        if(uploads!=count||generated!=count||last_bytes!=chain_bytes)return 22;
        mcc_texture_bridge_dispose();
        if(deleted!=count||unbound!=count||live_allocations)return 23;
        for(i=0;i<count;i++)free(data[i]);
    } else return 101;
    mcc_texture_bridge_dispose();
    return live_allocations?102:0;
}
'''
    path = work / "bridge_check.c"
    path.write_text(source)
    binary = work / ("bridge_check.exe" if sys.platform == "win32" else "bridge_check")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror", "-I", str(work),
               "-I", str(ROOT / "port/linux/src"), str(path), "-o", str(binary)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    else:
        command[1:1] = ["-m32"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


@pytest.mark.parametrize("case", ["identity", "bounds", "failure", "dispose", "palette", "descriptors", "pressure"])
def test_mcc_texture_bridge(bridge_tool, case):
    result = subprocess.run([str(bridge_tool), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, f"{case}: exit {result.returncode}\n{result.stdout}{result.stderr}"
