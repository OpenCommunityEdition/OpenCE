"""The guest's architectures.

The builds that run the game as a guest image - ILP32 code, 32-bit pointers
as the game's data formats require, in a 64-bit process - are Android
(tools/android_build.py) and the desktop guest builds (tools/linux_guest_build.py).
Everything about the guest that depends on its processor is described here,
once per architecture, and read by the guest image's graph
(generate_guest_image in tools/android_build.py) and by its generators:
tools/android_imports.py (the stubs that call the host) and
tools/android_gl_stubs.py (the OpenGL entry points).

This module imports nothing from the build scripts, so that the generators,
which ninja runs as scripts, can import it too.
"""

from dataclasses import dataclass
from typing import Dict, Optional, Tuple


@dataclass(frozen=True)
class GuestArch:
    # the name the generators are given (--arch), and the guest's libc
    # folder, port/android/guest/libc/arch/<name>
    name: str
    # clang's target and code generation for this guest, and what hides the
    # target's own platform from the sources
    target_flags: Tuple[str, ...]
    # the defines that select this architecture's code paths in the sources,
    # beside every guest's HALO_GUEST (tools/android_build.py's
    # guest_abi_flags; the instruction set is mostly told by the compiler's
    # own __aarch64__ or __x86_64__)
    defines: Tuple[str, ...]
    # true when clang emits Mach-O assembly for this target, which
    # tools/android_asm_convert.py turns into ELF assembly before it is
    # assembled; false when clang writes the ELF object itself
    darwin_assembly: bool
    # the architecture of the ELF triple the assembly is assembled with
    # (assembler_target), what that triple's environment ends with (x32's
    # gnux32), and ld.lld's emulation for the image
    elf_arch: str
    environment_suffix: str
    ld_emulation: str
    # the address the image is linked at (port/android/guest/guest.ld), which
    # must be HALO_GUEST_IMAGE_BASE of port/android/include/halo_android_abi.h
    # for this architecture (the host loads the image there, and checks it)
    image_base: int
    # the flags with which the compiler is asked for its runtime library
    # (-print-libgcc-file-name), the builtins the image links with; None
    # for a guest that links none, whose image must then need none (the
    # link fails on any function it would have taken from them)
    builtins_flags: Optional[Tuple[str, ...]]
    # the assembly of one host import's stub, which loads the host's 64-bit
    # address from __host_import_table+{offset} and jumps to it, leaving the
    # arguments as they are ({name} and {offset} are filled in)
    import_stub: str
    # the host's calling convention for the OpenGL stubs: the integer and
    # pointer arguments it takes in registers, and whether the guest packs
    # the stack's arguments by size where the host has 8-byte slots (so that
    # those arguments are passed as 64-bit values)
    integer_registers: int
    packed_stack_arguments: bool
    # the processor features zlib must not use, which the guest's assembly
    # step is not told it may
    zlib_flags: Tuple[str, ...]
    # whether the guest sees the compiler's own headers (its intrinsics), after
    # the C library's: the guest is compiled -nostdinc
    compiler_headers: bool
    # what the disassembly of an instruction that uses the processor's thread
    # pointer, which belongs to the host, looks like (regular expressions,
    # tools/guest_image_check.py, matched ignoring case)
    thread_pointer_instructions: Tuple[str, ...]

    def assembler_target(self, environment: str) -> str:
        """the triple the guest's assembly is assembled with, for the
        environment (android, gnu) of the toolchain"""
        return f"{self.elf_arch}-linux-{environment}{self.environment_suffix}"

    def builtins(self, compiler: str) -> str:
        """the ninja text of the runtime library's path, as compiler names
        it, or nothing for a guest without one"""
        if self.builtins_flags is None:
            return ""
        return f"$$({' '.join((compiler, *self.builtins_flags, '-print-libgcc-file-name'))})"


