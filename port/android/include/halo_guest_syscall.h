/*
HALO_GUEST_SYSCALL.H

The guest's system call interface: the numbers, structures and constants
the guest's C library (port/android/guest/libc) hands to host_syscall
(port/android/host/host_syscall.c). They belong to the guest, not to the
kernel the host runs on. The host translates each call to its own kernel's
numbers, layouts and flags, so the same guest interface runs on an AArch64
host and an x86-64 host.

The interface is Linux's asm-generic one as AArch64 has it (the guest's
first architecture, arm64_32, was made from musl's aarch64), with these
differences:

- there are no legacy calls (open, stat, poll, ...): musl uses the *at and
  p* forms;
- number 62 is _llseek, as on 32-bit asm-generic architectures, not lseek:
  the guest's system calls return a 32-bit long, which cannot hold a 64-bit
  file offset, so the offset is returned through a pointer;
- timespec, iovec and sysinfo have the guest's 32-bit layouts.

The guest's libc headers hold the same values for musl
(port/android/guest/libc/arch/generic: bits/syscall.h.in, kstat.h,
bits/fcntl.h); guest_syscall.c checks at compile time that they agree with
this header, and the host checks the values it passes on unchanged
against its own headers (host_syscall.c).

This header is included by both halves.
*/

#ifndef __HALO_GUEST_SYSCALL_H
#define __HALO_GUEST_SYSCALL_H

#include <stddef.h>
#include <stdint.h>

/* ---------- numbers (the calls the host performs or answers) */

#define GUEST_SYS_getcwd 17
#define GUEST_SYS_epoll_pwait 22
#define GUEST_SYS_dup 23
#define GUEST_SYS_dup3 24
#define GUEST_SYS_fcntl 25
#define GUEST_SYS_ioctl 29
#define GUEST_SYS_flock 32
#define GUEST_SYS_mkdirat 34
#define GUEST_SYS_unlinkat 35
#define GUEST_SYS_renameat 38
#define GUEST_SYS_statfs 43
#define GUEST_SYS_fstatfs 44
#define GUEST_SYS_ftruncate 46
#define GUEST_SYS_faccessat 48
#define GUEST_SYS_chdir 49
#define GUEST_SYS_fchmod 52
#define GUEST_SYS_fchmodat 53
#define GUEST_SYS_openat 56
#define GUEST_SYS_close 57
#define GUEST_SYS_pipe2 59
#define GUEST_SYS_getdents64 61
#define GUEST_SYS__llseek 62
#define GUEST_SYS_read 63
#define GUEST_SYS_write 64
#define GUEST_SYS_readv 65
#define GUEST_SYS_writev 66
#define GUEST_SYS_pread64 67
#define GUEST_SYS_pwrite64 68
#define GUEST_SYS_preadv 69
#define GUEST_SYS_pwritev 70
#define GUEST_SYS_pselect6 72
#define GUEST_SYS_ppoll 73
#define GUEST_SYS_readlinkat 78
#define GUEST_SYS_newfstatat 79
#define GUEST_SYS_fstat 80
#define GUEST_SYS_fsync 82
#define GUEST_SYS_fdatasync 83
#define GUEST_SYS_utimensat 88
#define GUEST_SYS_exit 93
#define GUEST_SYS_exit_group 94
#define GUEST_SYS_waitid 95
#define GUEST_SYS_set_tid_address 96
#define GUEST_SYS_futex 98
#define GUEST_SYS_nanosleep 101
#define GUEST_SYS_getitimer 102
#define GUEST_SYS_setitimer 103
#define GUEST_SYS_timer_create 107
#define GUEST_SYS_timer_gettime 108
#define GUEST_SYS_timer_settime 110
#define GUEST_SYS_clock_gettime 113
#define GUEST_SYS_clock_getres 114
#define GUEST_SYS_clock_nanosleep 115
#define GUEST_SYS_sched_yield 124
#define GUEST_SYS_kill 129
#define GUEST_SYS_tkill 130
#define GUEST_SYS_tgkill 131
#define GUEST_SYS_sigaltstack 132
#define GUEST_SYS_rt_sigaction 134
#define GUEST_SYS_rt_sigprocmask 135
#define GUEST_SYS_rt_sigtimedwait 137
#define GUEST_SYS_times 153
#define GUEST_SYS_uname 160
#define GUEST_SYS_getrlimit 163
#define GUEST_SYS_getrusage 165
#define GUEST_SYS_umask 166
#define GUEST_SYS_gettimeofday 169
#define GUEST_SYS_getpid 172
#define GUEST_SYS_getppid 173
#define GUEST_SYS_getuid 174
#define GUEST_SYS_geteuid 175
#define GUEST_SYS_getgid 176
#define GUEST_SYS_getegid 177
#define GUEST_SYS_gettid 178
#define GUEST_SYS_sysinfo 179
#define GUEST_SYS_sendmsg 211
#define GUEST_SYS_recvmsg 212
#define GUEST_SYS_brk 214
#define GUEST_SYS_munmap 215
#define GUEST_SYS_mremap 216
#define GUEST_SYS_clone 220
#define GUEST_SYS_execve 221
#define GUEST_SYS_mmap 222
#define GUEST_SYS_mprotect 226
#define GUEST_SYS_madvise 233
#define GUEST_SYS_wait4 260
#define GUEST_SYS_prlimit64 261
#define GUEST_SYS_renameat2 276
#define GUEST_SYS_getrandom 278
#define GUEST_SYS_membarrier 283
#define GUEST_SYS_statx 291
#define GUEST_SYS_clone3 435

