#include <stdio.h>
#include "cseries.h"
#include "physics/collision_bsp_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "game/game_globals.h"
#include "effects/decal_definitions.h"
#include "objects/objects.h"
#include "physics/breakable_surfaces.h"
#include "bitmaps/bitmap_group.h"
#include "rasterizer/rasterizer_geometry.h"
#include "engine_world.h"
#include "retail_collision.h"
#include "retail_geometry.h"
#include "rasterizer/rasterizer_geometry_environment.h"
#include "retail_spawns.h"
#include "native_scenario_blocks.h"
#include "native_scenario_gameplay.h"
#include "native_scenario_scripts.h"
#include "native_scenario_skies.h"
#include "engine_script_cache.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>
/* port: the loading screen's pump (port/mac/native_tags.c): a map's BSPs take a while to build */
extern void (*mac_loading_pump_hook)(void);
#define LOADING_PUMP() do { if(mac_loading_pump_hook)mac_loading_pump_hook(); } while(0)

/* These two layouts are TU-private in collision_bsp.c; matching native fields
   are witnessed here rather than reinterpreting the portable decoder arrays. */
struct world_collision_leaf { word flags;short count;long first; };
struct world_collision_reference { long plane,root; };
struct world_portal {
    short clusters[2];long plane;real_point3d center;real radius;
    unsigned long flags;long unused[6];struct tag_block vertices;
};
struct world_cluster_runtime_decals {
    byte unused[0xC];
    short first_decal_index;
    word decal_count;
    byte unused2[0x58];
};
struct world_surface_reference { long surface_index,bsp3d_node_index; };
struct world_visibility_cluster {
    byte header[68];
    struct tag_block surface_indices;
    struct tag_block mirrors;
    struct tag_block portal_indices;
};
struct world_environment_vertex_compressed {
    real_point3d position;
    unsigned long normal,binormal,tangent;
    real_point2d texcoord;
};
struct world_environment_lightmap_vertex_compressed {
    unsigned long incident_radiosity;
    short lightmap_u,lightmap_v;
};
struct world_state {
    struct collision_bsp collision;
    struct structure_bsp structure;
    struct scenario scenario;
    struct mac_native_scenario_blocks scenario_objects;
    struct mac_native_scenario_gameplay scenario_gameplay;
    struct mac_native_scenario_scripts scenario_scripts;
    struct mac_native_scenario_skies scenario_skies;
    struct scenario_structure_bsp_reference references[32];
    struct player_starting_location spawns[MAC_RETAIL_MAX_SPAWNS];
    uint32_t fog_planes,fog_regions,fog_palette,runtime_decal_count;
    uint32_t scenario_decal_palette_count,clusters_with_fog,clusters_with_decals;
    uint32_t bsp,scenario_handle,lightmap_handle;
    int graph_ready,render_surfaces_ready;
    /* port: a world for a BSP other than the session's own holds that BSP
       only (the scenario is the primary world's); the primary keeps the
       others to switch to (scenario_switch_structure_bsp), and which BSP is
       the active one */
    int bsp_only;
    struct world_state *others[32];
    uint32_t bsp_count,active_bsp;
};
static struct world_state *primary_world;
extern struct structure_bsp *global_structure_bsp;
static int querying;
#define WITNESS(name,test) typedef char name[(test)?1:-1]
WITNESS(world_portal_size,sizeof(struct world_portal)==64);
WITNESS(world_portal_vertices,offsetof(struct world_portal,vertices)==52);
WITNESS(world_portals_offset,offsetof(struct structure_bsp,cluster_portals)==0x154);
WITNESS(world_collision_leaf_size,sizeof(struct world_collision_leaf)==8);
WITNESS(world_collision_reference_size,sizeof(struct world_collision_reference)==8);
WITNESS(world_collision_material_size,sizeof(struct structure_collision_material)==20);
WITNESS(world_collision_material_offset,offsetof(struct structure_bsp,collision_materials)==0xa4);
WITNESS(world_collision_material_type_offset,offsetof(struct structure_collision_material,runtime_physics_material_type)==18);
WITNESS(world_runtime_decal_size,sizeof(struct structure_runtime_decal)==16);
WITNESS(world_runtime_decal_palette_offset,offsetof(struct structure_runtime_decal,palette_index)==12);
WITNESS(world_runtime_decal_block_offset,offsetof(struct structure_bsp,runtime_decals)==0x258);
WITNESS(world_cluster_runtime_decals_size,sizeof(struct world_cluster_runtime_decals)==0x68);
WITNESS(world_cluster_runtime_decals_first,offsetof(struct world_cluster_runtime_decals,first_decal_index)==0x0c);
WITNESS(world_cluster_runtime_decals_count,offsetof(struct world_cluster_runtime_decals,decal_count)==0x0e);
WITNESS(world_scenario_decal_palette_entry_size,sizeof(struct scenario_decal_palette_entry)==16);
WITNESS(world_scenario_decal_palette_offset,offsetof(struct scenario,decal_palette)==0x3b4);
WITNESS(world_collision_bsp_size,sizeof(struct collision_bsp)==96);
WITNESS(world_structure_leaf_size,sizeof(struct structure_leaf)==16);
WITNESS(world_structure_leaf_cluster,offsetof(struct structure_leaf,cluster_index)==8);
WITNESS(world_structure_leaves,offsetof(struct structure_bsp,leaves)==0xe0);
WITNESS(world_surface_references_size,sizeof(struct world_surface_reference)==8);
WITNESS(world_surface_references_offset,offsetof(struct structure_bsp,surface_references)==0xec);
WITNESS(world_surfaces_offset,offsetof(struct structure_bsp,surfaces)==0xf8);
WITNESS(world_lightmaps_offset,offsetof(struct structure_bsp,lightmaps)==0x104);
WITNESS(world_surface_size,sizeof(struct structure_surface)==6);
WITNESS(world_lightmap_size,sizeof(struct structure_lightmap)==32);
WITNESS(world_lightmap_material_offset,offsetof(struct structure_lightmap,materials)==20);
WITNESS(world_material_size,sizeof(struct structure_material)==0x100);
WITNESS(world_material_first_surface,offsetof(struct structure_material,first_surface_index)==20);
WITNESS(world_material_surface_count,offsetof(struct structure_material,surface_count)==24);
WITNESS(world_material_vertices,offsetof(struct structure_material,vertices)==176);
WITNESS(world_material_lightmap_vertices,offsetof(struct structure_material,lightmap_vertices)==196);
WITNESS(world_material_compressed_data,offsetof(struct structure_material,compressed_vertex_data)==236);
WITNESS(world_environment_vertex_compressed_size,sizeof(struct world_environment_vertex_compressed)==32);
WITNESS(world_environment_lightmap_vertex_compressed_size,sizeof(struct world_environment_lightmap_vertex_compressed)==8);
WITNESS(world_lightmap_group_offset,offsetof(struct structure_bsp,lightmap_group)==0);
WITNESS(world_lightmap_group_reference_size,sizeof(struct tag_reference)==16);
WITNESS(world_structure_clusters,offsetof(struct structure_bsp,clusters)==0x134);
WITNESS(world_visibility_cluster_surface_list,offsetof(struct world_visibility_cluster,surface_indices)==68);
WITNESS(world_visibility_cluster_portal_list,offsetof(struct world_visibility_cluster,portal_indices)==92);
WITNESS(world_visibility_cluster_size,sizeof(struct world_visibility_cluster)==104);
WITNESS(world_scenario_spawns,offsetof(struct scenario,players)==0x354);
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t u16(const unsigned char *p) { return (uint16_t)(p[0]|p[1]<<8); }
static int16_t s16(const unsigned char *p)
{ uint16_t bits=u16(p);int16_t value;memcpy(&value,&bits,2);return value; }
static long signed_word(uint32_t bits) { long result;memcpy(&result,&bits,4);return result; }
static int finite_float(float x)
{ uint32_t bits;memcpy(&bits,&x,4);return (bits&UINT32_C(0x7fffffff))<UINT32_C(0x7f800000); }
static uint32_t tag_name_length(const char *name);
static int cache_handle_valid(const struct mac_retail_cache *cache,uint32_t group,uint32_t handle)
{
    struct mac_retail_tag tag;
    uint32_t index;
    if(handle==UINT32_MAX)return 1;
    index=handle&UINT32_C(0xffff);
    return index<cache->tag_count && mac_retail_tag_get(cache,index,&tag) &&
        tag.handle==handle && tag.group==group;
}
static int copy_tag_reference(const struct mac_retail_cache *cache,const unsigned char *raw,
    struct tag_reference *reference)
{
    uint32_t group=u32(raw),name_address=u32(raw+4),name_length=u32(raw+8),handle=u32(raw+12);
    struct mac_retail_tag tag;
    uint32_t actual_length;
    reference->group_tag=group;reference->index=signed_word(handle);
    reference->name=NULL;reference->name_length=0;
    if(handle==UINT32_MAX)return name_address==0 && name_length==0;
    if((handle&UINT32_C(0xffff))>=cache->tag_count ||
       !mac_retail_tag_get(cache,handle&UINT32_C(0xffff),&tag) || tag.handle!=handle ||
       (tag.group!=group && tag.parents[0]!=group && tag.parents[1]!=group))return 0;
    actual_length=tag_name_length(tag.name);
    if(name_address!=u32(cache->tags+cache->instances_offset+(handle&UINT32_C(0xffff))*32+16) ||
       (name_length && name_length!=actual_length))return 0;
    reference->name=(char *)malloc((size_t)actual_length+1);
    if(!reference->name)return 0;
    memcpy(reference->name,tag.name,(size_t)actual_length+1);
    reference->name_length=(long)actual_length;
    return 1;
}
struct world_geometry_binding {
    struct structure_material **materials;
    short *lightmap_bitmaps;
    uint32_t count,next;
};
static int bind_geometry_material(void *context,const struct mac_retail_material *decoded)
{
    struct world_geometry_binding *binding=(struct world_geometry_binding *)context;
    struct structure_material *material;
    unsigned long packed_size,environment_size,lightmap_size;
    unsigned char *packed;
    if(binding->next>=binding->count)return 0;
    material=binding->materials[binding->next];
    if(decoded->shader_handle!=(uint32_t)material->shader.index ||
       decoded->lightmap_bitmap!=binding->lightmap_bitmaps[binding->next] ||
       decoded->vertex_count!=(uint32_t)material->vertices.count ||
       decoded->light_vertex_count!=(uint32_t)material->lightmap_vertices.count ||
       decoded->triangle_count!=(uint32_t)material->surface_count)return 0;
    environment_size=decoded->vertex_count*sizeof(struct world_environment_vertex_compressed);
    lightmap_size=decoded->light_vertex_count*sizeof(struct world_environment_lightmap_vertex_compressed);
    packed_size=environment_size+lightmap_size;
    if(packed_size>0x7fffffffUL)return 0;
    if(packed_size) {
        packed=(unsigned char *)malloc(packed_size);
        if(!packed)return 0;
        if(decoded->vertex_count)
            rasterizer_geometry_compress_vertices(_rasterizer_vertex_type_environment_uncompressed,
                (long)decoded->vertex_count,packed,environment_size,
                (void *)decoded->vertices,decoded->vertex_count*sizeof(struct mac_retail_vertex));
        if(decoded->light_vertex_count)
            rasterizer_geometry_compress_vertices(_rasterizer_vertex_type_environment_lightmap_uncompressed,
                (long)decoded->light_vertex_count,packed+environment_size,lightmap_size,
                (void *)decoded->light_vertices,decoded->light_vertex_count*sizeof(struct mac_retail_light_vertex));
    } else packed=NULL;
    material->compressed_vertex_data.size=(long)packed_size;
    material->compressed_vertex_data.address=packed;
    material->vertices.type=_rasterizer_vertex_type_environment_compressed;
    material->vertices.count=(long)decoded->vertex_count;
    material->vertices.base_address=packed;
    material->lightmap_vertices.type=_rasterizer_vertex_type_environment_lightmap_compressed;
    material->lightmap_vertices.count=(long)decoded->light_vertex_count;
    material->lightmap_vertices.base_address=packed?packed+environment_size:NULL;
    ++binding->next;
    return 1;
}
static uint32_t tag_name_length(const char *name)
{
    uint32_t length=0;
    while(length<256 && name[length])++length;
    return length;
}
static const unsigned char *span(const struct mac_retail_cache *cache,const struct mac_retail_bsp *bsp,uint32_t address,uint32_t size)
{
    uint32_t offset;
    if(address<bsp->base || (address&3))return NULL;
    offset=address-bsp->base;
    if(offset<24 || offset>bsp->size || size>bsp->size-offset)return NULL;
    return cache->file+bsp->file_offset+offset;
}
static void *allocate_block(struct tag_block *block,uint32_t count,size_t stride)
{
    void *result;
    if(count>UINT32_C(0x7fffffff)/stride)return NULL;
    result=calloc(count?count:1,stride);
    if(result) { block->count=count;block->address=result; }
    return result;
}
/* Planar fog (structures.c): the BSP's fog planes (structures.c's
   structure_fog_plane_render: the 12 "unused" bytes on disk are the plane's
   vertex block), fog regions and fog palette, which clusters name by their
   fog designator. The fog tags themselves are loaded with the session's
   assets (mac_engine_world_fog_handles). */
