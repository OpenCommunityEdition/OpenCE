"""Where a client's players' shots leave from when not from their units' cameras (a VR build's hand): taken by the
host for its players' own machine only and within a player's reach, and sent only by a build that has one."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure


def generated(fault=None):
    source = read('port/linux/game/network_damage.c')
    distributed = read('port/linux/game/network_distributed.c')
    header = read('port/linux/game/network_distributed.h')
    defines = '\n'.join(re.findall(r'^#define FIRE_ORIGIN_\w+ [^\n]+', source, re.M))
    sent = re.search(r'^static struct\n\{\n\tlong time;\n\tstruct distributed_vector offset;\n\} damage_fire_origins_sent\[[^;]+;',
                     source, re.M).group()
    fresh = re.search(r'^\s*(FIRE_ORIGIN_FRESH_TICKS) = ([^,]+),', source, re.M)
    state = re.search(r'^static struct damage_fire_origin\n\{.*?\} damage_fire_origins\[MAXIMUM_TRACKED_PLAYERS\];',
                      source, re.M | re.S).group()
    source_type = re.search(r'^typedef boolean \(\*network_damage_fire_origin_source\)[^;]+;', header, re.M).group()
    code = '\n'.join([
        '#define TICKS_PER_SECOND 30', f'#define {fresh.group(1)} ({fresh.group(2)})', defines, source_type,
        structure(header, 'distributed_vector'), structure(header, 'distributed_message_header'),
        structure(source, 'distributed_fire_origin'), structure(source, 'distributed_fire_origin_message'),
        state, 'static network_damage_fire_origin_source damage_fire_origin_source;', sent,
        function(distributed, 'distributed_vector_pack'), function(distributed, 'distributed_vector_unpack'),
        function(source, 'distributed_fire_origin_within_reach'),
        function(source, 'distributed_fire_origin_shape'),
        function(source, 'network_damage_handle_fire_origins'),
        function(source, 'network_damage_set_fire_origin_source'),
    ])
    send = function(source, 'distributed_send_fire_origins')
    if fault:
        code, send = (mutated(code, *fault[1:]), send) if fault[0] == 'code' else (code, mutated(send, *fault[1:]))
    return (('under_test.inc', code), ('under_test_send.inc', send))


@pytest.mark.parametrize('case', ['own', 'foreign', 'reach', 'send', 'resend', 'shape'])
def test_case(case):
    status, output = run(build('fire_origins', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('code', ' ||\n\t\t\t!distributed_machine_has_player(machine_index, origin.player_index)', ''), 'foreign'),
    (('code', 'if (!distributed_fire_origin_within_reach(&offset))\n\t\t\tcontinue;', ''), 'reach'),
    (('code', 'damage_fire_origins[origin.player_index].time = game_time_get();', ''), 'own'),
    (('send', ' ||\n\t\t\t!distributed_fire_origin_within_reach(&offset)', ''), 'send'),
    (('send', '< FIRE_ORIGIN_RESEND_TICKS &&', '< 0 &&'), 'resend'),
    (('send', 'damage_fire_origins_sent[local_player_index].offset = message.origins[count].offset;', ''), 'resend'),
    (('code', 'along >= -FIRE_ORIGIN_BEHIND && ', ''), 'shape'),
    (('code', 'sideways_squared <= FIRE_ORIGIN_SIDEWAYS * FIRE_ORIGIN_SIDEWAYS', 'sideways_squared >= 0.0f'), 'shape'),
    (('code', ' || offset.k > FIRE_ORIGIN_ABOVE', ''), 'shape'),
])
def test_negative_control(fault, case):
    status, output = run(build('fire_origins', generated(fault)), case)
    assert status == CHECK_FAILED, output
