"""halo_fmod (port/include/halo_math.h) gives the C library's fmod to the bit, its fast fraction of a whole too."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run

CASES = ['fraction', 'other']
CONTROLS = [
    ('fraction != 0.0 ? fraction : x * 0.0', 'fraction', 'fraction'),
    ('x < 2147483648.0', 'x < 4294967296.0', 'fraction'),
    ('y == 1.0 && ', '', 'other'),
]


def generated(fault=None):
    code = function(read('port/include/halo_math.h'), 'halo_fmod')
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('fmod_fraction', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('fmod_fraction', generated((before, after))), case)
    assert status == CHECK_FAILED, output
