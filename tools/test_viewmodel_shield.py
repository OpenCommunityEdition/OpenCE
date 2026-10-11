"""Capture the actual plasma draw constants; no maps or graphics driver needed."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("control", [None, "no-limit", "world-limit", "overwrite-small"])
def test_plasma_first_person_displacement(tmp_path, control):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    text = (ROOT / "source/rasterizer/xbox/rasterizer_xbox_plasma_energy.c").read_text()
    draw = function(text, "rasterizer_plasma_energy_draw")
    if control == "no-limit":
        draw = draw.replace("offset_amount > 0.0075f", "offset_amount > 1.0f")
    elif control == "world-limit":
        draw = draw.replace("group->geometry_flags & (1UL << 7)", "1")
    elif control == "overwrite-small":
        draw = draw.replace("offset_amount > 0.0075f", "1")
    types = "\n".join(structure(text, n) for n in [
        "plasma_runtime_parameters", "rasterizer_transparent_geometry_group_plasma",
        "shader_transparent_plasma_definition"])
    defines = "\n".join(f"#define {s} {i}" for i, s in enumerate(sorted(set(re.findall(r"\bD3D[A-Z0-9_]+\b", draw)))))
    code = r'''
#include <math.h>
#include <string.h>
#include <stddef.h>
typedef float real;
typedef unsigned char byte;
typedef struct {float red,green,blue;} real_rgb_color;
typedef struct {float i,j,k;} real_vector3d;
struct shader {byte data[40];};
struct transparent_geometry_group;
#define FALSE 0
#define TRUE 1
#define match_assert(a,b,c) ((void)0)
#define csmemset memset
''' + types + "\n" + defines + r'''
static int global_d3d_device=1;
static struct {real game_time_sec;} global_frame_parameters={2};
static struct {int plasma_energy_enabled;} rasterizer_debug_options={1};
static real_rgb_color white={1,1,1}, *global_real_rgb_white=&white;
static struct {unsigned texture_modes,combiner_count,alpha_inputs[4],alpha_outputs[4],rgb_inputs[4],rgb_outputs[4],final_combiner_inputs_abcd,final_combiner_inputs_efg;} pixel_shader;
static real captured_offset,captured_intensity;
static int draws;
static void capture(int reg,real *data){if(reg==-81)captured_offset=data[2];if(reg==-84)captured_intensity=data[11];}
#define IDirect3DDevice8_SetVertexShaderConstant(dev,reg,data,count) capture(reg,(real *)(data))
#define IDirect3DDevice8_SetTextureStageState(...) ((void)0)
#define IDirect3DDevice8_SetRenderState(...) ((void)0)
#define rasterizer_set_texture(...) ((void)0)
#define rasterizer_set_vertex_shader_permutation(...) ((void)0)
#define rasterizer_set_pixel_shader(...) ((void)0)
#define rasterizer_transparent_geometry_group_draw__internal(...) (++draws)
#define shader_get_and_verify_type(shader,type) (shader)
#define shield_color_override(colors,chosen) 0
#define shield_color_apply(...) ((void)0)
''' + draw + r'''
#define CHECK(c) do {if(!(c))return __LINE__;}while(0)
static int near(real a,real b){return fabs(a-b)<0.000001;}
int main(void){
    struct {struct shader base;struct shader_transparent_plasma_definition plasma;} tag;
    struct rasterizer_transparent_geometry_group_plasma group;
    real values[4]={0,.5f,0,0};
    struct plasma_runtime_parameters runtime={NULL,values};
    real amounts[]={.03f,.01f,.0075f,.005f,0,-.01f};
    real saved;
    int i;
    memset(&tag,0,sizeof(tag));memset(&group,0,sizeof(group));
    group.shader=&tag.base;group.runtime_parameters=&runtime;
    tag.plasma.intensity_exponent_source=2;tag.plasma.intensity_exponent=1;
    tag.plasma.offset_exponent_source=2;tag.plasma.offset_exponent=2;
    tag.plasma.parallel_alpha=1;
    tag.plasma.primary_noise_map_animation_period=1;
    tag.plasma.secondary_noise_map_animation_period=1;
    for(i=0;i<6;i++){
        tag.plasma.offset_amount=amounts[i];saved=tag.plasma.offset_amount;
        group.geometry_flags=0;rasterizer_plasma_energy_draw(&group);
        CHECK(near(captured_offset,amounts[i]>0?amounts[i]*.25f:0));
        CHECK(near(captured_intensity,.5f));
        group.geometry_flags=128;rasterizer_plasma_energy_draw(&group);
        CHECK(near(captured_offset,i<2?.001875f:amounts[i]>0?amounts[i]*.25f:0));
        CHECK(near(captured_intensity,.5f)&&tag.plasma.offset_amount==saved);
        /* A later world draw of the same tag must retain its original shell. */
        group.geometry_flags=0;rasterizer_plasma_energy_draw(&group);
        CHECK(near(captured_offset,amounts[i]>0?amounts[i]*.25f:0));
    }
    group.geometry_flags=128;tag.plasma.offset_amount=.03f;
    values[1]=1;rasterizer_plasma_energy_draw(&group);CHECK(near(captured_offset,.0075f));
    values[1]=0;rasterizer_plasma_energy_draw(&group);CHECK(captured_offset==0&&captured_intensity==0);
    group.runtime_parameters=NULL;rasterizer_plasma_energy_draw(&group);CHECK(captured_offset==0);
    CHECK(draws==21);
    rasterizer_debug_options.plasma_energy_enabled=0;rasterizer_plasma_energy_draw(&group);CHECK(draws==21);
    return 0;
}
'''
    source = tmp_path / "plasma.c"
    source.write_text(code)
    exe = tmp_path / ("plasma.exe" if sys.platform == "win32" else "plasma")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else ["-lm"]
    result = subprocess.run([compiler, str(source), *flags, "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(exe)], timeout=10)
    assert (result.returncode != 0) == bool(control)
