#!/usr/bin/env python3
"""Check the renderer-to-RT camera boundary without loading a game map.

Compile the actual frustum basis statements and render-window ray calls in
the guest translation unit, using its injected declarations. The separate
platform translation unit uses the actual projection hook and uniform
builder. This catches an implicit float/double ABI mismatch as well as
passing unnormalized camera vectors to rays.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def function(source, name):
    match = re.search(r"(?:^|\n)(?:static\s+)?void\s+" + name + r"\([^;]*?\)\s*\{", source)
    if not match:
        raise RuntimeError("Missing production function " + name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


renderer = function((ROOT / "source/render/render.c").read_text(), "render_window")
frustum = function((ROOT / "source/render/render_cameras.c").read_text(), "render_camera_build_frustum")
platform = (ROOT / "port/linux/src/raytrace_gl.c").read_text()

# Keep the boundary statements verbatim: assertions below exercise their
# values, including the direction/sign of the rasterizer's camera axes.
basis_start = frustum.index("cross_product3d(&camera->forward")
basis_end = frustum.index("matrix4x3_inverse(", basis_start)
basis = frustum[basis_start:basis_end]
ray_start = renderer.index("const real_matrix4x3 *ray_basis")
ray_end = renderer.index("profile_render_window_start", ray_start)
ray_basis = renderer[ray_start:ray_end]
calls = []
for name in ("halo_ray_traced_projection", "halo_ray_traced_light_buffer", "halo_ray_traced_lighting"):
    match = re.search(r"\b" + name + r"\([^;]+;", renderer)
    if not match:
        raise RuntimeError("Missing render-window call " + name)
    calls.append(match.group())

header = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float real;
typedef struct { float x,y,z; } real_point3d;
typedef struct { float i,j,k; } real_vector3d;
typedef struct real_matrix4x3 {
    float scale; real_vector3d forward,left,up; real_point3d position;
} real_matrix4x3;
struct render_camera {
    real_point3d position; real_vector3d forward,up;
    float z_near,z_far,vertical_field_of_view;
};
struct render_frustum { real_matrix4x3 view_to_world; float projection_matrix[4][4]; };
static void cross_product3d(const real_vector3d *a,const real_vector3d *b,real_vector3d *v) {
    *v=(real_vector3d){a->j*b->k-a->k*b->j,a->k*b->i-a->i*b->k,a->i*b->j-a->j*b->i};
}
static void negate_vector3d(const real_vector3d *v,real_vector3d *out) { *out=(real_vector3d){-v->i,-v->j,-v->k}; }
static void normalize3d(real_vector3d *v) { float s=sqrtf(v->i*v->i+v->j*v->j+v->k*v->k); v->i/=s;v->j/=s;v->k/=s; }
void inspect_camera(float *uniforms,float *forward,float *up,int *count);
'''

guest = '''#include "camera_harness.h"
#include "halo_linux_source_fixups.h"
static void build_basis(const struct render_camera *camera,struct render_frustum *frustum) {
    real_vector3d x_axis,y_axis,z_axis;
''' + basis + '''
}
static void draw_rays(const struct render_camera *rasterizer_camera,const struct render_frustum *rasterizer_frustum) {
''' + ray_basis + "\n".join(calls) + r'''
}
static void near(float a,float b) { assert(isfinite(a)&&fabsf(a-b)<1e-6f); }
int main(void) {
    /* Unequal lengths and a forward component in up are intentional;
       rasterization removes them before drawing the depth buffer. */
    struct render_camera camera={.position={1,2,3},.forward={0,.92f,0},.up={0,.08f,.87f},
        .z_near=.0625f,.z_far=1024,.vertical_field_of_view=1.7f};
    struct render_frustum frustum={0};
    build_basis(&camera,&frustum);
    /* Deliberately disagree with FOV and physical viewport aspect. */
    frustum.projection_matrix[0][0]=.7f;frustum.projection_matrix[1][1]=1.3f;
    draw_rays(&camera,&frustum);
    float uniforms[16],forward[3],up[3];int count;
    inspect_camera(uniforms,forward,up,&count);
    assert(count==2);near(forward[0],0);near(forward[1],1);near(forward[2],0);
    near(up[0],0);near(up[1],0);near(up[2],1);
    near(uniforms[2],1.0f/1.3f);near(uniforms[3],1.3f/.7f);
    puts("PASS: guest camera hooks preserve float ABI and rasterizer basis/projection");
}
'''

implementation = r'''
#include "camera_harness.h"
#include "raytrace_gl.h"
static struct {
    float projection_x,projection_y,radius,occlusion_strength,reflection_strength,bounce_strength;
    unsigned frame;int hardware;
} ray;
''' + function(platform, "halo_ray_traced_projection") + function(platform, "lighting_uniforms") + r'''
static float saved_uniforms[16],saved_forward[3],saved_up[3];static int calls;
void halo_ray_traced_lighting(float n,float f,float fov,const float *p,const float *forward,const float *up) {
    assert(p[0]==1&&p[1]==2&&p[2]==3);assert(n==.0625f&&f==1024);
    memcpy(saved_forward,forward,sizeof(saved_forward));memcpy(saved_up,up,sizeof(saved_up));
    const int viewport[4]={0,0,1920,1080};
    lighting_uniforms(saved_uniforms,n,f,fov,viewport,1920,1080);calls++;
}
void halo_ray_traced_light_buffer(float n,float f,float fov,const float *p,const float *forward,const float *up) {
    halo_ray_traced_lighting(n,f,fov,p,forward,up);
    assert(fabsf(forward[1]-1)<1e-6f&&fabsf(up[2]-1)<1e-6f);
}
void inspect_camera(float *uniforms,float *forward,float *up,int *count) {
    memcpy(uniforms,saved_uniforms,sizeof(saved_uniforms));memcpy(forward,saved_forward,sizeof(saved_forward));
    memcpy(up,saved_up,sizeof(saved_up));*count=calls;
}
'''

with tempfile.TemporaryDirectory(prefix="halo-ray-camera-") as folder:
    folder = Path(folder)
    (folder / "camera_harness.h").write_text(header)
    (folder / "guest.c").write_text(guest)
    (folder / "platform.c").write_text(implementation)
    subprocess.run(["clang", "-std=c11", "-O1", "-g", "-Werror=implicit-function-declaration",
                    "-fsanitize=undefined,float-divide-by-zero",
                    "-iquote", str(ROOT / "port/linux/include"), "-iquote", str(ROOT / "port/linux/src"),
                    str(folder / "guest.c"), str(folder / "platform.c"), "-o", str(folder / "camera_test")], check=True)
    subprocess.run([str(folder / "camera_test")], check=True, timeout=30)
