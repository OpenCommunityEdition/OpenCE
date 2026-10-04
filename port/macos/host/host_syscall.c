/*
HOST_SYSCALL.C

System calls on behalf of the guest's musl runtime (its syscall_arch.h
sends every call here), Linux numbers on Darwin. The port's Android host
(port/android/host/host_syscall.c) passes most calls straight through to
the Linux kernel; Darwin's kernel has different numbers, different
structures and no futexes, so this file translates:

- structure layouts: timespec and timeval (the guest's time_t is 32-bit),
  stat (musl's, not Darwin's), iovec;
- futexes, which musl's locks are built on: Darwin has none, so the four
  words of a futex wait become a pthread mutex/cond pair per futex word,
  in a global table (the game uses a handful; the table grows on demand);
- memory mappings, which the pools own (host_low_map, never a real mmap);
- the standard streams, which go to the log;
- process exit, and calls with no meaning here (signal handlers, which
  the host owns; clone, which pthread_create replaces through
  host_thread_create's doorbell).
*/

#include "host.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

uint32_t host_guest_address(const void *host);

/* the guest's structures */
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

#define GUEST(type, value) ((type)(uintptr_t)(uint32_t)(value))

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

static long result_of(long value)
{
	return value == -1 ? -errno : value;
}

/* ---------- standard output and error */

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
			host_log(fd == 2 ? HOST_LOG_WARN : HOST_LOG_INFO, stream->line);
			stream->length = 0;
			if (c == '\n')
				continue;
		}
		stream->line[stream->length++] = c;
	}
	pthread_mutex_unlock(&log_lock);
}

static long guest_writev(int fd, uint64_t vector, int count)
{
	const struct guest_iovec *guest_vector = GUEST(const struct guest_iovec *, vector);
	int index;
	long total = 0;

	if (count < 0 || count > 64)
		return -EINVAL;
	for (index = 0; index < count; index++)
	{
		log_bytes(fd, GUEST(const char *, guest_vector[index].base), guest_vector[index].length);
		total += (long)guest_vector[index].length;
	}
	return total;
}

static long guest_readv(int fd, uint64_t vector, int count)
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
	return result_of((long)readv(fd, host_vector, (int)count));
}

/* ---------- futexes on pthread primitives

musl's locks are futex words: 0 is unlocked, 1 locked, 2 locked with
waiters. Each word gets a mutex/cond pair (lazily, in a table keyed by
the guest address) while waiters exist.

WAKE wakes that many; WAIT compares and sleeps; the PI operations never
come from this guest (its musl has no priority inheritance). */

#define FUTEX_TABLE 512

struct futex_entry
{
	uint32_t *word;      /* the guest address, as a host pointer */
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int waiters;
	int used;
};

static struct futex_entry futexes[FUTEX_TABLE];
static pthread_mutex_t futexes_lock = PTHREAD_MUTEX_INITIALIZER;

static struct futex_entry *futex_of(uint32_t address)
{
	uint32_t index = (address / 4) % FUTEX_TABLE;
	struct futex_entry *entry = &futexes[index];
	uint32_t *word = GUEST(uint32_t *, address);

	pthread_mutex_lock(&futexes_lock);
	if (entry->used && entry->word == word)
	{
		pthread_mutex_unlock(&futexes_lock);
		return entry;
	}
	if (entry->used && entry->waiters)
	{
		/* (a hash collision with live waiters: linear probe) */
		uint32_t attempt;

		for (attempt = 1; attempt < FUTEX_TABLE; attempt++)
		{
			entry = &futexes[(index + attempt) % FUTEX_TABLE];
			if (!entry->used || (entry->used && entry->word == word))
				break;
		}
		if (entry->used && entry->word != word)
		{
			pthread_mutex_unlock(&futexes_lock);
			return NULL;
		}
	}
	if (entry->used)
	{
		pthread_mutex_destroy(&entry->lock);
		pthread_cond_destroy(&entry->cond);
	}
	entry->word = word;
	entry->waiters = 0;
	pthread_mutex_init(&entry->lock, NULL);
	pthread_cond_init(&entry->cond, NULL);
	entry->used = 1;
	pthread_mutex_unlock(&futexes_lock);
	return entry;
}

