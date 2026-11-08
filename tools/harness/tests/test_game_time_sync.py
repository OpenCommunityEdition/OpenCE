"""Actual frame processing catches small reliable-host clock lag without jumps."""
from pathlib import Path
import re
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, constant, enum_with, function, mutated, read, run, structure, CHECK_FAILED


FUNCTIONS = (
    'game_time_initialize', 'game_time_initialize_for_new_map', 'game_time_dispose_from_old_map',
    'game_time_end', 'game_time_set_distributed', 'game_time_synchronize_distributed',
    'game_time_get', 'game_time_get_paused', 'game_time_set_paused',
    'game_time_get_speed', 'game_time_held', 'game_time_menu_scene', 'game_time_start', 'game_time_update',
)


def generated(fault=None):
    time_source = read('source/game/game_time.c')
    config = '\n'.join((
        enum_with(read('source/game/game.h'), '_game_connection_local'),
        enum_with(read('source/scenario/scenario_definitions.h'), '_scenario_type_main_menu'),
        f'#define TICKS_PER_SECOND {constant(read("source/cseries/cseries.h"), "TICKS_PER_SECOND")}',
        f'#define SOME_LARGE_NUMBER_OF_TICKS {constant(time_source, "SOME_LARGE_NUMBER_OF_TICKS")}',
        structure(time_source, 'game_time_globals_struct'),
    ))
    # The target is transient engine state, not a field of the saved clock.
    transient = re.findall(r'^static long game_time_distributed_target\s*=\s*[^;]+;', time_source, re.M)
    assert len(transient) == 1, 'The saved clock fixture needs its actual transient target declaration'
    functions = {name:function(time_source, name) for name in FUNCTIONS}
    functions['network_game_client_handle_game_update'] = function(
        read('source/networking/network_client_manager.c'), 'network_game_client_handle_game_update')
    if fault:
        name, before, after = fault
        functions[name] = mutated(functions[name], before, after)
    source = '\n'.join(transient + list(functions.values()))
    return (('config.inc', config), ('under_test.inc', source))


CASES = (
    'lag-5', 'lag-6', 'lag-7', 'lag-8', 'lag-30', 'scaled-convergence',
    'nominal', 'fraction', 'work-budget', 'target-bound', 'repeat-target',
    'stale-target', 'sub-tick', 'no-target', 'client-ahead', 'held', 'pause',
    'mcc-rate-changes', 'menu', 'local', 'host', 'initial-sync', 'large-stall',
    'invalid-times', 'lifecycle-new-map', 'lifecycle-dispose', 'lifecycle-end',
    'lifecycle-set', 'invalid-targets', 'setter-local', 'setter-host',
    'setter-menu', 'setter-inactive', 'setter-uninitialized', 'negative-clock',
    'lifecycle-initialize', 'initialize-start',
)


@pytest.mark.parametrize('case', CASES)
def test_frame_clock(case):
    status, output = run(build('game_time_sync', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', (
    (('network_game_client_handle_game_update',
      'game_time_synchronize_distributed(message_packet->game_time);', '(void)message_packet;'), 'lag-7'),
    (('game_time_update', 'ticks_elapsed += MIN(extra_ticks, TICKS_PER_SECOND - ticks_elapsed);',
      '(void)extra_ticks;'), 'lag-8'),
    (('game_time_update', 'ticks_elapsed += MIN(extra_ticks, TICKS_PER_SECOND - ticks_elapsed);',
      'game_time_set_distributed(game_time_distributed_target);'), 'lag-7'),
    (('game_time_update', 'MIN(ticks_elapsed, ticks_behind - ticks_elapsed)',
      '(ticks_behind - ticks_elapsed)'), 'lag-8'),
    (('game_time_update', 'MIN(extra_ticks, TICKS_PER_SECOND - ticks_elapsed)',
      'extra_ticks'), 'work-budget'),
    (('game_time_update', '!game_time_menu_scene() &&', 'TRUE &&'), 'menu'),
    (('game_time_update', 'game_time_globals->local_time >= 0 &&', 'TRUE &&'), 'negative-clock'),
    (('game_time_synchronize_distributed', 'time > 0x3FFFFFFF', 'FALSE'), 'invalid-targets'),
    (('game_time_synchronize_distributed', 'game_time_distributed_target = time;',
      '{ game_time_distributed_target = time; game_time_globals->leftover_dt = 0.f; }'), 'fraction'),
    (('game_time_initialize_for_new_map', 'game_time_distributed_target = NONE;', '(void)0;'), 'lifecycle-new-map'),
))
def test_fault_is_detected(fault, case):
    status, output = run(build('game_time_sync', generated(fault)), case)
    assert status == CHECK_FAILED, output
