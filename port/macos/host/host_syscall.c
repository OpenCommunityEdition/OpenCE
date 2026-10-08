/*
HOST_SYSCALL.C

System calls on behalf of the guest's musl runtime (its syscall_arch.h
sends every call here, guest_syscall.c), on Darwin.

The guest speaks Linux: AArch64 Linux's call numbers (generated from the
guest's own list into guest_syscall_numbers.h; never Darwin's
<sys/syscall.h>, whose numbers mean other calls), flags, structures and
errno values. Each call is performed with Darwin's C library and translated
both ways (host_linux.c): flags and constants in, structures and -errno
(Linux's numbering) out. The guest's pointers are valid host addresses; its
32-bit integers arrive zero-extended, so signed ones are narrowed first.

Some calls have no Darwin counterpart and are built from others:

- futex, from os_sync_wait_on_address (macOS 14.4 and later) or the
  __ulock calls it is made of (earlier); a requeue wakes instead, which
  futex semantics allow (musl's condition variables wait again);
- getdents64, from a directory stream per descriptor, packed into Linux's
  linux_dirent64 records;
- clock_nanosleep's absolute sleeps, from the clock and nanosleep;
- stat's kernel structure, utsname and sysinfo, filled field by field.

The standard output and error streams also go to the host log.
*/

#include "host.h"
#include "host_linux.h"
#include "guest_syscall_numbers.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <os/os_sync_wait_on_address.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

void host_abort(const char *reason) __attribute__((noreturn));
/* (host_main.c) the guest's standard output and error, for the host log */
void host_log_guest_output(int fd, const char *bytes, size_t size);

/* the private calls os_sync_wait_on_address is built on, for macOS before
14.4 (present since 11) */
extern int __ulock_wait2(uint32_t operation, void *address, uint64_t value, uint64_t timeout_nanoseconds,
	uint64_t value2);
extern int __ulock_wake(uint32_t operation, void *address, uint64_t wake_value);
#define UL_COMPARE_AND_WAIT 1
#define ULF_WAKE_ALL 0x00000100

struct guest_iovec
{
	uint32_t base;
	uint32_t length;
};

/* a guest address argument, which arrives zero-extended (the native test of
these calls, tools/test_macos_host_linux.py, passes host addresses whole) */
#ifndef GUEST
#define GUEST(type, value) ((type)(uintptr_t)(uint32_t)(value))
#endif

static int timespec_in(uint64_t address, struct timespec *result)
{
	const struct guest_timespec *value = GUEST(const struct guest_timespec *, address);

	if (!(uint32_t)address)
		return 0;
	result->tv_sec = value->seconds;
	result->tv_nsec = value->nanoseconds;
	return 1;
}

static void timespec_out(uint64_t address, const struct timespec *value)
{
	struct guest_timespec *result = GUEST(struct guest_timespec *, address);

	if (!(uint32_t)address)
		return;
	result->seconds = (int32_t)value->tv_sec;
	result->nanoseconds = (int32_t)value->tv_nsec;
}

static long result_of(long value)
{
	return value == -1 ? -host_linux_errno(errno) : value;
}

static int64_t nanoseconds_of(const struct timespec *value)
{
	return (int64_t)value->tv_sec * 1000000000ll + value->tv_nsec;
}

/* ---------- reading and writing */

static long guest_writev(int fd, uint64_t vector, int count, int64_t offset, int positional)
{
	struct iovec host_vector[64];
	const struct guest_iovec *guest_vector = GUEST(const struct guest_iovec *, vector);
	long result;
	int index;

	if (count < 0 || count > 64)
		return -LINUX_EINVAL;
	for (index = 0; index < count; index++)
	{
		host_vector[index].iov_base = GUEST(void *, guest_vector[index].base);
		host_vector[index].iov_len = guest_vector[index].length;
	}
	if (positional)
		result = result_of(pwritev(fd, host_vector, count, offset));
	else
		result = result_of(writev(fd, host_vector, count));
	if ((fd == 1 || fd == 2) && result > 0)
	{
		long left = result;

		for (index = 0; index < count && left > 0; index++)
		{
			size_t size = host_vector[index].iov_len < (size_t)left ? host_vector[index].iov_len : (size_t)left;

			host_log_guest_output(fd, host_vector[index].iov_base, size);
			left -= (long)size;
		}
	}
	return result;
}

static long guest_readv(int fd, uint64_t vector, int count, int64_t offset, int positional)
{
	struct iovec host_vector[64];
	const struct guest_iovec *guest_vector = GUEST(const struct guest_iovec *, vector);
	int index;

	if (count < 0 || count > 64)
		return -LINUX_EINVAL;
	for (index = 0; index < count; index++)
	{
		host_vector[index].iov_base = GUEST(void *, guest_vector[index].base);
		host_vector[index].iov_len = guest_vector[index].length;
	}
	if (positional)
		return result_of(preadv(fd, host_vector, count, offset));
	return result_of(readv(fd, host_vector, count));
}