static long guest_futex(uint32_t address, int operation, uint32_t value, uint64_t timeout)
{
	struct futex_entry *entry = futex_of(address);

	if (!entry)
		return -ENOMEM;
	switch (operation & 0x7f)
	{
	case 0: /* FUTEX_WAIT */
	case 9: /* FUTEX_WAIT_BITSET (the guest's musl uses it for robust
	           wakes with all bits set, which compares equal) */
	{
		struct timespec host_timeout;
		struct timespec *timeout_pointer = NULL;
		uint32_t current;
		long result = 0;

		if (timespec_in(timeout, &host_timeout))
			timeout_pointer = &host_timeout;
		pthread_mutex_lock(&entry->lock);
		current = *entry->word;
		if (current != value)
		{
			pthread_mutex_unlock(&entry->lock);
			return -EAGAIN;
		}
		entry->waiters++;
		if (timeout_pointer)
			pthread_cond_timedwait_relative_np(&entry->cond, &entry->lock, &host_timeout);
		else
			pthread_cond_wait(&entry->cond, &entry->lock);
		entry->waiters--;
		pthread_mutex_unlock(&entry->lock);
		return result;
	}
	case 1: /* FUTEX_WAKE */
	case 10: /* FUTEX_WAKE_BITSET */
	{
		uint32_t woken = 0;

		pthread_mutex_lock(&entry->lock);
		while (woken < value && entry->waiters)
		{
			pthread_cond_signal(&entry->cond);
			woken++;
		}
		pthread_mutex_unlock(&entry->lock);
		return (long)woken;
	}
	default:
		return -ENOSYS;
	}
}

/* ---------- memory: the pools own the guest's mappings */

static long guest_mmap(uint64_t address, uint32_t size, int protection, int flags, int fd, int64_t offset)
{
	(void)protection;
	(void)flags;
	(void)fd;
	(void)offset;
	/* the Xbox window: already mapped (host_vm.c); succeed */
	if (address == GUEST_WINDOW_BASE && size <= GUEST_WINDOW_SIZE)
		return (long)GUEST_WINDOW_BASE;
	if (address)
		host_fatal("the guest's mmap asked for %llx specifically (only the window is fixed)",
			(unsigned long long)address);
	{
		void *mapping = host_low_map(size);

		if (!mapping)
			return -ENOMEM;
		return (long)(uintptr_t)host_guest_address(mapping);
	}
}

/* ---------- structures the guest's musl reads

fstat and statx: musl's struct kstat (its arch's kstat.h) is what the
guest's syscalls fill; statx the guest only uses for sizes and times on
new kernels, and musl's build for this guest (arm64_32) sends fstat64
numbers 80 (statx) and 79 (fstat) — the macOS side answers both from
fstat(2). */

struct guest_kstat
{
	uint32_t device;
	uint32_t inode;
	uint32_t mode;
	uint32_t links;
	uint32_t user;
	uint32_t group;
	uint32_t rdev;
	uint64_t size;
	int64_t blocks;
	int64_t atime_seconds;
	int64_t atime_nanoseconds;
	int64_t mtime_seconds;
	int64_t mtime_nanoseconds;
	int64_t ctime_seconds;
	int64_t ctime_nanoseconds;
	uint64_t inode_high;
	uint64_t device_high;
};

static long guest_fstat(int fd, uint64_t result_address)
{
	struct stat information;
	struct guest_kstat *result = GUEST(struct guest_kstat *, result_address);

	if (fstat(fd, &information) != 0)
		return -errno;
	memset(result, 0, sizeof(*result));
	result->device = (uint32_t)information.st_dev;
	result->inode = (uint32_t)information.st_ino;
	result->mode = (uint32_t)information.st_mode;
	result->links = (uint32_t)information.st_nlink;
	result->user = (uint32_t)information.st_uid;
	result->group = (uint32_t)information.st_gid;
	result->rdev = (uint32_t)information.st_rdev;
	result->size = (uint64_t)information.st_size;
	result->blocks = (int64_t)information.st_blocks;
	result->atime_seconds = (int64_t)information.st_atime;
	result->mtime_seconds = (int64_t)information.st_mtime;
	result->ctime_seconds = (int64_t)information.st_ctime;
	return 0;
}

/* ---------- dispatch */

