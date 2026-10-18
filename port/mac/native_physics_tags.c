#include "cseries.h"
#include "physics/physics_definitions.h"
#include "native_physics_layout.h"
#include "native_physics_tags.h"
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef calloc
#undef memcpy
#undef memset

struct physics_allocation { void *data;struct physics_allocation *next; };
struct physics_owner { struct physics_allocation *head;size_t bytes; };

static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t le16(const unsigned char *p)
{ return (uint16_t)(p[0]|(unsigned)p[1]<<8); }
static short les16(const unsigned char *p)
{ uint16_t bits=le16(p);short value;memcpy(&value,&bits,sizeof(value));return value; }
static int finite_real_bits(uint32_t bits)
{ return (bits&UINT32_C(0x7f800000))!=UINT32_C(0x7f800000); }
static int read_real(const unsigned char *p,real *out)
{
    uint32_t bits=le32(p);
    if(!finite_real_bits(bits))return 0;
    memcpy(out,&bits,sizeof(bits));return 1;
}
static void *owned_alloc(struct physics_owner *owner,size_t size)
{
    const size_t budget=16U*1024U*1024U;
    struct physics_allocation *a;void *data;
    if(!size || size>budget-sizeof(*a) || owner->bytes>budget-size-sizeof(*a))return NULL;
    a=(struct physics_allocation *)malloc(sizeof(*a));if(!a)return NULL;
    data=calloc(1,size);if(!data){free(a);return NULL;}
    a->data=data;a->next=owner->head;owner->head=a;owner->bytes+=size+sizeof(*a);return data;
}
static void *block_alloc(struct physics_owner *owner,struct tag_block *block,
    const unsigned char *disk,uint32_t limit,size_t stride,const struct mac_retail_cache *cache)
{
    uint32_t count=le32(disk),address=le32(disk+4);const unsigned char *source;void *result;
    if(count>limit || !stride || count>UINT32_MAX/stride)return NULL;
    block->count=0;block->address=NULL;block->definition=NULL;
    if(!count)return (void *)1;
    if(!address || address==UINT32_MAX || (address&3))return NULL;
    source=mac_retail_tag_span(cache,address,count*(uint32_t)stride);if(!source)return NULL;
    result=owned_alloc(owner,(size_t)count*stride);if(!result)return NULL;
    block->count=(long)count;block->address=result;return (void *)source;
}
void mac_native_physics_owner_free(void *context)
{
    struct physics_owner *owner=(struct physics_owner *)context;
    struct physics_allocation *a,*next;
    if(!owner)return;
    for(a=owner->head;a;a=next){next=a->next;free(a->data);free(a);}free(owner);
}
void mac_native_physics_tag_free(struct mac_native_physics_tag *tag)
{ if(tag){mac_native_physics_owner_free(tag->owner);memset(tag,0,sizeof(*tag));} }

