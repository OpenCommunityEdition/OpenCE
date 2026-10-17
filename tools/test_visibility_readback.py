"""Run the renderer's visibility functions against an unfinished fake GPU query."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("negative_control", [None, "readback", "depth_restore", "offset_restore"])
def test_visibility_readback(tmp_path, negative_control):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    code = source.split("/* ---------- visibility (occlusion) tests */", 1)[1]
    code = code.split("/* ---------- render and texture stage state */", 1)[0]
    depth_code = source.split("static void apply_raster_state(BOOL has_depth)", 1)[1]
    depth_code = depth_code.split("\tstate_enable(&gl_state.stencil_test", 1)[0]
    offset_code = source.split("\t/* D3DRS_ZBIAS is expressed in these states", 1)[1]
    offset_code = offset_code.split("\n#ifdef HALO_GLES\n/* ES has no debug callback", 1)[0]
    depth_code = ("static void apply_raster_state(BOOL has_depth)" + depth_code
                  + "\t/* D3DRS_ZBIAS is expressed in these states" + offset_code)
    diagnostic_fields = source.split("\tBOOL visibility_sync;", 1)[1]
    diagnostic_fields = diagnostic_fields.split("\t/* each test's latest result", 1)[0]
    if negative_control == "readback":
        assert code.count("available = 1;") == 1
        code = code.replace("available = 1;", "available = 0;")
    if negative_control == "depth_restore":
        depth_code = depth_code.replace("device.visibility_test_active && device.visibility_no_depth",
                                        "device.visibility_no_depth")
    if negative_control == "offset_restore":
        code = code.replace("device.visibility_test_active && device.visibility_explicit_depth",
                            "device.visibility_explicit_depth")
    prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned int GLuint, DWORD, UINT;
typedef unsigned long long ULONGLONG;
typedef unsigned long long Uint64;
typedef unsigned int GLenum;
typedef int GLint;
typedef int BOOL, HRESULT;
#define WINAPI
#define TRUE 1
#define FALSE 0
#define S_OK 0
#define VISIBILITY_TEST_SLOTS 8
#define VISIBILITY_QUERY 1
#define GL_QUERY_RESULT_AVAILABLE 2
#define GL_QUERY_RESULT 3
#define GL_NO_ERROR 0
#define GL_DEPTH_TEST 4
#define GL_SCISSOR_TEST 5
#define GL_NEVER 6
#define GL_POLYGON_OFFSET_FILL 7
#define GL_POLYGON_OFFSET_LINE 8
#define GL_TRUE 1
#define GL_FALSE 0
#define D3DRS_COLORWRITEENABLE 0
#define D3DRS_ZENABLE 1
#define D3DRS_STENCILENABLE 2
#define D3DRS_ZFUNC 3
#define D3DRS_CULLMODE 4
#define D3DRS_ZBIAS 5
#define D3DRS_ZWRITEENABLE 6
#define D3DRS_SOLIDOFFSETENABLE 7
#define D3DRS_POLYGONOFFSETZSLOPESCALE 8
#define D3DRS_POLYGONOFFSETZOFFSET 9
#define platform_log(...) ((void)0)
static Uint64 ticks;
static Uint64 SDL_GetTicks(void) { return ticks; }
static GLenum glGetError(void) { return GL_NO_ERROR; }
static struct {
    BOOL gl_ready, visibility_test_active, visibility_sync;
    DIAGNOSTIC_FIELDS
    unsigned long frame;
    struct { unsigned X, Y, Width, Height; float MinZ, MaxZ; } viewport;
    GLuint scratch_query, active_query, queries[8];
    GLuint visibility_known[8];
    BOOL query_pending[8], visibility_unread[8];
    float query_area[8];
    volatile GLuint *visibility_results;
    struct { GLuint queries[8]; unsigned short slots[8]; unsigned long count; } visibility_batches[2];
    unsigned long visibility_batch;
} device;
static float target_scale[2] = {2, 3};
static int target_samples = 4;
static struct { GLuint samples; BOOL available; } gpu[10];
static GLuint active, next_samples;
static unsigned reads, polls;
static DWORD D3D__RenderState[10];
static struct {
    GLint viewport[4], scissor[4]; float depth_range[2];
    unsigned char scissor_test, depth_test, depth_mask;
    GLenum depth_function;
    unsigned char offset_fill, offset_line;
    float polygon_offset[2];
} gl_state;
static BOOL actual_depth, actual_depth_mask;
static BOOL actual_offset_fill, actual_offset_line;
static float actual_slope, actual_units;
static float dword_to_float(DWORD bits) { float value; memcpy(&value, &bits, 4); return value; }
static DWORD float_to_dword(float value) { DWORD bits; memcpy(&bits, &value, 4); return bits; }
static void glPolygonOffset(float slope, float units) { actual_slope = slope; actual_units = units; }
static int target_pixel(float v, int axis) { return (int)(v * target_scale[axis]); }
static void glViewport(int x, int y, int w, int h) {}
static void glScissor(int x, int y, int w, int h) {}
static void glDepthRange(float a, float b) {}
static void glDepthFunc(GLenum function) {}
static void glDepthMask(BOOL enabled) { actual_depth_mask = enabled; }
static void state_enable(unsigned char *cached, GLenum state, BOOL enabled) {
    if (*cached != enabled) {
        *cached = enabled;
        if (state == GL_DEPTH_TEST) actual_depth = enabled;
        if (state == GL_POLYGON_OFFSET_FILL) actual_offset_fill = enabled;
        if (state == GL_POLYGON_OFFSET_LINE) actual_offset_line = enabled;
    }
}
static void glBeginQuery(int target, GLuint query) {
    assert(target == VISIBILITY_QUERY && !active && query);
    active = query;
}
static void glEndQuery(int target) {
    assert(target == VISIBILITY_QUERY && active);
    gpu[active].samples = next_samples;
    gpu[active].available = FALSE;
    active = 0;
}
static void glGetQueryObjectuiv(GLuint query, int pname, GLuint *value) {
    assert(query);
    if (pname == GL_QUERY_RESULT_AVAILABLE) {
        polls++;
        *value = gpu[query].available;
    } else {
        assert(pname == GL_QUERY_RESULT);
        /* A blocking GL read completes the pending GPU work. */
        reads++;
        gpu[query].available = TRUE;
        *value = gpu[query].samples;
    }
}
'''
    main = r'''
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); return 1; } } while (0)
static void submit(unsigned slot, unsigned samples) {
    next_samples = samples;
    D3DDevice_BeginVisibilityTest();
    D3DDevice_EndVisibilityTest(slot);
}
int main(void) {
    UINT value = 99;
    ULONGLONG stamp = 99;
    unsigned i;
    GLuint mapped[8] = {0};
    device.gl_ready = TRUE;
    device.scratch_query = 9;
    for (i = 0; i < 8; i++) device.queries[i] = i + 1;
    D3DDevice_GetVisibilityTestResult(0, &value, &stamp);
    CHECK(value == 0 && stamp == 0 && reads == 0);
    submit(0, 240);
    device.visibility_known[0] = 3;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 3 && reads == 0 && polls == 1);
    gpu[device.queries[0]].available = TRUE;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 10 && reads == 1);
    device.visibility_sync = TRUE;
    submit(0, 480);
    submit(1, 0);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 20 && reads == 2 && polls == 2);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 20 && reads == 2);
    D3DDevice_GetVisibilityTestResult(1, &value, NULL);
    CHECK(value == 0 && reads == 3);
    CHECK(device.visibility_reads == 2 && device.visibility_zeros == 1);
    CHECK(device.visibility_examples[1].raw == 480);
    CHECK(device.visibility_examples[0].slot == 1);
    /* Reporting continues after its first interval, including small counts
       rounded to zero. Cached reads above must not be counted twice. */
    ticks = 1000;
    visibility_log_summary();
    CHECK(device.visibility_reads == 0 && device.visibility_log_time == 1000);
    submit(0, 1);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 0 && device.visibility_rounded_zeros == 1);
    CHECK(device.visibility_zeros == 0 && device.visibility_min == 1);
    target_scale[0] = target_scale[1] = 1;
    target_samples = 1;
    submit(0, 7);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 7 && reads == 5);
    /* The default mapped path continues to read the mapped count. */
    device.visibility_sync = FALSE;
    device.visibility_results = mapped;
    mapped[0] = 13;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 13 && reads == 5);
    /* The actual depth-state application must restore ordinary draws,
       including their write masks, immediately after visibility draws. */
    D3D__RenderState[D3DRS_ZENABLE] = TRUE;
    D3D__RenderState[D3DRS_ZWRITEENABLE] = TRUE;
    apply_raster_state(TRUE);
    CHECK(actual_depth && actual_depth_mask);
    device.visibility_test_active = TRUE;
    apply_raster_state(TRUE);
    CHECK(actual_depth && actual_depth_mask);
    device.visibility_no_depth = TRUE;
    device.visibility_sync = TRUE;
    apply_raster_state(TRUE);
    CHECK(!actual_depth && !actual_depth_mask && !device.visibility_draw.depth);
    device.visibility_test_active = FALSE;
    apply_raster_state(TRUE);
    CHECK(actual_depth && actual_depth_mask);
    apply_raster_state(FALSE);
    CHECK(!actual_depth && !actual_depth_mask);
    /* Explicit bias replaces native bias only within the query; depth and
       ordinary raster state survive the switch, including subsequent bias changes. */
    device.visibility_no_depth = FALSE;
    device.visibility_explicit_depth = TRUE;
    device.visibility_test_active = TRUE;
    D3D__RenderState[D3DRS_ZWRITEENABLE] = FALSE;
    D3D__RenderState[D3DRS_SOLIDOFFSETENABLE] = TRUE;
    D3D__RenderState[D3DRS_ZBIAS] = 8;
    D3D__RenderState[D3DRS_POLYGONOFFSETZSLOPESCALE] = float_to_dword(-2.0f);
    D3D__RenderState[D3DRS_POLYGONOFFSETZOFFSET] = float_to_dword(-8.0f);
    apply_raster_state(TRUE);
    CHECK(actual_depth && !actual_depth_mask);
    CHECK(!actual_offset_fill && !actual_offset_line);
    CHECK(visibility_depth_offset() < -4.7683e-7f && visibility_depth_offset() > -4.7685e-7f);
    CHECK(device.visibility_draw.explicit_offset == visibility_depth_offset());
    D3D__RenderState[D3DRS_ZBIAS] = 16;
    CHECK(visibility_depth_offset() < -9.5366e-7f && visibility_depth_offset() > -9.5370e-7f);
    D3D__RenderState[D3DRS_SOLIDOFFSETENABLE] = FALSE;
    CHECK(visibility_depth_offset() == 0.0f);
    D3D__RenderState[D3DRS_SOLIDOFFSETENABLE] = TRUE;
    device.visibility_test_active = FALSE;
    apply_raster_state(TRUE);
    CHECK(actual_offset_fill && actual_offset_line);
    CHECK(actual_slope == -2.0f && actual_units == -8.0f);
    CHECK(visibility_depth_offset() == 0.0f);
    device.visibility_test_active = TRUE;
    device.visibility_explicit_depth = FALSE;
    apply_raster_state(TRUE);
    CHECK(actual_offset_fill && actual_offset_line && actual_depth);
    CHECK(visibility_depth_offset() == 0.0f);
    return 0;
}
'''
    path = tmp_path / "visibility.c"
    path.write_text(prefix.replace("DIAGNOSTIC_FIELDS", diagnostic_fields) + code + depth_code + main)
    binary = tmp_path / ("visibility.exe" if os.name == "nt" else "visibility")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    build = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                           capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    if negative_control:
        assert run.returncode != 0, "The negative control must fail"
        expected = {"readback": "value == 20", "depth_restore": "actual_depth && actual_depth_mask",
                    "offset_restore": "actual_offset_fill && actual_offset_line"}[negative_control]
        assert expected in run.stderr
    else:
        assert run.returncode == 0, run.stderr
