#!/usr/bin/env python3
"""Generate the macOS host's import dispatcher (port/macos/host).

The guest's import stubs (tools/android_imports.py --svc) exit to the host
with the import's index in w16; the index is the import's place in the same
lists the stubs are made from. For each import this writes a case that reads
its arguments from the vCPU's registers as its prototype says (integers from
x0-x7, floats from d0-d7, the rest from the guest's stack), turns guest
pointers into host pointers (guest NULL stays NULL), calls the host
function, and puts its result in x0 or d0. Arguments past the registers are
on the stack as Apple's AArch64 ABI has them, which the arm64_32 guest
follows: each at its own size's alignment (4 bytes for an int or a guest
pointer, 8 for a 64-bit value), not in 8-byte slots.

The prototypes are those of port/android/guest/runtime/guest_host.h (the
host_* imports) and port/linux/src/posix.h (each hostposix_<name> import is
the host's posix_<name>). The OpenGL ES imports (hostgl_*) are not carried
out while the host is headless: they return 0 and are reported once.

Usage: macos_dispatch.py --prototypes header.h [--prototypes ...] output.c list...
"""

import re
import sys

# pointer-typed values that are not guest pointers: the directory handles of
# port/linux/src/posix_files.c, small integers cast to void *
RAW_PARAMETERS = {("posix_directory_next", "directory"), ("posix_directory_close", "directory")}
RAW_RESULTS = {"posix_directory_open"}
# results that are the host's errno values, which the guest reads as Linux's
# (none: posix_socket_last_error gives Windows Sockets codes, which
# port/linux/src/posix_net.c makes from the host's own errno names; the
# guest's errno after an import is host_errno's, which host_imports.c
# translates)
ERRNO_RESULTS = set()
SIXTY_FOUR_BIT = {"long long", "unsigned long long", "int64_t", "uint64_t"}
FLOATS = {"float", "double"}


def parse_prototypes(paths):
    prototypes = {}
    for path in paths:
        text = open(path, encoding="utf-8").read()
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        text = re.sub(r"__attribute__\s*\(\(.*?\)\)", "", text)
        for result, name, parameters in re.findall(
                r"^([A-Za-z_][\w \*]*?)\b((?:host|posix)_\w+)\s*\(([^;{]*?)\)\s*;", text, re.M | re.S):
            parameters = " ".join(parameters.split())
            parsed = []
            if parameters not in ("", "void"):
                for parameter in parameters.split(","):
                    parameter = parameter.strip()
                    match = re.match(r"(.*?)(\w+)$", parameter)
                    parsed.append((" ".join(match.group(1).split()), match.group(2)))
            prototypes[name] = (" ".join(result.split()), parsed)
    return prototypes


def read_names(lists):
    names = []
    for path in lists:
        for line in open(path, encoding="utf-8"):
            line = line.split("#", 1)[0].strip()
            if line and line not in names:
                names.append(line)
    return names


def argument(function, kind, name, position):
    """the C expression for one argument; position is [integer registers,
    floating-point registers, stack bytes] used so far, and is updated"""
    if kind in FLOATS:
        if position[1] < 8:
            reader = "host_call_float" if kind == "float" else "host_call_double"
            position[1] += 1
            return f"{reader}(call->d[{position[1] - 1}])"
        size = 4 if kind == "float" else 8
        value = f"host_call_stack{size * 8}(call, {stack_offset(position, size)})"
        return f"host_call_float({value})" if kind == "float" else f"host_call_double({value})"
    if position[0] < 8:
        value = f"call->x[{position[0]}]"
        position[0] += 1
    else:
        size = 8 if kind in SIXTY_FOUR_BIT else 4
        value = f"host_call_stack{size * 8}(call, {stack_offset(position, size)})"
    if "*" in kind:
        if (function, name) in RAW_PARAMETERS:
            return f"({kind})(uintptr_t)(uint32_t){value}"
        return f"({kind})host_guest_pointer((uint32_t){value})"
    return f"({kind}){value}"


def stack_offset(position, size):
    """the next stack argument's offset from the guest's stack pointer"""
    offset = (position[2] + size - 1) // size * size
    position[2] = offset + size
    return offset


