/*
GUEST_SETJMP.C

setjmp and longjmp, which the guest's subset of musl does not build (its
src/setjmp is assembly) and Lua's errors need (port/third_party/lua/ldo.c,
the mutators' scripts). musl 1.2.5's src/setjmp/aarch64/setjmp.s and
longjmp.s (MIT licence: COPYRIGHT in its source), in the Darwin syntax the
guest is compiled in, which tools/android_asm_convert.py makes ELF: the
callee-saved registers, the stack pointer and the floating-point ones they
save fill the guest's __jmp_buf of 22 64-bit words
(port/android/guest/libc/arch/arm64_32/bits/setjmp.h). The guest's pointers
are 32 bits, its addresses in the low 4 GB: the buffer's is zero-extended
before it is used.
*/

__asm__(
	".text\n"
	".p2align 2\n"
	".globl ___setjmp\n"
	".globl __setjmp\n"
	".globl _setjmp\n"
	"___setjmp:\n"
	"__setjmp:\n"
	"_setjmp:\n"
	"	mov w0, w0\n"
	"	stp x19, x20, [x0, #0]\n"
	"	stp x21, x22, [x0, #16]\n"
	"	stp x23, x24, [x0, #32]\n"
	"	stp x25, x26, [x0, #48]\n"
	"	stp x27, x28, [x0, #64]\n"
	"	stp x29, x30, [x0, #80]\n"
	"	mov x2, sp\n"
	"	str x2, [x0, #104]\n"
	"	stp d8, d9, [x0, #112]\n"
	"	stp d10, d11, [x0, #128]\n"
	"	stp d12, d13, [x0, #144]\n"
	"	stp d14, d15, [x0, #160]\n"
	"	mov x0, #0\n"
	"	ret\n"
	".globl __longjmp\n"
	".globl _longjmp\n"
	"__longjmp:\n"
	"_longjmp:\n"
	"	mov w0, w0\n"
	"	ldp x19, x20, [x0, #0]\n"
	"	ldp x21, x22, [x0, #16]\n"
	"	ldp x23, x24, [x0, #32]\n"
	"	ldp x25, x26, [x0, #48]\n"
	"	ldp x27, x28, [x0, #64]\n"
	"	ldp x29, x30, [x0, #80]\n"
	"	ldr x2, [x0, #104]\n"
	"	mov sp, x2\n"
	"	ldp d8, d9, [x0, #112]\n"
	"	ldp d10, d11, [x0, #128]\n"
	"	ldp d12, d13, [x0, #144]\n"
	"	ldp d14, d15, [x0, #160]\n"
	"	cmp w1, #0\n"
	"	csinc w0, w1, wzr, ne\n"
	"	br x30\n");
