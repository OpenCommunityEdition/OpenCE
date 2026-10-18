/* Native Darwin implementation of the small synchronous Xbox file subset.
   Drive mapping and create dispositions follow xbox_files.c; file objects
   are owned and leased by the shared native handle registry. */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
/* These host headers may leave Darwin's private fd_set bound visible. The
   clean XDK declarations below use the separate Winsock array ABI. */
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define fast_ftol mac_engine_files_unused_fast_ftol
#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
#undef BUILDING_CSERIES
#undef fast_ftol
#include "engine_handles.h"
#include "engine_config.h"
#include "engine_files.h"
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif
#define MAC_FILE_SIGNED_OFFSET_MAX 0x7fffffffffffffffULL
#define MAC_FILE_MAX_TRANSFER ((size_t)0x7fffffffUL)

struct mac_file {
    int descriptor;
    int readable,writable;
    pthread_mutex_t lock;
    char path[PATH_MAX];
    struct mac_open_file *open_record;
};

struct mac_open_file {
    struct mac_open_file *next;
    char path[PATH_MAX];
    unsigned long access,share;
    dev_t device;ino_t inode;int has_identity;
};
static pthread_mutex_t open_files_lock=PTHREAD_MUTEX_INITIALIZER;
static struct mac_open_file *open_files;
#define MAC_FILE_SHARE_READ 0x00000001UL
#define MAC_FILE_SHARE_WRITE 0x00000002UL
#define MAC_ERROR_SHARING_VIOLATION 32UL

static int share_compatible(const struct mac_open_file *old,unsigned long access,unsigned long share)
{
    return !((access&GENERIC_READ)&&!(old->share&MAC_FILE_SHARE_READ))&&
        !((access&GENERIC_WRITE)&&!(old->share&MAC_FILE_SHARE_WRITE))&&
        !((old->access&GENERIC_READ)&&!(share&MAC_FILE_SHARE_READ))&&
        !((old->access&GENERIC_WRITE)&&!(share&MAC_FILE_SHARE_WRITE));
}
static int open_record_conflicts(const struct mac_open_file *record,const char *path,
    const struct stat *st,unsigned long access,unsigned long share)
{
    int same_path=strcasecmp(record->path,path)==0;
    int same_file=record->has_identity&&st&&record->device==st->st_dev&&record->inode==st->st_ino;
    return (same_path||same_file)&&!share_compatible(record,access,share);
}
static void unlink_open_record(struct mac_open_file *record)
{
    struct mac_open_file **cursor;
    pthread_mutex_lock(&open_files_lock);
    for(cursor=&open_files;*cursor;cursor=&(*cursor)->next)if(*cursor==record) {
        *cursor=record->next;break;
    }
    pthread_mutex_unlock(&open_files_lock);
    free(record);
}

static int path_format(char *out,size_t capacity,const char *format,const char *a,const char *b)
{
    int n=snprintf(out,capacity,format,a,b);
    return n>=0&&(size_t)n<capacity;
}
static int directory(const char *path)
{
    struct stat st;
    return stat(path,&st)==0&&S_ISDIR(st.st_mode);
}
static int mkdir_tree(const char *path)
{
    char copy[PATH_MAX];char *p;
    if(!path||!*path||strlen(path)>=sizeof(copy))return 0;
    strcpy(copy,path);
    for(p=copy+1;*p;++p)if(*p=='/') {
        *p=0;if(mkdir(copy,0755)!=0&&errno!=EEXIST)return 0;
        if(!directory(copy))return 0;*p='/';
    }
    if(mkdir(copy,0755)!=0&&errno!=EEXIST)return 0;
    return directory(copy);
}
static int entry_case_insensitive(const char *directory_path,const char *component,char *actual,size_t capacity)
{
    DIR *dir=opendir(directory_path);struct dirent *entry;int found=0;
    if(!dir)return 0;
    while((entry=readdir(dir))!=NULL)if(strcasecmp(entry->d_name,component)==0) {
        if(strlen(entry->d_name)<capacity) { strcpy(actual,entry->d_name);found=1; }
        break;
    }
    closedir(dir);return found;
}
static int root_has_maps(const char *root)
{
    char path[PATH_MAX],actual[NAME_MAX+1];
    struct stat st;
    return path_format(path,sizeof(path),"%s/%s",root,"maps")&&
        ((stat(path,&st)==0&&S_ISDIR(st.st_mode)) ||
         (entry_case_insensitive(root,"maps",actual,sizeof(actual))&&
          path_format(path,sizeof(path),"%s/%s",root,actual)&&stat(path,&st)==0&&S_ISDIR(st.st_mode)));
}
/* port/linux/src/port_config.c, when linked (the tools that open files
   without the settings do not): paths.data and paths.saves */
