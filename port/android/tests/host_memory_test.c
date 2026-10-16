/*
HOST_MEMORY_TEST.C

Tests of port/android/host/host_memory.c on the build machine: a crash
report whose frame pointer points into a PROT_NONE page. Built and run
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

/* ---------- tests */

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
	test_crash_report();
	if (failures)
	{
		printf("%d failures\n", failures);
		return 1;
	}
	printf("host_memory: all passed\n");
	return 0;
}
