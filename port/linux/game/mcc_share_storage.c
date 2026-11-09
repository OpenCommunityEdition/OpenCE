/* The receipt is created locally after validation, not accepted from a host.
 * Explicit bytes keep this small ownership record independent of platform ABI. */
#include "mcc_share_storage.h"
#include "mcc_package.h"
#include "../src/posix.h"
#include <stdlib.h>
#include <string.h>
#ifdef HALO_WEB
#include "../../web/src/web_mcc_storage.h"
#endif

/* Header: magic[8], folder[24], source[64], maker[64], item id[24], title[40],
 * little-endian count[4]. Entry: kind[1], size[4], SHA-256[32], relative path[128]. */
#define STORAGE_HEADER 228u
#define STORAGE_ENTRY 165u
#define STORAGE_FILES (MCC_SHARE_STAGE_FILES+1)
static unsigned char const storage_magic[8]={'M','C','C','D','L','0','1','\n'};
struct storage_receipt {
    struct mcc_share_storage_item item;
    struct mcc_share_file files[STORAGE_FILES];
    int count;
};

FILE *mcc_share_open_native(char const *path)
{
    int descriptor=posix_mcc_share_open(path);
    FILE *file;
    if (descriptor<0) return NULL;
#ifdef _WIN32
    file=_fdopen(descriptor,"rb");
#else
    file=fdopen(descriptor,"rb");
#endif
    if (!file) posix_mcc_share_close(descriptor);
    return file;
}

static int native_directory(char const *path)
{
    struct posix_file_information information;
    return posix_mcc_share_path_safe(path) && !posix_stat(path,&information) &&
        (information.flags&_posix_file_is_directory)!=0;
}

static int native_delete(char const *path) { return posix_mcc_share_remove(path,0); }
static int native_remove_directory(char const *path) { return posix_mcc_share_remove(path,1); }

void mcc_share_storage_native_io(struct mcc_share_io *io)
{
    if (!io) return;
    memset(io,0,sizeof(*io));
    io->path_safe=posix_mcc_share_path_safe;
    io->open_native_read=mcc_share_open_native;
    io->directory_exists=native_directory;
    io->delete_file=native_delete;
    io->remove_directory=native_remove_directory;
}

static int text_valid(char const *text, unsigned capacity, int required)
{
    unsigned i;
    if (!text || (required && !*text)) return 0;
    for (i=0;i<capacity;i++) {
        unsigned char c=(unsigned char)text[i];
        if (!c) return 1;
        if (c<32 || c>126) return 0;
    }
    return 0;
}

static int folder_valid(char const *folder)
{
    unsigned i;
    if (!folder || strlen(folder)!=23 || memcmp(folder,"dl_",3)) return 0;
    for (i=3;i<23;i++) if (!strchr("0123456789abcdef",folder[i])) return 0;
    return 1;
}

static int workshop_valid(struct mcc_workshop_item const *item)
{
    unsigned i;
    if (!item || !text_valid(item->id,sizeof(item->id),1) ||
        !text_valid(item->title,sizeof(item->title),0) || strlen(item->id)>20) return 0;
    for (i=0;item->id[i];i++) if (item->id[i]<'0' || item->id[i]>'9') return 0;
    return 1;
}

static int path_join(char path[512], char const *parent, char const *name)
{
    int n=snprintf(path,512,"%s/%s",parent,name);
    return n>0 && n<512;
}

static void put32(unsigned char *p, uint32_t value)
{
    unsigned i;
    for (i=0;i<4;i++) p[i]=(unsigned char)(value>>(8*i));
}

