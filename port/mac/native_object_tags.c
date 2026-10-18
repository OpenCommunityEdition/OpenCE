#include "cseries.h"
#include "units/biped_definitions.h"
#include "ai/actor_definitions.h"
#include "ai/actor_types.h"
#include "items/weapon_definitions.h"
#include "items/equipment_definitions.h"
#include "items/projectile_definitions.h"
#include "effects/contrail_definitions.h"
#include "cache/predicted_resources.h"
#include "native_object_tags.h"
#include "native_effect_layout.h"
#include "native_shader_layout.h"
#include "effects/particle_system_definitions.h"
#include "render/render_sprite.h"
#include "native_looping_sound_tags.h"
#include "native_sound_tags.h"
#include "engine_cache_files.h"
#include "sound/sound_definitions.h"
#include "sound/sound_environment_definitions.h"
#include "native_decal_tags.h"
#include "native_dialogue_tags.h"
#include "game/game_globals.h"
#include "objects/damage_effect_definitions.h"
#include "objects/object_types.h"
#include "devices/device_definitions.h"
#include "devices/device_controls.h"
#include "effects/material_effect_definitions.h"
#include "physics/breakable_surfaces.h"
#include "native_globals_layout.h"
#include "interface/hud_definitions.h"
#include "interface/unit_hud_interface_definition.h"
#include "interface/weapon_hud_interface_definition.h"
#include "native_hud_layout.h"
#include "native_hud_tags.h"
#include "native_hud_component_layout.h"
#include "native_grenade_hud_layout.h"
#include "native_lens_flare_layout.h"
#include "native_widget_layout.h"
#include "bitmaps/color_table_group.h"
#include "objects/widgets/light_volumes.h"
#include "native_camera_track_layout.h"
#include "native_vehicle_layout.h"
#include "native_tags.h"
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
/* Retail sound strides must match the native structures the generated schema
   walks. Keep these guards in production, not just the standalone probe. */
typedef char native_sound_root_size[sizeof(struct sound_definition)==0xa4?1:-1];
typedef char native_sound_range_size[sizeof(struct sound_pitch_range)==0x48?1:-1];
typedef char native_sound_permutation_size[sizeof(struct sound_permutation)==0x7c?1:-1];
typedef char native_sound_samples_offset[offsetof(struct sound_permutation,samples)==0x40?1:-1];
typedef char native_item_collection_size[sizeof(struct item_collection_definition)==0x5c?1:-1];
typedef char native_item_permutation_size[sizeof(struct item_permutation_definition)==0x54?1:-1];
typedef char native_item_reference_offset[offsetof(struct item_permutation_definition,item)==0x24?1:-1];
/* Structures absent from public engine headers, documented by the engine
   users and tag_schema_objects.c. Each is guarded by its known disk stride. */
struct native_unit_camera_track { struct tag_reference track; long unused[3]; };
struct native_unit_initial_weapon { struct tag_reference weapon; long unused[5]; };
struct native_unit_dialogue { short variant_number; short pad; long unused; struct tag_reference dialogue; };
/* actors.c's variant colours (TU-private there) */
struct actor_variant_change_colors { real_rgb_color color_lower_bound,color_upper_bound; unsigned long unused[2]; };
typedef char native_actor_variant_colors_size[sizeof(struct actor_variant_change_colors)==0x20?1:-1];
struct native_biped_contact { byte unused[32]; char marker_name[32]; };
struct native_weapon_firing_effect {
    short shot_count_lower_bound;short shot_count_upper_bound;long unused[8];
    struct tag_reference firing_effect;struct tag_reference misfire_effect;struct tag_reference empty_effect;
    struct tag_reference firing_damage;struct tag_reference misfire_damage;struct tag_reference empty_damage;
};
/* The decal definition is private to decals.c. Keep this full field layout
   local to the native cache decoder and guard its independently named offsets. */
struct native_decal_shader_definition {
    struct shader shader;
    word flags;
    short type;
    short framebuffer_blend_function;
    word pad02E;
    long unused030[5];
    struct tag_reference map;
    long unused054[5];
};
struct native_decal_definition {
    word flags;
    short type;
    short layer;
    word pad006;
    struct tag_reference next_decal_in_chain;
    real radius_lower_bound;
    real radius_upper_bound;
    long unused020[3];
    real intensity_lower_bound;
    real intensity_upper_bound;
    real_rgb_color color_lower_bound;
    real_rgb_color color_upper_bound;
    long unused04C[3];
    short animation_loop_frame_index;
    short animation_speed;
    long unused05C[7];
    real lifetime_lower_bound;
    real lifetime_upper_bound;
    real decay_time_lower_bound;
    real decay_time_upper_bound;
    long unused088[3];
    struct native_decal_shader_definition shader;
    real runtime_maximum_sprite_extent;
    word runtime_incremental_counter;
    word pad102;
    long unused104[2];
};
struct native_dialogue_definition {
    short vocalization_enum_version;
    word pad;
    long unused[3];
    struct tag_reference vocalizations[209];
    long unused_vocalizations[188];
};
typedef char native_biped_size_check[sizeof(struct biped_definition)==0x4f4?1:-1];
typedef char native_camera_track_size_check[sizeof(struct native_unit_camera_track)==28?1:-1];
typedef char native_initial_weapon_size_check[sizeof(struct native_unit_initial_weapon)==36?1:-1];
typedef char native_dialogue_size_check[sizeof(struct native_unit_dialogue)==24?1:-1];
typedef char native_contact_size_check[sizeof(struct native_biped_contact)==64?1:-1];
typedef char native_weapon_size_check[sizeof(struct weapon_definition)==0x508?1:-1];
typedef char native_weapon_firing_size_check[sizeof(struct native_weapon_firing_effect)==132?1:-1];
typedef char native_dialogue_index_check[offsetof(struct native_unit_dialogue,dialogue.index)==offsetof(struct unit_dialogue_variant,dialogue_index)?1:-1];

typedef char native_contrail_shader_size[sizeof(struct contrail_shader)==sizeof(struct shader_effect_definition)?1:-1];
typedef char native_projectile_size[sizeof(struct projectile_definition)==0x24c?1:-1];
typedef char native_decal_shader_size[sizeof(struct native_decal_shader_definition)==0x68?1:-1];
typedef char native_decal_size[sizeof(struct native_decal_definition)==0x10c?1:-1];
typedef char native_decal_shader_map_index[offsetof(struct native_decal_definition,shader.map.index)==0xe4?1:-1];
typedef char native_dialogue_size[sizeof(struct native_dialogue_definition)==0x1010?1:-1];
typedef char native_dialogue_vocalizations_offset[offsetof(struct native_dialogue_definition,vocalizations)==0x10?1:-1];
typedef char native_dialogue_unused_vocalizations_offset[offsetof(struct native_dialogue_definition,unused_vocalizations)==0xd20?1:-1];

typedef char native_damage_effect_size[sizeof(struct damage_effect_definition)==0x2a0?1:-1];
typedef char native_damage_effect_payload[offsetof(struct damage_effect_definition,damage)==0x1c4?1:-1];
typedef char native_damage_effect_sound[offsetof(struct damage_effect_definition,sound)==0x114?1:-1];
typedef char native_continuous_damage_size[sizeof(struct continuous_damage_effect_definition)==0x200?1:-1];

