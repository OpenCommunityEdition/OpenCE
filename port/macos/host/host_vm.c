/*
HOST_VM.C

The virtual machine that runs the guest (see host.h for the memory map).

One Hypervisor.framework VM per process. The stage-2 tables map every
guest page the guest may touch to ordinary host memory, identity-style
(guest address == guest IPA). The guest runs at EL1 with its stage-1 MMU
off: its 32-bit addresses are physical from the hypervisor's point of
view, so there is no page-table work on the guest's side at all, and the
host reaches every guest byte directly. A mirror table (one entry per
16 KiB guest page: the host address of its backing memory, NULL when the
page is not mapped) translates guest pointers for the host's own access.

The vCPU starts with the registers a bare ARM machine would have (SCTLR
with the MMU off, FP enabled, timers masked), and the guest's own code
expects exactly that: its musl runtime is freestanding, and the port's
memory_watch uses SIGSEGV, not MMU control.

Guest calls arrive as data-abort exits on the doorbell page
(tools/macos_imports.py): the faulting IPA names the import; the host
transfers the guest's registers into the import's C function, writes the
result back and advances the guest PC past the doorbell store. Host calls
into the guest aim the link register at a doorbell store of its own; the
return store brings the vCPU back here.

Each thread that runs guest code owns one vCPU (hypervisor vCPUs are
bound to their creating thread).
*/

#include "host.h"

#include <Hypervisor/Hypervisor.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* ---------- the stage-2 mirror */

#define MIRROR_PAGES (GUEST_LOW_LIMIT / GUEST_PAGE_SIZE)
#define MIRROR_INDEX(address) ((address) / GUEST_PAGE_SIZE)

static uint8_t *mirror[MIRROR_PAGES];
static pthread_mutex_t vm_lock = PTHREAD_MUTEX_INITIALIZER;
static int vm_ready;

/* the doorbell page: mapped nowhere, so its stores exit (one word per
import; the stubs also store to DOORBELL_RETURN for host-to-guest calls) */
#define DOORBELL_RETURN (GUEST_DOORBELL_BASE + GUEST_DOORBELL_SIZE - 4)

/* ---------- low memory pools */

struct pool_region
{
	uint8_t *host;    /* the host memory backing the region */
	uint32_t base;    /* guest address */
	uint32_t size;
	uint8_t *used;    /* one byte per page */
	uint32_t pages;
	uint32_t free_pages;
};

#define POOL_COUNT 6

static struct pool_region *pools[POOL_COUNT];
static int pool_count;
static uint32_t pool_next = GUEST_POOLS_BASE;

/* sizes of the regions, largest first so that big allocations do not
fragment the small ones */
static const uint32_t pool_sizes[POOL_COUNT] = {
	0x10000000, /* 256 MB */
	0x08000000, /* 128 MB */
	0x04000000, /* 64 MB */
	0x04000000, /* 64 MB */
	0x02000000, /* 32 MB */
	0x02000000, /* 32 MB */
};

static uint32_t round_page(uint32_t size)
{
	return (size + GUEST_PAGE_SIZE - 1) & ~(GUEST_PAGE_SIZE - 1);
}

static struct pool_region *pool_create(uint32_t size)
{
	struct pool_region *pool = calloc(1, sizeof(*pool));

