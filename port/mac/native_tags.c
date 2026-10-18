#include "native_tags.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define NONE UINT32_MAX
#define GROUP(a,b,c,d) ((uint32_t)(a)<<24|(uint32_t)(b)<<16|(uint32_t)(c)<<8|(uint32_t)(d))
/* Bounded bring-up ceiling measured against the real Blood Gulch session;
   this is not a minimum-memory target for the native port. */
#define NATIVE_BUDGET (256U*1024U*1024U)
struct entry {
    uint32_t handle,group,native_group,parents[2];char name[256];
    void *root,*owner;size_t root_bytes,owned_bytes,base_owned_bytes;void (*destroy)(void *);
};
struct state { uint32_t count,checksum;size_t bytes,budget;struct entry *entries; };
/* Changes whenever an entry's handle, group or root may have (what a memo of
   lookups, as tag_get's, must be discarded on) */
unsigned long mac_native_tags_epoch=1;
/* port: the loading screen (tools/mac_engine_session_probe.c) keeps the window drawn and
   its events handled while a map's tags are made: the pump is called for each tag published,
   the progress hook says how far a session's load has come (0 to 1) */
void (*mac_loading_pump_hook)(void)=NULL;
void (*mac_loading_progress_hook)(float)=NULL;
/* Inheritance comes from engine tag families, not an untrusted parent claim.
   Unknown leaf groups may be inventoried, but cannot claim an inherited type. */
