#include "cseries.h"
#include "native_effect_assets.h"
#include "native_dialogue_tags.h"
#include "retail_collision_model.h"
#include "native_object_tags.h"
#include "native_sound_tags.h"
#include "native_looping_sound_tags.h"
#include "native_decal_tags.h"
#include "native_antenna_tags.h"
#include "native_light_tags.h"
#include "effects/particle_system_definitions.h"
#include "bitmaps/bitmap_group.h"
#include "objects/object_definitions.h"
#include "items/weapon_definitions.h"
#undef malloc
#undef calloc
#undef free
#undef memset
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
struct effect_queue {
    const struct mac_retail_cache *cache;
    uint32_t *handles;
    unsigned count,capacity;
    const char *error;
};
static void nonnegative(real *value) { if(*value<0.0f)*value=0.0f; }
static int particle_system_finish(struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct particle_system_definition *system=(void *)mac_native_tags_get(&assets->tags,
        UINT32_C(0x7063746c),handle,sizeof(*system),error);
    struct particle_system_type *types;long i,j;
    if(!system)return 0;types=system->types.address;
    for(i=0;i<system->types.count;++i) {
        struct particle_system_type_state *states=types[i].type_states.address;
        struct particle_system_type_particle_state *particles=types[i].particle_states.address;
        real loop_time=0.0f;
        nonnegative(&types[i].variables.radius);
        for(j=0;j<types[i].type_states.count;++j) {
            nonnegative(&states[j].variables.particle_state_multipliers.radius);
            nonnegative(&states[j].duration_lower_bound);nonnegative(&states[j].duration_upper_bound);
            nonnegative(&states[j].transition_time_lower_bound);nonnegative(&states[j].transition_time_upper_bound);
            loop_time+=states[j].duration_lower_bound+states[j].transition_time_lower_bound;
        }
        /* Match tag_schema_render.c's correction for zero-time state loops. */
        if(loop_time<0.01f)types[i].flags&=~(1UL<<_particle_system_type_type_states_loop_bit);
        loop_time=0.0f;
        for(j=0;j<types[i].particle_states.count;++j) {
            const struct bitmap_group *bitmap=mac_native_tags_get(&assets->tags,UINT32_C(0x6269746d),
                (uint32_t)particles[j].bitmaps.index,sizeof(*bitmap),error);
            long sequence=particles[j].sequence_index;
            long last=sequence+(types[i].complex_sprite_render_mode==_particle_system_type_complex_sprite_render_mode_rotational);
            nonnegative(&particles[j].variables.radius);
            nonnegative(&particles[j].duration_lower_bound);nonnegative(&particles[j].duration_upper_bound);
            nonnegative(&particles[j].transition_time_lower_bound);nonnegative(&particles[j].transition_time_upper_bound);
            loop_time+=particles[j].duration_lower_bound+particles[j].transition_time_lower_bound;
            if(!(types[i].flags&(1UL<<_particle_system_type_disabled_bit))) {
                const struct bitmap_group_sequence *sequences=bitmap?bitmap->sequences.address:NULL;
                if(!bitmap || sequence<0 || last>=bitmap->sequences.count ||
                   sequences[sequence].sprites.count<=0 || sequences[last].sprites.count<=0) {
                    if(error)*error="particle state has no usable bitmap sprite sequence";return 0;
                }
            }
        }
        if(loop_time<0.01f)types[i].flags&=~(1UL<<_particle_system_type_particle_states_loop_bit);
    }
    return 1;
}
static int enqueue(void *context,uint32_t group,uint32_t handle)
{
    struct effect_queue *q=context;struct mac_retail_tag tag;unsigned i;
    if(handle==UINT32_MAX)return 1;
    if(!mac_retail_tag_get(q->cache,handle&65535,&tag) || tag.handle!=handle ||
       (group && group!=tag.group && group!=tag.parents[0] && group!=tag.parents[1])) {
        q->error="effect dependency handle or family";return 0;
    }
    for(i=0;i<q->count;++i)if(q->handles[i]==handle)return 1;
    if(q->count==q->capacity){q->error="effect dependency count limit";return 0;}
    q->handles[q->count++]=handle;return 1;
}
int mac_native_effect_assets_load(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct effect_queue q;unsigned cursor=0;int ok=0;
    memset(&q,0,sizeof(q));q.cache=cache;
    if(error)*error=NULL;
    if(!cache || !assets || !assets->tags.state || handle==UINT32_MAX || assets->bitmap_cache_bound ||
       !cache->tag_count || cache->tag_count>65536) {
        if(error)*error="effect asset arguments or bound texture cache";return 0;
    }
    q.capacity=cache->tag_count;q.handles=calloc(q.capacity,sizeof(*q.handles));
    if(!q.handles){if(error)*error="effect dependency allocation";return 0;}
    if(!enqueue(&q,0,handle))goto done;
    for(cursor=0;cursor<q.count;++cursor) {
        struct mac_retail_tag metadata;struct mac_native_object_tag tag;
        uint32_t current=q.handles[cursor],published;int decoded;
        memset(&tag,0,sizeof(tag));
        if(!mac_retail_tag_get(cache,current&65535,&metadata))goto done;
        published=mac_native_tags_group(&assets->tags,current);
        if(published!=UINT32_MAX && published!=metadata.group){q.error="effect published family mismatch";goto done;}
        if(metadata.group==UINT32_C(0x6269746d)) {
            if(!mac_actor_assets_load_bitmap(cache,bitmaps,bitmap_bytes,assets,current,&q.error)) {
                fprintf(stderr,"native effect bitmap %08lx (%s): %s\n",
                    (unsigned long)current,metadata.name,q.error?q.error:"load failed");
                goto done;
            }
            continue;
        }
        if(metadata.group==UINT32_C(0x67617262)) {
            /* debris an effect creates: a whole object, with its model */
            if(published!=UINT32_MAX)continue;
            if(!mac_actor_assets_load_garbage_with_resources(cache,bitmaps,bitmap_bytes,sounds,
                sound_bytes,assets,current,&q.error))goto done;
            continue;
        }
        if(metadata.group==UINT32_C(0x70726f6a)) {
            /* a projectile an effect creates (campaign effects: grenades,
               bullets), with its model and effects */
            if(published!=UINT32_MAX)continue;
            if(!mac_actor_assets_load_projectile_with_resources(cache,bitmaps,bitmap_bytes,sounds,
                sound_bytes,assets,current,&q.error))goto done;
            continue;
        }
        if(metadata.group==UINT32_C(0x70706879)) {
            struct mac_native_point_physics_tag physics;
            if(published!=UINT32_MAX)continue;
            if(!mac_native_point_physics_decode(cache,current,&physics,&q.error))goto done;
            if(!mac_native_tags_publish(&assets->tags,current,physics.group,physics.root,
                physics.root_bytes,physics.bytes_owned,physics.owner,mac_native_point_physics_owner_free,&q.error)) {
                mac_native_point_physics_tag_free(&physics);goto done;
            }
            ++assets->published_count;continue;
        }
        if(metadata.group==UINT32_C(0x6c696768)) {
            struct mac_native_light_tag light;
            if(!mac_native_light_decode(cache,current,&light,&q.error))goto done;
            if(!mac_native_light_visit_references(&light,enqueue,&q)) {
                mac_native_light_tag_free(&light);goto done;
            }
            if(published!=UINT32_MAX)mac_native_light_tag_free(&light);
            else {
                if(!mac_native_tags_publish(&assets->tags,current,light.group,light.root,
                    light.root_bytes,light.bytes_owned,light.owner,mac_native_light_owner_free,&q.error)) {
                    mac_native_light_tag_free(&light);goto done;
                }
                ++assets->published_count;
            }
            continue;
        }
        switch(metadata.group) {
        case 0x73676c61:decoded=mac_native_glass_shader_decode(cache,current,&tag,&q.error);break;
        case 0x73776174:decoded=mac_native_water_shader_decode(cache,current,&tag,&q.error);break;
        case 0x73706c61:decoded=mac_native_plasma_shader_decode(cache,current,&tag,&q.error);break;
        case 0x666f6720:decoded=mac_native_fog_decode(cache,current,&tag,&q.error);break;
        case 0x6c736e64:decoded=mac_native_looping_sound_decode(cache,current,&tag,&q.error);break;
        case 0x736e6465:decoded=mac_native_sound_environment_decode(cache,current,&tag,&q.error);break;
        case 0x736d6574:decoded=mac_native_meter_shader_decode(cache,current,&tag,&q.error);break;
        case 0x736f736f:decoded=mac_native_model_shader_decode(cache,current,&tag,&q.error);break;
        case 0x73656e76:decoded=mac_native_environment_shader_decode(cache,current,&tag,&q.error);break;
        case 0x73636869:case 0x73636578:decoded=mac_native_chicago_shader_decode(cache,current,&tag,&q.error);break;
        case 0x65666665:decoded=mac_native_effect_decode(cache,current,&tag,&q.error);break;
        case 0x7063746c:decoded=mac_native_particle_system_decode(cache,current,&tag,&q.error);break;
        case 0x70617274:decoded=mac_native_particle_decode(cache,current,&tag,&q.error);break;
        case 0x636f6e74:decoded=mac_native_contrail_decode(cache,current,&tag,&q.error);break;
        case 0x64656361:decoded=mac_native_decal_decode(cache,current,&tag,&q.error);break;
        case 0x736e6421:decoded=mac_native_sound_decode(cache,current,sounds,sound_bytes,&tag,&q.error);break;
        case 0x6a707421:decoded=mac_native_damage_effect_decode(cache,current,&tag,&q.error);break;
        case 0x666f6f74:decoded=mac_native_material_effects_decode(cache,current,&tag,&q.error);break;
        case 0x6c656e73:decoded=mac_native_lens_flare_decode(cache,current,&tag,&q.error);break;
        case 0x6d677332:decoded=mac_native_light_volume_decode(cache,current,&tag,&q.error);break;
        case 0x676c7721:decoded=mac_native_glow_decode(cache,current,&tag,&q.error);break;
        case 0x656c6563:decoded=mac_native_lightning_decode(cache,current,&tag,&q.error);break;
        case 0x666c6167:decoded=mac_native_flag_decode(cache,current,&tag,&q.error);break;
        case 0x7472616b:decoded=mac_native_camera_track_decode(cache,current,&tag,&q.error);break;
        default:q.error="unsupported effect dependency family";goto done;
        }
        if(!decoded)goto done;
        if(!mac_native_object_visit_references(&tag,enqueue,&q)) {
            mac_native_object_tag_free(&tag);goto done;
        }
        if(published!=UINT32_MAX)mac_native_object_tag_free(&tag);
        else {
            if(!mac_native_tags_publish(&assets->tags,current,tag.group,tag.root,tag.root_bytes,
                tag.bytes_owned,tag.owner,mac_native_object_owner_free,&q.error)) {
                mac_native_object_tag_free(&tag);goto done;
            }
            ++assets->published_count;
        }
    }
    for(cursor=0;cursor<q.count;++cursor)
        if(mac_native_tags_group(&assets->tags,q.handles[cursor])==UINT32_C(0x7063746c) &&
           !particle_system_finish(assets,q.handles[cursor],&q.error))goto done;
    ok=1;
done:
    if(!ok && cursor<q.count) {
        struct mac_retail_tag failed;
        if(mac_retail_tag_get(cache,q.handles[cursor]&65535,&failed))
            fprintf(stderr,"native effect dependency %08lx group %08lx (%s): %s\n",
                (unsigned long)failed.handle,(unsigned long)failed.group,failed.name,
                q.error?q.error:"dependency closure failed");
    }
    free(q.handles);if(error)*error=ok?NULL:(q.error?q.error:"effect dependency closure");return ok;
}

