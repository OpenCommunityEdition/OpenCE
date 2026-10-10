/* Independent MCC cache lifetime and I/O. Source files are opened read-only.
 * Conversion uses exclusively MCC-owned memory and a separate tag window. */
#include "cseries.h"
#include "errors.h"
#include "cache/cache_files.h"
#include "scenario/scenario_definitions.h"
#include "interface/ui_widget.h"
#include "mcc_cache.h"
#include "mcc_maps.h"
#include "mcc_runtime.h"
#include "mcc_tag_validate.h"
#include "mcc_checkpoint.h"
#include "mcc_grenades.h"
#include "mcc_script_parameters.h"
#include "mcc_texture_cache.h"
#include "mcc_ui_teams.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void *mcc_memory_reserve(uint32_t address, uint32_t size);
void mcc_memory_release(void *allocation, uint32_t size);
int mcc_tags_prepare(struct mcc_runtime *runtime);
int mcc_scripts_prepare(struct mcc_runtime *runtime);
short mcc_geometry_part_palette(struct mcc_runtime *runtime, void const *buffer, byte const **nodes);

static struct mcc_runtime mcc_loaded_cache;
static FILE *mcc_stream;
static boolean mcc_loaded;

static int mcc_prefix(char const *name)
{
    static char const prefix[] = "mcc_maps";
    size_t i;
    if (!name)
        return FALSE;
    for (i = 0; i < sizeof(prefix) - 1; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (c != (unsigned char)prefix[i]) return FALSE;
    }
    return name[i] == '\\' || name[i] == '/';
}

boolean mcc_level_name(char const *name)
{
    return mcc_prefix(name);
}

static int mcc_path(char const *name, char path[128])
{
    size_t n, i;
    char const *stem;
    if (!mcc_prefix(name)) return FALSE;
    stem = name + sizeof(MCC_MAPS_LEVEL_PREFIX) - 1;
    n = strlen(stem);
    if (!n || n > 63 - (sizeof(MCC_MAPS_LEVEL_PREFIX) - 1) ||
        stem[n - 1] == '.' || stem[n - 1] == ' ')
        return FALSE;
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)stem[i];
        if (c < 32 || strchr("/\\:*?\"<>|", c)) return FALSE;
    }
    snprintf(path, 128, "%s%s.map", MCC_MAPS_DIRECTORY, stem);
    return TRUE;
}

/* Checkpoints store the scenario tag's name, not the menu's level name.
 * Give this MCC instance its canonical namespace so Continue/reload resolves
 * the same file even when its internal scenario path names a stock level. */
static int mcc_scenario_identity(struct mcc_runtime *runtime, char const *name)
{
    char checked_path[128];
    char *identity;
    uint32_t address, index;
    size_t length;
    if (!runtime || !runtime->tag_index || !mcc_path(name, checked_path)) return FALSE;
    index = runtime->report.scenario_handle & 0xFFFFu;
    if (index >= runtime->report.tag_count) return FALSE;
    length = strlen(name + sizeof(MCC_MAPS_LEVEL_PREFIX) - 1);
    identity = mcc_runtime_allocate(runtime, (uint32_t)(sizeof(MCC_MAPS_LEVEL_PREFIX) + length));
    if (!identity) return FALSE;
    memcpy(identity, MCC_MAPS_LEVEL_PREFIX, sizeof(MCC_MAPS_LEVEL_PREFIX) - 1);
    memcpy(identity + sizeof(MCC_MAPS_LEVEL_PREFIX) - 1,
        name + sizeof(MCC_MAPS_LEVEL_PREFIX) - 1, length + 1);
    address = (uint32_t)(uintptr_t)identity;
    memcpy(runtime->tag_index + index * 32 + 16, &address, sizeof(address));
    return TRUE;
}

