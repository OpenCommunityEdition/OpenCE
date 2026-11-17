"""The enlarged HUD sheets are shared by their pixels and deleted only when no texture uses them."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure

CASES = ['reuse', 'distinct', 'distinct-same-crc', 'eviction', 'bytes']


def generated(fault=None):
    source = read('port/linux/src/hud_hires.c')
    constants = '\n'.join(re.findall(r'^#define UPSCALE_CACHE_\w+.*$', source, re.M))
    code = '\n\n'.join([
        constants,
        structure(source, 'upscale_cached'),
        'static struct upscale_cached upscale_cache[UPSCALE_CACHE_SIZE];',
        'static unsigned long upscale_cache_clock;',
        function(source, 'upscale_cache_trim'),
        function(source, 'hud_hires_upscale_release'),
        function(source, 'hud_hires_upscale_texture'),
    ])
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code + '\n'),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('hud_upscale_cache', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    # a texture in use counted as unused
    (('if (!upscale_cache[index].texture || upscale_cache[index].users > 0)',
      'if (!upscale_cache[index].texture)'), 'eviction'),
    # sheets matched by their CRC, not their pixels
    (('&&\n\t\t\t!memcmp(cached->source, bgra, width * height * 4))', ')'), 'distinct-same-crc'),
    # sheets of another kind matched
    (('cached->height == height && cached->kind == kind &&', 'cached->height == height &&'), 'distinct'),
    # the bytes kept unused not limited
    (('unused_bytes += upscale_cache[index].bytes;', 'unused_bytes += 0;'), 'bytes'),
])
def test_negative_control(fault, case):
    status, output = run(build('hud_upscale_cache', generated(fault)), case)
    assert status == CHECK_FAILED, output