enum native_field_kind { N8,N16,N32,NREAL,NSTRING,NREF,NBLOCK,NSTRUCT,NSTRUCTARRAY,NTAGINDEX,NEMPTYBLOCK,NDATA,NFILEDATA };
struct native_schema;
struct native_field { size_t offset,size;enum native_field_kind kind;const struct native_schema *child;unsigned limit;const uint32_t *groups; };
struct native_schema { size_t size;const struct native_field *fields;unsigned count; };
#define FIELD(t,f,k) {offsetof(struct t,f),sizeof(((struct t *)0)->f),k,NULL,0,NULL}
#define CHILD(t,f,k,s,n) {offsetof(struct t,f),sizeof(((struct t *)0)->f),k,&schema_##s,n,NULL}
#define REF(t,f,g) {offsetof(struct t,f),sizeof(((struct t *)0)->f),NREF,NULL,0,g}
#define SCHEMA(t) static const struct native_schema schema_##t={sizeof(struct t),fields_##t,sizeof(fields_##t)/sizeof(fields_##t[0])}
#include "native_object_schema.inc"

struct native_allocation { void *data;struct native_allocation *next; };
struct native_owner { struct native_allocation *head;size_t bytes; };
static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t le16(const unsigned char *p) { return (uint16_t)(p[0]|(unsigned)p[1]<<8); }
static void *owned_alloc(struct native_owner *o,size_t size)
{
    struct native_allocation *entry;
    void *p;
    if(!size || size>16*1024*1024-sizeof(*entry) || o->bytes>16*1024*1024-size-sizeof(*entry))return NULL;
    entry=(struct native_allocation *)malloc(sizeof(*entry));if(!entry)return NULL;
    p=calloc(1,size);if(!p){free(entry);return NULL;}
    entry->data=p;entry->next=o->head;o->head=entry;o->bytes+=size+sizeof(*entry);return p;
}
void mac_native_object_owner_free(void *owner)
{
    struct native_owner *o=(struct native_owner *)owner;
    struct native_allocation *a,*next;
    if(!o)return;
    for(a=o->head;a;a=next){next=a->next;free(a->data);free(a);}free(o);
}
void mac_native_object_tag_free(struct mac_native_object_tag *tag)
{ if(tag){mac_native_object_owner_free(tag->owner);memset(tag,0,sizeof(*tag));} }
static int valid_handle(const struct mac_retail_cache *cache,uint32_t handle,uint32_t group)
{
    struct mac_retail_tag t;
    if(handle==UINT32_MAX)return 1;
    return mac_retail_tag_get(cache,handle&65535,&t) && t.handle==handle &&
        (!group || mac_native_tag_group_matches(t.group,group));
}
static int decode_reference(const struct mac_retail_cache *cache,struct native_owner *owner,
    const unsigned char *disk,struct tag_reference *out,const uint32_t *groups)
{
    uint32_t group=le32(disk),address=le32(disk+4),length=le32(disk+8),handle=le32(disk+12);
    const unsigned char *name,*end;
    size_t available,copy_size;
    if(!valid_handle(cache,handle,group) || length>255)return 0;
    if(handle!=UINT32_MAX && groups && groups[0]) {
        unsigned i;int found=0;
        for(i=0;groups[i];++i)if(valid_handle(cache,handle,groups[i]))found=1;
        if(!found)return 0;
    }
    out->group_tag=group;memcpy(&out->index,&handle,4);out->name_length=(long)length;out->name=NULL;
    if(address) {
        name=mac_retail_tag_span(cache,address,1);if(!name)return 0;
        available=cache->tag_size-(address-MAC_RETAIL_BASE);if(available>256)available=256;
        end=(const unsigned char *)memchr(name,0,available);if(!end)return 0;
        copy_size=(size_t)(end-name)+1;
        /* Cached references retain a name pointer with name_length zero. */
        if(length && copy_size!=length+1)return 0;
        out->name=(char *)owned_alloc(owner,copy_size);if(!out->name)return 0;
        memcpy(out->name,name,copy_size);
    } else if(length)return 0;
    return 1;
}
static int decode_struct(const struct mac_retail_cache *cache,struct native_owner *owner,
    const struct native_schema *schema,const unsigned char *disk,void *output,unsigned depth)
{
    unsigned i;size_t j;
    unsigned char *out=(unsigned char *)output;
    if(depth>16)return 0;
    memcpy(out,disk,schema->size);
    for(i=0;i<schema->count;++i) {
        const struct native_field *f=schema->fields+i;
        const unsigned char *src=disk+f->offset;
        unsigned char *dst=out+f->offset;
        uint32_t word;
        switch(f->kind) {
        case N8:break;
        case N16:
            for(j=0;j<f->size;j+=2){uint16_t half=le16(src+j);memcpy(dst+j,&half,2);}break;
        case N32:case NREAL:case NTAGINDEX:
            for(j=0;j<f->size;j+=4) {
                word=le32(src+j);
                if(f->kind==NREAL && (word&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))return 0;
                if(f->kind==NTAGINDEX && !valid_handle(cache,word,0))return 0;
                memcpy(dst+j,&word,4);
            }break;
        case NSTRING:
            if(!memchr(src,0,f->size))return 0;break;
        case NREF:
            for(j=0;j<f->size;j+=sizeof(struct tag_reference))
                if(!decode_reference(cache,owner,src+j,(struct tag_reference *)(dst+j),f->groups))return 0;break;
        case NSTRUCTARRAY:
            if(!f->child->size || f->size%f->child->size)return 0;
            for(j=0;j<f->size;j+=f->child->size)
                if(!decode_struct(cache,owner,f->child,src+j,dst+j,depth+1))return 0;
            break;
        case NSTRUCT:
            if(!decode_struct(cache,owner,f->child,src,dst,depth+1))return 0;break;
        case NEMPTYBLOCK:
            if(le32(src))return 0;
            memset(dst,0,sizeof(struct tag_block));break;
        case NDATA:case NFILEDATA:
            {
                struct tag_data *data=(struct tag_data *)dst;
                uint32_t bytes=le32(src),flags=le32(src+4),offset=le32(src+8);
                const unsigned char *payload;
                memset(data,0,sizeof(*data));
                if(bytes>f->limit || offset>UINT32_C(0x7fffffff))return 0;
                data->size=(long)bytes;data->pad=flags;data->file_offset=(long)offset;
                if(f->kind==NFILEDATA)break; /* wrapper validates/copies file payload */
                if(!bytes)break;
                payload=mac_retail_tag_span(cache,le32(src+12),bytes);
                if(!payload)return 0;
                data->address=owned_alloc(owner,bytes);if(!data->address)return 0;
                memcpy(data->address,payload,bytes);
            }break;
        case NBLOCK:
            {
                struct tag_block *block=(struct tag_block *)dst;
                uint32_t count=le32(src),address=le32(src+4);
                const unsigned char *data;
                block->count=0;block->address=NULL;block->definition=NULL;
                if(count>f->limit)return 0;
                if(!count)break;
                data=mac_retail_tag_span(cache,address,count*(uint32_t)f->child->size);
                if(!data)return 0;
                block->address=owned_alloc(owner,count*f->child->size);if(!block->address)return 0;
                block->count=(long)count;
                for(j=0;j<count;++j)if(!decode_struct(cache,owner,f->child,data+j*f->child->size,
                    (unsigned char *)block->address+j*f->child->size,depth+1))return 0;
            }break;
        }
    }
    return 1;
}
static void digest_word(uint32_t *hash,uint32_t value) { *hash=(*hash^value)*UINT32_C(16777619); }
static void digest_schema(const struct native_schema *schema,const void *native,uint32_t *hash,uint32_t *values)
{
    unsigned i;size_t j;uint32_t word;
    const unsigned char *data=(const unsigned char *)native;
    for(i=0;i<schema->count;++i) {
        const struct native_field *f=schema->fields+i;
        const unsigned char *p=data+f->offset;
        switch(f->kind) {
        case N8:for(j=0;j<f->size;++j){digest_word(hash,p[j]);++*values;}break;
        case N16:
            for(j=0;j<f->size;j+=2){uint16_t half;memcpy(&half,p+j,2);digest_word(hash,half);++*values;}break;
        case N32:case NREAL:case NTAGINDEX:
            for(j=0;j<f->size;j+=4){memcpy(&word,p+j,4);digest_word(hash,word);++*values;}break;
        case NSTRING:
            for(j=0;j<f->size;++j)digest_word(hash,p[j]);++*values;break;
        case NREF:
            {
                size_t ri;for(ri=0;ri<f->size;ri+=sizeof(struct tag_reference)) {
                const struct tag_reference *r=(const struct tag_reference *)(p+ri);
                digest_word(hash,(uint32_t)r->group_tag);digest_word(hash,(uint32_t)r->index);
                digest_word(hash,(uint32_t)r->name_length);*values+=3;
                if(r->name)for(j=0;r->name[j];++j)digest_word(hash,(unsigned char)r->name[j]);
                }
            }break;
        case NSTRUCTARRAY:
            for(j=0;j<f->size;j+=f->child->size)digest_schema(f->child,p+j,hash,values);break;
        case NSTRUCT:digest_schema(f->child,p,hash,values);break;
        case NEMPTYBLOCK:digest_word(hash,0);++*values;break;
        case NDATA:case NFILEDATA:
            {
                const struct tag_data *d=(const struct tag_data *)p;
                digest_word(hash,(uint32_t)d->size);digest_word(hash,(uint32_t)d->pad);
                digest_word(hash,(uint32_t)d->file_offset);*values+=3;
                if(d->address)for(j=0;j<(size_t)d->size;++j)digest_word(hash,((const unsigned char *)d->address)[j]);
            }break;
        case NBLOCK:
            {
                const struct tag_block *b=(const struct tag_block *)p;
                digest_word(hash,(uint32_t)b->count);++*values;
                for(j=0;j<(size_t)b->count;++j)digest_schema(f->child,(const unsigned char *)b->address+j*f->child->size,hash,values);
            }break;
        }
    }
}
static int decode_object(const struct mac_retail_cache *cache,uint32_t handle,uint32_t group,
    unsigned type,const struct native_schema *schema,struct mac_native_object_tag *out,const char **error)
{
    struct mac_native_object_tag result;struct native_owner *owner;struct mac_retail_tag t;
    const unsigned char *disk;
    memset(&result,0,sizeof(result));if(error)*error="native object tag";
    if(!out)return 0;memset(out,0,sizeof(*out));
    if(!cache || !mac_retail_tag_get(cache,handle&65535,&t) || t.handle!=handle || t.group!=group) {
        int known=cache && mac_retail_tag_get(cache,handle&65535,&t) && t.handle==handle;
        fprintf(stderr,"native object tag %08lx (%s): a %08lx tag, not %08lx\n",(unsigned long)handle,
            known&&t.name?t.name:"?",known?(unsigned long)t.group:0UL,(unsigned long)group);
        return 0;
    }
    disk=mac_retail_tag_span(cache,t.address,schema->size);if(!disk || (type!=UINT32_MAX && le16(disk)!=type)) {
        fprintf(stderr,"native object tag %08lx (%s): %s\n",(unsigned long)handle,t.name?t.name:"?",
            disk?"object type differs from its group":"root outside the map");
        return 0;
    }
    owner=(struct native_owner *)calloc(1,sizeof(*owner));if(!owner)return 0;owner->bytes=sizeof(*owner);
    result.owner=owner;result.root=owned_alloc(owner,schema->size);
    if(!result.root || !decode_struct(cache,owner,schema,disk,result.root,0)) {
        fprintf(stderr,"native object tag %08lx (%s) group %08lx: a field is outside its schema\n",
            (unsigned long)handle,t.name?t.name:"?",(unsigned long)group);
        mac_native_object_tag_free(&result);return 0;
    }
    result.handle=handle;result.group=t.group;result.root_bytes=schema->size;result.bytes_owned=owner->bytes;
    result.field_digest=UINT32_C(2166136261);
    digest_schema(schema,result.root,&result.field_digest,&result.converted_values);
    *out=result;if(error)*error=NULL;return 1;
}

