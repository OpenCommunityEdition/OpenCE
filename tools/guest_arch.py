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
from typing import Dict, Tuple


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
    # (assembler_target), and ld.lld's emulation for the image
    elf_arch: str
    ld_emulation: str
    # the flags with which the compiler is asked for its runtime library
    # (-print-libgcc-file-name), the builtins the image links with
    builtins_flags: Tuple[str, ...]
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

    def assembler_target(self, environment: str) -> str:
        """the triple the guest's assembly is assembled with, for the
        environment (android, gnu) of the toolchain"""
        return f"{self.elf_arch}-linux-{environment}"

    def builtins(self, compiler: str) -> str:
        """the ninja text of the runtime library's path, as compiler names it"""
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
    ld_emulation="aarch64linux",
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
)

GUEST_ARCHES: Dict[str, GuestArch] = {arch.name: arch for arch in (ARM64_32,)}
