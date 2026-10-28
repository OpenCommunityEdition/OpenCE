#include "host.h"
#include "bits/syscall.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#define GUEST(type, value) ((type)HALO_GUEST_ADDRESS(value))

#define LINUX_AT_FDCWD -100
#define LINUX_FUTEX_WAIT 0
#define LINUX_FUTEX_WAKE 1
#define LINUX_CLOCK_MONOTONIC 1
#define LINUX_CLOCK_PROCESS_CPUTIME_ID 2
#define LINUX_CLOCK_THREAD_CPUTIME_ID 3
#define LINUX_CLOCK_MONOTONIC_RAW 4
#define LINUX_TIMER_ABSTIME 1

#define ULOCK_COMPARE_AND_WAIT 1
#define ULOCK_WAKE_ALL 0x100
#define ULOCK_NO_ERRNO 0x1000000
int __ulock_wait(uint32_t operation, void *address, uint64_t value, uint32_t timeout_microseconds);
int __ulock_wake(uint32_t operation, void *address, uint64_t wake_value);

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

#ifdef __aarch64__
struct guest_stat
{
	uint64_t device, inode;
	uint32_t mode, links, user, group;
	uint64_t special_device, pad;
	int64_t size;
	int32_t block_size, pad2;
	int64_t blocks;
	int64_t access_seconds, access_nanoseconds;
	int64_t modification_seconds, modification_nanoseconds;
	int64_t change_seconds, change_nanoseconds;
	uint32_t unused[2];
};
#else
struct guest_stat
{
	uint64_t device, inode;
	uint32_t links, mode, user, group, pad;
	uint64_t special_device;
	int64_t size;
	int32_t block_size;
	int64_t blocks;
	int64_t access_seconds;
	int32_t access_nanoseconds;
	int64_t modification_seconds;
	int32_t modification_nanoseconds;
	int64_t change_seconds;
	int32_t change_nanoseconds;
	int64_t unused[3];
};
#endif

int host_linux_errno(int error)
{
	static const unsigned char linux_errors[] =
	{
		[EDEADLK] = 35, [EAGAIN] = 11, [EINPROGRESS] = 115, [EALREADY] = 114, [ENOTSOCK] = 88,
		[EDESTADDRREQ] = 89, [EMSGSIZE] = 90, [EPROTOTYPE] = 91, [ENOPROTOOPT] = 92, [EPROTONOSUPPORT] = 93,
		[ESOCKTNOSUPPORT] = 94, [ENOTSUP] = 95, [EPFNOSUPPORT] = 96, [EAFNOSUPPORT] = 97, [EADDRINUSE] = 98,
		[EADDRNOTAVAIL] = 99, [ENETDOWN] = 100, [ENETUNREACH] = 101, [ENETRESET] = 102, [ECONNABORTED] = 103,
		[ECONNRESET] = 104, [ENOBUFS] = 105, [EISCONN] = 106, [ENOTCONN] = 107, [ESHUTDOWN] = 108,
		[ETOOMANYREFS] = 109, [ETIMEDOUT] = 110, [ECONNREFUSED] = 111, [ELOOP] = 40, [ENAMETOOLONG] = 36,
		[EHOSTDOWN] = 112, [EHOSTUNREACH] = 113, [ENOTEMPTY] = 39, [EUSERS] = 87, [EDQUOT] = 122, [ESTALE] = 116,
		[ENOLCK] = 37, [ENOSYS] = 38, [EOVERFLOW] = 75, [ECANCELED] = 125, [EIDRM] = 43, [ENOMSG] = 42,
		[EILSEQ] = 84, [EBADMSG] = 74, [EMULTIHOP] = 72, [ENODATA] = 61, [ENOLINK] = 67, [ENOSR] = 63,
		[ENOSTR] = 60, [EPROTO] = 71, [ETIME] = 62, [EOWNERDEAD] = 130, [ENOTRECOVERABLE] = 131,
	};

	if (error > 0 && error < (int)sizeof(linux_errors) && linux_errors[error])
		return linux_errors[error];
	return error < 35 ? error : 5 ;
}

static long result_of(long value)
{
	return value == -1 ? -host_linux_errno(errno) : value;
}

static int timespec_in(uint64_t address, struct timespec *result)
{
	const struct guest_timespec *value = GUEST(const struct guest_timespec *, address);

	if (!address)
		return 0;
	result->tv_sec = value->seconds;
	result->tv_nsec = value->nanoseconds;
	return 1;
}

static void timespec_out(uint64_t address, const struct timespec *value)
{
	struct guest_timespec *result = GUEST(struct guest_timespec *, address);

	if (!address)
		return;
	result->seconds = (int32_t)value->tv_sec;
	result->nanoseconds = (int32_t)value->tv_nsec;
}

