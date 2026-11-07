"""Exercise the production dispatcher up to its handlers across world boundaries."""
from pathlib import Path

import pytest
from harness import enum_with, function, structure
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def generated():
    source = (ROOT / "port/linux/game/network_distributed.c").read_text()
    header = (ROOT / "port/linux/game/network_distributed.h").read_text()
    enum_start = header.index("enum\n{")
    kinds = header[enum_start:header.index("\n};", enum_start) + 3]
    dispatch = function(source, "network_distributed_handle_message")
    # Preserve all real framing, batch recursion, source, epoch and stale checks.
    # Only the final gameplay handlers are replaced by observable deliveries.
    dispatch = dispatch[:dispatch.rindex("\tswitch (header.type)")] + "\tdelivered[header.type]++;\n}\n"
    prefix = r'''
#include <assert.h>
#include <string.h>
#include <limits.h>
typedef unsigned char byte;
typedef unsigned short word, message_header;
typedef int boolean;
typedef float real;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define csmemcpy memcpy
#define csmemset memset
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 4
#define MAXIMUM_SENDERS 5
#define HOST_SENDER 4
#define DATAGRAM_MAXIMUM_SIZE 1200
#define DISTRIBUTED_TIME_WINDOW_TICKS 450
#define DISTRIBUTED_UNIT_STATE_MINIMUM_SIZE 1
#define DISTRIBUTED_RELAYED_ACTION_MINIMUM_SIZE 1
#define _game_connection_network_client 1
#define _game_connection_network_server 2
#define _error_silent 0
struct distributed_player_statistics {byte data;};
struct distributed_player_ping {byte data;};
struct distributed_structure_bsp {byte data;};
struct distributed_pickup {byte data;};
struct distributed_player_input {byte data;};
struct distributed_client_identity {byte data;};
struct distributed_client_build {byte data;};
static int connection=1, unreliable, in_game=1, mcc=1, resets, clock_notes;
static int delivered[256];
static int distributed_handling_batch, distributed_handling_stream_message;
static unsigned long distributed_checkpoint_epoch, distributed_host_epoch;
static boolean distributed_checkpoint_synchronized;
static boolean distributed_identity_sent=1, distributed_build_sent=1;
static long distributed_host_time=NONE;
static unsigned long distributed_host_time_received;
static struct {int received,sent;} distributed_statistics;
static struct {unsigned long heard;} distributed_round_trips[4];
static int game_connection(void){return connection;}
static int game_in_progress(void){return in_game;}
static int mcc_cache_tags_loaded(void){return mcc;}
static int network_connection_last_read_was_unreliable(void){return unreliable;}
static unsigned long system_milliseconds(void){return 1000;}
static void distributed_restore_checkpoint(void){resets++;}
static void distributed_note_client_clock(long m,long t){(void)m;(void)t;clock_notes++;}
static int distributed_machine_clock_fast(long m){(void)m;return 0;}
static void error(int level,char const *fmt,...){(void)level;(void)fmt;}
#define network_aim_assist_handles_message(m,s) 0
#define network_shield_effects_handles_message(m,s) 0
#define network_voice_handles_message(m,s) 0
#define map_share_handles_message(m,s) 0
#define network_map_vote_handles_message(m,s) 0
#define network_aim_assist_handle_message(m,p,s) ((void)0)
#define network_shield_effects_handle_message(m,p,s) ((void)0)
#define network_voice_handle_message(m,p,s) ((void)0)
#define map_share_handle_message(m,p,s,r) ((void)0)
#define network_map_vote_handle_message(m,p,s,r) ((void)0)
'''
    import re
    sizes = sorted(set(re.findall(r"\b(\w+_entry_size)\(", dispatch)))
    prefix += "\n".join(f"#define {name}(...) 1" for name in sizes) + "\n"
    code = prefix + kinds + structure(header, "distributed_message_header")
    code += r'''
static long distributed_sender_times[MAXIMUM_SENDERS];
static long distributed_received_times[MAXIMUM_SENDERS][NUMBER_OF_DISTRIBUTED_MESSAGES];
'''
    for name in ("distributed_advance_checkpoint", "distributed_message_epoch_valid",
                 "distributed_message_time_bounded", "distributed_message_stale"):
        code += function(source, name)
    code += dispatch
    code += r'''
static void send(long machine,byte type,unsigned long epoch,long time,byte count,word size){
 union {word alignment;byte bytes[128];} message;
 struct distributed_message_header h;
 assert(size<=sizeof(message));memset(&message,0,sizeof(message));memset(&h,0,sizeof(h));
 h.type=type;h.count=count;h.checkpoint_epoch=epoch;h.game_time=time;
 memcpy(message.bytes,&h,sizeof(h));network_distributed_handle_message(machine,(word *)message.bytes,size);
}
static void batch(byte type,unsigned long epoch,long time){
 union {word alignment;byte bytes[128];} message;
 struct distributed_message_header outer,inner;word length;
 memset(&message,0,sizeof(message));memset(&outer,0,sizeof(outer));memset(&inner,0,sizeof(inner));
 outer.type=_distributed_message_batch;outer.checkpoint_epoch=epoch;outer.game_time=time;
 inner.type=type;inner.checkpoint_epoch=epoch;inner.game_time=time;
 length=sizeof(inner)-sizeof(message_header);
 memcpy(message.bytes,&outer,sizeof(outer));memcpy(message.bytes+sizeof(outer),&length,sizeof(length));
 memcpy(message.bytes+sizeof(outer)+sizeof(length),(byte *)&inner+sizeof(message_header),length);
 network_distributed_handle_message(NONE,(word *)message.bytes,sizeof(outer)+sizeof(length)+length);
}
static void setup(void){
 memset(distributed_sender_times,255,sizeof(distributed_sender_times));
 memset(distributed_received_times,255,sizeof(distributed_received_times));
}
'''
    return code


