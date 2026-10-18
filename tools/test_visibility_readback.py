"""Exercise the desktop renderer's real query allocation and asynchronous readback."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

from harness import function, mutated, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("negative_control", [None, "untyped_pool", "invalid_pool",
                                             "early_read", "wrong_scale", "wrong_range",
                                             "reuse_pending", "no_flush", "mapped_read"])
def test_visibility_readback(tmp_path, negative_control):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    code = source.split("/* ---------- visibility (occlusion) tests */", 1)[1]
    code = code.split("/* ---------- render and texture stage state */", 1)[0]
    copy_code = function(source, "visibility_copy_batch")
    batch_type = structure(source, "visibility_batch")
    if negative_control == "untyped_pool":
        code = mutated(code, "glCreateQueries(VISIBILITY_QUERY, (GLsizei)count, queries)",
                       "glGenQueries((GLsizei)count, queries)")
    if negative_control == "invalid_pool":
        code = mutated(code, "if (!success || zeros || duplicates || invalid)", "if (FALSE)")
    if negative_control == "early_read":
        copy_code = mutated(copy_code, "status == GL_TIMEOUT_EXPIRED", "FALSE")
        copy_code = mutated(copy_code, "status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED", "TRUE")
    if negative_control == "wrong_scale":
        copy_code = mutated(copy_code, "float area = batch->areas[test];", "float area = device.query_area[batch->slots[test]];")
    if negative_control == "wrong_range":
        copy_code = mutated(copy_code, "i * VISIBILITY_TEST_SLOTS + test", "batch->slots[test]")
    if negative_control == "reuse_pending":
        copy_code = mutated(copy_code, "device.visibility_batch = i;", "device.visibility_batch = i;\n\t\tbatch->sealed = FALSE;")
    if negative_control == "no_flush":
        copy_code = mutated(copy_code, "glFlush();", "((void)0);")
    if negative_control == "mapped_read":
        code = mutated(code, "\t\t\t*result = device.visibility_known[index];",
                       "\t\t\t*result = device.visibility_results[index];")
    prefix = r"""
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned int GLuint, DWORD, UINT, GLenum;
typedef unsigned long long ULONGLONG;
typedef struct fence *GLsync;
typedef int BOOL, HRESULT, GLsizei;
typedef long GLintptr;
#define WINAPI
#define TRUE 1
#define FALSE 0
#define S_OK 0
#define VISIBILITY_TEST_SLOTS 8
#define VISIBILITY_BATCH_COUNT 3
#define VISIBILITY_QUERY 1
#define GL_QUERY_RESULT_AVAILABLE 2
#define GL_QUERY_RESULT 3
#define GL_NO_ERROR 0
#define GL_QUERY_BUFFER 4
#define GL_MAP_READ_BIT 1
#define GL_MAP_PERSISTENT_BIT 2
#define GL_MAP_COHERENT_BIT 4
#define GL_SYNC_GPU_COMMANDS_COMPLETE 5
#define GL_ALREADY_SIGNALED 6
#define GL_CONDITION_SATISFIED 7
#define GL_TIMEOUT_EXPIRED 8
#define GL_WAIT_FAILED 9
#define platform_log(...) ((void)0)
static BOOL config_boolean(const char *key) { return FALSE; }
/* BATCH_TYPE */
static struct {
    BOOL gl_ready, visibility_test_active;
    GLuint queries[8], scratch_query, active_query;
    GLuint visibility_known[8];
    BOOL query_pending[8], visibility_unread[8];
    float query_area[8];
    GLuint visibility_results_buffer;
    volatile GLuint *visibility_results;
    struct visibility_batch visibility_batches[VISIBILITY_BATCH_COUNT];
    unsigned long visibility_batch, frame;
    ULONGLONG visibility_serial;
    BOOL visibility_logging;
    struct { unsigned long submitted, completed, samples, nonzero, maximum,
                           timeouts, dropped, failures; } visibility_stats;
} device;
static float target_scale[2] = {2, 3};
static int target_samples = 4;
static struct { GLuint samples; BOOL available, reserved, object, pending; } gpu[1024];
static GLuint active, next_samples, next_query = 1, mapped[8 * VISIBILITY_BATCH_COUNT];
static unsigned reads, polls, copies;
static struct { GLuint query, samples; unsigned offset; } commands[4096];
static struct fence { unsigned end; BOOL flushed, done, deleted; } fences[1024];
static unsigned command_count, command_done, fence_count;
static BOOL write_pending[8 * VISIBILITY_BATCH_COUNT];
static BOOL fail_fence, fail_wait, fail_mapping;
static unsigned sync_polls, flushes;
static int zero_at = -1, duplicate_at = -1;
static BOOL fail_object, fail_create, fail_all;
static GLenum gpu_error;
static GLenum glGetError(void) { GLenum error = gpu_error; gpu_error = 0; return error; }
static void glGenQueries(GLsizei count, GLuint *queries) {
    int i;
    for (i = 0; i < count; i++) {
        if (fail_all || i == zero_at) { queries[i] = 0; continue; }
        if (i == duplicate_at && i) { queries[i] = queries[i - 1]; continue; }
        assert(next_query < 1024);
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
static BOOL glIsQuery(GLuint query) { return query < 1024 && gpu[query].object; }
static void glBeginQuery(GLenum target, GLuint query) {
    assert(target == VISIBILITY_QUERY && !active && query && query < 1024);
    assert(gpu[query].reserved && gpu[query].object);
    assert(!gpu[query].pending); /* Never reuse an outstanding query. */
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
    assert(!gpu[query].pending && command_count < 4096);
    assert(!write_pending[offset / sizeof(GLuint)]);
    write_pending[offset / sizeof(GLuint)] = TRUE;
    /* Enqueue only. Neither command submission nor a frame implies completion. */
    commands[command_count].query = query;
    commands[command_count].samples = gpu[query].samples;
    commands[command_count++].offset = offset / sizeof(GLuint);
    gpu[query].pending = TRUE;
    copies++;
}
static GLsync glFenceSync(GLenum condition, unsigned flags) {
    GLsync fence;
    assert(condition == GL_SYNC_GPU_COMMANDS_COMPLETE && !flags);
    if (fail_fence) return NULL;
    assert(fence_count < 1024);
    fence = &fences[fence_count++];
    fence->end = command_count;
    return fence;
}
static void glFlush(void) {
    unsigned i;
    flushes++;
    for (i = 0; i < fence_count; i++) fences[i].flushed = TRUE;
}
static GLenum glClientWaitSync(GLsync fence, unsigned flags, ULONGLONG timeout) {
    assert(fence && !fence->deleted && !flags && !timeout); /* No blocking waits. */
    sync_polls++;
    if (fail_wait) return GL_WAIT_FAILED;
    return fence->done ? (fence == fences ? GL_ALREADY_SIGNALED : GL_CONDITION_SATISFIED) : GL_TIMEOUT_EXPIRED;
}
static void glDeleteSync(GLsync fence) { assert(!fence->deleted); fence->deleted = TRUE; }
static void finish_gpu(unsigned fence_index) {
    unsigned i;
    GLsync fence = &fences[fence_index];
    assert(fence->flushed);
    while (command_done < fence->end) {
        unsigned c = command_done++;
        mapped[commands[c].offset] = commands[c].samples;
        write_pending[commands[c].offset] = FALSE;
        gpu[commands[c].query].pending = FALSE;
    }
    for (i = 0; i <= fence_index; i++) fences[i].done = TRUE;
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
    return fail_mapping ? NULL : mapped;
}
"""
    main = r"""
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); return 1; } } while (0)
static void submit(unsigned slot, unsigned samples) {
    next_samples = samples;
    D3DDevice_BeginVisibilityTest();
    D3DDevice_EndVisibilityTest(slot);
}
static void present(void) { visibility_copy_batch(); device.frame++; }
static UINT result(unsigned slot) {
    UINT value = 999;
    D3DDevice_GetVisibilityTestResult(slot, &value, NULL);
    return value;
}
static void reset(void) {
    memset(&device, 0, sizeof(device));
    memset(gpu, 0, sizeof(gpu));
    memset(fences, 0, sizeof(fences));
    memset(write_pending, 0, sizeof(write_pending));
    next_query = 1;
    command_count = command_done = fence_count = 0;
    copies = sync_polls = flushes = reads = polls = 0;
    fail_fence = fail_wait = fail_mapping = FALSE;
    target_scale[0] = 2; target_scale[1] = 3; target_samples = 4;
}
int main(void) {
    UINT value = 99;
    ULONGLONG stamp = 99;
    unsigned i, before, batch;
    GLsync later;
    D3DDevice_GetVisibilityTestResult(0, &value, &stamp);
    CHECK(value == 0 && stamp == 0 && !reads && !polls);
    CHECK(visibility_queries_initialize());
    visibility_results_initialize();
    device.gl_ready = TRUE;
    /* Poisoned staging must never leak, even before the first GPU completion. */
    memset(mapped, 0xcc, sizeof(mapped));
    submit(0, 240);
    submit(1, 0);
    CHECK(result(0) == 0);
    present();
    CHECK(copies == 0); /* Still defer query copies by one frame. */
    target_samples = 1;
    submit(0, 120);
    present();
    CHECK(copies == 2 && fence_count == 1 && result(0) == 0);
    submit(0, 180);
    present();
    CHECK(copies == 3 && result(0) == 0);
    /* GPU is delayed longer than the entire ring. Queries and ranges remain
       owned, new measurements are dropped and the CPU never waits. */
    for (i = 0; i < 12; i++) {
        submit(0, 999);
        present();
        CHECK(result(0) == 0);
    }
    CHECK(copies == 4 && fence_count == 3 && device.visibility_stats.dropped == 12);
    CHECK(sync_polls && !reads && !polls);
    /* A later notification must not retire ahead of an older batch. Real GL
       fences are ordered; simulate staggered host observation defensively. */
    later = &fences[2];
    later->done = TRUE;
    present();
    CHECK(!later->deleted && result(0) == 0);
    later->done = FALSE;
    finish_gpu(0);
    present();
    CHECK(result(0) == 10 && result(1) == 0); /* Original MSAA scale, not 6. */
    CHECK(device.visibility_stats.completed == 1 && device.visibility_stats.samples == 2);
    /* The released batch can record again while the others remain pending. */
    submit(0, 240);
    present();
    finish_gpu(2);
    present();
    CHECK(result(0) == 30); /* Older batches retire in order. */
    finish_gpu(3);
    present();
    CHECK(result(0) == 40);
    /* Zero is a completed result too. Keep the old nonzero value until ready. */
    submit(0, 0);
    present(); present();
    CHECK(result(0) == 40);
    finish_gpu(fence_count - 1);
    present();
    CHECK(result(0) == 0);
    /* Duplicate slots use distinct staging locations and per-test scales. */
    for (i = 0; i < 20; i++) {
        target_samples = 4;
        submit(0, 240);
        target_samples = 1;
        submit(0, 120);
        submit(7, 60);
        present(); present();
        finish_gpu(fence_count - 1);
        present();
        CHECK(result(0) == 20 && result(7) == 10);
    }
    before = copies;
    for (i = 0; i < VISIBILITY_TEST_SLOTS + 2; i++) submit(i % VISIBILITY_TEST_SLOTS, 6 * (i + 1));
    present(); present();
    finish_gpu(fence_count - 1);
    present();
    CHECK(copies == before + VISIBILITY_TEST_SLOTS && result(0) == 1 && result(7) == 8);
    CHECK(!reads && !polls);
    for (i = 0; i < fence_count; i++) CHECK(fences[i].deleted);
    /* A failed fence must never permit a read or range/query reuse. The
       remaining batches continue; failures are counted and quarantined. */
    batch = device.visibility_batch;
    fail_fence = TRUE;
    submit(0, 600);
    present(); present();
    CHECK(device.visibility_batches[batch].failed && result(0) == 1);
    fail_fence = FALSE;
    batch = device.visibility_batch;
    submit(0, 1200);
    present(); present();
    fail_wait = TRUE;
    present();
    CHECK(device.visibility_batches[batch].failed && result(0) == 1);
    fail_wait = FALSE;
    CHECK(device.visibility_stats.failures == 2);
    for (i = 0; i < 5; i++) {
        submit(0, 180);
        present(); present();
        finish_gpu(fence_count - 1);
        present();
        CHECK(result(0) == 30);
    }
    /* All batches failed: stay nonblocking and preserve the completed cache. */
    fail_fence = TRUE;
    submit(0, 600);
    present(); present();
    before = copies;
    for (i = 0; i < 10; i++) { submit(0, 900); present(); }
    CHECK(copies == before && result(0) == 30 && device.visibility_stats.failures == 3);
    /* Mapping failure retains the availability-polled CPU fallback. */
    reset();
    CHECK(visibility_queries_initialize());
    fail_mapping = TRUE;
    visibility_results_initialize();
    device.gl_ready = TRUE;
    CHECK(!device.visibility_results);
    submit(0, 240);
    CHECK(result(0) == 0 && reads == 0 && polls == 1);
    gpu[device.queries[0]].available = TRUE;
    CHECK(result(0) == 10 && reads == 1 && polls == 2);
    CHECK(result(0) == 10 && reads == 1 && polls == 2);
    submit(0, 0);
    CHECK(result(0) == 10 && reads == 1 && polls == 3);
    gpu[device.queries[0]].available = TRUE;
    CHECK(result(0) == 0 && reads == 2 && polls == 4);
    present();
    CHECK(!copies && !sync_polls);
    /* No partially valid pool may be published, including every batch. */
    memset(&device, 0, sizeof(device));
    zero_at = 0;
    CHECK(!visibility_queries_initialize() && !device.scratch_query && !device.queries[0]);
    for (i = 0; i < VISIBILITY_BATCH_COUNT; i++) {
        duplicate_at = VISIBILITY_TEST_SLOTS * (i + 1) + 1;
        CHECK(!visibility_queries_initialize() && !device.visibility_batches[i].queries[0]);
    }
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
    path.write_text(prefix.replace("/* BATCH_TYPE */", batch_type) + code + copy_code + main, encoding="utf-8")
    binary = tmp_path / ("visibility.exe" if os.name == "nt" else "visibility")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
    build = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                           capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True)
    if negative_control:
        assert run.returncode != 0, "negative control unexpectedly passed"
    else:
        assert run.returncode == 0, run.stderr
