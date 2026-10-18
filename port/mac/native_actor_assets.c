#include "cseries.h"
#include "objects/object_definitions.h"
#include "items/weapon_definitions.h"
#include "items/equipment_definitions.h"
#include "items/projectile_definitions.h"
#include "effects/contrail_definitions.h"
#include "models/model_definitions.h"
#include "models/model_animation_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/physics_definitions.h"
#include "native_actor_assets.h"
#include "engine_world.h"
#include "native_looping_sound_tags.h"
#include "native_sound_tags.h"
#include "sound/sound_definitions.h"
#include "native_object_tags.h"
#include "native_dialogue_tags.h"
#include "units/unit_definitions.h"
#include "ai/actor_definitions.h"
#include "native_model_tags.h"
#include "native_model_buffers.h"
#include "native_collision_tags.h"
#include "native_physics_tags.h"
#include "native_animation_tags.h"
#include "native_light_tags.h"
#include "native_antenna_tags.h"
#include "native_vehicle_layout.h"
#include "native_effect_assets.h"
#include "retail_object_spawn.h"
#include <stdio.h>
#undef memset
#undef memcmp
#undef strcmp
#include <string.h>
#include <stdlib.h>
#define MODE 0x6d6f6465U
#define COLL 0x636f6c6cU
#define ANTR 0x616e7472U
#define PHYS 0x70687973U
typedef char native_initial_weapon_stride[(sizeof(struct tag_reference)+5*sizeof(long))==36?1:-1];
static int model_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,uint32_t h,const char**error)
{
    struct mac_native_model_tag t;struct mac_native_model_buffers b;uint32_t g=mac_native_tags_group(&a->tags,h);
    if(g!=UINT32_MAX)return g==MODE;
    if(!mac_native_model_decode(c,h,&t,error))return 0;mac_native_model_buffer_callbacks(&b);
    if(!mac_native_model_finalize(&t,&b,error)||!mac_native_tags_publish_model(&a->tags,h,t.root,t.root_bytes,t.bytes_owned,t.owner,mac_native_model_owner_free,error)){mac_native_model_tag_free(&t);return 0;}
    ++a->published_count;return 1;
}
/* port: names the tag an asset load stopped on (HALO_MAC_LEVEL_DIAG=1) */
static void level_diag(const struct mac_retail_cache*c,uint32_t h,const char*kind,const char**error)
{
    struct mac_retail_tag t;
    if(getenv("HALO_MAC_LEVEL_DIAG")&&mac_retail_tag_get(c,h&65535,&t))
        fprintf(stderr,"level diag: %s %08lx (%s): %s\n",kind,(unsigned long)h,t.name,error&&*error?*error:"failed");
}
static int collision_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,uint32_t h,const char**error)
{
    struct mac_native_collision_tag t;uint32_t g;
    /* port: a vehicle or device may have no collision model (a10's) */
    if(h==UINT32_MAX)return 1;g=mac_native_tags_group(&a->tags,h);if(g!=UINT32_MAX)return g==COLL;
    if(!mac_native_collision_decode(c,h,&t,error))return 0;
    if(!mac_native_tags_publish(&a->tags,h,COLL,t.root,t.root_bytes,t.bytes_owned,t.owner,mac_native_collision_owner_free,error)){mac_native_collision_tag_free(&t);return 0;}
    ++a->published_count;return 1;
}
static int physics_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,uint32_t h,const char**error)
{
    struct mac_native_physics_tag t;uint32_t g;
    /* port: a vehicle may have no physics (a10's pilot chair) */
    if(h==UINT32_MAX)return 1;g=mac_native_tags_group(&a->tags,h);
    if(g!=UINT32_MAX)return g==PHYS;
    if(!mac_native_physics_decode(c,h,&t,error))return 0;
    if(((struct physics_definition *)t.root)->mass<=0.f) {
        if(error)*error="vehicle physics mass must be positive";
        mac_native_physics_tag_free(&t);return 0;
    }
    if(!mac_native_tags_publish(&a->tags,h,PHYS,t.root,t.root_bytes,t.bytes_owned,t.owner,
        mac_native_physics_owner_free,error)) {
        mac_native_physics_tag_free(&t);return 0;
    }
    ++a->published_count;return 1;
}
static int animation_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,uint32_t h,const char**error)
{
    struct mac_native_animation_tag t;uint32_t g; if(h==UINT32_MAX)return 1;g=mac_native_tags_group(&a->tags,h);if(g!=UINT32_MAX)return g==ANTR;
    if(!mac_native_animation_decode(c,h,&t,error)) {
        struct mac_retail_tag metadata;
        if(mac_retail_tag_get(c,h&65535,&metadata))
            fprintf(stderr,"native animation %08lx (%s): %s\n",
                (unsigned long)h,metadata.name,error&&*error?*error:"conversion failed");
        return 0;
    }
    if(!mac_native_tags_publish(&a->tags,h,ANTR,t.root,t.root_bytes,t.bytes_owned,t.owner,mac_native_animation_owner_free,error)){mac_native_animation_tag_free(&t);return 0;}
    ++a->published_count;return 1;
}
static int zero_node_graph_matches_model(const struct animation_graph*g,const struct model*m)
{
    const struct animation*animations;long i;
    if(!g||!m||g->nodes.count||!g->animations.count)return 0;
    animations=g->animations.address;
    for(i=0;i<g->animations.count;++i)if(animations[i].node_count!=m->nodes.count)return 0;
    return 1;
}
static int compatible(struct mac_actor_assets*a,struct object_definition*o,int allow_zero_node_vehicle_graph,const char**error)
{
    const struct model*m=o->object.model.index==NONE?NULL:mac_native_tags_get(&a->tags,MODE,(uint32_t)o->object.model.index,sizeof(*m),error);
    const struct collision_model*c=o->object.collision_model.index==NONE?NULL:mac_native_tags_get(&a->tags,COLL,(uint32_t)o->object.collision_model.index,sizeof(*c),error);
    const struct animation_graph*g=o->object.animation_graph.index==NONE?NULL:mac_native_tags_get(&a->tags,ANTR,(uint32_t)o->object.animation_graph.index,sizeof(*g),error);
    long i;const struct model_node*mn;const struct collision_node*cn;const struct animation*clip;
    if((o->object.model.index!=NONE&&!m)||(o->object.collision_model.index!=NONE&&!c)||(!g&&o->object.animation_graph.index!=NONE))return 0;
    if((m&&c&&m->nodes.count!=c->nodes.count)||(g&&m&&m->nodes.count!=g->nodes.count&&
       !(allow_zero_node_vehicle_graph && !g->nodes.count &&
         (g->object_overlays.count || zero_node_graph_matches_model(g,m))))){if(error)*error="actor asset node count mismatch";return 0;}
    mn=m?m->nodes.address:NULL;cn=c?c->nodes.address:NULL;clip=g?g->animations.address:NULL;
    for(i=0;m&&c&&i<m->nodes.count;++i)if(mn[i].parent_node_index!=cn[i].parent_node_index){if(error)*error="actor collision/model hierarchy mismatch";return 0;}
    for(i=0;g&&m&&i<g->animations.count;++i)if(clip[i].node_count!=m->nodes.count||(clip[i].node_list_checksum&&m->node_list_checksum&&clip[i].node_list_checksum!=m->node_list_checksum)){if(error)*error="actor animation/model checksum mismatch";return 0;}
    return 1;
}
static int contrail_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,uint32_t h,const char**error)
{
    struct mac_native_object_tag t;uint32_t g=mac_native_tags_group(&a->tags,h);
    if(g!=UINT32_MAX)return g==0x636f6e74U;
    if(!mac_native_contrail_decode(c,h,&t,error))return 0;
    if(!mac_native_tags_publish(&a->tags,h,t.group,t.root,t.root_bytes,t.bytes_owned,t.owner,mac_native_object_owner_free,error)){mac_native_object_tag_free(&t);return 0;}
    ++a->published_count;return 1;
}
static int attachments_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,struct object_definition*o,const char**error)
{
    long i;struct object_attachment_definition*attachments=o->object.attachments.address;
    for(i=0;i<o->object.attachments.count;++i){struct mac_native_light_tag light;uint32_t h=(uint32_t)attachments[i].type.index,g;
        if(h==UINT32_MAX)continue;
        if((uint32_t)attachments[i].type.group_tag==UINT32_C(0x65666665) ||
           (uint32_t)attachments[i].type.group_tag==UINT32_C(0x7063746c) ||
           /* port: light volumes, glows and lightning, loaded by the effect walk */
           (uint32_t)attachments[i].type.group_tag==UINT32_C(0x6d677332) ||
           (uint32_t)attachments[i].type.group_tag==UINT32_C(0x676c7721) ||
           (uint32_t)attachments[i].type.group_tag==UINT32_C(0x656c6563)) {
            if(mac_native_tags_group(&a->tags,h)!=(uint32_t)attachments[i].type.group_tag) {
                struct mac_retail_tag metadata;
                if(mac_retail_tag_get(c,h&65535,&metadata))
                    fprintf(stderr,"native attachment %08lx (%s) has no owned resources\n",
                        (unsigned long)h,metadata.name);
                if(error)*error="actor effect attachment needs loaded resources";return 0;
            }
            continue;
        }
        if((uint32_t)attachments[i].type.group_tag==0x636f6e74U){if(!contrail_load(c,a,h,error))return 0;continue;}
        if((uint32_t)attachments[i].type.group_tag==UINT32_C(0x6c736e64)) {
            struct mac_native_object_tag looping_sound;
            g=mac_native_tags_group(&a->tags,h);
            if(g==UINT32_C(0x6c736e64))continue;
            if(g!=UINT32_MAX){if(error)*error="actor attachment published family mismatch";return 0;}
            if(!mac_native_looping_sound_decode(c,h,&looping_sound,error))return 0;
            if(!mac_native_tags_publish(&a->tags,h,looping_sound.group,looping_sound.root,
                looping_sound.root_bytes,looping_sound.bytes_owned,looping_sound.owner,
                mac_native_object_owner_free,error)) {
                mac_native_object_tag_free(&looping_sound);return 0;
            }
            ++a->published_count;continue;
        }
        if((uint32_t)attachments[i].type.group_tag!=0x6c696768U){
            struct mac_retail_tag metadata;
            if(getenv("HALO_MAC_LEVEL_DIAG")&&mac_retail_tag_get(c,h&65535,&metadata))
                fprintf(stderr,"level diag: attachment %08lx (%s) group %08lx\n",(unsigned long)h,metadata.name,(unsigned long)attachments[i].type.group_tag);
            if(error)*error="actor attachment family unsupported";return 0;}
        g=mac_native_tags_group(&a->tags,h);if(g==0x6c696768U)continue;
        if(g!=UINT32_MAX){if(error)*error="actor attachment published family mismatch";return 0;}
        if(!mac_native_light_decode(c,h,&light,error))return 0;
        if(!mac_native_tags_publish(&a->tags,h,light.group,light.root,light.root_bytes,light.bytes_owned,light.owner,mac_native_light_owner_free,error)){mac_native_light_tag_free(&light);return 0;}
        ++a->published_count;
    }
    return 1;
}
static int projectile_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,uint32_t h,const char**error)
{
    struct mac_native_object_tag t;struct object_definition*o;uint32_t g=mac_native_tags_group(&a->tags,h);
    if(g!=UINT32_MAX)return g==0x70726f6aU;
    if(!mac_native_projectile_decode(c,h,&t,error))return 0;
    o=t.root;
    /* A projectile is drawn and collides with its own model, as any object. */
    if((o->object.model.index!=NONE&&!model_load(c,a,(uint32_t)o->object.model.index,error))||
       (o->object.collision_model.index!=NONE&&!collision_load(c,a,(uint32_t)o->object.collision_model.index,error))||
       (o->object.animation_graph.index!=NONE&&!animation_load(c,a,(uint32_t)o->object.animation_graph.index,error))||
       !compatible(a,o,0,error)||!attachments_load(c,a,o,error)){mac_native_object_tag_free(&t);return 0;}
    if(o->object.model.index!=NONE)o->object.model.group_tag=MODE;
    if(!mac_native_tags_publish(&a->tags,h,t.group,t.root,t.root_bytes,t.bytes_owned,t.owner,mac_native_object_owner_free,error)){mac_native_object_tag_free(&t);return 0;}
    ++a->published_count;return 1;
}
static int weapon_projectiles_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,struct weapon_definition*w,const char**error)
{
    long i;struct weapon_trigger_definition*t=w->weapon.triggers.address;
    for(i=0;i<w->weapon.triggers.count;++i) {
        uint32_t h=(uint32_t)t[i].projectile.index;
        if(h!=UINT32_MAX&&!projectile_load(c,a,h,error))return 0;
    }
    return 1;
}
static int dialogue_load(const struct mac_retail_cache *cache,struct mac_actor_assets *assets,
    const struct unit_definition *unit,const char **error)
{
    const struct unit_dialogue_variant *variants=unit->unit.dialogue_variants.address;
    long i;
    for(i=0;i<unit->unit.dialogue_variants.count;++i) {
        struct mac_native_object_tag tag;
        uint32_t handle=(uint32_t)variants[i].dialogue_index;
        uint32_t group;
        if(handle==UINT32_MAX)continue;
        group=mac_native_tags_group(&assets->tags,handle);
        if(group!=UINT32_MAX){if(group!=UINT32_C(0x75646c67))return 0;continue;}
        if(!mac_native_dialogue_decode(cache,handle,&tag,error))return 0;
        if(!mac_native_tags_publish(&assets->tags,handle,tag.group,tag.root,tag.root_bytes,
            tag.bytes_owned,tag.owner,mac_native_object_owner_free,error)) {
            mac_native_object_tag_free(&tag);return 0;
        }
        ++assets->published_count;
    }
    return 1;
}
static int object_load(const struct mac_retail_cache*c,struct mac_actor_assets*a,uint32_t h,int biped,const char**error)
{
    long i;struct mac_native_object_tag t;struct object_definition*o;uint32_t group=biped?0x62697064U:0x77656170U,g=mac_native_tags_group(&a->tags,h);
    if(g!=UINT32_MAX)return g==group;
    if(!(biped?mac_native_biped_decode(c,h,&t,error):mac_native_weapon_decode(c,h,&t,error)))return 0;
    o=t.root;
    if((o->object.model.index!=NONE&&!model_load(c,a,(uint32_t)o->object.model.index,error))||
       (o->object.collision_model.index!=NONE&&!collision_load(c,a,(uint32_t)o->object.collision_model.index,error))||
       (o->object.animation_graph.index!=NONE&&!animation_load(c,a,(uint32_t)o->object.animation_graph.index,error))||
       /* port: a biped's graph may have no nodes of its own, as a vehicle's (the engineer's) */
       !compatible(a,o,biped,error)||!attachments_load(c,a,o,error)){mac_native_object_tag_free(&t);return 0;}
    if(biped&&!dialogue_load(c,a,t.root,error)){mac_native_object_tag_free(&t);return 0;}
    if(!biped) {
        struct weapon_definition *weapon=t.root;
        uint32_t fp_model=(uint32_t)weapon->weapon.interface_definition.first_person_model.index;
        uint32_t fp_animation=(uint32_t)weapon->weapon.interface_definition.first_person_animations.index;
        if((fp_model!=UINT32_MAX&&!model_load(c,a,fp_model,error)) ||
           !animation_load(c,a,fp_animation,error) || !weapon_projectiles_load(c,a,t.root,error)) {
            mac_native_object_tag_free(&t);return 0;
        }
        if(fp_model!=UINT32_MAX)weapon->weapon.interface_definition.first_person_model.group_tag=MODE;
    }
    /* Only after complete conversion/publication: source mod2 is now mode. */
    o->object.model.group_tag=MODE;
    if(!mac_native_tags_publish(&a->tags,h,group,t.root,t.root_bytes,t.bytes_owned,t.owner,mac_native_object_owner_free,error)){mac_native_object_tag_free(&t);return 0;}
    ++a->published_count;return 1;
}
/* The placed objects without a unit's parts, by kind: scenery, sound
   scenery and the devices (light fixtures, machines, controls). */
