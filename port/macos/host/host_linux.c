/*
HOST_LINUX.C

Translation between the guest's Linux system call interface and Darwin's
(host_linux.h): constant tables and structure conversions only.
*/

#include "host_linux.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

/* ---------- errno */

/* indexed by Darwin's value; values Linux shares (1 to 34) included so
the table reads as one list */
static const unsigned char linux_errno_values[] =
{
	[EPERM] = 1, [ENOENT] = 2, [ESRCH] = 3, [EINTR] = 4, [EIO] = 5, [ENXIO] = 6, [E2BIG] = 7,
	[ENOEXEC] = 8, [EBADF] = 9, [ECHILD] = 10, [EDEADLK] = 35, [ENOMEM] = 12, [EACCES] = 13,
	[EFAULT] = 14, [ENOTBLK] = 15, [EBUSY] = 16, [EEXIST] = 17, [EXDEV] = 18, [ENODEV] = 19,
	[ENOTDIR] = 20, [EISDIR] = 21, [EINVAL] = 22, [ENFILE] = 23, [EMFILE] = 24, [ENOTTY] = 25,
	[ETXTBSY] = 26, [EFBIG] = 27, [ENOSPC] = 28, [ESPIPE] = 29, [EROFS] = 30, [EMLINK] = 31,
	[EPIPE] = 32, [EDOM] = 33, [ERANGE] = 34,
	[EAGAIN] = 11, [EINPROGRESS] = 115, [EALREADY] = 114,
	[ENOTSOCK] = 88, [EDESTADDRREQ] = 89, [EMSGSIZE] = 90, [EPROTOTYPE] = 91, [ENOPROTOOPT] = 92,
	[EPROTONOSUPPORT] = 93, [ESOCKTNOSUPPORT] = 94, [ENOTSUP] = 95, [EPFNOSUPPORT] = 96,
	[EAFNOSUPPORT] = 97, [EADDRINUSE] = 98, [EADDRNOTAVAIL] = 99,
	[ENETDOWN] = 100, [ENETUNREACH] = 101, [ENETRESET] = 102, [ECONNABORTED] = 103,
	[ECONNRESET] = 104, [ENOBUFS] = 105, [EISCONN] = 106, [ENOTCONN] = 107, [ESHUTDOWN] = 108,
	[ETOOMANYREFS] = 109, [ETIMEDOUT] = 110, [ECONNREFUSED] = 111,
	[ELOOP] = 40, [ENAMETOOLONG] = 36,
	[EHOSTDOWN] = 112, [EHOSTUNREACH] = 113, [ENOTEMPTY] = 39,
	[EPROCLIM] = 11, [EUSERS] = 87, [EDQUOT] = 122,
	[ESTALE] = 116, [EREMOTE] = 66,
	[EBADRPC] = 5, [ERPCMISMATCH] = 5, [EPROGUNAVAIL] = 5, [EPROGMISMATCH] = 5, [EPROCUNAVAIL] = 5,
	[ENOLCK] = 37, [ENOSYS] = 38,
	[EFTYPE] = 22, [EAUTH] = 13, [ENEEDAUTH] = 13,
	[EPWROFF] = 5, [EDEVERR] = 5,
	[EOVERFLOW] = 75,
	[EBADEXEC] = 8, [EBADARCH] = 8, [ESHLIBVERS] = 80, [EBADMACHO] = 8,
	[ECANCELED] = 125, [EIDRM] = 43, [ENOMSG] = 42, [EILSEQ] = 84, [ENOATTR] = 61,
	[EBADMSG] = 74, [EMULTIHOP] = 72, [ENODATA] = 61, [ENOLINK] = 67, [ENOSR] = 63,
	[ENOSTR] = 60, [EPROTO] = 71, [ETIME] = 62, [EOPNOTSUPP] = 95,
	[ENOPOLICY] = 22, [ENOTRECOVERABLE] = 131, [EOWNERDEAD] = 130, [EQFULL] = 105,
};

int host_linux_errno(int darwin_errno)
{
	if (darwin_errno == 0)
		return 0;
	if (darwin_errno > 0 && (size_t)darwin_errno < sizeof(linux_errno_values) && linux_errno_values[darwin_errno])
		return linux_errno_values[darwin_errno];
	return LINUX_EINVAL;
}

/* ---------- open(2) and the *at calls */

