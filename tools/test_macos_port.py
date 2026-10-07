"""Tests for the macOS port (port/macos, tools/guest_asm_rebase.py,
tools/macos_host_wrappers.py).

The rewriting rules are checked anywhere. On an Apple silicon Mac with
clang and ld.lld, C code is also run through the guest's whole pipeline
(arm64_32 assembly, ELF, rebased, linked at the guest image's address) and
executed at a 4 GB-aligned base, and a build of the game (ninja macos) is
started without a window.
"""

import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

from tools import guest_asm_rebase

ROOT = Path(__file__).resolve().parent.parent


def rebase(*instructions):
    text = "\n".join(f"\t{instruction}" if not instruction.endswith(":") else instruction
                     for instruction in instructions)
    return [line.strip() for line in guest_asm_rebase.rebase(text).split("\n") if line.strip()]


# ---------- the rewriting rules


def test_a_register_base_goes_through_x28():
    assert rebase("ldr w0, [x1]") == ["ldr w0, [x28, w1, uxtw]"]
    assert rebase("strb w2, [x3]") == ["strb w2, [x28, w3, uxtw]"]
    assert rebase("ldr q0, [x8]") == ["ldr q0, [x28, w8, uxtw]"]


def test_offsets_pairs_and_exclusives_use_the_scratch():
    assert rebase("ldr w0, [x1, #8]") == ["add x27, x28, w1, uxtw", "ldr w0, [x27, #8]"]
    assert rebase("ldp x0, x1, [x2, #16]") == ["add x27, x28, w2, uxtw", "ldp x0, x1, [x27, #16]"]
    assert rebase("ldaxr w8, [x0]") == ["add x27, x28, w0, uxtw", "ldaxr w8, [x27]"]
    assert rebase("stlxr w9, w10, [x0]") == ["add x27, x28, w0, uxtw", "stlxr w9, w10, [x27]"]
    assert rebase("ldr w8, [x8, :lo12:table]") == ["add x27, x28, w8, uxtw", "ldr w8, [x27, :lo12:table]"]


def test_writeback_advances_the_register():
    # pre-index: advance, then access; post-index: access, then advance
    assert rebase("ldr w0, [x1, #4]!") == ["add x1, x1, #4", "ldr w0, [x28, w1, uxtw]"]
    assert rebase("stp x0, x1, [x2, #-16]!") == ["sub x2, x2, #16", "add x27, x28, w2, uxtw", "stp x0, x1, [x27]"]
    assert rebase("ldrb w8, [x1], #1") == ["ldrb w8, [x28, w1, uxtw]", "add x1, x1, #1"]
    assert rebase("ld1 {v0.16b}, [x1], x9") == ["add x27, x28, w1, uxtw", "ld1 {v0.16b}, [x27]", "add x1, x1, x9"]


def test_register_offsets_are_added_first():
    assert rebase("ldr w0, [x0, w1, sxtw #2]") == ["add x27, x0, w1, sxtw #2", "ldr w0, [x28, w27, uxtw]"]
    assert rebase("ldrb w10, [x9, x8]") == ["add x27, x9, x8", "ldrb w10, [x28, w27, uxtw]"]
    assert rebase("str w19, [x8, wzr, uxtw #2]") == ["str w19, [x28, w8, uxtw]"]


def test_the_stack_and_frame_pointer_stay():
    for instruction in ("stp x29, x30, [sp, #-16]!", "ldr x0, [sp, #8]", "stur w8, [x29, #-4]",
                        "ldp x29, x30, [sp], #16", "add x29, sp, #16", "mov x29, sp", "sub sp, sp, #32",
                        "sub sp, x29, #16", "sub sp, sp, x8"):
        assert rebase(instruction) == [instruction]