enum { SCENERY_KIND, SOUND_SCENERY_KIND, LIGHT_FIXTURE_KIND, MACHINE_KIND, CONTROL_KIND, GARBAGE_KIND };
static const uint32_t scenery_kind_groups[]={UINT32_C(0x7363656e),UINT32_C(0x73736365),
    UINT32_C(0x6c696669),UINT32_C(0x6d616368),UINT32_C(0x6374726c),UINT32_C(0x67617262)};
static int scenery_kind_decode(const struct mac_retail_cache *cache,uint32_t handle,int kind,
    struct mac_native_object_tag *tag,const char **error)
{
    switch(kind) {
    case SOUND_SCENERY_KIND: return mac_native_sound_scenery_decode(cache,handle,tag,error);
    case LIGHT_FIXTURE_KIND: return mac_native_light_fixture_decode(cache,handle,tag,error);
    case MACHINE_KIND: return mac_native_machine_decode(cache,handle,tag,error);
    case CONTROL_KIND: return mac_native_control_decode(cache,handle,tag,error);
    case GARBAGE_KIND: return mac_native_garbage_decode(cache,handle,tag,error);
    default: return mac_native_scenery_decode(cache,handle,tag,error);
    }
}
static int scenery_assets_load(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,int kind,const char **error)
{
    struct mac_native_object_tag tag;
    struct object_definition *object;
    const struct model *model=NULL;
    const struct collision_model *collision=NULL;
    const struct animation_graph *animation=NULL;
    uint32_t group;
    long i;
    if(error)*error="scenery assets arguments";
    if(!cache || !assets || !assets->tags.state)return 0;
    group=mac_native_tags_group(&assets->tags,handle);
    if(group!=UINT32_MAX)return group==scenery_kind_groups[kind];
    if(!scenery_kind_decode(cache,handle,kind,&tag,error))return 0;
    object=tag.root;
    /* port: widgets (light volumes, glows, lightning) were loaded with the
       object's effect references; any other family is still unsupported */
    {
        struct object_definition_widget *widgets=object->object.widgets.address;
        for(i=0;i<object->object.widgets.count;++i) {
            uint32_t family=(uint32_t)widgets[i].type.group_tag,widget=(uint32_t)widgets[i].type.index;
            if(widget==UINT32_MAX)continue;
            if(family!=UINT32_C(0x6d677332)&&family!=UINT32_C(0x676c7721)&&family!=UINT32_C(0x656c6563)) {
                if(error)*error="scenery widget family unsupported";goto fail;
            }
            if(mac_native_tags_group(&assets->tags,widget)!=family) {
                if(error)*error="scenery widget tag not loaded";goto fail;
            }
        }
    }
    if(object->object.model.index!=NONE) {
        if(!model_load(cache,assets,(uint32_t)object->object.model.index,error))goto fail;
        model=mac_native_tags_get(&assets->tags,MODE,(uint32_t)object->object.model.index,sizeof(*model),error);
        if(!model)goto fail;
    }
    if(object->object.collision_model.index!=NONE) {
        if(!collision_load(cache,assets,(uint32_t)object->object.collision_model.index,error))goto fail;
        collision=mac_native_tags_get(&assets->tags,COLL,(uint32_t)object->object.collision_model.index,sizeof(*collision),error);
        if(!collision)goto fail;
    }
    if(!animation_load(cache,assets,(uint32_t)object->object.animation_graph.index,error))goto fail;
    if(object->object.animation_graph.index!=NONE) {
        animation=mac_native_tags_get(&assets->tags,ANTR,(uint32_t)object->object.animation_graph.index,sizeof(*animation),error);
        if(!animation)goto fail;
    }
    if((collision && (!model || model->nodes.count!=collision->nodes.count)) ||
       (animation && animation->nodes.count && (!model || model->nodes.count!=animation->nodes.count))) {
        if(error)*error="scenery asset node count mismatch";goto fail;
    }
    if(animation && animation->animations.count && !model) {
        if(error)*error="scenery animations require a model hierarchy";goto fail;
    }
    for(i=0;collision && i<collision->nodes.count;++i) {
        const struct model_node *mn=model->nodes.address;
        const struct collision_node *cn=collision->nodes.address;
        /* port: retail data has collision nodes named differently from the
           model's (a10's door jeff tube); the game matches them by index, so
           only the hierarchy is required to agree */
        if(mn[i].parent_node_index!=cn[i].parent_node_index) {
            if(getenv("HALO_MAC_LEVEL_DIAG"))fprintf(stderr,"level diag: node %ld model %s parent %d, collision %s parent %d (of %ld)\n",i,mn[i].name,(int)mn[i].parent_node_index,cn[i].name,(int)cn[i].parent_node_index,(long)collision->nodes.count);
            if(error)*error="scenery collision/model hierarchy mismatch";goto fail;
        }
    }
    for(i=0;animation && i<animation->animations.count;++i) {
        const struct animation *clip=animation->animations.address;
        if(clip[i].node_count!=model->nodes.count || (clip[i].node_list_checksum &&
           model->node_list_checksum && clip[i].node_list_checksum!=model->node_list_checksum)) {
            if(error)*error="scenery animation/model checksum mismatch";goto fail;
        }
    }
    if(!attachments_load(cache,assets,object,error))goto fail;
    if(object->object.model.index!=NONE)object->object.model.group_tag=MODE;
    if(!mac_native_tags_publish(&assets->tags,handle,tag.group,tag.root,tag.root_bytes,
        tag.bytes_owned,tag.owner,mac_native_object_owner_free,error))goto fail;
    ++assets->published_count;return 1;
fail:
    level_diag(cache,handle,"scenery",error);
    mac_native_object_tag_free(&tag);return 0;
}
int mac_actor_assets_load_scenery(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{ return scenery_assets_load(cache,assets,handle,SCENERY_KIND,error); }
int mac_actor_assets_load_sound_scenery(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{ return scenery_assets_load(cache,assets,handle,SOUND_SCENERY_KIND,error); }
int mac_actor_assets_load_light_fixture(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{ return scenery_assets_load(cache,assets,handle,LIGHT_FIXTURE_KIND,error); }
int mac_actor_assets_load_machine(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{ return scenery_assets_load(cache,assets,handle,MACHINE_KIND,error); }
int mac_actor_assets_load_control(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{ return scenery_assets_load(cache,assets,handle,CONTROL_KIND,error); }

struct vehicle_sound_queue { uint32_t *handles; unsigned count,capacity; int failed; };
static int vehicle_sound_reference(void *context,uint32_t group,uint32_t handle)
{
    struct vehicle_sound_queue *q=(struct vehicle_sound_queue *)context;unsigned i;
    if(group!=UINT32_C(0x736e6421))return 1;
    for(i=0;i<q->count;++i)if(q->handles[i]==handle)return 1;
    if(q->count>=q->capacity){q->failed=1;return 0;}
    q->handles[q->count++]=handle;return 1;
}
static int vehicle_sound_dependencies_load(const struct mac_retail_cache *cache,
    const void *sounds,size_t sound_bytes,struct mac_actor_assets *assets,
    const struct mac_native_object_tag *vehicle,const char **error)
{
    struct vehicle_sound_queue q;uint32_t cursor=0,handle;const char *why=NULL;
    memset(&q,0,sizeof(q));q.capacity=cache->tag_count;
    if(!q.capacity || q.capacity>65536U)goto invalid;
    q.handles=(uint32_t *)calloc(q.capacity,sizeof(*q.handles));if(!q.handles)goto invalid;
    if(vehicle && (!mac_native_object_visit_references(vehicle,vehicle_sound_reference,&q)||q.failed))goto fail;
    while((handle=mac_native_tags_next(&assets->tags,UINT32_C(0x6c736e64),&cursor))!=UINT32_MAX) {
        struct mac_native_object_tag looping;
        const void *root=mac_native_tags_get(&assets->tags,UINT32_C(0x6c736e64),handle,
            sizeof(struct looping_sound_definition),&why);
        if(!root)goto fail;
        memset(&looping,0,sizeof(looping));looping.handle=handle;
        looping.group=UINT32_C(0x6c736e64);looping.root=(void *)root;
        looping.root_bytes=sizeof(struct looping_sound_definition);
        if(!mac_native_object_visit_references(&looping,vehicle_sound_reference,&q)||q.failed)goto fail;
    }
    /* Sound definitions can promote another sound. Decode each unique tag
       once, and continue until the actual reference graph is closed. */
    for(cursor=0;cursor<q.count;++cursor) {
        struct mac_native_object_tag sound;
        uint32_t current=q.handles[cursor],group=mac_native_tags_group(&assets->tags,current);
        if(group==UINT32_C(0x736e6421))continue;
        if(group!=UINT32_MAX)goto fail;
        if(!mac_native_sound_decode(cache,current,sounds,sound_bytes,&sound,&why))goto fail;
        /* The decoder returns an owned native root. Visit it before ownership
           transfers to the registry, then publish exactly that allocation. */
        if(!mac_native_object_visit_references(&sound,vehicle_sound_reference,&q)||q.failed) {
            mac_native_object_tag_free(&sound);goto fail;
        }
        if(!mac_native_tags_publish(&assets->tags,current,sound.group,sound.root,sound.root_bytes,
            sound.bytes_owned,sound.owner,mac_native_object_owner_free,&why)) {
            mac_native_object_tag_free(&sound);goto fail;
        }
        ++assets->published_count;
    }
    free(q.handles);return 1;
fail:
    free(q.handles);if(error)*error=why?why:(q.failed?"vehicle sound dependency limit":"vehicle sound dependency closure");return 0;
invalid:
    free(q.handles);if(error)*error="vehicle sound dependency arguments or limit";return 0;
}

static int vehicle_widgets_load(const struct mac_retail_cache *cache,const void *pixels,size_t pixel_bytes,
    struct mac_actor_assets *assets,struct object_definition *object,const char **error)
{
    struct object_definition_widget *widgets=object->object.widgets.address;long i;
    for(i=0;i<object->object.widgets.count;++i) {
        uint32_t family=(uint32_t)widgets[i].type.group_tag,handle=(uint32_t)widgets[i].type.index,group;
        struct mac_native_antenna_tag antenna;
        if(handle==UINT32_MAX)continue;
        if(family!=UINT32_C(0x616e7421)){if(error)*error="unsupported nonempty vehicle widget family";return 0;}
        group=mac_native_tags_group(&assets->tags,handle);
        if(group!=UINT32_MAX){if(group!=family){if(error)*error="vehicle widget family mismatch";return 0;}continue;}
        if(!mac_native_antenna_decode(cache,handle,&antenna,error))return 0;
        if(antenna.root->texture.index!=NONE &&
           !mac_actor_assets_load_bitmap(cache,pixels,pixel_bytes,assets,
               (uint32_t)antenna.root->texture.index,error)) {
            mac_native_antenna_tag_free(&antenna);return 0;
        }
        if(antenna.root->physics.index!=NONE) {
            uint32_t physics_handle=(uint32_t)antenna.root->physics.index;
            group=mac_native_tags_group(&assets->tags,physics_handle);
            if(group==UINT32_MAX) {
                struct mac_native_point_physics_tag physics;
                if(!mac_native_point_physics_decode(cache,physics_handle,&physics,error)) {
                    mac_native_antenna_tag_free(&antenna);return 0;
                }
                if(!mac_native_tags_publish(&assets->tags,physics_handle,physics.group,physics.root,
                    physics.root_bytes,physics.bytes_owned,physics.owner,mac_native_point_physics_owner_free,error)) {
                    mac_native_point_physics_tag_free(&physics);mac_native_antenna_tag_free(&antenna);return 0;
                }
                ++assets->published_count;
            } else if(group!=UINT32_C(0x70706879)) {
                mac_native_antenna_tag_free(&antenna);if(error)*error="antenna physics family mismatch";return 0;
            }
        }
        if(!mac_native_tags_publish(&assets->tags,handle,antenna.group,antenna.root,antenna.root_bytes,
            antenna.bytes_owned,antenna.owner,mac_native_antenna_owner_free,error)) {
            mac_native_antenna_tag_free(&antenna);return 0;
        }
        ++assets->published_count;
    }
    return 1;
}

int mac_actor_assets_load_vehicle_with_resources(const struct mac_retail_cache *cache,const void *pixels,
    size_t pixel_bytes,const void *sounds,size_t sound_bytes,struct mac_actor_assets *assets,
    uint32_t handle,const char **error)
{
    struct mac_native_object_tag tag;struct vehicle_definition *vehicle;
    struct object_definition *object;struct tag_block *initial_weapons;long i;uint32_t group;
    if(error)*error="native vehicle assets arguments";
    if(!cache || !assets || !assets->tags.state)return 0;
    group=mac_native_tags_group(&assets->tags,handle);
    if(group!=UINT32_MAX)return group==UINT32_C(0x76656869);
    if(!mac_native_vehicle_decode(cache,handle,&tag,error))return 0;
    vehicle=(struct vehicle_definition *)tag.root;object=(struct object_definition *)&vehicle->unit.object;
    if(!mac_native_object_effect_assets_load(cache,pixels,pixel_bytes,sounds,sound_bytes,
           assets,handle,error) ||
       !model_load(cache,assets,(uint32_t)object->object.model.index,error) ||
       !collision_load(cache,assets,(uint32_t)object->object.collision_model.index,error) ||
       !animation_load(cache,assets,(uint32_t)object->object.animation_graph.index,error) ||
       !compatible(assets,object,1,error) ||
       !physics_load(cache,assets,(uint32_t)object->object.physics.index,error) ||
       !attachments_load(cache,assets,object,error) ||
       !vehicle_widgets_load(cache,pixels,pixel_bytes,assets,object,error) ||
       !dialogue_load(cache,assets,&vehicle->unit,error))goto fail;
    initial_weapons=&vehicle->unit.unit.initial_weapons;
    for(i=0;i<initial_weapons->count;++i) {
        struct tag_reference *weapon=(struct tag_reference *)((unsigned char *)initial_weapons->address+
            i*(sizeof(struct tag_reference)+5*sizeof(long)));
        uint32_t weapon_handle=(uint32_t)weapon->index;
        if(weapon_handle!=UINT32_MAX &&
           (!mac_native_object_effect_assets_load(cache,pixels,pixel_bytes,sounds,sound_bytes,
                assets,weapon_handle,error) ||
            !object_load(cache,assets,weapon_handle,0,error)))goto fail;
    }
    if(!vehicle_sound_dependencies_load(cache,sounds,sound_bytes,assets,&tag,error))goto fail;
    object->object.model.group_tag=MODE;
    if(!mac_native_tags_publish(&assets->tags,handle,UINT32_C(0x76656869),tag.root,
        tag.root_bytes,tag.bytes_owned,tag.owner,mac_native_object_owner_free,error))goto fail;
    ++assets->published_count;return 1;
fail:
    level_diag(cache,handle,"vehicle",error);
    mac_native_object_tag_free(&tag);return 0;
}
int mac_actor_assets_load_vehicle_with_bitmaps(const struct mac_retail_cache *cache,const void *pixels,
    size_t pixel_bytes,struct mac_actor_assets *assets,uint32_t handle,const char **error)
{ return mac_actor_assets_load_vehicle_with_resources(cache,pixels,pixel_bytes,NULL,0,assets,handle,error); }
int mac_actor_assets_load_vehicle(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{ return mac_actor_assets_load_vehicle_with_bitmaps(cache,NULL,0,assets,handle,error); }
int mac_actor_assets_load_world_sounds(const struct mac_retail_cache *cache,
    const struct mac_engine_world *world,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,const char **error)
{
    uint32_t *handles,count,i,group;const char *why=NULL;
    if(error)*error="world sound asset arguments";
    if(!cache||!world||!assets||!assets->tags.state||!cache->tag_count||cache->tag_count>65536U)return 0;
    handles=(uint32_t *)calloc(cache->tag_count,sizeof(*handles));if(!handles)return 0;
    for(group=UINT32_C(0x6c736e64);;group=UINT32_C(0x736e6465)) {
        if(!mac_engine_world_sound_handles(world,group,handles,cache->tag_count,&count))goto fail;
        for(i=0;i<count;++i) {
            uint32_t h=handles[i],loaded=mac_native_tags_group(&assets->tags,h);
            if(h==UINT32_MAX)continue;
            if(group==UINT32_C(0x6c736e64)) {
                struct mac_native_object_tag tag;
                if(loaded==group)continue;
                if(loaded!=UINT32_MAX){why="world background sound palette family mismatch";goto fail;}
                if(!mac_native_looping_sound_decode(cache,h,&tag,&why))goto fail;
                if(!mac_native_tags_publish(&assets->tags,h,tag.group,tag.root,tag.root_bytes,
                    tag.bytes_owned,tag.owner,mac_native_object_owner_free,&why)) {
                    mac_native_object_tag_free(&tag);goto fail;
                }
                ++assets->published_count;
            } else {
                struct mac_native_object_tag tag;
                if(loaded==group)continue;
                if(loaded!=UINT32_MAX){why="world sound environment palette family mismatch";goto fail;}
                if(!mac_native_sound_environment_decode(cache,h,&tag,&why))goto fail;
                if(!mac_native_tags_publish(&assets->tags,h,tag.group,tag.root,tag.root_bytes,
                    tag.bytes_owned,tag.owner,mac_native_object_owner_free,&why)) {
                    mac_native_object_tag_free(&tag);goto fail;
                }
                ++assets->published_count;
            }
        }
        if(group==UINT32_C(0x736e6465))break;
    }
    free(handles);
    if(!vehicle_sound_dependencies_load(cache,sounds,sound_bytes,assets,NULL,error))return 0;
    return 1;
fail:
    free(handles);if(error)*error=why?why:"world sound palette validation";return 0;
}
/* The sounds every published looping sound plays: the weapons, AI and
   scripts loaded after the world's sounds publish looping sounds of their
   own (a plasma rifle's charge), so the closure runs again once all are in. */
int mac_actor_assets_load_looping_sound_dependencies(const struct mac_retail_cache *cache,
    const void *sounds,size_t sound_bytes,struct mac_actor_assets *assets,const char **error)
{
    if(!cache || !assets || !assets->tags.state) {
        if(error)*error="looping sound dependency arguments";return 0;
    }
    return vehicle_sound_dependencies_load(cache,sounds,sound_bytes,assets,NULL,error);
}
/* port: the title map's music, the looping sound ui_start_main_menu_music
   looks for (its sounds follow in the looping sound closure). 1 even if the
   map has none. */
int mac_actor_assets_load_title_music(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,const char **error)
{
    uint32_t i;
    if(!cache || !assets || !assets->tags.state) {
        if(error)*error="title music arguments";return 0;
    }
    for(i=0;i<cache->tag_count;++i) {
        struct mac_retail_tag t;struct mac_native_object_tag tag;const char *why=NULL;
        if(!mac_retail_tag_get(cache,i,&t)) {if(error)*error="title music tag lookup";return 0;}
        if(t.group!=UINT32_C(0x6c736e64) || strcmp(t.name,"sound\\music\\title1\\title1"))continue;
        if(mac_native_tags_group(&assets->tags,t.handle)==t.group)return 1;
        if(!mac_native_looping_sound_decode(cache,t.handle,&tag,&why)) {
            if(error)*error=why?why:"title music conversion";return 0;
        }
        if(!mac_native_tags_publish(&assets->tags,t.handle,tag.group,tag.root,tag.root_bytes,
            tag.bytes_owned,tag.owner,mac_native_object_owner_free,&why)) {
            mac_native_object_tag_free(&tag);if(error)*error=why?why:"title music publication";return 0;
        }
        ++assets->published_count;
        return 1;
    }
    return 1;
}
int mac_actor_assets_bind_texture_cache(struct mac_actor_assets*a)
{
    if(!a||a->bitmap_cache_bound||!a->bitmap_count)return 0;
    if(!mac_native_bitmap_cache_bind(a->bitmaps,a->bitmap_count))return 0;
    a->bitmap_cache_bound=1;return 1;
}
void mac_actor_assets_unbind_texture_cache(struct mac_actor_assets*a)
{if(a&&a->bitmap_cache_bound){mac_native_bitmap_cache_unbind();a->bitmap_cache_bound=0;}}
void mac_actor_assets_free(struct mac_actor_assets*a){if(a){mac_actor_assets_unbind_texture_cache(a);mac_native_tags_free(&a->tags);free(a->bitmaps);memset(a,0,sizeof(*a));}}
int mac_actor_assets_load(const struct mac_retail_cache*c,struct mac_actor_assets*out,const char**error)
{
    struct mac_actor_assets a;struct mac_retail_object_spawn spawn;uint32_t i;if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));memset(&a,0,sizeof(a));
    if(!mac_retail_object_spawn_decode(c,&spawn,error)||!mac_native_tags_create(c,&a.tags,error))return 0;
    a.biped=spawn.biped.handle;a.weapon_count=spawn.weapon_count;
    if(!object_load(c,&a,a.biped,1,error))goto fail;
    for(i=0;i<a.weapon_count;++i){a.weapons[i]=spawn.weapons[i].object.handle;if(!object_load(c,&a,a.weapons[i],0,error))goto fail;}
    *out=a;return 1;
fail:mac_actor_assets_free(&a);return 0;
}

int mac_actor_assets_load_bitmap(const struct mac_retail_cache*c,const void*pixels,size_t size,struct mac_actor_assets*a,uint32_t h,const char**error)
{
    struct mac_native_bitmap_tag t;struct mac_native_bitmap_tag *grown;uint32_t group;unsigned capacity;
    if(h==UINT32_MAX)return 1;
    group=mac_native_tags_group(&a->tags,h);
    if(group!=UINT32_MAX)return group==0x6269746dU;
    if(a->bitmap_cache_bound){if(error)*error="cannot add bitmap while texture cache is bound";return 0;}
    if(a->bitmap_count==a->bitmap_capacity) {
        capacity=a->bitmap_capacity?a->bitmap_capacity*2:16;
        if(capacity<a->bitmap_capacity || capacity>(unsigned)-1/sizeof(*a->bitmaps)) {
            if(error)*error="actor bitmap capacity overflow";return 0;
        }
        grown=(struct mac_native_bitmap_tag *)realloc(a->bitmaps,(size_t)capacity*sizeof(*a->bitmaps));
        if(!grown){if(error)*error="actor bitmap registry allocation";return 0;}
        a->bitmaps=grown;a->bitmap_capacity=capacity;
    }
    if(!mac_native_bitmap_decode(c,h,pixels,size,&t,error))return 0;
    if(!mac_native_bitmap_owner_accounting_bind(t.owner,&a->tags,h)) {
        if(error)*error="native bitmap registry accounting bind";
        mac_native_bitmap_tag_free(&t);return 0;
    }
    if(!mac_native_tags_publish(&a->tags,h,t.group,t.root,t.root_bytes,t.bytes_owned,t.owner,mac_native_bitmap_owner_free,error)){mac_native_bitmap_tag_free(&t);return 0;}
    /* Keep borrowed owner pointers for explicit cache binding only. */
    a->bitmaps[a->bitmap_count++]=t;++a->published_count;return 1;
}
/* port: no biped and no weapons: the title screen's map (scenario type main
   menu) spawns no player unit, and its globals name none */
int mac_actor_assets_load_empty(const struct mac_retail_cache*c,struct mac_actor_assets*out,const char**error)
{
    struct mac_actor_assets a;
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));memset(&a,0,sizeof(a));
    if(!mac_native_tags_create(c,&a.tags,error))return 0;
    a.biped=UINT32_MAX;*out=a;return 1;
}
int mac_actor_assets_load_with_bitmaps(const struct mac_retail_cache*c,const void*pixels,size_t size,
    struct mac_actor_assets*out,const char**error)
{
    struct mac_actor_assets a;uint32_t cursor=0,h;
    if(!out)return 0;memset(out,0,sizeof(*out));
    if(!mac_actor_assets_load(c,&a,error))return 0;
    while((h=mac_native_tags_next(&a.tags,0x636f6e74U,&cursor))!=UINT32_MAX) {
        const struct contrail_definition*trail=mac_native_tags_get(&a.tags,0x636f6e74U,h,sizeof(*trail),error);
        const struct shader_effect_definition*shader;
        if(!trail)goto fail;shader=(const struct shader_effect_definition*)&trail->shader;
        if(!mac_actor_assets_load_bitmap(c,pixels,size,&a,(uint32_t)trail->bitmap.index,error)||
            !mac_actor_assets_load_bitmap(c,pixels,size,&a,(uint32_t)shader->secondary_map.index,error))goto fail;
    }
    *out=a;return 1;
fail:mac_actor_assets_free(&a);return 0;
}

int mac_actor_assets_load_equipment_with_sounds(const struct mac_retail_cache *cache,
    const void *sounds,size_t sound_bytes,struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct mac_native_object_tag tag;struct object_definition *object;uint32_t group;
    if(error)*error="equipment asset arguments";
    if(!cache || !assets || !assets->tags.state)return 0;
    group=mac_native_tags_group(&assets->tags,handle);
    if(group!=UINT32_MAX)return group==UINT32_C(0x65716970);
    if(!mac_native_equipment_decode(cache,handle,&tag,error))return 0;
    object=tag.root;
    if(object->object.widgets.count) {if(error)*error="equipment widgets unsupported";goto fail;}
    if((object->object.model.index!=NONE && !model_load(cache,assets,(uint32_t)object->object.model.index,error)) ||
       (object->object.collision_model.index!=NONE && !collision_load(cache,assets,(uint32_t)object->object.collision_model.index,error)) ||
       !animation_load(cache,assets,(uint32_t)object->object.animation_graph.index,error) ||
       !compatible(assets,object,0,error) || !attachments_load(cache,assets,object,error) ||
       !vehicle_sound_dependencies_load(cache,sounds,sound_bytes,assets,&tag,error))goto fail;
    if(object->object.model.index!=NONE)object->object.model.group_tag=MODE;
    if(!mac_native_tags_publish(&assets->tags,handle,tag.group,tag.root,tag.root_bytes,
        tag.bytes_owned,tag.owner,mac_native_object_owner_free,error))goto fail;
    ++assets->published_count;return 1;
fail:
    mac_native_object_tag_free(&tag);return 0;
}
int mac_actor_assets_load_item_collection_with_sounds(const struct mac_retail_cache *cache,
    const void *sounds,size_t sound_bytes,struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct mac_native_object_tag tag;struct item_collection_definition *collection;
    struct item_permutation_definition *permutations;uint32_t group;long i;
    if(error)*error="item collection asset arguments";
    if(!cache || !assets || !assets->tags.state)return 0;
    group=mac_native_tags_group(&assets->tags,handle);
    if(group!=UINT32_MAX)return group==UINT32_C(0x69746d63);
    if(!mac_native_item_collection_decode(cache,handle,&tag,error))return 0;
    collection=tag.root;permutations=collection->permutations.address;
    for(i=0;i<collection->permutations.count;++i) {
        struct mac_retail_tag item;uint32_t h=(uint32_t)permutations[i].item.index;
        if(!mac_retail_tag_get(cache,h&65535,&item) || item.handle!=h)goto fail;
        if(item.group==UINT32_C(0x77656170)) {
            if(!object_load(cache,assets,h,0,error))goto fail;
        } else if(item.group==UINT32_C(0x65716970)) {
            if(!mac_actor_assets_load_equipment_with_sounds(cache,sounds,sound_bytes,assets,h,error))goto fail;
        } else {if(error)*error="item collection family unsupported";goto fail;}
    }
    if(!mac_native_tags_publish(&assets->tags,handle,tag.group,tag.root,tag.root_bytes,
        tag.bytes_owned,tag.owner,mac_native_object_owner_free,error))goto fail;
    ++assets->published_count;return 1;
fail:
    mac_native_object_tag_free(&tag);return 0;
}

int mac_actor_assets_load_model(const struct mac_retail_cache *cache,struct mac_actor_assets *assets,
    uint32_t handle,const char **error)
{
    if(!cache || !assets || !assets->tags.state || handle==UINT32_MAX) {
        if(error)*error="model asset arguments";return 0;
    }
    return model_load(cache,assets,handle,error);
}
int mac_actor_assets_load_animation(const struct mac_retail_cache *cache,struct mac_actor_assets *assets,
    uint32_t handle,const char **error)
{
    if(!cache || !assets || !assets->tags.state) {if(error)*error="animation asset arguments";return 0;}
    return animation_load(cache,assets,handle,error);
}

int mac_actor_assets_load_item_collection_with_resources(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct mac_native_object_tag tag;struct item_collection_definition *collection;
    struct item_permutation_definition *items;long i;
    if(!cache || !assets || !assets->tags.state) {
        if(error)*error="item collection resource arguments";return 0;
    }
    if(!mac_native_item_collection_decode(cache,handle,&tag,error))return 0;
    collection=tag.root;items=collection->permutations.address;
    for(i=0;i<collection->permutations.count;++i)
        if(!mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
            assets,(uint32_t)items[i].item.index,error)) {
            mac_native_object_tag_free(&tag);return 0;
        }
    mac_native_object_tag_free(&tag);
    return mac_actor_assets_load_item_collection_with_sounds(cache,sounds,sound_bytes,assets,handle,error);
}

int mac_actor_assets_load_weapon_with_resources(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    if(!cache || !assets || !assets->tags.state || assets->bitmap_cache_bound) {
        if(error)*error="weapon resource arguments or bound cache";return 0;
    }
    if(!mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
        assets,handle,error))return 0;
    return object_load(cache,assets,handle,0,error);
}

