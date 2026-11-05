#include "mcc_share_session.h"
#include "mcc_share_rules.h"
#include "mcc_package.h"
#include <stdio.h>
#include <string.h>

#define MCC_SHARE_ASK_TIMEOUT 30000u
#define MCC_SHARE_STALL_TIMEOUT 60000u
#define MCC_SHARE_HOST_IDLE_TIMEOUT 30000u
#define MCC_SHARE_ACTIVE_HEARD 20000u
#define MCC_SHARE_ACTIVE_ADVANCED 80000u
#define MCC_SHARE_MANIFEST_INTERVAL 1000u
#define MCC_SHARE_HASH_SLICE 0x400000u

static int share_elapsed(uint32_t now, uint32_t since, uint32_t span)
{
    return (uint32_t)(now-since)>=span;
}

/* A time that is never 0, which means "not yet" in the tables; it is not later than now. */
static uint32_t share_stamp(uint32_t now)
{
    return now ? now : 1;
}

static uint32_t share_now(struct mcc_share const *share)
{
    return share->env.now(share->env.user);
}

static int share_same(char const *a, char const *b)
{
    for (;*a && *b;a++,b++) {
        unsigned char x=(unsigned char)*a,y=(unsigned char)*b;
        if (x>='A' && x<='Z') x=(unsigned char)(x+'a'-'A');
        if (y>='A' && y<='Z') y=(unsigned char)(y+'a'-'A');
        if (x!=y) return 0;
    }
    return !*a && !*b;
}

/* The names a package may be shared under, by the rules module's own name checks.
 * The facts passed besides the name are fixed here: only the name is judged, and
 * nothing a remote host says establishes content or origin. */
static int share_level_split(char const *level, char folder[64], char leaf[MCC_SHARE_LEAF_SIZE])
{
    struct mcc_share_package package;
    char canonical[64],*separator;
    size_t length=strlen(level);
    memset(&package,0,sizeof(package));
    package.name=level;
    package.name_length=length;
    package.sidecar=MCC_SHARE_SIDECAR_ABSENT;
    package.content=MCC_SHARE_CONTENT_CUSTOM;
    package.content_known=1;
    package.host_opt_in=1;
    if (mcc_share_decide(&package)!=MCC_SHARE_ELIGIBLE || !mcc_package_name(level,canonical)) return 0;
    separator=strchr(canonical+9,'\\');
    if (!separator || strlen(separator+1)>=MCC_SHARE_LEAF_SIZE-4) return 0;
    *separator=0;
    strcpy(folder,canonical+9);
    strcpy(leaf,separator+1);
    return 1;
}

static int share_level_text(char const *text)
{
    return memchr(text,0,MCC_SHARE_LEVEL_SIZE)!=NULL;
}

enum mcc_share_result mcc_share_admit(char const *level, int opt_in, enum mcc_share_sidecar sidecar,
    unsigned char const digest[32], int digest_known, char const *list, unsigned long list_length,
    char credit[MCC_SHARE_CREDIT_SIZE])
{
    struct mcc_share_package package;
    int listed;
    credit[0]=0;
    if (!opt_in) return MCC_SHARE_RESULT_OFF;
    listed=digest_known ? mcc_share_list_find(list,list_length,digest,credit) : 0;
    memset(&package,0,sizeof(package));
    package.name=level;
    package.name_length=level ? strlen(level) : 0;
    package.sidecar=sidecar;
    package.content=listed ? MCC_SHARE_CONTENT_CUSTOM : MCC_SHARE_CONTENT_UNKNOWN;
    package.content_known=digest_known;
    package.host_opt_in=1;
    switch (mcc_share_decide(&package)) {
    case MCC_SHARE_ELIGIBLE:
        /* Listed without a maker: permission and credit are unclear, so the joiner is
         * pointed to the maker instead. */
        if (listed<0) {
            credit[0]=0;
            return MCC_SHARE_RESULT_NO_CREDIT;
        }
        return MCC_SHARE_RESULT_OK;
    case MCC_SHARE_WORKSHOP: return MCC_SHARE_RESULT_WORKSHOP;
    case MCC_SHARE_NOT_OPTED_IN: return MCC_SHARE_RESULT_OFF;
    default: break;
    }
    credit[0]=0;
    return MCC_SHARE_RESULT_NOT_SHAREABLE;
}

