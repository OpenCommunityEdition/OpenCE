/* Native save-game and content-signature services used by the unchanged game. */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define fast_ftol mac_engine_xapi_unused_fast_ftol
#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
#undef BUILDING_CSERIES
#undef fast_ftol
#include "engine_files.h"
#include "engine_handles.h"
#include "mbedtls/sha1.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

struct mac_save_find {
    DIR *directory;
    pthread_mutex_t lock;
    char xbox_directory[MAX_PATH];
    char host_directory[1024];
};

struct mac_signature {
    mbedtls_sha1_context sha1;
    pthread_mutex_t lock;
    int consumed;
};

struct mac_nickname_find { unsigned long marker; };
static WCHAR mac_last_nickname[MAX_NICKNAME];
static pthread_mutex_t mac_nickname_lock = PTHREAD_MUTEX_INITIALIZER;
static int remove_tree(const char *path,unsigned depth);

static void set_errno_error(int error)
{
    switch(error) {
    case ENOENT: SetLastError(ERROR_FILE_NOT_FOUND); break;
    case ENOTDIR: SetLastError(ERROR_PATH_NOT_FOUND); break;
    case EACCES: case EPERM: SetLastError(ERROR_ACCESS_DENIED); break;
    case ENOMEM: SetLastError(ERROR_NOT_ENOUGH_MEMORY); break;
    case EEXIST: SetLastError(ERROR_ALREADY_EXISTS); break;
    case ENOSPC: SetLastError(ERROR_DISK_FULL); break;
    default: SetLastError(ERROR_GEN_FAILURE); break;
    }
}

static int path_join(char *out, size_t size, const char *left, const char *right)
{
    int n = snprintf(out, size, "%s%s%s", left,
        *left && left[strlen(left)-1] != '\\' && left[strlen(left)-1] != '/' ? "\\" : "", right);
    return n >= 0 && (size_t)n < size;
}

/* Host paths belong to Darwin's filesystem and always use '/'.  Xbox paths
   above use '\\' so they retain the game-visible spelling. */
static int host_path_join(char *out, size_t size, const char *left, const char *right)
{
    int n = snprintf(out, size, "%s%s%s", left,
        *left && left[strlen(left)-1] != '/' ? "/" : "", right);
    return n >= 0 && (size_t)n < size;
}

static int valid_save_root(LPCSTR root)
{
    if(!root || (root[0]!='u' && root[0]!='U') || root[1]!=':')return 0;
    return root[2]==0 || (root[2]=='\\' && root[3]==0) ||
        (root[2]=='/' && root[3]==0);
}

static int name_length(LPCWSTR name, size_t *length)
{
    size_t n;
    if(!name) return 0;
    for(n=0;n<MAX_GAMENAME;n++) if(name[n]==0) { *length=n;return 1; }
    return 0;
}

static uint64_t save_name_hash(LPCWSTR name, size_t length)
{
    uint64_t hash=UINT64_C(1469598103934665603);size_t i;
    for(i=0;i<length;i++) { hash^=(uint16_t)name[i];hash*=UINT64_C(1099511628211); }
    return hash&UINT64_C(0xffffffffffff);
}

static int save_paths(LPCSTR root, LPCWSTR name, size_t name_chars,
    char *xbox_directory, size_t xbox_size, char *host_directory, size_t host_size)
{
    char suffix[64];int n;
    if(!valid_save_root(root))return 0;
    n=snprintf(suffix,sizeof(suffix),"UDATA\\%012llX\\",
        (unsigned long long)save_name_hash(name,name_chars));
    return n>=0&&(size_t)n<sizeof(suffix)&&path_join(xbox_directory,xbox_size,root,suffix)&&
        !mac_engine_files_translate_path(xbox_directory,host_directory,host_size);
}

static int ensure_directory(const char *path)
{
    struct stat st;
    if(mkdir(path,0755)==0)return 1;
    return errno==EEXIST&&stat(path,&st)==0&&S_ISDIR(st.st_mode);
}

static int save_metadata_path(const char *directory, char *path, size_t size)
{ return host_path_join(path,size,directory,"SaveMeta.xbx"); }