	if (!pool)
		return NULL;
	pool->size = round_page(size);
	pool->host = mmap(NULL, pool->size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (pool->host == MAP_FAILED)
	{
		free(pool);
		return NULL;
	}
	if (hv_vm_map(pool->host, pool_next, pool->size,
		HV_MEMORY_READ | HV_MEMORY_WRITE) != HV_SUCCESS)
	{
		munmap(pool->host, pool->size);
		free(pool);
		return NULL;
	}
	pool->base = pool_next;
	pool_next += pool->size;
	pool->pages = pool->size / GUEST_PAGE_SIZE;
	pool->free_pages = pool->pages;
	pool->used = calloc(pool->pages, 1);
	if (!pool->used)
	{
		hv_vm_unmap(pool->base, pool->size);
		munmap(pool->host, pool->size);
		free(pool);
		return NULL;
	}
	return pool;
}

void *host_low_map(size_t request)
{
	uint32_t pages = round_page((uint32_t)request) / GUEST_PAGE_SIZE;
	void *result = NULL;
	int index;

	if (!pages)
		return NULL;
	pthread_mutex_lock(&vm_lock);
	for (index = 0; index < pool_count && !result; index++)
	{
		struct pool_region *pool = pools[index];
		uint32_t run = 0, page;

		if (pool->free_pages < pages)
			continue;
		for (page = 0; page < pool->pages; page++)
		{
			if (pool->used[page])
			{
				run = 0;
				continue;
			}
			if (++run < pages)
				continue;
			run = page + 1 - pages;
			memset(&pool->used[run], 1, pages);
			pool->free_pages -= pages;
			{
				uint32_t guest = pool->base + run * GUEST_PAGE_SIZE;

				for (page = 0; page < pages; page++)
					mirror[MIRROR_INDEX(guest) + page] = pool->host + (run + page) * GUEST_PAGE_SIZE;
			}
			result = pool->host + run * GUEST_PAGE_SIZE;
			break;
		}
	}
	if (!result && pool_count < POOL_COUNT)
	{
		uint32_t want = round_page((uint32_t)request);
		uint32_t size = want * 2 > pool_sizes[pool_count] ? want * 2 : pool_sizes[pool_count];

		if (pool_next + size > GUEST_POOLS_BASE + GUEST_POOLS_SIZE)
			size = GUEST_POOLS_BASE + GUEST_POOLS_SIZE - pool_next;
		if (size >= want)
		{
			struct pool_region *pool = pool_create(size);

			if (pool)
			{
				pools[pool_count++] = pool;
				memset(pool->used, 1, pages);
				pool->free_pages -= pages;
				{
					uint32_t page;

					for (page = 0; page < pages; page++)
						mirror[MIRROR_INDEX(pool->base) + page] = pool->host + page * GUEST_PAGE_SIZE;
				}
				result = pool->host;
			}
		}
	}
	pthread_mutex_unlock(&vm_lock);
	return result;
}

static struct pool_region *pool_of(uint32_t address, size_t size)
{
	int index;

