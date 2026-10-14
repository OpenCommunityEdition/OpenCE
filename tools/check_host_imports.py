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

- a result is returned in a register the guest reads 32 bits of (64 for
  a 64-bit integer), so a pointer result is refused: the host's would
  lose its upper half. A host function that returns guest memory returns
  its address as a uint32_t, or a handle; POINTER_RESULTS names the
  exceptions and why each is safe;
- each host function's definition (in port/android/host and
  port/linux/host, as the host compiles it, LP64) takes the same classes
  of arguments as the guest's declaration, and returns the same class of
  result: a 32-bit integer where the guest passes one (the guest leaves
  the upper half of its register undefined), a 64-bit integer where it
  passes one, the same floating-point type; a guest pointer may arrive as
  a pointer, a uint64_t or a uint32_t (the guests zero-extend pointers);
  a pointer result as a uint32_t guest address.

posix.h's functions are imported too (hostposix_<name>,
tools/android_posix_stubs.py) and are checked the same way; both sides
compile them from posix.h itself.

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
# where the host defines the functions the import lists name
HOST_DEFINITION_FOLDERS = [
    Path("port/android/host"),
    Path("port/linux/host"),
]

# functions whose pointer result is safe for the guest, and why
POINTER_RESULTS = {
    "posix_directory_open": "a 64-bit host returns a handle from 1 to 64, not an address (posix_files.c)",
}

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
# a definition at file scope: the same, with a body (its result may be on
# a line of its own)
DEFINITION = re.compile(r"^(?!static\b|typedef\b|return\b|else\b|extern\b|do\b)([A-Za-z_][\w \t\*]*?\n?)\b(host_\w+)"
                        r"\s*\(([^;{}]*?)\)\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?\{", re.M | re.S)


class Prototype:
    def __init__(self, name: str, result: str, parameters: List[str], where: str):
        self.name = name
        self.result = result
        self.parameters = parameters
        self.where = where

    def classes(self) -> List[str]:
        return [classify(parameter)[0] for parameter in self.parameters]

    def result_class(self) -> str:
        return result_class(self.result)


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


def parse_prototypes(text: str, where: str, pattern: "re.Pattern[str]" = PROTOTYPE) -> List[Prototype]:
    """The file-scope function declarations of host_* and posix_* names in
    C source text (definitions, static functions and calls are not)."""
    prototypes = []
    code = strip_comments(text)
    for match in pattern.finditer(code):
        result, name, parameters = match.groups()
        line = code.count("\n", 0, match.start()) + 1
        prototypes.append(Prototype(name, " ".join(result.split()), split_parameters(parameters),
                                    f"{where}:{line}"))
    return prototypes


def parse_definitions(text: str, where: str) -> List[Prototype]:
    """The file-scope function definitions of host_* names in C source
    text (static functions are not)."""
    return parse_prototypes(text, where, DEFINITION)


TYPE_WORDS = {"unsigned", "long", "short", "int", "char", "float", "double", "void", "_Bool", "bool"}


def base_type(parameter: str) -> str:
    """The parameter's type without its qualifiers and name."""
    words = [word for word in re.findall(r"[A-Za-z_]\w*", parameter) if word not in QUALIFIERS]
    tagged = len(words) == 2 and words[0] in ("struct", "union", "enum")
    if len(words) >= 2 and words[-1] not in TYPE_WORDS and not tagged:
        words = words[:-1]
    return " ".join(words)


def classify(parameter: str, host: bool = False) -> Tuple[str, Optional[str]]:
    """("pointer" | "int32" | "int64" | "float" | "double" | "bad", the
    reason if bad), as the guest declares the parameter, or with host as the
    LP64 host defines it (where long and size_t are 64-bit)."""
    if parameter == "...":
        return "bad", "is variadic"
    if "*" in parameter or "[" in parameter or "(" in parameter:
        return "pointer", None
    kind = base_type(parameter)
    if kind in FLOATS:
        return kind, None
    if host and kind in DIFFERENT_SIZES:
        return "int64", None
    if kind in INTEGERS_64:
        return "int64", None
    if kind in INTEGERS_32 or kind.startswith("enum "):
        return "int32", None
    if kind in DIFFERENT_SIZES:
        return "bad", f"has a {kind}, whose size differs between the ILP32 guest and the 64-bit host"
    if kind.startswith("struct ") or kind.startswith("union "):
        return "bad", f"passes {kind} by value"
    return "bad", f"has the type {kind or parameter!r}, which tools/check_host_imports.py does not know"


def result_class(result: str, host: bool = False) -> str:
    """A result type's class (as classify's), or "void"."""
    result = result.replace("extern", "")
    if "*" not in result and base_type(result) == "void":
        return "void"
    return classify(result.strip(), host)[0]


