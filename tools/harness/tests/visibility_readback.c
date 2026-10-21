#include "harness.h"

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>

typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned long long GLuint64;
typedef intptr_t GLintptr;
typedef unsigned long long ULONGLONG;
typedef int BOOL;
typedef struct fence *GLsync;
#define GL_NO_ERROR 0
#define GL_INVALID_OPERATION 0x0502
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_QUERY_BUFFER 0x9192
#define GL_QUERY_BUFFER_BINDING 0x9193
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#define GL_ALREADY_SIGNALED 0x911A
#define GL_TIMEOUT_EXPIRED 0x911B
#define GL_CONDITION_SATISFIED 0x911C
#define GL_WAIT_FAILED 0x911D
#include "config.inc"
/* (the code before the change has none) */
#ifndef VISIBILITY_UNCOPIED
#define VISIBILITY_UNCOPIED 0xFFFFFFFFUL
#endif

static struct
{
	unsigned long frame;
	GLuint visibility_known[VISIBILITY_TEST_SLOTS];
	struct visibility_batch visibility_batches[VISIBILITY_BATCH_COUNT];
	unsigned long visibility_batch;
	ULONGLONG visibility_serial;
	BOOL visibility_unavailable;
	BOOL visibility_logging;
	GLuint visibility_buffer;
	volatile GLuint *visibility_mapped;
	struct
	{
		unsigned long submitted, completed, samples, nonzero, maximum;
		unsigned long unavailable, dropped, failures;
	} visibility_stats;
} device;

/* the GPU: each query's count, and the commands it has yet to carry out, in
the order they were sent (gpu_run carries them out) */
struct fence
{
	BOOL signaled, broken;
};
enum { COMMAND_COPY, COMMAND_FENCE };
static struct
{
	int type;
	GLuint query;
	GLintptr offset;
	struct fence *fence;
} commands[512];
static unsigned long command_count;
static GLuint query_samples[64];
static GLuint results[VISIBILITY_BATCH_COUNT * VISIBILITY_TEST_SLOTS];
static struct fence fences[512];
static unsigned long fence_count;
static BOOL copy_lost, copy_error, fence_broken;
static GLenum gl_error;
/* calls that make a threaded driver catch up with every command before them
(they answer the CPU), all of them and those before the swap; the flushes */
static unsigned long driver_answers, answers_before_swap, flushes, logs;
static BOOL before_swap;

static void answered(void)
{
	driver_answers++;
	answers_before_swap += before_swap;
}

static GLenum glGetError(void)
{
	GLenum error = gl_error;

	answered();
	gl_error = GL_NO_ERROR;
	return error;
}

static void glGetQueryBufferObjectuiv(GLuint query, GLuint buffer, GLenum name, GLintptr offset)
{
	CHECK(buffer == device.visibility_buffer && name == GL_QUERY_RESULT, "a copy into another buffer");
	commands[command_count].type = COMMAND_COPY;
	commands[command_count].query = query;
	commands[command_count++].offset = offset;
	if (copy_error)
		gl_error = GL_INVALID_OPERATION;
}

static GLsync glFenceSync(GLenum condition, GLbitfield flags)
{
	struct fence *fence = &fences[fence_count++ % 512];

	fence->signaled = FALSE;
	fence->broken = fence_broken;
	commands[command_count].type = COMMAND_FENCE;
	commands[command_count++].fence = fence;
	return fence;
}

static GLenum glClientWaitSync(GLsync fence, GLbitfield flags, GLuint64 timeout)
{
	/* (a fence not yet sent to the GPU may never pass without a flush) */
	CHECK(flags & GL_SYNC_FLUSH_COMMANDS_BIT, "a fence waited on without asking for a flush");
	answered();
	if (fence->broken)
		return GL_WAIT_FAILED;
	return fence->signaled ? GL_ALREADY_SIGNALED : GL_TIMEOUT_EXPIRED;
}

static void glDeleteSync(GLsync fence) { }
static void glFlush(void) { flushes++; }
static void glBindBuffer(GLenum target, GLuint buffer) { }
static void glGetIntegerv(GLenum name, GLint *value) { answered(); *value = 0; }
static void glGetQueryObjectuiv(GLuint query, GLenum name, GLuint *value)
{
	answered();
	*value = name == GL_QUERY_RESULT_AVAILABLE ? 1 : query_samples[query];
}

static void platform_log(const char *format, ...)
{
	va_list arguments;

	logs++;
	va_start(arguments, format);
	vfprintf(stderr, format, arguments);
	va_end(arguments);
	fputc('\n', stderr);
}

#include "under_test.inc"

/* the GPU carries out the first count of its commands (all of them with
ULONG_MAX): a copy writes its query's count (none when copy_lost), a fence
passes */
static void gpu_run(unsigned long count)
{
	unsigned long i;

	if (count > command_count)
		count = command_count;
	for (i = 0; i < count; i++)
	{
		if (commands[i].type == COMMAND_COPY && !copy_lost)
			results[commands[i].offset / sizeof(GLuint)] = query_samples[commands[i].query];
		else if (commands[i].type == COMMAND_FENCE)
			commands[i].fence->signaled = TRUE;
	}
	for (i = count; i < command_count; i++)
		commands[i - count] = commands[i];
	command_count -= count;
}

/* a present: the batch sent before the swap, the counts read after it */
static void present(unsigned long index)
{
	device.frame = index;
	before_swap = TRUE;
	visibility_send_batch();
	before_swap = FALSE;
	visibility_read_batch();
}

/* a frame's two tests in the batch being filled (slots 10 and 11, counts by
frame), then its present */
static struct visibility_batch *frame(unsigned long index)
{
	struct visibility_batch *batch = &device.visibility_batches[device.visibility_batch];