void mcc_share_init(struct mcc_share *share, struct mcc_share_env const *env)
{
    memset(share,0,sizeof(*share));
    share->env=*env;
    share->next_session=0x4d435331u;
}

void mcc_share_refusal_text(enum mcc_share_result result, char const *level, char *text, unsigned size)
{
    char const *leaf=strrchr(level,'\\');
    leaf=leaf ? leaf+1 : level;
    switch (result) {
    case MCC_SHARE_RESULT_OFF:
        snprintf(text,size,"The host does not send MCC maps. Get the package for %s from its maker, "
            "or ask the host to turn sharing on.",leaf);
        break;
    case MCC_SHARE_RESULT_WORKSHOP:
        snprintf(text,size,"%s is a Steam Workshop map, which is not sent. Subscribe to it on the Steam Workshop.",leaf);
        break;
    case MCC_SHARE_RESULT_NO_CREDIT:
        snprintf(text,size,"The host has no maker credit for %s, so it does not send it. "
            "Get the package from its maker.",leaf);
        break;
    case MCC_SHARE_RESULT_BUSY:
        snprintf(text,size,"The host is sending maps to other players. Try joining again in a moment.");
        break;
    case MCC_SHARE_RESULT_UNAVAILABLE:
        snprintf(text,size,"The host could not read the files of %s. Get the package from its maker.",leaf);
        break;
    case MCC_SHARE_RESULT_CHANGED:
        snprintf(text,size,"The host's files for %s changed during the download. Join again.",leaf);
        break;
    default:
        snprintf(text,size,"The host cannot send %s: it is not a package the host can share. "
            "Get the package from its maker.",leaf);
        break;
    }
}

/* ---------- the host */

static void host_reply(struct mcc_share *share, long machine, uint8_t op, enum mcc_share_result result,
    char const *level, uint32_t session, uint8_t file, uint8_t count, uint32_t offset, uint32_t length,
    char const *credit, void const *body, unsigned body_bytes)
{
    static uint8_t buffer[MCC_SHARE_RESPONSE_MAXIMUM];
    struct mcc_share_response head;
    memset(&head,0,sizeof(head));
    head.op=op;
    head.result=(uint8_t)result;
    head.file=file;
    head.count=count;
    head.session=session;
    head.offset=offset;
    head.length=length;
    snprintf(head.level,sizeof(head.level),"%s",level);
    if (credit) snprintf(head.credit,sizeof(head.credit),"%s",credit);
    if (body_bytes>sizeof(buffer)-sizeof(head)) return;
    memcpy(buffer,&head,sizeof(head));
    if (body_bytes) memcpy(buffer+sizeof(head),body,body_bytes);
    share->env.send_response(share->env.user,machine,buffer,(unsigned)sizeof(head)+body_bytes);
}

static struct mcc_share_host_transfer *host_find(struct mcc_share *share, long machine)
{
    int i;
    for (i=0;i<MCC_SHARE_HOST_TRANSFERS;i++)
        if (share->transfers[i].used && share->transfers[i].machine==machine) return &share->transfers[i];
    return NULL;
}

static void host_free(struct mcc_share_host_transfer *transfer)
{
    if (transfer->hash_stream) fclose(transfer->hash_stream);
    memset(transfer,0,sizeof(*transfer));
}

void mcc_share_host_forget(struct mcc_share *share, long machine)
{
    struct mcc_share_host_transfer *transfer=host_find(share,machine);
    if (transfer) host_free(transfer);
}

void mcc_share_host_stop(struct mcc_share *share)
{
    int i;
    for (i=0;i<MCC_SHARE_HOST_TRANSFERS;i++) if (share->transfers[i].used) host_free(&share->transfers[i]);
}

