"""Signed animation time samples the same periodic table in either direction."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CASES = ['negative', 'slide-wrap', 'integer', 'positive', 'range', 'uninitialized']
ADJUST = '\t\tif (fraction < 0.0f)\n\t\t{\n\t\t\tfraction += 1.0f;\n\t\t\tindex--;\n\t\t}'


def generated(fault=None):
    source = read('source/math/periodic_functions.c')
    enums = '\n'.join(enum_with(source, name) for name in (
        '_periodic_function_one', 'PERIODIC_FUNCTION_TABLE_SIZE', 'SLIDE_PERIODIC_FUNCTION_FLAGS'))
    code = function(source, 'periodic_function_evaluate')
    original = mutated(code, ADJUST, '').replace('periodic_function_evaluate(', 'original_evaluate(', 1)
    if fault:
        code = mutated(code, *fault)
    return (('enums.inc', enums), ('under_test.inc', code + '\n' + original))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('periodic_negative_time', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    ((ADJUST, ''), 'negative'),
    (('index--;', ';'), 'negative'),
    (('fraction += 1.0f;', ';'), 'slide-wrap'),
])
def test_negative_control(fault, case):
    status, output = run(build('periodic_negative_time', generated(fault)), case)
    assert status == CHECK_FAILED, output
