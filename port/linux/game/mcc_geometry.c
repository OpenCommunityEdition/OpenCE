/* Geometry adapter for MCC caches. Wire model and BSP streams are read only;
 * Xbox render descriptors are constructed in MCC-owned allocations. */
#include "cseries.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "structures/structure_bsp_definitions.h"
#include "mcc_runtime.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct mcc_render_part {
    unsigned long flags;
    short shader;
    signed char previous, next;
    short centroid_nodes[2];
    float centroid_weights[2];
    real_point3d centroid;
    struct tag_block uncompressed, compressed, triangles;
    struct triangle_buffer indices;
    struct vertex_buffer vertices;
};

struct mcc_geometry_buffer {
    struct mcc_geometry_buffer *next;
    void *descriptor;
    void *payload;
    uint32_t bytes;
    void *allocation;
    uint32_t allocation_bytes;
    int is_index, is_bsp;
    unsigned char palette[22];
    unsigned char palette_count;
};

struct mcc_converted_model {
    struct mcc_converted_model *next;
    void *address;
};

struct mcc_geometry_state {
    struct mcc_geometry_buffer *buffers;
    struct mcc_converted_model *models;
};

typedef char mcc_render_part_size_check[sizeof(struct mcc_render_part)==0x68 ? 1 : -1];

static uint16_t mg_u16(void const *p) {
    unsigned char const *v=p;
    return (uint16_t)(v[0]|((uint16_t)v[1]<<8));
}
static uint32_t mg_u32(void const *p) {
    unsigned char const *v=p;
    return v[0]|((uint32_t)v[1]<<8)|((uint32_t)v[2]<<16)|((uint32_t)v[3]<<24);
}
static int mg_range(uint32_t offset,uint64_t length,uint32_t capacity) {
    return offset<=capacity && length<=capacity-offset;
}
static int mg_error(char const *text) {
    error(_error_silent,"MCC geometry: %s",text);
    return 0;
}

static void *mg_block(struct mcc_runtime *r, struct tag_block const *b, uint32_t stride) {
    if (b->count<0 || (uint32_t)b->count>UINT32_MAX/stride) return NULL;
    if (!b->count) return r->tags;
    return mcc_runtime_pointer(r,(uint32_t)(uintptr_t)b->address,(uint32_t)b->count*stride);
}

static struct mcc_geometry_buffer *mg_record(struct mcc_runtime *r,void *descriptor,
    void *payload,uint32_t bytes,void *allocation,uint32_t allocation_bytes,int index,int bsp) {
    struct mcc_geometry_state *state=r->geometry;
    struct mcc_geometry_buffer *b=malloc(sizeof(*b));
    if (!b) return NULL;
    memset(b,0,sizeof(*b));
    b->descriptor=descriptor; b->payload=payload; b->bytes=bytes;
    b->allocation=allocation; b->allocation_bytes=allocation_bytes;
    b->is_index=index; b->is_bsp=bsp;
    b->next=state->buffers; state->buffers=b;
    return b;
}

static struct mcc_geometry_buffer *mg_vertices(struct mcc_runtime *r,
    struct vertex_buffer *descriptor,short type,uint32_t count,void *payload,uint32_t bytes,
    void *allocation,uint32_t allocation_bytes,int bsp) {
    struct mcc_geometry_buffer *b=mg_record(r,descriptor,payload,bytes,allocation,allocation_bytes,0,bsp);
    if (!b) { if (allocation) free(allocation); return NULL; }
    if (!rasterizer_vertex_buffer_new(descriptor,type,(long)count,payload,(long)bytes)) return NULL;
    return b;
}

static int mg_indices(struct mcc_runtime *r,struct triangle_buffer *descriptor,
    uint32_t triangles,void *payload,uint32_t bytes) {
    if (!mg_record(r,descriptor,payload,bytes,payload,bytes,1,0)) { free(payload); return 0; }
    return rasterizer_triangle_buffer_new(descriptor,_triangle_buffer_type_precompiled_strip,
        (long)triangles,payload);
}

/* Check every floating-point attribute before calling the engine compressor;
 * it assumes finite normals and asserts on components outside [-1.005,1.005].
 * The compiler builtin is available even with the Linux port's C89 headers. */
