"""The real bind_textures (d3d8_gl.c): a texture stage bound again as it was, or to one of the few textures it took
turns between, is given the texture and sampler its lookup gave without looking either up; a written page, a changed lookup or setting, a header rewritten in place or a
sampler state changed looks them up again, and a render target's or a palettized or replaced texture's lookup is
never remembered."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ['remembered', 'serials', 'header', 'sampler-states', 'not-direct', 'render-target', 'key', 'ways']

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # a page written since not looked at
    'watch-unchecked': (('entry->watch_serial != watch_serial', '0'), 'serials'),
    # a lookup that could give something else not looked at
    'lookup-unchecked': (('entry->lookup_serial != lookup_serial', '0'), 'serials'),
    # a setting changed not looked at
    'setting-unchecked': (('\t\tentry->config_serial != config_changes())', '\t\t0)'), 'serials'),
    # the header taken by its address alone
    'header-unchecked': (('difference = (entry->data ^ texture->Data) | (entry->format ^ texture->Format) | '
                          '(entry->size ^ texture->Size);', 'difference = 0;'), 'header'),
    # the sampler states not compared
    'sampler-unchecked': (('difference |= entry->sampler_states[index] ^ state[texture_stage_sampler_states[index]];',
                           ';'), 'sampler-states'),
    # one texture remembered a stage, not one a way
    'one-way': (('\t\tway = texture_stage_next_way[stage]++ % TEXTURE_STAGE_WAYS;',
                 '\t\tway = 0;'), 'ways'),
    # a texture remembered again in the next way, forgetting another, rather than its own
    'way-not-reused': (('\t/* (in the way it was remembered in before, if it was: once a way) */\n'
                        '\tfor (way = 0; way < TEXTURE_STAGE_WAYS && texture_stages[stage][way].texture != texture; '
                        'way++)\n\t\t;\n', '\tway = TEXTURE_STAGE_WAYS;\n'), 'ways'),
    # every lookup remembered, a palettized or replaced texture's too
    'not-direct-remembered': (('if (!target && description.direct)', 'if (!target)'), 'not-direct'),
    # a render target's lookup remembered
    'render-target-remembered': (('if (!target && description.direct)', 'if (description.direct)'), 'render-target'),
    # a remembered stage's sampler not bound again after the bindings were lost
    'sampler-not-bound': (('\t\t\tstate_sampler(stage, remembered->sampler);\n', ''), 'remembered'),
    # a remembered linear texture's scale left at 1
    'scale-not-given': (('\t\t\ttexture_scale[stage][0] = remembered->scale[0];\n', ''), 'remembered'),
    # a remembered stage 0 keeping whatever meter channels the key had
    'key-kept': (('\t\t\t\tkey->coverage_alpha = key->point_threshold = key->ce_meter_channels = 0;\n', ''), 'key'),
}


def generated(fault=None):
    source = read('port/linux/src/d3d8_gl.c')
    start = source.index('/* Draws mostly bind the textures the draws before them bound, a stage taking')
    code = function(source, 'stage_texture_mode') + '\n' + source[start:source.index('static GLenum stencil_operation(')]
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('texture_stage_memo', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build('texture_stage_memo', generated(fault)), case)
    assert status == CHECK_FAILED, f"bind_textures with a fault ({control}) passed '{case}': the test cannot see it"