# AArch64 with 32-bit pointers. clang's only such target is Apple's
# arm64_32, whose Mach-O assembly is converted afterwards; the Darwin
# environment is hidden from the sources, and the C library is musl.
ARM64_32 = GuestArch(
    name="arm64_32",
    target_flags=(
        "--target=arm64_32-apple-watchos",
        "-U__APPLE__",
        "-U__MACH__",
        "-fno-define-target-os-macros",
        "-D__linux__=1",
        "-D__unix__=1",
        # ARMv8.0: nothing the emulator's binary translation or an older
        # device could lack (Darwin targets otherwise assume pointer
        # authentication and FP16)
        "-mcpu=cortex-a53",
        # (NEON's generic syntax, which the conversion to ELF assembly reads)
        "-mllvm",
        "-aarch64-neon-syntax=generic",
    ),
    # (HALO_GUEST on AArch64, by its name before HALO_GUEST)
    defines=("-DHALO_ARM64_GUEST=1",),
    darwin_assembly=True,
    elf_arch="aarch64",
    environment_suffix="",
    ld_emulation="aarch64linux",
    # (just above the Xbox window: Android's ART fills the low 4 GB upwards)
    image_base=0x88000000,
    # (the 64-bit AArch64 library, whose functions take and return the same
    # registers in the guest)
    builtins_flags=(),
    import_stub=(
        "\t.globl {name}\n"
        "\t.type {name},%function\n"
        "\t.p2align 2\n"
        "{name}:\n"
        "\tadrp x16, __host_import_table+{offset}\n"
        "\tldr x16, [x16, :lo12:__host_import_table+{offset}]\n"
        "\tbr x16"
    ),
    integer_registers=8,
    packed_stack_arguments=True,
    zlib_flags=("-U__ARM_FEATURE_CRC32",),
    compiler_headers=False,
    thread_pointer_instructions=(r"\btpidr(ro)?_el0\b",),
)

# x86-64 with 32-bit pointers: clang's x32 target (x86_64-linux-gnux32),
# which writes ELF objects itself, with musl. The code runs in a 64-bit
# process and never enters the kernel itself, so the kernel's own x32
# support (CONFIG_X86_X32_ABI, off in most distributions) is not needed.
X32 = GuestArch(
    name="x32",
    target_flags=(
        "--target=x86_64-linux-gnux32",
        # (static code at a fixed address: absolute addresses fit the 32-bit
        # sign-extended immediates of the small code model, as the image lies
        # below 2 GB, image_base)
        "-fno-pic",
        "-fno-pie",
        # every x86-64 processor (SSE2), as the portable x86 builds
        "-march=x86-64",
        # MSVC's long double, and arm64_32's: a double (x87's 80-bit one
        # would be computed differently, and laid out differently)
        "-mlong-double-64",
    ),
    defines=(),
    darwin_assembly=False,
    elf_arch="x86_64",
    environment_suffix="x32",
    ld_emulation="elf32_x86_64",
    # Below the 2 GB line: x32's static code reaches its own symbols through
    # 32-bit immediates and displacements that are sign-extended (the
    # R_X86_64_32S relocations of absolute addressing modes), so a symbol at
    # or above 0x80000000 cannot be addressed; the Xbox window, which the
    # code reaches only through registers, is above it.
    image_base=0x70000000,
    # (none: x86-64 has every operation the guest's integers and floats
    # need, so clang calls no runtime function, and there is no x32 runtime
    # library to take one from; refer to port/linux/README.md, "64-bit x86")
    builtins_flags=None,
    import_stub=(
        "\t.globl {name}\n"
        "\t.type {name},@function\n"
        "\t.p2align 3\n"
        "{name}:\n"
        "\tjmpq *__host_import_table+{offset}(%rip)"
    ),
    integer_registers=6,
    # (each argument on the stack has an 8-byte slot in either ABI; clang's
    # x32 code writes a pointer there zero-extended, all 8 bytes, but the
    # stubs do not rely on it and pass a pointer there as a 64-bit value
    # that they extend themselves, tools/android_gl_stubs.py)
    packed_stack_arguments=False,
    zlib_flags=(),
    # (SSE's xmmintrin.h, which Opus uses as on the x86 build)
    compiler_headers=True,
    # (%fs is the host's thread pointer, %gs no one's: any segment override,
    # and the instructions that read or write either base)
    thread_pointer_instructions=(r"%[fg]s:", r"\b(rd|wr)[fg]sbase\b"),
)

GUEST_ARCHES: Dict[str, GuestArch] = {arch.name: arch for arch in (ARM64_32, X32)}