/* the Linux numbers the guest's musl makes (its arch's syscall.h) */
#define SYS_read 63
#define SYS_write 64
#define SYS_writev 66
#define SYS_readv 65
#define SYS_pread64 67
#define SYS_pwrite64 68
#define SYS_readlinkat 78
#define SYS_newfstatat 79
#define SYS_fstat 80
#define SYS_statx 291
#define SYS_exit 93
#define SYS_exit_group 94
#define SYS_set_tid_address 96
#define SYS_futex 98
#define SYS_set_robust_list 99
#define SYS_get_robust_list 100
#define SYS_clock_gettime 113
#define SYS_clock_getres 114
#define SYS_clock_nanosleep 115
#define SYS_syslog 116
#define SYS_kill 129
#define SYS_tkill 130
#define SYS_tgkill 131
#define SYS_rt_sigaction 134
#define SYS_rt_sigprocmask 135
#define SYS_sigaltstack 132
#define SYS_getpid 172
#define SYS_getppid 173
#define SYS_getuid 174
#define SYS_geteuid 175
#define SYS_getgid 176
#define SYS_getegid 177
#define SYS_gettid 178
#define SYS_getrandom 278
#define SYS_membarrier 283
#define SYS_nanosleep 101
#define SYS_getdents64 61
#define SYS_openat 56
#define SYS_close 57
#define SYS_lseek 62
#define SYS_unlinkat 35
#define SYS_renameat 38
#define SYS_renameat2 276
#define SYS_mkdirat 34
#define SYS_fchmod 52
#define SYS_fchmodat 53
#define SYS_ftruncate 46
#define SYS_faccessat 48
#define SYS_chdir 49
#define SYS_getcwd 17
#define SYS_fcntl 25
#define SYS_ioctl 29
#define SYS_flock 32
#define SYS_fsync 82
#define SYS_fdatasync 83
#define SYS_dup 23
#define SYS_dup3 24
#define SYS_pipe2 59
#define SYS_madvise 233
#define SYS_sched_yield 124
#define SYS_prlimit64 261
#define SYS_getrlimit 163
#define SYS_umask 166
#define SYS_getrusage 165
#define SYS_uname 160
#define SYS_socket 198
#define SYS_socketpair 199
#define SYS_bind 200
#define SYS_listen 201
#define SYS_accept 202
#define SYS_connect 203
#define SYS_getsockname 204
#define SYS_getpeername 205
#define SYS_sendto 206
#define SYS_recvfrom 207
#define SYS_setsockopt 208
#define SYS_getsockopt 209
#define SYS_shutdown 210
#define SYS_sendmsg 211
#define SYS_recvmsg 212
#define SYS_mmap 222
#define SYS_brk 214
#define SYS_gettimeofday 169
#define SYS_munmap 215
#define SYS_mprotect 226

