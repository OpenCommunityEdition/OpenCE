#!/usr/bin/env python3

import sys


def stub(name, offset, arch):
    if arch == "x86_64":
        return [f"\tjmp *__host_import_table+{offset}(%rip)"]
    return [f"\tadrp x16, __host_import_table+{offset}",
            f"\tldr x16, [x16, :lo12:__host_import_table+{offset}]",
            "\tbr x16"]


def main():
    arch, flag, host_table, output, *lists = sys.argv[1:]
    assert flag == "--host-table"
    names = []
    for path in lists:
        for line in open(path, encoding="utf-8"):
            line = line.split("#", 1)[0].strip()
            if line and line not in names:
                names.append(line)
    out = ["\t.text"]
    for index, name in enumerate(names):
        out += [f"\t.globl {name}", f"\t.type {name},%function", "\t.p2align 2", f"{name}:",
                *stub(name, index * 8, arch)]
    out += ["\t.section .rodata", "\t.globl __host_import_names", "__host_import_names:"]
    out += [f'\t.asciz "{name}"' for name in names]
    out += ["\t.p2align 2", "\t.globl __host_import_count", "__host_import_count:", f"\t.long {len(names)}",
            "\t.data", "\t.p2align 3", "\t.globl __host_import_table", "__host_import_table:",
            f"\t.zero {max(len(names), 1) * 8}"]
    with open(output, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")

    def symbol(name):
        return "posix_" + name[len("hostposix_"):] if name.startswith("hostposix_") else name

    host_names = [name for name in names if not name.startswith("hostgl_")]
    table = ["#include <string.h>", ""]
    table += [f"extern void {symbol(name)}(void);" for name in host_names]
    table += ["", "static const struct { const char *name; void *function; } imports[] =", "{"]
    table += [f'\t{{ "{name}", (void *){symbol(name)} }},' for name in host_names]
    table += ["};", "", "void *host_resolve_import(const char *name)", "{", "\tunsigned long index;", "",
              "\tfor (index = 0; index < sizeof(imports) / sizeof(imports[0]); index++)", "\t{",
              "\t\tif (!strcmp(imports[index].name, name))", "\t\t\treturn imports[index].function;", "\t}",
              "\treturn NULL;", "}"]
    with open(host_table, "w", encoding="utf-8") as f:
        f.write("\n".join(table) + "\n")


if __name__ == "__main__":
    main()
