/* MCC-owned stock-style pause resources. The map's widgets, HUD and objective
 * tags are never overwritten. A copied tag table is published only once the
 * complete menu and its independently allocated art are ready. */
#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "networking/network_game_globals.h"
#include "rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"
#include "mcc_cache.h"
#include "mcc_pause.h"
#include "mcc_pause_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MCC_PAUSE_MAX_ART 64
struct mcc_pause_instance {
    long group, parents[2], handle;
    char *name;
    void *data;
    unsigned long unused[2];
};
typedef char mcc_pause_instance_size[sizeof(struct mcc_pause_instance)==32?1:-1];
struct mcc_pause_art {
    short width, height;
    enum mcc_pause_art_kind kind;
    long handle;
    char name[64];
    struct bitmap_group group;
    struct bitmap_group_sequence sequence;
    struct bitmap_data frames[2];
};
static struct {
    struct mcc_pause_instance *original, *table;
    long original_count, art_count;
    struct mcc_pause_art art[MCC_PAUSE_MAX_ART];
    boolean failed;
} pause_runtime;

void *cache_files_tag_instances(long *count);
void cache_files_set_tag_instances(void *instances, long count);
char const *config_string(char const *name);
void platform_log(char const *format, ...);
void xgpu_mcc_texture_unbind(unsigned long const *resource);

static short mcc_pause_power_two(short value)
{
    short size=1;
    while(size<value) size*=2;
    return size;
}

/* Rounded stock-blue outlines in logical UI pixels. Transparent texture
 * padding permits the native widget renderer's ordinary texel clipping. */
static boolean mcc_pause_inside(short x, short y, short w, short h, short inset)
{
    long dx,dy,r=7-inset;
    if(x<inset || y<inset || x>=w-inset || y>=h-inset) return FALSE;
    if(r<=0) return TRUE;
    dx=x<7 ? 7-x : x>=w-7 ? x-(w-8) : 0;
    dy=y<7 ? 7-y : y>=h-7 ? y-(h-8) : 0;
    return !dx || !dy || dx*dx+dy*dy<=r*r;
}

static long mcc_pause_art_add(short width, short height, enum mcc_pause_art_kind kind)
{
    struct mcc_pause_art *art;
    long i;
    if(width<=0 || height<=0 || width>1024 || height>512) goto failed;
    for(i=0;i<pause_runtime.art_count;i++) {
        art=&pause_runtime.art[i];
        if(art->width==width && art->height==height && art->kind==kind) return art->handle;
    }
    if(pause_runtime.art_count==MCC_PAUSE_MAX_ART) goto failed;
    art=&pause_runtime.art[pause_runtime.art_count++];
    art->width=width;art->height=height;art->kind=kind;
    art->handle=(0x5A00UL<<16)|(pause_runtime.original_count+pause_runtime.art_count-1);
    snprintf(art->name,sizeof(art->name),"mcc_pause\\art_%d_%dx%d",(int)kind,(int)width,(int)height);
    art->sequence.bitmap_count=kind==MCC_PAUSE_ART_HIGHLIGHT?2:1;
    art->group.sequences.count=1;art->group.sequences.address=&art->sequence;
    art->group.bitmaps.count=art->sequence.bitmap_count;art->group.bitmaps.address=art->frames;
    for(i=0;i<art->sequence.bitmap_count;i++) {
        struct bitmap_data *bitmap=&art->frames[i];
        unsigned long *pixels;
        short x,y;
        bitmap->signature='bitm';bitmap->width=kind==MCC_PAUSE_ART_DIM?4:mcc_pause_power_two(width);
        bitmap->height=kind==MCC_PAUSE_ART_DIM?4:mcc_pause_power_two(height);bitmap->depth=1;
        bitmap->format=11;bitmap->flags=1;bitmap->tag_index=art->handle;
        bitmap->cache_block_index=NONE;
        bitmap->pixels_size=bitmap->width*bitmap->height*4;
        bitmap->base_address=malloc(bitmap->pixels_size);
        if(!bitmap->base_address) goto failed;
        memset(bitmap->base_address,0,bitmap->pixels_size);
        pixels=bitmap->base_address;
        for(y=0;y<(kind==MCC_PAUSE_ART_DIM?4:height);y++) for(x=0;x<(kind==MCC_PAUSE_ART_DIM?4:width);x++) {
            unsigned long color=0;
            if(kind==MCC_PAUSE_ART_DIM) color=0x80000000UL;
            else if((kind==MCC_PAUSE_ART_PANEL || i==1) && mcc_pause_inside(x,y,width,height,0))
                color=mcc_pause_inside(x,y,width,height,2)?
                    (kind==MCC_PAUSE_ART_PANEL?0xC00A2649UL:0x60204A75UL):0xFF2896FFUL;
            pixels[y*bitmap->width+x]=color;
        }
        if(!rasterizer_bitmap_new(bitmap)) goto failed;
        rasterizer_bitmap_changed(bitmap);
    }
    return art->handle;
failed:
    pause_runtime.failed=TRUE;
    return NONE;
}

