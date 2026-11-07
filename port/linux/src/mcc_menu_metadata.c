#include "mcc_menu_metadata.h"
#include "mcc_manifest.h"
#include "posix.h"
#include "server_json.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int same(const char *a, const char *b)
{
    while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
    return !*a && !*b;
}

const struct steam_workshop_map_text *mcc_menu_metadata_find(const struct mcc_menu_metadata *metadata, const char *name)
{
    int i;
    const struct steam_workshop_map_text *found=NULL;
    for (i=0; i<metadata->count; i++) if (same(name,metadata->maps[i].name)) {
        if (found) return NULL;
        found=&metadata->maps[i];
    }
    return found;
}

/* Read at most one package/family at a time. The catalog calls this once per
 * package, not once per level. Every path uses the same contained reader as
 * images, including metadata that names another metadata file. */
int mcc_menu_metadata_load(const char *root, int campaign, struct mcc_menu_metadata *metadata)
{
    struct steam_workshop_item_text *item;
    struct server_json_document document;
    char *text;
    int length, count, index, valid=0;
    memset(metadata,0,sizeof(*metadata));
    text=malloc(STEAM_WORKSHOP_MAXIMUM_JSON);
    if (!text) return 0;
    /* A full collection is too large for the native/guest thread stacks. Reuse
     * its path buffer for campaigns instead of keeping a second copy. */
    item=malloc(sizeof(*item));
    if (!item) goto done;
    length=posix_mcc_preview_read(root,"ModInfo.json",text,STEAM_WORKSHOP_MAXIMUM_JSON);
    if (!steam_workshop_read_item(text,length,item) || !item->halo1) goto done;
    strcpy(metadata->title,item->title);
    count=item->map_count;
    if (campaign) {
        int token;
        if (!item->has_campaign) goto done;
        length=posix_mcc_preview_read(root,"CampaignInfo.json",text,STEAM_WORKSHOP_MAXIMUM_JSON);
        if (!server_json_parse(&document,text,length,16)) goto done;
        token=server_json_member(&document,0,"CampaignMaps");
        count=token>=0 && document.tokens[token].type==_server_json_array ? document.tokens[token].size : 0;
        if (count<1 || count>MCC_MANIFEST_MAXIMUM_MISSIONS) count=0;
        for (index=0; index<count; index++) if (!server_json_get_string(&document,server_json_element(&document,token,index),item->map_json[index],sizeof(item->map_json[index]))) break;
        server_json_document_free(&document);
        if (!count || index!=count) goto done;
    }
    for (index=0; index<count; index++) {
        struct steam_workshop_map_text *map=&metadata->maps[metadata->count];
        length=posix_mcc_preview_read(root,item->map_json[index],text,STEAM_WORKSHOP_MAXIMUM_JSON);
        if (!steam_workshop_read_map(text,length,map)) continue;
        if (campaign) {
            int token, supported; char kind[32];
            if (!server_json_parse(&document,text,length,16)) continue;
            token=server_json_member(&document,0,"CampaignMapKind");
            supported=token<0 || (server_json_get_string(&document,token,kind,sizeof(kind)) && mcc_manifest_default_kind(kind));
            server_json_document_free(&document);
            if (!supported) continue;
        }
        metadata->count++;
    }
    valid=1;
done:
    free(item); free(text); return valid;
}