static int mg_float_vertices(void const *data,uint32_t count,uint32_t stride,
    uint32_t float_count,uint32_t vector_start,uint32_t vector_components) {
    uint32_t i,j;
    for (i=0;i<count;i++) {
        float const *v=(float const *)((unsigned char const *)data+i*stride);
        for (j=0;j<float_count;j++) {
            if (!__builtin_isfinite(v[j])) return 0;
            if (j>=vector_start && j<vector_start+vector_components && fabsf(v[j])>1.005f) return 0;
        }
    }
    return 1;
}

static int mg_palette_index(unsigned char *palette,unsigned char *count,short node) {
    unsigned char i;
    for (i=0;i<*count;i++) if (palette[i]==node) return i;
    if (*count==22) return -1;
    palette[*count]=(unsigned char)node;
    return (*count)++;
}

static int mg_part(struct mcc_runtime *r,struct model const *model,unsigned char const *wire,
    uint32_t part_index,uint32_t part_count,struct mcc_render_part *part) {
    uint32_t count=mg_u32(wire+0x58), triangles=mg_u32(wire+0x48);
    uint32_t vertex_offset=mg_u32(wire+0x64), index_offset=mg_u32(wire+0x4C);
    uint32_t i,raw_bytes,vertex_bytes,index_bytes;
    struct model_vertex_uncompressed *vertices=NULL;
    struct model_vertex_compressed *compressed=NULL;
    unsigned short *indices=NULL;
    unsigned char palette[22],palette_count=0;
    int local=(model->flags&2)!=0, many=model->nodes.count>=RASTERIZER_MAXIMUM_NODES_PER_MODEL;
    struct mcc_geometry_buffer *record;
    (void)part_index;
    memset(part,0,sizeof(*part));
    if (!count || count>65535 || !triangles || triangles>65535 || mg_u16(wire+0x44)!=1 ||
        mg_u16(wire+0x54)!=4 || model->nodes.count<1 || model->nodes.count>64 ||
        (mg_u16(wire+4)!=0xFFFFu && mg_u16(wire+4)>=(uint32_t)model->shaders.count) ||
        ((signed char)wire[6]!=-1 && (unsigned char)wire[6]>=part_count) ||
        ((signed char)wire[7]!=-1 && (unsigned char)wire[7]>=part_count))
        return mg_error("invalid model part descriptor");
    raw_bytes=count*68; vertex_bytes=count*32; index_bytes=(triangles+2)*2;
    if (!mg_range(vertex_offset,raw_bytes,r->report.model_vertex_bytes) ||
        !mg_range(index_offset,index_bytes,r->report.model_bytes-r->report.model_vertex_bytes))
        return mg_error("model stream exceeds its declared range");
    if (local) {
        palette_count=wire[0x6B];
        if (!palette_count || palette_count>22) return mg_error("invalid model node palette size");
        memcpy(palette,wire+0x6C,palette_count);
        for (i=0;i<palette_count;i++) if (palette[i]>=model->nodes.count)
            return mg_error("model node palette references a missing node");
    }
    vertices=malloc(raw_bytes); compressed=malloc(vertex_bytes); indices=malloc(index_bytes);
    if (!vertices || !compressed || !indices) goto fail;
    if (!mcc_runtime_read(r,r->report.model_file_offset+vertex_offset,raw_bytes,vertices) ||
        !mcc_runtime_read(r,r->report.model_file_offset+r->report.model_vertex_bytes+index_offset,index_bytes,indices)) goto fail;
    if (!mg_float_vertices(vertices,count,68,14,3,9)) goto fail;
    for (i=0;i<count;i++) {
        int j;
        if (!__builtin_isfinite(vertices[i].node_weights[0]) || !__builtin_isfinite(vertices[i].node_weights[1])) goto fail;
        for (j=0;j<2;j++) {
            short node=vertices[i].nodes[j];
            if (node<0 || node>=(local ? palette_count : model->nodes.count)) goto fail;
            if (local && !many) vertices[i].nodes[j]=palette[node];
            else if (many && !local) {
                int remap=mg_palette_index(palette,&palette_count,node);
                if (remap<0) goto fail;
                vertices[i].nodes[j]=(short)remap;
            }
        }
    }
    for (i=0;i<triangles+2;i++) if (indices[i]>=count) goto fail;
    /* Only the common 32-byte material/centroid prefix is shared by the two
     * formats. Blocks and hardware descriptors are new runtime objects. */
    memcpy(part,wire,0x20);
    part->flags&=~2u;
    if (local) {
        for (i=0;i<2;i++) {
            short node=part->centroid_nodes[i];
            if (node!=-1 && (node<0 || node>=palette_count)) goto fail;
            if (node!=-1) part->centroid_nodes[i]=palette[node];
        }
    }
    rasterizer_geometry_compress_vertices(_rasterizer_vertex_type_model_uncompressed,
        (long)count,compressed,(long)vertex_bytes,vertices,(long)raw_bytes);
    free(vertices); vertices=NULL;
    record=mg_vertices(r,&part->vertices,_rasterizer_vertex_type_model_compressed,count,
        compressed,vertex_bytes,compressed,vertex_bytes,0);
    compressed=NULL;
    if (!record) goto fail;
    if (many) { memcpy(record->palette,palette,palette_count); record->palette_count=palette_count; }
    if (!mg_indices(r,&part->indices,triangles,indices,index_bytes)) { indices=NULL; goto fail; }
    return 1;
fail:
    if (vertices) free(vertices);
    if (compressed) free(compressed);
    if (indices) free(indices);
    return mg_error("model vertices, node indices or hardware buffers could not be converted");
}

