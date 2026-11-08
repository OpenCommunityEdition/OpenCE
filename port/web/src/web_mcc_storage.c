/* The game runs on a pthread. Small page requests wait on a static mailbox;
 * OPFS streams themselves stay in WasmFS, never in a whole-package JS buffer. */
#include "web_mcc_storage.h"
#include "../../linux/game/mcc_share_stage.h"
#include <emscripten.h>
#include <emscripten/threading.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void platform_translate_path(char const *,char *,unsigned long);
void web_js_mcc_storage(char const *action,char const *payload,int *mailbox,int generation);
void web_js_mcc_storage_fence(void);

#ifndef WEB_MCC_STORAGE_WAIT_MS
#define WEB_MCC_STORAGE_WAIT_MS 30000
#endif

static pthread_mutex_t storage_lock=PTHREAD_MUTEX_INITIALIZER;
static int storage_mailbox[4];
static unsigned storage_generation;
static int storage_disabled;
/* Async proxy arguments can outlive the caller after a timeout. Serialized
 * requests reuse these buffers only after completion; a timeout disables reuse. */
static char storage_action[16];
static char storage_payload[MCC_SHARE_STAGE_FILES*(MCC_SHARE_LEAF_SIZE+64)+256];

static int storage_request(char const *action,char const *payload,uint64_t *available)
{
    int result,timed_out=0;
    double deadline;
    pthread_mutex_lock(&storage_lock);
    if (storage_disabled) { pthread_mutex_unlock(&storage_lock);return -2; }
    if (strlen(action)>=sizeof(storage_action) || strlen(payload)>=sizeof(storage_payload)) {
        pthread_mutex_unlock(&storage_lock);return 0;
    }
    strcpy(storage_action,action);strcpy(storage_payload,payload);
    storage_generation=(storage_generation+1)&0x7FFFFFFFu;
    __atomic_store_n(storage_mailbox,(int)storage_generation,__ATOMIC_RELEASE);
    __atomic_store_n(storage_mailbox+1,-1,__ATOMIC_RELEASE);
    web_js_mcc_storage(storage_action,storage_payload,storage_mailbox,(int)storage_generation);
    deadline=emscripten_get_now()+WEB_MCC_STORAGE_WAIT_MS;
    while (__atomic_load_n(storage_mailbox+1,__ATOMIC_ACQUIRE)==-1) {
        double remaining=deadline-emscripten_get_now();
        if (remaining<=0) {
            timed_out=1;
            break;
        }
        emscripten_futex_wait(storage_mailbox+1,-1,remaining<1000 ? remaining : 1000);
    }
    result=timed_out ? -2 : __atomic_load_n(storage_mailbox+1,__ATOMIC_ACQUIRE);
    if (result<0) {
        storage_disabled=1;
        __atomic_store_n(storage_mailbox,(int)(storage_generation+1),__ATOMIC_RELEASE);
        web_js_mcc_storage_fence();
        fprintf(stderr,"mcc browser storage: response unavailable; close this page and reopen it to recover the download.\n");
    }
    if (available) *available=(uint32_t)storage_mailbox[2]|((uint64_t)(uint32_t)storage_mailbox[3]<<32);
    pthread_mutex_unlock(&storage_lock);
    return result;
}

static int managed_folder(char const *folder)
{
    unsigned i;
    if (!folder || strlen(folder)!=23 || memcmp(folder,"dl_",3)) return 0;
    for (i=3;i<23;i++) if (!strchr("0123456789abcdef",folder[i])) return 0;
    return 1;
}

static int directory_folder(char const *directory,char folder[24])
{
    char native[1024];
    char const *leaf;
    if (!directory || !*directory) return 0;
    if (directory[0]=='/') snprintf(native,sizeof(native),"%s",directory);
    else platform_translate_path(directory,native,sizeof(native));
    if (strncmp(native,"/data/mcc_maps/",15)) return 0;
    leaf=native+15;
    if (!managed_folder(leaf)) return 0;
    strcpy(folder,leaf);
    return 1;
}

static int folder_request(char const *action,char const *folder,int clean)
{
    char payload[96];
    if (!managed_folder(folder)) return 0;
    snprintf(payload,sizeof(payload),"{\"folder\":\"%s\",\"clean\":%s}",folder,clean ? "true" : "false");
    return storage_request(action,payload,NULL);
}

static int direct_begin(char const *directory,struct mcc_share_file const *files,int count)
{
    char folder[24],*payload;
    unsigned size,used;
    int i,result,sidecar=0;
    if (!directory_folder(directory,folder) || count<1 || count>MCC_SHARE_STAGE_FILES) return 0;
    size=(unsigned)count*(MCC_SHARE_LEAF_SIZE+64)+256;
    payload=malloc(size);
    if (!payload) return 0;
    used=(unsigned)snprintf(payload,size,"{\"folder\":\"%s\",\"files\":[",folder);
    /* stage_manifest already canonicalized printable paths and rejected JSON
     * quote/backslash characters. The browser validates this boundary again. */
    for (i=0;i<count;i++) {
        used+=(unsigned)snprintf(payload+used,size-used,
            "%s{\"path\":\"%s\",\"size\":%u}",i ? "," : "",files[i].leaf,files[i].size);
        if (files[i].kind==MCC_SHARE_FILE_SIDECAR) sidecar=1;
    }
    /* A rehosted package can already transfer its sidecar. Journal a local
     * companion only when storage preparation may synthesize the absent file. */
    snprintf(payload+used,size-used,"],\"companions\":[%s\"download.receipt\"]}",
        sidecar ? "" : "\"workshop_id.txt\",");
    result=storage_request("begin",payload,NULL);
    free(payload);
    return result;
}

static int direct_finish(char const *directory,int publish)
{
    char folder[24],native[1024];
    struct stat status;
    if (!directory_folder(directory,folder)) return 0;
    if (directory[0]=='/') snprintf(native,sizeof(native),"%s",directory);
    else platform_translate_path(directory,native,sizeof(native));
    return folder_request(publish ? "commit" : "rollback",folder,lstat(native,&status)!=0 && errno==ENOENT);
}

static int direct_flush(FILE *file)
{
    return fflush(file)==0 && fsync(fileno(file))==0;
}

void web_mcc_share_io(struct mcc_share_io *io)
{
    static struct mcc_share_direct_io const direct={direct_begin,direct_finish,direct_flush};
    io->direct=&direct;
}

uint64_t web_mcc_free_bytes(void)
{
    uint64_t available=0;
    if (storage_request("space","{}",&available)<=0) return 0;
    /* The session's existing zero means unknown. One byte conservatively
     * represents a known exhausted quota and refuses every valid offer. */
    return available ? available : 1;
}

int web_mcc_directory_visible(char const *directory)
{
    char folder[24];
    return directory && *directory && (!directory_folder(directory,folder) || folder_request("visible",folder,0)>0);
}

int web_mcc_package_visible(char const *level)
{
    char folder[24];
    char const *end;
    unsigned i;
    if (!level || strncmp(level,"mcc_maps\\",9)) return 0;
    end=strchr(level+9,'\\');
    if (!end || end-level-9!=23) return 1;
    memcpy(folder,level+9,23);folder[23]=0;
    for (i=0;i<23;i++) if (folder[i]>='A' && folder[i]<='Z') folder[i]+='a'-'A';
    return !managed_folder(folder) || folder_request("visible",folder,0)>0;
}

int web_mcc_remove_begin(char const *folder) { return folder_request("removeBegin",folder,0)>0; }
void web_mcc_remove_end(char const *folder) { folder_request("removeEnd",folder,0); }
