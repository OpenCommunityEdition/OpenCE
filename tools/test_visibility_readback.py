"""Exercise actual CPU and buffered visibility readback with a delayed GPU."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

from harness import function, mutated, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("negative_control", [None, "untyped_pool", "invalid_pool",
                                             "early_read", "wrong_scale", "wrong_query",
                                             "reuse_pending", "no_flush", "no_unbind",
                                             "ignore_error", "wrong_order", "buffered",
                                             "early_buffer", "wrong_offset", "blocking_wait",
                                             "early_fence", "unchecked_copy", "unchecked_wait"])
def test_visibility_readback(tmp_path, negative_control):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/d3d8_gl.c").read_text(encoding="utf-8")
    code = source.split("/* ---------- visibility (occlusion) tests */", 1)[1]
    code = code.split("/* ---------- render and texture stage state */", 1)[0]
    read_code = function(source, "visibility_read_batch")
    batch_type = structure(source, "visibility_batch")
    for forbidden in ("glFinish", "glWaitSync"):
        assert forbidden not in code + read_code
    buffered = negative_control in ("buffered", "early_buffer", "wrong_offset", "blocking_wait",
                                    "early_fence", "unchecked_copy", "unchecked_wait")
    if negative_control == "early_buffer":
        read_code = mutated(read_code, "GLenum status = glClientWaitSync(batch->fence, 0, 0);",
                            "GLenum status = GL_ALREADY_SIGNALED;")
    if negative_control == "wrong_offset":
        read_code = mutated(read_code, "device.visibility_batch * VISIBILITY_TEST_SLOTS + i", "i")
    if negative_control == "blocking_wait":
        read_code = mutated(read_code, "glClientWaitSync(batch->fence, 0, 0)", "glClientWaitSync(batch->fence, 0, 1)")
    if negative_control == "early_fence":
        statement = "batch->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);"
        read_code = mutated(read_code, statement, "((void)0);")
        read_code = mutated(read_code, "for (i = 0; i < batch->count; i++)",
                            statement + "\nfor (i = 0; i < batch->count; i++)")
    if negative_control == "unchecked_copy":
        read_code = mutated(read_code, "if (!success || !batch->fence)", "if (!batch->fence)")
    if negative_control == "unchecked_wait":
        read_code = mutated(read_code, "if (!success || (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED))",
                            "if (FALSE)")
    if negative_control == "untyped_pool":
        code = mutated(code, "glCreateQueries(VISIBILITY_QUERY, (GLsizei)count, queries)",
                       "glGenQueries((GLsizei)count, queries)")
    if negative_control == "invalid_pool":
        code = mutated(code, "if (!success || zeros || duplicates || invalid)", "if (FALSE)")
    if negative_control == "early_read":
        read_code = mutated(read_code, "if (!available)", "if (FALSE)")
    if negative_control == "wrong_scale":
        read_code = mutated(read_code, "float area = batch->areas[test];",
                            "float area = device.query_area[batch->slots[test]];")
    if negative_control == "wrong_query":
        read_code = mutated(read_code, "glGetQueryObjectuiv(batch->queries[test], GL_QUERY_RESULT, &samples);",
                            "glGetQueryObjectuiv(batch->queries[(test + 1) % VISIBILITY_TEST_SLOTS], GL_QUERY_RESULT, &samples);")
    if negative_control == "reuse_pending":
        read_code = mutated(read_code, "&& !batch->sealed && !batch->failed)", ")")
        code = mutated(code, "if (!batch->sealed && !batch->failed && batch->count < VISIBILITY_TEST_SLOTS)",
                       "if ((batch->count = 0) == 0)")
    if negative_control == "no_flush":
        read_code = mutated(read_code, "glFlush();", "((void)0);")
    if negative_control == "no_unbind":
        read_code = mutated(read_code, "glBindBuffer(GL_QUERY_BUFFER, 0);", "((void)0);")
    if negative_control == "ignore_error":
        read_code = mutated(read_code, 'if (!visibility_readback_gl_ok("result"))', "if (FALSE)")
    if negative_control == "wrong_order":
        read_code = mutated(read_code, "batch->serial < device.visibility_batches[oldest].serial",
                            "batch->serial > device.visibility_batches[oldest].serial")
    prefix = r"""
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef unsigned int GLuint, DWORD, UINT, GLenum;
typedef unsigned int GLbitfield;
typedef intptr_t GLintptr, GLsizeiptr;
typedef struct fake_fence { int complete; unsigned copies; } *GLsync;
typedef unsigned long long ULONGLONG;
typedef int BOOL, HRESULT, GLsizei, GLint;
#define WINAPI
#define TRUE 1
#define FALSE 0
#define S_OK 0
#define VISIBILITY_TEST_SLOTS 8
#define VISIBILITY_BATCH_COUNT 3
#define VISIBILITY_ALL_SAMPLES 0x7fffffff
#define VISIBILITY_QUERY 1
#define GL_QUERY_RESULT_AVAILABLE 2
#define GL_QUERY_RESULT 3
#define GL_NO_ERROR 0
#define GL_QUERY_BUFFER 4
#define GL_QUERY_BUFFER_BINDING 5
#define GL_VENDOR 6
#define GL_MAP_READ_BIT 1
#define GL_MAP_PERSISTENT_BIT 2
#define GL_MAP_COHERENT_BIT 4
#define GL_SYNC_GPU_COMMANDS_COMPLETE 7
#define GL_TIMEOUT_EXPIRED 8
#define GL_ALREADY_SIGNALED 9
#define GL_CONDITION_SATISFIED 10
#define GL_WAIT_FAILED 11
#define platform_log(...) ((void)0)
static BOOL config_boolean(const char *key) { return FALSE; }
/* BATCH_TYPE */
static struct {
    BOOL gl_ready, visibility_test_active;
    GLuint active_query, visibility_known[8];
    BOOL query_pending[8];
    float query_area[8];
    struct visibility_batch visibility_batches[VISIBILITY_BATCH_COUNT];
    unsigned long visibility_batch, frame;
    ULONGLONG visibility_serial;
    BOOL visibility_logging, visibility_unavailable;
    GLuint visibility_buffer;
    volatile GLuint *visibility_mapped;
    struct { unsigned long submitted, completed, samples, nonzero, maximum,
                           unavailable, dropped, failures; } visibility_stats;
} device;
static float target_scale[2] = {2, 3};
static int target_samples = 4;
static struct {
    GLuint samples;
    BOOL available, object, pending, flushed, checked;
} gpu[1024];
static GLuint active, next_samples, next_query = 1, query_buffer;
static unsigned reads, polls, flushes;
static BOOL direct_error, availability_error, binding_error, unbind_error;
static int zero_at = -1, duplicate_at = -1;
static BOOL fail_object, fail_create, fail_all;
static GLenum gpu_error;
static BOOL visibility_readback_gl_ok(const char *phase);
static GLenum glGetError(void) { GLenum error = gpu_error; gpu_error = 0; return error; }
static void glGenQueries(GLsizei count, GLuint *queries) {
    int i;
    for (i = 0; i < count; i++) {
        if (fail_all || i == zero_at) { queries[i] = 0; continue; }
        if (i == duplicate_at && i) { queries[i] = queries[i - 1]; continue; }
        assert(next_query < 1024);
        queries[i] = next_query++;
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
static BOOL glIsQuery(GLuint query) { return query < 1024 && gpu[query].object; }
static void glBeginQuery(GLenum target, GLuint query) {
    assert(target == VISIBILITY_QUERY && !active && query && query < 1024);
    assert(gpu[query].object && !gpu[query].pending);
    gpu[query].available = gpu[query].flushed = gpu[query].checked = FALSE;
    active = query;
}
static void glEndQuery(GLenum target) {
    assert(target == VISIBILITY_QUERY && active);
    gpu[active].samples = next_samples;
    gpu[active].pending = TRUE;
    active = 0;
}
static void glGetQueryObjectuiv(GLuint query, GLenum pname, GLuint *value) {
    assert(query && gpu[query].object && gpu[query].pending && !query_buffer);
    if (pname == GL_QUERY_RESULT_AVAILABLE) {
        polls++;
        *value = gpu[query].available;
        gpu[query].checked = *value;
        if (availability_error) gpu_error = 0x502;
    } else {
        assert(pname == GL_QUERY_RESULT && gpu[query].available && gpu[query].checked);
        reads++;
        if (direct_error) { gpu_error = 0x502; return; }
        *value = gpu[query].samples;
        gpu[query].pending = FALSE;
    }
}
static void glFlush(void) {
    unsigned i;
    flushes++;
    for (i = 1; i < next_query; i++)
        if (gpu[i].pending) gpu[i].flushed = TRUE;
}
static void ready(unsigned batch, unsigned test) {
    GLuint query = device.visibility_batches[batch].queries[test];
    assert(gpu[query].pending && gpu[query].flushed);
    gpu[query].available = TRUE;
}
static void finish_gpu(void) {
    unsigned i;
    for (i = 1; i < next_query; i++) if (gpu[i].pending) {
        assert(gpu[i].flushed);
        gpu[i].available = TRUE;
    }
}
static void glBindBuffer(GLenum target, GLuint buffer) {
    assert(target == GL_QUERY_BUFFER);
    if (!buffer && unbind_error) { gpu_error = 0x502; return; }
    query_buffer = buffer;
}
static void glGetIntegerv(GLenum pname, GLint *value) {
    assert(pname == GL_QUERY_BUFFER_BINDING);
    if (binding_error) { gpu_error = 0x502; return; }
    *value = query_buffer;
}
static const char *vendor = "ATI Technologies Inc.";
static GLuint mapped[VISIBILITY_BATCH_COUNT * VISIBILITY_TEST_SLOTS];
static GLintptr offsets[1024];
static unsigned copy_serial[1024];
static unsigned buffer_calls, fence_count, deletes, copies;
static struct fake_fence fences[128];
static BOOL map_null, map_error, copy_error, fence_null, wait_error;
static const char *glGetString(GLenum name) { assert(name == GL_VENDOR); return vendor; }
static void glGenBuffers(GLsizei n, GLuint *buffer) {
    assert(n == 1 && strcmp(vendor, "ATI Technologies Inc."));
    buffer_calls++; *buffer = 123;
}
static void glBufferStorage(GLenum target, GLsizeiptr size, void *data, GLbitfield flags) {
    assert(target == GL_QUERY_BUFFER && query_buffer == 123 && size == sizeof(mapped));
    assert(!data && flags == 7); buffer_calls++;
}
static void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr size, GLbitfield flags) {
    assert(target == GL_QUERY_BUFFER && !offset && size == sizeof(mapped) && flags == 7);
    buffer_calls++; if (map_error) gpu_error = 0x502;
    return map_null ? NULL : mapped;
}
static void glDeleteBuffers(GLsizei n, GLuint *buffer) { assert(n == 1); buffer_calls++; }
static void glGetQueryBufferObjectuiv(GLuint query, GLuint buffer, GLenum pname, GLintptr offset) {
    assert(buffer == 123 && pname == GL_QUERY_RESULT && gpu[query].pending);
    assert(offset >= 0 && offset < sizeof(mapped) && !(offset % sizeof(GLuint)));
    offsets[query] = offset; copy_serial[query] = ++copies;
    if (copy_error) gpu_error = 0x502;
}
static GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    assert(condition == GL_SYNC_GPU_COMMANDS_COMPLETE && !flags && fence_count < 128);
    if (fence_null) return NULL;
    fences[fence_count].copies = copies;
    return &fences[fence_count++];
}
static GLenum glClientWaitSync(GLsync fence, GLbitfield flags, ULONGLONG timeout) {
    assert(fence && !flags && !timeout); /* Never flush or wait in the CPU poll. */
    return wait_error ? GL_WAIT_FAILED : !fence->complete ? GL_TIMEOUT_EXPIRED :
        (fence_count & 1) ? GL_ALREADY_SIGNALED : GL_CONDITION_SATISFIED;
}
static void glDeleteSync(GLsync fence) { assert(fence); deletes++; }
static void complete_buffer(unsigned b) {
    struct visibility_batch *batch = &device.visibility_batches[b];
    unsigned i;
    assert(batch->sealed && !batch->failed && batch->fence);
    for (i = 0; i < batch->count; i++) {
        GLuint q = batch->queries[i];
        assert(gpu[q].pending && gpu[q].flushed);
        assert(copy_serial[q] && copy_serial[q] <= batch->fence->copies);
        mapped[offsets[q] / sizeof(GLuint)] = gpu[q].samples;
        gpu[q].pending = FALSE;
    }
    batch->fence->complete = TRUE;
}
"""
    main = r"""
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); return 1; } } while (0)
static void submit(unsigned slot, unsigned samples) {
    next_samples = samples;
    D3DDevice_BeginVisibilityTest();
    D3DDevice_EndVisibilityTest(slot);
}
static void present(void) { visibility_read_batch(); device.frame++; }
static UINT result(unsigned slot) {
    UINT value = 999;
    unsigned before = reads + polls;
    D3DDevice_GetVisibilityTestResult(slot, &value, NULL);
    assert(before == reads + polls); /* Game retrieval never polls or waits. */
    return value;
}
int main(void) {
    UINT value = 99;
    ULONGLONG stamp = 99;
    unsigned i, before, batch, partial;
    D3DDevice_GetVisibilityTestResult(0, &value, &stamp);
    CHECK(value == 0 && stamp == 0 && !reads && !polls);
    D3DDevice_GetVisibilityTestResult(0, NULL, NULL);
    CHECK(visibility_queries_initialize());
    visibility_results_initialize();
    device.gl_ready = TRUE;
    query_buffer = 91; /* Save/clear/restore an unrelated buffer binding. */
    submit(0, 240); submit(1, 0);
    CHECK(result(0) == 0);
    present();
    CHECK(!reads && !polls && flushes == 1 && query_buffer == 91);
    target_samples = 1;
    submit(0, 120); present();
    submit(0, 180); present();
    /* Delay beyond the whole ring without allowing query reuse. */
    for (i = 0; i < 12; i++) {
        submit(0, 999); present();
        CHECK(result(0) == 0 && query_buffer == 91);
    }
    CHECK(!reads && device.visibility_stats.dropped == 12 && flushes == 3);
    ready(2, 0); /* Later availability cannot publish ahead of older work. */
    present(); CHECK(!reads && result(0) == 0);
    ready(0, 0); present();
    CHECK(result(0) == 10 && reads == 1); /* Original scale was 24, now 6. */
    CHECK(device.visibility_batches[0].sealed && device.visibility_batches[0].read == 1);
    CHECK(device.visibility_stats.completed == 0);
    ready(0, 1); present();
    CHECK(result(0) == 10 && result(1) == 0 && device.visibility_stats.completed == 1);
    /* Released batch records again while other batches remain pending. */
    submit(0, 240); present();
    ready(1, 0); present();
    CHECK(result(0) == 30 && device.visibility_stats.completed == 3);
    ready(0, 0); present(); CHECK(result(0) == 40);
    /* Real completed zero replaces the prior nonzero count, only when ready. */
    submit(0, 0); present();
    for (i = 0; i < 6; i++) { present(); CHECK(result(0) == 40); }
    finish_gpu(); present(); CHECK(result(0) == 0);
    /* Repeated slots, per-test scales, partial readiness and ring wrap. */
    for (i = 0; i < 20; i++) {
        batch = device.visibility_batch;
        target_samples = 4; submit(0, 240);
        target_samples = 1; submit(0, 120); submit(7, 60);
        present(); ready(batch, 0); present(); CHECK(result(0) == 10);
        partial = reads;
        present(); CHECK(reads == partial && result(0) == 10);
        finish_gpu(); present(); CHECK(result(0) == 20 && result(7) == 10);
    }
    before = reads;
    for (i = 0; i < VISIBILITY_TEST_SLOTS + 2; i++) submit(i, 6 * (i + 1));
    present(); finish_gpu(); present();
    CHECK(reads == before + VISIBILITY_TEST_SLOTS && result(0) == 1 && result(7) == 8);
    /* GL errors cannot publish initialized zero or recycle uncertain queries. */
    batch = device.visibility_batch;
    submit(0, 600); present(); finish_gpu();
    direct_error = TRUE; present(); direct_error = FALSE;
    CHECK(device.visibility_batches[batch].failed && result(0) == 1);
    batch = device.visibility_batch;
    submit(0, 1200); present(); finish_gpu(); before = reads;
    availability_error = TRUE; present(); availability_error = FALSE;
    CHECK(device.visibility_batches[batch].failed && result(0) == 1 && reads == before);
    CHECK(device.visibility_stats.failures == 2 && query_buffer == 91);
    for (i = 0; i < 5; i++) {
        submit(0, 180); present(); finish_gpu(); present(); CHECK(result(0) == 30);
    }
    /* Binding errors preserve pending results for a subsequent safe read. */
    submit(0, 240); present(); finish_gpu(); before = reads;
    binding_error = TRUE; present(); binding_error = FALSE;
    CHECK(reads == before && result(0) == 30 && query_buffer == 91);
    unbind_error = TRUE; present(); unbind_error = FALSE;
    CHECK(reads == before && result(0) == 30 && query_buffer == 91);
    present(); CHECK(result(0) == 40);
    /* All batches quarantined: retain the cache and stay nonblocking. */
    submit(0, 600); present(); finish_gpu();
    direct_error = TRUE; present(); direct_error = FALSE;
    before = reads;
    for (i = 0; i < 10; i++) { submit(0, 900); present(); }
    CHECK(reads == before && result(0) == 40 && device.visibility_stats.failures == 5);
    /* Reject every kind of invalid pool before publishing any objects. */
    memset(&device, 0, sizeof(device));
    zero_at = 0; CHECK(!visibility_queries_initialize());
    for (i = 0; i < VISIBILITY_BATCH_COUNT; i++) {
        duplicate_at = VISIBILITY_TEST_SLOTS * i + 1;
        CHECK(!visibility_queries_initialize() && !device.visibility_batches[i].queries[0]);
    }
    fail_object = TRUE; CHECK(!visibility_queries_initialize());
    fail_object = FALSE; fail_create = TRUE; CHECK(!visibility_queries_initialize());
    fail_create = FALSE; fail_all = TRUE; CHECK(!visibility_queries_initialize());
    for (i = 0; i < VISIBILITY_BATCH_COUNT; i++) CHECK(!device.visibility_batches[i].queries[0]);
    CHECK(!buffer_calls && !copies && !fence_count); /* AMD never uses mapped delivery. */
    return 0;
}
"""
    if buffered:
        main = main[:main.index("int main(void)")] + r"""
