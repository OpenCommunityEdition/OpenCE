#include "retail_animation_graph.h"
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t u16(const unsigned char *p) { return (uint16_t)(p[0]|(unsigned)p[1]<<8); }
static int16_t s16(const unsigned char *p) { unsigned v=u16(p);return (int16_t)(v<32768?(int)v:(int)v-65536); }
static int scalar(const unsigned char *p,float *f)
{ uint32_t v=u32(p);if((v&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))return 0;memcpy(f,&v,4);return 1; }
static int name(char out[32],const unsigned char *p)
{ if(!memchr(p,0,32))return 0;memcpy(out,p,32);return 1; }
static int block(const struct mac_retail_cache *c,const unsigned char *b,unsigned stride,unsigned limit,
    uint32_t *count,const unsigned char **data)
{
    *count=u32(b);*data=NULL;
    if(*count>limit)return 0;
    if(!*count)return 1;
    *data=mac_retail_tag_span(c,u32(b+4),*count*stride);return *data!=NULL;
}
static int indices(const struct mac_retail_cache *c,const unsigned char *b,unsigned limit,unsigned animations,
    struct mac_animation_indices *out)
{
    const unsigned char *p;uint32_t i;
    if(!block(c,b,2,limit,&out->count,&p))return 0;
    if(!out->count)return 1;
    out->indices=(int16_t *)malloc(out->count*sizeof(*out->indices));if(!out->indices)return 0;
    for(i=0;i<out->count;++i) {
        out->indices[i]=s16(p+i*2);
        if(out->indices[i]<-1 || (out->indices[i]>=0 && (unsigned)out->indices[i]>=animations))return 0;
    }
    return 1;
}
static int aiming(const unsigned char *p,struct mac_animation_aiming *a)
{
    if(!scalar(p,&a->negative_yaw_delta) || !scalar(p+4,&a->positive_yaw_delta) ||
       !scalar(p+12,&a->negative_pitch_delta) || !scalar(p+16,&a->positive_pitch_delta))return 0;
    a->negative_yaw_frame_count=s16(p+8);a->positive_yaw_frame_count=s16(p+10);
    a->negative_pitch_frame_count=s16(p+20);a->positive_pitch_frame_count=s16(p+22);
    /* These are source `short` fields, not allocation sizes. Some first-person
       weapon graphs carry large looking-screen values that are not used by
       unit animation selection; keep their native signed-short values. */
    return a->negative_yaw_frame_count>=0 && a->positive_yaw_frame_count>=0 &&
        a->negative_pitch_frame_count>=0 && a->positive_pitch_frame_count>=0;
}
static int ik(const struct mac_retail_cache *c,const unsigned char *b,uint32_t *count,struct mac_animation_ik **out)
{
    const unsigned char *p;uint32_t i;
    if(!block(c,b,64,4,count,&p))return 0;
    if(!*count)return 1;
    *out=(struct mac_animation_ik *)calloc(*count,sizeof(**out));if(!*out)return 0;
    for(i=0;i<*count;++i)if(!name((*out)[i].marker_name,p+i*64) ||
        !name((*out)[i].attached_to_marker_name,p+i*64+32))return 0;
    return 1;
}
void mac_retail_animation_graph_free(struct mac_retail_animation_graph *g)
{
    uint32_t i,j,k;
    if(!g)return;
    if(g->seats)for(i=0;i<g->seat_count;++i) {
        struct mac_animation_seat *s=g->seats+i;
        if(s->classes)for(j=0;j<s->class_count;++j) {
            struct mac_animation_class *c=s->classes+j;
            if(c->types)for(k=0;k<c->type_count;++k)free(c->types[k].animations.indices);
            free(c->types);free(c->ik_points);free(c->animations.indices);
        }
        free(s->classes);free(s->ik_points);free(s->animations.indices);
    }
    if(g->animations)for(i=0;i<g->animation_count;++i)mac_retail_animation_free(g->animations+i);
    if(g->vehicle_animations)for(i=0;i<g->vehicle_animation_count;++i){free(g->vehicle_animations[i].animations.indices);free(g->vehicle_animations[i].suspensions);}
    free(g->vehicle_animations);free(g->animations);free(g->seats);free(g->overlays);free(g->sound_handles);
    free(g->first_person_weapon_animations.indices);free(g->weapon_animations.indices);free(g->unit_damage_animations.indices);memset(g,0,sizeof(*g));
}
int mac_retail_animation_graph_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_retail_animation_graph *out,const char **error)
{
    struct mac_retail_animation_graph g;
    struct mac_retail_tag t;
    const unsigned char *p,*seats,*classes,*types,*sounds,*weapons,*overlays;
    uint32_t i,j,k,n,bytes=0;
    memset(&g,0,sizeof(g));if(error)*error="animation selection arguments";
    if(!out)return 0;memset(out,0,sizeof(*out));
#define FAIL(message) do { if(error)*error=(message);mac_retail_animation_graph_free(&g);return 0; } while(0)
    if(!mac_retail_animation_count(cache,handle,&g.animation_count,error))return 0;
    if(!mac_retail_tag_get(cache,handle&65535,&t))FAIL("animation selection handle");
    p=mac_retail_tag_span(cache,t.address,128);if(!p)FAIL("animation selection root");
    g.handle=handle;g.node_count=u32(p+104);g.flags=u16(p+100);
    if(!scalar(p+96,&g.limp_body_node_collision_radius))FAIL("animation graph radius");
    if(!block(cache,p,20,256,&g.overlay_count,&overlays))FAIL("animation overlay block");
    if(g.overlay_count) {
        g.overlays=(struct mac_animation_overlay *)calloc(g.overlay_count,sizeof(*g.overlays));
        if(!g.overlays)FAIL("animation overlay allocation");
        for(i=0;i<g.overlay_count;++i) {
            const unsigned char *q=overlays+i*20;
            g.overlays[i].animation_index=s16(q);
            g.overlays[i].function_index=s16(q+2);
            g.overlays[i].mode=s16(q+4);
            if(g.overlays[i].animation_index < -1 ||
               (g.overlays[i].animation_index >= 0 && (unsigned)g.overlays[i].animation_index >= g.animation_count) ||
               g.overlays[i].function_index < -1 || g.overlays[i].function_index > 3 ||
               g.overlays[i].mode < 0 || g.overlays[i].mode > 1)
                FAIL("animation overlay fields");
        }
    }
    if(!block(cache,p+36,116,16,&g.vehicle_animation_count,&weapons))FAIL("animation vehicle block");
    if(g.vehicle_animation_count) {
        g.vehicle_animations=(struct mac_vehicle_animation *)calloc(g.vehicle_animation_count,sizeof(*g.vehicle_animations));
        if(!g.vehicle_animations)FAIL("animation vehicle allocation");
        for(i=0;i<g.vehicle_animation_count;++i) {
            struct mac_vehicle_animation *v=g.vehicle_animations+i;const unsigned char *q=weapons+i*116,*susp;
            if(!aiming(q,&v->steering)||!indices(cache,q+92,32,g.animation_count,&v->animations)||
               !block(cache,q+104,20,16,&v->suspension_count,&susp))FAIL("animation vehicle fields");
            if(v->suspension_count){v->suspensions=(struct mac_vehicle_suspension *)calloc(v->suspension_count,sizeof(*v->suspensions));if(!v->suspensions)FAIL("animation suspension allocation");}
            for(j=0;j<v->suspension_count;++j){const unsigned char *r=susp+j*20;struct mac_vehicle_suspension *d=v->suspensions+j;
                d->mass_point_index=s16(r);d->animation_index=s16(r+2);
                if(d->mass_point_index < -1 || d->animation_index < -1 || (d->animation_index>=0 && (unsigned)d->animation_index>=g.animation_count) || !scalar(r+4,&d->unknown4)||!scalar(r+8,&d->unknown8))FAIL("animation suspension fields");
                memcpy(d->opaque,r+12,8);
            }
        }
    }
    /* devices.c's animation_graph_device_animations (96 bytes: 84 unused,
       then the position and power animation indices), as machines and
       controls have */
    {
        const unsigned char *devices;uint32_t device_count;
        struct mac_animation_indices device_indices;
        if(!block(cache,p+48,96,4,&device_count,&devices))FAIL("device animation block");
        for(i=0;i<device_count;++i) {
            memset(&device_indices,0,sizeof(device_indices));
            if(!indices(cache,devices+i*96+84,8,g.animation_count,&device_indices)) {
                free(device_indices.indices);FAIL("device animation indices");
            }
            free(device_indices.indices);
        }
    }
    if(!block(cache,p+24,28,1,&n,&weapons))FAIL("weapon animation block");
    if(n && !indices(cache,weapons+16,11,g.animation_count,&g.weapon_animations))FAIL("weapon animation indices");
    if(!block(cache,p+72,28,1,&n,&weapons))FAIL("first person animation block");
    if(n && !indices(cache,weapons+16,28,g.animation_count,&g.first_person_weapon_animations))FAIL("first person animation indices");
    if(!indices(cache,p+60,176,g.animation_count,&g.unit_damage_animations))FAIL("damage animation indices");
    if(!block(cache,p+84,20,257,&g.sound_count,&sounds))FAIL("animation sound block");
    if(g.sound_count) {
        g.sound_handles=(uint32_t *)calloc(g.sound_count,sizeof(*g.sound_handles));
        if(!g.sound_handles)FAIL("animation sound allocation");
    }
    for(i=0;i<g.sound_count;++i) {
        uint32_t h=u32(sounds+i*20+12);
        g.sound_handles[i]=h;
        if(h!=UINT32_MAX && (!mac_retail_tag_get(cache,h&65535,&t) || t.handle!=h ||
            t.group!=UINT32_C(0x736e6421) || u32(sounds+i*20)!=t.group))FAIL("animation sound handle");
    }
    if(!block(cache,p+12,100,32,&g.seat_count,&seats))FAIL("animation seat block");
    if(g.seat_count) {
        g.seats=(struct mac_animation_seat *)calloc(g.seat_count,sizeof(*g.seats));
        if(!g.seats)FAIL("animation seat allocation");
    }
    for(i=0;i<g.seat_count;++i) {
        struct mac_animation_seat *s=g.seats+i;const unsigned char *raw=seats+i*100;
        if(!name(s->label,raw) || !aiming(raw+32,&s->looking) ||
           !indices(cache,raw+64,30,g.animation_count,&s->animations) ||
           !ik(cache,raw+76,&s->ik_count,&s->ik_points) ||
           !block(cache,raw+88,188,16,&s->class_count,&classes))FAIL("animation seat fields");
        if(s->class_count) { s->classes=(struct mac_animation_class *)calloc(s->class_count,sizeof(*s->classes));if(!s->classes)FAIL("animation class allocation"); }
        for(j=0;j<s->class_count;++j) {
            struct mac_animation_class *c=s->classes+j;raw=classes+j*188;
            if(!name(c->label,raw) || !name(c->grip_marker_name,raw+32) || !name(c->hand_marker_name,raw+64) ||
               !aiming(raw+96,&c->aiming) || !indices(cache,raw+152,55,g.animation_count,&c->animations) ||
               !ik(cache,raw+164,&c->ik_count,&c->ik_points) ||
               !block(cache,raw+176,60,16,&c->type_count,&types))FAIL("animation class fields");
            if(c->type_count) { c->types=(struct mac_animation_type *)calloc(c->type_count,sizeof(*c->types));if(!c->types)FAIL("animation type allocation"); }
            for(k=0;k<c->type_count;++k)if(!name(c->types[k].label,types+k*60) ||
                !indices(cache,types+k*60+48,10,g.animation_count,&c->types[k].animations))FAIL("animation weapon type");
        }
    }
    if(g.animation_count) { g.animations=(struct mac_retail_animation *)calloc(g.animation_count,sizeof(*g.animations));if(!g.animations)FAIL("animation clip allocation"); }
    for(i=0;i<g.animation_count;++i) {
        if(!mac_retail_animation_decode(cache,handle,i,g.animations+i,error)) { mac_retail_animation_graph_free(&g);return 0; }
        bytes+=g.animations[i].frame_info_size+g.animations[i].default_size+g.animations[i].data_size;
        if(bytes>32*1024*1024)FAIL("animation graph payload budget");
    }
    /* A next-animation chain drives weighted selection. Cycles cannot select
       a terminating list and must not reach native traversal routines. */
    for(i=0;i<g.animation_count;++i) {
        int next=(int)i;unsigned steps=0;
        while(next!=-1) {
            if(++steps>g.animation_count)FAIL("cyclic animation variant chain");
            next=g.animations[next].next_animation_index;
        }
    }
    *out=g;if(error)*error=NULL;return 1;
#undef FAIL
}
