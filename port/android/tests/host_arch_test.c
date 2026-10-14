/*
HOST_ARCH_TEST.C

Tests of port/android/host/host_arch.c on the build machine's processor
(AArch64 or x86-64): the registers read from a ucontext_t made here, the
register dump, the frame walker on frame records made here (with its reader
and with host_arch_read_memory, over a PROT_NONE page and an unmapped one),
and on real signals' contexts, one with a garbage frame pointer. Built and run by tools/test_host_units.py; exits
nonzero on a failure.
*/

#include "host.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <unistd.h>

static int failures;

static void expect(int condition, const char *what, int line)
{
	if (!condition)
	{
		printf("line %d: %s\n", line, what);
		failures++;
	}
}

#define EXPECT(condition) expect((condition), #condition, __LINE__)

/* ---------- registers */

static void set_registers(ucontext_t *context, uint64_t pc, uint64_t sp, uint64_t fp, uint64_t lr)
{
	memset(context, 0, sizeof(*context));
#if defined(__aarch64__)
	{
		struct sigcontext *registers = (struct sigcontext *)&context->uc_mcontext;
		int index;

		for (index = 0; index < 31; index++)
			registers->regs[index] = 0x1000 + index;
		registers->pc = pc;
		registers->sp = sp;
		registers->regs[29] = fp;
		registers->regs[30] = lr;
	}
#elif defined(__x86_64__)
	{
		int index;

		(void)lr;
		for (index = 0; index < NGREG; index++)
			context->uc_mcontext.gregs[index] = 0x1000 + index;
		context->uc_mcontext.gregs[REG_RIP] = (greg_t)pc;
		context->uc_mcontext.gregs[REG_RSP] = (greg_t)sp;
		context->uc_mcontext.gregs[REG_RBP] = (greg_t)fp;
	}
#endif
}

static void test_context(void)
{
	ucontext_t context;
	uint64_t pc = 1, sp = 1, fp = 1, lr = 1;
	char line[256];
	char dump[4096] = "";
	int count;

	set_registers(&context, 0x88001234ULL, 0x7fff0000ULL, 0x7fff0040ULL, 0x88005678ULL);
	host_arch_context(&context, &pc, &sp, &fp, &lr);
	EXPECT(pc == 0x88001234ULL);
	EXPECT(sp == 0x7fff0000ULL);
	EXPECT(fp == 0x7fff0040ULL);
#if defined(__aarch64__)
	EXPECT(lr == 0x88005678ULL);
#else
	EXPECT(lr == 0);
#endif

	for (count = 0; host_arch_register_line(&context, count, line, sizeof(line)); count++)
	{
		EXPECT(strlen(line) > 10 && strlen(line) < sizeof(line) - 1);
		strncat(dump, line, sizeof(dump) - strlen(dump) - 2);
		strcat(dump, "\n");
	}
	EXPECT(!host_arch_register_line(&context, -1, line, sizeof(line)));
	EXPECT(strstr(dump, "0000000088001234") != NULL);
	EXPECT(strstr(dump, "000000007fff0000") != NULL);
#if defined(__aarch64__)
	EXPECT(count == 9);
	EXPECT(strstr(dump, "x0  0000000000001000") != NULL);
	EXPECT(strstr(dump, "x28 000000000000101c") != NULL);
	EXPECT(strstr(dump, "0000000088005678") != NULL);
#else
	EXPECT(count == 5);
	EXPECT(strstr(dump, "rip 0000000088001234") != NULL);
	EXPECT(strstr(dump, "rbp 000000007fff0040") != NULL);
	EXPECT(strstr(dump, "r15 ") != NULL);
#endif
}

/* ---------- frames */

static uint64_t stack[64];

static int stack_read(uintptr_t address, void *buffer, size_t size)
{
	uintptr_t base = (uintptr_t)stack;

	if (!(address >= base && address + size <= base + sizeof(stack)))
		return 0;
	memcpy(buffer, (const void *)address, size);
	return 1;
}

static uint64_t at(int slot)
{
	return (uint64_t)(uintptr_t)&stack[slot];
}