struct object_effect_context {
    const struct mac_retail_cache *cache;
    const void *bitmaps,*sounds;size_t bitmap_bytes,sound_bytes;
    struct mac_actor_assets *assets;const char **error;
};
static int load_collision_effect_references(struct object_effect_context *context,uint32_t handle)
{
    struct mac_retail_collision_model *collision;
    unsigned i;int ok=0;
    collision=(struct mac_retail_collision_model *)malloc(sizeof(*collision));
    if(!collision){if(context->error)*context->error="collision effect staging allocation";return 0;}
    if(!mac_retail_collision_model_decode(context->cache,handle,collision,context->error)) {
        free(collision);return 0;
    }
    /* Collision resistance and region effects are outside the object tag's
       field graph. Resolve all of them before damage can select a variant. */
    for(i=0;i<8+collision->region_count;++i) {
        const struct mac_collision_effect *effect=i<8?
            &collision->resistance.effects[i]:&collision->regions[i-8].effect;
        if(effect->handle!=UINT32_MAX &&
           !mac_native_effect_assets_load(context->cache,context->bitmaps,context->bitmap_bytes,
               context->sounds,context->sound_bytes,context->assets,effect->handle,context->error))
            goto done;
    }
    ok=1;
done:
    mac_retail_collision_model_free(collision);free(collision);return ok;
}
static int load_object_effect_reference(void *opaque,uint32_t group,uint32_t handle)
{
    struct object_effect_context *c=opaque;
    switch(group) {
    case 0x636f6c6c:return load_collision_effect_references(c,handle);
    case 0x75646c67: {
        struct mac_native_object_tag dialogue;int ok;
        if(!mac_native_dialogue_decode(c->cache,handle,&dialogue,c->error))return 0;
        ok=mac_native_object_visit_references(&dialogue,load_object_effect_reference,c);
        mac_native_object_tag_free(&dialogue);return ok;
    }
    case 0x65666665:case 0x6a707421:case 0x666f6f74:case 0x736e6421:
    case 0x636f6e74:case 0x7063746c:case 0x70617274:case 0x64656361:case 0x6c696768:case 0x6c656e73:
    case 0x6d677332:case 0x7472616b:case 0x676c7721:case 0x656c6563:case 0x666c6167:
    /* an object's modifier shader (the shield's shimmer, a plasma shader),
       which render_objects.c looks up whenever the object is drawn */
    case 0x73706c61:
        if(!mac_native_effect_assets_load(c->cache,c->bitmaps,c->bitmap_bytes,
            c->sounds,c->sound_bytes,c->assets,handle,c->error)) {
            struct mac_retail_tag metadata;
            fprintf(stderr,"native object effect reference %08lx (%s) group %08lx: %s\n",(unsigned long)handle,
                mac_retail_tag_get(c->cache,handle&65535,&metadata)?metadata.name:"?",(unsigned long)group,
                c->error&&*c->error?*c->error:"no reason given");
            return 0;
        }
        return 1;
    default:
        /* Models, animation, HUD and other object resources have distinct
           owners. This walk resolves only effect/sound/damage dependencies. */
        return 1;
    }
}

