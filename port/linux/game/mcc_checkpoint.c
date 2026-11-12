/* MCC grenade counts travel inside the complete native game-state image.
 * The CPU allocator's high-water mark proves this fixed tail is unused;
 * no game_state_malloc call, pool size, or allocation checksum is changed.
 * Original tail bytes are retained and restored when MCC ownership ends. */
#include "cseries.h"
#include "errors.h"
#include "saved games/game_state.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "mcc_cache.h"
#include "mcc_checkpoint.h"
#include "mcc_grenades.h"
#include "mcc_campaign.h"
#include "mcc_object_names.h"
#include "mcc_script_threads.h"
#include "../../third_party/zlib/zlib_prefixed.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MCC_CHECKPOINT_PAYLOAD_LIMIT (8u + 8192u * 8u)
#define MCC_CHECKPOINT_CAMPAIGN_BYTES MCC_CAMPAIGN_SNAPSHOT_V5_BYTES
#define MCC_CHECKPOINT_TOTAL_PAYLOAD (MCC_CHECKPOINT_PAYLOAD_LIMIT + MCC_CHECKPOINT_CAMPAIGN_BYTES)
#define MCC_CHECKPOINT_FOOTER_BYTES 96u
#define MCC_CHECKPOINT_SLOT_BYTES (MCC_CHECKPOINT_TOTAL_PAYLOAD + MCC_CHECKPOINT_FOOTER_BYTES)

/* Reserve below every supported name/inventory slot, aligned for thread data. */
void *mcc_checkpoint_script_storage(void *image, unsigned long used,
    unsigned long capacity, unsigned long bytes)
{
    unsigned long reserved=MCC_CHECKPOINT_SLOT_BYTES+MCC_OBJECT_NAMES_SNAPSHOT_MAXIMUM+16;
    unsigned long offset;
    if (!image || capacity<reserved || bytes>capacity-reserved) return NULL;
    offset=(capacity-reserved-bytes)&~15u;
    return used<=offset ? (unsigned char *)image+offset : NULL;
}

static struct {
    unsigned char *original;
    unsigned char *address;
} mcc_checkpoint_owner;

static struct {
    unsigned char *original, *address;
    unsigned long bytes;
} mcc_checkpoint_names_owner;

static uint32_t mcc_checkpoint_word(void const *data)
{
    unsigned char const *p = data;
    return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void mcc_checkpoint_put(void *data, uint32_t value)
{
    unsigned char *p = data;
    p[0] = (unsigned char)value; p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16); p[3] = (unsigned char)(value >> 24);
}

static uint32_t mcc_checkpoint_checksum(unsigned char const *data, uint32_t bytes)
{
    return (uint32_t)crc32(0, data, (uInt)bytes);
}

static unsigned char *mcc_checkpoint_tail(void const *image, unsigned long total,
    unsigned long used, unsigned long capacity)
{
    if (!image || total < sizeof(struct game_state_header) || capacity > total ||
        used < sizeof(struct game_state_header) || used > capacity ||
        MCC_CHECKPOINT_SLOT_BYTES > capacity - used) return NULL;
    return (unsigned char *)image + capacity - MCC_CHECKPOINT_SLOT_BYTES;
}

static int mcc_checkpoint_identity(struct game_state_header const *header)
{
    return memchr(header->map_name, 0, 64) && mcc_level_name(header->map_name);
}

static int mcc_checkpoint_claim(unsigned char *tail)
{
    if (mcc_checkpoint_owner.original) return mcc_checkpoint_owner.address == tail;
    mcc_checkpoint_owner.original = malloc(MCC_CHECKPOINT_SLOT_BYTES);
    if (!mcc_checkpoint_owner.original) return 0;
    memcpy(mcc_checkpoint_owner.original, tail, MCC_CHECKPOINT_SLOT_BYTES);
    mcc_checkpoint_owner.address = tail;
    return 1;
}