/* A projectile no weapon fires (the globals' grenades), with its effects. */
int mac_actor_assets_load_projectile_with_resources(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    if(!cache || !assets || !assets->tags.state || assets->bitmap_cache_bound) {
        if(error)*error="projectile resource arguments or bound cache";return 0;
    }
    if(!mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
        assets,handle,error))return 0;
    return projectile_load(cache,assets,handle,error);
}

int mac_actor_assets_load_biped_with_resources(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    if(!cache || !assets || !assets->tags.state || assets->bitmap_cache_bound) {
        if(error)*error="biped resource arguments or bound cache";return 0;
    }
    return mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,
        sounds,sound_bytes,assets,handle,error);
}

/* A biped a campaign scenario places (a palette entry): its effects, model,
   collision, animation and dialogue, then the weapons it starts with. */
int mac_actor_assets_load_placed_biped(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    const struct unit_definition *unit;
    const unsigned char *weapons;
    long i;
    if(!cache || !assets || !assets->tags.state || assets->bitmap_cache_bound) {
        if(error)*error="placed biped arguments or bound cache";return 0;
    }
    if(mac_native_tags_group(&assets->tags,handle)==UINT32_C(0x62697064))return 1;
    if(!mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
        assets,handle,error))return 0;
    if(!object_load(cache,assets,handle,1,error))return 0;
    unit=mac_native_tags_get(&assets->tags,UINT32_C(0x62697064),handle,sizeof(*unit),error);
    if(!unit)return 0;
    weapons=(const unsigned char *)unit->unit.initial_weapons.address;
    for(i=0;i<unit->unit.initial_weapons.count;++i) {
        const struct tag_reference *reference=(const struct tag_reference *)(weapons+(size_t)i*36);
        if((uint32_t)reference->index==UINT32_MAX)continue;
        if(!mac_actor_assets_load_weapon_with_resources(cache,bitmaps,bitmap_bytes,sounds,
            sound_bytes,assets,(uint32_t)reference->index,error))return 0;
    }
    return 1;
}

