/*
HOST_MAIN_THREAD.C

The imports AppKit needs on the main thread (windows, events, message
boxes, the clipboard: tools/macos_dispatch.py's MAIN_THREAD), carried out
there for the vCPU threads that ask for them.

Once the app (or the command-line host's main-loop mode) has called
halo_host_use_main_queue, a request from another thread is queued, the main
dispatch queue is asked to run it, and the asking thread waits for it to be
done. AppKit's run loop drains the main queue, so the request runs between
its events. Before then, and on the main thread itself, a request runs where
it is asked. After halo_host_shutdown, queued and new requests fail at once
(ECANCELED) instead of waiting for a run loop that has stopped.
*/

#include "host.h"
#include "halo_host.h"

#include <dispatch/dispatch.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

struct host_main_request
{
	void (*work)(void *context);
	void *context;
	const char *name;
	/* 0 queued, 1 done, -1 failed */
	int state;
	struct host_main_request *next;
};

static pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t main_changed = PTHREAD_COND_INITIALIZER;
static struct host_main_request *main_queue;
static struct host_main_request **main_queue_tail = &main_queue;
static int main_queued;
static int main_shut_down;
static atomic_int main_posting;

/* ---------- HALO_HOST_TRACE_MAIN_THREAD: each import's first run there */

#define TRACED_NAMES 64

static void main_thread_trace(const char *name)
{
	static const char *traced[TRACED_NAMES];
	static int count = -1;
	int i;

	if (!name)
		return;
	if (count < 0)
		count = getenv("HALO_HOST_TRACE_MAIN_THREAD") ? 0 : TRACED_NAMES;
	for (i = 0; i < count; i++)
	{
		if (traced[i] == name)
			return;
	}
	if (count < TRACED_NAMES)
	{
		traced[count++] = name;
		/* (asked for, as the import counts are: not kept quiet) */
		fprintf(stderr, "halo_host: %s ran on the main thread\n", name);
	}
}

/* ---------- the queue */

/* (on the main thread, from the main dispatch queue) */
static void main_thread_drain(void *unused)
{
	struct host_main_request *request;

	(void)unused;
	pthread_mutex_lock(&main_lock);
	while ((request = main_queue) != NULL)
	{
		main_queue = request->next;
		if (!main_queue)
			main_queue_tail = &main_queue;
		main_queued--;
		pthread_mutex_unlock(&main_lock);
		main_thread_trace(request->name);
		request->work(request->context);
		pthread_mutex_lock(&main_lock);
		request->state = 1;
		pthread_cond_broadcast(&main_changed);
	}
	pthread_mutex_unlock(&main_lock);
}

void halo_host_use_main_queue(void)
{
	atomic_store(&main_posting, 1);
}

void halo_host_shutdown(void)
{
	struct host_main_request *request;

	pthread_mutex_lock(&main_lock);
	main_shut_down = 1;
	while ((request = main_queue) != NULL)
	{
		main_queue = request->next;
		request->state = -1;
	}
	main_queue_tail = &main_queue;
	main_queued = 0;
	pthread_cond_broadcast(&main_changed);
	pthread_mutex_unlock(&main_lock);
}

int host_main_thread_run(const char *name, void (*work)(void *context), void *context)
{
	struct host_main_request request;
	int state;

	if (!atomic_load(&main_posting) || pthread_main_np())
	{
		work(context);
		return 0;
	}
	request.work = work;
	request.context = context;
	request.name = name;
	request.state = 0;
	request.next = NULL;
	pthread_mutex_lock(&main_lock);
	if (main_shut_down)
	{
		pthread_mutex_unlock(&main_lock);
		return ECANCELED;
	}
	*main_queue_tail = &request;
	main_queue_tail = &request.next;
	main_queued++;
	dispatch_async_f(dispatch_get_main_queue(), NULL, main_thread_drain);
	while (request.state == 0)
		pthread_cond_wait(&main_changed, &main_lock);
	state = request.state;
	pthread_mutex_unlock(&main_lock);
	return state > 0 ? 0 : ECANCELED;
}

int host_main_thread_queued(void)
{
	int queued;

	pthread_mutex_lock(&main_lock);
	queued = main_queued;
	pthread_mutex_unlock(&main_lock);
	return queued;
}

/* ---------- the dispatcher's imports */

struct host_main_import
{
	struct host_call *call;
	void (*work)(struct host_call *call);
};

static void main_thread_import(void *context)
{
	struct host_main_import *import = context;

	import->work(import->call);
}

void host_call_on_main_thread(struct host_call *call, const char *name, void (*work)(struct host_call *call))
{
	struct host_main_import import;

	import.call = call;
	import.work = work;
	call->error = 0;
	if (host_main_thread_run(name, main_thread_import, &import))
	{
		call->result_x = 0;
		call->result_d = 0;
		call->error = ECANCELED;
	}
	/* (errno is the thread's own: the main thread's is brought back) */
	host_saved_errno = call->error;
}
