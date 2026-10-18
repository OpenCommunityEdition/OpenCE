/* Spike: can an arm64 macOS process map the guest's fixed range below 4 GB?

The guest needs the Xbox memory at 0x80000000-0x88000000 and its image above
it, to 0x8c000000 (port/android/README.md, "The fixed addresses"). arm64
macOS executables normally have a 4 GB __PAGEZERO over that range. This
program prints how the range is held at start-up, tries each way to map it,
writes and reads its first and last pages, and prints PASS or FAIL. Build it
with and without -Wl,-pagezero_size,0x4000. */

#include <errno.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define GUEST_BASE 0x80000000ULL
#define GUEST_SIZE 0x0c000000ULL

static void print_region(mach_vm_address_t address)
{
	mach_vm_address_t region = address;
	mach_vm_size_t size = 0;
	vm_region_basic_info_data_64_t info;
	mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
	mach_port_t object = MACH_PORT_NULL;
	kern_return_t kr = mach_vm_region(mach_task_self(), &region, &size, VM_REGION_BASIC_INFO_64,
		(vm_region_info_t)&info, &count, &object);

	if (kr != KERN_SUCCESS)
		printf("region at 0x%llx: none above it (%s)\n", address, mach_error_string(kr));
	else if (region > address)
		printf("region at 0x%llx: free; next region 0x%llx-0x%llx\n", address, region, region + size);
	else
		printf("region at 0x%llx: 0x%llx-0x%llx prot %d/%d\n", address, region, region + size,
			info.protection, info.max_protection);
}

static int touch(void)
{
	volatile uint32_t *first = (volatile uint32_t *)(uintptr_t)GUEST_BASE;
	volatile uint32_t *last = (volatile uint32_t *)(uintptr_t)(GUEST_BASE + GUEST_SIZE - sizeof(uint32_t));

	*first = 0x12345678u;
	*last = 0x9abcdef0u;
	return *first == 0x12345678u && *last == 0x9abcdef0u;
}

static int try_mmap(void)
{
	void *p = mmap((void *)(uintptr_t)GUEST_BASE, GUEST_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

	if (p == MAP_FAILED) {
		printf("mmap with a hint: failed (%s)\n", strerror(errno));
		return 0;
	}
	if ((uintptr_t)p != GUEST_BASE) {
		printf("mmap with a hint: placed at %p instead\n", p);
		munmap(p, GUEST_SIZE);
		return 0;
	}
	printf("mmap with a hint: placed at the fixed address\n");
	return 1;
}

static int try_vm_allocate(void)
{
	mach_vm_address_t address = GUEST_BASE;
	kern_return_t kr = mach_vm_allocate(mach_task_self(), &address, GUEST_SIZE, VM_FLAGS_FIXED);

	printf("mach_vm_allocate fixed: %s\n", mach_error_string(kr));
	return kr == KERN_SUCCESS;
}

static int try_free_then_allocate(void)
{
	kern_return_t kr = mach_vm_deallocate(mach_task_self(), GUEST_BASE, GUEST_SIZE);

	printf("mach_vm_deallocate of the range: %s\n", mach_error_string(kr));
	if (kr != KERN_SUCCESS)
		return 0;
	print_region(GUEST_BASE);
	return try_vm_allocate();
}

int main(void)
{
	const char *way = NULL;

	print_region(0);
	print_region(GUEST_BASE);

	if (try_mmap())
		way = "mmap with a hint";
	else if (try_vm_allocate())
		way = "mach_vm_allocate fixed";
	else if (try_free_then_allocate())
		way = "free the range, then mach_vm_allocate fixed";

	if (!way) {
		printf("FAIL: no way to map 0x%llx-0x%llx\n", GUEST_BASE, GUEST_BASE + GUEST_SIZE);
		return 1;
	}
	print_region(GUEST_BASE);
	if (!touch()) {
		printf("FAIL: mapped by \"%s\", but the pages do not hold what was written\n", way);
		return 1;
	}
	printf("mapped by: %s\n", way);
	printf("PASS\n");
	return 0;
}
