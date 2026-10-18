/*
HOST_SYSCALL.C

The guest's system calls (its musl sends each one to host_syscall). The
guest is Linux code: the numbers are Linux's (the guest's own
bits/syscall.h.in), and so are its flags, structures and errno values. Each
call is carried out with the Darwin call that does the same, converting
where the two differ:

- open, fcntl, AT_* and mmap flags, clocks, UTIME_NOW and UTIME_OMIT, and
  errno values (EAGAIN is 11 on Linux, 35 on Darwin);
- struct stat (the guest's kstat, the AArch64 kernel's layout), directory
  entries (linux_dirent64), timespec and iovec (32-bit in the guest);
- futexes, which Darwin has not: os_sync_wait_on_address and its wakes;
- memory: the guest's mappings are pages of the pools (host_guest_allocate),
  already in the VM.

Guest addresses become host ones with host_guest_pointer, and each buffer
the guest passes is checked first (outside): one that reaches outside the
guest's memory fails with EFAULT, as on Linux, instead of reaching the
host's own memory past the guest's 4 GB.
*/

#include "host.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <os/clock.h>
#include <os/os_sync_wait_on_address.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

/* the guest's Linux system call numbers (__NR_*) */
#include "bits/syscall.h.in"

/* ---------- Linux's values */

#define LINUX_AT_FDCWD (-100)
#define LINUX_AT_SYMLINK_NOFOLLOW 0x100
#define LINUX_AT_REMOVEDIR 0x200
#define LINUX_AT_EACCESS 0x200
#define LINUX_AT_EMPTY_PATH 0x1000

/* (the guest's bits/fcntl.h) */
#define LINUX_O_ACCMODE 03
#define LINUX_O_CREAT 0100
#define LINUX_O_EXCL 0200
#define LINUX_O_NOCTTY 0400
#define LINUX_O_TRUNC 01000
#define LINUX_O_APPEND 02000
#define LINUX_O_NONBLOCK 04000
#define LINUX_O_DSYNC 010000
#define LINUX_O_SYNC 04010000
#define LINUX_O_DIRECTORY 040000
#define LINUX_O_NOFOLLOW 0100000
#define LINUX_O_CLOEXEC 02000000

#define LINUX_F_DUPFD_CLOEXEC 1030

#define LINUX_MAP_FIXED 0x10
#define LINUX_MAP_ANONYMOUS 0x20
#define LINUX_MAP_FIXED_NOREPLACE 0x100000
#define LINUX_MADV_DONTNEED 4

#define LINUX_CLOCK_REALTIME 0
#define LINUX_CLOCK_MONOTONIC 1
#define LINUX_CLOCK_PROCESS_CPUTIME_ID 2
#define LINUX_CLOCK_THREAD_CPUTIME_ID 3
#define LINUX_CLOCK_MONOTONIC_RAW 4
#define LINUX_CLOCK_REALTIME_COARSE 5
#define LINUX_CLOCK_MONOTONIC_COARSE 6
#define LINUX_CLOCK_BOOTTIME 7
#define LINUX_TIMER_ABSTIME 1

#define LINUX_UTIME_NOW ((1L << 30) - 1)
#define LINUX_UTIME_OMIT ((1L << 30) - 2)

#define LINUX_FUTEX_WAIT 0
#define LINUX_FUTEX_WAKE 1
#define LINUX_FUTEX_REQUEUE 3
#define LINUX_FUTEX_CMP_REQUEUE 4
#define LINUX_FUTEX_WAIT_BITSET 9
#define LINUX_FUTEX_WAKE_BITSET 10
#define LINUX_FUTEX_CMD_MASK 0x7f

#define LINUX_TIOCGWINSZ 0x5413

#define LINUX(error) (-(long long)(error))

/* ---------- errno */

