"""The real files_gather, file_find and file_add (port/linux/src/menu_files.c): a file of the menus folder with an
embedded file's name replaces it and is not added again, also where the folder's listing spells its subfolder with
a backslash (Windows: ce\\main_menu.xml), which made every widget of a replaced file twice named, and the game fall
back to the Xbox's menus."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["slashes", "backslashes"]
NEGATIVE_CONTROLS = {
    # the listing's names taken as they are
    "no-normalising": (("\t\t\tfor (separator = names[name]; (separator = strchr(separator, '\\\\')) != NULL; separator++)\n"
                        "\t\t\t\t*separator = '/';\n", ""), "backslashes"),
}


def generated(fault=None):
    source = read("port/linux/src/menu_files.c").replace("\r\n", "\n")
    code = "\n".join([
        "struct menu_file { char *path; const unsigned char *data; unsigned long size; int loaded; };",
        "static struct menu_file *files;",
        "static long file_count;",
        "static char folder[1024];",
        function(source, "file_find"),
        function(source, "file_add"),
        function(source, "files_gather"),
    ]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("menu_files_gather", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("menu_files_gather", generated(fault)), case)
    assert status == CHECK_FAILED, output
