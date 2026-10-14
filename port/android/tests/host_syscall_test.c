/*
HOST_SYSCALL_TEST.C

Tests of the translation of the guest's system calls
(port/android/host/host_syscall.c) on the build machine, whatever its
architecture: the numbers, open's flags, the kernel stat layout, fcntl's
locks, _llseek and vectored I/O. Built and run by
tools/test_host_syscall.py; exits nonzero on a failure.

The calls are made as the guest makes them, through host_syscall, with the
guest's numbers and structures in memory below 4 GB (the guest's pointers
are 32-bit).
*/

#include "../host/host_syscall.c"

#include <dirent.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>

static int failures;

#define CHECK(condition) \
	do \
	{ \
		if (!(condition)) \
		{ \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
			failures++; \
		} \
	} while (0)

/* ---------- the host functions host_syscall.c calls */

static int warnings;

void host_logf(int priority, const char *format, ...)
{
	(void)priority;
	(void)format;
	warnings++;
}

void host_exit(int code)
{
	_exit(code);
}

long host_guest_mmap(uint64_t address, uint64_t size, int protection, int flags, int fd, int64_t offset)
{
	void *result = mmap((void *)(uintptr_t)address, size, protection, flags, fd, offset);

	return result == MAP_FAILED ? -errno : (long)(uintptr_t)result;
}

long host_guest_munmap(uint64_t address, uint64_t size)
{
	return munmap((void *)(uintptr_t)address, size) ? -errno : 0;
}

long host_guest_mprotect(uint64_t address, uint64_t size, int protection)
{
	return mprotect((void *)(uintptr_t)address, size, protection) ? -errno : 0;
}

long host_guest_madvise(uint64_t address, uint64_t size, int advice)
{
	return madvise((void *)(uintptr_t)address, size, advice) ? -errno : 0;
}

int host_low_owns(uintptr_t address, size_t size)
{
	(void)address;
	(void)size;
	return 1;
}

/* ---------- guest memory */

#define LOW_SIZE (1 << 20)

static uint8_t *low;
static uint32_t low_used;

/* memory with a 32-bit address, as the guest's */
static int low_initialize(void)
{
	uint64_t address;

	for (address = 0x40000000u; address < 0xf0000000u; address += 0x10000000u)
	{
		void *result = mmap((void *)(uintptr_t)address, LOW_SIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);

		if (result == (void *)(uintptr_t)address)
		{
			low = result;
			return 1;
		}
		if (result != MAP_FAILED)
			munmap(result, LOW_SIZE);
	}
	return 0;
}

static void *low_alloc(size_t size)
{
	void *result = low + low_used;

	low_used += (uint32_t)((size + 15) & ~(size_t)15);
	if (low_used > LOW_SIZE)
	{
		printf("FAIL: out of low memory\n");
		exit(1);
	}
	memset(result, 0, size);
	return result;
}

static uint32_t guest_address(const void *pointer)
{
	return (uint32_t)(uintptr_t)pointer;
}

static char *low_string(const char *text)
{
	char *result = low_alloc(strlen(text) + 1);

	strcpy(result, text);
	return result;
}

static long long call(long long number, long long a, long long b, long long c, long long d, long long e)
{
	return host_syscall(number, a, b, c, d, e, 0);
}

/* a 32-bit int as musl passes it (unsigned long, zero-extended) */
static long long guest_int(int value)
{
	return (long long)(uint32_t)value;
}

static char directory[256];

static char *path_in(const char *name)
{
	char buffer[512];

	snprintf(buffer, sizeof(buffer), "%s/%s", directory, name);
	return low_string(buffer);
}

/* ---------- tests */

