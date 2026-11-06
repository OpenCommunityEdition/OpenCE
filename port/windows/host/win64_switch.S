/*
WIN64_SWITCH.S

host_enter_guest: calls guest code from the host, on the guest's stack.

	uint32_t host_enter_guest(uint64_t function, uint64_t a, uint64_t b, uint64_t c, uint64_t d);

The host calls with Windows' convention (function in rcx, the arguments in
rdx, r8, r9 and on the stack); the guest takes System V's (rdi, rsi, rdx,
rcx), and its 32-bit arguments zero-extended (host_thread.c explains why).
Windows' rdi, rsi and xmm6-xmm15 are the caller's but System V's are not,
so they are kept here; rbx, rbp and r12-r15 are kept by both.

The thread's struct host_switch (win64_thread.c) says where the guest's
stack is. The host's stack pointer is kept in it as host_sp for the
import thunks (tools/windows_host_thunks.py), which switch back to it, and
the TEB's stack limits are the guest stack's while guest code runs (the
thunks give the host's back while a host function runs).

host_sp points at the address of `resume' below (8 bytes off 16-byte
alignment: the thunks allocate their frames to suit), as if the thunk at the
top of the host's stack had been called from there: Windows unwinds a host
function's frames through the thunk to here, then through this function's
frame (which its unwind information describes from resume, the 8 bytes of
that address excepted) to the host's caller.
*/

/* struct host_switch */
#define HOST_SP 0
#define GUEST_SP 8
#define HOST_BASE 16
#define HOST_LIMIT 24
#define GUEST_BASE 32
#define GUEST_LIMIT 40
#define HOST_DEALLOCATION 48
#define GUEST_DEALLOCATION 56

/* the TEB's NT_TIB StackBase and StackLimit, and DeallocationStack */
#define TEB_STACK_BASE 8
#define TEB_STACK_LIMIT 16
#define TEB_DEALLOCATION_STACK 0x1478

	.text
	.globl host_enter_guest
	.def host_enter_guest; .scl 2; .type 32; .endef
	.p2align 4
host_enter_guest:
	.seh_proc host_enter_guest
	pushq %rbp
	.seh_pushreg %rbp
	pushq %rbx
	.seh_pushreg %rbx
	pushq %rdi
	.seh_pushreg %rdi
	pushq %rsi
	.seh_pushreg %rsi
	/* xmm6-xmm15 at 0 (16-byte aligned), the previous host_sp at 160 */
	subq $168, %rsp
	.seh_stackalloc 168
	movaps %xmm6, 0(%rsp)
	.seh_savexmm %xmm6, 0
	movaps %xmm7, 16(%rsp)
	.seh_savexmm %xmm7, 16
	movaps %xmm8, 32(%rsp)
	.seh_savexmm %xmm8, 32
	movaps %xmm9, 48(%rsp)
	.seh_savexmm %xmm9, 48
	movaps %xmm10, 64(%rsp)
	.seh_savexmm %xmm10, 64
	movaps %xmm11, 80(%rsp)
	.seh_savexmm %xmm11, 80
	movaps %xmm12, 96(%rsp)
	.seh_savexmm %xmm12, 96
	movaps %xmm13, 112(%rsp)
	.seh_savexmm %xmm13, 112
	movaps %xmm14, 128(%rsp)
	.seh_savexmm %xmm14, 128
	movaps %xmm15, 144(%rsp)
	.seh_savexmm %xmm15, 144
	.seh_endprologue

	movl host_switch_tls_offset(%rip), %r10d
	movq %gs:(%r10), %r10
	movq HOST_SP(%r10), %rax
	movq %rax, 160(%rsp)
	/* the arguments: d is above the return address, the 32 bytes of the
	caller's home area, and this frame (32 + 168 bytes) */
	movq %rcx, %r11
	movl %edx, %edi
	movl %r8d, %esi
	movl %r9d, %edx
	movl 240(%rsp), %ecx
	/* the host's stack and limits, kept; the guest's in the TEB */
	leaq resume(%rip), %rax
	pushq %rax
	movq %rsp, HOST_SP(%r10)
	movq %gs:TEB_STACK_BASE, %rax
	movq %rax, HOST_BASE(%r10)
	movq %gs:TEB_STACK_LIMIT, %rax
	movq %rax, HOST_LIMIT(%r10)
	movq %gs:TEB_DEALLOCATION_STACK, %rax
	movq %rax, HOST_DEALLOCATION(%r10)
	movq GUEST_BASE(%r10), %rax
	movq %rax, %gs:TEB_STACK_BASE
	movq GUEST_LIMIT(%r10), %rax
	movq %rax, %gs:TEB_STACK_LIMIT
	movq GUEST_DEALLOCATION(%r10), %rax
	movq %rax, %gs:TEB_DEALLOCATION_STACK
	/* the guest's stack, 16-byte aligned at the call */
	movq %rsp, %rax
	movq GUEST_SP(%r10), %rsp
	andq $-16, %rsp
	pushq %rax
	subq $8, %rsp
	callq *%r11
	addq $8, %rsp
	popq %rsp
resume:
	/* (rax holds the result) */
	movl host_switch_tls_offset(%rip), %r10d
	movq %gs:(%r10), %r10
	movq HOST_BASE(%r10), %rcx
	movq %rcx, %gs:TEB_STACK_BASE
	movq HOST_LIMIT(%r10), %rcx
	movq %rcx, %gs:TEB_STACK_LIMIT
	movq HOST_DEALLOCATION(%r10), %rcx
	movq %rcx, %gs:TEB_DEALLOCATION_STACK
	addq $8, %rsp
	movq 160(%rsp), %rcx
	movq %rcx, HOST_SP(%r10)
	movaps 0(%rsp), %xmm6
	movaps 16(%rsp), %xmm7
	movaps 32(%rsp), %xmm8
	movaps 48(%rsp), %xmm9
	movaps 64(%rsp), %xmm10
	movaps 80(%rsp), %xmm11
	movaps 96(%rsp), %xmm12
	movaps 112(%rsp), %xmm13
	movaps 128(%rsp), %xmm14
	movaps 144(%rsp), %xmm15
	addq $168, %rsp
	popq %rsi
	popq %rdi
	popq %rbx
	popq %rbp
	retq
	.seh_endproc
