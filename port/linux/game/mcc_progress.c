/* Append-only, bounded completion records in the player's existing directory.
 * A failed/partial append cannot destroy earlier records. Invalid tails are
 * reported and preserved, rather than overwriting a potentially useful save. */
#include "cseries.h"
#include "errors.h"
#include "main/main.h"
#include "interface/player_ui.h"
#include "saved games/player_profile.h"
#include "mcc_progress.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <xtl.h>

#define MCC_PROGRESS_RECORD_BYTES 76
#define MCC_PROGRESS_MAX_RECORDS 4096

static uint32_t mcc_progress_checksum(unsigned char const *record)
{
    uint32_t value = 2166136261u;
    unsigned i;
    for (i = 0; i < MCC_PROGRESS_RECORD_BYTES - 4; i++) value = (value ^ record[i]) * 16777619u;
    return value;
}

static int mcc_progress_level(char const *level)
{
    size_t size;
    if (!level) return FALSE;
    size = strlen(level);
    return size > 9 && size < 64 && !strncmp(level, "mcc_maps\\", 9);
}

static int mcc_progress_path(long profile, char path[300])
{
    char directory[257];
    size_t size;
    if (profile == NONE || !player_profile_get_enclosing_directory_path(profile, directory)) return FALSE;
    directory[256] = 0;
    size = strlen(directory);
    if (!size) return FALSE;
    snprintf(path, 300, "%s%smcc_completed.dat", directory,
        directory[size - 1] == '\\' || directory[size - 1] == '/' ? "" : "\\");
    return TRUE;
}

/* Profile renaming copies the native save before deleting its old directory.
 * Carry the MCC companion across that boundary, or leave the old profile intact. */
boolean mcc_progress_copy(char const *profile_file, char const *new_directory)
{
    char old_path[300], new_path[300];
    char *leaf, *slash;
    DWORD attributes, failure;
    size_t size;
    if (!profile_file || !new_directory || strlen(profile_file) >= 257 || strlen(new_directory) >= 257) return FALSE;
    strcpy(old_path, profile_file);
    leaf = strrchr(old_path, '\\');
    slash = strrchr(old_path, '/');
    if (!leaf || (slash && slash > leaf)) leaf = slash;
    if (!leaf) return FALSE;
    strcpy(leaf + 1, "mcc_completed.dat");
    attributes = GetFileAttributesA(old_path);
    if (attributes == (DWORD)-1)
    {
        failure = GetLastError();
        return failure == ERROR_FILE_NOT_FOUND || failure == ERROR_PATH_NOT_FOUND;
    }
    if (attributes & FILE_ATTRIBUTE_DIRECTORY) return FALSE;
    size = strlen(new_directory);
    if (!size) return FALSE;
    snprintf(new_path, sizeof(new_path), "%s%smcc_completed.dat", new_directory,
        new_directory[size - 1] == '\\' || new_directory[size - 1] == '/' ? "" : "\\");
    if (!CopyFileA(old_path, new_path, TRUE))
    {
        error(_error_silent, "mcc progress: could not preserve completion records while renaming a profile");
        return FALSE;
    }
    return TRUE;
}

/* Return valid-prefix flags even after a damaged tail, but refuse to append
 * to that file. Checksums detect torn writes; this is not an authenticity claim. */
static unsigned mcc_progress_read(FILE *file, char const *level, int *count, int *valid)
{
    unsigned char record[MCC_PROGRESS_RECORD_BYTES];
    unsigned flags = 0, i;
    size_t bytes;
    uint32_t checksum;
    *count = 0;
    *valid = TRUE;
    while ((bytes = fread(record, 1, sizeof(record), file)) != 0)
    {
        if (bytes != sizeof(record)) { *valid = FALSE; break; }
        checksum = 0;
        for (i = 0; i < 4; i++) checksum |= (uint32_t)record[72 + i] << (8 * i);
        if (++*count > MCC_PROGRESS_MAX_RECORDS || bytes != sizeof(record) ||
            memcmp(record, "MCC1", 4) || !memchr(record + 4, 0, 64) ||
            !mcc_progress_level((char const *)record + 4) ||
            !record[68] || (record[68] & ~15u) || record[69] || record[70] || record[71] ||
            checksum != mcc_progress_checksum(record))
        {
            *valid = FALSE;
            break;
        }
        if (!csstrcasecmp((char const *)record + 4, level)) flags |= record[68];
    }
    if (ferror(file)) *valid = FALSE;
    return flags;
}

unsigned mcc_progress_flags(long profile, char const *level)
{
    char path[300];
    FILE *file;
    unsigned flags;
    int count, valid;
    if (!mcc_progress_level(level) || !mcc_progress_path(profile, path) || !(file = fopen(path, "rb"))) return 0;
    flags = mcc_progress_read(file, level, &count, &valid);
    fclose(file);
    return flags;
}

/* The stock selector updates every frame. Cache one selected mission for a
 * second, keyed by the full profile and level identity, including failed reads.
 * No widget or catalog pointer survives a selection or map change. */
unsigned mcc_progress_menu_flags(long profile, char const *level, unsigned long now)
{
    static char selected[64];
    static long selected_profile = NONE;
    static unsigned long read_time;
    static unsigned flags;
    if (profile == NONE || !mcc_progress_level(level)) return 0;
    if (profile != selected_profile || strcmp(level, selected) || now - read_time >= 1000)
    {
        flags = mcc_progress_flags(profile, level);
        strcpy(selected, level);
        selected_profile = profile;
        read_time = now;
    }
    return flags;
}

static boolean mcc_progress_save(long profile, char const *level, short difficulty)
{
    char path[300];
    unsigned char record[MCC_PROGRESS_RECORD_BYTES];
    FILE *file;
    unsigned flags, i;
    int count, valid, success;
    uint32_t checksum;
    if (!mcc_progress_level(level) || difficulty < 0 || difficulty > 3 || !mcc_progress_path(profile, path)) return FALSE;
    file = fopen(path, "a+b");
    if (!file) return FALSE;
    rewind(file);
    flags = mcc_progress_read(file, level, &count, &valid);
    if (flags & (1u << difficulty)) { fclose(file); return TRUE; }
    if (!valid || count >= MCC_PROGRESS_MAX_RECORDS || fseek(file, 0, SEEK_END)) { fclose(file); return FALSE; }
    memset(record, 0, sizeof(record));
    memcpy(record, "MCC1", 4);
    strcpy((char *)record + 4, level);
    record[68] = (unsigned char)(1u << difficulty);
    checksum = mcc_progress_checksum(record);
    for (i = 0; i < 4; i++) record[72 + i] = (unsigned char)(checksum >> (8 * i));
    success = fwrite(record, 1, sizeof(record), file) == sizeof(record);
    if (fclose(file)) success = FALSE;
    return success;
}

void mcc_progress_complete(char const *level, short difficulty)
{
    short player;
    long profile;
    for (player = 0; player < player_spawn_count && player < 4; player++)
    {
        profile = player_ui_get_active_player_profile_index(player);
        if (profile != NONE && !mcc_progress_save(profile, level, difficulty))
            error(_error_silent, "mcc progress: could not record completion of %s for local player %d", level, player);
    }
}
