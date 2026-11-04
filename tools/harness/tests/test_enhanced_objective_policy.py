import sys
from pathlib import Path
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from harness import read,function,build,run
@pytest.mark.parametrize("case",["attacker","defender","carrier","hill"])
def test_objective_policy(case):
    source=read("port/linux/game/enhanced_bots.c")
    code=function(source,"enhanced_bots_goal")+function(source,"enhanced_bots_advancing")
    status,output=run(build("enhanced_objective_policy",(("under_test.inc",code),)),case)
    assert status==0,output
