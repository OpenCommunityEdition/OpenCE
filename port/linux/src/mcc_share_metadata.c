#include "mcc_share_metadata.h"
#include "mcc_manifest.h"
#include "mcc_preview_image.h"
#include "posix.h"
#include "server_json.h"
#include "../game/mcc_sha256.h"
#include <stdlib.h>
#include <string.h>

static unsigned char fold(unsigned char c)
{
    return c>='A' && c<='Z' ? (unsigned char)(c+'a'-'A') : c;
}

static int same(const char *a, const char *b)
{
    while (*a && *b) {
        int x=*a=='\\' ? '/' : fold((unsigned char)*a);
        int y=*b=='\\' ? '/' : fold((unsigned char)*b);
        if (x!=y) break;
        a++; b++;
    }
    return !*a && !*b;
}

/* Match mcc_share_stage.c's portable ASCII components before offering files.
 * Reject Windows aliases on every host; optional unsupported image paths then
 * retain a placeholder instead of making the whole transfer unstageable. */
static int relative_path(const char *path, char *out, unsigned capacity)
{
    size_t i, start=0, length;
    if (!path || !(length=strlen(path)) || length>=capacity) return 0;
    for (i=0; i<=length; i++) {
        unsigned char c=(unsigned char)path[i];
        if (!c || c=='/' || c=='\\') {
            char device[9];
            size_t j, end=start;
            if (i==start || path[start]=='.' || path[start]==' ' || path[i-1]=='.' || path[i-1]==' ') return 0;
            while (end<i && path[end]!='.') end++;
            while (end>start && path[end-1]==' ') end--;
            if (end-start<sizeof(device)) {
                for (j=start; j<end; j++) device[j-start]=(char)fold((unsigned char)path[j]);
                device[end-start]=0;
                if (!strcmp(device,"con") || !strcmp(device,"prn") || !strcmp(device,"aux") ||
                    !strcmp(device,"nul") || !strcmp(device,"conin$") || !strcmp(device,"conout$") ||
                    (end-start==4 && (!memcmp(device,"com",3) || !memcmp(device,"lpt",3)) &&
                     device[3]>='0' && device[3]<='9')) return 0;
            }
            out[i]=c ? '/' : 0;
            start=i+1;
        } else {
            if (c<32 || c>126 || strchr(":*?\"<>|",c)) return 0;
            out[i]=(char)c;
        }
    }
    return 1;
}

static int relative(const char *path, char out[MCC_SHARE_METADATA_PATH])
{
    return relative_path(path,out,MCC_SHARE_METADATA_PATH);
}

static int suffix(const char *path, const char *extension)
{
    size_t n=strlen(path), e=strlen(extension);
    return n>e && same(path+n-e,extension);
}

static int add(struct mcc_share_metadata_plan *plan, const char *path, unsigned kind, unsigned size)
{
    char canonical[MCC_SHARE_METADATA_PATH];
    unsigned total=size;
    int i;
    if (size>MCC_SHARE_METADATA_BYTES || !relative(path,canonical)) return 0;
    /* References resolve without case, but staged files must share one spelling
     * on case-sensitive hosts: META/a.json and meta/b.json own one directory. */
    for (i=0; canonical[i]; i++) canonical[i]=(char)fold((unsigned char)canonical[i]);
    for (i=0; i<plan->count; i++) {
        if (same(plan->files[i].path,canonical))
            return plan->files[i].kind==kind && plan->files[i].size==size;
        if (plan->files[i].size>MCC_SHARE_METADATA_BYTES-total) return 0;
        total+=plan->files[i].size;
    }
    if (total>MCC_SHARE_METADATA_BYTES || plan->count>=MCC_SHARE_METADATA_FILES) return 0;
    strcpy(plan->files[plan->count].path,canonical);
    plan->files[plan->count].kind=kind;
    plan->files[plan->count++].size=size;
    return 1;
}

