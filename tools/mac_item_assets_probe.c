#include "cseries.h"
#include "items/item_definitions.h"
#include "items/equipment_definitions.h"
#include "native_actor_assets.h"
#include "mapped_file.h"
#undef free
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);}}while(0)
int main(int argc,char **argv)
{
    struct mac_mapped_file map={0},sounds={0},bitmaps={0};struct mac_retail_cache cache;
    struct mac_actor_assets assets={0};struct mac_retail_tag tag;const char *error;
    uint32_t i,collections=0,equipment=0,cursor=0,handle;size_t before;int ok=0,equipment_only,resources;
    if(argc!=3 && argc!=4)return 2;
    equipment_only=argc==4&&!strcmp(argv[3],"--equipment-only");resources=argc==4&&!equipment_only;
    if(resources&&!mac_mapped_file_open(argv[3],0x30000000,&bitmaps,&error))goto done;
    if(!mac_mapped_file_open(argv[1],0x30000000,&map,&error)||
       !mac_mapped_file_open(argv[2],0x30000000,&sounds,&error)||
       !mac_retail_open(map.data,map.size,&cache,&error)||
       !mac_native_tags_create(&cache,&assets.tags,&error))goto done;
    for(i=0;i<cache.tag_count;++i) {
        int loaded;
        if(!mac_retail_tag_get(&cache,i,&tag))goto done;
        if(tag.group==UINT32_C(0x69746d63)) {
            if(equipment_only)continue;
            loaded=(resources?mac_actor_assets_load_item_collection_with_resources(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,tag.handle,&error):mac_actor_assets_load_item_collection_with_sounds(&cache,sounds.data,sounds.size,&assets,tag.handle,&error));
            ++collections;
        } else if(tag.group==UINT32_C(0x65716970)) {
            loaded=mac_actor_assets_load_equipment_with_sounds(&cache,sounds.data,sounds.size,&assets,tag.handle,&error);
            ++equipment;
        } else continue;
        if(!loaded){fprintf(stderr,"asset %08lx %s: %s\n",(unsigned long)tag.handle,tag.name,error?error:"failed");goto done;}
        CHECK(mac_native_tags_group(&assets.tags,tag.handle)==tag.group);
        before=mac_native_tags_owned_bytes(&assets.tags);
        if(tag.group==UINT32_C(0x69746d63))CHECK((resources?mac_actor_assets_load_item_collection_with_resources(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,tag.handle,&error):mac_actor_assets_load_item_collection_with_sounds(&cache,sounds.data,sounds.size,&assets,tag.handle,&error)));
        else CHECK(mac_actor_assets_load_equipment_with_sounds(&cache,sounds.data,sounds.size,&assets,tag.handle,&error));
        CHECK(mac_native_tags_owned_bytes(&assets.tags)==before);
    }
    mac_mapped_file_close(&map);mac_mapped_file_close(&sounds);mac_mapped_file_close(&bitmaps);
    while((handle=mac_native_tags_next(&assets.tags,UINT32_C(0x69746d63),&cursor))!=UINT32_MAX) {
        const struct item_collection_definition *collection=mac_native_tags_get(&assets.tags,UINT32_C(0x69746d63),handle,sizeof(*collection),&error);
        const struct item_permutation_definition *permutations;long j;
        CHECK(collection!=NULL);if(!collection)continue;permutations=collection->permutations.address;
        for(j=0;j<collection->permutations.count;++j) {
            const struct item_definition *item=mac_native_tags_get(&assets.tags,UINT32_C(0x6974656d),(uint32_t)permutations[j].item.index,sizeof(*item),&error);
            CHECK(item!=NULL);if(item)CHECK(item->object.type==2||item->object.type==3);
        }
    }
    CHECK(collections==(equipment_only?0:14)&&equipment==14);
    printf("native item assets: %u checks, %u failures; collections=%lu equipment=%lu published=%lu bytes=%lu\n",
        checks,failures,(unsigned long)collections,(unsigned long)equipment,(unsigned long)assets.published_count,
        (unsigned long)mac_native_tags_owned_bytes(&assets.tags));ok=!failures;
done:
    if(!ok)fprintf(stderr,"item asset closure incomplete: %s\n",error?error:"test failure");
    mac_actor_assets_free(&assets);mac_mapped_file_close(&map);mac_mapped_file_close(&sounds);mac_mapped_file_close(&bitmaps);
    return ok?0:1;
}
