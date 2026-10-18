#include "cseries.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "scenario/sky_definitions.h"
#include "models/model_definitions.h"
#include "models/model_animation_definitions.h"
#include "bitmaps/bitmap_group.h"
#include "engine_world.h"
#include "native_actor_assets.h"
#include "native_sky_assets.h"
#include "native_tags.h"
#undef memcpy
#undef memset
#include <string.h>

#define SKY UINT32_C(0x736b7920)
#define MODE UINT32_C(0x6d6f6465)
#define ANTR UINT32_C(0x616e7472)
#define LENS UINT32_C(0x6c656e73)
#define BITM UINT32_C(0x6269746d)
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL sky assets line %d: %s\n",__LINE__,#x);}}while(0)
struct probe_sky_root {
    struct tag_reference model,animation_graph;byte pad20[0x38];
    struct sky_atmospheric_fog outdoor_fog,indoor_fog;struct tag_reference indoor_fog_screen;
    long unusedA8;struct tag_block regions,animations,lights;
};
struct probe_sky_light { struct tag_reference lens;char marker[TAG_STRING_LENGTH+1];byte pad[0x37];real_euler_angles2d direction;byte tail[4]; };
struct probe_lens_root {
    byte prefix[0x20];struct tag_reference primary_map;byte between[0x94];
    struct tag_block reflections;byte tail[0x20];
};
static unsigned checks,failures;
int mac_sky_assets_runtime_probe(const struct mac_retail_cache *cache,const void *pixels,size_t pixel_bytes,
    struct mac_engine_world *world,struct mac_actor_assets *assets)
{
    const char *error=NULL;uint32_t handle,i;struct probe_sky_root *sky;struct model *model;
    struct animation_graph *graph;struct probe_sky_light *lights;
    checks=failures=0;
    CHECK(cache&&pixels&&world&&assets);
    CHECK(mac_engine_world_sky_handle(world,0,&handle));
    CHECK(mac_native_sky_assets_load(cache,pixels,pixel_bytes,assets,handle,&error));
    if(failures){fprintf(stderr,"sky assets load: %s\n",error?error:"probe precondition");return 0;}
    CHECK(mac_native_tags_group(&assets->tags,handle)==SKY);
    sky=(struct probe_sky_root *)mac_native_tags_get(&assets->tags,SKY,handle,sizeof(*sky),&error);
    CHECK(sky!=NULL);if(!sky)return 0;
    CHECK(sky->indoor_fog_screen.index==NONE);
    CHECK(mac_native_tags_group(&assets->tags,(uint32_t)sky->model.index)==MODE);
    model=(struct model *)mac_native_tags_get(&assets->tags,MODE,(uint32_t)sky->model.index,sizeof(*model),&error);
    CHECK(model!=NULL&&model->nodes.count>0);
    if(sky->animation_graph.index!=NONE) {
        CHECK(mac_native_tags_group(&assets->tags,(uint32_t)sky->animation_graph.index)==ANTR);
        graph=(struct animation_graph *)mac_native_tags_get(&assets->tags,ANTR,(uint32_t)sky->animation_graph.index,sizeof(*graph),&error);
        CHECK(graph!=NULL&&model&&(!graph->nodes.count||graph->nodes.count==model->nodes.count));
    } else CHECK(sky->animations.count==0);
    lights=sky->lights.address;
    for(i=0;i<(uint32_t)sky->lights.count;++i)if(lights[i].lens.index!=NONE) {
        struct probe_lens_root *lens;struct bitmap_group *bitmap;
        uint32_t lens_handle=(uint32_t)lights[i].lens.index,map_handle;
        CHECK(mac_native_tags_group(&assets->tags,lens_handle)==LENS);
        lens=(struct probe_lens_root *)mac_native_tags_get(&assets->tags,LENS,lens_handle,sizeof(*lens),&error);
        CHECK(lens!=NULL);if(!lens)continue;map_handle=(uint32_t)lens->primary_map.index;
        CHECK(mac_native_tags_group(&assets->tags,map_handle)==BITM);
        bitmap=(struct bitmap_group *)mac_native_tags_get(&assets->tags,BITM,map_handle,sizeof(*bitmap),&error);
        CHECK(bitmap!=NULL&&bitmap->bitmaps.count>0);
    }
    printf("sky assets runtime: checks=%u failures=%u sky=%08lx model=%08lx lights=%ld published=%lu\n",
        checks,failures,(unsigned long)handle,(unsigned long)sky->model.index,sky->lights.count,
        (unsigned long)assets->published_count);
    return failures==0;
}