long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	(void)e;
	(void)f;
	switch (number)
	{
	case SYS_write:
		if (a == 1 || a == 2)
		{
			log_bytes((int)a, GUEST(const char *, b), (size_t)(uint32_t)c);
			return (uint32_t)c;
		}
		return result_of((long)write((int)a, GUEST(const void *, b), (size_t)(uint32_t)c));
	case SYS_writev:
		if (a == 1 || a == 2)
			return guest_writev((int)a, (uint64_t)b, (int)c);
		return result_of((long)writev((int)a, GUEST(const struct iovec *, b), (int)c));
	case SYS_readv:
		return guest_readv((int)a, (uint64_t)b, (int)c);
	case SYS_read:
		return result_of((long)read((int)a, GUEST(void *, b), (size_t)(uint32_t)c));
	case SYS_pread64:
	{
		off_t where = (off_t)(int64_t)d;

		return result_of((long)pread((int)a, GUEST(void *, b), (size_t)(uint32_t)c, where));
	}
	case SYS_pwrite64:
	{
		off_t where = (off_t)(int64_t)d;

		return result_of((long)pwrite((int)a, GUEST(const void *, b), (size_t)(uint32_t)c, where));
	}
	case SYS_lseek:
		return result_of((long)lseek((int)a, (off_t)(int64_t)b, (int)c));

	case SYS_clock_gettime:
	case SYS_clock_getres:
	{
		struct timespec value;
		long result;

		if (number == SYS_clock_gettime)
			result = clock_gettime((clockid_t)(a == 1 ? CLOCK_MONOTONIC : a == 0 ? CLOCK_REALTIME : a), &value);
		else
			result = 0, value.tv_sec = 1, value.tv_nsec = 0;
		if (result == 0)
			timespec_out((uint64_t)b, &value);
		return result_of(result);
	}
	case SYS_gettimeofday:
	{
		struct timeval value;
		struct guest_timespec *result = GUEST(struct guest_timespec *, a);

		gettimeofday(&value, NULL);
		if (result)
		{
			result->seconds = (int32_t)value.tv_sec;
			result->nanoseconds = (int32_t)(value.tv_usec * 1000);
		}
		return 0;
	}
	case SYS_nanosleep:
	{
		struct timespec request, remaining;
		long result;

		if (!timespec_in((uint64_t)a, &request))
			return -EFAULT;
		result = result_of((long)nanosleep(&request, &remaining));
		if (result == -EINTR)
			timespec_out((uint64_t)b, &remaining);
		return result;
	}
	case SYS_clock_nanosleep:
	{
		struct timespec request, remaining;
		int result;

		if (!timespec_in((uint64_t)c, &request))
			return -EFAULT;
		result = (int)nanosleep(&request, &remaining);
		if (result == EINTR)
			timespec_out((uint64_t)d, &remaining);
		return -result;
	}
	case SYS_futex:
		return guest_futex((uint32_t)a, (int)b, (uint32_t)c, (uint64_t)d);
	case SYS_set_robust_list:
	case SYS_get_robust_list:
		return 0;

	case SYS_brk:
	{
		/* one contiguous region for musl's brk (its malloc grows
		through here; the pools back it); growth past the region
		fails and musl falls back to mmap */
		static uint32_t brk_base, brk_cursor, brk_end;

		if (!brk_base)
		{
			void *region = host_low_map(0x4000000); /* 64 MB */

			if (!region)
				return -ENOMEM;
			brk_base = host_guest_address(region);
			brk_cursor = brk_base;
			brk_end = brk_base + 0x4000000;
			host_logf(HOST_LOG_INFO, "guest brk at %x", brk_base);
		}
		if (!a)
			return brk_cursor;
		{
			uint32_t want = (uint32_t)a;

			if (want < brk_base || want > brk_end)
				return brk_cursor; /* refuse: musl falls back to mmap */
			brk_cursor = want;
			return brk_cursor;
		}
	}
	case SYS_mmap:
		return guest_mmap((uint64_t)a, (uint32_t)b, (int)c, (int)d, (int)e, (int64_t)f);
	case SYS_munmap:
		host_low_unmap(GUEST(void *, a), (size_t)(uint32_t)b);
		return 0;


	case SYS_exit:
	case SYS_exit_group:
		host_exit((int)a);

	case SYS_set_tid_address:
		return (long)pthread_mach_thread_np(pthread_self());
	case SYS_gettid:
		return (long)pthread_mach_thread_np(pthread_self());
	case SYS_getpid:
		return (long)getpid();
	case SYS_getppid:
		return (long)getppid();
	case SYS_getuid:
		return (long)getuid();
	case SYS_geteuid:
		return (long)geteuid();
	case SYS_getgid:
		return (long)getgid();
	case SYS_getegid:
		return (long)getegid();
	case SYS_sched_yield:
		return sched_yield();
	case SYS_rt_sigaction:
	case SYS_sigaltstack:
	case SYS_rt_sigprocmask:
		return 0; /* the host owns signals */
	case SYS_ioctl:
		return -ENOTTY;
	case SYS_getrandom:
		arc4random_buf(GUEST(void *, b), (size_t)(uint32_t)c);
		return (uint32_t)c;
	case SYS_membarrier:
	case SYS_syslog:
	case SYS_madvise:
	case SYS_mprotect:
		return 0;
	case SYS_kill:
	case SYS_tkill:
	case SYS_tgkill:
		return 0;

	/* everything else the guest's musl might ask (the platform's file
	and socket work goes through the hostposix_* doorbells) */
	default:
		host_logf(HOST_LOG_WARN, "guest system call %lld is not supported", number);
		return -ENOSYS;
	}
}
