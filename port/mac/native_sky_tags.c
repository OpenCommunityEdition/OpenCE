#include "cseries.h"
#include "math/real_math.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "scenario/sky_definitions.h"
#include "native_sky_tags.h"
#undef malloc
#undef calloc
#undef free
#undef memcpy
#undef memset
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SKY UINT32_C(0x736b7920)
#define MODE UINT32_C(0x6d6f6432)
#define ANTR UINT32_C(0x616e7472)
#define FOG UINT32_C(0x666f6720)
#define LENS UINT32_C(0x6c656e73)
#define NONE32 UINT32_MAX
#define SKY_BLOCK_MAX 8U
#define SKY_OWNER_MAX (1024U*1024U)
#define WITNESS(name,test) typedef char name[(test)?1:-1]

struct native_sky_light {
    struct tag_reference lens_flare;
    char marker_name[TAG_STRING_LENGTH+1];
    byte padding[0x37];
    real_euler_angles2d direction;
    byte tail_padding[4];
};
struct native_sky_animation {
    short animation_index;
    word pad;
    real period;
    byte unused[0x1c];
};
struct native_sky_shader_function {
    long unused;
    char global_function_name[TAG_STRING_LENGTH+1];
};
struct native_sky_root {
    struct tag_reference model,animation_graph;
    byte pad20[0x38];
    struct sky_atmospheric_fog outdoor_fog,indoor_fog;
    struct tag_reference indoor_fog_screen;
    long unusedA8;
    struct tag_block render_model_regions,animations,lights;
};
struct sky_allocation { void *data;size_t size;struct sky_allocation *next; };
struct sky_owner { struct sky_allocation *allocations;size_t bytes; };

WITNESS(sky_reference_size,sizeof(struct tag_reference)==16);
WITNESS(sky_fog_size,sizeof(struct sky_atmospheric_fog)==0x20);
WITNESS(sky_light_size,sizeof(struct native_sky_light)==0x74);
WITNESS(sky_light_direction_offset,offsetof(struct native_sky_light,direction)==0x68);
WITNESS(sky_animation_size,sizeof(struct native_sky_animation)==0x24);
WITNESS(sky_shader_function_size,sizeof(struct native_sky_shader_function)==0x24);
WITNESS(sky_root_model_offset,offsetof(struct native_sky_root,model)==0x00);
WITNESS(sky_root_animation_offset,offsetof(struct native_sky_root,animation_graph)==0x10);
WITNESS(sky_root_outdoor_fog_offset,offsetof(struct native_sky_root,outdoor_fog)==0x58);
WITNESS(sky_root_indoor_fog_offset,offsetof(struct native_sky_root,indoor_fog)==0x78);
WITNESS(sky_root_fog_reference_offset,offsetof(struct native_sky_root,indoor_fog_screen)==0x98);
WITNESS(sky_root_regions_offset,offsetof(struct native_sky_root,render_model_regions)==0xac);
WITNESS(sky_root_animations_offset,offsetof(struct native_sky_root,animations)==0xb8);
WITNESS(sky_root_lights_offset,offsetof(struct native_sky_root,lights)==0xc4);
WITNESS(sky_root_size,sizeof(struct native_sky_root)==0xd0);