int host_linux_errno(int error)
{
	switch (error)
	{
	case 0: return 0;
	/* 1 to 34 are the same on both, but for EAGAIN (below) and EDEADLK */
	case EDEADLK: return 35;
	case EAGAIN: return 11;
	case ENAMETOOLONG: return 36;
	case ENOLCK: return 37;
	case ENOSYS: return 38;
	case ENOTEMPTY: return 39;
	case ELOOP: return 40;
	case ENOMSG: return 42;
	case EIDRM: return 43;
	case ENOSTR: return 60;
	case ENODATA: return 61;
	case ETIME: return 62;
	case ENOSR: return 63;
	case EREMOTE: return 66;
	case ENOLINK: return 67;
	case EPROTO: return 71;
	case EMULTIHOP: return 72;
	case EBADMSG: return 74;
	case EOVERFLOW: return 75;
	case EILSEQ: return 84;
	case EUSERS: return 87;
	case ENOTSOCK: return 88;
	case EDESTADDRREQ: return 89;
	case EMSGSIZE: return 90;
	case EPROTOTYPE: return 91;
	case ENOPROTOOPT: return 92;
	case EPROTONOSUPPORT: return 93;
	case ESOCKTNOSUPPORT: return 94;
	case EOPNOTSUPP: return 95;
	case ENOTSUP: return 95;
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
	case EHOSTDOWN: return 112;
	case EHOSTUNREACH: return 113;
	case EALREADY: return 114;
	case EINPROGRESS: return 115;
	case ESTALE: return 116;
	case EDQUOT: return 122;
	case ECANCELED: return 125;
	case EOWNERDEAD: return 130;
	case ENOTRECOVERABLE: return 131;
	default:
		return error <= 34 ? error : 5; /* EIO */
	}
}

static long long result_of(long long value)
{
	return value == -1 ? LINUX(host_linux_errno(errno)) : value;
}

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

/* (the guest's arch/arm64_32/kstat.h) */
struct guest_kstat
{
	uint64_t st_dev;
	uint64_t st_ino;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
	uint64_t st_rdev;
	uint64_t pad;
	int64_t st_size;
	int32_t st_blksize;
	int32_t pad2;
	int64_t st_blocks;
	int64_t st_atime_sec;
	int64_t st_atime_nsec;
	int64_t st_mtime_sec;
	int64_t st_mtime_nsec;
	int64_t st_ctime_sec;
	int64_t st_ctime_nsec;
	uint32_t unused[2];
};

struct guest_dirent64
{
	uint64_t d_ino;
	int64_t d_off;
	uint16_t d_reclen;
	uint8_t d_type;
	char d_name[];
};

struct guest_rlimit64
{
	uint64_t current;
	uint64_t maximum;
};

#define GUEST(type, value) ((type)host_guest_pointer((uint32_t)(value)))

/* whether all `size` bytes at guest `address` are mapped guest memory */
static int in_guest_memory(uint64_t address, uint64_t size)
{
	return address < 0x100000000ULL && size <= 0x100000000ULL - address &&
		opence_guest_host_pointer(host_guest, (uint32_t)address, size) != NULL;
}

/* whether a buffer the guest passes, `size` bytes at `address`, reaches
outside its memory (an empty one never does). A bug in the guest's code
that a map file or a packet sets off must not become a write to the
host's memory. (Paths, which are only read, up to their NUL, are not
checked) */
static int outside(uint64_t address, uint64_t size)
{
	return size && !in_guest_memory(address, size);
}

/* 1 with the time in *result, 0 for none (NULL), -1 if it is outside the
guest's memory */
static int timespec_in(uint64_t address, struct timespec *result)
{
	const struct guest_timespec *value = GUEST(const struct guest_timespec *, address);

	if (!value)
		return 0;
	if (outside(address, sizeof(*value)))
		return -1;
	result->tv_sec = value->seconds;
	result->tv_nsec = value->nanoseconds;
	return 1;
}

static void timespec_out(uint64_t address, const struct timespec *value)
{
	struct guest_timespec *result = GUEST(struct guest_timespec *, address);

	if (!result || outside(address, sizeof(*result)))
		return;
	result->seconds = (int32_t)value->tv_sec;
	result->nanoseconds = (int32_t)value->tv_nsec;
}

