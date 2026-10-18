/* Whole engine players.c/data.c integration: player records only. No spawned
   biped, tag loading, simulation tick or gameplay service is faked here. */
#include "cseries.h"
#include "memory/data.h"
#include "game/players.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,failures;
static void check(int pass,const char *name)
{
    ++checks;
    if(!pass) { ++failures;fprintf(stderr,"FAIL: %s\n",name); }
}
static void record_checks(long index,short local)
{
    struct player_datum *p;
    check(index!=NONE,"player_new succeeded");
    if(index==NONE)return;
    p=player_get(index);
    check(p!=NULL,"real datum lookup");
    if(!p)return;
    check(p->local_player_index==local,"local player index");
    check(p->unit_index==NONE && p->dead_unit_index==NONE,"record has no biped");
    check(p->squad_index==NONE && p->cluster_index==NONE,"unplaced record indices");
    check(p->aim_assist_unit_index==NONE,"aim assist sentinel");
    check(p->speed_multiplier==1.f,"speed multiplier");
    check(p->team_index==1,"initial team");
    check(p->action_result==_player_action_result_reload && p->action_object_index==NONE,"initial action");
    check(p->quit_out_of_game_time==NONE && !p->quit_out_of_game,"initial quit state");
}
int main(void)
{
    struct network_player network;
    struct player_datum *p;
    long *machine;
    long first,second,explicit_index,reused;
    long size;
    unsigned i;
    typedef char engine_wchar_must_be_utf16[sizeof(wchar_t)==2?1:-1];
    (void)sizeof(engine_wchar_must_be_utf16);
    size=data_allocation_size(4,sizeof(struct player_datum));
    player_data=(struct data_array *)malloc((size_t)size);
    if(!player_data)return 2;
    data_initialize(player_data,"",4,sizeof(struct player_datum));
    data_make_valid(player_data);
    check((unsigned short)player_data->next_identifier==0x8000,"empty name seed padding");
    data_initialize(player_data,"p",4,sizeof(struct player_datum));
    data_make_valid(player_data);
    check((unsigned short)player_data->next_identifier==0x8070,"one character name seed padding");
    data_initialize(player_data,"players",4,sizeof(struct player_datum));
    data_make_valid(player_data);
    /* Exercise the real machine list accessor. Full game initialization also
       clears the other machine lists and numerous unrelated subsystems. */
    machine=machine_get_player_list(0);
    for(i=0;i<MAXIMUM_LOCAL_PLAYERS;++i)machine[i]=NONE;
    check((unsigned short)player_data->next_identifier==0xec70,
        "little-endian datum seed from first two name bytes");
    first=player_new(0,NONE,0,NULL);
    record_checks(first,0);
    if(first!=NONE) {
        p=player_get(first);
        check(p->name[0]==0 && p->name[11]==0,"empty UTF16 player name");
        check(machine[0]==first,"machine player registration");
    }
    memset(&network,0,sizeof(network));
    network.name[0]='G';network.name[1]='4';network.name[2]=0x03a9;
    network.primary_color_index=7;network.icon_index=3;
    network.machine_index=0;network.controller_index=1;network.team_index=1;
    second=player_new(0,NONE,1,&network);
    record_checks(second,1);
    if(second!=NONE) {
        p=player_get(second);
        check(p->name[0]=='G' && p->name[1]=='4' && p->name[2]==0x03a9 && p->name[3]==0 && p->name[11]==0,
            "UTF16 player name copied without Darwin wchar32 ABI");
        check(!memcmp(&p->network_player_data,&network,sizeof(network)),"network metadata copy");
        check(machine[1]==second,"second machine registration");
    }
    explicit_index=player_new(0,0x12340003,2,NULL);
    record_checks(explicit_index,2);
    check(explicit_index==0x12340003,"requested salted datum index");
    check(player_data->actual_count==3,"three allocated player records");
    if(first!=NONE) {
        machine_remove_player(first);datum_delete(player_data,first);
        check(machine[0]==NONE,"machine removal");
        check(player_try_and_get(first)==NULL,"deleted handle rejected");
        reused=player_new(0,NONE,0,NULL);
        record_checks(reused,0);
        check(reused!=NONE && reused!=first && (reused&0xffff)==(first&0xffff),"reused slot has new salt");
        check(player_try_and_get(first)==NULL,"old salt rejected after reuse");
        if(reused!=NONE) { machine_remove_player(reused);datum_delete(player_data,reused); }
    }
    if(second!=NONE) { machine_remove_player(second);datum_delete(player_data,second); }
    if(explicit_index!=NONE) { machine_remove_player(explicit_index);datum_delete(player_data,explicit_index); }
    check(player_data->actual_count==0,"all records deleted");
    for(i=0;i<MAXIMUM_LOCAL_PLAYERS;++i)check(machine[i]==NONE,"machine list empty");
    printf("actual engine player records: first=%08lx second=%08lx explicit=%08lx checks=%u failures=%u; no biped spawned\n",
        (unsigned long)first,(unsigned long)second,(unsigned long)explicit_index,checks,failures);
    free(player_data);player_data=NULL;
    return failures?1:0;
}
