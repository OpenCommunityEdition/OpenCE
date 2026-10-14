#!/usr/bin/env python3
"""Check a linked guest image (tools/android_build.py, generate_guest_image)
for what the host cannot give it.

The guest shares its process with the host, whose C library owns the
processor's thread pointer (AArch64's tpidr_el0, x86-64's %fs; %gs is
unused by Linux and must stay so): the guest's thread-local variables are
emulated (-femulated-tls) and its thread pointer is the host's to keep
(port/android/guest/runtime/guest_thread.c). Code that read or wrote the
thread pointer would use the host's thread's data as its own. For the
arm64_32 guest, tools/android_asm_convert.py also refuses thread-local
storage as it converts the assembly; the x32 guest's objects come from
clang directly, and nothing else would notice.

This checks that the image

- is a static executable of the architecture's class and machine;
- has no thread-local storage (a PT_TLS segment, or .tdata or .tbss
  sections);
- has its loadable segments inside the range the host reserves for it,
  from the architecture's image_base (tools/guest_arch.py), which must be
  the host's HALO_GUEST_IMAGE_BASE (port/android/include/halo_android_abi.h);
- has constructors (.guest_init, which __guest_start calls as 4-byte
  pointers: port/android/guest/guest.ld) that are each the address of code
  in the image, so that an 8-byte entry (an ELF64 object's .init_array: the
  64-bit runtime library an arm64_32 guest links) is refused, and, with the
  link's map (--map), that come only from the architecture's input
  sections: a Mach-O guest's converted .guest_init, an ELF guest's
  .init_array;
- has no instruction that uses the thread pointer
  (GuestArch.thread_pointer_instructions), from llvm-objdump's disassembly.

Usage: guest_image_check.py --arch NAME --objdump LLVM_OBJDUMP [--map MAP] image stamp
Exits 1 and names each problem when there is one; otherwise touches stamp.
"""

import argparse
import re
import struct
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

from guest_arch import GUEST_ARCHES, GuestArch  # noqa: E402

# the room the host reserves for the image (HALO_GUEST_IMAGE_RESERVE of
# port/android/include/halo_android_abi.h: tools/test_guest_layout.py)
IMAGE_RESERVE = 0x04000000
PT_LOAD, PT_DYNAMIC, PT_INTERP, PT_TLS = 1, 2, 3, 7
PF_X = 1
MACHINES = {"aarch64": 183, "x86_64": 62}


def elf_problems(data: bytes, arch: GuestArch) -> List[str]:
    """the problems with the ELF file's headers"""
    if data[:4] != b"\x7fELF":
        return ["not an ELF file"]
    elf_class = data[4]
    if elf_class not in (1, 2):
        return [f"ELF class {elf_class}"]
    wide = elf_class == 2
    if wide:
        machine, = struct.unpack_from("<H", data, 18)
        phoff, shoff = struct.unpack_from("<QQ", data, 32)
        phentsize, phnum, shentsize, shnum, shstrndx = struct.unpack_from("<HHHHH", data, 54)
    else:
        machine, = struct.unpack_from("<H", data, 18)
        phoff, shoff = struct.unpack_from("<II", data, 28)
        phentsize, phnum, shentsize, shnum, shstrndx = struct.unpack_from("<HHHHH", data, 42)
    problems = []
    if machine != MACHINES[arch.elf_arch]:
        problems.append(f"machine {machine}, not {arch.elf_arch}'s {MACHINES[arch.elf_arch]}")
    # (x32's images are ELF32; arm64_32's, ELF64, as AArch64 Linux has no
    # ELF32 executables)
    expected_class = 1 if arch.ld_emulation.startswith("elf32") else 2
    if elf_class != expected_class:
        problems.append(f"ELF class {elf_class}, not {expected_class}")
    reserve_end = arch.image_base + IMAGE_RESERVE
    lowest = None
    code = []
    for index in range(phnum):
        offset = phoff + index * phentsize
        if wide:
            kind, flags, _, address, _, _, memory_size = struct.unpack_from("<IIQQQQQ", data, offset)
        else:
            kind, _, address, _, _, memory_size, flags = struct.unpack_from("<IIIIIII", data, offset)
        if kind == PT_LOAD and flags & PF_X:
            code.append((address, address + memory_size))
        if kind in (PT_DYNAMIC, PT_INTERP):
            problems.append(f"segment {index} is {'PT_DYNAMIC' if kind == PT_DYNAMIC else 'PT_INTERP'}: "
                            "the image must be static")
        elif kind == PT_TLS:
            problems.append(f"segment {index} is PT_TLS: thread-local storage, which the guest cannot have "
                            "(compile with -femulated-tls)")
        elif kind == PT_LOAD and memory_size:
            if address < arch.image_base or address + memory_size > reserve_end:
                problems.append(f"segment {index} ({address:#x}, {memory_size:#x} bytes) is outside "
                                f"{arch.image_base:#x}-{reserve_end:#x}")
            lowest = address if lowest is None else min(lowest, address)
    if lowest is not None and lowest & ~0xfff != arch.image_base:
        problems.append(f"the image starts at {lowest:#x}, not at its base {arch.image_base:#x}")
    if shoff and shstrndx < shnum:
        names_offset = struct.unpack_from("<QQ" if wide else "<II", data,
                                          shoff + shstrndx * shentsize + (24 if wide else 16))[0]
        for index in range(shnum):
            header = shoff + index * shentsize
            name_offset, = struct.unpack_from("<I", data, header)
            end = data.index(b"\0", names_offset + name_offset)
            name = data[names_offset + name_offset:end].decode("ascii", "replace")
            if name == ".tdata" or name == ".tbss" or name.startswith((".tdata.", ".tbss.")):
                problems.append(f"section {name}: thread-local storage, which the guest cannot have")
            elif name == ".guest_init":
                if wide:
                    section_offset, size = struct.unpack_from("<QQ", data, header + 24)
                else:
                    section_offset, size = struct.unpack_from("<II", data, header + 16)
                problems += constructor_problems(data[section_offset:section_offset + size], code)
    return problems


