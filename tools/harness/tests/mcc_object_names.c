#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mcc_object_names.h"
typedef unsigned char byte,boolean;
#define TRUE 1
typedef unsigned short word;
#define NONE (-1)
#define FALSE 0
#define MAXIMUM_OBJECT_NAMES_PER_SCENARIO 512
#define NUMBER_OF_OBJECT_TYPES 12
#define MIN(a,b) ((a)<(b)?(a):(b))
#define VALID_INDEX(i,n) ((i)>=0 && (i)<(n))
#define TEST_FLAG(v,b) ((v)&(1u<<(b)))
#define _object_mask_biped 1
#define _object_mask_vehicle 2
#define _object_mask_weapon 4
#define _object_mask_equipment 8
#define _error_silent 0
#define match_assert(a,b,c) do {if(!(c))abort();} while(0)
#define CHECK(c) do {if(!(c)){fprintf(stderr,"line %d: %s\n",__LINE__,#c);return 1;}}while(0)
enum {_game_connection_network_server=1,_game_connection_network_client=2,_distributed_message_mcc_object_names=89,_distributed_to_clients=0};
struct scenario_object_name {char name[32];short runtime_object_type;};
static struct scenario_object_name definitions[32767];
struct scenario {struct {long count;void *address;} object_names;};
struct object_datum {long definition_index;struct {short name_index;long first_child_object_index,next_object_index;} object;};
struct distributed_message_header {long tick;short type,count;};
static struct scenario scenario;
static struct object_datum objects[8];
static long legacy[512],*object_name_list=legacy;
static int loaded=1,connection=1,bsp=0,sends,created,deleted;
static long now=15;
static byte packets[64][72];
#define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)(b)->address+(i))
static struct scenario *global_scenario_get(void){return &scenario;}
static int mcc_cache_tags_loaded(void){return loaded;}
static long game_time_get(void){return now;}
static short game_connection(void){return (short)connection;}
static short global_structure_bsp_index_get(void){return (short)bsp;}
static struct object_datum *object_get(long i){if(i<0||i>=8)abort();return objects+i;}
static long player_index_from_unit_index(long i){(void)i;return NONE;}
static char const *tag_get_name(long id){(void)id;return "object";}
static void error(int level,char const *fmt,...){(void)level;(void)fmt;}
static void distributed_send(void *msg,short type,short count,word size,short dest){
    if(type!=89||count!=1||size!=80||dest||sends>=64)abort();
    memcpy(packets[sends++],(byte *)msg+sizeof(struct distributed_message_header),72);
}
long object_index_from_name_index(short name);
static void object_name_list_new(long object,short name);
static void object_name_list_delete(long object);
static long object_new_by_name(short name){long id=++created;object_name_list_new(id,name);return id;}
static void object_delete(long object){deleted++;object_name_list_delete(object);}
/* IMPLEMENTATION */
int main(int argc,char **argv)
{
    long i;
    struct mcc_name_chunk bad;
    CHECK(argc==2);
    scenario.object_names.count=640;scenario.object_names.address=definitions;
    for(i=0;i<32767;i++)definitions[i].runtime_object_type=6;
    for(i=0;i<8;i++)objects[i].object.first_child_object_index=NONE;
    object_name_list_clear();mcc_object_names_network_reset();
    CHECK(mcc_object_names_entry_size()==72);
    if(!strcmp(argv[1],"legacy")){
        loaded=0;scenario.object_names.count=512;
        object_name_list_new(1,511);CHECK(object_index_from_name_index(511)==1);
        object_set_object_index_for_name_index(512,2);CHECK(object_index_from_name_index(512)==NONE);
        object_name_list_delete(1);CHECK(object_name_list_lookup(511)==NONE);
        loaded=1;CHECK(!mcc_object_names_active());CHECK(!mcc_object_names_snapshot(NULL));
    }else if(!strcmp(argv[1],"boundary")||!strcmp(argv[1],"maximum")){
        short last=!strcmp(argv[1],"maximum")?32766:639;
        scenario.object_names.count=(long)last+1;
        object_name_list_new(1,511);object_name_list_new(2,512);object_claim_scenario_name(3,last);
        CHECK(object_name_list_lookup(511)==1&&object_name_list_lookup(512)==2&&object_index_from_name_index(last)==3);
        object_claim_scenario_name(4,last);CHECK(object_index_from_name_index(last)==3);
        CHECK(object_index_from_name_index(-1)==NONE&&object_index_from_name_index((short)(last+1))==NONE);
        CHECK(legacy[511]==NONE);CHECK(objects[3].object.name_index==last);
        object_name_list_clear();CHECK(object_index_from_name_index(last)==NONE);
    }else if(!strcmp(argv[1],"aliases")){
        object_name_list_new(1,513);object_set_object_index_for_name_index(638,1);
        object_name_list_delete(1);CHECK(object_index_from_name_index(513)==NONE&&object_index_from_name_index(638)==NONE);
        object_name_list_new(2,513);CHECK(object_index_from_name_index(513)==2);
    }else{
        scenario.object_names.count=32767;
        object_name_list_new(1,512);object_name_list_new(2,32766);
        mcc_object_names_host_tick();CHECK(sends==64);
        object_name_list_clear();connection=2;
        if(!strcmp(argv[1],"network")){
            for(i=63;i>=0;i--)mcc_object_names_receive(packets[i],1,15);
            CHECK(created==0);
            for(i=0;i<64;i++)mcc_object_names_receive(packets[i],1,15);
            CHECK(created==0);
            for(i=0;i<64;i++)mcc_object_names_receive(packets[i],1,30);
            CHECK(created==2&&object_index_from_name_index(32766)!=NONE);
            memset(packets[63]+8,0,64);
            mcc_object_names_receive(packets[63],1,45);CHECK(!deleted);
            mcc_object_names_receive(packets[63],1,44);CHECK(!deleted);
            mcc_object_names_receive(packets[63],1,60);CHECK(deleted==1);
        }else if(!strcmp(argv[1],"invalid")){
            memcpy(&bad,packets[63],72);bad.first=32767;
            mcc_object_names_receive(&bad,1,15);bad.first=-512;mcc_object_names_receive(&bad,1,30);
            memcpy(&bad,packets[63],72);bad.present[63]|=128;mcc_object_names_receive(&bad,1,45);
            mcc_object_names_receive(packets[63],0,60);mcc_object_names_receive(packets[63],2,75);
            mcc_object_names_receive(packets[63],1,90);CHECK(!created);
            mcc_object_names_receive(packets[63],1,105);CHECK(created==1);
        }else{
            mcc_object_names_receive(packets[63],1,15);bsp=1;
            mcc_object_names_receive(packets[63],1,30);bsp=0;
            mcc_object_names_receive(packets[63],1,45);CHECK(!created);
            mcc_object_names_receive(packets[63],1,60);CHECK(created==1);
        }
    }
    return 0;
}
