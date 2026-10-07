/*
HOST_SYSCALL.C

System calls on behalf of the guest's musl runtime (its syscall_arch.h
sends every call here), for macOS.

The guest's musl was built for Linux (port/android/guest/libc), so every
call is a Linux one, by its Linux number, with Linux's flags, structures
and error numbers, which this file performs with Darwin's equivalents:

- pointers are guest addresses (G2H);
- the guest's structures have ILP32 layouts with a 32-bit time_t, as in the
  MSVC runtime (timespec, iovec), or the Linux kernel's (stat, dirent64,
  utsname);
- open flags, AT_* flags, clocks, signals and errno values are numbered
  differently;
- memory mappings go through host_memory.c, which keeps the guest's 4 KB
  pages on the host's 16 KB ones;
- futexes are Darwin's __ulock_wait and __ulock_wake, the primitive under
  its own C++ library's atomic waits;
- what has no meaning for the guest (signal handlers, which the host owns)
  succeeds without effect, and the rest fails with ENOSYS.
*/

#include "host.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

/* ---------- Linux's numbers (AArch64, musl's arch/arm64_32) */

enum
{
	LINUX_SYS_getcwd = 17, LINUX_SYS_epoll_pwait = 22, LINUX_SYS_dup = 23, LINUX_SYS_dup3 = 24,
	LINUX_SYS_fcntl = 25, LINUX_SYS_ioctl = 29, LINUX_SYS_flock = 32, LINUX_SYS_mkdirat = 34,
	LINUX_SYS_unlinkat = 35, LINUX_SYS_symlinkat = 36, LINUX_SYS_linkat = 37, LINUX_SYS_renameat = 38,
	LINUX_SYS_statfs = 43, LINUX_SYS_fstatfs = 44, LINUX_SYS_ftruncate = 46, LINUX_SYS_faccessat = 48,
	LINUX_SYS_chdir = 49, LINUX_SYS_fchmod = 52, LINUX_SYS_fchmodat = 53, LINUX_SYS_openat = 56,
	LINUX_SYS_close = 57, LINUX_SYS_pipe2 = 59, LINUX_SYS_getdents64 = 61, LINUX_SYS_lseek = 62,
	LINUX_SYS_read = 63, LINUX_SYS_write = 64, LINUX_SYS_readv = 65, LINUX_SYS_writev = 66,
	LINUX_SYS_pread64 = 67, LINUX_SYS_pwrite64 = 68, LINUX_SYS_preadv = 69, LINUX_SYS_pwritev = 70,
	LINUX_SYS_pselect6 = 72, LINUX_SYS_ppoll = 73, LINUX_SYS_readlinkat = 78, LINUX_SYS_newfstatat = 79,
	LINUX_SYS_fstat = 80, LINUX_SYS_fsync = 82, LINUX_SYS_fdatasync = 83, LINUX_SYS_utimensat = 88,
	LINUX_SYS_exit = 93, LINUX_SYS_exit_group = 94, LINUX_SYS_set_tid_address = 96, LINUX_SYS_futex = 98,
	LINUX_SYS_nanosleep = 101, LINUX_SYS_clock_gettime = 113, LINUX_SYS_clock_getres = 114,
	LINUX_SYS_clock_nanosleep = 115, LINUX_SYS_sched_getaffinity = 123, LINUX_SYS_sched_yield = 124,
	LINUX_SYS_kill = 129, LINUX_SYS_tkill = 130, LINUX_SYS_tgkill = 131, LINUX_SYS_sigaltstack = 132,
	LINUX_SYS_rt_sigaction = 134, LINUX_SYS_rt_sigprocmask = 135, LINUX_SYS_uname = 160,
	LINUX_SYS_getrlimit = 163, LINUX_SYS_umask = 166, LINUX_SYS_gettimeofday = 169, LINUX_SYS_getpid = 172,
	LINUX_SYS_getppid = 173, LINUX_SYS_getuid = 174, LINUX_SYS_geteuid = 175, LINUX_SYS_getgid = 176,
	LINUX_SYS_getegid = 177, LINUX_SYS_gettid = 178, LINUX_SYS_sysinfo = 179, LINUX_SYS_brk = 214,
	LINUX_SYS_munmap = 215, LINUX_SYS_mremap = 216, LINUX_SYS_mmap = 222, LINUX_SYS_mprotect = 226,
	LINUX_SYS_madvise = 233, LINUX_SYS_prlimit64 = 261, LINUX_SYS_renameat2 = 276, LINUX_SYS_getrandom = 278,
	LINUX_SYS_membarrier = 283, LINUX_SYS_statx = 291, LINUX_SYS_faccessat2 = 439,
};

