/*
WIN64_SYSCALL.C

System calls on behalf of the guest's musl runtime, on Windows: what the
Linux host's host_syscall.c does with Linux's, for the guest's own system
call interface (port/android/include/halo_guest_syscall.h).

The guest is the 64-bit x86 Linux build's image, so it asks for Linux's
calls with Linux's structures and errno values; this answers the ones the
game makes (files, directories, memory, futexes, clocks, sleeping,
randomness, the process), with -ENOSYS for the rest, as the Linux host does
for the calls it does not perform. Sockets, the dedicated server's console,
UPnP and the rest of posix.h are host imports, the 32-bit Windows build's
port/windows/src/win32_*.c.

Files:

- a guest file descriptor is the C runtime's (_open_osfhandle), so that
  posix.h's functions in win32_files.c, which take descriptors, see the
  same files; the C runtime's 0, 1 and 2 always exist (win64_main.c), and
  the guest's writes to 1 and 2 go to the host's log;
- files are opened with every sharing mode, as POSIX files are shared, and
  O_APPEND opens a file for appending only, which Windows then does itself;
- paths are UTF-8 (host_path_in), and forward slashes are Windows' too; the
  guest gets paths in that form back (host_path_out: getcwd, and
  /proc/self/exe, which the game reads to find its folder);
- a directory is opened as a file too, and getdents64 lists it with
  GetFileInformationByHandleEx.
*/

#include "win64_host.h"
#include "halo_guest_syscall.h"

#include <bcrypt.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GUEST_EIO 5
#define GUEST_ENOTTY 25
#define GUEST_ESPIPE 29
#define GUEST_EXDEV 18
/* EMFILE: the C runtime's descriptor table is full */
#define GUEST_EMFILE_OR_NOMEM 24

#define GUEST(type, value) ((type)(uintptr_t)(uint32_t)(value))

void host_console_write(int fd, const char *bytes, size_t size);

/* ---------- errors and paths */

int host_errno_of_windows(DWORD error)
{
	switch (error)
	{
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_NAME:
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_PATHNAME:
	case ERROR_INVALID_DRIVE:
	case ERROR_NOT_READY:
		return GUEST_ENOENT;
	case ERROR_ACCESS_DENIED:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
	case ERROR_WRITE_PROTECT:
	case ERROR_CURRENT_DIRECTORY:
		return GUEST_EACCES;
	case ERROR_FILE_EXISTS:
	case ERROR_ALREADY_EXISTS:
		return GUEST_EEXIST;
	case ERROR_DIR_NOT_EMPTY:
		return GUEST_ENOTEMPTY;
	case ERROR_INVALID_HANDLE:
		return GUEST_EBADF;
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return GUEST_ENOMEM;
	case ERROR_DISK_FULL:
	case ERROR_HANDLE_DISK_FULL:
		return GUEST_ENOSPC;
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_BUFFER_OVERFLOW:
		return GUEST_ENAMETOOLONG;
	case ERROR_DIRECTORY:
		return GUEST_ENOTDIR;
	case ERROR_NOT_SAME_DEVICE:
		return GUEST_EXDEV;
	case ERROR_NEGATIVE_SEEK:
	case ERROR_INVALID_PARAMETER:
		return GUEST_EINVAL;
	case ERROR_NOT_SUPPORTED:
		return GUEST_EOPNOTSUPP;
	case ERROR_LOCK_FAILED:
	case ERROR_IO_PENDING:
		return GUEST_EAGAIN;
	case ERROR_TIMEOUT:
		return GUEST_ETIMEDOUT;
	}
	return GUEST_EIO;
}

static long windows_failure(void)
{
	return -host_errno_of_windows(GetLastError());
}

wchar_t *host_path_in(const char *path, int *error)
{
	int length;
	wchar_t *result;

	if (!path)
	{
		*error = GUEST_EFAULT;
		return NULL;
	}
	if (!strcmp(path, "/dev/null"))
		path = "NUL";
	length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
	if (length <= 0)
	{
		*error = GUEST_ENOENT;
		return NULL;
	}
	if (length > 32767)
	{
		*error = GUEST_ENAMETOOLONG;
		return NULL;
	}
	result = malloc((size_t)length * sizeof(wchar_t));
	if (!result)
	{
		*error = GUEST_ENOMEM;
		return NULL;
	}
	MultiByteToWideChar(CP_UTF8, 0, path, -1, result, length);
	return result;
}

int host_path_out(const wchar_t *path, char *buffer, size_t size)
{
	int length;
	char *cursor;

	/* (the \\?\ prefix of GetFinalPathNameByHandle's) */
	if (!wcsncmp(path, L"\\\\?\\", 4) && path[4] && path[5] == L':')
		path += 4;
	length = WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer, (int)size, NULL, NULL);
	if (length <= 0)
		return -1;
	for (cursor = buffer; *cursor; cursor++)
	{
		if (*cursor == '\\')
			*cursor = '/';
	}
	return length - 1;
}

static int is_absolute(const char *path)
{
	return path[0] == '/' || path[0] == '\\' || (path[0] && path[1] == ':');
}

static HANDLE handle_of(int fd)
{
	intptr_t handle = fd < 0 ? -1 : _get_osfhandle(fd);

	return handle == -1 || handle == -2 ? INVALID_HANDLE_VALUE : (HANDLE)handle;
}

/* the path for openat and the other *at calls: AT_FDCWD's, or one inside
the directory fd names */
static wchar_t *at_path(int directory, uint64_t address, int *error)
{
	const char *path = GUEST(const char *, address);
	wchar_t *relative, *full;
	wchar_t base[32768];
	DWORD length;
	HANDLE handle;

	if (!address)
	{
		*error = GUEST_EFAULT;
		return NULL;
	}
	if (directory == GUEST_AT_FDCWD || is_absolute(path))
		return host_path_in(path, error);
	handle = handle_of(directory);
	if (handle == INVALID_HANDLE_VALUE)
	{
		*error = GUEST_EBADF;
		return NULL;
	}
	length = GetFinalPathNameByHandleW(handle, base, (DWORD)(sizeof(base) / sizeof(base[0])), FILE_NAME_NORMALIZED);
	if (!length || length >= sizeof(base) / sizeof(base[0]))
	{
		*error = GUEST_ENOTDIR;
		return NULL;
	}
	relative = host_path_in(path, error);
	if (!relative)
		return NULL;
	full = malloc((length + wcslen(relative) + 2) * sizeof(wchar_t));
	if (!full)
	{
		free(relative);
		*error = GUEST_ENOMEM;
		return NULL;
	}
	swprintf(full, length + wcslen(relative) + 2, L"%ls\\%ls", base, relative);
	free(relative);
	return full;
}

/* ---------- each descriptor's open flags and directory listing */

#define DESCRIPTOR_COUNT 8192

struct listing
{
	uint8_t buffer[0x10000];
	uint32_t used, offset;
	int64_t position;
	int started, finished;
};

static uint32_t descriptor_flags[DESCRIPTOR_COUNT];
static struct listing *listings[DESCRIPTOR_COUNT];
static SRWLOCK descriptor_lock = SRWLOCK_INIT;

