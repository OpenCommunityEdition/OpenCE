/* MCC metadata normalization. All writes are to an MCC-owned, checked slice;
 * the original map and both established format implementations are untouched. */
#include "cseries.h"
#include "errors.h"
#include "mcc_runtime.h"
#include "mcc_recordings.h"
#include <string.h>

static uint32_t mcc_word(void const *p)
{
    uint32_t value;
    memcpy(&value, p, 4);
    return value;
}

static void *mcc_block(struct mcc_runtime *r, unsigned char *p, uint32_t stride, uint32_t *count)
{
    *count = mcc_word(p);
    if (*count > r->used / stride) return NULL;
    return *count ? mcc_runtime_pointer(r, mcc_word(p + 4), *count * stride) : NULL;
}

static void mcc_hud_placement(unsigned char *p)
{
    uint16_t flags;
    float x, y;
    memcpy(&flags, p + 12, 2);
    if (!(flags & 4)) return;
    memcpy(&x, p + 4, 4);
    memcpy(&y, p + 8, 4);
    x *= 0.5f;
    y *= 0.5f;
    memcpy(p + 4, &x, 4);
    memcpy(p + 8, &y, 4);
    flags &= ~4u;
    memcpy(p + 12, &flags, 2);
}

static int mcc_scenario_recordings(struct mcc_runtime *r, unsigned char *scenario)
{
    uint32_t count, i;
    unsigned char *recordings = mcc_block(r, scenario + 0x36C, 0x40, &count);
    if (count && !recordings) return FALSE;
    for (i = 0; i < count; ++i) {
        unsigned char *recording = recordings + i * 0x40;
        uint32_t size = mcc_word(recording + 0x2C), corrected;
        unsigned char *stream = mcc_runtime_pointer(r, mcc_word(recording + 0x38), size);
        /* Preserve an explicitly disabled, empty recording. */
        if (!recording[0x20] && !recording[0x24] && !recording[0x25] &&
            !size && !mcc_word(recording + 0x38)) continue;
        if (!mcc_recording_prepare(stream, size, recording[0x20], recording[0x22], &corrected)) {
            error(_error_silent, "mcc: recorded animation #%lu has an unsupported or damaged control stream",
                (unsigned long)i);
            return FALSE;
        }
        if (corrected) error(_error_silent,
            "mcc: recorded animation #%lu: %lu invalid weapon selections changed to NONE",
            (unsigned long)i, (unsigned long)corrected);
    }
    return TRUE;
}

static int mcc_scenario_cameras(struct mcc_runtime *r, unsigned char *scenario)
{
    uint32_t count, i;
    unsigned char *points = mcc_block(r, scenario + 0x4F0, 0x68, &count);
    float const native_maximum = 1.5707963267948966f;
    if (count && !points) return FALSE;
    for (i = 0; i < count; ++i) {
        unsigned char *field = points + i * 0x68 + 0x40;
        float angle;
        memcpy(&angle, field, 4);
        /* Zero selects the native default. Wide MCC cinematic lenses are
         * valid authored data, but the native observer both validates and
         * clamps to 90 degrees. Normalize the MCC copy before commands reach
         * it; the Xbox/CE camera and its validation remain unchanged. */
        if (angle == 0.0f) continue;
        if (!(angle >= 0.001f && angle < 3.1415926535897932f)) {
            error(_error_silent, "mcc: cutscene camera #%lu has an invalid field of view",
                (unsigned long)i);
            return FALSE;
        }
        if (angle > native_maximum) {
            memcpy(field, &native_maximum, 4);
            error(_error_silent, "mcc: cutscene camera #%lu: field of view limited to 90 degrees",
                (unsigned long)i);
        }
    }
    return TRUE;
}