def test_host_addresses_are_zero_extended():
    assert rebase("add x9, sp, #12") == ["add x9, sp, #12", "mov w9, w9"]
    assert rebase("sub x0, x29, #4") == ["sub x0, x29, #4", "mov w0, w0"]
    assert rebase("mov x8, sp") == ["mov x8, sp", "mov w8, w8"]
    assert rebase("adrp x8, symbol") == ["adrp x8, symbol", "mov w8, w8"]
    assert rebase("adr x9, .LJTI0_0") == ["adr x9, .LJTI0_0", "mov w9, w9"]
    # (their low halves are the guest's)
    assert rebase("tst w29, #0x3") == ["tst w29, #0x3"]
    # (x30 is an ordinary register once the return address is saved)
    assert rebase("cmp w30, w8") == ["cmp w30, w8"]
    assert rebase("add x8, x30, #1") == ["add x8, x30, #1"]


def test_a_used_once_page_address_stays_a_host_one():
    assert rebase("adrp x8, value", "ldr w8, [x8, :lo12:value]") == ["adrp x8, value", "ldr w8, [x8, :lo12:value]"]
    # (but not when the register lives on)
    assert rebase("adrp x8, value", "ldr w9, [x8, :lo12:value]") == [
        "adrp x8, value", "mov w8, w8", "add x27, x28, w8, uxtw", "ldr w9, [x27, :lo12:value]"]


def test_the_stack_pointer_set_from_a_guest_value_gets_the_base():
    assert rebase("mov sp, x8") == ["mov x27, x8", "add sp, x28, w27, uxtw"]
    assert rebase("and sp, x9, #0xfffffffffffffff0") == ["and x27, x9, #0xfffffffffffffff0", "add sp, x28, w27, uxtw"]


def test_indirect_branches_go_through_x28():
    assert rebase("blr x8") == ["add x27, x28, w8, uxtw", "blr x27"]
    assert rebase("br x9") == ["add x27, x28, w9, uxtw", "br x27"]
    assert rebase("ret") == ["ret"]


def test_prefetches_are_dropped():
    assert rebase("prfm pldl1keep, [x8, #64]") == []


def test_what_cannot_be_rebased_is_refused():
    for instruction in ("ldr x0, [x27]", "add x28, x28, #1", "cmp x29, x8", "mov x29, x0"):
        with pytest.raises(guest_asm_rebase.RebaseError):
            rebase(instruction)


# ---------- the host's wrappers


def test_posix_wrappers_add_the_base_but_to_handles(tmp_path):
    sys.path.insert(0, str(ROOT / "tools"))
    try:
        import macos_host_wrappers
    finally:
        sys.path.pop(0)
    text = macos_host_wrappers.posix_wrappers(str(ROOT / "port/linux/src/posix.h"))
    assert "posix_directory_next((void *)(uintptr_t)directory, (char *)G2H(name), name_size)" in text
    assert "posix_find_entry_case_insensitive((const char *)G2H(directory)" in text
    assert "uint32_t hostposix_directory_open(uint32_t path)" in text


