"""Lazy render-target allocation must preserve already selected texture inputs."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def target_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if compiler is None:
        pytest.skip("clang is required")
    directory = tmp_path_factory.mktemp("render-target-binding")
    source = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    (directory / "under_test.inc").write_text("\n".join(
        function(source, name) for name in
        ["render_target_bucket", "render_target_remember", "render_target_storage", "render_target_get"]))
    (directory / "target.c").write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef int BOOL, GLint, GLsizei;
typedef unsigned int GLuint, GLenum;
typedef unsigned long DWORD;
#define TRUE 1
#define FALSE 0
#define SCREEN_HEIGHT 480
#define SECONDARY_TARGET_WIDTH 320
#define SECONDARY_TARGET_HEIGHT 240
#define RENDER_TARGET_BUCKET_COUNT 256
#define RECENT_RENDER_TARGET_COUNT 4
enum { GL_TEXTURE_2D=1, GL_TEXTURE_BINDING_2D, GL_TEXTURE_MAX_LEVEL,
    GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8, GL_RGBA16F, GL_RGBA8,
    GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
    GL_RGBA, GL_HALF_FLOAT, GL_BGRA, GL_UNSIGNED_BYTE };
enum render_target_kind { _render_target_plain, _render_target_screen, _render_target_shadow_map };
typedef struct { DWORD Data, Format, Size; unsigned long width,height; BOOL depth; } D3DSurface;
struct xgpu_render_target {
    unsigned long data,width,height,gl_width,gl_height;
    BOOL depth; float scale[2]; GLuint texture; GLenum format;
};
struct render_target_entry {
    struct render_target_entry *next,*next_in_bucket;
    struct xgpu_render_target target;
    unsigned long last_rendered; BOOL screen_buffer; enum render_target_kind kind;
};
static struct render_target_entry *render_targets;
static struct render_target_entry *render_target_buckets[RENDER_TARGET_BUCKET_COUNT];
static struct { DWORD data,format,size; long screen_width; float scale[2];
    struct render_target_entry *entry; } recent_render_targets[RECENT_RENDER_TARGET_COUNT];
static unsigned long recent_render_target_next;
static float screen_scale[2]={1,1};
static struct { D3DSurface back_buffer,depth_buffer; } device;
static struct { int active; } hdr;
static int xgpu_reversed_depth;
static unsigned active_unit=3, bindings[4]={81,82,83,99};
static unsigned next_texture=1000, creates,storage_calls,queries,binds,invalidations;
static unsigned stored_texture;
static GLenum stored_format;
static long halo_screen_width(void) { return 640; }
static int halo_shadow_map_scale(void) { return 1; }
static int surface_is_shadow_map(const D3DSurface *surface) { (void)surface; return 0; }
static void surface_dimensions(const D3DSurface *surface,unsigned long *width,unsigned long *height,BOOL *depth)
{ *width=surface->width;*height=surface->height;*depth=surface->depth; }
static void glGetIntegerv(GLenum name,GLint *value)
{ assert(name==GL_TEXTURE_BINDING_2D);queries++;*value=(GLint)bindings[active_unit]; }
static void glGenTextures(int count,GLuint *texture)
{ assert(count==1);creates++;*texture=next_texture++; }
static void glBindTexture(GLenum target,GLuint texture)
{ assert(target==GL_TEXTURE_2D);binds++;bindings[active_unit]=texture; }
static void glTexParameteri(GLenum target,GLenum name,GLint value)
{ assert(target==GL_TEXTURE_2D && name==GL_TEXTURE_MAX_LEVEL && value==0); }
static void glTexImage2D(GLenum target,int level,GLenum format,GLsizei width,GLsizei height,
    int border,GLenum channels,GLenum type,const void *data)
{
    assert(target==GL_TEXTURE_2D && !level && width==128 && height==128 && !border && !data);
    assert(bindings[active_unit]>=1000);
    assert((format==GL_RGBA8 && channels==GL_BGRA && type==GL_UNSIGNED_BYTE) ||
        (format==GL_DEPTH24_STENCIL8 && channels==GL_DEPTH_STENCIL && type==GL_UNSIGNED_INT_24_8));
    stored_texture=bindings[active_unit];stored_format=format;storage_calls++;
}
static void xgpu_gl_state_invalidate(void) { invalidations++; }
#include "under_test.inc"

static D3DSurface surface(unsigned long data,int depth)
{ D3DSurface s={0};s.Data=data;s.Format=depth?2:1;s.Size=7;s.width=s.height=128;s.depth=depth;return s; }
static void created(int depth,unsigned input,unsigned unit)
{
    D3DSurface s=surface(0x1000,depth);
    struct render_target_entry *entry;
    unsigned original[4], before_unit;
    active_unit=unit;bindings[unit]=input;before_unit=active_unit;
    memcpy(original,bindings,sizeof(bindings));
    entry=render_target_get(&s);
    assert(entry && creates==1 && storage_calls==1 && invalidations==1);
    assert(entry->target.texture==stored_texture && stored_format==(depth?GL_DEPTH24_STENCIL8:GL_RGBA8));
    /* Sampling input must survive allocating a distinct output image. The
     * real failure sampled the new FBO attachment instead (WebGL feedback). */
    assert(active_unit==before_unit && !memcmp(original,bindings,sizeof(bindings)));
    assert(bindings[unit]!=entry->target.texture);
    assert(queries==1 && binds==2);
}
static void cached(int recent)
{
    D3DSurface s=surface(0x2000,0);
    struct render_target_entry entry;
    memset(&entry,0,sizeof(entry));entry.target.data=s.Data;entry.target.width=entry.target.height=128;
    entry.target.scale[0]=entry.target.scale[1]=1;entry.target.texture=99;
    if(recent)render_target_remember(&s,640,&entry);else *render_target_bucket(s.Data)=&entry;
    /* Intentional feedback is left for the existing feedback-copy path:
     * lookup itself neither substitutes the input nor queries GL state. */
    assert(render_target_get(&s)==&entry);
    assert(!queries && !binds && !creates && !storage_calls && bindings[3]==99);
}
int main(int argc,char **argv)
{
    struct render_target_entry *entry,*next;
    assert(argc==2);
    if(!strcmp(argv[1],"color"))created(0,99,3);
    else if(!strcmp(argv[1],"depth"))created(1,99,3);
    else if(!strcmp(argv[1],"unbound"))created(0,0,3);
    else if(!strcmp(argv[1],"unit-zero"))created(0,81,0);
    else if(!strcmp(argv[1],"recent"))cached(1);
    else if(!strcmp(argv[1],"bucket"))cached(0);
    else if(!strcmp(argv[1],"absent")) {
        D3DSurface s=surface(0,0);
        assert(!render_target_get(NULL) && !render_target_get(&s));
        assert(!queries && !binds && !creates);
    } else if(!strcmp(argv[1],"color-depth")) {
        D3DSurface color=surface(0x3000,0),depth=surface(0x4000,1);
        struct render_target_entry *c=render_target_get(&color),*d=render_target_get(&depth);
        assert(c!=d && creates==2 && storage_calls==2 && queries==2 && binds==4);
        assert(bindings[3]==99 && bindings[0]==81 && active_unit==3);
    } else if(!strcmp(argv[1],"explicit-bind")) {
        D3DSurface s=surface(0x5000,0);
        entry=render_target_get(&s);
        assert(bindings[3]==99);
        glBindTexture(GL_TEXTURE_2D,entry->target.texture);
        assert(bindings[3]==entry->target.texture);
    } else return 2;
    for(entry=render_targets;entry;entry=next) { next=entry->next;free(entry); }
    return 0;
}
''')
    binary = directory / ("target.exe" if os.name == "nt" else "target")
    command = [compiler, "-m32", "-std=gnu89", "-O1", "-Wall", "-Werror",
               "-Wno-unused-function", str(directory / "target.c"), "-o", str(binary)]
    if os.name == "nt":
        command.append("-fuse-ld=lld")
    subprocess.run(command, check=True, capture_output=True, text=True)
    return binary


@pytest.mark.parametrize("case", ["color", "depth", "unbound", "unit-zero", "recent",
                                 "bucket", "absent", "color-depth", "explicit-bind"])
def test_target_allocation_preserves_sample_binding(target_tool, case):
    subprocess.run([str(target_tool), case], check=True, capture_output=True, timeout=10)