	for (index = 0; index < pool_count; index++)
	{
		struct pool_region *pool = pools[index];

		if (address >= pool->base && address - pool->base <= pool->size &&
			size <= pool->size - (address - pool->base))
			return pool;
	}
	return NULL;
}

void host_low_unmap(void *pointer, size_t request)
{
	uintptr_t host = (uintptr_t)pointer;
	int index;

	pthread_mutex_lock(&vm_lock);
	for (index = 0; index < pool_count; index++)
	{
		struct pool_region *pool = pools[index];
		uintptr_t start = (uintptr_t)pool->host;
		uintptr_t end = start + (uintptr_t)pool->size;

		if (host < start || host + request > end)
			continue;
		{
			uint32_t first = (uint32_t)((host - start) / GUEST_PAGE_SIZE);
			uint32_t guest = pool->base + first * GUEST_PAGE_SIZE;
			uint32_t pages = round_page((uint32_t)request) / GUEST_PAGE_SIZE;
			uint32_t page;

			mmap((void *)(start + (size_t)first * GUEST_PAGE_SIZE), (size_t)pages * GUEST_PAGE_SIZE,
				PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
			for (page = 0; page < pages; page++)
			{
				uint32_t mirror_index = MIRROR_INDEX(guest) + page;

				if (mirror[mirror_index] && pool->used[first + page])
				{
					pool->used[first + page] = 0;
					pool->free_pages++;
				}
				mirror[mirror_index] = NULL;
			}
		}
		break;
	}
	pthread_mutex_unlock(&vm_lock);
}

int host_low_owns(uintptr_t host, size_t size)
{
	int result;

	if (!size)
		return 1;
	if (host >= GUEST_WINDOW_BASE && host < GUEST_WINDOW_BASE + GUEST_WINDOW_SIZE)
		return size <= GUEST_WINDOW_SIZE - (host - GUEST_WINDOW_BASE);
	if (host >= GUEST_POOLS_BASE && host + size <= GUEST_POOLS_BASE + GUEST_POOLS_SIZE)
	{
		uint32_t base = (uint32_t)host;
		uint32_t offset;

		pthread_mutex_lock(&vm_lock);
		result = pool_of(base, size) != NULL;
		for (offset = 0; result && offset < size; offset += GUEST_PAGE_SIZE)
			result = mirror[MIRROR_INDEX(base) + offset] != NULL;
		pthread_mutex_unlock(&vm_lock);
		return result;
	}
	return 0;
}

static inline uint8_t *host_pointer(uint32_t guest)
{
	return mirror[MIRROR_INDEX(guest)];
}

/* ---------- the VM */

/* sizes the hypervisor demands (this machine's page size) */
static uint32_t machine_page;

static int map_fixed(void *host, uint32_t guest, uint32_t size, hv_memory_flags_t flags)
{
	return hv_vm_map(host, guest, size, flags) == HV_SUCCESS ? 0 : -1;
}

int host_vm_start(void)
{
	machine_page = (uint32_t)sysconf(_SC_PAGESIZE);
	/* (the hypervisor maps 16 KiB at least; this machine's page size is
	16 KiB, which the build asserts) */
	if (hv_vm_create(NULL) != HV_SUCCESS)
	{
		host_logf(HOST_LOG_ERROR, "hv_vm_create failed (is the com.apple.security.hypervisor "
			"entitlement set?)");
		return -1;
	}
	vm_ready = 1;
	/* the doorbell page and the pools are mapped as they are allocated */
	return 0;
}

int host_vm_map_window(void)
{
	/* the Xbox window: one mapping, lazily backed by the kernel (the
	guest zeroes or streams into it) */
	static uint8_t *window;

	if (!window)
	{
		window = mmap(NULL, GUEST_WINDOW_SIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
		if (window == MAP_FAILED)
			return -1;
		if (map_fixed(window, GUEST_WINDOW_BASE, GUEST_WINDOW_SIZE,
			HV_MEMORY_READ | HV_MEMORY_WRITE) != 0)
			return -1;
		{
			uint32_t page;

			for (page = 0; page < GUEST_WINDOW_SIZE / GUEST_PAGE_SIZE; page++)
				mirror[MIRROR_INDEX(GUEST_WINDOW_BASE) + page] = window + page * GUEST_PAGE_SIZE;
		}
	}
	return 0;
}

/* ---------- vCPUs */

struct host_vcpu
{
	hv_vcpu_t vcpu;
	hv_vcpu_exit_t *exit;
	int attached;
	/* where a host-to-guest call returns (the doorbell address the
	caller planted), and whether one is under way */
	uint32_t return_doorbell;
	uint64_t pc_after_exit;
	uint64_t pc_after_return;
	int in_call;
	/* the guest stack of this thread (host_thread.c) */
	uint32_t stack_top;
};

static pthread_key_t vcpu_key;
static int vcpu_key_made;

static void vcpu_destroy(void *context)
{
	struct host_vcpu *vcpu = context;

	if (!vcpu)
		return;
	hv_vcpu_destroy(vcpu->vcpu);
	free(vcpu);
}

static struct host_vcpu *vcpu_of(void)
{
	return pthread_getspecific(vcpu_key);
}

int host_vcpu_attach(void)
{
	struct host_vcpu *vcpu;

	if (!vcpu_key_made)
	{
		pthread_key_create(&vcpu_key, vcpu_destroy);
		vcpu_key_made = 1;
	}
	vcpu = calloc(1, sizeof(*vcpu));
	if (!vcpu)
		return -1;
	if (hv_vcpu_create(&vcpu->vcpu, &vcpu->exit, NULL) != HV_SUCCESS)
	{
		free(vcpu);
		return -1;
	}
	/* a bare ARM machine: MMU off, FP/SIMD on, timers quiet, no vectors */
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_SCTLR_EL1, 0);
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_CPACR_EL1, 0x300000);
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_CNTV_CTL_EL0, 0);
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_CNTVOFF_EL2, 0);
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_VBAR_EL1, 0);
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_TPIDR_EL0, 0);
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_TPIDRRO_EL0, 0);
	hv_vcpu_set_reg(vcpu->vcpu, HV_REG_CPSR, 0x3c5); /* EL1h, DAIF masked */
	pthread_setspecific(vcpu_key, vcpu);
	vcpu->attached = 1;
	return 0;
}

void host_vcpu_detach(void)
{
	struct host_vcpu *vcpu = vcpu_of();

	if (vcpu)
	{
		vcpu->attached = 0;
		vcpu_destroy(vcpu);
		pthread_setspecific(vcpu_key, NULL);
	}
}