/* Saved native pointers still address the fixed live arena. Translate them
 * only after proving the full prospective object/header lies in CPU memory. */
static void const *mcc_checkpoint_incoming(void const *image, void const *current,
    unsigned long used, void const *address, unsigned long bytes)
{
    uintptr_t offset = (uintptr_t)address - (uintptr_t)current;
    if (offset > used || bytes > used - offset) return NULL;
    return (unsigned char const *)image + offset;
}

static int mcc_checkpoint_units(void const *image, void const *current,
    unsigned long used, unsigned char const *payload)
{
    unsigned count = payload[6] | ((unsigned)payload[7] << 8), i;
    struct data_array const *array;
    struct object_header_datum const *headers;
    if (!count) return 1;
    array = mcc_checkpoint_incoming(image, current, used, object_header_data, sizeof(*array));
    if (!array || !array->valid || array->size != sizeof(*headers) || array->count < 0 ||
        array->count > 8192 || array->count > array->maximum_count) return 0;
    headers = mcc_checkpoint_incoming(image, current, used, array->data,
        (unsigned long)array->count * sizeof(*headers));
    if (!headers) return 0;
    for (i = 0; i < count; ++i) {
        unsigned char const *record = payload + 8 + i * 8;
        uint32_t handle = mcc_checkpoint_word(record), index = handle & 0xFFFFu;
        struct object_header_datum const *object;
        if (index >= (unsigned)array->count) return 0;
        object = headers + index;
        if (!(uint16_t)object->identifier || (uint16_t)object->identifier != (uint16_t)(handle >> 16) ||
            object->type >= 32 || !(_object_mask_unit & (1u << object->type)) ||
            !mcc_checkpoint_incoming(image, current, used, object->datum, sizeof(struct object_datum)) ||
            (record[4] && mcc_grenades_type_count() < 3) ||
            (record[5] && mcc_grenades_type_count() < 4)) return 0;
    }
    return 1;
}

/* Extended names occupy a second checked slot immediately before the existing
 * footer/payload. Small MCC maps retain their existing checkpoint format. */
static unsigned char *mcc_checkpoint_names_tail(void const *image, unsigned long used,
    unsigned long capacity, unsigned long bytes)
{
    unsigned long required = MCC_CHECKPOINT_SLOT_BYTES + bytes + 16;
    if (used > capacity || required > capacity - used) return NULL;
    return (unsigned char *)image + capacity - required;
}

static int mcc_checkpoint_names_claim(unsigned char *tail, unsigned long bytes)
{
    if (mcc_checkpoint_names_owner.original)
        return mcc_checkpoint_names_owner.address == tail && mcc_checkpoint_names_owner.bytes == bytes;
    mcc_checkpoint_names_owner.original = malloc(bytes);
    if (!mcc_checkpoint_names_owner.original) return 0;
    memcpy(mcc_checkpoint_names_owner.original, tail, bytes);
    mcc_checkpoint_names_owner.address = tail;
    mcc_checkpoint_names_owner.bytes = bytes;
    return 1;
}

static int mcc_checkpoint_names_capture(void *image, unsigned long used, unsigned long capacity)
{
    unsigned long bytes = mcc_object_names_snapshot(NULL);
    unsigned char *tail;
    if (!bytes) return 1;
    tail = mcc_checkpoint_names_tail(image, used, capacity, bytes);
    if (!tail || !mcc_checkpoint_names_claim(tail, bytes + 16)) return 0;
    memset(tail, 0, 16);
    if (mcc_object_names_snapshot(tail + 16) != bytes) return 0;
    memcpy(tail, "MCCN", 4);
    mcc_checkpoint_put(tail + 4, 1);
    mcc_checkpoint_put(tail + 8, bytes);
    mcc_checkpoint_put(tail + 12, mcc_checkpoint_checksum(tail + 16, bytes));
    return 1;
}