static void test_numbers(void)
{
	int before = warnings;

	CHECK(passthrough_number(GUEST_SYS_read) == SYS_read);
	CHECK(passthrough_number(GUEST_SYS_close) == SYS_close);
	CHECK(passthrough_number(GUEST_SYS_getdents64) == SYS_getdents64);
	CHECK(passthrough_number(GUEST_SYS_prlimit64) == SYS_prlimit64);
	CHECK(passthrough_number(GUEST_SYS_gettid) == SYS_gettid);
	CHECK(passthrough_number(GUEST_SYS_uname) == SYS_uname);
	/* translated calls are never passed through */
	CHECK(passthrough_number(GUEST_SYS_openat) == -1);
	CHECK(passthrough_number(GUEST_SYS_fstat) == -1);
	CHECK(passthrough_number(GUEST_SYS_newfstatat) == -1);
	CHECK(passthrough_number(GUEST_SYS_fcntl) == -1);
	CHECK(passthrough_number(GUEST_SYS__llseek) == -1);
	/* the guest's numbers are the guest's, not the host's: the host's
	getpid is not the guest's 172 (on x86-64, 172 is iopl) */
	CHECK(call(GUEST_SYS_getpid, 0, 0, 0, 0, 0) == getpid());
	CHECK(call(GUEST_SYS_gettid, 0, 0, 0, 0, 0) == gettid());
	CHECK(call(GUEST_SYS_set_tid_address, 0, 0, 0, 0, 0) == gettid());
	/* unsupported calls */
	CHECK(call(GUEST_SYS_statx, 0, 0, 0, 0, 0) == -ENOSYS);
	CHECK(warnings == before);
	CHECK(call(0 /* io_setup */, 0, 0, 0, 0, 0) == -ENOSYS);
	CHECK(call(9999, 0, 0, 0, 0, 0) == -ENOSYS);
	CHECK(warnings == before + 2);
	CHECK(call(GUEST_SYS_ioctl, 0, 0, 0, 0, 0) == -ENOTTY);
}

static void test_open_flags(void)
{
	static const uint32_t single[] =
	{
		GUEST_O_CREAT, GUEST_O_EXCL, GUEST_O_NOCTTY, GUEST_O_TRUNC, GUEST_O_APPEND, GUEST_O_NONBLOCK,
		GUEST_O_DSYNC, GUEST_O_ASYNC, GUEST_O_DIRECTORY, GUEST_O_NOFOLLOW, GUEST_O_DIRECT,
		GUEST_O_NOATIME, GUEST_O_CLOEXEC, GUEST_O_SYNC, GUEST_O_PATH, GUEST_O_TMPFILE,
	};
	int index;

	CHECK(open_flags_in(GUEST_O_RDONLY) == O_RDONLY);
	CHECK(open_flags_in(GUEST_O_WRONLY) == O_WRONLY);
	CHECK(open_flags_in(GUEST_O_RDWR) == O_RDWR);
	CHECK(open_flags_in(GUEST_O_DIRECTORY) == O_DIRECTORY);
	CHECK(open_flags_in(GUEST_O_NOFOLLOW) == O_NOFOLLOW);
	CHECK(open_flags_in(GUEST_O_DIRECT) == O_DIRECT);
	CHECK(open_flags_in(GUEST_O_CLOEXEC | GUEST_O_RDWR) == (O_CLOEXEC | O_RDWR));
	CHECK(open_flags_in(GUEST_O_SYNC) == O_SYNC);
	CHECK(open_flags_in(GUEST_O_TMPFILE | GUEST_O_RDWR) == (O_TMPFILE | O_RDWR));
	CHECK(open_flags_in(GUEST_O_LARGEFILE) == O_LARGEFILE);
	/* a bit outside the interface */
	CHECK(open_flags_in(0x40000000u) == -EINVAL);
	/* each flag comes back as it went */
	for (index = 0; index < (int)(sizeof(single) / sizeof(single[0])); index++)
	{
		long host = open_flags_in(single[index] | GUEST_O_WRONLY);

		CHECK(host >= 0);
		CHECK(open_flags_out(host) == (single[index] | GUEST_O_WRONLY));
	}
}