static void test_frames(void)
{
	uint64_t returns[8];
	int count;

	/* three records going up the stack, the last ending the chain */
	memset(stack, 0, sizeof(stack));
	stack[4] = at(10);
	stack[5] = 0x88000100;
	stack[10] = at(20);
	stack[11] = 0x88000200;
	stack[20] = 0;
	stack[21] = 0x88000300;
	count = host_arch_walk_frames(at(4), stack_read, returns, 8);
	EXPECT(count == 3);
	EXPECT(returns[0] == 0x88000100 && returns[1] == 0x88000200 && returns[2] == 0x88000300);
	/* at most maximum */
	EXPECT(host_arch_walk_frames(at(4), stack_read, returns, 2) == 2);
	/* none from no frame pointer, a misaligned one, or one outside memory */
	EXPECT(host_arch_walk_frames(0, stack_read, returns, 8) == 0);
	EXPECT(host_arch_walk_frames(at(4) + 4, stack_read, returns, 8) == 0);
	EXPECT(host_arch_walk_frames(at(63), stack_read, returns, 8) == 0);
	/* a loop, or a chain going down the stack, ends the walk */
	stack[10] = at(4);
	EXPECT(host_arch_walk_frames(at(4), stack_read, returns, 8) == 2);
	stack[10] = at(10);
	EXPECT(host_arch_walk_frames(at(4), stack_read, returns, 8) == 2);
	/* a record pointing out of readable memory */
	stack[10] = at(63);
	EXPECT(host_arch_walk_frames(at(4), stack_read, returns, 8) == 2);
}

/* three pages: frame records in the first, the second PROT_NONE, the
third unmapped */
static uint8_t *pages;
static long page_size;

static void test_reader(void)
{
	uint64_t *records;
	uint64_t returns[8];
	uint64_t value = 0;

	page_size = sysconf(_SC_PAGESIZE);
	pages = mmap(NULL, 3 * page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	EXPECT(pages != MAP_FAILED);
	if (pages == MAP_FAILED)
		return;
	EXPECT(mprotect(pages + page_size, page_size, PROT_NONE) == 0);
	EXPECT(munmap(pages + 2 * page_size, page_size) == 0);
	records = (uint64_t *)pages;

	/* the reader: what is readable, and nothing else, without faulting */
	records[0] = 0x1122334455667788ULL;
	EXPECT(host_arch_read_memory((uintptr_t)records, &value, sizeof(value)) && value == 0x1122334455667788ULL);
	EXPECT(host_arch_read_memory((uintptr_t)records, &value, 0));
	EXPECT(!host_arch_read_memory((uintptr_t)(pages + page_size), &value, sizeof(value)));
	EXPECT(!host_arch_read_memory((uintptr_t)(pages + 2 * page_size), &value, sizeof(value)));
	/* (straddling the end of the readable page) */
	EXPECT(!host_arch_read_memory((uintptr_t)(pages + page_size - 8), returns, 16));
	EXPECT(!host_arch_read_memory(0, &value, sizeof(value)));
	EXPECT(!host_arch_read_memory(UINTPTR_MAX - 4, &value, sizeof(value)));

	/* a chain into the PROT_NONE page */
	records[0] = (uint64_t)(uintptr_t)&records[4];
	records[1] = 0x88000100;
	records[4] = (uint64_t)(uintptr_t)(pages + page_size + 64);
	records[5] = 0x88000200;
	EXPECT(host_arch_walk_frames((uintptr_t)records, host_arch_read_memory, returns, 8) == 2);
	EXPECT(returns[0] == 0x88000100 && returns[1] == 0x88000200);
	/* and into the unmapped one */
	records[4] = (uint64_t)(uintptr_t)(pages + 2 * page_size + 64);
	EXPECT(host_arch_walk_frames((uintptr_t)records, host_arch_read_memory, returns, 8) == 2);
	/* starting there, and from garbage */
	EXPECT(host_arch_walk_frames((uintptr_t)(pages + page_size), host_arch_read_memory, returns, 8) == 0);
	EXPECT(host_arch_walk_frames(0x00007ff0deadbee0ULL, host_arch_read_memory, returns, 8) == 0);
	EXPECT(host_arch_walk_frames(0xdeadbeefdeadbee0ULL, host_arch_read_memory, returns, 8) == 0);
	EXPECT(host_arch_walk_frames(UINT64_MAX - 7, host_arch_read_memory, returns, 8) == 0);
}

/* ---------- a real signal */

static volatile uint64_t signal_pc, signal_sp, signal_fp, signal_lr;
static volatile int signal_lines;

static void handler(int number, siginfo_t *information, void *context)
{
	char line[256];
	uint64_t pc, sp, fp, lr;

	(void)number;
	(void)information;
	host_arch_context(context, &pc, &sp, &fp, &lr);
	signal_pc = pc;
	signal_sp = sp;
	signal_fp = fp;
	signal_lr = lr;
	while (host_arch_register_line(context, signal_lines, line, sizeof(line)))
		signal_lines++;
}

static void test_signal(void)
{
	struct sigaction action;
	char here;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = handler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);
	sigaction(SIGUSR1, &action, NULL);
	raise(SIGUSR1);
	/* the interrupted code's stack: near a local of this function */
	{
		uint64_t local = (uint64_t)(uintptr_t)&here;
		uint64_t distance = local > signal_sp ? local - signal_sp : signal_sp - local;

		EXPECT(signal_sp && distance < 0x10000);
	}
	EXPECT(signal_pc != 0);
	EXPECT(signal_lines > 0);
}