static int mcc_checkpoint_names_validate(void const *image, void const *current,
    unsigned long used, unsigned long capacity)
{
    unsigned long bytes = mcc_object_names_snapshot(NULL), i;
    unsigned char *tail, *current_tail;
    struct data_array const *array;
    struct object_header_datum const *headers;
    if (!bytes) return 1;
    tail = mcc_checkpoint_names_tail(image, used, capacity, bytes);
    current_tail = mcc_checkpoint_names_tail(current, used, capacity, bytes);
    if (!tail || !current_tail || memcmp(tail, "MCCN", 4) ||
        mcc_checkpoint_word(tail + 4) != 1 || mcc_checkpoint_word(tail + 8) != bytes ||
        mcc_checkpoint_word(tail + 12) != mcc_checkpoint_checksum(tail + 16, bytes) ||
        !mcc_object_names_validate(tail + 16, bytes)) return 0;
    array = mcc_checkpoint_incoming(image, current, used, object_header_data, sizeof(*array));
    if (!array || !array->valid || array->size != sizeof(*headers) || array->count < 0 ||
        array->count > 8192 || array->count > array->maximum_count) return 0;
    headers = mcc_checkpoint_incoming(image, current, used, array->data,
        (unsigned long)array->count * sizeof(*headers));
    if (!headers) return 0;
    for (i = 0; i < bytes; i += 4) {
        uint32_t handle = mcc_checkpoint_word(tail + 16 + i), index = handle & 0xFFFFu;
        struct object_header_datum const *object;
        if (handle == 0xFFFFFFFFu) continue;
        if (index >= (unsigned)array->count) return 0;
        object = headers + index;
        if (!(uint16_t)object->identifier || (uint16_t)object->identifier != (uint16_t)(handle >> 16) ||
            object->type >= 12 ||
            !mcc_checkpoint_incoming(image, current, used, object->datum, sizeof(struct object_datum))) return 0;
    }
    return mcc_checkpoint_names_claim(current_tail, bytes + 16);
}

/* Version 3 uses the footer's last eight previously reserved bytes for the
 * lighting value and a footer checksum. Keep every arena offset unchanged:
 * growing the payload reservation would relocate saved thread pointers. */
static int mcc_checkpoint_campaign(unsigned char const *payload, unsigned long bytes,
    unsigned char const *footer, int restore)
{
    unsigned char state[MCC_CAMPAIGN_SNAPSHOT_BYTES];
    if (mcc_checkpoint_word(footer+4)==3) {
        if (bytes!=MCC_CHECKPOINT_CAMPAIGN_BYTES || sizeof(state)!=bytes+4) return 0;
        memcpy(state,payload,bytes);
        memcpy(state+bytes,footer+88,4);
        payload=state;bytes+=4;
    }
    return restore ? mcc_campaign_restore(payload,bytes) : mcc_campaign_validate(payload,bytes);
}

