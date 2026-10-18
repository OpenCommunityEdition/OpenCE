#include "cseries.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "native_scenario_skies.h"
#undef malloc
#undef calloc
#undef free
#undef memset
#include <stdlib.h>
#include <string.h>

#define WITNESS(name,test) typedef char name[(test)?1:-1]
WITNESS(scenario_sky_reference_size,sizeof(struct tag_reference)==16);
WITNESS(scenario_sky_references_offset,offsetof(struct scenario,sky_references)==0x30);

struct sky_owner { char **names; uint32_t count; };
static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint32_t le_group(const char a,const char b,const char c,const char d)
{ return ((uint32_t)(unsigned char)a<<24)|((uint32_t)(unsigned char)b<<16)|((uint32_t)(unsigned char)c<<8)|(unsigned char)d; }
static int32_t signed32(uint32_t bits)
{ int32_t value;memcpy(&value,&bits,4);return value; }

void mac_native_scenario_skies_free(struct mac_native_scenario_skies *out)
{
    struct sky_owner *owner;uint32_t i;
    if(!out)return;
    owner=(struct sky_owner *)out->owner;
    if(owner) { for(i=0;i<owner->count;++i)free(owner->names[i]);free(owner->names);free(owner); }
    free(out->references);memset(out,0,sizeof(*out));
}

int mac_native_scenario_skies_decode(const struct mac_retail_cache *cache,
    struct mac_native_scenario_skies *out,const char **error)
{
    const unsigned char *root,*field,*raw;struct mac_native_scenario_skies result;
    struct sky_owner *owner;struct tag_reference *references;uint32_t count,address,i,sky;
    if(error)*error="native sky output or cache";
    if(!out)return 0;memset(out,0,sizeof(*out));memset(&result,0,sizeof(result));
    if(!cache||!cache->tags)return 0;
    root=mac_retail_tag_span(cache,MAC_RETAIL_BASE+cache->scenario_offset,sizeof(struct scenario));
    if(!root)return 0;
    field=root+offsetof(struct scenario,sky_references);count=le32(field);address=le32(field+4);
    if(count>MAXIMUM_SKIES_PER_SCENARIO) { if(error)*error="scenario sky reference count";return 0; }
    if(!count) {
        if(address && address!=UINT32_MAX && ((address&3)||!mac_retail_tag_span(cache,address,1))) {
            if(error)*error="empty scenario sky reference span";return 0;
        }
        if(error)*error=NULL;return 1;
    }
    if(!address||address==UINT32_MAX||(address&3)) { if(error)*error="scenario sky reference address";return 0; }
    raw=mac_retail_tag_span(cache,address,count*16);if(!raw) { if(error)*error="scenario sky reference span";return 0; }
    owner=(struct sky_owner *)calloc(1,sizeof(*owner));if(!owner) { if(error)*error="scenario sky owner allocation";return 0; }
    references=(struct tag_reference *)calloc(count,sizeof(*references));
    owner->names=(char **)calloc(count,sizeof(*owner->names));
    if(!references||!owner->names) { free(references);free(owner->names);free(owner);if(error)*error="scenario sky allocation";return 0; }
    owner->count=count;result.references=references;result.count=count;result.owner=owner;sky=le_group('s','k','y',' ');
    for(i=0;i<count;++i) {
        const unsigned char *p=raw+i*16;uint32_t group=le32(p),name_address=le32(p+4),name_length=le32(p+8),handle=le32(p+12);
        struct mac_retail_tag tag;size_t length;
        references[i].group_tag=group;references[i].index=signed32(handle);
        references[i].name=NULL;references[i].name_length=0;
        if(group!=sky || name_length>255)goto invalid;
        if(handle==UINT32_MAX) {
            if(name_address||name_length)goto invalid;
            continue;
        }
        if((handle&0xffff)>=cache->tag_count || !mac_retail_tag_get(cache,handle&0xffff,&tag) ||
           tag.handle!=handle || tag.group!=sky || !name_address ||
           name_address!=le32(cache->tags+cache->instances_offset+(handle&0xffff)*32+16))goto invalid;
        for(length=0;length<256 && tag.name[length];++length) {}
        if(length==256 || (name_length && name_length!=length))goto invalid;
        owner->names[i]=(char *)malloc(length+1);if(!owner->names[i])goto invalid;
        memcpy(owner->names[i],tag.name,length+1);references[i].name=owner->names[i];references[i].name_length=(long)length;
    }
    *out=result;if(error)*error=NULL;return 1;
invalid:
    mac_native_scenario_skies_free(&result);if(error)*error="scenario sky reference group, handle, or name";return 0;
}

int mac_native_scenario_skies_attach(const struct mac_native_scenario_skies *skies,
    struct scenario *scenario)
{
    if(!skies||!scenario)return 0;
    scenario->sky_references.count=(short)skies->count;
    scenario->sky_references.address=skies->references;
    scenario->sky_references.definition=NULL;
    return 1;
}
