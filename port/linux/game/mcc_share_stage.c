/* Only directories created by this transfer are writable or removable. */
#include "mcc_share_stage.h"
#include "mcc_share_rules.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char const *stage_companions[MCC_SHARE_STAGE_COMPANIONS]={
    "workshop_id.txt", MCC_SHARE_STAGE_RECEIPT
};

static int stage_same(char const *a, char const *b)
{
    for (;*a && *b;a++,b++) {
        unsigned char x=(unsigned char)*a,y=(unsigned char)*b;
        if (x>='A' && x<='Z') x=(unsigned char)(x+'a'-'A');
        if (y>='A' && y<='Z') y=(unsigned char)(y+'a'-'A');
        if (x!=y) return 0;
    }
    return !*a && !*b;
}

static int stage_component(char const *name, unsigned length)
{
    char base[128];
    unsigned i,n=0;
    if (!length || length>=sizeof(base) || name[0]=='.' || name[0]==' ' ||
        name[length-1]=='.' || name[length-1]==' ') return 0;
    for (i=0;i<length;i++) {
        unsigned char c=(unsigned char)name[i];
        if (c<32 || c>126 || strchr("/\\:*?\"<>|",c)) return 0;
    }
    while (n<length && name[n]!='.') n++;
    while (n && name[n-1]==' ') n--;
    memcpy(base,name,n); base[n]=0;
    if (stage_same(base,"con") || stage_same(base,"prn") || stage_same(base,"aux") ||
        stage_same(base,"nul") || stage_same(base,"conin$") || stage_same(base,"conout$")) return 0;
    if (n==4 && base[3]>='0' && base[3]<='9') {
        base[3]=0;
        if (stage_same(base,"com") || stage_same(base,"lpt")) return 0;
    }
    return 1;
}

static int stage_relative(char *out, char const *name, unsigned size, int nested)
{
    unsigned i,start=0;
    if (!name || !memchr(name,0,size)) return 0;
    for (i=0;name[i];i++) {
        char c=name[i];
        if (c=='/' || c=='\\') {
            if (!nested || !stage_component(name+start,i-start)) return 0;
            start=i+1; c='/';
        }
        out[i]=c;
    }
    out[i]=0;
    return stage_component(name+start,i-start);
}

int mcc_share_credit_allowed(char const *leaf, unsigned length)
{
    char name[16];
    if (!leaf || length<9 || length>12 || memchr(leaf,0,length)) return 0;
    memcpy(name,leaf,length); name[length]=0;
    return stage_same(name,"credits.txt") || stage_same(name,"readme.txt") || stage_same(name,"license.txt");
}

