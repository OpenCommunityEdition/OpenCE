#include "cseries.h"
#include "native_antenna_tags.h"
#include "physics/point_physics.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>

typedef char witness_antenna_size[sizeof(struct antenna_definition)==0xd0?1:-1];
typedef char witness_antenna_vertices_size[sizeof(struct antenna_vertex_definition)==0x80?1:-1];
typedef char witness_point_physics_size[sizeof(struct point_physics_definition)==0x40?1:-1];
typedef char witness_antenna_vertices_offset[offsetof(struct antenna_definition,vertices)==0xc4?1:-1];
typedef char witness_antenna_texture_offset[offsetof(struct antenna_definition,texture)==0x20?1:-1];
typedef char witness_antenna_physics_offset[offsetof(struct antenna_definition,physics)==0x30?1:-1];
typedef char witness_antenna_spring_offset[offsetof(struct antenna_definition,spring_coefficient)==0x90?1:-1];
typedef char witness_antenna_vertex_vector_offset[offsetof(struct antenna_vertex_definition,vector_to_next)==0x74?1:-1];
typedef char witness_point_physics_density_offset[offsetof(struct point_physics_definition,density)==0x20?1:-1];
typedef char witness_point_physics_elasticity_offset[offsetof(struct point_physics_definition,elasticity)==0x30?1:-1];

struct antenna_owner { struct antenna_definition root;void *vertices;char *names[2];size_t bytes; };
static uint32_t u32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static int finite_bits(uint32_t bits){return (bits&UINT32_C(0x7f800000))!=UINT32_C(0x7f800000);}
static int reference(const struct mac_retail_cache *c,const unsigned char *p,
    struct tag_reference *out,uint32_t expected,char **owned,size_t *bytes)
{
    uint32_t group=u32(p),address=u32(p+4),length=u32(p+8),handle=u32(p+12);const unsigned char *name,*end;
    if(handle==UINT32_MAX) {
        if((group!=UINT32_MAX && group!=expected) || address || length)return 0;
        out->group_tag=group;out->name=NULL;out->name_length=0;out->index=NONE;return 1;
    }
    if(group!=expected || length>255)return 0;
    {
        struct mac_retail_tag tag;
        if(!mac_retail_tag_get(c,handle&65535,&tag) || tag.handle!=handle || tag.group!=expected)return 0;
    }
    out->group_tag=group;out->name=NULL;out->name_length=(long)length;out->index=(long)handle;
    if(address) {
        size_t relative,available;
        if(address<MAC_RETAIL_BASE)return 0;
        relative=(size_t)(address-MAC_RETAIL_BASE);if(relative>=c->tag_size)return 0;
        available=c->tag_size-relative;
        if(available>256)available=256;
        name=mac_retail_tag_span(c,address,1);if(!name)return 0;
        end=(const unsigned char *)memchr(name,0,available);if(!end)return 0;
        if(length && (uint32_t)(end-name)!=length)return 0;
        *owned=(char *)malloc((size_t)(end-name)+1);if(!*owned)return 0;
        memcpy(*owned,name,(size_t)(end-name)+1);out->name=*owned;*bytes+=(size_t)(end-name)+1;
    } else if(length)return 0;
    return 1;
}
void mac_native_antenna_owner_free(void *p)
{
    struct antenna_owner *o=(struct antenna_owner *)p;if(!o)return;
    free(o->vertices);free(o->names[0]);free(o->names[1]);free(o);
}
void mac_native_antenna_tag_free(struct mac_native_antenna_tag *p)
{if(p){mac_native_antenna_owner_free(p->owner);memset(p,0,sizeof(*p));}}
void mac_native_point_physics_owner_free(void *p){free(p);}
void mac_native_point_physics_tag_free(struct mac_native_point_physics_tag *p)
{if(p){mac_native_point_physics_owner_free(p->owner);memset(p,0,sizeof(*p));}}

