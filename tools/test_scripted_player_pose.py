"""Host scripted unit state through the production compact wire codec.

The body, desired facing and current aim intentionally differ, as in the
late-join cinematic capture. Ordinary input must not acquire pose correction.
"""
from pathlib import Path
import subprocess

import pytest
from harness import constant, enum_with, function, structure
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def generated():
    source = (ROOT/'port/linux/game/network_distributed.c').read_text()
    header = (ROOT/'port/linux/game/network_distributed.h').read_text()
    types = enum_with((ROOT/'source/game/players.h').read_text(), 'NUMBER_OF_PLAYER_POWERUPS,')
    types += '\n'.join(enum_with(source, token) for token in (
        '_distributed_unit_alive_bit =', '_distributed_unit_camouflaged_bit =',
        '_distributed_unit_part_riding_bit =', '_distributed_pose_facing,'))
    types += structure(header, 'distributed_vector')
    types += structure((ROOT/'port/linux/game/vehicle_riding.h').read_text(), 'vehicle_riding_deck')
    types += structure(source, 'distributed_unit_state')
    types += enum_with(source, 'DISTRIBUTED_UNIT_STATE_MINIMUM_SIZE =')
    history = constant((ROOT/'source/game/player_queues_new.h').read_text(), 'DISTRIBUTED_INPUT_HISTORY')
    routines = '\n'.join(function(source, name) for name in (
        'distributed_vector_pack', 'distributed_vector_unpack', 'distributed_unit_vector_unpack',
        'distributed_put', 'distributed_take', 'distributed_deck_put', 'distributed_deck_take', 'distributed_scripted_pose_from_unit',
        'distributed_scripted_pose_valid', 'distributed_scripted_pose_apply',
        'distributed_unit_state_write', 'distributed_unit_state_read', 'distributed_handle_predictions'))
    return r'''
#include <assert.h>
#include <string.h>
#include <math.h>
typedef unsigned char byte;
typedef unsigned short word;
typedef int boolean;
typedef float real;
typedef struct {real i,j,k;} real_vector3d;
typedef struct {real x,y,z;} real_point3d;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define NO_PLAYER 255
#define FLAG(b) (1u<<(b))
#define TEST_FLAG(v,b) ((v)&FLAG(b))
#define SET_FLAG(v,b,on) ((v)=(on)?(v)|FLAG(b):(v)&~FLAG(b))
#define csmemcpy memcpy
#define csmemset memset
#define DISTRIBUTED_UNIT_SCALE 32767.f
#define DISTRIBUTED_VELOCITY_SCALE 1024.f
#define vehicle_riding_deck_vehicle_named(...) 0
#define vehicle_riding_deck_valid(...) 0
#define vehicle_riding_deck_values_valid(...) 0
#define biped_riding_vehicle_node_count(...) 0
#define DISTRIBUTED_INPUT_HISTORY HISTORY_COUNT
enum {_game_connection_local, _game_connection_network_server, _game_connection_network_client};
enum {_unit_suspended_bit=24};
static int connection,enabled;
static int game_connection(void) {return connection;}
static int player_input_enabled(void) {return enabled;}
struct unit_datum {struct {
    unsigned flags;
    real_vector3d desired_facing_vector,desired_aiming_vector,desired_looking_vector;
    real_vector3d aiming_vector,looking_vector;
} unit;};
'''.replace('HISTORY_COUNT', str(history)) + types + r'''
#define MAXIMUM_TRACKED_PLAYERS 2
static struct {boolean valid;long time;struct distributed_unit_state state;} distributed_predictions[2];
static boolean distributed_machine_has_player(long machine,short player) {return machine==4 && player==0;}
''' + routines + r'''
static void near_vector(real_vector3d a,real_vector3d b) {
    assert(fabs(a.i-b.i)<.0001 && fabs(a.j-b.j)<.0001 && fabs(a.k-b.k)<.0001);
}
int main(void) {
    struct unit_datum host,client,unchanged;
    struct distributed_unit_state sent,received,ordinary;
    byte packet[512],bad[512];
    byte const ordinary_wire[11]={0,1,0,42,0,0,0,0,0,0,0};
    byte bounded[DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE+1];
    word count,plain;
    int i,suspended;
    memset(&host,0,sizeof(host));
    host.unit.desired_facing_vector.i=-1;
    host.unit.desired_aiming_vector.j=1;
    host.unit.desired_looking_vector.k=-1;
    host.unit.aiming_vector.i=1;
    host.unit.looking_vector.j=-1;
    for(suspended=0;suspended<2;suspended++) {
        memset(&sent,0,sizeof(sent));sent.flags=FLAG(_distributed_unit_alive_bit);
        sent.unit_index=42;sent.vehicle_index=NONE;sent.killing_player_index=NO_PLAYER;
        sent.forward.i=32767; /* Body direction does not supply the pose. */
        ordinary=sent;plain=distributed_unit_state_write(&ordinary,FALSE,packet);
        assert(plain==sizeof(ordinary_wire) && !memcmp(packet,ordinary_wire,plain));
        connection=_game_connection_network_server;enabled=suspended;
        host.unit.flags=suspended?FLAG(_unit_suspended_bit):0;
        distributed_scripted_pose_from_unit(&host,&sent);
        assert(sent.scripted_pose);
        count=distributed_unit_state_write(&sent,FALSE,packet);
        assert(count==plain+30+(suspended?2:0));
        assert(distributed_unit_state_read(packet,packet+count,&received)==count);
        memset(&client,0,sizeof(client));client.unit.flags=1;
        distributed_scripted_pose_apply(&client,&received);
        near_vector(client.unit.desired_facing_vector,host.unit.desired_facing_vector);
        near_vector(client.unit.desired_aiming_vector,host.unit.desired_aiming_vector);
        near_vector(client.unit.desired_looking_vector,host.unit.desired_looking_vector);
        near_vector(client.unit.aiming_vector,host.unit.aiming_vector);
        near_vector(client.unit.looking_vector,host.unit.looking_vector);
        assert(client.unit.flags==(1u|host.unit.flags));
        for(i=0;i<count;i++) assert(!distributed_unit_state_read(packet,packet+i,&received));
        /* Invalid optional pose vectors are omitted without discarding other unit state. */
        memcpy(bad,packet,count);memset(bad+count-6,0,6);
        assert(distributed_unit_state_read(bad,bad+count,&received)==count && !received.scripted_pose);
        memcpy(bad,packet,count);memset(bad+count-6,127,6);
        assert(distributed_unit_state_read(bad,bad+count,&received)==count && !received.scripted_pose);
        memcpy(bad,packet,count);bad[1]=0;
        assert(!distributed_unit_state_read(bad,bad+count,&received));
        /* All optional live fields fit the packer's declared bound. */
        received=sent;received.flags|=FLAG(_distributed_unit_placed_bit)|FLAG(_distributed_unit_predicted_bit);
        received.vehicle_index=7;received.up.i=1;
        received.current_body_damage=1;received.powerup_durations[0]=1;
        received.unit_flags|=FLAG(_distributed_unit_camouflaged_bit);
        memset(bounded,0xA5,sizeof(bounded));
        count=distributed_unit_state_write(&received,TRUE,bounded);
        assert(count<=DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE && bounded[DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE]==0xA5);
        assert(distributed_unit_state_read(bounded,bounded+count,&received)==count);
        for(i=0;i<count;i++) assert(!distributed_unit_state_read(bounded,bounded+i,&received));
        /* Untrusted clients cannot inject the host-owned extension through
           the prediction decoder shared with authoritative unit states. */
        received=sent;received.flags|=FLAG(_distributed_unit_placed_bit);
        count=distributed_unit_state_write(&received,TRUE,packet);
        distributed_predictions[0].valid=FALSE;
        distributed_handle_predictions(4,20,packet,packet+count,1);
        assert(!distributed_predictions[0].valid);
        received.scripted_pose=FALSE;received.unit_flags=FLAG(_distributed_unit_suspended_bit);
        count=distributed_unit_state_write(&received,TRUE,packet);
        distributed_handle_predictions(4,20,packet,packet+count,1);
        assert(!distributed_predictions[0].valid);
        received.unit_flags=0;count=distributed_unit_state_write(&received,TRUE,packet);
        distributed_handle_predictions(5,20,packet,packet+count,1);
        assert(!distributed_predictions[0].valid);
        distributed_handle_predictions(4,20,packet,packet+count,1);
        assert(distributed_predictions[0].valid && distributed_predictions[0].time==20);
        /* Releasing the script clears suspension but does not change aim. */
        unchanged=client;host.unit.flags=0;enabled=1;sent=ordinary;
        distributed_scripted_pose_from_unit(&host,&sent);assert(!sent.scripted_pose);
        count=distributed_unit_state_write(&sent,FALSE,packet);assert(count==plain);
        assert(distributed_unit_state_read(packet,packet+count,&received)==count);
        distributed_scripted_pose_apply(&client,&received);
        unchanged.unit.flags&=~FLAG(_unit_suspended_bit);
        assert(!memcmp(&client,&unchanged,sizeof(client)));
    }
    for(connection=_game_connection_local;connection<=_game_connection_network_client;connection++) {
        if(connection==_game_connection_network_server)continue;
        sent=ordinary;enabled=0;host.unit.flags=FLAG(_unit_suspended_bit);
        distributed_scripted_pose_from_unit(&host,&sent);
        assert(!sent.scripted_pose && !sent.unit_flags); /* Clients cannot author it. */
    }
    return 0;
}
'''