static int write_save_name(const char *directory,LPCWSTR name,size_t length)
{
    char path[1024],temporary[1060];int fd;size_t i,bytes=length*sizeof(WCHAR),done=0;
    unsigned char encoded[MAX_GAMENAME*sizeof(WCHAR)];
    for(i=0;i<length;i++) { encoded[i*2]=(unsigned char)name[i];encoded[i*2+1]=(unsigned char)((uint16_t)name[i]>>8); }
    if(!save_metadata_path(directory,path,sizeof(path)))return 0;
    if(snprintf(temporary,sizeof(temporary),"%s.tmp.XXXXXX",path)<0||strlen(path)+11>=sizeof(temporary))return 0;
    fd=mkstemp(temporary);if(fd<0)return 0;
    while(done<bytes) {
        ssize_t n=write(fd,(const char *)encoded+done,bytes-done);
        if(n<0&&errno==EINTR)continue;
        if(n<=0) { int saved=errno;close(fd);unlink(temporary);errno=saved;return 0; }
        done+=(size_t)n;
    }
    if(fsync(fd)!=0) { int saved=errno;close(fd);unlink(temporary);errno=saved;return 0; }
    if(close(fd)!=0) { int saved=errno;unlink(temporary);errno=saved;return 0; }
    if(rename(temporary,path)!=0) { int saved=errno;unlink(temporary);errno=saved;return 0; }
    return 1;
}

static int read_save_name(const char *directory,WCHAR *name,size_t capacity)
{
    char path[1024];int fd;size_t bytes=0,limit=(capacity-1)*sizeof(WCHAR);unsigned char encoded[MAX_GAMENAME*sizeof(WCHAR)];struct stat st;
    if(!capacity||capacity>MAX_GAMENAME||!save_metadata_path(directory,path,sizeof(path)))return 0;
    if(lstat(path,&st)!=0||!S_ISREG(st.st_mode)||S_ISLNK(st.st_mode))return 0;
    fd=open(path,O_RDONLY);if(fd<0)return 0;
    memset(name,0,capacity*sizeof(WCHAR));
    while(bytes<limit) {
        ssize_t n=read(fd,(char *)encoded+bytes,limit-bytes);
        if(n<0&&errno==EINTR)continue;
        if(n<0) { close(fd);return 0; }
        if(n==0)break;
        bytes+=(size_t)n;
    }
    close(fd);
    if(bytes&1)return 0;
    { size_t i;for(i=0;i<bytes/2;i++)name[i]=(WCHAR)((uint16_t)encoded[i*2]|((uint16_t)encoded[i*2+1]<<8)); }
    return name[0]!=0;
}

DWORD WINAPI XGetLanguage(void)
{ return XC_LANGUAGE_ENGLISH; }

DWORD WINAPI XCreateSaveGame(LPCSTR root,LPCWSTR name,DWORD disposition,DWORD flags,
    LPSTR path_buffer,UINT buffer_size)
{
    char xbox_directory[MAX_PATH],host_directory[1024],xbox_data[MAX_PATH],host_data[1024];
    struct stat st;size_t chars;int exists;
    if(!root||!valid_save_root(root)||!name||!name_length(name,&chars)||chars==0||
        (disposition!=CREATE_NEW&&disposition!=CREATE_ALWAYS&&disposition!=OPEN_EXISTING&&
         disposition!=OPEN_ALWAYS&&disposition!=TRUNCATE_EXISTING)||flags!=0||
        !save_paths(root,name,chars,xbox_directory,sizeof(xbox_directory),host_directory,sizeof(host_directory))||
        !path_join(xbox_data,sizeof(xbox_data),root,"UDATA\\")||
        mac_engine_files_translate_path(xbox_data,host_data,sizeof(host_data))) {
        SetLastError(ERROR_INVALID_PARAMETER);return ERROR_INVALID_PARAMETER;
    }
    if(lstat(host_directory,&st)==0) {
        if(S_ISLNK(st.st_mode)){SetLastError(ERROR_ACCESS_DENIED);return ERROR_ACCESS_DENIED;}
        if(!S_ISDIR(st.st_mode)){SetLastError(ERROR_PATH_NOT_FOUND);return ERROR_PATH_NOT_FOUND;}
        exists=1;
    }
    else if(errno==ENOENT)exists=0;
    else { set_errno_error(errno);return GetLastError(); }
    if(exists&&disposition==CREATE_NEW) { SetLastError(ERROR_ALREADY_EXISTS);return ERROR_ALREADY_EXISTS; }
    if(!exists&&disposition==OPEN_EXISTING) { SetLastError(ERROR_PATH_NOT_FOUND);return ERROR_PATH_NOT_FOUND; }
    if(!exists&&disposition==TRUNCATE_EXISTING) { SetLastError(ERROR_PATH_NOT_FOUND);return ERROR_PATH_NOT_FOUND; }
    if(!exists) {
        char translated_root[1024];
        if(mac_engine_files_translate_path(root,translated_root,sizeof(translated_root))||
            !ensure_directory(translated_root)||!ensure_directory(host_data)||!ensure_directory(host_directory)) {
            DWORD error=GetLastError();if(error==ERROR_SUCCESS) { set_errno_error(errno);error=GetLastError(); }
            return error;
        }
        if(!write_save_name(host_directory,name,chars)) { set_errno_error(errno);return GetLastError(); }
    }
    else if(disposition==CREATE_ALWAYS||disposition==TRUNCATE_EXISTING) {
        if(!write_save_name(host_directory,name,chars)) { set_errno_error(errno);return GetLastError(); }
    }
    if(path_buffer&&buffer_size) {
        size_t length=strlen(xbox_directory);
        if(length+1>buffer_size) { SetLastError(ERROR_INSUFFICIENT_BUFFER);return ERROR_INSUFFICIENT_BUFFER; }
        memcpy(path_buffer,xbox_directory,length+1);
    }
    SetLastError(ERROR_SUCCESS);return ERROR_SUCCESS;
}