struct flag_pair
{
	int linux_flag;
	int darwin_flag;
};

static const struct flag_pair open_flags[] =
{
	{ LINUX_O_CREAT, O_CREAT },
	{ LINUX_O_EXCL, O_EXCL },
	{ LINUX_O_NOCTTY, O_NOCTTY },
	{ LINUX_O_TRUNC, O_TRUNC },
	{ LINUX_O_APPEND, O_APPEND },
	{ LINUX_O_NONBLOCK, O_NONBLOCK },
	{ LINUX_O_DSYNC, O_DSYNC },
	{ LINUX_O_ASYNC, O_ASYNC },
	{ LINUX_O_DIRECTORY, O_DIRECTORY },
	{ LINUX_O_NOFOLLOW, O_NOFOLLOW },
	{ LINUX_O_CLOEXEC, O_CLOEXEC },
	/* (O_SYNC is O_DSYNC plus a bit of its own on Linux) */
	{ LINUX_O_SYNC & ~LINUX_O_DSYNC, O_SYNC },
};

int host_linux_open_flags_to_darwin(int linux_flags)
{
	int darwin_flags = linux_flags & LINUX_O_ACCMODE;
	size_t index;

	if (linux_flags & LINUX_O_TMPFILE)
		return -1;
	/* a path-only descriptor: Darwin has none, a read-only one serves */
	if (linux_flags & LINUX_O_PATH)
		darwin_flags = O_RDONLY;
	for (index = 0; index < sizeof(open_flags) / sizeof(open_flags[0]); index++)
	{
		if ((linux_flags & open_flags[index].linux_flag) == open_flags[index].linux_flag)
			darwin_flags |= open_flags[index].darwin_flag;
	}
	/* O_LARGEFILE (musl sets it on every open), O_DIRECT and O_NOATIME
	have no Darwin flag and are dropped */
	return darwin_flags;
}

int host_linux_open_flags_from_darwin(int darwin_flags)
{
	int linux_flags = darwin_flags & O_ACCMODE;
	size_t index;

	for (index = 0; index < sizeof(open_flags) / sizeof(open_flags[0]); index++)
	{
		if (darwin_flags & open_flags[index].darwin_flag)
			linux_flags |= open_flags[index].linux_flag;
	}
	if (darwin_flags & O_SYNC)
		linux_flags |= LINUX_O_SYNC;
	return linux_flags | LINUX_O_LARGEFILE;
}

int host_linux_at_fd(int64_t linux_fd)
{
	int fd = (int)linux_fd;

	return fd == LINUX_AT_FDCWD ? AT_FDCWD : fd;
}

int host_linux_at_flags_to_darwin(int linux_flags, int bit_0x200)
{
	int darwin_flags = 0;

	if (linux_flags & LINUX_AT_SYMLINK_NOFOLLOW)
		darwin_flags |= AT_SYMLINK_NOFOLLOW;
	if (linux_flags & 0x200)
		darwin_flags |= bit_0x200;
	if (linux_flags & LINUX_AT_SYMLINK_FOLLOW)
		darwin_flags |= AT_SYMLINK_FOLLOW;
	if (linux_flags & ~(LINUX_AT_SYMLINK_NOFOLLOW | 0x200 | LINUX_AT_SYMLINK_FOLLOW | LINUX_AT_NO_AUTOMOUNT |
		LINUX_AT_EMPTY_PATH))
	{
		return -1;
	}
	return darwin_flags;
}

int host_linux_whence_to_darwin(int linux_whence)
{
	switch (linux_whence)
	{
	case 0: return SEEK_SET;
	case 1: return SEEK_CUR;
	case 2: return SEEK_END;
	case 3: return SEEK_DATA;
	case 4: return SEEK_HOLE;
	default: return -1;
	}
}

long host_linux_utime_nanoseconds_to_darwin(long linux_nanoseconds)
{
	if (linux_nanoseconds == LINUX_UTIME_NOW)
		return UTIME_NOW;
	if (linux_nanoseconds == LINUX_UTIME_OMIT)
		return UTIME_OMIT;
	return linux_nanoseconds;
}

/* ---------- fcntl */