static void descriptor_forget(int fd)
{
	if (fd < 0 || fd >= DESCRIPTOR_COUNT)
		return;
	AcquireSRWLockExclusive(&descriptor_lock);
	free(listings[fd]);
	listings[fd] = NULL;
	descriptor_flags[fd] = 0;
	ReleaseSRWLockExclusive(&descriptor_lock);
}

/* ---------- times */

/* 100-nanosecond intervals between 1601 and 1970 */
#define UNIX_EPOCH_FILETIME 116444736000000000ULL

static int64_t filetime_nanoseconds(const FILETIME *time)
{
	uint64_t value = ((uint64_t)time->dwHighDateTime << 32) | time->dwLowDateTime;

	return ((int64_t)value - (int64_t)UNIX_EPOCH_FILETIME) * 100;
}

static FILETIME nanoseconds_filetime(int64_t nanoseconds)
{
	uint64_t value = (uint64_t)(nanoseconds / 100 + (int64_t)UNIX_EPOCH_FILETIME);
	FILETIME time;

	time.dwLowDateTime = (DWORD)value;
	time.dwHighDateTime = (DWORD)(value >> 32);
	return time;
}

static int64_t realtime_nanoseconds(void)
{
	FILETIME now;

	GetSystemTimePreciseAsFileTime(&now);
	return filetime_nanoseconds(&now);
}

static int64_t monotonic_nanoseconds(void)
{
	static LARGE_INTEGER frequency;
	LARGE_INTEGER counter;

	if (!frequency.QuadPart)
		QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&counter);
	return (int64_t)((counter.QuadPart / frequency.QuadPart) * 1000000000LL +
		(counter.QuadPart % frequency.QuadPart) * 1000000000LL / frequency.QuadPart);
}

static int64_t cpu_nanoseconds(int thread)
{
	FILETIME creation, exit, kernel, user;
	BOOL ok = thread ? GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user) :
		GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);

	if (!ok)
		return 0;
	return (int64_t)((((uint64_t)kernel.dwHighDateTime << 32) | kernel.dwLowDateTime) +
		(((uint64_t)user.dwHighDateTime << 32) | user.dwLowDateTime)) * 100;
}

/* Linux's clock numbers */
static int clock_now(int clock, int64_t *nanoseconds)
{
	switch (clock)
	{
	case 0: /* CLOCK_REALTIME */
	case 5: /* CLOCK_REALTIME_COARSE */
	case 8: /* CLOCK_REALTIME_ALARM */
	case 11: /* CLOCK_TAI */
		*nanoseconds = realtime_nanoseconds();
		return 0;
	case 1: /* CLOCK_MONOTONIC */
	case 4: /* CLOCK_MONOTONIC_RAW */
	case 6: /* CLOCK_MONOTONIC_COARSE */
	case 7: /* CLOCK_BOOTTIME */
	case 9: /* CLOCK_BOOTTIME_ALARM */
		*nanoseconds = monotonic_nanoseconds();
		return 0;
	case 2: /* CLOCK_PROCESS_CPUTIME_ID */
		*nanoseconds = cpu_nanoseconds(0);
		return 0;
	case 3: /* CLOCK_THREAD_CPUTIME_ID */
		*nanoseconds = cpu_nanoseconds(1);
		return 0;
	}
	return -GUEST_EINVAL;
}

static int timespec_in(uint64_t address, int64_t *nanoseconds)
{
	const struct halo_guest_timespec *value = GUEST(const struct halo_guest_timespec *, address);

	if (!address)
		return 0;
	*nanoseconds = (int64_t)value->seconds * 1000000000LL + value->nanoseconds;
	return 1;
}

static void timespec_out(uint64_t address, int64_t nanoseconds)
{
	struct halo_guest_timespec *result = GUEST(struct halo_guest_timespec *, address);

	if (!address)
		return;
	result->seconds = (int32_t)(nanoseconds / 1000000000LL);
	result->nanoseconds = (int32_t)(nanoseconds % 1000000000LL);
}

/* sleeps for a relative time, with a high-resolution waitable timer where
Windows has one (Windows 10 1803 and later) */
static void sleep_nanoseconds(int64_t nanoseconds)
{
	HANDLE timer;
	LARGE_INTEGER due;

	if (nanoseconds <= 0)
	{
		SwitchToThread();
		return;
	}
	timer = host_thread_timer();
	due.QuadPart = -(nanoseconds + 99) / 100;
	if (timer && SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE))
	{
		WaitForSingleObject(timer, INFINITE);
		return;
	}
	Sleep((DWORD)((nanoseconds + 999999) / 1000000));
}

/* ---------- files */

/* GetFileInformationByHandleEx's FILE_FULL_DIR_INFO (FileFullDirectoryInfo
and FileFullDirectoryRestartInfo), which not every SDK's headers have */
struct full_directory_entry
{
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime, LastAccessTime, LastWriteTime, ChangeTime, EndOfFile, AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	ULONG EaSize;
	WCHAR FileName[1];
};

#define FULL_DIRECTORY_INFO ((FILE_INFO_BY_HANDLE_CLASS)14)
#define FULL_DIRECTORY_RESTART_INFO ((FILE_INFO_BY_HANDLE_CLASS)15)

struct halo_guest_dirent64_header
{
	uint64_t d_ino;
	int64_t d_off;
	uint16_t d_reclen;
	uint8_t d_type;
};

static void kstat_from_information(const BY_HANDLE_FILE_INFORMATION *information, DWORD type,
	struct halo_guest_kstat *result)
{
	uint32_t mode;