def test_gl_wrappers_take_what_the_guest_passes():
    """each hostgl_ function's parameters, as the guest's side
    (tools/android_gl_stubs.py) passes them and the host's wrapper takes
    them: a pointer as a 32-bit value (or a 64-bit one, on the stack), and
    every other parameter the same"""
    import re

    headers = ROOT / "build/macos/third_party/khronos"
    if not (headers / "GLES3/gl32.h").is_file():
        pytest.skip("needs the OpenGL ES headers (python3 configure.py on a Mac)")
    sys.path.insert(0, str(ROOT / "tools"))
    try:
        import android_gl_stubs
        import macos_host_wrappers
    finally:
        sys.path.pop(0)
    gl_header = str(ROOT / "port/linux/src/gl.h")
    gl32, gl2ext = str(headers / "GLES3/gl32.h"), str(headers / "GLES2/gl2ext.h")
    guest_out = ROOT / "build/macos/test_guest_gl.c"
    imports_out = ROOT / "build/macos/test_gl_imports.list"
    old_argv = sys.argv
    sys.argv = ["android_gl_stubs.py", gl_header, gl32, gl2ext, str(guest_out), str(imports_out)]
    try:
        android_gl_stubs.main()
    finally:
        sys.argv = old_argv
    guest = guest_out.read_text()
    host = macos_host_wrappers.gl_wrappers(gl_header, gl32, gl2ext)

    def parameters(text, pattern):
        return {name: [p.strip() for p in params.split(",")] if params.strip() != "void" else []
                for name, params in re.findall(pattern, text)}

    # (the declarations, at the start of their lines: not the calls)
    guest_declarations = parameters(guest, r"(?m)^[^\t\n]*?\b(hostgl_\w+)\((.*?)\);$")
    host_definitions = parameters(host, r"\n\w[\w \*]*?\b(hostgl_\w+)\((.*?)\)\n\{")
    guest_out.unlink()
    imports_out.unlink()
    assert set(guest_declarations) == set(host_definitions)
    # (written by hand on both sides: the guest passes its array of pointers
    # widened to 64 bits)
    assert guest_declarations.pop("hostgl_glShaderSource") == [
        "GLuint shader", "GLsizei count", "const unsigned long long *strings", "const GLint *lengths"]
    assert host_definitions["hostgl_glShaderSource"] == [
        "GLuint shader", "GLsizei count", "uint32_t strings", "uint32_t lengths"]
    for name, guest_parameters in guest_declarations.items():
        host_parameters = host_definitions[name]
        assert len(guest_parameters) == len(host_parameters), name
        for guest_parameter, host_parameter in zip(guest_parameters, host_parameters):
            # (the guest's declarations are of types alone)
            guest_type = guest_parameter
            host_type = host_parameter.rsplit(" ", 1)[0]
            if "*" in guest_type:
                assert host_type == "uint32_t", (name, guest_parameter, host_parameter)
            elif guest_type == "unsigned long long":
                assert host_type in ("uint64_t", "uint32_t"), (name, guest_parameter, host_parameter)
            else:
                assert host_type == guest_type, (name, guest_parameter, host_parameter)


# ---------- C code through the guest's pipeline, run rebased


