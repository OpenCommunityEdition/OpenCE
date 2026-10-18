#include "engine_animation_selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned long mac_engine_service_errors;
static unsigned checks,failures;
#define CHECK(x) do { ++checks; if(!(x)) { ++failures;printf("FAIL line %d: %s\n",__LINE__,#x); } } while(0)
int main(int argc,char **argv)
{
    FILE *file;
    unsigned char *bytes;
    long length;
    struct mac_retail_cache cache;
    struct mac_retail_animation_graph graph;
    struct mac_retail_skeleton skeleton;
    struct mac_engine_animation_selection selection;
    struct mac_engine_seat_result seat;
    const char *error=NULL;
    uint32_t biped,model,handle=UINT32_C(0xe211009d),i;
    char upper[32];
    unsigned n;
    int index,idle,front;
    if(argc<2) { fprintf(stderr,"usage: %s bloodgulch.map [animation_handle]\n",argv[0]);return 2; }
    if(argc>2)handle=(uint32_t)strtoul(argv[2],NULL,0);
    file=fopen(argv[1],"rb");if(!file) { perror(argv[1]);return 2; }
    if(fseek(file,0,SEEK_END) || (length=ftell(file))<0 || length>0x30000000L || fseek(file,0,SEEK_SET)) { fclose(file);return 2; }
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes) { fclose(file);return 2; }
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length) { fclose(file);free(bytes);return 2; }
    fclose(file);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error) ||
        !mac_retail_animation_graph_decode(&cache,handle,&graph,&error)) {
        fprintf(stderr,"map: %s\n",error);free(bytes);return 1;
    }
    if(!mac_retail_multiplayer_model(&cache,&biped,&model,&error) ||
        !mac_retail_skeleton_decode(&cache,model,&skeleton,&error) ||
        !mac_engine_animation_selection_create(&graph,&skeleton,&selection,&error)) {
        fprintf(stderr,"selection: %s\n",error);mac_retail_animation_graph_free(&graph);free(bytes);return 1;
    }
    for(i=0;i<graph.animation_count;++i) {
        CHECK(mac_engine_animation_selection_find(&selection,graph.animations[i].name)==(int)i);
        memcpy(upper,graph.animations[i].name,32);
        for(n=0;n<32 && upper[n];++n)if(upper[n]>='a' && upper[n]<='z')upper[n]-='a'-'A';
        CHECK(mac_engine_animation_selection_find(&selection,upper)==(int)i);
    }
    idle=mac_engine_animation_selection_idle_slot();front=mac_engine_animation_selection_move_front_slot();
    CHECK(idle==0 && front==8);
    index=mac_engine_animation_selection_slot(&selection,"stand","pistol",idle);
    CHECK(index==146 && !strcmp(graph.animations[index].name,"stand pistol idle") && graph.animations[index].type==0);
    printf("stand pistol idle: %d\n",index);
    index=mac_engine_animation_selection_slot(&selection,"stand","pistol",front);
    CHECK(index==150 && !strcmp(graph.animations[index].name,"stand pistol move-front") && graph.animations[index].type==0);
    printf("stand pistol move-front: %d\n",index);
    index=mac_engine_animation_selection_slot(&selection,"stand","rifle",idle);
    CHECK(index==168 && !strcmp(graph.animations[index].name,"stand rifle idle") && graph.animations[index].type==0);
    printf("stand rifle idle: %d\n",index);
    index=mac_engine_animation_selection_slot(&selection,"STAND","RIFLE",front);
    CHECK(index==173 && !strcmp(graph.animations[index].name,"stand rifle move-front") && graph.animations[index].type==0);
    printf("stand rifle move-front: %d\n",index);
    CHECK(mac_engine_animation_selection_find(&selection,"no such animation")==-1);
    CHECK(mac_engine_animation_selection_slot(&selection,"no such seat","rifle",idle)==-1);
    CHECK(mac_engine_animation_selection_slot(&selection,"stand","no such class",idle)==-1);
    CHECK(mac_engine_animation_selection_slot(&selection,"stand","rifle",-1)==-1);
    CHECK(mac_engine_animation_selection_slot(&selection,"stand","rifle",999)==-1);
    CHECK(mac_engine_animation_selection_fits(&selection,168,19));
    CHECK(!mac_engine_animation_selection_fits(&selection,-1,19));
    CHECK(!mac_engine_animation_selection_fits(&selection,254,19));
    CHECK(!mac_engine_animation_selection_fits(&selection,168,18));
    CHECK(mac_engine_service_errors==1);
    CHECK(mac_engine_animation_diagnostic_seat(&selection,biped,"stand",&seat)==1);
    CHECK(seat.seat==4 && seat.base_seat==2 && seat.weapon_class==5 && seat.weapon_type==0);
    printf("actual unit_set_seat stand: seat %d, base %d, class %d, type %d, flags %u\n",seat.seat,seat.base_seat,seat.weapon_class,seat.weapon_type,seat.flags);
    CHECK(mac_engine_animation_diagnostic_seat(&selection,biped,"STAND",&seat)==1);
    CHECK(seat.seat==4 && seat.base_seat==2 && seat.weapon_class==5 && seat.weapon_type==0);
    CHECK(mac_engine_animation_diagnostic_seat(&selection,biped,"no such seat",&seat)==0);
    CHECK(seat.seat==-1 && seat.base_seat==-1 && seat.weapon_class==-1 && seat.weapon_type==-1);
    mac_engine_animation_selection_free(&selection);
    ++skeleton.count;
    CHECK(!mac_engine_animation_selection_create(&graph,&skeleton,&selection,&error));
    --skeleton.count;
    skeleton.checksum=1;
    CHECK(mac_engine_animation_selection_create(&graph,&skeleton,&selection,&error));
    CHECK(mac_engine_animation_selection_find(&selection,"stand rifle idle")==-1);
    mac_engine_animation_selection_free(&selection);
    CHECK(mac_engine_service_errors==1);
    printf("engine selection: %u checks, %u failures; %u names; expected engine errors %lu; actual diagnostic seat mutation, no playable biped\n",
        checks,failures,(unsigned)graph.animation_count,mac_engine_service_errors);
    mac_retail_animation_graph_free(&graph);free(bytes);return failures?1:0;
}