static void kstat_out(uint64_t address, const struct stat *information)
{
	struct guest_kstat *result = GUEST(struct guest_kstat *, address);

	memset(result, 0, sizeof(*result));
	result->st_dev = (uint64_t)information->st_dev;
	result->st_ino = information->st_ino;
	result->st_mode = information->st_mode;
	result->st_nlink = information->st_nlink;
	result->st_uid = information->st_uid;
	result->st_gid = information->st_gid;
	result->st_rdev = (uint64_t)information->st_rdev;
	result->st_size = information->st_size;
	result->st_blksize = information->st_blksize;
	result->st_blocks = information->st_blocks;
	result->st_atime_sec = information->st_atimespec.tv_sec;
	result->st_atime_nsec = information->st_atimespec.tv_nsec;
	result->st_mtime_sec = information->st_mtimespec.tv_sec;
	result->st_mtime_nsec = information->st_mtimespec.tv_nsec;
	result->st_ctime_sec = information->st_ctimespec.tv_sec;
	result->st_ctime_nsec = information->st_ctimespec.tv_nsec;
}

/* ---------- flags */

static int directory_fd(long long fd)
{
	return (int)fd == LINUX_AT_FDCWD ? AT_FDCWD : (int)fd;
}

static int at_flags(long long flags)
{
	int result = 0;

	if (flags & LINUX_AT_SYMLINK_NOFOLLOW)
		result |= AT_SYMLINK_NOFOLLOW;
	return result;
}

static int open_flags_in(long long flags)
{
	int result = (int)(flags & LINUX_O_ACCMODE);

	if (flags & LINUX_O_CREAT)
		result |= O_CREAT;
	if (flags & LINUX_O_EXCL)
		result |= O_EXCL;
	if (flags & LINUX_O_NOCTTY)
		result |= O_NOCTTY;
	if (flags & LINUX_O_TRUNC)
		result |= O_TRUNC;
	if (flags & LINUX_O_APPEND)
		result |= O_APPEND;
	if (flags & LINUX_O_NONBLOCK)
		result |= O_NONBLOCK;
	if ((flags & LINUX_O_SYNC) == LINUX_O_SYNC)
		result |= O_SYNC;
	else if (flags & LINUX_O_DSYNC)
		result |= O_DSYNC;
	if (flags & LINUX_O_DIRECTORY)
		result |= O_DIRECTORY;
	if (flags & LINUX_O_NOFOLLOW)
		result |= O_NOFOLLOW;
	if (flags & LINUX_O_CLOEXEC)
		result |= O_CLOEXEC;
	return result;
}

static long long open_flags_out(int flags)
{
	long long result = flags & O_ACCMODE;

	if (flags & O_APPEND)
		result |= LINUX_O_APPEND;
	if (flags & O_NONBLOCK)
		result |= LINUX_O_NONBLOCK;
	if (flags & O_SYNC)
		result |= LINUX_O_SYNC;
	else if (flags & O_DSYNC)
		result |= LINUX_O_DSYNC;
	return result;
}

static int clock_in(long long clock)
{
	switch ((int)clock)
	{
	case LINUX_CLOCK_REALTIME:
	case LINUX_CLOCK_REALTIME_COARSE:
		return CLOCK_REALTIME;
	case LINUX_CLOCK_MONOTONIC:
	case LINUX_CLOCK_MONOTONIC_COARSE:
	case LINUX_CLOCK_BOOTTIME:
		return CLOCK_MONOTONIC;
	case LINUX_CLOCK_MONOTONIC_RAW:
		return CLOCK_MONOTONIC_RAW;
	case LINUX_CLOCK_PROCESS_CPUTIME_ID:
		return CLOCK_PROCESS_CPUTIME_ID;
	case LINUX_CLOCK_THREAD_CPUTIME_ID:
		return CLOCK_THREAD_CPUTIME_ID;
	default:
		return -1;
	}
}

/* ---------- threads' ids: the main thread's is the process's, as on Linux */

static atomic_int next_thread_id;
static __thread int thread_id;

static int current_thread_id(void)
{
	if (!thread_id)
	{
		int expected = 0;

		/* (the first thread to ask is the main thread) */
		if (atomic_compare_exchange_strong(&next_thread_id, &expected, (int)getpid() + 1))
			thread_id = (int)getpid();
		else
			thread_id = atomic_fetch_add(&next_thread_id, 1);
	}
	return thread_id;
}

/* ---------- reading and writing */

