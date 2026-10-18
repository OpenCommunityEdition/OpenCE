"""Run the renderer's visibility functions against an unfinished fake GPU query."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("negative_control", [None, "readback", "depth_restore", "offset_restore",
                                             "invalid_query", "failed_read", "untyped_pool", "wrong_batch"])
def test_visibility_readback(tmp_path, negative_control):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    code = source.split("/* ---------- visibility (occlusion) tests */", 1)[1]
    code = code.split("/* ---------- render and texture stage state */", 1)[0]
    from harness import function
    copy_code = function(source, "visibility_copy_batch")
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
    if negative_control == "invalid_query":
        code = code.replace("if (device.scratch_query && !visibility_query_alias(device.scratch_query, VISIBILITY_TEST_SLOTS))",
                            "if (TRUE)")
    if negative_control == "failed_read":
        code = code.replace("if (!visibility_query_read(index, &samples))",
                            "if ((visibility_query_read(index, &samples), FALSE))")
    if negative_control == "untyped_pool":
        code = code.replace("glCreateQueries(VISIBILITY_QUERY, (GLsizei)count, queries)",
                            "glGenQueries((GLsizei)count, queries)")
    if negative_control == "wrong_batch":
        copy_code = copy_code.replace("device.visibility_batch ^= 1;", "/* no swap */")
    prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef long GLintptr;
typedef unsigned int GLuint, DWORD, UINT;
typedef unsigned long long ULONGLONG;
typedef unsigned long long Uint64;
typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
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
static GLenum gpu_error;
static GLenum glGetError(void) { GLenum error = gpu_error; gpu_error = GL_NO_ERROR; return error; }
static struct {
    BOOL gl_ready, visibility_test_active, visibility_sync;
    DIAGNOSTIC_FIELDS
    unsigned long frame;
    struct { unsigned X, Y, Width, Height; float MinZ, MaxZ; } viewport;
    GLuint scratch_query, active_query, queries[8];
    GLuint visibility_known[8];
    BOOL query_pending[8], visibility_unread[8];
    float query_area[8];
    GLuint visibility_results_buffer;
    volatile GLuint *visibility_results;
    struct { GLuint queries[8]; unsigned short slots[8]; unsigned long count; } visibility_batches[2];
    unsigned long visibility_batch;
} device;
static float target_scale[2] = {2, 3};
static int target_samples = 4;
static struct { GLuint samples; BOOL available, reserved, object; } gpu[256];
static GLuint active, next_samples;
static unsigned reads, polls;
static unsigned next_query = 1, invalid_begins, invalid_ends, end_calls;
static int zero_at = -1, duplicate_at = -1;
static BOOL fail_generation, fail_begin, fail_end, fail_read, fail_object, fail_create;
static unsigned copies;
static void glGenQueries(GLsizei count, GLuint *queries) {
    int i;
    for (i = 0; i < count; i++) {
        if (fail_generation || i == zero_at) { queries[i] = 0; continue; }
        if (i == duplicate_at && i) { queries[i] = queries[i - 1]; continue; }
        assert(next_query < 256);
        queries[i] = next_query++;
        gpu[queries[i]].reserved = TRUE;
    }
    zero_at = duplicate_at = -1;
}
static void glCreateQueries(GLenum target, GLsizei count, GLuint *queries) {
    int i;
    assert(target == VISIBILITY_QUERY);
    glGenQueries(count, queries);
    for (i = 0; i < count; i++)
        if (queries[i]) gpu[queries[i]].object = !fail_object;
    if (fail_create) gpu_error = 0x505;
}
static void glGetQueryBufferObjectuiv(GLuint query, GLuint buffer, GLenum pname, GLintptr offset) {
    assert(query && gpu[query].object && pname == GL_QUERY_RESULT);
    assert(offset >= 0 && offset < sizeof(GLuint) * VISIBILITY_TEST_SLOTS);
    assert(device.visibility_results && buffer == device.visibility_results_buffer);
    ((GLuint *)device.visibility_results)[offset / sizeof(GLuint)] = gpu[query].samples;
    copies++;
}
static BOOL glIsQuery(GLuint query) { return query < 256 && gpu[query].object; }
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
    assert(target == VISIBILITY_QUERY && !active);
    if (!query || query >= 256 || !gpu[query].reserved) {
        invalid_begins++; gpu_error = 0x502; return;
    }
    if (fail_begin) { gpu_error = 0x502; return; }
    gpu[query].object = TRUE;
    active = query;
}
static void glEndQuery(int target) {
    assert(target == VISIBILITY_QUERY);
    end_calls++;
    if (!active) { invalid_ends++; gpu_error = 0x502; return; }
    gpu[active].samples = next_samples;
    gpu[active].available = FALSE;
    active = 0;
    if (fail_end) gpu_error = 0x502;
}
static void glGetQueryObjectuiv(GLuint query, int pname, GLuint *value) {
    assert(query);
    if (pname == GL_QUERY_RESULT_AVAILABLE) {
        polls++;
        *value = gpu[query].available;
    } else {
        assert(pname == GL_QUERY_RESULT);
        if (fail_read) { gpu_error = 0x502; return; }
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
    unsigned old_ends, old_reads;
    GLuint mapped[8] = {0};
    device.gl_ready = TRUE;
    CHECK(visibility_queries_initialize());
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
    /* Inject corruption after a valid startup to exercise the optional
       synchronized diagnostic's last-valid-result recovery. */
    memset(&device, 0, sizeof(device));
    device.gl_ready = device.visibility_sync = TRUE;
    CHECK(visibility_queries_initialize());
    CHECK(glIsQuery(device.scratch_query));
    device.queries[0] = 0;
    submit(0, 7);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 7 && device.scratch_query == 0);
    /* A zero recycled from slot 0 is replaced before the next BeginQuery. */
    submit(0, 11);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 11 && device.visibility_replacements == 1 && invalid_begins == 0);
    /* A scratch name that aliases a slot must not overwrite that slot's GPU result. */
    device.scratch_query = device.queries[0];
    submit(1, 13);
    D3DDevice_GetVisibilityTestResult(1, &value, NULL);
    CHECK(value == 13 && device.visibility_replacements == 2);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 11);
    /* If a replacement cannot be generated, do not begin/end/read the
       invalid query or overwrite the last valid result. Later draws recover. */
    device.scratch_query = 0;
    fail_generation = TRUE;
    old_ends = end_calls;
    old_reads = reads;
    submit(0, 99);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 11 && reads == old_reads && end_calls == old_ends);
    CHECK(device.visibility_failed_tests == 1 && !device.visibility_test_active);
    fail_generation = FALSE;
    submit(0, 17);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 17);
    fail_begin = TRUE;
    old_ends = end_calls;
    submit(0, 99);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 17 && end_calls == old_ends && device.visibility_failed_tests == 2);
    fail_begin = FALSE;
    fail_end = TRUE;
    submit(0, 99);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 17 && device.visibility_failed_tests == 3);
    fail_end = FALSE;
    submit(0, 19);
    fail_read = TRUE;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 17 && device.visibility_failed_reads == 1);
    CHECK(!device.visibility_unread[0]);
    fail_read = FALSE;
    submit(0, 19);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 19);
    /* Invalid read objects (zero or a reserved but never-used name) are
       rejected before retrieval. A genuine successful zero still replaces 19. */
    device.queries[0] = 0;
    device.visibility_unread[0] = TRUE;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 19 && device.visibility_failed_reads == 2);
    glGenQueries(1, &device.queries[0]);
    device.visibility_unread[0] = TRUE;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 19 && device.visibility_failed_reads == 3);
    submit(0, 0);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 0 && invalid_begins == 0 && invalid_ends == 0);
    /* The normal asynchronous path must use separate objects in the two
       frame batches, copy the previous frame, and never read/poll on CPU. */
    memset(&device, 0, sizeof(device));
    memset(gpu, 0, sizeof(gpu));
    next_query = 1;
    device.gl_ready = TRUE;
    CHECK(visibility_queries_initialize());
    device.visibility_results = mapped;
    memset(mapped, 0, sizeof(mapped));
    old_reads = reads;
    old_ends = polls;
    submit(0, 120);
    submit(1, 0);
    visibility_copy_batch();
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 0 && copies == 0);
    submit(0, 240);
    visibility_copy_batch();
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 120 && copies == 2);
    visibility_copy_batch();
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 240 && copies == 3);
    submit(0, 0);
    visibility_copy_batch();
    visibility_copy_batch();
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 0 && copies == 4);
    /* Overflow uses scratch only; no out-of-bounds batch access or copy. */
    {
        unsigned i;
        for (i = 0; i < VISIBILITY_TEST_SLOTS + 2; i++) submit(i % VISIBILITY_TEST_SLOTS, i + 1);
    }
    visibility_copy_batch();
    visibility_copy_batch();
    CHECK(copies == VISIBILITY_TEST_SLOTS + 4);
    CHECK(reads == old_reads && polls == old_ends && !invalid_begins && !invalid_ends);
    /* Refuse zero, duplicate (including across batches), uninstantiated
       objects and GL errors before publishing any IDs to the renderer. */
    memset(&device, 0, sizeof(device));
    zero_at = 0;
    CHECK(!visibility_queries_initialize() && !device.scratch_query && !device.queries[0]);
    duplicate_at = VISIBILITY_TEST_SLOTS + 1;
    CHECK(!visibility_queries_initialize() && !device.visibility_batches[0].queries[0]);
    duplicate_at = VISIBILITY_TEST_SLOTS * 2 + 1;
    CHECK(!visibility_queries_initialize() && !device.visibility_batches[1].queries[0]);
    fail_object = TRUE;
    CHECK(!visibility_queries_initialize() && !device.scratch_query);
    fail_object = FALSE;
    fail_create = TRUE;
    CHECK(!visibility_queries_initialize() && !device.scratch_query);
    fail_create = FALSE;
    return 0;
}
'''
    path = tmp_path / "visibility.c"
    path.write_text(prefix.replace("DIAGNOSTIC_FIELDS", diagnostic_fields) + code + copy_code + depth_code + main)
    binary = tmp_path / ("visibility.exe" if os.name == "nt" else "visibility")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    build = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                           capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    if negative_control:
        assert run.returncode != 0, "The negative control must fail"
        expected = {"readback": "value == 20", "depth_restore": "actual_depth && actual_depth_mask",
                    "offset_restore": "actual_offset_fill && actual_offset_line",
                    "invalid_query": "value == 11 && device.visibility_replacements",
                    "failed_read": "value == 17 && device.visibility_failed_reads",
                    "untyped_pool": "visibility_queries_initialize()",
                    "wrong_batch": "value == 0 && copies == 0"}[negative_control]
        assert expected in run.stderr
    else:
        assert run.returncode == 0, run.stderr
