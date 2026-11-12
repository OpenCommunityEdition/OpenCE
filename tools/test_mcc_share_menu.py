"""Run the real download-management menu with observable storage/session boundaries."""
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

import pytest

from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"


@pytest.mark.parametrize("platform", ["desktop", "android", "web"])
@pytest.mark.parametrize("in_game", [False, True])
def test_stock_network_menu_reaches_sharing(platform, in_game):
    """Traverse the shipped Stock widgets without loading optional frontend tags."""
    base = "main_menu/settings_select/player_setup/player_profile_edit/network_setup/"
    widgets = {w.get("name"): w for file in (ROOT / "port/assets/menus/ce").glob("*.xml")
               for w in ET.parse(file).getroot().findall("widget")}
    pending = [base + ("in_game/in_game_network_settings_screen" if in_game else "network_settings_screen")]
    reached = set()
    visible = lambda node: platform in node.get("platform", "desktop android web").split()
    while pending:
        name = pending.pop()
        if name in reached or name not in widgets or not visible(widgets[name]):
            continue
        reached.add(name)
        for node in widgets[name]:
            if visible(node):
                target = node.get("widget") if node.tag == "child" else node.get("open")
                if target:
                    pending.append(target)
    settings = {widgets[name].get("setting") for name in reached}
    assert {"network.map_download", "network.map_seed",
            "game.mcc_campaigns", "game.mcc_collections"} <= settings
    assert ("game.workshop_maps" in settings) == (platform == "desktop")
    assert base + "mcc_downloads/screen" in reached
    assert base + "mcc_downloads/remove" in reached


def test_generated_remove_button_has_one_text_owner():
    import port_settings

    base = port_settings.PE + "/network_setup/mcc_downloads"
    name = base.replace("/", ".") + ".xml"
    generated = "\n".join(port_settings.settings_files()[name])
    assert (ROOT / "port/assets/menus/ce" / name).read_text(encoding="utf-8") == generated
    document = ET.fromstring(generated)
    remove = document.find(f"widget[@name='{base}/remove']")
    assert remove is not None
    # widget_instance_render_text_box replaces and resizes dynamic text when a
    # definition has a string-list binding. The screen's live update owns this
    # button's text buffer instead, for both removal and confirmation labels.
    assert not {"text", "strings", "string_list"}.intersection(remove.attrib)
    screen = document.find(f"widget[@name='{base}/screen']")
    assert screen.find("data[@input='port mcc downloads update']") is not None
    for event in ("a", "start"):
        assert remove.find(f"on[@event='{event}'][@run='port mcc downloads remove']") is not None
    for button, caption in (("previous", "PREVIOUS"), ("next", "NEXT"), ("back", "BACK")):
        assert document.find(f"widget[@name='{base}/{button}']").get("text") == caption

HEADERS = {
    "cseries.h": "#include <stddef.h>\n#include <wchar.h>\ntypedef int boolean;\n#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))\n",
    "game/game.h": "int game_in_progress(void);\nint game_time_menu_scene(void);\nint game_time_get_paused(void);\n",
    "networking/network_game_globals.h": """struct network_game_client;
struct network_game_server;
struct network_game_client *global_network_game_client_get(void);
struct network_game_server *global_network_game_server_get(void);
""",
}

DRIVER = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "mcc_share_menu.h"
#include "mcc_share_storage.h"
#define CHECK(c) do { if(!(c)){fprintf(stderr,"line %d: %s\n",__LINE__,#c);exit(1);} } while(0)
static char const *native_maps="native-root/mcc_maps";
static struct mcc_share_storage_item records[2];
static int record_count=2,exists=1,list_error,list_truncated;
static int initialized,stat_calls,list_calls,remove_calls,delete_calls,rescans;
static int playing,menu_scene,paused,client,server;
static enum mcc_share_storage_result removal=MCC_SHARE_STORAGE_REMOVED;
static char removed[64];

