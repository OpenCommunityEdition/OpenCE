/*
WIN32_FILES.C

The file system half of port/linux/src/posix.h for Windows (the Linux
version is posix_files.c). Paths are the platform layer's host paths, in the
code page of the C runtime's narrow functions that open the same files
(xbox_files.c opens them with open()). Failures set errno, which the callers
turn into Xbox error codes.
*/

#include <windows.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "posix.h"

static int fail(void)
{
	switch (GetLastError())
	{
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_DRIVE:
	case ERROR_INVALID_NAME:
	case ERROR_BAD_NETPATH:
		errno = ENOENT;
		break;
	case ERROR_ACCESS_DENIED:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
	case ERROR_WRITE_PROTECT:
		errno = EACCES;
		break;
	case ERROR_ALREADY_EXISTS:
	case ERROR_FILE_EXISTS:
		errno = EEXIST;
		break;
	case ERROR_DIR_NOT_EMPTY:
		errno = ENOTEMPTY;
		break;
	case ERROR_DISK_FULL:
	case ERROR_HANDLE_DISK_FULL:
		errno = ENOSPC;
		break;
	case ERROR_INVALID_HANDLE:
		errno = EBADF;
		break;
	default:
		errno = EIO;
		break;
	}
	return -1;
}

static void split64(unsigned long long value, posix_ulong *low, posix_ulong *high)
{
	*low = (posix_ulong)(value & 0xffffffffULL);
	*high = (posix_ulong)(value >> 32);
}

/* 100 ns intervals since 1601 <-> seconds and nanoseconds since 1970 */
#define UNIX_EPOCH_INTERVALS 116444736000000000ULL

static void split_time(const FILETIME *time, posix_ulong *seconds, posix_ulong *nanoseconds)
{
	unsigned long long intervals = ((unsigned long long)time->dwHighDateTime << 32) | time->dwLowDateTime;

	if (intervals < UNIX_EPOCH_INTERVALS)
	{
		*seconds = 0;
		*nanoseconds = 0;
		return;
	}
	intervals -= UNIX_EPOCH_INTERVALS;
	*seconds = (posix_ulong)(intervals / 10000000ULL);
	*nanoseconds = (posix_ulong)(intervals % 10000000ULL) * 100;
}

static FILETIME join_time(posix_ulong seconds, posix_ulong nanoseconds)
{
	unsigned long long intervals = (unsigned long long)seconds * 10000000ULL + nanoseconds / 100 + UNIX_EPOCH_INTERVALS;
	FILETIME time;

	time.dwLowDateTime = (DWORD)intervals;
	time.dwHighDateTime = (DWORD)(intervals >> 32);
	return time;
}

static void fill_information(DWORD attributes, DWORD size_low, DWORD size_high,
	const FILETIME *creation, const FILETIME *access, const FILETIME *modification,
	struct posix_file_information *information)
{
	memset(information, 0, sizeof(*information));
	if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		information->flags |= _posix_file_is_directory;
	if (attributes & FILE_ATTRIBUTE_READONLY)
		information->flags |= _posix_file_is_read_only;
	information->size_low = size_low;
	information->size_high = size_high;
	split_time(modification, &information->modification_seconds, &information->modification_nanoseconds);
	split_time(access, &information->access_seconds, &information->access_nanoseconds);
	split_time(creation, &information->creation_seconds, &information->creation_nanoseconds);
}

int posix_stat(const char *path, struct posix_file_information *information)
{
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
		return fail();
	fill_information(data.dwFileAttributes, data.nFileSizeLow, data.nFileSizeHigh,
		&data.ftCreationTime, &data.ftLastAccessTime, &data.ftLastWriteTime, information);
	return 0;
}

int posix_fstat(int descriptor, struct posix_file_information *information)
{
	HANDLE handle = (HANDLE)_get_osfhandle(descriptor);
	BY_HANDLE_FILE_INFORMATION data;

	if (handle == INVALID_HANDLE_VALUE)
	{
		errno = EBADF;
		return -1;
	}
	if (!GetFileInformationByHandle(handle, &data))
		return fail();
	fill_information(data.dwFileAttributes, data.nFileSizeLow, data.nFileSizeHigh,
		&data.ftCreationTime, &data.ftLastAccessTime, &data.ftLastWriteTime, information);
	return 0;
}

