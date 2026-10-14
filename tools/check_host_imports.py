#!/usr/bin/env python3
"""Check the host imports of the ILP32 guest builds (Android, Linux).

The guest calls the host through import stubs (tools/android_imports.py)
that jump to the host's function with the arguments as the guest left them.
That works only where the guest's ABI and the host's put each argument in
the same place, which this checks for every import, from the declarations
the guest compiles:

- every name in an import list is declared, and every host function that
  guest_host.h, guest_desktop.c and xgpu.h declare is in a list (so the
  lists and the declarations cannot drift apart), and the declarations of a
  name agree with each other;
- no argument is variadic, a structure by value, or a type whose size
  differs between the ILP32 guest and the 64-bit host (long, size_t);
- at most 8 floating-point arguments (the vector registers of both
  hosts);
- integer and pointer arguments: x86-64 passes six in registers, AArch64
  eight. A seventh or later goes on the stack, where each of the x32 guest
  and the host gives it 8 bytes, but where the guest writes 4 bytes of a
  pointer that the host reads 8 of: so an argument after the sixth must be
  a 32-bit or 64-bit integer, never a pointer (pass a pointer to a structure
  of 32-bit members instead). The arm64_32 guest packs its stack arguments
  where AArch64 Linux gives each 8 bytes, which agree only for the first:
  so there are at most 9.

posix.h's functions are imported too (hostposix_<name>,
tools/android_posix_stubs.py) and are checked the same way.

Usage: check_host_imports.py [--root DIR] [--stamp FILE]
Exits 1 and names each problem when there is one; with --stamp, touches
FILE when there is none (the build's edge).
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

ROOT = Path(__file__).resolve().parent.parent

# the import lists, and the files whose declarations each must cover
# (every host function those files declare must be in one of the lists)
IMPORT_LISTS = [
    Path("port/android/host_imports.list"),
    Path("port/android/android_imports.list"),
    Path("port/linux/host/host_imports.list"),
]
LISTED_DECLARATIONS = [
    Path("port/android/guest/runtime/guest_host.h"),
    Path("port/linux/host/guest_desktop.c"),
    Path("port/linux/src/xgpu.h"),
]
# where else the guest declares host functions (their declarations must agree)
DECLARATION_FOLDERS = [
    Path("port/android/guest/runtime"),
    Path("port/linux/src"),
    Path("port/linux/game"),
]
POSIX_HEADER = Path("port/linux/src/posix.h")

REGISTERS_X86_64 = 6
STACK_ARGUMENTS_ARM64_32 = 1
REGISTERS_ARM64 = 8
FLOAT_REGISTERS = 8

QUALIFIERS = {"const", "volatile", "restrict", "__restrict", "register", "signed"}
FLOATS = {"float", "double"}
INTEGERS_64 = {"long long", "unsigned long long", "long long int", "unsigned long long int", "int64_t", "uint64_t",
               "Sint64", "Uint64"}
INTEGERS_32 = {"int", "unsigned int", "unsigned", "short", "unsigned short", "short int", "unsigned short int",
               "char", "unsigned char", "int32_t", "uint32_t", "int16_t", "uint16_t", "int8_t", "uint8_t", "Sint32",
               "Uint32", "Sint16", "Uint16", "Uint8", "Sint8", "bool", "_Bool", "posix_long", "posix_ulong"}
# the same name, a different size in the ILP32 guest and the LP64 host
DIFFERENT_SIZES = {"long", "unsigned long", "long int", "unsigned long int", "size_t", "ssize_t", "intptr_t",
                   "uintptr_t", "ptrdiff_t", "off_t", "time_t"}

PROTOTYPE = re.compile(r"^(?!static\b|typedef\b|return\b|extern\s+\"C\")([A-Za-z_][\w \t\*]*?)\b((?:host|posix)_\w+)"
                       r"\s*\(([^;{}]*?)\)\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?;", re.M | re.S)


class Prototype:
    def __init__(self, name: str, result: str, parameters: List[str], where: str):
        self.name = name
        self.result = result
        self.parameters = parameters
        self.where = where

    def classes(self) -> List[str]:
        return [classify(parameter)[0] for parameter in self.parameters]


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def split_parameters(text: str) -> List[str]:
    parameters, depth, current = [], 0, ""
    for c in text:
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        if c == "," and depth == 0:
            parameters.append(current)
            current = ""
        else:
            current += c
    parameters.append(current)
    parameters = [" ".join(p.split()) for p in parameters]
    if parameters == ["void"] or parameters == [""]:
        return []
    return parameters


def parse_prototypes(text: str, where: str) -> List[Prototype]:
    """The file-scope function declarations of host_* and posix_* names in
    C source text (definitions, static functions and calls are not)."""
    prototypes = []
    code = strip_comments(text)
    for match in PROTOTYPE.finditer(code):
        result, name, parameters = match.groups()
        line = code.count("\n", 0, match.start()) + 1
        prototypes.append(Prototype(name, " ".join(result.split()), split_parameters(parameters),
                                    f"{where}:{line}"))
    return prototypes


TYPE_WORDS = {"unsigned", "long", "short", "int", "char", "float", "double", "void", "_Bool", "bool"}


def base_type(parameter: str) -> str:
    """The parameter's type without its qualifiers and name."""
    words = [word for word in re.findall(r"[A-Za-z_]\w*", parameter) if word not in QUALIFIERS]
    tagged = len(words) == 2 and words[0] in ("struct", "union", "enum")
    if len(words) >= 2 and words[-1] not in TYPE_WORDS and not tagged:
        words = words[:-1]
    return " ".join(words)


