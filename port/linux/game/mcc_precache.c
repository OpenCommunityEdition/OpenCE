/* MCC lobby readiness is polled every second. Keep its successful validation
 * while the map and every possible resource override remain unchanged.
 * Admission and actual loading still validate independently in mcc_cache.c. */
#include "cseries.h"
#include "mcc_cache.h"
#include "mcc_package.h"
#include <xtl.h>
#include <stdio.h>
#include <string.h>

#define MCC_PRECACHE_FILES 9
struct mcc_precache_stamp {
    DWORD present, attributes, size_low, size_high;
    FILETIME created, written;
};
static struct {
    char name[64];
    struct mcc_precache_stamp files[MCC_PRECACHE_FILES];
} mcc_precache;

void mcc_cache_precache_clear(void)
{
    memset(&mcc_precache, 0, sizeof(mcc_precache));
}

static boolean mcc_precache_file(char const *path, struct mcc_precache_stamp *stamp)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    DWORD error;
    memset(stamp, 0, sizeof(*stamp));
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    stamp->present = 1;
    stamp->attributes = data.dwFileAttributes;
    stamp->size_low = data.nFileSizeLow;
    stamp->size_high = data.nFileSizeHigh;
    stamp->created = data.ftCreationTime;
    stamp->written = data.ftLastWriteTime;
    return TRUE;
}

static boolean mcc_precache_files(char const *name, struct mcc_precache_stamp *files)
{
    char path[128];
    unsigned type, set, index = 1;
    if (!mcc_package_map_path(name, path) || !mcc_precache_file(path, files)) return FALSE;
    for (type = 1; type <= 2; type++) {
        if (!mcc_package_resource_path(name, type, path) ||
            !mcc_precache_file(path, &files[index++]) ||
            !mcc_package_resource_path("mcc_maps/root", type, path) ||
            !mcc_precache_file(path, &files[index++])) return FALSE;
        /* Include absent candidates: adding an override or a second matching
         * shared set must invalidate the result as well as editing a file. */
        for (set = 0; set < 2; set++) {
            snprintf(path, sizeof(path), "mcc_maps/_shared/%s/%s.map",
                set ? "multiplayer" : "singleplayer", type == 1 ? "bitmaps" : "sounds");
            if (!mcc_precache_file(path, &files[index++])) return FALSE;
        }
    }
    return TRUE;
}

boolean mcc_cache_precache_ready(char const *name)
{
    char canonical[64];
    struct mcc_precache_stamp before[MCC_PRECACHE_FILES], after[MCC_PRECACHE_FILES];
    boolean stamped;
    if (!mcc_package_name(name, canonical)) {
        mcc_cache_precache_clear();
        return FALSE;
    }
    stamped = mcc_precache_files(canonical, before);
    if (stamped && !strcmp(canonical, mcc_precache.name) &&
        !memcmp(before, mcc_precache.files, sizeof(before))) return TRUE;
    mcc_cache_precache_clear();
    if (!mcc_cache_require(canonical, 0)) return FALSE;
    if (stamped) {
        /* Do not retain a result while an installer is changing its inputs. */
        if (!mcc_precache_files(canonical, after) ||
            memcmp(before, after, sizeof(before))) return FALSE;
        strcpy(mcc_precache.name, canonical);
        memcpy(mcc_precache.files, after, sizeof(after));
    }
    return TRUE;
}