void mcc_pause_runtime_unload(void)
{
    long i,j;
    extern void mcc_ui_settings_close(short);
    mcc_ui_settings_close(NONE);
    if(pause_runtime.table) {
        cache_files_set_tag_instances(pause_runtime.original,pause_runtime.original_count);
        free(pause_runtime.table);
    }
    mcc_pause_dispose();
    for(i=0;i<pause_runtime.art_count;i++) for(j=0;j<2;j++) {
        struct bitmap_data *bitmap=&pause_runtime.art[i].frames[j];
        if(bitmap->hardware_format) {
            /* A normal frame end clears texture stages, but partial load
             * cleanup must also be safe before the next frame completes. */
            xgpu_mcc_texture_unbind(bitmap->hardware_format);
            rasterizer_bitmap_delete(bitmap);
        }
        if(bitmap->base_address) free(bitmap->base_address);
    }
    memset(&pause_runtime,0,sizeof(pause_runtime));
}

boolean mcc_pause_runtime_load(void)
{
    struct mcc_pause_assets assets;
    long i,total,font,small;
    if(!mcc_cache_tags_loaded()) return FALSE;
    if(pause_runtime.table) return TRUE;
    pause_runtime.original=cache_files_tag_instances(&pause_runtime.original_count);
    if(!pause_runtime.original || pause_runtime.original_count<0 || pause_runtime.original_count>63000) goto failed;
    memset(&assets,0,sizeof(assets));
    font=tag_loaded('font',"ui\\large_ui");small=tag_loaded('font',"ui\\small_ui");
    if(font==NONE) font=small;
    if(small==NONE) small=font;
    if(font==NONE) {
        for(i=0;i<pause_runtime.original_count;i++) if(pause_runtime.original[i].group=='font') {
            font=small=pause_runtime.original[i].handle;
            break;
        }
    }
    if(font==NONE) goto failed;
    for(i=0;i<MCC_PAUSE_LAYOUT_COUNT;i++) {
        assets.font[i]=i==MCC_PAUSE_QUARTER?small:font;
        assets.small_font[i]=small;
    }
    assets.art=mcc_pause_art_add;
    assets.settings=!strcmp(config_string("display.menus"),"pc");
    /* Discover the bounded, deduplicated art inventory before assigning the
     * final widget indices. Neither pass publishes partial definitions. */
    if(!mcc_pause_build(pause_runtime.original_count+MCC_PAUSE_MAX_ART,0x6000,&assets) || pause_runtime.failed) goto failed;
    mcc_pause_dispose();
    if(!mcc_pause_build(pause_runtime.original_count+pause_runtime.art_count,0x6000,&assets) || pause_runtime.failed) goto failed;
    total=pause_runtime.original_count+pause_runtime.art_count+mcc_pause_tag_count();
    if(total>=65535) goto failed;
    pause_runtime.table=malloc(total*sizeof(*pause_runtime.table));
    if(!pause_runtime.table) goto failed;
    memcpy(pause_runtime.table,pause_runtime.original,pause_runtime.original_count*sizeof(*pause_runtime.table));
    for(i=pause_runtime.original_count;i<total;i++) {
        struct mcc_pause_instance *entry=&pause_runtime.table[i];
        long ordinal=i-pause_runtime.original_count;
        memset(entry,0,sizeof(*entry));
        entry->parents[0]=entry->parents[1]=NONE;
        if(ordinal<pause_runtime.art_count) {
            struct mcc_pause_art *art=&pause_runtime.art[ordinal];
            entry->group='bitm';entry->handle=art->handle;entry->name=art->name;entry->data=&art->group;
        } else {
            struct mcc_pause_tag const *tag=mcc_pause_tag_at(ordinal-pause_runtime.art_count);
            if(!tag) goto failed;
            entry->group=tag->group;entry->handle=tag->handle;entry->name=(char *)tag->name;entry->data=tag->data;
        }
    }
    cache_files_set_tag_instances(pause_runtime.table,total);
    platform_log("mcc: native pause menus added (%ld widgets/strings, %ld art groups)",mcc_pause_tag_count(),pause_runtime.art_count);
    return TRUE;
failed:
    platform_log("mcc: native pause menus could not be built");
    mcc_pause_runtime_unload();
    return FALSE;
}

boolean mcc_pause_runtime_active(void) {return mcc_cache_tags_loaded();}

long mcc_pause_runtime_screen(short local_count, boolean first_player)
{
    boolean network,host,teams=FALSE;
    if(!mcc_cache_tags_loaded() || !pause_runtime.table || !global_scenario) return NONE;
    network=game_connection()==_game_connection_network_server || game_connection()==_game_connection_network_client;
    host=global_network_game_server_get()!=NULL;
    if(global_scenario->type==1) teams=game_engine_get_variant()->universal_variant.teams;
    return mcc_pause_select(global_scenario->type==0,network,host,teams,local_count,first_player);
}