int posix_set_file_times(const char *path,
	posix_ulong access_seconds, posix_ulong access_nanoseconds,
	posix_ulong modification_seconds, posix_ulong modification_nanoseconds)
{
	HANDLE handle = CreateFileA(path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	FILETIME access, modification;
	BOOL result;

	if (handle == INVALID_HANDLE_VALUE)
		return fail();
	access = join_time(access_seconds, access_nanoseconds);
	modification = join_time(modification_seconds, modification_nanoseconds);
	/* a zero time leaves that time alone, as UTIME_OMIT does on Linux */
	result = SetFileTime(handle, NULL, access_seconds ? &access : NULL, modification_seconds ? &modification : NULL);
	CloseHandle(handle);
	return result ? 0 : fail();
}

int posix_seek(int descriptor, posix_long offset_low, posix_long offset_high, int whence,
	posix_ulong *position_low, posix_ulong *position_high)
{
	long long offset = (long long)(((unsigned long long)(posix_ulong)offset_high << 32) | (posix_ulong)offset_low);
	long long result = _lseeki64(descriptor, offset, whence);

	if (result < 0)
		return -1;
	split64((unsigned long long)result, position_low, position_high);
	return 0;
}

int posix_truncate(int descriptor, posix_ulong size_low, posix_ulong size_high)
{
	errno_t error = _chsize_s(descriptor, (long long)(((unsigned long long)size_high << 32) | size_low));

	if (error)
	{
		errno = error;
		return -1;
	}
	return 0;
}

int posix_disk_space(const char *path,
	posix_ulong *free_low, posix_ulong *free_high,
	posix_ulong *total_low, posix_ulong *total_high)
{
	ULARGE_INTEGER available, total;

	if (!GetDiskFreeSpaceExA(path, &available, &total, NULL))
		return fail();
	split64(available.QuadPart, free_low, free_high);
	split64(total.QuadPart, total_low, total_high);
	return 0;
}

int posix_set_read_only(const char *path, int read_only)
{
	DWORD attributes = GetFileAttributesA(path);

	if (attributes == INVALID_FILE_ATTRIBUTES)
		return fail();
	attributes = read_only ? (attributes | FILE_ATTRIBUTE_READONLY) : (attributes & ~FILE_ATTRIBUTE_READONLY);
	return SetFileAttributesA(path, attributes) ? 0 : fail();
}

int posix_make_directory(const char *path)
{
	return CreateDirectoryA(path, NULL) ? 0 : fail();
}

/* ---------- directories */

struct directory
{
	HANDLE find;
	WIN32_FIND_DATAA data;
	BOOL pending; /* data holds an entry not returned yet */
};

void *posix_directory_open(const char *path)
{
	struct directory *directory = malloc(sizeof(*directory));
	char pattern[MAX_PATH + 3];
	size_t length = strlen(path);

	if (!directory || length + 3 > sizeof(pattern))
	{
		free(directory);
		errno = ENOMEM;
		return NULL;
	}
	memcpy(pattern, path, length);
	if (length && pattern[length - 1] != '\\' && pattern[length - 1] != '/')
		pattern[length++] = '\\';
	pattern[length++] = '*';
	pattern[length] = 0;
	directory->find = FindFirstFileA(pattern, &directory->data);
	if (directory->find == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();

		free(directory);
		if (error == ERROR_FILE_NOT_FOUND)
		{
			/* an empty directory (drive roots have no . entry) */
			directory = malloc(sizeof(*directory));
			if (!directory)
				return NULL;
			directory->find = INVALID_HANDLE_VALUE;
			directory->pending = FALSE;
			return directory;
		}
		SetLastError(error);
		fail();
		return NULL;
	}
	directory->pending = TRUE;
	return directory;
}

int posix_directory_next(void *handle, char *name, posix_ulong name_size)
{
	struct directory *directory = handle;

	if (!directory || directory->find == INVALID_HANDLE_VALUE)
		return 0;
	for (;;)
	{
		if (!directory->pending && !FindNextFileA(directory->find, &directory->data))
			return 0;
		directory->pending = FALSE;
		if (!strcmp(directory->data.cFileName, ".") || !strcmp(directory->data.cFileName, ".."))
			continue;
		if (strlen(directory->data.cFileName) + 1 > name_size)
			continue;
		strcpy(name, directory->data.cFileName);
		return 1;
	}
}

void posix_directory_close(void *handle)
{
	struct directory *directory = handle;

	if (!directory)
		return;
	if (directory->find != INVALID_HANDLE_VALUE)
		FindClose(directory->find);
	free(directory);
}

int posix_find_entry_case_insensitive(const char *directory, const char *name,
	char *result, posix_ulong result_size)
{
	/* Windows file systems already ignore case; looking the name up gives
	its on-disk spelling */
	char path[MAX_PATH];
	WIN32_FIND_DATAA data;
	HANDLE find;
	int written;

	if (strpbrk(name, "*?"))
		return 0;
	written = *directory ?
		_snprintf(path, sizeof(path), "%s\\%s", directory, name) :
		_snprintf(path, sizeof(path), "%s", name);
	if (written < 0 || written >= (int)sizeof(path))
		return 0;
	find = FindFirstFileA(path, &data);
	if (find == INVALID_HANDLE_VALUE)
		return 0;
	FindClose(find);
	if (strlen(data.cFileName) + 1 > result_size)
		return 0;
	strcpy(result, data.cFileName);
	return 1;
}