static int stage_manifest(struct mcc_share_file *out, struct mcc_share_file const *files, int count,
    char const *level)
{
    int i,j,maps=0,bitmaps=0,sounds=0,credits=0,sidecars=0;
    char map[MCC_SHARE_LEAF_SIZE],name[MCC_SHARE_LEAF_SIZE];
    uint64_t total=0,metadata=0;
    size_t length;
    if (!files || count<1 || count>MCC_SHARE_STAGE_FILES || !level) return 0;
    length=strlen(level);
    if (length>=MCC_SHARE_LEAF_SIZE-4 || !stage_component(level,(unsigned)length)) return 0;
    sprintf(map,"%s.map",level);
    for (i=0;i<count;i++) {
        struct mcc_share_file file=files[i];
        char const *extension;
        if (!stage_relative(name,file.leaf,sizeof(file.leaf),file.kind==MCC_SHARE_FILE_JSON || file.kind==MCC_SHARE_FILE_PNG)) return 0;
        if (file.kind>=MCC_SHARE_FILE_KINDS || !file.size || file.size>MCC_SHARE_MAXIMUM_FILE_BYTES) return 0;
        extension=strrchr(name,'.');
        switch (file.kind) {
        case MCC_SHARE_FILE_MAP:
            if (!stage_same(name,map)) return 0;
            strcpy(name,map); maps++; break;
        case MCC_SHARE_FILE_BITMAPS:
            if (!stage_same(name,"bitmaps.map")) return 0;
            strcpy(name,"bitmaps.map"); bitmaps++; break;
        case MCC_SHARE_FILE_SOUNDS:
            if (!stage_same(name,"sounds.map")) return 0;
            strcpy(name,"sounds.map"); sounds++; break;
        case MCC_SHARE_FILE_CREDIT:
            if (file.size>MCC_SHARE_MAXIMUM_CREDIT_BYTES || !mcc_share_credit_allowed(name,(unsigned)strlen(name))) return 0;
            credits++; break;
        case MCC_SHARE_FILE_JSON:
            if (!extension || !stage_same(extension,".json") || file.size>MCC_SHARE_MAXIMUM_JSON_BYTES) return 0;
            metadata+=file.size; break;
        case MCC_SHARE_FILE_PNG:
            if (!extension || !stage_same(extension,".png") || file.size>MCC_SHARE_MAXIMUM_PNG_BYTES) return 0;
            metadata+=file.size; break;
        case MCC_SHARE_FILE_SIDECAR:
            if (!stage_same(name,"workshop_id.txt") || file.size>128) return 0;
            strcpy(name,"workshop_id.txt"); sidecars++; break;
        default: return 0;
        }
        /* Resource ownership is also checked by the existing rules. */
        if ((file.kind==MCC_SHARE_FILE_BITMAPS || file.kind==MCC_SHARE_FILE_SOUNDS) &&
            !mcc_share_file_allowed(name,strlen(name))) return 0;
        for (j=0;j<i;j++) {
            char prefix[MCC_SHARE_LEAF_SIZE];
            unsigned k;
            if (stage_same(out[j].leaf,name)) return 0;
            /* A file cannot also be a directory, including case aliases. */
            strcpy(prefix,name);
            for (k=0;prefix[k];k++) if (prefix[k]=='/') {
                prefix[k]=0;
                if (stage_same(out[j].leaf,prefix)) return 0;
                prefix[k]='/';
            }
            strcpy(prefix,out[j].leaf);
            for (k=0;prefix[k];k++) if (prefix[k]=='/') {
                prefix[k]=0;
                if (stage_same(prefix,name)) return 0;
                prefix[k]='/';
            }
        }
        total+=file.size;
        strcpy(file.leaf,name); out[i]=file;
    }
    return maps==1 && bitmaps<=1 && sounds<=1 && credits<=MCC_SHARE_MAXIMUM_CREDITS && sidecars<=1 &&
        total<=MCC_SHARE_MAXIMUM_TOTAL_BYTES && metadata<=MCC_SHARE_MAXIMUM_METADATA_BYTES;
}

int mcc_share_manifest_valid(struct mcc_share_file const *files, int count, char const *level)
{
    struct mcc_share_file *scratch=malloc(sizeof(*scratch)*MCC_SHARE_STAGE_FILES);
    int valid;
    if (!scratch) return 0;
    valid=stage_manifest(scratch,files,count,level);
    free(scratch);
    return valid;
}

static int stage_path(char *path, unsigned size, char const *directory, char const *leaf, char const *suffix)
{
    int length=snprintf(path,size,"%s/%s%s",directory,leaf,suffix);
    return length>0 && (unsigned)length<size;
}

static void stage_remove_parents(struct mcc_share_stage *stage, char const *leaf)
{
    char path[512],*last;
    if (!stage_path(path,sizeof(path),stage->stage_dir,leaf,"")) return;
    while ((last=strrchr(path,'/')) && (unsigned)(last-path)>strlen(stage->stage_dir)) {
        *last=0;
        if (!stage->io.path_safe(path)) return;
        stage->io.remove_directory(path);
    }
}

static void stage_forget(struct mcc_share_stage *stage)
{
    char path[512];
    int i;
    if (stage->cancel_check) {
        void (*cancel)(void *)=stage->cancel_check;
        stage->cancel_check=NULL;
        cancel(stage->check_user);
    }
    if (stage->stream) { fclose(stage->stream); stage->stream=NULL; }
    if (!stage->owns_directory || !stage->stage_dir[0] || !stage->io.path_safe(stage->stage_dir)) goto finish;
    for (i=0;i<stage->count;i++) {
        if (i==stage->current && stage->owns_partial &&
            stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,stage->io.direct ? "" : ".part") && stage->io.path_safe(path))
            stage->io.delete_file(path);
        if (stage->owned[i] && stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,"") && stage->io.path_safe(path))
            stage->io.delete_file(path);
    }
    for (i=stage->count-1;i>=0;i--) stage_remove_parents(stage,stage->files[i].leaf);
    for (i=0;i<MCC_SHARE_STAGE_COMPANIONS;i++)
        if (stage->companion_owned[i] && stage_path(path,sizeof(path),stage->stage_dir,stage_companions[i],"") &&
            stage->io.path_safe(path)) stage->io.delete_file(path);
    stage->io.remove_directory(stage->stage_dir);
    stage->owns_directory=0; stage->owns_partial=0;