int game_in_progress(void){return playing;}
int game_time_menu_scene(void){return menu_scene;}
int game_time_get_paused(void){return paused;}
struct network_game_client *global_network_game_client_get(void){return client ? (void*)&client : NULL;}
struct network_game_server *global_network_game_server_get(void){return server ? (void*)&server : NULL;}
void mcc_maps_rescan(void){rescans++;}
void platform_translate_path(char const *path,char *out,unsigned long size){
 CHECK(!strcmp(path,"d:\\mcc_maps"));CHECK(strlen(native_maps)<size);strcpy(out,native_maps);
}
static int native_exists(char const *path){
 CHECK(initialized);CHECK(!strcmp(path,native_maps));stat_calls++;return exists;
}
static int native_delete(char const *path){
 CHECK(initialized);CHECK(!strncmp(path,native_maps,strlen(native_maps)));
 CHECK(path[strlen(native_maps)]=='/');delete_calls++;return 1;
}
void mcc_share_storage_native_io(struct mcc_share_io *io){
 initialized++;memset(io,0,sizeof(*io));io->directory_exists=native_exists;io->delete_file=native_delete;
}
int mcc_share_storage_list(char const *maps,struct mcc_share_io const *io,
 struct mcc_share_storage_item *items,int capacity,int *truncated){
 CHECK(!strcmp(maps,native_maps));CHECK(io->directory_exists==native_exists);
 CHECK(io->delete_file==native_delete);CHECK(capacity>=record_count);list_calls++;
 *truncated=list_truncated;
 if(list_error)return -1;
 memcpy(items,records,(size_t)record_count*sizeof(*items));return record_count;
}
enum mcc_share_storage_result mcc_share_storage_remove(char const *maps,
 struct mcc_share_io const *io,char const *folder){
 char path[160];int i;
 CHECK(!strcmp(maps,native_maps));CHECK(io->directory_exists==native_exists);
 CHECK(io->delete_file==native_delete);remove_calls++;strcpy(removed,folder);
 if(removal==MCC_SHARE_STORAGE_ERROR)return removal;
 snprintf(path,sizeof(path),"%s/%s/managed.map",maps,folder);CHECK(io->delete_file(path));
 for(i=0;i<record_count;i++)if(!strcmp(records[i].folder,folder)){
  memmove(records+i,records+i+1,(size_t)(record_count-i-1)*sizeof(records[0]));record_count--;return removal;
 }
 CHECK(0);return MCC_SHARE_STORAGE_ERROR;
}
static void text(char out[512]){
 wchar_t wide[512];unsigned i;
 mcc_share_menu_text(wide);
 for(i=0;i<512 && wide[i];i++){CHECK((unsigned)wide[i]<128);out[i]=(char)wide[i];}
 CHECK(i<512);out[i]=0;
}
int main(int argc,char **argv){
 char shown[512];char const *mode;int *active=NULL;
 CHECK(argc==2);mode=argv[1];
 strcpy(records[0].folder,"dl_first");strcpy(records[0].source,"mcc_maps\\host\\first");
 strcpy(records[0].maker,"Maker %s <tag>|name");strcpy(records[0].workshop.id,"12345");
 records[0].bytes=3u*1024u*1024u/2u;records[0].files=7;
 strcpy(records[1].folder,"dl_second");strcpy(records[1].source,"mcc_maps\\host\\second");
 strcpy(records[1].maker,"Second maker");records[1].bytes=2u*1024u*1024u;records[1].files=3;
 if(!strcmp(mode,"missing"))exists=0;
 if(!strcmp(mode,"list_error"))list_error=1;
 if(!strcmp(mode,"empty"))record_count=0;
 if(!strcmp(mode,"text"))list_truncated=1;
 if(!strncmp(mode,"ui_",3)){playing=1;menu_scene=1;}
 mcc_share_menu_open();CHECK(initialized==1 && stat_calls==1);
 if(!strcmp(mode,"missing")||!strcmp(mode,"empty")||!strcmp(mode,"list_error")){
  CHECK(!mcc_share_menu_can_remove());mcc_share_menu_remove();mcc_share_menu_remove();mcc_share_menu_step(1);
  CHECK(!mcc_share_menu_confirming() && !remove_calls && !delete_calls && !rescans);text(shown);
  CHECK(strstr(shown,list_error?"could not be read":"No managed MCC downloads"));
  CHECK(list_calls==(exists?1:0));return 0;
 }
 CHECK(list_calls==1 && mcc_share_menu_can_remove());
 if(!strcmp(mode,"text")){
  text(shown);CHECK(strstr(shown,"1 of 2 (more packages exist)"));
  CHECK(strstr(shown,"mcc_maps\\host\\first"));CHECK(strstr(shown,"Maker: Maker %s <tag>/name"));
  CHECK(strstr(shown,"1.5 MiB, 7 files"));CHECK(strstr(shown,"Workshop: 12345"));CHECK(!strchr(shown,'|'));
  mcc_share_menu_step(-1);text(shown);CHECK(strstr(shown,"2 of 2"));CHECK(strstr(shown,"2.0 MiB, 3 files"));
  CHECK(!strstr(shown,"Workshop:"));CHECK(!remove_calls);return 0;
 }
 mcc_share_menu_remove();CHECK(mcc_share_menu_confirming());
 CHECK(!remove_calls && !delete_calls && !rescans);text(shown);CHECK(strstr(shown,"CONFIRM REMOVAL"));
 if(!strcmp(mode,"selection")){
  mcc_share_menu_step(1);CHECK(!mcc_share_menu_confirming());
  mcc_share_menu_remove();CHECK(mcc_share_menu_confirming() && !remove_calls);
  mcc_share_menu_remove();CHECK(remove_calls==1 && delete_calls==1 && !strcmp(removed,"dl_second"));
  CHECK(rescans==1 && !mcc_share_menu_confirming());text(shown);CHECK(strstr(shown,"host\\first"));
  mcc_share_menu_remove();CHECK(remove_calls==1 && mcc_share_menu_confirming());
  mcc_share_menu_open();CHECK(initialized==2 && !mcc_share_menu_confirming());return 0;
 }
 if(!strcmp(mode,"playing"))active=&playing;
 if(!strcmp(mode,"paused")){active=&playing;paused=1;CHECK(game_time_get_paused());}
 if(!strcmp(mode,"client"))active=&client;
 if(!strcmp(mode,"server"))active=&server;
 if(!strcmp(mode,"ui_client"))active=&client;
 if(!strcmp(mode,"ui_server"))active=&server;
 if(!strcmp(mode,"ui_to_game")){
  menu_scene=0;CHECK(playing && !mcc_share_menu_can_remove());
  mcc_share_menu_remove();CHECK(!mcc_share_menu_confirming() && !remove_calls && !delete_calls && !rescans);
  menu_scene=1;mcc_share_menu_remove();CHECK(mcc_share_menu_confirming() && !remove_calls);
  mcc_share_menu_remove();CHECK(remove_calls==1 && !strcmp(removed,"dl_first"));return 0;
 }
 if(active){
  *active=1;CHECK(!mcc_share_menu_can_remove());text(shown);CHECK(strstr(shown,"Leave the game/session"));
  mcc_share_menu_remove();CHECK(!mcc_share_menu_confirming() && !remove_calls && !delete_calls && !rescans);
  *active=0;mcc_share_menu_remove();CHECK(mcc_share_menu_confirming() && !remove_calls);
  mcc_share_menu_remove();CHECK(remove_calls==1 && !strcmp(removed,"dl_first"));return 0;
 }
 if(!strcmp(mode,"leftovers"))removal=MCC_SHARE_STORAGE_LEFTOVERS;
 if(!strcmp(mode,"remove_error"))removal=MCC_SHARE_STORAGE_ERROR;
 mcc_share_menu_remove();CHECK(remove_calls==1 && !strcmp(removed,"dl_first"));
 CHECK(rescans==1 && list_calls==2 && !mcc_share_menu_confirming());text(shown);
 CHECK(strstr(shown,removal==MCC_SHARE_STORAGE_REMOVED?"Downloaded snapshot removed.":
  removal==MCC_SHARE_STORAGE_LEFTOVERS?"Files you added were kept.":"Removal stopped: content changed"));
 CHECK(delete_calls==(removal!=MCC_SHARE_STORAGE_ERROR));
 CHECK(strstr(shown,removal==MCC_SHARE_STORAGE_ERROR?"host\\first":"host\\second"));return 0;
}
'''


def build(work, implementation=None):
    for name, text in HEADERS.items():
        path = work / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    source = work / "driver.c"
    source.write_text(DRIVER)
    return compile_tool(work, [source, implementation or GAME / "mcc_share_menu.c"])


@pytest.fixture(scope="module")
def menu_tool(tmp_path_factory):
    return build(tmp_path_factory.mktemp("mcc-share-menu"))


@pytest.mark.parametrize("case", ["selection", "playing", "paused", "client", "server",
                                  "ui_removed", "ui_client", "ui_server", "ui_to_game", "text", "missing",
                                  "empty", "list_error", "removed", "leftovers", "remove_error"])
def test_download_management(menu_tool, case):
    result = subprocess.run([str(menu_tool), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("before,after,case", [
    ("if (!confirming) { confirming=1; return; }", "", "selection"),
    ("if (!mcc_share_menu_can_remove()) { confirming=0; return; }", "", "playing"),
    ("(!game_in_progress() || game_time_menu_scene())", "!game_in_progress()", "ui_removed"),
    ("(!game_in_progress() || game_time_menu_scene())", "(!game_in_progress() || game_time_get_paused() || game_time_menu_scene())", "paused"),
])
def test_confirmation_and_session_guards_detect_removal_regressions(tmp_path, before, after, case):
    original = (GAME / "mcc_share_menu.c").read_text()
    assert original.count(before) == 1
    # Relocate the relative platform include only; compile the real menu body.
    changed = tmp_path / "mcc_share_menu.c"
    changed.write_text(original.replace(before, after).replace('"../src/posix.h"',
                       f'"{(ROOT / "port/linux/src/posix.h").as_posix()}"'))
    executable = build(tmp_path, changed)
    result = subprocess.run([str(executable), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 1, result.stderr


@pytest.mark.parametrize("platform", ["desktop", "android", "web"])
def test_prompt_focus_tracks_visible_actions(tmp_path, platform):
    from harness import function

    source = (GAME / "menu_functions.c").read_text()
    start = source.index('else if (!strcmp(name,"port mcc share update")) {')
    end = source.index('\n\telse if', start)
    body = source[start:end].replace('else if', 'if', 1)
    driver = r'''
