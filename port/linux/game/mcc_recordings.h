/* MCC recording conversion; the established playback codecs stay unchanged. */
#ifndef MCC_RECORDINGS_H
#define MCC_RECORDINGS_H
#include <stdint.h>

/* Validate the complete stream before changing any bytes. Invalid weapon
 * selections become NONE (keep the unit's current weapon). */
int mcc_recording_prepare(unsigned char *stream, uint32_t size,
    unsigned version, unsigned control_version, uint32_t *corrected);

#endif