extern const char *config_string(const char *name) __attribute__((weak));
static const char *setting_path(const char *environment,const char *name)
{
    const char *value=getenv(environment);
    if(value&&*value)return value;
    value=config_string?config_string(name):NULL;
    return value&&*value?value:NULL;
}
static int data_root(char *root,size_t capacity);
int mac_engine_files_data_root(char *root,size_t capacity);
int mac_engine_files_data_root(char *root,size_t capacity)
{
    return data_root(root,capacity);
}
static int data_root(char *root,size_t capacity)
{
    const char *override=setting_path("HALO_MAC_DATA_ROOT","paths.data"),*base;
    char candidate[PATH_MAX],cwd[PATH_MAX];
    if(override&&*override) {
        if(strlen(override)>=capacity)return 0;
        strcpy(root,override);return 1;
    }
    if(getcwd(cwd,sizeof(cwd))&&root_has_maps(cwd)) {
        if(strlen(cwd)>=capacity)return 0;strcpy(root,cwd);return 1;
    }
    base=mac_config_base_path();
    if(base&&root_has_maps(base)) {
        if(strlen(base)>=capacity)return 0;strcpy(root,base);return 1;
    }
    if(base&&path_format(candidate,sizeof(candidate),"%s/%s",base,"../Resources/GameData")&&root_has_maps(candidate)) {
        if(strlen(candidate)>=capacity)return 0;strcpy(root,candidate);return 1;
    }
    if(base&&path_format(candidate,sizeof(candidate),"%s/%s",base,"../Resources")&&root_has_maps(candidate)) {
        if(strlen(candidate)>=capacity)return 0;strcpy(root,candidate);return 1;
    }
    if(getcwd(cwd,sizeof(cwd))&&strlen(cwd)<capacity) { strcpy(root,cwd);return 1; }
    if(base&&strlen(base)<capacity) { strcpy(root,base);return 1; }
    return 0;
}
static int save_root(char *root,size_t capacity)
{
    const char *override=setting_path("HALO_MAC_SAVE_ROOT","paths.saves"),*home=getenv("HOME");
    int n;
    if(override&&*override) {
        if(strlen(override)>=capacity)return 0;strcpy(root,override);
    } else {
        if(!home||!*home)return 0;
        n=snprintf(root,capacity,"%s/Library/Application Support/OpenCE",home);
        if(n<0||(size_t)n>=capacity)return 0;
    }
    return mkdir_tree(root);
}
static int append_component(char *path,size_t capacity,const char *component)
{
    char actual[NAME_MAX+1];size_t length=strlen(path);const char *use=component;
    char exact[PATH_MAX];struct stat st;
    if(strlen(component)>NAME_MAX)return 0;
    if(path_format(exact,sizeof(exact),"%s/%s",path,component)&&stat(exact,&st)==0)use=component;
    else if(entry_case_insensitive(path,component,actual,sizeof(actual)))use=actual;
    if(length+1+strlen(use)+1>capacity)return 0;
    path[length++]='/';strcpy(path+length,use);return 1;
}
int mac_engine_files_translate_path(const char *xbox_path,char *host_path,size_t host_path_size)
{
    char resolved[PATH_MAX];const char *cursor=xbox_path;char drive='d';
    if(!xbox_path||!host_path||host_path_size==0)return 1;
    if(isalpha((unsigned char)cursor[0])&&cursor[1]==':') { drive=(char)tolower((unsigned char)cursor[0]);cursor+=2; }
    if(drive=='d') { if(!data_root(resolved,sizeof(resolved)))return 1; }
    else if(drive=='h'&&getenv("HALO_MAC_CUSTOM_EDITION_ROOT")&&root_has_maps(getenv("HALO_MAC_CUSTOM_EDITION_ROOT"))) {
        if(strlen(getenv("HALO_MAC_CUSTOM_EDITION_ROOT"))>=sizeof(resolved))return 1;
        strcpy(resolved,getenv("HALO_MAC_CUSTOM_EDITION_ROOT"));
    } else {
        char root[PATH_MAX];
        int n;
        if(!save_root(root,sizeof(root)))return 1;
        n=snprintf(resolved,sizeof(resolved),"%s/%c",root,drive);
        if(n<0||(size_t)n>=sizeof(resolved)||!mkdir_tree(resolved))return 1;
    }
    while(*cursor) {
        char component[NAME_MAX+1];size_t length=0;
        while(*cursor=='/'||*cursor=='\\')++cursor;
        if(!*cursor)break;
        while(*cursor&&*cursor!='/'&&*cursor!='\\') {
            if(length>=NAME_MAX)return 1;
            component[length++]=*cursor++;
        }
        component[length]=0;
        /* FATX never has . or .. components; dropping them prevents escape
           from the selected drive root on a POSIX host. */
        if(length&&strspn(component,". ")!=length&&!append_component(resolved,sizeof(resolved),component))return 1;
    }
    if(strlen(resolved)>=host_path_size)return 1;
    strcpy(host_path,resolved);return 0;
}
/* port: nothing is written into the data root (the Xbox's drive d, the disc, which is read-only
   there too: a path with no drive is on it) or into an app bundle (a host path with a ".app/"
   folder): what the game keeps goes to drive z (the save root, HALO_MAC_SAVE_ROOT or
   ~/Library/Application Support/OpenCE). A write to either is refused (access denied), and said
   the first few times. HALO_MAC_ALLOW_DATA_WRITES=1 lifts it (a diagnostic). */
