/*
THUNK_TEST.C

Tests the Windows x64 host's way into the guest and back
(port/windows/host/win64_switch.S, and the thunks tools/windows_host_thunks.py
generates from thunk_test.h), as a Windows program: tools/test_windows_host_thunks.py
builds it and runs it under Wine. "Guest" code here is System V code
(sysv_abi) on a stack below 4 GB, as the x32 guest's is; it calls the host
functions below (Windows' convention) through their stubs, and checks:

- every argument arrives, integers in registers and on the stack, floats
  and doubles in registers and on the stack, in any mix, and a pointer;
- the host function runs on the thread's own stack with the TEB's limits
  of that stack, and the guest's code on its stack with its limits;
- a host function can call guest code again (nested switches);
- an exception raised in a host function is caught by an __except in the
  host code that entered the guest (Windows unwinds through the thunk and
  host_enter_guest), and a stack walk from a host function reaches main;
- a fault in guest code (a write to a read-only page, as the write tracking
  makes them, win64_memory.c) reaches a vectored exception handler, which
  makes the page writable and lets the guest go on.
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xmmintrin.h>

#include "thunk_test.h"

/* win64_thread.c's, which the assembly reads */
struct host_switch
{
	uint64_t host_sp;
	uint64_t guest_sp;
	uint64_t host_base, host_limit;
	uint64_t guest_base, guest_limit;
	uint64_t host_deallocation, guest_deallocation;
};

uint32_t host_switch_tls_offset;
uint32_t host_enter_guest(uint64_t function, uint64_t a, uint64_t b, uint64_t c, uint64_t d);
void *host_resolve_import(const char *name);
void host_thunk_unresolved(void)
{
}

#define GUEST_STACK_BASE 0x60000000u
#define GUEST_STACK_SIZE 0x100000u

static struct host_switch context;
static int failures;

#define CHECK(condition) \
	do { if (!(condition)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); failures++; } } while (0)

static int on_guest_stack(void)
{
	volatile char probe = 0;
	uintptr_t address = (uintptr_t)&probe;

	return address >= GUEST_STACK_BASE && address < GUEST_STACK_BASE + GUEST_STACK_SIZE;
}

static void check_host_side(void)
{
	NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
	volatile char probe = 0;

	CHECK(!on_guest_stack());
	CHECK((char *)&probe < (char *)tib->StackBase && (char *)&probe >= (char *)tib->StackLimit);
}

static void check_guest_side(void)
{
	NT_TIB *tib = (NT_TIB *)NtCurrentTeb();

	CHECK(on_guest_stack());
	CHECK((uintptr_t)tib->StackBase == GUEST_STACK_BASE + GUEST_STACK_SIZE);
	CHECK((uintptr_t)tib->StackLimit == GUEST_STACK_BASE + 0x10000);
}

/* ---------- the host functions */

long long host_test_integers(unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5,
	unsigned int a6, unsigned int a7, unsigned int a8, long long a9, unsigned int a10, unsigned int a11,
	unsigned int a12, unsigned int a13, unsigned int a14, unsigned int a15)
{
	check_host_side();
	CHECK(a1 == 1 && a2 == 2 && a3 == 3 && a4 == 4 && a5 == 5 && a6 == 6 && a7 == 0x80000007u);
	CHECK(a8 == 8 && a9 == -0x123456789LL && a10 == 10 && a11 == 11 && a12 == 12 && a13 == 13 && a14 == 14);
	CHECK(a15 == 0xfffffff5u);
	return (long long)a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12 + a13 + a14 + a15;
}

double host_test_mixed(int a, float b, int c, double d, int e, float f, int g, int h, int i, float j)
{
	check_host_side();
	CHECK(a == -1 && b == 2.5f && c == 3 && d == 4.25 && e == 5 && f == -6.5f && g == 7 && h == 8 && i == 9);
	CHECK(j == 10.75f);
	return a + b + c + d + e + f + g + h + i + j;
}

double host_test_floats(double x1, double x2, double x3, double x4, double x5, double x6, double x7, double x8,
	double x9, float x10)
{
	check_host_side();
	CHECK(x1 == 1.0 && x2 == 2.0 && x3 == 3.0 && x4 == 4.0 && x5 == 5.0 && x6 == 6.0 && x7 == 7.0 && x8 == 8.0);
	CHECK(x9 == 9.5 && x10 == 10.5f);
	return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10;
}

