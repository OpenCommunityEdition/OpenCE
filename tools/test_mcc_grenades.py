"""Execute the MCC-only inventory and snapshot routines with salted fake units.

The native two-byte inventory has guard bytes, so additional slots cannot
silently extend its layout. No game data or CE code participates.
"""
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def grenade_tool(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is required for MCC inventory tests")
    work = tmp_path_factory.mktemp("mcc-grenades")
    implementation = (ROOT / "port/linux/game/mcc_grenades.c").read_text()
    names = ["mcc_grenades_active", "mcc_grenades_type_count", "mcc_grenade_definition",
        "mcc_grenade_inventory", "mcc_grenades_reset", "mcc_grenades_clear", "mcc_grenades_get",
        "mcc_grenades_set", "mcc_grenades_total", "mcc_grenades_next", "mcc_grenades_add",
        "mcc_grenade_u32", "mcc_grenades_snapshot", "mcc_grenades_snapshot_validate", "mcc_grenades_restore"]
    source = r'''
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
typedef int boolean;
#define NONE -1
#define MAXIMUM_OBJECTS_PER_MAP 8
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
struct unit_datum {struct {signed char grenade_counts[2];unsigned char guards[2];char current_grenade_index,desired_grenade_index;} unit;};
struct game_globals_grenade {short maximum_count;};
struct game_globals {struct {long count;void *address;} grenades;};
static struct unit_datum units[8];
static long handles[8];
static int active=1;
static struct game_globals_grenade definitions[4];
static struct game_globals globals={ {4,definitions} };
static boolean mcc_cache_tags_loaded(void){return active;}
static struct game_globals *scenario_get_game_globals(void){return &globals;}
static struct unit_datum *unit_try_and_get(long handle){
    unsigned i=(unsigned long)handle&65535;
    return handle!=NONE && i<8 && handles[i]==handle ? &units[i] : NULL;
}
''' + structure(implementation, "mcc_grenade_inventory") + r'''
static struct mcc_grenade_inventory mcc_grenade_inventories[MAXIMUM_OBJECTS_PER_MAP];
static long mcc_grenade_flash_time[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
''' + '\n'.join(function(implementation, n) for n in names) + r'''
int main(int argc,char **argv){
    long unit=0x12340001;unsigned char snapshot[72],bad[72];uint32_t bytes;unsigned i;
    if(argc!=2)return 99;
    for(i=0;i<8;i++)handles[i]=NONE;
    handles[1]=unit;units[1].unit.guards[0]=0xAA;units[1].unit.guards[1]=0x55;
    mcc_grenades_reset();
    mcc_grenades_set(unit,0,2);mcc_grenades_set(unit,1,3);mcc_grenades_set(unit,2,4);mcc_grenades_set(unit,3,5);
    if(!strcmp(argv[1],"inventory")){
        if(mcc_grenades_total(unit)!=14 || mcc_grenades_get(unit,2)!=4 || mcc_grenades_get(unit,3)!=5)return 1;
        if(units[1].unit.guards[0]!=0xAA || units[1].unit.guards[1]!=0x55)return 2;
        if(mcc_grenades_next(unit,1,1)!=2 || mcc_grenades_next(unit,0,-1)!=3 || mcc_grenades_next(unit,2,0)!=2)return 3;
        mcc_grenades_set(unit,2,0);
        if(mcc_grenades_next(unit,1,1)!=3)return 4;
        if(mcc_grenades_add(unit,2,7)!=7 || units[1].unit.current_grenade_index!=2)return 5;
        mcc_grenades_set(unit,2,300);mcc_grenades_set(unit,3,-3);
        if(mcc_grenades_get(unit,2)!=127 || mcc_grenades_get(unit,3)!=0)return 6;
        mcc_grenades_clear(unit);
        if(mcc_grenades_get(unit,2) || mcc_grenades_total(unit)!=5)return 7;
        active=0;mcc_grenades_set(unit,0,99);
        if(mcc_grenades_get(unit,0) || units[1].unit.grenade_counts[0]!=2)return 8;
        return 0;
    }
    bytes=mcc_grenades_snapshot(snapshot,sizeof(snapshot));
    if(bytes!=16 || mcc_grenades_snapshot(NULL,0)!=bytes || mcc_grenades_snapshot(bad,15))return 9;
    if(memcmp(snapshot,"GMC4\1\0\1\0\1\0\x34\x12\4\5\0\0",16))return 10;
    if(!strcmp(argv[1],"roundtrip")){
        mcc_grenades_clear(unit);
        if(!mcc_grenades_restore(snapshot,bytes) || mcc_grenades_get(unit,2)!=4 || mcc_grenades_get(unit,3)!=5)return 11;
        if(units[1].unit.grenade_counts[0]!=2 || units[1].unit.guards[0]!=0xAA)return 12;
        handles[1]=0x23450001;
        if(mcc_grenades_get(handles[1],2) || mcc_grenades_restore(snapshot,bytes))return 13;
        mcc_grenades_set(handles[1],2,9);
        if(mcc_grenades_get(handles[1],2)!=9 || mcc_grenades_get(unit,2))return 14;
        return 0;
    }
    if(!strcmp(argv[1],"invalid")){
        for(i=0;i<16;i++)if(mcc_grenades_snapshot_validate(snapshot,i))return 15;
        memcpy(bad,snapshot,16);bad[4]=2;if(mcc_grenades_snapshot_validate(bad,16))return 16;
        memcpy(bad,snapshot,16);bad[12]=128;if(mcc_grenades_snapshot_validate(bad,16))return 17;
        memcpy(bad,snapshot,16);bad[14]=1;if(mcc_grenades_snapshot_validate(bad,16))return 18;
        memcpy(bad,snapshot,16);bad[8]=8;if(mcc_grenades_snapshot_validate(bad,16))return 19;
        memcpy(bad,snapshot,16);bad[6]=2;memcpy(bad+16,snapshot+8,8);
        if(mcc_grenades_snapshot_validate(bad,24) || mcc_grenades_restore(bad,24))return 20;
        if(mcc_grenades_get(unit,2)!=4 || mcc_grenades_get(unit,3)!=5)return 21;
        return 0;
    }
    return 98;
}
'''
    path = work / "grenades.c"
    path.write_text(source)
    output = work / ("grenades.exe" if sys.platform == "win32" else "grenades")
    command = [clang, "-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror", str(path), "-o", str(output)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return output


@pytest.mark.parametrize("case", ["inventory", "roundtrip", "invalid"])
def test_mcc_inventory_and_snapshots(grenade_tool, case):
    result = subprocess.run([str(grenade_tool), case], capture_output=True, text=True)
    assert result.returncode == 0, (case, result.returncode, result.stdout, result.stderr)
