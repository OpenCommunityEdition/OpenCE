"""The real menu_pc_art.c: a PC cache written for the test is read as the player's install, its frame decoded, and
offsets and sizes that leave the files, or a cache that is not the PC's, give nothing."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = ["decodes-a-frame", "other-pngs-have-none", "refuses-the-xbox-version", "refuses-pixels-outside-bitmaps-map",
         "refuses-short-pixels"]

NEGATIVE_CONTROLS = {
    "alpha-scale": (("& 15) * 17);\n\t\t\t\t\t\telse if (format == _format_dxt5)",
                    "& 15) * 16);\n\t\t\t\t\t\telse if (format == _format_dxt5)"),
                    "decodes-a-frame"),
    "takes-version-5": (("read_u32(map + 4) != VERSION_PC &&", "0 &&"), "refuses-the-xbox-version"),
    "no-size-check": (("if (located->bytes < need)", "if (0)"), "refuses-short-pixels"),
}


def generated(fault=None):
    source = read("port/linux/src/menu_pc_art.c")
    if fault:
        source = mutated(source, *fault)
    # the harness's include folder has the declarations the file takes from the port's
    return (("under_test.inc", source), ("halo_menus.h", read("port/linux/include/halo_menus.h")),
            ("port_config.h", "const char *config_string(const char *name);\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("menu_pc_art", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("menu_pc_art", generated(fault)), case)
    assert status == CHECK_FAILED, f"menu_pc_art.c with a fault ({control}) passed '{case}'"
