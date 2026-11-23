"""The queued solid parts' order (rasterizer_xbox_models.c): each kind of part together, kinds in the order first
queued, a kind's parts in the order queued, every part once."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure

CASES = ['kinds', 'full', 'flushes']
CONTROLS = [
    # parts of another permutation taken for the same kind
    (' &&\n\t\t\t\tkind->shader_permutation_index == draw->shader_permutation_index)', ')', 'kinds'),
    # a kind's parts placed last first
    ('for (draw_index = 0; draw_index < model_batch_draw_count; draw_index++)\n\t\tmodel_batch_order',
     'for (draw_index = model_batch_draw_count - 1; draw_index >= 0; draw_index--)\n\t\tmodel_batch_order', 'kinds'),
    # the last flush's kinds taken for this one's
    ('model_batch_kind_stamp++;', 'model_batch_kind_stamp = 1;', 'flushes'),
]


def generated(fault=None):
    source = read('source/rasterizer/xbox/rasterizer_xbox_models.c')
    config = '\n'.join(structure(source, name) for name in ['model_batch_draw', 'model_batch_kind']) + '\n'
    code = function(source, 'model_batch_sort') + '\n'
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('model_batch_sort', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('model_batch_sort', generated((before, after))), case)
    assert status == CHECK_FAILED, output
