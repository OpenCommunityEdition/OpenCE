/*
HALO_WINDOWS_FILE_NAMES.H

The game opens some files through the C runtime with Xbox paths (d:\...,
z:\...), which on Windows would name real drives. Its calls go to wrappers
that translate the path the way CreateFile does (port/windows/src/
windows_crt.c). Every header declaring these functions is included first, so
the Windows declarations are never renamed.
*/

#ifndef __HALO_WINDOWS_FILE_NAMES_H
#define __HALO_WINDOWS_FILE_NAMES_H

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>

#ifndef HALO_LINUX_PLATFORM_LAYER
FILE *halo_windows_fopen(const char *path, const char *mode);
FILE *halo_windows_freopen(const char *path, const char *mode, FILE *stream);
int halo_windows_remove(const char *path);
int halo_windows_rename(const char *old_path, const char *new_path);
int halo_windows_open(const char *path, int flags, ...);
int halo_windows_unlink(const char *path);
int halo_windows_access(const char *path, int mode);
int halo_windows_mkdir(const char *path);
int halo_windows_rmdir(const char *path);

#define fopen halo_windows_fopen
#define freopen halo_windows_freopen
#define remove halo_windows_remove
#define rename halo_windows_rename
#define open halo_windows_open
#define _open halo_windows_open
#define unlink halo_windows_unlink
#define _unlink halo_windows_unlink
#define access halo_windows_access
#define _access halo_windows_access
#define mkdir halo_windows_mkdir
#define _mkdir halo_windows_mkdir
#define rmdir halo_windows_rmdir
#define _rmdir halo_windows_rmdir
#endif

#endif
