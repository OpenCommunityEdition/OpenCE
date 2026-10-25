"""The real checks between a mod's files and the game: the keys a map's tag names find files by (port_mods.c), a tag
file's bounds (mods.c), and the format comparison that keeps a mod's string from reading arguments the game does not
pass (mod_strings.c)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure  # noqa: E402

CASES = ["keys", "bounds", "formats", "long-formats"]

SOURCES = {
    "port/linux/src/port_mods.c": ["lower", "key_hash", "key_make", "names_equal"],
    "port/linux/game/mods.c": ["mods_in_file"],
    "port/linux/game/mod_strings.c": ["character_get", "character_in", "format_conversions", "formats_match"],
}

# a fault in the code, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "keeps-slashes": (
        ("key[length++] = *text == '/' ? '\\\\' : lower(*text);", "key[length++] = lower(*text);"),
        "key_make",
        "keys",
    ),
    "adds-offset-and-size": (
        ("offset <= file_size && size <= file_size - offset", "offset + size <= file_size"),
        "mods_in_file",
        "bounds",
    ),
    "ignores-stars": (("conversions[index].stars != map_conversions[index].stars ||", ""), "formats_match", "formats"),
    "ignores-modifiers": (
        ("strcmp(conversions[index].modifier, map_conversions[index].modifier)", "0"),
        "formats_match",
        "formats",
    ),
    "counts-percent-signs": (
        ("if (character_get(string, size, index, unicode) == '%')\n\t\t{", "if (0)\n\t\t{"),
        "format_conversions",
        "formats",
    ),
    "reads-low-bytes": (
        ("return character && character < 0x80 &&", "return character &&"),
        "character_in",
        "long-formats",
    ),
}


def generated(fault=None, faulty_function=None):
    strings = read("port/linux/game/mod_strings.c")
    config = structure(strings, "conversion") + "\n"
    config += f"#define MAXIMUM_CONVERSIONS {constant(strings, 'MAXIMUM_CONVERSIONS')}\n"
    text = ""
    for path, names in SOURCES.items():
        source = read(path)
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("mod_files", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("mod_files", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
