"""Whole MCC HUD conversion must preserve equivalent shared authored data.

Synthetic tag arenas use the production structures, converter and layout visitor.
The fixture supplies only allocation, bounds checking, settings and diagnostics.
"""
from pathlib import Path
import json
import re
import shutil
import subprocess
import sys

import pytest

from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def normalization_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is needed for MCC HUD normalization tests")
    work = tmp_path_factory.mktemp("mcc-hud-normalization")
    hud = (ROOT / "port/linux/game/mcc_hud.c").read_text()
    layout = (ROOT / "port/linux/game/mcc_hud_layout.inl").read_text()
    declarations = []
    for path in ["source/interface/hud_definitions.h",
                 "source/interface/weapon_hud_interface_definition.h",
                 "source/bitmaps/bitmap_group.h"]:
        header = (ROOT / path).read_text()
        declarations.extend(structure(header, name) for name in re.findall(r"^struct (\w+)\s*\{", header, re.M))
    body = hud.replace('#include "mcc_hud_layout.inl"', layout)
    body = body.replace('#include "mcc_hud_ownership.inl"',
                        (ROOT / "port/linux/game/mcc_hud_ownership.inl").read_text())
    body = "\n".join(line for line in body.splitlines() if not line.startswith("#include "))
    tags = (ROOT / "port/linux/game/mcc_tags.c").read_text()
    body += "\n" + "\n".join(function(tags, name) for name in [
        "mcc_word", "mcc_block", "mcc_hud"])
    code = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define _hud_anchor_center 4
