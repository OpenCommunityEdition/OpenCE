"""Tests for the macOS host's Linux system call layer
(port/macos/host/host_linux.c and host_syscall.c).

The macOS guest's C library speaks Linux (AArch64 Linux's numbers, flags
and structures) and the host performs each call on Darwin, whose errno
values and flag constants differ. Two C programs check it, built natively
(no Rosetta needed) and run:

- the translation tables, against the Linux and Darwin numbers written out
  as literals;
- the calls themselves, made as the guest makes them (files, directories
  through getdents64, clocks and absolute sleeps, futexes on both of
  Darwin's wait primitives, process queries), with host addresses passed
  whole and the memory calls, which need the low 4 GB, left out.

Only on a Mac: the layer is written with Darwin's headers.
"""
from pathlib import Path
import re
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
HOST = ROOT / "port/macos/host"
GUEST_SYSCALLS = ROOT / "port/macos/guest/libc/arch/x32/bits/syscall.h.in"
FLAGS = ["-std=gnu11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-fsanitize=undefined",
         "-fsanitize-trap=undefined", "-O1"]

TABLE_TESTS = r"""/* Checks port/macos/host/host_linux.c's tables against the Linux (AArch64
generic ABI) and Darwin numbers, written out as literals so that a wrong
Darwin header or a wrong table entry both show. Built natively; no Rosetta. */

#include "host_linux.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int failures, checks;

#define CHECK_EQ(actual, expected) do { \
	long long a_ = (long long)(actual), e_ = (long long)(expected); \
	checks++; \
	if (a_ != e_) { failures++; printf("FAIL %s:%d: %s = %lld, expected %lld\n", __FILE__, __LINE__, #actual, a_, e_); } \
} while (0)

static void test_errno(void)
{
	int value;

	/* 1-34 are the same, except that Darwin's 11 is EDEADLK */
	for (value = 1; value <= 34; value++)
	{
		if (value != 11)
			CHECK_EQ(host_linux_errno(value), value);
	}
	CHECK_EQ(host_linux_errno(0), 0);
	CHECK_EQ(host_linux_errno(11), 35);  /* EDEADLK */
	CHECK_EQ(host_linux_errno(35), 11);  /* EAGAIN */
	CHECK_EQ(host_linux_errno(63), 36);  /* ENAMETOOLONG */
	CHECK_EQ(host_linux_errno(77), 37);  /* ENOLCK */
	CHECK_EQ(host_linux_errno(78), 38);  /* ENOSYS */
	CHECK_EQ(host_linux_errno(66), 39);  /* ENOTEMPTY */
	CHECK_EQ(host_linux_errno(62), 40);  /* ELOOP */
	CHECK_EQ(host_linux_errno(84), 75);  /* EOVERFLOW */
	CHECK_EQ(host_linux_errno(38), 88);  /* ENOTSOCK */
	CHECK_EQ(host_linux_errno(102), 95); /* EOPNOTSUPP */
	CHECK_EQ(host_linux_errno(45), 95);  /* ENOTSUP */
	CHECK_EQ(host_linux_errno(48), 98);  /* EADDRINUSE */
	CHECK_EQ(host_linux_errno(49), 99);  /* EADDRNOTAVAIL */
	CHECK_EQ(host_linux_errno(54), 104); /* ECONNRESET */
	CHECK_EQ(host_linux_errno(60), 110); /* ETIMEDOUT */
	CHECK_EQ(host_linux_errno(61), 111); /* ECONNREFUSED */
	CHECK_EQ(host_linux_errno(37), 114); /* EALREADY */
	CHECK_EQ(host_linux_errno(36), 115); /* EINPROGRESS */
	CHECK_EQ(host_linux_errno(57), 107); /* ENOTCONN */
	CHECK_EQ(host_linux_errno(65), 113); /* EHOSTUNREACH */
	CHECK_EQ(host_linux_errno(89), 125); /* ECANCELED */
	CHECK_EQ(host_linux_errno(92), 84);  /* EILSEQ */
	CHECK_EQ(host_linux_errno(105), 130); /* EOWNERDEAD */
	CHECK_EQ(host_linux_errno(9999), 22); /* unknown: EINVAL */
	CHECK_EQ(host_linux_errno(-1), 22);
	/* every Darwin errno up to ELAST has a translation */
	for (value = 1; value <= 106; value++)
	{
		checks++;
		if (host_linux_errno(value) <= 0 || host_linux_errno(value) > 133)
		{
			failures++;
			printf("FAIL errno %d has no translation\n", value);
		}
	}
}

static void test_open_flags(void)
{
	CHECK_EQ(host_linux_open_flags_to_darwin(0), 0);
	CHECK_EQ(host_linux_open_flags_to_darwin(1), 1);
	CHECK_EQ(host_linux_open_flags_to_darwin(2), 2);
	CHECK_EQ(host_linux_open_flags_to_darwin(0x40), 0x200);      /* O_CREAT */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x80), 0x800);      /* O_EXCL */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x200), 0x400);     /* O_TRUNC */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x400), 0x8);       /* O_APPEND */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x800), 0x4);       /* O_NONBLOCK */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x4000), 0x100000); /* O_DIRECTORY */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x8000), 0x100);    /* O_NOFOLLOW */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x80000), 0x1000000); /* O_CLOEXEC */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x100), 0x20000);   /* O_NOCTTY */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x1000), 0x400000); /* O_DSYNC */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x101000), 0x400080); /* O_SYNC: O_DSYNC | O_SYNC */
	/* musl's fopen("w"): O_WRONLY|O_CREAT|O_TRUNC|O_LARGEFILE (dropped) */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x1 | 0x40 | 0x200 | 0x20000), 0x601);
	/* opendir: O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_LARGEFILE */
	CHECK_EQ(host_linux_open_flags_to_darwin(0x4000 | 0x80000 | 0x20000), 0x1100000);
	/* O_TMPFILE cannot be honoured */
	CHECK_EQ(host_linux_open_flags_to_darwin(020040000 | 2), -1);
	/* O_PATH: read-only */
	CHECK_EQ(host_linux_open_flags_to_darwin(010000000 | 0x80000), 0x1000000);
	/* F_GETFL back to Linux */
	CHECK_EQ(host_linux_open_flags_from_darwin(0x2 | 0x8 | 0x4), 0x2 | 0x400 | 0x800 | 0x20000);
	CHECK_EQ(host_linux_open_flags_from_darwin(0x601), 0x1 | 0x40 | 0x200 | 0x20000);
}

static void test_at(void)
{
	CHECK_EQ(host_linux_at_fd(-100), -2);
	/* AT_FDCWD as the guest passes it: zero-extended */
	CHECK_EQ(host_linux_at_fd(0xffffff9cll), -2);
	CHECK_EQ(host_linux_at_fd(5), 5);
	CHECK_EQ(host_linux_at_flags_to_darwin(0x100, 0x80), 0x20);  /* AT_SYMLINK_NOFOLLOW */
	CHECK_EQ(host_linux_at_flags_to_darwin(0x200, 0x80), 0x80);  /* AT_REMOVEDIR (unlinkat) */
	CHECK_EQ(host_linux_at_flags_to_darwin(0x200, 0x10), 0x10);  /* AT_EACCESS (faccessat) */
	CHECK_EQ(host_linux_at_flags_to_darwin(0x400, 0), 0x40);     /* AT_SYMLINK_FOLLOW */
	CHECK_EQ(host_linux_at_flags_to_darwin(0x1000, 0), 0);       /* AT_EMPTY_PATH: the caller's */
	CHECK_EQ(host_linux_at_flags_to_darwin(0x2, 0), -1);
	/* SEEK_DATA 3 and SEEK_HOLE 4 are swapped on Darwin */
	CHECK_EQ(host_linux_whence_to_darwin(0), 0);
	CHECK_EQ(host_linux_whence_to_darwin(1), 1);
	CHECK_EQ(host_linux_whence_to_darwin(2), 2);
	CHECK_EQ(host_linux_whence_to_darwin(3), 4);
	CHECK_EQ(host_linux_whence_to_darwin(4), 3);
	CHECK_EQ(host_linux_whence_to_darwin(5), -1);
	CHECK_EQ(host_linux_utime_nanoseconds_to_darwin(0x3fffffff), -1); /* UTIME_NOW */
	CHECK_EQ(host_linux_utime_nanoseconds_to_darwin(0x3ffffffe), -2); /* UTIME_OMIT */
	CHECK_EQ(host_linux_utime_nanoseconds_to_darwin(500), 500);
}

static void test_clocks(void)
{
	clockid_t clock = (clockid_t)-1;

	CHECK_EQ(host_linux_clock_to_darwin(0, &clock), 0);
	CHECK_EQ(clock, 0);  /* CLOCK_REALTIME */
	CHECK_EQ(host_linux_clock_to_darwin(1, &clock), 0);
	CHECK_EQ(clock, 8);  /* CLOCK_MONOTONIC: CLOCK_UPTIME_RAW */
	CHECK_EQ(host_linux_clock_to_darwin(2, &clock), 0);
	CHECK_EQ(clock, 12); /* CLOCK_PROCESS_CPUTIME_ID */
	CHECK_EQ(host_linux_clock_to_darwin(3, &clock), 0);
	CHECK_EQ(clock, 16); /* CLOCK_THREAD_CPUTIME_ID */
	CHECK_EQ(host_linux_clock_to_darwin(4, &clock), 0);
	CHECK_EQ(clock, 4);  /* CLOCK_MONOTONIC_RAW */
	CHECK_EQ(host_linux_clock_to_darwin(7, &clock), 0);
	CHECK_EQ(clock, 6);  /* CLOCK_BOOTTIME: Darwin's CLOCK_MONOTONIC */
	CHECK_EQ(host_linux_clock_to_darwin(11, &clock), -1);
	CHECK_EQ(host_linux_clock_to_darwin(-6, &clock), -1); /* a CPU clock of another thread */
	CHECK_EQ(sizeof(struct guest_timespec), 8);
}

static void test_fcntl(void)
{
	struct guest_flock guest = { 0 }, back = { 0 };
	struct flock darwin;

	CHECK_EQ(host_linux_fcntl_command_to_darwin(0), 0);
	CHECK_EQ(host_linux_fcntl_command_to_darwin(1), 1);
	CHECK_EQ(host_linux_fcntl_command_to_darwin(2), 2);
	CHECK_EQ(host_linux_fcntl_command_to_darwin(3), 3);
	CHECK_EQ(host_linux_fcntl_command_to_darwin(4), 4);
	CHECK_EQ(host_linux_fcntl_command_to_darwin(5), 7);    /* F_GETLK */
	CHECK_EQ(host_linux_fcntl_command_to_darwin(6), 8);    /* F_SETLK */
	CHECK_EQ(host_linux_fcntl_command_to_darwin(7), 9);    /* F_SETLKW */
	CHECK_EQ(host_linux_fcntl_command_to_darwin(8), 6);    /* F_SETOWN */
	CHECK_EQ(host_linux_fcntl_command_to_darwin(9), 5);    /* F_GETOWN */
	CHECK_EQ(host_linux_fcntl_command_to_darwin(1030), 67); /* F_DUPFD_CLOEXEC */
	CHECK_EQ(host_linux_fcntl_command_to_darwin(1031), -1);
	CHECK_EQ(sizeof(struct guest_flock), 32);
	CHECK_EQ(offsetof(struct guest_flock, l_start), 8);
	CHECK_EQ(offsetof(struct guest_flock, l_len), 16);
	CHECK_EQ(offsetof(struct guest_flock, l_pid), 24);
	guest.l_type = 1; /* F_WRLCK */
	guest.l_whence = 0;
	guest.l_start = 100;
	guest.l_len = 0x123456789ll;
	CHECK_EQ(host_linux_flock_to_darwin(&guest, &darwin), 0);
	CHECK_EQ(darwin.l_type, 3); /* Darwin F_WRLCK */
	CHECK_EQ(darwin.l_whence, 0);
	CHECK_EQ(darwin.l_start, 100);
	CHECK_EQ(darwin.l_len, 0x123456789ll);
	guest.l_type = 0; /* F_RDLCK */
	host_linux_flock_to_darwin(&guest, &darwin);
	CHECK_EQ(darwin.l_type, 1);
	guest.l_type = 2; /* F_UNLCK */
	host_linux_flock_to_darwin(&guest, &darwin);
	CHECK_EQ(darwin.l_type, 2);
	guest.l_type = 7;
	CHECK_EQ(host_linux_flock_to_darwin(&guest, &darwin), -1);
	darwin.l_type = 3;
	darwin.l_pid = 42;
	darwin.l_len = 0x123456789ll;
	host_linux_flock_from_darwin(&darwin, &back);
	CHECK_EQ(back.l_type, 1);
	CHECK_EQ(back.l_pid, 42);
	CHECK_EQ(back.l_len, 0x123456789ll);
}

static void test_memory(void)
{
	int flags = 0, no_replace = -1;

	CHECK_EQ(host_linux_prot_to_darwin(0), 0);
	CHECK_EQ(host_linux_prot_to_darwin(7), 7);
	CHECK_EQ(host_linux_prot_to_darwin(3 | 0x10), 3); /* PROT_BTI dropped */
	CHECK_EQ(host_linux_prot_to_darwin(0x01000000 | 3), -1);
	/* MAP_PRIVATE|MAP_ANONYMOUS */
	CHECK_EQ(host_linux_mmap_flags_to_darwin(0x22, &flags, &no_replace), 0);
	CHECK_EQ(flags, 0x1002);
	CHECK_EQ(no_replace, 0);
	/* the Xbox window's reservation: MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE|MAP_FIXED_NOREPLACE */
	CHECK_EQ(host_linux_mmap_flags_to_darwin(0x2 | 0x20 | 0x4000 | 0x100000, &flags, &no_replace), 0);
	CHECK_EQ(flags, 0x2 | 0x1000 | 0x40 | 0x10);
	CHECK_EQ(no_replace, 1);
	/* MAP_FIXED wins over MAP_FIXED_NOREPLACE */
	CHECK_EQ(host_linux_mmap_flags_to_darwin(0x2 | 0x10 | 0x100000, &flags, &no_replace), 0);
	CHECK_EQ(no_replace, 0);
	/* MAP_SHARED of a file */
	CHECK_EQ(host_linux_mmap_flags_to_darwin(0x1, &flags, &no_replace), 0);
	CHECK_EQ(flags, 0x1);
	/* MAP_POPULATE and MAP_STACK are dropped */
	CHECK_EQ(host_linux_mmap_flags_to_darwin(0x22 | 0x8000 | 0x20000, &flags, &no_replace), 0);
	CHECK_EQ(flags, 0x1002);
	CHECK_EQ(host_linux_mmap_flags_to_darwin(0x20, &flags, &no_replace), -1); /* no type */
	CHECK_EQ(host_linux_mmap_flags_to_darwin(0x122, &flags, &no_replace), -1); /* MAP_GROWSDOWN */
	CHECK_EQ(host_linux_madvise_to_darwin(0), 0);
	CHECK_EQ(host_linux_madvise_to_darwin(3), 3);
	CHECK_EQ(host_linux_madvise_to_darwin(4), HOST_LINUX_MADVISE_ZERO);
	CHECK_EQ(host_linux_madvise_to_darwin(8), 5); /* MADV_FREE */
	CHECK_EQ(host_linux_madvise_to_darwin(14), HOST_LINUX_MADVISE_IGNORE); /* MADV_HUGEPAGE */
	CHECK_EQ(host_linux_madvise_to_darwin(99), HOST_LINUX_MADVISE_INVALID);
}

static void test_signals_and_limits(void)
{
	CHECK_EQ(host_linux_signal_to_darwin(0), 0);
	CHECK_EQ(host_linux_signal_to_darwin(6), 6);   /* SIGABRT */
	CHECK_EQ(host_linux_signal_to_darwin(7), 10);  /* SIGBUS */
	CHECK_EQ(host_linux_signal_to_darwin(9), 9);   /* SIGKILL */
	CHECK_EQ(host_linux_signal_to_darwin(10), 30); /* SIGUSR1 */
	CHECK_EQ(host_linux_signal_to_darwin(11), 11); /* SIGSEGV */
	CHECK_EQ(host_linux_signal_to_darwin(12), 31); /* SIGUSR2 */
	CHECK_EQ(host_linux_signal_to_darwin(16), -1); /* SIGSTKFLT */
	CHECK_EQ(host_linux_signal_to_darwin(17), 20); /* SIGCHLD */
	CHECK_EQ(host_linux_signal_to_darwin(18), 19); /* SIGCONT */
	CHECK_EQ(host_linux_signal_to_darwin(19), 17); /* SIGSTOP */
	CHECK_EQ(host_linux_signal_to_darwin(20), 18); /* SIGTSTP */
	CHECK_EQ(host_linux_signal_to_darwin(23), 16); /* SIGURG */
	CHECK_EQ(host_linux_signal_to_darwin(29), 23); /* SIGIO */
	CHECK_EQ(host_linux_signal_to_darwin(31), 12); /* SIGSYS */
	CHECK_EQ(host_linux_signal_to_darwin(32), -1);
	CHECK_EQ(host_linux_rlimit_resource_to_darwin(7), 8); /* RLIMIT_NOFILE */
	CHECK_EQ(host_linux_rlimit_resource_to_darwin(9), 5); /* RLIMIT_AS */
	CHECK_EQ(host_linux_rlimit_resource_to_darwin(3), 3); /* RLIMIT_STACK */
	CHECK_EQ(host_linux_rlimit_resource_to_darwin(6), 7); /* RLIMIT_NPROC */
	CHECK_EQ(host_linux_rlimit_resource_to_darwin(8), 6); /* RLIMIT_MEMLOCK */
	CHECK_EQ(host_linux_rlimit_resource_to_darwin(13), -1);
	CHECK_EQ(host_linux_rlimit_value_from_darwin(0x7fffffffffffffffull), -1); /* RLIM_INFINITY: ~0 */
	CHECK_EQ(host_linux_rlimit_value_from_darwin(256), 256);
	CHECK_EQ(host_linux_rlimit_value_to_darwin(~0ull), 0x7fffffffffffffffll);
	CHECK_EQ(host_linux_rlimit_value_to_darwin(10240), 10240);
	CHECK_EQ(host_linux_poll_events_to_darwin(0x1 | 0x4), 0x5);
	CHECK_EQ(host_linux_poll_events_to_darwin(0x100), 0x4);   /* POLLWRNORM */
	CHECK_EQ(host_linux_poll_events_to_darwin(0x200), 0x100); /* POLLWRBAND */
	CHECK_EQ(host_linux_poll_events_from_darwin(0x4 | 0x10), 0x4 | 0x10 | 0x100);
	CHECK_EQ(host_linux_poll_events_from_darwin(0x20), 0x20); /* POLLNVAL */
}

static void test_kstat(void)
{
	struct stat darwin;
	struct guest_kstat guest;

	CHECK_EQ(sizeof(struct guest_kstat), 128);
	CHECK_EQ(offsetof(struct guest_kstat, st_ino), 8);
	CHECK_EQ(offsetof(struct guest_kstat, st_mode), 16);
	CHECK_EQ(offsetof(struct guest_kstat, st_nlink), 20);
	CHECK_EQ(offsetof(struct guest_kstat, st_uid), 24);
	CHECK_EQ(offsetof(struct guest_kstat, st_rdev), 32);
	CHECK_EQ(offsetof(struct guest_kstat, st_size), 48);
	CHECK_EQ(offsetof(struct guest_kstat, st_blksize), 56);
	CHECK_EQ(offsetof(struct guest_kstat, st_blocks), 64);
	CHECK_EQ(offsetof(struct guest_kstat, st_atime_sec), 72);
	CHECK_EQ(offsetof(struct guest_kstat, st_mtime_sec), 88);
	CHECK_EQ(offsetof(struct guest_kstat, st_ctime_nsec), 112);
	memset(&darwin, 0, sizeof(darwin));
	darwin.st_dev = (dev_t)0x1000013;
	darwin.st_ino = 0x123456789abcull;
	darwin.st_mode = 0040755; /* S_IFDIR, the same value on Linux */
	darwin.st_nlink = 3;
	darwin.st_uid = 501;
	darwin.st_gid = 20;
	darwin.st_size = 0x100000001ll;
	darwin.st_blksize = 4096;
	darwin.st_blocks = 8;
	darwin.st_atimespec.tv_sec = 1700000000;
	darwin.st_atimespec.tv_nsec = 1;
	darwin.st_mtimespec.tv_sec = 1700000001;
	darwin.st_mtimespec.tv_nsec = 2;
	darwin.st_ctimespec.tv_sec = 1700000002;
	darwin.st_ctimespec.tv_nsec = 3;
	host_linux_fill_kstat(&guest, &darwin);
	CHECK_EQ(guest.st_dev, 0x1000013);
	CHECK_EQ(guest.st_ino, 0x123456789abcll);
	CHECK_EQ(guest.st_mode, 040755);
	CHECK_EQ(guest.st_nlink, 3);
	CHECK_EQ(guest.st_uid, 501);
	CHECK_EQ(guest.st_gid, 20);
	CHECK_EQ(guest.st_size, 0x100000001ll);
	CHECK_EQ(guest.st_blksize, 4096);
	CHECK_EQ(guest.st_blocks, 8);
	CHECK_EQ(guest.st_atime_sec, 1700000000);
	CHECK_EQ(guest.st_atime_nsec, 1);
	CHECK_EQ(guest.st_mtime_sec, 1700000001);
	CHECK_EQ(guest.st_mtime_nsec, 2);
	CHECK_EQ(guest.st_ctime_sec, 1700000002);
	CHECK_EQ(guest.st_ctime_nsec, 3);
	CHECK_EQ(guest.pad, 0);
}

static void test_dirent(void)
{
	unsigned char buffer[64];
	uint64_t inode;
	int64_t offset;
	uint16_t length;

	CHECK_EQ(host_linux_dirent64_size(0), 24);
	CHECK_EQ(host_linux_dirent64_size(4), 24); /* 19 + 4 + 1 rounded to 8 */
	CHECK_EQ(host_linux_dirent64_size(5), 32);
	CHECK_EQ(host_linux_dirent64_size(12), 32);
	CHECK_EQ(host_linux_dirent64_size(13), 40);
	memset(buffer, 0xee, sizeof(buffer));
	CHECK_EQ(host_linux_pack_dirent64(buffer, sizeof(buffer), 77, 3, 4 /* DT_DIR */, "maps", 4), 24);
	memcpy(&inode, buffer, 8);
	memcpy(&offset, buffer + 8, 8);
	memcpy(&length, buffer + 16, 2);
	CHECK_EQ(inode, 77);
	CHECK_EQ(offset, 3);
	CHECK_EQ(length, 24);
	CHECK_EQ(buffer[18], 4);
	CHECK_EQ(memcmp(buffer + 19, "maps", 5), 0);
	CHECK_EQ(buffer[23], 0);
	CHECK_EQ(buffer[24], 0xee); /* nothing written past the record */
	CHECK_EQ(host_linux_pack_dirent64(buffer, 23, 1, 1, 8, "maps", 4), 0); /* does not fit */
	CHECK_EQ(host_linux_pack_dirent64(buffer, 32, 1, 1, 8 /* DT_REG */, "ui.map", 6), 32);
}

static void test_utsname_and_sysinfo(void)
{
	struct utsname darwin;
	char guest[LINUX_UTSNAME_SIZE];
	unsigned char info[LINUX_SYSINFO_SIZE];
	uint32_t word;
	uint16_t half;

	memset(&darwin, 0, sizeof(darwin));
	strcpy(darwin.sysname, "Darwin");
	strcpy(darwin.nodename, "mac");
	strcpy(darwin.release, "25.5.0");
	memset(darwin.version, 'v', sizeof(darwin.version) - 1); /* longer than Linux's 65 */
	strcpy(darwin.machine, "x86_64");
	memset(guest, 0x55, sizeof(guest));
	host_linux_fill_utsname(guest, &darwin);
	CHECK_EQ(strcmp(guest, "Darwin"), 0);
	CHECK_EQ(strcmp(guest + 65, "mac"), 0);
	CHECK_EQ(strcmp(guest + 130, "25.5.0"), 0);
	CHECK_EQ(strlen(guest + 195), 64);
	CHECK_EQ(strcmp(guest + 260, "x86_64"), 0);
	CHECK_EQ(guest[325], 0);
	CHECK_EQ(guest[389], 0);

	host_linux_fill_sysinfo(info, 1234, 16ull << 30, 3ull << 30, 400);
	memcpy(&word, info, 4);
	CHECK_EQ(word, 1234);
	memcpy(&word, info + 16, 4);
	CHECK_EQ(word, (16ull << 30) / 4096); /* totalram, in pages */
	memcpy(&word, info + 20, 4);
	CHECK_EQ(word, (3ull << 30) / 4096);
	memcpy(&half, info + 40, 2);
	CHECK_EQ(half, 400);
	memcpy(&word, info + 52, 4);
	CHECK_EQ(word, 4096); /* mem_unit */
}

int main(void)
{
	test_errno();
	test_open_flags();
	test_at();
	test_clocks();
	test_fcntl();
	test_memory();
	test_signals_and_limits();
	test_kstat();
	test_dirent();
	test_utsname_and_sysinfo();
	printf("%d checks, %d failures\n", checks, failures);
	return failures != 0;
}
"""

