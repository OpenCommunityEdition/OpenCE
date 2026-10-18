#ifndef HALO_MAC_VOICE_DSP_H
#define HALO_MAC_VOICE_DSP_H
/* The microphone's samples on their way to voice chat's encoder: a ring of
   mono float samples (written by CoreAudio's thread, read by the game's) and
   a streaming resampler from the device's rate to the encoder's. No
   CoreAudio in here, so the host's tests build it. */
#include <pthread.h>
#include <stddef.h>

/* the encoder's rate (voice_audio.c's VOICE_RATE) */
#define MAC_VOICE_RATE 48000
/* the ring holds one second; a write that does not fit drops the oldest */
#define MAC_VOICE_RING_SAMPLES 48000

struct mac_voice_ring {
    float data[MAC_VOICE_RING_SAMPLES];
    unsigned read, count;
    pthread_mutex_t lock;
};
int mac_voice_ring_init(struct mac_voice_ring *);
void mac_voice_ring_destroy(struct mac_voice_ring *);
void mac_voice_ring_clear(struct mac_voice_ring *);
/* any thread */
void mac_voice_ring_write(struct mac_voice_ring *, const float *samples, unsigned count);
/* up to maximum samples, the oldest first; how many */
unsigned mac_voice_ring_read(struct mac_voice_ring *, float *samples, unsigned maximum);
/* drops the oldest so that at most keep remain; how many were dropped */
unsigned mac_voice_ring_trim(struct mac_voice_ring *, unsigned keep);
unsigned mac_voice_ring_available(struct mac_voice_ring *);

/* in_rate to out_rate, mono, in chunks of any size. Above the output rate
   each output sample is the mean of the input's span it covers; at or below,
   it is interpolated between neighbours. */
struct mac_voice_resampler {
    double step;        /* input samples per output sample */
    double position;    /* the next output sample's place in the chunk (linear) */
    float previous;     /* the last sample of the chunk before (linear) */
    double sum, filled; /* the cell being averaged (averaging) */
    int averaging;
};
/* 0 for a rate that is not in 8000 to 192000 */
int mac_voice_resampler_init(struct mac_voice_resampler *, double in_rate, double out_rate);
/* the samples written to out (at most maximum; the input past what fits is
   lost, so give room for in_count / step + 2) */
unsigned mac_voice_resampler_process(struct mac_voice_resampler *, const float *in, unsigned in_count,
    float *out, unsigned maximum);
#endif
