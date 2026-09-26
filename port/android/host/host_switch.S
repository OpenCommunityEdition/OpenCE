/*
HOST_SWITCH.S

uint64_t host_call_on_stack(uint64_t function, uint64_t a, uint64_t b,
	uint64_t c, uint64_t d, uint64_t stack_top)

Calls function(a, b, c, d) with the stack pointer moved to stack_top, which
lies in guest memory: guest (ILP32) code keeps stack addresses in 32-bit
registers, so it cannot run on a stack above 4 GB.
*/

	.text
	.globl host_call_on_stack
	.type host_call_on_stack, %function
	.p2align 2
host_call_on_stack:
	stp x29, x30, [sp, #-32]!
	mov x29, sp
	str x19, [sp, #16]
	mov x19, sp
	mov x16, x0
	mov x0, x1
	mov x1, x2
	mov x2, x3
	mov x3, x4
	and x5, x5, #~15
	mov sp, x5
	blr x16
	mov sp, x19
	ldr x19, [sp, #16]
	ldp x29, x30, [sp], #32
	ret
	.size host_call_on_stack, . - host_call_on_stack

	.section .note.GNU-stack, "", %progbits
