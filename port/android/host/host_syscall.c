/*
HOST_SYSCALL.C

System calls on behalf of the guest's musl runtime (its syscall_arch.h
sends every call here).

The guest has a system call interface of its own
(port/android/include/halo_guest_syscall.h): its numbers, its kernel stat
layout and its open flags are AArch64's, whatever the host's architecture
is. This file translates each call to the host's kernel. It never passes a
guest number to the kernel: a call is chosen by its guest number and made
with the host's number, or with the C library's function.

Per call:

- most take integers, strings and buffers, which pass unchanged: the
  guest's pointers are valid host addresses (zero-extended), and the kernel
  reads an int argument from the low 32 bits of its register on every
  architecture, so a negative int (AT_FDCWD) arrives correctly;
- constants that are the same on every host architecture (AT_, SEEK_,
  PROT_, MAP_, POLL, signals, RLIMIT_, LOCK_, errno) pass unchanged, and
  the build stops on a host whose headers disagree (the checks below);
- open's flags differ (x86-64's O_DIRECTORY is AArch64's O_DIRECT, for
  one), so openat, pipe2, dup3 and fcntl's F_GETFL and F_SETFL translate
  them;
- structures with the guest's ILP32 layouts are converted: timespec and
  timeval (the guest's time_t is 32-bit, as in the MSVC runtime), iovec,
  struct sysinfo (its longs are 32-bit), and the kernel stat of fstat and
  newfstatat; fcntl's struct flock is copied field by field;
- a 64-bit argument the guest passes in two halves (preadv's and pwritev's
  offset, as musl passes it on 32-bit architectures) is put together;
- structures that are the same on every 64-bit Linux pass unchanged: the
  directory entries of getdents64 (struct linux_dirent64), pollfd, utsname,
  rlimit64 (prlimit64, and getrlimit, which becomes prlimit64), the signal
  mask of rt_sigprocmask (8 bytes), f_owner_ex;
- memory mappings must stay below 4 GB (host_memory.c);
- the standard output and error streams go to logcat (on Android; on Linux
  arm64, port/linux/host, they are the process's);
- process exit, and calls that have no meaning for the guest (signal
  handlers, which the host owns), are answered here;
- a call the guest's interface has but the host does not perform returns
  -ENOSYS.

Results: the guest's system calls return a 32-bit long (guest_syscall.c).
Every result fits: sizes are limited by the guest's 32-bit size_t and by
the kernel (a read or write moves less than 2 GB), addresses lie below
4 GB, and the one 64-bit result, a file offset, is returned through
_llseek's pointer.
*/

#include "host.h"
#include "halo_guest_syscall.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

/* ---------- the values passed unchanged (halo_guest_syscall.h) */

#define SAME(name) _Static_assert(name == GUEST_##name, "the host's " #name " differs from the guest's")
SAME(AT_FDCWD);
SAME(AT_SYMLINK_NOFOLLOW);
SAME(AT_REMOVEDIR);
SAME(AT_EMPTY_PATH);
SAME(SEEK_SET);
SAME(SEEK_CUR);
SAME(SEEK_END);
SAME(PROT_READ);
SAME(PROT_WRITE);
SAME(PROT_EXEC);
SAME(MAP_SHARED);
SAME(MAP_PRIVATE);
SAME(MAP_FIXED);
SAME(MAP_ANONYMOUS);
SAME(MAP_NORESERVE);
SAME(MAP_FIXED_NOREPLACE);
SAME(MADV_DONTNEED);
SAME(MADV_FREE);
SAME(POLLIN);
SAME(POLLPRI);
SAME(POLLOUT);
SAME(POLLERR);
SAME(POLLHUP);
SAME(POLLNVAL);
SAME(SIG_BLOCK);
SAME(SIG_UNBLOCK);
SAME(SIG_SETMASK);
SAME(SIGABRT);
SAME(SIGKILL);
SAME(SIGSEGV);
SAME(SIGTERM);
SAME(RLIMIT_CPU);
SAME(RLIMIT_FSIZE);
SAME(RLIMIT_DATA);
SAME(RLIMIT_STACK);
SAME(RLIMIT_CORE);
SAME(RLIMIT_NOFILE);
SAME(RLIMIT_AS);
SAME(TIMER_ABSTIME);
SAME(LOCK_SH);
SAME(LOCK_EX);
SAME(LOCK_NB);
SAME(LOCK_UN);
SAME(F_RDLCK);
SAME(F_WRLCK);
SAME(F_UNLCK);
SAME(FD_CLOEXEC);
SAME(O_ACCMODE);
SAME(O_RDONLY);
SAME(O_WRONLY);
SAME(O_RDWR);
SAME(EPERM);
SAME(ENOENT);
SAME(EINTR);
SAME(EBADF);
SAME(EAGAIN);
SAME(ENOMEM);
SAME(EACCES);
SAME(EFAULT);
SAME(EEXIST);
SAME(ENOTDIR);
SAME(EISDIR);
SAME(EINVAL);
SAME(ENOSPC);
SAME(ERANGE);
SAME(EDEADLK);
SAME(ENAMETOOLONG);
SAME(ENOSYS);
SAME(ENOTEMPTY);
SAME(ELOOP);
SAME(EOVERFLOW);
SAME(EOPNOTSUPP);
SAME(ETIMEDOUT);
#undef SAME

