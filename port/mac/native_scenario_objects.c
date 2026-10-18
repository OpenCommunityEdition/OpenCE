#include "native_scenario_objects.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* These offsets, strides and group tags are the matching rows in
   source/objects/object_types.c. scenario_object_datum and scenario_object_name
   are from source/scenario/scenario_definitions.h. */
struct family_layout { uint32_t placement,palette,stride,group; };
static const struct family_layout layouts[MAC_SCENARIO_FAMILY_COUNT]={
    {0x210,0x21c,0x48,UINT32_C(0x7363656e)}, /* scenery */
    {0x228,0x234,0x78,UINT32_C(0x62697064)}, /* biped */
    {0x240,0x24c,0x78,UINT32_C(0x76656869)}, /* vehicle */
    {0x258,0x264,0x28,UINT32_C(0x65716970)}, /* equipment */
    {0x270,0x27c,0x5c,UINT32_C(0x77656170)}, /* weapon */
    {0x294,0x2a0,0x40,UINT32_C(0x6d616368)}, /* machine */
    {0x2ac,0x2b8,0x40,UINT32_C(0x6374726c)}, /* control */
    {0x2c4,0x2d0,0x58,UINT32_C(0x6c696669)}, /* light fixture */
    {0x2dc,0x2e8,0x28,UINT32_C(0x73736365)}  /* sound scenery */
};
struct allocation { void *p;struct allocation *next; };
struct owner { struct allocation *head;size_t bytes; };
static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t le16(const unsigned char *p)
{ return (uint16_t)(p[0]|(unsigned)p[1]<<8); }
static int16_t sle16(const unsigned char *p)
{ uint16_t u=le16(p);int16_t v;memcpy(&v,&u,2);return v; }
static int finite_word(uint32_t u)
{ return (u&UINT32_C(0x7fffffff))<UINT32_C(0x7f800000); }
static void *owned(struct owner *o,size_t n)
{
    struct allocation *a;void *p;
    if(!n || n>UINT32_C(0x4000000)-sizeof(*a) || o->bytes>UINT32_C(0x4000000)-n-sizeof(*a))return NULL;
    a=(struct allocation *)malloc(sizeof(*a));if(!a)return NULL;
    p=calloc(1,n);if(!p){free(a);return NULL;}
    a->p=p;a->next=o->head;o->head=a;o->bytes+=n+sizeof(*a);return p;
}
static const unsigned char *block_span(const struct mac_retail_cache *c,
    uint32_t count,uint32_t address,uint32_t stride)
{
    uint64_t bytes=(uint64_t)count*stride;
    if(count>32767 || bytes>UINT32_MAX)return NULL;
    if(!count) {
        if(address==0 || address==UINT32_MAX)return (const unsigned char *)1;
        if(address&3)return NULL;
        return mac_retail_tag_span(c,address,1);
    }
    if(address==0 || address==UINT32_MAX || (address&3))return NULL;
    return mac_retail_tag_span(c,address,(uint32_t)bytes);
}
static int decode_palette(const struct mac_retail_cache *c,struct owner *o,
    uint32_t expected,const unsigned char *p,struct mac_native_scenario_palette *out)
{
    struct mac_retail_tag tag;uint32_t group=le32(p),address=le32(p+4),length=le32(p+8),handle=le32(p+12);
    const unsigned char *name,*end;size_t available,n;
    out->group_tag=group;out->handle=handle;out->name=NULL;out->name_length=0;
    if(handle==UINT32_MAX)return group==0 && address==0 && length==0;
    if(group!=expected || length>255 || !mac_retail_tag_get(c,handle&65535,&tag) ||
       tag.handle!=handle || tag.group!=expected)return 0;
    if(!address || address!=le32(c->tags+c->instances_offset+(handle&65535)*32+16))return 0;
    name=mac_retail_tag_span(c,address,1);if(!name)return 0;
    available=c->tag_size-(address-MAC_RETAIL_BASE);if(available>256)available=256;
    end=(const unsigned char *)memchr(name,0,available);if(!end)return 0;
    n=(size_t)(end-name);
    if((length && length!=n) || (size_t)(end-name)!=strlen(tag.name))return 0;
    out->name=(char *)owned(o,n+1);if(!out->name)return 0;
    memcpy(out->name,name,n+1);out->name_length=(uint32_t)n;return 1;
}
static int decode_placement(const unsigned char *p,uint32_t stride,uint32_t palettes,
    uint32_t names,uint32_t device_groups,uint32_t family,struct mac_native_scenario_object *out)
{
    uint32_t j;
    if(stride<40)return 0;
    out->palette_entry_index=sle16(p);out->name_index=sle16(p+2);
    out->placement_flags=le16(p+4);out->variant_number=sle16(p+6);
    if((out->palette_entry_index!=-1 && (out->palette_entry_index<0 || (uint32_t)out->palette_entry_index>=palettes)) ||
       (out->name_index!=-1 && (out->name_index<0 || (uint32_t)out->name_index>=names)) ||
       (out->placement_flags&~UINT16_C(0x001f)))return 0;
    for(j=0;j<6;++j) {
        uint32_t bits=le32(p+8+j*4);float f;
        if(!finite_word(bits))return 0;memcpy(&f,&bits,4);
        if(j<3)out->position[j]=f;else out->rotation[j-3]=f;
    }
    out->on_bsp_flags=le16(p+32);out->misc_flags=le16(p+34);out->unused=le32(p+36);
    /* Scenery, biped, vehicle and weapon placement records carry the
       scenario_object_permutation at byte 0x28 (scenario_definitions.h;
       vehicles.c has the same fixed 0x20-byte permutation field). */
    if((family==MAC_SCENARIO_SCENERY || family==MAC_SCENARIO_BIPED ||
        family==MAC_SCENARIO_VEHICLE || family==MAC_SCENARIO_WEAPON) && stride>=72) {
        for(j=0;j<4;++j)out->change_colors[j]=le32(p+40+j*4);
        memcpy(out->region_permutations,p+56,8);
        out->permutation_unused[0]=le32(p+64);out->permutation_unused[1]=le32(p+68);
        out->has_permutation=1;
    }
    /* biped and vehicle scenario records append scenario_unit_datum at 0x48;
       unit_place reads both the vitality and flags fields. */
    if((family==MAC_SCENARIO_BIPED || family==MAC_SCENARIO_VEHICLE) && stride>=80) {
        uint32_t bits=le32(p+72);
        if(!finite_word(bits))return 0;
        memcpy(&out->unit_body_vitality,&bits,4);out->unit_flags=le32(p+76);
        out->has_unit_state=1;
    }
    /* scenario_weapon_datum: rounds total and loaded, then flags, after the
       permutation. */
    if(family==MAC_SCENARIO_WEAPON && stride>=0x50) {
        out->weapon_rounds_total=sle16(p+0x48);out->weapon_rounds_loaded=sle16(p+0x4a);
        out->weapon_flags=le16(p+0x4c);out->has_weapon=1;
    }
    /* Custom Edition's vehicle placement extension occupies bytes 0x58..0x5b
       of the retail 0x78 record. Preserve it for the stock placement path;
       port/linux/game/custom_edition_objects.c verifies these source offsets. */
    if(family==MAC_SCENARIO_VEHICLE && stride==0x78) {
        out->multiplayer_team_index=(int8_t)p[0x58];
        out->multiplayer_spawn_flags_pad=p[0x59];
        out->multiplayer_spawn_flags=le16(p+0x5a);
        out->has_vehicle_state=1;
    }
    /* Devices (devices.h, device_machines.h, device_controls.c,
       device_light_fixtures.h): scenario_device_datum at 0x28, whose groups
       name the scenario's device groups by index. */
    if((family==MAC_SCENARIO_MACHINE || family==MAC_SCENARIO_CONTROL ||
        family==MAC_SCENARIO_LIGHT_FIXTURE) && stride>=0x40) {
        out->device_power_group=sle16(p+40);out->device_position_group=sle16(p+42);
        out->device_flags=le32(p+44);
        if((out->device_power_group!=-1 && (out->device_power_group<0 || (uint32_t)out->device_power_group>=device_groups)) ||
           (out->device_position_group!=-1 && (out->device_position_group<0 || (uint32_t)out->device_position_group>=device_groups)))
            return 0;
        out->has_device=1;
        if(family==MAC_SCENARIO_MACHINE)out->machine_flags=p[48];
        else if(family==MAC_SCENARIO_CONTROL) {
            out->control_flags=le16(p+48);out->control_custom_name_index=sle16(p+52);
        } else {
            float *light[6];
            light[0]=&out->light_color[0];light[1]=&out->light_color[1];light[2]=&out->light_color[2];
            light[3]=&out->light_intensity;light[4]=&out->light_falloff_angle;light[5]=&out->light_cutoff_angle;
            if(stride<0x48)return 0;
            for(j=0;j<6;++j) {
                uint32_t bits=le32(p+48+j*4);
                if(!finite_word(bits))return 0;memcpy(light[j],&bits,4);
            }
        }
    }
    return 1;
}
void mac_native_scenario_objects_free(struct mac_native_scenario_objects *view)
{
    struct owner *o;struct allocation *a,*next;
    if(!view)return;o=(struct owner *)view->owner;
    if(o){for(a=o->head;a;a=next){next=a->next;free(a->p);free(a);}free(o);}
    memset(view,0,sizeof(*view));
}
int mac_native_scenario_objects_decode(const struct mac_retail_cache *c,
    struct mac_native_scenario_objects *out,const char **error)
{
    struct mac_native_scenario_objects v;struct owner *o=NULL;
    const unsigned char *scenario,*raw;uint32_t i,j,count,address,device_groups;
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));memset(&v,0,sizeof(v));
#define FAIL(s) do { if(error)*error=(s);goto fail; } while(0)
    if(!c || !c->tags || c->scenario_offset>c->tag_size || 0x2f0>c->tag_size-c->scenario_offset)FAIL("scenario object arguments/range");
    scenario=c->tags+c->scenario_offset;o=(struct owner *)calloc(1,sizeof(*o));if(!o)FAIL("scenario object owner allocation");
    /* object_names is at 0x204; struct scenario_object_name is 36 bytes. */
    count=le32(scenario+0x204);address=le32(scenario+0x208);
    raw=block_span(c,count,address,36);if(!raw)FAIL("scenario object-names block");
    v.name_count=count;
    if(count){v.names=(struct mac_native_scenario_name *)owned(o,(size_t)count*sizeof(*v.names));if(!v.names)FAIL("scenario object-names allocation");}
    for(i=0;i<count;++i) {
        const unsigned char *p=raw+i*36;const unsigned char *end=(const unsigned char *)memchr(p,0,32);
        if(!end)FAIL("scenario object-name termination");
        memcpy(v.names[i].name,p,(size_t)(end-p));
        v.names[i].runtime_object_type=sle16(p+32);v.names[i].runtime_scenario_datum_index=sle16(p+34);
    }
    /* device_groups is at 0x288 (devices.c); devices name its entries */
    device_groups=le32(scenario+0x288);
    for(i=0;i<MAC_SCENARIO_FAMILY_COUNT;++i) {
        const struct family_layout *l=layouts+i;struct mac_native_scenario_family *f=v.families+i;
        uint32_t pn,pa,dn,da;const unsigned char *pal,*dat;
        dn=le32(scenario+l->placement);da=le32(scenario+l->placement+4);
        pn=le32(scenario+l->palette);pa=le32(scenario+l->palette+4);
        dat=block_span(c,dn,da,l->stride);pal=block_span(c,pn,pa,48);
        if(!dat || !pal)FAIL("scenario object placement/palette block range");
        if(dn && !pn)FAIL("scenario placements without palette");
        f->placement_count=dn;f->palette_count=pn;
        if(pn){f->palette=(struct mac_native_scenario_palette *)owned(o,(size_t)pn*sizeof(*f->palette));if(!f->palette)FAIL("scenario palette allocation");}
        if(dn){f->placements=(struct mac_native_scenario_object *)owned(o,(size_t)dn*sizeof(*f->placements));if(!f->placements)FAIL("scenario placements allocation");}
        for(j=0;j<pn;++j)if(!decode_palette(c,o,l->group,pal+j*48,f->palette+j))FAIL("scenario palette reference/group/name");
        for(j=0;j<dn;++j)if(!decode_placement(dat+j*l->stride,l->stride,pn,count,device_groups,i,f->placements+j))FAIL("scenario placement fields/index/pose");
    }
    v.owner=o;v.bytes_owned=o->bytes;*out=v;return 1;
fail:
    if(o){struct allocation *a=o->head,*next;for(;a;a=next){next=a->next;free(a->p);free(a);}free(o);}
    memset(out,0,sizeof(*out));return 0;
#undef FAIL
}