/* ---------- directories (getdents64)

Darwin has no getdents64 with Linux's records: each directory descriptor
the guest reads gets a stream of its own (on a duplicate of the
descriptor), whose entries are packed into the guest's buffer. An entry
that does not fit waits for the next call. */

struct directory_stream
{
	struct directory_stream *next;
	int fd;
	DIR *directory;
	int pending;
	uint64_t pending_inode;
	int64_t pending_offset;
	unsigned char pending_type;
	size_t pending_length;
	char pending_name[1024];
};

static struct directory_stream *directory_streams;
static pthread_mutex_t directory_lock = PTHREAD_MUTEX_INITIALIZER;

/* called with the lock held */
static struct directory_stream **directory_stream_link(int fd)
{
	struct directory_stream **link;

	for (link = &directory_streams; *link; link = &(*link)->next)
	{
		if ((*link)->fd == fd)
			return link;
	}
	return NULL;
}

static void directory_stream_forget(int fd)
{
	struct directory_stream **link;

	pthread_mutex_lock(&directory_lock);
	link = directory_stream_link(fd);
	if (link)
	{
		struct directory_stream *stream = *link;

		*link = stream->next;
		closedir(stream->directory);
		free(stream);
	}
	pthread_mutex_unlock(&directory_lock);
}

static long guest_getdents64(int fd, uint64_t buffer_address, uint32_t capacity)
{
	unsigned char *buffer = GUEST(unsigned char *, buffer_address);
	struct directory_stream **link, *stream;
	long written = 0;

	pthread_mutex_lock(&directory_lock);
	link = directory_stream_link(fd);
	stream = link ? *link : NULL;
	if (!stream)
	{
		int duplicate = dup(fd);
		DIR *directory = duplicate >= 0 ? fdopendir(duplicate) : NULL;

		if (!directory)
		{
			int error = errno;

			if (duplicate >= 0)
				close(duplicate);
			pthread_mutex_unlock(&directory_lock);
			return -host_linux_errno(error);
		}
		stream = calloc(1, sizeof(*stream));
		stream->fd = fd;
		stream->directory = directory;
		stream->next = directory_streams;
		directory_streams = stream;
	}
	for (;;)
	{
		unsigned size;

		if (!stream->pending)
		{
			struct dirent *entry;

			errno = 0;
			entry = readdir(stream->directory);
			if (!entry)
			{
				if (errno && !written)
					written = -host_linux_errno(errno);
				break;
			}
			stream->pending = 1;
			stream->pending_inode = entry->d_ino;
			/* (where the next entry starts, for seekdir) */
			stream->pending_offset = telldir(stream->directory);
			stream->pending_type = entry->d_type;
			stream->pending_length = entry->d_namlen < sizeof(stream->pending_name) - 1 ? entry->d_namlen :
				sizeof(stream->pending_name) - 1;
			memcpy(stream->pending_name, entry->d_name, stream->pending_length);
			stream->pending_name[stream->pending_length] = 0;
		}
		size = host_linux_pack_dirent64(buffer + written, capacity - (uint32_t)written, stream->pending_inode,
			stream->pending_offset, stream->pending_type, stream->pending_name, stream->pending_length);
		if (!size)
		{
			if (!written)
				written = -LINUX_EINVAL;
			break;
		}
		stream->pending = 0;
		written += size;
	}
	pthread_mutex_unlock(&directory_lock);
	return written;
}

/* lseek on a directory the guest reads: rewinddir and seekdir */
static int directory_seek(int fd, int64_t offset, int whence, long *result)
{
	struct directory_stream **link;
	int handled = 0;

	pthread_mutex_lock(&directory_lock);
	link = directory_stream_link(fd);
	if (link && whence == 0)
	{
		struct directory_stream *stream = *link;

		if (offset == 0)
			rewinddir(stream->directory);
		else
			seekdir(stream->directory, (long)offset);
		stream->pending = 0;
		*result = (long)offset;
		handled = 1;
	}
	pthread_mutex_unlock(&directory_lock);
	return handled;
}

/* ---------- time */

