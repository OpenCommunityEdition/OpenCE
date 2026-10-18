#include "engine_world.h"
#include "retail_collision.h"
#include "retail_spawns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned long mac_engine_service_errors;
/* Observe restoration without dereferencing these opaque engine globals. */
struct structure_bsp;
struct collision_bsp;
struct bsp3d;
struct scenario;
extern struct structure_bsp *global_structure_bsp;
extern struct collision_bsp *global_collision_bsp;
extern struct bsp3d *global_bsp3d;
extern struct scenario *global_scenario;
extern long global_scenario_index;
extern short global_structure_bsp_index;
static unsigned long checks,failures;
static uint32_t digest=UINT32_C(2166136261);
#define CHECK(x) do { ++checks;if(!(x)) { if(failures<12)printf("FAIL line %d: %s\n",__LINE__,#x);++failures; } } while(0)
static uint32_t u32(const unsigned char *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int reference_cluster(const struct mac_retail_cache *cache,int leaf)
{
    const struct mac_retail_bsp *bsp=&cache->bsps[0];
    const unsigned char *root=cache->file+bsp->file_offset+bsp->root-bsp->base;
    const unsigned char *record;
    unsigned value;
    if(leaf<0)return -1;
    if((unsigned)leaf>=u32(root+0xe0))return -1;
    record=cache->file+bsp->file_offset+u32(root+0xe4)-bsp->base+16*(unsigned)leaf;
    value=record[8]|record[9]<<8;
    return value==65535?-1:(int)value;
}
static void query(const struct mac_engine_world *world,const struct mac_retail_cache *cache,const struct mac_retail_collision *collision,const float point[3])
{
    struct mac_engine_location location;
    int leaf=mac_retail_collision_leaf(collision,point),cluster;
    unsigned i;
    uint32_t bits;
    CHECK(mac_engine_world_location(world,point,&location));
    cluster=reference_cluster(cache,leaf);
    CHECK(location.leaf==leaf && location.cluster==cluster);
    CHECK(location.bonus==65535);
    CHECK(!global_structure_bsp && !global_collision_bsp && !global_bsp3d && !global_scenario && global_scenario_index==-1 && global_structure_bsp_index==-1);
    bits=(uint32_t)location.leaf;
    for(i=0;i<4;++i) { digest^=(bits>>(i*8))&255;digest*=UINT32_C(16777619); }
    bits=(uint16_t)location.cluster;
    for(i=0;i<2;++i) { digest^=(bits>>(i*8))&255;digest*=UINT32_C(16777619); }
}
int main(int argc,char **argv)
{
    FILE *file;
    unsigned char *bytes;
    long length;
    struct mac_retail_cache cache;
    struct mac_retail_collision collision;
    struct mac_retail_spawn starts[MAC_RETAIL_MAX_SPAWNS];
    struct mac_engine_world world;
    struct mac_engine_world_info info;
    struct mac_engine_location location,before;
    const char *error=NULL;
    uint32_t count,i,nan_bits=UINT32_C(0x7fc00000);
    const unsigned char *sound_bytes;
    uint32_t sound_size,sound_digest=UINT32_C(2166136261),sound_required;
    float point[3],facing;
    int x,y,z;
    unsigned open_starts=0;
    if(argc!=2) { fprintf(stderr,"usage: %s bloodgulch.map\n",argv[0]);return 2; }
    file=fopen(argv[1],"rb");if(!file) { perror(argv[1]);return 2; }
    if(fseek(file,0,SEEK_END) || (length=ftell(file))<0 || length>0x30000000L || fseek(file,0,SEEK_SET)) { fclose(file);return 2; }
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes) { fclose(file);return 2; }
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length) { fclose(file);free(bytes);return 2; }
    fclose(file);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error) || !mac_engine_world_create(&cache,0,&world,&error)) { fprintf(stderr,"world: %s\n",error);free(bytes);return 1; }
    if(!mac_retail_collision_decode(&cache,0,&collision,&error) || !mac_retail_spawns(&cache,starts,&count,&error)) { fprintf(stderr,"reference: %s\n",error);mac_engine_world_free(&world);free(bytes);return 1; }
    CHECK(mac_engine_world_info(&world,&info));
    CHECK(info.leaves==5971 && info.clusters==30 && info.spawns==72 && count==72);
    {
        sound_required=info.clusters*(info.clusters-1)/2;
        CHECK(mac_engine_world_sound_cluster_data(&world,&sound_bytes,&sound_size));
        CHECK(sound_size>=sound_required && sound_bytes!=NULL);
        for(i=0;i<sound_size;++i) { sound_digest^=sound_bytes[i];sound_digest*=UINT32_C(16777619); }
    }
    {
        const unsigned char *scenario=cache.tags+cache.scenario_offset;
        const unsigned char *raw_palette;
        uint32_t handle;
        raw_palette=mac_retail_tag_span(&cache,u32(scenario+0x220),19*48);
        CHECK(raw_palette!=NULL);
        if(raw_palette) {
            CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_SCENERY,1,&handle));
            CHECK(handle==u32(raw_palette+48+12));
            CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_SCENERY,18,&handle));
            CHECK(handle==u32(raw_palette+18*48+12));
            CHECK(!mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_SCENERY,19,&handle));
        }
        raw_palette=mac_retail_tag_span(&cache,u32(scenario+0x250),5*48);
        CHECK(raw_palette!=NULL);
        if(raw_palette) {
            CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_VEHICLE,1,&handle));
            CHECK(handle==u32(raw_palette+48+12));
            CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_VEHICLE,4,&handle));
            CHECK(handle==u32(raw_palette+4*48+12));
            CHECK(!mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_VEHICLE,5,&handle));
        }
        CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_SOUND_SCENERY,0,&handle));
        CHECK(handle==u32(mac_retail_tag_span(&cache,u32(scenario+0x2ec),48)+12));
        CHECK(!mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_WEAPON,0,&handle));
    }
    for(i=0;i<count;++i) {
        CHECK(mac_engine_world_spawn(&world,i,point,&facing,&location));
        CHECK(point[0]==starts[i].position[0] && point[1]==starts[i].position[1] && point[2]==starts[i].position[2] && facing==starts[i].facing);
        if(location.leaf>=0 && location.cluster>=0)++open_starts;
        query(&world,&cache,&collision,point);
    }
    CHECK(open_starts==count);
    {
        unsigned long marker; /* aligned opaque sentinel; never dereferenced */
        global_structure_bsp=(struct structure_bsp *)&marker;global_collision_bsp=(struct collision_bsp *)&marker;global_bsp3d=(struct bsp3d *)&marker;global_scenario=(struct scenario *)&marker;
        global_scenario_index=123;global_structure_bsp_index=7;
        CHECK(mac_engine_world_location(&world,point,&location));
        CHECK((void *)global_structure_bsp==&marker && (void *)global_collision_bsp==&marker && (void *)global_bsp3d==&marker && (void *)global_scenario==&marker && global_scenario_index==123 && global_structure_bsp_index==7);
        global_structure_bsp=NULL;global_collision_bsp=NULL;global_bsp3d=NULL;global_scenario=NULL;
        global_scenario_index=-1;global_structure_bsp_index=-1;
    }
    /* Cross-section grid includes solid, open and far-outside locations. */
    for(z=-20;z<=80;z+=20)for(y=-100;y<=100;y+=10)for(x=-100;x<=100;x+=10) {
        point[0]=(float)x;point[1]=(float)y;point[2]=(float)z;
        query(&world,&cache,&collision,point);
    }
    before=location;memcpy(&point[0],&nan_bits,4);
    CHECK(!mac_engine_world_location(&world,point,&location));
    CHECK(!memcmp(&before,&location,sizeof(location)));
    CHECK(!mac_engine_world_spawn(&world,count,point,&facing,&location));
    {
        const struct mac_retail_bsp *bsp=&cache.bsps[0];
        unsigned char *root=bytes+bsp->file_offset+bsp->root-bsp->base;
        unsigned char *leaf=bytes+bsp->file_offset+u32(root+0xe4)-bsp->base;
        unsigned char saved[4];
        struct mac_engine_world rejected;
        memcpy(saved,leaf+8,2);leaf[8]=255;leaf[9]=127;
        CHECK(!mac_engine_world_create(&cache,0,&rejected,&error));CHECK(!rejected.state);
        memcpy(leaf+8,saved,2);
        memcpy(saved,root+0xe0,4);memset(root+0xe0,255,4);
        CHECK(!mac_engine_world_create(&cache,0,&rejected,&error));CHECK(!rejected.state);
        memcpy(root+0xe0,saved,4);
        memcpy(saved,root+0x138,4);memset(root+0x138,255,4);
        CHECK(!mac_engine_world_create(&cache,0,&rejected,&error));CHECK(!rejected.state);
        memcpy(root+0x138,saved,4);
        CHECK(mac_engine_world_location(&world,starts[0].position,&location));
        CHECK(location.cluster>=0);
    }
    CHECK(mac_engine_service_errors==0);
    {
        uint32_t scenery_handle,vehicle_handle,test_handle;
        CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_SCENERY,1,&scenery_handle));
        CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_VEHICLE,4,&vehicle_handle));
        free(bytes);bytes=NULL;
        CHECK(mac_engine_world_sound_cluster_data(&world,&sound_bytes,&sound_size));
        {
            uint32_t digest_after=UINT32_C(2166136261);
            for(i=0;i<sound_size;++i) { digest_after^=sound_bytes[i];digest_after*=UINT32_C(16777619); }
            CHECK(digest_after==sound_digest);
        }
        CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_SCENERY,1,&test_handle));
        CHECK(test_handle==scenery_handle);
        CHECK(mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_VEHICLE,4,&test_handle));
        CHECK(test_handle==vehicle_handle);
    }
    printf("engine world: %lu checks, %lu failures; %u leaves, %u clusters; sound bytes %u digest %08lx; %u/%u starts open; digest %08lx; engine errors %lu\n",checks,failures,(unsigned)info.leaves,(unsigned)info.clusters,(unsigned)sound_size,(unsigned long)sound_digest,open_starts,(unsigned)count,(unsigned long)digest,mac_engine_service_errors);
    mac_retail_collision_free(&collision);mac_engine_world_free(&world);free(bytes);return failures?1:0;
}
