"""effect_real_to_byte (effects.c) gives the byte the x86 gives for a real outside a long, whatever the host's own
conversion does; the negative control is the saturating conversion of PowerPC and ARM (host-independent: a function)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["counts", "split-screen", "not-a-number"]

SATURATE = """
static long saturating_long(real value)
{
    if (value != value)
        return 0;
    if (value >= 2147483648.0f)
        return 0x7FFFFFFF;
    if (value < -2147483648.0f)
        return (long)0x80000000UL;
    return (long)value;
}
"""


def generated(saturating=False):
    code = function(read("source/effects/effects.c"), "effect_real_to_byte")
    if saturating:
        # a PowerPC's conversion: no guard, and an infinity is the largest long
        code = mutated(code, "if (!(value >= -2147483648.0f && value < 2147483648.0f))\n\t\treturn 0;\n\n\treturn (byte)(long)value;",
                       "return (byte)saturating_long(value);")
        code = SATURATE + code
    return (("under_test.inc", code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("effect_particle_count", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("case", ["split-screen", "not-a-number"])
def test_negative_control_saturating_conversion(case):
    status, output = run(build("effect_particle_count", generated(saturating=True)), case)
    assert status == CHECK_FAILED, f"the saturating conversion passed '{case}': {output}"
