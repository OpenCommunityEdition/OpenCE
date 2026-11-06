/*
UNISTD.H

The few POSIX names of unistd.h the files the Windows x64 host shares with
the Linux and Android hosts use (host_common.c), from the C runtime.
*/

#ifndef WIN64_HOST_UNISTD_H
#define WIN64_HOST_UNISTD_H

#include <io.h>
#include <process.h>
#include <stdlib.h>

#define write(fd, buffer, size) _write((fd), (buffer), (unsigned int)(size))
#define getpagesize() 4096

#endif