static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t le16(const unsigned char *p)
{ return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
static int16_t sle16(const unsigned char *p)
{ uint16_t bits=le16(p);int16_t value;memcpy(&value,&bits,2);return value; }
static int32_t sle32(const unsigned char *p)
{ uint32_t bits=le32(p);int32_t value;memcpy(&value,&bits,4);return value; }
static int finite_bits(uint32_t bits)
{ return (bits&UINT32_C(0x7fffffff))<UINT32_C(0x7f800000); }
static int read_real(const unsigned char *p,real *out)
{ uint32_t bits=le32(p);if(!finite_bits(bits))return 0;memcpy(out,&bits,4);return 1; }
static void *owned(struct sky_owner *owner,size_t bytes)
{
    struct sky_allocation *allocation;void *data;
    if(!owner||!bytes||bytes>SKY_OWNER_MAX-sizeof(*allocation)||
       owner->bytes>SKY_OWNER_MAX-bytes-sizeof(*allocation))return NULL;
    allocation=(struct sky_allocation *)malloc(sizeof(*allocation));if(!allocation)return NULL;
    data=calloc(1,bytes);if(!data){free(allocation);return NULL;}
    allocation->data=data;allocation->size=bytes;allocation->next=owner->allocations;owner->allocations=allocation;
    owner->bytes+=bytes+sizeof(*allocation);return data;
}
static void owner_free(void *opaque)
{
    struct sky_owner *owner=opaque;struct sky_allocation *allocation,*next;
    if(!owner)return;for(allocation=owner->allocations;allocation;allocation=next) {
        next=allocation->next;free(allocation->data);free(allocation);
    }
    free(owner);
}
void mac_native_sky_tag_free(struct mac_native_sky_tag *tag)
{
    if(!tag)return;owner_free(tag->owner);memset(tag,0,sizeof(*tag));
}
static int copy_reference(const struct mac_retail_cache *cache,struct sky_owner *owner,
    const unsigned char *raw,uint32_t expected,struct tag_reference *out,int required)
{
    uint32_t group=le32(raw),name_address=le32(raw+4),name_length=le32(raw+8),handle=le32(raw+12);
    struct mac_retail_tag tag;size_t length;
    out->group_tag=group;out->index=0;memcpy(&out->index,&handle,4);out->name=NULL;out->name_length=0;
    /* (an empty reference may carry no group: Chill Out's sky's) */
    if(handle==NONE32)return !required&&(group==expected||group==0||group==NONE32)&&!name_address&&!name_length;
    if(group!=expected||name_length>255)return 0;
    if((handle&65535)>=cache->tag_count||!mac_retail_tag_get(cache,handle&65535,&tag)||
       tag.handle!=handle||tag.group!=expected||!name_address||
       name_address!=le32(cache->tags+cache->instances_offset+(handle&65535)*32+16))return 0;
    for(length=0;length<256&&tag.name[length];++length){}
    if(length==256||(name_length&&name_length!=length))return 0;
    out->name=owned(owner,length+1);if(!out->name)return 0;
    memcpy(out->name,tag.name,length+1);out->name_length=(long)length;return 1;
}
static int copy_fog(const unsigned char *raw,struct sky_atmospheric_fog *out)
{
    unsigned i;
    for(i=0;i<3;++i)if(!read_real(raw+i*4,&out->color.n[i])||out->color.n[i]<0.f||out->color.n[i]>1.f)return 0;
    for(i=0;i<3;++i)if(!read_real(raw+20+i*4,(&out->maximum_density)+i))return 0;
    if(out->maximum_density<0.f||out->maximum_density>1.f||out->start_distance<0.f||
       out->opaque_distance<0.f||out->opaque_distance<out->start_distance)return 0;
    return 1;
}
static const unsigned char *raw_block(const struct mac_retail_cache *cache,
    const unsigned char *root,uint32_t offset,uint32_t stride,uint32_t *count)
{
    uint32_t address;*count=le32(root+offset);address=le32(root+offset+4);
    if(*count>SKY_BLOCK_MAX)return NULL;
    if(!*count) {
        if(!address||address==NONE32)return (const unsigned char *)1;
        return !(address&3)&&mac_retail_tag_span(cache,address,1)?(const unsigned char *)1:NULL;
    }
    if(!address||address==NONE32||(address&3))return NULL;
    return mac_retail_tag_span(cache,address,*count*stride);
}
int mac_native_sky_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_sky_tag *out,const char **error)
{
    struct mac_native_sky_tag result;struct sky_owner *owner;struct native_sky_root *root;
    struct mac_retail_tag tag;const unsigned char *raw;uint32_t counts[3],i;
    const uint32_t offsets[3]={0xac,0xb8,0xc4},strides[3]={0x24,0x24,0x74};
    if(error)*error="native sky arguments";if(!out)return 0;memset(out,0,sizeof(*out));memset(&result,0,sizeof(result));
    if(!cache||!cache->tags||handle==NONE32||(handle&65535)>=cache->tag_count||
       !mac_retail_tag_get(cache,handle&65535,&tag)||tag.handle!=handle||tag.group!=SKY)goto fail;
    raw=mac_retail_tag_span(cache,tag.address,0xd0);if(!raw)goto fail;
    owner=(struct sky_owner *)calloc(1,sizeof(*owner));if(!owner)goto fail;
    root=owned(owner,sizeof(*root));if(!root){owner_free(owner);goto fail;}
    {
        /* (no model: an indoor map's sky, which structure_visibility.c
           draws only when it has one) */
        const char *part=!copy_reference(cache,owner,raw,MODE,&root->model,0)?"model":
            !copy_reference(cache,owner,raw+0x10,ANTR,&root->animation_graph,0)?"animation graph":
            !copy_reference(cache,owner,raw+0x98,FOG,&root->indoor_fog_screen,0)?"indoor fog screen":
            !copy_fog(raw+0x58,&root->outdoor_fog)?"outdoor fog":
            !copy_fog(raw+0x78,&root->indoor_fog)?"indoor fog":NULL;
        if(part) {
            fprintf(stderr,"native sky %08lx (%s): its %s\n",(unsigned long)handle,tag.name,part);
            owner_free(owner);goto fail;
        }
    }
    root->unusedA8=sle32(raw+0xa8);
    for(i=0;i<3;++i) {
        const unsigned char *source=raw_block(cache,raw,offsets[i],strides[i],counts+i);
        struct tag_block *dest=(struct tag_block *)((unsigned char *)root+offsets[i]);
        uint32_t j;
        if(!source){owner_free(owner);goto fail;}
        dest->count=(long)counts[i];dest->definition=NULL;
        if(counts[i]) {
            void *array=owned(owner,(size_t)counts[i]*strides[i]);if(!array){owner_free(owner);goto fail;}
            dest->address=array;
            for(j=0;j<counts[i];++j) {
                const unsigned char *src=source+(size_t)j*strides[i];
                if(i==0) {
                    struct native_sky_shader_function *d=(struct native_sky_shader_function *)array+j;
                    size_t n;d->unused=sle32(src);
                    for(n=0;n<TAG_STRING_LENGTH+1 && src[4+n];++n) {}
                    if(n==TAG_STRING_LENGTH+1){owner_free(owner);goto fail;}
                    memcpy(d->global_function_name,src+4,n);d->global_function_name[n]=0;
                } else if(i==1) {
                    struct native_sky_animation *d=(struct native_sky_animation *)array+j;
                    d->animation_index=sle16(src);d->pad=le16(src+2);
                    if(!read_real(src+4,&d->period)){owner_free(owner);goto fail;}
                } else {
                    struct native_sky_light *d=(struct native_sky_light *)array+j;size_t n;
                    if(!copy_reference(cache,owner,src,LENS,&d->lens_flare,0)) {owner_free(owner);goto fail;}
                    for(n=0;n<TAG_STRING_LENGTH+1 && src[16+n];++n) {}
                    if(n==TAG_STRING_LENGTH+1){owner_free(owner);goto fail;}
                    memcpy(d->marker_name,src+16,n);d->marker_name[n]=0;
                    if(!read_real(src+0x68,&d->direction.yaw)||!read_real(src+0x6c,&d->direction.pitch)) {owner_free(owner);goto fail;}
                }
            }
        }
    }
    result.root=root;result.owner=owner;result.root_bytes=sizeof(*root);result.bytes_owned=owner->bytes;
    result.handle=handle;*out=result;if(error)*error=NULL;return 1;
fail:
    if(error)*error="native sky handle, references, fog values, or block range";return 0;
}
int mac_native_sky_publish(const struct mac_retail_cache *cache,struct mac_native_tags *tags,
    uint32_t handle,const char **error)
{
    struct mac_native_sky_tag sky;int result;
    if(!mac_native_sky_decode(cache,handle,&sky,error))return 0;
    result=mac_native_tags_publish(tags,handle,SKY,sky.root,sky.root_bytes,sky.bytes_owned,
        sky.owner,owner_free,error);
    if(result)memset(&sky,0,sizeof(sky));else mac_native_sky_tag_free(&sky);
    return result;
}
