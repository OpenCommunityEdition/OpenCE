#include "native_decal_assets.h"
#include "native_decal_tags.h"
struct decal_load {
    const struct mac_retail_cache *cache;const void *pixels;size_t size;
    struct mac_actor_assets *assets;const char **error;
    uint32_t active[32];unsigned depth,loaded;
};
static int load_decal(struct decal_load *,uint32_t);
static int dependency(void *context,uint32_t group,uint32_t handle)
{
    struct decal_load *c=context;
    if(group==UINT32_C(0x64656361))return load_decal(c,handle);
    if(group==UINT32_C(0x6269746d))return mac_actor_assets_load_bitmap(c->cache,c->pixels,c->size,c->assets,handle,c->error);
    if(c->error)*c->error="unsupported decal dependency group";return 0;
}
static int load_decal(struct decal_load *c,uint32_t handle)
{
    struct mac_native_object_tag tag;unsigned i;uint32_t group;
    if(handle==UINT32_MAX)return 1;
    for(i=0;i<c->depth;++i)if(c->active[i]==handle){if(c->error)*c->error="cyclic decal chain";return 0;}
    group=mac_native_tags_group(&c->assets->tags,handle);
    if(group!=UINT32_MAX)return group==UINT32_C(0x64656361);
    if(c->depth>=32 || c->loaded>=64){if(c->error)*c->error="decal chain capacity";return 0;}
    if(!mac_native_decal_decode(c->cache,handle,&tag,c->error))return 0;
    c->active[c->depth++]=handle;
    if(!mac_native_object_visit_references(&tag,dependency,c)){--c->depth;mac_native_object_tag_free(&tag);return 0;}
    --c->depth;
    if(c->loaded>=64){if(c->error)*c->error="decal asset capacity";mac_native_object_tag_free(&tag);return 0;}
    if(!mac_native_tags_publish(&c->assets->tags,handle,tag.group,tag.root,tag.root_bytes,
       tag.bytes_owned,tag.owner,mac_native_object_owner_free,c->error)){mac_native_object_tag_free(&tag);return 0;}
    ++c->loaded;++c->assets->published_count;return 1;
}
int mac_native_decal_assets_load(const struct mac_retail_cache *cache,const void *pixels,size_t size,
    struct mac_actor_assets *assets,const uint32_t *handles,unsigned count,const char **error)
{
    struct decal_load c;unsigned i;
    if(error)*error=NULL;
    if(!cache || !assets || assets->bitmap_cache_bound || (count&&!handles) || count>64)return 0;
    c.cache=cache;c.pixels=pixels;c.size=size;c.assets=assets;c.error=error;c.depth=0;c.loaded=0;
    for(i=0;i<count;++i)if(!load_decal(&c,handles[i]))return 0;
    return 1;
}