#define LINUX_AT_FDCWD (-100)
#define LINUX_AT_SYMLINK_NOFOLLOW 0x100
#define LINUX_AT_REMOVEDIR 0x200
#define LINUX_AT_EACCESS 0x200
#define LINUX_AT_EMPTY_PATH 0x1000

#define LINUX_O_ACCMODE 03
#define LINUX_O_CREAT 0100
#define LINUX_O_EXCL 0200
#define LINUX_O_NOCTTY 0400
#define LINUX_O_TRUNC 01000
#define LINUX_O_APPEND 02000
#define LINUX_O_NONBLOCK 04000
#define LINUX_O_DIRECTORY 040000
#define LINUX_O_NOFOLLOW 0100000
#define LINUX_O_CLOEXEC 02000000

#define LINUX_F_DUPFD_CLOEXEC 1030

#define LINUX_EAGAIN 11
#define LINUX_ENOSYS 38
#define LINUX_ETIMEDOUT 110

#define LINUX_SIGABRT 6

#define LINUX_FUTEX_WAIT 0
#define LINUX_FUTEX_WAKE 1
#define LINUX_FUTEX_REQUEUE 3
#define LINUX_FUTEX_CMP_REQUEUE 4
#define LINUX_FUTEX_WAIT_BITSET 9
#define LINUX_FUTEX_WAKE_BITSET 10
#define LINUX_FUTEX_COMMAND 0x7f

#define LINUX_TIOCGWINSZ 0x5413

#define LINUX_RLIMIT_STACK 3
#define LINUX_RLIMIT_NOFILE 7

/* ---------- Darwin's futex (libc++'s atomic waits use it) */

extern int __ulock_wait(uint32_t operation, void *address, uint64_t value, uint32_t timeout_microseconds);
extern int __ulock_wake(uint32_t operation, void *address, uint64_t wake_value);
#define UL_COMPARE_AND_WAIT 1
#define ULF_WAKE_ALL 0x00000100
#define ULF_NO_ERRNO 0x01000000

/* ---------- the guest's structures */

struct guest_timespec
{
	int32_t seconds;
	int32_t nanoseconds;
};

struct guest_iovec
{
	uint32_t base;
	uint32_t length;
};

/* the AArch64 kernel's struct stat (musl's arch/arm64_32/kstat.h) */
struct guest_kstat
{
	uint64_t dev;
	uint64_t ino;
	uint32_t mode;
	uint32_t nlink;
	uint32_t uid;
	uint32_t gid;
	uint64_t rdev;
	uint64_t pad;
	int64_t size;
	int32_t blksize;
	int32_t pad2;
	int64_t blocks;
	int64_t atime_seconds;
	int64_t atime_nanoseconds;
	int64_t mtime_seconds;
	int64_t mtime_nanoseconds;
	int64_t ctime_seconds;
	int64_t ctime_nanoseconds;
	uint32_t unused[2];
};

struct guest_utsname
{
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};

/* ---------- translations */

int host_linux_errno(int error)
{
	/* 1 to 34 are the same but Darwin's 11 */
	if (error == EDEADLK)
		return 35;
	if (error < 35)
		return error;
	switch (error)
	{
	case EAGAIN: return 11;
	case EINPROGRESS: return 115;
	case EALREADY: return 114;
	case ENOTSOCK: return 88;
	case EDESTADDRREQ: return 89;
	case EMSGSIZE: return 90;
	case EPROTOTYPE: return 91;
	case ENOPROTOOPT: return 92;
	case EPROTONOSUPPORT: return 93;
	case ESOCKTNOSUPPORT: return 94;
	case ENOTSUP: return 95;
	case EOPNOTSUPP: return 95;
	case EPFNOSUPPORT: return 96;
	case EAFNOSUPPORT: return 97;
	case EADDRINUSE: return 98;
	case EADDRNOTAVAIL: return 99;
	case ENETDOWN: return 100;
	case ENETUNREACH: return 101;
	case ENETRESET: return 102;
	case ECONNABORTED: return 103;
	case ECONNRESET: return 104;
	case ENOBUFS: return 105;
	case EISCONN: return 106;
	case ENOTCONN: return 107;
	case ESHUTDOWN: return 108;
	case ETOOMANYREFS: return 109;
	case ETIMEDOUT: return 110;
	case ECONNREFUSED: return 111;
	case ELOOP: return 40;
	case ENAMETOOLONG: return 36;
	case EHOSTDOWN: return 112;
	case EHOSTUNREACH: return 113;
	case ENOTEMPTY: return 39;
	case EUSERS: return 87;
	case EDQUOT: return 122;
	case ESTALE: return 116;
	case EREMOTE: return 66;
	case ENOLCK: return 37;
	case ENOSYS: return 38;
	case EOVERFLOW: return 75;
	case ECANCELED: return 125;
	case EIDRM: return 43;
	case ENOMSG: return 42;
	case EILSEQ: return 84;
	case ENOATTR: return 61;
	case EBADMSG: return 74;
	case EMULTIHOP: return 72;
	case ENODATA: return 61;
	case ENOLINK: return 67;
	case ENOSR: return 63;
	case ENOSTR: return 60;
	case EPROTO: return 71;
	case ETIME: return 62;
	case ENOTRECOVERABLE: return 131;
	case EOWNERDEAD: return 130;
	case EQFULL: return 105;
	case EBADEXEC: case EBADARCH: case ESHLIBVERS: case EBADMACHO: return 8;
	case EPWROFF: case EDEVERR: return 5;
	case EAUTH: case ENEEDAUTH: return 1;
	default: return 22;
	}
}