static int open_flags(int flags)
{
	static const int pairs[][2] =
	{
		{ 0100, O_CREAT }, { 0200, O_EXCL }, { 0400, O_NOCTTY }, { 01000, O_TRUNC }, { 02000, O_APPEND },
		{ 04000, O_NONBLOCK }, { 0200000, O_DIRECTORY }, { 0400000, O_NOFOLLOW }, { 02000000, O_CLOEXEC },
	};
	int result = flags & O_ACCMODE, index;

	for (index = 0; index < (int)(sizeof(pairs) / sizeof(pairs[0])); index++)
	{
		if (flags & pairs[index][0])
			result |= pairs[index][1];
	}
	return result;
}

static int linux_open_flags(int flags)
{
	return (flags & O_ACCMODE) | (flags & O_APPEND ? 02000 : 0) | (flags & O_NONBLOCK ? 04000 : 0);
}

static int folder(long long fd)
{
	return (int)fd == LINUX_AT_FDCWD ? AT_FDCWD : (int)fd;
}

static clockid_t clock_of(long long clock)
{
	switch ((int)clock)
	{
	case LINUX_CLOCK_MONOTONIC:
	case LINUX_CLOCK_MONOTONIC_RAW:
		return CLOCK_MONOTONIC;
	case LINUX_CLOCK_PROCESS_CPUTIME_ID:
		return CLOCK_PROCESS_CPUTIME_ID;
	case LINUX_CLOCK_THREAD_CPUTIME_ID:
		return CLOCK_THREAD_CPUTIME_ID;
	default:
		return CLOCK_REALTIME;
	}
}

static long stat_out(int result, const struct stat *information, uint64_t address)
{
	struct guest_stat *value = GUEST(struct guest_stat *, address);

	if (result != 0)
		return -host_linux_errno(errno);
	memset(value, 0, sizeof(*value));
	value->device = (uint64_t)information->st_dev;
	value->inode = information->st_ino;
	value->links = information->st_nlink;
	value->mode = information->st_mode;
	value->user = information->st_uid;
	value->group = information->st_gid;
	value->special_device = (uint64_t)information->st_rdev;
	value->size = information->st_size;
	value->block_size = information->st_blksize;
	value->blocks = information->st_blocks;
	value->access_seconds = information->st_atimespec.tv_sec;
	value->access_nanoseconds = (int32_t)information->st_atimespec.tv_nsec;
	value->modification_seconds = information->st_mtimespec.tv_sec;
	value->modification_nanoseconds = (int32_t)information->st_mtimespec.tv_nsec;
	value->change_seconds = information->st_ctimespec.tv_sec;
	value->change_nanoseconds = (int32_t)information->st_ctimespec.tv_nsec;
	return 0;
}

static long guest_vector(int fd, uint64_t vector, int count, int writing)
{
	struct iovec host_vector[64];
	const struct guest_iovec *guest_vector = GUEST(const struct guest_iovec *, vector);
	int index;

	if (count < 0 || count > 64)
		return -EINVAL;
	for (index = 0; index < count; index++)
	{
		host_vector[index].iov_base = GUEST(void *, guest_vector[index].base);
		host_vector[index].iov_len = guest_vector[index].length;
	}
	return result_of(writing ? writev(fd, host_vector, count) : readv(fd, host_vector, count));
}

static long guest_futex(uint64_t address, int operation, uint32_t value, uint64_t timeout)
{
	struct timespec duration;
	uint64_t microseconds = 0;
	int result;

	switch (operation & 127)
	{
	case LINUX_FUTEX_WAIT:
		if (timespec_in(timeout, &duration))
		{
			microseconds = (uint64_t)duration.tv_sec * 1000000 + (uint64_t)duration.tv_nsec / 1000;
			microseconds = microseconds < 1 ? 1 : microseconds > UINT32_MAX ? UINT32_MAX : microseconds;
		}
		result = __ulock_wait(ULOCK_COMPARE_AND_WAIT | ULOCK_NO_ERRNO, GUEST(void *, address), value,
			(uint32_t)microseconds);
		return result >= 0 ? 0 : result == -ETIMEDOUT ? -110 : result == -EINTR ? -EINTR : -11 ;
	case LINUX_FUTEX_WAKE:
		__ulock_wake(ULOCK_COMPARE_AND_WAIT | ULOCK_NO_ERRNO | (value > 1 ? ULOCK_WAKE_ALL : 0),
			GUEST(void *, address), 0);
		return 0;
	default:
		return -38 ;
	}
}

long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	(void)f;
#ifdef SYS_open
	switch (number)
	{
	case SYS_open:
		return host_syscall(SYS_openat, LINUX_AT_FDCWD, a, b, c, 0, 0);
	case SYS_unlink:
		return host_syscall(SYS_unlinkat, LINUX_AT_FDCWD, a, 0, 0, 0, 0);
	case SYS_rename:
		return host_syscall(SYS_renameat, LINUX_AT_FDCWD, a, LINUX_AT_FDCWD, b, 0, 0);
	case SYS_readlink:
		return host_syscall(SYS_readlinkat, LINUX_AT_FDCWD, a, b, c, 0, 0);
	}
