"""Exercise the real MCC progress controller with a recording device backend."""
from pathlib import Path
import re
import subprocess

import pytest
from test_mcc_pause_runtime import _compile

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def loading_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-loading")
    implementation = (ROOT / "port/linux/game/mcc_loading.c").read_text()
    implementation = re.sub(r'^#include [<"][^\n]+[>"]\s*$', '', implementation, flags=re.M)
    source = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mcc_loading.h"
typedef float real;
typedef uint32_t DWORD;
typedef int IDirect3DSurface8;
typedef struct {DWORD X,Y,Width,Height;float MinZ,MaxZ;} D3DVIEWPORT8;
typedef struct {DWORD Width,Height;} D3DSURFACE_DESC;
#define FALSE 0
#define _error_silent 0
static void error(int level,char const *format,...) {(void)level;(void)format;}
static int device, surface, null_renderer, native_active, starts, ends, frames, releases;
static int failure, reenter;
static DWORD clock_ms;
static real drawn;
static void *global_d3d_device=&device;
static DWORD GetTickCount(void) {return clock_ms;}
static int config_boolean(char const *name) {(void)name;return null_renderer;}
static int mcc_level_name(char const *name) {return name && !strncmp(name,"mcc_maps/",9);}
static int progress_bar_is_active(void) {return native_active;}
static void progress_bar_eachframe(void) { }
static void progress_bar_begin(int skip) {(void)skip;starts++;native_active=1;}
static void progress_bar_end(void) {ends++;native_active=0;}
static void progress_bar_display(real value) {
    assert(value>=0 && value<=1);assert(value>=drawn);drawn=value;
    if(reenter){clock_ms+=100;mcc_loading_pulse();}
}
static int IDirect3DDevice8_GetBackBuffer(void *d,int a,int b,IDirect3DSurface8 **s) {
    (void)d;(void)a;(void)b;if(failure==1)return -1;if(failure==2)return 0;*s=&surface;return 0;
}
static int IDirect3DSurface8_GetDesc(IDirect3DSurface8 *s,D3DSURFACE_DESC *desc) {
    assert(s==&surface);desc->Width=1280;desc->Height=720;return failure==3 ? -1 : 0;
}
static void IDirect3DSurface8_Release(IDirect3DSurface8 *s) {assert(s==&surface);releases++;}
static void IDirect3DDevice8_SetViewport(void *d,D3DVIEWPORT8 *v) {
    (void)d;assert(v->Width==1280 && v->Height==720 && v->MaxZ==1);
}
static void IDirect3DDevice8_BeginScene(void *d) {(void)d;}
static void IDirect3DDevice8_EndScene(void *d) {(void)d;}
static void IDirect3DDevice8_Present(void *d,void *a,void *b,void *c,void *e) {
    (void)d;(void)a;(void)b;(void)c;(void)e;frames++;
}
''' + implementation + r'''
int main(int argc,char **argv) {
    int test=argc>1 ? atoi(argv[1]) : -1, before;
    if(test<4 || test==6 || test==7) {
        if(test==1)null_renderer=1;
        if(test==2)global_d3d_device=NULL;
        if(test==3)native_active=1;
        mcc_loading_begin(test==0 ? "maps/a10" : test==6 ? "custom_maps/a10" : test==7 ? NULL : "mcc_maps/a10");
        mcc_loading_pulse();mcc_loading_stage(MCC_LOADING_AUDIO);mcc_loading_end(1);
        assert(!starts && !frames && !ends);assert(native_active==(test==3));return 0;
    }
    if(test==4)clock_ms=UINT32_MAX-20;
    if(test>=8 && test<=10)failure=test-7;
    if(test==11)reenter=1;
    assert(mcc_loading_begin("mcc_maps/a10"));assert(starts==1);
    assert(!mcc_loading_begin("mcc_maps/b30"));assert(starts==1);
    if(failure) {
        assert(!frames);assert(releases==(failure==3));
        failure=0;clock_ms+=50;mcc_loading_pulse();assert(frames==1);
    }
    before=frames;mcc_loading_pulse();assert(frames==before);
    clock_ms+=50;mcc_loading_stage(MCC_LOADING_AUDIO);assert(frames==before+1);
    clock_ms+=50;mcc_loading_update(MCC_LOADING_AUDIO,5,10);assert(drawn>0.3f && drawn<0.55f);
    before=frames;clock_ms+=50;
    mcc_loading_update(MCC_LOADING_AUDIO,11,10);
    mcc_loading_update(MCC_LOADING_AUDIO,0,0);
    mcc_loading_update((enum mcc_loading_stage)-1,1,1);
    mcc_loading_update(MCC_LOADING_STAGES,1,1);
    mcc_loading_stage((enum mcc_loading_stage)-1);mcc_loading_stage(MCC_LOADING_STAGES);
    mcc_loading_stage(MCC_LOADING_TAGS);assert(frames==before);
    mcc_loading_update(MCC_LOADING_AUDIO,1,10);assert(drawn>0.3f);
    mcc_loading_end(test!=5);assert(ends==1 && !native_active && !mcc_loading.active);
    assert(test==5 ? drawn<1 : drawn==1);
    before=frames;clock_ms+=100;mcc_loading_pulse();mcc_loading_end(1);assert(frames==before && ends==1);
    drawn=0;mcc_loading_begin("mcc_maps/b30");assert(starts==2);mcc_loading_end(0);assert(ends==2);
    return 0;
}
'''
    path = work / "loading.c"
    path.write_text(source)
    output = work / "loading.exe"
    _compile(path, output)
    return output


@pytest.mark.parametrize("case", range(12))
def test_loading_lifetime_progress_and_device_failures(loading_tool, case):
    subprocess.run([str(loading_tool), str(case)], check=True, capture_output=True)


def test_loading_wraps_scenario_load_without_normal_frame_callbacks():
    from harness import function
    load = function((ROOT / "source/game/game.c").read_text(), "game_load")
    assert load.index("mcc_loading_begin") < load.index("scenario_load(") < load.index("mcc_loading_end")
    controller = (ROOT / "port/linux/game/mcc_loading.c").read_text()
    for unsafe in ["main_loop(", "main_pregame_render(", "render_frame_pregame(", "network_game_update("]:
        assert unsafe not in controller


def test_menu_probe_never_opens_a_loading_screen(tmp_path):
    from harness import function
    implementation = (ROOT / "port/linux/game/mcc_resources.c").read_text()
    source = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mcc_cache_format.h"
#include "mcc_errors.c"
struct mcc_runtime {char level_name[64];struct mcc_cache_source source;
    struct {int external_bitmap_count,external_sound_count;} report;char failure[320];};
static int owns,starts,ends,last_success,fail;
static int mcc_package_name(char const *name,char *out) {(void)name;out[0]=0;return 1;}
enum mcc_cache_status mcc_cache_identify(void const *h,size_t b,uint64_t s,struct mcc_cache_identity *i) {
    (void)h;(void)b;(void)s;i->tag_data_size=16;return fail==5 ? MCC_CACHE_WRONG_VERSION : MCC_CACHE_OK;
}
static int read_source(void *c,uint32_t o,uint32_t b,void *out) {(void)c;(void)o;memset(out,0,b);return fail!=4;}
static int mcc_loading_begin(char const *name) {(void)name;starts++;return owns;}
static void mcc_loading_end(int success) {assert(owns);ends++;last_success=success;}
static enum mcc_cache_status load_stub(struct mcc_cache_source const *s,void *r,uint32_t n,void *report) {
    (void)s;(void)r;(void)n;(void)report;return fail==6 ? MCC_CACHE_BAD_SOUND : MCC_CACHE_OK;
}
#define mcc_cache_load load_stub
static int mcc_resources_select(struct mcc_runtime *r,void *raw) {(void)r;(void)raw;return fail!=1;}
static int mcc_resources_fingerprint(struct mcc_runtime *r,unsigned char *d) {(void)r;(void)d;return fail!=2;}
static int mcc_resources_checksum(struct mcc_runtime *r,unsigned bits,uint32_t *sum) {(void)r;(void)bits;*sum=1;return fail!=3;}
static void mcc_resources_dispose(struct mcc_runtime *r) {(void)r;}
''' + function(implementation, "mcc_resources_inspect") + r'''
int main(void) {
    struct mcc_cache_source source;unsigned char digest[32];uint32_t sum;int expected,started;char reason[320];
    source.context=NULL;source.read=read_source;source.size=2048;
    for(owns=0;owns<=1;owns++)for(fail=0;fail<7;fail++) {
        starts=ends=0;last_success=-1;expected=fail==0;
        started=fail!=4 && fail!=5;strcpy(reason,"stale failure");
        assert(mcc_resources_inspect("mcc_maps/a10",&source,&sum,digest,reason,sizeof(reason))==expected);
        (void)started;(void)mcc_loading_begin;(void)mcc_loading_end;
        assert(starts==0 && ends==0 && last_success==-1);
        assert((reason[0]==0)==expected);
    }
    return 0;
}
'''
    path = tmp_path / "probe.c"
    path.write_text(source)
    output = tmp_path / "probe.exe"
    _compile(path, output)
    subprocess.run([str(output)], check=True, capture_output=True)
