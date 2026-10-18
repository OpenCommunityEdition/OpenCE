#include "retail_object_spawn.h"
#include "retail_skeleton.h"
#include <string.h>
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int16_t s16(const unsigned char*p){int n=p[0]|(int)p[1]<<8;return (int16_t)(n<32768?n:n-65536);}
static int floats(float*out,const unsigned char*p,unsigned n)
{ unsigned i;uint32_t w;for(i=0;i<n;++i){w=u32(p+4*i);if((w&UINT32_C(0x7fffffff))>=UINT32_C(0x7f800000))return 0;memcpy(out+i,&w,4);}return 1; }
static const unsigned char *definition(const struct mac_retail_cache*c,uint32_t h,uint32_t group,uint32_t size)
{struct mac_retail_tag t;if(h==UINT32_MAX||!mac_retail_tag_get(c,h&65535,&t)||t.handle!=h||t.group!=group)return NULL;return mac_retail_tag_span(c,t.address,size);}
static int reference(const struct mac_retail_cache*c,const unsigned char*p,struct mac_spawn_reference*out)
{
    struct mac_retail_tag t;out->group=u32(p);out->handle=u32(p+12);
    if(out->handle==UINT32_MAX)return 1;
    return mac_retail_tag_get(c,out->handle&65535,&t)&&t.handle==out->handle&&
        (t.group==out->group||t.parents[0]==out->group||t.parents[1]==out->group);
}
static int allowed(const struct mac_retail_cache*c,const struct mac_spawn_reference*r,uint32_t a,uint32_t b)
{
    struct mac_retail_tag t;if(r->handle==UINT32_MAX)return 1;
    if(!mac_retail_tag_get(c,r->handle&65535,&t))return 0;
    return t.group==a||t.parents[0]==a||t.parents[1]==a||
        (b&&(t.group==b||t.parents[0]==b||t.parents[1]==b));
}
static int block(const struct mac_retail_cache*c,const unsigned char*p,uint32_t stride,uint32_t max,uint32_t*n,const unsigned char**raw)
{
    uint32_t address=u32(p+4);*n=u32(p);*raw=NULL;
    if(*n>max)return 0;if(!*n)return 1;
    *raw=mac_retail_tag_span(c,address,*n*stride);return *raw!=NULL&&!(address&3);
}
static int object(const struct mac_retail_cache*c,const unsigned char*p,uint32_t handle,int type,struct mac_spawn_object*o)
{
    static const unsigned offsets[6]={0x28,0x38,0x70,0x80,0x90,0xa0};
    const unsigned char *raw;uint32_t i,ignored;
    o->handle=handle;o->type=s16(p);o->flags=(uint16_t)s16(p+2);o->runtime_flags=u32(p+36);o->forced_shader=s16(p+318);
    if(o->type!=type||!floats(&o->radius,p+4,1)||!floats(o->bounding_offset,p+8,3)||!floats(o->origin_offset,p+20,3)||
       !floats(&o->acceleration_scale,p+32,1)||!floats(&o->render_radius,p+260,1)||o->radius<0||o->render_radius<0)return 0;
    for(i=0;i<6;++i)if(!reference(c,p+offsets[i],&o->refs[i]))return 0;
    if(!allowed(c,&o->refs[0],UINT32_C(0x6d6f6432),0)||!allowed(c,&o->refs[1],UINT32_C(0x616e7472),0)||
       !allowed(c,&o->refs[2],UINT32_C(0x636f6c6c),0)||!allowed(c,&o->refs[3],UINT32_C(0x70687973),0)||
       !allowed(c,&o->refs[4],UINT32_C(0x73686472),0)||!allowed(c,&o->refs[5],UINT32_C(0x65666665),0))return 0;
    if(!block(c,p+0x140,72,8,&o->attachment_count,&raw))return 0;
    for(i=0;i<o->attachment_count;++i)if(!reference(c,raw+i*72,&o->attachments[i])||!memchr(raw+i*72+16,0,32))return 0;
    if(!block(c,p+0x158,360,4,&o->function_count,&raw)||!block(c,p+0x164,44,4,&o->color_count,&raw))return 0;
    /* Bounds only for these blocks: their payload conversion belongs to the
       full object/function/color/widget services, not this input bundle. */
    if(!block(c,p+0x14c,32,4,&ignored,&raw)||!block(c,p+0x170,8,1024,&ignored,&raw))return 0;
    return 1;
}
static int weapon(const struct mac_retail_cache*c,uint32_t handle,struct mac_spawn_weapon*w)
{
    const unsigned char*p,*magazines,*triggers,*m,*t,*ammo;uint32_t i,j;
    p=definition(c,handle,UINT32_C(0x77656170),0x508);
    if(!p||!object(c,p,handle,2,&w->object))return 0;
    w->flags=u32(p+0x308);if(!memchr(p+0x30c,0,32))return 0;memcpy(w->label,p+0x30c,32);
    if(!reference(c,p+0x45c,&w->first_person_model)||!reference(c,p+0x46c,&w->first_person_animation)||!reference(c,p+0x480,&w->hud))return 0;
    if(!allowed(c,&w->first_person_model,UINT32_C(0x6d6f6432),0)||!allowed(c,&w->first_person_animation,UINT32_C(0x616e7472),0)||!allowed(c,&w->hud,UINT32_C(0x77706869),0))return 0;
    if(!block(c,p+0x4f0,112,2,&w->magazine_count,&magazines)||!block(c,p+0x4fc,276,2,&w->trigger_count,&triggers))return 0;
    for(i=0;i<w->magazine_count;++i){
        struct mac_spawn_magazine*out=&w->magazines[i];m=magazines+i*112;
        out->flags=u32(m);out->recharged=s16(m+4);out->initial=s16(m+6);out->total_max=s16(m+8);out->loaded_max=s16(m+10);out->reloaded=s16(m+24);
        if(out->recharged<0||out->initial<0||out->total_max<0||out->loaded_max<0||out->reloaded<0||
           !floats(&out->reload_time,m+20,1)||!floats(&out->chamber_time,m+28,1)||out->reload_time<0||out->chamber_time<0)return 0;
        /* weapons.c weapon_new: total_initial includes loaded rounds. Do not
           clamp to total_max here; it is a separate engine capacity field. */
        out->initial_loaded=out->initial<out->loaded_max?out->initial:out->loaded_max;out->initial_spare=out->initial-out->initial_loaded;
        if(!reference(c,m+56,&out->reload_effect)||!reference(c,m+72,&out->chamber_effect)||!block(c,m+100,28,8,&out->ammunition_count,&ammo))return 0;
        if(!allowed(c,&out->reload_effect,UINT32_C(0x65666665),UINT32_C(0x736e6421))||!allowed(c,&out->chamber_effect,UINT32_C(0x65666665),UINT32_C(0x736e6421)))return 0;
        for(j=0;j<out->ammunition_count;++j){out->ammunition_rounds[j]=s16(ammo+j*28);if(out->ammunition_rounds[j]<0||!reference(c,ammo+j*28+12,&out->ammunition[j])||!allowed(c,&out->ammunition[j],UINT32_C(0x65716970),0))return 0;}
    }
    for(i=0;i<w->trigger_count;++i){
        struct mac_spawn_trigger*out=&w->triggers[i];t=triggers+i*276;
        out->flags=u32(t);out->magazine=s16(t+32);out->rounds_per_shot=s16(t+34);out->minimum_loaded=s16(t+36);out->tracer_rounds=s16(t+38);out->projectiles_per_shot=s16(t+110);
        if(out->magazine< -1||(out->magazine>=0&&(uint32_t)out->magazine>=w->magazine_count)||out->rounds_per_shot<0||out->minimum_loaded<0||out->tracer_rounds<0||out->projectiles_per_shot<0||
           !floats(&out->initial_rate,t+4,1)||!floats(&out->final_rate,t+8,1)||out->initial_rate<0||out->final_rate<0||
           !reference(c,t+92,&out->charging_effect)||!reference(c,t+148,&out->projectile)||
           !allowed(c,&out->charging_effect,UINT32_C(0x65666665),UINT32_C(0x736e6421))||!allowed(c,&out->projectile,UINT32_C(0x6f626a65),0))return 0;
    }
    return 1;
}
int mac_retail_object_spawn_decode(const struct mac_retail_cache*c,struct mac_retail_object_spawn*out,const char**error)
{
    struct mac_retail_object_spawn s;uint32_t bh,mh,i;const unsigned char*p,*weapons;
    struct mac_spawn_reference ref;
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));memset(&s,0,sizeof(s));
#define FAIL(text) do{if(error)*error=(text);return 0;}while(0)
    if(!mac_retail_multiplayer_model(c,&bh,&mh,error))return 0;
    p=definition(c,bh,UINT32_C(0x62697064),0x4f4);
    if(!p||!object(c,p,bh,0,&s.biped))FAIL("spawn biped object inputs");
    s.unit_flags=u32(p+0x17c);s.biped_flags=u32(p+0x2f4);s.default_team=s16(p+0x180);
    s.grenade_type=s16(p+0x2c4);s.grenade_count=s16(p+0x2c6);s.pelvis_node=s16(p+0x4e4);s.head_node=s16(p+0x4e6);
    if(!floats(&s.standing_camera,p+0x400,1)||!floats(&s.crouching_camera,p+0x404,1)||
       !floats(&s.standing_height,p+0x424,1)||!floats(&s.crouching_height,p+0x428,1)||!floats(&s.radius,p+0x42c,1)||
       s.radius<=0||s.standing_height<2*s.radius||s.crouching_height<2*s.radius)FAIL("spawn biped capsule inputs");
    if(s.biped.refs[1].handle==UINT32_MAX)FAIL("spawn biped requires animation");
    /* Hierarchy itself is owned/validated by the skeleton decoder. Here only
       verify the biped's cached model node indices against the model count. */
    p=definition(c,mh,UINT32_C(0x6d6f6432),0xe8);
    if(!p||u32(p+0xb8)>64||s.pelvis_node< -1||s.head_node< -1||
       (s.pelvis_node>=0&&(uint32_t)s.pelvis_node>=u32(p+0xb8))||
       (s.head_node>=0&&(uint32_t)s.head_node>=u32(p+0xb8)))FAIL("spawn biped cached nodes");
    p=definition(c,bh,UINT32_C(0x62697064),0x4f4);
    if(!block(c,p+0x2d8,36,4,&s.weapon_count,&weapons))FAIL("spawn initial weapons block");
    for(i=0;i<s.weapon_count;++i){
        if(!reference(c,weapons+i*36,&ref))FAIL("spawn initial weapon reference");
        if(ref.handle==UINT32_MAX){s.weapons[i].object.handle=UINT32_MAX;continue;}
        if(!weapon(c,ref.handle,&s.weapons[i]))FAIL("spawn weapon inputs");
    }
    *out=s;return 1;
#undef FAIL
}
