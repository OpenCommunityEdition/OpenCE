"""Compile the complete production self-kill handler against a small world."""
import re,sys
from pathlib import Path
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from harness import build,read,mutated,run,CHECK_FAILED
def generated(fault=None):
    code=re.sub(r'^#include "[^"\n]+"\n','',read('port/linux/game/network_suicide.c'),flags=re.M)
    if fault=='stream': code=mutated(code,'!from_stream ||','FALSE ||')
    if fault=='owner': code=mutated(code,'player->network_player_data.machine_index != machine_index','FALSE')
    if fault=='unit': code=mutated(code,'player->unit_index == unit_index &&','TRUE &&')
    return (('under_test.inc',code),('message_ids.inc',message_ids()))
@pytest.mark.parametrize('case',['commands','client','admission','stale','rate','scope'])
def test_case(case):
    status,output=run(build('console_suicide',generated()),case)
    assert status==0,output
@pytest.mark.parametrize('fault,case',[('stream','admission'),('owner','admission'),('unit','stale')])
def test_negative(fault,case):
    status,output=run(build('console_suicide',generated(fault)),case)
    assert status==CHECK_FAILED,output


def message_ids():
    """Compile the production IDs so map-sharing collisions reach the tests."""
    return re.search(r'enum\s*\{.*?_distributed_message_player_prediction.*?\};',
                     read('port/linux/game/network_distributed.h'), re.S).group(0)


def test_message_ids():
    import network_format
    messages = {name: int(value) for name, value in network_format.items().items()
                if name.startswith('enum _distributed_message_')}
    assert len(set(messages.values())) == len(messages), messages
    assert messages['enum _distributed_message_suicide_request'] == 88
    assert [messages['enum _distributed_message_map_share_' + name]
            for name in ('request', 'offer', 'sources', 'progress')] == [83, 84, 85, 86]