static int remove_tree(const char *path,unsigned depth)
{
    DIR *directory;struct dirent *entry;int ok=1,saved_errno=0;struct stat root_stat;
    if(depth>16)return 0;
    if(lstat(path,&root_stat)!=0)return errno==ENOENT;
    if(!S_ISDIR(root_stat.st_mode)||S_ISLNK(root_stat.st_mode)){errno=ELOOP;return 0;}
    directory=opendir(path);
    if(!directory)return errno==ENOENT;
    for(;;) {
        char child[1024];struct stat st;int n;
        errno=0;entry=readdir(directory);
        if(!entry){if(errno!=0){ok=0;saved_errno=errno;}break;}
        if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,".."))continue;
        n=snprintf(child,sizeof(child),"%s/%s",path,entry->d_name);
        if(n<0||(size_t)n>=sizeof(child)||lstat(child,&st)!=0) { ok=0;continue; }
        if(S_ISDIR(st.st_mode)&&!S_ISLNK(st.st_mode)) { if(!remove_tree(child,depth+1))ok=0; }
        else if(unlink(child)!=0)ok=0;
    }
    if(closedir(directory)!=0)ok=0;
    if(!ok){if(saved_errno)errno=saved_errno;return 0;}
    return rmdir(path)==0;
}

DWORD WINAPI XDeleteSaveGame(LPCSTR root,LPCWSTR name)
{
    char xbox_directory[MAX_PATH],host_directory[1024];struct stat st;size_t chars;
    if(!root||!valid_save_root(root)||!name||!name_length(name,&chars)||
        !save_paths(root,name,chars,xbox_directory,sizeof(xbox_directory),host_directory,sizeof(host_directory))) {
        SetLastError(ERROR_INVALID_PARAMETER);return ERROR_INVALID_PARAMETER;
    }
    if(lstat(host_directory,&st)!=0||!S_ISDIR(st.st_mode)||S_ISLNK(st.st_mode)) { SetLastError(ERROR_PATH_NOT_FOUND);return ERROR_PATH_NOT_FOUND; }
    if(!remove_tree(host_directory,0)) { set_errno_error(errno);return GetLastError(); }
    SetLastError(ERROR_SUCCESS);return ERROR_SUCCESS;
}

static void save_find_destroy(void *payload)
{
    struct mac_save_find *find=(struct mac_save_find *)payload;
    if(find->directory)closedir(find->directory);
    pthread_mutex_destroy(&find->lock);free(find);
}