#define GUEST(type, value) ((type)(uintptr_t)(uint32_t)(value))

static long result_of(long value)
{
	return value == -1 ? -errno : value;
}

/* ---------- open's flags */

struct open_flag
{
	uint32_t guest;
	int host;
};

/* each of the guest's bits and the host's for it; O_SYNC and O_TMPFILE are
a bit of their own with O_DSYNC and O_DIRECTORY. A 64-bit host's C library
may define O_LARGEFILE as 0 (its kernel sets it on every file). */
static const struct open_flag open_flags[] =
{
	{GUEST_O_CREAT, O_CREAT},
	{GUEST_O_EXCL, O_EXCL},
	{GUEST_O_NOCTTY, O_NOCTTY},
	{GUEST_O_TRUNC, O_TRUNC},
	{GUEST_O_APPEND, O_APPEND},
	{GUEST_O_NONBLOCK, O_NONBLOCK},
	{GUEST_O_DSYNC, O_DSYNC},
	{GUEST_O_ASYNC, O_ASYNC},
	{GUEST_O_DIRECTORY, O_DIRECTORY},
	{GUEST_O_NOFOLLOW, O_NOFOLLOW},
	{GUEST_O_DIRECT, O_DIRECT},
	{GUEST_O_LARGEFILE, O_LARGEFILE},
	{GUEST_O_NOATIME, O_NOATIME},
	{GUEST_O_CLOEXEC, O_CLOEXEC},
	{GUEST_O_SYNC_BIT, O_SYNC & ~O_DSYNC},
	{GUEST_O_PATH, O_PATH},
	{GUEST_O_TMPFILE_BIT, O_TMPFILE & ~O_DIRECTORY},
};

#define OPEN_FLAG_COUNT ((int)(sizeof(open_flags) / sizeof(open_flags[0])))

/* the host's flags for the guest's; -EINVAL for a bit the guest's
interface does not have */
static long open_flags_in(uint32_t guest)
{
	uint32_t remaining = guest & ~(uint32_t)GUEST_O_ACCMODE;
	long host = guest & GUEST_O_ACCMODE;
	int index;

	for (index = 0; index < OPEN_FLAG_COUNT; index++)
	{
		if (remaining & open_flags[index].guest)
		{
			host |= open_flags[index].host;
			remaining &= ~open_flags[index].guest;
		}
	}
	return remaining ? -EINVAL : host;
}

/* the guest's flags for the host's (F_GETFL); bits the guest's interface
does not have are left out */
static uint32_t open_flags_out(long host)
{
	uint32_t guest = (uint32_t)(host & O_ACCMODE);
	int index;

	for (index = 0; index < OPEN_FLAG_COUNT; index++)
	{
		if (open_flags[index].host && (host & open_flags[index].host) == open_flags[index].host)
			guest |= open_flags[index].guest;
	}
	return guest;
}

/* ---------- structures */

static int timespec_in(uint64_t address, struct timespec *result)
{
	const struct halo_guest_timespec *value = GUEST(const struct halo_guest_timespec *, address);

	if (!address)
		return 0;
	result->tv_sec = value->seconds;
	result->tv_nsec = value->nanoseconds;
	return 1;
}