GUEST_TEST = r"""
#include <stdarg.h>
typedef unsigned long size_t;
struct node { int value; short s; char c[10]; struct node *next; double d; };
static struct node nodes[8];
int global_array[64];
static int (*ops[4])(int, int);
static int add(int a, int b) { return a + b; }
static int sub(int a, int b) { return a - b; }
static int mul(int a, int b) { return a * b; }
static int dvd(int a, int b) { return b ? a / b : 0; }
__attribute__((noinline)) int twice(int *p) { return *p * 2; }
__attribute__((noinline)) int is_fifth(int *p) { return p == &global_array[5]; }
__attribute__((noinline)) int same(int *a, int *b) { return a == b; }
__attribute__((noinline)) int sum_list(struct node *p)
{ int t = 0; while (p) { t += p->value + p->s + p->c[3]; p = p->next; } return t; }
__attribute__((noinline)) int vla(int n)
{ int a[n]; for (int i = 0; i < n; i++) a[i] = i * 3; int s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
__attribute__((noinline)) int pick(int x)
{ switch (x) { case 0: return 3; case 1: return 7; case 2: return 9; case 3: return 1; case 4: return 22;
  case 5: return 8; case 6: return 11; default: return -1; } }
__attribute__((noinline)) int sum_ints(int n, ...)
{ va_list ap; va_start(ap, n); int s = 0; for (int i = 0; i < n; i++) s += va_arg(ap, int); va_end(ap); return s; }
__attribute__((noinline)) double sum_doubles(int n, ...)
{ va_list ap; va_start(ap, n); double s = 0; for (int i = 0; i < n; i++) s += va_arg(ap, double); va_end(ap);
  return s; }
__attribute__((noinline)) int big_frame(int k)
{ volatile char buffer[20000]; for (int i = 0; i < 20000; i += 100) buffer[i] = (char)(i + k); int s = 0;
  for (int i = 0; i < 20000; i += 100) s += buffer[i]; return s; }
__attribute__((noinline)) int atomics(int *p)
{ __atomic_add_fetch(p, 5, __ATOMIC_SEQ_CST); int expected = 10;
  __atomic_compare_exchange_n(p, &expected, 20, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return *p; }
__attribute__((noinline)) void copy(char *d, const char *s, size_t n) { while (n--) *d++ = *s++; }
__attribute__((noinline)) float dot(float *a, float *b, int n)
{ float s = 0; for (int i = 0; i < n; i++) s += a[i] * b[i]; return s; }
__attribute__((noinline)) long long wide(long long *p, int i) { return p[i] * 3 + p[i + 1]; }
__attribute__((noinline)) int recurse(int n) { int local = n; if (n == 0) return 0; return twice(&local) + recurse(n - 1); }
__attribute__((noinline)) unsigned long long widen(void *p) { return (unsigned long long)(unsigned long)p; }
static char text[] = "hello rebased world";
__attribute__((noinline)) int length(const char *s) { int n = 0; while (s[n]) n++; return n; }
struct big { int a[20]; };
__attribute__((noinline)) struct big make_big(int k) { struct big b; for (int i = 0; i < 20; i++) b.a[i] = i + k; return b; }
__attribute__((noinline)) int use_big(void) { struct big b = make_big(3); return b.a[0] + b.a[19]; }

int test_main(void)
{
	int failures = 0, bit = 0;
	int local = 21;
	int array[10];
	float fa[17], fb[17];
	long long w[4] = { 1, 2, 3, 4 };
	char destination[32];
#define CHECK(x) do { if (!(x)) failures |= 1 << bit; bit++; } while (0)
	CHECK(twice(&local) == 42);
	CHECK(is_fifth(&global_array[5]) && !is_fifth(&global_array[4]));
	CHECK(same(&local, &local) && !same(&local, &array[0]));
	for (int i = 0; i < 8; i++)
	{ nodes[i].value = i; nodes[i].s = 1; nodes[i].c[3] = 2; nodes[i].next = i < 7 ? &nodes[i + 1] : 0; }
	CHECK(sum_list(&nodes[0]) == 28 + 8 + 16);
	CHECK(vla(10) == 135);
	CHECK(pick(4) == 22 && pick(6) == 11 && pick(9) == -1);
	ops[0] = add; ops[1] = sub; ops[2] = mul; ops[3] = dvd;
	CHECK(ops[0](3, 4) == 7 && ops[1](3, 4) == -1 && ops[2](3, 4) == 12 && ops[3](12, 4) == 3);
	CHECK(sum_ints(4, 1, 2, 3, 4) == 10);
	CHECK(sum_doubles(3, 1.5, 2.5, 3.0) == 7.0);
	CHECK(big_frame(1) == -200);
	global_array[1] = 5;
	CHECK(atomics(&global_array[1]) == 20);
	copy(destination, text, sizeof(text));
	CHECK(length(destination) == 19);
	for (int i = 0; i < 17; i++) { fa[i] = (float)i; fb[i] = 2.0f; }
	CHECK(dot(fa, fb, 17) == 272.0f);
	CHECK(wide(w, 1) == 9);
	CHECK(recurse(5) == 30);
	CHECK(widen(&local) == (unsigned long)&local && widen(&global_array[0]) == (unsigned long)&global_array[0]);
	CHECK(((unsigned long long)(unsigned long)&local >> 32) == 0);
	CHECK(use_big() == 25);
	return failures | (bit << 24);
}
"""

GUEST_LINKER_SCRIPT = """
ENTRY(test_main)
SECTIONS
{
	. = 0x88000000;
	.text : { *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	. = ALIGN(0x10000);
	.data : { *(.data .data.*) }
	.bss : { *(.bss .bss.*) *(COMMON) }
	/DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame) }
}
"""

# a host that maps the image at a 4 GB-aligned base and calls test_main
# with x28 set, on a stack in the guest's 4 GB
TEST_HOST = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <libkern/OSCacheControl.h>

