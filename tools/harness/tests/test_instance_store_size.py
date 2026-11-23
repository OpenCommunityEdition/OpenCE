"""Batching's stores (d3d8_gl.c): twice what a recent frame wrote, in steps of the least, to the ceiling; larger at
once, smaller only after a wait (a time, the same at any frame rate) in which no frame wrote a quarter."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure

CASES = ['grow', 'ceiling', 'shrink', 'frame rates', 'scenes']
CONTROLS = [
    # a store only as large as the frame (no room to grow into)
    ([('bytes = (bytes * 2 + ', 'bytes = (bytes + ')], 'grow'),
    # no ceiling
    ([('return bytes < store->ceiling ? bytes : store->ceiling;', 'return bytes;')], 'ceiling'),
    # made smaller as soon as one frame writes under a quarter
    ([('now - store->since >= INSTANCE_STORE_SHRINK_MILLISECONDS', 'now - store->since >= 1')], 'shrink'),
    # the wait a count of frames (1,800, half a minute at 60 frames a second), not a time
    ([('now - store->since >= INSTANCE_STORE_SHRINK_MILLISECONDS', '++store->since >= 1800'),
      ('store->since = now;', 'store->since = 0;')], 'frame rates'),
    # the wait not started again: smaller a frame after the wait the crowd's frame was in
    ([('\t\tstore->since = now;\n', '')], 'scenes'),
    # the most of the frames counted never forgotten: never smaller again
    ([('\t\tstore->most = 0;\n', '')], 'scenes'),
]


def define(source, name):
    return re.search(r'^#define ' + name + r' .*', source, re.M).group()


def generated(faults=()):
    source = read('port/linux/src/d3d8_gl.c')
    code = '\n'.join(
        [define(source, name) for name in
         ['INSTANCE_STORE_LEAST', 'INSTANCE_STORE_MOST', 'INSTANCE_STORE_SHRINK_MILLISECONDS']] +
        [structure(source, 'instance_store_size')] +
        [function(source, name) for name in ['instance_store_bytes', 'instance_store_frame']]) + '\n'
    for before, after in faults:
        code = mutated(code, before, after)
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('instance_store_size', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('faults,case', CONTROLS)
def test_negative_control(faults, case):
    status, output = run(build('instance_store_size', generated(tuple(faults))), case)
    assert status == CHECK_FAILED, output