static int json(const char *root, const char *path, char *text, struct server_json_document *document)
{
    char canonical[MCC_SHARE_METADATA_PATH];
    int length;
    if (!relative(path,canonical) || !suffix(canonical,".json")) return 0;
    length=posix_mcc_preview_read(root,canonical,text,STEAM_WORKSHOP_MAXIMUM_JSON);
    if (!length || !server_json_parse(document,text,length,16)) return 0;
    if (document->tokens[0].type!=_server_json_object) {
        server_json_document_free(document);
        return 0;
    }
    return length;
}

static int map(const char *root, const char *path, int campaign, char *text,
    struct steam_workshop_map_text *result)
{
    struct server_json_document document;
    char scenario[256], canonical[256], kind[32];
    int length=json(root,path,text,&document), token, valid;
    if (!length) return 0;
    valid=server_json_get_string(&document,server_json_member(&document,0,"ScenarioFile"),scenario,sizeof(scenario)) &&
        relative_path(scenario,canonical,sizeof(canonical));
    token=server_json_member(&document,0,"CampaignMapKind");
    if (campaign && token>=0)
        valid=valid && server_json_get_string(&document,token,kind,sizeof(kind)) && mcc_manifest_default_kind(kind);
    server_json_document_free(&document);
    if (!valid || !steam_workshop_read_map(text,length,result)) return 0;
    return length;
}

static int picture(const char *root, const char *path, struct mcc_share_metadata_plan *plan)
{
    unsigned char *bytes;
    struct mcc_preview_image image;
    char canonical[MCC_SHARE_METADATA_PATH];
    int length=0;
    /* Unsupported or unsafe optional references keep the menu placeholder. */
    if (!*path || !relative(path,canonical) || !suffix(canonical,".png")) return 1;
    bytes=malloc(MCC_PREVIEW_FILE_LIMIT);
    if (bytes) {
        length=posix_mcc_preview_read(root,canonical,bytes,MCC_PREVIEW_FILE_LIMIT);
        if (!mcc_preview_png(bytes,(unsigned)length,&image)) length=0;
        mcc_preview_image_free(&image);
        free(bytes);
    }
    return add(plan,canonical,MCC_SHARE_METADATA_PNG,(unsigned)length);
}

static int workshop(const char *root, struct mcc_share_metadata_plan *plan)
{
    char found[256], text[128];
    int length, i=0, title=0;
    if (!posix_find_entry_case_insensitive(root,"workshop_id.txt",found,sizeof(found))) return 1;
    length=posix_mcc_preview_read(root,"workshop_id.txt",text,sizeof(text));
    if (!length) return 0;
    while (i<length && text[i]>='0' && text[i]<='9') i++;
    if (i<1 || i>20) return 0;
    if (i<length && text[i]=='\r') i++;
    if (i<length) {
        if (text[i++]!='\n') return 0;
        while (i<length && text[i]!='\r' && text[i]!='\n') {
            if ((unsigned char)text[i]<32 || (unsigned char)text[i]>126 || ++title>39) return 0;
            i++;
        }
        if (i<length && text[i]=='\r') i++;
        if (i<length && text[i]=='\n') i++;
    }
    return i==length && add(plan,"workshop_id.txt",MCC_SHARE_METADATA_WORKSHOP,(unsigned)length);
}