/* ---------- structures (fixed-width members; the guest's own layouts) */

struct halo_guest_timespec
{
	int32_t seconds;
	int32_t nanoseconds;
};

struct halo_guest_iovec
{
	uint32_t base;
	uint32_t length;
};

/* the most iovecs one readv or writev takes (IOV_MAX, as Linux's UIO_MAXIOV) */
#define GUEST_IOV_MAX 1024

/* fstat and newfstatat's result: the AArch64 kernel's struct stat, which
musl's kstat.h has for the guest */
struct halo_guest_kstat
{
	uint64_t st_dev;
	uint64_t st_ino;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
	uint64_t st_rdev;
	uint64_t pad1;
	int64_t st_size;
	int32_t st_blksize;
	int32_t pad2;
	int64_t st_blocks;
	/* (not st_atime_sec and the like: bionic defines those names as macros
	for its own struct stat, which would rewrite these fields) */
	int64_t atime_seconds;
	int64_t atime_nanoseconds;
	int64_t mtime_seconds;
	int64_t mtime_nanoseconds;
	int64_t ctime_seconds;
	int64_t ctime_nanoseconds;
	uint32_t unused[2];
};

/* fcntl's F_GETLK, F_SETLK, F_SETLKW and the F_OFD_ forms */
struct halo_guest_flock
{
	int16_t l_type;
	int16_t l_whence;
	int32_t pad;
	int64_t l_start;
	int64_t l_len;
	int32_t l_pid;
	int32_t pad2;
};

#define HALO_GUEST_CHECK_LAYOUT(name, condition) typedef char halo_guest_layout_##name[(condition) ? 1 : -1]
/* sysinfo's result, musl's struct sysinfo with 32-bit longs */
struct halo_guest_sysinfo
{
	uint32_t uptime;
	uint32_t loads[3];
	uint32_t totalram;
	uint32_t freeram;
	uint32_t sharedram;
	uint32_t bufferram;
	uint32_t totalswap;
	uint32_t freeswap;
	uint16_t procs;
	uint16_t pad;
	uint32_t totalhigh;
	uint32_t freehigh;
	uint32_t mem_unit;
	char reserved[256];
};

