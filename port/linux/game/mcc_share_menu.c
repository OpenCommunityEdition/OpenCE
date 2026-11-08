#include "cseries.h"
#include "game/game.h"
#include "networking/network_game_globals.h"
#include "mcc_maps.h"
#include "mcc_share_menu.h"
#include "mcc_share_storage.h"
#include "../src/posix.h"
#include <stdio.h>
#include <string.h>

void platform_translate_path(char const *,char *,unsigned long);
static struct mcc_share_storage_item items[MCC_SHARE_STORAGE_LIMIT];
static struct mcc_share_io storage_io;
static char maps[1024],notice[160];
static int count,selected,truncated,confirming;

static void refresh(void)
{
    count=storage_io.directory_exists(maps) ? mcc_share_storage_list(maps,&storage_io,items,NUMBEROF(items),&truncated) : 0;
    if (count<=0) selected=0;
    else if (selected>=count) selected=count-1;
    confirming=0;
}

void mcc_share_menu_open(void)
{
    mcc_share_storage_native_io(&storage_io);
    platform_translate_path("d:\\mcc_maps",maps,sizeof(maps));
    selected=0; notice[0]=0; truncated=0;
    refresh();
}

void mcc_share_menu_step(int direction)
{
    confirming=0; notice[0]=0;
    if (count>0) selected=(selected+(direction<0 ? count-1 : 1))%count;
}

int mcc_share_menu_can_remove(void)
{
    /* The main-menu scene also runs a game clock. A session can already own a
       selected download there, before gameplay begins. */
    return count>0 && (!game_in_progress() || game_time_menu_scene()) &&
        !global_network_game_client_get() && !global_network_game_server_get();
}

int mcc_share_menu_confirming(void) { return confirming; }

void mcc_share_menu_remove(void)
{
    enum mcc_share_storage_result result;
    if (!mcc_share_menu_can_remove()) { confirming=0; return; }
    if (!confirming) { confirming=1; return; }
    result=mcc_share_storage_remove(maps,&storage_io,items[selected].folder);
    snprintf(notice,sizeof(notice),"%s",result==MCC_SHARE_STORAGE_REMOVED ? "Downloaded snapshot removed." :
        result==MCC_SHARE_STORAGE_LEFTOVERS ? "Downloaded files removed. Files you added were kept." :
        "Removal stopped: content changed or a file could not be safely removed. Check the package folder.");
    refresh();
    mcc_maps_rescan();
}

void mcc_share_menu_text(wchar_t text[512])
{
    char message[512];
    unsigned i;
    if (count<=0) snprintf(message,sizeof(message),"%s\n%s",notice,
        count<0 ? "The MCC download folder could not be read." : "No managed MCC downloads. Original and manually installed packages are not listed.");
    else {
        struct mcc_share_storage_item const *item=&items[selected];
        snprintf(message,sizeof(message),"%s\n%d of %d%s\n%s\nMaker: %s\n%.1f MiB, %u files%s%s\n%s",
            notice,selected+1,count,truncated ? " (more packages exist)" : "",item->source,item->maker,
            (double)item->bytes/(1024.0*1024.0),item->files,
            item->workshop.id[0] ? "\nWorkshop: " : "",item->workshop.id,
            !mcc_share_menu_can_remove() ? "Leave the game/session before removing downloads." :
            confirming ? "Remove this downloaded snapshot? Select CONFIRM REMOVAL. Saves and original installations are kept." :
            "Host downloads are separate copies. Removing one may require another download to join its host.");
    }
    for (i=0;message[i] && i<511;i++) text[i]=message[i]=='|' ? '/' : (unsigned char)message[i];
    text[i]=0;
}