int mcc_geometry_prepare(struct mcc_runtime *r) {
    uint32_t i;
    struct mcc_geometry_state *state;
    if (!r || r->geometry) return 0;
    state=malloc(sizeof(*state));
    if (!state) return 0;
    memset(state,0,sizeof(*state));
    r->geometry=state;
    for (i=0;i<r->report.tag_count;i++) {
        unsigned char *entry=r->tag_index+i*0x20;
        struct model *model;
        struct mcc_converted_model *known;
        unsigned char *geometries,*new_geometries;
        long g;
        if (mg_u32(entry)!='mod2') continue;
        model=mcc_runtime_pointer(r,mg_u32(entry+0x14),sizeof(*model));
        if (!model) return mg_error("model header lies outside loaded tags");
        for (known=state->models;known;known=known->next) if (known->address==model) break;
        if (known) { *(uint32_t *)entry='mode'; continue; }
        geometries=mg_block(r,&model->geometries,0x30);
        if (!geometries || model->geometries.count>256) return mg_error("invalid model geometry block");
        new_geometries=mcc_runtime_allocate(r,(uint32_t)model->geometries.count*0x30);
        if (model->geometries.count && !new_geometries) return mg_error("no room for model geometry descriptors");
        if (model->geometries.count) memcpy(new_geometries,geometries,(uint32_t)model->geometries.count*0x30);
        for (g=0;g<model->geometries.count;g++) {
            struct tag_block *block=(struct tag_block *)(geometries+g*0x30+0x24);
            struct tag_block *destination=(struct tag_block *)(new_geometries+g*0x30+0x24);
            unsigned char *parts=mg_block(r,block,0x84);
            struct mcc_render_part *new_parts;
            long p;
            if (!parts || block->count>128) return mg_error("invalid model parts block");
            new_parts=mcc_runtime_allocate(r,(uint32_t)block->count*sizeof(*new_parts));
            if (block->count && !new_parts) return mg_error("no room for model part descriptors");
            for (p=0;p<block->count;p++) if (!mg_part(r,model,parts+p*0x84,(uint32_t)p,(uint32_t)block->count,&new_parts[p])) return 0;
            destination->address=new_parts;
            destination->definition=NULL;
        }
        model->geometries.address=new_geometries;
        model->geometries.definition=NULL;
        model->flags&=~2u;
        known=malloc(sizeof(*known));
        if (!known) return 0;
        known->address=model; known->next=state->models; state->models=known;
        *(uint32_t *)entry='mode';
    }
    return 1;
}

static void *mg_bsp_pointer(void *base,uint32_t size,uint32_t address,uint32_t bytes) {
    uintptr_t start=(uintptr_t)base;
    if (address<start || !mg_range((uint32_t)(address-start),bytes,size)) return NULL;
    return (unsigned char *)base+(address-start);
}

