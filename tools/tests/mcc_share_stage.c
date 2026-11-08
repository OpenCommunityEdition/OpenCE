/* Real staging and native no-follow/no-replace operations over disposable files. */
#include "mcc_share_stage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir(p,m) _mkdir(p)
#define rmdir _rmdir
#define S_ISDIR(m) (((m)&_S_IFMT)==_S_IFDIR)
#else
#include <unistd.h>
#endif
int posix_mcc_share_path_safe(char const *path);
int posix_mcc_share_publish(char const *from,char const *to);
static int make(char const *p) { return mkdir(p,0700)==0; }
static int erase_directory(char const *p) { return rmdir(p)==0; }
static int exists(char const *p) { struct stat s;return !stat(p,&s) && S_ISDIR(s.st_mode); }
static int move(char const *a,char const *b) { return posix_mcc_share_publish(a,b); }
static int erase(char const *p) { return remove(p)==0; }
static int verified(void *user,char const *path) { (void)user;(void)path;return 1; }
static struct mcc_share_io io={make,erase_directory,exists,move,erase,posix_mcc_share_path_safe,posix_mcc_share_publish,NULL,NULL};
static char const *direct_mode;
static int direct_pending,direct_committed,direct_flushes;
static int direct_begin(char const *directory,struct mcc_share_file const *files,int count)
{
    char parent[512],*end;
    if (count!=3 || strcmp(files[1].leaf,"metadata/m0.json")) return 0;
    snprintf(parent,sizeof(parent),"%s",directory);end=strrchr(parent,'/');if (!end) return 0;*end=0;
    if ((!exists(parent) && !make(parent)) || !make(directory)) return 0;
    if (!strcmp(direct_mode,"direct_begin_uncertain")) return -1;
    direct_pending=1;return 1;
}
static int direct_finish(char const *directory,int publish)
{
    (void)directory;
    if (!direct_pending) return 0;
    if (publish && !strcmp(direct_mode,"direct_commit_fail")) return 0;
    if (publish && !strcmp(direct_mode,"direct_commit_uncertain")) {
        direct_pending=0;direct_committed=1;return -1;
    }
    direct_pending=0;direct_committed=publish;return 1;
}
static int direct_flush(FILE *file)
{
    direct_flushes++;
    return strcmp(direct_mode,"direct_flush_fail") && !fflush(file);
}
static int never_move(char const *from,char const *to)
{
    (void)from;(void)to;fprintf(stderr,"direct strategy attempted rename\n");exit(10);
}
static struct mcc_share_direct_io const direct_io={direct_begin,direct_finish,direct_flush};
static void entry(struct mcc_share_file *f,unsigned kind,char const *name)
{
    struct mcc_sha256 hash;
    memset(f,0,sizeof(*f));f->kind=(uint8_t)kind;snprintf(f->leaf,sizeof(f->leaf),"%s",name);f->size=1;
    mcc_sha256_begin(&hash);mcc_sha256_add(&hash,"x",1);mcc_sha256_end(&hash,f->sha256);
}
int main(int argc,char **argv)
{
    static struct mcc_share_stage stage;
    struct mcc_share_file files[3];
    int result;
    char root[200];
    if (argc<3) return 2;
    if (!strcmp(argv[1],"safe")) { printf("%d\n",posix_mcc_share_path_safe(argv[2]));return 0; }
    if (!strcmp(argv[1],"publish") && argc==4) { printf("%d\n",posix_mcc_share_publish(argv[2],argv[3]));return 0; }
    entry(files,MCC_SHARE_FILE_MAP,"m0.map");
    entry(files+1,MCC_SHARE_FILE_JSON,argc>3 ? argv[3] : "metadata/m0.json");
    entry(files+2,MCC_SHARE_FILE_PNG,"assets/m0.png");
    if (!strcmp(argv[1],"manifest")) { printf("%d\n",mcc_share_manifest_valid(files,3,"m0"));return 0; }
    if (!strcmp(argv[1],"collision")) {
        entry(files+1,MCC_SHARE_FILE_JSON,"A.json");entry(files+2,MCC_SHARE_FILE_JSON,"a.json/b.json");
        printf("%d\n",mcc_share_manifest_valid(files,3,"m0"));return 0;
    }
    if (!strcmp(argv[1],"budget")) {
        struct mcc_share_file many[6];
        int i;
        entry(many,MCC_SHARE_FILE_MAP,"m0.map");
        for (i=1;i<6;i++) {
            char name[32];
            sprintf(name,"assets/%d.png",i);entry(many+i,MCC_SHARE_FILE_PNG,name);
            many[i].size=MCC_SHARE_MAXIMUM_PNG_BYTES;
        }
        printf("%d %d\n",mcc_share_manifest_valid(many,5,"m0"),mcc_share_manifest_valid(many,6,"m0"));return 0;
    }
    snprintf(root,sizeof(root),"%s/",argv[2]);
    if (!strncmp(argv[1],"direct_",7)) {
        direct_mode=argv[1];io.direct=&direct_io;io.move_file=never_move;io.publish_directory=never_move;
    }
    result=mcc_share_stage_begin(&stage,&io,root,"snapshot","m0",files,3);
    if (result!=MCC_SHARE_STAGE_OK) { printf("begin %d\n",result);return 0; }
    if (!strcmp(argv[1],"partial_collision")) {
        char path[512];
        FILE *file;
        snprintf(path,sizeof(path),"%s/m0.map.part",stage.stage_dir);
        file=fopen(path,"wb");if (!file) return 5;
        if (fwrite("not ours",1,8,file)!=8 || fclose(file)) return 6;
        result=mcc_share_stage_write(&stage,"x",1);
        printf("write %d\n",result);return 0;
    }
    result=mcc_share_stage_write(&stage,"x",1);
    if (io.direct && !strcmp(direct_mode,"direct_flush_fail")) {
        printf("write %d pending %d committed %d\n",result,direct_pending,direct_committed);return 0;
    }
    if (result!=MCC_SHARE_STAGE_FILE_DONE) { printf("write %d\n",result);return 0; }
    result=mcc_share_stage_write(&stage,"x",1);
    if (result!=MCC_SHARE_STAGE_FILE_DONE) { printf("write %d\n",result);return 0; }
    result=mcc_share_stage_write(&stage,"x",1);
    if (result!=MCC_SHARE_STAGE_ALL_DONE) { printf("write %d\n",result);return 0; }
    if (io.direct) {
        if (!direct_pending || direct_committed || direct_flushes!=3) return 11;
        if (!strcmp(direct_mode,"direct_cancel")) {
            mcc_share_stage_cancel(&stage);
            printf("cancel pending %d committed %d\n",direct_pending,direct_committed);return 0;
        }
        if (!strcmp(direct_mode,"direct_unverified")) {
            result=mcc_share_stage_install(&stage);
            if (!direct_pending || direct_committed) return 12;
            mcc_share_stage_cancel(&stage);
            printf("install %d pending %d committed %d\n",result,direct_pending,direct_committed);return 0;
        }
        if (mcc_share_stage_verify(&stage,verified,NULL)!=MCC_SHARE_STAGE_OK ||
            !mcc_share_stage_companion(&stage,"download.receipt","receipt",7) || direct_flushes!=4) return 13;
        result=mcc_share_stage_install(&stage);
        if (!strcmp(direct_mode,"direct_commit_uncertain")) {
            mcc_share_stage_cancel(&stage);mcc_share_stage_cancel(&stage);
            if (!exists(stage.package_dir)) return 14;
        }
        printf("install %d pending %d committed %d\n",result,direct_pending,direct_committed);return 0;
    }
    if (!strcmp(argv[1],"cancel")) { mcc_share_stage_cancel(&stage);printf("cancelled\n");return 0; }
    if (!strcmp(argv[1],"discard")) {
        if (!mcc_share_stage_discard_preview(&stage,2) || mcc_share_stage_discard_preview(&stage,1)) return 3;
    }
    if (!strcmp(argv[1],"unverified")) {
        result=mcc_share_stage_install(&stage);mcc_share_stage_cancel(&stage);printf("install %d\n",result);return 0;
    }
    if (mcc_share_stage_verify(&stage,verified,NULL)!=MCC_SHARE_STAGE_OK) return 4;
    result=mcc_share_stage_install(&stage);printf("install %d\n",result);return 0;
}
