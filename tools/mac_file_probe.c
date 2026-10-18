#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* Host headers above use Darwin's fd_set macro; game headers use Winsock. */
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define fast_ftol mac_file_probe_unused_fast_ftol
#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
#undef BUILDING_CSERIES
#undef fast_ftol
#include "engine_files.h"
#include "engine_handles.h"
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

static unsigned checks,failures;
#define CHECK(x) do { ++checks;if(!(x)){if(failures<16)fprintf(stderr,"FAIL line %d: %s (error=%lu)\n",__LINE__,#x,(unsigned long)GetLastError());++failures;} } while(0)
int main(void)
{
    char xbox_path[128],host_path[PATH_MAX],lower_path[128],directory_path[128],child_path[160];
    const char source[]="owned native file handle";
    char target[sizeof(source)];
    WIN32_FIND_DATAA find_data;
    HANDLE file,existing,event,shared;
    DWORD count=0,size_high=0;OVERLAPPED overlapped;
    int n;
    n=snprintf(xbox_path,sizeof(xbox_path),"t:\\MacFileProbe_%ld.bin",(long)getpid());
    CHECK(n>0&&(size_t)n<sizeof(xbox_path));
    CHECK(mac_engine_files_translate_path(xbox_path,host_path,sizeof(host_path))==0);
    CHECK(strstr(host_path,"/t/MacFileProbe_")!=NULL);
    file=CreateFileA(xbox_path,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    CHECK(file!=INVALID_HANDLE_VALUE);
    if(file==INVALID_HANDLE_VALUE)return 1;
    CHECK(WriteFile(file,source,sizeof(source),&count,NULL));CHECK(count==sizeof(source));
    CHECK(GetFileSize(file,&size_high)==sizeof(source)&&size_high==0);
    CHECK(SetFilePointer(file,5,NULL,FILE_BEGIN)==5);
    memset(target,0,sizeof(target));CHECK(ReadFile(file,target,4,&count,NULL));
    CHECK(count==4&&!memcmp(target,source+5,4));
    memset(&overlapped,0,sizeof(overlapped));overlapped.Offset=0;
    event=CreateEventA(NULL,1,0,NULL);CHECK(event!=NULL);overlapped.hEvent=event;
    CHECK(WriteFile(file,"MAC",3,NULL,&overlapped));
    CHECK(overlapped.Internal==ERROR_SUCCESS&&overlapped.InternalHigh==3);
    CHECK(WaitForSingleObject(event,0)==WAIT_OBJECT_0);
    CHECK(CloseHandle(event));
    CHECK(SetFilePointer(file,0,NULL,FILE_BEGIN)==0);
    memset(target,0,sizeof(target));CHECK(ReadFile(file,target,sizeof(target),&count,NULL));
    CHECK(count==sizeof(source)&&!memcmp(target,"MAC",3)&&!memcmp(target+3,source+3,sizeof(source)-3));
    CHECK(CloseHandle(file));
    CHECK(!ReadFile(file,target,1,&count,NULL)&&GetLastError()==ERROR_INVALID_HANDLE);

    file=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    CHECK(file!=INVALID_HANDLE_VALUE);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==32UL);
    if(file!=INVALID_HANDLE_VALUE)CHECK(CloseHandle(file));
    file=CreateFileA(xbox_path,GENERIC_READ,1,NULL,OPEN_EXISTING,0,NULL);
    CHECK(file!=INVALID_HANDLE_VALUE);
    shared=CreateFileA(xbox_path,GENERIC_READ,1,NULL,OPEN_EXISTING,0,NULL);
    CHECK(shared!=INVALID_HANDLE_VALUE);
    existing=CreateFileA(xbox_path,GENERIC_WRITE,1,NULL,OPEN_EXISTING,0,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==32UL);
    if(shared!=INVALID_HANDLE_VALUE)CHECK(CloseHandle(shared));
    if(file!=INVALID_HANDLE_VALUE)CHECK(CloseHandle(file));

    strcpy(lower_path,xbox_path);
    {
        char *name=strstr(lower_path,"MacFileProbe");
        if(name)name[0]=(char)tolower((unsigned char)name[0]);
    }
    existing=CreateFileA(lower_path,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE);
    if(existing!=INVALID_HANDLE_VALUE) {
        CHECK(!WriteFile(existing,source,1,&count,NULL)&&GetLastError()==ERROR_ACCESS_DENIED);
        CHECK(CloseHandle(existing));
    }
    existing=CreateFileA(xbox_path,0,0,NULL,OPEN_EXISTING,0,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE);
    if(existing!=INVALID_HANDLE_VALUE) {
        CHECK(!ReadFile(existing,target,1,&count,NULL)&&GetLastError()==ERROR_ACCESS_DENIED);
        CHECK(CloseHandle(existing));
    }
    existing=CreateFileA(xbox_path,GENERIC_READ,4,NULL,OPEN_EXISTING,0,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)1,OPEN_EXISTING,0,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,0,(HANDLE)1);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER);
    existing=CreateFileA(xbox_path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE);
    if(existing!=INVALID_HANDLE_VALUE) {
        memset(&overlapped,0,sizeof(overlapped));overlapped.OffsetHigh=0x80000000UL;
        CHECK(!ReadFile(existing,target,1,&count,&overlapped)&&GetLastError()==ERROR_INVALID_PARAMETER);
        CHECK(GetFileSize(existing,&size_high)==sizeof(source)&&size_high==0);
        CHECK(CloseHandle(existing));
    }
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,CREATE_ALWAYS,0,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ACCESS_DENIED);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,TRUNCATE_EXISTING,0,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ACCESS_DENIED);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE);
    if(existing!=INVALID_HANDLE_VALUE) {
        CHECK(GetFileSize(existing,&size_high)==sizeof(source)&&size_high==0);
        CHECK(CloseHandle(existing));
    }
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,0x00000040UL,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_INVALID_PARAMETER);
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_NO_BUFFERING,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE);
    if(existing!=INVALID_HANDLE_VALUE)CHECK(CloseHandle(existing));
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ALREADY_EXISTS);
    if(existing!=INVALID_HANDLE_VALUE)CHECK(CloseHandle(existing));
    existing=CreateFileA(xbox_path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    CHECK(existing==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ALREADY_EXISTS);
    CHECK(GetFileAttributesA(xbox_path)==FILE_ATTRIBUTE_NORMAL);
    {
        WIN32_FILE_ATTRIBUTE_DATA attributes;
        CHECK(GetFileAttributesExA(xbox_path,GetFileExInfoStandard,&attributes));
        CHECK(attributes.dwFileAttributes==FILE_ATTRIBUTE_NORMAL&&attributes.nFileSizeLow==sizeof(source)&&attributes.nFileSizeHigh==0);
        CHECK(attributes.ftLastWriteTime.dwLowDateTime!=0||attributes.ftLastWriteTime.dwHighDateTime!=0);
        CHECK(!GetFileAttributesExA(xbox_path,(GET_FILEEX_INFO_LEVELS)1,&attributes)&&GetLastError()==ERROR_INVALID_PARAMETER);
    }
    existing=CreateFileA(xbox_path,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE);
    CHECK(!DeleteFileA(xbox_path)&&GetLastError()==32UL);
    if(existing!=INVALID_HANDLE_VALUE)CHECK(CloseHandle(existing));
    n=snprintf(directory_path,sizeof(directory_path),"t:\\MacFileDir_%ld",(long)getpid());
    CHECK(n>0&&(size_t)n<sizeof(directory_path));
    CHECK(CreateDirectoryA(directory_path,NULL));
    CHECK(GetFileAttributesA(directory_path)==FILE_ATTRIBUTE_DIRECTORY);
    CHECK(!CreateDirectoryA(directory_path,(LPSECURITY_ATTRIBUTES)1)&&GetLastError()==ERROR_INVALID_PARAMETER);
    n=snprintf(child_path,sizeof(child_path),"%s\\entry.bin",directory_path);
    CHECK(n>0&&(size_t)n<sizeof(child_path));
    existing=CreateFileA(child_path,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    CHECK(existing!=INVALID_HANDLE_VALUE);
    if(existing!=INVALID_HANDLE_VALUE) {
        CHECK(!FindNextFileA(existing,&find_data)&&GetLastError()==ERROR_INVALID_HANDLE);
        CHECK(!XFindClose(existing));
        CHECK(CloseHandle(existing));
    }
    {
        char other_path[160],search_path[180];HANDLE find;
        n=snprintf(other_path,sizeof(other_path),"%s\\other.dat",directory_path);
        CHECK(n>0&&(size_t)n<sizeof(other_path));
        existing=CreateFileA(other_path,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
        CHECK(existing!=INVALID_HANDLE_VALUE);
        if(existing!=INVALID_HANDLE_VALUE)CHECK(CloseHandle(existing));
        n=snprintf(search_path,sizeof(search_path),"%s\\*.bin",directory_path);
        CHECK(n>0&&(size_t)n<sizeof(search_path));
        find=FindFirstFileA(search_path,&find_data);
        CHECK(find!=INVALID_HANDLE_VALUE&&strcmp(find_data.cFileName,"entry.bin")==0);
        if(find!=INVALID_HANDLE_VALUE) {
            CHECK(!FindNextFileA(find,&find_data)&&GetLastError()==ERROR_NO_MORE_FILES);
            CHECK(!CloseHandle(find));
            CHECK(XFindClose(find));
        }
        n=snprintf(search_path,sizeof(search_path),"%s\\/*.bin",directory_path);
        CHECK(n>0&&(size_t)n<sizeof(search_path));
        find=FindFirstFileA(search_path,&find_data);
        CHECK(find!=INVALID_HANDLE_VALUE&&strcmp(find_data.cFileName,"entry.bin")==0);
        if(find!=INVALID_HANDLE_VALUE)CHECK(XFindClose(find));
        n=snprintf(search_path,sizeof(search_path),"%s\\*.missing",directory_path);
        CHECK(n>0&&(size_t)n<sizeof(search_path));
        find=FindFirstFileA(search_path,&find_data);
        CHECK(find==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_FILE_NOT_FOUND);
        CHECK(DeleteFileA(other_path));
    }
    CHECK(!RemoveDirectoryA(directory_path)&&GetLastError()==ERROR_DIR_NOT_EMPTY);
    CHECK(DeleteFileA(child_path));
    CHECK(RemoveDirectoryA(directory_path));
    CHECK(GetFileAttributesA(directory_path)==(DWORD)-1);
    {
        char copy_path[160];ULARGE_INTEGER available,total,free_bytes;
        CHECK(GetDiskFreeSpaceExA("t:",&available,&total,&free_bytes));
        CHECK(total.QuadPart>0 && available.QuadPart<=free_bytes.QuadPart && free_bytes.QuadPart<=total.QuadPart);
        CHECK(!GetDiskFreeSpaceExA("t:\\nonexistent-volume-directory",&available,NULL,NULL));
        snprintf(copy_path,sizeof(copy_path),"%s.copy",xbox_path);
        CHECK(CopyFileA(xbox_path,copy_path,TRUE));
        CHECK(!CopyFileA(xbox_path,copy_path,TRUE));
        CHECK(!CopyFileA(xbox_path,xbox_path,FALSE));
        existing=CreateFileA(copy_path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
        CHECK(existing!=INVALID_HANDLE_VALUE);
        if(existing!=INVALID_HANDLE_VALUE) {
            CHECK(ReadFile(existing,target,sizeof(target),&count,NULL));
            CHECK(count==sizeof(source) && !memcmp(target,"MAC",3) && !memcmp(source+3,target+3,sizeof(source)-3));
            CHECK(SetFilePointer(existing,3,NULL,FILE_BEGIN)==3 && SetEndOfFile(existing));
            CHECK(GetFileSize(existing,NULL)==3);
            CHECK(SetFilePointer(existing,64,NULL,FILE_BEGIN)==64 && SetEndOfFile(existing));
            CHECK(GetFileSize(existing,NULL)==64);
            CHECK(SetFilePointer(existing,3,NULL,FILE_BEGIN)==3);
            memset(target,1,sizeof(target));CHECK(ReadFile(existing,target,sizeof(target),&count,NULL));
            CHECK(count==sizeof(target) && target[0]==0 && target[sizeof(target)-1]==0);
            CHECK(CloseHandle(existing));
        }
        CHECK(SetFileAttributesA(copy_path,FILE_ATTRIBUTE_READONLY));
        CHECK(GetFileAttributesA(copy_path)==FILE_ATTRIBUTE_READONLY);
        existing=CreateFileA(copy_path,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
        CHECK(existing!=INVALID_HANDLE_VALUE);
        if(existing!=INVALID_HANDLE_VALUE) {CHECK(!SetEndOfFile(existing));CHECK(CloseHandle(existing));}
        CHECK(!SetFileAttributesA(copy_path,2UL));
        CHECK(SetFileAttributesA(copy_path,FILE_ATTRIBUTE_NORMAL));
        CHECK(CopyFileA(xbox_path,copy_path,FALSE));
        CHECK(DeleteFileA(copy_path));
        CHECK(!SetEndOfFile(INVALID_HANDLE_VALUE));
    }
    CHECK(DeleteFileA(xbox_path));
    CHECK(mac_engine_files_translate_path("t:\\..\\..\\escape",lower_path,sizeof(lower_path))==0);
    CHECK(strstr(lower_path,"/t/escape")!=NULL&&strstr(lower_path,"/../")==NULL);
    printf("native file service: %u checks, %u failures; leased files, sharing, path operations, sequential/overlapped I/O, seek/size\n",checks,failures);
    return failures?1:0;
}
