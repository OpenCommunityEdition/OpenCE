#include <stdio.h>
#include <string.h>
#include <math.h>
typedef unsigned char byte, boolean;
typedef unsigned short word;
typedef float real;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CHECK(c) do {if(!(c)){fprintf(stderr,"line %d: %s\n",__LINE__,#c);return 1;}}while(0)
#define csmemset memset
#define csmemcpy memcpy
#define DATUM_INDEX_TO_ABSOLUTE_INDEX(i) ((unsigned long)(i)&65535)
#define MAXIMUM_TRACKED_OBJECTS 16
#define MAXIMUM_TRACKED_PLAYERS 4
#define MAXIMUM_LOCAL_PLAYERS 1
#define MAXIMUM_GRENADE_THROWS 8
#define TICKS_PER_SECOND 30
#define DATAGRAM_ENTRIES(t) 64
#define _object_mask_unit 3
#define _game_connection_network_server 1
#define _game_connection_network_client 2
#define _distributed_message_mcc_grenades 83
#define _distributed_message_mcc_campaign 84
#define MCC_CAMPAIGN_SNAPSHOT_BYTES 752
#define _distributed_to_clients 0
struct distributed_message_header {word header;byte type,count;long time;};
struct object_iterator {long index;int done;};
struct unit_datum {struct {long player_index;} unit;};
struct player_datum {long unit;};
static long now=3;
static int active=1,connection=1,types=4,sends;
static long live_unit=(long)0xe0010001UL;
static short counts[4]={1,2,3,4};
static struct unit_datum unit={{0}};
static struct player_datum player;
static byte sent[1024];
static short sent_count;
static word sent_size;
static int known=1;
static boolean local=0;
static real round_trip=6;
static int campaign_sends,campaign_restores;
static byte campaign_state[MCC_CAMPAIGN_SNAPSHOT_BYTES];
boolean mcc_cache_tags_loaded(void) { return active; }
void mcc_campaign_snapshot(void *out) { memcpy(out,campaign_state,sizeof(campaign_state)); }
int mcc_campaign_restore(void const *in,unsigned long bytes) {
    if(bytes!=sizeof(campaign_state))return 0;
    memcpy(campaign_state,in,bytes);campaign_restores++;return 1;
}
short mcc_grenades_get(long index,short type) {return index==live_unit&&type>=0&&type<types?counts[type]:0;}
void mcc_grenades_set(long index,short type,short count) {if(index==live_unit&&type>=0&&type<types)counts[type]=count;}
boolean mcc_grenades_active(void){return active;}
short mcc_grenades_type_count(void){return (short)types;}
long game_time_get(void){return now;}
short game_connection(void){return (short)connection;}
void object_iterator_new(struct object_iterator *it,long mask,byte flags){(void)mask;(void)flags;it->done=0;it->index=live_unit;}
void *object_iterator_next(struct object_iterator *it){if(it->done++)return NULL;return &unit;}
boolean distributed_object_index_valid(long index){return ((unsigned long)index>>16)!=0 && index!=NONE;}
boolean network_objects_client_has(long index){return known&&index==live_unit;}
void *object_try_and_get_and_verify_type(long index,long mask){(void)mask;return index==live_unit?&unit:NULL;}
boolean distributed_player_is_local(long index){return index==0&&local;}
real distributed_own_round_trip_ticks(void){return round_trip;}
long local_player_get_player_index(short index){return index==0?0:NONE;}
struct player_datum *player_try_and_get(long index){return index==0?&player:NULL;}
long distributed_living_unit(struct player_datum const *p){return p?p->unit:NONE;}
void distributed_send(void *message,byte type,short count,word size,short destination){
    if(type==84) { if(count==1&&size==sizeof(struct distributed_message_header)+MCC_CAMPAIGN_SNAPSHOT_BYTES)campaign_sends++;return; }
    (void)destination;if(type!=83||size>sizeof(sent))return;
    memcpy(sent,message,size);sent_count=count;sent_size=size;sends++;
}
/* MCC_INVENTORY_IMPLEMENTATION */

struct tag_block {long count;void *address;};
struct game_globals_grenade {struct {long index;} projectile;};
struct game_globals {struct tag_block grenades;};
struct damage_reach {real distance,ticks,carried_ticks;};
static struct {long throw_times[MAXIMUM_GRENADE_THROWS];short throw_types[MAXIMUM_GRENADE_THROWS];} damage_players[MAXIMUM_TRACKED_PLAYERS];
static struct game_globals globals;
static struct game_globals_grenade grenades[4];
static int paid;
#define TAG_BLOCK_GET_ELEMENT(block,index,type) ((type *)(block)->address+(index))
struct game_globals *scenario_get_game_globals(void){return &globals;}
short distributed_grenade_throw(short p,short type){short i;for(i=0;i<8;i++)if(damage_players[p].throw_times[i]!=NONE&&damage_players[p].throw_types[i]==type)return i;return NONE;}
boolean distributed_explosion_this_tick(short p,short type,void *point){(void)p;(void)type;(void)point;return paid;}
real distributed_source_deals(long projectile,long damage,boolean grenade,byte *kinds,struct damage_reach *reach){
    (void)grenade;*kinds=projectile==damage?2:0;reach->distance=8;reach->ticks=3;reach->carried_ticks=1;return projectile==damage?4:0;
}
void distributed_reach_combine(struct damage_reach *reach,struct damage_reach const *other){reach->distance=MAX(reach->distance,other->distance);}
/* MCC_DAMAGE_IMPLEMENTATION */

int main(int argc,char **argv){
    struct mcc_network_inventory packet;
    int i;
    CHECK(argc==2);player.unit=live_unit;mcc_network_new_game();
    memset(&packet,0,sizeof(packet));packet.unit=live_unit;packet.counts[0]=7;packet.counts[1]=8;
    if(!strcmp(argv[1],"host")){
        mcc_network_host_tick();CHECK(sends==1&&sent_count==1&&sent_size==16);
        CHECK(((struct mcc_network_inventory *)(sent+8))->counts[0]==3);
        now=6;mcc_network_host_tick();CHECK(sends==1);
        counts[2]=2;mcc_network_host_tick();CHECK(sends==2);
        now=36;mcc_network_host_tick();CHECK(sends==3);
        active=0;now=66;mcc_network_host_tick();CHECK(sends==3);
    }else if(!strcmp(argv[1],"receive")){
        connection=2;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7&&counts[3]==8&&counts[0]==1&&counts[1]==2);
        packet.counts[0]=128;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7);
        packet.counts[0]=4;packet.unit^=0x10000;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7);
        packet.unit=live_unit;packet.reserved[0]=1;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7);
        packet.reserved[0]=0;known=0;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7);
    }else if(!strcmp(argv[1],"prediction")){
        connection=2;local=1;mcc_network_client_tick();
        now=4;counts[2]=2;mcc_network_client_tick();
        mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==2&&counts[3]==8);
        now=16;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7);
        packet.counts[0]=1;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==1);
        live_unit^=0x10000;packet.unit=live_unit;packet.counts[0]=6;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==6);
    }else if(!strcmp(argv[1],"rewind_host")){
        now=300;mcc_network_host_tick();CHECK(sends==1);
        now=303;mcc_network_host_tick();CHECK(sends==1);
        now=30;mcc_network_host_tick();CHECK(sends==2);
        now=33;mcc_network_host_tick();CHECK(sends==2);
    }else if(!strcmp(argv[1],"rewind_client")){
        connection=2;local=1;now=300;mcc_network_client_tick();
        now=301;counts[2]=2;mcc_network_client_tick();
        mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==2);
        now=30;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7);
        now=31;counts[2]=6;mcc_network_client_tick();
        mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==6);
        now=43;mcc_network_handle_inventories(&packet,1);CHECK(counts[2]==7);
    }else if(!strcmp(argv[1],"damage")){
        byte kinds=0;struct damage_reach reach={0,0,0};short type=NONE;real rate=0;
        globals.grenades.count=4;globals.grenades.address=grenades;for(i=0;i<4;i++)grenades[i].projectile.index=100+i;
        for(i=0;i<8;i++)damage_players[0].throw_times[i]=NONE;
        mcc_network_note_extra_grenade(live_unit,4);CHECK(distributed_grenade_throw(0,4)==NONE);
        mcc_network_note_extra_grenade(live_unit,2);CHECK(distributed_grenade_throw(0,2)!=NONE);
        mcc_network_extra_grenade_damage(0,102,&kinds,&reach,&type,&rate);CHECK(rate==4&&type==2&&kinds==2&&reach.distance==8);
        for(i=0;i<8;i++)damage_players[0].throw_times[i]=NONE;
        rate=0;type=NONE;kinds=0;mcc_network_extra_grenade_damage(0,102,&kinds,&reach,&type,&rate);CHECK(rate==0&&type==NONE);
        paid=1;mcc_network_extra_grenade_damage(0,102,&kinds,&reach,&type,&rate);CHECK(rate==4&&type==2);
        connection=2;mcc_network_note_extra_grenade(live_unit,3);CHECK(distributed_grenade_throw(0,3)==NONE);
    }else if(!strcmp(argv[1],"campaign")){
        types=2;mcc_network_host_tick();CHECK(campaign_sends==1&&sends==0);
        mcc_network_handle_campaign(campaign_state,1);CHECK(campaign_restores==0);
        connection=2;mcc_network_handle_campaign(campaign_state,0);mcc_network_handle_campaign(campaign_state,2);
        CHECK(campaign_restores==0);mcc_network_handle_campaign(campaign_state,1);CHECK(campaign_restores==1);
        active=0;mcc_network_handle_campaign(campaign_state,1);CHECK(campaign_restores==1);
        connection=1;mcc_network_host_tick();CHECK(campaign_sends==1);
    }else if(!strcmp(argv[1],"legacy")){
        active=0;mcc_network_host_tick();connection=2;mcc_network_client_tick();mcc_network_handle_inventories(&packet,1);
        CHECK(sends==0&&campaign_sends==0&&counts[2]==3&&counts[3]==4&&counts[0]==1&&counts[1]==2);
    }else return 2;
    return 0;
}
