"""The real names of map sharing (map_share_files.c): a second version of a map is known by its name's end, the menus
leave the hash out and number the versions (custom_edition_maps.c), and a map's version is its digits' number."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["hashed-names", "safe-names", "base-length", "digits", "pieces", "display-names", "versions-numbered",
         "versions-without-the-first", "versions"]

# a fault in map_share_files.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "takes-uppercase": (("(character >= 'a' && character <= 'f')", "(character >= 'A' && character <= 'f')"),
                        "hex_digit", "hashed-names"),
    "lets-slashes-through": (("strchr(\"/\\\\:", "strchr(\"\\\\:"), "map_share_name_safe", "safe-names"),
    "lets-dots-through": (("name[0] == '.' || ", ""), "map_share_name_safe", "safe-names"),
    "lets-devices-through": ((" ||\n\t\tname_device(name)", ""), "map_share_name_safe", "safe-names"),
    "com-ports-not-devices": (("(!_strnicmp(name, \"com\", 3) || ", "("), "name_device", "safe-names"),
    "spaces-make-a-name": (("while (length && name[length - 1] == ' ')", "while (0)"), "name_device", "safe-names"),
    "every-con-a-device": (("(long)csstrlen(devices[index]) == length && ", ""), "name_device", "safe-names"),
    "no-underscore": (("|| name[length - MAP_SHARE_NAME_DIGITS - 1] != '_'", ""), "map_share_name_hashed",
                      "hashed-names"),
    "keeps-the-underscore": (("length - MAP_SHARE_NAME_DIGITS - 1)", "length - MAP_SHARE_NAME_DIGITS)"),
                             "map_share_name_base_length", "base-length"),
    "nibbles-swapped": (("hex[hash[index] >> 4]", "hex[hash[index] & 15]"), "map_share_hash_digits", "digits"),
    "pieces-rounded-down": (("(size + MAP_SHARE_PIECE_BYTES - 1)", "(size)"), "map_share_piece_count", "pieces"),
    "shows-the-hash": (("map_share_name_base_length(name)", "(short)csstrlen(name)"), "display_name_make",
                       "display-names"),
    "stock-by-whole-name": (("map_share_name_base_length(name)", "(short)csstrlen(name)"), "stock_map_index",
                            "display-names"),
    "the-first-numbered-too": (("short number = 1;", "short number = 2;"), "custom_edition_maps_tell_versions_apart",
                               "versions-numbered"),
    "longer-name-a-version": (("map_share_name_base_length(version->name) != length ||", "0 ||"),
                              "custom_edition_maps_tell_versions_apart", "versions-numbered"),
    "versions-not-numbered": (("if (number > 1)", "if (0)"), "custom_edition_maps_tell_versions_apart",
                              "versions-without-the-first"),
    "version-bytes-swapped": (("(unsigned long)hash[0] << 24 | (unsigned long)hash[1] << 16",
                               "(unsigned long)hash[1] << 24 | (unsigned long)hash[0] << 16"),
                              "map_share_hash_version", "versions"),
    "version-zero": (("return version ? version : 1;", "return version;"), "map_share_hash_version", "versions"),
}


def generated(fault=None, faulty_function=None):
    header = read("port/linux/game/map_share.h")
    source = read("port/linux/game/map_share_files.c")
    config = ""
    for name in ("MAP_SHARE_NAME_DIGITS", "MAP_SHARE_PIECE_BYTES", "MAP_SHARE_HASH_BYTES"):
        config += f"#define {name} {constant(header, name)}\n"
    text = ""
    for name in ("hex_digit", "map_share_name_hashed", "name_device", "map_share_name_safe",
                 "map_share_name_base_length", "map_share_hash_digits", "map_share_piece_count",
                 "map_share_hash_version"):
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    # the menus' names (custom_edition_maps.c): Halo PC's own maps, and the rest
    maps = read("port/linux/game/custom_edition_maps.c")
    start = maps.index("static struct\n{\n\tchar const *file;")
    text += maps[start:maps.index("};", start) + 2] + "\n"
    for name in ("stock_map_index", "display_name_make", "custom_edition_maps_tell_versions_apart"):
        code = function(maps, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("map_share_names", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("map_share_names", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"map_share_files.c with a fault ({control}) passed '{case}': the test cannot see it"