int mac_native_scenery_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x7363656e),6,&schema_object_definition,out,error); }
int mac_native_sound_scenery_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x73736365),11,&schema_object_definition,out,error); }
/* Devices: a light fixture's definition is the device definition itself
   (tag_schema_objects.c), machines and controls add their own parts. */
int mac_native_machine_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x6d616368),7,&schema_machine_definition,out,error); }
int mac_native_control_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x6374726c),8,&schema_control_definition,out,error); }
int mac_native_light_fixture_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x6c696669),9,&schema_device_definition,out,error); }
int mac_native_biped_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x62697064),0,&schema_biped_definition,out,error); }
int mac_native_vehicle_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct vehicle_definition *vehicle;
    unsigned i;
    if(!decode_object(cache,handle,UINT32_C(0x76656869),1,&schema_vehicle_definition,out,error))return 0;
    vehicle=(struct vehicle_definition *)out->root;
    if(vehicle->unit.object.type!=_object_type_vehicle || vehicle->vehicle_type<0 || vehicle->vehicle_type>=7)goto invalid;
    for(i=0;i<4;++i)if(vehicle->function_modes[i]<0 || vehicle->function_modes[i]>=37)goto invalid;
    return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native vehicle type or function mode";return 0;
}
int mac_native_weapon_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct weapon_definition *w;long i,j;
    if(!decode_object(cache,handle,UINT32_C(0x77656170),2,&schema_weapon_definition,out,error))return 0;
    w=(struct weapon_definition *)out->root;
    for(i=0;i<w->weapon.magazines.count;++i) {
        struct weapon_magazine_definition *m=(struct weapon_magazine_definition *)w->weapon.magazines.address+i;
        if(m->rounds_recharged_per_second<0 || m->rounds_total_initial<0 || m->rounds_total_maximum<0 ||
           m->rounds_loaded_maximum<0 || m->rounds_reloaded<0 || m->reload_time<0 || m->chamber_time<0)goto invalid;
        for(j=0;j<m->ammunition_objects.count;++j)
            if(((struct weapon_ammunition_object *)m->ammunition_objects.address)[j].rounds<0)goto invalid;
    }
    for(i=0;i<w->weapon.triggers.count;++i) {
        struct weapon_trigger_definition *t=(struct weapon_trigger_definition *)w->weapon.triggers.address+i;
        if(t->magazine_index < -1 || t->magazine_index>=w->weapon.magazines.count ||
           t->rounds_per_shot<0 || t->minimum_rounds_loaded_per_shot<0 || t->rounds_between_tracers<0 ||
           t->projectiles_per_shot<0 || t->initial_rate_of_fire<0 || t->final_rate_of_fire<0)goto invalid;
    }
    return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native weapon magazine or trigger domain";return 0;
}

int mac_native_projectile_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x70726f6a),5,&schema_projectile_definition,out,error); }
int mac_native_contrail_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x636f6e74),UINT32_MAX,&schema_contrail_definition,out,error); }

