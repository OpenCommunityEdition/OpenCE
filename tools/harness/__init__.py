"""Asset-free regression tests: the engine's real code compiled against a fake world (README.md)."""

import functools
import os
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
HARNESS = Path(__file__).resolve().parent


def read(relative):
    """A source file of the repository, as the compiler reads it."""
    return (ROOT / relative).read_text(encoding="latin-1")


def function(source, name):
    """A function's definition (static, inline or not), taken by matching braces; LookupError if not found."""
    match = re.search(r"^(?:static |__inline )?[\w *]+?\b" + re.escape(name) + r"\(\s*[^;{]*\)\s*\{", source, re.M)
    if not match:
        raise LookupError(f"function not found in the sources: {name}")
    end = source.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def biped_riding_sources():
    """The native movement with its port hooks expanded for existing slice tests.

    No code is copied: undo only the tick-context and physics-pointer spelling,
    so the same cases and mutations still compile the actual hook statements.
    Assert the calls before expanding them; a disconnected hook must fail too.
    """
    native = read("source/units/bipeds.c")
    header = read("port/linux/game/vehicle_riding_bipeds.h")
    port = read("port/linux/game/vehicle_riding_bipeds.c")
    fields = structure(header, "biped_riding_tick").split("{\n", 1)[1].rsplit("};", 1)[0]
    bodies = {}
    for name in ("prepare", "finish"):
        definition = function(port, "biped_riding_" + name)
        body = definition.split("\n{\n", 1)[1].rsplit("\n}", 1)[0]
        body = re.sub(r"\btick->(\w+)", r"\1", body).replace("physics->", "physics.")
        body = body.replace("biped_riding_turn(biped, physics,", "biped_riding_turn(biped, &physics,")
        if name == "prepare":
            initializers, body = body.split("\n\n", 1)
            for field, value in re.findall(r"\t(\w+) = ([^;]+);", initializers):
                fields = re.sub(r"\b" + field + ";", field + " = " + value + ";", fields)
        # (a body that returns early is one pass of a do/while, so its break leaves it and nothing more: a loop or a
        # switch of its own would take the break)
        assert not re.search(r"\b(for|while|switch|do)\b", re.sub(r"/\*.*?\*/", "", body, flags=re.S)), name
        body = "\tdo\n\t{\n" + re.sub(r"\breturn;", "break;", body).rstrip("\n") + "\n\t} while (0);\n"
        bodies[name] = body
        call = "\tbiped_riding_" + name + "(biped_index, biped, definition, &physics, &riding_tick);\n"
        assert native.count(call) == 1, call
        native = native.replace(call, body)
    declaration = "\tstruct biped_riding_tick riding_tick;\n"
    assert native.count(declaration) == 1
    native = native.replace(declaration, fields)
    return header + "\n" + native + "\n" + port


def inline(source, name):
    """An __inline header function, as a static one."""
    return function(source, name).replace("__inline", "static", 1)


def enum_with(source, member):
    """The whole enum that declares a member."""
    at = source.index(member)
    start = source.rindex("enum", 0, at)
    return source[start:source.index("};", at) + 2]


def structure(source, name):
    """An actual named structure's definition, including nested unions/structures."""
    match = re.search(r"^struct\s+" + re.escape(name) + r"\s*\{", source, re.M)
    if not match:
        raise LookupError(f"structure not found in the sources: {name}")
    end = source.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():source.index(";", end) + 1]


def constant(source, name):
    """An enum constant's or #define's integer value, following a name it is set to (as the port's capacities are)."""
    match = re.search(r"\b" + re.escape(name) + r"\s*=\s*(\w+)", source) or \
        re.search(r"#define\s+" + re.escape(name) + r"\s+(\w+)", source)
    if not match:
        raise LookupError(f"constant not found in the sources: {name}")
    value = match.group(1)
    if re.fullmatch(r"0x[0-9A-Fa-f]+|\d+", value):
        return int(value, 0)
    return constant(source + read("port/linux/include/halo_port_capacity.h"), value)


def mutated(text, before, after):
    """The code with one deliberate fault (a negative control); LookupError once the code no longer has it."""
    if text.count(before) != 1:
        raise LookupError(f"negative control no longer applies (found {text.count(before)} times): {before!r}")
    return text.replace(before, after, 1)


_directory = tempfile.TemporaryDirectory(prefix="halo-harness-")


CHECK_FAILED = 1  # a case's exit status when a CHECK fails (harness.h)


@functools.lru_cache(maxsize=None)
def build(test, generated):
    """tests/<test>.c compiled with the generated includes ((name, text), ...) for the game's 32-bit ABI."""
    work = Path(_directory.name) / f"{test}-{abs(hash(generated)):x}"
    work.mkdir(parents=True, exist_ok=True)
    for name, text in generated:
        (work / name).write_text(text)
    executable = work / (test + (".exe" if os.name == "nt" else ""))
    command = [os.environ.get("CC", "clang"), "-m32", "-std=gnu99", "-O2", "-Wall", "-Werror", "-Wno-unused-function",
               "-Wno-unused-variable", "-I", str(HARNESS / "include"), "-I", str(work),
               str(HARNESS / "tests" / f"{test}.c"), "-o", str(executable)]
    # MSVC's runtime includes maths; there is no separate m.lib on Windows.
    if os.name == "nt":
        command.append("-D_CRT_SECURE_NO_WARNINGS=")
    else:
        command.append("-lm")
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"{test}.c does not compile:\n{result.stderr}")
    return executable


def run(executable, case):
    """Run one case: (exit status, output)."""
    result = subprocess.run([str(executable), case], capture_output=True, text=True, errors="replace", timeout=60)
    return result.returncode, (result.stdout + result.stderr).strip()