static int mcc_file_read(void *context, uint32_t offset, uint32_t bytes, void *out)
{
    FILE *stream = context;
    return stream && offset <= 0x7FFFFFFFu &&
        !fseek(stream, (long)offset, SEEK_SET) && fread(out, 1, bytes, stream) == bytes;
}

static FILE *mcc_open(char const *name, struct mcc_cache_source *source,
    struct mcc_cache_identity *identity)
{
    char path[128];
    unsigned char header[MCC_CACHE_HEADER_BYTES];
    FILE *stream;
    long size;
    if (!mcc_path(name, path)) return NULL;
    stream = fopen(path, "rb");
    if (!stream) return NULL;
    if (fseek(stream, 0, SEEK_END) || (size = ftell(stream)) < 0 ||
        !mcc_file_read(stream, 0, sizeof(header), header) ||
        mcc_cache_identify(header, sizeof(header), (uint64_t)size, identity) != MCC_CACHE_OK) {
        fclose(stream);
        return NULL;
    }
    source->context = stream;
    source->read = mcc_file_read;
    source->size = (uint64_t)size;
    return stream;
}

boolean mcc_cache_available(char const *name)
{
    struct mcc_cache_identity identity;
    struct mcc_cache_source source;
    FILE *stream = mcc_open(name, &source, &identity);
    if (!stream) return FALSE;
    fclose(stream);
    return TRUE;
}

unsigned long mcc_cache_checksum(char const *name)
{
    struct mcc_cache_identity identity;
    struct mcc_cache_source source;
    FILE *stream = mcc_open(name, &source, &identity);
    if (!stream) return 0;
    fclose(stream);
    return identity.checksum;
}

boolean mcc_cache_present(char const *name, unsigned long checksum, char *message, long size)
{
    struct mcc_cache_identity identity;
    struct mcc_cache_source source;
    FILE *stream = mcc_open(name, &source, &identity);
    if (!stream) {
        snprintf(message, (size_t)size, "The MCC map %.63s is missing or is not a valid version-13 cache. Copy its .map file into mcc_maps.", name ? name : "");
        return FALSE;
    }
    fclose(stream);
    if (checksum && identity.checksum != checksum) {
        snprintf(message, (size_t)size, "The MCC map %.63s differs from the host's copy. Put the same version in mcc_maps.", name);
        return FALSE;
    }
    return TRUE;
}

boolean mcc_cache_require(char const *name, unsigned long checksum)
{
    char message[256];
    wchar_t text[256];
    size_t i;
    if (mcc_cache_present(name, checksum, message, sizeof(message))) return TRUE;
    error(_error_silent, "mcc: %s", message);
    for (i = 0; message[i] && i < NUMBEROF(text) - 1; ++i) text[i] = (unsigned char)message[i];
    text[i] = 0;
    display_error_text_when_main_menu_loaded(text);
    return FALSE;
}

void *mcc_runtime_pointer(struct mcc_runtime *runtime, uint32_t address, uint32_t bytes)
{
    uint32_t offset = address - runtime->report.tag_base;
    if (address < runtime->report.tag_base || offset > runtime->used || bytes > runtime->used - offset)
        return NULL;
    return runtime->tags + offset;
}

int mcc_runtime_read(struct mcc_runtime *runtime, uint32_t offset, uint32_t bytes, void *out)
{
    return (uint64_t)offset <= runtime->source.size &&
        bytes <= runtime->source.size - offset &&
        runtime->source.read(runtime->source.context, offset, bytes, out);
}

void *mcc_runtime_allocate(struct mcc_runtime *runtime, uint32_t bytes)
{
    uint32_t limit = runtime->capacity, index;
    uint32_t start = (runtime->used + 15u) & ~15u;
    void *result;
    for (index = 0; index < runtime->report.bsp_count; ++index) {
        uint32_t bsp_offset = runtime->report.bsps[index].address - runtime->report.tag_base;
        if (bsp_offset < limit) limit = bsp_offset;
    }
    if (start < runtime->used || start > limit || bytes > limit - start) return NULL;
    result = runtime->tags + start;
    memset(result, 0, bytes);
    runtime->used = start + bytes;
    return result;
}

