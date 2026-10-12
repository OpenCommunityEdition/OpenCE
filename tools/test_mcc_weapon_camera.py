"""Real MCC selector and director transitions against a bounded fake inventory."""
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def camera(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    work = tmp_path_factory.mktemp("mcc-weapon-camera")
    adapter = (ROOT / "port/linux/game/mcc_camera.c").read_text()
    director = (ROOT / "source/camera/director.c").read_text()
    source = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean;
typedef short director_perspective;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAXIMUM_WEAPONS_PER_UNIT 4
#define TEST_FLAG(v,b) (((v)&(1UL<<(b)))!=0)
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
enum { _object_dead_bit, _object_mask_unit=2, _unit_seat_third_person_on_enter_bit=6,
       _unit_seat_third_person_camera_bit=4, _unit_state_entering_seat=1,
       _unit_state_exiting_seat=2, _director_perspective_third_person=1 };
struct object_datum { long definition_index; struct {long parent_object_index; unsigned long damage_flags; short type;} object; };
struct unit_datum {long definition_index; struct {long parent_object_index; unsigned long damage_flags; short type;} object;
    struct {short current_weapon_index,current_zoom_level,parent_seat_index;
        long weapon_object_indices[4];struct {short state;} animation;} unit;};
struct weapon_datum {long definition_index;};
struct weapon_definition {struct {unsigned long flags;} weapon;};
struct unit_seat {unsigned long flags;};
struct unit_definition {struct {struct {void *address;} seats;} unit;};
static struct unit_datum units[2];
static struct weapon_datum weapons[2]={{0},{1}};
static struct weapon_definition definitions[2];
static struct unit_seat seat;
static struct unit_definition unit_definition={{{&seat}}};
static int active=1, reads;
static boolean mcc_cache_tags_loaded(void) {return active;}
static struct unit_datum *unit_try_and_get(long i) {++reads;return i==0x10000?&units[0]:NULL;}
static struct unit_datum *unit_get(long i) {return &units[i&1];}
static struct object_datum *object_get(long i) {return (struct object_datum *)&units[i&1];}
static struct unit_definition *unit_definition_get(long i) {(void)i;return &unit_definition;}
static struct weapon_datum *weapon_try_and_get(long i) {return i==0x20000?&weapons[0]:i==0x20001?&weapons[1]:NULL;}
static struct weapon_definition *weapon_definition_get(long i) {return &definitions[i];}
''' + function(adapter, "mcc_camera_weapon_following") + '\n' + function(director, "director_desired_perspective") + r'''
typedef void (*director_camera_update_proc)(void *,void *,void *);
struct first_person_camera {int unused;};struct following_camera {int unused;};
struct director {short seat_state;director_camera_update_proc camera_proc;char camera_data[64];};
static struct director view;
static int changes, interpolated;
static void first_person_camera_update(void *a,void *b,void *c){(void)a;(void)b;(void)c;}
static void following_camera_update(void *a,void *b,void *c){(void)a;(void)b;(void)c;}
static void first_person_camera_new(struct first_person_camera *p){(void)p;}
static void following_camera_new(struct following_camera *p){(void)p;}
static struct director *director_get(short i){(void)i;return &view;}
static long player_control_get_unit_index(short i){(void)i;return 0x10000;}
static void director_set_camera(short i,director_camera_update_proc p,boolean interpolate){
    (void)i;view.camera_proc=p;++changes;interpolated=interpolate;
}
''' + function(director, "director_choose_game_perspective") + r'''
#define CHECK(v) do {if(!(v)){fprintf(stderr,"line %d\n",__LINE__);return 1;}} while(0)
int main(int argc,char **argv){
    director_perspective perspective;
    int n;
    if(argc!=2)return 2;
    units[0].object.parent_object_index=NONE;
    units[0].unit.current_zoom_level=NONE;
    units[0].unit.weapon_object_indices[0]=0x20000;
    units[0].unit.weapon_object_indices[1]=0x20001;
    definitions[0].weapon.flags=0x20000UL;
    view.camera_proc=first_person_camera_update;
    if(!strcmp(argv[1],"flags")){
        CHECK(mcc_camera_weapon_following(0x10000));
        definitions[0].weapon.flags=0x10000;CHECK(!mcc_camera_weapon_following(0x10000));
        definitions[0].weapon.flags=0x40000;CHECK(!mcc_camera_weapon_following(0x10000));
    }else if(!strcmp(argv[1],"isolation")){
        active=0;CHECK(!mcc_camera_weapon_following(0x10000));CHECK(reads==0);
        CHECK(!director_desired_perspective(0x10000,&perspective));CHECK(perspective==0);
        active=1;CHECK(mcc_camera_weapon_following(0x10000));
        active=0;CHECK(!mcc_camera_weapon_following(0x10000));
    }else if(!strcmp(argv[1],"invalid")){
        CHECK(!mcc_camera_weapon_following(NONE));CHECK(!mcc_camera_weapon_following(0x30000));
        for(n=-2;n<=5;++n){units[0].unit.current_weapon_index=n;
            CHECK(mcc_camera_weapon_following(0x10000)==(n==0));}
        units[0].unit.current_weapon_index=0;
        units[0].unit.weapon_object_indices[0]=NONE;CHECK(!mcc_camera_weapon_following(0x10000));
        units[0].unit.weapon_object_indices[0]=0x30000;CHECK(!mcc_camera_weapon_following(0x10000));
    }else if(!strcmp(argv[1],"lifecycle")){
        units[0].object.damage_flags=1;CHECK(!mcc_camera_weapon_following(0x10000));
        units[0].object.damage_flags=0;CHECK(mcc_camera_weapon_following(0x10000));
        units[0].unit.current_zoom_level=0;CHECK(!mcc_camera_weapon_following(0x10000));
        units[0].unit.current_zoom_level=NONE;CHECK(mcc_camera_weapon_following(0x10000));
    }else if(!strcmp(argv[1],"seats")){
        units[0].object.parent_object_index=1;units[1].object.type=1;
        CHECK(!mcc_camera_weapon_following(0x10000));
        CHECK(!director_desired_perspective(0x10000,&perspective));CHECK(perspective==2);
        seat.flags=1<<4;CHECK(director_desired_perspective(0x10000,&perspective));CHECK(perspective==2);
        active=0;CHECK(director_desired_perspective(0x10000,&perspective));CHECK(perspective==2);
    }else if(!strcmp(argv[1],"switch")){
        director_choose_game_perspective(0,FALSE);
        CHECK(view.camera_proc==following_camera_update&&changes==1&&interpolated);
        director_choose_game_perspective(0,FALSE);CHECK(changes==1);
        units[0].unit.current_weapon_index=1;director_choose_game_perspective(0,FALSE);
        CHECK(view.camera_proc==first_person_camera_update&&changes==2);
        units[0].unit.current_weapon_index=0;director_choose_game_perspective(0,FALSE);
        CHECK(view.camera_proc==following_camera_update&&changes==3);
        active=0;director_choose_game_perspective(0,FALSE);
        CHECK(view.camera_proc==first_person_camera_update&&changes==4);
    }else return 3;
    return 0;
}
'''
    path = work / "camera.c"
    path.write_text(source)
    exe = work / ("camera.exe" if sys.platform == "win32" else "camera")
    command = [clang, "-std=gnu89", "-Wall", "-Wextra", "-Werror", str(path), "-o", str(exe)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.mark.parametrize("case", ["flags", "isolation", "invalid", "lifecycle", "seats", "switch"])
def test_weapon_camera(camera, case):
    result = subprocess.run([str(camera), case], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