int host_errno(void)
{
	return host_linux_errno(errno);
}

static long result_of(long value)
{
	return value == -1 ? -host_linux_errno(errno) : value;
}

static int open_flags(long long linux_flags)
{
	int flags = (int)(linux_flags & LINUX_O_ACCMODE);

	if (linux_flags & LINUX_O_CREAT) flags |= O_CREAT;
	if (linux_flags & LINUX_O_EXCL) flags |= O_EXCL;
	if (linux_flags & LINUX_O_NOCTTY) flags |= O_NOCTTY;
	if (linux_flags & LINUX_O_TRUNC) flags |= O_TRUNC;
	if (linux_flags & LINUX_O_APPEND) flags |= O_APPEND;
	if (linux_flags & LINUX_O_NONBLOCK) flags |= O_NONBLOCK;
	if (linux_flags & LINUX_O_DIRECTORY) flags |= O_DIRECTORY;
	if (linux_flags & LINUX_O_NOFOLLOW) flags |= O_NOFOLLOW;
	if (linux_flags & LINUX_O_CLOEXEC) flags |= O_CLOEXEC;
	return flags;
}

static long long linux_status_flags(int flags)
{
	long long result = flags & O_ACCMODE;

	if (flags & O_APPEND) result |= LINUX_O_APPEND;
	if (flags & O_NONBLOCK) result |= LINUX_O_NONBLOCK;
	return result;
}

static int directory(long long fd)
{
	return (int)fd == LINUX_AT_FDCWD ? AT_FDCWD : (int)fd;
}

static int clock_from_linux(long long clock)
{
	switch ((int)clock)
	{
	case 0: case 5: return CLOCK_REALTIME;
	case 1: case 6: case 7: return CLOCK_MONOTONIC;
	case 2: return CLOCK_PROCESS_CPUTIME_ID;
	case 3: return CLOCK_THREAD_CPUTIME_ID;
	case 4: return CLOCK_MONOTONIC_RAW;
	default: return -1;
	}
}

static int timespec_in(uint64_t address, struct timespec *result)
{
	const struct guest_timespec *value = G2H(address);

	if (!value)
		return 0;
	result->tv_sec = value->seconds;
	result->tv_nsec = value->nanoseconds;
	return 1;
}

static void timespec_out(uint64_t address, const struct timespec *value)
{
	struct guest_timespec *result = G2H(address);

	if (!result)
		return;
	result->seconds = (int32_t)value->tv_sec;
	result->nanoseconds = (int32_t)value->tv_nsec;
}

static void stat_out(uint64_t address, const struct stat *information)
{
	struct guest_kstat *result = G2H(address);

	memset(result, 0, sizeof(*result));
	result->dev = (uint64_t)(uint32_t)information->st_dev;
	result->ino = information->st_ino;
	result->mode = information->st_mode;
	result->nlink = information->st_nlink;
	result->uid = information->st_uid;
	result->gid = information->st_gid;
	result->rdev = (uint64_t)(uint32_t)information->st_rdev;
	result->size = information->st_size;
	result->blksize = information->st_blksize;
	result->blocks = information->st_blocks;
	result->atime_seconds = information->st_atimespec.tv_sec;
	result->atime_nanoseconds = information->st_atimespec.tv_nsec;
	result->mtime_seconds = information->st_mtimespec.tv_sec;
	result->mtime_nanoseconds = information->st_mtimespec.tv_nsec;
	result->ctime_seconds = information->st_ctimespec.tv_sec;
	result->ctime_nanoseconds = information->st_ctimespec.tv_nsec;
}