int mcc_checkpoint_capture(void *image, unsigned long total,
    unsigned long used, unsigned long capacity)
{
    struct game_state_header const *header = image;
    unsigned char *tail, *footer;
    uint32_t bytes;
    unsigned char campaign[MCC_CAMPAIGN_SNAPSHOT_BYTES];
    if (!mcc_cache_tags_loaded()) return 1;
    tail = mcc_checkpoint_tail(image, total, used, capacity);
    if (!tail || !mcc_checkpoint_identity(header) || !mcc_checkpoint_claim(tail) ||
        !mcc_checkpoint_names_capture(image, used, capacity) || !mcc_script_threads_capture()) goto failed;
    bytes = mcc_grenades_snapshot(NULL, 0);
    if (bytes < 8 || bytes > MCC_CHECKPOINT_PAYLOAD_LIMIT) goto failed;
    footer = tail + MCC_CHECKPOINT_TOTAL_PAYLOAD;
    /* Invalidate the previous footer before any partial payload write. */
    memset(footer, 0, MCC_CHECKPOINT_FOOTER_BYTES);
    if (mcc_grenades_snapshot(tail, bytes) != bytes || !mcc_grenades_snapshot_validate(tail, bytes)) goto failed;
    mcc_campaign_snapshot(campaign);
    if (sizeof(campaign)!=MCC_CHECKPOINT_CAMPAIGN_BYTES+4 ||
        !mcc_campaign_validate(campaign,sizeof(campaign))) goto failed;
    memcpy(tail+bytes,campaign,MCC_CHECKPOINT_CAMPAIGN_BYTES);
    memcpy(footer+88,campaign+MCC_CHECKPOINT_CAMPAIGN_BYTES,4);
    bytes += MCC_CHECKPOINT_CAMPAIGN_BYTES;
    memcpy(footer, "MCCS", 4);
    mcc_checkpoint_put(footer + 4, 3);
    mcc_checkpoint_put(footer + 8, bytes);
    mcc_checkpoint_put(footer + 12, mcc_checkpoint_checksum(tail, bytes));
    mcc_checkpoint_put(footer + 16, (uint32_t)header->cache_file_checksum);
    mcc_checkpoint_put(footer + 20, MCC_CHECKPOINT_CAMPAIGN_BYTES);
    memcpy(footer + 24, header->map_name, strlen(header->map_name) + 1);
    mcc_checkpoint_put(footer+92,mcc_checkpoint_checksum(footer,92));
    return 1;
failed:
    error(_error_silent, "mcc checkpoint: the extra state could not be captured in unused CPU memory");
    return 0;
}

int mcc_checkpoint_validate(void const *image, unsigned long total,
    unsigned long used, unsigned long capacity, void const *current_image)
{
    struct game_state_header const *header = image, *current = current_image;
    unsigned char *tail, *current_tail, *footer;
    uint32_t bytes, campaign_bytes, version;
    if (!mcc_cache_tags_loaded()) return 1;
    tail = mcc_checkpoint_tail(image, total, used, capacity);
    current_tail = mcc_checkpoint_tail(current_image, total, used, capacity);
    if (!tail || !current_tail || !mcc_checkpoint_identity(header) ||
        !mcc_checkpoint_identity(current) || strcmp(header->map_name, current->map_name) ||
        header->cache_file_checksum != current->cache_file_checksum) goto failed;
    footer = tail + MCC_CHECKPOINT_TOTAL_PAYLOAD;
    bytes = mcc_checkpoint_word(footer + 8);
    version = mcc_checkpoint_word(footer + 4);
    campaign_bytes = mcc_checkpoint_word(footer + 20);
    /* The footer stays at the same CPU address. Version 1 had only inventory;
     * those maps could not use these formerly unsupported campaign functions. */
    if (version == 1) tail += MCC_CHECKPOINT_CAMPAIGN_BYTES;
    else if (version == 2 && (campaign_bytes == MCC_CAMPAIGN_SNAPSHOT_V1_BYTES ||
        campaign_bytes == MCC_CAMPAIGN_SNAPSHOT_V4_BYTES))
        tail += MCC_CHECKPOINT_CAMPAIGN_BYTES - campaign_bytes;
    if (memcmp(footer, "MCCS", 4) || (version != 1 && version != 2 && version != 3) ||
        (version == 1 ? campaign_bytes != 0 :
            (campaign_bytes != MCC_CHECKPOINT_CAMPAIGN_BYTES && campaign_bytes != MCC_CAMPAIGN_SNAPSHOT_V4_BYTES &&
             campaign_bytes != MCC_CAMPAIGN_SNAPSHOT_V1_BYTES)) ||
        bytes < 8 + campaign_bytes || bytes - campaign_bytes > MCC_CHECKPOINT_PAYLOAD_LIMIT ||
        mcc_checkpoint_word(footer + 12) != mcc_checkpoint_checksum(tail, bytes) ||
        mcc_checkpoint_word(footer + 16) != (uint32_t)header->cache_file_checksum ||
        (version==3 ? (campaign_bytes!=MCC_CHECKPOINT_CAMPAIGN_BYTES ||
            mcc_checkpoint_word(footer+92)!=mcc_checkpoint_checksum(footer,92)) :
            (mcc_checkpoint_word(footer+88) || mcc_checkpoint_word(footer+92))) ||
        !memchr(footer + 24, 0, 64) ||
        strcmp((char const *)footer + 24, header->map_name) ||
        !mcc_grenades_snapshot_validate(tail, bytes - campaign_bytes) ||
        (campaign_bytes && !mcc_checkpoint_campaign(tail + bytes - campaign_bytes, campaign_bytes, footer, 0)) ||
        !mcc_checkpoint_units(image, current_image, used, tail) ||
        !mcc_checkpoint_names_validate(image, current_image, used, capacity) ||
        !mcc_script_threads_validate(image,current_image,used,capacity) ||
        !mcc_checkpoint_claim(current_tail)) goto failed;
    return 1;
failed:
    error(_error_silent, "mcc checkpoint: missing, mismatched or damaged extra state; image refused");
    return 0;
}

