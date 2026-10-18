#include "cseries.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "ai/ai_scenario_definitions.h"
#include "cutscene/recorded_animation_definitions.h"
#include "ai/actions.h"
#include "native_scenario_gameplay.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct allocation { void *data; struct allocation *next; };
struct owner { struct allocation *head; size_t bytes; };
#define WITNESS(name,test) typedef char name[(test)?1:-1]
WITNESS(scenario_gameplay_flag_size,sizeof(struct scenario_netgame_flag)==0x94);
WITNESS(scenario_gameplay_equipment_size,sizeof(struct scenario_netgame_equipment)==0x90);
WITNESS(scenario_gameplay_starting_size,sizeof(struct scenario_starting_equipment)==0xcc);
WITNESS(scenario_gameplay_reference_size,sizeof(struct tag_reference)==16);
WITNESS(scenario_gameplay_flags_offset,offsetof(struct scenario,netgame_flags)==0x378);
WITNESS(scenario_gameplay_equipment_offset,offsetof(struct scenario,netgame_equipment)==0x384);
WITNESS(scenario_gameplay_starting_offset,offsetof(struct scenario,scenario_starting_equipment)==0x390);

/* devices.c's (TU-private) scenario_device_group, 52 bytes on disk */
struct native_scenario_device_group {
    char name[32];
    real initial_value;
    unsigned long flags;
    long unused[3];
};
WITNESS(scenario_gameplay_device_group_size,sizeof(struct native_scenario_device_group)==0x34);
WITNESS(scenario_gameplay_device_groups_offset,offsetof(struct scenario,device_groups)==0x288);

/* cinematics.c's (TU-private) scenario_cutscene_title, 96 bytes on disk */
struct native_scenario_title {
    long flags;
    char name[32];
    long pad24;
    short bounds[4];
    short text_index;
    unsigned short style,justification,pad36;
    unsigned long text_flags,foreground_color,shadow_color;
    real fade_in_time,up_time,fade_out_time;
    unsigned char unused50[16];
};
WITNESS(scenario_gameplay_title_size,sizeof(struct native_scenario_title)==0x60);
WITNESS(scenario_gameplay_title_fade_offset,offsetof(struct native_scenario_title,fade_in_time)==0x44);
WITNESS(scenario_gameplay_trigger_size,sizeof(struct scenario_trigger_volume)==0x60);
WITNESS(scenario_gameplay_flag_record_size,sizeof(struct scenario_cutscene_flag)==0x5c);
WITNESS(scenario_gameplay_camera_size,sizeof(struct scenario_cutscene_camera_point)==0x68);
WITNESS(scenario_gameplay_profile_size,sizeof(struct scenario_starting_profile)==0x68);
WITNESS(scenario_gameplay_recording_size,sizeof(struct recorded_animation_definition)==0x40);
WITNESS(scenario_gameplay_encounter_size,sizeof(struct encounter_definition)==0xb0);
WITNESS(scenario_gameplay_squad_size,sizeof(struct squad_definition)==0xe8);
WITNESS(scenario_gameplay_platoon_size,sizeof(struct platoon_definition)==0xac);
WITNESS(scenario_gameplay_command_list_size,sizeof(struct ai_command_list_definition)==0x60);
WITNESS(scenario_gameplay_conversation_size,sizeof(struct ai_conversation)==0x74);
WITNESS(scenario_gameplay_trigger_offset,offsetof(struct scenario,trigger_volumes)==0x360);
WITNESS(scenario_gameplay_encounters_offset,offsetof(struct scenario,ai_encounters)==0x42c);

/* ai_communication.c's (TU-private) conversation participant and line views,
   84 and 124 bytes on disk */
struct native_conversation_participant {
    word pad00;
    word flags;
    short selection_type;
    short actor_type;
    short preexisting_object_name_index;
    short new_attach_object_name_index;
    byte unknown0c[0x0c];
    short dialogue_variants[6];
    char ai_index_name[32];
    long runtime_ai_index;
    byte unknown48[0x0c];
};
struct native_conversation_line {
    word flags;
    short participant_index;
    short address_type;
    short address_participant_index;
    long unknown08;
    real delay_time;
    byte unknown10[0x0c];
    struct tag_reference dialogue[6];
};
WITNESS(scenario_gameplay_participant_size,sizeof(struct native_conversation_participant)==0x54);
WITNESS(scenario_gameplay_line_size,sizeof(struct native_conversation_line)==0x7c);
WITNESS(scenario_gameplay_command_size,sizeof(struct ai_command_definition)==0x20);
WITNESS(scenario_gameplay_command_point_size,sizeof(struct ai_command_point_definition)==0x14);
WITNESS(scenario_gameplay_firing_size,sizeof(struct firing_position_definition)==0x18);
WITNESS(scenario_gameplay_move_position_size,sizeof(struct move_position_definition)==0x50);
WITNESS(scenario_gameplay_player_location_size,sizeof(struct encounter_player_starting_location)==0x34);
WITNESS(scenario_gameplay_animation_reference_size,sizeof(struct ai_animation_reference_definition)==0x3c);

