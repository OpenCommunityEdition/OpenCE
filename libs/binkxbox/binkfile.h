#ifndef BINK_FILE_H
#define BINK_FILE_H
#include "binkio.h"

enum
{
	BINK_FILE_PAGE_SIZE= 4096,
	BINK_FILE_HANDLE= 0x00800000
};
void LockedAddFunc(
	unsigned long *value,
	unsigned long amount);
long __stdcall BinkFileOpen(
	BINKIO *io,
	const char *name,
	unsigned long flags);
#endif