boolean mcc_cache_tags_loaded(void) { return mcc_loaded; }

boolean mcc_cache_contains(void const *address, long bytes)
{
    uintptr_t offset = (uintptr_t)address - (uintptr_t)mcc_loaded_cache.tags;
    return mcc_loaded && bytes > 0 && offset <= mcc_loaded_cache.capacity &&
        (uint32_t)bytes <= mcc_loaded_cache.capacity - offset;
}

void mcc_cache_tags_unload(void)
{
    mcc_ui_teams_reset();
    mcc_checkpoint_dispose();
    mcc_grenades_reset();
    mcc_parameters_dispose();
    mcc_loaded = FALSE;
    mcc_texture_cache_dispose();
    mcc_geometry_dispose(&mcc_loaded_cache);
    mcc_audio_dispose(&mcc_loaded_cache);
    mcc_bitmaps_dispose(&mcc_loaded_cache);
    mcc_validation_dispose(&mcc_loaded_cache);
    mcc_memory_release(mcc_loaded_cache.tags, mcc_loaded_cache.capacity);
    if (mcc_stream) fclose(mcc_stream);
    mcc_stream = NULL;
    memset(&mcc_loaded_cache, 0, sizeof(mcc_loaded_cache));
}

struct cache_file_tag_header *mcc_cache_tags_load(char const *name, void *header)
{
    struct mcc_runtime *runtime = &mcc_loaded_cache;
    struct mcc_cache_identity identity;
    unsigned char *raw = NULL;
    enum mcc_cache_status status;
    char const *problem = "the map could not be opened";
    mcc_cache_tags_unload();
    mcc_stream = mcc_open(name, &runtime->source, &identity);
    if (!mcc_stream) goto failed;
    /* The virtual streams are disjoint from the on-disk file. */
    problem = "the file overlaps the MCC virtual resource range";
    if (identity.file_length >= 0x50000000u) goto failed;
    raw = malloc(identity.tag_data_size);
    problem = "not enough memory to inspect tags";
    if (!raw) goto failed;
    status = mcc_cache_load(&runtime->source, raw, identity.tag_data_size, &runtime->report);
    problem = mcc_cache_status_string(status);
    if (status != MCC_CACHE_OK) goto failed;
    runtime->capacity = MCC_CACHE_TAG_CAPACITY;
    runtime->tags = mcc_memory_reserve(runtime->report.tag_base, runtime->capacity);
    problem = "the MCC tag address window is unavailable";
    if (!runtime->tags) goto failed;
    memcpy(runtime->tags, raw, identity.tag_data_size);
    runtime->used = identity.tag_data_size;
    runtime->tag_index = runtime->tags + 40;
    free(raw);
    raw = NULL;
    problem = "the MCC scenario identity could not be retained for saved games";
    if (!mcc_scenario_identity(runtime, name)) goto failed;
    problem = "MCC tag conversion failed (see the preceding diagnostic)";
    if (!mcc_hud_bitmaps_prepare(runtime) || !mcc_tags_prepare(runtime) || !mcc_scripts_prepare(runtime) ||
        !mcc_geometry_prepare(runtime) || !mcc_audio_prepare(runtime) ||
        !mcc_bitmaps_prepare(runtime)) goto failed;
    /* The runtime tag table is stock-shaped. The independent wire reader's
     * report retains model offsets; no other loader sees an MCC header. */
    memset(runtime->tags + 16, 0, 20);
    memcpy(runtime->tags, &runtime->tag_index, 4);
    memcpy(runtime->tags + 12, &runtime->report.tag_count, 4);
    memcpy(runtime->tags + 32, "sgat", 4);
    problem = "converted MCC tags did not pass engine validation";
    if (!mcc_tags_validate(runtime)) goto failed;
    if (!mcc_runtime_read(runtime, 0, MCC_CACHE_HEADER_BYTES, header)) {
        problem = "the map header could not be read";
        goto failed;
    }
    mcc_loaded = TRUE;
    error(_error_silent, "mcc: loaded %s (%lu tags, %lu bytes of runtime tags)", name,
        (unsigned long)runtime->report.tag_count, (unsigned long)runtime->used);
    return (struct cache_file_tag_header *)runtime->tags;
failed:
    error(_error_silent, "mcc: refused %s: %s", name ? name : "(unnamed)", problem);
    {
        wchar_t text[256];
        char message[256];
        size_t i;
        snprintf(message, sizeof(message), "MCC map could not load: %s. See debug.txt for details.", problem);
        for (i = 0; message[i] && i < NUMBEROF(text) - 1; ++i) text[i] = (unsigned char)message[i];
        text[i] = 0;
        display_error_text_when_main_menu_loaded(text);
    }
    if (raw) free(raw);
    mcc_cache_tags_unload();
    return NULL;
}

