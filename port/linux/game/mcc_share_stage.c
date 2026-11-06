#include "mcc_share_stage.h"
#include "mcc_share_rules.h"
#include <stdio.h>
#include <string.h>

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

/* A plain file name made of the characters hand-placed packages use. The sender's
 * text is never joined to a path before it passes this. */
static int stage_name_plain(char const *name, unsigned limit)
{
    unsigned i;
    if (!name[0] || name[0]=='.' || name[0]==' ') return 0;
    for (i=0;name[i];i++) {
        unsigned char c=(unsigned char)name[i];
        if (i>=limit || c<32 || c>126 || strchr("/\\:*?\"<>|",c)) return 0;
    }
    return name[i-1]!='.' && name[i-1]!=' ';
}

int mcc_share_credit_allowed(char const *leaf, unsigned length)
{
    char name[16];
    if (!leaf || length<9 || length>12) return 0;
    memcpy(name,leaf,length);
    name[length]=0;
    return stage_same(name,"modinfo.json") || stage_same(name,"credits.txt") ||
        stage_same(name,"readme.txt") || stage_same(name,"license.txt");
}

/* The spelling a leaf is written with, whatever case the sender used. */
static char const *stage_canonical(uint8_t kind, char const *leaf, char const *level, char *scratch)
{
    switch (kind) {
    case MCC_SHARE_FILE_BITMAPS: return stage_same(leaf,"bitmaps.map") ? "bitmaps.map" : NULL;
    case MCC_SHARE_FILE_SOUNDS: return stage_same(leaf,"sounds.map") ? "sounds.map" : NULL;
    case MCC_SHARE_FILE_MAP:
        sprintf(scratch,"%s.map",level);
        return stage_same(leaf,scratch) ? scratch : NULL;
    case MCC_SHARE_FILE_CREDIT:
        if (stage_same(leaf,"modinfo.json")) return "ModInfo.json";
        if (stage_same(leaf,"credits.txt")) return "credits.txt";
        if (stage_same(leaf,"readme.txt")) return "readme.txt";
        if (stage_same(leaf,"license.txt")) return "license.txt";
        return NULL;
    }
    return NULL;
}

static int stage_manifest(struct mcc_share_file *out, struct mcc_share_file const *files, int count,
    char const *level)
{
    int i,j,maps=0,bitmaps=0,sounds=0,credits=0;
    char scratch[MCC_SHARE_LEAF_SIZE+8];
    char const *canonical;
    uint64_t total=0;
    if (!files || count<1 || count>MCC_SHARE_STAGE_FILES || !level ||
        !stage_name_plain(level,MCC_SHARE_LEAF_SIZE-5)) return 0;
    for (i=0;i<count;i++) {
        struct mcc_share_file file=files[i];
        file.leaf[MCC_SHARE_LEAF_SIZE-1]=0;
        if (memchr(files[i].leaf,0,MCC_SHARE_LEAF_SIZE)==NULL || !stage_name_plain(file.leaf,MCC_SHARE_LEAF_SIZE-1)) return 0;
        if (file.kind>=MCC_SHARE_FILE_KINDS || !file.size || file.size>MCC_SHARE_MAXIMUM_FILE_BYTES) return 0;
        canonical=stage_canonical(file.kind,file.leaf,level,scratch);
        if (!canonical) return 0;
        /* The rules module's allowlist is the authority for the resource maps. */
        if ((file.kind==MCC_SHARE_FILE_BITMAPS || file.kind==MCC_SHARE_FILE_SOUNDS) &&
            !mcc_share_file_allowed(file.leaf,strlen(file.leaf))) return 0;
        if (file.kind==MCC_SHARE_FILE_CREDIT &&
            (file.size>MCC_SHARE_MAXIMUM_CREDIT_BYTES || !mcc_share_credit_allowed(file.leaf,(unsigned)strlen(file.leaf)))) return 0;
        maps+=file.kind==MCC_SHARE_FILE_MAP;
        bitmaps+=file.kind==MCC_SHARE_FILE_BITMAPS;
        sounds+=file.kind==MCC_SHARE_FILE_SOUNDS;
        credits+=file.kind==MCC_SHARE_FILE_CREDIT;
        for (j=0;j<i;j++) if (stage_same(out[j].leaf,canonical)) return 0;
        total+=file.size;
        strcpy(file.leaf,canonical);
        out[i]=file;
    }
    return maps==1 && bitmaps<=1 && sounds<=1 && credits<=MCC_SHARE_MAXIMUM_CREDITS &&
        total<=MCC_SHARE_MAXIMUM_TOTAL_BYTES;
}