static void host_send_manifest(struct mcc_share *share, struct mcc_share_host_transfer *transfer)
{
    struct mcc_share_entry entries[MCC_SHARE_STAGE_FILES];
    int i;
    memset(entries,0,sizeof(entries));
    for (i=0;i<transfer->package.count;i++) {
        entries[i].kind=transfer->files[i].kind;
        entries[i].size=transfer->files[i].size;
        memcpy(entries[i].leaf,transfer->files[i].leaf,sizeof(entries[i].leaf));
        memcpy(entries[i].sha256,transfer->files[i].sha256,32);
    }
    host_reply(share,transfer->machine,MCC_SHARE_OP_MANIFEST,MCC_SHARE_RESULT_OK,transfer->level,
        transfer->session,0,(uint8_t)transfer->package.count,0,0,transfer->credit,
        entries,(unsigned)(transfer->package.count*sizeof(entries[0])));
}

static void host_request_manifest(struct mcc_share *share, long machine, struct mcc_share_request const *request)
{
    struct mcc_share_host_transfer *transfer=host_find(share,machine);
    uint32_t now=share_now(share);
    int i;
    if (share->manifest_at[machine] && !share_elapsed(now,share->manifest_at[machine],MCC_SHARE_MANIFEST_INTERVAL))
        return;
    share->manifest_at[machine]=share_stamp(now);
    if (transfer) {
        if (transfer->hashing) return;
        if (share_same(transfer->level,request->level)) {
            host_send_manifest(share,transfer);
            return;
        }
        host_free(transfer);
    }
    for (i=0;i<MCC_SHARE_HOST_TRANSFERS && share->transfers[i].used;i++) {}
    if (i==MCC_SHARE_HOST_TRANSFERS) {
        host_reply(share,machine,MCC_SHARE_OP_MANIFEST,MCC_SHARE_RESULT_BUSY,request->level,0,0,0,0,0,NULL,NULL,0);
        return;
    }
    transfer=&share->transfers[i];
    memset(transfer,0,sizeof(*transfer));
    /* The admission (opt-in, the rules module, the host's attestation) is the game's;
     * a refusal carries only its reason. */
    share->env.host_package(share->env.user,request->level,&transfer->package);
    if (transfer->package.refusal!=MCC_SHARE_RESULT_OK || transfer->package.count<1 ||
        transfer->package.count>MCC_SHARE_STAGE_FILES) {
        enum mcc_share_result refusal=transfer->package.refusal!=MCC_SHARE_RESULT_OK ?
            transfer->package.refusal : MCC_SHARE_RESULT_NOT_SHAREABLE;
        memset(transfer,0,sizeof(*transfer));
        host_reply(share,machine,MCC_SHARE_OP_MANIFEST,refusal,request->level,0,0,0,0,0,NULL,NULL,0);
        return;
    }
    transfer->used=1;
    transfer->machine=machine;
    transfer->hashing=1;
    transfer->heard_at=transfer->advanced_at=share_stamp(now);
    snprintf(transfer->level,sizeof(transfer->level),"%s",request->level);
    snprintf(transfer->credit,sizeof(transfer->credit),"%s",transfer->package.credit);
    for (i=0;i<transfer->package.count;i++) {
        transfer->files[i].kind=transfer->package.files[i].kind;
        memcpy(transfer->files[i].leaf,transfer->package.files[i].leaf,MCC_SHARE_LEAF_SIZE);
        transfer->files[i].leaf[MCC_SHARE_LEAF_SIZE-1]=0;
    }
}

static void host_fail(struct mcc_share *share, struct mcc_share_host_transfer *transfer,
    enum mcc_share_result result)
{
    host_reply(share,transfer->machine,MCC_SHARE_OP_MANIFEST,result,transfer->level,0,0,0,0,0,NULL,NULL,0);
    host_free(transfer);
}

