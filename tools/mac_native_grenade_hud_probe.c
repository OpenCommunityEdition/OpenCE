#include "cseries.h"
#include "native_interface_assets.h"
#include "native_grenade_hud_layout.h"
#include "native_object_tags.h"
#include "native_bitmap_tags.h"
#include "mapped_file.h"
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

static unsigned checks,failures,grenade_roots,grenade_references;
#define CHECK(x) do {++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static uint16_t read16(const unsigned char *p)
{ return (uint16_t)(p[0]|((uint16_t)p[1]<<8)); }
struct reference_context { struct mac_actor_assets *assets; };
static int check_reference(void *opaque,uint32_t group,uint32_t handle)
{
    struct reference_context *context=(struct reference_context *)opaque;
    CHECK(mac_native_tags_group(&context->assets->tags,handle)==group);
    ++grenade_references;return failures==0;
}
int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *map;struct mac_retail_cache cache;
    struct mac_mapped_file bitmaps={0},sounds={0};struct mac_actor_assets assets={0};
    struct reference_context refs;const char *error=NULL;uint32_t i,handle,cursor;
    unsigned bitmap_count=0;
    if(argc!=4||!(file=fopen(argv[1],"rb")))return 2;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||fseek(file,0,SEEK_SET))return 2;
    map=(unsigned char *)malloc((size_t)length);
    if(!map||fread(map,1,(size_t)length,file)!=(size_t)length)return 2;
    fclose(file);
    if(!mac_retail_open(map,(size_t)length,&cache,&error)||
       !mac_mapped_file_open(argv[2],(size_t)1<<31,&bitmaps,&error)||
       !mac_mapped_file_open(argv[3],(size_t)1<<31,&sounds,&error)||
       !mac_native_tags_create(&cache,&assets.tags,&error)) {
        fprintf(stderr,"open: %s\n",error?error:"invalid inputs");return 2;
    }
    CHECK(mac_native_interface_assets_load(&cache,bitmaps.data,bitmaps.size,
        sounds.data,sounds.size,&assets,&error));
    if(error)fprintf(stderr,"interface closure: %s\n",error);
    mac_mapped_file_close(&bitmaps);mac_mapped_file_close(&sounds);
    refs.assets=&assets;
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag metadata;const unsigned char *disk;
        struct grenade_hud_interface_definition *hud;
        struct mac_native_object_tag view;const void *root;
        uint32_t corner;uint16_t flash_cutoff;
        if(!mac_retail_tag_get(&cache,i,&metadata)||metadata.group!=UINT32_C(0x67726869))continue;
        ++grenade_roots;
        root=mac_native_tags_get(&assets.tags,metadata.group,metadata.handle,
            sizeof(*hud),&error);CHECK(root!=NULL);if(!root)continue;
        hud=(struct grenade_hud_interface_definition *)root;
        disk=mac_retail_tag_span(&cache,metadata.address,sizeof(*hud));CHECK(disk!=NULL);if(!disk)continue;
        corner=read16(disk+offsetof(struct grenade_hud_interface_definition,absolute_placement.corner));
        flash_cutoff=read16(disk+offsetof(struct grenade_hud_interface_definition,grenade_count_panel.flash_cutoff));
        CHECK((uint16_t)hud->absolute_placement.corner==(uint16_t)corner);
        CHECK((uint16_t)hud->grenade_count_panel.flash_cutoff==flash_cutoff);
        CHECK(hud->absolute_placement.corner>=0&&hud->absolute_placement.corner<NUMBER_OF_HUD_ANCHORS);
        CHECK(hud->grenade_count_panel.numbers.digits==disk[
            offsetof(struct grenade_hud_interface_definition,grenade_count_panel.numbers.digits)]);
        CHECK((uint8_t)hud->grenade_count_panel.numbers.number_flags==disk[
            offsetof(struct grenade_hud_interface_definition,grenade_count_panel.numbers.number_flags)]);
        CHECK((uint8_t)hud->grenade_count_panel.numbers.fractional_digits==disk[
            offsetof(struct grenade_hud_interface_definition,grenade_count_panel.numbers.fractional_digits)]);
        memset(&view,0,sizeof(view));view.root=hud;view.handle=metadata.handle;
        view.group=metadata.group;view.root_bytes=sizeof(*hud);
        CHECK(mac_native_object_visit_references(&view,check_reference,&refs));
    }
    CHECK(grenade_roots>0);
    cursor=0;
    while((handle=mac_native_tags_next(&assets.tags,UINT32_C(0x6269746d),&cursor))!=UINT32_MAX)++bitmap_count;
    CHECK(bitmap_count>0&&grenade_references>0);
    printf("native grenade HUD roots=%u bitmaps=%u references=%u checks=%u failures=%u\n",
        grenade_roots,bitmap_count,grenade_references,checks,failures);
    free(map);mac_actor_assets_free(&assets);
    return failures?1:0;
}
