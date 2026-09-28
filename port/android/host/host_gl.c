/*
HOST_GL.C

OpenGL ES for the guest. Its generated entry points (guest_gl.c) import
hostgl_<function>, resolved here to the driver's function; the arguments
already have host types by then. Only strings need copying back.
*/

#include "host.h"

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <dlfcn.h>
#include <string.h>
#include <time.h>

void *host_gl_resolve(const char *name)
{
	static void *library;
	void *function = NULL;

	if (!library)
		library = dlopen("libGLESv3.so", RTLD_NOW | RTLD_GLOBAL);
	if (library)
		function = dlsym(library, name);
	if (!function)
		function = (void *)eglGetProcAddress(name);
	return function;
}

void host_gl_get_string(uint32_t name, int index, char *buffer, uint32_t size)
{
	const GLubyte *text = index >= 0 ? glGetStringi(name, (GLuint)index) : glGetString(name);

	if (!size)
		return;
	buffer[0] = 0;
	if (text)
	{
		strncpy(buffer, (const char *)text, size - 1);
		buffer[size - 1] = 0;
	}
}

int host_gl_has_extension(const char *name)
{
	GLint count = 0, index;

	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)index);

		if (extension && !strcmp(extension, name))
			return 1;
	}
	return 0;
}

/* one 32-bit word of a buffer object (the visibility test counters of
d3d8_gl.c); ES has no glGetBufferSubData, and the mapping it offers
instead is a host pointer */
uint32_t host_gl_read_buffer_word(uint32_t buffer, uint32_t offset)
{
	uint32_t value = 0;
	GLint previous = 0;
	const void *mapping;

	glGetIntegerv(GL_ATOMIC_COUNTER_BUFFER_BINDING, &previous);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer);
	mapping = glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, offset, sizeof(value), GL_MAP_READ_BIT);
	if (mapping)
	{
		memcpy(&value, mapping, sizeof(value));
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	}
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, (GLuint)previous);
	return value;
}

/* The renderer streams each frame's vertices and indices into the next of
a ring of buffers (d3d8_gl.c). A fence marks the end of each frame's work,
and a buffer is written again only once the GPU has passed the fence of the
frame that last used it: drivers queue several frames, and a draw still
waiting to run would otherwise read a later frame's vertices. */
#define FRAME_FENCE_SLOTS 8

static GLsync frame_fences[FRAME_FENCE_SLOTS];

/* ---------- fence telemetry (debug.fence_stats)

The renderer's pacing rests on the wait below returning before the timeout.
That is an assumption, not something the code checked: if the wait does give
up, the renderer goes on to fill a buffer the GPU may still be reading, and
the resulting vertex garbage would show up much later as a frame that is
suddenly and inexplicably slow. So the wait is timed and classified, and the
buffers filled after a wait that gave up are counted separately.

This exists to settle which of two explanations fits the slowdown Android
shows (docs/android_graphics_threading_investigation.md): the GPU falling
behind (waits block, timeouts accumulate), or the unsynchronized mapping in
host_gl_buffer_write corrupting vertex data (uploads counted as racing). A
timeout count above zero means the second explanation is live, whatever the
wait times say. */

static int fence_stats_enabled;
static uint32_t active_slot;
static int slot_racing[FRAME_FENCE_SLOTS];

static struct
{
	uint64_t window_start;
	unsigned long frames, signalled, satisfied, timed_out, failed;
	uint64_t blocked_ns, worst_blocked_ns;
	unsigned long uploads, racing_uploads, upload_bytes;
	uint64_t call_ns, worst_call_ns, probe_ns;
} fence_stats;

static uint64_t monotonic_ns(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (uint64_t)now.tv_sec * 1000000000ull + (uint64_t)now.tv_nsec;
}

void host_gl_fence_stats(int enabled)
{
	memset(&fence_stats, 0, sizeof(fence_stats));
	memset(slot_racing, 0, sizeof(slot_racing));
	fence_stats.window_start = monotonic_ns();
	fence_stats_enabled = enabled;
}

/* once a second: the wait, and what it cost */
static void fence_stats_report(void)
{
	uint64_t now = monotonic_ns();
	uint64_t elapsed = now - fence_stats.window_start;
	unsigned long frames = fence_stats.frames;

	if (elapsed < 1000000000ull || !frames)
		return;
	host_logf(HOST_LOG_INFO, "fences: %lu frames, %.1f fps; %lu already signalled, %lu waited, "
		"%lu TIMED OUT, %lu failed; blocked %.1f ms of %.1f ms, worst %.1f ms",
		frames, (double)frames * 1000000000.0 / (double)elapsed,
		fence_stats.signalled, fence_stats.satisfied, fence_stats.timed_out, fence_stats.failed,
		(double)fence_stats.blocked_ns / 1000000.0, (double)elapsed / 1000000.0,
		(double)fence_stats.worst_blocked_ns / 1000000.0);
	host_logf(HOST_LOG_INFO, "uploads: %lu calls, %lu KB; %lu RACE against a "
		"buffer the GPU is still reading",
		fence_stats.uploads, fence_stats.upload_bytes / 1024,
		fence_stats.racing_uploads);
	/* the guest times the same calls from the other side, so the two
	together say whether the cost is the driver's or the crossing from
	the 32-bit guest into this process */
	host_logf(HOST_LOG_INFO, "uploads: %.2f ms in the driver, %.3f ms per call, worst %.3f",
		(double)fence_stats.call_ns / 1000000.0,
		fence_stats.uploads ? (double)fence_stats.call_ns / (double)fence_stats.uploads / 1000000.0 : 0.0,
		(double)fence_stats.worst_call_ns / 1000000.0);
	memset(&fence_stats, 0, sizeof(fence_stats));
	fence_stats.window_start = now;
}