int mcc_share_manifest_valid(struct mcc_share_file const *files, int count, char const *level)
{
    struct mcc_share_file scratch[MCC_SHARE_STAGE_FILES];
    return stage_manifest(scratch,files,count,level);
}

static int stage_path(char *path, unsigned size, char const *directory, char const *leaf, char const *suffix)
{
    int length=snprintf(path,size,"%s/%s%s",directory,leaf,suffix);
    return length>0 && (unsigned)length<size;
}

static void stage_forget(struct mcc_share_stage *stage)
{
    char path[320];
    int i;
    if (stage->stream) {
        fclose(stage->stream);
        stage->stream=NULL;
    }
    /* Without a staging folder there is nothing of ours to delete; a path built
     * from an empty folder would name the root. */
    if (!stage->stage_dir[0]) return;
    for (i=0;i<stage->count;i++) {
        if (stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,".part")) stage->io.delete_file(path);
        if (stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,"")) stage->io.delete_file(path);
    }
    stage->io.remove_directory(stage->stage_dir);
}

static enum mcc_share_stage_result stage_fail(struct mcc_share_stage *stage,
    enum mcc_share_stage_result result)
{
    stage_forget(stage);
    stage->state=MCC_SHARE_STAGE_FAILED;
    return result;
}

enum mcc_share_stage_result mcc_share_stage_begin(struct mcc_share_stage *stage,
    struct mcc_share_io const *io, char const *root, char const *folder, char const *level,
    struct mcc_share_file const *files, int count)
{
    char staging[260];
    int length;
    memset(stage,0,sizeof(*stage));
    if (!io || !io->make_directory || !io->remove_directory || !io->directory_exists || !io->move_file || !io->delete_file || !root || !folder ||
        strlen(root)>=sizeof(stage->root) || !stage_name_plain(folder,40) ||
        !stage_manifest(stage->files,files,count,level)) {
        memset(stage,0,sizeof(*stage));
        return MCC_SHARE_STAGE_BAD_MANIFEST;
    }
    stage->io=*io;
    strcpy(stage->root,root);
    strcpy(stage->folder,folder);
    strcpy(stage->level,level);
    stage->count=count;
    length=snprintf(stage->package_dir,sizeof(stage->package_dir),"%smcc_maps/%s",root,folder);
    if (length<=0 || (unsigned)length>=sizeof(stage->package_dir)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_MANIFEST);
    length=snprintf(staging,sizeof(staging),"%smcc_staging",root);
    if (length<=0 || (unsigned)length>=sizeof(staging)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_MANIFEST);
    /* A taken name is refused before any file of ours exists to clean up. */
    if (io->directory_exists(stage->package_dir)) {
        memset(stage,0,sizeof(*stage));
        return MCC_SHARE_STAGE_EXISTS;
    }
    length=snprintf(stage->stage_dir,sizeof(stage->stage_dir),"%s/%s",staging,folder);
    if (length<=0 || (unsigned)length>=sizeof(stage->stage_dir)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_MANIFEST);
    if (!io->directory_exists(staging)) io->make_directory(staging);
    if (!io->directory_exists(stage->stage_dir) && !io->make_directory(stage->stage_dir)) {
        stage->stage_dir[0]=0;
        return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
    }
    {
        char path[320];
        int i;
        for (i=0;i<count;i++) {
            if (stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,".part")) io->delete_file(path);
            if (stage_path(path,sizeof(path),stage->stage_dir,stage->files[i].leaf,"")) io->delete_file(path);
            stage->total+=stage->files[i].size;
        }
    }
    stage->state=MCC_SHARE_STAGE_WRITING;
    return MCC_SHARE_STAGE_OK;
}

uint32_t mcc_share_stage_wanted(struct mcc_share_stage const *stage)
{
    if (stage->state!=MCC_SHARE_STAGE_WRITING) return 0;
    return stage->files[stage->current].size-stage->written;
}

