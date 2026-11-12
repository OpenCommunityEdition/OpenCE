/* Local and Workshop packages share one policy and source planner. */
#include "cseries.h"
#include "cseries/errors.h"
#include "mcc_share_package.h"
#include "mcc_share_session.h"
#include "mcc_identity.h"
#include "mcc_package.h"
#include "mcc_workshop.h"
#include "mcc_validation.h"
#include "../src/mcc_manifest.h"
#include "../src/mcc_share_metadata.h"
#include "../src/posix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int config_boolean(char const *name);
void platform_translate_path(char const *name, char *path, unsigned long size);

/* Preserve actual disk spelling on case-sensitive systems. Never follow a
 * metadata-supplied link, even if the bytes would happen to parse. */
static int source_path(char const *root, char const *relative, char path[1024])
{
    char part[128],found[256],next[1024];
    char const *p=relative;
    if (strlen(root)>=1024) return 0;
    strcpy(path,root);
    while (*p) {
        size_t n=strcspn(p,"/\\");
        if (!n || n>=sizeof(part)) return 0;
        memcpy(part,p,n); part[n]=0;
        if (!strcmp(part,".") || !strcmp(part,"..") || strchr(part,':') ||
            !posix_find_entry_case_insensitive(path,part,found,sizeof(found)) ||
            snprintf(next,sizeof(next),"%s/%s",path,found)>=(int)sizeof(next)) return 0;
        strcpy(path,next);
        p+=n; if (*p) p++;
    }
    return posix_mcc_share_path_safe(path);
}

static int add_file(struct mcc_share_host_package *package, unsigned kind,
    char const *leaf, char const *path)
{
    int i=package->count;
    if (i>=MCC_SHARE_STAGE_FILES || strlen(leaf)>=MCC_SHARE_LEAF_SIZE ||
        strlen(path)>=sizeof(package->files[i].path)) return 0;
    package->files[i].kind=(uint8_t)kind;
    strcpy(package->files[i].leaf,leaf);
    strcpy(package->files[i].path,path);
    package->count++;
    return 1;
}

void mcc_share_package_build(char const *level, struct mcc_share_host_package *package)
{
    static char list[65537];
    static char const *credits[]={"credits.txt","readme.txt","license.txt"};
    struct mcc_share_metadata_plan *plan;
    struct mcc_workshop_item item;
    char canonical[64],map[128],path[1024],root[1024],maps[1024],reason[128],hex[65];
    char resources[2][128];
    char const *leaf;
    char *end;
    unsigned char digest[32];
    FILE *file;
    size_t length=0;
    unsigned i;
    memset(package,0,sizeof(*package));
    package->refusal=MCC_SHARE_RESULT_NOT_SHAREABLE;
    if (!mcc_package_name(level,canonical) || !mcc_identity_host_package(canonical,digest)) return;
    leaf=strrchr(canonical,'\\')+1;
    if (!strchr(canonical+9,'\\') || !mcc_package_map_path(canonical,map) ||
        !mcc_manifest_package_root(canonical,root)) return;
    platform_translate_path(map,maps,sizeof(maps));
    for (end=maps;*end;end++) if (*end=='\\') *end='/';
    end=strrchr(maps,'/'); if (!end) return; *end=0;
    file=fopen(MCC_SHARE_LIST_FILE,"rb");
    if (file) {
        length=fread(list,1,sizeof(list),file);
        if (ferror(file) || length==sizeof(list)) length=0;
        fclose(file);
    }
    package->refusal=mcc_share_admit(canonical,config_boolean("network.mcc_map_share"),
        mcc_workshop_local_item(canonical,&item) ? MCC_SHARE_SIDECAR_VALID : MCC_SHARE_SIDECAR_ABSENT,
        digest,1,list,(unsigned long)length,package->credit);
    if (package->refusal!=MCC_SHARE_RESULT_OK) {
        mcc_share_list_hex(digest,hex);
        error(_error_silent,"mcc share: not offering %s (reason %d); list fingerprint %s with its maker in mcc_share.txt",
            canonical,(int)package->refusal,hex);
        return;
    }
    package->refusal=MCC_SHARE_RESULT_UNAVAILABLE;
    if (!mcc_validation_resources(canonical,resources)) {
        error(_error_silent,"mcc share: %s no longer has validated resource selections; select the map again",canonical);
        return;
    }
    snprintf(map,sizeof(map),"%s.map",leaf);
    if (!source_path(maps,map,path) || !add_file(package,MCC_SHARE_FILE_MAP,map,path)) {
        error(_error_silent,"mcc share: cannot safely read %s/%s",maps,map);
        return;
    }
    for (i=1;i<=2;i++) {
        char const *resource=i==1 ? "bitmaps.map" : "sounds.map";
        char expected[128];
        if (!resources[i-1][0]) continue;
        /* Share the validated package selection, never unrelated siblings or
         * a stock resource selected from the MCC root/shared installation. */
        if (!mcc_package_resource_path(canonical,i,expected) || csstrcasecmp(expected,resources[i-1])) {
            error(_error_silent,"mcc share: %s requires %s; only package-owned resources can be shared",
                canonical,resources[i-1]);
            return;
        }
        /* The platform resolves Workshop halo1/maps fallbacks exactly as the
         * resource reader did. The beside-level directory alone is insufficient. */
        platform_translate_path(resources[i-1],path,sizeof(path));
        if (!posix_mcc_share_path_safe(path) || !add_file(package,i,resource,path)) {
            error(_error_silent,"mcc share: cannot safely read selected resource %s",resources[i-1]);
            return;
        }
    }
    plan=malloc(sizeof(*plan));
    if (!plan) return;
    if (!mcc_share_metadata_plan(root,-1,leaf,plan,reason)) {
        error(_error_silent,"mcc share: %s metadata refused: %s",canonical,reason);
        free(plan); return;
    }
    for (i=0;i<(unsigned)plan->count;i++) {
        struct mcc_share_metadata_entry const *entry=&plan->files[i];
        unsigned kind=entry->kind==MCC_SHARE_METADATA_JSON ? MCC_SHARE_FILE_JSON :
            entry->kind==MCC_SHARE_METADATA_PNG ? MCC_SHARE_FILE_PNG : MCC_SHARE_FILE_SIDECAR;
        if (!entry->size) continue;
        if (!source_path(root,entry->path,path) || !add_file(package,kind,entry->path,path)) {
            error(_error_silent,"mcc share: cannot safely read package metadata %s/%s",root,entry->path);
            free(plan); return;
        }
    }
    free(plan);
    for (i=0;i<NUMBEROF(credits);i++)
        if (source_path(root,credits[i],path) && !add_file(package,MCC_SHARE_FILE_CREDIT,credits[i],path)) return;
    package->refusal=MCC_SHARE_RESULT_OK;
}
