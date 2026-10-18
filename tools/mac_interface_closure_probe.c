#include "cseries.h"
#include "native_interface_assets.h"
#include "native_actor_assets.h"
#include "native_object_tags.h"
#include "native_hud_message_layout.h"
#include "native_bitmap_tags.h"
#include "mapped_file.h"
#include "interface/hud_definitions.h"
#include "interface/unit_hud_interface_definition.h"
#include "interface/weapon_hud_interface_definition.h"
#include "text/text_group.h"
#include "text/font_group.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memset
#undef printf
#undef fprintf
#undef fopen
#undef fclose
#undef fread
#undef fseek
#undef ftell
static unsigned checks,failures,reference_count;
struct probe_hud_number_definition {
    struct tag_reference bitmap; char metrics[6]; short pad; long unused[19];
};
#define CHECK(x) do {++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static int check_reference(void *context,uint32_t group,uint32_t handle)
{
    struct mac_actor_assets *assets=context;
    CHECK(mac_native_tags_group(&assets->tags,handle)==group);
    ++reference_count;return failures==0;
}
static int check_family(struct mac_actor_assets *assets,uint32_t group,size_t size)
{
    uint32_t cursor=0,handle;const char *error=NULL;
    while((handle=mac_native_tags_next(&assets->tags,group,&cursor))!=UINT32_MAX) {
        const void *root=mac_native_tags_get(&assets->tags,group,handle,size,&error);
        struct mac_native_object_tag tag;
        CHECK(root!=NULL);if(!root)return 0;
        memset(&tag,0,sizeof(tag));tag.root=(void *)root;tag.handle=handle;
        tag.group=group;tag.root_bytes=size;
        CHECK(mac_native_object_visit_references(&tag,check_reference,assets));
    }
    return failures==0;
}
static uint32_t first_tag(struct mac_native_tags *tags,uint32_t group)
{uint32_t cursor=0;return mac_native_tags_next(tags,group,&cursor);}
static unsigned count_tags(struct mac_native_tags *tags,uint32_t group)
{uint32_t cursor=0,handle;unsigned count=0;while((handle=mac_native_tags_next(tags,group,&cursor))!=UINT32_MAX)++count;return count;}
int main(int argc,char **argv)
{
    FILE *f;long map_length;unsigned char *map;struct mac_retail_cache cache;
    struct mac_mapped_file bitmaps={0},sounds={0};struct mac_actor_assets assets={0};
    const char *error=NULL;uint32_t hudg=0,numbers=0,units=0,weapons=0,hmt=0;
    unsigned bitm=0,snd=0,lsnd=0;
    if(argc!=4||!(f=fopen(argv[1],"rb")))return 2;
    if(fseek(f,0,SEEK_END)||(map_length=ftell(f))<2048||fseek(f,0,SEEK_SET))return 2;
    map=malloc(map_length);if(!map||fread(map,1,map_length,f)!=(size_t)map_length)return 2;fclose(f);
    if(!mac_retail_open(map,map_length,&cache,&error)||
       !mac_mapped_file_open(argv[2],(size_t)1<<31,&bitmaps,&error)||
       !mac_mapped_file_open(argv[3],(size_t)1<<31,&sounds,&error)||
       !mac_native_tags_create(&cache,&assets.tags,&error)) {
        fprintf(stderr,"open: %s\n",error?error:"invalid inputs");return 2;
    }
    CHECK(mac_native_interface_assets_load(&cache,bitmaps.data,bitmaps.size,
        sounds.data,sounds.size,&assets,&error));
    if(error)fprintf(stderr,"interface closure: %s\n",error);
    mac_mapped_file_close(&bitmaps);mac_mapped_file_close(&sounds);
    free(map);map=NULL;
    if(!failures) {
        CHECK(check_family(&assets,UINT32_C(0x68756467),sizeof(struct hud_globals_definition)));
        CHECK(check_family(&assets,UINT32_C(0x68756423),sizeof(struct probe_hud_number_definition)));
        CHECK(check_family(&assets,UINT32_C(0x756e6869),sizeof(struct unit_hud_interface_definition)));
        CHECK(check_family(&assets,UINT32_C(0x77706869),sizeof(struct weapon_hud_interface_definition)));
        hudg=first_tag(&assets.tags,UINT32_C(0x68756467));
        numbers=first_tag(&assets.tags,UINT32_C(0x68756423));
        units=first_tag(&assets.tags,UINT32_C(0x756e6869));
        weapons=first_tag(&assets.tags,UINT32_C(0x77706869));
        hmt=first_tag(&assets.tags,UINT32_C(0x686d7420));
        bitm=count_tags(&assets.tags,UINT32_C(0x6269746d));
        snd=count_tags(&assets.tags,UINT32_C(0x736e6421));
        lsnd=count_tags(&assets.tags,UINT32_C(0x6c736e64));
        CHECK(hudg!=UINT32_MAX&&numbers!=UINT32_MAX&&units!=UINT32_MAX&&weapons!=UINT32_MAX);
        CHECK(hmt!=UINT32_MAX&&bitm!=UINT32_MAX);
    }
    CHECK(assets.bitmap_count>0);
    printf("native interface closure roots=%08lx/%08lx/%08lx/%08lx hmt=%08lx bitm=%u sound=%u/%u refs=%u published=%lu checks=%u failures=%u\n",
        (unsigned long)hudg,(unsigned long)numbers,(unsigned long)units,(unsigned long)weapons,
        (unsigned long)hmt,(unsigned long)assets.bitmap_count,(unsigned long)snd,(unsigned long)lsnd,
        reference_count,(unsigned long)assets.published_count,checks,failures);
    mac_actor_assets_free(&assets);mac_actor_assets_free(&assets);
    CHECK(!assets.tags.state&&!assets.bitmaps&&!assets.bitmap_count);
    return failures?1:0;
}