/* Hash a slice of the files each frame, so a large map does not stall the host's frame. */
static void host_hash(struct mcc_share *share, struct mcc_share_host_transfer *transfer)
{
    uint32_t budget=MCC_SHARE_HASH_SLICE;
    static uint8_t block[16384];                       /* (static: this runs in the game's frame, on its stack) */
    char level_leaf[MCC_SHARE_LEAF_SIZE],folder[64];
    while (budget && transfer->hash_file<transfer->package.count) {
        struct mcc_share_file *file=&transfer->files[transfer->hash_file];
        if (!transfer->hash_stream) {
            long size;
            transfer->hash_stream=fopen(transfer->package.files[transfer->hash_file].path,"rb");
            if (!transfer->hash_stream || fseek(transfer->hash_stream,0,SEEK_END) || (size=ftell(transfer->hash_stream))<=0 ||
                (uint32_t)size>MCC_SHARE_MAXIMUM_FILE_BYTES || fseek(transfer->hash_stream,0,SEEK_SET)) {
                host_fail(share,transfer,MCC_SHARE_RESULT_UNAVAILABLE);
                return;
            }
            file->size=(uint32_t)size;
            transfer->hash_offset=0;
            mcc_sha256_begin(&transfer->hash);
        }
        {
            uint32_t want=file->size-transfer->hash_offset;
            size_t got;
            if (want>sizeof(block)) want=sizeof(block);
            if (want>budget) want=budget;
            got=fread(block,1,want,transfer->hash_stream);
            if (got!=want) {
                host_fail(share,transfer,MCC_SHARE_RESULT_UNAVAILABLE);
                return;
            }
            mcc_sha256_add(&transfer->hash,block,(int)got);
            transfer->hash_offset+=(uint32_t)got;
            budget-=(uint32_t)got;
        }
        if (transfer->hash_offset==file->size) {
            mcc_sha256_end(&transfer->hash,file->sha256);
            fclose(transfer->hash_stream);
            transfer->hash_stream=NULL;
            transfer->hash_file++;
        }
    }
    if (transfer->hash_file<transfer->package.count) return;
    if (!share_level_split(transfer->level,folder,level_leaf) ||
        !mcc_share_manifest_valid(transfer->files,transfer->package.count,level_leaf)) {
        host_fail(share,transfer,MCC_SHARE_RESULT_UNAVAILABLE);
        return;
    }
    transfer->hashing=0;
    share->next_session+=0x9E3779B1u;
    transfer->session=(share->next_session^share_now(share))|1;
    transfer->heard_at=transfer->advanced_at=share_stamp(share_now(share));
    transfer->budget_at=share_now(share);
    host_send_manifest(share,transfer);
}

static void host_request_chunk(struct mcc_share *share, long machine, struct mcc_share_request const *request)
{
    struct mcc_share_host_transfer *transfer=host_find(share,machine);
    struct mcc_share_file const *file;
    if (!transfer || transfer->hashing) return;
    if (request->session!=transfer->session || !share_same(request->level,transfer->level)) {
        host_reply(share,machine,MCC_SHARE_OP_CHUNK,MCC_SHARE_RESULT_CHANGED,request->level,request->session,
            request->file,0,request->offset,0,NULL,NULL,0);
        return;
    }
    if (request->file>=transfer->package.count || !request->length || request->length>MCC_SHARE_CHUNK_BYTES) {
        host_reply(share,machine,MCC_SHARE_OP_CHUNK,MCC_SHARE_RESULT_BAD_REQUEST,request->level,request->session,
            request->file,0,request->offset,0,NULL,NULL,0);
        return;
    }
    file=&transfer->files[request->file];
    if (request->offset>=file->size || request->length>file->size-request->offset) {
        host_reply(share,machine,MCC_SHARE_OP_CHUNK,MCC_SHARE_RESULT_BAD_REQUEST,request->level,request->session,
            request->file,0,request->offset,0,NULL,NULL,0);
        return;
    }
    transfer->heard_at=share_stamp(share_now(share));
    if (transfer->queued>=(int)(sizeof(transfer->queue)/sizeof(transfer->queue[0]))) return;
    transfer->queue[transfer->queued].file=request->file;
    transfer->queue[transfer->queued].offset=request->offset;
    transfer->queue[transfer->queued].length=request->length;
    transfer->queued++;
}