static void timespec_out(uint64_t address, const struct timespec *value)
{
	struct halo_guest_timespec *result = GUEST(struct halo_guest_timespec *, address);

	if (!address)
		return;
	result->seconds = (int32_t)value->tv_sec;
	result->nanoseconds = (int32_t)value->tv_nsec;
}

/* the host's struct stat, from its C library, in the guest's kernel stat
layout */
static void kstat_out(const struct stat *value, struct halo_guest_kstat *result)
{
	memset(result, 0, sizeof(*result));
	result->st_dev = (uint64_t)value->st_dev;
	result->st_ino = (uint64_t)value->st_ino;
	result->st_mode = (uint32_t)value->st_mode;
	result->st_nlink = (uint32_t)value->st_nlink;
	result->st_uid = (uint32_t)value->st_uid;
	result->st_gid = (uint32_t)value->st_gid;
	result->st_rdev = (uint64_t)value->st_rdev;
	result->st_size = (int64_t)value->st_size;
	result->st_blksize = (int32_t)value->st_blksize;
	result->st_blocks = (int64_t)value->st_blocks;
	result->atime_seconds = (int64_t)value->st_atim.tv_sec;
	result->atime_nanoseconds = (int64_t)value->st_atim.tv_nsec;
	result->mtime_seconds = (int64_t)value->st_mtim.tv_sec;
	result->mtime_nanoseconds = (int64_t)value->st_mtim.tv_nsec;
	result->ctime_seconds = (int64_t)value->st_ctim.tv_sec;
	result->ctime_nanoseconds = (int64_t)value->st_ctim.tv_nsec;
}

static long guest_fstatat(int directory, uint64_t path, uint64_t result, int flags, int by_descriptor)
{
	struct stat value;
	int status;

	if (!result || (!by_descriptor && !path))
		return -EFAULT;
	if (by_descriptor)
		status = fstat(directory, &value);
	else
		status = fstatat(directory, GUEST(const char *, path), &value, flags);
	if (status != 0)
		return -errno;
	kstat_out(&value, GUEST(struct halo_guest_kstat *, result));
	return 0;
}

/* the host's struct sysinfo in the guest's, whose longs are 32-bit: as a
64-bit kernel does for its 32-bit programs, a memory size too large for 32
bits is counted in larger units (mem_unit), and the rest saturate */
static uint32_t saturate(uint64_t value)
{
	return value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
}

static void sysinfo_out(const struct sysinfo *value, struct halo_guest_sysinfo *result)
{
	uint64_t sizes[8];
	uint64_t unit = value->mem_unit ? value->mem_unit : 1;
	int index, large;

	sizes[0] = value->totalram;
	sizes[1] = value->freeram;
	sizes[2] = value->sharedram;
	sizes[3] = value->bufferram;
	sizes[4] = value->totalswap;
	sizes[5] = value->freeswap;
	sizes[6] = value->totalhigh;
	sizes[7] = value->freehigh;
	for (;;)
	{
		large = 0;
		for (index = 0; index < 8; index++)
			large |= sizes[index] > UINT32_MAX;
		if (!large || unit >= 0x80000000u)
			break;
		unit <<= 1;
		for (index = 0; index < 8; index++)
			sizes[index] >>= 1;
	}
	memset(result, 0, sizeof(*result));
	result->uptime = value->uptime < 0 ? 0 : saturate((uint64_t)value->uptime);
	for (index = 0; index < 3; index++)
		result->loads[index] = saturate(value->loads[index]);
	result->totalram = saturate(sizes[0]);
	result->freeram = saturate(sizes[1]);
	result->sharedram = saturate(sizes[2]);
	result->bufferram = saturate(sizes[3]);
	result->totalswap = saturate(sizes[4]);
	result->freeswap = saturate(sizes[5]);
	result->procs = value->procs;
	result->totalhigh = saturate(sizes[6]);
	result->freehigh = saturate(sizes[7]);
	result->mem_unit = (uint32_t)unit;
}

/* ---------- standard output and error */

#ifdef __ANDROID__
struct log_stream
{
	char line[1024];
	size_t length;
};

static struct log_stream log_streams[2];
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