#include <assert.h>
#include <string.h>
typedef int boolean;
#define FALSE 0
#define NONE -1
struct widget_instance {
    struct widget_instance *child,*next,*focused_child;
    int visible;
    struct {struct {int selected_index;} list;} parameters;
};
static struct widget_instance children[6];
static int download_on,workshop_on;
static int mcc_share_game_can_download(void) {return download_on;}
static int mcc_share_game_has_workshop(void) {return workshop_on;}
static struct widget_instance *named(struct widget_instance *w,char const *name,int unused) {
    (void)w;(void)unused;
    return children+(!strcmp(name,"download")?1:!strcmp(name,"workshop")?2:3);
}
''' + function(source, "focus_row") + '\nstatic void update(struct widget_instance *widget) {char const *name="port mcc share update";\n' + body + r'''
}
int main(void) {
    struct widget_instance screen;
    int d,w,i,expected;
    memset(&screen,0,sizeof(screen));screen.child=children;
    for(i=0;i<6;i++){children[i].visible=1;children[i].next=i<5?children+i+1:0;}
    for(d=0;d<2;d++)for(w=0;w<2;w++) {
        download_on=d;workshop_on=w;
        screen.focused_child=children+1;screen.parameters.list.selected_index=1;
        expected=d?1:w?2:4;
#if defined(HALO_ANDROID) || defined(HALO_WEB)
        if(!d&&w)expected=3;
#endif
        update(&screen);
        assert(screen.focused_child==children+expected);
        assert(screen.parameters.list.selected_index==expected&&screen.focused_child->visible);
        if(w) {
            screen.focused_child=children+3;screen.parameters.list.selected_index=3;
            update(&screen);assert(screen.focused_child==children+3);
            workshop_on=0;update(&screen);assert(screen.focused_child==children+(d?1:4));
        }
    }
    return 0;
}
'''
    prefix = "" if platform == "desktop" else "#define HALO_" + platform.upper() + " 1\n"
    path = tmp_path / "prompt.c"
    path.write_text(prefix + driver)
    exe = compile_tool(tmp_path, [path])
    subprocess.run([str(exe)], check=True, capture_output=True)
