"""The lruv cache keeps its list of blocks whole through making, eviction, deletion, flushes and resizes."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run

CASES = ['sequence', 'ends', 'damaged']


def generated(fault=None):
    code = re.sub(r'^#include .*$', '', read('source/memory/lruv_cache.c'), flags=re.M)
    header = re.sub(r'^#include .*$', '', read('source/memory/lruv_cache.h'), flags=re.M)
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', header + '\n' + code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('lruv_cache', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    # a delete that does not check the block's links first
    (('\tlruv_cache_port_verify_block(cache, block_index);\n\n\tif (cache->delete_block_proc)',
      '\n\tif (cache->delete_block_proc)'), 'damaged'),
    # the last block deleted without the cache's last moving back
    (('\t\tcache->last_block_index = block->previous_block_index;\n', '\n'), 'ends'),
    # a block made before the first left out of the first's links
    (('\t\t\t\t\tnew_block->previous_block_index = NONE;\n\t\t\t\t\tnext_block->previous_block_index = new_block_index;\n',
      '\t\t\t\t\tnew_block->previous_block_index = NONE;\n'), 'sequence'),
])
def test_negative_control(fault, case):
    status, output = run(build('lruv_cache', generated(fault)), case)
    assert status == CHECK_FAILED, output