static int mg_bsp_material(struct mcc_runtime *r,struct mcc_cache_bsp const *b,
    struct structure_material *m) {
    uint32_t count=(uint32_t)m->vertices.count, lit=(uint32_t)m->lightmap_vertices.count;
    uint32_t vo=(uint32_t)m->vertices.offset, lo=(uint32_t)m->lightmap_vertices.offset;
    uint32_t raw_bytes,compressed_bytes;
    unsigned char *raw,*compressed;
    if (count>65535 || lit>65535 || (lit && lit!=count) ||
        !mg_range(vo,(uint64_t)count*56,b->vertex_bytes) ||
        !mg_range(lo,(uint64_t)lit*20,b->vertex_bytes)) return mg_error("invalid BSP material vertex range");
    memset(&m->vertices,0,sizeof(m->vertices));
    memset(&m->lightmap_vertices,0,sizeof(m->lightmap_vertices));
    m->vertices.type=_rasterizer_vertex_type_environment_compressed;
    m->lightmap_vertices.type=_rasterizer_vertex_type_environment_lightmap_compressed;
    memset(&m->uncompressed_vertex_data,0,sizeof(m->uncompressed_vertex_data));
    memset(&m->compressed_vertex_data,0,sizeof(m->compressed_vertex_data));
    if (!count) return 1;
    raw_bytes=count*56+lit*20;
    compressed_bytes=count*32+lit*8;
    raw=malloc(raw_bytes); compressed=malloc(compressed_bytes);
    if (!raw || !compressed) {
        if (raw) free(raw);
        if (compressed) free(compressed);
        return 0;
    }
    if (!mcc_runtime_read(r,b->vertex_file_offset+vo,count*56,raw) ||
        (lit && !mcc_runtime_read(r,b->vertex_file_offset+lo,lit*20,raw+count*56)) ||
        !mg_float_vertices(raw,count,56,14,3,9) ||
        !mg_float_vertices(raw+count*56,lit,20,5,0,3)) {
        free(raw); free(compressed); return mg_error("invalid BSP vertex payload");
    }
    rasterizer_geometry_compress_vertices(_rasterizer_vertex_type_environment_uncompressed,
        (long)count,compressed,(long)(count*32),raw,(long)(count*56));
    if (lit) rasterizer_geometry_compress_vertices(_rasterizer_vertex_type_environment_lightmap_uncompressed,
        (long)lit,compressed+count*32,(long)(lit*8),raw+count*56,(long)(lit*20));
    free(raw);
    /* CPU BSP queries expect the lightmap vertices immediately after the
     * environment vertices, so both streams have one lifetime/allocation. */
    if (!mg_vertices(r,&m->vertices,_rasterizer_vertex_type_environment_compressed,count,
        compressed,count*32,compressed,compressed_bytes,1)) return 0;
    if (lit && !mg_vertices(r,&m->lightmap_vertices,_rasterizer_vertex_type_environment_lightmap_compressed,
        lit,compressed+count*32,lit*8,NULL,0,1)) return 0;
    m->compressed_vertex_data.size=(long)compressed_bytes;
    m->compressed_vertex_data.address=compressed;
    return 1;
}

int mcc_geometry_bsp_load(struct mcc_runtime *r,uint32_t handle,uint32_t offset,
    uint32_t size,void *address,void **result) {
    struct mcc_cache_bsp const *b=NULL;
    struct structure_bsp *structure;
    struct structure_lightmap *lightmaps;
    uint32_t i;
    long l;
    if (!r || !r->geometry || !result) return 0;
    *result=NULL;
    mcc_geometry_bsp_unload(r);
    for (i=0;i<r->report.bsp_count;i++) {
        struct mcc_cache_bsp const *candidate=&r->report.bsps[i];
        if (candidate->tag_handle==handle && candidate->file_offset==offset && candidate->size==size &&
            candidate->address==(uint32_t)(uintptr_t)address) { b=candidate; break; }
    }
    if (!b || !mcc_runtime_read(r,offset,size,address)) return mg_error("BSP request differs from validated reference");
    structure=mg_bsp_pointer(address,size,b->structure_address,sizeof(*structure));
    if (!structure || structure->lightmaps.count<0 || (uint32_t)structure->lightmaps.count>size/0x20)
        return mg_error("invalid BSP lightmaps");
    lightmaps=mg_bsp_pointer(address,size,(uint32_t)(uintptr_t)structure->lightmaps.address,
        (uint32_t)structure->lightmaps.count*0x20);
    if (structure->lightmaps.count && !lightmaps) return mg_error("BSP lightmaps point outside BSP");
    for (l=0;l<structure->lightmaps.count;l++) {
        struct tag_block const *block=&lightmaps[l].materials;
        struct structure_material *materials;
        long m;
        if (block->count<0 || (uint32_t)block->count>size/sizeof(*materials)) goto fail;
        materials=mg_bsp_pointer(address,size,(uint32_t)(uintptr_t)block->address,
            (uint32_t)block->count*sizeof(*materials));
        if (block->count && !materials) goto fail;
        for (m=0;m<block->count;m++) if (!mg_bsp_material(r,b,&materials[m])) goto fail;
    }
    /* The MCC file stream descriptors are not Xbox vertex-buffer tables. */
    memset((unsigned char *)address+4,0,16);
    *result=structure;
    return 1;
fail:
    mcc_geometry_bsp_unload(r);
    return mg_error("BSP material conversion failed");
}

