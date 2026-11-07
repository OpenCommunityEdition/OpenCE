"""Real camera/cloth updates and saved-state dispatcher over a minimal render world."""
import re
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure


def generated(missing=None):
    camera = read('source/effects/player_effects.c')
    damage = read('source/objects/damage_effect_definitions.h')
    flags = read('source/objects/widgets/flags.c')
    config = '\n'.join(structure(damage, n) for n in ('screen_flash_definition', 'camera_impulse_definition', 'camera_shake_definition'))
    config += '\n' + '\n'.join(structure(camera, n) for n in ('continuous_player_effect_datum', 'player_effect_datum', 'screen_fade_definition', 'scripted_player_effect_definition', 'player_effect_globals_definition'))
    config += '\n' + enum_with(camera, '_scripted_player_effect_active_bit')
    config += '\n' + enum_with(camera, '_player_effect_screen_flash_just_started_bit')
    cache = re.search(r'^static struct\s*\{[^}]*\} camera_effect_of_tick\[[^;]*;', camera, re.M).group(0)
    clocks = flags.split('/* ---------- globals */')[1].split('/* ---------- public code */')[0]
    code = cache + '\n' + clocks + '\n'
    code += '\n'.join(function(camera, n) for n in ('player_effect_get', 'player_effect_reset_render_state', 'player_effect_initialize_for_new_map', 'player_effect_dispose_from_old_map', 'player_effect_get_camera_effect_matrix'))
    code += '\n' + '\n'.join(function(flags, n) for n in ('flags_reset_render_state', 'flags_initialize_for_new_map', 'flags_dispose_from_old_map', 'flags_update'))
    state = read('source/saved games/game_state.c')
    callbacks = re.search(r'static game_state_after_load_proc after_load_procs\[\] =\s*\{.*?\};', state, re.S).group(0)
    names = re.findall(r'^\s*(\w+)\s*,?\s*$', callbacks, re.M)
    code += '\ntypedef void (*game_state_after_load_proc)(void);\n'
    code += '\n'.join(f'static void {name}(void) {{ }}' for name in names if name not in ('player_effect_reset_render_state', 'flags_reset_render_state'))
    if missing:
        callbacks = mutated(callbacks, '\t' + missing + ',', '')
    code += '\n' + callbacks + '\n' + function(state, 'game_state_call_after_load_procs')
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', ['camera-checkpoint', 'camera-maps', 'camera-windows', 'cloth-checkpoint', 'cloth-maps', 'cloth-frames'])
def test_lifecycle(case):
    status, output = run(build('render_effect_lifecycle', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('hook,case', [('player_effect_reset_render_state', 'camera-checkpoint'), ('flags_reset_render_state', 'cloth-checkpoint')])
def test_missing_hook(hook, case):
    status, output = run(build('render_effect_lifecycle', generated(hook)), case)
    assert status == CHECK_FAILED, output