static int native_decal_semantics(const struct native_decal_definition *decal)
{
    if(decal->type<0 || decal->type>=4 || decal->layer<0 || decal->layer>=5 ||
       decal->radius_lower_bound<0.f || decal->radius_upper_bound<decal->radius_lower_bound ||
       decal->intensity_upper_bound<decal->intensity_lower_bound ||
       decal->lifetime_lower_bound<0.f || decal->lifetime_upper_bound<decal->lifetime_lower_bound ||
       decal->decay_time_lower_bound<0.f || decal->decay_time_upper_bound<decal->decay_time_lower_bound ||
       !(decal->runtime_maximum_sprite_extent>0.f) ||
       decal->shader.map.index==NONE || decal->shader.map.group_tag!=UINT32_C(0x6269746d))
        return 0;
    return 1;
}

static int native_decal_chain_validate(const struct mac_retail_cache *cache,
    uint32_t first_handle,const char **error)
{
    unsigned char *visited;
    uint32_t handle=first_handle,steps=0;
    if(!cache || !cache->tag_count || cache->tag_count>65536)return 0;
    visited=(unsigned char *)calloc(cache->tag_count,1);
    if(!visited){if(error)*error="native decal chain allocation";return 0;}
    while(handle!=UINT32_MAX) {
        struct mac_native_object_tag current;
        struct native_decal_definition *decal;
        uint32_t index=handle&65535,next;
        const char *decode_error=NULL;
        if(index>=cache->tag_count || visited[index]) {
            free(visited);if(error)*error="native decal chain cycle";return 0;
        }
        if(steps++>=cache->tag_count ||
           !decode_object(cache,handle,UINT32_C(0x64656361),UINT32_MAX,
               &schema_native_decal_definition,&current,&decode_error)) {
            free(visited);if(error)*error="native decal chain tag/reference";return 0;
        }
        visited[index]=1;
        decal=(struct native_decal_definition *)current.root;
        if(!native_decal_semantics(decal)) {
            mac_native_object_tag_free(&current);free(visited);
            if(error)*error="native decal chain field domain";return 0;
        }
        next=(uint32_t)decal->next_decal_in_chain.index;
        mac_native_object_tag_free(&current);
        handle=next;
    }
    free(visited);
    if(error)*error=NULL;
    return 1;
}

int mac_native_decal_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct native_decal_definition *decal;
    if(!decode_object(cache,handle,UINT32_C(0x64656361),UINT32_MAX,
        &schema_native_decal_definition,out,error))return 0;
    decal=(struct native_decal_definition *)out->root;
    if(!native_decal_semantics(decal))goto invalid;
    if(!native_decal_chain_validate(cache,handle,error))goto invalid;
    return 1;
invalid:
    mac_native_object_tag_free(out);
    if(error && !*error)*error="native decal definition field domain";
    return 0;
}

/* These zeroed slots are the engine's enum gaps, not malformed tag
   references. tag_schema_objects.c documents and clears the same ranges. */
static int dialogue_padding_slot(unsigned index)
{
    static const unsigned char ranges[][2]={{3,5},{20,20},{25,28},{46,48},
        {77,79},{93,95},{106,107},{121,122},{144,147},{171,176},
        {184,187},{193,196},{205,208}};
    unsigned i;
    for(i=0;i<sizeof(ranges)/sizeof(ranges[0]);++i)
        if(index>=ranges[i][0] && index<=ranges[i][1])return 1;
    return 0;
}
static void dialogue_reference_none(unsigned char *reference)
{
    reference[0]='!';reference[1]='d';reference[2]='n';reference[3]='s';
    memset(reference+4,0,8);
    reference[12]=0xff;reference[13]=0xff;reference[14]=0xff;reference[15]=0xff;
}
static int dialogue_zero_bytes(const unsigned char *bytes,size_t size)
{
    size_t i;for(i=0;i<size;++i)if(bytes[i])return 0;return 1;
}
int mac_native_dialogue_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct mac_native_object_tag result;struct native_owner *owner;struct mac_retail_tag tag;
    unsigned char *normalized;const unsigned char *disk;unsigned i;
    if(error)*error="native dialogue tag";
    if(!out)return 0;
    memset(out,0,sizeof(*out));memset(&result,0,sizeof(result));
    if(!cache || !mac_retail_tag_get(cache,handle&65535,&tag) || tag.handle!=handle ||
       tag.group!=UINT32_C(0x75646c67))return 0;
    disk=mac_retail_tag_span(cache,tag.address,sizeof(struct native_dialogue_definition));
    if(!disk)return 0;
    /* The stock validator explicitly clears these enum gaps after checking
       their all-zero retail representation (tag_schema_objects.c). Reject
       unexpected data before converting them to native NONE references. */
    for(i=0;i<209;++i)if(dialogue_padding_slot(i) &&
        !dialogue_zero_bytes(disk+0x10+i*sizeof(struct tag_reference),sizeof(struct tag_reference))) {
        if(error)*error="native dialogue nonzero enum-gap reference";return 0;
    }
    if(!dialogue_zero_bytes(disk+0x10+209*sizeof(struct tag_reference),47*sizeof(struct tag_reference))) {
        if(error)*error="native dialogue nonzero unused references";return 0;
    }
    normalized=(unsigned char *)malloc(sizeof(struct native_dialogue_definition));
    if(!normalized)return 0;
    memcpy(normalized,disk,sizeof(struct native_dialogue_definition));
    for(i=0;i<209;++i)if(dialogue_padding_slot(i))
        dialogue_reference_none(normalized+0x10+i*sizeof(struct tag_reference));
    owner=(struct native_owner *)calloc(1,sizeof(*owner));
    if(!owner){free(normalized);return 0;}
    owner->bytes=sizeof(*owner);result.owner=owner;
    result.root=owned_alloc(owner,sizeof(struct native_dialogue_definition));
    if(!result.root || !decode_struct(cache,owner,&schema_native_dialogue_definition,
        normalized,result.root,0)) {
        free(normalized);mac_native_object_tag_free(&result);return 0;
    }
    free(normalized);
    result.handle=handle;result.group=tag.group;result.root_bytes=sizeof(struct native_dialogue_definition);
    result.bytes_owned=owner->bytes;result.field_digest=UINT32_C(2166136261);
    digest_schema(&schema_native_dialogue_definition,result.root,&result.field_digest,&result.converted_values);
    *out=result;if(error)*error=NULL;return 1;
}