finish:
    if (stage->direct_active) {
        stage->io.direct->finish(stage->stage_dir,0);
        stage->direct_active=0;
    }
}

/* A staging folder of this name left by a download that was cut off (the game
 * crashed or was closed): its name comes from the offer, so every retry of
 * the offer meets it. Delete the files this manifest names and their folders,
 * never through a link; a folder holding anything else stays, and the stage
 * fails as before. */
static int stage_clear_leftover(struct mcc_share_stage *stage)
{
    char path[512];
    int i;
    if (!stage->io.path_safe(stage->stage_dir) || !stage->io.directory_exists(stage->stage_dir)) return 0;
    for (i=0;i<stage->count;i++) {
        if (stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,".part") && stage->io.path_safe(path))
            stage->io.delete_file(path);
        if (stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,"") && stage->io.path_safe(path))
            stage->io.delete_file(path);
    }
    for (i=stage->count-1;i>=0;i--) stage_remove_parents(stage,stage->files[i].leaf);
    for (i=0;i<MCC_SHARE_STAGE_COMPANIONS;i++)
        if (stage_path(path,sizeof(path),stage->stage_dir,stage_companions[i],"") && stage->io.path_safe(path))
            stage->io.delete_file(path);
    return stage->io.remove_directory(stage->stage_dir);
}

static enum mcc_share_stage_result stage_fail(struct mcc_share_stage *stage, enum mcc_share_stage_result result)
{
    stage_forget(stage); stage->state=MCC_SHARE_STAGE_FAILED; return result;
}

static int stage_parents(struct mcc_share_stage *stage, char const *leaf)
{
    char path[512];
    size_t i,start=strlen(stage->stage_dir)+1;
    if (!stage_path(path,sizeof(path),stage->stage_dir,leaf,"")) return 0;
    for (i=start;path[i];i++) if (path[i]=='/') {
        path[i]=0;
        if (!stage->io.path_safe(path) || (!stage->io.directory_exists(path) && !stage->io.make_directory(path))) return 0;
        path[i]='/';
    }
    return stage->io.path_safe(path);
}

enum mcc_share_stage_result mcc_share_stage_begin(struct mcc_share_stage *stage,
    struct mcc_share_io const *io, char const *root, char const *folder, char const *level,
    struct mcc_share_file const *files, int count)
{
    char staging[260];
    int length,i;
    memset(stage,0,sizeof(*stage));
    if (!io || !io->make_directory || !io->remove_directory || !io->directory_exists || !io->move_file ||
        !io->delete_file || !io->path_safe || !io->publish_directory ||
        (io->direct && (!io->direct->begin || !io->direct->finish || !io->direct->flush)) || !root || !folder ||
        strlen(root)>=sizeof(stage->root) || strlen(folder)>40 || !stage_component(folder,(unsigned)strlen(folder)) ||
        !stage_manifest(stage->files,files,count,level)) return MCC_SHARE_STAGE_BAD_MANIFEST;
    stage->io=*io;
    strcpy(stage->root,root); strcpy(stage->folder,folder); strcpy(stage->level,level); stage->count=count;
    length=snprintf(stage->package_dir,sizeof(stage->package_dir),"%smcc_maps/%s",root,folder);
    if (length<=0 || (unsigned)length>=sizeof(stage->package_dir)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_MANIFEST);
    length=snprintf(staging,sizeof(staging),"%smcc_staging",root);
    if (length<=0 || (unsigned)length>=sizeof(staging)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_MANIFEST);
    if (!io->path_safe(stage->package_dir) || io->directory_exists(stage->package_dir)) return stage_fail(stage,MCC_SHARE_STAGE_EXISTS);
    if (io->direct) {
        int begun;
        strcpy(stage->stage_dir,stage->package_dir);
        begun=io->direct->begin(stage->stage_dir,stage->files,count);
        if (begun<0) { stage->state=MCC_SHARE_STAGE_FAILED;return MCC_SHARE_STAGE_STORAGE_UNCERTAIN; }
        if (!begun) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        stage->direct_active=1;
    } else {
        length=snprintf(stage->stage_dir,sizeof(stage->stage_dir),"%s/%s",staging,folder);
        if (length<=0 || (unsigned)length>=sizeof(stage->stage_dir)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_MANIFEST);
        if (!io->path_safe(staging) || (!io->directory_exists(staging) && !io->make_directory(staging)) ||
            !io->path_safe(stage->stage_dir) ||
            (!io->make_directory(stage->stage_dir) &&
            (!stage_clear_leftover(stage) || !io->make_directory(stage->stage_dir))))
            return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
    }
    stage->owns_directory=1;
    for (i=0;i<count;i++) stage->total+=stage->files[i].size;
    stage->state=MCC_SHARE_STAGE_WRITING;
    return MCC_SHARE_STAGE_OK;
}