static void log_bytes(int fd, const char *bytes, size_t size)
{
	struct log_stream *stream = &log_streams[fd == 2];
	size_t index;

	pthread_mutex_lock(&log_lock);
	for (index = 0; index < size; index++)
	{
		char c = bytes[index];

		if (c == '\n' || stream->length == sizeof(stream->line) - 1)
		{
			stream->line[stream->length] = 0;
			__android_log_write(fd == 2 ? ANDROID_LOG_WARN : ANDROID_LOG_INFO, "halo", stream->line);
			stream->length = 0;
			if (c == '\n')
				continue;
		}
		stream->line[stream->length++] = c;
	}
	pthread_mutex_unlock(&log_lock);
}
#endif

/* ---------- vectored reads and writes */

/* vectors up to this size are converted on the stack, longer ones (up to
GUEST_IOV_MAX) in an allocation */
#define IOVEC_STACK_COUNT 64

/* the host's iovecs for the guest's; *result is buffer or an allocation
iovec_free releases. Returns 0 or -errno. */
static long iovec_in(uint64_t vector, int count, struct iovec *buffer, struct iovec **result)
{
	const struct halo_guest_iovec *guest_vector = GUEST(const struct halo_guest_iovec *, vector);
	struct iovec *host_vector = buffer;
	int index;

	if (count < 0 || count > GUEST_IOV_MAX)
		return -EINVAL;
	if (count && !vector)
		return -EFAULT;
	if (count > IOVEC_STACK_COUNT)
	{
		host_vector = malloc((size_t)count * sizeof(*host_vector));
		if (!host_vector)
			return -ENOMEM;
	}
	for (index = 0; index < count; index++)
	{
		host_vector[index].iov_base = GUEST(void *, guest_vector[index].base);
		host_vector[index].iov_len = guest_vector[index].length;
	}
	*result = host_vector;
	return 0;
}

static void iovec_free(struct iovec *vector, struct iovec *buffer)
{
	if (vector != buffer)
		free(vector);
}

static long guest_writev(int fd, uint64_t vector, int count, int64_t offset, int positional)
{
	struct iovec buffer[IOVEC_STACK_COUNT];
	struct iovec *host_vector;
	long result = iovec_in(vector, count, buffer, &host_vector);

	if (result)
		return result;
#ifdef __ANDROID__
	if (fd == 1 || fd == 2)
	{
		int index;

		for (index = 0; index < count; index++)
		{
			log_bytes(fd, host_vector[index].iov_base, host_vector[index].iov_len);
			result += (long)host_vector[index].iov_len;
		}
		iovec_free(host_vector, buffer);
		return result;
	}
#endif
	if (positional)
		result = result_of(pwritev(fd, host_vector, count, offset));
	else
		result = result_of(writev(fd, host_vector, count));
	iovec_free(host_vector, buffer);
	return result;
}

static long guest_readv(int fd, uint64_t vector, int count, int64_t offset, int positional)
{
	struct iovec buffer[IOVEC_STACK_COUNT];
	struct iovec *host_vector;
	long result = iovec_in(vector, count, buffer, &host_vector);

	if (result)
		return result;
	if (positional)
		result = result_of(preadv(fd, host_vector, count, offset));
	else
		result = result_of(readv(fd, host_vector, count));
	iovec_free(host_vector, buffer);
	return result;
}

/* ---------- files */

/* a 64-bit offset from the two longs musl passes for it, low half first:
each arrives sign-extended (__scc), so only its lower 32 bits count */
static int64_t offset_of_halves(long long low, long long high)
{
	return (int64_t)((uint64_t)(uint32_t)low | ((uint64_t)(uint32_t)high << 32));
}

/* _llseek(fd, offset_high, offset_low, result, whence): the 64-bit offset
in two halves, and the new position through a pointer, because the guest's
result is a 32-bit long */
static long guest_llseek(int fd, uint32_t offset_high, uint32_t offset_low, uint64_t result, int whence)
{
	int64_t offset = (int64_t)(((uint64_t)offset_high << 32) | offset_low);
	off_t position;

	if (!result)
		return -EFAULT;
	position = lseek(fd, (off_t)offset, whence);
	if (position == (off_t)-1)
		return -errno;
	memcpy(GUEST(void *, result), &(int64_t){(int64_t)position}, sizeof(int64_t));
	return 0;
}

static int lock_command(int command)
{
	switch (command)
	{
	case GUEST_F_GETLK: return F_GETLK;
	case GUEST_F_SETLK: return F_SETLK;
	case GUEST_F_SETLKW: return F_SETLKW;
	case GUEST_F_OFD_GETLK: return F_OFD_GETLK;
	case GUEST_F_OFD_SETLK: return F_OFD_SETLK;
	case GUEST_F_OFD_SETLKW: return F_OFD_SETLKW;
	}
	return -1;
}

