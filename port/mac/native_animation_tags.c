#include "cseries.h"
#include "models/model_animation_definitions.h"
#include "native_animation_tags.h"
#include "retail_animation_graph.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>
/* TU-private layout also declared in model_animations.c. */
struct native_animation_node {
    char name[32];short sibling,child,parent;word pad;
    unsigned long flags;real_vector3d base_vector;real range;long pad1;
};
struct native_animation_sound { struct tag_reference sound;long unused; };
struct native_animation_weapon { long unused[4];struct tag_block animations; };
struct native_vehicle_suspension { short mass_point_index,animation_index;real unknown4,unknown8;byte opaque[8]; };
struct native_vehicle_animation { struct animation_aiming_screen_bounds steering;long unused[17];struct tag_block animations,suspensions; };
/* devices.c's animation_graph_device_animations */
struct native_device_animations { long unused[21];struct tag_block animations; };
typedef char device_animations_size[sizeof(struct native_device_animations)==96?1:-1];
typedef char graph_size[sizeof(struct animation_graph)==128?1:-1];
typedef char clip_size[sizeof(struct animation)==180?1:-1];
typedef char node_size[sizeof(struct native_animation_node)==64?1:-1];
typedef char seat_size[sizeof(struct animation_graph_unit_seat)==100?1:-1];
typedef char class_size[sizeof(struct animation_graph_weapon_class)==188?1:-1];
typedef char type_size[sizeof(struct animation_graph_weapon_type)==60?1:-1];
typedef char sound_size[sizeof(struct native_animation_sound)==20?1:-1];
typedef char overlay_size[sizeof(struct animation_graph_object_overlay)==20?1:-1];
typedef char vehicle_animation_size[sizeof(struct native_vehicle_animation)==116?1:-1];
typedef char vehicle_suspension_size[sizeof(struct native_vehicle_suspension)==20?1:-1];
typedef char vehicle_animation_indices_offset[offsetof(struct native_vehicle_animation,animations)==92?1:-1];
typedef char vehicle_animation_suspensions_offset[offsetof(struct native_vehicle_animation,suspensions)==104?1:-1];
#define OFFSET(type,field,n) typedef char offset_##type##_##field[offsetof(struct type,field)==(n)?1:-1]
OFFSET(animation_graph,animations,116);
OFFSET(animation,frame_info,72);
OFFSET(animation,nodes_with_translation_flags,92);
OFFSET(animation,nodes_with_rotation_flags,108);
OFFSET(animation,nodes_with_scale_flags,124);
OFFSET(animation,default_data,140);
OFFSET(animation,data,160);
OFFSET(animation_graph_unit_seat,animations,64);
OFFSET(animation_graph_weapon_class,animations,152);
OFFSET(animation_graph_weapon_type,animations,48);
OFFSET(native_animation_node,base_vector,44);
struct allocation { struct allocation *next;void *bytes; };
struct owner { struct allocation *list;size_t bytes; };
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static unsigned u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static void words(void *v,const unsigned char *p,unsigned begin,unsigned end,unsigned width)
{
    unsigned i;for(i=begin;i<end;i+=width) {
        if(width==2){uint16_t x=(uint16_t)u16(p+i);memcpy((char *)v+i,&x,2);}
        else {uint32_t x=u32(p+i);memcpy((char *)v+i,&x,4);}
    }
}
static int finite_words(const unsigned char *p,unsigned count)
{ unsigned i;for(i=0;i<count;++i)if((u32(p+i*4)&0x7f800000U)==0x7f800000U)return 0;return 1; }
static void *owned(struct owner *o,const void *p,size_t n)
{
    struct allocation *a;
    if(!n)n=4;
    if(n>48U*1024*1024-sizeof(*a) || o->bytes>48U*1024*1024-n-sizeof(*a))return NULL;
    a=(struct allocation *)malloc(sizeof(*a));if(!a)return NULL;
    a->bytes=calloc(1,n);if(!a->bytes){free(a);return NULL;}
    if(p)memcpy(a->bytes,p,n);
    a->next=o->list;o->list=a;o->bytes+=n+sizeof(*a);return a->bytes;
}
void mac_native_animation_owner_free(void *p)
{
    struct owner *o=(struct owner *)p;struct allocation *a,*next;
    if(!o)return;for(a=o->list;a;a=next){next=a->next;free(a->bytes);free(a);}free(o);
}
void mac_native_animation_tag_free(struct mac_native_animation_tag *p)
{ if(p){mac_native_animation_owner_free(p->owner);memset(p,0,sizeof(*p));} }
static const unsigned char *bind_block(const struct mac_retail_cache *c,struct owner *o,
    const unsigned char *p,struct tag_block *b,unsigned stride,unsigned limit)
{
    const unsigned char *raw;unsigned count=u32(p);
    memset(b,0,sizeof(*b));if(count>limit)return NULL;
    if(!count)return p;
    raw=mac_retail_tag_span(c,u32(p+4),count*stride);if(!raw)return NULL;
    b->address=owned(o,raw,count*stride);if(!b->address)return NULL;b->count=count;return raw;
}
static int bind_indices(const struct mac_retail_cache *c,struct owner *o,const unsigned char *p,
    struct tag_block *b,unsigned limit)
{
    const unsigned char *raw=bind_block(c,o,p,b,2,limit);unsigned i;
    if(!raw)return 0;for(i=0;i<(unsigned)b->count;++i)words((char *)b->address+i*2,raw+i*2,0,2,2);return 1;
}
static int bind_stream(struct owner *o,struct tag_data *d,const unsigned char *raw,
    const void *native,unsigned size)
{
    memset(d,0,sizeof(*d));d->size=size;d->pad=u32(raw+4);d->file_offset=(long)u32(raw+8);
    d->address=owned(o,size?native:NULL,size);return d->address!=NULL;
}
int mac_native_animation_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_animation_tag *out,const char **error)
{
    struct mac_retail_animation_graph decoded;struct owner *o=NULL;struct animation_graph *g;
    struct mac_retail_tag tag;const unsigned char *p,*seats,*classes,*types,*raw,*nodes,*clips,*vehicles;
    unsigned i,j,k;int result=0;
    memset(&decoded,0,sizeof(decoded));if(error)*error="native animation arguments";
    if(!out)return 0;memset(out,0,sizeof(*out));
    if(!mac_retail_animation_graph_decode(cache,handle,&decoded,error))return 0;
#define TEXT_(x) #x
#define TEXT(x) TEXT_(x)
#define REQUIRE(x) do {if(!(x)){if(error)*error="native animation check at line " TEXT(__LINE__);goto done;}}while(0)
    if(error)*error="native animation layout, hierarchy, reference or allocation";
    REQUIRE(mac_retail_tag_get(cache,handle&65535,&tag));
    p=mac_retail_tag_span(cache,tag.address,128);REQUIRE(p);
    o=(struct owner *)calloc(1,sizeof(*o));REQUIRE(o);o->bytes=sizeof(*o);
    g=(struct animation_graph *)owned(o,p,128);REQUIRE(g);
    /* Device animation records: their indices (position, power) converted,
       the rest of each record kept as it is (unused). */
    raw=bind_block(cache,o,p+48,&g->device_animations,96,4);REQUIRE(raw);
    for(i=0;i<(unsigned)g->device_animations.count;++i) {
        struct native_device_animations *d=(struct native_device_animations *)g->device_animations.address+i;
        REQUIRE(bind_indices(cache,o,raw+i*96+84,&d->animations,8));
        for(j=0;j<(unsigned)d->animations.count;++j) {
            short index=((short *)d->animations.address)[j];
            /* (the clip count decoded: g's own is not converted yet) */
            REQUIRE(index>=-1 && (index<0 || (unsigned)index<decoded.animation_count));
        }
    }
    vehicles=bind_block(cache,o,p+36,&g->vehicle_animations,116,16);REQUIRE(vehicles);
    for(i=0;i<(unsigned)g->vehicle_animations.count;++i) {
        struct native_vehicle_animation *v=(struct native_vehicle_animation *)g->vehicle_animations.address+i;
        raw=vehicles+i*116;words(v,raw,0,8,4);words(v,raw,8,12,2);
        words(v,raw,12,20,4);words(v,raw,20,24,2);words(v,raw,24,92,4);
        REQUIRE(bind_indices(cache,o,raw+92,&v->animations,32));
        raw=bind_block(cache,o,raw+104,&v->suspensions,20,16);REQUIRE(raw);
        for(j=0;j<(unsigned)v->suspensions.count;++j) {
            struct native_vehicle_suspension *s=(struct native_vehicle_suspension *)v->suspensions.address+j;
            const unsigned char *r=raw+j*20;
            words(s,r,0,4,2);words(s,r,4,12,4);
            REQUIRE(s->mass_point_index>=-1 && s->animation_index>=-1 && (s->animation_index<0 || s->animation_index<g->animations.count));
        }
    }
    raw=bind_block(cache,o,p,&g->object_overlays,20,256);REQUIRE(raw);
    for(i=0;i<(unsigned)g->object_overlays.count;++i) {
        struct animation_graph_object_overlay *overlay=(struct animation_graph_object_overlay *)g->object_overlays.address+i;
        raw=mac_retail_tag_span(cache,u32(p+4),g->object_overlays.count*20);REQUIRE(raw);
        words(overlay,raw+i*20,0,6,2);
    }
    words(g,p,96,100,4);words(g,p,100,102,2);
    seats=bind_block(cache,o,p+12,&g->unit_seats,100,32);REQUIRE(seats);
    for(i=0;i<(unsigned)g->unit_seats.count;++i) {
        struct animation_graph_unit_seat *s=(struct animation_graph_unit_seat *)g->unit_seats.address+i;
        raw=seats+i*100;words(s,raw,32,40,4);words(s,raw,40,44,2);words(s,raw,44,52,4);words(s,raw,52,56,2);
        REQUIRE(bind_indices(cache,o,raw+64,&s->animations,30));
        REQUIRE(bind_block(cache,o,raw+76,&s->ik_points,64,4));
        classes=bind_block(cache,o,raw+88,&s->weapon_classes,188,16);REQUIRE(classes);
        for(j=0;j<(unsigned)s->weapon_classes.count;++j) {
            struct animation_graph_weapon_class *c=(struct animation_graph_weapon_class *)s->weapon_classes.address+j;
            raw=classes+j*188;words(c,raw,96,104,4);words(c,raw,104,108,2);words(c,raw,108,116,4);words(c,raw,116,120,2);
            REQUIRE(bind_indices(cache,o,raw+152,&c->animations,55));
            REQUIRE(bind_block(cache,o,raw+164,&c->ik_points,64,4));
            types=bind_block(cache,o,raw+176,&c->weapon_types,60,16);REQUIRE(types);
            for(k=0;k<(unsigned)c->weapon_types.count;++k) {
                struct animation_graph_weapon_type *t=(struct animation_graph_weapon_type *)c->weapon_types.address+k;
                REQUIRE(bind_indices(cache,o,types+k*60+48,&t->animations,10));
            }
        }
    }
    raw=bind_block(cache,o,p+24,&g->weapon_animations,28,1);REQUIRE(raw);
    if(g->weapon_animations.count)REQUIRE(bind_indices(cache,o,raw+16,
        &((struct native_animation_weapon *)g->weapon_animations.address)->animations,11));
    raw=bind_block(cache,o,p+72,&g->first_person_weapon_animations,28,1);REQUIRE(raw);
    if(g->first_person_weapon_animations.count)REQUIRE(bind_indices(cache,o,raw+16,
        &((struct native_animation_weapon *)g->first_person_weapon_animations.address)->animations,28));
    REQUIRE(bind_indices(cache,o,p+60,&g->unit_damage_animations,176));
    raw=bind_block(cache,o,p+84,&g->sound_references,20,257);REQUIRE(raw);
    for(i=0;i<(unsigned)g->sound_references.count;++i) {
        struct tag_reference *r=&((struct native_animation_sound *)g->sound_references.address)[i].sound;
        const unsigned char *q=raw+i*20,*name;unsigned len=0;uint32_t address=u32(q+4);
        r->group_tag=u32(q);r->index=(long)u32(q+12);r->name_length=(long)u32(q+8);r->name=NULL;
        REQUIRE(r->group_tag==0x736e6421UL || (r->index==-1 && r->group_tag==0xffffffffUL));
        if(address) {
            do {name=mac_retail_tag_span(cache,address+len,1);REQUIRE(name && len<256);if(!*name)break;++len;}while(1);
            REQUIRE(!r->name_length || r->name_length==(long)len);
            name=mac_retail_tag_span(cache,address,len+1);REQUIRE(name);r->name=(char *)owned(o,name,len+1);REQUIRE(r->name);
        } else REQUIRE(!r->name_length);
    }
    /* A retail vehicle graph can have no graph-level node block while its
       clips still address the vehicle model's nodes. The source graph-matrix
       walker explicitly does nothing when nodes.count is zero; clip/model
       compatibility is checked by the actor asset owner. */
    nodes=bind_block(cache,o,p+104,&g->nodes,64,64);REQUIRE(nodes &&
        g->nodes.count==decoded.node_count);
    for(i=0;i<(unsigned)g->nodes.count;++i) {
        struct native_animation_node *node=(struct native_animation_node *)g->nodes.address+i;
        raw=nodes+i*64;REQUIRE(memchr(raw,0,32) && finite_words(raw+44,4));
        words(node,raw,32,38,2);words(node,raw,40,60,4);
        REQUIRE(node->sibling>=-1 && node->sibling<g->nodes.count && node->child>=-1 && node->child<g->nodes.count && node->parent>=-1 && node->parent<g->nodes.count);
        REQUIRE(node->sibling!=(short)i && node->child!=(short)i && (!i || node->parent!=(short)i));
    }
    /* Parent chains must terminate; parent/child/sibling reciprocity follows. */
    for(i=0;i<(unsigned)g->nodes.count;++i) {
        struct native_animation_node *a=(struct native_animation_node *)g->nodes.address;int at=(int)i;unsigned steps=0;
        REQUIRE(a[0].parent==-1 || a[0].parent==0);
        /* The engine ignores root parent; retail stores zero here. */
        while(at>0){REQUIRE(++steps<=(unsigned)g->nodes.count);at=a[at].parent;}
        at=(int)i;steps=0;
        while(at!=-1){REQUIRE(++steps<=(unsigned)g->nodes.count && a[at].parent==a[i].parent);at=a[at].sibling;}
        at=a[i].child;steps=0;
        while(at!=-1){REQUIRE(++steps<=(unsigned)g->nodes.count && a[at].parent==(short)i);at=a[at].sibling;}
        if(i && a[i].parent!=-1){at=a[a[i].parent].child;steps=0;while(at!=-1 && at!=(int)i){REQUIRE(++steps<=(unsigned)g->nodes.count);at=a[at].sibling;}REQUIRE(at==(int)i);}
    }
    clips=bind_block(cache,o,p+116,&g->animations,180,256);REQUIRE(clips);
    for(i=0;i<(unsigned)g->animations.count;++i) {
        struct animation *a=(struct animation *)g->animations.address+i;struct mac_retail_animation *d=decoded.animations+i;
        raw=clips+i*180;words(a,raw,32,40,2);words(a,raw,40,44,4);words(a,raw,44,48,2);
        words(a,raw,48,52,4);words(a,raw,52,64,2);words(a,raw,66,68,2);words(a,raw,68,72,4);
        words(a,raw,92,100,4);words(a,raw,108,116,4);words(a,raw,124,132,4);a->compressed_data_offset=0;
        REQUIRE(bind_stream(o,&a->frame_info,raw+72,d->frame_info,d->frame_info_size));
        REQUIRE(bind_stream(o,&a->default_data,raw+140,d->defaults,d->default_size));
        REQUIRE(bind_stream(o,&a->data,raw+160,d->data,d->data_size));
    }
    out->root=g;out->owner=o;out->handle=handle;out->group=0x616e7472U;out->root_bytes=128;out->bytes_owned=o->bytes;
    o=NULL;result=1;if(error)*error=NULL;
done:
    mac_native_animation_owner_free(o);mac_retail_animation_graph_free(&decoded);return result;
#undef REQUIRE
}
int mac_native_animation_visit_sounds(const struct animation_graph *graph,
    mac_native_animation_sound_visitor visitor,void *context)
{
    const struct native_animation_sound *sounds;
    unsigned i;
    if(!graph || !visitor || graph->sound_references.count<0 ||
       graph->sound_references.count>257 ||
       (graph->sound_references.count && !graph->sound_references.address))return 0;
    sounds=(const struct native_animation_sound *)graph->sound_references.address;
    for(i=0;i<(unsigned)graph->sound_references.count;++i) {
        const struct tag_reference *sound=&sounds[i].sound;
        if(sound->index==-1)continue;
        /* Engine tag indices are signed longs but carry the full salted
           32-bit handle. Only -1 is the NONE sentinel. */
        if((uint32_t)sound->group_tag!=UINT32_C(0x736e6421) ||
           !visitor(context,(uint32_t)sound->group_tag,(uint32_t)sound->index))return 0;
    }
    return 1;
}