int main(void) {
    unsigned i, b, before;
    /* Only the proprietary AMD driver uses direct CPU delivery. */
    visibility_results_initialize(); CHECK(!buffer_calls && !device.visibility_mapped);
    vendor = "NVIDIA Corporation";
    map_null = TRUE; visibility_results_initialize();
    CHECK(!device.visibility_mapped && !device.visibility_buffer);
    CHECK(visibility_queries_initialize()); device.gl_ready = TRUE;
    submit(0, 240); present(); ready(0, 0); present();
    CHECK(result(0) == 10 && reads == 1 && !copies && !fence_count);
    memset(&device, 0, sizeof(device)); reads = polls = flushes = 0;
    map_null = FALSE; map_error = TRUE; visibility_results_initialize();
    CHECK(!device.visibility_mapped && !device.visibility_buffer);
    map_error = FALSE;
    CHECK(visibility_queries_initialize()); visibility_results_initialize();
    CHECK(device.visibility_mapped == mapped && !query_buffer);
    device.gl_ready = TRUE;
    query_buffer = 91;
    submit(0, 240); submit(1, 0); present();
    CHECK(copies == 2 && !reads && !polls && result(0) == 0 && query_buffer == 91);
    target_samples = 1;
    submit(0, 120); present(); submit(0, 180); present();
    for (i = 0; i < 12; i++) { submit(0, 999); present(); CHECK(result(0) == 0); }
    CHECK(device.visibility_stats.dropped == 12 && flushes == 3);
    complete_buffer(2); present(); CHECK(result(0) == 0);
    complete_buffer(0); present(); CHECK(result(0) == 10 && result(1) == 0);
    CHECK(device.visibility_stats.completed == 1);
    submit(0, 240); present(); complete_buffer(1); present();
    CHECK(result(0) == 30 && device.visibility_stats.completed == 3);
    complete_buffer(0); present(); CHECK(result(0) == 40);
    b = device.visibility_batch; submit(0, 0); present();
    for (i = 0; i < 6; i++) { present(); CHECK(result(0) == 40); }
    complete_buffer(b); present(); CHECK(result(0) == 0);
    for (i = 0; i < 20; i++) {
        b = device.visibility_batch;
        target_samples = 4; submit(0, 240);
        target_samples = 1; submit(0, 120); submit(7, 60); present();
        /* Query completion alone is insufficient: wait for buffer copies too. */
        ready(b, 0); present(); CHECK(device.visibility_batches[b].sealed);
        complete_buffer(b); present(); CHECK(result(0) == 20 && result(7) == 10);
    }
    b = device.visibility_batch; before = copies;
    for (i = 0; i < VISIBILITY_TEST_SLOTS + 2; i++) submit(i, 6 * (i + 1));
    present(); complete_buffer(b); present();
    CHECK(copies == before + VISIBILITY_TEST_SLOTS && result(0) == 1 && result(7) == 8);
    b = device.visibility_batch; submit(0, 600);
    copy_error = TRUE; present(); copy_error = FALSE;
    CHECK(device.visibility_batches[b].failed && result(0) == 1);
    b = device.visibility_batch; submit(0, 1200); present();
    wait_error = TRUE; present(); wait_error = FALSE;
    CHECK(device.visibility_batches[b].failed && result(0) == 1);
    b = device.visibility_batch; submit(0, 240); present(); complete_buffer(b); present();
    CHECK(result(0) == 40);
    b = device.visibility_batch; submit(0, 600);
    fence_null = TRUE; present(); fence_null = FALSE;
    CHECK(device.visibility_batches[b].failed && result(0) == 40);
    before = copies;
    for (i = 0; i < 10; i++) { submit(0, 900); present(); }
    CHECK(copies == before && result(0) == 40);
    CHECK(!reads && !polls && deletes == fence_count && query_buffer == 91);
    return 0;
}
"""
    path = tmp_path / "visibility.c"
    path.write_text(prefix.replace("/* BATCH_TYPE */", batch_type) + code + read_code + main, encoding="utf-8")
    binary = tmp_path / ("visibility.exe" if os.name == "nt" else "visibility")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
    build = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                           capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    if negative_control and negative_control != "buffered":
        assert run.returncode != 0, "negative control unexpectedly passed"
    else:
        assert run.returncode == 0, run.stderr