static void test_open_and_fcntl(void)
{
	char *file = path_in("file");
	char *link = path_in("link");
	char *dir = low_string(directory);
	long long fd, result;

	fd = call(GUEST_SYS_openat, guest_int(GUEST_AT_FDCWD), guest_address(file),
		GUEST_O_CREAT | GUEST_O_WRONLY | GUEST_O_TRUNC | GUEST_O_CLOEXEC | GUEST_O_LARGEFILE | GUEST_O_APPEND,
		0644, 0);
	CHECK(fd >= 0);
	CHECK((fcntl((int)fd, F_GETFD) & FD_CLOEXEC) != 0);
	CHECK((fcntl((int)fd, F_GETFL) & O_APPEND) != 0);
	/* F_GETFL gives the guest's flags */
	result = call(GUEST_SYS_fcntl, fd, GUEST_F_GETFL, 0, 0, 0);
	CHECK(result >= 0 && (result & GUEST_O_ACCMODE) == GUEST_O_WRONLY);
	CHECK(result >= 0 && (result & GUEST_O_APPEND));
	CHECK(result >= 0 && !(result & GUEST_O_NONBLOCK));
	CHECK(call(GUEST_SYS_fcntl, fd, GUEST_F_SETFL, GUEST_O_NONBLOCK, 0, 0) == 0);
	CHECK((fcntl((int)fd, F_GETFL) & O_NONBLOCK) != 0);
	CHECK((call(GUEST_SYS_fcntl, fd, GUEST_F_GETFL, 0, 0, 0) & GUEST_O_NONBLOCK) != 0);
	CHECK(call(GUEST_SYS_fcntl, fd, GUEST_F_SETFL, 0x40000000u, 0, 0) == -EINVAL);
	CHECK(call(GUEST_SYS_fcntl, fd, GUEST_F_SETFD, 0, 0, 0) == 0);
	CHECK(call(GUEST_SYS_fcntl, fd, GUEST_F_GETFD, 0, 0, 0) == 0);
	CHECK(call(GUEST_SYS_fcntl, fd, 12345, 0, 0, 0) == -EINVAL);
	result = call(GUEST_SYS_fcntl, fd, GUEST_F_DUPFD_CLOEXEC, 100, 0, 0);
	CHECK(result >= 100);
	CHECK(call(GUEST_SYS_close, result, 0, 0, 0, 0) == 0);
	CHECK(call(GUEST_SYS_close, fd, 0, 0, 0, 0) == 0);

	/* the guest's O_DIRECTORY (x86-64's O_DIRECT) */
	fd = call(GUEST_SYS_openat, guest_int(GUEST_AT_FDCWD), guest_address(dir), GUEST_O_RDONLY | GUEST_O_DIRECTORY, 0, 0);
	CHECK(fd >= 0);
	call(GUEST_SYS_close, fd, 0, 0, 0, 0);
	CHECK(call(GUEST_SYS_openat, guest_int(GUEST_AT_FDCWD), guest_address(file),
		GUEST_O_RDONLY | GUEST_O_DIRECTORY, 0, 0) == -ENOTDIR);
	/* the guest's O_NOFOLLOW */
	CHECK(symlink(file, link) == 0);
	CHECK(call(GUEST_SYS_openat, guest_int(GUEST_AT_FDCWD), guest_address(link),
		GUEST_O_RDONLY | GUEST_O_NOFOLLOW, 0, 0) == -ELOOP);
	/* the guest's O_EXCL */
	CHECK(call(GUEST_SYS_openat, guest_int(GUEST_AT_FDCWD), guest_address(file),
		GUEST_O_WRONLY | GUEST_O_CREAT | GUEST_O_EXCL, 0644, 0) == -EEXIST);
	CHECK(call(GUEST_SYS_openat, guest_int(GUEST_AT_FDCWD), guest_address(file),
		GUEST_O_RDONLY | 0x40000000u, 0, 0) == -EINVAL);
}

