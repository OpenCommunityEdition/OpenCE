"""Run the renderer's visibility functions against an unfinished fake GPU query."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("negative_control", [False, True])
def test_visibility_readback(tmp_path, negative_control):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    code = source.split("/* ---------- visibility (occlusion) tests */", 1)[1]
    code = code.split("/* ---------- render and texture stage state */", 1)[0]
    if negative_control:
        assert code.count("available = 1;") == 1
        code = code.replace("available = 1;", "available = 0;")
    prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned int GLuint, DWORD, UINT;
typedef unsigned long long ULONGLONG;
typedef int BOOL, HRESULT;
#define WINAPI
#define TRUE 1
#define FALSE 0
#define S_OK 0
#define VISIBILITY_TEST_SLOTS 8
#define VISIBILITY_QUERY 1
#define GL_QUERY_RESULT_AVAILABLE 2
#define GL_QUERY_RESULT 3
#define platform_log(...) ((void)0)
static struct {
    BOOL gl_ready, visibility_test_active, visibility_sync;
    unsigned long visibility_sync_reads;
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
    target_scale[0] = target_scale[1] = 1;
    target_samples = 1;
    submit(0, 7);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 7 && reads == 4);
    /* The default mapped path continues to read the mapped count. */
    device.visibility_sync = FALSE;
    device.visibility_results = mapped;
    mapped[0] = 13;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 13 && reads == 4);
    return 0;
}
'''
    path = tmp_path / "visibility.c"
    path.write_text(prefix + code + main)
    binary = tmp_path / ("visibility.exe" if os.name == "nt" else "visibility")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    build = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                           capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    if negative_control:
        assert run.returncode != 0, "Removing the synchronous read must fail"
        assert "value == 20" in run.stderr
    else:
        assert run.returncode == 0, run.stderr
