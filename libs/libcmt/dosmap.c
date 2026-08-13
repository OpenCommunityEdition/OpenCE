/*
DOSMAP.C

symbols in this file:
	__errno (.text, 0x9)
	___doserrno (.text, 0x9)
	__dosmaperr (.text, 0x73)
	_data_003190c0 (.data, 0x168)
*/

#include <errno.h>
#include <winerror.h>

/* ---------- types */

struct thread_data
{
	unsigned long thread_id;
	unsigned long thread_handle;
	int error_number;
	unsigned long dos_error_number;
};

struct error_entry
{
	unsigned long os_error;
	int error_number;
};

/* ---------- constants */

enum
{
	minimum_access_error = ERROR_WRITE_PROTECT,
	maximum_access_error = ERROR_SHARING_BUFFER_EXCEEDED,
	minimum_execution_error = ERROR_INVALID_STARTING_CODESEG,
	maximum_execution_error = ERROR_INFLOOP_IN_RELOC_CHAIN
};

/* ---------- external code */

struct thread_data * __cdecl _getptd(
	void);

/* ---------- public data */

struct error_entry data_003190c0[] =
{
	{ ERROR_INVALID_FUNCTION, EINVAL },
	{ ERROR_FILE_NOT_FOUND, ENOENT },
	{ ERROR_PATH_NOT_FOUND, ENOENT },
	{ ERROR_TOO_MANY_OPEN_FILES, EMFILE },
	{ ERROR_ACCESS_DENIED, EACCES },
	{ ERROR_INVALID_HANDLE, EBADF },
	{ ERROR_ARENA_TRASHED, ENOMEM },
	{ ERROR_NOT_ENOUGH_MEMORY, ENOMEM },
	{ ERROR_INVALID_BLOCK, ENOMEM },
	{ ERROR_BAD_ENVIRONMENT, E2BIG },
	{ ERROR_BAD_FORMAT, ENOEXEC },
	{ ERROR_INVALID_ACCESS, EINVAL },
	{ ERROR_INVALID_DATA, EINVAL },
	{ ERROR_INVALID_DRIVE, ENOENT },
	{ ERROR_CURRENT_DIRECTORY, EACCES },
	{ ERROR_NOT_SAME_DEVICE, EXDEV },
	{ ERROR_NO_MORE_FILES, ENOENT },
	{ ERROR_LOCK_VIOLATION, EACCES },
	{ ERROR_BAD_NETPATH, ENOENT },
	{ ERROR_NETWORK_ACCESS_DENIED, EACCES },
	{ ERROR_BAD_NET_NAME, ENOENT },
	{ ERROR_FILE_EXISTS, EEXIST },
	{ ERROR_CANNOT_MAKE, EACCES },
	{ ERROR_FAIL_I24, EACCES },
	{ ERROR_INVALID_PARAMETER, EINVAL },
	{ ERROR_NO_PROC_SLOTS, EAGAIN },
	{ ERROR_DRIVE_LOCKED, EACCES },
	{ ERROR_BROKEN_PIPE, EPIPE },
	{ ERROR_DISK_FULL, ENOSPC },
	{ ERROR_INVALID_TARGET_HANDLE, EBADF },
	/* BUG (preserved for exact matching): this duplicates the earlier
	 * ERROR_INVALID_HANDLE entry and therefore can never map to EINVAL.
	 * Microsoft's source labels this row as OS error 124; a corrected table
	 * should use ERROR_INVALID_LEVEL here instead.
	 */
	{ ERROR_INVALID_HANDLE, EINVAL },
	{ ERROR_WAIT_NO_CHILDREN, ECHILD },
	{ ERROR_CHILD_NOT_COMPLETE, ECHILD },
	{ ERROR_DIRECT_ACCESS_HANDLE, EBADF },
	{ ERROR_NEGATIVE_SEEK, EINVAL },
	{ ERROR_SEEK_ON_DEVICE, EACCES },
	{ ERROR_DIR_NOT_EMPTY, ENOTEMPTY },
	{ ERROR_NOT_LOCKED, EACCES },
	{ ERROR_BAD_PATHNAME, ENOENT },
	{ ERROR_MAX_THRDS_REACHED, EAGAIN },
	{ ERROR_LOCK_FAILED, EACCES },
	{ ERROR_ALREADY_EXISTS, EEXIST },
	{ ERROR_FILENAME_EXCED_RANGE, ENOENT },
	{ ERROR_NESTING_NOT_ALLOWED, EAGAIN },
	{ ERROR_NOT_ENOUGH_QUOTA, ENOMEM }
};

/* ---------- public code */

int * __cdecl _errno(
	void)
{
	return &_getptd()->error_number;
}

unsigned long * __cdecl __doserrno(
	void)
{
	return &_getptd()->dos_error_number;
}

void __cdecl _dosmaperr(
	unsigned long os_error)
{
	int index;

	_getptd()->dos_error_number = os_error;

	for (index = 0; index < sizeof(data_003190c0) / sizeof(data_003190c0[0]); index++)
	{
		if (os_error == data_003190c0[index].os_error)
		{
			_getptd()->error_number = data_003190c0[index].error_number;
			return;
		}
	}

	if (os_error >= minimum_access_error && os_error <= maximum_access_error)
		_getptd()->error_number = EACCES;
	else if (os_error >= minimum_execution_error && os_error <= maximum_execution_error)
		_getptd()->error_number = ENOEXEC;
	else
		_getptd()->error_number = EINVAL;
}