HALO_GUEST_CHECK_LAYOUT(timespec_size, sizeof(struct halo_guest_timespec) == 8);
HALO_GUEST_CHECK_LAYOUT(iovec_size, sizeof(struct halo_guest_iovec) == 8);
HALO_GUEST_CHECK_LAYOUT(kstat_size, sizeof(struct halo_guest_kstat) == 128);
HALO_GUEST_CHECK_LAYOUT(kstat_mode, offsetof(struct halo_guest_kstat, st_mode) == 16);
HALO_GUEST_CHECK_LAYOUT(kstat_rdev, offsetof(struct halo_guest_kstat, st_rdev) == 32);
HALO_GUEST_CHECK_LAYOUT(kstat_size_field, offsetof(struct halo_guest_kstat, st_size) == 48);
HALO_GUEST_CHECK_LAYOUT(kstat_blksize, offsetof(struct halo_guest_kstat, st_blksize) == 56);
HALO_GUEST_CHECK_LAYOUT(kstat_blocks, offsetof(struct halo_guest_kstat, st_blocks) == 64);
HALO_GUEST_CHECK_LAYOUT(kstat_atime, offsetof(struct halo_guest_kstat, atime_seconds) == 72);
HALO_GUEST_CHECK_LAYOUT(kstat_ctime_nsec, offsetof(struct halo_guest_kstat, ctime_nanoseconds) == 112);
HALO_GUEST_CHECK_LAYOUT(flock_size, sizeof(struct halo_guest_flock) == 32);
HALO_GUEST_CHECK_LAYOUT(flock_start, offsetof(struct halo_guest_flock, l_start) == 8);
HALO_GUEST_CHECK_LAYOUT(flock_pid, offsetof(struct halo_guest_flock, l_pid) == 24);
HALO_GUEST_CHECK_LAYOUT(sysinfo_size, sizeof(struct halo_guest_sysinfo) == 312);
HALO_GUEST_CHECK_LAYOUT(sysinfo_totalram, offsetof(struct halo_guest_sysinfo, totalram) == 16);
HALO_GUEST_CHECK_LAYOUT(sysinfo_freeswap, offsetof(struct halo_guest_sysinfo, freeswap) == 36);
HALO_GUEST_CHECK_LAYOUT(sysinfo_procs, offsetof(struct halo_guest_sysinfo, procs) == 40);
HALO_GUEST_CHECK_LAYOUT(sysinfo_totalhigh, offsetof(struct halo_guest_sysinfo, totalhigh) == 44);
HALO_GUEST_CHECK_LAYOUT(sysinfo_mem_unit, offsetof(struct halo_guest_sysinfo, mem_unit) == 52);
HALO_GUEST_CHECK_LAYOUT(sysinfo_reserved, offsetof(struct halo_guest_sysinfo, reserved) == 56);

/* ---------- open's flags (openat, fcntl's F_GETFL and F_SETFL, pipe2,
dup3), which differ between architectures; the host translates them, and
refuses a bit outside this set with EINVAL */

#define GUEST_O_ACCMODE 03
#define GUEST_O_RDONLY 00
#define GUEST_O_WRONLY 01
#define GUEST_O_RDWR 02
#define GUEST_O_CREAT 0100
#define GUEST_O_EXCL 0200
#define GUEST_O_NOCTTY 0400
#define GUEST_O_TRUNC 01000
#define GUEST_O_APPEND 02000
#define GUEST_O_NONBLOCK 04000
#define GUEST_O_DSYNC 010000
#define GUEST_O_ASYNC 020000
#define GUEST_O_DIRECTORY 040000
#define GUEST_O_NOFOLLOW 0100000
#define GUEST_O_DIRECT 0200000
#define GUEST_O_LARGEFILE 0400000
#define GUEST_O_NOATIME 01000000
#define GUEST_O_CLOEXEC 02000000
/* O_SYNC is this bit with O_DSYNC, and O_TMPFILE this bit with O_DIRECTORY */
#define GUEST_O_SYNC_BIT 04000000
#define GUEST_O_PATH 010000000
#define GUEST_O_TMPFILE_BIT 020000000
#define GUEST_O_SYNC (GUEST_O_SYNC_BIT | GUEST_O_DSYNC)
#define GUEST_O_TMPFILE (GUEST_O_TMPFILE_BIT | GUEST_O_DIRECTORY)

/* ---------- fcntl's commands (others are refused with EINVAL) */

