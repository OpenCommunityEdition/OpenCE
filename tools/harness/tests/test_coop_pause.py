"""Co-op pause routing, host authority, and generated multiplayer layout."""
import sys
from pathlib import Path
import xml.etree.ElementTree as ET
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from harness import build,function,read,run,mutated,CHECK_FAILED
import port_settings

def generated(fault=None):
    leave=function(read("port/linux/game/menu_functions.c"),"coop_leave_game")
    route=function(read("port/linux/game/menu_tags.c"),"pc_menus_screen")
    if fault=="host":leave=mutated(leave,"if (global_network_game_server_get())","if (TRUE)")
    if fault=="client":leave=mutated(leave,"network_game_server_port_cooperative_won(NULL);","network_game_client_local_player_quit(0);")
    return (("under_test.inc",leave+"\n"+route),)

@pytest.mark.parametrize("case",["host","client","local","routes"])
def test_case(case):
    status,output=run(build("coop_pause",generated()),case)
    assert status==0,output

@pytest.mark.parametrize("fault,case",[("host","client"),("client","host")])
def test_negative(fault,case):
    status,output=run(build("coop_pause",generated(fault)),case)
    assert status==CHECK_FAILED,output

def test_generated_menu():
    text="\n".join(port_settings.multiplayer_files()["pause.coop.xml"])
    assert text==read("port/assets/menus/ce/pause.coop.xml")
    widgets={w.attrib["name"]:w for w in ET.fromstring(text).findall("widget")}
    base="pause/coop/"
    for n,width,height in [(1,640,480),(2,640,240),(4,320,240)]:
        w=widgets[base+str(n)+"p_pause_game"]
        assert (int(w.attrib["width"]),int(w.attrib["height"]))==(width,height)
        assert "pause_game" not in w.attrib["flags"].split()
        assert {h.attrib["event"] for h in w.findall("on")}=={"b","back"}
        assert all(h.attrib["close"]=="all" for h in w.findall("on"))
        for c in w.findall("child"):
            assert 0<=int(c.attrib["x"])<width and 0<=int(c.attrib["y"])<height
    assert [c.attrib["widget"] for c in widgets[base+"list"].findall("child")]==[base+s for s in ["resume","settings","leave"]]
    for s in ["resume","settings","leave"]:
        assert {h.attrib["event"] for h in widgets[base+s].findall("on")}=={"a","start","left_mouse"}