@pytest.mark.parametrize("case", ["barrier", "late_packets", "ready", "batch", "clock", "rollover"])
def test_epoch_dispatch(tmp_path, case):
    cases = {
        "barrier": r'''
 int i;unsigned long epoch;
 for(i=0;i<sizeof(struct distributed_message_header);i++)
  send(NONE,_distributed_message_checkpoint,1,100,0,i);
 assert(!resets);
 unreliable=1;send(NONE,_distributed_message_checkpoint,1,100,0,12);assert(!resets);unreliable=0;
 send(NONE,_distributed_message_checkpoint,0,100,0,12);
 send(NONE,_distributed_message_checkpoint,1,100,1,12);
 send(NONE,_distributed_message_checkpoint,1,100,0,13);
 assert(!resets);
 connection=2;distributed_handling_stream_message=1;
 send(0,_distributed_message_checkpoint,1,100,0,12);assert(!resets);
 connection=1;distributed_handling_stream_message=0;
 send(NONE,_distributed_message_checkpoint,1,100,0,12);
 assert(resets==1 && distributed_checkpoint_epoch==1);
 assert(!distributed_identity_sent && !distributed_build_sent);
 send(NONE,_distributed_message_checkpoint,1,101,0,12);assert(resets==1);
 send(NONE,_distributed_message_checkpoint,2,101,0,12);assert(resets==2);
 for(epoch=0;epoch<3;epoch++)send(NONE,_distributed_message_checkpoint,epoch,102,0,12);
 assert(resets==2 && distributed_checkpoint_epoch==2);
''',
        "late_packets": r'''
 int i;byte types[]={_distributed_message_damage_events,_distributed_message_object_changes,
 _distributed_message_unit_states,_distributed_message_inventories,_distributed_message_object_states,
 _distributed_message_actor_states,_distributed_message_damage_animations,_distributed_message_coop_events,
 _distributed_message_mcc_grenades,_distributed_message_mcc_object_names};
 send(NONE,_distributed_message_checkpoint,1,100,0,12);
 send(NONE,_distributed_message_objects_synchronized,1,100,0,12);
 for(i=0;i<sizeof(types);i++)send(NONE,types[i],1,100,1,13);
 send(NONE,_distributed_message_checkpoint,2,100,0,12); /* Same host tick, new world. */
 send(NONE,_distributed_message_objects_synchronized,2,100,0,12);
 for(i=0;i<sizeof(types);i++){
  assert(delivered[types[i]]==1);
  send(NONE,types[i],1,101,1,13); /* Old world, newer timestamp. */
  unreliable=1;send(NONE,types[i],1,100,1,13);unreliable=0;
  assert(delivered[types[i]]==1);
  send(NONE,types[i],2,101,1,13);assert(delivered[types[i]]==2);
 }
''',
        "ready": r'''
 connection=2;distributed_checkpoint_epoch=4;
 send(0,_distributed_message_client_ready,0,100,0,12);assert(!delivered[_distributed_message_client_ready]);
 distributed_handling_stream_message=1;
 send(0,_distributed_message_client_ready,0,100,0,12);
 send(0,_distributed_message_client_ready,1,100,1,12);
 assert(delivered[_distributed_message_client_ready]==2 && !resets);
 assert(!clock_notes && distributed_sender_times[0]==NONE);
 send(0,_distributed_message_client_ready,4,100,2,12);
 send(0,_distributed_message_client_ready,4,100,0,13);
 send(4,_distributed_message_client_ready,4,100,0,12);
 assert(delivered[_distributed_message_client_ready]==2);
 send(0,_distributed_message_hit_reports,3,101,1,13);
 send(0,_distributed_message_player_inputs,3,101,1,13);
 assert(!delivered[_distributed_message_hit_reports] && !delivered[_distributed_message_player_inputs]);
 send(0,_distributed_message_hit_reports,4,101,1,13);assert(delivered[_distributed_message_hit_reports]==1);
''',
        "batch": r'''
 batch(_distributed_message_checkpoint,2,100);assert(!resets);
 batch(_distributed_message_damage_events,2,100);assert(!delivered[_distributed_message_damage_events]);
 send(NONE,_distributed_message_checkpoint,2,100,0,12);
 send(NONE,_distributed_message_object_changes,2,100,1,13);
 assert(delivered[_distributed_message_object_changes]==1);
 unreliable=1;send(NONE,_distributed_message_inventories,2,100,1,13);unreliable=0;
 assert(!delivered[_distributed_message_inventories]);
 send(NONE,_distributed_message_inventories,2,100,1,13);
 assert(delivered[_distributed_message_inventories]==1);
 batch(_distributed_message_damage_events,1,101);assert(!delivered[_distributed_message_damage_events]);
 batch(_distributed_message_damage_events,2,101);assert(!delivered[_distributed_message_damage_events]);
 batch(_distributed_message_objects_synchronized,2,101);assert(!distributed_checkpoint_synchronized);
 unreliable=1;send(NONE,_distributed_message_objects_synchronized,2,101,0,12);
 assert(!distributed_checkpoint_synchronized);unreliable=0;
 send(NONE,_distributed_message_objects_synchronized,2,101,1,12);
 send(NONE,_distributed_message_objects_synchronized,2,101,0,13);assert(!distributed_checkpoint_synchronized);
 send(NONE,_distributed_message_objects_synchronized,2,101,0,12);
 unreliable=1;send(NONE,_distributed_message_object_changes,2,101,1,13);unreliable=0;
 assert(delivered[_distributed_message_object_changes]==1);
 batch(_distributed_message_damage_events,2,101);assert(delivered[_distributed_message_damage_events]==1);
 batch(_distributed_message_checkpoint,3,102);assert(resets==1 && distributed_checkpoint_epoch==2);
''',
        "clock": r'''
 send(NONE,_distributed_message_damage_events,3,LONG_MAX,0,12);
 assert(distributed_host_time==NONE && distributed_sender_times[HOST_SENDER]==NONE);
 send(NONE,_distributed_message_checkpoint,2,100,0,12);
 send(NONE,_distributed_message_objects_synchronized,2,100,0,12);
 send(NONE,_distributed_message_unit_states,2,100,0,12);
 send(NONE,_distributed_message_unit_states,1,LONG_MAX,0,12);
 assert(distributed_host_time==100 && distributed_sender_times[HOST_SENDER]==100);
 send(NONE,_distributed_message_unit_states,2,101,0,12);
 assert(delivered[_distributed_message_unit_states]==2);
 send(NONE,_distributed_message_unit_states,2,100,0,12);
 assert(delivered[_distributed_message_unit_states]==2); /* Ordinary stale checks retained. */
''',
        "rollover": r'''
 distributed_host_epoch=ULONG_MAX-1;distributed_advance_checkpoint();assert(distributed_checkpoint_epoch==ULONG_MAX);
 distributed_advance_checkpoint();assert(distributed_checkpoint_epoch==1);
 distributed_checkpoint_epoch=ULONG_MAX;
 send(NONE,_distributed_message_checkpoint,1,100,0,12);assert(resets==1 && distributed_checkpoint_epoch==1);
 send(NONE,_distributed_message_checkpoint,ULONG_MAX,101,0,12);assert(resets==1);
''',
    }
    run_c(tmp_path, generated() + "\nint main(void){setup();\n" + cases[case] + "\nreturn 0;}\n")


