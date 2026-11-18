"""Palettized and 32-bit textures decoded a row at a time give the texels convert_texel gives."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure

# decode_level's row-at-a-time paths, which the reference has off
FAST_PATHS = [('if (information.kind == _texel_p8 && palette)', 'if (0)'),
              ('else if (information.kind == _texel_a8r8g8b8 || information.kind == _texel_x8r8g8b8)', 'else if (0)')]


def generated(fault=None):
    source = read('port/linux/src/xbox_textures.c')
    decoder = function(source, 'decode_level')
    texel_macros = re.search(r'^#define TEXEL16.*?\n#define TEXEL32[^\n]*\n', source, re.S | re.M).group(0)
    helpers = '\n\n'.join([enum_with(source, '_texel_p8') + ';', structure(source, 'format_information'),
                           structure(source, 'swizzle_masks')] +
                          [function(source, name) for name in (
                              'format_information', 'kind_compressed', 'floor_log2', 'level_dimension',
                              'swizzle_masks', 'spread', 'expand5', 'expand6', 'expand4', 'argb', 'clamp_byte',
                              'yuv_to_argb')] +
                          [texel_macros, function(source, 'convert_texel')]) + '\n\n'
    reference = decoder.replace('decode_level(', 'decode_level_reference(', 1)
    for before, after in FAST_PATHS:
        reference = mutated(reference, before, after)
    if fault:
        decoder = mutated(decoder, *fault)
    structures = structure(read('port/linux/src/xgpu.h'), 'xgpu_texture_description')
    return (('structures.inc', structures + '\n'),
            ('under_test.inc', helpers + decoder + '\n\n' + reference + '\n'))


def test_identical():
    status, output = run(build('texture_decode_rows', generated()), 'identical')
    assert status == 0, output


@pytest.mark.parametrize('fault', [
    # a palette entry looked up by the texel's place in the row, not in the swizzle
    ('row[x] = palette[source[x_offsets[x] | y_offset]];', 'row[x] = palette[source[x | y_offset]];'),
    # X8R8G8B8's alpha taken from the texel
    ("? 0xff000000UL : 0;", "? 0 : 0;"),
])
def test_negative_control(fault):
    status, output = run(build('texture_decode_rows', generated(fault)), 'identical')
    assert status == CHECK_FAILED, output