#define _error_silent 0
#define MCC_HUD_NATIVE_FRAME 0x4D53
typedef unsigned char byte;
typedef unsigned short word;
typedef float real;
typedef union point2d {struct {short x,y;};uint32_t value;} point2d;
typedef struct {float i,j;} real_vector2d;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
typedef struct {float alpha,red,green,blue;} real_argb_color;
typedef struct {float red,green,blue;} real_rgb_color;
typedef struct {float x0,x1,y0,y1;} real_rectangle2d;
typedef struct {float x,y;} real_point2d;
struct tag_reference {long group_tag;char *name;long name_length,index;};
struct tag_block {long count;void *address,*definition;};
struct tag_data {long size,flags,file_offset;void *address,*definition;};
struct mcc_runtime {
    unsigned char *tags,*tag_index;
    uint32_t used,capacity;
    struct {uint32_t tag_count;} report;
};
static unsigned allocation_calls,allocation_fail_at,warnings;
static unsigned heap_calls,heap_fail_at,heap_live;
static uint32_t handle_base;
static void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t address,uint32_t bytes) {
    uintptr_t start=(uintptr_t)r->tags;
    return address>=start && address-start<=r->used && bytes<=r->used-(address-start) ?
        (void *)(uintptr_t)address : NULL;
}
static void *mcc_runtime_allocate(struct mcc_runtime *r,uint32_t bytes) {
    uint32_t at=(r->used+15)&~15u;
    void *p;
    if(++allocation_calls==allocation_fail_at || at>r->capacity || bytes>r->capacity-at)return NULL;
    p=r->tags+at;memset(p,0,bytes);r->used=at+bytes;return p;
}
static void error(short level,char const *text,...) {(void)level;(void)text;warnings++;}
char const *config_string(char const *name) {(void)name;return "tab";}
static void *hud_malloc(size_t bytes) {
    void *p;
    if(bytes>=0x10000000u) {fputs("engine allocation limit exceeded\n",stderr);exit(92);}
    if(++heap_calls==heap_fail_at)return NULL;
    p=malloc(bytes);if(p) {memset(p,0xCA,bytes);heap_live++;}return p;
}
/* cseries.h routes free through debug_free, which requires a live pointer. */
static void hud_free(void *p) {
    if(!p || !heap_live) {fputs("engine free requires a live allocation\n",stderr);exit(91);}
    heap_live--;free(p);
}
#define malloc hud_malloc
#define free hud_free
''' + "\n".join(declarations) + "\n" + body + r'''
#undef malloc
#undef free
static void put_word(void *p,uint32_t value) {memcpy(p,&value,4);}
static void tag(struct mcc_runtime *r,unsigned index,uint32_t group,void *root,int renamed) {
    uint32_t *entry=(uint32_t *)(r->tag_index+index*32);
    char *name=mcc_runtime_allocate(r,64);
    strcpy(name,renamed ? "unrelated\\replacement" : "synthetic\\hud");
    entry[0]=group;entry[1]=entry[2]=UINT32_MAX;
    entry[3]=handle_base|index;entry[4]=(uint32_t)(uintptr_t)name;entry[5]=(uint32_t)(uintptr_t)root;
}
static void placement(struct hud_placement_definition *p,short x,short y,int highres) {
    p->offset.x=x;p->offset.y=y;p->scale.i=1.0f;p->scale.j=2.0f;
    p->multiplayer_scaling_flags=highres ? 4 : 0;p->pad=0x1234;
}
static unsigned char *elements(struct mcc_runtime *r,unsigned count,unsigned stride,int highres) {
    unsigned char *p=mcc_runtime_allocate(r,count*stride);
    unsigned i;
    for(i=0;i<count;i++)placement((void *)(p+i*stride),(short)(100+200*i),-50,highres);
    return p;
}
static void *root(struct mcc_runtime *r,unsigned slot) {
    return (void *)(uintptr_t)((uint32_t *)(r->tag_index+slot*32))[5];
}
static void print_placement(struct hud_placement_definition const *p) {
    printf("[%d,%d,%.6f,%.6f,%d,%d]",p->offset.x,p->offset.y,p->scale.i,p->scale.j,p->pad,p->multiplayer_scaling_flags);
}
static int post_hud(struct mcc_runtime *r) {
    unsigned char *expected=malloc(r->used);
    unsigned i,n;
    int ok=1;
    assert(expected);memcpy(expected,r->tags,r->used);
    for(i=0;i<r->report.tag_count;i++) {
        uint32_t const *entry=(void *)(r->tag_index+i*32);
        unsigned char *p=(void *)(uintptr_t)entry[5],*block;
        uint32_t count;
        if(entry[0]=='unhi') {
            memset(expected+(p-r->tags)+0xF4+0x58,0,12);
            memset(expected+(p-r->tags)+0x1E4+0x58,0,12);
            block=mcc_hud_block(r,p+0x3CC,0x144,&count);assert(block);
            for(n=0;n<count;n++)memset(expected+(block-r->tags)+n*0x144+0x7C+0x58,0,12);
        } else if(entry[0]=='wphi') {
            block=mcc_hud_block(r,p+0x6C,0xB4,&count);assert(block);
            for(n=0;n<count;n++)memset(expected+(block-r->tags)+n*0xB4+0x24+0x58,0,12);
        }
        if(entry[0]=='unhi' || entry[0]=='wphi' || entry[0]=='grhi' || entry[0]=='hudg')
            ok=ok && mcc_hud(r,entry[0],p);
    }
    ok=ok && !memcmp(expected,r->tags,r->used);free(expected);return ok;
}
int main(int argc,char **argv) {
    struct mcc_runtime r={0};
    struct bitmap_group *bitmaps[2];
    struct weapon_hud_interface_definition *weapons[2];
    struct hud_globals_definition *globals[2];
    unsigned char *digits[2],*blocks[2],*items[2],*grenades[2];
    unsigned slots[2],i,j,count,original_used;
    unsigned char *original;
    int reverse,half,highres,renamed,ok,post_ok=0;
    char const *kind,*sharing;
    if(argc!=7 && argc!=9)return 2;
    kind=argv[1];sharing=argv[2];reverse=atoi(argv[3]);half=atoi(argv[4]);highres=atoi(argv[5]);renamed=atoi(argv[6]);
    handle_base=renamed ? 0xF2000000u : 0xE1000000u;
    r.capacity=65536;r.tags=calloc(1,r.capacity);assert(r.tags);
    r.tag_index=mcc_runtime_allocate(&r,8*32);r.report.tag_count=!strcmp(kind,"globals") ? 5 : 4;
    slots[0]=reverse ? 1 : 0;slots[1]=reverse ? 0 : 1;
    for(i=0;i<2;i++) {
        bitmaps[i]=mcc_runtime_allocate(&r,sizeof(*bitmaps[i]));
        bitmaps[i]->flags=i && half ? 0x10 : 0;
        tag(&r,2+i,'bitm',bitmaps[i],renamed);
    }
    if(!strcmp(kind,"bitmaps") || !strcmp(kind,"invalidcount")) {
        memcpy(r.tag_index,r.tag_index+64,64);
        for(i=0;i<2;i++)put_word(r.tag_index+i*32+12,handle_base|i);
        r.report.tag_count=!strcmp(kind,"bitmaps") ? 2 : !strcmp(sharing,"overflow") ? 65536 : 0;
    } else if(!strcmp(kind,"globals")) {
        struct weapon_hud_interface_definition *empty=mcc_runtime_allocate(&r,sizeof(*empty));
        empty->parent_hud.index=NONE;tag(&r,4,'wphi',empty,renamed);
        for(i=0;i<2;i++) {
            globals[i]=mcc_runtime_allocate(&r,sizeof(*globals[i]));
            placement(&globals[i]->messaging.placement,0,120,highres);
            globals[i]->waypoint.top_offset=120;
            globals[i]->damage_indicators.top_offset=120;
            globals[i]->defaults.default_weapon_hud.group_tag='wphi';
            globals[i]->defaults.default_weapon_hud.index=handle_base|4;
        }
        if(!strcmp(sharing,"root"))globals[1]=globals[0];
        for(i=0;i<2;i++)tag(&r,slots[i],'hudg',globals[i],renamed);
    } else if(!strcmp(kind,"digits")) {
        for(i=0;i<2;i++) {
            digits[i]=mcc_runtime_allocate(&r,0x64);digits[i][0x11]=12;digits[i][0x14]=4;
        }
        if(!strcmp(sharing,"root"))digits[1]=digits[0];
        for(i=0;i<2;i++)tag(&r,slots[i],'hud#',digits[i],renamed);
    } else if(!strcmp(kind,"unit")) {
        static unsigned const placements[]={0x24,0x8C,0xF4,0x17C,0x1E4,0x26C,0x2D4};
        for(i=0;i<2;i++) {
            unsigned char *aux=mcc_runtime_allocate(&r,0x84),*meter=mcc_runtime_allocate(&r,0x144);
            grenades[i]=mcc_runtime_allocate(&r,0x56C);
            for(j=0;j<NUMBEROF(placements);j++) {
                placement((void *)(grenades[i]+placements[j]),100,-50,highres);
                put_word(grenades[i]+placements[j]+0x30,handle_base|2);
            }
            placement((void *)(grenades[i]+0x35C),100,-50,highres);
            placement((void *)aux,100,-50,highres);put_word(aux+0x30,handle_base|2);
            placement((void *)(meter+0x14),100,-50,highres);put_word(meter+0x44,handle_base|2);
            placement((void *)(meter+0x7C),100,-50,highres);put_word(meter+0xAC,handle_base|2);
            put_word(grenades[i]+0x3A4,1);put_word(grenades[i]+0x3A8,(uint32_t)(uintptr_t)aux);
            put_word(grenades[i]+0x3CC,1);put_word(grenades[i]+0x3D0,(uint32_t)(uintptr_t)meter);
            memset(grenades[i]+0xF4+0x58,0xA5,12);memset(grenades[i]+0x1E4+0x58,0xA5,12);
            memset(meter+0x7C+0x58,0xA5,12);
        }
        if(!strcmp(sharing,"root"))grenades[1]=grenades[0];
        for(i=0;i<2;i++)tag(&r,slots[i],'unhi',grenades[i],renamed);
    } else if(!strcmp(kind,"grenade")) {
        for(i=0;i<2;i++) {
            struct hud_placement_definition *p;
            grenades[i]=mcc_runtime_allocate(&r,0x1F8);
            p=(void *)(grenades[i]+0x24);placement(p,164,2,0);p->scale.j=1;
            p=(void *)(grenades[i]+0x8C);placement(p,164,2,0);p->scale.j=1;
            p=(void *)(grenades[i]+0xF4);placement(p,(short)(214+2*i),6,0);p->scale.j=1;
            put_word(grenades[i]+0x54,UINT32_MAX);put_word(grenades[i]+0xBC,UINT32_MAX);
            grenades[i][0x138]=grenades[i][0x139]=1;
            items[i]=elements(&r,1,0x88,highres);
            put_word(grenades[i]+0x158,handle_base|(2+i));
            put_word(grenades[i]+0x15C,1);
        }
        if(!strcmp(sharing,"block"))items[1]=items[0];
        for(i=0;i<2;i++) {
            put_word(grenades[i]+0x160,(uint32_t)(uintptr_t)items[i]);
            tag(&r,slots[i],'grhi',grenades[i],renamed);
        }
    } else {
        for(i=0;i<2;i++) {
            weapons[i]=mcc_runtime_allocate(&r,sizeof(*weapons[i]));
            weapons[i]->parent_hud.index=NONE;
        }
        if(!strcmp(kind,"static") || !strcmp(kind,"meter")) {
            for(i=0;i<2;i++) {
                blocks[i]=mcc_runtime_allocate(&r,0xB4);
                placement((void *)(blocks[i]+0x24),100,-50,highres);
                put_word(blocks[i]+0x54,handle_base|2);
                if(!strcmp(kind,"meter"))memset(blocks[i]+0x7C,0xA5,12);
            }
            if(!strcmp(sharing,"block"))blocks[1]=blocks[0];
            for(i=0;i<2;i++) {
                struct tag_block *b=!strcmp(kind,"meter") ? &weapons[i]->meters : &weapons[i]->statics;
                b->count=1;b->address=blocks[i];
            }
        } else {
            unsigned extent=!strcmp(sharing,"prefix") || !strcmp(sharing,"shifted") ? 2 : 1;
            for(i=0;i<2;i++) {
                blocks[i]=mcc_runtime_allocate(&r,0x68);
                items[i]=elements(&r,extent,0x88,highres);
                put_word(blocks[i]+0x30,handle_base|(2+i));
                put_word(blocks[i]+0x34,extent);
                put_word(blocks[i]+0x38,(uint32_t)(uintptr_t)items[i]);
            }
            if(!strcmp(sharing,"block") || !strcmp(sharing,"prefix"))
                put_word(blocks[1]+0x38,(uint32_t)(uintptr_t)items[0]);
            if(!strcmp(sharing,"prefix"))put_word(blocks[0]+0x34,1);
            if(!strcmp(sharing,"shifted")) {
                put_word(blocks[1]+0x34,1);
                put_word(blocks[1]+0x38,(uint32_t)(uintptr_t)(items[0]+0x88));
            }
            if(!strncmp(sharing,"shifted-fields",14)) {
                unsigned char *overlap=mcc_runtime_allocate(&r,0x90);
                placement((void *)overlap,100,-50,0);
                placement((void *)(overlap+8),300,3,0);
                if(!strcmp(sharing,"shifted-fields-copy")) {
                    memcpy(items[0],overlap,0x88);memcpy(items[1],overlap+8,0x88);
                } else {items[0]=overlap;items[1]=overlap+8;}
                for(i=0;i<2;i++)put_word(blocks[i]+0x38,(uint32_t)(uintptr_t)items[i]);
            }
            if(!strcmp(sharing,"owner"))blocks[1]=blocks[0];
            for(i=0;i<2;i++) {
                weapons[i]->overlays.count=1;weapons[i]->overlays.address=blocks[i];
            }
            if(!strcmp(sharing,"outside"))put_word(blocks[1]+0x38,(uint32_t)(uintptr_t)(r.tags+r.capacity-1));
        }
        if(!strcmp(sharing,"root"))weapons[1]=weapons[0];
        for(i=0;i<2;i++)tag(&r,slots[i],'wphi',weapons[i],renamed);
    }
    original_used=r.used;original=malloc(original_used);assert(original);
    memcpy(original,r.tags,original_used);
    if(argc==9) {allocation_fail_at=atoi(argv[7]);heap_fail_at=atoi(argv[8]);}
    allocation_calls=warnings=heap_calls=0;
    ok=mcc_hud_bitmaps_prepare(&r);
    if(ok)post_ok=post_hud(&r);
    printf("{\"ok\":%d,\"allocations\":%u,\"warnings\":%u,\"heap_calls\":%u,\"heap_live\":%u,"
        "\"original_unchanged\":%d,\"post_ok\":%d,\"values\":[",ok,allocation_calls,warnings,heap_calls,heap_live,
        !memcmp(original,r.tags,original_used),post_ok);
    for(i=0;i<2;i++) {
        if(i)putchar(',');
        if(!strcmp(kind,"bitmaps") || !strcmp(kind,"invalidcount")) {
            printf("0");
        } else if(!strcmp(kind,"globals")) {
            struct hud_globals_definition const *g=root(&r,slots[i]);
            printf("[");print_placement(&g->messaging.placement);
            printf(",%.6f,%d]",g->waypoint.top_offset,g->damage_indicators.top_offset);
        } else if(!strcmp(kind,"digits")) {
            signed char const *p=root(&r,slots[i]);printf("[%d,%d]",p[0x11],p[0x14]);
        } else if(!strcmp(kind,"grenade")) {
            unsigned char const *p=root(&r,slots[i]);
            items[i]=(void *)(uintptr_t)mcc_hud_word(p+0x160);
            print_placement((void *)items[i]);
        } else if(!strcmp(kind,"unit")) {
            print_placement((void *)((unsigned char *)root(&r,slots[i])+0x24));
        } else {
            struct weapon_hud_interface_definition const *w=root(&r,slots[i]);
            if(!strcmp(kind,"static") || !strcmp(kind,"meter"))
                print_placement((void *)((unsigned char *)(!strcmp(kind,"meter") ? w->meters.address : w->statics.address)+0x24));
            else {
                unsigned char const *b=w->overlays.address;
                count=mcc_hud_word(b+0x34);
                items[i]=(void *)(uintptr_t)mcc_hud_word(b+0x38);
                printf("[");
                if(ok)for(j=0;j<count;j++) {if(j)putchar(',');print_placement((void *)(items[i]+j*0x88));}
                printf("]");
            }
        }
    }
    printf("]}\n");free(original);free(r.tags);return 0;
}
'''
    path = work / "normalization.c"
    path.write_text(code)
    binary = work / ("normalization.exe" if sys.platform == "win32" else "normalization")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if sys.platform == "win32" else ["-m32"]
    command = [compiler, *target, "-std=gnu89", "-Wall", "-Wextra", "-Werror", "-Wno-multichar",
               str(path), "-o", str(binary)]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


def normalize(tool, kind, sharing, reverse=0, half=0, highres=0, renamed=0, fail_runtime=0, fail_heap=0):
    result = subprocess.run([str(tool), kind, sharing, str(reverse), str(half), str(highres), str(renamed),
                             str(fail_runtime), str(fail_heap)],
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stderr
    return json.loads(result.stdout)


def test_each_planning_allocation_failure_leaves_input_unpublished(normalization_tool):
    control = normalize(normalization_tool, "overlay", "owner")
    assert control["ok"] == 1
    assert control["allocations"] == 3  # One shared owner and two scalar views.
    assert control["heap_live"] == 0
    for allocator, calls in [("fail_runtime", control["allocations"]), ("fail_heap", control["heap_calls"])]:
        for fail_at in range(1, calls + 1):
            result = normalize(normalization_tool, "overlay", "owner", **{allocator: fail_at})
            assert result["ok"] == 0, (allocator, fail_at, result)
            assert result["heap_live"] == 0
            assert result["original_unchanged"] == 1


@pytest.mark.parametrize("kind", ["globals", "digits", "static"])
def test_unique_pointer_owners_do_not_allocate_runtime_copies(normalization_tool, kind):
    result = normalize(normalization_tool, kind, "separate")
    assert result["ok"] == 1
    assert result["allocations"] == result["heap_live"] == 0


def test_valid_bitmap_only_table_needs_no_hud_owned_blocks(normalization_tool):
    result = normalize(normalization_tool, "bitmaps", "separate")
    assert result["ok"] == result["post_ok"] == 1
    assert result["allocations"] == result["heap_live"] == 0


@pytest.mark.parametrize("count", ["empty", "overflow"])
def test_invalid_tag_count_disposes_an_empty_plan(normalization_tool, count):
    result = normalize(normalization_tool, "invalidcount", count)
    assert result["ok"] == result["heap_live"] == result["heap_calls"] == 0
    assert result["original_unchanged"] == 1


@pytest.mark.parametrize("kind,highres", [("globals", 0), ("globals", 1), ("digits", 0), ("static", 0), ("static", 1)])
@pytest.mark.parametrize("sharing", ["separate", "root"])
@pytest.mark.parametrize("reverse", [0, 1])
def test_shared_roots_match_independent_normalization(normalization_tool, kind, highres, sharing, reverse):
    result = normalize(normalization_tool, kind, sharing, reverse, highres=highres)
    assert result["ok"] == 1
    expected = {"globals": [[0, 120 if highres else 60, 1.0, 2.0, 0 if highres else 0x4D53, 0], 60.0, 60],
                "digits": [6, 2], "static": [100 if highres else 50, -50 if highres else -25, 0.5, 1.0, 0, 0]}[kind]
    assert result["values"] == [expected, expected]


@pytest.mark.parametrize("sharing", ["separate", "block", "owner", "prefix", "shifted"])
@pytest.mark.parametrize("reverse", [0, 1])
@pytest.mark.parametrize("half", [0, 1])
def test_shared_overlay_items_keep_each_consumers_context(normalization_tool, sharing, reverse, half):
    result = normalize(normalization_tool, "overlay", sharing, reverse, half)
    assert result["ok"] == 1
    second_half = half and sharing != "owner"
    a = [50, -25, 0.5, 1.0, 0, 0]
    b = [50, -25, 0.25 if second_half else 0.5, 0.5 if second_half else 1.0, 0, 0]
    if sharing == "prefix":
        expected = [[a], [b, [150, *b[1:]]]]
    elif sharing == "shifted":
        expected = [[a, [150, *a[1:]]], [[150, *b[1:]]]]
    else:
        expected = [[a], [b]]
    assert result["values"] == expected


def test_outside_overlay_is_refused(normalization_tool):
    assert normalize(normalization_tool, "overlay", "outside")["ok"] == 0


@pytest.mark.parametrize("kind", ["globals", "digits", "static", "overlay"])
def test_alias_normalization_is_independent_of_names_and_handles(normalization_tool, kind):
    control = normalize(normalization_tool, kind, "separate", half=1)
    shared = normalize(normalization_tool, kind, "block" if kind == "overlay" else "root", half=1, renamed=1)
    assert control["ok"] == shared["ok"] == 1
    assert shared["values"] == control["values"]


@pytest.mark.parametrize("reverse", [0, 1])
@pytest.mark.parametrize("half", [0, 1])
def test_shifted_scalar_padding_cannot_change_another_consumers_authored_scale(normalization_tool, reverse, half):
    control = normalize(normalization_tool, "overlay", "shifted-fields-copy", reverse, half)
    shared = normalize(normalization_tool, "overlay", "shifted-fields", reverse, half)
    assert control["ok"] == shared["ok"] == 1
    assert shared["values"] == control["values"]
    assert shared["values"][1][0][2] == (0.25 if half else 0.5)


@pytest.mark.parametrize("reverse", [0, 1])
@pytest.mark.parametrize("half", [0, 1])
def test_shared_items_keep_distinct_structural_frame_choices(normalization_tool, reverse, half):
    # Numeric panel geometry only; both bitmap references are absent. The
    # second panel differs by two authored pixels and must retain its frame.
    control = normalize(normalization_tool, "grenade", "separate", reverse, half)
    shared = normalize(normalization_tool, "grenade", "block", reverse, half, renamed=1)
    assert control["ok"] == shared["ok"] == 1
    assert shared["values"] == control["values"]
    assert shared["values"] == [[50, -25, 0.5, 1.0, 0x4D53, 0],
                                [50, -25, 0.25 if half else 0.5, 0.5 if half else 1.0, 0, 0]]


@pytest.mark.parametrize("kind", ["unit", "static", "meter", "grenade", "globals"])
@pytest.mark.parametrize("highres", [0, 1])
def test_later_metadata_pass_only_clears_unsupported_meter_overlays(normalization_tool, kind, highres):
    result = normalize(normalization_tool, kind, "separate", highres=highres)
    assert result["ok"] == result["post_ok"] == 1
