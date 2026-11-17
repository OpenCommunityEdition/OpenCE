"""The memory pool keeps its blocks' contents and links, and its checks still catch a damaged pool or block."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run, structure

CASES = ['sequence', 'ends', 'empty-pool-damaged', 'block-damaged']


def generated(fault=None):
    header = read('source/memory/memory_pool.h')
    code = re.sub(r'^#include .*$', '', read('source/memory/memory_pool.c'), flags=re.M)
    if fault:
        code = mutated(code, *fault)
    structures = structure(header, 'memory_pool_block') + '\n' + structure(header, 'memory_pool') + '\n'
    return (('under_test.inc', structures + code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('memory_pool', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    # an empty pool's allocation not checked
    (('\tmemory_pool_port_verify_block(pool, pool->last_block);\n\tmatch_assert',
      '\tif (pool->last_block)\n\t\tmemory_pool_port_verify_block(pool, pool->last_block);\n\tmatch_assert'),
     'empty-pool-damaged'),
    # a block's link from the one before not checked
    (('352, block->previous_block ? block->previous_block->next_block==block : pool->first_block==block',
      '352, TRUE'), 'block-damaged'),
    # a moved block's owner not told
    (('\t\t\t\t*block->reference = block+1;\n', '\n'), 'sequence'),
])
def test_negative_control(fault, case):
    status, output = run(build('memory_pool', generated(fault)), case)
    assert status == CHECK_FAILED, output