boolean mcc_cache_read(long tag, long offset, long bytes, void *out)
{
    int result;
    if (!mcc_loaded || offset < 0 || bytes < 0 || !out) return FALSE;
    result = mcc_bitmaps_read(&mcc_loaded_cache, tag, (uint32_t)offset, (uint32_t)bytes, out);
    if (result < 0) result = mcc_audio_read(&mcc_loaded_cache, (uint32_t)offset, (uint32_t)bytes, out);
    if (result < 0) result = mcc_runtime_read(&mcc_loaded_cache, (uint32_t)offset, (uint32_t)bytes, out);
    if (!result) {
        memset(out, 0, (size_t)bytes);
        error(_error_silent, "mcc: I/O failed at %08lx (%ld bytes)", (unsigned long)offset, bytes);
    }
    return result ? TRUE : FALSE;
}

boolean mcc_cache_bsp_load(struct scenario_structure_bsp_reference const *reference,
    void **header, void **structure)
{
    uint32_t i;
    for (i = 0; mcc_loaded && i < mcc_loaded_cache.report.bsp_count; ++i) {
        struct mcc_cache_bsp const *bsp = &mcc_loaded_cache.report.bsps[i];
        if (bsp->tag_handle != (uint32_t)reference->structure_bsp.index) continue;
        if (bsp->address != (uint32_t)(uintptr_t)reference->base_address ||
            bsp->file_offset != (uint32_t)reference->file_offset || bsp->size != (uint32_t)reference->file_size)
            return FALSE;
        if (!mcc_geometry_bsp_load(&mcc_loaded_cache, bsp->tag_handle, bsp->file_offset,
            bsp->size, reference->base_address, structure)) return FALSE;
        *header = reference->base_address;
        if (!mcc_bsp_validate(&mcc_loaded_cache, (long)bsp->tag_handle, *header, (long)bsp->size)) {
            mcc_geometry_bsp_unload(&mcc_loaded_cache);
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

void mcc_cache_bsp_unload(void) { mcc_geometry_bsp_unload(&mcc_loaded_cache); }

short mcc_cache_part_palette(void const *buffer, byte const **nodes)
{
    return mcc_loaded ? mcc_geometry_part_palette(&mcc_loaded_cache, buffer, nodes) : 0;
}

boolean mcc_cache_bitmap_valid(struct bitmap_data *bitmap)
{
    extern int mcc_bitmaps_valid(struct mcc_runtime *runtime, struct bitmap_data *bitmap);
    return mcc_loaded && mcc_bitmaps_valid(&mcc_loaded_cache, bitmap);
}

void const *mcc_cache_bitmap_pixels(struct bitmap_data const *bitmap,unsigned long *bytes)
{
    uint32_t size=0;
    void const *pixels=mcc_loaded ? mcc_bitmaps_pixels(&mcc_loaded_cache,bitmap,&size) : NULL;
    if (bytes) *bytes=size;
    return pixels;
}
