"""A skipped shadow pass must not expose an uninitialized volume to rendering."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import enum_with, function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def shadow_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if compiler is None:
        pytest.skip("clang is required")
    directory = tmp_path_factory.mktemp("shadow-setup")
    source = (ROOT / "source/rasterizer/xbox/rasterizer_xbox_shadows.c").read_text()
    (directory / "under_test.inc").write_text(
        enum_with(source, "_rasterizer_target_render_primary =") + "\n" +
        function(source, "_rasterizer_environment_shadow_begin"))
    (directory / "shadow.c").write_text(r'''
#include <assert.h>
#include <string.h>
typedef int boolean;
typedef float real;
typedef struct { real i,j,k; } real_vector3d;
typedef struct { real x,y,z; } real_point3d;
typedef struct { real scale; real_vector3d forward,left,up; real_point3d position; } real_matrix4x3;
typedef struct { real red,green,blue; } real_rgb_color;
#define TRUE 1
#define FALSE 0
#define match_assert(file,line,condition) assert(condition)
#define csmemset memset
enum { D3DRS_CULLMODE, D3DCULL_CCW, D3DRS_COLORWRITEENABLE,
       D3DCOLORWRITEENABLE_RED, D3DCOLORWRITEENABLE_GREEN, D3DCOLORWRITEENABLE_BLUE,
       D3DRS_ALPHABLENDENABLE, D3DRS_ALPHATESTENABLE, D3DRS_ALPHAREF,
       D3DRS_ZENABLE, D3DRS_ZBIAS, _rasterizer_statistics_mode_enabled };
static int global_d3d_device=1, device_calls;
static struct { int rasterizer_target; } global_window_parameters;
static struct { int draw_environment_shadows, shadow_debug_enabled, statistics_mode; } rasterizer_debug_options;
static struct { int shadow_count; } rasterizer_frame_statistics;
static struct { int texture_modes,combiner_count,final_combiner_inputs_abcd,final_combiner_inputs_efg; } pixel_shader;
static real_matrix4x3 local_shadow_matrix;
static real_rgb_color local_shadow_color;
static real local_object_bounding_radius;
static int local_parameters, shadow_setup, shadow_used, shadow_restored;
#define IDirect3DDevice8_SetRenderState(...) (++device_calls)
#define IDirect3DDevice8_SetVertexShaderConstant(...) (++device_calls)
#define rasterizer_set_pixel_shader(...) (++device_calls)
#define rasterizer_set_target(...) (++device_calls)
#define rasterizer_set_stencil_mode(...) (++device_calls)
static real dot_product3d(real_vector3d const *a, real_vector3d const *b)
{ return a->i*b->i+a->j*b->j+a->k*b->k; }
#include "under_test.inc"

static void skipped(int target, int enabled)
{
    real output=-123.0f;
    global_window_parameters.rasterizer_target=target;
    rasterizer_debug_options.draw_environment_shadows=enabled;
    device_calls=0;
    local_object_bounding_radius=-456.0f;
    local_parameters=shadow_setup=shadow_used=shadow_restored=7;
    rasterizer_frame_statistics.shadow_count=0;
    /* The caller renders a shadow volume only after a successful begin.
     * Ineligible passes must leave both its output and rasterizer state alone. */
    assert(!_rasterizer_environment_shadow_begin(0,NULL,NULL,6.5f,&output));
    assert(output==-123.0f && local_object_bounding_radius==-456.0f);
    assert(local_parameters==7 && shadow_setup==7 && shadow_used==7 && shadow_restored==7);
    assert(!device_calls && !rasterizer_frame_statistics.shadow_count);
}

static void enabled(int output_present)
{
    real_matrix4x3 matrix={1,{1,0,0},{0,1,0},{0,0,1},{2,3,4}};
    real_rgb_color color={0.2f,0.3f,0.4f};
    real output=-123.0f;
    global_window_parameters.rasterizer_target=0;
    rasterizer_debug_options.draw_environment_shadows=1;
    rasterizer_debug_options.statistics_mode=_rasterizer_statistics_mode_enabled;
    device_calls=0;
    rasterizer_frame_statistics.shadow_count=0;
    assert(_rasterizer_environment_shadow_begin(0,&matrix,&color,6.5f,output_present?&output:NULL));
    assert(output==(output_present?6.5f:-123.0f));
    assert(local_object_bounding_radius==6.5f);
    assert(!memcmp(&matrix,&local_shadow_matrix,sizeof(matrix)));
    assert(!memcmp(&color,&local_shadow_color,sizeof(color)));
    assert(!local_parameters && !shadow_setup && !shadow_used && !shadow_restored);
    assert(device_calls && rasterizer_frame_statistics.shadow_count==1);
}

int main(int argc,char **argv)
{
    assert(argc==2);
    if (!strcmp(argv[1],"secondary")) skipped(1,1);
    else if (!strcmp(argv[1],"disabled")) skipped(0,0);
    else if (!strcmp(argv[1],"enabled")) { enabled(1); enabled(0); }
    else if (!strcmp(argv[1],"transitions")) {
        enabled(1); skipped(1,1); enabled(1); skipped(0,0); enabled(1);
    } else return 2;
    return 0;
}
''')
    binary = directory / ("shadow.exe" if os.name == "nt" else "shadow")
    command = [compiler, "-m32", "-std=gnu89", "-O1", "-Wall", "-Werror",
               "-Wno-unused-but-set-variable", str(directory / "shadow.c"), "-o", str(binary)]
    if os.name == "nt":
        command.append("-fuse-ld=lld")
    subprocess.run(command, check=True, capture_output=True, text=True)
    return binary


@pytest.mark.parametrize("case", ["secondary", "disabled", "enabled", "transitions"])
def test_shadow_setup_contract(shadow_tool, case):
    subprocess.run([str(shadow_tool), case], check=True, capture_output=True, timeout=10)
