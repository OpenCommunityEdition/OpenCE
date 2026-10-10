#ifndef MCC_VORBIS_H
#define MCC_VORBIS_H
#include <stdint.h>

/* MCC-owned decoder. On success release *samples with mcc_vorbis_free(): callers
 * using cseries' tracked allocator must not pass it to their free macro. */
int mcc_vorbis_decode(unsigned char const *bytes, uint32_t size, uint32_t frame_limit,
    short **samples, uint32_t *frames, uint32_t *rate, int *channels);
void mcc_vorbis_free(short *samples);

#endif