uint32_t mcc_share_stage_wanted(struct mcc_share_stage const *stage)
{
    return stage->state==MCC_SHARE_STAGE_WRITING ? stage->files[stage->current].size-stage->written : 0;
}

enum mcc_share_stage_result mcc_share_stage_write(struct mcc_share_stage *stage, void const *data, uint32_t bytes)
{
    struct mcc_share_file *file;
    char path[512];
    if (stage->state!=MCC_SHARE_STAGE_WRITING) return MCC_SHARE_STAGE_BAD_STATE;
    file=&stage->files[stage->current];
    if (bytes>file->size-stage->written || (bytes && !data)) return stage_fail(stage,MCC_SHARE_STAGE_TOO_MUCH);
    if (!stage->stream) {
        if (!stage_parents(stage,file->leaf) || !stage_path(path,sizeof(path),stage->stage_dir,file->leaf,stage->io.direct ? "" : ".part") ||
            !stage->io.path_safe(path) || !(stage->stream=fopen(path,"wbx"))) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        stage->owns_partial=1;
        mcc_sha256_begin(&stage->hash); stage->written=0;
    }
    if (bytes) {
        if (fwrite(data,1,bytes,stage->stream)!=bytes) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        mcc_sha256_add(&stage->hash,data,(int)bytes); stage->written+=bytes; stage->done+=bytes;
    }
    if (stage->written<file->size) return MCC_SHARE_STAGE_OK;
    {
        unsigned char digest[32];
        char part[512],final[512];
        int closed;
        mcc_sha256_end(&stage->hash,digest);
        closed=stage->io.direct && !stage->io.direct->flush(stage->stream);
        if (fclose(stage->stream)) closed=1;
        stage->stream=NULL;
        if (closed) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        if (memcmp(digest,file->sha256,32)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_HASH);
        if (!stage->io.direct && (!stage_path(part,sizeof(part),stage->stage_dir,file->leaf,".part") ||
            !stage_path(final,sizeof(final),stage->stage_dir,file->leaf,"") ||
            !stage->io.path_safe(part) || !stage->io.path_safe(final) || !stage->io.move_file(part,final)))
            return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        stage->owned[stage->current]=1; stage->owns_partial=0;
    }
    stage->written=0;
    if (++stage->current<stage->count) return MCC_SHARE_STAGE_FILE_DONE;
    stage->state=MCC_SHARE_STAGE_STAGED;
    return MCC_SHARE_STAGE_ALL_DONE;
}

enum mcc_share_stage_result mcc_share_stage_verify(struct mcc_share_stage *stage,
    int (*check)(void *user, char const *stage_dir), void *user)
{
    int result;
    if (stage->state!=MCC_SHARE_STAGE_STAGED || !check) return MCC_SHARE_STAGE_BAD_STATE;
    stage->verified=0;
    result=check(user,stage->stage_dir);
    if (result==MCC_SHARE_CHECK_PENDING) return MCC_SHARE_STAGE_PENDING;
    if (result!=MCC_SHARE_CHECK_READY) return stage_fail(stage,MCC_SHARE_STAGE_REFUSED);
    stage->verified=1;
    return MCC_SHARE_STAGE_OK;
}

int mcc_share_stage_discard_preview(struct mcc_share_stage *stage, int index)
{
    char path[512];
    if (stage->state!=MCC_SHARE_STAGE_STAGED || index<0 || index>=stage->count ||
        stage->files[index].kind!=MCC_SHARE_FILE_PNG || !stage->owned[index]) return 0;
    if (!stage_path(path,sizeof(path),stage->stage_dir,stage->files[index].leaf,"") ||
        !stage->io.path_safe(path) || !stage->io.delete_file(path)) return 0;
    stage->owned[index]=0;
    stage_remove_parents(stage,stage->files[index].leaf);
    return 1;
}

