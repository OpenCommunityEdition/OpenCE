/*
HOST_SYSCALL_TEST.C

A native test of port/macos/host/host_syscall.c (tools/test_macos_vm.py):
its translation of Darwin's errno values into Linux's, and system calls
made with Linux's numbers and flags. Guest memory is a buffer here, so a
guest address is an offset into it.
*/

#include "host.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bits/syscall.h.in"

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* the host's functions host_syscall.c calls, for a guest memory that is a
buffer */
static unsigned char memory[1 << 20];
OpenceGuest *host_guest;

void *host_guest_pointer(uint32_t address)
{
	return address ? memory + address : NULL;
}

uint32_t host_guest_allocate(uint64_t size)
{
	(void)size;
	return 0;
}

void host_guest_free(uint32_t address, uint64_t size)
{
	(void)address;
	(void)size;
}

uint8_t *opence_guest_host_pointer(const OpenceGuest *guest, uint32_t address, uint64_t size)
{
	(void)guest;
	return address + size <= sizeof(memory) ? memory + address : NULL;
}

void host_logf(int priority, const char *format, ...)
{
	(void)priority;
	(void)format;
}

void host_fatal(const char *format, ...)
{
	(void)format;
	exit(3);
}

void host_exit(int code)
{
	exit(code);
}

long long host_syscall(long long number, long long a, long long b, long long c, long long d, long long e, long long f);

int main(int argc, char **argv)
{
	/* Linux's numbers for Darwin's errno values: those that differ, and some
	that do not */
	static const int pairs[][2] =
	{
		{ 0, 0 }, { EPERM, 1 }, { ENOENT, 2 }, { EINTR, 4 }, { EBADF, 9 }, { ENOMEM, 12 }, { EACCES, 13 },
		{ EEXIST, 17 }, { EINVAL, 22 }, { ERANGE, 34 }, { EAGAIN, 11 }, { EDEADLK, 35 }, { ENAMETOOLONG, 36 },
		{ ENOSYS, 38 }, { ENOTEMPTY, 39 }, { ELOOP, 40 }, { EOVERFLOW, 75 }, { ENOTSOCK, 88 },
		{ EOPNOTSUPP, 95 }, { EAFNOSUPPORT, 97 }, { EADDRINUSE, 98 }, { EADDRNOTAVAIL, 99 }, { ENETUNREACH, 101 },
		{ ECONNRESET, 104 }, { ENOBUFS, 105 }, { EISCONN, 106 }, { ENOTCONN, 107 }, { ETIMEDOUT, 110 },
		{ ECONNREFUSED, 111 }, { EHOSTUNREACH, 113 }, { EALREADY, 114 }, { EINPROGRESS, 115 }, { ECANCELED, 125 },
	};
	char *path = (char *)memory + 0x1000;
	char *text = (char *)memory + 0x2000;
	unsigned int index;
	long long fd;

	(void)argc;
	for (index = 0; index < sizeof(pairs) / sizeof(pairs[0]); index++)
	{
		if (host_linux_errno(pairs[index][0]) != pairs[index][1])
		{
			printf("failed: errno %d is %d to Linux, not %d\n", pairs[index][0], host_linux_errno(pairs[index][0]),
				pairs[index][1]);
			return 1;
		}
	}

	/* a call the host does not make is ENOSYS to the guest */
	CHECK(host_syscall(__NR_clone, 0, 0, 0, 0, 0, 0) == -38);
	CHECK(host_syscall(100000, 0, 0, 0, 0, 0, 0) == -38);
	CHECK(host_syscall(__NR_getpid, 0, 0, 0, 0, 0, 0) == getpid());
	/* one musl makes that the host does not, quietly */
	CHECK(host_syscall(__NR_sysinfo, 0, 0, 0, 0, 0, 0) == -38);

	/* openat with Linux's AT_FDCWD (-100) and its O_CREAT | O_EXCL | O_WRONLY
	(0100 | 0200 | 01): the file is made, and a second time EEXIST (17) */
	snprintf(path, 512, "%s/made.txt", argv[1]);
	fd = host_syscall(__NR_openat, -100, 0x1000, 0100 | 0200 | 01, 0644, 0, 0);
	CHECK(fd >= 0);
	memcpy(text, "linux", 5);
	CHECK(host_syscall(__NR_write, fd, 0x2000, 5, 0, 0, 0) == 5);
	CHECK(host_syscall(__NR_close, fd, 0, 0, 0, 0, 0) == 0);
	CHECK(host_syscall(__NR_openat, -100, 0x1000, 0100 | 0200 | 01, 0644, 0, 0) == -17);
	/* O_RDONLY | O_APPEND (02000): F_GETFL gives Linux's flags back */
	fd = host_syscall(__NR_openat, -100, 0x1000, 02000, 0, 0, 0);
	CHECK(fd >= 0);
	CHECK(host_syscall(__NR_fcntl, fd, F_GETFL, 0, 0, 0, 0) == 02000);
	CHECK(host_syscall(__NR_close, fd, 0, 0, 0, 0, 0) == 0);
	/* unlinkat, and then ENOENT (2) */
	CHECK(host_syscall(__NR_unlinkat, -100, 0x1000, 0, 0, 0, 0) == 0);
	CHECK(host_syscall(__NR_openat, -100, 0x1000, 0, 0, 0, 0) == -2);
	printf("ok\n");
	return 0;
}