def test_outgoing_snapshot_boundary(tmp_path):
    source = (ROOT / "port/linux/game/network_distributed.c").read_text()
    header = (ROOT / "port/linux/game/network_distributed.h").read_text()
    code = generated() + enum_with(header, "_distributed_to_clients")
    code += structure(source, "distributed_packer") + r'''
static int wire_calls;
static long game_time_get(void){return 100;}
static void build_message_header(message_header *h,word size,int type,int version){*h=size;(void)type;(void)version;}
static short distributed_client_machines(long *ids,short n){assert(n==4);ids[0]=1;return 1;}
static void distributed_batch_add(short sender,void const *message,word size,word entry){
 const struct distributed_message_header *h=message;
 (void)sender;(void)entry;assert(size>=12 && h->checkpoint_epoch==distributed_checkpoint_epoch);wire_calls++;
}
#define network_distributed_server_send_to_all_reliably(m,s) distributed_batch_add(0,m,s,0)
#define network_distributed_client_send_reliably(m,s) distributed_batch_add(HOST_SENDER,m,s,0)
#define network_distributed_server_send_to_machine_reliably(i,m,s) distributed_batch_add(i,m,s,0)
'''
    for name in ("distributed_fill_header", "distributed_entry_size", "distributed_send",
                 "distributed_send_to_machine", "distributed_send_to_machine_reliably",
                 "distributed_packer_flush"):
        code += function(source, name)
    code += r'''
int main(void){
 struct distributed_message_header message;
 struct distributed_packer packer;
 setup();memset(&message,0,sizeof(message));memset(&packer,0,sizeof(packer));
 distributed_checkpoint_epoch=2;
 distributed_send(&message,_distributed_message_hit_reports,0,sizeof(message),_distributed_to_host_reliably);
 distributed_send(&message,_distributed_message_player_inputs,0,sizeof(message),_distributed_to_host);
 assert(!wire_calls && !distributed_statistics.sent && !message.checkpoint_epoch);
 packer.sender=HOST_SENDER;packer.type=_distributed_message_player_prediction;
 packer.count=1;packer.prefix_size=4;packer.size=8;
 distributed_packer_flush(&packer);
 assert(!wire_calls && !packer.count && packer.size==4);
 distributed_send(&message,_distributed_message_client_ready,0,sizeof(message),_distributed_to_host_reliably);
 assert(wire_calls==1 && message.checkpoint_epoch==2);
 distributed_identity_sent=distributed_build_sent=TRUE;
 send(NONE,_distributed_message_objects_synchronized,2,100,0,12);
 assert(distributed_checkpoint_synchronized && !distributed_identity_sent && !distributed_build_sent);
 distributed_send(&message,_distributed_message_hit_reports,0,sizeof(message),_distributed_to_host_reliably);
 packer.count=1;packer.size=8;distributed_packer_flush(&packer);
 assert(wire_calls==3 && !packer.count && packer.size==4);
 connection=2;distributed_checkpoint_synchronized=FALSE;
 distributed_send_to_machine(1,&message,_distributed_message_unit_states,0,sizeof(message));
 distributed_send_to_machine_reliably(1,&message,_distributed_message_object_changes,0,sizeof(message));
 distributed_send(&message,_distributed_message_damage_events,0,sizeof(message),_distributed_to_clients);
 assert(wire_calls==6);
 connection=1;in_game=0;
 distributed_send(&message,_distributed_message_chat_up,0,sizeof(message),_distributed_to_host_reliably);
 assert(wire_calls==7);return 0;
}
'''
    run_c(tmp_path, code)