static long guest_clock_nanosleep(int linux_clock, int flags, uint64_t request_address, uint64_t remaining_address)
{
	struct timespec request;
	clockid_t clock;

	if (host_linux_clock_to_darwin(linux_clock, &clock) != 0)
		return -LINUX_EINVAL;
	if (!timespec_in(request_address, &request))
		return -LINUX_EFAULT;
	if (request.tv_nsec < 0 || request.tv_nsec >= 1000000000)
		return -LINUX_EINVAL;
	if (flags & LINUX_TIMER_ABSTIME)
	{
		/* Darwin has no absolute sleep: sleep for what is left until then
		(again, should anything interrupt it) */
		int64_t target = nanoseconds_of(&request);

		for (;;)
		{
			struct timespec now, left;
			int64_t remaining;

			clock_gettime(clock, &now);
			remaining = target - nanoseconds_of(&now);
			if (remaining <= 0)
				return 0;
			left.tv_sec = (time_t)(remaining / 1000000000ll);
			left.tv_nsec = (long)(remaining % 1000000000ll);
			nanosleep(&left, NULL);
		}
	}
	/* the guest has no signal handlers of its own, so an interruption
	(the sampler's signal) never ends a sleep early */
	while (nanosleep(&request, &request) != 0)
	{
		if (errno != EINTR)
			return -host_linux_errno(errno);
	}
	(void)remaining_address;
	return 0;
}

/* ---------- futex */

static int use_os_sync(void)
{
	if (__builtin_available(macOS 14.4, *))
		return 1;
	return 0;
}

/* waits while the 32-bit word at address holds value; timeout in
nanoseconds, or < 0 for none */
static long futex_wait(void *address, uint32_t value, int64_t timeout)
{
	int result;

	if (__atomic_load_n((volatile uint32_t *)address, __ATOMIC_SEQ_CST) != value)
		return -LINUX_EAGAIN;
	if (timeout == 0)
		return -LINUX_ETIMEDOUT;
	/* (always the process-private form, whatever the guest asked for:
	Darwin keys private and shared waits differently, and the guest is one
	process) */
	if (use_os_sync())
	{
		if (__builtin_available(macOS 14.4, *))
		{
			if (timeout < 0)
				result = os_sync_wait_on_address(address, value, 4, OS_SYNC_WAIT_ON_ADDRESS_NONE);
			else
				result = os_sync_wait_on_address_with_timeout(address, value, 4, OS_SYNC_WAIT_ON_ADDRESS_NONE,
					OS_CLOCK_MACH_ABSOLUTE_TIME, (uint64_t)timeout);
		}
		else
		{
			result = 0;
		}
	}
	else
	{
		result = __ulock_wait2(UL_COMPARE_AND_WAIT, address, value, timeout < 0 ? 0 : (uint64_t)timeout, 0);
	}
	if (result >= 0)
		return 0;
	switch (errno)
	{
	case ETIMEDOUT: return -LINUX_ETIMEDOUT;
	case EINTR: return -LINUX_EINTR;
	case EFAULT: return -LINUX_EFAULT;
	default: return -host_linux_errno(errno);
	}
}

static long futex_wake(void *address, int count)
{
	int result;

	if (count <= 0)
		return 0;
	if (use_os_sync())
	{
		if (__builtin_available(macOS 14.4, *))
		{
			result = count == 1 ? os_sync_wake_by_address_any(address, 4, OS_SYNC_WAKE_BY_ADDRESS_NONE) :
				os_sync_wake_by_address_all(address, 4, OS_SYNC_WAKE_BY_ADDRESS_NONE);
		}
		else
		{
			result = 0;
		}
	}
	else
	{
		result = __ulock_wake(UL_COMPARE_AND_WAIT | (count == 1 ? 0 : ULF_WAKE_ALL), address, 0);
	}
	/* (how many woke is not known; musl does not ask) */
	return result == 0 ? 1 : 0;
}

static long guest_futex(uint64_t address, int operation, uint32_t value, uint64_t timeout_address,
	uint64_t address2, uint32_t value3)
{
	void *word = GUEST(void *, address);
	int command = operation & LINUX_FUTEX_COMMAND_MASK;

	(void)address2;
	switch (command)
	{
	case LINUX_FUTEX_WAIT:
	case LINUX_FUTEX_WAIT_BITSET:
	{
		struct timespec timeout;
		int64_t nanoseconds = -1;

		if (command == LINUX_FUTEX_WAIT_BITSET && !value3)
			return -LINUX_EINVAL;
		if (timespec_in(timeout_address, &timeout))
		{
			nanoseconds = nanoseconds_of(&timeout);
			/* FUTEX_WAIT's timeout is relative; FUTEX_WAIT_BITSET's an
			absolute time on the monotonic clock (or the real-time one) */
			if (command == LINUX_FUTEX_WAIT_BITSET)
			{
				struct timespec now;

				clock_gettime((operation & LINUX_FUTEX_CLOCK_REALTIME) ? CLOCK_REALTIME : CLOCK_UPTIME_RAW, &now);
				nanoseconds -= nanoseconds_of(&now);
				if (nanoseconds < 0)
					nanoseconds = 0;
			}
			if (nanoseconds < 0)
				return -LINUX_EINVAL;
		}
		return futex_wait(word, value, nanoseconds);
	}
	case LINUX_FUTEX_WAKE:
	case LINUX_FUTEX_WAKE_BITSET:
		return futex_wake(word, (int)value);
	case LINUX_FUTEX_CMP_REQUEUE:
		if (__atomic_load_n((volatile uint32_t *)word, __ATOMIC_SEQ_CST) != value3)
			return -LINUX_EAGAIN;
		/* fall through */
	case LINUX_FUTEX_REQUEUE:
	{
		/* wake those that would have been requeued as well (the fourth
		argument is their number): a waiter that wakes finds its condition
		unchanged and waits again */
		uint64_t total = (uint64_t)(uint32_t)value + (uint32_t)timeout_address;

		futex_wake(word, total > 1 ? INT_MAX : (int)total);
		return 0;
	}
	default:
		return -LINUX_ENOSYS;
	}
}