static BOOL save_find_next(struct mac_save_find *find,PXGAME_FIND_DATA data)
{
    struct dirent *entry;
    while((entry=readdir(find->directory))!=NULL) {
        char child[1024];struct stat st;int n;
        if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,".."))continue;
        n=snprintf(child,sizeof(child),"%s/%s",find->host_directory,entry->d_name);
        if(n<0||(size_t)n>=sizeof(child)||stat(child,&st)!=0||!S_ISDIR(st.st_mode))continue;
        memset(data,0,sizeof(*data));
        data->wfd.dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY;
        if(strlen(entry->d_name)>=sizeof(data->wfd.cFileName))continue;
        strcpy(data->wfd.cFileName,entry->d_name);
        if(!path_join(data->szSaveGameDirectory,sizeof(data->szSaveGameDirectory),
            find->xbox_directory,entry->d_name))continue;
        {
            size_t length=strlen(data->szSaveGameDirectory);
            if(length+1>=sizeof(data->szSaveGameDirectory))continue;
            data->szSaveGameDirectory[length]='\\';data->szSaveGameDirectory[length+1]=0;
        }
        if(read_save_name(child,data->szSaveGameName,MAX_GAMENAME))return TRUE;
    }
    SetLastError(ERROR_NO_MORE_FILES);return FALSE;
}

