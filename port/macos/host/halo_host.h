/*
HALO_HOST.H

The macOS host's entry point (port/macos/host/host_main.c), which
libhalo_host.a exports to the command-line host and the app.
*/

#ifndef HALO_HOST_H
#define HALO_HOST_H

#include <stddef.h>

/* halo_guest.elf beside the executable, or in ../share/opence */
void halo_host_default_image_path(char *path, size_t size);

/* loads the guest image at image_path, creates the VM and runs the guest's
main with argc and argv (argv[0] is the guest's program name) on the
calling thread. It returns only if the VM cannot be created, with 1; a
guest image that cannot be read or loaded ends the process (71) */
int halo_host_run(const char *image_path, int argc, char **argv);

#endif