/* the guest is about to have the kernel write into its memory: a watched
page would fail the call (EFAULT) instead of faulting */
static void prepare_write(uint64_t address, uint64_t size)
{
	void host_memory_watch_prepare_write(uint32_t address, uint32_t size);

	if (size)
		host_memory_watch_prepare_write((uint32_t)address, (uint32_t)(size > 0xffffffffu ? 0xffffffffu : size));
}

/* ---------- vectors */

static int vector_in(uint64_t vector, int count, struct iovec *host_vector, int writing)
{
	const struct guest_iovec *guest_vector = G2H(vector);
	int index;

	if (count < 0 || count > 64)
		return -1;
	for (index = 0; index < count; index++)
	{
		host_vector[index].iov_base = G2H(guest_vector[index].base);
		host_vector[index].iov_len = guest_vector[index].length;
		if (writing)
			prepare_write(guest_vector[index].base, guest_vector[index].length);
	}
	return 0;
}

/* ---------- directories: Linux's getdents64 over Darwin's readdir */

#define DIRECTORY_MAXIMUM 4096

static DIR *directories[DIRECTORY_MAXIMUM];
static pthread_mutex_t directory_lock = PTHREAD_MUTEX_INITIALIZER;

static void directory_forget(int fd)
{
	if (fd < 0 || fd >= DIRECTORY_MAXIMUM)
		return;
	pthread_mutex_lock(&directory_lock);
	if (directories[fd])
	{
		closedir(directories[fd]);
		directories[fd] = NULL;
	}
	pthread_mutex_unlock(&directory_lock);
}

static long guest_getdents64(int fd, uint64_t buffer, uint32_t size)
{
	unsigned char *out = G2H(buffer);
	long used = 0;
	DIR *stream;

	if (fd < 0 || fd >= DIRECTORY_MAXIMUM)
		return -EBADF;
	prepare_write(buffer, size);
	pthread_mutex_lock(&directory_lock);
	stream = directories[fd];
	if (!stream)
	{
		int copy = dup(fd);

		stream = copy >= 0 ? fdopendir(copy) : NULL;
		if (!stream)
		{
			int error = errno;

			if (copy >= 0)
				close(copy);
			pthread_mutex_unlock(&directory_lock);
			return -host_linux_errno(error);
		}
		directories[fd] = stream;
	}
	for (;;)
	{
		long position = telldir(stream);
		struct dirent *entry = readdir(stream);
		size_t length, record;

		if (!entry)
			break;
		length = strlen(entry->d_name);
		/* d_ino, d_off, d_reclen, d_type, the name and its NUL, 8-aligned */
		record = (19 + length + 1 + 7) & ~(size_t)7;
		if (used + (long)record > (long)size)
		{
			seekdir(stream, position);
			if (!used)
			{
				pthread_mutex_unlock(&directory_lock);
				return -EINVAL;
			}
			break;
		}
		memset(out + used, 0, record);
		*(uint64_t *)(out + used) = entry->d_ino;
		*(int64_t *)(out + used + 8) = telldir(stream);
		*(uint16_t *)(out + used + 16) = (uint16_t)record;
		out[used + 18] = entry->d_type;
		memcpy(out + used + 19, entry->d_name, length);
		used += (long)record;
	}
	pthread_mutex_unlock(&directory_lock);
	return used;
}

/* a directory's position, set by the guest's rewinddir and seekdir */
static int directory_seek(int fd, long long offset, int whence)
{
	int found = 0;

	if (fd < 0 || fd >= DIRECTORY_MAXIMUM || whence != SEEK_SET)
		return 0;
	pthread_mutex_lock(&directory_lock);
	if (directories[fd])
	{
		if (offset == 0)
			rewinddir(directories[fd]);
		else
			seekdir(directories[fd], (long)offset);
		found = 1;
	}
	pthread_mutex_unlock(&directory_lock);
	return found;
}

/* ---------- futexes */

