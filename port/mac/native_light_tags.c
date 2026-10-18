#include "cseries.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "native_light_tags.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>
/* Complete layout combines the object_lights.c and renderer TU-private
   views, as witnessed by the clean native tag_schema_render.c definition. */
struct native_light_definition
{
	unsigned long flags;
	real radius;
	real radius_modifier_lower_bound;
	real radius_modifier_upper_bound;
	real falloff_angle;
	real cutoff_angle;
	real lens_flare_radius;
	real runtime_cosine_falloff_angle;
	real runtime_cosine_cutoff_angle;
	real specular_radius_multiplier;
	real runtime_sine_cutoff_angle;
	byte reserved2C[0x8];
	unsigned long color_interpolation_flags;
	real_argb_color color_lower_bound;
	real_argb_color color_upper_bound;
	byte reserved58[0xC];
	/* gel */
	struct tag_reference gel_map;
	word pad74;
	short texture_animation_function;
	real texture_animation_rate;
	struct tag_reference gel_secondary_map;
	word pad8C;
	short yaw_function;
	real yaw_period;
	word pad94;
	short roll_function;
	real roll_period;
	word pad9C;
	short pitch_function;
	real pitch_period;
	byte reservedA4[0x8];
	struct tag_reference lens_flare;
	byte reservedBC[0x38];
	real transition_duration;
	word padF8;
	short falloff_function;
	byte reservedFC[0x64];
};

typedef char verify_light_gel_map_offset[offsetof(struct native_light_definition, gel_map) == 0x64 ? 1 : -1];
typedef char verify_light_pitch_function_offset[offsetof(struct native_light_definition, pitch_function) == 0x9E ? 1 : -1];
typedef char verify_light_lens_flare_offset[offsetof(struct native_light_definition, lens_flare) == 0xAC ? 1 : -1];
typedef char verify_light_falloff_function_offset[offsetof(struct native_light_definition, falloff_function) == 0xFA ? 1 : -1];
typedef char verify_light_size[sizeof(struct native_light_definition) == 0x160 ? 1 : -1];

struct light_owner { struct native_light_definition root;char *names[3];size_t bytes; };
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
void mac_native_light_owner_free(void *p)
{struct light_owner *o=(struct light_owner *)p;unsigned i;if(!o)return;for(i=0;i<3;++i)free(o->names[i]);free(o);}
void mac_native_light_tag_free(struct mac_native_light_tag *p)
{if(p){mac_native_light_owner_free(p->owner);memset(p,0,sizeof(*p));}}
int mac_native_light_decode(const struct mac_retail_cache *c,uint32_t handle,
    struct mac_native_light_tag *out,const char **error)
{
    static const unsigned floats[]={4,8,12,16,20,24,28,32,36,40,56,60,64,68,72,76,80,84,120,144,152,160,244};
    static const unsigned shorts[]={118,142,150,158,250};
    static const unsigned refs[]={100,124,172};
    static const uint32_t groups[]={0x6269746dU,0x6269746dU,0x6c656e73U};
    struct light_owner *o=NULL;struct mac_retail_tag t;const unsigned char *p,*q,*name;
    unsigned i,j,len;uint32_t value,digest=2166136261U;uint16_t half;
    if(error)*error="native light arguments";if(!out)return 0;memset(out,0,sizeof(*out));
    if(!c || !mac_retail_tag_get(c,handle&65535,&t) || t.handle!=handle || t.group!=0x6c696768U)return 0;
    p=mac_retail_tag_span(c,t.address,352);if(!p)return 0;
    o=(struct light_owner *)calloc(1,sizeof(*o));if(!o)return 0;o->bytes=sizeof(*o);memcpy(&o->root,p,352);
#define FAIL() do {if(error)*error="invalid native light scalar/reference or allocation";mac_native_light_owner_free(o);return 0;}while(0)
#define HASH(v) (digest=(digest^(uint32_t)(v))*16777619U)
    o->root.flags=u32(p);o->root.color_interpolation_flags=u32(p+52);HASH(o->root.flags);HASH(o->root.color_interpolation_flags);
    for(i=0;i<sizeof(floats)/sizeof(*floats);++i){value=u32(p+floats[i]);if((value&0x7f800000U)==0x7f800000U)FAIL();memcpy((char *)&o->root+floats[i],&value,4);HASH(value);}
    for(i=0;i<sizeof(shorts)/sizeof(*shorts);++i){half=u16(p+shorts[i]);memcpy((char *)&o->root+shorts[i],&half,2);HASH(half);}
    if(o->root.yaw_function<0 || o->root.yaw_function>=12 || o->root.roll_function<0 || o->root.roll_function>=12 || o->root.pitch_function<0 || o->root.pitch_function>=12 || o->root.falloff_function<0 || o->root.falloff_function>=6)FAIL();
    for(i=0;i<3;++i) {
        struct tag_reference *r=(struct tag_reference *)((char *)&o->root+refs[i]);
        q=p+refs[i];r->group_tag=u32(q);r->name=NULL;r->name_length=(long)u32(q+8);r->index=(long)u32(q+12);
        if(r->group_tag!=groups[i] && !(r->group_tag==0xffffffffUL && r->index==-1))FAIL();
        if(r->index!=-1 && (!mac_retail_tag_get(c,(uint32_t)r->index&65535,&t) || t.handle!=(uint32_t)r->index || t.group!=groups[i]))FAIL();
        HASH(r->group_tag);HASH(r->index);HASH(r->name_length);
        if(u32(q+4)) {
            for(len=0;len<256;++len){name=mac_retail_tag_span(c,u32(q+4)+len,1);if(!name)FAIL();if(!*name)break;}
            if(len==256 || (r->name_length && r->name_length!=(long)len))FAIL();
            name=mac_retail_tag_span(c,u32(q+4),len+1);if(!name)FAIL();
            o->names[i]=(char *)malloc(len+1);if(!o->names[i])FAIL();memcpy(o->names[i],name,len+1);r->name=o->names[i];o->bytes+=len+1;
            for(j=0;j<len;++j)HASH(name[j]);
        }else if(r->name_length)FAIL();
    }
    out->root=&o->root;out->owner=o;out->handle=handle;out->group=0x6c696768U;out->field_digest=digest;out->root_bytes=352;out->bytes_owned=o->bytes;
    if(error)*error=NULL;return 1;
#undef HASH
#undef FAIL
}

int mac_native_light_visit_references(const struct mac_native_light_tag *tag,
    int (*visit)(void *,uint32_t,uint32_t),void *context)
{
    const struct native_light_definition *light;
    const struct tag_reference *references[3];
    unsigned i;
    if(!tag||tag->group!=UINT32_C(0x6c696768)||!tag->root||
       tag->root_bytes!=sizeof(*light)||!visit)return 0;
    light=tag->root;
    references[0]=&light->gel_map;references[1]=&light->gel_secondary_map;
    references[2]=&light->lens_flare;
    for(i=0;i<3;++i)
        if(references[i]->index!=NONE &&
           !visit(context,(uint32_t)references[i]->group_tag,(uint32_t)references[i]->index))return 0;
    return 1;
}
