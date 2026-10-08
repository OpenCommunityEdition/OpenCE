/*
HOST_LINUX.H

The Linux system call interface the macOS guest speaks, and its translation
to Darwin's.

The guest's C library (musl, with the guest arch in
port/macos/guest/libc/arch/x32) makes Linux system calls with AArch64
Linux's numbers, flag values and structure layouts, the same as the Android
guest's: no guest call ever reaches a kernel, so these are a private
protocol between guest and host (host_syscall.c), not the x86 Linux ABI.
Darwin numbers almost every constant differently (errno values, open and
*at flags, clock ids, mmap flags, fcntl commands, signals, resource limits)
and lays out stat, dirent and utsname differently, so every call is
translated here.

This file is the pure part: constants, tables and structure conversions,
with no system calls, so that it can be tested natively on any Mac
(the translation tables' tests run without Rosetta).
*/

#ifndef __HALO_MACOS_HOST_LINUX_H
#define __HALO_MACOS_HOST_LINUX_H

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>

/* ---------- errno */

/* Linux's numbers for the errors the host returns itself */
#define LINUX_EPERM 1
#define LINUX_ENOENT 2
#define LINUX_ESRCH 3
#define LINUX_EINTR 4
#define LINUX_EIO 5
#define LINUX_EBADF 9
#define LINUX_EAGAIN 11
#define LINUX_ENOMEM 12
#define LINUX_EACCES 13
#define LINUX_EFAULT 14
#define LINUX_EEXIST 17
#define LINUX_ENOTDIR 20
#define LINUX_EISDIR 21
#define LINUX_EINVAL 22
#define LINUX_ENOTTY 25
#define LINUX_ERANGE 34
#define LINUX_ENAMETOOLONG 36
#define LINUX_ENOSYS 38
#define LINUX_EOPNOTSUPP 95
#define LINUX_ETIMEDOUT 110

/* the Linux errno for a Darwin errno value (EINVAL for one Linux has no
equivalent of) */
int host_linux_errno(int darwin_errno);

/* ---------- open(2) and the *at calls */

#define LINUX_O_ACCMODE 03
#define LINUX_O_CREAT 0100
#define LINUX_O_EXCL 0200
#define LINUX_O_NOCTTY 0400
#define LINUX_O_TRUNC 01000
#define LINUX_O_APPEND 02000
#define LINUX_O_NONBLOCK 04000
#define LINUX_O_DSYNC 010000
#define LINUX_O_ASYNC 020000
#define LINUX_O_DIRECTORY 040000
#define LINUX_O_NOFOLLOW 0100000
#define LINUX_O_DIRECT 0200000
#define LINUX_O_LARGEFILE 0400000
#define LINUX_O_NOATIME 01000000
#define LINUX_O_CLOEXEC 02000000
#define LINUX_O_SYNC 04010000
#define LINUX_O_PATH 010000000
#define LINUX_O_TMPFILE 020000000 /* (with O_DIRECTORY) */

/* Darwin open flags for Linux ones; -1 for a request Darwin cannot honour
(O_TMPFILE) */
int host_linux_open_flags_to_darwin(int linux_flags);
/* Linux open flags for Darwin ones (fcntl F_GETFL) */
int host_linux_open_flags_from_darwin(int darwin_flags);

#define LINUX_AT_FDCWD (-100)
#define LINUX_AT_SYMLINK_NOFOLLOW 0x100
#define LINUX_AT_REMOVEDIR 0x200
#define LINUX_AT_EACCESS 0x200
#define LINUX_AT_SYMLINK_FOLLOW 0x400
#define LINUX_AT_NO_AUTOMOUNT 0x800
#define LINUX_AT_EMPTY_PATH 0x1000

/* a directory descriptor argument (Linux's AT_FDCWD is -100, Darwin's -2;
the guest passes it zero-extended, so it is narrowed here first) */
int host_linux_at_fd(int64_t linux_fd);
/* Darwin *at flags for Linux ones. Bit 0x200 is AT_REMOVEDIR to unlinkat
and AT_EACCESS to faccessat, so the caller says which Darwin flag it
becomes (bit_0x200); AT_EMPTY_PATH is the caller's to handle and is
dropped. Returns -1 for flags Darwin has no equivalent of. */
int host_linux_at_flags_to_darwin(int linux_flags, int bit_0x200);

