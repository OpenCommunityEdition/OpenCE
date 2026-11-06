#!/usr/bin/env python3
"""Generate the Windows x64 host's import thunks (port/windows/host).

The guest image is the 64-bit x86 Linux build's (tools/linux_guest_build.py):
x32 code whose import stubs jump to the host's function with the arguments
where the System V ABI puts them, on the guest's own stack. A Windows host
cannot take them like that, for two reasons:

- Windows' x64 calling convention is another one: the first four
  arguments go in rcx, rdx, r8 and r9 or xmm0-xmm3 by position, the rest on
  the stack above 32 bytes the caller leaves for the callee, and rdi, rsi
  and xmm6-xmm15 belong to the caller;
- Windows code expects its stack to be the thread's: __chkstk, stack
  growth and exception dispatch compare the stack pointer with the stack's
  limits in the thread's TEB, and the guest's stack is somewhere else, below
  4 GB.

So each import gets a stub that jumps to a thunk for its signature (the
classes of its arguments, integer or floating-point, in order), with the
host function's address in r11. The thunk switches to the thread's own
stack and limits (port/windows/host/win64_thread.c, struct host_switch),
moves the arguments from the System V places to the Windows ones, calls,
and switches back; the result is in rax or xmm0 under both conventions.
Its unwind information makes its frame look like a call from the point where
the host last entered the guest (host_enter_guest, win64_switch.S), so that
Windows unwinds a host function's frames through it to the host's own.

The signatures come from the declarations the guest compiles, as
tools/check_host_imports.py reads them (which also checks that no argument
is variadic or a structure by value), and the OpenGL entry points' from the
generated guest_gl.c (tools/android_gl_stubs.py). The OpenGL functions'
addresses are found once a context exists (host_thunks_resolve_gl); every
other import is the host function of its name (posix_<name> for
hostposix_<name>), or the one --override names.

Usage: windows_host_thunks.py [--override NAME=SYMBOL]... --declarations FILE...
           --gl guest_gl.c thunks.S table.c LIST...
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_host_imports import classify, parse_prototypes  # noqa: E402

GL_FLOATS = {"GLfloat", "GLclampf", "GLdouble", "GLclampd", "float", "double"}
GL_DECLARATION = re.compile(r"^[A-Za-z_][\w \t\*]*?\b(hostgl_\w+)\s*\(([^;{}]*?)\)\s*;", re.M)

# the System V argument registers, and Windows'
SYSV_INTEGERS = ["%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"]
WINDOWS_INTEGERS = ["%rcx", "%rdx", "%r8", "%r9"]

# struct host_switch (win64_thread.c)
HOST_SP, GUEST_SP, HOST_BASE, HOST_LIMIT, GUEST_BASE, GUEST_LIMIT, HOST_DEALLOCATION, GUEST_DEALLOCATION = \
    0, 8, 16, 24, 32, 40, 48, 56
# the TEB's NT_TIB StackBase and StackLimit, and its DeallocationStack
TEB_STACK_BASE, TEB_STACK_LIMIT, TEB_DEALLOCATION_STACK = 8, 16, 0x1478


def read_lists(paths: List[str]) -> List[str]:
    names: List[str] = []
    for path in paths:
        for line in Path(path).read_text(encoding="utf-8").splitlines():
            line = line.split("#", 1)[0].strip()
            if line and line not in names:
                names.append(line)
    return names


def gl_signatures(path: str) -> Dict[str, str]:
    signatures = {}
    text = Path(path).read_text(encoding="utf-8")
    for name, parameters in GL_DECLARATION.findall(text):
        parameters = " ".join(parameters.split())
        classes = ""
        if parameters not in ("", "void"):
            for parameter in parameters.split(","):
                words = parameter.replace("const", " ").split()
                floating = "*" not in parameter and words and words[0] in GL_FLOATS
                classes += "f" if floating else "i"
        signatures[name] = classes
    return signatures


def host_signatures(paths: List[str]) -> Dict[str, str]:
    signatures: Dict[str, str] = {}
    for path in paths:
        for prototype in parse_prototypes(Path(path).read_text(encoding="utf-8", errors="replace"), path):
            classes = ""
            for parameter in prototype.parameters:
                kind, reason = classify(parameter)
                if kind == "bad":
                    raise SystemExit(f"{prototype.where}: {prototype.name}: argument {parameter!r} {reason}")
                classes += "f" if kind in ("float", "double") else "i"
            name = prototype.name
            if name.startswith("posix_"):
                name = "host" + name
            signatures.setdefault(name, classes)
    return signatures


def thunk(signature: str) -> List[str]:
    """the assembly of the thunk for a signature (r11: the function)"""
    count = len(signature)
    outgoing = 8 * max(4, count)
    spill = outgoing
    # (the System V places of each argument: a register or a stack slot)
    integers = floats = slots = 0
    sources: List[Tuple[str, object]] = []
    for kind in signature:
        if kind == "i" and integers < 6:
            sources.append(("ireg", integers))
            integers += 1
        elif kind == "f" and floats < 8:
            sources.append(("freg", floats))
            floats += 1
        else:
            sources.append(("stack", slots))
            slots += 1
    # (host_sp is 8 bytes off 16-byte alignment, win64_switch.S; with the
    # two values pushed, the frame aligns the call)
    frame = ((outgoing + 8 * integers + 15) & ~15) + 8
    name = f"host_thunk_{signature or 'v'}"
    out = [
        "",
        f"\t.def {name}; .scl 3; .type 32; .endef",
        "\t.p2align 4",
        f"{name}:",
        f"\t.seh_proc {name}",
        "\tmovl host_switch_tls_offset(%rip), %r10d",
        "\tmovq %gs:(%r10), %r10",
        "\tmovq %rsp, %rax",
        f"\tmovq {HOST_SP}(%r10), %rsp",
        "\tpushq %rax",
        f"\tpushq {GUEST_SP}(%r10)",
        "\t.seh_stackalloc 16",
        f"\tmovq %rax, {GUEST_SP}(%r10)",
        f"\tsubq ${frame}, %rsp",
        f"\t.seh_stackalloc {frame}",
        "\t.seh_endprologue",
    ]
    # the integer registers, kept aside: Windows' argument registers are
    # System V's too, in other places
    for index in range(integers):
        out.append(f"\tmovq {SYSV_INTEGERS[index]}, {spill + 8 * index}(%rsp)")
    # floating-point registers: the k-th floating-point argument (xmmk)
    # goes to its position p >= k, so from the last one down nothing is
    # overwritten before it is moved
    for position in reversed(range(count)):
        kind, index = sources[position]
        if kind != "freg":
            continue
        if position < 4:
            if position != index:
                out.append(f"\tmovaps %xmm{index}, %xmm{position}")
        else:
            out.append(f"\tmovsd %xmm{index}, {8 * position}(%rsp)")
    # the guest's stack arguments (above its return address)
    for position in range(count):
        kind, index = sources[position]
        if kind != "stack":
            continue
        source = f"{8 + 8 * index}(%rax)"
        if position < 4:
            if signature[position] == "f":
                out.append(f"\tmovsd {source}, %xmm{position}")
            else:
                out.append(f"\tmovq {source}, {WINDOWS_INTEGERS[position]}")
        else:
            out.append(f"\tmovq {source}, %rsi")
            out.append(f"\tmovq %rsi, {8 * position}(%rsp)")
    # the thread's own stack limits
    out += [
        f"\tmovq {HOST_BASE}(%r10), %rsi",
        f"\tmovq %rsi, %gs:{TEB_STACK_BASE}",
        f"\tmovq {HOST_LIMIT}(%r10), %rsi",
        f"\tmovq %rsi, %gs:{TEB_STACK_LIMIT}",
        f"\tmovq {HOST_DEALLOCATION}(%r10), %rsi",
        f"\tmovq %rsi, %gs:{TEB_DEALLOCATION_STACK}",
    ]
    # the integer register arguments, last (some were loaded from the guest's
    # stack above into the same registers only when they had no register)
    for position in range(count):
        kind, index = sources[position]
        if kind != "ireg":
            continue
        if position < 4:
            out.append(f"\tmovq {spill + 8 * index}(%rsp), {WINDOWS_INTEGERS[position]}")
        else:
            out.append(f"\tmovq {spill + 8 * index}(%rsp), %rsi")
            out.append(f"\tmovq %rsi, {8 * position}(%rsp)")
    out += [
        "\tcallq *%r11",
        # back: the stack limit may have grown, then the guest's
        "\tmovl host_switch_tls_offset(%rip), %r10d",
        "\tmovq %gs:(%r10), %r10",
        f"\tmovq %gs:{TEB_STACK_LIMIT}, %rcx",
        f"\tmovq %rcx, {HOST_LIMIT}(%r10)",
        f"\tmovq {GUEST_BASE}(%r10), %rcx",
        f"\tmovq %rcx, %gs:{TEB_STACK_BASE}",
        f"\tmovq {GUEST_LIMIT}(%r10), %rcx",
        f"\tmovq %rcx, %gs:{TEB_STACK_LIMIT}",
        f"\tmovq {GUEST_DEALLOCATION}(%r10), %rcx",
        f"\tmovq %rcx, %gs:{TEB_DEALLOCATION_STACK}",
        f"\taddq ${frame}, %rsp",
        f"\tpopq {GUEST_SP}(%r10)",
        "\tpopq %rsp",
        "\tretq",
        "\t.seh_endproc",
    ]
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    parser.add_argument("--override", action="append", default=[], metavar="NAME=SYMBOL")
    parser.add_argument("--declarations", nargs="+", required=True)
    parser.add_argument("--gl", required=True)
    parser.add_argument("thunks")
    parser.add_argument("table")
    parser.add_argument("lists", nargs="+")
    options = parser.parse_args()

    overrides = dict(item.split("=", 1) for item in options.override)
    names = read_lists(options.lists)
    signatures = host_signatures(options.declarations)
    signatures.update(gl_signatures(options.gl))
    missing = [name for name in names if name not in signatures]
    if missing:
        raise SystemExit("windows_host_thunks: no declaration of " + ", ".join(missing))

    used = sorted({signatures[name] for name in names}, key=lambda s: (len(s), s))
    asm = ["/* generated by tools/windows_host_thunks.py; do not edit */", "\t.text"]
    for signature in used:
        asm += thunk(signature)
    asm.append("")
    for index, name in enumerate(names):
        asm += [
            f"\t.globl host_stub_{index}",
            f"\t.def host_stub_{index}; .scl 2; .type 32; .endef",
            "\t.p2align 3",
            f"host_stub_{index}:",
            f"\tmovq host_thunk_targets+{8 * index}(%rip), %r11",
            f"\tjmp host_thunk_{signatures[name] or 'v'}",
        ]
    Path(options.thunks).write_text("\n".join(asm) + "\n", encoding="utf-8")

    def symbol(name: str) -> str:
        if name in overrides:
            return overrides[name]
        return "posix_" + name[len("hostposix_"):] if name.startswith("hostposix_") else name

    host_names = [name for name in names if not name.startswith("hostgl_")]
    table = [
        "/* generated by tools/windows_host_thunks.py; do not edit */",
        "#include <string.h>",
        "",
        f"#define HOST_THUNK_COUNT {len(names)}",
        "",
        "void host_thunk_unresolved(void);",
        *[f"void host_stub_{index}(void);" for index in range(len(names))],
        *sorted({f"void {symbol(name)}(void);" for name in host_names}),
        "",
        "/* the functions the stubs call (win64_switch.S) */",
        "void *host_thunk_targets[HOST_THUNK_COUNT] =",
        "{",
        *[f"\t(void *){symbol(name) if not name.startswith('hostgl_') else 'host_thunk_unresolved'},"
          for name in names],
        "};",
        "",
        "static const struct { const char *name; void (*stub)(void); } imports[HOST_THUNK_COUNT] =",
        "{",
        *[f'\t{{ "{name}", host_stub_{index} }},' for index, name in enumerate(names)],
        "};",
        "",
        "/* the stub of an import, which the host's loader puts in the guest's table */",
        "void *host_resolve_import(const char *name)",
        "{",
        "\tint index;",
        "",
        "\tfor (index = 0; index < HOST_THUNK_COUNT; index++)",
        "\t{",
        "\t\tif (!strcmp(imports[index].name, name))",
        "\t\t\treturn (void *)imports[index].stub;",
        "\t}",
        "\treturn NULL;",
        "}",
        "",
        "/* the OpenGL entry points, once a context is current; the number still",
        "unresolved */",
        "int host_thunks_resolve_gl(void *(*resolve)(const char *name))",
        "{",
        "\tint index, missing = 0;",
        "",
        "\tfor (index = 0; index < HOST_THUNK_COUNT; index++)",
        "\t{",
        "\t\tvoid *function;",
        "",
        "\t\tif (strncmp(imports[index].name, \"hostgl_\", 7) || host_thunk_targets[index] != (void *)host_thunk_unresolved)",
        "\t\t\tcontinue;",
        "\t\tfunction = resolve(imports[index].name + 7);",
        "\t\tif (function)",
        "\t\t\thost_thunk_targets[index] = function;",
        "\t\telse",
        "\t\t\tmissing++;",
        "\t}",
        "\treturn missing;",
        "}",
    ]
    Path(options.table).write_text("\n".join(table) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
