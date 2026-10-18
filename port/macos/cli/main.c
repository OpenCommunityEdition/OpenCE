/*
MAIN.C

The command-line macOS host, build/macos/halo_host: libhalo_host.a's
front end (docs/macos_host.md).

  halo_host [image] [arguments]

The image defaults to halo_guest.elf beside the host, or in
../share/opence. The guest's arguments are from the image's path on, if
one was given, which so is its argv[0].
*/

#include "halo_host.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	char image_path[1100];

	if (argc > 1)
		snprintf(image_path, sizeof(image_path), "%s", argv[1]);
	else
		halo_host_default_image_path(image_path, sizeof(image_path));
	return halo_host_run(image_path, argc > 1 ? argc - 1 : argc, argc > 1 ? argv + 1 : argv);
}
