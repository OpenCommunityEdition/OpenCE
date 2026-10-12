"""Execute the real MCC interaction and shared seat receiver against fake objects."""
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def seats(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    folder = tmp_path_factory.mktemp("mcc-seats")
    adapter = (ROOT / "port/linux/game/mcc_seats.c").read_text()
    network = (ROOT / "port/linux/game/network_objects.c").read_text()
    source = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define NULL ((void *)0)
#define TEST_FLAG(v,b) (((v)&(1UL<<(b)))!=0)
enum {_object_dead_bit=0,_unit_control_action_bit=1,_object_mask_biped=1,_object_mask_vehicle=2,
      _object_mask_unit=3,_game_connection_network_client=2,_unit_nearby_seat_available=2};
struct object_data {long parent_object_index;unsigned long damage_flags;int type;};
struct object_datum {long definition_index;struct object_data object;};
struct unit_datum {long definition_index;struct object_data object;struct {short parent_seat_index;unsigned long control_flags;} unit;};
struct unit_camera {int unused;};
struct unit_seat {struct unit_camera camera;};
static struct unit_seat seats[16];
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&seats[i])
struct player_control_unit_camera_info {long unit_index;short seat_index;struct unit_camera *camera;};
struct unit_definition {struct {struct {int count;} seats;} unit;};
struct player_datum {long unit_index;};
struct object_header_datum {struct object_datum *datum;};
static struct unit_datum units[20];
static struct unit_definition definitions[20];
static struct object_header_datum headers[20];
static struct player_datum player={0};
static int active=1,reads,connection,overcharged,nearby=2,boarded,exited,animation=1;
static long occupant=NONE;
static int exists[20],known[20];
static void *object_header_data;
static boolean mcc_cache_tags_loaded(void){return active;}
static struct object_datum *object_try_and_get(long i){++reads;return i>=0&&i<20&&exists[i]?(struct object_datum *)&units[i]:NULL;}
static struct object_datum *object_try_and_get_and_verify_type(long i,int mask){
    struct object_datum *o=object_try_and_get(i);return o&&(o->object.type&mask)?o:NULL;
}
static struct unit_definition *unit_definition_get(long i){return &definitions[i];}
static struct player_datum *player_try_and_get(long i){return i==0?&player:NULL;}
static struct player_datum *player_get(long i){return player_try_and_get(i);}
static int unit_overcharged(long i){(void)i;return overcharged;}
static short unit_find_nearby_seat(long u,long p,short *s){(void)u;(void)p;*s=0;return nearby;}
static int game_connection(void){return connection;}
static int unit_enter_seat(long u,long p,short s){++boarded;units[u].object.parent_object_index=p;units[u].unit.parent_seat_index=s;return 1;}
static void unit_exit_seat_end(long u){++exited;units[u].object.parent_object_index=NONE;units[u].unit.parent_seat_index=NONE;}
static int unit_try_and_exit_seat(long u){unit_exit_seat_end(u);return 1;}
static int distributed_object_index_valid(long i){return i>=0&&i<20&&exists[i];}
static int network_objects_client_has(long i){return i>=0&&i<20&&known[i];}
static int unit_has_animation_to_enter_seat(long u,long p,short s){(void)u;return animation&&s>=0&&s<definitions[p].unit.seats.count;}
static void *datum_try_and_get(void *d,long i){(void)d;return i>=0&&i<20&&exists[i]?&headers[i]:NULL;}
static int unit_can_enter_seat(long u,long p,short s,long *out){(void)u;(void)p;(void)s;*out=occupant;return occupant==NONE;}
''' + '\n'.join(function(adapter, n) for n in ("mcc_seat_parent", "mcc_seat_nearby", "mcc_seat_action", "mcc_seat_update", "mcc_seat_camera")) + '\n' + function(network, "network_objects_set_seat") + r'''
#define CHECK(v) do{if(!(v)){fprintf(stderr,"line %d\n",__LINE__);return 1;}}while(0)
int main(int argc,char **argv){
    int i;short s;
    if(argc!=2)return 2;
    for(i=0;i<20;++i){exists[i]=known[i]=1;units[i].definition_index=i;
        units[i].object.type=1;units[i].object.parent_object_index=NONE;
        units[i].unit.parent_seat_index=NONE;headers[i].datum=(struct object_datum *)&units[i];}
    definitions[1].unit.seats.count=1;
    if(!strcmp(argv[1],"isolation")){
        active=0;CHECK(!mcc_seat_nearby(0,1,&s));CHECK(reads==0);
        CHECK(!mcc_seat_action(0,1,0));network_objects_set_seat(0,1,0);CHECK(!boarded);
        units[1].object.type=2;network_objects_set_seat(0,1,0);CHECK(boarded==1);
    }else if(!strcmp(argv[1],"interaction")){
        CHECK(mcc_seat_nearby(0,1,&s)&&s==0);CHECK(mcc_seat_action(0,1,0));CHECK(boarded==1);
        CHECK(!mcc_seat_nearby(0,1,&s));unit_exit_seat_end(0);CHECK(mcc_seat_nearby(0,1,&s));
    }else if(!strcmp(argv[1],"authority")){
        connection=2;CHECK(mcc_seat_nearby(0,1,&s));CHECK(mcc_seat_action(0,1,0));CHECK(!boarded);
        connection=1;nearby=0;CHECK(mcc_seat_action(0,1,0));CHECK(!boarded);
        nearby=2;CHECK(mcc_seat_action(0,1,1));CHECK(!boarded);
        CHECK(mcc_seat_action(0,1,0));CHECK(boarded==1);
    }else if(!strcmp(argv[1],"dead")){
        units[0].object.damage_flags=1;CHECK(!mcc_seat_nearby(0,1,&s));
        units[0].object.damage_flags=0;units[1].object.damage_flags=1;CHECK(!mcc_seat_nearby(0,1,&s));
        network_objects_set_seat(0,1,0);CHECK(!boarded);
        units[1].object.damage_flags=0;overcharged=1;CHECK(!mcc_seat_nearby(0,1,&s));
    }else if(!strcmp(argv[1],"invalid")){
        CHECK(!mcc_seat_nearby(0,NONE,&s));CHECK(!mcc_seat_nearby(1,1,&s));
        CHECK(!mcc_seat_nearby(0,0,&s));definitions[1].unit.seats.count=17;
        CHECK(!mcc_seat_nearby(0,1,&s));definitions[1].unit.seats.count=0;CHECK(!mcc_seat_nearby(0,1,&s));
    }else if(!strcmp(argv[1],"cycles")){
        units[1].object.parent_object_index=0;CHECK(!mcc_seat_nearby(0,1,&s));
        network_objects_set_seat(0,1,0);CHECK(!boarded);
        units[1].object.parent_object_index=1;CHECK(!mcc_seat_nearby(0,1,&s));
        network_objects_set_seat(0,1,0);CHECK(!boarded);
        units[1].object.parent_object_index=19;exists[19]=0;CHECK(!mcc_seat_nearby(0,1,&s));
        network_objects_set_seat(0,1,0);CHECK(!boarded);
    }else if(!strcmp(argv[1],"replication")){
        connection=2;network_objects_set_seat(0,1,0);CHECK(boarded==1);
        CHECK(units[0].object.parent_object_index==1);network_objects_set_seat(0,NONE,NONE);
        CHECK(exited==1&&units[0].object.parent_object_index==NONE);
        occupant=2;network_objects_set_seat(0,1,0);CHECK(boarded==2&&exited==2);
    }else if(!strcmp(argv[1],"seat_update")){
        units[0].object.parent_object_index=1;
        CHECK(!mcc_seat_update(0)); /* Ordinary infection attachment has no seat. */
        units[0].unit.parent_seat_index=0;active=0;CHECK(!mcc_seat_update(0));
        active=1;CHECK(mcc_seat_update(0)&&!exited);
        units[0].unit.control_flags=2;connection=2;CHECK(mcc_seat_update(0)&&!exited);
        connection=1;CHECK(mcc_seat_update(0)&&exited==1);
        units[0].object.parent_object_index=1;units[0].unit.parent_seat_index=0;
        units[1].object.damage_flags=1;CHECK(mcc_seat_update(0)&&exited==2);
    }else if(!strcmp(argv[1],"camera")){
        struct player_control_unit_camera_info info={0,NONE,NULL};
        mcc_seat_camera(&info);CHECK(info.unit_index==0&&info.camera==NULL);
        units[0].object.parent_object_index=1;units[0].unit.parent_seat_index=0;
        active=0;mcc_seat_camera(&info);CHECK(info.camera==NULL);
        active=1;mcc_seat_camera(&info);CHECK(info.unit_index==1&&info.seat_index==0&&info.camera==&seats[0].camera);
        info.unit_index=0;info.seat_index=NONE;info.camera=NULL;units[0].unit.parent_seat_index=1;
        mcc_seat_camera(&info);CHECK(info.camera==NULL);
    }else if(!strcmp(argv[1],"network_bounds")){
        for(i=-2;i<20;++i)if(i!=0){network_objects_set_seat(0,1,(short)i);CHECK(!boarded);}
        known[1]=0;network_objects_set_seat(0,1,0);CHECK(!boarded);
        known[1]=1;animation=0;network_objects_set_seat(0,1,0);CHECK(!boarded);
        animation=1;exists[1]=0;network_objects_set_seat(0,1,0);CHECK(!boarded);
        network_objects_set_seat(0,0,0);CHECK(!boarded);
    }else return 3;
    return 0;
}
'''
    path = folder / "seats.c"
    path.write_text(source)
    exe = folder / ("seats.exe" if sys.platform == "win32" else "seats")
    args = [clang, "-std=gnu89", str(path), "-o", str(exe)]
    if sys.platform == "win32":
        args[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"]
    result = subprocess.run(args, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.mark.parametrize("case", ["isolation", "interaction", "authority", "dead", "invalid", "cycles", "replication", "seat_update", "camera", "network_bounds"])
def test_mcc_seats(seats, case):
    result = subprocess.run([str(seats), case], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
