/* Independently written MCC cache normalization. Solar Flare M1's pilot
 * recordings contain 0xFF00 in the initial weapon field. Passing that signed
 * short (-256) to the Xbox interpreter indexes outside the unit's inventory.
 * There is no valid slot to translate it to: retain the current weapon instead.
 * Apply the same rule to later selection events, without changing their timing,
 * movement or flags. This runs only on the MCC loader's private tag buffer. */
#include "mcc_recordings.h"

static unsigned mcc_recording_word(unsigned char const *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static uint32_t mcc_recording_weapon(unsigned char *p, int write)
{
    unsigned value = mcc_recording_word(p);
    /* Xbox units have four slots; 0xFFFF requests no selection change. */
    if (value < 4 || value == 0xFFFF) return 0;
    if (write) p[0] = p[1] = 0xFF;
    return 1;
}

static int mcc_recording_walk(unsigned char *stream, uint32_t size,
    unsigned version, unsigned control_version, int write, uint32_t *corrected)
{
    uint32_t offset = 52, changes;
    unsigned i;
    if (!stream || version < 1 || version > 4 || control_version > 4) return 0;
    for (i = 1; i < control_version; ++i) offset += i == 1 ? 4 : 2;
    if (version == 4) offset += 12; /* facing/aiming/looking angle state */
    if (offset > size) return 0;
    changes = mcc_recording_weapon(stream + 4, write);
    while (offset < size) {
        uint32_t header_size, payload_size;
        unsigned type;
        if (version == 4) {
            unsigned time = stream[offset] & 3;
            type = stream[offset] >> 2;
            header_size = time < 2 ? 1 : time;
            payload_size = type < 2 ? 0 : type < 4 ? 1 : type < 6 ? 2 :
                type == 6 ? 8 : type < 15 ? 2 : 4;
        } else {
            if (size - offset < 4) return 0;
            type = mcc_recording_word(stream + offset);
            header_size = 4;
            payload_size = type < 2 || type == 7 || type == 8 ? 0 :
                type < 6 ? 2 : type == 6 || type >= 16 ? 8 : 12;
        }
        if (type >= 23 || header_size > size - offset) return 0;
        offset += header_size;
        if (payload_size > size - offset) return 0;
        if (type == 1) {
            *corrected = changes;
            return 1;
        }
        if (type == 5) changes += mcc_recording_weapon(stream + offset, write);
        offset += payload_size;
    }
    return 0; /* missing end event */
}

int mcc_recording_prepare(unsigned char *stream, uint32_t size,
    unsigned version, unsigned control_version, uint32_t *corrected)
{
    uint32_t changes;
    *corrected = 0;
    if (!mcc_recording_walk(stream, size, version, control_version, 0, &changes)) return 0;
    if (changes) return mcc_recording_walk(stream, size, version, control_version, 1, corrected);
    return 1;
}