static int visit_references(const struct native_schema *schema,const void *root,
    mac_native_reference_visitor visitor,void *context,unsigned depth)
{
    unsigned i;
    long j;
    const unsigned char *data=(const unsigned char *)root;
    if(depth>16)return 0;
    for(i=0;i<schema->count;++i) {
        const struct native_field *field=schema->fields+i;
        const void *value=data+field->offset;
        if(field->kind==NREF) {
            size_t ri;for(ri=0;ri<field->size;ri+=sizeof(struct tag_reference)) {
            const struct tag_reference *ref=(const struct tag_reference *)((const unsigned char *)value+ri);
            if(ref->index!=NONE && !visitor(context,(uint32_t)ref->group_tag,(uint32_t)ref->index))return 0;
            }
        } else if(field->kind==NTAGINDEX) {
            uint32_t handle;
            memcpy(&handle,value,4);
            if(handle!=UINT32_MAX && !visitor(context,0,handle))return 0;
        } else if(field->kind==NSTRUCTARRAY) {
            size_t offset;
            if(!field->child->size || field->size%field->child->size)return 0;
            for(offset=0;offset<field->size;offset+=field->child->size)
                if(!visit_references(field->child,(const unsigned char *)value+offset,visitor,context,depth+1))return 0;
        } else if(field->kind==NSTRUCT) {
            if(!visit_references(field->child,value,visitor,context,depth+1))return 0;
        } else if(field->kind==NBLOCK) {
            const struct tag_block *block=(const struct tag_block *)value;
            if(block->count<0 || (unsigned long)block->count>field->limit ||
                (block->count && !block->address))return 0;
            for(j=0;j<block->count;++j)
                if(!visit_references(field->child,(const unsigned char *)block->address+j*field->child->size,
                    visitor,context,depth+1))return 0;
        }
    }
    return 1;
}
int mac_native_object_visit_references(const struct mac_native_object_tag *tag,
    mac_native_reference_visitor visitor,void *context)
{
    const struct native_schema *schema;
    if(!tag || !tag->root || !visitor)return 0;
    /* HUD message text contains data and scalar blocks, but no tag references. */
    if(tag->group==UINT32_C(0x686d7420))return tag->root_bytes==0x80;
    if(tag->group==UINT32_C(0x62697064))schema=&schema_biped_definition;
    else if(tag->group==UINT32_C(0x76656869))schema=&schema_vehicle_definition;
    else if(tag->group==UINT32_C(0x7363656e) || tag->group==UINT32_C(0x73736365))schema=&schema_object_definition;
    else if(tag->group==UINT32_C(0x6c696669))schema=&schema_device_definition;
    else if(tag->group==UINT32_C(0x6d616368))schema=&schema_machine_definition;
    else if(tag->group==UINT32_C(0x6374726c))schema=&schema_control_definition;
    else if(tag->group==UINT32_C(0x77656170))schema=&schema_weapon_definition;
    else if(tag->group==UINT32_C(0x70726f6a))schema=&schema_projectile_definition;
    else if(tag->group==UINT32_C(0x636f6e74))schema=&schema_contrail_definition;
    else if(tag->group==UINT32_C(0x6d617467))schema=&schema_game_globals;
    else if(tag->group==UINT32_C(0x68756467))schema=&schema_hud_globals_definition;
    else if(tag->group==UINT32_C(0x68756423))schema=&schema_hud_number_definition;
    else if(tag->group==UINT32_C(0x756e6869))schema=&schema_unit_hud_interface_definition;
    else if(tag->group==UINT32_C(0x77706869))schema=&schema_weapon_hud_interface_definition;
    else if(tag->group==UINT32_C(0x67726869))schema=&schema_grenade_hud_interface_definition;
    else if(tag->group==UINT32_C(0x6c656e73))schema=&schema_lens_flare_definition;
    else if(tag->group==UINT32_C(0x6d677332))schema=&schema_light_volume_definition;
    else if(tag->group==UINT32_C(0x676c7721))schema=&schema_glow_definition;
    else if(tag->group==UINT32_C(0x656c6563))schema=&schema_lightning_definition;
    else if(tag->group==UINT32_C(0x666c6167))schema=&schema_flag_definition;
    else if(tag->group==UINT32_C(0x7472616b))schema=&schema_native_camera_track_definition;
    else if(tag->group==UINT32_C(0x64656361))schema=&schema_native_decal_definition;
    else if(tag->group==UINT32_C(0x70617274))schema=&schema_particle_definition;
    else if(tag->group==UINT32_C(0x666f6f74))schema=&schema_material_effects_definition;
    else if(tag->group==UINT32_C(0x6a707421))schema=&schema_damage_effect_definition;
    else if(tag->group==UINT32_C(0x63646d67))schema=&schema_continuous_damage_effect_definition;
    else if(tag->group==UINT32_C(0x73656e76))schema=&schema_shader_environment_definition;
    else if(tag->group==UINT32_C(0x736f736f))schema=&schema_shader_model_definition;
    else if(tag->group==UINT32_C(0x73676c61))schema=&schema_shader_transparent_glass_definition;
    else if(tag->group==UINT32_C(0x73776174))schema=&schema_shader_transparent_water_definition;
    else if(tag->group==UINT32_C(0x73706c61))schema=&schema_shader_transparent_plasma_definition;
    else if(tag->group==UINT32_C(0x666f6720))schema=&schema_fog_definition_full;
    else if(tag->group==UINT32_C(0x736d6574))schema=&schema_shader_transparent_meter_definition;
    else if(tag->group==UINT32_C(0x73636869))schema=&schema_shader_transparent_chicago_definition;
    else if(tag->group==UINT32_C(0x73636578))schema=&schema_shader_transparent_chicago_extended_definition;
    else if(tag->group==UINT32_C(0x65666665))schema=&schema_effect_definition;
    else if(tag->group==UINT32_C(0x7063746c))schema=&schema_particle_system_definition;
    else if(tag->group==UINT32_C(0x65716970))schema=&schema_equipment_definition;
    else if(tag->group==UINT32_C(0x67617262))schema=&schema_item_definition;
    else if(tag->group==UINT32_C(0x69746d63))schema=&schema_item_collection_definition;
    else if(tag->group==UINT32_C(0x736e6465))schema=&schema_sound_environment_definition;
    else if(tag->group==UINT32_C(0x736e6421))schema=&schema_sound_definition;
    else if(tag->group==UINT32_C(0x6c736e64))schema=&schema_looping_sound_definition;
    else if(tag->group==UINT32_C(0x75646c67))schema=&schema_native_dialogue_definition;
    else if(tag->group==UINT32_C(0x61637472))schema=&schema_actor_definition;
    else if(tag->group==UINT32_C(0x61637476))schema=&schema_actor_variant_definition;
    else return 0;
    if(tag->root_bytes!=schema->size)return 0;
    return visit_references(schema,tag->root,visitor,context,0);
}

int mac_native_globals_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct game_globals *g;
    if(!decode_object(cache,handle,UINT32_C(0x6d617467),UINT32_MAX,&schema_game_globals,out,error))return 0;
    g=(struct game_globals *)out->root;
    if(g->player_control.count && ((struct game_globals_player_control *)g->player_control.address)->look_function.count &&
       g->camera.count && g->player_information.count && g->first_person_interface.count &&
       g->rasterizer_data.count && g->interface_tag_references.count)return 1;
    mac_native_object_tag_free(out);if(error)*error="native globals missing required singleton or look function";return 0;
}

int mac_native_hud_globals_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct hud_globals_definition *hud;
    if(!decode_object(cache,handle,UINT32_C(0x68756467),UINT32_MAX,&schema_hud_globals_definition,out,error))return 0;
    hud=(struct hud_globals_definition *)out->root;
    if(hud->messaging.absolute_placement.corner>=0 && hud->messaging.absolute_placement.corner<NUMBER_OF_HUD_ANCHORS)return 1;
    mac_native_object_tag_free(out);if(error)*error="native HUD anchor";return 0;
}