int mcc_share_stage_companion(struct mcc_share_stage *stage, char const *leaf,
    void const *data, unsigned bytes)
{
    char path[512];
    struct mcc_sha256 hash;
    FILE *file;
    int i,ok;
    if (!stage || stage->state!=MCC_SHARE_STAGE_STAGED || !stage->verified ||
        !stage->owns_directory || !leaf || !data || !bytes) return 0;
    for (i=0;i<MCC_SHARE_STAGE_COMPANIONS;i++) if (!strcmp(leaf,stage_companions[i])) break;
    if (i==MCC_SHARE_STAGE_COMPANIONS || stage->companion_owned[i] ||
        bytes>(i==MCC_SHARE_STAGE_WORKSHOP ? 128u : MCC_SHARE_STAGE_COMPANION_BYTES) ||
        !stage_path(path,sizeof(path),stage->stage_dir,leaf,"") || !stage->io.path_safe(path)) return 0;
    file=fopen(path,"wbx");
    if (!file) return 0;
    stage->companion_owned[i]=1;
    ok=fwrite(data,1,bytes,file)==bytes;
    if (ok && stage->io.direct && !stage->io.direct->flush(file)) ok=0;
    if (fclose(file)) ok=0;
    if (!ok) { stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED); return 0; }
    stage->companion_size[i]=bytes;
    mcc_sha256_begin(&hash); mcc_sha256_add(&hash,data,(int)bytes);
    mcc_sha256_end(&hash,stage->companion_sha256[i]);
    return 1;
}

static int stage_file_present(struct mcc_share_stage *stage, char const *leaf, uint32_t size)
{
    char path[512];
    FILE *file;
    long bytes;
    if (!stage_path(path,sizeof(path),stage->stage_dir,leaf,"") || !stage->io.path_safe(path)) return 0;
    file=fopen(path,"rb");
    if (!file) return 0;
    bytes=fseek(file,0,SEEK_END) ? -1 : ftell(file);
    fclose(file);
    return bytes>=0 && (uint32_t)bytes==size;
}

enum mcc_share_stage_result mcc_share_stage_install(struct mcc_share_stage *stage)
{
    char maps[260];
    int i;
    if (stage->state!=MCC_SHARE_STAGE_STAGED || !stage->verified) return MCC_SHARE_STAGE_BAD_STATE;
    for (i=0;i<stage->count;i++) if (stage->owned[i] &&
        !stage_file_present(stage,stage->files[i].leaf,stage->files[i].size))
        return stage_fail(stage,MCC_SHARE_STAGE_INSTALL_FAILED);
    for (i=0;i<MCC_SHARE_STAGE_COMPANIONS;i++) if (stage->companion_owned[i] &&
        !stage_file_present(stage,stage_companions[i],stage->companion_size[i]))
        return stage_fail(stage,MCC_SHARE_STAGE_INSTALL_FAILED);
    if (!stage->io.path_safe(stage->package_dir) || (!stage->io.direct && stage->io.directory_exists(stage->package_dir)))
        return stage_fail(stage,MCC_SHARE_STAGE_EXISTS);
    if (stage->io.direct) {
        int published=stage->direct_active ? stage->io.direct->finish(stage->stage_dir,1) : 0;
        if (published<0) {
            /* A late durable commit must never race live cancellation. The
             * browser has fenced mutations; startup recovery owns this record. */
            stage->direct_active=0;stage->owns_directory=0;
            stage->state=MCC_SHARE_STAGE_FAILED;
            return MCC_SHARE_STAGE_STORAGE_UNCERTAIN;
        }
        if (!published)
            return stage_fail(stage,MCC_SHARE_STAGE_INSTALL_FAILED);
        stage->direct_active=0;
    } else {
        snprintf(maps,sizeof(maps),"%smcc_maps",stage->root);
        if (!stage->io.path_safe(maps) || (!stage->io.directory_exists(maps) && !stage->io.make_directory(maps)) ||
            !stage->io.path_safe(stage->stage_dir) || !stage->io.publish_directory(stage->stage_dir,stage->package_dir))
            return stage_fail(stage,MCC_SHARE_STAGE_INSTALL_FAILED);
    }
    stage->owns_directory=0; stage->state=MCC_SHARE_STAGE_INSTALLED;
    return MCC_SHARE_STAGE_OK;
}

void mcc_share_stage_cancel(struct mcc_share_stage *stage)
{
    if (stage->state==MCC_SHARE_STAGE_IDLE || stage->state==MCC_SHARE_STAGE_INSTALLED) return;
    stage_forget(stage); stage->state=MCC_SHARE_STAGE_FAILED;
}
