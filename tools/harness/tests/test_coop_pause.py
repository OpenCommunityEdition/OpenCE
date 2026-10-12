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
    tags=read("port/linux/game/menu_tags.c")
    route='\n'.join(function(tags,n) for n in ['pc_menus_screen','pc_menus_coop_pause_screen','pc_menus_postgame_screen'])
    reset=function(read('source/interface/ui_widget.c'),'network_game_reset_to_pregame_ui')
    transition=function(read('source/networking/network_client_manager.c'),'network_game_client_switch_to_pregame')
    if fault=="host":leave=mutated(leave,"if (global_network_game_server_get())","if (TRUE)")
    if fault=="client":leave=mutated(leave,"network_game_server_port_cooperative_won(NULL);","network_game_client_local_player_quit(0);")
    if fault=='xbox-pause':route=mutated(route,'if (menu_tags.loaded)','if (TRUE)')
    if fault=='xbox-postgame':route=mutated(route,'if (menu_tags.loaded && cooperative)','if (cooperative)')
    if fault=='mp-postgame':reset=mutated(reset,'pc_menus_postgame_screen(cooperative)','pc_menus_postgame_screen(TRUE)')
    if fault=='late-capture':
        transition=mutated(transition,'boolean cooperative = network_coop_active();','boolean cooperative;')
        transition=mutated(transition,'network_game_reset_for_next_round(&client->game, TRUE);',
                           'network_game_reset_for_next_round(&client->game, TRUE); cooperative = network_coop_active();')
    return (("under_test.inc",'\n'.join([leave,route,reset,transition])),)

@pytest.mark.parametrize("case",['host','client','local','pause-pc','pause-xbox','generic-routes',
    'coop-pc-host','coop-pc-client','coop-xbox-host','coop-xbox-client',
    'mp-pc-host','mp-pc-client','mp-xbox-host','mp-xbox-client','already-pregame','splitscreen'])
def test_case(case):
    status,output=run(build("coop_pause",generated()),case)
    assert status==0,output

@pytest.mark.parametrize("fault,case",[("host","client"),("client","host"),
    ('xbox-pause','pause-xbox'),('xbox-postgame','coop-xbox-host'),
    ('mp-postgame','mp-pc-host'),('late-capture','coop-pc-host')])
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
