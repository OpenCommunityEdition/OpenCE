"""The texture cache drops a texture not drawn for half a minute of time, at any frame rate."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run

CASES = ['fast', 'steady', 'late']
SOURCE = 'port/linux/src/xbox_textures.c'


def generated(fault=None, config_fault=None):
    source = read(SOURCE)
    config = '\n'.join(re.findall(r'^#define TEXTURE_\w+.*$', source, re.M) + re.findall(
        r'^static (?:unsigned long|Uint64) texture_(?:frame|drop_serial|sweep_\w+|next_sweep)\b.*;$', source, re.M))
    code = function(source, 'xgpu_texture_cache_begin_frame')
    if config_fault:
        config = mutated(config, *config_fault)
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config + '\n'), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('texture_idle', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    # a sweep every frame
    (('texture_next_sweep = now + TEXTURE_SWEEP_SECONDS * 1000;', 'texture_next_sweep = now;'), 'fast'),
    # the sweep's own frame taken as the one half a minute before
    (('\tidle_since = texture_sweep_frames[texture_sweep_count % TEXTURE_SWEEP_COUNT];\n'
      '\ttexture_sweep_frames[texture_sweep_count % TEXTURE_SWEEP_COUNT] = texture_frame;\n',
      '\ttexture_sweep_frames[texture_sweep_count % TEXTURE_SWEEP_COUNT] = texture_frame;\n'
      '\tidle_since = texture_sweep_frames[texture_sweep_count % TEXTURE_SWEEP_COUNT];\n'), 'fast'),
    # the sweeps not counted: the one before taken each time
    (('\ttexture_sweep_count++;\n', '\n'), 'late'),
])
def test_negative_control(fault, case):
    status, output = run(build('texture_idle', generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_negative_control_idle_time():
    fault = ('#define TEXTURE_IDLE_SECONDS 30', '#define TEXTURE_IDLE_SECONDS 60')
    status, output = run(build('texture_idle', generated(config_fault=fault)), 'steady')
    assert status == CHECK_FAILED, output