struct world_fog_plane { short region_index;word pad;real_plane3d plane;struct tag_block vertices; };
WITNESS(world_fog_plane_size,sizeof(struct world_fog_plane)==0x20);
static int bind_fog(const struct mac_retail_cache *cache,const struct mac_retail_bsp *bsp,
    const unsigned char *root,struct world_state *s)
{
    uint32_t planes=u32(root+offsetof(struct structure_bsp,fog_planes));
    uint32_t regions=u32(root+offsetof(struct structure_bsp,fog_regions));
    uint32_t palette=u32(root+offsetof(struct structure_bsp,fog_palette));
    const unsigned char *raw;uint32_t i,j,k;
    if(planes>1024 || regions>1024 || palette>256)return 0;
    if(planes) {
        struct world_fog_plane *out;
        raw=span(cache,bsp,u32(root+offsetof(struct structure_bsp,fog_planes)+4),planes*0x20);if(!raw)return 0;
        out=(struct world_fog_plane *)allocate_block(&s->structure.fog_planes,planes,sizeof(*out));if(!out)return 0;
        for(i=0;i<planes;++i) {
            const unsigned char *q=raw+i*0x20,*v;uint32_t n;real_point3d *vertices;
            out[i].region_index=s16(q);
            if(out[i].region_index<NONE || (out[i].region_index>=0 && (uint32_t)out[i].region_index>=regions))return 0;
            for(j=0;j<4;++j) {
                uint32_t bits=u32(q+4+j*4);float f;memcpy(&f,&bits,4);if(!finite_float(f))return 0;
                if(j<3)out[i].plane.n.n[j]=f;else out[i].plane.d=f;
            }
            n=u32(q+0x14);if(n>1024)return 0;
            if(!n)continue;
            v=span(cache,bsp,u32(q+0x18),n*12);if(!v)return 0;
            vertices=(real_point3d *)allocate_block(&out[i].vertices,n,sizeof(*vertices));if(!vertices)return 0;
            for(j=0;j<n;++j)for(k=0;k<3;++k) {
                uint32_t bits=u32(v+j*12+k*4);float f;memcpy(&f,&bits,4);if(!finite_float(f))return 0;
                vertices[j].n[k]=f;
            }
        }
    }
    if(regions) {
        struct structure_fog_region *out;
        raw=span(cache,bsp,u32(root+offsetof(struct structure_bsp,fog_regions)+4),regions*0x28);if(!raw)return 0;
        out=(struct structure_fog_region *)allocate_block(&s->structure.fog_regions,regions,sizeof(*out));if(!out)return 0;
        for(i=0;i<regions;++i) {
            const unsigned char *q=raw+i*0x28;
            memcpy(out[i].unused,q,sizeof(out[i].unused));
            out[i].fog_palette_index=s16(q+0x24);
            /* (the weather palette is not converted: no region names one) */
            out[i].weather_palette_index=NONE;
            if(out[i].fog_palette_index<NONE || (out[i].fog_palette_index>=0 && (uint32_t)out[i].fog_palette_index>=palette))return 0;
        }
    }
    if(palette) {
        struct structure_fog_palette_entry *out;
        raw=span(cache,bsp,u32(root+offsetof(struct structure_bsp,fog_palette)+4),palette*0x88);if(!raw)return 0;
        out=(struct structure_fog_palette_entry *)allocate_block(&s->structure.fog_palette,palette,sizeof(*out));if(!out)return 0;
        for(i=0;i<palette;++i) {
            const unsigned char *q=raw+i*0x88;uint32_t group=u32(q+0x20),handle=u32(q+0x2c);
            memcpy(out[i].unused,q,sizeof(out[i].unused));memcpy(out[i].unused2,q+0x30,sizeof(out[i].unused2));
            out[i].fog.group_tag=group;out[i].fog.index=signed_word(handle);
            out[i].fog.name=NULL;out[i].fog.name_length=0;
            if(handle!=UINT32_MAX) {
                struct mac_retail_tag tag;
                if(!mac_retail_tag_get(cache,handle&UINT32_C(0xffff),&tag) || tag.handle!=handle ||
                   tag.group!=UINT32_C(0x666f6720))return 0;
            }
        }
    }
    for(i=0;i<(uint32_t)s->structure.clusters.count;++i) {
        short designator=((struct structure_cluster *)s->structure.clusters.address)[i].fog_reference;
        if(designator==NONE)continue;
        if(designator&SHORT_MIN?((uint32_t)(designator&SHORT_MAX)>=planes):((uint32_t)designator>=regions))return 0;
    }
    return 1;
}
/* Breakable surfaces (breakable_surfaces.c, Prisoner's glass): the BSP's
   records, whose collision surfaces name them back; materials that draw
   them are checked against them when the surfaces are enabled. */
static int bind_breakable_surfaces(const struct mac_retail_cache *cache,const struct mac_retail_bsp *bsp,
    const unsigned char *root,struct world_state *s)
{
    uint32_t count=u32(root+offsetof(struct structure_bsp,breakable_surfaces)),i,j;
    const unsigned char *raw;struct structure_breakable_surface *out;
    const struct collision_surface *surfaces=(const struct collision_surface *)s->collision.surfaces.address;
    if(count>MAXIMUM_BREAKABLE_SURFACES_PER_MAP)return 0;
    /* (a collision surface's index means something only on one with the
       breakable flag, collision_bsp.c's bit 3) */
    for(i=0;i<(uint32_t)s->collision.surfaces.count;++i)
        if((surfaces[i].flags&8) && (uint32_t)surfaces[i].breakable_surface_index>=count)return 0;
    if(!count)return 1;
    raw=span(cache,bsp,u32(root+offsetof(struct structure_bsp,breakable_surfaces)+4),count*0x30);if(!raw)return 0;
    out=(struct structure_breakable_surface *)allocate_block(&s->structure.breakable_surfaces,count,sizeof(*out));if(!out)return 0;
    for(i=0;i<count;++i) {
        const unsigned char *q=raw+i*0x30;
        for(j=0;j<4;++j) {
            uint32_t bits=u32(q+j*4);float f;memcpy(&f,&bits,4);if(!finite_float(f))return 0;
            if(j<3)out[i].centroid.n[j]=f;else out[i].bounding_radius=f;
        }
        if(out[i].bounding_radius<0.f)return 0;
        out[i].collision_surface_index=signed_word(u32(q+16));
        if(out[i].collision_surface_index<0 || out[i].collision_surface_index>=s->collision.surfaces.count)return 0;
        for(j=0;j<7;++j)out[i].unused[j]=signed_word(u32(q+20+j*4));
    }
    return 1;
}
int mac_engine_world_fog_handles(const struct mac_engine_world *world,uint32_t *handles,uint32_t capacity,uint32_t *count)
{
    const struct world_state *s;uint32_t i;
    if(count)*count=0;
    if(!world || !world->state || !handles || !count)return 0;
    s=(const struct world_state *)world->state;
    if((uint32_t)s->structure.fog_palette.count>capacity)return 0;
    for(i=0;i<(uint32_t)s->structure.fog_palette.count;++i) {
        long index=((const struct structure_fog_palette_entry *)s->structure.fog_palette.address)[i].fog.index;
        uint32_t handle;memcpy(&handle,&index,4);handles[i]=handle;
    }
    *count=(uint32_t)s->structure.fog_palette.count;return 1;
}
/* BSP sound palettes contain opaque labels/reserved bytes and one native
   reference. Never retain a disk pointer or a disk block definition. */