/* the sampler's case (host_debug.c): a signal arriving while the frame
pointer register holds anything, as in code built without frame pointers;
the handler walks from it as the sampler does */
static volatile int walked = -1;
static volatile uint64_t walked_fp;

static void walking_handler(int number, siginfo_t *information, void *context)
{
	uint64_t pc, sp, fp, lr;
	uint64_t returns[12];
	int saved_errno = errno;

	(void)number;
	(void)information;
	host_arch_context(context, &pc, &sp, &fp, &lr);
	walked_fp = fp;
	walked = host_arch_walk_frames(fp, host_arch_read_memory, returns, 12);
	errno = saved_errno;
}

/* sends this thread signal number with the frame pointer register holding
fp, from the system call's return (where the signal is taken) */
static void signal_with_frame_pointer(uint64_t fp, int number)
{
	long pid = getpid(), tid = syscall(SYS_gettid);
#if defined(__x86_64__)
	register long r8 __asm__("r8") = (long)fp;
	long result = SYS_tgkill;

	__asm__ volatile(
		"sub $128, %%rsp\n\t" /* (past the red zone) */
		"push %%rbp\n\t"
		"mov %%r8, %%rbp\n\t"
		"syscall\n\t"
		"pop %%rbp\n\t"
		"add $128, %%rsp"
		: "+a"(result)
		: "D"(pid), "S"(tid), "d"((long)number), "r"(r8)
		: "rcx", "r11", "memory");
#elif defined(__aarch64__)
	register long x0 __asm__("x0") = pid;
	register long x1 __asm__("x1") = tid;
	register long x2 __asm__("x2") = number;
	register long x8 __asm__("x8") = SYS_tgkill;
	register long x9 __asm__("x9") = (long)fp;

	__asm__ volatile(
		"mov x19, x29\n\t"
		"mov x29, x9\n\t"
		"svc #0\n\t"
		"mov x29, x19"
		: "+r"(x0)
		: "r"(x1), "r"(x2), "r"(x8), "r"(x9)
		: "x19", "memory");
#endif
}

static void test_signal_garbage_frame_pointer(void)
{
	struct sigaction action;
	static const uint64_t garbage[] = { 0x00007ff0deadbee0ULL, 0xdeadbeefdeadbee0ULL, 0x10, 0 };
	int index;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = walking_handler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);
	sigaction(SIGUSR2, &action, NULL);
	for (index = 0; index < (int)(sizeof(garbage) / sizeof(garbage[0])); index++)
	{
		walked = -1;
		signal_with_frame_pointer(garbage[index], SIGUSR2);
		EXPECT(walked_fp == garbage[index]);
		EXPECT(walked == 0);
	}
	/* a frame pointer into the PROT_NONE page, and into the unmapped one */
	if (pages != MAP_FAILED && pages)
	{
		walked = -1;
		signal_with_frame_pointer((uintptr_t)(pages + page_size + 16), SIGUSR2);
		EXPECT(walked == 0);
		walked = -1;
		signal_with_frame_pointer((uintptr_t)(pages + 2 * page_size + 16), SIGUSR2);
		EXPECT(walked == 0);
		/* a record in readable memory pointing at the PROT_NONE page */
		((uint64_t *)pages)[8] = (uintptr_t)(pages + page_size + 32);
		((uint64_t *)pages)[9] = 0x88000300;
		walked = -1;
		signal_with_frame_pointer((uintptr_t)&((uint64_t *)pages)[8], SIGUSR2);
		EXPECT(walked == 1);
	}
}

int main(void)
{
	test_context();
	test_frames();
	test_reader();
	test_signal();
	test_signal_garbage_frame_pointer();
	if (failures)
		printf("%d failures\n", failures);
	else
		printf("host_arch: all passed\n");
	return failures != 0;
}