#define LINUX_RENAME_NOREPLACE 1
#define LINUX_RENAME_EXCHANGE 2

/* lseek's whence (SEEK_DATA and SEEK_HOLE are swapped); -1 if unknown */
int host_linux_whence_to_darwin(int linux_whence);

/* utimensat's special nanosecond values */
#define LINUX_UTIME_NOW ((1l << 30) - 1l)
#define LINUX_UTIME_OMIT ((1l << 30) - 2l)
long host_linux_utime_nanoseconds_to_darwin(long linux_nanoseconds);

/* ---------- fcntl */

#define LINUX_F_DUPFD 0
#define LINUX_F_GETFD 1
#define LINUX_F_SETFD 2
#define LINUX_F_GETFL 3
#define LINUX_F_SETFL 4
#define LINUX_F_GETLK 5
#define LINUX_F_SETLK 6
#define LINUX_F_SETLKW 7
#define LINUX_F_SETOWN 8
#define LINUX_F_GETOWN 9
#define LINUX_F_DUPFD_CLOEXEC 1030

#define LINUX_F_RDLCK 0
#define LINUX_F_WRLCK 1
#define LINUX_F_UNLCK 2

/* the Darwin fcntl command for a Linux one; -1 if unsupported */
int host_linux_fcntl_command_to_darwin(int linux_command);

/* the guest's struct flock (musl's, 64-bit off_t, x32 alignment) */
struct guest_flock
{
	int16_t l_type;
	int16_t l_whence;
	int32_t pad;
	int64_t l_start;
	int64_t l_len;
	int32_t l_pid;
	int32_t pad2;
};

/* returns 0, or -1 for a lock type or whence Darwin does not have */
int host_linux_flock_to_darwin(const struct guest_flock *guest, struct flock *darwin);
void host_linux_flock_from_darwin(const struct flock *darwin, struct guest_flock *guest);

/* ---------- clocks and time */

#define LINUX_CLOCK_REALTIME 0
#define LINUX_CLOCK_MONOTONIC 1
#define LINUX_CLOCK_PROCESS_CPUTIME_ID 2
#define LINUX_CLOCK_THREAD_CPUTIME_ID 3
#define LINUX_CLOCK_MONOTONIC_RAW 4
#define LINUX_CLOCK_REALTIME_COARSE 5
#define LINUX_CLOCK_MONOTONIC_COARSE 6
#define LINUX_CLOCK_BOOTTIME 7

#define LINUX_TIMER_ABSTIME 1

/* the Darwin clock for a Linux clock id; returns 0, or -1 if unknown.
Linux's CLOCK_MONOTONIC stops while the computer sleeps, as Darwin's
CLOCK_UPTIME_RAW does (Darwin's CLOCK_MONOTONIC does not) */
int host_linux_clock_to_darwin(int linux_clock, clockid_t *darwin_clock);

/* the guest's timespec and timeval (32-bit time_t, as in the MSVC runtime) */
struct guest_timespec
{
	int32_t seconds;
	int32_t nanoseconds;
};

/* ---------- memory */

#define LINUX_PROT_GROWSDOWN 0x01000000
#define LINUX_PROT_GROWSUP 0x02000000

#define LINUX_MAP_SHARED 0x01
#define LINUX_MAP_PRIVATE 0x02
#define LINUX_MAP_SHARED_VALIDATE 0x03
#define LINUX_MAP_TYPE 0x0f
#define LINUX_MAP_FIXED 0x10
#define LINUX_MAP_ANONYMOUS 0x20
#define LINUX_MAP_GROWSDOWN 0x100
#define LINUX_MAP_NORESERVE 0x4000
#define LINUX_MAP_FIXED_NOREPLACE 0x100000