def test_scripted_pose_wire_and_ordinary_input(tmp_path):
    run_c(tmp_path, generated(), link_math=True)


@pytest.mark.parametrize('before,after', [
    ('if (!state->scripted_pose)\n\t\treturn;', 'return;'),
    ('&unit->unit.aiming_vector);', '&unit->unit.desired_aiming_vector);'),
    ('state.scripted_pose ||', ''),
    ('TEST_FLAG(state.unit_flags, _distributed_unit_suspended_bit) ||', ''),
])
def test_regression_detects_missing_or_conflated_pose(tmp_path, before, after):
    code = generated()
    assert before in code
    with pytest.raises(subprocess.CalledProcessError):
        run_c(tmp_path, code.replace(before, after), link_math=True)


def test_pose_is_captured_and_applied_on_existing_unit_state_path():
    source = (ROOT/'port/linux/game/network_distributed.c').read_text()
    capture = function(source, 'distributed_state_from_player')
    apply = function(source, 'distributed_handle_unit_state')
    assert 'distributed_scripted_pose_from_unit(unit, state);' in capture
    assert apply.index('network_player_attach_unit(') < apply.index('distributed_scripted_pose_apply(')


def test_turn_in_place_reconciles_only_scripted_orientation(tmp_path):
    source = (ROOT/'port/linux/game/network_distributed.c').read_text()
    body = function(source, 'distributed_apply_state')
    run_c(tmp_path, r'''
#include <assert.h>
#include <math.h>
#include <stddef.h>
typedef int boolean;
typedef float real;
typedef struct {real i,j,k;} real_vector3d;
typedef struct {real x,y,z;} real_point3d;
struct distributed_unit_state {real_vector3d velocity,forward,up;int scripted_pose;};
struct object_datum {struct {real_point3d position;real_vector3d translational_velocity;} object;};
#define _network_vehicle_write_state 0
static struct object_datum object;
static struct {int corrections;} distributed_statistics;
static int reconciles;
#define FALSE 0
#define TRUE 1
#define DISTRIBUTED_VELOCITY_SCALE 1024
#define DISTRIBUTED_UNIT_SCALE 32767
#define UNIT_WORLD_BOUND 32768
static struct object_datum *object_get(long index) {assert(index==3);return &object;}
static void distributed_vector_unpack(real_vector3d const *a,real scale,real_vector3d *b) {*b=*a;}
static int distributed_point_valid(real_point3d const *p,real limit) {return p->x<limit;}
static int distributed_axes_make_valid(real_vector3d *a,real_vector3d *b) {return a->i==1 && b->k==1;}
static int network_objects_reconcile(long index,real_point3d const *p,real_vector3d const *f,
 real_vector3d const *u,real_vector3d const *v,void *angular,real blend,int flags) {
 assert(index==3 && f->i==1 && u->k==1);reconciles++;return 0;
}
''' + body + r'''
int main(void) {
 struct distributed_unit_state state={{0,0,0},{1,0,0},{0,0,1},0};
 real_point3d p={0,0,0};
 assert(distributed_apply_state(3,&state,&p,NULL,1,1,0,0) && reconciles==0);
 state.scripted_pose=1;
 assert(distributed_apply_state(3,&state,&p,NULL,1,1,0,0) && reconciles==1);
 state.forward.i=0;
 assert(!distributed_apply_state(3,&state,&p,NULL,1,1,0,0) && reconciles==1);
 state.forward.i=1;p.x=40000;
 assert(!distributed_apply_state(3,&state,&p,NULL,1,1,0,0) && reconciles==1);
 state.scripted_pose=0;p.x=2;
 assert(distributed_apply_state(3,&state,&p,NULL,1,1,0,0) && reconciles==2);
 return 0;
}
''', link_math=True)