static long guest_fcntl(int fd, int command, uint64_t argument)
{
	switch (command)
	{
	case GUEST_F_DUPFD:
		return result_of(fcntl(fd, F_DUPFD, (int)argument));
	case GUEST_F_DUPFD_CLOEXEC:
		return result_of(fcntl(fd, F_DUPFD_CLOEXEC, (int)argument));
	case GUEST_F_GETFD:
		return result_of(fcntl(fd, F_GETFD));
	case GUEST_F_SETFD:
		return result_of(fcntl(fd, F_SETFD, (int)argument));
	case GUEST_F_GETFL:
	{
		long result = result_of(fcntl(fd, F_GETFL));

		return result < 0 ? result : (long)open_flags_out(result);
	}
	case GUEST_F_SETFL:
	{
		long flags = open_flags_in((uint32_t)argument);

		if (flags < 0)
			return flags;
		return result_of(fcntl(fd, F_SETFL, (int)flags));
	}
	case GUEST_F_GETOWN:
		return result_of(fcntl(fd, F_GETOWN));
	case GUEST_F_SETOWN:
		return result_of(fcntl(fd, F_SETOWN, (int)argument));
	case GUEST_F_GETOWN_EX:
	case GUEST_F_SETOWN_EX:
		/* struct f_owner_ex is two ints everywhere */
		return result_of(fcntl(fd, command == GUEST_F_GETOWN_EX ? F_GETOWN_EX : F_SETOWN_EX,
			GUEST(void *, argument)));
	case GUEST_F_GETLK:
	case GUEST_F_SETLK:
	case GUEST_F_SETLKW:
	case GUEST_F_OFD_GETLK:
	case GUEST_F_OFD_SETLK:
	case GUEST_F_OFD_SETLKW:
	{
		struct halo_guest_flock *guest_lock = GUEST(struct halo_guest_flock *, argument);
		struct flock lock;
		long result;

		if (!argument)
			return -EFAULT;
		memset(&lock, 0, sizeof(lock));
		lock.l_type = guest_lock->l_type;
		lock.l_whence = guest_lock->l_whence;
		lock.l_start = guest_lock->l_start;
		lock.l_len = guest_lock->l_len;
		lock.l_pid = guest_lock->l_pid;
		result = result_of(fcntl(fd, lock_command(command), &lock));
		if (result == 0)
		{
			guest_lock->l_type = lock.l_type;
			guest_lock->l_whence = lock.l_whence;
			guest_lock->l_start = lock.l_start;
			guest_lock->l_len = lock.l_len;
			guest_lock->l_pid = lock.l_pid;
		}
		return result;
	}
	default:
		return -EINVAL;
	}
}

/* ---------- time */

#ifndef FUTEX_LOCK_PI2
/* (Linux 5.14; older headers lack it) */
#define FUTEX_LOCK_PI2 13
#endif

static long guest_futex(uint64_t address, int operation, uint32_t value, uint64_t timeout,
	uint64_t address2, uint32_t value3)
{
	struct timespec host_timeout;
	struct timespec *timeout_pointer = NULL;
	int command = operation & FUTEX_CMD_MASK;

	/* for these operations the fourth argument is a timeout; for the
	others it is a count passed as a pointer-sized integer */
	if (command == FUTEX_WAIT || command == FUTEX_WAIT_BITSET || command == FUTEX_LOCK_PI ||
		command == FUTEX_LOCK_PI2 || command == FUTEX_WAIT_REQUEUE_PI)
	{
		if (timespec_in(timeout, &host_timeout))
			timeout_pointer = &host_timeout;
		return result_of(syscall(SYS_futex, GUEST(void *, address), operation, value, timeout_pointer,
			GUEST(void *, address2), value3));
	}
	return result_of(syscall(SYS_futex, GUEST(void *, address), operation, value, (void *)(uintptr_t)timeout,
		GUEST(void *, address2), value3));
}

/* ---------- dispatch */

