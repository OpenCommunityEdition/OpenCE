/*
HOST_VM.C

The VM the guest runs in (docs/macos_host.md).

The guest's memory (the runtime crate's OpenceGuest) is mapped into the VM
at the guest's own addresses, and its stage-1 tables map each address to
itself. Guest code runs at EL0. Its EL1 has only the exception vectors
written here: an exception from EL0 makes an hvc to the host, then an eret
back. Thus

- an import (an svc stub, tools/android_imports.py --svc, its index in w16)
  is carried out by the host (host_dispatch) and returns after the svc;
- a guest function the host calls returns to RETURN_ADDRESS, in the
  unmapped first 2 MB, and the instruction abort there ends the call;
- any other exception is a crash, which the host reports.

Every host thread that runs guest code has a vCPU of its own (Hypervisor.
framework runs a vCPU only on the thread that made it) and a stack in guest
memory for it.
*/

#include "host.h"

#include <Hypervisor/Hypervisor.h>
#include <errno.h>
#include <stdatomic.h>
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* where a guest function the host calls returns to: in the first 2 MB,
which nothing maps (crate plan.rs's NULL_GUARD) */
#define RETURN_ADDRESS 0x1000u
#define DEFAULT_STACK_SIZE (1024 * 1024)
/* below the interrupted guest code's stack pointer, a nested call's stack
starts this far down (past the 128-byte red zone of Apple's AArch64 ABI) */
#define NESTED_STACK_GAP 0x400

/* exception classes (ESR_ELx.EC) */
#define EC_SVC64 0x15
#define EC_HVC64 0x16
#define EC_INSTRUCTION_ABORT_LOWER 0x20
/* CNTKCTL_EL1: EL0 may read the virtual count */
#define CNTKCTL_EL0VCTEN (1u << 1)

/* the vectors' hvc numbers */
enum
{
	_vector_lower_sync = 0,
	_vector_lower_other = 1,
	_vector_el1 = 3,
};

/* AArch64 instructions */
#define HVC(n) (0xd4000002u | ((uint32_t)(n) << 5))
#define ERET 0xd69f03e0u
#define BRANCH_TO_SELF 0x14000000u

struct vcpu
{
	hv_vcpu_t id;
	hv_vcpu_exit_t *exit;
	uint32_t stack;
	uint32_t stack_size;
	/* host_call_guest calls under way on this vCPU */
	int depth;
};

/* (build/macos/host/host_import_names.c) */
extern const char *const host_import_names[];
extern const unsigned int host_import_name_count;

/* HALO_HOST_IMPORT_COUNTS: each import's calls, and every exit from the
VM (imports, timers, the sampler's), counted from the VM's start */
#define COUNTED_IMPORTS 1024
static int counting;
static _Atomic unsigned long long import_counts[COUNTED_IMPORTS];
static _Atomic unsigned long long exit_count;
static struct timespec counting_start;

/* the vCPUs, for the sampler (HALO_HOST_SAMPLE) */
#define MAXIMUM_VCPUS 64
static hv_vcpu_t vcpu_ids[MAXIMUM_VCPUS];
static int vcpu_count;
static pthread_mutex_t vcpu_lock = PTHREAD_MUTEX_INITIALIZER;
/* set while the sampler asks every vCPU for its state */
static atomic_int sampling;

static __thread struct vcpu *current_vcpu;
/* the guest stack size the next vCPU made on this thread gets */
static __thread uint32_t next_stack_size;
static uint64_t hvc_address;

/* ---------- registers */

static uint64_t get_reg(struct vcpu *vcpu, hv_reg_t reg)
{
	uint64_t value = 0;

	hv_vcpu_get_reg(vcpu->id, reg, &value);
	return value;
}

static void set_reg(struct vcpu *vcpu, hv_reg_t reg, uint64_t value)
{
	if (hv_vcpu_set_reg(vcpu->id, reg, value) != HV_SUCCESS)
		host_fatal("cannot set a vCPU register");
}

static uint64_t get_sys(struct vcpu *vcpu, hv_sys_reg_t reg)
{
	uint64_t value = 0;

	hv_vcpu_get_sys_reg(vcpu->id, reg, &value);
	return value;
}

static void set_sys(struct vcpu *vcpu, hv_sys_reg_t reg, uint64_t value)
{
	if (hv_vcpu_set_sys_reg(vcpu->id, reg, value) != HV_SUCCESS)
		host_fatal("cannot set a vCPU system register");
}

static uint64_t get_d(struct vcpu *vcpu, int index)
{
	hv_simd_fp_uchar16_t value;
	uint64_t low;

	memset(&value, 0, sizeof(value));
	hv_vcpu_get_simd_fp_reg(vcpu->id, (hv_simd_fp_reg_t)(HV_SIMD_FP_REG_Q0 + index), &value);
	memcpy(&low, &value, sizeof(low));
	return low;
}

static void set_d(struct vcpu *vcpu, int index, uint64_t low)
{
	hv_simd_fp_uchar16_t value;

	memset(&value, 0, sizeof(value));
	memcpy(&value, &low, sizeof(low));
	hv_vcpu_set_simd_fp_reg(vcpu->id, (hv_simd_fp_reg_t)(HV_SIMD_FP_REG_Q0 + index), value);
}

/* ---------- the guest state a nested call must give back */

struct saved_state
{
	uint64_t x[31];
	uint64_t pc, cpsr, sp_el0, elr_el1, spsr_el1, fpcr, fpsr;
	hv_simd_fp_uchar16_t q[32];
};

static void save_state(struct vcpu *vcpu, struct saved_state *state)
{
	int index;

	for (index = 0; index < 31; index++)
		state->x[index] = get_reg(vcpu, (hv_reg_t)(HV_REG_X0 + index));
	state->pc = get_reg(vcpu, HV_REG_PC);
	state->cpsr = get_reg(vcpu, HV_REG_CPSR);
	state->fpcr = get_reg(vcpu, HV_REG_FPCR);
	state->fpsr = get_reg(vcpu, HV_REG_FPSR);
	state->sp_el0 = get_sys(vcpu, HV_SYS_REG_SP_EL0);
	state->elr_el1 = get_sys(vcpu, HV_SYS_REG_ELR_EL1);
	state->spsr_el1 = get_sys(vcpu, HV_SYS_REG_SPSR_EL1);
	for (index = 0; index < 32; index++)
		hv_vcpu_get_simd_fp_reg(vcpu->id, (hv_simd_fp_reg_t)(HV_SIMD_FP_REG_Q0 + index), &state->q[index]);
}

static void restore_state(struct vcpu *vcpu, const struct saved_state *state)
{
	int index;

	for (index = 0; index < 31; index++)
		set_reg(vcpu, (hv_reg_t)(HV_REG_X0 + index), state->x[index]);
	set_reg(vcpu, HV_REG_PC, state->pc);
	set_reg(vcpu, HV_REG_CPSR, state->cpsr);
	set_reg(vcpu, HV_REG_FPCR, state->fpcr);
	set_reg(vcpu, HV_REG_FPSR, state->fpsr);
	set_sys(vcpu, HV_SYS_REG_SP_EL0, state->sp_el0);
	set_sys(vcpu, HV_SYS_REG_ELR_EL1, state->elr_el1);
	set_sys(vcpu, HV_SYS_REG_SPSR_EL1, state->spsr_el1);
	for (index = 0; index < 32; index++)
		hv_vcpu_set_simd_fp_reg(vcpu->id, (hv_simd_fp_reg_t)(HV_SIMD_FP_REG_Q0 + index), state->q[index]);
}

/* ---------- crashes */

/* the guest's calls, from its frame records (x29: the previous x29, then the
return address; the guest keeps frame pointers) */
static void print_calls(struct vcpu *vcpu)
{
	uint64_t frame = get_reg(vcpu, HV_REG_FP);
	int depth;

	fputs("halo_host:   called from", stderr);
	for (depth = 0; depth < 24 && frame && frame < 0xfffffff0u && !(frame & 7); depth++)
	{
		const uint64_t *record = (const uint64_t *)opence_guest_host_pointer(host_guest, (uint32_t)frame, 16);

		if (!record || !record[1])
			break;
		fprintf(stderr, " 0x%08llx", (unsigned long long)record[1]);
		frame = record[0];
	}
	fputc('\n', stderr);
}

static void report_crash(struct vcpu *vcpu, const char *what, uint64_t syndrome)
{
	uint64_t esr = get_sys(vcpu, HV_SYS_REG_ESR_EL1);
	int index;

	fprintf(stderr, "halo_host: guest crash: %s (ESR_EL2 %#llx; ESR_EL1 %#llx, EC %#llx) at %#llx, address %#llx\n",
		what, (unsigned long long)syndrome, (unsigned long long)esr, (unsigned long long)(esr >> 26),
		(unsigned long long)get_sys(vcpu, HV_SYS_REG_ELR_EL1), (unsigned long long)get_sys(vcpu, HV_SYS_REG_FAR_EL1));
	for (index = 0; index < 31; index++)
	{
		if (index % 4 == 0)
			fputs("halo_host:  ", stderr);
		fprintf(stderr, " x%-2d 0x%08llx", index, (unsigned long long)get_reg(vcpu, (hv_reg_t)(HV_REG_X0 + index)));
		if (index % 4 == 3 || index == 30)
			fputc('\n', stderr);
	}
	/* (pc is the vectors', where the exception was taken: elr is the
	guest's) */
	fprintf(stderr, "halo_host:   sp 0x%08llx elr 0x%08llx\n", (unsigned long long)get_sys(vcpu, HV_SYS_REG_SP_EL0),
		(unsigned long long)get_sys(vcpu, HV_SYS_REG_ELR_EL1));
	print_calls(vcpu);
	fflush(stderr);
	_exit(70);
}

/* ---------- vCPUs */

static struct vcpu *vcpu_current(void)
{
	const OpenceSystemRegisters *registers = &host_image_info.registers;
	struct vcpu *vcpu = current_vcpu;

	if (vcpu)
		return vcpu;
	vcpu = calloc(1, sizeof(*vcpu));
	if (!vcpu || hv_vcpu_create(&vcpu->id, &vcpu->exit, NULL) != HV_SUCCESS)
		host_fatal("cannot create a vCPU");
	set_sys(vcpu, HV_SYS_REG_MAIR_EL1, registers->mair_el1);
	set_sys(vcpu, HV_SYS_REG_TCR_EL1, registers->tcr_el1);
	set_sys(vcpu, HV_SYS_REG_TTBR0_EL1, registers->ttbr0_el1);
	set_sys(vcpu, HV_SYS_REG_CPACR_EL1, registers->cpacr_el1);
	set_sys(vcpu, HV_SYS_REG_VBAR_EL1, registers->vbar_el1);
	set_sys(vcpu, HV_SYS_REG_SCTLR_EL1, registers->sctlr_el1);
	set_sys(vcpu, HV_SYS_REG_TPIDR_EL0, 0);
	/* (no timer interrupts are wanted) */
	hv_vcpu_set_vtimer_mask(vcpu->id, true);
	/* the guest reads the virtual count at EL0 (EL0VCTEN), the same on
	every vCPU: SDL_GetTicks, port/android/guest/runtime/guest_sdl.c */
	set_sys(vcpu, HV_SYS_REG_CNTKCTL_EL1, CNTKCTL_EL0VCTEN);
	hv_vcpu_set_vtimer_offset(vcpu->id, 0);
	pthread_mutex_lock(&vcpu_lock);
	if (vcpu_count < MAXIMUM_VCPUS)
		vcpu_ids[vcpu_count++] = vcpu->id;
	pthread_mutex_unlock(&vcpu_lock);
	vcpu->stack_size = next_stack_size ? next_stack_size : DEFAULT_STACK_SIZE;
	vcpu->stack = host_guest_allocate(vcpu->stack_size);
	if (!vcpu->stack)
		host_fatal("no guest memory for a thread's stack");
	current_vcpu = vcpu;
	return vcpu;
}

static void vcpu_release(void)
{
	struct vcpu *vcpu = current_vcpu;

	int index;

	if (!vcpu)
		return;
	pthread_mutex_lock(&vcpu_lock);
	for (index = 0; index < vcpu_count; index++)
	{
		if (vcpu_ids[index] == vcpu->id)
		{
			vcpu_ids[index] = vcpu_ids[--vcpu_count];
			break;
		}
	}
	pthread_mutex_unlock(&vcpu_lock);
	hv_vcpu_destroy(vcpu->id);
	host_guest_free(vcpu->stack, vcpu->stack_size);
	free(vcpu);
	current_vcpu = NULL;
}

/* carries out the import the guest's svc asked for */
static void vcpu_import(struct vcpu *vcpu)
{
	struct host_call call;
	uint32_t index = (uint32_t)get_reg(vcpu, HV_REG_X16);
	int register_index;

	for (register_index = 0; register_index < 8; register_index++)
	{
		call.x[register_index] = get_reg(vcpu, (hv_reg_t)(HV_REG_X0 + register_index));
		call.d[register_index] = get_d(vcpu, register_index);
	}
	if (counting > 0 && index < COUNTED_IMPORTS)
		import_counts[index]++;
	call.sp = (uint32_t)get_sys(vcpu, HV_SYS_REG_SP_EL0);
	call.result_x = 0;
	call.result_d = 0;
	if (!host_dispatch(&call, index))
		host_fatal("the guest called import %u, which this host does not have", index);
	set_reg(vcpu, HV_REG_X0, call.result_x);
	set_d(vcpu, 0, call.result_d);
	/* the errno the import left, as Linux's, which a hostposix_* stub keeps
	(tools/android_imports.py --svc) */
	set_reg(vcpu, HV_REG_X1, (uint32_t)host_linux_errno(host_saved_errno));
}

/* runs the vCPU until the guest function it was set to returns */
static void vcpu_run(struct vcpu *vcpu)
{
	for (;;)
	{
		hv_return_t result = hv_vcpu_run(vcpu->id);
		uint64_t syndrome;

		if (result != HV_SUCCESS)
			host_fatal("hv_vcpu_run failed (%#x)", (unsigned int)result);
		if (counting > 0)
			exit_count++;
		if (vcpu->exit->reason == HV_EXIT_REASON_CANCELED)
		{
			/* the sampler's request: where this thread's guest code is */
			if (atomic_load(&sampling))
			{
				uint64_t pc = get_reg(vcpu, HV_REG_PC), cpsr = get_reg(vcpu, HV_REG_CPSR);

				flockfile(stderr);
				fprintf(stderr, "halo_host: sample: vCPU %llu at %#llx%s\n", (unsigned long long)vcpu->id,
					(unsigned long long)((cpsr & 0xf) ? get_sys(vcpu, HV_SYS_REG_ELR_EL1) : pc),
					(cpsr & 0xf) ? " (in an import)" : "");
				print_calls(vcpu);
				funlockfile(stderr);
			}
			continue;
		}
		if (vcpu->exit->reason == HV_EXIT_REASON_VTIMER_ACTIVATED)
			continue;
		if (vcpu->exit->reason != HV_EXIT_REASON_EXCEPTION)
			host_fatal("unexpected vCPU exit %u", (unsigned int)vcpu->exit->reason);
		syndrome = vcpu->exit->exception.syndrome;
		if ((syndrome >> 26) != EC_HVC64)
		{
			/* a fault the VM's own mapping took: the guest reached memory
			outside its mapped regions through a table */
			report_crash(vcpu, "outside the guest's memory", syndrome);
		}
		/* resume after the vectors' hvc (Hypervisor.framework leaves PC
		after it; were it at it, the hvc would come again) */
		if (get_reg(vcpu, HV_REG_PC) == hvc_address)
			set_reg(vcpu, HV_REG_PC, hvc_address + 4);
		if ((syndrome & 0xffff) == _vector_lower_sync)
		{
			uint64_t esr = get_sys(vcpu, HV_SYS_REG_ESR_EL1);
			uint64_t class = esr >> 26;

			if (class == EC_SVC64)
			{
				vcpu_import(vcpu);
				continue;
			}
			if (class == EC_INSTRUCTION_ABORT_LOWER && get_sys(vcpu, HV_SYS_REG_FAR_EL1) == RETURN_ADDRESS)
				return;
			report_crash(vcpu, "an exception in guest code", syndrome);
		}
		report_crash(vcpu, (syndrome & 0xffff) == _vector_el1 ? "an exception at EL1" : "an interrupt", syndrome);
	}
}

/* calls the guest function at `function` on this thread's vCPU */
static uint64_t vcpu_call(uint32_t function, const uint32_t *arguments, int count)
{
	struct vcpu *vcpu = vcpu_current();
	struct saved_state *saved = NULL;
	uint64_t stack, result;
	int index;

	if (vcpu->depth)
	{
		saved = malloc(sizeof(*saved));
		if (!saved)
			host_fatal("no memory for a nested guest call");
		save_state(vcpu, saved);
		stack = (saved->sp_el0 - NESTED_STACK_GAP) & ~0xfULL;
	}
	else
		stack = (uint64_t)vcpu->stack + vcpu->stack_size;
	vcpu->depth++;
	for (index = 0; index < 8; index++)
		set_reg(vcpu, (hv_reg_t)(HV_REG_X0 + index), index < count ? arguments[index] : 0);
	set_reg(vcpu, HV_REG_LR, RETURN_ADDRESS);
	set_reg(vcpu, HV_REG_PC, function);
	/* EL0t */
	set_reg(vcpu, HV_REG_CPSR, 0);
	set_sys(vcpu, HV_SYS_REG_SP_EL0, stack);
	vcpu_run(vcpu);
	result = get_reg(vcpu, HV_REG_X0);
	vcpu->depth--;
	if (saved)
	{
		restore_state(vcpu, saved);
		free(saved);
	}
	return result;
}

/* HALO_HOST_SAMPLE=<seconds>: every so often, each vCPU's guest code's
address and calls, to standard error (where a guest that hangs is) */
static void *sampler(void *context)
{
	unsigned int seconds = (unsigned int)(uintptr_t)context;

	for (;;)
	{
		hv_vcpu_t ids[MAXIMUM_VCPUS];
		int count;

		sleep(seconds);
		pthread_mutex_lock(&vcpu_lock);
		count = vcpu_count;
		memcpy(ids, vcpu_ids, sizeof(ids[0]) * (size_t)count);
		pthread_mutex_unlock(&vcpu_lock);
		fprintf(stderr, "halo_host: sample: %d vCPUs\n", count);
		atomic_store(&sampling, 1);
		if (count)
			hv_vcpus_exit(ids, (uint32_t)count);
		/* (a vCPU that is in an import is sampled when it next runs) */
		usleep(100000);
		atomic_store(&sampling, 0);
	}
	return NULL;
}

void host_vm_start_sampler(void)
{
	const char *setting = getenv("HALO_HOST_SAMPLE");
	pthread_t thread;
	int seconds = setting ? atoi(setting) : 0;

	if (seconds > 0 && pthread_create(&thread, NULL, sampler, (void *)(uintptr_t)seconds) == 0)
		pthread_detach(thread);
}

static int by_count(const void *left, const void *right)
{
	unsigned long long a = import_counts[*(const uint32_t *)left], b = import_counts[*(const uint32_t *)right];

	return a < b ? 1 : a > b ? -1 : 0;
}

void host_vm_report_import_counts(void)
{
	static uint32_t order[COUNTED_IMPORTS];
	uint32_t count = host_import_name_count < COUNTED_IMPORTS ? host_import_name_count : COUNTED_IMPORTS;
	uint32_t index;
	struct timespec now;
	double seconds;

	if (counting <= 0)
		return;
	clock_gettime(CLOCK_MONOTONIC, &now);
	seconds = (double)(now.tv_sec - counting_start.tv_sec) + (now.tv_nsec - counting_start.tv_nsec) / 1e9;
	if (seconds <= 0)
		seconds = 1e-9;
	fprintf(stderr, "halo_host: exits: %llu in %.1f s, %.0f/s\n", (unsigned long long)exit_count, seconds,
		exit_count / seconds);
	/* (the busiest first) */
	for (index = 0; index < count; index++)
		order[index] = index;
	qsort(order, count, sizeof(order[0]), by_count);
	for (index = 0; index < count && import_counts[order[index]]; index++)
		fprintf(stderr, "halo_host: import %s: %llu, %.0f/s\n", host_import_names[order[index]],
			(unsigned long long)import_counts[order[index]], import_counts[order[index]] / seconds);
}

/* ---------- the VM */

int host_vm_initialize(void)
{
	uint32_t vectors[512];
	uint32_t count = opence_guest_region_count(host_guest), index;
	void *at;

	counting = getenv("HALO_HOST_IMPORT_COUNTS") && *getenv("HALO_HOST_IMPORT_COUNTS");
	clock_gettime(CLOCK_MONOTONIC, &counting_start);
	if (hv_vm_create(NULL) != HV_SUCCESS)
	{
		host_logf(HOST_LOG_ERROR, "cannot create a VM (is the host signed with com.apple.security.hypervisor?)");
		return -1;
	}
	for (index = 0; index < count; index++)
	{
		OpenceRegion region;
		void *memory;

		if (opence_guest_region(host_guest, index, &region) != OPENCE_STATUS_OK)
			return -1;
		memory = opence_guest_host_pointer(host_guest, region.start, region.size);
		if (!memory || hv_vm_map(memory, region.start, region.size, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC) != HV_SUCCESS)
		{
			host_logf(HOST_LOG_ERROR, "cannot map %#x bytes of guest memory at %08x into the VM",
				(unsigned int)region.size, region.start);
			return -1;
		}
	}

	/* the EL1 vectors: from the current EL, and from a lower EL in AArch32,
	nothing is expected (hvc 3); from EL0 in AArch64, a synchronous
	exception is an hvc 0, anything else an hvc 1, each then an eret */
	for (index = 0; index < 512; index++)
		vectors[index] = BRANCH_TO_SELF;
	for (index = 0; index < 16; index++)
	{
		uint32_t *entry = &vectors[index * 32];

		if (index == 8)
		{
			entry[0] = HVC(_vector_lower_sync);
			entry[1] = ERET;
		}
		else if (index > 8 && index < 12)
		{
			entry[0] = HVC(_vector_lower_other);
			entry[1] = ERET;
		}
		else
			entry[0] = HVC(_vector_el1);
	}
	at = host_guest_pointer((uint32_t)host_image_info.registers.vbar_el1);
	memcpy(at, vectors, sizeof(vectors));
	sys_icache_invalidate(at, sizeof(vectors));
	hvc_address = host_image_info.registers.vbar_el1 + 8 * 0x80;
	/* the image's code, written as data by the loader */
	sys_icache_invalidate(host_guest_pointer(host_image_info.image_start),
		(size_t)(host_image_info.image_end - host_image_info.image_start));
	return 0;
}

/* ---------- calls into the guest, and threads */

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	uint32_t arguments[4];

	arguments[0] = a;
	arguments[1] = b;
	arguments[2] = c;
	arguments[3] = d;
	vcpu_current();
	if (!host_vcpu_thread_pointer())
		vcpu_call(host_image_info.header.thread_attach, NULL, 0);
	return (uint32_t)vcpu_call(function, arguments, 4);
}

