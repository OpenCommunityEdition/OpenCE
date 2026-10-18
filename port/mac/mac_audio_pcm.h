#ifndef HALO_MAC_AUDIO_PCM_H
#define HALO_MAC_AUDIO_PCM_H
#include <stddef.h>
#include <stdint.h>
/* Decode complete little-endian PCM16 mono/stereo frames into aligned native
   shorts. Output must not overlap source. Failure leaves output untouched and
   sets frames to zero. A zero-byte packet is valid with supported channels. */
int mac_audio_pcm16le_decode(const void *source,size_t bytes,unsigned channels,
    int16_t *output,size_t output_samples,size_t *frames);
#endif