int mcc_checkpoint_restore(void const *image, unsigned long total,
    unsigned long used, unsigned long capacity)
{
    unsigned char *tail, *footer;
    uint32_t bytes, campaign_bytes, version;
    if (!mcc_cache_tags_loaded()) return 1;
    if (!mcc_checkpoint_validate(image, total, used, capacity, image) ||
        !mcc_script_threads_restore((void *)image,used,capacity)) return 0;
    if (mcc_object_names_active()) {
        unsigned long names_bytes = mcc_object_names_snapshot(NULL);
        unsigned char *names = mcc_checkpoint_names_tail(image, used, capacity, names_bytes);
        if (!mcc_object_names_restore(names + 16, names_bytes)) return 0;
    }
    tail = mcc_checkpoint_tail(image, total, used, capacity);
    footer=tail+MCC_CHECKPOINT_TOTAL_PAYLOAD;
    bytes = mcc_checkpoint_word(footer + 8);
    campaign_bytes = mcc_checkpoint_word(tail + MCC_CHECKPOINT_TOTAL_PAYLOAD + 20);
    version = mcc_checkpoint_word(tail + MCC_CHECKPOINT_TOTAL_PAYLOAD + 4);
    if (version == 1) tail += MCC_CHECKPOINT_CAMPAIGN_BYTES;
    else if (version == 2 && (campaign_bytes == MCC_CAMPAIGN_SNAPSHOT_V1_BYTES ||
        campaign_bytes == MCC_CAMPAIGN_SNAPSHOT_V4_BYTES))
        tail += MCC_CHECKPOINT_CAMPAIGN_BYTES - campaign_bytes;
    if (mcc_grenades_restore(tail, bytes - campaign_bytes)) {
        if (campaign_bytes) return mcc_checkpoint_campaign(tail + bytes - campaign_bytes, campaign_bytes, footer, 1);
        mcc_campaign_begin();
        return 1;
    }
    error(_error_silent, "mcc checkpoint: grenade records do not match restored units");
    return 0;
}

void mcc_checkpoint_dispose(void)
{
    if (mcc_checkpoint_names_owner.original) {
        memcpy(mcc_checkpoint_names_owner.address, mcc_checkpoint_names_owner.original, mcc_checkpoint_names_owner.bytes);
        free(mcc_checkpoint_names_owner.original);
    }
    memset(&mcc_checkpoint_names_owner, 0, sizeof(mcc_checkpoint_names_owner));
    if (mcc_checkpoint_owner.original) {
        memcpy(mcc_checkpoint_owner.address, mcc_checkpoint_owner.original, MCC_CHECKPOINT_SLOT_BYTES);
        free(mcc_checkpoint_owner.original);
    }
    memset(&mcc_checkpoint_owner, 0, sizeof(mcc_checkpoint_owner));
}
