/*
XISO.H

The maps folder out of an Xbox disc image (xiso.c).
*/

#ifndef __HALO_LINUX_XISO_H
#define __HALO_LINUX_XISO_H

/* called as the copy goes: the file being copied, and the bytes copied of
all the files' */
typedef void (*xiso_progress_proc)(void *context, const char *file, unsigned long long done,
	unsigned long long total);

/* what probing an image found: whether it is a Halo disc, and how whole it
is (xiso_probe) */
struct xiso_probe_result
{
	int halo;
	int map_count;
	unsigned long long data_size;
	unsigned long long file_size;
	int complete;
};

/* reads an image far enough to tell a Halo disc from another ROM, and
whether every map it names lies inside the file, without copying anything.
Returns nonzero if the image was readable, when result->halo says whether it
is Halo; 0 if it could not be read. Called from any thread */
int xiso_probe(const char *image_path, struct xiso_probe_result *result);

/* copies the image's maps folder to <destination>/maps; returns nonzero on
success, or 0 with the reason (for the player) in error. Called from any
thread */
int xiso_extract_maps(const char *image_path, const char *destination, xiso_progress_proc progress, void *context,
	char *error, int error_size);

#endif