/* the host's number for a guest call whose arguments the host's kernel
takes as they are (integers, strings, buffers, and the structures and
constants that are the same everywhere), or -1 */
static long passthrough_number(long long number)
{
	switch (number)
	{
	case GUEST_SYS_read: return SYS_read;
	case GUEST_SYS_close: return SYS_close;
	case GUEST_SYS_pread64: return SYS_pread64;
	case GUEST_SYS_pwrite64: return SYS_pwrite64;
	case GUEST_SYS_getdents64: return SYS_getdents64;
	case GUEST_SYS_unlinkat: return SYS_unlinkat;
	case GUEST_SYS_renameat2: return SYS_renameat2;
	case GUEST_SYS_mkdirat: return SYS_mkdirat;
	case GUEST_SYS_fchmod: return SYS_fchmod;
	case GUEST_SYS_fchmodat: return SYS_fchmodat;
	case GUEST_SYS_ftruncate: return SYS_ftruncate;
	case GUEST_SYS_fsync: return SYS_fsync;
	case GUEST_SYS_fdatasync: return SYS_fdatasync;
	case GUEST_SYS_getcwd: return SYS_getcwd;
	case GUEST_SYS_chdir: return SYS_chdir;
	case GUEST_SYS_readlinkat: return SYS_readlinkat;
	case GUEST_SYS_faccessat: return SYS_faccessat;
	case GUEST_SYS_dup: return SYS_dup;
	case GUEST_SYS_getpid: return SYS_getpid;
	case GUEST_SYS_getppid: return SYS_getppid;
	case GUEST_SYS_gettid: return SYS_gettid;
	case GUEST_SYS_getuid: return SYS_getuid;
	case GUEST_SYS_geteuid: return SYS_geteuid;
	case GUEST_SYS_getgid: return SYS_getgid;
	case GUEST_SYS_getegid: return SYS_getegid;
	case GUEST_SYS_sched_yield: return SYS_sched_yield;
	case GUEST_SYS_getrandom: return SYS_getrandom;
	case GUEST_SYS_kill: return SYS_kill;
	case GUEST_SYS_tkill: return SYS_tkill;
	case GUEST_SYS_tgkill: return SYS_tgkill;
	case GUEST_SYS_rt_sigprocmask: return SYS_rt_sigprocmask;
	case GUEST_SYS_uname: return SYS_uname;
	case GUEST_SYS_prlimit64: return SYS_prlimit64;
	case GUEST_SYS_umask: return SYS_umask;
	case GUEST_SYS_flock: return SYS_flock;
	case GUEST_SYS_membarrier: return SYS_membarrier;
	}
	return -1;
}

