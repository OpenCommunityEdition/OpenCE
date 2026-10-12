"""The complete co-op death formatter, with full-handle and mode checks."""
import re,sys
from pathlib import Path
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from harness import build,read,mutated,run,CHECK_FAILED
def generated(fault=None):
    code=re.sub(r'^#include "[^"\n]+"\n','',read('port/linux/game/network_killfeed.c'),flags=re.M)
    if fault=='host':code=mutated(code,'game_connection() != _game_connection_network_server','FALSE')
    if fault=='dedup':code=mutated(code,'killfeed_dead_units[slot] == unit_index','FALSE')
    if fault=='backlink':code=mutated(code,'dead->unit_index != unit_index','FALSE')
    dispatch=read('port/linux/game/network_distributed.c').split('case _distributed_message_coop_death:',2)[2].split('case _distributed_message_coop_events:',1)[0]
    if fault=='stream':dispatch=mutated(dispatch,'!network_connection_last_read_was_unreliable()','distributed_handling_stream_message')
    dispatch='static void dispatch(void const *entries,word size,byte count) { struct distributed_message_header header={0}; header.count=count; switch(_distributed_message_coop_death) { case _distributed_message_coop_death: '+dispatch+'} }'
    return (('under_test.inc',code),('dispatch.inc',dispatch),('message_ids.inc',message_ids()))
@pytest.mark.parametrize('case',['causes','players','dedup','scope','stale','fallback','direct','assets','hud'])
def test_case(case):
    status,output=run(build('coop_killfeed',generated()),case)
    assert status==0,output
@pytest.mark.parametrize('fault,case',[('host','scope'),('dedup','dedup'),('backlink','stale'),('stream','hud')])
def test_negative(fault,case):
    status,output=run(build('coop_killfeed',generated(fault)),case)
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
    assert messages['enum _distributed_message_coop_death'] == 89
    assert [messages['enum _distributed_message_map_share_' + name]
            for name in ('request', 'offer', 'sources', 'progress')] == [83, 84, 85, 86]