int mac_native_antenna_decode(const struct mac_retail_cache *c,uint32_t handle,
    struct mac_native_antenna_tag *out,const char **error)
{
    struct mac_native_antenna_tag result;struct antenna_owner *o=NULL;struct mac_retail_tag tag;
    const unsigned char *p,*rows;uint32_t address,count,bits;unsigned i,j;
    static const uint32_t vertex_floats[]={0,28,32,36,44,48,52,56,60,64,68,72,116,120,124};
    if(error)*error="native antenna arguments";if(!out)return 0;memset(out,0,sizeof(*out));memset(&result,0,sizeof(result));
#define FAIL(s) do{if(error)*error=(s);mac_native_antenna_owner_free(o);return 0;}while(0)
    if(!c || !mac_retail_tag_get(c,handle&65535,&tag) || tag.handle!=handle || tag.group!=UINT32_C(0x616e7421))FAIL("antenna tag handle/group");
    p=mac_retail_tag_span(c,tag.address,sizeof(struct antenna_definition));if(!p)FAIL("antenna root bounds");
    o=(struct antenna_owner *)calloc(1,sizeof(*o));if(!o)FAIL("antenna owner allocation");
    o->bytes=sizeof(*o);memcpy(&o->root,p,sizeof(o->root));
    if(!memchr(p,0,32))FAIL("antenna attachment marker is unterminated");
    if(!reference(c,p+0x20,&o->root.texture,UINT32_C(0x6269746d),&o->names[0],&o->bytes) ||
       !reference(c,p+0x30,&o->root.physics,UINT32_C(0x70706879),&o->names[1],&o->bytes))FAIL("antenna texture/physics reference");
    for(i=0;i<4;++i){bits=u32(p+0x90+i*4);if(!finite_bits(bits))FAIL("antenna scalar is non-finite");memcpy((unsigned char *)&o->root+0x90+i*4,&bits,4);}
    count=u32(p+0xc4);address=u32(p+0xc8);
    if(count>MAXIMUM_ANTENNA_VERTICES || (count && !address) || (!count && address))FAIL("antenna vertex block count/address");
    memset(&o->root.vertices,0,sizeof(o->root.vertices));o->root.vertices.count=(long)count;
    /* The runtime expects a native tag_block_definition pointer here. The
       on-disk third word is an address and cannot be reused as a host pointer. */
    o->root.vertices.definition=NULL;
    if(count) {
        if(count>SIZE_MAX/sizeof(struct antenna_vertex_definition))FAIL("antenna vertex block overflow");
        rows=mac_retail_tag_span(c,address,(size_t)count*sizeof(struct antenna_vertex_definition));if(!rows)FAIL("antenna vertex block bounds");
        o->vertices=malloc((size_t)count*sizeof(struct antenna_vertex_definition));if(!o->vertices)FAIL("antenna vertices allocation");
        memcpy(o->vertices,rows,(size_t)count*sizeof(struct antenna_vertex_definition));o->bytes+=(size_t)count*sizeof(struct antenna_vertex_definition);
        o->root.vertices.address=o->vertices;
        for(i=0;i<count;++i){unsigned char *dst=(unsigned char *)o->vertices+i*sizeof(struct antenna_vertex_definition);const unsigned char *src=rows+i*sizeof(struct antenna_vertex_definition);
            for(j=0;j<sizeof(vertex_floats)/sizeof(vertex_floats[0]);++j){unsigned off=vertex_floats[j];bits=u32(src+off);if(!finite_bits(bits))FAIL("antenna vertex scalar is non-finite");memcpy(dst+off,&bits,4);}
            {uint16_t seq=u16(src+40);memcpy(dst+40,&seq,2);}
        }
    }
    result.root=&o->root;result.owner=o;result.handle=handle;result.group=tag.group;
    result.root_bytes=sizeof(o->root);result.bytes_owned=o->bytes;*out=result;if(error)*error=NULL;return 1;
#undef FAIL
}

int mac_native_point_physics_decode(const struct mac_retail_cache *c,uint32_t handle,
    struct mac_native_point_physics_tag *out,const char **error)
{
    static const unsigned offsets[]={4,8,12,32,36,40,44,48};
    struct mac_native_point_physics_tag result;struct mac_retail_tag tag;struct point_physics_definition *root;
    const unsigned char *p;uint32_t bits;unsigned i;
    if(error)*error="native point-physics arguments";if(!out)return 0;memset(out,0,sizeof(*out));memset(&result,0,sizeof(result));
#define FAIL(s) do{if(error)*error=(s);free(root);return 0;}while(0)
    if(!c || !mac_retail_tag_get(c,handle&65535,&tag) || tag.handle!=handle || tag.group!=UINT32_C(0x70706879)){if(error)*error="point-physics tag handle/group";return 0;}
    p=mac_retail_tag_span(c,tag.address,sizeof(*root));if(!p){if(error)*error="point-physics root bounds";return 0;}
    root=(struct point_physics_definition *)malloc(sizeof(*root));if(!root){if(error)*error="point-physics owner allocation";return 0;}memcpy(root,p,sizeof(*root));
    root->flags=u32(p);
    for(i=0;i<sizeof(offsets)/sizeof(offsets[0]);++i){bits=u32(p+offsets[i]);if(!finite_bits(bits))FAIL("point-physics scalar is non-finite");memcpy((unsigned char *)root+offsets[i],&bits,4);}
    result.root=root;result.owner=root;result.handle=handle;result.group=tag.group;result.root_bytes=sizeof(*root);result.bytes_owned=sizeof(*root);*out=result;if(error)*error=NULL;return 1;
#undef FAIL
}
