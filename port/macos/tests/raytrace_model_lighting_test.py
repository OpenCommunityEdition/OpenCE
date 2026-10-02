#!/usr/bin/env python3
"""Exercise the production per-draw model GI helper and renderer call site.

The actual helper, packed guest lighting structures and opaque model draw
block are compiled with mocked GPU constant uploads. Metal probe transport
and kernel radiometry are covered by the separate Metal regression fixture.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
platform = (ROOT / "port/linux/src/raytrace_gl.c").read_text()
renderer = (ROOT / "source/rasterizer/xbox/rasterizer_xbox_models.c").read_text()
render_header = (ROOT / "source/render/render.h").read_text()


def balanced(text, start):
    opening = text.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def function(text, name):
    match = re.search(r"(?:^|\n)(?:static )?(?:int |void |const char \*)" + name + r"\([^;]*?\)\s*\{", text)
    if not match:
        raise RuntimeError("Missing production function " + name)
    return balanced(text, match.start())


def structure(name):
    match = re.search(r"struct " + name + r"\n\{[^}]*\};", render_header)
    if not match:
        raise RuntimeError("Missing renderer structure " + name)
    return re.sub(r"\blong\b", "int32_t", match.group())


def guest_words(text):
    # The game runs ILP32 even on the 64-bit Mac host. In particular the
    # frame counter and probe ages must wrap at 32 bits in this fixture.
    return re.sub(r"\bunsigned long\b", "uint32_t", text)


helper = function(platform, "halo_ray_traced_model_lighting")
cache_helpers = "\n".join(function(platform, name) for name in ("probe_cache_reset", "probe_result_valid", "probe_cache_merge"))
ray_registry = re.search(r"static struct\s*\{\s*int initialized;[^}]*\} ray;", platform)
ray_modes = re.search(r"enum\s*\{\s*_ray_tracing_off,[^}]*\};", platform)
settings_views = re.search(r"static const int settings_views\[\] = [^;]*;", platform)
trace_scale_define = re.search(r"^#define TRACE_SCALE [^\n]+", platform, re.MULTILINE)
if not ray_registry or not ray_modes or not settings_views or not trace_scale_define:
    raise RuntimeError("Missing ray tracing settings state")
probe_registry = re.search(r"static struct\s*\{\s*float requests\[[^}]*\} probes;", platform)
if not probe_registry:
    raise RuntimeError("Missing probe registry")
call_start = renderer.index("\t\t\t{\n\t\t\t\tstruct render_lighting lighting = parameters->lighting;")
draw_block = balanced(renderer, call_start)
if "halo_ray_traced_model_lighting" in (ROOT / "source/objects/object_lights.c").read_text():
    raise RuntimeError("Traced GI must not modify cached object lighting")
if "halo_ray_traced_object_lighting(" in (ROOT / "source/objects/object_lights.c").read_text():
    raise RuntimeError("Old cached object-lighting override remains installed")

preamble = r'''
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "halo_linux_source_fixups.h"
#include "raytrace_gl.h"
typedef float real;
typedef uint16_t word;
typedef struct { real red,green,blue; } real_rgb_color;
typedef struct { real alpha,red,green,blue; } real_argb_color;
typedef struct { real i,j,k; } real_vector3d;
typedef struct { real x,y,z; } real_point3d;
typedef uint32_t GLuint;
typedef int GLint;
#define MAXIMUM_RENDERED_DISTANT_LIGHTS 2
#define MAXIMUM_RENDERED_POINT_LIGHTS 2
#define TEST_FLAG(flags,bit) (((flags)&(1u<<(bit)))!=0)
#define _rasterizer_geometry_first_person_bit 7
'''

settings_stubs = r'''
static void initialize(void) { assert(!"Unexpected GPU initialization in CPU fixture"); }
static void hardware_link(void) { ray.hardware=ray.hardware_linked; }
static void platform_log(const char *format,...) { (void)format; }
'''

boundary = r'''
_Static_assert(sizeof(struct render_distant_light)==6*sizeof(float),"distant ABI");
_Static_assert(offsetof(struct render_distant_light,direction)==3*sizeof(float),"direction ABI");
_Static_assert(offsetof(struct render_lighting,distant_lights)==16,"distant array ABI");
_Static_assert(offsetof(struct render_lighting,point_light_count)==64,"point lights ABI");
_Static_assert(offsetof(struct render_lighting,reflection_tint_color)==0x4c,"reflection ABI");
_Static_assert(sizeof(struct render_lighting)==0x74,"guest lighting size");
static int local_sky_flag;
struct rasterizer_model_begin_parameters {
    uint32_t geometry_flags;struct render_lighting lighting;real_point3d centroid;
};
static struct render_lighting uploaded;
static void rasterizer_set_model_lighting(const struct render_lighting *lighting) { uploaded=*lighting; }
static void draw(const struct rasterizer_model_begin_parameters *parameters) {
'''

checks = r'''
}
static void near(float actual,float expected) { assert(isfinite(actual)&&fabsf(actual-expected)<1e-6f); }
static void active(void) {
    memset(&ray,0,sizeof(ray));ray.initialized=ray.hardware_linked=ray.enabled=ray.hardware=ray.drawn_level=ray.objects=1;ray.gi=1;
    memset(&probes,0,sizeof(probes));local_sky_flag=0;
}
static struct rasterizer_model_begin_parameters model(void) {
    struct rasterizer_model_begin_parameters m;memset(&m,0,sizeof(m));
    m.lighting.ambient_color=(real_rgb_color){.12f,.23f,.34f};
    m.lighting.distant_light_count=0; /* A valid custom model need not have distant lights. */
    m.lighting.distant_lights[0].color=(real_rgb_color){.45f,.56f,.67f};
    m.lighting.distant_lights[0].direction=(real_vector3d){.25f,.5f,.75f};
    m.lighting.distant_lights[1].color=(real_rgb_color){.78f,.89f,.91f};
    m.lighting.distant_lights[1].direction=(real_vector3d){-.25f,-.5f,-.75f};
    m.lighting.point_light_count=2;m.lighting.point_light_indices[0]=0x12345678;m.lighting.point_light_indices[1]=-1;
    m.lighting.reflection_tint_color=(real_argb_color){.11f,.22f,.33f,.44f};
    m.lighting.shadow_vector=(real_vector3d){1,2,3};m.lighting.shadow_color=(real_rgb_color){.1f,.2f,.3f};
    m.lighting.pad=0x1234;m.lighting.pad1=0xabcd;return m;
}
static void probe(int index,float x,float y,float z,float r,float g,float b,float accuracy,float dx,float dy,float dz) {
    const float values[10]={x,y,z,r,g,b,accuracy,dx,dy,dz};memcpy(probes.results+index*10,values,sizeof(values));
    if(probes.result_count<=index)probes.result_count=index+1;
    probes.result_frames[index]=(uint32_t)ray.frame;
}
static void expect_untouched(const struct render_lighting *original) {
    /* Everything after the distant-light array is preserved, including point
       lights, reflection tint, shadow metadata and padding. */
    size_t start=offsetof(struct render_lighting,point_light_count);
    assert(!memcmp((const char*)&uploaded+start,(const char*)original+start,sizeof(uploaded)-start));
}
int main(void) {
    struct rasterizer_model_begin_parameters cached=model(),snapshot=cached;
    active();draw(&cached);assert(uploaded.distant_light_count==2&&probes.request_count==1);
    assert(!memcmp(&uploaded.ambient_color,&cached.lighting.ambient_color,sizeof(real_rgb_color)));
    for(int i=0;i<12;i++)near(((float*)uploaded.distant_lights)[i],0);
    expect_untouched(&cached.lighting);assert(!memcmp(&cached,&snapshot,sizeof(cached)));
    probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,1);draw(&cached);
    near(uploaded.ambient_color.red,.6f);near(uploaded.ambient_color.green,.3f);near(uploaded.ambient_color.blue,.15f);
    near(uploaded.distant_lights[0].color.red,.2f);near(uploaded.distant_lights[0].color.green,.1f);near(uploaded.distant_lights[0].color.blue,.05f);
    near(uploaded.distant_lights[0].direction.i,0);near(uploaded.distant_lights[0].direction.j,0);near(uploaded.distant_lights[0].direction.k,-1);
    for(int i=6;i<12;i++)near(((float*)uploaded.distant_lights)[i],0);
    expect_untouched(&cached.lighting);assert(!memcmp(&cached,&snapshot,sizeof(cached)));
    ray.enabled=0;draw(&cached);assert(!memcmp(&uploaded,&cached.lighting,sizeof(uploaded))); /* F9 restores this draw. */

    /* Every disabled/incompatible mode must preserve the game's draw data. */
    for(int mode=0;mode<9;mode++) {
        active();probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,1);
        switch(mode) {
        case 0:ray.failed=1;break;case 1:ray.hardware=0;break;case 2:ray.drawn_level=0;break;
        case 3:ray.objects=0;break;case 4:ray.shapes=1;break;case 5:ray.gi=0;break;
        case 6:ray.gi=2;break;case 7:local_sky_flag=1;break;case 8:cached.geometry_flags=1u<<7;break;
        }
        draw(&cached);assert(!memcmp(&uploaded,&cached.lighting,sizeof(uploaded)));assert(probes.request_count==0);
        cached.geometry_flags=0;
    }
    active();ray.gi=3;probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,1);draw(&cached);near(uploaded.ambient_color.red,.6f);

    active();cached.centroid=(real_point3d){10,20,30};draw(&cached);
    assert(probes.request_count==1);near(probes.requests[0],10);near(probes.requests[1],20);near(probes.requests[2],30);
    draw(&cached);assert(probes.request_count==1);cached.centroid.x+=.125f;draw(&cached);assert(probes.request_count==1);
    cached.centroid.x=10.25f;draw(&cached);assert(probes.request_count==2); /* Strict dedup radius boundary. */
    for(int i=0;i<80;i++) {cached.centroid.x=100.f+i;draw(&cached);}assert(probes.request_count==8);
    assert(!memcmp(&uploaded.ambient_color,&cached.lighting.ambient_color,sizeof(real_rgb_color)));
    cached=model();active();probe(0,.5f,0,0,.4f,.4f,.4f,0,0,0,1);probe(1,.125f,0,0,.8f,.4f,.2f,.25f,0,0,1);
    draw(&cached);near(uploaded.ambient_color.red,.6f); /* Nearest available result wins. */
    active();probe(0,.75f,0,0,.8f,.4f,.2f,.25f,0,0,1);draw(&cached);
    assert(!memcmp(&uploaded.ambient_color,&cached.lighting.ambient_color,sizeof(real_rgb_color)));
    active();probe(0,0,0,0,-1,2,.5f,2,0,0,1);draw(&cached);
    near(uploaded.ambient_color.red,0);near(uploaded.distant_lights[0].color.red,0);near(uploaded.distant_lights[0].color.green,1);
    for(int component=0;component<10;component++) {
        active();probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,1);probes.results[component]=NAN;draw(&cached);
        assert(!memcmp(&uploaded.ambient_color,&cached.lighting.ambient_color,sizeof(real_rgb_color)));
        for(int i=0;i<12;i++)near(((float*)uploaded.distant_lights)[i],0);
    }
    active();probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,0);draw(&cached);
    assert(!memcmp(&uploaded.ambient_color,&cached.lighting.ambient_color,sizeof(real_rgb_color)));
    active();probe(0,0,0,0,NAN,.4f,.2f,.25f,0,0,1);probe(1,.125f,0,0,.8f,.4f,.2f,.25f,0,0,1);
    draw(&cached);near(uploaded.ambient_color.red,.6f); /* Invalid nearest result cannot hide a valid neighbour. */
    active();probe(0,0,0,0,.8f,.4f,.2f,0,0,0,0);draw(&cached);near(uploaded.ambient_color.red,.8f);
    active();draw(&cached);probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,1);draw(&cached);assert(probes.request_count&&probes.result_count);
    halo_ray_tracing_toggle();assert(!ray.enabled&&!probes.request_count&&!probes.result_count);
    draw(&cached);assert(!memcmp(&uploaded,&cached.lighting,sizeof(uploaded)));
    halo_ray_tracing_toggle();assert(ray.enabled&&!probes.request_count&&!probes.result_count);
    draw(&cached);assert(!memcmp(&uploaded.ambient_color,&cached.lighting.ambient_color,sizeof(real_rgb_color)));
    active();struct halo_ray_tracing_settings settings={.tracing=1,.view=0,.gi=1,.trace_scale=4,.objects=1};
    halo_ray_tracing_set(&settings);draw(&cached);probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,1);draw(&cached);
    halo_ray_tracing_set(&settings);assert(probes.request_count&&probes.result_count); /* Stable settings keep probes. */
    settings.gi=3;halo_ray_tracing_set(&settings);assert(!probes.request_count&&!probes.result_count);
    probe(0,0,0,0,.8f,.4f,.2f,.25f,0,0,1);draw(&cached);settings.gi_bounce=.5f;
    halo_ray_tracing_set(&settings);assert(!probes.request_count&&!probes.result_count);
    active();cached.centroid.x=NAN;draw(&cached);assert(!memcmp(&uploaded,&cached.lighting,sizeof(uploaded))&&probes.request_count==0);
    float ambient[3]={1,2,3},distant[12]={0},position[3]={0};
    assert(!halo_ray_traced_model_lighting(NULL,ambient,distant));assert(!halo_ray_traced_model_lighting(position,NULL,distant));assert(!halo_ray_traced_model_lighting(position,ambient,NULL));
    /* A partial readback refreshes matching points without erasing all other
       visible models. Zero/no new readback does not make old probes younger. */
    float batch[65*10]={0};
    for(int i=0;i<65;i++) {
        const float value[10]={i*2.f,0,0,.8f,.4f,.2f,.25f,0,0,1};memcpy(batch+i*10,value,sizeof(value));
    }
    cached=model();active();ray.frame=10;probe_cache_merge(batch,2);assert(probes.result_count==2);
    draw(&cached);assert(probes.request_count==0);near(uploaded.ambient_color.red,.6f);
    ray.frame=39;draw(&cached);assert(probes.request_count==0);
    probe_cache_merge(NULL,2);probe_cache_merge(batch,0);probe_cache_merge(batch,-1);
    assert(probes.result_count==2&&probes.result_frames[0]==10&&probes.result_frames[1]==10);
    ray.frame=40;draw(&cached);assert(probes.request_count==1); /* Refresh starts at age30, not every draw. */
    probes.request_count=0;ray.frame=41;batch[3]=.4f;probe_cache_merge(batch,1);
    assert(probes.result_count==2&&probes.result_frames[0]==41&&probes.result_frames[1]==10);
    draw(&cached);assert(probes.request_count==0);near(uploaded.ambient_color.red,.3f);
    cached.centroid.x=2;draw(&cached);assert(probes.request_count==1);near(uploaded.ambient_color.red,.6f);
    probes.request_count=0;cached.centroid.x=.125f;draw(&cached);assert(probes.request_count==0);
    cached.centroid.x=.25f;draw(&cached);assert(probes.request_count==1);near(uploaded.ambient_color.red,.3f);
    /* Movement schedules new work while retaining nearby GI until it returns. */
    probes.request_count=0;batch[0]=.125f;ray.frame=42;probe_cache_merge(batch,1);
    assert(probes.result_count==2);near(probes.results[0],.125f);assert(probes.result_frames[0]==42);
    batch[0]=.375f;probe_cache_merge(batch,1);assert(probes.result_count==3); /* Strict .25 merge boundary. */
    for(int component=0;component<10;component++) {
        float invalid[10];memcpy(invalid,batch,sizeof(invalid));invalid[component]=NAN;
        probe_cache_merge(invalid,1);assert(probes.result_count==3);
    }
    float zero_direction[10]={100,0,0,.8f,.4f,.2f,.25f,0,0,0};probe_cache_merge(zero_direction,1);assert(probes.result_count==3);
    zero_direction[6]=0;probe_cache_merge(zero_direction,1);assert(probes.result_count==4);

    /* Host ABI caps each readback to64 records; cache retains256 samples and
       replaces the oldest timestamp rather than exceeding either array. */
    active();probe_cache_merge(batch,65);assert(probes.result_count==64);
    active();
    for(int i=0;i<256;i++) {float value[10]={i*2.f,0,0,.8f,.4f,.2f,.25f,0,0,1};ray.frame=i;probe_cache_merge(value,1);}
    assert(probes.result_count==256);ray.frame=300;float extra[10]={10000,0,0,.8f,.4f,.2f,.25f,0,0,1};
    probe_cache_merge(extra,1);assert(probes.result_count==256);near(probes.results[0],10000);assert(probes.result_frames[0]==300);
    extra[0]=10002;probe_cache_merge(extra,1);near(probes.results[10],10002);assert(probes.result_frames[1]==300);

    /* Refresh age is modulo2^32 because this is an ILP32 guest. */
    active();cached=model();ray.frame=UINT32_MAX-10u;float origin[10]={0,0,0,.8f,.4f,.2f,.25f,0,0,1};
    probe_cache_merge(origin,1);ray.frame=5;draw(&cached);assert(probes.request_count==0);
    ray.frame=18;draw(&cached);assert(probes.request_count==0);ray.frame=19;draw(&cached);assert(probes.request_count==1);
    /* Reset clears cache and rejects one already queued nonempty readback. */
    probe_cache_reset();assert(!probes.request_count&&!probes.result_count&&probes.discard_result);
    probe_cache_merge(origin,0);assert(probes.discard_result);probe_cache_merge(origin,1);
    assert(!probes.result_count&&!probes.discard_result);probe_cache_merge(origin,1);assert(probes.result_count==1);
    /* Trace quality bounds cold and refresh work separately. Lower grid
       resolutions retain GI for longer; movement still requests new data. */
    const int scales[]={0,2,4,8},budgets[]={8,8,4,2};
    const uint32_t intervals[]={30,30,60,120};
    for(int quality=0;quality<4;quality++) {
        active();ray.trace_scale=scales[quality];cached=model();
        for(int i=0;i<80;i++) {cached.centroid.x=i*2.f;draw(&cached);}
        assert(probes.request_count==budgets[quality]);
        active();ray.trace_scale=scales[quality];ray.frame=100;cached=model();
        probe_cache_merge(origin,1);draw(&cached);assert(probes.request_count==0);
        ray.frame=100+intervals[quality]-1;draw(&cached);assert(probes.request_count==0);
        ray.frame=100+intervals[quality];draw(&cached);assert(probes.request_count==1);near(uploaded.ambient_color.red,.6f);
        probes.request_count=0;ray.frame=100;cached.centroid.x=.125f;draw(&cached);assert(probes.request_count==0);
        cached.centroid.x=.25f;draw(&cached);assert(probes.request_count==1);near(uploaded.ambient_color.red,.6f);
        active();ray.trace_scale=scales[quality];ray.frame=100;cached=model();
        for(int i=0;i<16;i++)probe(i,i*2.f,0,0,.8f,.4f,.2f,.25f,0,0,1);
        ray.frame=100+intervals[quality];
        for(int i=0;i<16;i++) {cached.centroid.x=i*2.f;draw(&cached);}
        assert(probes.request_count==budgets[quality]&&probes.result_count==16);
        active();ray.trace_scale=scales[quality];ray.frame=UINT32_MAX-50u;cached=model();probe_cache_merge(origin,1);
        ray.frame=(uint32_t)(UINT32_MAX-50u+intervals[quality]-1u);draw(&cached);assert(probes.request_count==0);
        ray.frame=(uint32_t)(UINT32_MAX-50u+intervals[quality]);draw(&cached);assert(probes.request_count==1);
    }
    puts("RT model lighting regression: layout/split/F9/guards, quality budgets8/4/2 and cadence30/60/120, retained256/partial merge, movement refresh, invalid probes, oldest eviction, ILP32 wrap and reset passed");
}
'''

with tempfile.TemporaryDirectory(prefix="halo-rt-model-lighting-test-") as folder:
    folder = Path(folder)
    c_file, binary = folder / "model_lighting_test.c", folder / "model_lighting_test"
    blocks = [preamble, ray_registry.group(), ray_modes.group(), structure("render_distant_light"),
              structure("render_lighting"), probe_registry.group(), cache_helpers, settings_stubs,
              function(platform, "trace_scale_value"), trace_scale_define.group(), settings_views.group(),
              function(platform, "halo_ray_tracing_toggle"), function(platform, "halo_ray_tracing_set"),
              helper, boundary, draw_block, checks]
    c_file.write_text(guest_words("\n".join(blocks)))
    subprocess.run(["clang", "-std=c11", "-O1", "-g", "-DHALO_MACOS", "-fsanitize=undefined,float-divide-by-zero", "-fno-sanitize-recover=all", "-Wall", "-Wextra", "-Werror", "-iquote", str(ROOT / "port/linux/include"), "-iquote", str(ROOT / "port/linux/src"), str(c_file), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
