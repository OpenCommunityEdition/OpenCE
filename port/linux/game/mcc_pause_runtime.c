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
    long handle, asset;
    boolean borrowed;
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
long hud_hires_asset_count(void);
char const *hud_hires_asset_tag(long asset);
void hud_hires_register_placeholder(long asset, unsigned long const *texture);
boolean pc_menu_tag(long tag);
int config_boolean(char const *name);

/* The native menu loader owns these redraw textures until after MCC pause
 * teardown. Copy descriptors and select our own frame; never alter its group,
 * sequence, texture or ownership. */
static boolean mcc_pause_borrow_art(struct mcc_pause_art *art)
{
    static char const *names[] = {"pc\\pause\\pausebox_left", "pc\\pause\\pausebox_center", "pc\\pause\\pausebox_right",
        "pc\\pause\\objectives_left", "pc\\pause\\objectives_center", "pc\\pause\\objectives_right"};
    char const *name;
    long tag, frame = 0, count = 1;
    struct bitmap_group const *group;
    if (art->kind == MCC_PAUSE_ART_ALERT) name = "pc\\bitmaps\\alert_bkd";
    else if (art->kind >= MCC_PAUSE_ART_FOOTER_LEFT && art->kind <= MCC_PAUSE_ART_OBJECTIVES_RIGHT) {
        name = names[art->kind - MCC_PAUSE_ART_FOOTER_LEFT];
        if (art->kind >= MCC_PAUSE_ART_OBJECTIVES_LEFT) frame = art->height == 225 ? 1 : 0;
        else frame = art->height == 194 ? 0 : art->height == 229 ? 1 : art->height == 195 ? 2 : 3;
    } else return FALSE;
    tag = tag_loaded('bitm', name);
    if (tag == NONE || !pc_menu_tag(tag)) return FALSE;
    group = tag_get('bitm', tag);
    if (!group || !group->bitmaps.address || frame < 0 || frame >= group->bitmaps.count) return FALSE;
    art->frames[0] = ((struct bitmap_data const *)group->bitmaps.address)[frame];
    art->group.bitmaps.count = count;
    art->sequence.bitmap_count = (short)count;
    art->borrowed = TRUE;
    return TRUE;
}


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

/* Antialias the stock two-unit outline at the logical resolution. The
 * Xbox highlight has a dark translucent fill and a ten-unit corner radius. */
