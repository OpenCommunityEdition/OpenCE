/*
HOST_MEMORY_TEST.C

Tests of the guest's memory system calls (port/android/host/host_memory.c)
on the build machine: which ranges the guest may map, unmap, reprotect and
advise; pool pages going back to their pool, partly too; MAP_FIXED and
MAP_FIXED_NOREPLACE inside a pool, the window and free address space; and a
crash report whose frame pointer points into a PROT_NONE page. Built and run
by tools/test_host_units.py; exits nonzero on a failure.
*/

#include "../host/host_memory.c"

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

/* ---------- the host functions host_memory.c calls */

struct host_guest_image host_image;

/* the log goes to this descriptor (the crash test's pipe), else nowhere */
static int log_descriptor = -1;

void host_log(int priority, const char *text)
{
	(void)priority;
	if (log_descriptor >= 0)
	{
		if (write(log_descriptor, text, strlen(text)) < 0 || write(log_descriptor, "\n", 1) < 0)
			return;
	}
}

void host_logf(int priority, const char *format, ...)
{
	char line[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	host_log(priority, line);
}

/* ---------- helpers */

#define READ_WRITE (PROT_READ | PROT_WRITE)
#define ANONYMOUS (MAP_PRIVATE | MAP_ANONYMOUS)

static int readable(uint64_t address)
{
	uint8_t byte;

	return host_arch_read_memory((uintptr_t)address, &byte, 1);
}

static int used(const struct pool *pool, uint64_t address)
{
	return pool->used[(address - pool->base) / PAGE];
}

/* free_pages agrees with used[] */
static int pool_consistent(const struct pool *pool)
{
	uint64_t page, free_pages = 0;

	for (page = 0; page < POOL_PAGES; page++)
		free_pages += !pool->used[page];
	return free_pages == pool->free_pages;
}

/* ---------- tests */

static void test_pool(void)
{
	long a = host_guest_mmap(0, 4 * PAGE, READ_WRITE, ANONYMOUS, -1, 0);
	struct pool *pool;
	long b;
	uint64_t far;

	CHECK(a > 0);
	if (a <= 0)
		return;
	pool = pool_of((uint64_t)a, 4 * PAGE);
	CHECK(pool != NULL);
	if (!pool)
		return;
	memset((void *)a, 0x11, 4 * PAGE);
	CHECK(used(pool, a) && used(pool, a + 3 * PAGE));

	/* the middle of a mapping goes back to the pool, its ends stay */
	CHECK(host_guest_munmap((uint64_t)a + PAGE, 2 * PAGE) == 0);
	CHECK(used(pool, a) && !used(pool, a + PAGE) && !used(pool, a + 2 * PAGE) && used(pool, a + 3 * PAGE));
	CHECK(readable(a) && !readable(a + PAGE) && !readable(a + 2 * PAGE) && readable(a + 3 * PAGE));
	CHECK(*(uint8_t *)(a + 3 * PAGE) == 0x11);
	CHECK(pool_consistent(pool));
	CHECK(host_guest_munmap((uint64_t)a + 1, PAGE) == -EINVAL);
	CHECK(host_guest_munmap((uint64_t)a, 0) == -EINVAL);

	/* no replace: refused over a page in use, which is left alone */
	CHECK(host_guest_mmap((uint64_t)a, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == -EEXIST);
	CHECK(host_guest_mmap((uint64_t)a + PAGE, 3 * PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == -EEXIST);
	CHECK(*(uint8_t *)a == 0x11 && !readable(a + PAGE));
	CHECK(!used(pool, a + PAGE));
	/* taken from the pool where its pages are free */
	CHECK(host_guest_mmap((uint64_t)a + PAGE, 2 * PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) ==
		(long)(a + PAGE));
	CHECK(used(pool, a + PAGE) && used(pool, a + 2 * PAGE));
	CHECK(readable(a + PAGE) && *(uint8_t *)(a + PAGE) == 0);
	CHECK(pool_consistent(pool));
	/* so the pool does not hand them out again */
	b = host_guest_mmap(0, PAGE, READ_WRITE, ANONYMOUS, -1, 0);
	CHECK(b > 0 && (b < a || b >= (long)(a + 4 * PAGE)));

	/* MAP_FIXED into free pages of the pool takes them too */
	far = pool->base + (POOL_PAGES - 2) * PAGE;
	CHECK(!used(pool, far));
	CHECK(host_guest_mmap(far, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == (long)far);
	CHECK(used(pool, far) && readable(far));
	/* and replaces a mapping of the guest's */
	CHECK(host_guest_mmap((uint64_t)a, PAGE, PROT_READ, ANONYMOUS | MAP_FIXED, -1, 0) == a);
	CHECK(*(uint8_t *)a == 0);
	CHECK(pool_consistent(pool));

	/* mprotect and madvise on the pool's pages */
	CHECK(host_guest_mprotect((uint64_t)a + PAGE, PAGE, PROT_READ) == 0);
	CHECK(host_guest_madvise((uint64_t)a + PAGE, PAGE, MADV_DONTNEED) == 0);

	CHECK(host_guest_munmap((uint64_t)a, 4 * PAGE) == 0);
	CHECK(host_guest_munmap(far, PAGE) == 0);
	CHECK(host_guest_munmap((uint64_t)b, PAGE) == 0);
	CHECK(!used(pool, a) && !used(pool, a + 3 * PAGE) && !used(pool, far) && !used(pool, b));
	CHECK(pool_consistent(pool));
}

/* memory of the host's below 4 GB, and free address space */
static void test_host_memory(void)
{
	uint64_t host = find_gap(16 * PAGE, LOW_START);
	void *mapped;
	struct pool *pool;
	long a;

	CHECK(host != 0);
	if (!host)
		return;
	mapped = mmap((void *)(uintptr_t)host, 16 * PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	CHECK(mapped == (void *)(uintptr_t)host);
	if (mapped != (void *)(uintptr_t)host)
		return;
	memset(mapped, 0x22, 16 * PAGE);
	CHECK(host_guest_munmap(host, PAGE) == -EINVAL);
	CHECK(host_guest_mprotect(host, PAGE, PROT_READ) == -ENOMEM);
	CHECK(host_guest_madvise(host, PAGE, MADV_DONTNEED) == -ENOMEM);
	CHECK(host_guest_mmap(host, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == -EINVAL);
	CHECK(host_guest_mmap(host, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == -EEXIST);
	CHECK(readable(host) && *(uint8_t *)(uintptr_t)host == 0x22 && *(uint8_t *)(uintptr_t)(host + 15 * PAGE) == 0x22);

	/* free address space the guest never mapped */
	CHECK(munmap(mapped, 16 * PAGE) == 0);
	CHECK(host_guest_munmap(host, PAGE) == -EINVAL);
	CHECK(host_guest_mprotect(host, PAGE, PROT_READ) == -ENOMEM);
	CHECK(host_guest_mmap(host, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == -EINVAL);

	/* a range partly a pool's and partly the host's: nothing is done */
	a = host_guest_mmap(0, PAGE, READ_WRITE, ANONYMOUS, -1, 0);
	pool = a > 0 ? pool_of((uint64_t)a, PAGE) : NULL;
	CHECK(pool != NULL);
	if (!pool)
		return;
	host = pool->base + POOL_SIZE;
	mapped = mmap((void *)(uintptr_t)host, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	if (mapped == (void *)(uintptr_t)host && !pool_of(host, PAGE))
	{
		uint64_t last = host - PAGE;

		CHECK(host_guest_mmap(last, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == (long)last);
		*(uint8_t *)(uintptr_t)last = 0x33;
		*(uint8_t *)mapped = 0x44;
		CHECK(host_guest_munmap(last, 2 * PAGE) == -EINVAL);
		CHECK(host_guest_mprotect(last, 2 * PAGE, PROT_READ) == -ENOMEM);
		CHECK(host_guest_mmap(last, 2 * PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == -EINVAL);
		CHECK(host_guest_mmap(last - PAGE, 2 * PAGE + PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) ==
			-EEXIST);
		CHECK(*(uint8_t *)(uintptr_t)last == 0x33 && *(uint8_t *)mapped == 0x44 && used(pool, last));
		CHECK(!used(pool, last - PAGE));
		CHECK(host_guest_munmap(last, PAGE) == 0);
		munmap(mapped, PAGE);
	}
	else
	{
		printf("(the page above the pool is taken: the partial overlap is not tested)\n");
		if (mapped != MAP_FAILED)
			munmap(mapped, PAGE);
	}
	CHECK(host_guest_munmap((uint64_t)a, PAGE) == 0);
}

/* ranges the guest maps itself at addresses of its choosing */
static void test_guest_ranges(void)
{
	uint64_t guest = find_gap(8 * PAGE, LOW_START);
	long result;

	CHECK(guest != 0);
	if (!guest)
		return;
	result = host_guest_mmap(guest, 8 * PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	CHECK(result == (long)guest);
	if (result != (long)guest)
		return;
	memset((void *)(uintptr_t)guest, 0x55, 8 * PAGE);
	CHECK(host_guest_mmap(guest + 4 * PAGE, 8 * PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == -EEXIST);
	CHECK(*(uint8_t *)(uintptr_t)(guest + 4 * PAGE) == 0x55);
	CHECK(host_guest_mprotect(guest, PAGE, PROT_READ) == 0);
	CHECK(host_guest_madvise(guest + PAGE, PAGE, MADV_DONTNEED) == 0);
	CHECK(host_guest_mmap(guest + PAGE, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == (long)(guest + PAGE));
	/* unmapped in parts */
	CHECK(host_guest_munmap(guest + 2 * PAGE, 2 * PAGE) == 0);
	CHECK(readable(guest + PAGE) && !readable(guest + 2 * PAGE) && !readable(guest + 3 * PAGE));
	CHECK(readable(guest + 4 * PAGE));
	/* what it unmapped is no longer its own */
	CHECK(host_guest_munmap(guest + 2 * PAGE, PAGE) == -EINVAL);
	CHECK(host_guest_mprotect(guest + 3 * PAGE, PAGE, PROT_READ) == -ENOMEM);
	CHECK(host_guest_munmap(guest + PAGE, 2 * PAGE) == -EINVAL);
	CHECK(readable(guest + PAGE));
	/* but it can map there again */
	CHECK(host_guest_mmap(guest + 2 * PAGE, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) ==
		(long)(guest + 2 * PAGE));
	CHECK(host_guest_munmap(guest, 3 * PAGE) == 0);
	CHECK(host_guest_munmap(guest + 4 * PAGE, 4 * PAGE) == 0);
	CHECK(!readable(guest) && !readable(guest + 7 * PAGE));
	CHECK(host_guest_munmap(guest, PAGE) == -EINVAL);
}

static void test_window(void)
{
	uint64_t window = HALO_GUEST_WINDOW_BASE;

	/* the guest reserves it as port/linux/src/xbox_memory.c does */
	CHECK(host_guest_mmap(window, HALO_GUEST_WINDOW_SIZE, PROT_NONE, ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE,
		-1, 0) == (long)window);
	CHECK(host_guest_mmap(window + PAGE, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == -EEXIST);
	CHECK(host_guest_mmap(window + PAGE, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == (long)(window + PAGE));
	*(uint8_t *)(uintptr_t)(window + PAGE) = 0x66;
	CHECK(host_guest_mprotect(window + PAGE, PAGE, PROT_READ) == 0);
	CHECK(host_guest_munmap(window + PAGE, PAGE) == 0);
	CHECK(!readable(window + PAGE));
	/* back to the reservation, which a "no replace" mapping may take */
	CHECK(host_guest_mmap(window + PAGE, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) ==
		(long)(window + PAGE));
	CHECK(readable(window + PAGE) && *(uint8_t *)(uintptr_t)(window + PAGE) == 0);
	/* a range across its end: what is there decides (free space, the image
	on AArch64, or a pool on x86-64, placed in the first gap above the
	image's range) */
	{
		uint64_t last = window + HALO_GUEST_WINDOW_SIZE - PAGE, after = last + PAGE;
		struct pool *pool;
		int kind = page_kind(after, &pool);

		CHECK(host_guest_mmap(last, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == (long)last);
		if (kind == PAGE_POOL)
		{
			/* the window's page goes back to its reservation, the pool's to
			the pool */
			CHECK(host_guest_mmap(after, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == (long)after);
			CHECK(used(pool, after));
			CHECK(host_guest_munmap(last, 2 * PAGE) == 0);
			CHECK(!used(pool, after) && pool_consistent(pool));
			CHECK(!readable(last) && !readable(after));
		}
		else
		{
			CHECK(kind == PAGE_HOST || kind == PAGE_IMAGE);
			CHECK(host_guest_munmap(last, 2 * PAGE) == -EINVAL);
			CHECK(readable(last));
			if (kind == PAGE_HOST)
				CHECK(host_guest_mmap(last, 2 * PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED, -1, 0) == -EINVAL);
		}
	}
	CHECK(host_guest_munmap(window, HALO_GUEST_WINDOW_SIZE) == 0);
	CHECK(!readable(window + PAGE));
}

static void test_image(void)
{
	uint64_t image = HALO_GUEST_IMAGE_BASE;

	CHECK(host_guest_munmap(image, PAGE) == -EINVAL);
	CHECK(host_guest_mmap(image, PAGE, READ_WRITE, ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == -EEXIST);
	CHECK(host_guest_mprotect(image, PAGE, READ_WRITE) == 0);
	CHECK(readable(image));
	CHECK(host_guest_mprotect(image, PAGE, PROT_NONE) == 0);
}

/* a crash whose frame pointer points into a pool's PROT_NONE page: one
report, then the default action (the walk must not fault inside the
handler, which would report again and again) */
static void test_crash_report(void)
{
	int channel[2];
	pid_t child;
	int status;
	char output[16384];
	size_t length = 0;
	ssize_t count;
	const char *at;
	int reports = 0;
	uint64_t guard;
	long a = host_guest_mmap(0, 2 * PAGE, READ_WRITE, ANONYMOUS, -1, 0);

	CHECK(a > 0);
	if (a <= 0)
		return;
	/* a guard page, as a thread stack has */
	CHECK(host_guest_mprotect((uint64_t)a, PAGE, PROT_NONE) == 0);
	guard = (uint64_t)a + 64;
	CHECK(pipe(channel) == 0);
	child = fork();
	if (child == 0)
	{
		close(channel[0]);
		log_descriptor = channel[1];
		host_install_signal_handlers();
#if defined(__x86_64__)
		__asm__ volatile("mov %0, %%rbp\n\tmovb 0, %%al" : : "r"(guard) : "rax", "memory");
#elif defined(__aarch64__)
		__asm__ volatile("mov x29, %0\n\tmov x1, #0\n\tldr x0, [x1]" : : "r"(guard) : "x0", "x1", "memory");
#endif
		_exit(0);
	}
	close(channel[1]);
	while (length < sizeof(output) - 1 && (count = read(channel[0], output + length, sizeof(output) - 1 - length)) > 0)
		length += (size_t)count;
	output[length] = '\0';
	close(channel[0]);
	CHECK(waitpid(child, &status, 0) == child);
	CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV);
	for (at = output; (at = strstr(at, "signal 11 at")) != NULL; at++)
		reports++;
	CHECK(reports == 1);
	CHECK(strstr(output, "frame  0") == NULL);
	if (reports != 1)
		printf("the child's log:\n%s\n", output);
	CHECK(host_guest_munmap((uint64_t)a, 2 * PAGE) == 0);
}

int main(void)
{
	if (host_memory_initialize(HALO_GUEST_IMAGE_BASE, 0x10000) != 0)
	{
		printf("FAIL: cannot reserve the window and the image's range\n");
		return 1;
	}
	test_pool();
	test_host_memory();
	test_guest_ranges();
	test_window();
	test_image();
	test_crash_report();
	if (failures)
	{
		printf("%d failures\n", failures);
		return 1;
	}
	printf("host_memory: all passed\n");
	return 0;
}