static int mcc_hud(struct mcc_runtime *r, uint32_t group, unsigned char *p)
{
    unsigned char *block;
    uint32_t count, i;
    if (group == 'unhi') {
        static unsigned const placements[] = {0x24, 0x8C, 0xF4, 0x17C, 0x1E4, 0x26C, 0x2D4};
        for (i = 0; i < NUMBEROF(placements); ++i) mcc_hud_placement(p + placements[i]);
        memset(p + 0xF4 + 0x58, 0, 12);
        memset(p + 0x1E4 + 0x58, 0, 12);
        block = mcc_block(r, p + 0x3A4, 0x84, &count);
        if (count && !block) return FALSE;
        for (i = 0; i < count; ++i) mcc_hud_placement(block + i * 0x84);
        block = mcc_block(r, p + 0x3CC, 0x144, &count);
        if (count && !block) return FALSE;
        for (i = 0; i < count; ++i) {
            mcc_hud_placement(block + i * 0x144 + 0x14);
            mcc_hud_placement(block + i * 0x144 + 0x7C);
            memset(block + i * 0x144 + 0x7C + 0x58, 0, 12);
        }
    } else if (group == 'wphi') {
        unsigned k;
        for (k = 0; k < 2; ++k) {
            block = mcc_block(r, p + 0x60 + 12 * k, 0xB4, &count);
            if (count && !block) return FALSE;
            for (i = 0; i < count; ++i) {
                mcc_hud_placement(block + i * 0xB4 + 0x24);
                if (k) memset(block + i * 0xB4 + 0x24 + 0x58, 0, 12);
            }
        }
    } else if (group == 'grhi') {
        mcc_hud_placement(p + 0x24);
        mcc_hud_placement(p + 0x8C);
    } else if (group == 'hudg') {
        mcc_hud_placement(p + 0x24);
    }
    return TRUE;
}

int mcc_tags_prepare(struct mcc_runtime *r)
{
    uint32_t i;
    for (i = 0; i < r->report.tag_count; ++i) {
        unsigned char *entry = r->tag_index + i * 32;
        uint32_t group = mcc_word(entry), address = mcc_word(entry + 20), size = 0;
        unsigned char *p;
        short shader = -1;
        switch (group) {
        case 'senv': shader = 3; break;
        case 'soso': shader = 4; break;
        case 'sotr': shader = 5; break;
        case 'schi': case 'scex': shader = 6; break;
        case 'swat': shader = 7; break;
        case 'sgla': shader = 8; break;
        case 'smet': shader = 9; break;
        case 'spla': shader = 10; break;
        case 'unhi': size = 0x56C; break;
        case 'wphi': size = 0x17C; break;
        case 'grhi': size = 0x1F8; break;
        case 'hudg': size = 0x450; break;
        case 'weap': size = 0x508; break;
        case 'DeLa': size = 0x60; break;
        case 'scnr': size = 0x5B0; break;
        default: break;
        }
        if (shader >= 0) size = group == 'scex' ? 0x78 : 0x28;
        if (!size) continue;
        p = mcc_runtime_pointer(r, address, size);
        if (!p) {
            error(_error_silent, "mcc: metadata for tag %lu is outside the tag data", (unsigned long)i);
            return FALSE;
        }
        if (shader >= 0) memcpy(p + 0x24, &shader, 2);
        if (group == 'scnr' && (!mcc_scenario_recordings(r, p) || !mcc_scenario_cameras(r, p))) return FALSE;
        if (group == 'scex') {
            uint32_t flags = mcc_word(p + 0x6C), new_group = 'schi';
            if (!mcc_word(p + 0x54)) memcpy(p + 0x54, p + 0x60, 12);
            memset(p + 0x60, 0, 12);
            memcpy(p + 0x60, &flags, 4);
            memcpy(entry, &new_group, 4);
        }
        if (group == 'weap') {
            unsigned j;
            for (j = 0; j < 4; ++j) {
                uint16_t value;
                memcpy(&value, p + 0x330 + j * 2, 2);
                if (value == 17 || value == 18) {
                    value -= 2;
                    memcpy(p + 0x330 + j * 2, &value, 2);
                }
            }
        }
        if (group == 'DeLa') {
            uint32_t count;
            unsigned char *events = mcc_block(r, p + 0x54, 0x48, &count);
            if (count && !events) return FALSE;
            /* The first 102 callback IDs retain their Xbox meanings. MCC's
             * additional callbacks are interpreted by mcc_ui.c only for an
             * MCC-owned widget. Preserve the handlers, including mouse input,
             * confirmation actions, scripts and their navigation flags. */
        }
        if (!mcc_hud(r, group, p)) return FALSE;
    }
    return TRUE;
}
