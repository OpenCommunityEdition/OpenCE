"""Android CE stable storage, upload reuse, HUD residency and state isolation."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import pytest
from harness import function
ROOT = Path(__file__).resolve().parents[1]

def run_c(tmp_path, code):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    path = tmp_path / "cache.c"
    path.write_text(code)
    exe = tmp_path / ("cache.exe" if os.name == "nt" else "cache")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if os.name == "nt" else ["-m32"]
    result = subprocess.run([compiler, *target, "-std=gnu89", "-Werror=format", str(path), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


GPU_FIXTURE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef unsigned int DWORD, GLuint, GLenum, D3DCOLOR;
typedef int BOOL;
typedef unsigned long long Uint64;
#define TRUE 1
#define FALSE 0
#define GL_TEXTURE_2D 1
#define GL_TEXTURE_CUBE_MAP 2
#define GL_TEXTURE_3D 3
#define D3DFORMAT_FORMAT_MASK 255
#define D3DFORMAT_FORMAT_SHIFT 0
#define PLATFORM_PHYSICAL_TO_VIRTUAL(x) ((void *)(uintptr_t)(x))
struct xgpu_texture_description {
 unsigned long width, height, depth, levels, format;
 BOOL cube_map;
};
static int android_texture_fixes=1;
static unsigned long serial=1, generation=1, uploads, next_texture;
static unsigned long memory_watch_serial(void) { return serial; }
static unsigned long memory_watch_generation(unsigned long a,unsigned long s) { return generation; }
static void memory_watch_protect(unsigned long a,unsigned long s) {}
static BOOL config_boolean(const char *s) { return FALSE; }
static unsigned long log_count;
static void platform_log(const char *s,...) { log_count++; }
static void xgpu_texture_describe(DWORD f,DWORD s,struct xgpu_texture_description *d) {
 memset(d,0,sizeof(*d));d->width=d->height=16;d->depth=d->levels=1;
}
static BOOL texture_size_supported(const struct xgpu_texture_description *d) { return TRUE; }
static unsigned long xgpu_texture_face_size(const struct xgpu_texture_description *d) { return 1024; }
static unsigned long xgpu_texture_level_offset(const struct xgpu_texture_description *d,int l) { return 1024; }
static void glGenTextures(int n,GLuint *t) { *t=++next_texture; }
static void glDeleteTextures(int n,GLuint *t) {}
static void xgpu_gl_state_invalidate(void) {}
static long hud_hires_override_find(unsigned long a,unsigned long w,unsigned long h,unsigned long b) { return -1; }
static GLuint hud_hires_override_texture(long i,unsigned long *l) { return 0; }
static BOOL platform_is_contiguous(void *p) { return TRUE; }
static unsigned char custom_edition_texels_order(unsigned long a) { return 0; }
static void xgpu_statistics_texture_upload(unsigned long s) {}
static void upload(GLuint t,GLenum g,const struct xgpu_texture_description *d,const unsigned char *b,const D3DCOLOR *p,unsigned char c) { uploads++; }
'''

def storage_program():
    gpu = (ROOT/'port/linux/src/xbox_textures.c').read_text()
    code = '#define HALO_ANDROID 1\n' + GPU_FIXTURE
    code = code.replace('static BOOL platform_is_contiguous(void *p) { return TRUE; }',
                        'static BOOL platform_is_contiguous(void *p) { return FALSE; }')
    code += r'''
struct bitmap_data { void *base_address; };
static unsigned long reads, unbinds, channel_forgets;
static int fail_allocation;
static void *storage_calloc(size_t n,size_t s) {
 if(fail_allocation && --fail_allocation==0)return NULL;
 return calloc(n,s);
}
static void halo_custom_edition_texels_channels(const void *p,unsigned char order) { channel_forgets++; }
void android_ce_texture_unbind(const void *r) { unbinds++; }
'''
    code += (ROOT/'port/linux/game/android_ce_texture_cache.h').read_text()
    storage = (ROOT/'port/linux/game/android_ce_texture_cache.c').read_text()
    # cseries redirects free, but not calloc. Exercise the production source's
    # allocator pairing, including unload, instead of hiding it in this fixture.
    code += '#define free(p) abort()\n#define calloc storage_calloc\n'
    code += re.sub(r'^#include .*$', '', storage, flags=re.M)
    code += '#undef calloc\n'
    code += gpu[gpu.index('struct texture_entry\n'):gpu.index('/* (every bit of the three mixed')]
    for name in ('bucket_index', 'palette_hash', 'android_ce_texture_gpu_forget', 'texture_entry_pixels_valid'):
        code += function(gpu, name) + '\n'
    code += r'''
static GLuint texture_entry_result(struct texture_entry *e,GLenum *t,struct xgpu_texture_description *d) {
 *t=e->target;*d=e->description;return e->texture;
}
static void read_pixels(struct bitmap_data *b,void *h) {
 unsigned long *header=h;reads++;header[3]=0;header[4]=0;
 ((unsigned char *)b->base_address)[0]=42;
}
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while(0)
'''
    return code + gpu_lookup(gpu)


def gpu_lookup(source):
    # Exercise the common texture lookup; other owners dispatch before it.
    lookup = function(source, 'xgpu_texture_get')
    return lookup[:lookup.index('{') + 1] + '\n' + lookup[lookup.index('\tDWORD data ='):]



@pytest.mark.parametrize('scenario', ['working_set', 'eviction', 'capacity', 'reload', 'bounds', 'palette', 'oom', 'resident'])
def test_stable_ce_storage_and_real_gpu_lookup(tmp_path, scenario):
    cases = {
        'working_set': r'''
/* 80 MiB of same-frame demand exceeds the old fixed 36 MiB arena.
 * Repeated frames must not reload pixels, relocate them, or re-upload them. */
for(i=0;i<80;i++) {
 headers[i]=android_ce_texture_get(&bitmaps[i],1024*1024,1,read_pixels);CHECK(headers[i]);
 CHECK(xgpu_texture_get(headers[i],NULL,&target,&description)!=0);
}
CHECK(reads==80 && uploads==80 && cache_bytes==80UL*1024*1024);
for(frame=0;frame<120;frame++) {
 android_ce_texture_idle();serial++;generation++;
 for(i=0;i<80;i++) {
  CHECK(android_ce_texture_get(&bitmaps[i],1024*1024,1,read_pixels)==headers[i]);
  xgpu_texture_get(headers[i],NULL,&target,&description);
 }
}
CHECK(reads==80 && uploads==80 && !cache_evictions && !cache_failed && !log_count);
android_ce_texture_reset();CHECK(cache_bytes==0 && cache_count==0 && unbinds==80 && log_count==1);
for(i=0;i<80;i++)CHECK(bitmaps[i].base_address==NULL);
for(i=0;i<TEXTURE_BUCKET_COUNT;i++)CHECK(texture_buckets[i]==NULL);''',
        'eviction': r'''
headers[0]=android_ce_texture_get(&bitmaps[0],80UL*1024*1024,1,read_pixels);CHECK(headers[0]);
xgpu_texture_get(headers[0],NULL,&target,&description);
android_ce_texture_idle();
headers[1]=android_ce_texture_get(&bitmaps[1],60UL*1024*1024,1,read_pixels);CHECK(headers[1]);
CHECK(!bitmaps[0].base_address && cache_evictions==1 && unbinds==1 && cache_bytes==60UL*1024*1024);
CHECK(android_ce_texture_get(&bitmaps[0],1024,0,read_pixels)==NULL);
xgpu_texture_get(headers[1],NULL,&target,&description);CHECK(uploads==2);
android_ce_texture_reset();''',
        'capacity': r'''
CHECK(android_ce_texture_get(&bitmaps[0],80UL*1024*1024,1,read_pixels));
CHECK(!android_ce_texture_get(&bitmaps[1],60UL*1024*1024,1,read_pixels));
CHECK(cache_failed==1 && cache_count==1 && !unbinds);
CHECK(!android_ce_texture_get(&bitmaps[1],~0UL,1,read_pixels));
CHECK(!android_ce_texture_get(&bitmaps[1],0,1,read_pixels));
CHECK(log_count==1);
android_ce_texture_reset();''',
        'reload': r'''
for(i=0;i<20;i++) {
 headers[0]=android_ce_texture_get(&bitmaps[0],1024,1,read_pixels);CHECK(headers[0]);
 xgpu_texture_get(headers[0],NULL,&target,&description);CHECK(uploads==(unsigned long)i+1);
 android_ce_texture_delete(&bitmaps[0]);
 CHECK(!bitmaps[0].base_address && cache_bytes==0);
 CHECK(!android_ce_texture_pixels(headers[0],&bytes) && bytes==0);
}
CHECK(unbinds==20 && channel_forgets==20);''',
        'bounds': r'''
headers[0]=android_ce_texture_get(&bitmaps[0],512,1,read_pixels);CHECK(headers[0]);
xgpu_texture_get(headers[0],NULL,&target,&description);CHECK(uploads==0);
/* A matching Data word without the registered header grants no heap access. */
memcpy(fake,headers[0],sizeof(fake));
xgpu_texture_get(fake,NULL,&target,&description);CHECK(uploads==0);
android_ce_texture_reset();''',
        'palette': r'''
headers[0]=android_ce_texture_get(&bitmaps[0],1024,1,read_pixels);CHECK(headers[0]);
((DWORD *)headers[0])[3]=11;
xgpu_texture_get(headers[0],palette,&target,&description);CHECK(uploads==1);
palette[5]=42;xgpu_texture_get(headers[0],palette,&target,&description);CHECK(uploads==2);
palette[5]=0;xgpu_texture_get(headers[0],palette,&target,&description);CHECK(uploads==2);
android_ce_texture_reset();
for(i=0;i<TEXTURE_BUCKET_COUNT;i++)CHECK(texture_buckets[i]==NULL);''',
        'oom': r'''
for(i=1;i<=2;i++) {
 fail_allocation=i;
 CHECK(!android_ce_texture_get(&bitmaps[0],1024,1,read_pixels));
 CHECK(!bitmaps[0].base_address && !cache_bytes && !cache_count && !reads);
}
fail_allocation=0;
CHECK(android_ce_texture_get(&bitmaps[0],1024,1,read_pixels));
android_ce_texture_reset();''',
        'resident': r'''
CHECK(!android_ce_texture_resident(&bitmaps[0]));
headers[0]=android_ce_texture_get(&bitmaps[0],80UL*1024*1024,1,read_pixels);
CHECK(headers[0] && android_ce_texture_resident(&bitmaps[0]));
android_ce_texture_idle();
/* HUD classification must not pin every registered bitmap for this frame. */
CHECK(android_ce_texture_resident(&bitmaps[0]));
CHECK(android_ce_texture_get(&bitmaps[1],60UL*1024*1024,1,read_pixels));
CHECK(!android_ce_texture_resident(&bitmaps[0]));
cache_frame=~0UL;android_ce_texture_idle();CHECK(cache_frame==1);
CHECK(android_ce_texture_get(&bitmaps[0],80UL*1024*1024,1,read_pixels));
android_ce_texture_reset();''',
    }
    code = storage_program() + r'''
int main(void) {
 struct bitmap_data bitmaps[80]={{0}};void *headers[80];
 DWORD fake[5]={0};D3DCOLOR palette[256]={0};unsigned long bytes;
 GLenum target;struct xgpu_texture_description description;int i,frame;
''' + cases[scenario] + '\nreturn 0;}\n'
    run_c(tmp_path, code)


def test_android_ce_scope_and_cleanup():
    source = (ROOT/'source/cache/xbox_texture_cache.c').read_text()
    get = function(source, '_texture_cache_bitmap_get_hardware_format')
    assert 'custom_edition_cache_tags_loaded()' in get
    assert 'TEST_FLAG(bitmap->flags, _bitmap_cached_bit)' in get
    assert 'texture_cache_bitmap_valid(bitmap)' in get
    stripped = re.sub(r'#ifdef HALO_ANDROID\n.*?#endif', '', source, flags=re.S)
    assert 'android_ce_texture_' not in stripped
    assert 'android_ce_texture_delete(bitmap)' in function(source, 'texture_cache_bitmap_delete')
    assert 'android_ce_texture_reset()' in function(source, 'texture_cache_flush')


@pytest.mark.parametrize('android', [False, True])
def test_hud_residency_uses_the_correct_owner(tmp_path, android):
    source = (ROOT/'port/linux/game/hud_hires_tags.c').read_text()
    code = ('#define HALO_ANDROID 1\n' if android else '') + r'''
#define TRUE 1
#define NONE -1
typedef int boolean;
struct bitmap_data { long cache_block_index; };
static int resident;
static int android_ce_texture_resident(const struct bitmap_data *b) { return resident; }
''' + function(source, 'hud_hires_bitmap_resident') + r'''
int main(void) {
 struct bitmap_data b={NONE};
 if(hud_hires_bitmap_resident(&b))return 1;
 b.cache_block_index=3;if(!hud_hires_bitmap_resident(&b))return 2;
 b.cache_block_index=NONE;resident=1;
'''
    code += f'if(hud_hires_bitmap_resident(&b)!={int(android)})return 3;\nreturn 0;}}\n'
    run_c(tmp_path, code)


@pytest.mark.parametrize('platform', ['android', 'android-normal', 'desktop', 'web'])
def test_normal_gpu_cache_keeps_write_tracking(tmp_path, platform):
    gpu = (ROOT/'port/linux/src/xbox_textures.c').read_text()
    code = ('#define HALO_ANDROID 1\n' if platform.startswith('android') else
            '#define HALO_WEB 1\n' if platform == 'web' else '') + GPU_FIXTURE
    code += 'static int ce_queries;static const void *android_ce_texture_pixels(const void *r,unsigned long *b) { ce_queries++;*b=0;return NULL; }\n'
    code += gpu[gpu.index('struct texture_entry\n'):gpu.index('/* (every bit of the three mixed')]
    for name in ('bucket_index', 'palette_hash', 'texture_entry_pixels_valid'):
        code += function(gpu,name) + '\n'
    code += r'''
static GLuint texture_entry_result(struct texture_entry *e,GLenum *t,struct xgpu_texture_description *d) {
 *t=e->target;*d=e->description;return e->texture;
}
'''
    code += gpu_lookup(gpu) + r'''
int main(void) {
 DWORD header[5]={0};GLenum target;struct xgpu_texture_description description;
 xgpu_texture_get(header,NULL,&target,&description);
 if(uploads!=1)return 1;
 xgpu_texture_get(header,NULL,&target,&description);
 if(uploads!=1)return 2;
 serial++;generation++;
 xgpu_texture_get(header,NULL,&target,&description);
 if(uploads!=2)return 3;
 return 0;
}
'''
    if platform == 'android-normal':
        code = code.replace('static int android_texture_fixes=1;', 'static int android_texture_fixes=0;')
    code = code.replace('if(uploads!=2)return 3;',
                        'if(uploads!=2)return 3; if(ce_queries!=' + str(3 if platform == 'android' else 0) + ')return 4;')
    run_c(tmp_path,code)


def test_draw_refresh_removed():
    draw = function((ROOT/'port/linux/src/d3d8_gl.c').read_text(), 'prepare_draw')
    assert 'xgpu_gl_state_invalidate(' not in draw


def test_unbind_clears_only_owned_headers(tmp_path):
    source = (ROOT/'port/linux/src/d3d8_gl.c').read_text()
    code = r'''
#include <stddef.h>
#define D3DTSS_MAXSTAGES 4
static struct { void *textures[4]; } device;
static int refreshes;
static void xgpu_gl_state_invalidate(void) {refreshes++;}
''' + function(source,'android_ce_texture_unbind') + r'''
int main(void) {
 int own,other;device.textures[0]=&own;device.textures[1]=&other;device.textures[3]=&own;
 android_ce_texture_unbind(&own);
 return device.textures[0]!=NULL || device.textures[1]!=&other || device.textures[3]!=NULL || refreshes!=1;
}
'''
    run_c(tmp_path,code)