int host_linux_fcntl_command_to_darwin(int linux_command)
{
	switch (linux_command)
	{
	case LINUX_F_DUPFD: return F_DUPFD;
	case LINUX_F_GETFD: return F_GETFD;
	case LINUX_F_SETFD: return F_SETFD;
	case LINUX_F_GETFL: return F_GETFL;
	case LINUX_F_SETFL: return F_SETFL;
	case LINUX_F_GETLK: return F_GETLK;
	case LINUX_F_SETLK: return F_SETLK;
	case LINUX_F_SETLKW: return F_SETLKW;
	case LINUX_F_SETOWN: return F_SETOWN;
	case LINUX_F_GETOWN: return F_GETOWN;
	case LINUX_F_DUPFD_CLOEXEC: return F_DUPFD_CLOEXEC;
	default: return -1;
	}
}

int host_linux_flock_to_darwin(const struct guest_flock *guest, struct flock *darwin)
{
	int whence = host_linux_whence_to_darwin(guest->l_whence);

	memset(darwin, 0, sizeof(*darwin));
	switch (guest->l_type)
	{
	case LINUX_F_RDLCK: darwin->l_type = F_RDLCK; break;
	case LINUX_F_WRLCK: darwin->l_type = F_WRLCK; break;
	case LINUX_F_UNLCK: darwin->l_type = F_UNLCK; break;
	default: return -1;
	}
	if (whence < 0)
		return -1;
	darwin->l_whence = (short)whence;
	darwin->l_start = guest->l_start;
	darwin->l_len = guest->l_len;
	darwin->l_pid = guest->l_pid;
	return 0;
}

void host_linux_flock_from_darwin(const struct flock *darwin, struct guest_flock *guest)
{
	switch (darwin->l_type)
	{
	case F_RDLCK: guest->l_type = LINUX_F_RDLCK; break;
	case F_WRLCK: guest->l_type = LINUX_F_WRLCK; break;
	default: guest->l_type = LINUX_F_UNLCK; break;
	}
	/* (SEEK_SET, SEEK_CUR and SEEK_END, the only ones a lock has, are the
	same) */
	guest->l_whence = darwin->l_whence;
	guest->l_start = darwin->l_start;
	guest->l_len = darwin->l_len;
	guest->l_pid = darwin->l_pid;
}

/* ---------- clocks */

int host_linux_clock_to_darwin(int linux_clock, clockid_t *darwin_clock)
{
	switch (linux_clock)
	{
	case LINUX_CLOCK_REALTIME:
	case LINUX_CLOCK_REALTIME_COARSE:
		*darwin_clock = CLOCK_REALTIME;
		return 0;
	case LINUX_CLOCK_MONOTONIC:
	case LINUX_CLOCK_MONOTONIC_COARSE:
		*darwin_clock = CLOCK_UPTIME_RAW;
		return 0;
	case LINUX_CLOCK_MONOTONIC_RAW:
		*darwin_clock = CLOCK_MONOTONIC_RAW;
		return 0;
	case LINUX_CLOCK_BOOTTIME:
		/* (counts sleep, as Linux's boot time does) */
		*darwin_clock = CLOCK_MONOTONIC;
		return 0;
	case LINUX_CLOCK_PROCESS_CPUTIME_ID:
		*darwin_clock = CLOCK_PROCESS_CPUTIME_ID;
		return 0;
	case LINUX_CLOCK_THREAD_CPUTIME_ID:
		*darwin_clock = CLOCK_THREAD_CPUTIME_ID;
		return 0;
	default:
		return -1;
	}
}

/* ---------- memory */

int host_linux_prot_to_darwin(int linux_protection)
{
	if (linux_protection & (LINUX_PROT_GROWSDOWN | LINUX_PROT_GROWSUP))
		return -1;
	/* PROT_READ, PROT_WRITE and PROT_EXEC are 1, 2 and 4 on both (AArch64's
	PROT_BTI and PROT_MTE, 0x10 and 0x20, mean nothing here) */
	return linux_protection & (PROT_READ | PROT_WRITE | PROT_EXEC);
}

