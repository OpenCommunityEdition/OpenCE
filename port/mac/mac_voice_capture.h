#ifndef HALO_MAC_VOICE_CAPTURE_H
#define HALO_MAC_VOICE_CAPTURE_H
/* The microphone for voice chat: a CoreAudio AUHAL input unit (Mac OS X 10.4
   and later, PowerPC and Intel). It delivers mono float samples at 48 kHz,
   native-endian, in a ring that the game's thread reads. Nothing is opened,
   and so no permission asked, until mac_voice_capture_open. */
struct mac_voice_capture { void *state; };
/* Opens the input device of this name (audio.input_device, the names of
   engine_audio_devices.c's list; NULL, "" or "default", or a name no device
   has, is the system's default input) and starts it. Zero-initialize capture.
   Returns nonzero when it runs; status is the failing CoreAudio error. */
int mac_voice_capture_open(struct mac_voice_capture *,const char *device,long *status);
/* Up to maximum samples, the oldest first; how many */
unsigned mac_voice_capture_read(struct mac_voice_capture *,float *samples,unsigned maximum);
unsigned mac_voice_capture_available(struct mac_voice_capture *);
/* Drops the oldest, keeping at most keep; how many were dropped */
unsigned mac_voice_capture_trim(struct mac_voice_capture *,unsigned keep);
/* The device's rate and channels (diagnostic) */
int mac_voice_capture_format(struct mac_voice_capture *,double *rate,unsigned *channels);
/* Stops and closes it; safe on one that is not open */
void mac_voice_capture_close(struct mac_voice_capture *);
#endif