static unsigned long mcc_pause_highlight_pixel(short x, short y, short width, short height)
{
    long sx, sy, alpha = 0, red = 0, green = 0, blue = 0;
    for (sy = 0; sy < 8; sy++) for (sx = 0; sx < 8; sx++) {
        long px = x * 16 + sx * 2 + 1, py = y * 16 + sy * 2 + 1;
        long dx = px < 160 ? 160 - px : px > width * 16 - 160 ? px - (width * 16 - 160) : 0;
        long dy = py < 160 ? 160 - py : py > height * 16 - 160 ? py - (height * 16 - 160) : 0;
        boolean inside = dx * dx + dy * dy <= 160 * 160;
        boolean fill = px >= 32 && py >= 32 && px < (width - 2) * 16 && py < (height - 2) * 16 &&
            dx * dx + dy * dy <= 128 * 128;
        if (!inside) continue;
        alpha += fill ? 187 : 255;
        red += fill ? 0 : 41 * 255;
        green += fill ? 13 * 187 : 149 * 255;
        blue += fill ? 32 * 187 : 255 * 255;
    }
    return alpha ? ((unsigned long)(alpha / 64) << 24) | ((red / alpha) << 16) |
        ((green / alpha) << 8) | (blue / alpha) : 0;
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
    art->width=width;art->height=height;art->kind=kind;art->asset=NONE;
    art->handle=(0x5A00UL<<16)|(pause_runtime.original_count+pause_runtime.art_count-1);
    snprintf(art->name,sizeof(art->name),"mcc_pause\\art_%d_%dx%d",(int)kind,(int)width,(int)height);
    art->sequence.bitmap_count=kind==MCC_PAUSE_ART_HIGHLIGHT?2:1;
    art->group.sequences.count=1;art->group.sequences.address=&art->sequence;
    art->group.bitmaps.count=art->sequence.bitmap_count;art->group.bitmaps.address=art->frames;
    if (kind == MCC_PAUSE_ART_ALERT || kind >= MCC_PAUSE_ART_FOOTER_LEFT) {
        if (!mcc_pause_borrow_art(art)) goto failed;
        return art->handle;
    }
    if (kind == MCC_PAUSE_ART_BUTTON_A || kind == MCC_PAUSE_ART_BUTTON_B) {
        char const *name = kind == MCC_PAUSE_ART_BUTTON_A ?
            "ui\\shell\\bitmaps\\a_butn_sm" : "ui\\shell\\bitmaps\\b_butn_sm";
        for (i = 0; i < hud_hires_asset_count(); i++)
            if (!strcmp(hud_hires_asset_tag(i), name)) { art->asset = i; break; }
        if (art->asset == NONE) goto failed;
    }
    for(i=0;i<art->sequence.bitmap_count;i++) {
        struct bitmap_data *bitmap=&art->frames[i];
        unsigned long *pixels;
        short x,y;
        bitmap->signature='bitm';bitmap->width=kind==MCC_PAUSE_ART_DIM?4:mcc_pause_power_two(width);
        bitmap->height=kind==MCC_PAUSE_ART_DIM?4:mcc_pause_power_two(height);bitmap->depth=1;
        if (art->asset != NONE) bitmap->width = bitmap->height = 32;
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
            else if((kind==MCC_PAUSE_ART_PANEL || kind==MCC_PAUSE_ART_FOOTER_PANEL ||
                kind==MCC_PAUSE_ART_OBJECTIVES_PANEL || kind==MCC_PAUSE_ART_CAMPAIGN_PANEL) && mcc_pause_inside(x,y,width,height,0))
                color=mcc_pause_inside(x,y,width,height,2)?
                    0xC00A2649UL:0xFF2896FFUL;
            /* The Xbox campaign body fades from (10,38,73) to (5,13,28).
             * Scale only the body, leaving its header, footer and outline intact. */
            if(color==0xC00A2649UL && (kind==MCC_PAUSE_ART_FOOTER_PANEL ||
                kind==MCC_PAUSE_ART_OBJECTIVES_PANEL || kind==MCC_PAUSE_ART_CAMPAIGN_PANEL)) {
                long top=kind==MCC_PAUSE_ART_OBJECTIVES_PANEL?29:2;
                long bottom=kind==MCC_PAUSE_ART_FOOTER_PANEL?height-28:height-2;
                long span=bottom-top, position=y-top;
                if(span>0) {
                    if(position<0) position=0;
                    if(position>span) position=span;
                    color=0xC0000000UL | ((10-5*position/span)<<16) |
                        ((38-25*position/span)<<8) | (73-45*position/span);
                }
            }
            /* Keep the stock 28-pixel bands inside the rounded outline. */
            if(mcc_pause_inside(x,y,width,height,2)) {
                if(kind==MCC_PAUSE_ART_OBJECTIVES_PANEL && y<=28)
                    color=y==28?0xFF2896FFUL:0xFF000000UL;
                else if(kind==MCC_PAUSE_ART_FOOTER_PANEL && y>=height-28)
                    /* The Xbox footer is black with alpha 191, replacing the body. */
                    color=y==height-28?0xFF2896FFUL:0xBF000000UL;
            }
            if (kind == MCC_PAUSE_ART_HIGHLIGHT && i == 1)
                color = mcc_pause_highlight_pixel(x, y, width, height);
            pixels[y*bitmap->width+x]=color;
        }
        if(!rasterizer_bitmap_new(bitmap)) goto failed;
        rasterizer_bitmap_changed(bitmap);
        if (art->asset != NONE) hud_hires_register_placeholder(art->asset, bitmap->hardware_format);
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
        if (pause_runtime.art[i].borrowed) continue;
        if (pause_runtime.art[i].asset != NONE && j == 0)
            hud_hires_register_placeholder(pause_runtime.art[i].asset, NULL);
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
    assets.settings=TRUE;
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
    mcc_pause_friendly_fire(config_boolean("network.coop_own_friendly_fire"));
    return mcc_pause_select(global_scenario->type==0,network,host,teams,local_count,first_player);
}