static void parents(uint32_t group,uint32_t p[2])
{
    p[0]=p[1]=NONE;
    switch(group) {
    case GROUP('b','i','p','d'):case GROUP('v','e','h','i'):
        p[0]=GROUP('u','n','i','t');p[1]=GROUP('o','b','j','e');break;
    case GROUP('w','e','a','p'):case GROUP('e','q','i','p'):case GROUP('g','a','r','b'):
        p[0]=GROUP('i','t','e','m');p[1]=GROUP('o','b','j','e');break;
    case GROUP('m','a','c','h'):case GROUP('c','t','r','l'):case GROUP('l','i','f','i'):
        p[0]=GROUP('d','e','v','i');p[1]=GROUP('o','b','j','e');break;
    case GROUP('u','n','i','t'):case GROUP('i','t','e','m'):case GROUP('d','e','v','i'):
    case GROUP('p','r','o','j'):case GROUP('s','c','e','n'):case GROUP('s','s','c','e'):
        p[0]=GROUP('o','b','j','e');break;
    case GROUP('s','o','s','o'):case GROUP('s','e','n','v'):case GROUP('s','c','h','i'):
    case GROUP('s','c','e','x'):case GROUP('s','p','l','a'):case GROUP('s','w','a','t'):
    case GROUP('s','g','l','a'):case GROUP('s','m','e','t'):case GROUP('s','o','t','r'):
        p[0]=GROUP('s','h','d','r');break;
    }
}
int mac_native_tag_group_matches(uint32_t actual,uint32_t requested)
{
    uint32_t p[2];
    if(actual==NONE || requested==NONE)return 0;
    parents(actual,p);
    return actual==requested || p[0]==requested || p[1]==requested;
}
static struct entry *lookup(const struct mac_native_tags *tags,uint32_t h)
{
    struct state *s=tags?(struct state *)tags->state:NULL;
    if(!s || h==NONE || (h&65535)>=s->count || s->entries[h&65535].handle!=h)return NULL;
    return &s->entries[h&65535];
}
void mac_native_tags_free(struct mac_native_tags *tags)
{
    struct state *s;uint32_t i;
    if(!tags)return;s=(struct state *)tags->state;
    ++mac_native_tags_epoch;
    if(s) { for(i=0;i<s->count;++i)if(s->entries[i].root)s->entries[i].destroy(s->entries[i].owner);
        free(s->entries);free(s); }
    tags->state=NULL;
}
int mac_native_tags_create(const struct mac_retail_cache *cache,struct mac_native_tags *out,const char **error)
{
    struct state *s;uint32_t i,p[2];struct mac_retail_tag t;const char *end;size_t name_size;
    if(error)*error="native tag metadata arguments";
    if(!out)return 0;out->state=NULL;
    ++mac_native_tags_epoch;
    if(!cache || !cache->tag_count || cache->tag_count>65536)return 0;
    s=(struct state *)calloc(1,sizeof(*s));if(!s)return 0;
    s->checksum=cache->tag_checksum;s->budget=NATIVE_BUDGET;
    s->entries=(struct entry *)calloc(cache->tag_count,sizeof(*s->entries));
    if(!s->entries) { free(s);return 0; }s->count=cache->tag_count;out->state=s;
    for(i=0;i<s->count;++i) {
        if(!mac_retail_tag_get(cache,i,&t))goto bad;
        parents(t.group,p);
        if((t.handle&65535)!=i || t.handle==NONE || t.group==NONE ||
            t.parents[0]!=p[0] || t.parents[1]!=p[1])goto bad;
        name_size=cache->tag_size-(size_t)((const unsigned char *)t.name-cache->tags);
        if(name_size>256)name_size=256;
        end=(const char *)memchr(t.name,0,name_size);if(!end)goto bad;
        s->entries[i].handle=t.handle;s->entries[i].group=t.group;s->entries[i].native_group=t.group;
        s->entries[i].parents[0]=p[0];s->entries[i].parents[1]=p[1];
        memcpy(s->entries[i].name,t.name,(size_t)(end-t.name)+1);
    }
    if(error)*error=NULL;return 1;
bad:
    mac_native_tags_free(out);if(error)*error="native tag salt, name or canonical inheritance";return 0;
}
static int publish(struct mac_native_tags *tags,uint32_t handle,uint32_t group,uint32_t native_group,
    void *root,size_t root_bytes,size_t owned_bytes,void *owner,void (*destroy)(void *),const char **error)
{
    if(mac_loading_pump_hook)mac_loading_pump_hook();
    struct entry *e=lookup(tags,handle);struct state *s=tags?(struct state *)tags->state:NULL;uint32_t i;
    if(error)*error="invalid or duplicate native tag publication";
    if(!e || e->group!=group || e->root || !root || !root_bytes || !owner || !destroy ||
        owned_bytes<root_bytes)return 0;
    if(owned_bytes>s->budget || s->bytes>s->budget-owned_bytes) {
        if(error)*error="native tag registry ownership budget exceeded";
        fprintf(stderr,"native tag budget: %08lx (%s), current=%lu requested=%lu limit=%lu\n",
            (unsigned long)handle,e->name,(unsigned long)s->bytes,
            (unsigned long)owned_bytes,(unsigned long)s->budget);
        return 0;
    }
    for(i=0;i<s->count;++i)if(s->entries[i].root &&
        (s->entries[i].owner==owner || s->entries[i].root==root))return 0;
    e->native_group=native_group;parents(native_group,e->parents);
    ++mac_native_tags_epoch;
    e->root=root;e->root_bytes=root_bytes;e->owned_bytes=owned_bytes;e->base_owned_bytes=owned_bytes;
    e->owner=owner;e->destroy=destroy;s->bytes+=owned_bytes;
    if(error)*error=NULL;return 1;
}
int mac_native_tags_publish(struct mac_native_tags *tags,uint32_t handle,uint32_t group,
    void *root,size_t root_bytes,size_t owned_bytes,void *owner,void (*destroy)(void *),const char **error)
{ return publish(tags,handle,group,group,root,root_bytes,owned_bytes,owner,destroy,error); }
int mac_native_tags_publish_model(struct mac_native_tags *tags,uint32_t handle,
    void *root,size_t root_bytes,size_t owned_bytes,void *owner,void (*destroy)(void *),const char **error)
{ return publish(tags,handle,GROUP('m','o','d','2'),GROUP('m','o','d','e'),root,root_bytes,owned_bytes,owner,destroy,error); }
uint32_t mac_native_tags_group(const struct mac_native_tags *tags,uint32_t handle)
{ struct entry *e=lookup(tags,handle);return e && e->root?e->native_group:NONE; }
uint32_t mac_native_tags_handle_at_index(const struct mac_native_tags *tags,uint32_t index)
{
    const struct state *s=tags?(const struct state *)tags->state:NULL;
    if(!s || index>=s->count || !s->entries[index].root)return NONE;
    return s->entries[index].handle;
}
static int equal_name(const char *a,const char *b)
{
    unsigned char x,y;
    do {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='A' && x<='Z')x=(unsigned char)(x-'A'+'a');
        if(y>='A' && y<='Z')y=(unsigned char)(y-'A'+'a');
        if(x!=y)return 0;
    } while(x);
    return 1;
}
/* Names the game looks up every frame (the HUD's, the game engine's
   messages): the last answer for each (tags, group, name), checked against
   the entry it names, or a miss while the tags are unchanged. */