static int bind_sound_palette(const struct mac_retail_cache *cache,
    const struct mac_retail_bsp *bsp,const unsigned char *raw_block,
    struct tag_block *block,uint32_t stride,uint32_t group)
{
    uint32_t count=u32(raw_block),i;
    const unsigned char *raw;
    unsigned char *entries;
    if(count>64)return 0;
    if(!count)return 1;
    raw=span(cache,bsp,u32(raw_block+4),count*stride);
    if(!raw)return 0;
    entries=allocate_block(block,count,stride);if(!entries)return 0;
    for(i=0;i<count;++i) {
        struct tag_reference *reference=(struct tag_reference *)(entries+i*stride+32);
        const unsigned char *source=raw+i*stride;
        memcpy(entries+i*stride,source,32);
        memcpy(entries+i*stride+48,source+48,stride-48);
        if(u32(source+32)!=group || !copy_tag_reference(cache,source+32,reference))return 0;
    }
    return 1;
}
static void free_sound_palette(struct tag_block *block,size_t stride)
{
    long i;unsigned char *entries=block->address;
    for(i=0;i<block->count;++i)
        free(((struct tag_reference *)(entries+i*stride+32))->name);
    free(entries);
}
static int copy_reals(const unsigned char *raw,void *output,unsigned count)
{
    unsigned i;unsigned char *destination=output;
    for(i=0;i<count;++i) {
        uint32_t bits=u32(raw+4*i);real value;
        memcpy(&value,&bits,4);if(!finite_float(value))return 0;
        memcpy(destination+4*i,&value,4);
    }
    return 1;
}
static int copy_lighting(const unsigned char *raw,struct render_lighting *lighting)
{
    unsigned i;
    lighting->distant_light_count=s16(raw+offsetof(struct render_lighting,distant_light_count));
    lighting->point_light_count=s16(raw+offsetof(struct render_lighting,point_light_count));
    /* Cached default/material lighting cannot refer to runtime point lights;
       this is also the shared render_lighting_schema's constraint. */
    if(lighting->distant_light_count<0 || lighting->distant_light_count>MAXIMUM_RENDERED_DISTANT_LIGHTS ||
       lighting->point_light_count!=0)return 0;
    lighting->pad=u16(raw+offsetof(struct render_lighting,pad));
    lighting->pad1=u16(raw+offsetof(struct render_lighting,pad1));
    for(i=0;i<MAXIMUM_RENDERED_POINT_LIGHTS;++i)
        lighting->point_light_indices[i]=signed_word(u32(raw+offsetof(struct render_lighting,point_light_indices)+4*i));
    return copy_reals(raw+offsetof(struct render_lighting,ambient_color),&lighting->ambient_color,3) &&
        copy_reals(raw+offsetof(struct render_lighting,distant_lights),lighting->distant_lights,6*MAXIMUM_RENDERED_DISTANT_LIGHTS) &&
        copy_reals(raw+offsetof(struct render_lighting,reflection_tint_color),&lighting->reflection_tint_color,4) &&
        copy_reals(raw+offsetof(struct render_lighting,shadow_vector),&lighting->shadow_vector,3) &&
        copy_reals(raw+offsetof(struct render_lighting,shadow_color),&lighting->shadow_color,3);
}
static int build_cluster_surface_lists(struct world_state *s,const char **error)
{
    struct structure_leaf *leaves=(struct structure_leaf *)s->structure.leaves.address;
    struct world_surface_reference *references=(struct world_surface_reference *)s->structure.surface_references.address;
    struct structure_lightmap *lightmaps=(struct structure_lightmap *)s->structure.lightmaps.address;
    unsigned char *membership;
    size_t row_bytes,total_bytes;
    uint32_t cluster_count=(uint32_t)s->structure.clusters.count;
    uint32_t surface_count=(uint32_t)s->structure.surfaces.count;
    uint32_t reference_count=(uint32_t)s->structure.surface_references.count;
    uint32_t cluster_index,leaf_index,reference_index,lightmap_index,material_index;
    uint32_t word_limit,words,total_words=0;
    if(error)*error="world cluster surface lists";
    if(!cluster_count || cluster_count>MAXIMUM_CLUSTERS_PER_STRUCTURE ||
       surface_count>MAXIMUM_SURFACES_PER_STRUCTURE || reference_count>131072)
        return 0;
    row_bytes=((size_t)surface_count+7)/8;
    if(row_bytes && cluster_count>((size_t)-1)/row_bytes)return 0;
    total_bytes=(size_t)cluster_count*row_bytes;
    /* At most one membership per leaf reference and one three-word group
       header per nonempty material group. */
    if(reference_count>UINT32_MAX/4)return 0;
    word_limit=reference_count*4;
    membership=(unsigned char *)calloc(total_bytes?total_bytes:1,1);
    if(!membership)return 0;
    for(leaf_index=0;leaf_index<(uint32_t)s->structure.leaves.count;++leaf_index) {
        struct structure_leaf *leaf=leaves+leaf_index;
        uint32_t first,count;
        unsigned char *row;
        if(leaf->cluster_index<0 || (uint32_t)leaf->cluster_index>=cluster_count ||
           leaf->surface_reference_count<=0)continue;
        first=(uint32_t)leaf->first_surface_reference_index;
        count=(uint32_t)leaf->surface_reference_count;
        if(first>reference_count || count>reference_count-first) {
            free(membership);return 0;
        }
        row=membership+(size_t)leaf->cluster_index*row_bytes;
        for(reference_index=first;reference_index<first+count;++reference_index) {
            long surface=references[reference_index].surface_index;
            if(surface==NONE)continue;
            if(surface<0 || (uint32_t)surface>=surface_count) {
                free(membership);return 0;
            }
            row[(uint32_t)surface>>3]|=(unsigned char)(1u<<((uint32_t)surface&7));
        }
    }
    for(cluster_index=0;cluster_index<cluster_count;++cluster_index) {
        unsigned char *row=membership+(size_t)cluster_index*row_bytes;
        LOADING_PUMP();
        for(lightmap_index=0;lightmap_index<(uint32_t)s->structure.lightmaps.count;++lightmap_index) {
            struct structure_material *materials=(struct structure_material *)lightmaps[lightmap_index].materials.address;
            for(material_index=0;material_index<(uint32_t)lightmaps[lightmap_index].materials.count;++material_index) {
                struct structure_material *material=materials+material_index;
                uint32_t surface,found=0;
                for(surface=(uint32_t)material->first_surface_index;
                    surface<(uint32_t)(material->first_surface_index+material->surface_count);++surface)
                    if(row[surface>>3]&(unsigned char)(1u<<(surface&7)))++found;
                if(found) {
                    if(found>UINT32_MAX-3 || found+3>word_limit ||
                       total_words>word_limit-(found+3)) {
                        free(membership);return 0;
                    }
                    total_words+=found+3;
                }
            }
        }
    }
    if(total_words>word_limit) { free(membership);return 0; }
    for(cluster_index=0;cluster_index<cluster_count;++cluster_index) {
        struct world_visibility_cluster *cluster=(struct world_visibility_cluster *)
            ((unsigned char *)s->structure.clusters.address+cluster_index*sizeof(struct world_visibility_cluster));
        unsigned char *row=membership+(size_t)cluster_index*row_bytes;
        long *output;
        LOADING_PUMP();
        words=0;
        for(lightmap_index=0;lightmap_index<(uint32_t)s->structure.lightmaps.count;++lightmap_index) {
            struct structure_material *materials=(struct structure_material *)lightmaps[lightmap_index].materials.address;
            for(material_index=0;material_index<(uint32_t)lightmaps[lightmap_index].materials.count;++material_index) {
                struct structure_material *material=materials+material_index;
                uint32_t surface,found=0;
                for(surface=(uint32_t)material->first_surface_index;
                    surface<(uint32_t)(material->first_surface_index+material->surface_count);++surface)
                    if(row[surface>>3]&(unsigned char)(1u<<(surface&7)))++found;
                if(found)words+=found+3;
            }
        }
        if(!words)continue;
        if(words>word_limit || !allocate_block(&cluster->surface_indices,words,sizeof(long))) {
            free(membership);return 0;
        }
        output=(long *)cluster->surface_indices.address;
        for(lightmap_index=0;lightmap_index<(uint32_t)s->structure.lightmaps.count;++lightmap_index) {
            struct structure_material *materials=(struct structure_material *)lightmaps[lightmap_index].materials.address;
            for(material_index=0;material_index<(uint32_t)lightmaps[lightmap_index].materials.count;++material_index) {
                struct structure_material *material=materials+material_index;
                uint32_t surface,found=0;
                for(surface=(uint32_t)material->first_surface_index;
                    surface<(uint32_t)(material->first_surface_index+material->surface_count);++surface)
                    if(row[surface>>3]&(unsigned char)(1u<<(surface&7)))++found;
                if(!found)continue;
                *output++=(long)lightmap_index;
                *output++=(long)material_index;
                *output++=(long)found;
                for(surface=(uint32_t)material->first_surface_index;
                    surface<(uint32_t)(material->first_surface_index+material->surface_count);++surface)
                    if(row[surface>>3]&(unsigned char)(1u<<(surface&7)))
                        *output++=(long)surface;
            }
        }
        if(output-(long *)cluster->surface_indices.address!=(long)cluster->surface_indices.count) {
            free(membership);return 0;
        }
    }
    free(membership);
    if(error)*error=NULL;
    return 1;
}
static int bind_render_surface_data(struct world_state *s,const struct mac_retail_cache *cache,
    const struct mac_retail_bsp *bsp,const unsigned char *root,const char **error)
{
    const unsigned char *raw_references,*raw_surfaces,*raw_lightmaps,*raw_bitmap_root,*raw_bitmap_rows;
    const unsigned char *raw_materials;
    struct world_surface_reference *references;
    struct structure_surface *surfaces;
    struct structure_lightmap *lightmaps;
    struct structure_material **material_slots=NULL;
    short *lightmap_bitmaps=NULL;
    struct world_geometry_binding binding;
    struct mac_retail_tag bitmap_tag;
    uint32_t reference_count=u32(root+0xec),surface_count=u32(root+0xf8);
    uint32_t lightmap_count=u32(root+0x104),bitmap_count=0,total_materials=0;
    uint32_t i,j,flat=0,previous_end=0;
    int has_previous=0;
    uint32_t lightmap_handle=u32(root+12);
    if(error)*error=NULL;
#define SURFACE_FAIL(message) do { if(error)*error=(message);free(material_slots);free(lightmap_bitmaps);return 0; } while(0)
    if(reference_count>131072 || surface_count>131072 || lightmap_count>128)
        SURFACE_FAIL("world render-surface block count");
    raw_references=reference_count?span(cache,bsp,u32(root+0xf0),reference_count*8):NULL;
    raw_surfaces=surface_count?span(cache,bsp,u32(root+0xfc),surface_count*6):NULL;
    raw_lightmaps=lightmap_count?span(cache,bsp,u32(root+0x108),lightmap_count*32):NULL;
    if((reference_count&&!raw_references)||(surface_count&&!raw_surfaces)||(lightmap_count&&!raw_lightmaps))
        SURFACE_FAIL("world render-surface span");
    if(u32(root)!=BITMAP_GROUP_TAG ||
       !copy_tag_reference(cache,root,&s->structure.lightmap_group))
        SURFACE_FAIL("world lightmap group reference");
    s->lightmap_handle=lightmap_handle;
    if(lightmap_handle!=UINT32_MAX) {
        if(!cache_handle_valid(cache,BITMAP_GROUP_TAG,lightmap_handle) ||
           !mac_retail_tag_get(cache,lightmap_handle&UINT32_C(0xffff),&bitmap_tag))
            SURFACE_FAIL("world lightmap group handle");
        raw_bitmap_root=mac_retail_tag_span(cache,bitmap_tag.address,0x6c);
        if(!raw_bitmap_root)SURFACE_FAIL("world lightmap group tag range");
        bitmap_count=u32(raw_bitmap_root+0x60);
        if(bitmap_count>65536)SURFACE_FAIL("world lightmap bitmap count");
        raw_bitmap_rows=bitmap_count?mac_retail_tag_span(cache,u32(raw_bitmap_root+0x64),bitmap_count*0x30):NULL;
        if(bitmap_count&&(!raw_bitmap_rows||(u32(raw_bitmap_root+0x64)&3)))
            SURFACE_FAIL("world lightmap bitmap block");
    } else {
        raw_bitmap_rows=NULL;
    }
    for(i=0;i<reference_count;++i) {
        int32_t surface_index=(int32_t)signed_word(u32(raw_references+i*8));
        int32_t node_index=(int32_t)signed_word(u32(raw_references+i*8+4));
        if((surface_index!=NONE && (surface_index<0||(uint32_t)surface_index>=surface_count)) ||
           (node_index!=NONE && (node_index<0||(uint32_t)node_index>=s->collision.bsp3d.nodes.count)) ||
           (node_index!=NONE && surface_index==NONE))
            SURFACE_FAIL("world surface-reference index");
    }
    for(i=0;i<lightmap_count;++i) {
        const unsigned char *row=raw_lightmaps+i*32;
        int16_t bitmap_index=s16(row);
        uint32_t count=u32(row+20),address=u32(row+24);
        if(bitmap_index!=NONE && (bitmap_index<0||(uint32_t)bitmap_index>=bitmap_count))
            SURFACE_FAIL("world lightmap bitmap index");
        if(count>2048 || count>65536-total_materials)
            SURFACE_FAIL("world lightmap material count");
        raw_materials=count?span(cache,bsp,address,count*0x100):NULL;
        if(count&&!raw_materials)SURFACE_FAIL("world lightmap material span");
        total_materials+=count;
    }
    if(!allocate_block(&s->structure.surface_references,reference_count,sizeof(*references)) ||
       !allocate_block(&s->structure.surfaces,surface_count,sizeof(*surfaces)) ||
       !allocate_block(&s->structure.lightmaps,lightmap_count,sizeof(*lightmaps)))
        SURFACE_FAIL("world render-surface allocation");
    references=(struct world_surface_reference *)s->structure.surface_references.address;
    for(i=0;i<reference_count;++i) {
        references[i].surface_index=signed_word(u32(raw_references+i*8));
        references[i].bsp3d_node_index=signed_word(u32(raw_references+i*8+4));
    }
    surfaces=(struct structure_surface *)s->structure.surfaces.address;
    for(i=0;i<surface_count;++i)for(j=0;j<3;++j)
        surfaces[i].vertex_indices[j]=u16(raw_surfaces+i*6+j*2);
    lightmaps=(struct structure_lightmap *)s->structure.lightmaps.address;
    if(total_materials) {
        material_slots=(struct structure_material **)calloc(total_materials,sizeof(*material_slots));
        lightmap_bitmaps=(short *)calloc(total_materials,sizeof(*lightmap_bitmaps));
        if(!material_slots||!lightmap_bitmaps)SURFACE_FAIL("world material binding allocation");
    }
    for(i=0;i<lightmap_count;++i) {
        const unsigned char *row=raw_lightmaps+i*32;
        uint32_t count=u32(row+20),address=u32(row+24);
        uint32_t first_in_lightmap=flat;
        int16_t bitmap_index=s16(row);
        lightmaps[i].bitmap_index=bitmap_index;lightmaps[i].pad=u16(row+2);
        lightmaps[i].materials.count=0;lightmaps[i].materials.address=NULL;
        if(!allocate_block(&lightmaps[i].materials,count,sizeof(struct structure_material)))
            SURFACE_FAIL("world material allocation");
        raw_materials=count?span(cache,bsp,address,count*0x100):NULL;
        for(j=0;j<count;++j) {
            const unsigned char *raw=raw_materials+j*0x100;
            struct structure_material *material=(struct structure_material *)lightmaps[i].materials.address+j;
            int32_t first=(int32_t)signed_word(u32(raw+20));
            int32_t number=(int32_t)signed_word(u32(raw+24));
            int16_t vertex_type=s16(raw+0xb0);
            int32_t vertex_count=(int32_t)signed_word(u32(raw+0xb4));
            int32_t light_vertex_count=(int32_t)signed_word(u32(raw+0xc8));
            int32_t vertex_bytes=(int32_t)signed_word(u32(raw+0xd8));
            uint32_t expected_bytes,vertex_address=u32(raw+0xe4);
            if(first<0||number<0||(uint32_t)first>surface_count||
               (uint32_t)number>surface_count-(uint32_t)first||
               (has_previous&&(uint32_t)first<previous_end))
                SURFACE_FAIL("world material surface range");
            if(vertex_type!=_rasterizer_vertex_type_environment_uncompressed ||
               vertex_count<0||vertex_count>64000||light_vertex_count<0||
               light_vertex_count!=(bitmap_index!=NONE?vertex_count:0))
                SURFACE_FAIL("world material vertex layout");
            expected_bytes=(uint32_t)vertex_count*sizeof(struct mac_retail_vertex)+
                (uint32_t)light_vertex_count*sizeof(struct mac_retail_light_vertex);
            if(vertex_bytes<0||(uint32_t)vertex_bytes!=expected_bytes||
               (expected_bytes&&!span(cache,bsp,vertex_address,expected_bytes)))
                SURFACE_FAIL("world material vertex span");
            if(!copy_reals(raw+offsetof(struct structure_material,centroid),&material->centroid,3) ||
               !copy_lighting(raw+offsetof(struct structure_material,lighting),&material->lighting) ||
               !copy_reals(raw+offsetof(struct structure_material,plane),&material->plane,4))
                SURFACE_FAIL("world material lighting or plane");
            material->breakable_surface_index=s16(raw+offsetof(struct structure_material,breakable_surface_index));
            material->copious_unused_space=u16(raw+offsetof(struct structure_material,copious_unused_space));
            if(material->breakable_surface_index<NONE ||
               (material->breakable_surface_index>=0 &&
                material->breakable_surface_index>=s->structure.breakable_surfaces.count))
                SURFACE_FAIL("world breakable material index");
            material->first_surface_index=first;material->surface_count=number;
            /* The geometry visitor uses these native counts to match each
               validated decoded material back to its disk row. The payload
               and packed pointer are installed only by the visitor. */
            material->vertices.count=vertex_count;
            material->lightmap_vertices.count=light_vertex_count;
            material->permutation_index=s16(raw+16);material->flags=u16(raw+18);
            if(!copy_tag_reference(cache,raw,&material->shader))
                SURFACE_FAIL("world material shader reference");
            material_slots[flat]=material;lightmap_bitmaps[flat]=bitmap_index;++flat;
            if(number) { previous_end=(uint32_t)first+(uint32_t)number;has_previous=1; }
        }
        if(flat-first_in_lightmap!=count)SURFACE_FAIL("world material binding order");
    }
    binding.materials=material_slots;binding.lightmap_bitmaps=lightmap_bitmaps;
    binding.count=total_materials;binding.next=0;
    if(!mac_retail_bsp_geometry(cache,s->bsp,bind_geometry_material,&binding,error)) {
        if(error&&!*error)*error="world render geometry";
        free(material_slots);free(lightmap_bitmaps);return 0;
    }
    if(binding.next!=total_materials)SURFACE_FAIL("world material geometry count");
    free(material_slots);free(lightmap_bitmaps);
    if (!build_cluster_surface_lists(s,error))
        return 0;
#undef SURFACE_FAIL
    return 1;
}
void mac_engine_world_free(struct mac_engine_world *world)
{
    struct world_state *s;
    if(!world || !world->state)return;
    s=(struct world_state *)world->state;
    if(!s->bsp_only) {
        uint32_t other;
        for(other=0;other<32;++other)
            if(s->others[other] && s->others[other]!=s) {
                struct mac_engine_world held;
                held.state=s->others[other];s->others[other]=NULL;
                mac_engine_world_free(&held);
            }
        if(primary_world==s)primary_world=NULL;
    }
    free(s->collision.bsp3d.nodes.address);free(s->collision.bsp3d.planes.address);
    free(s->collision.leaves.address);free(s->collision.bsp2d_references.address);
    free(s->collision.bsp2d.nodes.address);free(s->collision.surfaces.address);
    free(s->collision.edges.address);free(s->collision.vertices.address);
    {
        long i;struct structure_collision_material *materials=s->structure.collision_materials.address;
        for(i=0;i<s->structure.collision_materials.count;++i)free(materials[i].shader.name);
        free(materials);
    }
    {
        long i,j;struct structure_lightmap *lightmaps=s->structure.lightmaps.address;
        for(i=0;i<s->structure.lightmaps.count;++i) {
            struct structure_material *materials=lightmaps[i].materials.address;
            for(j=0;j<lightmaps[i].materials.count;++j) {
                free(materials[j].shader.name);
                free(materials[j].compressed_vertex_data.address);
            }
            free(materials);
        }
        free(lightmaps);
    }
    {
        long i;struct scenario_decal_palette_entry *palette=s->scenario.decal_palette.address;
        for(i=0;i<s->scenario.decal_palette.count;++i)free(palette[i].reference.name);
        free(palette);
    }
    {
        long i;
        struct world_portal *portals=(struct world_portal *)s->structure.cluster_portals.address;
        for(i=0;i<s->structure.cluster_portals.count;++i)free(portals[i].vertices.address);
        for(i=0;i<s->structure.clusters.count;++i) {
            struct tag_block *surface_indices=(struct tag_block *)((unsigned char *)s->structure.clusters.address+i*104+68);
            struct tag_block *portal_indices=(struct tag_block *)((unsigned char *)s->structure.clusters.address+i*104+92);
            free(surface_indices->address);
            free(portal_indices->address);
        }
        free(portals);
    }
    free_sound_palette(&s->structure.background_sound_palette,sizeof(struct structure_background_sound_palette_entry));
    free_sound_palette(&s->structure.sound_environment_palette,sizeof(struct structure_sound_environment_palette_entry));
    free(s->structure.cluster_data.address);
    free(s->structure.sound_cluster_data.address);
    free(s->structure.surface_references.address);free(s->structure.surfaces.address);
    free(s->structure.leaves.address);free(s->structure.clusters.address);
    free(s->structure.runtime_decals.address);
    if(s->structure.fog_planes.address) {
        struct world_fog_plane *planes=(struct world_fog_plane *)s->structure.fog_planes.address;
        long plane;
        for(plane=0;plane<s->structure.fog_planes.count;++plane)free(planes[plane].vertices.address);
        free(planes);
    }
    free(s->structure.fog_regions.address);free(s->structure.fog_palette.address);
    free(s->structure.breakable_surfaces.address);
    free(s->structure.pathfinding_surfaces.address);
    free(s->structure.lightmap_group.name);
    mac_native_scenario_blocks_free(&s->scenario_objects);
    mac_native_scenario_gameplay_free(&s->scenario_gameplay);
    mac_native_scenario_scripts_free(&s->scenario_scripts);
    mac_native_scenario_skies_free(&s->scenario_skies);
    free(s);world->state=NULL;
}

