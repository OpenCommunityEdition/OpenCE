#include <stdio.h>
#include "native_scenario_blocks.h"
#include "cseries.h"
#include "devices/device_light_fixtures.h"
#include "devices/device_machines.h"
/* the device records written below, by the game's own layouts */
typedef char device_record_offsets_witness[
    (offsetof(struct scenario_light_fixture_datum,device)==40 &&
     offsetof(struct scenario_light_fixture_datum,color)==48 &&
     offsetof(struct scenario_light_fixture_datum,intensity)==60 &&
     offsetof(struct scenario_light_fixture_datum,falloff_angle)==64 &&
     offsetof(struct scenario_light_fixture_datum,cutoff_angle)==68 &&
     sizeof(struct scenario_light_fixture_datum)<=0x58 &&
     offsetof(struct scenario_machine_datum,flags)==48 &&
     sizeof(struct scenario_device_datum)==8)?1:-1];
#include <stdlib.h>
#include <string.h>

struct owned_array { void *data;struct owned_array *next; };
struct block_owner { struct owned_array *head;size_t bytes; };
struct mac_native_scenario_scenery_record { struct mac_native_scenario_object_core object;struct mac_native_scenario_permutation permutation; };
struct mac_native_scenario_sound_scenery_record { struct mac_native_scenario_object_core object; };
typedef char native_object_core_size[sizeof(struct mac_native_scenario_object)>=40?1:-1];
typedef char native_object_position_offset[offsetof(struct mac_native_scenario_object,position)==8?1:-1];
typedef char native_permutation_offset[offsetof(struct mac_native_scenario_object,change_colors)==40?1:-1];
typedef char native_name_size[sizeof(struct mac_native_scenario_name)==36?1:-1];
typedef char scenario_object_core_source_size[sizeof(struct mac_native_scenario_object_core)==0x28?1:-1];
typedef char scenario_permutation_source_size[sizeof(struct mac_native_scenario_permutation)==0x20?1:-1];
typedef char scenery_engine_stride[sizeof(struct mac_native_scenario_scenery_record)==0x48?1:-1];
typedef char vehicle_engine_stride[sizeof(struct mac_native_scenario_vehicle_record)==0x78?1:-1];
typedef char sound_scenery_engine_stride[sizeof(struct mac_native_scenario_sound_scenery_record)==0x28?1:-1];
#if UINTPTR_MAX == UINT32_MAX
typedef char native_tag_reference_size[sizeof(struct mac_native_scenario_reference)==16?1:-1];
typedef char native_palette_entry_size[sizeof(struct mac_native_scenario_palette_entry)==48?1:-1];
#endif
static void *allocate(struct block_owner *o,size_t size)
{
    struct owned_array *a;void *p;
    if(!size || size>UINT32_C(0x4000000)-sizeof(*a) || o->bytes>UINT32_C(0x4000000)-size-sizeof(*a))return NULL;
    a=(struct owned_array *)malloc(sizeof(*a));if(!a)return NULL;
    p=calloc(1,size);if(!p){free(a);return NULL;}
    a->data=p;a->next=o->head;o->head=a;o->bytes+=size+sizeof(*a);return p;
}
static void free_arrays(struct block_owner *o)
{
    struct owned_array *a,*next;if(!o)return;
    for(a=o->head;a;a=next){next=a->next;free(a->data);free(a);}free(o);
}
void mac_native_scenario_blocks_free(struct mac_native_scenario_blocks *blocks)
{
    if(!blocks)return;
    free_arrays((struct block_owner *)blocks->owner);
    mac_native_scenario_objects_free(&blocks->decoded);
    memset(blocks,0,sizeof(*blocks));
}
static void set_name_mappings(struct mac_native_scenario_blocks *b)
{
    /* This is object_names_postprocess(FALSE)'s object-type then datum order:
       object_types.c's table enum is biped, vehicle, weapon, equipment, ... */
    static const unsigned order[]={MAC_SCENARIO_BIPED,MAC_SCENARIO_VEHICLE,
        MAC_SCENARIO_WEAPON,MAC_SCENARIO_EQUIPMENT,MAC_SCENARIO_SCENERY,
        MAC_SCENARIO_MACHINE,MAC_SCENARIO_CONTROL,MAC_SCENARIO_LIGHT_FIXTURE,
        MAC_SCENARIO_SOUND_SCENERY};
    static const int16_t object_type[MAC_SCENARIO_FAMILY_COUNT]={6,0,1,3,2,7,8,9,11};
    uint32_t i,j;
    for(i=0;i<(uint32_t)b->object_name_count;++i) {
        b->object_names[i].runtime_object_type=-1;
        b->object_names[i].runtime_scenario_datum_index=-1;
    }
    for(i=0;i<sizeof(order)/sizeof(order[0]);++i) {
        struct mac_native_scenario_family *f=b->decoded.families+order[i];
        for(j=0;j<f->placement_count;++j) {
            int16_t name_index=f->placements[j].name_index;
            if(name_index>=0) {
                b->object_names[name_index].runtime_object_type=object_type[order[i]];
                b->object_names[name_index].runtime_scenario_datum_index=(int16_t)j;
            }
        }
    }
}
static uint32_t family_for_object_type(int16_t type)
{
    switch(type) {
    case 0:return MAC_SCENARIO_BIPED;case 1:return MAC_SCENARIO_VEHICLE;
    case 2:return MAC_SCENARIO_WEAPON;case 3:return MAC_SCENARIO_EQUIPMENT;
    case 6:return MAC_SCENARIO_SCENERY;case 7:return MAC_SCENARIO_MACHINE;
    case 8:return MAC_SCENARIO_CONTROL;case 9:return MAC_SCENARIO_LIGHT_FIXTURE;
    case 11:return MAC_SCENARIO_SOUND_SCENERY;default:return MAC_SCENARIO_FAMILY_COUNT;
    }
}
static int validate_name_mappings(const struct mac_native_scenario_blocks *b)
{
    uint32_t i,j;
    for(i=0;i<MAC_SCENARIO_FAMILY_COUNT;++i) {
        const struct mac_native_scenario_family *f=b->decoded.families+i;
        for(j=0;j<f->placement_count;++j) {
            int16_t ni=f->placements[j].name_index;uint32_t family;
            const struct mac_native_scenario_family *mapped;
            if(ni<0)continue;
            if((uint32_t)ni>=(uint32_t)b->object_name_count)return 0;
            family=family_for_object_type(b->object_names[ni].runtime_object_type);
            if(family==MAC_SCENARIO_FAMILY_COUNT)return 0;
            mapped=b->decoded.families+family;
            if(b->object_names[ni].runtime_scenario_datum_index<0 ||
               (uint32_t)b->object_names[ni].runtime_scenario_datum_index>=mapped->placement_count)return 0;
            if(mapped->placements[b->object_names[ni].runtime_scenario_datum_index].name_index!=ni)return 0;
        }
    }
    return 1;
}
static void copy_reference(struct mac_native_scenario_reference *out,
    const struct mac_native_scenario_palette *in)
{
    uint32_t bits=in->handle;out->group_tag=in->group_tag;out->name=in->name;
    out->name_length=(int32_t)in->name_length;memcpy(&out->index,&bits,4);
}
static void copy_palette_entry(struct mac_native_scenario_palette_entry *out,
    const struct mac_native_scenario_palette *in)
{
    /* The placement code consumes the reference only. The original
       scenario_object_palette_entry unused[8] is not read on placement. */
    copy_reference(&out->reference,in);
    memset(out->unused,0,sizeof(out->unused));
}
int mac_native_scenario_blocks_decode(const struct mac_retail_cache *cache,
    struct mac_native_scenario_blocks *out,const char **error)
{
    struct mac_native_scenario_blocks b;struct block_owner *owner=NULL;uint32_t i,j;
    static const uint32_t allowed[]={MAC_SCENARIO_SCENERY,MAC_SCENARIO_BIPED,
        MAC_SCENARIO_EQUIPMENT,MAC_SCENARIO_WEAPON,MAC_SCENARIO_VEHICLE,MAC_SCENARIO_SOUND_SCENERY,MAC_SCENARIO_MACHINE,
        MAC_SCENARIO_CONTROL,MAC_SCENARIO_LIGHT_FIXTURE};
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));memset(&b,0,sizeof(b));
#define FAIL(s) do { if(error)*error=(s);goto fail; } while(0)
    if(!mac_native_scenario_objects_decode(cache,&b.decoded,error))return 0;
    for(i=0;i<MAC_SCENARIO_FAMILY_COUNT;++i) {
        const struct mac_native_scenario_family *f=b.decoded.families+i;int accepted=0;
        for(j=0;j<sizeof(allowed)/sizeof(allowed[0]);++j)if(i==allowed[j])accepted=1;
        /* (a family with a palette but nothing placed places nothing: its
           palette is kept, as other maps' weapons and equipment lists are) */
        if(!accepted && f->placement_count) {
            /* (every such family named, then the first refused) */
            for(j=i;j<MAC_SCENARIO_FAMILY_COUNT;++j) {
                const struct mac_native_scenario_family *g=b.decoded.families+j;
                if((j==MAC_SCENARIO_EQUIPMENT || j==MAC_SCENARIO_WEAPON) && g->placement_count)
                    fprintf(stderr,"native scenario object family %u: %u placements, %u palette entries\n",
                        (unsigned)j,(unsigned)g->placement_count,(unsigned)g->palette_count);
            }
            FAIL("unsupported nonempty scenario object family");
        }
    }
    owner=(struct block_owner *)calloc(1,sizeof(*owner));if(!owner)FAIL("scenario block owner allocation");
    b.object_names=b.decoded.names;b.object_name_count=(int32_t)b.decoded.name_count;
    set_name_mappings(&b);
    if(!validate_name_mappings(&b))FAIL("scenario object-name runtime mapping");
    for(i=0;i<MAC_SCENARIO_FAMILY_COUNT;++i) {
        const struct mac_native_scenario_family *src=b.decoded.families+i;
        struct mac_native_scenario_native_family *dst=b.families+i;
        /* (the retail strides, which the game steps by: object_types.c's
           placement_tag_block_element_size) */
        uint32_t stride=(i==MAC_SCENARIO_SCENERY?0x48:(i==MAC_SCENARIO_VEHICLE||i==MAC_SCENARIO_BIPED)?0x78:
            i==MAC_SCENARIO_WEAPON?0x5c:
            i==MAC_SCENARIO_LIGHT_FIXTURE?0x58:(i==MAC_SCENARIO_MACHINE||i==MAC_SCENARIO_CONTROL)?0x40:0x28);
        dst->placement_count=(int32_t)src->placement_count;
        dst->palette_count=(int32_t)src->palette_count;
        if(src->palette_count) {
            dst->palette=(struct mac_native_scenario_palette_entry *)allocate(owner,src->palette_count*sizeof(*dst->palette));
            if(!dst->palette)FAIL("scenario native palette allocation");
            for(j=0;j<src->palette_count;++j)copy_palette_entry(dst->palette+j,src->palette+j);
        }
        if(src->placement_count) {
            unsigned char *records;
            if(i==MAC_SCENARIO_SCENERY && stride!=0x48)FAIL("scenery stride witness");
            if(i==MAC_SCENARIO_VEHICLE && stride!=0x78)FAIL("vehicle stride witness");
            if(i==MAC_SCENARIO_SOUND_SCENERY && stride!=0x28)FAIL("sound-scenery stride witness");
            records=(unsigned char *)allocate(owner,(size_t)src->placement_count*stride);
            if(!records)FAIL("scenario native placement allocation");
            dst->placements=records;
            for(j=0;j<src->placement_count;++j) {
                const struct mac_native_scenario_object *p=src->placements+j;
                unsigned char *record=records+(size_t)j*stride;
                memcpy(record,p,40);
                if(i==MAC_SCENARIO_SCENERY)memcpy(record+40,p->change_colors,32);
                else if(i==MAC_SCENARIO_BIPED || i==MAC_SCENARIO_WEAPON) {
                    /* scenario_biped_datum: object, permutation, then the
                       scenario_unit_datum; scenario_weapon_datum: object,
                       permutation, rounds total and loaded, flags. Both
                       leave the remainder unused. */
                    memcpy(record+40,p->change_colors,32);
                    if(i==MAC_SCENARIO_BIPED) {
                        memcpy(record+72,&p->unit_body_vitality,4);memcpy(record+76,&p->unit_flags,4);
                    } else {
                        memcpy(record+0x48,&p->weapon_rounds_total,2);
                        memcpy(record+0x4a,&p->weapon_rounds_loaded,2);
                        memcpy(record+0x4c,&p->weapon_flags,2);
                    }
                } else if(i==MAC_SCENARIO_VEHICLE) {
                    memcpy(record+40,p->change_colors,32);
                    memcpy(record+72,&p->unit_body_vitality,4);memcpy(record+76,&p->unit_flags,4);
                    record[0x58]=(unsigned char)p->multiplayer_team_index;
                    record[0x59]=p->multiplayer_spawn_flags_pad;
                    memcpy(record+0x5a,&p->multiplayer_spawn_flags,2);
                    /* The remaining 0x1c bytes are marked unused by the
                       project-owned CE placement layout. */
                } else if(p->has_device) {
                    /* scenario_device_datum, then the kind's own fields
                       (device_machines.h, device_controls.c,
                       device_light_fixtures.h) */
                    memcpy(record+40,&p->device_power_group,2);
                    memcpy(record+42,&p->device_position_group,2);
                    memcpy(record+44,&p->device_flags,4);
                    if(i==MAC_SCENARIO_MACHINE)record[48]=p->machine_flags;
                    else if(i==MAC_SCENARIO_CONTROL) {
                        memcpy(record+48,&p->control_flags,2);
                        memcpy(record+52,&p->control_custom_name_index,2);
                    } else {
                        memcpy(record+48,p->light_color,12);
                        memcpy(record+60,&p->light_intensity,4);
                        memcpy(record+64,&p->light_falloff_angle,4);
                        memcpy(record+68,&p->light_cutoff_angle,4);
                    }
                } else if(i==MAC_SCENARIO_MACHINE || i==MAC_SCENARIO_CONTROL || i==MAC_SCENARIO_LIGHT_FIXTURE)
                    FAIL("device placement without its device fields");
            }
        }
    }
    b.owner=owner;b.bytes_owned=b.decoded.bytes_owned+owner->bytes;*out=b;return 1;
fail:
    free_arrays(owner);
    mac_native_scenario_objects_free(&b.decoded);memset(out,0,sizeof(*out));return 0;
#undef FAIL
}