#define FIND_CACHE_SIZE 128
struct find_cache_entry { const struct state *s;uint32_t count,group,hash,handle;char name[64]; };
static struct find_cache_entry find_cache[FIND_CACHE_SIZE];
static uint32_t find_name_hash(const char *name)
{
    uint32_t h=2166136261U;unsigned char c;
    while((c=(unsigned char)*name++)!=0) { if(c>='A'&&c<='Z')c=(unsigned char)(c-'A'+'a');h=(h^c)*16777619U; }
    return h;
}
uint32_t mac_native_tags_find(const struct mac_native_tags *tags,uint32_t group,const char *name)
{
    struct state *s=tags?(struct state *)tags->state:NULL;uint32_t i,hash;
    struct find_cache_entry *cached;
    if(!s || !name || group==NONE)return NONE;
    hash=find_name_hash(name);
    cached=&find_cache[(hash^group)%FIND_CACHE_SIZE];
    if(cached->s==s&&cached->count==s->count&&cached->group==group&&cached->hash==hash&&
       equal_name(cached->name,name)) {
        if((cached->handle&65535)<s->count&&s->entries[cached->handle&65535].handle==cached->handle&&
           s->entries[cached->handle&65535].root&&s->entries[cached->handle&65535].native_group==group&&
           equal_name(s->entries[cached->handle&65535].name,name))return cached->handle;
    }
    for(i=0;i<s->count;++i)if(s->entries[i].root &&
        s->entries[i].native_group==group && equal_name(s->entries[i].name,name))break;
    /* (hits only: an entry may gain its root later) */
    if(i<s->count&&strlen(name)<sizeof(cached->name)) {
        cached->s=s;cached->count=s->count;cached->group=group;cached->hash=hash;
        cached->handle=i<s->count?s->entries[i].handle:NONE;
        strcpy(cached->name,name);
    }
    return i<s->count?s->entries[i].handle:NONE;
}
uint32_t mac_native_tags_find_metadata(const struct mac_native_tags *tags,uint32_t group,const char *name)
{
    struct state *s=tags?(struct state *)tags->state:NULL;uint32_t i;
    if(!s || !name || group==NONE)return NONE;
    for(i=0;i<s->count;++i)if(s->entries[i].group==group && equal_name(s->entries[i].name,name))return s->entries[i].handle;
    return NONE;
}
uint32_t mac_native_tags_next(const struct mac_native_tags *tags,uint32_t group,uint32_t *cursor)
{
    const struct state *s=tags?(const struct state *)tags->state:NULL;
    const struct entry *e;
    if(!s || !cursor)return NONE;
    while(*cursor<s->count) {
        e=&s->entries[(*cursor)++];
        if(e->root && (group==NONE || mac_native_tag_group_matches(e->native_group,group)))return e->handle;
    }
    return NONE;
}
const void *mac_native_tags_get(const struct mac_native_tags *tags,uint32_t group,uint32_t handle,
    size_t minimum,const char **error)
{
    struct entry *e=lookup(tags,handle);
    if(error)*error="native tag handle or type mismatch";
    if(!e || group==NONE || (group!=e->native_group && group!=e->parents[0] && group!=e->parents[1]))return NULL;
    if(!e->root) { if(error)*error="tag family has not been converted to native memory";return NULL; }
    if(minimum>e->root_bytes) { if(error)*error="native tag root is too small";return NULL; }
    if(error)*error=NULL;return e->root;
}
/* Inventory of the map's tags whose family has no native root yet, by group:
   a lookup of any of them fails as "tag family has not been converted". */
