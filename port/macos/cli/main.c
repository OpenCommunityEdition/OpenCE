/*
MAIN.C

The command-line macOS host, build/macos/halo_host: libhalo_host.a's
front end (docs/macos_host.md).

  halo_host [image] [arguments]

The image defaults to halo_guest.elf beside the host, or in
../share/opence. The guest's arguments are from the image's path on, if
one was given, which so is its argv[0].

With HALO_HOST_MAIN_LOOP=1 the guest runs on a thread of its own and the
main thread runs the main dispatch queue, as the app's does: the imports
that need the main thread are carried out there (host_main_thread.c).
*/

#include "halo_host.h"

#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct guest_start
{
	char image_path[1100];
	int argc;
	char **argv;
};

static void *run_guest(void *argument)
{
	struct guest_start *start = argument;

	/* (halo_host_run returns only if the VM cannot be made) */
	exit(halo_host_run(start->image_path, start->argc, start->argv));
}

int main(int argc, char **argv)
{
	static struct guest_start start;
	const char *main_loop = getenv("HALO_HOST_MAIN_LOOP");
	pthread_attr_t attributes;
	pthread_t thread;

	if (argc > 1)
		snprintf(start.image_path, sizeof(start.image_path), "%s", argv[1]);
	else
		halo_host_default_image_path(start.image_path, sizeof(start.image_path));
	start.argc = argc > 1 ? argc - 1 : argc;
	start.argv = argc > 1 ? argv + 1 : argv;
	if (!main_loop || strcmp(main_loop, "1"))
		return halo_host_run(start.image_path, start.argc, start.argv);
	halo_host_use_main_queue();
	/* (the guest's main thread's stack is the host thread's) */
	pthread_attr_init(&attributes);
	pthread_attr_setstacksize(&attributes, 8 << 20);
	if (pthread_create(&thread, &attributes, run_guest, &start))
	{
		fprintf(stderr, "halo_host: cannot start the guest's thread\n");
		return 1;
	}
	dispatch_main();
}
