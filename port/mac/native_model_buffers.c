#include <mach/mach.h>
#include "cseries.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "native_model_buffers.h"
#include "native_resource_memory.h"
#include "tag_files/tag_groups.h"
#include "cache/cache_files.h"
#include "models/model_definitions.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>
#define MAC_RESOURCE_BASE 0x80000000UL
#define MAC_RESOURCE_END (PLATFORM_CONTIGUOUS_BASE+PLATFORM_CONTIGUOUS_SIZE)
struct native_vertex_resource { D3DVertexBuffer resource;vm_address_t address;vm_size_t size; };
/* The models' index copies, by address. They are written once, in create,
   and only read after, so the renderer may keep each in a GL element buffer
   (HALO_MAC_INDEX_VBO) instead of handing the driver client indices on
   every draw. A copy's serial is never reused. */
struct index_span {unsigned long address,bytes,serial;};
static struct index_span *index_spans;
static unsigned long index_span_count,index_span_capacity,index_span_serial;
static int index_span_add(const void *p,unsigned long bytes)
{
    unsigned long address=(unsigned long)p,at;
    if(index_span_count==index_span_capacity) {
        unsigned long capacity=index_span_capacity?index_span_capacity*2:256;
        struct index_span *grown=(struct index_span *)realloc(index_spans,capacity*sizeof(*grown));
        if(!grown)return 0;
        index_spans=grown;index_span_capacity=capacity;
    }
    for(at=index_span_count;at&&index_spans[at-1].address>address;--at)index_spans[at]=index_spans[at-1];
    index_spans[at].address=address;index_spans[at].bytes=bytes;index_spans[at].serial=++index_span_serial;
    ++index_span_count;return 1;
}
static void index_span_remove(const void *p)
{
    unsigned long address=(unsigned long)p,i;
    for(i=0;i<index_span_count;i++)if(index_spans[i].address==address) {
        memmove(&index_spans[i],&index_spans[i+1],(index_span_count-i-1)*sizeof(*index_spans));
        --index_span_count;return;
    }
}
int mac_native_model_index_span(const void *p,unsigned long bytes,unsigned long *serial,
    unsigned long *offset,const void **base,unsigned long *total)
{
    unsigned long address=(unsigned long)p,low=0,high=index_span_count;
    const struct index_span *span;
    if(!p||!bytes||!serial||!offset||!base||!total)return 0;
    while(low<high){unsigned long middle=low+(high-low)/2;if(index_spans[middle].address<=address)low=middle+1;else high=middle;}
    if(!low)return 0;
    span=&index_spans[low-1];
    if(address-span->address>span->bytes||bytes>span->bytes-(address-span->address))return 0;
    *serial=span->serial;*offset=address-span->address;*base=(const void *)span->address;*total=span->bytes;
    return 1;
}
static void destroy(void *context,struct vertex_buffer *v,struct triangle_buffer *t)
{
    struct native_vertex_resource *r=(struct native_vertex_resource *)v->hardware_format;
    D3DIndexBuffer *index=(D3DIndexBuffer *)t->hardware_format;
    (void)context;
    if(r) { mac_resource_certificate_revoke(&r->resource);
        mac_resource_unregister_owned_range((void *)r->address);vm_deallocate(mach_task_self(),r->address,r->size);free(r); }
    if(index) { index_span_remove((void *)index->Data);mac_resource_unregister_owned_range((void *)index->Data);free((void *)index->Data);free(index); }
    memset(v,0,sizeof(*v));memset(t,0,sizeof(*t));
}
static int create(void *context,struct vertex_buffer *v,struct triangle_buffer *t,
    const struct model_vertex_uncompressed *vertices,uint32_t count,
    const uint16_t *indices,uint32_t triangle_count)
{
    struct native_vertex_resource *r;
    D3DIndexBuffer *index;
    vm_address_t address;
    vm_size_t size;
    uint16_t *copy;
    size_t vertex_bytes,index_bytes;
    kern_return_t result=KERN_NO_SPACE;
    (void)context;
    if(!v || !t || !vertices || !indices || !count || count>65535 || triangle_count>65535)return 0;
    vertex_bytes=count*sizeof(struct model_vertex_compressed);
    index_bytes=(triangle_count+2)*sizeof(uint16_t);
    size=(vm_size_t)((vertex_bytes+vm_page_size-1)&~(vm_page_size-1));
    r=(struct native_vertex_resource *)calloc(1,sizeof(*r));
    index=(D3DIndexBuffer *)calloc(1,sizeof(*index));copy=(uint16_t *)malloc(index_bytes);
    if(!r || !index || !copy) { free(r);free(index);free(copy);return 0; }
    /* FALSE requires the requested range to be free; it never replaces a map. */
    {
        unsigned long found=0;
        result=mac_resource_vm_allocate_next(MAC_RESOURCE_BASE+vm_page_size,MAC_RESOURCE_END-size,
            vm_page_size,size,&found)?KERN_SUCCESS:KERN_NO_SPACE;
        address=(vm_address_t)found;
    }
    if(result!=KERN_SUCCESS) { free(r);free(index);free(copy);return 0; }
    r->address=address;r->size=size;
    rasterizer_geometry_compress_vertices(_rasterizer_vertex_type_model_uncompressed,count,
        (void *)address,(long)vertex_bytes,(void *)vertices,count*sizeof(*vertices));
    memcpy(copy,indices,index_bytes);
    if(!mac_resource_register_owned_range((void *)address,vertex_bytes)) {
        vm_deallocate(mach_task_self(),address,size);free(r);free(index);free(copy);return 0;
    }
    if(!mac_resource_register_owned_range(copy,index_bytes)) {
        mac_resource_unregister_owned_range((void *)address);
        vm_deallocate(mach_task_self(),address,size);free(r);free(index);free(copy);return 0;
    }
    r->resource.Common=D3DCOMMON_TYPE_VERTEXBUFFER|1;
    r->resource.Data=address&~MAC_RESOURCE_BASE;
    index->Common=D3DCOMMON_TYPE_INDEXBUFFER|1;index->Data=(DWORD)copy;
    memset(v,0,sizeof(*v));memset(t,0,sizeof(*t));
    v->type=_rasterizer_vertex_type_model_compressed;v->count=count;
    v->base_address=(void *)address;v->hardware_format=&r->resource;
    t->type=_triangle_buffer_type_precompiled_strip;t->count=triangle_count;
    t->base_address=copy;t->hardware_format=index;
    {
        /* Model vertices are written once, above. A certificate lets the
           renderer keep a decoded copy (HALO_MAC_WORLD_VBO); without one the
           buffer is drawn as before. */
        unsigned long long owner_token;
        (void)mac_resource_certificate_register(&r->resource,(const void *)address,
            (unsigned long)vertex_bytes,&owner_token);
    }
    (void)index_span_add(copy,(unsigned long)index_bytes);
    return 1;
}
static size_t bytes_owned(void *context,const struct vertex_buffer *v,const struct triangle_buffer *t)
{
    const struct native_vertex_resource *r=(const struct native_vertex_resource *)v->hardware_format;
    (void)context;
    return r?sizeof(*r)+r->size+sizeof(D3DIndexBuffer)+(t->count+2)*sizeof(uint16_t):0;
}
void mac_native_model_buffer_callbacks(struct mac_native_model_buffers *out)
{ memset(out,0,sizeof(*out));out->create=create;out->destroy=destroy;out->bytes_owned=bytes_owned; }