def classify(parameter: str) -> Tuple[str, Optional[str]]:
    """("pointer" | "int32" | "int64" | "float" | "bad", the reason if bad)."""
    if parameter == "...":
        return "bad", "is variadic"
    if "*" in parameter or "[" in parameter or "(" in parameter:
        return "pointer", None
    kind = base_type(parameter)
    if kind in FLOATS:
        return "float", None
    if kind in INTEGERS_64:
        return "int64", None
    if kind in INTEGERS_32 or kind.startswith("enum "):
        return "int32", None
    if kind in DIFFERENT_SIZES:
        return "bad", f"has a {kind}, whose size differs between the ILP32 guest and the 64-bit host"
    if kind.startswith("struct ") or kind.startswith("union "):
        return "bad", f"passes {kind} by value"
    return "bad", f"has the type {kind or parameter!r}, which tools/check_host_imports.py does not know"


def check_arguments(prototype: Prototype) -> List[str]:
    """What is wrong with how the prototype's arguments would cross."""
    problems = []
    integers = 0
    floats = 0
    for index, parameter in enumerate(prototype.parameters):
        kind, reason = classify(parameter)
        if kind == "bad":
            problems.append(f"argument {index + 1} ({parameter}) {reason}")
            continue
        if kind == "float":
            floats += 1
            continue
        integers += 1
        if integers > REGISTERS_X86_64 and kind == "pointer":
            problems.append(f"argument {index + 1} ({parameter}) is a pointer after the {REGISTERS_X86_64}th "
                            "integer or pointer argument: on x86-64 it is passed on the stack, where the x32 "
                            "guest writes 4 bytes and the host reads 8; pass a pointer to a structure instead")
    if integers > REGISTERS_ARM64 + STACK_ARGUMENTS_ARM64_32:
        problems.append(f"{integers} integer and pointer arguments: at most "
                        f"{REGISTERS_ARM64 + STACK_ARGUMENTS_ARM64_32} cross between the arm64_32 guest and the host")
    if floats > FLOAT_REGISTERS:
        problems.append(f"{floats} floating-point arguments: at most {FLOAT_REGISTERS}")
    result = base_type(prototype.result.replace("extern", ""))
    if "*" not in prototype.result and result in DIFFERENT_SIZES:
        problems.append(f"returns a {result}, whose size differs between the ILP32 guest and the 64-bit host")
    if "*" not in prototype.result and (result.startswith("struct ") or result.startswith("union ")):
        problems.append(f"returns {result} by value")
    return problems