static void test_pipe_and_dup(void)
{
	int32_t *fds = low_alloc(2 * sizeof(int32_t));
	long long result;

	CHECK(call(GUEST_SYS_pipe2, guest_address(fds), GUEST_O_CLOEXEC | GUEST_O_NONBLOCK, 0, 0, 0) == 0);
	CHECK((fcntl(fds[0], F_GETFD) & FD_CLOEXEC) != 0);
	CHECK((fcntl(fds[1], F_GETFL) & O_NONBLOCK) != 0);
	result = call(GUEST_SYS_dup3, fds[0], 200, GUEST_O_CLOEXEC, 0, 0);
	CHECK(result == 200);
	CHECK((fcntl(200, F_GETFD) & FD_CLOEXEC) != 0);
	CHECK(call(GUEST_SYS_dup3, fds[0], 201, GUEST_O_APPEND, 0, 0) == -EINVAL);
	close(200);
	close(fds[0]);
	close(fds[1]);
}

static void test_stat(void)
{
	char *file = path_in("stat");
	char *link = path_in("stat_link");
	struct halo_guest_kstat *guest = low_alloc(sizeof(*guest) + 16);
	uint8_t *after = (uint8_t *)(guest + 1);
	char *empty = low_string("");
	struct stat host;
	struct timespec times[2] = {{1000000000, 123456789}, {1500000000, 987654321}};
	int fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0640);
	char data[5000];

	CHECK(fd >= 0);
	memset(data, 'x', sizeof(data));
	CHECK(write(fd, data, sizeof(data)) == (ssize_t)sizeof(data));
	CHECK(futimens(fd, times) == 0);
	CHECK(fstat(fd, &host) == 0);
	memset(after, 0xa5, 16);

	CHECK(call(GUEST_SYS_fstat, fd, guest_address(guest), 0, 0, 0) == 0);
	CHECK(guest->st_dev == (uint64_t)host.st_dev);
	CHECK(guest->st_ino == (uint64_t)host.st_ino);
	CHECK(guest->st_mode == (uint32_t)host.st_mode);
	CHECK((guest->st_mode & 07777) == 0640 && S_ISREG(guest->st_mode));
	CHECK(guest->st_nlink == 1);
	CHECK(guest->st_uid == (uint32_t)getuid());
	CHECK(guest->st_gid == (uint32_t)host.st_gid);
	CHECK(guest->st_size == 5000);
	CHECK(guest->st_blksize == (int32_t)host.st_blksize);
	CHECK(guest->st_blocks == (int64_t)host.st_blocks);
	CHECK(guest->atime_seconds == 1000000000 && guest->atime_nanoseconds == 123456789);
	CHECK(guest->mtime_seconds == 1500000000 && guest->mtime_nanoseconds == 987654321);
	CHECK(guest->ctime_seconds == (int64_t)host.st_ctim.tv_sec);
	/* the layout as musl reads it (arch/generic/kstat.h), byte by byte */
	CHECK(*(int64_t *)((uint8_t *)guest + 48) == 5000);
	CHECK(*(uint32_t *)((uint8_t *)guest + 16) == (uint32_t)host.st_mode);
	CHECK(*(int64_t *)((uint8_t *)guest + 88) == 1500000000);
	CHECK(*(int64_t *)((uint8_t *)guest + 96) == 987654321);
	/* nothing written past the structure */
	CHECK(after[0] == 0xa5 && after[15] == 0xa5);

	/* newfstatat: by path, by descriptor (AT_EMPTY_PATH), and a link
	itself (AT_SYMLINK_NOFOLLOW) */
	memset(guest, 0, sizeof(*guest));
	CHECK(call(GUEST_SYS_newfstatat, guest_int(GUEST_AT_FDCWD), guest_address(file), guest_address(guest), 0, 0) == 0);
	CHECK(guest->st_ino == (uint64_t)host.st_ino && guest->st_size == 5000);
	memset(guest, 0, sizeof(*guest));
	CHECK(call(GUEST_SYS_newfstatat, fd, guest_address(empty), guest_address(guest), GUEST_AT_EMPTY_PATH, 0) == 0);
	CHECK(guest->st_ino == (uint64_t)host.st_ino);
	CHECK(symlink(file, link) == 0);
	CHECK(call(GUEST_SYS_newfstatat, guest_int(GUEST_AT_FDCWD), guest_address(link), guest_address(guest),
		GUEST_AT_SYMLINK_NOFOLLOW, 0) == 0);
	CHECK(S_ISLNK(guest->st_mode));
	CHECK(call(GUEST_SYS_newfstatat, guest_int(GUEST_AT_FDCWD), guest_address(link), guest_address(guest), 0, 0) == 0);
	CHECK(S_ISREG(guest->st_mode) && guest->st_size == 5000);
	CHECK(call(GUEST_SYS_newfstatat, guest_int(GUEST_AT_FDCWD), guest_address(path_in("missing")),
		guest_address(guest), 0, 0) == -ENOENT);
	CHECK(call(GUEST_SYS_fstat, -1, guest_address(guest), 0, 0, 0) == -EBADF);
	close(fd);
}