enum mcc_share_stage_result mcc_share_stage_write(struct mcc_share_stage *stage,
    void const *data, uint32_t bytes)
{
    struct mcc_share_file *file;
    char path[320];
    if (stage->state!=MCC_SHARE_STAGE_WRITING) return MCC_SHARE_STAGE_BAD_STATE;
    file=&stage->files[stage->current];
    if (bytes>file->size-stage->written) return stage_fail(stage,MCC_SHARE_STAGE_TOO_MUCH);
    if (!stage->stream) {
        if (!stage_path(path,sizeof(path),stage->stage_dir,file->leaf,".part") ||
            !(stage->stream=fopen(path,"wb"))) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        mcc_sha256_begin(&stage->hash);
        stage->written=0;
    }
    if (bytes) {
        if (fwrite(data,1,bytes,stage->stream)!=bytes) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        mcc_sha256_add(&stage->hash,data,(int)bytes);
        stage->written+=bytes;
        stage->done+=bytes;
    }
    if (stage->written<file->size) return MCC_SHARE_STAGE_OK;
    {
        unsigned char digest[32];
        char part[320],final[320];
        int closed;
        mcc_sha256_end(&stage->hash,digest);
        closed=fclose(stage->stream);
        stage->stream=NULL;
        if (closed) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
        if (memcmp(digest,file->sha256,32)) return stage_fail(stage,MCC_SHARE_STAGE_BAD_HASH);
        if (!stage_path(part,sizeof(part),stage->stage_dir,file->leaf,".part") ||
            !stage_path(final,sizeof(final),stage->stage_dir,file->leaf,"") ||
            !stage->io.move_file(part,final)) return stage_fail(stage,MCC_SHARE_STAGE_WRITE_FAILED);
    }
    stage->written=0;
    if (++stage->current<stage->count) return MCC_SHARE_STAGE_FILE_DONE;
    stage->state=MCC_SHARE_STAGE_STAGED;
    return MCC_SHARE_STAGE_ALL_DONE;
}

enum mcc_share_stage_result mcc_share_stage_verify(struct mcc_share_stage *stage,
    int (*check)(void *user, char const *stage_dir), void *user)
{
    if (stage->state!=MCC_SHARE_STAGE_STAGED || !check) return MCC_SHARE_STAGE_BAD_STATE;
    if (!check(user,stage->stage_dir)) return stage_fail(stage,MCC_SHARE_STAGE_REFUSED);
    return MCC_SHARE_STAGE_OK;
}

enum mcc_share_stage_result mcc_share_stage_install(struct mcc_share_stage *stage)
{
    char from[320],to[320],maps[260];
    int i,moved[MCC_SHARE_STAGE_FILES],moved_count=0,pass;
    if (stage->state!=MCC_SHARE_STAGE_STAGED) return MCC_SHARE_STAGE_BAD_STATE;
    /* An existing package is never written to: the name is checked again now,
     * and the folder is made fresh. */
    if (stage->io.directory_exists(stage->package_dir)) return stage_fail(stage,MCC_SHARE_STAGE_EXISTS);
    snprintf(maps,sizeof(maps),"%smcc_maps",stage->root);
    if (!stage->io.directory_exists(maps)) stage->io.make_directory(maps);
    if (!stage->io.make_directory(stage->package_dir)) return stage_fail(stage,MCC_SHARE_STAGE_EXISTS);
    for (pass=0;pass<2;pass++) {
        /* The map goes last: a package without its map is not a package. */
        for (i=0;i<stage->count;i++) {
            if ((stage->files[i].kind==MCC_SHARE_FILE_MAP)!=pass) continue;
            if (!stage_path(from,sizeof(from),stage->stage_dir,stage->files[i].leaf,"") ||
                !stage_path(to,sizeof(to),stage->package_dir,stage->files[i].leaf,"") || !stage->io.move_file(from,to)) {
                int k;
                for (k=0;k<moved_count;k++)
                    if (stage_path(to,sizeof(to),stage->package_dir,stage->files[moved[k]].leaf,"")) stage->io.delete_file(to);
                stage->io.remove_directory(stage->package_dir);
                return stage_fail(stage,MCC_SHARE_STAGE_INSTALL_FAILED);
            }
            moved[moved_count++]=i;
        }
    }
    stage->io.remove_directory(stage->stage_dir);
    stage->state=MCC_SHARE_STAGE_INSTALLED;
    return MCC_SHARE_STAGE_OK;
}

void mcc_share_stage_cancel(struct mcc_share_stage *stage)
{
    if (stage->state==MCC_SHARE_STAGE_IDLE) return;
    if (stage->state==MCC_SHARE_STAGE_INSTALLED) {
        stage->io.remove_directory(stage->stage_dir);
        return;
    }
    stage_forget(stage);
    stage->state=MCC_SHARE_STAGE_FAILED;
}