int host_linux_mmap_flags_to_darwin(int linux_flags, int *darwin_flags, int *no_replace)
{
	int flags;

	switch (linux_flags & LINUX_MAP_TYPE)
	{
	case LINUX_MAP_SHARED:
	case LINUX_MAP_SHARED_VALIDATE:
		flags = MAP_SHARED;
		break;
	case LINUX_MAP_PRIVATE:
		flags = MAP_PRIVATE;
		break;
	default:
		return -1;
	}
	if (linux_flags & LINUX_MAP_GROWSDOWN)
		return -1;
	if (linux_flags & LINUX_MAP_ANONYMOUS)
		flags |= MAP_ANON;
	if (linux_flags & LINUX_MAP_NORESERVE)
		flags |= MAP_NORESERVE;
	*no_replace = 0;
	if (linux_flags & LINUX_MAP_FIXED_NOREPLACE)
	{
		flags |= MAP_FIXED;
		*no_replace = 1;
	}
	if (linux_flags & LINUX_MAP_FIXED)
	{
		flags |= MAP_FIXED;
		*no_replace = 0;
	}
	/* the rest (MAP_POPULATE, MAP_STACK, MAP_LOCKED, MAP_32BIT and the
	like) are hints, or say what every guest mapping is anyway */
	*darwin_flags = flags;
	return 0;
}

int host_linux_madvise_to_darwin(int linux_advice)
{
	switch (linux_advice)
	{
	case LINUX_MADV_NORMAL: return MADV_NORMAL;
	case LINUX_MADV_RANDOM: return MADV_RANDOM;
	case LINUX_MADV_SEQUENTIAL: return MADV_SEQUENTIAL;
	case LINUX_MADV_WILLNEED: return MADV_WILLNEED;
	/* Darwin's MADV_DONTNEED is only a hint; Linux's drops the pages, which
	read as zeroes afterwards (musl relies on it) */
	case LINUX_MADV_DONTNEED: return HOST_LINUX_MADVISE_ZERO;
	case LINUX_MADV_FREE: return MADV_FREE;
	default:
		/* MADV_REMOVE (9) to MADV_COLLAPSE (25): hints and fork and dump
		policies */
		if (linux_advice >= 9 && linux_advice <= 25)
			return HOST_LINUX_MADVISE_IGNORE;
		return HOST_LINUX_MADVISE_INVALID;
	}
}

/* ---------- signals and resource limits */

int host_linux_signal_to_darwin(int linux_signal)
{
	static const signed char darwin_signals[] =
	{
		0, SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE, SIGKILL,
		SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, -1 /* SIGSTKFLT */, SIGCHLD, SIGCONT, SIGSTOP,
		SIGTSTP, SIGTTIN, SIGTTOU, SIGURG, SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF, SIGWINCH, SIGIO,
		-1 /* SIGPWR */, SIGSYS,
	};

	if (linux_signal < 0 || (size_t)linux_signal >= sizeof(darwin_signals))
		return -1;
	return darwin_signals[linux_signal];
}

int host_linux_rlimit_resource_to_darwin(int linux_resource)
{
	switch (linux_resource)
	{
	case 0: return RLIMIT_CPU;
	case 1: return RLIMIT_FSIZE;
	case 2: return RLIMIT_DATA;
	case 3: return RLIMIT_STACK;
	case 4: return RLIMIT_CORE;
	case 5: return RLIMIT_RSS;
	case 6: return RLIMIT_NPROC;
	case 7: return RLIMIT_NOFILE;
	case 8: return RLIMIT_MEMLOCK;
	case 9: return RLIMIT_AS;
	default: return -1;
	}
}

uint64_t host_linux_rlimit_value_from_darwin(uint64_t darwin_value)
{
	return darwin_value >= (uint64_t)RLIM_INFINITY ? LINUX_RLIM_INFINITY : darwin_value;
}

uint64_t host_linux_rlimit_value_to_darwin(uint64_t linux_value)
{
	return linux_value >= (uint64_t)RLIM_INFINITY ? (uint64_t)RLIM_INFINITY : linux_value;
}

/* POLLIN, POLLPRI, POLLOUT, POLLERR, POLLHUP, POLLNVAL, POLLRDNORM and
POLLRDBAND are the same; POLLWRNORM and POLLWRBAND differ */
#define LINUX_POLL_SHARED 0xff
#define LINUX_POLLWRNORM 0x100
#define LINUX_POLLWRBAND 0x200

short host_linux_poll_events_to_darwin(short linux_events)
{
	short darwin_events = linux_events & LINUX_POLL_SHARED;

	if (linux_events & LINUX_POLLWRNORM)
		darwin_events |= POLLWRNORM;
	if (linux_events & LINUX_POLLWRBAND)
		darwin_events |= POLLWRBAND;
	return darwin_events;
}

