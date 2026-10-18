#include "cseries.h"
#include "cache/cache_files.h"
#include "engine_synchronous_precache.h"

#include <string.h>

static const struct mac_retail_cache *active_cache;

static const char *map_leaf(const char *name)
{
    const char *leaf=name,*p;
    if(!name)return NULL;
    for(p=name;*p;++p)if(*p=='/' || *p=='\\')leaf=p+1;
    return *leaf?leaf:NULL;
}

static int ascii_equal_folded(const char *a,const char *b)
{
    unsigned char ca,cb;
    if(!a||!b)return 0;
    while(*a&&*b) {
        ca=(unsigned char)*a++;cb=(unsigned char)*b++;
        if(ca>='A'&&ca<='Z')ca=(unsigned char)(ca-'A'+'a');
        if(cb>='A'&&cb<='Z')cb=(unsigned char)(cb-'A'+'a');
        if(ca!=cb)return 0;
    }
    return *a==0&&*b==0;
}

int mac_engine_precache_bind(const struct mac_retail_cache *cache)
{
    if(active_cache || !cache || !cache->file || !cache->tag_count || !cache->name[0])return 0;
    active_cache=cache;
    return 1;
}

int mac_engine_precache_unbind(const struct mac_retail_cache *cache)
{
    if(!cache || active_cache!=cache)return 0;
    active_cache=NULL;
    return 1;
}

void cache_files_precache_set_priority(boolean blocking)
{
    (void)blocking;
    /* There is no asynchronous native map copy to reprioritize. */
}

boolean cache_files_precache_in_progress(void)
{
    return FALSE;
}

boolean cache_files_precache_is_copying_map(const char *map_name)
{
    (void)map_name;
    return FALSE;
}

boolean cache_files_precache_map_loaded(const char *map_name)
{
    const char *leaf=map_leaf(map_name);
    return active_cache && leaf && ascii_equal_folded(leaf,active_cache->name);
}

short cache_files_precache_map_status(real *progress)
{
    boolean loaded=(active_cache!=NULL && active_cache->name[0]);
    if(progress)*progress=loaded?1.0f:0.0f;
    return loaded?_cached_map_file_success:_cached_map_file_failed;
}

void cache_files_precache_map_end(void)
{
    /* An already-open native cache has no copy operation to end. */
}

void cache_files_precache_map_queue_end(void)
{
    /* No queued copy exists in the synchronous native owner. */
}

/* port/mac/engine_menu.c (a weak reference: the tools without the menus do not link it) */
extern int mac_engine_menu_enabled(void) __attribute__((weak));

boolean cache_files_precache_map_begin(const char *map_name,boolean copy_map)
{
    (void)copy_map;
    /* TRUE means the requested map is already the validated native map.
       A different/missing map is refused; this never copies or opens it. */
    if(cache_files_precache_map_loaded(map_name))return TRUE;
    /* port: with the menus, a map they name that is not the one loaded (the map
       of a game they host, which they name before they start it) is not
       precached here: its game opens it, as the menus' other games do. The
       refusal is the damaged disc error, which never ends. */
    return mac_engine_menu_enabled && mac_engine_menu_enabled() && map_name && map_name[0];
}