static void test_llseek(void)
{
	char *file = path_in("sparse");
	int64_t *position = low_alloc(sizeof(int64_t));
	int fd = open(file, O_CREAT | O_RDWR | O_TRUNC, 0644);
	const int64_t far = 5LL << 30; /* beyond 4 GB */

	CHECK(fd >= 0);
	CHECK(call(GUEST_SYS__llseek, fd, (uint32_t)(far >> 32), (uint32_t)far, guest_address(position), GUEST_SEEK_SET) == 0);
	CHECK(*position == far);
	CHECK(lseek(fd, 0, SEEK_CUR) == far);
	CHECK(write(fd, "z", 1) == 1);
	/* musl passes the whole offset as the low half; only its low 32 bits count */
	CHECK(call(GUEST_SYS__llseek, fd, 0, 0, guest_address(position), GUEST_SEEK_END) == 0);
	CHECK(*position == far + 1);
	CHECK(call(GUEST_SYS__llseek, fd, 0xffffffffu, (uint32_t)-1, guest_address(position), GUEST_SEEK_CUR) == 0);
	CHECK(*position == far);
	CHECK(call(GUEST_SYS__llseek, fd, 0, 0, guest_address(position), 99) == -EINVAL);
	CHECK(call(GUEST_SYS__llseek, fd, 0, 0, 0, GUEST_SEEK_SET) == -EFAULT);
	close(fd);
}

static void test_locks(void)
{
	char *file = path_in("locks");
	struct halo_guest_flock *lock = low_alloc(sizeof(*lock));
	int fd = open(file, O_CREAT | O_RDWR | O_TRUNC, 0644);
	pid_t child;
	int status;

	CHECK(fd >= 0);
	lock->l_type = GUEST_F_WRLCK;
	lock->l_whence = GUEST_SEEK_SET;
	lock->l_start = 10;
	lock->l_len = 20;
	CHECK(call(GUEST_SYS_fcntl, fd, GUEST_F_SETLK, guest_address(lock), 0, 0) == 0);
	/* another process sees the lock through F_GETLK */
	child = fork();
	if (child == 0)
	{
		int other = open(file, O_RDWR);
		int ok;

		memset(lock, 0, sizeof(*lock));
		lock->l_type = GUEST_F_WRLCK;
		lock->l_whence = GUEST_SEEK_SET;
		lock->l_start = 0;
		lock->l_len = 0;
		ok = call(GUEST_SYS_fcntl, other, GUEST_F_GETLK, guest_address(lock), 0, 0) == 0 &&
			lock->l_type == GUEST_F_WRLCK && lock->l_start == 10 && lock->l_len == 20 &&
			lock->l_pid == getppid();
		lock->l_type = GUEST_F_WRLCK;
		lock->l_start = 15;
		lock->l_len = 1;
		ok = ok && call(GUEST_SYS_fcntl, other, GUEST_F_SETLK, guest_address(lock), 0, 0) == -EAGAIN;
		_exit(ok ? 0 : 1);
	}
	CHECK(child > 0);
	CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
	CHECK(call(GUEST_SYS_flock, fd, GUEST_LOCK_EX | GUEST_LOCK_NB, 0, 0, 0) == 0);
	CHECK(call(GUEST_SYS_flock, fd, GUEST_LOCK_UN, 0, 0, 0) == 0);
	close(fd);
}

