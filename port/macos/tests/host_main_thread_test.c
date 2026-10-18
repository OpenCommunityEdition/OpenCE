/*
HOST_MAIN_THREAD_TEST.C

A native test of port/macos/host/host_main_thread.c (tools/test_macos_vm.py):
requests from other threads run on the main thread, by its run loop, and
get their results back; once the host shuts down, a waiting request and
later ones fail instead of waiting.
*/

#include "host.h"
#include "halo_host.h"

#include <CoreFoundation/CoreFoundation.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

__thread int host_saved_errno;

#define WORKERS 4
#define REQUESTS 50

struct worker
{
	pthread_t thread;
	int on_main;
	int sum;
	int result;
	volatile int done;
};

static void add_on_main(void *context)
{
	struct worker *worker = context;

	if (pthread_main_np())
		worker->on_main++;
	worker->sum++;
}

static void *requests(void *argument)
{
	struct worker *worker = argument;
	int i;

	for (i = 0; i < REQUESTS; i++)
	{
		if (host_main_thread_run("add_on_main", add_on_main, worker))
			worker->result = -1;
	}
	worker->done = 1;
	return NULL;
}

/* an import as the dispatcher has it, which sets errno */
static void import_with_errno(struct host_call *call)
{
	call->result_x = 7;
	errno = ENOENT;
	call->error = errno;
}

static void *import(void *argument)
{
	struct worker *worker = argument;
	struct host_call call;

	memset(&call, 0, sizeof(call));
	host_saved_errno = 0;
	host_call_on_main_thread(&call, "import_with_errno", import_with_errno);
	worker->result = (int)call.result_x;
	worker->sum = host_saved_errno;
	worker->done = 1;
	return NULL;
}

static void run_main_loop_until(volatile int *done)
{
	int turns;

	for (turns = 0; !*done && turns < 1000; turns++)
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, true);
	CHECK(*done);
}

int main(void)
{
	struct worker workers[WORKERS], waiting;
	int i, turns;

	/* before halo_host_use_main_queue, a request runs where it is made */
	memset(&waiting, 0, sizeof(waiting));
	pthread_create(&waiting.thread, NULL, requests, &waiting);
	pthread_join(waiting.thread, NULL);
	CHECK(waiting.sum == REQUESTS && waiting.on_main == 0 && waiting.result == 0);

	/* from then on, on the main thread, while it runs its run loop */
	halo_host_use_main_queue();
	memset(workers, 0, sizeof(workers));
	for (i = 0; i < WORKERS; i++)
		CHECK(pthread_create(&workers[i].thread, NULL, requests, &workers[i]) == 0);
	for (i = 0; i < WORKERS; i++)
	{
		run_main_loop_until(&workers[i].done);
		pthread_join(workers[i].thread, NULL);
		CHECK(workers[i].sum == REQUESTS && workers[i].on_main == REQUESTS && workers[i].result == 0);
	}
	/* the main thread's own request runs at once */
	memset(&waiting, 0, sizeof(waiting));
	CHECK(host_main_thread_run("add_on_main", add_on_main, &waiting) == 0 && waiting.on_main == 1);

	/* an import's result and errno come back to the thread that asked */
	memset(&waiting, 0, sizeof(waiting));
	pthread_create(&waiting.thread, NULL, import, &waiting);
	run_main_loop_until(&waiting.done);
	pthread_join(waiting.thread, NULL);
	CHECK(waiting.result == 7 && waiting.sum == ENOENT);

	/* a request waiting as the host shuts down fails, and is not run */
	memset(&waiting, 0, sizeof(waiting));
	pthread_create(&waiting.thread, NULL, requests, &waiting);
	for (turns = 0; host_main_thread_queued() == 0 && turns < 1000; turns++)
		usleep(1000);
	CHECK(host_main_thread_queued() == 1);
	halo_host_shutdown();
	pthread_join(waiting.thread, NULL);
	CHECK(waiting.done && waiting.result == -1 && waiting.sum == 0);
	CHECK(host_main_thread_queued() == 0);
	/* (the main queue's drain, still posted, finds nothing) */
	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
	CHECK(waiting.sum == 0);

	/* an import asked for later fails at once, with ECANCELED */
	memset(&waiting, 0, sizeof(waiting));
	pthread_create(&waiting.thread, NULL, import, &waiting);
	pthread_join(waiting.thread, NULL);
	CHECK(waiting.result == 0 && waiting.sum == ECANCELED);
	printf("ok\n");
	return 0;
}
