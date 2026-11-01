/* The host's real frame fences and upload, with a GPU that can outlive the
wait timeout. A queued draw must see its own bytes before the next upload. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int GLenum;
typedef unsigned long long GLuint64;
typedef struct fence *GLsync;
#define GL_SYNC_GPU_COMMANDS_COMPLETE 1
#define GL_SYNC_FLUSH_COMMANDS_BIT 2
#define GL_ALREADY_SIGNALED 3
#define GL_CONDITION_SATISFIED 4
#define GL_TIMEOUT_EXPIRED 5
#define GL_WAIT_FAILED 6
#define GL_MAP_WRITE_BIT 1
#define GL_MAP_UNSYNCHRONIZED_BIT 2
#define HOST_LOG_WARN 5
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static struct fence { int live; } fences[64];
static unsigned int created, deleted, waits, finishes, logs;
static GLenum wait_result;
static int refuse_fence, pending;
static unsigned char storage[16], expected[16];

static void complete_draw(void)
{
	if (pending)
		CHECK(!memcmp(storage, expected, sizeof(storage)));
	pending = 0;
}

static GLsync glFenceSync(GLenum condition, unsigned int flags)
{
	CHECK(condition == GL_SYNC_GPU_COMMANDS_COMPLETE && flags == 0);
	if (refuse_fence)
		return NULL;
	CHECK(created < 64);
	fences[created].live = 1;
	return &fences[created++];
}

static GLenum glClientWaitSync(GLsync fence, unsigned int flags, GLuint64 timeout)
{
	CHECK(fence && fence->live && flags == GL_SYNC_FLUSH_COMMANDS_BIT);
	CHECK(timeout > 0);
	waits++;
	if (wait_result == GL_ALREADY_SIGNALED || wait_result == GL_CONDITION_SATISFIED)
		complete_draw();
	return wait_result;
}

static void glDeleteSync(GLsync fence)
{
	CHECK(fence && fence->live);
	fence->live = 0;
	deleted++;
}

static void glFinish(void) { finishes++; complete_draw(); }
static void host_logf(int priority, const char *format, ...)
{
	CHECK(priority == HOST_LOG_WARN && format);
	logs++;
}

static void *glMapBufferRange(uint32_t target, uint32_t offset, uint32_t size, unsigned int flags)
{
	CHECK(target == 1 && offset == 0 && size == sizeof(storage));
	CHECK(flags == (GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT));
	return storage;
}
static void glBufferSubData(uint32_t target, uint32_t offset, uint32_t size, const void *data)
{
	(void)target; (void)offset; (void)size; (void)data;
	CHECK(0);
}
static void glUnmapBuffer(uint32_t target) { CHECK(target == 1); }

#include "frame_fences.inc"

int main(int argc, char **argv)
{
	unsigned int iteration, slot;
	unsigned char next[sizeof(storage)];
	int fallback;

	CHECK(argc == 2);
	wait_result = GL_ALREADY_SIGNALED;
	if (!strcmp(argv[1], "satisfied")) wait_result = GL_CONDITION_SATISFIED;
	if (!strcmp(argv[1], "timeout")) wait_result = GL_TIMEOUT_EXPIRED;
	if (!strcmp(argv[1], "failed")) wait_result = GL_WAIT_FAILED;
	refuse_fence = !strcmp(argv[1], "creation_failed");
	if (!strcmp(argv[1], "empty"))
	{
		host_gl_wait_frame(0);
		host_gl_wait_frame(FRAME_FENCE_SLOTS);
		host_gl_fence_frame(FRAME_FENCE_SLOTS);
		CHECK(!created && !waits && !finishes && !deleted && !logs);
		return 0;
	}
	fallback = refuse_fence || wait_result == GL_TIMEOUT_EXPIRED || wait_result == GL_WAIT_FAILED;
	/* Reuse all three ring slots repeatedly, as across frames and levels. */
	for (iteration = 0; iteration < 18; iteration++)
	{
		slot = iteration % 3;
		memset(storage, (int)iteration, sizeof(storage));
		memcpy(expected, storage, sizeof(storage));
		pending = 1;
		host_gl_fence_frame(slot);
		if (!strcmp(argv[1], "replacement")) host_gl_fence_frame(slot);
		host_gl_wait_frame(slot);
		memset(next, (int)iteration + 1, sizeof(next));
		host_gl_buffer_write(1, 0, sizeof(next), next);
		complete_draw();
		CHECK(!frame_fences[slot]);
		host_gl_wait_frame(slot);
	}
	CHECK(deleted == created);
	CHECK(waits == (refuse_fence ? 0U : 18U));
	CHECK(finishes == (fallback ? 18U : 0U));
	CHECK(logs == (fallback ? 1U : 0U));
	return 0;
}
