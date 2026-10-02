#!/usr/bin/env python3
"""Exercise production authored-model normal skinning and guest float layout.

These are CPU checks. The separate Metal fixture covers the reordered opaque
and cutout triangle upload, barycentric interpolation and shading/geometry split.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
world = (ROOT / "port/linux/game/raytrace_world.c").read_text()
geometry = (ROOT / "source/rasterizer/rasterizer_geometry.c").read_text()
matrices = (ROOT / "source/math/matrix_math.c").read_text()


def function(source, name):
    match = re.search(r"(?:^|\n)(?:static )?(?:void|real_vector3d(?: \*)?)\s*" + name + r"\([^;]*?\)\s*\{", source)
    if not match:
        raise RuntimeError("Missing production function " + name)
    start = source.index("{", match.start())
    end, depth = start + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


# Decode with the game's ILP32 shift/sign conversion, not the host's LP64 long.
decode = function(geometry, "uncompress_int32_to_real_vector3d")
decode = re.sub(r"\bunsigned long\b", "uint32_t", decode)
decode = re.sub(r"\blong\b", "int32_t", decode)
vertex = re.search(r"struct model_vertex_view\n\{.*?\n\};", world, re.S).group()
vertex = re.sub(r"\bunsigned long\b", "uint32_t", vertex)
source = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float real;
typedef uint8_t byte;
typedef int boolean;
typedef struct { real x,y,z; } real_point3d;
typedef struct { real i,j,k; } real_vector3d;
typedef struct { float scale; real_vector3d forward,left,up; real_point3d position; } real_matrix4x3;
'''+vertex+decode+function(matrices,"matrix4x3_transform_vector")+function(world,"model_vertex_normal")+r'''
_Static_assert(sizeof(real)==4,"guest real is f32");
_Static_assert(sizeof(struct model_vertex_view)==32,"compressed model vertex ABI");
_Static_assert(sizeof(real_matrix4x3)==52,"guest skinning matrix ABI");
static void near(float a,float b) { assert(isfinite(a)&&fabsf(a-b)<1e-5f); }
int main(void) {
    /* Translation belongs to positions, never to normals. Different bone
       rotations and scales must survive weighted skinning until interpolation. */
    real_matrix4x3 bones[64]={0};
    bones[0]=(real_matrix4x3){1,{1,0,0},{0,1,0},{0,0,1},{300,400,500}};
    bones[43]=(real_matrix4x3){2,{0,1,0},{-1,0,0},{0,0,1},{-200,-300,-400}};
    struct model_vertex_view vertex={.normal=1023u,.node_indices={(char)129,0},.node_weight=8192};
    float out[3];
    model_vertex_normal((const byte*)&vertex,1,bones,64,43,0,0.25f,out);
    near(out[0],0.75f-0.5f/2047.0f);
    near(out[1],0.5f+0.75f/2047.0f);
    near(out[2],1.25f/1023.0f);
    /* The compressed byte can exceed signed-char's range for bone 43. */
    assert((unsigned char)vertex.node_indices[0]/3==43);
    float uncompressed[17]={0};uncompressed[3]=1;
    model_vertex_normal((const byte*)uncompressed,0,bones,64,0,43,0.25f,out);
    near(out[0],0.25f);near(out[1],1.5f);near(out[2],0);
    model_vertex_normal((const byte*)uncompressed,0,bones,64,43,-1,0.1f,out);
    near(out[0],0);near(out[1],2);near(out[2],0);
    model_vertex_normal((const byte*)uncompressed,0,bones,64,-1,-1,0.5f,out);
    near(out[0],0);near(out[1],0);near(out[2],0);
    uncompressed[3]=NAN;
    model_vertex_normal((const byte*)uncompressed,0,bones,64,0,43,0.25f,out);
    near(out[0],0);near(out[1],0);near(out[2],0);
    memset(uncompressed,0,sizeof(uncompressed));
    model_vertex_normal((const byte*)uncompressed,0,bones,64,0,43,0.25f,out);
    near(out[0],0);near(out[1],0);near(out[2],0);
    puts("PASS: model normals use guest f32/packed decode, translated rotated scaled bone skinning, and missing-data fallback");
}
'''
with tempfile.TemporaryDirectory(prefix="halo-mesh-normals-") as directory:
    fixture=Path(directory)/"mesh.c";fixture.write_text(source)
    executable=Path(directory)/"mesh"
    subprocess.run(["clang","-std=c11","-Wall","-Wextra","-Werror",str(fixture),"-o",str(executable)],check=True)
    subprocess.run([str(executable)],check=True)