/* ---------- other calls with structures */

static long guest_fcntl(int fd, int command, uint64_t argument)
{
	int darwin_command = host_linux_fcntl_command_to_darwin(command);

	if (darwin_command < 0)
		return -LINUX_EINVAL;
	switch (command)
	{
	case LINUX_F_GETFL:
	{
		int flags = fcntl(fd, F_GETFL);

		return flags < 0 ? -host_linux_errno(errno) : host_linux_open_flags_from_darwin(flags);
	}
	case LINUX_F_SETFL:
	{
		int flags = host_linux_open_flags_to_darwin((int)argument);

		return result_of(fcntl(fd, F_SETFL, flags < 0 ? 0 : flags));
	}
	case LINUX_F_GETLK:
	case LINUX_F_SETLK:
	case LINUX_F_SETLKW:
	{
		struct guest_flock *guest = GUEST(struct guest_flock *, argument);
		struct flock lock;
		long result;

		if (!guest || host_linux_flock_to_darwin(guest, &lock) != 0)
			return -LINUX_EINVAL;
		result = result_of(fcntl(fd, darwin_command, &lock));
		if (result >= 0 && command == LINUX_F_GETLK)
			host_linux_flock_from_darwin(&lock, guest);
		return result;
	}
	case LINUX_F_DUPFD:
	case LINUX_F_DUPFD_CLOEXEC:
	case LINUX_F_SETFD:
	case LINUX_F_SETOWN:
		/* (FD_CLOEXEC is 1 on both) */
		return result_of(fcntl(fd, darwin_command, (int)argument));
	default:
		return result_of(fcntl(fd, darwin_command));
	}
}

static long guest_stat(int dirfd, uint64_t path_address, uint64_t kstat_address, int flags)
{
	const char *path = GUEST(const char *, path_address);
	struct guest_kstat *result = GUEST(struct guest_kstat *, kstat_address);
	struct stat information;
	int at_flags = host_linux_at_flags_to_darwin(flags, 0);

	if (!result || at_flags < 0)
		return -LINUX_EINVAL;
	if ((flags & LINUX_AT_EMPTY_PATH) && (!path || !*path))
	{
		if (fstat(dirfd, &information) != 0)
			return -host_linux_errno(errno);
	}
	else if (!path)
	{
		return -LINUX_EFAULT;
	}
	else if (fstatat(dirfd, path, &information, at_flags) != 0)
	{
		return -host_linux_errno(errno);
	}
	host_linux_fill_kstat(result, &information);
	return 0;
}

static long guest_readlinkat(int dirfd, uint64_t path_address, uint64_t buffer_address, uint32_t size)
{
	const char *path = GUEST(const char *, path_address);
	char *buffer = GUEST(char *, buffer_address);

	if (!path)
		return -LINUX_EFAULT;
	/* the one /proc link the platform layer reads (xbox_files.c) */
	if (!strcmp(path, "/proc/self/exe"))
	{
		char executable[PATH_MAX], resolved[PATH_MAX];
		uint32_t length = sizeof(executable);
		size_t copied;

		if (_NSGetExecutablePath(executable, &length) != 0)
			return -LINUX_ENAMETOOLONG;
		if (!realpath(executable, resolved))
			snprintf(resolved, sizeof(resolved), "%s", executable);
		copied = strlen(resolved);
		if (copied > size)
			copied = size;
		memcpy(buffer, resolved, copied);
		return (long)copied;
	}
	return result_of(readlinkat(dirfd, path, buffer, size));
}

static long guest_utimensat(int dirfd, uint64_t path_address, uint64_t times_address, int flags)
{
	const char *path = GUEST(const char *, path_address);
	struct timespec times[2];
	int at_flags = host_linux_at_flags_to_darwin(flags, 0);

	if (at_flags < 0)
		return -LINUX_EINVAL;
	if ((uint32_t)times_address)
	{
		int index;

		timespec_in(times_address, &times[0]);
		timespec_in(times_address + sizeof(struct guest_timespec), &times[1]);
		for (index = 0; index < 2; index++)
			times[index].tv_nsec = host_linux_utime_nanoseconds_to_darwin(times[index].tv_nsec);
	}
	/* (no path: the descriptor itself, musl's futimens) */
	if (!path)
		return result_of(futimens(dirfd, (uint32_t)times_address ? times : NULL));
	return result_of(utimensat(dirfd, path, (uint32_t)times_address ? times : NULL, at_flags));
}