static int metadata_plan(const char *root, int campaign, const char *level,
    struct mcc_share_metadata_plan *plan, char reason[128], int pictures)
{
    struct steam_workshop_item_text *item=NULL;
    struct steam_workshop_map_text selected, current;
    struct server_json_document document;
    void *directory;
    char *text=NULL, found[256], names[MCC_MANIFEST_MAXIMUM_MISSIONS][64];
    int length, count, i, j, matched=0, ok=0, automatic=campaign<0;
    if (!plan || !reason) return 0;
    memset(plan,0,sizeof(*plan));
    strcpy(reason,"Invalid or unsupported MCC package metadata.");
    if (!root || !level || !*level || strlen(level)>=64 || strchr(level,'/') || strchr(level,'\\')) return 0;
    directory=posix_directory_open(root);
    if (!directory) return 0;
    posix_directory_close(directory);
    if (!posix_find_entry_case_insensitive(root,"ModInfo.json",found,sizeof(found))) {
        if (posix_find_entry_case_insensitive(root,"CampaignInfo.json",found,sizeof(found))) return 0;
        ok=!pictures || workshop(root,plan);
        if (ok) reason[0]=0;
        return ok;
    }
    text=malloc(STEAM_WORKSHOP_MAXIMUM_JSON);
    item=malloc(sizeof(*item));
    if (!text || !item) goto done;
    length=json(root,"ModInfo.json",text,&document);
    if (!length) goto done;
    server_json_document_free(&document);
    if (!steam_workshop_read_item(text,length,item) || !item->halo1 ||
        !add(plan,"ModInfo.json",MCC_SHARE_METADATA_JSON,(unsigned)length)) goto done;
    if (automatic) campaign=item->has_campaign;
    count=item->map_count;
    if (campaign) {
        int token;
        if (!item->has_campaign) goto done;
        length=json(root,"CampaignInfo.json",text,&document);
        if (!length) goto done;
        token=server_json_member(&document,0,"CampaignMaps");
        count=token>=0 && document.tokens[token].type==_server_json_array ? document.tokens[token].size : 0;
        if (count<1 || count>MCC_MANIFEST_MAXIMUM_MISSIONS) count=0;
        for (i=0; i<count; i++)
            if (!server_json_get_string(&document,server_json_element(&document,token,i),item->map_json[i],sizeof(item->map_json[i]))) break;
        server_json_document_free(&document);
        if (!count || i!=count || !add(plan,"CampaignInfo.json",MCC_SHARE_METADATA_JSON,(unsigned)length)) goto done;
    }
scan_maps:
    for (i=0; i<count; i++) {
        length=map(root,item->map_json[i],campaign,text,&current);
        if (!length) {
            if (campaign) goto done;
            continue;
        }
        if (campaign) {
            for (j=0; j<i; j++) if (same(current.name,names[j])) goto done;
            strcpy(names[i],current.name);
        }
        if (campaign || same(current.name,level))
            if (!add(plan,item->map_json[i],MCC_SHARE_METADATA_JSON,(unsigned)length)) goto done;
        if (same(current.name,level)) { selected=current; matched++; }
    }
    /* HasCampaign remains in the unchanged ModInfo sent to clients. Preserve
     * its complete campaign JSON even when the selected level is multiplayer,
     * so a staged subset can rebuild this same plan and metadata identity. */
    if (!matched && automatic && campaign && item->map_count) {
        length=json(root,"ModInfo.json",text,&document);
        if (!length) goto done;
        server_json_document_free(&document);
        if (!steam_workshop_read_item(text,length,item)) goto done;
        campaign=0;
        count=item->map_count;
        goto scan_maps;
    }
    if (matched!=1 || (pictures && (!picture(root,selected.thumbnail,plan) ||
        !picture(root,selected.large,plan) || !workshop(root,plan)))) goto done;
    ok=1;
done:
    free(item); free(text);
    if (ok) reason[0]=0;
    else memset(plan,0,sizeof(*plan));
    return ok;
}

int mcc_share_metadata_plan(const char *root, int campaign, const char *level,
    struct mcc_share_metadata_plan *plan, char reason[128])
{
    return metadata_plan(root,campaign,level,plan,reason,1);
}