/* Darwin protection for a Linux one; -1 if unsupported */
int host_linux_prot_to_darwin(int linux_protection);
/* Darwin mmap flags for Linux ones: MAP_FIXED_NOREPLACE becomes MAP_FIXED
with *no_replace set (the host checks its own map of the address space);
returns 0, or -1 if the request is invalid or unsupported */
int host_linux_mmap_flags_to_darwin(int linux_flags, int *darwin_flags, int *no_replace);

#define LINUX_MADV_NORMAL 0
#define LINUX_MADV_RANDOM 1
#define LINUX_MADV_SEQUENTIAL 2
#define LINUX_MADV_WILLNEED 3
#define LINUX_MADV_DONTNEED 4
#define LINUX_MADV_FREE 8

/* what madvise(advice) becomes: a Darwin advice value (>= 0), or one of
these */
#define HOST_LINUX_MADVISE_ZERO (-2) /* Linux MADV_DONTNEED: the pages read as zeroes afterwards */
#define HOST_LINUX_MADVISE_IGNORE (-3) /* a hint Darwin has no equivalent of: succeed */
#define HOST_LINUX_MADVISE_INVALID (-1)
int host_linux_madvise_to_darwin(int linux_advice);

/* ---------- signals and resource limits */

/* the Darwin signal for a Linux one (0 stays 0); -1 if Darwin has none */
int host_linux_signal_to_darwin(int linux_signal);

#define LINUX_RLIM_INFINITY (~0ull)
/* the Darwin resource for a Linux RLIMIT_ value; -1 if unknown */
int host_linux_rlimit_resource_to_darwin(int linux_resource);
uint64_t host_linux_rlimit_value_from_darwin(uint64_t darwin_value);
uint64_t host_linux_rlimit_value_to_darwin(uint64_t linux_value);

/* poll's event bits */
short host_linux_poll_events_to_darwin(short linux_events);
short host_linux_poll_events_from_darwin(short darwin_events);

/* ---------- futex */

#define LINUX_FUTEX_WAIT 0
#define LINUX_FUTEX_WAKE 1
#define LINUX_FUTEX_REQUEUE 3
#define LINUX_FUTEX_CMP_REQUEUE 4
#define LINUX_FUTEX_WAIT_BITSET 9
#define LINUX_FUTEX_WAKE_BITSET 10
#define LINUX_FUTEX_PRIVATE_FLAG 128
#define LINUX_FUTEX_CLOCK_REALTIME 256
#define LINUX_FUTEX_COMMAND_MASK (~(LINUX_FUTEX_PRIVATE_FLAG | LINUX_FUTEX_CLOCK_REALTIME))

/* ---------- structures */

/* the AArch64 kernel's struct stat, which the guest's musl reads
(port/macos/guest/libc/arch/x32/kstat.h) */
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

/* file type bits (S_IFMT) and permissions are the same on both */
void host_linux_fill_kstat(struct guest_kstat *guest, const struct stat *darwin);

/* Linux's struct linux_dirent64, which getdents64 fills a buffer with:
{ u64 d_ino; s64 d_off; u16 d_reclen; u8 d_type; char d_name[]; },
each record 8-byte aligned. DT_ values are the same on both. */
#define LINUX_DIRENT64_NAME_OFFSET 19
/* the record size for a name of that length */
unsigned host_linux_dirent64_size(size_t name_length);
/* writes one record at buffer if it fits in capacity bytes; returns its
size, or 0 if it does not fit */
unsigned host_linux_pack_dirent64(void *buffer, size_t capacity, uint64_t inode, int64_t offset,
	unsigned char type, const char *name, size_t name_length);

/* Linux's struct utsname: six fields of 65 bytes */
#define LINUX_UTSNAME_LENGTH 65
#define LINUX_UTSNAME_SIZE (6 * LINUX_UTSNAME_LENGTH)
void host_linux_fill_utsname(void *guest, const struct utsname *darwin);

/* musl's struct sysinfo with the guest's 32-bit longs; memory is counted
in pages (mem_unit 4096) so that it fits */
#define LINUX_SYSINFO_SIZE 312
void host_linux_fill_sysinfo(void *guest, uint64_t uptime_seconds, uint64_t total_bytes, uint64_t free_bytes,
	unsigned processes);

#endif
