/* Stand-in for port/linux/src/posix.h, for compiling xiso.c on the Mac
without the rest of the port. Only what xiso.c uses (xiso_shim.c). */
#ifndef XISO_MAC_POSIX_H
#define XISO_MAC_POSIX_H

typedef long posix_long;
typedef unsigned long posix_ulong;

int posix_seek(int descriptor, posix_long offset_low, posix_long offset_high, int whence,
	posix_ulong *position_low, posix_ulong *position_high);
int posix_make_directory(const char *path);

#endif
