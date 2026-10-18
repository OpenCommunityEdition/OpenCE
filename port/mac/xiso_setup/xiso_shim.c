/*
XISO_SHIM.C

The three port functions xiso.c calls, on libc, for the Mac setup app. Mac OS
X's off_t is 64 bits, so lseek reaches past 4 GB on Tiger and Leopard, PowerPC
and Intel alike.

Cancelling: xiso.c has no cancel hook, so the shim makes the next seek fail
once the flag is set. xiso.c then stops with a read error, and
xiso_install() reports the cancel instead.
*/

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "platform.h"
#include "posix.h"
#include "xiso_install.h"

static volatile int *cancel_flag;

void xiso_shim_set_cancel(volatile int *flag)
{
	cancel_flag = flag;
}

void platform_log(const char *format, ...)
{
	/* the file names are the player's disc's, and the log is not wanted */
	(void)format;
}

int posix_seek(int descriptor, posix_long offset_low, posix_long offset_high, int whence,
	posix_ulong *position_low, posix_ulong *position_high)
{
	off_t offset, result;

	if (cancel_flag && *cancel_flag)
		return -1;
	offset = (off_t)(((unsigned long long)(unsigned long)offset_high << 32) | (unsigned long)offset_low);
	result = lseek(descriptor, offset, whence);
	if (result < 0)
		return -1;
	if (position_low)
		*position_low = (posix_ulong)((unsigned long long)result & 0xFFFFFFFFu);
	if (position_high)
		*position_high = (posix_ulong)((unsigned long long)result >> 32);
	return 0;
}

int posix_make_directory(const char *path)
{
	return mkdir(path, 0755);
}