static void test_iovecs(void)
{
	char *file = path_in("vectors");
	struct halo_guest_iovec *vector = low_alloc(sizeof(*vector) * (GUEST_IOV_MAX + 1));
	uint8_t *bytes = low_alloc(GUEST_IOV_MAX * 2);
	uint8_t check[GUEST_IOV_MAX * 2];
	int fd = open(file, O_CREAT | O_RDWR | O_TRUNC, 0644);
	int index;

	CHECK(fd >= 0);
	for (index = 0; index < GUEST_IOV_MAX * 2; index++)
		bytes[index] = (uint8_t)(index * 7);
	/* the whole of IOV_MAX, two bytes each */
	for (index = 0; index <= GUEST_IOV_MAX; index++)
	{
		vector[index].base = guest_address(bytes + 2 * (index % GUEST_IOV_MAX));
		vector[index].length = 2;
	}
	CHECK(call(GUEST_SYS_writev, fd, guest_address(vector), GUEST_IOV_MAX, 0, 0) == GUEST_IOV_MAX * 2);
	CHECK(pread(fd, check, sizeof(check), 0) == (ssize_t)sizeof(check));
	CHECK(memcmp(check, bytes, sizeof(check)) == 0);
	CHECK(call(GUEST_SYS_writev, fd, guest_address(vector), GUEST_IOV_MAX + 1, 0, 0) == -EINVAL);
	CHECK(call(GUEST_SYS_writev, fd, guest_address(vector), guest_int(-1), 0, 0) == -EINVAL);
	CHECK(call(GUEST_SYS_writev, fd, guest_address(vector), 0, 0, 0) == 0);
	/* a short vector (the stack's) */
	CHECK(call(GUEST_SYS_pwritev, fd, guest_address(vector), 3, 4096, 0) == 6);
	CHECK(pread(fd, check, 6, 4096) == 6 && memcmp(check, bytes, 6) == 0);

	/* reading back, in 300 pieces of different lengths */
	memset(bytes, 0, GUEST_IOV_MAX * 2);
	for (index = 0; index < 300; index++)
	{
		vector[index].base = guest_address(bytes + index * 6);
		vector[index].length = 6;
	}
	CHECK(call(GUEST_SYS_preadv, fd, guest_address(vector), 300, 0, 0) == 1800);
	CHECK(pread(fd, check, 1800, 0) == 1800 && memcmp(check, bytes, 1800) == 0);
	CHECK(lseek(fd, 0, SEEK_SET) == 0);
	memset(bytes, 0, 1800);
	CHECK(call(GUEST_SYS_readv, fd, guest_address(vector), 300, 0, 0) == 1800);
	CHECK(memcmp(check, bytes, 1800) == 0);
	close(fd);
}

