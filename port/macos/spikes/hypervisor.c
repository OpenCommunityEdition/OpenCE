/* Spike: can the guest run under Hypervisor.framework at its fixed address?

A process on arm64 macOS cannot map below 4 GB (low_memory.c), so the guest
cannot live in the host's own address space. A VM has an address space of its
own. This program maps a page of host memory at guest physical address
0x80000000, runs two instructions there at EL1 with the MMU off (so the
address the code sees is the physical address), takes the hvc as an exit back
to the host, and times that round trip, which every host import would cost.

Needs the com.apple.security.hypervisor entitlement (hypervisor.entitlements),
signed ad hoc: codesign -s - --entitlements hypervisor.entitlements <exe> */

#include <Hypervisor/Hypervisor.h>
#include <mach/mach_time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define GUEST_BASE 0x80000000ULL
#define PAGE 0x4000
#define EXITS 100000
#define EC_HVC64 0x16

static const char *status_name(hv_return_t r)
{
	switch (r) {
	case HV_SUCCESS: return "success";
	case HV_DENIED: return "denied (entitlement missing?)";
	case HV_UNSUPPORTED: return "unsupported";
	case HV_NO_RESOURCES: return "no resources";
	case HV_BAD_ARGUMENT: return "bad argument";
	case HV_BUSY: return "busy";
	default: return "error";
	}
}

int main(void)
{
	/* add w0, w0, #1; hvc #0 */
	static const uint32_t code[] = { 0x11000400u, 0xd4000002u };
	mach_timebase_info_data_t timebase;
	hv_vcpu_exit_t *exit_info;
	hv_vcpu_t vcpu;
	hv_return_t r;
	uint64_t start, ns, x0 = 0;
	uint32_t ec;
	void *memory;
	int i;

	memory = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (memory == MAP_FAILED) {
		printf("FAIL: mmap\n");
		return 1;
	}
	memcpy(memory, code, sizeof(code));

	r = hv_vm_create(NULL);
	printf("hv_vm_create: %s\n", status_name(r));
	if (r != HV_SUCCESS) {
		printf("FAIL: no VM\n");
		return 1;
	}
	r = hv_vm_map(memory, GUEST_BASE, PAGE, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
	printf("hv_vm_map at 0x%llx: %s\n", GUEST_BASE, status_name(r));
	r |= hv_vcpu_create(&vcpu, &exit_info, NULL);
	r |= hv_vcpu_set_reg(vcpu, HV_REG_CPSR, 0x3c5); /* EL1h, interrupts masked */
	r |= hv_vcpu_set_reg(vcpu, HV_REG_X0, 0);
	if (r != HV_SUCCESS) {
		printf("FAIL: vCPU set-up\n");
		return 1;
	}

	mach_timebase_info(&timebase);
	start = mach_absolute_time();
	for (i = 0; i < EXITS; i++) {
		hv_vcpu_set_reg(vcpu, HV_REG_PC, GUEST_BASE);
		r = hv_vcpu_run(vcpu);
		ec = (uint32_t)(exit_info->exception.syndrome >> 26) & 0x3f;
		if (r != HV_SUCCESS || exit_info->reason != HV_EXIT_REASON_EXCEPTION || ec != EC_HVC64) {
			printf("FAIL: exit %d: run %s, reason %u, EC 0x%x\n", i, status_name(r), exit_info->reason, ec);
			return 1;
		}
	}
	ns = (mach_absolute_time() - start) * timebase.numer / timebase.denom;
	hv_vcpu_get_reg(vcpu, HV_REG_X0, &x0);

	printf("guest code at 0x%llx ran %d times; x0 = %llu\n", GUEST_BASE, EXITS, x0);
	printf("round trip guest -> host -> guest: %.0f ns\n", (double)ns / EXITS);
	if (x0 != EXITS) {
		printf("FAIL: x0 should be %d\n", EXITS);
		return 1;
	}
	hv_vcpu_destroy(vcpu);
	hv_vm_destroy();
	printf("PASS\n");
	return 0;
}
