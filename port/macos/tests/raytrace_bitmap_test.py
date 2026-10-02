#!/usr/bin/env python3
"""CPU regressions for the actual ray tracing bitmap and registry code.

Run from the repository root: python3 port/macos/tests/raytrace_bitmap_test.py.
Texture-cache availability and bitmap pixel reads are mocked; production
averaging, RGBA uploads, alpha/surface registries, packed payloads and reset
logic are extracted unchanged. Guest integer words are represented with
fixed-width types while the test keeps native host pointers.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
source = (ROOT / "port/linux/game/raytrace_world.c").read_text()
reference = (ROOT / "source/rasterizer/xbox/rasterizer_xbox_models.c").read_text()


def function(name):
    match = re.search(r"(?:^|\n)(?:static )?(?:boolean|void|long|float) " + name + r"\([^;]*?\)\n\{", source)
    if not match:
        raise RuntimeError("Missing source function " + name)
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def structure(text, name):
    match = re.search(r"struct " + name + r"\n\{[^}]*\};", text)
    if not match:
        raise RuntimeError("Missing source structure " + name)
    return match.group(0)


def guest_words(text):
    text = re.sub(r"\bunsigned long\b", "uint32_t", text)
    text = re.sub(r"\blong\b", "int32_t", text)
    # Only the hash consumes integer pointer bits. A host pointer is otherwise
    # kept intact in this native harness, unlike the ILP32 game's pointers.
    return text.replace("(uint32_t)bitmap", "(uint32_t)(uintptr_t)bitmap")


preamble = r'''
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float real;
typedef unsigned char byte;
typedef uint16_t word;
typedef uint32_t pixel32;
typedef int boolean;
typedef struct { real x,y; } real_point2d;
typedef struct { real i,j; } real_vector2d;
typedef struct { real red,green,blue; } real_rgb_color;
struct shader { byte data[0x28]; };
struct tag_reference { uint32_t group,name;int32_t length,index; };
struct collision_bsp { struct { void *address;int32_t count; } surfaces,edges,vertices; };
struct collision_surface;struct collision_edge;struct collision_vertex;
struct bitmap_data { int32_t width,height,mipmap_count,type,flags,format;void *base_address;boolean loaded,checker;pixel32 color,other; };
#define FALSE 0
#define TRUE 1
#define NONE (-1)
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PIN(x,a,b) MIN(MAX((x),(a)),(b))
static uint32_t surface_generation;
static int pixel_reads;
static float last_lod;
static void *_texture_cache_bitmap_get_hardware_format(struct bitmap_data *bitmap,boolean a,boolean b) { assert(!a&&b);return bitmap->loaded?bitmap:NULL; }
static pixel32 bitmap_2d_get_pixel(struct bitmap_data *bitmap,const real_point2d *point,float lod) {
    assert(lod>=0&&lod<=1);assert(point->x>=0&&point->x<=1&&point->y>=0&&point->y<=1);pixel_reads++;last_lod=lod;
    return bitmap->checker&&point->x>=.5f?bitmap->other:bitmap->color;
}
static struct bitmap_data bitmap(pixel32 color) { return (struct bitmap_data){.width=16,.height=16,.format=11,.base_address=(void*)1,.loaded=TRUE,.color=color}; }
static uint32_t payload(float value) { uint32_t word;memcpy(&word,&value,4);return word; }
static void near(float a,float b) { assert(fabsf(a-b)<.00001f); }
/* Match cseries.h's allocator ownership without relying on reading a debug
   header before an unowned libc allocation. calloc deliberately remains libc. */
static void *owned_allocations[32];
static int allocation_calls, allocation_fail_at=-1, allocation_live;
static void *fixture_debug_malloc(size_t size) {
    if(allocation_calls++==allocation_fail_at)return NULL;
    void *p=malloc(size);assert(p);memset(p,0xa5,size);
    for(int i=0;i<32;i++)if(!owned_allocations[i]) {owned_allocations[i]=p;allocation_live++;return p;}
    abort();
}
static void fixture_debug_free(void *p) {
    for(int i=0;i<32;i++)if(owned_allocations[i]==p) {owned_allocations[i]=NULL;allocation_live--;free(p);return;}
    assert(!"level cleanup received a pointer outside Halo's debug allocator");
}
#define malloc(size) fixture_debug_malloc(size)
#define free(pointer) fixture_debug_free(pointer)
'''

blocks = []
world = re.search(r"static struct\s*\{[^}]*\} world;", source)
if not world:
    raise RuntimeError("Missing world registry")
blocks.append(world.group(0))
for capacity, registry in [("RAY_MASKS", "masks"), ("RAY_SURFACES", "surfaces")]:
    match = re.search(r"#define " + capacity + r" \d+\s+static struct\s*\{[^}]*\} " + registry + r";", source)
    if not match:
        raise RuntimeError("Missing registry " + registry)
    blocks.append(match.group(0))
blocks.append(re.search(r"static struct\s*\{[^}]*\} normals;", source).group(0))
blocks.append(structure(source, "ray_level_shader_environment"))
groups = re.search(r"#define RAY_TRACED_OBJECT_GROUPS \d+", source)
group_slots = re.search(r"static struct ray_object_group_slot\s*\{[^}]*\} ray_object_groups\[[^;]*;", source)
if not groups or not group_slots:
    raise RuntimeError("Missing stable object group slots")
blocks.extend([groups.group(0), group_slots.group(0)])
blocks.extend(function(name) for name in ["ray_object_groups_reset", "ray_object_groups_begin",
              "ray_object_group_keep", "ray_object_groups_sweep", "ray_object_group_get", "ray_object_group_release"])
for name in ["shader_environment_diffuse_properties", "shader_environment_specular_properties", "shader_environment_reflection_properties", "shader_environment_properties", "shader_environment_definition"]:
    blocks.append(structure(reference, name))

names = ["level_shader_uses_bump", "level_bitmap_readable", "level_bitmap_average_linear", "mask_index", "surface_index", "normal_index", "halo_ray_tracing_normal", "reset_rt_bitmaps", "halo_ray_tracing_surface", "halo_ray_tracing_mask", "object_albedo"]
blocks.extend(function(name) for name in names)
# Execute the production teardown/reset/null-validation prefix; geometry
# triangulation after allocation belongs to the game integration tests.
world_build = function("world_build")
blocks.append(world_build[:world_build.index("\n\tworld.vertices = malloc")] + "\n}")
# Exercise the real allocation and teardown statements as a pair. A fixture
# that maps both malloc and calloc to libc would hide allocator mismatches.
level = re.search(r"static struct\s*\{[^}]*\} level;", source)
if not level:
    raise RuntimeError("Missing level registry")
blocks.append("#define RAY_LEVEL_MATERIAL_FLOATS 8\n" + level.group(0))
blocks.append(function("level_free"))
level_build = function("level_build")
allocation_start = level_build.index("\n\tlevel.vertices = malloc")
allocation_end = level_build.index("\n\tlevel.page_count = bsp->lightmaps.count;")
blocks.append("static void level_allocate_fixture(long material_count, long vertex_count, long triangle_count, long pages) {\n"
              "struct { struct { long count; } lightmaps; } storage={{pages}}, *bsp=&storage;\n"
              + level_build[allocation_start:allocation_end] + "\n}")

checks = r'''
_Static_assert(offsetof(struct ray_level_shader_environment,reflection_perpendicular_brightness)==offsetof(struct shader_environment_definition,environment.reflection.view_perpendicular_brightness),"reflection layout differs from renderer");
_Static_assert(offsetof(struct ray_level_shader_environment,reflection_cube_map)==offsetof(struct shader_environment_definition,environment.reflection.cube_map),"cube map layout differs from renderer");
_Static_assert(offsetof(struct ray_level_shader_environment,reflection_perpendicular_brightness)==0x2F4,"brightness offset");
_Static_assert(offsetof(struct ray_level_shader_environment,reflection_cube_map)==0x324,"cube map offset");
_Static_assert(offsetof(struct ray_level_shader_environment,bump_map)==offsetof(struct shader_environment_definition,environment.diffuse.bump_map),"bump layout differs from renderer");
_Static_assert(offsetof(struct ray_level_shader_environment,runtime_bump_map_scale)==offsetof(struct shader_environment_definition,environment.diffuse.runtime_bump_map_scale),"bump scale layout differs from renderer");
_Static_assert(offsetof(struct ray_level_shader_environment,bump_map)==0x128,"bump offset");
_Static_assert(offsetof(struct ray_level_shader_environment,runtime_bump_map_scale)==0x138,"bump scale offset");
int main(void) {
    {
    struct ray_level_shader_environment shader={0};shader.bump_map.index=3;
    assert(level_shader_uses_bump(&shader));shader.environment_flags=2;assert(!level_shader_uses_bump(&shader));
    shader.environment_flags=1;assert(level_shader_uses_bump(&shader));shader.bump_map.index=NONE;assert(!level_shader_uses_bump(&shader));
    struct bitmap_data b=bitmap(0xff8080ff);b.format=17;
    const unsigned char *pixels;const float *scale;int32_t index,w,h;uint32_t generation;
    assert(level_bitmap_readable(&b));
    assert(normal_index(NULL,1,1)==-1&&normal_index(&b,NAN,1)==-1&&normal_index(&b,0,1)==-1);
    assert(normal_index(&b,2,3)==0&&normal_index(&b,2,3)==0&&normal_index(&b,3,2)==1);
    assert(halo_ray_tracing_normal(&index,&pixels,&w,&h,&generation,&scale));
    assert(index==0&&pixels[0]==128&&pixels[1]==128&&pixels[2]==255);near(scale[0],2);near(scale[1],3);
    assert(halo_ray_tracing_normal(&index,&pixels,&w,&h,&generation,&scale)&&index==1);
    assert(!halo_ray_tracing_normal(&index,&pixels,&w,&h,&generation,&scale));
    for(int i=2;i<254;i++)assert(normal_index(&b,(float)i,1)==i);
    assert(normal_index(&b,254,1)==-1&&masks.count==0&&surfaces.count==0);
    reset_rt_bitmaps();assert(normals.count==0);assert(normal_index(&b,1,1)==0);
    assert(halo_ray_tracing_normal(&index,&pixels,&w,&h,&generation,&scale)&&generation==surface_generation);
    reset_rt_bitmaps();
    b.width=512;b.height=256;b.mipmap_count=3;normal_index(&b,1,1);
    assert(halo_ray_tracing_normal(&index,&pixels,&w,&h,&generation,&scale)&&w==256&&h==128);
    near(last_lod,1.0f-1.25f/3.0f);assert(pixels[(w*h-1)*4+2]==255);
    reset_rt_bitmaps();
    }
    /* An earlier corpse disappearing or a new early iterator entry must not
       change surviving units' mesh IDs. Retain residents before new entries. */
    {
    int32_t a=0x10001,b=0x10002,c=0x10003,d=0x20001;
    ray_object_groups_reset();assert(ray_object_group_get(a)==1);
    assert(ray_object_group_get(b)==2&&ray_object_group_get(c)==3);
    ray_object_groups_begin();ray_object_group_keep(c);ray_object_group_keep(b);ray_object_groups_sweep();
    assert(ray_object_group_get(d)==1&&ray_object_group_get(c)==3&&ray_object_group_get(b)==2);
    assert(ray_object_group_get(d)==1); /* Duplicate request keeps one slot. */
    ray_object_groups_begin();ray_object_group_keep(b);ray_object_group_keep(c);ray_object_group_keep(d);ray_object_groups_sweep();
    assert(ray_object_group_get((int32_t)0xf0010001u)==4); /* Full salt bits, not low16 index. */
    assert(ray_object_group_get(NONE)==31);
    ray_object_group_release(d);assert(ray_object_group_get(a)==1);
    ray_object_groups_reset();
    for(int i=0;i<30;i++)assert(ray_object_group_get(i)==i+1);
    assert(ray_object_group_get(30)==31&&ray_object_group_get(31)==31); /* Overflow remains loose. */
    ray_object_groups_begin();for(int i=1;i<30;i++)ray_object_group_keep(i);ray_object_groups_sweep();
    assert(ray_object_group_get(100)==1);for(int i=1;i<30;i++)assert(ray_object_group_get(i)==i+1);
    reset_rt_bitmaps();assert(ray_object_group_get(100)==1&&ray_object_group_get(29)==2);
    ray_object_groups_reset();
    }
    /* A populated BSP can be replaced repeatedly, and every failed allocation
       must release all earlier blocks through the same allocator. */
    for(int repeat=0;repeat<3;repeat++) {
        allocation_calls=0;allocation_fail_at=-1;level_allocate_fixture(7,11,13,2);
        assert(allocation_live==12&&allocation_calls==12&&level.surface_properties);
        for(int i=0;i<7*4;i++)assert(level.surface_properties[i]==0.f);
        level_free();assert(allocation_live==0&&!level.surface_properties);
        level_free();assert(allocation_live==0);
    }
    for(int fail=0;fail<12;fail++) {
        allocation_calls=0;allocation_fail_at=fail;level_allocate_fixture(7,11,13,2);
        assert(allocation_live==0&&!level.vertices&&!level.surface_properties&&!level.pages_done);
    }
    allocation_fail_at=-1;
    struct bitmap_data b=bitmap(0xff808080u),items[256];float average[3];
    level_bitmap_average_linear(&b,average);for(int i=0;i<3;i++)near(average[i],.2158605f);
    b.color=0xff000000u;b.other=0xffffffffu;b.checker=TRUE;
    level_bitmap_average_linear(&b,average);for(int i=0;i<3;i++)near(average[i],.5f);
    assert(average[0]>.4f); /* Decode(mean(encoded)) would incorrectly be about .214. */
    b.checker=FALSE;b.color=0xff0a0a0au;level_bitmap_average_linear(&b,average);near(average[0],10.f/255.f/12.92f);
    b.color=0xff0b0b0bu;level_bitmap_average_linear(&b,average);near(average[0],powf((11.f/255.f+.055f)/1.055f,2.4f));
    assert(level_bitmap_readable(&b));b.loaded=FALSE;assert(!level_bitmap_readable(&b));b.loaded=TRUE;
    b.flags=1<<4;assert(!level_bitmap_readable(&b));b.flags=0;b.format=31;assert(!level_bitmap_readable(&b));b.format=11;

    reset_rt_bitmaps();
    for(int i=0;i<254;i++) {items[i]=bitmap(0xffffffffu);assert(surface_index(&items[i])==i);assert(surface_index(&items[i])==i);}
    assert(surfaces.count==254&&masks.count==0);items[254]=bitmap(0xffffffffu);assert(surface_index(&items[254])==-1);
    for(int i=0;i<254;i++) {
        float stored=object_albedo(&items[i]);uint32_t bits=payload(stored);assert((bits>>24)==(uint32_t)(i+2));assert((bits&0xffffffu)==0xffffffu);
        /* Payloads include signed and NaN float bit patterns. They must survive
           float return/assignment and memcpy without numeric conversion. */
        float copied[8]={0};copied[7]=stored;assert(payload(copied[7])==bits);
    }
    assert((payload(object_albedo(&items[254]))>>24)==1u);
    assert(masks.count==0);items[255]=bitmap(0xffffffffu);for(int i=0;i<256;i++)assert(mask_index(&items[i])==i);assert(masks.count==256);assert(mask_index(&b)==-1);
    assert(mask_index(NULL)==-1&&surface_index(NULL)==-1);

    uint32_t before_world_reset=surface_generation;world.vertices=malloc(12);world.indices=malloc(12);world.vertex_count=world.triangle_count=1;
    world_build(NULL);assert(!world.vertices&&!world.indices&&!world.bsp&&!world.vertex_count&&!world.triangle_count);assert(surface_generation!=before_world_reset&&surfaces.count==0&&masks.count==0);
    reset_rt_bitmaps();b=bitmap(0xff808080u);int32_t index,w,h;const unsigned char *pixels;uint32_t generation;
    assert(surface_index(&b)==0);assert(halo_ray_tracing_surface(&index,&pixels,&w,&h,&generation));
    assert(index==0&&w==16&&h==16&&generation==surface_generation);assert(pixels[0]==128&&pixels[1]==128&&pixels[2]==128&&pixels[3]==255);
    assert(!halo_ray_tracing_surface(&index,&pixels,&w,&h,&generation));
    uint32_t old=payload(object_albedo(&b));int reads=pixel_reads;b.color=0xff000000u;
    assert(payload(object_albedo(&b))==old&&pixel_reads==reads);uint32_t old_generation=surface_generation;reset_rt_bitmaps();
    assert(surface_generation!=old_generation&&surfaces.count==0&&masks.count==0);
    uint32_t fresh=payload(object_albedo(&b));assert((fresh&0xffffffu)==0u&&fresh!=old&&pixel_reads>reads);
    assert(halo_ray_tracing_surface(&index,&pixels,&w,&h,&generation)&&generation==surface_generation&&pixels[0]==0);
    reset_rt_bitmaps();b=bitmap(0x33112244u);b.width=b.height=256;b.mipmap_count=4;surface_index(&b);
    assert(halo_ray_tracing_surface(&index,&pixels,&w,&h,&generation));assert(w==128&&h==128);near(last_lod,.6875f);
    assert(pixels[0]==0x11&&pixels[1]==0x22&&pixels[2]==0x44&&pixels[3]==0x33);
    assert(mask_index(&b)==0);assert(halo_ray_tracing_mask(&index,&pixels,&w,&h,&generation));assert(w==128&&h==128&&pixels[0]==0x33&&generation==masks.generation);
    /* A partial mip chain reaches its final level before the alpha mask
       fits the atlas. The fractional mip bias must never make LOD negative. */
    for(int mips=0;mips<=3;mips++) {
        reset_rt_bitmaps();b=bitmap(0x73112244u);b.width=b.height=256;b.mipmap_count=mips;
        assert(mask_index(&b)==0);
        int ok=halo_ray_tracing_mask(&index,&pixels,&w,&h,&generation);
        if(!mips)assert(!ok); /* Oversized no-mip texture is not a mask tile. */
        else {assert(ok&&pixels[0]==0x73);assert(isfinite(last_lod)&&last_lod>=0&&last_lod<=1);}
    }
    reset_rt_bitmaps();b=bitmap(0x73112244u);b.width=b.height=1024;b.mipmap_count=3;mask_index(&b);
    assert(halo_ray_tracing_mask(&index,&pixels,&w,&h,&generation));assert(w==128&&h==128);near(last_lod,0);
    reset_rt_bitmaps();b.loaded=FALSE;surface_index(&b);assert(!halo_ray_tracing_surface(&index,&pixels,&w,&h,&generation));b.loaded=TRUE;assert(halo_ray_tracing_surface(&index,&pixels,&w,&h,&generation));
    puts("RT bitmap regression: stable salted object groups/reclamation/overflow/reset, allocator ownership/failed-allocation cleanup, linear averages, payload IDs 2-255, independent alpha capacity, RGBA mipmaps, BSP/NULL teardown cache reset and renderer bump/reflection offsets, normal registry capacity/tiling/linear P8/mask eligibility passed");
}
'''

with tempfile.TemporaryDirectory(prefix="halo-rt-bitmap-test-") as folder:
    folder = Path(folder)
    c_file = folder / "raytrace_bitmap_test.c"
    binary = folder / "raytrace_bitmap_test"
    c_file.write_text(preamble + guest_words("\n".join(blocks)) + checks)
    subprocess.run(["clang", "-std=c11", "-O2", "-g", "-ffp-contract=off", "-fsanitize=undefined,float-divide-by-zero", str(c_file), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