uint64_t host_enter_guest(uint64_t function, uint64_t base, uint64_t a, uint64_t b, uint64_t c, uint64_t d);
void host_switch_stack(uint64_t stack, void (*function)(void *), void *argument);

struct header { unsigned char ident[16]; uint16_t type, machine; uint32_t version; uint64_t entry, phoff, shoff;
	uint32_t flags; uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx; };
struct segment { uint32_t type, flags; uint64_t offset, address, physical, file_size, memory_size, alignment; };

static uint64_t base, entry, result;

static void run(void *unused)
{
	(void)unused;
	result = host_enter_guest(base + entry, base, 0, 0, 0, 0);
}

int main(int argc, char **argv)
{
	FILE *file = fopen(argv[1], "rb");
	long size;
	unsigned char *image;
	struct header *elf;
	struct segment *segments;
	uint8_t *reservation;
	int index;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	image = malloc(size);
	fread(image, 1, size, file);
	fclose(file);
	reservation = mmap(NULL, 8ull << 30, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
	base = ((uint64_t)reservation + 0xffffffffull) & ~0xffffffffull;
	elf = (struct header *)image;
	segments = (struct segment *)(image + elf->phoff);
	mmap((void *)(base + 0x88000000ull), 0x1000000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
	for (index = 0; index < elf->phnum; index++)
	{
		if (segments[index].type == 1)
			memcpy((void *)(base + segments[index].address), image + segments[index].offset,
				segments[index].file_size);
	}
	for (index = 0; index < elf->phnum; index++)
	{
		if (segments[index].type == 1 && (segments[index].flags & 1))
		{
			uint64_t start = (base + segments[index].address) & ~0x3fffull;
			uint64_t end = (base + segments[index].address + segments[index].memory_size + 0x3fff) & ~0x3fffull;

			mprotect((void *)start, end - start, PROT_READ | PROT_EXEC);
			sys_icache_invalidate((void *)start, end - start);
		}
	}
	mmap((void *)(base + 0x40000000ull - 0x100000), 0x100000, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
	entry = elf->entry;
	host_switch_stack(base + 0x40000000ull, run, NULL);
	printf("%llu %llu\n", (unsigned long long)(result >> 24) & 0xff, (unsigned long long)result & 0xffffff);
	return 0;
}
"""

GUEST_FLAGS = [
    "--target=arm64_32-apple-watchos", "-U__APPLE__", "-U__MACH__", "-fno-define-target-os-macros",
    "-mcpu=cortex-a53", "-ffixed-x27", "-ffixed-x28", "-mllvm", "-aarch64-enable-compress-jump-tables=false",
    "-O2", "-fno-stack-protector", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-mllvm",
    "-aarch64-neon-syntax=generic", "-ffreestanding", "-fno-builtin", "-fno-omit-frame-pointer",
    "-Wno-incompatible-sysroot",
]


def apple_silicon_tools():
    if sys.platform != "darwin" or platform.machine() != "arm64":
        return None
    lld = shutil.which("ld.lld") or next((candidate for candidate in ("/opt/homebrew/opt/lld/bin/ld.lld",
                                                                       "/opt/homebrew/opt/llvm/bin/ld.lld")
                                          if Path(candidate).is_file()), None)
    if not shutil.which("clang") or not lld:
        return None
    return lld


def test_rebased_code_runs_at_its_base(tmp_path):
    lld = apple_silicon_tools()
    if not lld:
        pytest.skip("needs an Apple silicon Mac with clang and ld.lld")
    (tmp_path / "test.c").write_text(GUEST_TEST)
    (tmp_path / "test.ld").write_text(GUEST_LINKER_SCRIPT)
    (tmp_path / "host.c").write_text(TEST_HOST)

    def run(*arguments):
        subprocess.run([str(a) for a in arguments], check=True, cwd=tmp_path)

    run("clang", *GUEST_FLAGS, "-S", "test.c", "-o", "test.darwin.s")
    run(sys.executable, ROOT / "tools/android_asm_convert.py", "test.darwin.s", "test.elf.s")
    run(sys.executable, ROOT / "tools/guest_asm_rebase.py", "test.elf.s", "test.s")
    run("clang", "--target=aarch64-linux-gnu", "-c", "test.s", "-o", "test.o")
    run(lld, "-m", "aarch64linux", "-static", "-nostdlib", "-T", "test.ld", "-o", "test.elf", "test.o")
    run("clang", "-O1", "host.c", ROOT / "port/macos/host/host_entry.S", "-o", "host")
    output = subprocess.run([str(tmp_path / "host"), "test.elf"], check=True, cwd=tmp_path, capture_output=True,
                            text=True).stdout.split()
    checks, failures = int(output[0]), int(output[1])
    assert checks == 18
    assert failures == 0, f"failed checks (bits): {failures:#x}"


# ---------- the guest's C library on the host, under load

GUEST_STRESS = r"""#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "guest_host.h"

static int failures;
#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #x); failures++; } } while (0)

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t turn = PTHREAD_COND_INITIALIZER;
static long counter;
static int token;

static unsigned next_random(unsigned *state) { *state = *state * 1103515245u + 12345u; return *state >> 8; }

static void *allocator(void *argument)
{
	unsigned state = (unsigned)(unsigned long)argument;
	void *blocks[64] = { 0 };
	size_t sizes[64] = { 0 };
	int round, index;

	for (round = 0; round < 20000; round++)
	{
		index = next_random(&state) % 64;
		if (blocks[index])
		{
			unsigned char *bytes = blocks[index];
			size_t i;
			for (i = 0; i < sizes[index]; i += 997)
				if (bytes[i] != (unsigned char)(index + i)) { failures++; break; }
			free(blocks[index]);
			blocks[index] = NULL;
		}
		else
		{
			/* small, page-sized and large (mmapped) blocks */
			size_t size = next_random(&state) % 3 == 0 ? 1 + next_random(&state) % 300000 : 1 + next_random(&state) % 5000;
			unsigned char *bytes = malloc(size);
			size_t i;
			if (!bytes) { failures++; continue; }
			for (i = 0; i < size; i += 997)
				bytes[i] = (unsigned char)(index + i);
			blocks[index] = bytes;
			sizes[index] = size;
		}
		pthread_mutex_lock(&lock);
		counter++;
		pthread_mutex_unlock(&lock);
	}
	for (index = 0; index < 64; index++)
		free(blocks[index]);
	return NULL;
}

/* two threads taking turns through a condition variable */
static void *ping(void *argument)
{
	int me = (int)(long)argument, round;

	for (round = 0; round < 2000; round++)
	{
		pthread_mutex_lock(&lock);
		while (token != me)
			pthread_cond_wait(&turn, &lock);
		token = !me;
		pthread_cond_broadcast(&turn);
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t threads[8];
	int index;
	char path[256];
	struct timespec start, now;

	printf("guest stress test (argc %d, %s)\n", argc, argv[0]);
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0; index < 6; index++)
		CHECK(pthread_create(&threads[index], NULL, allocator, (void *)(unsigned long)(index + 1)) == 0);
	CHECK(pthread_create(&threads[6], NULL, ping, (void *)0L) == 0);
	CHECK(pthread_create(&threads[7], NULL, ping, (void *)1L) == 0);
	for (index = 0; index < 8; index++)
		CHECK(pthread_join(threads[index], NULL) == 0);
	CHECK(counter == 6 * 20000);

	/* mappings at 4 KB, protections, partial unmaps */
	{
		unsigned char *map = mmap(NULL, 5 * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		CHECK(map != MAP_FAILED);
		CHECK(((unsigned long)map & 4095) == 0);
		CHECK(map[0] == 0 && map[5 * 4096 - 1] == 0);
		memset(map, 7, 5 * 4096);
		CHECK(munmap(map + 4096, 4096) == 0);
		CHECK(map[0] == 7 && map[2 * 4096] == 7);
		CHECK(mprotect(map, 4096, PROT_READ) == 0);
		CHECK(map[0] == 7);
		CHECK(munmap(map, 5 * 4096) == 0);
		map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		CHECK(map != MAP_FAILED && map[100] == 0);
		munmap(map, 4096);
	}

	/* files and directories */
	CHECK(mkdir("stress_dir", 0755) == 0 || errno == EEXIST);
	for (index = 0; index < 300; index++)
	{
		FILE *file;
		snprintf(path, sizeof(path), "stress_dir/file_%03d.txt", index);
		file = fopen(path, "w");
		CHECK(file != NULL);
		if (file) { fprintf(file, "file %d\n", index); fclose(file); }
	}
	{
		DIR *directory = opendir("stress_dir");
		struct dirent *entry;
		int found = 0;
		long position = -1;
		CHECK(directory != NULL);
		while (directory && (entry = readdir(directory)))
		{
			if (entry->d_name[0] != '.') found++;
			if (found == 10 && position < 0) position = telldir(directory);
		}
		CHECK(found == 300);
		if (directory) { rewinddir(directory); found = 0;
			while ((entry = readdir(directory))) if (entry->d_name[0] != '.') found++;
			CHECK(found == 300); closedir(directory); }
	}
	{
		struct stat information;
		FILE *file = fopen("stress_dir/file_042.txt", "r");
		char line[64] = "";
		CHECK(stat("stress_dir/file_042.txt", &information) == 0 && information.st_size == 8 && S_ISREG(information.st_mode));
		CHECK(stat("stress_dir", &information) == 0 && S_ISDIR(information.st_mode));
		CHECK(stat("no_such_file", &information) != 0 && errno == ENOENT);
		CHECK(file && fgets(line, sizeof(line), file) && !strcmp(line, "file 42\n"));
		if (file) { CHECK(fseek(file, 5, SEEK_SET) == 0 && fgetc(file) == '4' && ftell(file) == 6); fclose(file); }
		CHECK(open("stress_dir/file_001.txt", O_WRONLY | O_CREAT | O_EXCL, 0644) < 0 && errno == EEXIST);
		CHECK(rename("stress_dir/file_001.txt", "stress_dir/renamed.txt") == 0);
		CHECK(unlink("stress_dir/renamed.txt") == 0);
	}
	{
		char cwd[512];
		CHECK(getcwd(cwd, sizeof(cwd)) != NULL && cwd[0] == '/');
	}
	{
		struct timespec delay = { 0, 20000000 };
		nanosleep(&delay, NULL);
		clock_gettime(CLOCK_MONOTONIC, &now);
		CHECK(now.tv_sec > start.tv_sec || now.tv_nsec > start.tv_nsec);
		CHECK(time(NULL) > 1700000000);
	}
	/* the Xbox window as port/linux/src/xbox_memory.c uses it: reserved,
	then 4 KB blocks inside one 16 KB host page, the renderer's write
	tracking on one of them */
	{
		unsigned char *window = (unsigned char *)0x80000000;
		unsigned char *block = window + 0x100000;
		unsigned int before, after;
		int descriptor;

		CHECK(mmap(window, 0x08000000, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | 0x100000, -1, 0) ==
			window);
		CHECK(mmap(block, 3 * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == block);
		CHECK(block[0] == 0 && block[3 * 4096 - 1] == 0);
		memset(block, 9, 3 * 4096);
		host_memory_watch_initialize();
		host_memory_watch_protect((unsigned int)block, 4096);
		before = host_memory_watch_generation((unsigned int)block, 4096);
		block[10] = 42;
		after = host_memory_watch_generation((unsigned int)block, 4096);
		CHECK(block[10] == 42 && after > before);
		/* the kernel writing into a watched page */
		host_memory_watch_protect((unsigned int)block, 4096);
		descriptor = open("stress_dir/file_042.txt", O_RDONLY);
		CHECK(descriptor >= 0 && read(descriptor, block + 100, 8) == 8 && block[100] == 'f');
		close(descriptor);
		/* a block freed between two others */
		CHECK(mmap(block + 4096, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0) ==
			block + 4096);
		CHECK(block[0] == 9 && block[2 * 4096] == 9);
		CHECK(mmap(block + 4096, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) ==
			block + 4096);
		CHECK(block[4096] == 0 && block[4096 + 4095] == 0 && block[2 * 4096] == 9);
	}
	printf("%s (%d failures)\n", failures ? "FAILED" : "ALL OK", failures);
	fflush(stdout);
	return failures != 0;
}
"""


def test_the_guest_runtime_under_load(tmp_path):
    """threads, locks, malloc's mappings at 4 KB, files and directories,
    through the guest's musl and the host's system calls, memory and threads
    (with a program of its own in place of the game)"""
    import shlex

    executable = ROOT / "build/macos/halo"
    objects = ROOT / "build/macos/guest/obj"
    lld = apple_silicon_tools()
    if not lld or not executable.is_file() or not (objects / "gen/imports.o").is_file():
        pytest.skip("needs an Apple silicon Mac and a macOS build (ninja macos)")
    source = tmp_path / "stress.c"
    source.write_text(GUEST_STRESS)
    # (compiled as the guest runtime is)
    runtime = "build/macos/guest/obj/port/android/guest/runtime/guest_sdl.o"
    command = subprocess.run(["ninja", "-t", "commands", runtime], cwd=ROOT, capture_output=True, text=True,
                             check=True).stdout.strip().splitlines()[-1]
    command = command.replace("port/android/guest/runtime/guest_sdl.c", str(source))
    command = command.replace(runtime, str(tmp_path / "stress.o")).replace(f"{runtime}.", str(tmp_path / "stress.o."))
    subprocess.run(command, shell=True, cwd=ROOT, check=True)
    runtime_objects = [path for path in sorted((objects / "port/android/guest/runtime").glob("*.o"))
                       if path.name != "guest_memory_watch.o"]
    musl = sorted((objects / "musl").rglob("*.o"))
    subprocess.run([lld, "-m", "aarch64linux", "-static", "-nostdlib", "-T", ROOT / "port/android/guest/guest.ld",
                    "-o", tmp_path / "halo_guest.elf", *runtime_objects, objects / "gen/imports.o",
                    objects / "guest/gen/guest_gl.o", objects / "guest/gen/guest_posix.o", tmp_path / "stress.o",
                    "--start-lib", *musl, "--end-lib"], cwd=ROOT, check=True)
    shutil.copy2(executable, tmp_path / "halo")
    result = subprocess.run([str(tmp_path / "halo")], cwd=tmp_path, capture_output=True, text=True, timeout=300)
    assert "ALL OK" in result.stdout, result.stdout + result.stderr
    assert result.returncode == 0
    assert "cannot be both" not in result.stderr


# ---------- the game, started without a window


def test_the_game_starts_and_exits(tmp_path):
    executable = ROOT / "build/macos/halo"
    image = ROOT / "build/macos/halo_guest.elf"
    if sys.platform != "darwin" or not executable.is_file() or not image.is_file():
        pytest.skip("needs a macOS build (ninja macos)")
    shutil.copy2(executable, tmp_path / "halo")
    shutil.copy2(image, tmp_path / "halo_guest.elf")
    environment = dict(os.environ, HALO_HIDDEN_WINDOW="1", HALO_EXIT_AFTER="2",
                       HALO_SAVE_ROOT=str(tmp_path / "saves"))
    result = subprocess.run([str(tmp_path / "halo")], cwd=tmp_path, env=environment, capture_output=True,
                            text=True, timeout=60)
    assert result.returncode == 0, result.stderr
    assert "the game exited (0)" in result.stderr
    assert "signal" not in result.stderr
    assert "cannot be both" not in result.stderr
    assert (tmp_path / "config.toml").is_file()
