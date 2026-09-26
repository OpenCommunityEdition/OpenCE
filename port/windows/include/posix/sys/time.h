/* POSIX <sys/time.h>: port/linux/src/platform.h includes it ahead of the Xbox
SDK's Winsock, whose names it would otherwise clash with. Windows' C runtime
has no BSD sockets, so there is nothing to declare. */
