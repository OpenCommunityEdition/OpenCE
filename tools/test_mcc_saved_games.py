"""Behavior checks for MCC-only checkpoint routing using actual menu functions."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def saved_game_tool(tmp_path_factory):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed for the MCC checkpoint tests")
    work = tmp_path_factory.mktemp("mcc-saved-games")
    menu = (ROOT / "port/linux/game/menu_functions.c").read_text()
    functions = "\n".join(function(menu, name) for name in
                          ("mcc_saved_game_read", "campaign_continue", "saved_games_read", "saved_game_continue"))
    source = r'''
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
typedef unsigned char boolean;
typedef unsigned short word;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define NUMBER_OF_GAME_DIFFICULTY_LEVELS 4
#define MAXIMUM_ROWS 11
#define MAXIMUM_PROFILES 32
#define MAXIMUM_PLAYER_PROFILE_NAME_LENGTH 11
#define PROFILE_VALID_BIT 0x80000000UL
#define PIN(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
struct player_profile {wchar_t player_name[12];};
''' + structure(menu, "campaign_saved_game") + r'''
static struct {
    short saved_game_count,shown_saved_game;
    struct campaign_saved_game saved_games[MAXIMUM_ROWS];
} campaign;
static char mcc_saved_game_names[MAXIMUM_ROWS][256];
static long active=PROFILE_VALID_BIT;
static char saves[3][256];
static short difficulty=2;
static int storage_valid=1,required=0,available=1,stock_calls=0,starts=0;
static char started[256];
long player_ui_get_active_player_profile_index(short player) {(void)player;return active;}
boolean game_state_test_persistent_storage(char *name,short *out,boolean *corrupt) {
    strcpy(name,saves[active&255]);*out=difficulty;*corrupt=0;return storage_valid;
}
boolean mcc_level_name(char const *name) {return !strncmp(name,"mcc_maps\\",9);}
boolean mcc_cache_require(char const *name,unsigned long checksum) {
    (void)checksum;if(!mcc_level_name(name))exit(10);required++;return available;
}
boolean campaign_profile(short controller,struct player_profile *profile) {
    (void)controller;wcscpy(profile->player_name,L"player");return active!=NONE;
}
boolean campaign_fail(void) {return FALSE;}
void campaign_start(char const *name,short level,short controller) {
    (void)level;(void)controller;strcpy(started,name);starts++;
}
boolean ui_widget_port_saved_game(char const **name,short *level,short *out) {
    stock_calls++;*name=saves[active&255];*level=0;*out=difficulty;
    return storage_valid && !strncmp(*name,"levels\\",7);
}
void player_profiles_enumerate_available_to_local_player_index(long player,word *count,long *profiles,boolean unused) {
    int i;(void)player;(void)unused;*count=3;
    for(i=0;i<3;i++)profiles[i]=PROFILE_VALID_BIT+i;
}
boolean player_profile_get(long index,struct player_profile *profile) {
    if((index&255)>2)return FALSE;wcscpy(profile->player_name,L"player");return TRUE;
}
void player_ui_set_active_player_profile(short player,long index,struct player_profile const *profile) {
    (void)player;(void)profile;active=index;
}
void ustrncpy(wchar_t *out,wchar_t const *in,long count) {wcsncpy(out,in,(size_t)count);}
void campaign_levels_read(struct player_profile const *profile) {(void)profile;}
char const *main_get_solo_level_name(short level) {if(level!=0)exit(11);return "levels\\a10\\a10";}
''' + functions + r'''
int main(int argc,char **argv) {
    int result=0;
    if(argc<2)return 2;
    strcpy(saves[0],"levels\\a10\\a10");
    strcpy(saves[1],"mcc_maps\\a10");
    strcpy(saves[2],"custom_maps\\a10");
    if(!strcmp(argv[1],"continue")) {
        if(argc<3)return 2;
        strcpy(saves[0],argv[2]);
        if(argc>3)available=atoi(argv[3]);
        result=campaign_continue(0);
    } else if(!strcmp(argv[1],"list")) {
        saved_games_read(0);
        if(campaign.saved_game_count!=2 || mcc_saved_game_names[0][0] ||
           strcmp(mcc_saved_game_names[1],"mcc_maps\\a10") || (unsigned long)active!=PROFILE_VALID_BIT)return 12;
        if(argc>2) {
            campaign.shown_saved_game=(short)atoi(argv[2]);
            result=saved_game_continue(0);
        } else result=1;
    } else if(!strcmp(argv[1],"read")) {
        char name[256];short out=0;
        if(argc<3)return 2;
        strcpy(saves[0],argv[2]);
        if(argc>3)difficulty=(short)atoi(argv[3]);
        result=mcc_saved_game_read(name,&out);
        if(!result && name[0])return 13;
        printf("difficulty=%d\n",out);
    } else return 2;
    printf("result=%d\nstarts=%d\nstarted=%s\nrequired=%d\nstock_calls=%d\ncount=%d\n",
        result,starts,started,required,stock_calls,campaign.saved_game_count);
    return 0;
}
'''
    path = work / "saved_games.c"
    path.write_text(source)
    output = work / ("saved_games.exe" if sys.platform == "win32" else "saved_games")
    command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror", str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


def run(tool, *args):
    result = subprocess.run([str(tool), *map(str, args)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return dict(line.split("=", 1) for line in result.stdout.splitlines())


@pytest.mark.parametrize("name,starts,required,stock_calls", [
    ("mcc_maps\\a10", 1, 1, 0),
    ("levels\\a10\\a10", 1, 0, 1),
    ("custom_maps\\a10", 0, 0, 1),
])
def test_continue_routes_only_mcc_to_new_loader(saved_game_tool, name, starts, required, stock_calls):
    fields = run(saved_game_tool, "continue", name)
    assert fields["starts"] == str(starts)
    assert fields["required"] == str(required)
    assert fields["stock_calls"] == str(stock_calls)
    if starts:
        assert fields["started"] == name


def test_missing_mcc_checkpoint_map_does_not_start(saved_game_tool):
    fields = run(saved_game_tool, "continue", "mcc_maps\\missing", 0)
    assert fields["result"] == fields["starts"] == "0"


def test_save_list_keeps_stock_and_mcc_identities_separate(saved_game_tool):
    fields = run(saved_game_tool, "list")
    assert fields["count"] == "2"


@pytest.mark.parametrize("row,expected,required", [(0, "levels\\a10\\a10", 0), (1, "mcc_maps\\a10", 1)])
def test_saved_game_selection_uses_its_own_namespace(saved_game_tool, row, expected, required):
    fields = run(saved_game_tool, "list", row)
    assert fields["result"] == "1"
    assert fields["started"] == expected
    assert fields["required"] == str(required)


@pytest.mark.parametrize("difficulty,expected", [(-8, 0), (8, 3)])
def test_mcc_saved_difficulty_is_bounded(saved_game_tool, difficulty, expected):
    fields = run(saved_game_tool, "read", "mcc_maps\\a10", difficulty)
    assert fields["result"] == "1"
    assert fields["difficulty"] == str(expected)