def constructor_problems(entries: bytes, code: List[Tuple[int, int]]) -> List[str]:
    """the entries of .guest_init that are not 4-byte addresses of code"""
    if len(entries) % 4:
        return [f".guest_init is {len(entries)} bytes, not a whole number of 4-byte constructors"]
    problems = []
    for index in range(0, len(entries), 4):
        address, = struct.unpack_from("<I", entries, index)
        if not any(start <= address < end for start, end in code):
            problems.append(f".guest_init's constructor {index // 4} ({address:#x}) is not the address of code "
                            "(an 8-byte entry, as a 64-bit object's .init_array has?)")
    return problems


def map_problems(link_map: str, arch: GuestArch) -> List[str]:
    """the input sections of .guest_init, as ld.lld's map lists them, that
    are not the architecture's constructors"""
    expected = re.compile(r"\.guest_init" if arch.darwin_assembly else r"\.init_array(\.\d+)?")
    problems = []
    inside = False
    for line in link_map.splitlines():
        # (VMA, LMA, size, alignment, and then the output section's name,
        # an input section's 8 columns further, a symbol's 16)
        match = re.match(r"\s*[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\d+ (\s*)(\S.*)$", line)
        if not match:
            continue
        size, indent, what = int(match.group(1), 16), len(match.group(2)), match.group(3)
        if not indent:
            inside = what == ".guest_init"
        elif inside and indent == 8 and " = " not in what:
            section = re.search(r":\(([^)+]+)(\+0x[0-9a-fA-F]+)?\)$", what)
            if not section or not expected.fullmatch(section.group(1)):
                problems.append(f".guest_init takes {what}, not only {arch.name}'s constructors "
                                f"({expected.pattern})")
            elif size % 4:
                problems.append(f".guest_init takes {what}, {size} bytes: not 4-byte constructors")
    return problems


def instruction_problems(image: Path, arch: GuestArch, objdump: str) -> List[str]:
    """the instructions of the image that use the thread pointer"""
    if not arch.thread_pointer_instructions:
        return []
    pattern = re.compile("|".join(arch.thread_pointer_instructions), re.IGNORECASE)
    disassembly = subprocess.run([objdump, "-d", "--no-show-raw-insn", str(image)], capture_output=True,
                                 text=True, check=True).stdout
    function = "?"
    problems = []
    for line in disassembly.splitlines():
        if line.endswith(">:"):
            function = line.split("<", 1)[-1][:-2]
        elif pattern.search(line):
            problems.append(f"{function} uses the host's thread pointer: {line.strip()}")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--arch", required=True, choices=sorted(GUEST_ARCHES))
    parser.add_argument("--objdump", default="llvm-objdump")
    parser.add_argument("--map", type=Path, help="ld.lld's map of the link (-Map), to check .guest_init's inputs")
    parser.add_argument("image", type=Path)
    parser.add_argument("stamp", type=Path)
    args = parser.parse_args()
    arch = GUEST_ARCHES[args.arch]
    problems = elf_problems(args.image.read_bytes(), arch)
    if args.map:
        problems += map_problems(args.map.read_text(errors="replace"), arch)
    if not problems:
        problems = instruction_problems(args.image, arch, args.objdump)
    for problem in problems[:50]:
        print(f"{args.image}: {problem}", file=sys.stderr)
    if problems:
        return 1
    args.stamp.write_text("")
    return 0


if __name__ == "__main__":
    sys.exit(main())
