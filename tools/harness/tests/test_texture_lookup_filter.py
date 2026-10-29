"""The menus' bitmaps and the high-res HUD's placeholders, asked after for each texture bound, are found through their
filters exactly when registered.

    python tools/harness/tests/test_texture_lookup_filter.py     # the cases, then how fast with and without filters
"""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["found", "replaced", "forgotten"]

# a fault in a filter, and the case that must catch it
NEGATIVE_CONTROLS = {
    "art-not-filtered-in": (("menu", "\tart_filter[art_filter_bit(data) >> 3] |=", "\t(void)"), "found"),
    "placeholders-not-filtered-again": (("hud", "\t}\n\tplaceholder_filter_build();\n}", "\t}\n}"), "replaced"),
    "filter-bit-mismatch": (("menu", "\tunsigned long bit = art_filter_bit(data);", "\tunsigned long bit = art_filter_bit(data) ^ 1;"),
                            "found"),
}

# the filters' checks taken out: the lookups as they were (for the benchmark)
UNFILTERED = [("menu", "\tif (!(art_filter[bit >> 3] & 1 << (bit & 7)))\n\t\treturn 0;\n", ""),
              ("hud", "\tif (!(placeholder_filter[bit >> 3] & 1 << (bit & 7)))\n\t\treturn 0;\n", "")]


def first_function(source, *names):
    """The first of the names the source defines (the render thread's builds run the registrations on it)."""
    for name in names:
        try:
            return name, function(source, name)
        except LookupError:
            pass
    raise LookupError(f"none of these functions is in the sources: {names}")


def under_test(faults=()):
    menu = read("port/linux/src/menu_files.c").replace("\r\n", "\n")
    hud = read("port/linux/src/hud_hires.c").replace("\r\n", "\n")
    for where, before, after in faults:
        if where == "menu":
            menu = mutated(menu, before, after)
        else:
            hud = mutated(hud, before, after)
    text = re.search(r"static struct\n\{\n\tunsigned long data;\n\tchar \*png;.*?\nstatic long art_count;\n", menu, re.S).group(0)
    text += menu[menu.index("static unsigned char art_filter["):menu.index("}\n", menu.index("art_filter_bit(")) + 2]
    register, code = first_function(menu, "run_art_register", "halo_menus_art_register")
    text += code + f"\n#define art_register {register}\n"
    forget, code = first_function(menu, "run_art_forget", "halo_menus_art_forget")
    text += code + f"\n#define art_forget {forget}\n"
    text += function(menu, "menu_art_texture") + "\n"
    text += hud[hud.index("#define MAXIMUM_PLACEHOLDERS"):hud.index("/* the sequences")]
    register, code = first_function(hud, "run_register_placeholder", "hud_hires_register_placeholder")
    text += code + "\n"
    if register == "run_register_placeholder":
        text += "#define placeholder_register(asset, texture) run_register_placeholder(asset, (texture) != NULL, (texture) ? (texture)[1] : 0)\n"
    else:
        text += "#define placeholder_register hud_hires_register_placeholder\n"
    text += function(hud, "hud_hires_placeholder_texture") + "\n"
    return (("under_test.inc", text),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("texture_lookup_filter", under_test()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("texture_lookup_filter", under_test((fault,))), case)
    assert status == CHECK_FAILED, f"the filters with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    status = pytest.main(["-q", __file__])
    print("without filters:", run(build("texture_lookup_filter", under_test(tuple(UNFILTERED))), "benchmark")[1])
    print("with filters:   ", run(build("texture_lookup_filter", under_test()), "benchmark")[1])
    sys.exit(status)