static long guest_ppoll(uint64_t descriptors_address, uint32_t count, uint64_t timeout_address)
{
	struct pollfd *descriptors = GUEST(struct pollfd *, descriptors_address);
	struct pollfd local[64];
	struct pollfd *host_descriptors = count <= 64 ? local : calloc(count, sizeof(struct pollfd));
	struct timespec timeout;
	int milliseconds = -1;
	uint32_t index;
	long result;

	if (!host_descriptors)
		return -LINUX_ENOMEM;
	if (timespec_in(timeout_address, &timeout))
	{
		int64_t nanoseconds = nanoseconds_of(&timeout);

		if (nanoseconds < 0)
			nanoseconds = 0;
		milliseconds = nanoseconds / 1000000 > INT_MAX ? INT_MAX : (int)((nanoseconds + 999999) / 1000000);
	}
	/* (struct pollfd is laid out alike; some event bits differ) */
	for (index = 0; index < count; index++)
	{
		host_descriptors[index].fd = descriptors[index].fd;
		host_descriptors[index].events = host_linux_poll_events_to_darwin(descriptors[index].events);
		host_descriptors[index].revents = 0;
	}
	result = result_of(poll(host_descriptors, count, milliseconds));
	for (index = 0; index < count; index++)
		descriptors[index].revents = host_linux_poll_events_from_darwin(host_descriptors[index].revents);
	if (host_descriptors != local)
		free(host_descriptors);
	return result;
}

static long guest_pipe2(uint64_t fds_address, int flags)
{
	int32_t *result = GUEST(int32_t *, fds_address);
	int fds[2];
	int index;

	if (flags & ~(LINUX_O_CLOEXEC | LINUX_O_NONBLOCK))
		return -LINUX_EINVAL;
	if (pipe(fds) != 0)
		return -host_linux_errno(errno);
	for (index = 0; index < 2; index++)
	{
		if (flags & LINUX_O_CLOEXEC)
			fcntl(fds[index], F_SETFD, FD_CLOEXEC);
		if (flags & LINUX_O_NONBLOCK)
			fcntl(fds[index], F_SETFL, fcntl(fds[index], F_GETFL) | O_NONBLOCK);
		result[index] = fds[index];
	}
	return 0;
}

static long guest_prlimit(int pid, int resource, uint64_t new_address, uint64_t old_address)
{
	int darwin_resource = host_linux_rlimit_resource_to_darwin(resource);
	uint64_t *new_limit = GUEST(uint64_t *, new_address);
	uint64_t *old_limit = GUEST(uint64_t *, old_address);
	struct rlimit limit;

	if (pid != 0 && pid != getpid())
		return -LINUX_EPERM;
	if (darwin_resource < 0)
		return -LINUX_EINVAL;
	if (old_limit)
	{
		if (getrlimit(darwin_resource, &limit) != 0)
			return -host_linux_errno(errno);
		old_limit[0] = host_linux_rlimit_value_from_darwin(limit.rlim_cur);
		old_limit[1] = host_linux_rlimit_value_from_darwin(limit.rlim_max);
	}
	if (new_limit)
	{
		limit.rlim_cur = host_linux_rlimit_value_to_darwin(new_limit[0]);
		limit.rlim_max = host_linux_rlimit_value_to_darwin(new_limit[1]);
		if (setrlimit(darwin_resource, &limit) != 0)
			return -host_linux_errno(errno);
	}
	return 0;
}

static long guest_sysinfo(uint64_t address)
{
	uint64_t memory = 0, free_memory = 0;
	size_t size = sizeof(memory);
	struct timeval boot;
	size_t boot_size = sizeof(boot);
	int name[2] = { CTL_KERN, KERN_BOOTTIME };
	uint64_t uptime = 0;
	vm_statistics64_data_t statistics;
	mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

	if (!(uint32_t)address)
		return -LINUX_EFAULT;
	sysctlbyname("hw.memsize", &memory, &size, NULL, 0);
	if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&statistics, &count) == KERN_SUCCESS)
		free_memory = (uint64_t)(statistics.free_count + statistics.inactive_count) * (uint64_t)vm_page_size;
	if (sysctl(name, 2, &boot, &boot_size, NULL, 0) == 0)
		uptime = (uint64_t)(time(NULL) - boot.tv_sec);
	host_linux_fill_sysinfo(GUEST(void *, address), uptime, memory, free_memory, 1);
	return 0;
}

static long guest_kill_thread(int thread, int signal_number)
{
	if (thread != host_thread_id())
		return -LINUX_ESRCH;
	if (signal_number == 0)
		return 0;
	/* abort() (musl raises SIGABRT on the calling thread) */
	if (signal_number == 6)
		host_abort("abort");
	signal_number = host_linux_signal_to_darwin(signal_number);
	if (signal_number < 0)
		return -LINUX_EINVAL;
	return -host_linux_errno(pthread_kill(pthread_self(), signal_number));
}

