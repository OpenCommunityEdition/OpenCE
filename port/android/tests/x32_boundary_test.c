/*
X32_BOUNDARY_TEST.C

A test of the call boundary between the x32 guest and the x86-64 host
(port/android/include/halo_android_abi.h): the host's functions read the
guest's pointer arguments as 64-bit registers, so the guest must pass them
zero-extended, whatever the registers held before. x32_boundary_guest.c,
linked into a flat image at IMAGE_BASE with the guest's import stubs
(tools/guest_arch.py), is called here as host_call_guest calls guest code,
on a stack below 4 GB, with integer arguments whose registers' upper halves
are garbage; the host functions it imports check what they are given. The
data the pointers point at, and the stack, lie above 2 GB, as the Xbox
window and the pools do: there a sign-extended pointer differs from a
zero-extended one.
tools/test_host_units.py builds and runs it on x86-64; exits nonzero on a
failure.

Usage: x32_boundary_test image.bin entry_offset table_offset
*/

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define IMAGE_BASE 0x60000000ULL
#define STACK_BASE 0x91000000ULL
#define STACK_SIZE 0x100000ULL
#define DATA_BASE 0x90100000ULL
#define DATA ((char *)(uintptr_t)DATA_BASE)

static int failures;
static uint64_t entry;

static void expect(int condition, const char *what)
{
	if (!condition)
	{
		printf("%s\n", what);
		failures++;
	}
}

/* (the arguments as the registers hold them) */
int host_check(uint64_t from_integer, uint64_t sum, uint64_t passed_on, uint64_t difference, uint64_t integer,
	uint64_t wide)
{
	expect(from_integer == 0x1234, "a pointer from an integer is not zero-extended");
	expect(sum == (uintptr_t)DATA + 16, "a pointer sum is not zero-extended");
	expect(passed_on == (uintptr_t)DATA, "a pointer argument passed on is not zero-extended");
	expect(difference == (uintptr_t)DATA - 16, "a pointer difference is not zero-extended");
	expect((uint32_t)integer == 0x1234, "a 32-bit integer's lower half is wrong");
	expect(wide == 0x1234, "a 64-bit integer is wrong");
	return 1;
}

char *host_pointer(void)
{
	return DATA + 0x100;
}

int host_check_returned(uint64_t returned)
{
	expect(returned == (uintptr_t)DATA + 0x101, "a pointer the host returned is not zero-extended when passed back");
	return 1;
}

typedef uint32_t (*guest_function)(uint64_t, uint64_t, uint64_t);

static void *run(void *unused)
{
	(void)unused;
	/* garbage in the integer arguments' upper halves, as a 32-bit argument
	of the host's may leave; a pointer the host passes zero-extended */
	if (((guest_function)(uintptr_t)entry)(0xdeadbeef00001234ULL, 0xfeedface00000010ULL, (uintptr_t)DATA) != 0x101)
		expect(0, "the guest's result is wrong");
	return NULL;
}

int main(int argc, char **argv)
{
	FILE *file;
	char *image;
	long size;
	pthread_attr_t attributes;
	pthread_t thread;
	void **table;
	void *stack, *data;

	if (argc != 4)
		return 2;
	file = fopen(argv[1], "rb");
	if (!file)
		return 2;
	image = mmap((void *)(uintptr_t)IMAGE_BASE, 0x200000, PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	stack = mmap((void *)(uintptr_t)STACK_BASE, STACK_SIZE, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	data = mmap((void *)(uintptr_t)DATA_BASE, 0x1000, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	if (image != (void *)(uintptr_t)IMAGE_BASE || stack != (void *)(uintptr_t)STACK_BASE ||
		data != (void *)(uintptr_t)DATA_BASE)
	{
		printf("cannot map below 4 GB\n");
		return 2;
	}
	size = (long)fread(image, 1, 0x100000, file);
	fclose(file);
	if (size <= 0)
		return 2;
	entry = IMAGE_BASE + strtoull(argv[2], NULL, 0);
	/* the import table, in the order of the stubs (tools/test_host_units.py) */
	table = (void **)(image + strtoull(argv[3], NULL, 0));
	table[0] = (void *)host_check;
	table[1] = (void *)host_pointer;
	table[2] = (void *)host_check_returned;

	pthread_attr_init(&attributes);
	pthread_attr_setstack(&attributes, stack, STACK_SIZE);
	if (pthread_create(&thread, &attributes, run, NULL) != 0)
		return 2;
	pthread_join(thread, NULL);
	if (!failures)
		printf("ok\n");
	return failures ? 1 : 0;
}
