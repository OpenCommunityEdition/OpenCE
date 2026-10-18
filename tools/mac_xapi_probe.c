#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define fast_ftol mac_xapi_probe_unused_fast_ftol
#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
#undef BUILDING_CSERIES
#undef fast_ftol
#include "engine_handles.h"
#include "engine_files.h"

static unsigned checks,failures;
#define CHECK(expression,label) do { ++checks;if(!(expression)){++failures;if(failures<12)fprintf(stderr,"FAIL %s (error=%lu)\n",label,(unsigned long)GetLastError());} } while(0)

static int same_signature(const XCALCSIG_SIGNATURE *value)
{
    static const unsigned char expected[20]={
        0xd1,0x1f,0x1c,0xa1,0x99,0x05,0xbc,0xdb,0x90,0x63,
        0x87,0xdb,0xfb,0xd4,0x86,0x88,0x21,0xe4,0xa4,0xd3};
    return memcmp(value->Signature,expected,sizeof(expected))==0;
}

int main(void)
{
    static const WCHAR name[]={ 'N','a','t','i','v','e',' ','S','a','v','e',0 };
    static const WCHAR nickname[]={ 'T','e','s','t',' ','P','l','a','y','e','r',0 };
    static const unsigned char payload[]="OpenCE native save signature";
    static const unsigned char expected_metadata[]={
        'N',0,'a',0,'t',0,'i',0,'v',0,'e',0,' ',0,'S',0,'a',0,'v',0,'e',0};
    static const char root[]="u:\\";
    char path[MAX_PATH];
    char host_save[1024],metadata[1200],sentinel[1200];
    char *save_root=getenv("HALO_MAC_SAVE_ROOT");
    unsigned char metadata_bytes[24];FILE *metadata_file;
    XGAME_FIND_DATA data;
    XCALCSIG_SIGNATURE signature;
    HANDLE find,calculation,nickname_find;
    WCHAR found_nickname[MAX_NICKNAME];
    DWORD result;

    result=XCreateSaveGame(root,name,CREATE_NEW,0,path,sizeof(path));
    CHECK(result==ERROR_SUCCESS,"create save container");
    CHECK(strstr(path,"UDATA\\")!=NULL,"save path names UDATA");
    CHECK(!mac_engine_files_translate_path(path,host_save,sizeof(host_save)),"translate created save path");
    snprintf(metadata,sizeof(metadata),"%s/SaveMeta.xbx",host_save);
    metadata_file=fopen(metadata,"rb");
    CHECK(metadata_file!=NULL,"metadata file created at native child path");
    if(metadata_file) {
        size_t read_count=fread(metadata_bytes,1,sizeof(metadata_bytes),metadata_file);
        fclose(metadata_file);
        CHECK(read_count==22,"metadata stores exact UTF-16 payload length");
        CHECK(!memcmp(metadata_bytes,expected_metadata,sizeof(expected_metadata)),
            "metadata bytes match canonical little-endian UTF-16");
    }
    CHECK(XCreateSaveGame("d:\\",name,CREATE_NEW,0,NULL,0)==ERROR_INVALID_PARAMETER,
        "save API rejects non-user drive roots");
    CHECK(XCreateSaveGame(root,name,77,0,NULL,0)==ERROR_INVALID_PARAMETER,
        "save API rejects unknown creation disposition");
    CHECK(XCreateSaveGame(root,name,OPEN_EXISTING,1,NULL,0)==ERROR_INVALID_PARAMETER,
        "save API rejects unknown create flags");
    CHECK(XFindFirstSaveGame("d:\\",&data)==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER,
        "save enumeration rejects non-user drive roots");
    CHECK(XDeleteSaveGame("d:\\",name)==ERROR_INVALID_PARAMETER,
        "save deletion rejects non-user drive roots");
    CHECK(XCalculateSignatureBegin(1)==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER,
        "signature API rejects unknown flags");
    { char short_path[1];CHECK(XCreateSaveGame(root,name,OPEN_EXISTING,0,short_path,sizeof(short_path))==ERROR_INSUFFICIENT_BUFFER,
        "save API reports insufficient output buffer"); }
    CHECK(XCreateSaveGame(root,name,CREATE_NEW,0,NULL,0)==ERROR_ALREADY_EXISTS,
        "CREATE_NEW rejects existing save");
    CHECK(XCreateSaveGame(root,name,OPEN_EXISTING,0,NULL,0)==ERROR_SUCCESS,
        "OPEN_EXISTING finds existing save");

    find=XFindFirstSaveGame(root,&data);
    CHECK(find!=INVALID_HANDLE_VALUE,"enumerate saved game");
    calculation=XCalculateSignatureBegin(0);
    CHECK(calculation!=INVALID_HANDLE_VALUE,"begin handle-type check signature");
    if(calculation!=INVALID_HANDLE_VALUE) {
        CHECK(XCalculateSignatureUpdate(find,payload,1)==ERROR_INVALID_HANDLE,
            "save-find handle cannot be used as signature");
        CHECK(!XFindClose(calculation),"signature handle cannot be closed as a find handle");
        CHECK(XCalculateSignatureEnd(calculation,&signature)==ERROR_SUCCESS,
            "wrong-type close leaves signature usable");
    }
    if(find!=INVALID_HANDLE_VALUE) {
        CHECK(data.wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY,"save result is directory");
        CHECK(!memcmp(data.szSaveGameName,name,sizeof(name)),"save metadata round trip");
        CHECK(!XFindNextSaveGame(find,&data)&&GetLastError()==ERROR_NO_MORE_FILES,
            "save enumeration reaches end");
        CHECK(XFindClose(find),"close save enumeration");
    }

    calculation=XCalculateSignatureBegin(0);
    CHECK(calculation!=INVALID_HANDLE_VALUE,"begin content signature");
    if(calculation!=INVALID_HANDLE_VALUE) {
        CHECK(XCalculateSignatureUpdate(calculation,payload,8)==ERROR_SUCCESS,"first signature update");
        CHECK(XCalculateSignatureUpdate(calculation,payload+8,sizeof(payload)-1-8)==ERROR_SUCCESS,
            "second signature update");
        CHECK(XCalculateSignatureEnd(calculation,&signature)==ERROR_SUCCESS,"finish content signature");
        CHECK(same_signature(&signature),"content signature matches Linux save contract");
    }

    CHECK(XSetNicknameW(nickname,FALSE),"set nickname");
    nickname_find=XFindFirstNicknameW(TRUE,found_nickname,MAX_NICKNAME);
    CHECK(nickname_find!=INVALID_HANDLE_VALUE,"enumerate nickname");
    if(nickname_find!=INVALID_HANDLE_VALUE) {
        CHECK(found_nickname[0]=='T'&&found_nickname[4]==' ',"nickname round trip");
        CHECK(XFindClose(nickname_find),"close nickname enumeration");
    }

    CHECK(XDeleteSaveGame(root,name)==ERROR_SUCCESS,"delete only created save");
    if(save_root&&*save_root&&host_save[0]) {
        char sentinel_dir[1200],sentinel_file[1200];FILE *sentinel_stream;
        snprintf(sentinel_dir,sizeof(sentinel_dir),"%s/symlink-target",save_root);
        snprintf(sentinel_file,sizeof(sentinel_file),"%s/keep.txt",sentinel_dir);
        CHECK(mkdir(sentinel_dir,0700)==0,"create symlink deletion sentinel");
        sentinel_stream=fopen(sentinel_file,"wb");
        CHECK(sentinel_stream!=NULL,"write symlink deletion sentinel");
        if(sentinel_stream){fputs("preserve",sentinel_stream);fclose(sentinel_stream);}
        CHECK(symlink(sentinel_dir,host_save)==0,"install save-directory symlink");
        CHECK(XDeleteSaveGame(root,name)!=ERROR_SUCCESS,"delete rejects symlink save directory");
        CHECK(access(sentinel_file,F_OK)==0,"delete does not follow symlink outside save directory");
        unlink(host_save);
    }
    find=XFindFirstSaveGame(root,&data);
    CHECK(find==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_NO_MORE_FILES,
        "deleted save no longer enumerates");
    printf("native XAPI save/signature checks: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