HANDLE WINAPI XFindFirstSaveGame(LPCSTR root,PXGAME_FIND_DATA data)
{
    struct mac_save_find *find;HANDLE handle;char xbox_data[MAX_PATH];
    if(!root||!valid_save_root(root)||!data||!path_join(xbox_data,sizeof(xbox_data),root,"UDATA\\")) {
        SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;
    }
    find=(struct mac_save_find *)calloc(1,sizeof(*find));
    if(!find) { SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    if(strlen(xbox_data)>=sizeof(find->xbox_directory)||
        mac_engine_files_translate_path(xbox_data,find->host_directory,sizeof(find->host_directory))) {
        free(find);SetLastError(ERROR_PATH_NOT_FOUND);return INVALID_HANDLE_VALUE;
    }
    strcpy(find->xbox_directory,xbox_data);
    find->directory=opendir(find->host_directory);
    if(!find->directory) { int saved=errno;free(find);set_errno_error(saved);return INVALID_HANDLE_VALUE; }
    if(pthread_mutex_init(&find->lock,NULL)) { closedir(find->directory);free(find);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    handle=(HANDLE)mac_service_handle_create(MAC_HANDLE_SAVE_FIND,find,save_find_destroy);
    if(!handle) { save_find_destroy(find);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    pthread_mutex_lock(&find->lock);
    if(!save_find_next(find,data)) { pthread_mutex_unlock(&find->lock);mac_service_handle_close(handle,MAC_HANDLE_SAVE_FIND);return INVALID_HANDLE_VALUE; }
    pthread_mutex_unlock(&find->lock);SetLastError(ERROR_SUCCESS);return handle;
}

BOOL WINAPI XFindNextSaveGame(HANDLE handle,PXGAME_FIND_DATA data)
{
    struct mac_save_find *find;void *lease,*payload=NULL;BOOL result;
    if(!data) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    lease=mac_service_handle_acquire(handle,MAC_HANDLE_SAVE_FIND,&payload);if(!lease) { SetLastError(ERROR_INVALID_HANDLE);return FALSE; }
    find=(struct mac_save_find *)payload;pthread_mutex_lock(&find->lock);
    result=save_find_next(find,data);pthread_mutex_unlock(&find->lock);
    mac_file_handle_release(lease);return result;
}

static void nickname_find_destroy(void *payload) { free(payload); }

BOOL WINAPI XSetNicknameW(LPCWSTR nickname,BOOL preserve_case)
{
    size_t i;(void)preserve_case;
    if(!nickname) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    pthread_mutex_lock(&mac_nickname_lock);
    for(i=0;i<MAX_NICKNAME-1&&nickname[i];i++)mac_last_nickname[i]=nickname[i];
    mac_last_nickname[i]=0;
    pthread_mutex_unlock(&mac_nickname_lock);SetLastError(ERROR_SUCCESS);return TRUE;
}

HANDLE WINAPI XFindFirstNicknameW(BOOL this_title_only,LPWSTR nickname,UINT size)
{
    struct mac_nickname_find *find;HANDLE handle;size_t i;(void)this_title_only;
    if(!nickname||!size) { SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE; }
    pthread_mutex_lock(&mac_nickname_lock);
    if(!mac_last_nickname[0]) { pthread_mutex_unlock(&mac_nickname_lock);SetLastError(ERROR_NO_MORE_FILES);return INVALID_HANDLE_VALUE; }
    for(i=0;i+1<size&&i<MAX_NICKNAME-1&&mac_last_nickname[i];i++)nickname[i]=mac_last_nickname[i];
    nickname[i]=0;pthread_mutex_unlock(&mac_nickname_lock);
    find=(struct mac_nickname_find *)calloc(1,sizeof(*find));
    if(!find) { SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    find->marker=1;handle=(HANDLE)mac_service_handle_create(MAC_HANDLE_NICKNAME_FIND,find,nickname_find_destroy);
    if(!handle) { free(find);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    SetLastError(ERROR_SUCCESS);return handle;
}

static void signature_destroy(void *payload)
{
    volatile unsigned char *bytes=(volatile unsigned char *)payload;size_t i;
    pthread_mutex_destroy(&((struct mac_signature *)payload)->lock);
    for(i=0;i<sizeof(struct mac_signature);i++)bytes[i]=0;
    free(payload);
}

HANDLE WINAPI XCalculateSignatureBegin(DWORD flags)
{
    static const unsigned char title_key[]="halo-linux content signature";
    struct mac_signature *signature;HANDLE handle;
    if(flags!=0){SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;}
    signature=(struct mac_signature *)calloc(1,sizeof(*signature));
    if(!signature) { SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    if(pthread_mutex_init(&signature->lock,NULL)) { free(signature);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    mbedtls_sha1_init(&signature->sha1);
    if(mbedtls_sha1_starts(&signature->sha1)!=0||
        mbedtls_sha1_update(&signature->sha1,title_key,sizeof(title_key))!=0) {
        signature_destroy(signature);SetLastError(ERROR_GEN_FAILURE);return INVALID_HANDLE_VALUE;
    }
    handle=(HANDLE)mac_service_handle_create(MAC_HANDLE_SIGNATURE,signature,signature_destroy);
    if(!handle) { signature_destroy(signature);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    SetLastError(ERROR_SUCCESS);return handle;
}

DWORD WINAPI XCalculateSignatureUpdate(HANDLE handle,const BYTE *data,ULONG size)
{
    struct mac_signature *signature;void *lease,*payload=NULL;int result;
    if(size&&!data) { SetLastError(ERROR_INVALID_PARAMETER);return ERROR_INVALID_PARAMETER; }
    lease=mac_service_handle_acquire(handle,MAC_HANDLE_SIGNATURE,&payload);if(!lease) { SetLastError(ERROR_INVALID_HANDLE);return ERROR_INVALID_HANDLE; }
    signature=(struct mac_signature *)payload;
    pthread_mutex_lock(&signature->lock);
    if(signature->consumed) { pthread_mutex_unlock(&signature->lock);mac_file_handle_release(lease);SetLastError(ERROR_INVALID_HANDLE);return ERROR_INVALID_HANDLE; }
    result=mbedtls_sha1_update(&signature->sha1,data,size);
    pthread_mutex_unlock(&signature->lock);
    mac_file_handle_release(lease);
    if(result!=0) { SetLastError(ERROR_GEN_FAILURE);return ERROR_GEN_FAILURE; }
    SetLastError(ERROR_SUCCESS);return ERROR_SUCCESS;
}

DWORD WINAPI XCalculateSignatureEnd(HANDLE handle,PXCALCSIG_SIGNATURE output)
{
    struct mac_signature *signature;void *lease,*payload=NULL;int result;
    if(!output) { SetLastError(ERROR_INVALID_PARAMETER);return ERROR_INVALID_PARAMETER; }
    lease=mac_service_handle_acquire(handle,MAC_HANDLE_SIGNATURE,&payload);if(!lease) { SetLastError(ERROR_INVALID_HANDLE);return ERROR_INVALID_HANDLE; }
    signature=(struct mac_signature *)payload;pthread_mutex_lock(&signature->lock);
    if(signature->consumed) { pthread_mutex_unlock(&signature->lock);mac_file_handle_release(lease);SetLastError(ERROR_INVALID_HANDLE);return ERROR_INVALID_HANDLE; }
    signature->consumed=1;result=mbedtls_sha1_finish(&signature->sha1,output->Signature);
    pthread_mutex_unlock(&signature->lock);
    mac_service_handle_close(handle,MAC_HANDLE_SIGNATURE);mac_file_handle_release(lease);
    if(result!=0) { SetLastError(ERROR_GEN_FAILURE);return ERROR_GEN_FAILURE; }
    SetLastError(ERROR_SUCCESS);return ERROR_SUCCESS;
}