#define GUEST_F_DUPFD 0
#define GUEST_F_GETFD 1
#define GUEST_F_SETFD 2
#define GUEST_F_GETFL 3
#define GUEST_F_SETFL 4
#define GUEST_F_GETLK 5
#define GUEST_F_SETLK 6
#define GUEST_F_SETLKW 7
#define GUEST_F_SETOWN 8
#define GUEST_F_GETOWN 9
#define GUEST_F_SETOWN_EX 15
#define GUEST_F_GETOWN_EX 16
#define GUEST_F_OFD_GETLK 36
#define GUEST_F_OFD_SETLK 37
#define GUEST_F_OFD_SETLKW 38
#define GUEST_F_DUPFD_CLOEXEC 1030
#define GUEST_F_SETPIPE_SZ 1031
#define GUEST_F_GETPIPE_SZ 1032
#define GUEST_F_ADD_SEALS 1033
#define GUEST_F_GET_SEALS 1034
#define GUEST_FD_CLOEXEC 1

/* ---------- values the host passes on unchanged

These are the same on every architecture the host is built for (AArch64
and x86-64 use the asm-generic values); host_syscall.c stops the build on
a host whose headers disagree. */

#define GUEST_AT_FDCWD (-100)
#define GUEST_AT_SYMLINK_NOFOLLOW 0x100
#define GUEST_AT_REMOVEDIR 0x200
#define GUEST_AT_EMPTY_PATH 0x1000

#define GUEST_SEEK_SET 0
#define GUEST_SEEK_CUR 1
#define GUEST_SEEK_END 2

#define GUEST_PROT_READ 1
#define GUEST_PROT_WRITE 2
#define GUEST_PROT_EXEC 4
#define GUEST_MAP_SHARED 0x01
#define GUEST_MAP_PRIVATE 0x02
#define GUEST_MAP_FIXED 0x10
#define GUEST_MAP_ANONYMOUS 0x20
#define GUEST_MAP_NORESERVE 0x4000
#define GUEST_MAP_FIXED_NOREPLACE 0x100000
#define GUEST_MADV_DONTNEED 4
#define GUEST_MADV_FREE 8

#define GUEST_POLLIN 0x001
#define GUEST_POLLPRI 0x002
#define GUEST_POLLOUT 0x004
#define GUEST_POLLERR 0x008
#define GUEST_POLLHUP 0x010
#define GUEST_POLLNVAL 0x020

#define GUEST_SIG_BLOCK 0
#define GUEST_SIG_UNBLOCK 1
#define GUEST_SIG_SETMASK 2
#define GUEST_SIGABRT 6
#define GUEST_SIGKILL 9
#define GUEST_SIGSEGV 11
#define GUEST_SIGTERM 15

#define GUEST_RLIMIT_CPU 0
#define GUEST_RLIMIT_FSIZE 1
#define GUEST_RLIMIT_DATA 2
#define GUEST_RLIMIT_STACK 3
#define GUEST_RLIMIT_CORE 4
#define GUEST_RLIMIT_NOFILE 7
#define GUEST_RLIMIT_AS 9

#define GUEST_TIMER_ABSTIME 1

#define GUEST_LOCK_SH 1
#define GUEST_LOCK_EX 2
#define GUEST_LOCK_NB 4
#define GUEST_LOCK_UN 8
#define GUEST_F_RDLCK 0
#define GUEST_F_WRLCK 1
#define GUEST_F_UNLCK 2

/* errno values, which the host returns as they are (-errno) */
#define GUEST_EPERM 1
#define GUEST_ENOENT 2
#define GUEST_EINTR 4
#define GUEST_EBADF 9
#define GUEST_EAGAIN 11
#define GUEST_ENOMEM 12
#define GUEST_EACCES 13
#define GUEST_EFAULT 14
#define GUEST_EEXIST 17
#define GUEST_ENOTDIR 20
#define GUEST_EISDIR 21
#define GUEST_EINVAL 22
#define GUEST_ENOSPC 28
#define GUEST_ERANGE 34
#define GUEST_EDEADLK 35
#define GUEST_ENAMETOOLONG 36
#define GUEST_ENOSYS 38
#define GUEST_ENOTEMPTY 39
#define GUEST_ELOOP 40
#define GUEST_EOVERFLOW 75
#define GUEST_EOPNOTSUPP 95
#define GUEST_ETIMEDOUT 110

#endif