static long long vector_io(int fd, uint64_t vector, int count, int64_t offset, int positional, int writing)
{
	struct iovec host_vector[64];
	const struct guest_iovec *guest_vector = GUEST(const struct guest_iovec *, vector);
	int index;

	if (count < 0 || count > 64)
		return LINUX(22);
	if (outside(vector, (uint64_t)count * sizeof(*guest_vector)))
		return LINUX(14);
	for (index = 0; index < count; index++)
	{
		if (outside(guest_vector[index].base, guest_vector[index].length))
			return LINUX(14);
		host_vector[index].iov_base = host_guest_pointer(guest_vector[index].base);
		host_vector[index].iov_len = guest_vector[index].length;
	}
	if (writing)
		return result_of(positional ? pwritev(fd, host_vector, count, offset) : writev(fd, host_vector, count));
	return result_of(positional ? preadv(fd, host_vector, count, offset) : readv(fd, host_vector, count));
}

/* ---------- directories: each fd read by getdents64 gets a DIR of its
own, made from a duplicate of it */

#define DIRECTORY_STREAMS 1024
static DIR *directory_streams[DIRECTORY_STREAMS];
static pthread_mutex_t directory_lock = PTHREAD_MUTEX_INITIALIZER;

static void directory_forget(int fd)
{
	if (fd < 0 || fd >= DIRECTORY_STREAMS)
		return;
	pthread_mutex_lock(&directory_lock);
	if (directory_streams[fd])
	{
		closedir(directory_streams[fd]);
		directory_streams[fd] = NULL;
	}
	pthread_mutex_unlock(&directory_lock);
}

static long long getdents64(int fd, uint64_t buffer_address, uint32_t size)
{
	uint8_t *buffer = host_guest_pointer((uint32_t)buffer_address);
	uint32_t used = 0;
	DIR *stream;
	long long result;

	if (fd < 0 || fd >= DIRECTORY_STREAMS)
		return LINUX(9);
	if (outside(buffer_address, size))
		return LINUX(14);
	pthread_mutex_lock(&directory_lock);
	stream = directory_streams[fd];
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
			return LINUX(host_linux_errno(error));
		}
		directory_streams[fd] = stream;
	}
	for (;;)
	{
		long position = telldir(stream);
		struct dirent *entry = readdir(stream);
		struct guest_dirent64 *record;
		size_t name_length, length;

		if (!entry)
			break;
		name_length = strlen(entry->d_name);
		length = (offsetof(struct guest_dirent64, d_name) + name_length + 1 + 7) & ~(size_t)7;
		if (used + length > size)
		{
			seekdir(stream, position);
			break;
		}
		record = (struct guest_dirent64 *)(buffer + used);
		record->d_ino = entry->d_ino;
		record->d_off = telldir(stream);
		record->d_reclen = (uint16_t)length;
		record->d_type = entry->d_type;
		memcpy(record->d_name, entry->d_name, name_length + 1);
		used += (uint32_t)length;
	}
	result = used ? used : (size && readdir(stream) == NULL ? 0 : LINUX(22));
	pthread_mutex_unlock(&directory_lock);
	return result;
}

/* ---------- memory: pages of the guest's pools, already in the VM */

static long long guest_mmap(uint64_t address, uint64_t size, int protection, long long flags, int fd, int64_t offset)
{
	uint64_t length = (size + 0xfff) & ~0xfffULL;
	uint8_t *memory;

	(void)protection;
	if (!length)
		return LINUX(22);
	if (flags & (LINUX_MAP_FIXED | LINUX_MAP_FIXED_NOREPLACE))
	{
		/* inside the guest's memory (the Xbox window, or a range the pools
		handed out): a mapping there replaces what was there */
		if (!in_guest_memory(address, length))
			return LINUX(12);
	}
	else
	{
		address = host_guest_allocate(length);
		if (!address)
			return LINUX(12);
	}
	memory = host_guest_pointer((uint32_t)address);
	memset(memory, 0, length);
	if (!(flags & LINUX_MAP_ANONYMOUS) && fd >= 0)
	{
		/* a file's contents, as a private copy */
		if (pread(fd, memory, length, offset) < 0)
		{
			int error = errno;

			if (!(flags & (LINUX_MAP_FIXED | LINUX_MAP_FIXED_NOREPLACE)))
				host_guest_free((uint32_t)address, length);
			return LINUX(host_linux_errno(error));
		}
	}
	return (long long)address;
}