int mac_native_physics_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_physics_tag *out,const char **error)
{
    struct mac_native_physics_tag result;struct physics_owner *owner;
    struct mac_retail_tag tag;const unsigned char *disk,*source;struct physics_definition *root;
    struct native_physics_inertial_matrix *matrices;struct powered_mass_point_definition *powered;
    struct mass_point_definition *points;uint32_t i,j,count,offset;real value;
    if(error)*error="native physics tag arguments";if(!out)return 0;memset(out,0,sizeof(*out));memset(&result,0,sizeof(result));
    if(!cache || handle==UINT32_MAX || !mac_retail_tag_get(cache,handle&65535,&tag) ||
       tag.handle!=handle || tag.group!=UINT32_C(0x70687973))return 0;
    disk=mac_retail_tag_span(cache,tag.address,sizeof(*root));if(!disk)return 0;
    owner=(struct physics_owner *)calloc(1,sizeof(*owner));if(!owner){if(error)*error="native physics owner allocation";return 0;}
    owner->bytes=sizeof(*owner);result.owner=owner;root=(struct physics_definition *)owned_alloc(owner,sizeof(*root));
    if(!root){if(error)*error="native physics root allocation";goto fail;}
    for(i=0;i<offsetof(struct physics_definition,inertial_matrix)/sizeof(real);++i) {
        if(!read_real(disk+i*sizeof(real),&value)){if(error)*error="native physics non-finite scalar";goto fail;}
        memcpy((unsigned char *)root+i*sizeof(real),&value,sizeof(value));
    }
    source=block_alloc(owner,&root->inertial_matrix,disk+offsetof(struct physics_definition,inertial_matrix),
        2,sizeof(struct native_physics_inertial_matrix),cache);
    if(!source){if(error)*error="native physics inertial-matrix block";goto fail;}
    count=(uint32_t)root->inertial_matrix.count;matrices=(struct native_physics_inertial_matrix *)root->inertial_matrix.address;
    for(i=0;i<count;++i)for(j=0;j<9;++j)
        if(!read_real(source+i*sizeof(*matrices)+j*sizeof(real),&matrices[i].values[j])) {
            if(error)*error="native physics non-finite inertial matrix";goto fail;
        }
    source=block_alloc(owner,&root->powered_mass_points,disk+offsetof(struct physics_definition,powered_mass_points),
        MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS,sizeof(*powered),cache);
    if(!source){if(error)*error="native physics powered-mass-point block";goto fail;}
    count=(uint32_t)root->powered_mass_points.count;powered=(struct powered_mass_point_definition *)root->powered_mass_points.address;
    for(i=0;i<count;++i) {
        const unsigned char *p=source+i*sizeof(*powered);
        memcpy(powered[i].name,p,sizeof(powered[i].name));
        { uint32_t flags=le32(p+offsetof(struct powered_mass_point_definition,flags));memcpy(&powered[i].flags,&flags,4); }
        for(j=0;j<6;++j)if(!read_real(p+offsetof(struct powered_mass_point_definition,antigrav_strength)+j*sizeof(real),
            (real *)((unsigned char *)&powered[i]+offsetof(struct powered_mass_point_definition,antigrav_strength)+j*sizeof(real)))) {
            if(error)*error="native physics non-finite powered-mass-point scalar";goto fail;
        }
    }
    source=block_alloc(owner,&root->mass_points,disk+offsetof(struct physics_definition,mass_points),
        MAXIMUM_MASS_POINTS_PER_PHYSICS,sizeof(*points),cache);
    if(!source){if(error)*error="native physics mass-point block";goto fail;}
    count=(uint32_t)root->mass_points.count;points=(struct mass_point_definition *)root->mass_points.address;
    for(i=0;i<count;++i) {
        const unsigned char *p=source+i*sizeof(*points);short powered_index;
        memcpy(points[i].name,p,sizeof(points[i].name));
        points[i].powered_mass_point_index=les16(p+offsetof(struct mass_point_definition,powered_mass_point_index));
        points[i].model_node_index=les16(p+offsetof(struct mass_point_definition,model_node_index));
        { uint32_t flags=le32(p+offsetof(struct mass_point_definition,flags));memcpy(&points[i].flags,&flags,4); }
        for(j=0;j<13;++j)if(!read_real(p+offsetof(struct mass_point_definition,relative_mass)+j*sizeof(real),
            (real *)((unsigned char *)&points[i]+offsetof(struct mass_point_definition,relative_mass)+j*sizeof(real)))) {
            if(error)*error="native physics non-finite mass-point scalar";goto fail;
        }
        points[i].friction_type=les16(p+offsetof(struct mass_point_definition,friction_type));
        if(points[i].friction_type<0 || points[i].friction_type>=4) {
            if(error)*error="native physics friction enum";goto fail;
        }
        for(j=0;j<3;++j)if(!read_real(p+offsetof(struct mass_point_definition,friction_parallel_scale)+j*sizeof(real),
            (real *)((unsigned char *)&points[i]+offsetof(struct mass_point_definition,friction_parallel_scale)+j*sizeof(real)))) {
            if(error)*error="native physics non-finite friction scalar";goto fail;
        }
        memcpy(points[i].reserved6C,p+offsetof(struct mass_point_definition,reserved6C),sizeof(points[i].reserved6C));
        powered_index=points[i].powered_mass_point_index;
        if(powered_index!=NONE && (powered_index<0 || powered_index>=root->powered_mass_points.count)) {
            if(error)*error="native physics powered-mass-point index";goto fail;
        }
    }
    result.handle=handle;result.root=root;result.root_bytes=sizeof(*root);result.bytes_owned=owner->bytes;
    *out=result;if(error)*error=NULL;return 1;
fail:
    mac_native_physics_owner_free(owner);memset(out,0,sizeof(*out));return 0;
}