struct structure_bsp *mac_engine_world_structure(struct mac_engine_world *world)
{
    struct world_state *s;
    if(!world || !world->state)return NULL;
    s=(struct world_state *)world->state;
    return &s->structure;
}
static void attach_scenario_object_blocks(struct world_state *s)
{
    struct scenario *scenario=&s->scenario;
    const struct mac_native_scenario_blocks *native=&s->scenario_objects;
#define ATTACH(field,family) do { scenario->field.count=native->families[family].placement_count; scenario->field.address=native->families[family].placements; } while(0)
#define ATTACH_PALETTE(field,family) do { scenario->field.count=native->families[family].palette_count; scenario->field.address=native->families[family].palette; } while(0)
    scenario->object_names.count=native->object_name_count;
    scenario->object_names.address=native->object_names;
    ATTACH(scenery,MAC_SCENARIO_SCENERY);ATTACH_PALETTE(scenery_palette,MAC_SCENARIO_SCENERY);
    ATTACH(bipeds,MAC_SCENARIO_BIPED);ATTACH_PALETTE(biped_palette,MAC_SCENARIO_BIPED);
    ATTACH(vehicles,MAC_SCENARIO_VEHICLE);ATTACH_PALETTE(vehicle_palette,MAC_SCENARIO_VEHICLE);
    ATTACH(equipment,MAC_SCENARIO_EQUIPMENT);ATTACH_PALETTE(equipment_palette,MAC_SCENARIO_EQUIPMENT);
    ATTACH(weapons,MAC_SCENARIO_WEAPON);ATTACH_PALETTE(weapon_palette,MAC_SCENARIO_WEAPON);
    ATTACH(machines,MAC_SCENARIO_MACHINE);ATTACH_PALETTE(machine_palette,MAC_SCENARIO_MACHINE);
    ATTACH(controls,MAC_SCENARIO_CONTROL);ATTACH_PALETTE(control_palette,MAC_SCENARIO_CONTROL);
    ATTACH(light_fixtures,MAC_SCENARIO_LIGHT_FIXTURE);ATTACH_PALETTE(light_fixtures_palette,MAC_SCENARIO_LIGHT_FIXTURE);
    ATTACH(sound_scenery,MAC_SCENARIO_SOUND_SCENERY);ATTACH_PALETTE(sound_scenery_palette,MAC_SCENARIO_SOUND_SCENERY);
#undef ATTACH
#undef ATTACH_PALETTE
}
static void attach_scenario_gameplay_blocks(struct world_state *s)
{
    s->scenario.netgame_flags.count=(long)s->scenario_gameplay.netgame_flag_count;
    s->scenario.netgame_flags.address=s->scenario_gameplay.netgame_flags;
    s->scenario.netgame_equipment.count=(long)s->scenario_gameplay.netgame_equipment_count;
    s->scenario.netgame_equipment.address=s->scenario_gameplay.netgame_equipment;
    s->scenario.scenario_starting_equipment.count=(long)s->scenario_gameplay.starting_equipment_count;
    s->scenario.scenario_starting_equipment.address=s->scenario_gameplay.starting_equipment;
    s->scenario.device_groups.count=(long)s->scenario_gameplay.device_group_count;
    s->scenario.device_groups.address=s->scenario_gameplay.device_groups;
#define ATTACH_CAMPAIGN(field,array,counter) do { s->scenario.field.count=(long)s->scenario_gameplay.counter; s->scenario.field.address=s->scenario_gameplay.array; } while(0)
    ATTACH_CAMPAIGN(trigger_volumes,trigger_volumes,trigger_volume_count);
    ATTACH_CAMPAIGN(cutscene_flags,cutscene_flags,cutscene_flag_count);
    ATTACH_CAMPAIGN(cutscene_camera_points,camera_points,camera_point_count);
    ATTACH_CAMPAIGN(cutscene_chapter_titles,chapter_titles,chapter_title_count);
    ATTACH_CAMPAIGN(starting_profiles,starting_profiles,starting_profile_count);
    ATTACH_CAMPAIGN(recorded_animations,recorded_animations,recorded_animation_count);
    ATTACH_CAMPAIGN(ai_encounters,ai_encounters,ai_encounter_count);
    ATTACH_CAMPAIGN(ai_command_lists,ai_command_lists,ai_command_list_count);
    ATTACH_CAMPAIGN(ai_conversations,ai_conversations,ai_conversation_count);
    ATTACH_CAMPAIGN(ai_actor_palette,ai_actor_palette,ai_actor_palette_count);
    ATTACH_CAMPAIGN(ai_animation_references,ai_animation_references,ai_animation_reference_count);
    ATTACH_CAMPAIGN(ai_script_references,ai_script_references,ai_script_reference_count);
    ATTACH_CAMPAIGN(bsp_switch_trigger_volumes,bsp_switch_volumes,bsp_switch_volume_count);
    ATTACH_CAMPAIGN(ai_recording_references,ai_recording_references,ai_recording_reference_count);
#undef ATTACH_CAMPAIGN
}
static int bind_collision(struct world_state *s,const struct mac_retail_collision *input)
{
    struct bsp3d_node *nodes;
    real_plane3d *planes;
    struct world_collision_leaf *leaves;
    struct world_collision_reference *references;
    struct bsp2d_node *nodes2;
    struct collision_surface *surfaces;
    struct collision_edge *edges;
    struct collision_vertex *vertices;
    uint32_t i,j;
#define ALLOC(name,block,type,slot) name=(type *)allocate_block(&(block),input->count[slot],sizeof(type));if(!name)return 0
    ALLOC(nodes,s->collision.bsp3d.nodes,struct bsp3d_node,0);
    ALLOC(planes,s->collision.bsp3d.planes,real_plane3d,1);
    ALLOC(leaves,s->collision.leaves,struct world_collision_leaf,2);
    ALLOC(references,s->collision.bsp2d_references,struct world_collision_reference,3);
    ALLOC(nodes2,s->collision.bsp2d.nodes,struct bsp2d_node,4);
    ALLOC(surfaces,s->collision.surfaces,struct collision_surface,5);
    ALLOC(edges,s->collision.edges,struct collision_edge,6);
    ALLOC(vertices,s->collision.vertices,struct collision_vertex,7);
#undef ALLOC
    for(i=0;i<input->count[0];++i) {
        nodes[i].plane_designator=signed_word(input->nodes[i].plane);
        for(j=0;j<2;++j)nodes[i].children[j]=signed_word(input->nodes[i].child[j]);
    }
    for(i=0;i<input->count[1];++i) {
        planes[i].n.i=input->planes[i].n[0];planes[i].n.j=input->planes[i].n[1];planes[i].n.k=input->planes[i].n[2];planes[i].d=input->planes[i].d;
    }
    for(i=0;i<input->count[2];++i) {
        leaves[i].flags=input->leaves[i].flags;leaves[i].count=input->leaves[i].count;leaves[i].first=signed_word(input->leaves[i].first);
    }
    for(i=0;i<input->count[3];++i) {
        references[i].plane=signed_word(input->references[i].plane);references[i].root=signed_word(input->references[i].root);
    }
    for(i=0;i<input->count[4];++i) {
        nodes2[i].plane.n.i=input->nodes2[i].plane[0];nodes2[i].plane.n.j=input->nodes2[i].plane[1];nodes2[i].plane.d=input->nodes2[i].plane[2];
        for(j=0;j<2;++j)nodes2[i].child_indices[j]=signed_word(input->nodes2[i].child[j]);
    }
    for(i=0;i<input->count[5];++i) {
        surfaces[i].plane_designator=signed_word(input->surfaces[i].plane);surfaces[i].first_edge_index=signed_word(input->surfaces[i].edge);
        surfaces[i].flags=input->surfaces[i].flags;surfaces[i].breakable_surface_index=input->surfaces[i].breakable;surfaces[i].material_index=input->surfaces[i].material;
    }
    for(i=0;i<input->count[6];++i)for(j=0;j<2;++j) {
        edges[i].vertex_indices[j]=signed_word(input->edges[i].vertex[j]);edges[i].edge_indices[j]=signed_word(input->edges[i].edge[j]);edges[i].surface_indices[j]=signed_word(input->edges[i].surface[j]);
    }
    for(i=0;i<input->count[7];++i) {
        vertices[i].point.x=input->vertices[i].p[0];vertices[i].point.y=input->vertices[i].p[1];vertices[i].point.z=input->vertices[i].p[2];vertices[i].first_edge_index=signed_word(input->vertices[i].edge);
    }
    s->structure.collision_bsp.count=1;s->structure.collision_bsp.address=&s->collision;
    return 1;
}
static int world_create(const struct mac_retail_cache *cache,uint32_t bsp_index,
    struct mac_engine_world *world,int only,const char **error)
{
    struct world_state *s;
    struct mac_retail_collision collision;
    struct mac_retail_spawn spawns[MAC_RETAIL_MAX_SPAWNS];
    const struct mac_retail_bsp *bsp;
    const unsigned char *root,*raw_leaves,*raw_clusters,*p,*raw_materials,*raw_runtime_decals,*raw_decal_palette,*raw_scenario;
    struct structure_leaf *leaves;
    struct structure_cluster *clusters;
    struct structure_collision_material *materials;
    struct structure_runtime_decal *runtime_decals;
    struct scenario_decal_palette_entry *decal_palette;
    uint32_t i,j,k,leaf_count,cluster_count,surface_refs,spawn_count,material_count;
    uint32_t runtime_decal_count,decal_palette_count,decal_ranges_total;
    uint32_t sound_cluster_size,sound_cluster_address,sound_cluster_required;
    const unsigned char *raw_sound_clusters;
    if(error)*error=NULL;
    if(!world)return 0;
    world->state=NULL;
    if(!cache || !cache->file || bsp_index>=cache->bsp_count || cache->bsp_count>32) { if(error)*error="invalid world cache";return 0; }
    bsp=&cache->bsps[bsp_index];
    if(bsp->file_offset>cache->file_size || bsp->size>cache->file_size-bsp->file_offset) { if(error)*error="world BSP range";return 0; }
    root=span(cache,bsp,bsp->root,0x288);
    if(!root) { if(error)*error="world BSP root";return 0; }
    leaf_count=u32(root+0xe0);cluster_count=u32(root+0x134);surface_refs=u32(root+0xec);
    if(!leaf_count || leaf_count>65536 || !cluster_count || cluster_count>512 || surface_refs>131072) { if(error)*error="world block count";return 0; }
    /* structure_bsp_get_cluster_encoded_sound_data indexes a flat triangular
       matrix of one byte per unordered cluster pair. The header is native
       integer metadata; the encoded bytes themselves are endian-neutral. */
    sound_cluster_size=u32(root+offsetof(struct structure_bsp,sound_cluster_data));
    sound_cluster_address=u32(root+offsetof(struct structure_bsp,sound_cluster_data)+12);
    sound_cluster_required=cluster_count*(cluster_count-1)/2;
    if(sound_cluster_size<sound_cluster_required || sound_cluster_size>bsp->size) {
        if(error)*error="world sound cluster data size";return 0;
    }
    raw_sound_clusters=sound_cluster_size?span(cache,bsp,sound_cluster_address,sound_cluster_size):NULL;
    if(sound_cluster_size && !raw_sound_clusters) { if(error)*error="world sound cluster data span";return 0; }
    raw_leaves=span(cache,bsp,u32(root+0xe4),leaf_count*16);
    raw_clusters=span(cache,bsp,u32(root+0x138),cluster_count*104);
    if(!raw_leaves || !raw_clusters) { if(error)*error="world leaf/cluster span";return 0; }
    /* Validate the actual references before publishing any native world. */
    for(i=0;i<leaf_count;++i) {
        int cluster,count;
        uint32_t first;
        p=raw_leaves+16*i;cluster=s16(p+8);count=s16(p+10);first=u32(p+12);
        if(cluster < -1 || (cluster>=0 && (unsigned)cluster>=cluster_count) || count<0 ||
            (count && (first>surface_refs || (unsigned)count>surface_refs-first))) {
            if(error)*error="world leaf cluster/surface reference";return 0;
        }
    }
    spawn_count=0;
    if((!only && !mac_retail_spawns(cache,spawns,&spawn_count,error)) || !mac_retail_collision_decode(cache,bsp_index,&collision,error))return 0;
    if(collision.count[2]!=leaf_count) { mac_retail_collision_free(&collision);if(error)*error="world structure/collision leaf mismatch";return 0; }
    material_count=u32(root+offsetof(struct structure_bsp,collision_materials));
    if(material_count>4096 || material_count>bsp->size/20) {
        mac_retail_collision_free(&collision);if(error)*error="world collision material count";return 0;
    }
    raw_materials=material_count?span(cache,bsp,
        u32(root+offsetof(struct structure_bsp,collision_materials)+4),material_count*20):NULL;
    if(material_count && !raw_materials) {
        mac_retail_collision_free(&collision);if(error)*error="world collision material span";return 0;
    }
    for(i=0;i<collision.count[5];++i) {
        int16_t index=collision.surfaces[i].material;
        if(index!=NONE && (index<0 || (uint32_t)index>=material_count)) {
            mac_retail_collision_free(&collision);if(error)*error="world collision surface material index";return 0;
        }
    }
    for(i=0;i<material_count;++i) {
        const unsigned char *m=raw_materials+i*20;
        int16_t type=s16(m+18);
        uint32_t handle=u32(m+12),name_length;
        const unsigned char *shader;
        struct mac_retail_tag tag;
        if(!cache_handle_valid(cache,u32(m),handle) ||
           (type!=NONE && (type<0 || type>=NUMBER_OF_MATERIAL_TYPES))) {
            mac_retail_collision_free(&collision);if(error)*error="world collision material reference/type";return 0;
        }
        if(handle!=UINT32_MAX) {
            if(!mac_retail_tag_get(cache,handle&UINT32_C(0xffff),&tag)) {
                mac_retail_collision_free(&collision);if(error)*error="world collision material tag handle";return 0;
            }
            if(tag.group!=UINT32_C(0x73686472) && tag.parents[0]!=UINT32_C(0x73686472) && tag.parents[1]!=UINT32_C(0x73686472)){
                mac_retail_collision_free(&collision);if(error)*error="world collision material shader group";return 0;
            }
            name_length=tag_name_length(tag.name);
            if(
               u32(m+4)!=u32(cache->tags+cache->instances_offset+(handle&UINT32_C(0xffff))*32+16) ||
               (u32(m+8)!=0 && u32(m+8)!=name_length)) {
                mac_retail_collision_free(&collision);if(error)*error="world collision material name reference";return 0;
            }
            shader=mac_retail_tag_span(cache,tag.address,36);
            if(!shader || s16(shader+34)!=type) {
                mac_retail_collision_free(&collision);if(error)*error="world collision material shader type";return 0;
            }
        } else if(u32(m+4)!=0 || u32(m+8)!=0) {
            mac_retail_collision_free(&collision);if(error)*error="world null collision material reference";return 0;
        }
    }
    runtime_decal_count=u32(root+offsetof(struct structure_bsp,runtime_decals));
    if(runtime_decal_count>65536 || runtime_decal_count>bsp->size/sizeof(struct structure_runtime_decal)) {
        mac_retail_collision_free(&collision);if(error)*error="world runtime decal count";return 0;
    }
    raw_runtime_decals=runtime_decal_count?span(cache,bsp,
        u32(root+offsetof(struct structure_bsp,runtime_decals)+4),
        runtime_decal_count*sizeof(struct structure_runtime_decal)):NULL;
    if(runtime_decal_count && !raw_runtime_decals) {
        mac_retail_collision_free(&collision);if(error)*error="world runtime decal span";return 0;
    }
    raw_scenario=cache->tags+cache->scenario_offset;
    decal_palette_count=u32(raw_scenario+offsetof(struct scenario,decal_palette));
    if(decal_palette_count>256 || decal_palette_count>cache->tag_size/sizeof(struct scenario_decal_palette_entry)) {
        mac_retail_collision_free(&collision);if(error)*error="world scenario decal palette count";return 0;
    }
    raw_decal_palette=decal_palette_count?mac_retail_tag_span(cache,
        u32(raw_scenario+offsetof(struct scenario,decal_palette)+4),
        decal_palette_count*sizeof(struct scenario_decal_palette_entry)):NULL;
    if(decal_palette_count && (!raw_decal_palette ||
       (u32(raw_scenario+offsetof(struct scenario,decal_palette)+4)&3))) {
        mac_retail_collision_free(&collision);if(error)*error="world scenario decal palette span";return 0;
    }
    for(i=0;i<decal_palette_count;++i) {
        const unsigned char *reference=raw_decal_palette+i*sizeof(struct scenario_decal_palette_entry);
        uint32_t group=u32(reference),handle=u32(reference+12),name_length;
        struct mac_retail_tag tag;
        if(handle==UINT32_MAX) {
            if(u32(reference+4)!=0 || u32(reference+8)!=0) {
                mac_retail_collision_free(&collision);if(error)*error="world null decal palette reference";return 0;
            }
            continue;
        }
        if(group!=DECAL_GROUP_TAG || !cache_handle_valid(cache,group,handle) ||
           !mac_retail_tag_get(cache,handle&UINT32_C(0xffff),&tag) ||
           tag.group!=DECAL_GROUP_TAG || !mac_retail_tag_span(cache,tag.address,1)) {
            mac_retail_collision_free(&collision);if(error)*error="world decal palette tag reference";return 0;
        }
        name_length=tag_name_length(tag.name);
        if(u32(reference+4)!=u32(cache->tags+cache->instances_offset+(handle&UINT32_C(0xffff))*32+16) ||
           (u32(reference+8)!=0 && u32(reference+8)!=name_length)) {
            mac_retail_collision_free(&collision);if(error)*error="world decal palette name reference";return 0;
        }
    }
    for(i=0;i<runtime_decal_count;++i) {
        const unsigned char *decal=raw_runtime_decals+i*sizeof(struct structure_runtime_decal);
        uint32_t bits;
        float value;
        for(j=0;j<3;++j) {
            bits=u32(decal+j*4);memcpy(&value,&bits,4);
            if(!finite_float(value)) {
                mac_retail_collision_free(&collision);if(error)*error="world runtime decal position";return 0;
            }
        }
        if(decal[12]>=decal_palette_count) {
            mac_retail_collision_free(&collision);if(error)*error="world runtime decal palette index";return 0;
        }
    }
    decal_ranges_total=0;
    for(i=0;i<cluster_count;++i) {
        const unsigned char *cluster=raw_clusters+i*sizeof(struct structure_cluster);
        int first=s16(cluster+0x0c);
        uint32_t count=u16(cluster+0x0e);
        if(count) {
            if(first<0 || (uint32_t)first>runtime_decal_count || count>runtime_decal_count-(uint32_t)first) {
                mac_retail_collision_free(&collision);if(error)*error="world cluster runtime decal range";return 0;
            }
            for(j=0;j<i;++j) {
                const unsigned char *previous=raw_clusters+j*sizeof(struct structure_cluster);
                int previous_first=s16(previous+0x0c);
                uint32_t previous_count=u16(previous+0x0e);
                if(previous_count && (uint32_t)first<(uint32_t)previous_first+previous_count &&
                   (uint32_t)previous_first<(uint32_t)first+count) {
                    mac_retail_collision_free(&collision);if(error)*error="overlapping cluster runtime decal ranges";return 0;
                }
            }
            if(decal_ranges_total>UINT32_MAX-count) {
                mac_retail_collision_free(&collision);if(error)*error="world cluster runtime decal total";return 0;
            }
            decal_ranges_total+=count;
        } else if(first!=NONE && (first<0 || (uint32_t)first>runtime_decal_count)) {
            mac_retail_collision_free(&collision);if(error)*error="world empty cluster runtime decal range";return 0;
        }
    }
    if(decal_ranges_total!=runtime_decal_count) {
        mac_retail_collision_free(&collision);if(error)*error="unowned runtime decal records";return 0;
    }
    s=(struct world_state *)calloc(1,sizeof(*s));
    if(!s) { mac_retail_collision_free(&collision);if(error)*error="world allocation failed";return 0; }
    world->state=s;s->bsp=bsp_index;s->scenario_handle=cache->scenario_handle;s->bsp_only=only;
    s->structure.sound_cluster_data.size=(long)sound_cluster_size;
    s->structure.sound_cluster_data.pad=u32(root+offsetof(struct structure_bsp,sound_cluster_data)+4);
    s->structure.sound_cluster_data.file_offset=signed_word(u32(root+offsetof(struct structure_bsp,sound_cluster_data)+8));
    if(sound_cluster_size) {
        s->structure.sound_cluster_data.address=malloc(sound_cluster_size);
        if(!s->structure.sound_cluster_data.address) {
            mac_retail_collision_free(&collision);mac_engine_world_free(world);
            if(error)*error="world sound cluster data allocation";return 0;
        }
        memcpy(s->structure.sound_cluster_data.address,raw_sound_clusters,sound_cluster_size);
    }
    /* The AI's walkable surface flags: one byte a collision surface (the
       block is read by path_structure_bsp.c and path_smoothing.c). A block
       that does not fit its BSP stays empty, as the engine treats a map
       without pathfinding data. */
    {
        uint32_t count=u32(root+offsetof(struct structure_bsp,pathfinding_surfaces));
        uint32_t address=u32(root+offsetof(struct structure_bsp,pathfinding_surfaces)+4);
        uint32_t offset=address-bsp->base;
        if(count && count<=bsp->size && address>=bsp->base && offset>=24 && offset<=bsp->size &&
           count<=bsp->size-offset) {
            s->structure.pathfinding_surfaces.address=malloc(count);
            if(s->structure.pathfinding_surfaces.address) {
                memcpy(s->structure.pathfinding_surfaces.address,cache->file+bsp->file_offset+offset,count);
                s->structure.pathfinding_surfaces.count=(long)count;
            }
        }
    }
    if(!only) {
    s->scenario.type=s16(raw_scenario+offsetof(struct scenario,type));
    s->scenario.flags=u16(raw_scenario+offsetof(struct scenario,flags));
    /* Object function inputs measure compass headings against local north. */
    if(!copy_reals(raw_scenario+offsetof(struct scenario,local_north),&s->scenario.local_north,1)) {
        mac_retail_collision_free(&collision);mac_engine_world_free(world);
        if(error)*error="world scenario local north";return 0;
    }
    {
        static const unsigned offsets[]={offsetof(struct scenario,ugly_structure_bsp),
            offsetof(struct scenario,unloved_globals),offsetof(struct scenario,bad_sky),
            offsetof(struct scenario,custom_object_names),offsetof(struct scenario,ingame_help_text),
            offsetof(struct scenario,hud_messages)};
        /* Blood Gulch has empty root references. Preserve NONE explicitly:
           calloc's index zero would name a real tag, not an absent reference. */
        for(i=0;i<sizeof(offsets)/sizeof(offsets[0]);++i) {
            const unsigned char *raw=raw_scenario+offsets[i];
            struct tag_reference *native=(struct tag_reference *)((unsigned char *)&s->scenario+offsets[i]);
            /* Campaign levels name their object names, help text and HUD
               messages (the last three offsets); the first three stay empty.
               The handle is kept; the session loads the HUD messages. */
            if(i>=3 && u32(raw+12)!=UINT32_MAX) {
                struct mac_retail_tag named;
                uint32_t handle=u32(raw+12);
                if(!mac_retail_tag_get(cache,handle&UINT32_C(0xffff),&named) || named.handle!=handle ||
                   named.group!=u32(raw)) {
                    mac_retail_collision_free(&collision);mac_engine_world_free(world);
                    if(error)*error="world scenario root reference does not name its tag";return 0;
                }
                native->group_tag=u32(raw);native->index=signed_word(handle);
                continue;
            }
            if(u32(raw+12)!=UINT32_MAX || u32(raw+4) || u32(raw+8)) {
                mac_retail_collision_free(&collision);mac_engine_world_free(world);
                if(error)*error="world nonempty scenario root reference unsupported";return 0;
            }
            native->group_tag=u32(raw);native->index=NONE;
        }
    }
    }
    /* These real bounds are consumed by visibility and aircraft physics. */
    for(i=0;i<8;++i) {
        real value;uint32_t bits=u32(root+(i<2?16+4*i:offsetof(struct structure_bsp,world_bounds)+4*(i-2)));
        memcpy(&value,&bits,4);
        if(!finite_float(value)) {mac_retail_collision_free(&collision);mac_engine_world_free(world);if(error)*error="world physical bounds nonfinite";return 0;}
        if(i==0)s->structure.vehicle_floor=value;
        else if(i==1)s->structure.vehicle_ceiling=value;
        else s->structure.world_bounds.n[i-2]=value;
    }
    /* port: the title map's bsp (scenario type main menu) has none (zero or inverted):
       a scene with no walls, so nothing is out of bounds */
    for(i=0;i<6;i+=2)if(s->scenario.type==2 && s->structure.world_bounds.n[i]>=s->structure.world_bounds.n[i+1]) {
        fprintf(stderr,"world: the title map's bounds are %g..%g on axis %u; widened\n",
            (double)s->structure.world_bounds.n[i],(double)s->structure.world_bounds.n[i+1],i/2);
        s->structure.world_bounds.n[i]=-100000.0f;s->structure.world_bounds.n[i+1]=100000.0f;
    }
    for(i=0;i<6;i+=2)if(s->structure.world_bounds.n[i]>s->structure.world_bounds.n[i+1]) {
        mac_retail_collision_free(&collision);mac_engine_world_free(world);if(error)*error="world inverted physical bounds";return 0;
    }
    if(!copy_lighting(root+offsetof(struct structure_bsp,default_lighting),&s->structure.default_lighting)) {
        mac_retail_collision_free(&collision);mac_engine_world_free(world);if(error)*error="world default lighting";return 0;
    }
    s->lightmap_handle=UINT32_MAX;
    if(!only) {
    if(!mac_native_scenario_blocks_decode(cache,&s->scenario_objects,error)) {
        mac_engine_world_free(world);if(error&&!*error)*error="world scenario object blocks";return 0;
    }
    attach_scenario_object_blocks(s);
    if(!mac_native_scenario_gameplay_decode(cache,&s->scenario_gameplay,error)) {
        mac_engine_world_free(world);if(error&&!*error)*error="world scenario gameplay blocks";return 0;
    }
    attach_scenario_gameplay_blocks(s);
    if(!mac_native_scenario_scripts_decode(cache,&s->scenario_scripts,error)) {
        mac_engine_world_free(world);if(error&&!*error)*error="world scenario scripts";return 0;
    }
    if(!mac_native_scenario_scripts_attach(&s->scenario_scripts,&s->scenario)) {
        mac_engine_world_free(world);if(error)*error="world scenario script attachment";return 0;
    }
    if(!mac_native_scenario_skies_decode(cache,&s->scenario_skies,error)) {
        mac_engine_world_free(world);if(error&&!*error)*error="world scenario sky references";return 0;
    }
    if(!mac_native_scenario_skies_attach(&s->scenario_skies,&s->scenario)) {
        mac_engine_world_free(world);if(error)*error="world scenario sky attachment";return 0;
    }
    }
    if(!bind_collision(s,&collision)) { mac_retail_collision_free(&collision);mac_engine_world_free(world);if(error)*error="world collision allocation";return 0; }
    mac_retail_collision_free(&collision);
    materials=(struct structure_collision_material *)allocate_block(
        &s->structure.collision_materials,material_count,sizeof(struct structure_collision_material));
    if(!materials) { mac_engine_world_free(world);if(error)*error="world collision material allocation";return 0; }
    for(i=0;i<material_count;++i) {
        const unsigned char *m=raw_materials+i*20;
        uint32_t handle=u32(m+12);
        struct mac_retail_tag tag;
        materials[i].shader.group_tag=u32(m);
        materials[i].shader.index=signed_word(handle);
        if(handle!=UINT32_MAX) {
            mac_retail_tag_get(cache,handle&UINT32_C(0xffff),&tag);
            materials[i].shader.name_length=(long)tag_name_length(tag.name);
            materials[i].shader.name=malloc((size_t)materials[i].shader.name_length+1);
            if(!materials[i].shader.name){mac_engine_world_free(world);if(error)*error="world shader name allocation";return 0;}
            memcpy(materials[i].shader.name,tag.name,(size_t)materials[i].shader.name_length+1);
        } else {
            materials[i].shader.name=NULL;
            materials[i].shader.name_length=0;
        }
        materials[i].pad=u16(m+16);
        materials[i].runtime_physics_material_type=s16(m+18);
    }
    runtime_decals=(struct structure_runtime_decal *)allocate_block(
        &s->structure.runtime_decals,runtime_decal_count,sizeof(*runtime_decals));
    decal_palette=(struct scenario_decal_palette_entry *)allocate_block(
        &s->scenario.decal_palette,only?0:decal_palette_count,sizeof(*decal_palette));
    if(!runtime_decals || !decal_palette) {
        mac_engine_world_free(world);if(error)*error="world decal allocation";return 0;
    }
    for(i=0;i<runtime_decal_count;++i) {
        const unsigned char *raw_decal=raw_runtime_decals+i*sizeof(*runtime_decals);
        uint32_t bits;
        bits=u32(raw_decal);memcpy(&runtime_decals[i].position.x,&bits,4);
        bits=u32(raw_decal+4);memcpy(&runtime_decals[i].position.y,&bits,4);
        bits=u32(raw_decal+8);memcpy(&runtime_decals[i].position.z,&bits,4);
        runtime_decals[i].palette_index=raw_decal[12];runtime_decals[i].unused=raw_decal[13];
        runtime_decals[i].yaw=(char)raw_decal[14];runtime_decals[i].pitch=(char)raw_decal[15];
    }
    for(i=0;i<(only?0:decal_palette_count);++i) {
        const unsigned char *raw_reference=raw_decal_palette+i*sizeof(*decal_palette);
        uint32_t handle=u32(raw_reference+12);
        decal_palette[i].reference.group_tag=u32(raw_reference);
        decal_palette[i].reference.index=signed_word(handle);
        if(handle!=UINT32_MAX) {
            struct mac_retail_tag tag;
            uint32_t name_length;
            mac_retail_tag_get(cache,handle&UINT32_C(0xffff),&tag);
            name_length=tag_name_length(tag.name);
            decal_palette[i].reference.name=(char *)malloc((size_t)name_length+1);
            if(!decal_palette[i].reference.name) {
                mac_engine_world_free(world);if(error)*error="world decal name allocation";return 0;
            }
            memcpy(decal_palette[i].reference.name,tag.name,(size_t)name_length+1);
            decal_palette[i].reference.name_length=(long)name_length;
        } else {
            decal_palette[i].reference.name=NULL;decal_palette[i].reference.name_length=0;
        }
    }
    leaves=(struct structure_leaf *)allocate_block(&s->structure.leaves,leaf_count,sizeof(*leaves));
    clusters=(struct structure_cluster *)allocate_block(&s->structure.clusters,cluster_count,sizeof(*clusters));
    if(!leaves || !clusters) { mac_engine_world_free(world);if(error)*error="world location allocation";return 0; }
    for(i=0;i<leaf_count;++i) {
        p=raw_leaves+16*i;
        for(j=0;j<6;++j)leaves[i].bounds.n[j]=p[j];
        leaves[i].pad=u16(p+6);leaves[i].cluster_index=s16(p+8);
        leaves[i].surface_reference_count=s16(p+10);leaves[i].first_surface_reference_index=signed_word(u32(p+12));
    }
    for(i=0;i<cluster_count;++i) {
        struct world_cluster_runtime_decals *decal_data;
        p=raw_clusters+104*i;
        clusters[i].sky_index=s16(p);clusters[i].fog_reference=s16(p+2);
        clusters[i].background_sound_palette_index=s16(p+4);clusters[i].sound_environment_palette_index=s16(p+6);clusters[i].weather_palette_index=s16(p+8);
        if(s16(p+2)!=NONE)++s->clusters_with_fog;
        if(s16(p+12)!=NONE || u16(p+14))++s->clusters_with_decals;
        decal_data=(struct world_cluster_runtime_decals *)((unsigned char *)clusters+i*sizeof(*clusters));
        decal_data->first_decal_index=s16(p+0x0c);decal_data->decal_count=u16(p+0x0e);
    }
    if(!bind_sound_palette(cache,bsp,root+offsetof(struct structure_bsp,background_sound_palette),
            &s->structure.background_sound_palette,sizeof(struct structure_background_sound_palette_entry),UINT32_C(0x6c736e64)) ||
       !bind_sound_palette(cache,bsp,root+offsetof(struct structure_bsp,sound_environment_palette),
            &s->structure.sound_environment_palette,sizeof(struct structure_sound_environment_palette_entry),UINT32_C(0x736e6465))) {
        mac_engine_world_free(world);if(error)*error="world sound palette range or reference";return 0;
    }
    for(i=0;i<cluster_count;++i) {
        short background=clusters[i].background_sound_palette_index;
        short environment=clusters[i].sound_environment_palette_index;
        if(background<NONE || background>=s->structure.background_sound_palette.count ||
           environment<NONE || environment>=s->structure.sound_environment_palette.count) {
            mac_engine_world_free(world);if(error)*error="world cluster sound palette index";return 0;
        }
    }
    s->fog_planes=u32(root+offsetof(struct structure_bsp,fog_planes));
    s->fog_regions=u32(root+offsetof(struct structure_bsp,fog_regions));
    s->fog_palette=u32(root+offsetof(struct structure_bsp,fog_palette));
    if(!bind_fog(cache,bsp,root,s)) {
        mac_engine_world_free(world);if(error)*error="world planar fog range, index or plane";return 0;
    }
    if(!bind_breakable_surfaces(cache,bsp,root,s)) {
        mac_engine_world_free(world);if(error)*error="world breakable surface range or index";return 0;
    }
    s->runtime_decal_count=runtime_decal_count;
    s->scenario_decal_palette_count=decal_palette_count;
    s->scenario.players.count=spawn_count;s->scenario.players.address=s->spawns;
    for(i=0;i<spawn_count;++i) {
        s->spawns[i].position.x=spawns[i].position[0];s->spawns[i].position.y=spawns[i].position[1];s->spawns[i].position.z=spawns[i].position[2];
        s->spawns[i].facing=spawns[i].facing;s->spawns[i].team_index=spawns[i].team;s->spawns[i].structure_bsp_reference_index=spawns[i].bsp;
        for(j=0;j<4;++j)s->spawns[i].game_types[j]=spawns[i].game_types[j];
    }
    s->scenario.structure_bsp_references.count=cache->bsp_count;s->scenario.structure_bsp_references.address=s->references;
    for(i=0;i<cache->bsp_count;++i) {
        s->references[i].file_offset=cache->bsps[i].file_offset;s->references[i].file_size=cache->bsps[i].size;
        s->references[i].structure_bsp.group_tag=STRUCTURE_BSP_TAG;s->references[i].structure_bsp.index=signed_word(cache->bsps[i].handle);
    }
    s->bsp_count=cache->bsp_count;s->active_bsp=bsp_index;
    return 1;
}
int mac_engine_world_create(const struct mac_retail_cache *cache,uint32_t bsp_index,
    struct mac_engine_world *world,const char **error)
{
    if(!world_create(cache,bsp_index,world,0,error))return 0;
    primary_world=(struct world_state *)world->state;
    primary_world->others[bsp_index]=primary_world;
    return 1;
}
/* port: the other BSPs of the scenario, converted as the session's own is
   (collision, clusters and portals, surfaces and materials, fog, decals,
   sound palettes, pathfinding flags) but without vertex buffers: those are
   made when the BSP is switched to (mac_engine_structure_bsp_load). */