	CHECK(device.visibility_batch < VISIBILITY_BATCH_COUNT, "no batch free at frame %lu", index);
	batch->queries[0] = (GLuint)(1 + (index * 2) % 62);
	batch->queries[1] = (GLuint)(2 + (index * 2) % 62);
	query_samples[batch->queries[0]] = (GLuint)(100 + index);
	query_samples[batch->queries[1]] = (GLuint)(200 + index);
	batch->slots[0] = 10;
	batch->slots[1] = 11;
	batch->areas[0] = batch->areas[1] = 1.0f;
	batch->count = 2;
	present(index);
	return batch;
}

static void start(BOOL logging)
{
	device.visibility_buffer = 7;
	device.visibility_mapped = results;
	device.visibility_logging = logging;
	device.visibility_known[10] = 77;
	device.visibility_known[11] = 88;
}

static void known(GLuint first, GLuint second, const char *what)
{
	CHECK(device.visibility_known[10] == first && device.visibility_known[11] == second,
		"%s: the slots hold %u and %u, not %u and %u", what, device.visibility_known[10], device.visibility_known[11],
		first, second);
}

/* frames whose commands the GPU carries out before the next present, which
reads their counts */
static void play(unsigned long frames)
{
	unsigned long index;

	for (index = 0; index < frames; index++)
	{
		frame(index);
		if (index)
			known((GLuint)(100 + index - 1), (GLuint)(200 + index - 1), "a frame on");
		gpu_run(ULONG_MAX);
	}
	CHECK(device.visibility_stats.completed == frames - 1, "%lu batches read, not %lu",
		device.visibility_stats.completed, frames - 1);
}

int main(int argc, char **argv)
{
	const char *case_name = argv[1];

	CASE("quiet")
	{
		/* without debug.gl_debug, nothing before the swap asks the driver
		for an answer, no present flushes (the swap does), and after the
		swap at most one question a present */
		start(FALSE);
		play(12);
		CHECK(answers_before_swap == 0, "the driver was asked %lu times before the swap in 12 presents",
			answers_before_swap);
		CHECK(flushes == 0, "%lu flushes in 12 presents", flushes);
		CHECK(driver_answers <= 12, "the driver was asked %lu times in 12 presents", driver_answers);
	}
	CASE("late")
	{
		/* a GPU 1000 presents behind: its counts are not taken before their
		fence passes, no batch is given up, and the counts are read once
		they come (visibility goes on updating) */
		unsigned long index;

		start(FALSE);
		frame(0);
		frame(1);
		frame(2);
		for (index = 3; index < 1003; index++)
			present(index);
		known(77, 88, "nothing carried out");
		for (index = 0; index < VISIBILITY_BATCH_COUNT; index++)
			CHECK(!device.visibility_batches[index].failed, "batch %lu given up while the GPU was late", index);
		gpu_run(2);
		present(1003);
		known(77, 88, "copies done, fence not passed");
		gpu_run(ULONG_MAX);
		present(1004);
		known(102, 202, "all carried out");
		frame(1005);
		gpu_run(ULONG_MAX);
		present(1006);
		known(1105, 1205, "the next batch after a late GPU");
	}
	CASE("lost_copy")
	{
		/* a copy that wrote nothing (a failed one) is seen without asking
		for GL errors: its place still holds VISIBILITY_UNCOPIED once the
		fence has passed. Its batch is dropped, the slots keep their earlier
		counts, and the counts are read from each query from then on: the
		batch sent meanwhile, and those after it. */
		start(FALSE);
		frame(0);
		copy_lost = TRUE;
		gpu_run(ULONG_MAX);
		copy_lost = FALSE;
		frame(1);
		CHECK(device.visibility_batches[0].failed, "a batch whose counts were never copied was kept");
		known(77, 88, "after a lost copy");
		CHECK(logs > 0 && device.visibility_stats.failures, "a lost copy not reported");
		CHECK(driver_answers <= 1, "GL errors asked for without debug.gl_debug (%lu answers)", driver_answers);
		CHECK(!device.visibility_mapped, "the buffer still read after its copies failed");
		gpu_run(ULONG_MAX);
		present(2);
		known(101, 201, "the batch sent before the lost copy was seen");
		frame(3);
		present(4);
		known(103, 203, "a batch sent after the lost copy");
	}
	CASE("fence_failed")
	{
		/* a fence that cannot be waited on drops its batch, keeping the
		slots' earlier counts; the counts are read from each query from
		then on */
		start(FALSE);
		fence_broken = TRUE;
		frame(0);
		fence_broken = FALSE;
		gpu_run(ULONG_MAX);
		frame(1);
		CHECK(device.visibility_batches[0].failed, "a batch whose fence failed was read");
		known(77, 88, "after a failed fence");
		CHECK(!device.visibility_mapped, "the buffer still read after a fence failed");
		present(2);
		known(101, 201, "the batch after a failed fence");
	}
	CASE("logging")
	{
		/* with debug.gl_debug, GL errors are asked for, and a copy that
		failed drops its batch as it is sent */
		start(TRUE);
		play(6);
		CHECK(driver_answers > 6, "GL errors never asked for with debug.gl_debug");
		copy_error = TRUE;
		CHECK(frame(6)->failed, "a batch whose copies failed was kept");
		CHECK(logs > 0 && device.visibility_stats.failures, "a failed copy not reported");
		CHECK(!device.visibility_mapped, "the buffer still read after its copies failed");
		copy_error = FALSE;
		frame(7);
		present(8);
		known(107, 207, "a batch after the failed copies");
	}
	return 0;
}
