/* The existing fake engine is shared; packet encoding/decoding below is
production code, while socket delivery and unrelated dispatch are boundaries. */
#define main surface_source_cases_main
#include "coop_surface_sync.c"
#undef main
#include <stddef.h>
typedef word message_header;
#define DATAGRAM_MAXIMUM_SIZE 1200
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 1
#define HOST_SENDER 1
#define EVENT_SENDS 3
#define csmemcpy memcpy
#define csmemmove memmove
#include "wire_types.inc"
#define BATCH_MESSAGE_OVERHEAD ((word)(sizeof(word)+sizeof(struct distributed_message_header)-sizeof(message_header)))
static struct distributed_batch distributed_batches[2];
static struct {int sent;} distributed_statistics;
static boolean distributed_handling_batch;
static union {long alignment;byte data[DATAGRAM_MAXIMUM_SIZE];} wire_packet;
static word wire_size;
static int wire_deliveries;
static unsigned long delivered_mask;
static short distributed_client_machines(long *indices,short maximum){CHECK(maximum>0,"client capacity");indices[0]=0;return 1;}
static void capture_packet(void const *message,word size){CHECK(size<=sizeof(wire_packet.data),"oversize packet");memcpy(wire_packet.data,message,size);wire_size=size;}
static void network_distributed_server_send_to_machine(short machine,void const *message,word size){CHECK(machine==0,"wrong recipient");capture_packet(message,size);}
static void network_distributed_client_send(void const *message,word size){capture_packet(message,size);}
static void network_distributed_server_send_to_all_reliably(void const *message,word size){capture_packet(message,size);}
static void network_distributed_client_send_reliably(void const *message,word size){capture_packet(message,size);}
static void network_distributed_handle_message(long machine_index,word const *message,word size);
#include "wire.inc"
static void network_distributed_handle_message(long machine_index,word const *message,word size)
{
 struct distributed_message_header header;
 CHECK(machine_index==NONE&&is_client,"test packet must arrive on client's host connection");
 CHECK(size>=sizeof(header),"short decoded header");memcpy(&header,message,sizeof(header));
 if(header.type==_distributed_message_batch){surface_decode_batch(machine_index,message,size);return;}
 CHECK(header.type==_distributed_message_coop_events&&header.count==1,"wrong decoded event header");
 CHECK(distributed_entry_size(header.count,size)==network_coop_event_entry_size(),"wire entry size changed");
 struct distributed_coop_event const *event=(void const *)((byte const *)message+sizeof(header));
 delivered_mask=(unsigned long)event->object_index&0xffffffffUL;wire_deliveries++;
 network_coop_handle_events(event,header.count);
}
int main(int argc,char **argv)
{
 boolean abi32=argc>1&&!strcmp(argv[1],"wire32");
 if(abi32){
  CHECK(sizeof(long)==4&&sizeof(struct distributed_message_header)==8,"not the engine's32-bit wire ABI");
  CHECK(sizeof(struct distributed_coop_event)==40,"surface event wire size");
  CHECK(offsetof(struct distributed_coop_event,object_index)==12,"surface mask wire offset");
 }
 for(int pattern=0;pattern<2;pattern++){
  setup();bsp.breakable_surfaces.count=32;wire_size=0;wire_deliveries=0;delivered_mask=0;
  memset(distributed_batches,0,sizeof(distributed_batches));memset(client.breakable_surface_flags[0],0,32);
  if(pattern==0)for(int index=0;index<31;index++)set_extant(&host,index,FALSE);
  host_send_broken_surfaces();host_send_events();distributed_batch_flush(0);
  CHECK(wire_size>0,"actual sender did not emit a packet");
  if(abi32){
   byte want[2][4]={{0,0,0,0x80},{0xff,0xff,0xff,0xff}};
   CHECK(wire_size==56,"32-bit packet length");
   CHECK(!memcmp(wire_packet.data+28,want[pattern],4),"serialized mask lost high/all bits");
  }
  select_client();network_distributed_handle_message(NONE,(word const *)wire_packet.data,wire_size);
  CHECK(wire_deliveries==1,"batch decoder lost or duplicated event");
  CHECK(delivered_mask==(pattern?0xffffffffUL:0x80000000UL),"mask changed through batch bytes");
  for(int index=0;index<32;index++)CHECK(breakable_surface_extant(index)==(pattern||index==31),"wire mask did not control surface%d",index);
 }
 puts(abi32?"PASS: 32-bit event ABI and mask wire roundtrip":"PASS: native-width batch byte roundtrip (not32-bit ABI proof)");
 return 0;
}