int mac_engine_world_create_other_bsps(struct mac_engine_world *world,const struct mac_retail_cache *cache,const char **error)
{
    struct world_state *s;uint32_t i;
    if(!world || !world->state || !cache){if(error)*error="other BSPs arguments";return 0;}
    s=(struct world_state *)world->state;
    for(i=0;i<s->bsp_count && i<32;++i) {
        struct mac_engine_world other;
        if(i==s->bsp || s->others[i])continue;
        LOADING_PUMP();
        /* a BSP that does not convert is left out (a switch to it is refused: the
           game stays in the one it has), not the whole session */
        if(!world_create(cache,i,&other,1,error)) {
            fprintf(stderr,"native BSP %u not built: %s\n",(unsigned)i,error&&*error?*error:"conversion failed");
            if(error)*error=NULL;
            continue;
        }
        if(!mac_engine_world_enable_graph(&other,cache,error) ||
           !mac_engine_world_enable_surfaces(&other,cache,error)) {
            fprintf(stderr,"native BSP %u not built: %s\n",(unsigned)i,error&&*error?*error:"graph or surfaces");
            if(error)*error=NULL;
            mac_engine_world_free(&other);
            continue;
        }
        s->others[i]=(struct world_state *)other.state;
    }
    return 1;
}
uint32_t mac_engine_world_bsp_count(const struct mac_engine_world *world)
{
    const struct world_state *s;
    if(!world || !world->state)return 0;
    s=(const struct world_state *)world->state;return s->bsp_count;
}
int mac_engine_world_other_bsp(const struct mac_engine_world *world,uint32_t index,struct mac_engine_world *out)
{
    const struct world_state *s;
    if(!world || !world->state || !out)return 0;
    s=(const struct world_state *)world->state;
    if(index>=s->bsp_count || index>=32 || index==s->bsp || !s->others[index])return 0;
    out->state=s->others[index];return 1;
}
/* The structure of the BSP whose tag has this handle (tag_get's answer for a
   structure_bsp tag: the native tags do not hold the BSPs). */
