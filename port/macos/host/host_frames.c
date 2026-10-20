/*
HOST_FRAMES.C

HALO_HOST_FRAME_LOG=<file>: a line for each frame the renderer has finished
presenting, with the time since the one before, the GPU's time on it and the
VM's exits in between, so that a run's frame times can be measured
(tools/test_macos_benchmark.py). The app's renderer calls
halo_host_frame_presented as each PRESENT's commands complete.
*/

#include "host.h"
#include "halo_host.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static pthread_once_t opened = PTHREAD_ONCE_INIT;
static pthread_mutex_t frame_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *frame_log;
static unsigned long long frames;
static unsigned long long last_exits;
static struct timespec last_present;

static void open_log(void)
{
	const char *path = getenv("HALO_HOST_FRAME_LOG");

	if (!path || !*path)
		return;
	frame_log = fopen(path, "w");
	if (!frame_log)
	{
		host_logf(HOST_LOG_WARN, "cannot write the frame log %s", path);
		return;
	}
	fputs("frame,milliseconds,gpu_milliseconds,exits\n", frame_log);
	clock_gettime(CLOCK_MONOTONIC, &last_present);
	last_exits = host_vm_exit_count();
}

int host_frames_logging(void)
{
	const char *path = getenv("HALO_HOST_FRAME_LOG");

	return path && *path;
}

int halo_host_frame_log_enabled(void)
{
	return host_frames_logging();
}

void halo_host_frame_presented(double gpu_milliseconds)
{
	struct timespec now;
	unsigned long long exits;
	double milliseconds;

	pthread_once(&opened, open_log);
	if (!frame_log)
		return;
	pthread_mutex_lock(&frame_lock);
	clock_gettime(CLOCK_MONOTONIC, &now);
	exits = host_vm_exit_count();
	milliseconds = (double)(now.tv_sec - last_present.tv_sec) * 1e3 + (now.tv_nsec - last_present.tv_nsec) / 1e6;
	frames++;
	fprintf(frame_log, "%llu,%.3f,%.3f,%llu\n", frames, milliseconds, gpu_milliseconds, exits - last_exits);
	last_present = now;
	last_exits = exits;
	pthread_mutex_unlock(&frame_lock);
}

void host_frames_close(void)
{
	pthread_mutex_lock(&frame_lock);
	if (frame_log)
		fflush(frame_log);
	pthread_mutex_unlock(&frame_lock);
}