static long guest_futex(uint64_t address, int operation, uint32_t value, uint64_t timeout, uint32_t value3)
{
	void *host = G2H(address);
	int command = operation & LINUX_FUTEX_COMMAND;
	int result;

	switch (command)
	{
	case LINUX_FUTEX_WAIT:
	case LINUX_FUTEX_WAIT_BITSET:
	{
		struct timespec relative;
		uint32_t microseconds = 0;

		if (timespec_in(timeout, &relative))
		{
			if (command == LINUX_FUTEX_WAIT_BITSET)
			{
				/* (an absolute time, on the monotonic clock unless it says
				otherwise) */
				struct timespec now;

				clock_gettime((operation & 256) ? CLOCK_REALTIME : CLOCK_MONOTONIC, &now);
				relative.tv_sec -= now.tv_sec;
				relative.tv_nsec -= now.tv_nsec;
				if (relative.tv_nsec < 0)
				{
					relative.tv_nsec += 1000000000;
					relative.tv_sec--;
				}
			}
			if (relative.tv_sec < 0)
				return -LINUX_ETIMEDOUT;
			/* (0 waits forever: a whole microsecond at least; and no more
			than an hour, after which the caller looks again) */
			if (relative.tv_sec >= 3600)
				microseconds = 3600u * 1000000u;
			else
				microseconds = (uint32_t)(relative.tv_sec * 1000000 + relative.tv_nsec / 1000);
			if (!microseconds)
				microseconds = 1;
		}
		result = __ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, host, value, microseconds);
		if (result >= 0)
			return 0;
		if (-result == ETIMEDOUT)
			return -LINUX_ETIMEDOUT;
		if (-result == EINTR)
			return -EINTR;
		return -LINUX_EAGAIN;
	}
	case LINUX_FUTEX_WAKE:
	case LINUX_FUTEX_WAKE_BITSET:
		if (!value)
			return 0;
		result = __ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO | (value > 1 ? ULF_WAKE_ALL : 0), host, 0);
		return result == 0 ? 1 : 0;
	case LINUX_FUTEX_REQUEUE:
	case LINUX_FUTEX_CMP_REQUEUE:
		/* no requeueing: every waiter wakes, and waits again on its own */
		if (command == LINUX_FUTEX_CMP_REQUEUE && *(volatile uint32_t *)host != value3)
			return -LINUX_EAGAIN;
		__ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO | ULF_WAKE_ALL, host, 0);
		return 0;
	default:
		return -LINUX_ENOSYS;
	}
}

/* ---------- dispatch */

static int64_t thread_id(void)
{
	uint64_t id = 0;

	pthread_threadid_np(NULL, &id);
	return (int32_t)(id & 0x7fffffff);
}

