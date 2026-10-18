#ifndef HALO_MAC_AUDIO_OUTPUT_H
#define HALO_MAC_AUDIO_OUTPUT_H
#include <stdint.h>
/* Native-endian interleaved float32 stereo, 48000 frames/second. Called on the
   AudioUnit render thread; fill all frames and return nonzero, or zero for
   silence. Do not allocate, block on the owner thread, or call output lifecycle
   functions from this callback. The caller owns context until close returns. */
typedef int (*mac_audio_fill_proc)(void *context,float *stereo,uint32_t frames);
struct mac_audio_output { void *state; };
struct mac_audio_output_stats {
    uint64_t callbacks,frames,nonzero_frames,silent_failures;
    uint32_t format_errors;
};
/* Owner-thread lifecycle. Zero-initialize output. Opens the default output
   only, without starting it. This is not a DirectSound implementation/mixer. */
int mac_audio_output_open(struct mac_audio_output *,mac_audio_fill_proc,void *,long *status);
/* port: the same on the output device of this name (audio.output_device, the
   names of engine_audio_devices.c's list); NULL, "" or "default", or a name
   no device has, is the system's default output */
int mac_audio_output_open_device(struct mac_audio_output *,mac_audio_fill_proc,void *,long *status,const char *device);
int mac_audio_output_start(struct mac_audio_output *,long *status);
int mac_audio_output_stop(struct mac_audio_output *,long *status);
int mac_audio_output_statistics(struct mac_audio_output *,struct mac_audio_output_stats *);
/* Failure retains the wrapper/context for a later retry; do not free context. */
int mac_audio_output_close(struct mac_audio_output *,long *status);
#endif
