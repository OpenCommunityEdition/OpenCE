"""A co-op client that joins during a cutscene's sound starts it where the host has reached (coop_sound_sync.c)."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, mutated, read, run, structure

CASES = ['pcm', 'chain', 'stereo-blocks', 'pitch', 'start-skipped', 'start-playing', 'fade-in-at-start', 'finished',
         'repeat', 'late', 'just-started', 'ended', 'placed', 'other-instance', 'unknown-compression', 'taken-once',
         'hostile', 'length', 'length-hostile']
NEGATIVE_CONTROLS = {
    'no-start-time': ('\t\tinto -= sync->start_milliseconds[track_index];\n', '', 'start-skipped'),
    'start-always-before': ('COOP_SOUND_TRACK_FADE_IN_AT_START_BIT))', '31))', 'fade-in-at-start'),
    'no-wrap': ('into % length', 'into', 'repeat'),
    'stereo-as-mono': ('COOP_SOUND_ADPCM_BLOCK_BYTES * channels;\n\t}', 'COOP_SOUND_ADPCM_BLOCK_BYTES;\n\t}',
                       'stereo-blocks'),
    'placed-again': ('if (sync->placed || (', 'if ((', 'placed'),
    'placed-unstarted': ('if (!started || (sync', 'if ((sync', 'placed'),
    'any-instance': ('runtime_scripting_sound_index != loop_identifier)\n\t{', 'runtime_scripting_sound_index !=\n'
                     '\t\tlooping_sound_definition_get(looping_definition_index)->runtime_scripting_sound_index)\n\t{',
                     'other-instance'),
    'stop-after-finish': ('return sync->finished ? _coop_sound_sync_skip : _coop_sound_sync_play;',
                          'return _coop_sound_sync_play;', 'finished'),
    'no-wait': ('return elapsed + (long)since;', 'return elapsed;', 'late'),
    'one-permutation': ('\t\t\t\tindex = permutation->next_permutation_index;\n', '\t\t\t\tbreak;\n', 'length'),
    'nominal-pitch': ('coop_sound_pitch_or_one(definition->random_pitch_bounds.lower) *', '', 'length'),
    'offset-kept': ('\t\t\tcoop_sound_offsets[index].sound_index = NONE;\n', '', 'taken-once'),
}


def generated(fault=None):
    module = read('port/linux/game/coop_sound_sync.c')
    header = read('port/linux/game/coop_sound_sync.h')
    definitions = read('source/sound/sound_definitions.h')
    types = enum_with(definitions, '_sound_definition_linked_permutations_bit') + '\n'
    types += f"#define MAXIMUM_TRACKS_PER_LOOPING_SOUND {constant(definitions, 'MAXIMUM_TRACKS_PER_LOOPING_SOUND')}\n"
    for path, names in [('source/tag_files/tag_groups.h', ['tag_block', 'tag_reference', 'tag_data']),
                        ('source/sound/sound_definitions.h', ['real_bounds', 'sound_permutation', 'sound_pitch_range',
                         'sound_definition', 'looping_sound_scale_modifiers', 'looping_sound_track',
                         'looping_sound_definition'])]:
        for name in names:
            types += structure(read(path), name) + '\n'
    # The module's own header: its parts, placements and functions.
    types += header + '\n'
    # The module itself, from its constants on (its includes are the fixture's).
    code = module[module.index('/* ---------- constants */'):]
    if fault:
        code = mutated(code, fault[0], fault[1])
    return (('types.inc', types), ('under_test.inc', code))


def test_track_flag_is_the_sound_managers():
    """The module's fade in at start bit is sound_manager.c's _fade_in_at_start_bit."""
    flags = enum_with(read('source/sound/sound_manager.c'), '_fade_in_at_start_bit')
    members = re.findall(r'^\s*(\w+)\s*,', flags[flags.index('{'):], re.M)
    assert members.index('_fade_in_at_start_bit') == \
        constant(read('port/linux/game/coop_sound_sync.c'), 'COOP_SOUND_TRACK_FADE_IN_AT_START_BIT')


def test_blocks_are_the_channels():
    """The module's ADPCM blocks and sample rates are those sound_dsound_xbox.c's channels play."""
    module = read('port/linux/game/coop_sound_sync.c')
    dsound = read('source/sound/sound_dsound_xbox.c')
    assert constant(module, 'COOP_SOUND_ADPCM_BLOCK_BYTES') == constant(dsound, 'SOUND_COMPRESSED_BLOCK_SIZE')
    assert constant(module, 'COOP_SOUND_ADPCM_BLOCK_SAMPLES') == constant(dsound, 'SOUND_COMPRESSED_SAMPLES_PER_BLOCK')
    assert constant(module, 'COOP_SOUND_SAMPLE_RATES') == constant(dsound, 'NUMBER_OF_SOUND_SAMPLE_RATES')


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('coop_sound_sync', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('control', NEGATIVE_CONTROLS)
def test_negative_control(control):
    before, after, case = NEGATIVE_CONTROLS[control]
    status, output = run(build('coop_sound_sync', generated((before, after))), case)
    assert status == CHECK_FAILED, f'{control} escaped {case}: {output}'
