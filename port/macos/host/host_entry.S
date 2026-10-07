/*
HOST_ENTRY.S

The two places the host passes control to guest code by hand
(host_thread.c).

The guest's code keeps the base of its 4 GB in x28 and uses x27 as scratch
(tools/guest_asm_rebase.py). Both are callee-saved in the host's ABI, so
the host's own code preserves them across every import the guest calls:
only entering the guest needs to set x28, and to save and restore the
host's own values of the two.
*/

	.text

/* uint64_t host_enter_guest(uint64_t function, uint64_t base,
	uint64_t a, uint64_t b, uint64_t c, uint64_t d)

calls the guest code at host address function with a to d, with x28 =
base; the stack must already lie in guest memory */
	.globl _host_enter_guest
	.p2align 2
_host_enter_guest:
	stp x29, x30, [sp, #-32]!
	mov x29, sp
	stp x27, x28, [sp, #16]
	mov x28, x1
	mov x16, x0
	mov x0, x2
	mov x1, x3
	mov x2, x4
	mov x3, x5
	blr x16
	ldp x27, x28, [sp, #16]
	ldp x29, x30, [sp], #32
	ret

/* void host_switch_stack(uint64_t stack, void (*function)(void *), void *argument)

calls function(argument) with the stack pointer at stack (16-byte aligned),
and returns on the stack it was called on */
	.globl _host_switch_stack
	.p2align 2
_host_switch_stack:
	stp x29, x30, [sp, #-16]!
	mov x29, sp
	mov sp, x0
	mov x0, x2
	blr x1
	mov sp, x29
	ldp x29, x30, [sp], #16
	ret