static void host_serve(struct mcc_share *share, struct mcc_share_host_transfer *transfer)
{
    uint32_t now=share_now(share),limit=share->env.upload_limit(share->env.user);
    uint32_t elapsed=now-transfer->budget_at;
    if (elapsed>1000) elapsed=1000;
    transfer->budget_at=now;
    transfer->budget+=(int32_t)(((uint64_t)limit*elapsed)/1000u);
    if (transfer->budget>(int32_t)limit) transfer->budget=(int32_t)limit;
    while (transfer->queued) {
        static uint8_t data[MCC_SHARE_CHUNK_BYTES];
        struct mcc_share_file const *file;
        FILE *stream;
        long end=0;
        uint32_t length=transfer->queue[0].length,offset=transfer->queue[0].offset;
        uint8_t index=transfer->queue[0].file;
        size_t got=0;
        /* A chunk goes whenever the budget is positive and puts it in debt, so a
         * limit lower than a chunk still moves at the limit on average. */
        if (transfer->budget<=0) return;
        file=&transfer->files[index];
        stream=fopen(transfer->package.files[index].path,"rb");
        if (stream && !fseek(stream,0,SEEK_END)) end=ftell(stream);
        if (!stream || end!=(long)file->size || fseek(stream,(long)offset,SEEK_SET) ||
            (got=fread(data,1,length,stream))!=length) {
            if (stream) fclose(stream);
            host_reply(share,transfer->machine,MCC_SHARE_OP_CHUNK,MCC_SHARE_RESULT_CHANGED,transfer->level,
                transfer->session,index,0,offset,0,NULL,NULL,0);
            host_free(transfer);
            return;
        }
        fclose(stream);
        host_reply(share,transfer->machine,MCC_SHARE_OP_CHUNK,MCC_SHARE_RESULT_OK,transfer->level,
            transfer->session,index,0,offset,length,NULL,data,length);
        transfer->budget-=(int32_t)length;
        transfer->sent_bytes+=length;
        transfer->advanced_at=share_stamp(now);
        memmove(&transfer->queue[0],&transfer->queue[1],(size_t)(transfer->queued-1)*sizeof(transfer->queue[0]));
        transfer->queued--;
    }
}

void mcc_share_host_request(struct mcc_share *share, long machine, void const *message, unsigned bytes)
{
    struct mcc_share_request request;
    if (bytes!=sizeof(request) || machine<0 || machine>=MCC_SHARE_HOST_MACHINES) return;
    memcpy(&request,message,sizeof(request));
    if (!share_level_text(request.level) || request.reserved) return;
    switch (request.op) {
    case MCC_SHARE_OP_MANIFEST:
        host_request_manifest(share,machine,&request);
        break;
    case MCC_SHARE_OP_CHUNK:
        host_request_chunk(share,machine,&request);
        break;
    case MCC_SHARE_OP_DONE:
        mcc_share_host_forget(share,machine);
        break;
    }
}

int mcc_share_host_active(struct mcc_share const *share, long machine)
{
    int i;
    uint32_t now=share->env.now(share->env.user);
    for (i=0;i<MCC_SHARE_HOST_TRANSFERS;i++) {
        struct mcc_share_host_transfer const *transfer=&share->transfers[i];
        if (!transfer->used || transfer->machine!=machine) continue;
        return transfer->hashing || (!share_elapsed(now,transfer->heard_at,MCC_SHARE_ACTIVE_HEARD) &&
            !share_elapsed(now,transfer->advanced_at,MCC_SHARE_ACTIVE_ADVANCED));
    }
    return 0;
}

/* ---------- the joiner */