struct structure_bsp *mac_engine_world_structure_for_handle(long handle)
{
    uint32_t i;
    if(!primary_world)return NULL;
    for(i=0;i<primary_world->bsp_count && i<32;++i)
        if(primary_world->others[i] && signed_word(primary_world->references[i].structure_bsp.index)==handle)
            return &primary_world->others[i]->structure;
    return NULL;
}
/* The structure of the BSP a scenario reference names, and the active one
   noted (the session makes and lets go of its vertex buffers). */
struct structure_bsp *mac_engine_world_structure_for_reference(const struct scenario_structure_bsp_reference *reference,int make_active)
{
    uint32_t i;
    if(!primary_world || !reference)return NULL;
    for(i=0;i<primary_world->bsp_count && i<32;++i)
        if(primary_world->others[i] && primary_world->references[i].structure_bsp.index==reference->structure_bsp.index) {
            if(make_active)primary_world->active_bsp=i;
            return &primary_world->others[i]->structure;
        }
    return NULL;
}
int mac_engine_world_info(const struct mac_engine_world *world,struct mac_engine_world_info *out)
{
    struct world_state *s;
    if(!world || !world->state || !out)return 0;
    s=(struct world_state *)world->state;
    out->leaves=s->structure.leaves.count;out->clusters=s->structure.clusters.count;
    out->spawns=s->scenario.players.count;out->collision_materials=s->structure.collision_materials.count;
    out->fog_planes=s->fog_planes;out->fog_regions=s->fog_regions;out->fog_palette=s->fog_palette;
    out->runtime_decals=s->runtime_decal_count;out->scenario_decal_palette=s->scenario_decal_palette_count;
    out->clusters_with_fog=s->clusters_with_fog;out->clusters_with_decals=s->clusters_with_decals;
    out->netgame_flags=s->scenario.netgame_flags.count;
    out->netgame_equipment=s->scenario.netgame_equipment.count;
    out->starting_equipment=s->scenario.scenario_starting_equipment.count;
    out->sky_references=s->scenario.sky_references.count;
    return 1;
}
int mac_engine_world_sound_cluster_data(const struct mac_engine_world *world,
    const unsigned char **data,uint32_t *size)
{
    const struct world_state *s;
    if(!world || !world->state || !data || !size)return 0;
    s=(const struct world_state *)world->state;
    *data=(const unsigned char *)s->structure.sound_cluster_data.address;
    *size=(uint32_t)s->structure.sound_cluster_data.size;
    return *size==0 || *data!=NULL;
}
int mac_engine_world_scenario_cache_contains(const struct mac_engine_world *world,
    const void *pointer,size_t size)
{
    const struct world_state *s;
    if(!world || !world->state || !pointer)return 0;
    s=(const struct world_state *)world->state;
    return mac_native_scenario_scripts_contains(&s->scenario_scripts,pointer,size);
}
int mac_engine_world_scenario_hud_messages(const struct mac_engine_world *world,uint32_t *handle)
{
    const struct world_state *s;
    if(!world || !world->state || !handle)return 0;
    s=(const struct world_state *)world->state;
    *handle=s->scenario.hud_messages.index==NONE?UINT32_MAX:(uint32_t)s->scenario.hud_messages.index;
    return 1;
}
uint32_t mac_engine_world_profile_weapons(const struct mac_engine_world *world,uint32_t *handles,uint32_t capacity)
{
    const struct world_state *s;
    if(!world || !world->state)return 0;
    s=(const struct world_state *)world->state;
    return mac_native_scenario_gameplay_profile_weapons(&s->scenario_gameplay,handles,capacity);
}
uint32_t mac_engine_world_ai_references(const struct mac_engine_world *world,uint32_t *groups,
    uint32_t *handles,uint32_t capacity,uint32_t *total)
{
    const struct world_state *s;
    if(total)*total=0;
    if(!world || !world->state)return 0;
    s=(const struct world_state *)world->state;
    return mac_native_scenario_gameplay_ai_references(&s->scenario_gameplay,groups,handles,capacity,total);
}
int mac_engine_world_script_references(const struct mac_engine_world *world,
    uint32_t *groups,uint32_t *handles,uint32_t capacity,uint32_t *count)
{
    const struct world_state *s;
    if(count)*count=0;
    if(!world || !world->state)return 0;
    s=(const struct world_state *)world->state;
    return mac_native_scenario_scripts_references(&s->scenario_scripts,groups,handles,capacity,count);
}
int mac_engine_world_sky_handle(const struct mac_engine_world *world,uint32_t index,uint32_t *handle)
{
    const struct world_state *s;const struct tag_reference *references;
    if(!world || !world->state || !handle)return 0;
    s=(const struct world_state *)world->state;
    if(index>=s->scenario_skies.count)return 0;
    references=(const struct tag_reference *)s->scenario_skies.references;
    memcpy(handle,&references[index].index,4);return 1;
}
int mac_engine_world_object_update_supported(const struct mac_engine_world *world)
{
    struct world_state *s;
    if(!world || !world->state)return 0;
    s=(struct world_state *)world->state;
    return s->graph_ready && s->render_surfaces_ready && s->fog_planes==0 && s->fog_regions==0 && s->fog_palette==0 && s->clusters_with_fog==0 &&
        s->structure.runtime_decals.count==s->runtime_decal_count &&
        s->scenario.decal_palette.count==s->scenario_decal_palette_count;
}
int mac_engine_world_sound_handles(const struct mac_engine_world *world,uint32_t group,
    uint32_t *handles,uint32_t capacity,uint32_t *count)
{
    const struct world_state *s;
    const struct tag_block *block;size_t stride;long i;
    if(count)*count=0;
    if(!world || !world->state || !count)return 0;
    s=world->state;
    if(group==UINT32_C(0x6c736e64)) {
        block=&s->structure.background_sound_palette;stride=sizeof(struct structure_background_sound_palette_entry);
    } else if(group==UINT32_C(0x736e6465)) {
        block=&s->structure.sound_environment_palette;stride=sizeof(struct structure_sound_environment_palette_entry);
    } else return 0;
    *count=(uint32_t)block->count;
    if(capacity<*count || (*count && !handles))return 0;
    for(i=0;i<block->count;++i) {
        const struct tag_reference *ref=(const struct tag_reference *)((const unsigned char *)block->address+i*stride+32);
        handles[i]=(uint32_t)ref->index;
    }
    return 1;
}
int mac_engine_world_decal_handles(const struct mac_engine_world *world,uint32_t *handles,
    uint32_t capacity,uint32_t *count)
{
    struct world_state *s;
    struct scenario_decal_palette_entry *palette;
    uint32_t i,total;
    if(!world || !world->state || !count)return 0;
    s=(struct world_state *)world->state;total=(uint32_t)s->scenario.decal_palette.count;
    *count=total;
    if(capacity<total || (total && !handles))return 0;
    palette=(struct scenario_decal_palette_entry *)s->scenario.decal_palette.address;
    for(i=0;i<total;++i)handles[i]=(uint32_t)palette[i].reference.index;
    return 1;
}
uint32_t mac_engine_world_lightmap_handle(const struct mac_engine_world *world)
{
    struct world_state *s;
    if(!world || !world->state)return UINT32_MAX;
    s=(struct world_state *)world->state;return s->lightmap_handle;
}
int mac_engine_world_material_shader_handles(const struct mac_engine_world *world,
    uint32_t *handles,uint32_t capacity,uint32_t *count)
{
    const struct world_state *s;const struct structure_lightmap *lightmaps;long i,j;uint32_t total=0;
    if(count)*count=0;if(!world || !world->state || !count)return 0;
    s=world->state;if(!s->render_surfaces_ready)return 0;lightmaps=s->structure.lightmaps.address;
    for(i=0;i<s->structure.lightmaps.count;++i) {
        if(lightmaps[i].materials.count<0 || (uint32_t)lightmaps[i].materials.count>65536-total)return 0;
        total+=(uint32_t)lightmaps[i].materials.count;
    }
    *count=total;if(total>capacity || (total&&!handles))return 0;total=0;
    for(i=0;i<s->structure.lightmaps.count;++i) {
        const struct structure_material *materials=lightmaps[i].materials.address;
        for(j=0;j<lightmaps[i].materials.count;++j)handles[total++]=(uint32_t)materials[j].shader.index;
    }
    return 1;
}
int mac_engine_world_object_palette_count(const struct mac_engine_world *world,
    uint32_t family,uint32_t *count)
{
    const struct world_state *s;
    if(count)*count=0;
    if(!world || !world->state || !count || family>=MAC_SCENARIO_FAMILY_COUNT)return 0;
    s=(const struct world_state *)world->state;
    if(s->scenario_objects.families[family].palette_count<0)return 0;
    *count=(uint32_t)s->scenario_objects.families[family].palette_count;
    return 1;
}
int mac_engine_world_object_palette_handle(const struct mac_engine_world *world,
    uint32_t family,uint32_t index,uint32_t *handle)
{
    const struct world_state *s;
    const struct tag_block *block=NULL;
    struct scenario_object_palette_entry *entry;
    if(!world || !world->state || !handle || family>=MAC_SCENARIO_FAMILY_COUNT)return 0;
    s=(const struct world_state *)world->state;
    switch(family) {
    case MAC_SCENARIO_SCENERY:block=&s->scenario.scenery_palette;break;
    case MAC_SCENARIO_BIPED:block=&s->scenario.biped_palette;break;
    case MAC_SCENARIO_VEHICLE:block=&s->scenario.vehicle_palette;break;
    case MAC_SCENARIO_EQUIPMENT:block=&s->scenario.equipment_palette;break;
    case MAC_SCENARIO_WEAPON:block=&s->scenario.weapon_palette;break;
    case MAC_SCENARIO_MACHINE:block=&s->scenario.machine_palette;break;
    case MAC_SCENARIO_CONTROL:block=&s->scenario.control_palette;break;
    case MAC_SCENARIO_LIGHT_FIXTURE:block=&s->scenario.light_fixtures_palette;break;
    case MAC_SCENARIO_SOUND_SCENERY:block=&s->scenario.sound_scenery_palette;break;
    default:return 0;
    }
    if(index>=(uint32_t)block->count)return 0;
    entry=TAG_BLOCK_GET_ELEMENT(block,index,struct scenario_object_palette_entry);
    if(!entry)return 0;
    *handle=(uint32_t)entry->reference.index;
    return 1;
}
int mac_engine_world_enable_surfaces(struct mac_engine_world *world,
    const struct mac_retail_cache *cache,const char **error)
{
    struct world_state *s;
    const struct mac_retail_bsp *bsp;
    const unsigned char *root;
    if(error)*error="world render surfaces";
    if(!world || !world->state || !cache || querying)return 0;
    s=(struct world_state *)world->state;
    if(s->render_surfaces_ready || s->bsp>=cache->bsp_count ||
       cache->scenario_handle!=s->scenario_handle)return 0;
    bsp=&cache->bsps[s->bsp];
    if(bsp->file_offset>cache->file_size || bsp->size>cache->file_size-bsp->file_offset)return 0;
    root=span(cache,bsp,bsp->root,0x288);
    if(!root || !bind_render_surface_data(s,cache,bsp,root,error))return 0;
    s->render_surfaces_ready=1;
    if(error)*error=NULL;
    return 1;
}
int mac_engine_world_location(const struct mac_engine_world *world,const float p[3],struct mac_engine_location *out)
{
    struct world_state *s;
    struct structure_bsp *old_structure;
    struct collision_bsp *old_collision;
    struct bsp3d *old_bsp;
    struct scenario *old_scenario;
    long old_scenario_index;
    short old_bsp_index;
    real_point3d point;
    struct location location;
    if(!world || !world->state || !out || !p || querying || !finite_float(p[0]) || !finite_float(p[1]) || !finite_float(p[2]))return 0;
    s=(struct world_state *)world->state;querying=1;
    old_structure=global_structure_bsp;old_collision=global_collision_bsp;old_bsp=global_bsp3d;old_scenario=global_scenario;
    old_scenario_index=global_scenario_index;old_bsp_index=global_structure_bsp_index;
    global_structure_bsp=&s->structure;global_collision_bsp=&s->collision;global_bsp3d=&s->collision.bsp3d;
    global_scenario=&s->scenario;global_scenario_index=signed_word(s->scenario_handle);global_structure_bsp_index=s->bsp;
    point.x=p[0];point.y=p[1];point.z=p[2];
    scenario_location_award_bonus(&location);
    scenario_location_from_point(&location,&point);
    out->leaf=location.leaf_index;out->cluster=location.cluster_index;out->bonus=location.bonus;
    global_structure_bsp=old_structure;global_collision_bsp=old_collision;global_bsp3d=old_bsp;global_scenario=old_scenario;
    global_scenario_index=old_scenario_index;global_structure_bsp_index=old_bsp_index;querying=0;
    return 1;
}
int mac_engine_world_spawn(const struct mac_engine_world *world,uint32_t index,float point[3],float *facing,struct mac_engine_location *out)
{
    struct world_state *s;
    struct player_starting_location *start;
    if(!world || !world->state || !point || !facing || !out)return 0;
    s=(struct world_state *)world->state;
    if(index>=(unsigned)s->scenario.players.count)return 0;
    start=&s->spawns[index];
    /* port: (a campaign level may start in another bsp than its start points') */
    if(s->scenario.type==0)(void)0;
    else if(start->structure_bsp_reference_index!=-1 && start->structure_bsp_reference_index!=(int)s->bsp)return 0;
    point[0]=start->position.x;point[1]=start->position.y;point[2]=start->position.z;*facing=start->facing;
    if(!mac_engine_world_location(world,point,out))return 0;
    if(out->cluster<0) {
        /* A start exactly on a floor (Hang 'Em High's) can test as solid:
           the game locates a unit by its centre, above its feet, so look a
           little higher. The start itself is unchanged. */
        static const float rise[]={0.05f,0.1f,0.2f,0.3f};
        unsigned k;
        for(k=0;k<sizeof(rise)/sizeof(rise[0])&&out->cluster<0;++k) {
            float raised[3];
            raised[0]=point[0];raised[1]=point[1];raised[2]=point[2]+rise[k];
            if(!mac_engine_world_location(world,raised,out))return 0;
        }
    }
    return 1;
}

