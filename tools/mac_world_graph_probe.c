#include "cseries.h"
#include "objects/objects.h"

#include "math/real_math.h"

#include "structures/structures.h"
#include "structures/structure_bsp_definitions.h"
#include "engine_world.h"
#include "engine_world_partition.h"
#include "structures/cluster_partitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "cache/cache_files.h"
#include <stdint.h>
#undef malloc
#undef free
#include <stdio.h>
#include <stdlib.h>
extern struct structure_bsp *global_structure_bsp;
extern unsigned long mac_engine_service_errors;
static unsigned checks,failures;
static uint32_t physical_bounds[8];
static uint32_t sound_counts[2],sound_handles[2][64];
static uint32_t expected_pvs[128];static unsigned pvs_words;
static void *observed_syntax;
static uint32_t read32(const unsigned char *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
#define CHECK(x) do { ++checks;if(!(x)){++failures;printf("FAIL %d: %s\n",__LINE__,#x);} }while(0)
struct context { real_point3d point;short cluster;const struct mac_engine_world *world;const struct mac_retail_cache *cache; };
static int run(void *opaque)
{
    struct context *c=(struct context *)opaque;short result[64],queue[64];unsigned seen[64]={0};
    unsigned head=0,tail=1,i,j;short n;
    struct cluster_partition partition={0};struct location location;long first=-1,ref,datum;
    const unsigned char *source_root=c->cache->tags+c->cache->scenario_offset;
    const void *source_syntax=mac_retail_tag_span(c->cache,read32(source_root+0x480),1);
    const void *source_strings=mac_retail_tag_span(c->cache,read32(source_root+0x494),1);
    {
        uint32_t bits;memcpy(&bits,&global_scenario->local_north,4);
        CHECK(bits==read32(source_root+offsetof(struct scenario,local_north)));
        CHECK(global_scenario->flags==(source_root[offsetof(struct scenario,flags)]|
            source_root[offsetof(struct scenario,flags)+1]<<8));
    }
    {
        static const unsigned offsets[]={offsetof(struct scenario,ugly_structure_bsp),
            offsetof(struct scenario,unloved_globals),offsetof(struct scenario,bad_sky),
            offsetof(struct scenario,custom_object_names),offsetof(struct scenario,ingame_help_text),
            offsetof(struct scenario,hud_messages)};
        for(i=0;i<sizeof(offsets)/sizeof(offsets[0]);++i) {
            const struct tag_reference *ref=(const struct tag_reference *)((const unsigned char *)global_scenario+offsets[i]);
            CHECK(ref->index==NONE && ref->name==NULL && ref->name_length==0);
            CHECK(ref->group_tag==read32(source_root+offsets[i]));
        }
    }
    CHECK(global_scenario->hs_syntax_data.size==56+20*19001);
    CHECK(global_scenario->hs_string_constants.size==1024);
    CHECK(global_scenario->hs_scripts.count==0&&global_scenario->hs_globals.count==0&&global_scenario->hs_references.count==0);
    observed_syntax=global_scenario->hs_syntax_data.address;
    CHECK(mac_engine_world_scenario_cache_contains(c->world,observed_syntax,(size_t)global_scenario->hs_syntax_data.size));
    CHECK(mac_engine_world_scenario_cache_contains(c->world,observed_syntax,1));
    CHECK(!mac_engine_world_scenario_cache_contains(c->world,(const void *)((uintptr_t)observed_syntax-1),1));
    CHECK(!mac_engine_world_scenario_cache_contains(c->world,observed_syntax,(size_t)global_scenario->hs_syntax_data.size+1));
    CHECK(mac_engine_world_scenario_cache_contains(c->world,global_scenario->hs_string_constants.address,
        (size_t)global_scenario->hs_string_constants.size));
    CHECK(!mac_engine_world_scenario_cache_contains(c->world,NULL,0));
    CHECK(source_syntax&&source_strings);
    CHECK(!cache_file_tag_cache_contains(source_syntax,1)&&!cache_file_tag_cache_contains(source_strings,1));
    CHECK(cache_file_tag_cache_contains(observed_syntax,global_scenario->hs_syntax_data.size));
    CHECK(cache_file_tag_cache_contains(global_scenario->hs_string_constants.address,global_scenario->hs_string_constants.size));
    CHECK(!cache_file_tag_cache_contains(observed_syntax,0));
    for(i=0;i<pvs_words;++i)CHECK(structure_bsp_get_cluster_pvs(global_structure_bsp,c->cluster)[i]==expected_pvs[c->cluster*pvs_words+i]);
    for(i=0;i<2;++i) {
        struct tag_block *block=i?&global_structure_bsp->sound_environment_palette:&global_structure_bsp->background_sound_palette;
        unsigned stride=i?sizeof(struct structure_sound_environment_palette_entry):sizeof(struct structure_background_sound_palette_entry);
        CHECK(block->count==sound_counts[i]);CHECK(block->definition==NULL);
        for(j=0;j<sound_counts[i];++j) {
            struct tag_reference *ref=(struct tag_reference *)((unsigned char *)block->address+j*stride+32);
            CHECK((uint32_t)ref->index==sound_handles[i][j]);
            CHECK(ref->group_tag==(i?UINT32_C(0x736e6465):UINT32_C(0x6c736e64)));
            if(ref->index!=NONE)CHECK(ref->name && ref->name_length>0);
        }
    }
    for(i=0;i<8;++i) {
        real value=i==0?global_structure_bsp->vehicle_floor:i==1?global_structure_bsp->vehicle_ceiling:global_structure_bsp->world_bounds.n[i-2];
        uint32_t bits;memcpy(&bits,&value,4);CHECK(bits==physical_bounds[i]);
    }
    queue[0]=c->cluster;seen[c->cluster]=1;
    n=structure_clusters_in_sphere(c->cluster,&c->point,0,64,result);
    CHECK(n==1 && result[0]==c->cluster);
    /* Independent graph reachability. A million-unit sphere surrounds all of
       Blood Gulch and therefore must visit every reachable portal endpoint. */
    while(head<tail) {
        struct tag_block *list=(struct tag_block *)((unsigned char *)global_structure_bsp->clusters.address+104*queue[head++]+92);
        for(i=0;i<(unsigned)list->count;++i) {
            short index=((short *)list->address)[i];
            short *ends=(short *)((unsigned char *)global_structure_bsp->cluster_portals.address+64*index);
            for(j=0;j<2;++j)if(!seen[ends[j]]) {seen[ends[j]]=1;queue[tail++]=ends[j];}
        }
    }
    n=structure_clusters_in_sphere(c->cluster,&c->point,1000000.f,64,result);
    CHECK(n==(short)tail);
    for(i=0;i<(unsigned)n && i<64;++i) {
        CHECK(result[i]>=0 && result[i]<global_structure_bsp->clusters.count);
        if(result[i]>=0 && result[i]<64){CHECK(seen[result[i]]==1);seen[result[i]]=2;}
    }
    CHECK(mac_engine_partition_start(&partition));
    scenario_location_from_point(&location,&c->point);
    cluster_partition_reconnect(&partition,0x12340001,&first,&c->point,1000000.f,&location);
    CHECK(first!=-1);CHECK(partition.data_reference_data->actual_count==n);
    CHECK(partition.cluster_reference_data->actual_count==n);
    for(i=0;i<(unsigned)n;++i) {
        datum=cluster_partition_get_first_datum(&partition,&ref,result[i]);CHECK(datum==0x12340001);
        CHECK(cluster_partition_get_next_datum(&partition,&ref)==-1);
    }
    CHECK(!mac_engine_partition_stop(&partition));
    cluster_partition_disconnect(&partition,0x12340001,&first);CHECK(first==-1);
    CHECK(partition.data_reference_data->actual_count==0 && partition.cluster_reference_data->actual_count==0);
    CHECK(mac_engine_partition_stop(&partition));
    return 1;
}
static void reject_graph(const struct mac_retail_cache *cache,unsigned char *field,unsigned size,uint32_t value)
{
    struct mac_engine_world test={0};const char *error;unsigned char saved[4];unsigned i;
    if(!mac_engine_world_create(cache,0,&test,&error)){CHECK(0);return;}
    for(i=0;i<size;++i){saved[i]=field[i];field[i]=(unsigned char)(value>>(8*i));}
    CHECK(!mac_engine_world_enable_graph(&test,cache,&error));
    for(i=0;i<size;++i)field[i]=saved[i];
    CHECK(mac_engine_world_enable_graph(&test,cache,&error));
    mac_engine_world_free(&test);
}
static void reject_world(const struct mac_retail_cache *cache,unsigned char *field,unsigned size,uint32_t value)
{
    struct mac_engine_world test={0};const char *error;unsigned char saved[4];unsigned i;
    for(i=0;i<size;++i){saved[i]=field[i];field[i]=(unsigned char)(value>>(8*i));}
    CHECK(!mac_engine_world_create(cache,0,&test,&error));
    mac_engine_world_free(&test);
    for(i=0;i<size;++i)field[i]=saved[i];
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes;struct mac_retail_cache cache;
    struct mac_engine_world world={0};struct mac_engine_world_info info;struct mac_engine_location location;
    const char *error;float point[3],facing;unsigned i;struct context c;
    if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc(length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,length,f)!=(size_t)length){fclose(f);free(bytes);return 2;}fclose(f);
    if(!mac_retail_open(bytes,length,&cache,&error)||!mac_engine_world_create(&cache,0,&world,&error)||
       !mac_engine_world_enable_graph(&world,&cache,&error)){fprintf(stderr,"graph: %s\n",error);mac_engine_world_free(&world);free(bytes);return 1;}
    CHECK(mac_engine_world_info(&world,&info));
    {
        unsigned char *scenario_root=(unsigned char *)cache.tags+cache.scenario_offset;
        uint32_t sky_count=read32(scenario_root+offsetof(struct scenario,sky_references));
        uint32_t sky_address=read32(scenario_root+offsetof(struct scenario,sky_references)+4);
        unsigned char *sky_refs=(unsigned char *)mac_retail_tag_span(&cache,sky_address,sky_count*16);
        uint32_t sky_handle=UINT32_MAX,sky_out=0;uint32_t i;
        CHECK(sky_count<=MAXIMUM_SKIES_PER_SCENARIO);
        CHECK(info.sky_references==sky_count);
        CHECK(sky_count==0 || sky_refs!=NULL);
        for(i=0;i<sky_count && i<MAXIMUM_SKIES_PER_SCENARIO;++i) {
            sky_handle=read32(sky_refs+i*16+12);
            CHECK(mac_engine_world_sky_handle(&world,i,&sky_out));CHECK(sky_out==sky_handle);
        }
        CHECK(!mac_engine_world_sky_handle(&world,sky_count,&sky_out));
        reject_world(&cache,scenario_root+offsetof(struct scenario,hud_messages)+12,4,0);
        reject_world(&cache,scenario_root+offsetof(struct scenario,custom_object_names)+4,4,1);
        reject_world(&cache,scenario_root+offsetof(struct scenario,ingame_help_text)+8,4,1);
        reject_world(&cache,scenario_root+offsetof(struct scenario,local_north),4,0x7fc00000);
        reject_world(&cache,scenario_root+offsetof(struct scenario,local_north),4,0x7f800000);
        reject_world(&cache,scenario_root+offsetof(struct scenario,sky_references),4,MAXIMUM_SKIES_PER_SCENARIO+1);
        if(sky_count) {
            reject_world(&cache,sky_refs,4,UINT32_C(0x62697064));
            reject_world(&cache,sky_refs+12,4,UINT32_C(0x12345678));
        }
    }
    { const struct mac_retail_bsp *b=&cache.bsps[0];
      const unsigned char *r=cache.file+b->file_offset+b->root-b->base;
      const unsigned char *v=cache.file+b->file_offset+read32(r+0x14c)-b->base;
      pvs_words=(info.clusters+31)/32;
      unsigned char *root=(unsigned char *)r;
      unsigned char *portal=(unsigned char *)cache.file+b->file_offset+read32(r+0x158)-b->base;
      unsigned char *cluster=(unsigned char *)cache.file+b->file_offset+read32(r+0x138)-b->base;
      reject_world(&cache,root+offsetof(struct structure_bsp,default_lighting),4,0x7fc00000);
      reject_world(&cache,root+offsetof(struct structure_bsp,default_lighting)+offsetof(struct render_lighting,distant_light_count),2,MAXIMUM_RENDERED_DISTANT_LIGHTS+1);
      reject_world(&cache,root+offsetof(struct structure_bsp,default_lighting)+offsetof(struct render_lighting,point_light_count),2,1);
      for(i=0;i<8;++i) {
          unsigned offset=i<2?16+4*i:offsetof(struct structure_bsp,world_bounds)+4*(i-2);
          physical_bounds[i]=read32(root+offset);
          reject_world(&cache,root+offset,4,0x7fc00000);
      }
      reject_world(&cache,root+offsetof(struct structure_bsp,world_bounds),4,0x7f7fffff);
      for(i=0;i<2;++i) {
          unsigned j,offset=i?offsetof(struct structure_bsp,sound_environment_palette):offsetof(struct structure_bsp,background_sound_palette);
          unsigned stride=i?sizeof(struct structure_sound_environment_palette_entry):sizeof(struct structure_background_sound_palette_entry);
          unsigned char *palette=(unsigned char *)cache.file+b->file_offset+read32(root+offset+4)-b->base;
          sound_counts[i]=read32(root+offset);CHECK(sound_counts[i]<=64);
          for(j=0;j<sound_counts[i]&&j<64;++j)sound_handles[i][j]=read32(palette+j*stride+44);
          {
              uint32_t actual[64],count=0;
              CHECK(mac_engine_world_sound_handles(&world,i?UINT32_C(0x736e6465):UINT32_C(0x6c736e64),actual,64,&count));
              CHECK(count==sound_counts[i]);
              for(j=0;j<count;++j)CHECK(actual[j]==sound_handles[i][j]);
              if(count)CHECK(!mac_engine_world_sound_handles(&world,i?UINT32_C(0x736e6465):UINT32_C(0x6c736e64),actual,count-1,&count));
          }
          reject_world(&cache,root+offset,4,65);
          if(sound_counts[i]) {
              reject_world(&cache,root+offset+4,4,0xffffffff);
              reject_world(&cache,palette+32,4,0);
              reject_world(&cache,palette+44,4,0x12345678);
          }
          reject_world(&cache,cluster+4+i*2,2,sound_counts[i]);
          reject_world(&cache,cluster+4+i*2,2,65534);
      }
      for(i=0;i<info.clusters*pvs_words;++i)expected_pvs[i]=read32(v+i*4);
      reject_graph(&cache,root+0x140,4,0);
      reject_graph(&cache,root+0x14c,4,0xffffffff);
      reject_graph(&cache,root+0x154,4,1025);
      reject_graph(&cache,portal,2,65535);
      reject_graph(&cache,portal+4,4,0xffffffff);
      reject_graph(&cache,portal+8,4,0x7fc00000);
      reject_graph(&cache,portal+52,4,129);
      reject_graph(&cache,portal+56,4,0xffffffff);
      reject_graph(&cache,cluster+92,4,1025);
    }
    for(i=0;i<info.spawns;++i)if(mac_engine_world_spawn(&world,i,point,&facing,&location)&&location.cluster>=0) {
        c.point.x=point[0];c.point.y=point[1];c.point.z=point[2];c.cluster=location.cluster;c.world=&world;c.cache=&cache;
        CHECK(mac_engine_world_scope(&world,run,&c));CHECK(global_structure_bsp==NULL);
        CHECK(!cache_file_tag_cache_contains(observed_syntax,1));
    }
    CHECK(mac_engine_service_errors==0);
    CHECK(mac_engine_world_scenario_cache_contains(&world,observed_syntax,1));
    mac_engine_world_free(&world);
    CHECK(!mac_engine_world_scenario_cache_contains(&world,observed_syntax,1));
    CHECK(!cache_file_tag_cache_contains(observed_syntax,1));free(bytes);
    printf("actual structure sphere traversal: checks=%u failures=%u spawns=%lu clusters=%lu\n",checks,failures,(unsigned long)info.spawns,(unsigned long)info.clusters);
    return failures?1:0;
}
