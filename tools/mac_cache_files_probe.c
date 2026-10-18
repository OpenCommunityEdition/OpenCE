#include "cseries.h"
#include "cache/cache_files.h"
#include "engine_cache_files.h"
#undef memcpy
#undef memset
#undef memcmp
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static void put32(unsigned char *p,unsigned long value)
{ p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);p[2]=(unsigned char)(value>>16);p[3]=(unsigned char)(value>>24); }
static unsigned char *make_cache(unsigned char *p,size_t *size)
{
    const unsigned long base=0x40440000UL,index=0x800UL,scenario=0x200UL,refs=0x800UL;
    memset(p,0,0x3000);*size=0x3000;
    put32(p,0x68656164);put32(p+4,7);put32(p+8,*size);put32(p+16,index);put32(p+20,0x1000);
    memcpy(p+0x20,"Synthetic native v7",20);memcpy(p+0x40,"test build",11);put32(p+0x7fc,0x666f6f74);
    put32(p+index,base+0x40);put32(p+index+4,0x12340000);put32(p+index+12,2);
    put32(p+index+20,0x1800);put32(p+index+32,0x100);put32(p+index+36,0x74616773);
    put32(p+index+0x40,0x73636e72);put32(p+index+0x4c,0x12340000);
    put32(p+index+0x50,base+0x100);put32(p+index+0x54,base+scenario);
    put32(p+index+0x60,0x73627370);put32(p+index+0x6c,0x12340001);put32(p+index+0x70,base+0x110);
    memcpy(p+index+0x100,"levels/test",12);memcpy(p+index+0x110,"levels/test",12);
    put32(p+index+scenario+0x5a4,1);put32(p+index+scenario+0x5a8,base+refs);
    put32(p+index+refs,0x1900);put32(p+index+refs+4,0x400);put32(p+index+refs+8,base+0x2000);
    put32(p+index+refs+0x1c,0x12340001);
    put32(p+0x1900,base+0x2018);put32(p+0x1914,0x73627370);
    return p;
}
static void expect_abort(int which)
{
    pid_t child=fork();int status=0;
    CHECK(child>=0);if(child<0)return;
    if(!child){
        struct rlimit limit={0,0};char data[16];boolean done=FALSE;
        setrlimit(RLIMIT_CORE,&limit);
        if(which==0)cache_file_read(NONE,-1,4,data,&done,FALSE);
        else if(which==1)cache_file_read(NONE,0,-1,data,&done,TRUE);
        else if(which==2)cache_file_read(NONE,MAC_CACHE_DECODED_BASE,4,data,&done,FALSE);
        else if(which==3)cache_file_read(NONE,0,4,data,NULL,FALSE);
        else if(which==4)cache_file_promote_read(1);
        else if(which==5)cache_file_read(NONE,0,4,data,&done,FALSE);
        _exit(9);
    }
    CHECK(waitpid(child,&status,0)==child);CHECK(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT);
}
int main(int argc,char **argv)
{
    char path[]="/tmp/opence-cache-XXXXXX",resource_path[]="/tmp/opence-resource-XXXXXX",data[32];int fd=mkstemp(path),resource_fd;unsigned i;size_t fixture_size;unsigned char fixture[0x3000];
    struct mac_cache_files scope={0},empty={0},bad={0};const char *error;boolean done;
    CHECK(fd>=0);if(fd<0)return 1;CHECK(make_cache(fixture,&fixture_size)!=NULL);CHECK(write(fd,fixture,fixture_size)==(ssize_t)fixture_size);close(fd);
    resource_fd=mkstemp(resource_path);CHECK(resource_fd>=0);if(resource_fd<0){unlink(path);return 1;}CHECK(write(resource_fd,"0123456789abcdef",16)==16);close(resource_fd);
    CHECK(!mac_cache_files_open(path,"/no-such-opence-file",NULL,&bad,&error));CHECK(!bad.map.data&&!bad.bitmaps.data);
    CHECK(mac_cache_files_open(path,resource_path,resource_path,&scope,&error));CHECK(mac_retail_open(scope.map.data,scope.map.size,&scope.retail,&error));CHECK(mac_retail_resources_check(&scope.retail,(uint32_t)scope.bitmaps.size,(uint32_t)scope.sounds.size,&scope.resources,&error));CHECK(scope.retail.tag_count==2);CHECK(mac_cache_files_bind(&scope));CHECK(!strcmp(cache_files_map_directory(),"/tmp/"));CHECK(!mac_cache_files_bind(&scope));
    CHECK(!mac_cache_files_unbind(&empty));
    memset(data,'x',sizeof(data));done=FALSE;
    CHECK(cache_file_read(NONE,1,3,data+2,&done,FALSE)==0&&done==TRUE);
    CHECK(!memcmp(data+2,fixture+1,3)&&data[1]=='x'&&data[5]=='x');
    done=FALSE;CHECK(cache_file_read(NONE,MAC_CACHE_BITMAP_BASE+2,3,data,&done,TRUE)==0&&done==TRUE);CHECK(!memcmp(data,"234",3));
    done=FALSE;CHECK(cache_file_read(NONE,MAC_CACHE_SOUND_BASE+3,3,data,&done,FALSE)==0&&done==TRUE);CHECK(!memcmp(data,"345",3));
    cache_file_promote_read(0);cache_file_block_until_not_busy();CHECK(scope.completed_reads==3);
    memset(data,'q',sizeof(data));
    CHECK(!mac_cache_files_read(&scope,0x2fff,2,data,sizeof(data),&error));CHECK(data[0]=='q');
    CHECK(!mac_cache_files_read(&scope,0,4,data,3,&error));CHECK(data[0]=='q');
    CHECK(!mac_cache_files_read(&scope,UINT32_MAX,4,data,sizeof(data),&error));
    CHECK(!mac_cache_files_read(&scope,MAC_CACHE_DECODED_BASE,4,data,sizeof(data),&error));
    CHECK(mac_cache_files_read(&scope,16,0,data,sizeof(data),&error));
    for(i=0;i<5;++i)expect_abort((int)i);
    CHECK(mac_cache_files_unbind(&scope));CHECK(!cache_files_map_directory()[0]);CHECK(!mac_cache_files_unbind(&scope));expect_abort(5);
    mac_cache_files_close(&scope);CHECK(!scope.map.data&&!scope.bitmaps.data&&!scope.sounds.data);
    CHECK(mac_cache_files_open(path,NULL,NULL,&scope,&error));
    CHECK(!mac_cache_files_read(&scope,MAC_CACHE_SOUND_BASE,1,data,sizeof(data),&error));
    CHECK(mac_cache_files_bind(&scope));mac_cache_files_close(&scope);
    CHECK(mac_cache_files_open(path,NULL,NULL,&empty,&error));CHECK(mac_cache_files_bind(&empty));mac_cache_files_close(&empty);unlink(path);
    if(argc==3){
        unsigned char expected[7];const void *source;
        CHECK(mac_cache_files_open(argv[1],argv[2],NULL,&scope,&error));CHECK(mac_cache_files_bind(&scope));
        source=mac_mapped_file_span(&scope.bitmaps,16,7);CHECK(source!=NULL);memcpy(expected,source,7);
        done=FALSE;CHECK(cache_file_read(NONE,MAC_CACHE_BITMAP_BASE+16,7,data,&done,FALSE)==0&&done==TRUE);
        CHECK(!memcmp(data,expected,7));mac_cache_files_close(&scope);
    }
    unlink(resource_path);
    printf("native cache files: %u checks %u failures; validated retail v7 map, exact synchronous bytes, failures abort before publication\n",checks,failures);
    return failures?1:0;
}
