/* Real WasmFS/OPFS staging, without game data or a game process. */
#include "mcc_share_stage.h"
#include "web_mcc_storage.h"
#include <emscripten.h>
#include <emscripten/wasmfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int posix_mcc_share_path_safe(char const *);
static char const folder[]="dl_0123456789abcdef0123";
static char const directory[]="/data/mcc_maps/dl_0123456789abcdef0123";
static char const level[]="mcc_maps\\dl_0123456789abcdef0123\\m0";
static char const *contents[]={"mapdata!","{}","png!","3021510694\n"};
static int make(char const *p) { return mkdir(p,0700)==0; }
static int remove_dir(char const *p) { return rmdir(p)==0; }
static int exists(char const *p) { struct stat s;return !lstat(p,&s) && S_ISDIR(s.st_mode); }
static int erase(char const *p) { return unlink(p)==0; }
static int never_move(char const *a,char const *b) { (void)a;(void)b;abort();return 0; }
static int (*normal_flush)(FILE *);
/* Shorten only the operation whose response this case deliberately loses.
 * Ordinary OPFS work retains the production deadline, including slow BEGIN. */
static int storage_wait_ms=30000;
int mcc_storage_test_wait_ms(void) { return storage_wait_ms; }
static int failed_flush(FILE *file) {
    if (fflush(file) || close(fileno(file))) return 0;
    return normal_flush(file); /* Actual WasmFS fsync/close now see EBADF. */
}
void platform_translate_path(char const *p,char *out,unsigned long size) {
    unsigned i;
    if (strncmp(p,"d:\\",3)) abort();
    snprintf(out,size,"/data/%s",p+3);
    for (i=0;out[i];i++) if (out[i]=='\\') out[i]='/';
}
static int check_file(char const *leaf,char const *expected) {
    char path[256],bytes[32];
    FILE *f;
    size_t count;
    snprintf(path,sizeof(path),"%s/%s",directory,leaf);f=fopen(path,"rb");if (!f) return 0;
    count=fread(bytes,1,sizeof(bytes),f);
    if (fclose(f)) return 0;
    return count==strlen(expected) && !memcmp(bytes,expected,count);
}
static int verify(void *user,char const *path) {
    (void)user;
    return !strcmp(path,directory) && check_file("m0.map",contents[0]) &&
        check_file("Metadata/Mission.json",contents[1]) && check_file("assets/p.png",contents[2]);
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"check failed at %d: %s\n",__LINE__,#x);return 1; } } while (0)
int main(int argc,char **argv) {
    backend_t opfs;
    struct mcc_share_io io={make,remove_dir,exists,never_move,erase,posix_mcc_share_path_safe,never_move,NULL,NULL};
    struct mcc_share_stage stage;
    struct mcc_share_direct_io failed_io;
    struct mcc_share_file files[4];
    char const *names[]={"m0.map","Metadata/Mission.json","assets/p.png","workshop_id.txt"};
    unsigned kinds[]={MCC_SHARE_FILE_MAP,MCC_SHARE_FILE_JSON,MCC_SHARE_FILE_PNG,MCC_SHARE_FILE_SIDECAR};
    int i,result,count;
    CHECK(argc==2);
    count=!strcmp(argv[1],"sidecar") ? 4 : 3;
    opfs=wasmfs_create_opfs_backend();CHECK(opfs && !wasmfs_create_directory("/data",0777,opfs));
    web_mcc_share_io(&io);
    CHECK(web_mcc_free_bytes()>0);
    if (!strcmp(argv[1],"recovered")) {
        CHECK(!exists(directory));CHECK(web_mcc_package_visible(level));puts("PASS recovered");return 0;
    }
    if (!strcmp(argv[1],"reload") || !strcmp(argv[1],"sidecar_reload")) {
        CHECK(exists(directory));CHECK(web_mcc_package_visible(level));CHECK(verify(NULL,directory));
        CHECK(check_file("download.receipt","receipt"));
        if (!strcmp(argv[1],"sidecar_reload")) CHECK(check_file("workshop_id.txt",contents[3]));
        printf("PASS %s\n",argv[1]);return 0;
    }
    memset(files,0,sizeof(files));
    for (i=0;i<count;i++) {
        struct mcc_sha256 hash;
        files[i].kind=(uint8_t)kinds[i];strcpy(files[i].leaf,names[i]);files[i].size=(uint32_t)strlen(contents[i]);
        mcc_sha256_begin(&hash);mcc_sha256_add(&hash,contents[i],(int)files[i].size);mcc_sha256_end(&hash,files[i].sha256);
    }
    if (!strcmp(argv[1],"begin_reply_lost") || !strcmp(argv[1],"begin_proxy_delayed")) storage_wait_ms=1000;
    result=mcc_share_stage_begin(&stage,&io,"/data/",folder,"m0",files,count);
    storage_wait_ms=30000;
    if (!strcmp(argv[1],"begin_proxy_delayed")) {
        CHECK(result==MCC_SHARE_STAGE_STORAGE_UNCERTAIN);mcc_share_stage_cancel(&stage);
        CHECK(!exists(directory));puts("PASS begin_proxy_delayed");return 0;
    }
    if (!strcmp(argv[1],"exists")) {
        CHECK(result==MCC_SHARE_STAGE_EXISTS);CHECK(verify(NULL,directory));puts("PASS exists");return 0;
    }
    if (!strcmp(argv[1],"quota_refused")) {
        CHECK(result==MCC_SHARE_STAGE_WRITE_FAILED);CHECK(!exists(directory));puts("PASS quota_refused");return 0;
    }
    if (!strcmp(argv[1],"begin_reply_lost")) {
        CHECK(result==MCC_SHARE_STAGE_STORAGE_UNCERTAIN);mcc_share_stage_cancel(&stage);
        CHECK(exists(directory));puts("PASS begin_reply_lost");return 0;
    }
    CHECK(result==MCC_SHARE_STAGE_OK);
    CHECK(!web_mcc_package_visible(level));CHECK(!web_mcc_directory_visible(directory));
    if (!strcmp(argv[1],"flush_failure")) {
        failed_io=*stage.io.direct;normal_flush=failed_io.flush;failed_io.flush=failed_flush;stage.io.direct=&failed_io;
        CHECK(mcc_share_stage_write(&stage,contents[0],files[0].size)==MCC_SHARE_STAGE_WRITE_FAILED);
        CHECK(!exists(directory));CHECK(web_mcc_package_visible(level));puts("PASS flush_failure");return 0;
    }
    CHECK(mcc_share_stage_write(&stage,contents[0],files[0].size)==MCC_SHARE_STAGE_FILE_DONE);
    CHECK(mcc_share_stage_write(&stage,contents[1],1)==MCC_SHARE_STAGE_OK);
    if (!strcmp(argv[1],"crash")) {
        CHECK(!fflush(stage.stream) && !fsync(fileno(stage.stream)));
        puts("PASS crash ready");emscripten_exit_with_live_runtime();
    }
    if (!strcmp(argv[1],"cancel")) {
        mcc_share_stage_cancel(&stage);CHECK(!exists(directory));CHECK(web_mcc_package_visible(level));
        puts("PASS cancel");return 0;
    }
    CHECK(mcc_share_stage_write(&stage,contents[1]+1,1)==MCC_SHARE_STAGE_FILE_DONE);
    CHECK(mcc_share_stage_write(&stage,contents[2],files[2].size)==
        (count==4 ? MCC_SHARE_STAGE_FILE_DONE : MCC_SHARE_STAGE_ALL_DONE));
    if (count==4) CHECK(mcc_share_stage_write(&stage,contents[3],files[3].size)==MCC_SHARE_STAGE_ALL_DONE);
    CHECK(!web_mcc_package_visible(level));
    CHECK(mcc_share_stage_verify(&stage,verify,NULL)==MCC_SHARE_STAGE_OK);
    CHECK(mcc_share_stage_companion(&stage,"download.receipt","receipt",7));
    if (!strcmp(argv[1],"commit_reply_lost") || !strcmp(argv[1],"commit_reply_delayed")) storage_wait_ms=1000;
    result=mcc_share_stage_install(&stage);
    storage_wait_ms=30000;
    if (!strcmp(argv[1],"commit_reply_lost") || !strcmp(argv[1],"commit_reply_delayed")) {
        CHECK(result==MCC_SHARE_STAGE_STORAGE_UNCERTAIN);
        mcc_share_stage_cancel(&stage);mcc_share_stage_cancel(&stage);
        CHECK(verify(NULL,directory));CHECK(check_file("download.receipt","receipt"));
        CHECK(!web_mcc_package_visible(level));printf("PASS %s\n",argv[1]);return 0;
    }
    CHECK(result==MCC_SHARE_STAGE_OK);
    CHECK(web_mcc_package_visible(level));CHECK(web_mcc_directory_visible(directory));
    CHECK(verify(NULL,directory));
    if (count==4) CHECK(check_file("workshop_id.txt",contents[3]));
    printf("PASS %s\n",count==4 ? "sidecar" : "install");return 0;
}