/* Debris an effect creates (garbage): its effects first, then its model,
   collision and attachments as a scenery object's. */
int mac_actor_assets_load_garbage_with_resources(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    if(!cache || !assets || !assets->tags.state || assets->bitmap_cache_bound) {
        if(error)*error="garbage arguments or bound cache";return 0;
    }
    if(mac_native_tags_group(&assets->tags,handle)==UINT32_C(0x67617262))return 1;
    if(!mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
        assets,handle,error))return 0;
    return scenery_assets_load(cache,assets,handle,GARBAGE_KIND,error);
}

/* An actor variant a campaign scenario's AI places (an actor palette entry):
   its actor, its unit (a biped or a vehicle), its weapon, equipment and the
   variant it upgrades to. */
static int load_actor_variant_deep(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,unsigned depth,const char **error)
{
    struct mac_native_object_tag tag,actor;
    const struct actor_variant_definition *variant;
    struct mac_retail_tag unit_tag;
    uint32_t reference;
    if(mac_native_tags_group(&assets->tags,handle)==UINT32_C(0x61637476))return 1;
    if(depth>8) {
        if(error)*error="actor variant upgrade chain too deep";return 0;
    }
    if(!mac_native_actor_variant_decode(cache,handle,&tag,error))return 0;
    variant=(const struct actor_variant_definition *)tag.root;
    reference=(uint32_t)variant->actor_reference.index;
    if(reference!=UINT32_MAX && mac_native_tags_group(&assets->tags,reference)==UINT32_MAX) {
        if(!mac_native_actor_decode(cache,reference,&actor,error))goto fail;
        if(!mac_native_tags_publish(&assets->tags,reference,actor.group,actor.root,actor.root_bytes,
            actor.bytes_owned,actor.owner,mac_native_object_owner_free,error)) {
            mac_native_object_tag_free(&actor);goto fail;
        }
        ++assets->published_count;
    }
    reference=(uint32_t)variant->unit_reference.index;
    if(reference!=UINT32_MAX) {
        if(!mac_retail_tag_get(cache,reference&65535,&unit_tag) || unit_tag.handle!=reference) {
            if(error)*error="actor variant unit reference";goto fail;
        }
        if(unit_tag.group==UINT32_C(0x62697064)) {
            if(!mac_actor_assets_load_placed_biped(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
                assets,reference,error))goto fail;
        } else if(unit_tag.group==UINT32_C(0x76656869)) {
            if(!mac_actor_assets_load_vehicle_with_resources(cache,bitmaps,bitmap_bytes,sounds,
                sound_bytes,assets,reference,error))goto fail;
        } else {
            if(error)*error="actor variant unit is not a biped or vehicle";goto fail;
        }
    }
    reference=(uint32_t)variant->ranged_combat.reference.index;
    if(reference!=UINT32_MAX && !mac_actor_assets_load_weapon_with_resources(cache,bitmaps,bitmap_bytes,
        sounds,sound_bytes,assets,reference,error))goto fail;
    reference=(uint32_t)variant->items.equipment_reference.index;
    if(reference!=UINT32_MAX && !mac_actor_assets_load_equipment_with_sounds(cache,sounds,sound_bytes,
        assets,reference,error))goto fail;
    reference=(uint32_t)variant->major_upgrade_reference.index;
    if(reference!=UINT32_MAX && !load_actor_variant_deep(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
        assets,reference,depth+1,error))goto fail;
    if(!mac_native_tags_publish(&assets->tags,handle,tag.group,tag.root,tag.root_bytes,
        tag.bytes_owned,tag.owner,mac_native_object_owner_free,error))goto fail;
    ++assets->published_count;return 1;
fail:
    mac_native_object_tag_free(&tag);return 0;
}
int mac_actor_assets_load_actor_variant(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    if(!cache || !assets || !assets->tags.state || assets->bitmap_cache_bound) {
        if(error)*error="actor variant arguments or bound cache";return 0;
    }
    return load_actor_variant_deep(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,assets,handle,0,error);
}
