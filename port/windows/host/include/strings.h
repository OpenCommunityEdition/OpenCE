/*
STRINGS.H

strcasecmp for the files the Windows x64 host shares with the Linux and
Android hosts (host_common.c).
*/

#ifndef WIN64_HOST_STRINGS_H
#define WIN64_HOST_STRINGS_H

#include <string.h>

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#endif
