/*
HOST_STACK.S

host_switch_stack(stack_top, function, argument): moves the calling thread
onto another stack and calls function(argument) there, for good.

host_main.c runs the game on the process's main thread, which Cocoa
requires for windows and events, but the main thread's own stack lies
above 4 GB, where x32 code cannot keep its stack pointer. pthreads cannot
give the main thread another stack, so this switches it by hand: the new
stack is in guest memory, below 4 GB, and the host code the guest calls
(SDL, Cocoa, OpenGL) runs on it too.

rbp is cleared so that frame walks (the crash reporter's, the sampler's)
stop here.
*/

	.text
	.globl _host_switch_stack
	.p2align 4
_host_switch_stack:
	movq %rdi, %rsp
	andq $-16, %rsp
	movq %rdx, %rdi
	xorl %ebp, %ebp
	/* (the call pushes the return address, so the callee starts with the
	stack 16-byte aligned less 8, as the ABI has it) */
	callq *%rsi
	ud2
