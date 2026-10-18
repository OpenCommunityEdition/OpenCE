/* Stand-in for port/linux/src/platform.h, for compiling xiso.c on the Mac
without the rest of the port. Only what xiso.c uses. */
#ifndef XISO_MAC_PLATFORM_H
#define XISO_MAC_PLATFORM_H

void platform_log(const char *format, ...);

#endif
