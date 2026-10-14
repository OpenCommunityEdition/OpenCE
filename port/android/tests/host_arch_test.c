/*
HOST_ARCH_TEST.C

Tests of port/android/host/host_arch.c on the build machine's processor
(AArch64 or x86-64): the registers read from a ucontext_t made here, the
register dump, and the frame walker on frame records made here, and on a
real signal's context. Built and run by tools/test_host_units.py; exits
nonzero on a failure.
*/

#include "host.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>

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

static int stack_readable(uintptr_t address, size_t size)
{
	uintptr_t base = (uintptr_t)stack;

	return address >= base && address + size <= base + sizeof(stack);
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
	count = host_arch_walk_frames(at(4), stack_readable, returns, 8);
	EXPECT(count == 3);
	EXPECT(returns[0] == 0x88000100 && returns[1] == 0x88000200 && returns[2] == 0x88000300);
	/* at most maximum */
	EXPECT(host_arch_walk_frames(at(4), stack_readable, returns, 2) == 2);
	/* none from no frame pointer, a misaligned one, or one outside memory */
	EXPECT(host_arch_walk_frames(0, stack_readable, returns, 8) == 0);
	EXPECT(host_arch_walk_frames(at(4) + 4, stack_readable, returns, 8) == 0);
	EXPECT(host_arch_walk_frames(at(63), stack_readable, returns, 8) == 0);
	/* a loop, or a chain going down the stack, ends the walk */
	stack[10] = at(4);
	EXPECT(host_arch_walk_frames(at(4), stack_readable, returns, 8) == 2);
	stack[10] = at(10);
	EXPECT(host_arch_walk_frames(at(4), stack_readable, returns, 8) == 2);
	/* a record pointing out of readable memory */
	stack[10] = at(63);
	EXPECT(host_arch_walk_frames(at(4), stack_readable, returns, 8) == 2);
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

int main(void)
{
	test_context();
	test_frames();
	test_signal();
	if (failures)
		printf("%d failures\n", failures);
	else
		printf("host_arch: all passed\n");
	return failures != 0;
}
