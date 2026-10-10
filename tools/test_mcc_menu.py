"""Run the real MCC menu dispatch against narrow widget/catalog stubs.

The mixed catalog deliberately gives filtered rows different absolute indices.
No game assets or rendering are required to check navigation and launch routes.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def menu_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed for MCC menu tests")
    implementation = (ROOT / "port/linux/game/menu_functions.c").read_text()
    kinds = re.search(r"enum\s*\{\s*MAP_KIND_SINGLEPLAYER,.*?\};", implementation, re.S).group()
    states = "\n".join(re.search(r"static struct\s*\{[^}]*\}\s*" + name + r";",
                                 implementation, re.S).group()
                       for name in ["mcc_solo_list", "level_list", "map_list", "mcc_host_list"])
    names = ["map_kind_mcc", "map_kind_singleplayer", "map_kind_shown", "map_kind_first",
             "mcc_solo_choose", "mcc_host_list_choose", "level_list_update",
             "map_list_update", "map_list_back"]
    source = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define MAP_KIND_ROWS 10
#define NUMBER_OF_GAME_DIFFICULTY_LEVELS 4
#define LEVEL_UNAVAILABLE 99
#define SOUND_FORWARD 1
#define SOUND_BACK 2
#define SERVER_SETUP_NAME "server_setup"
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define csmemset memset
enum {MAP_STEP_MAPS, MAP_STEP_DIFFICULTIES};
struct player_profile {int unused;};
struct widget_instance {struct {struct {short selected_index;struct widget_instance *extended_description;} list;} parameters;};
static struct widget_instance spinner, description, list;
static struct {short shown_level,saved_level;struct {boolean available;void *finished;} levels[10];} campaign;
static int no_spinner, empty_catalog, failed, launched, deferred, cooperated, multiplayer;
static int opened, backed, last_controller, last_difficulty, last_sound, focus_first, focus_chosen;
static int solo_updated, host_updated, legacy_updated;
static char last_level[40];
''' + kinds + '\n' + states + r'''
static struct widget_instance *named(struct widget_instance *widget,char const *name,short marker) {
    (void)widget;(void)marker;
    return !strcmp(name,"list_item_0_map_kind_spinner") ? (no_spinner ? NULL : &spinner) : &description;
}
/* A sorted mixed catalog: MP, SP, MP, SP, MP. */
static short mcc_maps_type_count(boolean singleplayer) {return empty_catalog ? 0 : singleplayer ? 2 : 3;}
static short mcc_maps_type_index(short row,boolean singleplayer) {
    if (row<0 || row>=mcc_maps_type_count(singleplayer))return NONE;
    return singleplayer ? row*2+1 : row*2;
}
static char const *mcc_maps_level_name(short index) {
    static char const *names[]={"mcc_maps\\mp0","mcc_maps\\sp0","mcc_maps\\mp1","mcc_maps\\sp1","mcc_maps\\mp2"};
    return index<0 || index>=5 ? NULL : names[index];
}
static boolean mcc_maps_campaign(short index) {return index==1 || index==3;}
static boolean campaign_profile(short controller,struct player_profile *profile) {(void)profile;last_controller=controller;return TRUE;}
static boolean campaign_fail(void) {failed++;return FALSE;}
static void main_set_map_name(char const *name) {strcpy(last_level,name);}
static void main_defer_map_map_change(void) {deferred++;}
static short main_get_difficulty(void) {return 2;}
static void campaign_start(char const *name,short difficulty,short controller) {
    launched++;strcpy(last_level,name);last_difficulty=difficulty;last_controller=controller;
}
static boolean ui_widget_port_mcc_cooperative_level_choose(char const *name,short difficulty) {
    cooperated++;strcpy(last_level,name);last_difficulty=difficulty;return TRUE;
}
static boolean ui_widget_port_mcc_multiplayer_map_choose(char const *name) {
    multiplayer++;strcpy(last_level,name);return TRUE;
}
static boolean ui_widget_port_open(struct widget_instance *widget,char const *name,boolean *deleted) {
    (void)widget; if(strcmp(name,SERVER_SETUP_NAME))exit(70);opened++;*deleted=TRUE;return TRUE;
}
static void map_kind_focus(struct widget_instance *widget,short first,short chosen) {(void)widget;focus_first=first;focus_chosen=chosen;}
static void ui_play_audio_feedback_sound(short sound) {last_sound=sound;}
static void ui_widget_port_go_back(struct widget_instance *widget) {(void)widget;backed++;}
static void map_step_open(struct widget_instance *widget,short step,short entry) {
    (void)widget;map_list.step=step;map_list.chosen=entry;
}
static void mcc_solo_list_update(struct widget_instance *widget) {(void)widget;solo_updated++;}
static void mcc_host_list_update(struct widget_instance *widget) {(void)widget;host_updated++;}
static short map_kind_count(short kind,short count) {(void)kind;(void)count;return 3;}
static short map_step_count(void) {return 3;}
static void map_difficulty_text(short entry,wchar_t *text) {(void)entry;(void)text;}
static void map_level_text(short entry,wchar_t *text) {(void)entry;(void)text;}
static void level_row_text(short entry,wchar_t *text) {(void)entry;(void)text;}
static void multiplayer_map_text(short entry,wchar_t *text) {(void)entry;(void)text;}
static void custom_campaign_map_text(short entry,wchar_t *text) {(void)entry;(void)text;}
static void custom_multiplayer_map_text(short entry,wchar_t *text) {(void)entry;(void)text;}
static short map_kind_rows_update(struct widget_instance *widget,short *first,short count,void(*text)(short,wchar_t *)) {
    (void)widget;(void)first;(void)count;(void)text;legacy_updated++;return NONE;
}
static short map_kind_display_index(short kind,short entry) {(void)kind;return entry;}
static void map_description_show(struct widget_instance *widget,short level,short map) {(void)widget;(void)level;(void)map;}
static void visible_set(struct widget_instance *widget,boolean visible) {(void)widget;(void)visible;}
static void profile_name_show(struct widget_instance *widget) {(void)widget;}
static void level_description(struct widget_instance *widget,char const *prefix,short level,boolean saved,void *finished,short controller) {
    (void)widget;(void)prefix;(void)level;(void)saved;(void)finished;(void)controller;
}
''' + '\n'.join(function(implementation, name) for name in names) + r'''
#define CHECK(c,n) do {if(!(c)){fprintf(stderr,"check %d failed\n",n);return n;}} while(0)
int main(int argc,char **argv) {
    boolean deleted=FALSE;
    if(argc<2)return 99;
    list.parameters.list.extended_description=&description;
    if(!strcmp(argv[1],"kinds")) {
        int i;printf("%d\n",NUMBER_OF_MAP_KINDS);
        for(i=0;i<NUMBER_OF_MAP_KINDS;i++)printf("%d %d\n",map_kind_singleplayer(i),map_kind_mcc(i));
        return 0;
    }
    if(!strcmp(argv[1],"spinner")) {
        int shown;
        if(argc!=6)return 98;
        spinner.parameters.list.selected_index=(short)atoi(argv[4]);no_spinner=atoi(argv[5]);
        shown=map_kind_shown(&list,atoi(argv[2]),(short)atoi(argv[3]));
        printf("%d %d\n",shown,spinner.parameters.list.selected_index);return 0;
    }
    if(!strcmp(argv[1],"solo")) {
        boolean singleplayer=atoi(argv[2]);
        mcc_solo_list.campaign=singleplayer;mcc_solo_list.chosen=1;
        CHECK(mcc_solo_choose(3)==singleplayer,1);
        CHECK(!strcmp(last_level,singleplayer?"mcc_maps\\sp1":"mcc_maps\\mp1"),2);
        CHECK(last_controller==3 && !failed,3);
        CHECK(deferred==singleplayer && launched==!singleplayer,4);
        if(!singleplayer)CHECK(last_difficulty==2,5);
        return 0;
    }
    if(!strcmp(argv[1],"host")) {
        boolean singleplayer=atoi(argv[2]);
        map_list.hosting=TRUE;mcc_host_list.campaign=singleplayer;mcc_host_list.chosen=1;
        CHECK(mcc_host_list_choose(&list,&deleted)==!singleplayer,6);
        if(singleplayer) {
            CHECK(mcc_host_list.level==3 && mcc_host_list.step==MAP_STEP_DIFFICULTIES,7);
            CHECK(mcc_host_list.chosen==2 && focus_chosen==2 && !opened && !multiplayer,8);
            CHECK(mcc_host_list_choose(&list,&deleted),9);
            CHECK(cooperated==1 && opened==1 && deleted && last_difficulty==2,10);
        } else CHECK(multiplayer==1 && !cooperated && !opened && mcc_host_list.step==MAP_STEP_MAPS,11);
        CHECK(!failed && !strcmp(last_level,singleplayer?"mcc_maps\\sp1":"mcc_maps\\mp1"),12);
        return 0;
    }
    if(!strcmp(argv[1],"local-reject")) {
        mcc_host_list.campaign=TRUE;mcc_host_list.multiplayer_only=TRUE;
        CHECK(!mcc_host_list_choose(&list,&deleted) && failed==1,13);
        CHECK(!multiplayer && !cooperated && !opened && mcc_host_list.step==MAP_STEP_MAPS,14);return 0;
    }
    if(!strcmp(argv[1],"empty")) {
        empty_catalog=TRUE;mcc_solo_list.campaign=atoi(argv[2]);mcc_host_list.campaign=atoi(argv[2]);
        CHECK(!mcc_solo_choose(0) && !mcc_host_list_choose(&list,&deleted),15);
        CHECK(failed==2 && !launched && !deferred && !cooperated && !multiplayer && !opened,16);return 0;
    }
    if(!strcmp(argv[1],"back")) {
        map_list.kind=MAP_KIND_MCC_SINGLEPLAYER;mcc_host_list.campaign=TRUE;
        mcc_host_list.step=MAP_STEP_DIFFICULTIES;mcc_host_list.level=3;mcc_host_list.chosen=2;
        CHECK(map_list_back(&list,&deleted),17);
        CHECK(mcc_host_list.step==MAP_STEP_MAPS && mcc_host_list.chosen==1 && focus_chosen==1,18);
        CHECK(!deleted && !backed && last_sound==SOUND_BACK,19);
        CHECK(map_list_back(&list,&deleted) && deleted && backed==1,20);return 0;
    }
    if(!strcmp(argv[1],"switch")) {
        int host=atoi(argv[2]),target=atoi(argv[3]);
        if(host) {
            map_list.kind=target==MAP_KIND_MCC_SINGLEPLAYER ? MAP_KIND_MCC_MULTIPLAYER : MAP_KIND_MCC_SINGLEPLAYER;
            map_list.hosting=TRUE;mcc_host_list.step=MAP_STEP_DIFFICULTIES;
            mcc_host_list.first=7;mcc_host_list.chosen=9;mcc_host_list.level=3;
            spinner.parameters.list.selected_index=target;map_list_update(&list);
            CHECK(map_list.kind==target && map_list.step==MAP_STEP_MAPS,21);
            if(map_kind_mcc(target)) {
                CHECK(mcc_host_list.step==MAP_STEP_MAPS && !mcc_host_list.first && !mcc_host_list.chosen,22);
                CHECK(mcc_host_list.campaign==(target==MAP_KIND_MCC_SINGLEPLAYER),23);
                CHECK(host_updated==1 && !legacy_updated && !mcc_host_list.multiplayer_only,24);
            } else CHECK(!host_updated && legacy_updated==1,25);
        } else {
            level_list.kind=target==MAP_KIND_MCC_SINGLEPLAYER ? MAP_KIND_MCC_MULTIPLAYER : MAP_KIND_MCC_SINGLEPLAYER;
            mcc_solo_list.first=7;mcc_solo_list.chosen=9;
            spinner.parameters.list.selected_index=target;level_list_update(&list);
            CHECK(level_list.kind==target,26);
            if(map_kind_mcc(target)) {
                CHECK(!mcc_solo_list.first && !mcc_solo_list.chosen,27);
                CHECK(mcc_solo_list.campaign==(target==MAP_KIND_MCC_SINGLEPLAYER),28);
                CHECK(solo_updated==1 && !legacy_updated,29);
            } else CHECK(!solo_updated && legacy_updated==1,30);
        }
        CHECK(!failed && !launched && !deferred && !cooperated && !multiplayer,31);return 0;
    }
    return 97;
}
'''
    work = tmp_path_factory.mktemp("mcc-menu")
    path = work / "menu.c"
    path.write_text(source)
    executable = work / ("menu.exe" if os.name == "nt" else "menu")
    command = [compiler, "-std=c99", str(path), "-o", str(executable)]
    if os.name == "nt":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return executable


def run(menu_tool, *args):
    return subprocess.run([str(menu_tool), *map(str, args)], check=True,
                          capture_output=True, text=True).stdout


def test_kind_numbers_and_classification(menu_tool):
    assert run(menu_tool, "kinds").splitlines() == ["6", "1 0", "0 0", "1 0", "0 0", "1 1", "0 1"]


@pytest.mark.parametrize("singleplayer", [False, True])
@pytest.mark.parametrize("direction", [-1, 1])
def test_complete_spinner_cycles(menu_tool, singleplayer, direction):
    allowed = [0, 1, 2, 3, 4, 5] if singleplayer else [1, 3, 5]
    for previous in allowed:
        requested = (previous + direction) % 6
        expected = allowed[(allowed.index(previous) + direction) % len(allowed)]
        assert run(menu_tool, "spinner", int(singleplayer), previous, requested, 0).split() == [str(expected)] * 2


@pytest.mark.parametrize("singleplayer,expected", [(0, 1), (1, 0)])
def test_absent_spinner_preserves_default(menu_tool, singleplayer, expected):
    assert run(menu_tool, "spinner", singleplayer, 4, 4, 1).split()[0] == str(expected)


@pytest.mark.parametrize("kind", range(6))
def test_host_direct_category_selection(menu_tool, kind):
    assert run(menu_tool, "spinner", 1, 0, kind, 0).split() == [str(kind)] * 2


@pytest.mark.parametrize("route", ["solo", "host", "empty"])
@pytest.mark.parametrize("campaign", [0, 1])
def test_type_filtered_selection_routes(menu_tool, route, campaign):
    run(menu_tool, route, campaign)


def test_local_split_screen_cannot_start_mcc_coop(menu_tool):
    run(menu_tool, "local-reject")


def test_coop_back_restores_filtered_row_and_then_exits(menu_tool):
    run(menu_tool, "back")


@pytest.mark.parametrize("host", [0, 1])
@pytest.mark.parametrize("target", [1, 3, 4, 5])
def test_category_change_resets_mcc_state_and_preserves_legacy_dispatch(menu_tool, host, target):
    run(menu_tool, "switch", host, target)