static void mg_release(struct mcc_geometry_buffer *b) {
    if (b->is_index) rasterizer_triangle_buffer_delete((struct triangle_buffer *)b->descriptor);
    else rasterizer_vertex_buffer_delete((struct vertex_buffer *)b->descriptor);
    if (b->allocation) free(b->allocation);
    free(b);
}

void mcc_geometry_bsp_unload(struct mcc_runtime *r) {
    struct mcc_geometry_state *state=r ? r->geometry : NULL;
    struct mcc_geometry_buffer **link;
    if (!state) return;
    link=&state->buffers;
    while (*link) {
        struct mcc_geometry_buffer *b=*link;
        if (b->is_bsp) { *link=b->next; mg_release(b); }
        else link=&b->next;
    }
}

void mcc_geometry_dispose(struct mcc_runtime *r) {
    struct mcc_geometry_state *state=r ? r->geometry : NULL;
    if (!state) return;
    while (state->buffers) {
        struct mcc_geometry_buffer *b=state->buffers;
        state->buffers=b->next; mg_release(b);
    }
    while (state->models) {
        struct mcc_converted_model *m=state->models;
        state->models=m->next; free(m);
    }
    free(state); r->geometry=NULL;
}

void *mcc_geometry_buffer_data(struct mcc_runtime *r,void const *hardware,
    int index_buffer,unsigned long *bytes) {
    struct mcc_geometry_state *state=r ? r->geometry : NULL;
    struct mcc_geometry_buffer *b;
    if (bytes) *bytes=0;
    if (!state || !hardware) return NULL;
    for (b=state->buffers;b;b=b->next) {
        void *current=b->is_index ? ((struct triangle_buffer *)b->descriptor)->hardware_format :
            ((struct vertex_buffer *)b->descriptor)->hardware_format;
        if (b->is_index==index_buffer && current==hardware) {
            if (bytes) *bytes=b->bytes;
            return b->payload;
        }
    }
    return NULL;
}

int mcc_geometry_contains(struct mcc_runtime *r,void const *address,unsigned long bytes) {
    struct mcc_geometry_state *state=r ? r->geometry : NULL;
    struct mcc_geometry_buffer *b;
    uintptr_t wanted=(uintptr_t)address;
    if (!state) return 0;
    for (b=state->buffers;b;b=b->next) {
        uintptr_t start=(uintptr_t)b->allocation;
        if (b->allocation && wanted>=start && wanted-start<=b->allocation_bytes &&
            bytes<=b->allocation_bytes-(wanted-start)) return 1;
    }
    return 0;
}

short mcc_geometry_part_palette(struct mcc_runtime *r,void const *vertex_buffer,
    unsigned char const **nodes) {
    struct mcc_geometry_state *state=r ? r->geometry : NULL;
    struct mcc_geometry_buffer *b;
    if (nodes) *nodes=NULL;
    if (!state || !nodes) return 0;
    for (b=state->buffers;b;b=b->next) if (!b->is_index && b->descriptor==vertex_buffer && b->palette_count) {
        *nodes=b->palette; return b->palette_count;
    }
    return 0;
}