long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	switch (number)
	{
	/* ---------- files */
	case LINUX_SYS_read:
		prepare_write((uint64_t)b, (uint32_t)c);
		return result_of(read((int)a, G2H(b), (size_t)(uint32_t)c));
	case LINUX_SYS_write:
		return result_of(write((int)a, G2H(b), (size_t)(uint32_t)c));
	case LINUX_SYS_pread64:
		prepare_write((uint64_t)b, (uint32_t)c);
		return result_of(pread((int)a, G2H(b), (size_t)(uint32_t)c, (off_t)d));
	case LINUX_SYS_pwrite64:
		return result_of(pwrite((int)a, G2H(b), (size_t)(uint32_t)c, (off_t)d));
	case LINUX_SYS_readv:
	case LINUX_SYS_preadv:
	case LINUX_SYS_writev:
	case LINUX_SYS_pwritev:
	{
		struct iovec vector[64];
		int writing = number == LINUX_SYS_readv || number == LINUX_SYS_preadv;

		if (vector_in((uint64_t)b, (int)c, vector, writing) != 0)
			return -EINVAL;
		if (number == LINUX_SYS_readv)
			return result_of(readv((int)a, vector, (int)c));
		if (number == LINUX_SYS_writev)
			return result_of(writev((int)a, vector, (int)c));
		if (number == LINUX_SYS_preadv)
			return result_of(preadv((int)a, vector, (int)c, (off_t)d));
		return result_of(pwritev((int)a, vector, (int)c, (off_t)d));
	}
	case LINUX_SYS_openat:
	{
		const char *path = G2H(b);

		/* (the Linux process information the platform layer asks for is
		readlinkat's /proc/self/exe; nothing else of /proc exists) */
		if (path && !strncmp(path, "/proc/", 6))
			return -ENOENT;
		return result_of(openat(directory(a), path, open_flags(c), (int)d));
	}
	case LINUX_SYS_close:
		directory_forget((int)a);
		return result_of(close((int)a));
	case LINUX_SYS_lseek:
		if (directory_seek((int)a, b, (int)c))
			return b;
		return result_of((long)lseek((int)a, (off_t)b, (int)c));
	case LINUX_SYS_getdents64:
		return guest_getdents64((int)a, (uint64_t)b, (uint32_t)c);
	case LINUX_SYS_fstat:
	{
		struct stat information;

		if (fstat((int)a, &information) != 0)
			return -host_errno();
		stat_out((uint64_t)b, &information);
		return 0;
	}
	case LINUX_SYS_newfstatat:
	{
		struct stat information;
		int result;

		if ((d & LINUX_AT_EMPTY_PATH) && !*(const char *)G2H(b))
			result = fstat((int)a, &information);
		else
			result = fstatat(directory(a), G2H(b), &information,
				(d & LINUX_AT_SYMLINK_NOFOLLOW) ? AT_SYMLINK_NOFOLLOW : 0);
		if (result != 0)
			return -host_errno();
		stat_out((uint64_t)c, &information);
		return 0;
	}
	case LINUX_SYS_readlinkat:
	{
		const char *path = G2H(b);

		prepare_write((uint64_t)c, (uint32_t)d);
		if (path && !strcmp(path, "/proc/self/exe"))
		{
			size_t length = strlen(host_executable_path);

			if (length > (size_t)(uint32_t)d)
				length = (size_t)(uint32_t)d;
			memcpy(G2H(c), host_executable_path, length);
			return (long)length;
		}
		return result_of(readlinkat(directory(a), path, G2H(c), (size_t)(uint32_t)d));
	}
	case LINUX_SYS_unlinkat:
		return result_of(unlinkat(directory(a), G2H(b), (c & LINUX_AT_REMOVEDIR) ? AT_REMOVEDIR : 0));
	case LINUX_SYS_renameat:
		return result_of(renameat(directory(a), G2H(b), directory(c), G2H(d)));
	case LINUX_SYS_renameat2:
		if (e)
			return -EINVAL;
		return result_of(renameat(directory(a), G2H(b), directory(c), G2H(d)));
	case LINUX_SYS_mkdirat:
		return result_of(mkdirat(directory(a), G2H(b), (mode_t)c));
	case LINUX_SYS_fchmod:
		return result_of(fchmod((int)a, (mode_t)b));
	case LINUX_SYS_fchmodat:
		return result_of(fchmodat(directory(a), G2H(b), (mode_t)c, 0));
	case LINUX_SYS_ftruncate:
		return result_of(ftruncate((int)a, (off_t)b));
	case LINUX_SYS_fsync:
	case LINUX_SYS_fdatasync:
		return result_of(fsync((int)a));
	case LINUX_SYS_faccessat:
	case LINUX_SYS_faccessat2:
		return result_of(faccessat(directory(a), G2H(b), (int)c,
			(number == LINUX_SYS_faccessat2 && (d & LINUX_AT_EACCESS)) ? AT_EACCESS : 0));
	case LINUX_SYS_utimensat:
	{
		struct timespec times[2];
		uint64_t guest_times = (uint64_t)(uint32_t)c;

		if (guest_times)
		{
			timespec_in(guest_times, &times[0]);
			timespec_in(guest_times + sizeof(struct guest_timespec), &times[1]);
		}
		return result_of(utimensat(directory(a), G2H(b), guest_times ? times : NULL,
			(d & LINUX_AT_SYMLINK_NOFOLLOW) ? AT_SYMLINK_NOFOLLOW : 0));
	}
	case LINUX_SYS_fcntl:
		switch ((int)b)
		{
		case F_DUPFD:
		case F_GETFD:
		case F_SETFD:
			return result_of(fcntl((int)a, (int)b, (int)c));
		case LINUX_F_DUPFD_CLOEXEC:
			return result_of(fcntl((int)a, F_DUPFD_CLOEXEC, (int)c));
		case F_GETFL:
		{
			int flags = fcntl((int)a, F_GETFL);

			return flags < 0 ? -host_errno() : linux_status_flags(flags);
		}
		case F_SETFL:
			return result_of(fcntl((int)a, F_SETFL, open_flags(c & (LINUX_O_APPEND | LINUX_O_NONBLOCK))));
		default:
			return -EINVAL;
		}
	case LINUX_SYS_flock:
		return result_of(flock((int)a, (int)b));
	case LINUX_SYS_getcwd:
	{
		char *buffer = G2H(a);

		prepare_write((uint64_t)a, (uint32_t)b);
		if (!getcwd(buffer, (size_t)(uint32_t)b))
			return -host_errno();
		return (long)strlen(buffer) + 1;
	}
	case LINUX_SYS_chdir:
		return result_of(chdir(G2H(a)));
	case LINUX_SYS_dup:
		return result_of(dup((int)a));
	case LINUX_SYS_dup3:
	{
		int result = dup2((int)a, (int)b);

		if (result >= 0 && (c & LINUX_O_CLOEXEC))
			fcntl(result, F_SETFD, FD_CLOEXEC);
		directory_forget((int)b);
		return result_of(result);
	}
	case LINUX_SYS_pipe2:
	{
		int descriptors[2];
		int *result = G2H(a);

		if (pipe(descriptors) != 0)
			return -host_errno();
		if (b & LINUX_O_CLOEXEC)
		{
			fcntl(descriptors[0], F_SETFD, FD_CLOEXEC);
			fcntl(descriptors[1], F_SETFD, FD_CLOEXEC);
		}
		if (b & LINUX_O_NONBLOCK)
		{
			fcntl(descriptors[0], F_SETFL, O_NONBLOCK);
			fcntl(descriptors[1], F_SETFL, O_NONBLOCK);
		}
		result[0] = descriptors[0];
		result[1] = descriptors[1];
		return 0;
	}
	case LINUX_SYS_ioctl:
		if ((unsigned long)b == LINUX_TIOCGWINSZ && isatty((int)a))
		{
			struct winsize size;

			if (ioctl((int)a, TIOCGWINSZ, &size) != 0)
				return -host_errno();
			memcpy(G2H(c), &size, sizeof(size));
			return 0;
		}
		return -ENOTTY;
	case LINUX_SYS_umask:
		return umask((mode_t)a);

	/* ---------- time */
	case LINUX_SYS_clock_gettime:
	case LINUX_SYS_clock_getres:
	{
		struct timespec value;
		int clock = clock_from_linux(a);
		int result;

		if (clock < 0)
			return -EINVAL;
		result = number == LINUX_SYS_clock_gettime ? clock_gettime(clock, &value) : clock_getres(clock, &value);
		if (result != 0)
			return -host_errno();
		timespec_out((uint64_t)b, &value);
		return 0;
	}
	case LINUX_SYS_gettimeofday:
	{
		struct timespec value;
		struct guest_timespec *result = G2H(a);

		clock_gettime(CLOCK_REALTIME, &value);
		if (result)
		{
			result->seconds = (int32_t)value.tv_sec;
			result->nanoseconds = (int32_t)(value.tv_nsec / 1000);
		}
		return 0;
	}
	case LINUX_SYS_nanosleep:
	{
		struct timespec request, remaining;
		long result;

		if (!timespec_in((uint64_t)a, &request))
			return -EFAULT;
		result = result_of(nanosleep(&request, &remaining));
		if (result == -EINTR)
			timespec_out((uint64_t)b, &remaining);
		return result;
	}
	case LINUX_SYS_clock_nanosleep:
	{
		struct timespec request;
		int clock = clock_from_linux(a);

		if (clock < 0)
			return -EINVAL;
		if (!timespec_in((uint64_t)c, &request))
			return -EFAULT;
		if (b & 1)
		{
			/* TIMER_ABSTIME: until then */
			struct timespec now;

			clock_gettime(clock, &now);
			request.tv_sec -= now.tv_sec;
			request.tv_nsec -= now.tv_nsec;
			if (request.tv_nsec < 0)
			{
				request.tv_nsec += 1000000000;
				request.tv_sec--;
			}
			if (request.tv_sec < 0)
				return 0;
		}
		return result_of(nanosleep(&request, NULL));
	}

	/* ---------- threads and synchronisation */
	case LINUX_SYS_futex:
		return guest_futex((uint64_t)a, (int)b, (uint32_t)c, (uint64_t)(uint32_t)d, (uint32_t)f);
	case LINUX_SYS_sched_yield:
		sched_yield();
		return 0;
	case LINUX_SYS_sched_getaffinity:
	{
		/* as many processors as the machine has */
		int processors = 1;
		size_t length = sizeof(processors);
		unsigned char *mask = G2H(c);
		uint32_t size = (uint32_t)b < 128 ? (uint32_t)b : 128;
		int index;

		sysctlbyname("hw.activecpu", &processors, &length, NULL, 0);
		memset(mask, 0, size);
		for (index = 0; index < processors && index / 8 < (int)size; index++)
			mask[index / 8] |= (unsigned char)(1 << (index % 8));
		return size;
	}
	case LINUX_SYS_set_tid_address:
	case LINUX_SYS_gettid:
		return thread_id();

	/* ---------- the process */
	case LINUX_SYS_getpid:
		return getpid();
	case LINUX_SYS_getppid:
		return getppid();
	case LINUX_SYS_getuid:
		return getuid();
	case LINUX_SYS_geteuid:
		return geteuid();
	case LINUX_SYS_getgid:
		return getgid();
	case LINUX_SYS_getegid:
		return getegid();
	case LINUX_SYS_exit:
	case LINUX_SYS_exit_group:
		host_exit((int)a);
	case LINUX_SYS_kill:
	case LINUX_SYS_tkill:
	case LINUX_SYS_tgkill:
	{
		int signal_number = (int)(number == LINUX_SYS_tgkill ? c : b);

		/* (only the guest's abort() signals itself) */
		if (signal_number == 0)
			return 0;
		if (signal_number == LINUX_SIGABRT)
		{
			host_logf(HOST_LOG_ERROR, "the guest aborted");
			abort();
		}
		return -LINUX_ENOSYS;
	}
	case LINUX_SYS_rt_sigprocmask:
		if (c)
			memset(G2H(c), 0, 8);
		return 0;
	case LINUX_SYS_rt_sigaction:
	case LINUX_SYS_sigaltstack:
		/* the host owns signal handling */
		return 0;
	case LINUX_SYS_uname:
	{
		struct utsname host;
		struct guest_utsname *result = G2H(a);

		memset(result, 0, sizeof(*result));
		if (uname(&host) != 0)
			return -host_errno();
		snprintf(result->sysname, sizeof(result->sysname), "%s", host.sysname);
		snprintf(result->nodename, sizeof(result->nodename), "%s", host.nodename);
		snprintf(result->release, sizeof(result->release), "%s", host.release);
		snprintf(result->version, sizeof(result->version), "%s", host.version);
		snprintf(result->machine, sizeof(result->machine), "%s", host.machine);
		return 0;
	}
	case LINUX_SYS_getrlimit:
	case LINUX_SYS_prlimit64:
	{
		long long resource = number == LINUX_SYS_prlimit64 ? b : a;
		uint64_t *result = G2H(number == LINUX_SYS_prlimit64 ? d : b);
		struct rlimit limit = { RLIM_INFINITY, RLIM_INFINITY };

		if (number == LINUX_SYS_prlimit64 && c)
			return -EPERM;
		if (resource == LINUX_RLIMIT_NOFILE)
			getrlimit(RLIMIT_NOFILE, &limit);
		else if (resource == LINUX_RLIMIT_STACK)
			getrlimit(RLIMIT_STACK, &limit);
		if (result)
		{
			result[0] = limit.rlim_cur == RLIM_INFINITY ? ~0ull : (uint64_t)limit.rlim_cur;
			result[1] = limit.rlim_max == RLIM_INFINITY ? ~0ull : (uint64_t)limit.rlim_max;
		}
		return 0;
	}
	case LINUX_SYS_getrandom:
		arc4random_buf(G2H(a), (size_t)(uint32_t)b);
		return (uint32_t)b;

	/* ---------- memory */
	case LINUX_SYS_mmap:
		return host_guest_mmap((uint32_t)a, (uint64_t)b, (int)c, (int)d, (int)e, f);
	case LINUX_SYS_munmap:
		return host_guest_munmap((uint32_t)a, (uint64_t)b);
	case LINUX_SYS_mprotect:
		return host_guest_mprotect((uint32_t)a, (uint64_t)b, (int)c);
	case LINUX_SYS_madvise:
		return host_guest_madvise((uint32_t)a, (uint64_t)b, (int)c);
	case LINUX_SYS_mremap:
	case LINUX_SYS_brk:
		/* musl then falls back to mmap and copying */
		return -ENOMEM;

	case LINUX_SYS_membarrier:
	case LINUX_SYS_statx:
	case LINUX_SYS_statfs:
	case LINUX_SYS_fstatfs:
	case LINUX_SYS_sysinfo:
	case LINUX_SYS_pselect6:
	case LINUX_SYS_epoll_pwait:
	case LINUX_SYS_linkat:
	case LINUX_SYS_symlinkat:
		return -LINUX_ENOSYS;

	case LINUX_SYS_ppoll:
	{
		struct timespec timeout;
		int milliseconds = -1;

		if (timespec_in((uint64_t)c, &timeout))
			milliseconds = (int)(timeout.tv_sec * 1000 + timeout.tv_nsec / 1000000);
		return result_of(poll(G2H(a), (nfds_t)(uint32_t)b, milliseconds));
	}

	default:
		host_logf(HOST_LOG_WARN, "guest system call %lld is not supported", number);
		return -LINUX_ENOSYS;
	}
}