static void client_idle(struct mcc_share *share)
{
    share->state=MCC_SHARE_CLIENT_IDLE;
    share->outstanding=0;
}

static void client_send(struct mcc_share *share, uint8_t op, uint8_t file, uint8_t flags,
    uint32_t offset, uint32_t length)
{
    struct mcc_share_request request;
    memset(&request,0,sizeof(request));
    request.op=op;
    request.file=file;
    request.flags=flags;
    request.session=share->session;
    request.offset=offset;
    request.length=length;
    snprintf(request.level,sizeof(request.level),"%s",share->level);
    share->env.send_request(share->env.user,&request);
}

static void client_fail(struct mcc_share *share, char const *message)
{
    if (share->state!=MCC_SHARE_CLIENT_IDLE) client_send(share,MCC_SHARE_OP_DONE,0,0,0,0);
    mcc_share_stage_cancel(&share->stage);
    memset(&share->stage,0,sizeof(share->stage));
    client_idle(share);
    share->env.finished(share->env.user,0,message);
}

static void client_pump(struct mcc_share *share)
{
    while (share->outstanding<MCC_SHARE_WINDOW && share->request_file<share->stage.count) {
        uint32_t size=share->stage.files[share->request_file].size;
        uint32_t length=size-share->request_offset;
        if (length>MCC_SHARE_CHUNK_BYTES) length=MCC_SHARE_CHUNK_BYTES;
        client_send(share,MCC_SHARE_OP_CHUNK,(uint8_t)share->request_file,0,share->request_offset,length);
        share->outstanding++;
        share->request_offset+=length;
        if (share->request_offset==size) {
            share->request_file++;
            share->request_offset=0;
        }
    }
}

int mcc_share_client_begin(struct mcc_share *share, char const *level, unsigned char const digest[32])
{
    char folder[64],leaf[MCC_SHARE_LEAF_SIZE];
    if (share->state!=MCC_SHARE_CLIENT_IDLE || !level || strlen(level)>=MCC_SHARE_LEVEL_SIZE ||
        !share_level_split(level,folder,leaf) || share->env.package_taken(share->env.user,level)) return 0;
    memset(&share->stage,0,sizeof(share->stage));
    snprintf(share->level,sizeof(share->level),"%s",level);
    memcpy(share->digest,digest,32);
    share->session=0;
    share->credit[0]=0;
    share->request_file=0;
    share->request_offset=0;
    share->outstanding=0;
    share->state=MCC_SHARE_CLIENT_ASKING;
    share->asked_at=share->progress_at=share_now(share);
    client_send(share,MCC_SHARE_OP_MANIFEST,0,0,0,0);
    return 1;
}

static void client_manifest(struct mcc_share *share, struct mcc_share_response const *head,
    struct mcc_share_entry const *entries)
{
    struct mcc_share_file files[MCC_SHARE_STAGE_FILES];
    char folder[64],leaf[MCC_SHARE_LEAF_SIZE],text[256];
    enum mcc_share_stage_result result;
    int i;
    if (head->count<1 || head->count>MCC_SHARE_STAGE_FILES) {
        client_fail(share,"The host's list of files was not acceptable.");
        return;
    }
    memset(files,0,sizeof(files));
    for (i=0;i<head->count;i++) {
        files[i].kind=entries[i].kind;
        memcpy(files[i].leaf,entries[i].leaf,sizeof(files[i].leaf));
        files[i].size=entries[i].size;
        memcpy(files[i].sha256,entries[i].sha256,32);
    }
    if (!share_level_split(share->level,folder,leaf)) {
        client_fail(share,"The host's map name is not one this machine accepts.");
        return;
    }
    /* The files are checked against the manifest's sizes before they are made room for. */
    if (mcc_share_manifest_valid(files,head->count,leaf) && share->env.free_bytes) {
        uint64_t total=0,free_bytes=share->env.free_bytes(share->env.user);
        for (i=0;i<head->count;i++) total+=files[i].size;
        /* (the files and some room to spare: they are moved within the same disk, not copied) */
        if (free_bytes && total+(64u<<20)>free_bytes) {
            client_fail(share,"There is not enough free disk space for the host's map. Free some space and join again.");
            return;
        }
    }
    result=mcc_share_stage_begin(&share->stage,&share->env.io,share->env.root,folder,leaf,files,head->count);
    if (result==MCC_SHARE_STAGE_EXISTS) {
        snprintf(text,sizeof(text),"A package folder named %s already exists here but does not match the host's. "
            "Rename or remove it, or install the host's version, then join again.",folder);
        client_fail(share,text);
        return;
    }
    if (result!=MCC_SHARE_STAGE_OK) {
        client_fail(share,"The host's list of files was not acceptable.");
        return;
    }
    {
        unsigned used=0,j;
        for (j=0;j<MCC_SHARE_CREDIT_SIZE-1 && head->credit[j];j++)
            if ((unsigned char)head->credit[j]>=32 && (unsigned char)head->credit[j]<127) share->credit[used++]=head->credit[j];
        share->credit[used]=0;
    }
    share->session=head->session;
    share->state=MCC_SHARE_CLIENT_TRANSFERRING;
    share->progress_at=share_now(share);
    client_pump(share);
}

