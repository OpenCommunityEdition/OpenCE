"""The real path and bounds checks of loose_sounds.c: no map's sound tag name opens a file outside the tags folder,
and no count or offset from a tag file reads past it."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["accepts-tag-names", "refuses-escapes", "refuses-long-names", "bounds"]

# a fault in loose_sounds.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "lets-parents-through": (("(*character == '.' && character[1] == '.' &&", "(0 &&"), "tag_file_path",
                             "refuses-escapes"),
    "lets-drives-through": (("if (*character == ':' ||", "if (0 ||"), "tag_file_path", "refuses-escapes"),
    "adds-offset-and-size": (("offset <= file_size && size <= file_size - offset", "offset + size <= file_size"),
                             "in_file", "bounds"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/loose_sounds.c")
    config = ""
    for name in ("TAG_FILE_FOLDER", "TAG_FILE_EXTENSION"):
        match = re.search(r"#define " + name + r" (\".*\")", source)
        assert match, f"{name} not found in loose_sounds.c"
        config += f"#define {name} {match.group(1)}\n"
    text = ""
    for name in ("in_file", "tag_file_path"):
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("loose_sound_paths", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("loose_sound_paths", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"loose_sounds.c with a fault ({control}) passed '{case}': the test cannot see it"