int mac_native_object_effect_assets_load(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,uint32_t handle,const char **error)
{
    struct mac_retail_tag metadata;struct mac_native_object_tag tag;
    long i;int ok;
    struct object_effect_context context;
    if(!mac_retail_tag_get(cache,handle&65535,&metadata) || metadata.handle!=handle) {
        if(error)*error="object effect dependency handle";return 0;
    }
    if(metadata.group==UINT32_C(0x62697064))ok=mac_native_biped_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x77656170))ok=mac_native_weapon_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x65716970))ok=mac_native_equipment_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x67617262))ok=mac_native_garbage_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x70726f6a))ok=mac_native_projectile_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x76656869))ok=mac_native_vehicle_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x7363656e))ok=mac_native_scenery_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x73736365))ok=mac_native_sound_scenery_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x6c696669))ok=mac_native_light_fixture_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x6d616368))ok=mac_native_machine_decode(cache,handle,&tag,error);
    else if(metadata.group==UINT32_C(0x6374726c))ok=mac_native_control_decode(cache,handle,&tag,error);
    else {
        fprintf(stderr,"native object effect dependency %08lx (%s): group %08lx\n",
            (unsigned long)handle,metadata.name,(unsigned long)metadata.group);
        if(error)*error="object effect dependency family";return 0;
    }
    if(!ok)return 0;
    context.cache=cache;context.bitmaps=bitmaps;context.bitmap_bytes=bitmap_bytes;
    context.sounds=sounds;context.sound_bytes=sound_bytes;context.assets=assets;context.error=error;
    if(!mac_native_object_visit_references(&tag,load_object_effect_reference,&context))goto fail;
    if(metadata.group==UINT32_C(0x77656170)) {
        struct weapon_definition *weapon=tag.root;
        struct weapon_trigger_definition *triggers=weapon->weapon.triggers.address;
        for(i=0;i<weapon->weapon.triggers.count;++i)
            if(triggers[i].projectile.index!=NONE &&
               !mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,assets,
                   (uint32_t)triggers[i].projectile.index,error))goto fail;
    }
    mac_native_object_tag_free(&tag);return 1;
fail:
    mac_native_object_tag_free(&tag);return 0;
}
