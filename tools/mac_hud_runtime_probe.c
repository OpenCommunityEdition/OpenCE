/* Compile the complete genuine messaging implementation so diagnostics can
   inspect its private records without a duplicate layout or replacement code. */
#include "../source/interface/hud_messaging.c"
#define HUD_CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"HUD FAIL %s line %d\n",#x,__LINE__);}}while(0)
int mac_hud_runtime_probe(long weapon)
{
    unsigned checks=0,failures=0,i,count=0;
    struct hud_messaging_datum_definition *datum=&hud_messaging_globals->message_data[0];
    struct hud_message_definition *found=NULL;
    HUD_CHECK(hud_msg_def==&hud_globals->messaging);
    for(i=0;i<NUMBER_OF_HUD_MESSAGES_PER_DATUM;++i)HUD_CHECK(!datum->messages[i].valid);
    hud_picked_up_weapon(0,weapon);
    hud_picked_up_ammunition(0,weapon,7);
    hud_picked_up_ammunition(0,weapon,5);
    for(i=0;i<NUMBER_OF_HUD_MESSAGES_PER_DATUM;++i){
        struct hud_message_definition *m=&datum->messages[i];
        if(m->valid){++count;HUD_CHECK(m->item_definition_index==weapon);HUD_CHECK(m->time==game_time_get());
            if(m->message_offset==1)found=m;}
    }
    HUD_CHECK(count==2);HUD_CHECK(found!=NULL);
    if(found)HUD_CHECK(found->quantity==12);
    HUD_CHECK(hud_messaging_globals->magic_number==3);
    printf("actual HUD messages: %u checks %u failures\n",checks,failures);
    return failures==0;
}
