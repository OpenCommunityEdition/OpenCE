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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MCC_CHECKPOINT_PAYLOAD_LIMIT (8u + 8192u * 8u)
#define MCC_CHECKPOINT_TOTAL_PAYLOAD (MCC_CHECKPOINT_PAYLOAD_LIMIT + MCC_CAMPAIGN_SNAPSHOT_BYTES)
#define MCC_CHECKPOINT_FOOTER_BYTES 96u
#define MCC_CHECKPOINT_SLOT_BYTES (MCC_CHECKPOINT_TOTAL_PAYLOAD + MCC_CHECKPOINT_FOOTER_BYTES)

static struct {
    unsigned char *original;
    unsigned char *address;
} mcc_checkpoint_owner;

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
    uint32_t result = 0xFFFFFFFFu, i, bit;
    for (i = 0; i < bytes; ++i) {
        result ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            result = (result >> 1) ^ (0xEDB88320u & (0u - (result & 1u)));
    }
    return ~result;
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
        if ((uint16_t)object->identifier != (uint16_t)(handle >> 16) ||
            object->type >= 32 || !(_object_mask_unit & (1u << object->type)) ||
            !mcc_checkpoint_incoming(image, current, used, object->datum, sizeof(struct object_datum)) ||
            (record[4] && mcc_grenades_type_count() < 3) ||
            (record[5] && mcc_grenades_type_count() < 4)) return 0;
    }
    return 1;
}

int mcc_checkpoint_capture(void *image, unsigned long total,
    unsigned long used, unsigned long capacity)
{
    struct game_state_header const *header = image;
    unsigned char *tail, *footer;
    uint32_t bytes;
    if (!mcc_cache_tags_loaded()) return 1;
    tail = mcc_checkpoint_tail(image, total, used, capacity);
    if (!tail || !mcc_checkpoint_identity(header) || !mcc_checkpoint_claim(tail)) goto failed;
    bytes = mcc_grenades_snapshot(NULL, 0);
    if (bytes < 8 || bytes > MCC_CHECKPOINT_PAYLOAD_LIMIT) goto failed;
    footer = tail + MCC_CHECKPOINT_TOTAL_PAYLOAD;
    /* Invalidate the previous footer before any partial payload write. */
    memset(footer, 0, MCC_CHECKPOINT_FOOTER_BYTES);
    if (mcc_grenades_snapshot(tail, bytes) != bytes || !mcc_grenades_snapshot_validate(tail, bytes)) goto failed;
    mcc_campaign_snapshot(tail + bytes);
    if (!mcc_campaign_validate(tail + bytes, MCC_CAMPAIGN_SNAPSHOT_BYTES)) goto failed;
    bytes += MCC_CAMPAIGN_SNAPSHOT_BYTES;
    memcpy(footer, "MCCS", 4);
    mcc_checkpoint_put(footer + 4, 2);
    mcc_checkpoint_put(footer + 8, bytes);
    mcc_checkpoint_put(footer + 12, mcc_checkpoint_checksum(tail, bytes));
    mcc_checkpoint_put(footer + 16, (uint32_t)header->cache_file_checksum);
    mcc_checkpoint_put(footer + 20, MCC_CAMPAIGN_SNAPSHOT_BYTES);
    memcpy(footer + 24, header->map_name, strlen(header->map_name) + 1);
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
    if (version == 1) tail += MCC_CAMPAIGN_SNAPSHOT_BYTES;
    if (memcmp(footer, "MCCS", 4) || (version != 1 && version != 2) ||
        (version == 1 ? campaign_bytes != 0 : campaign_bytes != MCC_CAMPAIGN_SNAPSHOT_BYTES) ||
        bytes < 8 + campaign_bytes || bytes - campaign_bytes > MCC_CHECKPOINT_PAYLOAD_LIMIT ||
        mcc_checkpoint_word(footer + 12) != mcc_checkpoint_checksum(tail, bytes) ||
        mcc_checkpoint_word(footer + 16) != (uint32_t)header->cache_file_checksum ||
        mcc_checkpoint_word(footer + 88) ||
        mcc_checkpoint_word(footer + 92) || !memchr(footer + 24, 0, 64) ||
        strcmp((char const *)footer + 24, header->map_name) ||
        !mcc_grenades_snapshot_validate(tail, bytes - campaign_bytes) ||
        (campaign_bytes && !mcc_campaign_validate(tail + bytes - campaign_bytes, campaign_bytes)) ||
        !mcc_checkpoint_units(image, current_image, used, tail) ||
        !mcc_checkpoint_claim(current_tail)) goto failed;
    return 1;
failed:
    error(_error_silent, "mcc checkpoint: missing, mismatched or damaged extra state; image refused");
    return 0;
}

int mcc_checkpoint_restore(void const *image, unsigned long total,
    unsigned long used, unsigned long capacity)
{
    unsigned char *tail;
    uint32_t bytes, campaign_bytes, version;
    if (!mcc_cache_tags_loaded()) return 1;
    if (!mcc_checkpoint_validate(image, total, used, capacity, image)) return 0;
    tail = mcc_checkpoint_tail(image, total, used, capacity);
    bytes = mcc_checkpoint_word(tail + MCC_CHECKPOINT_TOTAL_PAYLOAD + 8);
    campaign_bytes = mcc_checkpoint_word(tail + MCC_CHECKPOINT_TOTAL_PAYLOAD + 20);
    version = mcc_checkpoint_word(tail + MCC_CHECKPOINT_TOTAL_PAYLOAD + 4);
    if (version == 1) tail += MCC_CAMPAIGN_SNAPSHOT_BYTES;
    if (mcc_grenades_restore(tail, bytes - campaign_bytes)) {
        if (campaign_bytes) return mcc_campaign_restore(tail + bytes - campaign_bytes, campaign_bytes);
        mcc_campaign_begin();
        return 1;
    }
    error(_error_silent, "mcc checkpoint: grenade records do not match restored units");
    return 0;
}

void mcc_checkpoint_dispose(void)
{
    if (mcc_checkpoint_owner.original) {
        memcpy(mcc_checkpoint_owner.address, mcc_checkpoint_owner.original, MCC_CHECKPOINT_SLOT_BYTES);
        free(mcc_checkpoint_owner.original);
    }
    memset(&mcc_checkpoint_owner, 0, sizeof(mcc_checkpoint_owner));
}
