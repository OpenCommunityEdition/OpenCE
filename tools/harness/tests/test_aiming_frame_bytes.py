"""The little-endian view of an aiming animation's frame that aiming_screen_apply's walk needs on a big-endian host:
the helpers are taken from model_animations.c (their big-endian branch forced on) and read a frame converted in the width
of each field as the x86 reads the little-endian one. The negative control reads the converted bytes as they are."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, structure, run  # noqa: E402

CASES = ["rotations", "translations"]


def generated(fault=None):
    source = read("source/models/model_animations.c")
    definitions = read("source/models/model_animation_definitions.h")
    start = source.index("#if defined(__BIG_ENDIAN__) || defined(__ppc__) || defined(__POWERPC__)\nstatic unsigned char aiming_data_little_endian_byte")
    end = source.index("#else\n#define aiming_decompress_8byte", start)
    helpers = "#if 1" + source[start + len("#if defined(__BIG_ENDIAN__) || defined(__ppc__) || defined(__POWERPC__)"):end] + "#endif\n"
    if fault:
        helpers = mutated(helpers, *fault)
    types = """
#define TAG_STRING_LENGTH 31
typedef struct { real_vector3d v; real w; } real_quaternion;
struct tag_data { long size; long flags; long file_offset; void *address; long definition; };
struct tag_reference { long group; long name; long name_length; long index; };
enum { COMPRESSED_QUATERNION_COMPONENT_MAXIMUM = 32767 };
""" + structure(source, "compressed_quaternion_8byte") + ";\n" + structure(definitions, "animation") + ";\n" + \
        function(source, "quaternion_decompress_8byte") + "\n"
    return (("types.inc", types), ("under_test.inc", helpers))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("aiming_frame_bytes", generated()), case)
    assert status == 0, output


def test_negative_control_raw_bytes():
    fault = ("return base[frame_start + cursor + unit_start + (unit - 1 - (relative - unit_start))];",
             "return base[frame_start + cursor + unit_start + (relative - unit_start)];")
    status, output = run(build("aiming_frame_bytes", generated(fault)), "rotations")
    assert status == CHECK_FAILED, f"reading the converted bytes as they are passed: {output}"