static void client_chunk(struct mcc_share *share, struct mcc_share_response const *head, uint8_t const *data,
    unsigned bytes)
{
    enum mcc_share_stage_result result;
    if (head->session!=share->session || head->file!=share->stage.current || head->offset!=share->stage.written ||
        !head->length || head->length!=bytes || head->length>MCC_SHARE_CHUNK_BYTES ||
        head->length>mcc_share_stage_wanted(&share->stage) || share->outstanding<=0) {
        client_fail(share,"The host sent data this machine did not ask for. The download was stopped.");
        return;
    }
    share->outstanding--;
    result=mcc_share_stage_write(&share->stage,data,head->length);
    share->progress_at=share_now(share);
    switch (result) {
    case MCC_SHARE_STAGE_OK:
    case MCC_SHARE_STAGE_FILE_DONE:
        client_pump(share);
        break;
    case MCC_SHARE_STAGE_ALL_DONE:
        share->state=MCC_SHARE_CLIENT_VERIFYING;
        break;
    case MCC_SHARE_STAGE_BAD_HASH:
        client_fail(share,"A file from the host did not match its SHA-256. Nothing was installed.");
        break;
    default:
        client_fail(share,"The files from the host could not be written. Nothing was installed.");
        break;
    }
}

void mcc_share_client_response(struct mcc_share *share, void const *message, unsigned bytes)
{
    struct mcc_share_response head;
    uint8_t const *body=(uint8_t const *)message+sizeof(head);
    char text[256];
    if (share->state==MCC_SHARE_CLIENT_IDLE || bytes<sizeof(head)) return;
    memcpy(&head,message,sizeof(head));
    if (!share_level_text(head.level) || !share_same(head.level,share->level)) return;
    bytes-=(unsigned)sizeof(head);
    if (head.result!=MCC_SHARE_RESULT_OK) {
        if ((head.op!=MCC_SHARE_OP_MANIFEST && head.op!=MCC_SHARE_OP_CHUNK) || bytes) return;
        if (head.op==MCC_SHARE_OP_MANIFEST && share->state!=MCC_SHARE_CLIENT_ASKING) return;
        if (head.op==MCC_SHARE_OP_CHUNK && share->state!=MCC_SHARE_CLIENT_TRANSFERRING) return;
        mcc_share_refusal_text((enum mcc_share_result)head.result,share->level,text,sizeof(text));
        client_fail(share,text);
        return;
    }
    if (head.op==MCC_SHARE_OP_MANIFEST) {
        struct mcc_share_entry entries[MCC_SHARE_STAGE_FILES];
        if (share->state!=MCC_SHARE_CLIENT_ASKING || head.count>MCC_SHARE_STAGE_FILES ||
            bytes!=head.count*sizeof(entries[0])) return;
        memcpy(entries,body,bytes);
        client_manifest(share,&head,entries);
    } else if (head.op==MCC_SHARE_OP_CHUNK) {
        if (share->state!=MCC_SHARE_CLIENT_TRANSFERRING) return;
        client_chunk(share,&head,body,bytes);
    }
}