#endif
	switch (number)
	{
	case SYS_read:
		return result_of(read((int)a, GUEST(void *, b), (size_t)(uint32_t)c));
	case SYS_write:
		return result_of(write((int)a, GUEST(const void *, b), (size_t)(uint32_t)c));
	case SYS_pread64:
		return result_of(pread((int)a, GUEST(void *, b), (size_t)(uint32_t)c, d));
	case SYS_pwrite64:
		return result_of(pwrite((int)a, GUEST(const void *, b), (size_t)(uint32_t)c, d));
	case SYS_readv:
		return guest_vector((int)a, (uint64_t)b, (int)c, 0);
	case SYS_writev:
		return guest_vector((int)a, (uint64_t)b, (int)c, 1);
	case SYS_openat:
		return result_of(openat(folder(a), GUEST(const char *, b), open_flags((int)c), (int)d));
	case SYS_close:
		return result_of(close((int)a));
	case SYS_lseek:
		return result_of(lseek((int)a, b, (int)c));
	case SYS_fstat:
	{
		struct stat information;

		return stat_out(fstat((int)a, &information), &information, (uint64_t)b);
	}
	case SYS_fcntl:
		switch ((int)b)
		{
		case F_GETFD:
		case F_SETFD:
			return result_of(fcntl((int)a, (int)b, (int)c));
		case F_GETFL:
		{
			int flags = fcntl((int)a, F_GETFL);

			return flags < 0 ? result_of(flags) : linux_open_flags(flags);
		}
		case F_SETFL:
			return result_of(fcntl((int)a, F_SETFL, open_flags((int)c)));
		default:
			return -EINVAL;
		}
	case SYS_unlinkat:
		return result_of(unlinkat(folder(a), GUEST(const char *, b), 0));
	case SYS_renameat:
		return result_of(renameat(folder(a), GUEST(const char *, b), folder(c), GUEST(const char *, d)));
	case SYS_getcwd:
		return getcwd(GUEST(char *, a), (size_t)(uint32_t)b) ? (long long)strlen(GUEST(char *, a)) + 1 :
			-host_linux_errno(errno);
	case SYS_readlinkat:
	{
		const char *name = GUEST(const char *, b);
		char *buffer = GUEST(char *, c);
		size_t size = (size_t)(uint32_t)d;
		char exe[1100];

		if (!strcmp(name, "/proc/self/exe"))
		{
			snprintf(exe, sizeof(exe), "%s/halo", host_data_folder);
			size = size < strlen(exe) ? size : strlen(exe);
			memcpy(buffer, exe, size);
			return (long long)size;
		}
		return result_of(readlinkat(folder(a), name, buffer, size));
	}

	case SYS_clock_gettime:
	{
		struct timespec value;
		int result = clock_gettime(clock_of(a), &value);

		if (result == 0)
			timespec_out((uint64_t)b, &value);
		return result_of(result);
	}
	case SYS_nanosleep:
	case SYS_clock_nanosleep:
	{
		struct timespec request, remaining;
		uint64_t time = (uint64_t)(number == SYS_nanosleep ? a : c);
		long result;

		if (!timespec_in(time, &request))
			return -EFAULT;
		if (number == SYS_clock_nanosleep && ((int)b & LINUX_TIMER_ABSTIME))
		{
			struct timespec now;

			clock_gettime(clock_of(a), &now);
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
		result = result_of(nanosleep(&request, &remaining));
		if (result == -EINTR)
			timespec_out((uint64_t)(number == SYS_nanosleep ? b : d), &remaining);
		return result;
	}
	case SYS_futex:
		return guest_futex((uint64_t)a, (int)b, (uint32_t)c, (uint64_t)d);

	case SYS_mmap:
		return host_guest_mmap((uint64_t)a, (uint64_t)b, (int)c, (int)d, (int)e, f);
	case SYS_munmap:
		return host_guest_munmap((uint64_t)a, (uint64_t)b);
	case SYS_mprotect:
		return host_guest_mprotect((uint64_t)a, (uint64_t)b, (int)c);
	case SYS_madvise:
		return host_guest_madvise((uint64_t)a, (uint64_t)b, (int)c);
	case SYS_mremap:
	case SYS_brk:
		return -ENOMEM;

	case SYS_exit:
	case SYS_exit_group:
		host_exit((int)a);
	case SYS_getpid:
		return getpid();
	case SYS_gettid:
	{
		uint64_t thread = 0;

		pthread_threadid_np(NULL, &thread);
		return (long long)(thread & 0x7fffffff);
	}
	case SYS_sched_yield:
		return sched_yield();
	case SYS_rt_sigaction:
		return 0;
	case SYS_ioctl:
		return -ENOTTY;
	case SYS_sysinfo:
		return -38 ;

	default:
		host_logf(HOST_LOG_WARN, "guest system call %lld is not supported", number & 0xffff);
		return -38 ;
	}
}
