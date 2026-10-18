#include "cseries.h"
#include "engine_world.h"
#include "engine_arena.h"
#include "engine_game_runtime.h"
int mac_constructor_time_start(long);
void mac_constructor_time_stop(void);
#include "retail_spawns.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_usage.h"
#include "structures/structure_bsp_definitions.h"
#include "structures/structures.h"
#include "structures/structure_vector_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct structure_bsp *global_structure_bsp;
static unsigned checks,failures;
static unsigned char expected_lighting[sizeof(struct render_lighting)];
static unsigned char *expected_materials;
static unsigned expected_material_count;
#define CHECK(x) do { ++checks; if(!(x)) { if(failures<12)fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
static uint32_t read_u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void write_u32(unsigned char *p,uint32_t v)
{ p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24); }
static unsigned char *read_file(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb");long n;unsigned char *bytes;
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END)||(n=ftell(f))<0||n>0x30000000L||fseek(f,0,SEEK_SET)) { fclose(f);return NULL; }
    bytes=(unsigned char *)malloc((size_t)n);
    if(!bytes||fread(bytes,1,(size_t)n,f)!=(size_t)n) { free(bytes);fclose(f);return NULL; }
    fclose(f);*size=(size_t)n;return bytes;
}
static void check_lighting(const struct render_lighting *lighting,const unsigned char *raw)
{
    unsigned i;const unsigned char *native=(const unsigned char *)lighting;
    for(i=0;i<sizeof(*lighting);i+=4) {
        if(i==offsetof(struct render_lighting,distant_light_count) || i==offsetof(struct render_lighting,point_light_count)) {
            unsigned short a,b;memcpy(&a,native+i,2);memcpy(&b,native+i+2,2);
            CHECK(a==(unsigned)(raw[i]|raw[i+1]<<8));CHECK(b==(unsigned)(raw[i+2]|raw[i+3]<<8));
        } else {uint32_t bits;memcpy(&bits,native+i,4);CHECK(bits==read_u32(raw+i));}
    }
}
static void check_cluster_surface_lists(struct structure_bsp *bsp)
{
    uint32_t cluster_index,surface_index,lightmap_index,material_index;
    unsigned long total_entries=0;
    unsigned char *renderable=(unsigned char *)calloc((bsp->surfaces.count+7)/8,1);
    CHECK(renderable!=NULL);
    if(!renderable)return;
    for(lightmap_index=0;lightmap_index<(uint32_t)bsp->lightmaps.count;++lightmap_index) {
        struct structure_lightmap *lightmap=(struct structure_lightmap *)bsp->lightmaps.address+lightmap_index;
        for(material_index=0;material_index<(uint32_t)lightmap->materials.count;++material_index) {
            struct structure_material *material=(struct structure_material *)lightmap->materials.address+material_index;
            for(surface_index=(uint32_t)material->first_surface_index;
                surface_index<(uint32_t)(material->first_surface_index+material->surface_count);++surface_index)
                renderable[surface_index>>3]|=(unsigned char)(1u<<(surface_index&7));
        }
    }
    for(cluster_index=0;cluster_index<(uint32_t)bsp->clusters.count;++cluster_index) {
        struct tag_block *list=(struct tag_block *)((unsigned char *)bsp->clusters.address+cluster_index*104+68);
        unsigned char *expected=(unsigned char *)calloc((bsp->surfaces.count+7)/8,1);
        unsigned char *emitted=(unsigned char *)calloc((bsp->surfaces.count+7)/8,1);
        struct structure_leaf *leaves=(struct structure_leaf *)bsp->leaves.address;
        struct world_surface_reference { long surface_index,bsp3d_node_index; };
        struct world_surface_reference *references=(struct world_surface_reference *)bsp->surface_references.address;
        long *entries=(long *)list->address;
        long cursor=0;
        uint32_t leaf_index;
        CHECK(expected!=NULL && emitted!=NULL);
        if(!expected||!emitted) { free(expected);free(emitted);break; }
        for(leaf_index=0;leaf_index<(uint32_t)bsp->leaves.count;++leaf_index) {
            struct structure_leaf *leaf=leaves+leaf_index;
            long ref;
            if(leaf->cluster_index!=(short)cluster_index)continue;
            for(ref=leaf->first_surface_reference_index;
                ref<leaf->first_surface_reference_index+leaf->surface_reference_count;++ref) {
                long surface=references[ref].surface_index;
                if(surface>=0&&surface<bsp->surfaces.count)
                    expected[(uint32_t)surface>>3]|=(unsigned char)(1u<<((uint32_t)surface&7));
            }
        }
        CHECK(list->count>=0 && (list->count==0 || entries!=NULL));
        while(cursor<list->count && entries) {
            long lm,mat,count,k;
            CHECK(list->count-cursor>=3);
            if(list->count-cursor<3)break;
            lm=entries[cursor++];mat=entries[cursor++];count=entries[cursor++];
            CHECK(lm>=0&&lm<bsp->lightmaps.count);
            CHECK(lm>=0&&lm<bsp->lightmaps.count&&mat>=0&&mat<((struct structure_lightmap *)bsp->lightmaps.address)[lm].materials.count);
            CHECK(count>0&&count<=list->count-cursor);
            if(lm<0||lm>=bsp->lightmaps.count||mat<0||
               mat>=((struct structure_lightmap *)bsp->lightmaps.address)[lm].materials.count||
               count<=0||count>list->count-cursor)break;
            for(k=0;k<count;++k) {
                long surface=entries[cursor++];
                CHECK(surface>=0&&surface<bsp->surfaces.count);
                if(surface<0||surface>=bsp->surfaces.count)continue;
                CHECK(expected[(uint32_t)surface>>3]&(unsigned char)(1u<<((uint32_t)surface&7)));
                CHECK(!(emitted[(uint32_t)surface>>3]&(unsigned char)(1u<<((uint32_t)surface&7))));
                emitted[(uint32_t)surface>>3]|=(unsigned char)(1u<<((uint32_t)surface&7));
                {
                    struct structure_lightmap *lightmap=(struct structure_lightmap *)bsp->lightmaps.address+lm;
                    struct structure_material *material=(struct structure_material *)lightmap->materials.address+mat;
                    CHECK(surface>=material->first_surface_index&&surface<material->first_surface_index+material->surface_count);
                }
                ++total_entries;
            }
        }
        CHECK(cursor==list->count);
        for(surface_index=0;surface_index<(uint32_t)bsp->surfaces.count;++surface_index)
            if((expected[surface_index>>3]&(unsigned char)(1u<<(surface_index&7))) &&
               (renderable[surface_index>>3]&(unsigned char)(1u<<(surface_index&7))))
                CHECK(emitted[surface_index>>3]&(unsigned char)(1u<<(surface_index&7)));
        free(expected);free(emitted);
    }
    CHECK(total_entries>0);
    free(renderable);
}
static void reject_surface(const struct mac_retail_cache *cache,unsigned char *field,uint32_t value)
{
    unsigned char saved[4];struct mac_engine_world test={0};const char *error;
    memcpy(saved,field,4);write_u32(field,value);
    CHECK(mac_engine_world_create(cache,0,&test,&error));
    CHECK(!mac_engine_world_enable_surfaces(&test,cache,&error));
    mac_engine_world_free(&test);memcpy(field,saved,4);
}
struct sample_context { float starts[MAC_RETAIL_MAX_SPAWNS][3];uint32_t count;int found; };
static int sample_surface(void *context)
{
    struct sample_context *samples=(struct sample_context *)context;
    struct structure_bsp *bsp=global_structure_bsp;
    uint32_t i;
    CHECK(bsp!=NULL && bsp->surface_references.count>0 && bsp->surfaces.count>0 && bsp->lightmaps.count>0);
    check_cluster_surface_lists(bsp);
    check_lighting(&bsp->default_lighting,expected_lighting);
    CHECK((uint32_t)bsp->lightmap_group.index==UINT32_C(0xeab3093d));
    {
        unsigned material_total=0,j,k;
    for(i=0;i<(uint32_t)bsp->lightmaps.count;++i) {
        struct structure_lightmap *lightmap=(struct structure_lightmap *)bsp->lightmaps.address+i;
        if(lightmap->materials.count>0) {
            struct structure_material *material=(struct structure_material *)lightmap->materials.address;
            CHECK(material->vertices.count>0 && material->lightmap_vertices.count==(lightmap->bitmap_index==NONE?0:material->vertices.count));
            CHECK(material->compressed_vertex_data.address!=NULL);
            for(j=0;j<(unsigned)lightmap->materials.count;++j) {
                const unsigned char *raw=expected_materials+256*material_total++;
                struct structure_material *m=material+j;
                CHECK(material_total<=expected_material_count);
                check_lighting(&m->lighting,raw+offsetof(struct structure_material,lighting));
                for(k=0;k<7;++k) {
                    uint32_t bits;unsigned offset=k<3?offsetof(struct structure_material,centroid)+4*k:offsetof(struct structure_material,plane)+4*(k-3);
                    memcpy(&bits,(unsigned char *)m+offset,4);CHECK(bits==read_u32(raw+offset));
                }
                CHECK(m->breakable_surface_index==NONE);
            }
        }
    }
        CHECK(material_total>0 && material_total==expected_material_count);
    }
    for(i=0;i<samples->count&&!samples->found;++i) {
        float offset;
        for(offset=0.25f;offset<=2.0f&&!samples->found;offset+=0.25f) {
            real_point3d start,point;
            real_vector3d vector;
            short lightmap_index=NONE,material_index=NONE;
            long surface_index=NONE;
            real s=0.0f,t=0.0f;
            start.x=samples->starts[i][0];start.y=samples->starts[i][1];start.z=samples->starts[i][2]+offset;
            vector.i=0.0f;vector.j=0.0f;vector.k=-4.0f;
            samples->found=structure_test_vector(&start,&vector,&point,&lightmap_index,&material_index,&surface_index,&s,&t);
            if(samples->found) {
                struct structure_lightmap *lightmap=(struct structure_lightmap *)bsp->lightmaps.address+lightmap_index;
                struct structure_material *material=(struct structure_material *)lightmap->materials.address+material_index;
                CHECK(lightmap_index>=0&&lightmap_index<bsp->lightmaps.count);
                CHECK(material_index>=0&&material_index<lightmap->materials.count);
                CHECK(surface_index>=material->first_surface_index&&surface_index<material->first_surface_index+material->surface_count);
                CHECK(lightmap->bitmap_index!=NONE);
                CHECK(s>=0.0f&&s<=1.0f&&t>=0.0f&&t<=1.0f);
            }
        }
    }
    CHECK(samples->found);
    return samples->found;
}
int main(int argc,char **argv)
{
    unsigned char *bytes;size_t size;
    struct mac_retail_cache cache;struct mac_engine_world world={0},rejected={0};
    struct mac_engine_world_info info;
    struct mac_engine_location location;
    struct sample_context samples;
    const struct mac_retail_bsp *bsp;
    unsigned char *root,*references,*lightmaps,*materials;
    const char *error=NULL;
    uint32_t i,spawn_count,reference_count,lightmap_count,material_count;
    float point[3],facing;
    unsigned char saved[4];
    if(argc!=2) { fprintf(stderr,"usage: %s bloodgulch.map\n",argv[0]);return 2; }
    bytes=read_file(argv[1],&size);if(!bytes) { perror(argv[1]);return 2; }
    if(!mac_retail_open(bytes,size,&cache,&error)) { fprintf(stderr,"cache: %s\n",error?error:"open failed");free(bytes);return 1; }
    bsp=&cache.bsps[0];root=bytes+bsp->file_offset+(bsp->root-bsp->base);
    reference_count=read_u32(root+0xec);references=bytes+bsp->file_offset+(read_u32(root+0xf0)-bsp->base);
    lightmap_count=read_u32(root+0x104);lightmaps=bytes+bsp->file_offset+(read_u32(root+0x108)-bsp->base);
    CHECK(reference_count>0&&lightmap_count>0);

    /* Mutate then restore an actual on-map surface index. Conversion must
       reject it before publishing a usable native world. */
    write_u32(saved,read_u32(references));write_u32(references,reference_count+7);
    CHECK(mac_engine_world_create(&cache,0,&rejected,&error));
    CHECK(!mac_engine_world_enable_surfaces(&rejected,&cache,&error));
    mac_engine_world_free(&rejected);memcpy(references,saved,4);

    /* The material vertex span is independently validated before allocation
       or callback publication. */
    material_count=read_u32(lightmaps+20);
    CHECK(material_count>0);
    materials=bytes+bsp->file_offset+(read_u32(lightmaps+24)-bsp->base);
    write_u32(saved,read_u32(materials+0xe4));write_u32(materials+0xe4,UINT32_MAX);
    CHECK(mac_engine_world_create(&cache,0,&rejected,&error));
    CHECK(!mac_engine_world_enable_surfaces(&rejected,&cache,&error));
    mac_engine_world_free(&rejected);memcpy(materials+0xe4,saved,4);

    reject_surface(&cache,materials+offsetof(struct structure_material,centroid),UINT32_C(0x7fc00000));
    reject_surface(&cache,materials+offsetof(struct structure_material,plane),UINT32_C(0x7fc00000));
    reject_surface(&cache,materials+offsetof(struct structure_material,lighting)+offsetof(struct render_lighting,distant_light_count),MAXIMUM_RENDERED_DISTANT_LIGHTS+1);
    reject_surface(&cache,materials+offsetof(struct structure_material,lighting)+offsetof(struct render_lighting,point_light_count),1);

    if(!mac_engine_world_create(&cache,0,&world,&error)||
       !mac_engine_world_enable_graph(&world,&cache,&error)||
       !mac_engine_world_enable_surfaces(&world,&cache,&error)) {
        fprintf(stderr,"world conversion: %s\n",error?error:"failed");
        mac_engine_world_free(&world);free(bytes);return 1;
    }
    memcpy(expected_lighting,bytes+bsp->file_offset+bsp->root-bsp->base+offsetof(struct structure_bsp,default_lighting),sizeof(expected_lighting));
    for(i=0;i<lightmap_count;++i)expected_material_count+=read_u32(lightmaps+32*i+20);
    expected_materials=malloc(expected_material_count*256);CHECK(expected_materials!=NULL);
    if(!expected_materials)return 2;
    { unsigned next=0;
      for(i=0;i<lightmap_count;++i) {
          unsigned count=read_u32(lightmaps+32*i+20);
          const unsigned char *rows=bytes+bsp->file_offset+read_u32(lightmaps+32*i+24)-bsp->base;
          memcpy(expected_materials+next*256,rows,count*256);next+=count;
      }
    }
    CHECK(mac_engine_world_info(&world,&info));
    CHECK(info.leaves>0&&info.clusters>0);
    CHECK(mac_engine_world_lightmap_handle(&world)==UINT32_C(0xeab3093d));
    memset(&samples,0,sizeof(samples));
    for(i=0;i<MAC_RETAIL_MAX_SPAWNS;++i) {
        if(i>=info.spawns)break;
        CHECK(mac_engine_world_spawn(&world,i,point,&facing,&location));
        memcpy(samples.starts[i],point,sizeof(point));
        ++samples.count;
    }
    CHECK(samples.count>0);
    CHECK(mac_engine_arena_start());
    CHECK(mac_constructor_time_start(123));CHECK(mac_game_core_start());
    breakable_surfaces_initialize();breakable_surfaces_initialize_for_new_map();
    collision_log_initialize();
    /* Conversion owns all native rows and strings; the source map is released
       before actual engine lookup proves those pointers do not alias it. */
    free(bytes);bytes=NULL;
    CHECK(mac_engine_world_scope(&world,sample_surface,&samples));
    breakable_surfaces_dispose_from_old_map();breakable_surfaces_dispose();
    mac_engine_world_free(&world);
    CHECK(mac_game_core_stop());mac_constructor_time_stop();
    CHECK(mac_engine_arena_stop());free(expected_materials);
    printf("world surfaces: %u checks, %u failures; refs=%u lightmaps=%u materials=%u lightmap=%08lx ground_sample=%d\n",
        checks,failures,reference_count,lightmap_count,material_count,
        (unsigned long)UINT32_C(0xeab3093d),samples.found);
    return failures?1:0;
}