static int client_check(void *user, char const *stage_dir)
{
    struct mcc_share *share=user;
    return share->env.identity_check(share->env.user,stage_dir);
}

void mcc_share_client_cancel(struct mcc_share *share)
{
    if (share->state==MCC_SHARE_CLIENT_IDLE) return;
    mcc_share_stage_cancel(&share->stage);
    memset(&share->stage,0,sizeof(share->stage));
    client_idle(share);
}

int mcc_share_client_busy(struct mcc_share const *share)
{
    return share->state!=MCC_SHARE_CLIENT_IDLE;
}

int mcc_share_client_level(struct mcc_share const *share, char const *level)
{
    return share->state!=MCC_SHARE_CLIENT_IDLE && level && share_same(share->level,level);
}

int mcc_share_client_percent(struct mcc_share const *share)
{
    if (share->state==MCC_SHARE_CLIENT_IDLE) return -1;
    if (share->state!=MCC_SHARE_CLIENT_TRANSFERRING || !share->stage.total) return 0;
    return (int)(((uint64_t)share->stage.done*100u)/share->stage.total);
}

static void client_update(struct mcc_share *share)
{
    uint32_t now=share_now(share);
    if (share->state==MCC_SHARE_CLIENT_ASKING && share_elapsed(now,share->asked_at,MCC_SHARE_ASK_TIMEOUT)) {
        client_fail(share,"The host did not answer the request for the map.");
    } else if (share->state==MCC_SHARE_CLIENT_TRANSFERRING &&
        share_elapsed(now,share->progress_at,MCC_SHARE_STALL_TIMEOUT)) {
        client_fail(share,"The download stopped: the host sent nothing for a minute.");
    } else if (share->state==MCC_SHARE_CLIENT_VERIFYING) {
        enum mcc_share_stage_result result=mcc_share_stage_verify(&share->stage,client_check,share);
        if (result==MCC_SHARE_STAGE_REFUSED) {
            memset(&share->stage,0,sizeof(share->stage));
            client_send(share,MCC_SHARE_OP_DONE,0,0,0,0);
            client_idle(share);
            share->env.finished(share->env.user,0,"The files from the host are not the host's map: their SHA-256 "
                "identity differs. Nothing was installed.");
            return;
        }
        if (result==MCC_SHARE_STAGE_OK) result=mcc_share_stage_install(&share->stage);
        if (result!=MCC_SHARE_STAGE_OK) {
            memset(&share->stage,0,sizeof(share->stage));
            client_send(share,MCC_SHARE_OP_DONE,0,0,0,0);
            client_idle(share);
            share->env.finished(share->env.user,0,result==MCC_SHARE_STAGE_EXISTS ?
                "A package of that name appeared during the download. Nothing was installed." :
                "The package could not be installed. Nothing was installed.");
            return;
        }
        client_send(share,MCC_SHARE_OP_DONE,0,1,0,0);
        memset(&share->stage,0,sizeof(share->stage));
        client_idle(share);
        share->env.finished(share->env.user,1,share->credit);
    }
}

void mcc_share_update(struct mcc_share *share)
{
    int i;
    uint32_t now=share_now(share);
    for (i=0;i<MCC_SHARE_HOST_TRANSFERS;i++) {
        struct mcc_share_host_transfer *transfer=&share->transfers[i];
        if (!transfer->used) continue;
        if (!transfer->hashing && share_elapsed(now,transfer->heard_at,MCC_SHARE_HOST_IDLE_TIMEOUT)) {
            host_free(transfer);
            continue;
        }
        if (transfer->hashing) host_hash(share,transfer);
        else host_serve(share,transfer);
    }
    client_update(share);
}
