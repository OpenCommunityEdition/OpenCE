"""Host camera policy, using the actual policy and pregame setter sources."""
import re
import shutil
import subprocess
import sys
from pathlib import Path
import pytest

ROOT = Path(__file__).resolve().parents[1]

@pytest.mark.parametrize("fault", [None, "local_override", "locked_toggle", "invalid_flags", "ingame_change", "riding_overlap"])
def test_camera_policy(tmp_path, fault):
    from harness import function
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    header = (ROOT / "port/linux/game/third_person.h").read_text()
    pad_header = (ROOT / "port/linux/include/controller_remap.h").read_text()
    pad_enums = re.search(r"enum controller_remap_action\s*\{.*?\};", pad_header, re.S).group(0)
    enums = re.search(r"enum third_person_match_policy\s*\{.*?\};", header, re.S).group(0)
    defines = "\n".join(re.findall(r"^#define THIRD_PERSON_POLICY_.*", header, re.M))
    network_header = (ROOT / "source/networking/network_game_manager.h").read_text()
    coop_enums = re.search(r"enum\s*\{[^}]*_network_game_cooperative_vehicle_riding_bit[^}]*\};", network_header, re.S).group(0)
    if fault == "riding_overlap":
        defines = defines.replace("POLICY_SHIFT 4", "POLICY_SHIFT 3").replace("POLICY_MASK 48", "POLICY_MASK 24")
    policy = re.sub(r"^#include[^\n]*", "", (ROOT / "port/linux/game/third_person_policy.c").read_text(), flags=re.M)
    policy += function((ROOT / "port/linux/game/third_person_input.c").read_text(), "third_person_input_update")
    menus = (ROOT / "port/linux/game/menu_functions.c").read_text()
    menu_functions = function(menus, "camera_setting_show") + function(menus, "setting_changed_save")
    if fault == "local_override":
        policy = policy.replace("policy == _third_person_player_choice && config_boolean", "config_boolean")
    elif fault == "locked_toggle":
        policy = policy.replace("third_person_policy() == _third_person_player_choice)", "1)")
    elif fault == "invalid_flags":
        policy = policy.replace("<= _third_person_forced_off;", "<= 3;")
    elif fault == "ingame_change":
        policy = policy.replace("network_game_server_get_state(server, NULL) != 0 ||", "")
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
static int network_game_is_active(void){return current!=NULL;}
static int main_menu_is_active(void){return current==NULL;}
static void main_goto_main_menu(void){/* No menu transition for camera settings. */}
static int network_game_is_splitscreen_local(void){return 0;}
static unsigned long held;
static struct network_game *network_game_get_game(void){return current;}
static struct network_game_server *global_network_game_server_get(void){return server_pointer;}
static int network_game_server_get_state(struct network_game_server *s,short *substate){(void)substate;return s->state;}
static struct network_game *network_game_server_get_game(struct network_game_server *s){return &s->game;}
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
unsigned int controller_remap_pressed(short port, unsigned int mask){(void)port;(void)mask;return 0;}
''' + pad_enums + enums + coop_enums + "\n" + defines + "\n" + policy + menu_functions + r'''
#define CHECK(c) do{if(!(c))return __LINE__;}while(0)
static void press(unsigned long key){held=0;third_person_input_update();held=key;third_person_input_update();}
int main(void){int i,p,saved;struct network_game remote;
 struct pc_menu_setting setting={"display.third_person",0,2,{"false","true"}};
 struct widget_instance spinner={{{0}}};
 CHECK(!(THIRD_PERSON_POLICY_MASK & (1UL<<_network_game_cooperative_vehicle_riding_bit)));
 CHECK(third_person_host_policy()==0);
 host_setting="forced_on";CHECK(third_person_host_policy()==0);
 host_setting="forced_off";CHECK(third_person_host_policy()==2);
 host_setting="bad";CHECK(third_person_host_policy()==0);
 for(i=0;i<256;i++)CHECK(!!third_person_match_flags_valid(i)==(i<=47));
 CHECK(!third_person_match_flags_valid(256));
 for(p=0;p<3;p++)for(i=0;i<2;i++){
  remote.cooperative_flags=(p<<THIRD_PERSON_POLICY_SHIFT)|15;current=&remote;
  preference=i;host_setting="forced_off";CHECK(third_person_policy()==p);
  CHECK(!!third_person_enabled()==(p==1||(p==0&&i)));
  saved=writes;press(1UL<<HALO_KEYBOARD_TOGGLE_PERSPECTIVE);
  CHECK(preference==(p==0?!i:i)&&writes==saved+(p==0));
  third_person_input_update();CHECK(writes==saved+(p==0)); /* held, not another press */
  saved=shoulder;press(1UL<<HALO_KEYBOARD_SWAP_SHOULDER);
  CHECK(shoulder==(third_person_enabled()?!saved:saved));
  current=NULL;CHECK(third_person_policy()==0&&third_person_enabled()==preference);
 }
 server_pointer=&host;host.game.cooperative_flags=11;host.state=0;
 third_person_set_match_policy(1);CHECK(host.game.cooperative_flags==11&&sends==0);
 third_person_set_match_policy(2);CHECK(host.game.cooperative_flags==43&&sends==1);
 third_person_set_match_policy(-1);third_person_set_match_policy(3);
 CHECK(host.game.cooperative_flags==43&&sends==1);
 host.state=1;third_person_set_match_policy(0);CHECK(host.game.cooperative_flags==43&&sends==1);
 host.state=0;third_person_set_match_policy(0);CHECK(host.game.cooperative_flags==11&&sends==2);
 server_pointer=NULL;third_person_set_match_policy(1);CHECK(sends==2);
 /* A lobby unlock just before OK must not save the forced display value. */
 current=&remote;remote.cooperative_flags=16;preference=0;saved=writes;
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


def test_host_camera_selector(tmp_path):
    from harness import function
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    menus = (ROOT / "port/linux/game/menu_functions.c").read_text()
    actual = function(menus, "session_options_initialize") + function(menus, "session_options_update")
    source = tmp_path / "selector.c"
    source.write_text(r'''
#include <stdio.h>
#include <string.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define TEXT_FIELD_LENGTH 256
#define PIN(v,a,b) ((v)<(a)?(a):(v)>(b)?(b):(v))
enum {_third_person_player_choice=0,_third_person_forced_off=2,_multiplayer_mode_host_internet=1};
struct widget_instance {
 struct {struct {short selected_index;struct widget_instance *extended_description;} list;
 struct {short string_list_index;} text_box;} parameters;
 struct widget_instance *focused_child;
};
static struct widget_instance spinner,help,invite;
static struct {int mode;} multiplayer;
static int policy,writes,locked;
static char saved[32];
static struct widget_instance *named(struct widget_instance *w,char const *n,int i){
 (void)w;(void)i;return !strcmp(n,"camera_policy_spinner")?&spinner:&invite;
}
static short third_person_policy(void){return policy;}
static void third_person_set_match_policy(short p){if(!locked)policy=p;}
static int config_write(char const *k,char const *v){(void)k;strcpy(saved,v);writes++;return 1;}
static int config_boolean(char const *k){(void)k;return 0;}
static int p2p_invite_link(char *s,int n){(void)s;(void)n;return 0;}
static void text_field_show(struct widget_instance *w,char *s,int b){(void)w;(void)s;(void)b;}
''' + actual + r'''
#define CHECK(c) do{if(!(c))return __LINE__;}while(0)
int main(void){struct widget_instance list={0};list.parameters.list.extended_description=&help;
 CHECK(session_options_initialize(&list));CHECK(spinner.parameters.list.selected_index==0);
 spinner.parameters.list.selected_index=1;CHECK(session_options_update(&list));
 CHECK(policy==2&&writes==1&&!strcmp(saved,"forced_off")&&help.parameters.text_box.string_list_index==1);
 CHECK(session_options_initialize(&list));CHECK(spinner.parameters.list.selected_index==1);
 locked=1;spinner.parameters.list.selected_index=0;session_options_update(&list);
 CHECK(policy==2&&writes==1&&spinner.parameters.list.selected_index==1);
 locked=0;spinner.parameters.list.selected_index=0;session_options_update(&list);
 CHECK(policy==0&&writes==2&&!strcmp(saved,"player_choice")&&help.parameters.text_box.string_list_index==0);
 list.focused_child=&invite;session_options_update(&list);CHECK(help.parameters.text_box.string_list_index==2);
 return 0;
}
''')
    exe = tmp_path / ("selector.exe" if sys.platform == "win32" else "selector")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else []
    subprocess.run([compiler, str(source), *flags, "-o", str(exe)], check=True, capture_output=True)
    subprocess.run([str(exe)], check=True, timeout=10)

def test_host_camera_menu_layout():
    import xml.etree.ElementTree as ET
    import port_settings
    root = ET.fromstring("\n".join(port_settings.multiplayer_files()["main_menu.multiplayer_type_select.server_settings.xml"]))
    base = "main_menu/multiplayer_type_select/server_settings/"
    menu = root.find(f"widget[@name='{base}options_menu']")
    rows = {node.attrib["widget"].removeprefix(base): int(node.attrib["y"]) for node in menu.findall("child")}
    help_box = root.find(f"widget[@name='{base}help']")
    assert int(help_box.attrib["left"]) == 54 + 13
    assert help_box.attrib["font"] == "ui\\small_ui"
    assert int(help_box.attrib["height"]) >= 40
    assert min(rows.values()) >= 49
    assert int(help_box.attrib["top"]) + int(help_box.attrib["height"]) <= rows["button_bar"]
    common = ["op_server_name", "op_max_players", "op_listing", "op_password", "op_session_options", "op_map_share_options"]
    for mode in (["op_coop_options", "op_voice_options"], ["op_game_type", "op_player_options", "op_item_options", "op_vehicle_options", "op_indicator_options", "op_team_options"]):
        positions = [rows[name] for name in common + mode]
        assert len(set(positions)) == len(positions)
        assert max(positions) + 28 <= int(root.find(f"widget[@name='{base}help']").attrib["top"])
        assert all(b - a >= 28 for a, b in zip(sorted(positions), sorted(positions)[1:]))
        assert all(int(root.find(f"widget[@name='{base}{name}']").attrib["height"]) >= 28 for name in common + mode)
    assert root.find(f"widget[@name='{base}op_session_options']/on[@event='a']").attrib["open"] == base + "session_options/session_options_screen"
    sub = base + "session_options/"
    assert "op_invite" not in rows
    assert root.find(f"widget[@name='{sub}options_menu']/child[@widget='{sub}op_invite']") is not None
    assert root.find(f"widget[@name='{sub}session_compatibility_spinner']") is None
    for key in ("camera_policy",):
        row = root.find(f"widget[@name='{sub}op_{key}']")
        label = root.find(f"widget[@name='{sub}{key}_label']")
        spinner = root.find(f"widget[@name='{sub}{key}_spinner']")
        child = row.find(f"child[@widget='{sub}{key}_spinner']")
        assert row.attrib["width"] == "512" and row.attrib["height"] == "28"
        assert label.attrib["font"] == spinner.attrib["font"] == "ui\\large_ui"
        assert int(child.attrib["y"]) + int(spinner.attrib["top"]) == int(label.attrib["text_y"])
        assert spinner.attrib["align"] == "center"
        for arrow in ("header_bounds", "footer_bounds"):
            top, left, bottom, right = map(int, spinner.attrib[arrow].split())
            assert 0 <= left < right <= int(spinner.attrib["width"])
            assert 0 <= top < bottom <= int(spinner.attrib["height"])
        assert spinner.find('on[@event="left_mouse"]').attrib["run"] == "mouse spinner 1wide click"
    assert root.find(f"widget[@name='{sub}camera_policy_spinner']").attrib["strings"] == "PLAYER CHOICE|FORCED OFF"
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


def test_host_help_is_generated():
    import xml.etree.ElementTree as ET
    import port_settings
    name = "main_menu.settings_select.player_setup.player_profile_edit.video_settings.fov_viewmodels.xml"
    root = ET.fromstring("\n".join(port_settings.settings_files()[name]))
    strings = next(node for node in root.findall("strings") if node.attrib["name"].endswith("/help_strings"))
    assert strings[10].attrib["text"].startswith("CONTROLLED BY HOST")

def test_host_hidden_rows_reflow(tmp_path):
    from harness import function
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    layout = function((ROOT / "port/linux/game/menu_functions.c").read_text(), "server_settings_layout")
    source = tmp_path / "layout.c"
    source.write_text('''#include <string.h>
struct widget_instance { char *name; int visible; short vertical_offset; struct widget_instance *child, *next; };
''' + layout + '''
int main(void) {
    struct widget_instance rows[13], list;
    int mask, i, y;
    memset(rows, 0, sizeof(rows)); memset(&list, 0, sizeof(list));
    list.child = rows;
    for (i = 0; i < 13; i++) {
        rows[i].name = i == 12 ? "button_bar" : "op_setting";
        rows[i].next = i == 12 ? 0 : &rows[i + 1];
    }
    rows[12].vertical_offset = 448; rows[12].visible = 1;
    /* Repeated updates also restore positions when rows become visible. */
    for (mask = 0; mask < 4096; mask++) {
        for (i = 0; i < 12; i++) rows[i].visible = !!(mask & (1 << i));
        server_settings_layout(&list); y = 49;
        for (i = 0; i < 12; i++) if (rows[i].visible) {
            if (rows[i].vertical_offset != y || y + 28 > 392) return 1;
            y += 28;
        }
        if (rows[12].vertical_offset != 448) return 2;
    }
    return 0;
}
''')
    exe = tmp_path / ("layout.exe" if sys.platform == "win32" else "layout")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else []
    result = subprocess.run([compiler, str(source), *flags, "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(exe)], check=True, timeout=10)