/* Cluster graph fields are TU-private in structures.c. Their native offsets
   match the witnessed 104-byte cluster and 64-byte portal records. */
int mac_engine_world_enable_graph(struct mac_engine_world *world,const struct mac_retail_cache *cache,const char **error)
{
    struct world_state *s;const struct mac_retail_bsp *bsp;
    const unsigned char *root,*raw,*p,*v,*indices;
    struct world_portal *portals=NULL;struct tag_block *lists=NULL;
    uint32_t count=0,i,j,k,n,pvs_size,pvs_words,cluster_count=0;unsigned long *pvs=NULL;int ok=0,fail_line=0;
    if(error)*error="world cluster graph";
    if(!world || !world->state || !cache)return 0;
    s=(struct world_state *)world->state;
    if(s->graph_ready || querying || s->bsp>=cache->bsp_count || cache->scenario_handle!=s->scenario_handle)return 0;
    /* The game's own limit (Icefields has more than 64). The 64-entry
       result array of cluster_partition_reconnect is safe beyond it: the
       stock traversal writes only while room remains (its pointer runs on,
       unwritten). */
    if(s->structure.clusters.count<0 || s->structure.clusters.count>MAXIMUM_CLUSTERS_PER_STRUCTURE){fprintf(stderr,"native world cluster graph refused at line %d\n",__LINE__);return 0;}
    cluster_count=(uint32_t)s->structure.clusters.count;
    bsp=&cache->bsps[s->bsp];root=span(cache,bsp,bsp->root,0x160);if(!root){fprintf(stderr,"native world cluster graph refused at line %d\n",__LINE__);return 0;}
    pvs_words=((uint32_t)s->structure.clusters.count+31)/32;
    pvs_size=pvs_words*4*(uint32_t)s->structure.clusters.count;
    if(u32(root+0x140)!=pvs_size){fprintf(stderr,"native world cluster graph refused at line %d\n",__LINE__);return 0;}
    v=span(cache,bsp,u32(root+0x14c),pvs_size);if(!v)return 0;
    count=u32(root+0x154);if(count>1024){fprintf(stderr,"native world cluster graph refused at line %d\n",__LINE__);return 0;}
    raw=count?span(cache,bsp,u32(root+0x158),count*64):NULL;
    if(count && !raw){fprintf(stderr,"native world cluster graph refused at line %d\n",__LINE__);return 0;}
    portals=(struct world_portal *)calloc(count?count:1,sizeof(*portals));if(!portals){fprintf(stderr,"native world cluster graph refused at line %d\n",__LINE__);return 0;}
    pvs=(unsigned long *)malloc(pvs_size);if(!pvs){fail_line=__LINE__;goto done;}
    lists=(struct tag_block *)calloc(cluster_count?cluster_count:1,sizeof(*lists));if(!lists){fail_line=__LINE__;goto done;}
    for(i=0;i<pvs_size/4;++i)pvs[i]=u32(v+i*4);
    for(i=0;i<count;++i) {
        struct world_portal *q=portals+i;
        p=raw+i*64;q->clusters[0]=s16(p);q->clusters[1]=s16(p+2);q->plane=signed_word(u32(p+4));
        if(q->clusters[0]<0 || q->clusters[1]<0 || q->clusters[0]>=s->structure.clusters.count ||
           q->clusters[1]>=s->structure.clusters.count ||
           q->plane<0 || q->plane>=s->collision.bsp3d.planes.count){
            fprintf(stderr,"native world portal %lu: clusters %d,%d of %ld, plane %ld of %ld\n",(unsigned long)i,
                (int)q->clusters[0],(int)q->clusters[1],(long)s->structure.clusters.count,(long)q->plane,(long)s->collision.bsp3d.planes.count);
            fail_line=__LINE__;goto done;}
        /* (a portal from a cluster to itself, as Rat Race and Damnation have,
           is passed over by the traversals: the cluster is already marked) */
        for(j=0;j<4;++j) { uint32_t bits=u32(p+8+j*4);float f;memcpy(&f,&bits,4);
            if(!finite_float(f) || (j==3 && f<0)){fail_line=__LINE__;goto done;}
            if(j<3)q->center.n[j]=f;else q->radius=f;
        }
        q->flags=u32(p+24);n=u32(p+52);if(n<3 || n>128){fail_line=__LINE__;goto done;}
        v=span(cache,bsp,u32(p+56),n*12);if(!v){fail_line=__LINE__;goto done;}
        if(!allocate_block(&q->vertices,n,sizeof(real_point3d))){fail_line=__LINE__;goto done;}
        for(j=0;j<n;++j)for(k=0;k<3;++k) {
            uint32_t bits=u32(v+j*12+k*4);float f;memcpy(&f,&bits,4);if(!finite_float(f)){fail_line=__LINE__;goto done;}
            ((real_point3d *)q->vertices.address)[j].n[k]=f;
        }
    }
    raw=span(cache,bsp,u32(root+0x138),(uint32_t)s->structure.clusters.count*104);if(!raw){fail_line=__LINE__;goto done;}
    for(i=0;i<(uint32_t)s->structure.clusters.count;++i) {
        p=raw+i*104;n=u32(p+92);if(n>count){fail_line=__LINE__;goto done;}
        indices=n?span(cache,bsp,u32(p+96),n*2):NULL;if(n && !indices){fail_line=__LINE__;goto done;}
        if(n && !allocate_block(lists+i,n,sizeof(short))){fail_line=__LINE__;goto done;}
        for(j=0;j<n;++j) {
            short index=s16(indices+j*2);
            if(index<0 || (uint32_t)index>=count ||
               (portals[index].clusters[0]!=i && portals[index].clusters[1]!=i)){fail_line=__LINE__;goto done;}
            for(k=0;k<j;++k)if(((short *)lists[i].address)[k]==index){fail_line=__LINE__;goto done;}
            ((short *)lists[i].address)[j]=index;
        }
    }
    /* Every portal must occur in both endpoint lists. */
    for(i=0;i<count;++i)for(j=0;j<2;++j) {
        struct tag_block *list=lists+portals[i].clusters[j];int found=0;
        for(k=0;k<(uint32_t)list->count;++k)if(((short *)list->address)[k]==i)found=1;
        if(!found){fail_line=__LINE__;goto done;}
    }
    for(i=0;i<(uint32_t)s->structure.clusters.count;++i)
        memcpy((unsigned char *)s->structure.clusters.address+i*104+92,lists+i,sizeof(struct tag_block));
    s->structure.cluster_portals.count=count;s->structure.cluster_portals.address=portals;
    s->structure.collision_bsp.count=1;s->structure.collision_bsp.address=&s->collision;
    s->structure.cluster_data.size=pvs_size;s->structure.cluster_data.address=pvs;
    s->graph_ready=1;ok=1;if(error)*error=NULL;
done:
    if(!ok&&fail_line)fprintf(stderr,"native world cluster graph refused at line %d\n",fail_line);
    if(!ok) { free(pvs);for(i=0;i<count;++i)free(portals[i].vertices.address);free(portals);
        if(lists)for(i=0;i<cluster_count;++i)free(lists[i].address); }
    free(lists);
    return ok;
}
int mac_engine_world_scope(const struct mac_engine_world *world,int (*operation)(void *),void *context)
{
    struct world_state *s;struct structure_bsp *old_structure;struct collision_bsp *old_collision;
    struct bsp3d *old_bsp;struct scenario *old_scenario;long old_scenario_index;short old_bsp_index;int result;
    if(!world || !world->state || !operation || querying)return 0;
    s=(struct world_state *)world->state;if(!s->graph_ready)return 0;
    if(!mac_engine_script_cache_bind(&s->scenario_scripts))return 0;
    querying=1;
    old_structure=global_structure_bsp;old_collision=global_collision_bsp;old_bsp=global_bsp3d;old_scenario=global_scenario;
    old_scenario_index=global_scenario_index;old_bsp_index=global_structure_bsp_index;
    global_structure_bsp=&s->structure;global_collision_bsp=&s->collision;global_bsp3d=&s->collision.bsp3d;
    global_scenario=&s->scenario;global_scenario_index=signed_word(s->scenario_handle);global_structure_bsp_index=s->bsp;
    result=operation(context);
    if(!mac_engine_script_cache_unbind(&s->scenario_scripts))result=0;
    global_structure_bsp=old_structure;global_collision_bsp=old_collision;global_bsp3d=old_bsp;global_scenario=old_scenario;
    global_scenario_index=old_scenario_index;global_structure_bsp_index=old_bsp_index;querying=0;
    return result;
}
