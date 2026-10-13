#include "mcc_game_speed.h"
#include "game/game.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "scenario/scenario_definitions.h"
#include "network_distributed.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#undef memcpy
#undef strcpy
void *csmemcpy(void *to,void const *from,unsigned long size) {return memcpy(to,from,size);}
char *csstrcpy(char *to,char const *from) {return strcpy(to,from);}

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while (0)
static boolean active=TRUE;
static short connection=_game_connection_local;
static real native_speed=1.0f,sent_speed;
static unsigned long wall;
static unsigned messages;
static struct scenario scenario;
static struct hs_global globals[2];
boolean mcc_cache_tags_loaded(void) {return active;}
short game_connection(void) {return connection;}
boolean game_in_progress(void) {return TRUE;}
real game_time_get_speed(void) {return connection==_game_connection_network_client ? mcc_game_speed_client(1.0f) : native_speed;}
void game_time_set_speed(real speed) {native_speed=speed;}
unsigned long system_milliseconds(void) {return wall;}
struct scenario *global_scenario_get(void) {return &scenario;}
long csstrcasecmp(char const *a,char const *b) {return _stricmp(a,b);}
void error(short priority,char const *format,...) {(void)priority;(void)format;}
void distributed_send(void *message,byte type,short count,word size,short destination) {
    if(type!=_distributed_message_mcc_game_speed || count!=1 ||
       size!=sizeof(struct distributed_message_header)+4 || destination!=_distributed_to_clients_reliably) return;
    memcpy(&sent_speed,(byte *)message+sizeof(struct distributed_message_header),4);messages++;
}
int main(void) {
    real value;union {uint32_t bits;real number;} bad;
    scenario.hs_globals.address=globals;scenario.hs_globals.count=2;
    strcpy(globals[0].name,"game_speed_value");globals[0].type=_hs_type_real;
    strcpy(globals[1].name,"other");globals[1].type=_hs_type_real;
    CHECK(mcc_game_speed_global(0) && !mcc_game_speed_global(1) && !mcc_game_speed_global(2) && !mcc_game_speed_global(0x8000));
    mcc_game_speed_set(0.5f);CHECK(native_speed==0.5f);
    mcc_game_speed_set(-1);CHECK(native_speed==0.5f);
    bad.bits=0x7fc00000;mcc_game_speed_set(bad.number);CHECK(native_speed==0.5f);
    bad.bits=0x7f800000;mcc_game_speed_set(bad.number);CHECK(native_speed==0.5f);
    connection=_game_connection_network_server;native_speed=1;wall=1000;mcc_game_speed_reset();
    CHECK(mcc_game_speed_clock(wall)==1000);
    wall+=1000;CHECK(mcc_game_speed_clock(wall)==2000);
    mcc_game_speed_set(2);wall+=1000;CHECK(mcc_game_speed_clock(wall)==4000);
    mcc_game_speed_set(0.5f);wall+=1000;CHECK(mcc_game_speed_clock(wall)==4500);
    mcc_game_speed_frame();CHECK(messages==1 && sent_speed==0.5f);
    mcc_game_speed_frame();CHECK(messages==1);
    mcc_game_speed_set(0);mcc_game_speed_frame();CHECK(messages==2 && sent_speed==0);
    wall+=250;mcc_game_speed_frame();CHECK(messages==3 && sent_speed==0);
    mcc_game_speed_set(1);mcc_game_speed_frame();CHECK(messages==4 && sent_speed==1);
    connection=_game_connection_network_client;
    value=0.25f;mcc_game_speed_receive(&value,1);CHECK(game_time_get_speed()==0.25f);
    mcc_game_speed_set(10);CHECK(native_speed==1 && game_time_get_speed()==0.25f);
    mcc_game_speed_receive(&bad.number,1);CHECK(game_time_get_speed()==0.25f);
    value=9;mcc_game_speed_receive(&value,0);CHECK(game_time_get_speed()==0.25f);
    mcc_game_speed_frame();CHECK(messages==4);
    active=FALSE;CHECK(game_time_get_speed()==1 && !mcc_game_speed_global(0));
    mcc_game_speed_set(3);CHECK(native_speed==1);
    CHECK(mcc_game_speed_clock(12345)==12345);
    mcc_game_speed_reset();active=TRUE;CHECK(game_time_get_speed()==1);
    connection=_game_connection_local;value=2;mcc_game_speed_receive(&value,1);CHECK(native_speed==1);
    return 0;
}
