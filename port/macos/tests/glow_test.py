#!/usr/bin/env python3
"""Regression tests for the game's glow path, using its actual C functions.

Run from the repository root: python3 port/macos/tests/glow_test.py.
The small host harness avoids loading game maps. UBSan checks arithmetic
and marker bounds; pass --asan to also enable AddressSanitizer. The normal
guest build separately checks the saved-data ABI.
"""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
source = (ROOT / "source/objects/widgets/glow.c").read_text()


def function(name):
    match = re.search(r"(?:^|\n)(?:static )?(?:boolean|void|real) " + name + r"\([^;]*?\)\n\{", source)
    if not match:
        raise RuntimeError("Missing source function " + name)
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


preamble = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float real;
typedef int boolean;
typedef struct { real x, y, z; } real_point3d;
typedef struct { real i, j, k; } real_vector3d;
struct object_marker { struct { real_vector3d forward, up; real_point3d position; } matrix; };
#define MAXIMUM_GLOW_MARKERS 5
#define _real_epsilon 0.0001f
#define NONE -1
#define TRUE 1
#define FALSE 0
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define NUMBEROF(x) (sizeof(x)/sizeof((x)[0]))
#define TEST_FLAG(flags,bit) ((flags) & (1UL << (bit)))
#define SET_FLAG(flags,bit,on) ((flags) = ((flags) & ~(1UL << (bit))) | ((on) ? (1UL << (bit)) : 0))
#define match_assert(file,line,test) assert(test)
#define match_vassert(file,line,test,text) assert(test)
enum { _glow_particle_moving_backwards_bit, _glow_particle_trailing_bit };
enum { _glow_boundary_effect_bounce, _glow_boundary_effect_wrap };
struct glow_datum { short number_of_markers; struct object_marker markers[5]; short marker_order[5]; real total_time, marker_time_index[5]; };
struct glow_particle { short parent_marker_index; real t; real_point3d position; real initial_angle, distance_to_object; unsigned long flags; };
static boolean valid_real(real v) { return isfinite(v); }
static void vector_from_points3d(const real_point3d *a, const real_point3d *b, real_vector3d *v) { v->i=b->x-a->x; v->j=b->y-a->y; v->k=b->z-a->z; }
static real normalize3d(real_vector3d *v) { real m=sqrtf(v->i*v->i+v->j*v->j+v->k*v->k); if(m>0) {v->i/=m;v->j/=m;v->k/=m;} return m; }
static real distance3d(const real_point3d *a,const real_point3d *b) { real_vector3d v; vector_from_points3d(a,b,&v);return normalize3d(&v); }
static void cross_product3d(const real_vector3d *a,const real_vector3d *b,real_vector3d *v) { v->i=a->j*b->k-a->k*b->j;v->j=a->k*b->i-a->i*b->k;v->k=a->i*b->j-a->j*b->i; }
'''

checks = r'''
static void near(real a,real b) { assert(fabsf(a-b)<0.0002f); }
static void path(struct glow_datum *g,int n) {
    memset(g,0,sizeof(*g));g->number_of_markers=n;g->total_time=n-1;
    for(int i=0;i<n;i++) {g->marker_order[i]=n-1-i;g->marker_time_index[i]=i;g->markers[i].matrix.position=(real_point3d){10+2*i,20+4*i,30+10*i};g->markers[i].matrix.up.k=1;g->markers[i].matrix.forward.i=1;}
}
int main(void) {
    struct glow_datum g;struct glow_particle p;
    real_point3d origin={0,11,17},out;real_vector3d direction={1,2,3};point_from_parametric_line(&origin,&direction,2,&out);near(out.z,23);
    for(int n=2;n<=5;n++) {
        path(&g,n);assert(glow_marker_path_valid(&g));
        for(int i=0;i<n;i++) {
            memset(&p,0,sizeof(p));p.t=i;get_particle_world_position(&g,&p,0);
            real_point3d expected=g.markers[g.marker_order[i]].matrix.position;
            near(p.position.x,expected.x);near(p.position.y,expected.y);near(p.position.z,expected.z);
            assert(p.parent_marker_index==g.marker_order[i<n-1?i:n-2]);
        }
        p.t=-1;get_particle_world_position(&g,&p,0);near(p.t,0);
        p.t=100;get_particle_world_position(&g,&p,0);near(p.t,g.total_time);
        p.t=NAN;get_particle_world_position(&g,&p,0);near(p.t,0);
        for(int i=0;i<n;i++) {
            path(&g,n);g.marker_order[i]=-1;assert(!glow_marker_path_valid(&g));get_particle_world_position(&g,&p,0);
            glow_marker_path_initialize(&g);assert(glow_marker_path_valid(&g));
            path(&g,n);g.marker_order[i]=n;assert(!glow_marker_path_valid(&g));glow_marker_path_initialize(&g);assert(glow_marker_path_valid(&g));
        }
        path(&g,n);g.marker_time_index[1]=NAN;assert(!glow_marker_path_valid(&g));glow_marker_path_initialize(&g);assert(glow_marker_path_valid(&g));
        memset(&g,0,sizeof(g));g.number_of_markers=n;glow_marker_path_initialize(&g);assert(glow_marker_path_valid(&g));
        memset(&p,0,sizeof(p));p.t=g.total_time;get_particle_world_position(&g,&p,0);assert(isfinite(p.position.z));
    }
    path(&g,2);g.markers[1].matrix.up.k=2;memset(&p,0,sizeof(p));p.t=.5f;p.initial_angle=1.5707963268f;p.distance_to_object=1;
    get_particle_world_position(&g,&p,0);near(p.position.z,36.5f);
    path(&g,2);memset(&p,0,sizeof(p));p.t=.25f;
    glow_particle_advance_time(&g,&p,2.5f,_glow_boundary_effect_bounce);near(p.t,.75f);assert(!TEST_FLAG(p.flags,0));
    p.t=.25f;p.flags=0;glow_particle_advance_time(&g,&p,1.5f,_glow_boundary_effect_bounce);near(p.t,.25f);assert(TEST_FLAG(p.flags,0));
    p.t=.25f;p.flags=0;glow_particle_advance_time(&g,&p,-.5f,_glow_boundary_effect_wrap);near(p.t,.75f);
    p.t=.25f;glow_particle_advance_time(&g,&p,1e30f,_glow_boundary_effect_bounce);assert(isfinite(p.t)&&p.t>=0&&p.t<=g.total_time);
    p.t=NAN;glow_particle_advance_time(&g,&p,0,_glow_boundary_effect_wrap);near(p.t,0);
    g.total_time=0;glow_particle_advance_time(&g,&p,1,_glow_boundary_effect_wrap);near(p.t,0);
    puts("Glow regression: endpoints, marker permutations, restored tables, coincident markers, finite positions, wrapping and multi-bounce steps passed");
}
'''

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--asan", action="store_true", help="Also enable AddressSanitizer")
args = parser.parse_args()
sanitisers = "undefined,float-divide-by-zero" + (",address" if args.asan else "")

names = ["point_from_parametric_line", "nonuniform_cubic_spline", "nonuniform_cubic_spline_vector3d", "glow_marker_path_valid", "glow_marker_path_initialize", "get_particle_world_position", "glow_particle_advance_time"]
with tempfile.TemporaryDirectory(prefix="halo-glow-test-") as folder:
    folder = Path(folder)
    c_file = folder / "glow_test.c"
    binary = folder / "glow_test"
    c_file.write_text(preamble + "\n".join(function(name) for name in names) + checks)
    subprocess.run(["clang", "-std=c11", "-O1", "-g", "-ffp-contract=off", "-fsanitize=" + sanitisers, str(c_file), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