def case(index, name, prototypes):
    if name.startswith("hostgl_"):
        return [f"\tcase {index}: /* {name}: no OpenGL ES while headless */",
                f'\t\thost_dispatch_unsupported(call, {index}, "{name}");',
                "\t\treturn 1;"]
    function = "posix_" + name[len("hostposix_"):] if name.startswith("hostposix_") else name
    if function not in prototypes:
        raise ValueError(f"no prototype for the import {name}")
    result, parameters = prototypes[function]
    position = [0, 0, 0]
    arguments = [argument(function, kind, parameter, position) for kind, parameter in parameters]
    call = f"{function}({', '.join(arguments)})"
    lines = [f"\tcase {index}: /* {name} */"]
    if result == "void":
        lines.append(f"\t\t{call};")
    elif result in FLOATS:
        setter = "host_call_set_float" if result == "float" else "host_call_set_double"
        lines.append(f"\t\tcall->result_d = {setter}({call});")
    elif function in RAW_RESULTS:
        lines.append(f"\t\tcall->result_x = (uint32_t)(uintptr_t){call};")
    elif function in ERRNO_RESULTS:
        lines.append(f"\t\tcall->result_x = (uint32_t)host_linux_errno({call});")
    elif result in SIXTY_FOUR_BIT:
        lines.append(f"\t\tcall->result_x = (uint64_t){call};")
    elif "*" in result:
        raise ValueError(f"{function} returns a host pointer, which the guest cannot hold")
    else:
        lines.append(f"\t\tcall->result_x = (uint32_t){call};")
    if function != "host_errno":
        lines.append("\t\thost_saved_errno = errno;")
    lines.append("\t\treturn 1;")
    return lines


def main():
    arguments = sys.argv[1:]
    headers = []
    while arguments and arguments[0] == "--prototypes":
        headers.append(arguments[1])
        arguments = arguments[2:]
    output, lists = arguments[0], arguments[1:]
    prototypes = parse_prototypes(headers)
    names = read_names(lists)
    out = [
        "/* generated by tools/macos_dispatch.py; do not edit */",
        "",
        '#include "host.h"',
        '#include "guest_host.h"',
        '#include "posix.h"',
        "",
        "#include <errno.h>",
        "#include <string.h>",
        "",
        "__thread int host_saved_errno;",
        "",
        "static float host_call_float(uint64_t bits)",
        "{",
        "\tuint32_t low = (uint32_t)bits;",
        "\tfloat value;",
        "",
        "\tmemcpy(&value, &low, sizeof(value));",
        "\treturn value;",
        "}",
        "",
        "static double host_call_double(uint64_t bits)",
        "{",
        "\tdouble value;",
        "",
        "\tmemcpy(&value, &bits, sizeof(value));",
        "\treturn value;",
        "}",
        "",
        "/* a stack argument: `offset` bytes above the guest's stack pointer */",
        "static uint64_t host_call_stack32(const struct host_call *call, uint32_t offset)",
        "{",
        "\tuint32_t value;",
        "",
        "\tmemcpy(&value, host_guest_pointer(call->sp + offset), sizeof(value));",
        "\treturn value;",
        "}",
        "",
        "static uint64_t host_call_stack64(const struct host_call *call, uint32_t offset)",
        "{",
        "\tuint64_t value;",
        "",
        "\tmemcpy(&value, host_guest_pointer(call->sp + offset), sizeof(value));",
        "\treturn value;",
        "}",
        "",
        "static uint64_t host_call_set_float(float value)",
        "{",
        "\tuint32_t bits;",
        "",
        "\tmemcpy(&bits, &value, sizeof(bits));",
        "\treturn bits;",
        "}",
        "",
        "static uint64_t host_call_set_double(double value)",
        "{",
        "\tuint64_t bits;",
        "",
        "\tmemcpy(&bits, &value, sizeof(bits));",
        "\treturn bits;",
        "}",
        "",
        f"static unsigned char unsupported_reported[{max(len(names), 1)}];",
        "",
        "/* an import the host does not carry out: it returns 0 */",
        "static void host_dispatch_unsupported(struct host_call *call, uint32_t index, const char *name)",
        "{",
        "\tif (!unsupported_reported[index])",
        "\t{",
        "\t\tunsupported_reported[index] = 1;",
        '\t\thost_logf(HOST_LOG_WARN, "the guest called %s, which this host does not carry out", name);',
        "\t}",
        "\tcall->result_x = 0;",
        "\tcall->result_d = 0;",
        "}",
        "",
        "int host_dispatch(struct host_call *call, uint32_t index)",
        "{",
        "\t(void)host_call_float;",
        "\t(void)host_call_double;",
        "\t(void)host_call_set_float;",
        "\t(void)host_call_set_double;",
        "\t(void)host_call_stack32;",
        "\t(void)host_call_stack64;",
        "\tswitch (index)",
        "\t{",
    ]
    try:
        for index, name in enumerate(names):
            out += case(index, name, prototypes)
    except ValueError as error:
        sys.exit(f"macos_dispatch.py: {error}")
    out += ["\tdefault:", "\t\treturn 0;", "\t}", "}"]
    with open(output, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