static long long guest_munmap(uint64_t address, uint64_t size)
{
	uint64_t length = (size + 0xfff) & ~0xfffULL;

	if (!length || !in_guest_memory(address, length))
		return LINUX(22);
	/* the memory is zeroed, as a fresh mapping would be, and pool pages
	go back to the pools */
	host_guest_free((uint32_t)address, length);
	return 0;
}

/* ---------- futexes */

static long long guest_futex(uint64_t address, long long operation, uint32_t value, uint64_t timeout,
	uint64_t address2, uint32_t value3)
{
	uint32_t *word = GUEST(uint32_t *, address);
	int command = (int)(operation & LINUX_FUTEX_CMD_MASK);
	struct timespec wait;

	(void)address2;
	if (!word || outside(address, sizeof(*word)))
		return LINUX(14);
	switch (command)
	{
	case LINUX_FUTEX_WAIT:
	case LINUX_FUTEX_WAIT_BITSET:
	{
		int result, timed;

		if (__atomic_load_n(word, __ATOMIC_SEQ_CST) != value)
			return LINUX(11);
		timed = timespec_in(timeout, &wait);
		if (timed < 0)
			return LINUX(14);
		if (timed)
		{
			uint64_t nanoseconds;

			/* WAIT's timeout is relative, WAIT_BITSET's an absolute time
			on the realtime (FUTEX_CLOCK_REALTIME) or monotonic clock */
			if (command == LINUX_FUTEX_WAIT_BITSET)
			{
				struct timespec now;

				clock_gettime((operation & 256) ? CLOCK_REALTIME : CLOCK_MONOTONIC, &now);
				wait.tv_sec -= now.tv_sec;
				wait.tv_nsec -= now.tv_nsec;
				if (wait.tv_nsec < 0)
				{
					wait.tv_nsec += 1000000000;
					wait.tv_sec--;
				}
				if (wait.tv_sec < 0)
					return LINUX(110);
			}
			nanoseconds = (uint64_t)wait.tv_sec * 1000000000ULL + (uint64_t)wait.tv_nsec;
			if (!nanoseconds)
				return LINUX(110);
			result = os_sync_wait_on_address_with_timeout(word, value, sizeof(*word), OS_SYNC_WAIT_ON_ADDRESS_NONE,
				OS_CLOCK_MACH_ABSOLUTE_TIME, nanoseconds);
		}
		else
			result = os_sync_wait_on_address(word, value, sizeof(*word), OS_SYNC_WAIT_ON_ADDRESS_NONE);
		if (result < 0)
			return errno == ETIMEDOUT ? LINUX(110) : errno == EINTR ? LINUX(4) : 0;
		return 0;
	}
	case LINUX_FUTEX_WAKE:
	case LINUX_FUTEX_WAKE_BITSET:
		if (value > 1)
			return os_sync_wake_by_address_all(word, sizeof(*word), OS_SYNC_WAKE_BY_ADDRESS_NONE) == 0 ? (long long)value : 0;
		return os_sync_wake_by_address_any(word, sizeof(*word), OS_SYNC_WAKE_BY_ADDRESS_NONE) == 0 ? 1 : 0;
	case LINUX_FUTEX_CMP_REQUEUE:
		if (__atomic_load_n(word, __ATOMIC_SEQ_CST) != value3)
			return LINUX(11);
		/* fall through */
	case LINUX_FUTEX_REQUEUE:
		/* (the waiters are woken rather than moved to the other word: a
		futex's users take a wake as possibly spurious, and check again) */
		os_sync_wake_by_address_all(word, sizeof(*word), OS_SYNC_WAKE_BY_ADDRESS_NONE);
		return 1;
	default:
		return LINUX(38);
	}
}

/* ---------- time */