int mcc_share_metadata_digest(const char *root, int campaign, const char *level,
    unsigned char digest[32], char reason[128])
{
    struct mcc_share_metadata_plan *plan;
    struct mcc_sha256 hash;
    char *text, canonical[MCC_SHARE_METADATA_PATH];
    int i, j, order[MCC_SHARE_METADATA_FILES], count=0, ok=0;
    if (!digest || !reason) return 0;
    memset(digest,0,32);
    strcpy(reason,"Cannot read MCC package metadata identity.");
    plan=malloc(sizeof(*plan)); text=malloc(STEAM_WORKSHOP_MAXIMUM_JSON);
    if (!plan || !text) goto done;
    if (!metadata_plan(root,campaign,level,plan,reason,0)) goto done;
    for (i=0; i<plan->count; i++) if (plan->files[i].kind==MCC_SHARE_METADATA_JSON) {
        for (j=0; plan->files[i].path[j]; j++)
            plan->files[i].path[j]=(char)fold((unsigned char)plan->files[i].path[j]);
        j=count;
        while (j && strcmp(plan->files[order[j-1]].path,plan->files[i].path)>0) {
            order[j]=order[j-1]; j--;
        }
        order[j]=i; count++;
    }
    mcc_sha256_begin(&hash);
    mcc_sha256_add(&hash,"MCC package metadata 1",sizeof("MCC package metadata 1")-1);
    for (i=0; i<count; i++) {
        const struct mcc_share_metadata_entry *entry=&plan->files[order[i]];
        unsigned char size[4];
        int length=posix_mcc_preview_read(root,entry->path,text,STEAM_WORKSHOP_MAXIMUM_JSON);
        unsigned bytes=(unsigned)length;
        if (!length || bytes!=entry->size || !relative(entry->path,canonical)) goto done;
        for (j=0; j<4; j++) size[j]=(unsigned char)(bytes>>(8*j));
        mcc_sha256_add(&hash,canonical,(int)strlen(canonical)+1);
        mcc_sha256_add(&hash,size,4);
        mcc_sha256_add(&hash,text,length);
    }
    if (count) mcc_sha256_end(&hash,digest);
    ok=1; reason[0]=0;
done:
    free(plan); free(text);
    if (!ok && !reason[0]) strcpy(reason,"MCC package metadata changed while being read.");
    return ok;
}

int mcc_share_metadata_validate(const char *root, int campaign, const char *level,
    const struct mcc_share_metadata_entry *received, int count,
    unsigned char keep[MCC_SHARE_METADATA_FILES], char reason[128])
{
    struct mcc_share_metadata_plan *plan;
    char path[MCC_SHARE_METADATA_PATH];
    int i, j, k, ok=0;
    if (!reason || !keep) return 0;
    memset(keep,0,MCC_SHARE_METADATA_FILES);
    strcpy(reason,"The received metadata does not match its package references.");
    if (count<0 || count>MCC_SHARE_METADATA_FILES || (count && !received)) return 0;
    plan=malloc(sizeof(*plan));
    if (!plan) return 0;
    if (!mcc_share_metadata_plan(root,campaign,level,plan,reason)) goto done;
    strcpy(reason,"The received metadata does not match its package references.");
    for (i=0; i<count; i++) {
        if (!memchr(received[i].path,0,sizeof(received[i].path)) || !relative(received[i].path,path)) goto done;
        if (!received[i].size || received[i].size>(received[i].kind==MCC_SHARE_METADATA_PNG ?
            MCC_PREVIEW_FILE_LIMIT : STEAM_WORKSHOP_MAXIMUM_JSON)) goto done;
        for (k=0; k<i; k++) if (same(received[k].path,path)) goto done;
        for (j=0; j<plan->count; j++) if (same(plan->files[j].path,path)) break;
        if (j==plan->count || received[i].kind!=plan->files[j].kind) goto done;
        if (plan->files[j].size && received[i].size!=plan->files[j].size) goto done;
        keep[i]=(unsigned char)(plan->files[j].size!=0);
    }
    for (j=0; j<plan->count; j++) {
        if (plan->files[j].kind==MCC_SHARE_METADATA_PNG) continue;
        for (i=0; i<count; i++) if (same(plan->files[j].path,received[i].path)) break;
        if (i==count) goto done;
    }
    ok=1; reason[0]=0;
done:
    free(plan);
    if (!ok) memset(keep,0,MCC_SHARE_METADATA_FILES);
    return ok;
}