	memset(result, 0, sizeof(*result));
	if (type == FILE_TYPE_CHAR)
		mode = 0020000 | 0666;
	else if (type == FILE_TYPE_PIPE)
		mode = 0010000 | 0600;
	else if (information->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		mode = 0040000 | 0755;
	else
		mode = 0100000 | ((information->dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? 0444 : 0644);
	result->st_mode = mode;
	result->st_nlink = information->nNumberOfLinks ? information->nNumberOfLinks : 1;
	result->st_uid = 1000;
	result->st_gid = 1000;
	result->st_dev = information->dwVolumeSerialNumber;
	result->st_ino = ((uint64_t)information->nFileIndexHigh << 32) | information->nFileIndexLow;
	if (type == FILE_TYPE_DISK)
	{
		int64_t accessed = filetime_nanoseconds(&information->ftLastAccessTime);
		int64_t modified = filetime_nanoseconds(&information->ftLastWriteTime);

		result->st_size = (int64_t)(((uint64_t)information->nFileSizeHigh << 32) | information->nFileSizeLow);
		result->atime_seconds = accessed / 1000000000LL;
		result->atime_nanoseconds = accessed % 1000000000LL;
		result->mtime_seconds = modified / 1000000000LL;
		result->mtime_nanoseconds = modified % 1000000000LL;
		result->ctime_seconds = result->mtime_seconds;
		result->ctime_nanoseconds = result->mtime_nanoseconds;
	}
	result->st_blksize = 4096;
	result->st_blocks = (result->st_size + 511) / 512;
}

static long fstat_descriptor(int fd, uint64_t address)
{
	BY_HANDLE_FILE_INFORMATION information;
	HANDLE handle = handle_of(fd);
	DWORD type;

	if (!address)
		return -GUEST_EFAULT;
	if (handle == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	type = GetFileType(handle);
	memset(&information, 0, sizeof(information));
	if (type == FILE_TYPE_DISK && !GetFileInformationByHandle(handle, &information))
		return windows_failure();
	kstat_from_information(&information, type, GUEST(struct halo_guest_kstat *, address));
	return 0;
}

static long fstat_path(int directory, uint64_t path_address, uint64_t address, int flags)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	BY_HANDLE_FILE_INFORMATION information;
	wchar_t *path;
	int error;

	if (!address)
		return -GUEST_EFAULT;
	if ((flags & GUEST_AT_EMPTY_PATH) && path_address && !*GUEST(const char *, path_address))
		return fstat_descriptor(directory, address);
	path = at_path(directory, path_address, &error);
	if (!path)
		return -error;
	if (!GetFileAttributesExW(path, GetFileExInfoStandard, &data))
	{
		long result = windows_failure();
		int device = !wcscmp(path, L"NUL");

		free(path);
		if (device)
		{
			memset(&information, 0, sizeof(information));
			kstat_from_information(&information, FILE_TYPE_CHAR, GUEST(struct halo_guest_kstat *, address));
			return 0;
		}
		return result;
	}
	free(path);
	memset(&information, 0, sizeof(information));
	information.dwFileAttributes = data.dwFileAttributes;
	information.ftLastAccessTime = data.ftLastAccessTime;
	information.ftLastWriteTime = data.ftLastWriteTime;
	information.ftCreationTime = data.ftCreationTime;
	information.nFileSizeHigh = data.nFileSizeHigh;
	information.nFileSizeLow = data.nFileSizeLow;
	kstat_from_information(&information, FILE_TYPE_DISK, GUEST(struct halo_guest_kstat *, address));
	return 0;
}

static long open_file(int directory, uint64_t path_address, uint32_t flags, uint32_t mode)
{
	uint32_t access_mode = flags & GUEST_O_ACCMODE;
	DWORD access = 0, disposition, attributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS;
	BY_HANDLE_FILE_INFORMATION information;
	HANDLE handle;
	wchar_t *path;
	int error, fd, is_directory;

	if (flags & (GUEST_O_PATH | GUEST_O_TMPFILE_BIT))
		return -GUEST_EOPNOTSUPP;
	path = at_path(directory, path_address, &error);
	if (!path)
		return -error;
	if (access_mode == GUEST_O_RDONLY || access_mode == GUEST_O_RDWR)
		access |= GENERIC_READ;
	if (access_mode == GUEST_O_WRONLY || access_mode == GUEST_O_RDWR)
		access |= (flags & GUEST_O_APPEND) ? (FILE_GENERIC_WRITE & ~FILE_WRITE_DATA) : GENERIC_WRITE;
	if ((flags & GUEST_O_CREAT) && (flags & GUEST_O_EXCL))
		disposition = CREATE_NEW;
	else if ((flags & GUEST_O_CREAT) && (flags & GUEST_O_TRUNC))
		disposition = CREATE_ALWAYS;
	else if (flags & GUEST_O_CREAT)
		disposition = OPEN_ALWAYS;
	else if (flags & GUEST_O_TRUNC)
		disposition = TRUNCATE_EXISTING;
	else
		disposition = OPEN_EXISTING;
	if ((flags & GUEST_O_CREAT) && !(mode & 0222))
		attributes |= FILE_ATTRIBUTE_READONLY;
	handle = CreateFileW(path, access ? access : FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, disposition, attributes, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		long result = windows_failure();
		DWORD existing = GetFileAttributesW(path);

		free(path);
		if (existing != INVALID_FILE_ATTRIBUTES && (existing & FILE_ATTRIBUTE_DIRECTORY) && access_mode != GUEST_O_RDONLY)
			return -GUEST_EISDIR;
		return result;
	}
	free(path);
	is_directory = GetFileType(handle) == FILE_TYPE_DISK && GetFileInformationByHandle(handle, &information) &&
		(information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	if ((flags & GUEST_O_DIRECTORY) && !is_directory)
	{
		CloseHandle(handle);
		return -GUEST_ENOTDIR;
	}
	if (is_directory && access_mode != GUEST_O_RDONLY)
	{
		CloseHandle(handle);
		return -GUEST_EISDIR;
	}
	fd = _open_osfhandle((intptr_t)handle, _O_BINARY | (access_mode == GUEST_O_RDONLY ? _O_RDONLY : 0));
	if (fd < 0)
	{
		CloseHandle(handle);
		return -GUEST_EMFILE_OR_NOMEM;
	}
	if (fd < DESCRIPTOR_COUNT)
	{
		AcquireSRWLockExclusive(&descriptor_lock);
		free(listings[fd]);
		listings[fd] = NULL;
		descriptor_flags[fd] = flags & ~(uint32_t)(GUEST_O_CREAT | GUEST_O_EXCL | GUEST_O_TRUNC | GUEST_O_NOCTTY);
		ReleaseSRWLockExclusive(&descriptor_lock);
	}
	return fd;
}

static long read_descriptor(int fd, void *buffer, uint32_t size, int64_t offset, int positional)
{
	HANDLE handle = handle_of(fd);
	OVERLAPPED overlapped, *at = NULL;
	LARGE_INTEGER saved, zero;
	DWORD got = 0;
	BOOL ok;

	if (handle == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	if (fd >= 0 && fd < DESCRIPTOR_COUNT && listings[fd])
		return -GUEST_EISDIR;
	zero.QuadPart = 0;
	if (positional)
	{
		/* (pread leaves the position alone; ReadFile at an offset moves it) */
		SetFilePointerEx(handle, zero, &saved, FILE_CURRENT);
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.Offset = (DWORD)offset;
		overlapped.OffsetHigh = (DWORD)((uint64_t)offset >> 32);
		at = &overlapped;
	}
	ok = ReadFile(handle, buffer, size, &got, at);
	if (positional)
		SetFilePointerEx(handle, saved, NULL, FILE_BEGIN);
	if (!ok)
	{
		DWORD error = GetLastError();

		if (error == ERROR_HANDLE_EOF || error == ERROR_BROKEN_PIPE)
			return 0;
		if (error == ERROR_ACCESS_DENIED)
			return -GUEST_EBADF;
		return -host_errno_of_windows(error);
	}
	return (long)got;
}

static long write_descriptor(int fd, const void *buffer, uint32_t size, int64_t offset, int positional)
{
	HANDLE handle;
	OVERLAPPED overlapped, *at = NULL;
	LARGE_INTEGER saved, zero;
	DWORD written = 0;
	BOOL ok;

	if (fd == 1 || fd == 2)
	{
		host_console_write(fd, buffer, size);
		return (long)size;
	}
	handle = handle_of(fd);
	if (handle == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	zero.QuadPart = 0;
	if (positional)
	{
		SetFilePointerEx(handle, zero, &saved, FILE_CURRENT);
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.Offset = (DWORD)offset;
		overlapped.OffsetHigh = (DWORD)((uint64_t)offset >> 32);
		at = &overlapped;
	}
	ok = WriteFile(handle, buffer, size, &written, at);
	if (positional)
		SetFilePointerEx(handle, saved, NULL, FILE_BEGIN);
	if (!ok)
	{
		DWORD error = GetLastError();

		if (error == ERROR_ACCESS_DENIED)
			return -GUEST_EBADF;
		return -host_errno_of_windows(error);
	}
	return (long)written;
}

/* readv and writev, and their positional forms, a vector at a time */
static long vector_io(int fd, uint64_t vector, int count, int64_t offset, int positional, int writing)
{
	const struct halo_guest_iovec *guest_vector = GUEST(const struct halo_guest_iovec *, vector);
	long total = 0;
	int index;

	if (count < 0 || count > GUEST_IOV_MAX)
		return -GUEST_EINVAL;
	if (count && !vector)
		return -GUEST_EFAULT;
	for (index = 0; index < count; index++)
	{
		void *base = GUEST(void *, guest_vector[index].base);
		uint32_t length = guest_vector[index].length;
		long done;

		if (!length)
			continue;
		done = writing ? write_descriptor(fd, base, length, offset + total, positional) :
			read_descriptor(fd, base, length, offset + total, positional);
		if (done < 0)
			return total ? total : done;
		total += done;
		if ((uint32_t)done < length)
			break;
	}
	return total;
}

/* a 64-bit offset from the two longs musl passes for it, low half first */
static int64_t offset_of_halves(long long low, long long high)
{
	return (int64_t)((uint64_t)(uint32_t)low | ((uint64_t)(uint32_t)high << 32));
}

static long seek_descriptor(int fd, uint32_t offset_high, uint32_t offset_low, uint64_t result, int whence)
{
	HANDLE handle = handle_of(fd);
	LARGE_INTEGER distance, position;
	DWORD method = whence == GUEST_SEEK_SET ? FILE_BEGIN : whence == GUEST_SEEK_CUR ? FILE_CURRENT :
		whence == GUEST_SEEK_END ? FILE_END : 0xffffffff;

	if (!result)
		return -GUEST_EFAULT;
	if (handle == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	if (method == 0xffffffff)
		return -GUEST_EINVAL;
	if (fd >= 0 && fd < DESCRIPTOR_COUNT && listings[fd])
	{
		/* (rewinddir) */
		if (whence != GUEST_SEEK_SET || offset_high || offset_low)
			return -GUEST_EINVAL;
		AcquireSRWLockExclusive(&descriptor_lock);
		listings[fd]->used = listings[fd]->offset = 0;
		listings[fd]->position = 0;
		listings[fd]->started = listings[fd]->finished = 0;
		ReleaseSRWLockExclusive(&descriptor_lock);
		memset(GUEST(void *, result), 0, sizeof(int64_t));
		return 0;
	}
	if (GetFileType(handle) != FILE_TYPE_DISK)
		return -GUEST_ESPIPE;
	distance.QuadPart = (int64_t)(((uint64_t)offset_high << 32) | offset_low);
	if (!SetFilePointerEx(handle, distance, &position, method))
		return windows_failure();
	memcpy(GUEST(void *, result), &position.QuadPart, sizeof(int64_t));
	return 0;
}

/* getdents64: the directory's entries as struct linux_dirent64 */
static long list_directory(int fd, uint64_t address, uint32_t size)
{
	uint8_t *out = GUEST(uint8_t *, address);
	HANDLE handle = handle_of(fd);
	struct listing *listing;
	uint32_t written = 0;
	long result;

	if (handle == INVALID_HANDLE_VALUE || fd >= DESCRIPTOR_COUNT)
		return -GUEST_EBADF;
	if (!address)
		return -GUEST_EFAULT;
	AcquireSRWLockExclusive(&descriptor_lock);
	listing = listings[fd];
	if (!listing)
	{
		BY_HANDLE_FILE_INFORMATION information;

		if (!GetFileInformationByHandle(handle, &information) ||
			!(information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			ReleaseSRWLockExclusive(&descriptor_lock);
			return -GUEST_ENOTDIR;
		}
		listing = listings[fd] = calloc(1, sizeof(*listing));
		if (!listing)
		{
			ReleaseSRWLockExclusive(&descriptor_lock);
			return -GUEST_ENOMEM;
		}
	}
	for (;;)
	{
		const struct full_directory_entry *entry;
		char name[1024];
		int length;
		uint32_t record;
		struct halo_guest_dirent64_header header;

		if (listing->offset >= listing->used)
		{
			if (listing->finished)
				break;
			if (!GetFileInformationByHandleEx(handle, listing->started ? FULL_DIRECTORY_INFO :
				FULL_DIRECTORY_RESTART_INFO, listing->buffer, sizeof(listing->buffer)))
			{
				DWORD error = GetLastError();

				listing->finished = 1;
				listing->used = listing->offset = 0;
				if (error == ERROR_NO_MORE_FILES)
					break;
				ReleaseSRWLockExclusive(&descriptor_lock);
				return written ? (long)written : -host_errno_of_windows(error);
			}
			listing->started = 1;
			listing->used = sizeof(listing->buffer);
			listing->offset = 0;
		}
		entry = (const struct full_directory_entry *)(listing->buffer + listing->offset);
		length = WideCharToMultiByte(CP_UTF8, 0, entry->FileName, (int)(entry->FileNameLength / sizeof(WCHAR)),
			name, (int)sizeof(name) - 1, NULL, NULL);
		if (length < 0)
			length = 0;
		name[length] = 0;
		record = (uint32_t)((sizeof(header) + (size_t)length + 1 + 7) & ~(size_t)7);
		if (written + record > size)
		{
			if (!written)
			{
				ReleaseSRWLockExclusive(&descriptor_lock);
				return -GUEST_EINVAL;
			}
			break;
		}
		header.d_ino = (uint64_t)entry->FileIndex + 1;
		header.d_off = ++listing->position;
		header.d_reclen = (uint16_t)record;
		header.d_type = (entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 4 : 8;
		memset(out + written, 0, record);
		memcpy(out + written, &header, sizeof(header));
		memcpy(out + written + sizeof(header), name, (size_t)length + 1);
		written += record;
		if (entry->NextEntryOffset)
			listing->offset += entry->NextEntryOffset;
		else
			listing->offset = listing->used;
	}
	result = (long)written;
	ReleaseSRWLockExclusive(&descriptor_lock);
	return result;
}

static long access_path(int directory, uint64_t address, int mode)
{
	wchar_t *path;
	DWORD attributes;
	int error;

	path = at_path(directory, address, &error);
	if (!path)
		return -error;
	attributes = GetFileAttributesW(path);
	free(path);
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return windows_failure();
	/* W_OK on a read-only file */
	if ((mode & 2) && (attributes & FILE_ATTRIBUTE_READONLY) && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
		return -GUEST_EACCES;
	return 0;
}

static long unlink_path(int directory, uint64_t address, int flags)
{
	wchar_t *path;
	int error;
	BOOL ok;

	path = at_path(directory, address, &error);
	if (!path)
		return -error;
	if (flags & GUEST_AT_REMOVEDIR)
		ok = RemoveDirectoryW(path);
	else
	{
		DWORD attributes = GetFileAttributesW(path);

		if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			free(path);
			return -GUEST_EISDIR;
		}
		/* (POSIX removes a read-only file, Windows does not) */
		if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY))
			SetFileAttributesW(path, attributes & ~(DWORD)FILE_ATTRIBUTE_READONLY);
		ok = DeleteFileW(path);
	}
	if (!ok)
	{
		long result = windows_failure();

		free(path);
		return result;
	}
	free(path);
	return 0;
}

static long rename_path(int from_directory, uint64_t from, int to_directory, uint64_t to, uint32_t flags)
{
	wchar_t *old_path, *new_path;
	int error;
	BOOL ok;

	/* RENAME_NOREPLACE only */
	if (flags & ~1u)
		return -GUEST_EINVAL;
	old_path = at_path(from_directory, from, &error);
	if (!old_path)
		return -error;
	new_path = at_path(to_directory, to, &error);
	if (!new_path)
	{
		free(old_path);
		return -error;
	}
	ok = MoveFileExW(old_path, new_path, (flags & 1) ? 0 : MOVEFILE_REPLACE_EXISTING);
	free(old_path);
	free(new_path);
	return ok ? 0 : windows_failure();
}

static long make_directory(int directory, uint64_t address)
{
	wchar_t *path;
	int error;
	BOOL ok;

	path = at_path(directory, address, &error);
	if (!path)
		return -error;
	ok = CreateDirectoryW(path, NULL);
	free(path);
	return ok ? 0 : windows_failure();
}

static long change_mode(int directory, uint64_t address, int fd, uint32_t mode)
{
	wchar_t *path;
	wchar_t buffer[32768];
	DWORD attributes;
	int error;
	BOOL ok;

	if (address)
	{
		path = at_path(directory, address, &error);
		if (!path)
			return -error;
	}
	else
	{
		HANDLE handle = handle_of(fd);
		DWORD length;

		if (handle == INVALID_HANDLE_VALUE)
			return -GUEST_EBADF;
		length = GetFinalPathNameByHandleW(handle, buffer, (DWORD)(sizeof(buffer) / sizeof(buffer[0])), 0);
		if (!length || length >= sizeof(buffer) / sizeof(buffer[0]))
			return windows_failure();
		path = _wcsdup(buffer);
		if (!path)
			return -GUEST_ENOMEM;
	}
	attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		long result = windows_failure();

		free(path);
		return result;
	}
	if (mode & 0200)
		attributes &= ~(DWORD)FILE_ATTRIBUTE_READONLY;
	else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY))
		attributes |= FILE_ATTRIBUTE_READONLY;
	ok = SetFileAttributesW(path, attributes ? attributes : FILE_ATTRIBUTE_NORMAL);
	free(path);
	return ok ? 0 : windows_failure();
}

/* utimensat's nanosecond values that mean now and leave alone */
#define GUEST_UTIME_NOW ((1l << 30) - 1l)
#define GUEST_UTIME_OMIT ((1l << 30) - 2l)

static long set_times(int directory, uint64_t address, uint64_t times)
{
	const struct halo_guest_timespec *values = GUEST(const struct halo_guest_timespec *, times);
	FILETIME access, modification;
	FILETIME *access_pointer = &access, *modification_pointer = &modification;
	int64_t now = realtime_nanoseconds();
	HANDLE handle;
	int close = 0;
	BOOL ok;

	access = modification = nanoseconds_filetime(now);
	if (times)
	{
		if (values[0].nanoseconds == GUEST_UTIME_OMIT)
			access_pointer = NULL;
		else if (values[0].nanoseconds != GUEST_UTIME_NOW)
			access = nanoseconds_filetime((int64_t)values[0].seconds * 1000000000LL + values[0].nanoseconds);
		if (values[1].nanoseconds == GUEST_UTIME_OMIT)
			modification_pointer = NULL;
		else if (values[1].nanoseconds != GUEST_UTIME_NOW)
			modification = nanoseconds_filetime((int64_t)values[1].seconds * 1000000000LL + values[1].nanoseconds);
	}
	if (address)
	{
		wchar_t *path;
		int error;

		path = at_path(directory, address, &error);
		if (!path)
			return -error;
		handle = CreateFileW(path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		free(path);
		if (handle == INVALID_HANDLE_VALUE)
			return windows_failure();
		close = 1;
	}
	else
	{
		handle = handle_of(directory);
		if (handle == INVALID_HANDLE_VALUE)
			return -GUEST_EBADF;
	}
	ok = SetFileTime(handle, NULL, access_pointer, modification_pointer);
	if (close)
		CloseHandle(handle);
	return ok ? 0 : windows_failure();
}

static long lock_descriptor(int fd, int operation)
{
	HANDLE handle = handle_of(fd);
	OVERLAPPED overlapped;
	DWORD flags = 0;

	if (handle == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	memset(&overlapped, 0, sizeof(overlapped));
	if (operation & GUEST_LOCK_UN)
		return UnlockFileEx(handle, 0, MAXDWORD, MAXDWORD, &overlapped) || GetLastError() == ERROR_NOT_LOCKED ? 0 :
			windows_failure();
	if (operation & GUEST_LOCK_EX)
		flags |= LOCKFILE_EXCLUSIVE_LOCK;
	if (operation & GUEST_LOCK_NB)
		flags |= LOCKFILE_FAIL_IMMEDIATELY;
	if (!LockFileEx(handle, flags, 0, MAXDWORD, MAXDWORD, &overlapped))
		return GetLastError() == ERROR_LOCK_VIOLATION ? -GUEST_EAGAIN : windows_failure();
	return 0;
}

static long control_descriptor(int fd, int command, uint64_t argument)
{
	if (handle_of(fd) == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	switch (command)
	{
	case GUEST_F_DUPFD:
	case GUEST_F_DUPFD_CLOEXEC:
	{
		int copy = _dup(fd);

		if (copy < 0)
			return -GUEST_EMFILE_OR_NOMEM;
		if (copy < DESCRIPTOR_COUNT && fd < DESCRIPTOR_COUNT)
			descriptor_flags[copy] = descriptor_flags[fd];
		return copy;
	}
	case GUEST_F_GETFD:
		return 0;
	case GUEST_F_SETFD:
		return 0;
	case GUEST_F_GETFL:
		return fd < DESCRIPTOR_COUNT ? (long)descriptor_flags[fd] : 0;
	case GUEST_F_SETFL:
		if (fd < DESCRIPTOR_COUNT)
			descriptor_flags[fd] = (descriptor_flags[fd] & ~(uint32_t)(GUEST_O_APPEND | GUEST_O_NONBLOCK)) |
				((uint32_t)argument & (GUEST_O_APPEND | GUEST_O_NONBLOCK));
		return 0;
	case GUEST_F_GETLK:
	case GUEST_F_OFD_GETLK:
	{
		struct halo_guest_flock *lock = GUEST(struct halo_guest_flock *, argument);

		if (!argument)
			return -GUEST_EFAULT;
		lock->l_type = GUEST_F_UNLCK;
		return 0;
	}
	case GUEST_F_SETLK:
	case GUEST_F_SETLKW:
	case GUEST_F_OFD_SETLK:
	case GUEST_F_OFD_SETLKW:
		/* (advisory locks between this game's own processes: none) */
		return 0;
	}
	return -GUEST_EINVAL;
}

static long truncate_descriptor(int fd, int64_t size)
{
	HANDLE handle = handle_of(fd);
	FILE_END_OF_FILE_INFO end;

	if (handle == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	if (size < 0)
		return -GUEST_EINVAL;
	end.EndOfFile.QuadPart = size;
	return SetFileInformationByHandle(handle, FileEndOfFileInfo, &end, sizeof(end)) ? 0 : windows_failure();
}

static long close_descriptor(int fd)
{
	if (handle_of(fd) == INVALID_HANDLE_VALUE)
		return -GUEST_EBADF;
	descriptor_forget(fd);
	/* (the standard streams stay: the host writes to them itself) */
	if (fd <= 2)
		return 0;
	return _close(fd) == 0 ? 0 : -GUEST_EBADF;
}

static long make_pipe(uint64_t address, uint32_t flags)
{
	int *result = GUEST(int *, address);
	HANDLE read_end, write_end;
	int descriptors[2];

	(void)flags;
	if (!address)
		return -GUEST_EFAULT;
	if (!CreatePipe(&read_end, &write_end, NULL, 0))
		return windows_failure();
	descriptors[0] = _open_osfhandle((intptr_t)read_end, _O_BINARY | _O_RDONLY);
	descriptors[1] = _open_osfhandle((intptr_t)write_end, _O_BINARY);
	if (descriptors[0] < 0 || descriptors[1] < 0)
	{
		if (descriptors[0] >= 0)
			_close(descriptors[0]);
		else
			CloseHandle(read_end);
		if (descriptors[1] >= 0)
			_close(descriptors[1]);
		else
			CloseHandle(write_end);
		return -GUEST_EMFILE_OR_NOMEM;
	}
	result[0] = descriptors[0];
	result[1] = descriptors[1];
	return 0;
}

static char executable_path[4096];

static long read_link(int directory, uint64_t address, uint64_t buffer, uint32_t size)
{
	const char *path = GUEST(const char *, address);
	size_t length;

	(void)directory;
	if (!address || !buffer)
		return -GUEST_EFAULT;
	if (strcmp(path, "/proc/self/exe"))
		return -GUEST_EINVAL;
	if (!executable_path[0])
	{
		wchar_t wide[4096];
		DWORD wide_length = GetModuleFileNameW(NULL, wide, (DWORD)(sizeof(wide) / sizeof(wide[0])));

		if (!wide_length || wide_length >= sizeof(wide) / sizeof(wide[0]) ||
			host_path_out(wide, executable_path, sizeof(executable_path)) < 0)
		{
			executable_path[0] = 0;
			return -GUEST_ENAMETOOLONG;
		}
	}
	length = strlen(executable_path);
	if (length > size)
		length = size;
	memcpy(GUEST(char *, buffer), executable_path, length);
	return (long)length;
}

static long current_directory(uint64_t buffer, uint32_t size)
{
	wchar_t wide[32768];
	char path[32768];
	DWORD length = GetCurrentDirectoryW((DWORD)(sizeof(wide) / sizeof(wide[0])), wide);
	int out;

	if (!buffer)
		return -GUEST_EFAULT;
	if (!length || length >= sizeof(wide) / sizeof(wide[0]))
		return windows_failure();
	out = host_path_out(wide, path, sizeof(path));
	if (out < 0)
		return -GUEST_ENAMETOOLONG;
	if ((uint32_t)out + 1 > size)
		return -GUEST_ERANGE;
	memcpy(GUEST(char *, buffer), path, (size_t)out + 1);
	return out + 1;
}

static long change_directory(uint64_t address)
{
	wchar_t *path;
	int error;
	BOOL ok;

	path = host_path_in(GUEST(const char *, address), &error);
	if (!path)
		return -error;
	ok = SetCurrentDirectoryW(path);
	free(path);
	return ok ? 0 : windows_failure();
}

/* ---------- futexes, on WaitOnAddress */

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_REQUEUE 3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP 5
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10
#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_CLOCK_REALTIME 256

/* A waiter wakes at least once for each wake that reaches it; waking more
than asked (all for a count above one, and for a requeue, which moves
nobody) only makes waiters look again, which musl's waits do anyway. */
static long futex(uint64_t address, int operation, uint32_t value, uint64_t timeout, uint64_t address2, uint32_t value3)
{
	volatile uint32_t *word = GUEST(volatile uint32_t *, address);
	int command = operation & ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME);

	(void)address2;
	if (!address || (address & 3))
		return -GUEST_EINVAL;
	switch (command)
	{
	case FUTEX_WAIT:
	case FUTEX_WAIT_BITSET:
	{
		int64_t nanoseconds = 0;
		DWORD milliseconds = INFINITE;
		int has_timeout = timespec_in(timeout, &nanoseconds);

		if (has_timeout && command == FUTEX_WAIT_BITSET)
		{
			int64_t now = 0;

			clock_now((operation & FUTEX_CLOCK_REALTIME) ? 0 : 1, &now);
			nanoseconds -= now;
		}
		if (has_timeout)
		{
			if (nanoseconds <= 0)
				return *word == value ? -GUEST_ETIMEDOUT : -GUEST_EAGAIN;
			milliseconds = (DWORD)((nanoseconds + 999999) / 1000000);
			if (milliseconds == INFINITE)
				milliseconds--;
		}
		if (*word != value)
			return -GUEST_EAGAIN;
		if (!WaitOnAddress((volatile void *)word, &value, sizeof(value), milliseconds))
			return GetLastError() == ERROR_TIMEOUT ? -GUEST_ETIMEDOUT : 0;
		return 0;
	}
	case FUTEX_WAKE:
	case FUTEX_WAKE_BITSET:
		if (value == 1)
			WakeByAddressSingle((void *)word);
		else if (value)
			WakeByAddressAll((void *)word);
		return value ? 1 : 0;
	case FUTEX_CMP_REQUEUE:
		if (*word != value3)
			return -GUEST_EAGAIN;
		/* (fall through) */
	case FUTEX_REQUEUE:
		WakeByAddressAll((void *)word);
		return 0;
	case FUTEX_WAKE_OP:
		WakeByAddressAll((void *)word);
		if (address2)
			WakeByAddressAll(GUEST(void *, address2));
		return 0;
	}
	return -GUEST_ENOSYS;
}

/* ---------- the process and the system */

/* struct utsname: six fields of 65 bytes */
static long uname_out(uint64_t address)
{
	char *fields = GUEST(char *, address);
	DWORD size = 65;

	if (!address)
		return -GUEST_EFAULT;
	memset(fields, 0, 6 * 65);
	strcpy(fields, "Windows");
	if (!GetComputerNameA(fields + 65, &size))
		strcpy(fields + 65, "localhost");
	strcpy(fields + 2 * 65, "10.0");
	strcpy(fields + 3 * 65, "halo-host");
	strcpy(fields + 4 * 65, "x86_64");
	return 0;
}

static uint32_t saturate(uint64_t value)
{
	return value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
}

static long sysinfo_out(uint64_t address)
{
	struct halo_guest_sysinfo *result = GUEST(struct halo_guest_sysinfo *, address);
	MEMORYSTATUSEX status;
	uint64_t sizes[4], unit = 1;
	int index, large;

	if (!address)
		return -GUEST_EFAULT;
	memset(&status, 0, sizeof(status));
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	sizes[0] = status.ullTotalPhys;
	sizes[1] = status.ullAvailPhys;
	sizes[2] = status.ullTotalPageFile > status.ullTotalPhys ? status.ullTotalPageFile - status.ullTotalPhys : 0;
	sizes[3] = status.ullAvailPageFile > status.ullAvailPhys ? status.ullAvailPageFile - status.ullAvailPhys : 0;
	for (;;)
	{
		large = 0;
		for (index = 0; index < 4; index++)
			large |= sizes[index] > UINT32_MAX;
		if (!large || unit >= 0x80000000u)
			break;
		unit <<= 1;
		for (index = 0; index < 4; index++)
			sizes[index] >>= 1;
	}
	memset(result, 0, sizeof(*result));
	result->uptime = saturate(GetTickCount64() / 1000);
	result->totalram = saturate(sizes[0]);
	result->freeram = saturate(sizes[1]);
	result->totalswap = saturate(sizes[2]);
	result->freeswap = saturate(sizes[3]);
	result->procs = 1;
	result->mem_unit = (uint32_t)unit;
	return 0;
}

/* prlimit64's and getrlimit's struct rlimit: two 64-bit values */
static long limit_out(int resource, uint64_t address)
{
	uint64_t *result = GUEST(uint64_t *, address);

	if (!address)
		return 0;
	switch (resource)
	{
	case GUEST_RLIMIT_NOFILE:
		result[0] = result[1] = DESCRIPTOR_COUNT;
		break;
	case GUEST_RLIMIT_STACK:
		result[0] = 8 * 1024 * 1024;
		result[1] = ~0ULL;
		break;
	case GUEST_RLIMIT_CORE:
		result[0] = result[1] = 0;
		break;
	default:
		result[0] = result[1] = ~0ULL;
		break;
	}
	return 0;
}

static long send_signal(int signal)
{
	if (!signal)
		return 0;
	if (signal == GUEST_SIGABRT || signal == GUEST_SIGKILL || signal == GUEST_SIGSEGV || signal == GUEST_SIGTERM)
	{
		host_logf(HOST_LOG_FATAL, "the game raised signal %d", signal);
		TerminateProcess(GetCurrentProcess(), 128 + signal);
	}
	return 0;
}

static long poll_descriptors(uint64_t address, uint32_t count, uint64_t timeout)
{
	struct guest_pollfd { int32_t fd; int16_t events, revents; } *fds = GUEST(struct guest_pollfd *, address);
	int64_t nanoseconds = 0;
	uint32_t index;
	long ready = 0;

	for (index = 0; index < count; index++)
	{
		fds[index].revents = 0;
		if (fds[index].fd < 0)
			continue;
		if (handle_of(fds[index].fd) == INVALID_HANDLE_VALUE)
			fds[index].revents = GUEST_POLLNVAL;
		else
			fds[index].revents = fds[index].events & (GUEST_POLLIN | GUEST_POLLOUT);
		if (fds[index].revents)
			ready++;
	}
	if (ready)
		return ready;
	if (timespec_in(timeout, &nanoseconds))
		sleep_nanoseconds(nanoseconds);
	else
		Sleep(INFINITE);
	return 0;
}

/* ---------- dispatch */

long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	switch (number)
	{
	case GUEST_SYS_read:
		return read_descriptor((int)a, GUEST(void *, b), (uint32_t)c, 0, 0);
	case GUEST_SYS_write:
		return write_descriptor((int)a, GUEST(const void *, b), (uint32_t)c, 0, 0);
	case GUEST_SYS_pread64:
		return read_descriptor((int)a, GUEST(void *, b), (uint32_t)c, (int64_t)d, 1);
	case GUEST_SYS_pwrite64:
		return write_descriptor((int)a, GUEST(const void *, b), (uint32_t)c, (int64_t)d, 1);
	case GUEST_SYS_readv:
		return vector_io((int)a, (uint64_t)b, (int)c, 0, 0, 0);
	case GUEST_SYS_writev:
		return vector_io((int)a, (uint64_t)b, (int)c, 0, 0, 1);
	case GUEST_SYS_preadv:
		return vector_io((int)a, (uint64_t)b, (int)c, offset_of_halves(d, e), 1, 0);
	case GUEST_SYS_pwritev:
		return vector_io((int)a, (uint64_t)b, (int)c, offset_of_halves(d, e), 1, 1);

	case GUEST_SYS_openat:
		return open_file((int)a, (uint64_t)b, (uint32_t)c, (uint32_t)d);
	case GUEST_SYS_close:
		return close_descriptor((int)a);
	case GUEST_SYS_fstat:
		return fstat_descriptor((int)a, (uint64_t)b);
	case GUEST_SYS_newfstatat:
		return fstat_path((int)a, (uint64_t)b, (uint64_t)c, (int)d);
	case GUEST_SYS__llseek:
		return seek_descriptor((int)a, (uint32_t)b, (uint32_t)c, (uint64_t)d, (int)e);
	case GUEST_SYS_getdents64:
		return list_directory((int)a, (uint64_t)b, (uint32_t)c);
	case GUEST_SYS_fcntl:
		return control_descriptor((int)a, (int)b, (uint64_t)c);
	case GUEST_SYS_flock:
		return lock_descriptor((int)a, (int)b);
	case GUEST_SYS_ftruncate:
		return truncate_descriptor((int)a, (int64_t)b);
	case GUEST_SYS_fsync:
	case GUEST_SYS_fdatasync:
	{
		HANDLE handle = handle_of((int)a);

		if (handle == INVALID_HANDLE_VALUE)
			return -GUEST_EBADF;
		FlushFileBuffers(handle);
		return 0;
	}
	case GUEST_SYS_dup:
	{
		int copy = handle_of((int)a) == INVALID_HANDLE_VALUE ? -1 : _dup((int)a);

		return copy < 0 ? -GUEST_EBADF : copy;
	}
	case GUEST_SYS_dup3:
		if (a == b)
			return -GUEST_EINVAL;
		if (handle_of((int)a) == INVALID_HANDLE_VALUE)
			return -GUEST_EBADF;
		descriptor_forget((int)b);
		return _dup2((int)a, (int)b) == 0 ? b : -GUEST_EBADF;
	case GUEST_SYS_pipe2:
		return make_pipe((uint64_t)a, (uint32_t)b);
	case GUEST_SYS_faccessat:
		return access_path((int)a, (uint64_t)b, (int)c);
	case GUEST_SYS_mkdirat:
		return make_directory((int)a, (uint64_t)b);
	case GUEST_SYS_unlinkat:
		return unlink_path((int)a, (uint64_t)b, (int)c);
	case GUEST_SYS_renameat:
		return rename_path((int)a, (uint64_t)b, (int)c, (uint64_t)d, 0);
	case GUEST_SYS_renameat2:
		return rename_path((int)a, (uint64_t)b, (int)c, (uint64_t)d, (uint32_t)e);
	case GUEST_SYS_readlinkat:
		return read_link((int)a, (uint64_t)b, (uint64_t)c, (uint32_t)d);
	case GUEST_SYS_getcwd:
		return current_directory((uint64_t)a, (uint32_t)b);
	case GUEST_SYS_chdir:
		return change_directory((uint64_t)a);
	case GUEST_SYS_fchmod:
		return change_mode(0, 0, (int)a, (uint32_t)b);
	case GUEST_SYS_fchmodat:
		return change_mode((int)a, (uint64_t)b, -1, (uint32_t)c);
	case GUEST_SYS_utimensat:
		return set_times((int)a, (uint64_t)b, (uint64_t)c);
	case GUEST_SYS_umask:
		return 022;

	case GUEST_SYS_clock_gettime:
	case GUEST_SYS_clock_getres:
	{
		int64_t nanoseconds = 0;
		long result = clock_now((int)a, &nanoseconds);

		if (result)
			return result;
		timespec_out((uint64_t)b, number == GUEST_SYS_clock_getres ? 100 : nanoseconds);
		return 0;
	}
	case GUEST_SYS_gettimeofday:
	{
		struct halo_guest_timespec *result = GUEST(struct halo_guest_timespec *, a);
		int64_t now = realtime_nanoseconds();

		if (result)
		{
			result->seconds = (int32_t)(now / 1000000000LL);
			result->nanoseconds = (int32_t)((now % 1000000000LL) / 1000);
		}
		return 0;
	}
	case GUEST_SYS_nanosleep:
	{
		int64_t nanoseconds;

		if (!timespec_in((uint64_t)a, &nanoseconds))
			return -GUEST_EFAULT;
		if (nanoseconds < 0)
			return -GUEST_EINVAL;
		sleep_nanoseconds(nanoseconds);
		return 0;
	}
	case GUEST_SYS_clock_nanosleep:
	{
		int64_t nanoseconds, now = 0;
		long result;

		if (!timespec_in((uint64_t)c, &nanoseconds))
			return -GUEST_EFAULT;
		result = clock_now((int)a, &now);
		if (result)
			return -result;
		if ((int)b & GUEST_TIMER_ABSTIME)
			nanoseconds -= now;
		sleep_nanoseconds(nanoseconds);
		return 0;
	}
	case GUEST_SYS_sched_yield:
		SwitchToThread();
		return 0;
	case GUEST_SYS_futex:
		return futex((uint64_t)a, (int)b, (uint32_t)c, (uint64_t)d, (uint64_t)e, (uint32_t)f);
	case GUEST_SYS_ppoll:
		return poll_descriptors((uint64_t)a, (uint32_t)b, (uint64_t)c);

	case GUEST_SYS_mmap:
		return host_guest_mmap((uint64_t)a, (uint64_t)b, (int)c, (int)d, (int)e, f);
	case GUEST_SYS_munmap:
		return host_guest_munmap((uint64_t)a, (uint64_t)b);
	case GUEST_SYS_mprotect:
		return host_guest_mprotect((uint64_t)a, (uint64_t)b, (int)c);
	case GUEST_SYS_madvise:
		return host_guest_madvise((uint64_t)a, (uint64_t)b, (int)c);
	case GUEST_SYS_mremap:
	case GUEST_SYS_brk:
		return -GUEST_ENOMEM;
	case GUEST_SYS_membarrier:
		/* MEMBARRIER_CMD_QUERY: GLOBAL, PRIVATE_EXPEDITED and its
		registration; each of them is FlushProcessWriteBuffers */
		if (a == 0)
			return 1 | 8 | 16;
		FlushProcessWriteBuffers();
		return 0;

	case GUEST_SYS_exit:
	case GUEST_SYS_exit_group:
		host_exit((int)a);

	case GUEST_SYS_getrandom:
		if (!b)
			return 0;
		if (!a)
			return -GUEST_EFAULT;
		if (BCryptGenRandom(NULL, GUEST(PUCHAR, a), (ULONG)(uint32_t)b, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
			return -GUEST_EIO;
		return (uint32_t)b;
	case GUEST_SYS_uname:
		return uname_out((uint64_t)a);
	case GUEST_SYS_sysinfo:
		return sysinfo_out((uint64_t)a);
	case GUEST_SYS_getrlimit:
		return limit_out((int)a, (uint64_t)b);
	case GUEST_SYS_prlimit64:
		return limit_out((int)b, (uint64_t)d);
	case GUEST_SYS_getpid:
		return (long)GetCurrentProcessId();
	case GUEST_SYS_getppid:
		return 1;
	case GUEST_SYS_gettid:
	case GUEST_SYS_set_tid_address:
		return (long)GetCurrentThreadId();
	case GUEST_SYS_getuid:
	case GUEST_SYS_geteuid:
	case GUEST_SYS_getgid:
	case GUEST_SYS_getegid:
		return 1000;
	case GUEST_SYS_kill:
		return send_signal((int)b);
	case GUEST_SYS_tkill:
		return send_signal((int)b);
	case GUEST_SYS_tgkill:
		return send_signal((int)c);
	case GUEST_SYS_rt_sigprocmask:
		if (c)
			memset(GUEST(void *, c), 0, 8);
		return 0;
	case GUEST_SYS_rt_sigaction:
	case GUEST_SYS_sigaltstack:
		/* the host owns exception handling */
		return 0;
	case GUEST_SYS_ioctl:
		return -GUEST_ENOTTY;

	case GUEST_SYS_statx:
	case GUEST_SYS_statfs:
	case GUEST_SYS_fstatfs:
	case GUEST_SYS_clone:
	case GUEST_SYS_clone3:
	case GUEST_SYS_execve:
	case GUEST_SYS_rt_sigtimedwait:
	case GUEST_SYS_pselect6:
	case GUEST_SYS_epoll_pwait:
	case GUEST_SYS_sendmsg:
	case GUEST_SYS_recvmsg:
	case GUEST_SYS_timer_create:
	case GUEST_SYS_timer_settime:
	case GUEST_SYS_timer_gettime:
	case GUEST_SYS_setitimer:
	case GUEST_SYS_getitimer:
	case GUEST_SYS_times:
	case GUEST_SYS_getrusage:
	case GUEST_SYS_wait4:
	case GUEST_SYS_waitid:
		return -GUEST_ENOSYS;
	}
	host_logf(HOST_LOG_WARN, "guest system call %lld is not supported", number);
	return -GUEST_ENOSYS;
}