void mac_native_tags_report_unconverted(const struct mac_native_tags *tags,FILE *out)
{
    enum { MAXIMUM_GROUPS=256, EXAMPLES=3 };
    struct report { uint32_t group,total,converted;const char *examples[EXAMPLES]; } groups[MAXIMUM_GROUPS];
    const struct state *s=tags?(const struct state *)tags->state:NULL;
    uint32_t count=0,i,g,missing=0,printed[MAXIMUM_GROUPS];
    if(!s || !out)return;
    memset(groups,0,sizeof(groups));
    for(i=0;i<s->count;++i) {
        const struct entry *e=&s->entries[i];
        for(g=0;g<count && groups[g].group!=e->group;++g) {}
        if(g==count) { if(count==MAXIMUM_GROUPS)continue;groups[count++].group=e->group; }
        ++groups[g].total;
        if(e->root)++groups[g].converted;
        else {
            uint32_t k=groups[g].total-groups[g].converted-1;
            if(k<EXAMPLES)groups[g].examples[k]=e->name;
            ++missing;
        }
    }
    fprintf(out,"native tag inventory: %lu tags, %lu groups, %lu unconverted\n",
        (unsigned long)s->count,(unsigned long)count,(unsigned long)missing);
    memset(printed,0,sizeof(printed));
    for(;;) {
        uint32_t best=MAXIMUM_GROUPS,best_missing=0,k;
        for(g=0;g<count;++g) {
            uint32_t m=groups[g].total-groups[g].converted;
            if(!printed[g] && m>best_missing) { best=g;best_missing=m; }
        }
        if(best==MAXIMUM_GROUPS)break;
        printed[best]=1;
        fprintf(out,"native tag unconverted: %c%c%c%c %lu of %lu:",
            (int)(groups[best].group>>24&255),(int)(groups[best].group>>16&255),
            (int)(groups[best].group>>8&255),(int)(groups[best].group&255),
            (unsigned long)best_missing,(unsigned long)groups[best].total);
        for(k=0;k<EXAMPLES && k<best_missing;++k)
            fprintf(out," %s%s",groups[best].examples[k]?groups[best].examples[k]:"?",k+1<EXAMPLES&&k+1<best_missing?",":"");
        fprintf(out,"\n");
    }
}
const char *mac_native_tags_name(const struct mac_native_tags *tags,uint32_t handle)
{ struct entry *e=lookup(tags,handle);return e?e->name:NULL; }
size_t mac_native_tags_owned_bytes(const struct mac_native_tags *tags)
{ const struct state *s=tags?(const struct state *)tags->state:NULL;return s?s->bytes:0; }
#ifdef HALO_MAC_NATIVE_TAGS_TESTING
int mac_native_tags_test_set_budget(struct mac_native_tags *tags,size_t budget)
{ struct state *s=tags?(struct state *)tags->state:NULL;if(!s||budget<s->bytes||budget>NATIVE_BUDGET)return 0;s->budget=budget;return 1; }
#endif
int mac_native_tags_owner_bytes_reserve(struct mac_native_tags *tags,uint32_t handle,
    void *owner,size_t bytes,const char **error)
{
    struct state *s=tags?(struct state *)tags->state:NULL;
    struct entry *e=lookup(tags,handle);
    if(error)*error="native tag owner accounting mismatch";
    if(!s||!e||!owner||!e->root||e->owner!=owner)return 0;
    if(bytes>s->budget||bytes>s->budget-s->bytes||
       bytes>(size_t)-1-e->owned_bytes) {
        if(error)*error="native tag registry ownership budget exceeded";
        fprintf(stderr,"native tag growth budget: %08lx (%s), current=%lu requested=%lu limit=%lu\n",
            (unsigned long)handle,e->name,(unsigned long)s->bytes,(unsigned long)bytes,
            (unsigned long)s->budget);
        return 0;
    }
    e->owned_bytes+=bytes;s->bytes+=bytes;if(error)*error=NULL;return 1;
}
int mac_native_tags_owner_bytes_release(struct mac_native_tags *tags,uint32_t handle,
    void *owner,size_t bytes)
{
    struct state *s=tags?(struct state *)tags->state:NULL;
    struct entry *e=lookup(tags,handle);
    if(!s||!e||!owner||!e->root||e->owner!=owner||
       e->owned_bytes<e->base_owned_bytes||bytes>e->owned_bytes-e->base_owned_bytes||bytes>s->bytes)return 0;
    e->owned_bytes-=bytes;s->bytes-=bytes;return 1;
}
int mac_native_tags_checksum(const struct mac_native_tags *tags,uint32_t *out)
{ const struct state *s=tags?(const struct state *)tags->state:NULL;if(!s||!out)return 0;*out=s->checksum;return 1; }