int mac_native_hud_number_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x68756423),UINT32_MAX,&schema_hud_number_definition,out,error); }
int mac_native_unit_hud_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x756e6869),UINT32_MAX,&schema_unit_hud_interface_definition,out,error); }
int mac_native_weapon_hud_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x77706869),UINT32_MAX,&schema_weapon_hud_interface_definition,out,error); }
int mac_native_grenade_hud_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct grenade_hud_interface_definition *hud;
    if(!decode_object(cache,handle,UINT32_C(0x67726869),UINT32_MAX,
        &schema_grenade_hud_interface_definition,out,error))return 0;
    hud=out->root;
    if(hud->absolute_placement.corner>=0 && hud->absolute_placement.corner<NUMBER_OF_HUD_ANCHORS)return 1;
    mac_native_object_tag_free(out);if(error)*error="native grenade HUD anchor";return 0;
}
int mac_native_color_table_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x636f6c6f),UINT32_MAX,&schema_color_table_definition,out,error); }
int mac_native_lens_flare_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x6c656e73),UINT32_MAX,&schema_lens_flare_definition,out,error); }
int mac_native_light_volume_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x6d677332),UINT32_MAX,&schema_light_volume_definition,out,error); }
int mac_native_glow_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x676c7721),UINT32_MAX,&schema_glow_definition,out,error); }
int mac_native_lightning_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x656c6563),UINT32_MAX,&schema_lightning_definition,out,error); }
int mac_native_flag_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x666c6167),UINT32_MAX,&schema_flag_definition,out,error); }
int mac_native_camera_track_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x7472616b),UINT32_MAX,&schema_native_camera_track_definition,out,error); }

int mac_native_looping_sound_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct looping_sound_definition *sound;
    if(!decode_object(cache,handle,UINT32_C(0x6c736e64),UINT32_MAX,
        &schema_looping_sound_definition,out,error))return 0;
    sound=(struct looping_sound_definition *)out->root;
    /* Runtime script ownership never survives a cache load (same reset as
       tag_schema_effects.c). Keep the digest describing the native result. */
    sound->runtime_scripting_sound_index=NONE;
    out->field_digest=UINT32_C(2166136261);out->converted_values=0;
    digest_schema(&schema_looping_sound_definition,out->root,&out->field_digest,&out->converted_values);
    return 1;
}

/* port: samples the sound manager would not play (Ogg Vorbis, uncompressed,
   mono at 44 kHz) are made Xbox ADPCM when the cache is read, as
   custom_edition_sounds.c does at load on Linux. The hook, set by
   mac_sound_convert_install (port/mac/mac_sound_convert.c) in the session
   program, registers a permutation and returns its offset in the cache's
   converted space and its size there, or 0. Without it nothing changes. */
uint32_t (*mac_native_sound_convert_hook)(const unsigned char *samples,size_t bytes,
    int compression,int channels,int source_rate,int target_rate,uint32_t group,uint32_t *converted_bytes)=NULL;

/* This owns sample bytes, not a streaming cache or DirectSound resource. The
   cache consumer must still choose/convert the compression and own playback. */
int mac_native_sound_decode(const struct mac_retail_cache *cache,uint32_t handle,
    const void *external,size_t external_bytes,struct mac_native_object_tag *out,const char **error)
{
    struct sound_definition *sound;long i,j;
    struct native_owner *owner;
    if(!decode_object(cache,handle,UINT32_C(0x736e6421),UINT32_MAX,&schema_sound_definition,out,error))return 0;
    sound=(struct sound_definition *)out->root;owner=(struct native_owner *)out->owner;
    if(sound->sound_class<0||sound->sound_class>=51||sound->sample_rate<0||sound->sample_rate>=2||
       sound->encoding<0||sound->encoding>=2||sound->compression<0||sound->compression>=4)goto invalid;
    if(sound->promotion_sound.index!=NONE &&
       (sound->promotion_count<0||sound->longest_permutation_length<0||
        (sound->longest_permutation_length && (unsigned long)sound->promotion_count>
          UINT32_C(0x7fffffff)/(unsigned long)sound->longest_permutation_length)))goto invalid;
    sound->promotion_counter=0;sound->promotion_time=0;
    sound->scripting_time=UINT32_MAX;sound->scripting_sound_index=NONE;
    {   /* port: whether permutations are converted, and whether any was or could not be */
    int convert_any=0,convert_failed=0;
    for(i=0;i<sound->pitch_ranges.count;++i) {
        struct sound_pitch_range *range=(struct sound_pitch_range *)sound->pitch_ranges.address+i;
        if((range->permutations.count && range->actual_permutation_count<1)||range->actual_permutation_count<0||range->actual_permutation_count>32||
           range->actual_permutation_count>range->permutations.count)goto invalid;
        range->played_permutation_mask=0;range->previous_permutation_index=NONE;range->forced_permutation_index=NONE;
        for(j=0;j<range->permutations.count;++j) {
            struct sound_permutation *perm=(struct sound_permutation *)range->permutations.address+j;
            struct tag_data *data=&perm->samples;
            const unsigned char *file=cache->file;size_t bytes=cache->file_size;
            if(perm->compression<0||perm->compression>=4||perm->next_permutation_index<NONE||
               perm->next_permutation_index>=range->permutations.count)goto invalid;
            perm->unknown0=NONE;perm->unknown1=0;perm->unknown2=handle;perm->unknown3=handle;
            if(data->pad&1){file=(const unsigned char *)external;bytes=external_bytes;}
            if(data->pad&~1UL)goto invalid;
            if(bytes>((data->pad&1)?MAC_CACHE_OFFSET_LIMIT-MAC_CACHE_SOUND_BASE:MAC_CACHE_DECODED_BASE))goto invalid;
            if((size_t)data->file_offset>bytes||(size_t)data->size>bytes-(size_t)data->file_offset)goto invalid;
            if(mac_native_sound_convert_hook && data->size &&
               (perm->compression==3 || perm->compression==0 ||
                (sound->encoding==0 && sound->sample_rate!=0))) {
                uint32_t converted_size=0,converted_offset;
                converted_offset=mac_native_sound_convert_hook(file?file+data->file_offset:NULL,(size_t)data->size,
                    perm->compression,sound->encoding==0?1:2,sound->sample_rate?44100:22050,
                    sound->encoding==0?22050:(sound->sample_rate?44100:22050),handle,&converted_size);
                if(converted_offset) {
                    perm->compression=1;perm->sample_buffer_size=0;
                    data->file_offset=(long)converted_offset;data->size=(long)converted_size;
                    data->pad=0;data->address=NULL;convert_any=1;continue;
                }
                convert_failed=1;
            }
            if(data->size) {
                if(!file)goto invalid;
                data->address=owned_alloc(owner,(size_t)data->size);if(!data->address)goto invalid;
                memcpy(data->address,file+data->file_offset,(size_t)data->size);
            }
            if(data->pad&1)data->file_offset+=(long)MAC_CACHE_SOUND_BASE;
        }
    }
    if(mac_native_sound_convert_hook && (convert_any || convert_failed || sound->compression!=1)) {
        sound->compression=1;
        if(sound->encoding==0)sound->sample_rate=0;
        /* with no pitch ranges the game neither plays nor loads the sound */
        if(convert_failed)sound->pitch_ranges.count=0;
    }
    }
    out->bytes_owned=owner->bytes;out->field_digest=UINT32_C(2166136261);out->converted_values=0;
    digest_schema(&schema_sound_definition,out->root,&out->field_digest,&out->converted_values);
    return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native sound metadata or sample range";return 0;
}

int mac_native_sound_environment_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    return decode_object(cache,handle,UINT32_C(0x736e6465),UINT32_MAX,
        &schema_sound_environment_definition,out,error);
}

int mac_native_item_collection_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct item_collection_definition *definition;
    struct item_permutation_definition *permutations;
    long i,total=0;
    if(!decode_object(cache,handle,UINT32_C(0x69746d63),UINT32_MAX,
        &schema_item_collection_definition,out,error))return 0;
    definition=out->root;permutations=definition->permutations.address;
    /* game_engine.c accumulates each weight into a long, then narrows to a
       signed short for seed_random_range. Bound before either conversion. */
    for(i=0;i<definition->permutations.count;++i) {
        real weight=permutations[i].weight;
        if(weight<0 || weight>(real)(32767-total) || permutations[i].item.index==NONE)goto invalid;
        total=(long)((real)total+weight);
    }
    if(total<=0)goto invalid;
    return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native item collection weights or empty reference";return 0;
}

