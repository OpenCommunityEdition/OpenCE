#include "cseries.h"
#include "engine_world.h"
#include "structures/structure_bsp_definitions.h"
#include <stdio.h>
#include <stdlib.h>

extern struct structure_bsp *global_structure_bsp;
static unsigned checks,failures;
static unsigned long total_emitted,total_expected;
#define CHECK(x) do { ++checks; if(!(x)) { if(failures<16)fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
struct world_surface_reference_probe { long surface_index,bsp3d_node_index; };
static int inspect_cluster_lists(void *unused)
{
    struct structure_bsp *bsp=global_structure_bsp;
    struct structure_leaf *leaves;
    struct world_surface_reference_probe *references;
    struct structure_lightmap *lightmaps;
    unsigned char *renderable;
    uint32_t cluster_index,surface_index,lightmap_index,material_index;
    (void)unused;
    CHECK(bsp!=NULL);
    if(!bsp)return 0;
    CHECK(bsp->clusters.count>0&&bsp->leaves.count>0&&bsp->surface_references.count>0&&bsp->surfaces.count>0);
    leaves=(struct structure_leaf *)bsp->leaves.address;
    references=(struct world_surface_reference_probe *)bsp->surface_references.address;
    lightmaps=(struct structure_lightmap *)bsp->lightmaps.address;
    renderable=(unsigned char *)calloc(((uint32_t)bsp->surfaces.count+7)/8,1);
    CHECK(renderable!=NULL);
    if(!renderable)return 0;
    for(lightmap_index=0;lightmap_index<(uint32_t)bsp->lightmaps.count;++lightmap_index) {
        struct structure_material *materials=(struct structure_material *)lightmaps[lightmap_index].materials.address;
        for(material_index=0;material_index<(uint32_t)lightmaps[lightmap_index].materials.count;++material_index) {
            struct structure_material *material=materials+material_index;
            for(surface_index=(uint32_t)material->first_surface_index;
                surface_index<(uint32_t)(material->first_surface_index+material->surface_count);++surface_index)
                renderable[surface_index>>3]|=(unsigned char)(1u<<(surface_index&7));
        }
    }
    for(cluster_index=0;cluster_index<(uint32_t)bsp->clusters.count;++cluster_index) {
        struct tag_block *list=(struct tag_block *)((unsigned char *)bsp->clusters.address+cluster_index*104+68);
        unsigned char *expected=(unsigned char *)calloc(((uint32_t)bsp->surfaces.count+7)/8,1);
        unsigned char *emitted=(unsigned char *)calloc(((uint32_t)bsp->surfaces.count+7)/8,1);
        long *entries=(long *)list->address;
        long cursor=0;
        uint32_t leaf_index;
        CHECK(expected!=NULL&&emitted!=NULL);
        if(!expected||!emitted) { free(expected);free(emitted);break; }
        for(leaf_index=0;leaf_index<(uint32_t)bsp->leaves.count;++leaf_index) {
            struct structure_leaf *leaf=leaves+leaf_index;
            long reference;
            if(leaf->cluster_index!=(short)cluster_index)continue;
            for(reference=leaf->first_surface_reference_index;
                reference<leaf->first_surface_reference_index+leaf->surface_reference_count;++reference) {
                long surface=references[reference].surface_index;
                if(surface>=0&&surface<bsp->surfaces.count)
                    expected[(uint32_t)surface>>3]|=(unsigned char)(1u<<((uint32_t)surface&7));
            }
        }
        CHECK(list->count>=0&&(list->count==0||entries!=NULL));
        while(entries&&cursor<list->count) {
            long lm,mat,count,k;
            if(list->count-cursor<3) { CHECK(FALSE);break; }
            lm=entries[cursor++];mat=entries[cursor++];count=entries[cursor++];
            CHECK(lm>=0&&lm<bsp->lightmaps.count);
            if(lm<0||lm>=bsp->lightmaps.count)break;
            CHECK(mat>=0&&mat<lightmaps[lm].materials.count);
            if(mat<0||mat>=lightmaps[lm].materials.count)break;
            CHECK(count>0&&count<=list->count-cursor);
            if(count<=0||count>list->count-cursor)break;
            for(k=0;k<count;++k) {
                long surface=entries[cursor++];
                struct structure_material *material=(struct structure_material *)lightmaps[lm].materials.address+mat;
                CHECK(surface>=0&&surface<bsp->surfaces.count);
                if(surface<0||surface>=bsp->surfaces.count)continue;
                CHECK(expected[(uint32_t)surface>>3]&(unsigned char)(1u<<((uint32_t)surface&7)));
                CHECK(renderable[(uint32_t)surface>>3]&(unsigned char)(1u<<((uint32_t)surface&7)));
                CHECK(!(emitted[(uint32_t)surface>>3]&(unsigned char)(1u<<((uint32_t)surface&7))));
                emitted[(uint32_t)surface>>3]|=(unsigned char)(1u<<((uint32_t)surface&7));
                CHECK(surface>=material->first_surface_index&&surface<material->first_surface_index+material->surface_count);
                ++total_emitted;
            }
        }
        CHECK(cursor==list->count);
        for(surface_index=0;surface_index<(uint32_t)bsp->surfaces.count;++surface_index)
            if((expected[surface_index>>3]&(unsigned char)(1u<<(surface_index&7)))&&
               (renderable[surface_index>>3]&(unsigned char)(1u<<(surface_index&7)))) {
                CHECK(emitted[surface_index>>3]&(unsigned char)(1u<<(surface_index&7)));
                ++total_expected;
            }
        free(expected);free(emitted);
    }
    CHECK(total_emitted>0&&total_emitted==total_expected);
    free(renderable);
    return failures==0;
}
static unsigned char *read_file(const char *path,size_t *size)
{
    FILE *file=fopen(path,"rb");long length;unsigned char *bytes;
    if(!file)return NULL;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<0||length>0x30000000L||fseek(file,0,SEEK_SET)) { fclose(file);return NULL; }
    bytes=(unsigned char *)malloc((size_t)length);
    if(!bytes||fread(bytes,1,(size_t)length,file)!=(size_t)length) { free(bytes);fclose(file);return NULL; }
    fclose(file);*size=(size_t)length;return bytes;
}
int main(int argc,char **argv)
{
    struct mac_retail_cache cache;struct mac_engine_world world={0};
    unsigned char *bytes;size_t size;const char *error=NULL;int scoped;
    if(argc!=2) { fprintf(stderr,"usage: %s bloodgulch.map\n",argv[0]);return 2; }
    bytes=read_file(argv[1],&size);if(!bytes) { perror(argv[1]);return 2; }
    if(!mac_retail_open(bytes,size,&cache,&error)||
       !mac_engine_world_create(&cache,0,&world,&error)||
       !mac_engine_world_enable_graph(&world,&cache,&error)||
       !mac_engine_world_enable_surfaces(&world,&cache,&error)) {
        fprintf(stderr,"world: %s\n",error?error:"conversion failed");
        mac_engine_world_free(&world);free(bytes);return 1;
    }
    free(bytes);
    scoped=mac_engine_world_scope(&world,inspect_cluster_lists,NULL);
    mac_engine_world_free(&world);
    printf("world cluster surfaces: %u checks, %u failures, emitted=%lu expected=%lu\n",
        checks,failures,total_emitted,total_expected);
    return scoped&&failures==0?0:1;
}
