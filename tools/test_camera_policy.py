"""Host camera policy, using the actual policy and pregame setter sources."""
import re
import shutil
import subprocess
import sys
from pathlib import Path
import pytest

ROOT = Path(__file__).resolve().parents[1]

@pytest.mark.parametrize("fault", [None, "local_override", "locked_toggle", "invalid_flags", "ingame_change"])
def test_camera_policy(tmp_path, fault):
    from harness import function
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    header = (ROOT / "port/linux/game/third_person.h").read_text()
    enums = re.search(r"enum third_person_match_policy\s*\{.*?\};", header, re.S).group(0)
    defines = "\n".join(re.findall(r"^#define THIRD_PERSON_POLICY_.*", header, re.M))
    policy = re.sub(r"^#include[^\n]*", "", (ROOT / "port/linux/game/third_person_policy.c").read_text(), flags=re.M)
    setter = function((ROOT / "source/networking/network_server_manager.c").read_text(), "network_game_server_port_set_camera_policy")
    menus = (ROOT / "port/linux/game/menu_functions.c").read_text()
    menu_functions = function(menus, "camera_setting_show") + function(menus, "setting_changed_save")
    if fault == "local_override":
        policy = policy.replace("policy == _third_person_player_choice && config_boolean", "config_boolean")
    elif fault == "locked_toggle":
        policy = policy.replace("third_person_policy() == _third_person_player_choice)", "1)")
    elif fault == "invalid_flags":
        policy = policy.replace("<= _third_person_forced_off;", "<= 3;")
    elif fault == "ingame_change":
        setter = setter.replace("server->state != _network_game_server_state_pregame ||", "")
    stub = r'''
#include <string.h>
#include <stddef.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
enum {HALO_KEYBOARD_TOGGLE_PERSPECTIVE=21,HALO_KEYBOARD_SWAP_SHOULDER=19};
enum {_network_game_server_state_pregame=0};
struct network_game {unsigned char cooperative_flags;};
struct network_game_server {int state;struct network_game game;};
static struct network_game_server host;
static struct network_game *current;
static struct network_game_server *server_pointer;
static char const *host_setting="player_choice";
static int preference,shoulder,writes,sends;
static int capability=1;
static int third_person_session_active(void){return capability;}
static int network_game_is_active(void){return current!=NULL;}
static int network_game_is_splitscreen_local(void){return 0;}
static unsigned long held;
static struct network_game *network_game_get_game(void){return current;}
static struct network_game_server *global_network_game_server_get(void){return server_pointer;}
static int network_game_server_send_game_data_pregame(struct network_game_server *s){(void)s;sends++;return 1;}
static void network_event(char const *s){(void)s;}
char const *config_string(char const *s){(void)s;return host_setting;}
int config_boolean(char const *s){return strstr(s,"left_shoulder")?shoulder:preference;}
int config_write(char const *s,char const *v){int value=!strcmp(v,"true");if(strstr(s,"left_shoulder"))shoulder=value;else preference=value;writes++;return 1;}
static unsigned long halo_keyboard_actions(short p){(void)p;return held;}
struct pc_menu_setting {char const *setting;short loaded_index,value_count;char const *values[2];};
struct widget_instance {struct {struct {short selected_index;} list;} parameters;};
static short setting_value_index(struct pc_menu_setting const *s,char const *v){int i;for(i=0;i<s->value_count;i++)if(!strcmp(s->values[i],v))return i;return NONE;}
static int setting_write(char const *s,char const *v){return config_write(s,v);}
#define platform_log(...) ((void)0)
''' + enums + "\n" + defines + "\n" + policy + setter + menu_functions + r'''
#define CHECK(c) do{if(!(c))return __LINE__;}while(0)
static void press(unsigned long key){held=0;third_person_input_update();held=key;third_person_input_update();}
int main(void){int i,p,saved;struct network_game remote;
 struct pc_menu_setting setting={"display.third_person",0,2,{"false","true"}};
 struct widget_instance spinner={{{0}}};
 CHECK(third_person_host_policy()==0);
 host_setting="forced_on";CHECK(third_person_host_policy()==1);
 host_setting="forced_off";CHECK(third_person_host_policy()==2);
 host_setting="bad";CHECK(third_person_host_policy()==0);
 capability=0;current=&remote;preference=1;CHECK(third_person_policy()==2&&!third_person_enabled());
 CHECK(!third_person_match_flags_valid(8)&&third_person_match_flags_valid(7));capability=1;current=NULL;
 for(i=0;i<256;i++)CHECK(!!third_person_match_flags_valid(i)==(i<=23));
 CHECK(!third_person_match_flags_valid(256));
 for(p=0;p<3;p++)for(i=0;i<2;i++){
  remote.cooperative_flags=(p<<THIRD_PERSON_POLICY_SHIFT)|1;current=&remote;
  preference=i;host_setting="forced_off";CHECK(third_person_policy()==p);
  CHECK(!!third_person_enabled()==(p==1||(p==0&&i)));
  saved=writes;press(1UL<<HALO_KEYBOARD_TOGGLE_PERSPECTIVE);
  CHECK(preference==(p==0?!i:i)&&writes==saved+(p==0));
  third_person_input_update();CHECK(writes==saved+(p==0)); /* held, not another press */
  saved=shoulder;press(1UL<<HALO_KEYBOARD_SWAP_SHOULDER);
  CHECK(shoulder==(third_person_enabled()?!saved:saved));
  current=NULL;CHECK(third_person_policy()==0&&third_person_enabled()==preference);
 }
 server_pointer=&host;host.game.cooperative_flags=3;host.state=0;
 network_game_server_port_set_camera_policy(1);CHECK(host.game.cooperative_flags==11&&sends==1);
 network_game_server_port_set_camera_policy(2);CHECK(host.game.cooperative_flags==19&&sends==2);
 network_game_server_port_set_camera_policy(-1);network_game_server_port_set_camera_policy(3);
 CHECK(host.game.cooperative_flags==19&&sends==2);
 host.state=1;network_game_server_port_set_camera_policy(0);CHECK(host.game.cooperative_flags==19&&sends==2);
 host.state=0;network_game_server_port_set_camera_policy(0);CHECK(host.game.cooperative_flags==3&&sends==3);
 server_pointer=NULL;network_game_server_port_set_camera_policy(1);CHECK(sends==3);
 /* A lobby unlock just before OK must not save the forced display value. */
 current=&remote;remote.cooperative_flags=8;preference=0;saved=writes;
 camera_setting_show(&spinner,&setting);CHECK(spinner.parameters.list.selected_index==1&&setting.loaded_index==NONE);
 setting_changed_save(&spinner,&setting);CHECK(writes==saved&&preference==0);
 remote.cooperative_flags=0;setting_changed_save(&spinner,&setting);CHECK(writes==saved&&preference==0);
 camera_setting_show(&spinner,&setting);CHECK(spinner.parameters.list.selected_index==0&&setting.loaded_index==0);
 spinner.parameters.list.selected_index=1;setting_changed_save(&spinner,&setting);CHECK(preference==1&&writes==saved+1);
 return 0;
}
'''
    source = tmp_path / "policy.c"
    source.write_text(stub)
    exe = tmp_path / ("policy.exe" if sys.platform == "win32" else "policy")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else ["-lm"]
    result = subprocess.run([compiler, str(source), *flags, "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(exe)], timeout=10)
    assert (result.returncode != 0) == bool(fault), result.returncode

def test_camera_policy_integration():
    from harness import function
    server = (ROOT / "source/networking/network_server_manager.c").read_text()
    assert "third_person_host_policy() << THIRD_PERSON_POLICY_SHIFT" in server
    client = (ROOT / "source/networking/network_client_manager.c").read_text()
    assert "third_person_match_flags_valid(message_packet->cooperative_flags)" in function(client, "network_game_client_game_settings_updated")
    menus = (ROOT / "port/linux/game/menu_functions.c").read_text()
    for name in ("setting_write", "setting_save"):
        assert '"display.third_person"' in function(menus, name)
        assert "third_person_policy() != _third_person_player_choice" in function(menus, name)
    assert "third_person_input_update();" in (ROOT / "source/main/main.c").read_text()
    assert 'config_write_boolean("display.third_person"' not in (ROOT / "port/linux/src/xinput_sdl.c").read_text()

def test_host_camera_menu_layout():
    import xml.etree.ElementTree as ET
    import port_settings
    root = ET.fromstring("\n".join(port_settings.multiplayer_files()["main_menu.multiplayer_type_select.server_settings.xml"]))
    base = "main_menu/multiplayer_type_select/server_settings/"
    menu = root.find(f"widget[@name='{base}options_menu']")
    rows = {node.attrib["widget"].removeprefix(base): int(node.attrib["y"]) for node in menu.findall("child")}
    common = ["op_server_name", "op_max_players", "op_invite", "op_listing", "op_password", "op_session_compatibility", "op_camera_policy", "op_map_share_options"]
    for mode in (["op_coop_options", "op_voice_options"], ["op_game_type", "op_player_options", "op_item_options", "op_vehicle_options", "op_indicator_options", "op_team_options"]):
        positions = [rows[name] for name in common + mode]
        assert len(set(positions)) == len(positions)
        assert max(positions) + 28 <= int(root.find(f"widget[@name='{base}help']").attrib["top"])
        assert all(b - a >= 28 for a, b in zip(sorted(positions), sorted(positions)[1:]))
        assert all(int(root.find(f"widget[@name='{base}{name}']").attrib["height"]) >= 28 for name in common + mode)
    spinner = root.find(f"widget[@name='{base}camera_policy_spinner']")
    assert spinner.attrib["strings"] == "PLAYER CHOICE|FORCED ON|FORCED OFF"
    compatibility = root.find(f"widget[@name='{base}session_compatibility_spinner']")
    assert compatibility.attrib["strings"] == "OPENCE CROSSPLAY|GULCHCE THIRD PERSON"
    assert compatibility.find('on[@event="left_mouse"]').attrib["run"] == "mouse spinner 1wide click"
    # Camera help adds three entries before map sharing and publisher help.
    import re
    menus = (ROOT / "port/linux/game/menu_functions.c").read_text()
    stored = ET.parse(ROOT / "port/assets/menus/ce/main_menu.multiplayer_type_select.server_settings.xml").getroot()
    for document in (root, stored):
        help_text = [entry.attrib["text"] for entry in
                     document.find(f"strings[@name='{base}help_strings']")]
        for name, phrase in (("MAP_SHARE_OPTIONS_HELP", "Custom Edition map"),
                             ("SERVER_LISTING_PUBLISHER_HELP", "publisher key")):
            index = int(re.search(r"#define " + name + r" (\d+)", menus).group(1))
            assert phrase in help_text[index]
    spec = port_settings.SCREENS["video_settings/fov_viewmodels"]
    assert 1 + len(spec["rows"]) + len(spec["categories"]) == 10
    assert spec["extra_help"][0].startswith("CONTROLLED BY HOST")