/* Garbage: effects' debris. The definition is an item and nothing else the
   game reads (tag_schema_objects.c). */
int mac_native_garbage_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{ return decode_object(cache,handle,UINT32_C(0x67617262),4,&schema_item_definition,out,error); }
/* Actors and their variants (ai/actor_definitions.h). The actor holds only
   scalars; the variant names its actor, unit, weapon and equipment. */
int mac_native_actor_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct actor_definition *actor;
    if(!decode_object(cache,handle,UINT32_C(0x61637472),UINT32_MAX,&schema_actor_definition,out,error))return 0;
    actor=(struct actor_definition *)out->root;
    if(actor->type<0 || actor->type>=NUMBER_OF_ACTOR_TYPES) {
        mac_native_object_tag_free(out);if(error)*error="native actor type";return 0;
    }
    return 1;
}
int mac_native_actor_variant_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    return decode_object(cache,handle,UINT32_C(0x61637476),UINT32_MAX,&schema_actor_variant_definition,out,error);
}
int mac_native_equipment_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct equipment_definition *definition;
    if(!decode_object(cache,handle,UINT32_C(0x65716970),3,
        &schema_equipment_definition,out,error))return 0;
    definition=out->root;
    if(definition->equipment.powerup_type<0 ||
       definition->equipment.powerup_type>=NUMBER_OF_EQUIPMENT_POWERUP_TYPES ||
       definition->equipment.grenade_type<0 || definition->equipment.grenade_type>=2) {
        mac_native_object_tag_free(out);if(error)*error="native equipment type";return 0;
    }
    return 1;
}

int mac_native_effect_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    static const uint32_t classes[]={0x7063746c,0x736e6421,0x6f626a65,0x64656361,0x6a707421,0x6c696768};
    struct effect_definition *definition;
    struct effect_event_definition *events;
    long i,j;
    if(!decode_object(cache,handle,UINT32_C(0x65666665),UINT32_MAX,&schema_effect_definition,out,error))return 0;
    definition=out->root;
    if(definition->loop_start_index<NONE || definition->loop_start_index>=definition->events.count ||
       definition->loop_stop_index<NONE || definition->loop_stop_index>=definition->events.count)goto invalid;
    events=definition->events.address;
    for(i=0;i<definition->events.count;++i) {
        struct effect_part_definition *parts=events[i].parts.address;
        for(j=0;j<events[i].parts.count;++j) {
            struct mac_retail_tag target;
            unsigned k;
            if(parts[j].reference.index==NONE)continue;
            if(!mac_retail_tag_get(cache,(uint32_t)parts[j].reference.index&65535,&target) ||
               target.handle!=(uint32_t)parts[j].reference.index)goto invalid;
            for(k=0;k<sizeof(classes)/sizeof(classes[0]);++k)
                if(target.group==classes[k] || target.parents[0]==classes[k] || target.parents[1]==classes[k])break;
            if(k==sizeof(classes)/sizeof(classes[0]))goto invalid;
            /* Match the shared validator: dispatch follows actual tag ancestry. */
            parts[j].runtime_base_class_tag=classes[k];
        }
    }
    return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native effect loop or part class";return 0;
}
int mac_native_particle_system_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct particle_system_definition *system;
    struct particle_system_type *types;long i,j;
    if(!decode_object(cache,handle,UINT32_C(0x7063746c),UINT32_MAX,&schema_particle_system_definition,out,error))return 0;
    system=out->root;types=system->types.address;
    if(system->system_update_physics<0 || system->system_update_physics>=NUMBER_OF_PARTICLE_SYSTEM_UPDATE_PHYSICS)goto invalid;
    for(i=0;i<system->types.count;++i) {
        struct particle_system_type_state *states=types[i].type_states.address;
        if(types[i].initial_particle_creation_physics<0 ||
           types[i].initial_particle_creation_physics>=NUMBER_OF_PARTICLE_SYSTEM_TYPE_CREATION_PHYSICS ||
           types[i].complex_sprite_render_mode<0 ||
           types[i].complex_sprite_render_mode>=NUMBER_OF_PARTICLE_SYSTEM_TYPE_COMPLEX_SPRITE_RENDER_MODES ||
           types[i].sprite_render_mode<0 || types[i].sprite_render_mode>=NUMBER_OF_BUILD_SPRITE_ORIENTATIONS)goto invalid;
        for(j=0;j<types[i].type_states.count;++j)
            if(states[j].particle_creation_physics<0 ||
               states[j].particle_creation_physics>=NUMBER_OF_PARTICLE_SYSTEM_TYPE_CREATION_PHYSICS ||
               states[j].particle_update_physics<0 ||
               states[j].particle_update_physics>=NUMBER_OF_PARTICLE_SYSTEM_TYPE_UPDATE_PHYSICS)goto invalid;
    }
    return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native particle system function or render index";return 0;
}

int mac_native_particle_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct particle_definition *particle;
    if(!decode_object(cache,handle,UINT32_C(0x70617274),UINT32_MAX,&schema_particle_definition,out,error))return 0;
    particle=out->root;
    if(particle->sprite_orientation<0 || particle->sprite_orientation>=NUMBER_OF_BUILD_SPRITE_ORIENTATIONS) {
        mac_native_object_tag_free(out);if(error)*error="native particle sprite orientation";return 0;
    }
    /* Apply the shared render validator's radius/rate corrections. */
    if(particle->radius_lower_bound<0.f)particle->radius_lower_bound=0.f;
    if(particle->radius_upper_bound<0.f)particle->radius_upper_bound=0.f;
    if(particle->frames_per_second_lower_bound<0.f)particle->frames_per_second_lower_bound=0.f;
    if(particle->frames_per_second_upper_bound<0.f)particle->frames_per_second_upper_bound=0.f;
    if(particle->frames_per_second_lower_bound>1000.f)particle->frames_per_second_lower_bound=1000.f;
    if(particle->frames_per_second_upper_bound>1000.f)particle->frames_per_second_upper_bound=1000.f;
    if(particle->frames_per_second_contact_deterioration<0.f)particle->frames_per_second_contact_deterioration=0.f;
    return 1;
}

int mac_native_environment_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct shader_environment_definition *shader;
    if(!decode_object(cache,handle,UINT32_C(0x73656e76),UINT32_MAX,&schema_shader_environment_definition,out,error))return 0;
    shader=out->root;
    /* Bounds witnessed by tag_schema_render.c and the rasterizer's tables. */
    if(shader->type<0 || shader->type>=3 || shader->detail_map_function<0 || shader->detail_map_function>=3 ||
       shader->micro_detail_map_function<0 || shader->micro_detail_map_function>=3 ||
       shader->reflection_type<0 || shader->reflection_type>=3 ||
       shader->u_animation_function<0 || shader->u_animation_function>=12 ||
       shader->v_animation_function<0 || shader->v_animation_function>=12 ||
       shader->primary_animation_function<0 || shader->primary_animation_function>=12 ||
       shader->secondary_animation_function<0 || shader->secondary_animation_function>=12 ||
       shader->plasma_animation_function<0 || shader->plasma_animation_function>=12) {
        mac_native_object_tag_free(out);if(error)*error="native environment shader enum";return 0;
    }
    /* Like shader_type_check, derive this runtime discriminator from the tag. */
    shader->shader.base.type=3;
    return 1;
}

