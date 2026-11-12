/* MCC preview files only. Resolve beneath an already selected package root;
 * never follow metadata-supplied links or read devices, pipes or large files.
 * Native structures stay on the host side of the Android ABI boundary. */
#include "posix.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <strings.h>
#endif

static int component(const char *part)
{
    const unsigned char *p = (const unsigned char *)part;
    size_t n = strlen(part);
    if (!n || n > 127 || part[n-1] == '.' || part[n-1] == ' ') return 0;
    for (; *p; p++) if (*p < 32 || *p == 127 || strchr(":*?\"<>|/\\", *p)) return 0;
    /* Windows device aliases apply even when followed by an extension. */
    if (n >= 3) {
        char base[5]; size_t i, end = strcspn(part, ".");
        if (end <= 4) {
            for (i = 0; i < end; i++) base[i] = (char)tolower((unsigned char)part[i]);
            base[end] = 0;
            if (!strcmp(base,"con") || !strcmp(base,"prn") || !strcmp(base,"aux") || !strcmp(base,"nul") ||
                (end == 4 && (!memcmp(base,"com",3) || !memcmp(base,"lpt",3)) && base[3] >= '0' && base[3] <= '9')) return 0;
        }
    }
    return 1;
}

int posix_mcc_preview_read(const char *root, const char *relative, void *buffer, posix_ulong capacity)
{
    char part[128]; const char *cursor = relative;
    int result = 0;
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE, parent;
    char path[2048], canonical_root[2048], canonical_file[2048];
    DWORD size, got, length, attributes;
    LARGE_INTEGER bytes;
    size_t used;
#else
    int directory = -1, descriptor = -1;
    struct stat status;
    size_t used = 0;
#endif
    if (!root || !relative || !*relative || !buffer || !capacity || capacity > 8*1024*1024) return 0;
#ifdef _WIN32
    parent = CreateFileA(root, FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (parent == INVALID_HANDLE_VALUE) return 0;
    length = GetFinalPathNameByHandleA(parent, canonical_root, sizeof(canonical_root), FILE_NAME_NORMALIZED);
    CloseHandle(parent);
    if (!length || length >= sizeof(canonical_root)-1) return 0;
    while (length && canonical_root[length-1] == '\\') length--;
    canonical_root[length++] = '\\'; canonical_root[length] = 0;
    if (strlen(root) >= sizeof(path)) return 0;
    strcpy(path, root);
#else
    directory = open(root, O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if (directory < 0) return 0;
#endif
    while (*cursor) {
        size_t n = strcspn(cursor, "/\\");
        if (!n || n >= sizeof(part)) goto done;
        memcpy(part, cursor, n); part[n] = 0;
        if (!component(part)) goto done;
        cursor += n;
        if (*cursor && !*++cursor) goto done;
#ifdef _WIN32
        used = strlen(path);
        if (used + n + 2 > sizeof(path)) goto done;
        path[used++] = '\\'; memcpy(path+used, part, n+1);
        handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (handle == INVALID_HANDLE_VALUE) goto done;
        { BY_HANDLE_FILE_INFORMATION info;
          if (!GetFileInformationByHandle(handle, &info)) goto done;
          attributes = info.dwFileAttributes; }
        if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) goto done;
        if (*cursor) {
            if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) goto done;
            CloseHandle(handle); handle = INVALID_HANDLE_VALUE;
        } else if (attributes & FILE_ATTRIBUTE_DIRECTORY) goto done;
#else
        /* Lookup through the held parent descriptor, including on case-sensitive
         * Android imports. Reject ambiguous case aliases rather than guess. */
        DIR *entries; struct dirent *entry; char found[256]; int matches = 0, scan;
        scan = dup(directory);
        if (scan < 0) goto done;
        entries = fdopendir(scan);
        if (!entries) { close(scan); goto done; }
        while ((entry = readdir(entries)) != NULL) if (!strcasecmp(part, entry->d_name)) {
            if (strlen(entry->d_name) < sizeof(found)) { strcpy(found, entry->d_name); matches++; }
        }
        closedir(entries);
        if (matches != 1) goto done;
        descriptor = openat(directory, found, O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC|(*cursor ? O_DIRECTORY : 0));
        if (descriptor < 0) goto done;
        if (*cursor) { close(directory); directory = descriptor; descriptor = -1; }
#endif
    }
#ifdef _WIN32
    if (GetFileType(handle) != FILE_TYPE_DISK || !GetFileSizeEx(handle,&bytes) || bytes.QuadPart <= 0 || bytes.QuadPart > capacity) goto done;
    size = GetFinalPathNameByHandleA(handle, canonical_file, sizeof(canonical_file), FILE_NAME_NORMALIZED);
    if (!size || size >= sizeof(canonical_file) || _strnicmp(canonical_root,canonical_file,length)) goto done;
    size = (DWORD)bytes.QuadPart;
    if (ReadFile(handle,buffer,size,&got,NULL) && got == size) result = (int)size;
#else
    if (descriptor < 0 || fstat(descriptor,&status) || !S_ISREG(status.st_mode) || status.st_size <= 0 || status.st_size > capacity) goto done;
    while (used < (size_t)status.st_size) {
        ssize_t got = read(descriptor,(char *)buffer+used,(size_t)status.st_size-used);
        if (got <= 0) goto done;
        used += (size_t)got;
    }
    { char extra; if (read(descriptor,&extra,1) != 0) goto done; }
    result = (int)used;
#endif
done:
#ifdef _WIN32
    if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
#else
    if (descriptor >= 0) close(descriptor);
    if (directory >= 0) close(directory);
#endif
    return result;
}
