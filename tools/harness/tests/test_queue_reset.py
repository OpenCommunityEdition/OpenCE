"""Checkpoint input queue resets with absent slot zero and multiple players."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED,build,function,mutated,read,run

def generated(fault=None):
    source=read('source/game/player_queues_new.c')
    code='\n'.join(function(source,n) for n in ['update_client_start','update_server_start','update_queues_reset_and_fill_with_lies'])
    if fault:code=mutated(code,*fault)
    return (('under_test.inc',code),)

@pytest.mark.parametrize('case',['empty','sparse','slot-zero','eight','full','dedicated','client-only','uninitialized','early'])
def test_reset(case):
    status,output=run(build('queue_reset',generated()),case)
    assert status==0,output

@pytest.mark.parametrize('fault,case',[
    (('while ((queue = data_iterator_next(&iterator)) != NULL)','if ((queue = data_iterator_next(&iterator)) != NULL)'), 'eight'),
    (('queue->next_update_number = update_server_globals.next_update_number_to_build;', 'queue->next_update_number = 0;'), 'sparse'),
])
def test_negative_control(fault,case):
    status,output=run(build('queue_reset',generated(fault)),case)
    assert status==CHECK_FAILED,output