long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	switch (number)
	{
	case GUEST_SYS_write:
#ifdef __ANDROID__
		if (a == 1 || a == 2)
		{
			log_bytes((int)a, GUEST(const char *, b), (size_t)(uint32_t)c);
			return (uint32_t)c;
		}
#endif
		return result_of(write((int)a, GUEST(const void *, b), (size_t)(uint32_t)c));
	case GUEST_SYS_writev:
		return guest_writev((int)a, (uint64_t)b, (int)c, 0, 0);
	case GUEST_SYS_pwritev:
		return guest_writev((int)a, (uint64_t)b, (int)c, offset_of_halves(d, e), 1);
	case GUEST_SYS_readv:
		return guest_readv((int)a, (uint64_t)b, (int)c, 0, 0);
	case GUEST_SYS_preadv:
		return guest_readv((int)a, (uint64_t)b, (int)c, offset_of_halves(d, e), 1);

	case GUEST_SYS_openat:
	{
		long flags = open_flags_in((uint32_t)c);

		if (flags < 0)
			return flags;
		return result_of(openat((int)a, GUEST(const char *, b), (int)flags, (mode_t)(uint32_t)d));
	}
	case GUEST_SYS_fstat:
		return guest_fstatat((int)a, 0, (uint64_t)b, 0, 1);
	case GUEST_SYS_newfstatat:
		return guest_fstatat((int)a, (uint64_t)b, (uint64_t)c, (int)d, 0);
	case GUEST_SYS__llseek:
		return guest_llseek((int)a, (uint32_t)b, (uint32_t)c, (uint64_t)d, (int)e);
	case GUEST_SYS_fcntl:
		return guest_fcntl((int)a, (int)b, (uint64_t)c);
	case GUEST_SYS_pipe2:
	{
		long flags = open_flags_in((uint32_t)b);

		if (flags < 0)
			return flags;
		return result_of(pipe2(GUEST(int *, a), (int)flags));
	}
	case GUEST_SYS_dup3:
	{
		long flags = open_flags_in((uint32_t)c);

		if (flags < 0)
			return flags;
		return result_of(dup3((int)a, (int)b, (int)flags));
	}
	case GUEST_SYS_renameat:
#ifdef SYS_renameat
		return result_of(syscall(SYS_renameat, (int)a, GUEST(const char *, b), (int)c, GUEST(const char *, d)));
#else
		/* (a host architecture without renameat: renameat2 without flags
		is the same call) */
		return result_of(syscall(SYS_renameat2, (int)a, GUEST(const char *, b), (int)c,
			GUEST(const char *, d), 0));
#endif
	case GUEST_SYS_getrlimit:
		/* the guest's struct rlimit is two 64-bit values, prlimit64's layout */
		return result_of(syscall(SYS_prlimit64, 0, (int)a, NULL, GUEST(void *, b)));

	case GUEST_SYS_clock_gettime:
	case GUEST_SYS_clock_getres:
	{
		struct timespec value;
		int result = number == GUEST_SYS_clock_gettime ?
			clock_gettime((clockid_t)a, &value) : clock_getres((clockid_t)a, &value);

		if (result == 0)
			timespec_out((uint64_t)b, &value);
		return result_of(result);
	}
	case GUEST_SYS_gettimeofday:
	{
		struct timespec value;
		struct halo_guest_timespec *result = GUEST(struct halo_guest_timespec *, a);

		clock_gettime(CLOCK_REALTIME, &value);
		if (result)
		{
			result->seconds = (int32_t)value.tv_sec;
			result->nanoseconds = (int32_t)(value.tv_nsec / 1000);
		}
		return 0;
	}
	case GUEST_SYS_nanosleep:
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
	case GUEST_SYS_clock_nanosleep:
	{
		struct timespec request, remaining;
		int result;

		if (!timespec_in((uint64_t)c, &request))
			return -EFAULT;
		result = clock_nanosleep((clockid_t)a, (int)b, &request, &remaining);
		/* (an absolute sleep has nothing remaining, and leaves it alone) */
		if (result == EINTR && !((int)b & TIMER_ABSTIME))
			timespec_out((uint64_t)d, &remaining);
		return -result;
	}
	case GUEST_SYS_futex:
		return guest_futex((uint64_t)a, (int)b, (uint32_t)c, (uint64_t)d, (uint64_t)e, (uint32_t)f);
	case GUEST_SYS_ppoll:
	{
		struct timespec timeout;
		int has_timeout = timespec_in((uint64_t)c, &timeout);

		return result_of(ppoll(GUEST(struct pollfd *, a), (nfds_t)(uint32_t)b, has_timeout ? &timeout : NULL, NULL));
	}
	case GUEST_SYS_utimensat:
	{
		struct timespec times[2];
		const struct halo_guest_timespec *guest_times = GUEST(const struct halo_guest_timespec *, c);

		if (guest_times)
		{
			timespec_in((uint64_t)c, &times[0]);
			timespec_in((uint64_t)c + sizeof(struct halo_guest_timespec), &times[1]);
		}
		return result_of(utimensat((int)a, GUEST(const char *, b), guest_times ? times : NULL, (int)d));
	}

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
		/* musl then falls back to mmap and copying */
		return -ENOMEM;

	case GUEST_SYS_exit:
	case GUEST_SYS_exit_group:
		host_exit((int)a);

	case GUEST_SYS_sysinfo:
	{
		struct sysinfo value;

		if (!a)
			return -EFAULT;
		if (sysinfo(&value) != 0)
			return -errno;
		sysinfo_out(&value, GUEST(struct halo_guest_sysinfo *, a));
		return 0;
	}

	case GUEST_SYS_set_tid_address:
		return gettid();
	case GUEST_SYS_rt_sigaction:
	case GUEST_SYS_sigaltstack:
		/* the host owns signal handling */
		return 0;
	case GUEST_SYS_ioctl:
		return -ENOTTY;
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
		return -ENOSYS;

	default:
	{
		long host_number = passthrough_number(number);

		if (host_number >= 0)
			return result_of(syscall(host_number, a, b, c, d, e, f));
		host_logf(HOST_LOG_WARN, "guest system call %lld is not supported", number);
		return -ENOSYS;
	}
	}
}
