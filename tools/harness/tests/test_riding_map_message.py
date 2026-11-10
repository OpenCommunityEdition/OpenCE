"""The host's map riding value, reliable and only when a map chooses."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run
from test_aim_assist import message_ids

CONTROLS = {'authority': ('machine_index != NONE ||', 'FALSE ||', 'admission'),
            'send-needs-map': ('if (!sent ||', 'if (vehicle_riding_map() != NONE) if (!sent ||', 'replication'),
            'unchanged-spam': ('!sent || sent_map != vehicle_riding_map() ||', 'TRUE ||', 'replication'),
            'values-not-resent': (' || memcmp(&sent_values, &message.values, sizeof(sent_values))', '', 'values'),
            'values-not-filled': ('vehicle_riding_latched_get(&message->values);', '', 'values'),
            'values-not-handed-on': ('vehicle_riding_latched_received(&message.values);', '', 'client-values'),
            'bad-look-carry-accepted': (' || message.values.look_carry > 1', '', 'client-values'),
            'host-values-kept': ('vehicle_riding_latched_forget();', '', 'client-values')}


def generated(fault=None):
    code = re.sub(r'^#include .*$', '', read('port/linux/game/network_riding_map.c'), flags=re.M)
    if fault:
        before, after, _ = CONTROLS[fault]
        code = mutated(code, before, after)
    return (('under_test.inc', code), ('message_ids.inc', message_ids()))


@pytest.mark.parametrize('case', ['replication', 'admission', 'reset', 'values', 'client-values'])
def test_case(case):
    status, output = run(build('riding_map_message', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault', sorted(CONTROLS))
def test_negative_control(fault):
    status, output = run(build('riding_map_message', generated(fault)), CONTROLS[fault][2])
    assert status == CHECK_FAILED and 'check failed:' in output, output


def test_message_ids():
    import network_format
    messages = {k: int(v) for k, v in network_format.items().items() if k.startswith('enum _distributed_message_')}
    assert messages['enum _distributed_message_vehicle_riding_map'] == 107
    assert len(set(messages.values())) == len(messages), messages