/* ---- tags added after the cache's (the menus') */
static void no_destroy(void *owner) { (void)owner; }
uint32_t mac_native_tags_count(const struct mac_native_tags *tags)
{ const struct state *s=tags?(const struct state *)tags->state:NULL;return s?s->count:0; }
int mac_native_tags_extend(struct mac_native_tags *tags,uint32_t count)
{
    struct state *s=tags?(struct state *)tags->state:NULL;struct entry *grown;
    if(!s || count<s->count || count>65535)return 0;
    if(count==s->count)return 1;
    ++mac_native_tags_epoch;
    grown=(struct entry *)realloc(s->entries,count*sizeof(*grown));
    if(!grown)return 0;
    memset(grown+s->count,0,(count-s->count)*sizeof(*grown));
    s->entries=grown;s->count=count;return 1;
}
int mac_native_tags_define(struct mac_native_tags *tags,uint32_t index,uint32_t handle,uint32_t group,
    const char *name,void *root,size_t root_bytes)
{
    struct state *s=tags?(struct state *)tags->state:NULL;struct entry *e;size_t length;
    if(!s || index>=s->count || (handle&65535)!=index || handle==NONE || group==NONE || !name || !root || !root_bytes)return 0;
    e=&s->entries[index];
    if(e->root || e->handle)return 0;
    ++mac_native_tags_epoch;
    length=strlen(name);if(length>=sizeof(e->name))length=sizeof(e->name)-1;
    memcpy(e->name,name,length);e->name[length]=0;
    e->handle=handle;e->group=group;e->native_group=group;parents(group,e->parents);
    e->root=root;e->root_bytes=root_bytes;e->owner=root;e->destroy=no_destroy;
    return 1;
}
int mac_native_tags_set_root(struct mac_native_tags *tags,uint32_t handle,void *root)
{
    struct entry *e=lookup(tags,handle);
    if(!e || !e->root || !root)return 0;
    ++mac_native_tags_epoch;
    e->root=root;e->owner=root;return 1;
}
void mac_native_tags_truncate(struct mac_native_tags *tags,uint32_t count)
{
    struct state *s=tags?(struct state *)tags->state:NULL;uint32_t i;
    if(!s || count>=s->count)return;
    ++mac_native_tags_epoch;
    for(i=count;i<s->count;++i)if(s->entries[i].root)s->entries[i].destroy(s->entries[i].owner);
    s->count=count;
}
void *mac_native_tags_root(const struct mac_native_tags *tags,uint32_t index,uint32_t *handle,
    uint32_t *group,const char **name)
{
    const struct state *s=tags?(const struct state *)tags->state:NULL;
    if(!s || index>=s->count)return NULL;
    if(handle)*handle=s->entries[index].handle;
    if(group)*group=s->entries[index].native_group;
    if(name)*name=s->entries[index].name;
    return s->entries[index].root;
}