/* the current vCPU's registers, into the doorbell call ABI
(struct guest_registers is in host.h) */

static void registers_load(struct host_vcpu *vcpu, struct guest_registers *out)
{
	static const hv_reg_t map[31] = {
		HV_REG_X0, HV_REG_X1, HV_REG_X2, HV_REG_X3, HV_REG_X4, HV_REG_X5, HV_REG_X6,
		HV_REG_X7, HV_REG_X8, HV_REG_X9, HV_REG_X10, HV_REG_X11, HV_REG_X12, HV_REG_X13,
		HV_REG_X14, HV_REG_X15, HV_REG_X16, HV_REG_X17, HV_REG_X18, HV_REG_X19, HV_REG_X20,
		HV_REG_X21, HV_REG_X22, HV_REG_X23, HV_REG_X24, HV_REG_X25, HV_REG_X26, HV_REG_X27,
		HV_REG_X28, HV_REG_X29, HV_REG_X30,
	};
	int index;
	uint64_t sp = 0;

	for (index = 0; index < 31; index++)
	{
		uint64_t value = 0;

		hv_vcpu_get_reg(vcpu->vcpu, map[index], &value);
		out->x[index] = (uint32_t)value;
	}
	hv_vcpu_get_reg(vcpu->vcpu, HV_REG_PC, &out->pc);
	hv_vcpu_get_sys_reg(vcpu->vcpu, HV_SYS_REG_SP_EL1, &sp);
	out->sp = (uint32_t)sp;
}

static void registers_store(struct host_vcpu *vcpu, const struct guest_registers *in)
{
	static const hv_reg_t map[31] = {
		HV_REG_X0, HV_REG_X1, HV_REG_X2, HV_REG_X3, HV_REG_X4, HV_REG_X5, HV_REG_X6,
		HV_REG_X7, HV_REG_X8, HV_REG_X9, HV_REG_X10, HV_REG_X11, HV_REG_X12, HV_REG_X13,
		HV_REG_X14, HV_REG_X15, HV_REG_X16, HV_REG_X17, HV_REG_X18, HV_REG_X19, HV_REG_X20,
		HV_REG_X21, HV_REG_X22, HV_REG_X23, HV_REG_X24, HV_REG_X25, HV_REG_X26, HV_REG_X27,
		HV_REG_X28, HV_REG_X29, HV_REG_X30,
	};
	int index;

	for (index = 0; index < 31; index++)
		hv_vcpu_set_reg(vcpu->vcpu, map[index], in->x[index]);
	hv_vcpu_set_reg(vcpu->vcpu, HV_REG_PC, in->pc);
	hv_vcpu_set_sys_reg(vcpu->vcpu, HV_SYS_REG_SP_EL1, in->sp);
}

/* ---------- the doorbell dispatch (the generated dispatch owns the
table; its declarations are in host.h) */

/* runs one doorbell call: the exit is a data abort on a doorbell address */
static void service_doorbell(struct host_vcpu *vcpu)
{
	uint64_t ipa = vcpu->exit->exception.physical_address;
	uint32_t index;
	struct guest_registers registers;

	if (ipa < GUEST_DOORBELL_BASE || ipa >= GUEST_DOORBELL_BASE + GUEST_DOORBELL_SIZE)
	{
		struct guest_registers registers;

		registers_load(vcpu, &registers);
		{
			uint64_t syndrome = vcpu->exit->exception.syndrome;
			unsigned is_write = (unsigned)((syndrome >> 6) & 1);
			uint64_t far_value = 0;
			int depth;
			uint32_t fp = registers.x[29];

			hv_vcpu_get_sys_reg(vcpu->vcpu, HV_SYS_REG_FAR_EL1, &far_value);
			host_logf(HOST_LOG_ERROR, "guest data abort: %s at guest %llx (pc %llx, sp %x, lr %x)",
				is_write ? "write" : "read", (unsigned long long)far_value,
				(unsigned long long)vcpu->pc_after_exit, registers.sp, registers.x[30]);
			for (depth = 0; depth < 12 && fp && host_guest_pointer(fp) && depth < 12; depth++)
			{
				uint8_t *frame = host_guest_pointer(fp);
				uint32_t next, pc;

				memcpy(&next, frame, 4);
				memcpy(&pc, frame + 8, 4);
				host_logf(HOST_LOG_ERROR, "  frame %d: return %x", depth, pc);
				if (!next || next <= fp)
					break;
				fp = next;
			}
		}
		host_fatal("the guest touched memory the host has not mapped");
	}
	index = (uint32_t)(ipa - GUEST_DOORBELL_BASE) / 4;
	if (vcpu->in_call && ipa == vcpu->return_doorbell)
	{
		/* the guest returned from a host-to-guest call: the caller (in
		host_call_guest) reads the registers */
		vcpu->in_call = 0;
		hv_vcpu_set_reg(vcpu->vcpu, HV_REG_PC, vcpu->pc_after_return);
		return;
	}
	if (index >= host_import_count)
		host_fatal("doorbell %u has no import (built images out of step?)", (unsigned)index);
	registers_load(vcpu, &registers);
	host_vcpu_dispatch_begin(&registers);
	host_import_table[index].function(); /* reads/writes the registers */
	/* (the wrapper got them through host_vcpu_registers()) */
	hv_vcpu_set_reg(vcpu->vcpu, HV_REG_PC, vcpu->pc_after_exit);
}