def read_list(path: Path) -> List[str]:
    names = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            names.append(line)
    return names


def check(lists: Dict[str, List[str]], listed_declarations: Dict[str, List[Prototype]],
          other_declarations: Iterable[Prototype], posix: Iterable[Prototype]) -> List[str]:
    """The problems: lists maps each list's name to its names,
    listed_declarations each file that the lists must cover to its
    prototypes; other_declarations are the guest's other declarations of
    host functions, posix the prototypes of posix.h."""
    problems = []
    declared: Dict[str, List[Prototype]] = {}
    for prototypes in listed_declarations.values():
        for prototype in prototypes:
            declared.setdefault(prototype.name, []).append(prototype)
    listed = set()
    for list_name, names in lists.items():
        seen = set()
        for name in names:
            if name in seen:
                problems.append(f"{list_name}: {name} is listed twice")
            seen.add(name)
            if name in listed:
                problems.append(f"{list_name}: {name} is also in another list")
            listed.add(name)
            if name not in declared:
                problems.append(f"{list_name}: {name} has no declaration in "
                                + ", ".join(sorted(listed_declarations)))
    for where, prototypes in listed_declarations.items():
        for prototype in prototypes:
            if prototype.name.startswith("host_") and prototype.name not in listed:
                problems.append(f"{prototype.where}: {prototype.name} is declared but in no import list "
                                f"({', '.join(sorted(lists))})")
    for prototype in other_declarations:
        if prototype.name in declared:
            declared[prototype.name].append(prototype)
    for name, prototypes in sorted(declared.items()):
        if name not in listed:
            continue
        first = prototypes[0]
        for other in prototypes[1:]:
            if other.classes() != first.classes():
                problems.append(f"{other.where}: {name}'s arguments ({', '.join(other.parameters)}) differ from "
                                f"{first.where}'s ({', '.join(first.parameters)})")
        for problem in check_arguments(first):
            problems.append(f"{first.where}: {name}: {problem}")
    for prototype in posix:
        for problem in check_arguments(prototype):
            problems.append(f"{prototype.where}: {prototype.name} (imported as host{prototype.name}): {problem}")
    return problems


def inputs() -> List[Path]:
    """The files a check reads, but for the other declarations' folders (the
    build's dependencies)."""
    return [*IMPORT_LISTS, *LISTED_DECLARATIONS, POSIX_HEADER]


def check_repository(root: Path) -> List[str]:
    lists = {str(path): read_list(root / path) for path in IMPORT_LISTS}
    listed_declarations = {
        str(path): parse_prototypes((root / path).read_text(encoding="utf-8"), str(path))
        for path in LISTED_DECLARATIONS
    }
    other = []
    for folder in DECLARATION_FOLDERS:
        for path in sorted((root / folder).glob("*.[ch]")):
            relative = path.relative_to(root)
            if relative in LISTED_DECLARATIONS or relative == POSIX_HEADER:
                continue
            other += parse_prototypes(path.read_text(encoding="utf-8", errors="replace"), str(relative))
    posix = [prototype for prototype in
             parse_prototypes((root / POSIX_HEADER).read_text(encoding="utf-8"), str(POSIX_HEADER))
             if prototype.name.startswith("posix_")]
    return check(lists, listed_declarations, other, posix)


def main(arguments: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--stamp", type=Path)
    options = parser.parse_args(arguments)
    problems = check_repository(options.root)
    for problem in problems:
        print(f"check_host_imports: {problem}", file=sys.stderr)
    if problems:
        return 1
    if options.stamp:
        options.stamp.parent.mkdir(parents=True, exist_ok=True)
        options.stamp.write_text("ok\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