static int chicago_maps_valid(const struct tag_block *block)
{
    const struct shader_transparent_chicago_map *maps=block->address;long i;
    for(i=0;i<block->count;++i) {
        const struct shader_texture_animation *a=&maps[i].map_animation;
        if(maps[i].color_function<0 || maps[i].color_function>=13 ||
           maps[i].alpha_function<0 || maps[i].alpha_function>=13 ||
           a->u_source<0 || a->u_source>=5 || a->v_source<0 || a->v_source>=5 ||
           a->r_source<0 || a->r_source>=5 || a->u_function<0 || a->u_function>=12 ||
           a->v_function<0 || a->v_function>=12 || a->r_function<0 || a->r_function>=12)return 0;
    }
    return 1;
}
int mac_native_chicago_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct mac_retail_tag metadata;struct shader_transparent_chicago_definition *shader;
    struct shader_transparent *t;const struct native_schema *schema;
    if(!cache || !mac_retail_tag_get(cache,handle&65535,&metadata) || metadata.handle!=handle ||
       (metadata.group!=UINT32_C(0x73636869) && metadata.group!=UINT32_C(0x73636578))) {
        if(error)*error="native Chicago shader tag";return 0;
    }
    schema=metadata.group==UINT32_C(0x73636869)?&schema_shader_transparent_chicago_definition:
        &schema_shader_transparent_chicago_extended_definition;
    if(!decode_object(cache,handle,metadata.group,UINT32_MAX,schema,out,error))return 0;
    shader=out->root;t=&shader->transparent;
    if(t->first_map_type<0 || t->first_map_type>=4 ||
       t->framebuffer_blend_function<0 || t->framebuffer_blend_function>=8 ||
       t->framebuffer_fade_mode<0 || t->framebuffer_fade_mode>=3 ||
       t->framebuffer_fade_source<0 || t->framebuffer_fade_source>=5 || !chicago_maps_valid(&t->maps))goto invalid;
    if(metadata.group==UINT32_C(0x73636578) &&
       !chicago_maps_valid(&((struct shader_transparent_chicago_extended_definition *)shader)->two_stage_maps))goto invalid;
    /* Lighting reads only this discriminator. Preserve both map blocks here;
       renderer preparation must apply the shared validator's extended-layout
       normalization and layer/numeric dependency checks before drawing. */
    shader->shader.base.type=6;return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native Chicago shader enum";return 0;
}

int mac_native_model_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct shader_model_definition *shader;
    if(!decode_object(cache,handle,UINT32_C(0x736f736f),UINT32_MAX,
        &schema_shader_model_definition,out,error))return 0;
    shader=out->root;
    if(shader->self_illumination_animation_function<0 ||
       shader->self_illumination_animation_function>=12 ||
       shader->detail_function<0 || shader->detail_function>=3 ||
       shader->detail_mask<0 || shader->detail_mask>=9) {
        mac_native_object_tag_free(out);if(error)*error="native model shader enum";return 0;
    }
    /* shader_model_check in tag_schema_render.c derives this discriminator. */
    shader->shader.base.type=4;
    return 1;
}

int mac_native_glass_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct shader_transparent_glass_definition *shader;
    if(!decode_object(cache,handle,UINT32_C(0x73676c61),UINT32_MAX,
        &schema_shader_transparent_glass_definition,out,error))return 0;
    shader=out->root;
    if(shader->reflection_type<0 || shader->reflection_type>=3) {
        mac_native_object_tag_free(out);if(error)*error="native glass shader enum";return 0;
    }
    /* shader_transparent_glass_check uses the transparent-glass type. */
    shader->shader.base.type=8;
    return 1;
}

int mac_native_water_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct shader_transparent_water_definition *shader;
    if(!decode_object(cache,handle,UINT32_C(0x73776174),UINT32_MAX,
        &schema_shader_transparent_water_definition,out,error))return 0;
    shader=out->root;
    /* (rasterizer_transparent_geometry.c's water discriminator) */
    shader->shader.base.type=7;
    return 1;
}

int mac_native_plasma_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct shader_transparent_plasma_definition *shader;
    if(!decode_object(cache,handle,UINT32_C(0x73706c61),UINT32_MAX,
        &schema_shader_transparent_plasma_definition,out,error))return 0;
    shader=out->root;
    /* (shaders.c's plasma discriminator; the renderer checks the function
       sources itself, rasterizer_xbox_plasma_energy.c) */
    shader->shader.base.type=10;
    return 1;
}

int mac_native_fog_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct fog_definition_full *fog;
    if(!decode_object(cache,handle,UINT32_C(0x666f6720),UINT32_MAX,
        &schema_fog_definition_full,out,error))return 0;
    fog=out->root;
    /* (the planar fog's density, distance and depth, and the screen's
       layers, which the renderer divides by and counts with) */
    if(fog->maximum_density<0.f || fog->maximum_distance<0.f || fog->maximum_depth<0.f ||
       fog->screen.layer_count<0 || fog->screen.layer_count>4) {
        mac_native_object_tag_free(out);if(error)*error="native fog values";return 0;
    }
    return 1;
}

int mac_native_meter_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct shader_transparent_meter_definition *shader;
    if(!decode_object(cache,handle,UINT32_C(0x736d6574),UINT32_MAX,
        &schema_shader_transparent_meter_definition,out,error))return 0;
    shader=out->root;
    /* shader_transparent_meter_check derives the meter discriminator. */
    shader->shader.base.type=9;
    return 1;
}

static int native_damage_valid(const struct damage_definition *damage)
{
    return damage->side_effect>=0 && damage->side_effect<4 && damage->category>=0 && damage->category<13;
}
int mac_native_damage_effect_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct damage_effect_definition *d;unsigned i;
    if(!decode_object(cache,handle,UINT32_C(0x6a707421),UINT32_MAX,&schema_damage_effect_definition,out,error))return 0;
    d=out->root;
    if(!native_damage_valid(&d->damage) || d->screen_flash.type<0 || d->screen_flash.type>=7 ||
       d->screen_flash.priority<0 || d->screen_flash.priority>=3 ||
       d->screen_flash.fade_function<0 || d->screen_flash.fade_function>=6 ||
       d->camera_impulse.temporary_transition<0 || d->camera_impulse.temporary_transition>=6 ||
       d->camera_shake.falloff_transition_function<0 || d->camera_shake.falloff_transition_function>=6 ||
       d->camera_shake.periodic_function<0 || d->camera_shake.periodic_function>=12)goto invalid;
    for(i=0;i<2;++i)if(d->vibrate.vibrate_frequencies[i].fade_function<0 ||
        d->vibrate.vibrate_frequencies[i].fade_function>=6)goto invalid;
    return 1;
invalid:
    mac_native_object_tag_free(out);if(error)*error="native damage effect enum";return 0;
}
int mac_native_continuous_damage_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    struct continuous_damage_effect_definition *d;
    if(!decode_object(cache,handle,UINT32_C(0x63646d67),UINT32_MAX,&schema_continuous_damage_effect_definition,out,error))return 0;
    d=out->root;
    if(!native_damage_valid(&d->damage) || d->camera_shake.periodic_function<0 || d->camera_shake.periodic_function>=12) {
        mac_native_object_tag_free(out);if(error)*error="native continuous damage enum";return 0;
    }
    return 1;
}

int mac_native_material_effects_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error)
{
    return decode_object(cache,handle,UINT32_C(0x666f6f74),UINT32_MAX,
        &schema_material_effects_definition,out,error);
}
