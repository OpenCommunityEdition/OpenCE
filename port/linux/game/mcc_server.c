#include "cseries.h"
#include "mcc_server.h"
#include "mcc_package.h"
#include "mcc_cache_format.h"
#include "mcc_maps.h"
#include "../src/server_rotation.h"
#include <stdio.h>
#include <string.h>

void mcc_server_resolve(struct server_entry *entry)
{
    char canonical[64],path[128];
    unsigned char header[MCC_CACHE_HEADER_BYTES];
    struct mcc_cache_identity identity;
    enum mcc_cache_status status;
    FILE *file;
    long size;
    if (!mcc_package_name(entry->map,canonical) || !mcc_package_map_path(canonical,path)) {
        snprintf(entry->rejected,sizeof(entry->rejected),"Invalid MCC map path: %s",entry->map);
        return;
    }
    strcpy(entry->level,canonical);
    strcpy(entry->map_name,strrchr(canonical,'\\')+1);
    file=fopen(path,"rb");
    if (!file) {
        entry->map_missing=1;
        snprintf(entry->rejected,sizeof(entry->rejected),"Cannot read MCC map %s",canonical);
        return;
    }
    status=MCC_CACHE_READ_FAILED;
    if (!fseek(file,0,SEEK_END) && (size=ftell(file))>=0 && !fseek(file,0,SEEK_SET) &&
        fread(header,1,sizeof(header),file)==sizeof(header))
        status=mcc_cache_identify(header,sizeof(header),(uint64_t)size,&identity);
    fclose(file);
    if (status!=MCC_CACHE_OK)
        snprintf(entry->rejected,sizeof(entry->rejected),"Invalid MCC map %s: %s",canonical,mcc_cache_status_string(status));
    else if ((identity.scenario_type==0)!=(!strcmp(entry->gametype,"coop")))
        snprintf(entry->rejected,sizeof(entry->rejected),"%s requires %s",canonical,
            identity.scenario_type==0 ? "the coop gametype" : "a multiplayer gametype");
}

int mcc_server_maps_each(void (*each)(void *, const char *, const char *, const char *),void *context)
{
    short row,count,index;
    /* Read the cached catalog; an HTTP refresh must not rescan mid-game. */
    count=mcc_maps_count(FALSE);
    for (row=0;row<count;row++) {
        char const *level;
        index=mcc_maps_index(row,FALSE);
        level=mcc_maps_level_name(index);
        if (level) each(context,level+9,level,"mcc");
    }
    return count;
}