CALL_TESTS = r"""/* Runs port/macos/host/host_syscall.c's calls natively against Darwin, as
the guest makes them (Linux numbers, flags and structures), and checks the
Linux results. Built for the machine's own architecture with GUEST() taking
whole host addresses; the memory calls are stubbed (they need the low 4 GB). */

#include "host_linux.h"
#include "guest_syscall_numbers.h"

#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

long long host_syscall(long long number, long long a, long long b, long long c, long long d, long long e,
	long long f);

/* ---------- what host_syscall.c calls in the rest of the host */

void host_logf(int priority, const char *format, ...)
{
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	vfprintf(stderr, format, arguments);
	va_end(arguments);
	fputc('\n', stderr);
}

void host_exit(int code) { _exit(code); }
void host_abort(const char *reason) { fprintf(stderr, "abort: %s\n", reason); _exit(134); }
void host_log_guest_output(int fd, const char *bytes, size_t size) { (void)fd; (void)bytes; (void)size; }
static __thread int test_thread_id;
static int last_thread_id;
int host_thread_id(void)
{
	if (!test_thread_id)
		test_thread_id = __sync_add_and_fetch(&last_thread_id, 1);
	return test_thread_id;
}
long host_guest_mmap(uint64_t a, uint64_t b, int c, int d, int e, int64_t f) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; return -LINUX_ENOSYS; }
long host_guest_munmap(uint64_t a, uint64_t b) { (void)a; (void)b; return -LINUX_ENOSYS; }
long host_guest_mprotect(uint64_t a, uint64_t b, int c) { (void)a; (void)b; (void)c; return -LINUX_ENOSYS; }
long host_guest_madvise(uint64_t a, uint64_t b, int c) { (void)a; (void)b; (void)c; return -LINUX_ENOSYS; }

/* ---------- checks */

static int failures, checks;

#define CHECK(condition) do { \
	checks++; \
	if (!(condition)) { failures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); } \
} while (0)
#define CHECK_EQ(actual, expected) do { \
	long long a_ = (long long)(actual), e_ = (long long)(expected); \
	checks++; \
	if (a_ != e_) { failures++; printf("FAIL %s:%d: %s = %lld, expected %lld\n", __FILE__, __LINE__, #actual, a_, e_); } \
} while (0)

#define P(x) ((long long)(uintptr_t)(x))
/* AT_FDCWD as the guest passes it: -100, zero-extended from 32 bits */
#define GUEST_AT_FDCWD 0xffffff9cll

static char directory[PATH_MAX];

static void test_files(void)
{
	char path[PATH_MAX + 32], buffer[256];
	struct guest_kstat information;
	long long fd, result;

	snprintf(path, sizeof(path), "%s/a.txt", directory);
	/* O_WRONLY|O_CREAT|O_TRUNC|O_LARGEFILE */
	fd = host_syscall(GUEST_SYS_openat, GUEST_AT_FDCWD, P(path), 0x1 | 0x40 | 0x200 | 0x20000, 0644, 0, 0);
	CHECK(fd >= 0);
	CHECK_EQ(host_syscall(GUEST_SYS_write, fd, P("hello"), 5, 0, 0, 0), 5);
	memset(&information, 0, sizeof(information));
	CHECK_EQ(host_syscall(GUEST_SYS_fstat, fd, P(&information), 0, 0, 0, 0), 0);
	CHECK_EQ(information.st_size, 5);
	CHECK_EQ(information.st_mode & 0170000, 0100000); /* S_IFREG */
	CHECK_EQ(host_syscall(GUEST_SYS_lseek, fd, 0, 2 /* SEEK_END */, 0, 0, 0), 5);
	CHECK_EQ(host_syscall(GUEST_SYS_close, fd, 0, 0, 0, 0, 0), 0);
	/* O_CREAT|O_EXCL on it: EEXIST (17 on both) */
	CHECK_EQ(host_syscall(GUEST_SYS_openat, GUEST_AT_FDCWD, P(path), 0x1 | 0x40 | 0x80, 0644, 0, 0), -17);
	/* O_RDWR|O_APPEND, read back with F_GETFL in Linux bits */
	fd = host_syscall(GUEST_SYS_openat, GUEST_AT_FDCWD, P(path), 0x2 | 0x400, 0, 0, 0);
	CHECK(fd >= 0);
	CHECK_EQ(host_syscall(GUEST_SYS_fcntl, fd, 3 /* F_GETFL */, 0, 0, 0, 0), 0x2 | 0x400 | 0x20000);
	CHECK_EQ(host_syscall(GUEST_SYS_fcntl, fd, 2 /* F_SETFD */, 1, 0, 0, 0), 0);
	CHECK_EQ(host_syscall(GUEST_SYS_fcntl, fd, 1 /* F_GETFD */, 0, 0, 0, 0), 1);
	CHECK_EQ(host_syscall(GUEST_SYS_pread64, fd, P(buffer), 3, 1, 0, 0), 3);
	CHECK_EQ(memcmp(buffer, "ell", 3), 0);
	CHECK_EQ(host_syscall(GUEST_SYS_close, fd, 0, 0, 0, 0, 0), 0);
	/* errors in Linux numbers */
	snprintf(path, sizeof(path), "%s/missing", directory);
	CHECK_EQ(host_syscall(GUEST_SYS_openat, GUEST_AT_FDCWD, P(path), 0, 0, 0, 0), -2);
	CHECK_EQ(host_syscall(GUEST_SYS_newfstatat, GUEST_AT_FDCWD, P(path), P(&information), 0, 0, 0), -2);
	CHECK_EQ(host_syscall(GUEST_SYS_mkdirat, GUEST_AT_FDCWD, P(directory), 0755, 0, 0, 0), -17);
	CHECK_EQ(host_syscall(GUEST_SYS_close, 9999, 0, 0, 0, 0, 0), -9);
	/* rmdir of a directory with files: ENOTEMPTY is 39 on Linux, 66 on Darwin */
	CHECK_EQ(host_syscall(GUEST_SYS_unlinkat, GUEST_AT_FDCWD, P(directory), 0x200 /* AT_REMOVEDIR */, 0, 0, 0), -39);
	/* stat through the directory */
	snprintf(path, sizeof(path), "%s/a.txt", directory);
	CHECK_EQ(host_syscall(GUEST_SYS_newfstatat, GUEST_AT_FDCWD, P(path), P(&information), 0x100, 0, 0), 0);
	CHECK_EQ(information.st_size, 5);
	/* renameat2 RENAME_NOREPLACE onto an existing file */
	{
		char other[PATH_MAX + 32];

		snprintf(other, sizeof(other), "%s/b.txt", directory);
		fd = host_syscall(GUEST_SYS_openat, GUEST_AT_FDCWD, P(other), 0x1 | 0x40, 0644, 0, 0);
		host_syscall(GUEST_SYS_close, fd, 0, 0, 0, 0, 0);
		CHECK_EQ(host_syscall(GUEST_SYS_renameat2, GUEST_AT_FDCWD, P(path), GUEST_AT_FDCWD, P(other), 1, 0), -17);
		CHECK_EQ(host_syscall(GUEST_SYS_unlinkat, GUEST_AT_FDCWD, P(other), 0, 0, 0, 0), 0);
	}
	/* utimensat: modification time set, access time UTIME_OMIT */
	{
		struct guest_timespec times[2] = { { 0, 0x3ffffffe }, { 1600000000, 0 } };

		CHECK_EQ(host_syscall(GUEST_SYS_utimensat, GUEST_AT_FDCWD, P(path), P(times), 0, 0, 0), 0);
		host_syscall(GUEST_SYS_newfstatat, GUEST_AT_FDCWD, P(path), P(&information), 0, 0, 0);
		CHECK_EQ(information.st_mtime_sec, 1600000000);
	}
	/* getcwd returns the length with its terminator */
	result = host_syscall(GUEST_SYS_getcwd, P(buffer), sizeof(buffer), 0, 0, 0, 0);
	CHECK_EQ(result, (long long)strlen(buffer) + 1);
	/* /proc/self/exe */
	result = host_syscall(GUEST_SYS_readlinkat, GUEST_AT_FDCWD, P("/proc/self/exe"), P(buffer), sizeof(buffer) - 1, 0, 0);
	CHECK(result > 0);
	if (result > 0)
	{
		buffer[result] = 0;
		CHECK(strstr(buffer, "test_host_syscall") != NULL);
	}
}

static void test_directories(void)
{
	char path[PATH_MAX + 32];
	unsigned char buffer[96];
	int index, found[40], dots = 0, total = 0, pass;
	long long fd;

	for (index = 0; index < 40; index++)
	{
		snprintf(path, sizeof(path), "%s/file%02d_with_a_longer_name", directory, index);
		fd = host_syscall(GUEST_SYS_openat, GUEST_AT_FDCWD, P(path), 0x1 | 0x40, 0644, 0, 0);
		host_syscall(GUEST_SYS_close, fd, 0, 0, 0, 0, 0);
	}
	/* opendir: O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_LARGEFILE */
	fd = host_syscall(GUEST_SYS_openat, GUEST_AT_FDCWD, P(directory), 0x4000 | 0x80000 | 0x20000, 0, 0, 0);
	CHECK(fd >= 0);
	for (pass = 0; pass < 2; pass++)
	{
		long long size;

		memset(found, 0, sizeof(found));
		dots = total = 0;
		/* a small buffer: two records at most per call */
		while ((size = host_syscall(GUEST_SYS_getdents64, fd, P(buffer), sizeof(buffer), 0, 0, 0)) > 0)
		{
			long long offset = 0;

			while (offset < size)
			{
				uint16_t length;
				const char *name = (const char *)buffer + offset + 19;

				memcpy(&length, buffer + offset + 16, 2);
				CHECK(length % 8 == 0 && length >= 24);
				if (!strcmp(name, ".") || !strcmp(name, ".."))
					dots++;
				else if (!strncmp(name, "file", 4))
					found[atoi(name + 4)]++;
				if (!strncmp(name, "file", 4))
					CHECK_EQ(buffer[offset + 18], 8); /* DT_REG */
				total++;
				offset += length;
			}
			CHECK_EQ(offset, size);
		}
		CHECK_EQ(size, 0);
		CHECK_EQ(dots, 2);
		for (index = 0; index < 40; index++)
			CHECK_EQ(found[index], 1);
		CHECK_EQ(total, 42 + 1); /* and a.txt */
		/* rewinddir */
		CHECK_EQ(host_syscall(GUEST_SYS_lseek, fd, 0, 0, 0, 0, 0), 0);
	}
	/* a buffer too small for one record */
	CHECK_EQ(host_syscall(GUEST_SYS_getdents64, fd, P(buffer), 10, 0, 0, 0), -22);
	CHECK_EQ(host_syscall(GUEST_SYS_close, fd, 0, 0, 0, 0, 0), 0);
	for (index = 0; index < 40; index++)
	{
		snprintf(path, sizeof(path), "%s/file%02d_with_a_longer_name", directory, index);
		host_syscall(GUEST_SYS_unlinkat, GUEST_AT_FDCWD, P(path), 0, 0, 0, 0);
	}
}

static int64_t monotonic_nanoseconds(void)
{
	struct guest_timespec now;

	host_syscall(GUEST_SYS_clock_gettime, 1 /* CLOCK_MONOTONIC */, P(&now), 0, 0, 0, 0);
	return (int64_t)now.seconds * 1000000000ll + now.nanoseconds;
}

static void test_time(void)
{
	struct guest_timespec value, target;
	int64_t start, elapsed;

	CHECK_EQ(host_syscall(GUEST_SYS_clock_gettime, 0, P(&value), 0, 0, 0, 0), 0);
	CHECK(value.seconds > 1700000000);
	CHECK_EQ(host_syscall(GUEST_SYS_clock_gettime, 42, P(&value), 0, 0, 0, 0), -22);
	CHECK_EQ(host_syscall(GUEST_SYS_clock_getres, 1, P(&value), 0, 0, 0, 0), 0);
	/* an absolute sleep, 30 ms ahead on the monotonic clock */
	start = monotonic_nanoseconds();
	target.seconds = (int32_t)((start + 30000000) / 1000000000);
	target.nanoseconds = (int32_t)((start + 30000000) % 1000000000);
	CHECK_EQ(host_syscall(GUEST_SYS_clock_nanosleep, 1, 1 /* TIMER_ABSTIME */, P(&target), 0, 0, 0), 0);
	elapsed = monotonic_nanoseconds() - start;
	CHECK(elapsed >= 30000000 && elapsed < 500000000);
	/* a relative one */
	value.seconds = 0;
	value.nanoseconds = 10000000;
	start = monotonic_nanoseconds();
	CHECK_EQ(host_syscall(GUEST_SYS_nanosleep, P(&value), 0, 0, 0, 0, 0), 0);
	CHECK(monotonic_nanoseconds() - start >= 10000000);
}

static volatile uint32_t futex_word;
static volatile int waiter_result = 1;

static void *futex_waiter(void *unused)
{
	(void)unused;
	waiter_result = (int)host_syscall(GUEST_SYS_futex, P(&futex_word), 0 | 128 /* FUTEX_WAIT_PRIVATE */, 0, 0, 0, 0);
	return NULL;
}

static void test_futex(void)
{
	struct guest_timespec timeout = { 0, 20000000 };
	pthread_t thread;
	int64_t start;
	int attempt;

	futex_word = 5;
	/* the value differs: EAGAIN (11) at once */
	CHECK_EQ(host_syscall(GUEST_SYS_futex, P(&futex_word), 128, 4, 0, 0, 0), -11);
	/* nobody wakes it: ETIMEDOUT (110) after the timeout */
	start = monotonic_nanoseconds();
	CHECK_EQ(host_syscall(GUEST_SYS_futex, P(&futex_word), 128, 5, P(&timeout), 0, 0), -110);
	CHECK(monotonic_nanoseconds() - start >= 15000000);
	/* a waiter woken by FUTEX_WAKE */
	futex_word = 0;
	pthread_create(&thread, NULL, futex_waiter, NULL);
	usleep(50000);
	futex_word = 1;
	for (attempt = 0; attempt < 100 && waiter_result == 1; attempt++)
	{
		host_syscall(GUEST_SYS_futex, P(&futex_word), 1 | 128 /* FUTEX_WAKE_PRIVATE */, 1, 0, 0, 0);
		usleep(10000);
	}
	pthread_join(thread, NULL);
	CHECK(waiter_result == 0 || waiter_result == -11);
	/* musl's condition variables requeue: a requeue wakes */
	futex_word = 0;
	waiter_result = 1;
	pthread_create(&thread, NULL, futex_waiter, NULL);
	usleep(50000);
	CHECK_EQ(host_syscall(GUEST_SYS_futex, P(&futex_word), 3 | 128 /* FUTEX_REQUEUE */, 0, 1, P(&timeout), 0), 0);
	pthread_join(thread, NULL);
	CHECK_EQ(waiter_result, 0);
}

static void test_process(void)
{
	char name[390];
	unsigned char info[LINUX_SYSINFO_SIZE];
	unsigned char mask[128];
	uint64_t limits[2];
	int32_t fds[2];
	uint32_t word;
	long long size;
	int bits = 0, index;

	CHECK_EQ(host_syscall(GUEST_SYS_uname, P(name), 0, 0, 0, 0, 0), 0);
	CHECK_EQ(strcmp(name, "Darwin"), 0);
	CHECK_EQ(host_syscall(GUEST_SYS_sysinfo, P(info), 0, 0, 0, 0, 0), 0);
	memcpy(&word, info + 16, 4);
	CHECK(word > 0);
	memcpy(&word, info + 52, 4);
	CHECK_EQ(word, 4096);
	size = host_syscall(GUEST_SYS_sched_getaffinity, 0, sizeof(mask), P(mask), 0, 0, 0);
	CHECK(size >= 8);
	for (index = 0; index < size; index++)
		bits += __builtin_popcount(mask[index]);
	CHECK_EQ(bits, sysconf(_SC_NPROCESSORS_ONLN));
	CHECK_EQ(host_syscall(GUEST_SYS_getrandom, P(mask), 64, 0, 0, 0, 0), 64);
	CHECK_EQ(host_syscall(GUEST_SYS_prlimit64, 0, 7 /* RLIMIT_NOFILE */, 0, P(limits), 0, 0), 0);
	CHECK(limits[0] > 0);
	CHECK_EQ(host_syscall(GUEST_SYS_prlimit64, 0, 15, 0, P(limits), 0, 0), -22);
	CHECK_EQ(host_syscall(GUEST_SYS_pipe2, P(fds), 0x80000 /* O_CLOEXEC */, 0, 0, 0, 0), 0);
	CHECK_EQ(host_syscall(GUEST_SYS_fcntl, fds[0], 1, 0, 0, 0, 0), 1);
	CHECK_EQ(host_syscall(GUEST_SYS_write, fds[1], P("x"), 1, 0, 0, 0), 1);
	{
		/* ppoll: the read end is readable (POLLIN), with a 0 timeout */
		struct { int fd; short events, revents; } poll_entry = { fds[0], 1, 0 };
		struct guest_timespec zero = { 0, 0 };

		CHECK_EQ(host_syscall(GUEST_SYS_ppoll, P(&poll_entry), 1, P(&zero), 0, 0, 0), 1);
		CHECK_EQ(poll_entry.revents & 1, 1);
	}
	host_syscall(GUEST_SYS_close, fds[0], 0, 0, 0, 0, 0);
	host_syscall(GUEST_SYS_close, fds[1], 0, 0, 0, 0, 0);
	CHECK(host_syscall(GUEST_SYS_gettid, 0, 0, 0, 0, 0, 0) > 0);
	CHECK_EQ(host_syscall(GUEST_SYS_gettid, 0, 0, 0, 0, 0, 0), host_syscall(GUEST_SYS_set_tid_address, 0, 0, 0, 0, 0, 0));
	CHECK_EQ(host_syscall(GUEST_SYS_getpid, 0, 0, 0, 0, 0, 0), getpid());
	CHECK_EQ(host_syscall(GUEST_SYS_ioctl, 1, 0x5413, 0, 0, 0, 0), -25);
	CHECK_EQ(host_syscall(GUEST_SYS_tkill, 999, 0, 0, 0, 0, 0), -3);
	CHECK_EQ(host_syscall(GUEST_SYS_clone, 0, 0, 0, 0, 0, 0), -38);
	CHECK_EQ(host_syscall(9999, 0, 0, 0, 0, 0, 0), -38);
}

int main(void)
{
	snprintf(directory, sizeof(directory), "%s/host_syscall_test_XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
	if (!mkdtemp(directory))
		return 2;
	test_files();
	test_directories();
	test_time();
	test_futex();
	test_process();
	{
		char path[PATH_MAX + 32];

		snprintf(path, sizeof(path), "%s/a.txt", directory);
		unlink(path);
		rmdir(directory);
	}
	printf("%d checks, %d failures\n", checks, failures);
	return failures != 0;
}
"""