/* runs the guest's __guest_start(boot) on this thread: the main thread's
guest code, which sets up its own thread pointer */
void host_vm_run_main(uint32_t boot)
{
	vcpu_call(host_image_info.header.start, &boot, 1);
	host_fatal("the guest returned from __guest_start");
}

uint32_t host_vcpu_thread_pointer(void)
{
	return (uint32_t)get_sys(vcpu_current(), HV_SYS_REG_TPIDR_EL0);
}

void host_vcpu_set_thread_pointer(uint32_t value)
{
	set_sys(vcpu_current(), HV_SYS_REG_TPIDR_EL0, value);
}

struct thread_start
{
	void *(*function)(void *);
	void *argument;
	uint32_t stack_size;
};

static void *thread_main(void *context)
{
	struct thread_start start = *(struct thread_start *)context;

	free(context);
	next_stack_size = start.stack_size;
	start.function(start.argument);
	/* (a host thread that ran guest code, the audio thread, still has the
	struct pthread __guest_thread_attach gave it) */
	if (current_vcpu && host_vcpu_thread_pointer())
		vcpu_call(host_image_info.header.thread_detach, NULL, 0);
	vcpu_release();
	return NULL;
}

int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size)
{
	struct thread_start *start = calloc(1, sizeof(*start));
	pthread_attr_t attributes;
	pthread_t thread;
	int error;

	if (!start)
		return ENOMEM;
	start->function = function;
	start->argument = argument;
	start->stack_size = (uint32_t)((stack_size + 0xffff) & ~(size_t)0xffff);
	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	error = pthread_create(&thread, &attributes, thread_main, start);
	pthread_attr_destroy(&attributes);
	if (error)
		free(start);
	return error;
}

static void *guest_thread_main(void *guest_thread)
{
	uint32_t thread = (uint32_t)(uintptr_t)guest_thread;

	vcpu_call(host_image_info.header.thread_start, &thread, 1);
	return NULL;
}

/* the import: a thread for the guest's pthread_create, which runs
__guest_thread_start(thread) */
int host_thread_create(uint32_t guest_thread, uint32_t stack_size)
{
	return host_native_thread_create(guest_thread_main, (void *)(uintptr_t)guest_thread, stack_size);
}
