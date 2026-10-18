#include "cseries.h"
#include "structures/structure_bsp_definitions.h"
#include "rasterizer/rasterizer_geometry.h"
#include "native_resource_memory.h"

static int certify_hardware_buffer(struct vertex_buffer *buffer,long bytes)
{
    IDirect3DVertexBuffer8 *hardware=(IDirect3DVertexBuffer8 *)buffer->hardware_format;
    byte *data=NULL;
    unsigned long long owner_token;
    long result;
    if(!hardware||bytes<=0)return 0;
    /* Lock the just-uploaded native VB to obtain its exact live Data span.
       This port's lock path records the write; the certificate snapshots the
       resulting whole-range generation after the lock is released. */
    result=IDirect3DVertexBuffer8_Lock(hardware,0,(UINT)bytes,&data,0);
    if(result<0||!data) {
        if(data)IDirect3DVertexBuffer8_Unlock(hardware);
        return 0;
    }
    result=IDirect3DVertexBuffer8_Unlock(hardware);
    if(result<0)return 0;
    return mac_resource_certificate_register(hardware,data,(unsigned long)bytes,
        &owner_token);
}

static int prepare_buffer(struct vertex_buffer *buffer)
{
    long vertex_size,bytes;
    if(!buffer->count)return buffer->hardware_format==NULL;
    vertex_size=rasterizer_geometry_get_vertex_size(buffer->type);
    if(vertex_size<=0 || buffer->count<0 || buffer->count>0x7fffffffL/vertex_size ||
       !buffer->base_address || buffer->hardware_format)return 0;
    bytes=buffer->count*vertex_size;
    if(!rasterizer_vertex_buffer_new(buffer,buffer->type,buffer->count,
        buffer->base_address,bytes))return 0;
    /* Certification only enables the optional decode cache. A failed or
       unavailable certificate must leave the real uploaded VB usable. */
    certify_hardware_buffer(buffer,bytes);
    return 1;
}

void mac_native_world_vertex_buffers_dispose(struct structure_bsp *bsp)
{
    struct structure_lightmap *lightmaps;
    long i,j;
    if(!bsp || !bsp->lightmaps.address)return;
    lightmaps=(struct structure_lightmap *)bsp->lightmaps.address;
    for(i=0;i<bsp->lightmaps.count;++i) {
        struct structure_material *materials=(struct structure_material *)lightmaps[i].materials.address;
        for(j=0;j<lightmaps[i].materials.count;++j) {
            if(materials[j].lightmap_vertices.hardware_format)
                mac_resource_certificate_revoke(materials[j].lightmap_vertices.hardware_format);
            rasterizer_vertex_buffer_delete(&materials[j].lightmap_vertices);
            if(materials[j].vertices.hardware_format)
                mac_resource_certificate_revoke(materials[j].vertices.hardware_format);
            rasterizer_vertex_buffer_delete(&materials[j].vertices);
        }
    }
}

int mac_native_world_vertex_buffers_prepare(struct structure_bsp *bsp)
{
    struct structure_lightmap *lightmaps;
    long i,j;
    if(!bsp || (bsp->lightmaps.count && !bsp->lightmaps.address))return 0;
    lightmaps=(struct structure_lightmap *)bsp->lightmaps.address;
    for(i=0;i<bsp->lightmaps.count;++i) {
        struct structure_material *materials=(struct structure_material *)lightmaps[i].materials.address;
        if(lightmaps[i].materials.count && !materials)goto fail;
        for(j=0;j<lightmaps[i].materials.count;++j) {
            if(!prepare_buffer(&materials[j].vertices) ||
               !prepare_buffer(&materials[j].lightmap_vertices))goto fail;
        }
    }
    return 1;
fail:
    mac_native_world_vertex_buffers_dispose(bsp);
    return 0;
}

/* port: an Xbox map's bsp buffers are the map's own (registered by cache_files.c
   when the bsp was read: their data lies in the console's tag cache). The draw
   path's world cache (certificates, the vertex buffer objects built from them)
   knows a buffer by an owned range and its certificate, so each material's
   buffers get both, as the native v7 session's uploads do. A buffer that
   cannot be certified is still drawn, its vertices decoded for each draw. */
static int certify_xbox_buffer(struct vertex_buffer *buffer)
{
    D3DVertexBuffer *hardware=(D3DVertexBuffer *)buffer->hardware_format;
    long size,bytes;
    void *data;
    unsigned long long owner_token;
    if(!buffer->count)return 1;
    if(!hardware||!hardware->Data)return 0;
    size=rasterizer_geometry_get_vertex_size(buffer->type);
    if(size<=0||buffer->count<0||buffer->count>0x7fffffffL/size)return 0;
    bytes=buffer->count*size;
    data=PLATFORM_PHYSICAL_TO_VIRTUAL(hardware->Data);
    if(!mac_resource_range_is_allocated(data,(unsigned long)bytes))return 0;
    if(!mac_resource_register_owned_range(data,(unsigned long)bytes))return 0;
    if(!mac_resource_certificate_register(hardware,data,(unsigned long)bytes,&owner_token)) {
        mac_resource_unregister_owned_range(data);
        return 0;
    }
    return 1;
}
static void release_xbox_buffer(struct vertex_buffer *buffer)
{
    D3DVertexBuffer *hardware=(D3DVertexBuffer *)buffer->hardware_format;
    if(!buffer->count||!hardware||!hardware->Data)return;
    if(mac_resource_certificate_revoke(hardware))
        mac_resource_unregister_owned_range(PLATFORM_PHYSICAL_TO_VIRTUAL(hardware->Data));
}
void mac_native_world_vertex_buffers_certify_xbox(struct structure_bsp *bsp)
{
    struct structure_lightmap *lightmaps;
    long i,j;
    if(!bsp||!bsp->lightmaps.address)return;
    lightmaps=(struct structure_lightmap *)bsp->lightmaps.address;
    for(i=0;i<bsp->lightmaps.count;++i) {
        struct structure_material *materials=(struct structure_material *)lightmaps[i].materials.address;
        for(j=0;materials&&j<lightmaps[i].materials.count;++j) {
            certify_xbox_buffer(&materials[j].vertices);
            certify_xbox_buffer(&materials[j].lightmap_vertices);
        }
    }
}
void mac_native_world_vertex_buffers_release_xbox(struct structure_bsp *bsp)
{
    struct structure_lightmap *lightmaps;
    long i,j;
    if(!bsp||!bsp->lightmaps.address)return;
    lightmaps=(struct structure_lightmap *)bsp->lightmaps.address;
    for(i=0;i<bsp->lightmaps.count;++i) {
        struct structure_material *materials=(struct structure_material *)lightmaps[i].materials.address;
        for(j=0;materials&&j<lightmaps[i].materials.count;++j) {
            release_xbox_buffer(&materials[j].vertices);
            release_xbox_buffer(&materials[j].lightmap_vertices);
        }
    }
}