float host_test_pointer(const char *text, float scale)
{
	check_host_side();
	CHECK((uintptr_t)text < 0x100000000ull && !strcmp(text, "guest text"));
	return (float)strlen(text) * scale;
}

typedef uint32_t (__attribute__((sysv_abi)) *guest_entry)(uint64_t, uint64_t, uint64_t, uint64_t);
static uint32_t __attribute__((sysv_abi)) guest_nested(uint64_t depth, uint64_t b, uint64_t c, uint64_t d);

int host_test_nested(int depth)
{
	check_host_side();
	if (depth <= 0)
		return 1000;
	/* into the guest again, below the guest frames already there */
	return 1 + (int)host_enter_guest((uint64_t)(uintptr_t)guest_nested, (uint64_t)(depth - 1), 0, 0, 0);
}

void host_test_raise(void)
{
	check_host_side();
	RaiseException(0xE0000001u, 0, 0, NULL);
}

int main(int argc, char **argv);

int host_test_unwind(void)
{
	void *frames[32];
	USHORT count = RtlCaptureStackBackTrace(0, 32, frames, NULL);
	ULONG64 base = 0;
	PRUNTIME_FUNCTION main_entry = RtlLookupFunctionEntry((ULONG64)(uintptr_t)main, &base, NULL);
	int index, reached = 0;

	check_host_side();
	for (index = 0; index < count; index++)
	{
		ULONG64 frame_base = 0;
		PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry((ULONG64)(uintptr_t)frames[index], &frame_base, NULL);

		if (entry && main_entry && entry->BeginAddress == main_entry->BeginAddress)
			reached = 1;
	}
	CHECK(reached);
	return reached;
}

/* ---------- the guest's code */

typedef long long (__attribute__((sysv_abi)) *integers_import)(unsigned int, unsigned int, unsigned int,
	unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, long long, unsigned int, unsigned int,
	unsigned int, unsigned int, unsigned int, unsigned int);
typedef double (__attribute__((sysv_abi)) *mixed_import)(int, float, int, double, int, float, int, int, int, float);
typedef double (__attribute__((sysv_abi)) *floats_import)(double, double, double, double, double, double, double,
	double, double, float);
typedef float (__attribute__((sysv_abi)) *pointer_import)(const char *, float);
typedef int (__attribute__((sysv_abi)) *int_import)(int);
typedef void (__attribute__((sysv_abi)) *void_import)(void);
typedef int (__attribute__((sysv_abi)) *unwind_import)(void);

static uint32_t __attribute__((sysv_abi)) guest_nested(uint64_t depth, uint64_t b, uint64_t c, uint64_t d)
{
	int_import nested = (int_import)host_resolve_import("host_test_nested");

	(void)b; (void)c; (void)d;
	check_guest_side();
	return (uint32_t)nested((int)depth);
}

static uint32_t __attribute__((sysv_abi)) guest_main(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
	integers_import integers = (integers_import)host_resolve_import("host_test_integers");
	mixed_import mixed = (mixed_import)host_resolve_import("host_test_mixed");
	floats_import floats = (floats_import)host_resolve_import("host_test_floats");
	pointer_import pointer = (pointer_import)host_resolve_import("host_test_pointer");
	int_import nested = (int_import)host_resolve_import("host_test_nested");
	unwind_import unwind = (unwind_import)host_resolve_import("host_test_unwind");
	volatile __m128 keep = _mm_set1_ps(3.0f);
	/* (on the guest's stack, below 4 GB, as the guest's memory is) */
	char guest_text[16];

	check_guest_side();
	CHECK(a == 11 && b == 22 && c == 33 && d == 0xfffffff4u);
	CHECK(integers(1, 2, 3, 4, 5, 6, 0x80000007u, 8, -0x123456789LL, 10, 11, 12, 13, 14, 0xfffffff5u) ==
		1 + 2 + 3 + 4 + 5 + 6 + 0x80000007LL + 8 - 0x123456789LL + 10 + 11 + 12 + 13 + 14 + 0xfffffff5LL);
	check_guest_side();
	CHECK(mixed(-1, 2.5f, 3, 4.25, 5, -6.5f, 7, 8, 9, 10.75f) == -1 + 2.5 + 3 + 4.25 + 5 - 6.5 + 7 + 8 + 9 + 10.75);
	CHECK(floats(1, 2, 3, 4, 5, 6, 7, 8, 9.5, 10.5f) == 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9.5 + 10.5);
	strcpy(guest_text, "guest text");
	CHECK(pointer(guest_text, 0.5f) == 5.0f);
	CHECK(nested(5) == 1005);
	CHECK(unwind() == 1);
	check_guest_side();
	CHECK(_mm_cvtss_f32(keep) == 3.0f);
	return 0x1234;
}

