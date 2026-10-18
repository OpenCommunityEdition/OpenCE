#include "engine_world.h"
#include "native_scenario_gameplay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct gameplay_reference { uint32_t group;char *name;int32_t name_length,index; };
struct gameplay_flag { float position[3],facing;int16_t type,team;int32_t unused[32]; };
struct gameplay_equipment { uint32_t flags;int16_t game_type[4],team,spawn_time;int32_t runtime_index,unused[11];
    float position[3],facing;struct gameplay_reference item_collection;int32_t unused2[12]; };
struct gameplay_starting_equipment { uint32_t flags;int16_t game_type[4];int32_t unused[12];
    struct gameplay_reference item_collection[6];int32_t unused2[12]; };

static unsigned checks,failures;
#define CHECK(x) do { ++checks;if(!(x)){if(failures<16)printf("FAIL line %d: %s\n",__LINE__,#x);++failures;} } while(0)
static unsigned char *read_file(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb");long n;unsigned char *p;
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END)||(n=ftell(f))<0||n>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    p=(unsigned char *)malloc((size_t)n);if(!p){fclose(f);return NULL;}
    if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return p;
}
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void put16(unsigned char *p,uint16_t v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void put32(unsigned char *p,uint32_t v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static int decode_rejects(unsigned char *copy,size_t size,size_t scenario_offset,
    size_t block_field,size_t record_offset,size_t field_offset,uint32_t value,int word)
{
    struct mac_retail_cache cache;struct mac_native_scenario_gameplay view;const char *error=NULL;
    if(!mac_retail_open(copy,size,&cache,&error))return 0;
    if(record_offset==SIZE_MAX)put32((unsigned char *)cache.tags+scenario_offset+block_field,value);
    else {
        uint32_t address=u32(cache.tags+scenario_offset+block_field+4);
        unsigned char *record=(unsigned char *)mac_retail_tag_span(&cache,address,record_offset+field_offset+4);
        if(!record)return 0;
        if(word)put32(record+record_offset+field_offset,value);
        else put16(record+record_offset+field_offset,(uint16_t)value);
    }
    return !mac_native_scenario_gameplay_decode(&cache,&view,&error) &&
        view.owner==NULL && view.netgame_flag_count==0 && view.netgame_equipment_count==0;
}
int main(int argc,char **argv)
{
    unsigned char *bytes,*copy;size_t size;struct mac_retail_cache cache;
    struct mac_native_scenario_gameplay view;struct mac_engine_world world={0};
    struct mac_engine_world_info info;const char *error=NULL;uint32_t i;
    const unsigned char *scenario,*flags,*equipment,*starting;
    if(argc!=2){fprintf(stderr,"usage: %s bloodgulch.map\n",argv[0]);return 2;}
    bytes=read_file(argv[1],&size);if(!bytes){perror(argv[1]);return 2;}
    if(!mac_retail_open(bytes,size,&cache,&error)){fprintf(stderr,"cache: %s\n",error);free(bytes);return 2;}
    if(!mac_native_scenario_gameplay_decode(&cache,&view,&error))printf("decode error: %s\n",error?error:"unknown");
    CHECK(view.owner!=NULL);
    CHECK(view.netgame_flag_count==191&&view.netgame_equipment_count==37&&view.starting_equipment_count==2);
    if(view.netgame_flags&&view.netgame_equipment&&view.starting_equipment) {
        const struct gameplay_flag *f=view.netgame_flags;
        const struct gameplay_equipment *e=view.netgame_equipment;
        const struct gameplay_starting_equipment *s=view.starting_equipment;
        for(i=0;i<view.netgame_flag_count;++i)
            CHECK(f[i].type>=0&&f[i].type<9&&
                (f[i].team==-1||(f[i].team>=0&&f[i].team<16)));
        for(i=0;i<view.netgame_equipment_count;++i) {
            unsigned j;CHECK(e[i].item_collection.index==-1||
                (e[i].item_collection.group==UINT32_C(0x69746d63)&&e[i].item_collection.name));
            CHECK(e[i].runtime_index==-1);
            for(j=0;j<4;++j)CHECK(e[i].game_type[j]==-1||(e[i].game_type[j]>=0&&e[i].game_type[j]<15));
        }
        for(i=0;i<view.starting_equipment_count;++i) {
            unsigned j;for(j=0;j<4;++j)CHECK(s[i].game_type[j]==-1||(s[i].game_type[j]>=0&&s[i].game_type[j]<15));
            for(j=0;j<6;++j)CHECK(s[i].item_collection[j].index==-1||
                (s[i].item_collection[j].group==UINT32_C(0x69746d63)&&s[i].item_collection[j].name));
        }
    }
    mac_native_scenario_gameplay_free(&view);
    if(!mac_engine_world_create(&cache,0,&world,&error))printf("world error: %s\n",error?error:"unknown");
    CHECK(world.state!=NULL);
    if(world.state) {
        CHECK(mac_engine_world_info(&world,&info));
        CHECK(info.spawns==72&&info.netgame_flags==191&&info.netgame_equipment==37&&info.starting_equipment==2);
        mac_engine_world_free(&world);
    }
    scenario=cache.tags+cache.scenario_offset;
    copy=(unsigned char *)malloc(size);CHECK(copy!=NULL);
    if(copy) {
        size_t scenario_offset=cache.scenario_offset;
        memcpy(copy,bytes,size);
        CHECK(decode_rejects(copy,size,scenario_offset,0x378,SIZE_MAX,0,0xc9,1));
        memcpy(copy,bytes,size);
        flags=mac_retail_tag_span(&cache,u32(scenario+0x37c),0x94);
        CHECK(flags!=NULL);
        if(flags)CHECK(decode_rejects(copy,size,scenario_offset,0x378,0,16,9,0));
        memcpy(copy,bytes,size);
        equipment=mac_retail_tag_span(&cache,u32(scenario+0x388),0x90);
        CHECK(equipment!=NULL);
        if(equipment)CHECK(decode_rejects(copy,size,scenario_offset,0x384,
            0,0x50,UINT32_C(0x77656170),1));
        memcpy(copy,bytes,size);
        starting=mac_retail_tag_span(&cache,u32(scenario+0x394),0xcc);
        CHECK(starting!=NULL);
        free(copy);
    }
    printf("scenario gameplay checks=%u failures=%u counts=191/37/2\n",checks,failures);
    free(bytes);return failures?1:0;
}