/* ---------- an Xbox map's models */
struct xbox_model_geometry { byte reserved[0x24]; struct tag_block parts; };
struct xbox_model_part
{
    unsigned long flags;
    short shader_index;
    char previous_part_index;
    char next_part_index;
    short centroid_primary_node_index;
    short centroid_secondary_node_index;
    real centroid_primary_node_weight;
    real centroid_secondary_node_weight;
    real_point3d centroid;
    struct tag_block uncompressed_vertices;
    struct tag_block compressed_vertices;
    struct tag_block triangles;
    struct triangle_buffer triangle_buffer;
    struct vertex_buffer vertex_buffer;
};
typedef char xbox_model_part_size[sizeof(struct xbox_model_part)==0x68?1:-1];
static void xbox_model_walk(int certify)
{
    struct tag_iterator iterator;
    long handle;
    tag_iterator_new(&iterator,MODELS_GROUP_TAG);
    while((handle=tag_iterator_next(&iterator))!=NONE) {
        struct model *model=(struct model *)tag_get(MODELS_GROUP_TAG,handle);
        long g,p;
        if(!model||model->geometries.count<=0||!model->geometries.address)continue;
        for(g=0;g<model->geometries.count&&g<MAXIMUM_GEOMETRIES_PER_MODEL;++g) {
            struct xbox_model_geometry *geometry=(struct xbox_model_geometry *)model->geometries.address+g;
            if(geometry->parts.count<=0||!geometry->parts.address||geometry->parts.count>MAXIMUM_PARTS_PER_MODEL_GEOMETRY)continue;
            for(p=0;p<geometry->parts.count;++p) {
                struct xbox_model_part *part=(struct xbox_model_part *)geometry->parts.address+p;
                D3DVertexBuffer *vertices=(D3DVertexBuffer *)part->vertex_buffer.hardware_format;
                D3DIndexBuffer *indices=(D3DIndexBuffer *)part->triangle_buffer.hardware_format;
                long size=rasterizer_geometry_get_vertex_size(part->vertex_buffer.type);
                unsigned long vertex_bytes,index_bytes;
                void *vertex_data,*index_data;
                if(part->vertex_buffer.count>0&&vertices&&vertices->Data&&size>0&&part->vertex_buffer.count<=0x7fffffffL/size) {
                    vertex_bytes=(unsigned long)(part->vertex_buffer.count*size);
                    vertex_data=PLATFORM_PHYSICAL_TO_VIRTUAL(vertices->Data);
                    if(certify) {
                        unsigned long long token;
                        if(mac_resource_range_is_allocated(vertex_data,vertex_bytes)&&
                           mac_resource_register_owned_range(vertex_data,vertex_bytes)) {
                            if(!mac_resource_certificate_register(vertices,vertex_data,vertex_bytes,&token))
                                mac_resource_unregister_owned_range(vertex_data);
                        }
                    } else if(mac_resource_certificate_revoke(vertices))
                        mac_resource_unregister_owned_range(vertex_data);
                }
                if(part->triangle_buffer.count>0&&indices&&indices->Data) {
                    index_bytes=((unsigned long)part->triangle_buffer.count+2)*sizeof(uint16_t);
                    index_data=PLATFORM_PHYSICAL_TO_VIRTUAL(indices->Data);
                    if(certify) {
                        if(mac_resource_range_is_allocated(index_data,index_bytes)&&
                           mac_resource_register_owned_range(index_data,index_bytes)) {
                            if(!index_span_add(index_data,index_bytes))mac_resource_unregister_owned_range(index_data);
                        }
                    } else {
                        unsigned long serial,offset,total;const void *base;
                        if(mac_native_model_index_span(index_data,2,&serial,&offset,&base,&total)&&base==index_data) {
                            index_span_remove(index_data);
                            mac_resource_unregister_owned_range(index_data);
                        }
                    }
                }
            }
        }
    }
}
void mac_native_model_buffers_certify_xbox(void){xbox_model_walk(1);}
void mac_native_model_buffers_release_xbox(void){xbox_model_walk(0);}