# the host's classes a guest argument's class may arrive as
ARRIVES_AS = {
    "int32": {"int32"},
    "int64": {"int64"},
    "pointer": {"pointer", "int64", "int32"},
    "float": {"float"},
    "double": {"double"},
}


def check_definition(declaration: Prototype, definition: Prototype) -> List[str]:
    """What is wrong with how the guest's declaration meets the host's
    definition."""
    problems = []
    if len(definition.parameters) != len(declaration.parameters):
        return [f"takes {len(definition.parameters)} arguments ({', '.join(definition.parameters)}) where "
                f"{declaration.where} declares {len(declaration.parameters)} ({', '.join(declaration.parameters)})"]
    for index, (guest, host) in enumerate(zip(declaration.parameters, definition.parameters)):
        guest_class, host_class = classify(guest)[0], classify(host, host=True)[0]
        if guest_class == "bad":
            continue  # (reported with the declaration)
        if host_class not in ARRIVES_AS[guest_class]:
            reason = {
                ("int32", "int64"): ": the host reads the upper half the guest leaves undefined",
                ("int32", "pointer"): ": the host reads the upper half the guest leaves undefined",
                ("int64", "int32"): ": the host drops the upper half",
            }.get((guest_class, host_class), "")
            problems.append(f"argument {index + 1} is {host_class} in the host ({host}) but {guest_class} in "
                            f"the guest ({guest}, {declaration.where}){reason}")
    guest_result, host_result = declaration.result_class(), result_class(definition.result, host=True)
    if guest_result == "pointer" and host_result == "int32":
        pass  # (a guest address)
    elif guest_result != "void" and host_result != guest_result:
        problems.append(f"returns {host_result} in the host ({definition.result}) but {guest_result} in the "
                        f"guest ({declaration.result}, {declaration.where})")
    return problems


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
        if kind in FLOATS:
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


def pointer_result_problem(prototype: Prototype) -> Optional[str]:
    if prototype.result_class() != "pointer" or prototype.name in POINTER_RESULTS:
        return None
    return ("returns a pointer, of which the guest keeps the lower 32 bits: return guest memory's address as a "
            "uint32_t, or a handle (or name it in tools/check_host_imports.py's POINTER_RESULTS, with why it is "
            "safe)")


def check(lists: Dict[str, List[str]], listed_declarations: Dict[str, List[Prototype]],
          other_declarations: Iterable[Prototype], posix: Iterable[Prototype],
          host_definitions: Optional[Dict[str, List[Prototype]]] = None) -> List[str]:
    """The problems: lists maps each list's name to its names,
    listed_declarations each file that the lists must cover to its
    prototypes; other_declarations are the guest's other declarations of
    host functions, posix the prototypes of posix.h, host_definitions
    (if not None) the host's definitions by name."""
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
            if other.result_class() != first.result_class():
                problems.append(f"{other.where}: {name}'s result ({other.result}) differs from "
                                f"{first.where}'s ({first.result})")
        for problem in check_arguments(first):
            problems.append(f"{first.where}: {name}: {problem}")
        definitions = (host_definitions or {}).get(name, [])
        if host_definitions is not None and not definitions:
            problems.append(f"{first.where}: {name} has no definition in the host")
        for definition in definitions:
            for problem in check_definition(first, definition):
                problems.append(f"{definition.where}: {name}: {problem}")
        result_problem = pointer_result_problem(first)
        if result_problem and not (definitions and all(result_class(definition.result, host=True) == "int32"
                                                       for definition in definitions)):
            problems.append(f"{first.where}: {name}: {result_problem}")
    for prototype in posix:
        for problem in check_arguments(prototype):
            problems.append(f"{prototype.where}: {prototype.name} (imported as host{prototype.name}): {problem}")
        result_problem = pointer_result_problem(prototype)
        if result_problem:
            problems.append(f"{prototype.where}: {prototype.name} (imported as host{prototype.name}): "
                            f"{result_problem}")
    return problems


def host_sources(root: Path) -> List[Path]:
    return [path for folder in HOST_DEFINITION_FOLDERS for path in sorted((root / folder).glob("*.c"))]


def inputs() -> List[Path]:
    """The files a check reads, but for the other declarations' folders (the
    build's dependencies)."""
    return [*IMPORT_LISTS, *LISTED_DECLARATIONS, POSIX_HEADER,
            *(path.relative_to(ROOT) for path in host_sources(ROOT))]


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
    definitions: Dict[str, List[Prototype]] = {}
    for path in host_sources(root):
        for definition in parse_definitions(path.read_text(encoding="utf-8", errors="replace"),
                                            str(path.relative_to(root))):
            definitions.setdefault(definition.name, []).append(definition)
    return check(lists, listed_declarations, other, posix, definitions)


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