static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t le16(const unsigned char *p)
{ return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
static int16_t sle16(const unsigned char *p)
{ uint16_t u=le16(p);int16_t v;memcpy(&v,&u,2);return v; }
static int finite_bits(uint32_t u)
{ return (u&UINT32_C(0x7fffffff))<UINT32_C(0x7f800000); }
static int read_float(const unsigned char *p, real *out)
{
    uint32_t bits=le32(p); if(!finite_bits(bits))return 0;
    memcpy(out,&bits,4);return 1;
}
static void *owned(struct owner *o,size_t size)
{
    struct allocation *a;void *p;
    if(!size || size>UINT32_C(0x4000000)-sizeof(*a) ||
       o->bytes>UINT32_C(0x4000000)-size-sizeof(*a))return NULL;
    a=(struct allocation *)malloc(sizeof(*a));if(!a)return NULL;
    p=calloc(1,size);if(!p){free(a);return NULL;}
    a->data=p;a->next=o->head;o->head=a;o->bytes+=size+sizeof(*a);return p;
}
static void owner_free(struct owner *o)
{
    struct allocation *a,*next;if(!o)return;
    for(a=o->head;a;a=next){next=a->next;free(a->data);free(a);}free(o);
}
void mac_native_scenario_gameplay_free(struct mac_native_scenario_gameplay *v)
{
    if(!v)return;owner_free((struct owner *)v->owner);memset(v,0,sizeof(*v));
}
static const unsigned char *block(const struct mac_retail_cache *c,
    const unsigned char *scenario,size_t offset,uint32_t stride,uint32_t maximum,uint32_t *count)
{
    uint32_t address;uint64_t bytes;
    *count=le32(scenario+offset);address=le32(scenario+offset+4);
    if(*count>maximum)return NULL;
    bytes=(uint64_t)*count*stride;if(bytes>UINT32_MAX)return NULL;
    if(!*count) {
        if(!address || address==UINT32_MAX)return (const unsigned char *)1;
        if(address&3)return NULL;
        return mac_retail_tag_span(c,address,1);
    }
    if(!address || address==UINT32_MAX || (address&3))return NULL;
    return mac_retail_tag_span(c,address,(uint32_t)bytes);
}
static int reference(const struct mac_retail_cache *c,struct owner *owner,
    const unsigned char *raw,struct tag_reference *out)
{
    static const uint32_t itmc=UINT32_C(0x69746d63);
    uint32_t group=le32(raw),name_address=le32(raw+4),name_length=le32(raw+8),handle=le32(raw+12);
    struct mac_retail_tag tag;size_t length;
    out->group_tag=group;out->index=0;memcpy(&out->index,&handle,4);out->name=NULL;out->name_length=0;
    if(handle==UINT32_MAX)return (group==0 || group==itmc) && name_address==0 && name_length==0;
    if(group!=itmc || name_length>255 || (handle&0xffff)>=c->tag_count ||
       !mac_retail_tag_get(c,handle&0xffff,&tag) || tag.handle!=handle || tag.group!=itmc)
        return 0;
    if(!name_address || name_address!=le32(c->tags+c->instances_offset+(handle&0xffff)*32+16))return 0;
    for(length=0;length<256 && tag.name[length];++length) {}
    if(length==256)return 0;
    if(name_length && name_length!=length)return 0;
    out->name=(char *)owned(owner,length+1);if(!out->name)return 0;
    memcpy(out->name,tag.name,length+1);out->name_length=(long)length;return 1;
}
static int copy_game_types(const unsigned char *raw,short out[4])
{
    unsigned i;
    for(i=0;i<4;++i) {
        int16_t v=sle16(raw+i*2);
        /* Engine game-type selectors use NONE or one of the stock type ids. */
        if(v!=NONE && (v<0 || v>=15))return 0;
        out[i]=v;
    }
    return 1;
}
static int copy_pose(const unsigned char *raw,real_point3d *position,real *facing)
{
    return read_float(raw,&position->x)&&read_float(raw+4,&position->y)&&
        read_float(raw+8,&position->z)&&read_float(raw+12,facing);
}
static int copy_team(int16_t team)
{ return team==NONE || (team>=0 && team<16); }


/* ---- campaign blocks.
   Each is read field by field from the little-endian map. The AI's encounters
   keep their names, platoons and squads (a script names them) but no firing
   positions, move positions or starting locations, and the command lists and
   conversations only their names: no AI is placed yet. Recorded animations keep
   their event streams, checked and (on a big-endian host) converted. */
static int name32(const unsigned char *raw,char *out,size_t out_size)
{
    if(!memchr(raw,0,32))return 0;
    memcpy(out,raw,out_size<32?out_size:32);
    if(out_size<=32)out[out_size-1]=0;
    return 1;
}
/* ---- recorded animations' event streams.
   The map holds a little-endian byte-coded stream: the unit control (its fields
   by unit control data version), for version 4 the animation state (six
   shorts), then events up to the end event. Version 4 events are a header byte
   (time delta in the low two bits, event type above), 0 to 2 bytes of delta and
   the event's data; versions 1 to 3 events are two shorts (type, delta) and
   data. The game reads the stream in place, in host order: this walks it to
   check every read is inside it and ends in an end event, and on a big-endian
   host converts it (including the header byte, whose bit-fields run from the
   other end there). */
static void rec_swap(unsigned char *p,unsigned size,int swap)
{
    if(swap && size==2){unsigned char t=p[0];p[0]=p[1];p[1]=t;}
    else if(swap && size==4){unsigned char t=p[0];p[0]=p[3];p[3]=t;t=p[1];p[1]=p[2];p[2]=t;}
}
static int recording_stream_walk(unsigned char *s,size_t size,unsigned version,unsigned ucv,int swap)
{
    static const unsigned char control0[]={1,1,2,2,2,4,4,4,4,4,4,4,4,4,4,4};
    static const unsigned char v4_data[23]={0,0,1,1,2,2,8,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4};
    size_t p=0;unsigned i,n=ucv<1?1:ucv;
    if(n>4)return 0;
    for(i=0;i<sizeof(control0);++i){if(p+control0[i]>size)return 0;rec_swap(s+p,control0[i],swap);p+=control0[i];}
    if(n>=2){if(p+4>size)return 0;rec_swap(s+p,4,swap);p+=4;}
    if(n>=3){if(p+2>size)return 0;rec_swap(s+p,2,swap);p+=2;}
    if(n>=4){if(p+2>size)return 0;rec_swap(s+p,2,swap);p+=2;}
    if(version==4) {
        if(p+12>size)return 0;
        for(i=0;i<6;++i)rec_swap(s+p+i*2,2,swap);
        p+=12;
        for(;;) {
            unsigned h,td,type,k,data;
            if(p>=size)return 0;
            h=s[p];td=h&3;type=h>>2;
            if(type>=23)return 0;
            data=v4_data[type];
            if(swap)s[p]=(unsigned char)((td<<6)|type);
            ++p;
            if(td==2){if(p+1>size)return 0;++p;}
            else if(td==3){if(p+2>size)return 0;rec_swap(s+p,2,swap);p+=2;}
            if(type==1)return 1;
            if(p+data>size)return 0;
            if(type==4||type==5)rec_swap(s+p,2,swap);
            else if(type==6){rec_swap(s+p,4,swap);rec_swap(s+p+4,4,swap);}
            else if(type>=15){for(k=0;k<2;++k)rec_swap(s+p+k*2,2,swap);}
            p+=data;
        }
    }
    if(version>=1 && version<=3) {
        static const unsigned char v1_size[23]={4,4,6,6,6,6,12,4,4,16,16,16,16,16,16,16,12,12,12,12,12,12,12};
        for(;;) {
            unsigned type,k,event;
            if(p+4>size)return 0;
            type=s[p]|(unsigned)s[p+1]<<8;
            if(type>=23)return 0;
            event=v1_size[type];
            if(p+event>size)return 0;
            rec_swap(s+p,2,swap);rec_swap(s+p+2,2,swap);
            if(type==1)return 1;
            if(type==4||type==5)rec_swap(s+p+4,2,swap);
            else if(type==6||(type>=16))for(k=4;k<event;k+=4)rec_swap(s+p+k,4,swap);
            else if(type>=9)for(k=4;k<16;k+=4)rec_swap(s+p+k,4,swap);
            p+=event;
        }
    }
    return 0;
}
static int read_floats(const unsigned char *raw,real *out,unsigned count)
{
    unsigned i;
    for(i=0;i<count;++i)if(!read_float(raw+i*4,out+i))return 0;
    return 1;
}
static int typed_reference(const struct mac_retail_cache *c,struct owner *owner,
    const unsigned char *raw,uint32_t expected,struct tag_reference *out)
{
    uint32_t group=le32(raw),handle=le32(raw+12);
    struct mac_retail_tag tag;size_t length;
    out->group_tag=group;out->index=0;memcpy(&out->index,&handle,4);out->name=NULL;out->name_length=0;
    if(handle==UINT32_MAX)return 1;
    if(group!=expected || !mac_retail_tag_get(c,handle&0xffff,&tag) || tag.handle!=handle || tag.group!=expected)return 0;
    for(length=0;length<256 && tag.name[length];++length) {}
    if(length==256)return 0;
    out->name=(char *)owned(owner,length+1);if(!out->name)return 0;
    memcpy(out->name,tag.name,length+1);out->name_length=(long)length;return 1;
}
static int decode_campaign(const struct mac_retail_cache *c,const unsigned char *scenario,
    struct owner *o,struct mac_native_scenario_gameplay *v,const char **error)
{
    const unsigned char *raw;uint32_t i,j,count;
    uint32_t total_squads=0,total_starts=0,total_moves=0,total_firing=0,total_platoons=0,total_commands=0,total_points=0,total_lines=0;
    uint32_t recordings_decoded=0,recordings_dropped=0;
    uint32_t palette_count=0,animation_reference_count=0,script_reference_count=0,recording_reference_count=0,command_list_count=0;
#define FAIL(message) do { *error=(message); return 0; } while(0)
    raw=block(c,scenario,offsetof(struct scenario,trigger_volumes),0x60,1024,&count);
    if(!raw)FAIL("scenario trigger volumes block range");
    v->trigger_volume_count=count;
    if(count) {
        struct scenario_trigger_volume *dst=(struct scenario_trigger_volume *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario trigger volumes allocation");v->trigger_volumes=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x60;
            dst[i].type=sle16(p);dst[i].pad=le16(p+2);
            if(!name32(p+4,dst[i].name,sizeof(dst[i].name)) || dst[i].type<0 || dst[i].type>1 ||
               !read_floats(p+48,&dst[i].forward.i,3) || !read_floats(p+60,&dst[i].up.i,3) ||
               !read_floats(p+72,&dst[i].position.x,3) || !read_floats(p+84,&dst[i].extents.i,3))
                FAIL("scenario trigger volume fields");
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,cutscene_flags),0x5c,1024,&count);
    if(!raw)FAIL("scenario cutscene flags block range");
    v->cutscene_flag_count=count;
    if(count) {
        struct scenario_cutscene_flag *dst=(struct scenario_cutscene_flag *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario cutscene flags allocation");v->cutscene_flags=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x5c;
            if(!name32(p+4,dst[i].name,sizeof(dst[i].name)) || !read_floats(p+36,&dst[i].position.x,3) ||
               !read_floats(p+48,&dst[i].facing.yaw,2))
                FAIL("scenario cutscene flag fields");
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,cutscene_camera_points),0x68,1024,&count);
    if(!raw)FAIL("scenario camera points block range");
    v->camera_point_count=count;
    if(count) {
        struct scenario_cutscene_camera_point *dst=(struct scenario_cutscene_camera_point *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario camera points allocation");v->camera_points=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x68;
            dst[i].flags=(long)le32(p);
            if(!name32(p+4,dst[i].name,sizeof(dst[i].name)) || !read_floats(p+40,&dst[i].position.x,3) ||
               !read_floats(p+52,&dst[i].orientation.yaw,3) || !read_float(p+64,&dst[i].field_of_view))
                FAIL("scenario camera point fields");
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,cutscene_chapter_titles),0x60,64,&count);
    if(!raw)FAIL("scenario chapter titles block range");
    v->chapter_title_count=count;
    if(count) {
        struct native_scenario_title *dst=(struct native_scenario_title *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario chapter titles allocation");v->chapter_titles=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x60;
            dst[i].flags=(long)le32(p);
            if(!name32(p+4,dst[i].name,sizeof(dst[i].name)))FAIL("scenario chapter title name");
            for(j=0;j<4;++j)dst[i].bounds[j]=sle16(p+40+j*2);
            dst[i].text_index=sle16(p+48);dst[i].style=le16(p+50);dst[i].justification=le16(p+52);
            dst[i].text_flags=le32(p+56);dst[i].foreground_color=le32(p+60);dst[i].shadow_color=le32(p+64);
            if(!read_floats(p+68,&dst[i].fade_in_time,3))FAIL("scenario chapter title times");
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,starting_profiles),0x68,64,&count);
    if(!raw)FAIL("scenario starting profiles block range");
    v->starting_profile_count=count;
    if(count) {
        struct scenario_starting_profile *dst=(struct scenario_starting_profile *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario starting profiles allocation");v->starting_profiles=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x68;
            if(!name32(p,dst[i].name,sizeof(dst[i].name)) || !read_float(p+32,&dst[i].starting_health_modifier) ||
               !read_float(p+36,&dst[i].starting_shield_modifier))FAIL("scenario starting profile fields");
            for(j=0;j<2;++j) {
                struct scenario_starting_profile_weapon *w=j?&dst[i].secondary_weapon:&dst[i].primary_weapon;
                const unsigned char *q=p+40+j*20;
                if(!typed_reference(c,o,q,UINT32_C(0x77656170),&w->weapon))FAIL("scenario starting profile weapon");
                w->rounds_loaded=sle16(q+16);w->rounds_total=sle16(q+18);
            }
            dst[i].grenade_counts[0]=(char)p[80];dst[i].grenade_counts[1]=(char)p[81];
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,recorded_animations),0x40,1024,&count);
    if(!raw)FAIL("scenario recorded animations block range");
    v->recorded_animation_count=count;
    if(count) {
        struct recorded_animation_definition *dst=(struct recorded_animation_definition *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario recorded animations allocation");v->recorded_animations=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x40;
            if(!name32(p,dst[i].name,sizeof(dst[i].name)))FAIL("scenario recorded animation name");
            /* the stream is copied (the map's is read only) and converted; one
               that is damaged is left empty (version 0): it doesn't play */
            {
            const unsigned char *q=p+44;
            int ok;
            uint32_t stream_size=le32(q),stream_address=le32(q+12);
            unsigned version=p[32],ucv=p[34];
            dst[i].event_stream.size=0;dst[i].event_stream.address=NULL;
            if(version>=1 && version<=4 && stream_size && stream_size<=UINT32_C(0x100000) && stream_address) {
                const unsigned char *src=mac_retail_tag_span(c,stream_address,stream_size);
                unsigned char *copy=src?(unsigned char *)owned(o,stream_size):NULL;
                if(copy) {
                    memcpy(copy,src,stream_size);
#if defined(__BIG_ENDIAN__)
                    ok=recording_stream_walk(copy,stream_size,version,ucv,1);
#else
                    ok=recording_stream_walk(copy,stream_size,version,ucv,0);
#endif
                    if(ok) {
                        dst[i].version=(byte)version;dst[i].raw_animation_data=(char)p[33];
                        dst[i].unit_control_data_version=(byte)ucv;dst[i].length_in_ticks=sle16(p+36);
                        dst[i].event_stream.size=(long)stream_size;dst[i].event_stream.address=copy;
                        ++recordings_decoded;
                    }
                }
            }
            if(!dst[i].version)++recordings_dropped;
            }
        }
    }
    if(count)fprintf(stderr,"native scenario recorded animations: %u of %u decoded, %u unplayable\n",(unsigned)recordings_decoded,(unsigned)count,(unsigned)recordings_dropped);
    /* actor palette: actor variant references, checked for the group */
    raw=block(c,scenario,offsetof(struct scenario,ai_actor_palette),16,256,&count);
    if(!raw)FAIL("scenario actor palette block range");
    v->ai_actor_palette_count=count;
    if(count) {
        struct tag_reference *dst=(struct tag_reference *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario actor palette allocation");v->ai_actor_palette=dst;
        for(i=0;i<count;++i)
            if(!typed_reference(c,o,raw+(size_t)i*16,UINT32_C(0x61637476),&dst[i]))FAIL("scenario actor palette reference");
    }
    palette_count=count;
    /* animation, script and recording references (action_obey's commands name them) */
    raw=block(c,scenario,offsetof(struct scenario,ai_animation_references),0x3c,256,&count);
    if(!raw)FAIL("scenario ai animation references block range");
    v->ai_animation_reference_count=count;animation_reference_count=count;
    if(count) {
        struct ai_animation_reference_definition *dst=(struct ai_animation_reference_definition *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario ai animation references allocation");v->ai_animation_references=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x3c;
            if(!name32(p,dst[i].animation_name,sizeof(dst[i].animation_name)) ||
               !typed_reference(c,o,p+32,UINT32_C(0x616e7472),&dst[i].animation_graph))
                FAIL("scenario ai animation reference");
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,ai_script_references),0x28,256,&count);
    if(!raw)FAIL("scenario ai script references block range");
    v->ai_script_reference_count=count;script_reference_count=count;
    if(count) {
        struct ai_script_reference_definition *dst=(struct ai_script_reference_definition *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario ai script references allocation");v->ai_script_references=dst;
        for(i=0;i<count;++i)
            if(!name32(raw+(size_t)i*0x28,dst[i].script_name,sizeof(dst[i].script_name)))FAIL("scenario ai script reference");
    }
    raw=block(c,scenario,offsetof(struct scenario,ai_recording_references),0x28,256,&count);
    if(!raw)FAIL("scenario ai recording references block range");
    v->ai_recording_reference_count=count;recording_reference_count=count;
    if(count) {
        struct ai_recording_reference_definition *dst=(struct ai_recording_reference_definition *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario ai recording references allocation");v->ai_recording_references=dst;
        for(i=0;i<count;++i)
            if(!name32(raw+(size_t)i*0x28,dst[i].recording_name,sizeof(dst[i].recording_name)))FAIL("scenario ai recording reference");
    }
    /* the volumes that switch the structure bsp when a player is inside them
       (players.c reads four shorts: trigger volume, source bsp, destination
       bsp, cutscene flag) */
    raw=block(c,scenario,offsetof(struct scenario,bsp_switch_trigger_volumes),8,256,&count);
    if(!raw)FAIL("scenario bsp switch trigger volumes block range");
    v->bsp_switch_volume_count=count;
    if(count) {
        short *dst=(short *)owned(o,(size_t)count*8);
        if(!dst)FAIL("scenario bsp switch trigger volumes allocation");v->bsp_switch_volumes=dst;
        for(i=0;i<count;++i)for(j=0;j<4;++j)dst[i*4+j]=sle16(raw+(size_t)i*8+j*2);
    }
    raw=block(c,scenario,offsetof(struct scenario,ai_command_lists),0x60,256,&count);
    if(!raw)FAIL("scenario command lists block range");
    v->ai_command_list_count=count;command_list_count=count;
    if(count) {
        struct ai_command_list_definition *dst=(struct ai_command_list_definition *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario command lists allocation");v->ai_command_lists=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x60;
            const unsigned char *commands,*points;uint32_t command_count,point_count;
            if(!name32(p,dst[i].name,sizeof(dst[i].name)))FAIL("scenario command list name");
            dst[i].flags=le32(p+32);
            dst[i].manual_structure_bsp_reference_index=sle16(p+44);
            dst[i].runtime_structure_bsp_reference_index=NONE;
            commands=block(c,p,48,0x20,4096,&command_count);
            points=block(c,p,60,0x14,1024,&point_count);
            total_commands+=command_count;total_points+=point_count;
            if(!commands || !points)FAIL("scenario command list commands/points block range");
            if(point_count) {
                struct ai_command_point_definition *pt=(struct ai_command_point_definition *)owned(o,(size_t)point_count*sizeof(*pt));
                if(!pt)FAIL("scenario command points allocation");
                dst[i].points.count=(long)point_count;dst[i].points.address=pt;
                for(j=0;j<point_count;++j) {
                    const unsigned char *q=points+(size_t)j*0x14;
                    if(!read_floats(q,&pt[j].position.x,3))FAIL("scenario command point position");
                    pt[j].surface_index=(long)le32(q+12);
                }
            }
            if(command_count) {
                struct ai_command_definition *cm=(struct ai_command_definition *)owned(o,(size_t)command_count*sizeof(*cm));
                if(!cm)FAIL("scenario commands allocation");
                dst[i].commands.count=(long)command_count;dst[i].commands.address=cm;
                for(j=0;j<command_count;++j) {
                    const unsigned char *q=commands+(size_t)j*0x20;
                    cm[j].atom_type=sle16(q);cm[j].atom_modifier=sle16(q+2);
                    if(!read_floats(q+4,&cm[j].parameter1,2))FAIL("scenario command parameters");
                    cm[j].point1_index=sle16(q+12);cm[j].point2_index=sle16(q+14);
                    cm[j].animation_reference_index=sle16(q+16);cm[j].script_reference_index=sle16(q+18);
                    cm[j].recording_reference_index=sle16(q+20);cm[j].command_index=sle16(q+22);
                    cm[j].object_name_index=sle16(q+24);
                    if(cm[j].atom_type<0 || cm[j].atom_type>=NUMBER_OF_AI_ATOM_TYPES)FAIL("scenario command atom");
                    /* (the animation, script, recording, object name, point
                       and command indices are bounds-checked where
                       action_obey uses them; the maps hold stale ones in
                       commands that do not, and off-by-one ones) */
                }
            }
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,ai_conversations),0x74,256,&count);
    if(!raw)FAIL("scenario conversations block range");
    v->ai_conversation_count=count;
    if(count) {
        struct ai_conversation *dst=(struct ai_conversation *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario conversations allocation");v->ai_conversations=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x74;
            const unsigned char *parts,*lines;uint32_t part_count,line_count;
            if(!name32(p,dst[i].name,sizeof(dst[i].name)))FAIL("scenario conversation name");
            dst[i].flags=le16(p+32);
            if(!read_floats(p+36,&dst[i].trigger_distance,2))FAIL("scenario conversation distances");
            parts=block(c,p,0x50,0x54,32,&part_count);
            lines=block(c,p,0x5c,0x7c,256,&line_count);
            if(!parts || !lines)FAIL("scenario conversation participants/lines block range");
            total_lines+=line_count;
            if(part_count) {
                struct native_conversation_participant *pa=(struct native_conversation_participant *)owned(o,(size_t)part_count*sizeof(*pa));
                if(!pa)FAIL("scenario conversation participants allocation");
                dst[i].participants.count=(long)part_count;dst[i].participants.address=pa;
                for(j=0;j<part_count;++j) {
                    const unsigned char *q=parts+(size_t)j*0x54;unsigned k;
                    pa[j].flags=le16(q+2);pa[j].selection_type=sle16(q+4);pa[j].actor_type=sle16(q+6);
                    pa[j].preexisting_object_name_index=sle16(q+8);pa[j].new_attach_object_name_index=sle16(q+10);
                    for(k=0;k<6;++k)pa[j].dialogue_variants[k]=sle16(q+24+k*2);
                    if(!name32(q+36,pa[j].ai_index_name,sizeof(pa[j].ai_index_name)))FAIL("scenario conversation participant name");
                    pa[j].runtime_ai_index=NONE;
                }
            }
            if(line_count) {
                struct native_conversation_line *ln=(struct native_conversation_line *)owned(o,(size_t)line_count*sizeof(*ln));
                if(!ln)FAIL("scenario conversation lines allocation");
                dst[i].lines.count=(long)line_count;dst[i].lines.address=ln;
                for(j=0;j<line_count;++j) {
                    const unsigned char *q=lines+(size_t)j*0x7c;unsigned k;
                    ln[j].flags=le16(q);ln[j].participant_index=sle16(q+2);ln[j].address_type=sle16(q+4);
                    ln[j].address_participant_index=sle16(q+6);ln[j].unknown08=(long)le32(q+8);
                    if(!read_float(q+12,&ln[j].delay_time))FAIL("scenario conversation line delay");
                    /* (a line's participants are checked where ai_communication.c plays it: a retail line holds none) */
                    for(k=0;k<6;++k)
                        if(!typed_reference(c,o,q+28+k*16,UINT32_C(0x736e6421),&ln[j].dialogue[k]))FAIL("scenario conversation line dialogue");
                }
            }
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,ai_encounters),0xb0,512,&count);
    if(!raw)FAIL("scenario encounters block range");
    v->ai_encounter_count=count;
    if(count) {
        struct encounter_definition *dst=(struct encounter_definition *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario encounters allocation");v->ai_encounters=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0xb0;
            const unsigned char *squads,*platoons,*firing,*players;
            uint32_t squad_count,platoon_count,firing_count,player_count;
            if(!name32(p,dst[i].name,sizeof(dst[i].name)))FAIL("scenario encounter name");
            dst[i].flags=le32(p+32);dst[i].team_index=sle16(p+36);dst[i].version=sle16(p+38);
            dst[i].searching=sle16(p+40);dst[i].manual_structure_bsp_reference_index=sle16(p+42);
            if(!read_floats(p+44,&dst[i].respawn_time_lower_bound,2))FAIL("scenario encounter respawn times");
            dst[i].runtime_structure_bsp_reference_index=NONE;
            squads=block(c,p,128,0xe8,128,&squad_count);
            platoons=block(c,p,140,0xac,128,&platoon_count);
            firing=block(c,p,152,0x18,8192,&firing_count);
            players=block(c,p,164,0x34,256,&player_count);
            if(!squads || !platoons || !firing || !players)FAIL("scenario encounter squads/platoons/positions block range");
            total_squads+=squad_count;total_platoons+=platoon_count;total_firing+=firing_count;
            if(squad_count) {
                struct squad_definition *sq=(struct squad_definition *)owned(o,(size_t)squad_count*sizeof(*sq));
                if(!sq)FAIL("scenario squads allocation");
                dst[i].squads.count=(long)squad_count;dst[i].squads.address=sq;
                for(j=0;j<squad_count;++j) {
                    const unsigned char *q=squads+(size_t)j*0xe8;unsigned k;
                    const unsigned char *starts,*moves;uint32_t start_count,move_count;
                    if(!name32(q,sq[j].name,sizeof(sq[j].name)))FAIL("scenario squad name");
                    sq[j].actor_palette_index=sle16(q+32);sq[j].platoon_index=sle16(q+34);
                    sq[j].initial_state=sle16(q+36);sq[j].default_state=sle16(q+38);
                    sq[j].flags=le32(q+40);sq[j].unique_leader_type=sle16(q+44);
                    sq[j].maneuver_squad_index=sle16(q+78);
                    if(!read_float(q+80,&sq[j].squad_delay_timer))FAIL("scenario squad delay");
                    for(k=0;k<MAXIMUM_NUMBER_OF_FIRING_POSITION_GROUPS;++k)sq[j].firing_position_groups[k]=le32(q+84+k*4);
                    sq[j].min_count=sle16(q+124);sq[j].max_count=sle16(q+126);sq[j].major_upgrade=sle16(q+128);
                    sq[j].respawn_min_actors=sle16(q+132);sq[j].respawn_max_actors=sle16(q+134);
                    sq[j].respawn_total_count=sle16(q+136);
                    if(!read_floats(q+140,&sq[j].respawn_time_lower_bound,2))FAIL("scenario squad respawn times");
                    /* (the palette and platoon indices are checked where encounters.c uses them: a retail squad holds a platoon of an encounter with none) */
                    starts=block(c,q,208,0x1c,1024,&start_count);
                    moves=block(c,q,196,0x50,1024,&move_count);
                    if(!starts || !moves)FAIL("scenario squad locations block range");
                    total_starts+=start_count;total_moves+=move_count;
                    if(start_count) {
                        struct actor_starting_location *st=(struct actor_starting_location *)owned(o,(size_t)start_count*sizeof(*st));
                        if(!st)FAIL("scenario starting locations allocation");
                        sq[j].starting_locations.count=(long)start_count;sq[j].starting_locations.address=st;
                        for(k=0;k<start_count;++k) {
                            const unsigned char *r=starts+(size_t)k*0x1c;
                            if(!copy_pose(r,&st[k].position,&st[k].facing))FAIL("scenario starting location pose");
                            st[k].cluster_index=sle16(r+16);st[k].noncombat_sequence_id=(signed char)r[18];st[k].flags=r[19];
                            st[k].default_state=sle16(r+20);st[k].initial_state=sle16(r+22);
                            st[k].actor_variant_index=sle16(r+24);st[k].command_list_index=sle16(r+26);
                            if((st[k].actor_variant_index!=NONE && (st[k].actor_variant_index<0 || (uint32_t)st[k].actor_variant_index>=palette_count)) ||
                               (st[k].command_list_index!=NONE && (st[k].command_list_index<0 || (uint32_t)st[k].command_list_index>=command_list_count)) ||
                               (st[k].default_state!=NONE && (st[k].default_state<0 || st[k].default_state>=12)) ||
                               (st[k].initial_state!=NONE && (st[k].initial_state<0 || st[k].initial_state>=12)))
                                FAIL("scenario starting location indices");
                        }
                    }
                    if(move_count) {
                        struct move_position_definition *mv=(struct move_position_definition *)owned(o,(size_t)move_count*sizeof(*mv));
                        if(!mv)FAIL("scenario move positions allocation");
                        sq[j].move_positions.count=(long)move_count;sq[j].move_positions.address=mv;
                        for(k=0;k<move_count;++k) {
                            const unsigned char *r=moves+(size_t)k*0x50;
                            if(!read_floats(r,&mv[k].position.x,3) || !read_floats(r+12,&mv[k].facing,4))FAIL("scenario move position fields");
                            mv[k].animation_reference_index=sle16(r+28);mv[k].sequence_id=(char)r[30];
                            mv[k].cluster_index=sle16(r+40);mv[k].surface_index=(long)le32(r+76);
                            if(mv[k].animation_reference_index!=NONE && (mv[k].animation_reference_index<0 ||
                               (uint32_t)mv[k].animation_reference_index>=animation_reference_count))
                                FAIL("scenario move position animation");
                        }
                    }
                }
            }
            if(platoon_count) {
                struct platoon_definition *pl=(struct platoon_definition *)owned(o,(size_t)platoon_count*sizeof(*pl));
                if(!pl)FAIL("scenario platoons allocation");
                dst[i].platoons.count=(long)platoon_count;dst[i].platoons.address=pl;
                for(j=0;j<platoon_count;++j) {
                    const unsigned char *q=platoons+(size_t)j*0xac;
                    if(!name32(q,pl[j].name,sizeof(pl[j].name)))FAIL("scenario platoon name");
                    pl[j].flags=le32(q+32);
                    pl[j].attacking_defending_rule.rule_type=sle16(q+48);pl[j].attacking_defending_rule.platoon_index=sle16(q+50);
                    pl[j].maneuvering_rule.rule_type=sle16(q+60);pl[j].maneuvering_rule.platoon_index=sle16(q+62);
                }
            }
            if(firing_count) {
                struct firing_position_definition *fp=(struct firing_position_definition *)owned(o,(size_t)firing_count*sizeof(*fp));
                if(!fp)FAIL("scenario firing positions allocation");
                dst[i].firing_positions.count=(long)firing_count;dst[i].firing_positions.address=fp;
                for(j=0;j<firing_count;++j) {
                    const unsigned char *q=firing+(size_t)j*0x18;
                    if(!read_floats(q,&fp[j].position.x,3))FAIL("scenario firing position");
                    fp[j].group_index=sle16(q+12);fp[j].cluster_index=sle16(q+14);fp[j].surface_index=(long)le32(q+20);
                }
            }
            if(player_count) {
                struct encounter_player_starting_location *ps=(struct encounter_player_starting_location *)owned(o,(size_t)player_count*sizeof(*ps));
                if(!ps)FAIL("scenario encounter player locations allocation");
                dst[i].player_starting_locations.count=(long)player_count;dst[i].player_starting_locations.address=ps;
                for(j=0;j<player_count;++j) {
                    const unsigned char *q=players+(size_t)j*0x34;unsigned k;
                    if(!copy_pose(q,&ps[j].position,&ps[j].facing))FAIL("scenario encounter player location");
                    ps[j].team_index=sle16(q+16);
                    for(k=0;k<4;++k)ps[j].game_types[k]=sle16(q+20+k*2);
                }
            }
        }
    }
    if(v->ai_encounter_count)
        fprintf(stderr,"native scenario ai: %u actor variants, %u encounters, %u squads, %u platoons, %u starting locations, "
            "%u move positions, %u firing positions, %u command lists (%u commands, %u points), %u conversations (%u lines)\n",
            (unsigned)v->ai_actor_palette_count,(unsigned)v->ai_encounter_count,(unsigned)total_squads,(unsigned)total_platoons,
            (unsigned)total_starts,(unsigned)total_moves,(unsigned)total_firing,(unsigned)v->ai_command_list_count,
            (unsigned)total_commands,(unsigned)total_points,(unsigned)v->ai_conversation_count,(unsigned)total_lines);
    return 1;
#undef FAIL
}
uint32_t mac_native_scenario_gameplay_profile_weapons(const struct mac_native_scenario_gameplay *v,
    uint32_t *handles,uint32_t capacity)
{
    uint32_t i,n=0;
    const struct scenario_starting_profile *profiles;
    if(!v || !handles)return 0;
    profiles=(const struct scenario_starting_profile *)v->starting_profiles;
    for(i=0;profiles && i<v->starting_profile_count;++i) {
        unsigned k;
        for(k=0;k<2 && n<capacity;++k) {
            const struct scenario_starting_profile_weapon *w=k?&profiles[i].secondary_weapon:&profiles[i].primary_weapon;
            if((uint32_t)w->weapon.index!=UINT32_MAX)handles[n++]=(uint32_t)w->weapon.index;
        }
    }
    return n;
}

uint32_t mac_native_scenario_gameplay_ai_references(const struct mac_native_scenario_gameplay *v,
    uint32_t *groups,uint32_t *handles,uint32_t capacity,uint32_t *total)
{
    uint32_t n=0,all=0,i,j,k;
    const struct tag_reference *palette;
    const struct ai_animation_reference_definition *animations;
    const struct ai_conversation *conversations;
#define ADD(group,reference) do { \
        if((uint32_t)(reference).index!=UINT32_MAX) { \
            if(n<capacity && groups && handles) { groups[n]=(group);handles[n]=(uint32_t)(reference).index;++n; } \
            ++all; \
        } } while(0)
    if(total)*total=0;
    if(!v)return 0;
    palette=(const struct tag_reference *)v->ai_actor_palette;
    for(i=0;palette && i<v->ai_actor_palette_count;++i)ADD(UINT32_C(0x61637476),palette[i]);
    animations=(const struct ai_animation_reference_definition *)v->ai_animation_references;
    for(i=0;animations && i<v->ai_animation_reference_count;++i)ADD(UINT32_C(0x616e7472),animations[i].animation_graph);
    conversations=(const struct ai_conversation *)v->ai_conversations;
    for(i=0;conversations && i<v->ai_conversation_count;++i) {
        const struct native_conversation_line *lines=(const struct native_conversation_line *)conversations[i].lines.address;
        for(j=0;lines && j<(uint32_t)conversations[i].lines.count;++j)
            for(k=0;k<6;++k)ADD(UINT32_C(0x736e6421),lines[j].dialogue[k]);
    }
#undef ADD
    if(total)*total=all;
    return n;
}

int mac_native_scenario_gameplay_decode(const struct mac_retail_cache *c,
    struct mac_native_scenario_gameplay *out,const char **error)
{
    struct mac_native_scenario_gameplay v;struct owner *o=NULL;
    const unsigned char *scenario,*raw;uint32_t i,j,count;
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));memset(&v,0,sizeof(v));
#define FAIL(message) do { if(error)*error=(message); goto fail; } while(0)
    if(!c || !c->tags || c->scenario_offset>c->tag_size ||
       sizeof(struct scenario)>c->tag_size-c->scenario_offset)FAIL("scenario gameplay root range");
    scenario=c->tags+c->scenario_offset;
    o=(struct owner *)calloc(1,sizeof(*o));if(!o)FAIL("scenario gameplay owner allocation");
    raw=block(c,scenario,offsetof(struct scenario,netgame_flags),sizeof(struct scenario_netgame_flag),
        MAXIMUM_SCENARIO_NETGAME_FLAGS_PER_SCENARIO,&count);
    if(!raw)FAIL("scenario netgame flags block range");
    v.netgame_flag_count=count;
    if(count) {
        struct scenario_netgame_flag *dst=(struct scenario_netgame_flag *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario netgame flags allocation");v.netgame_flags=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x94;unsigned k;
            for(k=0;k<3;++k)if(!read_float(p+k*4,&dst[i].position.n[k]))FAIL("scenario netgame flag position");
            if(!read_float(p+12,&dst[i].facing))FAIL("scenario netgame flag facing");
            dst[i].type=sle16(p+16);dst[i].team_index=sle16(p+18);
            /* (a teleporter's "team" is its channel, which pairs a source
               with its targets: Putput's go past 15) */
            if(dst[i].type<0 || dst[i].type>=NUMBER_OF_NETGAME_FLAG_TYPES ||
               ((dst[i].type==_netgame_flag_teleporter_source || dst[i].type==_netgame_flag_teleporter_target) ?
                   dst[i].team_index<NONE : !copy_team(dst[i].team_index))) {
                fprintf(stderr,"native netgame flag %lu: type %d team %d\n",(unsigned long)i,(int)dst[i].type,(int)dst[i].team_index);
                FAIL("scenario netgame flag type/team");
            }
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,netgame_equipment),sizeof(struct scenario_netgame_equipment),
        MAXIMUM_SCENARIO_NETGAME_EQUIPMENT_PER_SCENARIO,&count);
    if(!raw)FAIL("scenario netgame equipment block range");
    v.netgame_equipment_count=count;
    if(count) {
        struct scenario_netgame_equipment *dst=(struct scenario_netgame_equipment *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario netgame equipment allocation");v.netgame_equipment=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x90;
            memcpy(&dst[i].flags,p,4);if(!copy_game_types(p+4,dst[i].game_type))FAIL("scenario netgame equipment game type");
            dst[i].team_index=sle16(p+12);dst[i].spawn_time=sle16(p+14);
            if(!copy_team(dst[i].team_index) || (dst[i].spawn_time!=NONE && dst[i].spawn_time<0))FAIL("scenario netgame equipment team/time");
            dst[i].run_time_spawned_item_index=NONE;
            if(!copy_pose(p+0x40,&dst[i].position,&dst[i].facing))FAIL("scenario netgame equipment pose");
            if(!reference(c,o,p+0x50,&dst[i].item_collection))FAIL("scenario netgame equipment item-collection reference");
        }
    }
    raw=block(c,scenario,offsetof(struct scenario,scenario_starting_equipment),sizeof(struct scenario_starting_equipment),
        MAXIMUM_SCENARIO_STARTING_EQUIPMDNG_PER_SCENARIO,&count);
    if(!raw)FAIL("scenario starting equipment block range");
    v.starting_equipment_count=count;
    if(count) {
        struct scenario_starting_equipment *dst=(struct scenario_starting_equipment *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario starting equipment allocation");v.starting_equipment=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0xcc;
            memcpy(&dst[i].flags,p,4);if(!copy_game_types(p+4,dst[i].game_type))FAIL("scenario starting equipment game type");
            for(j=0;j<6;++j)if(!reference(c,o,p+0x3c+j*16,&dst[i].item_collection[j]))
                FAIL("scenario starting equipment item-collection reference");
        }
    }
    /* (the device groups: the game makes one data entry each, the first
       at index 0, and devices name them by index) */
    raw=block(c,scenario,offsetof(struct scenario,device_groups),sizeof(struct native_scenario_device_group),
        1024,&count);
    if(!raw)FAIL("scenario device groups block range");
    v.device_group_count=count;
    if(count) {
        struct native_scenario_device_group *dst=(struct native_scenario_device_group *)owned(o,(size_t)count*sizeof(*dst));
        if(!dst)FAIL("scenario device groups allocation");v.device_groups=dst;
        for(i=0;i<count;++i) {
            const unsigned char *p=raw+(size_t)i*0x34;
            memcpy(dst[i].name,p,32);dst[i].name[31]=0;
            if(!read_float(p+32,&dst[i].initial_value))FAIL("scenario device group initial value");
            dst[i].flags=le32(p+36);
        }
    }
    if(!decode_campaign(c,scenario,o,&v,error))goto fail;
    v.owner=o;*out=v;return 1;
fail:
    owner_free(o);memset(out,0,sizeof(*out));return 0;
#undef FAIL
}
