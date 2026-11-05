"""Exercise the real emitter policy, render-time easing and light ownership."""
import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, read, run


def source():
    return re.sub(r'^#include.*$', '', read('port/linux/game/ambient_emitter_lighting.h') + '\n' + read('port/linux/game/needler_lighting.c') + '\n' + read('port/linux/game/ammo_counter_lighting.c') + '\n' + read('port/linux/game/covenant_emitter_lighting.c'), flags=re.M)


def test_emitters():
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', source()),)), 'emitters')
    assert status == 0, output


def test_ambient_saturation_preserves_existing_emitters():
    code = source().replace('sample.kind < EMITTER_VEHICLE && sample.strength > 0.0f', 'FALSE')
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', code),)), 'emitters')
    assert status == CHECK_FAILED, output


def test_shield_permutation_guard_is_required():
    code = source().replace('object->object.region_permutations[i] == 0', 'TRUE')
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', code),)), 'emitters')
    assert status == CHECK_FAILED, output


def test_tick_only_fade_is_detected():
    code = source().replace('render_interpolation_game_time_sec(game_time_get())', '(real)game_time_get() / 30.0f')
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', code),)), 'emitters')
    assert status == CHECK_FAILED, output


def test_first_person_counter_pose_is_required():
    code = source().replace('if (first_person_weapon_adjust_light(index, "primary trigger", position, &forward, &up))', 'if (FALSE)')
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', code),)), 'emitters')
    assert status == CHECK_FAILED, output


def test_shield_damage_colour_is_required():
    code = source().replace('PIN(object->object.outgoing_function_values[0], 0.0f, 1.0f)', '0.0f')
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', code),)), 'emitters')
    assert status == CHECK_FAILED, output


def test_needle_crystal_glow_is_required():
    code = source().replace('*output |= 0x10000UL;', '*output |= 0;')
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', code),)), 'emitters')
    assert status == CHECK_FAILED, output


def test_first_person_counter_strength_is_required():
    code = source().replace('counter_pose == 2 ? 1.35f / 0.65f : 1.0f', '1.0f')
    status, output = run(build('covenant_emitter_lighting', (('under_test.inc', code),)), 'emitters')
    assert status == CHECK_FAILED, output