void host_gl_fence_frame(uint32_t slot)
{
	if (slot >= FRAME_FENCE_SLOTS)
		return;
	if (frame_fences[slot])
		glDeleteSync(frame_fences[slot]);
	frame_fences[slot] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void host_gl_wait_frame(uint32_t slot)
{
	uint64_t start, duration;
	GLenum result;

	if (slot >= FRAME_FENCE_SLOTS)
		return;
	if (!frame_fences[slot])
	{
		/* nothing was fenced for this slot, so nothing gates the fills
		that follow; the first frames before any fence exists are the
		only time this happens, so it is not worth counting */
		return;
	}
	start = fence_stats_enabled ? monotonic_ns() : 0;
	/* at most a second: a lost context must not hang the game */
	result = glClientWaitSync(frame_fences[slot], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
	duration = fence_stats_enabled ? monotonic_ns() - start : 0;

	/* the renderer fills this slot's buffers next, so a wait that gave up
	means the GPU is still reading them */
	active_slot = slot;
	slot_racing[slot] = result == GL_TIMEOUT_EXPIRED;

	if (fence_stats_enabled)
	{
		fence_stats.frames++;
		fence_stats.blocked_ns += duration;
		if (duration > fence_stats.worst_blocked_ns)
			fence_stats.worst_blocked_ns = duration;
		switch (result)
		{
			case GL_ALREADY_SIGNALED: fence_stats.signalled++; break;
			case GL_CONDITION_SATISFIED: fence_stats.satisfied++; break;
			case GL_TIMEOUT_EXPIRED: fence_stats.timed_out++; break;
			default: fence_stats.failed++; break;
		}
		fence_stats_report();
	}
	glDeleteSync(frame_fences[slot]);
	frame_fences[slot] = NULL;
}

/* writes data into the buffer bound to target. The renderer streams a
range per draw, so a frame makes hundreds of these and the cost per call,
not per byte, is what the frame is made of.

That cost was where the game went on Android. On an Adreno 8 Gen 1 every
call cost about 0.85 ms whatever range it wrote - enough to spend 96% of a
550 ms frame in here and leave the GPU idle the whole time - and the frame
rate was 1.8. GL_MAP_INVALIDATE_RANGE_BIT was the cause: it tells the driver
the range's previous contents are undefined and must be discarded, which is
exactly the work it was avoiding by being told the range was free. Together
with GL_MAP_UNSYNCHRONIZED_BIT, which says no queued draw is reading this
range, the two contradict each other and the driver pays for the discard.

Dropping INVALIDATE_RANGE, and with it glBufferSubData, takes the call to
well under a microsecond and the frame to 8.3 ms, which is the display's
refresh rate. The safety argument is the same as it was: the renderer only
writes ranges that no queued draw reads, because the ring fences in
host_gl_wait_frame release the slot first, and the RACE counter below
reports if that ever stops being true. */
void host_gl_buffer_write(uint32_t target, uint32_t offset, uint32_t size, const void *data)
{
	/* whether the wait that opened this slot really did release it, which
	is the assumption the whole scheme rests on */
	int racing = fence_stats_enabled && slot_racing[active_slot];
	uint64_t start = 0, duration;
	void *mapping;

	if (fence_stats_enabled)
	{
		fence_stats.uploads++;
		fence_stats.upload_bytes += size;
		if (racing)
			fence_stats.racing_uploads++;
		start = monotonic_ns();
	}
	/* GL_MAP_UNSYNCHRONIZED on its own, without INVALIDATE_RANGE. The
	invalidation asks the driver to discard what the range held, which
	is the one thing that can pull the queued draws that reference it
	back into the call; the renderer only ever writes ranges no queued
	draw uses, so the promise unsynchronized makes is true here. */
	mapping = glMapBufferRange((GLenum)target, offset, size, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	if (mapping)
	{
		memcpy(mapping, data, size);
		glUnmapBuffer((GLenum)target);
	}
	else
	{
		glBufferSubData((GLenum)target, offset, size, data);
	}
	if (fence_stats_enabled)
	{
		duration = monotonic_ns() - start;
		fence_stats.call_ns += duration;
		if (duration > fence_stats.worst_call_ns)
			fence_stats.worst_call_ns = duration;
	}
}