#define WATCHED_PAGE 0x60200000u

static volatile LONG watch_faults;

static LONG CALLBACK watch_handler(EXCEPTION_POINTERS *exception)
{
	const EXCEPTION_RECORD *record = exception->ExceptionRecord;
	DWORD old;

	if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2 &&
		record->ExceptionInformation[0] == 1 && (record->ExceptionInformation[1] & ~0xfffull) == WATCHED_PAGE)
	{
		InterlockedIncrement(&watch_faults);
		VirtualProtect((void *)(uintptr_t)WATCHED_PAGE, 0x1000, PAGE_READWRITE, &old);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static uint32_t __attribute__((sysv_abi)) guest_write(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
	volatile uint32_t *word = (volatile uint32_t *)(uintptr_t)(WATCHED_PAGE + 0x10);

	(void)a; (void)b; (void)c; (void)d;
	check_guest_side();
	*word = 0x5a5a5a5a;
	check_guest_side();
	return *word;
}

static uint32_t __attribute__((sysv_abi)) guest_raise(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
	void_import raise = (void_import)host_resolve_import("host_test_raise");

	(void)a; (void)b; (void)c; (void)d;
	raise();
	return 0;
}

int main(int argc, char **argv)
{
	DWORD slot = TlsAlloc();
	ULONG_PTR low = 0, high = 0;
	char *stack = VirtualAlloc((void *)(uintptr_t)GUEST_STACK_BASE, GUEST_STACK_SIZE, MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);
	volatile double kept = 1.5;
	int caught = 0;

	(void)argc;
	(void)argv;
	if (slot == TLS_OUT_OF_INDEXES || slot >= 64 || stack != (char *)(uintptr_t)GUEST_STACK_BASE)
	{
		printf("FAIL: setup (slot %lu, stack %p)\n", (unsigned long)slot, (void *)stack);
		return 2;
	}
	host_switch_tls_offset = 0x1480 + 8 * slot;
	GetCurrentThreadStackLimits(&low, &high);
	context.guest_base = GUEST_STACK_BASE + GUEST_STACK_SIZE;
	context.guest_limit = GUEST_STACK_BASE + 0x10000;
	context.guest_deallocation = GUEST_STACK_BASE;
	context.guest_sp = context.guest_base - 0x100;
	TlsSetValue(slot, &context);

	CHECK(host_enter_guest((uint64_t)(uintptr_t)guest_main, 11, 22, 33, 0xfffffff4u) == 0x1234);
	check_host_side();
	CHECK(kept == 1.5);
	CHECK(context.guest_sp == context.guest_base - 0x100);

	__try
	{
		host_enter_guest((uint64_t)(uintptr_t)guest_raise, 0, 0, 0, 0);
	}
	__except (GetExceptionCode() == 0xE0000001u ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		caught = 1;
	}
	CHECK(caught);
	check_host_side();
	/* (the __except left the switch's state at the abandoned call) */
	context.guest_sp = context.guest_base - 0x100;

	CHECK(VirtualAlloc((void *)(uintptr_t)WATCHED_PAGE, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READONLY) ==
		(void *)(uintptr_t)WATCHED_PAGE);
	AddVectoredExceptionHandler(1, watch_handler);
	CHECK(host_enter_guest((uint64_t)(uintptr_t)guest_write, 0, 0, 0, 0) == 0x5a5a5a5au);
	CHECK(watch_faults == 1);
	check_host_side();

	printf(failures ? "%d failures\n" : "thunk test passed\n", failures);
	return failures ? 1 : 0;
}