static long long guest_clock_nanosleep(long long clock, long long flags, uint64_t request_address, uint64_t remaining_address)
{
	struct timespec request, remaining;
	int host_clock = clock_in(clock);

	if (host_clock < 0)
		return LINUX(22);
	if (timespec_in(request_address, &request) <= 0)
		return LINUX(14);
	if (flags & LINUX_TIMER_ABSTIME)
	{
		struct timespec now;

		clock_gettime(host_clock, &now);
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
	if (nanosleep(&request, &remaining) == -1)
	{
		if (errno == EINTR && !(flags & LINUX_TIMER_ABSTIME))
			timespec_out(remaining_address, &remaining);
		return LINUX(host_linux_errno(errno));
	}
	return 0;
}

static long long utime_value(long nanoseconds)
{
	return nanoseconds == LINUX_UTIME_NOW ? UTIME_NOW : nanoseconds == LINUX_UTIME_OMIT ? UTIME_OMIT : nanoseconds;
}

/* ---------- dispatch */

long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	switch (number)
	{
	case __NR_read:
		if (outside((uint64_t)b, (uint32_t)c))
			return LINUX(14);
		return result_of(read((int)a, GUEST(void *, b), (size_t)(uint32_t)c));
	case __NR_write:
		if (outside((uint64_t)b, (uint32_t)c))
			return LINUX(14);
		return result_of(write((int)a, GUEST(const void *, b), (size_t)(uint32_t)c));
	case __NR_pread64:
		if (outside((uint64_t)b, (uint32_t)c))
			return LINUX(14);
		return result_of(pread((int)a, GUEST(void *, b), (size_t)(uint32_t)c, d));
	case __NR_pwrite64:
		if (outside((uint64_t)b, (uint32_t)c))
			return LINUX(14);
		return result_of(pwrite((int)a, GUEST(const void *, b), (size_t)(uint32_t)c, d));
	case __NR_readv:
		return vector_io((int)a, (uint64_t)b, (int)c, 0, 0, 0);
	case __NR_writev:
		return vector_io((int)a, (uint64_t)b, (int)c, 0, 0, 1);
	case __NR_preadv:
		return vector_io((int)a, (uint64_t)b, (int)c, d, 1, 0);
	case __NR_pwritev:
		return vector_io((int)a, (uint64_t)b, (int)c, d, 1, 1);
	case __NR_openat:
		return result_of(openat(directory_fd(a), GUEST(const char *, b), open_flags_in(c), (mode_t)d));
	case __NR_close:
		directory_forget((int)a);
		return result_of(close((int)a));
	case __NR_lseek:
		if ((int)c == SEEK_SET && b == 0 && a >= 0 && a < DIRECTORY_STREAMS && directory_streams[a])
			directory_forget((int)a);
		return result_of(lseek((int)a, b, (int)c));
	case __NR_getdents64:
		return getdents64((int)a, (uint64_t)b, (uint32_t)c);
	case __NR_fcntl:
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

			return flags == -1 ? result_of(-1) : open_flags_out(flags);
		}
		case F_SETFL:
			return result_of(fcntl((int)a, F_SETFL, open_flags_in(c)));
		default:
			return LINUX(22);
		}
	case __NR_ioctl:
		return LINUX(25); /* ENOTTY: no terminals */
	case __NR_fstat:
	{
		struct stat information;

		if (outside((uint64_t)b, sizeof(struct guest_kstat)))
			return LINUX(14);
		if (fstat((int)a, &information) == -1)
			return result_of(-1);
		kstat_out((uint64_t)b, &information);
		return 0;
	}
	case __NR_newfstatat:
	{
		struct stat information;
		const char *path = GUEST(const char *, b);
		int result;

		if (outside((uint64_t)c, sizeof(struct guest_kstat)))
			return LINUX(14);
		if ((d & LINUX_AT_EMPTY_PATH) && (!path || !*path))
			result = fstat(directory_fd(a), &information);
		else
			result = fstatat(directory_fd(a), path, &information, at_flags(d));
		if (result == -1)
			return result_of(-1);
		kstat_out((uint64_t)c, &information);
		return 0;
	}
	case __NR_unlinkat:
		return result_of(unlinkat(directory_fd(a), GUEST(const char *, b), (c & LINUX_AT_REMOVEDIR) ? AT_REMOVEDIR : 0));
	case __NR_renameat:
		return result_of(renameat(directory_fd(a), GUEST(const char *, b), directory_fd(c), GUEST(const char *, d)));
	case __NR_renameat2:
		if (e)
			return LINUX(22);
		return result_of(renameat(directory_fd(a), GUEST(const char *, b), directory_fd(c), GUEST(const char *, d)));
	case __NR_mkdirat:
		return result_of(mkdirat(directory_fd(a), GUEST(const char *, b), (mode_t)c));
	case __NR_fchmod:
		return result_of(fchmod((int)a, (mode_t)b));
	case __NR_fchmodat:
		return result_of(fchmodat(directory_fd(a), GUEST(const char *, b), (mode_t)c, at_flags(d)));
	case __NR_ftruncate:
		return result_of(ftruncate((int)a, b));
	case __NR_fsync:
	case __NR_fdatasync:
		return result_of(fsync((int)a));
	case __NR_getcwd:
	{
		char *buffer = GUEST(char *, a);

		if (!buffer || outside((uint64_t)a, (uint32_t)b))
			return LINUX(14);
		if (!getcwd(buffer, (size_t)(uint32_t)b))
			return result_of(-1);
		return (long long)strlen(buffer) + 1;
	}
	case __NR_chdir:
		return result_of(chdir(GUEST(const char *, a)));
	case __NR_readlinkat:
		if (outside((uint64_t)c, (uint32_t)d))
			return LINUX(14);
		return result_of(readlinkat(directory_fd(a), GUEST(const char *, b), GUEST(char *, c), (size_t)(uint32_t)d));
	case __NR_faccessat:
		return result_of(faccessat(directory_fd(a), GUEST(const char *, b), (int)c, (d & LINUX_AT_EACCESS) ? AT_EACCESS : 0));
	case __NR_dup:
		return result_of(dup((int)a));
	case __NR_dup3:
	{
		int result = dup2((int)a, (int)b);

		if (result >= 0 && (c & LINUX_O_CLOEXEC))
			fcntl(result, F_SETFD, FD_CLOEXEC);
		return result_of(result);
	}
	case __NR_pipe2:
	{
		int *fds = GUEST(int *, a);

		if (!fds || outside((uint64_t)a, 2 * sizeof(*fds)))
			return LINUX(14);
		if (pipe(fds) == -1)
			return result_of(-1);
		if (b & LINUX_O_CLOEXEC)
		{
			fcntl(fds[0], F_SETFD, FD_CLOEXEC);
			fcntl(fds[1], F_SETFD, FD_CLOEXEC);
		}
		if (b & LINUX_O_NONBLOCK)
		{
			fcntl(fds[0], F_SETFL, O_NONBLOCK);
			fcntl(fds[1], F_SETFL, O_NONBLOCK);
		}
		return 0;
	}
	case __NR_ppoll:
	{
		struct timespec timeout;
		int milliseconds = -1, timed = timespec_in((uint64_t)c, &timeout);

		if (timed < 0 || outside((uint64_t)a, (uint64_t)(uint32_t)b * sizeof(struct pollfd)))
			return LINUX(14);
		if (timed)
			milliseconds = (int)(timeout.tv_sec * 1000 + timeout.tv_nsec / 1000000);
		/* (struct pollfd and the POLL* bits are the same on both) */
		return result_of(poll(GUEST(struct pollfd *, a), (nfds_t)(uint32_t)b, milliseconds));
	}
	case __NR_utimensat:
	{
		struct timespec times[2];
		const struct guest_timespec *guest_times = GUEST(const struct guest_timespec *, c);
		int index;

		if (guest_times && outside((uint64_t)c, 2 * sizeof(*guest_times)))
			return LINUX(14);
		for (index = 0; guest_times && index < 2; index++)
		{
			times[index].tv_sec = guest_times[index].seconds;
			times[index].tv_nsec = utime_value(guest_times[index].nanoseconds);
		}
		return result_of(utimensat(directory_fd(a), GUEST(const char *, b), guest_times ? times : NULL, at_flags(d)));
	}

	case __NR_clock_gettime:
	case __NR_clock_getres:
	{
		struct timespec value;
		int clock = clock_in(a);

		if (clock < 0)
			return LINUX(22);
		if ((number == __NR_clock_gettime ? clock_gettime(clock, &value) : clock_getres(clock, &value)) == -1)
			return result_of(-1);
		timespec_out((uint64_t)b, &value);
		return 0;
	}
	case __NR_gettimeofday:
	{
		struct timespec value;
		struct guest_timespec *result = GUEST(struct guest_timespec *, a);

		clock_gettime(CLOCK_REALTIME, &value);
		if (result && outside((uint64_t)a, sizeof(*result)))
			return LINUX(14);
		if (result)
		{
			result->seconds = (int32_t)value.tv_sec;
			result->nanoseconds = (int32_t)(value.tv_nsec / 1000);
		}
		return 0;
	}
	case __NR_nanosleep:
		return guest_clock_nanosleep(LINUX_CLOCK_MONOTONIC, 0, (uint64_t)a, (uint64_t)b);
	case __NR_clock_nanosleep:
		return guest_clock_nanosleep(a, b, (uint64_t)c, (uint64_t)d);
	case __NR_futex:
		return guest_futex((uint64_t)a, b, (uint32_t)c, (uint64_t)d, (uint64_t)e, (uint32_t)f);

	case __NR_mmap:
		return guest_mmap((uint64_t)a, (uint64_t)b, (int)c, d, (int)e, f);
	case __NR_munmap:
		return guest_munmap((uint64_t)a, (uint64_t)b);
	case __NR_mprotect:
		/* (the VM's memory is all readable and writable) */
		return 0;
	case __NR_madvise:
		/* Linux's MADV_DONTNEED leaves anonymous memory zero */
		if ((int)c == LINUX_MADV_DONTNEED && in_guest_memory((uint64_t)a, (uint64_t)b))
			memset(host_guest_pointer((uint32_t)a), 0, (size_t)b);
		return 0;
	case __NR_mremap:
	case __NR_brk:
		/* musl then maps anew and copies */
		return LINUX(12);

	case __NR_exit:
	case __NR_exit_group:
		host_exit((int)a);

	case __NR_set_tid_address:
	case __NR_gettid:
		return current_thread_id();
	case __NR_getpid:
		return getpid();
	case __NR_getppid:
		return getppid();
	case __NR_getuid:
		return getuid();
	case __NR_geteuid:
		return geteuid();
	case __NR_getgid:
		return getgid();
	case __NR_getegid:
		return getegid();
	case __NR_sched_yield:
		sched_yield();
		return 0;
	case __NR_getrandom:
		if (outside((uint64_t)a, (uint32_t)b))
			return LINUX(14);
		arc4random_buf(GUEST(void *, a), (size_t)(uint32_t)b);
		return (uint32_t)b;
	case __NR_kill:
	case __NR_tkill:
	case __NR_tgkill:
	{
		int signal_number = (int)(number == __NR_tgkill ? c : b);

		if (!signal_number)
			return 0;
		/* (abort() and raise() of SIGABRT, SIGSEGV and the like) */
		host_fatal("the guest raised signal %d", signal_number);
	}
	case __NR_rt_sigprocmask:
		if (c && d > 0 && outside((uint64_t)c, (uint32_t)d))
			return LINUX(14);
		if (c && d > 0)
			memset(GUEST(void *, c), 0, (size_t)(uint32_t)d);
		return 0;
	case __NR_rt_sigaction:
	case __NR_sigaltstack:
		/* the host owns signal handling */
		return 0;
	case __NR_uname:
	{
		struct utsname information;
		char *result = GUEST(char *, a);
		const char *fields[6];
		int index;

		if (!result || outside((uint64_t)a, 6 * 65))
			return LINUX(14);
		if (uname(&information) == -1)
			return result_of(-1);
		fields[0] = information.sysname;
		fields[1] = information.nodename;
		fields[2] = information.release;
		fields[3] = information.version;
		fields[4] = information.machine;
		fields[5] = "";
		/* (Linux's struct utsname: six fields of 65 bytes) */
		for (index = 0; index < 6; index++)
			snprintf(result + 65 * index, 65, "%s", fields[index]);
		return 0;
	}
	case __NR_prlimit64:
	{
		struct guest_rlimit64 *old_limit = GUEST(struct guest_rlimit64 *, d);

		if (old_limit && outside((uint64_t)d, sizeof(*old_limit)))
			return LINUX(14);
		if (old_limit)
		{
			old_limit->current = ~0ULL;
			old_limit->maximum = ~0ULL;
		}
		return 0;
	}
	case __NR_umask:
		return umask((mode_t)a);
	case __NR_flock:
		return result_of(flock((int)a, (int)b));

	case __NR_statx:
	case __NR_sysinfo:
	case __NR_statfs:
	case __NR_fstatfs:
	case __NR_membarrier:
	case __NR_clone:
	case __NR_execve:
		return LINUX(38);

	default:
		host_logf(HOST_LOG_WARN, "guest system call %lld is not supported", number);
		return LINUX(38);
	}
}
