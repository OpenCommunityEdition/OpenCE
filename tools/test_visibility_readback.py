"""Exercise the desktop renderer's real query allocation and asynchronous readback."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

from harness import function, mutated

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("negative_control", [None, "untyped_pool", "wrong_batch",
                                             "invalid_pool", "uninitialized_buffer"])
def test_visibility_readback(tmp_path, negative_control):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    code = source.split("/* ---------- visibility (occlusion) tests */", 1)[1]
    code = code.split("/* ---------- render and texture stage state */", 1)[0]
    copy_code = function(source, "visibility_copy_batch")
    buffer_code = source.split("\tglGenBuffers(1, &device.visibility_results_buffer);", 1)[1]
    buffer_code = buffer_code.split("\tglBindBuffer(GL_QUERY_BUFFER, 0);", 1)[0]
    buffer_code = ("static void initialize_results(void) {\n"
                   "glGenBuffers(1, &device.visibility_results_buffer);\n" + buffer_code
                   + "glBindBuffer(GL_QUERY_BUFFER, 0);\n}\n")
    if negative_control == "untyped_pool":
        code = mutated(code, "glCreateQueries(VISIBILITY_QUERY, (GLsizei)count, queries)",
                       "glGenQueries((GLsizei)count, queries)")
    if negative_control == "wrong_batch":
        copy_code = mutated(copy_code, "device.visibility_batch ^= 1;", "/* no swap */")
    if negative_control == "invalid_pool":
        code = mutated(code, "if (!success || zeros || duplicates || invalid)", "if (FALSE)")
    if negative_control == "uninitialized_buffer":
        buffer_code = mutated(buffer_code, "sizeof(device.visibility_known), device.visibility_known,",
                              "sizeof(device.visibility_known), NULL,")
    prefix = r"""
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned int GLuint, DWORD, UINT, GLenum;
typedef unsigned long long ULONGLONG;
typedef int BOOL, HRESULT, GLsizei;
typedef long GLintptr;
#define WINAPI
#define TRUE 1
#define FALSE 0
#define S_OK 0
#define VISIBILITY_TEST_SLOTS 8
#define VISIBILITY_QUERY 1
#define GL_QUERY_RESULT_AVAILABLE 2
#define GL_QUERY_RESULT 3
#define GL_NO_ERROR 0
#define GL_QUERY_BUFFER 4
#define GL_MAP_READ_BIT 1
#define GL_MAP_PERSISTENT_BIT 2
#define GL_MAP_COHERENT_BIT 4
#define platform_log(...) ((void)0)
static struct {
    BOOL gl_ready, visibility_test_active;
    GLuint queries[8], scratch_query, active_query;
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
static GLuint active, next_samples, next_query = 1, mapped[8];
static unsigned reads, polls, copies;
static int zero_at = -1, duplicate_at = -1;
static BOOL fail_object, fail_create, fail_all;
static GLenum gpu_error;
static GLenum glGetError(void) { GLenum error = gpu_error; gpu_error = 0; return error; }
static void glGenQueries(GLsizei count, GLuint *queries) {
    int i;
    for (i = 0; i < count; i++) {
        if (fail_all || i == zero_at) { queries[i] = 0; continue; }
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
static BOOL glIsQuery(GLuint query) { return query < 256 && gpu[query].object; }
static void glBeginQuery(GLenum target, GLuint query) {
    assert(target == VISIBILITY_QUERY && !active && query && query < 256);
    assert(gpu[query].reserved && gpu[query].object);
    active = query;
}
static void glEndQuery(GLenum target) {
    assert(target == VISIBILITY_QUERY && active);
    gpu[active].samples = next_samples;
    gpu[active].available = FALSE;
    active = 0;
}
static void glGetQueryObjectuiv(GLuint query, GLenum pname, GLuint *value) {
    assert(query && gpu[query].object);
    if (pname == GL_QUERY_RESULT_AVAILABLE) {
        polls++;
        *value = gpu[query].available;
    } else {
        assert(pname == GL_QUERY_RESULT && gpu[query].available);
        reads++;
        *value = gpu[query].samples;
    }
}
static void glGetQueryBufferObjectuiv(GLuint query, GLuint buffer, GLenum pname, GLintptr offset) {
    assert(query && gpu[query].object && pname == GL_QUERY_RESULT);
    assert(offset >= 0 && offset < sizeof(mapped));
    assert(buffer == device.visibility_results_buffer && device.visibility_results);
    /* Emulate completion of the queued GPU copy, without a CPU query read. */
    mapped[offset / sizeof(GLuint)] = gpu[query].samples;
    copies++;
}
static void glGenBuffers(GLsizei count, GLuint *buffers) { assert(count == 1); *buffers = 1; }
static void glBindBuffer(GLenum target, GLuint buffer) { assert(target == GL_QUERY_BUFFER); }
static void glBufferStorage(GLenum target, size_t size, const void *data, unsigned flags) {
    assert(target == GL_QUERY_BUFFER && size == sizeof(mapped) && flags == 7);
    if (data) memcpy(mapped, data, size);
    else memset(mapped, 0xcc, size); /* Undefined storage must not reach the first read. */
}
static void *glMapBufferRange(GLenum target, GLintptr offset, size_t size, unsigned flags) {
    assert(target == GL_QUERY_BUFFER && !offset && size == sizeof(mapped) && flags == 7);
    return mapped;
}
"""
    main = r"""
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); return 1; } } while (0)
static void submit(unsigned slot, unsigned samples) {
    next_samples = samples;
    D3DDevice_BeginVisibilityTest();
    D3DDevice_EndVisibilityTest(slot);
}
int main(void) {
    UINT value = 99;
    ULONGLONG stamp = 99;
    unsigned i, before;
    D3DDevice_GetVisibilityTestResult(0, &value, &stamp);
    CHECK(value == 0 && stamp == 0 && !reads && !polls);
    CHECK(visibility_queries_initialize());
    initialize_results();
    device.gl_ready = TRUE;
    /* Neither this frame's data nor the other batch's newer result may
       replace the previous-frame result before that batch is copied. */
    submit(0, 240);
    submit(1, 0);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 0); /* initialized mapped storage */
    visibility_copy_batch();
    CHECK(copies == 0);
    submit(0, 480);
    visibility_copy_batch();
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 10 && copies == 2);
    visibility_copy_batch();
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 20 && copies == 3);
    /* Repeated slots in one frame: the last result wins. Recycle both batches. */
    for (i = 0; i < 20; i++) {
        submit(0, 240);
        submit(0, 0);
        visibility_copy_batch();
        visibility_copy_batch();
        D3DDevice_GetVisibilityTestResult(0, &value, NULL);
        CHECK(value == 0);
    }
    before = copies;
    for (i = 0; i < VISIBILITY_TEST_SLOTS + 2; i++) submit(i % VISIBILITY_TEST_SLOTS, i + 1);
    visibility_copy_batch();
    visibility_copy_batch();
    CHECK(copies == before + VISIBILITY_TEST_SLOTS && !reads && !polls);
    /* Mapping failure retains the nonblocking availability-polled fallback. */
    device.visibility_results = NULL;
    submit(0, 240);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 0 && reads == 0 && polls == 1);
    gpu[device.queries[0]].available = TRUE;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 10 && reads == 1 && polls == 2);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 10 && reads == 1 && polls == 2);
    submit(0, 0);
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 10 && reads == 1 && polls == 3);
    gpu[device.queries[0]].available = TRUE;
    D3DDevice_GetVisibilityTestResult(0, &value, NULL);
    CHECK(value == 0 && reads == 2 && polls == 4);
    /* No partially valid pool may be published, even if only a batch is bad. */
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
    fail_all = TRUE;
    CHECK(!visibility_queries_initialize() && !device.scratch_query);
    return 0;
}
"""
    path = tmp_path / "visibility.c"
    path.write_text(prefix + code + copy_code + buffer_code + main)
    binary = tmp_path / ("visibility.exe" if os.name == "nt" else "visibility")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
    build = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                           capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    if negative_control:
        expected = {"untyped_pool": "visibility_queries_initialize()", "wrong_batch": "copies == 0",
                    "invalid_pool": "!visibility_queries_initialize()",
                    "uninitialized_buffer": "value == 0"}[negative_control]
        assert run.returncode != 0 and expected in run.stderr, run.stderr
    else:
        assert run.returncode == 0, run.stderr