short host_linux_poll_events_from_darwin(short darwin_events)
{
	short linux_events = darwin_events & LINUX_POLL_SHARED;

	/* (Darwin's POLLWRNORM is POLLOUT itself) */
	if (darwin_events & POLLOUT)
		linux_events |= LINUX_POLLWRNORM;
	if (darwin_events & POLLWRBAND)
		linux_events |= LINUX_POLLWRBAND;
	return linux_events;
}

/* ---------- structures */

void host_linux_fill_kstat(struct guest_kstat *guest, const struct stat *darwin)
{
	memset(guest, 0, sizeof(*guest));
	guest->st_dev = (uint32_t)darwin->st_dev;
	guest->st_ino = darwin->st_ino;
	guest->st_mode = darwin->st_mode;
	guest->st_nlink = darwin->st_nlink;
	guest->st_uid = darwin->st_uid;
	guest->st_gid = darwin->st_gid;
	guest->st_rdev = (uint32_t)darwin->st_rdev;
	guest->st_size = darwin->st_size;
	guest->st_blksize = darwin->st_blksize;
	guest->st_blocks = darwin->st_blocks;
	guest->st_atime_sec = darwin->st_atimespec.tv_sec;
	guest->st_atime_nsec = darwin->st_atimespec.tv_nsec;
	guest->st_mtime_sec = darwin->st_mtimespec.tv_sec;
	guest->st_mtime_nsec = darwin->st_mtimespec.tv_nsec;
	guest->st_ctime_sec = darwin->st_ctimespec.tv_sec;
	guest->st_ctime_nsec = darwin->st_ctimespec.tv_nsec;
}

unsigned host_linux_dirent64_size(size_t name_length)
{
	return (unsigned)((LINUX_DIRENT64_NAME_OFFSET + name_length + 1 + 7) & ~(size_t)7);
}

unsigned host_linux_pack_dirent64(void *buffer, size_t capacity, uint64_t inode, int64_t offset,
	unsigned char type, const char *name, size_t name_length)
{
	unsigned size = host_linux_dirent64_size(name_length);
	unsigned char *record = buffer;
	uint16_t length = (uint16_t)size;

	if (size > capacity || size > 0xffff)
		return 0;
	memset(record, 0, size);
	memcpy(record, &inode, 8);
	memcpy(record + 8, &offset, 8);
	memcpy(record + 16, &length, 2);
	record[18] = type;
	memcpy(record + LINUX_DIRENT64_NAME_OFFSET, name, name_length);
	return size;
}

void host_linux_fill_utsname(void *guest, const struct utsname *darwin)
{
	const char *fields[6];
	char *out = guest;
	int index;

	fields[0] = darwin->sysname;
	fields[1] = darwin->nodename;
	fields[2] = darwin->release;
	fields[3] = darwin->version;
	fields[4] = darwin->machine;
	fields[5] = "";
	memset(out, 0, LINUX_UTSNAME_SIZE);
	for (index = 0; index < 6; index++)
		snprintf(out + index * LINUX_UTSNAME_LENGTH, LINUX_UTSNAME_LENGTH, "%s", fields[index]);
}

void host_linux_fill_sysinfo(void *guest, uint64_t uptime_seconds, uint64_t total_bytes, uint64_t free_bytes,
	unsigned processes)
{
	/* { u32 uptime; u32 loads[3]; u32 totalram, freeram, sharedram,
	bufferram, totalswap, freeswap; u16 procs, pad; u32 totalhigh,
	freehigh; u32 mem_unit; char reserved[256]; } */
	unsigned char *out = guest;
	uint32_t uptime = uptime_seconds > 0xffffffffu ? 0xffffffffu : (uint32_t)uptime_seconds;
	uint32_t total = (uint32_t)(total_bytes / 4096), available = (uint32_t)(free_bytes / 4096);
	uint16_t procs = processes > 0xffff ? 0xffff : (uint16_t)processes;
	uint32_t unit = 4096;

	memset(out, 0, LINUX_SYSINFO_SIZE);
	memcpy(out + 0, &uptime, 4);
	memcpy(out + 16, &total, 4);
	memcpy(out + 20, &available, 4);
	memcpy(out + 40, &procs, 2);
	memcpy(out + 52, &unit, 4);
}