static uint32_t get32(unsigned char const *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

/* Reuse the network manifest's path, name, kind and overlap validation. The
 * sole additional file allowed beyond its capacity is our generated sidecar. */
static int files_valid(struct storage_receipt const *receipt)
{
    char canonical[64];
    char const *leaf;
    int i,count=receipt->count;
    if (count<1 || count>STORAGE_FILES || !mcc_package_name(receipt->item.source,canonical) ||
        strcmp(canonical,receipt->item.source) || !strchr(canonical+9,'\\')) return 0;
    leaf=strrchr(canonical,'\\')+1;
    for (i=0;i<count;i++) {
        if (!memchr(receipt->files[i].leaf,0,sizeof(receipt->files[i].leaf)) ||
            strchr(receipt->files[i].leaf,'\\')) return 0;
    }
    if (count>MCC_SHARE_STAGE_FILES) {
        struct mcc_share_file const *last=&receipt->files[--count];
        if (last->kind!=MCC_SHARE_FILE_SIDECAR || strcmp(last->leaf,"workshop_id.txt") ||
            !last->size || last->size>128) return 0;
        for (i=0;i<count;i++) {
            unsigned j;
            if (receipt->files[i].kind==MCC_SHARE_FILE_SIDECAR) return 0;
            /* The extra sidecar cannot replace a directory used by metadata. */
            for (j=0;j<15;j++) {
                unsigned char c=(unsigned char)receipt->files[i].leaf[j];
                if (c>='A' && c<='Z') c=(unsigned char)(c+'a'-'A');
                if (c!=(unsigned char)"workshop_id.txt"[j]) break;
            }
            if (j==15 && receipt->files[i].leaf[j]=='/') return 0;
        }
    }
    return mcc_share_manifest_valid(receipt->files,count,leaf);
}

static int read_bytes(FILE *file, unsigned char *bytes, unsigned capacity)
{
    size_t got;
    int ok;
    if (!file) return 0;
    got=fread(bytes,1,capacity,file);
    ok=got>0 && fgetc(file)==EOF && !ferror(file);
    if (fclose(file)) ok=0;
    return ok ? (int)got : 0;
}

static int receipt_read(char const *maps, struct mcc_share_io const *io,
    char const *folder, struct storage_receipt *receipt)
{
    unsigned char *bytes;
    char directory[512],path[512];
    unsigned length,count,i;
    int read,ok=0;
    memset(receipt,0,sizeof(*receipt));
    if (!maps || !io || !io->path_safe || !io->directory_exists || !io->open_native_read || !folder_valid(folder) ||
        !path_join(directory,maps,folder) || !io->path_safe(directory) || !io->directory_exists(directory) ||
        !path_join(path,directory,MCC_SHARE_STAGE_RECEIPT)) return 0;
#ifdef HALO_WEB
    if (!web_mcc_directory_visible(directory)) return 0;
#endif
    bytes=malloc(MCC_SHARE_STAGE_COMPANION_BYTES);
    if (!bytes) return 0;
    read=io->path_safe(path) ? read_bytes(io->open_native_read(path),bytes,MCC_SHARE_STAGE_COMPANION_BYTES) : 0;
    if (read<(int)STORAGE_HEADER || memcmp(bytes,storage_magic,8) ||
        memcmp(bytes+8,folder,24)) goto done;
    count=get32(bytes+224);
    if (!count || count>STORAGE_FILES) goto done;
    length=STORAGE_HEADER+count*STORAGE_ENTRY;
    if ((unsigned)read!=length) goto done;
    strcpy(receipt->item.folder,folder);
    memcpy(receipt->item.source,bytes+32,64);
    memcpy(receipt->item.maker,bytes+96,64);
    memcpy(receipt->item.workshop.id,bytes+160,24);
    memcpy(receipt->item.workshop.title,bytes+184,40);
    if (!text_valid(receipt->item.source,64,1) || !text_valid(receipt->item.maker,64,1) ||
        (!workshop_valid(&receipt->item.workshop) &&
         (receipt->item.workshop.id[0] || receipt->item.workshop.title[0]))) goto done;
    receipt->count=(int)count;
    receipt->item.files=count+1;
    receipt->item.bytes=length;
    for (i=0;i<count;i++) {
        unsigned char const *p=bytes+STORAGE_HEADER+i*STORAGE_ENTRY;
        struct mcc_share_file *file=&receipt->files[i];
        file->kind=p[0]; file->size=get32(p+1);
        memcpy(file->sha256,p+5,32); memcpy(file->leaf,p+37,128);
        receipt->item.bytes+=file->size;
    }
    ok=files_valid(receipt);
done:
    free(bytes); return ok;
}

int mcc_share_storage_record(struct mcc_share_stage *stage, char const *source,
    char const *maker, struct mcc_workshop_item const *workshop)
{
    struct storage_receipt *receipt;
    unsigned char *bytes;
    char canonical[64],path[512],sidecar[128];
    int i,sidecar_index=-1,length,ok=0;
    unsigned size;
    if (!stage || stage->state!=MCC_SHARE_STAGE_STAGED || !stage->verified || !stage->owns_directory ||
        stage->count<1 || stage->count>MCC_SHARE_STAGE_FILES ||
        !folder_valid(stage->folder) || !text_valid(maker,64,1) ||
        (workshop && !workshop_valid(workshop)) ||
        !mcc_package_name(source,canonical) || !text_valid(canonical,sizeof(canonical),1) ||
        strcmp(strrchr(canonical,'\\')+1,stage->level)) return 0;
    receipt=calloc(1,sizeof(*receipt)); bytes=calloc(1,MCC_SHARE_STAGE_COMPANION_BYTES);
    if (!receipt || !bytes) goto done;
    strcpy(receipt->item.folder,stage->folder); strcpy(receipt->item.source,canonical); strcpy(receipt->item.maker,maker);
    for (i=0;i<stage->count;i++) if (stage->owned[i]) {
        if (stage->files[i].kind==MCC_SHARE_FILE_SIDECAR) sidecar_index=receipt->count;
        receipt->files[receipt->count++]=stage->files[i];
    }
    if (sidecar_index>=0) {
        if (!path_join(path,stage->stage_dir,receipt->files[sidecar_index].leaf)) goto done;
        /* The active stage uses Xbox virtual paths; native storage paths below
         * must instead use open_native_read to avoid a second drive translation. */
        length=stage->io.path_safe(path) ? read_bytes(fopen(path,"rb"),(unsigned char *)sidecar,sizeof(sidecar)-1) : 0;
        if (!length) goto done;
        sidecar[length]=0;
        if (!mcc_workshop_parse_sidecar(sidecar,&receipt->item.workshop) || !workshop_valid(&receipt->item.workshop)) goto done;
        if (workshop && strcmp(workshop->id,receipt->item.workshop.id)) goto done;
    } else if (workshop) {
        struct mcc_share_file *file;
        if (!workshop_valid(workshop)) goto done;
        length=snprintf(sidecar,sizeof(sidecar),"%s\n%s\n",workshop->id,workshop->title);
        if (length<=0 || length>=(int)sizeof(sidecar) ||
            !mcc_share_stage_companion(stage,"workshop_id.txt",sidecar,(unsigned)length)) goto done;
        strcpy(receipt->item.workshop.id,workshop->id);
        strcpy(receipt->item.workshop.title,workshop->title);
        file=&receipt->files[receipt->count++];
        file->kind=MCC_SHARE_FILE_SIDECAR; strcpy(file->leaf,"workshop_id.txt");
        file->size=stage->companion_size[MCC_SHARE_STAGE_WORKSHOP];
        memcpy(file->sha256,stage->companion_sha256[MCC_SHARE_STAGE_WORKSHOP],32);
    }
    if (!files_valid(receipt)) goto done;
    memcpy(bytes,storage_magic,8); memcpy(bytes+8,receipt->item.folder,24);
    memcpy(bytes+32,receipt->item.source,64); memcpy(bytes+96,receipt->item.maker,64);
    memcpy(bytes+160,receipt->item.workshop.id,24); memcpy(bytes+184,receipt->item.workshop.title,40);
    put32(bytes+224,(uint32_t)receipt->count);
    for (i=0;i<receipt->count;i++) {
        struct mcc_share_file const *file=&receipt->files[i];
        unsigned char *p=bytes+STORAGE_HEADER+(unsigned)i*STORAGE_ENTRY;
        p[0]=file->kind; put32(p+1,file->size);
        memcpy(p+5,file->sha256,32); memcpy(p+37,file->leaf,128);
    }
    size=STORAGE_HEADER+(unsigned)receipt->count*STORAGE_ENTRY;
    ok=mcc_share_stage_companion(stage,MCC_SHARE_STAGE_RECEIPT,bytes,size);
done:
    free(receipt); free(bytes); return ok;
}

int mcc_share_storage_read(char const *maps, struct mcc_share_io const *io,
    char const *folder, struct mcc_share_storage_item *item)
{
    struct storage_receipt *receipt;
    int ok;
    if (!item) return 0;
    memset(item,0,sizeof(*item)); receipt=malloc(sizeof(*receipt));
    if (!receipt) return 0;
    ok=receipt_read(maps,io,folder,receipt);
    if (ok) *item=receipt->item;
    free(receipt); return ok;
}

int mcc_share_storage_list(char const *maps, struct mcc_share_io const *io,
    struct mcc_share_storage_item *items, int capacity, int *truncated)
{
    struct mcc_share_storage_item item;
    char name[256];
    void *directory;
    int count=0,scanned=0;
    if (truncated) *truncated=0;
    if (!maps || !io || !io->path_safe || !io->open_native_read || !items || capacity<1 || capacity>MCC_SHARE_STORAGE_LIMIT ||
        !io->path_safe(maps) || !(directory=posix_directory_open(maps))) return -1;
    while (posix_directory_next(directory,name,sizeof(name))) {
        if (++scanned>4096) { if (truncated) *truncated=1; break; }
        if (!mcc_share_storage_read(maps,io,name,&item)) continue;
        if (count==capacity) { if (truncated) *truncated=1; break; }
        items[count++]=item;
    }
    posix_directory_close(directory);
    return count;
}

int mcc_share_storage_find_resource(char const *maps, struct mcc_share_io const *io,
    struct mcc_share_file const *expected, char path[1024])
{
    struct storage_receipt *receipt;
    char name[256],package[512],candidate[512];
    void *directory;
    int scanned=0,managed=0,i,found=0;
    if (!path) return 0;
    path[0]=0;
    if (!maps || !io || !io->path_safe || !io->directory_exists || !io->open_native_read || !expected ||
        (expected->kind!=MCC_SHARE_FILE_BITMAPS && expected->kind!=MCC_SHARE_FILE_SOUNDS) ||
        !memchr(expected->leaf,0,sizeof(expected->leaf)) || !expected->size ||
        strcmp(expected->leaf,expected->kind==MCC_SHARE_FILE_BITMAPS ? "bitmaps.map" : "sounds.map") ||
        !io->path_safe(maps) || !(directory=posix_directory_open(maps))) return 0;
    receipt=malloc(sizeof(*receipt));
    if (!receipt) { posix_directory_close(directory); return 0; }
    while (!found && posix_directory_next(directory,name,sizeof(name))) {
        if (++scanned>4096) break;
        if (!receipt_read(maps,io,name,receipt)) continue;
        if (++managed>MCC_SHARE_STORAGE_LIMIT) break;
        for (i=0;i<receipt->count;i++) {
            struct mcc_share_file const *file=&receipt->files[i];
            FILE *stream;
            long bytes;
            if (file->kind!=expected->kind || file->size!=expected->size || memcmp(file->sha256,expected->sha256,32)) continue;
            if (!path_join(package,maps,name) || !path_join(candidate,package,file->leaf) ||
                !io->path_safe(candidate) || io->directory_exists(candidate) || !(stream=io->open_native_read(candidate))) continue;
            bytes=fseek(stream,0,SEEK_END) ? -1 : ftell(stream); fclose(stream);
            if (bytes>=0 && (uint32_t)bytes==expected->size) { strcpy(path,candidate); found=1; break; }
        }
    }
    free(receipt); posix_directory_close(directory); return found;
}

static int file_matches(char const *directory, struct mcc_share_io const *io,
    struct mcc_share_file const *expected)
{
    struct mcc_sha256 hash;
    unsigned char bytes[16384],digest[32];
    char path[512];
    FILE *file;
    size_t got;
    uint32_t total=0;
    int ok=1;
    if (!path_join(path,directory,expected->leaf) || !io->path_safe(path) || !(file=io->open_native_read(path))) return 0;
    mcc_sha256_begin(&hash);
    while ((got=fread(bytes,1,sizeof(bytes),file))!=0) {
        if (got>expected->size-total) { ok=0; break; }
        total+=(uint32_t)got; mcc_sha256_add(&hash,bytes,(int)got);
    }
    if (ferror(file) || total!=expected->size) ok=0;
    if (fclose(file)) ok=0;
    mcc_sha256_end(&hash,digest);
    return ok && !memcmp(digest,expected->sha256,32);
}

enum mcc_share_storage_result mcc_share_storage_remove(char const *maps,
    struct mcc_share_io const *io, char const *folder)
{
    struct storage_receipt *receipt;
    char directory[512],path[512],*last;
    int i;
#ifdef HALO_WEB
    int lease=0;
#endif
    enum mcc_share_storage_result result=MCC_SHARE_STORAGE_ERROR;
    if (!io || !io->delete_file || !io->remove_directory) return result;
    receipt=malloc(sizeof(*receipt));
    if (!receipt) return result;
#ifdef HALO_WEB
    if (!web_mcc_remove_begin(folder)) goto done;
    lease=1;
#endif
    if (!receipt_read(maps,io,folder,receipt) || !path_join(directory,maps,folder)) goto done;
    /* Complete verification first: a changed or missing file prevents all deletion. */
    for (i=0;i<receipt->count;i++) if (!file_matches(directory,io,&receipt->files[i])) goto done;
    for (i=0;i<receipt->count;i++) {
        if (!path_join(path,directory,receipt->files[i].leaf) || !io->path_safe(path) || !io->delete_file(path)) goto done;
    }
    for (i=receipt->count-1;i>=0;i--) {
        if (!path_join(path,directory,receipt->files[i].leaf)) goto done;
        while ((last=strrchr(path,'/')) && (unsigned)(last-path)>strlen(directory)) {
            *last=0;
            if (!io->path_safe(path)) goto done;
            io->remove_directory(path); /* A nonempty directory contains added files. */
        }
    }
    if (!path_join(path,directory,MCC_SHARE_STAGE_RECEIPT) || !io->path_safe(path) || !io->delete_file(path)) goto done;
    result=io->remove_directory(directory) ? MCC_SHARE_STORAGE_REMOVED : MCC_SHARE_STORAGE_LEFTOVERS;
done:
#ifdef HALO_WEB
    if (lease) web_mcc_remove_end(folder);
#endif
    free(receipt); return result;
}