/* the dispatch helpers the import wrappers use */

static __thread struct guest_registers *dispatch_registers;

void host_vcpu_dispatch_begin(struct guest_registers *registers)
{
	dispatch_registers = registers;
}

struct guest_registers *host_vcpu_dispatch_registers(void)
{
	return dispatch_registers;
}

/* ---------- the run loop and calls in both directions */

/* runs the vCPU until it stops again, and services every exit */
static void run_until_stops(struct host_vcpu *vcpu)
{
	for (;;)
	{
		if (hv_vcpu_run(vcpu->vcpu) != HV_SUCCESS)
			host_fatal("hv_vcpu_run failed");
		if (vcpu->exit->reason == HV_EXIT_REASON_EXCEPTION)
		{
			unsigned ec = (unsigned)((vcpu->exit->exception.syndrome >> 26) & 0x3f);

			/* remember the faulting PC: the doorbell store re-executes
			as a plain store after the call (stage 2 grants the page
			around the call), or simply continues past it */
			vcpu->pc_after_exit = vcpu->exit->exception.physical_address ? vcpu->pc_after_exit : 0;
			{
				uint64_t pc = 0;

				hv_vcpu_get_reg(vcpu->vcpu, HV_REG_PC, &pc);
				vcpu->pc_after_exit = pc + 4;
			}
			if (ec == 0x24) /* data abort from a lower or same EL */
			{
				if (host_memory_watch_fault(vcpu->exit->exception.physical_address))
					continue; /* the store re-executes (the page is writable now) */
				service_doorbell(vcpu);
				if (vcpu->in_call == 0 && vcpu->return_doorbell &&
					vcpu->exit->exception.physical_address == vcpu->return_doorbell)
					return;
				continue;
			}
			host_fatal("guest exception: syndrome %llx at guest %llx (pc %llx)",
				(unsigned long long)vcpu->exit->exception.syndrome,
				(unsigned long long)vcpu->exit->exception.physical_address,
				(unsigned long long)vcpu->pc_after_exit);
		}
		if (vcpu->exit->reason == HV_EXIT_REASON_CANCELED)
			return;
		if (vcpu->exit->reason == HV_EXIT_REASON_VTIMER_ACTIVATED)
		{
			/* the guest masked the timer itself (its interrupts are
			masked at all times in this design); resume */
			hv_vcpu_set_vtimer_mask(vcpu->vcpu, false);
			continue;
		}
		host_fatal("unexpected vCPU exit %u", vcpu->exit->reason);
	}
}

/* the doorbell of host-to-guest returns: a word in the doorbell page,
mapped and writable in stage 2 while a call runs (see doorbell_grant) */
#define RETURN_DOORBELL (GUEST_DOORBELL_BASE + GUEST_DOORBELL_SIZE - 4)

/* stage-2 permission surgery for the return doorbell: the page only
faults for the stubs' own stores, which the host never runs; the return
doorbell must store without an exit, so it gets its own mapped page */
static uint8_t *return_doorbell_host;