static int write_refused(const char *name,const char *host_path,const char *what)
{
    const char *cursor=name;char drive='d';
    static unsigned said;
    if(getenv("HALO_MAC_ALLOW_DATA_WRITES")&&getenv("HALO_MAC_ALLOW_DATA_WRITES")[0]=='1')return 0;
    if(isalpha((unsigned char)cursor[0])&&cursor[1]==':')drive=(char)tolower((unsigned char)cursor[0]);
    if(drive!='d'&&!(host_path&&strstr(host_path,".app/")))return 0;
    if(said<8) {
        ++said;fprintf(stderr,"files: %s of %s refused (%s)\n",what,name,drive=='d'?"the data root is read-only":"inside an app");
    }
    return 1;
}
static void set_errno_error(int error)
{
    switch(error) {
    case ENOENT:SetLastError(ERROR_FILE_NOT_FOUND);break;
    case ENOTDIR:SetLastError(ERROR_PATH_NOT_FOUND);break;
    case EACCES:case EPERM:SetLastError(ERROR_ACCESS_DENIED);break;
    case ENOMEM:SetLastError(ERROR_NOT_ENOUGH_MEMORY);break;
    case EMFILE:case ENFILE:SetLastError(ERROR_TOO_MANY_OPEN_FILES);break;
    case EEXIST:SetLastError(ERROR_ALREADY_EXISTS);break;
    case EISDIR:SetLastError(ERROR_ACCESS_DENIED);break;
    case ENOTEMPTY:SetLastError(ERROR_DIR_NOT_EMPTY);break;
    case ENOSPC:SetLastError(ERROR_DISK_FULL);break;
    case EBADF:SetLastError(ERROR_INVALID_HANDLE);break;
    case EINVAL:SetLastError(ERROR_INVALID_PARAMETER);break;
    default:SetLastError(ERROR_GEN_FAILURE);break;
    }
}
static void file_destroy(void *payload)
{
    struct mac_file *file=(struct mac_file *)payload;
    close(file->descriptor);pthread_mutex_destroy(&file->lock);
    unlink_open_record(file->open_record);free(file);
}
static struct mac_file *file_acquire(HANDLE handle,void **lease)
{
    void *payload=NULL;
    *lease=mac_file_handle_acquire(handle,&payload);
    return *lease?(struct mac_file *)payload:NULL;
}
HANDLE WINAPI CreateFileA(LPCSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES security,
    DWORD disposition,DWORD attributes,HANDLE template_file)
{
    struct mac_file *file;struct mac_open_file *record;struct stat st;char path[PATH_MAX];int flags,existed,fd,mode;
    /* This subset supports read/write sharing; delete sharing, inherited
       security, templates, and unknown access/attribute bits are unsupported. */
    if((share&~(MAC_FILE_SHARE_READ|MAC_FILE_SHARE_WRITE))!=0||security||template_file||
       (access&~(GENERIC_READ|GENERIC_WRITE))!=0||
       (attributes&~(FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_NO_BUFFERING))!=0||
       ((attributes&FILE_ATTRIBUTE_NORMAL)&&((attributes&0x0000ffffUL)!=FILE_ATTRIBUTE_NORMAL))) {
        SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;
    }
#ifndef F_NOCACHE
    if(attributes&FILE_FLAG_NO_BUFFERING) { SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE; }
#endif
#ifndef F_RDAHEAD
    if(attributes&FILE_FLAG_SEQUENTIAL_SCAN) { SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE; }
#endif
    if(!name||mac_engine_files_translate_path(name,path,sizeof(path))) { SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE; }
    if(((access&GENERIC_WRITE)||disposition==CREATE_NEW||disposition==CREATE_ALWAYS||disposition==OPEN_ALWAYS||
        disposition==TRUNCATE_EXISTING)&&write_refused(name,path,"opening for writing")) {
        SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;
    }
    existed=stat(path,&st)==0;
    if(existed&&S_ISDIR(st.st_mode)) { SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE; }
    if((access&GENERIC_READ)&&(access&GENERIC_WRITE))flags=O_RDWR;
    else if(access&GENERIC_WRITE)flags=O_WRONLY;
    else flags=O_RDONLY;
    switch(disposition) {
    case CREATE_NEW:flags|=O_CREAT|O_EXCL;break;
    case CREATE_ALWAYS:flags|=O_CREAT;break;
    case OPEN_EXISTING:break;
    case OPEN_ALWAYS:flags|=O_CREAT;break;
    case TRUNCATE_EXISTING:break;
    default:SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;
    }
    if((disposition==CREATE_ALWAYS||disposition==TRUNCATE_EXISTING)&&!(access&GENERIC_WRITE)) {
        SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;
    }
    record=(struct mac_open_file *)calloc(1,sizeof(*record));
    if(!record) { SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    strcpy(record->path,path);record->access=access;record->share=share;
    pthread_mutex_lock(&open_files_lock);
    {
        struct mac_open_file *other;
        for(other=open_files;other;other=other->next)if(open_record_conflicts(other,path,NULL,access,share))break;
        if(other) {
            pthread_mutex_unlock(&open_files_lock);free(record);
            SetLastError(MAC_ERROR_SHARING_VIOLATION);return INVALID_HANDLE_VALUE;
        }
        record->next=open_files;open_files=record;
    }
    pthread_mutex_unlock(&open_files_lock);
    fd=open(path,flags,0644);
    if(fd<0) { int saved_errno=errno;unlink_open_record(record);set_errno_error(saved_errno);if(saved_errno==ENOENT&&!existed&&(flags&O_CREAT))SetLastError(ERROR_PATH_NOT_FOUND);return INVALID_HANDLE_VALUE; }
    if(fstat(fd,&st)!=0) { int saved_errno=errno;close(fd);unlink_open_record(record);set_errno_error(saved_errno);return INVALID_HANDLE_VALUE; }
    pthread_mutex_lock(&open_files_lock);
    {
        struct mac_open_file *other;
        for(other=open_files;other;other=other->next)if(other!=record&&open_record_conflicts(other,path,&st,access,share))break;
        if(other) {
            pthread_mutex_unlock(&open_files_lock);close(fd);unlink_open_record(record);
            SetLastError(MAC_ERROR_SHARING_VIOLATION);return INVALID_HANDLE_VALUE;
        }
        record->device=st.st_dev;record->inode=st.st_ino;record->has_identity=1;
    }
    pthread_mutex_unlock(&open_files_lock);
    fcntl(fd,F_SETFD,FD_CLOEXEC);
#ifdef F_NOCACHE
    if((attributes&FILE_FLAG_NO_BUFFERING)&&fcntl(fd,F_NOCACHE,1)!=0) {
        int saved_errno=errno;close(fd);unlink_open_record(record);set_errno_error(saved_errno);return INVALID_HANDLE_VALUE;
    }
#endif
#ifdef F_RDAHEAD
    if((attributes&FILE_FLAG_SEQUENTIAL_SCAN)&&fcntl(fd,F_RDAHEAD,1)!=0) {
        int saved_errno=errno;close(fd);unlink_open_record(record);set_errno_error(saved_errno);return INVALID_HANDLE_VALUE;
    }
#endif
    if(disposition==CREATE_ALWAYS||disposition==TRUNCATE_EXISTING) {
        if(ftruncate(fd,0)!=0) { int saved_errno=errno;close(fd);unlink_open_record(record);set_errno_error(saved_errno);return INVALID_HANDLE_VALUE; }
    }
    file=(struct mac_file *)calloc(1,sizeof(*file));
    if(!file) { close(fd);unlink_open_record(record);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    if(pthread_mutex_init(&file->lock,NULL)) { close(fd);free(file);unlink_open_record(record);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    file->descriptor=fd;file->readable=(access&GENERIC_READ)!=0;file->writable=(access&GENERIC_WRITE)!=0;
    strcpy(file->path,path);file->open_record=record;
    mode=(existed&&(disposition==OPEN_ALWAYS||disposition==CREATE_ALWAYS));
    {
        HANDLE handle=(HANDLE)mac_file_handle_create(file,file_destroy);
        if(!handle) { file_destroy(file);return INVALID_HANDLE_VALUE; }
        SetLastError(mode?ERROR_ALREADY_EXISTS:ERROR_SUCCESS);return handle;
    }
}
static int transfer_file(HANDLE handle,void *buffer,DWORD count,LPDWORD transferred,LPOVERLAPPED overlapped,int writing)
{
    struct mac_file *file;void *lease;uint32_t done=0;int result=1;uint64_t offset=0;
    if(transferred)*transferred=0;
    if(!transferred&&!overlapped) { SetLastError(ERROR_INVALID_PARAMETER);return 0; }
    if(count&&!buffer) { SetLastError(ERROR_INVALID_PARAMETER);return 0; }
    file=file_acquire(handle,&lease);if(!file)return 0;
    if((writing&&!file->writable)||(!writing&&!file->readable)) { SetLastError(ERROR_ACCESS_DENIED);mac_file_handle_release(lease);return 0; }
    /* OVERLAPPED requests are synchronous explicit-offset I/O: completion is
       reported before return and the event is signaled here. Async file flags
       are rejected by CreateFileA until a real asynchronous backend exists. */
    if(overlapped) {
        offset=((uint64_t)overlapped->OffsetHigh<<32)|overlapped->Offset;
        if(offset>MAC_FILE_SIGNED_OFFSET_MAX||(uint64_t)count>MAC_FILE_SIGNED_OFFSET_MAX-offset) {
            SetLastError(ERROR_INVALID_PARAMETER);mac_file_handle_release(lease);return 0;
        }
    }
    pthread_mutex_lock(&file->lock);
    while(done<count) {
        ssize_t n;
        size_t amount=(size_t)(count-done);
        if(amount>MAC_FILE_MAX_TRANSFER)amount=MAC_FILE_MAX_TRANSFER;
        if(overlapped) n=writing?pwrite(file->descriptor,(const char *)buffer+done,amount,(off_t)(offset+done)):
            pread(file->descriptor,(char *)buffer+done,amount,(off_t)(offset+done));
        else n=writing?write(file->descriptor,(const char *)buffer+done,amount):read(file->descriptor,(char *)buffer+done,amount);
        if(n<0) { if(errno==EINTR)continue;set_errno_error(errno);result=0;break; }
        if(n==0)break;
        done+=(uint32_t)n;
    }
    pthread_mutex_unlock(&file->lock);
    if(transferred)*transferred=done;
    if(overlapped) {
        overlapped->Internal=result?ERROR_SUCCESS:GetLastError();overlapped->InternalHigh=done;
        if(overlapped->hEvent&&!SetEvent(overlapped->hEvent))result=0;
    } else if(result)SetLastError(ERROR_SUCCESS);
    mac_file_handle_release(lease);return result;
}
BOOL WINAPI ReadFile(HANDLE handle,LPVOID buffer,DWORD count,LPDWORD bytes_read,LPOVERLAPPED overlapped)
{ return transfer_file(handle,buffer,count,bytes_read,overlapped,0); }
BOOL WINAPI WriteFile(HANDLE handle,LPCVOID buffer,DWORD count,LPDWORD bytes_written,LPOVERLAPPED overlapped)
{ return transfer_file(handle,(void *)buffer,count,bytes_written,overlapped,1); }
DWORD WINAPI SetFilePointer(HANDLE handle,LONG distance,PLONG distance_high,DWORD method)
{
    struct mac_file *file;void *lease;off_t displacement;int whence;off_t result;
    file=file_acquire(handle,&lease);if(!file)return INVALID_SET_FILE_POINTER;
    if(method==FILE_BEGIN)whence=SEEK_SET;else if(method==FILE_CURRENT)whence=SEEK_CUR;else if(method==FILE_END)whence=SEEK_END;
    else { SetLastError(ERROR_INVALID_PARAMETER);mac_file_handle_release(lease);return INVALID_SET_FILE_POINTER; }
    if(distance_high)displacement=(off_t)(((uint64_t)(uint32_t)*distance_high<<32)|(uint32_t)distance);
    else displacement=(off_t)(int32_t)distance;
    pthread_mutex_lock(&file->lock);result=lseek(file->descriptor,displacement,whence);pthread_mutex_unlock(&file->lock);
    if(result==(off_t)-1) { set_errno_error(errno);mac_file_handle_release(lease);return INVALID_SET_FILE_POINTER; }
    if(distance_high)*distance_high=(LONG)(uint32_t)((uint64_t)result>>32);
    mac_file_handle_release(lease);SetLastError(ERROR_SUCCESS);return (DWORD)(uint32_t)result;
}
DWORD WINAPI GetFileSize(HANDLE handle,LPDWORD size_high)
{
    struct mac_file *file;void *lease;struct stat st;
    file=file_acquire(handle,&lease);if(!file)return INVALID_FILE_SIZE;
    if(fstat(file->descriptor,&st)!=0) { set_errno_error(errno);mac_file_handle_release(lease);return INVALID_FILE_SIZE; }
    if(size_high)*size_high=(DWORD)((uint64_t)st.st_size>>32);
    mac_file_handle_release(lease);SetLastError(ERROR_SUCCESS);return (DWORD)(uint64_t)st.st_size;
}

static void unix_time_to_filetime(time_t seconds,long nanoseconds,FILETIME *value)
{
    int64_t adjusted=(int64_t)seconds+11644473600LL;
    uint64_t ticks;
    if(adjusted<0)adjusted=0;
    ticks=(uint64_t)adjusted*10000000ULL+(uint64_t)(nanoseconds<0?0:nanoseconds)/100ULL;
    value->dwLowDateTime=(DWORD)ticks;value->dwHighDateTime=(DWORD)(ticks>>32);
}
static void fill_file_attributes(const struct stat *st,WIN32_FILE_ATTRIBUTE_DATA *data)
{
    data->dwFileAttributes=0;
    if(S_ISDIR(st->st_mode))data->dwFileAttributes|=FILE_ATTRIBUTE_DIRECTORY;
    if(!(st->st_mode&(S_IWUSR|S_IWGRP|S_IWOTH)))data->dwFileAttributes|=FILE_ATTRIBUTE_READONLY;
    if(data->dwFileAttributes==0)data->dwFileAttributes=FILE_ATTRIBUTE_NORMAL;
    /* Darwin 10.4 exposes no stable birth-time field here; ctime is a
       metadata-change-time proxy, not the file's actual creation time. */
    unix_time_to_filetime(st->st_ctime,0,&data->ftCreationTime);
    unix_time_to_filetime(st->st_atimespec.tv_sec,st->st_atimespec.tv_nsec,&data->ftLastAccessTime);
    unix_time_to_filetime(st->st_mtimespec.tv_sec,st->st_mtimespec.tv_nsec,&data->ftLastWriteTime);
    data->nFileSizeHigh=(DWORD)((uint64_t)st->st_size>>32);data->nFileSizeLow=(DWORD)st->st_size;
}
DWORD WINAPI GetFileAttributesA(LPCSTR name)
{
    char path[PATH_MAX];struct stat st;WIN32_FILE_ATTRIBUTE_DATA data;
    if(!name||mac_engine_files_translate_path(name,path,sizeof(path))) { SetLastError(ERROR_INVALID_PARAMETER);return (DWORD)-1; }
    if(stat(path,&st)!=0) { set_errno_error(errno);return (DWORD)-1; }
    fill_file_attributes(&st,&data);SetLastError(ERROR_SUCCESS);return data.dwFileAttributes;
}
BOOL WINAPI GetFileAttributesExA(LPCSTR name,GET_FILEEX_INFO_LEVELS level,LPVOID information)
{
    char path[PATH_MAX];struct stat st;
    if(level!=GetFileExInfoStandard||!name||!information||mac_engine_files_translate_path(name,path,sizeof(path))) {
        SetLastError(ERROR_INVALID_PARAMETER);return FALSE;
    }
    if(stat(path,&st)!=0) { set_errno_error(errno);return FALSE; }
    fill_file_attributes(&st,(WIN32_FILE_ATTRIBUTE_DATA *)information);SetLastError(ERROR_SUCCESS);return TRUE;
}
static int path_has_open_file_locked(const char *path,const struct stat *st)
{
    struct mac_open_file *record;int found=0;
    for(record=open_files;record;record=record->next)if(strcasecmp(record->path,path)==0||
        (st&&record->has_identity&&record->device==st->st_dev&&record->inode==st->st_ino)) { found=1;break; }
    return found;
}
BOOL WINAPI DeleteFileA(LPCSTR name)
{
    char path[PATH_MAX];struct stat st;
    if(!name||mac_engine_files_translate_path(name,path,sizeof(path))) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    if(write_refused(name,path,"deleting")) { SetLastError(ERROR_ACCESS_DENIED);return FALSE; }
    if(lstat(path,&st)!=0) { set_errno_error(errno);return FALSE; }
    if(S_ISDIR(st.st_mode)) { SetLastError(ERROR_ACCESS_DENIED);return FALSE; }
    pthread_mutex_lock(&open_files_lock);
    if(path_has_open_file_locked(path,&st)) {
        pthread_mutex_unlock(&open_files_lock);SetLastError(MAC_ERROR_SHARING_VIOLATION);return FALSE;
    }
    if(unlink(path)!=0) { int saved_errno=errno;pthread_mutex_unlock(&open_files_lock);set_errno_error(saved_errno);return FALSE; }
    pthread_mutex_unlock(&open_files_lock);
    SetLastError(ERROR_SUCCESS);return TRUE;
}
/* Map sharing renames each downloaded map into place when it is whole
   (port/linux/game/map_share_files.c), and counts on Windows' rename, which
   refuses a name that is taken rather than replacing it. */
BOOL WINAPI MoveFileA(LPCSTR existing_name,LPCSTR new_name)
{
    char from[PATH_MAX],to[PATH_MAX];struct stat st;
    if(!existing_name||!new_name||
        mac_engine_files_translate_path(existing_name,from,sizeof(from))||
        mac_engine_files_translate_path(new_name,to,sizeof(to))) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    if(lstat(to,&st)==0) { SetLastError(ERROR_ALREADY_EXISTS);return FALSE; }
    if(lstat(from,&st)!=0) { set_errno_error(errno);return FALSE; }
    pthread_mutex_lock(&open_files_lock);
    if(path_has_open_file_locked(from,&st)) {
        pthread_mutex_unlock(&open_files_lock);SetLastError(MAC_ERROR_SHARING_VIOLATION);return FALSE;
    }
    if(rename(from,to)!=0) { int saved_errno=errno;pthread_mutex_unlock(&open_files_lock);set_errno_error(saved_errno);return FALSE; }
    pthread_mutex_unlock(&open_files_lock);
    SetLastError(ERROR_SUCCESS);return TRUE;
}
BOOL WINAPI CreateDirectoryA(LPCSTR name,LPSECURITY_ATTRIBUTES security)
{
    char path[PATH_MAX];
    if(security||!name||mac_engine_files_translate_path(name,path,sizeof(path))) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    if(write_refused(name,path,"making a folder")) { SetLastError(ERROR_ACCESS_DENIED);return FALSE; }
    if(mkdir(path,0755)!=0) { set_errno_error(errno);return FALSE; }
    SetLastError(ERROR_SUCCESS);return TRUE;
}
BOOL WINAPI RemoveDirectoryA(LPCSTR name)
{
    char path[PATH_MAX];
    if(!name||mac_engine_files_translate_path(name,path,sizeof(path))) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    if(write_refused(name,path,"removing a folder")) { SetLastError(ERROR_ACCESS_DENIED);return FALSE; }
    if(rmdir(path)!=0) { set_errno_error(errno);return FALSE; }
    SetLastError(ERROR_SUCCESS);return TRUE;
}

struct mac_file_find {
    DIR *directory;
    pthread_mutex_t lock;
    char directory_path[PATH_MAX];
    char pattern[260];
};
static void find_destroy(void *payload)
{
    struct mac_file_find *find=(struct mac_file_find *)payload;
    if(find->directory)closedir(find->directory);
    pthread_mutex_destroy(&find->lock);free(find);
}
static int wildcard_match(const char *pattern,const char *name)
{
    const char *star=NULL,*retry=NULL;
    if(strcmp(pattern,"*.*")==0)pattern="*";
    while(*name) {
        if(*pattern=='?'||tolower((unsigned char)*pattern)==tolower((unsigned char)*name)) { ++pattern;++name;continue; }
        if(*pattern=='*') { star=++pattern;retry=name;continue; }
        if(star) { pattern=star;name=++retry;continue; }
        return 0;
    }
    while(*pattern=='*')++pattern;
    return *pattern==0;
}
static BOOL find_next_entry(struct mac_file_find *find,LPWIN32_FIND_DATAA data)
{
    struct dirent *entry;
    for(;;) {
        char full_path[PATH_MAX];struct stat st;int n;
        errno=0;
        entry=readdir(find->directory);
        if(!entry) {
            if(errno) set_errno_error(errno);else SetLastError(ERROR_NO_MORE_FILES);
            return FALSE;
        }
        if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,"..")||!wildcard_match(find->pattern,entry->d_name))continue;
        n=snprintf(full_path,sizeof(full_path),"%s/%s",find->directory_path,entry->d_name);
        if(n<0||(size_t)n>=sizeof(full_path)||stat(full_path,&st)!=0)continue;
        if(strlen(entry->d_name)>=sizeof(data->cFileName))continue;
        {
            WIN32_FILE_ATTRIBUTE_DATA attributes;
            memset(data,0,sizeof(*data));fill_file_attributes(&st,&attributes);
            data->dwFileAttributes=attributes.dwFileAttributes;
            data->ftCreationTime=attributes.ftCreationTime;
            data->ftLastAccessTime=attributes.ftLastAccessTime;
            data->ftLastWriteTime=attributes.ftLastWriteTime;
            data->nFileSizeHigh=attributes.nFileSizeHigh;data->nFileSizeLow=attributes.nFileSizeLow;
        }
        strcpy(data->cFileName,entry->d_name);return TRUE;
    }
}
HANDLE WINAPI FindFirstFileA(LPCSTR name,LPWIN32_FIND_DATAA data)
{
    struct mac_file_find *find;char directory_name[PATH_MAX];const char *separator;size_t directory_length;HANDLE handle;
    if(!name||!data) { SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE; }
    {
        const char *backslash=strrchr(name,'\\'),*slash=strrchr(name,'/');
        separator=backslash&&slash?(backslash>slash?backslash:slash):(backslash?backslash:slash);
    }
    if(separator) {
        directory_length=(size_t)(separator-name);
        if(directory_length>=sizeof(directory_name)||strlen(separator+1)>=sizeof(((struct mac_file_find *)0)->pattern)) {
            SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;
        }
        memcpy(directory_name,name,directory_length);directory_name[directory_length]=0;
    } else { directory_name[0]=0;if(strlen(name)>=sizeof(((struct mac_file_find *)0)->pattern)) { SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE; } }
    find=(struct mac_file_find *)calloc(1,sizeof(*find));
    if(!find) { SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    strcpy(find->pattern,separator?separator+1:name);
    if(mac_engine_files_translate_path(directory_name,find->directory_path,sizeof(find->directory_path))) {
        free(find);SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;
    }
    find->directory=opendir(find->directory_path);
    if(!find->directory) { int saved_errno=errno;free(find);set_errno_error(saved_errno);return INVALID_HANDLE_VALUE; }
    if(pthread_mutex_init(&find->lock,NULL)) { closedir(find->directory);free(find);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    handle=(HANDLE)mac_find_handle_create(find,find_destroy);
    if(!handle) { find_destroy(find);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE; }
    pthread_mutex_lock(&find->lock);
    if(!find_next_entry(find,data)) {
        int error=(int)GetLastError();
        pthread_mutex_unlock(&find->lock);mac_find_handle_close(handle);
        if(error==ERROR_NO_MORE_FILES)SetLastError(ERROR_FILE_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }
    pthread_mutex_unlock(&find->lock);SetLastError(ERROR_SUCCESS);return handle;
}
BOOL WINAPI FindNextFileA(HANDLE handle,LPWIN32_FIND_DATAA data)
{
    struct mac_file_find *find;void *lease,*payload=NULL;
    if(!data) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    lease=mac_find_handle_acquire(handle,&payload);find=(struct mac_file_find *)payload;
    if(!lease) { SetLastError(ERROR_INVALID_HANDLE);return FALSE; }
    pthread_mutex_lock(&find->lock);
    {
        BOOL result=find_next_entry(find,data);
        pthread_mutex_unlock(&find->lock);mac_find_handle_release(lease);return result;
    }
}
BOOL WINAPI FindClose(HANDLE handle)
{ return mac_find_handle_close(handle); }
BOOL WINAPI XFindClose(HANDLE handle)
{ return mac_find_handle_close(handle); }

/* These operations share the same leased descriptors and drive mapping as
   normal reads/writes. No asynchronous completion or alternate path namespace. */
BOOL WINAPI SetEndOfFile(HANDLE handle)
{
    struct mac_file *file;void *lease;off_t position;int result,saved_errno;
    file=file_acquire(handle,&lease);if(!file)return FALSE;
    if(!file->writable) {mac_file_handle_release(lease);SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    pthread_mutex_lock(&file->lock);
    position=lseek(file->descriptor,0,SEEK_CUR);
    result=position==(off_t)-1?-1:ftruncate(file->descriptor,position);
    saved_errno=errno;pthread_mutex_unlock(&file->lock);mac_file_handle_release(lease);
    if(result) {set_errno_error(saved_errno);return FALSE;}
    SetLastError(ERROR_SUCCESS);return TRUE;
}
BOOL WINAPI SetFileAttributesA(LPCSTR name,DWORD attributes)
{
    char path[PATH_MAX];struct stat st;mode_t mode;
    if(!name || (attributes!=FILE_ATTRIBUTE_NORMAL && attributes!=FILE_ATTRIBUTE_READONLY) ||
        mac_engine_files_translate_path(name,path,sizeof(path))) {
        SetLastError(ERROR_INVALID_PARAMETER);return FALSE;
    }
    if(stat(path,&st)) {set_errno_error(errno);return FALSE;}
    mode=st.st_mode&07777;
    if(attributes==FILE_ATTRIBUTE_READONLY)mode&=~(S_IWUSR|S_IWGRP|S_IWOTH);
    else mode|=S_IWUSR;
    if(chmod(path,mode)) {set_errno_error(errno);return FALSE;}
    SetLastError(ERROR_SUCCESS);return TRUE;
}
BOOL WINAPI CopyFileA(LPCSTR source,LPCSTR destination,BOOL fail_if_exists)
{
    HANDLE input,output;char buffer[16384];DWORD count,written,error,attributes;
    BOOL result=TRUE;
    input=CreateFileA(source,GENERIC_READ,MAC_FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(input==INVALID_HANDLE_VALUE)return FALSE;
    attributes=GetFileAttributesA(source);
    /* The source's deny-write lease also rejects aliases/hard links to itself
       before CreateFileA can truncate the destination. */
    output=CreateFileA(destination,GENERIC_WRITE,0,NULL,fail_if_exists?CREATE_NEW:CREATE_ALWAYS,0,NULL);
    if(output==INVALID_HANDLE_VALUE) {error=GetLastError();CloseHandle(input);SetLastError(error);return FALSE;}
    for(;;) {
        if(!ReadFile(input,buffer,sizeof(buffer),&count,NULL)) {result=FALSE;break;}
        if(!count)break;
        if(!WriteFile(output,buffer,count,&written,NULL)) {result=FALSE;break;}
        if(written!=count) {SetLastError(ERROR_DISK_FULL);result=FALSE;break;}
    }
    error=GetLastError();CloseHandle(output);CloseHandle(input);
    if(result && attributes!=(DWORD)-1 && (attributes&FILE_ATTRIBUTE_READONLY))
        return SetFileAttributesA(destination,FILE_ATTRIBUTE_READONLY);
    SetLastError(result?ERROR_SUCCESS:error);return result;
}
BOOL WINAPI GetDiskFreeSpaceExA(LPCSTR directory_name,PULARGE_INTEGER available,
    PULARGE_INTEGER total,PULARGE_INTEGER free_bytes)
{
    char path[PATH_MAX];struct statvfs fs;
    if(mac_engine_files_translate_path(directory_name?directory_name:"d:",path,sizeof(path))) {
        SetLastError(ERROR_INVALID_PARAMETER);return FALSE;
    }
    if(statvfs(path,&fs)) {set_errno_error(errno);return FALSE;}
    if(available)available->QuadPart=(uint64_t)fs.f_bavail*(uint64_t)fs.f_frsize;
    if(total)total->QuadPart=(uint64_t)fs.f_blocks*(uint64_t)fs.f_frsize;
    if(free_bytes)free_bytes->QuadPart=(uint64_t)fs.f_bfree*(uint64_t)fs.f_frsize;
    SetLastError(ERROR_SUCCESS);return TRUE;
}
