#include "cseries.h"
#include "real_math.h"
#include "scenario/scenario_definitions.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/data.h"
#include "native_scenario_scripts.h"
#include "engine_script_cache.h"
#include "cache/cache_files.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line%d %s\n",__LINE__,#x);}}while(0)
#include "mac_scenario_script_validators.h"
static uint32_t read32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void reject(const struct mac_retail_cache *cache,unsigned char *field,unsigned count,uint32_t value)
{
    struct mac_native_scenario_scripts invalid={0};const char *error;unsigned char saved[4];unsigned i;
    for(i=0;i<count;++i){saved[i]=field[i];field[i]=(unsigned char)(value>>(i*8));}
    CHECK(!mac_native_scenario_scripts_decode(cache,&invalid,&error));CHECK(invalid.state==NULL);
    mac_native_scenario_scripts_free(&invalid);for(i=0;i<count;++i)field[i]=saved[i];
}
int main(int argc,char **argv)
{
    FILE *file;long bytes;unsigned char *raw,*root,*syntax,*strings;
    struct mac_retail_cache cache;struct mac_native_scenario_scripts scripts={0},other={0};
    struct scenario scenario;struct data_array *data;struct hs_syntax_node *nodes,*node;
    const char *error;uint32_t string_hash=2166136261UL,hash=2166136261UL;unsigned i;long index;
    if(argc!=2)return 2;file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(bytes=ftell(file))<2048||bytes>0x30000000L||fseek(file,0,SEEK_SET)){fclose(file);return 2;}
    raw=malloc(bytes);if(!raw){fclose(file);return 2;}
    if(fread(raw,1,bytes,file)!=(size_t)bytes){fclose(file);free(raw);return 2;}fclose(file);
    if(!mac_retail_open(raw,bytes,&cache,&error)){free(raw);return 1;}
    root=(unsigned char *)cache.tags+cache.scenario_offset;
    syntax=(unsigned char *)mac_retail_tag_span(&cache,read32(root+0x474+12),read32(root+0x474));
    strings=(unsigned char *)mac_retail_tag_span(&cache,read32(root+0x488+12),read32(root+0x488));
    CHECK(syntax && strings);if(!syntax||!strings){free(raw);return 1;}
    for(i=0;i<read32(root+0x488);++i)string_hash=(string_hash^strings[i])*16777619UL;
    reject(&cache,root+0x474,4,1);reject(&cache,root+0x474+4,4,1);
    reject(&cache,root+0x474+12,4,0xffffffff);reject(&cache,root+0x488,4,1023);
    reject(&cache,root+0x488+12,4,0xffffffff);reject(&cache,root+0x49c,4,1);
    reject(&cache,root+0x4a8,4,1);reject(&cache,root+0x4b4,4,1);
    reject(&cache,syntax+32,2,19000);reject(&cache,syntax+34,2,19);
    reject(&cache,syntax+36,1,0);reject(&cache,syntax+40,4,0);
    reject(&cache,syntax+44,2,1);reject(&cache,syntax+46,2,1);
    reject(&cache,syntax+48,2,1);reject(&cache,syntax+50,2,0);
    reject(&cache,syntax+56,2,1);
    CHECK(mac_native_scenario_scripts_decode(&cache,&scripts,&error));
    if(!scripts.state){fprintf(stderr,"scripts: %s\n",error);free(raw);return 1;}
    memset(&scenario,0,sizeof(scenario));CHECK(mac_native_scenario_scripts_attach(&scripts,&scenario));
    data=scenario.hs_syntax_data.address;nodes=data->data;
    CHECK(data->maximum_count==19001 && data->size==20);CHECK(data->signature=='d@t@');
    CHECK(data->data==data+1);CHECK(data->count==0 && data->actual_count==0);
    for(i=0;i<19001;++i) {
        const unsigned char *p=syntax+56+i*20;
        CHECK(nodes[i].datum_header==0);CHECK((uint32_t)nodes[i].data==read32(p+16));
        CHECK((uint32_t)nodes[i].next_node_index==read32(p+8));
        CHECK((unsigned short)nodes[i].short_value==(unsigned short)read32(p+16));
    }
    free(raw);raw=NULL;
    CHECK(!cache_file_tag_cache_contains(data,1));
    CHECK(!mac_engine_script_cache_bind(NULL));CHECK(!mac_engine_script_cache_bind(&other));
    CHECK(mac_engine_script_cache_bind(&scripts));CHECK(!mac_engine_script_cache_bind(&scripts));
    CHECK(!mac_engine_script_cache_unbind(&other));CHECK(!mac_engine_script_cache_unbind(NULL));
    CHECK(!cache_file_tag_cache_contains(data,0));
    CHECK(hs_scenario_syntax_data_valid(&scenario));CHECK(hs_scenario_string_constants_valid(&scenario));
    CHECK(!cache_file_tag_cache_contains((unsigned char *)data-1,1));
    CHECK(!cache_file_tag_cache_contains(data,scenario.hs_syntax_data.size+1));
    CHECK(!cache_file_tag_cache_contains(data,-1));CHECK(!cache_file_tag_cache_contains(NULL,0));
    for(i=0;i<(unsigned)scenario.hs_string_constants.size;++i)hash=(hash^((unsigned char *)scenario.hs_string_constants.address)[i])*16777619UL;
    CHECK(hash==string_hash);
    data_verify(data);index=datum_new(data);CHECK(index!=NONE);node=datum_get(data,index);
    node->data=0x12345678;CHECK(node->short_value==0x5678);
    datum_delete(data,index);CHECK(data->actual_count==0);data_verify(data);
    CHECK(hs_scenario_syntax_data_valid(&scenario));
    CHECK(mac_engine_script_cache_unbind(&scripts));CHECK(!mac_engine_script_cache_unbind(&scripts));
    CHECK(!cache_file_tag_cache_contains(data,1));
    CHECK(mac_engine_script_cache_bind(&scripts));CHECK(mac_engine_script_cache_unbind(&scripts));
    mac_native_scenario_scripts_free(&scripts);CHECK(!cache_file_tag_cache_contains(data,1));
    CHECK(!mac_engine_script_cache_bind(&scripts));
    printf("native scenario scripts: %u checks, %u failures; strings=%08lx\n",checks,failures,(unsigned long)hash);
    return failures?1:0;
}