/* ---------- dispatch */

static void report_unsupported(long long number)
{
	static uint8_t reported[512];

	if (number >= 0 && number < (long long)sizeof(reported))
	{
		if (reported[number])
			return;
		reported[number] = 1;
	}
	host_logf(HOST_LOG_WARN, "guest system call %lld is not supported", number);
}

long long host_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	switch (number)
	{
	case GUEST_SYS_read:
		return result_of(read((int)a, GUEST(void *, b), (size_t)(uint32_t)c));
	case GUEST_SYS_write:
	{
		long result = result_of(write((int)a, GUEST(const void *, b), (size_t)(uint32_t)c));

		if (((int)a == 1 || (int)a == 2) && result > 0)
			host_log_guest_output((int)a, GUEST(const char *, b), (size_t)result);
		return result;
	}
	case GUEST_SYS_writev:
		return guest_writev((int)a, (uint64_t)b, (int)c, 0, 0);
	/* (the offset comes as two 32-bit halves, low in d, as on 32-bit
	Linux: musl's pwritev and preadv split it) */
	case GUEST_SYS_pwritev:
		return guest_writev((int)a, (uint64_t)b, (int)c, (long long)(((uint64_t)(uint32_t)e << 32) | (uint32_t)d), 1);
	case GUEST_SYS_readv:
		return guest_readv((int)a, (uint64_t)b, (int)c, 0, 0);
	case GUEST_SYS_preadv:
		return guest_readv((int)a, (uint64_t)b, (int)c, (long long)(((uint64_t)(uint32_t)e << 32) | (uint32_t)d), 1);
	case GUEST_SYS_pread64:
		return result_of(pread((int)a, GUEST(void *, b), (size_t)(uint32_t)c, (off_t)d));
	case GUEST_SYS_pwrite64:
		return result_of(pwrite((int)a, GUEST(const void *, b), (size_t)(uint32_t)c, (off_t)d));

	case GUEST_SYS_openat:
	{
		int flags = host_linux_open_flags_to_darwin((int)c);

		if (flags < 0)
			return -LINUX_EOPNOTSUPP;
		return result_of(openat(host_linux_at_fd(a), GUEST(const char *, b), flags, (mode_t)(uint32_t)d));
	}
	case GUEST_SYS_close:
		directory_stream_forget((int)a);
		return result_of(close((int)a));
	case GUEST_SYS_lseek:
	{
		int whence = host_linux_whence_to_darwin((int)c);
		long result;

		if (whence < 0)
			return -LINUX_EINVAL;
		if (directory_seek((int)a, b, (int)c, &result))
			return result;
		return result_of((long)lseek((int)a, (off_t)b, whence));
	}
	case GUEST_SYS_getdents64:
		return guest_getdents64((int)a, (uint64_t)b, (uint32_t)c);
	case GUEST_SYS_unlinkat:
	{
		int flags = host_linux_at_flags_to_darwin((int)c, AT_REMOVEDIR);

		if (flags < 0)
			return -LINUX_EINVAL;
		if (unlinkat(host_linux_at_fd(a), GUEST(const char *, b), flags) == 0)
			return 0;
		/* Darwin's unlink of a directory is EPERM; Linux's is EISDIR, which
		musl's remove() looks for before trying rmdir */
		if (errno == EPERM && !(flags & AT_REMOVEDIR))
		{
			struct stat status;

			if (fstatat(host_linux_at_fd(a), GUEST(const char *, b), &status, AT_SYMLINK_NOFOLLOW) == 0 &&
				S_ISDIR(status.st_mode))
				return -LINUX_EISDIR;
			errno = EPERM;
		}
		return -host_linux_errno(errno);
	}
	case GUEST_SYS_renameat:
		return result_of(renameat(host_linux_at_fd(a), GUEST(const char *, b), host_linux_at_fd(c),
			GUEST(const char *, d)));
	case GUEST_SYS_renameat2:
	{
		unsigned flags = 0;

		if ((uint32_t)e & ~(uint32_t)(LINUX_RENAME_NOREPLACE | LINUX_RENAME_EXCHANGE))
			return -LINUX_EINVAL;
		if ((uint32_t)e & LINUX_RENAME_NOREPLACE)
			flags |= RENAME_EXCL;
		if ((uint32_t)e & LINUX_RENAME_EXCHANGE)
			flags |= RENAME_SWAP;
		return result_of(renameatx_np(host_linux_at_fd(a), GUEST(const char *, b), host_linux_at_fd(c),
			GUEST(const char *, d), flags));
	}
	case GUEST_SYS_mkdirat:
		return result_of(mkdirat(host_linux_at_fd(a), GUEST(const char *, b), (mode_t)(uint32_t)c));
	case GUEST_SYS_fchmod:
		return result_of(fchmod((int)a, (mode_t)(uint32_t)b));
	case GUEST_SYS_fchmodat:
		return result_of(fchmodat(host_linux_at_fd(a), GUEST(const char *, b), (mode_t)(uint32_t)c, 0));
	case GUEST_SYS_ftruncate:
		return result_of(ftruncate((int)a, (off_t)b));
	case GUEST_SYS_fsync:
	case GUEST_SYS_fdatasync:
		return result_of(fsync((int)a));
	case GUEST_SYS_fcntl:
		return guest_fcntl((int)a, (int)b, (uint64_t)c);
	case GUEST_SYS_getcwd:
	{
		char *buffer = GUEST(char *, a);

		/* (the system call returns the length, with the terminator) */
		if (!getcwd(buffer, (size_t)(uint32_t)b))
			return -host_linux_errno(errno);
		return (long)strlen(buffer) + 1;
	}
	case GUEST_SYS_chdir:
		return result_of(chdir(GUEST(const char *, a)));
	case GUEST_SYS_readlinkat:
		return guest_readlinkat(host_linux_at_fd(a), (uint64_t)b, (uint64_t)c, (uint32_t)d);
	case GUEST_SYS_faccessat:
		return result_of(faccessat(host_linux_at_fd(a), GUEST(const char *, b), (int)c, 0));
	case GUEST_SYS_faccessat2:
	{
		int flags = host_linux_at_flags_to_darwin((int)d, AT_EACCESS);

		if (flags < 0)
			return -LINUX_EINVAL;
		return result_of(faccessat(host_linux_at_fd(a), GUEST(const char *, b), (int)c, flags));
	}
	case GUEST_SYS_dup:
		return result_of(dup((int)a));
	case GUEST_SYS_dup3:
	{
		long result;

		if ((int)a == (int)b || ((int)c & ~LINUX_O_CLOEXEC))
			return -LINUX_EINVAL;
		result = result_of(dup2((int)a, (int)b));
		/* (only once b is replaced: a failed dup3 leaves b, and its
		directory stream, as they were) */
		if (result >= 0)
			directory_stream_forget((int)b);
		if (result >= 0 && ((int)c & LINUX_O_CLOEXEC))
			fcntl((int)b, F_SETFD, FD_CLOEXEC);
		return result;
	}
	case GUEST_SYS_pipe2:
		return guest_pipe2((uint64_t)a, (int)b);
	case GUEST_SYS_fstat:
		return guest_stat((int)a, 0, (uint64_t)b, LINUX_AT_EMPTY_PATH);
	case GUEST_SYS_newfstatat:
		return guest_stat(host_linux_at_fd(a), (uint64_t)b, (uint64_t)c, (int)d);
	case GUEST_SYS_utimensat:
		return guest_utimensat(host_linux_at_fd(a), (uint64_t)b, (uint64_t)c, (int)d);
	case GUEST_SYS_umask:
		return umask((mode_t)(uint32_t)a);
	case GUEST_SYS_flock:
		/* (LOCK_SH, LOCK_EX, LOCK_NB and LOCK_UN are the same) */
		return result_of(flock((int)a, (int)b));

	case GUEST_SYS_clock_gettime:
	case GUEST_SYS_clock_getres:
	{
		struct timespec value;
		clockid_t clock;
		int result;

		if (host_linux_clock_to_darwin((int)a, &clock) != 0)
			return -LINUX_EINVAL;
		result = number == GUEST_SYS_clock_gettime ? clock_gettime(clock, &value) : clock_getres(clock, &value);
		if (result != 0)
			return -host_linux_errno(errno);
		timespec_out((uint64_t)b, &value);
		return 0;
	}
	case GUEST_SYS_gettimeofday:
	{
		struct timespec value;
		struct guest_timespec *result = GUEST(struct guest_timespec *, a);

		clock_gettime(CLOCK_REALTIME, &value);
		if (result)
		{
			result->seconds = (int32_t)value.tv_sec;
			result->nanoseconds = (int32_t)(value.tv_nsec / 1000);
		}
		return 0;
	}
	case GUEST_SYS_nanosleep:
		return guest_clock_nanosleep(LINUX_CLOCK_MONOTONIC, 0, (uint64_t)a, (uint64_t)b);
	case GUEST_SYS_clock_nanosleep:
		/* (returns the error itself, not -1) */
		return guest_clock_nanosleep((int)a, (int)b, (uint64_t)c, (uint64_t)d);
	case GUEST_SYS_futex:
		return guest_futex((uint64_t)a, (int)b, (uint32_t)c, (uint64_t)d, (uint64_t)e, (uint32_t)f);
	case GUEST_SYS_ppoll:
		return guest_ppoll((uint64_t)a, (uint32_t)b, (uint64_t)c);

	case GUEST_SYS_mmap:
		return host_guest_mmap((uint64_t)(uint32_t)a, (uint64_t)(uint32_t)b, (int)c, (int)d, (int)e, f);
	case GUEST_SYS_munmap:
		return host_guest_munmap((uint64_t)(uint32_t)a, (uint64_t)(uint32_t)b);
	case GUEST_SYS_mprotect:
		return host_guest_mprotect((uint64_t)(uint32_t)a, (uint64_t)(uint32_t)b, (int)c);
	case GUEST_SYS_madvise:
		return host_guest_madvise((uint64_t)(uint32_t)a, (uint64_t)(uint32_t)b, (int)c);
	case GUEST_SYS_mremap:
	case GUEST_SYS_brk:
		/* musl then falls back to mmap and copying */
		return -LINUX_ENOMEM;
	case GUEST_SYS_mlock:
	case GUEST_SYS_munlock:
		return 0;

	case GUEST_SYS_exit:
	case GUEST_SYS_exit_group:
		host_exit((int)a);

	case GUEST_SYS_set_tid_address:
	case GUEST_SYS_gettid:
		return host_thread_id();
	case GUEST_SYS_getpid:
		return getpid();
	case GUEST_SYS_getppid:
		return getppid();
	case GUEST_SYS_getuid:
		return getuid();
	case GUEST_SYS_geteuid:
		return geteuid();
	case GUEST_SYS_getgid:
		return getgid();
	case GUEST_SYS_getegid:
		return getegid();
	case GUEST_SYS_sched_yield:
		sched_yield();
		return 0;
	case GUEST_SYS_sched_getaffinity:
	{
		/* the processors there are (musl counts them for
		sysconf(_SC_NPROCESSORS_ONLN)) */
		unsigned char *mask = GUEST(unsigned char *, c);
		uint32_t size = (uint32_t)b < 128 ? (uint32_t)b : 128;
		long processors = sysconf(_SC_NPROCESSORS_ONLN);
		long index;

		if (!mask || size < 8)
			return -LINUX_EINVAL;
		size &= ~7u;
		memset(mask, 0, size);
		for (index = 0; index < processors && index < (long)size * 8; index++)
			mask[index / 8] |= (unsigned char)(1u << (index % 8));
		return size;
	}
	case GUEST_SYS_getrandom:
		arc4random_buf(GUEST(void *, a), (size_t)(uint32_t)b);
		return (uint32_t)b;
	case GUEST_SYS_uname:
	{
		struct utsname name;

		if (uname(&name) != 0)
			return -host_linux_errno(errno);
		host_linux_fill_utsname(GUEST(void *, a), &name);
		return 0;
	}
	case GUEST_SYS_sysinfo:
		return guest_sysinfo((uint64_t)a);
	case GUEST_SYS_prlimit64:
		return guest_prlimit((int)a, (int)b, (uint64_t)c, (uint64_t)d);
	case GUEST_SYS_getrlimit:
	{
		uint64_t limit[2];
		uint32_t *result = GUEST(uint32_t *, b);
		long error = guest_prlimit(0, (int)a, 0, (uint64_t)(uintptr_t)limit);

		/* (the guest's 32-bit unsigned longs; ~0, infinity, saturates) */
		if (!error && result)
		{
			result[0] = limit[0] > 0xffffffffull ? 0xffffffffu : (uint32_t)limit[0];
			result[1] = limit[1] > 0xffffffffull ? 0xffffffffu : (uint32_t)limit[1];
		}
		return error;
	}

	case GUEST_SYS_kill:
	{
		int signal_number = host_linux_signal_to_darwin((int)b);

		if ((int)a == getpid() || (int)a == 0)
		{
			if ((int)b == 6)
				host_abort("SIGABRT");
		}
		if (signal_number < 0)
			return -LINUX_EINVAL;
		return result_of(kill((pid_t)(int)a, signal_number));
	}
	case GUEST_SYS_tkill:
		return guest_kill_thread((int)a, (int)b);
	case GUEST_SYS_tgkill:
		return guest_kill_thread((int)b, (int)c);
	case GUEST_SYS_rt_sigprocmask:
		/* the host owns the signal masks; a Linux signal set means nothing
		to Darwin. The old set reads as empty */
		if ((uint32_t)c && (uint32_t)d <= 128)
			memset(GUEST(void *, c), 0, (size_t)(uint32_t)d);
		return 0;
	case GUEST_SYS_rt_sigaction:
	case GUEST_SYS_sigaltstack:
		/* the host owns signal handling */
		return 0;
	case GUEST_SYS_ioctl:
		return -LINUX_ENOTTY;
	case GUEST_SYS_membarrier:
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
		return -LINUX_ENOSYS;

	default:
		report_unsupported(number);
		return -LINUX_ENOSYS;
	}
}
