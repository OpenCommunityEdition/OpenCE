/*
XISO_INSTALL.H

OpenCE Setup's game data step: the maps folder out of the player's Xbox
Halo disc image, checked, into <data root>/maps.
*/
#ifndef XISO_INSTALL_H
#define XISO_INSTALL_H

/* (as xiso_progress_proc in port/linux/src/xiso.h) */
typedef void (*xiso_install_progress)(void *context, const char *file, unsigned long long done,
	unsigned long long total);

/* the cancel flag the shim watches (xiso_shim.c) */
void xiso_shim_set_cancel(volatile int *flag);

/* whether <maps_folder> holds the Xbox maps this port needs: every .map in
it an Xbox cache (version 5) and the required ones present. Returns nonzero
if so, else 0 with the reason in error. */
int xiso_verify_maps(const char *maps_folder, char *error, int error_size);

/* copies the image's maps to <data_root>/maps through a temporary folder
beside it and verifies them before the final rename, so a maps folder there
is always whole. Sets *cancelled if the flag stopped it. Returns nonzero on
success, else 0 with the reason for the player in error. */
int xiso_install(const char *image_path, const char *data_root, xiso_install_progress progress, void *context,
	volatile int *cancel, int *cancelled, char *error, int error_size);

#endif
