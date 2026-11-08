"""The NV2A vertex programs' rcp and rsq as the port writes them in GLSL (nv2a_vsh.c's nv2a_rcp and nv2a_rsq): zero
times either is zero, as on the NV2A, not the GLSL infinity's not a number, which made every triangle of a bumped
lightmap's vertex of no incident radiosity not a number (a10's floors drawn in wedges, differently on each GPU)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = ["zero", "incident radiosity", "values"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # rsq as it was (before fix-vertex-reciprocal-zero)
    "rsq-infinite": (("s > 0.0 ? min(inversesqrt(s), 1.884467e+19) : 1.884467e+19", "inversesqrt(s)"), "incident radiosity"),
    # rcp as it was
    "rcp-infinite": (("x != 0.0 ? clamp(1.0 / x, -1.884467e+19, 1.884467e+19) : 1.884467e+19", "1.0 / x"), "zero"),
    # no limit to how large rcp is
    "rcp-unclamped": (("clamp(1.0 / x, -1.884467e+19, 1.884467e+19)", "(1.0 / x)"), "values"),
}


def helpers(source):
    """The GLSL of nv2a_rcp and nv2a_rsq: the string literals from the first's line to the second's end."""
    lines = source.splitlines()
    start = next(i for i, line in enumerate(lines) if '"vec4 nv2a_rcp(float x)\\n"' in line)
    stop = next(i for i, line in enumerate(lines) if '"vec4 nv2a_rsq(float x)\\n"' in line)
    stop = next(i for i in range(stop, len(lines)) if lines[i].strip() == '"}\\n"')
    text = []
    for line in lines[start:stop + 1]:
        match = re.match(r'\s*"(.*)"\s*$', line)
        if match:
            text.append(match.group(1).replace("\\n", "\n").replace("\\t", "\t"))
    return "".join(text)


def generated(fault=None):
    code = helpers(read("port/linux/src/nv2a_vsh.c"))
    if fault:
        code = mutated(code, *fault)
    return (("helpers.inc", code),)


def test_programs_take_them():
    """A program's rcp and rsq are written as the helpers, and rcc as before."""
    source = read("port/linux/src/nv2a_vsh.c")
    assert 'case _ilu_rcp: xgpu_text_append(&text, "\\tilu = nv2a_rcp(C.x);\\n"); break;' in source
    assert 'case _ilu_rsq: xgpu_text_append(&text, "\\tilu = nv2a_rsq(C.x);\\n"); break;' in source
    assert 'case _ilu_rcc: xgpu_text_append(&text, "\\tilu = nv2a_rcc(C.x);\\n"); break;' in source


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("vertex_reciprocal", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("vertex_reciprocal", generated(fault)), case)
    assert status == CHECK_FAILED, f"the helpers with a fault ({control}) passed '{case}': the test cannot see it"
