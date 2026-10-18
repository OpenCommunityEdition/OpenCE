#include "cseries.h"
#include "native_shader_layout.h"
#include "native_environment_assets.h"
#include "native_object_tags.h"
#include "native_effect_assets.h"
#include "engine_world.h"
#undef calloc
#undef free
#include <stdlib.h>
int mac_native_environment_lighting_load(const struct mac_retail_cache *cache,
    const struct mac_engine_world *world,const void *pixels,size_t pixel_bytes,
    struct mac_actor_assets *assets,const char **error)
{
    uint32_t *handles,count,i,lightmap;int ok=0;
    if(!cache || !world || !assets || !assets->tags.state || assets->bitmap_cache_bound) {
        if(error)*error="environment lighting arguments or bound cache";return 0;
    }
    handles=calloc(65536,sizeof(*handles));
    if(!handles){if(error)*error="environment lighting handle allocation";return 0;}
    if(!mac_engine_world_material_shader_handles(world,handles,65536,&count)) {
        if(error)*error="environment lighting native material references";goto done;
    }
    lightmap=mac_engine_world_lightmap_handle(world);
    if(!mac_actor_assets_load_bitmap(cache,pixels,pixel_bytes,assets,lightmap,error))goto done;
    for(i=0;i<count;++i) {
        struct mac_native_object_tag tag;struct shader_environment_definition *shader;
        struct mac_retail_tag metadata;
        uint32_t existing=mac_native_tags_group(&assets->tags,handles[i]);
        if(existing!=UINT32_MAX) {
            if(existing==UINT32_C(0x73636869) || existing==UINT32_C(0x73636578) ||
               existing==UINT32_C(0x73676c61) || existing==UINT32_C(0x73776174))continue;
            if(existing!=UINT32_C(0x73656e76)){if(error)*error="environment lighting shader family";goto done;}
            shader=(void *)mac_native_tags_get(&assets->tags,existing,handles[i],sizeof(*shader),error);
            if(!shader || !mac_actor_assets_load_bitmap(cache,pixels,pixel_bytes,assets,(uint32_t)shader->base_map.index,error))goto done;
            continue;
        }
        if(!mac_retail_tag_get(cache,handles[i]&65535,&metadata) || metadata.handle!=handles[i]) {
            if(error)*error="environment lighting material handle";goto done;
        }
        if(metadata.group==UINT32_C(0x73676c61) || metadata.group==UINT32_C(0x73776174)) {
            /* glass and water surfaces (Chill Out's, Beaver Creek's): the
               effect walk publishes the shader with its bitmaps */
            if(!mac_native_effect_assets_load(cache,pixels,pixel_bytes,NULL,0,assets,handles[i],error))goto done;
            continue;
        }
        if(metadata.group==UINT32_C(0x73636869) || metadata.group==UINT32_C(0x73636578)) {
            if(!mac_native_chicago_shader_decode(cache,handles[i],&tag,error))goto done;
        } else if(!mac_native_environment_shader_decode(cache,handles[i],&tag,error))goto done;
        shader=tag.root;
        if((tag.group==UINT32_C(0x73656e76) &&
            !mac_actor_assets_load_bitmap(cache,pixels,pixel_bytes,assets,(uint32_t)shader->base_map.index,error)) ||
           !mac_native_tags_publish(&assets->tags,handles[i],tag.group,tag.root,tag.root_bytes,
               tag.bytes_owned,tag.owner,mac_native_object_owner_free,error)) {
            mac_native_object_tag_free(&tag);goto done;
        }
        ++assets->published_count;
    }
    ok=1;
done:
    free(handles);return ok;
}