def clang_or_skip() -> str:
    if sys.platform != "darwin":
        pytest.skip("the system call layer is built with Darwin's headers")
    clang = shutil.which("clang")
    if clang is None:
        pytest.skip("clang is needed")
    return clang


def build_and_run(clang: str, folder: Path, name: str, text: str, sources, flags) -> None:
    source = folder / f"{name}.c"
    source.write_text(text)
    program = folder / name
    result = subprocess.run([clang, *FLAGS, *flags, *map(str, sources), str(source), "-o", str(program)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(program)], capture_output=True, text=True, timeout=120)
    assert result.returncode == 0 and " 0 failures" in result.stdout, result.stdout + result.stderr


def test_translation_tables(tmp_path):
    clang = clang_or_skip()
    build_and_run(clang, tmp_path, "test_host_linux", TABLE_TESTS, [HOST / "host_linux.c"], [f"-I{HOST}"])


@pytest.mark.parametrize("futex", ["os_sync", "ulock"])
def test_system_calls(tmp_path, futex):
    clang = clang_or_skip()
    # the guest's call numbers, as tools/macos_build.py generates them
    numbers = re.findall(r"^#define __NR_(\w+)\s+(\d+)", GUEST_SYSCALLS.read_text(), re.M)
    (tmp_path / "guest_syscall_numbers.h").write_text(
        "".join(f"#define GUEST_SYS_{name} {number}\n" for name, number in numbers))
    flags = [f"-I{HOST}", f"-I{ROOT / 'port/android/include'}", f"-I{tmp_path}", "-DHALO_MACOS=1",
             "-DGUEST(type,value)=((type)(uintptr_t)(value))"]
    if futex == "ulock":
        # the __ulock calls macOS before 14.4 has instead
        flags.append("-D__builtin_available(...)=0")
    build_and_run(clang, tmp_path, "test_host_syscall", CALL_TESTS, [HOST / "host_linux.c", HOST / "host_syscall.c"],
                  flags)