static void test_directories_and_limits(void)
{
	char *sub = path_in("sub");
	char *renamed = path_in("sub2");
	uint8_t *entries = low_alloc(4096);
	uint64_t *limit = low_alloc(2 * sizeof(uint64_t));
	struct rlimit host_limit;
	long long fd, size;
	int found = 0;
	long offset;

	CHECK(call(GUEST_SYS_mkdirat, guest_int(GUEST_AT_FDCWD), guest_address(sub), 0755, 0, 0) == 0);
	CHECK(call(GUEST_SYS_renameat, guest_int(GUEST_AT_FDCWD), guest_address(sub), guest_int(GUEST_AT_FDCWD),
		guest_address(renamed), 0) == 0);
	fd = call(GUEST_SYS_openat, guest_int(GUEST_AT_FDCWD), guest_address(low_string(directory)),
		GUEST_O_RDONLY | GUEST_O_DIRECTORY | GUEST_O_CLOEXEC, 0, 0);
	CHECK(fd >= 0);
	/* struct linux_dirent64 is the same everywhere */
	size = call(GUEST_SYS_getdents64, fd, guest_address(entries), 4096, 0, 0);
	CHECK(size > 0);
	for (offset = 0; offset < size;)
	{
		uint16_t length;

		memcpy(&length, entries + offset + 16, sizeof(length));
		if (!strcmp((const char *)entries + offset + 19, "sub2") && entries[offset + 18] == DT_DIR)
			found = 1;
		if (!length)
			break;
		offset += length;
	}
	CHECK(found);
	call(GUEST_SYS_close, fd, 0, 0, 0, 0);
	CHECK(call(GUEST_SYS_unlinkat, guest_int(GUEST_AT_FDCWD), guest_address(renamed), GUEST_AT_REMOVEDIR, 0, 0) == 0);

	CHECK(getrlimit(RLIMIT_NOFILE, &host_limit) == 0);
	CHECK(call(GUEST_SYS_getrlimit, GUEST_RLIMIT_NOFILE, guest_address(limit), 0, 0, 0) == 0);
	CHECK(limit[0] == (uint64_t)host_limit.rlim_cur && limit[1] == (uint64_t)host_limit.rlim_max);
	memset(limit, 0, 2 * sizeof(uint64_t));
	CHECK(call(GUEST_SYS_prlimit64, 0, GUEST_RLIMIT_NOFILE, 0, guest_address(limit), 0) == 0);
	CHECK(limit[0] == (uint64_t)host_limit.rlim_cur);
}

static void test_time(void)
{
	struct halo_guest_timespec *value = low_alloc(sizeof(*value));
	struct timespec host;

	CHECK(call(GUEST_SYS_clock_gettime, CLOCK_MONOTONIC, guest_address(value), 0, 0, 0) == 0);
	CHECK(clock_gettime(CLOCK_MONOTONIC, &host) == 0);
	CHECK(value->seconds <= (int32_t)host.tv_sec && value->seconds >= (int32_t)host.tv_sec - 1);
	CHECK(value->nanoseconds >= 0 && value->nanoseconds < 1000000000);
	CHECK(call(GUEST_SYS_clock_getres, CLOCK_MONOTONIC, guest_address(value), 0, 0, 0) == 0);
	CHECK(value->seconds == 0 && value->nanoseconds > 0);
	CHECK(call(GUEST_SYS_clock_gettime, 12345, guest_address(value), 0, 0, 0) == -EINVAL);
}

/* the files go in a new folder in argv[1], or in /tmp */
int main(int argc, char **argv)
{
	char template[256];
	char command[600];

	snprintf(template, sizeof(template), "%s/host_syscall_test.XXXXXX", argc > 1 ? argv[1] : "/tmp");

	if (!low_initialize())
	{
		printf("FAIL: no memory below 4 GB\n");
		return 1;
	}
	if (!mkdtemp(template))
	{
		printf("FAIL: mkdtemp\n");
		return 1;
	}
	snprintf(directory, sizeof(directory), "%s", template);

	test_numbers();
	test_open_flags();
	test_open_and_fcntl();
	test_pipe_and_dup();
	test_stat();
	test_llseek();
	test_locks();
	test_iovecs();
	test_directories_and_limits();
	test_time();

	snprintf(command, sizeof(command), "rm -rf '%s'", directory);
	if (system(command) != 0)
		printf("could not remove %s\n", directory);
	if (failures)
	{
		printf("%d failures\n", failures);
		return 1;
	}
	printf("all passed\n");
	return 0;
}
