/* MCC download staging only. Remote paths are validated before reaching this
 * boundary; existing links and special files are refused again on the host.
 * Exclusive ownership protects against remote senders, not a local process
 * with the same user's authority concurrently replacing the data directory. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "posix.h"
#include <errno.h>
#include <string.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdint.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/syscall.h>
#endif
#endif

int posix_mcc_share_path_safe(const char *name)
{
    char path[2048];
    size_t i,start,length;
    int missing=0;
    if (!name || !*name || strlen(name)>=sizeof(path)) return 0;
#ifdef _WIN32
    length=GetFullPathNameA(name,sizeof(path),path,NULL);
    if (!length || length>=sizeof(path)) return 0;
    /* Only ordinary drive-root paths; device namespaces and UNC roots are not
     * destinations for managed downloads. Installed package reads are separate. */
    if (length<3 || path[1]!=':' || (path[2]!='\\' && path[2]!='/') || strchr(path+2,':')) return 0;
    start=3;
#else
    strcpy(path,name); length=strlen(path); start=path[0]=='/' ? 1 : 0;
#endif
    for (i=start;i<=length;i++) {
        char saved;
        if (path[i] && path[i]!='/' && path[i]!='\\') continue;
        if (i==start) { start=i+1; continue; }
        if ((i-start==1 && path[start]=='.') ||
            (i-start==2 && path[start]=='.' && path[start+1]=='.')) return 0;
        saved=path[i]; path[i]=0;
        if (!missing) {
#ifdef _WIN32
            DWORD attributes=GetFileAttributesA(path);
            if (attributes==INVALID_FILE_ATTRIBUTES) {
                DWORD error=GetLastError();
                if (error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND) return 0;
                missing=1;
            } else if ((attributes&FILE_ATTRIBUTE_REPARSE_POINT) ||
                (saved && !(attributes&FILE_ATTRIBUTE_DIRECTORY)) || (attributes&FILE_ATTRIBUTE_DEVICE)) return 0;
#else
            struct stat status;
            if (lstat(path,&status)) {
                if (errno!=ENOENT) return 0;
                missing=1;
            } else if (S_ISLNK(status.st_mode) ||
                (saved ? !S_ISDIR(status.st_mode) : !S_ISDIR(status.st_mode) && !S_ISREG(status.st_mode))) return 0;
#endif
        }
        path[i]=saved; start=i+1;
    }
    return 1;
}

int posix_mcc_share_publish(const char *from, const char *to)
{
    if (!posix_mcc_share_path_safe(from) || !posix_mcc_share_path_safe(to)) return 0;
#ifdef _WIN32
    /* MoveFile fails for an existing target; no REPLACE_EXISTING flag. */
    return MoveFileA(from,to)!=0;
#elif defined(__linux__) && defined(SYS_renameat2)
    return syscall(SYS_renameat2,AT_FDCWD,from,AT_FDCWD,to,1 /* RENAME_NOREPLACE */)==0;
#else
    /* Browser persistence needs an equivalent no-replace publication primitive
     * before MCC downloads can be enabled there (Milestone 14). */
    errno=ENOTSUP;
    return 0;
#endif
}

int posix_mcc_share_open(const char *path)
{
    int descriptor;
#ifdef _WIN32
    HANDLE handle;
    BY_HANDLE_FILE_INFORMATION info;
#else
    struct stat status;
#endif
    if (!posix_mcc_share_path_safe(path)) return -1;
#ifdef _WIN32
    handle=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if (handle==INVALID_HANDLE_VALUE) return -1;
    if (GetFileType(handle)!=FILE_TYPE_DISK || !GetFileInformationByHandle(handle,&info) ||
        (info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DEVICE))) {
        CloseHandle(handle); return -1;
    }
    descriptor=_open_osfhandle((intptr_t)handle,_O_RDONLY|_O_BINARY);
    if (descriptor<0) CloseHandle(handle);
#else
    descriptor=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
    if (descriptor>=0 && (fstat(descriptor,&status) || !S_ISREG(status.st_mode))) {
        close(descriptor); descriptor=-1;
    }
#endif
    return descriptor;
}

void posix_mcc_share_close(int descriptor)
{
    if (descriptor<0) return;
#ifdef _WIN32
    _close(descriptor);
#else
    close(descriptor);
#endif
}

int posix_mcc_share_remove(const char *path, int directory)
{
    if ((directory!=0 && directory!=1) || !posix_mcc_share_path_safe(path)) return 0;
#ifdef _WIN32
    return directory ? RemoveDirectoryA(path)!=0 : DeleteFileA(path)!=0;
#else
    return (directory ? rmdir(path) : unlink(path))==0;
#endif
}