static void return_doorbell_map(void)
{
	if (!return_doorbell_host)
	{
		return_doorbell_host = mmap(NULL, GUEST_PAGE_SIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (return_doorbell_host == MAP_FAILED)
			host_fatal("cannot allocate the return doorbell page");
		if (hv_vm_map(return_doorbell_host, RETURN_DOORBELL & ~(GUEST_PAGE_SIZE - 1),
			GUEST_PAGE_SIZE, HV_MEMORY_READ | HV_MEMORY_WRITE) != HV_SUCCESS)
			host_fatal("cannot map the return doorbell page");
	}
}

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	struct host_vcpu *vcpu = vcpu_of();
	struct guest_registers registers;

	if (!vcpu)
		host_fatal("guest code called on a thread without a vCPU");
	if (!guest_tp)
	{
		/* first guest call on this thread: attach the runtime (the call
		runs on this thread's guest stack) */
		memset(&registers, 0, sizeof(registers));
		registers.pc = host_image.header->thread_attach;
		registers.sp = vcpu->stack_top;
		registers_store(vcpu, &registers);
		return_doorbell_map();
		vcpu->return_doorbell = RETURN_DOORBELL;
		vcpu->in_call = 1;
		run_until_stops(vcpu);
		/* (x0 = the new thread pointer; guest_tp reads it in
		host_thread.c through the same doorbells) */
	}
	memset(&registers, 0, sizeof(registers));
	registers.x[0] = a;
	registers.x[1] = b;
	registers.x[2] = c;
	registers.x[3] = d;
	registers.pc = function;
	registers.sp = vcpu->stack_top;
	registers_store(vcpu, &registers);
	return_doorbell_map();
	vcpu->return_doorbell = RETURN_DOORBELL;
	vcpu->in_call = 1;
	run_until_stops(vcpu);
	registers_load(vcpu, &registers);
	return registers.x[0];
}

void host_vcpu_set_stack(uint32_t stack_top)
{
	struct host_vcpu *vcpu = vcpu_of();

	if (vcpu)
		vcpu->stack_top = stack_top;
}

/* a thread's main run loop (host_thread.c's guest threads): runs guest
code until the thread's function returns; the function's doorbell is the
thread's own (the caller plants the return address) */
void host_vcpu_run_forever(void)
{
	struct host_vcpu *vcpu = vcpu_of();

	if (!vcpu)
		host_fatal("no vCPU on a guest thread");
	vcpu->in_call = 0;
	vcpu->return_doorbell = 0;
	for (;;)
		run_until_stops(vcpu);
}

/* ---------- the host's view of guest memory (the mirror) */

uint8_t *host_guest_pointer(uint32_t guest)
{
	uint8_t *page = mirror[MIRROR_INDEX(guest)];

	return page ? page + (guest & (GUEST_PAGE_SIZE - 1)) : NULL;
}

void host_mirror_install(unsigned int guest, void *host)
{
	mirror[MIRROR_INDEX(guest)] = host;
}

/* the game thread's entry (host_main.c's game_main ends here): runs the
image's __guest_start(boot) on this thread's vCPU and guest stack, and
never comes back (the guest's exit goes through the doorbells) */
void host_run_guest_main(uint32_t boot)
{
	struct host_vcpu *vcpu = vcpu_of();
	struct guest_registers registers;

	if (!vcpu)
		host_fatal("no vCPU on the game thread");
	memset(&registers, 0, sizeof(registers));
	registers.x[0] = boot;
	registers.pc = host_image.header->start;
	registers.sp = vcpu->stack_top;
	registers_store(vcpu, &registers);
	host_vcpu_run_forever();
}

/* the guest address of a host pointer from the pools (host_main.c's boot
block, host_sdl.c's nothing else), or 0 */
uint32_t host_guest_address(const void *pointer)
{
	uintptr_t host = (uintptr_t)pointer;
	uint32_t result = 0;
	int index;

	if (host >= GUEST_WINDOW_BASE && host < GUEST_WINDOW_BASE + GUEST_WINDOW_SIZE)
		return (uint32_t)host; /* (the window is not a pool; unreachable) */
	pthread_mutex_lock(&vm_lock);
	for (index = 0; index < pool_count && !result; index++)
	{
		struct pool_region *pool = pools[index];
		uintptr_t start = (uintptr_t)pool->host;
		uintptr_t end = start + (uintptr_t)pool->size;

		if (host >= start && host < end)
			result = pool->base + (uint32_t)(host - start);
	}
	pthread_mutex_unlock(&vm_lock);
	return result;
}
